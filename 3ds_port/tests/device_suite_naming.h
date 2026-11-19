/* ---- naming: the naming keyboard as the red device, through the field dispatcher (board def_naming) ---------------
 * Included by device_render_test.c before its kSuites. (The Pokemon's icon is the harness's CtrPokeTouch_MonIcon, in
 * STUB_ICON while a suite sets monArt.) */

/* The name's screen's paper (kDevNameScreen 6 px in) and, on it, the rows of the characters' capitals (from NAME_TOP,
 * UI_TEXT_H) and of the underlines (NAME_LINE); the slots (DevNameSlot) from x 54 with or without an icon, each 14 wide
 * and 16 apart; the question's and the counter's capitals (NAME_SMALL_TOP). */
static const CtrPokeTouchRect kNamePaper = {14, 14, 292, 28};
#define NAME_TOP 28
#define NAME_LINE 39
#define NAME_SMALL_TOP 17
#define NAME_SLOT(i) (DevNameSlot(i).x)

static CtrPokeTouchView gNameOld;

static int NameHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_NameHit((const CtrPokeTouchView *)view, x, y);
}

/* `ascii` as FireRed's codes (capitals, small letters, digits, the space, ? . , ! - /), into out[size], the rest 0xff. */
static void NameCodes(uint8_t *out, int size, const char *ascii)
{
    memset(out, 0xff, (size_t)size);
    for (int i = 0; ascii[i] && i < size; ++i) {
        char c = ascii[i];

        out[i] = (uint8_t)(c >= 'A' && c <= 'Z' ? 0xbb + (c - 'A') : c >= 'a' && c <= 'z' ? 0xd5 + (c - 'a')
                         : c >= '0' && c <= '9' ? 0xa1 + (c - '0') : c == '?' ? 0xac : c == '.' ? 0xad : c == ','
                         ? 0xb8 : c == '!' ? 0xab : c == '-' ? 0xae : c == '/' ? 0xba : 0x00);
    }
}

/* FireRed's page `page` (0 the symbols, 1 the capitals, 2 the small letters) as CtrBottomNaming_Key gives each place of
 * it: sKeyboardChars, "ABCDEF ." "GHIJKL ," "MNOPQRS" "TUVWXYZ" in 8 columns, the small letters the same, the symbols in
 * 6 ("01234" "56789" "!?♂♀/-" "…“”‘'"). Its rows are __() strings, which preproc writes without a terminator, and C
 * fills the rest of each row of 8 with 0x00, FireRed's space: a short row ends in space keys (column 7 of the letters'
 * last two rows, column 5 of the symbols' rows 0, 1 and 3), which FireRed's cursor reaches and its A types. Past a
 * page's columns, EOS (0xff). */
static void NameFill(CtrPokeTouchView *v, uint8_t page)
{
    static const char *const kLetters[PT_NAME_ROWS] = {"ABCDEF .", "GHIJKL ,", "MNOPQRS", "TUVWXYZ"};
    static const uint8_t kSymbols[PT_NAME_ROWS][6] = {
        {0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0x00}, {0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0x00},
        {0xab, 0xac, 0xb5, 0xb6, 0xba, 0xae}, {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0x00},
    };
    CtrPokeTouchName *n = &v->name;

    n->page = page;
    n->cols = page == 0 ? 6 : 8;
    memset(n->key, 0xff, sizeof(n->key));
    for (int r = 0; r < PT_NAME_ROWS; ++r) {
        if (page == 0) {
            memcpy(n->key[r], kSymbols[r], sizeof(kSymbols[r]));
            continue;
        }
        NameCodes(n->key[r], PT_NAME_COLS, kLetters[r]);
        for (int c = 0; c < PT_NAME_COLS; ++c)
            if (n->key[r][c] == 0xff)
                n->key[r][c] = 0x00;                /* the row's zero fill: a space */
            else if (page == 2 && n->key[r][c] >= 0xbb && n->key[r][c] <= 0xd4)
                n->key[r][c] += 0x1a;               /* A..Z -> a..z */
    }
}

/* The keyboard as 3ds_bottom.c gives it (FieldNameFill): the capitals' page, a name of 7 with three typed (the board's
 * RED), a port-written question, the cursor on the first key, nothing held, no Pokemon, no BACK (DELETE is its B). */
static CtrPokeTouchView NameBoard(void)
{
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.name.on = 1;
    v.name.maxChars = 7;
    v.name.pressed = -1;
    NameFill(&v, 1);
    NameCodes(v.name.text, PT_NAME_MAX, "RED");
    v.name.len = 3;
    NameCodes(v.name.title, (int)sizeof(v.name.title), "Name for this test?");
    snprintf(v.bar[0], PT_BAR_LEN, "Name");                     /* the bar: not drawn by the keyboard */
    return v;
}

/* Review Focus 1: on each of FireRed's pages every key it has is hit where it is drawn (every place of its columns: the
 * space keys that end its short rows too), the three page keys turn to their pages, DELETE and OK are hit, and the page
 * shown is sunk; then the capitals' page again with one place without a key (EOS, synthetic: FireRed's pages have none
 * inside their columns), neither drawn nor hit. */
static void NameEveryKey(CtrPokeTouchView v)
{
    static const uint8_t page[4] = {1, 2, 0, 1};
    int bare = 0;

    for (int p = 0; p < 4; ++p) {
        NameFill(&v, page[p]); if (p == 3) v.name.key[2][7] = 0xff; DevCheckTouch(&gNameOld, &v);
        for (int r = 0; r < 4; ++r) for (int c = 0; c < v.name.cols; ++c) {
            CtrPokeTouchRect k = DevNameKey(v.name.cols, r, c);
            if (v.name.key[r][c] == 0xff) { assert(CtrPokeTouch_NameHit(&v, k.x + k.w / 2, k.y + k.h / 2) == -1 && at(k.x + 3, k.y + k.h / 2) == DEV_SHELL_RED); ++bare; }
            else AssertHits(NameHitV, &v, k, PT_NAME_HIT_KEY + r * PT_NAME_COLS + c);
        }
        for (int t = 0; t < 3; ++t) AssertHits(NameHitV, &v, DevNameTop(t), PT_NAME_HIT_TAB + page[t]);
        AssertHits(NameHitV, &v, DevNameTop(3), PT_NAME_HIT_DELETE); AssertHits(NameHitV, &v, DevNameTop(4), PT_NAME_HIT_OK); AssertKey(DevNameTop((page[p] + 2) % 3), DEV_LOOK_KEY, 1);
    }
    assert(bare == 1);
}

/* Every key of the page drawn up but `key` (row * PT_NAME_COLS + column; -1 none), sunk; DELETE and OK up but `button`
 * (PT_NAME_HIT_DELETE or _OK; -1 none). */
static void NameSunk(const CtrPokeTouchView *v, int key, int button)
{
    for (int r = 0; r < PT_NAME_ROWS; ++r)
        for (int c = 0; c < v->name.cols; ++c)
            if (v->name.key[r][c] != 0xff)
                AssertKey(DevNameKey(v->name.cols, r, c), DEV_LOOK_KEY, r * PT_NAME_COLS + c == key);
    AssertKey(DevNameTop(3), DEV_LOOK_KEY, button == PT_NAME_HIT_DELETE);
    AssertKey(DevNameTop(4), DEV_LOOK_KEY, button == PT_NAME_HIT_OK);
}

/* The canvas's pixels in key r's face less its corners and edges, as `drawn` has them. */
static int NameFaceAs(const uint16_t *drawn, CtrPokeTouchRect r)
{
    for (int x = r.x + 6; x < r.x + r.w - 6; ++x)
        for (int y = r.y + 2; y < r.y + r.h - 2; ++y)
            if (drawn[x * H + H - 1 - y] != at(x, y))
                return 0;
    return 1;
}

/* DELETE, a port word, in capitals (decision 5): with small letters wider than capitals (as TouchSamePixels has them)
 * its face is the key drawn again, up, with the word in capitals, and (where the port writes it with small letters) not
 * with the word as written. */
static void NameDeleteInCapitals(const CtrPokeTouchView *v)
{
    static uint16_t drawn[W * H];
    uint8_t keep[256];
    char upper[24];
    const char *raw = CtrUiText(UIS_NAME_DELETE);
    CtrPokeTouchRect r = DevNameTop(3);
    int faceY;

    memcpy(keep, fontWidths, sizeof(keep));
    for (unsigned c = 0xd5; c <= 0xee; ++c)        /* a..z */
        fontWidths[c] = 7;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    DevUpper(upper, raw, sizeof(upper));
    CtrPokeTouch_Draw(canvas, v);
    memcpy(drawn, canvas, sizeof(drawn));
    faceY = DevKey(&r, DEV_LOOK_KEY, 0, 6);
    DevKeyWord(&r, faceY, DEV_LOOK_KEY, 0, upper);
    assert(NameFaceAs(drawn, r));
    faceY = DevKey(&r, DEV_LOOK_KEY, 0, 6);
    DevKeyWord(&r, faceY, DEV_LOOK_KEY, 0, raw);
    assert(NameFaceAs(drawn, r) == !strcmp(raw, upper));
    memcpy(fontWidths, keep, sizeof(keep));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
}

/* Words (decision 5): the name as typed, FireRed's question and characters as FireRed has them (each drawn differently
 * in the other case), DELETE in capitals. */
static void NameWords(CtrPokeTouchView v)
{
    CtrPokeTouchView cased = v;
    int typed = 0;

    /* each character has its own slot, so a wider small letter moves nothing (TouchSamePixels cannot tell): the trace
     * says what each slot's text is, FireRed's code as typed (Red's e, 0xd9, not E's 0xbf) */
    NameCodes(cased.name.text, PT_NAME_MAX, "Red");
    CtrPokeTouch_Draw(canvas, &cased);
    for (int i = 0; i < gTraceCount; ++i)
        if (gTrace[i].op == DEV_TRACE_TEXT && gTrace[i].x == DevNameSlot(1).x + 2 && gTrace[i].y == NAME_TOP)
            typed += !strcmp(gTrace[i].s, "\x01\xd9") ? 1 : 100;
    assert(typed == 1);
    cased = v;
    NameCodes(cased.name.title, (int)sizeof(cased.name.title), "NAME FOR THIS TEST?");
    TouchSamePixels(&cased, &v, 0, "the question, as FireRed writes it");
    cased = v;
    NameFill(&cased, 2);
    cased.name.page = v.name.page;
    TouchSamePixels(&cased, &v, 0, "the keys, FireRed's own characters");
    NameDeleteInCapitals(&v);
}

/* The plan's suite (every key of the three pages hit where drawn, the cursor on a key, DELETE, OK and the page button,
 * a held key, an empty and a full name, a gender sign; what the keyboard does not draw presents nothing), then the
 * screen's pieces, the words, the ways in and out and the states for the eye. */
static void SuiteNaming(void)
{
    static const int kPageKey[3] = {2, 0, 1};   /* the page key of FireRed's page 0, 1, 2 */
    CtrPokeTouchView v = NameBoard(), other;
    CtrPokeTouchRect r;
    uint8_t keepW;
    int i, q;

    gNameOld = v;
    CtrPokeTouch_Draw(canvas, &gNameOld);
    /* the plan's suite */
    v = NameBoard(); DevCheckTouch(&gNameOld, &v); assert(DrawnBy(&v, DevNameShown));
    NameEveryKey(v);
    v = NameBoard(); v.name.curCol = 3; v.name.curRow = 2; DevCheckTouch(&gNameOld, &v); NameSunk(&v, 2 * PT_NAME_COLS + 3, -1);
    v.name.curCol = (int8_t)v.name.cols; v.name.curRow = 1; DevCheckTouch(&gNameOld, &v); NameSunk(&v, -1, PT_NAME_HIT_DELETE);
    v.name.curRow = 2; DevCheckTouch(&gNameOld, &v); NameSunk(&v, -1, PT_NAME_HIT_OK);
    for (q = 0; q < 3; ++q) {                   /* the page button: the key of the page its A turns to lit, up */
        NameFill(&v, (uint8_t)q); v.name.curCol = (int8_t)v.name.cols; v.name.curRow = 0; DevCheckTouch(&gNameOld, &v);
        r = DevNameTop(kPageKey[(q + 1) % 3]);
        assert(at(r.x + r.w - 9, r.y + 6) == DEV_LED); AssertKey(r, DEV_LOOK_KEY, 0);
        AssertKey(DevNameTop(kPageKey[q]), DEV_LOOK_KEY, 1); NameSunk(&v, -1, -1);
        r = DevNameTop(kPageKey[(q + 2) % 3]); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) == 0);
    }
    v = NameBoard(); v.name.curCol = 3; v.name.curRow = 2; v.name.pressed = PT_NAME_HIT_KEY + 1 * PT_NAME_COLS + 5;
    DevCheckTouch(&gNameOld, &v); NameSunk(&v, 1 * PT_NAME_COLS + 5, -1);
    v.name.pressed = -1; v.name.len = 0; memset(v.name.text, 0xff, sizeof(v.name.text)); DevCheckTouch(&gNameOld, &v);
    assert(Count(NAME_SLOT(0), NAME_LINE, 14, 1, DEV_MARK_ON) == 14 && Count(NAME_SLOT(0), NAME_TOP, 14, UI_TEXT_H, DEV_TITLE_INK) == 0);
    for (i = 1; i < 7; ++i) assert(Count(NAME_SLOT(i), NAME_LINE, 14, 1, DEV_PAPER_LINE) == 14);
    assert(Count(NAME_SLOT(7), NAME_LINE, 306 - NAME_SLOT(7), 1, DEV_PAPER_LINE) == 0 && CountIn(kNamePaper, DEV_TITLE_INK) == 0);
    v.name.maxChars = 10; NameCodes(v.name.text, PT_NAME_MAX, "Kitkatkitk"); v.name.len = 10; DevCheckTouch(&gNameOld, &v);
    assert(Count(kNamePaper.x, NAME_LINE, kNamePaper.w, 1, DEV_MARK_ON) == 0 && Count(kNamePaper.x, NAME_LINE, kNamePaper.w, 1, DEV_PAPER_LINE) == 10 * 14);
    for (i = 0; i < 10; ++i) assert(Count(NAME_SLOT(i), NAME_TOP, 14, UI_TEXT_H, DEV_TITLE_INK) > 0);
    /* "10 / 10": from x 262 on, ending at 298 */
    assert(Count(262, NAME_SMALL_TOP, 37, 7, DEV_INK2) > 0 && Count(299, 14, 7, 28, DEV_INK2) == 0 && Count(200, 14, 62, 28, DEV_INK2) == 0);
    v = NameBoard(); v.name.gender = 1; DevCheckTouch(&gNameOld, &v);
    assert(Count(NAME_SLOT(7) + 2, 27, 8, 10, DEV_MALE) == CountIn(kNamePaper, DEV_MALE) && CountIn(kNamePaper, DEV_MALE) > 0 && CountIn(kNamePaper, DEV_FEMALE) == 0);
    v.name.gender = 2; DevCheckTouch(&gNameOld, &v); assert(Count(NAME_SLOT(7) + 2, 27, 8, 10, DEV_FEMALE) > 0 && CountIn(kNamePaper, DEV_MALE) == 0);
    v.name.gender = 0; DevCheckTouch(&gNameOld, &v); assert(CountIn(kNamePaper, DEV_MALE) == 0 && CountIn(kNamePaper, DEV_FEMALE) == 0);
    v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&gNameOld, &v).count == 0);

    /* the name strip (def_naming): every slot's underline in the paper's line, the next to type's in the mark's blue; a
     * typed letter in the title's ink from its slot's x + 2; FireRed's question from 52 and the counter "3 / 7" ending
     * at 298 (capitals 17); a full name no blue; a Pokemon's icon (a whole 32x32 square) 1:1 from (16, 10) clipped to its
     * 24x24 box, the question and the slots where they were */
    v = NameBoard(); DevCheckTouch(&gNameOld, &v);
    for (i = 0; i < 7; ++i) assert(Count(NAME_SLOT(i), NAME_LINE, 14, 1, i == 3 ? DEV_MARK_ON : DEV_PAPER_LINE) == 14);
    assert(Count(NAME_SLOT(0) + 2, 28, 12, 9, DEV_TITLE_INK) > 0 && Count(52, 17, 50, 7, DEV_INK2) > 0);
    assert(Count(NAME_SLOT(0), 14, 2, 28, DEV_TITLE_INK) == 0 && Count(51, 14, 1, 12, DEV_INK2) == 0);
    assert(Count(262, 17, 37, 7, DEV_INK2) > 0 && Count(299, 14, 7, 28, DEV_INK2) == 0);                  /* "3 / 7" */
    assert(CountIn(kDevNameIcon, DEV_PAPER) == 24 * 24);                                                    /* no icon */
    v.name.len = 7; DevCheckTouch(&gNameOld, &v); assert(Count(14, 39, 292, 1, DEV_MARK_ON) == 0);          /* full */
    monArt = 2; v.name.species = 4; DevCheckTouch(&gNameOld, &v);                                          /* a whole 32x32 square */
    assert(CountIn(kDevNameIcon, STUB_ICON) == 24 * 24 && Count(16, 10, 4, 32, STUB_ICON) == 0 && Count(44, 14, 4, 28, STUB_ICON) == 0);
    assert(CountIn(kNamePaper, STUB_ICON) == 24 * 24);
    assert(Count(52, 17, 50, 7, DEV_INK2) > 0);                                                            /* the column does not move */
    v.name.len ^= 1; assert(DevWhole(DevCheckTouch(&gNameOld, &v)));
    monArt = 0;
    /* FireRed's question cut before the counter (8 px short of it) and counted: 31 glyphs 8 wide */
    v = NameBoard(); memset(v.name.title, 0xbb + ('W' - 'A'), 31); v.name.title[31] = 0xff;
    keepW = smallWidths[0xbb + ('W' - 'A')]; smallWidths[0xbb + ('W' - 'A')] = 8; memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    q = sDevKeyCut; CtrPokeTouch_Draw(canvas, &v);
    assert(sDevKeyCut == q + 1 && Count(52, 17, 8, 7, DEV_INK2) > 0);
    r.x = (int16_t)(DEV_NAME_COUNT_RIGHT - DevSmallTextWidth("3 / 7")); assert(Count(r.x - 8, 14, 8, 12, DEV_INK2) == 0 && Count(r.x, 14, 8, 12, DEV_INK2) > 0);
    smallWidths[0xbb + ('W' - 'A')] = keepW; memset(sDevSmallReady, 0, sizeof(sDevSmallReady)); sDevKeyCut = 0;
    /* a typed character wider than its 14 px (from 2 px into its slot to the next slot) left out and counted */
    keepW = fontWidths[0xbb + ('R' - 'A')]; fontWidths[0xbb + ('R' - 'A')] = 15; memset(sPtFontReady, 0, sizeof(sPtFontReady));
    v = NameBoard(); q = sDevKeyCut; CtrPokeTouch_Draw(canvas, &v);
    assert(sDevKeyCut == q + 1 && Count(NAME_SLOT(0), NAME_TOP, 16, UI_TEXT_H, DEV_TITLE_INK) == 0 && Count(NAME_SLOT(1), NAME_TOP, 14, UI_TEXT_H, DEV_TITLE_INK) > 0);
    fontWidths[0xbb + ('R' - 'A')] = keepW; memset(sPtFontReady, 0, sizeof(sPtFontReady)); sDevKeyCut = 0;
    v = NameBoard(); DevCheckTouch(&gNameOld, &v);

    /* the keys: FireRed's character on each, a space key its mark alone (the letters' column 6, and the places a short
     * row leaves: column 7 of the letters' last two rows, column 5 of the symbols' rows 0, 1, 3); the cursor's key sunk
     * with its LED */
    v = NameBoard(); v.name.curCol = 3; v.name.curRow = 2; DevCheckTouch(&gNameOld, &v);
    r = DevNameKey(8, 2, 3); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) > 0 && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_SUNK_INK) > 0);
    r = DevNameKey(8, 0, 0); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) == 0 && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_KEY_INK) > 0);
    r = DevNameKey(8, 0, 6); assert(v.name.key[0][6] == 0 && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_KEY_INK) == 13 + 3 + 3);
    for (i = 2; i < 4; ++i) { r = DevNameKey(8, i, 7); assert(v.name.key[i][7] == 0 && Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_KEY_INK) == 13 + 3 + 3); }
    other = v; NameFill(&other, 0); other.name.curCol = 0; DevCheckTouch(&gNameOld, &other);
    for (i = 0; i < 4; ++i) {
        r = DevNameKey(6, i, 5);
        assert((other.name.key[i][5] == 0) == (i != 2) && (i == 2 || Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_KEY_INK) == 13 + 3 + 3));
    }
    DevCheckTouch(&gNameOld, &v);
    /* a touch sinks what it holds instead of the cursor's: another key, OK over the cursor's key, a key over the cursor
     * on DELETE, a page key over the page shown (and no LED of the page button's while it is held) */
    v.name.pressed = PT_NAME_HIT_OK; DevCheckTouch(&gNameOld, &v); NameSunk(&v, -1, PT_NAME_HIT_OK);
    v.name.curCol = 8; v.name.curRow = 1; v.name.pressed = PT_NAME_HIT_KEY; DevCheckTouch(&gNameOld, &v); NameSunk(&v, 0, -1);
    v.name.curRow = 0; v.name.pressed = PT_NAME_HIT_TAB + 0; DevCheckTouch(&gNameOld, &v);
    AssertKey(DevNameTop(2), DEV_LOOK_KEY, 1); AssertKey(DevNameTop(0), DEV_LOOK_KEY, 0); AssertKey(DevNameTop(1), DEV_LOOK_KEY, 0);
    NameSunk(&v, -1, -1); r = DevNameTop(1); assert(Count(r.x, r.y, r.w, r.h + DEV_DROP, DEV_LED) == 0);
    v.name.pressed = -1; v.name.curCol = 0; DevCheckTouch(&gNameOld, &v); NameSunk(&v, 0, -1);
    assert(Count(8, 56, 304, 27, DEV_LED) > 0);  /* the page shown's own */

    /* the screen: the question small in the secondary ink over the slots (none, nothing; the counter stays); a Pokemon's
     * icon 1:1 from (16, 10) in its box (the harness's 24x24 square in a clear border of 4: rows 14-37, from 16) with
     * the slots where they were, and none (the box the paper's) once it goes; the most a name takes, the gender sign
     * after it short of the counter */
    assert(Count(DEV_NAME_TITLE_X, kNamePaper.y, 200, 12, DEV_INK2) > 0);
    memset(v.name.title, 0xff, sizeof(v.name.title)); DevCheckTouch(&gNameOld, &v);
    assert(Count(kNamePaper.x, kNamePaper.y, 262 - kNamePaper.x, kNamePaper.h, DEV_INK2) == 0 && Count(262, NAME_SMALL_TOP, 37, 7, DEV_INK2) > 0);
    monArt = 1; v = NameBoard(); v.name.species = 4; DevCheckTouch(&gNameOld, &v);
    assert(Count(20, 16, 24, 22, STUB_ICON) == 24 * 22 && CountIn(kNamePaper, STUB_ICON) == 24 * 22);
    assert(Count(NAME_SLOT(3), NAME_LINE, 14, 1, DEV_MARK_ON) == 14 && Count(NAME_SLOT(0), NAME_TOP, 14, UI_TEXT_H, DEV_TITLE_INK) > 0);
    v.name.species = 0; DevCheckTouch(&gNameOld, &v);
    assert(CountIn(kNamePaper, STUB_ICON) == 0 && CountIn(kDevNameIcon, DEV_PAPER) == 24 * 24 && Count(NAME_SLOT(3), NAME_LINE, 14, 1, DEV_MARK_ON) == 14);
    monArt = 0;
    v.name.maxChars = PT_NAME_MAX; v.name.gender = 2; DevCheckTouch(&gNameOld, &v);
    assert(Count(NAME_SLOT(PT_NAME_MAX - 1), NAME_LINE, 14, 1, DEV_PAPER_LINE) == 14 && Count(NAME_SLOT(PT_NAME_MAX), NAME_LINE, 306 - NAME_SLOT(PT_NAME_MAX), 1, DEV_PAPER_LINE) == 0);
    assert(Count(NAME_SLOT(PT_NAME_MAX) + 2, 27, 8, 10, DEV_FEMALE) > 0 && Count(NAME_SLOT(PT_NAME_MAX) + 2, 27, 8, 10, DEV_FEMALE) == CountIn(kNamePaper, DEV_FEMALE));
    assert(Count(NAME_SLOT(PT_NAME_MAX) + 10, 14, 262 - NAME_SLOT(PT_NAME_MAX) - 10, 28, DEV_INK2) == 0 && Count(262, NAME_SMALL_TOP, 37, 7, DEV_INK2) > 0);

    /* no BACK (DELETE is FireRed's B): nothing hit where a view's BACK is, even with BACK asked for */
    v = NameBoard(); v.backVisible = v.backEnabled = 1; DevCheckTouch(&gNameOld, &v);
    assert(!CtrPokeTouch_BackHit(&v, 278, 210) && !CtrPokeTouch_BackHit(&v, 50, 220) && !CtrPokeTouch_BackHit(&v, 20, 20));

    /* what the keyboard does not draw presents nothing; the shell's colour whole; the ways in and out (HOME; FireRed's
     * picture: in the frame's screen, nothing of the keyboard hit) */
    strcpy(v.bar[0], "Anything"); v.enabled[PT_BAG] = 1; v.playerX = 3; v.backVisible = 0;
    assert(DevCheckTouch(&gNameOld, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&gNameOld, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&gNameOld, &v);
    other = v; other.name.on = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&gNameOld, &other)));
    v = NameBoard(); assert(DevWhole(DevCheckTouch(&gNameOld, &v)));
    v.viewport = 1; assert(DevWhole(DevCheckTouch(&gNameOld, &v)) && DrawnBy(&v, DevFrameShown) && CtrPokeTouch_NameHit(&v, 30, 100) == -1);
    v = NameBoard(); DevCheckTouch(&gNameOld, &v);
    NameWords(NameBoard());
    CtrPokeTouch_Draw(canvas, &gNameOld);

    /* The states for the eye, each through Update first. */
    v = NameBoard(); v.name.curCol = 2; v.name.curRow = 1; DevCheckTouch(&gNameOld, &v); shot("upper");
    NameFill(&v, 2); DevCheckTouch(&gNameOld, &v); shot("lower");
    NameFill(&v, 0); DevCheckTouch(&gNameOld, &v); shot("symbols");
    NameFill(&v, 1); v.name.curCol = 8; v.name.curRow = 1; DevCheckTouch(&gNameOld, &v); shot("cursor_delete");
    v.name.curRow = 0; DevCheckTouch(&gNameOld, &v); shot("cursor_page");
    v = NameBoard(); NameCodes(v.name.text, PT_NAME_MAX, "Kitkatk"); v.name.len = 7; v.name.curCol = 8; v.name.curRow = 2;
    DevCheckTouch(&gNameOld, &v); shot("full");
    monArt = 1; v = NameBoard(); v.name.species = 4; v.name.gender = 2; v.name.maxChars = 10; NameFill(&v, 2);
    NameCodes(v.name.title, (int)sizeof(v.name.title), "PKMN for this test?"); v.name.curCol = 4; v.name.curRow = 1;
    DevCheckTouch(&gNameOld, &v); shot("pokemon");
    monArt = 0;
    assert(sDevKeyCut == 0);                      /* main prints PASS device naming */
}
