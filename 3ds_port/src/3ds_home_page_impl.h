/* HOME as a full-screen page (phase 1 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md, the owner's
 * "F"): FireRed's Region Map at 1:1 in a panel with the standard frame and a caption strip (the place, the region), the
 * entries as rows of a menu window like FireRed's START menu, RUN and the game's speed in a window under it, and the
 * party in a window along the bottom (the Fly's hint has it while the map has a place picked). X and Y are not on the
 * page; their buttons work as ever. Where everything is comes from 3ds_lower_layout.h. Included by 3ds_poketouch.c after Changed and Mark; HOME is this page whenever it is
 * up alone, and every other view keeps the old frame and column until it is reworked. */

#define HOME_MAP_W 192
#define HOME_MAP_H 128

/* The page's own colour behind its panels (provisional, with the layout). */
static uint16_t HomeBg(void)
{
    return KitInk(3);
}

bool CtrPokeTouch_HomePageShown(const CtrPokeTouchView *v)
{
    return v->home && !v->viewport && !v->options && !v->save && !v->bag.on && !v->dex.on && !v->card.on
        && !v->learn.on && !v->name.on && !v->box.on && !v->choice.on;
}

/* A button's touch place on the page: the entries' rows and RUN's half of its window; none for X and Y. */
static const CtrPokeTouchRect *HomeRect(int id)
{
    if (id >= PT_POKEDEX && id <= PT_OPTIONS)
        return &kHomeEntry[id - PT_POKEDEX];
    return id == PT_RUN ? &kHomeRun : NULL;
}

/* A button, SPEED (PT_HOME_SPEED), a Pokemon of the party (PT_HOME_PARTY + its slot; not while the hint has the
 * window), or -1. */
int CtrPokeTouch_HomeHit(const CtrPokeTouchView *v, int x, int y)
{
    for (int i = 0; i < PT_COUNT; ++i) {
        const CtrPokeTouchRect *r = HomeRect(i);

        if (r && UiIn(r, x, y))
            return i;
    }
    if (UiIn(&kHomeSpeed, x, y))
        return PT_HOME_SPEED;
    if (!v->homeHint)
        for (int i = 0; i < 6; ++i)
            if (v->party[i] && UiIn(&kHomePartySlot[i], x, y))
                return PT_HOME_PARTY + i;
    return -1;
}

/* The map shows the Region Map's 192x128 at (PT_HOME_SRC_X, PT_HOME_SRC_Y) from (mx, my), so the 8x8 cell (cx, cy),
 * at (32 + 8 cx, 32 + 8 cy) on the Region Map's screen, is at (mx + 8 + 8 cx, my + 4 + 8 cy). */
bool CtrPokeTouch_HomeMapCell(int x, int y, int *cx, int *cy)
{
    int dx = x - (kHomeMapPanel.x + 8) - (32 - PT_HOME_SRC_X), dy = y - (kHomeMapPanel.y + 8) - (32 - PT_HOME_SRC_Y);

    if (dx < 0 || dy < 0 || dx / 8 >= MAP_CELLS_X || dy / 8 >= MAP_CELLS_Y)
        return false;
    *cx = dx / 8;
    *cy = dy / 8;
    return true;
}

/* The map at 1:1, composed once per (map, version) like HOME's old enlarged one and laid out like the canvas. */
static struct {
    int group, version, ok;
    uint16_t cols[HOME_MAP_W][HOME_MAP_H];
} sHomeMap = {-1, 0, 0, {{0}}};

static void HomeMapBuild(int group, int version)
{
    static uint16_t tilemap[MAP_TILEMAP_W * MAP_TILEMAP_H], bgTilemap[MAP_TILEMAP_W * MAP_TILEMAP_H];
    static uint8_t tiles[MAP_TILES * 32], bgTiles[MAP_TILES * 32];
    static uint16_t gba[MAP_COLORS];
    uint16_t pal[MAP_COLORS];

    sHomeMap.group = group;
    sHomeMap.version = version;
    sHomeMap.ok = group >= 0 && group < PT_MAP_COUNT && CtrPokeTouch_MapArt(group, tilemap, tiles, gba, bgTilemap, bgTiles);
    if (!sHomeMap.ok)
        return;
    for (int i = 0; i < MAP_COLORS; ++i)
        pal[i] = Gba(gba[i]);
    for (int dx = 0; dx < HOME_MAP_W; ++dx)
        for (int dy = 0; dy < HOME_MAP_H; ++dy)
            sHomeMap.cols[dx][HOME_MAP_H - 1 - dy] =
                pal[MapColor(tilemap, tiles, bgTilemap, bgTiles, PT_HOME_SRC_X + dx, PT_HOME_SRC_Y + dy)];
}

/* A map icon at 1:1: its 16x16 is centred on the cell, (mx + 4 + 8 cx, my + 8 cy). */
static void HomeMarker(const struct Icon *icon, int frame, int cx, int cy, int mx, int my)
{
    if (!icon->ok || cx < 0 || cy < 0 || cx >= MAP_CELLS_X || cy >= MAP_CELLS_Y)
        return;
    Art(icon->pixels[frame % icon->frames], icon->pal, MARKER, 0, 0, MARKER, MARKER,
        mx + (28 - PT_HOME_SRC_X) + 8 * cx, my + (28 - PT_HOME_SRC_Y) + 8 * cy, 1);
}

/* The map's picture with the player and the selection on it; white when there is no map. */
static void HomeMap(const CtrPokeTouchView *v)
{
    int mx = kHomeMapPanel.x + 8, my = kHomeMapPanel.y + 8;

    if (sHomeMap.group != v->mapGroup || sHomeMap.version != v->mapVersion)
        HomeMapBuild(v->mapGroup, v->mapVersion);
    if (!sHomeMap.ok) {
        Fill(mx, my, HOME_MAP_W, HOME_MAP_H, KitInk(1));
        return;
    }
    for (int x = 0; x < HOME_MAP_W; ++x)
        memcpy(sCanvas + (size_t)(mx + x) * H + (H - my - HOME_MAP_H), sHomeMap.cols[x], HOME_MAP_H * sizeof(uint16_t));
    if (sPlayer.key != v->playerFemale)
        IconBuild(&sPlayer, v->playerFemale, CtrPokeTouch_MapPlayerIcon);
    HomeMarker(&sPlayer, 0, v->playerX, v->playerY, mx, my);
    if (v->cursorX >= 0) {
        if (sCursor.key != 0)
            IconBuild(&sCursor, 0, CtrPokeTouch_MapCursorIcon);
        HomeMarker(&sCursor, v->cursorFrame, v->cursorX, v->cursorY, mx, my);
    }
}

/* The map panel: its frame (the player's FRAME type), the map, and the caption with bar[0] (the place) on the left and
 * bar[1] (the region) on the right. The page's colour goes under it first: the frame's clear pixels show it. */
static void HomeMapPanel(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kHomeMapPanel;

    Fill(r->x, r->y, r->w, r->h, HomeBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    HomeMap(v);
    KitCaption(&kHomeCaption, v->bar[0], v->bar[1]);
}

/* The six entries' FireRed item icons (the same items the column's pack icons use), read from the pack once. */
static struct {
    int tried, ok;
    uint8_t pixels[24 * 24];
    uint16_t pal[16];
} sHomeIcon[6];

static const uint8_t *HomeEntryIcon(int i, const uint16_t **pal)
{
    if (!sHomeIcon[i].tried) {
        sHomeIcon[i].tried = 1;
        sHomeIcon[i].ok = PokeTouch_PackIcon24(sMenuItemArt[i], sHomeIcon[i].pixels, sHomeIcon[i].pal);
    }
    *pal = sHomeIcon[i].pal;
    return sHomeIcon[i].ok ? sHomeIcon[i].pixels : NULL;
}

/* A row of the menu window, as FireRed's START menu draws it: the arrow on the row held under a touch (or the screen
 * shown), the label greyed while it cannot be used. */
static void HomeRow(int id, const CtrPokeTouchView *v)
{
    int on = v->enabled[id] || v->selected == id;
    int chosen = (v->enabled[id] && v->pressed == id) || v->selected == id;
    char name[16];

    KitRow(HomeRect(id), ColumnLabel(id, v, name), chosen, on);
}

/* The menu window: its frame (the player's FRAME type) and every row. The page's colour goes under it first: the
 * frame's clear pixels show it. */
static void HomeMenu(const CtrPokeTouchView *v)
{
    Fill(kHomeMenu.x, kHomeMenu.y, kHomeMenu.w, kHomeMenu.h, HomeBg());
    KitPanel(&kHomeMenu, KIT_FRAME_STD, v->frameType);
    for (int id = PT_POKEDEX; id <= PT_OPTIONS; ++id)
        HomeRow(id, v);
}

/* The multiplier the field runs at: "1x" with the turbo off, else its own (2x to 4x). */
static void HomeSpeedLabel(unsigned speed, char out[8])
{
    snprintf(out, 8, "%ux", speed >= 2 && speed <= 4 ? speed : 1u);
}

/* A line of RUN and SPEED's window, as a menu row: the arrow while its half is held, the label (greyed while it cannot
 * be used), the value at the line's right. */
static void HomeSettingLine(const CtrPokeTouchRect *line, const char *label, const char *value, int held, int on)
{
    KitRow(line, label, held, on);
    KitTextOn(value, line->x + line->w - UiTextWidth(value, 1), KitLineTop(line), line->x + line->w, on);
}

/* RUN (whether the running mode is on) and SPEED (the turbo's multiplier) in their window. */
static void HomeSettings(const CtrPokeTouchView *v)
{
    const char *run = v->enabled[PT_RUN] && v->running ? CTR_TR("ON", "S\xc3\x8d", "AN") : CTR_TR("OFF", "NO", "AUS");
    char speed[8];

    Fill(kHomeSettings.x, kHomeSettings.y, kHomeSettings.w, kHomeSettings.h, HomeBg());
    KitPanel(&kHomeSettings, KIT_FRAME_STD, v->frameType);
    HomeSpeedLabel(v->speed, speed);
    HomeSettingLine(&kHomeRunLine, CTR_TR("RUN", "CORRER", "Rennen"), run, v->enabled[PT_RUN] && v->pressed == PT_RUN,
                    v->enabled[PT_RUN]);
    HomeSettingLine(&kHomeSpeedLine, CTR_TR("SPEED", "VELOCIDAD", "Tempo"), speed,
                    v->speedEnabled && v->pressed == PT_HOME_SPEED, v->speedEnabled);
}

/* The battle's party strip (3ds_battle_page_impl.h): six slots 48 px apart from (x, y). */
static void PartyStrip(int x, int y, const uint8_t kind[6], const uint8_t hp[6], const uint8_t status[6],
                       const uint16_t icon[6]);

/* The party window: the six as the battle's party bar shows them, or, while the map has a place picked, the Fly's
 * hint (bar[2]) as a message. */
static void HomeParty(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kHomeParty;

    Fill(r->x, r->y, r->w, r->h, HomeBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    if (v->homeHint)
        KitWrap(v->bar[2], r->x + 12, r->y + 10, r->w - 24, 3);
    else
        PartyStrip(kHomePartySlot[0].x + 8, kHomePartySlot[0].y + 7, v->party, v->partyHp, v->partyStatus, v->partyIcon);
}

static void HomePageDraw(const CtrPokeTouchView *v)
{
    Fill(0, 0, W, H, HomeBg());
    HomeMapPanel(v);
    HomeMenu(v);
    HomeSettings(v);
    HomeParty(v);
}

/* What RUN and SPEED's window shows changed: a value, whether one can be used, or which of its halves is held. */
static int HomeSettingsChanged(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return a->running != b->running || a->speed != b->speed || a->speedEnabled != b->speedEnabled
        || a->enabled[PT_RUN] != b->enabled[PT_RUN] || (a->pressed == PT_RUN) != (b->pressed == PT_RUN)
        || (a->pressed == PT_HOME_SPEED) != (b->pressed == PT_HOME_SPEED);
}

/* What the party window shows changed: the hint coming or going, the hint's text, or (without it) the party. */
static int HomePartyChanged(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    if (a->homeHint != b->homeHint)
        return 1;
    if (b->homeHint)
        return strcmp(a->bar[2], b->bar[2]) != 0;
    return memcmp(a->party, b->party, sizeof(a->party)) || memcmp(a->partyHp, b->partyHp, sizeof(a->partyHp))
        || memcmp(a->partyStatus, b->partyStatus, sizeof(a->partyStatus))
        || memcmp(a->partyIcon, b->partyIcon, sizeof(a->partyIcon));
}

/* Only what changed: the framed windows when the FRAME type changes, the map panel for the caption, the map alone for
 * its icons, RUN and SPEED's window, the party's, and the rows whose state changed. Each repaints its whole rectangle,
 * so the result equals HomePageDraw. */
static void HomePageUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)
{
    int framed = old->frameType != v->frameType;

    if (framed || old->mapGroup != v->mapGroup || old->mapVersion != v->mapVersion || strcmp(old->bar[0], v->bar[0])
        || strcmp(old->bar[1], v->bar[1])) {
        HomeMapPanel(v);
        Mark(dirty, &kHomeMapPanel);
    } else if (old->playerX != v->playerX || old->playerY != v->playerY || old->playerFemale != v->playerFemale
               || old->cursorX != v->cursorX || old->cursorY != v->cursorY || old->cursorFrame != v->cursorFrame) {
        CtrPokeTouchRect map = {(int16_t)(kHomeMapPanel.x + 8), (int16_t)(kHomeMapPanel.y + 8), HOME_MAP_W, HOME_MAP_H};

        HomeMap(v);
        Mark(dirty, &map);
    }
    if (framed) {
        HomeMenu(v);
        Mark(dirty, &kHomeMenu);
    } else {
        for (int i = PT_POKEDEX; i <= PT_OPTIONS; ++i)
            if (Changed(i, old, v)) {
                HomeRow(i, v);
                Mark(dirty, HomeRect(i));
            }
    }
    if (framed || HomeSettingsChanged(old, v)) {
        HomeSettings(v);
        Mark(dirty, &kHomeSettings);
    }
    if (framed || HomePartyChanged(old, v)) {
        HomeParty(v);
        Mark(dirty, &kHomeParty);
    }
}
