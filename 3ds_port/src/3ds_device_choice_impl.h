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
 * A priced row (recipe 0267 splits FireRed's label at its CLEAR_TO: the drinks, the Game Corner's coins and prizes)
 * shows its price in the keys' secondary ink: ending 12 px in from the right of a list's key, or small under the label
 * on the others (a narrower key, and the tall keys of two rows in one column: the Cerulean bike shop's BICYCLE; a YES /
 * NO has no price, so it stays as it is).
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

/* YES / NO: two rows in FireRed's one column, the two tall keys (def_choice_yesno). */
static int DevChoiceYesNo(const CtrPokeTouchChoice *c)
{
    return c->count == 2 && c->cols < 2;
}

/* Where key r's word starts: 12 px in on a list's key across the screen (def_choice_list); 0, centred, on YES / NO's
 * tall keys (def_choice_yesno) and on a key of two or three columns (def_choice_grid). */
static int DevChoiceInset(const CtrPokeTouchChoice *c, const CtrPokeTouchRect *r)
{
    return r->w == 304 && !DevChoiceYesNo(c) ? 12 : 0;
}

/* `word` as it fits `room`: itself, or cut with "..." into `cut` and counted in sDevKeyCut. */
static const char *DevChoiceFit(const char *word, int room, char *cut, size_t size)
{
    if (DevTextWidth(word) <= room)
        return word;
    DevCutText(word, room, cut, size);
    ++sDevKeyCut;
    return cut;
}

/* A priced key's label and price. On a list's key (`inset` 12, def_choice_list) the price ends `inset` px in from the
 * right in DEV_KEY_INK2 without a shadow and the label runs from `inset` px in to 8 px before it, both centred down the
 * key; on any other (inset 0: a narrower key, or a tall one of two rows in one column) the label centred across and the
 * price small and centred under it, the pair (the label's capitals, 2 px, the price's 7) centred down the face: their
 * capitals 10 and 21 px down a 38 px key, 33 and 44 down an 84 px one. What does not fit is cut and counted. */
static void DevChoicePriced(const CtrPokeTouchRect *r, int faceY, int sunk, const char *word, const char *price,
                            int inset)
{
    uint16_t ink = DevKeyInk(DEV_LOOK_KEY, sunk), shadow = DevKeyInkShadow(DEV_LOOK_KEY, sunk);
    char cut[32];
    int w, top = faceY + (r->h - (UI_TEXT_H + 2 + 7)) / 2;

    if (inset) {
        int right = r->x + r->w - inset, end = right - DevTextWidth(price) - 8, line = faceY + (r->h - UI_TEXT_H) / 2;

        DevTextRight(price, right, line, DEV_KEY_INK2);
        DevTextInk(DevChoiceFit(word, end - (r->x + inset), cut, sizeof(cut)), r->x + inset, line, end, ink, shadow);
        return;
    }
    word = DevChoiceFit(word, r->w - 8, cut, sizeof(cut));
    DevTextInk(word, r->x + (r->w - DevTextWidth(word)) / 2, top, r->x + r->w - 4, ink, shadow);
    w = DevSmallTextWidth(price);
    if (w > r->w - 8)
        ++sDevKeyCut;
    DevSmallText(price, w > r->w - 8 ? r->x + 4 : r->x + (r->w - w) / 2, top + UI_TEXT_H + 2, DEV_KEY_INK2,
                 r->x + r->w - 4);
}

/* The choice whole (after DevBegin: the dispatcher's). */
static void DevChoiceDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchChoice *c = &v->choice;
    CtrPokeTouchRect paper;
    char word[sizeof(c->label[0])], price[sizeof(c->price[0])];

    DevShell();
    for (int i = 0; i < DevChoiceCount(c); ++i) {
        CtrPokeTouchRect r = DevChoiceKey(c->count, c->cols, i);
        int sunk = DevChosen(i, c->cursor, c->pressed - PT_CHOICE_HIT_ROW), faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);

        DevUpper(word, c->label[i], sizeof(word));
        DevUpper(price, c->price[i], sizeof(price));
        if (price[0])
            DevChoicePriced(&r, faceY, sunk, word, price, DevChoiceInset(c, &r));
        else
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
