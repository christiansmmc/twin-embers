/* The PC box as the red device (no board; plan 3), included by 3ds_poketouch.c after the naming keyboard, whose
 * FireRed text (FNText) it shares: a row of the field dispatcher (3ds_device_field_impl.h), shown while view->box.on and
 * there is no GBA picture (DevBoxShown). A remote for FireRed's own box (3ds_poketouch.h CtrPokeTouchBox):
 *  - the box's screen: its paper is FireRed's 160x144 wallpaper at 1:1 (owner decision 2; none: the second paper with
 *    the places a lighter grid), the box's name as typed on the wallpaper's band at its top (inverted while FireRed's
 *    cursor is on it), the thirty places 24 px apart with the Pokemon's icons (FireRed's, clipped to the paper), the
 *    cursor's place framed; MOVE ITEMS: a blue square at a place's lower right when its Pokemon holds an item. With the
 *    cursor in the Party the Party's rows on the same paper instead (half icon, nickname, level; CANCEL), the cursor's
 *    inverted, and the keys beside it grey (v0.6.0 takes only the Party then);
 *  - the right column: ◀ ▶ (sunk while held), PARTY and CLOSE BOX (dark; sunk by FireRed's cursor on its buttons or a
 *    held touch), the hand's screen (IN HAND and the Pokemon in hand, or MOVE ITEMS' item in hand);
 *  - the note across under them: FireRed's message (its ▼ while it waits for A, and then a tap anywhere is its A: the box
 *    and its keys grey); else MOVE ITEMS' item under the cursor; else the Pokemon under the cursor (FireRed's lines about
 *    it); else the box's name;
 *  - a menu, YES / NO or the marks: the box's screen grey, the note narrow, the keys in the right column (the menu's rows,
 *    the last dark; YES and NO; the four marks at 2x, OK and CANCEL dark), FireRed's cursor sunk;
 *  - the box chooser: ◀ ▶ and the chosen box's key (its name and how many it holds), always sunk; the box grey.
 * Under a held touch the key or row it holds is chosen instead of the cursor's (DevChosen, among the box's places, band
 * and keys, the Party's rows, a menu's keys). While FireRed is busy the box is drawn without the cursor's marks and
 * nothing is hit. No BACK: CLOSE BOX is FireRed's exit and every menu has its CANCEL. A touch is the PC's own
 * controller (3ds_bottom.c ProcessStorageTouch, the 3ds_storage_touch.h codes: the cursor walked there and A). Every
 * rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): FireRed's menu rows, species line and items in capitals (DevUpper); a nickname, the box's name and
 * the chosen box's name as typed, a nickname that is its species' own name in capitals; FireRed's messages and
 * descriptions as it writes them; the port's words in capitals.
 *
 * Redraw: the screen whole when the box as drawn differs (DevBoxSame), and nothing else. */
#include "3ds_storage_touch.h"

#define DEV_BOX_TEXT     200                        /* FireRed's codes as text: two bytes a code at most */
#define DEV_BOX_MARKS    6                          /* the marks' keys: the four marks, OK, CANCEL */
#define DEV_BOX_TITLE_SH UI_RGB(56, 56, 56)         /* FireRed's box name: white over this shadow on every wallpaper */

static int DevBoxShown(const CtrPokeTouchView *v)
{
    return v->box.on && !v->viewport;
}

/* The keys in the right column: the menu's rows, YES and NO, the marks; none in the other states. */
static int DevBoxKeyCount(const CtrPokeTouchBox *b)
{
    switch (b->state) {
    case PT_BOX_STATE_MENU:
        return b->menuCount < PT_BOX_MENU_MAX ? b->menuCount : PT_BOX_MENU_MAX;
    case PT_BOX_STATE_YESNO:
        return 2;
    case PT_BOX_STATE_MARKS:
        return DEV_BOX_MARKS;
    default:
        return 0;
    }
}

int CtrPokeTouch_BoxHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchBox *b = &v->box;
    int n = DevBoxKeyCount(b);

    if (!DevBoxShown(v))
        return ST_HIT_NONE;
    switch (b->state) {
    case PT_BOX_STATE_MESSAGE:
        return PT_BOX_HIT_MESSAGE;                  /* anywhere: the message waits for A */
    case PT_BOX_STATE_MENU:
    case PT_BOX_STATE_YESNO:
    case PT_BOX_STATE_MARKS:
        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r = DevBoxMenuKey(n, i);

            if (DevBagOnKey(&r, x, y))
                return ST_HIT_MENU_BASE + i;
        }
        return ST_HIT_NONE;
    case PT_BOX_STATE_CHOOSE:
        return DevBagOnKey(&kDevBoxPrev, x, y) ? ST_HIT_PREV : DevBagOnKey(&kDevBoxNext, x, y) ? ST_HIT_NEXT
             : DevBagOnKey(&kDevBoxChoose, x, y) ? ST_HIT_TITLE : ST_HIT_NONE;
    case PT_BOX_STATE_BOX:
        break;
    default:
        return ST_HIT_NONE;                         /* busy */
    }
    if (b->area == ST_AREA_PARTY) {
        for (int i = 0; i < 7; ++i) {
            CtrPokeTouchRect r = DevBoxPartyRow(i);

            if (UiIn(&r, x, y))
                return ST_HIT_PARTY_BASE + i;
        }
        return ST_HIT_NONE;
    }
    if (DevBagOnKey(&kDevBoxPrev, x, y))
        return ST_HIT_PREV;
    if (DevBagOnKey(&kDevBoxNext, x, y))
        return ST_HIT_NEXT;
    if (DevBagOnKey(&kDevBoxParty, x, y))
        return ST_HIT_BUTTON_PARTY;
    if (DevBagOnKey(&kDevBoxClose, x, y))
        return ST_HIT_BUTTON_CLOSE;
    if (UiIn(&kDevBoxTitle, x, y))
        return ST_HIT_TITLE;
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = DevBoxCell(i);

        if (UiIn(&r, x, y))
            return i;
    }
    return ST_HIT_NONE;
}

/* The touch held as it is drawn: a code of what is drawn and taken in the state shown (a place, the band, ◀ ▶, PARTY,
 * CLOSE BOX; a Party row; a key of the right column; the chooser's ◀ ▶), else -1. */
static int DevBoxPress(const CtrPokeTouchBox *b)
{
    int p = b->pressed;

    switch (b->state) {
    case PT_BOX_STATE_BOX:
        if (b->area == ST_AREA_PARTY)
            return p >= ST_HIT_PARTY_BASE && p <= ST_HIT_PARTY_BASE + 6 ? p : -1;
        return (p >= 0 && p < PT_BOX_SLOTS) || (p >= ST_HIT_TITLE && p <= ST_HIT_NEXT) ? p : -1;
    case PT_BOX_STATE_MENU:
    case PT_BOX_STATE_YESNO:
    case PT_BOX_STATE_MARKS:
        return p >= ST_HIT_MENU_BASE && p < ST_HIT_MENU_BASE + DevBoxKeyCount(b) ? p : -1;
    case PT_BOX_STATE_CHOOSE:
        return p == ST_HIT_PREV || p == ST_HIT_NEXT ? p : -1;   /* the chosen box's key is sunk anyway */
    default:
        return -1;
    }
}

/* FireRed's cursor as the code of what it is on (a place, ST_HIT_TITLE, PARTY, CLOSE BOX, a Party row), or -1 (none;
 * busy). */
static int DevBoxCursor(const CtrPokeTouchBox *b)
{
    if (b->state == PT_BOX_STATE_BUSY)
        return -1;
    switch (b->area) {
    case ST_AREA_BOX:
        return b->pos >= 0 && b->pos < PT_BOX_SLOTS ? b->pos : -1;
    case ST_AREA_TITLE:
        return ST_HIT_TITLE;
    case ST_AREA_BUTTONS:
        return b->pos == 0 ? ST_HIT_BUTTON_PARTY : b->pos == 1 ? ST_HIT_BUTTON_CLOSE : -1;
    case ST_AREA_PARTY:
        return b->pos >= 0 && b->pos <= 6 ? ST_HIT_PARTY_BASE + b->pos : -1;
    default:
        return -1;
    }
}

/* ---- FireRed's text and art --------------------------------------------------------------------------------------- */

/* FireRed's codes as text (FNText), its letters and digits as ASCII: they draw the same (GlyphCode) and DevUpper can
 * put them in capitals. */
static void DevBoxText(const uint8_t *raw, int n, char *out, int size)
{
    int i = 0, o = 0;

    FNText(raw, n, out, size);
    while (out[i]) {
        unsigned char c = (unsigned char)out[i], code = (unsigned char)out[i + 1];

        if ((c == 0x01 || c == 0x02) && code) {
            char a = c != 0x01 ? 0 : code >= 0xbb && code <= 0xd4 ? (char)('A' + code - 0xbb)
                   : code >= 0xd5 && code <= 0xee ? (char)('a' + code - 0xd5) : code >= 0xa1 && code <= 0xaa
                   ? (char)('0' + code - 0xa1) : 0;

            if (a) {
                out[o++] = a;
                i += 2;
                continue;
            }
            out[o++] = out[i++];
        }
        out[o++] = out[i++];
    }
    out[o] = 0;
}

/* The same in capitals (FireRed's strings that arrive title-cased: a menu row, a species, an item). */
static void DevBoxUpper(const uint8_t *raw, int n, char *out, int size)
{
    char text[DEV_BOX_TEXT];

    DevBoxText(raw, n, text, (int)sizeof(text));
    DevUpper(out, text, (size_t)size);
}

/* How many codes `raw` holds before its end, the spaces padding it left out. */
static int DevBoxCodeLen(const uint8_t *raw, int n)
{
    int len = 0;

    while (len < n && raw[len] != 0xff)
        ++len;
    while (len > 0 && raw[len - 1] == 0x00)
        --len;
    return len;
}

/* The cursor's Pokemon's name: in capitals when it is its species' own (FireRed's "/" line without the slash), else as
 * typed. */
static void DevBoxMonName(const CtrPokeTouchBox *b, char *out, int size)
{
    const uint8_t *species = b->monText[1] + 1;
    int len = DevBoxCodeLen(b->monText[0], (int)sizeof(b->monText[0]));

    if (b->monText[1][0] == 0xba && len && len == DevBoxCodeLen(species, (int)sizeof(b->monText[1]) - 1)
        && !memcmp(b->monText[0], species, (size_t)len))
        DevBoxUpper(b->monText[0], (int)sizeof(b->monText[0]), out, size);
    else
        DevBoxText(b->monText[0], (int)sizeof(b->monText[0]), out, size);
}

/* The box's wallpaper (FireRed's 160x144 picture) in the lower screen's colours, decoded once per wallpaper. */
static uint16_t sDevBoxWall[160 * 144];
static int sDevBoxWallId = -1, sDevBoxWallOk;

static int DevBoxWallpaper(unsigned id)
{
    static unsigned short raw[160 * 144];

    if ((int)id != sDevBoxWallId) {
        sDevBoxWallId = (int)id;
        sDevBoxWallOk = CtrPokeTouch_Wallpaper(id, raw);
        for (int i = 0; sDevBoxWallOk && i < 160 * 144; ++i) {
            unsigned c = raw[i];

            sDevBoxWall[i] = GbaColor(c);
        }
    }
    return sDevBoxWallOk;
}

/* A Pokemon's icon (FireRed's, its first frame) with its top-left at (x, y): 32x32 clipped to `clip`, or (`half`) at
 * half size (DevIconPixels). */
static void DevBoxIcon(const CtrPokeTouchBoxMon *m, int x, int y, const CtrPokeTouchRect *clip, int half)
{
    static unsigned char tiles[PT_MON_ICON_BYTES];  /* the bridge writes both frames; the first is drawn */
    static unsigned short pal[16];
    static uint8_t px[32 * 32];
    static uint16_t pal565[16];
    int x0 = x > clip->x ? x : clip->x, y0 = y > clip->y ? y : clip->y;
    int x1 = x + 32 < clip->x + clip->w ? x + 32 : clip->x + clip->w;
    int y1 = y + 32 < clip->y + clip->h ? y + 32 : clip->y + clip->h;

    if (!m->species)
        return;
    if (half)
        DevArt(DEV_ART_ICON, x, y, 16, 16);
    else if (x1 > x0 && y1 > y0)
        DevArt(DEV_ART_ICON, x0, y0, x1 - x0, y1 - y0);
    if (!CtrPokeTouch_MonIcon(m->species, m->personality, m->egg, tiles, pal))
        return;
    FDTiles(tiles, 32, 32, px);
    FDPal(pal, pal565);
    if (half) {
        DevIconPixels(px, pal565, x, y, 1, 0);
        return;
    }
    for (int py = y0; py < y1; ++py)
        for (int qx = x0; qx < x1; ++qx) {
            uint8_t c = px[(py - y) * 32 + qx - x];

            if (c)
                Px(qx, py, pal565[c]);
        }
}

/* An item's icon (FireRed's 24x24) at (x, y). */
static void DevBoxItem(unsigned item, int x, int y)
{
    DevArt(DEV_ART_ITEM, x, y, 24, 24);
    DevBagIcon(item, x, y, 0);
}

/* ---- The box's screen ----------------------------------------------------------------------------------------------- */

/* A place's frame: 2 px in the bezel's colour round the inside of r. */
static void DevBoxFrame(const CtrPokeTouchRect *r)
{
    DevRect(r->x, r->y, r->w, 2, DEV_BEZEL);
    DevRect(r->x, r->y + r->h - 2, r->w, 2, DEV_BEZEL);
    DevRect(r->x, r->y + 2, 2, r->h - 4, DEV_BEZEL);
    DevRect(r->x + r->w - 2, r->y + 2, 2, r->h - 4, DEV_BEZEL);
}

/* The box on its paper: the wallpaper (or the second paper and its places' grid), the name on the band (inverted when
 * `cursor` or `press` chooses it), the icons, MOVE ITEMS' squares, the chosen place's frame. */
static void DevBoxPlaces(const CtrPokeTouchBox *b, const CtrPokeTouchRect *paper, int cursor, int press)
{
    int wall = DevBoxWallpaper(b->wallpaper), band = DevChosen(ST_HIT_TITLE, cursor, press);
    char name[DEV_BOX_TEXT];
    int w;

    if (wall) {
        DevArt(DEV_ART_WALLPAPER, paper->x, paper->y, 160, 144);
        for (int y = 0; y < 144; ++y)
            for (int x = 0; x < 160; ++x)
                Px(paper->x + x, paper->y + y, sDevBoxWall[y * 160 + x]);
    } else {
        DevRect(paper->x, paper->y, paper->w, paper->h, DEV_PAPER2);
        for (int i = 0; i < PT_BOX_SLOTS; ++i) {
            CtrPokeTouchRect r = DevBoxCell(i);

            DevRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, DEV_PAPER);
        }
    }
    if (band)
        DevRow(&kDevBoxTitle, 1);
    DevBoxText(b->boxName, (int)sizeof(b->boxName), name, (int)sizeof(name));
    w = DevTextWidth(name);
    DevTextInk(name, kDevBoxTitle.x + (kDevBoxTitle.w - w) / 2, kDevBoxTitle.y + (kDevBoxTitle.h - UI_TEXT_H) / 2,
               kDevBoxTitle.x + kDevBoxTitle.w, band ? DEV_PAPER : wall ? DEV_WHITE : DEV_INK,
               band ? DEV_PAPER : wall ? DEV_BOX_TITLE_SH : DEV_INK_SH);
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = DevBoxCell(i);

        DevBoxIcon(&b->slot[i], r.x - 4, r.y - 6, paper, 0);
    }
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = DevBoxCell(i);

        if (b->items && b->slot[i].species && b->slotItem[i])
            DevRect(r.x + r.w - 8, r.y + r.h - 8, 6, 6, DEV_BLUE_INK);
        if (DevChosen(i, cursor, press))
            DevBoxFrame(&r);
    }
}

/* The Party on the box's paper: a row for each slot (its icon at half size, its nickname as typed, its level) and
 * CANCEL; the chosen row inverted; MOVE ITEMS: a blue square by the icon of a Pokemon that holds an item. */
static void DevBoxParty(const CtrPokeTouchBox *b, int cursor, int press)
{
    char text[DEV_BOX_TEXT];

    for (int i = 0; i < 7; ++i) {
        CtrPokeTouchRect r = DevBoxPartyRow(i);
        int chosen = DevChosen(ST_HIT_PARTY_BASE + i, cursor, press);
        int top = r.y + (r.h - UI_TEXT_H) / 2, right = r.x + r.w - 6;
        uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;

        DevRow(&r, chosen);
        if (i == 6) {
            DevUpper(text, CtrUiText(UIS_CANCEL), sizeof(text));
            DevTextInk(text, r.x + 22, top, right, ink, shadow);
            continue;
        }
        if (!b->party[i].species)
            continue;
        DevBoxIcon(&b->party[i], r.x + 2, r.y + 2, &r, 1);
        if (b->items && b->partyItem[i])
            DevRect(r.x + 14, r.y + 12, 6, 6, DEV_BLUE_INK);
        if (!b->party[i].egg) {
            snprintf(text, sizeof(text), CTR_TR("Lv%u", "Nv%u", "Lv%u"), (unsigned)b->partyLevel[i]);
            right -= DevTextWidth(text);
            DevTextInk(text, right, top, r.x + r.w, ink, shadow);
            right -= 6;
        }
        DevBoxText(b->partyName[i], (int)sizeof(b->partyName[i]), text, (int)sizeof(text));
        DevTextInk(text, r.x + 22, top, right, ink, shadow);
    }
}

/* ---- The right column, the hand, the note ------------------------------------------------------------------------- */

/* ◀ ▶, PARTY and CLOSE BOX (dark): sunk as chosen; grey (`dim`) while they are not taken. */
static void DevBoxSide(int cursor, int press, int dim)
{
    char word[32];
    int sunk, faceY;

    DevDexArrows(&kDevBoxPrev, &kDevBoxNext, 1, 1, press == ST_HIT_PREV, press == ST_HIT_NEXT, 1);
    sunk = DevChosen(ST_HIT_BUTTON_PARTY, cursor, press);
    faceY = DevKey(&kDevBoxParty, DEV_LOOK_KEY, sunk, 6);
    DevUpper(word, CtrUiText(UIS_BOX_PARTY), sizeof(word));
    DevKeyWord(&kDevBoxParty, faceY, DEV_LOOK_KEY, sunk, word);
    sunk = DevChosen(ST_HIT_BUTTON_CLOSE, cursor, press);
    faceY = DevKey(&kDevBoxClose, DEV_LOOK_DARK, sunk, 6);
    DevUpper(word, CtrUiText(UIS_BOX_CLOSE), sizeof(word));
    DevKeyWord(&kDevBoxClose, faceY, DEV_LOOK_DARK, sunk, word);
    if (!dim)
        return;
    DevDim(&kDevBoxPrev);
    DevDim(&kDevBoxNext);
    DevDim(&kDevBoxParty);
    DevDim(&kDevBoxClose);
}

/* The hand's screen: IN HAND small at its left; the Pokemon in hand at its right, or MOVE ITEMS' item in hand. */
static void DevBoxHand(const CtrPokeTouchBox *b)
{
    CtrPokeTouchRect paper = DevScreen(&kDevBoxHand);
    int room = paper.w - 42;                        /* left of the 32 px icon */
    char word[32];

    DevUpper(word, CtrUiText(UIS_BOX_IN_HAND), sizeof(word));
    if (DevSmallTextWidth(word) > room)
        ++sDevKeyCut;
    DevSmallText(word, paper.x + 6, paper.y + (paper.h - 7) / 2, DEV_INK2, paper.x + 6 + room);
    if (b->items && b->handItem)
        DevBoxItem(b->handItem, paper.x + paper.w - 30, paper.y + 4);
    else if (b->holding)
        DevBoxIcon(&b->held, paper.x + paper.w - 34, paper.y, &paper, 0);
}

/* The Pokemon under the cursor on the note's paper: its name, the gender's sign and level (FireRed's line), the species
 * line and the item in the secondary ink. */
static void DevBoxMonNote(const CtrPokeTouchBox *b, const CtrPokeTouchRect *paper)
{
    int x = paper->x + 6, right = paper->x + paper->w - 6, top = paper->y + 5, at;
    char text[DEV_BOX_TEXT];
    const char *rest = text;

    DevBoxMonName(b, text, (int)sizeof(text));
    DevText(text, x, top, right);
    at = x + DevTextWidth(text) + 6;
    DevBoxText(b->monText[2], (int)sizeof(b->monText[2]), text, (int)sizeof(text));
    if (text[0] == 0x01 && (text[1] == (char)0xb5 || text[1] == (char)0xb6)) {
        DevGender(text[1] == (char)0xb6, at, top - 1);
        at += 12;
        rest += 2;
    }
    while (*rest == ' ')
        ++rest;
    DevText(rest, at, top, right);
    DevBoxUpper(b->monText[1], (int)sizeof(b->monText[1]), text, (int)sizeof(text));
    DevText2(text, x, top + DEV_LINE_H, right);
    DevBoxUpper(b->monText[3], (int)sizeof(b->monText[3]), text, (int)sizeof(text));
    DevText2(text, x, top + 2 * DEV_LINE_H, right);
}

/* MOVE ITEMS: the item the Pokemon under the cursor holds (its icon, its name, the Pokemon's at the right, FireRed's
 * description small), or the Pokemon's name and that it holds none. */
static void DevBoxItemNote(const CtrPokeTouchBox *b, const CtrPokeTouchRect *paper)
{
    int x = paper->x + 34, right = paper->x + paper->w - 6, top = paper->y + 5;
    char text[DEV_BOX_TEXT], name[DEV_BOX_TEXT];
    int w;

    DevBoxMonName(b, name, (int)sizeof(name));
    if (!b->monItem) {
        DevText(name, paper->x + 6, top, right);
        DevUpper(text, CtrUiText(UIS_BOX_NO_ITEM), sizeof(text));
        DevText2(text, paper->x + 6, top + DEV_LINE_H, right);
        return;
    }
    DevBoxItem(b->monItem, paper->x + 4, paper->y + 4);
    w = DevTextWidth(name);
    DevText2(name, right - w, top, right);
    DevBoxUpper(b->itemName, (int)sizeof(b->itemName), text, (int)sizeof(text));
    DevText(text, x, top, right - w - 6);
    DevBoxText(b->itemDesc, (int)sizeof(b->itemDesc), text, (int)sizeof(text));
    DevBagSmallWrap(text, x, x, top + 14, right, 3, DEV_INK);
}

/* The note (`outer`: wide, or narrow beside the right column's keys): FireRed's message while there is one (its ▼ while
 * it waits for A); else MOVE ITEMS' item, the Pokemon under the cursor, or the box's name. */
static void DevBoxNote(const CtrPokeTouchBox *b, const CtrPokeTouchRect *outer)
{
    CtrPokeTouchRect paper = DevScreen(outer);
    int x = paper.x + 6, right = paper.x + paper.w - 6;
    char text[DEV_BOX_TEXT];

    if (b->message[0] != 0xff) {
        int wait = b->state == PT_BOX_STATE_MESSAGE, width = right - x - (wait ? 12 : 0), lines;

        DevBoxText(b->message, (int)sizeof(b->message), text, (int)sizeof(text));
        lines = DevWrapLines(text, width, 3);
        if (lines)
            DevWrap(text, x, paper.y + (paper.h - (8 + DEV_LINE_H * (lines - 1))) / 2, width, 3, DEV_INK, DEV_INK_SH);
        if (wait)
            DevTriangle(paper.x + paper.w - 10, paper.y + paper.h - 8, 1, DEV_INK2);
        return;
    }
    if (!b->mon.species) {
        DevBoxText(b->boxName, (int)sizeof(b->boxName), text, (int)sizeof(text));
        DevText(text, x, paper.y + (paper.h - 8) / 2, right);
    } else if (b->items)
        DevBoxItemNote(b, &paper);
    else
        DevBoxMonNote(b, &paper);
}

/* A mark's shape (FireRed's 8x8: the mark and its edge) at 2x in `ink`, its top-left at (x, y). */
static void DevBoxMark(int mark, int x, int y, uint16_t ink)
{
    unsigned char px[64];

    DevArt(DEV_ART_MARK, x, y, 16, 16);
    if (!CtrPokeTouch_MarkShape(mark, px))
        return;
    for (int i = 0; i < 64; ++i)
        if (px[i] == 3 || px[i] == 4)
            DevRect(x + 2 * (i % 8), y + 2 * (i / 8), 2, 2, ink);
}

/* The right column's keys: the menu's rows (the last dark), YES and NO, or the four marks (on in the key's ink, off dim),
 * OK and CANCEL (dark); FireRed's cursor sunk, or the key under a held touch. */
static void DevBoxMenu(const CtrPokeTouchBox *b, int press)
{
    int n = DevBoxKeyCount(b), held = press >= 0 ? press - ST_HIT_MENU_BASE : -1;
    char word[DEV_BOX_TEXT];

    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = DevBoxMenuKey(n, i);
        int look = b->state != PT_BOX_STATE_YESNO && i == n - 1 ? DEV_LOOK_DARK : DEV_LOOK_KEY;
        int sunk = DevChosen(i, b->menuCursor, held), faceY = DevKey(&r, look, sunk, 6);

        if (b->state == PT_BOX_STATE_MARKS && i < 4) {
            DevBoxMark(i, r.x + (r.w - 16) / 2, faceY + (r.h - 16) / 2, (b->marks >> i) & 1 ? DEV_KEY_INK : DEV_INK_DIM);
            continue;
        }
        if (b->state == PT_BOX_STATE_MENU)
            DevBoxUpper(b->menu[i], (int)sizeof(b->menu[i]), word, (int)sizeof(word));
        else if (b->state == PT_BOX_STATE_YESNO)
            DevUpper(word, CtrUiText(i ? UIS_NO : UIS_YES), sizeof(word));
        else
            DevUpper(word, CtrUiText(i == 4 ? UIS_OK : UIS_CANCEL), sizeof(word));
        DevBagWord(&r, faceY, look, sunk, word, look == DEV_LOOK_KEY);
    }
}

/* The chooser: ◀ ▶ (sunk while held) and the chosen box's key, always sunk: its name as typed and how many it holds. */
static void DevBoxChooser(const CtrPokeTouchBox *b, int press)
{
    const CtrPokeTouchRect *r = &kDevBoxChoose;
    int faceY = DevKey(r, DEV_LOOK_KEY, 1, 6), w;
    char text[DEV_BOX_TEXT];

    DevDexArrows(&kDevBoxPrev, &kDevBoxNext, 1, 1, press == ST_HIT_PREV, press == ST_HIT_NEXT, 1);
    DevBoxText(b->chooseName, (int)sizeof(b->chooseName), text, (int)sizeof(text));
    w = DevTextWidth(text);
    DevTextInk(text, r->x + (r->w - w) / 2, faceY + 22, r->x + r->w - 4, DEV_SUNK_INK, DEV_SUNK_INK_SH);
    snprintf(text, sizeof(text), "%u/%u", (unsigned)b->chooseCount, (unsigned)PT_BOX_SLOTS);
    w = DevSmallTextWidth(text);
    DevSmallText(text, r->x + (r->w - w) / 2, faceY + 42, DEV_SUNK_INK, r->x + r->w - 4);
}

/* The box whole (after DevBegin: the dispatcher's). A held touch chooses among the box's own pieces only while the box
 * takes it (BOX); a menu's keys, the chooser's ◀ ▶ take theirs. */
static void DevBoxDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    int press = DevBoxPress(b), cursor = DevBoxCursor(b), boxPress = b->state == PT_BOX_STATE_BOX ? press : -1;
    int keys = DevBoxKeyCount(b) > 0, message = b->state == PT_BOX_STATE_MESSAGE;
    int grey = keys || message || b->state == PT_BOX_STATE_CHOOSE;
    CtrPokeTouchRect paper;

    DevShell(0);
    paper = DevScreen(&kDevBoxScreen);
    if (b->area == ST_AREA_PARTY)
        DevBoxParty(b, cursor, boxPress);
    else
        DevBoxPlaces(b, &paper, cursor, boxPress);
    if (grey)
        DevDim(&kDevBoxScreen);
    if (keys) {
        DevBoxNote(b, &kDevBoxNoteNarrow);
        DevBoxMenu(b, press);
        return;
    }
    DevBoxNote(b, &kDevBoxNote);
    if (b->state == PT_BOX_STATE_CHOOSE)
        DevBoxChooser(b, press);
    else
        DevBoxSide(cursor, boxPress, message || (b->state == PT_BOX_STATE_BOX && b->area == ST_AREA_PARTY));
    DevBoxHand(b);
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* What of `b` is drawn: the box but its number, the press as drawn, the right column's keys and the chooser only in
 * their states, the Pokemon in hand while it is, the Pokemon's lines and MOVE ITEMS' item while the note shows them. */
static void DevBoxDrawn(const CtrPokeTouchBox *b, CtrPokeTouchBox *out)
{
    int keys = DevBoxKeyCount(b) > 0, lines = b->message[0] == 0xff && b->mon.species;

    memcpy(out, b, sizeof(*out));
    out->box = 0;
    out->pressed = (int16_t)DevBoxPress(b);
    if (!keys) {
        out->menuCount = 0;
        out->menuCursor = 0;
    }
    if (b->state != PT_BOX_STATE_MENU)
        memset(out->menu, 0, sizeof(out->menu));
    if (b->state != PT_BOX_STATE_MARKS)
        out->marks = 0;
    if (b->state != PT_BOX_STATE_CHOOSE) {
        memset(out->chooseName, 0, sizeof(out->chooseName));
        out->chooseCount = 0;
    }
    if (!b->holding)
        memset(&out->held, 0, sizeof(out->held));
    if (!lines) {
        memset(out->monText, 0, sizeof(out->monText));
        out->monItem = 0;
    }
    if (!lines || !b->items || !b->monItem) {
        memset(out->itemName, 0, sizeof(out->itemName));
        memset(out->itemDesc, 0, sizeof(out->itemDesc));
    }
}

/* Nothing the box draws differs between `a` and `b` (the shell is the dispatcher's; it draws no BACK). */
static int DevBoxSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    CtrPokeTouchBox p, q;

    DevBoxDrawn(&a->box, &p);
    DevBoxDrawn(&b->box, &q);
    return !memcmp(&p, &q, sizeof(p));
}

/* No BACK: CLOSE BOX is FireRed's exit, and every menu has its CANCEL. */
static const CtrPokeTouchRect *DevBoxBackKey(const CtrPokeTouchView *v)
{
    (void)v;
    return NULL;
}
