/* ---- relearn: the move relearner as the red device (board def_relearn; plan 04a Task 15), through the field dispatcher
 * Included by device_render_test.c before its kSuites, after the learn suite (LearnDescenders). */

/* The papers it writes on: the grid's title, its screen, its narrow screen and its note (each screen's paper is 6 px
 * in). */
static const CtrPokeTouchRect kRlTitlePaper = {14, 12, 292, 16}, kListPaper = {14, 46, 248, 132};
static const CtrPokeTouchRect kNarrowPaper = {14, 46, 176, 132}, kDescPaper = {14, 196, 216, 30};

static int RlHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_RelearnHit((const CtrPokeTouchView *)view, x, y);
}

static const struct { const char *name, *type, *power, *accuracy; int8_t t; } kRlMove[8] = {
    {"EMBER", "FIRE", "40", "100", 10}, {"FLAMETHROWER", "FIRE", "95", "100", 10}, {"SLASH", "NORMAL", "70", "100", 0},
    {"DRAGON CLAW", "DRAGON", "80", "100", 16}, {"FIRE SPIN", "FIRE", "15", "70", 10},
    {"WING ATTACK", "FLYING", "60", "100", 2}, {"SCARY FACE", "NORMAL", "---", "90", 0}, {"HYPER BEAM", "NORMAL", "150", "90", 0},
};
static const char kRlDesc[] = "Port-written words in place of what a move does.";   /* two lines on the note */

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

/* A name in the normal font drawn whole from x at capitals `top` (the stub's capitals: rows top..top + 7): its last
 * glyph's ink there, and nothing from its end to `limit`. */
static void RlNameWhole(const char *name, int x, int top, int limit, uint16_t ink)
{
    int w = DevTextWidth(name);

    assert(x + w <= limit && Count(x + w - 4, top, 4, 8, ink) > 0 && Count(x + w, top - 3, limit - x - w, 16, ink) == 0);
}

/* The plan's suite (every row hit where drawn, the type's chip at 170 and the PP on a row off the cursor, CANCEL a dark
 * key hit as PT_RL_HIT_CANCEL, the description on the note; YES / NO in the column and nothing in the exit corner; ▲ ▼
 * by v0.6.0's rules, no row for FireRed's CANCEL row; a message hit only when it waits; nothing hit while it is set up;
 * what the view does not draw presents nothing), then def_relearn's pieces where the board puts them: the rows, the
 * footer, a touch, the scroll, CANCEL, YES / NO, the widest names, messages, the blank screen, no BACK, the ways in and
 * out, upper case and the states for the eye. */
static void SuiteRelearn(void)
{
    CtrPokeTouchView rv = RelearnBoard(), ro = rv, other;
    CtrPokeTouchRect r;
    int i, w, x, y;

    CtrPokeTouch_Draw(canvas, &ro);
    /* the plan's suite (the cursor on the first move) */
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv); assert(DrawnBy(&rv, DevRelearnShown));
    /* relearn */ for (i = 0; i < 5; ++i) AssertRowHits(RlHitV, &rv, DevGridRow(i, 0), PT_RL_HIT_ROW + i);
    r = DevGridRow(1, 0);                                                                  /* a row not under the cursor */
    assert(Count(170, r.y + 6, 40, 12, UiTypeColor(rv.relearn.row[1].type)) > 0 && Count(214, r.y + 8, 40, 8, DEV_INK) > 0);
    AssertHits(RlHitV, &rv, kDevGridExit, PT_RL_HIT_CANCEL); assert(Count(20, 209, 204, 17, DEV_INK) > 0);
    rv.relearn.panel = PT_RL_PANEL_YESNO; DevCheckTouch(&ro, &rv);
    AssertHits(RlHitV, &rv, DevGridColumnKey(40, 36, 30, 0), PT_RL_HIT_YES); AssertHits(RlHitV, &rv, DevGridColumnKey(40, 36, 30, 1), PT_RL_HIT_NO);
    assert(CtrPokeTouch_RelearnHit(&rv, 278, 210) == -1 && CtrPokeTouch_RelearnHit(&rv, 60, 60) == -1);
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 73) == -1); AssertHits(RlHitV, &rv, kDevGridDown, PT_RL_HIT_DOWN);
    for (i = 0; i < 4; ++i)                                                 /* the 2 px between two rows take nothing */
        assert(CtrPokeTouch_RelearnHit(&rv, 100, DevGridRow(i, 0).y + 24) == -1 && CtrPokeTouch_RelearnHit(&rv, 100, DevGridRow(i, 0).y + 25) == -1);
    rv.relearn.cursor = 8; RelearnRows(&rv, 3); DevCheckTouch(&ro, &rv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertRowHits(RlHitV, &rv, DevGridRow(4, 0), PT_RL_HIT_ROW + 4);
    rv.relearn.count = 3; rv.relearn.cursor = 3; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 130, DevGridRow(3, 0).y + 5) == -1);
    rv.relearn.panel = PT_RL_PANEL_MESSAGE; rv.relearn.tap = 1; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == PT_RL_HIT_MESSAGE);
    rv.relearn.tap = 0; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == -1);
    rv.relearn.panel = PT_RL_PANEL_BLANK; DevCheckTouch(&ro, &rv); for (y = 0; y < 240; y += 4) for (x = 0; x < 320; x += 4) assert(CtrPokeTouch_RelearnHit(&rv, x, y) == -1);
    rv.mapGroup ^= 1; rv.speed ^= 1; assert(DevCheckTouch(&ro, &rv).count == 0);
    assert(PT_RL_HIT_CANCEL == 235);

    /* LIST (def_relearn): the question on the title; five rows 24 tall and 26 apart, the cursor's inverted, the others
     * the paper's, each the move's name from x 22 (capitals r.y + 8, over its shadow off the cursor's row) cut before
     * 166, its type's chip 12 tall at (170, r.y + 6), "PP n" ending at 254 (capitals r.y + 8, the row's ink, no
     * shadow); the 2 px between two rows the paper's; ▲ grey at the top with the cursor on the first move, ▼ dark;
     * CANCEL dark, up */
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv);
    assert(CountIn(kRlTitlePaper, DEV_INK) > 0);
    for (i = 0; i < PT_RL_ROWS; ++i) {
        uint16_t bg = i == 0 ? DEV_BEZEL : DEV_PAPER, ink = DevRowInk(i == 0), type = UiTypeColor(kRlMove[i].t);
        char pp[16];

        snprintf(pp, sizeof(pp), "PP %s", rv.relearn.row[i].pp);
        w = DevTextWidth(pp);
        r = DevGridRow(i, 0);
        assert(at(r.x + 1, r.y + 1) == bg && at(r.x + r.w - 1, r.y + r.h - 1) == bg);
        assert(at(170, r.y + 6) == type && at(209, r.y + 17) == type && at(169, r.y + 10) == bg && at(210, r.y + 10) == bg);
        assert(at(170, r.y + 5) == bg && at(170, r.y + 18) == bg);
        assert(Count(22, r.y + 8, 80, 8, ink) > 0 && Count(r.x, r.y, 150, 8, ink) == 0 && Count(r.x, r.y, 6, r.h, ink) == 0);
        assert(Count(166, r.y, 4, r.h, ink) == 0 && Count(211, r.y, 254 - w - 211, r.h, ink) == 0);
        assert(Count(254 - w, r.y + 8, w, 8, ink) > 0 && Count(254 - 4, r.y + 8, 4, 8, ink) > 0 && Count(254, r.y, 6, r.h, ink) == 0);
        assert(Count(211, r.y, 49, r.h, DEV_INK_SH) == 0 && (i == 0) == (Count(22, r.y + 16, 80, 1, DEV_INK_SH) == 0));
        if (i < PT_RL_ROWS - 1)
            assert(Count(r.x, r.y + 24, r.w, 2, DEV_PAPER) == r.w * 2);
    }
    assert(Count(276, 40, 36, 66, DEV_ARROW_OFF) > 0 && Count(276, 114, 36, 66, DEV_KEY_INK) > 0);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 43, DEV_LED) == 0 && Count(244, 190, 68, 43, DEV_DARK_INK) > 0);
    /* the footer (the grid's note, its paper 14..229 x 196..225; Call 13): the cursor's move's type on its chip 10 tall at
     * (20, 198); "POW n  ACC n" small in the secondary ink 4 px after it, capitals 198; FireRed's description small in
     * the paper's ink, two lines from x 20 to 224, capitals 209 and 218 (DEV_SMALL_LINE_TIGHT) */
    assert(CountIn(kDescPaper, UiTypeColor(10)) > 0 && CountIn(kDescPaper, DEV_INK2) > 0 && CountIn(kDescPaper, DEV_INK) > 0);
    w = DevTypeChipWidth("FIRE");
    assert(at(20, 198) == UiTypeColor(10) && at(19 + w, 207) == UiTypeColor(10) && at(19, 202) == DEV_PAPER && at(20 + w, 202) == DEV_PAPER);
    assert(at(20, 197) == DEV_PAPER && at(20, 208) == DEV_PAPER && Count(14, 196, 6, 30, UiTypeColor(10)) == 0);
    {
        char stats[48];
        int sw;

        DevPowAcc(stats, sizeof(stats), "40", "100");
        sw = DevSmallTextWidth(stats);
        x = 20 + w + 4;
        assert(Count(x, 198, sw, 7, DEV_INK2) > 0 && Count(x + sw - 4, 198, 4, 7, DEV_INK2) > 0 && x + sw <= 224);
        assert(Count(20 + w, 196, 4, 30, DEV_INK2) == 0 && Count(x + sw, 196, 230 - x - sw, 30, DEV_INK2) == 0);
        assert(Count(14, 206, 216, 20, DEV_INK2) == 0);
    }
    assert(Count(20, 209, 204, 7, DEV_INK) > 0 && Count(20, 218, 204, 7, DEV_INK) > 0 && Count(14, 196, 216, 11, DEV_INK) == 0);
    assert(Count(14, 196, 6, 30, DEV_INK) == 0 && Count(224, 196, 6, 30, DEV_INK) == 0);
    /* the description's last line: with a descender on every small glyph (the game's g, p, q, y and comma have theirs in
     * glyph row 11: LearnDescenders) it ends on the paper's last row, 225, and none reaches the bezel under it */
    LearnDescenders(1); CtrPokeTouch_Draw(canvas, &rv);
    assert(Count(20, 225, 204, 1, DEV_INK) > 0 && Count(14, 226, 216, 5, DEV_INK) == 0 && Count(14, 226, 216, 5, DEV_INK2) == 0);
    LearnDescenders(0); CtrPokeTouch_Draw(canvas, &ro);
    /* the cursor past the first move: ▲ on and hit */
    rv.relearn.cursor = 1; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv);
    AssertHits(RlHitV, &rv, kDevGridUp, PT_RL_HIT_UP); assert(Count(276, 40, 36, 66, DEV_KEY_INK) > 0);
    r = DevGridRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    /* a touch: the row under it inverted, not the cursor's; ▼ sunk (the cursor's row stays); CANCEL sunk with its LED */
    rv.relearn.pressed = PT_RL_HIT_ROW + 3; DevCheckTouch(&ro, &rv);
    r = DevGridRow(3, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); r = DevGridRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
    rv.relearn.pressed = PT_RL_HIT_DOWN; DevCheckTouch(&ro, &rv); AssertKey(kDevGridDown, DEV_LOOK_KEY, 1); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    rv.relearn.pressed = PT_RL_HIT_CANCEL; DevCheckTouch(&ro, &rv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    assert(Count(244, 190, 68, 43, DEV_LED) > 0 && at(r.x + 1, r.y + 1) == DEV_PAPER);
    /* ▲ held where it cannot go: not sunk, the cursor's row stays */
    rv.relearn.cursor = 0; RelearnRows(&rv, 0); rv.relearn.pressed = PT_RL_HIT_UP; DevCheckTouch(&ro, &rv);
    AssertKey(kDevGridUp, DEV_LOOK_KEY, 0); r = DevGridRow(0, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    rv.relearn.pressed = -1; DevCheckTouch(&ro, &rv);
    /* the scroll: from the third move, the cursor on the fifth */
    rv.relearn.cursor = 4; RelearnRows(&rv, 2); DevCheckTouch(&ro, &rv);
    r = DevGridRow(2, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); AssertHits(RlHitV, &rv, kDevGridUp, PT_RL_HIT_UP);
    /* the end (RelearnFill's window ends at the last move): the last five moves, none inverted; CANCEL sunk with its LED;
     * ▼ grey and not hit; nothing to describe. A list shorter than the window: FireRed's CANCEL row after the last move is
     * no row (the paper's, not hit) */
    rv.relearn.cursor = 8; RelearnRows(&rv, 3); DevCheckTouch(&ro, &rv);
    for (i = 0; i < PT_RL_ROWS; ++i) { r = DevGridRow(i, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER && Count(r.x, r.y, 80, r.h, DEV_INK) > 0); }
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); assert(Count(244, 190, 68, 43, DEV_LED) > 0);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0);
    assert(CountIn(kDescPaper, DEV_INK) == 0 && CountIn(kDescPaper, DEV_INK2) == 0);
    rv.relearn.count = 3; rv.relearn.cursor = 3; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv);
    r = DevGridRow(3, 0); assert(Count(r.x, r.y, r.w, r.h, DEV_PAPER) == r.w * r.h && CtrPokeTouch_RelearnHit(&rv, r.x + 10, r.y + 10) == -1);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);

    /* YES / NO (the Decision: the keys in the column, no key in the exit corner): the question on the grid's narrow
     * screen (its paper 14..189 x 46..177), YES and NO cream in the column (DevGridColumnKey(40, 36, 30, i)), their words
     * from x + 10, FireRed's cursor sunk (or the key under a held touch); the cursor's move on the grid's note: its type's
     * chip 12 tall at (20, 205), its name 6 px after it, capitals 207; ▲ ▼ and CANCEL neither drawn nor hit */
    rv = RelearnBoard(); rv.relearn.cursor = 1; RelearnRows(&rv, 0);
    rv.relearn.panel = PT_RL_PANEL_YESNO; strcpy(rv.relearn.message, "Port-written words of a question?"); DevCheckTouch(&ro, &rv);
    assert(CountIn(kNarrowPaper, DEV_INK) > 0 && at(189, 177) == DEV_PAPER && at(190, 100) == DEV_BEZEL);
    assert(at(196, 60) == DEV_SHELL_RED && at(203, 60) == DEV_SHELL_RED && at(200, 150) == DEV_SHELL_RED);
    for (i = 0; i < 2; ++i) {
        CtrPokeTouchRect k = DevGridColumnKey(40, 36, 30, i);
        uint16_t ink = DevKeyInk(DEV_LOOK_KEY, i == 0);

        AssertKey(k, DEV_LOOK_KEY, i == 0);
        assert(Count(k.x + 10, k.y, 40, k.h + DEV_DROP, ink) > 0 && Count(k.x, k.y, 10, k.h + DEV_DROP, ink) == 0);
    }
    w = DevTypeChipWidth("FIRE");
    assert(at(20, 205) == UiTypeColor(10) && at(19 + w, 216) == UiTypeColor(10) && at(20, 204) == DEV_PAPER && at(20, 217) == DEV_PAPER);
    assert(Count(26 + w, 207, 150, 8, DEV_INK) > 0 && Count(14, 196, 216, 9, DEV_INK) == 0 && CountIn(kDescPaper, DEV_INK2) == 0);
    RlNameWhole("FLAMETHROWER", 26 + w, 207, 224, DEV_INK);
    assert(at(247, 210) == DEV_SHELL_RED && at(278, 210) == DEV_SHELL_RED && at(294, 150) == DEV_SHELL_RED);
    assert(CtrPokeTouch_RelearnHit(&rv, 247, 210) == -1 && CtrPokeTouch_RelearnHit(&rv, 294, 150) == -1 && CtrPokeTouch_RelearnHit(&rv, 30, 60) == -1);
    rv.relearn.menuCursor = 1; DevCheckTouch(&ro, &rv);
    AssertKey(DevGridColumnKey(40, 36, 30, 0), DEV_LOOK_KEY, 0); AssertKey(DevGridColumnKey(40, 36, 30, 1), DEV_LOOK_KEY, 1);
    rv.relearn.pressed = PT_RL_HIT_YES; DevCheckTouch(&ro, &rv);
    AssertKey(DevGridColumnKey(40, 36, 30, 0), DEV_LOOK_KEY, 1); AssertKey(DevGridColumnKey(40, 36, 30, 1), DEV_LOOK_KEY, 0);
    rv.relearn.pressed = -1; DevCheckTouch(&ro, &rv);
    /* FireRed's widest move names (12 glyphs, as SWORDS DANCE: 72 px in the game's normal font) keep every letter, on a
     * row (room to 166) and on the question's note (to 224), in every language; a name too wide for either is cut there
     * and counted, as a cut key label is */
    rv = RelearnBoard(); strcpy(rv.relearn.row[1].name, "SWORDS DANCE"); DevCheckTouch(&ro, &rv);
    r = DevGridRow(1, 0); RlNameWhole("SWORDS DANCE", 22, r.y + 8, 166, DEV_INK); assert(sDevKeyCut == 0);
    rv.relearn.cursor = 1; rv.relearn.panel = PT_RL_PANEL_YESNO; strcpy(rv.relearn.message, "Port-written words of a question?");
    DevCheckTouch(&ro, &rv); RlNameWhole("SWORDS DANCE", 26 + w, 207, 224, DEV_INK); assert(sDevKeyCut == 0);
    {
        static uint8_t keepWidths[256];

        memcpy(keepWidths, fontWidths, sizeof(keepWidths));
        fontWidths[0xd1] = 11;                          /* W 11 px: fifteen of them 165 px, past both rooms */
        memset(sPtFontReady, 0, sizeof(sPtFontReady));
        strcpy(rv.relearn.row[1].name, "WWWWWWWWWWWWWWW");
        rv.relearn.panel = PT_RL_PANEL_LIST; sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &rv);   /* the cursor's row */
        assert(sDevKeyCut == 1 && Count(166, r.y, 4, r.h, DevRowInk(1)) == 0 && Count(22, r.y + 8, 140, 8, DevRowInk(1)) > 0);
        rv.relearn.panel = PT_RL_PANEL_YESNO; sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &rv);
        assert(sDevKeyCut == 1 && Count(224, 196, 6, 30, DEV_INK) == 0 && Count(26 + w, 207, 150, 8, DEV_INK) > 0);
        sDevKeyCut = 0;
        memcpy(fontWidths, keepWidths, sizeof(keepWidths));
        memset(sPtFontReady, 0, sizeof(sPtFontReady));
        CtrPokeTouch_Draw(canvas, &ro);
    }
    rv = RelearnBoard(); rv.relearn.cursor = 1; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv);
    /* a message: on the list's screen, FireRed's ▼ while it waits for A, the keys grey; no row, nothing but it hit */
    rv.relearn.panel = PT_RL_PANEL_MESSAGE; rv.relearn.tap = 1; strcpy(rv.relearn.message, "Port-written words of a message.");
    DevCheckTouch(&ro, &rv);
    assert(CountIn(kListPaper, DEV_INK) > 0 && Count(242, 160, 20, 18, DEV_INK2) > 0);
    r = DevGridRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
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
