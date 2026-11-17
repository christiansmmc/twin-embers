/* ---- choice, learn, relearn: the field's choices and YES / NO, the move to forget and the relearner as the red device,
 * through the field dispatcher (no board; plan 3 Task 4) ----------------------------------------------------------------
 * Included by device_render_test.c before its kSuites: three suites. Also the harness's species picture:
 * CtrPokeTouch_DexMonPic gives a 48x48 square in STUB_PIC (a clear border of 8) while a suite sets picArt (none
 * otherwise, as the other art stubs); a suite that sets it forgets the pictures kept (sFDPic). */

#define PIC_ART_CYAN 0x7fe0                     /* GBA BGR555 */
#define STUB_PIC GbaColor(PIC_ART_CYAN)
static int picArt;

int CtrPokeTouch_DexMonPic(unsigned species, unsigned char *tiles, unsigned short *palette)
{
    (void)species;
    if (!picArt) return 0;
    memset(tiles, 0, 0x800); memset(palette, 0, 32); palette[1] = PIC_ART_CYAN;
    for (int y = 8; y < 56; ++y)
        for (int x = 8; x < 56; ++x)
            tiles[(y / 8 * 8 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2] |= (unsigned char)(1 << (x & 1) * 4);
    return 1;
}

/* The pictures kept by species (3ds_device_dex_impl.h), forgotten: the next one asks the stub again. */
static void ForgetPictures(void)
{
    memset(sFDPic, 0, sizeof(sFDPic));
    sFDPicNext = 0;
}

/* The papers the views write on: the choice's note, the Summary's note, the Bag's title, list, narrow list and the two
 * descriptions (each screen's paper is 6 px in). */
static const CtrPokeTouchRect kChoicePaper = {14, 166, 216, 60}, kSumNotePaper = {14, 194, 216, 32};
static const CtrPokeTouchRect kTitlePaper = {14, 12, 292, 16}, kListPaper = {14, 46, 248, 132};
static const CtrPokeTouchRect kNarrowPaper = {14, 46, 172, 132}, kDescPaper = {14, 196, 216, 30};
static const CtrPokeTouchRect kNarrowDescPaper = {14, 196, 172, 30};

static int CountIn(CtrPokeTouchRect r, uint16_t c)
{
    return Count(r.x, r.y, r.w, r.h, c);
}

/* A dark key dimmed (DevDim): its face halfway to the red shell. */
#define DIM_DARK DevBlend(DEV_SHELL_RED, DEV_BEZEL, DEV_A_DIM)

/* ---- choice ----------------------------------------------------------------------------------------------------------- */

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

/* ---- learn ------------------------------------------------------------------------------------------------------------ */

static int LearnHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_LearnHit((const CtrPokeTouchView *)view, x, y);
}

static const struct { const char *name, *type, *pp, *power, *accuracy; int8_t t; } kLearnMove[PT_LEARN_ROWS] = {
    {"FLAMETHROWER", "FIRE", "15/15", "95", "100", 10}, {"WING ATTACK", "FLYING", "35/35", "60", "100", 2},
    {"DRAGON CLAW", "DRAGON", "15/15", "80", "100", 16}, {"SLASH", "NORMAL", "20/20", "70", "100", 0},
    {"FIRE SPIN", "FIRE", "15/15", "15", "70", 10},
};

/* The move to forget as 3ds_bottom.c gives it (FieldLearnFill): four moves and the new one in row 4 (FireRed's names
 * and types in capitals), the cursor on the new one, the Pokemon called by its species' name (recipe 0251's title
 * case: nameDefault), port-written stand-ins for the question and the description; BACK shown and usable. */
static CtrPokeTouchView LearnBoard(void)
{
    CtrPokeTouchView v;
    CtrPokeTouchLearn *l = &v.learn;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.selected = PT_POKEMON;
    l->on = 1;
    l->cursor = 4;
    l->pressed = -1;
    l->species = 6;
    l->nameDefault = 1;
    strcpy(l->nickname, "Charizard");
    strcpy(l->level, "Lv36");
    strcpy(l->question, "Port-written question for this test?");
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        l->row[i].move = (uint16_t)(i + 1);
        l->row[i].type = kLearnMove[i].t;
        strcpy(l->row[i].name, kLearnMove[i].name);
        strcpy(l->row[i].typeName, kLearnMove[i].type);
        strcpy(l->row[i].pp, kLearnMove[i].pp);
        strcpy(l->row[i].power, kLearnMove[i].power);
        strcpy(l->row[i].accuracy, kLearnMove[i].accuracy);
    }
    strcpy(l->desc, "Port-written words for this test. They stand in for what a move does here.");
    snprintf(v.bar[0], PT_BAR_LEN, "Move / item");             /* the bar: not drawn by the move to forget */
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* Upper case (owner decision 5): FireRed's move and type names, which v0.6.0 title-cased, in capitals; the name its
 * species' own in capitals, a nickname as typed; FireRed's question and description as they are. */
static void LearnUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        CtrTextCase(cased.learn.row[i].name);
        CtrTextCase(cased.learn.row[i].typeName);
    }
    TouchSamePixels(&cased, &v, 1, "the moves and types in v0.6.0's case and as FireRed writes them");
    cased = v;
    strcpy(cased.learn.nickname, "CHARIZARD");
    TouchSamePixels(&cased, &v, 1, "its species' name in title case and in capitals");
    v.learn.nameDefault = cased.learn.nameDefault = 0;
    strcpy(v.learn.nickname, "Blaze");
    strcpy(cased.learn.nickname, "BLAZE");
    TouchSamePixels(&cased, &v, 0, "a nickname, as typed");
    cased = v;
    DevUpper(cased.learn.question, v.learn.question, sizeof(cased.learn.question));
    TouchSamePixels(&cased, &v, 0, "the question, as FireRed writes it");
    cased = v;
    DevUpper(cased.learn.desc, v.learn.desc, sizeof(cased.learn.desc));
    TouchSamePixels(&cased, &v, 0, "the description, as FireRed writes it");
}

/* The plan's suite (every row hit where drawn, the new one inverted, BACK; an empty row neither drawn nor hit; what the
 * view does not draw, the HM lock among it, presents nothing), then the column, the rows, the rule, the note, a touch,
 * BACK's states, the ways in and out, upper case and the states for the eye. */
static void SuiteLearn(void)
{
    CtrPokeTouchView lv = LearnBoard(), lo = lv, other;
    CtrPokeTouchRect r;
    int i;

    picArt = 1;
    ForgetPictures();
    CtrPokeTouch_Draw(canvas, &lo);
    /* the plan's suite */
    lv = LearnBoard(); DevCheckTouch(&lo, &lv); assert(CtrPokeTouch_DeviceShown(&lv));
    /* learn */ for (i = 0; i < 5; ++i) AssertRowHits(LearnHitV, &lv, DevSumRow(i), PT_LEARN_HIT_ROW + i);
    assert(at(DevSumRow(4).x + 1, DevSumRow(4).y + 11) == DEV_BEZEL); AssertHits(BackV, &lv, kDevSumBack, 1);   /* before the name */
    lv.learn.row[2].move = lv.learn.row[3].move = 0; DevCheckTouch(&lo, &lv); assert(CtrPokeTouch_LearnHit(&lv, 130, DevSumRow(2).y + 5) == -1);
    lv.learn.row[0].locked ^= 1; lv.mapGroup ^= 1; lv.speed ^= 1; assert(DevCheckTouch(&lo, &lv).count == 0);

    /* the title: the question in the paper's ink; the column (the paper's second tone): the name, the level small in the
     * secondary ink, the picture in the Summary's place, the cursor's move's POWER and ACCURACY at its foot */
    lv = LearnBoard(); DevCheckTouch(&lo, &lv);
    assert(CountIn(kTitlePaper, DEV_INK) > 0);
    assert(at(15, 47) == DEV_PAPER2 && at(116, 172) == DEV_PAPER2);
    assert(Count(20, 49, 92, 12, DEV_INK) > 0 && Count(20, 63, 92, 10, DEV_INK2) > 0 && Count(20, 63, 92, 10, DEV_INK) == 0);
    assert(Count(kDevSumPic.x, kDevSumPic.y, kDevSumPic.w, kDevSumPic.h, STUB_PIC) == 48 * 48);
    assert(Count(20, 148, 92, 26, DEV_INK2) > 0 && Count(20, 148, 92, 26, DEV_INK) > 0);
    /* the rows: the cursor's inverted, the others the paper's; each its name, its type's chip and its PP; the rule over
     * the new one; the description on the note */
    for (i = 0; i < PT_LEARN_ROWS; ++i) {
        r = DevSumRow(i);
        assert(at(r.x + 1, r.y + 1) == (i == 4 ? DEV_BEZEL : DEV_PAPER));
        assert(Count(r.x + r.w - 90, r.y, 90, r.h, UiTypeColor(kLearnMove[i].t)) > 0);
        assert(Count(r.x, r.y, 80, r.h, i == 4 ? DEV_PAPER : DEV_INK) > 0 && Count(r.x + r.w - 40, r.y, 40, r.h, i == 4 ? DEV_PAPER : DEV_INK) > 0);
    }
    r = DevSumRow(4); assert(Count(r.x, r.y - 2, r.w, 1, DEV_INK_SH) == r.w);
    assert(CountIn(kSumNotePaper, DEV_INK) > 0);
    /* a touch inverts the row under it, not the cursor's; the cursor's move's numbers follow it */
    lv.learn.pressed = PT_LEARN_HIT_ROW + 1; DevCheckTouch(&lo, &lv);
    r = DevSumRow(1); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); r = DevSumRow(4); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
    lv.learn.pressed = -1; lv.learn.cursor = 1; DevCheckTouch(&lo, &lv);
    other = lv; strcpy(other.learn.row[1].power, "1000"); TouchSamePixels(&lv, &other, 0, "the cursor's move's power");
    other = lv; strcpy(other.learn.row[1].accuracy, "1000"); TouchSamePixels(&lv, &other, 0, "the cursor's move's accuracy");
    other = lv; strcpy(other.learn.row[0].power, "1000"); TouchSamePixels(&lv, &other, 1, "another move's power");
    CtrPokeTouch_Draw(canvas, &lo);
    /* two moves and the new one: the empty rows not drawn (the paper), not hit */
    lv = LearnBoard(); lv.learn.row[2].move = lv.learn.row[3].move = 0; DevCheckTouch(&lo, &lv);
    for (i = 2; i < 4; ++i) {
        r = DevSumRow(i);
        assert(Count(r.x, r.y, r.w, r.h, DEV_PAPER) == r.w * r.h && CtrPokeTouch_LearnHit(&lv, r.x + 10, r.y + 10) == -1);
    }
    /* BACK: dark in the Summary's place, sunk under a touch, grey while it cannot be used, gone while not shown */
    AssertKey(kDevSumBack, DEV_LOOK_DARK, 0); assert(Count(244, 188, 68, 45, DEV_LED) == 0);
    lv.backPressed = 1; DevCheckTouch(&lo, &lv); assert(Count(244, 188, 68, 45, DEV_LED) > 0);
    lv.backPressed = 0; lv.backEnabled = 0; DevCheckTouch(&lo, &lv); assert(at(247, 209) == DIM_DARK);
    lv.backEnabled = 1; lv.backVisible = 0; DevCheckTouch(&lo, &lv); assert(at(247, 209) == DEV_SHELL_RED && !BackV(&lv, 247, 209));
    lv.backVisible = 1; DevCheckTouch(&lo, &lv);

    /* what the move to forget does not draw presents nothing; the shell's colour whole; the ways in and out (HOME; the
     * Summary's own picture before it is ready: not the device's, nothing hit) */
    strcpy(lv.bar[0], "Anything"); lv.registeredItem = 7; lv.enabled[PT_BAG] = 1; lv.playerX = 3;
    assert(DevCheckTouch(&lo, &lv).count == 0);
    lv.shell = 2; assert(DevWhole(DevCheckTouch(&lo, &lv)) && at(1, 100) == DEV_SHELL_GREEN);
    lv.shell = 0; DevCheckTouch(&lo, &lv);
    lv.learn.nameDefault = 0; assert(DevWhole(DevCheckTouch(&lo, &lv)));
    other = lv; other.learn.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&lo, &other)));
    lv = LearnBoard(); assert(DevWhole(DevCheckTouch(&lo, &lv)));
    other = lv; memset(&other.learn, 0, sizeof(other.learn)); other.viewport = 1;
    assert(DevWhole(DevCheckTouch(&lo, &other)) && !CtrPokeTouch_DeviceShown(&other) && CtrPokeTouch_LearnHit(&other, 130, 60) == -1);
    lv.viewport = 1; assert(!CtrPokeTouch_DeviceShown(&lv) && CtrPokeTouch_LearnHit(&lv, 130, 60) == -1);
    lv = LearnBoard(); assert(DevWhole(DevCheckTouch(&lo, &lv)));
    LearnUpperCase(LearnBoard());
    CtrPokeTouch_Draw(canvas, &lo);

    /* The states for the eye, each through Update first. */
    lv = LearnBoard(); lv.learn.cursor = 1; DevCheckTouch(&lo, &lv); shot("move");
    lv.learn.cursor = 4; DevCheckTouch(&lo, &lv); shot("new");
    lv.learn.row[2].move = lv.learn.row[3].move = 0; lv.learn.row[2].name[0] = lv.learn.row[3].name[0] = 0;
    DevCheckTouch(&lo, &lv); shot("two");
    picArt = 0;
    ForgetPictures();
    assert(sDevKeyCut == 0);                      /* main prints PASS device learn */
}

/* ---- relearn ---------------------------------------------------------------------------------------------------------- */

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
    v.pressed = v.selected = PT_NONE;
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
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_DeviceShown(&rv));
    /* relearn */ for (i = 0; i < 5; ++i) AssertRowHits(RlHitV, &rv, DevTallRow(i, 0), PT_RL_HIT_ROW + i);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 73) == -1); AssertHits(RlHitV, &rv, kDevBagDown, PT_RL_HIT_DOWN); AssertHits(RlHitV, &rv, kDevBagClose, PT_RL_HIT_CANCEL);
    rv.relearn.first = 4; rv.relearn.cursor = 8; DevCheckTouch(&ro, &rv); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1); assert(CtrPokeTouch_RelearnHit(&rv, 130, DevTallRow(4, 0).y + 5) == -1);
    rv.relearn.panel = PT_RL_PANEL_YESNO; DevCheckTouch(&ro, &rv); AssertHits(RlHitV, &rv, DevBagMenuKey(2, 0), PT_RL_HIT_YES); AssertHits(RlHitV, &rv, DevBagMenuKey(2, 1), PT_RL_HIT_NO); assert(CtrPokeTouch_RelearnHit(&rv, 60, 60) == -1);
    rv.relearn.panel = PT_RL_PANEL_MESSAGE; rv.relearn.tap = 1; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == PT_RL_HIT_MESSAGE);
    rv.relearn.tap = 0; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == -1);
    rv.relearn.panel = PT_RL_PANEL_BLANK; DevCheckTouch(&ro, &rv); for (y = 0; y < 240; y += 4) for (x = 0; x < 320; x += 4) assert(CtrPokeTouch_RelearnHit(&rv, x, y) == -1);
    rv.mapGroup ^= 1; rv.speed ^= 1; assert(DevCheckTouch(&ro, &rv).count == 0);
    assert(PT_RL_HIT_CANCEL == 235);

    /* LIST: the question on the title; the cursor's row inverted, the others the paper's, each its name and its type's
     * chip; ▲ grey at the top with the cursor on the first move, ▼ dark; the cursor's move's type, power and accuracy
     * and description on the description's screen; CANCEL dark, up */
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv);
    assert(CountIn(kTitlePaper, DEV_INK) > 0);
    for (i = 0; i < PT_RL_ROWS; ++i) {
        r = DevTallRow(i, 0);
        assert(at(r.x + 1, r.y + 1) == (i == 0 ? DEV_BEZEL : DEV_PAPER));
        assert(Count(r.x, r.y, 80, r.h, i == 0 ? DEV_PAPER : DEV_INK) > 0 && Count(r.x + r.w - 60, r.y, 60, r.h, UiTypeColor(kRlMove[i].t)) > 0);
    }
    assert(Count(276, 40, 36, 66, DEV_BAG_ARROW_OFF) > 0 && Count(276, 114, 36, 66, DEV_KEY_INK) > 0);
    assert(CountIn(kDescPaper, UiTypeColor(10)) > 0 && CountIn(kDescPaper, DEV_INK2) > 0 && CountIn(kDescPaper, DEV_INK) > 0);
    AssertKey(kDevBagClose, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 43, DEV_LED) == 0 && Count(244, 190, 68, 43, DEV_DARK_INK) > 0);
    /* the cursor past the first move: ▲ on and hit */
    rv.relearn.cursor = 1; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv);
    AssertHits(RlHitV, &rv, kDevBagUp, PT_RL_HIT_UP); assert(Count(276, 40, 36, 66, DEV_KEY_INK) > 0);
    r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    /* a touch: the row under it inverted, not the cursor's; ▼ sunk (the cursor's row stays); CANCEL sunk with its LED */
    rv.relearn.pressed = PT_RL_HIT_ROW + 3; DevCheckTouch(&ro, &rv);
    r = DevTallRow(3, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
    rv.relearn.pressed = PT_RL_HIT_DOWN; DevCheckTouch(&ro, &rv); AssertKey(kDevBagDown, DEV_LOOK_KEY, 1); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    rv.relearn.pressed = PT_RL_HIT_CANCEL; DevCheckTouch(&ro, &rv); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1);
    assert(Count(244, 190, 68, 43, DEV_LED) > 0 && at(r.x + 1, r.y + 1) == DEV_PAPER);
    /* ▲ held where it cannot go: not sunk, the cursor's row stays */
    rv.relearn.cursor = 0; RelearnRows(&rv, 0); rv.relearn.pressed = PT_RL_HIT_UP; DevCheckTouch(&ro, &rv);
    AssertKey(kDevBagUp, DEV_LOOK_KEY, 0); r = DevTallRow(0, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    rv.relearn.pressed = -1; DevCheckTouch(&ro, &rv);
    /* the scroll: from the third move, the cursor on the fifth */
    rv.relearn.cursor = 4; RelearnRows(&rv, 2); DevCheckTouch(&ro, &rv);
    r = DevTallRow(2, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); AssertHits(RlHitV, &rv, kDevBagUp, PT_RL_HIT_UP);
    /* the end: FireRed's CANCEL row is no row (the paper's, not hit) but CANCEL, sunk with its LED; ▼ grey and not hit;
     * nothing to describe */
    rv.relearn.cursor = 8; RelearnRows(&rv, 4); DevCheckTouch(&ro, &rv);
    r = DevTallRow(4, 0); assert(Count(r.x, r.y, r.w, r.h, DEV_PAPER) == r.w * r.h && CtrPokeTouch_RelearnHit(&rv, r.x + 10, r.y + 10) == -1);
    for (i = 0; i < 4; ++i) { r = DevTallRow(i, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER); }
    AssertKey(kDevBagClose, DEV_LOOK_DARK, 1); assert(Count(244, 190, 68, 43, DEV_LED) > 0);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_BAG_ARROW_OFF) > 0);
    assert(CountIn(kDescPaper, DEV_INK) == 0 && CountIn(kDescPaper, DEV_INK2) == 0);

    /* YES / NO: the question on the narrow screen, YES and NO in the Bag's place, FireRed's cursor sunk (or the key under
     * a held touch); the cursor's move on the narrow description; ▲ ▼ and CANCEL neither drawn nor hit */
    rv = RelearnBoard(); rv.relearn.cursor = 1; RelearnRows(&rv, 0);
    rv.relearn.panel = PT_RL_PANEL_YESNO; strcpy(rv.relearn.message, "Port-written words of a question?"); DevCheckTouch(&ro, &rv);
    assert(CountIn(kNarrowPaper, DEV_INK) > 0 && at(196, 60) == DEV_SHELL_RED);
    AssertKey(DevBagMenuKey(2, 0), DEV_LOOK_KEY, 1); AssertKey(DevBagMenuKey(2, 1), DEV_LOOK_KEY, 0);
    assert(CountIn(kNarrowDescPaper, UiTypeColor(10)) > 0 && CountIn(kNarrowDescPaper, DEV_INK) > 0);
    assert(at(247, 210) == DEV_SHELL_RED && at(294, 150) == DEV_SHELL_RED && CtrPokeTouch_RelearnHit(&rv, 247, 210) == -1);
    assert(CtrPokeTouch_RelearnHit(&rv, 294, 150) == -1 && CtrPokeTouch_RelearnHit(&rv, 30, 60) == -1);
    rv.relearn.menuCursor = 1; DevCheckTouch(&ro, &rv); AssertKey(DevBagMenuKey(2, 0), DEV_LOOK_KEY, 0); AssertKey(DevBagMenuKey(2, 1), DEV_LOOK_KEY, 1);
    rv.relearn.pressed = PT_RL_HIT_YES; DevCheckTouch(&ro, &rv); AssertKey(DevBagMenuKey(2, 0), DEV_LOOK_KEY, 1); AssertKey(DevBagMenuKey(2, 1), DEV_LOOK_KEY, 0);
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
    assert(CountIn(kTitlePaper, DEV_INK) == 0 && CountIn(kListPaper, DEV_INK) == 0 && CountIn(kDescPaper, DEV_INK) == 0);
    assert(at(16, 50) == DEV_PAPER && at(247, 210) == DIM_DARK && at(279, 73) == DevBlend(DEV_SHELL_RED, DEV_KEY, DEV_A_DIM));
    /* no BACK in any panel: CANCEL and NO are its B */
    for (int panel = PT_RL_PANEL_LIST; panel <= PT_RL_PANEL_BLANK; ++panel) {
        rv = RelearnBoard(); rv.relearn.panel = (uint8_t)panel; rv.relearn.tap = 1;
        for (y = 0; y < 240; y += 4) for (x = 0; x < 320; x += 4) assert(!CtrPokeTouch_BackHit(&rv, x, y));
    }

    /* what the relearner does not draw presents nothing: BACK's fields, the Pokemon (no column), the YES / NO cursor and
     * the wait for A outside their panels, the bar; the shell's colour whole; the ways in and out (HOME; FireRed's
     * picture: not the device's, nothing hit) */
    rv = RelearnBoard(); DevCheckTouch(&ro, &rv);
    rv.backPressed = 1; rv.backEnabled = 0; rv.relearn.species = 7; strcpy(rv.relearn.nickname, "Blaze"); strcpy(rv.relearn.level, "37");
    rv.relearn.menuCursor = 1; rv.relearn.tap = 1; strcpy(rv.bar[0], "Anything"); rv.registeredItem = 7; rv.playerX = 3;
    assert(DevCheckTouch(&ro, &rv).count == 0);
    rv.relearn.pressed = PT_RL_HIT_UP; assert(DevCheckTouch(&ro, &rv).count == 0);         /* ▲ cannot go: not drawn sunk */
    rv.relearn.pressed = -1; DevCheckTouch(&ro, &rv);
    rv.shell = 2; assert(DevWhole(DevCheckTouch(&ro, &rv)) && at(1, 100) == DEV_SHELL_GREEN);
    rv.shell = 0; DevCheckTouch(&ro, &rv);
    other = rv; other.relearn.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&ro, &other)));
    rv = RelearnBoard(); assert(DevWhole(DevCheckTouch(&ro, &rv)));
    other = rv; memset(&other.relearn, 0, sizeof(other.relearn)); other.viewport = 1;
    assert(DevWhole(DevCheckTouch(&ro, &other)) && !CtrPokeTouch_DeviceShown(&other) && CtrPokeTouch_RelearnHit(&other, 130, 60) == -1);
    rv.viewport = 1; assert(!CtrPokeTouch_DeviceShown(&rv) && CtrPokeTouch_RelearnHit(&rv, 130, 60) == -1);
    rv = RelearnBoard(); assert(DevWhole(DevCheckTouch(&ro, &rv)));
    RelearnUpperCase(RelearnBoard());
    CtrPokeTouch_Draw(canvas, &ro);

    /* The states for the eye, each through Update first. */
    rv = RelearnBoard(); rv.relearn.cursor = 1; RelearnRows(&rv, 0); DevCheckTouch(&ro, &rv); shot("list");
    rv.relearn.cursor = 4; RelearnRows(&rv, 2); DevCheckTouch(&ro, &rv); shot("scroll");
    rv.relearn.cursor = 8; RelearnRows(&rv, 4); DevCheckTouch(&ro, &rv); shot("cancel");
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
