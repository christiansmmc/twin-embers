/* ---- easychat: Easy Chat as the red device, through the field dispatcher (no board; plan 3 Task 7) -----------------
 * Included by device_render_test.c before its kSuites. Port-written words stand in for FireRed's: its words and groups
 * in capitals, as the game has them, its title and question as it writes them. */

/* The papers it draws on (each screen's paper is 6 px in): the phrase's, the groups' title's, the list title's (its two
 * lines: the title from y 16, the phrase from y 30), the list's, the strip's and the question's note's; the rail's track
 * (4 px from x 227) and its thumb (8 px from x 225). */
static const CtrPokeTouchRect kEcPhrasePaper = {14, 14, 292, 24}, kEcTitlePaper = {14, 12, 292, 22};
static const CtrPokeTouchRect kEcListLine1 = {14, 12, 292, 13}, kEcListLine2 = {14, 26, 292, 14};
static const CtrPokeTouchRect kEcStripPaper = {290, 58, 16, 124}, kEcAskPaper = {14, 162, 188, 64};
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
 * shown) and its title filled for the screens the suite turns to; the strip's letter the first word's. No viewport, no
 * BACK of the view's. */
static CtrPokeTouchView EcBoard(void)
{
    static const char *const kPhrase[PT_EC_WORDS] = {"HELLO", "KIT", "", "EMBER", "", "", "STONES", "", ""};
    CtrPokeTouchView v;
    CtrPokeTouchEasy *e = &v.ec;

    memset(&v, 0, sizeof(v));
    v.pressed = v.selected = PT_NONE;
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

/* ▲ and ▼: each arrow in the key's ink while the list goes that way, grey otherwise (25 px, DevTriangle's). */
static void EcArrows(int up, int down)
{
    assert(EcKeyCount(kDevEcUp, up ? DEV_KEY_INK : DEV_BAG_ARROW_OFF) == 25);
    assert(EcKeyCount(kDevEcDown, down ? DEV_KEY_INK : DEV_BAG_ARROW_OFF) == 25);
}

/* Upper case (decision 5): FireRed's title, the phrase's words, the groups, the list's words and title and the question
 * are drawn in capitals however they arrive (FireRed's as it writes them, or v0.6.0's title case): with small letters
 * wider than capitals, each drawn the same as in capitals; POKeMON's e stays small. */
static void EcWords(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    strcpy(cased.ec.title, "PORT TITLE");
    TouchSamePixels(&cased, &v, 1, "FireRed's title, in capitals");
    cased = v; strcpy(cased.ec.chip[0], "Hello"); strcpy(cased.ec.chip[6], "Stones");
    TouchSamePixels(&cased, &v, 1, "the phrase's words, in capitals");
    v.ec.screen = PT_EC_GROUP; cased = v;
    strcpy(cased.ec.group[0], "Pok\xC3\xA9mon"); strcpy(cased.ec.group[1], "Stones");
    TouchSamePixels(&cased, &v, 1, "the groups, in capitals");
    v.ec.screen = PT_EC_LIST; cased = v;
    strcpy(cased.ec.list[0], "Acorn"); strcpy(cased.ec.list[11], "Hill"); strcpy(cased.ec.listTitle, "Stones");
    TouchSamePixels(&cased, &v, 1, "the list's words and title, in capitals");
    v.ec.screen = PT_EC_EDIT; v.ec.ask = PT_EC_ASK_DELETE; strcpy(v.ec.ask1, "PORT WORDS FOR"); strcpy(v.ec.ask2, "THIS TEST?");
    cased = v; strcpy(cased.ec.ask1, "Port words for"); strcpy(cased.ec.ask2, "this test?");
    TouchSamePixels(&cased, &v, 1, "the question, in capitals");
}

/* The plan's suite (Review Focus 4: the list at its first and last rows, the rail and the strip at both ends, the last
 * row's words hit and nothing after them; the phrase of nine slots with four words, the groups of 21 and of 14, the
 * letters, a 5-word list, A-Z on, the question), then each state's pieces, the words, the ways in and out and the
 * states for the eye. */
static void SuiteEasyChat(void)
{
    CtrPokeTouchView v = EcBoard(), old = v, other;
    CtrPokeTouchRect r;
    int i, k;

    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    DevCheckTouch(&old, &v); assert(CtrPokeTouch_DeviceShown(&v));
    for (i = 0; i < 9; ++i) AssertHits(EcHitV, &v, DevEcChip(9, i), PT_EC_HIT_CHIP + i);
    assert(CtrPokeTouch_EasyHit(&v, DevEcChip(9, 9).x + 3, 100) == -1);
    AssertHits(EcHitV, &v, DevAcross(2, 0, 136, 30), PT_EC_HIT_MODE_GROUPS); AssertHits(EcHitV, &v, DevAcross(2, 1, 136, 30), PT_EC_HIT_MODE_ALPHA);
    AssertHits(EcHitV, &v, DevAcross(3, 0, 178, 52), PT_EC_HIT_DELETE); AssertHits(EcHitV, &v, DevAcross(3, 1, 178, 52), PT_EC_HIT_CANCEL); AssertHits(EcHitV, &v, DevAcross(3, 2, 178, 52), PT_EC_HIT_CONFIRM);
    AssertKey(DevAcross(3, 1, 178, 52), DEV_LOOK_DARK, 0);
    v.ec.screen = PT_EC_GROUP; v.ec.groupCount = 21; DevCheckTouch(&old, &v); for (i = 0; i < 21; ++i) AssertHits(EcHitV, &v, DevEcCell(i), PT_EC_HIT_CELL + i);
    AssertHits(EcHitV, &v, DevAcross(2, 0, 206, 26), PT_EC_HIT_CELL + PT_EC_CELL_TOGGLE); AssertHits(EcHitV, &v, DevAcross(2, 1, 206, 26), PT_EC_HIT_CELL + PT_EC_CELL_BACK);
    v.ec.groupCount = 14; DevCheckTouch(&old, &v); assert(CtrPokeTouch_EasyHit(&v, DevEcCell(14).x + 3, DevEcCell(14).y + 3) == -1);
    v.ec.screen = PT_EC_LETTER; v.ec.alpha = 1; DevCheckTouch(&old, &v); for (i = 0; i < 26; ++i) AssertHits(EcHitV, &v, DevEcLetter(i), PT_EC_HIT_CELL + i);
    v.ec.screen = PT_EC_LIST; v.ec.alpha = 0; v.ec.listCount = 60; v.ec.first = 0; DevCheckTouch(&old, &v); for (k = 0; k < 12; ++k) AssertRowHits(EcHitV, &v, DevEcRow(k), PT_EC_HIT_ROW + k);
    assert(CtrPokeTouch_EasyRailTarget(&v.ec, 58) == 0 && CtrPokeTouch_EasyRailTarget(&v.ec, 181) == 24);
    assert(CtrPokeTouch_EasyStripLetter(291, 60) == 0 && CtrPokeTouch_EasyStripLetter(305, 181) == 25 && CtrPokeTouch_EasyStripLetter(311, 187) == 25
           && CtrPokeTouch_EasyStripLetter(284, 52) == 0 && CtrPokeTouch_EasyStripLetter(283, 60) == -1);
    for (i = 0; i < 3; ++i) AssertHits(EcHitV, &v, DevAcross(3, i, 196, 36), i == 0 ? PT_EC_HIT_BACK : i == 1 ? PT_EC_HIT_TOGGLE : PT_EC_HIT_PLACE);
    AssertHits(EcHitV, &v, kDevEcUp, PT_EC_HIT_UP); AssertHits(EcHitV, &v, kDevEcDown, PT_EC_HIT_DOWN); AssertRowHits(EcHitV, &v, kDevEcRail, PT_EC_HIT_RAIL); AssertRowHits(EcHitV, &v, kDevEcStrip, PT_EC_HIT_STRIP);
    v.ec.first = 24; DevCheckTouch(&old, &v); AssertRowHits(EcHitV, &v, DevEcRow(11), PT_EC_HIT_ROW + 11);
    v.ec.listCount = 5; v.ec.first = 0; DevCheckTouch(&old, &v); assert(CtrPokeTouch_EasyHit(&v, DevEcRow(5).x + 3, DevEcRow(5).y + 3) == -1);
    v.ec.alpha = 1; DevCheckTouch(&old, &v); assert(CtrPokeTouch_EasyHit(&v, 300, 100) == -1);
    v.ec.screen = PT_EC_EDIT; v.ec.ask = PT_EC_ASK_QUIT; DevCheckTouch(&old, &v); AssertHits(EcHitV, &v, kDevAskYes, PT_EC_HIT_YES); AssertHits(EcHitV, &v, kDevAskNo, PT_EC_HIT_NO);
    assert(CtrPokeTouch_EasyHit(&v, DevEcChip(9, 0).x + 3, 60) == -1); v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* Review Focus 4 at the list's last row: a list of 59 (30 rows, the last with one word) at its last first row: that
     * word hit, nothing after it; the thumb at the rail's bottom, ▼ grey; the rail and the strip monotonic along them,
     * each letter of the strip taken where it is drawn */
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.listCount = 59; v.ec.first = 24; v.ec.word = 2 * 24 + 10; DevCheckTouch(&old, &v);
    AssertRowHits(EcHitV, &v, DevEcRow(10), PT_EC_HIT_ROW + 10);
    r = DevEcRow(11); assert(CtrPokeTouch_EasyHit(&v, r.x + 3, r.y + 3) == -1 && CtrPokeTouch_EasyHit(&v, r.x + r.w - 1, r.y + r.h - 1) == -1);
    EcRowsInverted(&v, 10); EcThumb(158, 181); EcArrows(1, 0);
    assert(CtrPokeTouch_EasyRailTarget(&v.ec, 0) == 0 && CtrPokeTouch_EasyRailTarget(&v.ec, 239) == 24);
    {
        int prev = -1;

        for (int y = 40; y < 200; ++y) { int t = CtrPokeTouch_EasyRailTarget(&v.ec, y); assert(t >= prev && t >= 0 && t <= 24); prev = t; }
        for (int y = 52; y < 188; ++y) assert(CtrPokeTouch_EasyStripLetter(311, y) == 13 + CtrPokeTouch_EasyStripLetter(284, y));
        prev = -1;
        for (int y = 52; y < 188; ++y) { int l = CtrPokeTouch_EasyStripLetter(291, y); assert(l >= prev && l >= 0 && l <= 12); prev = l; }
    }
    for (i = 0; i < 26; ++i) { r = DevEcStripCell(i); assert(CtrPokeTouch_EasyStripLetter(r.x + 1, r.y + 4) == i && CtrPokeTouch_EasyStripLetter(r.x + r.w - 1, r.y + r.h - 2) == i); }

    /* the phrase: the screen's title and count; on each slot its word, an empty one a short rule; FireRed's cursor's slot
     * sunk, or the one held (among the slots: a held button leaves it); the slots of every phrase size hit where drawn,
     * no slot after them */
    v = EcBoard(); DevCheckTouch(&old, &v);
    assert(CountIn(kEcPhrasePaper, DEV_INK) > 0 && CountIn(kEcPhrasePaper, DEV_INK2) > 0);
    for (i = 0; i < 9; ++i) {
        r = DevEcChip(9, i);
        AssertKey(r, DEV_LOOK_KEY, i == 0);
        assert(EcKeyCount(r, DEV_INK_DIM) == (v.ec.chip[i][0] ? 0 : 16 * 2));
        assert((EcKeyCount(r, i ? DEV_KEY_INK : DEV_SUNK_INK) > 0) == (v.ec.chip[i][0] != 0));
    }
    v.ec.pressed = PT_EC_HIT_CHIP + 5; DevCheckTouch(&old, &v); AssertKey(DevEcChip(9, 5), DEV_LOOK_KEY, 1); AssertKey(DevEcChip(9, 0), DEV_LOOK_KEY, 0);
    v.ec.pressed = PT_EC_HIT_CONFIRM; DevCheckTouch(&old, &v); AssertKey(DevEcChip(9, 0), DEV_LOOK_KEY, 1); AssertKey(DevEcFooter(2), DEV_LOOK_KEY, 1);
    v.ec.pressed = -1; v.ec.cursor = -1; v.ec.footer = 1; DevCheckTouch(&old, &v);
    AssertKey(DevEcFooter(0), DEV_LOOK_KEY, 0); EcDark(DevEcFooter(1), 1); AssertKey(DevEcFooter(2), DEV_LOOK_KEY, 0);
    for (i = 0; i < 9; ++i) AssertKey(DevEcChip(9, i), DEV_LOOK_KEY, 0);
    v.ec.pressed = PT_EC_HIT_DELETE; DevCheckTouch(&old, &v); AssertKey(DevEcFooter(0), DEV_LOOK_KEY, 1); EcDark(DevEcFooter(1), 0);
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

    /* the groups: the title's screen (the title, the count); the groups' names as keys, FireRed's cursor's sunk (a group,
     * the toggle, BACK: dark) or the one held; past the groups nothing drawn or hit */
    v = EcBoard(); v.ec.screen = PT_EC_GROUP; v.ec.cell = 5; DevCheckTouch(&old, &v);
    for (i = 0; i < 21; ++i) { AssertKey(DevEcCell(i), DEV_LOOK_KEY, i == 5); assert(EcKeyCount(DevEcCell(i), i == 5 ? DEV_SUNK_INK : DEV_KEY_INK) > 0); }
    AssertKey(DevEcToggle(0), DEV_LOOK_KEY, 0); EcDark(DevEcToggle(1), 0);
    assert(CountIn(kEcTitlePaper, DEV_INK) > 0 && CountIn(kEcTitlePaper, DEV_INK2) > 0);
    v.ec.cell = PT_EC_CELL_TOGGLE; DevCheckTouch(&old, &v); AssertKey(DevEcToggle(0), DEV_LOOK_KEY, 1); AssertKey(DevEcCell(5), DEV_LOOK_KEY, 0);
    v.ec.cell = PT_EC_CELL_BACK; DevCheckTouch(&old, &v); EcDark(DevEcToggle(1), 1); AssertKey(DevEcToggle(0), DEV_LOOK_KEY, 0);
    v.ec.pressed = PT_EC_HIT_CELL + 9; DevCheckTouch(&old, &v); AssertKey(DevEcCell(9), DEV_LOOK_KEY, 1); EcDark(DevEcToggle(1), 0);
    v.ec.pressed = PT_EC_HIT_CELL + 17; v.ec.groupCount = 14; v.ec.cell = 13; DevCheckTouch(&old, &v);  /* a group not drawn: as none */
    AssertKey(DevEcCell(13), DEV_LOOK_KEY, 1);
    for (i = 14; i < 21; ++i) { r = DevEcCell(i); assert(at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED && CtrPokeTouch_EasyHit(&v, r.x + 3, r.y + 3) == -1); }

    /* the letters: A..Z where FireRed's keys lie (no seventh key on the rows of six); Y and Z letters, the toggle and BACK
     * apart from them */
    v = EcBoard(); v.ec.screen = PT_EC_LETTER; v.ec.alpha = 1; v.ec.cell = 24; DevCheckTouch(&old, &v);
    for (i = 0; i < 26; ++i) { AssertKey(DevEcLetter(i), DEV_LOOK_KEY, i == 24); assert(EcKeyCount(DevEcLetter(i), i == 24 ? DEV_SUNK_INK : DEV_KEY_INK) > 0); }
    AssertKey(DevEcToggle(0), DEV_LOOK_KEY, 0); EcDark(DevEcToggle(1), 0);
    AssertHits(EcHitV, &v, DevEcToggle(0), PT_EC_HIT_CELL + PT_EC_CELL_TOGGLE); AssertHits(EcHitV, &v, DevEcToggle(1), PT_EC_HIT_CELL + PT_EC_CELL_BACK);
    for (i = 0; i < 2; ++i) { r = DevAcross(7, 6, 40 + 32 * i, 28); assert(at(r.x + 3, r.y + 14) == DEV_SHELL_RED && CtrPokeTouch_EasyHit(&v, r.x + 3, r.y + 3) == -1); }
    v.ec.cell = PT_EC_CELL_TOGGLE; DevCheckTouch(&old, &v); AssertKey(DevEcToggle(0), DEV_LOOK_KEY, 1); AssertKey(DevEcLetter(24), DEV_LOOK_KEY, 0);
    other = v; other.ec.alpha = 0; other.ec.screen = PT_EC_GROUP; TouchSamePixels(&other, &v, 0, "the toggle's word in either order");

    /* a list: its title (the group) over the phrase in one line; the selected word's row inverted, or the one held; the
     * rail's thumb where the rows shown are (24 px of 124 for 30 rows: the top, the middle, the bottom); ▲ ▼ grey where
     * the list goes no further, sunk while held; the strip's lit letter boxed; BACK (dark), the toggle, USE WORD */
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.word = 3; DevCheckTouch(&old, &v);
    EcRowsInverted(&v, 3);
    assert(CountIn(kEcListLine1, DEV_INK) > 0 && CountIn(kEcListLine1, DEV_INK2) == 0 && CountIn(kEcListLine2, DEV_INK2) > 0);
    EcThumb(58, 81); assert(at(EC_TRACK_X, 100) == DEV_INK_DIM && at(EC_TRACK_X + 3, 181) == DEV_INK_DIM && at(EC_TRACK_X + 4, 100) == DEV_PAPER);
    EcArrows(0, 1);
    EcDark(DevEcListKey(0), 0); AssertKey(DevEcListKey(1), DEV_LOOK_KEY, 0); AssertKey(DevEcListKey(2), DEV_LOOK_KEY, 0);
    v.ec.pressed = PT_EC_HIT_ROW + 6; DevCheckTouch(&old, &v); EcRowsInverted(&v, 6);
    v.ec.pressed = PT_EC_HIT_PLACE; DevCheckTouch(&old, &v); EcRowsInverted(&v, 3); AssertKey(DevEcListKey(2), DEV_LOOK_KEY, 1);
    v.ec.pressed = PT_EC_HIT_BACK; DevCheckTouch(&old, &v); EcDark(DevEcListKey(0), 1); AssertKey(DevEcListKey(2), DEV_LOOK_KEY, 0);
    v.ec.pressed = PT_EC_HIT_UP; DevCheckTouch(&old, &v); AssertKey(kDevEcUp, DEV_LOOK_KEY, 1); AssertKey(kDevEcDown, DEV_LOOK_KEY, 0);
    assert(EcKeyCount(kDevEcUp, DEV_BAG_ARROW_OFF) == 25);                   /* held where it goes no further: still taken */
    v.ec.pressed = PT_EC_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevEcDown, DEV_LOOK_KEY, 1); assert(EcKeyCount(kDevEcDown, DEV_SUNK_INK) == 25);
    v.ec.pressed = -1; v.ec.first = 12; v.ec.word = 2 * 12; DevCheckTouch(&old, &v); EcThumb(108, 131); EcArrows(1, 1); EcRowsInverted(&v, 0);
    v.ec.first = 24; v.ec.word = 2 * 24 + 11; DevCheckTouch(&old, &v); EcThumb(158, 181); EcArrows(1, 0); EcRowsInverted(&v, 11);
    v.ec.word = 7; DevCheckTouch(&old, &v); EcRowsInverted(&v, -1);           /* the selected word not shown */
    /* the strip: the lit letter boxed in the paper's ink, the others in the secondary ink; none lit; grey under A-Z */
    v.ec.first = 0; v.ec.letter = 2; DevCheckTouch(&old, &v);
    for (i = 0; i < 26; ++i) {
        r = DevEcStripCell(i);
        assert((at(r.x, r.y) == DEV_INK && at(r.x + r.w - 1, r.y + r.h - 1) == DEV_INK) == (i == 2));
        assert(Count(r.x + 1, r.y + 1, r.w - 2, r.h - 2, i == 2 ? DEV_INK : DEV_INK2) > 0 && Count(r.x + 1, r.y + 1, r.w - 2, r.h - 2, i == 2 ? DEV_INK2 : DEV_INK) == 0);
    }
    v.ec.letter = 255; DevCheckTouch(&old, &v); assert(CountIn(kEcStripPaper, DEV_INK) == 0 && CountIn(kEcStripPaper, DEV_INK2) > 0);
    /* A-Z: the letter on the title with A-Z small at its right, the strip grey (and not taken), the toggle's word */
    v.ec.alpha = 1; v.ec.letter = 2; strcpy(v.ec.listTitle, "C"); DevCheckTouch(&old, &v);
    assert(at(kEcStripPaper.x, kEcStripPaper.y + 1) == EC_DIM(DEV_PAPER) && CountIn(kEcStripPaper, DEV_INK) == 0 && CountIn(kEcStripPaper, DEV_INK2) == 0);
    assert(CountIn(kEcListLine1, DEV_INK) > 0 && CountIn(kEcListLine1, DEV_INK2) > 0);
    other = v; other.ec.alpha = 0; TouchSamePixels(&other, &v, 0, "the list's toggle in either order");
    /* an empty phrase: no line under the title */
    memset(v.ec.chip, 0, sizeof(v.ec.chip)); DevCheckTouch(&old, &v); assert(CountIn(kEcListLine2, DEV_INK2) == 0);
    /* a 5-word list: three rows, the thumb the whole rail, no arrow lit, nothing to scroll */
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.listCount = 5; v.ec.word = 4; DevCheckTouch(&old, &v);
    EcRowsInverted(&v, 4); EcThumb(58, 181); EcArrows(0, 0); assert(CtrPokeTouch_EasyRailTarget(&v.ec, 100) == -1);
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
    for (i = 0; i < 3; ++i) { r = DevEcFooter(i); assert(CtrPokeTouch_EasyHit(&v, r.x + 5, r.y + 5) == (i == 2 ? PT_EC_HIT_YES : -1)); }
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
    strcpy(v.bar[0], "Anything"); v.registeredItem = 7; v.enabled[PT_BAG] = 1; v.playerX = 3; v.backVisible = 0;
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
    v = EcBoard(); v.ec.screen = PT_EC_GROUP; v.ec.cell = 5; DevCheckTouch(&old, &v); shot("groups");
    v.ec.groupCount = 14; v.ec.cell = 13; DevCheckTouch(&old, &v); shot("groups14");
    v = EcBoard(); v.ec.screen = PT_EC_LETTER; v.ec.alpha = 1; v.ec.cell = 7; DevCheckTouch(&old, &v); shot("letters");
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.word = 3; DevCheckTouch(&old, &v); shot("list");
    v.ec.first = 24; v.ec.word = 2 * 24 + 5; v.ec.letter = 22; DevCheckTouch(&old, &v); shot("list_end");
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.listCount = 5; v.ec.word = 2; DevCheckTouch(&old, &v); shot("list_short");
    v = EcBoard(); v.ec.screen = PT_EC_LIST; v.ec.alpha = 1; v.ec.word = 1; v.ec.letter = 2; strcpy(v.ec.listTitle, "C");
    DevCheckTouch(&old, &v); shot("list_alpha");
    v = EcBoard(); v.ec.ask = PT_EC_ASK_QUIT; v.ec.askCursor = 1; strcpy(v.ec.ask1, "Port question for this test?");
    DevCheckTouch(&old, &v); shot("ask");
    assert(sDevKeyCut == 0);                      /* main prints PASS device easychat */
}
