/* The battle screen (3ds_battle_ui_impl.h) through the production renderer with synthetic game bridges.
 *
 * Every state of the controller (actions, moves, targets, Safari, Bag with its menu and message, Party with its modals)
 * is drawn; each press, cursor move and state change must leave the canvas exactly as a full redraw would (the dirty
 * rectangles are complete); the screen is the PokeTouch red (no navy of the shared theme remains in it) and the buttons'
 * hit areas answer to the same points as before. No ROM-derived data: the font is blank unless BATTLE_UI_FONT names
 * a 256x64-byte glyph file (a local viewing aid), and BATTLE_UI_OUT names a directory for one PPM per state.
 *
 * usage: battle_ui_render_test [print-hits] */
#include <assert.h>
#ifndef PT_SOURCE
#define PT_SOURCE "3ds_poketouch.c"
#endif
#include PT_SOURCE
#include <math.h>

static uint8_t font[256 * 64];
const uint8_t *CtrTwinFontData(void) { return font; }
unsigned CtrTwinFontWidth(unsigned code) { return code == 0xEF ? 8 : 6; }
/* FireRed's menu arrow (0xEF) as a solid 8x14 block of text ink, so a test can find it; every other glyph stays blank. */
static void ArrowGlyph(void)
{
    for (int y = 0; y < 14; ++y)
        for (int x = 0; x < 8; ++x) {
            uint8_t *src = font + 0xEF * 64 + ((y / 8) * 2 + x / 8) * 16 + (y % 8) * 2;
            unsigned bits = ((unsigned)src[1] << 8 | src[0]) | 1u << ((7 - x % 8) * 2);

            src[0] = (uint8_t)bits;
            src[1] = (uint8_t)(bits >> 8);
        }
}
const uint8_t *CtrTwinJapaneseGlyph(unsigned code, uint8_t out[64]) { (void)code; memset(out, 0, 64); return out; }
unsigned CtrTwinJapaneseFontWidth(unsigned code) { (void)code; return 8; }
static uint8_t smallFont[256 * 32], smallWidth[256];
const uint8_t *CtrTwinSmallFontData(void) { return smallFont; }
unsigned CtrTwinSmallFontWidth(unsigned code) { return code < 256 && smallWidth[code] ? smallWidth[code] : 5; }
#ifndef BATTLE_UI_EXTERNAL_LOAD
/* The pack's two item icons, synthesised: a literal-only GBA LZ stream of 24x24 4bpp tiles (3x3) and of a palette, so
 * the full-size icon path runs without game data. With BATTLE_UI_NO_PACK nothing is found and the fallbacks draw. */
int gNoPack;
void *CtrData_Load(const char *path, uint32_t *len)
{
    uint8_t raw[288];
    int palette = strstr(path, "icon_palettes/") != NULL;
    unsigned size = palette ? 32u : 288u;
    uint8_t *out;
    unsigned pos = 4;

    if (gNoPack || !strstr(path, "graphics/items/icon"))
        return NULL;
    memset(raw, 0, sizeof(raw));
    if (palette) {
        raw[2] = 0xff; raw[3] = 0x03;               /* index 1: green */
    } else {
        for (unsigned i = 0; i < 288; ++i)
            raw[i] = (uint8_t)((i % 32) < 16 ? 0x11 : 0x00);   /* stripes, so tile order shows */
        raw[0] = 0x21;                              /* the very first pixel: index 1, the second 2 */
    }
    *len = 4 + size + (size + 7) / 8;
    out = malloc(*len);
    assert(out);
    out[0] = 0x10; out[1] = (uint8_t)size; out[2] = (uint8_t)(size >> 8); out[3] = 0;
    for (unsigned i = 0; i < size; ++i) {
        if (!(i % 8)) out[pos++] = 0;
        out[pos++] = raw[i];
    }
    return out;
}
#endif
int CtrPokeTouch_DexMonPic(unsigned species, unsigned char *tiles, unsigned short *palette)
{ (void)species; (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_DexMonPic80(unsigned species, unsigned char *pixels, unsigned short *palette)
{ (void)species; (void)pixels; (void)palette; return 0; }
int CtrPokeTouch_DexMenuIcon(int id, unsigned char *tiles, unsigned short *palette)
{ (void)id; (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_DexFootprint(unsigned species, unsigned char *bits)
{ (void)species; (void)bits; return 0; }
int CtrPokeTouch_DexCaughtMarker(unsigned char *tiles, unsigned short *palette)
{ (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_CardTrainerPic(unsigned char *tiles, unsigned short *palette)
{ (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_CardBadges(unsigned char *tiles, unsigned short *palette)
{ (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_MonIcon(unsigned species, unsigned personality, int egg, unsigned char *tiles, unsigned short *palette)
{
    (void)species; (void)personality; (void)egg; (void)tiles; (void)palette;
    return 0;
}
int CtrPokeTouch_ItemIcon(unsigned item, unsigned char *tiles, unsigned short *palette)
{
    memset(tiles, 0x11, 288); memset(palette, 0, 32);
    palette[1] = item == 1 ? 31 : item == 2 ? 31 << 5 : 31 << 10;
    return item != 0;
}
int CtrPokeTouch_MapArt(int group, uint16_t *map, uint8_t *tiles, uint16_t *pal, uint16_t *bgMap, uint8_t *bgTiles)
{ (void)group; (void)map; (void)tiles; (void)pal; (void)bgMap; (void)bgTiles; return 0; }
int CtrPokeTouch_MapPlayerIcon(uint8_t *tiles, uint16_t *pal) { (void)tiles; (void)pal; return 0; }
int CtrPokeTouch_MarkShape(int mark, unsigned char *indexes) { (void)mark; (void)indexes; return 0; }
int CtrPokeTouch_MapCursorIcon(uint8_t *tiles, uint16_t *pal) { (void)tiles; (void)pal; return 0; }
int CtrBottomParty_Icon(int slot, unsigned char *tiles, unsigned short *pal)
{
    /* A flat 32x32 icon in the slot's own colour, so the plate and the faint tone are visible. */
    memset(tiles, 0x11, 512); memset(pal, 0, 32);
    pal[1] = (unsigned short)((slot * 5 + 6) | (31 - slot * 4) << 5 | (slot * 3) << 10);
    return 1;
}

unsigned CtrBottomSummary_IconSpecies(void) { return 0; }
int CtrBottomSummary_Icon(unsigned char *tiles, unsigned short *pal)
{ (void)tiles; (void)pal; return 0; }
int CtrBottomSummary_Pic(unsigned char *tiles, unsigned short *pal, int *flip)
{ (void)tiles; (void)pal; (void)flip; return 0; }
int CtrBottomSummary_Pic80(unsigned char *pixels, unsigned short *pal, int *flip)
{ (void)pixels; (void)pal; (void)flip; return 0; }
int CtrPokeTouch_Wallpaper(unsigned id, unsigned short *rgb555)
{ (void)id; (void)rgb555; return 0; }

static uint16_t canvas[W * H], reference[W * H];
static uint16_t at(int x, int y) { return canvas[x * H + H - 1 - y]; }

/* The state last drawn: the action menu and the Bag were redrawn for the quick ball (issue #16), so their geometry
 * is checked by the assertions on them below, not against the renderer before the restyle. */
static int sState;

static void settle(const CtrBattleView *old, const CtrBattleView *v)
{
    CtrDirtyList dirty;

    sState = v->state;

    CtrDirty_Clear(&dirty);
    CtrBattleUi_Update(canvas, old, v, &dirty);
    memcpy(reference, canvas, sizeof(canvas));
    CtrBattleUi_Draw(canvas, v);
    if (memcmp(reference, canvas, sizeof(canvas))) {
        for (int x = 0; x < W; ++x)
            for (int y = 0; y < H; ++y)
                if (reference[x * H + H - 1 - y] != canvas[x * H + H - 1 - y]) {
                    fprintf(stderr, "battle redraw mismatch at %d,%d state=%u pressed=%d cursor=%d ball=%u"
                            " (from state=%u pressed=%d cursor=%d ball=%u)\n", x, y, v->state, v->pressed, v->cursor,
                            v->ball, old->state, old->pressed, old->cursor, old->ball);
                    for (size_t n = 0; n < sizeof(*v); ++n)       /* the view's bytes that changed, by offset */
                        if (((const uint8_t *)old)[n] != ((const uint8_t *)v)[n])
                            fprintf(stderr, "  view byte %zu: %u -> %u\n", n, ((const uint8_t *)old)[n], ((const uint8_t *)v)[n]);
                    assert(0);
                }
    }
}

/* The silhouette of a drawn state: which pixels are not the backdrop's. Restyling must not move an edge. */
static unsigned sMaskCrc;
static void mask(void)
{
    uint16_t bg = at(1, 100);

    if (sState == BUI_ACTION || sState == BUI_BAG)
        return;

    for (int x = 0; x < W; ++x)
        for (int y = 0; y < H; ++y)
            sMaskCrc = sMaskCrc * 33u + (at(x, y) != bg);
}

#ifndef BATTLE_UI_BEFORE
/* WCAG contrast of two RGB565 colours. */
static double lin(unsigned v) { double c = v / 255.0; return c <= 0.03928 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4); }
static double lum(uint16_t c)
{
    return 0.2126 * lin((c >> 11) << 3 | (c >> 13)) + 0.7152 * lin(((c >> 5) & 63) << 2 | ((c >> 5) & 63) >> 4)
         + 0.0722 * lin((c & 31) << 3 | (c & 31) >> 2);
}
static double contrast(uint16_t a, uint16_t b)
{
    double la = lum(a), lb = lum(b);

    return la > lb ? (la + 0.05) / (lb + 0.05) : (lb + 0.05) / (la + 0.05);
}

/* Recommendations for text on a game UI (WCAG 2.x): 4.5:1 for small text, 3:1 for large (2x and 3x labels and
 * UI components). The red surfaces and the button colours the battle screen draws white on are held to them. */
static void legibility(void)
{
    const UiSurface *red = &sUiRed;
    const UiButtonStyle *big[4] = {&red->fight, &red->bag, &red->pokemon, &red->run};
    const UiButtonStyle *small[3] = {&red->primary, &red->neutral, &red->cancel};
    const uint16_t surfaces[3] = {red->bg, red->panelLo, red->panel};

    for (int i = 0; i < 4; ++i) {
        assert(contrast(UI_TEXT, big[i]->body) >= 3.0 && contrast(UI_TEXT, big[i]->lit) >= 3.0);
    }
    for (int i = 0; i < 3; ++i)
        assert(contrast(UI_TEXT, small[i]->body) >= 4.5 && contrast(UI_TEXT, small[i]->lit) >= 4.5);
    for (int i = 0; i < 3; ++i) {
        assert(contrast(UI_TEXT, surfaces[i]) >= 4.5);
        assert(contrast(red->textDim, surfaces[i]) >= 4.5);
    }
    assert(contrast(UI_TEXT, UI_SELECT) >= 4.5 && contrast(UI_TEXT, UI_SELECT_HI) >= 4.5);
    /* The type colours behind a move's name or a badge: the ink is chosen to read on every one. */
    for (int t = 0; t < 18; ++t) {
        uint16_t c = UiTypeColor(t), ink = UiInkOn(c);

        assert(contrast(ink, c) >= 3.0);
    }
}
#endif

static void put(const char *dir, const char *name)
{
    char path[512];
    FILE *f;

    /* A modal dims the screen behind it, and a halved colour can land on the backdrop by chance: not a silhouette. */
    if (!strstr(name, "menu") && !strstr(name, "message") && !strstr(name, "yesno"))
        mask();
    if (getenv("BATTLE_UI_MASKS")) fprintf(stderr, "%s %08x\n", name, sMaskCrc);
    if (!dir)
        return;
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            unsigned c = at(x, y);
            fputc((int)((c >> 11) << 3), f);
            fputc((int)(((c >> 5) & 63) << 2), f);
            fputc((int)((c & 31) << 3), f);
        }
    fclose(f);
}

static unsigned sHitCrc;
static void hits(const CtrBattleView *v)
{
    if (v->state == BUI_ACTION || v->state == BUI_BAG)
        return;
    for (int y = 0; y < H; y += 3)
        for (int x = 0; x < W; x += 3) {
            int h = CtrBattleUi_Hit(v, x, y);

            sHitCrc = sHitCrc * 31u + (unsigned)(h + 7);
        }
}

static void settle_summary(const CtrSummaryUiView *old, const CtrSummaryUiView *v)
{
    CtrDirtyList dirty;

    CtrDirty_Clear(&dirty);
    CtrSummaryUi_Update(canvas, old, v, &dirty);
    memcpy(reference, canvas, sizeof(canvas));
    CtrSummaryUi_Draw(canvas, v);
    if (memcmp(reference, canvas, sizeof(canvas))) {
        for (int x = 0; x < W; ++x)
            for (int y = 0; y < H; ++y)
                if (reference[x * H + H - 1 - y] != canvas[x * H + H - 1 - y]) {
                    fprintf(stderr, "summary redraw mismatch at %d,%d page=%d pressed=%d\n", x, y, v->data.page, v->pressed);
                    assert(0);
                }
    }
}

static void summary_hits(const CtrSummaryUiView *v)
{
    for (int y = 0; y < H; y += 3)
        for (int x = 0; x < W; x += 3)
            sHitCrc = sHitCrc * 31u + (unsigned)(CtrSummaryUi_Hit(v, x, y) + 7);
}

static void names(char *dst, size_t n, const char *s) { snprintf(dst, n, "%s", s); }

#ifndef BATTLE_UI_BEFORE
/* Text ink inside a framed cell's interior (8 px in): only the arrow draws it here (the test font is blank otherwise). */
static int InkIn(const CtrPokeTouchRect *r)
{
    int n = 0;

    for (int x = r->x + 8; x < r->x + r->w - 8; ++x)
        for (int y = r->y + 8; y < r->y + r->h - 8; ++y)
            n += at(x, y) == KitInk(2);
    return n;
}

static int ColorIn(const CtrPokeTouchRect *r, uint16_t c)
{
    int n = 0;

    for (int x = r->x; x < r->x + r->w; ++x)
        for (int y = r->y; y < r->y + r->h; ++y)
            n += at(x, y) == c;
    return n;
}

/* action_hit_centres: every command, the quick ball and its arrows answer at their rectangles' centres. */
static void action_hit_centres(const CtrBattleView *v)
{
    const CtrPokeTouchRect *row = v->ball == CTR_QUICK_BALL_NONE ? kBatAction : kBatActionBall;

    for (int i = 0; i < 4; ++i)
        assert(CtrBattleUi_Hit(v, row[i].x + row[i].w / 2, row[i].y + row[i].h / 2) == i);
    if (v->ball != CTR_QUICK_BALL_NONE) {
        assert(CtrBattleUi_Hit(v, kBatBall.x + kBatBall.w / 2, kBatBall.y + kBatBall.h / 2) == BUI_ACT_BALL);
        if (v->ball == CTR_QUICK_BALL_READY && v->ballCount > 1) {
            assert(CtrBattleUi_Hit(v, kBatBallArrow[0].x + 12, kBatBallArrow[0].y + 12) == BUI_ACT_BALL_PREV);
            assert(CtrBattleUi_Hit(v, kBatBallArrow[1].x + 12, kBatBallArrow[1].y + 12) == BUI_ACT_BALL_NEXT);
        }
    }
    assert(CtrBattleUi_Hit(v, kBatParty.x + 4, kBatParty.y + 4) == -1);
}

/* arrow_follows_cursor_and_press: the arrow (a solid 8x14 here) is in the cell of FireRed's cursor, or in the one held
 * under a touch; never in two. */
static void arrow_follows_cursor_and_press(const CtrBattleView *base)
{
    CtrBattleView v = *base, old = *base;

    v.state = BUI_ACTION; v.ball = CTR_QUICK_BALL_NONE; v.showCursor = 1; v.cursor = 2; v.pressed = -1;
    settle(&old, &v); old = v;
    for (int i = 0; i < 4; ++i)
        assert(InkIn(&kBatAction[i]) == (i == 2 ? 8 * 14 : 0));
    v.pressed = 1; settle(&old, &v); old = v;
    for (int i = 0; i < 4; ++i)
        assert(InkIn(&kBatAction[i]) == (i == 1 ? 8 * 14 : 0));
    v.state = BUI_SAFARI; v.pressed = -1; v.cursor = 3; settle(&old, &v); old = v;
    for (int i = 0; i < 4; ++i)
        assert(InkIn(&kBatSafari[i]) == (i == 3 ? 8 * 14 : 0));
    v.pressed = 0; settle(&old, &v); old = v;
    for (int i = 0; i < 4; ++i)
        assert(InkIn(&kBatSafari[i]) == (i == 0 ? 8 * 14 : 0));
    CtrBattleUi_Draw(canvas, base);         /* the canvas back to the caller's view, for its next settle */
}

/* quick_ball_states: no button without the quick ball; READY in the ball's colours; EMPTY toned grey. */
static void quick_ball_states(const CtrBattleView *base)
{
    CtrBattleView v = *base, old = *base;
    uint16_t green = GbaColor(31 << 5);     /* the test's icon colour for item 2 */

    v.state = BUI_ACTION; v.pressed = -1; v.cursor = 0; v.ballItem = 2; v.ballQty = 5; v.ballCount = 2; v.ballPlace = 0;
    v.ball = CTR_QUICK_BALL_NONE; settle(&old, &v); old = v;
    {   /* no quick ball: its icon (the only pure green here) nowhere on the screen */
        const CtrPokeTouchRect all = {0, 0, W, H};

        assert(ColorIn(&all, green) == 0);
    }
    v.ball = CTR_QUICK_BALL_READY; settle(&old, &v); old = v;
    assert(ColorIn(&kBatBall, green) > 0);
    v.ball = CTR_QUICK_BALL_EMPTY; settle(&old, &v); old = v;
    assert(ColorIn(&kBatBall, green) == 0 && ColorIn(&kBatBall, Tone(green, 0)) > 0);
    v.ball = CTR_QUICK_BALL_FULL; settle(&old, &v); old = v;
    assert(ColorIn(&kBatBall, green) == 0);
    CtrBattleUi_Draw(canvas, base);         /* the canvas back to the caller's view, for its next settle */
}

/* The arrow at the start of a move or target cell's first line (8 px wide, the line's 14 rows): 8x14 of ink when there. */
static int ArrowAtLine(const CtrPokeTouchRect *r)
{
    int n = 0;

    for (int x = r->x + 8; x < r->x + 16; ++x)
        for (int y = r->y + 8; y < r->y + 24; ++y)
            n += at(x, y) == KitInk(2);
    return n;
}

/* move_hit_centres, empty_move_slot and the arrow on the moves and the targets. */
static void moves_and_targets(const CtrBattleView *base)
{
    CtrBattleView v = *base, old = *base;
    const CtrPokeTouchRect *c = &kBatCancel;

    v.state = BUI_MOVE; v.pressed = -1; v.cursor = 1;
    v.move[3].present = 0;
    settle(&old, &v); old = v;
    for (int i = 0; i < 3; ++i)
        assert(CtrBattleUi_Hit(&v, kBatMove[i].x + kBatMove[i].w / 2, kBatMove[i].y + kBatMove[i].h / 2) == i);
    assert(CtrBattleUi_Hit(&v, c->x + c->w / 2, c->y + c->h / 2) == BUI_CANCEL);
    /* empty_move_slot: nothing but the page, and no hit */
    assert(CtrBattleUi_Hit(&v, kBatMove[3].x + kBatMove[3].w / 2, kBatMove[3].y + kBatMove[3].h / 2) == -1);
    assert(ColorIn(&kBatMove[3], BPageBg()) == kBatMove[3].w * kBatMove[3].h);
    for (int i = 0; i < 3; ++i)
        assert(ArrowAtLine(&kBatMove[i]) == (i == 1 ? 8 * 14 : 0));
    v.pressed = 2; settle(&old, &v); old = v;
    for (int i = 0; i < 3; ++i)
        assert(ArrowAtLine(&kBatMove[i]) == (i == 2 ? 8 * 14 : 0));
    v.pressed = -1; v.cursor = 0; settle(&old, &v); old = v;
    v.pressed = BUI_CANCEL; settle(&old, &v); old = v;
    v.pressed = -1; settle(&old, &v); old = v;
    v.move[3].present = 1; settle(&old, &v); old = v;

    v.state = BUI_TARGET; v.cursor = 2;
    for (int i = 0; i < 4; ++i)
        v.target[i].present = 1;
    v.target[3].present = 0;
    settle(&old, &v); old = v;
    for (int i = 0; i < 3; ++i)
        assert(CtrBattleUi_Hit(&v, kBatTarget[i].x + kBatTarget[i].w / 2, kBatTarget[i].y + kBatTarget[i].h / 2) == i);
    assert(CtrBattleUi_Hit(&v, kBatTarget[3].x + kBatTarget[3].w / 2, kBatTarget[3].y + kBatTarget[3].h / 2) == -1);
    assert(CtrBattleUi_Hit(&v, c->x + c->w / 2, c->y + c->h / 2) == BUI_CANCEL);
    for (int i = 0; i < 3; ++i)
        assert(ArrowAtLine(&kBatTarget[i]) == (i == 2 ? 8 * 14 : 0));
    v.pressed = 0; settle(&old, &v); old = v;
    for (int i = 0; i < 3; ++i)
        assert(ArrowAtLine(&kBatTarget[i]) == (i == 0 ? 8 * 14 : 0));
    CtrBattleUi_Draw(canvas, base);         /* the canvas back to the caller's view, for its next settle */
}

/* no_pp_toned (its own process, BATTLE_UI_PP: the digits get synthetic glyphs here and nowhere else): a move with no
 * PP left draws its PP in the toned grey, never in the plain text colour. */
static void no_pp_toned(void)
{
    CtrBattleView v;

    for (unsigned code = 0xA1; code <= 0xAA; ++code)     /* 0-9: ink on the capitals' rows */
        for (int y = 3; y <= 11; ++y)
            for (int x = 0; x <= 4; ++x) {
                uint8_t *src = font + code * 64 + ((y / 8) * 2 + x / 8) * 16 + (y % 8) * 2;
                unsigned bits = ((unsigned)src[1] << 8 | src[0]) | 1u << ((7 - x % 8) * 2);

                src[0] = (uint8_t)bits;
                src[1] = (uint8_t)(bits >> 8);
            }
    memset(&v, 0, sizeof(v));
    v.state = BUI_MOVE; v.pressed = -1; v.cursor = 1;
    v.move[0].present = 1; v.move[0].pp = 0; v.move[0].maxPp = 25;
    names(v.move[0].name, BUI_NAME_LEN, "SURF"); names(v.move[0].typeName, 10, "WATER");
    CtrBattleUi_Draw(canvas, &v);
    {
        /* the PP sits at the end of the cell's first line */
        const CtrPokeTouchRect *r = &kBatMove[0];
        CtrPokeTouchRect pp = {(int16_t)(r->x + r->w - 8 - 32), (int16_t)(r->y + 8), 32, 16};

        assert(ColorIn(&pp, Tone(KitInk(2), 0)) > 0 && ColorIn(&pp, KitInk(2)) == 0);
    }
    printf("PASS battle ui no-PP\n");
}
#endif

int main(int argc, char **argv)
{
    const char *out = getenv("BATTLE_UI_OUT"), *fontPath = getenv("BATTLE_UI_FONT");
    CtrBattleView v, old;
    int k;

    (void)argv;
    ArrowGlyph();
#ifndef BATTLE_UI_BEFORE
    if (getenv("BATTLE_UI_PP")) {
        no_pp_toned();
        return 0;
    }
#endif
#ifndef BATTLE_UI_EXTERNAL_LOAD
#ifndef BATTLE_UI_BEFORE
    gNoPack = getenv("BATTLE_UI_NO_PACK") != NULL;
    {
        uint8_t px[24 * 24];
        uint16_t pal[16];

        /* The full-size pack icon: GBA tile order, 3x3 tiles, no sampling; absent pack: nothing, and the fallback draws. */
        if (gNoPack)
            assert(!PokeTouch_PackIcon24("berry_pouch", px, pal));
        else {
            assert(PokeTouch_PackIcon24("berry_pouch", px, pal));
            assert(px[0] == 1 && px[1] == 2);                  /* tile 0, first byte */
            assert(px[8] == 1 && px[16] == 1 && px[8 * 24] == 1);    /* tiles 1, 2 and 3 (a row of 3) */
            assert(px[4 * 24] == 0 && px[4 * 24 + 7] == 0);    /* rows 4-7 of a tile are empty in the fixture */
            assert(pal[0] == 0 && pal[1] != 0);
        }
    }
#else
    gNoPack = getenv("BATTLE_UI_NO_PACK") != NULL;
#endif
#endif
    if (fontPath) {
        FILE *f = fopen(fontPath, "rb");
        const char *smallPath = getenv("BATTLE_UI_SMALL_FONT");

        assert(f && fread(font, 1, sizeof(font), f) == sizeof(font));
        fclose(f);
        if (smallPath && (f = fopen(smallPath, "rb")) != NULL) {     /* the game's small font: blank unless named */
            assert(fread(smallFont, 1, sizeof(smallFont), f) == sizeof(smallFont));
            fclose(f);
            if ((smallPath = getenv("BATTLE_UI_SMALL_WIDTHS")) != NULL && (f = fopen(smallPath, "rb")) != NULL) {
                assert(fread(smallWidth, 1, sizeof(smallWidth), f) == sizeof(smallWidth));
                fclose(f);
            }
        }
    }
#ifndef BATTLE_UI_BEFORE
    legibility();
#endif
    memset(&v, 0, sizeof(v));
    v.pressed = -1;
    v.party[0] = v.party[1] = v.party[2] = v.party[4] = 1; v.party[3] = 2;
    v.partyHp[0] = 100; v.partyHp[1] = 62; v.partyHp[2] = 35; v.partyHp[4] = 9; v.party[1] = 3; v.partyStatus[1] = 2;
    names(v.activeName, sizeof(v.activeName), "Charizard"); v.activeLevel = 52; v.activeHp = 158; v.activeMaxHp = 158;
    CtrBattleUi_Draw(canvas, &v);   /* BUI_IDLE: the backdrop and the strip */
    put(out, "00_idle");
    old = v;

    /* ACTION */
    v.state = BUI_ACTION; v.showCursor = 1; v.cursor = BUI_ACT_FIGHT;
    settle(&old, &v); old = v; put(out, "01_action"); hits(&v);
    for (k = 0; k < 4; ++k) {
        v.cursor = (int8_t)k; settle(&old, &v); old = v;
        v.pressed = (int16_t)k; settle(&old, &v); old = v;
        if (k == 3) put(out, "02_action_pressed_run");
        v.pressed = -1; settle(&old, &v); old = v;
    }
    v.showCursor = 0; settle(&old, &v); old = v;
#ifndef BATTLE_UI_BEFORE
    /* The quick ball (a wild battle): four buttons in the row, the ball's button between BAG and POKeMON with its
     * arrows in its lower corners; the cursor on it and off it, its presses, another ball, none left, the boxes full,
     * one kind only (no arrows), and back to three buttons (a trainer). */
    v.ball = CTR_QUICK_BALL_READY; v.ballCount = 4; v.ballPlace = 1; v.ballItem = 2; v.ballQty = 21;
    names(v.ballName, BUI_ITEM_LEN, "GREAT BALL");
    settle(&old, &v); old = v; put(out, "02b_action_ball");
    action_hit_centres(&v);
    v.showCursor = 1;
    for (k = 0; k < 5; ++k) {
        v.cursor = (int8_t)k; settle(&old, &v); old = v;
    }
    v.cursor = CTR_ACTION_BALL;
    for (k = BUI_ACT_BALL; k <= BUI_ACT_BALL_NEXT; ++k) {
        v.pressed = (int16_t)k; settle(&old, &v); old = v;
        if (k == BUI_ACT_BALL) put(out, "02c_action_ball_pressed");
        v.pressed = -1; settle(&old, &v); old = v;
    }
    v.pressed = 2; settle(&old, &v); old = v; v.pressed = -1; settle(&old, &v); old = v;
    v.ballPlace = 2; v.ballItem = 3; v.ballQty = 8; names(v.ballName, BUI_ITEM_LEN, "ULTRA BALL");
    settle(&old, &v); old = v;
    v.ball = CTR_QUICK_BALL_EMPTY; v.ballCount = 0; settle(&old, &v); old = v; put(out, "02d_action_no_balls");
    assert(CtrBattleUi_Hit(&v, kBatBallArrow[0].x + 12, kBatBallArrow[0].y + 12) == -1);    /* no arrows */
    v.ball = CTR_QUICK_BALL_FULL; v.ballCount = 4; settle(&old, &v); old = v;
    v.ball = CTR_QUICK_BALL_READY; v.ballCount = 1; settle(&old, &v); old = v;
    assert(CtrBattleUi_Hit(&v, kBatBallArrow[1].x + 12, kBatBallArrow[1].y + 12) == -1);    /* one kind: no other */
    v.ball = CTR_QUICK_BALL_NONE; v.cursor = 0; settle(&old, &v); old = v;
    action_hit_centres(&v);
    arrow_follows_cursor_and_press(&v);
    quick_ball_states(&v);
    v.showCursor = 0; settle(&old, &v); old = v;
#endif

    /* MOVE */
    v.state = BUI_MOVE; v.showCursor = 1; v.cursor = 1;
    for (k = 0; k < 4; ++k) {
        CtrBattleMove *m = &v.move[k];
        static const char *const nm[4] = {"FLAMETHROWER", "EARTHQUAKE", "SURF", "THUNDERBOLT"};
        static const char *const tn[4] = {"FIRE", "GROUND", "WATER", "ELECTR"};
        static const int ty[4] = {10, 4, 11, 13};

        m->present = k != 3 || 1; m->type = (uint8_t)ty[k];
        names(m->name, sizeof(m->name), nm[k]); names(m->typeName, sizeof(m->typeName), tn[k]);
        names(m->power, sizeof(m->power), k == 2 ? "-" : "90");
        m->maxPp = 15; m->pp = (uint8_t)(k == 0 ? 15 : k == 1 ? 7 : k == 2 ? 3 : 0);
    }
    v.move[3].present = 0;
    settle(&old, &v); old = v; put(out, "03_moves"); hits(&v);
#ifndef BATTLE_UI_BEFORE
    if (getenv("BATTLE_UI_MATCHUPS")) {
        /* The matchup pills (opt-in: the baseline comparison above does not draw them). On the second row of each move
         * card, right of POW, a pill in the colour of its meaning (x2 and x4 green, x1/2 orange, x0 grey): that colour
         * is counted over the row, and the other three must not appear in it. */
        static const uint8_t fx[4] = {CTR_FX_DOUBLE, CTR_FX_HALF, CTR_FX_IMMUNE, CTR_FX_QUADRUPLE};
        const uint16_t fill[4] = {kBFxColor[CTR_FX_DOUBLE], kBFxColor[CTR_FX_HALF], kBFxColor[CTR_FX_IMMUNE], kBFxColor[CTR_FX_QUADRUPLE]};
        CtrBattleView w = v;

        for (k = 0; k < 4; ++k) {
            w.move[k] = v.move[k < 3 ? k : 0];
            w.move[k].present = 1;
            w.move[k].matchup = fx[k];
        }
        w.showCursor = 0;
        settle(&old, &w); old = w; put(out, "03b_move_matchups");
        for (k = 0; k < 4; ++k) {
            const CtrPokeTouchRect *r = &kBatMove[k];
            int found = 0;

            for (int y = r->y + 8 + 16; y < r->y + 8 + 32; ++y)
                for (int x = r->x + 56; x < r->x + r->w - 8; ++x)
                    found += at(x, y) == fill[k];
            assert(found >= 12);           /* the mark is there, in the colour of its meaning */
            for (int c = 0; c < 4; ++c)
                if (c != k) {
                    int other = 0;

                    for (int y = r->y + 8 + 16; y < r->y + 8 + 32; ++y)
                        for (int x = r->x + 56; x < r->x + r->w - 8; ++x)
                            other += at(x, y) == fill[c];
                    assert(other == 0);    /* and in no other */
                }
        }
        /* A neutral hit draws nothing: the row of the first card has none of its colour any more. */
        {
            const CtrPokeTouchRect *r = &kBatMove[0];
            int left = 0;

            w.move[0].matchup = CTR_FX_NONE;
            settle(&old, &w); old = w;
            for (int y = r->y + 8 + 16; y < r->y + 8 + 32; ++y)
                for (int x = r->x + 56; x < r->x + r->w - 8; ++x)
                    left += at(x, y) == fill[0];
            assert(left == 0);
        }
        settle(&old, &v); old = v;       /* and back to the cards without tags */
    }
#endif
#ifndef BATTLE_UI_BEFORE
    if (getenv("BATTLE_UI_TYPES")) {
        /* The type sheet's eighteen cards (opt-in), four to a screen: "98_types_N", for a comparison with the sheet. */
        static const struct { const char *name, *type; int t, pp, max, fx; } kSheet[18] = {
            {"FLAMETHROWER", "FIRE", 10, 10, 15, CTR_FX_DOUBLE}, {"EARTHQUAKE", "GROUND", 4, 8, 10, CTR_FX_HALF},
            {"SURF", "WATER", 11, 3, 15, CTR_FX_IMMUNE}, {"SOLARBEAM", "GRASS", 12, 4, 10, CTR_FX_QUADRUPLE},
            {"THUNDERBOLT", "ELECTR", 13, 15, 15, CTR_FX_DOUBLE}, {"ICE BEAM", "ICE", 15, 10, 10, CTR_FX_DOUBLE},
            {"AERIAL ACE", "FLYING", 2, 20, 20, CTR_FX_DOUBLE}, {"ROCK SLIDE", "ROCK", 5, 10, 10, CTR_FX_DOUBLE},
            {"CROSS CHOP", "FIGHT", 1, 5, 5, CTR_FX_DOUBLE}, {"SLUDGE BOMB", "POISON", 3, 10, 10, CTR_FX_DOUBLE},
            {"MEGAHORN", "BUG", 6, 7, 10, CTR_FX_DOUBLE}, {"IRON TAIL", "STEEL", 8, 12, 15, CTR_FX_HALF},
            {"PSYCHIC", "PSYCHC", 14, 10, 10, CTR_FX_DOUBLE}, {"SHADOW BALL", "GHOST", 7, 12, 15, CTR_FX_DOUBLE},
            {"CRUNCH", "DARK", 17, 15, 15, CTR_FX_DOUBLE}, {"DRAGON CLAW", "DRAGON", 16, 15, 15, CTR_FX_DOUBLE},
            {"BODY SLAM", "NORMAL", 0, 15, 15, CTR_FX_NEUTRAL}, {"CURSE", "???", 9, 10, 10, CTR_FX_NEUTRAL}};
        CtrBattleView w = v;

        w.showCursor = 0; w.pressed = -1;
        for (int screen = 0; screen < 5; ++screen) {
            char name[32];

            for (k = 0; k < 4; ++k) {
                CtrBattleMove *m = &w.move[k];
                int n = screen * 4 + k;

                memset(m, 0, sizeof(*m));
                if (n >= 18)
                    continue;
                m->present = 1; m->type = (uint8_t)kSheet[n].t; m->pp = (uint8_t)kSheet[n].pp; m->maxPp = (uint8_t)kSheet[n].max;
                m->matchup = (uint8_t)kSheet[n].fx;
                names(m->name, sizeof(m->name), kSheet[n].name); names(m->typeName, sizeof(m->typeName), kSheet[n].type);
            }
            settle(&old, &w); old = w;
            snprintf(name, sizeof(name), "98_types_%d", screen);
            put(out, name);
        }
        settle(&old, &v); old = v;
    }
#endif
#ifndef BATTLE_UI_BEFORE
    if (getenv("BATTLE_UI_MOCK")) {
        /* The reference mockup's data (opt-in): drawn as "99_mock" for a pixel comparison with the design. */
        static const char *const nm[4] = {"FLAMETHROWER", "EARTHQUAKE", "SURF", "SOLARBEAM"};
        static const char *const tn[4] = {"FIRE", "GROUND", "WATER", "GRASS"};
        static const int ty[4] = {10, 4, 11, 12}, pp[4] = {10, 8, 3, 4}, mp[4] = {15, 10, 15, 10};
        static const uint8_t fx[4] = {CTR_FX_DOUBLE, CTR_FX_HALF, CTR_FX_IMMUNE, CTR_FX_QUADRUPLE};
        CtrBattleView w = v;

        for (k = 0; k < 4; ++k) {
            CtrBattleMove *m = &w.move[k];

            memset(m, 0, sizeof(*m));
            m->present = 1; m->type = (uint8_t)ty[k]; m->pp = (uint8_t)pp[k]; m->maxPp = (uint8_t)mp[k]; m->matchup = fx[k];
            names(m->name, sizeof(m->name), nm[k]); names(m->typeName, sizeof(m->typeName), tn[k]);
            names(m->power, sizeof(m->power), "90");
        }
        names(w.move[0].accuracy, sizeof(w.move[0].accuracy), "100");
        w.move[0].effectChance = 10; w.move[0].stab = 1;
        names(w.move[0].desc, sizeof(w.move[0].desc), "A powerful fire attack that may inflict a burn.");
        w.cursor = 0; w.showCursor = 1; w.pressed = -1;
        for (k = 0; k < 6; ++k) { w.party[k] = 1; w.partyHp[k] = 100; w.partyStatus[k] = 0; }
        w.party[1] = 3; w.partyHp[1] = 50; w.partyStatus[1] = 2; w.party[3] = 2; w.partyHp[3] = 0;
        settle(&old, &w); old = w; put(out, "99_mock");
        w.showCursor = 0; w.pressed = 1; settle(&old, &w); old = w; put(out, "99_mock_card");
        w.pressed = BUI_CANCEL; settle(&old, &w); old = w; put(out, "99_mock_cancel");
        settle(&old, &v); old = v;
    }
#endif
    v.pressed = 2; settle(&old, &v); old = v; v.pressed = -1; settle(&old, &v); old = v;
    v.pressed = BUI_CANCEL; settle(&old, &v); old = v; put(out, "04_moves_cancel_pressed");
    v.pressed = -1; v.cursor = 0; settle(&old, &v); old = v;

    /* TARGET */
    v.state = BUI_TARGET;
    for (k = 0; k < 4; ++k) {
        CtrBattleTarget *t = &v.target[k];
        static const char *const nm[4] = {"RATICATE", "PIDGEOT", "CHARIZARD", "BLASTOISE"};

        t->present = 1; t->side = (uint8_t)(k >= 2); t->battler = (int8_t)k; t->level = (uint8_t)(30 + 7 * k);
        t->maxHp = 120; t->hp = (uint16_t)(120 - 35 * k);
        names(t->name, sizeof(t->name), nm[k]);
    }
    v.target[1].present = 0;
    settle(&old, &v); old = v; put(out, "05_targets"); hits(&v);
    v.pressed = 0; settle(&old, &v); old = v; v.pressed = -1; settle(&old, &v); old = v;

#ifndef BATTLE_UI_BEFORE
    moves_and_targets(&v);
#endif
    /* SAFARI */
    v.state = BUI_SAFARI; v.cursor = 2;
    settle(&old, &v); old = v; put(out, "06_safari"); hits(&v);
    v.pressed = 1; settle(&old, &v); old = v; v.pressed = -1; settle(&old, &v); old = v;

    /* BAG */
    v.state = BUI_BAG;
    {
        CtrBattleBag *b = &v.bag;
        static const char *const nm[5] = {"POTION", "SUPER POTION", "ANTIDOTE", "REVIVE", "FULL HEAL"};

        b->pocket = 0; b->count = 9; b->cursor = 1; b->first = 0;
        for (k = 0; k < 5; ++k) { b->row[k].item = (uint16_t)(k + 1); b->row[k].qty = (uint16_t)(20 - 3 * k); names(b->row[k].name, BUI_ITEM_LEN, nm[k]); }
        b->selItem = 2; names(b->selName, BUI_ITEM_LEN, "SUPER POTION");
        names(b->selDesc, BUI_DESC_LEN, "Restores the HP of one POKeMON by 50 points.");
    }
    settle(&old, &v); old = v; put(out, "07_bag"); hits(&v);
    v.pressed = BUI_BAG_ROW + 2; settle(&old, &v); old = v; v.pressed = -1; settle(&old, &v); old = v;
    /* The Berry Pouch in the Bag: BERRIES beside the title, which takes the whole width without it. */
    v.bag.berries = 365; settle(&old, &v); old = v;
    assert(CtrBattleUi_Hit(&v, 260, 16) == BUI_BAG_BERRIES);
    v.pressed = BUI_BAG_BERRIES; settle(&old, &v); old = v; put(out, "08_bag_berries_pressed"); v.pressed = -1; settle(&old, &v); old = v;
    v.bag.berries = 0; settle(&old, &v); old = v;
    assert(CtrBattleUi_Hit(&v, 260, 16) == -1);
    v.bag.menuCount = 2; v.bag.menuCursor = 0; names(v.bag.menuName[0], 12, "USE"); names(v.bag.menuName[1], 12, "CANCEL");
    settle(&old, &v); old = v; put(out, "09_bag_menu"); hits(&v);
    v.bag.menuCount = 0; names(v.bag.message, sizeof(v.bag.message), "The POKeMON's HP was restored by 50 points.");
    settle(&old, &v); old = v; put(out, "10_bag_message"); v.bag.message[0] = 0; settle(&old, &v); old = v;

    /* PARTY */
    v.state = BUI_PARTY;
    {
        CtrBattleParty *p = &v.pty;
        static const char *const nm[6] = {"CHARIZARD", "PIDGEOT", "SNORLAX", "ALAKAZAM", "GENGAR", "BLASTOISE"};

        for (k = 0; k < 6; ++k) {
            p->present[k] = 1; p->level[k] = (uint8_t)(50 + k); p->maxHp[k] = 150; p->hp[k] = (uint16_t)(150 - 30 * k);
            p->icon[k] = (uint16_t)(k + 1); names(p->name[k], sizeof(p->name[k]), nm[k]);
        }
        p->status[1] = 2; p->status[4] = 5; p->hp[3] = 0; p->cursor = 2; p->mode = BUI_PTY_SWITCH;
    }
    settle(&old, &v); old = v; put(out, "11_party"); hits(&v);
    v.pressed = BUI_PTY_SLOT + 1; settle(&old, &v); old = v; v.pressed = -1; settle(&old, &v); old = v;
    v.pressed = BUI_PTY_CANCEL; settle(&old, &v); old = v; put(out, "12_party_cancel_pressed"); v.pressed = -1; settle(&old, &v); old = v;
    v.pty.panel = BUI_PTY_ACTIONS; v.pty.menuCount = 3; v.pty.menuCursor = 0;
    names(v.pty.menuName[0], 14, "SHIFT"); names(v.pty.menuName[1], 14, "SUMMARY"); names(v.pty.menuName[2], 14, "CANCEL");
    settle(&old, &v); old = v; put(out, "13_party_menu"); hits(&v);
    v.pty.panel = BUI_PTY_YESNO; v.pty.menuCount = 2; names(v.pty.menuName[0], 14, "YES"); names(v.pty.menuName[1], 14, "NO");
    settle(&old, &v); old = v;
    v.pty.panel = BUI_PTY_TEXT; names(v.pty.message, sizeof(v.pty.message), "CHARIZARD is already in battle!");
    settle(&old, &v); old = v; put(out, "14_party_message"); hits(&v);
    v.pty.panel = BUI_PTY_NONE; v.pty.tag[0] = BUI_PTY_TAG_ABLE; v.pty.tag[1] = BUI_PTY_TAG_NOT_ABLE; v.pty.tag[2] = BUI_PTY_TAG_LEARNED;
    v.pty.tag[3] = BUI_PTY_TAG_NO_USE; v.pty.tag[4] = BUI_PTY_TAG_ABLE; settle(&old, &v); old = v; put(out, "12b_party_teach");
    memset(v.pty.tag, 0, sizeof(v.pty.tag)); settle(&old, &v); old = v;
    v.pty.mode = BUI_PTY_FORCED; settle(&old, &v); old = v; put(out, "15_party_forced"); hits(&v);
    v.state = BUI_ACTION; settle(&old, &v); old = v;
    v.state = BUI_IDLE; settle(&old, &v);

    /* SUMMARY */
    {
        CtrSummaryUiView sv, so;
        int tab;

        memset(&sv, 0, sizeof(sv));
        sv.pressed = -1;
        sv.icon = 1;
        sv.data.ready = 1; sv.data.mon = 0; sv.data.cursor = 0; sv.data.level[0] = 0;
        names(sv.data.nickname, 12, "CHARIZARD"); names(sv.data.species, 12, "CHARIZARD"); names(sv.data.level, 8, "Lv50");
        names(sv.data.gender, 2, "M"); names(sv.data.dex, 8, "No.006"); names(sv.data.ot, 16, "RED"); names(sv.data.id, 8, "12345");
        names(sv.data.item, 20, "LEFTOVERS"); names(sv.data.types[0], 12, "FIRE"); names(sv.data.types[1], 12, "FLYING");
        sv.data.typeIds[0] = 10; sv.data.typeIds[1] = 2;
        names(sv.data.hp, 12, "120/150"); sv.data.hpCur = 120; sv.data.hpMax = 150;
        for (k = 0; k < 5; ++k) names(sv.data.stats[k], 8, k == 0 ? "104" : "89");
        names(sv.data.exp, 12, "125000"); names(sv.data.nextExp, 12, "4200");
        names(sv.data.ability, 16, "BLAZE"); names(sv.data.abilityDesc, 64, "Powers up FIRE moves in a pinch.");
        names(sv.data.memo, 256, "Hatched at Pallet Town. Met at Lv5.");
        names(sv.data.nature, 12, "ADAMANT"); sv.data.ribbons = 2;
        for (k = 0; k < 4; ++k) {
            static const char *const mv[4] = {"FLAMETHROWER", "WING ATTACK", "DRAGON CLAW", "SLASH"};
            static const char *const mt[4] = {"FIRE", "FLYING", "DRAGON", "NORMAL"};
            static const unsigned char mi[4] = {10, 2, 16, 0};

            sv.data.moves[k] = (unsigned short)(k + 1);
            names(sv.data.moveNames[k], 16, mv[k]); names(sv.data.pp[k], 12, "15/15"); names(sv.data.moveTypes[k], 12, mt[k]);
            names(sv.data.power[k], 8, "90"); names(sv.data.accuracy[k], 8, "100"); sv.data.moveTypeIds[k] = mi[k];
        }
        names(sv.data.moveDesc, 256, "A powerful fire attack that may burn the target.");
        so = sv;
        CtrSummaryUi_Draw(canvas, &so);   /* the screen opens drawn in full; updates follow */
        for (tab = 0; tab < 5; ++tab) {
            static const char *const nm[5] = {"16_summary_info", "17_summary_stats", "18_summary_moves", "19_summary_move_details",
                                              "20_summary_trainer"};

            sv.data.page = tab == 1 ? SUI_PAGE_SKILLS : tab == 2 ? SUI_PAGE_MOVES : tab == 3 ? SUI_PAGE_MOVES_INFO : SUI_PAGE_INFO;
            sv.trainer = tab == 4;
            sv.data.cursor = tab == 3 ? 1 : 0;
            settle_summary(&so, &sv); so = sv; put(out, nm[tab]); summary_hits(&sv);
            sv.pressed = SUI_HIT_NEXT; settle_summary(&so, &sv); so = sv;
            sv.pressed = SUI_HIT_BACK; settle_summary(&so, &sv); so = sv;
            sv.pressed = SUI_HIT_TAB + 1; settle_summary(&so, &sv); so = sv;
            sv.pressed = -1; settle_summary(&so, &sv); so = sv;
        }
        /* STATS' other view: the IVs as bars, the EVs, Hidden Power; the switch in its corner is hit on STATS only. */
        {
            static const unsigned char iv[6] = {25, 30, 14, 31, 19, 28}, ev[6] = {6, 0, 0, 252, 0, 252};

            sv.data.page = SUI_PAGE_SKILLS; sv.trainer = 0; sv.pressed = -1; sv.ivView = 1;
            memcpy(sv.data.iv, iv, sizeof(iv)); memcpy(sv.data.ev, ev, sizeof(ev));
            sv.data.hpType = 10; sv.data.hpPower = 70; names(sv.data.hpTypeName, sizeof(sv.data.hpTypeName), "FIRE");
            settle_summary(&so, &sv); so = sv; put(out, "21_summary_iv_ev"); summary_hits(&sv);
            assert(CtrSummaryUi_Hit(&sv, 285, 85) == SUI_HIT_IVTOGGLE);
            sv.pressed = SUI_HIT_IVTOGGLE; settle_summary(&so, &sv); so = sv;
            sv.pressed = -1; sv.ivView = 0; settle_summary(&so, &sv); so = sv;
            assert(CtrSummaryUi_Hit(&sv, 285, 85) == SUI_HIT_IVTOGGLE);
            sv.data.page = SUI_PAGE_MOVES; settle_summary(&so, &sv); so = sv;
            assert(CtrSummaryUi_Hit(&sv, 285, 85) != SUI_HIT_IVTOGGLE);
        }
        sv.loading = 1; settle_summary(&so, &sv);
    }

    /* The red of PokeTouch: nothing of the shared navy theme may remain on any state. */
#ifndef BATTLE_UI_BEFORE
    {
        static const uint16_t navy[] = {UI_BG, UI_BG_TOP, UI_OUTLINE, UI_PANEL_LO, UI_PANEL, UI_PANEL_HI, UI_BORDER,
                                        UI_BORDER_LIT, UI_SEPARATOR, UI_TEXT_SH, UI_TEXT_DIM, UI_HP_TRACK,
                                        UI_RGB(34, 56, 100)};
        CtrBattleView s = v;
        int states[] = {BUI_IDLE, BUI_ACTION, BUI_MOVE, BUI_TARGET, BUI_SAFARI, BUI_BAG, BUI_PARTY};

        for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
            s.state = (uint8_t)states[i];
            CtrBattleUi_Draw(canvas, &s);
            if (states[i] == BUI_MOVE || states[i] == BUI_IDLE || states[i] == BUI_ACTION)
                continue;                  /* these screens are their artwork, whatever colours it holds */
            for (int n = 0; n < W * H; ++n)
                for (size_t j = 0; j < sizeof(navy) / sizeof(navy[0]); ++j)
                    if (canvas[n] == navy[j]) {
                        fprintf(stderr, "state %d keeps the navy theme colour %04x at %d\n", states[i], navy[j], n);
                        assert(0);
                    }
            if (states[i] == BUI_SAFARI || states[i] == BUI_TARGET)
                assert(at(1, 100) == BPageBg());    /* reworked: the page's own colour */
            else
                assert(at(1, 100) == EMBER_BG(1, 100) || at(1, 100) == PT_RED_BG);   /* the ember backdrop, or the flat red not yet redone */
        }
        {
            CtrSummaryUiView sv;

            memset(&sv, 0, sizeof(sv));
            sv.pressed = -1; sv.data.ready = 1; sv.data.typeIds[0] = 10; sv.data.typeIds[1] = 2; sv.data.hpCur = 90; sv.data.hpMax = 150;
            for (int page = 0; page < 4; ++page) {
                sv.data.page = page;
                CtrSummaryUi_Draw(canvas, &sv);
                for (int n = 0; n < W * H; ++n)
                    for (size_t j = 0; j < sizeof(navy) / sizeof(navy[0]); ++j)
                        if (canvas[n] == navy[j]) {
                            fprintf(stderr, "summary page %d keeps the navy theme colour %04x at %d\n", page, navy[j], n);
                            assert(0);
                        }
                assert(at(1, 100) == EMBER_BG(1, 100));
            }
        }
    }
#endif
    if (argc > 1)
        printf("hits %08x\nmask %08x\n", sHitCrc, sMaskCrc);
    puts("PASS battle ui");
    return 0;
}
