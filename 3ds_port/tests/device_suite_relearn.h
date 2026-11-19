/* ---- relearn: the move relearner as the red device, through the field dispatcher (no board; plan 3 Task 4) --------
 * Included by device_render_test.c before its kSuites. */

/* The papers it writes on: the Bag's title, list, narrow list and the two descriptions (each screen's paper is 6 px
 * in). */
static const CtrPokeTouchRect kRlTitlePaper = {14, 12, 292, 16}, kListPaper = {14, 46, 248, 132};
static const CtrPokeTouchRect kNarrowPaper = {14, 46, 172, 132}, kDescPaper = {14, 196, 216, 30};
static const CtrPokeTouchRect kNarrowDescPaper = {14, 196, 172, 30};

static int RlHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_RelearnHit((const CtrPokeTouchView *)view, x, y);
}

static const struct { const char *name, *type, *power, *accuracy; int8_t t; } kRlMove[8] = {
    {"EMBER", "FIRE", "40", "100", 10}, {"FLAMETHROWER", "FIRE", "95", "100", 10}, {"SLASH", "NORMAL", "70", "100", 0},
    {"DRAGON CLAW", "DRAGON", "80", "100", 16}, {"FIRE SPIN", "FIRE", "15", "70", 10},
    {"WING ATTACK", "FLYING", "60", "100", 2}, {"SCARY FACE", "NORMAL", "---", "90", 0}, {"HYPER BEAM", "NORMAL", "150", "90", 0},
};
static const char kRlDesc[] = "Port-written words for this test, in place of what a move does.";

/* The list's rows from `first` as RelearnFill reads them: the moves, FireRed's CANCEL row (no move) after the last,
 * nothing past it; the description of the cursor's move. */
static void RelearnRows(CtrPokeTouchView *v, int first)
{
    CtrPokeTouchRelearn *l = &v->relearn;

    l->first = (uint8_t)first;
    memset(l->row, 0, sizeof(l->row));
    l->desc[0] = 0;
    for (int i = 0; i < PT_RL_ROWS; ++i) {
        int k = first + i;

        l->row[i].type = -1;
        if (k > l->count)
            break;
        if (k == l->count) {
            strcpy(l->row[i].name, "CANCEL");
            continue;
        }
        l->row[i].move = (uint16_t)(k + 1);
        l->row[i].type = kRlMove[k].t;
        strcpy(l->row[i].name, kRlMove[k].name);
        strcpy(l->row[i].typeName, kRlMove[k].type);
        strcpy(l->row[i].pp, "15");
        strcpy(l->row[i].power, kRlMove[k].power);
        strcpy(l->row[i].accuracy, kRlMove[k].accuracy);
        if (k == l->cursor)
            strcpy(l->desc, kRlDesc);
    }
}

/* The relearner as 3ds_bottom.c gives it (RelearnFill, RelearnSnapshot): eight moves it can remember and CANCEL, the
 * list from its first row, the cursor on the first move; port-written stand-ins for FireRed's question and words. */
static CtrPokeTouchView RelearnBoard(void)
{
    CtrPokeTouchView v;
    CtrPokeTouchRelearn *l = &v.relearn;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    l->on = 1;
    l->panel = PT_RL_PANEL_LIST;
    l->count = 8;
    l->cursor = 0;
    l->pressed = -1;
    l->species = 6;
    strcpy(l->nickname, "Charizard");
    strcpy(l->level, "36");
    strcpy(l->question, "Port-written question for Charizard?");
    RelearnRows(&v, 0);
    snprintf(v.bar[0], PT_BAR_LEN, "Relearner");                /* the bar: not drawn by the relearner */
    v.backVisible = v.backEnabled = 1;                          /* RelearnSnapshot's: the relearner draws no BACK */
    return v;
}

/* Upper case (owner decision 5): FireRed's move and type names, which v0.6.0 title-cased, in capitals; FireRed's
 * question, description and messages as they are. */
static void RelearnUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    for (int i = 0; i < PT_RL_ROWS; ++i) {
        CtrTextCase(cased.relearn.row[i].name);
        CtrTextCase(cased.relearn.row[i].typeName);
    }
    TouchSamePixels(&cased, &v, 1, "the moves and types in v0.6.0's case and as FireRed writes them");
    cased = v;
    DevUpper(cased.relearn.question, v.relearn.question, sizeof(cased.relearn.question));
    TouchSamePixels(&cased, &v, 0, "the question, as FireRed writes it");
    cased = v;
    DevUpper(cased.relearn.desc, v.relearn.desc, sizeof(cased.relearn.desc));
    TouchSamePixels(&cased, &v, 0, "the description, as FireRed writes it");
    v.relearn.panel = PT_RL_PANEL_MESSAGE;
    strcpy(v.relearn.message, "Port-written words of a message.");
    cased = v;
    DevUpper(cased.relearn.message, v.relearn.message, sizeof(cased.relearn.message));
    TouchSamePixels(&cased, &v, 0, "a message, as FireRed writes it");
}

/* The plan's suite (every row hit where drawn, ▲ ▼ by v0.6.0's rules, CANCEL a dark key sunk on FireRed's CANCEL row
 * and hit as PT_RL_HIT_CANCEL, no row for it; YES / NO; a message hit only when it waits; nothing hit while it is set
 * up; what the view does not draw presents nothing), then the list's looks, a touch, the scroll, CANCEL, YES / NO,
 * messages, the blank screen, no BACK, the ways in and out, upper case and the states for the eye. */
static void SuiteRelearn(void)
{
    CtrPokeTouchView rv = RelearnBoard(), ro = rv, other;
    CtrPokeTouchRect r;
    int i, x, y;

    CtrPokeTouch_Draw(canvas, &ro);
    /* the plan's suite */
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv); assert(DrawnBy(&rv, DevRelearnShown));
    /* relearn */ for (i = 0; i < 5; ++i) AssertRowHits(RlHitV, &rv, DevTallRow(i, 0), PT_RL_HIT_ROW + i);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 73) == -1); AssertHits(RlHitV, &rv, kDevGridDown, PT_RL_HIT_DOWN); AssertHits(RlHitV, &rv, kDevGridExit, PT_RL_HIT_CANCEL);
    rv.relearn.cursor = 8; RelearnRows(&rv, 3); DevCheckTouch(&ro, &rv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertRowHits(RlHitV, &rv, DevTallRow(4, 0), PT_RL_HIT_ROW + 4);
    rv.relearn.count = 3; rv.relearn.cursor = 3; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 130, DevTallRow(3, 0).y + 5) == -1);
    rv.relearn.panel = PT_RL_PANEL_YESNO; DevCheckTouch(&ro, &rv); AssertHits(RlHitV, &rv, DevRlKey(0), PT_RL_HIT_YES); AssertHits(RlHitV, &rv, DevRlKey(1), PT_RL_HIT_NO); assert(CtrPokeTouch_RelearnHit(&rv, 60, 60) == -1);
    rv.relearn.panel = PT_RL_PANEL_MESSAGE; rv.relearn.tap = 1; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == PT_RL_HIT_MESSAGE);
    rv.relearn.tap = 0; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == -1);
    rv.relearn.panel = PT_RL_PANEL_BLANK; DevCheckTouch(&ro, &rv); for (y = 0; y < 240; y += 4) for (x = 0; x < 320; x += 4) assert(CtrPokeTouch_RelearnHit(&rv, x, y) == -1);
    rv.mapGroup ^= 1; rv.speed ^= 1; assert(DevCheckTouch(&ro, &rv).count == 0);
    assert(PT_RL_HIT_CANCEL == 235);

    /* LIST: the question on the title; the cursor's row inverted, the others the paper's, each its name and its type's
     * chip; ▲ grey at the top with the cursor on the first move, ▼ dark; the cursor's move's type, power and accuracy
     * and description on the description's screen; CANCEL dark, up */
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv);
    assert(CountIn(kRlTitlePaper, DEV_INK) > 0);
    for (i = 0; i < PT_RL_ROWS; ++i) {
        r = DevTallRow(i, 0);
        assert(at(r.x + 1, r.y + 1) == (i == 0 ? DEV_BEZEL : DEV_PAPER));
        assert(Count(r.x, r.y, 80, r.h, i == 0 ? DEV_PAPER : DEV_INK) > 0 && Count(r.x + r.w - 60, r.y, 60, r.h, UiTypeColor(kRlMove[i].t)) > 0);
    }
    assert(Count(276, 40, 36, 66, DEV_ARROW_OFF) > 0 && Count(276, 114, 36, 66, DEV_KEY_INK) > 0);
    assert(CountIn(kDescPaper, UiTypeColor(10)) > 0 && CountIn(kDescPaper, DEV_INK2) > 0 && CountIn(kDescPaper, DEV_INK) > 0);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 43, DEV_LED) == 0 && Count(244, 190, 68, 43, DEV_DARK_INK) > 0);
    /* the cursor past the first move: ▲ on and hit */
    rv.relearn.cursor = 1; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv);
    AssertHits(RlHitV, &rv, kDevGridUp, PT_RL_HIT_UP); assert(Count(276, 40, 36, 66, DEV_KEY_INK) > 0);
    r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    /* a touch: the row under it inverted, not the cursor's; ▼ sunk (the cursor's row stays); CANCEL sunk with its LED */
    rv.relearn.pressed = PT_RL_HIT_ROW + 3; DevCheckTouch(&ro, &rv);
    r = DevTallRow(3, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
    rv.relearn.pressed = PT_RL_HIT_DOWN; DevCheckTouch(&ro, &rv); AssertKey(kDevGridDown, DEV_LOOK_KEY, 1); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    rv.relearn.pressed = PT_RL_HIT_CANCEL; DevCheckTouch(&ro, &rv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    assert(Count(244, 190, 68, 43, DEV_LED) > 0 && at(r.x + 1, r.y + 1) == DEV_PAPER);
    /* ▲ held where it cannot go: not sunk, the cursor's row stays */
    rv.relearn.cursor = 0; RelearnRows(&rv, 0); rv.relearn.pressed = PT_RL_HIT_UP; DevCheckTouch(&ro, &rv);
    AssertKey(kDevGridUp, DEV_LOOK_KEY, 0); r = DevTallRow(0, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    rv.relearn.pressed = -1; DevCheckTouch(&ro, &rv);
    /* the scroll: from the third move, the cursor on the fifth */
    rv.relearn.cursor = 4; RelearnRows(&rv, 2); DevCheckTouch(&ro, &rv);
    r = DevTallRow(2, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); AssertHits(RlHitV, &rv, kDevGridUp, PT_RL_HIT_UP);
    /* the end (RelearnFill's window ends at the last move): the last five moves, none inverted; CANCEL sunk with its LED;
     * ▼ grey and not hit; nothing to describe. A list shorter than the window: FireRed's CANCEL row after the last move is
     * no row (the paper's, not hit) */
    rv.relearn.cursor = 8; RelearnRows(&rv, 3); DevCheckTouch(&ro, &rv);
    for (i = 0; i < PT_RL_ROWS; ++i) { r = DevTallRow(i, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER && Count(r.x, r.y, 80, r.h, DEV_INK) > 0); }
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); assert(Count(244, 190, 68, 43, DEV_LED) > 0);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0);
    assert(CountIn(kDescPaper, DEV_INK) == 0 && CountIn(kDescPaper, DEV_INK2) == 0);
    rv.relearn.count = 3; rv.relearn.cursor = 3; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv);
    r = DevTallRow(3, 0); assert(Count(r.x, r.y, r.w, r.h, DEV_PAPER) == r.w * r.h && CtrPokeTouch_RelearnHit(&rv, r.x + 10, r.y + 10) == -1);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);

    /* YES / NO: the question on the narrow screen, YES and NO in the Bag's place, FireRed's cursor sunk (or the key under
     * a held touch); the cursor's move on the narrow description; ▲ ▼ and CANCEL neither drawn nor hit */
    rv = RelearnBoard(); rv.relearn.cursor = 1; RelearnRows(&rv, 0);
    rv.relearn.panel = PT_RL_PANEL_YESNO; strcpy(rv.relearn.message, "Port-written words of a question?"); DevCheckTouch(&ro, &rv);
    assert(CountIn(kNarrowPaper, DEV_INK) > 0 && at(196, 60) == DEV_SHELL_RED);
    AssertKey(DevRlKey(0), DEV_LOOK_KEY, 1); AssertKey(DevRlKey(1), DEV_LOOK_KEY, 0);
    assert(CountIn(kNarrowDescPaper, UiTypeColor(10)) > 0 && CountIn(kNarrowDescPaper, DEV_INK) > 0);
    assert(at(247, 210) == DEV_SHELL_RED && at(294, 150) == DEV_SHELL_RED && CtrPokeTouch_RelearnHit(&rv, 247, 210) == -1);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 150) == -1 && CtrPokeTouch_RelearnHit(&rv, 30, 60) == -1);
    rv.relearn.menuCursor = 1; DevCheckTouch(&ro, &rv); AssertKey(DevRlKey(0), DEV_LOOK_KEY, 0); AssertKey(DevRlKey(1), DEV_LOOK_KEY, 1);
    rv.relearn.pressed = PT_RL_HIT_YES; DevCheckTouch(&ro, &rv); AssertKey(DevRlKey(0), DEV_LOOK_KEY, 1); AssertKey(DevRlKey(1), DEV_LOOK_KEY, 0);
    rv.relearn.pressed = -1; DevCheckTouch(&ro, &rv);
    /* a message: on the list's screen, FireRed's ▼ while it waits for A, the keys grey; no row, nothing but it hit */
    rv.relearn.panel = PT_RL_PANEL_MESSAGE; rv.relearn.tap = 1; strcpy(rv.relearn.message, "Port-written words of a message.");
    DevCheckTouch(&ro, &rv);
    assert(CountIn(kListPaper, DEV_INK) > 0 && Count(242, 160, 20, 18, DEV_INK2) > 0);
    r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
    assert(at(247, 210) == DIM_DARK && at(279, 73) == DevBlend(DEV_SHELL_RED, DEV_KEY, DEV_A_DIM));
    for (y = 0; y < 240; y += 8) for (x = 0; x < 320; x += 8) assert(CtrPokeTouch_RelearnHit(&rv, x, y) == PT_RL_HIT_MESSAGE);
    rv.relearn.tap = 0; DevCheckTouch(&ro, &rv); assert(Count(242, 160, 20, 18, DEV_INK2) == 0);
    /* while it is set up: empty screens, the keys grey, nothing hit */
    rv = RelearnBoard(); memset(&rv.relearn, 0, sizeof(rv.relearn)); rv.relearn.on = 1; rv.relearn.panel = PT_RL_PANEL_BLANK;
    DevCheckTouch(&ro, &rv);
    assert(CountIn(kRlTitlePaper, DEV_INK) == 0 && CountIn(kListPaper, DEV_INK) == 0 && CountIn(kDescPaper, DEV_INK) == 0);
    assert(at(16, 50) == DEV_PAPER && at(247, 210) == DIM_DARK && at(279, 73) == DevBlend(DEV_SHELL_RED, DEV_KEY, DEV_A_DIM));
    /* no BACK in any panel: CANCEL and NO are its B */
    for (int panel = PT_RL_PANEL_LIST; panel <= PT_RL_PANEL_BLANK; ++panel) {
        rv = RelearnBoard(); rv.relearn.panel = (uint8_t)panel; rv.relearn.tap = 1;
        for (y = 0; y < 240; y += 4) for (x = 0; x < 320; x += 4) assert(!CtrPokeTouch_BackHit(&rv, x, y));
    }

    /* what the relearner does not draw presents nothing: BACK's fields, the Pokemon (no column), the YES / NO cursor and
     * the wait for A outside their panels, the bar; the shell's colour whole; the ways in and out (HOME; FireRed's
     * picture: in the frame's screen, nothing of the relearner hit) */
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv);
    rv.backPressed = 1; rv.backEnabled = 0; rv.relearn.species = 7; strcpy(rv.relearn.nickname, "Blaze"); strcpy(rv.relearn.level, "37");
    rv.relearn.menuCursor = 1; rv.relearn.tap = 1; strcpy(rv.bar[0], "Anything"); rv.playerX = 3;
    assert(DevCheckTouch(&ro, &rv).count == 0);
    rv.relearn.pressed = PT_RL_HIT_UP; assert(DevCheckTouch(&ro, &rv).count == 0);         /* ▲ cannot go: not drawn sunk */
    rv.relearn.pressed = -1; DevCheckTouch(&ro, &rv);
    rv.shell = 2; assert(DevWhole(DevCheckTouch(&ro, &rv)) && at(1, 100) == DEV_SHELL_GREEN);
    rv.shell = 0; DevCheckTouch(&ro, &rv);
    other = rv; other.relearn.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&ro, &other)));
    rv = RelearnBoard(); assert(DevWhole(DevCheckTouch(&ro, &rv)));
    other = rv; memset(&other.relearn, 0, sizeof(other.relearn)); other.viewport = 1;
    assert(DevWhole(DevCheckTouch(&ro, &other)) && DrawnBy(&other, DevFrameShown) && CtrPokeTouch_RelearnHit(&other, 130, 60) == -1);
    rv.viewport = 1; assert(DrawnBy(&rv, DevFrameShown) && CtrPokeTouch_RelearnHit(&rv, 130, 60) == -1);
    rv = RelearnBoard(); assert(DevWhole(DevCheckTouch(&ro, &rv)));
    RelearnUpperCase(RelearnBoard());
    CtrPokeTouch_Draw(canvas, &ro);

    /* The states for the eye, each through Update first. */
    rv = RelearnBoard(); rv.relearn.cursor = 1; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv); shot("list");
    rv.relearn.cursor = 4; RelearnRows(&rv, 2); DevCheckTouch(&ro, &rv); shot("scroll");
    rv.relearn.cursor = 8; RelearnRows(&rv, 3); DevCheckTouch(&ro, &rv); shot("cancel");
    rv.relearn.cursor = 1; RelearnRows(&rv, 0); rv.relearn.panel = PT_RL_PANEL_YESNO;
    strcpy(rv.relearn.message, "Port-written words of a question, long enough for a few lines on the narrow screen?");
    DevCheckTouch(&ro, &rv); shot("yesno");
    rv.relearn.panel = PT_RL_PANEL_MESSAGE; strcpy(rv.relearn.message, "Port-written words of a message.");
    DevCheckTouch(&ro, &rv); shot("message");
    rv.relearn.tap = 1; DevCheckTouch(&ro, &rv); shot("message_tap");
    memset(&rv.relearn, 0, sizeof(rv.relearn)); rv.relearn.on = 1; rv.relearn.panel = PT_RL_PANEL_BLANK;
    DevCheckTouch(&ro, &rv); shot("blank");
    assert(sDevKeyCut == 0);                      /* main prints PASS device relearn */
}
