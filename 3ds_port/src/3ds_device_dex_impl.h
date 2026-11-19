/* The Pokedex as the red device (the top menu: board def_dex_menu, plan 04b Task 6; the list: def_dex_list, Task 7;
 * the rest plan 3's), included by 3ds_poketouch.c after the letter: a row of the field dispatcher
 * (3ds_device_field_impl.h), shown while view->dex.on and there is no GBA picture (DevDexShown; its AREA page stays
 * FireRed's picture). A remote for FireRed's own Pokedex (3ds_poketouch.h CtrPokeTouchDex): every cursor is the
 * Pokedex's own (3ds_bottom.c walks them with its keys); every picture and word is FireRed's but for the port's few.
 *
 * The top menu (MENU): its three sections as the grid's tabs (DevGridTab(3, i)), the one the cursor is in sunk (CLOSE
 * belongs to the lists: v0.6.0's rule) or the one under a held touch; that section's rows as keys, six to a page
 * (v0.6.0's paging), each FireRed's 64x48 menu icon over its name in the small font (capitals faceY + 56: Call 16), the
 * cursor's sunk, a habitat with nothing seen yet beige, its icon half over the face; SEEN and OWNED on the small note;
 * ▲ ▼ beside it always, grey (and not hit) where there is no page to turn to, so both on a tab of one page; CLOSE,
 * FireRed's CLOSE POKeDEX row, the dark key in the exit corner, sunk while the cursor is on it.
 * An ordered list (LIST): its order on the title (SEEN and OWNED at its right for the numbered ones), the grid's five
 * rows (24 tall, 26 apart: the number, the name, a caught one's mark), the cursor's inverted, ▲ ▼ grey (and not hit)
 * where the list goes no further; the selected one's types and kind on the grid's note; BACK. A habitat's page
 * (CATEGORY, no board: the menu's grid): the habitat and the page on the title, its Pokemon in up to four slots on the
 * menu's rows (the picture of one seen, the number, the name), ◀ ▶ grey where there is no page; the selected one on
 * the small note; BACK in the exit corner. A Pokemon's page (PAGE): its picture, number, name, kind, types (a caught
 * one's), height, weight, footprint and entry on the tall screen; AREA, CRY and BACK; the page a catch has just added:
 * OK alone (the battle's A), the screen taking a touch too. While the Pokedex is busy the last picture is kept
 * (FieldDexFill); nothing kept yet, the shell. The chosen key, row or slot is the one under a held touch, else the
 * game's cursor (DevChosen). Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes; the
 * codes are v0.6.0's (PT_DEX_HIT_*).
 *
 * Words (decision 5): FireRed's menu words, names, kinds and types (which v0.6.0 title-cased) through DevUpper, a menu
 * word without its " POKeMON" or " MODE"; the port's words in capitals; FireRed's entry as it is.
 *
 * Redraw: the screen whole when the Pokedex as drawn differs (DevDexSame), and nothing else.
 *
 * The species' pictures (FDPicSlot, FDPicDraw, FDPicDrawBig and their cache) live here; the move to forget, the
 * relearner, the naming screen and the PC box, included after, use them too. */

static int DevDexShown(const CtrPokeTouchView *v)
{
    return v->dex.on && !v->viewport;
}

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

/* A top menu row's icon (FireRed's 64x48) at (x, y): as it is, or (not `on`: a habitat with nothing seen yet, on its
 * beige key) each colour half over the beige face (DEV_DEX_A_LOCKED, the board's opacity 0.5); decoded once, by id + 3
 * (the header, CANCEL, the habitats and the lists). */
#define DEV_DEX_A_LOCKED 128

static struct {
    uint8_t ready, ok;
    uint8_t px[64 * 48];
    uint16_t pal[16];
} sDevDexIcon[24];

static void DevDexIcon(int id, int x, int y, int on)
{
    static unsigned char tiles[64 * 48 / 2];
    static unsigned short pal[16];
    int k = id + 3;

    if (k < 0 || k >= (int)(sizeof(sDevDexIcon) / sizeof(sDevDexIcon[0])))
        return;
    if (!sDevDexIcon[k].ready) {
        sDevDexIcon[k].ready = 1;
        sDevDexIcon[k].ok = (uint8_t)(CtrPokeTouch_DexMenuIcon(id, tiles, pal) != 0);
        if (sDevDexIcon[k].ok) {
            FDTiles(tiles, 64, 48, sDevDexIcon[k].px);
            FDPal(pal, sDevDexIcon[k].pal);
        }
    }
    if (!sDevDexIcon[k].ok)
        return;
    if (on) {
        UiImage(sDevDexIcon[k].px, sDevDexIcon[k].pal, 64, 64, 48, x, y, 1, 1);
        return;
    }
    for (int py = 0; py < 48; ++py)
        for (int px = 0; px < 64; ++px) {
            uint8_t c = sDevDexIcon[k].px[py * 64 + px];

            if (c)
                Px(x + px, y + py, DevBlend(DEV_FAINT, sDevDexIcon[k].pal[c], DEV_DEX_A_LOCKED));
        }
}

/* FireRed's caught mark (8x8), decoded once. */
static struct {
    uint8_t ready, ok;
    uint8_t px[64];
    uint16_t pal[16];
} sDevDexMark;

static void DevDexMark(int x, int y)
{
    if (!sDevDexMark.ready) {
        unsigned char tiles[32];
        unsigned short pal[16];

        sDevDexMark.ready = 1;
        sDevDexMark.ok = (uint8_t)(CtrPokeTouch_DexCaughtMarker(tiles, pal) != 0);
        if (sDevDexMark.ok) {
            FDTiles(tiles, 8, 8, sDevDexMark.px);
            FDPal(pal, sDevDexMark.pal);
        }
    }
    if (sDevDexMark.ok)
        UiImage(sDevDexMark.px, sDevDexMark.pal, 8, 8, 8, x, y, 1, 1);
}

/* A species' footprint (FireRed's 16x16, four 8x8 tiles of 1 bit) in the paper's ink, its top-left at (x, y). */
static void DevDexFootprint(unsigned species, int x, int y)
{
    unsigned char bits[32];

    if (!CtrPokeTouch_DexFootprint(species, bits))
        return;
    for (int t = 0; t < 4; ++t)
        for (int row = 0; row < 8; ++row)
            for (int col = 0; col < 8; ++col)
                if (bits[t * 8 + row] & (1 << col))
                    Px(x + (t & 1) * 8 + col, y + (t >> 1) * 8 + row, DEV_INK);
}

/* ---- The top menu: its sections (the rows after each header), the one the cursor is in, its pages ------------------ */

#define DEV_DEX_PAGE 6                          /* the menu's keys a page */
#define DEV_DEX_WORD 32                         /* a word as drawn */

/* The section row `index` is in (0 the lists, 1 the habitats, 2 the searches, 3 CLOSE's): the headers over it. */
static int DevDexSection(const CtrPokeTouchDex *d, int index)
{
    int section = -1;

    for (int i = 0; i <= index && i < d->menuCount; ++i)
        if (d->menu[i].id == PT_DEX_ID_HEADER)
            ++section;
    return section;
}

/* The rows of section k but its header and CLOSE: how many, the first in *first (-1: none). */
static int DevDexSectionRows(const CtrPokeTouchDex *d, int k, int *first)
{
    int section = -1, n = 0;

    *first = -1;
    for (int i = 0; i < d->menuCount; ++i) {
        if (d->menu[i].id == PT_DEX_ID_HEADER) {
            ++section;
            continue;
        }
        if (section != k || d->menu[i].id == PT_DEX_ID_CANCEL)
            continue;
        if (*first < 0)
            *first = i;
        ++n;
    }
    return n;
}

/* CLOSE's row (FireRed's CANCEL), or -1. */
static int DevDexCancelRow(const CtrPokeTouchDex *d)
{
    for (int i = 0; i < d->menuCount; ++i)
        if (d->menu[i].id == PT_DEX_ID_CANCEL)
            return i;
    return -1;
}

/* The section shown: the cursor's (CLOSE's belongs to the lists). */
static int DevDexTab(const CtrPokeTouchDex *d)
{
    int k = DevDexSection(d, d->menuCursor);

    return k < 0 || k > 2 ? 0 : k;
}

/* The page of section k the cursor is on (the first when it is elsewhere): its first row, its keys, the pages. */
static int DevDexGridPage(const CtrPokeTouchDex *d, int k, int *first, int *count, int *pages)
{
    int start, n = DevDexSectionRows(d, k, &start), at = d->menuCursor - start, page;

    page = at >= 0 && at < n ? at / DEV_DEX_PAGE : 0;
    *pages = (n + DEV_DEX_PAGE - 1) / DEV_DEX_PAGE;
    *first = start + page * DEV_DEX_PAGE;
    *count = n - page * DEV_DEX_PAGE > DEV_DEX_PAGE ? DEV_DEX_PAGE : n - page * DEV_DEX_PAGE;
    return page;
}

int CtrPokeTouch_DexTabRow(const CtrPokeTouchView *v, int tab)
{
    int first;

    return DevDexSectionRows(&v->dex, tab, &first) > 0 ? first : -1;
}

int CtrPokeTouch_DexGridPageRow(const CtrPokeTouchView *v, int dir)
{
    const CtrPokeTouchDex *d = &v->dex;
    int tab = DevDexTab(d), start, n, first, count, pages, page;

    if (tab == 0)
        return -1;
    n = DevDexSectionRows(d, tab, &start);
    page = DevDexGridPage(d, tab, &first, &count, &pages) + dir;
    return page >= 0 && page < pages && start + page * DEV_DEX_PAGE < start + n ? start + page * DEV_DEX_PAGE : -1;
}

/* ---- Where a touch goes ---------------------------------------------------------------------------------------------- */

int CtrPokeTouch_DexHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchDex *d = &v->dex;

    if (!DevDexShown(v))
        return -1;
    switch (d->state) {
    case PT_DEX_STATE_MENU: {
        int first, count, pages, page, cancel = DevDexCancelRow(d);

        for (int i = 0; i < 3; ++i) {
            CtrPokeTouchRect r = DevGridTab(3, i);

            if (DevOnKey(&r, x, y))
                return PT_DEX_HIT_TAB + i;
        }
        page = DevDexGridPage(d, DevDexTab(d), &first, &count, &pages);
        if (DevOnKey(&kDevSmallKey[0], x, y))           /* ▲ ▼ taken only where there is a page to turn to */
            return page > 0 ? PT_DEX_HIT_UP : -1;
        if (DevOnKey(&kDevSmallKey[1], x, y))
            return page < pages - 1 ? PT_DEX_HIT_DOWN : -1;
        for (int i = 0; i < count; ++i) {
            CtrPokeTouchRect r = DevDexItem(i);

            if (DevOnKey(&r, x, y))
                return PT_DEX_HIT_ITEM + first + i;
        }
        return cancel >= 0 && DevOnKey(&kDevGridExit, x, y) ? PT_DEX_HIT_ITEM + cancel : -1;
    }
    case PT_DEX_STATE_LIST:
        if (DevOnKey(&kDevGridUp, x, y))
            return d->listFirst > 0 ? PT_DEX_HIT_UP : -1;
        if (DevOnKey(&kDevGridDown, x, y))
            return d->listFirst + PT_DEX_ROWS < d->listCount ? PT_DEX_HIT_DOWN : -1;
        for (int i = 0; i < PT_DEX_ROWS && d->listFirst + i < d->listCount; ++i) {
            CtrPokeTouchRect r = DevGridRow(i, 0);              /* the row itself: the 2 px gaps take nothing */

            if (UiIn(&r, x, y))
                return PT_DEX_HIT_ROW + i;
        }
        return -1;
    case PT_DEX_STATE_CATEGORY:
        if (DevOnKey(&kDevSmallKey[0], x, y))
            return d->canPrev ? PT_DEX_HIT_PREV : -1;
        if (DevOnKey(&kDevSmallKey[1], x, y))
            return d->canNext ? PT_DEX_HIT_NEXT : -1;
        for (int i = 0; i < d->slots && i < 4; ++i) {
            CtrPokeTouchRect r = DevDexSlot(i);

            if (DevOnKey(&r, x, y))
                return PT_DEX_HIT_SLOT + i;
        }
        return -1;
    case PT_DEX_STATE_PAGE:
        if (d->added)                       /* the page a catch added: OK or the screen goes on with the battle (its A) */
            return UiIn(&kDevTallScreen, x, y) || DevOnKey(&kDevWideKey, x, y) ? PT_DEX_HIT_CONTINUE : -1;
        if (DevOnKey(&kDevDexArea, x, y))
            return PT_DEX_HIT_AREA;
        return DevOnKey(&kDevDexCry, x, y) ? PT_DEX_HIT_CRY : -1;
    }
    return -1;
}

/* ---- Words ----------------------------------------------------------------------------------------------------------- */

/* `s` in capitals without `tail` at its end (FireRed's " POKeMON", " MODE": cut after the capitals, so in either case),
 * into out. */
static void DevDexUpperStrip(const char *s, const char *tail, char *out, size_t size)
{
    size_t n, t = strlen(tail);

    DevUpper(out, s, size);
    n = strlen(out);
    if (n > t && !strcmp(out + n - t, tail))
        out[n - t] = 0;
}

/* A top menu row's word: KANTO / NATIONAL for the lists (v0.6.0's), FireRed's own otherwise without its " POKeMON" or
 * " MODE". */
static void DevDexItemWord(const CtrPokeTouchDex *d, int row, char *out, size_t size)
{
    int id = d->menu[row].id;

    if (id == PT_DEX_ID_KANTO || id == PT_DEX_ID_NATIONAL)
        DevUpper(out, CtrUiText(id == PT_DEX_ID_KANTO ? UIS_DEX_KANTO : UIS_DEX_NATIONAL), size);
    else
        DevDexUpperStrip(d->menu[row].name, id >= 0 && id < 9 ? " POK\xC3\xA9MON" : " MODE", out, size);
}

/* A habitat's name on its page's title: its row's in the top menu (FireRed fills the menu there too). */
static void DevDexHabitat(const CtrPokeTouchDex *d, char *out, size_t size)
{
    out[0] = 0;
    for (int i = 0; i < d->menuCount; ++i)
        if (d->menu[i].id == (int)d->category) {
            DevDexUpperStrip(d->menu[i].name, " POK\xC3\xA9MON", out, size);
            return;
        }
}

static void DevDexNumber(unsigned n, char *out, size_t size)
{
    snprintf(out, size, "No.%03u", n > 999 ? 999 : n);
}

/* A Pokemon's name in capitals once seen, else `unseen`. */
static const char *DevDexName(const CtrPokeTouchDexMon *m, const char *unseen, char *out, size_t size)
{
    if (!(m->flags & 1))
        return unseen;
    DevUpper(out, m->name, size);
    return out;
}

/* FireRed's own conversions: feet and inches, pounds to the tenth (v0.6.0's); unknown for one not caught. */
static void DevDexHeight(const CtrPokeTouchDex *d, char *out, size_t size)
{
    unsigned inches, feet;

    if (!(d->sel.flags & 2)) {
        snprintf(out, size, "??" "'??\"");
        return;
    }
    inches = 10000u * d->height / 254u;
    if (inches % 10 >= 5)
        inches += 10;
    feet = inches / 120;
    inches = (inches - feet * 120) / 10;
    snprintf(out, size, "%u'%02u\"", feet, inches);
}

static void DevDexWeight(const CtrPokeTouchDex *d, char *out, size_t size)
{
    unsigned lbs;

    if (!(d->sel.flags & 2)) {
        snprintf(out, size, "????.? %s", CtrUiText(UIS_DEX_LBS));
        return;
    }
    lbs = (unsigned)d->weight * 100000u / 4536u;
    if (lbs % 10 >= 5)
        lbs += 10;
    snprintf(out, size, "%u.%u %s", lbs / 100, lbs % 100 / 10, CtrUiText(UIS_DEX_LBS));
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

static const CtrUiString kDevDexTabWord[3] = {UIS_DEX_LISTS, UIS_DEX_HABITATS, UIS_DEX_SEARCH};
/* The ordered list's title by its order (FireRed's DEX_ORDER_*). */
static const CtrUiString kDevDexOrder[6] = {UIS_DEX_KANTO, UIS_DEX_ATOZ, UIS_DEX_TYPE, UIS_DEX_LIGHTEST, UIS_DEX_SMALLEST,
                                            UIS_DEX_NATIONAL};

/* Whether the menu lists the National Pokedex (FireRed does once the player has it): then its counts are the note's. */
static int DevDexNationalListed(const CtrPokeTouchDex *d)
{
    for (int i = 0; i < d->menuCount; ++i)
        if (d->menu[i].id == PT_DEX_ID_NATIONAL)
            return 1;
    return 0;
}

/* SEEN and OWNED on the small note, a row each 14 tall from 2 px into the paper (def_dex_menu: the words' capitals 201
 * and 215, the counts' 200 and 214, ending at 148): the word small at the left, the count at the right. */
static void DevDexCounts(const CtrPokeTouchDex *d)
{
    CtrPokeTouchRect paper = DevScreen(&kDevSmallNote);
    int nat = DevDexNationalListed(d);
    char label[DEV_DEX_WORD], value[8];

    for (int i = 0; i < 2; ++i) {
        CtrPokeTouchRect row = {(int16_t)(paper.x + 6), (int16_t)(paper.y + 2 + 14 * i), (int16_t)(paper.w - 12), 14};

        DevUpper(label, CtrUiText(i ? UIS_DEX_OWNED : UIS_DEX_SEEN), sizeof(label));
        snprintf(value, sizeof(value), "%u", (unsigned)(i ? d->owned[nat] : d->seen[nat]));
        DevLabelValue(&row, label, value);
    }
}

/* The top menu (MENU, def_dex_menu). A key: FireRed's 64x48 icon 16 px in and 3 under the face's top, its word small
 * at capitals faceY + 56 (Call 16: the board's stand-in is 44 tall, FireRed's icon 48). */
static void DevDexMenu(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    int pressed = d->pressed, tab = DevDexTab(d), cancel = DevDexCancelRow(d), first, count, pages, page;
    int pressTab = pressed >= PT_DEX_HIT_TAB && pressed < PT_DEX_HIT_TAB + 3 ? pressed - PT_DEX_HIT_TAB : -1;
    int pressRow = pressed >= PT_DEX_HIT_ITEM && pressed < PT_DEX_HIT_ITEM + PT_DEX_MENU_MAX ? pressed - PT_DEX_HIT_ITEM : -1;
    char word[DEV_DEX_WORD];

    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect r = DevGridTab(3, i);
        int sunk = DevChosen(i, tab, pressTab), faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 5);

        DevUpper(word, CtrUiText(kDevDexTabWord[i]), sizeof(word));
        DevKeyWord(&r, faceY, DEV_LOOK_KEY, sunk, word);
    }
    page = DevDexGridPage(d, tab, &first, &count, &pages);
    for (int i = 0; i < count; ++i) {
        CtrPokeTouchRect r = DevDexItem(i);
        int row = first + i, look = d->menu[row].locked ? DEV_LOOK_FAINT : DEV_LOOK_KEY;
        int sunk = DevChosen(row, d->menuCursor, pressRow), faceY = DevKey(&r, look, sunk, 6);

        DevArt(DEV_ART_ICON, r.x + 16, faceY + 3, 64, 48);
        DevDexIcon(d->menu[row].id, r.x + 16, faceY + 3, look == DEV_LOOK_KEY);
        DevDexItemWord(d, row, word, sizeof(word));
        DevKeySmallWord(&r, faceY + 56, look, sunk, word);
    }
    DevArrowKeys(&kDevSmallKey[0], &kDevSmallKey[1], page > 0, page < pages - 1, pressed == PT_DEX_HIT_UP,
                 pressed == PT_DEX_HIT_DOWN, 0);
    DevDexCounts(d);
    if (cancel >= 0) {
        int sunk = DevChosen(cancel, d->menuCursor, pressRow), faceY = DevKey(&kDevGridExit, DEV_LOOK_DARK, sunk, 6);

        DevKeyWord(&kDevGridExit, faceY, DEV_LOOK_DARK, sunk, DevCloseWord());
    }
}

/* A caught one's type chips (DevTypeChip, 12 tall: the second when it has one) from (x, y), 3 apart, ending by `right`;
 * a chip that would pass it is left out and counted in sDevKeyCut (FireRed's two widest end far short of it). Returns
 * the x 3 px after them. */
static int DevDexTypes(const CtrPokeTouchDex *d, int x, int y, int right)
{
    for (int i = 0; i < 2; ++i) {
        if (d->type[i] < 0)
            continue;
        if (x + DevTypeChipWidth(d->typeName[i]) > right) {
            ++sDevKeyCut;
            break;
        }
        x += DevTypeChip(d->typeName[i], d->type[i], x, y, 12) + 3;
    }
    return x;
}

/* A list row's pieces (def_dex_list), from its left and top: the number, the name, the capitals; a caught one's mark
 * (8x8) DEV_DEX_MARK_IN from its right, its top level with the capitals, the name ending 4 px before it. */
#define DEV_DEX_NO_X     6
#define DEV_DEX_NAME_X   52
#define DEV_DEX_CAPS_Y   8
#define DEV_DEX_MARK_IN  16
_Static_assert(PT_DEX_ROWS == DEV_GRID_ROWS, "the Pokedex's list shows the grid's rows");

/* A row's word from x, capitals at `top`, ending by `right`: one wider is cut there and counted in sDevKeyCut (none
 * FireRed has is: CHARMELEON's and WIGGLYTUFF's 60 px of the name's 172; the number is No.000 to No.999). */
static void DevDexRowWord(const char *s, int x, int top, int right, uint16_t ink, uint16_t shadow)
{
    if (DevTextWidth(s) > right - x)
        ++sDevKeyCut;
    DevTextInk(s, x, top, right, ink, shadow);
}

/* Row i of the list (the grid's: 24 tall, 26 apart): inverted when chosen; the number and the name ("-----" not seen
 * yet) at the same capitals, a caught one's mark at its right. */
static void DevDexRow(const CtrPokeTouchDexMon *m, int i, int chosen)
{
    CtrPokeTouchRect r = DevGridRow(i, 0);
    int top = r.y + DEV_DEX_CAPS_Y, markX = r.x + r.w - DEV_DEX_MARK_IN, nameX = r.x + DEV_DEX_NAME_X;
    uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;
    char no[12], name[DEV_DEX_WORD];

    DevRow(&r, chosen);
    DevDexNumber(m->number, no, sizeof(no));
    DevDexRowWord(no, r.x + DEV_DEX_NO_X, top, nameX - 4, ink, shadow);
    DevDexRowWord(DevDexName(m, "-----", name, sizeof(name)), nameX, top, markX - 4, ink, shadow);
    if (m->flags & 2) {
        DevArt(DEV_ART_MARK, markX, r.y + DEV_DEX_CAPS_Y, 8, 8);
        DevDexMark(markX, r.y + DEV_DEX_CAPS_Y);
    }
}

/* The ordered list (LIST, def_dex_list): the order and the counts on the title; the grid's five rows, ▲ ▼ grey where
 * the list goes no further; on the note a caught one's type chips (12 tall, from x 20 at y 205) and its kind small
 * after them (from 109 with two chips, capitals 208, one line: a kind wider than its room is cut after the words that
 * fit and counted), else "?????"; BACK. */
static void DevDexList(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    int pressRow = d->pressed >= PT_DEX_HIT_ROW && d->pressed < PT_DEX_HIT_ROW + PT_DEX_ROWS ? d->pressed - PT_DEX_HIT_ROW : -1;
    CtrPokeTouchRect paper;
    char title[DEV_DEX_WORD], counts[3 * DEV_DEX_WORD] = "";

    DevUpper(title, CtrUiText(kDevDexOrder[d->order < 6 ? d->order : 0]), sizeof(title));
    if (d->order == 0 || d->order == 5) {
        char seen[DEV_DEX_WORD], owned[DEV_DEX_WORD];
        int nat = d->order == 5;

        DevUpper(seen, CtrUiText(UIS_DEX_SEEN), sizeof(seen));
        DevUpper(owned, CtrUiText(UIS_DEX_OWNED), sizeof(owned));
        snprintf(counts, sizeof(counts), "%s %u  %s %u", seen, (unsigned)d->seen[nat], owned, (unsigned)d->owned[nat]);
    }
    DevTitleLine(&kDevGridTitle, title, counts);        /* the counts ending at 298, capitals 17 (def_dex_list) */
    DevScreen(&kDevGridScreen);
    for (int i = 0; i < PT_DEX_ROWS && d->listFirst + i < d->listCount; ++i)
        DevDexRow(&d->row[i], i, DevChosen(i, d->listCursor - d->listFirst, pressRow));
    DevArrowKeys(&kDevGridUp, &kDevGridDown, d->listFirst > 0, d->listFirst + PT_DEX_ROWS < d->listCount,
                 d->pressed == PT_DEX_HIT_UP, d->pressed == PT_DEX_HIT_DOWN, 0);
    paper = DevScreen(&kDevGridNote);                   /* the selected one: a caught one's types and kind */
    if (d->sel.flags & 2) {
        char kind[DEV_DEX_WORD];
        int chipY = paper.y + (paper.h - 12) / 2, right = paper.x + paper.w - 6;
        int x = DevDexTypes(d, paper.x + 6, chipY, right) + 3;

        DevUpper(kind, d->kind, sizeof(kind));
        if (DevSmallTextWidth(kind) > right - x)
            ++sDevKeyCut;                               /* the words that fit drawn, the rest left out */
        DevSmallWrap(kind, x, x, chipY + 3, right, 1, DEV_SMALL_LINE, DEV_INK2);
    } else
        DevText("?????", paper.x + 6, paper.y + (paper.h - UI_TEXT_H) / 2, paper.x + paper.w - 6);
    DevFieldBack(v, &kDevGridExit, DevBackWord());
}

/* Slot i of a habitat's page (m its Pokemon, `sunk` chosen), a key on the menu's rows: the picture of one seen (64x64
 * at x + 4, faceY + 1: rows faceY + 1 to faceY + 64 of the 66-tall face, up or sunk) beside its number and its name
 * (from x + 74, capitals faceY + 18 and faceY + 36, ending by x + w - 6: a name wider is cut there and counted in
 * sDevKeyCut, though none FireRed has is: WIGGLYTUFF's 60 px of the 68); a caught one's mark at its top-left corner,
 * over the picture (v0.6.0's place: x + 6, faceY + 4). */
static void DevDexSlotDraw(const CtrPokeTouchDexMon *m, int i, int sunk)
{
    CtrPokeTouchRect r = DevDexSlot(i);
    int faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6), x = r.x + 74, end = r.x + r.w - 6;
    uint16_t ink = DevKeyInk(DEV_LOOK_KEY, sunk), shadow = DevKeyInkShadow(DEV_LOOK_KEY, sunk);
    char text[DEV_DEX_WORD];
    const char *name;

    if (m->flags & 1) {
        DevArt(DEV_ART_PICTURE, r.x + 4, faceY + 1, 64, 64);
        FDPicDraw(m->species, r.x + 4, faceY + 1, 0);
    }
    if (m->flags & 2) {
        DevArt(DEV_ART_MARK, r.x + 6, faceY + 4, 8, 8);
        DevDexMark(r.x + 6, faceY + 4);
    }
    DevDexNumber(m->number, text, sizeof(text));
    DevTextInk(text, x, faceY + 18, end, ink, shadow);
    name = DevDexName(m, "?????", text, sizeof(text));
    if (DevTextWidth(name) > end - x)
        ++sDevKeyCut;
    DevTextInk(name, x, faceY + 36, end, ink, shadow);
}

/* A habitat's page (CATEGORY): the menu's grid (no board of its own). */
static void DevDexCategory(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    int pressSlot = d->pressed >= PT_DEX_HIT_SLOT && d->pressed < PT_DEX_HIT_SLOT + 4 ? d->pressed - PT_DEX_HIT_SLOT : -1;
    CtrPokeTouchRect paper;
    char habitat[DEV_DEX_WORD], word[DEV_DEX_WORD], page[2 * DEV_DEX_WORD];

    DevDexHabitat(d, habitat, sizeof(habitat));
    DevUpper(word, CtrUiText(UIS_DEX_PAGE), sizeof(word));
    snprintf(page, sizeof(page), "%s %u/%u", word, (unsigned)d->page, (unsigned)d->pages);
    DevTitleLine(&kDevGridTitle, habitat, page);        /* "PAGE n/m" ending at 298, capitals 17 */
    for (int i = 0; i < d->slots && i < 4; ++i)
        DevDexSlotDraw(&d->mon[i], i, DevChosen(i, d->slot, pressSlot));
    DevArrowKeys(&kDevSmallKey[0], &kDevSmallKey[1], d->canPrev, d->canNext, d->pressed == PT_DEX_HIT_PREV,
                 d->pressed == PT_DEX_HIT_NEXT, 1);
    paper = DevScreen(&kDevSmallNote);                  /* the selected one: its number small, its name */
    if (d->sel.species) {
        DevDexNumber(d->sel.number, word, sizeof(word));
        DevSmallText(word, paper.x + 6, paper.y + 4, DEV_INK2, paper.x + paper.w - 6);
        DevText(DevDexName(&d->sel, "?????", word, sizeof(word)), paper.x + 6, paper.y + 16, paper.x + paper.w - 6);
    }
    DevFieldBack(v, &kDevGridExit, DevBackWord());
}

/* A Pokemon's page (PAGE): the picture (HeartGold's 80x80 when the pack has it, else the 64x64 centred), the number and
 * the name, the kind, a caught one's types, height and weight beside it, the footprint, the rule and the entry; AREA,
 * CRY and BACK, or the page a catch added: OK. */
static void DevDexPage(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;
    const CtrPokeTouchRect *data = &kDevDexData;
    int caught = (d->sel.flags & 2) != 0, x = data->x, right = data->x + data->w;
    CtrPokeTouchRect row = {data->x, (int16_t)(data->y + 52), 120, 14};
    char no[12], word[DEV_DEX_WORD], value[32];

    DevScreen(&kDevTallScreen);
    DevArt(DEV_ART_PICTURE, kDevDexPic.x, kDevDexPic.y, kDevDexPic.w, kDevDexPic.h);
    FDPicDrawBig(d->sel.species, kDevDexPic.x + 8, kDevDexPic.y + 8);
    DevDexNumber(d->sel.number, no, sizeof(no));
    DevText2(no, x, data->y + 2, right);
    DevUpper(word, d->sel.name, sizeof(word));
    DevText(word, x + DevTextWidth(no) + 8, data->y + 2, right);
    if (caught)
        DevUpper(word, d->kind, sizeof(word));
    DevText2(caught ? word : "?????", x, data->y + 18, right);
    if (caught)
        DevDexTypes(d, x, data->y + 37, right);
    DevUpper(word, CtrUiText(UIS_DEX_HT), sizeof(word));
    DevDexHeight(d, value, sizeof(value));
    DevLabelValue(&row, word, value);
    row.y = (int16_t)(row.y + row.h);
    DevUpper(word, CtrUiText(UIS_DEX_WT), sizeof(word));
    DevDexWeight(d, value, sizeof(value));
    DevLabelValue(&row, word, value);
    DevArt(DEV_ART_MARK, kDevDexFoot.x, kDevDexFoot.y, kDevDexFoot.w, kDevDexFoot.h);
    DevDexFootprint(d->sel.species, kDevDexFoot.x, kDevDexFoot.y);
    DevRect(kDevDexRule.x, kDevDexRule.y, kDevDexRule.w, kDevDexRule.h, DEV_INK_SH);
    DevWrap(d->desc, kDevDexEntry.x, kDevDexEntry.y + 1, kDevDexEntry.w, 5, DEV_INK, DEV_INK_SH);
    if (d->added) {                                     /* OK: the battle's A, the only key */
        int faceY = DevKey(&kDevWideKey, DEV_LOOK_KEY, 1, 6);

        DevUpper(word, CtrUiText(UIS_OK), sizeof(word));
        DevKeyWord(&kDevWideKey, faceY, DEV_LOOK_KEY, 1, word);
        return;
    }
    for (int k = 0; k < 2; ++k) {
        const CtrPokeTouchRect *r = k ? &kDevDexCry : &kDevDexArea;
        int sunk = d->pressed == (k ? PT_DEX_HIT_CRY : PT_DEX_HIT_AREA), faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);

        DevUpper(word, CtrUiText(k ? UIS_DEX_CRY : UIS_DEX_AREA), sizeof(word));
        DevKeyWord(r, faceY, DEV_LOOK_KEY, sunk, word);
    }
    DevFieldBack(v, &kDevTallBack, DevBackWord());
}

/* The Pokedex whole (after DevBegin: the dispatcher's); busy with nothing kept yet, the shell. */
static void DevDexDraw(const CtrPokeTouchView *v)
{
    DevShell();
    if (v->dex.state == PT_DEX_STATE_MENU)
        DevDexMenu(v);
    else if (v->dex.state == PT_DEX_STATE_LIST)
        DevDexList(v);
    else if (v->dex.state == PT_DEX_STATE_CATEGORY)
        DevDexCategory(v);
    else if (v->dex.state == PT_DEX_STATE_PAGE)
        DevDexPage(v);
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the Pokedex draws differs between `a` and `b` (the shell is the dispatcher's): the Pokedex as filled but for
 * whether the National Pokedex is on (never drawn) and the habitat (drawn only on its page's title), and BACK. */
static int DevDexSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    CtrPokeTouchDex p, q;

    memcpy(&p, &a->dex, sizeof(p));
    memcpy(&q, &b->dex, sizeof(q));
    p.national = q.national = 0;
    if (q.state != PT_DEX_STATE_CATEGORY)
        p.category = q.category = 0;
    return !memcmp(&p, &q, sizeof(p)) && DevFieldBackSame(a, b);
}

/* BACK as drawn now: the grid's exit beside a list or a habitat's page, the tall BACK on a Pokemon's page; none on the
 * top menu (its CLOSE is FireRed's CLOSE POKeDEX), the page a catch added or while nothing is kept. */
static const CtrPokeTouchRect *DevDexBackKey(const CtrPokeTouchView *v)
{
    const CtrPokeTouchDex *d = &v->dex;

    if (!v->backVisible)
        return NULL;
    if (d->state == PT_DEX_STATE_LIST || d->state == PT_DEX_STATE_CATEGORY)
        return &kDevGridExit;
    return d->state == PT_DEX_STATE_PAGE && !d->added ? &kDevTallBack : NULL;
}
