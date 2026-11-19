/* FireRed's picture in an embedded screen, and the idle screen, as the Definitiva draws them (def_frame, def_idle),
 * included by 3ds_poketouch.c after the field's views and before their dispatcher.
 *
 * The frame is the field dispatcher's last row (3ds_device_field_impl.h): it draws any field view no other row draws
 * (DevFrameShown: not HOME, not the Bag). FireRed's own screen in the viewport (the PC's menus, a link card, FireRed's
 * own Summary, the Pokedex's AREA page, the Fame Checker...) is drawn by the GPU on the paper of an embedded screen
 * centred across the top: the paper is black under it, at PT_VIEW_X/Y (the one place of the picture, where a touch is a
 * GBA point). With no picture the screen is empty (its paper), never the ember look. Under it the grid's footer: the
 * note (kDevGridNote) with the bar's title in the normal font (capitals 198) and its two hints in the small font and the
 * secondary ink under it (capitals 209 and 218), one a line (the title alone centred down the paper); BACK in the exit
 * (kDevGridExit) while the view shows it, a dark key (sunk under a held touch, grey while it cannot be used), else the
 * wide note across (kDevGridNoteWide). The shell has its four screws, as every view's. Every rectangle is
 * 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): the bar (v0.6.0 title-cased it) through DevUpper.
 *
 * Redraw: the screen whole when the picture's presence, the bar or BACK as drawn differ (DevFrameSame), and nothing
 * else.
 *
 * The idle screen (CtrPokeTouch_DrawIdle): the shell in the player's colour with its four screws and an embedded screen
 * asleep, its paper the bezel's dark (owner decision 3: the logo was made over the dark ember backdrop), the project's
 * logo blended over it once nothing has been shown for a while. */

static int DevFrameShown(const CtrPokeTouchView *v)
{
    return !DevHomeShown(v) && !DevBagShown(v);
}

/* The note under the picture: the title (bar[0]) in the normal font, its capitals 2 px into the paper (198: an accent
 * stays off the bezel), and the hints (bar[1], bar[2]) in the small font and the secondary ink under it, capitals 13 and
 * 22 px in (209 and 218: the footer's last line, its descenders on the paper's last row), one a line while each fits,
 * else as one text wrapped into the two lines (DevSmallFit: what they cannot hold dropped and counted in sDevKeyCut;
 * the port's widest bars fit in the three languages); without hints the title alone, centred down the paper. A title
 * wider than its line is cut 6 px short of the paper's end and counted. */
static void DevFrameNote(const CtrPokeTouchView *v)
{
    CtrPokeTouchRect paper = DevScreen(v->backVisible ? &kDevGridNote : &kDevGridNoteWide);
    char title[PT_BAR_LEN], hint[2][PT_BAR_LEN], text[2 * PT_BAR_LEN + 1];
    int x = paper.x + 6, right = paper.x + paper.w - 6, top = paper.y + 13, any = v->bar[1][0] || v->bar[2][0];

    DevUpper(title, v->bar[0], sizeof(title));
    if (DevTextWidth(title) > right - x)
        ++sDevKeyCut;
    DevText(title, x, any ? paper.y + 2 : paper.y + (paper.h - UI_TEXT_H) / 2, right);
    if (!any)
        return;
    DevUpper(hint[0], v->bar[1], sizeof(hint[0]));
    DevUpper(hint[1], v->bar[2], sizeof(hint[1]));
    if (DevSmallTextWidth(hint[0]) <= right - x && DevSmallTextWidth(hint[1]) <= right - x) {
        for (int i = 0, line = 0; i < 2; ++i)
            if (hint[i][0])
                DevSmallText(hint[i], x, top + DEV_SMALL_LINE_TIGHT * line++, DEV_INK2, right);
        return;
    }
    snprintf(text, sizeof(text), "%s %s", hint[0], hint[1]);
    DevSmallFit(text, x, x, top, right, 2, DEV_SMALL_LINE_TIGHT, DEV_INK2);
}

/* The frame whole (after DevBegin: the dispatcher's). */
static void DevFrameDraw(const CtrPokeTouchView *v)
{
    DevShell();
    DevScreen(&kDevViewScreen);
    if (v->viewport) {
        DevArt(DEV_ART_GBA, PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H);
        Fill(PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H, 0x0000);       /* covered by the GPU's picture */
    }
    DevFrameNote(v);
    DevFieldBack(v, &kDevGridExit, DevBackWord());
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the frame draws differs between `a` and `b` (the shell is the dispatcher's): the picture shown or not, the
 * bar, BACK. */
static int DevFrameSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    for (int i = 0; i < PT_BAR_LINES; ++i)
        if (strcmp(a->bar[i], b->bar[i]))
            return 0;
    return !a->viewport == !b->viewport && DevFieldBackSame(a, b);
}

/* BACK (kDevGridExit) while the view shows it. */
static const CtrPokeTouchRect *DevFrameBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevGridExit : NULL;
}

/* ---- The idle screen --------------------------------------------------------------------------------------------- */

#include "idle_logo.h"   /* the project's logo (tools/make_idle_logo.py): its colours, their alpha, its pixels */

void CtrPokeTouch_DrawIdle(uint16_t *canvas, int logo, uint8_t shell)
{
    CtrPokeTouchRect paper;
    int x0, y0;

    sHomeShown = 0;                     /* HOME's region key and FLY are not on the screen */
    DevBegin(canvas, shell);
    DevShell();
    paper = DevScreen(&kDevIdleScreen);
    DevRect(paper.x, paper.y, paper.w, paper.h, DEV_BEZEL);         /* the screen asleep */
    if (!logo)
        return;
    x0 = paper.x + (paper.w - IDLE_LOGO_W) / 2;
    y0 = paper.y + (paper.h - IDLE_LOGO_H_PX) / 2;
    DevArt(DEV_ART_LOGO, x0, y0, IDLE_LOGO_W, IDLE_LOGO_H_PX);
    for (int y = 0; y < IDLE_LOGO_H_PX; ++y)
        for (int x = 0; x < IDLE_LOGO_W; ++x) {
            int k = kIdleLogo[y * IDLE_LOGO_W + x], a = kIdleLogoAlpha[k];
            unsigned c = kIdleLogoColour[k], b;

            if (a == 0)
                continue;
            if (a < 15) {                          /* a/15 of the logo over the paper, channel by channel */
                b = DevGet(x0 + x, y0 + y);
                c = ((((c >> 11) * a + (b >> 11) * (15 - a)) / 15) << 11)
                  | (((((c >> 5) & 63) * a + ((b >> 5) & 63) * (15 - a)) / 15) << 5)
                  | (((c & 31) * a + (b & 31) * (15 - a)) / 15);
            }
            Px(x0 + x, y0 + y, (uint16_t)c);
        }
}
