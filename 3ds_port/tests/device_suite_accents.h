/* ---- accents: a note's first line keeps FireRed's accents on its paper (plan 04b's final review) --------------------
 * Included by device_render_test.c before its suites (after SmallDescenders): Accents, which every suite may use, and the
 * probes of the notes whose views' suites are in device_render_test.c (battle, party, summary, bag); the other views'
 * are in their own suite headers.
 *
 * FireRed's accented capitals rise 3 rows over the capitals in both fonts (the normal font's Á É Í Ó Ú Ñ Ä Ö Ü, the
 * small font's acute and tilde; its umlauts 2), and the small font's descenders fall 7 under them: a note on the grid's
 * footer (paper 196..225) has its first line's capitals 3 rows into the paper, 199, and its small lines 9 apart, 199,
 * 208 and 217. With an accent on every glyph the bezel's row over the paper, 195, keeps its colour. */

/* While `on`: every glyph of both fonts but the space with an accent, an ink pixel at its left in the row 3 over the
 * capitals (the normal font's glyph row 0, the small font's row 1; the stubs have none), the highest the game's letters
 * reach. The pixel is set, not added to (SmallDescenders). Off puts the glyphs back as they were. */
static void Accents(int on)
{
    static uint8_t keep[sizeof(font)], keepSmall[sizeof(smallFont)];

    if (on) {
        memcpy(keep, font, sizeof(keep));
        memcpy(keepSmall, smallFont, sizeof(keepSmall));
        for (unsigned code = 1; code < 256; ++code)
            for (int small = 0; small < 2; ++small) {
                uint8_t *row = small ? smallFont + code * 32u + 1 * 2 : font + code * 64u;   /* the first tile's row */
                unsigned bits = ((unsigned)row[1] << 8 | row[0]) & ~(3u << 14);                /* x 0: the top two bits */

                bits |= 1u << 14;
                row[0] = (uint8_t)bits;
                row[1] = (uint8_t)(bits >> 8);
            }
    } else {
        memcpy(font, keep, sizeof(keep));
        memcpy(smallFont, keepSmall, sizeof(keepSmall));
    }
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* The bezel's row over a footer's paper (its x and width), 195, is all bezel: no accent reached it. */
static int BezelOver(int x, int w)
{
    return Count(x, 195, w, 1, DEV_BEZEL) == w;
}

/* While `on`: every glyph of the normal font 16 px wide, so a text DevReadText reads goes to its small lines (the
 * port's own questions fit the normal font's two). What it cuts (key labels) the caller forgives. */
static void WideGlyphs(int on)
{
    static uint8_t keep[sizeof(fontWidths)];

    if (on) {
        memcpy(keep, fontWidths, sizeof(keep));
        memset(fontWidths, 16, sizeof(fontWidths));
    } else {
        memcpy(fontWidths, keep, sizeof(keep));
    }
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
}

/* Port-written words three small lines long on the grid's note, more than the normal font's two. */
static const char kAccentsLong[] = "Port words for a message so long that the normal font would need three lines "
                                   "for all of them.";

static CtrBattleView PtyBoard(void);
static CtrSummaryUiView SumBoard(void);
static CtrPokeTouchView BagBoard(void);

/* TARGET's question read small (glyphs too wide for the normal font's two lines): capitals 199, its accents on the
 * paper. */
static void AccentsBattle(void)
{
    CtrBattleView v = BatBoard();
    int cut = sDevKeyCut;

    v.state = BUI_TARGET; v.cursor = 1;
    WideGlyphs(1); CtrBattleUi_Draw(canvas, &v);
    assert(Count(20, 199, 204, 1, DEV_INK) > 0 && Count(14, 196, 216, 3, DEV_INK) == 0);
    Accents(1); CtrBattleUi_Draw(canvas, &v); assert(BezelOver(14, 216));
    Accents(0); WideGlyphs(0);
    sDevKeyCut = cut;
}

/* A message too long for the normal font's two lines: three small lines from x 22, capitals 199, 208 and 217, kept
 * clear of FireRed's arrow; their accents on the paper, the third's descenders by 224. */
static void AccentsParty(void)
{
    CtrBattleView v = PtyBoard();

    v.pty.panel = BUI_PTY_TEXT; strcpy(v.pty.message, kAccentsLong); CtrBattleUi_Draw(canvas, &v);
    assert(Count(22, 199, 188, 1, DEV_INK) > 0 && Count(22, 208, 188, 1, DEV_INK) > 0 && Count(22, 217, 188, 1, DEV_INK) > 0);
    assert(Count(14, 196, 216, 3, DEV_INK) == 0 && Count(22, 207, 188, 1, DEV_INK) == 0 && Count(22, 216, 188, 1, DEV_INK) == 0);
    Accents(1); CtrBattleUi_Draw(canvas, &v); assert(BezelOver(14, 216)); Accents(0);
    SmallDescenders(1); CtrBattleUi_Draw(canvas, &v);
    assert(Count(22, 224, 188, 1, DEV_INK) > 0 && Count(14, 225, 216, 6, DEV_INK) == 0);
    SmallDescenders(0);
}

/* The Summary's note: the ability's name (DevNoteTitle) on INFO, the move's description in two small lines on the
 * details, and the hint read small on MOVES (glyphs too wide for the normal font's two), each from capitals 199 (the
 * description's second at 208), their accents on the paper. */
static void AccentsSummary(void)
{
    CtrSummaryUiView v = SumBoard();
    int cut = sDevKeyCut;

    CtrSummaryUi_Draw(canvas, &v);
    assert(Count(14, 199, 216, 1, DEV_TITLE_INK) > 0 && Count(14, 196, 216, 3, DEV_TITLE_INK) == 0);
    Accents(1); CtrSummaryUi_Draw(canvas, &v); assert(BezelOver(14, 216)); Accents(0);
    v.data.page = SUI_PAGE_MOVES_INFO; v.data.cursor = 0;
    strcpy(v.data.moveDesc, "Port-written words in place of what a move does, set in two small lines.");
    CtrSummaryUi_Draw(canvas, &v);
    assert(Count(20, 199, 204, 1, DEV_INK) > 0 && Count(20, 208, 204, 1, DEV_INK) > 0 && Count(14, 196, 216, 3, DEV_INK) == 0
           && Count(20, 207, 204, 1, DEV_INK) == 0);
    Accents(1); CtrSummaryUi_Draw(canvas, &v); assert(BezelOver(14, 216)); Accents(0);
    v.data.page = SUI_PAGE_MOVES;
    WideGlyphs(1); CtrSummaryUi_Draw(canvas, &v);
    assert(Count(20, 199, 204, 1, DEV_INK) > 0 && Count(14, 196, 216, 3, DEV_INK) == 0);
    Accents(1); CtrSummaryUi_Draw(canvas, &v); assert(BezelOver(14, 216));
    Accents(0); WideGlyphs(0);
    sDevKeyCut = cut;
}

/* The Bag's footer: the description (and a TM's chip on its first line), a question read small; their accents on the
 * paper (the rows are the suite's). */
static void AccentsBag(void)
{
    CtrPokeTouchView v = BagBoard();

    Accents(1); CtrPokeTouch_Draw(canvas, &v); assert(BezelOver(14, 216));
    v.bag.kind = PT_BAG_KIND_TMCASE; strcpy(v.bag.title, "TM CASE"); v.bag.moveType = 4; strcpy(v.bag.typeName, "GROUND");
    CtrPokeTouch_Draw(canvas, &v); assert(BezelOver(14, 216) && Count(48, 199, 60, 9, UiTypeColor(4)) > 0);
    v = BagBoard(); v.bag.state = PT_BAG_STATE_YESNO;
    strcpy(v.bag.prompt, "Port words for a question so long that it takes three lines of the small font.");
    CtrPokeTouch_Draw(canvas, &v); assert(BezelOver(14, 216) && Count(48, 196, 178, 30, DEV_INK) > 0);
    Accents(0);
}
