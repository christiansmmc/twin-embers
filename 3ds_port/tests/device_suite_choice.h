/* ---- choice: the field's choices and YES / NO on the grid (boards def_choice_yesno, def_choice_list and
 * def_choice_grid; plan 04a Task 10), through the field dispatcher -------------------------------------------------
 * Included by device_render_test.c before its kSuites. */

/* The grid's note's paper (kDevGridNote 6 px in). */
static const CtrPokeTouchRect kChoicePaper = {14, 196, 216, 30};

static int ChoiceHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_ChoiceHit((const CtrPokeTouchView *)view, x, y);
}

static const char *const kChoiceLabel[PT_CHOICE_MAX] = {
    "FRESH WATER", "SODA POP", "LEMONADE", "WITHDRAW", "DEPOSIT", "MOVE ITEMS", "SEE YA!", "CANCEL",
};
/* Port-written stand-ins for FireRed's questions: one line of the normal font, two, one the normal font would set in
 * three (three lines of the small font), and one the small font needs more than three for. */
static const char kChoicePrompt[] = "Port-written words for this test.";
static const char kChoiceTwo[] = "Port-written words for this test, set in two lines.";
static const char kChoiceThree[] = "Port-written words for a question so long that the normal font would need three "
                                   "lines for it.";
static const char kChoiceLong[] = "Representative Accomplishment Responsibility Infrastructure Recommendation "
                                  "Characteristic Understandably Administration Transformation Overwhelmingly";

/* A choice as 3ds_bottom.c gives it (ChoiceSnapshot): n of FireRed's labels (YES and NO for two) as its menu prints
 * them, `cols` a grid's columns (0: FireRed's one column), the cursor on the first, the message box's question; BACK
 * shown and usable, nothing held. All eight labels are there, so a suite may change the count. */
static CtrPokeTouchView ChoiceBoard(int n, int cols)
{
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.choice.on = 1;
    v.choice.count = (uint8_t)n;
    v.choice.cols = (uint8_t)cols;
    v.choice.pressed = -1;
    for (int i = 0; i < PT_CHOICE_MAX; ++i)
        strcpy(v.choice.label[i], kChoiceLabel[i]);
    if (n == 2) {
        strcpy(v.choice.label[0], "YES");
        strcpy(v.choice.label[1], "NO");
    }
    strcpy(v.choice.prompt, kChoicePrompt);
    snprintf(v.bar[0], PT_BAR_LEN, "Choice");                   /* the bar: not drawn by the choice */
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* Upper case (owner decision 5): FireRed's labels, which v0.6.0 title-cased, in capitals; FireRed's question as it is. */
static void ChoiceUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    for (int i = 0; i < PT_CHOICE_MAX; ++i)
        CtrTextCase(cased.choice.label[i]);                     /* "FRESH WATER" -> "Fresh Water" */
    TouchSamePixels(&cased, &v, 1, "the labels in v0.6.0's case and as FireRed writes them");
    cased = v;
    DevUpper(cased.choice.prompt, v.choice.prompt, sizeof(cased.choice.prompt));
    TouchSamePixels(&cased, &v, 0, "the question, as FireRed writes it");
}

/* The question on the note's paper: one line or two of the normal font (its shadow), centred down the paper; three of
 * the small one (no shadow) from the paper's top + 2, 10 apart; a question that needs more counted once a draw; none,
 * nothing. */
static void ChoiceQuestion(CtrPokeTouchView *old, CtrPokeTouchView v)
{
    int cut;

    strcpy(v.choice.prompt, kChoicePrompt); DevCheckTouch(old, &v);
    assert(CountIn(kChoicePaper, DEV_INK) > 0 && CountIn(kChoicePaper, DEV_INK_SH) > 0);
    strcpy(v.choice.prompt, kChoiceTwo); assert(DevWhole(DevCheckTouch(old, &v)));
    assert(CountIn(kChoicePaper, DEV_INK) > 0 && CountIn(kChoicePaper, DEV_INK_SH) > 0);
    assert(Count(20, 196, 204, 13, DEV_INK) > 0 && Count(20, 211, 204, 15, DEV_INK) > 0);    /* two lines */
    strcpy(v.choice.prompt, kChoiceThree); DevCheckTouch(old, &v);
    assert(CountIn(kChoicePaper, DEV_INK) > 0 && CountIn(kChoicePaper, DEV_INK_SH) == 0);
    assert(Count(20, 198, 204, 1, DEV_INK) > 0 && Count(20, 208, 204, 1, DEV_INK) > 0 && Count(20, 218, 204, 1, DEV_INK) > 0);
    assert(Count(20, 196, 204, 2, DEV_INK) == 0 && Count(20, 207, 204, 1, DEV_INK) == 0 && Count(20, 217, 204, 1, DEV_INK) == 0);
    assert(Count(14, 196, 6, 30, DEV_INK) == 0 && Count(224, 196, 6, 30, DEV_INK) == 0);
    strcpy(v.choice.prompt, kChoiceLong); DevCheckTouch(old, &v);
    cut = sDevKeyCut; CtrPokeTouch_Draw(canvas, &v);                    /* one draw: one cut */
    assert(sDevKeyCut == cut + 1 && CountIn(kChoicePaper, DEV_INK_SH) == 0 && Count(20, 218, 204, 1, DEV_INK) > 0);
    sDevKeyCut = 0;
    v.choice.prompt[0] = 0; DevCheckTouch(old, &v); assert(CountIn(kChoicePaper, DEV_INK) == 0);
}

/* The plan's suite (YES / NO two tall keys; every key hit where drawn in one column, in two and in FireRed's grids;
 * BACK in the exit corner; nothing hit between the keys and the footer; the question on the note), then the layouts,
 * the keys' looks and words, the question, BACK's states, the ways in and out, upper case and the states for the eye. */
static void SuiteChoice(void)
{
    CtrPokeTouchView v = ChoiceBoard(8, 0), old = v, other;
    CtrPokeTouchRect r;
    int n, i;

    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v = ChoiceBoard(8, 0); DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevChoiceShown));
    v.choice.count = 2; v.choice.cols = 0; DevCheckTouch(&old, &v);
    AssertHits(ChoiceHitV, &v, (CtrPokeTouchRect){8, 8, 304, 84}, PT_CHOICE_HIT_ROW); AssertHits(ChoiceHitV, &v, (CtrPokeTouchRect){8, 100, 304, 84}, PT_CHOICE_HIT_ROW + 1);
    for (n = 1; n <= 8; ++n) { v.choice.count = (uint8_t)n; DevCheckTouch(&old, &v); for (i = 0; i < n; ++i) AssertHits(ChoiceHitV, &v, DevChoiceKey(n, 0, i), PT_CHOICE_HIT_ROW + i); }
    assert(Same(DevChoiceKey(4, 0, 3), (CtrPokeTouchRect){8, 140, 304, 38}) && Same(DevChoiceKey(5, 0, 3), (CtrPokeTouchRect){8, 140, 148, 38})
           && Same(DevChoiceKey(5, 0, 4), (CtrPokeTouchRect){164, 8, 148, 38}) && Same(DevChoiceKey(6, 3, 4), (CtrPokeTouchRect){112, 52, 96, 38}));
    AssertHits(BackV, &v, kDevGridExit, 1); assert(CtrPokeTouch_ChoiceHit(&v, 160, 186) == -1 && Count(14, 196, 216, 30, DEV_INK) > 0);
    v.choice.count = 8; v.choice.cols = 3; DevCheckTouch(&old, &v); AssertHits(ChoiceHitV, &v, DevChoiceKey(8, 3, 7), PT_CHOICE_HIT_ROW + 7);
    AssertKey(DevChoiceKey(8, 3, v.choice.cursor), DEV_LOOK_KEY, 1);
    v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* the layouts: YES / NO two tall keys; up to four keys in one column across the screen, 38 tall and 44 apart; five
     * to eight in two columns going down first, the left one always four; FireRed's grid of 2 and 3 columns across, as
     * its grid goes (a grid of one is the one column); every key of each hit where drawn */
    assert(Same(DevChoiceKey(2, 0, 0), (CtrPokeTouchRect){8, 8, 304, 84}) && Same(DevChoiceKey(2, 1, 1), (CtrPokeTouchRect){8, 100, 304, 84}));
    assert(Same(DevChoiceKey(1, 0, 0), (CtrPokeTouchRect){8, 8, 304, 38}) && Same(DevChoiceKey(3, 0, 2), (CtrPokeTouchRect){8, 96, 304, 38}));
    for (i = 0; i < 4; ++i)
        assert(Same(DevChoiceKey(4, 0, i), (CtrPokeTouchRect){8, (int16_t)(8 + 44 * i), 304, 38}));
    for (n = 5; n <= 8; ++n)
        for (i = 0; i < n; ++i)
            assert(Same(DevChoiceKey(n, 0, i), DevAcross(2, i / 4, 8 + 44 * (i % 4), 38)));
    assert(Same(DevChoiceKey(8, 0, 7), (CtrPokeTouchRect){164, 140, 148, 38}) && Same(DevChoiceKey(2, 2, 1), (CtrPokeTouchRect){164, 8, 148, 38}));
    assert(Same(DevChoiceKey(6, 2, 1), (CtrPokeTouchRect){164, 8, 148, 38}) && Same(DevChoiceKey(6, 2, 2), (CtrPokeTouchRect){8, 52, 148, 38}));
    assert(Same(DevChoiceKey(8, 3, 2), (CtrPokeTouchRect){216, 8, 96, 38}) && Same(DevChoiceKey(8, 3, 7), (CtrPokeTouchRect){112, 96, 96, 38}));
    for (int cols = 2; cols <= 3; ++cols)
        for (n = 1; n <= 8; ++n) {
            v.choice.count = (uint8_t)n; v.choice.cols = (uint8_t)cols; DevCheckTouch(&old, &v);
            for (i = 0; i < n; ++i) AssertHits(ChoiceHitV, &v, DevChoiceKey(n, cols, i), PT_CHOICE_HIT_ROW + i);
        }
    /* five keys in two columns: the right column's second place empty, the shell's, nothing hit */
    v = ChoiceBoard(5, 0); DevCheckTouch(&old, &v); r = DevChoiceKey(6, 0, 5);
    assert(Same(r, (CtrPokeTouchRect){164, 52, 148, 38}));
    assert(at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED && CtrPokeTouch_ChoiceHit(&v, r.x + 10, r.y + 10) == -1);

    /* the keys: the cursor's sunk with its LED, the others up, FireRed's words on them; a touch sinks the key under it,
     * not the cursor's */
    v = ChoiceBoard(6, 0); v.choice.cursor = 4; DevCheckTouch(&old, &v);
    for (i = 0; i < 6; ++i) AssertKey(DevChoiceKey(6, 0, i), DEV_LOOK_KEY, i == 4);
    r = DevChoiceKey(6, 0, 4); assert(LedAt(r) && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_SUNK_INK) > 0);
    r = DevChoiceKey(6, 0, 0); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) == 0 && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_KEY_INK) > 0);
    v.choice.pressed = PT_CHOICE_HIT_ROW + 2; DevCheckTouch(&old, &v);
    AssertKey(DevChoiceKey(6, 0, 2), DEV_LOOK_KEY, 1); AssertKey(DevChoiceKey(6, 0, 4), DEV_LOOK_KEY, 0);
    v.choice.pressed = -1; DevCheckTouch(&old, &v);
    /* the words: on a key across the screen from 12 px in (def_choice_list), centred on a narrower key (two columns,
     * FireRed's grid: def_choice_grid) and on YES / NO's tall keys (def_choice_yesno); the normal font everywhere (its
     * shadow in the face's middle rows: the small font has none) */
    r = DevChoiceKey(6, 0, 1); i = r.x + (r.w - DevTextWidth("SODA POP")) / 2;
    assert(Count(r.x, r.y, i - r.x, r.h, DEV_KEY_INK) == 0 && Count(i, r.y, 2, r.h, DEV_KEY_INK) > 0 && Count(r.x, r.y + 8, r.w, r.h - 16, DEV_WHITE) > 0);
    v = ChoiceBoard(4, 0); DevCheckTouch(&old, &v); r = DevChoiceKey(4, 0, 1);
    assert(Count(r.x, r.y, 12, r.h, DEV_KEY_INK) == 0 && Count(r.x + 12, r.y, 2, r.h, DEV_KEY_INK) > 0 && Count(r.x, r.y + 8, r.w, r.h - 16, DEV_WHITE) > 0);
    r = DevChoiceKey(4, 0, 0); assert(Count(r.x, r.y, 12, r.h + DEV_DROP, DEV_SUNK_INK) == 0 && Count(r.x + 12, r.y, 2, r.h + DEV_DROP, DEV_SUNK_INK) > 0);
    v = ChoiceBoard(2, 0); DevCheckTouch(&old, &v); r = DevChoiceKey(2, 0, 1); i = r.x + (r.w - DevTextWidth("NO")) / 2;
    assert(Count(r.x, r.y, i - r.x, r.h, DEV_KEY_INK) == 0 && Count(i, r.y, 2, r.h, DEV_KEY_INK) > 0 && Count(r.x, r.y + 8, r.w, r.h - 16, DEV_WHITE) > 0);

    /* the question on the note's paper in the paper's ink */
    ChoiceQuestion(&old, ChoiceBoard(4, 0));
    /* BACK in the exit corner: dark, sunk under a touch, grey while it cannot be used, gone (and not hit) while not
     * shown */
    v = ChoiceBoard(2, 0); DevCheckTouch(&old, &v);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 43, DEV_LED) == 0 && Count(244, 190, 68, 43, DEV_DARK_INK) > 0);
    v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); assert(LedAt(kDevGridExit));
    v.backPressed = 0; v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DIM_DARK);
    v.backEnabled = 1; v.backVisible = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DEV_SHELL_RED && !BackV(&v, 247, 210));
    v.backVisible = 1; DevCheckTouch(&old, &v);

    /* what the choice does not draw presents nothing; the shell's colour whole; the ways in and out (HOME; FireRed's
     * picture: in the frame's screen, nothing of the choice hit) */
    strcpy(v.bar[0], "Anything"); v.enabled[PT_BAG] = 1; v.playerX = 3;
    assert(DevCheckTouch(&old, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&old, &v);
    other = v; other.choice.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)));
    v = ChoiceBoard(2, 0); assert(DevWhole(DevCheckTouch(&old, &v)));
    v.viewport = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && DrawnBy(&v, DevFrameShown) && CtrPokeTouch_ChoiceHit(&v, 20, 20) == -1);
    v = ChoiceBoard(2, 0); DevCheckTouch(&old, &v);
    ChoiceUpperCase(ChoiceBoard(8, 0));
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first. */
    v = ChoiceBoard(2, 0); DevCheckTouch(&old, &v); shot("yesno");
    v = ChoiceBoard(5, 0); v.choice.cursor = 2; DevCheckTouch(&old, &v); shot("five");
    v = ChoiceBoard(6, 0); v.choice.cursor = 3; DevCheckTouch(&old, &v); shot("six");
    v = ChoiceBoard(8, 0); v.choice.cursor = 5; DevCheckTouch(&old, &v); shot("eight");
    v.choice.cols = 3; DevCheckTouch(&old, &v); shot("grid");
    v = ChoiceBoard(4, 0); strcpy(v.choice.label[1], "TWENTY-THREE CHARACTERS"); strcpy(v.choice.label[3], "CANCEL");
    assert(strlen(v.choice.label[1]) == 23);
    DevCheckTouch(&old, &v); shot("long");
    strcpy(v.choice.prompt, kChoiceThree); DevCheckTouch(&old, &v); shot("question");
    assert(sDevKeyCut == 0);                      /* main prints PASS device choice */
}
