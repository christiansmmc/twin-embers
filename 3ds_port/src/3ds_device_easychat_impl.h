/* Easy Chat as the red device (no board; plan 3), included by 3ds_poketouch.c after the PC box: the field dispatcher's
 * first row (3ds_device_field_impl.h), shown while view->ec.on (DevEcShown), the whole lower screen (the upper one keeps
 * FireRed's own). A remote for FireRed's Easy Chat (3ds_poketouch.h CtrPokeTouchEasy), every state of it drawn here:
 *  - the phrase: FireRed's title and how many of its slots hold a word on the phrase's screen; the slots as keys in two
 *    rows (the word, an empty slot a short rule); BY GROUP and A TO Z (the order the next word is picked in, the one
 *    wantAlpha names sunk); CLEAR, CANCEL (dark: FireRed's B) and CONFIRM;
 *  - the groups, or in the A-Z order the letters (FireRed's rows of 6, 6, 7 and 7): the title and the count on the
 *    title's screen, the groups' names or A..Z as keys, the toggle to the other order and BACK (dark);
 *  - a group's or a letter's words: the list's title (the group, or the letter with A-Z) over the phrase in one line;
 *    the words two a row in six rows, the selected inverted; the rail on the list's paper (a 4 px track, the thumb where
 *    the rows shown are: a slider); ▲ ▼ (FireRed's UP and DOWN, always taken: the arrow grey while the list goes no
 *    further that way); the letter strip (a slider: the first word of a letter; a group's list only, grey under A-Z, its
 *    first row's letter boxed); USE WORD, the toggle and BACK (dark, at the right as on every B view);
 *  - a question (QUIT, CLEAR, CONFIRM): the phrase's screen and slots grey, FireRed's question (and v0.6.0's line after
 *    QUIT's) on the question's note, YES and NO.
 * A key is sunk by FireRed's cursor (a slot, a button, a group or letter, YES / NO; the word's row inverted) or, among
 * the keys of its group, by a held touch instead (DevChosen). A touch is walked to with FireRed's own keys
 * (3ds_bottom_easychat_impl.h ProcessEasyTouch); the hit codes are v0.6.0's (PT_EC_HIT_*). Every rectangle is
 * 3ds_device_layout.h's, so what is drawn is what a touch takes. No BACK of the view's: Easy Chat's keys are its own.
 *
 * Words (decision 5): FireRed's words and groups (ALL CAPS: v0.6.0 title-cased them) through DevUpper; its title and
 * questions as it writes them ("Quit editing?", "PROFILE", "Interview"); the port's words in capitals.
 *
 * Redraw: the screen whole when the Easy Chat view differs (DevEcSame: the ec struct), and nothing else. */

static int DevEcShown(const CtrPokeTouchView *v)
{
    return v->ec.on;
}

/* The phrase's slots: FireRed's, 1 to PT_EC_WORDS. */
static int DevEcSlots(const CtrPokeTouchEasy *e)
{
    return e->numWords < 1 ? 1 : e->numWords > PT_EC_WORDS ? PT_EC_WORDS : e->numWords;
}

/* The keys of the groups' or the letters' screen: FireRed's groups (PT_EC_GROUPS at most), or the 26 letters. */
static int DevEcCells(const CtrPokeTouchEasy *e)
{
    if (e->screen == PT_EC_LETTER)
        return PT_EC_LETTERS;
    return e->groupCount > PT_EC_GROUPS ? PT_EC_GROUPS : e->groupCount;
}

/* Cell i's key: a letter's or a group's. */
static CtrPokeTouchRect DevEcCellKey(const CtrPokeTouchEasy *e, int i)
{
    return e->screen == PT_EC_LETTER ? DevEcLetter(i) : DevEcCell(i);
}

/* The list's rows, two words each. */
static int DevEcRows(const CtrPokeTouchEasy *e)
{
    return (e->listCount + 1) / 2;
}

/* Row place k (0-11) holds a word of the list: nothing past its last. */
static int DevEcRowOn(const CtrPokeTouchEasy *e, int k)
{
    return 2 * e->first + k < e->listCount && e->list[k][0];
}

/* The rail's thumb: its top and height in the rail, v0.6.0's on the rail's 124 px (the share of the rows shown, 14 px
 * at least; the whole rail when the list does not scroll). */
static void DevEcThumb(const CtrPokeTouchEasy *e, int *top, int *h)
{
    int total = DevEcRows(e), range = total - PT_EC_ROWS;

    *top = 0;
    *h = kDevEcRail.h;
    if (range < 1)
        return;
    *h = kDevEcRail.h * PT_EC_ROWS / total;
    if (*h < 14)
        *h = 14;
    *top = (e->first > range ? range : (int)e->first) * (kDevEcRail.h - *h) / range;
}

int CtrPokeTouch_EasyRailTarget(const CtrPokeTouchEasy *e, int y)
{
    int range = DevEcRows(e) - PT_EC_ROWS, top, h, t;

    if (range < 1)
        return -1;
    DevEcThumb(e, &top, &h);
    t = y - kDevEcRail.y - h / 2;
    t = t < 0 ? 0 : t > kDevEcRail.h - h ? kDevEcRail.h - h : t;
    return (t * range * 2 + (kDevEcRail.h - h)) / (2 * (kDevEcRail.h - h));
}

int CtrPokeTouch_EasyStripLetter(int x, int y)
{
    CtrPokeTouchRect a = DevEcStripCell(0);
    int col = (x - a.x) / a.w, row = (y - (a.y + 1)) / a.h;     /* from the letters' capitals: x 290, y 60 */

    if (!UiIn(&kDevEcStrip, x, y))
        return -1;
    return 13 * (col < 0 ? 0 : col > 1 ? 1 : col) + (row < 0 ? 0 : row > 12 ? 12 : row);
}

/* The codes of a list's bottom keys (DevEcListKey): USE WORD, the toggle, BACK. */
static const int16_t kDevEcListCode[3] = {PT_EC_HIT_PLACE, PT_EC_HIT_TOGGLE, PT_EC_HIT_BACK};

int CtrPokeTouch_EasyHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchEasy *e = &v->ec;
    int n;

    if (!DevEcShown(v))
        return -1;
    if (e->ask) {
        if (DevOnKey(&kDevAskYes, x, y))
            return PT_EC_HIT_YES;
        return DevOnKey(&kDevAskNo, x, y) ? PT_EC_HIT_NO : -1;
    }
    if (e->screen == PT_EC_EDIT) {
        n = DevEcSlots(e);
        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r = DevEcChip(n, i);

            if (DevOnKey(&r, x, y))
                return PT_EC_HIT_CHIP + i;
        }
        for (int i = 0; i < 2; ++i) {
            CtrPokeTouchRect r = DevEcMode(i);

            if (DevOnKey(&r, x, y))
                return PT_EC_HIT_MODE_GROUPS + i;   /* BY GROUP, A TO Z */
        }
        for (int i = 0; i < 3; ++i) {
            CtrPokeTouchRect r = DevEcFooter(i);

            if (DevOnKey(&r, x, y))
                return PT_EC_HIT_DELETE + i;        /* CLEAR, CANCEL, CONFIRM */
        }
        return -1;
    }
    if (e->screen == PT_EC_GROUP || e->screen == PT_EC_LETTER) {
        n = DevEcCells(e);
        for (int i = 0; i < n + 2; ++i) {
            CtrPokeTouchRect r = i < n ? DevEcCellKey(e, i) : DevEcToggle(i - n);

            if (DevOnKey(&r, x, y))
                return PT_EC_HIT_CELL + (i < n ? i : i == n ? PT_EC_CELL_TOGGLE : PT_EC_CELL_BACK);
        }
        return -1;
    }
    for (int k = 0; k < 2 * PT_EC_ROWS; ++k) {
        CtrPokeTouchRect r = DevEcRow(k);

        if (DevEcRowOn(e, k) && UiIn(&r, x, y))
            return PT_EC_HIT_ROW + k;
    }
    if (DevOnKey(&kDevEcUp, x, y))
        return PT_EC_HIT_UP;
    if (DevOnKey(&kDevEcDown, x, y))
        return PT_EC_HIT_DOWN;
    if (UiIn(&kDevEcRail, x, y))
        return PT_EC_HIT_RAIL;
    if (!e->alpha && UiIn(&kDevEcStrip, x, y))
        return PT_EC_HIT_STRIP;
    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect r = DevEcListKey(i);

        if (DevOnKey(&r, x, y))
            return kDevEcListCode[i];
    }
    return -1;
}

/* The held code as drawn among [lo, lo + n): its place from lo, or -1 (none of them, or none held). */
static int DevEcHeld(const CtrPokeTouchEasy *e, int lo, int n)
{
    return e->pressed >= lo && e->pressed < lo + n ? e->pressed - lo : -1;
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

/* The toggle's word: to the A-Z order, or (`alpha`, it is on) back to the groups. */
static const char *DevEcToggleWord(int alpha)
{
    return alpha ? CTR_TR("GROUPS", "GRUPOS", "GRUPPEN") : CTR_TR("A TO Z", "DE LA A A LA Z", "VON A BIS Z");
}

/* A key of `look` in r, sunk when `sunk`, with `word`. */
static void DevEcKey(const CtrPokeTouchRect *r, int look, int sunk, const char *word)
{
    int faceY = DevKey(r, look, sunk, 6);

    DevKeyWord(r, faceY, look, sunk, word);
}

/* On screen s's paper, one line centred down it: `left` (FireRed's title, as it writes it) in the paper's ink, `right`
 * small in the secondary ink at its right. */
static void DevEcTitle(const CtrPokeTouchRect *s, const char *left, const char *right)
{
    CtrPokeTouchRect paper = DevScreen(s);
    int top = paper.y + (paper.h - 8) / 2, end = paper.x + paper.w - 6, rw = DevSmallTextWidth(right);

    DevSmallText(right, end - rw, top, DEV_INK2, end);
    DevText(left, paper.x + 6, top, end - rw - 8);
}

/* How many of the phrase's slots hold a word, of how many. */
static void DevEcCount(const CtrPokeTouchEasy *e, char *out, size_t size)
{
    int n = DevEcSlots(e), filled = 0;

    for (int i = 0; i < n; ++i)
        filled += e->chip[i][0] != 0;
    snprintf(out, size, CTR_TR("WORDS %d OF %d", "PALABRAS %d DE %d", "W\xC3\x96RTER %d VON %d"), filled, n);
}

/* The phrase's screen (the title, the count) and its slots: a word in capitals, an empty slot a short rule; the
 * cursor's slot sunk, or the one held. */
static void DevEcPhrase(const CtrPokeTouchEasy *e)
{
    int n = DevEcSlots(e), held = DevEcHeld(e, PT_EC_HIT_CHIP, n);
    char count[32], word[PT_EC_WORD_LEN];

    DevEcCount(e, count, sizeof(count));
    DevEcTitle(&kDevEcPhrase, e->title, count);
    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = DevEcChip(n, i);
        int sunk = DevChosen(i, e->cursor, held), faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);

        if (!e->chip[i][0]) {
            DevRect(r.x + (r.w - 16) / 2, faceY + r.h / 2 - 1, 16, 2, DEV_INK_DIM);
            continue;
        }
        DevUpper(word, e->chip[i], sizeof(word));
        DevKeyWord(&r, faceY, DEV_LOOK_KEY, sunk, word);
    }
}

/* The phrase: its screen and slots, the mode keys (the one wantAlpha names sunk, or the one held) and the buttons
 * (FireRed's cursor's sunk, or the one held; CANCEL dark). */
static void DevEcEdit(const CtrPokeTouchEasy *e)
{
    const char *const kButton[3] = {CTR_TR("CLEAR", "BORRAR", "LEEREN"), DevCancelWord(),
                                    CTR_TR("CONFIRM", "CONFIRMAR", "BEST\xC3\x84TIGEN")};
    int mode = DevEcHeld(e, PT_EC_HIT_MODE_GROUPS, 2), button = DevEcHeld(e, PT_EC_HIT_DELETE, 3);

    DevEcPhrase(e);
    for (int i = 0; i < 2; ++i) {
        CtrPokeTouchRect r = DevEcMode(i);

        DevEcKey(&r, DEV_LOOK_KEY, DevChosen(i, e->wantAlpha != 0, mode),
                 i ? DevEcToggleWord(0) : CTR_TR("BY GROUP", "POR GRUPOS", "NACH GRUPPEN"));
    }
    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect r = DevEcFooter(i);

        DevEcKey(&r, i == 1 ? DEV_LOOK_DARK : DEV_LOOK_KEY, DevChosen(i, e->footer, button), kButton[i]);
    }
}

/* The groups or the letters: the title and the count, a key for each (FireRed's cursor's sunk, or the one held), the
 * toggle and BACK (dark), sunk by FireRed's cursor in its blue box or held. */
static void DevEcGrid(const CtrPokeTouchEasy *e)
{
    int n = DevEcCells(e), letters = e->screen == PT_EC_LETTER, held = DevEcHeld(e, PT_EC_HIT_CELL, PT_EC_CELL_BACK + 1);
    char count[32], word[PT_EC_GROUP_LEN];

    if (held >= n && held != PT_EC_CELL_TOGGLE && held != PT_EC_CELL_BACK)
        held = -1;                                  /* a cell not drawn */
    DevEcCount(e, count, sizeof(count));
    DevEcTitle(&kDevGridTitle, e->title, count);
    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = DevEcCellKey(e, i);

        word[0] = (char)('A' + i);
        word[1] = 0;
        if (!letters)
            DevUpper(word, e->group[i], sizeof(word));
        DevEcKey(&r, DEV_LOOK_KEY, DevChosen(i, e->cell, held), word);
    }
    for (int i = 0; i < 2; ++i) {
        CtrPokeTouchRect r = DevEcToggle(i);

        DevEcKey(&r, i ? DEV_LOOK_DARK : DEV_LOOK_KEY, DevChosen(i ? PT_EC_CELL_BACK : PT_EC_CELL_TOGGLE, e->cell, held),
                 i ? DevBackWord() : DevEcToggleWord(letters));
    }
}

/* `s` in the small font from x in `ink`, cut with "..." to end by `right`. */
static void DevEcSmallLine(const char *s, int x, int top, int right, uint16_t ink)
{
    char cut[PT_EC_WORDS * PT_EC_WORD_LEN + 4];
    const char *p = s;
    int w = 0, dots = DevSmallTextWidth("...");

    if (DevSmallTextWidth(s) > right - x) {
        while (*p && (size_t)(p - s) + 8 < sizeof(cut)) {
            const char *q = p;
            int gw = DevSmallGlyph(&q)->width;

            if (w + gw + dots > right - x)
                break;
            w += gw;
            p = q;
        }
        memcpy(cut, s, (size_t)(p - s));
        memcpy(cut + (p - s), "...", 4);
        s = cut;
    }
    DevSmallText(s, x, top, ink, right);
}

/* The phrase in one line (v0.6.0's preview): its words in capitals with a space between, empty slots left out. */
static void DevEcPhraseLine(const CtrPokeTouchEasy *e, char *out, size_t size)
{
    char line[PT_EC_WORDS * PT_EC_WORD_LEN];
    size_t n = 0;

    for (int i = 0; i < DevEcSlots(e); ++i) {
        const char *w = e->chip[i];

        if (!w[0])
            continue;
        if (n && n + 1 < sizeof(line))
            line[n++] = ' ';
        while (*w && n + 1 < sizeof(line))
            line[n++] = *w++;
    }
    line[n] = 0;
    DevUpper(out, line, size);
}

/* Row place k: inverted when chosen, its word in capitals (cut with "..." where it does not fit). */
static void DevEcRowDraw(const CtrPokeTouchEasy *e, int k, int chosen)
{
    CtrPokeTouchRect r = DevEcRow(k);
    uint16_t ink = DevRowInk(chosen);
    char word[PT_EC_WORD_LEN], cut[PT_EC_WORD_LEN + 4];
    const char *s = word;

    DevRow(&r, chosen);
    DevUpper(word, e->list[k], sizeof(word));
    if (DevTextWidth(word) > r.w - 8) {
        DevCutText(word, r.w - 8, cut, sizeof(cut));
        s = cut;
    }
    DevTextInk(s, r.x + 4, r.y + (r.h - UI_TEXT_H) / 2, r.x + r.w - 4, ink, chosen ? ink : DEV_INK_SH);
}

/* ▲ (dir 0) or ▼ (1): sunk while held; its arrow grey while the list goes no further that way (FireRed's UP and DOWN
 * still move its cursor: the key is taken). */
static void DevEcArrow(const CtrPokeTouchRect *r, int dir, int on, int held)
{
    int faceY = DevKey(r, DEV_LOOK_KEY, held, 6);

    DevTriangle(r->x + r->w / 2, faceY + r->h / 2, dir, on ? DevKeyInk(DEV_LOOK_KEY, held) : DEV_ARROW_OFF);
}

/* The letter strip: A..Z small, the letter of the list's first row boxed; grey under A-Z (not taken there). */
static void DevEcStrip(const CtrPokeTouchEasy *e)
{
    DevScreen(&kDevEcStrip);
    for (int i = 0; i < PT_EC_LETTERS; ++i) {
        CtrPokeTouchRect c = DevEcStripCell(i);
        char one[2] = {(char)('A' + i), 0};
        int lit = !e->alpha && e->letter == i;

        if (lit) {
            DevRect(c.x, c.y, c.w, 1, DEV_INK);
            DevRect(c.x, c.y + c.h - 1, c.w, 1, DEV_INK);
            DevRect(c.x, c.y + 1, 1, c.h - 2, DEV_INK);
            DevRect(c.x + c.w - 1, c.y + 1, 1, c.h - 2, DEV_INK);
        }
        DevSmallText(one, c.x + 1, c.y + 1, lit ? DEV_INK : DEV_INK2, c.x + c.w);
    }
    if (e->alpha)
        DevDim(&kDevEcStrip);
}

/* A list: its title (the group, or the letter with A-Z small at the right) over the phrase; the words (the selected
 * row inverted, or the one held); the rail; ▲ ▼; the strip; USE WORD, the toggle and BACK (dark), sunk while held. */
static void DevEcList(const CtrPokeTouchEasy *e)
{
    static const char *const kAz = "A-Z";
    CtrPokeTouchRect paper = DevScreen(&kDevEcListTitle);
    int end = paper.x + paper.w - 6, rw = e->alpha ? DevSmallTextWidth(kAz) : 0;
    int row = DevEcHeld(e, PT_EC_HIT_ROW, 2 * PT_EC_ROWS);
    int top, h;
    char word[PT_EC_GROUP_LEN], line[PT_EC_WORDS * PT_EC_WORD_LEN];

    if (e->alpha)
        DevSmallText(kAz, end - rw, paper.y + 4, DEV_INK2, end);
    DevUpper(word, e->listTitle, sizeof(word));
    DevText(word, paper.x + 6, paper.y + 4, e->alpha ? end - rw - 8 : end);
    DevEcPhraseLine(e, line, sizeof(line));
    DevEcSmallLine(line, paper.x + 6, paper.y + 18, end, DEV_INK2);
    DevScreen(&kDevEcList);
    if (row >= 0 && !DevEcRowOn(e, row))
        row = -1;                                   /* a row not drawn */
    for (int k = 0; k < 2 * PT_EC_ROWS; ++k)
        if (DevEcRowOn(e, k))
            DevEcRowDraw(e, k, DevChosen(k, e->word - 2 * e->first, row));
    DevEcThumb(e, &top, &h);
    DevRect(kDevEcRail.x + (kDevEcRail.w - 4) / 2, kDevEcRail.y, 4, kDevEcRail.h, DEV_INK_DIM);
    DevRect(kDevEcRail.x + (kDevEcRail.w - 8) / 2, kDevEcRail.y + top, 8, h, DEV_BEZEL);
    DevEcArrow(&kDevEcUp, 0, e->first > 0, e->pressed == PT_EC_HIT_UP);
    DevEcArrow(&kDevEcDown, 1, e->first + PT_EC_ROWS < DevEcRows(e), e->pressed == PT_EC_HIT_DOWN);
    DevEcStrip(e);
    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect r = DevEcListKey(i);

        DevEcKey(&r, i == 2 ? DEV_LOOK_DARK : DEV_LOOK_KEY, e->pressed == kDevEcListCode[i],
                 i == 0 ? CTR_TR("USE WORD", "PONER PALABRA", "WORT SETZEN") : i == 1 ? DevEcToggleWord(e->alpha) : DevBackWord());
    }
}

/* A question over the phrase: its screen and slots grey; FireRed's question (its two lines as one text, as it writes
 * it) on the question's note, v0.6.0's line after QUIT's (FireRed's has no second line; the port's words, in capitals)
 * in the secondary ink, all centred down the paper; YES and NO, FireRed's cursor's sunk, or the one held. */
static void DevEcAsk(const CtrPokeTouchEasy *e)
{
    const char *after = !e->ask2[0] && e->ask == PT_EC_ASK_QUIT
        ? CTR_TR("YOUR CHANGES WILL BE LOST.", "SE PERDER\xC3\x81N LOS CAMBIOS.", "\xC3\x84NDERUNGEN GEHEN VERLOREN.") : "";
    int n = DevEcSlots(e), held = DevEcHeld(e, PT_EC_HIT_YES, 2), width, a, b, top;
    char question[2 * PT_EC_ASK_LEN], word[24];
    CtrPokeTouchRect paper;

    DevEcPhrase(e);
    DevDim(&kDevEcPhrase);
    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = DevEcChip(n, i);

        DevDim(&r);
    }
    paper = DevScreen(&kDevAskNote);
    width = paper.w - 12;
    snprintf(question, sizeof(question), "%s%s%s", e->ask1, e->ask1[0] && e->ask2[0] ? " " : "", e->ask2);
    a = DevWrapLines(question, width, 3);
    b = DevWrapLines(after, width, 4 - a);
    top = paper.y + (paper.h - (8 + DEV_LINE_H * (a + b - 1))) / 2;
    DevWrap(question, paper.x + 6, top, width, a, DEV_INK, DEV_INK_SH);
    DevWrap(after, paper.x + 6, top + DEV_LINE_H * a, width, b, DEV_INK2, DEV_INK2);
    for (int i = 0; i < 2; ++i) {
        DevUpper(word, CtrUiText(i ? UIS_NO : UIS_YES), sizeof(word));
        DevEcKey(i ? &kDevAskNo : &kDevAskYes, DEV_LOOK_KEY, DevChosen(i, e->askCursor, held), word);
    }
}

/* Easy Chat whole (after DevBegin: the dispatcher's). */
static void DevEcDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchEasy *e = &v->ec;

    DevShell();
    if (e->ask)
        DevEcAsk(e);
    else if (e->screen == PT_EC_GROUP || e->screen == PT_EC_LETTER)
        DevEcGrid(e);
    else if (e->screen == PT_EC_LIST)
        DevEcList(e);
    else
        DevEcEdit(e);
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing Easy Chat draws differs between `a` and `b` (the shell is the dispatcher's): the Easy Chat view as filled. */
static int DevEcSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return !memcmp(&a->ec, &b->ec, sizeof(a->ec));
}

/* No BACK of the view's: Easy Chat's BACK, CANCEL and NO are its own keys (FireRed's B). */
static const CtrPokeTouchRect *DevEcBackKey(const CtrPokeTouchView *v)
{
    (void)v;
    return NULL;
}
