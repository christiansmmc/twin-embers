/* FireRed's own picture as a page (phase 2 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md): the
 * Fame Checker, Teachy TV, mail, Easy Chat, the Hall of Fame, the Pokedex's AREA, a link's Trainer Card, the PC's own
 * screens. The GPU draws the GBA picture 1:1 at (PT_VIEW_X, PT_VIEW_Y) over this page (3ds_video.c); the page puts a
 * window with the player's frame round it, the band with what the screen is (the bar's lines) and BACK, the screen's B.
 * A view with no page of its own and nothing in the picture's place gets the same frame, empty. Included by
 * 3ds_poketouch.c after the field pages' pieces. */

static void ViewPageDraw(const CtrPokeTouchView *v)
{
    char body[2 * PT_BAR_LEN + 2];

    snprintf(body, sizeof(body), "%s%s%s", v->bar[1], v->bar[1][0] && v->bar[2][0] ? " " : "", v->bar[2]);
    Fill(0, 0, W, H, BPageBg());
    FPageWindow(&kViewFrame, v->frameType);
    if (v->viewport)
        Fill(PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H, 0);   /* under the GPU's picture */
    FPageBand(v->backVisible ? &kViewNote : &kViewNoteWide, v->bar[0], body, 0);
    if (v->backVisible)
        FPageBack(v, &kViewBack);
}
