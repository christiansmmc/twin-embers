/* A PC box as a page (3ds_poketouch.h CtrPokeTouchBox; phase 2, FireRed's own arrangement at 1:1), included at the end
 * of 3ds_poketouch.c after the naming screen, whose FireRed text it shares, and the Pokedex, whose picture helpers it
 * shares. A remote for FireRed's own box: its wallpaper at 1:1 in the box's window (the box's name between the arrows on
 * its header, the thirty places on its body, each Pokemon's icon over its place, FireRed's cursor a white place),
 * PARTY and CLOSE BOX beside it over the picture of the Pokemon under the cursor, the Pokemon (or MOVE ITEMS' item) in
 * hand over the cursor, the Party's panel in the box's window while the cursor is in it; the box's menu, YES / NO and
 * marks in the Party's window at the bottom right, the box chooser as three cells over the wallpaper. The band says
 * what the box says (a message that waits for A takes it whole), else the lines FireRed shows about the Pokemon under
 * the cursor (MOVE ITEMS: the item it holds). Every word is FireRed's (CtrBottomStorage_BoxFill) but for the cells'
 * labels (3ds_ui_strings.h). */
#include "3ds_storage_touch.h"

/* The rows of the menu-like window the box has open: its menu, YES / NO, or the marks with OK and CANCEL. */
static int FXMenuRows(const CtrPokeTouchBox *b)
{
    int n = b->state == PT_BOX_STATE_YESNO ? 2 : b->state == PT_BOX_STATE_MARKS ? 6 : b->menuCount;

    return n > PARTY_MENU_ROWS ? PARTY_MENU_ROWS : n;
}

static int FXMenuOpen(const CtrPokeTouchBox *b)
{
    return b->state == PT_BOX_STATE_MENU || b->state == PT_BOX_STATE_YESNO || b->state == PT_BOX_STATE_MARKS;
}

/* BACK (the box's B through ProcessBack) shows where nothing covers it: not under a menu, nor while a message waits. */
static const CtrPokeTouchRect *FXBackRect(const CtrPokeTouchView *v)
{
    return FXMenuOpen(&v->box) || v->box.state == PT_BOX_STATE_MESSAGE ? NULL : &kBoxBack;
}

int CtrPokeTouch_BoxHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchBox *b = &v->box;

    if (!b->on)
        return ST_HIT_NONE;
    switch (b->state) {
    case PT_BOX_STATE_MESSAGE:
        return x >= 0 && y >= 0 && x < W && y < H ? PT_BOX_HIT_MESSAGE : ST_HIT_NONE;   /* anywhere: it waits for A */
    case PT_BOX_STATE_MENU:
    case PT_BOX_STATE_YESNO:
    case PT_BOX_STATE_MARKS:
        for (int i = 0; i < FXMenuRows(b); ++i) {
            CtrPokeTouchRect r = PartyMenuRow(FXMenuRows(b), i);

            if (UiIn(&r, x, y))
                return ST_HIT_MENU_BASE + i;
        }
        return ST_HIT_NONE;
    case PT_BOX_STATE_CHOOSE:
        return UiIn(&kBoxChoosePrev, x, y) ? ST_HIT_PREV : UiIn(&kBoxChooseNext, x, y) ? ST_HIT_NEXT
             : UiIn(&kBoxChooseName, x, y) ? ST_HIT_TITLE : ST_HIT_NONE;
    case PT_BOX_STATE_BOX:
        break;
    default:
        return ST_HIT_NONE;
    }
    if (b->area == ST_AREA_PARTY) {
        if (UiIn(&kBoxLead, x, y))
            return ST_HIT_PARTY_BASE;
        for (int i = 1; i < 6; ++i) {
            CtrPokeTouchRect r = BoxPartyRow(i);

            if (UiIn(&r, x, y))
                return ST_HIT_PARTY_BASE + i;
        }
        return UiIn(&kBoxPartyCancel, x, y) ? ST_HIT_PARTY_BASE + 6 : ST_HIT_NONE;
    }
    if (UiIn(&kBoxPrev, x, y))
        return ST_HIT_PREV;
    if (UiIn(&kBoxNext, x, y))
        return ST_HIT_NEXT;
    if (UiIn(&kBoxTitle, x, y))
        return ST_HIT_TITLE;
    if (UiIn(&kBoxPartyButton, x, y))
        return ST_HIT_BUTTON_PARTY;
    if (UiIn(&kBoxClose, x, y))
        return ST_HIT_BUTTON_CLOSE;
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = BoxCell(i);

        if (UiIn(&r, x, y))
            return i;
    }
    return ST_HIT_NONE;
}

/* The box's wallpaper, FireRed's own picture of it (160x144) in the lower screen's colours; decoded once per wallpaper. */
static uint16_t sFXWall[160 * 144];
static int sFXWallId = -1, sFXWallOk;

static int FXWallpaper(unsigned id)
{
    static unsigned short raw[160 * 144];

    if ((int)id != sFXWallId) {
        sFXWallId = (int)id;
        sFXWallOk = CtrPokeTouch_Wallpaper(id, raw);
        for (int i = 0; sFXWallOk && i < 160 * 144; ++i) {
            unsigned c = raw[i];
            sFXWall[i] = GbaColor(c);
        }
    }
    return sFXWallOk;
}

/* A Pokemon's icon (FireRed's, its first frame), its 32x32 centred on (cx, cy). */
static void FXIcon(const CtrPokeTouchBoxMon *m, int cx, int cy)
{
    static unsigned char tiles[1024];
    static unsigned short pal[16];
    static uint8_t px[32 * 32];
    static uint16_t pal565[16];

    if (!m->species || !CtrPokeTouch_MonIcon(m->species, m->personality, m->egg, tiles, pal))
        return;
    FDTiles(tiles, 32, 32, px);
    FDPal(pal, pal565);
    UiImage(px, pal565, 32, 32, 32, cx - 16, cy - 16, 1, 1);
}

/* MOVE ITEMS: the tag of a Pokemon that holds an item, its 7x7 centred on (cx, cy), in FireRed's red. */
static void FXItemTag(int cx, int cy)
{
    Fill(cx - 3, cy - 3, 7, 7, KitInk(4));
    Fill(cx - 1, cy - 1, 3, 3, KitInk(1));
}

/* A FireRed text (codes, 0xff ended) on a line. */
static void FXLine(const uint8_t *raw, int n, int x, int lineTop, int limit)
{
    char text[64];

    FNText(raw, n, text, (int)sizeof(text));
    KitText(text, x, lineTop, limit);
}

/* The arrow's place among controls: the one held under a touch, else FireRed's cursor. */
static int FXChosen(const CtrPokeTouchBox *b, int code, int cursorHere)
{
    return b->pressed >= 0 ? b->pressed == code : cursorHere;
}

/* The box's window: the wallpaper at 1:1, the name between the arrows on its header (the arrow before the name while
 * FireRed's cursor is there), the places (FireRed's cursor a white place, one held light grey), the icons, MOVE ITEMS'
 * tags. Without a wallpaper the header is the light grey and the body white. */
static void FXBoxWindow(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    int ready = b->state != PT_BOX_STATE_BUSY, top = kBoxTitle.y + (kBoxTitle.h - KIT_LINE_H) / 2;
    char name[32];

    FPageWindow(&kBoxWin, v->frameType);
    if (FXWallpaper(b->wallpaper)) {
        for (int y = 0; y < 144; ++y)
            for (int x = 0; x < 160; ++x)
                Px(BOX_WALL_X + x, BOX_WALL_Y + y, sFXWall[y * 160 + x]);
    } else
        Fill(BOX_WALL_X, BOX_WALL_Y, 160, 24, KitInk(3));
    if (b->pressed == ST_HIT_PREV || b->pressed == ST_HIT_NEXT || b->pressed == ST_HIT_TITLE) {
        const CtrPokeTouchRect *r = b->pressed == ST_HIT_PREV ? &kBoxPrev : b->pressed == ST_HIT_NEXT ? &kBoxNext : &kBoxTitle;

        Fill(r->x, r->y, r->w, r->h, KitInk(3));
    }
    KitTriangle(kBoxPrev.x + kBoxPrev.w / 2, kBoxPrev.y + kBoxPrev.h / 2, 2, KitInk(2));
    KitTriangle(kBoxNext.x + kBoxNext.w / 2, kBoxNext.y + kBoxNext.h / 2, 3, KitInk(2));
    FNText(b->boxName, (int)sizeof(b->boxName), name, (int)sizeof(name));
    {
        int w = UiTextWidth(name, 1), x = kBoxTitle.x + (kBoxTitle.w - w) / 2;

        if (FXChosen(b, ST_HIT_TITLE, ready && b->area == ST_AREA_TITLE))
            KitText(KIT_ARROW, x - KIT_ARROW_W, top, x);
        KitText(name, x, top, kBoxTitle.x + kBoxTitle.w);
    }
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = BoxCell(i);

        if (b->pressed == i)
            Fill(r.x, r.y, r.w, r.h, KitInk(3));
        else if (ready && b->area == ST_AREA_BOX && b->pos == i) {
            Fill(r.x, r.y, r.w, r.h, KitInk(2));
            Fill(r.x + 1, r.y + 1, r.w - 2, r.h - 2, KitInk(1));
        }
    }
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = BoxCell(i);

        FXIcon(&b->slot[i], r.x + r.w / 2, r.y + r.h / 2);
        if (b->items && b->slot[i].species && b->slotItem[i])
            FXItemTag(r.x + r.w - 4, r.y + r.h - 4);
    }
}

/* The Party's panel in the box's window, as FireRed's: the lead's place on the left, five rows, CANCEL across; each
 * Pokemon's icon (its lines are in the band while the cursor is on it), FireRed's arrow on the cursor or the one held. */
static void FXPartyPanel(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;

    FPageWindow(&kBoxWin, v->frameType);
    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect r = i == 0 ? kBoxLead : BoxPartyRow(i);
        int cx = i == 0 ? r.x + r.w / 2 + 4 : r.x + KIT_ARROW_W + 4 + 16, cy = r.y + r.h / 2;

        Fill(r.x, r.y, r.w, r.h, b->pressed == ST_HIT_PARTY_BASE + i ? KitInk(3) : KitInk(1));
        if (FXChosen(b, ST_HIT_PARTY_BASE + i, b->pos == i))
            KitText(KIT_ARROW, r.x, cy - KIT_LINE_H / 2, r.x + KIT_ARROW_W);
        FXIcon(&b->party[i], cx, cy);
        if (b->items && b->party[i].species && b->partyItem[i])
            FXItemTag(cx + 12, cy + 10);
    }
    FPageRow(&kBoxPartyCancel, CtrUiText(UIS_CANCEL), FXChosen(b, ST_HIT_PARTY_BASE + 6, b->pos == 6),
             b->pressed == ST_HIT_PARTY_BASE + 6, 1);
}

/* PARTY and CLOSE BOX (the arrow while FireRed's cursor is on one, or it is held), and the picture of the Pokemon under
 * the cursor (an Egg: its icon); MOVE ITEMS: the item it holds under the picture. */
static void FXSide(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    const CtrPokeTouchRect *m = &kBoxMon;
    int buttons = b->state == PT_BOX_STATE_BOX && b->area == ST_AREA_BUTTONS;

    BagCell(&kBoxPartyButton, v->frameType, CtrUiText(UIS_BOX_PARTY),
            FXChosen(b, ST_HIT_BUTTON_PARTY, buttons && b->pos == 0), b->pressed == ST_HIT_BUTTON_PARTY, 1);
    BagCell(&kBoxClose, v->frameType, CtrUiText(UIS_BOX_CLOSE),
            FXChosen(b, ST_HIT_BUTTON_CLOSE, buttons && b->pos == 1), b->pressed == ST_HIT_BUTTON_CLOSE, 1);
    FPageWindow(m, v->frameType);
    if (b->mon.species && b->mon.egg)
        FXIcon(&b->mon, m->x + m->w / 2, m->y + m->h / 2);
    else if (b->mon.species)
        FDPicDraw(b->mon.species, m->x + (m->w - 64) / 2, m->y + (b->items ? 8 : (m->h - 64) / 2), 0);
    if (b->items && b->monItem)
        BItemDraw(b->monItem, m->x + 8, m->y + m->h - 8 - 24, 1);
}

/* The Pokemon (or MOVE ITEMS' item) in hand, over the cursor's place and a little above it, as FireRed's hand holds it. */
static void FXHand(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    CtrPokeTouchRect r;

    if (!b->holding && !(b->items && b->handItem))
        return;
    switch (b->area) {
    case ST_AREA_BOX:
        r = BoxCell(b->pos >= 0 && b->pos < PT_BOX_SLOTS ? b->pos : 0);
        break;
    case ST_AREA_PARTY:
        r = b->pos <= 0 ? kBoxLead : b->pos >= 6 ? kBoxPartyCancel : BoxPartyRow(b->pos);
        break;
    case ST_AREA_BUTTONS:
        r = b->pos == 1 ? kBoxClose : kBoxPartyButton;
        break;
    default:
        r = kBoxTitle;
        break;
    }
    if (b->items && b->handItem)
        BItemDraw(b->handItem, r.x + r.w / 2 - 12, r.y + r.h / 2 - 22, 1);
    else
        FXIcon(&b->held, r.x + r.w / 2, r.y + r.h / 2 - 10);
}

/* The box chooser over the wallpaper: its arrows and the box shown, with how many it holds. */
static void FXChoose(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    const CtrPokeTouchRect *n = &kBoxChooseName;
    char name[32], count[12];
    int top = n->y + (n->h - 2 * KIT_LINE_H) / 2;

    BagArrowCell(&kBoxChoosePrev, v->frameType, 2, 1, b->pressed == ST_HIT_PREV);
    BagArrowCell(&kBoxChooseNext, v->frameType, 3, 1, b->pressed == ST_HIT_NEXT);
    FPageWindow(n, v->frameType);
    if (b->pressed == ST_HIT_TITLE)
        Fill(n->x + 8, n->y + 8, n->w - 16, n->h - 16, KitInk(3));
    FNText(b->chooseName, (int)sizeof(b->chooseName), name, (int)sizeof(name));
    snprintf(count, sizeof(count), "%u/%u", (unsigned)b->chooseCount, (unsigned)PT_BOX_SLOTS);
    KitText(name, n->x + (n->w - UiTextWidth(name, 1)) / 2, top, n->x + n->w - 8);
    KitText(count, n->x + (n->w - UiTextWidth(count, 1)) / 2, top + KIT_LINE_H, n->x + n->w - 8);
}

/* FireRed's own mark (its 8x8 shape at twice its size) at (x, y): ink while on, the light grey while off. */
static void FXMarkShape(int mark, int on, int x, int y)
{
    unsigned char px[64];

    if (!CtrPokeTouch_MarkShape(mark, px))
        return;
    for (int i = 0; i < 64; ++i)
        if (px[i] == 3 || px[i] == 4)
            Fill(x + 2 * (i % 8), y + 2 * (i / 8), 2, 2, on && px[i] == 3 ? KitInk(2) : KitInk(3));
}

/* The window the box has open in the Party's place: its menu, YES / NO, or the four marks (on or off) with OK and CANCEL;
 * FireRed's arrow on its cursor or the row held. */
static void FXMenu(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    int n = FXMenuRows(b), held = b->pressed >= ST_HIT_MENU_BASE && b->pressed < ST_HIT_MENU_BASE + n
                                ? b->pressed - ST_HIT_MENU_BASE : -1;
    int chosen = held >= 0 ? held : b->menuCursor;
    char text[PT_BOX_MENU_MAX][40];
    const char *names[PARTY_MENU_ROWS];

    for (int i = 0; i < n; ++i) {
        if (b->state == PT_BOX_STATE_YESNO)
            snprintf(text[i], sizeof(text[i]), "%s", CtrUiText(i ? UIS_NO : UIS_YES));
        else if (b->state == PT_BOX_STATE_MARKS)
            snprintf(text[i], sizeof(text[i]), "%s", i == 4 ? CtrUiText(UIS_OK) : i == 5 ? CtrUiText(UIS_CANCEL) : "");
        else
            FNText(b->menu[i], (int)sizeof(b->menu[i]), text[i], (int)sizeof(text[i]));
        names[i] = text[i];
    }
    FPageMenu(v->frameType, names, n, chosen, held);
    for (int i = 0; b->state == PT_BOX_STATE_MARKS && i < 4; ++i) {
        CtrPokeTouchRect r = PartyMenuRow(n, i);

        FXMarkShape(i, (b->marks >> i) & 1, r.x + KIT_ARROW_W, r.y);
    }
}

/* The band: the box's message (a message that waits for A takes the whole band, beside an open window it is narrower),
 * else the Pokemon under the cursor (its name, gender sign and level; its species; its item), MOVE ITEMS' item and its
 * description, or the box's name. */
static void FXBand(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    const CtrPokeTouchRect *n = b->state == PT_BOX_STATE_MESSAGE ? &kBoxNoteWide : FXMenuOpen(b) ? &kBoxNoteMenu : &kBoxNote;
    int x = n->x + 16, right = n->x + n->w - 16, top = n->y + 6;
    char text[200];

    if (b->message[0] != 0xff) {
        FNText(b->message, (int)sizeof(b->message), text, (int)sizeof(text));
        FPageBand(n, NULL, text, b->state == PT_BOX_STATE_MESSAGE);
        return;
    }
    FPageBand(n, NULL, NULL, 0);
    if (b->items && b->mon.species) {
        if (!b->monItem) {
            FXLine(b->monText[0], (int)sizeof(b->monText[0]), x, top, right);
            KitText(CtrUiText(UIS_BOX_NO_ITEM), x, top + KIT_LINE_H, right);
            return;
        }
        FXLine(b->itemName, (int)sizeof(b->itemName), x, top, right);
        FNText(b->itemDesc, (int)sizeof(b->itemDesc), text, (int)sizeof(text));
        KitWrap(text, x, top + KIT_LINE_H, right - x, 2);
        return;
    }
    if (!b->mon.species) {
        FXLine(b->boxName, (int)sizeof(b->boxName), x, top, right);
        return;
    }
    {
        /* FireRed's "<gender> Lv5" line after the name, its sign blue or red. */
        char name[32], line[32], sign[3];
        const char *rest = line;
        int g = 0, at;

        FNText(b->monText[0], (int)sizeof(b->monText[0]), name, (int)sizeof(name));
        FNText(b->monText[2], (int)sizeof(b->monText[2]), line, (int)sizeof(line));
        if (line[0] == 0x01 && (line[1] == (char)0xb5 || line[1] == (char)0xb6)) {
            g = line[1] == (char)0xb6 ? 2 : 1;
            rest += 2;
        }
        while (*rest == ' ')
            ++rest;
        KitText(name, x, top, right);
        at = x + UiTextWidth(name, 1) + 6;
        if (g) {
            FNChar(g == 2 ? 0xb6 : 0xb5, sign);
            UiText(sign, at, top + 3, g == 2 ? kPPageFemale : kPPageMale, KitInk(3), 1, right);
            at += UiTextWidth(sign, 1) + 4;
        }
        KitText(rest, at, top, right);
    }
    FXLine(b->monText[1], (int)sizeof(b->monText[1]), x, top + KIT_LINE_H, right);
    FXLine(b->monText[3], (int)sizeof(b->monText[3]), x, top + 2 * KIT_LINE_H, right);
}

static void FBoxPageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    const CtrPokeTouchRect *back = FXBackRect(v);

    Fill(0, 0, W, H, BPageBg());
    FXSide(v);
    if (b->area == ST_AREA_PARTY)
        FXPartyPanel(v);
    else
        FXBoxWindow(v);
    if (b->state == PT_BOX_STATE_CHOOSE)
        FXChoose(v);
    FXHand(v);
    FXBand(v);
    if (FXMenuOpen(b))
        FXMenu(v);
    if (back && v->backVisible)
        FPageBack(v, back);
}
