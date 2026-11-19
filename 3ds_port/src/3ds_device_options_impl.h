/* OPTIONS as the red device (def_opt_settings, def_opt_graphics, def_opt_looks), included by 3ds_poketouch.c after
 * the Summary: a row of the field dispatcher (3ds_device_field_impl.h), shown while view->options and there is no GBA
 * picture (DevOptShown).
 *
 * The grid's list page. Across the top the four sections as the grid's tabs, each named by its first page whichever page
 * is shown, the shown one sunk, or the one under a held touch. The page's rows on the list's screen, the row described
 * inverted (or the one under a held touch): its label, ◀ in PREV, the value centred in the board's box and ▶; a
 * switch's value the port's ON / OFF; a row of levels its levels in boxes the size of their words, packed right, the
 * chosen one outlined (a held one instead). ▲ ▼ beside the rows while there are pages to turn (always: the six
 * wrap). Under the rows a section of several pages (GRAPHICS' three) has its page line, PAGE 2/3 · LOOKS, and a dot a
 * page, the shown one's on. Under them the description in the small note (the small font, three lines), ◀ ▶ stepping
 * the row described (sunk while its own zone is held), and BACK, a dark key (grey while it cannot be used). FRAME's
 * sample (the frame chosen, from the pack) under five rows; a look's panel (its walking picture, its name and its
 * artists) under four. Every rectangle is 3ds_device_layout.h's, a level's box the row's sized by its word
 * (DevOptLevelBox), so what is drawn is what a touch takes.
 *
 * Words (decision 5): v0.6.0's words (the sections, the page's name, the labels, the values, the levels, the
 * description, a look's name) through DevUpper; the port's in capitals; a look's credit (its artists' names) as typed.
 *
 * Redraw: the screen whole when what it draws differs (DevOptSame), and nothing else. */

#define DEV_OPT_WORD 24                         /* a word as drawn (a label, a value, a level, a section, a look's name) */

static int DevOptShown(const CtrPokeTouchView *v)
{
    return v->options && !v->viewport;
}

static int DevOptRows(const CtrPokeTouchView *v)
{
    return v->optCount < PT_OPT_MAX ? v->optCount : PT_OPT_MAX;
}

static int DevOptLevels(const CtrPokeTouchView *v, int i)
{
    return v->optSegCount[i] < PT_OPT_SEGS ? v->optSegCount[i] : PT_OPT_SEGS;
}

/* ---- A row of levels (def_opt_graphics' COMPANION: OFF, FOLLOW, BE IT) ---------------------------------------------
 * Each level's word (DevUpper'd) in a box of its width + 8, 14 tall from the row's top + 2, the boxes packed right 4
 * apart, the last ending 10 px before the row's end (x 250): they have the 114 px from PREV (x 136) to there. The
 * normal font while the set fits there, else the small one (DevOptLevelSmall); a set too wide even small shares the
 * 114 px out evenly, its words cut in their boxes, and is counted in sDevKeyCut as it is drawn. A touch on PREV or
 * NEXT is the level there, the gaps split at their middles (DevOptLevelAt). */

/* Where row i's levels end: the row's end less 10. */
static int DevOptLevelEnd(int i)
{
    CtrPokeTouchRect r = DevOptRow(i);

    return r.x + r.w - 10;
}

/* Level k's word on row i as drawn, its width in the small font or the normal one. */
static int DevOptLevelWidth(const CtrPokeTouchView *v, int i, int k, int small)
{
    char word[DEV_OPT_WORD];

    DevUpper(word, v->optSegName[i][k], sizeof(word));
    return small ? DevSmallTextWidth(word) : DevTextWidth(word);
}

/* What row i's levels need in that font: each word + 8, 4 between them. */
static int DevOptLevelNeed(const CtrPokeTouchView *v, int i, int small)
{
    int n = DevOptLevels(v, i), need = 4 * (n - 1);

    for (int k = 0; k < n; ++k)
        need += DevOptLevelWidth(v, i, k, small) + 8;
    return need;
}

/* 1 when row i's levels in the normal font need more than the 114 px from PREV: they are drawn in the small one. */
static int DevOptLevelSmall(const CtrPokeTouchView *v, int i)
{
    return DevOptLevelNeed(v, i, 0) > DevOptLevelEnd(i) - DEV_OPT_PREV_X;
}

/* 1 when they need more than that even small: drawn cut (DevOptLevelBox shares the room out), and counted. */
static int DevOptLevelCut(const CtrPokeTouchView *v, int i)
{
    return DevOptLevelSmall(v, i) && DevOptLevelNeed(v, i, 1) > DevOptLevelEnd(i) - DEV_OPT_PREV_X;
}

/* Level k's box on row i: its word + 8 (in the font DevOptLevelSmall picks) by 14 from the row's top + 2, the boxes
 * packed right 4 apart, the last ending at DevOptLevelEnd; a set too wide even small has the 114 px shared out evenly. */
static CtrPokeTouchRect DevOptLevelBox(const CtrPokeTouchView *v, int i, int k)
{
    CtrPokeTouchRect r = DevOptRow(i);
    int n = DevOptLevels(v, i), small = DevOptLevelSmall(v, i), end = DevOptLevelEnd(i);
    int even = DevOptLevelCut(v, i) ? (end - DEV_OPT_PREV_X - 4 * (n - 1)) / n : 0;

    for (int j = n - 1; j >= k; --j) {
        r.w = (int16_t)(even ? even : DevOptLevelWidth(v, i, j, small) + 8);
        r.x = (int16_t)(end - r.w);
        end = r.x - 4;
    }
    r.y = (int16_t)(r.y + 2);
    r.h = 14;
    return r;
}

/* The level of row i under column x of PREV or NEXT: each from the middle of the gap before its box (the first from
 * PREV) to the middle of the gap after it (the last to the row's end). */
static int DevOptLevelAt(const CtrPokeTouchView *v, int i, int x)
{
    int k = DevOptLevels(v, i) - 1;

    while (k > 0 && x < DevOptLevelBox(v, i, k).x - 2)
        --k;
    return k;
}

/* ---- Where a touch goes ---------------------------------------------------------------------------------------------- */

bool CtrPokeTouch_OptionHit(const CtrPokeTouchView *v, int x, int y, int *row, int *zone)
{
    if (!DevOptShown(v))
        return false;
    for (int k = 0; k < 2; ++k)
        if (v->optSelected < DevOptRows(v) && DevOnKey(&kDevSmallKey[k], x, y)) {
            *row = v->optSelected;
            *zone = k ? PT_OPT_ZONE_NEXT : PT_OPT_ZONE_PREV;
            return true;
        }
    for (int i = 0; i < DevOptRows(v); ++i) {
        CtrPokeTouchRect r = DevOptRow(i);

        if (!UiIn(&r, x, y))
            continue;
        *row = i;
        *zone = x < DEV_OPT_PREV_X ? PT_OPT_ZONE_LABEL : x < DEV_OPT_NEXT_X ? PT_OPT_ZONE_PREV : PT_OPT_ZONE_NEXT;
        if (DevOptLevels(v, i) && *zone != PT_OPT_ZONE_LABEL)
            *zone = PT_OPT_ZONE_SEG + DevOptLevelAt(v, i, x);
        return true;
    }
    return false;
}

int CtrPokeTouch_OptionKey(const CtrPokeTouchView *v, int x, int y)
{
    if (!DevOptShown(v))
        return -1;
    for (int s = 0; s < v->optSectionCount && s < PT_OPT_SECTIONS; ++s) {
        CtrPokeTouchRect r = DevGridTab(4, s);

        if (DevOnKey(&r, x, y))
            return PT_OPT_KEY_SECTION + s;
    }
    if (v->optPageCount > 1 && DevOnKey(&kDevGridUp, x, y))
        return PT_OPT_KEY_UP;
    if (v->optPageCount > 1 && DevOnKey(&kDevGridDown, x, y))
        return PT_OPT_KEY_DOWN;
    return -1;
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

/* FRAME's sample: the chosen frame (the game's own graphics/text_window/typeN, 3x3 tiles of 8x8) as FireRed draws a
 * window with it, stretched to kDevOptSample, with YES / NO inside. The tiles and palette are read from the pack when
 * the type changes; if they are not there the sample is left out. */
static int FrameLoadFile(const char *base, const char *ext, uint8_t *out, uint32_t cap, uint32_t want)
{
    char path[64];
    uint32_t len = 0;
    uint8_t *data;
    int n = 0;

    snprintf(path, sizeof(path), "graphics/text_window/%s.%s", base, ext);
    data = CtrData_Load(path, &len);
    if (data != NULL && len == want) {
        memcpy(out, data, want);
        n = (int)want;
    }
    free(data);
    if (n == (int)want)
        return n;
    snprintf(path, sizeof(path), "graphics/text_window/%s.%s.lz", base, ext);
    return PokeTouch_LoadLz(path, out, cap) == (int)want ? (int)want : 0;
}

static int FrameSample(int type, int x, int y)
{
    static int loaded = -1;
    static uint16_t px[24 * 24];
    static int ok;
    const int w = kDevOptSample.w, h = kDevOptSample.h;
    char word[DEV_OPT_WORD];

    if (loaded != type) {
        uint8_t tiles[9 * 32], colors[32];
        char base[16];
        uint16_t pal[16];

        loaded = type;
        ok = 0;
        snprintf(base, sizeof(base), "type%d", type + 1);
        if (FrameLoadFile(base, "4bpp", tiles, sizeof(tiles), sizeof(tiles))
            && FrameLoadFile(base, "gbapal", colors, sizeof(colors), sizeof(colors))) {
            for (int i = 0; i < 16; ++i) {
                unsigned c = (unsigned)colors[i * 2] | (unsigned)colors[i * 2 + 1] << 8;

                pal[i] = GbaColor(c);
            }
            for (int ty = 0; ty < 24; ++ty)
                for (int tx = 0; tx < 24; ++tx) {
                    uint8_t b = tiles[(ty / 8 * 3 + tx / 8) * 32 + ty % 8 * 4 + tx % 8 / 2];
                    uint8_t v = (tx & 1) ? b >> 4 : b & 15;

                    px[ty * 24 + tx] = v ? pal[v] : RGB(255, 255, 255);
                }
            ok = 1;
        }
    }
    if (!ok)
        return 0;
    for (int dy = 0; dy < h; ++dy)
        for (int dx = 0; dx < w; ++dx) {
            int sx = dx < 8 ? dx : dx >= w - 8 ? 16 + dx - (w - 8) : 8 + dx % 8;
            int sy = dy < 8 ? dy : dy >= h - 8 ? 16 + dy - (h - 8) : 8 + dy % 8;

            Px(x + dx, y + dy, px[sy * 24 + sx]);
        }
    DevUpper(word, CtrUiText(UIS_YES), sizeof(word));
    DevText(word, x + 14, y + 6, x + w - 6);
    DevUpper(word, CtrUiText(UIS_NO), sizeof(word));
    DevText(word, x + 14, y + 18, x + w - 6);
    return 1;
}

/* A word from x to `right` (centred in it when `centre`), its capitals from `top`: cut at `right`, and then counted in
 * sDevKeyCut, when it does not fit (no word of a row may be cut). */
static void DevOptWord(const char *s, int x, int right, int top, int centre, uint16_t ink, uint16_t shadow)
{
    int w = DevTextWidth(s);

    if (w > right - x)
        ++sDevKeyCut;
    DevTextInk(s, centre && w < right - x ? x + (right - x - w) / 2 : x, top, right, ink, shadow);
}

/* The four sections (each its first page's name): the shown one sunk, or the one under a held touch. */
static void DevOptSections(const CtrPokeTouchView *v)
{
    int press = v->optKey >= PT_OPT_KEY_SECTION && v->optKey < PT_OPT_KEY_SECTION + PT_OPT_SECTIONS
                ? v->optKey - PT_OPT_KEY_SECTION : -1;
    char word[DEV_OPT_WORD];

    for (int s = 0; s < v->optSectionCount && s < PT_OPT_SECTIONS; ++s) {
        CtrPokeTouchRect r = DevGridTab(4, s);
        int sunk = DevChosen(s, v->optSection, press), faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 5);

        DevUpper(word, v->optSectionName[s], sizeof(word));
        DevKeyWord(&r, faceY, DEV_LOOK_KEY, sunk, word);
    }
}

/* The chosen level's outline: 1 px on its box's own edge, in the row's ink. */
static void DevOptBox(const CtrPokeTouchRect *s, uint16_t ink)
{
    DevRect(s->x, s->y, s->w, 1, ink);
    DevRect(s->x, s->y + s->h - 1, s->w, 1, ink);
    DevRect(s->x, s->y + 1, 1, s->h - 2, ink);
    DevRect(s->x + s->w - 1, s->y + 1, 1, s->h - 2, ink);
}

/* Row i: inverted when it is the one described (or under a held touch); its label, then ◀ value ▶, or its levels (each
 * word centred in its box, the chosen one or the held one outlined). */
static void DevOptRowDraw(const CtrPokeTouchView *v, int i)
{
    CtrPokeTouchRect r = DevOptRow(i);
    int chosen = DevChosen(i, v->optSelected, v->optPressed), top = r.y + (r.h - UI_TEXT_H) / 2, mid = r.y + r.h / 2;
    int count = DevOptLevels(v, i);
    uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;
    char word[DEV_OPT_WORD];

    DevRow(&r, chosen);
    DevUpper(word, v->optLabel[i], sizeof(word));
    DevOptWord(word, r.x + 6, DEV_OPT_PREV_X, top, 0, ink, shadow);
    if (count) {
        int held = v->optPressed == i && v->optZone >= PT_OPT_ZONE_SEG ? v->optZone - PT_OPT_ZONE_SEG : -1;
        int small = DevOptLevelSmall(v, i);

        if (DevOptLevelCut(v, i))
            ++sDevKeyCut;
        for (int k = 0; k < count; ++k) {
            CtrPokeTouchRect s = DevOptLevelBox(v, i, k);

            DevUpper(word, v->optSegName[i][k], sizeof(word));
            DevFitWord(word, small, s.x + 4, s.w - 8, s.y, s.h, 1, ink, shadow);
            if (DevChosen(k, v->optSegValue[i], held))
                DevOptBox(&s, ink);
        }
        return;
    }
    DevTriangle(DEV_OPT_PREV_X + (DEV_OPT_NEXT_X - DEV_OPT_PREV_X) / 2, mid, 2, ink);
    DevUpper(word, !v->optSwitch[i] ? v->optValue[i] : v->optSwitch[i] == 2 ? CTR_TR("ON", "ON", "AN")
                   : CTR_TR("OFF", "OFF", "AUS"), sizeof(word));
    DevOptWord(word, DEV_OPT_VALUE_X, DEV_OPT_VALUE_X + DEV_OPT_VALUE_W, top, 1, ink, shadow);
    DevTriangle(DEV_OPT_NEXT_ARROW, mid, 3, ink);
}

/* The look's panel under four rows (def_opt_looks): its walking picture (facing down) at its own size at (22, 124), its
 * name from x 62 (capitals 128) and its artists under it (capitals 142), two small lines at most (more is counted). */
static void DevOptLookPanel(const CtrPokeTouchView *v)
{
    const CtrPokeTouchLook *l = &v->optLook;
    const CtrPokeTouchRect *r = &kDevOptLook;
    int px = r->x + 6, py = r->y + 2, textX = r->x + 46, right = r->x + r->w - 6;
    char name[DEV_OPT_WORD];

    DevRect(r->x, r->y, r->w, r->h, DEV_PAPER2);
    if (l->has) {
        static uint8_t pixels[32 * 32];
        uint16_t pal[16];

        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x)
                pixels[y * 32 + x] = (uint8_t)((l->tiles[(y / 8 * 4 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2] >> (x & 1) * 4) & 15);
        for (int i = 0; i < 16; ++i)
            pal[i] = GbaColor(l->palette[i]);
        DevArt(DEV_ART_SPRITE, px, py, 32, 32);
        UiImage(pixels, pal, 32, 32, 32, px, py, 1, 1);
    }
    DevUpper(name, l->name, sizeof(name));
    DevText(name, textX, r->y + 6, right);
    DevSmallFit(l->credit, textX, textX, r->y + 20, right, 2, DEV_SMALL_LINE_TIGHT, DEV_INK2);
}

/* A section of several pages: its page line under the rows, PAGE 2/3 · LOOKS (the small font, the secondary ink, from
 * x 22, ending by FRAME's sample less 4: cut there, not counted, as FireRed's page names fit), and a dot a page at the
 * paper's right, the page shown's on. */
static void DevOptPageLine(const CtrPokeTouchView *v)
{
    char page[DEV_OPT_WORD], name[DEV_OPT_WORD], line[2 * DEV_OPT_WORD + 16];

    if (v->optSectionPages <= 1)
        return;
    DevUpper(page, CtrUiText(UIS_DEX_PAGE), sizeof(page));
    DevUpper(name, v->optPageName, sizeof(name));
    snprintf(line, sizeof(line), "%s %u/%u \xC2\xB7 %s", page, (unsigned)v->optSectionPage, (unsigned)v->optSectionPages, name);
    DevSmallText(line, 22, DEV_OPT_PAGE_Y, DEV_INK2, kDevOptSample.x - 4);
    for (int k = 0; k < v->optSectionPages && k < 3; ++k) {
        CtrPokeTouchRect d = DevOptDot(k);

        DevSpot(d.x, d.y, d.w, k == v->optSectionPage - 1 ? DEV_MARK_ON : DEV_PAPER_LINE);
    }
}

/* ▲ ▼ while there are pages (always, from the fill: the six wrap, so both are on): sunk under a held touch. */
static void DevOptPages(const CtrPokeTouchView *v)
{
    if (v->optPageCount > 1)
        DevArrowKeys(&kDevGridUp, &kDevGridDown, 1, 1, v->optKey == PT_OPT_KEY_UP, v->optKey == PT_OPT_KEY_DOWN, 0);
}

/* The row described's description: its two lines as one text, the small font from x 18 to 150, three lines 10 apart
 * from capitals 198 (the third's 218: its descenders end on the paper's last row); a text that needs more is counted. */
static void DevOptNote(const CtrPokeTouchView *v)
{
    char text[2 * PT_BAR_LEN + 1], upper[2 * PT_BAR_LEN + 1];

    DevScreen(&kDevSmallNote);
    snprintf(text, sizeof(text), "%s %s", v->bar[1], v->bar[2]);
    DevUpper(upper, text, sizeof(upper));
    DevSmallFit(upper, 18, 18, 198, 150, 3, DEV_SMALL_LINE, DEV_INK);
}

/* ◀ ▶: they step the row described, sunk while its own zone of that kind is held. */
static void DevOptSteps(const CtrPokeTouchView *v)
{
    for (int k = 0; k < 2; ++k) {
        const CtrPokeTouchRect *r = &kDevSmallKey[k];
        int sunk = v->optPressed >= 0 && v->optPressed == v->optSelected
                   && v->optZone == (k ? PT_OPT_ZONE_NEXT : PT_OPT_ZONE_PREV);
        int faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);

        DevTriangle(r->x + r->w / 2, faceY + r->h / 2, k ? 3 : 2, DevKeyInk(DEV_LOOK_KEY, sunk));
    }
}

/* OPTIONS whole (after DevBegin: the dispatcher's). */
static void DevOptDraw(const CtrPokeTouchView *v)
{
    DevShell();
    DevOptSections(v);
    DevScreen(&kDevGridScreen);
    for (int i = 0; i < DevOptRows(v); ++i)
        DevOptRowDraw(v, i);
    if (v->optFrameShow) {
        DevArt(DEV_ART_FRAME, kDevOptSample.x, kDevOptSample.y, kDevOptSample.w, kDevOptSample.h);
        FrameSample(v->optFrameType, kDevOptSample.x, kDevOptSample.y);
    }
    if (v->optLook.on && DevOptRows(v) <= 4)
        DevOptLookPanel(v);
    DevOptPageLine(v);
    DevOptPages(v);
    DevOptNote(v);
    DevOptSteps(v);
    DevFieldBack(v, &kDevGridExit, DevBackWord());          /* BACK while shown */
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Row i of `a` and `b` draws the same: its label and its levels, its switch or its value. */
static int DevOptRowSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b, int i)
{
    if (strcmp(a->optLabel[i], b->optLabel[i]) || DevOptLevels(a, i) != DevOptLevels(b, i))
        return 0;
    if (DevOptLevels(b, i)) {
        for (int k = 0; k < DevOptLevels(b, i); ++k)
            if (strcmp(a->optSegName[i][k], b->optSegName[i][k]))
                return 0;
        return a->optSegValue[i] == b->optSegValue[i];
    }
    return a->optSwitch[i] == b->optSwitch[i] && (b->optSwitch[i] || !strcmp(a->optValue[i], b->optValue[i]));
}

/* Nothing OPTIONS draws differs between `a` and `b` (the shell is the dispatcher's). */
static int DevOptSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    int look = b->optLook.on && DevOptRows(b) <= 4;

    if (a->optSection != b->optSection || a->optSectionCount != b->optSectionCount || a->optKey != b->optKey
        || (a->optPageCount > 1) != (b->optPageCount > 1) || (a->optSectionPages > 1) != (b->optSectionPages > 1)
        || (b->optSectionPages > 1 && (a->optSectionPages != b->optSectionPages || a->optSectionPage != b->optSectionPage
                                       || strcmp(a->optPageName, b->optPageName)))
        || DevOptRows(a) != DevOptRows(b)
        || a->optSelected != b->optSelected || a->optPressed != b->optPressed
        || (b->optPressed >= 0 && a->optZone != b->optZone) || a->optFrameShow != b->optFrameShow
        || (b->optFrameShow && a->optFrameType != b->optFrameType) || (a->optLook.on && DevOptRows(a) <= 4) != look
        || strcmp(a->bar[1], b->bar[1]) || strcmp(a->bar[2], b->bar[2]) || !DevFieldBackSame(a, b))
        return 0;
    for (int s = 0; s < b->optSectionCount && s < PT_OPT_SECTIONS; ++s)
        if (strcmp(a->optSectionName[s], b->optSectionName[s]))
            return 0;
    for (int i = 0; i < DevOptRows(b); ++i)
        if (!DevOptRowSame(a, b, i))
            return 0;
    return !look || (a->optLook.has == b->optLook.has && !strcmp(a->optLook.name, b->optLook.name)
                     && !strcmp(a->optLook.credit, b->optLook.credit)
                     && (!b->optLook.has || (!memcmp(a->optLook.tiles, b->optLook.tiles, sizeof(b->optLook.tiles))
                                             && !memcmp(a->optLook.palette, b->optLook.palette, sizeof(b->optLook.palette)))));
}

/* BACK as drawn now: kDevGridExit while shown. */
static const CtrPokeTouchRect *DevOptBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevGridExit : NULL;
}
