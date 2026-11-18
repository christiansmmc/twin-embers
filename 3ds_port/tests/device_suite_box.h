/* ---- box: the PC box as the red device, through the field dispatcher (no board; plan 3 Task 6) ----------------------
 * Included by device_render_test.c before its kSuites (after the naming suite, whose NameCodes it uses). Its art is the
 * harness's: the Pokemon's icons (monArt, STUB_ICON), the wallpaper (wallArt, WallArtColor) and the marks (markArt). */

/* The papers it draws on (each screen's paper is 6 px in): the box's (FireRed's 160x144 wallpaper at 1:1), the note's,
 * the narrow note's and the hand's. */
static const CtrPokeTouchRect kBoxPaper = {14, 14, 160, 144}, kBoxNotePaper = {14, 178, 292, 48};
static const CtrPokeTouchRect kBoxNarrowPaper = {14, 178, 160, 48}, kBoxHandPaper = {194, 126, 112, 32};
#define BOX_DIM(c) DevBlend(DEV_SHELL_RED, (c), DEV_A_DIM)   /* a pixel under DevDim, on the red shell */

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

/* The box as 3ds_bottom.c gives it (CtrBottomStorage_BoxFill, BoxSnapshot): a full box of thirty, the cursor on its
 * last place, a Pokemon in hand, a full Party, the cursor's Pokemon's lines (a nickname that is its species', a male of
 * level 12, no item), no message, nothing held; port-written names. The view's BACK is asked for (BACK of the storage
 * modes) but the box draws none. */
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
    memset(b->message, 0xff, sizeof(b->message));
    snprintf(v.bar[0], PT_BAR_LEN, "Box 01");                  /* the bar: not drawn by the box */
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* The box's menu as FireRed gives it: n rows ("Move", "Summary"... the last "Cancel"). */
static void BoxMenu(CtrPokeTouchView *v, int n)
{
    static const char *const kRows[PT_BOX_MENU_MAX] = {"Move", "Summary", "Withdraw", "Mark", "Release", "Item", "Cancel"};

    v->box.state = PT_BOX_STATE_MENU;
    v->box.menuCount = (uint8_t)n;
    memset(v->box.menu, 0xff, sizeof(v->box.menu));
    for (int i = 0; i < n; ++i)
        NameCodes(v->box.menu[i], (int)sizeof(v->box.menu[i]), kRows[i == n - 1 ? PT_BOX_MENU_MAX - 1 : i]);
}

/* The keys of the right column up but `sunk` (-1 none), the last dark when `darkLast`. */
static void BoxKeysSunk(int n, int sunk, int darkLast)
{
    for (int i = 0; i < n; ++i)
        AssertKey(DevBoxMenuKey(n, i), darkLast && i == n - 1 ? DEV_LOOK_DARK : DEV_LOOK_KEY, i == sunk);
}

/* A port word on a key, in capitals (decision 5): with small letters wider than capitals the key's face is the kit's
 * key drawn again with the word in capitals, and (where the port writes it with small letters) not with it as written. */
static void BoxKeyInCapitals(const CtrPokeTouchView *v, CtrPokeTouchRect r, int look, int sunk, const char *raw)
{
    static uint16_t drawn[W * H];
    uint8_t keep[256];
    char upper[32];
    int faceY, same;

    memcpy(keep, fontWidths, sizeof(keep));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    DevUpper(upper, raw, sizeof(upper));
    CtrPokeTouch_Draw(canvas, v);
    memcpy(drawn, canvas, sizeof(drawn));
    for (int pass = 0; pass < 2; ++pass) {
        faceY = DevKey(&r, look, sunk, 6);
        DevKeyWord(&r, faceY, look, sunk, pass ? raw : upper);
        same = 1;
        for (int x = r.x + 6; x < r.x + r.w - 6; ++x)
            for (int y = faceY + 2; y < faceY + r.h - 2; ++y)
                same &= drawn[x * H + H - 1 - y] == at(x, y);
        assert(pass ? same == !strcmp(raw, upper) : same);
    }
    memcpy(fontWidths, keep, sizeof(keep));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
}

/* The wallpaper (wallArt) at 1:1 on the paper: every pixel under the band (no place outlined on it) and the band's
 * corners, as WallArtColor gives the picture's pixel there. */
static void BoxWallpaperAt1To1(unsigned id)
{
    for (int y = kDevBoxTitle.y + kDevBoxTitle.h; y < kBoxPaper.y + kBoxPaper.h; ++y)
        for (int x = kBoxPaper.x; x < kBoxPaper.x + kBoxPaper.w; ++x)
            assert(at(x, y) == GbaColor(WallArtColor(id, x - kBoxPaper.x, y - kBoxPaper.y)));
    assert(at(14, 14) == GbaColor(WallArtColor(id, 0, 0)) && at(173, 37) == GbaColor(WallArtColor(id, 159, 23)));
}

/* Upper case (decision 5): FireRed's menu rows, species line and item in capitals; the box's name, a nickname and the
 * chosen box's name as typed (a nickname that is its species' own in capitals); the port's PARTY and CLOSE BOX. */
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
    TouchSamePixels(&cased, &v, 1, "the species line and the item, in capitals");
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
    BoxKeyInCapitals(&v, kDevBoxParty, DEV_LOOK_KEY, 1, CtrUiText(UIS_BOX_PARTY));
    BoxKeyInCapitals(&v, kDevBoxClose, DEV_LOOK_DARK, 0, CtrUiText(UIS_BOX_CLOSE));
}

/* The plan's suite (Review Focus 3: a full box and an empty one with every place hit where it is drawn, the cursor's
 * frame on place 29, the Pokemon in hand on the hand's screen, nothing of the box hit under a menu; the Party, a menu of
 * seven, YES / NO, the chooser, the marks, a message), then each state's pieces, the words, the ways in and out and the
 * states for the eye. */
static void SuiteBox(void)
{
    CtrPokeTouchView v = BoxBoard(), old = v, other;
    CtrPokeTouchRect r;
    int i;

    monArt = 1;
    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevBoxShown));
    for (i = 0; i < 30; ++i) AssertRowHits(BoxHitV, &v, DevBoxCell(i), i);
    AssertRowHits(BoxHitV, &v, kDevBoxTitle, ST_HIT_TITLE); AssertHits(BoxHitV, &v, kDevBoxPrev, ST_HIT_PREV); AssertHits(BoxHitV, &v, kDevBoxNext, ST_HIT_NEXT);
    AssertHits(BoxHitV, &v, kDevBoxParty, ST_HIT_BUTTON_PARTY); AssertHits(BoxHitV, &v, kDevBoxClose, ST_HIT_BUTTON_CLOSE);
    assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == DEV_BEZEL && Count(188, 120, 124, 44, STUB_ICON) > 0);   /* the cursor's frame; the held icon */
    assert(CtrPokeTouch_BoxHit(&v, 250, 140) == ST_HIT_NONE && !CtrPokeTouch_BackHit(&v, 278, 210));
    memset(v.box.slot, 0, sizeof(v.box.slot)); v.box.holding = 0; DevCheckTouch(&old, &v); for (i = 0; i < 30; ++i) AssertRowHits(BoxHitV, &v, DevBoxCell(i), i);
    v.box.area = ST_AREA_PARTY; v.box.pos = 2; DevCheckTouch(&old, &v); for (i = 0; i < 7; ++i) AssertRowHits(BoxHitV, &v, DevBoxPartyRow(i), ST_HIT_PARTY_BASE + i);
    v.box.state = PT_BOX_STATE_MENU; v.box.menuCount = 7; DevCheckTouch(&old, &v); for (i = 0; i < 7; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(7, i), ST_HIT_MENU_BASE + i);
    AssertKey(DevBoxMenuKey(7, 6), DEV_LOOK_DARK, 0); assert(CtrPokeTouch_BoxHit(&v, 60, 60) == ST_HIT_NONE);
    v.box.state = PT_BOX_STATE_YESNO; DevCheckTouch(&old, &v); AssertHits(BoxHitV, &v, DevBoxMenuKey(2, 1), ST_HIT_MENU_BASE + 1);
    v.box.state = PT_BOX_STATE_CHOOSE; DevCheckTouch(&old, &v); AssertHits(BoxHitV, &v, kDevBoxChoose, ST_HIT_TITLE); AssertHits(BoxHitV, &v, kDevBoxNext, ST_HIT_NEXT);
    v.box.state = PT_BOX_STATE_MARKS; v.box.menuCount = 6; DevCheckTouch(&old, &v); for (i = 0; i < 6; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(6, i), ST_HIT_MENU_BASE + i);
    v.box.state = PT_BOX_STATE_MESSAGE; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BoxHit(&v, 5, 5) == PT_BOX_HIT_MESSAGE);
    v.box.state = PT_BOX_STATE_BOX; DevCheckTouch(&old, &v);
    v.box.box ^= 1; v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* the box: the cursor's frame (2 px, over the icons) on its place alone; the icons clipped to the paper; the places
     * of an empty box lighter on the second paper without a wallpaper; the wallpaper at 1:1 */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    r = DevBoxCell(29);
    assert(at(r.x + 23, r.y + 12) == DEV_BEZEL && at(r.x + 12, r.y) == DEV_BEZEL && at(r.x + 12, r.y + 23) == DEV_BEZEL);
    assert(at(r.x + 1, r.y + 12) == DEV_BEZEL && at(r.x + 2, r.y + 12) == STUB_ICON);
    r = DevBoxCell(28); assert(at(r.x, r.y + 12) == STUB_ICON && Count(r.x, r.y, r.w, r.h, DEV_BEZEL) == 0);
    v.box.pos = 0; DevCheckTouch(&old, &v); assert(at(DevBoxCell(0).x, DevBoxCell(0).y + 12) == DEV_BEZEL);
    assert(Count(DevBoxCell(29).x, DevBoxCell(29).y, 24, 24, DEV_BEZEL) == 0);
    monArt = 2; v.box.pos = 3; DevCheckTouch(&old, &v);       /* icons that fill their 32x32: the last row cut at the paper */
    assert(at(DevBoxCell(24).x, 157) == STUB_ICON && Count(8, 158, 172, 7, STUB_ICON) == 0 && Count(8, 8, 172, 6, STUB_ICON) == 0);
    assert(Count(188, 120, 124, 6, STUB_ICON) == 0 && Count(188, 158, 124, 6, STUB_ICON) == 0 && CountIn(kBoxHandPaper, STUB_ICON) > 0);
    monArt = 1;
    memset(v.box.slot, 0, sizeof(v.box.slot)); DevCheckTouch(&old, &v);
    r = DevBoxCell(10); assert(at(r.x, r.y) == DEV_PAPER2 && at(r.x + 1, r.y + 1) == DEV_PAPER && at(r.x + 22, r.y + 22) == DEV_PAPER);
    assert(at(15, 100) == DEV_PAPER2 && CountIn(kBoxPaper, STUB_ICON) == 0);
    wallArt = 1; sDevBoxWallId = -1; v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v);
    BoxWallpaperAt1To1(3);
    v.box.wallpaper = 9; DevCheckTouch(&old, &v); BoxWallpaperAt1To1(9);
    assert(Count(kBoxPaper.x, kBoxPaper.y, kBoxPaper.w, 24, DEV_WHITE) > 0);     /* FireRed's white name on the band */
    v.box.area = ST_AREA_TITLE; DevCheckTouch(&old, &v);       /* the band inverted, the name in the paper's colour */
    assert(at(14, 14) == DEV_BEZEL && at(173, 37) == DEV_BEZEL && Count(14, 14, 160, 24, DEV_PAPER) > 0);
    wallArt = 0; sDevBoxWallId = -1;
    v.box.wallpaper = 3; DevCheckTouch(&old, &v); assert(at(14, 14) == DEV_BEZEL && Count(14, 14, 160, 24, DEV_PAPER) > 0);
    v.box.area = ST_AREA_BOX; v.box.pos = 5; DevCheckTouch(&old, &v);
    assert(at(14, 14) == DEV_PAPER2 && Count(14, 14, 160, 24, DEV_INK) > 0 && Count(14, 14, 160, 24, DEV_BEZEL) == 0);

    /* the right column: ◀ ▶ sunk while held; PARTY and CLOSE BOX sunk by FireRed's cursor on its buttons or a held touch,
     * which takes the cursor's marks (the frame, the band, a key) away from the rest of the box */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0); AssertKey(kDevBoxNext, DEV_LOOK_KEY, 0); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0); AssertKey(kDevBoxClose, DEV_LOOK_DARK, 0);
    v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 1); AssertKey(kDevBoxClose, DEV_LOOK_DARK, 0);
    v.box.pos = 1; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0); AssertKey(kDevBoxClose, DEV_LOOK_DARK, 1);
    v.box.pressed = ST_HIT_BUTTON_PARTY; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 1); AssertKey(kDevBoxClose, DEV_LOOK_DARK, 0);
    v.box.pressed = ST_HIT_PREV; DevCheckTouch(&old, &v); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 1); AssertKey(kDevBoxClose, DEV_LOOK_DARK, 0);
    v.box.pressed = ST_HIT_NEXT; DevCheckTouch(&old, &v); AssertKey(kDevBoxNext, DEV_LOOK_KEY, 1); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0);
    v.box.area = ST_AREA_BOX; v.box.pos = 29; v.box.pressed = 4; DevCheckTouch(&old, &v);   /* a held place takes the frame */
    assert(at(DevBoxCell(4).x, DevBoxCell(4).y + 12) == DEV_BEZEL && at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == STUB_ICON);
    v.box.pressed = ST_HIT_TITLE; DevCheckTouch(&old, &v);
    assert(at(14, 14) == DEV_BEZEL && Count(DevBoxCell(29).x, DevBoxCell(29).y, 24, 24, DEV_BEZEL) == 0);
    v.box.pressed = ST_HIT_BUTTON_CLOSE; v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v);
    AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0); AssertKey(kDevBoxClose, DEV_LOOK_DARK, 1);
    v.box.pressed = PT_BOX_HIT_MESSAGE; assert(DevCheckTouch(&old, &v).count != 0); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 1);
    v.box.pressed = ST_HIT_MENU_BASE; assert(DevCheckTouch(&old, &v).count == 0);   /* nothing of the box's: drawn as none */

    /* the hand's screen: its word small and the Pokemon in hand, or none; MOVE ITEMS: the item in hand, and on each place
     * and Party row whose Pokemon holds one a blue square at its lower right */
    v = BoxBoard(); DevCheckTouch(&old, &v); assert(CountIn(kBoxHandPaper, DEV_INK2) > 0 && CountIn(kBoxHandPaper, STUB_ICON) == 24 * 24);
    v.box.holding = 0; DevCheckTouch(&old, &v); assert(CountIn(kBoxHandPaper, DEV_INK2) > 0 && CountIn(kBoxHandPaper, STUB_ICON) == 0);
    v.box.held.species = 3; assert(DevCheckTouch(&old, &v).count == 0);       /* not in hand: not drawn */
    v.box.items = 1; v.box.handItem = 1; v.box.slotItem[0] = 5; v.box.slotItem[7] = 6; v.box.slotItem[8] = 7; v.box.slot[8].species = 0;
    DevCheckTouch(&old, &v);
    assert(CountIn(kBoxHandPaper, GbaColor(31)) == 24 * 24 && CountIn(kBoxHandPaper, STUB_ICON) == 0);
    for (i = 0; i < 9; ++i) {
        r = DevBoxCell(i);
        assert(Count(r.x + 16, r.y + 16, 6, 6, DEV_BLUE_INK) == (i == 0 || i == 7 ? 36 : 0));
    }
    v.box.area = ST_AREA_PARTY; v.box.pos = 0; v.box.partyItem[3] = 9; DevCheckTouch(&old, &v);
    for (i = 0; i < 6; ++i) { r = DevBoxPartyRow(i); assert((Count(r.x, r.y, r.w, r.h, DEV_BLUE_INK) == 36) == (i == 3)); }
    v.box.items = 0; v.box.area = ST_AREA_BOX; DevCheckTouch(&old, &v);
    assert(CountIn(kBoxPaper, DEV_BLUE_INK) == 0 && CountIn(kBoxHandPaper, GbaColor(31)) == 0);

    /* the note: the cursor's Pokemon (its nickname, the gender's sign and level, the species line, the item), MOVE ITEMS'
     * item and its description, or the box's name; a message (its ▼ while it waits for A) */
    v = BoxBoard(); DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_INK) > 0 && CountIn(kBoxNotePaper, DEV_INK2) > 0 && CountIn(kBoxNotePaper, DEV_MALE) > 0 && CountIn(kBoxNotePaper, DEV_FEMALE) == 0);
    BoxMonText(&v.box, "Kitmon", "Kitmon", 0xb6, 12, "Leaf charm"); DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_FEMALE) > 0 && CountIn(kBoxNotePaper, DEV_MALE) == 0);
    BoxMonText(&v.box, "Kitmon", "Kitmon", 0x00, 12, ""); DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_FEMALE) == 0 && CountIn(kBoxNotePaper, DEV_MALE) == 0);
    v.box.items = 1; v.box.monItem = 2; NameCodes(v.box.itemName, (int)sizeof(v.box.itemName), "Leaf charm");
    NameCodes(v.box.itemDesc, (int)sizeof(v.box.itemDesc), "Port words for this test in place of an item's.");
    DevCheckTouch(&old, &v); assert(CountIn(kBoxNotePaper, GbaColor(31 << 5)) == 24 * 24 && CountIn(kBoxNotePaper, DEV_INK) > 0);
    v.box.monItem = 0; DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, GbaColor(31 << 5)) == 0 && CountIn(kBoxNotePaper, DEV_INK) > 0 && CountIn(kBoxNotePaper, DEV_INK2) > 0);
    v.box.items = 0; v.box.mon.species = 0; DevCheckTouch(&old, &v); assert(CountIn(kBoxNotePaper, DEV_INK) > 0);
    memset(v.box.boxName, 0xff, sizeof(v.box.boxName)); DevCheckTouch(&old, &v); assert(CountIn(kBoxNotePaper, DEV_INK) == 0);
    BoxCodes(v.box.boxName, (int)sizeof(v.box.boxName), "Box one", 0); DevCheckTouch(&old, &v);
    BoxMonText(&v.box, "Kit", "Kitmon", 0xb5, 3, ""); assert(DevCheckTouch(&old, &v).count == 0);   /* no Pokemon: not drawn */
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port words for this test in place of the box's message."); DevCheckTouch(&old, &v);
    assert(CountIn(kBoxNotePaper, DEV_INK) > 0 && Count(286, 210, 20, 15, DEV_INK2) == 0);
    v.box.state = PT_BOX_STATE_MESSAGE; DevCheckTouch(&old, &v); assert(Count(286, 210, 20, 15, DEV_INK2) > 0);

    /* a message waiting for A: a tap anywhere is its A; the box's screen and its keys grey, the hand as it is */
    assert(CtrPokeTouch_BoxHit(&v, 319, 239) == PT_BOX_HIT_MESSAGE && CtrPokeTouch_BoxHit(&v, DevBoxCell(3).x + 5, DevBoxCell(3).y + 5) == PT_BOX_HIT_MESSAGE);
    assert(at(15, 100) == BOX_DIM(DEV_PAPER2) && at(kDevBoxParty.x + 3, kDevBoxParty.y + 16) == BOX_DIM(DEV_KEY));
    assert(at(kDevBoxClose.x + 3, kDevBoxClose.y + 16) == BOX_DIM(DEV_BEZEL) && at(kDevBoxPrev.x + 3, kDevBoxPrev.y + 12) == BOX_DIM(DEV_KEY));
    assert(at(kBoxHandPaper.x + 1, kBoxHandPaper.y + 1) == DEV_PAPER);
    v.box.pressed = ST_HIT_BUTTON_PARTY; assert(DevCheckTouch(&old, &v).count == 0);   /* nothing sinks under a message */

    /* while FireRed is busy: the box kept, no cursor's frame, nothing hit */
    v = BoxBoard(); v.box.state = PT_BOX_STATE_BUSY; DevCheckTouch(&old, &v);
    assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == STUB_ICON && CountIn(kBoxPaper, DEV_BEZEL) == 0);
    assert(CtrPokeTouch_BoxHit(&v, DevBoxCell(3).x + 5, DevBoxCell(3).y + 5) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 200, 50) == ST_HIT_NONE);
    v.box.area = ST_AREA_BUTTONS; v.box.pos = 0; DevCheckTouch(&old, &v); AssertKey(kDevBoxParty, DEV_LOOK_KEY, 0);
    assert(CountIn(kBoxHandPaper, STUB_ICON) > 0);

    /* the Party on the box's paper: a row for each slot (half icon, nickname, level) and CANCEL, the cursor's inverted or
     * the one under a held touch; the box's keys grey and not hit, nothing of the box hit */
    v = BoxBoard(); v.box.area = ST_AREA_PARTY; v.box.pos = 2; DevCheckTouch(&old, &v);
    for (i = 0; i < 7; ++i) { r = DevBoxPartyRow(i); assert(at(r.x + 1, r.y + 1) == (i == 2 ? DEV_BEZEL : DEV_PAPER)); }
    r = DevBoxPartyRow(0); assert(Count(r.x + 2, r.y + 2, 16, 16, STUB_ICON) == 12 * 12 && Count(r.x, r.y, r.w, r.h, DEV_INK) > 0);
    r = DevBoxPartyRow(6); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) > 0 && Count(r.x, r.y, r.w, r.h, STUB_ICON) == 0);
    assert(at(kDevBoxParty.x + 3, kDevBoxParty.y + 16) == BOX_DIM(DEV_KEY) && at(kDevBoxClose.x + 3, kDevBoxClose.y + 16) == BOX_DIM(DEV_BEZEL));
    assert(at(kDevBoxNext.x + 3, kDevBoxNext.y + 12) == BOX_DIM(DEV_KEY));
    assert(CtrPokeTouch_BoxHit(&v, kDevBoxParty.x + 5, kDevBoxParty.y + 5) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, kDevBoxPrev.x + 5, kDevBoxPrev.y + 5) == ST_HIT_NONE);
    assert(CtrPokeTouch_BoxHit(&v, kDevBoxClose.x + 5, kDevBoxClose.y + 5) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 14, 14) == ST_HIT_NONE);
    v.box.pressed = ST_HIT_PARTY_BASE + 4; DevCheckTouch(&old, &v);
    for (i = 0; i < 7; ++i) { r = DevBoxPartyRow(i); assert(at(r.x + 1, r.y + 1) == (i == 4 ? DEV_BEZEL : DEV_PAPER)); }
    v.box.pressed = ST_HIT_PREV; DevCheckTouch(&old, &v); r = DevBoxPartyRow(2); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    v.box.pressed = -1; v.box.party[5].species = 0; v.box.party[4].egg = 1; DevCheckTouch(&old, &v);
    r = DevBoxPartyRow(5); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) == 0 && Count(r.x, r.y, r.w, r.h, STUB_ICON) == 0);
    r = DevBoxPartyRow(4); assert(Count(r.x, r.y, r.w, r.h, DEV_INK) > 0);   /* an egg: its name, no level */

    /* a menu (five, seven): the box's screen grey, its note narrow, FireRed's rows as keys in the right column, the last
     * dark, FireRed's cursor's sunk or the one under a held touch; nothing of the box hit or drawn there */
    v = BoxBoard(); BoxMenu(&v, 5); v.box.menuCursor = 1; DevCheckTouch(&old, &v);
    for (i = 0; i < 5; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(5, i), ST_HIT_MENU_BASE + i);
    BoxKeysSunk(5, 1, 1);
    assert(at(15, 100) == BOX_DIM(DEV_PAPER2) && at(kBoxNarrowPaper.x + 1, kBoxNarrowPaper.y + 1) == DEV_PAPER && at(183, 200) == DEV_SHELL_RED);
    assert(CountIn(kBoxNarrowPaper, DEV_MALE) > 0 && Count(188, 0, 132, 240, STUB_ICON) == 0);
    for (i = 0; i < 30; ++i) { r = DevBoxCell(i); assert(CtrPokeTouch_BoxHit(&v, r.x + 12, r.y + 12) == ST_HIT_NONE); }
    assert(CtrPokeTouch_BoxHit(&v, 20, 20) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 100, 200) == ST_HIT_NONE);
    v.box.pressed = ST_HIT_MENU_BASE + 3; DevCheckTouch(&old, &v); BoxKeysSunk(5, 3, 1);
    assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == BOX_DIM(DEV_BEZEL));   /* a held key is the menu's alone */
    v.box.pressed = ST_HIT_MENU_BASE + 6; DevCheckTouch(&old, &v); BoxKeysSunk(5, 1, 1);   /* no such key: as none */
    v.box.pressed = 3; assert(DevCheckTouch(&old, &v).count == 0);
    assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == BOX_DIM(DEV_BEZEL));   /* the place the menu is for */
    v.box.pressed = -1; v.box.chooseCount = 9; v.box.marks = 3; assert(DevCheckTouch(&old, &v).count == 0);
    BoxMenu(&v, 7); v.box.menuCursor = 6; DevCheckTouch(&old, &v); BoxKeysSunk(7, 6, 1);
    for (i = 0; i < 7; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(7, i), ST_HIT_MENU_BASE + i);
    for (int n = 1; n <= PT_BOX_MENU_MAX; ++n) {
        BoxMenu(&v, n); v.box.menuCursor = 0; DevCheckTouch(&old, &v);
        for (i = 0; i < n; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(n, i), ST_HIT_MENU_BASE + i);
        BoxKeysSunk(n, 0, 1);
    }
    v.box.area = ST_AREA_PARTY; v.box.pos = 1; DevCheckTouch(&old, &v);   /* a Party Pokemon's menu: the Party grey */
    r = DevBoxPartyRow(1); assert(at(r.x + 1, r.y + 1) == BOX_DIM(DEV_BEZEL));

    /* YES / NO: both cream, FireRed's cursor sunk or the one held; the narrow note, the right column's rest the shell */
    v = BoxBoard(); v.box.state = PT_BOX_STATE_YESNO; v.box.menuCount = 2; v.box.menuCursor = 1;
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Port question for this test?"); DevCheckTouch(&old, &v);
    AssertHits(BoxHitV, &v, DevBoxMenuKey(2, 0), ST_HIT_MENU_BASE); BoxKeysSunk(2, 1, 0);
    assert(at(250, 140) == DEV_SHELL_RED && at(250, 200) == DEV_SHELL_RED && CountIn(kBoxNarrowPaper, DEV_INK) > 0);
    assert(CtrPokeTouch_BoxHit(&v, 250, 140) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 60, 60) == ST_HIT_NONE);
    v.box.pressed = ST_HIT_MENU_BASE; DevCheckTouch(&old, &v); BoxKeysSunk(2, 0, 0);
    v.box.pressed = ST_HIT_MENU_BASE + 2; DevCheckTouch(&old, &v); BoxKeysSunk(2, 1, 0);

    /* the chooser: ◀ ▶ (sunk while held) and the chosen box's key, always sunk: its name and how many it holds; the box's
     * screen grey and not hit; the note narrow and CANCEL beside it, the view's BACK (FireRed's B), a dark key hit where
     * it is drawn, sunk while held, grey while the chooser takes no B */
    v = BoxBoard(); v.box.state = PT_BOX_STATE_CHOOSE; BoxCodes(v.box.chooseName, (int)sizeof(v.box.chooseName), "Box two", 0);
    v.box.chooseCount = 12; DevCheckTouch(&old, &v);
    AssertHits(BoxHitV, &v, kDevBoxPrev, ST_HIT_PREV); AssertHits(BoxHitV, &v, kDevBoxNext, ST_HIT_NEXT); AssertHits(BoxHitV, &v, kDevBoxChoose, ST_HIT_TITLE);
    AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0); AssertKey(kDevBoxNext, DEV_LOOK_KEY, 0);
    assert(at(15, 100) == BOX_DIM(DEV_PAPER2) && Count(kDevBoxChoose.x, kDevBoxChoose.y, kDevBoxChoose.w, kDevBoxChoose.h, DEV_SUNK_INK) > 0);
    assert(CtrPokeTouch_BoxHit(&v, 60, 60) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 20, 20) == ST_HIT_NONE && CtrPokeTouch_BoxHit(&v, 250, 140) == ST_HIT_NONE);
    v.box.pressed = ST_HIT_PREV; DevCheckTouch(&old, &v); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 1); AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1);
    v.box.pressed = ST_HIT_TITLE; DevCheckTouch(&old, &v); AssertKey(kDevBoxPrev, DEV_LOOK_KEY, 0); AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1);
    other = v; other.box.chooseCount = 5; TouchSamePixels(&other, &v, 0, "how many the chosen box holds");   /* "5/30" */
    v.box.pressed = -1; NameCodes(v.box.message, (int)sizeof(v.box.message), "Port words: which box?"); DevCheckTouch(&old, &v);
    AssertHits(BackV, &v, kDevBoxCancel, 1); AssertKey(kDevBoxCancel, DEV_LOOK_DARK, 0);
    assert(CtrPokeTouch_BoxHit(&v, kDevBoxCancel.x + 5, kDevBoxCancel.y + 5) == ST_HIT_NONE && !CtrPokeTouch_BackHit(&v, 183, 200));
    assert(at(183, 200) == DEV_SHELL_RED && CountIn(kBoxNarrowPaper, DEV_INK) > 0 && CountIn(kBoxHandPaper, STUB_ICON) > 0);
    v.backPressed = 1; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevBoxCancel, DEV_LOOK_DARK, 1); AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1);
    v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(kDevBoxCancel.x + 3, kDevBoxCancel.y + kDevBoxCancel.h / 2) == DIM_DARK);
    v.backPressed = 0; v.backEnabled = 1; DevCheckTouch(&old, &v); AssertKey(kDevBoxCancel, DEV_LOOK_DARK, 0);
    BoxKeyInCapitals(&v, kDevBoxCancel, DEV_LOOK_DARK, 0, CtrUiText(UIS_CANCEL));
    CtrPokeTouch_Draw(canvas, &old);
    v.backVisible = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_BackHit(&v, kDevBoxCancel.x + 5, kDevBoxCancel.y + 5));
    assert(at(kDevBoxCancel.x + 3, kDevBoxCancel.y + kDevBoxCancel.h / 2) == DEV_SHELL_RED);
    v.backVisible = 1; DevCheckTouch(&old, &v);
    for (i = PT_BOX_STATE_BUSY; i <= PT_BOX_STATE_MARKS; ++i) {   /* CANCEL is the chooser's alone */
        v.box.state = (uint8_t)i; v.box.menuCount = 2; v.backPressed = 0; DevCheckTouch(&old, &v);
        assert(CtrPokeTouch_BackHit(&v, kDevBoxCancel.x + 5, kDevBoxCancel.y + 5) == (i == PT_BOX_STATE_CHOOSE));
        if (i != PT_BOX_STATE_CHOOSE) { v.backPressed = 1; assert(DevCheckTouch(&old, &v).count == 0); }
    }

    /* the marks: the four marks at 2x (on in the key's ink, off dim), OK, CANCEL dark; FireRed's cursor sunk */
    markArt = 1; v = BoxBoard(); v.box.state = PT_BOX_STATE_MARKS; v.box.menuCount = 6; v.box.marks = 5; v.box.menuCursor = 4;
    DevCheckTouch(&old, &v); BoxKeysSunk(6, 4, 1);
    for (i = 0; i < 4; ++i) {
        r = DevBoxMenuKey(6, i);
        assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, (v.box.marks >> i) & 1 ? DEV_KEY_INK : DEV_INK_DIM) == 12 * 12);
    }
    v.box.menuCursor = 1; DevCheckTouch(&old, &v); BoxKeysSunk(6, 1, 1);
    r = DevBoxMenuKey(6, 1); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_INK_DIM) == 12 * 12);
    v.box.pressed = ST_HIT_MENU_BASE + 5; DevCheckTouch(&old, &v); BoxKeysSunk(6, 5, 1);
    markArt = 0;

    /* no BACK but the chooser's (CLOSE BOX is FireRed's exit, every menu has its CANCEL): nothing hit where a view's BACK
     * is */
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
    v = BoxBoard(); v.box.holding = 0; DevCheckTouch(&old, &v); shot("full");
    memset(v.box.slot, 0, sizeof(v.box.slot)); v.box.mon.species = 0; v.box.pos = 8; DevCheckTouch(&old, &v); shot("empty");
    wallArt = 1; sDevBoxWallId = -1;
    v = BoxBoard(); for (i = 0; i < 30; i += 4) v.box.slot[i].species = 0;
    v.box.pos = 14; BoxMonText(&v.box, "Kit", "Kitmon", 0xb6, 45, "Leaf charm"); DevCheckTouch(&old, &v); shot("held");
    v.box.area = ST_AREA_TITLE; DevCheckTouch(&old, &v); shot("title");
    v.box.area = ST_AREA_BUTTONS; v.box.pos = 1; v.box.pressed = ST_HIT_NEXT; DevCheckTouch(&old, &v); shot("buttons");
    wallArt = 0; sDevBoxWallId = -1;
    v = BoxBoard(); v.box.area = ST_AREA_PARTY; v.box.pos = 3; v.box.party[5].species = 0; DevCheckTouch(&old, &v); shot("party");
    v = BoxBoard(); BoxMenu(&v, 5); v.box.menuCursor = 0;
    NameCodes(v.box.message, (int)sizeof(v.box.message), "Kitmon is selected."); DevCheckTouch(&old, &v); shot("menu5");
    BoxMenu(&v, 7); v.box.menuCursor = 2; DevCheckTouch(&old, &v); shot("menu7");
    v.box.state = PT_BOX_STATE_YESNO; v.box.menuCount = 2; v.box.menuCursor = 0;
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
