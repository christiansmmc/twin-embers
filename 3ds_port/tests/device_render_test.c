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
 *   FIRERED_TEST_PAK=pak      the pack (ABI 0x8f71cf3a, its ROM's SHA-1): its glyphs, item icons and art
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

/* ---- The pack (FIRERED_TEST_PAK) --------------------------------------------------------------------------------- */

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

/* ---- Game bridges: the battle's, the map's art, the item icons' files -------------------------------------------- */

/* An item icon's files as the kit's decoder checks set them (KitDecoders): while `on`, every icon's tiles are the first
 * `size` bytes of `tiles` and its palette `colors`, or (`fail`) the files are missing, pack or not. */
static struct {
    int on, fail;
    unsigned size;
    uint8_t tiles[288], colors[32];
} gIconFile;

/* The pack's files with FIRERED_TEST_PAK; else the two item icons, synthesised: a literal-only GBA LZ stream of 24x24
 * 4bpp tiles (3x3) and of a palette, so the full-size icon path runs without game data. */
void *CtrData_Load(const char *path, uint32_t *len)
{
    uint8_t raw[288];
    int palette = strstr(path, "icon_palettes/") != NULL;
    unsigned size = palette ? 32u : 288u;
    uint8_t *out;
    unsigned pos = 4;

    if (gIconFile.on && strstr(path, "graphics/items/icon")) {
        if (gIconFile.fail)
            return NULL;
        size = palette ? 32u : gIconFile.size;
        memcpy(raw, palette ? gIconFile.colors : gIconFile.tiles, size);
    } else if (realPack) {
        return PackRead(path, len);
    } else if (!strstr(path, "graphics/items/icon")) {
        return NULL;
    } else {
        memset(raw, 0, sizeof(raw));
        if (palette) {
            raw[2] = 0xff; raw[3] = 0x03;           /* index 1: green */
        } else {
            for (unsigned i = 0; i < 288; ++i)
                raw[i] = (uint8_t)((i % 32) < 16 ? 0x11 : 0x00);   /* stripes, so tile order shows */
            raw[0] = 0x21;                          /* the very first pixel: index 1, the second 2 */
        }
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
    DEV_STAT_UP, DEV_MALE, DEV_FEMALE, DEV_ARROW_OFF, DEV_BOX_TITLE_SH, DEV_CARD_GOLD, DEV_KEY_INK2, DEV_TAG_STAB,
    DEV_TAG_NEW,
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
            fprintf(f, ", \"look\": \"%s\", \"sunk\": %d, \"radius\": %d", kLook[a], b & 1, b >> 1);
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

static int Count(int x, int y, int w, int h, uint16_t c);

/* ...and on a dark or faint key, whose face is the same up and sunk, the LED: lit in its area when sunk, and not when up
 * (a cream key's area may show an LED while up: the naming keyboard's page key FireRed's cursor turns to). */
static __attribute__((unused)) void AssertKey(CtrPokeTouchRect r, int look, int sunk)
{
    int faceY = r.y + (sunk ? DEV_SINK : 0), areaH = r.y + r.h + DEV_DROP > H ? H - r.y : r.h + DEV_DROP;

    if (at(r.x + 3, faceY + r.h / 2) != KeyFace(look, sunk)) {
        fprintf(stderr, "%s: key {%d,%d,%d,%d} look %d sunk %d: face %04x at %d,%d, wanted %04x\n", gSuite, r.x, r.y, r.w,
                r.h, look, sunk, at(r.x + 3, faceY + r.h / 2), r.x + 3, faceY + r.h / 2, KeyFace(look, sunk));
        assert(0);
    }
    if (look != DEV_LOOK_KEY && (Count(r.x, r.y, r.w, areaH, DEV_LED) > 0) != sunk) {
        fprintf(stderr, "%s: key {%d,%d,%d,%d} look %d: LED %s, wanted %s\n", gSuite, r.x, r.y, r.w, r.h, look,
                sunk ? "dark" : "lit", sunk ? "sunk" : "up");
        assert(0);
    }
}

/* Sunk key r's LED lit as the kit's rule has it: a 4 px disc at (x + w - 11, its face's top + 4) whatever the key's
 * height, the corners of its box not lit. */
static __attribute__((unused)) int LedAt(CtrPokeTouchRect r)
{
    int x = r.x + r.w - 11, y = r.y + DEV_SINK + 4;

    for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i)
            if ((at(x + i, y + j) == DEV_LED) != !((i == 0 || i == 3) && (j == 0 || j == 3)))
                return 0;
    return 1;
}

/* Two rectangles alike. */
static __attribute__((unused)) int Same(CtrPokeTouchRect a, CtrPokeTouchRect b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
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
    for (int s = 0; s < 4; ++s) {                               /* four screws, always */
        DevBegin(canvas, (uint8_t)s); DevShell();
        assert(at(160, 120) == kShell[s] && at(160, 0) == DevBlend(kShell[s], 0xFFFF, 64) && at(160, 1) == at(160, 0) && at(160, 2) == kShell[s]);
        assert(at(160, 237) == DevBlend(kShell[s], 0, 77) && at(160, 239) == at(160, 237) && at(160, 236) == kShell[s]);
        assert(at(4, 4) == DevBlend(kShell[s], 0, 90) && at(315, 4) == at(4, 4) && at(4, 234) == at(4, 4) && at(315, 234) == at(4, 4));
    }
    DevBegin(canvas, 0); DevShell();
    p = DevScreen(&(CtrPokeTouchRect){8, 8, 304, 140});
    assert(p.x == 14 && p.y == 14 && p.w == 292 && p.h == 128 && at(8, 70) == DEV_BEZEL_LINE && at(9, 70) == DEV_BEZEL
           && at(13, 70) == DEV_BEZEL && at(14, 70) == DEV_PAPER && at(8, 8) == DEV_SHELL_RED && at(160, 148) == DevBlend(DEV_SHELL_RED, 0xFFFF, 64));
    k = (CtrPokeTouchRect){8, 156, 96, 36};                     /* HOME's POKeDEX key, on a fresh shell each time */
    DevShell();
    assert(DevKey(&k, DEV_LOOK_KEY, 0, 6) == 156 && at(11, 174) == DEV_KEY && at(56, 156) == 0xFFFF && at(56, 190) == DEV_KEY_EDGE
           && at(56, 191) == DEV_KEY_EDGE && at(56, 192) == DevBlend(DEV_SHELL_RED, 0, 97) && at(56, 194) == at(56, 192) && at(56, 195) == DEV_SHELL_RED && at(8, 156) == DEV_SHELL_RED);
    DevShell();
    assert(DevKey(&k, DEV_LOOK_KEY, 1, 6) == 158 && at(56, 157) == DEV_SHELL_RED && at(56, 158) == DEV_SUNK_EDGE && at(11, 176) == DEV_SUNK
           && at(56, 194) == DevBlend(DEV_SHELL_RED, 0, 97) && at(56, 195) == DEV_SHELL_RED);
    /* the LED: a 4 px disc (no corner pixels) at (x + w - 11, faceY + 4), whatever the key's height */
    assert(at(94, 162) == DEV_LED && at(95, 165) == DEV_LED && at(93, 162) != DEV_LED && at(92, 163) != DEV_LED && at(97, 164) != DEV_LED);
    k = (CtrPokeTouchRect){8, 8, 304, 84}; DevShell(); DevKey(&k, DEV_LOOK_KEY, 1, 6); assert(at(302, 14) == DEV_LED && at(303, 17) == DEV_LED && at(300, 15) != DEV_LED);
    k = (CtrPokeTouchRect){244, 190, 68, 40}; DevShell(); DevKey(&k, DEV_LOOK_DARK, 1, 6); assert(at(302, 196) == DEV_LED);
    k = (CtrPokeTouchRect){8, 156, 96, 36};
    DevShell();
    assert(DevKey(&k, DEV_LOOK_DARK, 0, 6) == 156 && at(11, 174) == DEV_BEZEL && at(56, 156) == DEV_BEZEL_LINE);
    DevShell();
    assert(DevKey(&k, DEV_LOOK_FAINT, 0, 6) == 156 && at(11, 174) == DEV_FAINT && at(56, 190) == DEV_FAINT_EDGE);
    assert(DevKeyInk(DEV_LOOK_FAINT, 0) == UI_RGB(0x8A, 0x84, 0x78));    /* an unused key's beige ink */
    DevShell();
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
    {
        /* a footer's small text that needs more lines than it is given is cut, and counted */
        static const char kLongWords[] = "Port-written words, more than one small line of this footer's paper can hold at once.";

        sDevKeyCut = 0; assert(DevSmallFit(kLongWords, 20, 20, 198, 224, 1, DEV_SMALL_LINE, DEV_INK) == 1 && sDevKeyCut == 1); sDevKeyCut = 0;
    }
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

/* Every piece on the red shell, for the eye (kit_sheet): the screen with its text, rows, icons, marks, bars and chips;
 * every key look up and sunk, a dimmed key, a hole, small keys (a tab, arrows, a pill). */
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
    DevShell();
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
    /* Chips (x 240..304). */
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

/* An item icon's files for the decoders (gIconFile): `size` bytes of clear tiles and a black palette. */
static void IconFiles(unsigned size)
{
    memset(&gIconFile, 0, sizeof(gIconFile));
    gIconFile.on = 1;
    gIconFile.size = size;
}

/* The decoders of FireRed's item icons. The pack's LZ77 (0x10): a known block decodes to its bytes (literals, a
 * back-reference), a block cut short, too big for the room or referring outside what is decoded is refused. The soft
 * 16x16 of HOME's keys (PokeTouch_SoftIcon): coverage by area, the colour of the drawn pixels only; none for an icon
 * missing, malformed or with nothing drawn. With FIRERED_TEST_PAK the pack's six HOME icons. */
static void KitDecoders(void)
{
    static const uint8_t literals[] = {0x10, 3, 0, 0, 0, 1, 2, 3};
    static const uint8_t repeated[] = {0x10, 6, 0, 0, 0x10, 1, 2, 3, 0, 2};   /* 1 2 3, then 3 back for 3 */
    static const uint8_t before[] = {0x10, 3, 0, 0, 0x80, 0, 0};              /* 1 back with nothing decoded */
    static const uint8_t overrun[] = {0x10, 2, 0, 0, 0x40, 1, 0, 0};          /* 3 bytes where 1 is left */
    static PtSoft soft;
    uint8_t out[512];

    assert(PokeTouch_Lz77(literals, sizeof(literals), out, 3) == 3 && !memcmp(out, "\1\2\3", 3));
    for (unsigned n = 0; n < sizeof(literals); ++n)
        assert(!PokeTouch_Lz77(literals, n, out, 3));
    assert(!PokeTouch_Lz77(literals, sizeof(literals), out, 2));
    assert(PokeTouch_Lz77(repeated, sizeof(repeated), out, 6) == 6 && !memcmp(out, "\1\2\3\1\2\3", 6));
    assert(!PokeTouch_Lz77(before, sizeof(before), out, sizeof(out)));
    assert(!PokeTouch_Lz77(overrun, sizeof(overrun), out, sizeof(out)));
    assert(!strcmp(sMenuItemArt[3], "card_key") && !strcmp(sMenuItemArt[5], "teachy_tv"));
    IconFiles(287);
    assert(!PokeTouch_SoftIcon("fame_checker", &soft));                       /* neither 3x3 nor 4x4 tiles */
    IconFiles(288);
    gIconFile.colors[2] = 0xff;                                                /* index 1: white */
    gIconFile.colors[3] = 0x7f;
    memset(gIconFile.tiles, 0x11, 288);
    assert(PokeTouch_SoftIcon("fame_checker", &soft) && soft.ok);
    for (int i = 0; i < PT_SOFT * PT_SOFT; ++i)
        assert(soft.alpha[i] == 255 && soft.rgb[i] == 0xffff);
    /* opaque left of x = 11 in every row: output columns 0-6 whole (to 10.5), 7 a third, 8 on none */
    memset(gIconFile.tiles, 0, 288);
    for (int tr = 0; tr < 3; ++tr)
        for (int row = 0; row < 8; ++row) {
            memset(&gIconFile.tiles[(tr * 3 + 0) * 32 + row * 4], 0x11, 4);
            gIconFile.tiles[(tr * 3 + 1) * 32 + row * 4 + 0] = 0x11;
            gIconFile.tiles[(tr * 3 + 1) * 32 + row * 4 + 1] = 0x01;
        }
    assert(PokeTouch_SoftIcon("fame_checker", &soft));
    assert(soft.alpha[5] == 255 && soft.alpha[7] > 70 && soft.alpha[7] < 100 && soft.alpha[8] == 0 && soft.alpha[15] == 0);
    assert(soft.rgb[7] == 0xffff);                                            /* the colour, not a darker mix */
    memset(gIconFile.tiles, 0, 288);
    assert(!PokeTouch_SoftIcon("fame_checker", &soft));                       /* nothing drawn */
    gIconFile.fail = 1;
    assert(!PokeTouch_SoftIcon("fame_checker", &soft));                       /* missing */
    memset(&gIconFile, 0, sizeof(gIconFile));
    for (unsigned i = 0; realPack && i < 6; ++i)
        assert(PokeTouch_SoftIcon(sMenuItemArt[i], &soft) && soft.ok);
}

static void SuiteKit(void)
{
    KitDecoders();
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
 * Pokemon and a hole (HP 100, 66, 34, 0 and 8 %), the play time 12:34, the speed at 2x, the map's art there, another
 * map open; no player on the map, no destination. Strings as FireRed has them: no casing (the B views are upper
 * case). */
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
    v.saveHours = 12;
    v.saveMinutes = 34;
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

/* The leftmost x in {x0, y0, w, h} of colour a and the rightmost of colour b: a pair's margins in its key (-1 for a
 * colour not there). */
static void InkSpan(int x0, int y0, int w, int h, uint16_t a, uint16_t b, int *left, int *right)
{
    *left = *right = -1;
    for (int x = x0; x < x0 + w; ++x)
        for (int y = y0; y < y0 + h; ++y) {
            if (at(x, y) == a && *left < 0)
                *left = x;
            if (at(x, y) == b)
                *right = x;
        }
}

/* The suite as the plan gives it (HOME's eight keys hit where they are drawn, RUN's LED lit and dim, the play time, the
 * map at 1:1, every cell of every map in the window, the name window inside it, a blink or a step presents only the
 * window), the review's checks (upper case, the name window clear of the region key and FLY, the Fly reasons under a
 * long name, an egg), then the states for the eye. */
static void SuiteHome(void)
{
    static const int corner[5][2] = {{0, 0}, {21, 0}, {0, 14}, {21, 14}, {10, 7}};
    static uint16_t tm[30 * 20], btm[30 * 20], gba[MAP_COLORS];
    static uint8_t tl[MAP_TILES * 32], btl[MAP_TILES * 32];
    CtrPokeTouchView v, old;
    CtrPokeTouchRect r, keyArea = DevKeyArea(&kDevHomeMapKey), flyArea = DevKeyArea(&kDevHomeFly);
    CtrDirtyList d;
    int i, id, lit, x, y, gx, gy, cx, cy, vx, vy, dx, dy, g, left, right;
    const int flyX = kDevHomeFly.x + kDevHomeFly.w / 2, flyY = kDevHomeFly.y + kDevHomeFly.h / 2;

    richArt = 1;
    iconArt = 1;
    sPlayer.key = -1;
    assert(CtrPokeTouch_MapPrepare(PT_MAP_KANTO, 0));
    v = HomeBoard();
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    shot("plain");
    /* the eight keys (the board's [8|85|162|239, 154|196, 73, 34]): each hit on its face and drop, up */
    for (id = PT_POKEDEX; id <= PT_OPTIONS; ++id) {
        AssertHits(HomeHitV, &v, kDevHomeKey[id], id);
        AssertKey(kDevHomeKey[id], DEV_LOOK_KEY, 0);
    }
    AssertHits(HomeHitV, &v, kDevHomeRun, PT_RUN); AssertHits(HomeHitV, &v, kDevHomeSpeed, PT_HOME_SPEED);
    AssertKey(kDevHomeRun, DEV_LOOK_KEY, 0); AssertKey(kDevHomeSpeed, DEV_LOOK_KEY, 0);
    /* the party's cells, each hit on itself where it holds a Pokemon; a hole, TIME and the window take nothing */
    for (i = 0; i < 6; ++i) if (v.party[i]) AssertRowHits(HomeHitV, &v, DevHomeCell(i), PT_HOME_PARTY + i);
    assert(CtrPokeTouch_HomeHit(&v, 82, 174) == PT_NONE && at(82, 174) == DEV_SHELL_RED);        /* the gap */
    assert(CtrPokeTouch_HomeHit(&v, 287, 103) == PT_NONE && CtrPokeTouch_HomeHit(&v, 232, 131) == PT_NONE
           && CtrPokeTouch_HomeHit(&v, 100, 80) == PT_NONE);
    assert(CtrPokeTouch_HomeWindowPoint(14, 14, &vx, &vy) && vx == 0 && vy == 0 && CtrPokeTouch_HomeWindowPoint(205, 141, &vx, &vy)
           && vx == 191 && vy == 127 && !CtrPokeTouch_HomeWindowPoint(206, 80, &vx, &vy) && !CtrPokeTouch_HomeWindowPoint(13, 80, &vx, &vy));
    /* the panel as the board has it: the name's capitals at 19, the rule at 57, the cells' bars [214 + 30c, 85 + 32r, 22,
     * 4], the hole's ring [277, 96, 16, 16], TIME's capitals at 130 */
    assert(Count(212, 16, 90, 3, DEV_INK) == 0 && Count(212, 19, 90, 1, DEV_INK) > 0);
    assert(at(210, 57) == DEV_RULE && at(301, 57) == DEV_RULE && at(210, 56) == DEV_PAPER && at(210, 58) == DEV_PAPER);
    assert(at(214, 85) == DEV_HP_FRAME && at(235, 88) == DEV_HP_FRAME && at(213, 86) == DEV_PAPER && at(236, 86) == DEV_PAPER
           && at(274, 117) == DEV_PAPER && at(244, 117) == DEV_HP_FRAME);                       /* cell 4's bar; 5 a hole */
    assert(at(284, 96) == DEV_HOME_HOLE && at(277, 104) == DEV_HOME_HOLE && at(284, 104) == DEV_PAPER && at(284, 95) == DEV_PAPER);
    assert(Count(212, 124, 90, 6, DEV_INK2) == 0 && Count(212, 130, 3, 7, DEV_INK2) > 0);
    /* RUN: the word and its LED, the pair centred, the LED's top at faceY + 14; lit while running, else a dim disc */
    v.running = 1; DevCheckTouch(&old, &v); lit = Count(239, 154, 73, 37, DEV_KEY_EDGE); assert(Count(239, 154, 73, 37, DEV_LED) >= 20);
    assert(Count(239, 168, 73, 6, DEV_LED) == Count(239, 154, 73, 37, DEV_LED));
    InkSpan(239, 154, 73, 34, DEV_KEY_INK, DEV_LED, &left, &right);
    assert(left > 239 && right > left && abs((left - 239) - (311 - right)) <= 1);
    v.running = 0; DevCheckTouch(&old, &v); assert(Count(239, 154, 73, 37, DEV_LED) == 0 && Count(239, 154, 73, 37, DEV_KEY_EDGE) - lit >= 20);  /* the dim disc */
    v.pressed = PT_RUN; DevCheckTouch(&old, &v); AssertKey(kDevHomeRun, DEV_LOOK_KEY, 1); assert(LedAt(kDevHomeRun)); v.pressed = PT_NONE;
    /* SPD: its label small in the key's second ink, 6 px, the multiplier, the pair centred; sunk under a touch */
    InkSpan(239, 196, 73, 34, DEV_KEY_INK2, DEV_KEY_INK, &left, &right);
    assert(left > 239 && right > left && abs((left - 239) - (311 - right)) <= 2);
    v.pressed = PT_HOME_SPEED; DevCheckTouch(&old, &v); AssertKey(kDevHomeSpeed, DEV_LOOK_KEY, 1); assert(LedAt(kDevHomeSpeed));
    v.pressed = PT_NONE; DevCheckTouch(&old, &v);
    /* TIME: FireRed's play clock, a minute later drawn whole */
    v.saveMinutes = 33; DevCheckTouch(&old, &v);
    v.saveMinutes = 34; assert(DevWhole(DevCheckTouch(&old, &v))); assert(Count(212, 130, 90, 7, DEV_INK2) > 0);
    v.saveHours = 13; assert(DevWhole(DevCheckTouch(&old, &v))); v.saveHours = 12; DevCheckTouch(&old, &v);
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
    assert(LedAt(kDevHomeKey[1]));
    v.pressed = PT_NONE;
    v.enabled[PT_SAVE] = 0;
    DevCheckTouch(&old, &v);
    assert(at(88, 213) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
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
    /* every entry held and let go, not usable and usable again; RUN on, another name; FireRed's picture and back: each
     * an Update equal to a full Draw */
    for (i = 0; i < PT_COUNT; ++i) {
        v.pressed = (uint8_t)i;
        DevCheckTouch(&old, &v);
        v.pressed = PT_NONE;
        DevCheckTouch(&old, &v);
        v.enabled[i] = 0;
        DevCheckTouch(&old, &v);
        v.enabled[i] = 1;
        DevCheckTouch(&old, &v);
    }
    v.running = 1;
    DevCheckTouch(&old, &v);
    strcpy(v.playerName, "ALEX");
    DevCheckTouch(&old, &v);
    v.home = 0;
    v.viewport = 1;
    DevCheckTouch(&old, &v);
    v.home = 1;
    v.viewport = 0;
    DevCheckTouch(&old, &v);

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
    assert(at(245, 117) == DEV_HP_FRAME && at(276, 117) == DEV_PAPER && at(287, 117) == DEV_PAPER);
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
    assert(at(242, 171) == DevBlend(DEV_SHELL_GREEN, DEV_KEY, DEV_A_DIM) && at(242, 213) == at(242, 171));   /* RUN, SPD */
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
    strcpy(v.move[0].desc, "Port-written words in place of what a move does.");
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
    assert(Count(76, 17, 140, 12, DEV_INK) > 0 && Count(20, 76, 280, 15, DEV_INK) > 0);
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
 * pieces where the boards put them, then the states for the eye. The commands are def_fight's (owner O1): FIGHT across,
 * under it a trainer's three keys of 96 (the board) or a wild battle's four of 73 with the quick ball, at y 168. */
static void SuiteBattle(void)
{
    static const int kWild[4] = {BUI_ACT_BAG, BUI_ACT_BALL, BUI_ACT_POKEMON, BUI_ACT_RUN};
    static const int kTrainer[3] = {BUI_ACT_BAG, BUI_ACT_POKEMON, BUI_ACT_RUN};
    CtrBattleView v = BatBoard(), old;
    CtrPokeTouchRect prev, next;
    int i, x, y, fightX;

    old = v;
    CtrBattleUi_Draw(canvas, &old);
    v.state = BUI_ACTION; v.cursor = 0; DevCheckBattle(&old, &v);            /* a trainer: three keys */
    AssertHits(BatHitV, &v, kDevBatFight, BUI_ACT_FIGHT); AssertKey(kDevBatFight, DEV_LOOK_KEY, 1); assert(LedAt(kDevBatFight));
    for (i = 0; i < 3; ++i) AssertHits(BatHitV, &v, kDevBatRow3[i], kTrainer[i]);
    assert(CtrBattleUi_Hit(&v, 108, 200) == -1 && CtrBattleUi_Hit(&v, 160, 50) == -1 && CtrBattleUi_Hit(&v, 160, 163) == -1);
    for (i = 0; i < 3; ++i)
        AssertKey(kDevBatRow3[i], DEV_LOOK_KEY, 0);
    /* the screen: the lead's icon at 2x (its frame 0 from (4, 8)), the balls, HP, the prompt */
    assert(BPartyIcon(0, v.activeIcon) && at(20, 18) == sBPIcon[0].pal[sBPIcon[0].pixels[8 * 32 + 4]] && at(21, 19) == at(20, 18));
    assert(at(40, 30) == DEV_PAPER);                                          /* the stub icon's hole */
    /* the board's balls, 9 across at (230 + 12i, 20): able, able, a status, fainted, able, none (a ring) */
    assert(at(234, 24) == DEV_INK && at(246, 24) == DEV_INK && at(258, 24) == UI_RGB(0xE0, 0xB0, 0x20)
           && at(270, 24) == UI_RGB(0xC8, 0xC8, 0xC0) && at(282, 24) == DEV_INK && at(230, 24) == DEV_INK && at(229, 24) == DEV_PAPER);
    assert(at(294, 20) == UI_RGB(0xB0, 0xB0, 0xA8) && at(294, 24) == DEV_PAPER);   /* none: a ring */
    /* HP: the label (76..87), a pixel of paper, the bar [89,54,160,6] (the board's) */
    assert(at(76, 52) == DEV_HP_FRAME && at(87, 60) == DEV_HP_FRAME && Count(88, 52, 1, 9, DEV_HP_FRAME) == 0 && Count(89, 54, 160, 6, DEV_HP_FRAME) > 0 && at(89, 54) == DEV_HP_FRAME && at(89, 53) == DEV_PAPER
           && at(89 + 80, 57) == DEV_HP_GREEN && at(89 + 159, 59) == DEV_HP_FRAME && at(89 + 160, 57) == DEV_PAPER && at(169, 60) == DEV_PAPER);
    /* the prompt, its capitals at 79 (the board's line at 76); the name */
    assert(Count(20, 79, 280, 1, DEV_INK) > 0 && Count(20, 64, 280, 13, DEV_INK) == 0 && Count(20, 76, 280, 15, DEV_INK) > 0);
    assert(Count(76, 17, 140, 12, DEV_INK) > 0);
    assert(Count(76, 33, 8, 10, DEV_MALE) > 0);                                           /* the gender mark */
    /* FIGHT [8,108,304,48]: the crossed swords, 22x22, 6 px left of the word, the pair centred on the key (sunk: face at
     * 110); blended at their edges, nothing between them and the word */
    fightX = 8 + (304 - (22 + 6 + UiTextWidth(CTR_TR("FIGHT", "LUCHAR", "KAMPF"), 2))) / 2;
    assert(Count(fightX, 123, 22, 22, UI_RGB(0xA0, 0x2A, 0x20)) > 20 && Count(fightX + 22, 110, 6, 48, UI_RGB(0xA0, 0x2A, 0x20)) == 0);
    assert(Count(fightX, 123, 22, 22, DEV_SUNK) + Count(fightX, 123, 22, 22, UI_RGB(0xA0, 0x2A, 0x20)) < 22 * 22);
    assert(Count(fightX + 28, 125, 40, 18, UI_RGB(0xA0, 0x2A, 0x20)) > 0 && Count(8, 110, fightX - 8, 48, UI_RGB(0xA0, 0x2A, 0x20)) == 0);
    assert(at(fightX + 4, 127) == UI_RGB(0xA0, 0x2A, 0x20) && at(fightX + 17, 127) == UI_RGB(0xA0, 0x2A, 0x20));   /* the blades */
    shot("action");
    v.runBlocked = 1; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 260, 200) == -1 && at(219, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    shot("action_runblocked");
    v.runBlocked = 0; v.ball = CTR_QUICK_BALL_READY; v.ballCount = 4; v.ballItem = 4; v.ballQty = 21; DevCheckBattle(&old, &v);   /* wild: four keys of 73 */
    AssertHits(BatHitV, &v, kDevBatFight, BUI_ACT_FIGHT);
    for (i = 0; i < 4; ++i) if (i != 1) AssertHits(BatHitV, &v, kDevBatRow4[i], kWild[i]);
    AssertHits(BatHitV, &v, kDevBatRow4[1], kWild[1]);         /* the ball's lower corner is under its arrow's slop */
    prev = DevBatBallArrow(&kDevBatRow4[1], 0);
    next = DevBatBallArrow(&kDevBatRow4[1], 1);
    assert(CtrBattleUi_Hit(&v, prev.x + 10, prev.y + 10) == BUI_ACT_BALL_PREV && CtrBattleUi_Hit(&v, next.x + 10, next.y + 10) == BUI_ACT_BALL_NEXT);
    assert(CtrBattleUi_Hit(&v, 121, 180) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, 160, 233) == -1);
    assert(CtrBattleUi_Hit(&v, 97, 218) == BUI_ACT_BALL_PREV && CtrBattleUi_Hit(&v, 151, 218) == BUI_ACT_BALL_NEXT
           && CtrBattleUi_Hit(&v, 44, 200) == BUI_ACT_BAG && CtrBattleUi_Hit(&v, 198, 200) == BUI_ACT_POKEMON && CtrBattleUi_Hit(&v, 275, 200) == BUI_ACT_RUN);
    /* an arrow answers 3 px round itself, on its key */
    assert(CtrBattleUi_Hit(&v, prev.x - 2, prev.y - 3) == BUI_ACT_BALL_PREV && CtrBattleUi_Hit(&v, next.x + next.w + 1, next.y + next.h + 2) == BUI_ACT_BALL_NEXT);
    assert(CtrBattleUi_Hit(&v, prev.x + prev.w + 3, prev.y) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, prev.x, prev.y - 4) == BUI_ACT_BALL);
    assert(Count(prev.x, prev.y, prev.w, prev.h, DEV_KEY_INK) > 0 && Count(next.x, next.y, next.w, next.h, DEV_KEY_INK) > 0);
    shot("action_ball");
    v.runBlocked = 1; DevCheckBattle(&old, &v);                                  /* wild, no running: RUN of 73 grey, not hit */
    assert(CtrBattleUi_Hit(&v, 275, 200) == -1 && at(242, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128) && at(211, 200) == DEV_KEY);
    v.runBlocked = 0;
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
    v.shell = 1; assert(DevWhole(DevCheckBattle(&old, &v)) && at(160, 104) == DEV_SHELL_BLUE);
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
    /* every key dimmed and up (none sunk): an up face's middle row, no LED */
    assert(at(kDevBatFight.x + 3, kDevBatFight.y + 24) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128) && !LedAt(kDevBatFight));
    for (i = 0; i < 4; ++i)
        assert(at(kDevBatRow4[i].x + 3, kDevBatRow4[i].y + 31) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128) && !LedAt(kDevBatRow4[i]));
    assert(Count(20, 76, 280, 15, DEV_INK) == 0 && Count(76, 17, 140, 12, DEV_INK) > 0);   /* no prompt; the lead still */
    shot("idle");
    v.cursor = 3; v.pressed = 2; assert(DevCheckBattle(&old, &v).count == 0);             /* idle: no key chosen */
    v.ball = CTR_QUICK_BALL_NONE; v.ballCount = 0; DevCheckBattle(&old, &v);                /* a trainer: three keys */
    assert(at(108, 200) == DEV_SHELL_RED && at(219, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    for (i = 0; i < 3; ++i)
        assert(at(kDevBatRow3[i].x + 3, kDevBatRow3[i].y + 31) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    v.safari = 1; assert(DevWhole(DevCheckBattle(&old, &v)));                              /* the Safari Zone: its own keys */
    for (y = 0; y < 240; y += 3) for (x = 0; x < 320; x += 3) assert(CtrBattleUi_Hit(&v, x, y) == -1);
    for (i = 0; i < 4; ++i)
        assert(at(kDevBatMove[i].x + 3, kDevBatMove[i].y + 31) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    assert(Count(14, 14, 292, 44, DEV_INK) == 0 && at(11, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
    shot("idle_safari");
    assert(sDevKeyCut == 0);
}

/* ---- party: the bays, the action menu, YES / NO, a message, the level up, picking several (boards B · POKeMON) -------- */

/* The board's Party (def_party): CHARMELEON ♂ 112/112 Lv36 (the cursor), PIDGEOTTO ♀ 62/94 Lv31, RATICATE ♂ 24/70
 * poisoned, NIDORINO ♂ 0/76 fainted, ONIX ♂ 5/64 Lv25 and a hole; the menu's rows the board's (SUMMARY, SWITCH, ITEM,
 * CUT, FLY, CANCEL) and two more for a menu of eight. Names as FireRed has them, each its species' own. */
static CtrBattleView PtyBoard(void)
{
    static const struct {
        const char *name;
        int level, hp, max, status, gender;
    } kMon[5] = {{"CHARMELEON", 36, 112, 112, 0, 0}, {"PIDGEOTTO", 31, 62, 94, 0, 1}, {"RATICATE", 28, 24, 70, 1, 0},
                 {"NIDORINO", 30, 0, 76, 0, 0}, {"ONIX", 25, 5, 64, 0, 0}};
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
        p->gender[i] = (uint8_t)kMon[i].gender;
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
    int i, k;
    uint16_t dim = DevBlend(DEV_SHELL_RED, DEV_KEY, 128);

    CtrBattleUi_Draw(canvas, &old);
    DevCheckBattle(&old, &v);
    /* the plan's suite (def_party, bay 0 sunk): the bays and CANCEL, the dark key in the exit corner, hit where they are
     * drawn; the hole not hit */
    for (i = 0; i < 5; ++i) AssertHits(BatHitV, &v, DevPtyBay(i), BUI_PTY_SLOT + i);
    AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_CANCEL); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(CtrBattleUi_Hit(&v, 238, 145) == -1 && at(238, 150) == DevBlend(DEV_SHELL_RED, 0, 72));      /* the hole */
    AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 1); AssertKey(DevPtyBay(3), DEV_LOOK_FAINT, 0);
    assert(at(47, 41) == DEV_HP_GREEN && at(46, 41) == DEV_HP_FRAME);                                   /* the bar from x+38 */
    assert(Count(8, 8, 304, 170, DEV_HP_LABEL) == 0);                                                   /* no "HP" tag on any bay */
    assert(Count(46, 18, 110, 11, DEV_MALE) > 0 && Count(202, 14, 110, 11, DEV_FEMALE) > 0);           /* CHARMELEON ♂, PIDGEOTTO ♀ */
    assert(Count(14, 196, 216, 30, DEV_INK) > 0 && Count(8, 180, 304, 9, DEV_INK) == 0);               /* the words on the footer */
    /* the board's pieces: the chosen bay's LED; RATICATE's PSN (10 tall from faceY + 38) and NIDORINO's FNT at the right;
     * NIDORINO's bar framed as any (no grey frame), its numbers in the faint ink; CHARMELEON's bar 56 wide; the level
     * small in DEV_KEY_INK2 (capitals faceY + 39), ending 8 px in; ONIX's numbers in the low ink; CANCEL's word */
    assert(LedAt(DevPtyBay(0)) && at(128, 105) == DEV_PSN && at(284, 105) == DEV_FNT);
    assert(at(146, 102) == DEV_PSN && at(146, 111) == DEV_PSN && at(146, 101) != DEV_PSN && at(146, 112) != DEV_PSN);
    assert(at(202, 93) == DEV_HP_FRAME && at(101, 41) == DEV_HP_FRAME && at(100, 41) == DEV_HP_GREEN && at(102, 41) != DEV_HP_FRAME);
    assert(Count(258, 91, 46, 7, DEV_FAINT_INK) > 0);
    assert(Count(110, 49, 38, 7, DEV_KEY_INK2) > 0 && Count(110, 46, 38, 3, DEV_KEY_INK2) == 0 && Count(148, 40, 8, 20, DEV_KEY_INK2) == 0);
    assert(Count(100, 145, 48, 9, DEV_LOW_INK) > 0 && Count(120, 33, 28, 9, DEV_LOW_INK) == 0);
    assert(Count(46, 19, 90, 8, DEV_SUNK_INK) > 0 && Count(202, 75, 90, 8, DEV_FAINT_INK) > 0);
    assert(Count(244, 190, 68, 40, DEV_DARK_INK) > 0 && Count(8, 64, 148, 53, DEV_LED) == 0);
    for (i = 0; i < 6; ++i)                                                    /* icons at 1:1, not hopping */
        if (i != 5) assert(at(DevPtyBay(i).x + 4 + 2, DevPtyBay(i).y + (i ? 0 : 2) + 9 + 2) != DevKeyFace(i == 3 ? DEV_LOOK_FAINT : DEV_LOOK_KEY, !i));
    shot("bays");
    /* the touch sinks one key of the group: a bay, CANCEL; the cursor on CANCEL (FireRed's slot 7) sinks it */
    v.pressed = BUI_PTY_SLOT + 2; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(2), DEV_LOOK_KEY, 1); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0);
    v.pressed = BUI_PTY_CANCEL; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0); assert(LedAt(kDevGridExit));
    v.pressed = -1; v.pty.cursor = -1; v.pty.button = 1; DevCheckBattle(&old, &v); assert(LedAt(kDevGridExit));
    v.pty.cursor = 0; v.pty.button = -1; DevCheckBattle(&old, &v); assert(Count(244, 190, 68, 43, DEV_LED) == 0);
    /* the gender sign: drawn (a change redraws), none for a Pokemon without one; a name as wide as the bay is cut to leave
     * the sign its room, short of the LED's column (x + w - 14) */
    v.pty.gender[2] ^= 1; assert(DevWhole(DevCheckBattle(&old, &v)));
    assert(Count(46, 72, 110, 11, DEV_FEMALE) > 0 && Count(46, 72, 110, 11, DEV_MALE) == 0);
    v.pty.gender[2] = 2; DevCheckBattle(&old, &v); assert(Count(46, 64, 110, 50, DEV_MALE) == 0 && Count(46, 64, 110, 50, DEV_FEMALE) == 0);
    v.pty.gender[2] = 0; DevCheckBattle(&old, &v);
    {
        uint8_t keep = fontWidths[0xc7];                                       /* M, 12 px: the name overruns the bay */

        fontWidths[0xc7] = 12; memset(sPtFontReady, 0, sizeof(sPtFontReady));
        strcpy(v.pty.name[0], "MMMMMMMMMM"); v.pty.nameIsSpecies[0] = 0; DevCheckBattle(&old, &v);
        assert(Count(46, 18, 96, 11, DEV_MALE) > 0 && Count(142, 8, 14, 52, DEV_MALE) == 0);
        fontWidths[0xc7] = keep; memset(sPtFontReady, 0, sizeof(sPtFontReady));
        strcpy(v.pty.name[0], "CHARMELEON"); v.pty.nameIsSpecies[0] = 1; DevCheckBattle(&old, &v);
    }
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
    v.pty.mode = BUI_PTY_FORCED; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 278, 210) == -1);
    assert(CtrBattleUi_Hit(&v, 275, 205) == -1 && CtrBattleUi_Hit(&v, 302, 190) == -1 && Count(14, 196, 216, 30, DEV_INK) > 0);
    assert(at(245, 200) == DevBlend(DEV_SHELL_RED, DevKeyFace(DEV_LOOK_DARK, 0), 128) && at(302, 197) != DevBlend(DEV_SHELL_RED, DEV_LED, 128));
    shot("forced");
    v.pty.cursor = -1; v.pty.button = 1; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0);
    assert(at(245, 200) == DevBlend(DEV_SHELL_RED, DevKeyFace(DEV_LOOK_DARK, 1), 128) && at(302, 197) == DevBlend(DEV_SHELL_RED, DEV_LED, 128));
    assert(CtrBattleUi_Hit(&v, 275, 205) == -1);
    shot("forced_cancel");
    v.pressed = BUI_PTY_SLOT + 1; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(1), DEV_LOOK_KEY, 1);
    assert(at(302, 197) != DevBlend(DEV_SHELL_RED, DEV_LED, 128));
    v.pressed = -1; v.pty.cursor = 0; v.pty.button = -1; DevCheckBattle(&old, &v);
    /* picking several (the Decisions; no board): the words on the small note, CONFIRM cream 8 px after it, CANCEL in the
     * exit corner; nothing between the note and CONFIRM, the words not past the small note's paper */
    v.pty.mode = BUI_PTY_MULTI; DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, kDevPtyConfirm, BUI_PTY_CONFIRM);
    AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_CANCEL); assert(CtrBattleUi_Hit(&v, 162, 210) == -1);
    AssertKey(kDevPtyConfirm, DEV_LOOK_KEY, 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(Count(14, 196, 140, 30, DEV_INK) > 0 && Count(154, 190, 90, 43, DEV_INK) == 0 && at(164, 210) == DEV_SHELL_RED);
    assert(Count(168, 190, 68, 43, DEV_KEY_INK) > 0);                                                   /* CONFIRM's word */
    v.pty.button = 1; DevCheckBattle(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertKey(kDevPtyConfirm, DEV_LOOK_KEY, 0);
    /* picking several: the order on the picked bays, a Pokemon that may not enter grey; the cursor on CONFIRM */
    v.pty.cursor = -1; v.pty.button = 0; v.pty.entry[0] = BUI_PTY_FIRST + 1; v.pty.entry[1] = BUI_PTY_FIRST; v.pty.entry[2] = BUI_PTY_NOT_ABLE;
    v.pty.entry[3] = v.pty.entry[4] = BUI_PTY_ABLE; DevCheckBattle(&old, &v);
    AssertKey(kDevPtyConfirm, DEV_LOOK_KEY, 1); AssertKey(DevPtyBay(2), DEV_LOOK_FAINT, 0); AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 0);
    assert(LedAt(kDevPtyConfirm) && Count(244, 190, 68, 43, DEV_LED) == 0);
    assert(at(146, 50) == DEV_BLUE_INK && at(302, 50) == DEV_BLUE_INK && at(146, 160) != DEV_BLUE_INK);
    shot("multi");
    v.pty.entry[4] = BUI_PTY_NOT_ABLE; DevCheckBattle(&old, &v); AssertKey(DevPtyBay(4), DEV_LOOK_FAINT, 0);   /* ONIX may not */
    assert(Count(110, 159, 38, 7, DEV_FAINT_INK) > 0 && Count(110, 159, 38, 7, DEV_KEY_INK2) == 0);   /* its level faint */
    v.pty.entry[4] = BUI_PTY_ABLE; DevCheckBattle(&old, &v);
    /* a message picking several: on the small note with FireRed's arrow at its paper's foot, both keys grey */
    v.pty.panel = BUI_PTY_TEXT; strcpy(v.pty.message, "CHARMELEON is already in battle!"); DevCheckBattle(&old, &v);
    assert(at(171, 210) == dim && at(247, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128) && Count(14, 196, 140, 30, DEV_INK) > 0);
    assert(at(144, 222) == DEV_INK2 && at(140, 218) == DEV_INK2 && Count(154, 190, 90, 43, DEV_INK) == 0);
    v.pty.panel = BUI_PTY_NONE; v.pty.message[0] = 0; DevCheckBattle(&old, &v);
    v.pty.button = 1; v.pty.cursor = 0; memset(v.pty.entry, 0, sizeof(v.pty.entry)); DevCheckBattle(&old, &v);
    /* the action menu (def_party_menu; owner O2): the slim bays, the actions in the column at x 204 (words from x + 10),
     * FireRed's CANCEL the dark key in the exit corner, the Party's words on the grid's note. menu3 (a battle's SHIFT
     * SUMMARY CANCEL): two keys and the exit; no cue */
    v.pty.mode = BUI_PTY_SWITCH; v.pty.panel = BUI_PTY_ACTIONS; v.pty.menuCount = 3; v.pty.menuCursor = 0; v.pty.menuDark = 0x04; DevCheckBattle(&old, &v);
    for (k = 0; k < 2; ++k) AssertHits(BatHitV, &v, DevPtyMenuKey(k), BUI_PTY_MENU + k);
    AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_MENU + 2); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(CtrBattleUi_Hit(&v, 100, 20) == -1 && CtrBattleUi_Hit(&v, 258, 76) == -1);                  /* bays: not while a menu is open; no third key */
    assert(at(258, 76) == DEV_SHELL_RED && Count(254, 180, 9, 5, DEV_DARK_INK) == 0 && Count(254, 2, 9, 5, DEV_ARROW_OFF) == 0);
    /* the slim bays (188 wide): the chosen one sunk, the hole; a level small in DEV_KEY_INK2 (#6A6A6A, the board's) and
     * the numbers under it; the Party's words on the grid's note, nothing over it; the first row chosen */
    AssertKey(DevPtySlim(0), DEV_LOOK_KEY, 1); AssertKey(DevPtySlim(1), DEV_LOOK_KEY, 0); AssertKey(DevPtySlim(3), DEV_LOOK_FAINT, 0);
    assert(at(100, 8 + 28 * 5 + 12) == DevBlend(DEV_SHELL_RED, 0, 72) && Count(14, 196, 216, 30, DEV_INK) > 0 && Count(8, 176, 304, 14, DEV_INK) == 0);
    assert(Count(150, 36, 33, 12, DEV_KEY_INK2) > 0 && Count(150, 46, 33, 14, DEV_KEY_INK) > 0 && Count(183, 36, 13, 24, DEV_KEY_INK2) == 0);
    AssertKey(DevPtyMenuKey(0), DEV_LOOK_KEY, 1); AssertKey(DevPtyMenuKey(1), DEV_LOOK_KEY, 0);
    v.pty.gender[1] ^= 1; v.pty.button ^= 1; assert(DevCheckBattle(&old, &v).count == 0);              /* no sign on a slim bay; the bays' CANCEL not drawn */
    v.pty.gender[1] ^= 1; v.pty.button ^= 1; assert(DevCheckBattle(&old, &v).count == 0);
    v.pressed = BUI_PTY_MENU + 1; DevCheckBattle(&old, &v); AssertKey(DevPtyMenuKey(1), DEV_LOOK_KEY, 1); AssertKey(DevPtyMenuKey(0), DEV_LOOK_KEY, 0);
    v.pressed = BUI_PTY_MENU + 2; DevCheckBattle(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertKey(DevPtyMenuKey(0), DEV_LOOK_KEY, 0);
    v.pressed = -1; DevCheckBattle(&old, &v);
    strcpy(v.pty.menuName[0], "SHIFT"); strcpy(v.pty.menuName[1], "SUMMARY"); strcpy(v.pty.menuName[2], "CANCEL"); DevCheckBattle(&old, &v);
    assert(Count(204, 36, 10, 27, DEV_KEY_INK) == 0 && Count(214, 36, 8, 27, DEV_KEY_INK) > 0);        /* SUMMARY from x + 10 */
    assert(Count(244, 190, 68, 43, DEV_DARK_INK) > 0);                                                  /* CANCEL's word */
    shot("menu3");
    /* menu6 (the board's SUMMARY SWITCH ITEM CUT FLY CANCEL): five keys, field moves in blue, CANCEL in the exit corner;
     * no cue */
    strcpy(v.pty.menuName[0], "SUMMARY"); strcpy(v.pty.menuName[1], "SWITCH"); strcpy(v.pty.menuName[2], "ITEM");
    v.pty.menuCount = 6; v.pty.menuBlue = 0x18; v.pty.menuDark = 0x20; DevCheckBattle(&old, &v);
    for (k = 0; k < 5; ++k) AssertHits(BatHitV, &v, DevPtyMenuKey(k), BUI_PTY_MENU + k);
    AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_MENU + 5); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(CtrBattleUi_Hit(&v, 258, 160) == -1 && at(258, 160) == DEV_SHELL_RED);                       /* no sixth key */
    assert(Count(204, 92, 108, 27, DEV_BLUE_INK) > 0 && Count(204, 120, 108, 27, DEV_BLUE_INK) > 0 && Count(204, 8, 108, 83, DEV_BLUE_INK) == 0);
    assert(Count(254, 180, 9, 5, DEV_DARK_INK) == 0 && Count(254, 2, 9, 5, DEV_ARROW_OFF) == 0 && at(200, 20) == DEV_SHELL_RED);
    v.pty.caption[0] = 0; strcpy(v.pty.caption, "Do what with this POK\xC3\xA9MON?"); DevCheckBattle(&old, &v);
    shot("menu6");
    /* menu8 (Review Focus 2: SUMMARY SWITCH ITEM CUT FLY HEAL REMEMBER CANCEL, HEAL and REMEMBER with CTR_PARTY_EXTRAS,
     * blue as the fill marks them): six keys shown, the window following FireRed's cursor (the seventh action, CANCEL,
     * and round to SUMMARY again), CANCEL in the exit corner hitting the dark row; the cue cream where actions are
     * hidden, grey where none are, never hit */
    {
        static const char *const kEight[8] = {"SUMMARY", "SWITCH", "ITEM", "CUT", "FLY", "HEAL", "REMEMBER", "CANCEL"};

        for (i = 0; i < 8; ++i) strcpy(v.pty.menuName[i], kEight[i]);
        v.pty.menuCount = 8; v.pty.menuBlue = 0x78; v.pty.menuDark = 0x80;
        v.pty.menuCursor = 0; DevCheckBattle(&old, &v);
        for (k = 0; k < 6; ++k) AssertHits(BatHitV, &v, DevPtyMenuKey(k), BUI_PTY_MENU + k);
        AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_MENU + 7); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
        assert(Count(254, 180, 9, 5, DEV_DARK_INK) > 0 && Count(254, 2, 9, 5, DEV_ARROW_OFF) > 0);               /* more below */
        assert(Count(254, 2, 9, 5, DEV_DARK_INK) == 0 && Count(254, 180, 9, 5, DEV_ARROW_OFF) == 0);
        assert(CtrBattleUi_Hit(&v, 258, 4) == -1 && CtrBattleUi_Hit(&v, 258, 182) == -1);                       /* the cue: not hit */
        AssertKey(DevPtyMenuKey(0), DEV_LOOK_KEY, 1); assert(Count(204, 148, 108, 27, DEV_BLUE_INK) > 0);       /* HEAL on the sixth */
        shot("menu8");
        v.pty.menuCursor = 6; DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, DevPtyMenuKey(5), BUI_PTY_MENU + 6); AssertKey(DevPtyMenuKey(5), DEV_LOOK_KEY, 1);
        AssertHits(BatHitV, &v, DevPtyMenuKey(0), BUI_PTY_MENU + 1); AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_MENU + 7);
        assert(Count(254, 2, 9, 5, DEV_DARK_INK) > 0 && Count(254, 180, 9, 5, DEV_ARROW_OFF) > 0);               /* more above */
        assert(Count(204, 148, 108, 27, DEV_BLUE_INK) > 0 && Count(204, 8, 108, 55, DEV_BLUE_INK) == 0);        /* REMEMBER in blue; SUMMARY gone */
        shot("menu8_seventh");
        v.pty.menuCursor = 7; DevCheckBattle(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertHits(BatHitV, &v, DevPtyMenuKey(0), BUI_PTY_MENU + 1);
        for (k = 0; k < 6; ++k) AssertKey(DevPtyMenuKey(k), DEV_LOOK_KEY, 0);
        AssertHits(BatHitV, &v, DevPtyMenuKey(5), BUI_PTY_MENU + 6); assert(Count(254, 2, 9, 5, DEV_DARK_INK) > 0);
        shot("menu8_cancel");
        v.pty.menuCursor = 0; DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, DevPtyMenuKey(0), BUI_PTY_MENU);   /* DOWN wraps to SUMMARY */
        AssertKey(DevPtyMenuKey(0), DEV_LOOK_KEY, 1); assert(Count(254, 180, 9, 5, DEV_DARK_INK) > 0);
        v.pressed = BUI_PTY_MENU + 7; DevCheckBattle(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);   /* a touch on CANCEL sinks it */
        v.pressed = -1; DevCheckBattle(&old, &v);
        /* a long word on a chosen key clears its LED (a German field move) */
        strcpy(v.pty.menuName[3], "ZERTRUEMMERER"); v.pty.menuCursor = 3; DevCheckBattle(&old, &v);
        assert(LedAt(DevPtyMenuKey(3)) && Count(296, 92, 16, 14, DEV_BLUE_INK) == 0 && Count(206, 92, 90, 14, DEV_BLUE_INK) > 0);
        strcpy(v.pty.menuName[3], "CUT");
        v.pty.menuCursor = 0;
    }
    /* YES / NO: two keys in the column, no exit key (the corner empty, not taken) */
    v.pty.panel = BUI_PTY_YESNO; v.pty.menuCount = 2; strcpy(v.pty.menuName[0], "YES"); strcpy(v.pty.menuName[1], "NO");
    v.pty.menuBlue = v.pty.menuDark = 0; strcpy(v.pty.caption, "Switch places?"); DevCheckBattle(&old, &v);
    AssertHits(BatHitV, &v, DevPtyMenuKey(0), BUI_PTY_MENU); AssertHits(BatHitV, &v, DevPtyMenuKey(1), BUI_PTY_MENU + 1);
    assert(CtrBattleUi_Hit(&v, 278, 210) == -1 && at(278, 210) == DEV_SHELL_RED && Count(244, 190, 68, 43, DEV_BEZEL) == 0);
    AssertKey(DevPtyMenuKey(0), DEV_LOOK_KEY, 1); AssertKey(DevPtyMenuKey(1), DEV_LOOK_KEY, 0);
    assert(Count(14, 196, 216, 30, DEV_INK) > 0 && Count(254, 180, 9, 5, DEV_DARK_INK) == 0);
    shot("yesno");
    v.pty.menuName[5][0] = 'X'; v.pty.menuBlue = 0x20; assert(DevCheckBattle(&old, &v).count == 0);      /* rows past the menu: not drawn */
    v.pty.panel = BUI_PTY_TEXT; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 10, 10) == BUI_PTY_MESSAGE && CtrBattleUi_Hit(&v, 300, 230) == BUI_PTY_MESSAGE);
    /* a message: on the grid's note with FireRed's arrow at its paper's foot, the keys grey and none chosen */
    strcpy(v.pty.message, "CHARMELEON is already in battle!"); DevCheckBattle(&old, &v);
    assert(at(11, 33) == dim && at(247, 205) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128) && Count(14, 196, 216, 30, DEV_INK) > 0);
    assert(at(220, 222) == DEV_INK2 && at(216, 218) == DEV_INK2 && Count(8, 180, 304, 9, DEV_INK) == 0);
    v.pressed = BUI_PTY_MESSAGE; v.pty.menuCount = 5; assert(DevCheckBattle(&old, &v).count == 0);         /* nothing to sink */
    v.pressed = -1; v.pty.menuCount = 2;
    shot("message");
    /* a level up (no board: Call 8): the stats and their gains on the tall screen (to y 184), the message on the grid's
     * note, OK cream in the exit corner, up, sunk while a touch is held (taken anywhere); nothing else of the Party drawn */
    {
        static const uint16_t kStat[6] = {120, 80, 74, 86, 72, 98};
        CtrBattleView lv = v, lo = old;

        lv.pty.lvSlot = 0; lv.pty.lvLevel = 37;
        for (i = 0; i < 6; ++i) { lv.pty.lvStat[i] = kStat[i]; lv.pty.lvGain[i] = (int16_t)(2 + (i & 1)); }
        strcpy(lv.pty.message, "CHARMELEON grew to Lv. 37!");
        lv.pty.panel = BUI_PTY_LEVELUP; DevCheckBattle(&lo, &lv); AssertKey(kDevGridExit, DEV_LOOK_KEY, 0);
        lv.pressed = BUI_PTY_MESSAGE; assert(DevWhole(DevCheckBattle(&lo, &lv))); AssertKey(kDevGridExit, DEV_LOOK_KEY, 1);
        assert(LedAt(kDevGridExit) && CtrBattleUi_Hit(&lv, 160, 100) == BUI_PTY_MESSAGE && CtrBattleUi_Hit(&lv, 278, 210) == BUI_PTY_MESSAGE);
        lv.pty.cursor = 3; lv.pty.mode = BUI_PTY_TARGET; assert(DevCheckBattle(&lo, &lv).count == 0);
        lv.pressed = -1; assert(DevWhole(DevCheckBattle(&lo, &lv))); AssertKey(kDevGridExit, DEV_LOOK_KEY, 0);
        assert(Count(244, 190, 68, 43, DEV_LED) == 0 && Count(244, 190, 68, 43, DEV_KEY_INK) > 0);        /* OK, up */
        assert(Count(14, 14, 292, 164, DEV_FX_SUPER) > 0 && Count(14, 14, 292, 20, DEV_INK) > 0 && at(20, 20) == DEV_PAPER);
        assert(at(100, 177) == DEV_PAPER && at(100, 178) != DEV_PAPER);                                  /* the screen to y 184 */
        assert(Count(14, 196, 216, 30, DEV_INK) > 0 && Count(8, 180, 304, 9, DEV_INK) == 0);
        lv.pty.cursor = 0; lv.pty.mode = BUI_PTY_SWITCH; assert(DevCheckBattle(&lo, &lv).count == 0);
        shot("levelup");
        v = lv;
        old = lo;
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
    assert(Count(164 + 38, 8 + 29, 100, 9, DEV_HP_FRAME) == 0 && Count(164, 8, 148, 53, DEV_FEMALE) == 0);  /* nor a sign */
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
 * 15/15 (FIRE, 95, 100), SCRATCH 15/35, GROWL 40/40 (a status move), BRICK BREAK 3/15 (FIGHT). Strings as FireRed
 * has them (the B views are upper case); FireRed's cursor on the first move. */
static CtrSummaryUiView SumBoard(void)
{
    static const char *const kMove[4] = {"FLAMETHROWER", "SCRATCH", "GROWL", "BRICK BREAK"};
    static const char *const kType[4] = {"FIRE", "NORMAL", "NORMAL", "FIGHT"};     /* FireRed's names: 6 letters at most */
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
    strcpy(d->abilityDesc, "Port words for an ability.");    /* one small line on the note, small letters 7 px too */
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

/* The plan's suite (the tabs, ▲▼, BACK, the switch and the move rows hit where they are drawn; the chosen tab; ▲▼ grey
 * and not taken in the move details; CANCEL inverted under FireRed's cursor; an egg's pages; the frame while loading),
 * the boards' pieces on the grid (def_sum_info, def_sum_stats, def_sum_moves: the page from x 116 to 256, nothing
 * past it), what is not drawn, upper case, sparse data, then the states for the eye. */
static void SuiteSummary(void)
{
    CtrSummaryUiView sv = SumBoard(), so = sv;
    CtrPokeTouchRect r, tab[SUI_TABS];
    uint16_t bar = UI_RGB(0x8A, 0x94, 0xA8), fire = UiTypeColor(10), red = GbaColor(SUM_ART_RED), blue = GbaColor(SUM_ART_BLUE);
    int i, cuts;

    for (i = 0; i < SUI_TABS; ++i)
        tab[i] = DevGridTab(4, i);
    assert(CtrNature_Up(3) == 0 && CtrNature_Down(3) == 2 && CtrNature_Up(15) == 2 && CtrNature_Down(15) == 0
           && CtrNature_Up(10) == 4 && CtrNature_Up(0) == -1 && CtrNature_Down(24) == -1 && CtrNature_Up(25) == -1);
    for (i = 0; i < 25; ++i) {          /* the five with n / 5 == n % 5 are neutral; each other raises one stat, lowers another */
        int up = CtrNature_Up(i), down = CtrNature_Down(i);

        assert(i / 5 == i % 5 ? up == -1 && down == -1 : up >= 0 && up <= 4 && down >= 0 && down <= 4 && up != down);
    }
    sumArt = 1;
    CtrSummaryUi_Draw(canvas, &so);
    DevCheckSummary(&so, &sv);
    for (i = 0; i < 4; ++i) AssertHits(SumHitV, &sv, DevGridTab(4, i), SUI_HIT_TAB + i);
    AssertHits(SumHitV, &sv, kDevGridUp, SUI_HIT_PREV); AssertHits(SumHitV, &sv, kDevGridDown, SUI_HIT_NEXT); AssertHits(SumHitV, &sv, kDevGridExit, SUI_HIT_BACK);
    AssertKey(kDevGridUp, DEV_LOOK_KEY, 0); assert(Count(276, 40, 36, 66, DEV_KEY_INK) > 0 && Count(257, 46, 5, 132, DEV_INK) == 0);   /* nothing past 256 */
    assert(Count(64, 155, 40, 7, DEV_INK2) > 0);                                                     /* No. beside one chip */
    /* INFO, the board's pieces: the chosen tab sunk with its LED, the others up; ▲▼ cream keys, BACK dark in the exit
     * corner; the left column's paper with the name, the gender and the level, the picture in its box, the FIRE chip and
     * the number beside it; the rows' labels and values (capitals 53 + 15k), the HP bar, the EXP chip and bar; the
     * ability's name and its description on the note */
    AssertKey(tab[0], DEV_LOOK_KEY, 1); assert(LedAt(tab[0]));
    for (i = 1; i < 4; ++i) AssertKey(tab[i], DEV_LOOK_KEY, 0);
    AssertKey(kDevGridDown, DEV_LOOK_KEY, 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(Count(276, 114, 36, 66, DEV_KEY_INK) > 0 && Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    assert(at(16, 48) == DEV_PAPER2 && at(109, 177) == DEV_PAPER2 && at(110, 48) == DEV_PAPER && at(261, 177) == DEV_PAPER);
    assert(Count(20, 52, 86, 8, DEV_INK) > 0 && Count(20, 52, 86, 8, DEV_INK2) == 0 && Count(20, 65, 9, 9, DEV_MALE) > 0 && Count(31, 66, 75, 8, DEV_INK) > 0);
    assert(at(62, 112) == red && at(31, 81) == DEV_PAPER2 && at(93, 143) == DEV_PAPER2 && Count(30, 80, 64, 64, blue) > 0);
    assert(at(21, 153) == fire && at(59, 163) == fire && at(60, 153) == DEV_PAPER2 && Count(20, 167, 86, 7, DEV_INK2) == 0);
    for (i = 0; i < 7; ++i)                                     /* SPECIES, OT, ID No., ITEM, NATURE, ABILITY, HP */
        assert(Count(116, 54 + 15 * i, 30, 7, DEV_INK2) > 0 && Count(226, 53 + 15 * i, 30, 8, DEV_INK) > 0);
    assert(at(116, 157) == DEV_HP_FRAME && at(117, 159) == DEV_HP_GREEN && at(254, 159) == DEV_HP_GREEN && at(255, 159) == DEV_HP_FRAME);
    assert(at(116, 165) == DEV_HP_FRAME && Count(116, 165, 140, 9, DEV_EXP_LABEL) > 0 && Count(116, 165, 140, 9, DEV_EXP) > 0);
    assert(Count(250, 167, 6, 4, DEV_HP_FRAME) > 0 && Count(256, 160, 6, 18, DEV_HP_FRAME) == 0);   /* the EXP bar to 256 */
    assert(Count(14, 196, 216, 12, DEV_TITLE_INK) > 0 && Count(14, 210, 216, 10, DEV_INK) > 0);
    shot("info");
    /* two types: the second chip after the first, the number under them */
    sv.data.typeIds[1] = 2; strcpy(sv.data.types[1], "Flying"); DevCheckSummary(&so, &sv); assert(Count(20, 167, 40, 7, DEV_INK2) > 0);
    assert(Count(64, 155, 42, 7, DEV_INK2) == 0 && at(64, 153) == UiTypeColor(2) && Count(106, 152, 4, 12, UiTypeColor(2)) == 0);
    /* the widest data: two 6-letter types (FireRed's names are 6 letters at most) share the column's 86 px, on INFO and
     * on MOVES; a 12-letter ability and item (FireRed's longest) beside the page's labels: nothing cut, in EN, ES and DE */
    cuts = sDevKeyCut;
    sv.data.typeIds[0] = 3; strcpy(sv.data.types[0], "POISON"); sv.data.typeIds[1] = 4; strcpy(sv.data.types[1], "GROUND");
    strcpy(sv.data.ability, "COMPOUNDEYES"); strcpy(sv.data.item, "SILVERPOWDER"); DevCheckSummary(&so, &sv);
    assert(sDevKeyCut == cuts && at(21, 153) == UiTypeColor(3) && at(102, 153) == UiTypeColor(4) && Count(106, 152, 4, 12, UiTypeColor(4)) == 0);
    sv.data.page = SUI_PAGE_MOVES; DevCheckSummary(&so, &sv);
    assert(sDevKeyCut == cuts && at(102, 85) == UiTypeColor(4) && Count(106, 84, 4, 12, UiTypeColor(4)) == 0);
    sv.data.page = SUI_PAGE_INFO; sv.data.typeIds[0] = sv.data.typeIds[1] = 10; strcpy(sv.data.types[0], "FIRE"); strcpy(sv.data.types[1], "FIRE");
    strcpy(sv.data.ability, "BLAZE"); strcpy(sv.data.item, "NONE"); DevCheckSummary(&so, &sv);
    /* a value too wide for its row is cut and counted, as a cut key label is (the label-fit builds see it) */
    DevSumRowText(0, CTR_TR("ABILITY", "HABILIDAD", "F\xC3\x84HIGKEIT"), "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
    assert(sDevKeyCut == cuts + 1);
    sDevKeyCut = cuts;
    CtrSummaryUi_Draw(canvas, &so);

    sv.data.page = SUI_PAGE_SKILLS; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 221, 55) == SUI_HIT_IVTOGGLE); AssertKey(tab[1], DEV_LOOK_KEY, 1);
    /* STATS: the switch hit round where it is drawn, its STATS half shown; the nature's name; ATTACK raised (red, ▲),
     * SP. ATK lowered (blue, ▼), the others in the secondary ink; six rows from 69, 14 apart: a bar each (HP's, the
     * largest, full) ending by 228, the value at 256; the EXP line and NEXT, the bar ending before NEXT */
    AssertRowHits(SumHitV, &sv, kDevSumIvSwitchHit, SUI_HIT_IVTOGGLE); AssertKey(tab[0], DEV_LOOK_KEY, 0);
    assert(Count(187, 50, 34, 11, DEV_SHELL_BLUE) > 0 && Count(222, 50, 33, 11, DEV_SHELL_BLUE) == 0 && at(186, 55) == DEV_BUTTON_EDGE);
    assert(Count(116, 52, 66, 7, DEV_INK2) > 0 && Count(257, 46, 5, 132, DEV_INK) == 0);
    assert(Count(116, 83, 64, 13, DEV_STAT_UP) > 0 && Count(116, 111, 64, 13, DEV_BLUE_INK) > 0);
    assert(Count(116, 69, 140, 84, DEV_STAT_UP) == Count(116, 83, 64, 13, DEV_STAT_UP));
    assert(Count(116, 69, 140, 84, DEV_BLUE_INK) == Count(116, 111, 64, 13, DEV_BLUE_INK));
    for (i = 0; i < 6; ++i) {
        assert(Count(164, 73 + 14 * i, 64, 4, bar) > 0 && Count(228, 69 + 14 * i, 28, 13, bar) == 0);
        assert(Count(240, 69 + 14 * i, 16, 13, DEV_INK) > 0);                     /* the value, ending at 256 */
        if (i != 1 && i != 3) assert(Count(116, 72 + 14 * i, 48, 7, DEV_INK2) > 0);
    }
    assert(Count(160, 73, 68, 4, bar) >= 4 * 48);       /* HP, the largest: the whole bar (shorter beside ES / DE labels) */
    assert(Count(116, 69, 140, 13, UI_RGB(0xE2, 0xE2, 0xDA)) == 0 && Count(116, 97, 140, 13, UI_RGB(0xE2, 0xE2, 0xDA)) > 0);
    assert(at(116, 158) == DEV_HP_FRAME && Count(116, 158, 140, 9, DEV_EXP) > 0 && Count(200, 158, 56, 7, DEV_INK2) > 0);
    assert(Count(211, 160, 45, 4, DEV_HP_FRAME) == 0 && Count(200, 158, 56, 7, DEV_INK) == 0);   /* NEXT n, clear of the bar */
    assert(Count(14, 196, 216, 12, DEV_TITLE_INK) > 0);
    shot("stats");
    /* a neutral nature marks nothing; a held touch on the switch draws nothing */
    sv.data.natureId = 6; DevCheckSummary(&so, &sv);
    assert(Count(116, 69, 140, 84, DEV_STAT_UP) == 0 && Count(116, 69, 140, 84, DEV_BLUE_INK) == 0);
    sv.data.natureId = 3; DevCheckSummary(&so, &sv);
    sv.pressed = SUI_HIT_IVTOGGLE; assert(DevCheckSummary(&so, &sv).count == 0);
    sv.pressed = -1; DevCheckSummary(&so, &sv);

    sv.ivView = 1; DevCheckSummary(&so, &sv);
    sv.ivView = 0; assert(DevWhole(DevCheckSummary(&so, &sv)));                 /* the switch: the screen whole */
    sv.ivView = 1; assert(DevWhole(DevCheckSummary(&so, &sv)));
    /* IV / EV: the switch's other half; a bar each (the IVs) ending by 210, the IV at 228, the EV at 256; Hidden Power's
     * type and power on the last line, capitals 158 */
    assert(Count(187, 50, 34, 11, DEV_SHELL_BLUE) == 0 && Count(222, 50, 33, 11, DEV_SHELL_BLUE) > 0);
    for (i = 0; i < 6; ++i)
        assert(Count(116, 69 + 14 * i, 140, 13, bar) > 0 && Count(210, 69 + 14 * i, 46, 13, bar) == 0
               && Count(212, 71 + 14 * i, 16, 8, DEV_INK) > 0 && Count(240, 71 + 14 * i, 16, 8, DEV_INK2) > 0);
    assert(Count(116, 154, 140, 13, UiTypeColor(12)) > 0 && Count(116, 154, 140, 13, DEV_EXP) == 0);
    assert(Count(116, 158, 60, 7, DEV_INK2) > 0 && Count(240, 158, 16, 7, DEV_INK) > 0 && Count(257, 46, 5, 132, DEV_INK) == 0);
    shot("ivev");

    sv.data.page = SUI_PAGE_MOVES; DevCheckSummary(&so, &sv);
    for (i = 0; i < 4; ++i) AssertRowHits(SumHitV, &sv, DevSumRow(i), SUI_HIT_MOVE + i);
    assert(CtrSummaryUi_Hit(&sv, 221, 55) != SUI_HIT_IVTOGGLE && CtrSummaryUi_Hit(&sv, 170, 164) == -1);
    /* MOVES: the rows on the paper, none chosen, each with its type's band (4 px); SCRATCH's PP at half, BRICK BREAK's
     * at a quarter (FireRed's colours), ending at 256; the column: no icon, the name, "Lv" small, the rule, the Pokemon's
     * type at y 84 and no number; no CANCEL row */
    AssertKey(tab[2], DEV_LOOK_KEY, 1);
    for (i = 0; i < 4; ++i) assert(at(130, DevSumRow(i).y + 2) == DEV_PAPER && at(118, DevSumRow(i).y + 2) == DEV_PAPER);
    assert(at(114, 48) == fire && at(117, 71) == fire && at(115, 76) == UiTypeColor(0) && at(115, 128) == UiTypeColor(1));
    assert(Count(206, 74, 50, 24, DEV_PP_HALF) > 0 && Count(206, 126, 50, 24, DEV_PP_QUARTER) > 0 && Count(206, 48, 50, 24, DEV_INK) > 0);
    assert(Count(122, 56, 60, 8, DEV_INK) > 0 && Count(257, 46, 5, 132, DEV_INK) == 0);
    assert(Count(14, 46, 96, 132, red) == 0 && Count(20, 52, 86, 8, DEV_INK) > 0 && Count(20, 66, 86, 7, DEV_INK2) > 0);
    assert(at(20, 78) == DEV_RULE && at(103, 78) == DEV_RULE && at(104, 78) == DEV_PAPER2 && at(21, 85) == fire);
    assert(Count(20, 98, 86, 80, DEV_INK2) == 0 && Count(20, 98, 86, 80, DEV_INK) == 0);
    assert(Count(114, 152, 146, 24, DEV_INK) == 0 && Count(114, 152, 146, 24, DEV_INK2) == 0);
    assert(Count(14, 196, 216, 30, DEV_INK) > 0);
    shot("moves");
    /* a held touch inverts its row, its band kept */
    sv.pressed = SUI_HIT_MOVE + 2; DevCheckSummary(&so, &sv); assert(at(130, 102) == DEV_BEZEL && at(130, 50) == DEV_PAPER && at(115, 102) == UiTypeColor(0));
    sv.pressed = -1; DevCheckSummary(&so, &sv);

    /* the details: FireRed's cursor inverts its row; the move's type and category (ending by 106), POWER and ACCURACY in
     * the left column; its description on the note; ▲▼ grey and not taken; CANCEL under the moves */
    sv.data.page = SUI_PAGE_MOVES_INFO; sv.data.cursor = 0; strcpy(sv.data.moveDesc, "Port-written words in place of what a move does.");
    DevCheckSummary(&so, &sv);
    AssertKey(tab[2], DEV_LOOK_KEY, 1);
    assert(at(130, 50) == DEV_BEZEL && at(115, 50) == fire && Count(114, 48, 146, 24, DEV_PAPER) > 0 && at(130, 76) == DEV_PAPER);
    assert(at(21, 85) == fire && at(59, 95) == fire && Count(60, 84, 46, 12, DEV_CAT_SPECIAL) > 0 && Count(106, 84, 4, 12, DEV_CAT_SPECIAL) == 0);
    assert(Count(20, 105, 86, 8, DEV_INK) > 0 && Count(20, 106, 50, 7, DEV_INK2) > 0 && Count(20, 121, 86, 8, DEV_INK) > 0 && Count(20, 122, 50, 7, DEV_INK2) > 0);
    assert(Count(105, 98, 5, 36, DEV_INK) == 0);                                /* the values end at 104 */
    assert(Count(276, 40, 36, 66, DEV_ARROW_OFF) > 0 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0 && Count(276, 40, 36, 144, DEV_KEY_INK) == 0);
    assert(Count(114, 152, 146, 24, DEV_INK2) > 0 && Count(257, 46, 5, 132, DEV_INK) == 0);
    AssertRowHits(SumHitV, &sv, DevSumRow(0), SUI_HIT_MOVE);
    assert(Count(14, 196, 216, 30, DEV_INK) > 0 && Count(14, 196, 216, 30, DEV_TITLE_INK) == 0);
    /* CONTACT (def_sum_moves): the row at y 136 on a move, a status move too; its bit is drawn on the details only */
    assert(Count(20, 137, 84, 8, DEV_INK) > 0 || Count(20, 137, 84, 8, DEV_INK2) > 0);
    sv.data.contact ^= 1; assert(DevWhole(DevCheckSummary(&so, &sv))); sv.data.contact ^= 1; DevCheckSummary(&so, &sv);
    shot("details");
    sv.data.cursor = 2; DevCheckSummary(&so, &sv); assert(Count(20, 137, 84, 8, DEV_INK) > 0);
    sv.data.contact ^= 4; assert(DevWhole(DevCheckSummary(&so, &sv))); sv.data.contact ^= 4;
    sv.data.cursor = 0; DevCheckSummary(&so, &sv);
    sv.data.cursor = 2; strcpy(sv.data.moveDesc, "Port words for one move."); DevCheckSummary(&so, &sv);   /* GROWL: no category */
    assert(at(130, 102) == DEV_BEZEL && Count(60, 84, 50, 12, DEV_CAT_SPECIAL) == 0 && Count(60, 84, 50, 12, DEV_CAT_PHYSICAL) == 0);
    sv.data.cursor = 3; strcpy(sv.data.moveDesc, "Port words for another move's text."); DevCheckSummary(&so, &sv);   /* FIGHT: physical */
    assert(Count(60, 84, 46, 12, DEV_CAT_PHYSICAL) > 0 && Count(106, 84, 4, 12, DEV_CAT_PHYSICAL) == 0);
    sv.data.moveDesc[0] = 0;                                                    /* CANCEL: no description */
    /* Review Focus 5: the details — ▲▼ grey, not hit; the CANCEL row chosen; the rows hit */
    sv.data.page = SUI_PAGE_MOVES_INFO; sv.data.cursor = 4; DevCheckSummary(&so, &sv);
    assert(CtrSummaryUi_Hit(&sv, 294, 73) == -1 && Count(276, 40, 36, 66, DEV_ARROW_OFF) > 0 && at(DevSumRow(4).x + 1, DevSumRow(4).y + 12) == DEV_BEZEL);
    for (i = 0; i < 5; ++i) AssertRowHits(SumHitV, &sv, DevSumRow(i), SUI_HIT_MOVE + i);
    assert(at(21, 78) == DEV_RULE);
    assert(CtrSummaryUi_Hit(&sv, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0);
    assert(Count(114, 152, 146, 24, DEV_PAPER) > 0 && Count(122, 156, 80, 8, DEV_PAPER) > 0 && at(130, 50) == DEV_PAPER);
    assert(at(21, 85) == fire && Count(20, 98, 86, 40, DEV_INK) == 0);         /* no move chosen: the Pokemon's type */
    assert(Count(14, 196, 216, 30, DEV_INK) == 0);                              /* CANCEL: nothing to describe */
    shot("details_cancel");
    /* a touch held on what is not drawn (the review's race: FireRed starts a swap under a touch on the CANCEL it hides; a
     * slot without a move) chooses nothing: FireRed's cursor row stays inverted, and the touch's end draws nothing */
    sv.data.cursor = 1; DevCheckSummary(&so, &sv);
    sv.data.swapping = 1; sv.data.swapCursor = 1; sv.pressed = SUI_HIT_CANCEL; DevCheckSummary(&so, &sv);
    assert(at(130, 76) == DEV_BEZEL);
    sv.pressed = -1; assert(DevCheckSummary(&so, &sv).count == 0);
    sv.data.swapping = 0; sv.data.moves[3] = 0; DevCheckSummary(&so, &sv);
    sv.pressed = SUI_HIT_MOVE + 3; DevCheckSummary(&so, &sv); assert(at(130, 76) == DEV_BEZEL);
    sv.pressed = -1; assert(DevCheckSummary(&so, &sv).count == 0);
    sv.data.moves[3] = 4; sv.data.cursor = 4; DevCheckSummary(&so, &sv);
    sv.data.swapping = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 170, 164) == -1);
    /* a swap: no CANCEL; the move being moved framed (over its band), the cursor's row inverted; the hint on the note */
    sv.data.swapCursor = 1; sv.data.cursor = 2; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 170, 164) == -1);
    assert(at(130, 164) == DEV_PAPER && Count(114, 152, 146, 24, DEV_INK) == 0 && Count(114, 152, 146, 24, DEV_INK2) == 0);
    assert(at(130, 74) == DEV_BEZEL && at(130, 75) == DEV_PAPER && at(114, 80) == DEV_BEZEL && at(115, 80) == UiTypeColor(0) && at(259, 80) == DEV_BEZEL);
    assert(at(130, 102) == DEV_BEZEL && Count(14, 196, 216, 30, DEV_INK) > 0);
    shot("swap");
    /* a battle's Summary: FireRed moves no move in a battle, so no swap hint on the note */
    sv.data.origin = CTR_SUMMARY_BATTLE_PARTY; DevCheckSummary(&so, &sv); assert(Count(14, 196, 216, 30, DEV_INK) == 0);
    sv.data.swapping = 0; DevCheckSummary(&so, &sv); assert(Count(14, 196, 216, 30, DEV_INK) == 0);         /* no description */
    sv.data.origin = CTR_SUMMARY_FIELD_PARTY; DevCheckSummary(&so, &sv); assert(Count(14, 196, 216, 30, DEV_INK) > 0);
    sv.data.swapping = 1; DevCheckSummary(&so, &sv);
    sv.data.swapCursor = 3; assert(DevCheckSummary(&so, &sv).count == 1);
    strcpy(sv.data.moveDesc, "Port words for one move."); assert(DevCheckSummary(&so, &sv).count == 0);   /* the hint, not it */
    sv.data.swapping = 0; sv.data.swapCursor = 0; sv.data.cursor = 4; sv.data.moveDesc[0] = 0; DevCheckSummary(&so, &sv);
    sv.data.swapCursor = 2; assert(DevCheckSummary(&so, &sv).count == 0);                    /* not swapping: not drawn */

    sv.data.page = SUI_PAGE_INFO; DevCheckSummary(&so, &sv); sv.data.contact ^= 1; assert(DevCheckSummary(&so, &sv).count == 0); sv.data.contact ^= 1;
    sv.data.swapping = 0; sv.trainer = 1; sv.data.page = SUI_PAGE_INFO; DevCheckSummary(&so, &sv); AssertKey(tab[3], DEV_LOOK_KEY, 1);
    /* MEMO: the left column as INFO's; NATURE and RIBBONS in rows, then FireRed's memo from capitals 85; ▲▼ back; the
     * hint */
    AssertKey(tab[0], DEV_LOOK_KEY, 0); assert(Count(276, 40, 36, 66, DEV_KEY_INK) > 0);
    assert(at(62, 112) == red && at(21, 153) == fire && Count(64, 155, 40, 7, DEV_INK2) > 0);
    for (i = 0; i < 2; ++i)
        assert(Count(116, 54 + 15 * i, 30, 7, DEV_INK2) > 0 && Count(226, 53 + 15 * i, 30, 8, DEV_INK) > 0);
    assert(Count(116, 77, 140, 101, DEV_INK2) == 0 && Count(116, 77, 140, 8, DEV_INK) == 0 && Count(116, 85, 140, 8, DEV_INK) > 0);
    assert(Count(257, 46, 5, 132, DEV_INK) == 0 && Count(14, 196, 216, 30, DEV_INK) > 0);
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
    assert(at(62, 112) == red && Count(22, 72, 80, 8, blue) > 0 && Count(22, 144, 80, 8, blue) > 0);
    sumArt = 3; sv.data.serial = 3; DevCheckSummary(&so, &sv);
    assert(at(30 + 4, 80 + 4) == red && at(30 + 60, 80 + 60) == red && Count(30, 80, 64, 64, blue) == 0);
    sumArt = 1; sv.data.serial = 1; DevCheckSummary(&so, &sv);
    SumUpperCase(sv);
    CtrSummaryUi_Draw(canvas, &so);

    /* a Pokemon with two moves: the others not drawn as moves and not taken */
    sv.trainer = 0; sv.data.page = SUI_PAGE_MOVES; sv.data.moves[2] = sv.data.moves[3] = 0;
    sv.data.moveNames[2][0] = sv.data.moveNames[3][0] = sv.data.pp[2][0] = sv.data.pp[3][0] = 0; DevCheckSummary(&so, &sv);
    for (i = 2; i < 4; ++i) {
        r = DevSumRow(i);
        assert(CtrSummaryUi_Hit(&sv, r.x + 80, r.y + 10) == -1 && at(r.x + 1, r.y + 10) == DEV_PAPER);
    }
    sv = SumBoard(); sv.trainer = 1; DevCheckSummary(&so, &sv);

    sv.data.egg = 1; DevCheckSummary(&so, &sv);
    assert(CtrSummaryUi_Hit(&sv, tab[1].x + 10, 20) == -1 && CtrSummaryUi_Hit(&sv, tab[2].x + 10, 20) == -1 && CtrSummaryUi_Hit(&sv, tab[3].x + 10, 20) == SUI_HIT_TAB + 3);
    /* an egg: STATS and MOVES beige (unused); its name, no gender, level, types or number; the memo and how soon it
     * hatches; ▲▼ still walk the party (FireRed's first page) */
    AssertKey(tab[1], DEV_LOOK_FAINT, 0); AssertKey(tab[2], DEV_LOOK_FAINT, 0);
    AssertKey(tab[0], DEV_LOOK_KEY, 0); AssertKey(tab[3], DEV_LOOK_KEY, 1);
    assert(Count(20, 63, 86, 12, DEV_MALE) == 0 && Count(20, 152, 86, 22, fire) == 0 && Count(20, 152, 86, 22, DEV_INK2) == 0);
    AssertHits(SumHitV, &sv, kDevGridUp, SUI_HIT_PREV); AssertKey(kDevGridUp, DEV_LOOK_KEY, 0);
    sv.pressed = SUI_HIT_TAB + 1; DevCheckSummary(&so, &sv); AssertKey(tab[3], DEV_LOOK_KEY, 1); AssertKey(tab[1], DEV_LOOK_FAINT, 0);   /* not chosen */
    sv.pressed = -1; assert(DevCheckSummary(&so, &sv).count == 0);
    strcpy(sv.data.nickname, "EGG"); strcpy(sv.data.species, "EGG"); strcpy(sv.data.hatch, "Port-written words in place of how soon an EGG will hatch.");
    DevCheckSummary(&so, &sv); assert(Count(116, 50, 140, 120, DEV_INK) > 0 && Count(257, 46, 5, 132, DEV_INK) == 0);
    shot("egg");
    /* Review Focus 5: an egg's INFO, how soon it hatches from (116, 53) */
    sv.trainer = 0; DevCheckSummary(&so, &sv); assert(Count(116, 53, 140, 60, DEV_INK) > 0 && Count(116, 46, 140, 6, DEV_INK) == 0);
    assert(Count(20, 152, 86, 12, fire) == 0 && Count(257, 46, 5, 132, DEV_INK) == 0 && Count(14, 196, 216, 30, DEV_INK) == 0);
    shot("egg_info");
    sv.trainer = 1;

    sv.loading = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 36, 20) == -1 && CtrSummaryUi_Hit(&sv, 278, 209) == SUI_HIT_BACK && CtrSummaryUi_Hit(&sv, 294, 73) == -1);
    /* loading: the frame only, every tab beige and ▲▼ grey, BACK taken */
    for (i = 0; i < 4; ++i) AssertKey(tab[i], DEV_LOOK_FAINT, 0);
    assert(Count(276, 40, 36, 144, DEV_ARROW_OFF) > 0 && Count(276, 40, 36, 144, DEV_KEY_INK) == 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(Count(14, 46, 248, 132, DEV_INK) == 0 && Count(14, 196, 216, 30, DEV_INK) == 0);
    sv.pressed = SUI_HIT_BACK; DevCheckSummary(&so, &sv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    sumArt = 0;
    assert(sDevKeyCut == 0);                      /* main prints PASS device summary */
}

/* ---- bag: the field Bag, a case, a mart's BUY, the item PC, the mailbox, the battle Bag (boards B · BAG) -------------- */

static int BagHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_BagHit((const CtrPokeTouchView *)view, x, y);
}

/* The board's Items pocket: the first five of nine, FireRed's cursor on SUPER POTION (row 1) with its description; no
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
    for (int i = 0; i < PT_BAG_ROWS; ++i) {
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
    for (int i = 0; i < BUI_BAG_ROWS; ++i) {
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

/* Row i's paper (1 px in, half way down: clear of its icon and words): DEV_BEZEL when the row is inverted. */
static uint16_t BagRowAt(int i)
{
    CtrPokeTouchRect r = DevGridRow(i, 0);

    return at(r.x + 1, r.y + 12);
}

/* Review Focus 1: a Bag-page list of nine items (`v` as one of the suite's states) at its top, at its end and on CLOSE:
 * the window FireRed's cursor gives, five rows each hit on itself and nothing under the fifth, ▲ grey and not hit at
 * the top, ▼ at the end, the fifth row inverted, CLOSE sunk on its row. The canvas is `v` again after it. */
static void BagEveryList(CtrPokeTouchView v)
{
    const CtrPokeTouchView start = v;
    CtrPokeTouchView old = v;
    int i;

    assert(CtrPokeTouch_ListFirst(0, 9, 5) == 0 && CtrPokeTouch_ListFirst(4, 9, 5) == 2 && CtrPokeTouch_ListFirst(8, 9, 5) == 4
           && CtrPokeTouch_ListFirst(9, 9, 5) == 4 && CtrPokeTouch_ListFirst(1, 3, 5) == 0);
    v.bag.count = 9;
    v.bag.first = 0; v.bag.cursor = 0; DevCheckTouch(&old, &v);
    for (i = 0; i < 5; ++i) AssertRowHits(BagHitV, &v, DevGridRow(i, 0), PT_BAG_HIT_ROW + i);
    assert(CtrPokeTouch_BagHit(&v, 100, 179) == -1 && CtrPokeTouch_BagHit(&v, 294, 73) == -1);                 /* ▲ at the top */
    v.bag.first = 4; v.bag.cursor = 8; DevCheckTouch(&old, &v); assert(at(DevGridRow(4, 0).x + 1, DevGridRow(4, 0).y + 12) == DEV_BEZEL);
    assert(CtrPokeTouch_BagHit(&v, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0); AssertHits(BagHitV, &v, kDevGridUp, PT_BAG_HIT_UP);
    v.bag.cursor = 9; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);                          /* on CLOSE */
    CtrPokeTouch_Draw(canvas, &start);
}

/* A name too long for its row (a TM with its move; here capitals 14 px wide): the small font, its capitals from the
 * row's y + 9, a row under a name's. The canvas is `v` again after it; the key words those capitals cut are not
 * counted. */
static void BagLongName(CtrPokeTouchView v)
{
    static uint8_t keepWidths[256];
    const CtrPokeTouchView start = v;
    CtrPokeTouchRect r = DevGridRow(0, 0);
    int cut = sDevKeyCut;

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    for (unsigned c = 0xbb; c <= 0xd4; ++c)        /* A..Z, the normal font's */
        fontWidths[c] = 14;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    strcpy(v.bag.row[0].name, "TM26 DIGDIGDIGDIG");
    CtrPokeTouch_Draw(canvas, &v);
    assert(Count(r.x + 22, r.y + 8, 100, 1, DEV_INK) == 0 && Count(r.x + 22, r.y + 9, 100, 1, DEV_INK) > 0);
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    sDevKeyCut = cut;
    CtrPokeTouch_Draw(canvas, &start);
}

/* The same on the battle's Bag (`v` as BagBattle gives it), through CtrBattleUi_Hit's BUI_BAG_* codes. */
static void BagEveryBattleList(CtrBattleView v)
{
    const CtrBattleView start = v;
    CtrBattleView old = v;
    int i;

    v.bag.count = 9;
    v.bag.first = 0; v.bag.cursor = 0; DevCheckBattle(&old, &v);
    for (i = 0; i < 5; ++i) AssertRowHits(BatHitV, &v, DevGridRow(i, 0), BUI_BAG_ROW + i);
    assert(CtrBattleUi_Hit(&v, 100, 179) == -1 && CtrBattleUi_Hit(&v, 294, 73) == -1);                         /* ▲ at the top */
    v.bag.first = 4; v.bag.cursor = 8; DevCheckBattle(&old, &v); assert(at(DevGridRow(4, 0).x + 1, DevGridRow(4, 0).y + 12) == DEV_BEZEL);
    assert(CtrBattleUi_Hit(&v, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0); AssertHits(BatHitV, &v, kDevGridUp, BUI_BAG_UP);
    v.bag.cursor = 9; DevCheckBattle(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);                          /* on CLOSE */
    CtrBattleUi_Draw(canvas, &start);
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
    for (i = 0; i < 3; ++i) AssertHits(BagHitV, &v, DevGridTab(3, i), PT_BAG_HIT_TAB + i);
    AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1);
    v.bag.asked = 2; DevCheckTouch(&old, &v); AssertKey(DevGridTab(3, 2), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 0); v.bag.asked = 0xFF;
    for (i = 0; i < PT_BAG_ROWS; ++i) AssertRowHits(BagHitV, &v, DevGridRow(i, 0), PT_BAG_HIT_ROW + i);
    assert(BagRowAt(1) == DEV_BEZEL && CtrPokeTouch_BagHit(&v, 100, 179) == -1);                     /* row 1, the cursor's, inverted; five rows */
    assert(CtrPokeTouch_BagHit(&v, 294, 73) == -1 && CtrPokeTouch_BagHit(&v, 294, 147) == PT_BAG_HIT_DOWN);   /* nothing above yet */
    AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_CLOSE);
    DevCheckTouch(&old, &v);
    /* the board's pieces: the pocket's LED, the rows on the paper with their counts at the right, the cursor's row in
     * the paper's ink, ▲ grey and ▼ dark, the item's icon and description, CLOSE dark */
    assert(LedAt(DevGridTab(3, 0)) && at(200, 15) != DEV_LED && BagRowAt(0) == DEV_PAPER);
    assert(Count(232, 48, 22, 24, DEV_INK) > 0 && Count(232, 74, 22, 24, DEV_PAPER) > 0 && Count(40, 74, 60, 24, DEV_PAPER) > 0);
    assert(Count(290, 70, 9, 7, arrowOff) > 0 && Count(290, 144, 9, 7, DEV_KEY_INK) > 0);
    /* a row as the board's: the icon 12x12 at (21, 54), the name's capitals from (38, 56), the count's ending at 254 */
    assert(Count(21, 54, 12, 12, DEV_PAPER) < 12 * 12 && Count(16, 48, 5, 24, DEV_PAPER) == 5 * 24 && Count(21, 48, 12, 6, DEV_PAPER) == 12 * 6);
    assert(Count(38, 56, 60, 8, DEV_INK) > 0 && Count(38, 48, 60, 8, DEV_INK) == 0 && Count(34, 48, 4, 24, DEV_INK) == 0);
    assert(Count(250, 56, 4, 8, DEV_INK) > 0 && Count(254, 48, 6, 24, DEV_INK) == 0);
    /* the description: the item's icon, FireRed's text in the small font from x 48, capitals 198, 208 and 218 */
    assert(Count(18, 199, 24, 24, DEV_PAPER) < 24 * 24 && Count(48, 198, 176, 7, DEV_INK) > 0 && Count(48, 218, 176, 7, DEV_INK) > 0
           && Count(48, 198, 176, 1, DEV_INK) > 0);
    assert(Count(42, 196, 6, 30, DEV_INK) == 0 && Count(48, 196, 178, 2, DEV_INK) == 0 && Count(48, 216, 178, 2, DEV_INK) == 0);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    shot("items");
    BagEveryList(v);
    BagLongName(v);
    /* Key Items: a registered item's X chip, 10 tall from the row's y + 7, ending at the row's right margin */
    v.bag.pocket = 1; v.bag.regX = 13; DevCheckTouch(&old, &v); AssertKey(DevGridTab(3, 1), DEV_LOOK_KEY, 1);
    assert(at(253, 55) == DEV_BLUE_INK && at(253, 64) == DEV_BLUE_INK && at(253, 54) != DEV_BLUE_INK
           && at(253, 65) != DEV_BLUE_INK && at(254, 60) != DEV_BLUE_INK && Count(232, 48, 22, 24, DEV_INK) == 0);
    v.bag.pocket = 0; v.bag.regX = 0; DevCheckTouch(&old, &v);
    /* a touch chooses: a row (the cursor's not), ▼; ▲ cannot go up, so its touch draws nothing; what the page does not
     * draw (HOME's keys, the bar, BACK, the registered items outside Key Items, the quantity box's maximum) draws nothing */
    v.bag.pressed = PT_BAG_HIT_ROW + 3; DevCheckTouch(&old, &v); assert(BagRowAt(3) == DEV_BEZEL && BagRowAt(1) == DEV_PAPER);
    v.bag.pressed = PT_BAG_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevGridDown, DEV_LOOK_KEY, 1); assert(BagRowAt(1) == DEV_BEZEL);
    v.bag.pressed = PT_BAG_HIT_CLOSE; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); assert(BagRowAt(1) == DEV_PAPER);
    v.bag.pressed = PT_BAG_HIT_TAB + 1; DevCheckTouch(&old, &v); AssertKey(DevGridTab(3, 1), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 0);
    v.bag.pressed = -1; DevCheckTouch(&old, &v);
    v.bag.pressed = PT_BAG_HIT_UP; assert(DevCheckTouch(&old, &v).count == 0);
    v.bag.pressed = -1; strcpy(v.bar[0], "Items"); v.enabled[PT_BAG] = 1; v.backVisible = 1;
    v.bag.regX = 13; v.bag.tossMax = 99; v.bag.titleItem = 7; strcpy(v.bag.prompt, "?"); v.bag.money = 500;
    assert(DevCheckTouch(&old, &v).count == 0);
    v.bag.prompt[0] = 0;
    v.shell = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_BLUE);
    v.shell = 0; DevCheckTouch(&old, &v);
    v.bag.cursor = v.bag.count; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);     /* the game's cursor on CLOSE BAG */
    assert(BagRowAt(1) == DEV_PAPER && Count(14, 196, 216, 30, DEV_INK) == 0);
    shot("close");
    v.bag.count = 0; v.bag.cursor = 0; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 138, 56) == -1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    assert(Count(16, 48, 244, 24, DEV_BEZEL) == 0 && Count(16, 48, 244, 128, DEV_INK2) > 0);            /* no rows: the pocket is empty */
    shot("empty");
    v.bag.count = 9; v.bag.cursor = 1;
    /* the item menu (def_bag_menu): USE GIVE TOSS REGISTER CANCEL, FireRed's cursor on USE. The actions in the grid's
     * column at x 204, CANCEL (FireRed's last row) the dark key in the exit corner; the pocket shown sunk with its LED,
     * the others beige and not hit; the narrow list's words grey; the footer the grid's note, "... is selected." */
    v.bag.state = PT_BAG_STATE_ACTIONS; v.bag.menuCount = 5; v.bag.menuCursor = 0; DevCheckTouch(&old, &v);
    for (i = 0; i < 4; ++i) AssertHits(BagHitV, &v, DevBagMenuKey(5, i), PT_BAG_HIT_MENU + i);
    AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_MENU + 4); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); AssertKey(DevBagMenuKey(5, 0), DEV_LOOK_KEY, 1);
    AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 1), DEV_LOOK_FAINT, 0); assert(CtrPokeTouch_BagHit(&v, 150, 20) == -1);
    assert(Count(16, 48, 172, 130, DEV_INK) == 0 && Count(48, 198, 176, 27, DEV_INK) > 0);       /* the list's words grey; "… is selected." */
    AssertKey(DevGridTab(3, 2), DEV_LOOK_FAINT, 0); assert(LedAt(DevGridTab(3, 0)) && Count(DevGridTab(3, 1).x, 8, 98, 27, DEV_FAINT_INK) > 0);
    assert(CtrPokeTouch_BagHit(&v, 56, 20) == -1 && CtrPokeTouch_BagHit(&v, 203, 56) == -1 && CtrPokeTouch_BagHit(&v, 204, 187) == -1);
    assert(Same(DevBagMenuKey(5, 0), (CtrPokeTouchRect){204, 40, 108, 30}) && Same(DevBagMenuKey(5, 3), (CtrPokeTouchRect){204, 148, 108, 30}));
    /* USE sunk with its LED (301, 46); the words from x + 10 (GIVE's from 214); the narrow list, its words grey but the
     * cursor's row; the footer's words from x 48, the board's */
    assert(LedAt(DevBagMenuKey(5, 0)) && at(302, 47) == DEV_LED && BagRowAt(1) == DEV_BEZEL);
    assert(Count(206, 76, 8, 30, DEV_KEY_INK) == 0 && Count(214, 76, 2, 30, DEV_KEY_INK) > 0);
    assert(Count(16, 48, 172, 24, DEV_INK_DIM) > 0 && Count(16, 48, 172, 128, DEV_INK) == 0);
    assert(at(195, 100) != DEV_SHELL_RED && at(196, 100) == DEV_SHELL_RED && at(203, 100) == DEV_SHELL_RED);   /* the grid's narrow screen */
    assert(Count(46, 196, 180, 30, DEV_INK) > 0 && Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    assert(Count(46, 196, 2, 30, DEV_INK) == 0 && Count(48, 196, 2, 30, DEV_INK) > 0 && Count(18, 199, 24, 24, DEV_PAPER) < 24 * 24);
    v.bag.pressed = PT_BAG_HIT_MENU + 2; DevCheckTouch(&old, &v); AssertKey(DevBagMenuKey(5, 2), DEV_LOOK_KEY, 1); AssertKey(DevBagMenuKey(5, 0), DEV_LOOK_KEY, 0);
    v.bag.pressed = PT_BAG_HIT_MENU + 4; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertKey(DevBagMenuKey(5, 0), DEV_LOOK_KEY, 0);
    v.bag.pressed = -1; v.bag.menuCursor = 4; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);   /* FireRed's cursor on CANCEL */
    v.bag.menuCursor = 0; DevCheckTouch(&old, &v);
    v.bag.pressed = PT_BAG_HIT_ROW + 2; assert(DevCheckTouch(&old, &v).count == 0);                 /* the rows take no touch now */
    v.bag.pressed = PT_BAG_HIT_TAB + 1; assert(DevCheckTouch(&old, &v).count == 0);                 /* nor the pockets */
    v.bag.pressed = -1; strcpy(v.bag.menuName[5], "X"); assert(DevCheckTouch(&old, &v).count == 0); /* rows past the menu */
    shot("menu");
    v.bag.menuCount = 6; strcpy(v.bag.menuName[4], "CHECK"); strcpy(v.bag.menuName[5], "CANCEL");
    DevCheckTouch(&old, &v); assert(Same(DevBagMenuKey(6, 4), (CtrPokeTouchRect){204, 152, 108, 24})); AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_MENU + 5);
    for (i = 0; i < 5; ++i) AssertHits(BagHitV, &v, DevBagMenuKey(6, i), PT_BAG_HIT_MENU + i);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); AssertKey(DevBagMenuKey(6, 4), DEV_LOOK_KEY, 0); AssertKey(DevBagMenuKey(6, 0), DEV_LOOK_KEY, 1);
    shot("menu6");
    v.bag.state = PT_BAG_STATE_QUANTITY; DevCheckTouch(&old, &v);
    AssertHits(BagHitV, &v, kDevBagQtyUp, PT_BAG_HIT_QTY_UP); AssertHits(BagHitV, &v, kDevBagQtyDown, PT_BAG_HIT_QTY_DOWN);
    AssertHits(BagHitV, &v, kDevBagQtyOk, PT_BAG_HIT_QTY_OK); AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_QTY_CANCEL);
    assert(CtrPokeTouch_BagHit(&v, 258, 94) == -1 && CtrPokeTouch_BagHit(&v, 150, 20) == -1);        /* the count's screen, a pocket */
    /* how many: ▲, the count on its screen, ▼, OK in the column; CANCEL dark in the exit corner; the question in the
     * footer; the pockets as in the menu; a touch sinks its key */
    v.bag.tossCount = 3; DevCheckTouch(&old, &v);
    AssertKey(kDevBagQtyUp, DEV_LOOK_KEY, 0); AssertKey(kDevBagQtyOk, DEV_LOOK_KEY, 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 1), DEV_LOOK_FAINT, 0);
    assert(Count(210, 82, 96, 24, DEV_INK) > 0 && Count(46, 196, 180, 30, DEV_INK) > 0 && at(207, 79) == DEV_BEZEL && at(203, 79) == DEV_SHELL_RED);
    assert(Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    v.bag.pressed = PT_BAG_HIT_QTY_UP; DevCheckTouch(&old, &v); AssertKey(kDevBagQtyUp, DEV_LOOK_KEY, 1);
    v.bag.pressed = PT_BAG_HIT_QTY_CANCEL; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    v.bag.pressed = -1; DevCheckTouch(&old, &v);
    shot("quantity");
    /* selling: what that many fetch and the money, on the count's paper (82-106) */
    v.bag.use = PT_BAG_USE_SELL; v.bag.sale = 450; v.bag.money = 3000; DevCheckTouch(&old, &v);
    assert(Count(210, 96, 96, 7, DEV_INK2) > 0 && Count(210, 104, 96, 2, DEV_INK2) == 0 && Count(210, 82, 96, 12, DEV_INK) > 0);
    v.bag.use = PT_BAG_USE_FIELD; v.bag.sale = 0; v.bag.money = 0;
    /* YES / NO: YES cream in the column, NO dark in the exit corner, YES the cursor's; the question in the footer */
    v.bag.state = PT_BAG_STATE_YESNO; DevCheckTouch(&old, &v);
    AssertHits(BagHitV, &v, DevGridColumnKey(40, 36, 30, 0), PT_BAG_HIT_MENU); AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_MENU + 1);
    assert(CtrPokeTouch_BagHit(&v, 256, 93) == -1 && CtrPokeTouch_BagHit(&v, 150, 20) == -1);
    strcpy(v.bag.prompt, "Throw away 3 of this item?"); DevCheckTouch(&old, &v);
    AssertKey(DevGridColumnKey(40, 36, 30, 0), DEV_LOOK_KEY, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(46, 196, 180, 30, DEV_INK) > 0);
    AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 1), DEV_LOOK_FAINT, 0);
    assert(Count(204, 76, 108, 108, DEV_KEY) == 0 && Count(244, 190, 68, 40, DEV_DARK_INK) > 0);  /* nothing under YES; NO's word */
    v.bag.menuCursor = 1; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertKey(DevGridColumnKey(40, 36, 30, 0), DEV_LOOK_KEY, 0);
    v.bag.menuCursor = 0; v.bag.pressed = PT_BAG_HIT_MENU + 1; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    v.bag.pressed = -1;
    /* a question too long for two lines: the small font from x 48, capitals 198, 208 and 218 (each line's top row inked,
     * the row over it clear; no apostrophe, which rises over its line's capitals) */
    strcpy(v.bag.prompt, "Port words for a question so long that it takes three lines of the small font."); DevCheckTouch(&old, &v);
    assert(Count(48, 198, 178, 1, DEV_INK) > 0 && Count(48, 208, 178, 1, DEV_INK) > 0 && Count(48, 218, 178, 1, DEV_INK) > 0);
    assert(Count(48, 196, 178, 2, DEV_INK) == 0 && Count(48, 207, 178, 1, DEV_INK) == 0 && Count(48, 217, 178, 1, DEV_INK) == 0);
    assert(Count(42, 196, 6, 30, DEV_INK) == 0 && Count(226, 196, 4, 30, DEV_INK) == 0);
    strcpy(v.bag.prompt, "Throw away 3 of this item?"); DevCheckTouch(&old, &v);
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
    for (i = 0; i < PT_BAG_ROWS; ++i) { snprintf(v.bag.row[i].name, sizeof(v.bag.row[i].name), "TM%02d DIG", 26 + i); v.bag.row[i].qty = (uint16_t)(i == 4 ? 0 : 1); }
    strcpy(v.bag.row[4].name, "HM01 CUT"); strcpy(v.bag.selName, "DIG"); strcpy(v.bag.closeName, "CLOSE");
    v.bag.moveType = 4; strcpy(v.bag.typeName, "GROUND"); DevCheckTouch(&old, &v);
    assert(Count(48, 198, 60, 9, UiTypeColor(4)) > 0 && Count(232, 152, 22, 24, DEV_INK) == 0 && Count(232, 126, 22, 24, DEV_INK) > 0);
    assert(at(48, 198) == UiTypeColor(4) && at(48, 206) == UiTypeColor(4) && at(47, 198) != UiTypeColor(4)
           && at(48, 197) != UiTypeColor(4) && at(48, 207) != UiTypeColor(4));                    /* the chip 9 tall at (48, 198) */
    shot("tmcase");
    /* a case's item menu: its key stays sunk (it is the list shown), not dimmed */
    v.bag.state = PT_BAG_STATE_ACTIONS; DevCheckTouch(&old, &v); AssertKey(kDevBagCaseKey, DEV_LOOK_KEY, 1); assert(LedAt(kDevBagCaseKey));
    AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_MENU + v.bag.menuCount - 1);
    v.bag.state = PT_BAG_STATE_LIST; DevCheckTouch(&old, &v);
    BagEveryList(v);
    v.bag.kind = PT_BAG_KIND_POUCH; strcpy(v.bag.title, "BERRY POUCH"); v.bag.moveType = -1;
    for (i = 0; i < PT_BAG_ROWS; ++i) { snprintf(v.bag.row[i].name, sizeof(v.bag.row[i].name), "%s BERRY", i & 1 ? "ORAN" : "CHERI"); v.bag.row[i].qty = (uint16_t)(1 + i); }
    strcpy(v.bag.selName, "ORAN BERRY"); DevCheckTouch(&old, &v); AssertKey(kDevBagCaseKey, DEV_LOOK_KEY, 1);
    shot("pouch");
    BagEveryList(v);
    v.bag.kind = PT_BAG_KIND_SHOP; strcpy(v.bag.title, "POK\xC3\xA9 MART"); v.bag.money = 3000; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 160, 20) == -1);   /* the title */
    /* a mart: the title on a screen with the money at its right, prices at the rows' right, how many the Bag holds */
    for (i = 0; i < PT_BAG_ROWS; ++i) { strcpy(v.bag.row[i].name, kBagItem[i].name); v.bag.row[i].qty = 0; v.bag.row[i].price = (uint16_t)(200 + 100 * i); }
    strcpy(v.bag.selName, "SUPER POTION"); v.bag.showOwned = 1; v.bag.owned = 2; strcpy(v.bag.closeName, "Cancel"); DevCheckTouch(&old, &v);
    assert(at(9, 20) == DEV_BEZEL && Count(250, 12, 56, 16, DEV_INK) > 0 && Count(222, 48, 32, 24, DEV_INK) > 0);
    assert(Count(48, 198, 60, 9, DEV_BLUE_INK) > 0 && Count(48, 197, 60, 1, DEV_BLUE_INK) == 0);
    v.bag.state = PT_BAG_STATE_QUANTITY; v.bag.sale = 1400; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(at(11, 20) == DEV_BEZEL && Count(250, 12, 56, 16, DEV_INK) > 0);                         /* the title not dimmed */
    AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_QTY_CANCEL);
    v.bag.state = PT_BAG_STATE_LIST; v.bag.sale = 0; DevCheckTouch(&old, &v);
    shot("shop");
    BagEveryList(v);
    v.bag.kind = PT_BAG_KIND_ITEMPC; strcpy(v.bag.title, "WITHDRAW ITEM"); v.bag.money = 0; v.bag.showOwned = 0;
    for (i = 0; i < PT_BAG_ROWS; ++i) { v.bag.row[i].price = 0; v.bag.row[i].qty = (uint16_t)kBagItem[i].qty; }
    DevCheckTouch(&old, &v); AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_CLOSE);
    shot("itempc");
    BagEveryList(v);
    strcpy(v.bag.title, "MAILBOX"); v.bag.typed = 1; v.bag.count = 3; v.bag.cursor = 0;
    for (i = 0; i < 3; ++i) { strcpy(v.bag.row[i].name, i ? "MOM" : "Rick"); v.bag.row[i].item = (uint16_t)(121 + i); v.bag.row[i].qty = 0; }
    strcpy(v.bag.selName, "Rick"); v.bag.selItem = 121; strcpy(v.bag.selDesc, "Port words for a MAIL's text.");
    DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 138, 48 + DEV_GRID_ROW_PITCH * 3 + 8) == -1 && CtrPokeTouch_BagHit(&v, 138, 48 + 8) == PT_BAG_HIT_ROW);
    shot("mailbox");
    BagEveryList(v);
    BagUpperCase(BagBoard());
    /* the way in and out: HOME and the Bag, each whole */
    CtrPokeTouch_Draw(canvas, &old);
    home = BagBoard(); home.bag.on = 0; home.home = 1; home.playerX = home.playerY = home.cursorX = home.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &home)));
    v = BagBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    /* the battle Bag */
    CtrBattleUi_Draw(canvas, &bo);
    DevCheckBattle(&bo, &bv); AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1);
    assert(CtrBattleUi_Hit(&bv, 56, 20) == -1 && CtrBattleUi_Hit(&bv, 160, 20) == -1);                 /* no Berry Pouch: no BERRIES */
    assert(at(160, 20) == DEV_SHELL_RED && at(264, 20) == DEV_SHELL_RED && BagRowAt(1) == DEV_BEZEL);
    shot("battle");
    BagEveryBattleList(bv);
    bv.bag.berries = 365; DevCheckBattle(&bo, &bv); AssertHits(BatHitV, &bv, DevGridTab(3, 1), BUI_BAG_BERRIES);
    AssertRowHits(BatHitV, &bv, DevGridRow(0, 0), BUI_BAG_ROW); AssertHits(BatHitV, &bv, kDevGridExit, BUI_BAG_CANCEL);
    /* BERRIES under a touch is the one sunk; the battle's own fields are not drawn; to the commands and back, whole */
    AssertKey(DevGridTab(3, 1), DEV_LOOK_KEY, 0); assert(at(264, 20) == DEV_SHELL_RED);
    shot("battle_berries");
    bv.pressed = BUI_BAG_BERRIES; DevCheckBattle(&bo, &bv); AssertKey(DevGridTab(3, 1), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 0);
    bv.pressed = BUI_BAG_DOWN; DevCheckBattle(&bo, &bv); AssertKey(kDevGridDown, DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1);
    bv.pressed = -1; DevCheckBattle(&bo, &bv);
    bv.activeHp = 7; bv.party[2] = 2; bv.cursor = 2; bv.showCursor = 1; bv.bag.bagState = 3; assert(DevCheckBattle(&bo, &bv).count == 0);
    strcpy(bv.bag.row[0].name, "Potion"); assert(DevCheckBattle(&bo, &bv).count == 0);           /* drawn as POTION all the same */
    strcpy(bv.bag.row[0].name, "POTION"); DevCheckBattle(&bo, &bv);
    bv.state = BUI_ACTION; assert(DevWhole(DevCheckBattle(&bo, &bv)));
    bv.state = BUI_BAG; assert(DevWhole(DevCheckBattle(&bo, &bv)));
    /* the battle's menu (USE, CANCEL): USE in the column, CANCEL in the exit corner; ITEMS sunk, BERRIES beige, not hit */
    bv.bag.menuCount = 2; DevCheckBattle(&bo, &bv); AssertHits(BatHitV, &bv, DevBagMenuKey(2, 0), BUI_BAG_MENU); AssertHits(BatHitV, &bv, kDevGridExit, BUI_BAG_MENU + 1);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); AssertKey(DevBagMenuKey(2, 0), DEV_LOOK_KEY, 1); assert(LedAt(DevBagMenuKey(2, 0)));
    AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 1), DEV_LOOK_FAINT, 0);
    assert(CtrBattleUi_Hit(&bv, 160, 20) == -1 && CtrBattleUi_Hit(&bv, 50, 60) == -1 && CtrBattleUi_Hit(&bv, 256, 93) == -1);
    assert(Count(48, 196, 180, 30, DEV_INK) > 0);                                                    /* "... is selected." */
    shot("battle_menu");
    bv.pressed = BUI_BAG_MENU + 1; DevCheckBattle(&bo, &bv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertKey(DevBagMenuKey(2, 0), DEV_LOOK_KEY, 0);
    bv.pressed = -1;
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
