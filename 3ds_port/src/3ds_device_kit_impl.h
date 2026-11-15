/* The red device (style B): the lower screen's drawing kit, included by 3ds_poketouch.c after the battle drawer.
 *
 * The B views draw the screen as a device: a shell in the player's colour (OPTIONS' Shell colour), screens with a dark
 * bezel and paper, cream keys with a drop shadow that sink when chosen and light a green LED, dark keys, faint keys,
 * holes for empty bays, list rows that invert when chosen, bars and chips. The values are the owner's boards (spec 5.3);
 * the boards are CSS, so a shadow here is CSS's box-shadow with no blur: the shape moved by its offset, where the shape
 * does not cover it (an inset one, inside it). A key's rectangle is its resting face as the boards give it; it paints
 * only inside DevKeyArea (3ds_device_layout.h), which is also where it is hit.
 *
 * A piece draws over what is there and leaves alone what it does not cover (a sunk key's two top rows, a translucent
 * drop); a view draws itself whole, shell first, whenever a field it draws changes. Text: FireRed's normal font for the
 * boards' 10-12 px text and its small font (recipe 0120) for their 7-9 px labels; `top` is where the capitals start.
 * Every function is UI_FN: a view uses the ones it needs.
 */
#include "3ds_device_layout.h"

/* Colours (RGB565 of the boards' values). The shell, by OPTIONS' Shell colour (shell & 3). */
#define DEV_SHELL_RED      UI_RGB(0xC2, 0x3A, 0x2E)
#define DEV_SHELL_BLUE     UI_RGB(0x3E, 0x4C, 0x6B)
#define DEV_SHELL_GREEN    UI_RGB(0x2E, 0x7A, 0x4E)
#define DEV_SHELL_GREY     UI_RGB(0x5A, 0x5A, 0x64)
/* An embedded screen: the bezel, its inner line, the paper and the paper's second tone; ink, its shadow, secondary
 * ink, dimmed ink. */
#define DEV_BEZEL          UI_RGB(0x2A, 0x2A, 0x30)
#define DEV_BEZEL_LINE     UI_RGB(0x4A, 0x4A, 0x54)
#define DEV_PAPER          UI_RGB(0xF8, 0xF8, 0xF4)
#define DEV_PAPER2         UI_RGB(0xEF, 0xEE, 0xE4)
#define DEV_INK            UI_RGB(0x62, 0x62, 0x62)
#define DEV_INK_SH         UI_RGB(0xD5, 0xD5, 0xCD)
#define DEV_INK2           UI_RGB(0x74, 0x74, 0x74)
#define DEV_INK_DIM        UI_RGB(0xA8, 0xA8, 0xA0)
/* A key: its face, inner bottom edge and ink (on a white shadow); chosen, it sinks: face, inner top edge, ink and its
 * shadow, and the LED. */
#define DEV_KEY            UI_RGB(0xF2, 0xEC, 0xE0)
#define DEV_KEY_EDGE       UI_RGB(0xD6, 0xCC, 0xB8)
#define DEV_KEY_INK        UI_RGB(0x4A, 0x4A, 0x4A)
#define DEV_SUNK           UI_RGB(0xE6, 0xDE, 0xCE)
#define DEV_SUNK_EDGE      UI_RGB(0xCF, 0xC4, 0xAE)
#define DEV_SUNK_INK       UI_RGB(0x3A, 0x3A, 0x3A)
#define DEV_SUNK_INK_SH    UI_RGB(0xF4, 0xEF, 0xE4)
#define DEV_LED            UI_RGB(0x58, 0xE0, 0x70)
/* The dark key's label and arrows (its face is DEV_BEZEL, its top line DEV_BEZEL_LINE). */
#define DEV_DARK_INK       UI_RGB(0xF2, 0xEC, 0xE0)
/* A fainted Pokemon's key. */
#define DEV_FAINT          UI_RGB(0xD2, 0xCA, 0xB8)
#define DEV_FAINT_EDGE     UI_RGB(0xBC, 0xB2, 0x9C)
#define DEV_FAINT_INK      UI_RGB(0x6E, 0x6E, 0x6E)
/* HOME's paper buttons (RUN, SPD): white with this border. */
#define DEV_BUTTON_EDGE    UI_RGB(0xB0, 0xB0, 0xA8)
/* Bars: HP (frame, track, the three levels, the "HP" tag's ink) and experience. */
#define DEV_HP_FRAME       UI_RGB(0x40, 0x48, 0x50)
#define DEV_HP_TRACK       UI_RGB(0xD8, 0xD8, 0xD0)
#define DEV_HP_GREEN       UI_RGB(0x48, 0xC0, 0x60)
#define DEV_HP_YELLOW      UI_RGB(0xF0, 0xC8, 0x30)
#define DEV_HP_RED         UI_RGB(0xE0, 0x40, 0x30)
#define DEV_HP_LABEL       UI_RGB(0xF8, 0xC8, 0x30)
#define DEV_EXP            UI_RGB(0x48, 0xA0, 0xF8)
#define DEV_EXP_LABEL      UI_RGB(0x78, 0xB8, 0xF8)
/* Chips: a move's effectiveness, its category, the status. */
#define DEV_FX_SUPER       UI_RGB(0x28, 0x96, 0x46)
#define DEV_FX_NOTVERY     UI_RGB(0xCC, 0x54, 0x18)
#define DEV_FX_EFFECTIVE   UI_RGB(0x64, 0x6A, 0x84)
#define DEV_FX_NONE        UI_RGB(0x7A, 0x7A, 0x7A)
#define DEV_CAT_SPECIAL    UI_RGB(0x5A, 0x78, 0xC0)
#define DEV_CAT_PHYSICAL   UI_RGB(0xC0, 0x38, 0x28)
#define DEV_PSN            UI_RGB(0x90, 0x50, 0xA8)
#define DEV_FNT            UI_RGB(0x7A, 0x7A, 0x7A)
/* Inks: low HP, PP at a quarter and at half, the blue ink, a raised stat, the gender marks (female: v0.6.0's red). */
#define DEV_LOW_INK        UI_RGB(0xB0, 0x30, 0x20)
#define DEV_PP_QUARTER     UI_RGB(0xB0, 0x4A, 0x10)
#define DEV_PP_HALF        UI_RGB(0x94, 0x6E, 0x06)
#define DEV_BLUE_INK       UI_RGB(0x2E, 0x5A, 0xA0)
#define DEV_STAT_UP        UI_RGB(0xC0, 0x38, 0x28)
#define DEV_MALE           UI_RGB(0x48, 0x78, 0xC8)
#define DEV_FEMALE         UI_RGB(255, 110, 120)

/* Alphas, of 256: the shell's light top edge (and every light row under a shape), its dark bottom edge, a screw, a key's
 * drop, a hole, a dimmed group, a fainted Pokemon's icon. */
#define DEV_A_LIGHT        64
#define DEV_A_DARK         77
#define DEV_A_SCREW        90
#define DEV_A_DROP         97
#define DEV_A_HOLE         72
#define DEV_A_DIM          128
#define DEV_A_FAINT_ART    115

#define DEV_SCREWS_TOP     1
#define DEV_SCREWS_BOTTOM  2
#define DEV_LINE_H         13       /* DevWrap's lines */

enum { DEV_LOOK_KEY, DEV_LOOK_DARK, DEV_LOOK_FAINT };

static uint16_t sDevShell __attribute__((unused)) = DEV_SHELL_RED;
/* Key labels cut to fit since the counter was last cleared: the host harness requires none. */
static int sDevKeyCut __attribute__((unused));

/* ---- Pixels -------------------------------------------------------------------------------------------------------- */

/* `over` on `under` at alpha/256, channel by channel, rounded. */
UI_FN uint16_t DevBlend(uint16_t under, uint16_t over, int alpha)
{
    int na = 256 - alpha;
    int r = ((under >> 11) * na + (over >> 11) * alpha + 128) >> 8;
    int g = (((under >> 5) & 63) * na + ((over >> 5) & 63) * alpha + 128) >> 8;
    int b = ((under & 31) * na + (over & 31) * alpha + 128) >> 8;

    return (uint16_t)(r << 11 | g << 5 | b);
}

/* Draws go into `canvas` from now on, in the shell colour `shell` (0 red, 1 blue, 2 green, 3 grey). */
UI_FN void DevBegin(uint16_t *canvas, uint8_t shell)
{
    static const uint16_t kShells[4] = {DEV_SHELL_RED, DEV_SHELL_BLUE, DEV_SHELL_GREEN, DEV_SHELL_GREY};

    sCanvas = canvas;
    sDevShell = kShells[shell & 3];
}

/* The canvas at (x, y); 0 off the screen. */
UI_FN uint16_t DevGet(int x, int y)
{
    if ((unsigned)x >= W || (unsigned)y >= H)
        return 0;
    return sCanvas[(size_t)x * H + (H - 1 - (size_t)y)];
}

/* ---- Rounded shapes and their CSS shadows ------------------------------------------------------------------------- */

/* A corner radius as CSS draws it: at most 10 here, and at most half the shape's width and height. */
UI_FN int DevRadius(int w, int h, int r)
{
    if (r > 10)
        r = 10;
    if (2 * r > w)
        r = w / 2;
    if (2 * r > h)
        r = h / 2;
    return r < 0 ? 0 : r;
}

/* The pixels a corner of radius r cuts from row j (0 is the first) of its corner square: a pixel is kept when its centre
 * lies inside the corner's circle. */
UI_FN int DevCut(int r, int j)
{
    int i = 0, dy = 2 * j + 1 - 2 * r;

    while (i < r && (2 * i + 1 - 2 * r) * (2 * i + 1 - 2 * r) + dy * dy > 4 * r * r)
        ++i;
    return i;
}

/* Row j of a w x h shape with corners of radius r (already DevRadius'd): its pixels [*a, *b), and 0 for a row outside the
 * shape. */
UI_FN int DevSpan(int w, int h, int r, int j, int *a, int *b)
{
    int c;

    if (j < 0 || j >= h || w <= 0)
        return 0;
    c = j < r ? DevCut(r, j) : j >= h - r ? DevCut(r, h - 1 - j) : 0;
    *a = c;
    *b = w - c;
    return *b > *a;
}

/* Pixels [a, b) of row y, from x: c, or (alpha > 0) what is there blended toward c by alpha. */
UI_FN void DevSpanPaint(int x, int y, int a, int b, uint16_t c, int alpha)
{
    if (b <= a)
        return;
    if (!alpha) {
        Fill(x + a, y, b - a, 1, c);
        return;
    }
    for (int i = a; i < b; ++i)
        Px(x + i, y, DevBlend(DevGet(x + i, y), c, alpha));
}

/* [a0, b0) less [a1, b1) of row y (nothing taken away when b1 <= a1). */
UI_FN void DevSpanMinus(int x, int y, int a0, int b0, int a1, int b1, uint16_t c, int alpha)
{
    if (b1 <= a1) {
        DevSpanPaint(x, y, a0, b0, c, alpha);
        return;
    }
    DevSpanPaint(x, y, a0, a1 < b0 ? a1 : b0, c, alpha);
    DevSpanPaint(x, y, b1 > a0 ? b1 : a0, b0, c, alpha);
}

/* box-shadow 0 dy 0 (dy > 0): the shape moved dy rows down, where the shape does not cover it, blended toward c. */
UI_FN void DevOuter(int x, int y, int w, int h, int r, int dy, uint16_t c, int alpha)
{
    for (int j = dy; j < h + dy; ++j) {
        int a0 = 0, b0 = 0, a1 = 0, b1 = 0;

        if (!DevSpan(w, h, r, j - dy, &a0, &b0))
            continue;
        if (!DevSpan(w, h, r, j, &a1, &b1))
            a1 = b1 = 0;
        DevSpanMinus(x, y + j, a0, b0, a1, b1, c, alpha);
    }
}

/* box-shadow inset 0 dy 0: the pixels of the shape that the shape moved dy rows (down when dy > 0, up when dy < 0) does
 * not cover; c, or blended toward c when alpha > 0. */
UI_FN void DevInset(int x, int y, int w, int h, int r, int dy, uint16_t c, int alpha)
{
    for (int j = 0; j < h; ++j) {
        int a0 = 0, b0 = 0, a1 = 0, b1 = 0;

        if (!DevSpan(w, h, r, j, &a0, &b0))
            continue;
        if (!DevSpan(w, h, r, j - dy, &a1, &b1))
            a1 = b1 = 0;
        DevSpanMinus(x, y + j, a0, b0, a1, b1, c, alpha);
    }
}

/* A filled rectangle whose corners are cut by the circle of radius r (r <= 10; a pixel is kept when its centre is
 * inside). */
UI_FN void DevRound(int x, int y, int w, int h, int r, uint16_t c)
{
    if (w <= 0 || h <= 0)
        return;
    r = DevRadius(w, h, r);
    for (int j = 0; j < r; ++j) {
        int cut = DevCut(r, j);

        Fill(x + cut, y + j, w - 2 * cut, 1, c);
        Fill(x + cut, y + h - 1 - j, w - 2 * cut, 1, c);
    }
    Fill(x, y + r, w, h - 2 * r, c);
}

/* The shape blended toward c by alpha over what is there. */
UI_FN void DevRoundBlend(int x, int y, int w, int h, int r, uint16_t c, int alpha)
{
    r = DevRadius(w, h, r);
    for (int j = 0; j < h; ++j) {
        int a = 0, b = 0;

        if (DevSpan(w, h, r, j, &a, &b))
            DevSpanPaint(x, y + j, a, b, c, alpha);
    }
}

/* A disc s pixels across, its box's top-left corner at (x, y): the pixels whose centre is inside the circle. */
UI_FN void DevDisc(int x, int y, int s, uint16_t c)
{
    for (int j = 0; j < s; ++j)
        for (int i = 0; i < s; ++i) {
            int dx = 2 * i + 1 - s, dy = 2 * j + 1 - s;

            if (dx * dx + dy * dy <= s * s)
                Px(x + i, y + j, c);
        }
}

/* ---- The shell, the screen, keys, holes, rows ---------------------------------------------------------------------- */

/* A screw: a dark disc in the shell, its top-left lit by a pixel. */
UI_FN void DevScrew(int x, int y)
{
    uint16_t c = DevBlend(sDevShell, 0x0000, DEV_A_SCREW);

    DevDisc(x, y, 5, c);
    Px(x + 1, y + 1, DevBlend(c, 0xFFFF, DEV_A_SCREW));
}

/* The whole screen in the shell colour: a light top edge (rows 0-1), a dark bottom edge (rows 237-239) and the screws
 * DEV_SCREWS_TOP / DEV_SCREWS_BOTTOM ask for, 5x5 in the corners. */
UI_FN void DevShell(int screws)
{
    Fill(0, 0, W, 2, DevBlend(sDevShell, 0xFFFF, DEV_A_LIGHT));
    Fill(0, 2, W, H - 5, sDevShell);
    Fill(0, H - 3, W, 3, DevBlend(sDevShell, 0x0000, DEV_A_DARK));
    if (screws & DEV_SCREWS_TOP) {
        DevScrew(2, 2);
        DevScrew(313, 2);
    }
    if (screws & DEV_SCREWS_BOTTOM) {
        DevScrew(2, 232);
        DevScrew(313, 232);
    }
}

/* An embedded screen in `outer`: the bezel (radius 6, its outer pixel ring DEV_BEZEL_LINE), a light row under it and the
 * square paper 6 px in. Returns the paper. */
UI_FN CtrPokeTouchRect DevScreen(const CtrPokeTouchRect *outer)
{
    int x = outer->x, y = outer->y, w = outer->w, h = outer->h, r = DevRadius(w, h, 6), ri = r > 0 ? r - 1 : 0;
    CtrPokeTouchRect paper = {(int16_t)(x + 6), (int16_t)(y + 6), (int16_t)(w - 12), (int16_t)(h - 12)};

    DevOuter(x, y, w, h, r, 1, 0xFFFF, DEV_A_LIGHT);
    DevRound(x, y, w, h, r, DEV_BEZEL);
    for (int j = 0; j < h; ++j) {                    /* inset 0 0 0 1px: the shape less the shape 1 px in */
        int a0 = 0, b0 = 0, a1 = 0, b1 = 0;

        if (!DevSpan(w, h, r, j, &a0, &b0))
            continue;
        if (DevSpan(w - 2, h - 2, ri, j - 1, &a1, &b1)) {
            ++a1;
            ++b1;
        } else {
            a1 = b1 = 0;
        }
        DevSpanMinus(x, y + j, a0, b0, a1, b1, DEV_BEZEL_LINE, 0);
    }
    Fill(paper.x, paper.y, paper.w, paper.h, DEV_PAPER);
    return paper;
}

UI_FN uint16_t DevKeyFace(int look, int sunk)
{
    if (look == DEV_LOOK_DARK)
        return DEV_BEZEL;
    if (look == DEV_LOOK_FAINT)
        return DEV_FAINT;
    return sunk ? DEV_SUNK : DEV_KEY;
}

/* The LED of a chosen key: a green disc s across at (x, y) with a 1 px glow on the face. */
UI_FN void DevLed(int x, int y, int s, uint16_t face)
{
    uint16_t glow = DevBlend(face, DEV_LED, 96);

    for (int j = -1; j <= s; ++j)
        for (int i = -1; i <= s; ++i) {
            int dx = 2 * i + 1 - s, dy = 2 * j + 1 - s, d = dx * dx + dy * dy;

            if (d <= s * s)
                Px(x + i, y + j, DEV_LED);
            else if (d <= (s + 2) * (s + 2))
                Px(x + i, y + j, glow);
        }
}

/* A key whose resting face is `r` (the boards' numbers). Up: the face at r.y, DEV_DROP rows of drop under it; a KEY has a
 * white top line and two inner bottom rows DEV_KEY_EDGE, a DARK one a DEV_BEZEL_LINE top line, a FAINT one two inner
 * bottom rows DEV_FAINT_EDGE. Sunk (any look): the face DEV_SINK rows lower, two inner top rows (DEV_SUNK_EDGE on a KEY,
 * the face darkened on the others), one row of drop, the LED at its top right; rows r.y..r.y+1 are left to what is
 * under. Paints only DevKeyArea(r). Returns the face's top row. */
UI_FN int DevKey(const CtrPokeTouchRect *r, int look, int sunk, int radius)
{
    int x = r->x, w = r->w, h = r->h, y = r->y + (sunk ? DEV_SINK : 0), rr = DevRadius(w, h, radius);
    uint16_t face = DevKeyFace(look, sunk);

    if (w <= 0 || h <= 0)
        return y;
    DevOuter(x, y, w, h, rr, sunk ? 1 : DEV_DROP, 0x0000, DEV_A_DROP);
    DevRound(x, y, w, h, rr, face);
    if (sunk) {
        int s = h >= 30 ? 5 : 4;

        DevInset(x, y, w, h, rr, 2, look == DEV_LOOK_KEY ? DEV_SUNK_EDGE : DevBlend(face, 0x0000, DEV_A_DROP), 0);
        DevLed(x + w - 7 - s, y + (h >= 40 ? 6 : 4), s, face);
    } else {
        if (look == DEV_LOOK_KEY)
            DevInset(x, y, w, h, rr, 1, 0xFFFF, 0);
        else if (look == DEV_LOOK_DARK)
            DevInset(x, y, w, h, rr, 1, DEV_BEZEL_LINE, 0);
        if (look != DEV_LOOK_DARK)
            DevInset(x, y, w, h, rr, -2, look == DEV_LOOK_KEY ? DEV_KEY_EDGE : DEV_FAINT_EDGE, 0);
    }
    return y;
}

/* Every pixel of a key's area (DevKeyArea(r)) halfway to the shell: a group shown but not for now. */
UI_FN void DevDim(const CtrPokeTouchRect *r)
{
    int x0 = r->x < 0 ? 0 : r->x, y0 = r->y < 0 ? 0 : r->y;
    int x1 = r->x + r->w > W ? W : r->x + r->w, y1 = r->y + r->h + DEV_DROP > H ? H : r->y + r->h + DEV_DROP;

    for (int x = x0; x < x1 && y1 > y0; ++x) {
        uint16_t *p = sCanvas + (size_t)x * H + (H - (size_t)y1);

        for (int n = y1 - y0; n > 0; --n, ++p)
            *p = DevBlend(sDevShell, *p, DEV_A_DIM);
    }
}

/* A hole in the shell (an empty party bay): what is there darkened, its three top inner rows darker still, a light row
 * under it. */
UI_FN void DevHole(const CtrPokeTouchRect *r, int radius)
{
    int rr = DevRadius(r->w, r->h, radius);

    DevOuter(r->x, r->y, r->w, r->h, rr, 1, 0xFFFF, DEV_A_LIGHT);
    DevRoundBlend(r->x, r->y, r->w, r->h, rr, 0x0000, DEV_A_HOLE);
    DevInset(r->x, r->y, r->w, r->h, rr, 3, 0x0000, DEV_A_SCREW);
}

/* A list row on the paper: inverted (DEV_BEZEL) when chosen, the paper otherwise; its text in DevRowInk(chosen). */
UI_FN void DevRow(const CtrPokeTouchRect *r, int chosen)
{
    Fill(r->x, r->y, r->w, r->h, chosen ? DEV_BEZEL : DEV_PAPER);
}

UI_FN uint16_t DevRowInk(int chosen)
{
    return chosen ? DEV_PAPER : DEV_INK;
}

/* HOME's RUN / SPD: a white button with a 1 px DEV_BUTTON_EDGE border, radius 2; pressed, inverted (DEV_BEZEL). Its
 * text in DevRowInk(pressed). */
UI_FN void DevPaperButton(const CtrPokeTouchRect *r, int pressed)
{
    if (pressed) {
        DevRound(r->x, r->y, r->w, r->h, 2, DEV_BEZEL);
        return;
    }
    DevRound(r->x, r->y, r->w, r->h, 2, DEV_BUTTON_EDGE);
    DevRound(r->x + 1, r->y + 1, r->w - 2, r->h - 2, 1, 0xFFFF);
}

/* ---- Text ---------------------------------------------------------------------------------------------------------- */

/* The small font's glyphs (half-width, capitals rows 4-10), decoded once. It has no Japanese glyphs: those are spaces. */
static PtGlyph sDevSmall[256] __attribute__((unused));
static bool sDevSmallReady[256] __attribute__((unused));

UI_FN const PtGlyph *DevSmallGlyph(const char **s)
{
    unsigned code = GlyphCode(s);

    if (code > 0xffu)
        code = 0;
    if (!sDevSmallReady[code]) {
        const uint8_t *font = CtrTwinSmallFontData();
        PtGlyph *g = &sDevSmall[code];
        unsigned w = CtrTwinSmallFontWidth(code);

        g->width = (uint8_t)(w > 8 ? 8 : w);
        if (font)
            TwinSmallGlyphDecode(font + code * 32, g->width, g->rows);
        else
            memset(g->rows, 0, sizeof(g->rows));
        sDevSmallReady[code] = true;
    }
    return &sDevSmall[code];
}

/* The glyphs of `s` in the normal or the small font from x, glyph row 0 at y, each drawn while it ends by `limit`: ink,
 * and the font's shadow pixels in `shadow` (none when shadow is ink). Returns the x after the last glyph drawn. */
UI_FN int DevGlyphs(const char *s, int x, int y, int limit, uint16_t ink, uint16_t shadow, int small)
{
    while (*s) {
        const PtGlyph *g = small ? DevSmallGlyph(&s) : Glyph(&s);

        if (x + g->width > limit)
            break;
        for (int row = 0; row < 16; ++row) {
            uint32_t bits = g->rows[row];

            for (int col = 0; bits && col < g->width; ++col, bits >>= 2) {
                unsigned v = bits & 3u;

                if (v == 1)
                    Px(x + col, y + row, ink);
                else if (v == 2 && shadow != ink)
                    Px(x + col, y + row, shadow);
            }
        }
        x += g->width;
    }
    return x;
}

UI_FN int DevTextWidth(const char *s)
{
    return UiTextWidth(s, 1);
}

UI_FN int DevSmallTextWidth(const char *s)
{
    int w = 0;

    while (*s)
        w += DevSmallGlyph(&s)->width;
    return w;
}

/* Normal-font text from x, capitals from `top`, stopping before `limit`: ink over shadow (no shadow when it is ink). */
UI_FN void DevTextInk(const char *s, int x, int top, int limit, uint16_t ink, uint16_t shadow)
{
    DevGlyphs(s, x, top - 3, limit, ink, shadow, 0);
}

/* The paper's ink and its shadow. */
UI_FN void DevText(const char *s, int x, int top, int limit)
{
    DevTextInk(s, x, top, limit, DEV_INK, DEV_INK_SH);
}

/* The secondary ink, no shadow. */
UI_FN void DevText2(const char *s, int x, int top, int limit)
{
    DevTextInk(s, x, top, limit, DEV_INK2, DEV_INK2);
}

/* Ending at `right`, in `ink`, no shadow. */
UI_FN void DevTextRight(const char *s, int right, int top, uint16_t ink)
{
    DevTextInk(s, right - DevTextWidth(s), top, right, ink, ink);
}

/* The small font (the boards' 7-9 px labels), capitals from `top`, in `ink`, no shadow. */
UI_FN void DevSmallText(const char *s, int x, int top, uint16_t ink, int limit)
{
    DevGlyphs(s, x, top - 4, limit, ink, ink, 1);
}

/* The first n bytes of s as one line, a line break drawn as the space it stands for. */
UI_FN void DevWrapCopy(char *line, const char *s, int n)
{
    for (int i = 0; i < n; ++i)
        line[i] = s[i] == '\n' ? ' ' : s[i];
    line[n] = 0;
}

/* Text wrapped at spaces and line breaks into at most maxLines lines from `top`, DEV_LINE_H apart, the first firstWidth
 * wide and the others `width`; a word wider than a line is cut at its end. A line break (FireRed's descriptions keep the
 * ones of its own window) is a space here. Breaking only there keeps UTF-8 and the raw-code escapes whole. Returns the
 * lines drawn. (Rework 1's KitWrapFirst.) */
UI_FN int DevWrapFirst(const char *s, int x, int top, int firstWidth, int width, int maxLines, uint16_t ink,
                       uint16_t shadow)
{
    char line[128];
    int lines = 0;

    while (lines < maxLines) {
        int fit = 0, end = 0, room = lines ? width : firstWidth;

        while (*s == ' ' || *s == '\n')
            ++s;
        if (!*s)
            break;
        for (;;) {
            while (s[end] && s[end] != ' ' && s[end] != '\n' && end < (int)sizeof(line) - 1)
                ++end;
            DevWrapCopy(line, s, end);
            if (fit > 0 && DevTextWidth(line) > room)
                break;
            fit = end;
            if (!s[end] || end >= (int)sizeof(line) - 1)
                break;
            ++end;                          /* past the space, to the next word */
        }
        DevWrapCopy(line, s, fit);
        DevTextInk(line, x, top + lines * DEV_LINE_H, x + room, ink, shadow);
        ++lines;
        s += fit;
    }
    return lines;
}

/* DevWrapFirst with every line `width` wide. */
UI_FN int DevWrap(const char *s, int x, int top, int width, int maxLines, uint16_t ink, uint16_t shadow)
{
    return DevWrapFirst(s, x, top, width, width, maxLines, ink, shadow);
}

/* `s` cut to fit `room` with "..." after it, into out (size bytes). */
UI_FN void DevCutText(const char *s, int room, char *out, size_t size)
{
    const char *p = s;
    int w = 0, dots = DevTextWidth("...");

    while (*p) {
        const char *q = p;
        int gw = Glyph(&q)->width;

        if (w + gw + dots > room || (size_t)(q - s) + 4 > size)
            break;
        w += gw;
        p = q;
    }
    memcpy(out, s, (size_t)(p - s));
    memcpy(out + (p - s), "...", 4);
}

/* A key's label, centred on its face in the look's ink and shadow; cut with "..." when wider than the key less 8 px (and
 * then counted in sDevKeyCut). */
UI_FN void DevKeyLabel(const CtrPokeTouchRect *r, int faceY, int look, int sunk, const char *s)
{
    char cut[64];
    uint16_t ink, shadow;
    int w = DevTextWidth(s);

    if (look == DEV_LOOK_DARK)
        ink = shadow = DEV_DARK_INK;
    else if (look == DEV_LOOK_FAINT)
        ink = shadow = DEV_FAINT_INK;
    else if (sunk) {
        ink = DEV_SUNK_INK;
        shadow = DEV_SUNK_INK_SH;
    } else {
        ink = DEV_KEY_INK;
        shadow = 0xFFFF;
    }
    if (w > r->w - 8) {
        DevCutText(s, r->w - 8, cut, sizeof(cut));
        s = cut;
        w = DevTextWidth(s);
        ++sDevKeyCut;
    }
    DevTextInk(s, r->x + (r->w - w) / 2, faceY + (r->h - UI_TEXT_H) / 2, r->x + r->w, ink, shadow);
}

/* ---- Marks, bars, chips, icons ------------------------------------------------------------------------------------- */

/* A triangle pointing `dir` (0 up, 1 down, 2 left, 3 right), 9 px across, centred on (cx, cy). (Rework 1's KitTriangle.) */
UI_FN void DevTriangle(int cx, int cy, int dir, uint16_t c)
{
    for (int i = 0; i < 5; ++i) {
        int len = 9 - 2 * i;

        if (dir == 0)
            Fill(cx - len / 2, cy + 2 - i, len, 1, c);
        else if (dir == 1)
            Fill(cx - len / 2, cy - 2 + i, len, 1, c);
        else if (dir == 2)
            Fill(cx + 2 - i, cy - len / 2, 1, len, c);
        else
            Fill(cx - 2 + i, cy - len / 2, 1, len, c);
    }
}

/* FireRed's HP thresholds: green over half, yellow over a fifth, red below. */
UI_FN uint16_t DevHpColour(unsigned hp, unsigned max)
{
    return hp * 2 > max ? DEV_HP_GREEN : hp * 5 > max ? DEV_HP_YELLOW : DEV_HP_RED;
}

/* An HP bar: a 1 px frame, the track and hp of max filled (at least a pixel while hp is not 0). */
UI_FN void DevHpBar(int x, int y, int w, int h, unsigned hp, unsigned max)
{
    int inner = w - 2, fill = max ? (int)((unsigned long)hp * (unsigned)(inner > 0 ? inner : 0) / max) : 0;

    if (w < 2 || h < 2)
        return;
    if (hp && fill < 1)
        fill = 1;
    if (fill > inner)
        fill = inner;
    Fill(x, y, w, h, DEV_HP_FRAME);
    Fill(x + 1, y + 1, inner, h - 2, DEV_HP_TRACK);
    if (fill > 0)
        Fill(x + 1, y + 1, fill, h - 2, DevHpColour(hp, max));
}

/* A chip h tall at (x, y): `text` in the small font, 3 px in on each side, in the ink that reads on bg. Returns its
 * width. */
UI_FN int DevChip(int x, int y, int h, const char *text, uint16_t bg)
{
    int w = DevSmallTextWidth(text) + 6;

    Fill(x, y, w, h, bg);
    DevSmallText(text, x + 3, y + (h - 6) / 2, UiInkOn(bg), x + w);
    return w;
}

/* A label chip ("HP", "EXP"): the small font with 1 px around it, 9 px tall. Returns its width. */
UI_FN int DevLabelChip(int x, int y, const char *text, uint16_t bg, uint16_t ink)
{
    int w = DevSmallTextWidth(text) + 2;

    Fill(x, y, w, 9, bg);
    DevSmallText(text, x + 1, y + 1, ink, x + w);
    return w;
}

/* A move's PP ink: none left, a quarter or less, half or less, else `normal`. */
UI_FN uint16_t DevPpInk(unsigned pp, unsigned max, uint16_t normal)
{
    return pp == 0 ? DEV_LOW_INK : pp * 4 <= max ? DEV_PP_QUARTER : pp * 2 <= max ? DEV_PP_HALF : normal;
}

/* The key shown sunk: the one under a held touch, else the game's cursor. */
UI_FN int DevChosen(int id, int cursor, int pressed)
{
    return pressed >= 0 ? id == pressed : id == cursor;
}

/* A gender mark, 7x9 (v0.6.0's shapes) with the paper's text shadow: 0 male, 1 female, 2 none (CtrBottomParty_Gender). */
UI_FN void DevGender(int gender, int x, int y)
{
    static const uint8_t kMale[9] = {0x0F, 0x03, 0x05, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00};
    static const uint8_t kFemale[9] = {0x1C, 0x22, 0x22, 0x22, 0x1C, 0x08, 0x3E, 0x08, 0x08};
    const uint8_t *g = gender == 0 ? kMale : kFemale;

    if (gender != 0 && gender != 1)
        return;
    for (int pass = 0; pass < 2; ++pass)
        for (int row = 0; row < 9; ++row)
            for (int col = 0; col < 7; ++col)
                if (g[row] & (0x40 >> col)) {
                    if (pass)
                        Px(x + col, y + row, gender == 0 ? DEV_MALE : DEV_FEMALE);
                    else
                        Px(x + col + 1, y + row + 1, DEV_INK_SH);
                }
}

/* A party slot's icon, FireRed's frame 0 (BPartyIcon): 32x32, or half (16x16: of each 2x2 block its first pixel that is
 * not clear, in reading order); faint, every colour toward DEV_FAINT. */
UI_FN void DevMonIcon(int slot, unsigned icon, int x, int y, int half, int faint)
{
    int size = half ? 16 : 32, step = half ? 2 : 1;

    if (slot < 0 || slot >= 6 || !BPartyIcon(slot, icon))
        return;
    for (int py = 0; py < size; ++py)
        for (int px = 0; px < size; ++px) {
            const uint8_t *src = sBPIcon[slot].pixels + py * step * 32 + px * step;
            uint8_t v = src[0];

            if (half && !v)
                v = src[1] ? src[1] : src[32] ? src[32] : src[33];
            if (v)
                Px(x + px, y + py, faint ? DevBlend(DEV_FAINT, sBPIcon[slot].pal[v], DEV_A_FAINT_ART) : sBPIcon[slot].pal[v]);
        }
}
