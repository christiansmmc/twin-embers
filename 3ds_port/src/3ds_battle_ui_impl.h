/* The battle screen (included at the end of 3ds_poketouch.c, whose drawing helpers and design system it uses).
 * The lower screen while FireRed's battle is on the upper one: the commands with the quick ball, the moves, the targets
 * and the Safari Zone's four are pages in the kit's look (3ds_battle_page_impl.h), and so is the Party, the field's
 * too (3ds_party_page_impl.h); the battle Bag still wears PokeTouch's ember look (3ds_touch_skin_impl.h) until its
 * turn in phase 2. Everything shown is what the controller reports. */

#include "3ds_icons.h"
#include "3ds_ui_strings.h"
#include "3ds_type_chart.h"
#include "3ds_battle_nav.h"

/* The party's balls, on their pill. */
static const CtrPokeTouchRect sBStrip = {70, 3, 180, 28};
/* The commands, FireRed's cursors 0-3 (FIGHT, BAG, POKeMON, RUN), and in a wild battle the quick ball
 * (CTR_ACTION_BALL) with its arrows: where 3ds_lower_layout.h puts them, the row with the quick ball apart. */
static const CtrPokeTouchRect *BActRect(const CtrBattleView *v, int i)
{
    if (i == CTR_ACTION_BALL)
        return &kBatBall;
    return &(v->ball == CTR_QUICK_BALL_NONE ? kBatAction : kBatActionBall)[i];
}
/* The battle Bag's note and CANCEL. */
static const CtrPokeTouchRect sBNote = {4, 176, 236, 60};
static const CtrPokeTouchRect sBCancel = {244, 180, 72, 52};
/* The battle Bag: its title (the Items pocket, the only one it shows) and BERRIES for the Berry Pouch, the item list
 * on the left with its scroll arrows, the selected item's card on the right, the description's note and CANCEL. */
static const CtrPokeTouchRect sBTitle = {6, 4, 204, 26}, sBTitleWide = {6, 4, 308, 26}, sBBerries = {214, 4, 100, 26};
static const int sBTabPocket[3] = {0, 2, 1};            /* FireRed's pockets in the order the tabs once showed them */
static const CtrPokeTouchRect sBList = {6, 36, 190, 138};
static const CtrPokeTouchRect sBDetail = {200, 36, 114, 138};
static const CtrPokeTouchRect sBUse = {210, 136, 94, 30};
static const CtrPokeTouchRect sBUpArrow = {176, 36, 20, 66}, sBDownArrow = {176, 108, 20, 66};
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
/* The Party is a page (3ds_party_page_impl.h, included further down). */
static int PPageHit(const CtrBattleView *v, int x, int y);

static int BIn(const CtrPokeTouchRect *r, int x, int y)
{
    return UiIn(r, x, y);
}

int CtrBattleUi_TabPocket(int tab)
{
    return tab >= 0 && tab < 3 ? sBTabPocket[tab] : -1;
}


/* The row of the Bag's selected item when it is on screen (USE stands for it), else -1. */
static int BBagSelRow(const CtrBattleBag *b)
{
    int i = b->cursor - b->first;

    return i >= 0 && i < BUI_BAG_ROWS && b->row[i].item ? i : -1;
}

int CtrBattleUi_Hit(const CtrBattleView *v, int x, int y)
{
    if (v->state == BUI_ACTION) {
        for (int i = 0; i < 4; ++i)
            if (BIn(BActRect(v, i), x, y))
                return i;
        if (v->ball != CTR_QUICK_BALL_NONE) {
            /* The arrows only while there is another ball to go to. */
            if (v->ball == CTR_QUICK_BALL_READY && v->ballCount > 1 && BIn(&kBatBallArrow[0], x, y))
                return BUI_ACT_BALL_PREV;
            if (v->ball == CTR_QUICK_BALL_READY && v->ballCount > 1 && BIn(&kBatBallArrow[1], x, y))
                return BUI_ACT_BALL_NEXT;
            if (BIn(&kBatBall, x, y))
                return BUI_ACT_BALL;
        }
    }
    if (v->state == BUI_MOVE || v->state == BUI_TARGET) {
        for (int i = 0; i < 4; ++i)
            if (v->state == BUI_MOVE ? v->move[i].present && BIn(&kBatMove[i], x, y)
                                     : v->target[i].present && BIn(&kBatTarget[i], x, y))
                return i;
        if (BIn(&kBatCancel, x, y))
            return BUI_CANCEL;
        return -1;
    }
    if (v->state == BUI_SAFARI) {
        for (int i = 0; i < 4; ++i)
            if (BIn(&kBatSafari[i], x, y))
                return i;
        return -1;
    }
    if (v->state == BUI_PARTY)
        return PPageHit(v, x, y);
    if (v->state == BUI_BAG) {
        const CtrBattleBag *b = &v->bag;

        if (b->message[0])
            return BUI_BAG_MESSAGE;                         /* anywhere: the message waits for A */
        if (BIn(&sBCancel, x, y))
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
        if (b->count > BUI_BAG_ROWS && BIn(&sBUpArrow, x, y) && b->first > 0)
            return BUI_BAG_UP;
        if (b->count > BUI_BAG_ROWS && BIn(&sBDownArrow, x, y) && b->first + BUI_BAG_ROWS < b->count)
            return BUI_BAG_DOWN;
        for (int i = 0; i < BUI_BAG_ROWS; ++i) {
            CtrPokeTouchRect r = BRow(i);
            if (b->row[i].item && BIn(&r, x, y))
                return BUI_BAG_ROW + i;
        }
    }
    return -1;
}

/* Appends `word` to `line` (a space first when it is not empty), within `size`. */
static void BAppend(char *line, size_t size, const char *word)
{
    size_t n = strlen(line);

    if (n && n + 1 < size)
        line[n++] = ' ';
    for (; *word && n + 1 < size; ++word)
        line[n++] = *word;
    line[n] = 0;
}

/* Running text wrapped to `width` pixels, `maxLines` at most (FireRed's line breaks are spaces here). */
static void BDescribeN(const char *text, int x, int y, int width, int maxLines, uint16_t color)
{
    char word[24], line[80], joined[80];
    int lines = 0;
    const char *p = text;

    line[0] = 0;
    while (*p && lines < maxLines) {
        int w = 0;

        while (*p == ' ' || *p == '\n')
            ++p;
        while (*p && *p != ' ' && *p != '\n' && w < (int)sizeof(word) - 1)
            word[w++] = *p++;
        word[w] = 0;
        if (!w)
            break;
        memcpy(joined, line, sizeof(joined));
        BAppend(joined, sizeof(joined), word);
        if (line[0] && UiTextWidth(joined, 1) > width) {
            UiText(line, x, y + lines * UI_LINE_H, color, UIS_TEXT_SH, 1, x + width);
            line[0] = 0;
            if (++lines >= maxLines)
                return;
            BAppend(line, sizeof(line), word);
        } else
            memcpy(line, joined, sizeof(line));
    }
    if (line[0] && lines < maxLines)
        UiText(line, x, y + lines * UI_LINE_H, color, UIS_TEXT_SH, 1, x + width);
}



/* The Party's icons (the Party page, the battle pages' party strip, HOME's party window). */
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

/* Text wrapped into `maxLines` lines of `width`, in `ink` over `shadow`. */
static void BPWrap(const char *text, int x, int y, int width, int maxLines, uint16_t ink, uint16_t shadow)
{
    char word[24], line[80], joined[80];
    int lines = 0;
    const char *p = text;

    line[0] = 0;
    while (*p && lines < maxLines) {
        int w = 0;

        while (*p == ' ' || *p == '\n')
            ++p;
        while (*p && *p != ' ' && *p != '\n' && w < (int)sizeof(word) - 1)
            word[w++] = *p++;
        word[w] = 0;
        if (!w)
            break;
        memcpy(joined, line, sizeof(joined));
        BAppend(joined, sizeof(joined), word);
        if (line[0] && UiTextWidth(joined, 1) > width) {
            UiText(line, x, y + lines * 16, ink, shadow, 1, x + width);
            line[0] = 0;
            if (++lines >= maxLines)
                return;
            BAppend(line, sizeof(line), word);
        } else
            memcpy(line, joined, sizeof(line));
    }
    if (line[0] && lines < maxLines)
        UiText(line, x, y + lines * 16, ink, shadow, 1, x + width);
}

/* The selected slot's icon hops with its frames, as FireRed's Party does: up on the first, down on the second. */
static int BPartyBounce(const CtrBattleParty *p, int i)
{
    return p->cursor == i ? (p->iconFrame[i] & 2 ? 1 : -3) : 0;
}

static const char *const sBPartyTagText[5] = {"", CTR_TR("ABLE", "PUEDE", "Kann"), CTR_TR("NOT ABLE", "NO PUEDE", "Nein"), CTR_TR("LEARNED", "APRENDIDO", "Kennt Att."), CTR_TR("NO USE", "NO SIRVE", "Nutzlos")};

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

/* The action menu's other colours: BAG blue, POKeMON green, RUN purple. */
static const EmPal kEmBlue = {UI_RGB(130, 188, 255), UI_RGB(44, 112, 224), UI_RGB(26, 70, 160), UI_RGB(14, 36, 96),
                              EM_WHITE, UI_RGB(14, 36, 96)};
static const EmPal kEmGreen = {UI_RGB(140, 224, 124), UI_RGB(44, 160, 72), UI_RGB(26, 110, 46), UI_RGB(12, 64, 24),
                               EM_WHITE, UI_RGB(12, 64, 24)};
static const EmPal kEmPurple = {UI_RGB(196, 168, 255), UI_RGB(122, 88, 222), UI_RGB(82, 56, 160), UI_RGB(44, 28, 96),
                                EM_WHITE, UI_RGB(44, 28, 96)};

/* A pill with `text` on it, 15 pixels tall; BPillW is its width, x its left edge. */
static int BPillW(const char *text)
{
    return UiTextWidth(text, 1) + 14;
}

static void BPill(int x, int y, const char *text, uint16_t bg, uint16_t ink, uint16_t shadow)
{
    int w = BPillW(text);

    UiRoundFill(x, y, w, 15, 5, UiMix(bg, UI_RGB(0, 0, 0), 3));
    UiRoundFill(x + 1, y + 1, w - 2, 13, 4, bg);
    UiText(text, x + 7, y + 3, ink, shadow, 1, x + w);
}

/* The menu's icons: FIGHT, the Poke Ball and the shoe drawn flat by tools/make_battle_icons.py, the game's own bag
 * from the user's pack (reduced to fit its disc), BAIT and ROCK the port's art (3ds_icons.h). */
#include "battle_flat_icons.h"
enum { BX_FIGHT, BX_BAG, BX_BALL, BX_SHOE, BX_BAIT, BX_ROCK };
#define BBAG_MAX 48
static struct {
    int ready, ok, w, h;
    uint16_t rgb[BBAG_MAX * BBAG_MAX];
    uint8_t a[BBAG_MAX * BBAG_MAX];
} sBBag;

/* FireRed's bag (the ITEMS frame of graphics/interface/bag_male) at most `maxH` tall, its edge blended. */
static void BBagPrepare(int maxH)
{
    static uint8_t tiles[64 * 256 / 2];
    uint8_t colors[32], px[64 * 64];
    uint16_t pal[16];
    uint32_t len = 0;
    uint8_t *packed;
    int n = 0, x0 = 64, y0 = 64, x1 = -1, y1 = -1, bw, bh, tw, th;
    float step;

    if (sBBag.ready)
        return;
    sBBag.ready = 1;
    packed = CtrData_Load("graphics/interface/bag_male.4bpp.lz", &len);
    if (packed) {
        n = PokeTouch_Lz77(packed, len, tiles, sizeof(tiles));
        free(packed);
    }
    if (n < 64 * 64 / 2 || PokeTouch_LoadLz("graphics/interface/bag.gbapal.lz", colors, sizeof(colors)) != 32)
        return;
    for (int i = 1; i < 16; ++i) {
        unsigned c = (unsigned)colors[i * 2] | (unsigned)colors[i * 2 + 1] << 8;
        pal[i] = GbaColor(c);
    }
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            uint8_t b = tiles[(y / 8 * 8 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2], v = (x & 1) ? b >> 4 : b & 15;

            px[y * 64 + x] = v;
            if (v) {
                x0 = x < x0 ? x : x0; x1 = x > x1 ? x : x1;
                y0 = y < y0 ? y : y0; y1 = y > y1 ? y : y1;
            }
        }
    if (x1 < 0)
        return;
    bw = x1 - x0 + 1;
    bh = y1 - y0 + 1;
    step = bh > maxH ? (float)bh / maxH : 1.0f;
    tw = (int)(bw / step + 0.5f);
    th = (int)(bh / step + 0.5f);
    if (tw > BBAG_MAX || th > BBAG_MAX)
        return;
    /* Area-averaged: each pixel the colours of the source pixels it covers (only the drawn ones), its cover as alpha. */
    for (int oy = 0; oy < th; ++oy)
        for (int ox = 0; ox < tw; ++ox) {
            float fx0 = ox * step, fx1 = fx0 + step, fy0 = oy * step, fy1 = fy0 + step, wsum = 0, wop = 0, r = 0, g = 0, b = 0;

            for (int sy = (int)fy0; sy < bh && sy < fy1; ++sy)
                for (int sx = (int)fx0; sx < bw && sx < fx1; ++sx) {
                    float wx = (sx + 1 < fx1 ? sx + 1 : fx1) - (sx > fx0 ? sx : fx0);
                    float wy = (sy + 1 < fy1 ? sy + 1 : fy1) - (sy > fy0 ? sy : fy0);
                    float w = wx * wy;
                    uint8_t v = px[(y0 + sy) * 64 + x0 + sx];

                    wsum += w;
                    if (v) {
                        wop += w;
                        r += w * (pal[v] >> 11);
                        g += w * ((pal[v] >> 5) & 63);
                        b += w * (pal[v] & 31);
                    }
                }
            sBBag.a[oy * tw + ox] = wop > 0 && wsum > 0 ? (uint8_t)(wop / wsum * 255.0f + 0.5f) : 0;
            sBBag.rgb[oy * tw + ox] = wop > 0 ? (uint16_t)((int)(r / wop + 0.5f) << 11 | (int)(g / wop + 0.5f) << 5
                                                           | (int)(b / wop + 0.5f)) : 0;
        }
    sBBag.w = tw;
    sBBag.h = th;
    sBBag.ok = 1;
}

static void BFlat(const uint16_t *px, int w, int h, int x, int y)
{
    for (int v = 0; v < h; ++v)
        for (int u = 0; u < w; ++u)
            if (px[v * w + u])
                Px(x + u, y + v, px[v * w + u]);
}

/* A light disc of the piece's colour, `r` its radius, centred at (cx, cy). */
static void BDisc(int cx, int cy, int r, const EmPal *p)
{
    UiDisc(cx, cy, r + 1, UiMix(p->bottom, p->lip, 3));
    UiDisc(cx, cy, r, UiMix(p->top, EM_WHITE, 4));
}

/* Icon `icon` (BX_*) centred at (cx, cy) on a light disc of the piece's colour, `r` its radius. */
/* Unused since the commands moved to the kit (3ds_battle_page_impl.h); goes with the ember look in phase 2. */
UI_FN void BDiscIcon(int icon, int cx, int cy, int r, const EmPal *p)
{
    BDisc(cx, cy, r, p);
    if (icon == BX_FIGHT)
        BFlat(kFlatFight, FLATFIGHT_W, FLATFIGHT_H, cx - FLATFIGHT_W / 2, cy - FLATFIGHT_H / 2);
    else if (icon == BX_BALL)
        BFlat(kFlatBall, FLATBALL_W, FLATBALL_H, cx - FLATBALL_W / 2, cy - FLATBALL_H / 2);
    else if (icon == BX_SHOE)
        BFlat(kFlatShoe, FLATSHOE_W, FLATSHOE_H, cx - FLATSHOE_W / 2, cy - FLATSHOE_H / 2);
    else if (icon == BX_BAG && (BBagPrepare(2 * r - 2), sBBag.ok)) {
        for (int v = 0; v < sBBag.h; ++v)
            for (int u = 0; u < sBBag.w; ++u)
                EmBlend(cx - sBBag.w / 2 + u, cy - sBBag.h / 2 + v, sBBag.rgb[v * sBBag.w + u], sBBag.a[v * sBBag.w + u]);
    } else {
        int art = icon == BX_BAG ? BI_BAG : icon == BX_BAIT ? BI_BAIT : BI_ROCK, w, h;

        BIconsPrepare();
        w = sBImg[art].image.w * sBImg[art].scale;
        h = sBImg[art].image.h * sBImg[art].scale;
        BIconAt(art, cx - w / 2, cy - h / 2);
    }
}

/* CANCEL: violet, as BACK and the Party's. */
static void BEmCancel(const CtrPokeTouchRect *r, int pressed)
{
    int fy;

    TsBgRect(r->x, r->y, r->w, r->h);
    fy = EmButton(r->x, r->y, r->w, r->h, &kEmViolet, B_DEPTH, pressed);
    EmLabel(CTR_TR("CANCEL", "CANCELAR", "Zur\xC3\xBC" "ck"), r->x, r->w, fy, r->h - B_DEPTH, &kEmViolet);
}

/* A Poke Ball of the party strip, 15 pixels across: red and white for a Pokemon that can fight, grey for one that
 * fainted (kind 2), a hollow ring for no Pokemon (kind 0); an amber dot for a status condition. */
static void BBall(int cx, int cy, int kind, int status)
{
    for (int dy = -7; dy <= 7; ++dy)
        for (int dx = -7; dx <= 7; ++dx) {
            int d2 = dx * dx + dy * dy;
            uint16_t c;

            if (d2 > 52)
                continue;
            if (kind == 0) {
                if (d2 >= 36)
                    Px(cx + dx, cy + dy, UI_RGB(120, 56, 52));
                continue;
            }
            if (d2 >= 40 || dy == 0)
                c = UI_RGB(40, 24, 28);
            else if (d2 <= 5)
                c = d2 <= 2 ? EM_WHITE : UI_RGB(40, 24, 28);
            else if (dy < 0)
                c = kind == 2 ? UI_RGB(150, 146, 156) : dx < -1 && dy < -3 ? UI_RGB(255, 140, 120) : UI_RGB(230, 64, 52);
            else
                c = kind == 2 ? UI_RGB(196, 192, 200) : UI_RGB(244, 242, 248);
            Px(cx + dx, cy + dy, c);
        }
    if (kind && kind != 2 && status) {
        UiDisc(cx + 6, cy - 6, 3, UI_RGB(110, 40, 6));
        UiDisc(cx + 6, cy - 6, 2, UI_RGB(255, 200, 60));
    }
}

/* ---- The action menu ---- */
/* A hit code of the quick ball: the button or one of its arrows. */
static int BBallPart(int code)
{
    return code >= BUI_ACT_BALL && code <= BUI_ACT_BALL_NEXT;
}

/* ---- The Safari Zone: BALL, BAIT, ROCK, RUN (the words of FireRed's gText_SafariZoneMenu) ---- */
/* ---- The battle Bag ---- */
/* Its title (the items: the balls have their button on the action screen) and, with the Berry Pouch, BERRIES. */
static void BTabs(const CtrBattleView *v)
{
    const CtrPokeTouchRect *t = v->bag.berries ? &sBTitle : &sBTitleWide;
    int fy = EmButton(t->x, t->y, t->w, t->h, &kEmAmber, 2, 0);

    EmLabel(CTR_TR("ITEMS", "OBJETOS", "Items"), t->x, t->w, fy, t->h - 2, &kEmAmber);
    if (v->bag.berries) {
        const CtrPokeTouchRect *r = &sBBerries;

        fy = EmButton(r->x, r->y, r->w, r->h, &kEmCream, 2, v->pressed == BUI_BAG_BERRIES);
        EmLabel(CTR_TR("BERRIES", "BAYAS", "Beeren"), r->x, r->w, fy, r->h - 2, &kEmCream);
    }
}

/* A list row: the icon, the name and (but for key items) the quantity; the Bag's selection amber. */
static void BBagRow(int i, const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;
    CtrPokeTouchRect r = BRow(i);
    const EmPal *p = b->first + i == b->cursor ? &kEmAmber : &kEmCream;
    int fy;

    if (!b->row[i].item)
        return;
    fy = EmButton(r.x, r.y, r.w, r.h, p, 2, v->pressed == BUI_BAG_ROW + i);
    BItemDraw(b->row[i].item, r.x + 3, fy, 1);
    UiText(b->row[i].name, r.x + 30, fy + 8, p->ink, p->inkSh, 1, r.x + r.w - 34);
    if (b->pocket != 1) {                       /* key items are one of each: no count */
        char qty[8];

        snprintf(qty, sizeof(qty), "x%u", b->row[i].qty > 999 ? 999 : b->row[i].qty);
        UiTextRight(qty, r.x + r.w - 8, fy + 8, p->ink, p->inkSh, 1);
    }
}

/* A scroll arrow: a dark pill, its arrow lit while the list can go that way. */
static void BBagArrow(const CtrPokeTouchRect *r, int dir, int on)
{
    UiRoundFill(r->x, r->y, r->w, r->h, 5, UI_RGB(24, 8, 10));
    EmGradFill(r->x + 1, r->y + 1, r->w - 2, r->h - 2, 4, UI_RGB(72, 26, 26), UI_RGB(44, 14, 16));
    UiArrow(r->x + 7, r->y + r->h / 2 - 3, dir, on ? EM_GLOW : UI_RGB(110, 50, 44));
}

static void BBagList(const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;

    for (int i = 0; i < BUI_BAG_ROWS; ++i)
        BBagRow(i, v);
    if (b->count > BUI_BAG_ROWS) {
        BBagArrow(&sBUpArrow, -2, b->first > 0);
        BBagArrow(&sBDownArrow, 2, b->first + BUI_BAG_ROWS < b->count);
    }
}

/* The selected item's card: its icon on a plate, name, quantity and USE; with FireRed's menu open, the menu. */
static void BBagDetail(const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;
    const CtrPokeTouchRect *d = &sBDetail;
    CtrPokeTouchRect name = {d->x, 0, d->w, 13};

    UiRoundFill(d->x, d->y, d->w, d->h, 5, kEmCream.edge);
    EmGradFill(d->x + 1, d->y + 1, d->w - 2, d->h - 2, 4, kEmCream.top, kEmCream.bottom);
    if (b->menuCount > 0) {
        if (b->selItem) {
            BItemDraw(b->selItem, d->x + 6, d->y + 8, 1);
            UiText(b->selName, d->x + 34, d->y + 14, EM_DARK, EM_DARK_SH, 1, d->x + d->w - 4);
        }
        for (int i = 0; i < b->menuCount && i < 3; ++i) {
            CtrPokeTouchRect r = BMenuRow(i);
            const EmPal *p = b->menuCursor == i ? &kEmAmber : i == b->menuCount - 1 ? &kEmViolet : &kEmCream;
            int fy = EmButton(r.x, r.y, r.w, r.h, p, B_DEPTH, v->pressed == BUI_BAG_MENU + i);

            EmLabel(b->menuName[i], r.x, r.w, fy, r.h - B_DEPTH, p);
        }
        return;
    }
    if (!b->selItem) {
        UiTextCentered(CTR_TR("Close the Bag", "Cerrar la MOCHILA", "Beutel schlie\xC3\x9F" "en"), &name, d->y + 60, EM_DIM, EM_DARK_SH, 1);
        return;
    }
    UiDisc(d->x + d->w / 2, d->y + 34, 27, UiMix(kEmCream.bottom, kEmCream.edge, 2));
    UiDisc(d->x + d->w / 2, d->y + 34, 26, UI_RGB(255, 226, 196));
    BItemDraw(b->selItem, d->x + d->w / 2 - 24, d->y + 10, 2);
    UiTextCentered(b->selName, &name, d->y + 66, EM_DARK, EM_DARK_SH, 1);
    if (b->pocket != 1) {
        char qty[8];

        snprintf(qty, sizeof(qty), "x%u", b->selItem && BBagSelRow(b) >= 0 ? b->row[BBagSelRow(b)].qty : 0);
        if (BBagSelRow(b) >= 0)
            UiTextCentered(qty, &name, d->y + 81, UI_RGB(52, 104, 220), EM_DARK_SH, 1);
    }
    if (BBagSelRow(b) >= 0) {
        int fy = EmButton(sBUse.x, sBUse.y, sBUse.w, sBUse.h, &kEmAmber, B_DEPTH, v->pressed == BUI_BAG_ROW + BBagSelRow(b));

        EmLabel(CTR_TR("USE", "USAR", "Benutzen"), sBUse.x, sBUse.w, fy, sBUse.h - B_DEPTH, &kEmAmber);
    }
}

/* The Bag's message over the dimmed Bag, in a card (A, or a tap anywhere, goes on). */
static void BBagMessage(const CtrBattleView *v)
{
    CtrPokeTouchRect all = {0, 0, W, H}, box = {20, 70, 280, 76};

    UiDim(&all);
    EmNote(box.x, box.y, box.w, box.h);
    BPWrap(v->bag.message, box.x + 14, box.y + 10, box.w - 24, 3, EM_DARK, EM_DARK_SH);
    UiText(CTR_TR("Tap to continue", "Toca para seguir", "Tippen zum Fortfahren"), box.x + box.w - 14 - UiTextWidth(CTR_TR("Tap to continue", "Toca para seguir", "Tippen zum Fortfahren"), 1) - 9, box.y + box.h - 16,
           EM_DIM, EM_DARK_SH, 1, box.x + box.w);
    UiArrow(box.x + box.w - 18, box.y + box.h - 16, 1, EM_STRIPE);
}

static void BBag(const CtrBattleView *v)
{
    const CtrPokeTouchRect *n = &sBNote;

    TsBackdrop();
    BTabs(v);
    BBagList(v);
    BBagDetail(v);
    EmNote(n->x, n->y, n->w, n->h);
    if (v->bag.selItem && v->bag.menuCount == 0)
        BDescribeN(v->bag.selDesc, n->x + 12, n->y + 9, n->w - 18, 4, EM_DARK);
    BEmCancel(&sBCancel, v->pressed == BUI_BAG_CANCEL);
    if (v->bag.message[0])
        BBagMessage(v);
}

/* A screen with the party's balls on top. */
#include "3ds_battle_page_impl.h"
/* POKeMON, field and battle, as a page. */
#include "3ds_party_page_impl.h"
/* The BAG's pieces as a page (the field's Bag and the battle's). */
#include "3ds_bag_page_impl.h"

/* Every state but the Bag and the Party: a page (3ds_battle_page_impl.h). */
static void BScreen(const CtrBattleView *v)
{
    BPageScreen(v);
}

void CtrBattleUi_Draw(uint16_t *canvas, const CtrBattleView *v)
{
    sCanvas = canvas;
    EmSurface();
    if (v->state == BUI_PARTY)
        PPageScreen(v);
    else if (v->state == BUI_BAG)
        BBag(v);
    else
        BScreen(v);
    sUiS = &sUiNavy;
}

/* Each changed battle element is presented through the shared dirty-rectangle path. */
static void BMark(CtrDirtyList *dirty, const CtrPokeTouchRect *r)
{
    CtrDirty_Add(dirty, r->x, r->y, r->x + r->w, r->y + r->h);
}

/* Whether a screen with the balls must be drawn again whole: another screen, other balls, other cards. */
static int BScreenChanged(const CtrBattleView *a, const CtrBattleView *b)
{
    if (a->state != b->state || a->frameType != b->frameType || memcmp(a->party, b->party, sizeof(b->party)) || memcmp(a->partyHp, b->partyHp, sizeof(b->partyHp))
        || memcmp(a->partyStatus, b->partyStatus, sizeof(b->partyStatus)) || memcmp(a->partyIcon, b->partyIcon, sizeof(b->partyIcon)))
        return 1;
    if (b->state == BUI_ACTION)
        return strcmp(a->activeName, b->activeName) || a->activeLevel != b->activeLevel || a->activeIcon != b->activeIcon
            || a->activeSlot != b->activeSlot || a->ball != b->ball || a->ballItem != b->ballItem
            || a->ballQty != b->ballQty || a->ballPlace != b->ballPlace || a->ballCount != b->ballCount
            || strcmp(a->ballName, b->ballName);
    if (b->state == BUI_MOVE)
        return memcmp(a->move, b->move, sizeof(b->move)) != 0;
    if (b->state == BUI_TARGET)
        return memcmp(a->target, b->target, sizeof(b->target)) != 0;
    return 0;
}

void CtrBattleUi_Update(uint16_t *canvas, const CtrBattleView *old, const CtrBattleView *v, CtrDirtyList *dirty)
{
    CtrPokeTouchRect all = {0, 0, W, H};

    sCanvas = canvas;
    EmSurface();
    CtrDirty_Clear(dirty);
    if (v->state == BUI_PARTY && old->state == BUI_PARTY) {
        PPageUpdate(old, v, dirty);
    } else if (v->state == BUI_PARTY || old->state == BUI_PARTY) {
        CtrBattleUi_Draw(canvas, v);
        EmSurface();
        BMark(dirty, &all);
    } else if (v->state == BUI_BAG || old->state == BUI_BAG) {
        if (old->state != v->state || memcmp(&old->bag, &v->bag, sizeof(v->bag)) || old->pressed != v->pressed) {
            CtrBattleUi_Draw(canvas, v);
            EmSurface();
            BMark(dirty, &all);
        }
    } else if (BScreenChanged(old, v)) {
        CtrBattleUi_Draw(canvas, v);
        EmSurface();
        BMark(dirty, &all);
    } else {
        BPageUpdate(old, v, dirty);
    }
    sUiS = &sUiNavy;
}
