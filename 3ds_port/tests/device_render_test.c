/* The red device (style B): its drawing kit (3ds_device_kit_impl.h) and the B views through the production renderer,
 * with synthetic game bridges.
 *
 * Each suite draws its view's states and checks them: an Update equals a full Draw pixel for pixel, every key is hit
 * exactly where it is drawn (3ds_device_layout.h is the one source of both), and no port-authored key label is cut
 * (sDevKeyCut stays 0). No ROM-derived data: the fonts are synthetic stubs, every glyph DEVICE_FONT_W px wide (default
 * 5; the small font's 5), unless FIRERED_TEST_PAK names the owner's own pack, whose glyphs are then read (a local
 * viewing aid, tools/render_device_views.py; never committed). What the kit is told each view draws (its DEV_TRACE,
 * defined here before the renderer) is always collected; a state keeps the pieces of its last full draw. Environment:
 *   DEVICE_OUT=dir            one PPM per drawn state, dir/<suite>_<state>.ppm
 *   DEVICE_TRACE=dir          one trace per drawn state, dir/<suite>_<state>.json (tools/make_device_boards.py)
 *   FIRERED_TEST_PAK=pak      the pack (ABI 0x8f71cf3a, its ROM's SHA-1), opened as poketouch_render_test.c does
 *   DEVICE_WIDTHS=file        the normal font's 256 glyph widths; DEVICE_SMALL_WIDTHS=file the small font's
 *   DEVICE_FONT_W=n           the stub glyph width
 *
 * usage: device_render_test <suite>   (kit, home, battle, party, summary, bag, options, save, card, mail, dex, choice,
 *        learn, relearn, naming, box, easychat, frame, idle, or all; prints PASS device <suite>) */
#include <assert.h>
/* The kit's trace: each piece it draws is told to TracePiece (with the canvas and the shots, below). */
static void TracePiece(int op, int x, int y, int w, int h, int a, int b, const char *s);
#define DEV_TRACE(op, x, y, w, h, a, b, s) TracePiece((op), (x), (y), (w), (h), (int)(a), (int)(b), (s))
#ifndef PT_SOURCE
#define PT_SOURCE "3ds_poketouch.c"
#endif
#include PT_SOURCE
#include "3ds_text_case_views.h"   /* the game names in proper case, as 3ds_bottom.c gives them */
#include "3ds_nature.h"            /* the Summary's nature marks */
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

/* ---- Game bridges: the battle's (the ember drawer's harness had them) and poketouch_render_test.c's map art ------- */

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
/* CtrPokeTouch_DexMonPic: device_suite_learn.h's (a species' picture while a suite sets picArt). */
int CtrPokeTouch_DexMonPic80(unsigned species, unsigned char *pixels, unsigned short *palette)
{ (void)species; (void)pixels; (void)palette; return 0; }
int CtrPokeTouch_DexMenuIcon(int id, unsigned char *tiles, unsigned short *palette)
{ (void)id; (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_DexFootprint(unsigned species, unsigned char *bits)
{ (void)species; (void)bits; return 0; }
/* CtrPokeTouch_DexCaughtMarker: device_suite_dex.h's (its caught mark while a suite sets dexArt). */
/* The Trainer Card's art: none, unless a suite asks for synthetic art (cardArt): the trainer a 48x48 square of index 1
 * (green) in a clear border of 8, every badge a 16x16 square of index 2 (blue). */
static int cardArt __attribute__((unused));
#define CARD_ART_GREEN 0x03e0   /* GBA BGR555 */
#define CARD_ART_BLUE 0x7c00
int CtrPokeTouch_CardTrainerPic(unsigned char *tiles, unsigned short *palette)
{
    if (!cardArt) return 0;
    memset(tiles, 0, 0x800); memset(palette, 0, 32); palette[1] = CARD_ART_GREEN;
    for (int y = 8; y < 56; ++y)
        for (int x = 8; x < 56; ++x)
            tiles[(y / 8 * 8 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2] |= (unsigned char)(1 << (x & 1) * 4);
    return 1;
}
int CtrPokeTouch_CardBadges(unsigned char *tiles, unsigned short *palette)
{
    if (!cardArt) return 0;
    memset(tiles, 0x22, 0x400); memset(palette, 0, 32); palette[2] = CARD_ART_BLUE; return 1;
}
/* A Pokemon's icon (the naming keyboard's, the PC box's): none, unless a suite asks for synthetic art (monArt 1: a 24x24
 * square of index 1 in STUB_ICON in a clear border of 4; 2: the whole 32x32 square). Every call writes both frames, the
 * PT_MON_ICON_BYTES the game's bridge writes, so a smaller buffer is overrun here too; so do the Party's and the
 * Summary's icon stubs below. */
#define MON_ART_ORANGE 0x021f                   /* GBA BGR555 */
#define STUB_ICON GbaColor(MON_ART_ORANGE)
static int monArt;
int CtrPokeTouch_MonIcon(unsigned species, unsigned personality, int egg, unsigned char *tiles, unsigned short *palette)
{
    int border = monArt == 2 ? 0 : 4;

    (void)species; (void)personality; (void)egg;
    memset(tiles, 0, PT_MON_ICON_BYTES); memset(palette, 0, 32);
    if (!monArt) return 0;
    palette[1] = MON_ART_ORANGE;
    for (int y = border; y < 32 - border; ++y)
        for (int x = border; x < 32 - border; ++x)
            tiles[(y / 8 * 4 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2] |= (unsigned char)(1 << (x & 1) * 4);
    return 1;
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
/* A mark's shape (the PC box's marks): none, unless a suite asks for synthetic art (markArt): a 4x4 square of index 3 in
 * a ring of index 4 (the mark and its edge), 6x6 in all, in the 8x8's rows and columns 1-6. */
static int markArt;
int CtrPokeTouch_MarkShape(int mark, unsigned char *indexes)
{
    (void)mark;
    if (!markArt) return 0;
    for (int i = 0; i < 64; ++i) {
        int x = i % 8, y = i / 8;

        indexes[i] = (unsigned char)(x >= 2 && x <= 5 && y >= 2 && y <= 5 ? 3 : x >= 1 && x <= 6 && y >= 1 && y <= 6 ? 4 : 0);
    }
    return 1;
}
int CtrBottomParty_Icon(int slot, unsigned char *tiles, unsigned short *pal)
{
    /* A flat 32x32 icon in the slot's own colour, a clear border and a hole in the middle, so the 2:1 sample shows. */
    memset(tiles, 0, PT_MON_ICON_BYTES); memset(pal, 0, 32);
    for (int y = 2; y < 30; ++y)
        for (int x = 2; x < 30; ++x)
            if (!(x >= 12 && x < 20 && y >= 12 && y < 20)) {
                unsigned char *b = &tiles[(y / 8 * 4 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2];
                *b = (unsigned char)(*b | (1 << (x & 1) * 4));
            }
    pal[1] = (unsigned short)((slot * 5 + 6) | (31 - slot * 4) << 5 | (slot * 3) << 10);
    return 1;
}

/* The Summary's Pokemon: none, unless a suite asks for synthetic art (sumArt 1: a 64x64 picture, 2: an 80x80 one, 3: only
 * the icon): a disc of index 1 (red) with a ring of index 2 (blue) round it, and the icon a square of index 1. */
static int sumArt __attribute__((unused));
#define SUM_ART_RED 0x001f      /* GBA BGR555 */
#define SUM_ART_BLUE 0x7c00
static int SumArtIndex(int x, int y, int size)
{
    int dx = 2 * x + 1 - size, dy = 2 * y + 1 - size, d = dx * dx + dy * dy, r = size - 8;

    return d <= r * r ? 1 : d <= size * size ? 2 : 0;
}
unsigned CtrBottomSummary_IconSpecies(void) { return 0; }
int CtrBottomSummary_Icon(unsigned char *tiles, unsigned short *pal)
{
    memset(tiles, 0, PT_MON_ICON_BYTES); memset(pal, 0, 32);
    if (!sumArt) return 0;
    memset(tiles, 0x11, PT_MON_ICON_BYTES); pal[1] = SUM_ART_RED; return 1;
}
int CtrBottomSummary_Pic(unsigned char *tiles, unsigned short *pal, int *flip)
{
    if (sumArt != 1) return 0;
    memset(tiles, 0, 0x800); memset(pal, 0, 32); pal[1] = SUM_ART_RED; pal[2] = SUM_ART_BLUE; *flip = 0;
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            tiles[(y / 8 * 8 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2] |= (unsigned char)(SumArtIndex(x, y, 64) << (x & 1) * 4);
    return 1;
}
int CtrBottomSummary_Pic80(unsigned char *pixels, unsigned short *pal, int *flip)
{
    if (sumArt != 2) return 0;
    memset(pixels, 0, 80 * 80 / 2); memset(pal, 0, 32); pal[1] = SUM_ART_RED; pal[2] = SUM_ART_BLUE; *flip = 0;
    for (int y = 0; y < 80; ++y)
        for (int x = 0; x < 80; ++x)
            pixels[y * 40 + x / 2] |= (unsigned char)(SumArtIndex(x, y, 80) << (x & 1) * 4);
    return 1;
}
/* A box's wallpaper (the PC box's): none, unless a suite asks for synthetic art (wallArt): every pixel of the 160x144 its
 * own colour, WallArtColor(id, x, y), so where a pixel lands shows. */
static int wallArt;
static unsigned short WallArtColor(unsigned id, int x, int y)
{
    return (unsigned short)((x & 31) | (y & 31) << 5 | (((x >> 5) + 5 * (y >> 5) + (int)id) & 31) << 10);
}
int CtrPokeTouch_Wallpaper(unsigned id, unsigned short *rgb555)
{
    if (!wallArt) return 0;
    for (int i = 0; i < 160 * 144; ++i)
        rgb555[i] = WallArtColor(id, i % 160, i / 160);
    return 1;
}

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

/* ---- The trace: what the kit is told the view draws (3ds_device_kit_impl.h's DEV_TRACE), for a board ------------- */

#define TRACE_MAX 1024                          /* pieces of one draw (the most a state draws: 231) */
#define TRACE_TEXT 256                          /* bytes of one piece's text */

static struct {
    int op, x, y, w, h, a, b;
    char s[TRACE_TEXT];
} gTrace[TRACE_MAX];
static int gTraceCount, gTraceLost, gTraceShot;
static int gTraceShell = DEV_SHELL_RED;
static const char *gTraceDir;

/* The bytes of `s` the kit drew w pixels of (DevGlyphs: its glyphs while they fit). */
static size_t TraceDrawn(const char *s, int w, int small)
{
    const char *p = s;

    for (int x = 0; *p;) {
        const char *q = p;
        int gw = (small ? DevSmallGlyph(&q) : Glyph(&q))->width;

        if (x + gw > w)
            break;
        x += gw;
        p = q;
    }
    return (size_t)(p - s);
}

/* DEV_TRACE: a piece kept (a text with nothing drawn left out), or BEGIN, which drops them all. */
static void TracePiece(int op, int x, int y, int w, int h, int a, int b, const char *s)
{
    size_t n = !s ? 0 : op == DEV_TRACE_TEXT && b < 2 ? TraceDrawn(s, w, b) : strlen(s);

    if (op == DEV_TRACE_BEGIN) {                /* a new whole draw: what was told before is gone */
        gTraceCount = gTraceLost = 0;
        gTraceShell = a;
        return;
    }
    if (op == DEV_TRACE_TEXT && w <= 0)
        return;
    if (gTraceCount == TRACE_MAX) {
        ++gTraceLost;
        return;
    }
    assert(n < TRACE_TEXT);
    gTrace[gTraceCount].op = op;
    gTrace[gTraceCount].x = x;
    gTrace[gTraceCount].y = y;
    gTrace[gTraceCount].w = w;
    gTrace[gTraceCount].h = h;
    gTrace[gTraceCount].a = a;
    gTrace[gTraceCount].b = b;
    memcpy(gTrace[gTraceCount].s, s ? s : "", n);
    gTrace[gTraceCount++].s[n] = 0;
}

/* The colours the views name (the kit's and the plan-3 views' own), as the boards give them: each name's UI_RGB(r, g,
 * b) read back as 0xrrggbb. A board shows a colour drawn as one of these in its board value. */
#pragma push_macro("UI_RGB")
#undef UI_RGB
#define UI_RGB(r, g, b) ((unsigned)(r) << 16 | (unsigned)(g) << 8 | (unsigned)(b))
static const unsigned kTraceRgb[] = {
    DEV_SHELL_RED, DEV_SHELL_BLUE, DEV_SHELL_GREEN, DEV_SHELL_GREY, DEV_BEZEL, DEV_BEZEL_LINE, DEV_PAPER, DEV_PAPER2,
    DEV_INK, DEV_INK_SH, DEV_INK2, DEV_INK_DIM, DEV_KEY, DEV_KEY_EDGE, DEV_KEY_INK, DEV_SUNK, DEV_SUNK_EDGE, DEV_SUNK_INK,
    DEV_SUNK_INK_SH, DEV_LED, DEV_DARK_INK, DEV_FAINT, DEV_FAINT_EDGE, DEV_FAINT_INK, DEV_BUTTON_EDGE, DEV_HP_FRAME,
    DEV_HP_TRACK, DEV_HP_GREEN, DEV_HP_YELLOW, DEV_HP_RED, DEV_HP_LABEL, DEV_EXP, DEV_EXP_LABEL, DEV_FX_SUPER,
    DEV_FX_NOTVERY, DEV_FX_EFFECTIVE, DEV_FX_NONE, DEV_CAT_SPECIAL, DEV_CAT_PHYSICAL, DEV_PSN, DEV_FX_SUPER_SH,
    DEV_FX_NOTVERY_SH, DEV_FX_EFFECTIVE_SH, DEV_CAT_SPECIAL_SH, DEV_LOW_INK, DEV_PP_QUARTER, DEV_PP_HALF, DEV_BLUE_INK,
    DEV_STAT_UP, DEV_MALE, DEV_FEMALE, DEV_BAG_ARROW_OFF, DEV_BOX_TITLE_SH, DEV_CARD_GOLD,
};
#pragma pop_macro("UI_RGB")

/* RGB565 c as "#rrggbb" into out: a named colour's board value, any other (a blend, a type's) widened. */
static const char *TraceHex(int c, char out[8])
{
    for (size_t i = 0; i < sizeof(kTraceRgb) / sizeof(kTraceRgb[0]); ++i)
        if (UI_RGB(kTraceRgb[i] >> 16, (kTraceRgb[i] >> 8) & 255, kTraceRgb[i] & 255) == c) {
            snprintf(out, 8, "#%06x", kTraceRgb[i]);
            return out;
        }
    snprintf(out, 8, "#%02x%02x%02x", (c >> 11) << 3 | (c >> 13), ((c >> 5) & 63) << 2 | ((c >> 9) & 3),
             (c & 31) << 3 | ((c >> 2) & 7));
    return out;
}

/* The character FireRed's code `code` draws, in UTF-8: the port's own spelling of it (GlyphCode's, read back), the
 * gender signs, the ellipsis and "Lv"; "?" for any other. */
static const char *TraceCodeText(unsigned code, char out[4])
{
    static const unsigned char kLead[3] = {0xc2, 0xc3, 0xc5};

    for (unsigned c = 0x20; c < 0x7f; ++c) {
        const char src[2] = {(char)c, 0}, *p = src;

        if (GlyphCode(&p) == code) {
            out[0] = (char)c; out[1] = 0;
            return out;
        }
    }
    for (int i = 0; i < 3; ++i)
        for (unsigned c = 0x80; c < 0xc0; ++c) {
            const char src[3] = {(char)kLead[i], (char)c, 0}, *p = src;

            if (GlyphCode(&p) == code) {
                out[0] = (char)kLead[i]; out[1] = (char)c; out[2] = 0;
                return out;
            }
        }
    return code == 0xb5 ? "\xE2\x99\x82" : code == 0xb6 ? "\xE2\x99\x80" : code == 0xb0 ? "\xE2\x80\xA6"
         : code == 0x34 ? "Lv" : "?";
}

/* `s` as a JSON string: UTF-8 as it is, a FireRed code (0x01 escaped) as the character it draws; a Japanese one (0x02
 * escaped) or a byte that is not UTF-8 as "?", a control as a space. */
static void TraceJsonText(FILE *f, const char *s)
{
    char text[4];

    fputc('"', f);
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        int more = c >= 0xc2 && c <= 0xdf ? 1 : c >= 0xe0 && c <= 0xef ? 2 : 0, k = 0;

        while (k < more && ((unsigned char)s[k] & 0xc0) == 0x80)
            ++k;
        if ((c == 0x01 || c == 0x02) && *s) {
            const char *drawn = c == 0x01 ? TraceCodeText((unsigned char)*s, text) : "?";

            ++s;
            fputs(!strcmp(drawn, "\"") ? "\\\"" : !strcmp(drawn, "\\") ? "\\\\" : drawn, f);
        } else if (c == '"' || c == '\\')
            fprintf(f, "\\%c", c);
        else if (c < 0x20)
            fputc(' ', f);
        else if (c < 0x80)
            fputc(c, f);
        else if (more && k == more) {
            fputc(c, f);
            fwrite(s, 1, (size_t)more, f);
            s += more;
        } else
            fputc('?', f);
    }
    fputc('"', f);
}

/* The trace of the last whole draw as dir/<suite>_<state>.json, with DEVICE_TRACE: {"suite", "state", "shot" (its
 * place among the suite's), "shell", "items": [...]} in draw order (tools/make_device_boards.py reads it). */
static void TraceWrite(const char *state)
{
    static const char *const kLook[3] = {"key", "dark", "faint"};
    static const char *const kArt[] = {"icon", "picture", "item", "badge", "wallpaper", "mark", "sprite", "frame",
                                       "logo", "gba"};
    static const char *const kDir[4] = {"up", "down", "left", "right"};
    static const char *const kOp[] = {"begin", "art", "shell", "screen", "key", "dim", "hole", "row", "rect", "led",
                                      "text", "chip", "bar", "tri"};
    char path[512], hex[8], hex2[8];
    FILE *f;

    ++gTraceShot;
    if (!gTraceDir)
        return;
    if (gTraceLost) {
        fprintf(stderr, "%s %s: the trace lost %d pieces (TRACE_MAX)\n", gSuite, state, gTraceLost);
        assert(0);
    }
    snprintf(path, sizeof(path), "%s/%s_%s.json", gTraceDir, gSuite, state);
    f = fopen(path, "wb");
    assert(f);
    fprintf(f, "{\"suite\": \"%s\", \"state\": \"%s\", \"shot\": %d, \"shell\": \"%s\", \"items\": [", gSuite, state,
            gTraceShot, TraceHex(gTraceShell, hex));
    for (int i = 0; i < gTraceCount; ++i) {
        const int op = gTrace[i].op, a = gTrace[i].a, b = gTrace[i].b;

        fprintf(f, "%s\n  {\"op\": \"%s\", \"x\": %d, \"y\": %d, \"w\": %d", i ? "," : "", kOp[op], gTrace[i].x,
                gTrace[i].y, gTrace[i].w);
        if (op != DEV_TRACE_TEXT)
            fprintf(f, ", \"h\": %d", gTrace[i].h);
        if (op == DEV_TRACE_SHELL)
            fprintf(f, ", \"screws\": %d", a);
        else if (op == DEV_TRACE_KEY)
            fprintf(f, ", \"look\": \"%s\", \"sunk\": %d", kLook[a], b);
        else if (op == DEV_TRACE_HOLE)
            fprintf(f, ", \"radius\": %d", a);
        else if (op == DEV_TRACE_ROW)
            fprintf(f, ", \"chosen\": %d", a);
        else if (op == DEV_TRACE_RECT)
            fprintf(f, ", \"fill\": \"%s\"", TraceHex(a, hex));
        else if (op == DEV_TRACE_TEXT) {
            fprintf(f, ", \"font\": \"%s\", \"ink\": \"%s\"", b == 1 ? "small" : "normal", TraceHex(a, hex));
            if (gTrace[i].h != a)
                fprintf(f, ", \"shadow\": \"%s\"", TraceHex(gTrace[i].h, hex2));
        } else if (op == DEV_TRACE_CHIP)
            fprintf(f, ", \"fill\": \"%s\", \"shadow\": \"%s\"", TraceHex(a, hex), TraceHex(b, hex2));
        else if (op == DEV_TRACE_BAR)
            fprintf(f, ", \"fill\": \"%s\", \"filled\": %d", TraceHex(a, hex), b);
        else if (op == DEV_TRACE_TRI)
            fprintf(f, ", \"ink\": \"%s\", \"dir\": \"%s\"", TraceHex(a, hex), kDir[b & 3]);
        else if (op == DEV_TRACE_ART)
            fprintf(f, ", \"kind\": \"%s\"", kArt[a]);
        if (op == DEV_TRACE_TEXT || op == DEV_TRACE_CHIP) {
            fputs(", \"text\": ", f);
            TraceJsonText(f, gTrace[i].s);
        }
        fputc('}', f);
    }
    fputs("\n]}\n", f);
    fclose(f);
}

/* The canvas as dir/<suite>_<state>.ppm, with DEVICE_OUT; the trace as dir/<suite>_<state>.json, with DEVICE_TRACE. */
static void shot(const char *state)
{
    char path[512];
    FILE *f;

    TraceWrite(state);
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

/* BACK of a field view without a board, as the dispatcher hits it (CtrPokeTouch_BackHit): 1 on it. */
static __attribute__((unused)) int BackV(const void *view, int x, int y)
{
    return CtrPokeTouch_BackHit((const CtrPokeTouchView *)view, x, y);
}

/* The dispatcher's row that draws `view` is the one whose `shown` this is (a view's own row, or DevFrameShown: the
 * frame, FireRed's picture in its screen). */
static __attribute__((unused)) int DrawnBy(const CtrPokeTouchView *view, int (*shown)(const CtrPokeTouchView *))
{
    int i = DevFieldIndex(view);

    return i >= 0 && kDevFieldViews[i].shown == shown;
}

/* With small letters wider than capitals (7 px in both fonts): `a` and `b` drawn alike, or (`same` 0) not. A suite's
 * upper-case check: a word drawn through DevUpper looks the same in either case, one drawn as typed does not. */
static __attribute__((unused)) void TouchSamePixels(const CtrPokeTouchView *a, const CtrPokeTouchView *b, int same,
                                                    const char *what)
{
    static uint16_t first[W * H];
    static uint8_t keepWidths[256], keepSmall[256];

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    CtrPokeTouch_Draw(canvas, a);
    memcpy(first, canvas, sizeof(first));
    CtrPokeTouch_Draw(canvas, b);
    if ((memcmp(first, canvas, sizeof(first)) == 0) != same) {
        fprintf(stderr, "%s: %s drawn %s\n", gSuite, what, same ? "differently" : "alike");
        assert(0);
    }
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
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

/* The same in rectangle r. */
static __attribute__((unused)) int CountIn(CtrPokeTouchRect r, uint16_t c)
{
    return Count(r.x, r.y, r.w, r.h, c);
}

/* A dark key dimmed (DevDim): its face halfway to the red shell. */
#define DIM_DARK DevBlend(DEV_SHELL_RED, DEV_BEZEL, DEV_A_DIM)

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

/* The HP and PP colours of FireRed's own battle screens (the top screen shows the same). */
static void KitBarLevels(void)
{
    /* HP: the share of the HUD's 48-pixel bar, rounded down, at least 1 while hp > 0: > 24 green, > 9 yellow, else red. */
    assert(DevHpColour(112, 112) == DEV_HP_GREEN && DevHpColour(59, 112) == DEV_HP_GREEN);           /* 25 px */
    assert(DevHpColour(57, 112) == DEV_HP_YELLOW && DevHpColour(58, 112) == DEV_HP_YELLOW);          /* 24 px: not over */
    assert(DevHpColour(50, 112) == DEV_HP_YELLOW && DevHpColour(24, 112) == DEV_HP_YELLOW);          /* 21, 10 px */
    assert(DevHpColour(23, 112) == DEV_HP_RED && DevHpColour(20, 112) == DEV_HP_RED);                /* 9, 8 px */
    assert(DevHpColour(1, 600) == DEV_HP_RED && DevHpColour(0, 112) == DEV_HP_RED);                  /* 1 px at least; empty */
    assert(DevHpColour(0, 0) == DEV_HP_GREEN);                                                       /* hp == max: full */
    /* PP: full is the usual ink; above 7, none / a quarter or less / more than half / the rest (rounded down). */
    assert(DevPpInk(15, 15, DEV_KEY_INK) == DEV_KEY_INK && DevPpInk(15, 35, 0) == DEV_PP_HALF && DevPpInk(3, 15, 0) == DEV_PP_QUARTER && DevPpInk(0, 15, 0) == DEV_LOW_INK);
    assert(DevPpInk(17, 35, 0) == DEV_PP_HALF && DevPpInk(18, 35, 9) == 9 && DevPpInk(8, 35, 0) == DEV_PP_QUARTER && DevPpInk(9, 35, 0) == DEV_PP_HALF);
    assert(DevPpInk(4, 8, 0) == DEV_PP_HALF && DevPpInk(5, 8, 9) == 9 && DevPpInk(2, 8, 0) == DEV_PP_QUARTER);
    /* A maximum of 7 or less goes by what is left: more than 2 usual, 2 half, 1 quarter, 0 empty. */
    assert(DevPpInk(6, 6, 9) == 9 && DevPpInk(3, 6, 9) == 9 && DevPpInk(2, 6, 9) == DEV_PP_HALF && DevPpInk(1, 6, 9) == DEV_PP_QUARTER && DevPpInk(0, 6, 9) == DEV_LOW_INK);
    assert(DevPpInk(7, 7, 9) == 9 && DevPpInk(3, 7, 9) == 9 && DevPpInk(2, 7, 9) == DEV_PP_HALF && DevPpInk(1, 7, 9) == DEV_PP_QUARTER && DevPpInk(0, 7, 9) == DEV_LOW_INK);
    /* A maximum of 2 or less: 1 left is the quarter colour, 0 empty. */
    assert(DevPpInk(2, 2, 9) == 9 && DevPpInk(1, 2, 9) == DEV_PP_QUARTER && DevPpInk(0, 2, 9) == DEV_LOW_INK);
    assert(DevPpInk(1, 1, 9) == 9 && DevPpInk(0, 1, 9) == DEV_LOW_INK);
}

/* Two RGB565 colours within one step of each other in every channel. */
static int Near565(uint16_t a, uint16_t b)
{
    int dr = (a >> 11) - (b >> 11), dg = ((a >> 5) & 63) - ((b >> 5) & 63), db = (a & 31) - (b & 31);

    return dr >= -1 && dr <= 1 && dg >= -1 && dg <= 1 && db >= -1 && db <= 1;
}

/* Chips: white small-font text with the chip's own dark shadow (none when it is DEV_WHITE); the width follows the text
 * with 4 px each side, or is fixed with the text centred. DevChipShadow lands on the boards' pairs. */
static void KitChips(void)
{
    static const struct { uint16_t bg, shadow; } kBoard[7] = {
        {UI_RGB(0xE8, 0x6E, 0x28), UI_RGB(0x8A, 0x3A, 0x10)}, {UI_RGB(0x5A, 0x78, 0xC0), UI_RGB(0x2E, 0x40, 0x70)},
        {UI_RGB(0x28, 0x96, 0x46), UI_RGB(0x14, 0x5A, 0x28)}, {UI_RGB(0x64, 0x6A, 0x84), UI_RGB(0x3A, 0x3E, 0x52)},
        {UI_RGB(0x8C, 0x8C, 0x78), UI_RGB(0x4A, 0x4A, 0x3A)}, {UI_RGB(0xB8, 0x40, 0x2C), UI_RGB(0x6A, 0x20, 0x14)},
        {UI_RGB(0xCC, 0x54, 0x18), UI_RGB(0x7A, 0x30, 0x08)},
    };
    const uint16_t bg = kBoard[0].bg, sh = DevChipShadow(bg);
    int tw = DevSmallTextWidth("FIRE"), pw = DevSmallTextWidth("PSN") + 8, white = 0, shade = 0, left = W, right = -1;

    for (int i = 0; i < 7; ++i)
        assert(Near565(DevChipShadow(kBoard[i].bg), kBoard[i].shadow));
    assert(Near565(DevChipShadow(DEV_FX_SUPER), DEV_FX_SUPER_SH) && Near565(DevChipShadow(DEV_FX_NOTVERY), DEV_FX_NOTVERY_SH)
           && Near565(DevChipShadow(DEV_FX_EFFECTIVE), DEV_FX_EFFECTIVE_SH) && Near565(DevChipShadow(DEV_CAT_SPECIAL), DEV_CAT_SPECIAL_SH));
    /* Auto width: the text and 4 px each side, h tall; only the chip's colour, white and the shadow; nothing else touched. */
    clear(SENTINEL);
    DevChip(10, 10, 0, 12, "FIRE", bg, sh);
    assert(at(10, 10) == bg && at(10 + tw + 7, 21) == bg && at(10 + tw + 8, 10) == SENTINEL && at(10, 22) == SENTINEL);
    for (int x = 10; x < 18 + tw; ++x)
        for (int y = 10; y < 22; ++y) {
            white += at(x, y) == DEV_WHITE;
            shade += at(x, y) == sh;
            assert(at(x, y) == bg || at(x, y) == DEV_WHITE || at(x, y) == sh);
        }
    assert(white > 0 && shade > 0);
    Untouched(10, 10, tw + 8, 12);
    /* Fixed width (BMoves' 40 px type chip): the white text inside the centred text box. */
    clear(SENTINEL);
    DevChip(10, 30, 40, 12, "FIRE", bg, sh);
    assert(at(49, 30) == bg && at(50, 30) == SENTINEL);
    for (int x = 10; x < 50; ++x)
        for (int y = 30; y < 42; ++y)
            if (at(x, y) == DEV_WHITE) {
                left = x < left ? x : left;
                right = x > right ? x : right;
            }
    assert(left >= 10 + (40 - tw) / 2 && left < 10 + (40 - tw) / 2 + 3 && right < 10 + (40 - tw) / 2 + tw);
    Untouched(10, 30, 40, 12);
    /* A fixed-width chip too narrow for its word clips it and counts it as a cut label; one wide enough does not. */
    sDevKeyCut = 0;
    DevChip(10, 30, 40, 12, "FIRE", bg, sh);
    assert(sDevKeyCut == 0);
    DevChip(10, 30, tw - 1, 12, "FIRE", bg, sh);
    assert(sDevKeyCut == 1);
    sDevKeyCut = 0;
    /* No shadow (PSN, FNT): only the chip's colour and white. */
    clear(SENTINEL);
    DevChip(10, 50, 0, 10, "PSN", DEV_PSN, DEV_WHITE);
    assert(Count(10, 50, pw, 10, DEV_WHITE) > 0 && Count(10, 50, pw, 10, DEV_PSN) + Count(10, 50, pw, 10, DEV_WHITE) == pw * 10);
    Untouched(10, 50, pw, 10);
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
    assert(at(238, 121) == DevBlend(DevBlend(DEV_SHELL_RED, 0, 72), 0, 90) && at(238, 123) == at(238, 150)
           && at(238, 170) == DevBlend(DEV_SHELL_RED, 0xFFFF, 51) && at(238, 171) == DEV_SHELL_RED);   /* top rows; light row */
    DevRow(&(CtrPokeTouchRect){16, 48, 244, 16}, 1); assert(at(20, 56) == DEV_BEZEL && DevRowInk(1) == DEV_PAPER && DevRowInk(0) == DEV_INK);
    DevHpBar(76, 52, 150, 6, 112, 112); assert(at(76, 52) == DEV_HP_FRAME && at(100, 54) == DEV_HP_GREEN);
    KitBarLevels();
    assert(DevChosen(2, 2, -1) && !DevChosen(1, 2, -1) && DevChosen(1, 2, 1) && !DevChosen(2, 2, 1));
    KitChips();
    clear(0); DevRound(0, 0, 20, 20, 6, 0x1234); assert(at(0, 0) == 0 && at(1, 1) == 0 && at(2, 2) == 0x1234 && at(6, 0) == 0x1234 && at(0, 6) == 0x1234);
    KitWrapLines();
    sDevKeyCut = 0; DevKeyLabel(&(CtrPokeTouchRect){0, 0, 30, 24}, 0, DEV_LOOK_KEY, 0, "A VERY LONG LABEL"); assert(sDevKeyCut == 1); sDevKeyCut = 0;
}

/* DevUpper: the boards' and FireRed's capitals; POKé's é stays small; a letter the font has no capital for stays. */
static void KitUpper(void)
{
    char out[40];

    DevUpper(out, "Pok\xC3\xA9mon", sizeof(out));
    assert(!strcmp(out, "POK\xC3\xA9MON"));
    DevUpper(out, "Pok\xC3\xA9" "dex", sizeof(out));
    assert(!strcmp(out, "POK\xC3\xA9" "DEX"));
    DevUpper(out, "Pok\xC3\xA9 Ball", sizeof(out));
    assert(!strcmp(out, "POK\xC3\xA9 BALL"));
    DevUpper(out, "Zur\xC3\xBC" "ck", sizeof(out));                    /* German: u umlaut */
    assert(!strcmp(out, "ZUR\xC3\x9C" "CK"));
    DevUpper(out, "Regi\xC3\xB3n, Ning\xC3\xBAn, Espa\xC3\xB1" "a", sizeof(out));   /* Spanish: o, u acute, n tilde */
    assert(!strcmp(out, "REGI\xC3\x93N, NING\xC3\x9AN, ESPA\xC3\x91" "A"));
    DevUpper(out, "Caf\xC3\xA9 Stra\xC3\x9F" "e \xC5\x93uvre", sizeof(out));   /* an e acute away from POK; sharp s; oe */
    assert(!strcmp(out, "CAF\xC3\x89 STRA\xC3\x9F" "E \xC5\x92UVRE"));
    DevUpper(out, "POK\xC3\xA9MON 2x \x01\xd5", sizeof(out));            /* idempotent; a raw FireRed code kept */
    assert(!strcmp(out, "POK\xC3\xA9MON 2X \x01\xd5"));
    {
        const char *e = "\xC3\x89", *u = "\xC3\x9C";                     /* the capitals made are the font's */

        assert(GlyphCode(&e) == 0x06 && GlyphCode(&u) == 0xf3);
    }
    /* Recipe 0251's title case leaves the accented small letters as FireRed's own codes, escaped by 0x01 (the bridges'
     * CtrBattleText): each becomes its capital's code where the charset has one; POKé's é stays small; a capital, or
     * a code with no capital, is kept. */
    DevUpper(out, "Zur\x01\xf6" "ck", sizeof(out));                       /* ü -> Ü */
    assert(!strcmp(out, "ZUR\x01\xf3" "CK"));
    DevUpper(out, "K\x01\xf4" "fer, Br\x01\xf5" "tchen", sizeof(out));     /* ä -> Ä, ö -> Ö */
    assert(!strcmp(out, "K\x01\xf1" "FER, BR\x01\xf2" "TCHEN"));
    DevUpper(out, "Regi\x01\x23" "n, Espa\x01\x29" "a, Ra\x01\x6f" "z", sizeof(out));   /* ó, ñ, í */
    assert(!strcmp(out, "REGI\x01\x0e" "N, ESPA\x01\x14" "A, RA\x01\x5a" "Z"));
    DevUpper(out, "Pok\x01\x1b" "mon, Caf\x01\x1b, \x01\x25" "uf", sizeof(out));       /* POKé stays; é, œ */
    assert(!strcmp(out, "POK\x01\x1b" "MON, CAF\x01\x06, \x01\x10" "UF"));
    DevUpper(out, "\x01\xf3\x01\x15\x01\x2a", sizeof(out));               /* Ü, ß, º kept */
    assert(!strcmp(out, "\x01\xf3\x01\x15\x01\x2a"));
    DevUpper(out, "AB\xC3\xA9", 4);                                       /* a letter that does not fit whole is left out */
    assert(!strcmp(out, "AB"));
    DevUpper(out, "Pok\xC3\xA9mon", 1);
    assert(!out[0]);
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
    DevChip(240, p.y + 38, 40, 12, "FIRE", UiTypeColor(10), DevChipShadow(UiTypeColor(10)));
    DevChip(240, p.y + 52, 0, 12, "SPECIAL", DEV_CAT_SPECIAL, DEV_CAT_SPECIAL_SH);
    DevChip(240, p.y + 66, 0, 12, "SUPER", DEV_FX_SUPER, DEV_FX_SUPER_SH);
    DevChip(282, p.y + 39, 0, 10, "PSN", DEV_PSN, DEV_WHITE);
    DevChip(282, p.y + 67, 0, 10, "FNT", DEV_FNT, DEV_WHITE);
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
    KitUpper();
    KitSheet();
    assert(sDevKeyCut == 0);
}

/* ---- home: HOME as the red device (board B · Início) --------------------------------------------------------------- */

static int HomeHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_HomeHit((const CtrPokeTouchView *)view, x, y);
}

/* The board's HOME: VERMILION CITY in KANTO (the map's region, under the name), the player's name on the card, five
 * Pokemon and a hole (HP 100, 66, 34, 0 and 8 %), the speed at 2x, the map's art there, another map open; no player on
 * the map, no destination. Strings as FireRed has them: no casing (the B views are upper case). */
static CtrPokeTouchView HomeBoard(void)
{
    static const uint8_t kHp[6] = {100, 66, 34, 0, 8, 0};
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.optPressed = -1;
    v.home = 1;
    for (int i = 0; i < PT_COUNT; ++i)
        v.enabled[i] = 1;
    v.mapGroup = PT_MAP_KANTO;
    v.mapReady = 1;
    v.mapOpen = 3;
    v.playerX = v.playerY = v.cursorX = v.cursorY = -1;
    strcpy(v.bar[0], "VERMILION CITY");
    strcpy(v.playerName, "RED");
    for (int i = 0; i < 6; ++i) {
        v.party[i] = (uint8_t)(i == 5 ? 0 : kHp[i] ? 1 : 2);
        v.partyHp[i] = kHp[i];
        v.partyIcon[i] = (uint16_t)(i == 5 ? 0 : 25 + i);
    }
    v.speed = 2;
    v.speedEnabled = 1;
    return v;
}

/* Two rectangles share a pixel. */
static int Meets(CtrPokeTouchRect a, CtrPokeTouchRect b)
{
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

/* Words arrive in any case and HOME draws them as FireRed writes them. With small letters made wider than capitals:
 * HOME with its game strings and reasons in proper case draws what it draws with them in capitals; the region key is
 * as wide as its name in capitals; each key's word is the entry's in capitals, the card's the player's name as typed. */
static void HomeUpperCase(CtrPokeTouchView v)
{
    static uint16_t capitals[W * H];
    static uint8_t keepWidths[256], keepSmall[256];
    CtrPokeTouchView proper;
    char word[24];

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    v.cursorX = 9;
    v.cursorY = 9;
    strcpy(v.playerName, "Rick");
    proper = v;
    strcpy(v.bar[0], "VERMILION CITY");
    strcpy(v.bar[2], "NOT VISITED YET");
    strcpy(proper.bar[0], "Vermilion City");
    strcpy(proper.bar[2], "Not visited yet");
    CtrPokeTouch_Draw(canvas, &v);
    memcpy(capitals, canvas, sizeof(capitals));
    CtrPokeTouch_Draw(canvas, &proper);
    assert(!memcmp(capitals, canvas, sizeof(capitals)));
    assert(sHomeRibbon.w == DevTextWidth("KANTO") + 23 && DevTextWidth("KANTO") != DevTextWidth("Kanto"));
    assert(!strcmp(DevHomeKeyWord(PT_POKEMON, &v, word), "POK\xC3\xA9MON") && !strcmp(DevHomeKeyWord(PT_CARD, &v, word), "Rick"));
    /* the five entries' keys say DevHomeWord's words, in capitals: the label-fit builds' sDevKeyCut == 0 is their fit */
    for (int id = PT_POKEDEX; id <= PT_OPTIONS; ++id) {
        char name[16], up[24];

        DevUpper(up, DevHomeWord(id, &v, name), sizeof(up));
        assert(id == PT_CARD || (up[0] && !strcmp(DevHomeKeyWord(id, &v, word), up)));
    }
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* The suite as the plan gives it (HOME's keys hit where they are drawn, the map at 1:1, every cell of every map in the
 * window, the name window inside it, a blink or a step presents only the window), the review's checks (upper case, the
 * name window clear of the region key and FLY, the Fly reasons under a long name, an egg), then the states for the eye. */
static void SuiteHome(void)
{
    static const int ids[6] = {PT_POKEDEX, PT_POKEMON, PT_BAG, PT_CARD, PT_SAVE, PT_OPTIONS};
    static const int corner[5][2] = {{0, 0}, {21, 0}, {0, 14}, {21, 14}, {10, 7}};
    static uint16_t tm[30 * 20], btm[30 * 20], gba[MAP_COLORS];
    static uint8_t tl[MAP_TILES * 32], btl[MAP_TILES * 32];
    CtrPokeTouchView v, old;
    CtrPokeTouchRect r, keyArea = DevKeyArea(&kDevHomeMapKey), flyArea = DevKeyArea(&kDevHomeFly);
    CtrDirtyList d;
    int i, x, y, gx, gy, cx, cy, vx, vy, dx, dy, g;
    const int flyX = kDevHomeFly.x + kDevHomeFly.w / 2, flyY = kDevHomeFly.y + kDevHomeFly.h / 2;

    richArt = 1;
    iconArt = 1;
    sPlayer.key = -1;
    assert(CtrPokeTouch_MapPrepare(PT_MAP_KANTO, 0));
    v = HomeBoard();
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    shot("plain");
    for (i = 0; i < 6; ++i) {
        AssertHits(HomeHitV, &v, kDevHomeKey[i], ids[i]);
        AssertKey(kDevHomeKey[i], DEV_LOOK_KEY, 0);
    }
    assert(CtrPokeTouch_HomeHit(&v, 108, 174) == PT_NONE && at(108, 174) == DEV_SHELL_RED);      /* the gap */
    assert(CtrPokeTouch_HomeHit(&v, 232, 131) == PT_RUN && CtrPokeTouch_HomeHit(&v, 280, 131) == PT_HOME_SPEED);
    assert(CtrPokeTouch_HomeHit(&v, 225, 75) == PT_HOME_PARTY && CtrPokeTouch_HomeHit(&v, 256, 103) == PT_HOME_PARTY + 4
           && CtrPokeTouch_HomeHit(&v, 287, 103) == PT_NONE && CtrPokeTouch_HomeHit(&v, 100, 80) == PT_NONE);
    assert(CtrPokeTouch_HomeWindowPoint(14, 14, &vx, &vy) && vx == 0 && vy == 0 && CtrPokeTouch_HomeWindowPoint(205, 141, &vx, &vy)
           && vx == 191 && vy == 127 && !CtrPokeTouch_HomeWindowPoint(206, 80, &vx, &vy) && !CtrPokeTouch_HomeWindowPoint(13, 80, &vx, &vy));
    /* RUN's lamp as the board draws it: an 8x8 at 4 px in, its 6x6 face */
    assert(at(241, 127) == DEV_HOME_LAMP_OFF_EDGE && at(248, 134) == DEV_HOME_LAMP_OFF_EDGE && at(242, 128) == DEV_HOME_LAMP_OFF
           && at(247, 133) == DEV_HOME_LAMP_OFF && at(249, 130) == DEV_WHITE && at(240, 130) == DEV_WHITE);
    /* the map at 1:1: window pixel (dx, dy) is the Region Map screen's (24 + dx, 28 + dy) (below the region key's rows) */
    CtrPokeTouch_MapArt(0, tm, tl, gba, btm, btl);
    for (dy = 20; dy < 128; dy += 9)
        for (dx = 0; dx < 192; dx += 7)
            assert(at(14 + dx, 14 + dy) == Gba(gba[MapColor(tm, tl, btm, btl, 24 + dx, 28 + dy)]));
    /* every cell of every region inside the window with room for its 16x16 icon; a tap on it gives it back; the name
     * window of a destination there stays in the window and clear of the region key and FLY (their widest, with drops) */
    strcpy(v.bar[0], "VERMILION CITY");
    for (g = 0; g < PT_MAP_COUNT; ++g) {
        v.mapGroup = (uint8_t)g;
        DevCheckTouch(&old, &v);
        for (cy = 0; cy < 15; ++cy)
            for (cx = 0; cx < 22; ++cx) {
                CtrPokeTouchView pick = v;

                assert(CtrPokeTouch_HomeCellPoint(&v, cx, cy, &x, &y) && x - 8 >= 14 && y - 8 >= 14 && x + 8 <= 206 && y + 8 <= 142);
                assert(CtrPokeTouch_HomeMapCell(&v, x, y, &gx, &gy) && gx == cx && gy == cy);
                pick.cursorX = (int8_t)cx;
                pick.cursorY = (int8_t)cy;
                assert(DevHomeLabelRect(&pick, &r) && r.x >= 14 && r.y >= 14 && r.x + r.w <= 206 && r.y + r.h <= 142);
                assert(!Meets(r, keyArea) && !Meets(r, flyArea));
            }
    }
    assert(CtrPokeTouch_HomeCellPoint(&v, 0, 0, &x, &y) && x == 26 && y == 22 && !CtrPokeTouch_HomeCellPoint(&v, 22, 0, &x, &y));
    v.mapGroup = 0;
    /* the destination's name window stays inside the window, at the corners and the middle */
    for (i = 0; i < 5; ++i) {
        v.cursorX = (int8_t)corner[i][0];
        v.cursorY = (int8_t)corner[i][1];
        DevCheckTouch(&old, &v);
        assert(DevHomeLabelRect(&v, &r) && r.x >= 14 && r.y >= 14 && r.x + r.w <= 206 && r.y + r.h <= 142);
    }
    /* a blink or a step: the window alone */
    v.cursorFrame ^= 1;
    d = DevCheckTouch(&old, &v);
    assert(d.count == 1 && d.rects[0].x0 == 14 && d.rects[0].y0 == 14 && d.rects[0].x1 == 206 && d.rects[0].y1 == 142);
    v.playerX = 21;
    v.playerY = 14;
    d = DevCheckTouch(&old, &v);
    assert(d.count == 1 && d.rects[0].x1 == 206 && d.rects[0].y1 == 142);
    v.pressed = PT_POKEMON;
    assert(DevWhole(DevCheckTouch(&old, &v)));
    AssertKey(kDevHomeKey[1], DEV_LOOK_KEY, 1);
    assert(at(198, 164) == DEV_LED);
    v.pressed = PT_NONE;
    v.enabled[PT_SAVE] = 0;
    DevCheckTouch(&old, &v);
    assert(at(115, 215) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    v.flyOk = 1;
    DevCheckTouch(&old, &v);
    assert(CtrPokeTouch_FlyHit(flyX, flyY));
    v.flyOk = 0;
    DevCheckTouch(&old, &v);
    assert(!CtrPokeTouch_FlyHit(flyX, flyY));
    assert(CtrPokeTouch_HomeRibbonHit(20, 20));
    v.mapOpen = 1;
    DevCheckTouch(&old, &v);
    assert(!CtrPokeTouch_HomeRibbonHit(20, 20));
    v.shell = 2;
    assert(DevWhole(DevCheckTouch(&old, &v)) && at(160, 152) == DEV_SHELL_GREEN);
    /* nothing HOME draws changed: nothing presented */
    v.optPressed = 3;
    v.backVisible = 1;
    d = DevCheckTouch(&old, &v);
    assert(d.count == 0);

    /* Upper case, whatever case the words arrive in. */
    HomeUpperCase(HomeBoard());
    v = HomeBoard();
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    /* Why a place cannot be flown to, under a name of two lines: one line, every reason whole; a reason too long for it
     * counts as cut. */
    strcpy(v.bar[0], "WATER LABYRINTH PATH");
    v.cursorX = 9;
    v.cursorY = 9;
    for (i = UIS_FLY_UNVISITED; i <= UIS_FLY_BUSY; ++i) {
        snprintf(v.bar[2], PT_BAR_LEN, "%s", CtrUiText((CtrUiString)i));
        DevCheckTouch(&old, &v);
        assert(Count(212, 33, 90, 8, DEV_INK) > 0 && Count(212, 46, 90, 7, DEV_INK2) > 0);   /* the name's 2nd line; the reason */
    }
    assert(sDevKeyCut == 0);
    strcpy(v.bar[2], "A REASON FAR TOO LONG FOR ONE LINE");
    DevCheckTouch(&old, &v);
    assert(sDevKeyCut > 0);
    sDevKeyCut = 0;
    /* An egg: its icon, no HP bar (FireRed shows none). */
    v = HomeBoard();
    v.party[5] = 1;
    v.partyHp[5] = 100;
    v.partyIcon[5] = 412;
    v.partyEgg = 1u << 5;
    DevCheckTouch(&old, &v);
    assert(at(245, 112) == DEV_HP_FRAME && at(276, 112) == DEV_PAPER && at(287, 112) == DEV_PAPER);
    assert(CtrPokeTouch_HomeHit(&v, 287, 103) == PT_HOME_PARTY + 5);

    /* The states for the eye, each through Update first. pressed: POKeMON held (the board's). */
    v = HomeBoard();
    v.playerX = 13;
    v.playerY = 9;
    DevCheckTouch(&old, &v);
    v.pressed = PT_POKEMON;
    DevCheckTouch(&old, &v);
    shot("pressed");
    /* selection: a town picked that can be flown to (its cursor and name; FLY), RUN on, SPD held */
    v.pressed = PT_HOME_SPEED;
    v.running = 1;
    v.cursorX = 14;
    v.cursorY = 6;
    v.flyOk = 1;
    DevCheckTouch(&old, &v);
    shot("selection");
    /* flypick: FLY chosen in the Party, the map waits for a town; the hint in two lines, where to tap and the way out
     * (v0.6.0's "Any button: cancel"), each line whole; a Pokemon held */
    v.pressed = PT_HOME_PARTY + 2;
    v.running = 0;
    v.cursorX = v.cursorY = -1;
    v.flyOk = 0;
    strcpy(v.bar[0], "FLY");
    snprintf(v.bar[2], PT_BAR_LEN, "%s", CtrUiText(UIS_FLY_PICK));
    DevCheckTouch(&old, &v);
    assert(Count(212, 33, 90, 7, DEV_INK2) > 0 && Count(212, 46, 90, 7, DEV_INK2) > 0);   /* the hint takes two lines */
    {
        const char *pick = CtrUiText(UIS_FLY_PICK), *out = strchr(pick, '\n');
        char first[PT_BAR_LEN], second[PT_BAR_LEN];
        int w1, w2;

        assert(out && !strchr(out + 1, '\n') && (size_t)(out - pick) < sizeof(first));
        memcpy(first, pick, (size_t)(out - pick));
        first[out - pick] = 0;
        DevUpper(second, first, sizeof(second));
        w1 = DevSmallTextWidth(second);
        DevUpper(second, out + 1, sizeof(second));
        w2 = DevSmallTextWidth(second);
        /* each line ends where its own words do: the break is the string's, not the width's */
        assert(w1 <= 90 && w2 <= 90 && Count(212 + w1 - 4, 33, 4, 7, DEV_INK2) > 0 && Count(212 + w1, 33, 90 - w1, 7, DEV_INK2) == 0);
        assert(Count(212 + w2 - 4, 46, 4, 7, DEV_INK2) > 0 && Count(212 + w2, 46, 90 - w2, 7, DEV_INK2) == 0);
    }
    shot("flypick");
    /* sevii: another map; a place that cannot be flown to says why */
    assert(CtrPokeTouch_MapPrepare(PT_MAP_SEVII123, 0));
    v.pressed = PT_NONE;
    v.mapGroup = PT_MAP_SEVII123;
    v.mapOpen = 3;
    v.playerX = v.playerY = -1;
    v.cursorX = 9;
    v.cursorY = 9;
    strcpy(v.bar[0], "TWO ISLAND");
    snprintf(v.bar[2], PT_BAR_LEN, "%s", CtrUiText(UIS_FLY_UNVISITED));
    DevCheckTouch(&old, &v);
    shot("sevii");
    /* fainted: two down, one with a status, an egg (no bar); SPD and RUN not usable now, SAVE neither; the green shell */
    assert(CtrPokeTouch_MapPrepare(PT_MAP_KANTO, 0));
    v = HomeBoard();
    v.playerX = 13;
    v.playerY = 9;
    v.party[0] = 2;
    v.partyHp[0] = 0;
    v.party[2] = 2;
    v.partyHp[2] = 0;
    v.party[4] = 3;
    v.party[5] = 1;
    v.partyHp[5] = 100;
    v.partyIcon[5] = 412;
    v.partyEgg = 1u << 5;
    v.speed = 1;
    v.speedEnabled = 0;
    v.enabled[PT_RUN] = 0;
    v.enabled[PT_SAVE] = 0;
    v.shell = 2;
    DevCheckTouch(&old, &v);
    shot("fainted");
    richArt = 0;
    iconArt = 0;
    sPlayer.key = -1;
    assert(sDevKeyCut == 0);
}

/* ---- battle: the commands, the quick ball, the moves, the targets, Safari, idle (boards B · Comandos, B · Golpes) ---- */

static int BatHitV(const void *view, int x, int y)
{
    return CtrBattleUi_Hit((const CtrBattleView *)view, x, y);
}

/* The boards' battle: CHARMELEON (male) Lv36 at 112/112 in the lead; the balls able, able, a status, fainted, able, none;
 * its moves FLAMETHROWER (FIRE, special, super effective, 15/15, with its description), SCRATCH (NORMAL, effective,
 * 15/35), GROWL (NORMAL, a status move: no power, no chip, 40/40) and BRICK BREAK (FIGHT, physical, not very effective,
 * 3/15); a double battle's targets: two foes and the ally. The quick ball's POKe BALL x21 among four kinds. Strings as
 * FireRed has them: no casing (the B views are upper case). */
static CtrBattleView BatBoard(void)
{
    static const struct {
        const char *name, *type, *power;
        int t, pp, max, fx;
    } kMove[4] = {
        {"FLAMETHROWER", "FIRE", "95", 10, 15, 15, CTR_FX_DOUBLE}, {"SCRATCH", "NORMAL", "40", 0, 15, 35, CTR_FX_NEUTRAL},
        {"GROWL", "NORMAL", "-", 0, 40, 40, CTR_FX_NONE}, {"BRICK BREAK", "FIGHT", "75", 1, 3, 15, CTR_FX_HALF},
    };
    static const struct {
        const char *name;
        int side, level, fx, hp, max;
    } kTarget[3] = {{"PIDGEOTTO", 0, 34, CTR_FX_NEUTRAL, 61, 98}, {"RATICATE", 0, 31, CTR_FX_DOUBLE, 80, 80},
                    {"PIKACHU", 1, 30, CTR_FX_NONE, 54, 80}};
    static const uint8_t kParty[6] = {1, 1, 3, 2, 1, 0}, kHp[6] = {100, 70, 52, 0, 90, 0};
    CtrBattleView v;

    memset(&v, 0, sizeof(v));
    v.pressed = -1;
    v.cursor = -1;
    memcpy(v.party, kParty, sizeof(kParty));
    memcpy(v.partyHp, kHp, sizeof(kHp));
    v.partyStatus[2] = 2;
    strcpy(v.activeName, "CHARMELEON");
    v.activeNameIsSpecies = 1;
    v.activeLevel = 36;
    v.activeGender = 0;
    v.activeSlot = 0;
    v.activeIcon = 5;
    v.activeHp = v.activeMaxHp = 112;
    v.activeType[0] = 10;
    v.activeType[1] = -1;
    strcpy(v.activeTypeName[0], "FIRE");
    v.ballItem = 4;
    v.ballQty = 21;
    v.ballPlace = 0;
    strcpy(v.ballName, "POK\xC3\xA9 BALL");
    for (int i = 0; i < 4; ++i) {
        CtrBattleMove *m = &v.move[i];

        m->present = 1;
        m->type = (uint8_t)kMove[i].t;
        strcpy(m->name, kMove[i].name);
        strcpy(m->typeName, kMove[i].type);
        strcpy(m->power, kMove[i].power);
        strcpy(m->accuracy, "100");
        m->pp = (uint8_t)kMove[i].pp;
        m->maxPp = (uint8_t)kMove[i].max;
        m->matchup = (uint8_t)kMove[i].fx;
    }
    v.move[0].stab = 1;
    v.move[0].effectChance = 10;
    strcpy(v.move[0].desc, "A powerful fire attack that may inflict a burn.");
    for (int i = 0; i < 3; ++i) {
        CtrBattleTarget *t = &v.target[i];

        t->present = 1;
        t->battler = (int8_t)(i == 2 ? 2 : 2 * i + 1);
        t->side = (uint8_t)kTarget[i].side;
        t->level = (uint8_t)kTarget[i].level;
        t->matchup = (uint8_t)kTarget[i].fx;
        t->hp = (uint16_t)kTarget[i].hp;
        t->maxHp = (uint16_t)kTarget[i].max;
        strcpy(t->name, kTarget[i].name);
        t->nameIsSpecies = 1;
    }
    return v;
}

/* The move screen and the keys draw the game's words in capitals, whatever case they arrive in: with small letters made
 * wider than capitals, the moves in proper case draw what they draw in capitals. */
static void BatUpperCase(CtrBattleView v)
{
    static uint16_t capitals[W * H];
    static uint8_t keepWidths[256], keepSmall[256];
    static const char *const kProper[4][2] = {{"Flamethrower", "Fire"}, {"Scratch", "Normal"}, {"Growl", "Normal"},
                                              {"Brick Break", "Fight"}};
    CtrBattleView proper = v;

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    for (int i = 0; i < 4; ++i) {
        strcpy(proper.move[i].name, kProper[i][0]);
        strcpy(proper.move[i].typeName, kProper[i][1]);
    }
    v.state = proper.state = BUI_MOVE;
    v.cursor = proper.cursor = 0;
    CtrBattleUi_Draw(canvas, &v);
    memcpy(capitals, canvas, sizeof(capitals));
    CtrBattleUi_Draw(canvas, &proper);
    assert(!memcmp(capitals, canvas, sizeof(capitals)));
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* A Pokemon's name: its species' own in capitals whatever case it arrives in (recipe 0251 title-cases the species
 * names: "Charmeleon" shows as CHARMELEON), one its player gave as typed ("Sparky"), on the action screen (the lead and
 * the question) and on the target keys. Small letters are made wider than capitals, so the case shows in the pixels. */
static void BatNames(CtrBattleView v)
{
    static uint16_t first[W * H];
    static uint8_t keepWidths[256], keepSmall[256];
    CtrBattleView other;

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    v.state = BUI_ACTION;
    other = v;
    strcpy(v.activeName, "CHARMELEON");
    strcpy(other.activeName, "Charmeleon");
    v.activeNameIsSpecies = other.activeNameIsSpecies = 1;
    CtrBattleUi_Draw(canvas, &v);
    memcpy(first, canvas, sizeof(first));
    CtrBattleUi_Draw(canvas, &other);
    assert(!memcmp(first, canvas, sizeof(first)));                 /* the species' own: CHARMELEON */
    strcpy(v.activeName, "SPARKY");
    strcpy(other.activeName, "Sparky");
    v.activeNameIsSpecies = other.activeNameIsSpecies = 0;
    CtrBattleUi_Draw(canvas, &v);
    memcpy(first, canvas, sizeof(first));
    CtrBattleUi_Draw(canvas, &other);
    assert(memcmp(first, canvas, sizeof(first)));                  /* a nickname: Sparky, as typed */
    assert(Count(76, 17, 140, 12, DEV_INK) > 0 && Count(20, 71, 280, 15, DEV_INK) > 0);
    v.state = other.state = BUI_TARGET;
    v.cursor = other.cursor = -1;
    strcpy(v.target[0].name, "PIDGEOTTO");
    strcpy(other.target[0].name, "Pidgeotto");
    strcpy(v.target[2].name, "PIKACHU");
    strcpy(other.target[2].name, "Pikachu");
    v.target[0].nameIsSpecies = other.target[0].nameIsSpecies = v.target[2].nameIsSpecies = other.target[2].nameIsSpecies = 1;
    CtrBattleUi_Draw(canvas, &v);
    memcpy(first, canvas, sizeof(first));
    CtrBattleUi_Draw(canvas, &other);
    assert(!memcmp(first, canvas, sizeof(first)));
    strcpy(v.target[2].name, "SPARKY");
    strcpy(other.target[2].name, "Sparky");
    v.target[2].nameIsSpecies = other.target[2].nameIsSpecies = 0;
    CtrBattleUi_Draw(canvas, &v);
    memcpy(first, canvas, sizeof(first));
    CtrBattleUi_Draw(canvas, &other);
    assert(memcmp(first, canvas, sizeof(first)));
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* The plan's suite (each key hit where it is drawn, the touch or else the cursor sinks one key, the quick ball's arrows
 * and its grey states, RUN when there is no running, holes for missing moves, nothing hit while idle), the boards'
 * pieces where the boards put them, then the states for the eye. */
static void SuiteBattle(void)
{
    CtrBattleView v = BatBoard(), old;
    CtrPokeTouchRect prev, next;
    int i, x, y, hpW, fightX;

    old = v;
    CtrBattleUi_Draw(canvas, &old);
    v.state = BUI_ACTION; v.cursor = 0; DevCheckBattle(&old, &v);            /* a trainer: three keys */
    AssertHits(BatHitV, &v, kDevBatFight, BUI_ACT_FIGHT); AssertKey(kDevBatFight, DEV_LOOK_KEY, 1); assert(at(302, 114) == DEV_LED);
    AssertHits(BatHitV, &v, kDevBatRow3[0], BUI_ACT_BAG); AssertHits(BatHitV, &v, kDevBatRow3[1], BUI_ACT_POKEMON); AssertHits(BatHitV, &v, kDevBatRow3[2], BUI_ACT_RUN);
    assert(CtrBattleUi_Hit(&v, 108, 200) == -1 && CtrBattleUi_Hit(&v, 160, 50) == -1);
    for (i = 0; i < 3; ++i)
        AssertKey(kDevBatRow3[i], DEV_LOOK_KEY, 0);
    /* the screen: the lead's icon at 2x (its frame 0 from (4, 8)), the balls, HP, the prompt */
    assert(BPartyIcon(0, v.activeIcon) && at(20, 18) == sBPIcon[0].pal[sBPIcon[0].pixels[8 * 32 + 4]] && at(21, 19) == at(20, 18));
    assert(at(40, 30) == DEV_PAPER);                                          /* the stub icon's hole */
    assert(at(232, 24) == UI_RGB(0x62, 0x62, 0x62) && at(244, 24) == at(232, 24) && at(256, 24) == UI_RGB(0xE0, 0xB0, 0x20)
           && at(268, 24) == UI_RGB(0xC8, 0xC8, 0xC0) && at(280, 24) == at(232, 24));
    assert(at(292, 20) == UI_RGB(0xB0, 0xB0, 0xA8) && at(292, 24) == DEV_PAPER);   /* none: a ring */
    hpW = DevSmallTextWidth("HP") + 2;
    assert(at(76, 52) == DEV_HP_FRAME && at(76 + hpW + 3, 53) == DEV_HP_FRAME && at(76 + hpW + 3 + 75, 55) == DEV_HP_GREEN
           && at(76 + hpW + 3 + 149, 58) == DEV_HP_FRAME && at(76 + hpW + 3 + 150, 55) == DEV_PAPER);
    assert(Count(20, 71, 280, 15, DEV_INK) > 0 && Count(76, 17, 140, 12, DEV_INK) > 0);   /* the prompt; the name */
    assert(Count(76, 33, 8, 10, DEV_MALE) > 0);                                           /* the gender mark */
    /* FIGHT's crossed swords: 24x24, 10 px left of the word, the pair centred on the key (sunk: face at 106); blended
     * at their edges, nothing between them and the word */
    fightX = 8 + (304 - (24 + 10 + UiTextWidth(CTR_TR("FIGHT", "LUCHAR", "KAMPF"), 2))) / 2;
    assert(Count(fightX, 122, 24, 24, UI_RGB(0xA0, 0x2A, 0x20)) > 20 && Count(fightX + 24, 106, 10, 56, UI_RGB(0xA0, 0x2A, 0x20)) == 0);
    assert(Count(fightX, 122, 24, 24, DEV_SUNK) + Count(fightX, 122, 24, 24, UI_RGB(0xA0, 0x2A, 0x20)) < 24 * 24);
    assert(Count(fightX + 34, 125, 40, 18, UI_RGB(0xA0, 0x2A, 0x20)) > 0 && Count(8, 106, fightX - 8, 56, UI_RGB(0xA0, 0x2A, 0x20)) == 0);
    assert(at(fightX + 5, 127) == UI_RGB(0xA0, 0x2A, 0x20) && at(fightX + 18, 127) == UI_RGB(0xA0, 0x2A, 0x20));   /* the blades */
    shot("action");
    v.runBlocked = 1; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 264, 200) == -1 && at(219, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    shot("action_runblocked");
    v.runBlocked = 0; v.ball = CTR_QUICK_BALL_READY; v.ballCount = 4; v.ballItem = 4; v.ballQty = 21; DevCheckBattle(&old, &v);   /* wild: four keys */
    assert(CtrBattleUi_Hit(&v, 121, 180) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, 97, 218) == BUI_ACT_BALL_PREV && CtrBattleUi_Hit(&v, 151, 218) == BUI_ACT_BALL_NEXT
           && CtrBattleUi_Hit(&v, 44, 200) == BUI_ACT_BAG && CtrBattleUi_Hit(&v, 198, 200) == BUI_ACT_POKEMON && CtrBattleUi_Hit(&v, 275, 200) == BUI_ACT_RUN);
    AssertHits(BatHitV, &v, kDevBatRow4[0], BUI_ACT_BAG); AssertHits(BatHitV, &v, kDevBatRow4[1], BUI_ACT_BALL);
    AssertHits(BatHitV, &v, kDevBatRow4[2], BUI_ACT_POKEMON); AssertHits(BatHitV, &v, kDevBatRow4[3], BUI_ACT_RUN);
    prev = DevBatBallArrow(&kDevBatRow4[1], 0);
    next = DevBatBallArrow(&kDevBatRow4[1], 1);
    /* an arrow answers 3 px round itself, on its key */
    assert(CtrBattleUi_Hit(&v, prev.x - 2, prev.y - 3) == BUI_ACT_BALL_PREV && CtrBattleUi_Hit(&v, next.x + next.w + 1, next.y + next.h + 2) == BUI_ACT_BALL_NEXT);
    assert(CtrBattleUi_Hit(&v, prev.x + prev.w + 3, prev.y) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, prev.x, prev.y - 4) == BUI_ACT_BALL);
    assert(Count(prev.x, prev.y, prev.w, prev.h, DEV_KEY_INK) > 0 && Count(next.x, next.y, next.w, next.h, DEV_KEY_INK) > 0);
    shot("action_ball");
    v.ballCount = 1; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 97, 218) == BUI_ACT_BALL);
    assert(Count(prev.x, prev.y, prev.w, prev.h, DEV_KEY_INK) == 0 && Count(next.x, next.y, next.w, next.h, DEV_KEY_INK) == 0);
    v.ball = CTR_QUICK_BALL_FULL; v.ballCount = 4; DevCheckBattle(&old, &v);       /* grey: dimmed, the throw still hit (the game says why), no arrows */
    assert(CtrBattleUi_Hit(&v, 121, 180) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, 97, 218) == BUI_ACT_BALL && at(88, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    shot("action_ballfull");
    v.ball = CTR_QUICK_BALL_EMPTY; v.ballCount = 0; v.ballItem = 0; v.ballQty = 0; DevCheckBattle(&old, &v);
    assert(CtrBattleUi_Hit(&v, 121, 180) == BUI_ACT_BALL && at(88, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    v.ball = CTR_QUICK_BALL_AREA; v.ballItem = 4; v.ballQty = 21; DevCheckBattle(&old, &v);
    assert(CtrBattleUi_Hit(&v, 151, 218) == BUI_ACT_BALL);
    v.ball = CTR_QUICK_BALL_READY; v.ballCount = 4; v.pressed = BUI_ACT_BALL_NEXT; DevCheckBattle(&old, &v);
    AssertKey(kDevBatRow4[1], DEV_LOOK_KEY, 1); AssertKey(kDevBatFight, DEV_LOOK_KEY, 0);                 /* the touch, not the cursor */
    v.pressed = -1; v.cursor = CTR_ACTION_BALL; DevCheckBattle(&old, &v);
    AssertKey(kDevBatRow4[1], DEV_LOOK_KEY, 1); AssertKey(kDevBatFight, DEV_LOOK_KEY, 0);                 /* the cursor on the ball */
    v.cursor = 0;
    v.pressed = -1; DevCheckBattle(&old, &v); v.activeFrame ^= 1; v.showCursor ^= 1; assert(DevCheckBattle(&old, &v).count == 0);   /* not drawn: no redraw */
    /* nor anything else the commands do not draw: the party's HP and conditions, the lead's types, the ball's name and
     * place, the moves and the targets, the Party and the Bag */
    v.partyHp[1] = 40; v.partyStatus[0] = 5; v.activeType[1] = 2; strcpy(v.activeTypeName[1], "FLYING");
    strcpy(v.ballName, "ULTRA BALL"); v.ballPlace = 2; v.move[0].priority = 1; v.move[1].pp = 3; v.target[0].battler = 3;
    v.target[1].hp = 7; v.bag.count = 7; v.pty.cursor = 2; v.safari = 1;
    assert(DevCheckBattle(&old, &v).count == 0);
    v.safari = 0; v.move[1].pp = 15; v.target[1].hp = 80; assert(DevCheckBattle(&old, &v).count == 0);
    v.shell = 1; assert(DevWhole(DevCheckBattle(&old, &v)) && at(160, 100) == DEV_SHELL_BLUE);
    v.shell = 0; DevCheckBattle(&old, &v);
    /* to the Bag and back: the whole screen each way */
    v.state = BUI_BAG; assert(DevWhole(DevCheckBattle(&old, &v)));
    v.state = BUI_ACTION; assert(DevWhole(DevCheckBattle(&old, &v)));

    v.state = BUI_MOVE; v.cursor = 1; assert(DevWhole(DevCheckBattle(&old, &v)));
    v.activeHp = 50; v.party[5] = 1; v.ballQty = 3; v.runBlocked = 1; assert(DevCheckBattle(&old, &v).count == 0);   /* the lead: not drawn here */
    v.activeHp = 112; v.party[5] = 0; v.ballQty = 21; v.runBlocked = 0; DevCheckBattle(&old, &v);
    for (i = 0; i < 4; ++i) AssertHits(BatHitV, &v, kDevBatMove[i], i);
    AssertHits(BatHitV, &v, kDevBatCancel, BUI_CANCEL); AssertKey(kDevBatMove[1], DEV_LOOK_KEY, 1);
    AssertKey(kDevBatMove[0], DEV_LOOK_KEY, 0); AssertKey(kDevBatCancel, DEV_LOOK_DARK, 0);
    v.pressed = BUI_CANCEL; DevCheckBattle(&old, &v); AssertKey(kDevBatCancel, DEV_LOOK_DARK, 1); AssertKey(kDevBatMove[1], DEV_LOOK_KEY, 0);
    v.pressed = -1;
    /* the board's state: the cursor on FLAMETHROWER */
    v.cursor = 0; DevCheckBattle(&old, &v);
    assert(at(19, 100) == UiTypeColor(10) && at(62, 100) == DEV_FX_SUPER);                    /* sunk: 2 px lower */
    assert(at(175, 98) == UiTypeColor(0) && at(218, 98) == DEV_FX_EFFECTIVE && at(218, 168) == DEV_FX_NOTVERY);
    assert(at(62, 168) == DEV_KEY && Count(62, 166, 40, 10, DEV_INK2) > 0);                   /* GROWL: "STATUS", no chip */
    assert(Count(164, 114, 148, 14, DEV_PP_HALF) > 0 && Count(164, 184, 148, 14, DEV_PP_QUARTER) > 0);   /* 15/35, 3/15 */
    assert(Count(14, 14, 292, 16, DEV_CAT_SPECIAL) > 0 && Count(14, 14, 292, 16, DEV_CAT_PHYSICAL) == 0);
    assert(Count(20, 31, 280, 26, DEV_INK2) > 0 && Count(12, 214, 120, 12, DEV_WHITE) > 0);   /* the description; the hint */
    shot("moves");
    v.cursor = 3; DevCheckBattle(&old, &v);
    assert(Count(14, 14, 292, 16, DEV_CAT_PHYSICAL) > 0 && Count(14, 14, 292, 16, DEV_CAT_SPECIAL) == 0);
    v.cursor = 2; DevCheckBattle(&old, &v);                                                     /* no power: no category */
    assert(Count(14, 14, 292, 16, DEV_CAT_PHYSICAL) == 0 && Count(14, 14, 292, 16, DEV_CAT_SPECIAL) == 0);
    v.cursor = 0; v.move[0].desc[0] = 0; DevCheckBattle(&old, &v);                            /* no description: STAB, the effect */
    assert(Count(20, 31, 280, 14, DEV_INK2) > 0);
    v.move[0] = BatBoard().move[0];
    v.cursor = 1; DevCheckBattle(&old, &v);
    BatUpperCase(v);
    BatNames(v);
    CtrBattleUi_Draw(canvas, &old);
    v.move[2].present = v.move[3].present = 0; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 82, 173) == -1);   /* two moves: holes */
    assert(CtrBattleUi_Hit(&v, 238, 173) == -1 && at(82, 173) == DevBlend(DEV_SHELL_RED, 0, DEV_A_HOLE));
    shot("moves_two");
    v.move[2] = BatBoard().move[2];
    v.move[3] = BatBoard().move[3];

    v.state = BUI_TARGET; /* target[0], target[1] foes, target[2] the ally */ DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, kDevBatMove[1], 1); AssertHits(BatHitV, &v, kDevBatCancel, BUI_CANCEL);
    AssertHits(BatHitV, &v, kDevBatMove[0], 0); AssertHits(BatHitV, &v, kDevBatMove[2], 2);
    assert(CtrBattleUi_Hit(&v, 238, 173) == -1);                                              /* no fourth: a hole */
    assert(at(19, 98) == DEV_STAT_UP && at(19, 168) == DEV_BLUE_INK && at(218, 100) == DEV_FX_SUPER);   /* FOE, ALLY; RATICATE */
    assert(at(18, 72 + 46) == DEV_HP_FRAME && Count(80, 184, 76, 14, DEV_KEY_INK) > 0);    /* the bar; the ally's hp/max */
    shot("target");

    v.state = BUI_SAFARI; v.cursor = 2; DevCheckBattle(&old, &v); for (i = 0; i < 4; ++i) AssertHits(BatHitV, &v, kDevBatMove[i], i);
    AssertKey(kDevBatMove[2], DEV_LOOK_KEY, 1); AssertKey(kDevBatMove[0], DEV_LOOK_KEY, 0);
    assert(CtrBattleUi_Hit(&v, 200, 222) == -1 && Count(14, 14, 292, 44, DEV_INK) > 0);       /* no CANCEL; the prompt */
    shot("safari");

    v.state = BUI_IDLE; v.ball = CTR_QUICK_BALL_READY; v.ballCount = 4; v.ballItem = 4; v.ballQty = 21;   /* a wild battle */
    DevCheckBattle(&old, &v); for (y = 0; y < 240; y += 3) for (x = 0; x < 320; x += 3) assert(CtrBattleUi_Hit(&v, x, y) == -1);
    assert(at(11, 132) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128) && at(11, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    assert(at(108, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));                         /* the quick ball's key, grey */
    assert(Count(20, 71, 280, 15, DEV_INK) == 0 && Count(76, 17, 140, 12, DEV_INK) > 0);   /* no prompt; the lead still */
    shot("idle");
    v.cursor = 3; v.pressed = 2; assert(DevCheckBattle(&old, &v).count == 0);             /* idle: no key chosen */
    v.ball = CTR_QUICK_BALL_NONE; v.ballCount = 0; DevCheckBattle(&old, &v);                /* a trainer: three keys */
    assert(at(108, 200) == DEV_SHELL_RED && at(219, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    v.safari = 1; assert(DevWhole(DevCheckBattle(&old, &v)));                              /* the Safari Zone: its own keys */
    for (y = 0; y < 240; y += 3) for (x = 0; x < 320; x += 3) assert(CtrBattleUi_Hit(&v, x, y) == -1);
    for (i = 0; i < 4; ++i)
        assert(at(kDevBatMove[i].x + 3, kDevBatMove[i].y + 31) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    assert(Count(14, 14, 292, 44, DEV_INK) == 0 && at(11, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    shot("idle_safari");
    assert(sDevKeyCut == 0);
}

/* ---- party: the bays, the action menu, YES / NO, a message, the level up, picking several (boards B · POKeMON) -------- */

/* The board's Party: CHARMELEON 112/112 Lv36 (the cursor), PIDGEOTTO 62/94 Lv31, RATICATE 24/70 poisoned, NIDORINO
 * 0/76 fainted, ONIX 5/64 Lv25 and a hole; the menu's rows the board's (SUMMARY, SWITCH, ITEM, CUT, FLY, CANCEL) and two
 * more for a menu of eight. Names as FireRed has them, each its species' own. */
static CtrBattleView PtyBoard(void)
{
    static const struct {
        const char *name;
        int level, hp, max, status;
    } kMon[5] = {{"CHARMELEON", 36, 112, 112, 0}, {"PIDGEOTTO", 31, 62, 94, 0}, {"RATICATE", 28, 24, 70, 1},
                 {"NIDORINO", 30, 0, 76, 0}, {"ONIX", 25, 5, 64, 0}};
    static const char *const kMenu[8] = {"SUMMARY", "SWITCH", "ITEM", "CUT", "FLY", "CANCEL", "SURF", "STRENGTH"};
    CtrBattleView v;
    CtrBattleParty *p = &v.pty;

    memset(&v, 0, sizeof(v));
    v.state = BUI_PARTY;
    v.pressed = -1;
    v.cursor = -1;
    p->cursor = 0;
    p->button = -1;
    p->mode = BUI_PTY_SWITCH;
    p->lead = 1;
    for (int i = 0; i < 5; ++i) {
        p->present[i] = 1;
        p->level[i] = (uint8_t)kMon[i].level;
        p->hp[i] = (uint16_t)kMon[i].hp;
        p->maxHp[i] = (uint16_t)kMon[i].max;
        p->status[i] = (uint8_t)kMon[i].status;
        p->icon[i] = (uint16_t)(i + 1);
        strcpy(p->name[i], kMon[i].name);
        p->nameIsSpecies[i] = 1;
    }
    for (int i = 0; i < 8; ++i)
        strcpy(p->menuName[i], kMenu[i]);
    return v;
}

/* With small letters wider than capitals: `a` and `b` drawn alike (or not, `same` 0). */
static void PtySamePixels(const CtrBattleView *a, const CtrBattleView *b, int same)
{
    static uint16_t first[W * H];
    static uint8_t keepWidths[256], keepSmall[256];

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    CtrBattleUi_Draw(canvas, a);
    memcpy(first, canvas, sizeof(first));
    CtrBattleUi_Draw(canvas, b);
    if ((memcmp(first, canvas, sizeof(first)) == 0) != same) {
        fprintf(stderr, "%s: panel %d, names %s / %s: drawn %s\n", gSuite, a->pty.panel, a->pty.name[0], b->pty.name[0],
                same ? "differently" : "alike");
        assert(0);
    }
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* Upper case (decision 5): a Pokemon's name in capitals when it is its species' own whatever case it arrives in, a
 * nickname as typed (the bays, the slim bays, the level up); the menu's names in capitals. */
static void PtyUpperCase(CtrBattleView v)
{
    CtrBattleView other;

    v.pty.panel = BUI_PTY_NONE;
    other = v;
    strcpy(other.pty.name[0], "Charmeleon");
    strcpy(other.pty.name[1], "Pidgeotto");
    PtySamePixels(&v, &other, 1);
    strcpy(v.pty.name[0], "SPARKY");
    strcpy(other.pty.name[0], "Sparky");
    v.pty.nameIsSpecies[0] = other.pty.nameIsSpecies[0] = 0;
    PtySamePixels(&v, &other, 0);
    v.pty.panel = other.pty.panel = BUI_PTY_ACTIONS;
    v.pty.menuCount = other.pty.menuCount = 6;
    strcpy(other.pty.name[0], "SPARKY");
    strcpy(v.pty.menuName[0], "SUMMARY");
    strcpy(v.pty.menuName[3], "CUT");
    strcpy(other.pty.menuName[0], "Summary");
    strcpy(other.pty.menuName[3], "Cut");
    PtySamePixels(&v, &other, 1);
    strcpy(other.pty.name[0], "Sparky");
    PtySamePixels(&v, &other, 0);
    v.pty.panel = other.pty.panel = BUI_PTY_LEVELUP;
    strcpy(other.pty.name[0], "SPARKY");
    v.pty.lvSlot = other.pty.lvSlot = 1;                    /* PIDGEOTTO and Pidgeotto, its species' own */
    PtySamePixels(&v, &other, 1);
}

static void SuiteParty(void)
{
    CtrBattleView v = PtyBoard(), old = v;
    int i;
    uint16_t dim = DevBlend(DEV_SHELL_RED, DEV_KEY, 128);

    CtrBattleUi_Draw(canvas, &old);
    DevCheckBattle(&old, &v);
    /* the plan's suite: the bays and CANCEL hit where they are drawn; the hole not hit */
    for (i = 0; i < 5; ++i) AssertHits(BatHitV, &v, DevPtyBay(i), BUI_PTY_SLOT + i);
    assert(CtrBattleUi_Hit(&v, 238, 145) == -1 && at(238, 150) == DevBlend(DEV_SHELL_RED, 0, 72));      /* the hole */
    AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 1); AssertKey(DevPtyBay(3), DEV_LOOK_FAINT, 0); AssertHits(BatHitV, &v, kDevPtyCancel, BUI_PTY_CANCEL);
    /* the board's pieces: the chosen bay's LED; RATICATE's PSN and NIDORINO's FNT at the right; NIDORINO's HP grey;
     * CHARMELEON's bar, ONIX's numbers in the low ink; the screen's words; CANCEL dark */
    assert(at(146, 18) == DEV_LED && at(128, 105) == DEV_PSN && at(284, 105) == DEV_FNT);
    assert(at(202, 93) == UI_RGB(0x8A, 0x8A, 0x8A) && at(60, 41) == DEV_HP_FRAME && at(61, 42) == DEV_HP_GREEN);
    assert(Count(100, 145, 48, 9, DEV_LOW_INK) > 0 && Count(120, 33, 28, 9, DEV_LOW_INK) == 0);
    assert(Count(14, 186, 210, 40, DEV_INK) > 0 && Count(46, 19, 90, 8, DEV_SUNK_INK) > 0 && Count(202, 75, 90, 8, DEV_FAINT_INK) > 0);
    AssertKey(kDevPtyCancel, DEV_LOOK_DARK, 0); assert(Count(238, 180, 74, 50, DEV_DARK_INK) > 0 && at(146, 18 + 56) != DEV_LED);
    for (i = 0; i < 6; ++i)                                                    /* icons at 1:1, not hopping */
        if (i != 5) assert(at(DevPtyBay(i).x + 4 + 2, DevPtyBay(i).y + (i ? 0 : 2) + 9 + 2) != DevKeyFace(i == 3 ? DEV_LOOK_FAINT : DEV_LOOK_KEY, !i));
    shot("bays");
    /* the touch sinks one key of the group: a bay, CANCEL; the cursor on CANCEL (FireRed's slot 7) sinks it */
    v.pressed = BUI_PTY_SLOT + 2; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(2), DEV_LOOK_KEY, 1); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0);
    v.pressed = BUI_PTY_CANCEL; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0); assert(at(302, 190) == DEV_LED);
    v.pressed = -1; v.pty.cursor = -1; v.pty.button = 1; DevCheckBattle(&old, &v); assert(at(302, 190) == DEV_LED);
    v.pty.cursor = 0; v.pty.button = -1; DevCheckBattle(&old, &v); assert(at(302, 190) != DEV_LED);
    /* not drawn: the icons' frames, the column layout, the battle's own fields */
    v.pty.iconFrame[1] = 3; v.pty.lead = 2; v.activeFrame = 1; v.cursor = 2; v.move[0].present = 1; v.activeHp = 9;
    v.party[3] = 2; v.bag.count = 3; v.ball = CTR_QUICK_BALL_READY; v.target[1].present = 1; v.showCursor = 1;
    assert(DevCheckBattle(&old, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckBattle(&old, &v)) && at(238, 150) == DevBlend(DEV_SHELL_GREEN, 0, 72));
    v.shell = 0; DevCheckBattle(&old, &v);
    /* an item's verdict at the right (TARGET), before a condition */
    v.pty.mode = BUI_PTY_TARGET; v.pty.tag[0] = BUI_PTY_TAG_ABLE; v.pty.tag[2] = BUI_PTY_TAG_NOT_ABLE; v.pty.tag[4] = BUI_PTY_TAG_LEARNED;
    DevCheckBattle(&old, &v); assert(at(146, 50) == UI_RGB(56, 168, 88) && at(146, 104) == UI_RGB(112, 112, 128) && at(146, 160) == UI_RGB(70, 132, 214));
    shot("teach");
    memset(v.pty.tag, 0, sizeof(v.pty.tag)); v.pty.mode = BUI_PTY_SWITCH; DevCheckBattle(&old, &v);

    /* FORCED (a Pokemon must be sent out): CANCEL grey, never taken; FireRed's cursor can still be on it (SEND OUT
     * then fails), and then it is the chosen key, sunk with its LED, grey all the same; a touch on a bay is chosen */
    v.pty.mode = BUI_PTY_FORCED; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 275, 205) == -1 && CtrBattleUi_Hit(&v, 302, 190) == -1);
    assert(at(245, 200) == DevBlend(DEV_SHELL_RED, DevKeyFace(DEV_LOOK_DARK, 0), 128) && at(302, 190) != DevBlend(DEV_SHELL_RED, DEV_LED, 128));
    shot("forced");
    v.pty.cursor = -1; v.pty.button = 1; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0);
    assert(at(245, 200) == DevBlend(DEV_SHELL_RED, DevKeyFace(DEV_LOOK_DARK, 1), 128) && at(302, 190) == DevBlend(DEV_SHELL_RED, DEV_LED, 128));
    assert(CtrBattleUi_Hit(&v, 275, 205) == -1);
    shot("forced_cancel");
    v.pressed = BUI_PTY_SLOT + 1; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(1), DEV_LOOK_KEY, 1);
    assert(at(302, 190) != DevBlend(DEV_SHELL_RED, DEV_LED, 128));
    v.pressed = -1; v.pty.cursor = 0; v.pty.button = -1; DevCheckBattle(&old, &v);
    v.pty.mode = BUI_PTY_MULTI; v.pty.button = 1; DevCheckBattle(&old, &v);
    AssertHits(BatHitV, &v, kDevPtyConfirm, BUI_PTY_CONFIRM); AssertHits(BatHitV, &v, kDevPtyCancelLow, BUI_PTY_CANCEL); AssertKey(kDevPtyCancelLow, DEV_LOOK_DARK, 1);
    /* picking several: the order on the picked bays, a Pokemon that may not enter grey; the cursor on CONFIRM */
    v.pty.cursor = -1; v.pty.button = 0; v.pty.entry[0] = BUI_PTY_FIRST + 1; v.pty.entry[1] = BUI_PTY_FIRST; v.pty.entry[2] = BUI_PTY_NOT_ABLE;
    v.pty.entry[3] = v.pty.entry[4] = BUI_PTY_ABLE; DevCheckBattle(&old, &v);
    AssertKey(kDevPtyConfirm, DEV_LOOK_KEY, 1); AssertKey(DevPtyBay(2), DEV_LOOK_FAINT, 0); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0);
    assert(at(146, 50) == DEV_BLUE_INK && at(302, 50) == DEV_BLUE_INK && at(146, 160) != DEV_BLUE_INK);
    shot("multi");
    v.pty.button = 1; v.pty.cursor = 0; memset(v.pty.entry, 0, sizeof(v.pty.entry)); DevCheckBattle(&old, &v);
    v.pty.mode = BUI_PTY_SWITCH; v.pty.panel = BUI_PTY_ACTIONS; v.pty.menuCount = 3; v.pty.menuCursor = 0; v.pty.menuDark = 0x04; DevCheckBattle(&old, &v);
    for (i = 0; i < 3; ++i) AssertHits(BatHitV, &v, DevPtyMenuKey(3, i), BUI_PTY_MENU + i);
    AssertKey(DevPtyMenuKey(3, 2), DEV_LOOK_DARK, 0); assert(CtrBattleUi_Hit(&v, 100, 20) == -1);           /* bays: not while a menu is open */
    /* the slim bays: the chosen one sunk, the hole; the menu's screen with the Party's words; the first row chosen */
    AssertKey(DevPtySlim(3, 0), DEV_LOOK_KEY, 1); AssertKey(DevPtySlim(3, 1), DEV_LOOK_KEY, 0); AssertKey(DevPtySlim(3, 3), DEV_LOOK_FAINT, 0);
    assert(at(100, 8 + 28 * 5 + 12) == DevBlend(DEV_SHELL_RED, 0, 72) && Count(14, 188, 292, 38, DEV_INK) > 0);
    AssertKey(DevPtyMenuKey(3, 0), DEV_LOOK_KEY, 1); AssertKey(DevPtyMenuKey(3, 1), DEV_LOOK_KEY, 0);
    v.pressed = BUI_PTY_MENU + 1; DevCheckBattle(&old, &v); AssertKey(DevPtyMenuKey(3, 1), DEV_LOOK_KEY, 1); AssertKey(DevPtyMenuKey(3, 0), DEV_LOOK_KEY, 0);
    v.pressed = -1; DevCheckBattle(&old, &v);
    strcpy(v.pty.menuName[0], "SHIFT"); strcpy(v.pty.menuName[1], "SUMMARY"); strcpy(v.pty.menuName[2], "CANCEL"); DevCheckBattle(&old, &v);
    shot("menu3");
    strcpy(v.pty.menuName[0], "SUMMARY"); strcpy(v.pty.menuName[1], "SWITCH"); strcpy(v.pty.menuName[2], "ITEM");
    v.pty.menuCount = 6; v.pty.menuBlue = 0x18; v.pty.menuDark = 0x20; DevCheckBattle(&old, &v);
    assert(CtrBattleUi_Hit(&v, 175, 20) == BUI_PTY_MENU && CtrBattleUi_Hit(&v, 175, 76) == BUI_PTY_MENU + 2 && CtrBattleUi_Hit(&v, 269, 20) == BUI_PTY_MENU + 3 && CtrBattleUi_Hit(&v, 269, 76) == BUI_PTY_MENU + 5);
    /* field moves in blue, CANCEL dark; the slim bays narrowed */
    assert(Count(226, 8, 86, 24, DEV_BLUE_INK) > 0 && Count(226, 36, 86, 24, DEV_BLUE_INK) > 0 && Count(132, 8, 86, 54, DEV_BLUE_INK) == 0);
    AssertKey(DevPtyMenuKey(6, 5), DEV_LOOK_DARK, 0); assert(at(128, 20) == DEV_SHELL_RED);
    v.pty.caption[0] = 0; strcpy(v.pty.caption, "Do what with this POK\xC3\xA9MON?"); DevCheckBattle(&old, &v);
    shot("menu6");
    v.pty.menuCount = 8; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 269, 104) == BUI_PTY_MENU + 7);
    {
        static const char *const kEight[8] = {"SUMMARY", "CUT", "FLY", "SURF", "STRENGTH", "SWITCH", "ITEM", "CANCEL"};

        for (i = 0; i < 8; ++i) strcpy(v.pty.menuName[i], kEight[i]);
        v.pty.menuBlue = 0x1E; v.pty.menuDark = 0x80; v.pty.menuCursor = 3; DevCheckBattle(&old, &v);
        AssertKey(DevPtyMenuKey(8, 3), DEV_LOOK_KEY, 1); AssertKey(DevPtyMenuKey(8, 7), DEV_LOOK_DARK, 0);
        shot("menu8");
        /* a long word on a chosen key clears its LED (a German field move) */
        strcpy(v.pty.menuName[1], "ZERTRUEMMERER"); v.pty.menuCursor = 1; DevCheckBattle(&old, &v);
        assert(at(209, 44) == DEV_LED && Count(205, 40, 12, 10, DEV_BLUE_INK) == 0 && Count(140, 40, 64, 10, DEV_BLUE_INK) > 0);
        strcpy(v.pty.menuName[1], "CUT");
        v.pty.menuCursor = 0;
    }
    v.pty.panel = BUI_PTY_YESNO; v.pty.menuCount = 2; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 259, 20) == BUI_PTY_MENU && CtrBattleUi_Hit(&v, 259, 48) == BUI_PTY_MENU + 1);
    strcpy(v.pty.menuName[0], "YES"); strcpy(v.pty.menuName[1], "NO"); v.pty.menuBlue = v.pty.menuDark = 0;
    strcpy(v.pty.caption, "Switch places?"); DevCheckBattle(&old, &v); AssertKey(DevPtyMenuKey(2, 1), DEV_LOOK_KEY, 0);
    shot("yesno");
    v.pty.menuName[5][0] = 'X'; v.pty.menuBlue = 0x20; assert(DevCheckBattle(&old, &v).count == 0);      /* rows past the menu: not drawn */
    v.pty.panel = BUI_PTY_TEXT; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 10, 10) == BUI_PTY_MESSAGE && CtrBattleUi_Hit(&v, 300, 230) == BUI_PTY_MESSAGE);
    /* a message: on the screen, the keys grey and none chosen */
    strcpy(v.pty.message, "CHARMELEON is already in battle!"); DevCheckBattle(&old, &v);
    assert(at(11, 33) == dim && at(241, 205) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128) && Count(14, 186, 210, 40, DEV_INK) > 0);
    v.pressed = BUI_PTY_MESSAGE; v.pty.menuCount = 5; assert(DevCheckBattle(&old, &v).count == 0);         /* nothing to sink */
    v.pressed = -1; v.pty.menuCount = 2;
    shot("message");
    v.pty.panel = BUI_PTY_LEVELUP; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 160, 100) == BUI_PTY_MESSAGE); AssertKey(kDevPtyCancel, DEV_LOOK_KEY, 1);
    /* a level up: the stats and their gains on the tall screen, the message, OK */
    {
        static const uint16_t kStat[6] = {120, 80, 74, 86, 72, 98};

        v.pty.lvSlot = 0; v.pty.lvLevel = 37;
        for (i = 0; i < 6; ++i) { v.pty.lvStat[i] = kStat[i]; v.pty.lvGain[i] = (int16_t)(2 + (i & 1)); }
        strcpy(v.pty.message, "CHARMELEON grew to Lv. 37!"); DevCheckBattle(&old, &v);
        assert(at(302, 190) == DEV_LED && Count(14, 14, 292, 154, DEV_FX_SUPER) > 0 && Count(14, 14, 292, 20, DEV_INK) > 0);
        assert(Count(14, 186, 210, 40, DEV_INK) > 0 && at(20, 20) == DEV_PAPER);
        v.pressed = BUI_PTY_MESSAGE; v.pty.cursor = 3; v.pty.mode = BUI_PTY_TARGET; assert(DevCheckBattle(&old, &v).count == 0);
        v.pressed = -1; v.pty.cursor = 0; v.pty.mode = BUI_PTY_SWITCH; DevCheckBattle(&old, &v);
        shot("levelup");
    }
    PtyUpperCase(v);
    CtrBattleUi_Draw(canvas, &old);
    v.pty.button = -1; v.pty.caption[0] = 0;                                   /* the cursor on a slot again, no prompt */
    v.pty.panel = BUI_PTY_NONE; DevCheckBattle(&old, &v); v.pty.iconFrame[0] ^= 1; assert(DevCheckBattle(&old, &v).count == 0);   /* icons are not animated in B */
    /* to the battle's commands and the Bag and back: the whole screen each way */
    v.state = BUI_ACTION; assert(DevWhole(DevCheckBattle(&old, &v)));
    v.state = BUI_PARTY; assert(DevWhole(DevCheckBattle(&old, &v)));
    v.state = BUI_BAG; assert(DevWhole(DevCheckBattle(&old, &v)));
    v.state = BUI_PARTY; assert(DevWhole(DevCheckBattle(&old, &v)));
    v.pty.egg[1] = 1; DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, DevPtyBay(1), BUI_PTY_SLOT + 1);   /* an egg: no bar, still a bay */
    assert(Count(164 + 38, 8 + 29, 100, 9, DEV_HP_FRAME) == 0 && Count(164 + 38, 8 + 29, 100, 9, DEV_HP_LABEL) == 0);
    for (i = 1; i < 6; ++i) v.pty.present[i] = 0;                                                          /* a party of one */
    DevCheckBattle(&old, &v);
    for (i = 1; i < 6; ++i) { CtrPokeTouchRect r = DevPtyBay(i); assert(CtrBattleUi_Hit(&v, r.x + 74, r.y + 40) == -1 && at(r.x + 74, r.y + 40) == DevBlend(DEV_SHELL_RED, 0, 72)); }
    shot("one");
    assert(sDevKeyCut == 0);                      /* main prints PASS device party */
}

/* ---- summary: INFO, STATS (IV/EV), MOVES, the move details, MEMO (boards B · Resumo) ---------------------------------- */

static int SumHitV(const void *view, int x, int y)
{
    return CtrSummaryUi_Hit((const CtrSummaryUiView *)view, x, y);
}

/* The boards' Pokemon: CHARMELEON (its species' own name), male, Lv36, No.005, OT RED, no item, ADAMANT (3), BLAZE,
 * 112/112; ATTACK 84, DEFENSE 66, SP. ATK 70, SP. DEF 68, SPEED 85, 41 % of the way to Lv37 (Medium Slow); FLAMETHROWER
 * 15/15 (FIRE, 95, 100), SCRATCH 15/35, GROWL 40/40 (a status move), BRICK BREAK 3/15 (FIGHTING). Strings as FireRed
 * has them (the B views are upper case); FireRed's cursor on the first move. */
static CtrSummaryUiView SumBoard(void)
{
    static const char *const kMove[4] = {"FLAMETHROWER", "SCRATCH", "GROWL", "BRICK BREAK"};
    static const char *const kType[4] = {"FIRE", "NORMAL", "NORMAL", "FIGHTING"};
    static const unsigned char kTypeId[4] = {10, 0, 0, 1};
    static const char *const kPp[4] = {"15/15", "15/35", "40/40", "3/15"};
    static const char *const kPower[4] = {"95", "40", "---", "75"};
    static const char *const kStat[5] = {"84", "66", "70", "68", "85"};
    static const unsigned char kIv[6] = {25, 30, 14, 31, 19, 28}, kEv[6] = {6, 0, 0, 252, 0, 252};
    CtrSummaryUiView v;
    CtrSummaryView *d = &v.data;

    memset(&v, 0, sizeof(v));
    v.pressed = -1;
    v.icon = 5;
    d->serial = 1;
    d->origin = CTR_SUMMARY_FIELD_PARTY;
    d->page = SUI_PAGE_INFO;
    strcpy(d->nickname, "CHARMELEON");
    strcpy(d->species, "CHARMELEON");
    strcpy(d->level, "36");
    strcpy(d->gender, "M");
    strcpy(d->status, "OK");
    strcpy(d->dex, "005");
    strcpy(d->ot, "RED");
    strcpy(d->id, "24601");
    strcpy(d->item, "NONE");
    strcpy(d->types[0], "FIRE");
    strcpy(d->types[1], "FIRE");
    d->typeIds[0] = d->typeIds[1] = 10;
    strcpy(d->nature, "ADAMANT");
    d->natureId = 3;
    strcpy(d->hp, "112/112");
    d->hpCur = d->hpMax = 112;
    for (int i = 0; i < 5; ++i)
        strcpy(d->stats[i], kStat[i]);
    strcpy(d->exp, "41000");
    strcpy(d->nextExp, "2808");
    strcpy(d->ability, "BLAZE");
    strcpy(d->abilityDesc, "Powers up FIRE moves in a pinch.");
    strcpy(d->memo, "ADAMANT nature.\nMet at Lv5, PALLET TOWN.");
    for (int i = 0; i < 4; ++i) {
        d->moves[i] = (unsigned short)(i + 1);
        strcpy(d->moveNames[i], kMove[i]);
        strcpy(d->pp[i], kPp[i]);
        strcpy(d->moveTypes[i], kType[i]);
        d->moveTypeIds[i] = kTypeId[i];
        strcpy(d->power[i], kPower[i]);
        strcpy(d->accuracy[i], "100");
    }
    memcpy(d->iv, kIv, sizeof(kIv));
    memcpy(d->ev, kEv, sizeof(kEv));
    d->hpType = 12;
    d->hpPower = 70;
    strcpy(d->hpTypeName, "GRASS");
    return v;
}

/* With small letters wider than capitals: `a` and `b` drawn alike (or not, `same` 0). */
static void SumSamePixels(const CtrSummaryUiView *a, const CtrSummaryUiView *b, int same)
{
    static uint16_t first[W * H];
    static uint8_t keepWidths[256], keepSmall[256];

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    CtrSummaryUi_Draw(canvas, a);
    memcpy(first, canvas, sizeof(first));
    CtrSummaryUi_Draw(canvas, b);
    if ((memcmp(first, canvas, sizeof(first)) == 0) != same) {
        fprintf(stderr, "%s: page %d, trainer %d, names %s / %s: drawn %s\n", gSuite, a->data.page, a->trainer,
                a->data.nickname, b->data.nickname, same ? "differently" : "alike");
        assert(0);
    }
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* Upper case (decision 5): FireRed's names arrive in capitals or in recipe 0251's title case and are drawn alike, on
 * every page; the Pokemon's name in capitals when it is its species' own, a nickname as typed. */
static void SumUpperCase(CtrSummaryUiView v)
{
    static const int kPage[5] = {SUI_PAGE_INFO, SUI_PAGE_SKILLS, SUI_PAGE_MOVES, SUI_PAGE_MOVES_INFO, SUI_PAGE_INFO};
    CtrSummaryUiView proper;

    for (int k = 0; k < 5; ++k) {
        v.data.page = kPage[k];
        v.trainer = k == 4;
        v.data.cursor = 0;
        proper = v;
        strcpy(proper.data.nickname, "Charmeleon");
        strcpy(proper.data.species, "Charmeleon");
        strcpy(proper.data.item, "None");
        strcpy(proper.data.types[0], "Fire");
        strcpy(proper.data.types[1], "Fire");
        strcpy(proper.data.nature, "Adamant");
        strcpy(proper.data.ability, "Blaze");
        strcpy(proper.data.moveNames[0], "Flamethrower");
        strcpy(proper.data.moveNames[3], "Brick Break");
        strcpy(proper.data.moveTypes[0], "Fire");
        strcpy(proper.data.hpTypeName, "Grass");
        SumSamePixels(&v, &proper, 1);
    }
    v.data.page = SUI_PAGE_INFO;
    v.trainer = 0;
    strcpy(v.data.nickname, "SPARKY");
    proper = v;
    strcpy(proper.data.nickname, "Sparky");
    SumSamePixels(&v, &proper, 0);
}

/* The plan's suite (the tabs, the arrows, BACK, the switch and the move rows hit where they are drawn; the chosen tab;
 * the arrows off in the move details; CANCEL inverted under FireRed's cursor; an egg's pages; the frame while loading),
 * the boards' pieces, what is not drawn, upper case, sparse data, then the states for the eye. */
static void SuiteSummary(void)
{
    CtrSummaryUiView sv = SumBoard(), so = sv;
    CtrPokeTouchRect r;
    uint16_t dimKey = DevBlend(DEV_SHELL_RED, DEV_KEY, 128), dimDark = DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128);
    uint16_t bar = UI_RGB(0x8A, 0x94, 0xA8), fire = UiTypeColor(10), red = GbaColor(SUM_ART_RED), blue = GbaColor(SUM_ART_BLUE);
    int i;

    assert(CtrNature_Up(3) == 0 && CtrNature_Down(3) == 2 && CtrNature_Up(15) == 2 && CtrNature_Down(15) == 0
           && CtrNature_Up(10) == 4 && CtrNature_Up(0) == -1 && CtrNature_Down(24) == -1 && CtrNature_Up(25) == -1);
    for (i = 0; i < 25; ++i) {          /* the five with n / 5 == n % 5 are neutral; each other raises one stat, lowers another */
        int up = CtrNature_Up(i), down = CtrNature_Down(i);

        assert(i / 5 == i % 5 ? up == -1 && down == -1 : up >= 0 && up <= 4 && down >= 0 && down <= 4 && up != down);
    }
    sumArt = 1;
    CtrSummaryUi_Draw(canvas, &so);
    DevCheckSummary(&so, &sv);
    for (i = 0; i < 4; ++i) AssertHits(SumHitV, &sv, kDevSumTab[i], SUI_HIT_TAB + i);
    AssertHits(SumHitV, &sv, kDevSumPrev, SUI_HIT_PREV); AssertHits(SumHitV, &sv, kDevSumNext, SUI_HIT_NEXT); AssertHits(SumHitV, &sv, kDevSumBack, SUI_HIT_BACK);
    AssertKey(kDevSumTab[0], DEV_LOOK_KEY, 1); AssertKey(kDevSumPrev, DEV_LOOK_DARK, 0);
    /* INFO, the board's pieces: the chosen tab's LED, the others up; the arrows and BACK dark with their light ink; the
     * left column's paper, the picture in its box, the FIRE chip; the rows' labels and values, the HP bar, the EXP chip
     * and bar; the ability's name and its description on the note */
    assert(at(54, 15) == DEV_LED);
    for (i = 1; i < 4; ++i) AssertKey(kDevSumTab[i], DEV_LOOK_KEY, 0);
    AssertKey(kDevSumNext, DEV_LOOK_DARK, 0); AssertKey(kDevSumBack, DEV_LOOK_DARK, 0);
    assert(Count(252, 8, 28, 24, DEV_DARK_INK) > 0 && Count(284, 8, 28, 24, DEV_DARK_INK) > 0 && Count(244, 188, 68, 42, DEV_DARK_INK) > 0);
    assert(at(16, 48) == DEV_PAPER2 && at(116, 172) == DEV_PAPER2 && at(119, 48) == DEV_PAPER);
    assert(at(66, 114) == red && at(35, 83) == DEV_PAPER2 && at(97, 145) == DEV_PAPER2 && Count(34, 82, 64, 64, blue) > 0);
    assert(at(21, 155) == fire && Count(20, 154, 92, 12, fire) > 0);
    assert(Count(126, 50, 172, 96, DEV_INK) > 0 && Count(126, 50, 60, 96, DEV_INK2) > 0);
    assert(at(126, 150) == DEV_HP_FRAME && at(128, 152) == DEV_HP_GREEN && at(296, 152) == DEV_HP_GREEN);
    assert(at(126, 159) == DEV_HP_FRAME && Count(126, 159, 172, 9, DEV_EXP_LABEL) > 0 && Count(126, 159, 172, 9, DEV_EXP) > 0);
    assert(Count(14, 194, 216, 32, DEV_SUNK_INK) > 0 && Count(14, 194, 216, 32, DEV_INK) > 0);
    shot("info");

    sv.data.page = SUI_PAGE_SKILLS; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 263, 55) == SUI_HIT_IVTOGGLE); AssertKey(kDevSumTab[1], DEV_LOOK_KEY, 1);
    /* STATS: the switch hit round where it is drawn, its STATS half shown; the nature's name; ATTACK raised (red, ▲),
     * SP. ATK lowered (blue, ▼), the others in the secondary ink; a bar each, HP's (the largest) full; the EXP line */
    AssertRowHits(SumHitV, &sv, kDevSumIvSwitchHit, SUI_HIT_IVTOGGLE); AssertKey(kDevSumTab[0], DEV_LOOK_KEY, 0);
    assert(Count(229, 50, 34, 11, DEV_SHELL_BLUE) > 0 && Count(264, 50, 33, 11, DEV_SHELL_BLUE) == 0 && at(228, 55) == DEV_BUTTON_EDGE);
    assert(Count(126, 49, 96, 13, DEV_INK2) > 0);
    assert(Count(126, 79, 64, 13, DEV_STAT_UP) > 0 && Count(126, 107, 64, 13, DEV_BLUE_INK) > 0);
    assert(Count(126, 65, 172, 84, DEV_STAT_UP) == Count(126, 79, 64, 13, DEV_STAT_UP));
    assert(Count(126, 65, 172, 84, DEV_BLUE_INK) == Count(126, 107, 64, 13, DEV_BLUE_INK));
    for (i = 0; i < 6; ++i) {
        assert(Count(126, 65 + 14 * i, 172, 13, bar) > 0 && Count(282, 65 + 14 * i, 16, 13, DEV_INK) > 0);
        if (i != 1 && i != 3) assert(Count(126, 65 + 14 * i, 48, 13, DEV_INK2) > 0);
    }
    assert(Count(126, 65, 172, 13, UI_RGB(0xE2, 0xE2, 0xDA)) == 0 && Count(126, 93, 172, 13, UI_RGB(0xE2, 0xE2, 0xDA)) > 0);
    assert(Count(126, 152, 172, 9, DEV_EXP) > 0 && Count(126, 152, 172, 9, DEV_INK2) > 0);
    assert(Count(14, 194, 216, 32, DEV_SUNK_INK) > 0);
    shot("stats");
    /* a neutral nature marks nothing; a held touch on the switch draws nothing */
    sv.data.natureId = 6; DevCheckSummary(&so, &sv);
    assert(Count(126, 65, 172, 84, DEV_STAT_UP) == 0 && Count(126, 65, 172, 84, DEV_BLUE_INK) == 0);
    sv.data.natureId = 3; DevCheckSummary(&so, &sv);
    sv.pressed = SUI_HIT_IVTOGGLE; assert(DevCheckSummary(&so, &sv).count == 0);
    sv.pressed = -1; DevCheckSummary(&so, &sv);

    sv.ivView = 1; DevCheckSummary(&so, &sv);
    sv.ivView = 0; assert(DevWhole(DevCheckSummary(&so, &sv)));                 /* the switch: the screen whole */
    sv.ivView = 1; assert(DevWhole(DevCheckSummary(&so, &sv)));
    /* IV / EV: the switch's other half; a bar each (the IVs), Hidden Power's type and power on the last line */
    assert(Count(229, 50, 34, 11, DEV_SHELL_BLUE) == 0 && Count(264, 50, 33, 11, DEV_SHELL_BLUE) > 0);
    for (i = 0; i < 6; ++i) assert(Count(126, 65 + 14 * i, 172, 13, bar) > 0);
    assert(Count(126, 150, 172, 13, UiTypeColor(12)) > 0 && Count(126, 150, 172, 13, DEV_EXP) == 0);
    shot("ivev");

    sv.data.page = SUI_PAGE_MOVES; DevCheckSummary(&so, &sv);
    for (i = 0; i < 4; ++i) AssertRowHits(SumHitV, &sv, DevSumRow(i), SUI_HIT_MOVE + i);
    assert(CtrSummaryUi_Hit(&sv, 263, 55) != SUI_HIT_IVTOGGLE && CtrSummaryUi_Hit(&sv, 212, 159) == -1);
    /* MOVES: the rows on the paper, none chosen, each with its type's band; SCRATCH's PP at half, BRICK BREAK's at a
     * quarter (FireRed's colours); the icon, the name and the level, a rule, the Pokemon's type; no CANCEL row */
    AssertKey(kDevSumTab[2], DEV_LOOK_KEY, 1);
    for (i = 0; i < 4; ++i) assert(at(126, DevSumRow(i).y + 2) == DEV_PAPER);
    assert(at(132, 58) == fire && at(132, 82) == UiTypeColor(0) && at(132, 130) == UiTypeColor(1));
    assert(Count(250, 76, 50, 22, DEV_PP_HALF) > 0 && Count(250, 124, 50, 22, DEV_PP_QUARTER) > 0 && Count(250, 52, 50, 22, DEV_INK) > 0);
    assert(at(30, 62) == red && at(60, 62) != red && at(20, 90) == DEV_HP_TRACK && at(21, 98) == fire);
    assert(Count(124, 148, 176, 22, DEV_INK) == 0 && Count(124, 148, 176, 22, DEV_INK2) == 0);
    assert(Count(14, 194, 216, 32, DEV_INK) > 0);
    shot("moves");
    /* a held touch inverts its row */
    sv.pressed = SUI_HIT_MOVE + 2; DevCheckSummary(&so, &sv); assert(at(126, 102) == DEV_BEZEL && at(126, 54) == DEV_PAPER);
    sv.pressed = -1; DevCheckSummary(&so, &sv);

    /* the details: FireRed's cursor inverts its row; the move's type and category, POWER and ACCURACY in the left
     * column; its description on the note; the arrows grey and not taken; CANCEL under the moves */
    sv.data.page = SUI_PAGE_MOVES_INFO; sv.data.cursor = 0; strcpy(sv.data.moveDesc, "A powerful fire attack that may inflict a burn.");
    DevCheckSummary(&so, &sv);
    AssertKey(kDevSumTab[2], DEV_LOOK_KEY, 1);
    assert(at(126, 54) == DEV_BEZEL && Count(124, 52, 176, 22, DEV_PAPER) > 0 && at(126, 78) == DEV_PAPER);
    assert(at(21, 98) == fire && Count(20, 97, 92, 12, DEV_CAT_SPECIAL) > 0 && Count(20, 116, 92, 30, DEV_INK) > 0 && Count(20, 116, 92, 30, DEV_INK2) > 0);
    assert(at(255, 20) == dimDark && at(287, 20) == dimDark && Count(124, 148, 176, 22, DEV_INK2) > 0);
    AssertRowHits(SumHitV, &sv, DevSumRow(0), SUI_HIT_MOVE);
    assert(Count(14, 194, 216, 32, DEV_INK) > 0);
    shot("details");
    sv.data.cursor = 2; strcpy(sv.data.moveDesc, "Reduces the foe's ATTACK."); DevCheckSummary(&so, &sv);   /* GROWL: no category */
    assert(at(126, 102) == DEV_BEZEL && Count(20, 97, 92, 12, DEV_CAT_SPECIAL) == 0 && Count(20, 97, 92, 12, DEV_CAT_PHYSICAL) == 0);
    sv.data.cursor = 3; strcpy(sv.data.moveDesc, "Destroys barriers like LIGHT SCREEN."); DevCheckSummary(&so, &sv);   /* FIGHTING: physical */
    assert(Count(20, 97, 92, 12, DEV_CAT_PHYSICAL) > 0);
    sv.data.moveDesc[0] = 0;                                                    /* CANCEL: no description */
    sv.data.page = SUI_PAGE_MOVES_INFO; sv.data.cursor = 4; DevCheckSummary(&so, &sv);
    AssertRowHits(SumHitV, &sv, DevSumRow(4), SUI_HIT_CANCEL); assert(at(130, 159) == DEV_BEZEL);        /* the cursor on CANCEL: its row inverted */
    assert(CtrSummaryUi_Hit(&sv, 266, 20) == -1 && CtrSummaryUi_Hit(&sv, 298, 20) == -1);                 /* ▲▼ off in the details */
    assert(Count(124, 148, 176, 22, DEV_PAPER) > 0 && at(126, 54) == DEV_PAPER);
    assert(Count(14, 194, 216, 32, DEV_INK) == 0);                              /* CANCEL: nothing to describe */
    shot("details_cancel");
    /* a touch held on what is not drawn (the review's race: FireRed starts a swap under a touch on the CANCEL it hides; a
     * slot without a move) chooses nothing: FireRed's cursor row stays inverted, and the touch's end draws nothing */
    sv.data.cursor = 1; DevCheckSummary(&so, &sv);
    sv.data.swapping = 1; sv.data.swapCursor = 1; sv.pressed = SUI_HIT_CANCEL; DevCheckSummary(&so, &sv);
    assert(at(126, 78) == DEV_BEZEL);
    sv.pressed = -1; assert(DevCheckSummary(&so, &sv).count == 0);
    sv.data.swapping = 0; sv.data.moves[3] = 0; DevCheckSummary(&so, &sv);
    sv.pressed = SUI_HIT_MOVE + 3; DevCheckSummary(&so, &sv); assert(at(126, 78) == DEV_BEZEL);
    sv.pressed = -1; assert(DevCheckSummary(&so, &sv).count == 0);
    sv.data.moves[3] = 4; sv.data.cursor = 4; DevCheckSummary(&so, &sv);
    sv.data.swapping = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 212, 159) == -1);
    /* a swap: no CANCEL; the move being moved framed, the cursor's row inverted; the hint on the note */
    sv.data.swapCursor = 1; sv.data.cursor = 2; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 212, 159) == -1);
    assert(at(130, 159) == DEV_PAPER && Count(124, 148, 176, 22, DEV_INK) == 0 && Count(124, 148, 176, 22, DEV_INK2) == 0);
    assert(at(124, 86) == DEV_BEZEL && at(126, 86) == DEV_PAPER && at(126, 110) == DEV_BEZEL && Count(14, 194, 216, 32, DEV_INK) > 0);
    shot("swap");
    /* a battle's Summary: FireRed moves no move in a battle, so no swap hint on the note */
    sv.data.origin = CTR_SUMMARY_BATTLE_PARTY; DevCheckSummary(&so, &sv); assert(Count(14, 194, 216, 32, DEV_INK) == 0);
    sv.data.swapping = 0; DevCheckSummary(&so, &sv); assert(Count(14, 194, 216, 32, DEV_INK) == 0);         /* no description */
    sv.data.origin = CTR_SUMMARY_FIELD_PARTY; DevCheckSummary(&so, &sv); assert(Count(14, 194, 216, 32, DEV_INK) > 0);
    sv.data.swapping = 1; DevCheckSummary(&so, &sv);
    sv.data.swapCursor = 3; assert(DevCheckSummary(&so, &sv).count == 1);
    strcpy(sv.data.moveDesc, "Reduces the foe's ATTACK."); assert(DevCheckSummary(&so, &sv).count == 0);   /* the hint, not it */
    sv.data.swapping = 0; sv.data.swapCursor = 0; sv.data.cursor = 4; sv.data.moveDesc[0] = 0; DevCheckSummary(&so, &sv);
    sv.data.swapCursor = 2; assert(DevCheckSummary(&so, &sv).count == 0);                    /* not swapping: not drawn */

    sv.data.swapping = 0; sv.trainer = 1; sv.data.page = SUI_PAGE_INFO; DevCheckSummary(&so, &sv); AssertKey(kDevSumTab[3], DEV_LOOK_KEY, 1);
    /* MEMO: the left column as INFO's; OT, ID No., the nature, the ribbons and FireRed's memo; the arrows back; the hint */
    AssertKey(kDevSumTab[0], DEV_LOOK_KEY, 0); AssertKey(kDevSumPrev, DEV_LOOK_DARK, 0);
    assert(at(66, 114) == red && at(21, 155) == fire && Count(126, 50, 172, 64, DEV_INK) > 0 && Count(126, 50, 60, 64, DEV_INK2) > 0);
    assert(Count(126, 116, 172, 56, DEV_INK) > 0 && Count(14, 194, 216, 32, DEV_INK) > 0);
    shot("memo");

    /* not drawn: where the Summary was opened from, FireRed's readiness, the cursor and a description outside the details,
     * the IV / EV view outside STATS */
    sv.data.origin = CTR_SUMMARY_PC_BOX; sv.data.ready = 1; sv.data.cursor = 1; sv.ivView = 0; strcpy(sv.data.moveDesc, "x");
    assert(DevCheckSummary(&so, &sv).count == 0);
    sv.data.moveDesc[0] = 0;
    /* the shell's colour: the whole screen */
    sv.shell = 2; assert(DevWhole(DevCheckSummary(&so, &sv)) && at(1, 100) == DEV_SHELL_GREEN);
    sv.shell = 0; DevCheckSummary(&so, &sv);
    /* the 80x80 picture, whole and centred on the box; without a picture, the icon at 2x in the box */
    sumArt = 2; sv.data.serial = 2; DevCheckSummary(&so, &sv);
    assert(at(66, 114) == red && Count(26, 74, 80, 8, blue) > 0 && Count(26, 146, 80, 8, blue) > 0);
    sumArt = 3; sv.data.serial = 3; DevCheckSummary(&so, &sv);
    assert(at(34 + 4, 82 + 4) == red && at(34 + 60, 82 + 60) == red && Count(34, 82, 64, 64, blue) == 0);
    sumArt = 1; sv.data.serial = 1; DevCheckSummary(&so, &sv);
    SumUpperCase(sv);
    CtrSummaryUi_Draw(canvas, &so);

    /* a Pokemon with two moves: the others not drawn as moves and not taken */
    sv.trainer = 0; sv.data.page = SUI_PAGE_MOVES; sv.data.moves[2] = sv.data.moves[3] = 0;
    sv.data.moveNames[2][0] = sv.data.moveNames[3][0] = sv.data.pp[2][0] = sv.data.pp[3][0] = 0; DevCheckSummary(&so, &sv);
    for (i = 2; i < 4; ++i) {
        r = DevSumRow(i);
        assert(CtrSummaryUi_Hit(&sv, r.x + 80, r.y + 10) == -1 && at(r.x + 8, r.y + 10) != UiTypeColor(0));
    }
    sv = SumBoard(); sv.trainer = 1; DevCheckSummary(&so, &sv);

    sv.data.egg = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 96, 20) == -1 && CtrSummaryUi_Hit(&sv, 156, 20) == -1 && CtrSummaryUi_Hit(&sv, 216, 20) == SUI_HIT_TAB + 3);
    /* an egg: STATS and MOVES grey; its name, no gender, level or types; the memo and how soon it hatches */
    assert(at(kDevSumTab[1].x + 3, 20) == dimKey && at(kDevSumTab[2].x + 3, 20) == dimKey);
    AssertKey(kDevSumTab[0], DEV_LOOK_KEY, 0); AssertKey(kDevSumTab[3], DEV_LOOK_KEY, 1);
    assert(Count(20, 63, 98, 12, DEV_MALE) == 0 && Count(20, 154, 92, 12, fire) == 0);
    sv.pressed = SUI_HIT_TAB + 1; DevCheckSummary(&so, &sv); AssertKey(kDevSumTab[3], DEV_LOOK_KEY, 1);    /* a grey tab: not chosen */
    sv.pressed = -1; assert(DevCheckSummary(&so, &sv).count == 0);
    strcpy(sv.data.nickname, "EGG"); strcpy(sv.data.species, "EGG"); strcpy(sv.data.hatch, "It appears to move occasionally. It may be close to hatching.");
    DevCheckSummary(&so, &sv); assert(Count(126, 50, 172, 120, DEV_INK) > 0);
    shot("egg");
    sv.trainer = 0; DevCheckSummary(&so, &sv); assert(Count(126, 50, 172, 120, DEV_INK) > 0 && Count(20, 154, 92, 12, fire) == 0);
    shot("egg_info");
    sv.trainer = 1;

    sv.loading = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 36, 20) == -1 && CtrSummaryUi_Hit(&sv, 278, 209) == SUI_HIT_BACK);
    /* loading: the frame only, every tab and arrow grey, BACK taken */
    for (i = 0; i < 4; ++i) assert(at(kDevSumTab[i].x + 3, 20) == dimKey);
    assert(at(255, 20) == dimDark && at(287, 20) == dimDark); AssertKey(kDevSumBack, DEV_LOOK_DARK, 0);
    assert(Count(14, 46, 292, 128, DEV_INK) == 0 && Count(14, 194, 216, 32, DEV_INK) == 0);
    sv.pressed = SUI_HIT_BACK; DevCheckSummary(&so, &sv); AssertKey(kDevSumBack, DEV_LOOK_DARK, 1);
    sumArt = 0;
    assert(sDevKeyCut == 0);                      /* main prints PASS device summary */
}

/* ---- bag: the field Bag, a case, a mart's BUY, the item PC, the mailbox, the battle Bag (boards B · BAG) -------------- */

static int BagHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_BagHit((const CtrPokeTouchView *)view, x, y);
}

/* The board's Items pocket: the first eight of nine, FireRed's cursor on SUPER POTION (row 1) with its description; no
 * pocket asked for; the item's menu (USE GIVE TOSS REGISTER CANCEL, and CHECK before CANCEL for a menu of six) ready for
 * its state. Names as FireRed has them. */
static const struct { const char *name; int qty; } kBagItem[8] = {
    {"POTION", 5}, {"SUPER POTION", 2}, {"ANTIDOTE", 3}, {"PARLYZ HEAL", 1}, {"ESCAPE ROPE", 2}, {"REPEL", 4},
    {"FULL HEAL", 1}, {"MAX REPEL", 3},
};
/* The selection's description: port-written words, as long as a game description (three lines of the small font). */
static const char kBagDesc[] = "Port-written words for this test. They stand in for what a POK\xC3\xA9MON item says here.";

static CtrPokeTouchView BagBoard(void)
{
    static const char *const kMenu[6] = {"USE", "GIVE", "TOSS", "REGISTER", "CANCEL", "CANCEL"};
    CtrPokeTouchView v;
    CtrPokeTouchBag *b = &v.bag;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.optPressed = -1;
    b->on = 1;
    b->kind = PT_BAG_KIND_BAG;
    b->asked = 0xFF;
    b->state = PT_BAG_STATE_LIST;
    b->count = 9;
    b->cursor = 1;
    b->moveType = -1;
    b->pressed = -1;
    for (int i = 0; i < 8; ++i) {
        b->row[i].item = (uint16_t)(13 + i);
        b->row[i].qty = (uint16_t)kBagItem[i].qty;
        strcpy(b->row[i].name, kBagItem[i].name);
    }
    b->selItem = 14;
    strcpy(b->selName, "SUPER POTION");
    strcpy(b->selDesc, kBagDesc);
    for (int i = 0; i < 6; ++i)
        strcpy(b->menuName[i], kMenu[i]);
    return v;
}

/* The battle's Bag with the same rows: the Items pocket, no Berry Pouch; its menu USE, CANCEL. */
static CtrBattleView BagBattle(void)
{
    CtrBattleView v;
    CtrBattleBag *b = &v.bag;

    memset(&v, 0, sizeof(v));
    v.state = BUI_BAG;
    v.pressed = -1;
    v.cursor = -1;
    b->count = 9;
    b->cursor = 1;
    for (int i = 0; i < 8; ++i) {
        b->row[i].item = (uint16_t)(13 + i);
        b->row[i].qty = (uint16_t)kBagItem[i].qty;
        strcpy(b->row[i].name, kBagItem[i].name);
    }
    b->selItem = 14;
    strcpy(b->selName, "SUPER POTION");
    strcpy(b->selDesc, kBagDesc);
    strcpy(b->menuName[0], "USE");
    strcpy(b->menuName[1], "CANCEL");
    return v;
}

/* With small letters wider than capitals: `a` and `b` drawn alike (or not, `same` 0). */
static void BagSamePixels(const CtrPokeTouchView *a, const CtrPokeTouchView *b, int same)
{
    static uint16_t first[W * H];
    static uint8_t keepWidths[256], keepSmall[256];

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    CtrPokeTouch_Draw(canvas, a);
    memcpy(first, canvas, sizeof(first));
    CtrPokeTouch_Draw(canvas, b);
    if ((memcmp(first, canvas, sizeof(first)) == 0) != same) {
        fprintf(stderr, "%s: kind %d state %d, rows %s / %s: drawn %s\n", gSuite, a->bag.kind, a->bag.state,
                a->bag.row[0].name, b->bag.row[0].name, same ? "differently" : "alike");
        assert(0);
    }
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* Upper case (decision 5): FireRed's names in any case drawn in capitals (the rows, the selection, the menu, a case's
 * name, a TM's type, CLOSE's word); a mailbox's writers as typed. */
static void BagUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView other;

    v.bag.state = PT_BAG_STATE_ACTIONS;
    v.bag.menuCount = 5;
    other = v;
    strcpy(other.bag.row[0].name, "Potion");
    strcpy(other.bag.row[1].name, "Super Potion");
    strcpy(other.bag.selName, "Super Potion");
    strcpy(other.bag.menuName[0], "Use");
    strcpy(other.bag.menuName[4], "Cancel");
    BagSamePixels(&v, &other, 1);
    v.bag.state = other.bag.state = PT_BAG_STATE_LIST;
    v.bag.kind = other.bag.kind = PT_BAG_KIND_TMCASE;
    strcpy(v.bag.title, "TM CASE");
    strcpy(other.bag.title, "Tm Case");
    v.bag.moveType = other.bag.moveType = 4;
    strcpy(v.bag.typeName, "GROUND");
    strcpy(other.bag.typeName, "Ground");
    strcpy(v.bag.closeName, "CLOSE");
    strcpy(other.bag.closeName, "Close");
    BagSamePixels(&v, &other, 1);
    v.bag.kind = other.bag.kind = PT_BAG_KIND_ITEMPC;
    v.bag.moveType = other.bag.moveType = -1;
    v.bag.typed = other.bag.typed = 1;
    strcpy(v.bag.row[1].name, "MOM");
    strcpy(other.bag.row[1].name, "MOM");
    strcpy(v.bag.row[0].name, "Rick");
    strcpy(other.bag.row[0].name, "Rick");
    BagSamePixels(&v, &other, 1);
    strcpy(v.bag.row[0].name, "RICK");
    BagSamePixels(&v, &other, 0);
}

/* The plan's suite (every key hit where it is drawn, the cursor's row inverted, CLOSE sunk for the game's cursor on its
 * row, the item's menu, how many, YES / NO, a message, a case, a mart, the battle's Bag), the review's checks (a touch
 * chooses, what is not drawn, upper case, the shell, the ways in and out), then the states for the eye. */
static void SuiteBag(void)
{
    CtrPokeTouchView v = BagBoard(), old = v, home;
    CtrBattleView bv = BagBattle(), bo = bv;
    const uint16_t arrowOff = UI_RGB(0xB8, 0xB0, 0xA0);
    int i;

    CtrPokeTouch_Draw(canvas, &old);
    DevCheckTouch(&old, &v);
    /* the plan's suite */
    for (i = 0; i < 3; ++i) AssertHits(BagHitV, &v, kDevBagPocket[i], PT_BAG_HIT_TAB + i);
    AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 1);
    v.bag.asked = 2; DevCheckTouch(&old, &v); AssertKey(kDevBagPocket[2], DEV_LOOK_KEY, 1); AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 0); v.bag.asked = 0xFF;
    for (i = 0; i < 8; ++i) AssertRowHits(BagHitV, &v, DevBagRow(i, 0), PT_BAG_HIT_ROW + i);
    assert(at(20, 72) == DEV_BEZEL);                                                                  /* row 1, the cursor's, inverted */
    assert(CtrPokeTouch_BagHit(&v, 294, 73) == -1 && CtrPokeTouch_BagHit(&v, 294, 147) == PT_BAG_HIT_DOWN);   /* nothing above yet */
    AssertHits(BagHitV, &v, kDevBagClose, PT_BAG_HIT_CLOSE);
    DevCheckTouch(&old, &v);
    /* the board's pieces: the pocket's LED, the rows on the paper with their counts at the right, the cursor's row in
     * the paper's ink, ▲ grey and ▼ dark, the item's icon and description, CLOSE dark */
    assert(at(94, 15) == DEV_LED && at(200, 15) != DEV_LED && at(20, 56) == DEV_PAPER);
    assert(Count(232, 48, 22, 16, DEV_INK) > 0 && Count(232, 64, 22, 16, DEV_PAPER) > 0 && Count(40, 64, 60, 16, DEV_PAPER) > 0);
    assert(Count(290, 70, 9, 7, arrowOff) > 0 && Count(290, 144, 9, 7, DEV_KEY_INK) > 0);
    assert(at(22, 51) != DEV_PAPER && Count(18, 199, 24, 24, DEV_PAPER) < 24 * 24 && Count(46, 197, 180, 27, DEV_INK) > 0);
    AssertKey(kDevBagClose, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    shot("items");
    /* a touch chooses: a row (the cursor's not), ▼; ▲ cannot go up, so its touch draws nothing; what the page does not
     * draw (HOME's keys, the bar, BACK, the registered items outside Key Items, the quantity box's maximum) draws nothing */
    v.bag.pressed = PT_BAG_HIT_ROW + 3; DevCheckTouch(&old, &v); assert(at(20, 104) == DEV_BEZEL && at(20, 72) == DEV_PAPER);
    v.bag.pressed = PT_BAG_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevBagDown, DEV_LOOK_KEY, 1); assert(at(20, 72) == DEV_BEZEL);
    v.bag.pressed = PT_BAG_HIT_CLOSE; DevCheckTouch(&old, &v); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1); assert(at(20, 72) == DEV_PAPER);
    v.bag.pressed = PT_BAG_HIT_TAB + 1; DevCheckTouch(&old, &v); AssertKey(kDevBagPocket[1], DEV_LOOK_KEY, 1); AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 0);
    v.bag.pressed = -1; DevCheckTouch(&old, &v);
    v.bag.pressed = PT_BAG_HIT_UP; assert(DevCheckTouch(&old, &v).count == 0);
    v.bag.pressed = -1; strcpy(v.bar[0], "Items"); v.enabled[PT_BAG] = 1; v.backVisible = 1; 
    v.bag.regX = 13; v.bag.tossMax = 99; v.bag.titleItem = 7; strcpy(v.bag.prompt, "?"); v.bag.money = 500;
    assert(DevCheckTouch(&old, &v).count == 0);
    v.bag.prompt[0] = 0;
    v.shell = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_BLUE);
    v.shell = 0; DevCheckTouch(&old, &v);
    v.bag.cursor = v.bag.count; DevCheckTouch(&old, &v); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1);     /* the game's cursor on CLOSE BAG */
    assert(at(20, 72) == DEV_PAPER && Count(46, 197, 180, 27, DEV_INK) == 0);
    shot("close");
    v.bag.count = 0; v.bag.cursor = 0; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 138, 56) == -1); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1);
    assert(Count(16, 48, 244, 16, DEV_BEZEL) == 0 && Count(16, 48, 244, 128, DEV_INK2) > 0);            /* no rows: the pocket is empty */
    shot("empty");
    v.bag.count = 9; v.bag.cursor = 1;
    /* the item menu: USE GIVE TOSS REGISTER CANCEL */
    v.bag.state = PT_BAG_STATE_ACTIONS; v.bag.menuCount = 5; v.bag.menuCursor = 0; DevCheckTouch(&old, &v);
    for (i = 0; i < 5; ++i) AssertHits(BagHitV, &v, DevBagMenuKey(5, i), PT_BAG_HIT_MENU + i);
    AssertKey(DevBagMenuKey(5, 4), DEV_LOOK_DARK, 0); assert(CtrPokeTouch_BagHit(&v, 56, 20) == -1 && CtrPokeTouch_BagHit(&v, 196, 56) == -1);
    assert(at(11, 20) == DevBlend(DEV_SHELL_RED, DEV_SUNK, 128));                                     /* the pockets at half strength */
    /* the narrow list, its words grey but the cursor's row; USE sunk with its LED; the narrow screen says what is selected */
    AssertKey(DevBagMenuKey(5, 0), DEV_LOOK_KEY, 1); assert(at(300 + 2, 46 + 2) == DEV_LED && at(20, 72) == DEV_BEZEL);
    assert(Count(16, 48, 168, 16, DEV_INK_DIM) > 0 && Count(16, 48, 168, 128, DEV_INK) == 0 && at(195, 100) == DEV_SHELL_RED);
    assert(Count(46, 196, 136, 30, DEV_INK) > 0 && Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    assert(Count(46, 196, 2, 30, DEV_INK) == 0 && Count(48, 196, 2, 30, DEV_INK) > 0);      /* from x 48, the board's */
    v.bag.pressed = PT_BAG_HIT_MENU + 2; DevCheckTouch(&old, &v); AssertKey(DevBagMenuKey(5, 2), DEV_LOOK_KEY, 1); AssertKey(DevBagMenuKey(5, 0), DEV_LOOK_KEY, 0);
    v.bag.pressed = -1; DevCheckTouch(&old, &v);
    v.bag.pressed = PT_BAG_HIT_ROW + 2; assert(DevCheckTouch(&old, &v).count == 0);                 /* the rows take no touch now */
    v.bag.pressed = -1; strcpy(v.bag.menuName[5], "X"); assert(DevCheckTouch(&old, &v).count == 0); /* rows past the menu */
    shot("menu");
    v.bag.menuCount = 6; strcpy(v.bag.menuName[4], "CHECK"); strcpy(v.bag.menuName[5], "CANCEL");
    DevCheckTouch(&old, &v); AssertHits(BagHitV, &v, DevBagMenuKey(6, 5), PT_BAG_HIT_MENU + 5);
    AssertKey(DevBagMenuKey(6, 5), DEV_LOOK_DARK, 0); AssertKey(DevBagMenuKey(6, 4), DEV_LOOK_KEY, 0);
    shot("menu6");
    v.bag.state = PT_BAG_STATE_QUANTITY; DevCheckTouch(&old, &v);
    assert(CtrPokeTouch_BagHit(&v, 256, 55) == PT_BAG_HIT_QTY_UP && CtrPokeTouch_BagHit(&v, 256, 137) == PT_BAG_HIT_QTY_DOWN
           && CtrPokeTouch_BagHit(&v, 256, 173) == PT_BAG_HIT_QTY_OK && CtrPokeTouch_BagHit(&v, 256, 212) == PT_BAG_HIT_QTY_CANCEL);
    /* how many: the count on its screen, OK a key, CANCEL dark, the question; a touch sinks its key */
    v.bag.tossCount = 3; DevCheckTouch(&old, &v);
    AssertKey(kDevBagQtyUp, DEV_LOOK_KEY, 0); AssertKey(kDevBagQtyOk, DEV_LOOK_KEY, 0); AssertKey(kDevBagQtyCancel, DEV_LOOK_DARK, 0);
    assert(Count(206, 82, 100, 28, DEV_INK) > 0 && Count(46, 196, 136, 30, DEV_INK) > 0 && at(203, 79) == DEV_BEZEL);
    v.bag.pressed = PT_BAG_HIT_QTY_UP; DevCheckTouch(&old, &v); AssertKey(kDevBagQtyUp, DEV_LOOK_KEY, 1);
    v.bag.pressed = -1; DevCheckTouch(&old, &v);
    shot("quantity");
    /* selling: what that many fetch and the money */
    v.bag.use = PT_BAG_USE_SELL; v.bag.sale = 450; v.bag.money = 3000; DevCheckTouch(&old, &v); assert(Count(206, 96, 100, 12, DEV_INK2) > 0);
    v.bag.use = PT_BAG_USE_FIELD; v.bag.sale = 0; v.bag.money = 0;
    v.bag.state = PT_BAG_STATE_YESNO; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 256, 55) == PT_BAG_HIT_MENU && CtrPokeTouch_BagHit(&v, 256, 93) == PT_BAG_HIT_MENU + 1);
    /* YES / NO: two keys, YES the cursor's; the question on the narrow screen */
    strcpy(v.bag.prompt, "Throw away 3 of this item?"); DevCheckTouch(&old, &v);
    AssertKey(DevBagMenuKey(2, 0), DEV_LOOK_KEY, 1); AssertKey(DevBagMenuKey(2, 1), DEV_LOOK_KEY, 0); assert(Count(46, 196, 136, 30, DEV_INK) > 0);
    shot("yesno");
    v.bag.state = PT_BAG_STATE_MESSAGE; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 5, 5) == PT_BAG_HIT_MESSAGE && CtrPokeTouch_BagHit(&v, 315, 235) == PT_BAG_HIT_MESSAGE);
    /* a message: on the list's screen with its arrow; the pockets, the arrows and CLOSE grey, none sunk */
    strcpy(v.bag.prompt, "Threw away 3 SUPER POTIONS."); DevCheckTouch(&old, &v);
    assert(Count(14, 46, 248, 132, DEV_INK) > 0 && Count(14, 46, 248, 132, DEV_BEZEL) == 0 && at(11, 20) == DevBlend(DEV_SHELL_RED, DEV_SUNK, 128));
    assert(at(279, 73) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128) && at(247, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    v.bag.pressed = PT_BAG_HIT_MESSAGE; assert(DevCheckTouch(&old, &v).count == 0);
    v.bag.pressed = -1;
    shot("message");
    v.bag.state = PT_BAG_STATE_LIST; v.bag.kind = PT_BAG_KIND_TMCASE; strcpy(v.bag.title, "TM CASE"); DevCheckTouch(&old, &v);
    AssertKey(kDevBagCaseKey, DEV_LOOK_KEY, 1); assert(CtrPokeTouch_BagHit(&v, 160, 20) == -1);
    /* a TM: its type's chip before the description; HMs without a count */
    for (i = 0; i < 8; ++i) { snprintf(v.bag.row[i].name, sizeof(v.bag.row[i].name), "TM%02d DIG", 26 + i); v.bag.row[i].qty = (uint16_t)(i == 7 ? 0 : 1); }
    strcpy(v.bag.row[7].name, "HM01 CUT"); strcpy(v.bag.selName, "DIG"); strcpy(v.bag.closeName, "CLOSE");
    v.bag.moveType = 4; strcpy(v.bag.typeName, "GROUND"); DevCheckTouch(&old, &v);
    assert(Count(46, 197, 60, 10, UiTypeColor(4)) > 0 && Count(232, 160, 22, 16, DEV_INK) == 0 && Count(232, 144, 22, 16, DEV_INK) > 0);
    shot("tmcase");
    v.bag.kind = PT_BAG_KIND_POUCH; strcpy(v.bag.title, "BERRY POUCH"); v.bag.moveType = -1;
    for (i = 0; i < 8; ++i) { snprintf(v.bag.row[i].name, sizeof(v.bag.row[i].name), "%s BERRY", i & 1 ? "ORAN" : "CHERI"); v.bag.row[i].qty = (uint16_t)(1 + i); }
    strcpy(v.bag.selName, "ORAN BERRY"); DevCheckTouch(&old, &v); AssertKey(kDevBagCaseKey, DEV_LOOK_KEY, 1);
    shot("pouch");
    v.bag.kind = PT_BAG_KIND_SHOP; strcpy(v.bag.title, "POK\xC3\xA9 MART"); v.bag.money = 3000; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 160, 20) == -1);   /* the title */
    /* a mart: the title on a screen with the money at its right, prices at the rows' right, how many the Bag holds */
    for (i = 0; i < 8; ++i) { strcpy(v.bag.row[i].name, kBagItem[i].name); v.bag.row[i].qty = 0; v.bag.row[i].price = (uint16_t)(200 + 100 * i); }
    strcpy(v.bag.selName, "SUPER POTION"); v.bag.showOwned = 1; v.bag.owned = 2; strcpy(v.bag.closeName, "Cancel"); DevCheckTouch(&old, &v);
    assert(at(9, 20) == DEV_BEZEL && Count(250, 12, 56, 16, DEV_INK) > 0 && Count(222, 48, 32, 16, DEV_INK) > 0);
    assert(Count(46, 197, 60, 10, DEV_BLUE_INK) > 0);
    v.bag.state = PT_BAG_STATE_QUANTITY; v.bag.sale = 1400; DevCheckTouch(&old, &v); AssertKey(kDevBagQtyCancel, DEV_LOOK_DARK, 0);
    assert(at(11, 20) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));                                   /* the title grey */
    v.bag.state = PT_BAG_STATE_LIST; v.bag.sale = 0; DevCheckTouch(&old, &v);
    shot("shop");
    v.bag.kind = PT_BAG_KIND_ITEMPC; strcpy(v.bag.title, "WITHDRAW ITEM"); v.bag.money = 0; v.bag.showOwned = 0;
    for (i = 0; i < 8; ++i) { v.bag.row[i].price = 0; v.bag.row[i].qty = (uint16_t)kBagItem[i].qty; }
    DevCheckTouch(&old, &v); AssertHits(BagHitV, &v, kDevBagClose, PT_BAG_HIT_CLOSE);
    shot("itempc");
    strcpy(v.bag.title, "MAILBOX"); v.bag.typed = 1; v.bag.count = 3; v.bag.cursor = 0;
    for (i = 0; i < 3; ++i) { strcpy(v.bag.row[i].name, i ? "MOM" : "Rick"); v.bag.row[i].item = (uint16_t)(121 + i); v.bag.row[i].qty = 0; }
    strcpy(v.bag.selName, "Rick"); v.bag.selItem = 121; strcpy(v.bag.selDesc, "A MAIL to be held by a POK\xC3\xA9MON.");
    DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 138, 48 + 16 * 3 + 8) == -1 && CtrPokeTouch_BagHit(&v, 138, 48 + 8) == PT_BAG_HIT_ROW);
    shot("mailbox");
    BagUpperCase(BagBoard());
    /* the way in and out: HOME and the Bag, each whole */
    CtrPokeTouch_Draw(canvas, &old);
    home = BagBoard(); home.bag.on = 0; home.home = 1; home.playerX = home.playerY = home.cursorX = home.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &home)));
    v = BagBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    /* the battle Bag */
    CtrBattleUi_Draw(canvas, &bo);
    DevCheckBattle(&bo, &bv); AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 1);
    assert(CtrBattleUi_Hit(&bv, 56, 20) == -1 && CtrBattleUi_Hit(&bv, 160, 20) == -1);                 /* no Berry Pouch: no BERRIES */
    assert(at(160, 20) == DEV_SHELL_RED && at(264, 20) == DEV_SHELL_RED && at(20, 72) == DEV_BEZEL);
    shot("battle");
    bv.bag.berries = 365; DevCheckBattle(&bo, &bv); AssertHits(BatHitV, &bv, kDevBagPocket[1], BUI_BAG_BERRIES);
    AssertRowHits(BatHitV, &bv, DevBagRow(0, 0), BUI_BAG_ROW); AssertHits(BatHitV, &bv, kDevBagClose, BUI_BAG_CANCEL);
    /* BERRIES under a touch is the one sunk; the battle's own fields are not drawn; to the commands and back, whole */
    AssertKey(kDevBagPocket[1], DEV_LOOK_KEY, 0); assert(at(264, 20) == DEV_SHELL_RED);
    shot("battle_berries");
    bv.pressed = BUI_BAG_BERRIES; DevCheckBattle(&bo, &bv); AssertKey(kDevBagPocket[1], DEV_LOOK_KEY, 1); AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 0);
    bv.pressed = BUI_BAG_DOWN; DevCheckBattle(&bo, &bv); AssertKey(kDevBagDown, DEV_LOOK_KEY, 1); AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 1);
    bv.pressed = -1; DevCheckBattle(&bo, &bv);
    bv.activeHp = 7; bv.party[2] = 2; bv.cursor = 2; bv.showCursor = 1; bv.bag.bagState = 3; assert(DevCheckBattle(&bo, &bv).count == 0);
    strcpy(bv.bag.row[0].name, "Potion"); assert(DevCheckBattle(&bo, &bv).count == 0);           /* drawn as POTION all the same */
    strcpy(bv.bag.row[0].name, "POTION"); DevCheckBattle(&bo, &bv);
    bv.state = BUI_ACTION; assert(DevWhole(DevCheckBattle(&bo, &bv)));
    bv.state = BUI_BAG; assert(DevWhole(DevCheckBattle(&bo, &bv)));
    bv.bag.menuCount = 2; DevCheckBattle(&bo, &bv); AssertHits(BatHitV, &bv, DevBagMenuKey(2, 0), BUI_BAG_MENU); AssertHits(BatHitV, &bv, DevBagMenuKey(2, 1), BUI_BAG_MENU + 1);
    AssertKey(DevBagMenuKey(2, 1), DEV_LOOK_DARK, 0); assert(CtrBattleUi_Hit(&bv, 160, 20) == -1 && CtrBattleUi_Hit(&bv, 50, 60) == -1);
    bv.bag.menuCount = 0; strcpy(bv.bag.message, "..."); DevCheckBattle(&bo, &bv); assert(CtrBattleUi_Hit(&bv, 5, 5) == BUI_BAG_MESSAGE);
    assert(sDevKeyCut == 0);                      /* main prints PASS device bag */
}

/* The views without a board (plan 3), a suite header each. */
#include "device_suite_options.h"
#include "device_suite_save.h"
#include "device_suite_card.h"
#include "device_suite_mail.h"
#include "device_suite_dex.h"
#include "device_suite_choice.h"
#include "device_suite_learn.h"
#include "device_suite_relearn.h"
#include "device_suite_naming.h"
#include "device_suite_box.h"
#include "device_suite_easychat.h"
#include "device_suite_frame.h"           /* frame and idle */

/* ---- main --------------------------------------------------------------------------------------------------------- */

static const struct { const char *name; void (*run)(void); } kSuites[] = {
    {"kit", SuiteKit},
    {"home", SuiteHome},
    {"battle", SuiteBattle},
    {"party", SuiteParty},
    {"summary", SuiteSummary},
    {"bag", SuiteBag},
    {"options", SuiteOptions},
    {"save", SuiteSave},
    {"card", SuiteCard},
    {"mail", SuiteMail},
    {"dex", SuiteDex},
    {"choice", SuiteChoice},
    {"learn", SuiteLearn},
    {"relearn", SuiteRelearn},
    {"naming", SuiteNaming},
    {"box", SuiteBox},
    {"easychat", SuiteEasyChat},
    {"frame", SuiteFrame},
    {"idle", SuiteIdle},
};

int main(int argc, char **argv)
{
    const char *pack = getenv("FIRERED_TEST_PAK"), *stub = getenv("DEVICE_FONT_W");
    int all = argc > 1 && !strcmp(argv[1], "all"), ran = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: device_render_test <suite> (kit, home, battle, party, summary, bag, options, save, card, "
                        "mail, dex, choice, learn, relearn, naming, box, easychat, frame, idle, all)\n");
        return 2;
    }
    gOut = getenv("DEVICE_OUT");
    gTraceDir = getenv("DEVICE_TRACE");
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
            gTraceShot = 0;
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
