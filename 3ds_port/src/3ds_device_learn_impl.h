/* The move to forget for a new one as the red device (board def_learn), included by 3ds_poketouch.c after the Pokedex
 * (whose species' pictures it draws): a row of the field dispatcher (3ds_device_field_impl.h), shown while
 * view->learn.on and there is no GBA picture (DevLearnShown). A remote for FireRed's Summary in that mode
 * (3ds_poketouch.h CtrPokeTouchLearn), on the grid: FireRed's question on the grid's title; the grid's wide screen with
 * the Pokemon in its column (the name, the level, its picture, the cursor's move's POWER and ACCURACY at the foot) and
 * the four moves and the new one in its rows (the name, the type's chip, the PP; the new one under a rule, NEW on its
 * chip before the name), the cursor's row inverted or the row under a held touch, an empty one not drawn; on the grid's
 * note the cursor's move's description and the hint for its row (tap it again: an old move is forgotten, the new one
 * not learnt, an HM cannot be forgotten); BACK (the Summary's B) in the exit corner. A row is walked to with the
 * Summary's UP / DOWN and taken with its A (3ds_bottom.c ProcessLearnTouch: a tap on the cursor's row is its A, so a
 * row is taken by tapping it twice). Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): FireRed's move and type names (which v0.6.0 title-cased) through DevUpper; the Pokemon's name in
 * capitals when it is its species' own (nameDefault: the battle fill's check), as typed when the player gave it;
 * FireRed's question and description as FireRed writes them; the port's words (NEW, the hints) in capitals.
 *
 * Redraw: the screen whole when the move to forget as drawn differs (DevLearnSame), and nothing else. */

#define DEV_LEARN_NAME_X     130                /* a row's name (after NEW on the new one's) */
#define DEV_LEARN_CHIP_X     216                /* the type's chip, 4 px after the room for the name */
#define DEV_LEARN_CHIP_MAX   226                /* ... or as far as this for a long new move: 2 px short of the widest PP */
#define DEV_LEARN_PP_RIGHT   298                /* where the PP ends (the widest, "40/40", 30 px, from 268) */
#define DEV_LEARN_NEW_PAD    4                  /* NEW's chip: its word and 2 px a side */

static int DevLearnShown(const CtrPokeTouchView *v)
{
    return v->learn.on && !v->viewport;
}

int CtrPokeTouch_LearnHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchLearn *l = &v->learn;

    if (!DevLearnShown(v))
        return -1;
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        CtrPokeTouchRect r = DevLearnSlot(i);

        if (l->row[i].move && UiIn(&r, x, y))
            return PT_LEARN_HIT_ROW + i;
    }
    return -1;
}

/* The cursor's row, or -1 when it is on none. */
static int DevLearnCursor(const CtrPokeTouchLearn *l)
{
    return l->cursor >= 0 && l->cursor < PT_LEARN_ROWS && l->row[l->cursor].move ? l->cursor : -1;
}

/* The hint for the cursor's row (def_learn's footer, Call 15): the new one (row 4) tapped again is not learnt, an HM
 * (FireRed answers A on one with its failure sound) cannot be forgotten, another old move tapped again is forgotten;
 * NULL with the cursor on no row. One literal per hint, so two views give the same pointer for the same words. */
static const char *DevLearnHint(const CtrPokeTouchLearn *l)
{
    int c = DevLearnCursor(l);

    if (c < 0)
        return NULL;
    if (c == PT_LEARN_ROWS - 1)
        return CTR_TR("TAP AGAIN TO SKIP IT.", "TOCA OTRA VEZ PARA NO APRENDERLO.", "NOCHMAL TIPPEN ZUM AUSLASSEN.");
    if (l->row[c].locked)
        return CTR_TR("HM MOVES CAN'T BE FORGOTTEN.", "LAS MO NO SE PUEDEN OLVIDAR.",
                      "VM K\xC3\x96NNEN NICHT VERGESSEN WERDEN.");
    return CTR_TR("TAP AGAIN TO FORGET IT.", "TOCA OTRA VEZ PARA OLVIDARLO.", "NOCHMAL TIPPEN ZUM VERGESSEN.");
}

/* The column (kDevLearnColumn, the paper's second tone): the name, capitals 52 (the small font, capitals 53, when the
 * normal one does not fit), the level small, capitals 66 ("Lv36" as the Summary writes it: the fill gives the number),
 * the picture in its box (kDevLearnPic), the cursor's move's POWER and ACCURACY under it, their numbers ending at 110. */
static void DevLearnColumn(const CtrPokeTouchLearn *l)
{
    const CtrPokeTouchRect *col = &kDevLearnColumn, *pic = &kDevLearnPic;
    int c = DevLearnCursor(l), x = col->x + 6, right = col->x + col->w - 8;
    char name[sizeof(l->nickname)], word[24];
    const char *shown = DevMonName(l->nickname, l->nameDefault, name, sizeof(name));

    DevRect(col->x, col->y, col->w, col->h, DEV_PAPER2);
    if (DevTextWidth(shown) <= right - x)
        DevText(shown, x, col->y + 6, right);
    else
        DevSmallText(shown, x, col->y + 7, DEV_INK, right);
    snprintf(word, sizeof(word), CTR_TR("Lv%s", "Nv%s", "Lv%s"), l->level);
    DevSmallText(word, x, col->y + 20, DEV_INK2, right);
    DevArt(DEV_ART_PICTURE, pic->x, pic->y, pic->w, pic->h);
    FDPicDraw(l->species, pic->x, pic->y, 0);
    if (c < 0)
        return;
    DevUpper(word, CtrUiText(UIS_MOVE_POWER), sizeof(word));
    DevColumnRow(pic->y + pic->h + 6, word, l->row[c].power[0] ? l->row[c].power : "-", x, right);
    DevUpper(word, CtrUiText(UIS_MOVE_ACCURACY), sizeof(word));
    DevColumnRow(pic->y + pic->h + 20, word, l->row[c].accuracy[0] ? l->row[c].accuracy : "-", x, right);
}

/* Row i, inverted when `chosen`: on the new one (row 4) NEW on its chip, 12 tall at (130, r.y + 5), its word and 2 px a
 * side; the move's name from x 130, or 3 px after NEW, capitals r.y + 7 over its shadow, ending 4 px before the type's
 * chip, 12 tall at (216, r.y + 5); the PP ending at 298, capitals r.y + 7, without a shadow. A name the normal font does
 * not fit before 212 is drawn in the small one (capitals r.y + 8); one the small font does not fit either (a long new
 * move after NUEVO: FireRed's widest names are 60 px in it) moves the type's chip right as far as it needs, at most to
 * 226, clear of the PP; what does not fit even then is cut there and counted in sDevKeyCut, as a cut key label is. */
static void DevLearnRow(const CtrPokeTouchLearn *l, int i, int chosen)
{
    CtrPokeTouchRect r = DevLearnSlot(i);
    uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;
    int x = DEV_LEARN_NAME_X, chip = DEV_LEARN_CHIP_X, small, w;
    char word[sizeof(l->row[0].name)];

    DevRow(&r, chosen);
    if (i == PT_LEARN_ROWS - 1) {
        const char *tag = CTR_TR("NEW", "NUEVO", "NEU");
        int tw = DevSmallTextWidth(tag) + DEV_LEARN_NEW_PAD;

        DevChip(x, r.y + 5, tw, 12, tag, DEV_TAG_NEW, DevChipShadow(DEV_TAG_NEW));
        x += tw + 3;
    }
    DevUpper(word, l->row[i].name, sizeof(word));
    small = DevTextWidth(word) > chip - 4 - x;
    w = small ? DevSmallTextWidth(word) : DevTextWidth(word);
    if (x + w + 4 > chip)
        chip = x + w + 4 < DEV_LEARN_CHIP_MAX ? x + w + 4 : DEV_LEARN_CHIP_MAX;
    if (x + w + 4 > chip)
        ++sDevKeyCut;
    if (small)
        DevSmallText(word, x, r.y + 8, ink, chip - 4);
    else
        DevTextInk(word, x, r.y + 7, chip - 4, ink, shadow);
    if (l->row[i].type >= 0)
        DevTypeChip(l->row[i].typeName, l->row[i].type, chip, r.y + 5, 12);
    DevTextInk(l->row[i].pp, DEV_LEARN_PP_RIGHT - DevTextWidth(l->row[i].pp), r.y + 7, DEV_LEARN_PP_RIGHT, ink, ink);
}

/* The grid's note: the cursor's move's description small in the paper's ink, two lines from capitals 199 (the game's
 * accents, 3 over them, on the paper), 9 apart, and the hint for its row small in the secondary ink, capitals 218 (Call
 * 18: its descenders end on the paper's last row, 225), from x 20 to 224; words that need more lines are cut and counted
 * (DevSmallFit). Empty with the cursor on no row. */
static void DevLearnNote(const CtrPokeTouchLearn *l)
{
    CtrPokeTouchRect paper = DevScreen(&kDevGridNote);
    const char *hint = DevLearnHint(l);
    int x = paper.x + 6, right = paper.x + paper.w - 6;

    if (!hint)
        return;
    DevSmallFit(l->desc, x, x, paper.y + 3, right, 2, DEV_SMALL_LINE_TIGHT, DEV_INK);
    DevSmallFit(hint, x, x, paper.y + 22, right, 1, DEV_SMALL_LINE, DEV_INK2);
}

/* The move to forget whole (after DevBegin: the dispatcher's). */
static void DevLearnDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchLearn *l = &v->learn;
    int pressed = l->pressed - PT_LEARN_HIT_ROW;
    CtrPokeTouchRect paper;

    DevShell();
    paper = DevScreen(&kDevGridTitle);
    DevText(l->question, paper.x + 6, paper.y + (paper.h - 8) / 2, paper.x + paper.w - 6);
    DevScreen(&kDevGridWide);
    DevLearnColumn(l);
    for (int i = 0; i < PT_LEARN_ROWS; ++i)
        if (l->row[i].move)
            DevLearnRow(l, i, DevChosen(i, l->cursor, pressed));
    DevRect(kDevLearnRule.x, kDevLearnRule.y, kDevLearnRule.w, kDevLearnRule.h, DEV_RULE);     /* the new one under it */
    DevLearnNote(l);
    DevFieldBack(v, &kDevGridExit, DevBackWord());
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the move to forget draws differs between `a` and `b` (the shell is the dispatcher's): the view as filled but
 * the HM locks, of which only the cursor's row's is drawn (its hint, DevLearnHint), and BACK. */
static int DevLearnSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    CtrPokeTouchLearn p, q;

    memcpy(&p, &a->learn, sizeof(p));
    memcpy(&q, &b->learn, sizeof(q));
    for (int i = 0; i < PT_LEARN_ROWS; ++i)
        p.row[i].locked = q.row[i].locked = 0;
    return !memcmp(&p, &q, sizeof(p)) && DevLearnHint(&a->learn) == DevLearnHint(&b->learn) && DevFieldBackSame(a, b);
}

/* BACK (kDevGridExit) while the view shows it. */
static const CtrPokeTouchRect *DevLearnBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevGridExit : NULL;
}
