/* SAVE as the Definitiva (boards def_save_confirm, def_save_writing, def_save_saved; plan 04b Task 4), included by
 * 3ds_poketouch.c after OPTIONS: a row of the field dispatcher (3ds_device_field_impl.h), shown while view->save and
 * there is no GBA picture (DevSaveShown).
 *
 * Its own screen (kDevSaveScreen): the place (bar[0]) as the title, a rule, the save's facts a row each (PLAYER, BADGES,
 * POKeDEX, TIME, MONEY: the label small at the row's left, the value ending at its right) beside the player's own map
 * icon (HOME's) at 4x, a rule, and under it the dialogue (bar[1] and bar[2] joined): one line when it fits the rule,
 * else two small ones. Under the screen, asking: YES wide and NO dark in the exit corner, the game's cursor sunk (or the
 * key under a held touch); writing: the same two beige and not hit, the dialogue small at the left and the bar of the
 * port's own steps at its right (saveStep of PT_SAVE_STEPS: the writing screen is shown, then the game is written; no
 * wait is added for it); a result (saved, an error): OK across both places, up (a lone OK), sunk only under a touch.
 * No BACK: NO and B leave. Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): the port's labels and dialogue in capitals as SaveFill gives them; the place (FireRed's name) and
 * YES / NO / OK through DevUpper; the player's name as typed (its row, the saved line); the values with v0.6.0's caps
 * (999 hours, 59 minutes, ¥999999). A title or dialogue that does not fit is counted in sDevKeyCut.
 *
 * Redraw: the screen whole when what it draws differs (DevSaveSame), and nothing else. */

/* The dialogue's small lines from capitals 118, 8 apart (Call 15): at the board's 120 / 129 the second's descenders
 * would fall on the bezel (the paper ends at 133). */
#define DEV_SAVE_SMALL_LINE 8

static int DevSaveShown(const CtrPokeTouchView *v)
{
    return v->save && !v->viewport;
}

/* The keys a state draws: YES and NO while asking and (beige) while writing, OK alone after a result. */
static int DevSaveKeyCount(int state)
{
    return state == SAVE_CONFIRM || state == SAVE_WRITING ? 2 : 1;
}

/* Key k of a state: YES (0) and NO (1) in their places, or a result's OK across both. */
static const CtrPokeTouchRect *DevSaveKeyRect(int state, int k)
{
    if (state != SAVE_CONFIRM && state != SAVE_WRITING)
        return &kDevSaveOk;
    return k ? &kDevSaveNo : &kDevSaveYes;
}

int CtrPokeTouch_SaveHit(int x, int y, int state)
{
    if (state == SAVE_WRITING)
        return PT_SAVE_NONE;                /* YES and NO beige: not for now */
    for (int k = 0; k < DevSaveKeyCount(state); ++k)
        if (DevOnKey(DevSaveKeyRect(state, k), x, y))
            return PT_SAVE_FIRST + k;
    return PT_SAVE_NONE;
}

/* Key k sunk: asking, the one under a held touch, else the game's cursor; a result's OK only under a touch; none while
 * the game is written. */
static int DevSaveSunk(const CtrPokeTouchView *v, int k)
{
    if (v->saveState == SAVE_CONFIRM)
        return DevChosen(k, v->saveSel, (int)v->savePressed - PT_SAVE_FIRST);
    return v->saveState != SAVE_WRITING && v->savePressed == PT_SAVE_FIRST;
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

/* Fact i's label (the port's words). */
static const char *DevSaveLabel(int i)
{
    switch (i) {
    case 0: return CTR_TR("PLAYER", "JUGADOR", "SPIELER");
    case 1: return CTR_TR("BADGES", "MEDALLAS", "ORDEN");
    case 2: return CTR_TR("POK\xC3\xA9" "DEX", "POK\xC3\xA9" "DEX", "POK\xC3\xA9" "DEX");
    case 3: return CTR_TR("TIME", "TIEMPO", "ZEIT");
    default: return CTR_TR("MONEY", "DINERO", "GELD");
    }
}

/* Fact i's value: the name as typed, the counts, "---" without a Pokedex, the play time and the money (v0.6.0's caps). */
static void DevSaveValue(const CtrPokeTouchView *v, int i, char *out, size_t size)
{
    switch (i) {
    case 0: snprintf(out, size, "%s", v->playerName); break;
    case 1: snprintf(out, size, "%u", (unsigned)v->saveBadges); break;
    case 2:
        if (v->saveHasDex)
            snprintf(out, size, "%u", (unsigned)v->saveDex);
        else
            snprintf(out, size, "---");
        break;
    case 3: snprintf(out, size, "%u:%02u", (unsigned)(v->saveHours > 999 ? 999 : v->saveHours),
                     (unsigned)(v->saveMinutes > 59 ? 59 : v->saveMinutes)); break;
    default: snprintf(out, size, "\xC2\xA5%u", (unsigned)(v->saveMoney > 999999u ? 999999u : v->saveMoney)); break;
    }
}

/* The dialogue: bar[1], and bar[2] after a space when there is one, into out. */
static void DevSaveDialogue(const CtrPokeTouchView *v, char *out, size_t size)
{
    snprintf(out, size, "%s%s%s", v->bar[1], v->bar[2][0] ? " " : "", v->bar[2]);
}

/* The place (bar[0], FireRed's name) in capitals as the screen's title: from the rules' left (20) with its capitals 3
 * into the paper (17, the board's), the title's ink over the paper's shadow, ending by the rules' right (300); one wider
 * is cut with "..." there and counted. */
static void DevSaveTitle(const CtrPokeTouchView *v, const CtrPokeTouchRect *paper)
{
    const CtrPokeTouchRect *rule = &kDevSaveRule[0];
    char place[PT_BAR_LEN], cut[64];
    const char *s = place;
    int right = rule->x + rule->w;

    DevUpper(place, v->bar[0], sizeof(place));
    if (DevTextWidth(place) > rule->w) {
        DevCutText(place, rule->w, cut, sizeof(cut));
        s = cut;
        ++sDevKeyCut;
    }
    DevTextInk(s, rule->x, paper->y + 3, right, DEV_TITLE_INK, DEV_INK_SH);
}

/* The facts, a row each (DevSaveRow): the label small in the secondary ink at its left, the value in the paper's ink
 * ending at its right (DevColumnRow). */
static void DevSaveFacts(const CtrPokeTouchView *v)
{
    char value[24];

    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevSaveRow(i);

        DevSaveValue(v, i, value, sizeof(value));
        DevColumnRow(r.y, DevSaveLabel(i), value, r.x, r.x + r.w);
    }
}

/* The player's own map icon (HOME's, Red's or Leaf's) at 4x. */
static void DevSaveSprite(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kDevSaveSprite;

    DevArt(DEV_ART_SPRITE, r->x, r->y, r->w, r->h);
    if (sPlayer.key != v->playerFemale)
        IconBuild(&sPlayer, v->playerFemale, CtrPokeTouch_MapPlayerIcon);
    if (sPlayer.ok)
        UiImage(sPlayer.pixels[0], sPlayer.pal, MARKER, MARKER, MARKER, r->x, r->y, r->w / MARKER, 1);
}

/* The dialogue under the second rule, from its left, in the paper's ink; the board's text box starts 4 under the rule
 * (118). While the game is written: small from capitals 118 in two lines DEV_SAVE_SMALL_LINE apart, ending 6 px left of
 * the bar (kDevSaveBar: the port's steps taken of PT_SAVE_STEPS filled in the EXP blue). Otherwise one normal line
 * (capitals 121) when it fits the rule's width, else two small lines ending by the rule's right. Text that needs a third
 * small line is cut there and counted (DevSmallFit). */
static void DevSaveSay(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *rule = &kDevSaveRule[1], *bar = &kDevSaveBar;
    char text[2 * PT_BAR_LEN];
    int x = rule->x, right = rule->x + rule->w, top = rule->y + 4;

    DevSaveDialogue(v, text, sizeof(text));
    if (v->saveState == SAVE_WRITING) {
        DevSmallFit(text, x, x, top, bar->x - 6, 2, DEV_SAVE_SMALL_LINE, DEV_INK);
        DevBar(bar->x, bar->y, bar->w, bar->h, (bar->w - 2) * v->saveStep / PT_SAVE_STEPS, DEV_EXP);
    } else if (DevTextWidth(text) <= rule->w) {
        DevText(text, x, top + 3, right);
    } else {
        DevSmallFit(text, x, x, top, right, 2, DEV_SAVE_SMALL_LINE, DEV_INK);
    }
}

/* The state's keys, their words (DevUpper'd) in the normal font: asking, YES cream and NO dark, the cursor or the key
 * under a touch sunk; writing, the same two beige and up; a result, OK cream across both, up, sunk under a touch. */
static void DevSaveKeys(const CtrPokeTouchView *v)
{
    int n = DevSaveKeyCount(v->saveState);
    char word[24];

    for (int k = 0; k < n; ++k) {
        const CtrPokeTouchRect *r = DevSaveKeyRect(v->saveState, k);
        int look = v->saveState == SAVE_WRITING ? DEV_LOOK_FAINT : k ? DEV_LOOK_DARK : DEV_LOOK_KEY;
        int sunk = DevSaveSunk(v, k), faceY = DevKey(r, look, sunk, 6);

        DevUpper(word, CtrUiText(n == 1 ? UIS_OK : k ? UIS_NO : UIS_YES), sizeof(word));
        DevKeyWord(r, faceY, look, sunk, word);
    }
}

/* SAVE whole (after DevBegin: the dispatcher's). */
static void DevSaveDraw(const CtrPokeTouchView *v)
{
    CtrPokeTouchRect paper;

    DevShell();
    paper = DevScreen(&kDevSaveScreen);
    DevSaveTitle(v, &paper);
    for (int i = 0; i < 2; ++i)
        DevRect(kDevSaveRule[i].x, kDevSaveRule[i].y, kDevSaveRule[i].w, kDevSaveRule[i].h, DEV_RULE);
    DevSaveFacts(v);
    DevSaveSprite(v);
    DevSaveSay(v);
    DevSaveKeys(v);
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing SAVE draws differs between `a` and `b` (the shell is the dispatcher's): the state, the place and the dialogue,
 * the keys sunk (the cursor while asking, OK's press after a result), the bar's steps while writing, the facts and the
 * icon. */
static int DevSaveSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    char x[24], y[24];

    if (a->saveState != b->saveState || a->playerFemale != b->playerFemale)
        return 0;
    if (b->saveState == SAVE_WRITING && a->saveStep != b->saveStep)
        return 0;
    for (int i = 0; i < PT_BAR_LINES; ++i)
        if (strcmp(a->bar[i], b->bar[i]))
            return 0;
    for (int k = 0; k < DevSaveKeyCount(b->saveState); ++k)
        if (DevSaveSunk(a, k) != DevSaveSunk(b, k))
            return 0;
    for (int i = 0; i < 5; ++i) {
        DevSaveValue(a, i, x, sizeof(x));
        DevSaveValue(b, i, y, sizeof(y));
        if (strcmp(x, y))
            return 0;
    }
    return 1;
}

/* SAVE has no BACK. */
static const CtrPokeTouchRect *DevSaveBackKey(const CtrPokeTouchView *v)
{
    (void)v;
    return NULL;
}
