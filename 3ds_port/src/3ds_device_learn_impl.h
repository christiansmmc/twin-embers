/* The move to forget for a new one as the red device (no board; plan 3), included by 3ds_poketouch.c after the Pokedex
 * (whose species' pictures it draws): a row of the field dispatcher (3ds_device_field_impl.h), shown while
 * view->learn.on and there is no GBA picture (DevLearnShown). A remote for FireRed's Summary in that mode
 * (3ds_poketouch.h CtrPokeTouchLearn), on the Summary's page: FireRed's question on the Bag's title; the Summary's
 * screen with the Pokemon in its column (the name, the level, its picture, the cursor's move's POWER and ACCURACY at
 * the foot) and the four moves and the new one in its rows (the name, the type's chip, the PP; a rule over the new
 * one), the cursor's row inverted or the row under a held touch, an empty one not drawn; the cursor's move's
 * description on the note; BACK (the Summary's B). A row is walked to with the Summary's UP / DOWN and taken with its A
 * (3ds_bottom.c ProcessLearnTouch). Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): FireRed's move and type names (which v0.6.0 title-cased) through DevUpper; the Pokemon's name in
 * capitals when it is its species' own (nameDefault: the battle fill's check), as typed when the player gave it;
 * FireRed's question and description as FireRed writes them; the port's words in capitals.
 *
 * Redraw: the screen whole when the move to forget as drawn differs (DevLearnSame), and nothing else. */

#define DEV_LEARN_PP_W 34                       /* a row's PP at its right ("35/35"), and its type's chip before it */

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
        CtrPokeTouchRect r = DevSumRow(i);

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

/* The column (the paper's second tone): the name (the small font when the normal one does not fit), the level small
 * ("Lv36" as the Summary writes it: the fill gives the number), the picture in the Summary's place, the cursor's move's
 * POWER and ACCURACY at the foot. */
static void DevLearnColumn(const CtrPokeTouchLearn *l)
{
    const CtrPokeTouchRect *col = &kDevSumColumn, *pic = &kDevSumPic;
    int c = DevLearnCursor(l), right = col->x + col->w - 6;
    char name[sizeof(l->nickname)], word[24];
    const char *shown = DevMonName(l->nickname, l->nameDefault, name, sizeof(name));

    DevRect(col->x, col->y, col->w, col->h, DEV_PAPER2);
    if (DevTextWidth(shown) <= right - (col->x + 6))
        DevText(shown, col->x + 6, col->y + 6, right);
    else
        DevSmallText(shown, col->x + 6, col->y + 7, DEV_INK, right);
    snprintf(word, sizeof(word), CTR_TR("Lv%s", "Nv%s", "Lv%s"), l->level);
    DevSmallText(word, col->x + 6, col->y + 21, DEV_INK2, right);
    DevArt(DEV_ART_PICTURE, pic->x, pic->y, pic->w, pic->h);
    FDPicDraw(l->species, pic->x, pic->y, 0);
    if (c < 0)
        return;
    DevUpper(word, CtrUiText(UIS_MOVE_POWER), sizeof(word));
    DevSumColumnRow(pic->y + pic->h + 2, word, l->row[c].power[0] ? l->row[c].power : "-");
    DevUpper(word, CtrUiText(UIS_MOVE_ACCURACY), sizeof(word));
    DevSumColumnRow(pic->y + pic->h + 14, word, l->row[c].accuracy[0] ? l->row[c].accuracy : "-");
}

/* A move's type chip: its name in capitals in the small font, 40 wide (the board's type chip) or the word's width and
 * 4 px a side. Its width; and the chip at (x, y), h tall, returning its width. (The move to forget's rows, the
 * relearner's rows and descriptions.) */
static int DevMoveChipWidth(const char *typeName)
{
    char word[16];
    int w;

    DevUpper(word, typeName, sizeof(word));
    w = DevSmallTextWidth(word) + 8;
    return w < 40 ? 40 : w;
}

static int DevMoveChip(const char *typeName, int type, int x, int y, int h)
{
    uint16_t bg = UiTypeColor(type);
    char word[16];
    int w = DevMoveChipWidth(typeName);

    DevUpper(word, typeName, sizeof(word));
    DevChip(x, y, w, h, word, bg, DevChipShadow(bg));
    return w;
}

/* Row i, inverted when `chosen`: the move's name, its type's chip and its PP at the right. */
static void DevLearnRow(const CtrPokeTouchLearn *l, int i, int chosen)
{
    CtrPokeTouchRect r = DevSumRow(i);
    uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;
    int top = r.y + (r.h - UI_TEXT_H) / 2, right = r.x + r.w - 6, nameRight = right - DEV_LEARN_PP_W - 4;
    char word[sizeof(l->row[0].name)];

    DevRow(&r, chosen);
    DevTextInk(l->row[i].pp, right - DevTextWidth(l->row[i].pp), top, right, ink, shadow);
    if (l->row[i].type >= 0) {
        nameRight -= DevMoveChipWidth(l->row[i].typeName);
        DevMoveChip(l->row[i].typeName, l->row[i].type, nameRight, r.y + (r.h - 12) / 2, 12);
    }
    DevUpper(word, l->row[i].name, sizeof(word));
    DevTextInk(word, r.x + 6, top, nameRight - 4, ink, shadow);
}

/* The move to forget whole (after DevBegin: the dispatcher's). */
static void DevLearnDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchLearn *l = &v->learn;
    int pressed = l->pressed - PT_LEARN_HIT_ROW, c = DevLearnCursor(l);
    CtrPokeTouchRect paper, last = DevSumRow(PT_LEARN_ROWS - 1);

    DevShell(0);
    paper = DevScreen(&kDevBagTitle);
    DevText(l->question, paper.x + 6, paper.y + (paper.h - 8) / 2, paper.x + paper.w - 6);
    DevScreen(&kDevSumScreen);
    DevLearnColumn(l);
    for (int i = 0; i < PT_LEARN_ROWS; ++i)
        if (l->row[i].move)
            DevLearnRow(l, i, DevChosen(i, l->cursor, pressed));
    DevRect(last.x, last.y - 2, last.w, 1, DEV_INK_SH);     /* the new move under a rule */
    paper = DevScreen(&kDevSumNote);
    if (c >= 0)
        DevBagRead(l->desc, &paper, paper.x + 6, paper.x + paper.w - 6);
    DevFieldBack(v, &kDevSumBack, DevBackWord());
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the move to forget draws differs between `a` and `b` (the shell is the dispatcher's): the view as filled but
 * the HM locks (never drawn: FireRed answers A on one with its failure sound), and BACK. */
static int DevLearnSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    CtrPokeTouchLearn p, q;

    memcpy(&p, &a->learn, sizeof(p));
    memcpy(&q, &b->learn, sizeof(q));
    for (int i = 0; i < PT_LEARN_ROWS; ++i)
        p.row[i].locked = q.row[i].locked = 0;
    return !memcmp(&p, &q, sizeof(p)) && DevFieldBackSame(a, b);
}

/* BACK in the Summary's place while the view shows it. */
static const CtrPokeTouchRect *DevLearnBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevSumBack : NULL;
}
