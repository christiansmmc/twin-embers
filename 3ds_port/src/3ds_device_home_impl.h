/* HOME as the red device (style B; board B · Início), included by 3ds_poketouch.c right after the kit. Every rectangle is
 * 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * The shell, a screen and eight keys under it. The screen's paper holds, on the left, the map's window: FireRed's own
 * Region Map at 1:1 (CtrPokeTouch_MapPrepare composes it once per map), the player's icon on his cell, the destination
 * as FireRed's own cursor (it blinks) with its name on a small window, the region key (a touch goes to the next map the
 * game has opened, when there is one) and FLY while the destination can be flown to. On the right, the panel: the
 * place's name, the region line (or why a destination cannot be flown to), the party (each Pokemon's icon over its HP
 * bar, a ring for a hole) and TIME, FireRed's play clock. The keys, two rows of four: the six START entries (the pack's
 * icon beside the word), RUN (its word beside its status LED: lit while the running mode is on, a dim disc otherwise)
 * and SPD (its label beside the turbo's multiplier); each sunk with its LED under a held touch, dimmed while it cannot
 * be used.
 *
 * Words: upper case, as the board and FireRed write them (DevUpper), port words and game strings alike; the player's name
 * as typed.
 *
 * Redraw (decision 7): a change of anything HOME draws redraws it whole; a step of the player or a blink of the cursor
 * redraws the window alone. */

#define DEV_HOME_HOLE          UI_RGB(0xC4, 0xC4, 0xC4)   /* an empty party cell's ring */
#define DEV_HOME_ICON          16                          /* a key's icon */
#define DEV_HOME_GAP           6                           /* between a key's two pieces: icon and word, word and LED, label
                                                            * and multiplier */
#define DEV_HOME_LED           6                           /* RUN's status LED, its top DEV_HOME_LED_TOP under the face's */
#define DEV_HOME_LED_TOP       14
#define DEV_HOME_LABEL_H       17                          /* the destination's name window */

/* FireRed's map cursor (two frames). */
static struct Icon sCursor = {-1, 0, 2, {{0}}, {0}};

static int DevHomeShown(const CtrPokeTouchView *v)
{
    return v->home && !v->viewport;
}

/* ---- The map's window and its cells ---------------------------------------------------------------------------------- */

bool CtrPokeTouch_HomeWindowPoint(int x, int y, int *vx, int *vy)
{
    x -= PT_HOME_X;
    y -= PT_HOME_Y;
    if (x < 0 || y < 0 || x >= PT_HOME_W || y >= PT_HOME_H)
        return false;
    *vx = x;
    *vy = y;
    return true;
}

/* Cell (cx, cy) is the Region Map screen's 8x8 at (32 + 8 cx, 32 + 8 cy); the window shows that screen from
 * (PT_HOME_SRC_X, PT_HOME_SRC_Y) at (PT_HOME_X, PT_HOME_Y). The view does not move the window (a fixed crop). */
bool CtrPokeTouch_HomeCellPoint(const CtrPokeTouchView *v, int cx, int cy, int *x, int *y)
{
    (void)v;
    if (cx < 0 || cy < 0 || cx >= MAP_CELLS_X || cy >= MAP_CELLS_Y)
        return false;
    *x = PT_HOME_X + 36 + 8 * cx - PT_HOME_SRC_X;
    *y = PT_HOME_Y + 36 + 8 * cy - PT_HOME_SRC_Y;
    return true;
}

bool CtrPokeTouch_HomeMapCell(const CtrPokeTouchView *v, int x, int y, int *cx, int *cy)
{
    int vx, vy, sx, sy;

    (void)v;
    if (!CtrPokeTouch_HomeWindowPoint(x, y, &vx, &vy))
        return false;
    sx = vx + PT_HOME_SRC_X - 32;
    sy = vy + PT_HOME_SRC_Y - 32;
    if (sx < 0 || sy < 0 || sx / 8 >= MAP_CELLS_X || sy / 8 >= MAP_CELLS_Y)
        return false;
    *cx = sx / 8;
    *cy = sy / 8;
    return true;
}

/* The first row under the window's top band: the region key and FLY with their drops, and a row of air. */
static int DevHomeKeysBottom(void)
{
    int fly = kDevHomeFly.y + kDevHomeFly.h, key = kDevHomeMapKey.y + kDevHomeMapKey.h;

    return (fly > key ? fly : key) + DEV_DROP + 1;
}

/* The destination's name window: centred over its cursor, 3 px above it, or below it when that would reach into the band
 * of the region key and FLY (and never into that band), kept 2 px inside the window (v0.6.0's rule otherwise); false
 * with no destination or no name. */
static int DevHomeLabelRect(const CtrPokeTouchView *v, CtrPokeTouchRect *r)
{
    int x, y, w, left = PT_HOME_X + 2, right = PT_HOME_X + PT_HOME_W - 2, top = DevHomeKeysBottom();
    int bottom = PT_HOME_Y + PT_HOME_H - 2, ly;
    char name[PT_BAR_LEN];

    if (!v->bar[0][0] || !CtrPokeTouch_HomeCellPoint(v, v->cursorX, v->cursorY, &x, &y))
        return 0;
    DevUpper(name, v->bar[0], sizeof(name));
    w = DevTextWidth(name) + 12;
    if (w > right - left)
        w = right - left;
    x -= w / 2;
    ly = y - MARKER / 2 - 3 - DEV_HOME_LABEL_H;
    if (ly < top)
        ly = y + MARKER / 2 + 3;
    if (ly < top)
        ly = top;
    if (ly + DEV_HOME_LABEL_H > bottom)
        ly = bottom - DEV_HOME_LABEL_H;
    if (x + w > right)
        x = right - w;
    if (x < left)
        x = left;
    r->x = (int16_t)x;
    r->y = (int16_t)ly;
    r->w = (int16_t)w;
    r->h = DEV_HOME_LABEL_H;
    return 1;
}

/* ---- The window -------------------------------------------------------------------------------------------------------- */

/* The region key: the shown map's name, with ▶ when the game has opened another map (a touch on it goes there). */
static void DevHomeMapKey(const CtrPokeTouchView *v)
{
    char name[16];
    int multi = (v->mapOpen & (v->mapOpen - 1)) != 0, tw, faceY;
    CtrPokeTouchRect r = kDevHomeMapKey;

    DevUpper(name, RegionName(v->mapGroup), sizeof(name));
    tw = DevTextWidth(name);

    r.w = (int16_t)(tw + 14 + (multi ? 9 : 0));
    if (r.w > kDevHomeMapKey.w)
        r.w = kDevHomeMapKey.w;
    faceY = DevKey(&r, DEV_LOOK_DARK, 0, 4);
    DevTextInk(name, r.x + 7, faceY + (r.h - UI_TEXT_H) / 2, r.x + r.w - 7 - (multi ? 9 : 0), DEV_DARK_INK, DEV_DARK_INK);
    if (multi)
        DevTriangle(r.x + r.w - 10, faceY + r.h / 2, 3, DEV_DARK_INK);
    sHomeRibbon = multi ? DevKeyArea(&r) : (CtrPokeTouchRect){0, 0, 0, 0};
}

/* FLY: while the destination can be flown to now. */
static void DevHomeFly(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kDevHomeFly;
    char fly[16];
    int faceY;

    DevUpper(fly, CTR_TR("Fly", "Volar", "Fliegen"), sizeof(fly));
    sFlyButton = (CtrPokeTouchRect){0, 0, 0, 0};
    if (!v->flyOk)
        return;
    faceY = DevKey(r, DEV_LOOK_KEY, 0, 4);
    DevFitWord(fly, DevFitFont(fly, r->w - 8, r->w - 8), r->x + 4, r->w - 8, faceY, r->h, 1, DEV_KEY_INK, DEV_WHITE);
    sFlyButton = DevKeyArea(r);
}

/* The destination's name on paper with a dark edge. */
static void DevHomeLabel(const CtrPokeTouchView *v)
{
    CtrPokeTouchRect r;
    char name[PT_BAR_LEN];

    if (!DevHomeLabelRect(v, &r))
        return;
    DevUpper(name, v->bar[0], sizeof(name));
    Fill(r.x, r.y, r.w, r.h, DEV_BEZEL);
    Fill(r.x + 1, r.y + 1, r.w - 2, r.h - 2, DEV_PAPER);
    DevText(name, r.x + 6, r.y + 4, r.x + r.w - 5);
}

/* The window whole: the map (the sea's blue without its art), the player, the destination's cursor and name, the region
 * key and FLY. Paints only the window. */
static void DevHomeWindow(const CtrPokeTouchView *v)
{
    int x, y;

    if (MapReady(v))
        for (int dx = 0; dx < PT_HOME_W; ++dx)
            memcpy(sCanvas + (size_t)(PT_HOME_X + dx) * H + (H - PT_HOME_Y - PT_HOME_H), sHomeMapCols[dx], sizeof(sHomeMapCols[dx]));
    else
        Fill(PT_HOME_X, PT_HOME_Y, PT_HOME_W, PT_HOME_H, C_MAP_WAIT);
    if (sPlayer.key != v->playerFemale)
        IconBuild(&sPlayer, v->playerFemale, CtrPokeTouch_MapPlayerIcon);
    if (CtrPokeTouch_HomeCellPoint(v, v->playerX, v->playerY, &x, &y))
        IconDraw(&sPlayer, 0, x, y);
    if (CtrPokeTouch_HomeCellPoint(v, v->cursorX, v->cursorY, &x, &y)) {
        if (sCursor.key != 0)
            IconBuild(&sCursor, 0, CtrPokeTouch_MapCursorIcon);
        IconDraw(&sCursor, v->cursorFrame, x, y);
    }
    DevHomeLabel(v);
    DevHomeMapKey(v);
    DevHomeFly(v);
}

/* ---- The panel --------------------------------------------------------------------------------------------------------- */

/* The party: each Pokemon's icon (half size; a fainted one faded) over its HP bar (an egg has none, as in FireRed), a
 * ring for a hole, a dark edge round the one under a held touch; where the board has them (DevHomeCell's places). */
static void DevHomeParty(const CtrPokeTouchView *v)
{
    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect c = DevHomeCell(i);

        if (!v->party[i]) {
            DevRing(c.x + 7, c.y + 4, 16, DEV_HOME_HOLE);
            continue;
        }
        if (v->pressed == PT_HOME_PARTY + i) {
            DevRound(c.x, c.y, c.w, c.h, 4, DEV_BEZEL);
            DevRound(c.x + 1, c.y + 1, c.w - 2, c.h - 2, 3, DEV_PAPER);
        }
        DevMonIcon(i, v->partyIcon[i], c.x + 7, c.y + 4, 1, v->party[i] == 2);
        if (!(v->partyEgg >> i & 1))
            DevHpBar(c.x + 4, c.y + 25, 22, 4, v->partyHp[i], 100);
    }
}

/* TIME: FireRed's play clock (the hours and minutes SAVE shows, with its caps), small in the secondary ink. */
static void DevHomeTime(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kDevHomeTime;
    char time[24];

    snprintf(time, sizeof(time), "%s %u:%02u", CTR_TR("TIME", "TIEMPO", "ZEIT"),
             (unsigned)(v->saveHours > 999 ? 999 : v->saveHours), (unsigned)(v->saveMinutes > 59 ? 59 : v->saveMinutes));
    DevSmallText(time, r->x, r->y + 2, DEV_INK2, r->x + r->w);
}

/* `s` in the small font in at most maxLines lines DEV_LINE_H apart, `width` wide, broken at spaces (DevWrap's rule) and
 * at a line break, which always ends its line (the Party's FLY's hint: where to tap, then the way out). A word wider
 * than a line is cut, and words left over are dropped: either counts in sDevKeyCut (the label-fit builds then fail), as
 * a cut key label does. */
static void DevHomeSmallWrap(const char *s, int x, int top, int width, int maxLines, uint16_t ink)
{
    char line[PT_BAR_LEN];

    for (int lines = 0; lines < maxLines; ++lines) {
        int fit = 0, end = 0;

        while (*s == ' ' || *s == '\n')
            ++s;
        if (!*s)
            return;
        for (;;) {
            while (s[end] && s[end] != ' ' && s[end] != '\n' && end < (int)sizeof(line) - 1)
                ++end;
            memcpy(line, s, (size_t)end);
            line[end] = 0;
            if (fit > 0 && DevSmallTextWidth(line) > width)
                break;
            fit = end;
            if (!s[end] || s[end] == '\n' || end >= (int)sizeof(line) - 1)
                break;
            ++end;                          /* past the space, to the next word */
        }
        memcpy(line, s, (size_t)fit);
        line[fit] = 0;
        if (DevSmallTextWidth(line) > width)
            ++sDevKeyCut;
        DevSmallText(line, x, top + lines * DEV_LINE_H, ink, x + width);
        s += fit;
    }
    while (*s == ' ' || *s == '\n')
        ++s;
    if (*s)
        ++sDevKeyCut;
}

/* The panel: the place's name (two lines), the line under it (the map's region, as the region key names it; or bar[2]
 * when 3ds_bottom.c has something to say there: why a place picked cannot be flown to, the dungeon over it, the Party's
 * FLY's hint; a line too long for the panel, or broken in two, takes the name's second line too when the name has left
 * it free), the rule, the party and TIME. */
static void DevHomePanel(const CtrPokeTouchView *v)
{
    char name[PT_BAR_LEN], line[PT_BAR_LEN];
    int nameLines, two;

    DevUpper(name, v->bar[0], sizeof(name));
    DevUpper(line, v->bar[2][0] ? v->bar[2] : RegionName(v->mapGroup), sizeof(line));
    nameLines = DevWrap(name, kDevHomeName.x, kDevHomeName.y + 3, kDevHomeName.w, 2, DEV_INK, DEV_INK_SH);
    two = nameLines < 2 && (strchr(line, '\n') || DevSmallTextWidth(line) > kDevHomeRegion.w);

    DevHomeSmallWrap(line, kDevHomeRegion.x, kDevHomeRegion.y + 2 - (two ? DEV_LINE_H : 0), kDevHomeRegion.w, two ? 2 : 1,
                     DEV_INK2);
    Fill(kDevHomeRule.x, kDevHomeRule.y, kDevHomeRule.w, kDevHomeRule.h, DEV_RULE);
    DevHomeParty(v);
    DevHomeTime(v);
}

/* ---- The keys ---------------------------------------------------------------------------------------------------------- */

/* A key's icon: the pack's soft 16x16 of the entry's item icon (sMenuItemArt), read once; NULL without the pack. */
static const PtSoft *DevHomeIcon(int id)
{
    static struct {
        int tried;
        PtSoft soft;
    } icons[6];

    if (!icons[id].tried) {
        icons[id].tried = 1;
        if (!PokeTouch_SoftIcon(sMenuItemArt[id], &icons[id].soft))
            icons[id].soft.ok = 0;
    }
    return icons[id].soft.ok ? &icons[id].soft : NULL;
}

/* A START entry's word (PT_POKEDEX..PT_OPTIONS): the entry's, or for the Trainer Card the player's name as typed (into
 * `name`). */
static const char *DevHomeWord(int id, const CtrPokeTouchView *v, char name[16])
{
    const char *word = id == PT_POKEDEX ? CTR_TR("Pok\xC3\xA9" "dex", "Pok\xC3\xA9" "dex", "Pok\xc3\xa9" "dex")
                     : id == PT_POKEMON ? CTR_TR("Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xc3\xa9" "mon")
                     : id == PT_BAG ? CTR_TR("Bag", "Mochila", "Beutel")
                     : id == PT_SAVE ? CTR_TR("Save", "Guardar", "Sichern") : id == PT_OPTIONS ? CTR_TR("Option", "Ajustes", "Option") : name;
    int n = 0;

    if (id == PT_CARD)
        for (; n < 15 && v->playerName[n]; ++n)
            name[n] = v->playerName[n];
    name[n] = 0;
    return word;
}

/* A key's word (into `out`): the entry's in capitals; the card's, the player's name as typed. */
static const char *DevHomeKeyWord(int id, const CtrPokeTouchView *v, char out[24])
{
    char name[16];
    const char *word = DevHomeWord(id, v, name);

    DevUpper(out, word, 24);
    return id == PT_CARD ? strcpy(out, name) : out;
}

/* A key's word in its own box at x, `w` wide (DevKeyLabel centres it there, and cuts and counts only a word wider than
 * w): a pair's word or multiplier. On HOME's 34 px keys a word's rows lie below a sunk key's LED, so no word has to
 * move off it (DevKeyWord's case). */
static void DevHomeWordAt(const CtrPokeTouchRect *r, int faceY, int sunk, int x, int w, const char *word)
{
    CtrPokeTouchRect box = {(int16_t)(x - 4), r->y, (int16_t)(w + 8), r->h};

    DevKeyLabel(&box, faceY, DEV_LOOK_KEY, sunk, word);
}

/* A START entry's key: the icon, 6 px, the word (the entry's, the player's name for the card), the pair centred; sunk
 * with its LED under a held touch; dimmed while it cannot be used. Without the pack the port's own 12x12 silhouette in
 * the key's ink stands for the icon. */
static void DevHomeKey(int id, const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kDevHomeKey[id];
    int sunk = v->enabled[id] && v->pressed == id, faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);
    int room = r->w - 8 - DEV_HOME_ICON - DEV_HOME_GAP, lw, x0, iy = faceY + (r->h - DEV_HOME_ICON) / 2;
    const PtSoft *soft = DevHomeIcon(id);
    char out[24];
    const char *label = DevHomeKeyWord(id, v, out);

    lw = DevTextWidth(label);
    if (lw > room)
        lw = room;
    x0 = r->x + (r->w - (DEV_HOME_ICON + DEV_HOME_GAP + lw)) / 2;
    if (soft)
        PokeTouch_SoftDraw(soft, x0, iy, 1);
    else
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 12; ++x)
                if ((sTwinIconRows[id][y] >> (11 - x)) & 1u)
                    Px(x0 + 2 + x, iy + 2 + y, sunk ? DEV_SUNK_INK : DEV_KEY_INK);
    DevHomeWordAt(r, faceY, sunk, x0 + DEV_HOME_ICON + DEV_HOME_GAP, lw, label);
    if (!v->enabled[id])
        DevDim(r);
}

/* RUN: the word, 6 px, its status LED (6 px, its top DEV_HOME_LED_TOP under the face's): lit while the running mode is
 * on, a dim disc in the key's edge colour otherwise; the pair centred. Sunk with its own LED under a held touch,
 * dimmed while it cannot be used. CORRER and RENNEN would crowd the LED: the shorter imperatives. */
static void DevHomeRun(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kDevHomeRun;
    int sunk = v->enabled[PT_RUN] && v->pressed == PT_RUN, faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);
    int room = r->w - 8 - DEV_HOME_GAP - DEV_HOME_LED, lw, x0, ledX;
    char run[16];

    DevUpper(run, CTR_TR("Run", "Corre", "Lauf"), sizeof(run));
    lw = DevTextWidth(run);
    if (lw > room)
        lw = room;
    x0 = r->x + (r->w - (lw + DEV_HOME_GAP + DEV_HOME_LED)) / 2;
    DevHomeWordAt(r, faceY, sunk, x0, lw, run);
    ledX = x0 + lw + DEV_HOME_GAP;
    if (v->running)
        DevLedPixels(ledX, faceY + DEV_HOME_LED_TOP, DEV_HOME_LED, DevKeyFace(DEV_LOOK_KEY, sunk));
    else
        DevDisc(ledX, faceY + DEV_HOME_LED_TOP, DEV_HOME_LED, DEV_KEY_EDGE);
    if (!v->enabled[PT_RUN])
        DevDim(r);
}

/* SPD: its label small in the key's second ink, 6 px, the turbo's multiplier ("2x") in the key's ink, the pair centred;
 * sunk with its LED under a held touch, dimmed while a tap cannot change it. */
static void DevHomeSpeed(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kDevHomeSpeed;
    const char *label = CTR_TR("SPD", "VEL", "TMP");
    int sunk = v->speedEnabled && v->pressed == PT_HOME_SPEED, faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);
    int lw = DevSmallTextWidth(label), vw, x0;
    char value[8];

    snprintf(value, sizeof(value), "%ux", v->speed >= 2 && v->speed <= 4 ? (unsigned)v->speed : 1u);
    vw = DevTextWidth(value);
    x0 = r->x + (r->w - (lw + DEV_HOME_GAP + vw)) / 2;
    DevSmallText(label, x0, faceY + (r->h - 7) / 2, DEV_KEY_INK2, x0 + lw);
    DevHomeWordAt(r, faceY, sunk, x0 + lw + DEV_HOME_GAP, vw, value);
    if (!v->speedEnabled)
        DevDim(r);
}

/* ---- HOME --------------------------------------------------------------------------------------------------------------- */

static void DevHomeDraw(const CtrPokeTouchView *v)
{
    DevBegin(sCanvas, v->shell);
    DevShell();
    DevScreen(&kDevHomeScreen);
    DevHomeWindow(v);
    DevHomePanel(v);
    for (int id = PT_POKEDEX; id <= PT_OPTIONS; ++id)
        DevHomeKey(id, v);
    DevHomeRun(v);
    DevHomeSpeed(v);
    sHomeShown = 1;
}

/* What HOME draws but for the window's moving pieces (the player's icon, the cursor's blink): any change of it redraws
 * HOME whole. */
typedef struct {
    uint8_t enabled[PT_OPTIONS + 1], runEnabled, pressed, running, shell;
    uint8_t mapGroup, mapVersion, mapReady, mapOpen, flyOk;
    int8_t cursorX, cursorY;
    uint8_t party[6], partyHp[6], speed, speedEnabled;
    uint16_t partyIcon[6];
    uint8_t partyEgg, saveMinutes;
    uint16_t saveHours;
    char bar[PT_BAR_LINES][PT_BAR_LEN];
    char playerName[16];
} DevHomeLook;

static void DevHomeLookOf(const CtrPokeTouchView *v, DevHomeLook *l)
{
    memset(l, 0, sizeof(*l));
    memcpy(l->enabled, v->enabled, sizeof(l->enabled));
    l->runEnabled = v->enabled[PT_RUN];
    l->pressed = v->pressed;
    l->running = v->running;
    l->shell = v->shell;
    l->mapGroup = v->mapGroup;
    l->mapVersion = v->mapVersion;
    l->mapReady = v->mapReady;
    l->mapOpen = v->mapOpen;
    l->flyOk = v->flyOk;
    l->cursorX = v->cursorX;
    l->cursorY = v->cursorY;
    memcpy(l->party, v->party, sizeof(l->party));
    memcpy(l->partyHp, v->partyHp, sizeof(l->partyHp));
    memcpy(l->partyIcon, v->partyIcon, sizeof(l->partyIcon));
    l->partyEgg = v->partyEgg;
    l->saveMinutes = v->saveMinutes;
    l->saveHours = v->saveHours;
    l->speed = v->speed;
    l->speedEnabled = v->speedEnabled;
    memcpy(l->bar, v->bar, sizeof(l->bar));
    memcpy(l->playerName, v->playerName, sizeof(l->playerName));
}

static void DevHomeUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)
{
    DevHomeLook a, b;

    DevHomeLookOf(old, &a);
    DevHomeLookOf(v, &b);
    if (memcmp(&a, &b, sizeof(a))) {
        DevHomeDraw(v);
        CtrDirty_Add(dirty, 0, 0, W, H);
    } else if (old->playerX != v->playerX || old->playerY != v->playerY || old->playerFemale != v->playerFemale
               || old->cursorFrame != v->cursorFrame) {
        DevBegin(sCanvas, v->shell);
        DevHomeWindow(v);
        sHomeShown = 1;
        CtrDirty_Add(dirty, PT_HOME_X, PT_HOME_Y, PT_HOME_X + PT_HOME_W, PT_HOME_Y + PT_HOME_H);
    }
}

/* What HOME shows under a lower-screen point: a key with its drop (a START entry, RUN, SPD), a cell that holds a
 * Pokemon. */
int CtrPokeTouch_HomeHit(const CtrPokeTouchView *v, int x, int y)
{
    for (int id = PT_POKEDEX; id <= PT_OPTIONS; ++id)
        if (DevOnKey(&kDevHomeKey[id], x, y))
            return id;
    if (DevOnKey(&kDevHomeRun, x, y))
        return PT_RUN;
    if (DevOnKey(&kDevHomeSpeed, x, y))
        return PT_HOME_SPEED;
    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect c = DevHomeCell(i);

        if (v->party[i] && UiIn(&c, x, y))
            return PT_HOME_PARTY + i;
    }
    return PT_NONE;
}
