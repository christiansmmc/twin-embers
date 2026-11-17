/* A choice of the field as the red device (no board; plan 3), included by 3ds_poketouch.c after the relearner: a row of
 * the field dispatcher (3ds_device_field_impl.h), shown while view->choice.on and there is no GBA picture
 * (DevChoiceShown). A remote for FireRed's menu (3ds_poketouch.h CtrPokeTouchChoice: a script's multichoice, YES/NO or
 * grid, the PC's menus): its rows as keys across the screen (DevChoiceKey: one column up to four, two beyond going down
 * first, a grid across), FireRed's cursor sunk, or the key under a held touch; the message box's question on the note
 * under them, the normal font up to four lines and the small one up to six beyond; BACK (its B) beside the note. A key is
 * asked of the menu and taken with its A (3ds_bottom.c ProcessChoiceTouch); the upper screen's hide and hold are
 * 3ds_bottom.c's, unchanged. Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): FireRed's labels (which v0.6.0 title-cased) through DevUpper; its question as FireRed writes it;
 * BACK's word the port's.
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

        if (DevBagOnKey(&r, x, y))
            return PT_CHOICE_HIT_ROW + i;
    }
    return -1;
}

/* The message box's question on the note's paper: the normal font in four lines at most, centred down the paper; a
 * question that needs more, the small font in six. */
static void DevChoicePrompt(const char *text, const CtrPokeTouchRect *paper)
{
    int x = paper->x + 6, width = paper->w - 12, lines = DevWrapLines(text, width, 5);

    if (!lines)
        return;
    if (lines <= 4)
        DevWrap(text, x, paper->y + (paper->h - (8 + DEV_LINE_H * (lines - 1))) / 2, width, 4, DEV_INK, DEV_INK_SH);
    else
        DevBagSmallWrap(text, x, x, paper->y + 4, x + width, 6, DEV_INK);
}

/* The choice whole (after DevBegin: the dispatcher's). */
static void DevChoiceDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchChoice *c = &v->choice;
    CtrPokeTouchRect paper;
    char word[sizeof(c->label[0])];

    DevShell(0);
    for (int i = 0; i < DevChoiceCount(c); ++i) {
        CtrPokeTouchRect r = DevChoiceKey(c->count, c->cols, i);
        int sunk = DevChosen(i, c->cursor, c->pressed - PT_CHOICE_HIT_ROW), faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);

        DevUpper(word, c->label[i], sizeof(word));
        DevKeyWord(&r, faceY, DEV_LOOK_KEY, sunk, word);
    }
    paper = DevScreen(&kDevChoiceNote);
    DevChoicePrompt(c->prompt, &paper);
    DevFieldBack(v, &kDevChoiceBack, DevBackWord());
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the choice draws differs between `a` and `b` (the shell is the dispatcher's): the choice as filled, and BACK. */
static int DevChoiceSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return !memcmp(&a->choice, &b->choice, sizeof(a->choice)) && DevFieldBackSame(a, b);
}

/* BACK beside the note while the choice shows it. */
static const CtrPokeTouchRect *DevChoiceBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevChoiceBack : NULL;
}
