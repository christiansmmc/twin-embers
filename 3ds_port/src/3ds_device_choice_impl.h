/* A choice of the field on the grid (boards def_choice_yesno, def_choice_list, def_choice_grid), included by
 * 3ds_poketouch.c after the relearner: a row of the field dispatcher (3ds_device_field_impl.h), shown while
 * view->choice.on and there is no GBA picture (DevChoiceShown). A remote for FireRed's menu (3ds_poketouch.h
 * CtrPokeTouchChoice: a script's multichoice, YES/NO or grid, the PC's menus): its rows as keys above the footer
 * (DevChoiceKey: YES / NO two tall keys, one column across the screen up to four, two beyond going down first with the
 * left one always four, FireRed's grid across), FireRed's cursor sunk, or the key under a held touch; the message box's
 * question on the grid's note (two lines of the normal font, or three of the small one); BACK (its B) in the exit
 * corner. A key is asked of the menu and taken with its A (3ds_bottom.c ProcessChoiceTouch); the upper screen's hide and
 * hold are 3ds_bottom.c's, unchanged. Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): FireRed's labels (which v0.6.0 title-cased) through DevUpper, in the normal font, from 12 px in on
 * a list's key across the screen and centred on the others; its question as FireRed writes it; BACK's word the port's.
 *
 * Redraw: the screen whole when the choice as drawn differs (DevChoiceSame), and nothing else. */

static int DevChoiceShown(const CtrPokeTouchView *v)
{
    return v->choice.on && !v->viewport;
}

/* The keys drawn: the count, PT_CHOICE_MAX at most. */
static int DevChoiceCount(const CtrPokeTouchChoice *c)
{
    return c->count < PT_CHOICE_MAX ? c->count : PT_CHOICE_MAX;
}

int CtrPokeTouch_ChoiceHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchChoice *c = &v->choice;

    if (!DevChoiceShown(v))
        return -1;
    for (int i = 0; i < DevChoiceCount(c); ++i) {
        CtrPokeTouchRect r = DevChoiceKey(c->count, c->cols, i);

        if (DevOnKey(&r, x, y))
            return PT_CHOICE_HIT_ROW + i;
    }
    return -1;
}

/* Where key r's word starts: 12 px in on a list's key across the screen (def_choice_list); 0, centred, on YES / NO's
 * tall keys (def_choice_yesno) and on a key of two or three columns (def_choice_grid). */
static int DevChoiceInset(const CtrPokeTouchChoice *c, const CtrPokeTouchRect *r)
{
    return r->w == 304 && !(c->count == 2 && c->cols < 2) ? 12 : 0;
}

/* The choice whole (after DevBegin: the dispatcher's). */
static void DevChoiceDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchChoice *c = &v->choice;
    CtrPokeTouchRect paper;
    char word[sizeof(c->label[0])];

    DevShell();
    for (int i = 0; i < DevChoiceCount(c); ++i) {
        CtrPokeTouchRect r = DevChoiceKey(c->count, c->cols, i);
        int sunk = DevChosen(i, c->cursor, c->pressed - PT_CHOICE_HIT_ROW), faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);

        DevUpper(word, c->label[i], sizeof(word));
        DevKeyWordAt(&r, faceY, DEV_LOOK_KEY, sunk, word, DevChoiceInset(c, &r));
    }
    paper = DevScreen(&kDevGridNote);
    if (c->prompt[0])
        DevReadText(c->prompt, &paper, paper.x + 6, paper.x + paper.w - 6, paper.y + 2, DEV_SMALL_LINE);
    DevFieldBack(v, &kDevGridExit, DevBackWord());
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the choice draws differs between `a` and `b` (the shell is the dispatcher's): the choice as filled, and BACK. */
static int DevChoiceSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return !memcmp(&a->choice, &b->choice, sizeof(a->choice)) && DevFieldBackSame(a, b);
}

/* BACK in the exit corner while the choice shows it. */
static const CtrPokeTouchRect *DevChoiceBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevGridExit : NULL;
}
