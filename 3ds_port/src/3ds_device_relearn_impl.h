/* The move relearner (REMEMBER, the Two Island man) as the red device (board def_relearn), included by 3ds_poketouch.c
 * after the move to forget: a row of the field dispatcher (3ds_device_field_impl.h), shown while view->relearn.on and
 * there is no GBA picture (DevRelearnShown). A remote for FireRed's screen (3ds_poketouch.h CtrPokeTouchRelearn), on
 * the grid's list page, every state of it drawn here (never FireRed's own picture):
 *  - the list: FireRed's question on the title; the moves it can remember in the list's five rows, 24 tall and 26 apart
 *    (the name, the type's chip at x 170, "PP n" at the right), the cursor's row inverted or the row under a held touch;
 *    ▲ ▼ on by v0.6.0's rules (▲ while the list or the cursor can go up, ▼ while the cursor is above CANCEL), grey and
 *    not hit otherwise; the cursor's move on the grid's note (its type's chip, POW and ACC, FireRed's description);
 *    FireRed's CANCEL row as CANCEL, a dark key in the grid's exit (owner decision 4: no BACK), sunk while the cursor is
 *    on that row or under a held touch;
 *  - a question: FireRed's words on the grid's narrow screen, YES and NO cream in the column beside it and no key in the
 *    exit corner (the Decision), FireRed's cursor sunk (or the key under a held touch), the cursor's move (its type's
 *    chip, its name) on the grid's note;
 *  - a message: on the list's screen, FireRed's ▼ while it waits for A (and then a tap anywhere is its A), the keys grey;
 *  - while it is set up: the empty screens, the keys grey, nothing hit.
 * A row is walked to with the list's own UP / DOWN and taken with its A; one tap on CANCEL is FireRed's B on the list
 * (3ds_bottom_relearn_impl.h ProcessRelearnTouch). Every rectangle is 3ds_device_layout.h's, so what is drawn is what a
 * touch takes.
 *
 * Words (decision 5): FireRed's move and type names (which v0.6.0 title-cased) through DevUpper; its question,
 * description and messages as FireRed writes them; the port's words in capitals.
 *
 * Redraw: the screen whole when the relearner as drawn differs (DevRelearnSame), and nothing else. */

#define DEV_RL_CHIP_X 170       /* a row's type chip; the name ends 4 px before it */

static int DevRelearnShown(const CtrPokeTouchView *v)
{
    return v->relearn.on && !v->viewport;
}

/* ▲ and ▼ by v0.6.0's rules: up while the list or the cursor can go up, down while the cursor is above CANCEL. */
static int DevRlUpOn(const CtrPokeTouchRelearn *l)
{
    return l->first > 0 || l->cursor > 0;
}

static int DevRlDownOn(const CtrPokeTouchRelearn *l)
{
    return l->cursor < l->count;
}

/* Row i holds a move (FireRed's CANCEL row, the list's last, is the CANCEL key; past it there is no row). */
static int DevRlRow(const CtrPokeTouchRelearn *l, int i)
{
    return l->first + i < l->count && l->row[i].move;
}

/* The row the cursor's move is in, or -1 (CANCEL, or a move not shown). */
static int DevRlCursorRow(const CtrPokeTouchRelearn *l)
{
    int c = l->cursor - l->first;

    return c >= 0 && c < PT_RL_ROWS && DevRlRow(l, c) ? c : -1;
}

int CtrPokeTouch_RelearnHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchRelearn *l = &v->relearn;

    if (!DevRelearnShown(v))
        return -1;
    switch (l->panel) {
    case PT_RL_PANEL_YESNO:
        for (int i = 0; i < 2; ++i) {
            CtrPokeTouchRect r = DevGridColumnKey(40, 36, 30, i);

            if (DevOnKey(&r, x, y))
                return i ? PT_RL_HIT_NO : PT_RL_HIT_YES;
        }
        return -1;
    case PT_RL_PANEL_MESSAGE:
        return l->tap ? PT_RL_HIT_MESSAGE : -1;           /* anywhere: it waits for A */
    case PT_RL_PANEL_BLANK:
        return -1;
    }
    if (DevOnKey(&kDevGridUp, x, y))
        return DevRlUpOn(l) ? PT_RL_HIT_UP : -1;
    if (DevOnKey(&kDevGridDown, x, y))
        return DevRlDownOn(l) ? PT_RL_HIT_DOWN : -1;
    if (DevOnKey(&kDevGridExit, x, y))
        return PT_RL_HIT_CANCEL;
    for (int i = 0; i < PT_RL_ROWS; ++i) {
        CtrPokeTouchRect r = DevGridRow(i, 0);

        if (DevRlRow(l, i) && UiIn(&r, x, y))
            return PT_RL_HIT_ROW + i;
    }
    return -1;
}

/* The touch held as it is drawn: a code of what is drawn and can be used in the panel shown, else -1 (▲ or ▼ where the
 * list goes no further, a row that is none, anything under a message). */
static int DevRlPress(const CtrPokeTouchRelearn *l)
{
    int p = l->pressed;

    if (l->panel == PT_RL_PANEL_YESNO)
        return p == PT_RL_HIT_YES || p == PT_RL_HIT_NO ? p : -1;
    if (l->panel != PT_RL_PANEL_LIST)
        return -1;
    if (p >= PT_RL_HIT_ROW && p < PT_RL_HIT_ROW + PT_RL_ROWS)
        return DevRlRow(l, p - PT_RL_HIT_ROW) ? p : -1;
    if (p == PT_RL_HIT_UP || p == PT_RL_HIT_DOWN)
        return (p == PT_RL_HIT_UP ? DevRlUpOn(l) : DevRlDownOn(l)) ? p : -1;
    return p == PT_RL_HIT_CANCEL ? p : -1;
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

/* A move's name in the normal font from x, capitals `top`, ending by `limit`: one that does not fit is cut there and
 * counted in sDevKeyCut, as a cut key label is (FireRed's widest, 72 px, fit wherever the relearner draws one). */
static void DevRlName(const char *word, int x, int top, int limit, uint16_t ink, uint16_t shadow)
{
    if (x + DevTextWidth(word) > limit)
        ++sDevKeyCut;
    DevTextInk(word, x, top, limit, ink, shadow);
}

/* Row i of the list (DevGridRow), inverted when `chosen`: the move's name from r.x + 6, capitals r.y + 8, over its
 * shadow, cut before 166 (DevRlName); its type's chip 12 tall at (170, r.y + 6); "PP n" (FireRed's base PP) ending at
 * r.x + r.w - 6, capitals r.y + 8, in the row's ink without a shadow. */
static void DevRlRowDraw(const CtrPokeTouchRelearn *l, int i, int chosen)
{
    CtrPokeTouchRect r = DevGridRow(i, 0);
    uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;
    int right = r.x + r.w - 6;
    char word[sizeof(l->row[0].name)], pp[sizeof(l->row[0].pp) + 4];

    DevRow(&r, chosen);
    DevUpper(word, l->row[i].name, sizeof(word));
    DevRlName(word, r.x + 6, r.y + 8, DEV_RL_CHIP_X - 4, ink, shadow);
    if (l->row[i].type >= 0)
        DevTypeChip(l->row[i].typeName, l->row[i].type, DEV_RL_CHIP_X, r.y + 6, 12);
    if (l->row[i].pp[0]) {
        snprintf(pp, sizeof(pp), "PP %s", l->row[i].pp);
        DevTextInk(pp, right - DevTextWidth(pp), r.y + 8, right, ink, ink);
    }
}

/* The grid's note under the list (Call 13): the cursor's move's type on its chip, 10 tall at (paper.x + 6, paper.y + 2);
 * "POW n  ACC n" small in the secondary ink 4 px after it, capitals paper.y + 2; FireRed's description small in the
 * paper's ink, two lines from capitals paper.y + 13, DEV_SMALL_LINE_TIGHT apart (the second's capitals at paper.y + 22,
 * so its descenders end on the paper's last row, not on the bezel), from x paper.x + 6 to paper.x + paper.w - 6; what
 * does not fit is cut and counted (DevSmallFit). Empty on CANCEL. */
static void DevRlDesc(const CtrPokeTouchRelearn *l)
{
    CtrPokeTouchRect paper = DevScreen(&kDevGridNote);
    int c = DevRlCursorRow(l), x = paper.x + 6, right = paper.x + paper.w - 6;
    char stats[48];

    if (c < 0)
        return;
    if (l->row[c].type >= 0)
        x += DevTypeChip(l->row[c].typeName, l->row[c].type, x, paper.y + 2, 10) + 4;
    DevPowAcc(stats, sizeof(stats), l->row[c].power, l->row[c].accuracy);
    if (x + DevSmallTextWidth(stats) > right)
        ++sDevKeyCut;
    DevSmallText(stats, x, paper.y + 2, DEV_INK2, right);
    DevSmallFit(l->desc, paper.x + 6, paper.x + 6, paper.y + 13, right, 2, DEV_SMALL_LINE_TIGHT, DEV_INK);
}

/* CANCEL, a dark key in the grid's exit: sunk as chosen; grey (`dim`) outside the list. */
static void DevRlCancel(int sunk, int dim)
{
    int faceY = DevKey(&kDevGridExit, DEV_LOOK_DARK, sunk, 6);

    DevKeyWord(&kDevGridExit, faceY, DEV_LOOK_DARK, sunk, DevCancelWord());
    if (dim)
        DevDim(&kDevGridExit);
}

/* ▲ and ▼ grey (a message, the screen being set up): drawn as the list has them, then dimmed. */
static void DevRlArrowsDim(const CtrPokeTouchRelearn *l, int on)
{
    DevArrowKeys(&kDevGridUp, &kDevGridDown, on && DevRlUpOn(l), on && DevRlDownOn(l), 0, 0, 0);
    DevDim(&kDevGridUp);
    DevDim(&kDevGridDown);
}

/* The list: the rows, ▲ ▼, the description and CANCEL; the chosen row or CANCEL the one under a held touch, else the
 * cursor's. */
static void DevRlList(const CtrPokeTouchRelearn *l, int press)
{
    int c = DevRlCursorRow(l), cursor = l->cursor >= l->count ? PT_RL_HIT_CANCEL : c >= 0 ? PT_RL_HIT_ROW + c : -1;
    int chosen = press == PT_RL_HIT_CANCEL || (press >= PT_RL_HIT_ROW && press < PT_RL_HIT_ROW + PT_RL_ROWS) ? press : -1;

    DevScreen(&kDevGridScreen);
    for (int i = 0; i < PT_RL_ROWS; ++i)
        if (DevRlRow(l, i))
            DevRlRowDraw(l, i, DevChosen(PT_RL_HIT_ROW + i, cursor, chosen));
    DevArrowKeys(&kDevGridUp, &kDevGridDown, DevRlUpOn(l), DevRlDownOn(l), press == PT_RL_HIT_UP,
                 press == PT_RL_HIT_DOWN, 0);
    DevRlDesc(l);
    DevRlCancel(DevChosen(PT_RL_HIT_CANCEL, cursor, chosen), 0);
}

/* A question (the Decision "Relearn YES / NO: keys in the column"): FireRed's words on the grid's narrow screen (nine
 * lines at most, centred down it); YES and NO cream in the column beside it (DevGridColumnKey(40, 36, 30, i)), their
 * words from x + 10 as the Bag's column keys have them, FireRed's cursor sunk or the key under a held touch; no key in
 * the exit corner (NO is FireRed's B). On the grid's note the cursor's move: its type's chip, 12 tall, and its name 6 px
 * after it (DevRlName: cut before paper.x + paper.w - 6 and counted), both centred down the paper. */
static void DevRlYesNo(const CtrPokeTouchRelearn *l, int press)
{
    CtrPokeTouchRect paper = DevScreen(&kDevGridScreenNarrow);
    int width = paper.w - 12, lines = DevWrapLines(l->message, width, 9), c = DevRlCursorRow(l);
    char word[24];

    if (lines)
        DevWrap(l->message, paper.x + 6, paper.y + (paper.h - (8 + DEV_LINE_H * (lines - 1))) / 2, width, 9, DEV_INK, DEV_INK_SH);
    for (int i = 0; i < 2; ++i) {
        CtrPokeTouchRect r = DevGridColumnKey(40, 36, 30, i);
        int sunk = DevChosen(i, l->menuCursor, press >= 0 ? press - PT_RL_HIT_YES : -1), faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);

        DevUpper(word, CtrUiText(i ? UIS_NO : UIS_YES), sizeof(word));
        DevKeyWordAt(&r, faceY, DEV_LOOK_KEY, sunk, word, 10);
    }
    paper = DevScreen(&kDevGridNote);
    if (c >= 0) {
        int x = paper.x + 6;

        if (l->row[c].type >= 0)
            x += DevTypeChip(l->row[c].typeName, l->row[c].type, x, paper.y + (paper.h - 12) / 2, 12) + 6;
        DevUpper(word, l->row[c].name, sizeof(word));
        DevRlName(word, x, paper.y + (paper.h - 8) / 2, paper.x + paper.w - 6, DEV_INK, DEV_INK_SH);
    }
}

/* A message on the list's screen, FireRed's ▼ while it waits for A, the keys grey. */
static void DevRlMessage(const CtrPokeTouchRelearn *l)
{
    CtrPokeTouchRect paper = DevScreen(&kDevGridScreen);

    DevWrap(l->message, paper.x + 8, paper.y + 8, paper.w - 28, (paper.h - 16) / DEV_LINE_H, DEV_INK, DEV_INK_SH);
    if (l->tap)
        DevTriangle(paper.x + paper.w - 10, paper.y + paper.h - 8, 1, DEV_INK2);
    DevRlArrowsDim(l, 1);
    DevRlDesc(l);
    DevRlCancel(0, 1);
}

/* The relearner whole (after DevBegin: the dispatcher's); while it is set up, the empty screens and the keys grey. */
static void DevRelearnDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRelearn *l = &v->relearn;
    CtrPokeTouchRect paper;
    int press = DevRlPress(l);

    DevShell();
    paper = DevScreen(&kDevGridTitle);
    DevText(l->question, paper.x + 6, paper.y + (paper.h - 8) / 2, paper.x + paper.w - 6);
    switch (l->panel) {
    case PT_RL_PANEL_YESNO:
        DevRlYesNo(l, press);
        break;
    case PT_RL_PANEL_MESSAGE:
        DevRlMessage(l);
        break;
    case PT_RL_PANEL_BLANK:
        DevScreen(&kDevGridScreen);
        DevRlArrowsDim(l, 0);
        DevScreen(&kDevGridNote);
        DevRlCancel(0, 1);
        break;
    default:
        DevRlList(l, press);
        break;
    }
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* What of `l` is drawn: the view but the Pokemon (no column here), the press as drawn, the YES / NO cursor outside a
 * question, the message and its wait for A outside the panels that show them. */
static void DevRlDrawn(const CtrPokeTouchRelearn *l, CtrPokeTouchRelearn *out)
{
    memcpy(out, l, sizeof(*out));
    out->species = 0;
    memset(out->nickname, 0, sizeof(out->nickname));
    memset(out->level, 0, sizeof(out->level));
    out->pressed = (int16_t)DevRlPress(l);
    if (l->panel != PT_RL_PANEL_YESNO)
        out->menuCursor = 0;
    if (l->panel != PT_RL_PANEL_MESSAGE)
        out->tap = 0;
    if (l->panel != PT_RL_PANEL_YESNO && l->panel != PT_RL_PANEL_MESSAGE)
        memset(out->message, 0, sizeof(out->message));
}

/* Nothing the relearner draws differs between `a` and `b` (the shell is the dispatcher's; it draws no BACK). */
static int DevRelearnSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    CtrPokeTouchRelearn p, q;

    DevRlDrawn(&a->relearn, &p);
    DevRlDrawn(&b->relearn, &q);
    return !memcmp(&p, &q, sizeof(p));
}

/* No BACK: CANCEL and NO are FireRed's B. */
static const CtrPokeTouchRect *DevRelearnBackKey(const CtrPokeTouchView *v)
{
    (void)v;
    return NULL;
}
