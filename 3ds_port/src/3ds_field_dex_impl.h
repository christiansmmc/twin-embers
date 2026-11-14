/* The Pokedex as a page (3ds_poketouch.h CtrPokeTouchDex; phase 2, drawn in the BAG's style), included at the end of
 * 3ds_poketouch.c after the field Bag. A remote for FireRed's own Pokedex: the top menu's three sections as the BAG's
 * tabs, the section's rows in a list window (six a page, the scroll cells beside it) with the row's own icon and counts
 * in a window on the right, as FireRed's top menu shows them; an ordered list as rows beside the Pokemon's picture and
 * types; a habitat page as its Pokemon two by two, its arrows beside the band; a Pokemon's page with its picture, data
 * and entry, AREA and CRY. The band says what the selection is; BACK is the Pokedex's B. Every cursor is the Pokedex's
 * own (3ds_bottom.c walks them with its keys); every picture and word is FireRed's (CtrPokeTouch_Dex*, the bridge's
 * text), but for the port's few labels (3ds_ui_strings.h). Its picture helpers are shared with the Trainer Card, the
 * forget-a-move page, the naming keyboard and the PC box. */

static const CtrUiString sFDTabLabel[3] = {UIS_DEX_LISTS, UIS_DEX_HABITATS, UIS_DEX_SEARCH};

/* ---- Pictures (decoded once, kept by species or row id) ---- */
#define FD_PICS 6
static struct {
    uint16_t species;
    uint8_t ok;
    uint8_t px[64 * 64];
    uint16_t pal[16];
    uint8_t hasBig;                 /* HeartGold's picture whole (CtrPokeTouch_DexMonPic80) */
    uint8_t big[80 * 80];
} sFDPic[FD_PICS];
static int sFDPicNext;
static struct {
    uint8_t ready, ok;
    uint8_t px[64 * 48];
    uint16_t pal[16];
} sFDIcon[24];                          /* by id + 3: the header, CANCEL, the habitats and the lists */
static struct {
    uint8_t ready, ok;
    uint8_t px[64];
    uint16_t pal[16];
} sFDCaught;

static void FDTiles(const unsigned char *tiles, int w, int h, uint8_t *px)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            unsigned byte = tiles[((y / 8) * (w / 8) + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2];
            px[y * w + x] = (uint8_t)((byte >> ((x & 1) * 4)) & 15);
        }
}

static void FDPal(const unsigned short *in, uint16_t *out)
{
    for (int i = 0; i < 16; ++i) {
        unsigned c = in[i];
        out[i] = GbaColor(c);
    }
}

static int FDPicSlot(unsigned species)
{
    static unsigned char tiles[0x800];
    static unsigned short pal[16];
    int slot;

    if (species == 0)
        return -1;
    for (int i = 0; i < FD_PICS; ++i)
        if (sFDPic[i].species == species)
            return sFDPic[i].ok ? i : -1;
    slot = sFDPicNext;
    sFDPicNext = (sFDPicNext + 1) % FD_PICS;
    sFDPic[slot].species = (uint16_t)species;
    sFDPic[slot].ok = (uint8_t)(CtrPokeTouch_DexMonPic(species, tiles, pal) != 0);
    if (!sFDPic[slot].ok)
        return -1;
    FDTiles(tiles, 64, 64, sFDPic[slot].px);
    FDPal(pal, sFDPic[slot].pal);
    {
        static unsigned char linear[80 * 80 / 2];
        unsigned short big[16];

        sFDPic[slot].hasBig = (uint8_t)(CtrPokeTouch_DexMonPic80(species, linear, big) != 0);
        for (int i = 0; sFDPic[slot].hasBig && i < 80 * 80; ++i)
            sFDPic[slot].big[i] = (uint8_t)((linear[i / 2] >> ((i & 1) * 4)) & 15);
    }
    return slot;
}

/* A species' front picture, its top-left corner at (x, y), its lowest `cut` rows left out. */
static void FDPicDraw(unsigned species, int x, int y, int cut)
{
    int slot = FDPicSlot(species);

    if (slot >= 0)
        UiImage(sFDPic[slot].px, sFDPic[slot].pal, 64, 64, 64 - cut, x, y, 1, 1);
}

/* The same, whole when HeartGold's 80x80 picture is there: on the 64x64 box's centre, 8 pixels past each side. */
static void FDPicDrawBig(unsigned species, int x, int y)
{
    int slot = FDPicSlot(species);

    if (slot >= 0 && sFDPic[slot].hasBig)
        UiImage(sFDPic[slot].big, sFDPic[slot].pal, 80, 80, 80, x - 8, y - 8, 1, 1);
    else if (slot >= 0)
        UiImage(sFDPic[slot].px, sFDPic[slot].pal, 64, 64, 64, x, y, 1, 1);
}

static void FDIconDraw(int id, int x, int y, int on)
{
    static unsigned char tiles[64 * 48 / 2];
    static unsigned short pal[16];
    int k = id + 3;

    if (k < 0 || k >= (int)(sizeof(sFDIcon) / sizeof(sFDIcon[0])))
        return;
    if (!sFDIcon[k].ready) {
        sFDIcon[k].ready = 1;
        sFDIcon[k].ok = (uint8_t)(CtrPokeTouch_DexMenuIcon(id, tiles, pal) != 0);
        if (sFDIcon[k].ok) {
            FDTiles(tiles, 64, 48, sFDIcon[k].px);
            FDPal(pal, sFDIcon[k].pal);
        }
    }
    if (sFDIcon[k].ok)
        UiImage(sFDIcon[k].px, sFDIcon[k].pal, 64, 64, 48, x, y, 1, on);
}

static int FDCaughtLoad(void)
{
    if (!sFDCaught.ready) {
        unsigned char tiles[32];
        unsigned short pal[16];

        sFDCaught.ready = 1;
        sFDCaught.ok = (uint8_t)(CtrPokeTouch_DexCaughtMarker(tiles, pal) != 0);
        if (sFDCaught.ok) {
            FDTiles(tiles, 8, 8, sFDCaught.px);
            FDPal(pal, sFDCaught.pal);
        }
    }
    return sFDCaught.ok;
}

static void FDCaughtDraw(int x, int y)
{
    if (FDCaughtLoad())
        UiImage(sFDCaught.px, sFDCaught.pal, 8, 8, 8, x, y, 1, 1);
}

static void FDFootprint(unsigned species, int x, int y)
{
    unsigned char bits[32];

    if (!CtrPokeTouch_DexFootprint(species, bits))
        return;
    for (int t = 0; t < 4; ++t)
        for (int row = 0; row < 8; ++row)
            for (int col = 0; col < 8; ++col)
                if (bits[t * 8 + row] & (1 << col))
                    Px(x + (t & 1) * 8 + col, y + (t >> 1) * 8 + row, UI_RGB(64, 72, 104));
}

/* ---- Text ---- */
/* `s` without `tail` at its end (FireRed's " POKeMON", " MODE"), into out. */
static void FDStrip(const char *s, const char *tail, char *out, int size)
{
    int n = (int)strlen(s), t = (int)strlen(tail);

    if (n > t && !strcmp(s + n - t, tail))
        n -= t;
    if (n >= size)
        n = size - 1;
    memcpy(out, s, (size_t)n);
    out[n] = 0;
}

/* A top menu row's label on its card: KANTO / NATIONAL for the lists, FireRed's own words otherwise. */
static void FDCardLabel(const CtrPokeTouchDex *d, int i, char *out, int size)
{
    int id = d->menu[i].id;

    if (id == PT_DEX_ID_KANTO || id == PT_DEX_ID_NATIONAL)
        snprintf(out, (size_t)size, "%s", CtrUiText(id == PT_DEX_ID_KANTO ? UIS_DEX_KANTO : UIS_DEX_NATIONAL));
    else if (id >= 0 && id < 9)
        FDStrip(d->menu[i].name, " POK\xC3\xA9MON", out, size);
    else
        FDStrip(d->menu[i].name, " MODE", out, size);
}

static void FDNumber(unsigned n, char *out, int size)
{
    snprintf(out, (size_t)size, "No.%03u", n > 999 ? 999 : n);
}

/* FireRed's own conversions: feet and inches, pounds to the tenth. */
static void FDHeight(const CtrPokeTouchDex *d, char *out, int size)
{
    unsigned inches, feet;

    if (!(d->sel.flags & 2)) {
        snprintf(out, (size_t)size, "??" "'??\"");
        return;
    }
    inches = 10000u * d->height / 254u;
    if (inches % 10 >= 5)
        inches += 10;
    feet = inches / 120;
    inches = (inches - feet * 120) / 10;
    snprintf(out, (size_t)size, "%u'%02u\"", feet, inches);
}

static void FDWeight(const CtrPokeTouchDex *d, char *out, int size)
{
    unsigned lbs;

    if (!(d->sel.flags & 2)) {
        snprintf(out, (size_t)size, "????.? %s", CtrUiText(UIS_DEX_LBS));
        return;
    }
    lbs = (unsigned)d->weight * 100000u / 4536u;
    if (lbs % 10 >= 5)
        lbs += 10;
    snprintf(out, (size_t)size, "%u.%u %s", lbs / 100, lbs % 100 / 10, CtrUiText(UIS_DEX_LBS));
}

/* ---- The top menu: the rows a tab shows, the tab and the page the cursor is on ---- */
/* The rows tab `tab` shows, as top menu indexes: its section's (FireRed's CLOSE left out), LISTS with CLOSE after its
 * own. */
static int FDTabRows(const CtrPokeTouchDex *d, int tab, int rows[PT_DEX_MENU_MAX])
{
    int section = -1, n = 0, cancel = -1;

    for (int i = 0; i < d->menuCount && i < PT_DEX_MENU_MAX; ++i) {
        if (d->menu[i].id == PT_DEX_ID_HEADER)
            ++section;
        else if (d->menu[i].id == PT_DEX_ID_CANCEL)
            cancel = i;
        else if (section == tab)
            rows[n++] = i;
    }
    if (tab == 0 && cancel >= 0)
        rows[n++] = cancel;
    return n;
}

/* The tab shown: the cursor's section (CLOSE is drawn in LISTS). */
static int FDTabShown(const CtrPokeTouchDex *d)
{
    int c = d->menuCursor, section = -1;

    if (c < 0 || c >= d->menuCount || d->menu[c].id == PT_DEX_ID_CANCEL)
        return 0;
    for (int i = 0; i <= c; ++i)
        section += d->menu[i].id == PT_DEX_ID_HEADER;
    return section < 0 || section > 2 ? 0 : section;
}

/* The tab shown, its rows, and the page of them (DEX_MENU_ROWS a page) that holds the cursor, the first otherwise. */
typedef struct {
    int tab, n, page, pages;
    int rows[PT_DEX_MENU_MAX];
} FDMenuPage;

static void FDMenuPageOf(const CtrPokeTouchDex *d, FDMenuPage *m)
{
    int at = 0;

    m->tab = FDTabShown(d);
    m->n = FDTabRows(d, m->tab, m->rows);
    for (int i = 0; i < m->n; ++i)
        if (m->rows[i] == d->menuCursor)
            at = i;
    m->pages = (m->n + DEX_MENU_ROWS - 1) / DEX_MENU_ROWS;
    m->page = at / DEX_MENU_ROWS;
}

/* The menu index of row i of the page shown, or -1. */
static int FDMenuRowIndex(const FDMenuPage *m, int i)
{
    int k = m->page * DEX_MENU_ROWS + i;

    return i >= 0 && i < DEX_MENU_ROWS && k < m->n ? m->rows[k] : -1;
}

int CtrPokeTouch_DexTabRow(const CtrPokeTouchView *v, int tab)
{
    int rows[PT_DEX_MENU_MAX];

    return FDTabRows(&v->dex, tab, rows) > 0 ? rows[0] : -1;
}

int CtrPokeTouch_DexGridPageRow(const CtrPokeTouchView *v, int dir)
{
    FDMenuPage m;
    int page;

    FDMenuPageOf(&v->dex, &m);
    page = m.page + dir;
    return page >= 0 && page < m.pages ? m.rows[page * DEX_MENU_ROWS] : -1;
}

int CtrPokeTouch_DexHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchDex *d = &v->dex;

    if (!d->on || v->viewport)
        return -1;
    switch (d->state) {
    case PT_DEX_STATE_MENU: {
        FDMenuPage m;

        FDMenuPageOf(d, &m);
        for (int i = 0; i < 3; ++i)
            if (UiIn(&kDexTab[i], x, y))
                return PT_DEX_HIT_TAB + i;
        if (UiIn(&kDexUp, x, y))
            return m.page > 0 ? PT_DEX_HIT_UP : -1;
        if (UiIn(&kDexDown, x, y))
            return m.page < m.pages - 1 ? PT_DEX_HIT_DOWN : -1;
        for (int i = 0; i < DEX_MENU_ROWS; ++i) {
            CtrPokeTouchRect r = DexMenuRow(i);
            int row = FDMenuRowIndex(&m, i);

            if (row >= 0 && UiIn(&r, x, y))
                return PT_DEX_HIT_ITEM + row;
        }
        return -1;
    }
    case PT_DEX_STATE_LIST:
        if (UiIn(&kDexUp, x, y))
            return d->listFirst > 0 ? PT_DEX_HIT_UP : -1;
        if (UiIn(&kDexDown, x, y))
            return d->listFirst + PT_DEX_ROWS < d->listCount ? PT_DEX_HIT_DOWN : -1;
        for (int i = 0; i < PT_DEX_ROWS; ++i) {
            CtrPokeTouchRect r = DexListRow(i);

            if (d->listFirst + i < d->listCount && UiIn(&r, x, y))
                return PT_DEX_HIT_ROW + i;
        }
        return -1;
    case PT_DEX_STATE_CATEGORY:
        if (UiIn(&kDexPrev, x, y))
            return d->canPrev ? PT_DEX_HIT_PREV : -1;
        if (UiIn(&kDexNext, x, y))
            return d->canNext ? PT_DEX_HIT_NEXT : -1;
        for (int i = 0; i < d->slots && i < 4; ++i) {
            CtrPokeTouchRect r = DexSlot(i);

            if (UiIn(&r, x, y))
                return PT_DEX_HIT_SLOT + i;
        }
        return -1;
    case PT_DEX_STATE_PAGE:
        if (d->added)
            return PT_DEX_HIT_CONTINUE;     /* the page a catch added: anywhere goes on with the battle (its A) */
        if (UiIn(&kDexArea, x, y))
            return PT_DEX_HIT_AREA;
        if (UiIn(&kDexCry, x, y))
            return PT_DEX_HIT_CRY;
        return -1;
    }
    return -1;
}

/* BACK's cell: on every page but the one a catch adds (a message that waits for A). */
static const CtrPokeTouchRect *FDexBackRect(const CtrPokeTouchView *v)
{
    return v->dex.state == PT_DEX_STATE_PAGE && v->dex.added ? NULL : &kDexBack;
}

/* ---- Drawing ---- */
/* The tabs: the arrow on the one held, else the section shown. */
static void FDTabs(const CtrPokeTouchView *v, int shown)
{
    const CtrPokeTouchDex *d = &v->dex;
    int held = d->pressed >= PT_DEX_HIT_TAB && d->pressed < PT_DEX_HIT_TAB + 3 ? d->pressed - PT_DEX_HIT_TAB : -1;

    for (int i = 0; i < 3; ++i)
        BagCell(&kDexTab[i], v->frameType, CtrUiText(sFDTabLabel[i]), held >= 0 ? held == i : shown == i, held == i, 1);
}

/* The top menu row the side window and the band describe: the one held, else the cursor's. */
static int FDMenuChosen(const CtrPokeTouchDex *d)
{
    int held = d->pressed >= PT_DEX_HIT_ITEM && d->pressed < PT_DEX_HIT_ITEM + PT_DEX_MENU_MAX
             ? d->pressed - PT_DEX_HIT_ITEM : -1;

    return held >= 0 && held < d->menuCount ? held : d->menuCursor;
}

/* The top menu: the tabs, the section's rows (the arrow on the chosen one, a habitat with nothing seen toned), the
 * scroll cells, the row's FireRed icon (and a list's counts) on the right, the band. */
static void FDMenu(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    int chosen = FDMenuChosen(d);
    FDMenuPage m;
    char title[28] = "", body[96] = "";

    FDMenuPageOf(d, &m);
    FDTabs(v, m.tab);
    FPageWindow(&kDexList, v->frameType);
    for (int i = 0; i < DEX_MENU_ROWS; ++i) {
        CtrPokeTouchRect r = DexMenuRow(i);
        int row = FDMenuRowIndex(&m, i);
        char label[28];

        if (row < 0)
            break;
        FDCardLabel(d, row, label, sizeof(label));
        FPageRow(&r, label, row == chosen, d->pressed == PT_DEX_HIT_ITEM + row, !d->menu[row].locked);
    }
    BagArrowCell(&kDexUp, v->frameType, 0, m.page > 0, d->pressed == PT_DEX_HIT_UP);
    BagArrowCell(&kDexDown, v->frameType, 1, m.page < m.pages - 1, d->pressed == PT_DEX_HIT_DOWN);
    FPageWindow(&kDexSide, v->frameType);
    if (chosen >= 0 && chosen < d->menuCount) {
        int id = d->menu[chosen].id, x = kDexSide.x + 8, right = kDexSide.x + kDexSide.w - 8;

        FDIconDraw(id, kDexSide.x + (kDexSide.w - 64) / 2, kDexSide.y + 12, !d->menu[chosen].locked);
        if (id == PT_DEX_ID_KANTO || id == PT_DEX_ID_NATIONAL) {
            int nat = id == PT_DEX_ID_NATIONAL;
            char n[8];

            snprintf(n, sizeof(n), "%u", d->seen[nat]);
            SLine(x, right, kDexSide.y + 72, CtrUiText(UIS_DEX_SEEN), n);
            snprintf(n, sizeof(n), "%u", d->owned[nat]);
            SLine(x, right, kDexSide.y + 72 + KIT_LINE_H + 2, CtrUiText(UIS_DEX_OWNED), n);
        }
        snprintf(title, sizeof(title), "%s", d->menu[chosen].name);
        snprintf(body, sizeof(body), "%s", CtrUiText(id == PT_DEX_ID_CANCEL ? UIS_DEX_NOTE_CLOSE
                                                   : id == PT_DEX_ID_KANTO || id == PT_DEX_ID_NATIONAL ? UIS_DEX_NOTE_LIST
                                                   : id >= 0 && id < 9 ? (d->menu[chosen].locked ? UIS_DEX_NOTE_LOCKED
                                                                                               : UIS_DEX_NOTE_HABITAT)
                                                   : UIS_DEX_NOTE_SEARCH));
    }
    FPageBand(&kDexNote, title, body, 0);
}

/* The selected Pokemon on the list's right: its picture (HeartGold's 80x80 whole when the pack has it) and its types. */
static void FDMonSide(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    const CtrPokeTouchRect *s = &kDexSide;
    int x = s->x + 8, w = s->w - 16;

    FPageWindow(s, v->frameType);
    if (d->sel.flags & 1)
        FDPicDrawBig(d->sel.species, x + (w - 64) / 2, s->y + 16);
    if (d->type[0] >= 0) {
        CtrPokeTouchRect t = {(int16_t)x, (int16_t)(s->y + s->h - 8 - (d->type[1] >= 0 ? 30 : 13)), (int16_t)w, 13};

        KitTypeLabel(&t, d->type[0], d->typeName[0]);
        if (d->type[1] >= 0) {
            t.y = (int16_t)(t.y + 17);
            KitTypeLabel(&t, d->type[1], d->typeName[1]);
        }
    }
}

/* The selected Pokemon's line for the band: its number and name ("-----" while not seen). */
static void FDMonTitle(const CtrPokeTouchDex *d, char *out, int size)
{
    char no[12];

    FDNumber(d->sel.number, no, sizeof(no));
    snprintf(out, (size_t)size, "%s %s", no, d->sel.flags & 1 ? d->sel.name : "-----");
}

/* An ordered list: its order and counts in the title, five rows (the caught ball, the number, the name), the scroll
 * cells, the Pokemon on the right, the band. */
static void FDList(const CtrPokeTouchView *v)
{
    static const CtrUiString kOrder[6] = {UIS_DEX_KANTO, UIS_DEX_ATOZ, UIS_DEX_TYPE, UIS_DEX_LIGHTEST, UIS_DEX_SMALLEST,
                                          UIS_DEX_NATIONAL};
    const CtrPokeTouchDex *d = &v->dex;
    const CtrPokeTouchRect *t = &kDexTitle;
    int held = d->pressed >= PT_DEX_HIT_ROW && d->pressed < PT_DEX_HIT_ROW + PT_DEX_ROWS ? d->pressed - PT_DEX_HIT_ROW : -1;
    int right = t->x + t->w - 8;
    char title[40];

    FPageWindow(t, v->frameType);
    if (d->order == 0 || d->order == 5) {
        int nat = d->order == 5;
        char counts[48];

        snprintf(counts, sizeof(counts), "%s %u  %s %u", CtrUiText(UIS_DEX_SEEN), d->seen[nat], CtrUiText(UIS_DEX_OWNED),
                 d->owned[nat]);
        KitText(counts, right - UiTextWidth(counts, 1), t->y + 8, right);
        right -= UiTextWidth(counts, 1) + 8;
    }
    KitText(CtrUiText(kOrder[d->order < 6 ? d->order : 0]), t->x + 8, t->y + 8, right);
    FPageWindow(&kDexList, v->frameType);
    for (int i = 0; i < PT_DEX_ROWS; ++i) {
        CtrPokeTouchRect r = DexListRow(i);
        const CtrPokeTouchDexMon *m = &d->row[i];
        int index = d->listFirst + i, top = r.y + (r.h - KIT_LINE_H) / 2, nx = r.x + KIT_ARROW_W + 10;
        char no[8];

        if (index >= d->listCount)
            break;
        Fill(r.x, r.y, r.w, r.h, held == i ? KitInk(3) : KitInk(1));
        if (held >= 0 ? held == i : index == d->listCursor)
            KitText(KIT_ARROW, r.x, top, r.x + KIT_ARROW_W);
        if (m->flags & 2)
            FDCaughtDraw(r.x + KIT_ARROW_W, r.y + (r.h - 8) / 2);
        snprintf(no, sizeof(no), "%03u", m->number > 999 ? 999 : m->number);
        KitText(no, nx, top, r.x + r.w);
        KitText(m->flags & 1 ? m->name : "-----", nx + UiTextWidth("000", 1) + 6, top, r.x + r.w);
    }
    BagArrowCell(&kDexUp, v->frameType, 0, d->listFirst > 0, d->pressed == PT_DEX_HIT_UP);
    BagArrowCell(&kDexDown, v->frameType, 1, d->listFirst + PT_DEX_ROWS < d->listCount, d->pressed == PT_DEX_HIT_DOWN);
    FDMonSide(v);
    FDMonTitle(d, title, sizeof(title));
    FPageBand(&kDexNote, title, CtrUiText(d->sel.flags & 1 ? UIS_DEX_NOTE_OPEN : UIS_DEX_NOTE_UNSEEN), 0);
}

/* A habitat page: its Pokemon two by two (the picture, the number with the caught ball, the name), the arrows that
 * turn the pages beside the band (the selected one and the page). */
static void FDCategory(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    int held = d->pressed >= PT_DEX_HIT_SLOT && d->pressed < PT_DEX_HIT_SLOT + 4 ? d->pressed - PT_DEX_HIT_SLOT : -1;
    char title[40], page[24];

    FPageWindow(&kDexHabitat, v->frameType);
    for (int i = 0; i < d->slots && i < 4; ++i) {
        CtrPokeTouchRect r = DexSlot(i);
        const CtrPokeTouchDexMon *m = &d->mon[i];
        int tx = r.x + 64 + 16, top = r.y + 18;
        char no[12];

        Fill(r.x, r.y, r.w, r.h, held == i ? KitInk(3) : KitInk(1));
        if (m->flags & 1)
            FDPicDraw(m->species, r.x + 4, r.y + (r.h - 64) / 2, 0);
        if (held >= 0 ? held == i : d->slot == i)
            KitText(KIT_ARROW, tx - KIT_ARROW_W, top, tx);
        FDNumber(m->number, no, sizeof(no));
        KitText(no, tx, top, r.x + r.w);
        if (m->flags & 2)
            FDCaughtDraw(tx + UiTextWidth(no, 1) + 4, top + 4);
        KitText(m->flags & 1 ? m->name : "-----", tx, top + KIT_LINE_H + 4, r.x + r.w);
    }
    BagArrowCell(&kDexPrev, v->frameType, 2, d->canPrev, d->pressed == PT_DEX_HIT_PREV);
    BagArrowCell(&kDexNext, v->frameType, 3, d->canNext, d->pressed == PT_DEX_HIT_NEXT);
    FDMonTitle(d, title, sizeof(title));
    snprintf(page, sizeof(page), "%s %u/%u", CtrUiText(UIS_DEX_PAGE), d->page, d->pages);
    FPageBand(&kDexNoteNarrow, title, page, 0);
}

/* A Pokemon's page: its picture; its number and name, category, height and weight (FireRed's conversions), types and
 * footprint; its entry; AREA and CRY (or, for the page a catch adds, the band that waits for A). */
static void FDPage(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    const CtrPokeTouchRect *p = &kDexPic, *in = &kDexInfo;
    int x = in->x + 8, right = in->x + in->w - 8, top = in->y + 8;
    char text[40];

    FPageWindow(p, v->frameType);
    FDPicDrawBig(d->sel.species, p->x + (p->w - 64) / 2, p->y + (p->h - 64) / 2);
    FPageWindow(in, v->frameType);
    FDMonTitle(d, text, sizeof(text));
    KitText(text, x, top, right);
    KitText(d->sel.flags & 2 ? d->kind : "?????", x, top + KIT_LINE_H, right);
    FDHeight(d, text, sizeof(text));
    SLine(x, right, top + 34, CtrUiText(UIS_DEX_HT), text);
    FDWeight(d, text, sizeof(text));
    SLine(x, right, top + 34 + KIT_LINE_H, CtrUiText(UIS_DEX_WT), text);
    if (d->type[0] >= 0) {
        CtrPokeTouchRect t = {(int16_t)x, (int16_t)(top + 70), 44, 13};

        KitTypeLabel(&t, d->type[0], d->typeName[0]);
        if (d->type[1] >= 0) {
            t.x = (int16_t)(x + 48);
            KitTypeLabel(&t, d->type[1], d->typeName[1]);
        }
    }
    FDFootprint(d->sel.species, right - 16, top + 68);
    FPageBand(&kDexEntry, NULL, d->desc, 0);
    if (d->added) {
        FPageBand(&kDexNoteWide, d->sel.name, CtrUiText(UIS_DEX_NOTE_ADDED), 1);
        return;
    }
    BagCell(&kDexArea, v->frameType, CtrUiText(UIS_DEX_AREA), d->pressed == PT_DEX_HIT_AREA, d->pressed == PT_DEX_HIT_AREA, 1);
    BagCell(&kDexCry, v->frameType, CtrUiText(UIS_DEX_CRY), d->pressed == PT_DEX_HIT_CRY, d->pressed == PT_DEX_HIT_CRY, 1);
}

/* The page the Pokedex shows (while it is busy with nothing shown yet: the band alone), and BACK. */
static void FDexPageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *back = FDexBackRect(v);

    Fill(0, 0, W, H, BPageBg());
    switch (v->dex.state) {
    case PT_DEX_STATE_MENU:
        FDMenu(v);
        break;
    case PT_DEX_STATE_LIST:
        FDList(v);
        break;
    case PT_DEX_STATE_CATEGORY:
        FDCategory(v);
        break;
    case PT_DEX_STATE_PAGE:
        FDPage(v);
        break;
    default:
        FPageBand(&kDexNote, NULL, NULL, 0);
        break;
    }
    if (back && v->backVisible)
        FPageBack(v, back);
}
