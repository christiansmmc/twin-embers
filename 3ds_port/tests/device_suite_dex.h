/* ---- dex: the Pokedex as the red device, through the field dispatcher (no board; plan 3 Task 3) -----------------------
 * Included by device_render_test.c before its kSuites. Also the harness's caught mark: CtrPokeTouch_DexCaughtMarker gives
 * an 8x8 tile in STUB_MARK while a suite sets dexArt (none otherwise, as the other art stubs). */

#define DEX_ART_MAGENTA 0x7c1f                  /* GBA BGR555 */
#define STUB_MARK GbaColor(DEX_ART_MAGENTA)
static int dexArt;

int CtrPokeTouch_DexCaughtMarker(unsigned char *tiles, unsigned short *palette)
{
    if (!dexArt) return 0;
    memset(tiles, 0x11, 32); memset(palette, 0, 32); palette[1] = DEX_ART_MAGENTA; return 1;
}

/* What the Pokedex hits under a point (CtrPokeTouch_DexHit). */
static int DexHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_DexHit((const CtrPokeTouchView *)view, x, y);
}

/* The fixture's rows read back: the row item key i shows on the habitats' first page (the rows after the second header),
 * and CLOSE's (FireRed's CANCEL). */
static int ItemRow(const CtrPokeTouchView *v, int i)
{
    for (int r = 0, headers = 0; r < v->dex.menuCount; ++r)
        if (v->dex.menu[r].id == PT_DEX_ID_HEADER && ++headers == 2)
            return r + 1 + i;
    return -1;
}

static int CancelRow(const CtrPokeTouchView *v)
{
    for (int r = 0; r < v->dex.menuCount; ++r)
        if (v->dex.menu[r].id == PT_DEX_ID_CANCEL)
            return r;
    return -1;
}

/* The Pokedex as 3ds_bottom.c gives it (FieldDexFill), words as FireRed writes them (the species names title-cased by
 * recipe 0251): the Kanto Pokedex's top menu (sListMenuItems_KantoDexModeSelect: the list, nine habitats, the sixth not
 * seen yet, four searches, CLOSE in its own section) with the cursor on the second habitat; the ordered list of 151 from
 * its first row (the third seen, not caught; the fourth not seen), Ivysaur selected, caught; a habitat's first page of
 * two with three Pokemon; Ivysaur's page. BACK shown and usable, nothing held. */
static CtrPokeTouchView DexBoard(void)
{
    static const struct { int8_t id; const char *name; } kRow[] = {
        {PT_DEX_ID_HEADER, "POK\xC3\xA9MON LIST"}, {PT_DEX_ID_KANTO, "NUMERICAL MODE"},
        {PT_DEX_ID_HEADER, "POK\xC3\xA9MON HABITATS"}, {0, "Grassland POK\xC3\xA9MON"}, {1, "Forest POK\xC3\xA9MON"},
        {2, "Water's-edge POK\xC3\xA9MON"}, {3, "Sea POK\xC3\xA9MON"}, {4, "Cave POK\xC3\xA9MON"}, {5, "Mountain POK\xC3\xA9MON"},
        {6, "Rough-terrain POK\xC3\xA9MON"}, {7, "Urban POK\xC3\xA9MON"}, {8, "Rare POK\xC3\xA9MON"},
        {PT_DEX_ID_HEADER, "SEARCH"}, {10, "A TO Z MODE"}, {11, "TYPE MODE"}, {12, "LIGHTEST MODE"}, {13, "SMALLEST MODE"},
        {PT_DEX_ID_HEADER, "OTHER"}, {PT_DEX_ID_CANCEL, "CLOSE POK\xC3\xA9" "DEX"},
    };
    static const struct { uint8_t flags; const char *name; } kList[PT_DEX_ROWS] = {
        {3, "Bulbasaur"}, {3, "Ivysaur"}, {1, "Venusaur"}, {0, ""}, {3, "Charmeleon"},
    };
    CtrPokeTouchView v;
    CtrPokeTouchDex *d = &v.dex;

    memset(&v, 0, sizeof(v));
    v.pressed = v.selected = PT_NONE;
    d->on = 1;
    d->state = PT_DEX_STATE_MENU;
    d->pressed = -1;
    d->seen[0] = 142; d->owned[0] = 118; d->seen[1] = 251; d->owned[1] = 203;
    d->menuCount = (uint8_t)(sizeof(kRow) / sizeof(kRow[0]));
    for (int i = 0; i < d->menuCount; ++i) {
        d->menu[i].id = kRow[i].id;
        strcpy(d->menu[i].name, kRow[i].name);
    }
    d->menu[8].locked = 1;                                  /* Mountain: the sixth habitat */
    d->menuCursor = 4;                                      /* Forest: the second */
    d->order = 0;
    d->listCount = 151;
    d->listFirst = 0;
    d->listCursor = 1;
    for (int i = 0; i < PT_DEX_ROWS; ++i) {
        d->row[i].species = d->row[i].number = (uint16_t)(1 + i);
        d->row[i].flags = kList[i].flags;
        strcpy(d->row[i].name, kList[i].name);
    }
    d->sel = d->row[1];
    d->type[0] = 12; d->type[1] = 3;                        /* GRASS, POISON */
    strcpy(d->typeName[0], "GRASS"); strcpy(d->typeName[1], "POISON");
    strcpy(d->kind, "SEED POK\xC3\xA9MON");
    d->height = 10; d->weight = 130;
    strcpy(d->desc, "There is a plant bud on its back. As it absorbs nutrients, the bud is said to bloom into a large "
                    "flower.");
    d->category = 0; d->page = 1; d->pages = 2; d->slots = 3; d->slot = 0; d->canNext = 1;
    d->mon[0] = d->row[0]; d->mon[1] = d->row[1]; d->mon[2] = d->row[3];
    snprintf(v.bar[0], PT_BAR_LEN, "Pok\xC3\xA9" "dex");          /* the bar: not drawn by the Pokedex */
    snprintf(v.bar[1], PT_BAR_LEN, "Tap to select, again to open.");
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* Upper case (owner decision 5): FireRed's menu words, species names, kinds and types, which v0.6.0 title-cased, drawn
 * in capitals; " POKeMON" and " MODE" cut from the menu's words in either case. */
static void DexUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;

    for (int r = 0; r < v.dex.menuCount; ++r)
        CtrTextCase(cased.dex.menu[r].name);                /* "A TO Z MODE" -> "A To Z Mode", "Forest POKeMON" kept */
    TouchSamePixels(&cased, &v, 1, "the habitats' words in v0.6.0's case and as FireRed writes them");
    v.dex.menuCursor = cased.dex.menuCursor = 13;
    TouchSamePixels(&cased, &v, 1, "the searches' words in v0.6.0's case and as FireRed writes them");
    v.dex.menuCursor = 4;
    cased = v;
    strcpy(cased.dex.menu[4].name, "FOREST");
    strcpy(cased.dex.menu[13].name, "A TO Z");
    v.dex.menuCursor = cased.dex.menuCursor = 13;
    TouchSamePixels(&cased, &v, 1, "A TO Z MODE and A TO Z");
    v.dex.menuCursor = cased.dex.menuCursor = 4;
    TouchSamePixels(&cased, &v, 1, "Forest POKeMON and FOREST");
    v.dex.state = PT_DEX_STATE_LIST;
    cased = v;
    for (int i = 0; i < PT_DEX_ROWS; ++i)
        DevUpper(cased.dex.row[i].name, v.dex.row[i].name, sizeof(cased.dex.row[i].name));
    CtrTextCase(cased.dex.kind);
    CtrTextCase(cased.dex.typeName[0]);
    CtrTextCase(cased.dex.typeName[1]);
    TouchSamePixels(&cased, &v, 1, "the list's names, the kind and the types in either case");
    v.dex.state = cased.dex.state = PT_DEX_STATE_PAGE;
    DevUpper(cased.dex.sel.name, v.dex.sel.name, sizeof(cased.dex.sel.name));
    TouchSamePixels(&cased, &v, 1, "a Pokemon's page in either case");
    strcpy(cased.dex.desc, "THERE IS A PLANT BUD ON ITS BACK.");
    strcpy(v.dex.desc, "There is a plant bud on its back.");
    TouchSamePixels(&cased, &v, 0, "the entry, as FireRed writes it");
}

/* The plan's suite (every key, row and slot hit where it is drawn; ▲ ▼ grey and not hit at the list's ends; the
 * seen-only row without its mark and its page without type chips; BACK where drawn), the review's checks (a touch
 * chooses, the menu's pages and tabs, CLOSE, what is not drawn, BACK's states, the ways in and out, upper case), then
 * the states for the eye. */
static void SuiteDex(void)
{
    CtrPokeTouchView v = DexBoard(), old = v, other;
    CtrPokeTouchRect r0, r2, r;
    int i;

    dexArt = 1;
    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v = DexBoard(); DevCheckTouch(&old, &v); assert(CtrPokeTouch_DeviceShown(&v));
    for (i = 0; i < 3; ++i) AssertHits(DexHitV, &v, kDevBagPocket[i], PT_DEX_HIT_TAB + i);
    AssertKey(kDevBagPocket[1], DEV_LOOK_KEY, 1);
    for (i = 0; i < 6; ++i) AssertHits(DexHitV, &v, DevDexItem(i), PT_DEX_HIT_ITEM + ItemRow(&v, i));
    AssertKey(DevDexItem(1), DEV_LOOK_KEY, 1); AssertKey(DevDexItem(5), DEV_LOOK_FAINT, 0);
    AssertHits(DexHitV, &v, kDevSmallKey[0], PT_DEX_HIT_UP); AssertHits(DexHitV, &v, kDevSmallKey[1], PT_DEX_HIT_DOWN); AssertHits(DexHitV, &v, kDevBagClose, PT_DEX_HIT_ITEM + CancelRow(&v)); assert(!CtrPokeTouch_BackHit(&v, 278, 210));
    v.dex.state = PT_DEX_STATE_LIST; DevCheckTouch(&old, &v); for (i = 0; i < 5; ++i) AssertRowHits(DexHitV, &v, DevTallRow(i, 0), PT_DEX_HIT_ROW + i);
    assert(CtrPokeTouch_DexHit(&v, 294, 73) == -1 && Count(276, 40, 36, 66, DEV_BAG_ARROW_OFF) > 0); AssertHits(DexHitV, &v, kDevBagDown, PT_DEX_HIT_DOWN); AssertHits(BackV, &v, kDevBagClose, 1);
    r0 = DevTallRow(0, 0); r2 = DevTallRow(2, 0); assert(Count(r0.x + r0.w - 14, r0.y, 14, r0.h, STUB_MARK) > 0 && Count(r2.x + r2.w - 14, r2.y, 14, r2.h, STUB_MARK) == 0);
    v.dex.listFirst = 146; v.dex.listCursor = 150; DevCheckTouch(&old, &v); assert(CtrPokeTouch_DexHit(&v, 294, 147) == -1); AssertHits(DexHitV, &v, kDevBagUp, PT_DEX_HIT_UP);
    v.dex.state = PT_DEX_STATE_CATEGORY; DevCheckTouch(&old, &v); for (i = 0; i < 3; ++i) AssertHits(DexHitV, &v, DevDexSlot(i), PT_DEX_HIT_SLOT + i);
    assert(CtrPokeTouch_DexHit(&v, 238, 150) == -1 && CtrPokeTouch_DexHit(&v, 184, 210) == -1); AssertHits(DexHitV, &v, kDevSmallKey[1], PT_DEX_HIT_NEXT);
    v.dex.canPrev = 1; DevCheckTouch(&old, &v); AssertHits(DexHitV, &v, kDevSmallKey[0], PT_DEX_HIT_PREV);
    v.dex.state = PT_DEX_STATE_PAGE; DevCheckTouch(&old, &v); AssertHits(DexHitV, &v, kDevDexArea, PT_DEX_HIT_AREA); AssertHits(DexHitV, &v, kDevDexCry, PT_DEX_HIT_CRY);
    v.dex.sel.flags = 1; DevCheckTouch(&old, &v); assert(Count(110, 56, 196, 14, UiTypeColor(v.dex.type[0])) == 0);        /* seen only: no type chips */
    v.dex.added = 1; DevCheckTouch(&old, &v); AssertHits(DexHitV, &v, kDevWideKey, PT_DEX_HIT_CONTINUE); assert(CtrPokeTouch_DexHit(&v, 160, 90) == PT_DEX_HIT_CONTINUE && !CtrPokeTouch_BackHit(&v, 278, 209));
    v.dex.national ^= 1; v.dex.category ^= 1; v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* MENU: HABITATS sunk with its LED, the cursor's habitat sunk, the sixth faint, the others up; ▲ grey on the first
     * page, ▼ dark; the counts in the small note (the words in the secondary ink); CLOSE dark */
    v = DexBoard(); DevCheckTouch(&old, &v);
    AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 0); AssertKey(kDevBagPocket[2], DEV_LOOK_KEY, 0);
    assert(Count(112, 8, 96, 26, DEV_LED) > 0 && Count(8, 8, 96, 26, DEV_LED) == 0 && Count(216, 8, 96, 26, DEV_LED) == 0);
    for (i = 0; i < 5; ++i)
        if (i != 1) AssertKey(DevDexItem(i), DEV_LOOK_KEY, 0);
    r = DevDexItem(1); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) > 0);
    r = DevDexItem(0); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) == 0 && Count(r.x, r.y + 52, r.w, 16, DEV_KEY_INK) > 0);
    r = DevDexItem(5); assert(Count(r.x, r.y + 52, r.w, 16, DEV_FAINT_INK) > 0);
    assert(Count(168, 190, 32, 40, DEV_BAG_ARROW_OFF) > 0 && Count(204, 190, 32, 40, DEV_KEY_INK) > 0);
    assert(Count(14, 196, 140, 30, DEV_INK2) > 0 && Count(14, 196, 140, 30, DEV_INK) > 0);
    AssertKey(kDevBagClose, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 40, DEV_DARK_INK) > 0 && Count(244, 190, 68, 43, DEV_LED) == 0);
    /* the counts are Kanto's (no NATIONAL row): the National ones are not drawn, Kanto's are, whatever `national` says;
     * with a NATIONAL row, the National ones */
    other = v; other.dex.seen[1] = 7; other.dex.owned[1] = 7; TouchSamePixels(&v, &other, 1, "the National counts without a NATIONAL row");
    other.dex.seen[0] = 7; TouchSamePixels(&v, &other, 0, "Kanto's seen count");
    CtrPokeTouch_Draw(canvas, &old);
    v.dex.seen[1] = 7; DevCheckTouch(&old, &v);                  /* (the stub font's digits are alike: another count of them) */
    v.dex.national = 1; assert(DevCheckTouch(&old, &v).count == 0);
    other = v; other.dex.menu[1].id = PT_DEX_ID_NATIONAL; other.dex.seen[1] = v.dex.seen[0]; other.dex.owned[1] = v.dex.owned[0];
    TouchSamePixels(&v, &other, 1, "the National counts (as many) with a NATIONAL row on another tab");
    other.dex.seen[1] = 7; TouchSamePixels(&v, &other, 0, "the National counts with a NATIONAL row");
    CtrPokeTouch_Draw(canvas, &old);
    v.dex.national = 0; DevCheckTouch(&old, &v);
    assert(CtrPokeTouch_DexGridPageRow(&v, 1) == ItemRow(&v, 6) && CtrPokeTouch_DexGridPageRow(&v, -1) == -1);
    /* a touch chooses: a tab, an item, ▼ (the cursor's item stays sunk); ▲ with no page before it does not sink */
    v.dex.pressed = PT_DEX_HIT_TAB + 2; DevCheckTouch(&old, &v); AssertKey(kDevBagPocket[2], DEV_LOOK_KEY, 1); AssertKey(kDevBagPocket[1], DEV_LOOK_KEY, 0); AssertKey(DevDexItem(1), DEV_LOOK_KEY, 1);
    v.dex.pressed = (int16_t)(PT_DEX_HIT_ITEM + ItemRow(&v, 3)); DevCheckTouch(&old, &v); AssertKey(DevDexItem(3), DEV_LOOK_KEY, 1); AssertKey(DevDexItem(1), DEV_LOOK_KEY, 0);
    v.dex.pressed = PT_DEX_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 1); AssertKey(DevDexItem(1), DEV_LOOK_KEY, 1);
    v.dex.pressed = PT_DEX_HIT_UP; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[0], DEV_LOOK_KEY, 0);
    v.dex.pressed = (int16_t)(PT_DEX_HIT_ITEM + CancelRow(&v)); DevCheckTouch(&old, &v); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1); AssertKey(DevDexItem(1), DEV_LOOK_KEY, 0);
    assert(Count(244, 190, 68, 43, DEV_LED) > 0);
    v.dex.pressed = -1; DevCheckTouch(&old, &v);
    /* the habitats' second page: three keys, the rest of the page empty; ▲ dark, ▼ grey; the pages a drag or ▲ ▼ go to */
    v.dex.menuCursor = (int8_t)ItemRow(&v, 7); DevCheckTouch(&old, &v);
    for (i = 0; i < 3; ++i) AssertHits(DexHitV, &v, DevDexItem(i), PT_DEX_HIT_ITEM + ItemRow(&v, 6 + i));
    AssertKey(DevDexItem(1), DEV_LOOK_KEY, 1);
    r = DevDexItem(3); assert(CtrPokeTouch_DexHit(&v, r.x + 10, r.y + 10) == -1 && at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED);
    assert(Count(168, 190, 32, 40, DEV_KEY_INK) > 0 && Count(204, 190, 32, 40, DEV_BAG_ARROW_OFF) > 0);
    assert(CtrPokeTouch_DexGridPageRow(&v, -1) == ItemRow(&v, 0) && CtrPokeTouch_DexGridPageRow(&v, 1) == -1);
    assert(CtrPokeTouch_DexTabRow(&v, 0) == 1 && CtrPokeTouch_DexTabRow(&v, 1) == ItemRow(&v, 0) && CtrPokeTouch_DexTabRow(&v, 2) == 13);
    /* SEARCH: its four keys, no pages; the cursor on CLOSE: the lists' one key, CLOSE sunk, no ▲ ▼ */
    v.dex.menuCursor = 14; DevCheckTouch(&old, &v); AssertKey(kDevBagPocket[2], DEV_LOOK_KEY, 1); AssertKey(DevDexItem(1), DEV_LOOK_KEY, 1);
    for (i = 0; i < 4; ++i) AssertHits(DexHitV, &v, DevDexItem(i), PT_DEX_HIT_ITEM + 13 + i);
    assert(CtrPokeTouch_DexHit(&v, 184, 210) == -1 && at(184, 210) == DEV_SHELL_RED && CtrPokeTouch_DexGridPageRow(&v, 1) == -1);
    v.dex.menuCursor = (int8_t)CancelRow(&v); DevCheckTouch(&old, &v);
    AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 1); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1); AssertKey(DevDexItem(0), DEV_LOOK_KEY, 0);
    assert(Count(244, 190, 68, 43, DEV_LED) > 0);                   /* CLOSE sunk: the cursor's */
    AssertHits(DexHitV, &v, DevDexItem(0), PT_DEX_HIT_ITEM + 1);
    r = DevDexItem(1); assert(CtrPokeTouch_DexHit(&v, r.x + 10, r.y + 10) == -1 && CtrPokeTouch_DexHit(&v, 184, 210) == -1 && at(220, 210) == DEV_SHELL_RED);
    assert(CtrPokeTouch_DexGridPageRow(&v, 1) == -1);

    /* LIST: the order on the title (SEEN / OWNED at its right for KANTO, none for A TO Z), the cursor's row inverted (or
     * the one under a held touch), the caught ones' marks; the selected one's types and kind, "?????" for one not caught;
     * ▲ grey and ▼ dark at the top; BACK dark, sunk, grey, gone */
    v = DexBoard(); v.dex.state = PT_DEX_STATE_LIST; DevCheckTouch(&old, &v);
    assert(Count(14, 12, 292, 16, DEV_INK) > 0 && Count(160, 12, 146, 16, DEV_INK2) > 0);
    r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL && Count(r.x, r.y, 60, r.h, DEV_PAPER) > 0);
    r = DevTallRow(0, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER && Count(r.x, r.y, 60, r.h, DEV_INK) > 0);
    for (i = 0; i < PT_DEX_ROWS; ++i) { r = DevTallRow(i, 0); assert((Count(r.x + r.w - 14, r.y, 14, r.h, STUB_MARK) > 0) == ((v.dex.row[i].flags & 2) != 0)); }
    assert(Count(14, 196, 216, 30, UiTypeColor(12)) > 0 && Count(14, 196, 216, 30, UiTypeColor(3)) > 0 && Count(14, 196, 216, 30, DEV_INK2) > 0);
    assert(Count(276, 114, 36, 66, DEV_KEY_INK) > 0); AssertKey(kDevBagClose, DEV_LOOK_DARK, 0);
    v.dex.pressed = PT_DEX_HIT_ROW + 3; DevCheckTouch(&old, &v);
    r = DevTallRow(3, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL); r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_PAPER);
    v.dex.pressed = PT_DEX_HIT_DOWN; DevCheckTouch(&old, &v); AssertKey(kDevBagDown, DEV_LOOK_KEY, 1); r = DevTallRow(1, 0); assert(at(r.x + 1, r.y + 1) == DEV_BEZEL);
    v.dex.pressed = -1; DevCheckTouch(&old, &v); v.dex.national = 1; assert(DevCheckTouch(&old, &v).count == 0);  /* the order says whose counts */
    v.dex.order = 1; DevCheckTouch(&old, &v); assert(Count(14, 12, 292, 16, DEV_INK2) == 0);
    v.dex.order = 0; v.dex.sel.flags = 1; DevCheckTouch(&old, &v);
    assert(Count(14, 196, 216, 30, UiTypeColor(12)) == 0 && Count(14, 196, 216, 30, DEV_INK) > 0);
    v.dex.sel.flags = 3;
    v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1);
    v.backPressed = 0; v.backEnabled = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    v.backEnabled = 1; v.backVisible = 0; DevCheckTouch(&old, &v); assert(at(247, 210) == DEV_SHELL_RED && !BackV(&v, 247, 210));
    v.backVisible = 1; DevCheckTouch(&old, &v);
    /* a list of three: no row past it is drawn or hit, ▲ ▼ grey */
    v.dex.listCount = 3; DevCheckTouch(&old, &v); r = DevTallRow(3, 0);
    assert(CtrPokeTouch_DexHit(&v, r.x + 10, r.y + 10) == -1 && Count(r.x, r.y, r.w, 2 * r.h, DEV_INK) == 0);
    assert(CtrPokeTouch_DexHit(&v, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_BAG_ARROW_OFF) > 0);

    /* CATEGORY: the habitat and the page on the title; the cursor's slot sunk (or the one under a held touch), the
     * unseen one's picture left out; no fourth slot; ◀ grey, ▶ dark; the selected one in the small note; BACK */
    v = DexBoard(); v.dex.state = PT_DEX_STATE_CATEGORY; DevCheckTouch(&old, &v);
    assert(Count(14, 12, 292, 16, DEV_INK) > 0);
    AssertKey(DevDexSlot(0), DEV_LOOK_KEY, 1); AssertKey(DevDexSlot(1), DEV_LOOK_KEY, 0); AssertKey(DevDexSlot(2), DEV_LOOK_KEY, 0);
    r = DevDexSlot(3); assert(at(r.x + 3, r.y + r.h / 2) == DEV_SHELL_RED);
    r = DevDexSlot(1); assert(Count(r.x + 70, r.y, r.w - 70, r.h, DEV_KEY_INK) > 0);
    assert(Count(168, 190, 32, 40, DEV_BAG_ARROW_OFF) > 0 && Count(204, 190, 32, 40, DEV_KEY_INK) > 0);
    assert(Count(14, 196, 140, 30, DEV_INK) > 0); AssertHits(BackV, &v, kDevBagClose, 1);
    v.dex.pressed = PT_DEX_HIT_SLOT + 2; DevCheckTouch(&old, &v); AssertKey(DevDexSlot(2), DEV_LOOK_KEY, 1); AssertKey(DevDexSlot(0), DEV_LOOK_KEY, 0);
    v.dex.pressed = PT_DEX_HIT_NEXT; DevCheckTouch(&old, &v); AssertKey(kDevSmallKey[1], DEV_LOOK_KEY, 1); AssertKey(DevDexSlot(0), DEV_LOOK_KEY, 1);
    v.dex.pressed = -1; DevCheckTouch(&old, &v);
    /* the habitat is the title's: another one draws whole; its name is the menu's, upper case, " POKeMON" left out */
    v.dex.category = 1; assert(DevWhole(DevCheckTouch(&old, &v)));
    other = v; strcpy(other.dex.menu[4].name, "FOREST"); TouchSamePixels(&v, &other, 1, "the habitat's name on the title");
    other.dex.category = 0; TouchSamePixels(&v, &other, 0, "another habitat on the title");
    CtrPokeTouch_Draw(canvas, &old);

    /* PAGE: the type chips of a caught one, the rule, the entry; AREA and CRY up (sunk under a touch), BACK in the
     * Summary's place; the page a catch added: OK sunk, nothing else hit, no BACK */
    v = DexBoard(); v.dex.state = PT_DEX_STATE_PAGE; DevCheckTouch(&old, &v);
    assert(Count(110, 56, 196, 14, UiTypeColor(12)) > 0 && Count(110, 56, 196, 14, UiTypeColor(3)) > 0);
    assert(Count(kDevDexRule.x, kDevDexRule.y, kDevDexRule.w, kDevDexRule.h, DEV_INK_SH) == kDevDexRule.w);
    assert(Count(20, 108, 280, 64, DEV_INK) > 0 && Count(110, 18, 168, 36, DEV_INK) > 0 && Count(110, 70, 120, 30, DEV_INK2) > 0);
    AssertKey(kDevDexArea, DEV_LOOK_KEY, 0); AssertKey(kDevDexCry, DEV_LOOK_KEY, 0); AssertKey(kDevSumBack, DEV_LOOK_DARK, 0);
    AssertHits(BackV, &v, kDevSumBack, 1); assert(CtrPokeTouch_DexHit(&v, 160, 90) == -1);
    v.dex.pressed = PT_DEX_HIT_AREA; DevCheckTouch(&old, &v); AssertKey(kDevDexArea, DEV_LOOK_KEY, 1); AssertKey(kDevDexCry, DEV_LOOK_KEY, 0);
    v.dex.pressed = PT_DEX_HIT_CRY; DevCheckTouch(&old, &v); AssertKey(kDevDexCry, DEV_LOOK_KEY, 1);
    v.dex.pressed = -1; v.dex.added = 1; DevCheckTouch(&old, &v); AssertKey(kDevWideKey, DEV_LOOK_KEY, 1);
    assert(!BackV(&v, 278, 209) && !BackV(&v, 247, 220) && at(247, 212) == DEV_SUNK && at(60, 212) == DEV_SUNK);
    v.dex.added = 0; DevCheckTouch(&old, &v);

    /* BUSY with nothing kept: the shell, nothing hit; what the Pokedex does not draw presents nothing; the shell's
     * colour whole; the ways in and out (HOME; the AREA page: FireRed's picture, no Pokedex) */
    v = DexBoard(); v.dex.state = PT_DEX_STATE_BUSY; assert(DevWhole(DevCheckTouch(&old, &v)));
    assert(CtrPokeTouch_DexHit(&v, 160, 100) == -1 && CtrPokeTouch_DexHit(&v, 20, 20) == -1 && at(160, 100) == DEV_SHELL_RED);
    v = DexBoard(); DevCheckTouch(&old, &v);
    strcpy(v.bar[0], "Anything"); v.bar[1][0] = 0; v.registeredItem = 7; v.enabled[PT_BAG] = 1; v.playerX = 3;
    assert(DevCheckTouch(&old, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&old, &v);
    other = v; other.dex.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)));
    v = DexBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    other = v; memset(&other.dex, 0, sizeof(other.dex)); other.viewport = 1;
    assert(DevWhole(DevCheckTouch(&old, &other)) && !CtrPokeTouch_DeviceShown(&other) && CtrPokeTouch_DexHit(&other, 160, 90) == -1);
    v = DexBoard(); v.viewport = 1; assert(!CtrPokeTouch_DeviceShown(&v) && CtrPokeTouch_DexHit(&v, 160, 20) == -1);
    v = DexBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    DexUpperCase(DexBoard());
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first. */
    v = DexBoard(); v.dex.menuCursor = 1; DevCheckTouch(&old, &v); shot("lists");
    v.dex.menuCursor = 4; DevCheckTouch(&old, &v); shot("habitats");
    v.dex.menuCursor = (int8_t)ItemRow(&v, 7); DevCheckTouch(&old, &v); shot("habitats_page2");
    v.dex.menuCursor = 14; DevCheckTouch(&old, &v); shot("search");
    v.dex.state = PT_DEX_STATE_LIST; DevCheckTouch(&old, &v); shot("list_top");
    v.dex.listFirst = 146; v.dex.listCursor = 150;
    for (i = 0; i < PT_DEX_ROWS; ++i) {
        static const char *const kEnd[PT_DEX_ROWS] = {"Dratini", "Dragonair", "Dragonite", "Mewtwo", "Mew"};
        v.dex.row[i].species = v.dex.row[i].number = (uint16_t)(147 + i);
        v.dex.row[i].flags = (uint8_t)(i < 3 ? 3 : i == 3 ? 1 : 0);
        strcpy(v.dex.row[i].name, i < 4 ? kEnd[i] : "");
    }
    v.dex.sel = v.dex.row[4];
    DevCheckTouch(&old, &v); shot("list_end");
    v = DexBoard(); v.dex.state = PT_DEX_STATE_CATEGORY; DevCheckTouch(&old, &v); shot("category");
    v.dex.state = PT_DEX_STATE_PAGE; DevCheckTouch(&old, &v); shot("page");
    v.dex.sel.flags = 1; DevCheckTouch(&old, &v); shot("page_seen");
    v.dex.sel.flags = 3; v.dex.added = 1; DevCheckTouch(&old, &v); shot("added");
    dexArt = 0;
    assert(sDevKeyCut == 0);                      /* main prints PASS device dex */
}
