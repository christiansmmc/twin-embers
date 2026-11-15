/* The battle Bag in the ember "Llama" look (included in 3ds_poketouch.c, whose drawing helpers and design system it
 * uses; the owner's mockup, llama2_mockup.html; the pieces are 3ds_ember2_impl.h's Ll*): the dark ember backdrop, raised
 * glass cards (flame for what is chosen, slate for CANCEL), FireRed's text window for the notes, its red cursor. The rest
 * of the battle's lower screen (the commands, the moves, the targets, the Safari Zone, idle) and the Party are the red
 * device's (3ds_device_battle_impl.h, 3ds_device_party_impl.h); the battle file owns the public CtrBattleUi_Draw / Update
 * / Hit and hands the Bag to BEmberDraw / BEmberUpdate / BEmberHit; the red device also uses the item and party icon
 * caches here. Everything shown is what the controller reports. */

#include "3ds_icons.h"
#include "3ds_text_case.h"
#include "3ds_ui_strings.h"

/* The battle Bag: its title (the Items pocket, the only one it shows) and BERRIES for the Berry Pouch, the item list
 * on the left with its scroll arrows, the selected item's card on the right, the description's note and CANCEL. */
static const CtrPokeTouchRect sBTitle = {6, 4, 204, 26}, sBTitleWide = {6, 4, 308, 26}, sBBerries = {214, 4, 100, 26};
static const CtrPokeTouchRect sBList = {6, 36, 190, 138};
static const CtrPokeTouchRect sBDetail = {200, 36, 114, 138};
static const CtrPokeTouchRect sBUse = {210, 136, 94, 30};
static const CtrPokeTouchRect sBUpArrow = {176, 36, 20, 66}, sBDownArrow = {176, 108, 20, 66};
/* The Bag's text window and CANCEL. */
static const CtrPokeTouchRect sBNote = {4, 176, 236, 60};
static const CtrPokeTouchRect sBBagCancel = {244, 180, 72, 52};
#define B_DEPTH 3

static CtrPokeTouchRect BRow(int i)
{
    CtrPokeTouchRect r = {6, (int16_t)(36 + 28 * i), 166, 26};
    return r;
}

static CtrPokeTouchRect BMenuRow(int i)
{
    CtrPokeTouchRect r = {208, (int16_t)(84 + 30 * i), 98, 27};
    return r;
}

static int BIn(const CtrPokeTouchRect *r, int x, int y)
{
    return UiIn(r, x, y);
}

/* The row of the Bag's selected item when it is on screen (USE stands for it), else -1. */
static int BBagSelRow(const CtrBattleBag *b)
{
    int i = b->cursor - b->first;

    return i >= 0 && i < BUI_BAG_ROWS && b->row[i].item ? i : -1;
}

/* What the battle Bag shows under a point (CtrBattleUi_Hit's for it). */
static int BEmberHit(const CtrBattleView *v, int x, int y)
{
    if (v->state == BUI_BAG) {
        const CtrBattleBag *b = &v->bag;

        if (b->message[0])
            return BUI_BAG_MESSAGE;                         /* anywhere: the message waits for A */
        if (BIn(&sBBagCancel, x, y))
            return BUI_BAG_CANCEL;
        if (b->berries && BIn(&sBBerries, x, y))
            return BUI_BAG_BERRIES;
        if (b->menuCount > 0 && BIn(&sBDetail, x, y)) {
            for (int i = 0; i < b->menuCount && i < 3; ++i) {
                CtrPokeTouchRect r = BMenuRow(i);
                if (BIn(&r, x, y))
                    return BUI_BAG_MENU + i;
            }
            return -1;
        }
        if (b->menuCount == 0 && BBagSelRow(b) >= 0 && BIn(&sBUse, x, y))
            return BUI_BAG_ROW + BBagSelRow(b);             /* USE: the selected row again, which uses it */
        if (b->count > BUI_BAG_ROWS) {
            /* The arrows are their 22 px buttons; between them the rail is a slider. */
            CtrPokeTouchRect up = {sBUpArrow.x, sBUpArrow.y, sBUpArrow.w, 22};
            CtrPokeTouchRect down = {sBDownArrow.x, sBDownArrow.y + sBDownArrow.h - 22, sBDownArrow.w, 22};
            CtrPokeTouchRect rail = {sBUpArrow.x, sBUpArrow.y + 22, sBUpArrow.w, sBDownArrow.y + sBDownArrow.h - 22 - (sBUpArrow.y + 22)};

            if (BIn(&up, x, y))
                return b->first > 0 ? BUI_BAG_UP : -1;
            if (BIn(&down, x, y))
                return b->first + BUI_BAG_ROWS < b->count ? BUI_BAG_DOWN : -1;
            if (BIn(&rail, x, y))
                return BUI_BAG_RAIL;
        }
        for (int i = 0; i < BUI_BAG_ROWS; ++i) {
            CtrPokeTouchRect r = BRow(i);
            if (b->row[i].item && BIn(&r, x, y))
                return BUI_BAG_ROW + i;
        }
    }
    return -1;
}

/* Running text wrapped to `width` pixels, `maxLines` at most (FireRed's line breaks are spaces here): the last line ends in "..." when
 * there is more (Em2Wrap). */
static void BDescribeSh(const char *text, int x, int y, int width, int maxLines, uint16_t color, uint16_t shadow)
{
    Em2Wrap(text, x, y, width, maxLines, UI_LINE_H, color, shadow);
}

static void BDescribeN(const char *text, int x, int y, int width, int maxLines, uint16_t color)
{
    BDescribeSh(text, x, y, width, maxLines, color, UIS_TEXT_SH);
}


/* The PP colour FireRed's move list uses: the usual ink, then yellow at half, orange at a quarter, red at none. */
static uint16_t BPpColor(unsigned pp, unsigned max)
{
    return pp == 0 ? UI_HP_LOW : pp * 4 <= max ? UI_WARN : pp * 2 <= max ? UI_HP_MID : UI_TEXT;
}


/* The Party's icons (the red device's Party, the battle's lead and HOME draw them). */
extern int CtrBottomParty_Icon(int slot, unsigned char *tilesOut, unsigned short *palOut);
/* Party icons: FireRed's 32x32 icon of a slot with both its frames (the second under the first), decoded once per
 * species and slot. */
static struct {
    unsigned icon;
    int ok;
    uint8_t pixels[32 * 64];
    uint16_t pal[16];
} sBPIcon[6];

static int BPartyIcon(int slot, unsigned icon)
{
    static unsigned char tiles[1024];
    static unsigned short pal[16];

    if (sBPIcon[slot].icon == icon + 1)
        return sBPIcon[slot].ok;
    memset(tiles, 0, sizeof(tiles));
    sBPIcon[slot].icon = icon + 1;
    sBPIcon[slot].ok = CtrBottomParty_Icon(slot, tiles, pal);
    if (!sBPIcon[slot].ok)
        return 0;
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 32; ++x) {
            unsigned byte = tiles[(y / 8 * 4 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2];
            sBPIcon[slot].pixels[y * 32 + x] = (uint8_t)((byte >> (x & 1) * 4) & 15);
        }
    for (int i = 0; i < 16; ++i) {
        unsigned c = pal[i];
        sBPIcon[slot].pal[i] = GbaColor(c);
    }
    return 1;
}

/* Text wrapped into `maxLines` lines of `width`, in `ink` over `shadow`, 16 apart; the last ends in "..." when there is more
 * (the field Bag's prompts). */
static void BPWrap(const char *text, int x, int y, int width, int maxLines, uint16_t ink, uint16_t shadow)
{
    Em2Wrap(text, x, y, width, maxLines, 16, ink, shadow);
}

/* ---- the Llama pieces the battle Bag uses ---- */

/* The round back arrow, a word under it: the label of a tall CANCEL (a slate button whose face starts at `fy`: the disc `discY` and the word
 * `wordY` below it). */
static void BBackLabel(const CtrPokeTouchRect *r, int fy, int discY, int wordY)
{
    LlBackDisc(r->x + (r->w - 24) / 2, fy + discY);
    EmLabel(CTR_TR("Cancel", "Cancelar", "Zur\xC3\xBC" "ck"), r->x, r->w, fy + wordY, UI_TEXT_H, &kEmSlate);
}

/* A message in FireRed's text window over the dimmed screen, with "tap to continue" and its arrow (the Bag's). */
static void BMessageWindow(const char *message)
{
    CtrPokeTouchRect all = {0, 0, W, H}, box = {20, 70, 280, 76};
    const char *tap = CTR_TR("Tap to continue", "Toca para seguir", "Tippen zum Fortfahren");

    UiDim(&all);
    LlWindow(box.x, box.y, box.w, box.h);
    Em2Wrap(message, box.x + 14, box.y + 11, box.w - 28, 3, 14, EM_DARK, EM_DARK_SH);
    UiText(tap, box.x + box.w - 30 - UiTextWidth(tap, 1), box.y + box.h - 19, EM_DIM, EM_DARK_SH, 1, box.x + box.w - 18);
    LlArrow(box.x + box.w - 26, box.y + box.h - 18, 8, 5, 1, LL_CURSOR);
}

/* Item icons: FireRed's 24x24 icon of an item, decoded once into a collision-free 12-slot FIFO cache. */
#define BICONS 12
static struct {
    unsigned item;
    int ok;
    uint8_t pixels[24 * 24];
    uint16_t pal[16];
} sBIcon[BICONS];
static int sBIconNext;

static int BItemIcon(unsigned item)
{
    static unsigned char tiles[0x200];
    static unsigned short pal[16];
    int slot = -1;

    if (item == 0)
        return -1;
    for (int i = 0; i < BICONS; ++i)
        if (sBIcon[i].item == item)
            return sBIcon[i].ok ? i : -1;
    for (int i = 0; i < BICONS; ++i)
        if (sBIcon[i].item == 0) {
            slot = i;
            break;
        }
    if (slot < 0) {
        slot = sBIconNext;
        sBIconNext = (sBIconNext + 1) % BICONS;
    }
    memset(tiles, 0, sizeof(tiles));
    sBIcon[slot].item = item;
    sBIcon[slot].ok = CtrPokeTouch_ItemIcon(item, tiles, pal);
    if (!sBIcon[slot].ok)
        return -1;
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 24; ++x) {
            unsigned byte = tiles[(y / 8 * 3 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2];
            sBIcon[slot].pixels[y * 24 + x] = (uint8_t)((byte >> (x & 1) * 4) & 15);
        }
    for (int i = 0; i < 16; ++i) {
        unsigned c = pal[i];
        sBIcon[slot].pal[i] = GbaColor(c);
    }
    return slot;
}

/* An item's icon at `scale` times its size, its top-left corner at (x, y). */
static void BItemDraw(unsigned item, int x, int y, int scale)
{
    int slot = BItemIcon(item);

    if (slot >= 0)
        UiImage(sBIcon[slot].pixels, sBIcon[slot].pal, 24, 24, 24, x, y, scale, 1);
}


/* The ember surface: PokeTouch's red theme with the shadows and outlines of the light pieces (the Summary's too). */
static UiSurface sUiEmber;

static void EmSurface(void)
{
    sUiEmber = sUiRed;
    sUiEmber.textSh = EM_DARK_SH;
    sUiEmber.textDim = EM_DIM;
    sUiEmber.outline = UI_RGB(64, 72, 104);
    sUiEmber.track = UI_RGB(64, 72, 104);
    sUiS = &sUiEmber;
}

/* A pill with `text` on it, 15 pixels tall; BPillW is its width, x its left edge. */
UI_FN int BPillW(const char *text)
{
    return UiTextWidth(text, 1) + 14;
}

/* A sunk pill (the mockup's ipill): outline, a flat fill, a shade under its top edge and a light line under it. */
UI_FN void BPill(int x, int y, const char *text, uint16_t bg, uint16_t ink, uint16_t shadow)
{
    int w = BPillW(text);

    UiRoundFill(x, y, w, 15, 5, shadow);
    UiRoundFill(x + 1, y + 1, w - 2, 13, 4, bg);
    Fill(x + 3, y + 1, w - 6, 1, UiMix(bg, UI_RGB(0, 0, 0), 3));
    for (int cx = x + 4; cx < x + w - 4; ++cx)
        EmBlend(cx, y + 15, EM_WHITE, 56);
    UiText(text, x + 7, y + 3, ink, shadow, 1, x + w);
}

/* The Bag's CANCEL: slate (the mockup's .sl) with its round back arrow and the word under it; a tall one has the arrow and
 * the word as the mockup draws them, a short one only the word. */
static void BEmCancel(const CtrPokeTouchRect *r, int pressed)
{
    int fy;

    TsBgRect(r->x - 2, r->y - 2, r->w + 4, r->h + 4);
    if (!pressed)
        Em2CardShadow(r->x, r->y, r->w, r->h);
    fy = EmButton(r->x, r->y, r->w, r->h, &kEmSlate, B_DEPTH, pressed);
    if (r->h >= 40)
        BBackLabel(r, fy, r->h >= 70 ? 13 : 6, r->h >= 70 ? 47 : 34);
    else
        EmLabel(CTR_TR("Cancel", "Cancelar", "Zur\xC3\xBC" "ck"), r->x, r->w, fy, r->h - B_DEPTH, &kEmSlate);
}

/* ---- The battle Bag ---- */
/* Its title (the items: the balls have their button on the action screen) in flame and, with the Berry Pouch, BERRIES. */
static void BTabs(const CtrBattleView *v)
{
    const CtrPokeTouchRect *t = v->bag.berries ? &sBTitle : &sBTitleWide;
    int fy = EmButton(t->x, t->y, t->w, t->h, &kEmFlame, 2, 0);

    EmLabel(CTR_TR("Items", "Objetos", "Items"), t->x, t->w, fy, t->h - 2, &kEmFlame);
    if (v->bag.berries) {
        const CtrPokeTouchRect *r = &sBBerries;

        fy = EmButton(r->x, r->y, r->w, r->h, &kEmCream, 2, v->pressed == BUI_BAG_BERRIES);
        EmLabel(CTR_TR("Berries", "Bayas", "Beeren"), r->x, r->w, fy, r->h - 2, &kEmCream);
    }
}

/* A list row: the icon, the name and (but for key items) the quantity in a sunk pill, in blue as FireRed's "x12"; the Bag's selection flame with
 * FireRed's cursor and the select ring (the rows are 2 px apart: the ring fills the gap). */
static void BBagRow(int i, const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;
    CtrPokeTouchRect r = BRow(i);
    int chosen = b->first + i == b->cursor;
    const EmPal *p = chosen ? &kEmFlame : &kEmCream;
    int fy;

    if (!b->row[i].item)
        return;
    if (chosen)
        LlSelectRing(r.x, r.y, r.w, r.h);
    fy = EmButton(r.x, r.y, r.w, r.h, p, 2, v->pressed == BUI_BAG_ROW + i);
    BItemDraw(b->row[i].item, r.x + 3, fy, 1);
    if (chosen)
        LlCursor(r.x + 29, fy + 7, EM_WHITE);
    if (b->pocket != 1) {                       /* key items are one of each: no count */
        char qty[8];

        snprintf(qty, sizeof(qty), "x%u", b->row[i].qty > 999 ? 999 : b->row[i].qty);
        LlWell(r.x + r.w - 42, fy + 4, 34, 15);
        UiText(qty, r.x + r.w - 42 + (34 - UiTextWidth(qty, 1)) / 2, fy + 7, EM2_COUNT_INK, EM2_COUNT_SH, 1, r.x + r.w - 8);
    }
    Em2Text(b->row[i].name, r.x + (chosen ? 39 : 30), fy + 8, p->ink, p->inkSh, r.x + r.w - (b->pocket != 1 ? 46 : 8));
}

/* A scroll arrow: a small cream button with a red triangle (FireRed's), the triangle grey while the list cannot go that way. The button sits at the
 * top of the arrow's area going up, at its foot going down. */
/* The slider: the cursor index that puts the thumb, centred on y, where the finger is. */
int CtrBattleUi_BagRailTarget(const CtrBattleBag *b, int y)
{
    int railY = sBUpArrow.y + 22, railH = sBDownArrow.y + sBDownArrow.h - 22 - railY, left = b->count - BUI_BAG_ROWS, thumbH, first, t;

    if (left <= 0)
        return -1;
    thumbH = railH * BUI_BAG_ROWS / b->count;
    if (thumbH < 14)
        thumbH = 14;
    t = y - railY - thumbH / 2;
    t = t < 0 ? 0 : t > railH - thumbH ? railH - thumbH : t;
    first = (t * left * 2 + (railH - thumbH)) / (2 * (railH - thumbH));
    first = first > left ? left : first;
    t = first + (y < railY ? 0 : y >= railY + railH ? BUI_BAG_ROWS - 1 : (y - railY) * (BUI_BAG_ROWS - 1) / railH);
    return t > b->count - 1 ? b->count - 1 : t;
}

static void BBagArrow(const CtrPokeTouchRect *r, int down, int on)
{
    int fy = EmButton(r->x, down ? r->y + r->h - 20 : r->y, 20, 20, &kEmCream, 2, 0);

    LlArrow(r->x + 6, fy + (down ? 7 : 6), 8, 5, down ? 1 : 0, on ? LL_CURSOR : UI_RGB(184, 176, 168));
}

static void BBagList(const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;

    for (int i = 0; i < BUI_BAG_ROWS; ++i)
        BBagRow(i, v);
    if (b->count > BUI_BAG_ROWS) {
        int railY = sBUpArrow.y + 22, railH = sBDownArrow.y + sBDownArrow.h - 22 - railY, left = b->count - BUI_BAG_ROWS;
        int thumbH = railH * BUI_BAG_ROWS / b->count, thumbY;

        if (thumbH < 14)
            thumbH = 14;
        thumbY = (railH - thumbH) * (b->first < 0 ? 0 : b->first > left ? left : b->first) / left;
        BBagArrow(&sBUpArrow, 0, b->first > 0);
        BBagArrow(&sBDownArrow, 1, b->first + BUI_BAG_ROWS < b->count);
        LlScrollRail(sBUpArrow.x + 5, railY, 10, railH, thumbY, thumbH);
    }
}

/* The selected item's window: its icon on a plate, name, quantity and USE; with FireRed's menu open, the menu. */
static void BBagDetail(const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;
    const CtrPokeTouchRect *d = &sBDetail;
    CtrPokeTouchRect name = {(int16_t)(d->x + 7), 0, (int16_t)(d->w - 14), 13};
    char fit[BUI_ITEM_LEN + 4];

    LlWindow(d->x, d->y, d->w, d->h);
    if (b->menuCount > 0) {
        if (b->selItem) {
            BItemDraw(b->selItem, d->x + 9, d->y + 8, 1);
            Em2Text(b->selName, d->x + 37, d->y + 14, EM_DARK, EM_DARK_SH, d->x + d->w - 8);
        }
        for (int i = 0; i < b->menuCount && i < 3; ++i) {
            CtrPokeTouchRect r = BMenuRow(i);
            const EmPal *p = b->menuCursor == i ? &kEmFlame : i == b->menuCount - 1 ? &kEmSlate : &kEmCream;
            int fy;

            if (b->menuCursor == i)
                LlSelectRing(r.x, r.y, r.w, r.h);
            fy = EmButton(r.x, r.y, r.w, r.h, p, B_DEPTH, v->pressed == BUI_BAG_MENU + i);
            EmLabel(b->menuName[i], r.x, r.w, fy, r.h - B_DEPTH, p);
        }
        return;
    }
    if (!b->selItem) {
        Em2Fit(CTR_TR("Close the Bag", "Cerrar la mochila", "Beutel schlie\xC3\x9F" "en"), name.w, fit, sizeof(fit));
        UiTextCentered(fit, &name, d->y + 60, EM_DIM, EM_DARK_SH, 1);
        return;
    }
    LlPlate(d->x + d->w / 2 - 27, d->y + 7, 54, LL_SLATE, 0, 0);
    BItemDraw(b->selItem, d->x + d->w / 2 - 24, d->y + 10, 2);
    Em2Fit(b->selName, name.w, fit, sizeof(fit));
    UiTextCentered(fit, &name, d->y + 66, EM_DARK, EM_DARK_SH, 1);
    if (b->pocket != 1) {
        char qty[8];

        snprintf(qty, sizeof(qty), "x%u", b->selItem && BBagSelRow(b) >= 0 ? b->row[BBagSelRow(b)].qty : 0);
        if (BBagSelRow(b) >= 0)
            UiTextCentered(qty, &name, d->y + 81, EM2_COUNT_INK, EM2_COUNT_SH, 1);
    }
    if (BBagSelRow(b) >= 0) {
        int fy = EmButton(sBUse.x, sBUse.y, sBUse.w, sBUse.h, &kEmFlame, B_DEPTH, v->pressed == BUI_BAG_ROW + BBagSelRow(b));

        EmLabel(CTR_TR("Use", "Usar", "Benutzen"), sBUse.x, sBUse.w, fy, sBUse.h - B_DEPTH, &kEmFlame);
    }
}

/* The Bag's message over the dimmed Bag, in the text window (A, or a tap anywhere, goes on). */
static void BBagMessage(const CtrBattleView *v)
{
    BMessageWindow(v->bag.message);
}

static void BBag(const CtrBattleView *v)
{
    const CtrPokeTouchRect *n = &sBNote;

    TsBackdrop();
    BTabs(v);
    BBagList(v);
    BBagDetail(v);
    LlWindow(n->x, n->y, n->w, n->h);
    if (v->bag.selItem && v->bag.menuCount == 0)
        Em2Wrap(v->bag.selDesc, n->x + 13, n->y + 11, n->w - 26, 4, UI_LINE_H, EM_DIM, EM_DARK_SH);
    BEmCancel(&sBBagCancel, v->pressed == BUI_BAG_CANCEL);
    if (v->bag.message[0])
        BBagMessage(v);
}

/* The battle Bag, whole (CtrBattleUi_Draw's for it). */
static void BEmberDraw(uint16_t *canvas, const CtrBattleView *v)
{
    sCanvas = canvas;
    EmSurface();
    BBag(v);
    sUiS = &sUiNavy;
}

/* Each changed battle element is presented through the shared dirty-rectangle path. */
static void BMark(CtrDirtyList *dirty, const CtrPokeTouchRect *r)
{
    CtrDirty_Add(dirty, r->x, r->y, r->x + r->w, r->y + r->h);
}

/* From one Bag to the next (CtrBattleUi_Update's for it): any change draws all of it. */
static void BEmberUpdate(uint16_t *canvas, const CtrBattleView *old, const CtrBattleView *v, CtrDirtyList *dirty)
{
    CtrPokeTouchRect all = {0, 0, W, H};

    sCanvas = canvas;
    EmSurface();
    CtrDirty_Clear(dirty);
    if (memcmp(&old->bag, &v->bag, sizeof(v->bag)) || old->pressed != v->pressed) {
        BEmberDraw(canvas, v);
        EmSurface();
        BMark(dirty, &all);
    }
    sUiS = &sUiNavy;
}
