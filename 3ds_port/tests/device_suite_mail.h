/* ---- mail: the letter being read as the red device (def_mail), through the field dispatcher -------------------------
 * Included by device_render_test.c before its kSuites. */

/* The letter as 3ds_bottom.c gives it (MailSnapshot): ORANGE MAIL (FireRed's name, its icon) from a writer as typed;
 * CLOSE shown and usable. */
static CtrPokeTouchView MailBoard(void)
{
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.mail.on = 1;
    v.mail.item = 121;
    strcpy(v.mail.name, "ORANGE MAIL");
    strcpy(v.mail.from, "Ash");
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* The columns of {x, y, w, h} holding ink `c`: the first and the last (-1 none). */
static void MailInkSpan(CtrPokeTouchRect r, uint16_t c, int *first, int *last)
{
    *first = *last = -1;
    for (int x = r.x; x < r.x + r.w; ++x)
        if (Count(x, r.y, 1, r.h, c)) {
            if (*first < 0)
                *first = x;
            *last = x;
        }
}

/* The German note's words (3ds_device_mail_impl.h DevMailNote; the DE build checks they are its), the longest: the
 * descender probe draws them in every build. */
static const char kMailNoteDe[] = "DER BRIEF IST AUF DEM OBEREN BILDSCHIRM. HIER TIPPEN ODER SCHLIE\xC3\x9F" "EN.";

/* Every small glyph, the space too, `w` px wide (the kit takes 8 at most: DevSmallGlyph); the stub's ink stays 4 px. */
static void MailSmallWidths(int w)
{
    memset(smallWidths, w, sizeof(smallWidths));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* The lines `s` takes on the note (DevMailNoteDraw's), up to `most`. */
static int MailNoteLines(const char *s, int most)
{
    return DevSmallWrapRun(s, 18, 18, 199, 226, most, DEV_SMALL_LINE_TIGHT, DEV_INK, 0);
}

/* The plan's suite (the screen and the note close the letter, CLOSE in the exit corner, the rule, the note, FROM and
 * the writer as one line, a letter without a writer), then the pieces (the paper's icon, its name, the rule, who wrote
 * it, at the board's places), the note's lines, CLOSE's states, what is not drawn, the ways in and out, upper case and
 * the states for the eye. */
static void SuiteMail(void)
{
    const uint16_t blue = GbaColor(31 << 10);              /* the synthetic icon of an item past 2 */
    CtrPokeTouchView mv = MailBoard(), mo = mv, a, b;
    uint8_t keep[256];
    char twice[2 * sizeof(kMailNoteDe)];
    int first, last;

    CtrPokeTouch_Draw(canvas, &mo);
    /* the plan's suite (def_mail) */
    mv = MailBoard(); DevCheckTouch(&mo, &mv); assert(DrawnBy(&mv, DevMailShown));
    assert(CtrPokeTouch_MailHit(&mv, 8, 8) && CtrPokeTouch_MailHit(&mv, 311, 183) && !CtrPokeTouch_MailHit(&mv, 8, 185)
           && CtrPokeTouch_MailHit(&mv, 20, 210) && !CtrPokeTouch_MailHit(&mv, 240, 210) && !CtrPokeTouch_MailHit(&mv, 278, 209));
    AssertHits(BackV, &mv, kDevGridExit, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(!BackV(&mv, 160, 90));
    assert(at(100, 100) == DEV_RULE && Count(18, 199, 208, 16, DEV_INK) > 0 && Count(22, 113, 276, 9, DEV_INK2) == 0 && Count(22, 113, 276, 9, DEV_INK) > 0);
    mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv); assert(Count(22, 109, 276, 18, DEV_INK) == 0);
    mv.mapGroup ^= 1; mv.speed ^= 1; assert(DevCheckTouch(&mo, &mv).count == 0);

    /* the screen and the note close the letter from their first pixels to their last; the gap between the note and
     * CLOSE, and CLOSE, are not the letter's; CLOSE is BACK's, nothing else is */
    mv = MailBoard(); DevCheckTouch(&mo, &mv);
    assert(Same(kDevMailScreen, (CtrPokeTouchRect){8, 8, 304, 176}));
    assert(!CtrPokeTouch_MailHit(&mv, 7, 8) && !CtrPokeTouch_MailHit(&mv, 312, 8) && !CtrPokeTouch_MailHit(&mv, 8, 7)
           && !CtrPokeTouch_MailHit(&mv, 311, 184) && !CtrPokeTouch_MailHit(&mv, 160, 189));
    assert(CtrPokeTouch_MailHit(&mv, 8, 190) && CtrPokeTouch_MailHit(&mv, 235, 231) && !CtrPokeTouch_MailHit(&mv, 236, 210)
           && !CtrPokeTouch_MailHit(&mv, 235, 232) && !CtrPokeTouch_MailHit(&mv, 7, 210));
    assert(!BackV(&mv, 20, 210) && !BackV(&mv, 236, 210));
    /* the paper's icon at its own size, centred on the screen; the name centred, capitals 81; the rule; "FROM <writer>"
     * one line in the paper's ink, centred, capitals 113 */
    assert(Count(kDevMailIcon.x, kDevMailIcon.y, kDevMailIcon.w, kDevMailIcon.h, blue) == 24 * 24
           && Count(148, 43, 24, 1, blue) == 0 && Count(148, 68, 24, 1, blue) == 0);
    MailInkSpan(kDevMailName, DEV_INK, &first, &last);
    assert(first > kDevMailName.x && abs((first - kDevMailName.x) - (kDevMailName.x + kDevMailName.w - 1 - last)) <= 2);
    assert(Count(22, 80, 276, 1, DEV_INK) == 0 && Count(22, 81, 276, 1, DEV_INK) > 0);
    assert(Count(80, 100, 160, 1, DEV_RULE) == 160 && Count(79, 100, 1, 1, DEV_RULE) == 0 && Count(240, 100, 1, 1, DEV_RULE) == 0
           && Count(80, 99, 160, 1, DEV_RULE) == 0 && Count(80, 101, 160, 1, DEV_RULE) == 0);
    MailInkSpan(kDevMailFrom, DEV_INK, &first, &last);
    assert(first > kDevMailFrom.x && abs((first - kDevMailFrom.x) - (kDevMailFrom.x + kDevMailFrom.w - 1 - last)) <= 2);
    assert(Count(22, 112, 276, 1, DEV_INK) == 0 && Count(22, 113, 276, 1, DEV_INK) > 0 && Count(22, 113, 276, 9, DEV_INK_SH) > 0
           && Count(22, 109, 276, 18, DEV_INK2) == 0);
    /* the note: small, from x 18 at capitals 199, nothing past 226; two lines as the stub's widths are, 9 apart */
    assert(Count(18, 199, 1, 7, DEV_INK) > 0 && Count(14, 196, 216, 3, DEV_INK) == 0 && Count(14, 196, 4, 30, DEV_INK) == 0
           && Count(226, 196, 4, 30, DEV_INK) == 0 && Count(18, 208, 208, 7, DEV_INK) > 0 && Count(18, 207, 208, 1, DEV_INK) == 0
           && Count(18, 217, 208, 7, DEV_INK) == 0);
    AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(244, 190, 68, 43, DEV_DARK_INK) > 0);
    /* the note is never cut: with every small glyph and the space 8 px wide (the kit's widest; FireRed's are 4-8, the
     * stub's 5, where each language's note takes two lines) the build's note takes three lines at most, nothing
     * counted */
    memcpy(keep, smallWidths, sizeof(keep));
    MailSmallWidths(8);
    assert(MailNoteLines(DevMailNote(), 4) <= 3);
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &mv); assert(sDevKeyCut == 0);
    /* the note's three lines (capitals 199 / 208 / 217): the German words 8 px wide drawn on the note; with a descender
     * on each glyph (SmallDescenders) the third ends by the paper's last row, none reaching the bezel under it, nothing
     * past 226, nothing counted; with an accent on each (Accents: LA CARTA ESTÁ ...) the first's stay on the paper,
     * none on the bezel over it. Twice the words need a fourth line: left out and counted once. The canvas shows `mo`
     * again after. */
    if (CTR_TR(0, 0, 1))
        assert(!strcmp(DevMailNote(), kMailNoteDe));
    assert(MailNoteLines(kMailNoteDe, 4) == 3);
    SmallDescenders(1); DevBegin(canvas, mv.shell); DevMailNoteDraw(kMailNoteDe);
    assert(Count(18, 199, 208, 7, DEV_INK) > 0 && Count(18, 208, 208, 7, DEV_INK) > 0 && Count(18, 217, 208, 7, DEV_INK) > 0);
    assert(Count(18, 224, 208, 1, DEV_INK) > 0 && Count(14, 225, 216, 6, DEV_INK) == 0 && Count(226, 196, 4, 30, DEV_INK) == 0
           && sDevKeyCut == 0);
    SmallDescenders(0);
    Accents(1); DevBegin(canvas, mv.shell); DevMailNoteDraw(kMailNoteDe); assert(BezelOver(14, 216) && sDevKeyCut == 0);
    DevBegin(canvas, mv.shell); DevMailNoteDraw(DevMailNote()); assert(BezelOver(14, 216) && sDevKeyCut == 0);
    Accents(0);
    snprintf(twice, sizeof(twice), "%s %s", kMailNoteDe, kMailNoteDe);
    assert(MailNoteLines(twice, 5) > 3); DevMailNoteDraw(twice); assert(sDevKeyCut == 1);
    sDevKeyCut = 0;
    memcpy(smallWidths, keep, sizeof(keep));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
    /* the note ends by 226: one word of 42 stub glyphs (210 px, 2 more than the line) is cut there, after the last
     * whole glyph (x 218-222), and counted */
    memset(twice, 'A', 42); twice[42] = 0; DevMailNoteDraw(twice);
    assert(sDevKeyCut == 1 && Count(218, 199, 4, 7, DEV_INK) > 0 && Count(222, 196, 8, 30, DEV_INK) == 0);
    sDevKeyCut = 0;
    CtrPokeTouch_Draw(canvas, &mo);
    /* a name or a writer wider than its line (FireRed's are far narrower: here every capital 12 px) is cut at the line's
     * end and counted, each once a draw (CLOSE not shown: so wide, the German word would be counted on its key too) */
    memcpy(keep, fontWidths, sizeof(keep));
    for (unsigned c = 0xbb; c <= 0xd4; ++c)             /* A..Z */
        fontWidths[c] = 12;
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    a = MailBoard(); memset(a.mail.name, 'W', 30); a.mail.name[30] = 0; memset(a.mail.from, 'W', 23); a.mail.from[23] = 0;
    a.backVisible = 0;
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &a);
    assert(sDevKeyCut == 2 && Count(22, 81, 276, 1, DEV_INK) > 0 && Count(22, 113, 276, 1, DEV_INK) > 0
           && Count(14, 77, 8, 50, DEV_INK) == 0 && Count(298, 77, 8, 50, DEV_INK) == 0);
    memcpy(fontWidths, keep, sizeof(keep));
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &mo);
    /* no writer: no FROM; no item: no icon */
    mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv);
    assert(Count(kDevMailFrom.x, kDevMailFrom.y, kDevMailFrom.w, kDevMailFrom.h, DEV_INK2) == 0
           && Count(kDevMailFrom.x, kDevMailFrom.y, kDevMailFrom.w, kDevMailFrom.h, DEV_INK) == 0);
    mv.mail.item = 0; assert(DevWhole(DevCheckTouch(&mo, &mv)));
    assert(Count(kDevMailIcon.x, kDevMailIcon.y, kDevMailIcon.w, kDevMailIcon.h, DEV_PAPER) == 24 * 24);
    /* CLOSE: sunk under a touch, grey while it cannot be used, gone when not shown (the shell there; the screen and the
     * note still close the letter) */
    mv = MailBoard(); DevCheckTouch(&mo, &mv);
    mv.backPressed = 1; DevCheckTouch(&mo, &mv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
    mv.backPressed = 0; mv.backEnabled = 0; DevCheckTouch(&mo, &mv); assert(at(247, 210) == DIM_DARK);
    mv.backEnabled = 1; mv.backVisible = 0; DevCheckTouch(&mo, &mv);
    assert(at(247, 210) == DEV_SHELL_RED && !BackV(&mv, 247, 210) && CtrPokeTouch_MailHit(&mv, 160, 90)
           && CtrPokeTouch_MailHit(&mv, 20, 210));
    /* what the letter does not draw presents nothing: the bar, HOME's keys, the map */
    mv = MailBoard(); DevCheckTouch(&mo, &mv);
    strcpy(mv.bar[0], "Anything"); mv.enabled[PT_BAG] = 1; mv.playerX = 3;
    assert(DevCheckTouch(&mo, &mv).count == 0);
    mv.shell = 1; assert(DevWhole(DevCheckTouch(&mo, &mv)) && at(1, 100) == DEV_SHELL_BLUE);
    mv.shell = 0; DevCheckTouch(&mo, &mv);
    strcpy(mv.mail.name, "RETRO MAIL"); assert(DevWhole(DevCheckTouch(&mo, &mv)));
    /* the ways in and out: the GBA picture (in the frame's screen: nothing of the letter hit, the frame's BACK) */
    mv.viewport = 1; assert(DevWhole(DevCheckTouch(&mo, &mv)) && DrawnBy(&mv, DevFrameShown));
    assert(!CtrPokeTouch_MailHit(&mv, 160, 90) && !CtrPokeTouch_MailHit(&mv, 20, 210) && CtrPokeTouch_BackHit(&mv, 278, 209)
           && !CtrPokeTouch_BackHit(&mv, 180, 210));
    mv = MailBoard(); mv.mail.on = 0; assert(!CtrPokeTouch_MailHit(&mv, 160, 90) && !CtrPokeTouch_MailHit(&mv, 20, 210));
    mv = MailBoard(); assert(DevWhole(DevCheckTouch(&mo, &mv)));
    /* upper case (owner decision 5): the paper's name, which v0.6.0 title-cased, in capitals; the writer as typed */
    a = b = MailBoard();
    strcpy(a.mail.name, "Orange Mail"); TouchSamePixels(&a, &b, 1, "the paper's name in v0.6.0's case and in capitals");
    strcpy(b.mail.from, "ASH"); TouchSamePixels(&a, &b, 0, "the writer as typed and in capitals");
    CtrPokeTouch_Draw(canvas, &mo);

    /* The states for the eye, each through Update first. */
    mv = MailBoard(); DevCheckTouch(&mo, &mv); shot("from");
    mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv); shot("bare");
    assert(sDevKeyCut == 0);                      /* main prints PASS device mail */
}
