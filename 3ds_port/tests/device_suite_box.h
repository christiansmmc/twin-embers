/* ---- box: the PC box as the red device (def_box; owner decision O3), through the field dispatcher --------------------
 * Included by device_render_test.c before its kSuites (after the naming suite, whose NameCodes it uses). Its art is the
 * harness's: the Pokemon's icons (monArt, STUB_ICON) and the marks (markArt). */

/* The papers it draws on (each screen's paper is 6 px in): the box's (plain paper, owner decision O3), the footer's (the
 * grid's note), the hand's and the count's. */
static const CtrPokeTouchRect kBoxPaper = {14, 14, 160, 164}, kBoxNotePaper = {14, 196, 216, 30};
static const CtrPokeTouchRect kBoxHandPaper = {194, 46, 112, 48}, kBoxCountPaper = {194, 162, 112, 16};
#define BOX_DIM(c) DevBlend(DEV_SHELL_RED, (c), DEV_A_DIM)   /* a pixel under DevDim, on the red shell */
#define BOX_ROCK 5                                           /* FireRed's TYPE_ROCK and TYPE_GROUND: the board's ONIX */
#define BOX_GROUND 4
#define BOX_ITEM CTR_TR("ITEM ", "OBJETO ", "ITEM ")           /* the footer's ITEM and a space before FireRed's item */

static int BoxHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_BoxHit((const CtrPokeTouchView *)view, x, y);
}

/* `ascii` as FireRed's codes padded with spaces to `pad` characters, as StringCopyPadded leaves them (NameCodes). */
static void BoxCodes(uint8_t *out, int size, const char *ascii, int pad)
{
    int n = (int)strlen(ascii);

    NameCodes(out, size, ascii);
    for (int i = n; i < pad && i < size - 1; ++i)
        out[i] = 0x00;
}

/* The cursor's Pokemon's lines as FireRed's box writes them (SetDisplayMonData): the nickname and "/" with the species
 * padded to 5, the gender sign (0xb5 male, 0xb6 female, 0x00 none) and FireRed's Lv with the level after their colour
 * codes, the item padded to 8 ("" none: 8 spaces). */
static void BoxMonText(CtrPokeTouchBox *b, const char *nick, const char *species, uint8_t sign, int level, const char *item)
{
    uint8_t *g = b->monText[2];
    int n = 0;

    BoxCodes(b->monText[0], (int)sizeof(b->monText[0]), nick, 5);
    b->monText[1][0] = 0xba;                                      /* "/" */
    BoxCodes(b->monText[1] + 1, (int)sizeof(b->monText[1]) - 1, species, 5);
    memset(g, 0xff, sizeof(b->monText[2]));
    g[n++] = 0xfc; g[n++] = 0x04; g[n++] = 1; g[n++] = 2; g[n++] = 3; g[n++] = sign;
    g[n++] = 0xfc; g[n++] = 0x04; g[n++] = 2; g[n++] = 1; g[n++] = 3; g[n++] = 0x00; g[n++] = 0xf9; g[n++] = 0x05;
    if (level >= 10)
        g[n++] = (uint8_t)(0xa1 + level / 10);
    g[n++] = (uint8_t)(0xa1 + level % 10);
    g[n++] = 0x00;
    BoxCodes(b->monText[3], (int)sizeof(b->monText[3]), item, 8);
}

/* The pointed Pokemon's types as BoxSnapshot fills them (CtrBottomDex_Types): -1 none. */
static void BoxTypes(CtrPokeTouchBox *b, int first, const char *firstName, int second, const char *secondName)
{
    b->monType[0] = (int8_t)first;
    b->monType[1] = (int8_t)second;
    snprintf(b->monTypeName[0], sizeof(b->monTypeName[0]), "%s", firstName);
    snprintf(b->monTypeName[1], sizeof(b->monTypeName[1]), "%s", secondName);
}

/* The box as 3ds_bottom.c gives it (CtrBottomStorage_BoxFill, BoxSnapshot): a full box of thirty, the cursor on its
 * last place, a Pokemon in hand, a full Party, the cursor's Pokemon's lines (a nickname that is its species', a male of
 * level 12, no item: eight spaces) and its types (ROCK, GROUND), no message, nothing held; port-written names. The
 * view's BACK is asked for (BACK of the storage modes) but the box draws none. */
static CtrPokeTouchView BoxBoard(void)
{
    static const char *const kParty[6] = {"Kitmon", "Pebble", "Moss", "Ember", "Dew", "Gale"};
    CtrPokeTouchView v;
    CtrPokeTouchBox *b = &v.box;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    b->on = 1;
    b->state = PT_BOX_STATE_BOX;
    b->area = ST_AREA_BOX;
    b->pos = 29;
    b->pressed = -1;
    b->wallpaper = 3;
    BoxCodes(b->boxName, (int)sizeof(b->boxName), "Box one", 0);
    for (int i = 0; i < PT_BOX_SLOTS; ++i)
        b->slot[i].species = (uint16_t)(1 + i);
    for (int i = 0; i < 6; ++i) {
        b->party[i].species = (uint16_t)(40 + i);
        BoxCodes(b->partyName[i], (int)sizeof(b->partyName[i]), kParty[i], 0);
        b->partyLevel[i] = (uint8_t)(5 + 7 * i);
    }
    b->holding = 1;
    b->held.species = 33;
    b->mon = b->slot[29];
    BoxMonText(b, "Kitmon", "Kitmon", 0xb5, 12, "");
    BoxTypes(b, BOX_ROCK, "ROCK", BOX_GROUND, "GROUND");
    memset(b->message, 0xff, sizeof(b->message));
    snprintf(v.bar[0], PT_BAR_LEN, "Box 01");                  /* the bar: not drawn by the box */
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* The box's menu as FireRed gives it: n rows of FireRed's Pokemon menu (MOVE SUMMARY WITHDRAW MARK RELEASE), the last
 * CANCEL (recipe 0268: menuCancel). */
static void BoxMenu(CtrPokeTouchView *v, int n)
{
    static const char *const kRows[6] = {"Move", "Summary", "Withdraw", "Mark", "Release", "Cancel"};

    v->box.state = PT_BOX_STATE_MENU;
    v->box.menuCount = (uint8_t)n;
    v->box.menuCancel = 1;
    memset(v->box.menu, 0xff, sizeof(v->box.menu));
    for (int i = 0; i < n; ++i)
        NameCodes(v->box.menu[i], (int)sizeof(v->box.menu[i]), kRows[i == n - 1 ? 5 : i]);
}

/* A wallpaper menu as FireRed gives it from the box's name (WALLPAPER): SCENERY 1-3 and ETCETERA, no CANCEL row (its B
 * leaves it), FireRed's cursor on the box's name. */
static void BoxWallMenu(CtrPokeTouchView *v)
{
    static const char *const kRows[4] = {"Scenery 1", "Scenery 2", "Scenery 3", "Etcetera"};

    v->box.state = PT_BOX_STATE_MENU;
    v->box.area = ST_AREA_TITLE;
    v->box.menuCount = 4;
    v->box.menuCancel = 0;
    memset(v->box.menu, 0xff, sizeof(v->box.menu));
    for (int i = 0; i < 4; ++i)
        NameCodes(v->box.menu[i], (int)sizeof(v->box.menu[i]), kRows[i]);
}

/* YES / NO as FireRed gives it, its cursor on `cursor`. */
static void BoxYesNo(CtrPokeTouchView *v, int cursor)
{
    v->box.state = PT_BOX_STATE_YESNO;
    v->box.menuCount = 2;
    v->box.menuCursor = (int8_t)cursor;
}

/* The faded colour of the harness's icon over `under` (DevBoxIcon's fade: half strength over what is under it). */
#define BOX_FADED(under) DevBlend((under), STUB_ICON, 128)

/* Key i of n of a menu or the marks: the right column's (DevBoxMenuKey), or (`exit`: a menu's CANCEL, the marks') the
 * last in the exit corner. */
static CtrPokeTouchRect BoxKey(int n, int i, int exit)
{
    return exit && i == n - 1 ? kDevGridExit : DevBoxMenuKey(i);
}

/* The n keys up but `sunk` (-1 none); with `exit` the last dark in the exit corner. */
static void BoxKeysSunk(int n, int sunk, int exit)
{
    for (int i = 0; i < n; ++i)
        AssertKey(BoxKey(n, i, exit), exit && i == n - 1 ? DEV_LOOK_DARK : DEV_LOOK_KEY, i == sunk);
}

/* The n keys hit where they are drawn, each its menu code. */
static void BoxKeysHit(const CtrPokeTouchView *v, int n, int exit)
{
    for (int i = 0; i < n; ++i)
        AssertHits(BoxHitV, v, BoxKey(n, i, exit), ST_HIT_MENU_BASE + i);
}

/* Key r of the view drawn with `word` (the kit's key drawn again over it, the word from `inset` px in as the drawer puts
 * it, leaves its face as it was). */
static void BoxKeyWord(const CtrPokeTouchView *v, CtrPokeTouchRect r, int look, int sunk, const char *word, int inset)
{
    static uint16_t drawn[W * H];
    int faceY, same = 1;

    CtrPokeTouch_Draw(canvas, v);
    memcpy(drawn, canvas, sizeof(drawn));
    faceY = DevKey(&r, look, sunk, 6);
    DevKeyWordAt(&r, faceY, look, sunk, word, inset);
    for (int x = r.x + 6; x < r.x + r.w - 6; ++x)
        for (int y = faceY + 2; y < faceY + r.h - 2; ++y)
            same &= drawn[x * H + H - 1 - y] == at(x, y);
    if (!same) {
        fprintf(stderr, "box: key {%d,%d,%d,%d} does not read \"%s\"\n", r.x, r.y, r.w, r.h, word);
        assert(0);
    }
    memcpy(canvas, drawn, sizeof(drawn));
}

/* A text the last whole draw told the kit of: from x with its capitals at y, small or not, in `ink`, the bytes it drew
 * `s`. */
static int BoxText(int x, int y, int small, uint16_t ink, const char *s)
{
    for (int i = 0; i < gTraceCount; ++i)
        if (gTrace[i].op == DEV_TRACE_TEXT && gTrace[i].x == x && gTrace[i].y == y && gTrace[i].b == small
            && gTrace[i].a == ink && !strcmp(gTrace[i].s, s))
            return 1;
    return 0;
}

/* Upper case (decision 5): FireRed's menu rows and item in capitals; the box's name, a nickname and the chosen box's
 * name as typed (a nickname that is its species' own in capitals); the port's PARTY POKéMON and CLOSE as the board
 * writes them. */
static void BoxWords(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    BoxCodes(cased.box.boxName, (int)sizeof(cased.box.boxName), "BOX ONE", 0);
    TouchSamePixels(&cased, &v, 0, "the box's name, as typed");
    cased = v; BoxMonText(&cased.box, "KITMON", "Kitmon", 0xb5, 12, "");
    TouchSamePixels(&cased, &v, 1, "a nickname that is its species', in capitals");
    BoxMonText(&v.box, "Kit", "Kitmon", 0xb5, 12, "Leaf charm"); cased = v;
    BoxMonText(&cased.box, "KIT", "Kitmon", 0xb5, 12, "Leaf charm");
    TouchSamePixels(&cased, &v, 0, "a nickname, as typed");
    BoxMonText(&cased.box, "Kit", "KITMON", 0xb5, 12, "LEAF CHARM");
    TouchSamePixels(&cased, &v, 1, "the item, in capitals");
    v.box.area = ST_AREA_PARTY; v.box.pos = 1; cased = v;
    BoxCodes(cased.box.partyName[3], (int)sizeof(cased.box.partyName[3]), "EMBER", 0);
    TouchSamePixels(&cased, &v, 0, "a Party nickname, as typed");
    v = BoxBoard(); BoxMenu(&v, 5); cased = v;
    for (int i = 0; i < 5; ++i)
        for (int k = 0; k < 16 && cased.box.menu[i][k] != 0xff; ++k)
            if (cased.box.menu[i][k] >= 0xd5 && cased.box.menu[i][k] <= 0xee)
                cased.box.menu[i][k] = (uint8_t)(cased.box.menu[i][k] - 0x1a);
    TouchSamePixels(&cased, &v, 1, "the menu's rows, in capitals");
    v = BoxBoard(); v.box.state = PT_BOX_STATE_CHOOSE; BoxCodes(v.box.chooseName, (int)sizeof(v.box.chooseName), "Box two", 0);
    cased = v; BoxCodes(cased.box.chooseName, (int)sizeof(cased.box.chooseName), "BOX TWO", 0);
    TouchSamePixels(&cased, &v, 0, "the chosen box's name, as typed");
    v = BoxBoard(); v.box.area = ST_AREA_BUTTONS; v.box.pos = 0;
    BoxKeyWord(&v, kDevBoxParty, DEV_LOOK_KEY, 1, CTR_TR("PARTY POK\xC3\xA9MON", "EQUIPO POK\xC3\xA9MON", "POK\xC3\xA9MON-TEAM"), 0);
    BoxKeyWord(&v, kDevGridExit, DEV_LOOK_DARK, 0, DevCloseWord(), 0);
}

/* The plan's suite (a full box and an empty one with every place hit where it is drawn, the cursor's frame round place
 * 29's square, an empty place's square, the footer's Pokemon with its types and item, the count, IN HAND and the
 * Pokemon in hand; the Party, FireRed's menu of six, YES / NO, the chooser, the marks, a message), then each state's
 * pieces, the words, the ways in and out and the states for the eye. */
static void SuiteBox(void)
{
    CtrPokeTouchView v = BoxBoard(), old = v, other, m6, cv, wv, yv, kv;
    CtrPokeTouchRect r;
    int i, cuts;
    char word[16];

    monArt = 1;
    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v.box.holding = 0; v.box.slot[28].species = 0; DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevBoxShown));
    for (i = 0; i < PT_BOX_SLOTS; ++i) AssertRowHits(BoxHitV, &v, DevBoxCell(i), i);
    AssertRowHits(BoxHitV, &v, kDevBoxTitle, ST_HIT_TITLE); AssertHits(BoxHitV, &v, kDevBoxPrev, ST_HIT_PREV); AssertHits(BoxHitV, &v, kDevBoxNext, ST_HIT_NEXT);
    AssertHits(BoxHitV, &v, kDevBoxParty, ST_HIT_BUTTON_PARTY); AssertHits(BoxHitV, &v, kDevGridExit, ST_HIT_BUTTON_CLOSE);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(BoxHitV(&v, 250, 170) == ST_HIT_NONE && BoxHitV(&v, 250, 70) == ST_HIT_NONE);
    r = DevBoxCell(29); assert(at(r.x - 1, r.y + 12) == DEV_BEZEL && at(r.x + r.w, r.y + 12) == DEV_BEZEL && at(r.x + 1, r.y + 12) != DEV_BEZEL);
    r = DevBoxCell(28); assert(at(r.x, r.y + 12) == DEV_PAPER2 && at(r.x + 1, r.y + 12) == DEV_PAPER);    /* an empty place's square */
    assert(Count(20, 199, 120, 9, DEV_INK) > 0 && Count(20, 212, 40, 10, UiTypeColor(v.box.monType[0])) > 0 && Count(110, 214, 100, 7, DEV_INK2) > 0);
    assert(Count(194, 165, 112, 7, DEV_INK2) > 0 && Count(196, 50, 40, 7, DEV_INK2) > 0);
    assert(CountIn(kBoxHandPaper, STUB_ICON) == 0 && Count(232, 72, 70, 7, DEV_INK2) == 0);   /* an empty hand: IN HAND only */
    v.box.holding = 1; DevCheckTouch(&old, &v); assert(Count(194, 46, 112, 48, STUB_ICON) > 0 && Count(232, 72, 70, 7, DEV_INK2) > 0);
    v.box.monType[1] ^= 1; assert(DevWhole(DevCheckTouch(&old, &v))); v.box.wallpaper ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
    assert(!CtrPokeTouch_BackHit(&v, 278, 210));
    memset(v.box.slot, 0, sizeof(v.box.slot)); v.box.holding = 0; DevCheckTouch(&old, &v); for (i = 0; i < 30; ++i) AssertRowHits(BoxHitV, &v, DevBoxCell(i), i);
    v.box.area = ST_AREA_PARTY; v.box.pos = 2; DevCheckTouch(&old, &v); for (i = 0; i < 7; ++i) AssertRowHits(BoxHitV, &v, DevBoxPartyRow(i), ST_HIT_PARTY_BASE + i);
    m6 = BoxBoard(); BoxMenu(&m6, 6); DevCheckTouch(&old, &m6);
    /* menu6, menuCancel 1, cursor on MOVE */ for (i = 0; i < 5; ++i) AssertHits(BoxHitV, &m6, DevBoxMenuKey(i), ST_HIT_MENU_BASE + i);
    AssertKey(DevBoxMenuKey(0), DEV_LOOK_KEY, 1); AssertHits(BoxHitV, &m6, kDevGridExit, ST_HIT_MENU_BASE + 5);
    assert(Count(14, 196, 216, 30, DEV_INK) > 0 && BoxHitV(&m6, 60, 60) == ST_HIT_NONE); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(!BackV(&m6, 278, 209)); m6.box.menuCursor = 5; DevCheckTouch(&old, &m6); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    r = DevBoxCell(m6.box.pos); assert(Count(r.x - 4, r.y - 6, 32, 32, STUB_ICON) > 0);                         /* the cursor's icon whole */
    r = DevBoxCell(0); assert(Count(r.x, r.y, 24, 24, STUB_ICON) == 0 && Count(r.x, r.y, 24, 24, DevBlend(DEV_PAPER, STUB_ICON, 128)) > 0);
    /* wv, a wallpaper menu: SCENERY 1-3, ETCETERA, menuCancel 0 */ wv = BoxBoard(); BoxWallMenu(&wv); DevCheckTouch(&old, &wv);
    for (i = 0; i < 4; ++i) AssertHits(BoxHitV, &wv, DevBoxMenuKey(i), ST_HIT_MENU_BASE + i);
    AssertHits(BackV, &wv, kDevGridExit, 1); AssertKey(DevBoxMenuKey(3), DEV_LOOK_KEY, 0);
    assert(BoxHitV(&wv, kDevGridExit.x + 5, kDevGridExit.y + 5) == ST_HIT_NONE && BoxHitV(&wv, 250, 160) == ST_HIT_NONE);
    /* YES / NO */ yv = BoxBoard(); BoxYesNo(&yv, 0); DevCheckTouch(&old, &yv);
    AssertHits(BoxHitV, &yv, kDevBoxYes, ST_HIT_MENU_BASE); AssertHits(BoxHitV, &yv, kDevBoxNo, ST_HIT_MENU_BASE + 1); AssertHits(BackV, &yv, kDevGridExit, 1);
    assert(BoxHitV(&yv, kDevGridExit.x + 5, kDevGridExit.y + 5) == ST_HIT_NONE);
    /* marks */ kv = BoxBoard(); kv.box.state = PT_BOX_STATE_MARKS; kv.box.menuCount = 6; DevCheckTouch(&old, &kv);
    for (i = 0; i < 5; ++i) AssertHits(BoxHitV, &kv, DevBoxMenuKey(i), ST_HIT_MENU_BASE + i);
    AssertHits(BoxHitV, &kv, kDevGridExit, ST_HIT_MENU_BASE + 5); assert(!BackV(&kv, 278, 209));
    /* menuCancel drawn: six rows without CANCEL lose the sixth (not hit), counted (the column holds five) */
    cuts = sDevKeyCut; m6.box.menuCancel ^= 1; assert(DevWhole(DevCheckTouch(&old, &m6)));
    AssertHits(BackV, &m6, kDevGridExit, 1); assert(BoxHitV(&m6, kDevGridExit.x + 5, kDevGridExit.y + 5) == ST_HIT_NONE);
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &m6); assert(sDevKeyCut == 1); sDevKeyCut = cuts;
    /* a held BACK redraws */ DevCheckTouch(&old, &wv); wv.backPressed = 1; assert(DevWhole(DevCheckTouch(&old, &wv))); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    DevCheckTouch(&old, &yv); yv.backPressed = 1; assert(DevWhole(DevCheckTouch(&old, &yv))); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    cv = BoxBoard(); cv.box.state = PT_BOX_STATE_CHOOSE; DevCheckTouch(&old, &cv);
    /* chooser */ AssertHits(BoxHitV, &cv, kDevBoxChoose, ST_HIT_TITLE); AssertHits(BackV, &cv, kDevGridExit, 1);
    AssertHits(BoxHitV, &cv, kDevBoxNext, ST_HIT_NEXT);
    v.box.state = PT_BOX_STATE_MESSAGE; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BoxHit(&v, 5, 5) == PT_BOX_HIT_MESSAGE);
    v.box.state = PT_BOX_STATE_BOX; DevCheckTouch(&old, &v);
    v.box.box ^= 1; v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* the box on plain paper (owner decision O3): the second paper, each place's square, the cursor's frame (2 px round
     * its square, over the icons) on its place alone; the icons clipped to the paper; the wallpaper not drawn */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    r = DevBoxCell(29);
    assert(at(r.x - 1, r.y - 1) == DEV_BEZEL && at(r.x + r.w, r.y + r.h) == DEV_BEZEL && at(r.x + 12, r.y) == DEV_BEZEL);
    assert(at(r.x + 12, r.y + r.h - 1) == DEV_BEZEL && at(r.x + 12, r.y + r.h + 1) == DEV_PAPER2);
    assert(at(r.x, r.y + 12) == DEV_BEZEL && at(r.x + 1, r.y + 12) == STUB_ICON && at(r.x + 22, r.y + 12) == STUB_ICON);
    r = DevBoxCell(28); assert(at(r.x, r.y + 12) == STUB_ICON && Count(r.x, r.y - 1, r.w - 1, r.h + 2, DEV_BEZEL) == 0);
    v.box.pos = 0; DevCheckTouch(&old, &v); assert(at(DevBoxCell(0).x, DevBoxCell(0).y + 12) == DEV_BEZEL);
    assert(Count(DevBoxCell(29).x, DevBoxCell(29).y, 24, 24, DEV_BEZEL) == 0);
    monArt = 2; v.box.pos = 3; DevCheckTouch(&old, &v);       /* icons that fill their 32x32: none past the paper */
    assert(at(DevBoxCell(24).x - 4, 155) == STUB_ICON && at(DevBoxCell(5).x + 27, DevBoxCell(5).y - 6) == STUB_ICON);
    assert(Count(8, 8, 172, 6, STUB_ICON) == 0 && Count(8, 178, 172, 7, STUB_ICON) == 0 && Count(8, 8, 6, 176, STUB_ICON) == 0);
    assert(Count(188, 40, 124, 6, STUB_ICON) == 0 && Count(188, 94, 124, 7, STUB_ICON) == 0 && CountIn(kBoxHandPaper, STUB_ICON) == 32 * 32);
    monArt = 1;
    memset(v.box.slot, 0, sizeof(v.box.slot)); DevCheckTouch(&old, &v);
    r = DevBoxCell(20);
    assert(at(r.x, r.y) == DEV_PAPER2 && at(r.x + 1, r.y + 1) == DEV_PAPER && at(r.x + 22, r.y + 22) == DEV_PAPER && at(r.x + 23, r.y + 23) == DEV_PAPER2);
    assert(at(15, 170) == DEV_PAPER2 && at(173, 177) == DEV_PAPER2 && CountIn(kBoxPaper, STUB_ICON) == 0);
    assert(CountIn(kBoxPaper, DEV_PAPER) == 30 * 22 * 22);
    v.box.wallpaper = 9; assert(DevCheckTouch(&old, &v).count == 0);     /* no wallpaper on the lower screen */
    /* the band: the name centred on it, capitals 19 (DEV_INK over its shadow); inverted while chosen */
    v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v);
    assert(at(14, 14) == DEV_PAPER2 && Count(14, 19, 160, 1, DEV_INK) > 0 && Count(14, 14, 160, 5, DEV_INK) == 0 && Count(14, 14, 160, 20, DEV_INK_SH) > 0);
    {
        int left = 0, right = 0;

        for (int x = 14; x < 174; ++x)
            if (Count(x, 14, 1, 20, DEV_INK)) { if (!left) left = x; right = x; }
        assert(left && abs((left - 14) - (173 - (right + 1))) <= 1);   /* the stub glyph's last column is blank */
    }
    v.box.area = ST_AREA_TITLE; DevCheckTouch(&old, &v);
    assert(at(14, 14) == DEV_BEZEL && at(173, 33) == DEV_BEZEL && at(100, 34) != DEV_BEZEL && Count(14, 14, 160, 20, DEV_PAPER) > 0);
    v.box.area = ST_AREA_BOX; v.box.pos = 5; DevCheckTouch(&old, &v);   /* (place 5's frame takes the band's last row) */
    assert(at(14, 14) == DEV_PAPER2 && Count(14, 14, 160, 20, DEV_INK) > 0 && Count(14, 14, 160, 19, DEV_BEZEL) == 0);

    /* the right column: ◀ ▶ sunk while held; PARTY POKéMON (cream) and CLOSE (dark, in the exit corner) sunk by FireRed's
     * cursor on its buttons or a held touch, which takes the cursor's marks (the frame, the band, a key) away from the
     * rest of the box (the dark CLOSE's face is the same up and sunk: AssertKey tells them apart by its LED) */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0); AssertKey(kDevBoxNext, DEV_LOOK_KEY, 0); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    v.box.pos = 1; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    v.box.pressed = ST_HIT_BUTTON_PARTY; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    v.box.pressed = ST_HIT_PREV; DevCheckTouch(&old, &v); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    v.box.pressed = ST_HIT_NEXT; DevCheckTouch(&old, &v); AssertKey(kDevBoxNext, DEV_LOOK_KEY, 1); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0);
    v.box.area = ST_AREA_BOX; v.box.pos = 29; v.box.pressed = 4; DevCheckTouch(&old, &v);   /* a held place takes the frame */
    assert(at(DevBoxCell(4).x, DevBoxCell(4).y + 12) == DEV_BEZEL && at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == STUB_ICON);
    v.box.pressed = ST_HIT_TITLE; DevCheckTouch(&old, &v);
    assert(at(14, 14) == DEV_BEZEL && Count(DevBoxCell(29).x, DevBoxCell(29).y, 24, 24, DEV_BEZEL) == 0);
    v.box.pressed = ST_HIT_BUTTON_CLOSE; v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v);
    AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    v.box.pressed = PT_BOX_HIT_MESSAGE; assert(DevCheckTouch(&old, &v).count != 0); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 1);
    v.box.pressed = ST_HIT_MENU_BASE; assert(DevCheckTouch(&old, &v).count == 0);   /* nothing of the box's: drawn as none */
    /* the box's count: how many of the thirty hold a Pokemon, centred on its screen at capitals 165 */
    v = BoxBoard(); v.box.slot[3].species = v.box.slot[17].species = 0; DevCheckTouch(&old, &v);
    i = kBoxCountPaper.x + (kBoxCountPaper.w - DevSmallTextWidth("28 / 30")) / 2;
    assert(BoxText(i, 165, 1, DEV_INK2, "28 / 30"));
    assert(Count(194, 162, 112, 2, DEV_INK2) == 0 && Count(194, 172, 112, 6, DEV_INK2) == 0);   /* (the slash: from 164) */
    v.box.slot[3].species = 9; assert(DevWhole(DevCheckTouch(&old, &v)) && BoxText(i, 165, 1, DEV_INK2, "29 / 30"));

    /* the hand's screen: IN HAND small at its top left; the Pokemon in hand (its icon at (194, 60), its name small from
     * x 232), or IN HAND alone; MOVE ITEMS: the item in hand at (198, 64), and on each place and Party row whose Pokemon
     * holds one a blue square at its lower right */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    assert(Count(196, 50, 40, 1, DEV_INK2) > 0 && Count(194, 46, 112, 4, DEV_INK2) == 0 && CountIn(kBoxHandPaper, STUB_ICON) == 24 * 24);
    assert(at(198, 64) == STUB_ICON && at(197, 64) != STUB_ICON && at(221, 87) == STUB_ICON && at(222, 87) != STUB_ICON);
    assert(BoxText(232, 72, 1, DEV_INK2, "KITMON") && Count(194, 72, 38, 7, DEV_INK2) == 0);
    v.box.holding = 0; DevCheckTouch(&old, &v);
    assert(BoxText(196, 50, 1, DEV_INK2, CTR_TR("IN HAND", "EN LA MANO", "IN DER HAND")));
    assert(CountIn(kBoxHandPaper, STUB_ICON) == 0 && Count(194, 57, 112, 37, DEV_INK2) == 0);
    v.box.held.species = 3; assert(DevCheckTouch(&old, &v).count == 0);       /* not in hand: not drawn */
    v.box.items = 1; v.box.handItem = 1; v.box.slotItem[0] = 5; v.box.slotItem[7] = 6; v.box.slotItem[8] = 7; v.box.slot[8].species = 0;
    DevCheckTouch(&old, &v);
    assert(CountIn(kBoxHandPaper, GbaColor(31)) == 24 * 24 && at(198, 64) == GbaColor(31) && at(221, 87) == GbaColor(31));
    assert(CountIn(kBoxHandPaper, STUB_ICON) == 0);
    for (i = 0; i < 9; ++i) {
        r = DevBoxCell(i);
        assert(Count(r.x + 16, r.y + 16, 6, 6, DEV_BLUE_INK) == (i == 0 || i == 7 ? 36 : 0));
    }
    v.box.area = ST_AREA_PARTY; v.box.pos = 0; v.box.partyItem[3] = 9; DevCheckTouch(&old, &v);
    for (i = 0; i < 6; ++i) { r = DevBoxPartyRow(i); assert((Count(r.x, r.y, r.w, r.h, DEV_BLUE_INK) == 36) == (i == 3)); }
    v.box.items = 0; v.box.area = ST_AREA_BOX; DevCheckTouch(&old, &v);
    assert(CountIn(kBoxPaper, DEV_BLUE_INK) == 0 && CountIn(kBoxHandPaper, GbaColor(31)) == 0);
    /* a name in hand wider than the screen is cut at 302 and counted (small letters 8 px for one draw) */
    v.box.holding = 1; BoxMonText(&v.box, "Kitmonkitm", "Kit", 0xb5, 12, ""); DevCheckTouch(&old, &v);
    {
        uint8_t keep[256];

        memcpy(keep, smallWidths, sizeof(keep));
        for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
            smallWidths[c] = 8;
        memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
        cuts = sDevKeyCut; CtrPokeTouch_Draw(canvas, &v);
        assert(sDevKeyCut == cuts + 1 && Count(232, 72, 70, 7, DEV_INK2) > 0 && Count(302, 68, 4, 12, DEV_INK2) == 0);
        memcpy(smallWidths, keep, sizeof(keep));
        memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
        sDevKeyCut = cuts; CtrPokeTouch_Draw(canvas, &old);
    }

    /* the footer: the pointed Pokemon (line 1 its name, the gender's sign and level from x 20 at capitals 199; line 2 its
     * type chips 10 tall at y 212, 3 apart from x 20, and ITEM with FireRed's item, its padding left out, or NONE, small
     * 7 px after the chips at capitals 214); an egg line 1 only; one not caught (no types) ITEM from x 20 */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_INK) > 0 && CountIn(kBoxNotePaper, DEV_MALE) > 0 && CountIn(kBoxNotePaper, DEV_FEMALE) == 0);
    assert(Count(14, 196, 216, 2, DEV_INK) == 0 && Count(20, 199, 60, 1, DEV_INK) > 0 && at(19, 202) != DEV_INK);   /* (digits: 198) */
    assert(at(20, 212) == UiTypeColor(BOX_ROCK) && at(59, 221) == UiTypeColor(BOX_ROCK) && at(60, 212) != UiTypeColor(BOX_ROCK));
    assert(at(20, 211) != UiTypeColor(BOX_ROCK) && at(20, 222) != UiTypeColor(BOX_ROCK));
    assert(at(63, 212) == UiTypeColor(BOX_GROUND) && at(102, 221) == UiTypeColor(BOX_GROUND) && at(62, 212) != UiTypeColor(BOX_GROUND));
    assert(BoxText(110, 214, 1, DEV_INK2, CTR_TR("ITEM NONE", "OBJETO NINGUNO", "ITEM KEINS")));
    BoxMonText(&v.box, "Kitmon", "Kitmon", 0xb6, 12, "Potion"); DevCheckTouch(&old, &v);   /* "Potion" padded to 8 */
    assert(CountIn(kBoxNotePaper, DEV_FEMALE) > 0 && CountIn(kBoxNotePaper, DEV_MALE) == 0 && BoxText(110, 214, 1, DEV_INK2, BOX_ITEM "POTION"));
    BoxMonText(&v.box, "Kitmon", "Kitmon", 0x00, 12, "Leaf charm"); DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_FEMALE) == 0 && CountIn(kBoxNotePaper, DEV_MALE) == 0 && BoxText(110, 214, 1, DEV_INK2, BOX_ITEM "LEAF CHARM"));
    BoxTypes(&v.box, BOX_ROCK, "ROCK", -1, ""); DevCheckTouch(&old, &v);
    assert(BoxText(67, 214, 1, DEV_INK2, BOX_ITEM "LEAF CHARM") && Count(60, 212, 50, 10, UiTypeColor(BOX_GROUND)) == 0);
    BoxTypes(&v.box, -1, "", -1, ""); DevCheckTouch(&old, &v);
    assert(BoxText(20, 214, 1, DEV_INK2, BOX_ITEM "LEAF CHARM") && Count(14, 212, 216, 10, UiTypeColor(BOX_ROCK)) == 0);
    v.box.mon.egg = 1; BoxTypes(&v.box, BOX_ROCK, "ROCK", BOX_GROUND, "GROUND"); DevCheckTouch(&old, &v);
    assert(Count(20, 199, 120, 9, DEV_INK) > 0 && Count(14, 208, 216, 18, DEV_INK2) == 0 && Count(14, 208, 216, 18, UiTypeColor(BOX_ROCK)) == 0);
    v.box.monType[0] = BOX_GROUND; assert(DevCheckTouch(&old, &v).count == 0);   /* an egg's types: not drawn */
    v.box.mon.egg = 0; v.box.monType[0] = BOX_ROCK; DevCheckTouch(&old, &v);
    /* the widest real pair of chips (FIGHTING, ELECTRIC) and a long item: the line fits by 224 */
    BoxTypes(&v.box, 1, "FIGHTING", 13, "ELECTRIC"); BoxMonText(&v.box, "Kitmon", "Kitmon", 0xb5, 12, "Silverpowder");
    cuts = sDevKeyCut; DevCheckTouch(&old, &v); assert(sDevKeyCut == cuts && Count(224, 212, 6, 10, DEV_INK2) == 0);
    /* MOVE ITEMS: the item the Pokemon holds (its icon at (18, 199), its name from x 48 at capitals 198, the Pokemon's
     * name ending at 224, FireRed's description small in two lines from capitals 209), or the Pokemon's name and that it
     * holds none */
    v = BoxBoard(); v.box.holding = 0; v.box.items = 1; v.box.monItem = 2;
    NameCodes(v.box.itemName, (int)sizeof(v.box.itemName), "Leaf charm");
    NameCodes(v.box.itemDesc, (int)sizeof(v.box.itemDesc), "Port words for this test in place of an item's.");
    DevCheckTouch(&old, &v); assert(CountIn(kBoxNotePaper, GbaColor(31 << 5)) == 24 * 24 && at(18, 199) == GbaColor(31 << 5) && at(41, 222) == GbaColor(31 << 5));
    assert(BoxText(48, 198, 0, DEV_INK, "LEAF CHARM") && BoxText(224 - DevTextWidth("KITMON"), 198, 0, DEV_INK2, "KITMON"));
    assert(Count(48, 209, 176, 7, DEV_INK) > 0 && Count(48, 218, 176, 7, DEV_INK) > 0 && Count(14, 212, 216, 10, UiTypeColor(BOX_ROCK)) == 0);
    SmallDescenders(1); CtrPokeTouch_Draw(canvas, &v);      /* the description's second line's descenders end on the paper */
    assert(Count(48, 225, 176, 1, DEV_INK) > 0 && Count(14, 226, 216, 5, DEV_INK) == 0);
    SmallDescenders(0); CtrPokeTouch_Draw(canvas, &old);
    v.box.monType[1] ^= 1; assert(DevCheckTouch(&old, &v).count == 0);         /* MOVE ITEMS: the types not drawn */
    v.box.monItem = 0; DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, GbaColor(31 << 5)) == 0 && Count(20, 199, 60, 9, DEV_INK) > 0);
    {
        char none[64];

        DevUpper(none, CtrUiText(UIS_BOX_NO_ITEM), sizeof(none));
        assert(BoxText(20, 214, 1, DEV_INK2, none));
    }
    v.box.items = 0; v.box.mon.species = 0; DevCheckTouch(&old, &v); assert(BoxText(20, 207, 0, DEV_INK, "Box one"));
    memset(v.box.boxName, 0xff, sizeof(v.box.boxName)); DevCheckTouch(&old, &v); assert(CountIn(kBoxNotePaper, DEV_INK) == 0);
    BoxCodes(v.box.boxName, (int)sizeof(v.box.boxName), "Box one", 0); DevCheckTouch(&old, &v);
    BoxMonText(&v.box, "Kit", "Kitmon", 0xb5, 3, ""); v.box.monType[0] = 9; assert(DevCheckTouch(&old, &v).count == 0);   /* no Pokemon: not drawn */
    /* FireRed's message, read on the footer (DevReadText from x 20; its ▼ at (220, 220) while it waits for A) */
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port words for this test in place of the box's message."); DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_INK) > 0 && CountIn(kBoxNotePaper, DEV_INK2) == 0 && Count(14, 196, 6, 30, DEV_INK) == 0);
    v.box.state = PT_BOX_STATE_MESSAGE; DevCheckTouch(&old, &v);
    assert(Count(216, 218, 9, 5, DEV_INK2) == 25 && CountIn(kBoxNotePaper, DEV_INK2) == 25);   /* ▼ 9 px across, 5 down */
    assert(Count(212, 196, 18, 30, DEV_INK) == 0);

    /* a message waiting for A: a tap anywhere is its A; the box's keys grey, the box's paper as it is (its icons faded but
     * the cursor's place's), the hand and the count as they are */
    assert(CtrPokeTouch_BoxHit(&v, 319, 239) == PT_BOX_HIT_MESSAGE && CtrPokeTouch_BoxHit(&v, DevBoxCell(3).x + 5, DevBoxCell(3).y + 5) == PT_BOX_HIT_MESSAGE);
    assert(at(15, 100) == DEV_PAPER2 && at(kDevBoxParty.x + 3, kDevBoxParty.y + 20) == BOX_DIM(DEV_KEY));
    r = DevBoxCell(3); assert(at(r.x + 12, r.y + 12) == BOX_FADED(DEV_PAPER) && at(r.x, r.y + 12) == BOX_FADED(DEV_PAPER2));
    r = DevBoxCell(29); assert(at(r.x + 12, r.y + 12) == STUB_ICON && at(r.x, r.y + 12) == DEV_BEZEL);
    assert(at(kDevGridExit.x + 3, kDevGridExit.y + 20) == BOX_DIM(DEV_BEZEL) && at(kDevBoxPrev.x + 3, kDevBoxPrev.y + 12) == BOX_DIM(DEV_KEY));
    assert(at(kBoxHandPaper.x + 1, kBoxHandPaper.y + 1) == DEV_PAPER && at(kBoxCountPaper.x + 1, kBoxCountPaper.y + 1) == DEV_PAPER);
    v.box.pressed = ST_HIT_BUTTON_PARTY; assert(DevCheckTouch(&old, &v).count == 0);   /* nothing sinks under a message */

    /* while FireRed is busy (its cursor walking, a fade): the box kept, its icons whole, no cursor's frame, nothing hit */
    v = BoxBoard(); v.box.state = PT_BOX_STATE_BUSY; DevCheckTouch(&old, &v);
    assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == STUB_ICON && CountIn(kBoxPaper, DEV_BEZEL) == 0);
    assert(at(DevBoxCell(3).x + 12, DevBoxCell(3).y + 12) == STUB_ICON && CountIn(kBoxPaper, BOX_FADED(DEV_PAPER)) == 0);
    assert(CtrPokeTouch_BoxHit(&v, DevBoxCell(3).x + 5, DevBoxCell(3).y + 5) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 200, 120) == ST_HIT_NONE);
    v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0);
    assert(CountIn(kBoxHandPaper, STUB_ICON) > 0 && CtrPokeTouch_BoxHit(&v, 278, 210) == ST_HIT_NONE);

    /* the Party on the box's paper: a row for each slot (half icon, nickname, level) and CANCEL, the cursor's inverted or
     * the one under a held touch; the box's keys grey and not hit, nothing of the box hit */
    v = BoxBoard(); v.box.area = ST_AREA_PARTY; v.box.pos = 2; DevCheckTouch(&old, &v);
    for (i = 0; i < 7; ++i) { r = DevBoxPartyRow(i); assert(at(r.x + 1, r.y + 1) == (i == 2 ? DEV_BEZEL : DEV_PAPER)); }
    r = DevBoxPartyRow(0); assert(Count(r.x + 2, r.y + 2, 16, 16, STUB_ICON) == 12 * 12 && Count(r.x, r.y, r.w, r.h, DEV_INK) > 0);
    r = DevBoxPartyRow(6); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) > 0 && Count(r.x, r.y, r.w, r.h, STUB_ICON) == 0);
    assert(at(kDevBoxParty.x + 3, kDevBoxParty.y + 20) == BOX_DIM(DEV_KEY) && at(kDevGridExit.x + 3, kDevGridExit.y + 20) == BOX_DIM(DEV_BEZEL));
    assert(at(kDevBoxNext.x + 3, kDevBoxNext.y + 12) == BOX_DIM(DEV_KEY));
    assert(CtrPokeTouch_BoxHit(&v, kDevBoxParty.x + 5, kDevBoxParty.y + 5) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, kDevBoxPrev.x + 5, kDevBoxPrev.y + 5) == ST_HIT_NONE);
    assert(CtrPokeTouch_BoxHit(&v, kDevGridExit.x + 5, kDevGridExit.y + 5) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 14, 14) == ST_HIT_NONE);
    v.box.pressed = ST_HIT_PARTY_BASE + 4; DevCheckTouch(&old, &v);
    for (i = 0; i < 7; ++i) { r = DevBoxPartyRow(i); assert(at(r.x + 1, r.y + 1) == (i == 4 ? DEV_BEZEL : DEV_PAPER)); }
    v.box.pressed = ST_HIT_PREV; DevCheckTouch(&old, &v); r = DevBoxPartyRow(2); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    v.box.pressed = -1; v.box.party[5].species = 0; v.box.party[4].egg = 1; DevCheckTouch(&old, &v);
    r = DevBoxPartyRow(5); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) == 0 && Count(r.x, r.y, r.w, r.h, STUB_ICON) == 0);
    r = DevBoxPartyRow(4); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) > 0);   /* an egg: its name, no level */

    /* a menu (five, FireRed's six): the box's paper, band and frame at full strength, each icon but the cursor's place's
     * at half over what is under it (never DevDim), the footer as it is, FireRed's rows but the last as keys in the right
     * column (their words from x + 10), the last (CANCEL: menuCancel) the dark key in the exit corner, FireRed's cursor's
     * sunk or the one under a held touch; no hand, no BACK, nothing of the box hit or drawn there */
    v = BoxBoard(); BoxMenu(&v, 5); v.box.menuCursor = 1; DevCheckTouch(&old, &v);
    BoxKeysHit(&v, 5, 1); BoxKeysSunk(5, 1, 1); assert(!BackV(&v, kDevGridExit.x + 5, kDevGridExit.y + 5));
    r = DevBoxMenuKey(4); assert(CtrPokeTouch_BoxHit(&v, r.x + 5, r.y + 5) == ST_HIT_NONE && at(r.x + 3, r.y + 15) == DEV_SHELL_RED);
    assert(at(15, 100) == DEV_PAPER2 && at(kBoxNotePaper.x + 1, kBoxNotePaper.y + 1) == DEV_PAPER && at(10, 100) == DEV_BEZEL);
    assert(CountIn(kDevBoxTitle, DEV_INK) > 0 && CountIn(kBoxPaper, DEV_BEZEL) > 0);
    for (i = 0; i < 29; ++i) { r = DevBoxCell(i); assert(at(r.x + 12, r.y + 12) == BOX_FADED(DEV_PAPER)); }
    r = DevBoxCell(29); assert(at(r.x + 12, r.y + 12) == STUB_ICON && at(r.x, r.y + 12) == DEV_BEZEL);
    assert(Count(r.x - 4, r.y - 6, 32, 32, STUB_ICON) == CountIn(kBoxPaper, STUB_ICON));   /* whole: the cursor's alone */
    v.box.slot[0].species = 0; DevCheckTouch(&old, &v); r = DevBoxCell(0); assert(at(r.x + 12, r.y + 12) == DEV_PAPER);
    v.box.slot[0].species = 1; DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_MALE) > 0 && Count(188, 0, 132, 186, STUB_ICON) == 0);
    for (i = 0; i < 30; ++i) { r = DevBoxCell(i); assert(CtrPokeTouch_BoxHit(&v, r.x + 12, r.y + 12) == ST_HIT_NONE); }
    assert(CtrPokeTouch_BoxHit(&v, 20, 20) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 100, 200) == ST_HIT_NONE);
    BoxKeyWord(&v, DevBoxMenuKey(0), DEV_LOOK_KEY, 0, "MOVE", 10); BoxKeyWord(&v, DevBoxMenuKey(1), DEV_LOOK_KEY, 1, "SUMMARY", 10);
    BoxKeyWord(&v, kDevGridExit, DEV_LOOK_DARK, 0, "CANCEL", 0);
    r = DevBoxMenuKey(0); assert(Count(r.x + 10, r.y + 10, 1, 9, DEV_KEY_INK) > 0 && Count(r.x, r.y, 10, r.h, DEV_KEY_INK) == 0);
    v.box.pressed = ST_HIT_MENU_BASE + 3; DevCheckTouch(&old, &v); BoxKeysSunk(5, 3, 1);
    assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == DEV_BEZEL);   /* a held key is the menu's alone */
    v.box.pressed = ST_HIT_MENU_BASE + 4; DevCheckTouch(&old, &v); BoxKeysSunk(5, 4, 1);   /* the dark CANCEL held: sunk, lit */
    v.box.pressed = ST_HIT_MENU_BASE + 6; DevCheckTouch(&old, &v); BoxKeysSunk(5, 1, 1);   /* no such key: as none */
    v.box.pressed = 3; assert(DevCheckTouch(&old, &v).count == 0);
    assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == DEV_BEZEL);   /* the place the menu is for */
    v.box.pressed = -1; v.box.chooseCount = 9; v.box.marks = 3; assert(DevCheckTouch(&old, &v).count == 0);
    v.backPressed = 1; assert(DevCheckTouch(&old, &v).count == 0);       /* a menu with its CANCEL: no BACK */
    v.backPressed = 0; DevCheckTouch(&old, &v);
    BoxMenu(&v, 6); v.box.menuCursor = 5; DevCheckTouch(&old, &v); BoxKeysSunk(6, 5, 1); BoxKeysHit(&v, 6, 1);
    for (int n = 1; n <= 6; ++n) {
        BoxMenu(&v, n); v.box.menuCursor = 0; DevCheckTouch(&old, &v);
        BoxKeysHit(&v, n, 1); BoxKeysSunk(n, 0, 1);
    }
    /* a menu of seven with its CANCEL (none in FireRed): the sixth row lost (not drawn, not hit), counted; CANCEL still in
     * the exit corner */
    cuts = sDevKeyCut; BoxMenu(&v, 7); DevCheckTouch(&old, &v);
    BoxKeysSunk(5, 0, 0); AssertHits(BoxHitV, &v, kDevGridExit, ST_HIT_MENU_BASE + 6);
    for (i = 0; i < 5; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(i), ST_HIT_MENU_BASE + i);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) assert(BoxHitV(&v, x, y) != ST_HIT_MENU_BASE + 5);
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &v); assert(sDevKeyCut == 1); sDevKeyCut = cuts;
    /* menus without FireRed's CANCEL (the wallpaper menus: B leaves them): every row a key of the column, FireRed's cursor
     * sunk on any of them, CANCEL in the exit corner the view's BACK (FireRed's B), sunk while held, grey while the box
     * takes no B, not drawn without BACK */
    v = BoxBoard(); BoxWallMenu(&v); v.box.menuCursor = 3; DevCheckTouch(&old, &v);
    BoxKeysSunk(4, 3, 0); BoxKeysHit(&v, 4, 0); AssertHits(BackV, &v, kDevGridExit, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    BoxKeyWord(&v, DevBoxMenuKey(3), DEV_LOOK_KEY, 1, "ETCETERA", 10); BoxKeyWord(&v, kDevGridExit, DEV_LOOK_DARK, 0, DevCancelWord(), 0);
    r = DevBoxMenuKey(4); assert(at(r.x + 3, r.y + 15) == DEV_SHELL_RED && BoxHitV(&v, r.x + 5, r.y + 5) == ST_HIT_NONE);
    for (i = 0; i < 30; ++i) { r = DevBoxCell(i); assert(at(r.x + 12, r.y + 12) == BOX_FADED(DEV_PAPER)); }   /* the cursor on the band */
    assert(at(14, 14) == DEV_BEZEL);
    v.box.pressed = ST_HIT_MENU_BASE + 1; DevCheckTouch(&old, &v); BoxKeysSunk(4, 1, 0);
    v.box.pressed = ST_HIT_MENU_BASE + 4; DevCheckTouch(&old, &v); BoxKeysSunk(4, 3, 0);   /* no such key: as none */
    v.box.pressed = -1; v.backPressed = 1; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(kDevGridExit.x + 3, kDevGridExit.y + kDevGridExit.h / 2) == DIM_DARK);
    v.backPressed = 0; v.backEnabled = 1; v.backVisible = 0; DevCheckTouch(&old, &v);
    assert(at(kDevGridExit.x + 3, kDevGridExit.y + kDevGridExit.h / 2) == DEV_SHELL_RED && !BackV(&v, kDevGridExit.x + 5, kDevGridExit.y + 5));
    v.backVisible = 1; DevCheckTouch(&old, &v);
    v = BoxBoard(); BoxMenu(&v, 5); v.box.area = ST_AREA_PARTY; v.box.pos = 1; DevCheckTouch(&old, &v);   /* a Party Pokemon's menu */
    r = DevBoxPartyRow(1); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL && Count(r.x + 2, r.y + 2, 16, 16, STUB_ICON) == 12 * 12);
    r = DevBoxPartyRow(0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER && Count(r.x + 2, r.y + 2, 16, 16, BOX_FADED(DEV_PAPER)) == 12 * 12);

    /* YES / NO (def_choice_yesno): two tall cream keys in the column, their words centred; FireRed's cursor sunk or the
     * one held; BACK (FireRed's B: NO) the dark key in the exit corner, sunk while held, grey while the box takes no B */
    v = BoxBoard(); BoxYesNo(&v, 1);
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port question for this test?"); DevCheckTouch(&old, &v);
    AssertKey(kDevBoxYes, DEV_LOOK_KEY, 0); AssertKey(kDevBoxNo, DEV_LOOK_KEY, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    AssertHits(BoxHitV, &v, kDevBoxYes, ST_HIT_MENU_BASE); AssertHits(BoxHitV, &v, kDevBoxNo, ST_HIT_MENU_BASE + 1);
    AssertHits(BackV, &v, kDevGridExit, 1); assert(CountIn(kBoxNotePaper, DEV_INK) > 0);
    assert(CtrPokeTouch_BoxHit(&v, 60, 60) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 278, 210) == ST_HIT_NONE && at(250, 96) == DEV_SHELL_RED);
    DevUpper(word, CtrUiText(UIS_YES), sizeof(word)); BoxKeyWord(&v, kDevBoxYes, DEV_LOOK_KEY, 0, word, 0);
    DevUpper(word, CtrUiText(UIS_NO), sizeof(word)); BoxKeyWord(&v, kDevBoxNo, DEV_LOOK_KEY, 1, word, 0);
    BoxKeyWord(&v, kDevGridExit, DEV_LOOK_DARK, 0, DevBackWord(), 0);
    r = DevBoxCell(29); assert(at(r.x + 12, r.y + 12) == STUB_ICON); r = DevBoxCell(28); assert(at(r.x + 12, r.y + 12) == BOX_FADED(DEV_PAPER));
    v.box.pressed = ST_HIT_MENU_BASE; DevCheckTouch(&old, &v); AssertKey(kDevBoxYes, DEV_LOOK_KEY, 1); AssertKey(kDevBoxNo, DEV_LOOK_KEY, 0);
    v.box.pressed = ST_HIT_MENU_BASE + 2; DevCheckTouch(&old, &v); AssertKey(kDevBoxYes, DEV_LOOK_KEY, 0); AssertKey(kDevBoxNo, DEV_LOOK_KEY, 1);
    v.box.pressed = -1; v.box.menuCancel = 1; v.box.menu[0][0] = 0xbb; assert(DevCheckTouch(&old, &v).count == 0);   /* a menu's: not drawn */
    v.backPressed = 1; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertKey(kDevBoxNo, DEV_LOOK_KEY, 1);
    v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(kDevGridExit.x + 3, kDevGridExit.y + kDevGridExit.h / 2) == DIM_DARK);
    v.backPressed = 0; v.backEnabled = 1; DevCheckTouch(&old, &v);

    /* the chooser: ◀ ▶ (sunk while held) and the chosen box's key in the hand's place, always sunk: its name and how many
     * it holds; the box's icons faded and not hit; CANCEL in the exit corner, the view's BACK (FireRed's B), a dark key
     * hit where it is drawn, sunk while held, grey while the chooser takes no B */
    v = BoxBoard(); v.box.state = PT_BOX_STATE_CHOOSE; BoxCodes(v.box.chooseName, (int)sizeof(v.box.chooseName), "Box two", 0);
    v.box.chooseCount = 12; DevCheckTouch(&old, &v);
    AssertHits(BoxHitV, &v, kDevBoxPrev, ST_HIT_PREV); AssertHits(BoxHitV, &v, kDevBoxNext, ST_HIT_NEXT); AssertHits(BoxHitV, &v, kDevBoxChoose, ST_HIT_TITLE);
    AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0); AssertKey(kDevBoxNext, DEV_LOOK_KEY, 0);
    assert(at(15, 100) == DEV_PAPER2 && Count(kDevBoxChoose.x, kDevBoxChoose.y, kDevBoxChoose.w, kDevBoxChoose.h, DEV_SUNK_INK) > 0);
    assert(at(DevBoxCell(3).x + 12, DevBoxCell(3).y + 12) == BOX_FADED(DEV_PAPER));
    assert(CountIn(kBoxHandPaper, STUB_ICON) == 0 && Count(188, 40, 124, 64, DEV_INK2) == 0);    /* no hand under the key */
    assert(CtrPokeTouch_BoxHit(&v, 60, 60) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 20, 20) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 250, 120) == ST_HIT_NONE);
    v.box.pressed = ST_HIT_PREV; DevCheckTouch(&old, &v); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 1); AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1);
    v.box.pressed = ST_HIT_TITLE; DevCheckTouch(&old, &v); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0); AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1);
    other = v; other.box.chooseCount = 5; TouchSamePixels(&other, &v, 0, "how many the chosen box holds");   /* "5/30" */
    v.box.pressed = -1; NameCodes(v.box.message, (int)sizeof(v.box.message), "Port words: which box?"); DevCheckTouch(&old, &v);
    AssertHits(BackV, &v, kDevGridExit, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    assert(CtrPokeTouch_BoxHit(&v, kDevGridExit.x + 5, kDevGridExit.y + 5) == ST_HIT_NONE && !CtrPokeTouch_BackHit(&v, 100, 210));
    assert(CountIn(kBoxNotePaper, DEV_INK) > 0);
    v.backPressed = 1; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1);
    v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(kDevGridExit.x + 3, kDevGridExit.y + kDevGridExit.h / 2) == DIM_DARK);
    v.backPressed = 0; v.backEnabled = 1; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
    BoxKeyWord(&v, kDevGridExit, DEV_LOOK_DARK, 0, DevCancelWord(), 0);
    CtrPokeTouch_Draw(canvas, &old);
    v.backVisible = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_BackHit(&v, kDevGridExit.x + 5, kDevGridExit.y + 5));
    assert(at(kDevGridExit.x + 3, kDevGridExit.y + kDevGridExit.h / 2) == DEV_SHELL_RED);
    v.backVisible = 1; DevCheckTouch(&old, &v);
    memset(v.box.menu, 0xff, sizeof(v.box.menu));             /* a menu of two empty rows in MENU, with and without CANCEL */
    for (int cancel = 0; cancel < 2; ++cancel)                 /* BACK: the chooser's, YES / NO's, a menu's without CANCEL */
        for (i = PT_BOX_STATE_BUSY; i <= PT_BOX_STATE_MARKS; ++i) {
            int back = i == PT_BOX_STATE_CHOOSE || i == PT_BOX_STATE_YESNO || (i == PT_BOX_STATE_MENU && !cancel);

            v.box.state = (uint8_t)i; v.box.menuCount = 2; v.box.menuCancel = (uint8_t)cancel; v.backPressed = 0;
            DevCheckTouch(&old, &v);
            assert(CtrPokeTouch_BackHit(&v, kDevGridExit.x + 5, kDevGridExit.y + 5) == back);
            v.backPressed = 1;
            assert(back ? DevWhole(DevCheckTouch(&old, &v)) : DevCheckTouch(&old, &v).count == 0);
        }
    v.backPressed = 0;

    /* the marks: the four marks at 2x (on in the key's ink, off dim) and OK in the column, CANCEL the dark key in the exit
     * corner; FireRed's cursor sunk */
    markArt = 1; v = BoxBoard(); v.box.state = PT_BOX_STATE_MARKS; v.box.menuCount = 6; v.box.marks = 5; v.box.menuCursor = 4;
    DevCheckTouch(&old, &v); BoxKeysSunk(6, 4, 1); BoxKeysHit(&v, 6, 1);
    for (i = 0; i < 4; ++i) {
        r = DevBoxMenuKey(i);
        assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, (v.box.marks >> i) & 1 ? DEV_KEY_INK : DEV_INK_DIM) == 12 * 12);
    }
    BoxKeyWord(&v, kDevGridExit, DEV_LOOK_DARK, 0, DevCancelWord(), 0);
    v.box.menuCursor = 1; DevCheckTouch(&old, &v); BoxKeysSunk(6, 1, 1);
    r = DevBoxMenuKey(1); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_INK_DIM) == 12 * 12);
    v.box.pressed = ST_HIT_MENU_BASE + 5; DevCheckTouch(&old, &v); BoxKeysSunk(6, 5, 1);
    markArt = 0;

    /* no BACK in the box's own state (CLOSE is FireRed's exit): nothing hit where a view's BACK is */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    assert(!CtrPokeTouch_BackHit(&v, 278, 210) && !CtrPokeTouch_BackHit(&v, 50, 220) && !CtrPokeTouch_BackHit(&v, 20, 20));

    /* what the box does not draw presents nothing; the shell's colour whole; the ways in and out (HOME; FireRed's own
     * picture while it carries several: in the frame's screen, nothing of the box hit) */
    strcpy(v.bar[0], "Anything"); v.enabled[PT_BAG] = 1; v.playerX = 3; v.backVisible = 0; v.box.box = 5;
    assert(DevCheckTouch(&old, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&old, &v);
    other = v; other.box.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)));
    v = BoxBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    v.viewport = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && DrawnBy(&v, DevFrameShown) && CtrPokeTouch_BoxHit(&v, 30, 50) == ST_HIT_NONE);
    v = BoxBoard(); DevCheckTouch(&old, &v);
    BoxWords(BoxBoard());
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first. */
    v = BoxBoard(); v.box.holding = 0; for (i = 0; i < 30; i += 5) v.box.slot[i].species = 0; v.box.pos = 8;
    DevCheckTouch(&old, &v); shot("full");
    memset(v.box.slot, 0, sizeof(v.box.slot)); v.box.mon.species = 0; DevCheckTouch(&old, &v); shot("empty");
    v = BoxBoard(); for (i = 0; i < 30; i += 4) v.box.slot[i].species = 0;
    v.box.pos = 14; BoxMonText(&v.box, "Kit", "Kitmon", 0xb6, 45, "Leaf charm"); DevCheckTouch(&old, &v); shot("held");
    v.box.area = ST_AREA_TITLE; DevCheckTouch(&old, &v); shot("title");
    v.box.area = ST_AREA_BUTTONS; v.box.pos = 1; v.box.pressed = ST_HIT_NEXT; DevCheckTouch(&old, &v); shot("buttons");
    v = BoxBoard(); v.box.area = ST_AREA_PARTY; v.box.pos = 3; v.box.party[5].species = 0; DevCheckTouch(&old, &v); shot("party");
    v = BoxBoard(); BoxMenu(&v, 5); v.box.menuCursor = 0;
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Kitmon is selected."); DevCheckTouch(&old, &v); shot("menu5");
    BoxMenu(&v, 6); v.box.menuCursor = 2; DevCheckTouch(&old, &v); shot("menu6");
    v = BoxBoard(); BoxWallMenu(&v); v.box.menuCursor = 1;
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port words: which theme?"); DevCheckTouch(&old, &v); shot("menu_wallpaper");
    v = BoxBoard(); BoxYesNo(&v, 0);
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port question for this test?"); DevCheckTouch(&old, &v); shot("yesno");
    v = BoxBoard(); v.box.state = PT_BOX_STATE_CHOOSE; v.box.area = ST_AREA_PARTY; v.box.pos = 1;
    BoxCodes(v.box.chooseName, (int)sizeof(v.box.chooseName), "Box two", 0); v.box.chooseCount = 12;
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port words: which box?"); DevCheckTouch(&old, &v); shot("choose");
    markArt = 1; v = BoxBoard(); v.box.state = PT_BOX_STATE_MARKS; v.box.menuCount = 6; v.box.marks = 9; v.box.menuCursor = 2;
    DevCheckTouch(&old, &v); shot("marks");
    markArt = 0;
    v = BoxBoard(); v.box.state = PT_BOX_STATE_MESSAGE;
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port words for this test in place of a message."); DevCheckTouch(&old, &v); shot("message");
    v = BoxBoard(); v.box.holding = 0; v.box.items = 1; v.box.handItem = 1; v.box.slotItem[0] = 5; v.box.slotItem[7] = 6;
    v.box.pos = 7; v.box.monItem = 2; NameCodes(v.box.itemName, (int)sizeof(v.box.itemName), "Leaf charm");
    NameCodes(v.box.itemDesc, (int)sizeof(v.box.itemDesc), "Port words for this test in place of an item's description.");
    DevCheckTouch(&old, &v); shot("items");
    monArt = 0;
    assert(sDevKeyCut == 0);                      /* main prints PASS device box */
}
