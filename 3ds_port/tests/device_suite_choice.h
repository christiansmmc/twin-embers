/* ---- choice: the field's choices and YES / NO as the red device, through the field dispatcher (no board; plan 3
 * Task 4) ---------------------------------------------------------------------------------------------------------
 * Included by device_render_test.c before its kSuites. */

/* The note's paper (kDevChoiceNote 6 px in). */
static const CtrPokeTouchRect kChoicePaper = {14, 166, 216, 60};

static int ChoiceHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_ChoiceHit((const CtrPokeTouchView *)view, x, y);
}

static const char *const kChoiceLabel[PT_CHOICE_MAX] = {
    "FRESH WATER", "SODA POP", "LEMONADE", "WITHDRAW", "DEPOSIT", "MOVE ITEMS", "SEE YA!", "CANCEL",
};
/* Port-written stand-ins for FireRed's questions: one line, and one the normal font sets in five lines. */
static const char kChoicePrompt[] = "Port-written words for this test.";
static const char kChoiceLong[] = "Representative Accomplishment Responsibility Infrastructure Recommendation "
                                  "Characteristic Understandably Administration Transformation Overwhelmingly";

/* A choice as 3ds_bottom.c gives it (ChoiceSnapshot): n of FireRed's labels (YES and NO for two) as its menu prints
 * them, `cols` a grid's columns (0: FireRed's one column), the cursor on the first, the message box's question; BACK
 * shown and usable, nothing held. All eight labels are there, so a suite may change the count. */
static CtrPokeTouchView ChoiceBoard(int n, int cols)
{
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = v.selected = PT_NONE;
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

/* The plan's suite (every key hit where drawn in one column, two and a grid; BACK; the cursor sunk; what the choice
 * does not draw presents nothing), then the keys' looks, the layouts, the question, BACK's states, the ways in and out,
 * upper case and the states for the eye. */
static void SuiteChoice(void)
{
    CtrPokeTouchView v = ChoiceBoard(8, 0), old = v, other;
    CtrPokeTouchRect r;
    int n, i;

    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v = ChoiceBoard(8, 0); DevCheckTouch(&old, &v); assert(CtrPokeTouch_DeviceShown(&v));
    for (n = 1; n <= 8; ++n) {
        v.choice.count = (uint8_t)n; v.choice.cols = 0; DevCheckTouch(&old, &v);
        for (i = 0; i < n; ++i) AssertHits(ChoiceHitV, &v, DevChoiceKey(n, 0, i), PT_CHOICE_HIT_ROW + i);
    }
    assert(DevChoiceKey(6, 0, 3).x == 164 && DevChoiceKey(6, 0, 3).y == 8);
    v.choice.count = 8; v.choice.cols = 3; DevCheckTouch(&old, &v); AssertHits(ChoiceHitV, &v, DevChoiceKey(8, 3, 7), PT_CHOICE_HIT_ROW + 7);
    AssertKey(DevChoiceKey(8, 3, v.choice.cursor), DEV_LOOK_KEY, 1); AssertHits(BackV, &v, kDevChoiceBack, 1); assert(CtrPokeTouch_ChoiceHit(&v, 160, 200) == -1);
    v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* the layouts: up to four keys one column across the screen, more in two going down first (FireRed's one column), a
     * grid across; up to three rows 40 tall, four 32; every key of a grid of 2 and 3 hit where drawn */
    for (i = 0; i < 4; ++i)
        assert(DevChoiceKey(4, 0, i).x == 8 && DevChoiceKey(4, 0, i).w == 304 && DevChoiceKey(4, 0, i).y == 8 + 36 * i);
    assert(DevChoiceKey(3, 0, 2).h == 40 && DevChoiceKey(4, 0, 3).h == 32 && DevChoiceKey(8, 0, 7).h == 32);
    assert(DevChoiceKey(6, 0, 1).x == 8 && DevChoiceKey(6, 0, 1).y == 52 && DevChoiceKey(6, 0, 5).x == 164 && DevChoiceKey(6, 0, 5).y == 96);
    assert(DevChoiceKey(6, 2, 1).x == 164 && DevChoiceKey(6, 2, 1).y == 8 && DevChoiceKey(6, 2, 2).x == 8 && DevChoiceKey(6, 2, 2).y == 52);
    assert(DevChoiceKey(8, 3, 2).x == 216 && DevChoiceKey(8, 3, 3).y == 52 && DevChoiceKey(8, 3, 3).w == 96);
    for (int cols = 2; cols <= 3; ++cols)
        for (n = 1; n <= 8; ++n) {
            v.choice.count = (uint8_t)n; v.choice.cols = (uint8_t)cols; DevCheckTouch(&old, &v);
            for (i = 0; i < n; ++i) AssertHits(ChoiceHitV, &v, DevChoiceKey(n, cols, i), PT_CHOICE_HIT_ROW + i);
        }
    /* five keys in two columns: the right column's third place empty, the shell's, nothing hit */
    v = ChoiceBoard(5, 0); DevCheckTouch(&old, &v); r = DevChoiceKey(6, 0, 5);
    assert(at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED && CtrPokeTouch_ChoiceHit(&v, r.x + 10, r.y + 10) == -1);

    /* the keys: the cursor's sunk with its LED, the others up, FireRed's words on them; a touch sinks the key under it,
     * not the cursor's */
    v = ChoiceBoard(6, 0); v.choice.cursor = 4; DevCheckTouch(&old, &v);
    for (i = 0; i < 6; ++i) AssertKey(DevChoiceKey(6, 0, i), DEV_LOOK_KEY, i == 4);
    r = DevChoiceKey(6, 0, 4); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) > 0 && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_SUNK_INK) > 0);
    r = DevChoiceKey(6, 0, 0); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) == 0 && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_KEY_INK) > 0);
    v.choice.pressed = PT_CHOICE_HIT_ROW + 2; DevCheckTouch(&old, &v);
    AssertKey(DevChoiceKey(6, 0, 2), DEV_LOOK_KEY, 1); AssertKey(DevChoiceKey(6, 0, 4), DEV_LOOK_KEY, 0);
    v.choice.pressed = -1; DevCheckTouch(&old, &v);
    /* the question on the note's paper in the paper's ink; one the normal font sets in five lines in the small font (no
     * shadow), up to six; none, nothing */
    assert(CountIn(kChoicePaper, DEV_INK) > 0 && CountIn(kChoicePaper, DEV_INK_SH) > 0);
    strcpy(v.choice.prompt, kChoiceLong); assert(DevWhole(DevCheckTouch(&old, &v)));
    assert(CountIn(kChoicePaper, DEV_INK) > 0 && CountIn(kChoicePaper, DEV_INK_SH) == 0);
    v.choice.prompt[0] = 0; DevCheckTouch(&old, &v); assert(CountIn(kChoicePaper, DEV_INK) == 0);
    /* BACK: dark, sunk under a touch, grey while it cannot be used, gone (and not hit) while not shown */
    v = ChoiceBoard(2, 0); DevCheckTouch(&old, &v);
    AssertKey(kDevChoiceBack, DEV_LOOK_DARK, 0); assert(Count(244, 160, 68, 73, DEV_LED) == 0 && Count(244, 160, 68, 73, DEV_DARK_INK) > 0);
    v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevChoiceBack, DEV_LOOK_DARK, 1); assert(Count(244, 160, 68, 73, DEV_LED) > 0);
    v.backPressed = 0; v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(247, 195) == DIM_DARK);
    v.backEnabled = 1; v.backVisible = 0; DevCheckTouch(&old, &v); assert(at(247, 195) == DEV_SHELL_RED && !BackV(&v, 247, 195));
    v.backVisible = 1; DevCheckTouch(&old, &v);

    /* what the choice does not draw presents nothing; the shell's colour whole; the ways in and out (HOME; FireRed's
     * picture: not the device's, nothing hit) */
    strcpy(v.bar[0], "Anything"); v.registeredItem = 7; v.enabled[PT_BAG] = 1; v.playerX = 3;
    assert(DevCheckTouch(&old, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&old, &v);
    other = v; other.choice.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)));
    v = ChoiceBoard(2, 0); assert(DevWhole(DevCheckTouch(&old, &v)));
    v.viewport = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && !CtrPokeTouch_DeviceShown(&v) && CtrPokeTouch_ChoiceHit(&v, 20, 20) == -1);
    v = ChoiceBoard(2, 0); DevCheckTouch(&old, &v);
    ChoiceUpperCase(ChoiceBoard(8, 0));
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first. */
    v = ChoiceBoard(2, 0); DevCheckTouch(&old, &v); shot("yesno");
    v = ChoiceBoard(6, 0); v.choice.cursor = 3; DevCheckTouch(&old, &v); shot("six");
    v = ChoiceBoard(8, 0); v.choice.cursor = 5; DevCheckTouch(&old, &v); shot("eight");
    v.choice.cols = 3; DevCheckTouch(&old, &v); shot("grid");
    v = ChoiceBoard(4, 0); strcpy(v.choice.label[1], "TWENTY-THREE CHARACTERS"); assert(strlen(v.choice.label[1]) == 23);
    DevCheckTouch(&old, &v); shot("long");
    assert(sDevKeyCut == 0);                      /* main prints PASS device choice */
}
