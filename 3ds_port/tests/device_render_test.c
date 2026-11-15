/* The red device (style B): its drawing kit (3ds_device_kit_impl.h) and the B views through the production renderer,
 * with synthetic game bridges.
 *
 * Each suite draws its view's states and checks them: an Update equals a full Draw pixel for pixel, every key is hit
 * exactly where it is drawn (3ds_device_layout.h is the one source of both), and no port-authored key label is cut
 * (sDevKeyCut stays 0). No ROM-derived data: the fonts are synthetic stubs, every glyph DEVICE_FONT_W px wide (default
 * 5; the small font's 5), unless FIRERED_TEST_PAK names the owner's own pack, whose glyphs are then read (a local
 * viewing aid, tools/render_device_views.py; never committed). Environment:
 *   DEVICE_OUT=dir            one PPM per drawn state, dir/<suite>_<state>.ppm
 *   FIRERED_TEST_PAK=pak      the pack (ABI 0x8f71cf3a, its ROM's SHA-1), opened as poketouch_render_test.c does
 *   DEVICE_WIDTHS=file        the normal font's 256 glyph widths; DEVICE_SMALL_WIDTHS=file the small font's
 *   DEVICE_FONT_W=n           the stub glyph width
 *
 * usage: device_render_test <suite>   (kit, or all; prints PASS device <suite>) */
#include <assert.h>
#ifndef PT_SOURCE
#define PT_SOURCE "3ds_poketouch.c"
#endif
#include PT_SOURCE
#include "3ds_text_case_views.h"   /* the game names in proper case, as 3ds_bottom.c gives them */
#include "3ds_pak.h"

/* ---- Fonts: the stubs, or the pack's glyphs ------------------------------------------------------------------------ */

static uint8_t font[256 * 64], fontWidths[256];
static uint8_t smallFont[256 * 32], smallWidths[256];
static unsigned gStubW = 5;
const uint8_t *CtrTwinFontData(void) { return font; }
unsigned CtrTwinFontWidth(unsigned code) { return code < 256 && fontWidths[code] ? fontWidths[code] : gStubW; }
const uint8_t *CtrTwinJapaneseGlyph(unsigned code, uint8_t out[64]) { (void)code; memset(out, 0, 64); return out; }
unsigned CtrTwinJapaneseFontWidth(unsigned code) { (void)code; return 8; }
const uint8_t *CtrTwinSmallFontData(void) { return smallFont; }
unsigned CtrTwinSmallFontWidth(unsigned code) { return code < 256 && smallWidths[code] ? smallWidths[code] : 5; }

/* One pixel of a glyph in the game's layout (TwinGlyphDecode / TwinSmallGlyphDecode read it back): value 1 ink, 2 shadow. */
static void GlyphPixel(uint8_t *glyph, int small, int x, int y, unsigned value)
{
    uint8_t *src = glyph + ((small ? y / 8 : (y / 8) * 2 + x / 8) * 16) + (y % 8) * 2;
    unsigned bits = (unsigned)src[1] << 8 | src[0];

    bits |= value << ((7 - x % 8) * 2);
    src[0] = (uint8_t)bits;
    src[1] = (uint8_t)(bits >> 8);
}

/* Every glyph but the space (code 0) a block of ink over the capitals' rows (normal 3-10, small 4-10), a column short of
 * its width, with a shadow row under it. */
static void SyntheticFont(uint8_t *data, int small)
{
    int w = small ? 5 : (int)gStubW, top = small ? 4 : 3;

    memset(data, 0, small ? sizeof(smallFont) : sizeof(font));
    for (unsigned code = 1; code < 256; ++code) {
        uint8_t *glyph = data + code * (small ? 32u : 64u);

        for (int x = 0; x + 1 < w && x < (small ? 8 : 16); ++x) {
            for (int y = top; y <= 10; ++y)
                GlyphPixel(glyph, small, x, y, 1);
            GlyphPixel(glyph, small, x + 1, 11, 2);
        }
    }
}

/* ---- The pack (FIRERED_TEST_PAK), as poketouch_render_test.c opens it --------------------------------------------- */

static FILE *realPack;
static CtrPakHeader realHeader;
static CtrPakEntry *realIndex;

/* An entry of the pack as it is stored, or NULL when it is missing or compressed (this harness does not inflate). */
static uint8_t *PackRead(const char *path, uint32_t *len)
{
    const CtrPakEntry *entry = CtrPak_Find(realIndex, realHeader.entryCount, path);
    uint8_t *out;

    if (!entry || (entry->flags & CTR_PAK_FLAG_COMPRESSED) || entry->storedSize != entry->rawSize)
        return NULL;
    out = malloc(entry->rawSize ? entry->rawSize : 1);
    assert(out);
    assert(!fseek(realPack, (long)entry->offset, SEEK_SET));
    assert(fread(out, 1, entry->rawSize, realPack) == entry->rawSize);
    assert(CtrPak_Crc32(0, out, entry->rawSize) == entry->crc32);
    *len = entry->rawSize;
    return out;
}

static void PackOpen(const char *path)
{
    static const uint8_t romHash[20] = {0x41, 0xcb, 0x23, 0xd8, 0xdc, 0xcc, 0x8e, 0xbd, 0x7c, 0x64,
                                        0x9c, 0xd8, 0xfb, 0xb5, 0x8e, 0xea, 0xce, 0x6e, 0x2f, 0xdc};
    uint8_t header[64], *index;
    size_t size;

    realPack = fopen(path, "rb");
    assert(realPack);
    assert(fread(header, 1, 64, realPack) == 64);
    assert(CtrPak_ParseHeader(header, 0x8f71cf3a, romHash, &realHeader) == CTR_PAK_OK);
    size = (size_t)realHeader.entryCount * CTR_PAK_ENTRY_BYTES;
    index = malloc(size);
    realIndex = calloc(realHeader.entryCount, sizeof(*realIndex));
    assert(index && realIndex && fread(index, 1, size, realPack) == size);
    assert(CtrPak_ParseIndex(index, &realHeader, realIndex) == CTR_PAK_OK);
    free(index);
}

/* The pack's glyphs (the first 256 of the file) into `data`; 0 when the pack does not hold them as they are. */
static int PackFont(const char *path, uint8_t *data, size_t size)
{
    uint32_t len = 0;
    uint8_t *raw = PackRead(path, &len);

    if (!raw || len < size) {
        free(raw);
        return 0;
    }
    memcpy(data, raw, size);
    free(raw);
    return 1;
}

static void ReadWidths(const char *path, uint8_t widths[256])
{
    FILE *f = path ? fopen(path, "rb") : NULL;

    if (!f)
        return;
    assert(fread(widths, 1, 256, f) == 256);
    fclose(f);
}

/* ---- Game bridges: battle_ui_render_test.c's set and poketouch_render_test.c's map art --------------------------- */

/* The pack's files with FIRERED_TEST_PAK; else the two item icons, synthesised: a literal-only GBA LZ stream of 24x24
 * 4bpp tiles (3x3) and of a palette, so the full-size icon path runs without game data. */
void *CtrData_Load(const char *path, uint32_t *len)
{
    uint8_t raw[288];
    int palette = strstr(path, "icon_palettes/") != NULL;
    unsigned size = palette ? 32u : 288u;
    uint8_t *out;
    unsigned pos = 4;

    if (realPack)
        return PackRead(path, len);
    if (!strstr(path, "graphics/items/icon"))
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
/* The map's art: a flat colour, or (richArt) a small synthetic Kanto: sea, a land of greens in three heights with a road
 * across it, a pale path and some towns (orbs), drawn in the colours the HOME map tells apart (each tile is one colour:
 * tile n has palette entry n). */
static int richArt __attribute__((unused));
int CtrPokeTouch_MapArt(int group, uint16_t *map, uint8_t *tiles, uint16_t *pal, uint16_t *bgMap, uint8_t *bgTiles)
{
    (void)group;
    memset(map, 0, 30 * 20 * 2); memset(bgMap, 0, 30 * 20 * 2);
    memset(tiles, 0x11, MAP_TILES * 32); memset(bgTiles, 0, MAP_TILES * 32);
    memset(pal, 0, MAP_COLORS * 2); pal[1] = 0x5ab3;
    if (richArt) {
        /* 1 green, 2 road, 3 orb, 4 sea, 5 pale path, 6 the lowest green, 7 a high green */
        static const unsigned short col[] = {0, 1703, 7933, 2303, 27210, 0, 0, 0};
        for (int n = 1; n < 8; ++n) { memset(tiles + n * 32, n * 0x11, 32); pal[n] = col[n]; }
        pal[0] = 32468;
        pal[5] = (unsigned short)(25 | 25 << 5 | 18 << 10);
        pal[6] = (unsigned short)(3 | 16 << 5 | 1 << 10);
        pal[7] = (unsigned short)(21 | 30 << 5 | 7 << 10);
        for (int ty = 2; ty < 18; ++ty)
            for (int tx = 3; tx < 26; ++tx) {
                int t = 1;
                if (tx < 8) t = 6; else if (tx > 19) t = 7;
                if (ty == 8 && tx > 5 && tx < 22) t = 2;
                if (tx == 14 && ty > 4 && ty < 15) t = 2;
                if (ty == 12 && tx > 6 && tx < 14) t = 5;
                if (t == 2 && (tx == 6 || tx == 14 || tx == 21) && ty == 8) t = 3;
                if (tx == 14 && (ty == 5 || ty == 14)) t = 3;
                map[ty * 30 + tx] = (uint16_t)t;
            }
    }
    return 1;
}
/* The map's two icons: none, unless a suite asks for synthetic art (a red square for the player, a blue one for the
 * cursor, whose second frame is half of it). */
static int iconArt __attribute__((unused));
int CtrPokeTouch_MapPlayerIcon(uint8_t *tiles, uint16_t *pal)
{
    if (!iconArt) return 0;
    memset(tiles, 0x11, 4 * 32); memset(pal, 0, 32); pal[1] = 0x001f; return 1;
}
int CtrPokeTouch_MapCursorIcon(uint8_t *tiles, uint16_t *pal)
{
    if (!iconArt) return 0;
    memset(tiles, 0x22, 4 * 32); memset(tiles + 4 * 32, 0x20, 4 * 32); memset(pal, 0, 32); pal[2] = 0x7c00; return 1;
}
int CtrPokeTouch_MarkShape(int mark, unsigned char *indexes) { (void)mark; (void)indexes; return 0; }
int CtrBottomParty_Icon(int slot, unsigned char *tiles, unsigned short *pal)
{
    /* A flat 32x32 icon in the slot's own colour, a clear border and a hole in the middle, so the 2:1 sample shows. */
    memset(tiles, 0, 1024); memset(pal, 0, 32);
    for (int y = 2; y < 30; ++y)
        for (int x = 2; x < 30; ++x)
            if (!(x >= 12 && x < 20 && y >= 12 && y < 20)) {
                unsigned char *b = &tiles[(y / 8 * 4 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2];
                *b = (unsigned char)(*b | (1 << (x & 1) * 4));
            }
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

/* ---- Canvas, shots and the checks every suite shares ------------------------------------------------------------- */

static uint16_t canvas[W * H], reference[W * H];
static const char *gOut;
static const char *gSuite = "";
static uint16_t at(int x, int y) { return canvas[x * H + H - 1 - y]; }

static void clear(uint16_t c)
{
    for (int i = 0; i < W * H; ++i)
        canvas[i] = c;
}

/* The canvas as dir/<suite>_<state>.ppm, with DEVICE_OUT. */
static void shot(const char *state)
{
    char path[512];
    FILE *f;

    if (!gOut)
        return;
    snprintf(path, sizeof(path), "%s/%s_%s.ppm", gOut, gSuite, state);
    f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            unsigned c = at(x, y);
            fputc((int)((c >> 11) << 3 | (c >> 13)), f);
            fputc((int)(((c >> 5) & 63) << 2 | ((c >> 9) & 3)), f);
            fputc((int)((c & 31) << 3 | ((c >> 2) & 7)), f);
        }
    fclose(f);
}

/* The canvas against `reference`: the first pixel that differs, reported, fails. */
static void SameAsReference(const char *what)
{
    if (!memcmp(reference, canvas, sizeof(canvas)))
        return;
    for (int x = 0; x < W; ++x)
        for (int y = 0; y < H; ++y)
            if (reference[x * H + H - 1 - y] != canvas[x * H + H - 1 - y]) {
                fprintf(stderr, "%s %s: update != draw at %d,%d (update %04x, draw %04x)\n", gSuite, what, x, y,
                        reference[x * H + H - 1 - y], canvas[x * H + H - 1 - y]);
                assert(0);
            }
}

/* Update from `old` into the canvas, then a full Draw of `v`: equal pixel for pixel. `old` becomes `v`; the dirty list is
 * returned. The views are given as 3ds_bottom.c gives them (a suite cases its names). */
static __attribute__((unused)) CtrDirtyList DevCheckTouch(CtrPokeTouchView *old, const CtrPokeTouchView *v)
{
    CtrDirtyList dirty;

    CtrDirty_Clear(&dirty);
    CtrPokeTouch_Update(canvas, old, v, &dirty);
    memcpy(reference, canvas, sizeof(canvas));
    CtrPokeTouch_Draw(canvas, v);
    SameAsReference("touch");
    *old = *v;
    return dirty;
}

static __attribute__((unused)) CtrDirtyList DevCheckBattle(CtrBattleView *old, const CtrBattleView *v)
{
    CtrDirtyList dirty;

    CtrDirty_Clear(&dirty);
    CtrBattleUi_Update(canvas, old, v, &dirty);
    memcpy(reference, canvas, sizeof(canvas));
    CtrBattleUi_Draw(canvas, v);
    SameAsReference("battle");
    *old = *v;
    return dirty;
}

static __attribute__((unused)) CtrDirtyList DevCheckSummary(CtrSummaryUiView *old, const CtrSummaryUiView *v)
{
    CtrDirtyList dirty;

    CtrDirty_Clear(&dirty);
    CtrSummaryUi_Update(canvas, old, v, &dirty);
    memcpy(reference, canvas, sizeof(canvas));
    CtrSummaryUi_Draw(canvas, v);
    SameAsReference("summary");
    *old = *v;
    return dirty;
}

/* The whole screen presented: one rectangle, 0,0 to 320,240. */
static __attribute__((unused)) int DevWhole(CtrDirtyList d)
{
    return d.count == 1 && d.rects[0].x0 == 0 && d.rects[0].y0 == 0 && d.rects[0].x1 == W && d.rects[0].y1 == H;
}

/* A key drawn in its look: the face's colour at (x + 3, the face's middle row). A dimmed key is checked by that pixel
 * directly. */
static uint16_t KeyFace(int look, int sunk)
{
    if (look == DEV_LOOK_DARK)
        return DEV_BEZEL;
    if (look == DEV_LOOK_FAINT)
        return DEV_FAINT;
    return sunk ? DEV_SUNK : DEV_KEY;
}

static __attribute__((unused)) void AssertKey(CtrPokeTouchRect r, int look, int sunk)
{
    int faceY = r.y + (sunk ? DEV_SINK : 0);

    if (at(r.x + 3, faceY + r.h / 2) != KeyFace(look, sunk)) {
        fprintf(stderr, "%s: key {%d,%d,%d,%d} look %d sunk %d: face %04x at %d,%d, wanted %04x\n", gSuite, r.x, r.y, r.w,
                r.h, look, sunk, at(r.x + 3, faceY + r.h / 2), r.x + 3, faceY + r.h / 2, KeyFace(look, sunk));
        assert(0);
    }
}

typedef int (*DevHitFn)(const void *view, int x, int y);

static void HitAt(DevHitFn hit, const void *view, int x, int y, int code, int want)
{
    int got = hit(view, x, y);

    if ((got == code) != want) {
        fprintf(stderr, "%s: hit at %d,%d is %d, %s %d\n", gSuite, x, y, got, want ? "wanted" : "must not be", code);
        assert(0);
    }
}

/* A key with its drop answers `code` from its top-left pixel to the bottom-right one of its drop, and not one past. */
static __attribute__((unused)) void AssertHits(DevHitFn hit, const void *view, CtrPokeTouchRect r, int code)
{
    HitAt(hit, view, r.x, r.y, code, 1);
    HitAt(hit, view, r.x + r.w - 1, r.y + r.h + DEV_DROP - 1, code, 1);
    HitAt(hit, view, r.x - 1, r.y, code, 0);
    HitAt(hit, view, r.x, r.y - 1, code, 0);
    HitAt(hit, view, r.x + r.w, r.y, code, 0);
    HitAt(hit, view, r.x, r.y + r.h + DEV_DROP, code, 0);
}

/* A row (no drop): its own rectangle. */
static __attribute__((unused)) void AssertRowHits(DevHitFn hit, const void *view, CtrPokeTouchRect r, int code)
{
    HitAt(hit, view, r.x, r.y, code, 1);
    HitAt(hit, view, r.x + r.w - 1, r.y + r.h - 1, code, 1);
    HitAt(hit, view, r.x - 1, r.y, code, 0);
    HitAt(hit, view, r.x + r.w, r.y, code, 0);
}

/* ---- kit: the kit's pieces, then a sheet of all of them on the red shell ----------------------------------------- */

/* Pixels of colour c in {x, y, w, h}. */
static int Count(int x, int y, int w, int h, uint16_t c)
{
    int n = 0;

    for (int i = x; i < x + w; ++i)
        for (int j = y; j < y + h; ++j)
            n += at(i, j) == c;
    return n;
}

#define SENTINEL 0x1234

/* Nothing but the sentinel outside {x, y, w, h}. */
static void Untouched(int x, int y, int w, int h)
{
    for (int i = 0; i < W; ++i)
        for (int j = 0; j < H; ++j)
            if (!(i >= x && i < x + w && j >= y && j < y + h))
                assert(at(i, j) == SENTINEL);
}

/* Rework 1's wrap assertions (archive lower_kit_test.c), with the widths of the font in use: the stub's, or the pack's
 * with its widths. Words that do not fit go to the next line, DEV_LINE_H apart; no more than maxLines; nothing outside
 * x..x+width. */
static void KitWrapLines(void)
{
    const uint16_t ink = DEV_INK, sh = DEV_INK_SH;
    int a = UiTextWidth("AAA", 1), ab = UiTextWidth("AAA BBB", 1), room = ab - 1, wide = 2 * ab;
    static uint16_t spaced[W * H];

    /* "AAA BBB" is one pixel wider than the room: each word takes its own line. */
    clear(SENTINEL);
    assert(DevWrap("AAA BBB CCC", 8, 8, room, 3, ink, sh) == 3);
    assert(Count(8, 5, room, DEV_LINE_H, ink) > 0 && Count(8, 5 + DEV_LINE_H, room, DEV_LINE_H, ink) > 0
           && Count(8, 5 + 2 * DEV_LINE_H, room, DEV_LINE_H, ink) > 0);
    Untouched(8, 5, room, 3 * DEV_LINE_H);
    clear(SENTINEL);
    assert(DevWrap("AAA BBB CCC", 8, 8, room, 2, ink, sh) == 2);
    assert(Count(8, 5 + 2 * DEV_LINE_H, room, DEV_LINE_H, ink) == 0);
    clear(SENTINEL);
    assert(DevWrap("AA BB", 8, 8, wide, 3, ink, sh) == 1);           /* fits on one line */
    clear(SENTINEL);
    assert(DevWrap("", 8, 8, wide, 3, ink, sh) == 0);
    Untouched(0, 0, 0, 0);
    /* A line break is a space, drawn as one, never glued into one long word (FireRed's move descriptions keep theirs). */
    clear(SENTINEL);
    assert(DevWrap("AAA\nBBB CCC", 8, 8, room, 3, ink, sh) == 3);
    assert(Count(8, 5 + 2 * DEV_LINE_H, room, DEV_LINE_H, ink) > 0);
    clear(SENTINEL);
    assert(DevWrap("AA BB", 8, 8, wide, 1, ink, sh) == 1);
    memcpy(spaced, canvas, sizeof(spaced));
    clear(SENTINEL);
    assert(DevWrap("AA\nBB", 8, 8, wide, 1, ink, sh) == 1);
    assert(!memcmp(spaced, canvas, sizeof(spaced)));
    clear(SENTINEL);
    assert(DevWrap("\n\nAA", 8, 8, wide, 2, ink, sh) == 1);
    Untouched(8, 5, wide, DEV_LINE_H);
    /* The first line can be narrower than the rest: "AAA BBB" does not fit the first `room`, so BBB starts the second
     * line, which takes "BBB CCC" in its `wide`. */
    clear(SENTINEL);
    assert(DevWrapFirst("AAA BBB CCC", 8, 8, room, wide, 3, ink, sh) == 2);
    assert(Count(8, 5, a, DEV_LINE_H, ink) > 0 && Count(8 + a + 1, 5, wide - a - 1, DEV_LINE_H, ink) == 0);
    assert(Count(8 + a + 1, 5 + DEV_LINE_H, ab - a - 1, DEV_LINE_H, ink) > 0);
    Untouched(8, 5, wide, 2 * DEV_LINE_H);
    /* No shadow when the shadow is the ink. */
    clear(SENTINEL);
    DevWrap("AA", 8, 8, wide, 1, ink, ink);
    assert(Count(0, 0, W, H, sh) == 0 && Count(0, 0, W, H, ink) > 0);
}

static const uint16_t kShell[4] = {DEV_SHELL_RED, DEV_SHELL_BLUE, DEV_SHELL_GREEN, DEV_SHELL_GREY};

/* The kit's contract (the plan's Task 3 assertions). */
static void KitPieces(void)
{
    CtrPokeTouchRect p, k;

    assert(DevBlend(0xFFFF, 0x0000, 0) == 0xFFFF && DevBlend(0xFFFF, 0x0000, 256) == 0x0000);
    assert(DevBlend(UI_RGB(248, 252, 248), 0x0000, 128) == UI_RGB(128, 128, 128));
    for (int s = 0; s < 4; ++s) {
        DevBegin(canvas, (uint8_t)s); DevShell(DEV_SCREWS_TOP | DEV_SCREWS_BOTTOM);
        assert(at(160, 120) == kShell[s] && at(160, 0) == DevBlend(kShell[s], 0xFFFF, 64) && at(160, 1) == at(160, 0) && at(160, 2) == kShell[s]);
        assert(at(160, 237) == DevBlend(kShell[s], 0, 77) && at(160, 239) == at(160, 237) && at(160, 236) == kShell[s]);
        assert(at(4, 4) == DevBlend(kShell[s], 0, 90) && at(315, 234) == DevBlend(kShell[s], 0, 90));
    }
    DevBegin(canvas, 0); DevShell(0); assert(at(4, 4) == DEV_SHELL_RED);
    p = DevScreen(&(CtrPokeTouchRect){8, 8, 304, 140});
    assert(p.x == 14 && p.y == 14 && p.w == 292 && p.h == 128 && at(8, 70) == DEV_BEZEL_LINE && at(9, 70) == DEV_BEZEL
           && at(13, 70) == DEV_BEZEL && at(14, 70) == DEV_PAPER && at(8, 8) == DEV_SHELL_RED && at(160, 148) == DevBlend(DEV_SHELL_RED, 0xFFFF, 64));
    k = (CtrPokeTouchRect){8, 156, 96, 36};                     /* HOME's POKeDEX key, on a fresh shell each time */
    DevShell(0);
    assert(DevKey(&k, DEV_LOOK_KEY, 0, 6) == 156 && at(11, 174) == DEV_KEY && at(56, 156) == 0xFFFF && at(56, 190) == DEV_KEY_EDGE
           && at(56, 191) == DEV_KEY_EDGE && at(56, 192) == DevBlend(DEV_SHELL_RED, 0, 97) && at(56, 194) == at(56, 192) && at(56, 195) == DEV_SHELL_RED && at(8, 156) == DEV_SHELL_RED);
    DevShell(0);
    assert(DevKey(&k, DEV_LOOK_KEY, 1, 6) == 158 && at(56, 157) == DEV_SHELL_RED && at(56, 158) == DEV_SUNK_EDGE && at(11, 176) == DEV_SUNK
           && at(56, 194) == DevBlend(DEV_SHELL_RED, 0, 97) && at(56, 195) == DEV_SHELL_RED && at(94, 164) == DEV_LED);
    DevShell(0);
    assert(DevKey(&k, DEV_LOOK_DARK, 0, 6) == 156 && at(11, 174) == DEV_BEZEL && at(56, 156) == DEV_BEZEL_LINE);
    DevShell(0);
    assert(DevKey(&k, DEV_LOOK_FAINT, 0, 6) == 156 && at(11, 174) == DEV_FAINT && at(56, 190) == DEV_FAINT_EDGE);
    DevShell(0);
    DevKey(&k, DEV_LOOK_KEY, 0, 6); DevDim(&k); assert(at(11, 174) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    DevHole(&(CtrPokeTouchRect){164, 120, 148, 50}, 6); assert(at(238, 150) == DevBlend(DEV_SHELL_RED, 0, 72));
    DevRow(&(CtrPokeTouchRect){16, 48, 244, 16}, 1); assert(at(20, 56) == DEV_BEZEL && DevRowInk(1) == DEV_PAPER && DevRowInk(0) == DEV_INK);
    DevHpBar(76, 52, 150, 6, 112, 112); assert(at(76, 52) == DEV_HP_FRAME && at(100, 54) == DEV_HP_GREEN);
    assert(DevHpColour(50, 112) == DEV_HP_YELLOW && DevHpColour(20, 112) == DEV_HP_RED && DevHpColour(57, 112) == DEV_HP_GREEN);
    assert(DevPpInk(15, 15, DEV_KEY_INK) == DEV_KEY_INK && DevPpInk(15, 35, 0) == DEV_PP_HALF && DevPpInk(3, 15, 0) == DEV_PP_QUARTER && DevPpInk(0, 15, 0) == DEV_LOW_INK);
    assert(DevChosen(2, 2, -1) && !DevChosen(1, 2, -1) && DevChosen(1, 2, 1) && !DevChosen(2, 2, 1));
    clear(0); DevRound(0, 0, 20, 20, 6, 0x1234); assert(at(0, 0) == 0 && at(1, 1) == 0 && at(2, 2) == 0x1234 && at(6, 0) == 0x1234 && at(0, 6) == 0x1234);
    KitWrapLines();
    sDevKeyCut = 0; DevKeyLabel(&(CtrPokeTouchRect){0, 0, 30, 24}, 0, DEV_LOOK_KEY, 0, "A VERY LONG LABEL"); assert(sDevKeyCut == 1); sDevKeyCut = 0;
}

/* Every piece on the red shell, for the eye (kit_sheet): the screen with its text, rows, icons, marks, bars, chips and
 * paper buttons; every key look up and sunk, a dimmed key, a hole, small keys (a tab, arrows, a pill). */
static void KitSheet(void)
{
    static const CtrPokeTouchRect kKeys[8] = {{8, 106, 72, 36}, {84, 106, 72, 36}, {160, 106, 72, 36}, {236, 106, 76, 36},
                                               {8, 150, 72, 36}, {84, 150, 72, 36}, {8, 194, 72, 30}, {84, 196, 56, 24}};
    static const int kLook[8] = {DEV_LOOK_KEY, DEV_LOOK_KEY, DEV_LOOK_DARK, DEV_LOOK_DARK, DEV_LOOK_FAINT, DEV_LOOK_FAINT,
                                 DEV_LOOK_KEY, DEV_LOOK_KEY};
    static const int kSunk[8] = {0, 1, 0, 1, 0, 1, 1, 1};
    static const char *const kLabel[8] = {"POKéDEX", "POKéMON", "CANCEL", "BACK", "FAINTED", "CHOSEN", "SAVE", "INFO"};
    static const unsigned kHp[3] = {112, 50, 9};
    CtrPokeTouchRect p, dim = {160, 150, 72, 36}, hole = {236, 150, 76, 36}, pill = {248, 196, 64, 20};
    int x, faceY;

    DevBegin(canvas, 0);
    DevShell(DEV_SCREWS_TOP | DEV_SCREWS_BOTTOM);
    p = DevScreen(&(CtrPokeTouchRect){8, 8, 304, 92});
    /* Text and rows (x 16..148). */
    DevText("VERMILION", p.x + 4, p.y + 4, p.x + 134);
    DevText2("Secondary ink", p.x + 4, p.y + 17, p.x + 134);
    DevSmallText("KANTO POW ACC PP Lv", p.x + 4, p.y + 31, DEV_INK2, p.x + 134);
    DevRow(&(CtrPokeTouchRect){p.x + 2, p.y + 42, 132, 16}, 1);
    DevTextInk("POTION", p.x + 6, p.y + 46, p.x + 110, DevRowInk(1), DevRowInk(1));
    DevTextRight("x5", p.x + 130, p.y + 46, DevRowInk(1));
    DevRow(&(CtrPokeTouchRect){p.x + 2, p.y + 58, 132, 16}, 0);
    DevTextInk("ANTIDOTE", p.x + 6, p.y + 62, p.x + 110, DevRowInk(0), DEV_INK_SH);
    DevTextRight("x3", p.x + 130, p.y + 62, DevPpInk(3, 15, DevRowInk(0)));
    /* Icons, marks, bars (x 152..236). */
    DevMonIcon(0, 1, 152, p.y + 1, 0, 0);
    DevMonIcon(1, 2, 186, p.y + 1, 1, 0);
    DevMonIcon(2, 3, 186, p.y + 18, 1, 1);
    DevGender(0, 206, p.y + 3);
    DevGender(1, 206, p.y + 19);
    DevTriangle(226, p.y + 7, 0, DEV_INK);
    DevTriangle(226, p.y + 23, 1, DEV_INK);
    for (int i = 0; i < 3; ++i) {
        int w = DevLabelChip(152, p.y + 38 + i * 10, "HP", DEV_HP_FRAME, DEV_HP_LABEL);

        DevHpBar(154 + w, p.y + 40 + i * 10, 82 - w, 5, kHp[i], 112);
    }
    x = DevLabelChip(152, p.y + 68, "EXP", DEV_HP_FRAME, DEV_EXP_LABEL);
    Fill(152 + x, p.y + 68, 40, 9, DEV_HP_TRACK);
    Fill(152 + x, p.y + 70, 26, 5, DEV_EXP);
    /* Paper buttons and chips (x 240..304). */
    DevPaperButton(&(CtrPokeTouchRect){244, p.y + 2, 44, 16}, 0);
    DevTextInk("RUN", 248, p.y + 6, 286, DevRowInk(0), DEV_INK_SH);
    DevPaperButton(&(CtrPokeTouchRect){244, p.y + 20, 44, 16}, 1);
    DevTextInk("2x", 248, p.y + 24, 286, DevRowInk(1), DevRowInk(1));
    DevChip(240, p.y + 40, 10, "FIRE", UiTypeColor(10));
    DevChip(240, p.y + 52, 10, "SPECIAL", DEV_CAT_SPECIAL);
    DevChip(240, p.y + 64, 10, "SUPER", DEV_FX_SUPER);
    DevChip(276, p.y + 40, 10, "PSN", DEV_PSN);
    DevChip(276, p.y + 64, 10, "FNT", DEV_FNT);
    /* The keys: KEY, DARK and FAINT up and sunk; a dimmed key, a hole; a short sunk key, a sunk tab, two dark arrows, a
     * key arrow, a dark pill. */
    for (int i = 0; i < 8; ++i) {
        faceY = DevKey(&kKeys[i], kLook[i], kSunk[i], i == 7 ? 5 : 6);
        DevKeyLabel(&kKeys[i], faceY, kLook[i], kSunk[i], kLabel[i]);
        AssertKey(kKeys[i], kLook[i], kSunk[i]);
    }
    faceY = DevKey(&dim, DEV_LOOK_KEY, 0, 6);
    DevKeyLabel(&dim, faceY, DEV_LOOK_KEY, 0, "DIMMED");
    DevDim(&dim);
    assert(at(dim.x + 3, dim.y + dim.h / 2) == DevBlend(DEV_SHELL_RED, DEV_KEY, DEV_A_DIM));
    DevHole(&hole, 6);
    faceY = DevKey(&(CtrPokeTouchRect){144, 196, 28, 24}, DEV_LOOK_DARK, 0, 5);
    DevTriangle(158, faceY + 11, 0, DEV_DARK_INK);
    faceY = DevKey(&(CtrPokeTouchRect){176, 196, 28, 24}, DEV_LOOK_DARK, 0, 5);
    DevTriangle(190, faceY + 11, 1, DEV_DARK_INK);
    faceY = DevKey(&(CtrPokeTouchRect){208, 194, 36, 30}, DEV_LOOK_KEY, 0, 6);
    DevTriangle(226, faceY + 13, 3, DEV_KEY_INK);
    faceY = DevKey(&pill, DEV_LOOK_DARK, 0, 10);
    DevKeyLabel(&pill, faceY, DEV_LOOK_DARK, 0, "CANCEL");
    shot("sheet");
}

static void SuiteKit(void)
{
    KitPieces();
    KitSheet();
    assert(sDevKeyCut == 0);
}

/* ---- main --------------------------------------------------------------------------------------------------------- */

static const struct { const char *name; void (*run)(void); } kSuites[] = {
    {"kit", SuiteKit},
};

int main(int argc, char **argv)
{
    const char *pack = getenv("FIRERED_TEST_PAK"), *stub = getenv("DEVICE_FONT_W");
    int all = argc > 1 && !strcmp(argv[1], "all"), ran = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: device_render_test <suite> (kit, all)\n");
        return 2;
    }
    gOut = getenv("DEVICE_OUT");
    if (stub && atoi(stub) > 1 && atoi(stub) <= 16)
        gStubW = (unsigned)atoi(stub);
    SyntheticFont(font, 0);
    SyntheticFont(smallFont, 1);
    if (pack) {
        PackOpen(pack);
        if (!PackFont("graphics/fonts/latin_normal.fwlatfont", font, sizeof(font))
            || !PackFont("graphics/fonts/latin_small.hwlatfont", smallFont, sizeof(smallFont))) {
            SyntheticFont(font, 0);
            SyntheticFont(smallFont, 1);
            puts("NOTE device: synthetic font");
        }
    }
    ReadWidths(getenv("DEVICE_WIDTHS"), fontWidths);
    ReadWidths(getenv("DEVICE_SMALL_WIDTHS"), smallWidths);
    for (unsigned i = 0; i < sizeof(kSuites) / sizeof(kSuites[0]); ++i)
        if (all || !strcmp(argv[1], kSuites[i].name)) {
            gSuite = kSuites[i].name;
            sDevKeyCut = 0;
            kSuites[i].run();
            printf("PASS device %s\n", kSuites[i].name);
            ++ran;
        }
    if (!ran) {
        fprintf(stderr, "no suite %s\n", argv[1]);
        return 2;
    }
    if (all)
        puts("PASS device all");
    if (realPack)
        fclose(realPack);
    free(realIndex);
    return 0;
}
