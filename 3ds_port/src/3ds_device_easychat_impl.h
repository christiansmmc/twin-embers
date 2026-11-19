/* Easy Chat as the red device (plan 3; the phrase's title and footer as the Definitiva's def_easychat_edit, the groups
 * as its def_easychat_groups, a list as its def_easychat_list), included by 3ds_poketouch.c after the PC box: the field
 * dispatcher's first row (3ds_device_field_impl.h), shown while view->ec.on (DevEcShown), the whole lower screen (the
 * upper one keeps FireRed's own). A remote for FireRed's Easy Chat (3ds_poketouch.h CtrPokeTouchEasy), every state of
 * it drawn here:
 *  - the phrase: FireRed's title and how many of its slots hold a word on the phrase's screen (DevTitleLine); the slots
 *    as keys in two rows (the word, an empty slot a short rule); BY GROUP and A TO Z (the order the next word is picked
 *    in, the one wantAlpha names sunk); CLEAR, CONFIRM in the middle and CANCEL dark in the exit corner (FireRed's B).
 *    FireRed's footer cursor keeps its own order (DELETE ALL, QUIT, OK), so the D-pad walks left, the corner, the
 *    middle (kDevEcFooterPlace);
 *  - the groups, or in the A-Z order the letters (FireRed's rows of 6, 6, 7 and 7): the phrase in one line and the
 *    count on the title's screen; the groups' names (small) as keys in a window of five rows of three that follows
 *    FireRed's cursor (groupFirst: 3ds_bottom_easychat_impl.h), ▲ ▼ in the grid's column walking that cursor a row of
 *    three (DevEcGroupStep; grey and not taken where it goes no further, both while the groups fit one window), or
 *    A..Z as keys; the footer's note (what to choose) and BACK (dark, in the exit corner). No key to the other order
 *    here: the phrase's A TO Z and the list's toggle are it;
 *  - a group's or a letter's words: the list's title (the group, or the letter with A-Z) over the phrase in one line
 *    (an empty slot "---"); the words two a row in six rows, the selected inverted; the rail on the list's paper (a 4 px
 *    track, the thumb where the rows shown are: a slider); ▲ ▼ (FireRed's UP and DOWN; at the list's ends, the
 *    legend's: the arrow grey and the key not taken); USE WORD, the toggle and BACK (dark, in the exit corner);
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

/* The first cell shown: the letter A, or the first group of the groups' window (its first row of three). */
static int DevEcCellFirst(const CtrPokeTouchEasy *e)
{
    return e->screen == PT_EC_LETTER ? 0 : 3 * e->groupFirst;
}

/* The cells shown end before this: the 26 letters, or the window's groups (DEV_EC_GROUP_ROWS rows of three from
 * DevEcCellFirst, FireRed's groups and PT_EC_GROUPS at most). */
static int DevEcCells(const CtrPokeTouchEasy *e)
{
    int n = e->groupCount > PT_EC_GROUPS ? PT_EC_GROUPS : e->groupCount;
    int end = DevEcCellFirst(e) + 3 * DEV_EC_GROUP_ROWS;

    if (e->screen == PT_EC_LETTER)
        return PT_EC_LETTERS;
    return n < end ? n : end;
}

/* Cell i's key (one shown): a letter's, or its place in the groups' window. */
static CtrPokeTouchRect DevEcCellKey(const CtrPokeTouchEasy *e, int i)
{
    return e->screen == PT_EC_LETTER ? DevEcLetter(i) : DevEcCell(i - DevEcCellFirst(e));
}

/* The groups' ▲ (`down` 0) or ▼ goes somewhere (each key hit and its arrow lit only then): DevEcGroupStep. */
static int DevEcGroupOn(const CtrPokeTouchEasy *e, int down)
{
    return e->screen == PT_EC_GROUP && DevEcGroupStep(e, down) >= 0;
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

/* ▲ and ▼ go somewhere: the list's window is not at its first row, or not at its last (each key hit and its arrow
 * lit only then). */
static int DevEcUpOn(const CtrPokeTouchEasy *e)
{
    return e->first > 0;
}

static int DevEcDownOn(const CtrPokeTouchEasy *e)
{
    return e->first + PT_EC_ROWS < DevEcRows(e);
}

/* The codes of a list's bottom keys (DevEcBottom at y 196, 34 tall): USE WORD, the toggle, BACK. */
static const int16_t kDevEcListCode[3] = {PT_EC_HIT_PLACE, PT_EC_HIT_TOGGLE, PT_EC_HIT_BACK};

/* The phrase's footer: the code of each drawn place (DevEcBottom: CLEAR, CONFIRM, CANCEL), and the place FireRed's
 * footer column (e->footer: 0 DELETE ALL, 1 QUIT, 2 OK) sinks. */
static const int16_t kDevEcFooterCode[3] = {PT_EC_HIT_DELETE, PT_EC_HIT_CONFIRM, PT_EC_HIT_CANCEL};
static const int8_t kDevEcFooterPlace[3] = {0, 2, 1};

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
            CtrPokeTouchRect r = DevEcBottom(i, 178, 52);

            if (DevOnKey(&r, x, y))
                return kDevEcFooterCode[i];         /* CLEAR, CONFIRM, CANCEL */
        }
        return -1;
    }
    if (e->screen == PT_EC_GROUP || e->screen == PT_EC_LETTER) {
        n = DevEcCells(e);
        for (int i = DevEcCellFirst(e); i < n; ++i) {
            CtrPokeTouchRect r = DevEcCellKey(e, i);

            if (DevOnKey(&r, x, y))
                return PT_EC_HIT_CELL + i;
        }
        if (DevEcGroupOn(e, 0) && DevOnKey(&kDevGridUp, x, y))
            return PT_EC_HIT_UP;
        if (DevEcGroupOn(e, 1) && DevOnKey(&kDevGridDown, x, y))
            return PT_EC_HIT_DOWN;
        return DevOnKey(&kDevGridExit, x, y) ? PT_EC_HIT_CELL + PT_EC_CELL_BACK : -1;
    }
    for (int k = 0; k < 2 * PT_EC_ROWS; ++k) {
        CtrPokeTouchRect r = DevEcRow(k);

        if (DevEcRowOn(e, k) && UiIn(&r, x, y))
            return PT_EC_HIT_ROW + k;
    }
    if (DevEcUpOn(e) && DevOnKey(&kDevEcUp, x, y))
        return PT_EC_HIT_UP;
    if (DevEcDownOn(e) && DevOnKey(&kDevEcDown, x, y))
        return PT_EC_HIT_DOWN;
    if (UiIn(&kDevEcRail, x, y))
        return PT_EC_HIT_RAIL;
    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect r = DevEcBottom(i, 196, 34);

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

/* How many of the phrase's slots hold a word, of how many. */
static void DevEcCount(const CtrPokeTouchEasy *e, char *out, size_t size)
{
    int n = DevEcSlots(e), filled = 0;

    for (int i = 0; i < n; ++i)
        filled += e->chip[i][0] != 0;
    snprintf(out, size, CTR_TR("WORDS %d OF %d", "PALABRAS %d DE %d", "W\xC3\x96RTER %d VON %d"), filled, n);
}

/* The phrase's screen (FireRed's title as it writes it, the count ending at 298: DevTitleLine) and its slots: a word in
 * capitals, an empty slot a short rule; the cursor's slot sunk, or the one held. */
static void DevEcPhrase(const CtrPokeTouchEasy *e)
{
    int n = DevEcSlots(e), held = DevEcHeld(e, PT_EC_HIT_CHIP, n);
    char count[32], word[PT_EC_WORD_LEN];

    DevEcCount(e, count, sizeof(count));
    DevTitleLine(&kDevEcPhrase, e->title, count);
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

/* The footer's place (DevEcBottom) of hit code `code`, or -1 (none of the footer's). */
static int DevEcFooterAt(int code)
{
    for (int i = 0; i < 3; ++i)
        if (kDevEcFooterCode[i] == code)
            return i;
    return -1;
}

/* The phrase: its screen and slots, the mode keys (the one wantAlpha names sunk, or the one held) and the buttons CLEAR,
 * CONFIRM and CANCEL (dark, in the exit corner): the place FireRed's footer cursor is on sunk, or the one held. */
static void DevEcEdit(const CtrPokeTouchEasy *e)
{
    const char *const kButton[3] = {CTR_TR("CLEAR", "BORRAR", "LEEREN"),
                                    CTR_TR("CONFIRM", "CONFIRMAR", "BEST\xC3\x84TIGEN"), DevCancelWord()};
    int mode = DevEcHeld(e, PT_EC_HIT_MODE_GROUPS, 2), button = DevEcFooterAt(e->pressed);
    int place = e->footer >= 0 && e->footer < 3 ? kDevEcFooterPlace[e->footer] : -1;

    DevEcPhrase(e);
    for (int i = 0; i < 2; ++i) {
        CtrPokeTouchRect r = DevEcMode(i);

        DevEcKey(&r, DEV_LOOK_KEY, DevChosen(i, e->wantAlpha != 0, mode),
                 i ? DevEcToggleWord(0) : CTR_TR("BY GROUP", "POR GRUPOS", "NACH GRUPPEN"));
    }
    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect r = DevEcBottom(i, 178, 52);

        DevEcKey(&r, i == 2 ? DEV_LOOK_DARK : DEV_LOOK_KEY, DevChosen(i, place, button), kButton[i]);
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

/* The phrase in one line (v0.6.0's preview): its words in capitals with a space between, an empty slot "---" (the
 * boards' "____": FireRed's font has no '_'; '-' is its 0xAE). */
static void DevEcPhraseLine(const CtrPokeTouchEasy *e, char *out, size_t size)
{
    char line[PT_EC_WORDS * PT_EC_WORD_LEN];
    size_t n = 0;

    for (int i = 0; i < DevEcSlots(e); ++i) {
        const char *w = e->chip[i][0] ? e->chip[i] : "---";

        if (n && n + 1 < sizeof(line))
            line[n++] = ' ';
        while (*w && n + 1 < sizeof(line))
            line[n++] = *w++;
    }
    line[n] = 0;
    DevUpper(out, line, size);
}

/* The groups or the letters (def_easychat_groups): the phrase in one line (an empty slot "---"; cut with "..." before
 * the count, not counted) and the count on the title's screen (DevTitleLine); the window's groups (their names small,
 * capitals faceY + 9) or A..Z as keys, FireRed's cursor's sunk or the one held (one not drawn: as none); for the groups
 * ▲ ▼ (grey where FireRed's cursor goes no further: DevEcGroupStep; sunk while held); the footer's note (what to
 * choose) and BACK (dark, in the exit corner), sunk by FireRed's cursor on its blue box's BACK or held. FireRed's blue
 * box's A-Z (PT_EC_CELL_TOGGLE) is drawn nowhere. */
static void DevEcGrid(const CtrPokeTouchEasy *e)
{
    int lo = DevEcCellFirst(e), n = DevEcCells(e), letters = e->screen == PT_EC_LETTER;
    int held = DevEcHeld(e, PT_EC_HIT_CELL, PT_EC_CELL_BACK + 1);
    char count[32], word[PT_EC_GROUP_LEN], line[PT_EC_WORDS * PT_EC_WORD_LEN];
    CtrPokeTouchRect paper;

    if (held >= 0 && held != PT_EC_CELL_BACK && (held < lo || held >= n))
        held = -1;                                  /* a cell not drawn */
    DevEcCount(e, count, sizeof(count));
    DevEcPhraseLine(e, line, sizeof(line));
    DevTitleLine(&kDevGridTitle, line, count);
    for (int i = lo; i < n; ++i) {
        CtrPokeTouchRect r = DevEcCellKey(e, i);
        int sunk = DevChosen(i, e->cell, held), faceY;

        if (letters) {
            word[0] = (char)('A' + i);
            word[1] = 0;
            DevEcKey(&r, DEV_LOOK_KEY, sunk, word);
            continue;
        }
        DevUpper(word, e->group[i], sizeof(word));
        faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);
        DevKeySmallWord(&r, faceY + 9, DEV_LOOK_KEY, sunk, word);
    }
    if (!letters)
        DevArrowKeys(&kDevGridUp, &kDevGridDown, DevEcGroupOn(e, 0), DevEcGroupOn(e, 1), e->pressed == PT_EC_HIT_UP,
                     e->pressed == PT_EC_HIT_DOWN, 0);
    paper = DevScreen(&kDevGridNote);
    DevReadText(letters ? CTR_TR("CHOOSE A LETTER.", "ELIGE UNA LETRA.", "W\xC3\x84HLE EINEN BUCHSTABEN.")
                        : CTR_TR("CHOOSE A GROUP.", "ELIGE UN GRUPO.", "W\xC3\x84HLE EINE GRUPPE."),
                &paper, 20, 224, paper.y + 2, DEV_SMALL_LINE);
    DevEcKey(&kDevGridExit, DEV_LOOK_DARK, DevChosen(PT_EC_CELL_BACK, e->cell, held), DevBackWord());
}

/* Row place k: inverted when chosen, its word in capitals from 4 px in at capitals r.y + 6 (def_easychat_list's; cut
 * with "..." where it does not fit). */
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
    DevTextInk(s, r.x + 4, r.y + 6, r.x + r.w - 4, ink, chosen ? ink : DEV_INK_SH);
}

/* A list: its title (the group, or the letter with A-Z small at the right) over the phrase; the words (the selected
 * row inverted, or the one held); the rail; ▲ ▼ (grey where the list goes no further, sunk while held); USE WORD,
 * the toggle and BACK (dark), sunk while held. */
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
    DevArrowKeys(&kDevEcUp, &kDevEcDown, DevEcUpOn(e), DevEcDownOn(e), e->pressed == PT_EC_HIT_UP,
                 e->pressed == PT_EC_HIT_DOWN, 0);
    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect r = DevEcBottom(i, 196, 34);

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
