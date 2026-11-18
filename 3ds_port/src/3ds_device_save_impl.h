/* SAVE as the red device (no board; plan 3), included by 3ds_poketouch.c after OPTIONS: a row of the field dispatcher
 * (3ds_device_field_impl.h), shown while view->save and there is no GBA picture (DevSaveShown).
 *
 * HOME's page. On its screen the save's facts, a row each (PLAYER, BADGES, POKeDEX, TIME, MONEY: the label small, the
 * value at the row's right) and the player's own map icon (HOME's) at 4x. Under it the dialogue on the question's note
 * (its subject, then what it says more in the secondary ink) and, in HOME's third and sixth key places, YES and NO, the
 * game's cursor sunk (or the key under a held touch); after a result (saved, an error) OK alone in YES's place, the
 * cursor; nothing while the game is written. No BACK: NO and B leave. Every rectangle is 3ds_device_layout.h's, so what
 * is drawn is what a touch takes.
 *
 * Words (decision 5): the port's labels in capitals; the dialogue (v0.6.0 title-cased bar[]) and YES / NO / OK through
 * DevUpper; the player's name as typed; the values with v0.6.0's caps (999 hours, 59 minutes, ¥999999).
 *
 * Redraw: the screen whole when what it draws differs (DevSaveSame), and nothing else. */

static int DevSaveShown(const CtrPokeTouchView *v)
{
    return v->save && !v->viewport;
}

/* The keys of a state: YES and NO while asking, OK alone after a result, none while writing. */
static int DevSaveKeyCount(int state)
{
    return state == SAVE_WRITING ? 0 : state == SAVE_CONFIRM ? 2 : 1;
}

static const CtrPokeTouchRect *DevSaveKeyRect(int k)
{
    return k ? &kDevAskNo : &kDevAskYes;
}

int CtrPokeTouch_SaveHit(int x, int y, int state)
{
    for (int k = 0; k < DevSaveKeyCount(state); ++k)
        if (DevOnKey(DevSaveKeyRect(k), x, y))
            return PT_SAVE_FIRST + k;
    return PT_SAVE_NONE;
}

/* Key k sunk: the one under a held touch, else the game's cursor (a lone OK is the cursor). */
static int DevSaveSunk(const CtrPokeTouchView *v, int k)
{
    return DevChosen(k, v->saveState == SAVE_CONFIRM ? v->saveSel : 0, (int)v->savePressed - PT_SAVE_FIRST);
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

/* The dialogue on the question's note: its lines (an empty one left out) centred down the paper, the first in the
 * paper's ink, the others in the secondary ink; a line cut at the paper's edge is counted in sDevKeyCut. */
static void DevSaveNote(const CtrPokeTouchView *v)
{
    CtrPokeTouchRect paper = DevScreen(&kDevAskNote);
    char line[PT_BAR_LINES][PT_BAR_LEN];
    int n = 0, x = paper.x + 6, right = paper.x + paper.w - 6, top;

    for (int i = 0; i < PT_BAR_LINES; ++i)
        if (v->bar[i][0])
            DevUpper(line[n++], v->bar[i], sizeof(line[0]));
    top = paper.y + (paper.h - (UI_TEXT_H + DEV_LINE_H * (n - 1))) / 2;
    for (int i = 0; i < n; ++i) {
        if (DevTextWidth(line[i]) > right - x)
            ++sDevKeyCut;
        if (i)
            DevText2(line[i], x, top + DEV_LINE_H * i, right);
        else
            DevText(line[i], x, top, right);
    }
}

/* YES and NO, or OK alone, or none: the cursor sunk, or the key under a held touch. */
static void DevSaveKeys(const CtrPokeTouchView *v)
{
    int n = DevSaveKeyCount(v->saveState);
    char word[24];

    for (int k = 0; k < n; ++k) {
        const CtrPokeTouchRect *r = DevSaveKeyRect(k);
        int sunk = DevSaveSunk(v, k), faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);

        DevUpper(word, CtrUiText(n == 1 ? UIS_OK : k ? UIS_NO : UIS_YES), sizeof(word));
        DevKeyWord(r, faceY, DEV_LOOK_KEY, sunk, word);
    }
}

/* SAVE whole (after DevBegin: the dispatcher's). */
static void DevSaveDraw(const CtrPokeTouchView *v)
{
    char value[24];

    DevShell();
    DevScreen(&kDevHomeScreen);
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevSaveRow(i);

        DevSaveValue(v, i, value, sizeof(value));
        DevLabelValue(&r, DevSaveLabel(i), value);
    }
    DevSaveSprite(v);
    DevSaveNote(v);
    DevSaveKeys(v);
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing SAVE draws differs between `a` and `b` (the shell is the dispatcher's). */
static int DevSaveSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    char x[24], y[24];

    if (a->saveState != b->saveState || a->playerFemale != b->playerFemale)
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
