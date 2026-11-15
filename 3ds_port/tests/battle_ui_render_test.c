/* The battle Bag (the ember look left in 3ds_battle_ui_impl.h) through the production renderer with synthetic game
 * bridges; the rest of the battle screen, the Party and the Summary are the red device's (device_render_test.c's
 * battle, party and summary suites).
 *
 * The Bag with its menu and message is drawn; each press, cursor move and state change
 * must leave the canvas exactly as a full redraw would (the dirty rectangles are complete); the screen is the PokeTouch
 * red (no navy of the shared theme remains in it) and the buttons' hit areas answer to the same points as before. No ROM-derived data: the font is blank unless BATTLE_UI_FONT names
 * a 256x64-byte glyph file (a local viewing aid), and BATTLE_UI_OUT names a directory for one PPM per state.
 *
 * usage: battle_ui_render_test [print-hits] */
#include <assert.h>
#ifndef PT_SOURCE
#define PT_SOURCE "3ds_poketouch.c"
#endif
#include PT_SOURCE
#include "3ds_text_case_views.h"   /* the game names in proper case, as 3ds_bottom.c gives them */
#include <math.h>

static uint8_t font[256 * 64];
const uint8_t *CtrTwinFontData(void) { return font; }
static uint8_t fontWidths[256];   /* the game font's widths with BATTLE_UI_WIDTHS (a viewing aid); else a fixed 6 */
unsigned CtrTwinFontWidth(unsigned code) { return code < 256 && fontWidths[code] ? fontWidths[code] : 6; }
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
    /* A flat 32x32 icon in the slot's own colour. */
    memset(tiles, 0x11, 512); memset(pal, 0, 32);
    pal[1] = (unsigned short)((slot * 5 + 6) | (31 - slot * 4) << 5 | (slot * 3) << 10);
    return 1;
}

/* The Summary's art (its drawer is in the renderer too, checked by device_render_test.c): none. */
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

/* The state last drawn: the Bag was redrawn for the quick ball (issue #16), so its geometry is checked by the
 * assertions on it below, not by its silhouette. */
static int sState;

static void settle(const CtrBattleView *oldRaw, const CtrBattleView *vRaw)
{
    CtrDirtyList dirty;
    static CtrBattleView oc, vc;
    const CtrBattleView *old = &oc, *v = &vc;

    oc = *oldRaw; vc = *vRaw; CtrTextCase_Battle(&oc); CtrTextCase_Battle(&vc);

    sState = v->state;

    CtrDirty_Clear(&dirty);
    CtrBattleUi_Update(canvas, old, v, &dirty);
    memcpy(reference, canvas, sizeof(canvas));
    CtrBattleUi_Draw(canvas, v);
    if (memcmp(reference, canvas, sizeof(canvas))) {
        for (int x = 0; x < W; ++x)
            for (int y = 0; y < H; ++y)
                if (reference[x * H + H - 1 - y] != canvas[x * H + H - 1 - y]) {
                    fprintf(stderr, "battle redraw mismatch at %d,%d state=%u pressed=%d cursor=%d\n", x, y, v->state,
                            v->pressed, v->cursor);
                    assert(0);
                }
    }
}

/* The silhouette of a drawn state: which pixels are not the backdrop's. Restyling must not move an edge. */
static unsigned sMaskCrc;
static void mask(void)
{
    uint16_t bg = at(1, 100);

    if (sState == BUI_BAG)
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
    if (v->state == BUI_BAG)
        return;
    for (int y = 0; y < H; y += 3)
        for (int x = 0; x < W; x += 3) {
            int h = CtrBattleUi_Hit(v, x, y);

            sHitCrc = sHitCrc * 31u + (unsigned)(h + 7);
        }
}

static void names(char *dst, size_t n, const char *s) { snprintf(dst, n, "%s", s); }
/* A name of the game's text: proper case, as the Bag's reader gives it (3ds_bottom.c CtrBottomBag_ItemName). */
static void gnames(char *dst, size_t n, const char *s) { names(dst, n, s); CtrTextCase(dst); }

int main(int argc, char **argv)
{
    const char *out = getenv("BATTLE_UI_OUT"), *fontPath = getenv("BATTLE_UI_FONT");
    CtrBattleView v, old;
    int k;

    (void)argv;
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
        if (getenv("BATTLE_UI_WIDTHS") && (f = fopen(getenv("BATTLE_UI_WIDTHS"), "rb")) != NULL) {
            assert(fread(fontWidths, 1, sizeof(fontWidths), f) == sizeof(fontWidths));
            fclose(f);
            memset(sPtFontReady, 0, sizeof(sPtFontReady));
        }
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

    /* BAG (the ember look's; the commands, the moves, the targets, Safari, idle and the Party are the red device's:
     * device_render_test.c's battle and party suites) */
    v.state = BUI_BAG;
    {
        CtrBattleBag *b = &v.bag;
        static const char *const nm[5] = {"POTION", "SUPER POTION", "ANTIDOTE", "REVIVE", "FULL HEAL"};

        b->pocket = 0; b->count = 9; b->cursor = 1; b->first = 0;
        for (k = 0; k < 5; ++k) { b->row[k].item = (uint16_t)(k + 1); b->row[k].qty = (uint16_t)(20 - 3 * k); gnames(b->row[k].name, BUI_ITEM_LEN, nm[k]); }
        b->selItem = 2; gnames(b->selName, BUI_ITEM_LEN, "SUPER POTION");
        names(b->selDesc, BUI_DESC_LEN, "Restores the HP of one POKeMON by 50 points.");
    }
    { static CtrBattleView c_; c_ = v; CtrTextCase_Battle(&c_); CtrBattleUi_Draw(canvas, &c_); }   /* the Bag opens drawn in full; updates follow */
    old = v;
    sState = v.state;
    put(out, "07_bag"); hits(&v);
    /* The scroll rail is a slider: the arrows are their own buttons, the rail between them takes a press anywhere on it, and its target
     * runs from the first item at the top to the last at the bottom, never going back. */
    assert(CtrBattleUi_Hit(&v, 180, 165) == BUI_BAG_DOWN);
    assert(CtrBattleUi_Hit(&v, 180, 40) == -1);                       /* the up arrow with nothing above */
    assert(CtrBattleUi_Hit(&v, 180, 80) == BUI_BAG_RAIL && CtrBattleUi_Hit(&v, 180, 140) == BUI_BAG_RAIL);
    {
        int last = -1;

        for (int y = 50; y <= 160; ++y) {
            int t = CtrBattleUi_BagRailTarget(&v.bag, y);

            assert(t >= last && t >= 0 && t <= v.bag.count - 1);
            last = t;
        }
        assert(CtrBattleUi_BagRailTarget(&v.bag, 40) == 0 && CtrBattleUi_BagRailTarget(&v.bag, 170) == v.bag.count - 1);
    }
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

    /* The red of PokeTouch: nothing of the shared navy theme may remain on any state. */
#ifndef BATTLE_UI_BEFORE
    {
        static const uint16_t navy[] = {UI_BG, UI_BG_TOP, UI_OUTLINE, UI_PANEL, UI_PANEL_HI, UI_BORDER,
                                        UI_BORDER_LIT, UI_SEPARATOR, UI_TEXT_SH, UI_TEXT_DIM,
                                        UI_RGB(34, 56, 100)};      /* (UI_PANEL_LO and UI_HP_TRACK are left out: the Llama slate pieces, #14182c and #20263f, squeeze to them) */
        CtrBattleView s = v;
        int states[] = {BUI_BAG};                  /* the ember state left (the others are the red device's) */

        for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
            s.state = (uint8_t)states[i];
            { static CtrBattleView c_; c_ = s; CtrTextCase_Battle(&c_); CtrBattleUi_Draw(canvas, &c_); }
            for (int n = 0; n < W * H; ++n)
                for (size_t j = 0; j < sizeof(navy) / sizeof(navy[0]); ++j)
                    /* The Bag's scroll rail is a slate gradient whose steps, squeezed to RGB565, land on a navy colour or
                     * two: its box is not looked at. */
                    if (canvas[n] == navy[j]
                        && !(states[i] == BUI_BAG && n / H >= 181 && n / H < 191 && H - 1 - n % H >= 58 && H - 1 - n % H < 152)) {
                        fprintf(stderr, "state %d keeps the navy theme colour %04x at %d\n", states[i], navy[j], n);
                        assert(0);
                    }
            assert(at(1, 100) == EMBER_BG(1, 100) || at(1, 100) == PT_RED_BG);   /* the ember backdrop, or the flat red not yet redone */
        }
    }
#endif
    if (argc > 1)
        printf("hits %08x\nmask %08x\n", sHitCrc, sMaskCrc);
    puts("PASS battle ui");
    return 0;
}
