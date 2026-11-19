/* The Pokemon Summary as the red device (style B; the Definitiva's boards def_sum_info, def_sum_stats, def_sum_moves),
 * included by 3ds_poketouch.c after the battle: CtrSummaryUi_Draw / Update / Hit / Tab (3ds_poketouch.h). One Summary
 * for the field's Party, the battle's and a PC box's; it only shows what FireRed's Summary reports (CtrSummaryView), and
 * every page, Pokemon and move cursor is FireRed's own (3ds_bottom.c walks them with FireRed's keys). Every rectangle is
 * 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * On the grid. Across the top the four tabs INFO, STATS, MOVES and MEMO (FireRed's first page shown as its trainer memo:
 * the port's), the shown one sunk, beige when it cannot be used (an egg has INFO and MEMO only; nothing while a Pokemon
 * loads). The screen, with ▲ and ▼ beside it: the previous and next Pokemon (FireRed's UP and DOWN), grey and not taken
 * in the move details, where those keys move FireRed's cursor instead. On the screen the Pokemon in its left column (name,
 * gender and level, picture, types and its number; on MOVES its name and level over a rule, and in the details the
 * chosen move's type, category, power and accuracy); right of it the page, from x 116 to 256: INFO's rows (species, OT,
 * ID No., item, nature, ability, HP) with the HP and EXP bars; STATS' six stats with a bar each (the nature's marks: the
 * stat it raises red with a triangle up, the one it lowers blue with one down) and the switch to the IVs, EVs and Hidden
 * Power; the moves' rows (FireRed's cursor inverted in the details, CANCEL under them); MEMO's rows (nature, ribbons) and
 * FireRed's memo (an egg: how soon it hatches). Under it the note: the ability and what it does, the chosen move's
 * description, or what the page lets one do; BACK, the dark key in the exit corner.
 *
 * Four screws (DevShell). The chosen key is the one under a held touch, else the page's (DevChosen). Words: upper case
 * (decision 5): FireRed's names through DevUpper, the Pokemon's name in capitals when it is its species' own and as typed
 * when the player gave it; FireRed's descriptions and memo as they are; the port's words written in capitals.
 *
 * Redraw (decision 7): a change of anything drawn redraws the screen whole; what no page draws (where the Summary was
 * opened from, FireRed's readiness, a touch held on the switch) and what the page shown does not (the move cursor, the
 * swap and the description outside the details, the swap's other row while not swapping, the IV / EV view outside
 * STATS) is copied over before the comparison (DevSumUndrawn). */

#include "3ds_nature.h"         /* CtrNature_Up / _Down: STATS' marks */

#define DEV_SUM_BAR        UI_RGB(0x8A, 0x94, 0xA8)   /* STATS' bars and their track (the board's) */
#define DEV_SUM_BAR_TRACK  UI_RGB(0xE2, 0xE2, 0xDA)
#define DEV_SUM_LEFT       116                        /* the page: 140 px from x 116, right of the column */
#define DEV_SUM_RIGHT      256
#define DEV_SUM_ROW_TOP    53                         /* INFO's and MEMO's rows: capitals from 53, 15 apart */
#define DEV_SUM_ROW_DY     15
#define DEV_SUM_STAT_Y     69                         /* STATS' six rows: 13 tall, 14 apart */
#define DEV_SUM_STAT_H     13
#define DEV_SUM_STAT_DY    14
#define DEV_SUM_COL_X      20                         /* the column's words and chips: from x 20, ending by 106 */
#define DEV_SUM_COL_END    106
#define DEV_SUM_COL_VALUE  104                        /* the details' POWER and ACCURACY values end here */

/* The tab shown: STATS on FireRed's skills page, MOVES on its moves page and its details, MEMO on its first page when
 * the port's MEMO was chosen, INFO otherwise. */
int CtrSummaryUi_Tab(const CtrSummaryUiView *v)
{
    int page = v->data.page;

    return page == SUI_PAGE_SKILLS ? SUI_TAB_STATS : page >= SUI_PAGE_MOVES ? SUI_TAB_MOVES
         : v->trainer ? SUI_TAB_TRAINER : SUI_TAB_INFO;
}

/* What can be used now: an egg has only its first page (INFO, MEMO); the previous and next Pokemon are FireRed's UP and
 * DOWN, which move the cursor instead in the move details (FireRed walks the party from an egg's first page too). Nothing
 * while a Pokemon loads but BACK. */
static int DevSumTabOn(const CtrSummaryUiView *v, int tab)
{
    return !v->loading && (!v->data.egg || tab == SUI_TAB_INFO || tab == SUI_TAB_TRAINER);
}

static int DevSumArrowsOn(const CtrSummaryUiView *v)
{
    return !v->loading && v->data.page != SUI_PAGE_MOVES_INFO;
}

static int DevSumMovePage(const CtrSummaryUiView *v)
{
    return !v->loading && (v->data.page == SUI_PAGE_MOVES || v->data.page == SUI_PAGE_MOVES_INFO);
}

/* The details' CANCEL row: in the details, not while a move is being moved. */
static int DevSumCancelOn(const CtrSummaryUiView *v)
{
    return !v->loading && v->data.page == SUI_PAGE_MOVES_INFO && !v->data.swapping;
}

/* The touch held as it is drawn: a key's or a row's code where that key or row is drawn and can be used, else -1 (the
 * switch's: not drawn). Every key and row is chosen by it, not by the raw touch: a touch on what is not there (the CANCEL
 * a swap hides, a slot without a move, a grey tab) takes nothing from FireRed's cursor. */
static int DevSumPressDrawn(const CtrSummaryUiView *v)
{
    int p = v->pressed;

    if (p == SUI_HIT_BACK)
        return p;
    if (p >= SUI_HIT_TAB && p < SUI_HIT_TAB + SUI_TABS)
        return DevSumTabOn(v, p - SUI_HIT_TAB) ? p : -1;
    if (p == SUI_HIT_PREV || p == SUI_HIT_NEXT)
        return DevSumArrowsOn(v) ? p : -1;
    if (p >= SUI_HIT_MOVE && p < SUI_HIT_MOVE + 4)
        return DevSumMovePage(v) && v->data.moves[p - SUI_HIT_MOVE] ? p : -1;
    if (p == SUI_HIT_CANCEL)
        return DevSumCancelOn(v) ? p : -1;
    return -1;
}

/* Keys are hit on their face and drop (DevOnKey), the switch on its larger rectangle, rows on themselves. */
int CtrSummaryUi_Hit(const CtrSummaryUiView *v, int x, int y)
{
    const CtrSummaryView *d = &v->data;
    CtrPokeTouchRect r;

    if (DevOnKey(&kDevGridExit, x, y))
        return SUI_HIT_BACK;
    if (v->loading)
        return -1;
    if (DevSumArrowsOn(v)) {
        if (DevOnKey(&kDevGridUp, x, y))
            return SUI_HIT_PREV;
        if (DevOnKey(&kDevGridDown, x, y))
            return SUI_HIT_NEXT;
    }
    for (int i = 0; i < SUI_TABS; ++i) {
        r = DevGridTab(SUI_TABS, i);
        if (DevSumTabOn(v, i) && DevOnKey(&r, x, y))
            return SUI_HIT_TAB + i;
    }
    if (d->page == SUI_PAGE_SKILLS && !d->egg && UiIn(&kDevSumIvSwitchHit, x, y))
        return SUI_HIT_IVTOGGLE;
    if (DevSumMovePage(v)) {
        for (int i = 0; i < 4; ++i) {
            r = DevSumRow(i);
            if (d->moves[i] && UiIn(&r, x, y))
                return SUI_HIT_MOVE + i;
        }
        r = DevSumRow(4);
        if (DevSumCancelOn(v) && UiIn(&r, x, y))
            return SUI_HIT_CANCEL;
    }
    return -1;
}

/* ---- v0.6.0's readers: numbers, the picture and the icon, the EXP, the hint ------------------------------------------ */

/* A number at the start of a Summary string ("15/15": 15; after the slash with `after`). */
static unsigned SNumber(const char *s, int after)
{
    unsigned n = 0;

    if (after) {
        const char *slash = strchr(s, '/');
        if (!slash)
            return 0;
        s = slash + 1;
    }
    while (*s == ' ')
        ++s;
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (unsigned)(*s++ - '0');
    return n;
}

/* The Pokemon's icon (FireRed's 32x32 icon of the Pokemon shown, a box's too) at `scale`, decoded once per icon; art. */
static void SIcon(const CtrSummaryUiView *v, int x, int y, int scale)
{
    static unsigned shown;
    static int ok;
    static uint8_t pixels[32 * 32];
    static uint16_t pal[16];

    if (v->loading || !v->icon)
        return;
    if (shown != v->icon + 1u) {
        static unsigned char tiles[PT_MON_ICON_BYTES];  /* the bridge writes both frames; the first is drawn */
        static unsigned short raw[16];

        shown = v->icon + 1u;
        ok = CtrBottomSummary_Icon(tiles, raw);
        for (int py = 0; ok && py < 32; ++py)
            for (int px = 0; px < 32; ++px) {
                unsigned byte = tiles[(py / 8 * 4 + px / 8) * 32 + py % 8 * 4 + px % 8 / 2];
                pixels[py * 32 + px] = (uint8_t)((byte >> (px & 1) * 4) & 15);
            }
        for (int i = 0; ok && i < 16; ++i) {
            unsigned c = raw[i];
            pal[i] = GbaColor(c);
        }
    }
    if (ok) {
        DevArt(DEV_ART_ICON, x, y, 32 * scale, 32 * scale);
        UiImage(pixels, pal, 32, 32, 32, x, y, scale, 1);
    }
}

/* The Pokemon's picture, FireRed's Summary's own (the 64x64 front picture, flipped where FireRed flips it), decoded
 * once per Pokemon shown; 0 when there is none (the caller draws the icon). With HeartGold's pictures in the pack
 * (CtrBottomSummary_Pic80) the 80x80 one, whole, centred on the same spot. Art, where it is drawn. */
static int SPic(const CtrSummaryUiView *v, int x, int y)
{
    static unsigned shownSerial, shownIcon;
    static int shownMon = -1, ok, size = 64;
    static uint8_t pixels[80 * 80];
    static uint16_t pal[16];

    if (v->loading || !v->icon)
        return 0;
    if (shownSerial != v->data.serial || shownMon != v->data.mon || shownIcon != v->icon) {
        static unsigned char tiles[0x800];
        static unsigned short raw[16];
        int flip = 0;

        shownSerial = v->data.serial;
        shownMon = v->data.mon;
        shownIcon = v->icon;
        static unsigned char big[80 * 80 / 2];
        size = 80;
        ok = CtrBottomSummary_Pic80(big, raw, &flip);
        for (int py = 0; ok && py < 80; ++py)
            for (int px = 0; px < 80; ++px)
                pixels[py * 80 + (flip ? 79 - px : px)] = (uint8_t)((big[py * 40 + px / 2] >> (px & 1) * 4) & 15);
        if (!ok) {
            size = 64;
            ok = CtrBottomSummary_Pic(tiles, raw, &flip);
        }
        for (int py = 0; ok && size == 64 && py < 64; ++py)
            for (int px = 0; px < 64; ++px) {
                unsigned byte = tiles[(py / 8 * 8 + px / 8) * 32 + py % 8 * 4 + px % 8 / 2];
                pixels[py * 64 + (flip ? 63 - px : px)] = (uint8_t)((byte >> (px & 1) * 4) & 15);
            }
        for (int i = 0; ok && i < 16; ++i) {
            unsigned c = raw[i];
            pal[i] = GbaColor(c);
        }
    }
    if (ok) {
        DevArt(DEV_ART_PICTURE, x - (size - 64) / 2, y - (size - 64) / 2, size, size);
        UiImage(pixels, pal, size, size, size, x - (size - 64) / 2, y - (size - 64) / 2, 1, 1);
    }
    return ok;
}

/* FireRed's six growth curves, to find which one the shown Pokemon follows from its level, its EXP and the EXP of the
 * next level. */
static long SExpAt(int rate, long n)
{
    long c = n * n * n;

    switch (rate) {
    case 0: return c;
    case 1: return c * 4 / 5;
    case 2: return c * 5 / 4;
    case 3: return c * 6 / 5 - 15 * n * n + 100 * n - 140;
    case 4: return n <= 50 ? c * (100 - n) / 50 : n <= 68 ? c * (150 - n) / 100
                 : n <= 98 ? c * ((1911 - 10 * n) / 3) / 500 : c * (160 - n) / 100;
    default: return n < 15 ? c * ((n + 1) / 3 + 24) / 50 : n < 36 ? c * (n + 14) / 50 : c * (n / 2 + 32) / 50;
    }
}

static long SDigits(const char *s)
{
    long v = 0;

    for (; *s; ++s)
        if (*s >= '0' && *s <= '9')
            v = v * 10 + (*s - '0');
    return v;
}

/* Progress to the next level, 0..100: level 100 (or no EXP left to take) is full. */
static int SExpPercent(const CtrSummaryView *d)
{
    long lv = SDigits(d->level), e = SDigits(d->exp), n = SDigits(d->nextExp), best = -1, bestDiff = 0, base, span;

    if (lv >= 100 || lv < 1)
        return 100;
    for (int r = 0; r < 6; ++r) {
        long diff = SExpAt(r, lv + 1) - (e + n);

        if (diff < 0)
            diff = -diff;
        if (best < 0 || diff < bestDiff) {
            best = r;
            bestDiff = diff;
        }
    }
    base = SExpAt((int)best, lv);
    span = e + n - base;
    if (span <= 0 || e <= base)
        return 0;
    return (int)((e - base) * 100 / span);
}

/* What the page lets one do (the note of MOVES, of the details while a move is moved or with no description, of MEMO). */
static const char *SHint(const CtrSummaryUiView *v)
{
    return v->loading ? "" : v->data.page == SUI_PAGE_MOVES ? CTR_TR("Tap a move for its details.", "Toca un ataque para ver sus datos.", "Attacke antippen f\xC3\xBCr Details.")
         : v->data.page == SUI_PAGE_MOVES_INFO ? (v->data.swapping ? CTR_TR("Tap where to move it.", "Toca d\xC3\xB3nde moverlo.", "Neuen Platz antippen.") : CTR_TR("Tap the selected move to move it.", "Toca el ataque elegido para moverlo.", "Ausgew\xC3\xA4hlte Attacke antippen zum Tauschen."))
         : v->data.egg ? "" : CTR_TR("Up / down: another POK\xC3\xA9MON.", "Arriba / abajo: otro POK\xC3\xA9MON.", "Hoch / Runter: anderes POK\xC3\xA9MON.");
}

/* ---- Pieces ------------------------------------------------------------------------------------------------------------ */

/* Two names the same but for case (FireRed's capitals, or recipe 0251's title case of the species names) and for the
 * spaces they end in (a species name keeps one where its gender sign was). */
static int DevSumSameName(const char *a, const char *b)
{
    char ua[32], ub[32];
    size_t na, nb;

    DevUpper(ua, a, sizeof(ua));
    DevUpper(ub, b, sizeof(ub));
    na = strlen(ua);
    nb = strlen(ub);
    while (na && ua[na - 1] == ' ')
        --na;
    while (nb && ub[nb - 1] == ' ')
        --nb;
    return na == nb && !memcmp(ua, ub, na);
}

/* The Pokemon's name: its species' own (or an egg's) in capitals, a nickname as typed. */
static const char *DevSumName(const CtrSummaryView *d, char *out, size_t size)
{
    return DevMonName(d->nickname, d->egg || DevSumSameName(d->nickname, d->species), out, size);
}

/* The level's digits (the Summary pads them). */
static const char *DevSumLevel(const CtrSummaryView *d)
{
    const char *s = d->level;

    while (*s == ' ')
        ++s;
    return s;
}

/* A chip 12 tall at (x, y): `word` in the small font with 4 px a side, at least `min` wide (the board's type chip is 40),
 * ending by `end` (cut there, and counted, when it would run past it). Returns where the next one starts, 3 px on. */
static int DevSumChip(int x, int y, int end, int min, const char *word, uint16_t bg, uint16_t shadow)
{
    int w = DevSmallTextWidth(word) + 8;

    if (w < min)
        w = min;
    if (x + w > end)
        w = end - x;
    if (w > 0)
        DevChip(x, y, w, 12, word, bg, shadow);
    return x + w + 3;
}

/* The Pokemon has a second type (FireRed gives a single-typed one its type twice). */
static int DevSumTwoTypes(const CtrSummaryView *d)
{
    return d->types[1][0] && strcmp(d->types[0], d->types[1]);
}

/* The Pokemon's type chips (the second when it has one) from x 20 at y, ending by the column's 106. Returns where the
 * last one ends. */
static int DevSumTypes(const CtrSummaryView *d, int y)
{
    char word[24];
    int x = DEV_SUM_COL_X;

    for (int i = 0; i < 1 + DevSumTwoTypes(d); ++i) {
        uint16_t bg = UiTypeColor(d->typeIds[i]);

        DevUpper(word, d->types[i], sizeof(word));
        x = DevSumChip(x, y, DEV_SUM_COL_END, 40, word, bg, DevChipShadow(bg));
    }
    return x - 3;
}

/* A move with no power ("---") is a status move: no category. */
static int DevSumStatusMove(const CtrSummaryView *d, int i)
{
    return !d->power[i][0] || d->power[i][0] == '-';
}

/* The left column of INFO, STATS and MEMO: the name (capitals 52), the gender and the level (capitals 66), the picture
 * in its box (the icon at 2x without one), the types at y 152 and the Pokemon's number small in the secondary ink, 4 px
 * after a single chip (the board's x 64) or under two (or under one it does not fit beside). An egg: its name and its
 * picture. */
static void DevSumColumn(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;
    char text[32];
    int end;

    DevText(DevSumName(d, text, sizeof(text)), DEV_SUM_COL_X, 52, DEV_SUM_COL_END);
    if (!d->egg) {
        int g = d->gender[0] == 'M' ? 0 : d->gender[0] == 'F' ? 1 : 2, x = DEV_SUM_COL_X;

        if (g < 2) {
            DevGender(g, x, 65);
            x += 11;
        }
        snprintf(text, sizeof(text), CTR_TR("Lv%s", "Nv%s", "Lv%s"), DevSumLevel(d));
        DevText(text, x, 66, DEV_SUM_COL_END);
    }
    if (!SPic(v, kDevSumPic.x, kDevSumPic.y))
        SIcon(v, kDevSumPic.x, kDevSumPic.y, 2);
    if (d->egg)
        return;
    end = DevSumTypes(d, 152);
    snprintf(text, sizeof(text), CTR_TR("No.%s", "N\xC2\xBA%s", "Nr.%s"), d->dex);
    if (!DevSumTwoTypes(d) && end + 4 + DevSmallTextWidth(text) <= DEV_SUM_COL_END)
        DevSmallText(text, end + 4, 155, DEV_INK2, DEV_SUM_COL_END);
    else
        DevSmallText(text, DEV_SUM_COL_X, 167, DEV_INK2, DEV_SUM_COL_END);
}

/* The left column of MOVES and the details: the name (capitals 52; in the small font past 86 px), the level small in the
 * secondary ink (capitals 66), a rule; the details' move (FireRed's cursor on one) its type and category chips, its
 * power and accuracy; else the Pokemon's types. */
static void DevSumMoveColumn(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;
    int i = d->cursor;
    const char *name;
    char text[32];

    name = DevSumName(d, text, sizeof(text));
    if (DevTextWidth(name) <= DEV_SUM_COL_END - DEV_SUM_COL_X)
        DevText(name, DEV_SUM_COL_X, 52, DEV_SUM_COL_END);
    else
        DevSmallText(name, DEV_SUM_COL_X, 53, DEV_INK, DEV_SUM_COL_END);
    snprintf(text, sizeof(text), CTR_TR("Lv%s", "Nv%s", "Lv%s"), DevSumLevel(d));
    DevSmallText(text, DEV_SUM_COL_X, 66, DEV_INK2, DEV_SUM_COL_END);
    DevRect(DEV_SUM_COL_X, 78, 84, 1, DEV_RULE);
    if (d->page != SUI_PAGE_MOVES_INFO || i < 0 || i >= 4 || !d->moves[i]) {
        DevSumTypes(d, 84);
        return;
    }
    {
        uint16_t bg = UiTypeColor(d->moveTypeIds[i]);
        int x;

        DevUpper(text, d->moveTypes[i], sizeof(text));
        x = DevSumChip(DEV_SUM_COL_X, 84, DEV_SUM_COL_END, 40, text, bg, DevChipShadow(bg));
        if (!DevSumStatusMove(d, i)) {
            if (d->moveTypeIds[i] < 10)
                DevSumChip(x, 84, DEV_SUM_COL_END, 0, DevCategoryWord(1), DEV_CAT_PHYSICAL, DevChipShadow(DEV_CAT_PHYSICAL));
            else
                DevSumChip(x, 84, DEV_SUM_COL_END, 0, DevCategoryWord(0), DEV_CAT_SPECIAL, DEV_CAT_SPECIAL_SH);
        }
        DevColumnRow(102, CTR_TR("POWER", "POTENCIA", "ST\xC3\x84RKE"), d->power[i][0] ? d->power[i] : "-", DEV_SUM_COL_X,
                     DEV_SUM_COL_VALUE);
        DevColumnRow(118, CTR_TR("ACCURACY", "PRECISI\xC3\x93N", "GENAUIGKEIT"), d->accuracy[i][0] ? d->accuracy[i] : "-",
                     DEV_SUM_COL_X, DEV_SUM_COL_VALUE);
        DevColumnRow(134, CTR_TR("CONTACT", "CONTACTO", "KONTAKT"),
                     (d->contact >> i) & 1 ? CTR_TR("YES", "S\xC3\x8D", "JA") : CTR_TR("NO", "NO", "NEIN"), DEV_SUM_COL_X,
                     DEV_SUM_COL_VALUE);
    }
}

/* Row k of INFO or MEMO, capitals from DEV_SUM_ROW_TOP + k * DEV_SUM_ROW_DY: its label small in the secondary ink at the
 * page's left (capitals 1 px lower), the value ending at its right in the paper's ink, cut with "..." to the room the
 * label leaves and then counted in sDevKeyCut, as a cut key label is (FireRed's longest names, 12 letters, fit beside
 * the ES and DE labels). */
static void DevSumRowText(int k, const char *label, const char *value)
{
    char cut[64];
    int top = DEV_SUM_ROW_TOP + k * DEV_SUM_ROW_DY, room = DEV_SUM_RIGHT - DEV_SUM_LEFT - DevSmallTextWidth(label) - 6;

    DevSmallText(label, DEV_SUM_LEFT, top + 1, DEV_INK2, DEV_SUM_RIGHT);
    if (DevTextWidth(value) > room) {
        DevCutText(value, room, cut, sizeof(cut));
        value = cut;
        ++sDevKeyCut;
    }
    DevText(value, DEV_SUM_RIGHT - DevTextWidth(value), top, DEV_SUM_RIGHT);
}

/* The lines of text from capitals `top` that end by the paper's foot (2 px above it). */
static int DevSumLinesTo(int top)
{
    return (kDevSumColumn.y + kDevSumColumn.h - 2 - 8 - top) / DEV_LINE_H + 1;
}

/* An EXP bar: a 1 px frame, the track, pct % filled (the boards' 4 px bar). */
static void DevSumExpBar(int x, int y, int w, int pct)
{
    int fill = (w - 2) * (pct > 100 ? 100 : pct < 0 ? 0 : pct) / 100;

    DevRect(x, y, w, 4, DEV_HP_FRAME);
    DevRect(x + 1, y + 1, w - 2, 2, DEV_HP_TRACK);
    if (fill > 0)
        DevRect(x + 1, y + 1, fill, 2, DEV_EXP);
}

/* The EXP chip at (x, y) and its bar 3 px after it, `w` wide (to the page's right, 256, when 0), ending by `end`. */
static void DevSumExp(const CtrSummaryView *d, int x, int y, int w, int end)
{
    int chip = DevLabelChip(x, y, CTR_TR("EXP", "EXP", "EP"), DEV_HP_FRAME, DEV_EXP_LABEL);

    x += chip + 3;
    if (!w || x + w > end)
        w = end - x;
    DevSumExpBar(x, y + 2, w, SExpPercent(d));
}

/* INFO: species, OT, ID No., item, nature, ability and HP in rows, the HP bar, the EXP chip and bar; an egg: how soon it
 * hatches. */
static void DevSumInfo(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;
    char text[48];

    if (d->egg) {
        DevWrap(d->hatch, DEV_SUM_LEFT, DEV_SUM_ROW_TOP, DEV_SUM_RIGHT - DEV_SUM_LEFT, DevSumLinesTo(DEV_SUM_ROW_TOP),
                DEV_INK, DEV_INK_SH);
        return;
    }
    DevUpper(text, d->species, sizeof(text));
    DevSumRowText(0, CTR_TR("SPECIES", "ESPECIE", "SPEZIES"), text);
    DevSumRowText(1, CTR_TR("OT", "EO", "OT"), d->ot);
    DevSumRowText(2, CTR_TR("IDNo.", "N.\xC2\xBA ID", "ID-Nr."), d->id);
    DevUpper(text, d->item[0] ? d->item : CTR_TR("NONE", "NINGUNO", "KEINES"), sizeof(text));
    DevSumRowText(3, CTR_TR("ITEM", "OBJETO", "ITEM"), text);
    DevUpper(text, d->nature, sizeof(text));
    DevSumRowText(4, CTR_TR("NATURE", "NATURALEZA", "WESEN"), text);
    DevUpper(text, d->ability, sizeof(text));
    DevSumRowText(5, CTR_TR("ABILITY", "HABILIDAD", "F\xC3\x84HIGKEIT"), text);
    snprintf(text, sizeof(text), "%u/%u", (unsigned)d->hpCur, (unsigned)d->hpMax);
    DevSumRowText(6, CTR_TR("HP", "PS", "KP"), text);
    DevHpBar(DEV_SUM_LEFT, 157, DEV_SUM_RIGHT - DEV_SUM_LEFT, 5, d->hpCur, d->hpMax);
    DevSumExp(d, DEV_SUM_LEFT, 165, 0, DEV_SUM_RIGHT);
}

/* STATS' switch: STATS | IV/EV in a white pill, the half shown in the blue shell's colour with white words. */
static void DevSumSwitch(int iv)
{
    const CtrPokeTouchRect *r = &kDevSumIvSwitch;
    int hw = (r->w - 8) / 2;

    DevRoundRect(r->x, r->y, r->w, r->h, 7, DEV_BUTTON_EDGE);
    DevRoundRect(r->x + 1, r->y + 1, r->w - 2, r->h - 2, 6, DEV_WHITE);
    for (int half = 0; half < 2; ++half) {
        const char *word = half ? "IV/EV" : CTR_TR("STATS", "STATS", "STATUS");
        int hx = r->x + 3 + half * (hw + 2), tw = DevSmallTextWidth(word), shown = half == iv;

        if (shown)
            DevRoundRect(hx, r->y + 1, hw, r->h - 2, 5, DEV_SHELL_BLUE);
        if (tw > hw)
            ++sDevKeyCut;
        DevSmallText(word, tw < hw ? hx + (hw - tw) / 2 : hx, r->y + 3, shown ? DEV_WHITE : DEV_INK, hx + hw);
    }
}

/* STATS: the nature's name (capitals 52) and the switch; the six stats (HP to SPEED) in rows from 69, each label 48 px
 * (more when a word and its mark need it), a bar of the stat over the largest of the six (64 px at most, ending by
 * 256 - 28), the value at 256; the nature's marks; the EXP line (capitals 158): the chip, a 74 px bar ending before
 * "NEXT n" at the right. The IV / EV view: the IVs' bars (of 31, ending by 256 - 46), each IV (at 228) and EV (at 256),
 * Hidden Power's type and power on the last line. */
static void DevSumStats(const CtrSummaryUiView *v)
{
    static const char *const kLabel[6] = {
        CTR_TR("HP", "PS", "KP"), CTR_TR("ATTACK", "ATAQUE", "ANGRIFF"), CTR_TR("DEFENSE", "DEFENSA", "VERT."),
        CTR_TR("SP. ATK", "AT. ESP.", "SP. ANG."), CTR_TR("SP. DEF", "DEF. ESP.", "SP. VER."), CTR_TR("SPEED", "VELOCIDAD", "INIT."),
    };
    const CtrSummaryView *d = &v->data;
    unsigned value[6], most = 1;
    int up = CtrNature_Up(d->natureId), down = CtrNature_Down(d->natureId), col = 48, barX, barW;
    char text[24];

    DevUpper(text, d->nature, sizeof(text));
    DevSmallText(text, DEV_SUM_LEFT, 52, DEV_INK2, kDevSumIvSwitch.x - 4);
    DevSumSwitch(v->ivView);
    value[0] = d->hpMax;
    for (int i = 1; i < 6; ++i)
        value[i] = SNumber(d->stats[i - 1], 0);
    for (int i = 0; i < 6; ++i) {
        int need = DevSmallTextWidth(kLabel[i]) + (i ? 3 + 9 : 0) + 2;

        if (need > col)
            col = need;
        if (value[i] > most)
            most = value[i];
    }
    barX = DEV_SUM_LEFT + col;
    barW = DEV_SUM_RIGHT - (v->ivView ? 46 : 28) - barX;
    if (barW > 64)
        barW = 64;
    for (int i = 0; i < 6; ++i) {
        int y = DEV_SUM_STAT_Y + DEV_SUM_STAT_DY * i, mark = !i ? 0 : i - 1 == up ? 1 : i - 1 == down ? 2 : 0;
        int labelTop = y + (DEV_SUM_STAT_H - 7) / 2, valueTop = y + (DEV_SUM_STAT_H - UI_TEXT_H) / 2;
        int barY = y + (DEV_SUM_STAT_H - 4) / 2;
        uint16_t ink = mark == 1 ? DEV_STAT_UP : mark == 2 ? DEV_BLUE_INK : DEV_INK2;
        unsigned of = v->ivView ? 31 : most, n = v->ivView ? d->iv[i] : value[i];
        int fill = (int)((unsigned long)(n > of ? of : n) * (unsigned)barW / of);

        DevSmallText(kLabel[i], DEV_SUM_LEFT, labelTop, ink, barX);
        if (mark)                                       /* the triangle (9x5) 3 px after the word, centred on the row */
            DevTriangle(DEV_SUM_LEFT + DevSmallTextWidth(kLabel[i]) + 3 + 4, y + DEV_SUM_STAT_H / 2, mark == 1 ? 0 : 1, ink);
        if (n && fill < 1)
            fill = 1;
        DevRect(barX, barY, barW, 4, DEV_SUM_BAR_TRACK);
        DevRect(barX, barY, fill, 4, DEV_SUM_BAR);
        snprintf(text, sizeof(text), "%u", n);
        if (v->ivView) {                                /* the IV, and the EV at the right in the secondary ink */
            DevText(text, DEV_SUM_RIGHT - 28 - DevTextWidth(text), valueTop, DEV_SUM_RIGHT - 28);
            snprintf(text, sizeof(text), "%u", (unsigned)d->ev[i]);
            DevTextRight(text, DEV_SUM_RIGHT, valueTop, DEV_INK2);
        } else
            DevText(text, DEV_SUM_RIGHT - DevTextWidth(text), valueTop, DEV_SUM_RIGHT);
    }
    if (v->ivView) {                                    /* Hidden Power: its name, its type's chip and its power */
        uint16_t bg = UiTypeColor(d->hpType);
        char type[24];
        int x;

        snprintf(text, sizeof(text), "%u", (unsigned)d->hpPower);
        x = DEV_SUM_RIGHT - DevSmallTextWidth(text);
        DevSmallText(text, x, 158, DEV_INK, DEV_SUM_RIGHT);
        DevUpper(type, d->hpTypeName, sizeof(type));
        x -= 4 + DevSmallTextWidth(type) + 8;
        DevChip(x, 156, 0, 11, type, bg, DevChipShadow(bg));
        DevSmallText(CTR_TR("HIDDEN POWER", "PODER OCULTO", "KRAFTRESERVE"), DEV_SUM_LEFT, 158, DEV_INK2, x - 4);
        return;
    }
    {                                                   /* the EXP line: the chip, its bar, "NEXT n" at the right */
        const char *next = SDigits(d->level) >= 100 ? "-" : d->nextExp;
        int x;

        while (*next == ' ')
            ++next;
        snprintf(text, sizeof(text), "%s %s", CTR_TR("NEXT", "FALTAN", "NOCH"), next);
        x = DEV_SUM_RIGHT - DevSmallTextWidth(text);
        DevSmallText(text, x, 158, DEV_INK2, DEV_SUM_RIGHT);
        DevSumExp(d, DEV_SUM_LEFT, 158, 74, x - 3);     /* a long count shortens the bar, never meets it */
    }
}

/* Move row i (4: CANCEL), inverted when `chosen`; `framed`, the move being moved (a swap: a 1 px frame round the row,
 * over its band). A move: its type's band down the row's left (4 px, on the inverted row too), its name from 8 px in, its
 * PP ending 4 px from the right in FireRed's PP colours, both capitals 8 px down; a slot without one, a dash. */
static void DevSumMoveRow(const CtrSummaryView *d, int i, int chosen, int framed)
{
    CtrPokeTouchRect r = DevSumRow(i);
    uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;
    int top = r.y + 8, right = r.x + r.w - 4, x = right;
    char text[24];

    DevRow(&r, chosen);
    if (i == 4) {
        uint16_t c = chosen ? DEV_PAPER : DEV_INK2;

        DevTextInk(DevCancelWord(), r.x + 8, top, right, c, c);
    } else if (!d->moves[i])
        DevTextInk("-", r.x + 8, top, right, DEV_INK_DIM, DEV_INK_DIM);
    else {
        unsigned pp = SNumber(d->pp[i], 0), max = SNumber(d->pp[i], 1);
        uint16_t ppInk = chosen ? ink : DevPpInk(pp, max, DEV_INK);

        DevRect(r.x, r.y, 4, r.h, UiTypeColor(d->moveTypeIds[i]));
        snprintf(text, sizeof(text), "%u/%u", pp, max);
        x = right - DevTextWidth(text);
        DevTextInk(text, x, top, right, ppInk, chosen ? ppInk : DEV_INK_SH);
        DevUpper(text, d->moveNames[i], sizeof(text));
        DevTextInk(text, r.x + 8, top, x - 4, ink, shadow);
    }
    if (framed && !chosen) {
        DevRect(r.x, r.y, r.w, 1, DEV_BEZEL);
        DevRect(r.x, r.y + r.h - 1, r.w, 1, DEV_BEZEL);
        DevRect(r.x, r.y, 1, r.h, DEV_BEZEL);
        DevRect(r.x + r.w - 1, r.y, 1, r.h, DEV_BEZEL);
    }
}

/* MOVES and the details: the four rows, and the details' CANCEL. The chosen row is the one under a held touch, else
 * FireRed's cursor in the details (none on MOVES). */
static void DevSumMoves(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;
    int details = d->page == SUI_PAGE_MOVES_INFO, cursor = -1, held = DevSumPressDrawn(v);
    int pressed = held >= SUI_HIT_MOVE && held <= SUI_HIT_CANCEL ? held : -1;

    if (details)
        cursor = d->cursor >= 0 && d->cursor < 4 ? SUI_HIT_MOVE + d->cursor : SUI_HIT_CANCEL;
    for (int i = 0; i < 4; ++i)
        DevSumMoveRow(d, i, d->moves[i] && DevChosen(SUI_HIT_MOVE + i, cursor, pressed),
                      details && d->swapping && d->swapCursor == i);
    if (DevSumCancelOn(v))
        DevSumMoveRow(d, 4, DevChosen(SUI_HIT_CANCEL, cursor, pressed), 0);
}

/* MEMO: the nature and the ribbons in rows, then FireRed's memo from capitals 85 to the paper's foot; an egg: its memo
 * and how soon it hatches. */
static void DevSumMemo(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;
    int width = DEV_SUM_RIGHT - DEV_SUM_LEFT, top = DEV_SUM_ROW_TOP, lines;
    char text[24];

    if (!d->egg) {
        DevUpper(text, d->nature, sizeof(text));
        DevSumRowText(0, CTR_TR("NATURE", "NATURALEZA", "WESEN"), text);
        snprintf(text, sizeof(text), "%u", (unsigned)d->ribbons);
        DevSumRowText(1, CTR_TR("RIBBONS", "CINTAS", "B\xC3\x84NDER"), text);
        DevWrap(d->memo, DEV_SUM_LEFT, 85, width, DevSumLinesTo(85), DEV_INK, DEV_INK_SH);
        return;
    }
    lines = DevWrap(d->memo, DEV_SUM_LEFT, top, width, 4, DEV_INK, DEV_INK_SH);
    top += DEV_LINE_H * lines + (lines ? 6 : 0);
    DevWrap(d->hatch, DEV_SUM_LEFT, top, width, DevSumLinesTo(top), DEV_INK, DEV_INK_SH);
}

/* The screen (the grid's): the left column and the page; empty while a Pokemon loads. */
static void DevSumScreen(const CtrSummaryUiView *v)
{
    DevScreen(&kDevGridScreen);
    DevRect(kDevSumColumn.x, kDevSumColumn.y, kDevSumColumn.w, kDevSumColumn.h, DEV_PAPER2);
    if (v->loading)
        return;
    switch (CtrSummaryUi_Tab(v)) {
    case SUI_TAB_INFO:
        DevSumColumn(v);
        DevSumInfo(v);
        break;
    case SUI_TAB_STATS:
        DevSumColumn(v);
        DevSumStats(v);
        break;
    case SUI_TAB_MOVES:
        DevSumMoveColumn(v);
        DevSumMoves(v);
        break;
    default:
        DevSumColumn(v);
        DevSumMemo(v);
        break;
    }
}

/* A battle's Summary: FireRed moves no move in a battle, so the details give no hint about moving one. */
static int DevSumBattle(const CtrSummaryUiView *v)
{
    return v->data.origin == CTR_SUMMARY_BATTLE_PARTY;
}

/* FireRed's cursor in the details is on a move (not on CANCEL). */
static int DevSumOnMove(const CtrSummaryView *d)
{
    return d->cursor >= 0 && d->cursor < 4 && d->moves[d->cursor];
}

/* The note, on the grid's (paper 14,196,216,30; words from x 20 to 224): on INFO and STATS the ability's name as the
 * note's title and what it does in one small line under it (capitals 212); in the details the chosen move's description
 * in two small lines from the paper's top (while a move is moved, or with none, the hint; nothing in a battle), nothing
 * on CANCEL; else what the page lets one do, read on the paper (DevReadText). */
static void DevSumNote(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;
    int tab = CtrSummaryUi_Tab(v);
    CtrPokeTouchRect paper = DevScreen(&kDevGridNote);
    int x = paper.x + 6, right = paper.x + paper.w - 6;

    if (v->loading)
        return;
    if ((tab == SUI_TAB_INFO || tab == SUI_TAB_STATS) && !d->egg) {
        char name[24];

        DevUpper(name, d->ability, sizeof(name));
        DevNoteTitle(&paper, x, name);
        DevSmallFit(d->abilityDesc, x, x, paper.y + 16, right, 1, DEV_SMALL_LINE, DEV_INK);
        return;
    }
    if (d->page == SUI_PAGE_MOVES_INFO) {
        const char *text = !DevSumOnMove(d) ? "" : d->swapping || !d->moveDesc[0] ? (DevSumBattle(v) ? "" : SHint(v))
                         : d->moveDesc;

        DevSmallFit(text, x, x, paper.y + 2, right, 2, DEV_SMALL_LINE, DEV_INK);
        return;
    }
    DevReadText(SHint(v), &paper, x, right, paper.y + 2, DEV_SMALL_LINE);
}

/* The tabs (the shown one sunk, or the one under a held touch; beige when they cannot be used), ▲▼ (cream keys, grey and
 * not taken in the details) and BACK, the dark key in the exit corner. */
static void DevSumKeys(const CtrSummaryUiView *v)
{
    static const char *const kTab[SUI_TABS] = {
        CTR_TR("INFO", "INFO", "INFO"), CTR_TR("STATS", "STATS", "STATUS"), CTR_TR("MOVES", "ATAQUES", "ATTACKEN"),
        CTR_TR("MEMO", "NOTAS", "MEMO"),
    };
    int held = DevSumPressDrawn(v), arrows = DevSumArrowsOn(v);
    int tabPressed = held >= SUI_HIT_TAB && held < SUI_HIT_TAB + SUI_TABS ? held : -1;
    int shown = v->loading ? -1 : SUI_HIT_TAB + CtrSummaryUi_Tab(v), sunk, faceY;

    for (int i = 0; i < SUI_TABS; ++i) {
        CtrPokeTouchRect r = DevGridTab(SUI_TABS, i);
        int look = DevSumTabOn(v, i) ? DEV_LOOK_KEY : DEV_LOOK_FAINT;

        sunk = look == DEV_LOOK_KEY && DevChosen(SUI_HIT_TAB + i, shown, tabPressed);
        faceY = DevKey(&r, look, sunk, 5);
        DevKeyWord(&r, faceY, look, sunk, kTab[i]);
    }
    DevArrowKeys(&kDevGridUp, &kDevGridDown, arrows, arrows, held == SUI_HIT_PREV, held == SUI_HIT_NEXT, 0);
    sunk = held == SUI_HIT_BACK;
    faceY = DevKey(&kDevGridExit, DEV_LOOK_DARK, sunk, 6);
    DevKeyWord(&kDevGridExit, faceY, DEV_LOOK_DARK, sunk, DevBackWord());
}

void CtrSummaryUi_Draw(uint16_t *canvas, const CtrSummaryUiView *v)
{
    DevBegin(canvas, v->shell);
    DevShell();
    DevSumKeys(v);
    DevSumScreen(v);
    DevSumNote(v);
}

/* What v does not draw, from v into `still` (a copy of the view drawn last), so that only a change of what it draws
 * redraws (the plan's rule): where the Summary was opened from (but whether it is a battle's), FireRed's readiness; a
 * touch that draws nothing; the move cursor, the swap and the description outside the details, the swap's row while not
 * swapping and the description while swapping or on CANCEL; the IV / EV view outside STATS. */
static void DevSumUndrawn(CtrSummaryUiView *still, const CtrSummaryUiView *v)
{
    CtrSummaryView *s = &still->data;
    const CtrSummaryView *d = &v->data;

    if (DevSumBattle(still) == DevSumBattle(v))
        s->origin = d->origin;
    s->ready = d->ready;
    if (d->page != SUI_PAGE_MOVES_INFO)
        s->contact = d->contact;
    if (DevSumPressDrawn(still) == DevSumPressDrawn(v))
        still->pressed = v->pressed;
    if (d->page != SUI_PAGE_MOVES_INFO) {
        s->cursor = d->cursor;
        s->swapping = d->swapping;
        s->swapCursor = d->swapCursor;
        memcpy(s->moveDesc, d->moveDesc, sizeof(s->moveDesc));
    } else {
        if (!d->swapping)
            s->swapCursor = d->swapCursor;
        if (d->swapping || !DevSumOnMove(d))                        /* the note is the hint, or nothing */
            memcpy(s->moveDesc, d->moveDesc, sizeof(s->moveDesc));
    }
    if (d->page != SUI_PAGE_SKILLS)
        still->ivView = v->ivView;
}

/* Any change of what is drawn: the screen whole. */
void CtrSummaryUi_Update(uint16_t *canvas, const CtrSummaryUiView *old, const CtrSummaryUiView *v, CtrDirtyList *dirty)
{
    CtrSummaryUiView still = *old;

    CtrDirty_Clear(dirty);
    DevSumUndrawn(&still, v);
    if (memcmp(&still, v, sizeof(still))) {
        CtrSummaryUi_Draw(canvas, v);
        CtrDirty_Add(dirty, 0, 0, W, H);
    }
}
