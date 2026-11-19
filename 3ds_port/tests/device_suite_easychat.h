/* ---- easychat: Easy Chat as the red device, through the field dispatcher (plan 3 Task 7; the phrase's title and its
 * footer CLEAR / CONFIRM / CANCEL as the Definitiva's def_easychat_edit, the groups' window as its def_easychat_groups,
 * the word list as its def_easychat_list) -----------------------------------------------------------------------------
 * Included by device_render_test.c before its kSuites. Port-written words stand in for FireRed's: its words and groups
 * in capitals, as the game has them, its title and question as it writes them. */

/* The papers it draws on (each screen's paper is 6 px in): the phrase's, the groups' title's, the list title's (its two
 * lines: the title from y 16, the phrase from y 30), the groups' footer note's and the question's note's; the rail's
 * track (4 px from x 227) and its thumb (8 px from x 225). */
static const CtrPokeTouchRect kEcPhrasePaper = {14, 14, 292, 24}, kEcTitlePaper = {14, 12, 292, 22};
static const CtrPokeTouchRect kEcListLine1 = {14, 12, 292, 13}, kEcListLine2 = {14, 26, 292, 14};
static const CtrPokeTouchRect kEcNotePaper = {14, 196, 216, 30}, kEcAskPaper = {14, 162, 188, 64};
#define EC_TRACK_X 227
#define EC_THUMB_X 225
#define EC_DIM(c) DevBlend(DEV_SHELL_RED, (c), DEV_A_DIM)    /* a pixel under DevDim, on the red shell */

static int EcHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_EasyHit((const CtrPokeTouchView *)view, x, y);
}

/* Pixels of colour c in key r's area (with its drop). */
static int EcKeyCount(CtrPokeTouchRect r, uint16_t c)
{
    return Count(r.x, r.y, r.w, r.h + DEV_DROP, c);
}

static const char *const kEcWords[2 * PT_EC_ROWS] = {"ACORN", "AMBER", "BADGE", "BELL", "CABIN", "CANDY", "DAWN",
                                                      "DUNE", "EMBER", "FERN", "GALE", "HILL"};
static const char *const kEcGroups[PT_EC_GROUPS] = {
    "POK\xC3\xA9MON", "STONES", "RIVERS", "CLOUDS", "EMBERS", "FIELDS", "WAVES", "PEAKS", "TRAILS", "WOODS", "LAKES",
    "SANDS", "WINDS", "STARS", "ROOTS", "SEEDS", "SHELLS", "CAVES", "BELLS", "KEYS", "MAPS"};

/* Easy Chat as 3ds_bottom.c gives it (EasyFill, EasySnapshot): a phrase of nine slots with four words, FireRed's title
 * as it writes it, its cursor on the first slot, nothing held; the groups, a list of sixty words (the first twelve
 * shown) and its title filled for the screens the suite turns to. No viewport, no BACK of the view's. */
static CtrPokeTouchView EcBoard(void)
{
    static const char *const kPhrase[PT_EC_WORDS] = {"HELLO", "KIT", "", "EMBER", "", "", "STONES", "", ""};
    CtrPokeTouchView v;
    CtrPokeTouchEasy *e = &v.ec;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    e->on = 1;
    e->screen = PT_EC_EDIT;
    e->numWords = PT_EC_WORDS;
    e->cursor = 0;
    e->footer = e->cell = -1;
    e->pressed = -1;
    e->groupCount = PT_EC_GROUPS;
    e->listCount = 60;
    strcpy(e->title, "Port title");
    strcpy(e->listTitle, "STONES");
    for (int i = 0; i < PT_EC_WORDS; ++i)
        strcpy(e->chip[i], kPhrase[i]);
    for (int i = 0; i < PT_EC_GROUPS; ++i)
        strcpy(e->group[i], kEcGroups[i]);
    for (int k = 0; k < 2 * PT_EC_ROWS; ++k)
        strcpy(e->list[k], kEcWords[k]);
    snprintf(v.bar[0], PT_BAR_LEN, "Easy chat");               /* the bar: not drawn by Easy Chat */
    return v;
}

/* A dark key (CANCEL, BACK) up or sunk: its face (the same colour either way) and the LED a sunk key lights. */
static void EcDark(CtrPokeTouchRect r, int sunk)
{
    AssertKey(r, DEV_LOOK_DARK, sunk);
    assert((EcKeyCount(r, DEV_LED) > 0) == sunk);
}

/* The list's rows inverted: `chosen` alone (-1 none), each drawn row with its word. */
static void EcRowsInverted(const CtrPokeTouchView *v, int chosen)
{
    for (int k = 0; k < 2 * PT_EC_ROWS; ++k) {
        CtrPokeTouchRect r = DevEcRow(k);
        int drawn = 2 * v->ec.first + k < v->ec.listCount;

        assert(at(r.x + 1, r.y + 1) == (k == chosen ? DEV_BEZEL : DEV_PAPER));
        assert((Count(r.x, r.y, r.w, r.h, k == chosen ? DEV_PAPER : DEV_INK) > 0) == drawn);
    }
}

/* The rail's thumb from row y0 to y1 (inclusive): the bezel there, the paper beside the track above and below it. */
static void EcThumb(int y0, int y1)
{
    assert(at(EC_THUMB_X, y0) == DEV_BEZEL && at(EC_THUMB_X + 7, y1) == DEV_BEZEL);
    assert(y0 == kDevEcRail.y || at(EC_THUMB_X, y0 - 1) == DEV_PAPER);
    assert(y1 == kDevEcRail.y + kDevEcRail.h - 1 || at(EC_THUMB_X, y1 + 1) == DEV_PAPER);
    assert(at(EC_THUMB_X - 1, y0) == DEV_PAPER && at(EC_THUMB_X + 8, y1) == DEV_PAPER);
}

/* A text the last whole draw told the kit of: from x with its capitals at y, small or not, in `ink`, the bytes it drew
 * `s`. */
static int EcText(int x, int y, int small, uint16_t ink, const char *s)
{
    for (int i = 0; i < gTraceCount; ++i)
        if (gTrace[i].op == DEV_TRACE_TEXT && gTrace[i].x == x && gTrace[i].y == y && gTrace[i].b == small
            && gTrace[i].a == ink && !strcmp(gTrace[i].s, s))
            return 1;
    return 0;
}

/* Key r reads `word` in the last whole draw: in its look's ink, centred on its face, up or sunk. */
static int EcKeyText(CtrPokeTouchRect r, int look, int sunk, const char *word)
{
    int faceY = r.y + (sunk ? DEV_SINK : 0);

    return EcText(r.x + (r.w - DevTextWidth(word)) / 2, faceY + (r.h - UI_TEXT_H) / 2, 0, DevKeyInk(look, sunk), word);
}

/* The phrase's footer key i (DevEcBottom at y 178, 52 tall) reads `word` (EcKeyText). */
static int EcFooterWord(int i, int look, int sunk, const char *word)
{
    return EcKeyText(DevEcBottom(i, 178, 52), look, sunk, word);
}

/* ▲ and ▼: each arrow in the key's ink while the list goes that way, grey otherwise (25 px, DevTriangle's). */
static void EcArrows(int up, int down)
{
    assert(EcKeyCount(kDevEcUp, up ? DEV_KEY_INK : DEV_ARROW_OFF) == 25);
    assert(EcKeyCount(kDevEcDown, down ? DEV_KEY_INK : DEV_ARROW_OFF) == 25);
}

/* The groups' ▲ and ▼ (the grid's column): each arrow in the key's ink while FireRed's cursor goes that way, grey
 * otherwise. */
static void EcGridArrows(int up, int down)
{
    assert(EcKeyCount(kDevGridUp, up ? DEV_KEY_INK : DEV_ARROW_OFF) == 25);
    assert(EcKeyCount(kDevGridDown, down ? DEV_KEY_INK : DEV_ARROW_OFF) == 25);
}

/* Group place r reads `group` in capitals in the last whole draw: small, centred across it, capitals faceY + 9, in the
 * key's ink up or sunk. */
static int EcCellWord(CtrPokeTouchRect r, int sunk, const char *group)
{
    char word[PT_EC_GROUP_LEN];

    DevUpper(word, group, sizeof(word));
    return EcText(r.x + (r.w - DevSmallTextWidth(word)) / 2, r.y + (sunk ? DEV_SINK : 0) + 9, 1,
                  DevKeyInk(DEV_LOOK_KEY, sunk), word);
}

/* Words (decision 5), with small letters wider than capitals: the phrase's words, the groups and the list's words and
 * title (FireRed's ALL CAPS, which v0.6.0 title-cased) are drawn in capitals however they arrive, POKeMON's e small;
 * FireRed's title and question are drawn as it writes them (each drawn differently in capitals). */
static void EcWords(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    strcpy(cased.ec.title, "PORT TITLE");
    TouchSamePixels(&cased, &v, 0, "FireRed's title, as it writes it");
    cased = v; strcpy(cased.ec.chip[0], "Hello"); strcpy(cased.ec.chip[6], "Stones");
    TouchSamePixels(&cased, &v, 1, "the phrase's words, in capitals");
    v.ec.screen = PT_EC_GROUP; cased = v;
    strcpy(cased.ec.group[0], "Pok\xC3\xA9mon"); strcpy(cased.ec.group[1], "Stones");
    TouchSamePixels(&cased, &v, 1, "the groups, in capitals");
    v.ec.screen = PT_EC_LIST; cased = v;
    strcpy(cased.ec.list[0], "Acorn"); strcpy(cased.ec.list[11], "Hill"); strcpy(cased.ec.listTitle, "Stones");
    TouchSamePixels(&cased, &v, 1, "the list's words and title, in capitals");
    v.ec.screen = PT_EC_EDIT; v.ec.ask = PT_EC_ASK_DELETE; strcpy(v.ec.ask1, "Port words for"); strcpy(v.ec.ask2, "this test?");
    cased = v; strcpy(cased.ec.ask1, "PORT WORDS FOR"); strcpy(cased.ec.ask2, "THIS TEST?");
    TouchSamePixels(&cased, &v, 0, "the question, as FireRed writes it");
    v.ec.screen = PT_EC_GROUP; v.ec.ask = 0; cased = v; strcpy(cased.ec.title, "PORT TITLE");
    TouchSamePixels(&cased, &v, 1, "the groups' title strip (the phrase: FireRed's title not drawn)");
    cased = v; strcpy(cased.ec.chip[0], "Hello"); strcpy(cased.ec.chip[1], "Kit");
    TouchSamePixels(&cased, &v, 1, "the groups' phrase, in capitals");
}

/* The plan's suite (Review Focus 4: the list at its first and last rows, the rail and ▲ ▼ at both ends, the last row's
 * words hit and nothing after them; Review Focus 3: the groups' window with 21 groups and with 14; the phrase of nine
 * slots with four words, the letters, a 5-word list, A-Z on, the question), then each state's pieces, the words, the
 * ways in and out and the states for the eye. */
static void SuiteEasyChat(void)
{
    static const int kCode[3] = {PT_EC_HIT_DELETE, PT_EC_HIT_CONFIRM, PT_EC_HIT_CANCEL};   /* the footer's, as drawn */
    static const int kList[3] = {PT_EC_HIT_PLACE, PT_EC_HIT_TOGGLE, PT_EC_HIT_BACK};       /* the list's, as drawn */
    const char *const kClear = CTR_TR("CLEAR", "BORRAR", "LEEREN"), *const kConfirm = CTR_TR("CONFIRM", "CONFIRMAR", "BEST\xC3\x84TIGEN");
    const char *const kCount = CTR_TR("WORDS 4 OF 9", "PALABRAS 4 DE 9", "W\xC3\x96RTER 4 VON 9");
    const char *const kChooseGroup = CTR_TR("CHOOSE A GROUP.", "ELIGE UN GRUPO.", "W\xC3\x84HLE EINE GRUPPE.");
    const char *const kChooseLetter = CTR_TR("CHOOSE A LETTER.", "ELIGE UNA LETRA.", "W\xC3\x84HLE EINEN BUCHSTABEN.");
    CtrPokeTouchView v = EcBoard(), old = v, other;
    CtrPokeTouchRect r;
    int i, k;

    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevEcShown));
    for (i = 0; i < 9; ++i) AssertHits(EcHitV, &v, DevEcChip(9, i), PT_EC_HIT_CHIP + i);
    assert(CtrPokeTouch_EasyHit(&v, DevEcChip(9, 9).x + 3, 100) == -1);
    AssertHits(EcHitV, &v, DevAcross(2, 0, 136, 30), PT_EC_HIT_MODE_GROUPS); AssertHits(EcHitV, &v, DevAcross(2, 1, 136, 30), PT_EC_HIT_MODE_ALPHA);
    for (i = 0; i < 3; ++i) AssertHits(EcHitV, &v, DevEcBottom(i, 178, 52), kCode[i]);
    AssertKey(DevEcBottom(2, 178, 52), DEV_LOOK_DARK, 0); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 0);
    /* Review Focus 3: 21 groups in a window of five rows of three. FireRed's cursor on group 0: the window's fifteen hit
     * as their groups, ▲ grey and not hit, ▼ hit (to group 3), BACK in the exit corner, the footer read; on group 19
     * with the window from row 2: place 13 is group 19 and sunk, ▼ grey and not hit, ▲ to group 16; the window's row
     * drawn. 14 groups fit one window: ▲ ▼ grey and neither hit. The letters keep FireRed's keys and BACK; the A TO Z
     * key is gone from both */
    v.ec.screen = PT_EC_GROUP; v.ec.groupCount = 21;
    v.ec.cell = 0; DevCheckTouch(&old, &v);
    for (i = 0; i < 15; ++i) AssertHits(EcHitV, &v, DevEcCell(i), PT_EC_HIT_CELL + i);
    assert(EcHitV(&v, 294, 73) == -1 && Count(276, 40, 36, 66, DEV_ARROW_OFF) > 0); AssertHits(EcHitV, &v, kDevGridDown, PT_EC_HIT_DOWN);
    AssertHits(EcHitV, &v, kDevGridExit, PT_EC_HIT_CELL + PT_EC_CELL_BACK); assert(Count(20, 206, 200, 9, DEV_INK) > 0);
    assert(DevEcGroupStep(&v.ec, 1) == 3 && DevEcGroupStep(&v.ec, 0) == -1);
    assert(EcHitV(&v, 20, 210) == -1 && EcHitV(&v, 200, 220) == -1);                             /* no A TO Z */
    v.ec.cell = 19; v.ec.groupFirst = 2; DevCheckTouch(&old, &v);
    AssertHits(EcHitV, &v, DevEcCell(13), PT_EC_HIT_CELL + 19); AssertKey(DevEcCell(13), DEV_LOOK_KEY, 1);
    assert(DevEcGroupStep(&v.ec, 1) == -1 && DevEcGroupStep(&v.ec, 0) == 16 && EcHitV(&v, 294, 147) == -1);
    AssertHits(EcHitV, &v, kDevGridUp, PT_EC_HIT_UP); AssertHits(EcHitV, &v, DevEcCell(0), PT_EC_HIT_CELL + 6);
    v.ec.groupFirst ^= 1; assert(DevWhole(DevCheckTouch(&old, &v)));
    {
        CtrPokeTouchView gv = v, tv = v;

        gv.ec.groupCount = 14; gv.ec.groupFirst = 0; gv.ec.cell = 13; DevCheckTouch(&old, &gv);       /* groups14: one window */
        assert(Count(276, 40, 36, 140, DEV_ARROW_OFF) > 0 && EcHitV(&gv, 294, 73) == -1 && EcHitV(&gv, 294, 147) == -1);
        assert(DevEcGroupStep(&gv.ec, 0) == -1 && DevEcGroupStep(&gv.ec, 1) == -1);
        for (i = 0; i < 14; ++i) AssertHits(EcHitV, &gv, DevEcCell(i), PT_EC_HIT_CELL + i);
        assert(EcHitV(&gv, DevEcCell(14).x + 3, DevEcCell(14).y + 3) == -1);
        tv.ec.screen = PT_EC_LETTER; tv.ec.alpha = 1; tv.ec.groupFirst = 0; tv.ec.cell = 0; DevCheckTouch(&old, &tv);   /* letters */
        for (i = 0; i < PT_EC_LETTERS; ++i) AssertHits(EcHitV, &tv, DevEcLetter(i), PT_EC_HIT_CELL + i);
        AssertHits(EcHitV, &tv, kDevGridExit, PT_EC_HIT_CELL + PT_EC_CELL_BACK);
        assert(EcHitV(&tv, 20, 210) == -1 && EcHitV(&tv, 200, 220) == -1 && EcHitV(&tv, 294, 73) == -1);
    }
    v.ec.screen = PT_EC_LIST; v.ec.alpha = 0; v.ec.listCount = 60; v.ec.first = 0; DevCheckTouch(&old, &v); for (k = 0; k < 12; ++k) AssertRowHits(EcHitV, &v, DevEcRow(k), PT_EC_HIT_ROW + k);
    assert(CtrPokeTouch_EasyRailTarget(&v.ec, 58) == 0 && CtrPokeTouch_EasyRailTarget(&v.ec, 181) == 24);
    AssertRowHits(EcHitV, &v, kDevEcRail, PT_EC_HIT_RAIL);
    /* the list as def_easychat_list: USE WORD, the toggle and BACK at y 196, 34 tall; ▲ grey and not hit at the top, ▼
     * hit, ▲ hit once the list has scrolled; a row's word at capitals r.y + 6; the phrase's empty slot "---", drawn */
    {
        CtrPokeTouchView lv = EcBoard(), lo = old;
        char line[PT_EC_WORDS * PT_EC_WORD_LEN];

        lv.ec.screen = PT_EC_LIST; lv.ec.word = 3; DevCheckTouch(&lo, &lv);
        for (i = 0; i < 3; ++i) AssertHits(EcHitV, &lv, DevEcBottom(i, 196, 34), kList[i]);
        lv.ec.first = 0; DevCheckTouch(&lo, &lv); assert(EcHitV(&lv, 282, 80) == -1 && Count(252, 52, 60, 62, DEV_ARROW_OFF) > 0);
        AssertHits(EcHitV, &lv, kDevEcDown, PT_EC_HIT_DOWN); lv.ec.first = 1; DevCheckTouch(&lo, &lv); AssertHits(EcHitV, &lv, kDevEcUp, PT_EC_HIT_UP);
        r = DevEcRow(0); assert(Count(r.x + 4, r.y + 6, 40, 9, DEV_INK) > 0 && Count(r.x + 4, r.y + 5, 40, 1, DEV_INK) == 0);
        lv.ec.numWords = 4; strcpy(lv.ec.chip[0], "Hello"); strcpy(lv.ec.chip[1], "Great");
        lv.ec.chip[2][0] = 0; strcpy(lv.ec.chip[3], "Ember"); DevEcPhraseLine(&lv.ec, line, sizeof(line)); assert(!strcmp(line, "HELLO GREAT --- EMBER"));
        DevCheckTouch(&lo, &lv); assert(Count(20 + DevSmallTextWidth("HELLO GREAT "), 30, DevSmallTextWidth("---"), 7, DEV_INK2) > 0);   /* drawn */
        assert(EcText(20, 30, 1, DEV_INK2, "HELLO GREAT --- EMBER"));
        old = lo;
    }
    v.ec.first = 24; DevCheckTouch(&old, &v); AssertRowHits(EcHitV, &v, DevEcRow(11), PT_EC_HIT_ROW + 11);
    AssertHits(EcHitV, &v, kDevEcUp, PT_EC_HIT_UP); assert(EcHitV(&v, 282, 150) == -1);           /* the end: ▼ not hit */
    v.ec.listCount = 5; v.ec.first = 0; DevCheckTouch(&old, &v); assert(CtrPokeTouch_EasyHit(&v, DevEcRow(5).x + 3, DevEcRow(5).y + 3) == -1);
    assert(EcHitV(&v, 282, 80) == -1 && EcHitV(&v, 282, 150) == -1);                             /* one window: neither */
    v.ec.alpha = 1; DevCheckTouch(&old, &v); for (i = 0; i < 3; ++i) AssertHits(EcHitV, &v, DevEcBottom(i, 196, 34), kList[i]);
    v.ec.screen = PT_EC_EDIT; v.ec.ask = PT_EC_ASK_QUIT; DevCheckTouch(&old, &v); AssertHits(EcHitV, &v, kDevAskYes, PT_EC_HIT_YES); AssertHits(EcHitV, &v, kDevAskNo, PT_EC_HIT_NO);
    assert(CtrPokeTouch_EasyHit(&v, DevEcChip(9, 0).x + 3, 60) == -1); v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* Review Focus 4 at the list's last row: a list of 59 (30 rows, the last with one word) at its last first row: that
     * word hit, nothing after it; the thumb at the rail's bottom, ▼ grey and not hit, ▲ hit; the rail monotonic along
     * it */
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.listCount = 59; v.ec.first = 24; v.ec.word = 2 * 24 + 10; DevCheckTouch(&old, &v);
    AssertRowHits(EcHitV, &v, DevEcRow(10), PT_EC_HIT_ROW + 10);
    r = DevEcRow(11); assert(CtrPokeTouch_EasyHit(&v, r.x + 3, r.y + 3) == -1 && CtrPokeTouch_EasyHit(&v, r.x + r.w - 1, r.y + r.h - 1) == -1);
    EcRowsInverted(&v, 10); EcThumb(158, 181); EcArrows(1, 0);
    r = kDevEcDown; assert(EcHitV(&v, r.x, r.y) == -1 && EcHitV(&v, r.x + r.w - 1, r.y + r.h + DEV_DROP - 1) == -1);
    AssertHits(EcHitV, &v, kDevEcUp, PT_EC_HIT_UP);
    assert(CtrPokeTouch_EasyRailTarget(&v.ec, 0) == 0 && CtrPokeTouch_EasyRailTarget(&v.ec, 239) == 24);
    {
        int prev = -1;

        for (int y = 40; y < 200; ++y) { int t = CtrPokeTouch_EasyRailTarget(&v.ec, y); assert(t >= prev && t >= 0 && t <= 24); prev = t; }
    }

    /* the phrase: the screen's title (FireRed's, capitals 22 from x 20) and count (small, capitals 23, ending at 298:
     * DevTitleLine); on each slot its word, an empty one a short rule; FireRed's cursor's slot sunk, or the one held
     * (among the slots: a held button leaves it); the slots of every phrase size hit where drawn, no slot after them */
    v = EcBoard(); DevCheckTouch(&old, &v);
    assert(CountIn(kEcPhrasePaper, DEV_INK) > 0 && CountIn(kEcPhrasePaper, DEV_INK2) > 0);
    assert(Count(250, 23, 48, 7, DEV_INK2) > 0 && Count(299, 14, 7, 24, DEV_INK2) == 0);
    assert(EcText(20, 22, 0, DEV_INK, "Port title") && EcText(298 - DevSmallTextWidth(kCount), 23, 1, DEV_INK2, kCount));
    for (i = 0; i < 9; ++i) {
        r = DevEcChip(9, i);
        AssertKey(r, DEV_LOOK_KEY, i == 0);
        assert(EcKeyCount(r, DEV_INK_DIM) == (v.ec.chip[i][0] ? 0 : 16 * 2));
        assert((EcKeyCount(r, i ? DEV_KEY_INK : DEV_SUNK_INK) > 0) == (v.ec.chip[i][0] != 0));
    }
    v.ec.pressed = PT_EC_HIT_CHIP + 5; DevCheckTouch(&old, &v); AssertKey(DevEcChip(9, 5), DEV_LOOK_KEY, 1); AssertKey(DevEcChip(9, 0), DEV_LOOK_KEY, 0);
    /* the buttons as def_easychat_edit: CLEAR, CONFIRM in the middle, CANCEL dark in the exit corner, each word centred
     * on its key; FireRed's footer cursor (DELETE ALL, QUIT, OK: the D-pad walks that order, left, the corner, the
     * middle) sinks its place, a held button its own (the cursor's then up) */
    v.ec.pressed = -1; DevCheckTouch(&old, &v);
    assert(EcFooterWord(0, DEV_LOOK_KEY, 0, kClear) && EcFooterWord(1, DEV_LOOK_KEY, 0, kConfirm)
           && EcFooterWord(2, DEV_LOOK_DARK, 0, DevCancelWord()));
    AssertKey(DevEcBottom(2, 178, 52), DEV_LOOK_DARK, 0); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 0);
    v.ec.footer = 1; DevCheckTouch(&old, &v); AssertKey(DevEcBottom(2, 178, 52), DEV_LOOK_DARK, 1);      /* QUIT: the corner */
    v.ec.footer = 2; DevCheckTouch(&old, &v); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 1);       /* OK: the middle */
    v.ec.footer = -1;
    v.ec.pressed = PT_EC_HIT_CONFIRM; DevCheckTouch(&old, &v); AssertKey(DevEcChip(9, 0), DEV_LOOK_KEY, 1); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 1);
    AssertKey(DevEcBottom(0, 178, 52), DEV_LOOK_KEY, 0); EcDark(DevEcBottom(2, 178, 52), 0);
    assert(EcFooterWord(1, DEV_LOOK_KEY, 1, kConfirm));
    v.ec.pressed = -1; v.ec.cursor = -1; v.ec.footer = 1; DevCheckTouch(&old, &v);                       /* QUIT: the corner */
    AssertKey(DevEcBottom(0, 178, 52), DEV_LOOK_KEY, 0); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 0); EcDark(DevEcBottom(2, 178, 52), 1);
    assert(EcFooterWord(2, DEV_LOOK_DARK, 1, DevCancelWord()));
    for (i = 0; i < 9; ++i) AssertKey(DevEcChip(9, i), DEV_LOOK_KEY, 0);
    v.ec.footer = 2; DevCheckTouch(&old, &v);                                                             /* OK: the middle */
    AssertKey(DevEcBottom(0, 178, 52), DEV_LOOK_KEY, 0); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 1); EcDark(DevEcBottom(2, 178, 52), 0);
    v.ec.footer = 0; DevCheckTouch(&old, &v);                                                             /* DELETE ALL: the left */
    AssertKey(DevEcBottom(0, 178, 52), DEV_LOOK_KEY, 1); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 0); EcDark(DevEcBottom(2, 178, 52), 0);
    assert(EcFooterWord(0, DEV_LOOK_KEY, 1, kClear));
    v.ec.pressed = PT_EC_HIT_CANCEL; DevCheckTouch(&old, &v); EcDark(DevEcBottom(2, 178, 52), 1); AssertKey(DevEcBottom(0, 178, 52), DEV_LOOK_KEY, 0);
    v.ec.footer = 1; v.ec.pressed = PT_EC_HIT_DELETE; DevCheckTouch(&old, &v);
    AssertKey(DevEcBottom(0, 178, 52), DEV_LOOK_KEY, 1); EcDark(DevEcBottom(2, 178, 52), 0);
    /* the modes: the one wantAlpha names sunk, or the one held */
    v.ec.pressed = -1; DevCheckTouch(&old, &v); AssertKey(DevEcMode(0), DEV_LOOK_KEY, 1); AssertKey(DevEcMode(1), DEV_LOOK_KEY, 0);
    v.ec.wantAlpha = 1; DevCheckTouch(&old, &v); AssertKey(DevEcMode(0), DEV_LOOK_KEY, 0); AssertKey(DevEcMode(1), DEV_LOOK_KEY, 1);
    v.ec.pressed = PT_EC_HIT_MODE_GROUPS; DevCheckTouch(&old, &v); AssertKey(DevEcMode(0), DEV_LOOK_KEY, 1); AssertKey(DevEcMode(1), DEV_LOOK_KEY, 0);
    v.ec.pressed = -1;
    for (int n = 1; n <= PT_EC_WORDS; ++n) {
        v.ec.numWords = (uint8_t)n; v.ec.cursor = (int8_t)(n - 1); DevCheckTouch(&old, &v);
        for (i = 0; i < n; ++i) { AssertHits(EcHitV, &v, DevEcChip(n, i), PT_EC_HIT_CHIP + i); AssertKey(DevEcChip(n, i), DEV_LOOK_KEY, i == n - 1); }
        if (n % 2) { r = DevEcChip(n, n); assert(at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED && CtrPokeTouch_EasyHit(&v, r.x + 3, r.y + 3) == -1); }
    }

    /* the groups (def_easychat_groups): the title strip holds the phrase in one line (capitals 16 from x 20, an empty
     * slot "---", cut with "..." before the count and not counted; FireRed's title not drawn) and the count (small,
     * capitals 17, ending at 298: DevTitleLine); the window's groups as keys three across, each name small and centred
     * at capitals faceY + 9, FireRed's cursor's sunk (a group; FireRed's blue box's CANCEL sinks BACK, its A-Z nothing)
     * or the one held (one not shown: as none); ▲ ▼ grey where FireRed's cursor goes no further (and then neither hit
     * nor sunk), sunk while held; the footer note's CHOOSE A GROUP. (capitals 207) and BACK dark in the exit corner;
     * past the groups nothing drawn or hit */
    v = EcBoard(); v.ec.screen = PT_EC_GROUP; v.ec.cell = 5; DevCheckTouch(&old, &v);
    for (i = 0; i < 15; ++i) { r = DevEcCell(i); AssertKey(r, DEV_LOOK_KEY, i == 5); assert(EcCellWord(r, i == 5, kEcGroups[i])); }
    r = DevEcCell(1); assert(Count(r.x, r.y + 9, r.w, 7, DEV_KEY_INK) > 0 && Count(r.x, r.y + 8, r.w, 1, DEV_KEY_INK) == 0);
    assert(LedAt(DevEcCell(5)));
    other = v; strcpy(other.ec.group[0], "CONDITIONS"); strcpy(other.ec.group[1], "ADJECTIVES");   /* FireRed's widest */
    strcpy(other.ec.group[2], "GREETINGS"); strcpy(other.ec.group[3], "POK\xC3\xA9MON2"); DevCheckTouch(&old, &other);
    assert(sDevKeyCut == 0 && EcCellWord(DevEcCell(0), 0, "CONDITIONS") && EcCellWord(DevEcCell(1), 0, "ADJECTIVES"));
    DevCheckTouch(&old, &v);
    EcGridArrows(1, 1); EcDark(kDevGridExit, 0); assert(EcKeyText(kDevGridExit, DEV_LOOK_DARK, 0, DevBackWord()));
    assert(CountIn(kEcNotePaper, DEV_INK) > 0 && EcText(20, 207, 0, DEV_INK, kChooseGroup));
    assert(CountIn(kEcTitlePaper, DEV_INK) > 0 && CountIn(kEcTitlePaper, DEV_INK2) > 0);
    assert(EcText(298 - DevSmallTextWidth(kCount), 17, 1, DEV_INK2, kCount));                         /* DevTitleLine */
    assert(Count(298 - DevSmallTextWidth(kCount) - 8, 12, 8, 16, DEV_INK) == 0);   /* the phrase cut before the count */
    {
        CtrPokeTouchView pv = v;
        char line[PT_EC_WORDS * PT_EC_WORD_LEN];

        pv.ec.numWords = 4; strcpy(pv.ec.chip[0], "Hello"); strcpy(pv.ec.chip[1], "Great"); pv.ec.chip[2][0] = 0;
        strcpy(pv.ec.chip[3], "Ember"); DevCheckTouch(&old, &pv);
        DevEcPhraseLine(&pv.ec, line, sizeof(line)); assert(!strcmp(line, "HELLO GREAT --- EMBER"));
        assert(EcText(20, 16, 0, DEV_INK, "HELLO GREAT --- EMBER"));
        assert(Count(20 + DevTextWidth("HELLO GREAT "), 16, DevTextWidth("---"), 8, DEV_INK) > 0);   /* "---" drawn */
        assert(!EcText(20, 16, 0, DEV_INK, "Port title"));
    }
    v.ec.cell = PT_EC_CELL_BACK; DevCheckTouch(&old, &v); EcDark(kDevGridExit, 1); AssertKey(DevEcCell(5), DEV_LOOK_KEY, 0);
    v.ec.cell = PT_EC_CELL_TOGGLE; DevCheckTouch(&old, &v); EcDark(kDevGridExit, 0); EcGridArrows(0, 1);
    for (i = 0; i < 15; ++i) AssertKey(DevEcCell(i), DEV_LOOK_KEY, 0);
    v.ec.cell = PT_EC_CELL_BACK; v.ec.pressed = PT_EC_HIT_CELL + 9; DevCheckTouch(&old, &v); AssertKey(DevEcCell(9), DEV_LOOK_KEY, 1); EcDark(kDevGridExit, 0);
    v.ec.pressed = PT_EC_HIT_CELL + 17; v.ec.cell = 13; DevCheckTouch(&old, &v); AssertKey(DevEcCell(13), DEV_LOOK_KEY, 1);   /* not shown */
    v.ec.pressed = PT_EC_HIT_CELL + PT_EC_CELL_TOGGLE; DevCheckTouch(&old, &v); AssertKey(DevEcCell(13), DEV_LOOK_KEY, 1);  /* not drawn */
    v.ec.pressed = PT_EC_HIT_CELL + PT_EC_CELL_BACK; DevCheckTouch(&old, &v); EcDark(kDevGridExit, 1); AssertKey(DevEcCell(13), DEV_LOOK_KEY, 0);
    /* ▲ ▼ with FireRed's cursor in the first row: ▲ grey, a held ▲ stays up; a held ▼ sinks with its LED */
    v.ec.pressed = -1; v.ec.cell = 1; DevCheckTouch(&old, &v); EcGridArrows(0, 1);
    v.ec.pressed = PT_EC_HIT_UP; DevCheckTouch(&old, &v); AssertKey(kDevGridUp, DEV_LOOK_KEY, 0); AssertKey(DevEcCell(1), DEV_LOOK_KEY, 1);
    assert(EcKeyCount(kDevGridUp, DEV_ARROW_OFF) == 25 && EcKeyCount(kDevGridUp, DEV_LED) == 0);
    v.ec.pressed = PT_EC_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevGridDown, DEV_LOOK_KEY, 1); assert(EcKeyCount(kDevGridDown, DEV_SUNK_INK) == 25);
    assert(LedAt(kDevGridDown)); AssertKey(DevEcCell(1), DEV_LOOK_KEY, 1);
    /* in the middle both; at the last row (the window from row 2: groups 6-20) ▼ grey and not hit, a held ▼ up */
    v.ec.pressed = -1; v.ec.cell = 10; DevCheckTouch(&old, &v); EcGridArrows(1, 1);
    AssertHits(EcHitV, &v, kDevGridUp, PT_EC_HIT_UP); AssertHits(EcHitV, &v, kDevGridDown, PT_EC_HIT_DOWN);
    v.ec.cell = 20; v.ec.groupFirst = 2; DevCheckTouch(&old, &v); EcGridArrows(1, 0);
    for (i = 0; i < 15; ++i) { r = DevEcCell(i); AssertKey(r, DEV_LOOK_KEY, i == 14); assert(EcCellWord(r, i == 14, kEcGroups[6 + i])); }
    AssertHits(EcHitV, &v, DevEcCell(14), PT_EC_HIT_CELL + 20); AssertHits(EcHitV, &v, DevEcCell(0), PT_EC_HIT_CELL + 6);
    r = kDevGridDown; assert(EcHitV(&v, r.x, r.y) == -1 && EcHitV(&v, r.x + r.w - 1, r.y + r.h + DEV_DROP - 1) == -1);
    v.ec.pressed = PT_EC_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevGridDown, DEV_LOOK_KEY, 0); EcGridArrows(1, 0);
    /* a short last row: 20 groups, the cursor on group 17 (row 5 of 7): ▼ to group 19, the last */
    v.ec.pressed = -1; v.ec.groupCount = 20; v.ec.cell = 17; assert(DevEcGroupStep(&v.ec, 1) == 19 && DevEcGroupStep(&v.ec, 0) == 14);
    DevCheckTouch(&old, &v); r = DevEcCell(14); assert(at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED && EcHitV(&v, r.x + 3, r.y + 3) == -1);
    /* 14 groups: one window (groups 0-13), both arrows grey and not hit; place 14 neither drawn nor hit */
    v.ec.groupCount = 14; v.ec.groupFirst = 0; v.ec.cell = 13; DevCheckTouch(&old, &v); EcGridArrows(0, 0);
    AssertKey(DevEcCell(13), DEV_LOOK_KEY, 1);
    r = DevEcCell(14); assert(at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED && CtrPokeTouch_EasyHit(&v, r.x + 3, r.y + 3) == -1);
    r = kDevGridUp; assert(EcHitV(&v, r.x + 3, r.y + 3) == -1); r = kDevGridDown; assert(EcHitV(&v, r.x + 3, r.y + 3) == -1);

    /* the letters: A..Z where FireRed's keys lie (no seventh key on the rows of six), FireRed's cursor's sunk (its blue
     * box's A-Z: none); the groups' footer with CHOOSE A LETTER. and BACK; no ▲ ▼ */
    v = EcBoard(); v.ec.screen = PT_EC_LETTER; v.ec.alpha = 1; v.ec.cell = 24; DevCheckTouch(&old, &v);
    for (i = 0; i < 26; ++i) { AssertKey(DevEcLetter(i), DEV_LOOK_KEY, i == 24); assert(EcKeyCount(DevEcLetter(i), i == 24 ? DEV_SUNK_INK : DEV_KEY_INK) > 0); }
    EcDark(kDevGridExit, 0); assert(EcKeyText(kDevGridExit, DEV_LOOK_DARK, 0, DevBackWord()) && EcText(20, 207, 0, DEV_INK, kChooseLetter));
    for (i = 0; i < 2; ++i) { r = DevAcross(7, 6, 40 + 32 * i, 28); assert(at(r.x + 3, r.y + 14) == DEV_SHELL_RED && CtrPokeTouch_EasyHit(&v, r.x + 3, r.y + 3) == -1); }
    assert(Count(276, 40, 36, 144, DEV_ARROW_OFF) == 0);
    v.ec.cell = PT_EC_CELL_TOGGLE; DevCheckTouch(&old, &v); for (i = 0; i < 26; ++i) AssertKey(DevEcLetter(i), DEV_LOOK_KEY, 0);
    EcDark(kDevGridExit, 0);
    v.ec.cell = PT_EC_CELL_BACK; DevCheckTouch(&old, &v); EcDark(kDevGridExit, 1);
    v.ec.pressed = PT_EC_HIT_CELL + 3; DevCheckTouch(&old, &v); AssertKey(DevEcLetter(3), DEV_LOOK_KEY, 1); EcDark(kDevGridExit, 0);
    other = v; other.ec.groupFirst = 1; TouchSamePixels(&other, &v, 1, "the letters, whatever the groups' window");

    /* a list (def_easychat_list): its title (the group, capitals 16 from x 20) over the phrase in one line (small,
     * capitals 30: an empty slot "---"); the words two a row, each from 4 px in at capitals r.y + 6, the selected
     * word's row inverted, or the one held; the rail's thumb where the rows shown are (24 px of 124 for 30 rows: the
     * top, the middle, the bottom); ▲ ▼ grey where the list goes no further (and then neither hit nor sunk), sunk while
     * held; USE WORD, the toggle and BACK (dark, in the exit corner) at y 196, 34 tall, each word centred */
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.word = 3; DevCheckTouch(&old, &v);
    EcRowsInverted(&v, 3);
    assert(CountIn(kEcListLine1, DEV_INK) > 0 && CountIn(kEcListLine1, DEV_INK2) == 0 && CountIn(kEcListLine2, DEV_INK2) > 0);
    assert(EcText(20, 16, 0, DEV_INK, "STONES") && EcText(20, 30, 1, DEV_INK2, "HELLO KIT --- EMBER --- --- STONES --- ---"));
    for (k = 0; k < 12; ++k) { r = DevEcRow(k); assert(EcText(r.x + 4, r.y + 6, 0, DevRowInk(k == 3), kEcWords[k])); }
    EcThumb(58, 81); assert(at(EC_TRACK_X, 100) == DEV_INK_DIM && at(EC_TRACK_X + 3, 181) == DEV_INK_DIM && at(EC_TRACK_X + 4, 100) == DEV_PAPER);
    EcArrows(0, 1);
    AssertKey(DevEcBottom(0, 196, 34), DEV_LOOK_KEY, 0); AssertKey(DevEcBottom(1, 196, 34), DEV_LOOK_KEY, 0); EcDark(DevEcBottom(2, 196, 34), 0);
    assert(EcKeyText(DevEcBottom(0, 196, 34), DEV_LOOK_KEY, 0, CTR_TR("USE WORD", "PONER PALABRA", "WORT SETZEN"))
           && EcKeyText(DevEcBottom(1, 196, 34), DEV_LOOK_KEY, 0, DevEcToggleWord(0))
           && EcKeyText(DevEcBottom(2, 196, 34), DEV_LOOK_DARK, 0, DevBackWord()));
    assert(DevEcBottom(2, 196, 34).x == kDevGridExit.x && DevEcBottom(2, 196, 34).x + DevEcBottom(2, 196, 34).w == 312);   /* BACK in the corner */
    v.ec.pressed = PT_EC_HIT_ROW + 6; DevCheckTouch(&old, &v); EcRowsInverted(&v, 6);
    v.ec.pressed = PT_EC_HIT_PLACE; DevCheckTouch(&old, &v); EcRowsInverted(&v, 3); AssertKey(DevEcBottom(0, 196, 34), DEV_LOOK_KEY, 1);
    EcDark(DevEcBottom(2, 196, 34), 0); assert(LedAt(DevEcBottom(0, 196, 34)));
    v.ec.pressed = PT_EC_HIT_TOGGLE; DevCheckTouch(&old, &v); AssertKey(DevEcBottom(1, 196, 34), DEV_LOOK_KEY, 1); AssertKey(DevEcBottom(0, 196, 34), DEV_LOOK_KEY, 0);
    v.ec.pressed = PT_EC_HIT_BACK; DevCheckTouch(&old, &v); EcDark(DevEcBottom(2, 196, 34), 1); AssertKey(DevEcBottom(0, 196, 34), DEV_LOOK_KEY, 0);
    v.ec.pressed = PT_EC_HIT_UP; DevCheckTouch(&old, &v); AssertKey(kDevEcUp, DEV_LOOK_KEY, 0); AssertKey(kDevEcDown, DEV_LOOK_KEY, 0);
    assert(EcKeyCount(kDevEcUp, DEV_ARROW_OFF) == 25 && EcKeyCount(kDevEcUp, DEV_LED) == 0);   /* held where it goes no further: up */
    v.ec.pressed = PT_EC_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevEcDown, DEV_LOOK_KEY, 1); assert(EcKeyCount(kDevEcDown, DEV_SUNK_INK) == 25);
    assert(LedAt(kDevEcDown));
    v.ec.pressed = -1; v.ec.first = 12; v.ec.word = 2 * 12; DevCheckTouch(&old, &v); EcThumb(108, 131); EcArrows(1, 1); EcRowsInverted(&v, 0);
    AssertHits(EcHitV, &v, kDevEcUp, PT_EC_HIT_UP); AssertHits(EcHitV, &v, kDevEcDown, PT_EC_HIT_DOWN);
    v.ec.first = 24; v.ec.word = 2 * 24 + 11; DevCheckTouch(&old, &v); EcThumb(158, 181); EcArrows(1, 0); EcRowsInverted(&v, 11);
    v.ec.pressed = PT_EC_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevEcDown, DEV_LOOK_KEY, 0); EcArrows(1, 0);   /* the end: up */
    v.ec.pressed = -1; v.ec.word = 7; DevCheckTouch(&old, &v); EcRowsInverted(&v, -1);           /* the selected word not shown */
    /* A-Z: the letter on the title with A-Z small at its right, the toggle's word back to the groups */
    v.ec.first = 0; v.ec.alpha = 1; strcpy(v.ec.listTitle, "C"); DevCheckTouch(&old, &v);
    assert(CountIn(kEcListLine1, DEV_INK) > 0 && CountIn(kEcListLine1, DEV_INK2) > 0);
    assert(EcKeyText(DevEcBottom(1, 196, 34), DEV_LOOK_KEY, 0, DevEcToggleWord(1)));
    other = v; other.ec.alpha = 0; TouchSamePixels(&other, &v, 0, "the list's toggle in either order");
    /* an empty phrase: "---" for each of its nine slots under the title */
    memset(v.ec.chip, 0, sizeof(v.ec.chip)); DevCheckTouch(&old, &v);
    assert(CountIn(kEcListLine2, DEV_INK2) > 0 && EcText(20, 30, 1, DEV_INK2, "--- --- --- --- --- --- --- --- ---"));
    /* a 5-word list: three rows, the thumb the whole rail, no arrow lit or hit, nothing to scroll */
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.listCount = 5; v.ec.word = 4; DevCheckTouch(&old, &v);
    EcRowsInverted(&v, 4); EcThumb(58, 181); EcArrows(0, 0); assert(CtrPokeTouch_EasyRailTarget(&v.ec, 100) == -1);
    assert(EcHitV(&v, kDevEcUp.x + 3, kDevEcUp.y + 3) == -1 && EcHitV(&v, kDevEcDown.x + 3, kDevEcDown.y + 3) == -1);
    v.ec.pressed = PT_EC_HIT_ROW + 7; DevCheckTouch(&old, &v); EcRowsInverted(&v, 4);   /* a row not drawn: as none */

    /* a question: the phrase's screen and slots grey (not hit), the modes and buttons not drawn; FireRed's question on
     * the note, v0.6.0's line after QUIT's alone; YES and NO, FireRed's cursor's sunk, or the one held */
    v = EcBoard(); v.ec.ask = PT_EC_ASK_QUIT; v.ec.askCursor = 1; strcpy(v.ec.ask1, "Port question?"); DevCheckTouch(&old, &v);
    AssertKey(kDevAskYes, DEV_LOOK_KEY, 0); AssertKey(kDevAskNo, DEV_LOOK_KEY, 1);
    r = DevEcChip(9, 1); assert(at(r.x + 3, r.y + r.h / 2) == EC_DIM(DEV_KEY));
    r = DevEcChip(9, 0); assert(at(r.x + 3, r.y + DEV_SINK + r.h / 2) == EC_DIM(DEV_SUNK));
    assert(at(kEcPhrasePaper.x, kEcPhrasePaper.y) == EC_DIM(DEV_PAPER) && at(11, 140) == DEV_SHELL_RED && at(180, 140) == DEV_SHELL_RED);
    assert(CountIn(kEcAskPaper, DEV_INK) > 0 && CountIn(kEcAskPaper, DEV_INK2) > 0);
    for (i = 0; i < 9; ++i) { r = DevEcChip(9, i); assert(CtrPokeTouch_EasyHit(&v, r.x + 5, r.y + 5) == -1); }
    for (i = 0; i < 3; ++i) { r = DevEcBottom(i, 178, 52); assert(CtrPokeTouch_EasyHit(&v, r.x + 5, r.y + 5) == (i == 2 ? PT_EC_HIT_YES : -1)); }
    v.ec.pressed = PT_EC_HIT_YES; DevCheckTouch(&old, &v); AssertKey(kDevAskYes, DEV_LOOK_KEY, 1); AssertKey(kDevAskNo, DEV_LOOK_KEY, 0);
    v.ec.pressed = -1; v.ec.ask = PT_EC_ASK_DELETE; strcpy(v.ec.ask1, "Port words for"); strcpy(v.ec.ask2, "this test?"); DevCheckTouch(&old, &v);
    assert(CountIn(kEcAskPaper, DEV_INK) > 0 && CountIn(kEcAskPaper, DEV_INK2) == 0);
    v.ec.ask2[0] = 0; DevCheckTouch(&old, &v); assert(CountIn(kEcAskPaper, DEV_INK2) == 0);   /* QUIT's line is QUIT's */
    v.ec.ask = PT_EC_ASK_CONFIRM; v.ec.askCursor = 0; DevCheckTouch(&old, &v); AssertKey(kDevAskYes, DEV_LOOK_KEY, 1); AssertKey(kDevAskNo, DEV_LOOK_KEY, 0);

    /* no BACK of the view's (Easy Chat's are its keys): nothing hit where a view's BACK is, even with BACK asked for */
    v = EcBoard(); v.backVisible = v.backEnabled = 1; DevCheckTouch(&old, &v);
    assert(!CtrPokeTouch_BackHit(&v, 278, 210) && !CtrPokeTouch_BackHit(&v, 50, 220) && !CtrPokeTouch_BackHit(&v, 20, 20));

    /* what Easy Chat does not draw presents nothing (the bar, the field's state, another view under it: Easy Chat is the
     * dispatcher's first row); the shell's colour whole; the ways in and out (HOME) */
    strcpy(v.bar[0], "Anything"); v.enabled[PT_BAG] = 1; v.playerX = 3; v.backVisible = 0;
    assert(DevCheckTouch(&old, &v).count == 0);
    other = v; other.box.on = other.dex.on = other.choice.on = 1; assert(DevCheckTouch(&old, &other).count == 0);
    DevCheckTouch(&old, &v);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&old, &v);
    other = v; other.ec.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)) && CtrPokeTouch_EasyHit(&other, 30, 60) == -1);
    v = EcBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    EcWords(EcBoard());
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first. */
    v = EcBoard(); v.ec.cursor = 3; DevCheckTouch(&old, &v); shot("edit");
    v.ec.numWords = 4; v.ec.cursor = 1; DevCheckTouch(&old, &v); shot("edit_four");
    v = EcBoard(); v.ec.screen = PT_EC_GROUP; v.ec.cell = 4; DevCheckTouch(&old, &v); shot("groups");
    v.ec.cell = 19; v.ec.groupFirst = 2; DevCheckTouch(&old, &v); shot("groups_end");
    v.ec.groupCount = 14; v.ec.groupFirst = 0; v.ec.cell = 13; DevCheckTouch(&old, &v); shot("groups14");
    v = EcBoard(); v.ec.screen = PT_EC_LETTER; v.ec.alpha = 1; v.ec.cell = 7; DevCheckTouch(&old, &v); shot("letters");
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.word = 3; DevCheckTouch(&old, &v); shot("list");
    v.ec.first = 24; v.ec.word = 2 * 24 + 5; DevCheckTouch(&old, &v); shot("list_end");
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.listCount = 5; v.ec.word = 2; DevCheckTouch(&old, &v); shot("list_short");
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.alpha = 1; v.ec.word = 1; strcpy(v.ec.listTitle, "C");
    DevCheckTouch(&old, &v); shot("list_alpha");
    v = EcBoard(); v.ec.ask = PT_EC_ASK_QUIT; v.ec.askCursor = 1; strcpy(v.ec.ask1, "Port question for this test?");
    DevCheckTouch(&old, &v); shot("ask");
    /* CLEAR's question, FireRed's longest (two lines of 30 and 25 characters, as it writes them): port words of the same
     * lengths, within the note's three lines */
    v.ec.ask = PT_EC_ASK_DELETE; v.ec.askCursor = 1; strcpy(v.ec.ask1, "Port words for this test, they"); strcpy(v.ec.ask2, "run on. Is that all fine?");
    assert(strlen(v.ec.ask1) == 30 && strlen(v.ec.ask2) == 25 && DevWrapLines("Port words for this test, they run on. Is that all fine?", kEcAskPaper.w - 12, 9) <= 3);
    DevCheckTouch(&old, &v); assert(CountIn(kEcAskPaper, DEV_INK) > 0 && CountIn(kEcAskPaper, DEV_INK2) == 0); shot("ask_delete");
    assert(sDevKeyCut == 0);                      /* main prints PASS device easychat */
}
