/* ---- card: the Trainer Card as the Definitiva (def_card_front, def_card_back), through the field dispatcher ---------
 * Included by device_render_test.c before its kSuites. */

/* The card under a point (CtrPokeTouch_CardHit): 1 on its screen or FLIP. */
static int CardHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_CardHit((const CtrPokeTouchView *)view, x, y);
}

/* The card as 3ds_bottom.c gives it (FieldCardFill: FireRed's labels and records as FireRed writes them, the name as
 * typed): its front with two stars and three badges of eight (the boards'), its back's five records; BACK shown and
 * usable, nothing held. */
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
    v.pressed = PT_NONE;
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
    c->stars = 2;
    c->badges = 0x07;
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

/* Upper case (owner decision 5): FireRed's labels and records' labels, which v0.6.0 title-cased, drawn in capitals, but
 * IDNo., drawn as FireRed writes it; the name as typed. */
static void CardUpperCase(CtrPokeTouchView v)
{
    static const char *const kTitle[6] = {"Name: ", "IDNo.", "Money", "\xC2\xA5", "Pok\xC3\xA9" "dex", "Time"};
    CtrPokeTouchView cased = v;

    for (int i = 0; i < 6; ++i)
        strcpy(cased.card.label[i], kTitle[i]);
    TouchSamePixels(&cased, &v, 1, "the front's labels in v0.6.0's case and in capitals");
    strcpy(cased.card.label[1], "IDNO.");
    TouchSamePixels(&cased, &v, 0, "IDNo. as FireRed writes it and in capitals");
    strcpy(cased.card.name, "RICK");
    TouchSamePixels(&cased, &v, 0, "the name as typed and in capitals");
    v.card.back = cased.card.back = 1;
    cased = v;
    strcpy(cased.card.row[0].label, "Hall of Fame Debut");
    strcpy(cased.card.row[2].label, "Pok\xC3\xA9mon Trades");
    TouchSamePixels(&cased, &v, 1, "the records' labels in v0.6.0's case and in capitals");
}

/* The card at FireRed's widest (trainer_card.c, the port's CtrBottomCard_*): ID 65535, money 999999, 386 caught, 999:59
 * played (the port caps the hours); the back's five records in FireRed's order with their widest values and FireRed's
 * labels (UNION TRADES & BATTLES the longest; HALL OF FAME DEBUT with its two trailing spaces). */
static CtrPokeTouchView CardMaxima(void)
{
    static const char *const kRecord[PT_CARD_ROWS][2] = {
        {"HALL OF FAME DEBUT  ", "999:59:59"}, {"LINK BATTLES", "W:9999  L:9999"}, {"POK\xC3\xA9MON TRADES", "65535"},
        {"UNION TRADES & BATTLES", "65535"}, {"BERRY CRUSH", "65535"},
    };
    CtrPokeTouchView v = CardBoard();

    v.card.id = 65535;
    v.card.money = 999999;
    v.card.caught = 386;
    v.card.hours = 999;
    v.card.minutes = 59;
    for (int i = 0; i < PT_CARD_ROWS; ++i) {
        strcpy(v.card.row[i].label, kRecord[i][0]);
        strcpy(v.card.row[i].value, kRecord[i][1]);
    }
    return v;
}

/* A DevColumnRow at box top y from x to `right` as drawn: the label (in capitals, as the card draws it) small from x,
 * the value ending at `right`, and at least DEV_COLUMN_GAP px between them with no ink of either. */
static void CardGap(int y, const char *label, const char *value, int x, int right)
{
    char upper[32];
    int end, from;

    DevUpper(upper, label, sizeof(upper));
    end = x + DevSmallTextWidth(upper);
    from = right - DevTextWidth(value);
    assert(from - end >= DEV_COLUMN_GAP && Count(x, y + 4, end - x, 7, DEV_INK2) > 0 && Count(from, y + 2, right - from, 9, DEV_INK) > 0
           && Count(end, y, from - end, 16, DEV_INK) == 0 && Count(end, y, from - end, 16, DEV_INK2) == 0);
}

/* The strip over the screen's paper in the card's colour for `stars` (either side): the colour whole along its top and
 * bottom rows and right of the stars, TRAINER CARD white from (22, capitals 21) over the colour darkened, the four
 * stars' places 13 apart from (241, 19), each 11 px over its shadow 1 px right and down: the first `stars` in gold, the
 * rest the colour's light; no fifth. */
static void CardStrip(int stars)
{
    const uint16_t colour = kDevCardColour[stars], off = DevBlend(colour, DEV_WHITE, DEV_A_CARD_OFF);
    const uint16_t sh = DevBlend(colour, 0, DEV_A_CARD_SH);

    assert(Count(14, 14, 292, 1, colour) == 292 && Count(14, 35, 292, 1, colour) == 292 && at(14, 36) == DEV_PAPER
           && Count(293, 14, 13, 22, colour) == 13 * 22 && at(150, 16) == colour);
    assert(Count(22, 20, 120, 1, DEV_WHITE) == 0 && Count(22, 21, 120, 1, DEV_WHITE) > 0 && Count(21, 21, 1, 8, DEV_WHITE) == 0
           && Count(22, 21, 1, 8, DEV_WHITE) > 0 && Count(22, 29, 120, 1, DevBlend(colour, 0, DEV_A_CARD_TITLE_SH)) > 0);
    for (int k = 0; k < 4; ++k) {
        int x = DEV_CARD_STAR_X + DEV_CARD_STAR_STEP * k;

        assert(at(x + 5, 19) == (k < stars ? DEV_CARD_STAR_ON : off) && at(x + 5, 18) == colour && at(x + 4, 19) == colour);
        assert(Count(x, 19, 12, 12, k < stars ? off : DEV_CARD_STAR_ON) == 0 && Count(x, 19, 12, 12, sh) > 0
               && at(x + 11, 23) == sh && at(x + 2, 29) == sh && at(x + 11, 29) == sh);
    }
}

/* The plan's suite (FLIP and the screen hit, BACK where drawn, FLIP sunk while the card is held, the back drawn whole),
 * plan 04b's (the strip's colour by the stars, the stars, a badge's hole, the first line's columns, the back's rule),
 * then the strip, the front's pieces (the first line, the rows, the picture, the badges), the back, BACK's states, what
 * is not drawn, the ways in and out, upper case, the counted cuts and the states for the eye. */
static void SuiteCard(void)
{
    const uint16_t green = GbaColor(CARD_ART_GREEN), blue = GbaColor(CARD_ART_BLUE);
    CtrPokeTouchView cv = CardBoard(), co = cv, a, b;
    CtrPokeTouchRect r;
    uint8_t keepWidths[256];
    char title[32];
    int s, cut;

    cardArt = 1;
    CtrPokeTouch_Draw(canvas, &co);
    /* the plan's suite */
    cv = CardBoard(); DevCheckTouch(&co, &cv); assert(DrawnBy(&cv, DevCardShown));
    /* card */ AssertHits(CardHitV, &cv, kDevCardFlip, 1); assert(CtrPokeTouch_CardHit(&cv, 160, 90) == 1 && CtrPokeTouch_CardHit(&cv, 240, 200) == 0); AssertHits(BackV, &cv, kDevTallBack, 1);
    cv.card.pressed = 1; DevCheckTouch(&co, &cv); AssertKey(kDevCardFlip, DEV_LOOK_KEY, 1); cv.card.back = 1; cv.card.pressed = 0; assert(DevWhole(DevCheckTouch(&co, &cv)));
    cv.mapGroup ^= 1; cv.speed ^= 1; assert(DevCheckTouch(&co, &cv).count == 0);
    /* plan 04b's (def_card_front, def_card_back) */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    for (s = 0; s <= 5; ++s) { cv.card.stars = (uint8_t)s; DevCheckTouch(&co, &cv); assert(at(150, 16) == kDevCardColour[s > 4 ? 4 : s]); }
    cv.card.stars = 2; DevCheckTouch(&co, &cv);
    assert(Count(241, 19, 12, 12, DEV_CARD_STAR_ON) > 0 && Count(267, 19, 12, 12, DEV_CARD_STAR_ON) == 0
           && Count(267, 19, 12, 12, DevBlend(kDevCardColour[2], DEV_WHITE, DEV_A_CARD_OFF)) > 0 && Count(22, 21, 120, 9, DEV_WHITE) > 0);
    r = DevCardBadge(7); assert(at(r.x + 16, r.y + 16) == DEV_PAPER_HOLE && at(r.x + 16, r.y + 1) == DEV_PAPER_LINE);
    assert(Count(206, 46, 30, 7, DEV_INK2) > 0 && Count(56, 45, 30, 9, DEV_INK) > 0 && Count(209, 62, 16, 50, DEV_INK) == 0);
    cv.card.back = 1; DevCheckTouch(&co, &cv); assert(at(100, 58) == DEV_RULE && at(150, 16) == kDevCardColour[2]);
    cv.card.stars = 3; assert(DevWhole(DevCheckTouch(&co, &cv)));
    AssertHits(CardHitV, &cv, kDevCardFlip, 1); AssertHits(BackV, &cv, kDevTallBack, 1);

    /* the screen is hit on itself, FLIP and BACK on their keys, nothing between or under them */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    AssertRowHits(CardHitV, &cv, kDevTallScreen, 1);
    assert(!CardHitV(&cv, 160, kDevTallScreen.y + kDevTallScreen.h) && !BackV(&cv, 160, 100) && !BackV(&cv, 100, 200));
    /* the strip: each count of stars its colour (FireRed's five: blue, green, bronze, silver, gold), on the front and
     * on the back; more than four is four */
    for (s = 0; s <= 4; ++s) {
        cv.card.stars = (uint8_t)s; cv.card.back = 0; DevCheckTouch(&co, &cv); CardStrip(s);
        cv.card.back = 1; DevCheckTouch(&co, &cv); CardStrip(s);
    }
    cv.card.stars = 9; assert(DevCheckTouch(&co, &cv).count == 0); CardStrip(4);
    a = b = CardBoard(); a.card.stars = 4; b.card.stars = 7; TouchSamePixels(&a, &b, 1, "four and seven stars");
    b.card.stars = 3; TouchSamePixels(&a, &b, 0, "four and three stars");
    a.card.back = b.card.back = 1; TouchSamePixels(&a, &b, 0, "the back with four and three stars");
    /* the front: FLIP up, BACK dark; the first line (NAME small from (22, capitals 46), the name from x 56 (capitals
     * 45), IDNo. small from (206, 46), the ID ending at 298 (capitals 45)). The rows checked empty over a text are over
     * what FireRed's own font raises above the capitals (digits and an accent by a row, the yen sign by two) */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    AssertKey(kDevCardFlip, DEV_LOOK_KEY, 0); AssertKey(kDevTallBack, DEV_LOOK_DARK, 0);
    assert(Count(8, 188, 228, 42, DEV_KEY_INK) > 0 && Count(244, 188, 68, 42, DEV_DARK_INK) > 0);
    assert(Count(22, 45, 30, 1, DEV_INK2) == 0 && Count(22, 46, 30, 1, DEV_INK2) > 0 && Count(21, 40, 1, 20, DEV_INK2) == 0
           && Count(22, 46, 1, 7, DEV_INK2) > 0);
    assert(Count(55, 45, 1, 8, DEV_INK) == 0 && Count(56, 45, 1, 8, DEV_INK) > 0 && Count(56, 44, 40, 1, DEV_INK) == 0);
    assert(Count(205, 46, 1, 7, DEV_INK2) == 0 && Count(206, 46, 1, 7, DEV_INK2) > 0 && Count(206, 45, 30, 1, DEV_INK2) == 0);
    assert(Count(293, 45, 4, 8, DEV_INK) > 0 && Count(298, 40, 8, 20, DEV_INK) == 0 && Count(250, 43, 48, 1, DEV_INK) == 0);
    /* a wider NAME takes the name 6 past it */
    a = CardBoard(); strcpy(a.card.label[0], "NOMBRE: "); CtrPokeTouch_Draw(canvas, &a);
    assert(Count(57, 45, 1, 8, DEV_INK) == 0 && Count(58, 45, 1, 8, DEV_INK) > 0);
    CtrPokeTouch_Draw(canvas, &cv);
    /* MONEY, POKeDEX, TIME (DevColumnRow from 22 to 208: the label's capitals 66 / 84 / 102, the value's a row higher,
     * ending at 208), nothing between them and the picture */
    for (int i = 1; i < 4; ++i) {
        r = DevCardRow(i);
        assert(r.y == 44 + 18 * i && Count(22, r.y + 2, 40, 1, DEV_INK2) == 0 && Count(22, r.y + 4, 40, 1, DEV_INK2) > 0
               && Count(170, r.y, 38, 1, DEV_INK) == 0 && Count(204, r.y + 3, 4, 8, DEV_INK) > 0);
    }
    assert(Count(208, 60, 18, 74, DEV_INK) == 0 && Count(208, 60, 18, 74, DEV_INK2) == 0);
    /* the trainer's picture at (226, 60) */
    assert(Count(kDevCardPic.x, kDevCardPic.y, kDevCardPic.w, kDevCardPic.h, green) == 48 * 48
           && at(kDevCardPic.x + 8, kDevCardPic.y + 8) == green && at(kDevCardPic.x + 7, kDevCardPic.y + 8) != green);
    /* the eight badges' places from (20, 134), 3 apart: the three had at 2x, the rest holes in the paper (a rim at the
     * top, the hole, the paper at the corners) */
    for (int i = 0; i < 8; ++i) {
        r = DevCardBadge(i);
        if (i < 3)
            assert(Count(r.x, r.y, r.w, r.h, blue) == 32 * 32);
        else
            assert(Count(r.x, r.y, r.w, r.h, blue) == 0 && at(r.x + 16, r.y + 16) == DEV_PAPER_HOLE
                   && at(r.x + 16, r.y + 1) == DEV_PAPER_LINE && at(r.x + 16, r.y + 30) == DEV_PAPER_HOLE
                   && at(r.x, r.y) == DEV_PAPER && at(r.x + 31, r.y + 31) == DEV_PAPER);
    }
    /* all eight had: no hole */
    cv.card.badges = 0xff; DevCheckTouch(&co, &cv);
    for (int i = 0; i < 8; ++i) {
        r = DevCardBadge(i);
        assert(Count(r.x, r.y, r.w, r.h, blue) == 32 * 32);
    }
    assert(Count(14, 36, 292, 138, DEV_PAPER_HOLE) == 0 && Count(14, 36, 292, 138, DEV_PAPER_LINE) == 0);
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
    cv.card.money = 7; cv.card.badges = 1; cv.card.id = 1; cv.card.hasDex = 0;
    strcpy(cv.card.label[2], "CASH"); assert(DevCheckTouch(&co, &cv).count == 0);

    /* the back: the strip, the name from (22, capitals 45), the rule (DEV_RULE) at y 58, the five records (the label
     * small from 22, capitals 70 + 21i; the value ending at 298, capitals a row higher); no picture or badges */
    cv = CardBoard(); cv.card.back = 1; DevCheckTouch(&co, &cv);
    CardStrip(2);
    assert(Count(kDevCardRule.x, kDevCardRule.y, kDevCardRule.w, kDevCardRule.h, DEV_RULE) == kDevCardRule.w && kDevCardRule.y == 58);
    assert(Count(22, 44, 40, 1, DEV_INK) == 0 && Count(22, 45, 40, 1, DEV_INK) > 0 && Count(21, 40, 1, 16, DEV_INK) == 0);
    for (int i = 0; i < PT_CARD_ROWS; ++i) {
        r = DevCardBackRow(i);
        assert(r.y == 66 + 21 * i && Count(22, r.y + 2, 60, 1, DEV_INK2) == 0 && Count(22, r.y + 4, 60, 1, DEV_INK2) > 0
               && Count(250, r.y + 1, 48, 1, DEV_INK) == 0 && Count(293, r.y + 3, 4, 8, DEV_INK) > 0
               && Count(298, r.y, 8, r.h, DEV_INK) == 0);
    }
    assert(Count(kDevCardPic.x, kDevCardPic.y, kDevCardPic.w, kDevCardPic.h, green) == 0 && Count(14, 100, 292, 70, blue) == 0
           && Count(14, 36, 292, 138, DEV_PAPER_HOLE) == 0 && Count(14, 36, 292, 138, DEV_CARD_STAR_ON) == 0);
    /* four records: the fifth's place is empty, and a change of it presents nothing */
    cv.card.rows = 4; DevCheckTouch(&co, &cv);
    r = DevCardBackRow(4); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) == 0 && Count(r.x, r.y, r.w, r.h, DEV_INK2) == 0);
    strcpy(cv.card.row[4].value, "99"); assert(DevCheckTouch(&co, &cv).count == 0);

    /* BACK: sunk under a touch, grey while it cannot be used, gone when not shown (the shell there); FLIP stays */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    cv.backPressed = 1; DevCheckTouch(&co, &cv); AssertKey(kDevTallBack, DEV_LOOK_DARK, 1);
    cv.backPressed = 0; cv.backEnabled = 0; DevCheckTouch(&co, &cv); assert(at(247, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    cv.backPressed = 1; assert(DevCheckTouch(&co, &cv).count == 0);                    /* a press on a grey BACK: not drawn */
    cv.backEnabled = 1; cv.backPressed = 0; cv.backVisible = 0; DevCheckTouch(&co, &cv);
    assert(at(247, 210) == DEV_SHELL_RED && !BackV(&cv, 247, 210)); AssertKey(kDevCardFlip, DEV_LOOK_KEY, 0);
    /* what the card does not draw presents nothing: the bar, HOME's keys, the map */
    cv = CardBoard(); DevCheckTouch(&co, &cv);
    strcpy(cv.bar[0], "Anything"); cv.bar[1][0] = 0; cv.enabled[PT_BAG] = 1; cv.playerX = 3;
    assert(DevCheckTouch(&co, &cv).count == 0);
    cv.shell = 3; assert(DevWhole(DevCheckTouch(&co, &cv)) && at(1, 100) == DEV_SHELL_GREY);
    cv.shell = 0; DevCheckTouch(&co, &cv);
    strcpy(cv.card.name, "Leaf"); assert(DevWhole(DevCheckTouch(&co, &cv)));
    /* the ways in and out: the GBA picture (a link card: in the frame's screen, nothing of the card hit, the frame's
     * BACK) */
    cv.viewport = 1; assert(DevWhole(DevCheckTouch(&co, &cv)) && DrawnBy(&cv, DevFrameShown));
    assert(!CardHitV(&cv, 160, 90) && !CardHitV(&cv, 100, 200) && CtrPokeTouch_BackHit(&cv, 278, 209) && !CtrPokeTouch_BackHit(&cv, 180, 210));
    cv = CardBoard(); cv.card.on = 0; assert(!CardHitV(&cv, 160, 90));
    cv = CardBoard(); assert(DevWhole(DevCheckTouch(&co, &cv)));
    CardUpperCase(CardBoard());
    /* no silent cut, on one draw each: the title past 236 (short of the stars), the name past 200 (short of IDNo.) and
     * IDNo. into the ID are cut and counted. Capitals 16 px wide (the widest a glyph is drawn) cut ES's title, not EN's
     * or DE's, nor FLIP or "Rick"; FireRed's seven letters fit as the stubs draw them */
    assert(sDevKeyCut == 0);
    a = CardBoard(); a.backVisible = 0;
    memcpy(keepWidths, fontWidths, sizeof(keepWidths));
    for (unsigned code = 0xbb; code <= 0xd4; ++code)       /* A..Z */
        fontWidths[code] = 16;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    DevUpper(title, CtrUiText(UIS_CARD_TITLE), sizeof(title));
    cut = DevTextWidth(title) > DEV_CARD_TITLE_END - 22;
    CtrPokeTouch_Draw(canvas, &a); assert(sDevKeyCut == cut && Count(DEV_CARD_TITLE_END, 14, 5, 22, DEV_WHITE) == 0);
    sDevKeyCut = 0; strcpy(a.card.name, "RICKRICKRICK"); CtrPokeTouch_Draw(canvas, &a);
    assert(sDevKeyCut == cut + 1 && Count(200, 40, 6, 20, DEV_INK) == 0 && Count(184, 45, 4, 8, DEV_INK) > 0);  /* R: 9th */
    memcpy(fontWidths, keepWidths, sizeof(keepWidths));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    sDevKeyCut = 0; strcpy(a.card.name, "MMMMMMM"); CtrPokeTouch_Draw(canvas, &a); assert(sDevKeyCut == 0);
    strcpy(a.card.label[1], "IDENTIFICATION"); CtrPokeTouch_Draw(canvas, &a);
    assert(sDevKeyCut == 1 && Count(298 - DevTextWidth("12345") - 6, 46, 6, 7, DEV_INK2) == 0);
    sDevKeyCut = 0;
    /* FireRed's widest card (CardMaxima): each row's label and value apart by paper and nothing cut, on either side */
    a = CardMaxima(); CtrPokeTouch_Draw(canvas, &a); assert(sDevKeyCut == 0);
    r = DevCardRow(1); CardGap(r.y, a.card.label[2], "\xC2\xA5" "999999", r.x, r.x + r.w);
    r = DevCardRow(2); CardGap(r.y, a.card.label[4], "386", r.x, r.x + r.w);
    r = DevCardRow(3); CardGap(r.y, a.card.label[5], "999:59", r.x, r.x + r.w);
    a.card.back = 1; CtrPokeTouch_Draw(canvas, &a); assert(sDevKeyCut == 0);
    for (int i = 0; i < PT_CARD_ROWS; ++i) {
        r = DevCardBackRow(i);
        CardGap(r.y, a.card.row[i].label, a.card.row[i].value, r.x, r.x + r.w);
    }
    /* a row too wide (never FireRed's) is counted once, its value whole and its label cut DEV_COLUMN_GAP px short of it */
    a = CardMaxima(); strcpy(a.card.label[2], "MMMMMMMMMMMMMMM"); strcpy(a.card.label[3], "999999999999999");
    a.card.money = 4294967295u; CtrPokeTouch_Draw(canvas, &a);
    r = DevCardRow(1); s = r.x + r.w - DevTextWidth("9999999999999994294967295");
    assert(sDevKeyCut == 1 && s > r.x + DEV_COLUMN_GAP && Count(s, r.y + 2, 4, 9, DEV_INK) > 0
           && Count(s - DEV_COLUMN_GAP, r.y, DEV_COLUMN_GAP, r.h, DEV_INK2) == 0 && Count(r.x, r.y + 4, 4, 7, DEV_INK2) > 0);
    sDevKeyCut = 0;
    CtrPokeTouch_Draw(canvas, &co);

    /* The states for the eye, each through Update first. */
    cv = CardBoard(); DevCheckTouch(&co, &cv); shot("front");
    cv.card.pressed = 1; DevCheckTouch(&co, &cv); shot("pressed");
    cv.card.pressed = 0; cv.card.back = 1; DevCheckTouch(&co, &cv); shot("back");
    cardArt = 0;
    assert(sDevKeyCut == 0);                      /* main prints PASS device card */
}
