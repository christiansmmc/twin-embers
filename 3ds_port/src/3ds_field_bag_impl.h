/* The ember pieces the field's views still in the Llama look share (llama2_mockup.html), included at the end of
 * 3ds_poketouch.c before them: the Pokedex, the move to forget, the relearner, the choice, the naming screen, the PC box
 * and Easy Chat use these until plan 3 draws them as the red device. The field's Bag itself is the red device's
 * (3ds_device_bag_impl.h). */
#include "3ds_ui_strings.h"

static const CtrPokeTouchRect sFBArea = {PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H};
#define FB_DEPTH 2
#define FB_IDLE_ARROW UI_RGB(200, 192, 176)             /* a scroll arrow with nothing to scroll to */

/* A window over a dimmed screen (a menu, a question, a message): FireRed's text window. */
static void FBWindow(const CtrPokeTouchRect *r)
{
    LlWindowPlain(r->x, r->y, r->w, r->h);
}

/* A card of the zone `r` (FB_DEPTH of it is the lip) with `label` centred on it; `chosen` adds the select ring and the
 * cursor. */
static void FBLlButton(const CtrPokeTouchRect *r, const EmPal *p, const char *label, int pressed, int chosen)
{
    int face = r->h - FB_DEPTH, fy;

    if (chosen)
        LlSelectRing(r->x, r->y, r->w, r->h);
    fy = EmButton(r->x, r->y, r->w, r->h, p, FB_DEPTH, pressed);
    if (chosen)
        LlCursor(r->x + 9, fy + (face - 10) / 2, EM_WHITE);
    if (UiTextWidth(label, 1) > r->w - 8 && strchr(label, ' ')) {   /* a label that would touch both edges: two lines (CLOSE / BOX) */
        char first[40];
        const char *rest = strchr(label, ' ');
        int n = (int)(rest - label), top = fy + (face - (2 * UI_TEXT_H + 2)) / 2;

        if (n > (int)sizeof(first) - 1)
            n = (int)sizeof(first) - 1;
        memcpy(first, label, (size_t)n);
        first[n] = 0;
        EmLabel(first, r->x, r->w, top, UI_TEXT_H, p);
        EmLabel(rest + 1, r->x, r->w, top + UI_TEXT_H + 2, UI_TEXT_H, p);
        return;
    }
    EmLabel(label, r->x, r->w, fy, face, p);
}
