/* PokeTouch: the bottom screen, drawn and hit-tested.
 *
 * Every view is a full-screen page in FireRed's own look (phase 2 of
 * docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md): HOME, the BAG, and the field's other views (the
 * Pokedex, OPTIONS, SAVE, the Trainer Card, the PC box, the field's choices, forgetting a move, the naming keyboard,
 * FireRed's own picture in a frame), drawn with the kit (3ds_lower_kit_impl.h: FireRed's window frames and font from
 * the player's pack) where 3ds_lower_layout.h puts them, and only when the view changes.
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
 * releases use (U+00A1..U+00FF and Œ/œ): accents, Ñ/ñ, ¿, ¡, º, ª, ¥. */
static unsigned LatinCode(unsigned char lead, unsigned char next)
{
    static const uint8_t sC3[64] = {
        /* U+00C0 À Á Â Ã Ä Å Æ Ç */ 0x01, 0x02, 0x03, 0, 0xf1, 0, 0, 0x04,
        /* U+00C8 È É Ê Ë Ì Í Î Ï */ 0x05, 0x06, 0x07, 0x08, 0x09, 0x5a, 0x0b, 0x0c,
        /* U+00D0 Ð Ñ Ò Ó Ô Õ Ö × */ 0, 0x14, 0x0d, 0x0e, 0x0f, 0, 0xf2, 0,
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
        case '\'': code=0xb4; break; case '"': code=0xb2; break;   /* FireRed's closing quotes: KING'S ROCK, 2'00" */
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
    for (int py = 0; py < h; ++py)
        for (int px = 0; px < w; ++px) {
            uint8_t v = pixels[(y0 + py) * stride + x0 + px];
            if (v)
                Px(x + px, y + py, Tone(pal[v], on));
        }
}

#include "3ds_ui_theme_impl.h"
#include "3ds_icons.h"
#include "3ds_touch_skin_impl.h"
#include "3ds_lower_kit_impl.h"
#include "3ds_lower_layout.h"

/* The HOME map: the 240x160 screen of FireRed's Region Map (its tilemap,
 * tiles and palette, decoded by the game bridge) composed once per map into
 * a cache laid out like the canvas: column-major, each column bottom-to-top,
 * so showing it is one copy per column. Nothing is read from the game while
 * the player walks; only the cache is. */
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

/* The map's two icons, FireRed's own 16x16 sprites (four 4bpp tiles in sprite
 * order, 0 clear): the player (Red or Leaf) and the selection cursor (two
 * frames, which blink). An icon is centred on its cell: 8x8 pixels at (32, 32)
 * + 8 per cell on the Region Map's screen, so its 16x16 is at (8 x + 28, 8 y + 28)
 * there. HOME enlarges both by the map's 5/4 (nearest neighbour): 20x20 icons
 * on 10x10 cells. The cursor is above the player. */
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
} sPlayer = {-1, 0, 1, {{0}}, {0}}, sCursor = {-1, 0, 2, {{0}}, {0}};

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

/* The field Bag the port draws (3ds_field_bag_impl.h, included at the end). */
static void BagPageDraw(const CtrPokeTouchView *v);
/* The field's pages (3ds_field_page_impl.h and the views' headers, included at the end). */
static void FieldPageDraw(const CtrPokeTouchView *v);

/* A text window's file from the pack (graphics/text_window/<base>.<ext>, raw or LZ): `want` bytes, 0 without it. */
static int FrameLoadFile(const char *base, const char *ext, uint8_t *out, uint32_t cap, uint32_t want)
{
    char path[64];
    uint32_t len = 0;
    uint8_t *data;
    int n = 0;

    snprintf(path, sizeof(path), "graphics/text_window/%s.%s", base, ext);
    data = CtrData_Load(path, &len);
    if (data != NULL && len == want) {
        memcpy(out, data, want);
        n = (int)want;
    }
    free(data);
    if (n == (int)want)
        return n;
    snprintf(path, sizeof(path), "graphics/text_window/%s.%s.lz", base, ext);
    return PokeTouch_LoadLz(path, out, cap) == (int)want ? (int)want : 0;
}

/* The label of a menu entry PT_POKEDEX .. PT_OPTIONS; the Trainer Card's is the player's name (in original case for
 * modern Title Case, or uppercase otherwise), written into `name`. */
static const char *ColumnLabel(int id, const CtrPokeTouchView *v, char name[16])
{
    if (id == PT_CARD) {
        int n = 0;

        for (; n < 15 && v->playerName[n]; ++n) {
            char ch = v->playerName[n];
#if CTR_LANG_DE
            name[n] = ch;
#else
            name[n] = (char)(ch >= 'a' && ch <= 'z' ? ch - ('a' - 'A') : ch);
#endif
        }
        name[n] = 0;
        return name;
    }
    return id == PT_POKEDEX ? CTR_TR("POK\xc3\xa9" "DEX", "POK\xc3\xa9" "DEX", "Pok\xc3\xa9" "dex")
         : id == PT_POKEMON ? CTR_TR("POK\xc3\xa9" "MON", "POK\xc3\xa9" "MON", "Pok\xc3\xa9" "mon")
         : id == PT_BAG ? CTR_TR("BAG", "MOCHILA", "Beutel")
         : id == PT_SAVE ? CTR_TR("SAVE", "GUARDAR", "Sichern") : id == PT_OPTIONS ? CTR_TR("OPTION", "AJUSTES", "Option") : "";
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

static void HomePageDraw(const CtrPokeTouchView *v);
static void HomePageUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty);

void CtrPokeTouch_Draw(uint16_t *canvas, const CtrPokeTouchView *v)
{
    sCanvas = canvas;
    if (CtrPokeTouch_HomePageShown(v)) {
        HomePageDraw(v);
        return;
    }
    if (CtrPokeTouch_BagPageShown(v)) {
        BagPageDraw(v);
        return;
    }
    FieldPageDraw(v);
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

#include "3ds_home_page_impl.h"

void CtrPokeTouch_Update(uint16_t *canvas, const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)
{
    sCanvas = canvas;
    if (CtrPokeTouch_HomePageShown(old) || CtrPokeTouch_HomePageShown(v)) {
        /* HOME's page: its own elements while it stays up; any change to or from it repaints the whole screen. */
        if (CtrPokeTouch_HomePageShown(old) && CtrPokeTouch_HomePageShown(v)) {
            HomePageUpdate(old, v, dirty);
        } else {
            CtrPokeTouch_Draw(canvas, v);
            CtrDirty_Add(dirty, 0, 0, W, H);
        }
        return;
    }
    if (CtrPokeTouch_BagPageShown(old) || CtrPokeTouch_BagPageShown(v)) {
        /* The Bag's page: drawn again whole when anything on it changes (the description follows the selection). */
        if (!CtrPokeTouch_BagPageShown(old) || !CtrPokeTouch_BagPageShown(v) || old->frameType != v->frameType
            || memcmp(&old->bag, &v->bag, sizeof(v->bag))) {
            CtrPokeTouch_Draw(canvas, v);
            CtrDirty_Add(dirty, 0, 0, W, H);
        }
        return;
    }
    /* A field page: drawn again whole when anything it shows changes (a selection moves its description). */
    if (memcmp(old, v, sizeof(*v))) {
        CtrPokeTouch_Draw(canvas, v);
        CtrDirty_Add(dirty, 0, 0, W, H);
    }
}

#include "idle_logo.h"

/* The lower screen while FireRed's screen above has nothing for it (phase 2): the pages' colour and, with `logo`, the
 * project's logo in a panel with FireRed's dialogue frame in the middle (3ds_lower_layout.h kIdlePanel). */
void CtrPokeTouch_DrawIdle(uint16_t *canvas, int logo)
{
    const CtrPokeTouchRect *p = &kIdlePanel;
    int x0 = p->x + (p->w - IDLE_LOGO_W) / 2, y0 = p->y + (p->h - IDLE_LOGO_H_PX) / 2;

    sCanvas = canvas;
    Fill(0, 0, W, H, KitInk(3));
    if (logo)
        KitPanel(p, KIT_FRAME_DIALOGUE, 0);
    for (int y = 0; logo && y < IDLE_LOGO_H_PX; ++y)
        for (int x = 0; x < IDLE_LOGO_W; ++x) {
            int k = kIdleLogo[y * IDLE_LOGO_W + x], a = kIdleLogoAlpha[k];
            unsigned c = kIdleLogoColour[k], b;

            if (a == 0)
                continue;
            if (a < 15) {                          /* a/15 of the logo over the panel, channel by channel */
                b = sCanvas[(size_t)(x0 + x) * H + (H - 1 - (size_t)(y0 + y))];
                c = ((((c >> 11) * a + (b >> 11) * (15 - a)) / 15) << 11)
                  | (((((c >> 5) & 63) * a + ((b >> 5) & 63) * (15 - a)) / 15) << 5)
                  | (((c & 31) * a + (b & 31) * (15 - a)) / 15);
            }
            Px(x0 + x, y0 + y, (uint16_t)c);
        }
}

#include "3ds_battle_ui_impl.h"
#include "3ds_summary_ui_impl.h"
#include "3ds_field_bag_impl.h"
#include "3ds_field_page_impl.h"
#include "3ds_options_page_impl.h"
#include "3ds_view_page_impl.h"
#include "3ds_field_dex_impl.h"
#include "3ds_field_card_impl.h"
#include "3ds_field_learn_impl.h"
#include "3ds_field_choice_impl.h"
#include "3ds_field_naming_impl.h"
#include "3ds_field_box_impl.h"

/* ---- The field's pages: which view is one, its BACK, its drawing ---- */
/* Every view but HOME and the BAG: the field's views each have a page, FireRed's picture has its frame's. */
bool CtrPokeTouch_FieldPageShown(const CtrPokeTouchView *v)
{
    return !CtrPokeTouch_HomePageShown(v) && !CtrPokeTouch_BagPageShown(v);
}

/* BACK's cell on the page shown (none without the view's BACK, or while the page has no room for it). */
static const CtrPokeTouchRect *FieldPageBack(const CtrPokeTouchView *v)
{
    if (!v->backVisible)
        return NULL;
    if (v->viewport)
        return &kViewBack;
    if (v->dex.on)
        return FDexBackRect(v);
    if (v->card.on)
        return &kCardBack;
    if (v->box.on)
        return FXBackRect(v);
    if (v->choice.on)
        return NULL;                    /* the menu's own rows, and a tap off its window, are its B */
    if (v->learn.on)
        return &kLearnBack;
    if (v->name.on)
        return NULL;                    /* DELETE is its B, OK its end */
    if (v->save)
        return NULL;                    /* NO and B leave SAVE */
    if (v->options)
        return &kOptBack;
    return &kViewBack;
}

static void FieldPageDraw(const CtrPokeTouchView *v)
{
    if (v->viewport)
        ViewPageDraw(v);
    else if (v->dex.on)
        FDexPageDraw(v);
    else if (v->card.on)
        FCardPageDraw(v);
    else if (v->box.on)
        FBoxPageDraw(v);
    else if (v->choice.on)
        FChoicePageDraw(v);
    else if (v->learn.on)
        FLearnPageDraw(v);
    else if (v->name.on)
        FNamePageDraw(v);
    else if (v->save)
        SavePageDraw(v);
    else if (v->options)
        OptionsPageDraw(v);
    else
        ViewPageDraw(v);
}

bool CtrPokeTouch_BackHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchRect *r = CtrPokeTouch_FieldPageShown(v) ? FieldPageBack(v) : NULL;

    return r && UiIn(r, x, y);
}
