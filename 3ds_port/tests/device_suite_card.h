/* ---- card: the Trainer Card as the red device, through the field dispatcher (no board; plan 3 Task 2) ------------------
 * Included by device_render_test.c before its kSuites. */

/* The card under a point (CtrPokeTouch_CardHit): 1 on its screen or FLIP. */
static int CardHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_CardHit((const CtrPokeTouchView *)view, x, y);
}

/* The card as 3ds_bottom.c gives it (FieldCardFill: FireRed's labels and records as FireRed writes them, the name as
 * typed): its front with 8 badges and 3 stars, its back's five records; BACK shown and usable, nothing held. */
static CtrPokeTouchView CardBoard(void)
{
    static const char *const kLabel[6] = {"NAME: ", "IDNo.", "MONEY", "\xC2\xA5", "POK\xC3\xA9" "DEX", "TIME"};
    static const char *const kRecord[PT_CARD_ROWS][2] = {
        {"HALL OF FAME DEBUT", "0:42"}, {"LINK BATTLES", "3 W 2 L"}, {"POK\xC3\xA9MON TRADES", "5"},
        {"BERRY CRUSH", "1"}, {"UNION ROOM TRADES", "2"},
    };
    CtrPokeTouchView v;
    CtrPokeTouchCard *c = &v.card;

    memset(&v, 0, sizeof(v));
    v.pressed = v.selected = PT_NONE;
    c->on = 1;
    strcpy(c->name, "Rick");
    for (int i = 0; i < 6; ++i)
        strcpy(c->label[i], kLabel[i]);
    c->id = 12345;
    c->money = 128450;
    c->hasDex = 1;
    c->caught = 118;
    c->hours = 47;
    c->minutes = 5;
    c->stars = 3;
    c->badges = 0xff;
    c->rows = PT_CARD_ROWS;
    for (int i = 0; i < PT_CARD_ROWS; ++i) {
        strcpy(c->row[i].label, kRecord[i][0]);
        strcpy(c->row[i].value, kRecord[i][1]);
    }
    snprintf(v.bar[0], PT_BAR_LEN, "%s", CtrUiText(UIS_CARD_TITLE));      /* the bar: not drawn by the card */
    snprintf(v.bar[1], PT_BAR_LEN, "%s", CtrUiText(UIS_CARD_TURN));
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* Upper case (owner decision 5): FireRed's labels and records' labels, which v0.6.0 title-cased, drawn in capitals; the
 * name as typed. */
static void CardUpperCase(CtrPokeTouchView v)
{
    static const char *const kTitle[6] = {"Name: ", "Idno.", "Money", "\xC2\xA5", "Pok\xC3\xA9" "dex", "Time"};
    CtrPokeTouchView cased = v;

    for (int i = 0; i < 6; ++i)
        strcpy(cased.card.label[i], kTitle[i]);
    TouchSamePixels(&cased, &v, 1, "the front's labels in v0.6.0's case and in capitals");
    strcpy(cased.card.name, "RICK");
    TouchSamePixels(&cased, &v, 0, "the name as typed and in capitals");
    v.card.back = cased.card.back = 1;
    cased = v;
    strcpy(cased.card.row[0].label, "Hall of Fame Debut");
    strcpy(cased.card.row[2].label, "Pok\xC3\xA9mon Trades");
    TouchSamePixels(&cased, &v, 1, "the records' labels in v0.6.0's case and in capitals");
}

/* The plan's suite (FLIP and the screen hit, BACK where drawn, FLIP sunk while the card is held, the back drawn whole),
 * then the front's pieces (the picture, the stars, the badges' places), its rows, the back, BACK's states, what is not
 * drawn, the ways in and out, upper case and the states for the eye. */
static void SuiteCard(void)
{
    const uint16_t gold = UI_RGB(0xE8, 0xB8, 0x30), green = GbaColor(CARD_ART_GREEN), blue = GbaColor(CARD_ART_BLUE);
    CtrPokeTouchView cv = CardBoard(), co = cv, a, b;
    CtrPokeTouchRect r;

    cardArt = 1;
    CtrPokeTouch_Draw(canvas, &co);
    /* the plan's suite */
    cv = CardBoard(); DevCheckTouch(&co, &cv); assert(CtrPokeTouch_DeviceShown(&cv));
    /* card */ AssertHits(CardHitV, &cv, kDevCardFlip, 1); assert(CtrPokeTouch_CardHit(&cv, 160, 90) == 1 && CtrPokeTouch_CardHit(&cv, 240, 200) == 0); AssertHits(BackV, &cv, kDevSumBack, 1);
    cv.card.pressed = 1; DevCheckTouch(&co, &cv); AssertKey(kDevCardFlip, DEV_LOOK_KEY, 1); cv.card.back = 1; cv.card.pressed = 0; assert(DevWhole(DevCheckTouch(&co, &cv)));
    cv.mapGroup ^= 1; cv.speed ^= 1; assert(DevCheckTouch(&co, &cv).count == 0);

    /* the screen is hit on itself, FLIP and BACK on their keys, nothing between or under them */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    AssertRowHits(CardHitV, &cv, kDevTallScreen, 1);
    assert(!CardHitV(&cv, 160, kDevTallScreen.y + kDevTallScreen.h) && !BackV(&cv, 160, 100) && !BackV(&cv, 100, 200));
    /* the front: the picture, three stars of five, the eight badges at 2x on their places; FLIP up, BACK dark */
    assert(Count(kDevCardPic.x, kDevCardPic.y, kDevCardPic.w, kDevCardPic.h, green) == 48 * 48);
    for (int i = 0; i < DEV_CARD_STARS; ++i)
        assert((Count(DEV_CARD_STAR_X + DEV_CARD_STAR_STEP * i, DEV_CARD_STAR_Y, 11, 11, gold) > 0) == (i < 3));
    for (int i = 0; i < 8; ++i) {
        r = DevCardBadge(i);
        assert(Count(r.x, r.y, r.w, r.h, blue) == 32 * 32);
    }
    AssertKey(kDevCardFlip, DEV_LOOK_KEY, 0); AssertKey(kDevSumBack, DEV_LOOK_DARK, 0);
    assert(Count(8, 188, 228, 42, DEV_KEY_INK) > 0 && Count(244, 188, 68, 42, DEV_DARK_INK) > 0);
    /* the first line: NAME and the name at its left, IDNo. and the ID at its right; then MONEY, POKeDEX, TIME */
    r = DevCardRow(0);
    assert(Count(r.x, r.y, 30, r.h, DEV_INK2) > 0 && Count(r.x + 30, r.y, 40, r.h, DEV_INK) > 0);
    assert(Count(r.x + r.w - 40, r.y, 40, r.h, DEV_INK) > 0 && Count(r.x + r.w - 100, r.y, 60, r.h, DEV_INK2) > 0);
    for (int i = 1; i < 4; ++i) {
        r = DevCardRow(i);
        assert(Count(r.x, r.y, 40, r.h, DEV_INK2) > 0 && Count(r.x + r.w - 30, r.y, 30, r.h, DEV_INK) > 0);
    }
    /* badges not earned: their places in the paper's second tone; more than five stars draw five */
    cv.card.badges = 0x0f; DevCheckTouch(&co, &cv);
    for (int i = 0; i < 8; ++i) {
        r = DevCardBadge(i);
        assert(Count(r.x, r.y, r.w, r.h, blue) == (i < 4 ? 32 * 32 : 0) && Count(r.x, r.y, r.w, r.h, DEV_PAPER2) == (i < 4 ? 0 : 32 * 32));
    }
    a = b = CardBoard(); a.card.stars = 5; b.card.stars = 7; TouchSamePixels(&a, &b, 1, "five and seven stars");
    b.card.stars = 4; TouchSamePixels(&a, &b, 0, "five and four stars");
    /* without a Pokedex its row goes and TIME takes it */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    cv.card.hasDex = 0; assert(DevWhole(DevCheckTouch(&co, &cv)));
    r = DevCardRow(3); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) == 0 && Count(r.x, r.y, r.w, r.h, DEV_INK2) == 0);
    r = DevCardRow(2); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) > 0);
    cv.card.caught = 3; assert(DevCheckTouch(&co, &cv).count == 0);                     /* the count is not drawn */
    /* the front does not draw the back's records, nor the back the front's facts */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    strcpy(cv.card.row[1].value, "9 W 9 L"); cv.card.rows = 2; assert(DevCheckTouch(&co, &cv).count == 0);
    cv = CardBoard(); cv.card.back = 1; DevCheckTouch(&co, &cv);
    cv.card.money = 7; cv.card.stars = 1; cv.card.badges = 1; cv.card.id = 1; cv.card.hasDex = 0;
    strcpy(cv.card.label[2], "CASH"); assert(DevCheckTouch(&co, &cv).count == 0);

    /* the back: the name, the rule under it, the five records (label small at the left, value at the right); no
     * picture, stars or badges */
    cv = CardBoard(); cv.card.back = 1; DevCheckTouch(&co, &cv);
    assert(Count(kDevCardRule.x, kDevCardRule.y, kDevCardRule.w, kDevCardRule.h, DEV_INK_SH) == kDevCardRule.w);
    r = DevCardRow(0); assert(Count(r.x, r.y, 30, r.h, DEV_INK) > 0);
    for (int i = 0; i < PT_CARD_ROWS; ++i) {
        r = DevCardBackRow(i);
        assert(Count(r.x, r.y, 60, r.h, DEV_INK2) > 0 && Count(r.x + r.w - 20, r.y, 20, r.h, DEV_INK) > 0);
    }
    assert(Count(kDevCardPic.x, kDevCardPic.y, kDevCardPic.w, kDevCardPic.h, green) == 0 && Count(14, 100, 292, 70, blue) == 0
           && Count(14, 100, 292, 70, gold) == 0);
    /* four records: the fifth's place is empty, and a change of it presents nothing */
    cv.card.rows = 4; DevCheckTouch(&co, &cv);
    r = DevCardBackRow(4); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) == 0 && Count(r.x, r.y, r.w, r.h, DEV_INK2) == 0);
    strcpy(cv.card.row[4].value, "99"); assert(DevCheckTouch(&co, &cv).count == 0);

    /* BACK: sunk under a touch, grey while it cannot be used, gone when not shown (the shell there); FLIP stays */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    cv.backPressed = 1; DevCheckTouch(&co, &cv); AssertKey(kDevSumBack, DEV_LOOK_DARK, 1);
    cv.backPressed = 0; cv.backEnabled = 0; DevCheckTouch(&co, &cv); assert(at(247, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    cv.backPressed = 1; assert(DevCheckTouch(&co, &cv).count == 0);                    /* a press on a grey BACK: not drawn */
    cv.backEnabled = 1; cv.backPressed = 0; cv.backVisible = 0; DevCheckTouch(&co, &cv);
    assert(at(247, 210) == DEV_SHELL_RED && !BackV(&cv, 247, 210)); AssertKey(kDevCardFlip, DEV_LOOK_KEY, 0);
    /* what the card does not draw presents nothing: the bar, the column's fields, the map */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    strcpy(cv.bar[0], "Anything"); cv.bar[1][0] = 0; cv.registeredItem = 7; cv.enabled[PT_BAG] = 1; cv.playerX = 3;
    assert(DevCheckTouch(&co, &cv).count == 0);
    cv.shell = 3; assert(DevWhole(DevCheckTouch(&co, &cv)) && at(1, 100) == DEV_SHELL_GREY);
    cv.shell = 0; DevCheckTouch(&co, &cv);
    strcpy(cv.card.name, "Leaf"); assert(DevWhole(DevCheckTouch(&co, &cv)));
    /* the ways in and out: the GBA picture (a link card: an ember view, nothing of the card hit, BACK's own rectangle) */
    cv.viewport = 1; assert(DevWhole(DevCheckTouch(&co, &cv)) && !CtrPokeTouch_DeviceShown(&cv));
    assert(!CardHitV(&cv, 160, 90) && !CardHitV(&cv, 100, 200) && CtrPokeTouch_BackHit(&cv, 180, 210));
    cv = CardBoard(); cv.card.on = 0; assert(!CardHitV(&cv, 160, 90));
    cv = CardBoard(); assert(DevWhole(DevCheckTouch(&co, &cv)));
    CardUpperCase(CardBoard());
    CtrPokeTouch_Draw(canvas, &co);

    /* The states for the eye, each through Update first. */
    cv = CardBoard(); DevCheckTouch(&co, &cv); shot("front");
    cv.card.pressed = 1; DevCheckTouch(&co, &cv); shot("pressed");
    cv.card.pressed = 0; cv.card.back = 1; DevCheckTouch(&co, &cv); shot("back");
    cardArt = 0;
    assert(sDevKeyCut == 0);                      /* main prints PASS device card */
}
