/* ---- options: OPTIONS as the red device, through the field dispatcher (no board; plan 3 Task 1) ------------------------
 * Included by device_render_test.c before its kSuites. */

/* A key OPTIONS hits (CtrPokeTouch_OptionKey) as 0x100 + its code, else a row's row * 16 + zone, else -1. */
static int OptHitV(const void *view, int x, int y)
{
    const CtrPokeTouchView *v = (const CtrPokeTouchView *)view;
    int key = CtrPokeTouch_OptionKey(v, x, y), row = 0, zone = 0;

    if (key >= 0)
        return 0x100 + key;
    return CtrPokeTouch_OptionHit(v, x, y, &row, &zone) ? row * 16 + zone : -1;
}

/* Whether the last whole draw told the kit's trace a piece `op` at (x, y): w x h too when w, its text too when s (a
 * text's y is its capitals', an ART's its kind's box). */
static int OptPiece(int op, int x, int y, int w, int h, const char *s)
{
    for (int i = 0; i < gTraceCount; ++i)
        if (gTrace[i].op == op && gTrace[i].x == x && gTrace[i].y == y && (!w || (gTrace[i].w == w && gTrace[i].h == h))
            && (!s || !strcmp(gTrace[i].s, s)))
            return 1;
    return 0;
}

/* One row of a page as CtrOptions_Fill gives it: a value, a switch (on 2, off 1) or a row of three levels. */
static void OptRowValue(CtrPokeTouchView *v, int i, const char *label, const char *value)
{
    snprintf(v->optLabel[i], sizeof(v->optLabel[i]), "%s", label);
    snprintf(v->optValue[i], sizeof(v->optValue[i]), "%s", value);
}

static void OptRowSwitch(CtrPokeTouchView *v, int i, const char *label, int on)
{
    OptRowValue(v, i, label, on ? "On" : "Off");
    v->optSwitch[i] = (uint8_t)(on ? 2 : 1);
}

static void OptRowLevels(CtrPokeTouchView *v, int i, const char *label, const char *const names[3], int value)
{
    OptRowValue(v, i, label, names[value]);
    v->optSegCount[i] = 3;
    v->optSegValue[i] = (uint8_t)value;
    for (int k = 0; k < 3; ++k)
        snprintf(v->optSegName[i][k], sizeof(v->optSegName[i][k]), "%s", names[k]);
}

/* A page of OPTIONS as 3ds_bottom.c gives it, without its rows: page `page` of six in section `section` of the four
 * (Settings, Graphics, QoL, Nuzlocke: each key its first page's name), the page's place in its section and its own name
 * as CtrOptions_Fill gives them (Graphics 1/3, Looks 2/3, Wilds 3/3; the other sections one page); nothing held, BACK
 * shown and usable; the description's lines. Words in v0.6.0's case. */
static CtrPokeTouchView OptPage(int page, int section, int rows, int selected, const char *line1, const char *line2)
{
    static const char *const names[4] = {"Settings", "Graphics", "QoL", "Nuzlocke"};
    static const char *const pageNames[6] = {"Settings", "Graphics", "Looks", "Wilds", "QoL", "Nuzlocke"};
    static const uint8_t place[6] = {1, 1, 2, 3, 1, 1}, pages[6] = {1, 3, 3, 3, 1, 1};
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.options = 1;
    v.optPage = (uint8_t)page;
    v.optPageCount = 6;
    v.optSection = (uint8_t)section;
    v.optSectionCount = 4;
    for (int s = 0; s < 4; ++s)
        snprintf(v.optSectionName[s], sizeof(v.optSectionName[s]), "%s", names[s]);
    v.optSectionPage = place[page];
    v.optSectionPages = pages[page];
    snprintf(v.optPageName, sizeof(v.optPageName), "%s", pageNames[page]);
    v.optKey = -1;
    v.optPressed = -1;
    v.optCount = (uint8_t)rows;
    v.optSelected = (uint8_t)selected;
    snprintf(v.bar[1], PT_BAR_LEN, "%s", line1);
    snprintf(v.bar[2], PT_BAR_LEN, "%s", line2);
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* The plan's board: page 0 of 6, five rows ("<>" rows at 0 and 1, switches at 2 and 3, a row of three levels at 4), row
 * 0 a 15-byte label and an 11-byte value of the widest capital, the second row the one described. */
static CtrPokeTouchView OptBoard(void)
{
    static const char *const kLevel[3] = {"Off", "Half", "Full"};
    CtrPokeTouchView v = OptPage(0, 0, 5, 1, "Shift offers a switch", "after a foe faints.");

    OptRowValue(&v, 0, "WWWWWWWWWWWWWWW", "WWWWWWWWWWW");
    OptRowValue(&v, 1, "Battle style", "Shift");
    OptRowSwitch(&v, 2, "FPS counter", 1);
    OptRowSwitch(&v, 3, "Battle scene", 0);
    OptRowLevels(&v, 4, "EXP share", kLevel, 1);
    strcpy(v.bar[0], "Battle style");
    return v;
}

/* The board's rows on the first page of GRAPHICS, a section of three pages: the page line and its dots under the rows. */
static CtrPokeTouchView OptGraphics(void)
{
    CtrPokeTouchView v = OptBoard();

    v.optPage = 1;
    v.optSection = 1;
    v.optSectionPage = 1;
    v.optSectionPages = 3;
    strcpy(v.optPageName, "Graphics");
    return v;
}

/* With small letters wider than capitals: `a` and `b` drawn alike (or not, `same` 0). */
static void OptSamePixels(const CtrPokeTouchView *a, const CtrPokeTouchView *b, int same)
{
    static uint16_t first[W * H];
    static uint8_t keepWidths[256], keepSmall[256];

    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = smallWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    CtrPokeTouch_Draw(canvas, a);
    memcpy(first, canvas, sizeof(first));
    CtrPokeTouch_Draw(canvas, b);
    if ((memcmp(first, canvas, sizeof(first)) == 0) != same) {
        fprintf(stderr, "%s: rows '%s' / '%s', look '%s' / '%s': drawn %s\n", gSuite, a->optLabel[1], b->optLabel[1],
                a->optLook.credit, b->optLook.credit, same ? "differently" : "alike");
        assert(0);
    }
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* Row i's levels as drawn (def_opt_graphics' COMPANION): boxes 14 tall from the row's top + 2, packed right 4 apart, the
 * last ending at 250, none left of PREV; a box's own columns its level, and every column from PREV to the row's end a
 * level (the gaps split at their middles), the label's before them. */
static void OptLevelsPacked(const CtrPokeTouchView *v, int i)
{
    int n = DevOptLevels(v, i), y = DevOptRow(i).y + 9;

    assert(n > 0);
    for (int k = 0; k < n; ++k) {
        CtrPokeTouchRect b = DevOptLevelBox(v, i, k);

        assert(b.x >= DEV_OPT_PREV_X && b.y == DevOptRow(i).y + 2 && b.h == 14
               && (k == n - 1 ? b.x + b.w == 250 : DevOptLevelBox(v, i, k + 1).x == b.x + b.w + 4));
        for (int x = b.x; x < b.x + b.w; ++x)
            assert(OptHitV(v, x, y) == i * 16 + PT_OPT_ZONE_SEG + k);
    }
    for (int x = DEV_OPT_PREV_X; x < 260; ++x)
        assert(OptHitV(v, x, y) >= i * 16 + PT_OPT_ZONE_SEG && OptHitV(v, x, y) < i * 16 + PT_OPT_ZONE_SEG + n);
    assert(OptHitV(v, DEV_OPT_PREV_X - 1, y) == i * 16 + PT_OPT_ZONE_LABEL && OptHitV(v, 260, y) == -1);
}

/* Upper case (owner decision 5): v0.6.0's words in proper case drawn in capitals (the sections, the labels, the values,
 * the levels, the description, the look's name); a switch says the port's ON / OFF whatever its value's text; the look's
 * credit (its artists' names) as typed. */
static void OptUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView capitals = v;

    for (int s = 0; s < 4; ++s)
        DevUpper(capitals.optSectionName[s], v.optSectionName[s], sizeof(capitals.optSectionName[s]));
    for (int i = 0; i < 5; ++i) {
        DevUpper(capitals.optLabel[i], v.optLabel[i], sizeof(capitals.optLabel[i]));
        DevUpper(capitals.optValue[i], v.optValue[i], sizeof(capitals.optValue[i]));
        for (int k = 0; k < 3; ++k)
            DevUpper(capitals.optSegName[i][k], v.optSegName[i][k], sizeof(capitals.optSegName[i][k]));
    }
    DevUpper(capitals.bar[1], v.bar[1], sizeof(capitals.bar[1]));
    DevUpper(capitals.bar[2], v.bar[2], sizeof(capitals.bar[2]));
    strcpy(capitals.optValue[2], "anything");               /* a switch row's value: not drawn */
    OptSamePixels(&v, &capitals, 1);
    v.optCount = capitals.optCount = 4;                     /* room for the look's panel */
    v.optLook.on = v.optLook.has = capitals.optLook.on = capitals.optLook.has = 1;
    strcpy(v.optLook.name, "Default");
    strcpy(capitals.optLook.name, "DEFAULT");
    strcpy(v.optLook.credit, "By Kalarie");
    strcpy(capitals.optLook.credit, "By Kalarie");
    OptSamePixels(&v, &capitals, 1);
    strcpy(capitals.optLook.credit, "BY KALARIE");
    OptSamePixels(&v, &capitals, 0);
}

/* The plan's suite (every key and row hit where it is drawn, the cursor's row inverted, ▲▼ only with pages, BACK only
 * while shown), the review's checks (a touch chooses, the levels, BACK's states, what is not drawn presents nothing, the
 * shell, the ways in and out, upper case), then the states for the eye. */
static void SuiteOptions(void)
{
    static const char *const kCompanion[3] = {"Off", "Follow", "Be it"};
    /* COMPANION's and EXP SHARE's levels in the build's language, as 3ds_poketouch_options.c has them (the ES / DE
     * builds draw theirs in the states for the eye: they fit, or go small uncut) */
    static const char *const kCompanionTr[3] = {CTR_TR("Off", "No", "Aus"), CTR_TR("Follow", "Sigue", "Folgt"),
                                                CTR_TR("Be it", "Ser", "Selbst")};
    static const char *const kShareTr[3] = {CTR_TR("Off", "No", "Aus"), CTR_TR("Half", "Mitad", "Halb"),
                                            CTR_TR("Full", "Toda", "Voll")};
    CtrPokeTouchView v = OptBoard(), old = v, other;
    int s, i, k, y;

    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v = OptBoard(); DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevOptShown));
    for (s = 0; s < 4; ++s) AssertHits(OptHitV, &v, DevGridTab(4, s), 0x100 + PT_OPT_KEY_SECTION + s);
    AssertKey(DevGridTab(4, 0), DEV_LOOK_KEY, 1); AssertHits(OptHitV, &v, kDevGridUp, 0x100 + PT_OPT_KEY_UP); AssertHits(OptHitV, &v, kDevGridDown, 0x100 + PT_OPT_KEY_DOWN);
    for (i = 0; i < 4; ++i) { y = DevOptRow(i).y + 9;
        assert(OptHitV(&v, 16, y) == i * 16 + PT_OPT_ZONE_LABEL && OptHitV(&v, 135, y) == i * 16 + PT_OPT_ZONE_LABEL && OptHitV(&v, 136, y) == i * 16 + PT_OPT_ZONE_PREV
               && OptHitV(&v, 160, y) == i * 16 + PT_OPT_ZONE_NEXT && OptHitV(&v, 259, y) == i * 16 + PT_OPT_ZONE_NEXT && OptHitV(&v, 15, y) == -1 && OptHitV(&v, 260, y) == -1); }
    OptLevelsPacked(&v, 4);                                                                      /* EXP SHARE's */
    AssertHits(OptHitV, &v, kDevSmallKey[0], 16 + PT_OPT_ZONE_PREV); AssertHits(OptHitV, &v, kDevSmallKey[1], 16 + PT_OPT_ZONE_NEXT); AssertHits(BackV, &v, kDevGridExit, 1);
    assert(OptHitV(&v, 20, DevOptRow(5).y + 9) == -1 && at(17, DevOptRow(1).y + 9) == DEV_BEZEL);                 /* x 17: before the label */
    v.optPressed = 3; v.optZone = PT_OPT_ZONE_LABEL; DevCheckTouch(&old, &v); assert(at(17, DevOptRow(3).y + 9) == DEV_BEZEL && at(17, DevOptRow(1).y + 9) != DEV_BEZEL);
    v.optPressed = -1; v.optKey = PT_OPT_KEY_DOWN; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevGridDown, DEV_LOOK_KEY, 1);
    AssertKey(DevGridTab(4, 0), DEV_LOOK_KEY, 1);                                                                /* ▼ held: the section stays sunk */
    v.optKey = -1; DevCheckTouch(&old, &v); v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
    v.optPageCount = 1; DevCheckTouch(&old, &v); assert(CtrPokeTouch_OptionKey(&v, 294, 73) == -1 && at(294, 73) == DEV_SHELL_RED);
    v.backVisible = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_BackHit(&v, 278, 210));
    v = OptBoard(); v.optCount = 4; v.optLook.on = 1; strcpy(v.optLook.name, "ASH"); DevCheckTouch(&old, &v);
    assert(at(200, 121) == DEV_PAPER && at(200, 122) == DEV_PAPER2 && at(200, 157) == DEV_PAPER2 && at(200, 158) == DEV_PAPER
           && at(200, 160) == DEV_PAPER && Count(62, 128, 190, 9, DEV_INK) > 0);                    /* the panel ends at 157 */
    assert(OptPiece(DEV_TRACE_ART, 22, 124, 32, 32, NULL) == 0 && OptPiece(DEV_TRACE_TEXT, 62, 128, 0, 0, "ASH"));  /* no picture: none */
    /* a section of several pages: its page line under the rows ("PAGE 1/3 · GRAPHICS": small, the secondary ink, from
     * x 22) and a dot a page at the right, the page shown's on; a page or a name that changes draws it again; with one
     * page neither, and its page and name are not drawn */
    v = OptGraphics(); DevCheckTouch(&old, &v);
    assert(Count(22, DEV_OPT_PAGE_Y, 140, 7, DEV_INK2) > 0 && at(DevOptDot(0).x + 2, DevOptDot(0).y + 2) == DEV_MARK_ON
           && at(DevOptDot(2).x + 2, DevOptDot(2).y + 2) == DEV_PAPER_LINE && Count(250, 160, 12, 12, DEV_PAPER_LINE) == 0);
    {
        char line[48], word[16];

        DevUpper(word, CtrUiText(UIS_DEX_PAGE), sizeof(word));
        snprintf(line, sizeof(line), "%s 1/3 \xC2\xB7 GRAPHICS", word);
        assert(OptPiece(DEV_TRACE_TEXT, 22, DEV_OPT_PAGE_Y, 0, 0, line));
        for (k = 0; k < 3; ++k) assert(OptPiece(DEV_TRACE_DISC, DevOptDot(k).x, DevOptDot(k).y, 5, 5, NULL));
    }
    v.optSectionPage = 2; assert(DevWhole(DevCheckTouch(&old, &v)));
    assert(at(DevOptDot(0).x + 2, DevOptDot(0).y + 2) == DEV_PAPER_LINE && at(DevOptDot(1).x + 2, DevOptDot(1).y + 2) == DEV_MARK_ON);
    strcpy(v.optPageName, "Looks"); assert(DevWhole(DevCheckTouch(&old, &v)));
    {   /* the line ends by FRAME's sample less 4: a name too long is cut there, not counted (FireRed's page names fit) */
        static uint8_t keepSmall[256];

        memcpy(keepSmall, smallWidths, sizeof(keepSmall));
        smallWidths[0xd1] = 8;                                                                         /* W */
        memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
        other = v; strcpy(other.optPageName, "WWWWWWWWWWW"); sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &other);
        assert(Count(22, DEV_OPT_PAGE_Y, kDevOptSample.x - 4 - 22, 7, DEV_INK2) > 0 && sDevKeyCut == 0
               && Count(kDevOptSample.x - 4, DEV_OPT_PAGE_Y, 60, 7, DEV_INK2) == 0);
        memcpy(smallWidths, keepSmall, sizeof(keepSmall));
        memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
        CtrPokeTouch_Draw(canvas, &old);
    }
    v.optSectionPages = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && Count(DevOptDot(2).x, DevOptDot(2).y, 5, 5, DEV_PAPER_LINE) == 0);
    v.optSectionPages = 3; assert(DevWhole(DevCheckTouch(&old, &v)));
    v.optSectionPages = 1; DevCheckTouch(&old, &v); assert(Count(16, 158, 244, 18, DEV_INK2) == 0 && Count(16, 158, 244, 18, DEV_PAPER_LINE) == 0);
    v.optSectionPage = 3; strcpy(v.optPageName, "Wilds"); assert(DevCheckTouch(&old, &v).count == 0);
    v.optSectionPages = 0; assert(DevCheckTouch(&old, &v).count == 0);                    /* 0 or 1 page: drawn alike */
    v.optSectionPages = 1; assert(DevCheckTouch(&old, &v).count == 0);
    /* the note: the description in the small font, three lines 9 apart from capitals 199 (the third's 217: with a
     * descender on every small glyph, SmallDescenders, it ends by the paper's last row, and none reaches the bezel under
     * it; with an accent on every glyph, Accents, the first's stay on the paper, none on the bezel over it); a text that
     * needs a fourth line is left out there and counted */
    v = OptBoard(); strcpy(v.bar[1], "Words words words words words words"); strcpy(v.bar[2], "words words words words words words");
    DevCheckTouch(&old, &v); assert(Count(18, 217, 132, 7, DEV_INK) > 0 && Count(18, 196, 132, 3, DEV_INK) == 0 && sDevKeyCut == 0);
    assert(Count(18, 199, 132, 1, DEV_INK) > 0 && Count(18, 208, 132, 1, DEV_INK) > 0 && Count(18, 207, 132, 1, DEV_INK) == 0
           && Count(18, 216, 132, 1, DEV_INK) == 0);
    SmallDescenders(1); CtrPokeTouch_Draw(canvas, &v);
    assert(Count(18, 224, 132, 1, DEV_INK) > 0 && Count(14, 225, 140, 6, DEV_INK) == 0);
    SmallDescenders(0);
    Accents(1); CtrPokeTouch_Draw(canvas, &v); assert(BezelOver(14, 140)); Accents(0);
    CtrPokeTouch_Draw(canvas, &old);
    other = v; strcpy(other.bar[1], "Abcdefghijklm abcdefghijklm"); strcpy(other.bar[2], "abcdefghijklm abcdefghijklm");
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &other); assert(sDevKeyCut == 1); sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &old);

    /* the board's pieces: the sections (the shown one sunk with its LED), the rows on the list's paper (the described
     * one inverted, none past the last), ▲▼, the description in the small note, ◀ ▶ up, BACK dark */
    v = OptBoard(); DevCheckTouch(&old, &v);
    for (s = 1; s < 4; ++s) AssertKey(DevGridTab(4, s), DEV_LOOK_KEY, 0);
    assert(Count(8, 8, 73, 24, DEV_LED) > 0 && Count(85, 8, 227, 24, DEV_LED) == 0);
    assert(at(17, DevOptRow(0).y + 9) == DEV_PAPER && at(20, DevOptRow(5).y + 9) == DEV_PAPER);
    AssertKey(kDevGridUp, DEV_LOOK_KEY, 0); AssertKey(kDevGridDown, DEV_LOOK_KEY, 0);
    AssertKey(kDevSmallKey[0], DEV_LOOK_KEY, 0); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(Count(14, 196, 140, 30, DEV_INK) > 0 && Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    /* a row's text: its label from 6 px in, ◀ in PREV, the value and ▶ in NEXT (the inverted row's in the paper's ink) */
    y = DevOptRow(1).y;
    assert(Count(22, y, 114, 18, DEV_PAPER) > 0 && Count(16, y, 6, 18, DEV_PAPER) == 0 && Count(136, y, 24, 18, DEV_PAPER) > 0);
    assert(Count(DEV_OPT_VALUE_X, y, DEV_OPT_VALUE_W, 18, DEV_PAPER) > 0 && Count(248, y, 9, 18, DEV_PAPER) > 0);
    {   /* the value centred in 152-247 (the board's box) */
        int w = DevTextWidth("SHIFT"), x0 = DEV_OPT_VALUE_X + (DEV_OPT_VALUE_W - w) / 2;

        assert(Count(DEV_OPT_VALUE_X, y, x0 - DEV_OPT_VALUE_X, 18, DEV_PAPER) == 0 && Count(x0, y, w, 18, DEV_PAPER) > 0
               && Count(x0 + w, y, DEV_OPT_VALUE_X + DEV_OPT_VALUE_W - x0 - w, 18, DEV_PAPER) == 0);
    }
    /* the levels (def_opt_graphics' COMPANION: OFF, FOLLOW, BE IT, FOLLOW chosen): each word in a box of its width + 8,
     * 14 tall from the row's top + 2, packed right 4 apart, the last ending at 250 (the small font when the normal one
     * needs more than the 114 px from PREV: FireRed's 6 px capitals need 116); every column from 136 to 259 a level, the
     * gaps split at their middles; the chosen one outlined 1 px on its box's edge in the row's ink (the paper's on the
     * row described), a held one instead; a set too wide even small counted once a draw */
    {
        CtrPokeTouchRect b[3];
        int small;

        v = OptBoard(); OptRowLevels(&v, 4, "Companion", kCompanion, 1); DevCheckTouch(&old, &v);
        y = DevOptRow(4).y + 9;
        for (k = 0; k < 3; ++k) b[k] = DevOptLevelBox(&v, 4, k);
        small = DevOptLevelSmall(&v, 4);
        assert(small == (DevTextWidth("OFF") + DevTextWidth("FOLLOW") + DevTextWidth("BE IT") + 3 * 8 + 2 * 4 > 114));
        assert(b[2].x + b[2].w == 250 && b[1].x == b[0].x + b[0].w + 4 && b[2].x == b[1].x + b[1].w + 4
               && b[0].y == DevOptRow(4).y + 2 && b[0].h == 14
               && b[1].w == (small ? DevSmallTextWidth("FOLLOW") : DevTextWidth("FOLLOW")) + 8);
        for (int x = 136; x < 260; ++x) assert(OptHitV(&v, x, y) >= 4 * 16 + PT_OPT_ZONE_SEG);
        assert(OptHitV(&v, b[1].x - 3, y) == 4 * 16 + PT_OPT_ZONE_SEG && OptHitV(&v, b[1].x - 2, y) == 4 * 16 + PT_OPT_ZONE_SEG + 1
               && OptHitV(&v, b[2].x - 3, y) == 4 * 16 + PT_OPT_ZONE_SEG + 1 && OptHitV(&v, b[2].x - 2, y) == 4 * 16 + PT_OPT_ZONE_SEG + 2
               && OptHitV(&v, 136, y) == 4 * 16 + PT_OPT_ZONE_SEG && OptHitV(&v, 259, y) == 4 * 16 + PT_OPT_ZONE_SEG + 2);
        OptLevelsPacked(&v, 4);
        assert(at(b[1].x, b[1].y + 7) == DEV_INK && at(b[0].x, b[0].y + 7) != DEV_INK);       /* FOLLOW outlined */
        assert(at(b[1].x + b[1].w - 1, b[1].y + 7) == DEV_INK && at(b[1].x + 6, b[1].y) == DEV_INK
               && at(b[1].x + 6, b[1].y + 13) == DEV_INK && at(b[1].x - 1, b[1].y + 7) == DEV_PAPER
               && at(b[1].x + 1, b[1].y + 7) == DEV_PAPER && at(b[1].x + 6, b[1].y - 1) == DEV_PAPER
               && at(b[1].x + 6, b[1].y + 14) == DEV_PAPER && at(b[1].x + b[1].w, b[1].y + 7) == DEV_PAPER);
        for (k = 0; k < 3; ++k) {                                      /* each word centred in its box, the row's ink */
            char word[8];

            DevUpper(word, kCompanion[k], sizeof(word));
            assert(OptPiece(DEV_TRACE_TEXT, b[k].x + 4, b[k].y + (small ? 3 : 2), 0, 0, word));
            assert(Count(b[k].x + 2, b[k].y + 2, b[k].w - 4, 10, DEV_INK) > 0);
        }
        v.optSegValue[4] = 2; assert(DevWhole(DevCheckTouch(&old, &v))); assert(at(b[2].x, b[2].y + 7) == DEV_INK);
        assert(at(b[1].x, b[1].y + 7) == DEV_PAPER);
        v.optPressed = 4; v.optZone = PT_OPT_ZONE_SEG; DevCheckTouch(&old, &v);           /* OFF held: the row inverted */
        assert(at(b[0].x, b[0].y + 7) == DEV_PAPER && at(b[2].x, b[2].y + 7) == DEV_BEZEL);
        v.optPressed = -1; v.optSelected = 4; DevCheckTouch(&old, &v);                       /* described: BE IT's */
        assert(at(b[2].x, b[2].y + 7) == DEV_PAPER && at(b[0].x, b[0].y + 7) == DEV_BEZEL);
        v.optSelected = 1; v.optSegValue[4] = 3; DevCheckTouch(&old, &v);                    /* none (a bad value) */
        for (k = 0; k < 3; ++k) assert(at(b[k].x, b[k].y + 7) == DEV_PAPER);
        {   /* capitals 8 wide: COMPANION's words need more than 114 in the normal font, not in the small one: drawn
             * small, uncut */
            static uint8_t keepWidths[256];
            CtrPokeTouchRect s;

            memcpy(keepWidths, fontWidths, sizeof(keepWidths));
            for (unsigned c = 0xbb; c <= 0xd4; ++c)       /* A..Z */
                fontWidths[c] = 8;
            memset(sPtFontReady, 0, sizeof(sPtFontReady));
            other = OptPage(1, 1, 5, 0, "", ""); OptRowLevels(&other, 4, "Companion", kCompanion, 1);
            sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &other);
            s = DevOptLevelBox(&other, 4, 1);
            assert(DevOptLevelSmall(&other, 4) && sDevKeyCut == 0 && s.w == DevSmallTextWidth("FOLLOW") + 8
                   && OptPiece(DEV_TRACE_TEXT, s.x + 4, s.y + 3, 0, 0, "FOLLOW") && at(s.x, s.y + 7) == DEV_INK);
            OptLevelsPacked(&other, 4);
            memcpy(fontWidths, keepWidths, sizeof(keepWidths));
            memset(sPtFontReady, 0, sizeof(sPtFontReady));
            CtrPokeTouch_Draw(canvas, &old);
        }
        v.optSegValue[4] = 1;
        for (k = 0; k < 3; ++k) strcpy(v.optSegName[4][k], "Abcdefg");                        /* too wide even small */
        DevCheckTouch(&old, &v); sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &v);              /* one draw: one cut */
        assert(DevOptLevelSmall(&v, 4) && sDevKeyCut == 1); sDevKeyCut = 0;
        OptLevelsPacked(&v, 4);                                  /* shared out from PREV, the words cut in their boxes */
        assert(Count(DEV_OPT_PREV_X - 10, DevOptRow(4).y, 10, 18, DEV_INK) == 0 && sDevKeyCut == 0);
        for (k = 0; k < 3; ++k) {                                     /* each word inside its box less 4 a side */
            CtrPokeTouchRect c = DevOptLevelBox(&v, 4, k);

            assert(Count(c.x + 4, c.y + 1, c.w - 8, c.h - 2, DEV_INK) > 0 && Count(c.x + 1, c.y + 1, 3, c.h - 2, DEV_INK) == 0
                   && Count(c.x + c.w - 4, c.y + 1, 3, c.h - 2, DEV_INK) == 0);
        }
        v = OptBoard(); DevCheckTouch(&old, &v);
    }
    /* a touch on ◀ or ▶ (or the described row's own zone) sinks that key; a held section key is the one sunk */
    v.optPressed = 1; v.optZone = PT_OPT_ZONE_PREV; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[0], DEV_LOOK_KEY, 1); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 0);
    v.optZone = PT_OPT_ZONE_NEXT; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 1); AssertKey(kDevSmallKey[0], DEV_LOOK_KEY, 0);
    v.optPressed = 3; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 0);                     /* another row's: not */
    v.optPressed = -1; v.optKey = PT_OPT_KEY_SECTION + 2; DevCheckTouch(&old, &v);
    AssertKey(DevGridTab(4, 2), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(4, 0), DEV_LOOK_KEY, 0);
    v.optKey = PT_OPT_KEY_UP; DevCheckTouch(&old, &v); AssertKey(kDevGridUp, DEV_LOOK_KEY, 1);
    v.optKey = -1;
    /* BACK: sunk under a touch, grey while it cannot be used, gone when not shown (and the shell there) */
    v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    v.backPressed = 0; v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    v.backEnabled = 1; v.backVisible = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DEV_SHELL_RED && !BackV(&v, 247, 210));
    v.backVisible = 1; DevCheckTouch(&old, &v);
    /* what OPTIONS does not draw presents nothing: the first line of the bar (the row's label), a row past the page's,
     * FRAME's type with no sample, a look not shown, a section of one page's page and name, HOME's keys, the map */
    strcpy(v.bar[0], "Anything"); strcpy(v.optLabel[6], "Seventh"); v.optFrameType = 3; strcpy(v.optLook.name, "ASH");
    v.optSectionPage = 2; strcpy(v.optPageName, "Anything");
    v.enabled[PT_BAG] = 1; v.playerX = 3;
    assert(DevCheckTouch(&old, &v).count == 0);
    v.optLook.name[0] = 0;
    v.shell = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_BLUE);
    v.shell = 0; DevCheckTouch(&old, &v);
    /* every level and section answers by v's own rows: a page with fewer rows hits none past them; no description */
    v.optCount = 2; DevCheckTouch(&old, &v); assert(OptHitV(&v, 100, DevOptRow(2).y + 9) == -1 && at(20, DevOptRow(2).y + 9) == DEV_PAPER);
    v.bar[1][0] = v.bar[2][0] = 0; DevCheckTouch(&old, &v); assert(Count(14, 196, 140, 30, DEV_INK) == 0);
    /* the ways in and out: HOME, OPTIONS, the GBA picture (in the frame's screen: the frame's BACK, nothing of OPTIONS
     * hit) */
    other = v; other.options = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)));
    v = OptBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    v.viewport = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && DrawnBy(&v, DevFrameShown));
    assert(OptHitV(&v, 100, DevOptRow(1).y + 9) == -1 && CtrPokeTouch_BackHit(&v, 278, 209) && !CtrPokeTouch_BackHit(&v, 180, 210));
    v = OptBoard(); DevCheckTouch(&old, &v);
    OptUpperCase(OptBoard());
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first (v0.6.0's pages, the words as 3ds_bottom.c gives them). */
    v = OptPage(0, 0, 5, 0, "How fast dialogue", "text is printed.");
    OptRowValue(&v, 0, "Text speed", "Mid"); OptRowSwitch(&v, 1, "Battle scene", 1); OptRowValue(&v, 2, "Battle style", "Shift");
    OptRowValue(&v, 3, "Sound", "Mono"); OptRowValue(&v, 4, "Button mode", "Help");
    DevCheckTouch(&old, &v); shot("settings");
    /* FRAME described: its sample (the pack's frame; none in the synthetic run) under the rows, left of the page's dots */
    v = OptPage(1, 1, 5, 0, "Menus, Yes/No", "box frame.");
    OptRowValue(&v, 0, "Frame", "Type1"); OptRowSwitch(&v, 1, "FPS counter", 0); OptRowValue(&v, 2, "Day/Night", "Clock");
    OptRowValue(&v, 3, "Hour", "12:00"); OptRowLevels(&v, 4, "Companion", kCompanionTr, 1);
    v.optFrameShow = 1; v.optFrameType = 0;
    DevCheckTouch(&old, &v); AssertKey(DevGridTab(4, 1), DEV_LOOK_KEY, 1);
    assert(OptPiece(DEV_TRACE_ART, kDevOptSample.x, kDevOptSample.y, kDevOptSample.w, kDevOptSample.h, NULL));
    OptLevelsPacked(&v, 4); assert(sDevKeyCut == 0);                     /* COMPANION's words: they fit, or go small */
    shot("graphics_frame");
    /* the looks page (GRAPHICS sunk, the page line PAGE 2/3 · LOOKS): RED LOOK described, the look's panel with its
     * walking picture, its name and its artists */
    v = OptPage(2, 1, 4, 1, "Red's look, from the", "alternative looks.");
    OptRowValue(&v, 0, "Shell colour", "Red"); OptRowValue(&v, 1, "Red look", "ASH"); OptRowValue(&v, 2, "Leaf look", "Default");
    OptRowValue(&v, 3, "Rival look", "Default");
    v.optLook.on = v.optLook.has = 1; strcpy(v.optLook.name, "ASH"); strcpy(v.optLook.credit, "By Kalarie");
    for (i = 0; i < 512; ++i) v.optLook.tiles[i] = (uint8_t)(i * 7);
    for (i = 1; i < 16; ++i) v.optLook.palette[i] = (uint16_t)(i * 0x0421);
    DevCheckTouch(&old, &v);
    assert(Count(kDevOptLook.x, kDevOptLook.y, kDevOptLook.w, kDevOptLook.h, DEV_PAPER2) > 0 && Count(62, 122, 192, 36, DEV_INK) > 0);
    assert(OptPiece(DEV_TRACE_ART, 22, 124, 32, 32, NULL) && OptPiece(DEV_TRACE_TEXT, 62, 128, 0, 0, "ASH")
           && OptPiece(DEV_TRACE_TEXT, 62, 142, 0, 0, "By Kalarie") && at(DevOptDot(1).x + 2, DevOptDot(1).y + 2) == DEV_MARK_ON);
    shot("looks");
    /* a credit wider than its 192 px: its second line's capitals 150, 8 under the first's, so with a descender on every
     * small glyph (SmallDescenders) it ends on the panel's last row, 157, none on the paper under it */
    other = v; strcpy(other.optLook.credit, "By Kalarie, and port-written words for more artists than a line holds");
    SmallDescenders(1); CtrPokeTouch_Draw(canvas, &other);
    assert(Count(62, 150, 192, 1, DEV_INK2) > 0 && Count(62, 157, 192, 1, DEV_INK2) > 0 && Count(16, 158, 244, 4, DEV_INK2) == 0);
    SmallDescenders(0); CtrPokeTouch_Draw(canvas, &old);
    /* QoL: EXP SHARE's levels described */
    v = OptPage(4, 2, 6, 3, "Share EXP with your", "team: Half or Full.");
    OptRowSwitch(&v, 0, "Type matchup", 1); OptRowSwitch(&v, 1, "Move details", 1); OptRowSwitch(&v, 2, "Repel again", 0);
    OptRowLevels(&v, 3, "EXP share", kShareTr, 2); OptRowSwitch(&v, 4, "Reusable TMs", 0); OptRowSwitch(&v, 5, "Free HMs", 0);
    DevCheckTouch(&old, &v); OptLevelsPacked(&v, 3); assert(sDevKeyCut == 0);           /* EXP SHARE's on the row described */
    {
        CtrPokeTouchRect full = DevOptLevelBox(&v, 3, 2), half = DevOptLevelBox(&v, 3, 1);

        assert(at(full.x, full.y + 7) == DEV_PAPER && at(half.x, half.y + 7) == DEV_BEZEL);   /* FULL outlined, paper's ink */
    }
    shot("qol_levels");
    v = OptPage(5, 3, 5, 1, "Wild and trainer Pokemon", "are random species.");
    OptRowSwitch(&v, 0, "Nuzlocke", 1); OptRowSwitch(&v, 1, "Randomlocke", 0); OptRowSwitch(&v, 2, "Duplicates", 1);
    OptRowSwitch(&v, 3, "Shiny clause", 0); OptRowSwitch(&v, 4, "Level caps", 1);
    DevCheckTouch(&old, &v); shot("nuzlocke");
    /* QoL's key held from the Nuzlocke page */
    v.optKey = PT_OPT_KEY_SECTION + 2; DevCheckTouch(&old, &v); AssertKey(DevGridTab(4, 2), DEV_LOOK_KEY, 1); shot("pressed_key");
    assert(sDevKeyCut == 0);                      /* main prints PASS device options */
}
