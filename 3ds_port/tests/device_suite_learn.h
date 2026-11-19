/* ---- learn: the move to forget for a new one as the red device (board def_learn; plan 04a Task 14), through the field
 * dispatcher ----------------------------------------------------------------------------------------------------------
 * Included by device_render_test.c before its kSuites. Also the harness's species picture: CtrPokeTouch_DexMonPic
 * gives a 48x48 square in STUB_PIC (a clear border of 8) while a suite sets picArt (none otherwise, as the other art
 * stubs); a suite that sets it forgets the pictures kept (sFDPic). */

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

/* The grid's title's paper (kDevGridTitle 6 px in), where FireRed's question is. */
static const CtrPokeTouchRect kLearnTitlePaper = {14, 12, 292, 16};

/* The hints the footer gives for the cursor's row, the board's words: an old move, an HM, the new one. */
static const char *const kLearnForget = CTR_TR("TAP AGAIN TO FORGET IT.", "TOCA OTRA VEZ PARA OLVIDARLO.",
                                               "NOCHMAL TIPPEN ZUM VERGESSEN.");
static const char *const kLearnHm = CTR_TR("HM MOVES CAN'T BE FORGOTTEN.", "LAS MO NO SE PUEDEN OLVIDAR.",
                                           "VM K\xC3\x96NNEN NICHT VERGESSEN WERDEN.");
static const char *const kLearnSkip = CTR_TR("TAP AGAIN TO SKIP IT.", "TOCA OTRA VEZ PARA NO APRENDERLO.",
                                             "NOCHMAL TIPPEN ZUM AUSLASSEN.");

static int LearnHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_LearnHit((const CtrPokeTouchView *)view, x, y);
}

/* The board's four moves and the new one (row 4), FireRed's names and types. */
static const struct { const char *name, *type, *pp, *power, *accuracy; int8_t t; } kLearnMove[PT_LEARN_ROWS] = {
    {"FLAMETHROWER", "FIRE", "15/15", "95", "100", 10}, {"SCRATCH", "NORMAL", "15/35", "40", "100", 0},
    {"GROWL", "NORMAL", "40/40", "", "100", 0}, {"BRICK BREAK", "FIGHT", "3/15", "75", "100", 1},
    {"SLASH", "NORMAL", "20/20", "70", "100", 0},
};

/* The move to forget as 3ds_bottom.c gives it (FieldLearnFill), the board's: four moves and SLASH new in row 4 (FireRed's
 * names and types in capitals), the cursor on SCRATCH (row 1), the Pokemon called by its species' name (recipe 0251's
 * title case: nameDefault), its level the number alone (recipe 0222's CtrBottomLearn_Name), port-written stand-ins for
 * the question and the description; BACK shown and usable. */
static CtrPokeTouchView LearnBoard(void)
{
    CtrPokeTouchView v;
    CtrPokeTouchLearn *l = &v.learn;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    l->on = 1;
    l->cursor = 1;
    l->pressed = -1;
    l->species = 5;
    l->nameDefault = 1;
    strcpy(l->nickname, "Charmeleon");
    strcpy(l->level, "36");                                     /* the fill gives the number */
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
    strcpy(l->desc, "Port-written words in place of what a move does.");
    snprintf(v.bar[0], PT_BAR_LEN, "Move / item");             /* the bar: not drawn by the move to forget */
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* The footer's hint as drawn: `hint` small in the secondary ink from x 20, capitals 218, as wide as its words and the
 * only secondary ink on the paper's lower rows; NULL: none. */
static void LearnHint(const char *hint)
{
    int w = hint ? DevSmallTextWidth(hint) : 0;

    if (!hint) {
        assert(Count(14, 196, 216, 30, DEV_INK2) == 0);
        return;
    }
    assert(w <= 204 && Count(20, 218, w, 7, DEV_INK2) > 0 && Count(20 + w - 5, 218, 5, 7, DEV_INK2) > 0);
    assert(Count(14, 196, 216, 20, DEV_INK2) == 0 && Count(14, 196, 6, 30, DEV_INK2) == 0);   /* the game's ' reaches 216 */
    assert(Count(20 + w, 196, 210 - w, 30, DEV_INK2) == 0 && Count(20, 225, 204, 1, DEV_INK2) == 0);
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
    strcpy(cased.learn.nickname, "CHARMELEON");
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

/* The plan's suite (every row hit where drawn, the new one's chip and the rule, BACK in the exit corner, the hint; the
 * cursor's row's HM lock drawn, another's not), then def_learn's pieces where the board puts them: the title, the wide
 * screen, the column, the rows, the footer and its hint for each kind of row, a name too long for the normal font, a
 * touch, BACK's states, the ways in and out, upper case and the states for the eye. */
static void SuiteLearn(void)
{
    CtrPokeTouchView lv = LearnBoard(), lo = lv, other;
    CtrPokeTouchRect r;
    int i, w, x;

    picArt = 1;
    ForgetPictures();
    CtrPokeTouch_Draw(canvas, &lo);
    /* the plan's suite (the board: four moves and SLASH new, the cursor on row 1) */
    lv = LearnBoard(); DevCheckTouch(&lo, &lv); assert(DrawnBy(&lv, DevLearnShown));
    /* learn */ for (i = 0; i < 5; ++i) AssertRowHits(LearnHitV, &lv, DevLearnSlot(i), PT_LEARN_HIT_ROW + i);
    AssertHits(BackV, &lv, kDevGridExit, 1); assert(at(160, 146) == DEV_RULE && Count(130, 155, 30, 12, DEV_TAG_NEW) > 0);
    assert(Count(216, DevLearnSlot(0).y + 5, 40, 12, UiTypeColor(lv.learn.row[0].type)) > 0 && Count(20, 218, 204, 7, DEV_INK2) > 0);
    lv.learn.row[1].locked = 1; assert(DevWhole(DevCheckTouch(&lo, &lv)));
    lv.learn.row[2].locked ^= 1; lv.mapGroup ^= 1; lv.speed ^= 1; assert(DevCheckTouch(&lo, &lv).count == 0);
    /* an empty row neither drawn nor hit; between the rows, on the rule and on the column nothing is */
    lv = LearnBoard(); lv.learn.row[2].move = lv.learn.row[3].move = 0; DevCheckTouch(&lo, &lv);
    assert(CtrPokeTouch_LearnHit(&lv, 130, DevLearnSlot(2).y + 5) == -1);
    assert(CtrPokeTouch_LearnHit(&lv, 130, 70) == -1 && CtrPokeTouch_LearnHit(&lv, 130, 146) == -1 && CtrPokeTouch_LearnHit(&lv, 60, 100) == -1);
    for (i = 2; i < 4; ++i) {
        r = DevLearnSlot(i);
        assert(Count(r.x, r.y, r.w, r.h, DEV_PAPER) == r.w * r.h);
    }

    /* the title: FireRed's question on the grid's title, capitals 16; the grid's wide screen (its paper 14..305 x
     * 46..177), nothing of the old tall note over the grid's (the shell at 188) */
    lv = LearnBoard(); DevCheckTouch(&lo, &lv);
    assert(Count(20, 16, 280, 8, DEV_INK) > 0 && Count(kLearnTitlePaper.x, kLearnTitlePaper.y, kLearnTitlePaper.w, 2, DEV_INK) == 0);
    assert(at(300, 177) == DEV_PAPER && at(300, 178) == DEV_BEZEL && at(305, 100) == DEV_PAPER && at(306, 100) == DEV_BEZEL);
    assert(at(100, 188) == DEV_SHELL_RED && at(100, 189) == DEV_SHELL_RED);
    /* the column (the paper's second tone, kDevLearnColumn): the name capitals 52, the level small capitals 66 in the
     * secondary ink with the Summary's prefix ("Lv36": the fill gives the number), the picture in kDevLearnPic, the
     * cursor's move's POWER and ACCURACY small (capitals 150 and 164) with its numbers ending at 110 (capitals 149 and
     * 163) */
    assert(at(14, 46) == DEV_PAPER2 && at(117, 177) == DEV_PAPER2 && at(118, 177) == DEV_PAPER && at(14, 45) == DEV_BEZEL);
    assert(Count(20, 52, 90, 8, DEV_INK) > 0 && Count(20, 46, 90, 6, DEV_INK) == 0 && Count(20, 60, 90, 16, DEV_INK) == 0);
    assert(Count(20, 66, 90, 7, DEV_INK2) > 0 && Count(20, 60, 90, 6, DEV_INK2) == 0);
    {
        char level[16];
        int bare = DevSmallTextWidth(lv.learn.level), full;

        snprintf(level, sizeof(level), CTR_TR("Lv%s", "Nv%s", "Lv%s"), lv.learn.level);
        full = DevSmallTextWidth(level);
        assert(full > bare && Count(20 + bare, 66, full - bare, 7, DEV_INK2) > 0 && Count(20 + full, 60, 90 - full, 16, DEV_INK2) == 0);
    }
    assert(Count(kDevLearnPic.x, kDevLearnPic.y, kDevLearnPic.w, kDevLearnPic.h, STUB_PIC) == 48 * 48);
    assert(Count(kDevLearnPic.x + 8, kDevLearnPic.y + 8, 48, 48, STUB_PIC) == 48 * 48);
    assert(Count(20, 150, 50, 7, DEV_INK2) > 0 && Count(20, 164, 50, 7, DEV_INK2) > 0);
    w = DevTextWidth("40"); assert(Count(110 - w, 149, w, 8, DEV_INK) > 0 && Count(110 - w, 141, w, 6, DEV_INK) == 0);
    w = DevTextWidth("100"); assert(Count(110 - w, 163, w, 8, DEV_INK) > 0 && Count(110, 140, 8, 38, DEV_INK) == 0);
    /* the rows, 22 tall and 24 apart (the board's), the new one under the rule: the cursor's (SCRATCH) inverted; each its
     * name from x 130 (capitals r.y + 7, its shadow off the cursor's row) cut before 212, its type's chip 12 tall at
     * (216, r.y + 5), its PP ending at 298 (capitals r.y + 7, no shadow; the game's digits and / reach 2 rows over the
     * capitals, so the probe over them stops short) */
    assert(at(124, 71) == DEV_PAPER && at(124, 72) == DEV_BEZEL && at(299, 93) == DEV_BEZEL && at(299, 94) == DEV_PAPER);
    assert(at(123, 80) == DEV_PAPER && at(300, 80) == DEV_PAPER);
    for (i = 0; i < PT_LEARN_ROWS; ++i) {
        uint16_t bg = i == 1 ? DEV_BEZEL : DEV_PAPER, ink = DevRowInk(i == 1), type = UiTypeColor(kLearnMove[i].t);
        int pp = DevTextWidth(kLearnMove[i].pp);

        r = DevLearnSlot(i);
        assert(at(r.x + 1, r.y + 1) == bg && at(r.x + r.w - 1, r.y + r.h - 1) == bg);
        assert(at(216, r.y + 5) == type && at(255, r.y + 16) == type && at(215, r.y + 10) == bg && at(256, r.y + 10) == bg);
        assert(at(216, r.y + 4) == bg && at(216, r.y + 17) == bg);
        assert(Count(130, r.y + 7, 80, 8, ink) > 0 && Count(130, r.y, 80, 7, ink) == 0 && Count(212, r.y, 4, r.h, ink) == 0);
        assert(Count(298 - pp, r.y + 7, pp, 8, ink) > 0 && Count(298 - pp, r.y, pp, 5, ink) == 0 && Count(298, r.y, 2, r.h, ink) == 0);
        assert(Count(260, r.y, 40, r.h, DEV_INK_SH) == 0 && (i == 1) == (Count(130, r.y + 15, 80, 1, DEV_INK_SH) == 0));
    }
    /* the new one: NEW on its chip 12 tall at (130, 155), its word and 2 px a side, the name 3 px after it; the rule over
     * it (kDevLearnRule) */
    r = DevLearnSlot(4); w = DevSmallTextWidth(CTR_TR("NEW", "NUEVO", "NEU")) + 4;
    assert(at(130, r.y + 5) == DEV_TAG_NEW && at(130 + w - 1, r.y + 16) == DEV_TAG_NEW && at(130 + w, r.y + 10) == DEV_PAPER);
    assert(at(130, r.y + 4) == DEV_PAPER && at(130, r.y + 17) == DEV_PAPER && Count(130, r.y + 5, w, 12, DEV_WHITE) > 0);
    assert(Count(130 + w, r.y, 3, r.h, DEV_INK) == 0 && Count(130 + w + 3, r.y + 7, 5, 8, DEV_INK) > 0);
    assert(Count(124, 146, 176, 1, DEV_RULE) == 176 && at(123, 146) == DEV_PAPER && at(300, 146) == DEV_PAPER);
    assert(Count(124, 145, 176, 1, DEV_PAPER) == 176 && Count(124, 147, 176, 1, DEV_PAPER) == 176);
    /* the footer (the grid's note, its paper 14..229 x 196..225): the cursor's move's description small in the paper's
     * ink in two lines from x 20, capitals 199 and 208; the hint for the cursor's row small in the secondary ink,
     * capitals 218 */
    assert(Count(20, 199, 204, 7, DEV_INK) > 0 && Count(20, 208, 204, 7, DEV_INK) > 0 && Count(14, 196, 216, 3, DEV_INK) == 0
           && Count(20, 207, 204, 1, DEV_INK) == 0);
    assert(Count(14, 196, 6, 30, DEV_INK) == 0 && Count(224, 196, 6, 30, DEV_INK) == 0 && Count(14, 215, 216, 11, DEV_INK) == 0);
    LearnHint(kLearnForget);
    /* the hint follows the cursor's row: an HM cannot be forgotten, the new one is skipped, another old move forgotten;
     * a held touch inverts the row under it, not the cursor's, and leaves the hint; no row, no description and no hint */
    lv.learn.row[1].locked = 1; DevCheckTouch(&lo, &lv); LearnHint(kLearnHm);
    lv.learn.pressed = PT_LEARN_HIT_ROW + 4; DevCheckTouch(&lo, &lv); LearnHint(kLearnHm);
    r = DevLearnSlot(4); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); r = DevLearnSlot(1); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
    lv.learn.pressed = -1; lv.learn.cursor = 4; DevCheckTouch(&lo, &lv); LearnHint(kLearnSkip);
    lv.learn.cursor = 0; DevCheckTouch(&lo, &lv); LearnHint(kLearnForget);
    lv.learn.cursor = 3; lv.learn.row[3].locked = 1; DevCheckTouch(&lo, &lv); LearnHint(kLearnHm);
    lv.learn.cursor = -1; DevCheckTouch(&lo, &lv); LearnHint(NULL);
    assert(Count(14, 196, 216, 30, DEV_INK) == 0 && Count(20, 140, 92, 38, DEV_INK2) == 0);
    other = lv; other.learn.row[1].locked ^= 1; other.learn.row[3].locked ^= 1;
    assert(DevCheckTouch(&lo, &other).count == 0);                  /* no row: no lock drawn */
    /* the hint is the footer's last line: with a descender on every small glyph (the game's g, p, q, y and comma have
     * theirs in glyph row 11: SmallDescenders) it ends on the paper's last row, 225, and none reaches the bezel under
     * it; the description's second line keeps its own off the hint. With an accent on every glyph (Accents) the
     * description's first line keeps its own on the paper, none on the bezel over it */
    lv = LearnBoard(); DevCheckTouch(&lo, &lv);
    SmallDescenders(1); CtrPokeTouch_Draw(canvas, &lv);
    assert(Count(20, 225, 204, 1, DEV_INK2) > 0 && Count(14, 226, 216, 5, DEV_INK2) == 0 && Count(14, 226, 216, 5, DEV_INK) == 0);
    assert(Count(20, 215, 204, 1, DEV_INK) > 0 && Count(14, 216, 216, 10, DEV_INK) == 0);
    SmallDescenders(0);
    Accents(1); CtrPokeTouch_Draw(canvas, &lv); assert(BezelOver(14, 216)); Accents(0);
    CtrPokeTouch_Draw(canvas, &lo);
    /* the cursor's move's numbers follow it; another move's do not show */
    other = lv; strcpy(other.learn.row[1].power, "1000"); TouchSamePixels(&lv, &other, 0, "the cursor's move's power");
    other = lv; strcpy(other.learn.row[1].accuracy, "1000"); TouchSamePixels(&lv, &other, 0, "the cursor's move's accuracy");
    other = lv; strcpy(other.learn.row[0].power, "1000"); TouchSamePixels(&lv, &other, 1, "another move's power");
    other = lv; other.learn.row[0].locked = 1; TouchSamePixels(&lv, &other, 1, "another move's HM lock");
    other = lv; other.learn.row[1].locked = 1; TouchSamePixels(&lv, &other, 0, "the cursor's move's HM lock");
    CtrPokeTouch_Draw(canvas, &lo);
    /* a long new move keeps every letter: a name the normal font does not fit before 212 (capitals 7 px wide here) in the
     * small one, capitals r.y + 8; FireRed's widest names (12 glyphs, 60 px small, as SWORDS DANCE) whole after NEW in
     * every language, the type's chip moved right only as far as the name needs (after NUEVO: to 226), clear of the
     * widest PP; nothing cut */
    {
        static uint8_t keepWidths[256];
        int pp = DEV_LEARN_PP_RIGHT - DevTextWidth("40/40"), chip;

        x = 130 + DevSmallTextWidth(CTR_TR("NEW", "NUEVO", "NEU")) + 4 + 3; r = DevLearnSlot(4);
        lv = LearnBoard(); strcpy(lv.learn.row[4].name, "SMOKESCREEN"); DevCheckTouch(&lo, &lv);
        memcpy(keepWidths, fontWidths, sizeof(keepWidths));
        for (unsigned c = 0xbb; c <= 0xd4; ++c)         /* A..Z */
            fontWidths[c] = 7;
        memset(sPtFontReady, 0, sizeof(sPtFontReady));
        CtrPokeTouch_Draw(canvas, &lv);
        w = DevSmallTextWidth("SMOKESCREEN"); chip = x + w + 4 > 216 ? x + w + 4 : 216;
        assert(Count(x, r.y + 7, w, 1, DEV_INK) == 0 && Count(x, r.y + 8, w, 7, DEV_INK) > 0 && Count(x + w - 5, r.y + 8, 5, 7, DEV_INK) > 0);
        assert(Count(x + w, r.y, chip - x - w, r.h, DEV_INK) == 0 && Count(x, r.y + 15, w, 1, DEV_INK_SH) == 0);
        assert(at(chip, r.y + 5) == UiTypeColor(0) && at(chip - 1, r.y + 10) == DEV_PAPER && sDevKeyCut == 0);
        memcpy(fontWidths, keepWidths, sizeof(keepWidths));
        memset(sPtFontReady, 0, sizeof(sPtFontReady));
        CtrPokeTouch_Draw(canvas, &lo);
        strcpy(lv.learn.row[4].name, "SWORDS DANCE"); DevCheckTouch(&lo, &lv);
        w = DevTextWidth("SWORDS DANCE") <= 212 - x ? DevTextWidth("SWORDS DANCE") : DevSmallTextWidth("SWORDS DANCE");
        chip = x + w + 4 > 216 ? x + w + 4 : 216;
        assert(w >= 60 && chip <= 226 && chip + 40 + 2 <= pp && sDevKeyCut == 0);
        assert(Count(x + w - 5, r.y + 7, 5, 9, DEV_INK) > 0 && Count(x + w, r.y, chip - x - w, r.h, DEV_INK) == 0);
        assert(at(chip, r.y + 5) == UiTypeColor(0) && at(chip + 39, r.y + 16) == UiTypeColor(0) && at(chip - 1, r.y + 10) == DEV_PAPER);
        assert(at(chip + 40, r.y + 10) == DEV_PAPER && Count(chip + 40, r.y, 2, r.h, DEV_INK) == 0);
        shot("long");
        /* one longer than any FireRed name (15 glyphs): cut before the chip at 226, and counted once a draw */
        strcpy(lv.learn.row[4].name, "PORTWRITTENWORD"); sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &lv);
        assert(sDevKeyCut == 1 && at(226, r.y + 5) == UiTypeColor(0) && Count(222, r.y, 4, r.h, DEV_INK) == 0);
        assert(Count(x, r.y + 8, 222 - x, 7, DEV_INK) > 0);
        sDevKeyCut = 0;
        lv = LearnBoard(); DevCheckTouch(&lo, &lv);
    }

    /* BACK: dark in the exit corner, sunk under a touch, grey while it cannot be used, gone while not shown */
    lv = LearnBoard(); DevCheckTouch(&lo, &lv);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 43, DEV_LED) == 0 && Count(244, 190, 68, 40, DEV_DARK_INK) > 0);
    lv.backPressed = 1; DevCheckTouch(&lo, &lv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    lv.backPressed = 0; lv.backEnabled = 0; DevCheckTouch(&lo, &lv); assert(at(247, 210) == DIM_DARK);
    lv.backEnabled = 1; lv.backVisible = 0; DevCheckTouch(&lo, &lv); assert(at(247, 210) == DEV_SHELL_RED && !BackV(&lv, 247, 210));
    lv.backVisible = 1; DevCheckTouch(&lo, &lv);

    /* what the move to forget does not draw presents nothing; the shell's colour whole; the ways in and out (HOME; the
     * Summary's own picture before it is ready: in the frame's screen, nothing of the move to forget hit) */
    strcpy(lv.bar[0], "Anything"); lv.enabled[PT_BAG] = 1; lv.playerX = 3;
    assert(DevCheckTouch(&lo, &lv).count == 0);
    lv.shell = 2; assert(DevWhole(DevCheckTouch(&lo, &lv)) && at(1, 100) == DEV_SHELL_GREEN);
    lv.shell = 0; DevCheckTouch(&lo, &lv);
    lv.learn.nameDefault = 0; assert(DevWhole(DevCheckTouch(&lo, &lv)));
    other = lv; other.learn.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&lo, &other)));
    lv = LearnBoard(); assert(DevWhole(DevCheckTouch(&lo, &lv)));
    other = lv; memset(&other.learn, 0, sizeof(other.learn)); other.viewport = 1;
    assert(DevWhole(DevCheckTouch(&lo, &other)) && DrawnBy(&other, DevFrameShown) && CtrPokeTouch_LearnHit(&other, 130, 60) == -1);
    lv.viewport = 1; assert(DrawnBy(&lv, DevFrameShown) && CtrPokeTouch_LearnHit(&lv, 130, 60) == -1);
    lv = LearnBoard(); assert(DevWhole(DevCheckTouch(&lo, &lv)));
    LearnUpperCase(LearnBoard());
    CtrPokeTouch_Draw(canvas, &lo);

    /* The states for the eye, each through Update first: the board (SCRATCH), the new one, two moves and the new one. */
    lv = LearnBoard(); lv.learn.cursor = 0; DevCheckTouch(&lo, &lv);
    lv.learn.cursor = 1; DevCheckTouch(&lo, &lv); shot("move");
    lv.learn.cursor = 4; DevCheckTouch(&lo, &lv); shot("new");
    lv.learn.row[2].move = lv.learn.row[3].move = 0; lv.learn.row[2].name[0] = lv.learn.row[3].name[0] = 0;
    DevCheckTouch(&lo, &lv); shot("two");
    picArt = 0;
    ForgetPictures();
    assert(sDevKeyCut == 0);                      /* main prints PASS device learn */
}
