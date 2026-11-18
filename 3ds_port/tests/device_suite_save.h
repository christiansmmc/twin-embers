/* ---- save: SAVE as the red device, through the field dispatcher (no board; plan 3 Task 2) ------------------------------
 * Included by device_render_test.c before its kSuites. */

/* SAVE's key under a point in the view's state (CtrPokeTouch_SaveHit): PT_SAVE_*. */
static int SaveHitV(const void *view, int x, int y)
{
    return CtrPokeTouch_SaveHit(x, y, ((const CtrPokeTouchView *)view)->saveState);
}

/* The dialogue for `state` as 3ds_bottom.c gives it (SaveFill's words); `overwrite`: another game is saved. */
static void SaveDialogue(CtrPokeTouchView *v, int state, int overwrite)
{
    memset(v->bar, 0, sizeof(v->bar));
    v->saveState = (uint8_t)state;
    if (state == SAVE_WRITING) {
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Saving...", "Guardando...", "Speichern..."));
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("Don't turn off the power.", "No apagues la consola.", "Nicht ausschalten."));
    } else if (state == SAVE_SUCCESS) {
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Game saved!", "\xc2\xa1Partida guardada!", "Spiel gespeichert!"));
    } else if (state == SAVE_ERROR) {
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Could not save.", "No se pudo guardar.", "Speichern fehlgeschlagen."));
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("Try again.", "Prueba otra vez.", "Bitte erneut versuchen."));
    } else if (overwrite) {
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Another game is saved.", "Ya hay otra partida.", "Anderer Spielstand da."));
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("Overwrite it?", "\xc2\xbfSobrescribirla?", "\xc3\x9c" "berschreiben?"));
    } else {
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Save your game?", "\xc2\xbfGuardar la partida?", "Spiel speichern?"));
    }
}

/* The plan's board: SAVE asking, YES chosen, nothing held; 8 badges, a Pokedex of 151, 123:45, ¥999999; the player as
 * typed. */
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

/* Upper case (owner decision 5): the dialogue (v0.6.0 title-cased bar[]) drawn in capitals; the player's name as typed. */
static void SaveUpperCase(CtrPokeTouchView v)
{
    CtrPokeTouchView capitals = v;

    for (int i = 0; i < PT_BAR_LINES; ++i)
        DevUpper(capitals.bar[i], v.bar[i], sizeof(capitals.bar[i]));
    TouchSamePixels(&v, &capitals, 1, "the dialogue in its case and in capitals");
    strcpy(capitals.playerName, "RICK");
    TouchSamePixels(&v, &capitals, 0, "the player's name as typed and in capitals");
}

/* The plan's suite (YES / NO hit where drawn, the cursor and a touch sink them, nothing while writing, OK alone after a
 * result, a value redraws the screen whole and what SAVE does not draw presents nothing), then the board's pieces, the
 * values, what is not drawn, the shell, the ways in and out, upper case and the states for the eye. */
static void SuiteSave(void)
{
    const uint16_t red = Gba(0x001f);                       /* the player's synthetic map icon */
    CtrPokeTouchView v = SaveBoard(), old = v, other, a, b;
    CtrPokeTouchRect r, paper = {14, 162, 188, 64};

    iconArt = 1;
    sPlayer.key = -1;
    CtrPokeTouch_Draw(canvas, &old);
    /* the plan's suite */
    v = SaveBoard(); DevCheckTouch(&old, &v); assert(DrawnBy(&v, DevSaveShown));
    AssertHits(SaveHitV, &v, kDevAskYes, PT_SAVE_FIRST); AssertHits(SaveHitV, &v, kDevAskNo, PT_SAVE_SECOND); AssertKey(kDevAskYes, DEV_LOOK_KEY, 1);
    v.saveSel = 1; DevCheckTouch(&old, &v); AssertKey(kDevAskNo, DEV_LOOK_KEY, 1); v.savePressed = PT_SAVE_FIRST; DevCheckTouch(&old, &v); AssertKey(kDevAskYes, DEV_LOOK_KEY, 1);
    v.saveState = SAVE_WRITING; DevCheckTouch(&old, &v); assert(CtrPokeTouch_SaveHit(260, 170, SAVE_WRITING) == PT_SAVE_NONE && at(260, 175) == DEV_SHELL_RED);
    v.saveState = SAVE_SUCCESS; DevCheckTouch(&old, &v); AssertHits(SaveHitV, &v, kDevAskYes, PT_SAVE_FIRST); assert(CtrPokeTouch_SaveHit(260, 215, SAVE_SUCCESS) == PT_SAVE_NONE);
    v.saveMinutes++; assert(DevWhole(DevCheckTouch(&old, &v))); v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);

    /* the board's pieces: HOME's screen with the five rows (the label small in the secondary ink at the left, the value
     * in the paper's ink at the right) and the player's icon at 4x; the question on its note; YES sunk with its LED, NO
     * up; no BACK anywhere */
    v = SaveBoard(); DevCheckTouch(&old, &v);
    for (int i = 0; i < 5; ++i) {
        r = DevSaveRow(i);
        assert(Count(r.x, r.y, 60, r.h, DEV_INK2) > 0 && Count(r.x + r.w - 40, r.y, 40, r.h, DEV_INK) > 0);
        assert(Count(r.x, r.y, 60, r.h, DEV_INK) == 0 && Count(r.x + r.w, r.y, kDevSaveSprite.x - r.x - r.w, r.h, DEV_INK) == 0);
    }
    assert(Count(kDevSaveSprite.x, kDevSaveSprite.y, kDevSaveSprite.w, kDevSaveSprite.h, red) == 64 * 64);
    assert(at(16, 16) == DEV_PAPER && at(kDevSaveSprite.x - 1, 60) == DEV_PAPER && at(kDevSaveSprite.x + 64, 60) == DEV_PAPER);
    assert(Count(paper.x, paper.y, paper.w, paper.h, DEV_INK) > 0 && Count(paper.x, paper.y, paper.w, paper.h, DEV_INK2) == 0);
    AssertKey(kDevAskYes, DEV_LOOK_KEY, 1); AssertKey(kDevAskNo, DEV_LOOK_KEY, 0);
    assert(Count(216, 156, 96, 39, DEV_LED) > 0 && Count(216, 198, 96, 37, DEV_LED) == 0);
    assert(Count(216, 156, 96, 39, DEV_SUNK_INK) > 0 && Count(216, 198, 96, 37, DEV_KEY_INK) > 0);
    for (int x = 0; x < W; x += 4)
        for (int y = 0; y < H; y += 4)
            assert(!CtrPokeTouch_BackHit(&v, x, y));
    /* a second line (another game saved): the subject in the paper's ink, the line under it in the secondary ink */
    SaveDialogue(&v, SAVE_CONFIRM, 1); DevCheckTouch(&old, &v);
    assert(Count(paper.x, paper.y, paper.w, 32, DEV_INK) > 0 && Count(paper.x, paper.y + 32, paper.w, 32, DEV_INK2) > 0);
    assert(Count(paper.x, paper.y + 32, paper.w, 32, DEV_INK) == 0);
    /* a touch sinks the key under it, whatever the cursor; a result's OK alone, sunk (it is the cursor), NO's place the
     * shell's; an error's OK too; while writing no key at all */
    v.savePressed = PT_SAVE_SECOND; DevCheckTouch(&old, &v); AssertKey(kDevAskNo, DEV_LOOK_KEY, 1); AssertKey(kDevAskYes, DEV_LOOK_KEY, 0);
    v.savePressed = PT_SAVE_NONE; SaveDialogue(&v, SAVE_SUCCESS, 0); v.saveSel = 1; DevCheckTouch(&old, &v);
    AssertKey(kDevAskYes, DEV_LOOK_KEY, 1); assert(at(260, 215) == DEV_SHELL_RED && SaveHitV(&v, 260, 215) == PT_SAVE_NONE);
    SaveDialogue(&v, SAVE_ERROR, 0); DevCheckTouch(&old, &v); AssertHits(SaveHitV, &v, kDevAskYes, PT_SAVE_FIRST);
    assert(SaveHitV(&v, 216, 198) == PT_SAVE_NONE && at(260, 215) == DEV_SHELL_RED);
    SaveDialogue(&v, SAVE_WRITING, 0); DevCheckTouch(&old, &v);
    assert(at(220, 170) == DEV_SHELL_RED && at(300, 220) == DEV_SHELL_RED && SaveHitV(&v, 220, 170) == PT_SAVE_NONE
           && SaveHitV(&v, 300, 220) == PT_SAVE_NONE && Count(paper.x, paper.y, paper.w, paper.h, DEV_INK2) > 0);

    /* the values: no Pokedex says "---" (and its count is not drawn), v0.6.0's caps (999 hours, 59 minutes, ¥999999) */
    v = SaveBoard(); DevCheckTouch(&old, &v);
    v.saveHasDex = 0; assert(DevWhole(DevCheckTouch(&old, &v)));
    v.saveDex = 7; assert(DevCheckTouch(&old, &v).count == 0);
    a = b = SaveBoard(); a.saveHours = 999; b.saveHours = 1000; TouchSamePixels(&a, &b, 1, "999 and 1000 hours");
    a.saveMinutes = 59; b.saveMinutes = 200; TouchSamePixels(&a, &b, 1, "59 and 200 minutes");
    a.saveMoney = 999999; b.saveMoney = 1000000; TouchSamePixels(&a, &b, 1, "999999 and 1000000");
    b = a; b.saveBadges = 10; TouchSamePixels(&a, &b, 0, "8 and 10 badges");   /* the stub's digits look alike: */
    b = a; b.saveDex = 15; TouchSamePixels(&a, &b, 0, "151 and 15 caught");    /* a count of another length */
    /* what SAVE does not draw presents nothing: BACK's fields, HOME's keys, the map, OPTIONS' */
    v = SaveBoard(); DevCheckTouch(&old, &v);
    v.backVisible = v.backEnabled = v.backPressed = 1; v.enabled[PT_BAG] = 1; v.playerX = 3;
    v.optCount = 2; strcpy(v.optLabel[0], "Text speed");
    assert(DevCheckTouch(&old, &v).count == 0);
    v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(1, 100) == DEV_SHELL_GREEN);
    v.shell = 0; DevCheckTouch(&old, &v);
    v.playerFemale = 1; assert(DevWhole(DevCheckTouch(&old, &v)));
    strcpy(v.playerName, "Leaf"); assert(DevWhole(DevCheckTouch(&old, &v)));
    /* the ways in and out: HOME, SAVE, the GBA picture (in the frame's screen: nothing of SAVE drawn) */
    other = v; other.save = 0; other.home = 1; other.playerX = other.playerY = other.cursorX = other.cursorY = -1;
    assert(DevWhole(DevCheckTouch(&old, &other)));
    v = SaveBoard(); assert(DevWhole(DevCheckTouch(&old, &v)));
    v.viewport = 1; assert(DevWhole(DevCheckTouch(&old, &v)) && DrawnBy(&v, DevFrameShown));
    v = SaveBoard(); DevCheckTouch(&old, &v);
    SaveUpperCase(SaveBoard());
    CtrPokeTouch_Draw(canvas, &old);

    /* The states for the eye, each through Update first. */
    v = SaveBoard(); DevCheckTouch(&old, &v); shot("confirm");
    v.saveSel = 1; DevCheckTouch(&old, &v); shot("no");
    v.saveSel = 0; SaveDialogue(&v, SAVE_CONFIRM, 1); DevCheckTouch(&old, &v); shot("overwrite");
    SaveDialogue(&v, SAVE_WRITING, 0); DevCheckTouch(&old, &v); shot("writing");
    SaveDialogue(&v, SAVE_SUCCESS, 0); DevCheckTouch(&old, &v); shot("saved");
    SaveDialogue(&v, SAVE_ERROR, 0); DevCheckTouch(&old, &v); shot("error");
    iconArt = 0;
    sPlayer.key = -1;
    assert(sDevKeyCut == 0);                      /* main prints PASS device save */
}
