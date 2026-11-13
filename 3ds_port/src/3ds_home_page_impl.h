/* HOME as a full-screen page (phase 1 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md): FireRed's
 * Region Map at 1:1 in a panel with the standard frame and a caption strip (the place, the region), the message panel
 * with the dialogue frame, the menu entries with FireRed's item icons, X, Y and the running mode. Where everything is
 * comes from 3ds_lower_layout.h. Included by 3ds_poketouch.c after Changed and Mark; HOME is this page whenever it is
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

static const CtrPokeTouchRect *HomeRect(int id)
{
    if (id >= PT_POKEDEX && id <= PT_OPTIONS)
        return &kHomeEntry[id - PT_POKEDEX];
    return id == PT_REGISTERED ? &kHomeRegistered : id == PT_BIKE ? &kHomeBike : id == PT_RUN ? &kHomeRun : NULL;
}

int CtrPokeTouch_HomeHit(const CtrPokeTouchView *v, int x, int y)
{
    (void)v;
    for (int i = 0; i < PT_COUNT; ++i) {
        const CtrPokeTouchRect *r = HomeRect(i);

        if (r && UiIn(r, x, y))
            return i;
    }
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

/* The message panel: bar[2] in the dialogue frame, wrapped. */
static void HomeMessage(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = &kHomeMessage;

    Fill(r->x, r->y, r->w, r->h, HomeBg());
    KitPanel(r, KIT_FRAME_DIALOGUE, 0);
    KitWrap(v->bar[2], r->x + 16, r->y + 8, r->w - 32, (r->h - 16) / KIT_LINE_H);
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

/* The running mode: the port's own shoe (FireRed has no icon for it) and whether it is on. */
static void HomeRun(const CtrPokeTouchRect *r, const CtrPokeTouchView *v, int pressed, int on)
{
    int running = v->enabled[PT_RUN] && v->running, sy = r->y + (r->h - FLATSHOESMALL_H) / 2;

    Fill(r->x, r->y, r->w, r->h, pressed ? KitInk(3) : KitInk(1));
    for (int py = 0; py < FLATSHOESMALL_H; ++py)
        for (int px = 0; px < FLATSHOESMALL_W; ++px)
            if (kFlatShoeSmall[py * FLATSHOESMALL_W + px])
                Px(r->x + 4 + px, sy + py, Tone(kFlatShoeSmall[py * FLATSHOESMALL_W + px], on));
    KitTextOn(running ? CTR_TR("ON", "S\xc3\x8d", "AN") : CTR_TR("OFF", "NO", "AUS"), r->x + 8 + FLATSHOESMALL_W,
              r->y + (r->h - KIT_LINE_H) / 2, r->x + r->w, on);
}

/* An element of the page, as the old column's Button decides its state: usable or the screen shown, pressed under a
 * held touch, selected when it is the screen shown. */
static void HomeButton(int id, const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *r = HomeRect(id);
    int on = v->enabled[id] || v->selected == id;
    int pressed = v->enabled[id] && v->pressed == id;
    int selected = v->selected == id && id != PT_RUN;

    if (id == PT_REGISTERED || id == PT_BIKE) {
        int slot = id == PT_REGISTERED ? ICON_REGISTERED : ICON_BIKE;
        int ok = ItemIcon(slot, id == PT_REGISTERED ? v->registeredItem : v->bikeItem);

        KitIconCell(r, ok ? sIcons[slot].pixels : NULL, sIcons[slot].pal, 24, 24, id == PT_REGISTERED ? "X" : "Y",
                    selected, pressed, on);
    } else if (id == PT_RUN) {
        HomeRun(r, v, pressed, on);
    } else {
        char name[16];
        const uint16_t *pal;
        const uint8_t *pixels = HomeEntryIcon(id - PT_POKEDEX, &pal);

        KitIconCell(r, pixels, pal, 24, 24, ColumnLabel(id, v, name), selected, pressed, on);
    }
}

static void HomePageDraw(const CtrPokeTouchView *v)
{
    Fill(0, 0, W, H, HomeBg());
    HomeMapPanel(v);
    HomeMessage(v);
    for (int i = 0; i < PT_COUNT; ++i)
        HomeButton(i, v);
}

/* Only what changed: the map panel for the frame or the caption, the map alone for its icons, the message, and the
 * elements whose state changed. Each repaints its whole rectangle, so the result equals HomePageDraw. */
static void HomePageUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)
{
    if (old->frameType != v->frameType || old->mapGroup != v->mapGroup || old->mapVersion != v->mapVersion
        || strcmp(old->bar[0], v->bar[0]) || strcmp(old->bar[1], v->bar[1])) {
        HomeMapPanel(v);
        Mark(dirty, &kHomeMapPanel);
    } else if (old->playerX != v->playerX || old->playerY != v->playerY || old->playerFemale != v->playerFemale
               || old->cursorX != v->cursorX || old->cursorY != v->cursorY || old->cursorFrame != v->cursorFrame) {
        CtrPokeTouchRect map = {(int16_t)(kHomeMapPanel.x + 8), (int16_t)(kHomeMapPanel.y + 8), HOME_MAP_W, HOME_MAP_H};

        HomeMap(v);
        Mark(dirty, &map);
    }
    if (strcmp(old->bar[2], v->bar[2])) {
        HomeMessage(v);
        Mark(dirty, &kHomeMessage);
    }
    for (int i = 0; i < PT_COUNT; ++i)
        if (Changed(i, old, v)) {
            HomeButton(i, v);
            Mark(dirty, HomeRect(i));
        }
}
