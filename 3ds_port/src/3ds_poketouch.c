/* PokeTouch: the bottom screen, drawn and hit-tested.
 *
 * Every view is the red device's (style B, its kit: 3ds_device_kit_impl.h). With no menu open the lower screen is HOME
 * (3ds_device_home_impl.h): FireRed's Region Map at 1:1 with the player on it, the party, RUN, the game's speed and the
 * six START entries as keys. Then the battle, the Party, the Summary, the Bag, and the field's other views through one
 * table, the field dispatcher (3ds_device_field_impl.h), whose last row shows FireRed's own picture (the GBA's 240x160,
 * drawn 1:1 by the GPU, 3ds_video.c) in an embedded screen.
 *
 * Art: FireRed's own, read from the game or the user's pack (its latin_normal font, its item icons, the Region Map),
 * never compiled in. Drawn only when the view changes.
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


/* HOME's six START entries' icons: these item graphics of the user's FireRed ROM, read from classicfirered3ds.pak
 * (3ds_device_home_impl.h draws their soft 16x16). No game pixels or palettes are compiled into the port. */
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

/* The canvas being drawn: the red device's kit (3ds_device_kit_impl.h) and the old theme's few components
 * (3ds_ui_theme_impl.h) draw on it. */
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
 * releases use (U+00A1..U+00FF and Œ/œ): accents, Ñ/ñ, ¿, ¡, º, ª, ¥, ×, ·. */
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
        case 0xb7: return 0xaf;   /* · */
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
    unsigned code = GlyphCode(s);

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

/* A colour of art as drawn: as it is when on, a light grey (its luma, lightened) when disabled. */
static uint16_t Tone(uint16_t c, int on)
{
    if (on)
        return c;
    unsigned r = c >> 11, g = (c >> 5) & 63, b = c & 31;
    unsigned luma = (r * 2 * 77 + g * 150 + b * 2 * 29) >> 8; /* 0..63 */
    luma = 34 + luma * 3 / 8;
    return (uint16_t)((luma >> 1) << 11 | luma << 5 | (luma >> 1));
}

#include "3ds_ui_theme_impl.h"
#include "3ds_icons.h"

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
/* Every other view, drawn as the red device through one table: the field dispatcher (3ds_device_field_impl.h, included
 * at the end), whose last row, FireRed's picture in its screen, takes any view the rows before it do not. */
static void DevFieldDraw(const CtrPokeTouchView *v);
static void DevFieldUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty);

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
    DevFieldDraw(v);
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
    DevFieldUpdate(old, v, dirty);
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
#include "3ds_device_box_impl.h"  /* the PC box (a view of the field dispatcher; the wallpaper, the icons) */
#include "3ds_device_easychat_impl.h"  /* Easy Chat (the field dispatcher's first row; the whole screen) */
#include "3ds_device_frame_impl.h"  /* FireRed's picture in its screen (the dispatcher's last row), the idle screen */
#include "3ds_device_field_impl.h"  /* the field dispatcher: the field's views without a board (last: it names them all) */
