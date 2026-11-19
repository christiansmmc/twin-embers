/* ---- save: SAVE as the Definitiva (boards def_save_confirm, def_save_writing, def_save_saved; plan 04b Task 4) ------
 * Included by device_render_test.c before its kSuites. */

/* SAVE's key under a point in the view's state (CtrPokeTouch_SaveHit): PT_SAVE_*. */
static int SaveHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_SaveHit(x, y, ((const CtrPokeTouchView *)view)->saveState);
}

/* The dialogue for `state` as 3ds_bottom.c's SaveFill gives it: the place on bar[0] (FireRed's name, PALLET TOWN), the
 * dialogue's two halves on bar[1] and bar[2] (the port's words, the boards'); `overwrite`: another game is saved. */
static void SaveDialogue(CtrPokeTouchView *v, int state, int overwrite)
{
    memset(v->bar, 0, sizeof(v->bar));
    v->saveState = (uint8_t)state;
    snprintf(v->bar[0], PT_BAR_LEN, "PALLET TOWN");
    if (state == SAVE_WRITING) {
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("SAVING...", "GUARDANDO...", "SPEICHERN..."));
        snprintf(v->bar[2], PT_BAR_LEN, CTR_TR("DON'T TURN OFF THE POWER.", "NO APAGUES LA CONSOLA.", "NICHT AUSSCHALTEN."));
    } else if (state == SAVE_SUCCESS) {
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("%s SAVED THE GAME.", "%s GUARD\xC3\x93 LA PARTIDA.", "%s HAT GESPEICHERT."),
                 v->playerName);
    } else if (state == SAVE_ERROR) {
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("COULD NOT SAVE.", "NO SE PUDO GUARDAR.", "SPEICHERN FEHLGESCHLAGEN."));
        snprintf(v->bar[2], PT_BAR_LEN, CTR_TR("TRY AGAIN.", "PRUEBA OTRA VEZ.", "BITTE ERNEUT VERSUCHEN."));
    } else if (overwrite) {
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("ANOTHER GAME IS SAVED.", "YA HAY OTRA PARTIDA.", "ANDERER SPIELSTAND DA."));
        snprintf(v->bar[2], PT_BAR_LEN, CTR_TR("OVERWRITE IT?", "\xC2\xBFSOBRESCRIBIRLA?", "\xC3\x9C" "BERSCHREIBEN?"));
    } else {
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("WOULD YOU LIKE TO SAVE THE GAME?", "\xC2\xBFQUIERES GUARDAR LA PARTIDA?",
                                               "M\xC3\x96" "CHTEST DU SPEICHERN?"));
    }
}

/* The plan's board: SAVE asking, YES chosen, nothing held, the bar at no step; 8 badges, a Pokedex of 151, 123:45,
 * ¥999999; the player as typed. */
static CtrPokeTouchView SaveBoard(void)
{
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.save = 1;
    v.saveSel = 0;
    v.savePressed = PT_SAVE_NONE;
    v.saveBadges = 8;
    v.saveHasDex = 1;
    v.saveDex = 151;
    v.saveHours = 123;
    v.saveMinutes = 45;
    v.saveMoney = 999999;
    strcpy(v.playerName, "Rick");
    SaveDialogue(&v, SAVE_CONFIRM, 0);
    return v;
}

/* A text the last whole draw told the kit of: from x with its capitals at y, small or not, in `ink` over `shadow`, and
 * (s not NULL) the bytes it drew `s`. */
static int SaveText(int x, int y, int small, uint16_t ink, uint16_t shadow, const char *s)
{
    for (int i = 0; i < gTraceCount; ++i)
        if (gTrace[i].op == DEV_TRACE_TEXT && gTrace[i].x == x && gTrace[i].y == y && gTrace[i].b == small
            && gTrace[i].a == ink && gTrace[i].h == shadow && (!s || !strcmp(gTrace[i].s, s)))
            return 1;
    return 0;
}

/* Every glyph `normal` px wide in the normal font and `small` in the small one (8 at most: the kit's small glyphs are),
 * for a test of a wider language; normal 0 puts the widths back. */
static void SaveWideFont(int normal, int small)
{
    static uint8_t keepWidths[256], keepSmall[256];

    if (normal) {
        memcpy(keepWidths, fontWidths, sizeof(keepWidths));
        memcpy(keepSmall, smallWidths, sizeof(keepSmall));
        memset(fontWidths, normal, sizeof(fontWidths));
        memset(smallWidths, small, sizeof(smallWidths));
    } else {
        memcpy(fontWidths, keepWidths, sizeof(keepWidths));
        memcpy(smallWidths, keepSmall, sizeof(keepSmall));
    }
    memset(sPtFontReady, 0, sizeof(sPtFontReady));
    memset(sDevSmallReady, 0, sizeof(sDevSmallReady));
}

/* The dialogue drawn as its rule has it (the halves joined by a space): one normal line in the paper's ink from (20,
 * capitals 121) when it fits 280 px, else small from (20, capitals 118) in as many lines as it needs, 8 apart (two at
 * most: the second's descenders end on the paper's last row); nothing right of 300 nor on the bezel, nothing cut. */
static void SaveSaid(const CtrPokeTouchView *v)
{
    char line[2 * PT_BAR_LEN];
    int lines;

    snprintf(line, sizeof(line), "%s%s%s", v->bar[1], v->bar[2][0] ? " " : "", v->bar[2]);
    sDevKeyCut = 0;
    CtrPokeTouch_Draw(canvas, v);
    assert(sDevKeyCut == 0 && Count(301, 115, 5, 19, DEV_INK) == 0 && Count(14, 134, 292, 6, DEV_INK) == 0);
    if (DevTextWidth(line) <= 280) {
        assert(SaveText(20, 121, 0, DEV_INK, DEV_INK_SH, line));
        return;
    }
    lines = DevSmallWrapRun(line, 20, 20, 118, 300, 3, DEV_SAVE_SMALL_LINE, DEV_INK, 0);
    assert(lines <= 2 && SaveText(20, 118, 1, DEV_INK, DEV_INK, NULL)
           && SaveText(20, 126, 1, DEV_INK, DEV_INK, NULL) == (lines == 2));
    assert(Count(14, 115, 292, 19, DEV_INK_SH) == 0);                 /* no normal line: its shadow */
}

/* Upper case (owner decision 5): the place (FireRed's name) drawn in capitals through DevUpper, as HOME draws it; the
 * port's words as given; the player's name as typed, in its row and in the dialogue. */
static void SaveUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView capitals = v;

    strcpy(v.bar[0], "Pallet Town");
    TouchSamePixels(&v, &capitals, 1, "the place in its case and in capitals");
    v = capitals;
    strcpy(capitals.playerName, "RICK");
    TouchSamePixels(&v, &capitals, 0, "the player's name as typed and in capitals");
    SaveDialogue(&v, SAVE_SUCCESS, 0);
    capitals = v;
    DevUpper(capitals.bar[1], v.bar[1], sizeof(capitals.bar[1]));
    TouchSamePixels(&v, &capitals, 0, "the name in the dialogue as typed and in capitals");
}

/* The plan's suite (YES / NO hit where drawn, the cursor and a touch sink them, beige and not hit while writing with the
 * bar of the port's steps, OK alone across both places after a result, up and sunk only under a touch; what SAVE does
 * not draw presents nothing), then the boards' pieces where the boards put them, the dialogue in one line or two small
 * ones in every language, the values, what is not drawn, the shell, the ways in and out, upper case and the states for
 * the eye. */
static void SuiteSave(void)
{
    const uint16_t red = Gba(0x001f);                       /* the player's synthetic map icon */
    CtrPokeTouchView v = SaveBoard(), old = v, other, a, b;
    char value[24];

    iconArt = 1;
    sPlayer.key = -1;
    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v = SaveBoard(); DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevSaveShown));
    AssertHits(SaveHitV, &v, kDevSaveYes, PT_SAVE_FIRST); AssertHits(SaveHitV, &v, kDevSaveNo, PT_SAVE_SECOND);
    AssertKey(kDevSaveYes, DEV_LOOK_KEY, 1); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 0);
    assert(at(100, 30) == DEV_RULE && at(100, 114) == DEV_RULE && Count(20, 17, 200, 9, DEV_TITLE_INK) > 0 && Count(20, 121, 280, 9, DEV_INK) > 0);
    assert(Count(150, 39, 58, 9, DEV_INK) > 0 && Count(209, 36, 18, 76, DEV_INK) == 0);                /* values end at 208 */
    v.saveSel = 1; DevCheckTouch(&old, &v); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 1);
    SaveDialogue(&v, SAVE_CONFIRM, 1); DevCheckTouch(&old, &v); assert(Count(14, 115, 292, 19, DEV_INK) > 0); /* one line or two small */
    v.saveState = SAVE_WRITING; v.saveStep = 1; SaveDialogue(&v, SAVE_WRITING, 0); DevCheckTouch(&old, &v);
    AssertKey(kDevSaveYes, DEV_LOOK_FAINT, 0); AssertKey(kDevSaveNo, DEV_LOOK_FAINT, 0);
    assert(CtrPokeTouch_SaveHit(100, 180, SAVE_WRITING) == PT_SAVE_NONE && at(197, 124) == DEV_EXP && at(294, 124) == DEV_HP_TRACK
           && Count(20, 118, 170, 15, DEV_INK) > 0 && Count(191, 118, 4, 15, DEV_INK) == 0);
    v.saveStep = 2; assert(DevWhole(DevCheckTouch(&old, &v))); assert(at(294, 124) == DEV_EXP);
    SaveDialogue(&v, SAVE_SUCCESS, 0); v.savePressed = PT_SAVE_NONE; DevCheckTouch(&old, &v);
    AssertKey(kDevSaveOk, DEV_LOOK_KEY, 0); AssertHits(SaveHitV, &v, kDevSaveOk, PT_SAVE_FIRST);
    v.savePressed = PT_SAVE_FIRST; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevSaveOk, DEV_LOOK_KEY, 1);
    SaveDialogue(&v, SAVE_ERROR, 0); v.savePressed = PT_SAVE_NONE; DevCheckTouch(&old, &v); AssertKey(kDevSaveOk, DEV_LOOK_KEY, 0);
    v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* def_save_confirm: SAVE's own screen (bezel 8..13, paper 14..305 x 14..133, the shell under it); the place from
     * (20, capitals 17) in the title's ink over the paper's shadow; the rules at 30 and 114, x 20..299; the five rows (the
     * label small in the secondary ink from 20, capitals the row's top + 4; the value in the paper's ink ending at 208,
     * capitals + 3); the player's icon at 4x in kDevSaveSprite; the question; YES cream and sunk (the cursor) with its
     * LED at (225, 154), NO dark and up in the exit corner; no BACK anywhere */
    v = SaveBoard(); DevCheckTouch(&old, &v);
    assert(at(11, 60) == DEV_BEZEL && at(160, 136) == DEV_BEZEL && at(14, 14) == DEV_PAPER && at(305, 133) == DEV_PAPER
           && at(160, 144) == DEV_SHELL_RED && at(4, 100) == DEV_SHELL_RED);
    assert(SaveText(20, 17, 0, DEV_TITLE_INK, DEV_INK_SH, "PALLET TOWN"));
    assert(Count(20, 30, 280, 1, DEV_RULE) == 280 && Count(20, 114, 280, 1, DEV_RULE) == 280 && at(19, 30) == DEV_PAPER
           && at(300, 30) == DEV_PAPER && at(19, 114) == DEV_PAPER && at(300, 114) == DEV_PAPER);
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevSaveRow(i);

        DevSaveValue(&v, i, value, sizeof(value));
        assert(SaveText(20, r.y + 4, 1, DEV_INK2, DEV_INK2, DevSaveLabel(i))
               && SaveText(208 - DevTextWidth(value), r.y + 3, 0, DEV_INK, DEV_INK_SH, value));
    }
    assert(Count(kDevSaveSprite.x, kDevSaveSprite.y, kDevSaveSprite.w, kDevSaveSprite.h, red) == 64 * 64);
    assert(at(227, 60) == DEV_PAPER && at(292, 60) == DEV_PAPER && at(260, 37) == DEV_PAPER && at(260, 102) == DEV_PAPER);
    assert(SaveText(20, 121, 0, DEV_INK, DEV_INK_SH, v.bar[1]));
    AssertKey(kDevSaveYes, DEV_LOOK_KEY, 1); assert(LedAt(kDevSaveYes)); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 0);
    assert(Count(8, 150, 228, 82, DEV_SUNK_INK) > 0 && Count(244, 148, 68, 82, DEV_DARK_INK) > 0);
    for (int x = 0; x < W; x += 4)
        for (int y = 0; y < H; y += 4)
            assert(!CtrPokeTouch_BackHit(&v, x, y));
    /* NO chosen: NO sunk with its LED at (301, 154), YES up; a touch sinks the key under it, whatever the cursor; the
     * keys never move */
    v.saveSel = 1; DevCheckTouch(&old, &v); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 1); assert(LedAt(kDevSaveNo));
    AssertKey(kDevSaveYes, DEV_LOOK_KEY, 0);
    v.savePressed = PT_SAVE_FIRST; DevCheckTouch(&old, &v); AssertKey(kDevSaveYes, DEV_LOOK_KEY, 1); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 0);
    v.savePressed = PT_SAVE_SECOND; v.saveSel = 0; DevCheckTouch(&old, &v); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 1);
    AssertKey(kDevSaveYes, DEV_LOOK_KEY, 0);
    v.savePressed = PT_SAVE_NONE; DevCheckTouch(&old, &v);
    v.saveStep = 1; assert(DevCheckTouch(&old, &v).count == 0);                 /* the bar is not drawn while asking */

    /* the dialogue in one normal line or two small ones (Review Focus 4): the question, another game's, the error, a
     * save of the widest name, in the build's language (the ES / DE label-fit builds run this too) */
    v = SaveBoard(); SaveSaid(&v);
    SaveDialogue(&v, SAVE_CONFIRM, 1); SaveSaid(&v);
    SaveDialogue(&v, SAVE_ERROR, 0); SaveSaid(&v);
    strcpy(v.playerName, "MMMMMMM"); SaveDialogue(&v, SAVE_SUCCESS, 0); SaveSaid(&v);
    CtrPokeTouch_Draw(canvas, &old);
    /* a wider language (every glyph 9 px, 8 in the small font): another game's question needs more than 280 px, so two
     * small lines from capitals 118 and 126 ending by 300, uncut; with a descender on every small glyph the second's end
     * on the paper's last row (133), none on the bezel */
    v = SaveBoard(); SaveDialogue(&v, SAVE_CONFIRM, 1); DevCheckTouch(&old, &v);
    SaveWideFont(9, 8);
    {
        char line[2 * PT_BAR_LEN];

        snprintf(line, sizeof(line), "%s %s", v.bar[1], v.bar[2]);
        assert(DevTextWidth(line) > 280 && DevSmallWrapRun(line, 20, 20, 118, 300, 3, DEV_SAVE_SMALL_LINE, DEV_INK, 0) == 2);
    }
    SaveSaid(&v);
    SmallDescenders(1); CtrPokeTouch_Draw(canvas, &v);
    assert(Count(20, 133, 280, 1, DEV_INK) > 0 && Count(14, 134, 292, 6, DEV_INK) == 0);
    SmallDescenders(0);
    /* a dialogue that needs a third small line is cut there and counted, once a draw; a place wider than 280 px is cut
     * with "..." by 300 and counted */
    other = v;
    strcpy(other.bar[1], "AAAAAAA AAAAAAA AAAAAAA AAAAAAA AAAAAAA");
    strcpy(other.bar[2], "AAAAAAA AAAAAAA AAAAAAA AAAAAAA AAAAAAA");
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &other);
    assert(sDevKeyCut == 1 && Count(14, 133, 292, 7, DEV_INK) == 0 && Count(301, 115, 5, 19, DEV_INK) == 0);
    other = v;
    strcpy(other.bar[0], "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");
    sDevKeyCut = 0; CtrPokeTouch_Draw(canvas, &other);
    assert(sDevKeyCut == 1 && Count(300, 14, 6, 14, DEV_TITLE_INK) == 0 && Count(20, 17, 40, 8, DEV_TITLE_INK) > 0);
    sDevKeyCut = 0;
    SaveWideFont(0, 0);
    CtrPokeTouch_Draw(canvas, &old);

    /* def_save_writing: the dialogue small from (20, capitals 118), 8 apart, ending by 190 (6 px left of the bar), no
     * normal line; the bar (kDevSaveBar: its frame, the track, the port's steps of PT_SAVE_STEPS filled in the EXP blue
     * from the left); YES and NO beige in their places, their words in the beige's ink, not hit; neither the cursor nor a
     * touch is drawn */
    v = SaveBoard(); SaveDialogue(&v, SAVE_WRITING, 0); DevCheckTouch(&old, &v);
    assert(SaveText(20, 118, 1, DEV_INK, DEV_INK, NULL) && Count(191, 114, 5, 20, DEV_INK) == 0
           && Count(14, 115, 292, 19, DEV_INK_SH) == 0 && Count(14, 134, 292, 6, DEV_INK) == 0);
    {
        char line[2 * PT_BAR_LEN];

        snprintf(line, sizeof(line), "%s %s", v.bar[1], v.bar[2]);
        assert(SaveText(20, 126, 1, DEV_INK, DEV_INK, NULL)
               == (DevSmallWrapRun(line, 20, 20, 118, 190, 3, DEV_SAVE_SMALL_LINE, DEV_INK, 0) == 2));
    }
    assert(at(196, 122) == DEV_HP_FRAME && at(295, 127) == DEV_HP_FRAME && at(197, 123) == DEV_HP_TRACK
           && at(294, 126) == DEV_HP_TRACK && at(195, 124) == DEV_PAPER && at(296, 124) == DEV_PAPER
           && at(240, 121) == DEV_PAPER && at(240, 128) == DEV_PAPER && Count(197, 123, 98, 4, DEV_EXP) == 0);
    v.saveStep = 1; assert(DevWhole(DevCheckTouch(&old, &v)));
    assert(Count(197, 123, 49, 4, DEV_EXP) == 49 * 4 && at(246, 124) == DEV_HP_TRACK);
    v.saveStep = 2; assert(DevWhole(DevCheckTouch(&old, &v))); assert(Count(197, 123, 98, 4, DEV_EXP) == 98 * 4);
    AssertKey(kDevSaveYes, DEV_LOOK_FAINT, 0); AssertKey(kDevSaveNo, DEV_LOOK_FAINT, 0);
    assert(Count(8, 148, 228, 85, DEV_FAINT_INK) > 0 && Count(244, 148, 68, 85, DEV_FAINT_INK) > 0);
    for (int x = 0; x < W; x += 2)
        for (int y = 140; y < H; y += 2)
            assert(SaveHitV(&v, x, y) == PT_SAVE_NONE);
    v.saveSel = 1; v.savePressed = PT_SAVE_FIRST; assert(DevCheckTouch(&old, &v).count == 0);

    /* def_save_saved: "<name> SAVED THE GAME." with the name as typed; OK across both places, up (a result has no
     * cursor), its word in the key's ink, sunk with its LED at (301, 154) only under a touch; an error's the same */
    v = SaveBoard(); SaveDialogue(&v, SAVE_SUCCESS, 0); DevCheckTouch(&old, &v);
    assert(strstr(v.bar[1], "Rick") && SaveText(20, 121, 0, DEV_INK, DEV_INK_SH, v.bar[1]));
    AssertKey(kDevSaveOk, DEV_LOOK_KEY, 0); AssertHits(SaveHitV, &v, kDevSaveOk, PT_SAVE_FIRST);
    assert(Count(8, 148, 304, 85, DEV_LED) == 0 && Count(8, 148, 304, 85, DEV_KEY_INK) > 0 && at(240, 189) == DEV_KEY);
    v.saveSel = 1; assert(DevCheckTouch(&old, &v).count == 0);
    v.saveStep = 1; assert(DevCheckTouch(&old, &v).count == 0);                 /* the bar is not drawn after a result */
    v.savePressed = PT_SAVE_FIRST; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevSaveOk, DEV_LOOK_KEY, 1);
    assert(LedAt(kDevSaveOk));
    v.savePressed = PT_SAVE_NONE; SaveDialogue(&v, SAVE_ERROR, 0); assert(DevWhole(DevCheckTouch(&old, &v)));
    AssertKey(kDevSaveOk, DEV_LOOK_KEY, 0); AssertHits(SaveHitV, &v, kDevSaveOk, PT_SAVE_FIRST);

    /* the values: no Pokedex says "---" (and its count is not drawn), v0.6.0's caps (999 hours, 59 minutes, ¥999999) */
    v = SaveBoard(); DevCheckTouch(&old, &v);
    v.saveHasDex = 0; assert(DevWhole(DevCheckTouch(&old, &v)));
    v.saveDex = 7; assert(DevCheckTouch(&old, &v).count == 0);
    a = b = SaveBoard(); a.saveHours = 999; b.saveHours = 1000; TouchSamePixels(&a, &b, 1, "999 and 1000 hours");
    a.saveMinutes = 59; b.saveMinutes = 200; TouchSamePixels(&a, &b, 1, "59 and 200 minutes");
    a.saveMoney = 999999; b.saveMoney = 1000000; TouchSamePixels(&a, &b, 1, "999999 and 1000000");
    b = a; b.saveBadges = 10; TouchSamePixels(&a, &b, 0, "8 and 10 badges");   /* the stub's digits look alike: */
    b = a; b.saveDex = 15; TouchSamePixels(&a, &b, 0, "151 and 15 caught");    /* a count of another length */
    /* what SAVE does not draw presents nothing: BACK's fields, HOME's keys, the map, OPTIONS'; what it draws redraws it
     * whole: the shell, the player's icon and name, the place */
    v = SaveBoard(); DevCheckTouch(&old, &v);
    v.backVisible = v.backEnabled = v.backPressed = 1; v.enabled[PT_BAG] = 1; v.playerX = 3;
    v.optCount = 2; strcpy(v.optLabel[0], "Text speed");
    assert(DevCheckTouch(&old, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&old, &v);
    v.playerFemale = 1; assert(DevWhole(DevCheckTouch(&old, &v)));
    strcpy(v.playerName, "Leaf"); assert(DevWhole(DevCheckTouch(&old, &v)));
    strcpy(v.bar[0], "VIRIDIAN CITY"); assert(DevWhole(DevCheckTouch(&old, &v)));
    /* the ways in and out: HOME, SAVE, the GBA picture (in the frame's screen: nothing of SAVE drawn) */
    other = v; other.save = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)));
    v = SaveBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    v.viewport = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && DrawnBy(&v, DevFrameShown));
    v = SaveBoard(); DevCheckTouch(&old, &v);
    SaveUpperCase(SaveBoard());
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first: asking (YES chosen, then NO), another game's question (NO
     * chosen, as the game opens it), writing (the screen shown during the write: one step of two), saved, the error. */
    v = SaveBoard(); DevCheckTouch(&old, &v); shot("confirm");
    v.saveSel = 1; DevCheckTouch(&old, &v); shot("no");
    SaveDialogue(&v, SAVE_CONFIRM, 1); DevCheckTouch(&old, &v); shot("overwrite");
    SaveDialogue(&v, SAVE_WRITING, 0); v.saveStep = 1; DevCheckTouch(&old, &v); shot("writing");
    SaveDialogue(&v, SAVE_SUCCESS, 0); v.saveStep = 0; DevCheckTouch(&old, &v); shot("saved");
    SaveDialogue(&v, SAVE_ERROR, 0); DevCheckTouch(&old, &v); shot("error");
    iconArt = 0;
    sPlayer.key = -1;
    assert(sDevKeyCut == 0);                      /* main prints PASS device save */
}
