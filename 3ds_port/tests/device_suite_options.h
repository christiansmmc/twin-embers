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
 * (Settings, `second` (the Graphics section's name: Graphics, or Looks while its looks page is shown), QoL, Nuzlocke);
 * nothing held, BACK shown and usable; the description's lines. Words in v0.6.0's case. */
static CtrPokeTouchView OptPage(int page, int section, const char *second, int rows, int selected, const char *line1,
                                const char *line2)
{
    const char *const names[4] = {"Settings", second, "QoL", "Nuzlocke"};
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
    CtrPokeTouchView v = OptPage(0, 0, "Graphics", 5, 1, "Shift offers a switch", "after a foe faints.");

    OptRowValue(&v, 0, "WWWWWWWWWWWWWWW", "WWWWWWWWWWW");
    OptRowValue(&v, 1, "Battle style", "Shift");
    OptRowSwitch(&v, 2, "FPS counter", 1);
    OptRowSwitch(&v, 3, "Battle scene", 0);
    OptRowLevels(&v, 4, "EXP share", kLevel, 1);
    strcpy(v.bar[0], "Battle style");
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
    static const char *const kCompanion[3] = {"Off", "Follow", "Be it"}, *const kShare[3] = {"Off", "Half", "Full"};
    CtrPokeTouchView v = OptBoard(), old = v, other;
    int s, i, k, y;

    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v = OptBoard(); DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevOptShown));
    for (s = 0; s < 4; ++s) AssertHits(OptHitV, &v, DevAcross(4, s, 8, 24), 0x100 + PT_OPT_KEY_SECTION + s);
    AssertKey(DevAcross(4, 0, 8, 24), DEV_LOOK_KEY, 1); AssertHits(OptHitV, &v, kDevBagUp, 0x100 + PT_OPT_KEY_UP); AssertHits(OptHitV, &v, kDevBagDown, 0x100 + PT_OPT_KEY_DOWN);
    for (i = 0; i < 4; ++i) { y = DevOptRow(i).y + 9;
        assert(OptHitV(&v, 16, y) == i * 16 + PT_OPT_ZONE_LABEL && OptHitV(&v, 135, y) == i * 16 + PT_OPT_ZONE_LABEL && OptHitV(&v, 136, y) == i * 16 + PT_OPT_ZONE_PREV
               && OptHitV(&v, 160, y) == i * 16 + PT_OPT_ZONE_NEXT && OptHitV(&v, 259, y) == i * 16 + PT_OPT_ZONE_NEXT && OptHitV(&v, 15, y) == -1 && OptHitV(&v, 260, y) == -1); }
    for (k = 0; k < 3; ++k) assert(OptHitV(&v, 137 + 41 * k, DevOptRow(4).y + 9) == 4 * 16 + PT_OPT_ZONE_SEG + k && CtrPokeTouch_OptionSegment(137 + 41 * k, 3) == k);
    AssertHits(OptHitV, &v, kDevSmallKey[0], 16 + PT_OPT_ZONE_PREV); AssertHits(OptHitV, &v, kDevSmallKey[1], 16 + PT_OPT_ZONE_NEXT); AssertHits(BackV, &v, kDevBagClose, 1);
    assert(OptHitV(&v, 20, DevOptRow(5).y + 9) == -1 && at(17, DevOptRow(1).y + 9) == DEV_BEZEL);                 /* x 17: before the label */
    v.optPressed = 3; v.optZone = PT_OPT_ZONE_LABEL; DevCheckTouch(&old, &v); assert(at(17, DevOptRow(3).y + 9) == DEV_BEZEL && at(17, DevOptRow(1).y + 9) != DEV_BEZEL);
    v.optPressed = -1; v.optKey = PT_OPT_KEY_DOWN; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevBagDown, DEV_LOOK_KEY, 1);
    AssertKey(DevAcross(4, 0, 8, 24), DEV_LOOK_KEY, 1);                                                          /* ▼ held: the section stays sunk */
    v.optKey = -1; DevCheckTouch(&old, &v); v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
    v.optPageCount = 1; DevCheckTouch(&old, &v); assert(CtrPokeTouch_OptionKey(&v, 294, 73) == -1 && at(294, 73) == DEV_SHELL_RED);
    v.backVisible = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_BackHit(&v, 278, 210));
    v = OptBoard(); v.optCount = 4; v.optLook.on = 1; DevCheckTouch(&old, &v); assert(at(200, 173) == DEV_PAPER2);

    /* the board's pieces: the sections (the shown one sunk with its LED), the rows on the list's paper (the described
     * one inverted, none past the last), ▲▼, the description in the small note, ◀ ▶ up, BACK dark */
    v = OptBoard(); DevCheckTouch(&old, &v);
    for (s = 1; s < 4; ++s) AssertKey(DevAcross(4, s, 8, 24), DEV_LOOK_KEY, 0);
    assert(Count(8, 8, 73, 24, DEV_LED) > 0 && Count(85, 8, 227, 24, DEV_LED) == 0);
    assert(at(17, DevOptRow(0).y + 9) == DEV_PAPER && at(20, DevOptRow(5).y + 9) == DEV_PAPER);
    AssertKey(kDevBagUp, DEV_LOOK_KEY, 0); AssertKey(kDevBagDown, DEV_LOOK_KEY, 0);
    AssertKey(kDevSmallKey[0], DEV_LOOK_KEY, 0); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 0); AssertKey(kDevBagClose, DEV_LOOK_DARK, 0);
    assert(Count(14, 196, 140, 30, DEV_INK) > 0 && Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    /* a row's text: its label from 6 px in, ◀ in PREV, the value and ▶ in NEXT (the inverted row's in the paper's ink) */
    y = DevOptRow(1).y;
    assert(Count(22, y, 114, 18, DEV_PAPER) > 0 && Count(16, y, 6, 18, DEV_PAPER) == 0 && Count(136, y, 24, 18, DEV_PAPER) > 0);
    assert(Count(160, y, 84, 18, DEV_PAPER) > 0 && Count(248, y, 9, 18, DEV_PAPER) > 0);
    /* the levels: each column of PREV and NEXT is the level drawn there, for rows of one to three; the chosen one boxed
     * (HALF), a held one boxed instead */
    for (int n = 1; n <= PT_OPT_SEGS; ++n) {
        assert(DevOptLevel(4, 0, n).x == DEV_OPT_PREV_X && DevOptLevel(4, n - 1, n).x + DevOptLevel(4, n - 1, n).w == 260);
        for (k = 0; k < n; ++k)
            for (int x = DevOptLevel(4, k, n).x; x < DevOptLevel(4, k, n).x + DevOptLevel(4, k, n).w; ++x)
                assert(CtrPokeTouch_OptionSegment(x, n) == k && (k == 0 || x > DevOptLevel(4, k - 1, n).x));
    }
    y = DevOptRow(4).y;
    assert(at(197, y + 1) == DEV_INK && at(156, y + 1) == DEV_PAPER && at(238, y + 1) == DEV_PAPER);
    v.optPressed = 4; v.optZone = PT_OPT_ZONE_SEG + 2; DevCheckTouch(&old, &v);
    assert(at(197, y + 1) != DEV_PAPER && at(238, y + 1) == DEV_PAPER && at(156, y + 1) == DEV_BEZEL);
    /* a touch on ◀ or ▶ (or the described row's own zone) sinks that key; a held section key is the one sunk */
    v.optPressed = 1; v.optZone = PT_OPT_ZONE_PREV; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[0], DEV_LOOK_KEY, 1); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 0);
    v.optZone = PT_OPT_ZONE_NEXT; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 1); AssertKey(kDevSmallKey[0], DEV_LOOK_KEY, 0);
    v.optPressed = 3; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 0);                     /* another row's: not */
    v.optPressed = -1; v.optKey = PT_OPT_KEY_SECTION + 2; DevCheckTouch(&old, &v);
    AssertKey(DevAcross(4, 2, 8, 24), DEV_LOOK_KEY, 1); AssertKey(DevAcross(4, 0, 8, 24), DEV_LOOK_KEY, 0);
    v.optKey = PT_OPT_KEY_UP; DevCheckTouch(&old, &v); AssertKey(kDevBagUp, DEV_LOOK_KEY, 1);
    v.optKey = -1;
    /* BACK: sunk under a touch, grey while it cannot be used, gone when not shown (and the shell there) */
    v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1);
    v.backPressed = 0; v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    v.backEnabled = 1; v.backVisible = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DEV_SHELL_RED && !BackV(&v, 247, 210));
    v.backVisible = 1; DevCheckTouch(&old, &v);
    /* what OPTIONS does not draw presents nothing: the first line of the bar (the row's label), a row past the page's,
     * FRAME's type with no sample, a look not shown, HOME's keys, the map */
    strcpy(v.bar[0], "Anything"); strcpy(v.optLabel[6], "Seventh"); v.optFrameType = 3; strcpy(v.optLook.name, "ASH");
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
    v = OptPage(0, 0, "Graphics", 5, 0, "How fast dialogue", "text is printed.");
    OptRowValue(&v, 0, "Text speed", "Mid"); OptRowSwitch(&v, 1, "Battle scene", 1); OptRowValue(&v, 2, "Battle style", "Shift");
    OptRowValue(&v, 3, "Sound", "Mono"); OptRowValue(&v, 4, "Button mode", "Help");
    DevCheckTouch(&old, &v); shot("settings");
    /* FRAME described: its sample (the pack's frame; none in the synthetic run) under the rows */
    v = OptPage(1, 1, "Graphics", 5, 0, "Menus, Yes/No", "box frame.");
    OptRowValue(&v, 0, "Frame", "Type1"); OptRowSwitch(&v, 1, "FPS counter", 0); OptRowValue(&v, 2, "Day/Night", "Clock");
    OptRowValue(&v, 3, "Hour", "12:00"); OptRowLevels(&v, 4, "Companion", kCompanion, 1);
    v.optFrameShow = 1; v.optFrameType = 0;
    DevCheckTouch(&old, &v); AssertKey(DevAcross(4, 1, 8, 24), DEV_LOOK_KEY, 1); shot("graphics_frame");
    /* the looks page (the section says LOOKS): RED LOOK described, the look's panel with its walking picture */
    v = OptPage(2, 1, "Looks", 4, 1, "Red's look, from the", "alternative looks.");
    OptRowValue(&v, 0, "Shell colour", "Red"); OptRowValue(&v, 1, "Red look", "ASH"); OptRowValue(&v, 2, "Leaf look", "Default");
    OptRowValue(&v, 3, "Rival look", "Default");
    v.optLook.on = v.optLook.has = 1; strcpy(v.optLook.name, "ASH"); strcpy(v.optLook.credit, "By Kalarie");
    for (i = 0; i < 512; ++i) v.optLook.tiles[i] = (uint8_t)(i * 7);
    for (i = 1; i < 16; ++i) v.optLook.palette[i] = (uint16_t)(i * 0x0421);
    DevCheckTouch(&old, &v);
    assert(Count(kDevOptLook.x, kDevOptLook.y, kDevOptLook.w, kDevOptLook.h, DEV_PAPER2) > 0 && Count(64, 122, 190, 54, DEV_INK) > 0);
    shot("looks");
    /* QoL: EXP SHARE's levels described */
    v = OptPage(4, 2, "Graphics", 6, 3, "Share EXP with your", "team: Half or Full.");
    OptRowSwitch(&v, 0, "Type matchup", 1); OptRowSwitch(&v, 1, "Move details", 1); OptRowSwitch(&v, 2, "Repel again", 0);
    OptRowLevels(&v, 3, "EXP share", kShare, 2); OptRowSwitch(&v, 4, "Reusable TMs", 0); OptRowSwitch(&v, 5, "Free HMs", 0);
    DevCheckTouch(&old, &v); shot("qol_levels");
    v = OptPage(5, 3, "Graphics", 5, 1, "Wild and trainer Pokemon", "are random species.");
    OptRowSwitch(&v, 0, "Nuzlocke", 1); OptRowSwitch(&v, 1, "Randomlocke", 0); OptRowSwitch(&v, 2, "Duplicates", 1);
    OptRowSwitch(&v, 3, "Shiny clause", 0); OptRowSwitch(&v, 4, "Level caps", 1);
    DevCheckTouch(&old, &v); shot("nuzlocke");
    /* QoL's key held from the Nuzlocke page */
    v.optKey = PT_OPT_KEY_SECTION + 2; DevCheckTouch(&old, &v); AssertKey(DevAcross(4, 2, 8, 24), DEV_LOOK_KEY, 1); shot("pressed_key");
    assert(sDevKeyCut == 0);                      /* main prints PASS device options */
}
