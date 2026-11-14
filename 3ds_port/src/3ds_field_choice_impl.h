/* A choice of the field as a page (3ds_poketouch.h CtrPokeTouchChoice; phase 2), included at the end of 3ds_poketouch.c
 * after the field pages' pieces. A remote for FireRed's menu (a script's multichoice, YES / NO or grid, the PC's menus):
 * its rows in FireRed's menu window at the bottom right, as the Party's, as wide as its longest row needs (a grid: a
 * window across the page over the band), FireRed's arrow on the cursor or the row held, the message box's question in
 * the band beside it. A tap off the window is the menu's B (as off the Bag's menu): CANCEL, NO, or nothing where the
 * menu takes no B. Every word is FireRed's (CtrFieldChoice_*). */

/* The width the longest row needs in the window: the arrow, the label, the frame. */
static int FChWidth(const CtrPokeTouchChoice *c)
{
    int w = 0;

    for (int i = 0; i < c->count && i < PT_CHOICE_MAX; ++i) {
        int t = UiTextWidth(c->label[i], 1);

        if (t > w)
            w = t;
    }
    return w + KIT_ARROW_W + 16;
}

int CtrPokeTouch_ChoiceHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchChoice *c = &v->choice;
    int n = c->count < PT_CHOICE_MAX ? c->count : PT_CHOICE_MAX, width = FChWidth(c);
    CtrPokeTouchRect w;

    if (!c->on || n <= 0)
        return -1;
    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = ChoiceRow(n, c->cols, width, i);

        if (UiIn(&r, x, y))
            return PT_CHOICE_HIT_ROW + i;
    }
    w = ChoiceWindow(n, c->cols, width);
    return UiIn(&w, x, y) || x < 0 || y < 0 || x >= W || y >= H ? -1 : PT_CHOICE_HIT_BACK;
}

static void FChoicePageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchChoice *c = &v->choice;
    int n = c->count < PT_CHOICE_MAX ? c->count : PT_CHOICE_MAX, width = FChWidth(c);
    int held = c->pressed >= PT_CHOICE_HIT_ROW && c->pressed < PT_CHOICE_HIT_ROW + n ? c->pressed - PT_CHOICE_HIT_ROW : -1;
    CtrPokeTouchRect band = ChoiceNote(n > 0 ? n : 1, c->cols, width);

    Fill(0, 0, W, H, BPageBg());
    FPageBand(&band, NULL, c->prompt, 0);
    if (n <= 0)
        return;
    {
        CtrPokeTouchRect w = ChoiceWindow(n, c->cols, width);

        FPageWindow(&w, v->frameType);
    }
    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = ChoiceRow(n, c->cols, width, i);

        FPageRow(&r, c->label[i], held >= 0 ? held == i : c->cursor == i, held == i, 1);
    }
}
