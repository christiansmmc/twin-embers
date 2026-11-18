/* ---- learn: the move to forget for a new one as the red device, through the field dispatcher (no board; plan 3
 * Task 4) ---------------------------------------------------------------------------------------------------------
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

/* The papers it writes on: the Bag's title, the Summary's note (each screen's paper is 6 px in). */
static const CtrPokeTouchRect kLearnTitlePaper = {14, 12, 292, 16}, kSumNotePaper = {14, 194, 216, 32};

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
 * case: nameDefault), its level the number alone (recipe 0222's CtrBottomLearn_Name), port-written stand-ins for the question and the description; BACK shown and usable. */
static CtrPokeTouchView LearnBoard(void)
{
    CtrPokeTouchView v;
    CtrPokeTouchLearn *l = &v.learn;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    l->on = 1;
    l->cursor = 4;
    l->pressed = -1;
    l->species = 6;
    l->nameDefault = 1;
    strcpy(l->nickname, "Charizard");
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
    lv = LearnBoard(); DevCheckTouch(&lo, &lv); assert(DrawnBy(&lv, DevLearnShown));
    /* learn */ for (i = 0; i < 5; ++i) AssertRowHits(LearnHitV, &lv, DevSumRow(i), PT_LEARN_HIT_ROW + i);
    assert(at(DevSumRow(4).x + 1, DevSumRow(4).y + 11) == DEV_BEZEL); AssertHits(BackV, &lv, kDevSumBack, 1);   /* before the name */
    lv.learn.row[2].move = lv.learn.row[3].move = 0; DevCheckTouch(&lo, &lv); assert(CtrPokeTouch_LearnHit(&lv, 130, DevSumRow(2).y + 5) == -1);
    lv.learn.row[0].locked ^= 1; lv.mapGroup ^= 1; lv.speed ^= 1; assert(DevCheckTouch(&lo, &lv).count == 0);

    /* the title: the question in the paper's ink; the column (the paper's second tone): the name, the level small in the
     * secondary ink with the Summary's prefix ("Lv36": the fill gives the number), the picture in the Summary's place, the
     * cursor's move's POWER and ACCURACY at its foot */
    lv = LearnBoard(); DevCheckTouch(&lo, &lv);
    assert(CountIn(kLearnTitlePaper, DEV_INK) > 0);
    assert(at(15, 47) == DEV_PAPER2 && at(116, 172) == DEV_PAPER2);
    assert(Count(20, 49, 92, 12, DEV_INK) > 0 && Count(20, 63, 92, 10, DEV_INK2) > 0 && Count(20, 63, 92, 10, DEV_INK) == 0);
    {
        char level[16];
        int bare = DevSmallTextWidth(lv.learn.level), full;

        snprintf(level, sizeof(level), CTR_TR("Lv%s", "Nv%s", "Lv%s"), lv.learn.level);
        full = DevSmallTextWidth(level);
        assert(full > bare && Count(20 + bare, 63, full - bare, 10, DEV_INK2) > 0 && Count(20 + full, 63, 92 - full, 10, DEV_INK2) == 0);
    }
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

    /* The states for the eye, each through Update first. */
    lv = LearnBoard(); lv.learn.cursor = 1; DevCheckTouch(&lo, &lv); shot("move");
    lv.learn.cursor = 4; DevCheckTouch(&lo, &lv); shot("new");
    lv.learn.row[2].move = lv.learn.row[3].move = 0; lv.learn.row[2].name[0] = lv.learn.row[3].name[0] = 0;
    DevCheckTouch(&lo, &lv); shot("two");
    picArt = 0;
    ForgetPictures();
    assert(sDevKeyCut == 0);                      /* main prints PASS device learn */
}
