/* The battle's healthboxes, drawn by the port (see include/3ds_battle_hud.h).
 *
 * The look (the fork owner's "Caixas 2"): the lower screen's window, that is the pack's standard frame (OPTIONS ->
 * FRAME) around white, FireRed's text colours; the foe's Poke Ball before its name in wild battles (red when its
 * species is caught); "HP" and "EXP" on dark tags before flat bars in FireRed's colours; the player's HP numbers
 * beside its EXP bar; a status as a coloured pill with its own icon. Every pixel is drawn here, at the screen's own
 * resolution (the battle's 1.5x zoom would render FireRed's own boxes unevenly); text is the game's font. */
#include <math.h>
#include <string.h>

#include "3ds_battle_hud.h"
#include "3ds_lang.h"
#include "3ds_video.h"

/* The level mark before the number: FireRed's "Lv", the Spanish game's "Nv". */
#define LEVEL CTR_TR("Lv", "Nv", "Lv")

/* The game's fonts (text.c, recipes 0096 and 0120): 16x16 two-bit glyphs and 8x16 ones, and their widths. */
extern const uint8_t *CtrTwinFontData(void);
extern unsigned CtrTwinFontWidth(unsigned code);
extern const uint8_t *CtrTwinSmallFontData(void);
extern unsigned CtrTwinSmallFontWidth(unsigned code);

#define EOS 0xFF

typedef struct { uint8_t r, g, b; } Rgb;
typedef struct {
    uint32_t *px;
    int w, h;
} Canvas;

static const Rgb OUTLINE = {28, 30, 44};
/* FireRed's window text (stdpal_0's 2 and 3) once the pack gives it; these until then. */
static Rgb sInk = {96, 96, 96}, sInkShadow = {216, 216, 208};

static void Put(Canvas *c, int x, int y, Rgb v)
{
    if (x >= 0 && y >= 0 && x < c->w && y < c->h)
        c->px[y * CTR_HUD_IMAGE_W + x] = (uint32_t)v.r << 24 | (uint32_t)v.g << 16 | (uint32_t)v.b << 8 | 255u;
}

/* ---- Text --------------------------------------------------------------------------------------------------------- */

static const uint8_t *Glyph(bool small, unsigned code, unsigned *width)
{
    const uint8_t *font = small ? CtrTwinSmallFontData() : CtrTwinFontData();
    unsigned w = small ? CtrTwinSmallFontWidth(code) : CtrTwinFontWidth(code);
    unsigned limit = small ? 8 : 16;

    *width = w > limit ? limit : w;
    return font ? font + code * (small ? 32 : 64) : NULL;
}

static unsigned GlyphValue(bool small, const uint8_t *src, unsigned x, unsigned y)
{
    unsigned tile = small ? y / 8 : (y / 8) * 2 + x / 8, row = y % 8;
    unsigned bits = (unsigned)src[tile * 16 + row * 2 + 1] << 8 | src[tile * 16 + row * 2];
    return (bits >> ((7 - x % 8) * 2)) & 3;
}

/* The ASCII the boxes print themselves, in the game's charset ('*' for its multiplication sign). */
static uint8_t Code(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(0xA1 + (c - '0'));
    if (c >= 'A' && c <= 'Z') return (uint8_t)(0xBB + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (uint8_t)(0xD5 + (c - 'a'));
    if (c == '/') return 0xBA;
    if (c == '*') return 0xB9;                    /* the multiplication sign */
    return 0x00;
}

static unsigned TextWidth(bool small, const uint8_t *codes)
{
    unsigned total = 0, w;
    for (; *codes != EOS; ++codes)
    {
        Glyph(small, *codes, &w);
        total += w;
    }
    return total;
}

static void Encode(const char *ascii, uint8_t *out, unsigned size)
{
    unsigned i = 0;
    for (; ascii[i] && i + 1 < size; ++i) out[i] = Code(ascii[i]);
    out[i] = EOS;
}

/* The design's gender marks, drawn rather than taken from the font: 8 rows standing on the baseline, a shadow a
 * pixel down and right. */
static const char *const MALE_MARK[8] = {"....###", ".....##", "....#.#", ".###...", "#...#..", "#...#..", "#...#..",
                                         ".###..."};
static const char *const FEMALE_MARK[8] = {".###.", "#...#", "#...#", "#...#", ".###.", "..#..", ".###.", "..#.."};

static int Mark(Canvas *c, int x, int base, unsigned gender)
{
    const char *const *mark = gender == CTR_HUD_MALE ? MALE_MARK : FEMALE_MARK;
    Rgb ink = gender == CTR_HUD_MALE ? (Rgb){56, 136, 248} : (Rgb){240, 88, 128};
    int top = base - 7, w = (int)strlen(mark[0]);

    for (int j = 0; j < 8; ++j)
        for (int k = 0; k < w; ++k)
            if (mark[j][k] == '#') Put(c, x + k + 2, top + j + 1, sInkShadow);
    for (int j = 0; j < 8; ++j)
        for (int k = 0; k < w; ++k)
            if (mark[j][k] == '#') Put(c, x + 1 + k, top + j, ink);
    return x + w + 2;
}

/* Text whose ink stands on row `base` (a glyph's row 10); `shadow` NULL leaves the shadow pixels out. */
static int Text(Canvas *c, bool small, int x, int base, const uint8_t *codes, Rgb ink, const Rgb *shadow)
{
    for (; *codes != EOS; ++codes)
    {
        unsigned w;
        const uint8_t *g = Glyph(small, *codes, &w);
        if (!g) return x;
        for (unsigned j = 0; j < 16; ++j)
            for (unsigned i = 0; i < w; ++i)
            {
                unsigned v = GlyphValue(small, g, i, j);
                if (v == 1) Put(c, x + (int)i, base - 10 + (int)j, ink);
                else if (v == 2 && shadow) Put(c, x + (int)i, base - 10 + (int)j, *shadow);
            }
        x += (int)w;
    }
    return x;
}

static int Ascii(Canvas *c, bool small, int x, int base, const char *ascii, Rgb ink, const Rgb *shadow)
{
    uint8_t codes[16];
    Encode(ascii, codes, sizeof(codes));
    return Text(c, small, x, base, codes, ink, shadow);
}

static unsigned AsciiWidth(bool small, const char *ascii)
{
    uint8_t codes[16];
    Encode(ascii, codes, sizeof(codes));
    return TextWidth(small, codes);
}

/* ---- The plate ---------------------------------------------------------------------------------------------------- */

static const char *const BALL[11] = {"...####...", "..#rrrr#..", ".#rRrrrr#.", "#rRrrrrrr#", "#rrr##rrr#",
                                     "####WW####", "#www##www#", "#wwwwwwww#", ".#wwwwww#.", "..#wwww#..",
                                     "...####..."};

static void Ball(Canvas *c, int x, int y, bool caught)
{
    for (int j = 0; j < 11; ++j)
        for (int i = 0; i < 10; ++i)
        {
            Rgb v;
            switch (BALL[j][i])
            {
            case '#': v = OUTLINE; break;
            case 'r': v = caught ? (Rgb){232, 56, 44} : (Rgb){170, 174, 186}; break;
            case 'R': v = caught ? (Rgb){255, 170, 150} : (Rgb){230, 232, 238}; break;
            case 'w': v = (Rgb){250, 250, 252}; break;
            case 'W': v = (Rgb){255, 255, 255}; break;
            default: continue;
            }
            Put(c, x + i, y + j, v);
        }
}

static void Level(Canvas *c, int right, int base, unsigned level)
{
    char digits[4];
    unsigned n = 0;
    uint8_t codes[4];

    do digits[n++] = (char)('0' + level % 10); while ((level /= 10) && n < 3);
    for (unsigned i = 0; i < n; ++i) codes[i] = Code(digits[n - 1 - i]);
    codes[n] = EOS;
    int w = (int)TextWidth(false, codes);
    Text(c, false, right - w, base, codes, sInk, &sInkShadow);
    Ascii(c, true, right - w - (int)AsciiWidth(true, LEVEL) - 1, base, LEVEL, sInk, NULL);
}

static void Fill(Canvas *c, int x, int y, int w, int h, Rgb v)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) Put(c, x + i, y + j, v);
}

/* The bars as FireRed's: a dark tag with the bar's name, then a flat track with a dark edge and a light inside. */
static const Rgb TAG = {64, 72, 80}, TRACK_EDGE = {64, 64, 64}, TRACK_IN = {228, 228, 220};

static void Tag(Canvas *c, int x, int y, int w, const char *text, Rgb ink)
{
    Fill(c, x, y, w, 9, TAG);
    Ascii(c, true, x + (w - (int)AsciiWidth(true, text)) / 2, y + 7, text, ink, NULL);
}

static void Track(Canvas *c, int x, int y, int w, int h, int filled, Rgb fill)
{
    Fill(c, x, y, w, h, TRACK_EDGE);
    Fill(c, x + 1, y + 1, w - 2, h - 2, TRACK_IN);
    if (filled > 0) Fill(c, x + 1, y + 1, filled, h - 2, fill);
}

/* HP from x on track row y to `end`: the 16-pixel "HP" tag, then a 7-pixel track filled as FireRed's (over half green,
 * over a fifth yellow, else red). */
static void HpRow(Canvas *c, int x, int y, int end, unsigned pixels)
{
    const Rgb fill = pixels > CTR_HUD_HP_PIXELS * 50 / 100 ? (Rgb){88, 208, 128}
                   : pixels > CTR_HUD_HP_PIXELS * 20 / 100 ? (Rgb){248, 200, 56} : (Rgb){240, 88, 72};
    int tx = x + 18, inner = end - tx - 2;

    Tag(c, x, y - 1, 16, CTR_TR("HP", "PS", "KP"), (Rgb){248, 208, 56});
    if (pixels > CTR_HUD_HP_PIXELS) pixels = CTR_HUD_HP_PIXELS;
    int fw = (int)lrintf((float)inner * (float)pixels / CTR_HUD_HP_PIXELS);
    if (pixels && !fw) fw = 1;
    Track(c, tx, y, end - tx, 7, fw, fill);
}

/* EXP from x on tag row y to `end`: its tag, then a 5-pixel track filled in blue. */
static void ExpRow(Canvas *c, int x, int y, int end, unsigned pixels)
{
    int tw = 4 + (int)AsciiWidth(true, "EXP") + 4, tx = x + tw + 2, inner = end - tx - 2;

    Tag(c, x, y, tw, "EXP", (Rgb){120, 200, 248});
    if (pixels > CTR_HUD_EXP_PIXELS) pixels = CTR_HUD_EXP_PIXELS;
    Track(c, tx, y + 2, end - tx, 5, (int)lrintf((float)inner * (float)pixels / CTR_HUD_EXP_PIXELS), (Rgb){64, 168, 248});
}

/* A status as FireRed marks it: a 9-pixel tag in the status's colour with its three letters (dark ink on the light
 * ones). Returns its width. */
static const struct { const char *label; Rgb base, ink; } STATUS[] = {
    [CTR_HUD_PSN] = {"PSN", {160, 80, 184}, {255, 255, 255}},
    [CTR_HUD_PAR] = {"PAR", {232, 184, 40}, {64, 64, 64}},
    [CTR_HUD_SLP] = {"SLP", {136, 140, 160}, {255, 255, 255}},
    [CTR_HUD_FRZ] = {"FRZ", {104, 192, 240}, {64, 64, 64}},
    [CTR_HUD_BRN] = {"BRN", {232, 104, 56}, {255, 255, 255}},
};

static int StatusTag(Canvas *c, int x, int y, unsigned s)
{
    int w = 4 + (int)AsciiWidth(true, STATUS[s].label) + 4;

    Fill(c, x, y, w, 9, STATUS[s].base);
    Ascii(c, true, x + 4, y + 7, STATUS[s].label, STATUS[s].ink, NULL);
    return w;
}

/* ---- The boxes -------------------------------------------------------------------------------------------------- */

static const char *const STAR[7] = {"...#...", "...#...", "..###..", "#######", ".#####.", "..#.#..", ".#...#."};

/* A shiny Pokemon's gold star after its name. */
static void Star(Canvas *c, int x, int y)
{
    for (int j = 0; j < 7; ++j)
        for (int k = 0; k < 7; ++k)
            if (STAR[j][k] == '#') Put(c, x + k + 1, y + j + 1, (Rgb){150, 96, 16});
    for (int j = 0; j < 7; ++j)
        for (int k = 0; k < 7; ++k)
            if (STAR[j][k] == '#') Put(c, x + k, y + j, j < 4 ? (Rgb){255, 214, 64} : (Rgb){240, 168, 32});
}

static int Name(Canvas *c, int x, int base, const CtrHudState *s)
{
    x = Text(c, false, x, base, s->name, sInk, &sInkShadow);
    if (s->gender != CTR_HUD_NO_GENDER) x = Mark(c, x, base, s->gender);
    if (s->shiny) Star(c, x + 2, base - 7);
    return x;
}

static bool HasStatus(const CtrHudState *s)
{
    return s->status >= CTR_HUD_PSN && s->status <= CTR_HUD_BRN;
}

/* "hp/max" in the game's charset. */
static void Numbers(const CtrHudState *s, char *out)
{
    char tmp[6];
    unsigned n = 0, t = 0, v = s->hp;

    do tmp[t++] = (char)('0' + v % 10); while ((v /= 10) && t < 5);
    while (t) out[n++] = tmp[--t];
    out[n++] = '/';
    v = s->maxHp;
    do tmp[t++] = (char)('0' + v % 10); while ((v /= 10) && t < 5);
    while (t) out[n++] = tmp[--t];
    out[n] = 0;
}

#define FOE_W 152
#define FOE_H 36
/* The player's boxes are kept clear of the foe's ground: narrower than the foe's. */
#define PLAYER_W 136
#define PLAYER_H 49
#define LOW_H 36                           /* the Safari Zone's box */
/* The player's boxes in a double battle: one row for the name and one for the bar, so both fit under the foe's ground. */
#define DOUBLE_H 26
#define PAD 10                             /* from the box's sides to its text and bars */

/* The pack's frame (CtrHud_SetFrame), as the lower screen's kit draws it. */
static struct { bool ok; uint8_t tiles[9 * 32]; Rgb pal[16]; } sFrame;
static const Rgb FRAME_OUTER = {62, 76, 107}, FRAME_INNER = {167, 182, 211};

/* The box's window: the frame's ring of tiles (corners at the corners, the edges' tiles repeated along them) around its
 * colour 1; without the pack, the frame's outline look: a 3-pixel dark rim, a 2-pixel light one, white, corners clear. */
static void Window(Canvas *c, int w, int h)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            if (sFrame.ok)
            {
                int col = x < 8 ? 0 : x >= w - 8 ? 2 : 1, row = y < 8 ? 0 : y >= h - 8 ? 2 : 1;
                int px = col == 0 ? x : col == 2 ? x - (w - 8) : (x - 8) % 8;
                int py = row == 0 ? y : row == 2 ? y - (h - 8) : (y - 8) % 8;

                if (row == 1 && col == 1) { Put(c, x, y, sFrame.pal[1]); continue; }
                uint8_t b = sFrame.tiles[(row * 3 + col) * 32 + py * 4 + px / 2];
                unsigned v = (px & 1) ? b >> 4 : b & 15u;
                if (v) Put(c, x, y, sFrame.pal[v]);
                continue;
            }
            if ((x == 0 || x == w - 1) && (y == 0 || y == h - 1)) continue;
            int d = x < y ? x : y;
            if (w - 1 - x < d) d = w - 1 - x;
            if (h - 1 - y < d) d = h - 1 - y;
            Put(c, x, y, d < 3 ? FRAME_OUTER : d < 5 ? FRAME_INNER : (Rgb){255, 255, 255});
        }
}

/* The HP row's start (its track on row y): after the status tag when there is one, else 28 in. */
static int HpStart(Canvas *c, const CtrHudState *s, int y)
{
    return HasStatus(s) ? PAD + StatusTag(c, PAD, y - 1, s->status) + 3 : 28;
}

static void FoeBox(Canvas *c, const CtrHudState *s)
{
    const int w = FOE_W;
    int nx = PAD;

    Window(c, w, FOE_H);
    if (!s->noBall)
    {
        Ball(c, 9, 4, s->caught);
        nx = 22;
    }
    Name(c, nx, 14, s);
    Level(c, w - PAD, 14, s->level);
    HpRow(c, HpStart(c, s, 21), 21, w - PAD, s->hpPixels);
}

static void PlayerBox(Canvas *c, const CtrHudState *s)
{
    const int w = PLAYER_W;
    char numbers[12];

    Window(c, w, PLAYER_H);
    Name(c, PAD, 14, s);
    Level(c, w - PAD, 14, s->level);
    HpRow(c, HpStart(c, s, 19), 19, w - PAD, s->hpPixels);
    /* The last row: EXP on the left, "hp/max" right-aligned beside it. */
    Numbers(s, numbers);
    Ascii(c, false, w - PAD - (int)AsciiWidth(false, numbers), 41, numbers, sInk, &sInkShadow);
    ExpRow(c, PAD, 32, w - PAD - 48, s->expPixels);
}

/* A double battle's player box: the name row, then HP with its numbers beside the bar. */
static void PlayerDoubleBox(Canvas *c, const CtrHudState *s)
{
    const int w = PLAYER_W;
    char numbers[12];

    Window(c, w, DOUBLE_H);
    Name(c, PAD, 11, s);
    Level(c, w - PAD, 11, s->level);
    Numbers(s, numbers);
    int nw = (int)AsciiWidth(false, numbers);
    Ascii(c, false, w - PAD - nw, 20, numbers, sInk, &sInkShadow);
    HpRow(c, HasStatus(s) ? PAD + StatusTag(c, PAD, 12, s->status) + 3 : PAD, 13, w - PAD - nw - 4, s->hpPixels);
}

/* The Safari Zone's box: SAFARI BALLS, a Poke Ball and how many are left below. */
static void SafariBox(Canvas *c, const CtrHudState *s)
{
    char count[8];
    unsigned n = 0, t = 0, v = s->balls;
    char tmp[4];

    Window(c, PLAYER_W, LOW_H);
    Ascii(c, false, PAD, 14, "SAFARI BALLS", sInk, &sInkShadow);
    Ball(c, PAD + 2, 19, true);
    count[n++] = '*';
    do tmp[t++] = (char)('0' + v % 10); while ((v /= 10) && t < 3);
    while (t) count[n++] = tmp[--t];
    count[n] = 0;
    Ascii(c, false, PAD + 16, 29, count, sInk, &sInkShadow);
}

void CtrHud_Render(const CtrHudState *s, uint32_t *pixels, unsigned *width, unsigned *height)
{
    int w = s->kind == CTR_HUD_FOE ? FOE_W : PLAYER_W, h = s->kind == CTR_HUD_FOE ? FOE_H
                                                       : s->kind == CTR_HUD_PLAYER ? PLAYER_H
                                                       : s->kind == CTR_HUD_PLAYER_DOUBLE ? DOUBLE_H : LOW_H;
    Canvas c = {pixels, w, h};

    for (int y = 0; y < c.h; ++y) memset(pixels + y * CTR_HUD_IMAGE_W, 0, (size_t)c.w * 4);
    switch (s->kind)
    {
    case CTR_HUD_FOE: FoeBox(&c, s); break;
    case CTR_HUD_PLAYER: PlayerBox(&c, s); break;
    case CTR_HUD_PLAYER_DOUBLE: PlayerDoubleBox(&c, s); break;
    case CTR_HUD_SAFARI: SafariBox(&c, s); break;
    default: break;
    }
    *width = (unsigned)c.w;
    *height = (unsigned)c.h;
}

/* ---- State ------------------------------------------------------------------------------------------------------- */

typedef struct {
    CtrHudState state;
    bool dirty;
    CtrHudImage image;
    uint32_t pixels[CTR_HUD_IMAGE_W * CTR_HUD_IMAGE_H];
} Box;

static Box sBoxes[CTR_HUD_BATTLERS];
static const uint16_t *sUnfaded;
static uint32_t sVersion;

/*
 * Where each box goes. FireRed's healthboxes rest with their main sprite's top left at these GBA points
 * (InitBattlerHealthboxCoords: the centre less half the sprite); the design puts the box's top left at these screen
 * points with the battle at CTR_BATTLE_ZOOM. The box follows its sprite from there: a slide-in or a bob moves both.
 * The player's single and Safari boxes are made 64x32 and turned 64x64 afterwards, keeping the 64x32 centre: their
 * top left is 16 rows lower than a 64x64 sprite's would be (the centre's y less 16, not less 32).
 */
static const struct { uint8_t kind; bool doubles; uint8_t position; int16_t gbaX, gbaY, x, y; } ANCHORS[] = {
    {CTR_HUD_FOE, false, 1, 12, 14, 10, 22},
    {CTR_HUD_PLAYER, false, 0, 126, 72, 260, 141},
    {CTR_HUD_SAFARI, false, 0, 126, 72, 260, 154},
    {CTR_HUD_FOE, true, 1, 12, 3, 24, 6},
    {CTR_HUD_FOE, true, 3, 0, 28, 6, 44},
    {CTR_HUD_PLAYER_DOUBLE, true, 0, 127, 59, 252, 136},
    {CTR_HUD_PLAYER_DOUBLE, true, 2, 139, 84, 260, 164},
};

static void Anchor(const CtrHudState *s, float *dx, float *dy)
{
    const float offX = 200.0f - 120.0f * CTR_BATTLE_ZOOM, offY = 192.0f - 112.0f * CTR_BATTLE_ZOOM;
    *dx = *dy = 0.0f;
    for (unsigned i = 0; i < sizeof(ANCHORS) / sizeof(ANCHORS[0]); ++i)
        if (ANCHORS[i].kind == s->kind && ANCHORS[i].doubles == s->doubles
         && (ANCHORS[i].position == s->position || !s->doubles))
        {
            *dx = ANCHORS[i].x - (ANCHORS[i].gbaX * CTR_BATTLE_ZOOM + offX);
            *dy = ANCHORS[i].y - (ANCHORS[i].gbaY * CTR_BATTLE_ZOOM + offY);
            return;
        }
}

static Rgb FromGba(uint16_t v)
{
    unsigned r = v & 31u, g = (v >> 5) & 31u, b = (v >> 10) & 31u;
    return (Rgb){(uint8_t)(r << 3 | r >> 2), (uint8_t)(g << 3 | g >> 2), (uint8_t)(b << 3 | b >> 2)};
}

void CtrHud_SetFrame(const uint8_t *tiles, const uint16_t *framePal, const uint16_t *textPal)
{
    sFrame.ok = tiles && framePal;
    if (sFrame.ok)
    {
        memcpy(sFrame.tiles, tiles, sizeof(sFrame.tiles));
        for (int i = 0; i < 16; ++i) sFrame.pal[i] = FromGba(framePal[i]);
    }
    sInk = textPal ? FromGba(textPal[2]) : (Rgb){96, 96, 96};
    sInkShadow = textPal ? FromGba(textPal[3]) : (Rgb){216, 216, 208};
    for (unsigned i = 0; i < CTR_HUD_BATTLERS; ++i) sBoxes[i].dirty = true;
}

static Box *BoxOf(unsigned battler)
{
    return battler < CTR_HUD_BATTLERS && sBoxes[battler].state.kind != CTR_HUD_OFF ? &sBoxes[battler] : NULL;
}

void CtrHud_Box(unsigned battler, unsigned kind, unsigned position, bool doubles, unsigned boxTile, unsigned barTile,
                const uint16_t *unfadedPalette)
{
    if (battler >= CTR_HUD_BATTLERS) return;
    /* A single battle has no right-hand battlers: what a double battle left there goes. */
    if (!doubles)
        for (unsigned i = 0; i < CTR_HUD_BATTLERS; ++i)
            if (sBoxes[i].state.doubles) sBoxes[i].state.kind = CTR_HUD_OFF;
    Box *b = &sBoxes[battler];
    memset(&b->state, 0, sizeof(b->state));
    b->state.kind = (uint8_t)(kind <= CTR_HUD_SAFARI ? kind : CTR_HUD_OFF);
    b->state.position = (uint8_t)position;
    b->state.doubles = doubles;
    b->state.boxTile = (uint16_t)boxTile;
    b->state.barTile = (uint16_t)barTile;
    b->state.gender = CTR_HUD_NO_GENDER;
    b->state.name[0] = EOS;
    b->dirty = true;
    sUnfaded = unfadedPalette;
    /* With FireRed's own boxes (CTR_PORT_BATTLE_HUD 0) the compositor is never handed these: the game's sprites show. */
    if (CTR_PORT_BATTLE_HUD && b->state.kind != CTR_HUD_OFF) CtrVideo_SetBattleHud(CtrHud_Match, CtrHud_Image);
}

#define SET(battler, field, value) do { Box *b_ = BoxOf(battler); \
        if (b_ && b_->state.field != (value)) { b_->state.field = (value); b_->dirty = true; } } while (0)

void CtrHud_Name(unsigned battler, const uint8_t *name, unsigned gender, bool shiny)
{
    Box *b = BoxOf(battler);
    uint8_t copy[CTR_HUD_NAME_LEN];
    unsigned i = 0;

    if (!b) return;
    for (; name && name[i] != EOS && i + 1 < CTR_HUD_NAME_LEN; ++i) copy[i] = name[i];
    copy[i] = EOS;
    if (memcmp(copy, b->state.name, i + 1) != 0)
    {
        memcpy(b->state.name, copy, i + 1);
        b->dirty = true;
    }
    SET(battler, gender, (uint8_t)(gender <= CTR_HUD_NO_GENDER ? gender : CTR_HUD_NO_GENDER));
    SET(battler, shiny, shiny);
}

void CtrHud_Level(unsigned battler, unsigned level) { SET(battler, level, (uint8_t)level); }

void CtrHud_Hp(unsigned battler, int value, bool max)
{
    uint16_t v = (uint16_t)(value < 0 ? 0 : value > 999 ? 999 : value);
    if (max) SET(battler, maxHp, v);
    else SET(battler, hp, v);
}

void CtrHud_HpBar(unsigned battler, unsigned pixels) { SET(battler, hpPixels, (uint8_t)pixels); }
void CtrHud_ExpBar(unsigned battler, unsigned pixels) { SET(battler, expPixels, (uint8_t)pixels); }
void CtrHud_Status(unsigned battler, unsigned status) { SET(battler, status, (uint8_t)status); }
void CtrHud_Caught(unsigned battler, bool caught) { SET(battler, caught, caught); }
void CtrHud_NoBall(unsigned battler, bool none) { SET(battler, noBall, none); }
void CtrHud_SafariBalls(unsigned battler, unsigned count) { SET(battler, balls, (uint8_t)(count > 99 ? 99 : count)); }

unsigned CtrHud_Match(unsigned tile, unsigned *battler)
{
    for (unsigned i = 0; i < CTR_HUD_BATTLERS; ++i)
    {
        const CtrHudState *s = &sBoxes[i].state;
        /* The box's two halves: 64x64 each for the player's in a single battle and the Safari Zone's, else 64x32. */
        unsigned boxTiles = s->kind == CTR_HUD_PLAYER || s->kind == CTR_HUD_SAFARI ? 128 : 64;
        if (s->kind == CTR_HUD_OFF) continue;
        *battler = i;
        if (tile == s->boxTile) return CTR_HUD_MAIN;
        if ((tile > s->boxTile && tile < s->boxTile + boxTiles) || (tile >= s->barTile && tile < s->barTile + 9u))
            return CTR_HUD_PART;
    }
    return CTR_HUD_NONE;
}

const CtrHudImage *CtrHud_Image(unsigned battler)
{
    Box *b = BoxOf(battler);
    if (!b) return NULL;
    if (b->dirty)
    {
        unsigned w, h;
        CtrHud_Render(&b->state, b->pixels, &w, &h);
        b->image.version = ++sVersion;
        b->image.width = (uint16_t)w;
        b->image.height = (uint16_t)h;
        Anchor(&b->state, &b->image.dx, &b->image.dy);
        b->image.pixels = b->pixels;
        b->dirty = false;
    }
    b->image.unfaded = sUnfaded;
    return &b->image;
}
