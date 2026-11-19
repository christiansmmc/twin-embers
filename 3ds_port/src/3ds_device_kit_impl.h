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
 * boards' 10-12 px text and its small font (recipe 0120) for their 7-9 px labels; `top` is where the capitals start,
 * and text is clipped only across (at its limit), never down.
 * Every function is UI_FN: a view uses the ones it needs.
 */
#include "3ds_device_layout.h"

/* Colours (RGB565 of the boards' values). White: key top lines, light rows, chip text. */
#define DEV_WHITE          0xFFFF
/* The shell, by OPTIONS' Shell colour (shell & 3). */
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
/* Secondary text on a key: a price, "PP", "STATUS", SPD, a bay's level. */
#define DEV_KEY_INK2       UI_RGB(0x6A, 0x6A, 0x6A)
/* A footer's title line (DevNoteTitle). */
#define DEV_TITLE_INK      UI_RGB(0x3A, 0x3A, 0x3A)
/* The dark key's label and arrows (its face is DEV_BEZEL, its top line DEV_BEZEL_LINE). */
#define DEV_DARK_INK       UI_RGB(0xF2, 0xEC, 0xE0)
/* ▲▼ (or ◀ ▶) with nothing that way (the board's). */
#define DEV_ARROW_OFF      UI_RGB(0xB8, 0xB0, 0xA0)
/* A key not for use now, beige (a fainted Pokemon's, an unused cream key: the board's OFF), its ink beige too. */
#define DEV_FAINT          UI_RGB(0xD2, 0xCA, 0xB8)
#define DEV_FAINT_EDGE     UI_RGB(0xBC, 0xB2, 0x9C)
#define DEV_FAINT_INK      UI_RGB(0x8A, 0x84, 0x78)
/* A rule on the paper (HOME's panel, the Summary's MOVES). */
#define DEV_RULE           UI_RGB(0xD8, 0xD8, 0xD0)
/* A white pill's border on the paper (the Summary's STATS switch). */
#define DEV_BUTTON_EDGE    UI_RGB(0xB0, 0xB0, 0xA8)
/* On the paper: a line (an off page dot, a paper hole's rim, a name slot's underline); a paper hole (a badge not
 * earned); a mark that is on (the page shown's dot, the next name slot's underline: the blue shell's value). */
#define DEV_PAPER_LINE     UI_RGB(0xC8, 0xC8, 0xC0)
#define DEV_PAPER_HOLE     UI_RGB(0xE2, 0xE2, 0xDA)
#define DEV_MARK_ON        UI_RGB(0x3E, 0x4C, 0x6B)
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
/* Tags: a move with STAB, a move the Pokemon would learn (NEW). */
#define DEV_TAG_STAB       UI_RGB(0x5A, 0x64, 0x78)
#define DEV_TAG_NEW        UI_RGB(0x2A, 0x7A, 0x3E)
/* The dark shadow of the white text on the chips of a fixed colour (the boards'); a chip whose colour comes from data
 * (a type) takes DevChipShadow. PSN and FNT have none (DEV_WHITE). */
#define DEV_FX_SUPER_SH    UI_RGB(0x14, 0x5A, 0x28)
#define DEV_FX_NOTVERY_SH  UI_RGB(0x7A, 0x30, 0x08)
#define DEV_FX_EFFECTIVE_SH UI_RGB(0x3A, 0x3E, 0x52)
#define DEV_CAT_SPECIAL_SH UI_RGB(0x2E, 0x40, 0x70)
/* Inks: low HP, PP at a quarter and at half, the blue ink, a raised stat, the gender marks (female: v0.6.0's red). */
#define DEV_LOW_INK        UI_RGB(0xB0, 0x30, 0x20)
#define DEV_PP_QUARTER     UI_RGB(0xB0, 0x4A, 0x10)
#define DEV_PP_HALF        UI_RGB(0x94, 0x6E, 0x06)
#define DEV_BLUE_INK       UI_RGB(0x2E, 0x5A, 0xA0)
#define DEV_STAT_UP        UI_RGB(0xC0, 0x38, 0x28)
#define DEV_MALE           UI_RGB(0x48, 0x78, 0xC8)
#define DEV_FEMALE         UI_RGB(255, 110, 120)

/* Alphas, of 256: the shell's light top edge (and the light row under a screen), its dark bottom edge, a screw, a key's
 * drop, a hole and the light row under it, a dimmed group, a fainted Pokemon's icon, a chip's shadow from its colour. */
#define DEV_A_LIGHT        64
#define DEV_A_DARK         77
#define DEV_A_SCREW        90
#define DEV_A_DROP         97
#define DEV_A_HOLE         72
#define DEV_A_HOLE_LIGHT   51
#define DEV_A_DIM          128
#define DEV_A_FAINT_ART    115
#define DEV_A_CHIP_SH      114      /* within one RGB565 step of all seven board pairs (measured) */

#define DEV_SCREWS_TOP     1
#define DEV_SCREWS_BOTTOM  2
/* A sunk key's LED (the board's TECLAS): DEV_LED_SIZE across, its left DEV_LED_IN px in from the key's right, its top
 * DEV_LED_TOP rows under the face's, whatever the key's height. */
#define DEV_LED_SIZE       4
#define DEV_LED_IN         11
#define DEV_LED_TOP        4
#define DEV_LINE_H         13       /* DevWrap's lines */
/* The small font's lines: a footer's (the board's 10 px), and tight, the board's 9 px (the Bag's description, the
 * frame's hints, OPTIONS, the PC box). */
#define DEV_SMALL_LINE     10
#define DEV_SMALL_LINE_TIGHT 9

enum { DEV_LOOK_KEY, DEV_LOOK_DARK, DEV_LOOK_FAINT };

static uint16_t sDevShell __attribute__((unused)) = DEV_SHELL_RED;
/* Key labels cut to fit since the counter was last cleared: the host harness requires none. */
static int sDevKeyCut __attribute__((unused));

/* What a view draws, told to a board maker (tools/make_device_boards.py): DEV_TRACE(op, x, y, w, h, a, b, s) is called
 * with each piece the kit draws, in draw order, {x, y, w, h} its rectangle; a is a look, a colour or a kind, b sunk, a
 * font or a direction, s a text:
 *   BEGIN   a new whole draw (DevBegin): a the shell colour; what was told before it is gone
 *   SHELL   the shell (the whole screen): a its screws (DEV_SCREWS_*: all four)
 *   SCREEN  an embedded screen: its outer rectangle (the paper is 6 px in)
 *   KEY     a key's resting face (sunk: drawn DEV_SINK lower, its LED at its top right): a the look, b sunk | its
 *           corners' radius as drawn (DevRadius'd) << 1
 *   DIM     the pixels dimmed halfway to the shell
 *   HOLE    a hole in the shell: a its corners' radius
 *   ROW     a list row: a chosen (inverted)
 *   RECT    a plain fill: a its colour
 *   LED     a lone LED (not a sunk key's): w and h its size
 *   TEXT    the glyphs drawn: y where the capitals start, w their width, h their shadow colour (the ink: none), a the
 *           ink, b the font (0 normal, 1 small, 2 a sign drawn as a shape: s its character in UTF-8), s the text (its
 *           first glyphs, as many as fill w)
 *   CHIP    a chip with its text: a its colour, b the text's shadow (DEV_WHITE: none), s the text
 *   BAR     a bar (DevBar: an HP bar, a progress bar): a the colour of its fill, b the fill's width
 *   TRI     a triangle: its 9x5 (5x9 across) box, a its colour, b the direction (DevTriangle's)
 *   DISC    a disc (DevSpot, DevPaperHole): its box, a its colour, b its 2 px top inset's colour (DEV_WHITE: none)
 *   ART     art a view draws (DevArt): a its kind (DEV_ART_*)
 * The game defines nothing and it does nothing; the host harness defines it before including the renderer. */
#ifndef DEV_TRACE
#define DEV_TRACE(op, x, y, w, h, a, b, s) \
    ((void)(op), (void)(x), (void)(y), (void)(w), (void)(h), (void)(a), (void)(b), (void)(s))
#endif
/* The kinds of art a view draws (a board shows a stand-in for each), and the pieces DEV_TRACE is told of. */
enum { DEV_ART_ICON, DEV_ART_PICTURE, DEV_ART_ITEM, DEV_ART_BADGE, DEV_ART_WALLPAPER, DEV_ART_MARK, DEV_ART_SPRITE,
       DEV_ART_FRAME, DEV_ART_LOGO, DEV_ART_GBA };
enum { DEV_TRACE_BEGIN = 0, DEV_TRACE_ART, DEV_TRACE_SHELL, DEV_TRACE_SCREEN, DEV_TRACE_KEY, DEV_TRACE_DIM, DEV_TRACE_HOLE,
       DEV_TRACE_ROW, DEV_TRACE_RECT, DEV_TRACE_LED, DEV_TRACE_TEXT, DEV_TRACE_CHIP, DEV_TRACE_BAR, DEV_TRACE_TRI,
       DEV_TRACE_DISC };

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
    DEV_TRACE(DEV_TRACE_BEGIN, 0, 0, W, H, sDevShell, 0, NULL);
}

/* The canvas at (x, y); 0 off the screen. */
UI_FN uint16_t DevGet(int x, int y)
{
    if ((unsigned)x >= W || (unsigned)y >= H)
        return 0;
    return sCanvas[(size_t)x * H + (H - 1 - (size_t)y)];
}

/* A plain fill that draws something (a rule, a cursor's frame, an underline, a panel): c over {x, y, w, h}. */
UI_FN void DevRect(int x, int y, int w, int h, uint16_t c)
{
    DEV_TRACE(DEV_TRACE_RECT, x, y, w, h, c, 0, NULL);
    Fill(x, y, w, h, c);
}

/* Art a view draws (a picture, an icon, a sprite, a frame's sample...) of `kind` (DEV_ART_*) in {x, y, w, h}, announced
 * (the art itself is the view's to draw): where a board puts its stand-in. */
UI_FN void DevArt(int kind, int x, int y, int w, int h)
{
    DEV_TRACE(DEV_TRACE_ART, x, y, w, h, kind, 0, NULL);
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

/* Pixel (i, j) of a disc s pixels across (0, 0 its box's top-left) is the disc's: its centre is inside the circle. */
UI_FN int DevInDisc(int s, int i, int j)
{
    int dx = 2 * i + 1 - s, dy = 2 * j + 1 - s;

    return i >= 0 && i < s && j >= 0 && j < s && dx * dx + dy * dy <= s * s;
}

/* A disc s pixels across, its box's top-left corner at (x, y): the pixels whose centre is inside the circle. */
UI_FN void DevDisc(int x, int y, int s, uint16_t c)
{
    for (int j = 0; j < s; ++j)
        for (int i = 0; i < s; ++i)
            if (DevInDisc(s, i, j))
                Px(x + i, y + j, c);
}

/* ---- The shell, the screen, keys, holes, rows ---------------------------------------------------------------------- */

/* A screw: a dark disc in the shell, its top-left lit by a pixel. */
UI_FN void DevScrew(int x, int y)
{
    uint16_t c = DevBlend(sDevShell, 0x0000, DEV_A_SCREW);

    DevDisc(x, y, 5, c);
    Px(x + 1, y + 1, DevBlend(c, DEV_WHITE, DEV_A_SCREW));
}

/* The whole screen in the shell colour: a light top edge (rows 0-1), a dark bottom edge (rows 237-239) and the four
 * screws, 5x5 in the corners (every view has them: the Definitiva's rule). */
UI_FN void DevShell(void)
{
    DEV_TRACE(DEV_TRACE_SHELL, 0, 0, W, H, DEV_SCREWS_TOP | DEV_SCREWS_BOTTOM, 0, NULL);
    Fill(0, 0, W, 2, DevBlend(sDevShell, DEV_WHITE, DEV_A_LIGHT));
    Fill(0, 2, W, H - 5, sDevShell);
    Fill(0, H - 3, W, 3, DevBlend(sDevShell, 0x0000, DEV_A_DARK));
    DevScrew(2, 2);
    DevScrew(313, 2);
    DevScrew(2, 232);
    DevScrew(313, 232);
}

/* An embedded screen in `outer`: the bezel (radius 6, its outer pixel ring DEV_BEZEL_LINE), a light row under it (so it
 * paints outer.h + 1 rows) and the square paper 6 px in. Returns the paper. */
UI_FN CtrPokeTouchRect DevScreen(const CtrPokeTouchRect *outer)
{
    int x = outer->x, y = outer->y, w = outer->w, h = outer->h, r = DevRadius(w, h, 6), ri = r > 0 ? r - 1 : 0;
    CtrPokeTouchRect paper = {(int16_t)(x + 6), (int16_t)(y + 6), (int16_t)(w - 12), (int16_t)(h - 12)};

    DEV_TRACE(DEV_TRACE_SCREEN, x, y, w, h, 0, 0, NULL);
    DevOuter(x, y, w, h, r, 1, DEV_WHITE, DEV_A_LIGHT);
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

/* The LED's pixels: a green disc s across at (x, y) with a 1 px glow on the face. (A sunk key's own, which its key
 * stands for when told; DevLed's.) */
UI_FN void DevLedPixels(int x, int y, int s, uint16_t face)
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

/* An LED lit on its own (the naming keyboard's page key FireRed's cursor turns to): DevLedPixels. */
UI_FN void DevLed(int x, int y, int s, uint16_t face)
{
    DEV_TRACE(DEV_TRACE_LED, x, y, s, s, 0, 0, NULL);
    DevLedPixels(x, y, s, face);
}

/* A key whose resting face is `r` (the boards' numbers). Up: the face at r.y, DEV_DROP rows of drop under it; a KEY has a
 * white top line and two inner bottom rows DEV_KEY_EDGE, a DARK one a DEV_BEZEL_LINE top line, a FAINT one two inner
 * bottom rows DEV_FAINT_EDGE. Sunk (any look): the face DEV_SINK rows lower, two inner top rows (DEV_SUNK_EDGE on a KEY,
 * the face darkened on the others), one row of drop, the LED (DEV_LED_SIZE at DEV_LED_IN from the right, DEV_LED_TOP
 * under the face's top); rows r.y..r.y+1 are left to what is under. Paints only DevKeyArea(r). Returns the face's top
 * row. */
UI_FN int DevKey(const CtrPokeTouchRect *r, int look, int sunk, int radius)
{
    int x = r->x, w = r->w, h = r->h, y = r->y + (sunk ? DEV_SINK : 0), rr = DevRadius(w, h, radius);
    uint16_t face = DevKeyFace(look, sunk);

    if (w <= 0 || h <= 0)
        return y;
    DEV_TRACE(DEV_TRACE_KEY, x, r->y, w, h, look, (sunk != 0) | (rr << 1), NULL);
    DevOuter(x, y, w, h, rr, sunk ? 1 : DEV_DROP, 0x0000, DEV_A_DROP);
    DevRound(x, y, w, h, rr, face);
    if (sunk) {
        DevInset(x, y, w, h, rr, 2, look == DEV_LOOK_KEY ? DEV_SUNK_EDGE : DevBlend(face, 0x0000, DEV_A_DROP), 0);
        DevLedPixels(x + w - DEV_LED_IN, y + DEV_LED_TOP, DEV_LED_SIZE, face);
    } else {
        if (look == DEV_LOOK_KEY)
            DevInset(x, y, w, h, rr, 1, DEV_WHITE, 0);
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

    DEV_TRACE(DEV_TRACE_DIM, x0, y0, x1 - x0, y1 - y0, 0, 0, NULL);
    for (int x = x0; x < x1 && y1 > y0; ++x) {
        uint16_t *p = sCanvas + (size_t)x * H + (H - (size_t)y1);

        for (int n = y1 - y0; n > 0; --n, ++p)
            *p = DevBlend(sDevShell, *p, DEV_A_DIM);
    }
}

/* A hole in the shell (an empty party bay): what is there darkened, its three top inner rows darker still, a light row
 * under it (so it paints r.h + 1 rows). */
UI_FN void DevHole(const CtrPokeTouchRect *r, int radius)
{
    int rr = DevRadius(r->w, r->h, radius);

    DEV_TRACE(DEV_TRACE_HOLE, r->x, r->y, r->w, r->h, rr, 0, NULL);
    DevOuter(r->x, r->y, r->w, r->h, rr, 1, DEV_WHITE, DEV_A_HOLE_LIGHT);
    DevRoundBlend(r->x, r->y, r->w, r->h, rr, 0x0000, DEV_A_HOLE);
    DevInset(r->x, r->y, r->w, r->h, rr, 3, 0x0000, DEV_A_SCREW);
}

/* A list row on the paper: inverted (DEV_BEZEL) when chosen, the paper otherwise; its text in DevRowInk(chosen). */
UI_FN void DevRow(const CtrPokeTouchRect *r, int chosen)
{
    DEV_TRACE(DEV_TRACE_ROW, r->x, r->y, r->w, r->h, chosen, 0, NULL);
    Fill(r->x, r->y, r->w, r->h, chosen ? DEV_BEZEL : DEV_PAPER);
}

UI_FN uint16_t DevRowInk(int chosen)
{
    return chosen ? DEV_PAPER : DEV_INK;
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

/* DevGlyphs' pixels, not told (a chip's text, which its chip tells). */
UI_FN int DevGlyphPixels(const char *s, int x, int y, int limit, uint16_t ink, uint16_t shadow, int small)
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

/* The glyphs of `s` in the normal or the small font from x, glyph row 0 at y, each drawn while it ends by `limit`: ink,
 * and the font's shadow pixels in `shadow` (none when shadow is ink). Text is clipped only across (at `limit`), never
 * down: a glyph's 16 rows are drawn wherever they fall. Returns the x after the last glyph drawn. */
UI_FN int DevGlyphs(const char *s, int x, int y, int limit, uint16_t ink, uint16_t shadow, int small)
{
    int end = DevGlyphPixels(s, x, y, limit, ink, shadow, small);

    DEV_TRACE(DEV_TRACE_TEXT, x, y + (small ? 4 : 3), end - x, shadow, ink, small, s);
    return end;
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

/* The code of FireRed's capital for the code of an accented small letter (à 0x16 -> À 0x01, ü 0xf6 -> Ü 0xf3, œ 0x25 ->
 * Œ 0x10 ...), read off LatinCode's table; any other code (a capital, ß, º, a letter without a capital) as it is. */
UI_FN unsigned DevCapitalCode(unsigned code)
{
    if (code == LatinCode(0xc5, 0x93))
        return LatinCode(0xc5, 0x92);
    for (unsigned b = 0xa0; b <= 0xbe; ++b)
        if (b != 0xb7 && LatinCode(0xc3, (unsigned char)b) == code) {
            unsigned capital = LatinCode(0xc3, (unsigned char)(b - 0x20));

            return capital ? capital : code;
        }
    return code;
}

/* `src` in upper case, as the boards and FireRed write their words, into dst (size bytes, always ended): a-z, and the
 * accented small letters whose capital the game's font has (UTF-8 à..þ, œ: ä->Ä, ó->Ó, ñ->Ñ...; and the same letters as
 * FireRed's own codes escaped by 0x01, as the bridges pass the ones recipe 0251's title case makes), become capitals;
 * the é of POKé (after "POK": POKéMON, POKéDEX, POKé BALL) stays small, as FireRed writes it; anything else (ß, digits,
 * signs, another raw FireRed code, a Japanese one) is copied as it is. A character that does not fit whole is left out.
 * The B views draw port words and game strings through it (a game string that arrives in proper case, v0.6.0's casing
 * or recipe 0251's, is shown as FireRed has it); a name the player typed is drawn as typed. */
UI_FN void DevUpper(char *dst, const char *src, size_t size)
{
    size_t n = 0;

    if (!size)
        return;
    while (*src) {
        unsigned char c = (unsigned char)src[0], next = (unsigned char)src[1];
        size_t len = ((c == 0x01 || c == 0x02) && next) || ((c == 0xc2 || c == 0xc3 || c == 0xc5) && (next & 0xc0) == 0x80) ? 2 : 1;
        char out[2] = {src[0], len > 1 ? src[1] : 0};

        if (len == 1 && c >= 'a' && c <= 'z')
            out[0] = (char)(c - 'a' + 'A');
        else if (c == 0xc3 && len == 2 && next >= 0xa0 && next <= 0xbe && next != 0xb7
                 && !(next == 0xa9 && n >= 3 && !memcmp(dst + n - 3, "POK", 3)) && LatinCode(0xc3, (unsigned char)(next - 0x20)))
            out[1] = (char)(next - 0x20);
        else if (c == 0xc5 && len == 2 && next == 0x93)
            out[1] = (char)0x92;                    /* œ -> Œ */
        else if (c == 0x01 && len == 2 && !(next == LatinCode(0xc3, 0xa9) && n >= 3 && !memcmp(dst + n - 3, "POK", 3)))
            out[1] = (char)DevCapitalCode(next);    /* an escaped FireRed code: its capital's */
        if (n + len >= size)
            break;
        memcpy(dst + n, out, len);
        n += len;
        src += len;
    }
    dst[n] = 0;
}

/* A Pokemon's name as FireRed shows it: its species' own name in capitals (`species`: the fill found it is that, recipe
 * 0251 having title-cased the species names), a name the player gave as typed. (The battle's and the Party's.) */
UI_FN const char *DevMonName(const char *name, int species, char *out, size_t size)
{
    if (!species)
        return name;
    DevUpper(out, name, size);
    return out;
}

/* The first n bytes of s as one line, a line break drawn as the space it stands for. */
UI_FN void DevWrapCopy(char *line, const char *s, int n)
{
    for (int i = 0; i < n; ++i)
        line[i] = s[i] == '\n' ? ' ' : s[i];
    line[n] = 0;
}

/* DevWrapFirst's lines, `pitch` apart, drawn when `draw`, else only counted; the first from firstX, the others from x.
 * A line always takes its first word: one wider than the line is drawn cut at the line's end and, when drawn, counted
 * in sDevKeyCut (once a word, a draw); a run that only counts lines (DevWrapLines) counts no cut. */
UI_FN int DevWrapRun(const char *s, int firstX, int x, int top, int firstWidth, int width, int maxLines, uint16_t ink,
                     uint16_t shadow, int pitch, int draw)
{
    char line[128];
    int lines = 0;

    while (lines < maxLines) {
        int fit = 0, end = 0, room = lines ? width : firstWidth, from = lines ? x : firstX;

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
        if (draw) {
            if (DevTextWidth(line) > room)
                ++sDevKeyCut;               /* a word alone wider than the line: cut at its end */
            DevTextInk(line, from, top + lines * pitch, from + room, ink, shadow);
        }
        ++lines;
        s += fit;
    }
    return lines;
}

/* Text wrapped at spaces and line breaks into at most maxLines lines from `top`, DEV_LINE_H apart, the first firstWidth
 * wide and the others `width`; a word wider than a line is cut at its end and counted in sDevKeyCut (DevWrapRun; lines
 * past maxLines are left out, not counted here). A line break (FireRed's descriptions keep the ones of its own window)
 * is a space here. Breaking only there keeps UTF-8 and the raw-code escapes whole. Returns the lines drawn. (Rework 1's
 * KitWrapFirst.) */
UI_FN int DevWrapFirst(const char *s, int x, int top, int firstWidth, int width, int maxLines, uint16_t ink,
                       uint16_t shadow)
{
    return DevWrapRun(s, x, x, top, firstWidth, width, maxLines, ink, shadow, DEV_LINE_H, 1);
}

/* DevWrapFirst with every line `width` wide. */
UI_FN int DevWrap(const char *s, int x, int top, int width, int maxLines, uint16_t ink, uint16_t shadow)
{
    return DevWrapFirst(s, x, top, width, width, maxLines, ink, shadow);
}

/* DevWrap with its lines `pitch` apart (a board's own line height; a word wider than a line counted). Returns the lines
 * drawn. */
UI_FN int DevWrapPitch(const char *s, int x, int top, int width, int maxLines, int pitch, uint16_t ink, uint16_t shadow)
{
    return DevWrapRun(s, x, x, top, width, width, maxLines, ink, shadow, pitch, 1);
}

/* The lines DevWrap would draw `s` in (at most maxLines; 0 for no text), drawing nothing: a text centred down a box. */
UI_FN int DevWrapLines(const char *s, int width, int maxLines)
{
    return DevWrapRun(s, 0, 0, 0, width, width, maxLines, 0, 0, DEV_LINE_H, 0);
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

/* The font a port word fits `room` in (`smallRoom` for the small font, which may start further out): 0 the normal one,
 * 1 the small one (the boards' label size). A word that fits neither is cut in the small one and counted in sDevKeyCut,
 * as a cut key label is. (HOME's FLY, the battle's grey quick ball.) */
UI_FN int DevFitFont(const char *s, int room, int smallRoom)
{
    if (DevTextWidth(s) <= room)
        return 0;
    if (DevSmallTextWidth(s) > smallRoom)
        ++sDevKeyCut;
    return 1;
}

/* A word in the font DevFitFont chose (`small`) from x to x + room (centred in it when `centre`), its capitals in the
 * middle of rows top..top + h - 1. */
UI_FN void DevFitWord(const char *s, int small, int x, int room, int top, int h, int centre, uint16_t ink, uint16_t shadow)
{
    int w = small ? DevSmallTextWidth(s) : DevTextWidth(s), at = centre && w < room ? x + (room - w) / 2 : x;

    if (small)
        DevSmallText(s, at, top + (h - 7) / 2, ink, x + room);
    else
        DevTextInk(s, at, top + (h - UI_TEXT_H) / 2, x + room, ink, shadow);
}

/* A key's text ink and its shadow in the look (the shadow is the ink where there is none). */
UI_FN uint16_t DevKeyInk(int look, int sunk)
{
    return look == DEV_LOOK_DARK ? DEV_DARK_INK : look == DEV_LOOK_FAINT ? DEV_FAINT_INK : sunk ? DEV_SUNK_INK : DEV_KEY_INK;
}

UI_FN uint16_t DevKeyInkShadow(int look, int sunk)
{
    return look == DEV_LOOK_KEY ? (sunk ? DEV_SUNK_INK_SH : DEV_WHITE) : DevKeyInk(look, sunk);
}

/* A key's label in `ink` over `shadow`, centred on its face; cut with "..." when wider than the key less 8 px (and then
 * counted in sDevKeyCut). */
UI_FN void DevKeyLabelInk(const CtrPokeTouchRect *r, int faceY, const char *s, uint16_t ink, uint16_t shadow)
{
    char cut[64];
    int w = DevTextWidth(s);

    if (w > r->w - 8) {
        DevCutText(s, r->w - 8, cut, sizeof(cut));
        s = cut;
        w = DevTextWidth(s);
        ++sDevKeyCut;
    }
    DevTextInk(s, r->x + (r->w - w) / 2, faceY + (r->h - UI_TEXT_H) / 2, r->x + r->w, ink, shadow);
}

/* A key's label in the look's ink and shadow (DevKeyLabelInk). */
UI_FN void DevKeyLabel(const CtrPokeTouchRect *r, int faceY, int look, int sunk, const char *s)
{
    DevKeyLabelInk(r, faceY, s, DevKeyInk(look, sunk), DevKeyInkShadow(look, sunk));
}

/* The x a word on sunk key r ends before to keep clear of its LED: 1 px short of the LED's glow (a pixel round its
 * DEV_LED_SIZE box). */
UI_FN int DevKeyLedClear(const CtrPokeTouchRect *r)
{
    return r->x + r->w - DEV_LED_IN - 2;
}

/* A key's label in `ink` over `shadow` (DevKeyLabelInk: centred, cut and counted when too wide), clear of the LED a
 * sunk key lights at its top right where the two share rows (keys 30 tall or less): moved left of it when it would run
 * under it; a word too long to fit left of it at all (CONFIRM in German, a Summary tab's word in Spanish) in the small
 * font, sunk or not, so it does not change size when chosen. (The Party's and the Summary's.) */
UI_FN void DevKeyWordInk(const CtrPokeTouchRect *r, int faceY, int sunk, const char *s, uint16_t ink, uint16_t shadow)
{
    int w = DevTextWidth(s), x = r->x + (r->w - w) / 2, top = faceY + (r->h - UI_TEXT_H) / 2;
    int ledX = DevKeyLedClear(r), ledY = faceY + DEV_LED_TOP;
    int room = ledX - (r->x + 4);

    if (top > ledY + 6 || top + UI_TEXT_H < ledY - 1) {             /* not in the LED's rows */
        DevKeyLabelInk(r, faceY, s, ink, shadow);
        return;
    }
    if (w > room && DevSmallTextWidth(s) <= room) {
        DevFitWord(s, 1, r->x + 4, room, faceY, r->h, 1, ink, shadow);
        return;
    }
    if (!sunk || w > r->w - 8 || x + w <= ledX) {
        DevKeyLabelInk(r, faceY, s, ink, shadow);
        return;
    }
    DevTextInk(s, ledX - w < r->x + 4 ? r->x + 4 : ledX - w, top, r->x + r->w, ink, shadow);
}

/* A key's label in its look's ink, clear of its LED (DevKeyWordInk). */
UI_FN void DevKeyWord(const CtrPokeTouchRect *r, int faceY, int look, int sunk, const char *s)
{
    DevKeyWordInk(r, faceY, sunk, s, DevKeyInk(look, sunk), DevKeyInkShadow(look, sunk));
}

/* A key's label in `ink` over `shadow` from `inset` px in (the boards' 12), when it fits before the LED's box; inset 0,
 * or a word too long for it, DevKeyWordInk's. */
UI_FN void DevKeyWordAtInk(const CtrPokeTouchRect *r, int faceY, int sunk, const char *s, int inset, uint16_t ink,
                           uint16_t shadow)
{
    int ledX = DevKeyLedClear(r);

    if (inset && DevTextWidth(s) <= ledX - (r->x + inset)) {
        DevTextInk(s, r->x + inset, faceY + (r->h - UI_TEXT_H) / 2, ledX, ink, shadow);
        return;
    }
    DevKeyWordInk(r, faceY, sunk, s, ink, shadow);
}

/* A key's label in its look's ink from `inset` px in (DevKeyWordAtInk). */
UI_FN void DevKeyWordAt(const CtrPokeTouchRect *r, int faceY, int look, int sunk, const char *s, int inset)
{
    DevKeyWordAtInk(r, faceY, sunk, s, inset, DevKeyInk(look, sunk), DevKeyInkShadow(look, sunk));
}

/* A word in the small font on key r, centred across it with its capitals at `top`, in the look's ink over its shadow;
 * one wider than the key less 8 px is cut at 4 px in on each side (and then counted in sDevKeyCut). */
UI_FN void DevKeySmallWord(const CtrPokeTouchRect *r, int top, int look, int sunk, const char *s)
{
    int w = DevSmallTextWidth(s), x = r->x + (r->w - w) / 2;

    if (w > r->w - 8) {
        x = r->x + 4;
        ++sDevKeyCut;
    }
    DevGlyphs(s, x, top - 4, r->x + r->w - 4, DevKeyInk(look, sunk), DevKeyInkShadow(look, sunk), 1);
}

/* DevSmallWrap's lines, drawn when `draw`, else only counted. A line always takes its first word: one wider than the
 * line (from where the line starts to `right`) is drawn cut at `right` and, when drawn, counted in sDevKeyCut (once a
 * word, a draw); a run that only counts lines counts no cut. */
UI_FN int DevSmallWrapRun(const char *s, int firstX, int x, int top, int right, int maxLines, int pitch, uint16_t ink,
                          int draw)
{
    char line[128];
    int lines = 0;

    while (lines < maxLines) {
        int fit = 0, end = 0, from = lines ? x : firstX;

        while (*s == ' ' || *s == '\n')
            ++s;
        if (!*s)
            break;
        for (;;) {
            while (s[end] && s[end] != ' ' && s[end] != '\n' && end < (int)sizeof(line) - 1)
                ++end;
            DevWrapCopy(line, s, end);
            if (fit > 0 && DevSmallTextWidth(line) > right - from)
                break;
            fit = end;
            if (!s[end] || end >= (int)sizeof(line) - 1)
                break;
            ++end;                          /* past the space, to the next word */
        }
        DevWrapCopy(line, s, fit);
        if (from != x && DevSmallTextWidth(line) > right - from) {
            ++lines;                        /* the first word does not fit after what is there: the next line */
            continue;
        }
        if (draw) {
            if (DevSmallTextWidth(line) > right - from)
                ++sDevKeyCut;               /* a word alone wider than the line: cut at `right` */
            DevSmallText(line, from, top + lines * pitch, ink, right);
        }
        ++lines;
        s += fit;
    }
    return lines;
}

/* Text in the small font wrapped at spaces and line breaks into at most maxLines lines `pitch` apart
 * (DEV_SMALL_LINE_TIGHT: the board's 9 px), ending by `right`: the first from firstX (after what is drawn before it on
 * that line; it starts on the next line when its first word does not fit there), the others from x; a word wider than
 * a whole line cut at `right` and counted in sDevKeyCut (DevSmallWrapRun). Returns the lines used. */
UI_FN int DevSmallWrap(const char *s, int firstX, int x, int top, int right, int maxLines, int pitch, uint16_t ink)
{
    return DevSmallWrapRun(s, firstX, x, top, right, maxLines, pitch, ink, 1);
}

/* DevSmallWrap (a word wider than a line counted), and a text that needs more than maxLines lines (the rest left out)
 * counted in sDevKeyCut, as a cut key label is: a footer's words must fit its paper. Returns the lines drawn. */
UI_FN int DevSmallFit(const char *s, int firstX, int x, int top, int right, int maxLines, int pitch, uint16_t ink)
{
    if (DevSmallWrapRun(s, firstX, x, top, right, maxLines + 1, pitch, ink, 0) > maxLines)
        ++sDevKeyCut;
    return DevSmallWrap(s, firstX, x, top, right, maxLines, pitch, ink);
}

/* Words read on a screen's paper, from x to `right`: the normal font in two lines at most, centred down the paper; a
 * text that needs more, the small font in three from `smallTop`, `pitch` apart (DevSmallFit: one that needs more
 * still is counted). */
UI_FN void DevReadText(const char *text, const CtrPokeTouchRect *paper, int x, int right, int smallTop, int pitch)
{
    int width = right - x, lines = DevWrapLines(text, width, 3);

    if (lines <= 2)
        DevWrap(text, x, paper->y + (paper->h - (8 + DEV_LINE_H * (lines - 1))) / 2, width, 2, DEV_INK, DEV_INK_SH);
    else
        DevSmallFit(text, x, x, smallTop, right, 3, pitch, DEV_INK);
}

/* A footer's title line on its paper, from x: `s` in the normal font, capitals from paper->y + 2, DEV_TITLE_INK over the
 * paper's shadow; cut with "..." at paper->x + paper->w - 6 (and then counted in sDevKeyCut). */
UI_FN void DevNoteTitle(const CtrPokeTouchRect *paper, int x, const char *s)
{
    char cut[64];
    int right = paper->x + paper->w - 6;

    if (DevTextWidth(s) > right - x) {
        DevCutText(s, right - x, cut, sizeof(cut));
        s = cut;
        ++sDevKeyCut;
    }
    DevTextInk(s, x, paper->y + 2, right, DEV_TITLE_INK, DEV_INK_SH);
}

/* A title line on its own screen (DevScreen(screen)): `left` in the normal font, DEV_INK over its shadow, from 6 px into
 * the paper, its capitals centred down it; `right` (none when empty) small in the secondary ink ending 8 px short of the
 * paper's right, its capitals a row lower. `left` is cut with "..." 8 px before `right`, or 6 px short of the paper's
 * right with no `right`, and not counted (FireRed's text or a phrase); a `right` wider than the paper is cut there and
 * counted in sDevKeyCut. */
UI_FN void DevTitleLine(const CtrPokeTouchRect *screen, const char *left, const char *right)
{
    CtrPokeTouchRect paper = DevScreen(screen);
    char cut[64];
    int x = paper.x + 6, top = paper.y + (paper.h - 8) / 2, end = paper.x + paper.w - 8, limit = paper.x + paper.w - 6;
    int from = end;

    if (right && right[0]) {
        from = end - DevSmallTextWidth(right);
        if (from < x) {
            from = x;
            ++sDevKeyCut;
        }
        limit = from - 8;
    }
    if (DevTextWidth(left) > limit - x) {
        DevCutText(left, limit - x, cut, sizeof(cut));
        left = cut;
    }
    DevTextInk(left, x, top, limit, DEV_INK, DEV_INK_SH);
    if (right && right[0])
        DevSmallText(right, from, top + 1, DEV_INK2, end);
}

/* ---- Marks, bars, chips, icons ------------------------------------------------------------------------------------- */

/* A ring s pixels across, its box's top-left at (x, y): the pixels whose centre is within the circle's last pixel. */
UI_FN void DevRing(int x, int y, int s, uint16_t c)
{
    for (int j = 0; j < s; ++j)
        for (int i = 0; i < s; ++i) {
            int dx = 2 * i + 1 - s, dy = 2 * j + 1 - s, d = dx * dx + dy * dy;

            if (d <= s * s && d > (s - 2) * (s - 2))
                Px(x + i, y + j, c);
        }
}

/* A disc s across at (x, y) in c (DevDisc), told as a DISC: a page's dot, a mark on the paper. */
UI_FN void DevSpot(int x, int y, int s, uint16_t c)
{
    DEV_TRACE(DEV_TRACE_DISC, x, y, s, s, c, DEV_WHITE, NULL);
    DevDisc(x, y, s, c);
}

/* A hole in the paper s across at (x, y) (a badge not earned): a DEV_PAPER_LINE disc, then the pixels of the same disc
 * 2 px lower that lie inside it in DEV_PAPER_HOLE, so a 2 px rim stays at its top (CSS's inset 0 2px 0). One DISC. */
UI_FN void DevPaperHole(int x, int y, int s)
{
    DEV_TRACE(DEV_TRACE_DISC, x, y, s, s, DEV_PAPER_HOLE, DEV_PAPER_LINE, NULL);
    DevDisc(x, y, s, DEV_PAPER_LINE);
    for (int j = 0; j < s; ++j)
        for (int i = 0; i < s; ++i)
            if (DevInDisc(s, i, j) && DevInDisc(s, i, j - 2))
                Px(x + i, y + j, DEV_PAPER_HOLE);
}

/* Line segments stroked as an SVG path draws them (with round ends here) into an alpha mask, size x size, of 256: the
 * share of each pixel's 4x4 samples inside the stroke. `seg` holds n segments (x0, y0, x1, y1) in a view box `view`
 * units square, the stroke `width10` tenths of a view unit wide. Integer only (positions in 1/(8 view) px); a caller
 * keeps the mask and blends it with DevAlphaArt. */
UI_FN void DevStrokeMask(const int8_t (*seg)[4], int n, int view, int size, int width10, uint16_t *alpha)
{
    int64_t reach = 2 * (int64_t)width10 * size;        /* 5 times the half width, in those units */

    for (int py = 0; py < size; ++py)
        for (int px = 0; px < size; ++px) {
            int in = 0;

            for (int s = 0; s < 16; ++s) {
                int64_t sx = (int64_t)(8 * px + 2 * (s & 3) + 1) * view, sy = (int64_t)(8 * py + 2 * (s >> 2) + 1) * view;

                for (int i = 0; i < n; ++i) {
                    int64_t ax = (int64_t)seg[i][0] * size * 8, ay = (int64_t)seg[i][1] * size * 8;
                    int64_t dx = (int64_t)seg[i][2] * size * 8 - ax, dy = (int64_t)seg[i][3] * size * 8 - ay;
                    int64_t qx = sx - ax, qy = sy - ay, dot = qx * dx + qy * dy, len2 = dx * dx + dy * dy, d2;

                    if (dot <= 0 || !len2)
                        d2 = qx * qx + qy * qy;
                    else if (dot >= len2)
                        d2 = (qx - dx) * (qx - dx) + (qy - dy) * (qy - dy);
                    else
                        d2 = (qx * dy - qy * dx) * (qx * dy - qy * dx) / len2;
                    if (25 * d2 <= reach * reach) {
                        ++in;
                        break;
                    }
                }
            }
            alpha[py * size + px] = (uint16_t)(in * 16);
        }
}

/* An alpha mask (DevStrokeMask's) in `ink` over what is there, its top-left at (x, y). */
UI_FN void DevAlphaArt(const uint16_t *alpha, int size, int x, int y, uint16_t ink)
{
    for (int j = 0; j < size; ++j)
        for (int i = 0; i < size; ++i)
            if (alpha[j * size + i])
                Px(x + i, y + j, DevBlend(DevGet(x + i, y + j), ink, alpha[j * size + i]));
}

/* A triangle pointing `dir` (0 up, 1 down, 2 left, 3 right), 9 px across, centred on (cx, cy). (Rework 1's KitTriangle.) */
UI_FN void DevTriangle(int cx, int cy, int dir, uint16_t c)
{
    if (dir < 2)
        DEV_TRACE(DEV_TRACE_TRI, cx - 4, cy - 2, 9, 5, c, dir, NULL);
    else
        DEV_TRACE(DEV_TRACE_TRI, cx - 2, cy - 4, 5, 9, c, dir, NULL);
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

/* Two keys with a triangle each (▲ ▼, or ◀ ▶ `across`): grey (DEV_ARROW_OFF) where there is nowhere to go (`on0`,
 * `on1`), sunk under a held touch (`held0`, `held1`) only where there is. */
UI_FN void DevArrowKeys(const CtrPokeTouchRect *r0, const CtrPokeTouchRect *r1, int on0, int on1, int held0, int held1,
                        int across)
{
    for (int k = 0; k < 2; ++k) {
        const CtrPokeTouchRect *r = k ? r1 : r0;
        int on = k ? on1 : on0, sunk = on && (k ? held1 : held0), faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);

        DevTriangle(r->x + r->w / 2, faceY + r->h / 2, across ? 2 + k : k,
                    on ? DevKeyInk(DEV_LOOK_KEY, sunk) : DEV_ARROW_OFF);
    }
}

/* The HP colour of FireRed's own battle HUD, so the two screens agree: a full bar is green; otherwise the hp is measured
 * on the HUD's 48-pixel bar (rounded down, but at least a pixel while any hp is left), and more than 24 pixels is green,
 * more than 9 yellow, 9 or less red. */
UI_FN uint16_t DevHpColour(unsigned hp, unsigned max)
{
    unsigned pixels;

    if (hp >= max)
        return DEV_HP_GREEN;
    pixels = hp * 48u / max;
    if (!pixels && hp)
        pixels = 1;
    return pixels > 24 ? DEV_HP_GREEN : pixels > 9 ? DEV_HP_YELLOW : DEV_HP_RED;
}

/* A bar: a 1 px DEV_HP_FRAME frame, the DEV_HP_TRACK track and `fill` of its columns (0 ... w - 2) in `colour` from the
 * left. Told as a BAR. (An HP bar's; a progress bar's.) */
UI_FN void DevBar(int x, int y, int w, int h, int fill, uint16_t colour)
{
    int inner = w - 2;

    if (w < 2 || h < 2)
        return;
    if (fill < 0)
        fill = 0;
    if (fill > inner)
        fill = inner;
    DEV_TRACE(DEV_TRACE_BAR, x, y, w, h, colour, fill, NULL);
    Fill(x, y, w, h, DEV_HP_FRAME);
    Fill(x + 1, y + 1, inner, h - 2, DEV_HP_TRACK);
    if (fill > 0)
        Fill(x + 1, y + 1, fill, h - 2, colour);
}

/* An HP bar (DevBar): hp of max filled (at least a pixel while hp is not 0), in DevHpColour. */
UI_FN void DevHpBar(int x, int y, int w, int h, unsigned hp, unsigned max)
{
    int inner = w - 2, fill = max ? (int)((unsigned long)hp * (unsigned)(inner > 0 ? inner : 0) / max) : 0;

    if (hp && fill < 1)
        fill = 1;
    DevBar(x, y, w, h, fill, DevHpColour(hp, max));
}

/* The dark shadow for a chip's white text when the chip's colour comes from data (a type's): the colour darkened. Within
 * one RGB565 step of every pair the boards give (FIRE, SPECIAL, SUPER, EFFECTIVE, NORMAL, FIGHT, NOT VERY). */
UI_FN uint16_t DevChipShadow(uint16_t bg)
{
    return DevBlend(bg, 0x0000, DEV_A_CHIP_SH);
}

/* A chip at (x, y), h tall: `text` in the small font, white, with `shadow` under it (DEV_WHITE: none). w 0: as wide as
 * the text and 4 px on each side; else w wide with the text centred (BMoves' type chips are 40), and a text wider than
 * that is clipped and counted in sDevKeyCut, as a cut key label is. */
UI_FN void DevChip(int x, int y, int w, int h, const char *text, uint16_t bg, uint16_t shadow)
{
    int tw = DevSmallTextWidth(text), tx;

    if (w <= 0)
        w = tw + 8;
    else if (tw > w)
        ++sDevKeyCut;
    tx = x + (w - tw) / 2;
    DEV_TRACE(DEV_TRACE_CHIP, x, y, w, h, bg, shadow, text);
    Fill(x, y, w, h, bg);
    /* The capitals are 7 rows, 8 with their shadow: centred, any odd row below. */
    DevGlyphPixels(text, tx < x ? x : tx, y + (h - (shadow == DEV_WHITE ? 7 : 8)) / 2 - 4, x + w, DEV_WHITE, shadow, 1);
}

/* A label chip ("HP", "EXP"): the small font with 1 px around it, 9 px tall. Returns its width. */
UI_FN int DevLabelChip(int x, int y, const char *text, uint16_t bg, uint16_t ink)
{
    int w = DevSmallTextWidth(text) + 2;

    DevRect(x, y, w, 9, bg);
    DevSmallText(text, x + 1, y + 1, ink, x + w);
    return w;
}

/* A type's chip: its name in capitals in the small font, 40 wide (the board's type chip) or the word's width and 4 px a
 * side. Its width; and the chip at (x, y), h tall, in the type's colour (FireRed's), returning its width. */
UI_FN int DevTypeChipWidth(const char *typeName)
{
    char word[16];
    int w;

    DevUpper(word, typeName, sizeof(word));
    w = DevSmallTextWidth(word) + 8;
    return w < 40 ? 40 : w;
}

UI_FN int DevTypeChip(const char *typeName, int type, int x, int y, int h)
{
    uint16_t bg = UiTypeColor(type);
    char word[16];
    int w = DevTypeChipWidth(typeName);

    DevUpper(word, typeName, sizeof(word));
    DevChip(x, y, w, h, word, bg, DevChipShadow(bg));
    return w;
}

/* A move's PP in the colours of FireRed's own move window, so the two screens agree: full PP, `normal`. With a maximum of 7
 * or less it goes by what is left: more than 2 (more than 1 when the maximum is 2 or less) `normal`, 2 the half colour,
 * 1 the quarter colour, none the empty one. Above 7: none left, empty; up to a quarter of the maximum (rounded down),
 * quarter; more than half of it (rounded down), `normal`; in between, half. */
UI_FN uint16_t DevPpInk(unsigned pp, unsigned max, uint16_t normal)
{
    if (pp == max)
        return normal;
    if (max <= 7) {
        if (pp > (max <= 2 ? 1u : 2u))
            return normal;
        return pp == 0 ? DEV_LOW_INK : pp == 1 ? DEV_PP_QUARTER : DEV_PP_HALF;
    }
    if (pp == 0)
        return DEV_LOW_INK;
    if (pp <= max / 4)
        return DEV_PP_QUARTER;
    return pp > max / 2 ? normal : DEV_PP_HALF;
}

/* BACK's word, on every view's BACK key. */
UI_FN const char *DevBackWord(void)
{
    return CTR_TR("BACK", "ATR\xC3\x81S", "ZUR\xC3\x9C" "CK");
}

/* CLOSE's word, on a key that closes an app (when the game names none of its own). */
UI_FN const char *DevCloseWord(void)
{
    return CTR_TR("CLOSE", "CERRAR", "SCHLIE\xC3\x9F" "EN");
}

/* CANCEL's word, on a key or row that cancels an action (German ABBRECHEN: ZURÜCK is BACK's). */
UI_FN const char *DevCancelWord(void)
{
    return CTR_TR("CANCEL", "CANCELAR", "ABBRECHEN");
}

/* A move's category on its chip: PHYSICAL (`physical`) or SPECIAL, each within 43 px with its chip's padding at 6 px
 * (the Summary's and the battle's). */
UI_FN const char *DevCategoryWord(int physical)
{
    return physical ? CTR_TR("PHYSICAL", "F\xC3\x8DSICO", "PHYS.") : CTR_TR("SPECIAL", "ESPEC.", "SPEZIAL");
}

/* "POW n  ACC n" into out (size bytes): a move's power and accuracy as FireRed gives them, "-" for none. */
UI_FN void DevPowAcc(char *out, size_t size, const char *power, const char *accuracy)
{
    snprintf(out, size, "%s %s  %s %s", CTR_TR("POW", "POT", "ST\xC3\x84"), power && power[0] ? power : "-",
             CTR_TR("ACC", "PRE", "GEN"), accuracy && accuracy[0] ? accuracy : "-");
}

/* The key shown sunk: the one under a held touch, else the game's cursor. */
UI_FN int DevChosen(int id, int cursor, int pressed)
{
    return pressed >= 0 ? id == pressed : id == cursor;
}

/* A touch at (x, y) is on key r: its face or the drop under it (DevKeyArea). */
UI_FN int DevOnKey(const CtrPokeTouchRect *r, int x, int y)
{
    CtrPokeTouchRect a = DevKeyArea(r);

    return UiIn(&a, x, y);
}

/* A label and its value across row r: the label small in the secondary ink at its left, the value in the paper's ink
 * ending at its right, cut with "..." to the room the label leaves (a label that does not fit, or a value cut, is
 * counted in sDevKeyCut). Both centred down the row. (The Pokedex's rows.) */
UI_FN void DevLabelValue(const CtrPokeTouchRect *r, const char *label, const char *value)
{
    char cut[64];
    int labelW = DevSmallTextWidth(label), right = r->x + r->w, room = r->w - labelW - 6;

    if (labelW > r->w)
        ++sDevKeyCut;
    DevSmallText(label, r->x, r->y + (r->h - 7) / 2, DEV_INK2, right);
    if (DevTextWidth(value) > room) {
        DevCutText(value, room, cut, sizeof(cut));
        value = cut;
        ++sDevKeyCut;
    }
    DevText(value, right - DevTextWidth(value), r->y + (r->h - UI_TEXT_H) / 2, right);
}

/* "LABEL value" across a column from x to `right` at box top y: the label small in the secondary ink (capitals y + 4),
 * the value whole in the paper's ink ending at `right` (capitals y + 3), at least DEV_COLUMN_GAP px of paper between
 * them. A row that does not fit (the label, the gap and the value wider than the column; the value alone, with no
 * label) is counted in sDevKeyCut, the label cut DEV_COLUMN_GAP px short of the value. */
#define DEV_COLUMN_GAP 6
UI_FN void DevColumnRow(int y, const char *label, const char *value, int x, int right)
{
    int from = right - DevTextWidth(value);

    if (DevSmallTextWidth(label) + (label[0] ? DEV_COLUMN_GAP : 0) > from - x)
        ++sDevKeyCut;
    DevSmallText(label, x, y + 4, DEV_INK2, from - DEV_COLUMN_GAP);
    DevText(value, from, y + 3, right);
}

/* A field view's BACK (or CLOSE) in key r with `word`, while the view shows it (backVisible): a dark key, sunk under a
 * held touch, grey while it cannot be used. */
UI_FN void DevFieldBack(const CtrPokeTouchView *v, const CtrPokeTouchRect *r, const char *word)
{
    int sunk = v->backEnabled && v->backPressed, faceY;

    if (!v->backVisible)
        return;
    faceY = DevKey(r, DEV_LOOK_DARK, sunk, 6);
    DevKeyWord(r, faceY, DEV_LOOK_DARK, sunk, word);
    if (!v->backEnabled)
        DevDim(r);
}

/* DevFieldBack draws the same for `a` and `b`. */
UI_FN int DevFieldBackSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return a->backVisible == b->backVisible
        && (!b->backVisible || (a->backEnabled == b->backEnabled
                                && (a->backEnabled && a->backPressed) == (b->backEnabled && b->backPressed)));
}

/* A gender mark, 7x9 (v0.6.0's shapes) with the paper's text shadow 1 px right and down, so it paints 8x10: 0 male,
 * 1 female, 2 none (CtrBottomParty_Gender). Told as a sign: its character, ♂ or ♀. */
UI_FN void DevGender(int gender, int x, int y)
{
    static const uint8_t kMale[9] = {0x0F, 0x03, 0x05, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00};
    static const uint8_t kFemale[9] = {0x1C, 0x22, 0x22, 0x22, 0x1C, 0x08, 0x3E, 0x08, 0x08};
    const uint8_t *g = gender == 0 ? kMale : kFemale;

    if (gender != 0 && gender != 1)
        return;
    DEV_TRACE(DEV_TRACE_TEXT, x, y, 7, DEV_INK_SH, gender == 0 ? DEV_MALE : DEV_FEMALE, 2,
              gender == 0 ? "\xE2\x99\x82" : "\xE2\x99\x80");
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

/* A Pokemon's icon from its 32x32 palette indices (`pixels`, rows of 32, 0 clear) and palette: 32x32, or half (16x16: of
 * each 2x2 block its first pixel that is not clear, in reading order); faint, every colour toward DEV_FAINT. */
UI_FN void DevIconPixels(const uint8_t *pixels, const uint16_t *pal, int x, int y, int half, int faint)
{
    int size = half ? 16 : 32, step = half ? 2 : 1;

    for (int py = 0; py < size; ++py)
        for (int px = 0; px < size; ++px) {
            const uint8_t *src = pixels + py * step * 32 + px * step;
            uint8_t v = src[0];

            if (half && !v)
                v = src[1] ? src[1] : src[32] ? src[32] : src[33];
            if (v)
                Px(x + px, y + py, faint ? DevBlend(DEV_FAINT, pal[v], DEV_A_FAINT_ART) : pal[v]);
        }
}

/* A party slot's icon, FireRed's frame 0 (BPartyIcon), as DevIconPixels draws it. */
UI_FN void DevMonIcon(int slot, unsigned icon, int x, int y, int half, int faint)
{
    if (slot < 0 || slot >= 6 || !BPartyIcon(slot, icon))
        return;
    DevIconPixels(sBPIcon[slot].pixels, sBPIcon[slot].pal, x, y, half, faint);
}

/* An item's icon (FireRed's 24x24) at (x, y), or at half size (12x12: of each 2x2 block its first pixel that is not
 * clear, in reading order). */
UI_FN void DevItemIcon(unsigned item, int x, int y, int half)
{
    int slot = BItemIcon(item), size = half ? 12 : 24, step = half ? 2 : 1;

    if (slot < 0)
        return;
    for (int py = 0; py < size; ++py)
        for (int px = 0; px < size; ++px) {
            const uint8_t *src = sBIcon[slot].pixels + py * step * 24 + px * step;
            uint8_t c = src[0];

            if (half && !c)
                c = src[1] ? src[1] : src[24] ? src[24] : src[25];
            if (c)
                Px(x + px, y + py, sBIcon[slot].pal[c]);
        }
}
