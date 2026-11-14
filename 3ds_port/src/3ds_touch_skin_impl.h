/* The lower screen's ember skin (ember_skin.h, made by tools/make_ember_skin.py): the backdrop every screen is drawn on
 * and the pieces of the modern menus drawn over it: raised rounded buttons with a lip (amber when chosen, cream
 * otherwise, violet for going back), the note card, the frame round the GBA picture, the switch, the icons. (The
 * sparks that rose over it went with the EMBERS option, in the lower screen rework.) Included by 3ds_poketouch.c after its Px / Fill and the theme. */
#include "ember_skin.h"

static uint16_t TsGet(int x, int y)
{
    return sCanvas[(size_t)x * H + (H - 1 - (size_t)y)];
}

/* The backdrop in the canvas' own order (column by column, bottom up), made the first time it is drawn: a rectangle of
 * it is then a copy per column instead of a lookup per pixel, which every screen pays more than once a redraw. */
static uint16_t sTsBg[W * H];
static int sTsBgReady;

static void TsBgPrepare(void)
{
    for (int cx = 0; cx < W; ++cx)
        for (int cy = 0; cy < H; ++cy)
            sTsBg[(size_t)cx * H + (H - 1 - (size_t)cy)] = EMBER_BG(cx, cy);
    sTsBgReady = 1;
}

/* The backdrop back over a rectangle. */
static void TsBgRect(int x, int y, int w, int h)
{
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y, x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;

    if (x0 >= x1 || y0 >= y1)
        return;
    if (!sTsBgReady)
        TsBgPrepare();
    for (int cx = x0; cx < x1; ++cx) {
        size_t at = (size_t)cx * H + (H - (size_t)y1);

        memcpy(sCanvas + at, sTsBg + at, (size_t)(y1 - y0) * sizeof(uint16_t));
    }
}

static void TsBackdrop(void)
{
    TsBgRect(0, 0, W, H);
}

/* `c` over the canvas at (x, y) by a/255. */
static void EmBlend(int x, int y, uint16_t c, int a)
{
    if ((unsigned)x >= W || (unsigned)y >= H || a <= 0)
        return;
    Px(x, y, a >= 255 ? c : UiMix(TsGet(x, y), c, a * 8 / 255));
}

/* The palettes of the raised pieces: the face from `top` to `bottom`, its lip and outline, and the ink of a label. */
typedef struct { uint16_t top, bottom, lip, edge, ink, inkSh; } EmPal;

#define EM_DARK UI_RGB(98, 98, 98)                  /* FireRed's text grey and its shadow, on the light pieces */
#define EM_DARK_SH UI_RGB(213, 213, 205)
#define EM_DIM UI_RGB(136, 120, 112)
#define EM_WHITE UI_RGB(255, 255, 255)
#define EM_STRIPE UI_RGB(232, 112, 28)
#define EM_GLOW UI_RGB(255, 170, 60)
#define EM_VALUE UI_RGB(208, 48, 32)                /* a value on a light piece (the game's red) */
static const EmPal kEmAmber = {UI_RGB(255, 206, 96), UI_RGB(236, 108, 28), UI_RGB(170, 70, 14), UI_RGB(110, 40, 6),
                               EM_WHITE, UI_RGB(110, 40, 6)};
static const EmPal kEmCream = {UI_RGB(255, 252, 244), UI_RGB(232, 226, 214), UI_RGB(150, 136, 126), UI_RGB(96, 80, 72),
                               EM_DARK, EM_DARK_SH};
static const EmPal kEmGrey = {UI_RGB(200, 192, 196), UI_RGB(150, 140, 146), UI_RGB(108, 98, 104), UI_RGB(70, 60, 66),
                              UI_RGB(120, 112, 116), UI_RGB(206, 200, 202)};
static const EmPal kEmViolet = {UI_RGB(120, 112, 128), UI_RGB(70, 62, 82), UI_RGB(46, 38, 58), UI_RGB(30, 24, 40),
                                EM_WHITE, UI_RGB(30, 24, 40)};

/* Rows [y, y + h) of a rounded rectangle, each row a blend from `top` to `bottom`. */
static void EmGradFill(int x, int y, int w, int h, int radius, uint16_t top, uint16_t bottom)
{
    if (w <= 0 || h <= 0)
        return;
    if (radius > 5)
        radius = 5;
    if (radius * 2 > h)
        radius = h / 2;
    /* Column by column, the canvas' own order: a row at a time would step a whole column of memory per pixel. A pixel
     * is drawn where its row's corner cut does not reach its column. */
    uint16_t colour[H];
    int r0 = y < 0 ? -y : 0, r1 = y + h > H ? H - y : h;

    for (int row = r0; row < r1; ++row)
        colour[row - r0] = UiMix(top, bottom, h > 1 ? row * 8 / (h - 1) : 0);
    for (int col = 0; col < w; ++col) {
        int cx = x + col, edgeX = col < w - 1 - col ? col : w - 1 - col, first = r0, last = r1;

        if ((unsigned)cx >= W)
            continue;
        while (first < last && first < 4 && sUiCut[radius][first] > edgeX)
            ++first;
        while (last > first && h - last < 4 && sUiCut[radius][h - last] > edgeX)
            --last;
        uint16_t *p = sCanvas + (size_t)cx * H + (H - 1 - (size_t)(y + first));

        for (int row = first; row < last; ++row)
            *p-- = colour[row - r0];
    }
}

/* A raised piece: the lip `depth` pixels under the face, both outlined; pressed, the face sinks onto the lip. The
 * caller has put the backdrop back under it. Returns where the face starts (for its label). */
static int EmButton(int x, int y, int w, int h, const EmPal *p, int depth, int pressed)
{
    int face = h - depth, fy = pressed ? y + depth : y;

    if (!pressed) {
        UiRoundFill(x, y + depth, w, face, 5, p->edge);
        UiRoundFill(x + 1, y + depth + 1, w - 2, face - 2, 4, p->lip);
    }
    UiRoundFill(x, fy, w, face, 5, p->edge);
    EmGradFill(x + 1, fy + 1, w - 2, face - 2, 4, pressed ? UiMix(p->top, p->bottom, 4) : p->top, p->bottom);
    Fill(x + 5, fy + 1, w - 10, 1, UiMix(p->top, EM_WHITE, 4));
    return fy;
}

/* The glow round a chosen piece: two soft rings outside it. */
static void EmGlowRing(int x, int y, int w, int h)
{
    for (int i = 1; i <= 2; ++i) {
        int a = i == 1 ? 150 : 70;

        for (int cx = x + 3; cx < x + w - 3; ++cx) {
            EmBlend(cx, y - i, EM_GLOW, a);
            EmBlend(cx, y + h - 1 + i, EM_GLOW, a);
        }
        for (int cy = y + 3; cy < y + h - 3; ++cy) {
            EmBlend(x - i, cy, EM_GLOW, a);
            EmBlend(x + w - 1 + i, cy, EM_GLOW, a);
        }
        EmBlend(x - i + 2, y - i + 2, EM_GLOW, a / 2);
        EmBlend(x + w - 3 + i, y - i + 2, EM_GLOW, a / 2);
        EmBlend(x - i + 2, y + h - 3 + i, EM_GLOW, a / 2);
        EmBlend(x + w - 3 + i, y + h - 3 + i, EM_GLOW, a / 2);
    }
}

/* A label centred across [x, x + w) on a face that starts at `fy` and is `fh` tall. */
static void EmLabel(const char *s, int x, int w, int fy, int fh, const EmPal *p)
{
    int tw = UiTextWidth(s, 1);

    UiText(s, x + (w - tw) / 2, fy + (fh - UI_TEXT_H) / 2, p->ink, p->inkSh, 1, x + w);
}

/* The note card: a light card with an ember stripe on its left; the text is the caller's. */
static void EmNote(int x, int y, int w, int h)
{
    UiRoundFill(x, y, w, h, 5, UI_RGB(60, 64, 92));
    EmGradFill(x + 1, y + 1, w - 2, h - 2, 4, UI_RGB(255, 255, 255), UI_RGB(226, 230, 238));
    Fill(x + 4, y + 6, 3, h - 12, EM_STRIPE);
}

/* The frame round the GBA picture (w x h at x, y): three pixels of dark slate outside it. */
static void EmBezel(int x, int y, int w, int h)
{
    CtrPokeTouchRect outer = {(int16_t)(x - 3), (int16_t)(y - 3), (int16_t)(w + 6), (int16_t)(h + 6)};
    CtrPokeTouchRect inner = UiInset(&outer, 1);

    UiRoundRect(&outer, 5, UI_RGB(24, 18, 28));
    UiRoundFill(inner.x, inner.y, inner.w, inner.h, 4, UI_RGB(52, 44, 60));
    UiRing(&inner, 4, 1, UI_RGB(96, 86, 104));
}

/* A switch: a 26x12 track, green and the knob on the right when on, grey with the knob on the left when off. */
static void EmSwitch(int x, int y, int on)
{
    UiRoundFill(x, y, 26, 12, 5, UI_RGB(70, 60, 66));
    UiRoundFill(x + 1, y + 1, 24, 10, 4, on ? UI_RGB(72, 196, 96) : UI_RGB(176, 172, 180));
    UiDisc(on ? x + 19 : x + 6, y + 6, 6, UI_RGB(70, 60, 66));
    UiDisc(on ? x + 19 : x + 6, y + 6, 5, UI_RGB(250, 250, 252));
}

/* A shortcut's key: its button's letter on a small blue pill, centred at the top of the tile (cx its centre). */
static void EmKeyTab(int cx, int y, const char *letter)
{
    UiRoundFill(cx - 10, y, 20, 11, 5, UI_RGB(30, 40, 96));
    EmGradFill(cx - 9, y + 1, 18, 9, 4, UI_RGB(96, 132, 236), UI_RGB(52, 80, 200));
    UiText(letter, cx - UiTextWidth(letter, 1) / 2, y + 1, EM_WHITE, UI_RGB(30, 40, 96), 1, cx + 9);
}

/* The soft well a shortcut's item sits in: the face's own colour, a little deeper, with a shade along its top. */
static void EmWell(int x, int y, int w, int h, const EmPal *p)
{
    uint16_t deep = UiMix(p->bottom, p->edge, 2);

    UiRoundFill(x, y, w, h, 4, deep);
    EmGradFill(x + 1, y + 2, w - 2, h - 3, 3, UiMix(deep, p->bottom, 5), UiMix(p->top, p->bottom, 4));
}

/* One of the column's icons (ember_skin.h), blended onto what is there; dim, it is toned grey. */
static void EmIcon(int icon, int x, int y, int on)
{
    for (int v = 0; v < EMBER_ICON; ++v)
        for (int u = 0; u < EMBER_ICON; ++u) {
            int a = kEmberIconA[icon][v * EMBER_ICON + u];

            if (a)
                EmBlend(x + u, y + v, Tone(kEmberIcon[icon][v * EMBER_ICON + u], on), a);
        }
}
