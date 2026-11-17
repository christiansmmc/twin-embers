/* PokeTouch: the bottom screen, drawn and hit-tested.
 *
 * Layout (320x240, see 3ds_poketouch.h): the frame with the 240x160 viewport
 * (the GBA picture is drawn there 1:1 by the GPU, 3ds_video.c), the
 * contextual bar under it, the side column POKeDEX / POKeMON / BAG / the
 * player's name (Trainer Card) / SAVE / OPTIONS and under it the X and Y
 * shortcuts and the Running Shoes (the running mode), icons only. With no
 * menu open the lower screen is HOME, the red device's (3ds_device_home_impl.h):
 * FireRed's Region Map at 1:1 with the player on it, the party, RUN, the
 * game's speed and the six START entries as keys.
 *
 * Look (the views not yet the red device's): the Llama mockup's (3ds_ember2_impl.h): FireRed's text window for the note and the
 * picture's frame; cream pieces that wear flame (with a ring) when chosen; slate for BACK.
 *
 * Art: FireRed's own item icons for the shortcuts (read from the game, so
 * they show whatever is configured), FireRed's latin_normal font for the
 * bar, a 5x7 bitmap font for buttons, and pack-backed item icons for
 * six menu entries (original shoe fallback). Drawn only when the view
 * changes.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "3ds_poketouch.h"
#include "3ds_bottom.h"
#include "3ds_data.h"
#include "poketouch_assets.h"
#include "twin_font.h"
#include "3ds_ui_theme.h"
#include "3ds_lang.h"
#include "battle_flat_icons.h"

/* Game bridge (src/ctr_poketouch.c): an item's 24x24 icon decompressed from
 * FireRed's own icon table. */
extern int CtrPokeTouch_ItemIcon(unsigned item, unsigned char *tiles, unsigned short *palette);

#define W CTR_BOTTOM_WIDTH
#define H CTR_BOTTOM_HEIGHT
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
/* A GBA colour (BGR555) as the canvas's RGB565, green's low bit clear: FireRed's palettes as its icons and pictures
 * are drawn here. */
static inline uint16_t GbaColor(uint16_t c)
{
    return RGB((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3);
}


/* Native menu icons: the six themed item graphics already reconstructed from
 * the user's FireRed ROM into classicfirered3ds.pak. No game pixels or palettes are
 * compiled into the port. Running Shoes keeps the port-authored shoe symbol:
 * the supported ROM has no corresponding item-icon entry. */
static const char *const sMenuItemArt[6] = {
    "fame_checker", "poke_ball", "berry_pouch",
    "card_key", "coin_case", "teachy_tv"
};

/* Bounded GBA LZ77 (0x10) decoder for the original item 4bpp/palette files.
 * Input is already size/CRC-verified by CtrData_Load. Still validate every
 * token and backreference before writing into the small local buffer. */
static int PokeTouch_Lz77(const uint8_t *src, uint32_t len, uint8_t *out, uint32_t cap)
{
    uint32_t used = 4, written = 0, size;
    if (!src || len < 4 || src[0] != 0x10)
        return 0;
    size = (uint32_t)src[1] | ((uint32_t)src[2] << 8) | ((uint32_t)src[3] << 16);
    if (!size || size > cap)
        return 0;
    while (written < size) {
        uint8_t flags;
        if (used >= len)
            return 0;
        flags = src[used++];
        for (int bit = 7; bit >= 0 && written < size; --bit) {
            if (flags & (1u << bit)) {
                uint8_t a, b;
                uint32_t count, back;
                if (len - used < 2)
                    return 0;
                a = src[used++];
                b = src[used++];
                count = (a >> 4) + 3u;
                back = (((uint32_t)a & 15u) << 8 | b) + 1u;
                if (back > written || count > size - written)
                    return 0;
                while (count--) {
                    out[written] = out[written - back];
                    ++written;
                }
            } else {
                if (used >= len)
                    return 0;
                out[written++] = src[used++];
            }
        }
    }
    return (int)size;
}

static int PokeTouch_LoadLz(const char *path, uint8_t *out, uint32_t cap)
{
    uint32_t len = 0;
    uint8_t *packed = CtrData_Load(path, &len);
    int unpacked = 0;
    if (packed) {
        if (len > 0 && len <= 4096)
            unpacked = PokeTouch_Lz77(packed, len, out, cap);
        free(packed);
    }
    return unpacked;
}

static int CtrPokeTouch_PackMenuIcon(unsigned icon, uint8_t pixels[12 * 12], uint16_t pal[16])
{
    char path[96];
    uint8_t tiles[512], colors[32], scaled[12 * 12];
    uint16_t converted[16];
    int pitch, n, visible = 0;
    if (icon >= 6)
        return 0;

    n = snprintf(path, sizeof(path), "graphics/items/icons/%s.4bpp.lz", sMenuItemArt[icon]);
    if (n < 0 || (size_t)n >= sizeof(path))
        return 0;
    n = PokeTouch_LoadLz(path, tiles, sizeof(tiles));
    /* FireRed icons are 24x24 (3x3 tiles), with some packed into 32x32.
     * Decode the upper 24x24 exactly in GBA 4bpp tile order. */
    if (n != 9 * 32 && n != 16 * 32)
        return 0;
    pitch = n == 9 * 32 ? 3 : 4;

    n = snprintf(path, sizeof(path), "graphics/items/icon_palettes/%s.gbapal.lz", sMenuItemArt[icon]);
    if (n < 0 || (size_t)n >= sizeof(path) ||
        PokeTouch_LoadLz(path, colors, sizeof(colors)) != 32)
        return 0;
    converted[0] = 0;
    for (int i = 1; i < 16; ++i) {
        uint16_t c = (uint16_t)colors[i * 2] | (uint16_t)(colors[i * 2 + 1] << 8);
        /* Replicate the high green bit into RGB565's sixth bit (white stays
         * white); transparency is an index, never a colour-key test. */
        unsigned g = (c >> 5) & 31;
        converted[i] = (uint16_t)((c & 31) << 11 | (g << 1 | g >> 4) << 5 | (c >> 10 & 31));
    }
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < 12; ++x) {
            /* A fixed 2x2 coverage sample preserves one-pixel outlines that
             * top-left-only sampling could discard. Prefer the first opaque
             * pixel, with no averaging, blur or invented palette colours. */
            uint8_t v = 0;
            for (int oy = 0; oy < 2 && !v; ++oy)
                for (int ox = 0; ox < 2 && !v; ++ox) {
                    int sx = x * 2 + ox, sy = y * 2 + oy;
                    int tile = (sy / 8 * pitch + sx / 8) * 32 + sy % 8 * 4 + sx % 8 / 2;
                    uint8_t packed = tiles[tile];
                    v = (sx & 1) ? packed >> 4 : packed & 15;
                }
            scaled[y * 12 + x] = v;
            visible |= v != 0;
        }
    if (!visible)
        return 0;
    memcpy(pixels, scaled, sizeof(scaled));
    memcpy(pal, converted, sizeof(converted));
    return 1;
}

/* The START column on the right of the GBA picture's frame (the picture ends at x = 244, its frame at 247): six raised
 * buttons, the X and Y shortcut tiles and RUN with its switch. */
const CtrPokeTouchRect gCtrPokeTouchRects[PT_COUNT] = {
    [PT_POKEDEX]    = {250,   4, 68, 21},
    [PT_POKEMON]    = {250,  29, 68, 21},
    [PT_BAG]        = {250,  54, 68, 21},
    [PT_CARD]       = {250,  79, 68, 21},
    [PT_SAVE]       = {250, 104, 68, 21},
    [PT_OPTIONS]    = {250, 129, 68, 21},
    [PT_REGISTERED] = {250, 158, 32, 40},
    [PT_BIKE]       = {286, 158, 32, 40},
    [PT_RUN]        = {250, 204, 68, 30},
};

/* Areas that are not buttons: the picture's frame, and under it the bar: the note card on the left and, on its
 * right, BACK or SAVE's buttons. */
static const CtrPokeTouchRect sMenuFrame = {0, 0, 248, 168};
static const CtrPokeTouchRect sBar = {0, 168, 248, 72};
const CtrPokeTouchRect gCtrPokeTouchBack = {172, 202, 72, 34};
#define NOTE_X 4
#define NOTE_Y 172
#define NOTE_H 64
#define NOTE_W_FULL 240
#define NOTE_W_SIDE 164

/* The palette and components are the lower screen's design system (3ds_ui_theme.h / .inc); the red device's B views
 * have their own kit (3ds_device_kit_impl.h). */
static uint16_t *sCanvas;

static void Px(int x, int y, uint16_t c)
{
    if ((unsigned)x < W && (unsigned)y < H)
        sCanvas[(size_t)x * H + (H - 1 - (size_t)y)] = c;
}

/* Two pixels a store where the column allows it: the ARM11 writes a word as fast as a halfword. */
typedef uint32_t __attribute__((may_alias)) PtPair;

static void Fill(int x, int y, int w, int h, uint16_t c)
{
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;
    uint32_t pair = (uint32_t)c * 0x10001u;

    for (int cx = x0; cx < x1; ++cx) {
        uint16_t *p = sCanvas + (size_t)cx * H + (H - (size_t)y1);
        int n = y1 - y0;

        if (n > 0 && ((uintptr_t)p & 2)) {
            *p++ = c;
            --n;
        }
        for (; n >= 2; n -= 2, p += 2)
            *(PtPair *)p = pair;
        if (n > 0)
            *p = c;
    }
}

/* FireRed's charmap for the two-byte UTF-8 Latin letters its European
 * releases use (U+00A1..U+00FF and Œ/œ): accents, Ñ/ñ, ¿, ¡, º, ª, ¥, ×. */
static unsigned LatinCode(unsigned char lead, unsigned char next)
{
    static const uint8_t sC3[64] = {
        /* U+00C0 À Á Â Ã Ä Å Æ Ç */ 0x01, 0x02, 0x03, 0, 0xf1, 0, 0, 0x04,
        /* U+00C8 È É Ê Ë Ì Í Î Ï */ 0x05, 0x06, 0x07, 0x08, 0x09, 0x5a, 0x0b, 0x0c,
        /* U+00D0 Ð Ñ Ò Ó Ô Õ Ö × */ 0, 0x14, 0x0d, 0x0e, 0x0f, 0, 0xf2, 0xb9,   /* ×: the Bag's counts, "×5" */
        /* U+00D8 Ø Ù Ú Û Ü Ý Þ ß */ 0, 0x11, 0x12, 0x13, 0xf3, 0, 0, 0x15,
        /* U+00E0 à á â ã ä å æ ç */ 0x16, 0x17, 0, 0, 0xf4, 0, 0, 0x19,
        /* U+00E8 è é ê ë ì í î ï */ 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x6f, 0x20, 0x21,
        /* U+00F0 ð ñ ò ó ô õ ö ÷ */ 0, 0x29, 0x22, 0x23, 0x24, 0, 0xf5, 0,
        /* U+00F8 ø ù ú û ü ý þ ÿ */ 0, 0x26, 0x27, 0x28, 0xf6, 0, 0, 0,
    };

    if (lead == 0xc3) return next >= 0x80 ? sC3[next - 0x80] : 0;
    if (lead == 0xc5) return next == 0x92 ? 0x10 : next == 0x93 ? 0x25 : 0;   /* Œ œ */
    switch (next) {   /* lead 0xc2 */
        case 0xa1: return 0x52;   /* ¡ */
        case 0xa5: return 0xb7;   /* ¥ */
        case 0xaa: return 0x2b;   /* ª */
        case 0xba: return 0x2a;   /* º */
        case 0xbf: return 0x51;   /* ¿ */
        default: return 0;
    }
}

/* The character code of the next UTF-8 character of `*s` in FireRed's charmap (0 for what the font does not draw). */
static unsigned GlyphCode(const char **s)
{
    unsigned char c = (unsigned char)**s;
    unsigned code = 0;

    ++*s;
    if (c >= 'A' && c <= 'Z') code = 0xbb + c - 'A';
    else if (c >= 'a' && c <= 'z') code = 0xd5 + c - 'a';
    else if (c >= '0' && c <= '9') code = 0xa1 + c - '0';
    else if ((c == 0xc2 || c == 0xc3 || c == 0xc5) && (**s & 0xc0) == 0x80)
        code = LatinCode(c, (unsigned char)*(*s)++);
    else if (c == 0x01 && **s) code = (unsigned char)*(*s)++;   /* a FireRed code as it is (CtrPokeTouch name text) */
    else if (c == 0x02 && **s) code = 0x100u | (unsigned char)*(*s)++;   /* a Japanese one (a Japanese Pokemon's name) */
    else switch (c) {
        case '!': code=0xab; break; case '?': code=0xac; break;
        case '.': code=0xad; break; case '-': code=0xae; break;
        case ',': code=0xb8; break; case '/': code=0xba; break;
        case ':': code=0xf0; break; case '(': code=0x5c; break;
        case ')': code=0x5d; break; case '%': code=0x5b; break;
        case '=': code=0x35; break; case ';': code=0x36; break;   /* BUTTON MODE's "L=A", the NUZLOCKE note */
        case '\'': code=0xb4; break; case '"': code=0xb2; break;   /* FireRed's closing quotes: KING'S ROCK, 2'00" */
        case '+': code=0x2e; break;                                 /* a level up's gains, "+2" */
        default: break;
    }
    return code;
}

/* FireRed's latin_normal glyphs, text and shadow pixels apart. Capitals
 * cover glyph rows 3-11, so a line drawn at `top` has its capitals there. */
static const PtGlyph *Glyph(const char **s)
{
    unsigned code;

    TwinIconsPrepare();
    code = GlyphCode(s);
    if (!sPtFontReady[code]) {
        static uint8_t jp[64];
        const uint8_t *src = code & 0x100u ? CtrTwinJapaneseGlyph(code & 0xffu, jp) : CtrTwinFontData() + code * 64;
        PtGlyph *g = &sPtFont[code];
        g->width = code & 0x100u ? CtrTwinJapaneseFontWidth(code & 0xffu) : CtrTwinFontWidth(code);
        if (g->width > 16) g->width = 16;
        TwinGlyphDecode(src, g->width, g->rows);
        sPtFontReady[code] = true;
    }
    return &sPtFont[code];
}

/* Images: palette indices, 0 clear; disabled art is a light grey. */
static uint16_t Tone(uint16_t c, int on)
{
    if (on)
        return c;
    unsigned r = c >> 11, g = (c >> 5) & 63, b = c & 31;
    unsigned luma = (r * 2 * 77 + g * 150 + b * 2 * 29) >> 8; /* 0..63 */
    luma = 34 + luma * 3 / 8;
    return (uint16_t)((luma >> 1) << 11 | luma << 5 | (luma >> 1));
}

static void Art(const uint8_t *pixels, const uint16_t *pal, int stride, int x0, int y0, int w, int h,
                int x, int y, int on)
{
    int px0 = x < 0 ? -x : 0, px1 = x + w > W ? W - x : w, py0 = y < 0 ? -y : 0, py1 = y + h > H ? H - y : h;

    for (int px = px0; px < px1; ++px) {                       /* a column at a time, the canvas' order, clipped once */
        uint16_t *p = sCanvas + (size_t)(x + px) * H + (H - 1 - (size_t)(y + py0));

        for (int py = py0; py < py1; ++py, --p) {
            uint8_t v = pixels[(y0 + py) * stride + x0 + px];
            if (v)
                *p = Tone(pal[v], on);
        }
    }
}

#include "3ds_ui_theme_impl.h"
#include "3ds_icons.h"
/* The skin keeps the Ascuas pieces (EmWell, EmKeyTab, EmSwitch, EmBezel) for the screens not converted to Llama; this one draws its own. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "3ds_touch_skin_impl.h"
#pragma GCC diagnostic pop

#define BTN_DEPTH 3                     /* the lip under a big button (BACK, SAVE's) */
#define COL_DEPTH 2                     /* and under the column's */

/* A pill: `rows` (h colours) in a shape w wide and h tall whose two ends are half discs (w >= h). One span a row. */
static void LhPill(int x, int y, int w, int h, const uint16_t *rows)
{
    for (int row = 0; row < h; ++row) {
        int dy = 2 * row + 1 - h, left = h;

        for (int col = 0; col < h; ++col) {
            int dx = 2 * col + 1 - h;

            if (dx * dx + dy * dy <= h * h) {
                left = col;
                break;
            }
        }
        if (left < h)
            Fill(x + left, y + row, w - 2 * left, 1, rows[row]);
    }
}

/* The mockup's switch (.sw): 30 x 15 with its outline (its box is 28 x 13), a slate outline, a glass track (green when on, grey when off) and
 * a 15 px knob, at the right when on and at the left when off. The knob is lit from its upper left (white, going to #d6d8e2 at 11 px). */
static void LhSwitch(int x, int y, int on)
{
    uint16_t rows[15];
    int kx = on ? x + 15 : x;

    for (int r = 0; r < 15; ++r)
        rows[r] = LL_SLATE;
    LhPill(x, y, 30, 15, rows);
    for (int r = 0; r < 13; ++r)
        rows[r] = on ? (r < 6 ? Em2Lerp(UI_RGB(166, 238, 174), UI_RGB(92, 200, 116), r, 5) : Em2Lerp(UI_RGB(60, 176, 92), UI_RGB(42, 148, 74), r - 6, 6))
                     : Em2Lerp(UI_RGB(216, 212, 220), UI_RGB(170, 166, 178), r, 12);
    LhPill(x + 1, y + 1, 28, 13, rows);
    Em2Disc(kx, y, 15, LL_SLATE);
    for (int row = 0; row < 13; ++row)
        for (int col = 0; col < 13; ++col) {
            int px = 2 * col + 1 - 13, py = 2 * row + 1 - 13, gx = col * 100 + 50 - 585, gy = row * 100 + 50 - 455, d;

            if (px * px + py * py > 13 * 13)
                continue;
            d = (int)LlIsqrt((unsigned)(gx * gx + gy * gy));
            Px(kx + 1 + col, y + 1 + row, d <= 385 ? EM_WHITE : Em2Lerp(EM_WHITE, UI_RGB(214, 216, 226), d - 385, 715));
        }
}

/* The frame round the GBA picture (x, y, w, h the picture): FireRed's window in the 3 px there is outside it: the slate
 * outline, the sky blue and the light blue next to the picture. Nothing inside the picture's rectangle. */
static void HomeFrame(int x, int y, int w, int h)
{
    UiRoundFill(x - 3, y - 3, w + 6, h + 6, 5, LL_SLATE);
    UiRoundFill(x - 2, y - 2, w + 4, h + 4, 4, UI_RGB(122, 180, 216));
    UiRoundFill(x - 1, y - 1, w + 2, h + 2, 3, UI_RGB(200, 228, 240));
}

/* The rectangle with the two rows the shadow of a raised piece under it takes. */
static CtrPokeTouchRect Under(const CtrPokeTouchRect *r)
{
    CtrPokeTouchRect u = *r;

    u.h = (int16_t)(u.h + 2);
    return u;
}

/* Shortcut item icons from FireRed's icon table, decoded once per item and
 * shown as the 20x20 around their drawn part. */
enum { ICON_REGISTERED, ICON_BIKE, ICON_BOX_HAND, ICON_BOX_NOTE, ICON_COUNT };

static struct {
    unsigned item;
    int ok, x0, y0;
    uint8_t pixels[24 * 24];
    uint16_t pal[16];
} sIcons[ICON_COUNT];

static int ItemIcon(int slot, unsigned item)
{
    static unsigned char tiles[0x200];
    static unsigned short pal[16];

    if (item == 0)
        return 0;
    if (sIcons[slot].item == item)
        return sIcons[slot].ok;
    sIcons[slot].item = item;
    sIcons[slot].ok = CtrPokeTouch_ItemIcon(item, tiles, pal);
    if (!sIcons[slot].ok)
        return 0;
    int left = 24, right = 0, top = 24, bottom = 0;
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 24; ++x) {
            unsigned byte = tiles[(y / 8 * 3 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2];
            uint8_t v = (uint8_t)((byte >> (x & 1) * 4) & 15);
            sIcons[slot].pixels[y * 24 + x] = v;
            if (v) {
                if (x < left) left = x;
                if (x >= right) right = x + 1;
                if (y < top) top = y;
                if (y >= bottom) bottom = y + 1;
            }
        }
    int x0 = left < right ? (left + right) / 2 - 10 : 2, y0 = top < bottom ? (top + bottom) / 2 - 10 : 2;
    sIcons[slot].x0 = x0 < 0 ? 0 : x0 > 4 ? 4 : x0;
    sIcons[slot].y0 = y0 < 0 ? 0 : y0 > 4 ? 4 : y0;
    for (int i = 0; i < 16; ++i) {
        unsigned c = pal[i];
        sIcons[slot].pal[i] = GbaColor(c);
    }
    return 1;
}

static void ItemArt(int slot, unsigned item, int x, int y, int on)
{
    if (ItemIcon(slot, item))
        Art(sIcons[slot].pixels, sIcons[slot].pal, 24, sIcons[slot].x0, sIcons[slot].y0, 20, 20, x, y, on);
}

/* The HOME map: the 240x160 screen of FireRed's Region Map (its tilemap, tiles and palette, decoded by the game bridge)
 * is read once per map and version, and the 192x128 of it HOME's window shows (PT_HOME_SRC_X, PT_HOME_SRC_Y on) is kept
 * at 1:1, laid out as the canvas is, so HOME copies it a column at a time (3ds_device_home_impl.h). Nothing is read from
 * the game while the player walks. */
extern int CtrPokeTouch_MapArt(int group, uint16_t *tilemap, uint8_t *tiles, uint16_t *palette, uint16_t *bgTilemap,
                               uint8_t *bgTiles);

#define MAP_TILEMAP_W 30
#define MAP_TILEMAP_H 20
#define MAP_TILES 320
#define MAP_COLORS 80

/* A GBA colour (15 bits, red lowest) as RGB565. */
static uint16_t Gba(uint16_t c)
{
    unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    return (uint16_t)(r << 11 | (g << 1 | g >> 4) << 5 | b);
}

/* The palette entry the Region Map's screen shows at its pixel (px, py): the map's layer over the
 * background's; index 0 is clear in both, and the screen shows the backdrop, colour 0 of palette 0,
 * only where both are. */
static unsigned MapColor(const uint16_t *tilemap, const uint8_t *tiles, const uint16_t *bgTilemap, const uint8_t *bgTiles,
                         int px, int py)
{
    int tx = px / 8, ty = py / 8, x = px % 8, y = py % 8;
    unsigned entry[2] = {tilemap[ty * MAP_TILEMAP_W + tx], bgTilemap[ty * MAP_TILEMAP_W + tx]};

    for (int layer = 0; layer < 2; ++layer) {
        unsigned tile = entry[layer] & 0x3FF, bank = entry[layer] >> 12, index = 0;
        int sx = entry[layer] & 0x400 ? 7 - x : x, sy = entry[layer] & 0x800 ? 7 - y : y;
        if (tile < MAP_TILES) {
            uint8_t byte = (layer ? bgTiles : tiles)[tile * 32 + sy * 4 + sx / 2];
            index = (sx & 1 ? byte >> 4 : byte) & 15;
        }
        if (index && bank * 16 + index < MAP_COLORS)
            return bank * 16 + index;
    }
    return 0;
}

static struct {
    int group;                          /* the map composed, -1 none */
    int version;                        /* and the version of its art */
    int ok;                             /* the game had its art */
} sMap = {-1, 0, 0};

/* HOME's window of the map: each column from its bottom pixel up, as the canvas holds it. */
static uint16_t sHomeMapCols[PT_HOME_W][PT_HOME_H];

/* The window's picture of a map, composed once per (map, version) when it is asked for (HOME asks every frame, for the
 * map it shows); false when the game has no art for it. */
bool CtrPokeTouch_MapPrepare(int group, int version)
{
    static uint16_t tilemap[MAP_TILEMAP_W * MAP_TILEMAP_H], bgTilemap[MAP_TILEMAP_W * MAP_TILEMAP_H], gba[MAP_COLORS];
    static uint8_t tiles[MAP_TILES * 32], bgTiles[MAP_TILES * 32];

    if (sMap.group == group && sMap.version == version)
        return sMap.ok != 0;
    sMap.group = group;
    sMap.version = version;
    sMap.ok = group >= 0 && group < PT_MAP_COUNT && CtrPokeTouch_MapArt(group, tilemap, tiles, gba, bgTilemap, bgTiles);
    for (int dx = 0; sMap.ok && dx < PT_HOME_W; ++dx)
        for (int dy = 0; dy < PT_HOME_H; ++dy)
            sHomeMapCols[dx][PT_HOME_H - 1 - dy] =
                Gba(gba[MapColor(tilemap, tiles, bgTilemap, bgTiles, PT_HOME_SRC_X + dx, PT_HOME_SRC_Y + dy)]);
    return sMap.ok != 0;
}

/* The view's map is the one composed, and the view knows its art was there. */
static int MapReady(const CtrPokeTouchView *v)
{
    return v->mapReady && sMap.ok && sMap.group == v->mapGroup && sMap.version == v->mapVersion;
}

#define C_MAP_WAIT UI_RGB(156, 213, 255)        /* the window without the map's art: the sea's blue */

/* The map's two icons, FireRed's own 16x16 sprites (four 4bpp tiles in sprite
 * order, 0 clear): the player (Red or Leaf) and the selection cursor (two
 * frames, which blink). An icon is centred on its cell: 8x8 pixels at (32, 32)
 * + 8 per cell on the Region Map's screen, so its 16x16 is at (8 x + 28, 8 y + 28)
 * there. HOME draws both at 1:1; the cursor is above the player. */
extern int CtrPokeTouch_MapPlayerIcon(uint8_t *tiles, uint16_t *palette);
extern int CtrPokeTouch_MapCursorIcon(uint8_t *tiles, uint16_t *palette);

#define MARKER 16
#define MAP_CELLS_X 22
#define MAP_CELLS_Y 15

static struct Icon {
    int key;                            /* what is cached (gender), -1 none */
    int ok;
    int frames;
    uint8_t pixels[2][MARKER * MARKER]; /* palette index per frame */
    uint16_t pal[16];
} sPlayer = {-1, 0, 1, {{0}}, {0}};

static void IconBuild(struct Icon *icon, int key, int (*art)(uint8_t *, uint16_t *))
{
    uint8_t tiles[8 * 32];
    uint16_t gba[16];

    icon->key = key;
    memset(tiles, 0, sizeof(tiles));
    icon->ok = art(tiles, gba);
    if (!icon->ok)
        return;
    for (int i = 0; i < 16; ++i)
        icon->pal[i] = Gba(gba[i]);
    for (int frame = 0; frame < icon->frames; ++frame)
        for (int y = 0; y < MARKER; ++y)
            for (int x = 0; x < MARKER; ++x) {
                int tile = frame * 4 + y / 8 * 2 + x / 8, tx = x % 8, ty = y % 8;
                uint8_t byte = tiles[tile * 32 + ty * 4 + tx / 2];
                icon->pixels[frame][y * MARKER + x] = (tx & 1 ? byte >> 4 : byte) & 15;
            }
}

/* An icon (16x16) centred at (cx, cy) of the lower screen, clipped to HOME's map window. */
static void IconDraw(const struct Icon *icon, int frame, int cx, int cy)
{
    if (!icon->ok)
        return;
    for (int py = 0; py < MARKER; ++py)
        for (int px = 0; px < MARKER; ++px) {
            int x = cx - MARKER / 2 + px, y = cy - MARKER / 2 + py;
            uint8_t index = icon->pixels[frame % icon->frames][py * MARKER + px];

            if (index && x >= PT_HOME_X && y >= PT_HOME_Y && x < PT_HOME_X + PT_HOME_W && y < PT_HOME_Y + PT_HOME_H)
                Px(x, y, icon->pal[index]);
        }
}

static const char *RegionName(int group)
{
    return group == PT_MAP_KANTO ? "Kanto" : group == PT_MAP_SEVII123 ? "Sevii 1-3" : group == PT_MAP_SEVII45 ? "Sevii 4-5" : "Sevii 6-7";
}

/* HOME's region key (only when it leads to another map) and FLY where a touch takes them, as last drawn (w 0: not
 * there), while HOME is the last thing drawn (3ds_device_home_impl.h). */
static CtrPokeTouchRect sHomeRibbon, sFlyButton;
static int sHomeShown;

bool CtrPokeTouch_HomeRibbonHit(int x, int y)
{
    return sHomeShown && sHomeRibbon.w && UiIn(&sHomeRibbon, x, y);
}

bool CtrPokeTouch_FlyHit(int x, int y)
{
    return sHomeShown && sFlyButton.w && UiIn(&sFlyButton, x, y);
}

/* The backdrop. */
static void Frame(void)
{
    TsBackdrop();
}

/* Where the frame goes round the GBA picture. */
static int FrameShown(const CtrPokeTouchView *v)
{
    return v->viewport;
}

/* Easy Chat, over the whole screen (3ds_field_easychat_impl.h, included at the end). */
static void FEasyDraw(const CtrPokeTouchView *v);
static void FEasyUpdate(uint16_t *canvas, const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty);
/* The PC box the port draws (3ds_field_box_impl.h, included at the end). */
static void FBoxFrame(const CtrPokeTouchView *v);
static void FBoxNote(const CtrPokeTouchView *v, int w);

/* The frame's area: the window frame round the GBA picture, or an ember view's screen (HOME, the Bag, OPTIONS, SAVE, the
 * Trainer Card, the letter, the Pokedex, the move to forget, the relearner, a choice and the naming keyboard are the red
 * device's: 3ds_device_home_impl.h, 3ds_device_bag_impl.h and the field dispatcher's views). */
static void MenuFrame(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &sMenuFrame;

    TsBgRect(r->x, r->y, r->w, r->h);
    if (v->box.on && !v->viewport) {
        FBoxFrame(v);
        return;
    }
    if (!FrameShown(v))
        return;
    HomeFrame(PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H);
    Fill(PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H, 0);   /* covered by the GPU's picture */
}

/* BACK: the slate button of going back ("<" and its word at the left, as the mockup draws it); pressed it sinks, disabled it is grey. */
static void Back(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &gCtrPokeTouchBack;
    const EmPal *pal = v->backEnabled ? &kEmSlate : &kEmGrey;
    int fy, fh = r->h - BTN_DEPTH;

    TsBgRect(r->x, r->y, r->w, r->h + 2);
    Em2CardShadow(r->x, r->y, r->w, r->h);
    fy = EmButton(r->x, r->y, r->w, r->h, pal, BTN_DEPTH, v->backEnabled && v->backPressed);
    LlArrow(r->x + 12, fy + (fh - 10) / 2, 6, 10, 2, EM_WHITE);
    UiText(CTR_TR("Back", "Atr\xC3\xA1s", "Zur\xc3\xbc" "ck"), r->x + 24, fy + (fh - UI_TEXT_H) / 2, pal->ink, pal->inkSh, 1, r->x + r->w - 3);
}

/* The bar: the note card with up to three lines, 15 pixels apart and centred on it; the first line is the subject,
 * the others what is known about it (dimmer). It is narrower when BACK is on its right. */
static void Bar(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &sBar;
    int lines = 0;
    int w = v->backVisible ? NOTE_W_SIDE : NOTE_W_FULL;

    TsBgRect(r->x, r->y, r->w, r->h);
    for (int i = 0; i < PT_BAR_LINES; ++i)
        if (v->bar[i][0])
            lines = i + 1;
    if (!v->viewport && v->box.on)
        Em2NoteShadow(NOTE_X, NOTE_Y, w, NOTE_H);           /* the screen that draws its own note (it puts the shadow its note throws) */
    if (v->box.on && !v->viewport)
        FBoxNote(v, w);
    else {
        /* The note is FireRed's window (the Llama mockup's): the subject at its top left, a rule under it, then the lines that say more, 14 apart,
         * dimmer. */
        int right = NOTE_X + w - 12;

        LlWindow(NOTE_X, NOTE_Y, w, NOTE_H);
        if (lines == 1) {
            Em2Text(v->bar[0], NOTE_X + 13, NOTE_Y + (NOTE_H - UI_TEXT_H) / 2, EM_DARK, EM_DARK_SH, right);
        } else if (lines > 1) {
            Em2Text(v->bar[0], NOTE_X + 13, NOTE_Y + 11, EM_DARK, EM_DARK_SH, right);
            LlRule(NOTE_X + 13, NOTE_Y + 26, right - NOTE_X - 13);
            for (int i = 1; i < lines; ++i)
                Em2Text(v->bar[i], NOTE_X + 13, NOTE_Y + 32 + (i - 1) * 14, EM_DIM, EM_DARK_SH, right);
        }
    }
    if (v->backVisible)
        Back(v);
}

/* An element's area: its rectangle and the ring (or the shadow) round it. */
static CtrPokeTouchRect Area(int id)
{
    return UiInset(&gCtrPokeTouchRects[id], -2);
}

/* The column's icons, by button. */
static int ColumnIcon(int id)
{
    return id;          /* kEmberIcon is in the order PT_POKEDEX .. PT_OPTIONS */
}

/* A START entry's word (PT_POKEDEX..PT_OPTIONS): the entry's, or for the Trainer Card the player's name as typed (into
 * `name`). The column's buttons and HOME's keys say the same. */
static const char *ColumnLabel(int id, const CtrPokeTouchView *v, char name[16])
{
    const char *label = id == PT_POKEDEX ? CTR_TR("Pok\xC3\xA9" "dex", "Pok\xC3\xA9" "dex", "Pok\xc3\xa9" "dex")
                      : id == PT_POKEMON ? CTR_TR("Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xc3\xa9" "mon")
                      : id == PT_BAG ? CTR_TR("Bag", "Mochila", "Beutel")
                      : id == PT_SAVE ? CTR_TR("Save", "Guardar", "Sichern") : id == PT_OPTIONS ? CTR_TR("Option", "Ajustes", "Option") : name;
    int n = 0;

    if (id == PT_CARD)
        for (; n < 15 && v->playerName[n]; ++n)
            name[n] = v->playerName[n];
    name[n] = 0;
    return label;
}

/* A button: cream, flame and ringed when selected (the screen shown, or the running mode on), sunk when pressed, grey when it cannot be used.
 * The column's carry their icon and label (the Trainer Card the player's name), the shortcuts their key on a blue ribbon and their item in
 * its well, RUN its switch. */
static void Button(int id, const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &gCtrPokeTouchRects[id];
    CtrPokeTouchRect area = Area(id);
    int on = v->enabled[id] || v->selected == id;
    int pressed = v->enabled[id] && v->pressed == id;
    int selected = (v->selected == id && id != PT_RUN) || (id == PT_RUN && v->enabled[id] && v->running);
    const EmPal *pal = !on ? &kEmGrey : id == PT_RUN ? &kEmCream : LlPick(selected, &kEmCream);
    int fy, fh = r->h - COL_DEPTH;

    TsBgRect(area.x, area.y, area.w, area.h);
    if (selected && id != PT_RUN && on)
        LlSelectRing(r->x, r->y, r->w, r->h);
    else
        Em2CardShadow(r->x, r->y, r->w, r->h);
    fy = EmButton(r->x, r->y, r->w, r->h, pal, COL_DEPTH, pressed);
    switch (id) {
    case PT_REGISTERED:
    case PT_BIKE: {
        const char *key = id == PT_REGISTERED ? "X" : "Y";

        LlRibbonBox(r->x + 1, fy + 1, UiTextWidth(key, 1) + 18, 11, 7, LL_TL, LL_BLUE_TOP, LL_BLUE_BOT, key);
        LlWell(r->x + 3, fy + 15, r->w - 6, fh - 18);
        ItemArt(id == PT_REGISTERED ? ICON_REGISTERED : ICON_BIKE, id == PT_REGISTERED ? v->registeredItem : v->bikeItem,
                r->x + (r->w - 20) / 2, fy + 15 + (fh - 18 - 20) / 2, on);
        break;
    }
    case PT_RUN:
        /* The battle menu's RUN shoe, small, beside the switch. */
        for (int v2 = 0; v2 < FLATSHOESMALL_H; ++v2)
            for (int u = 0; u < FLATSHOESMALL_W; ++u)
                if (kFlatShoeSmall[v2 * FLATSHOESMALL_W + u])
                    Px(r->x + 5 + u, fy + (fh - FLATSHOESMALL_H) / 2 + v2, kFlatShoeSmall[v2 * FLATSHOESMALL_W + u]);
        LhSwitch(r->x + 34, fy + (fh - 15) / 2, selected);
        break;
    default: {
        char name[16];
        const char *label = ColumnLabel(id, v, name);

        EmIcon(ColumnIcon(id), r->x + 4, fy + (fh - EMBER_ICON) / 2, on);
        UiText(label, r->x + 23, fy + (fh - UI_TEXT_H) / 2, pal->ink, pal->inkSh, 1, r->x + r->w - 2);
        break;
    }
    }
}

int CtrPokeTouch_HitTest(int x, int y)
{
    for (int i = 0; i < PT_COUNT; ++i) {
        const CtrPokeTouchRect *r = &gCtrPokeTouchRects[i];
        if (x >= r->x && y >= r->y && x < r->x + r->w && y < r->y + r->h)
            return i;
    }
    return PT_NONE;
}

/* The viewport shows the GBA picture 1:1. */
bool CtrPokeTouch_ViewportToGba(int x, int y, int *gx, int *gy)
{
    x -= PT_VIEW_X;
    y -= PT_VIEW_Y;
    if (x < 0 || y < 0 || x >= PT_VIEW_W || y >= PT_VIEW_H)
        return false;
    *gx = x;
    *gy = y;
    return true;
}

/* HOME, the red device's (3ds_device_home_impl.h, included at the end). */
static int DevHomeShown(const CtrPokeTouchView *v);
static void DevHomeDraw(const CtrPokeTouchView *v);
static void DevHomeUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty);
/* The field's Bag, its cases, a mart's BUY, the item PC, the mailbox: the red device's (3ds_device_bag_impl.h). */
static int DevBagShown(const CtrPokeTouchView *v);
static void DevBagFieldDraw(const CtrPokeTouchView *v);
static void DevBagFieldUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty);
/* The field's views without a board drawn as the red device, through one table: the field dispatcher
 * (3ds_device_field_impl.h, included at the end). 0 when `v` is none of them. */
static int DevFieldDraw(const CtrPokeTouchView *v);
static int DevFieldUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty);

void CtrPokeTouch_Draw(uint16_t *canvas, const CtrPokeTouchView *v)
{
    sCanvas = canvas;
    if (DevHomeShown(v)) {
        DevHomeDraw(v);
        return;
    }
    sHomeShown = 0;                     /* HOME's region key and FLY are not on the screen */
    if (DevBagShown(v)) {
        DevBagFieldDraw(v);
        return;
    }
    if (DevFieldDraw(v))
        return;
    if (v->ec.on) {
        FEasyDraw(v);
        return;
    }
    Frame();
    MenuFrame(v);
    Bar(v);
    for (int i = 0; i < PT_COUNT; ++i)
        Button(i, v);
}

/* Whether element `id` looks different between two views. */
static int Changed(int id, const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    if (a->enabled[id] != b->enabled[id] || (a->pressed == id) != (b->pressed == id)
        || (a->selected == id) != (b->selected == id))
        return 1;
    return (id == PT_RUN && a->running != b->running)
        || (id == PT_REGISTERED && a->registeredItem != b->registeredItem)
        || (id == PT_BIKE && a->bikeItem != b->bikeItem)
        || (id == PT_CARD && strcmp(a->playerName, b->playerName));
}

static void Mark(CtrDirtyList *dirty, const CtrPokeTouchRect *r)
{
    CtrDirty_Add(dirty, r->x, r->y, r->x + r->w, r->y + r->h);
}

void CtrPokeTouch_Update(uint16_t *canvas, const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)
{
    sCanvas = canvas;
    if (DevHomeShown(old) && DevHomeShown(v)) {
        DevHomeUpdate(old, v, dirty);
        return;
    }
    if (DevHomeShown(old) || DevHomeShown(v)) {
        CtrPokeTouch_Draw(canvas, v);
        CtrDirty_Add(dirty, 0, 0, W, H);
        return;
    }
    if (DevBagShown(old) && DevBagShown(v)) {
        DevBagFieldUpdate(old, v, dirty);
        return;
    }
    if (DevBagShown(old) || DevBagShown(v)) {
        CtrPokeTouch_Draw(canvas, v);
        CtrDirty_Add(dirty, 0, 0, W, H);
        return;
    }
    if (DevFieldUpdate(old, v, dirty))
        return;
    if (v->ec.on || old->ec.on) {
        FEasyUpdate(canvas, old, v, dirty);
        return;
    }
    if (memcmp(&old->box, &v->box, sizeof(v->box))) {
        /* The PC box: its whole area and its note (the note follows the selection). */
        MenuFrame(v);
        Mark(dirty, &sMenuFrame);
        Bar(v);
        Mark(dirty, &sBar);
    } else if (old->viewport != v->viewport || old->home != v->home) {
        MenuFrame(v);
        Mark(dirty, &sMenuFrame);
    }
    if (memcmp(old->bar, v->bar, sizeof(v->bar)) || old->backVisible != v->backVisible) {
        Bar(v);
        Mark(dirty, &sBar);
    } else if (v->backVisible && (old->backEnabled != v->backEnabled || old->backPressed != v->backPressed)) {
        CtrPokeTouchRect back = Under(&gCtrPokeTouchBack);

        Back(v);                            /* a press or a state: only BACK's rectangle and its shadow */
        Mark(dirty, &back);
    }
    for (int i = 0; i < PT_COUNT; ++i) {
        if (!Changed(i, old, v))
            continue;
        Button(i, v);
        {
            CtrPokeTouchRect area = Area(i);

            Mark(dirty, &area);
        }
    }
}

#include "idle_logo.h"

/* The lower screen while FireRed's screen above has nothing for it: the ember backdrop and the project's logo. */
void CtrPokeTouch_DrawIdle(uint16_t *canvas, int logo)
{
    int x0 = (W - IDLE_LOGO_W) / 2, y0 = (H - IDLE_LOGO_H_PX) / 2 - 6;

    sCanvas = canvas;
    sHomeShown = 0;
    TsBackdrop();
    for (int y = 0; logo && y < IDLE_LOGO_H_PX; ++y)
        for (int x = 0; x < IDLE_LOGO_W; ++x) {
            int k = kIdleLogo[y * IDLE_LOGO_W + x], a = kIdleLogoAlpha[k];
            unsigned c = kIdleLogoColour[k], b;

            if (a == 0)
                continue;
            if (a < 15) {                          /* a/15 of the logo over the backdrop, channel by channel */
                b = TsGet(x0 + x, y0 + y);
                c = ((((c >> 11) * a + (b >> 11) * (15 - a)) / 15) << 11)
                  | (((((c >> 5) & 63) * a + ((b >> 5) & 63) * (15 - a)) / 15) << 5)
                  | (((c & 31) * a + (b & 31) * (15 - a)) / 15);
            }
            Px(x0 + x, y0 + y, (uint16_t)c);
        }
}

#include "3ds_battle_ui_impl.h"
#include "3ds_device_kit_impl.h"   /* the red device (style B): its kit, for the B views below */
#include "3ds_device_home_impl.h"  /* HOME */
#include "3ds_device_party_impl.h"  /* the Party: bays, its menu, YES / NO, a message, the level up */
#include "3ds_device_bag_impl.h"  /* the Bag, its cases, a mart's BUY, the item PC, the mailbox, the battle's Bag */
#include "3ds_device_battle_impl.h"  /* the battle: commands, moves, targets, Safari, idle */
#include "3ds_device_summary_impl.h"  /* the Summary: INFO, STATS (IV / EV), MOVES, the move details, MEMO */
#include "3ds_device_options_impl.h"  /* OPTIONS (a view of the field dispatcher) */
#include "3ds_device_save_impl.h"  /* SAVE (a view of the field dispatcher) */
#include "3ds_device_card_impl.h"  /* the Trainer Card (a view of the field dispatcher; FireRed's tiles and palettes) */
#include "3ds_device_mail_impl.h"  /* the letter being read (a view of the field dispatcher) */
#include "3ds_device_dex_impl.h"  /* the Pokedex (a view of the field dispatcher; the species' pictures) */
#include "3ds_device_learn_impl.h"  /* the move to forget (a view of the field dispatcher; a move's type chip) */
#include "3ds_device_relearn_impl.h"  /* the relearner (a view of the field dispatcher) */
#include "3ds_device_choice_impl.h"  /* a choice of the field (a view of the field dispatcher) */
#include "3ds_device_naming_impl.h"  /* the naming keyboard (a view of the field dispatcher; FireRed's codes as text) */
#include "3ds_field_bag_impl.h"
#include "3ds_field_box_impl.h"
#include "3ds_field_easychat_impl.h"
#include "3ds_device_field_impl.h"  /* the field dispatcher: the field's views without a board (last: it names them all) */
