/* ---- frame, idle: FireRed's picture in an embedded screen, and the idle screen (no board; plan 3 Task 8) ------------
 * Included by device_render_test.c before its kSuites: two suites, frame and idle. Port-written words stand in for the
 * bar's. */

/* The papers of the note (the Summary's beside BACK, and across without it), where the title's capitals start and
 * where the hints' two lines (the small font, DEV_BAG_SMALL_LINE apart) start; the room of a line beside BACK. */
static const CtrPokeTouchRect kFrameNotePaper = {14, 194, 216, 32}, kFrameWidePaper = {14, 194, 292, 32};
#define FRAME_TITLE_Y 196
#define FRAME_HINT_Y 208
#define FRAME_HINT_X (14 + 6)
#define FRAME_HINT_ROOM (216 - 12)
#define FRAME_SCREW DevBlend(DEV_SHELL_RED, 0x0000, DEV_A_SCREW)

/* FireRed's own screen in the viewport as 3ds_bottom.c gives it (SnapshotState's MODE_GBAVIEW: the screen's title and
 * two lines on how to use it): the picture shown, BACK shown and usable, nothing held. */
static CtrPokeTouchView FrameBoard(void)
{
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.viewport = 1;
    strcpy(v.bar[0], "Fame checker");
    strcpy(v.bar[1], "Use the game's own controls.");
    strcpy(v.bar[2], "Back is the B button.");
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* The dispatcher's row that draws `v` is the frame, its last. */
static int FrameRow(const CtrPokeTouchView *v)
{
    return DevFieldIndex(v) == (int)(sizeof(kDevFieldViews) / sizeof(kDevFieldViews[0])) - 1;
}

/* BACK (a dark key) up or sunk: its face (the same colour either way) and the LED a sunk key lights. */
static void FrameBack(int sunk)
{
    const CtrPokeTouchRect r = kDevSumBack;

    AssertKey(r, DEV_LOOK_DARK, sunk);
    assert((Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) > 0) == sunk);
}

/* The small font's ink of hint line `line` (0, 1) from x to the room's end. */
static int FrameHintInk(int line, int x)
{
    return Count(x, FRAME_HINT_Y + DEV_BAG_SMALL_LINE * line, FRAME_HINT_X + FRAME_HINT_ROOM - x, 7, DEV_INK2);
}

/* Each hint on a line of its own while each fits the room: hint i's ink on line i, from its start to its width and none
 * after it. */
static void FrameHintsALine(const CtrPokeTouchView *v)
{
    for (int i = 0; i < 2; ++i) {
        char hint[PT_BAR_LEN];
        int w;

        DevUpper(hint, v->bar[1 + i], sizeof(hint));
        w = DevSmallTextWidth(hint);
        assert(w <= FRAME_HINT_ROOM && FrameHintInk(i, FRAME_HINT_X) > 0 && FrameHintInk(i, FRAME_HINT_X + w) == 0);
    }
}

/* Upper case (owner decision 5): the bar, which v0.6.0 title-cased, drawn in capitals. */
static void FrameUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    strcpy(v.bar[0], "FAME CHECKER");
    strcpy(v.bar[1], "USE THE GAME'S OWN CONTROLS.");
    strcpy(v.bar[2], "BACK IS THE B BUTTON.");
    strcpy(cased.bar[0], "Fame Checker");
    TouchSamePixels(&cased, &v, 1, "the bar in v0.6.0's case and in capitals");
}

/* The plan's suite (the picture's place in its screen, the viewport's GBA points, BACK where drawn and its states, the
 * wide note, an empty screen, what is not drawn), then the shell and the screen, the note, BACK's states, the redraws,
 * the dispatcher's rows, the ways in and out, upper case and the states for the eye. */
static void SuiteFrame(void)
{
    static const uint16_t kScrew[4][2] = {{4, 4}, {315, 4}, {4, 234}, {315, 234}};
    CtrPokeTouchView v = FrameBoard(), old = v, other;
    uint8_t keepSmall[256];
    int x, y;

    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v.viewport = 1; v.backVisible = v.backEnabled = 1; DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevFrameShown));
    assert(at(PT_VIEW_X, PT_VIEW_Y) == 0 && at(PT_VIEW_X + PT_VIEW_W - 1, PT_VIEW_Y + PT_VIEW_H - 1) == 0 && at(PT_VIEW_X - 1, PT_VIEW_Y + 20) == DEV_BEZEL);
    for (y = -1; y <= H; ++y) for (x = -1; x <= W; ++x) {                                       /* every point, and one past each edge */
        int in = x >= PT_VIEW_X && x < PT_VIEW_X + PT_VIEW_W && y >= PT_VIEW_Y && y < PT_VIEW_Y + PT_VIEW_H, gx, gy;
        assert(CtrPokeTouch_ViewportToGba(x, y, &gx, &gy) == in && (!in || (gx == x - PT_VIEW_X && gy == y - PT_VIEW_Y))); }
    AssertHits(BackV, &v, kDevSumBack, 1); v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevSumBack, DEV_LOOK_DARK, 1);
    v.backEnabled = 0; v.backPressed = 0; DevCheckTouch(&old, &v); assert(at(247, 209) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    v.backVisible = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_BackHit(&v, 278, 209) && at(278, 209) != DEV_SHELL_RED);
    v.viewport = 0; DevCheckTouch(&old, &v); assert(at(160, 90) == DEV_PAPER);                   /* nothing to show: an empty screen, never the embers */
    v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0); assert(sDevKeyCut == 0);

    /* the shell with its four screws; the screen centred on it, its paper FireRed's picture's place: black under the
     * GPU's picture while there is one, the paper without one */
    v = FrameBoard(); DevCheckTouch(&old, &v);
    for (int i = 0; i < 4; ++i)
        assert(at(kScrew[i][0], kScrew[i][1]) == FRAME_SCREW);
    assert(at(160, 0) == DevBlend(DEV_SHELL_RED, DEV_WHITE, DEV_A_LIGHT) && at(160, 239) == DevBlend(DEV_SHELL_RED, 0x0000, DEV_A_DARK));
    assert(at(33, 90) == DEV_SHELL_RED && at(34, 90) == DEV_BEZEL_LINE && at(285, 90) == DEV_BEZEL_LINE && at(286, 90) == DEV_SHELL_RED);
    assert(at(160, 8) == DEV_BEZEL_LINE && at(160, 179) == DEV_BEZEL_LINE && at(160, 180) == DevBlend(DEV_SHELL_RED, DEV_WHITE, DEV_A_LIGHT));
    assert(Count(PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H, 0x0000) == PT_VIEW_W * PT_VIEW_H);
    assert(at(PT_VIEW_X - 1, PT_VIEW_Y - 1) == DEV_BEZEL && at(PT_VIEW_X + PT_VIEW_W, PT_VIEW_Y + PT_VIEW_H) == DEV_BEZEL);
    v.viewport = 0; assert(DevWhole(DevCheckTouch(&old, &v)));
    assert(Count(PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H, DEV_PAPER) == PT_VIEW_W * PT_VIEW_H);
    /* the note beside BACK: the title in the paper's ink, the two hints in the small font and the secondary ink under
     * it, one a line (each fits); nothing between the note and BACK */
    v = FrameBoard(); DevCheckTouch(&old, &v); FrameHintsALine(&v);
    assert(Count(kFrameNotePaper.x, FRAME_TITLE_Y, kFrameNotePaper.w, UI_TEXT_H, DEV_INK) > 0);
    assert(Count(kFrameNotePaper.x, FRAME_HINT_Y, kFrameNotePaper.w, 7, DEV_INK2) > 0
           && Count(kFrameNotePaper.x, FRAME_HINT_Y + 9, kFrameNotePaper.w, 7, DEV_INK2) > 0);
    /* each line where its capitals start: their first row inked, the row over it (and the two between the hints) not */
    assert(Count(kFrameNotePaper.x, FRAME_TITLE_Y, kFrameNotePaper.w, 1, DEV_INK) > 0
           && Count(kFrameNotePaper.x, FRAME_TITLE_Y - 1, kFrameNotePaper.w, 1, DEV_INK) == 0);
    assert(Count(kFrameNotePaper.x, FRAME_HINT_Y, kFrameNotePaper.w, 1, DEV_INK2) > 0
           && Count(kFrameNotePaper.x, FRAME_HINT_Y + 7, kFrameNotePaper.w, 2, DEV_INK2) == 0
           && Count(kFrameNotePaper.x, FRAME_HINT_Y + 9, kFrameNotePaper.w, 1, DEV_INK2) > 0);
    assert(Count(kFrameNotePaper.x, kFrameNotePaper.y, kFrameNotePaper.w, FRAME_HINT_Y - 2 - kFrameNotePaper.y, DEV_INK2) == 0
           && Count(kFrameNotePaper.x, FRAME_HINT_Y, kFrameNotePaper.w, 16, DEV_INK) == 0);     /* (an apostrophe's 2 rows over) */
    assert(at(237, 209) == DEV_SHELL_RED && at(kFrameNotePaper.x + kFrameNotePaper.w - 1, 200) == DEV_PAPER);
    /* no hints: the title alone, centred down the paper */
    v.bar[1][0] = v.bar[2][0] = 0; assert(DevWhole(DevCheckTouch(&old, &v)));
    assert(Count(kFrameNotePaper.x, kFrameNotePaper.y, kFrameNotePaper.w, kFrameNotePaper.h, DEV_INK2) == 0);
    assert(Count(kFrameNotePaper.x, kFrameNotePaper.y, kFrameNotePaper.w, 11, DEV_INK) == 0
           && Count(kFrameNotePaper.x, kFrameNotePaper.y + 11, kFrameNotePaper.w, UI_TEXT_H, DEV_INK) > 0);
    /* one hint short enough: one line */
    strcpy(v.bar[2], "Back is the B button."); DevCheckTouch(&old, &v);
    assert(Count(kFrameNotePaper.x, FRAME_HINT_Y, kFrameNotePaper.w, 7, DEV_INK2) > 0
           && Count(kFrameNotePaper.x, FRAME_HINT_Y + 9, kFrameNotePaper.w, 7, DEV_INK2) == 0);
    /* with capitals 8 px wide in the small font, a hint wider than the room: the two as one text, wrapped into the two
     * lines (the second holds the first hint's end, not "BACK." alone); a text the two lines cannot hold: what is left
     * dropped and counted as a cut */
    memcpy(keepSmall, smallWidths, sizeof(keepSmall));
    for (unsigned c = 0xbb; c <= 0xd4; ++c)        /* A..Z */
        smallWidths[c] = 8;
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    strcpy(v.bar[1], "Port words that run on too long"); strcpy(v.bar[2], "Back.");
    DevCheckTouch(&old, &v); assert(sDevKeyCut == 0);
    assert(FrameHintInk(0, FRAME_HINT_X + 100) > 0 && FrameHintInk(1, FRAME_HINT_X + 60) > 0);
    strcpy(v.bar[1], "Port words for this test, and on and on"); strcpy(v.bar[2], "More port words that two lines drop");
    DevCheckTouch(&old, &v); sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &v);         /* one draw: one cut */
    assert(sDevKeyCut == 1 && FrameHintInk(1, FRAME_HINT_X + FRAME_HINT_ROOM - 40) > 0);
    sDevKeyCut = 0;
    memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    CtrPokeTouch_Draw(canvas, &old);
    /* without BACK the note goes across, its paper where BACK was */
    v = FrameBoard(); v.backVisible = 0; DevCheckTouch(&old, &v);
    assert(at(300, 199) == DEV_PAPER && at(kFrameWidePaper.x + kFrameWidePaper.w - 1, 200) == DEV_PAPER && at(312, 209) == DEV_SHELL_RED);
    assert(Count(kDevSumBack.x, kFrameWidePaper.y, kFrameWidePaper.x + kFrameWidePaper.w - kDevSumBack.x, kFrameWidePaper.h, DEV_BEZEL) == 0);
    /* BACK: a dark key with BACK's word, sunk with its LED under a touch, grey while it cannot be used (a touch on it then
     * sinks nothing), hit where it is drawn and nowhere else; gone with the note across when it is not shown */
    v = FrameBoard(); DevCheckTouch(&old, &v); FrameBack(0);
    assert(Count(kDevSumBack.x, kDevSumBack.y, kDevSumBack.w, kDevSumBack.h, DEV_DARK_INK) > 0);
    AssertHits(BackV, &v, kDevSumBack, 1);
    assert(!BackV(&v, 180, 210) && !BackV(&v, 160, 90) && !BackV(&v, 100, 210));     /* not v0.6.0's BACK place, the picture, the note */
    v.backPressed = 1; DevCheckTouch(&old, &v); FrameBack(1);
    v.backEnabled = 0; DevCheckTouch(&old, &v);
    assert(at(247, 209) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, DEV_A_DIM) && Count(kDevSumBack.x, kDevSumBack.y, kDevSumBack.w, kDevSumBack.h + DEV_DROP, DEV_LED) == 0);
    v.backPressed = 0; assert(DevCheckTouch(&old, &v).count == 0);                     /* not usable: never drawn held */
    v.backVisible = 0; DevCheckTouch(&old, &v);
    for (y = 0; y < H; y += 3)
        for (x = 0; x < W; x += 3)
            assert(!BackV(&v, x, y));
    /* redraws: what the frame draws (the picture shown or not, the bar, BACK, the shell) draws it whole; anything else
     * presents nothing */
    v = FrameBoard(); DevCheckTouch(&old, &v);
    assert(DevCheckTouch(&old, &v).count == 0);
    v.enabled[PT_BAG] = 1; v.running = 1; v.pressed = PT_BAG;
    strcpy(v.playerName, "RED"); v.playerX = 3; v.optCount = 3;
    assert(DevCheckTouch(&old, &v).count == 0);
    strcpy(v.bar[0], "Teachy TV"); assert(DevWhole(DevCheckTouch(&old, &v)));
    strcpy(v.bar[2], "Anything"); assert(DevWhole(DevCheckTouch(&old, &v)));
    v.backPressed = 1; assert(DevWhole(DevCheckTouch(&old, &v)));
    v.viewport = 0; assert(DevWhole(DevCheckTouch(&old, &v)));
    v.shell = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_BLUE && at(4, 4) == DevBlend(DEV_SHELL_BLUE, 0x0000, DEV_A_SCREW));
    v.shell = 0; DevCheckTouch(&old, &v);
    /* the dispatcher's rows: the frame is its last, for any field view no other row draws (FireRed's picture, or nothing
     * at all), never for a view of its own (Easy Chat even with a picture), HOME or the Bag */
    v = FrameBoard(); assert(FrameRow(&v));
    memset(&other, 0, sizeof(other)); assert(FrameRow(&other));
    for (int i = 0; i < 11; ++i) {
        other = FrameBoard();
        switch (i) {
        case 0: other.ec.on = 1; break;
        case 1: other.dex.on = 1; break;
        case 2: other.card.on = 1; break;
        case 3: other.learn.on = 1; break;
        case 4: other.relearn.on = 1; break;
        case 5: other.mail.on = 1; break;
        case 6: other.choice.on = 1; break;
        case 7: other.name.on = 1; break;
        case 8: other.box.on = 1; break;
        case 9: other.save = 1; break;
        default: other.options = 1; break;
        }
        assert(FrameRow(&other) == (i != 0));         /* a view's own row only without the picture (Easy Chat always) */
        other.viewport = 0; assert(!FrameRow(&other) && DevFieldIndex(&other) >= 0);
    }
    other = FrameBoard(); other.viewport = 0; other.home = 1; assert(DevFieldIndex(&other) == -1);
    other.home = 0; other.bag.on = 1; assert(DevFieldIndex(&other) == -1);
    other.viewport = 1; assert(FrameRow(&other));     /* the Bag's picture (GIVE, a case) is FireRed's: the frame */
    /* HOME and the Bag draw no BACK: none of their pixels is BACK, not even with BACK shown and usable (3ds_bottom.c
     * asks it of HOME in the frames after OPTIONS opens, HOME still the picture shown, where OPTIONS' key is) */
    other = HomeBoard(); other.backVisible = other.backEnabled = 1; assert(DevHomeShown(&other));
    for (y = 0; y < H; ++y) for (x = 0; x < W; ++x) assert(!CtrPokeTouch_BackHit(&other, x, y));
    other = BagBoard(); other.backVisible = other.backEnabled = 1; assert(DevBagShown(&other));
    for (y = 0; y < H; ++y) for (x = 0; x < W; ++x) assert(!CtrPokeTouch_BackHit(&other, x, y));
    /* the ways in and out: HOME, the frame, the Bag, the frame */
    other = HomeBoard(); assert(DevWhole(DevCheckTouch(&old, &other)));
    v = FrameBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    other = v; other.viewport = 0; other.bag.on = 1; assert(DevWhole(DevCheckTouch(&old, &other)));
    v = FrameBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    FrameUpperCase(FrameBoard());
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first. */
    v = FrameBoard(); DevCheckTouch(&old, &v); shot("back");
    v.backPressed = 1; DevCheckTouch(&old, &v); shot("back_pressed");
    v.backPressed = 0; v.backEnabled = 0; DevCheckTouch(&old, &v); shot("back_off");
    v = FrameBoard(); v.backVisible = 0; DevCheckTouch(&old, &v); shot("wide");
    memset(&v, 0, sizeof(v)); v.pressed = PT_NONE; DevCheckTouch(&old, &v); shot("empty");
    assert(sDevKeyCut == 0);                      /* main prints PASS device frame */
}

/* The logo's pixel (x, y) as the idle screen draws it over its dark paper: a/15 of the logo's colour over DEV_BEZEL,
 * channel by channel (none at alpha 0). */
static uint16_t IdleLogoPixel(int x, int y)
{
    int k = kIdleLogo[y * IDLE_LOGO_W + x], a = kIdleLogoAlpha[k];
    unsigned c = kIdleLogoColour[k], b = DEV_BEZEL;

    if (a == 0)
        return DEV_BEZEL;
    if (a >= 15)
        return (uint16_t)c;
    return (uint16_t)(((((c >> 11) * a + (b >> 11) * (15 - a)) / 15) << 11)
                      | (((((c >> 5) & 63) * a + ((b >> 5) & 63) * (15 - a)) / 15) << 5)
                      | (((c & 31) * a + (b & 31) * (15 - a)) / 15));
}

/* The plan's suite (each shell without the logo, the logo on the dark screen), then the shell and the screen in each
 * colour, the logo exactly where and as it is drawn, HOME's touch places gone, and the states for the eye. */
static void SuiteIdle(void)
{
    static uint16_t bare[W * H];
    CtrPokeTouchView home = HomeBoard();
    int s;

    /* the plan's suite */
    /* idle */ for (s = 0; s < 4; ++s) { CtrPokeTouch_DrawIdle(canvas, 0, s); assert(at(160, 20) == kShell[s] && at(160, 120) == DEV_BEZEL && at(4, 4) == DevBlend(kShell[s], 0, 90)); }
    CtrPokeTouch_DrawIdle(canvas, 1, 0); assert(Count(48, 57, 224, 126, DEV_BEZEL) < 224 * 126);

    /* each shell: its light top and dark bottom, four screws, the screen centred on it, asleep (its paper the bezel's
     * dark), the light row under it */
    for (s = 0; s < 4; ++s) {
        CtrPokeTouch_DrawIdle(canvas, 0, (uint8_t)s);
        assert(at(160, 0) == DevBlend(kShell[s], DEV_WHITE, DEV_A_LIGHT) && at(160, 239) == DevBlend(kShell[s], 0x0000, DEV_A_DARK));
        assert(at(4, 4) == at(315, 4) && at(4, 234) == at(315, 234) && at(4, 4) == at(4, 234));
        assert(at(39, 120) == kShell[s] && at(40, 120) == DEV_BEZEL_LINE && at(279, 120) == DEV_BEZEL_LINE && at(280, 120) == kShell[s]);
        assert(at(160, 44) == kShell[s] && at(160, 45) == DEV_BEZEL_LINE && at(160, 194) == DEV_BEZEL_LINE);
        assert(at(160, 195) == DevBlend(kShell[s], DEV_WHITE, DEV_A_LIGHT) && at(160, 196) == kShell[s]);
        assert(Count(46, 51, 228, 138, DEV_BEZEL) == 228 * 138);
    }
    /* the logo, 224x126, centred on the paper at (48, 57): there each pixel blended over the dark paper, nothing else
     * changed (the same in another shell) */
    assert(IDLE_LOGO_W == 224 && IDLE_LOGO_H_PX == 126);
    for (s = 0; s < 4; s += 3) {
        CtrPokeTouch_DrawIdle(canvas, 0, (uint8_t)s);
        memcpy(bare, canvas, sizeof(bare));
        CtrPokeTouch_DrawIdle(canvas, 1, (uint8_t)s);
        for (int x = 0; x < W; ++x)
            for (int y = 0; y < H; ++y) {
                int in = x >= 48 && x < 48 + 224 && y >= 57 && y < 57 + 126;

                assert(at(x, y) == (in ? IdleLogoPixel(x - 48, y - 57) : bare[x * H + H - 1 - y]));
            }
    }
    /* HOME's region key and FLY take no touch once the idle screen is drawn */
    CtrPokeTouch_Draw(canvas, &home);
    assert(sHomeRibbon.w && CtrPokeTouch_HomeRibbonHit(sHomeRibbon.x + 1, sHomeRibbon.y + 1));
    CtrPokeTouch_DrawIdle(canvas, 0, 0);
    assert(!CtrPokeTouch_HomeRibbonHit(sHomeRibbon.x + 1, sHomeRibbon.y + 1));

    /* The states for the eye. */
    CtrPokeTouch_DrawIdle(canvas, 0, 0); shot("shell");
    CtrPokeTouch_DrawIdle(canvas, 1, 0); shot("logo");
    CtrPokeTouch_DrawIdle(canvas, 1, 1); shot("blue");
    CtrPokeTouch_DrawIdle(canvas, 1, 2); shot("green");
    CtrPokeTouch_DrawIdle(canvas, 1, 3); shot("grey");
    assert(sDevKeyCut == 0);                      /* main prints PASS device idle */
}
