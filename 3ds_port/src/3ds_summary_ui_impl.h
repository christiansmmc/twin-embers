/* The Pokemon Summary on the lower screen (3ds_poketouch.h CtrSummaryUi_*), included at the end of 3ds_poketouch.c after
 * the battle screen, whose party page helpers (the ailment tag, the gender signs) it shares: one Summary for the field's
 * Party, the battle's and a PC box's, as a page in the kit's look where 3ds_lower_layout.h puts it (phase 2, the owner's
 * "3", FireRed's own arrangement). The page's title with its dots and the arrows that turn the pages; the Pokemon on the
 * left (name and gender sign, Lv and its ailment, its picture, its types); the page on the right; under them the
 * previous and next Pokemon, a band with the ability's or the move's description and BACK. It only shows what FireRed's
 * Summary reports; every page, Pokemon and move cursor is FireRed's own (3ds_bottom.c walks them with FireRed's keys). */

int CtrSummaryUi_Tab(const CtrSummaryUiView *v)
{
    int page = v->data.page;

    return page == SUI_PAGE_SKILLS ? SUI_TAB_STATS : page >= SUI_PAGE_MOVES ? SUI_TAB_MOVES
         : v->trainer ? SUI_TAB_TRAINER : SUI_TAB_INFO;
}

/* What can be used now: an egg has only its first page (INFO, TRAINER); the previous and next Pokemon are FireRed's
 * UP and DOWN, which move the cursor instead in the move details. */
static int STabEnabled(const CtrSummaryUiView *v, int tab)
{
    return !v->loading && (!v->data.egg || tab == SUI_TAB_INFO || tab == SUI_TAB_TRAINER);
}

int CtrSummaryUi_PageStep(const CtrSummaryUiView *v, int dir)
{
    if (v->loading || (dir != -1 && dir != 1))
        return -1;
    for (int tab = CtrSummaryUi_Tab(v) + dir; tab >= 0 && tab < SUI_TABS; tab += dir)
        if (STabEnabled(v, tab))
            return tab;
    return -1;
}

static int SSwitchEnabled(const CtrSummaryUiView *v)
{
    return !v->loading && v->data.page != SUI_PAGE_MOVES_INFO;
}

int CtrSummaryUi_Hit(const CtrSummaryUiView *v, int x, int y)
{
    const CtrSummaryView *d = &v->data;

    if (UiIn(&kSumBack, x, y))
        return SUI_HIT_BACK;
    if (v->loading)
        return -1;
    if (SSwitchEnabled(v) && UiIn(&kSumMonPrev, x, y))
        return SUI_HIT_PREV;
    if (SSwitchEnabled(v) && UiIn(&kSumMonNext, x, y))
        return SUI_HIT_NEXT;
    if (CtrSummaryUi_PageStep(v, -1) >= 0 && UiIn(&kSumPagePrev, x, y))
        return SUI_HIT_PAGE_PREV;
    if (CtrSummaryUi_PageStep(v, 1) >= 0 && UiIn(&kSumPageNext, x, y))
        return SUI_HIT_PAGE_NEXT;
    if (d->page == SUI_PAGE_SKILLS && !d->egg && UiIn(&kSumPage, x, y))
        return SUI_HIT_IVTOGGLE;
    if (d->page == SUI_PAGE_MOVES)
        for (int i = 0; i < 4; ++i) {
            CtrPokeTouchRect r = SumMoveCard(i);

            if (d->moves[i] && UiIn(&r, x, y))
                return SUI_HIT_MOVE + i;
        }
    if (d->page == SUI_PAGE_MOVES_INFO)
        for (int i = 0; i < 5; ++i) {
            CtrPokeTouchRect r = SumMoveRow(i);

            if (UiIn(&r, x, y) && (i < 4 ? d->moves[i] != 0 : !d->swapping))
                return i < 4 ? SUI_HIT_MOVE + i : SUI_HIT_CANCEL;
        }
    return -1;
}

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

/* An ailment's tag colour by the Summary's own code (0: none). */
static uint16_t SStatusColor(const char *status)
{
    static const struct { const char *code; uint16_t color; } sColors[] = {
        {CTR_TR("PSN", "ENV", "Gft"), UI_RGB(170, 80, 190)}, {CTR_TR("PRZ", "PAR", "Par"), UI_RGB(214, 170, 30)}, {CTR_TR("SLP", "DOR", "Slf"), UI_RGB(120, 128, 150)},
        {CTR_TR("FRZ", "CON", "Efr"), UI_RGB(80, 170, 220)}, {CTR_TR("BRN", "QUE", "Brn"), UI_RGB(226, 96, 48)}, {CTR_TR("PKR", "PKR", "PKR"), UI_RGB(200, 90, 160)}, {CTR_TR("FNT", "DEB", "Bes"), UI_RGB(90, 80, 90)},
    };

    for (unsigned i = 0; i < sizeof(sColors) / sizeof(sColors[0]); ++i)
        if (!strcmp(status, sColors[i].code))
            return sColors[i].color;
    return 0;
}

/* The Pokemon's icon (FireRed's 32x32 icon of the Pokemon shown, a box's too) at `scale`, decoded once per icon. */
static void SIcon(const CtrSummaryUiView *v, int x, int y, int scale)
{
    static unsigned shown;
    static int ok;
    static uint8_t pixels[32 * 32];
    static uint16_t pal[16];

    if (v->loading || !v->icon)
        return;
    if (shown != v->icon + 1u) {
        static unsigned char tiles[512];
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
    if (ok)
        UiImage(pixels, pal, 32, 32, 32, x, y, scale, 1);
}

/* The Pokemon's picture, FireRed's Summary's own (the 64x64 front picture, flipped where FireRed flips it), decoded
 * once per Pokemon shown; 0 when there is none (the caller draws the icon). With HeartGold's pictures in the pack
 * (CtrBottomSummary_Pic80) the 80x80 one, whole, centred on the same spot. */
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
        static unsigned char big[80 * 80 / 2];
        int flip = 0;

        shownSerial = v->data.serial;
        shownMon = v->data.mon;
        shownIcon = v->icon;
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
    if (ok)
        UiImage(pixels, pal, size, size, size, x - (size - 64) / 2, y - (size - 64) / 2, 1, 1);
    return ok;
}

/* FireRed's PP colours on the page's white: the ink, then yellow at half, orange at a quarter, red at none. */
static uint16_t SPpColor(unsigned pp, unsigned max)
{
    return pp == 0 ? UI_HP_LOW : pp * 4 <= max ? UI_RGB(222, 96, 24) : pp * 2 <= max ? UI_RGB(196, 128, 0) : KitInk(2);
}

/* A label (toned) on the left and its value on the right of a line from x to right. */
static void SLine(int x, int right, int lineTop, const char *label, const char *value)
{
    KitTextOn(label, x, lineTop, right, 0);
    KitText(value, right - UiTextWidth(value, 1), lineTop, right);
}

/* A framed cell with a triangle: the light grey inside while held, the triangle toned while it cannot be used. */
static void SArrowCell(const CtrSummaryUiView *v, const CtrPokeTouchRect *r, int dir, int on, int pressed)
{
    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    if (pressed)
        Fill(r->x + 8, r->y + 8, r->w - 16, r->h - 16, KitInk(3));
    KitTriangle(r->x + r->w / 2, r->y + r->h / 2, dir, Tone(KitInk(2), on));
}

/* The page's title, its dots (the one shown dark) and the arrows that turn the pages. */
static void STitle(const CtrSummaryUiView *v)
{
    static const char *const kTitle[SUI_TABS] = {
        CTR_TR("POK\xC3\xA9MON INFO", "DATOS POK\xC3\xA9MON", "Pok\xC3\xA9mon-Info"),
        CTR_TR("POK\xC3\xA9MON SKILLS", "ESTAD\xC3\x8DSTICAS", "Statuswerte"),
        CTR_TR("KNOWN MOVES", "ATAQUES", "Attacken"), CTR_TR("TRAINER MEMO", "NOTAS ENTREN.", "Trainer-Memo")};
    const CtrPokeTouchRect *r = &kSumTitle;
    int tab = CtrSummaryUi_Tab(v), right = r->x + r->w - 8;

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    if (!v->loading) {
        KitText(kTitle[tab], r->x + 8, r->y + 8, right - 4 * 9 - 4);
        for (int i = 0; i < SUI_TABS; ++i)
            Fill(right - 4 * 9 + 3 + 9 * i, r->y + 12, 6, 6, i == tab ? KitInk(2) : KitInk(3));
    }
    SArrowCell(v, &kSumPagePrev, 2, CtrSummaryUi_PageStep(v, -1) >= 0, v->pressed == SUI_HIT_PAGE_PREV);
    SArrowCell(v, &kSumPageNext, 3, CtrSummaryUi_PageStep(v, 1) >= 0, v->pressed == SUI_HIT_PAGE_NEXT);
}

/* The Pokemon's column: its name and gender sign, Lv and its ailment's tag, its picture (or its icon at 2x without
 * one), its types. */
static void SMon(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;
    const CtrPokeTouchRect *r = &kSumMon;
    int x = r->x + 8, right = r->x + r->w - 8, top = r->y + 8;

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    if (v->loading)
        return;
    if (!SPic(v, r->x + 22, top + 30))
        SIcon(v, r->x + 22, top + 30, 2);
    {
        const char *sign = d->egg ? NULL : d->gender[0] == 'M' ? PPAGE_MALE : d->gender[0] == 'F' ? PPAGE_FEMALE : NULL;
        int limit = right - (sign ? UiTextWidth(sign, 1) + 2 : 0), end = x + UiTextWidth(d->nickname, 1);

        KitText(d->nickname, x, top, limit);
        if (sign)
            UiText(sign, (end < limit ? end : limit) + 2, top + 3, d->gender[0] == 'M' ? kPPageMale : kPPageFemale,
                   KitInk(3), 1, right);
    }
    if (d->egg)
        return;
    {
        const char *level = d->level;
        char text[12];

        while (*level == ' ')       /* the Summary's level is right-aligned */
            ++level;
        snprintf(text, sizeof(text), "Lv%s", level);
        KitText(text, x, top + 14, right);
        if (SStatusColor(d->status))
            PPageTag(right - UiTextWidth(d->status, 1) - 8, top + 14, d->status, SStatusColor(d->status));
    }
    {
        int second = d->types[1][0] && strcmp(d->types[0], d->types[1]) != 0;
        CtrPokeTouchRect t0 = {(int16_t)x, (int16_t)(r->y + r->h - 8 - 13), (int16_t)(second ? 44 : 92), 13};
        CtrPokeTouchRect t1 = {(int16_t)(x + 48), t0.y, 44, 13};

        KitTypeLabel(&t0, d->typeIds[0], d->types[0]);
        if (second)
            KitTypeLabel(&t1, d->typeIds[1], d->types[1]);
    }
}

/* INFO: the species and its number, the item, the nature, the ability, the HP and its bar; an Egg's hatch text. */
static void SInfo(const CtrSummaryUiView *v, int x, int right, int top)
{
    const CtrSummaryView *d = &v->data;
    char text[32];

    if (d->egg) {
        KitWrap(d->hatch, x, top, right - x, 8);
        return;
    }
    snprintf(text, sizeof(text), "No.%s %s", d->dex, d->species);
    SLine(x, right, top, CTR_TR("SPECIES", "ESPECIE", "Spezies"), text);
    SLine(x, right, top + 18, CTR_TR("ITEM", "OBJETO", "Item"), d->item[0] ? d->item : CTR_TR("NONE", "NINGUNO", "Keines"));
    SLine(x, right, top + 36, CTR_TR("NATURE", "NATURALEZA", "Wesen"), d->nature);
    SLine(x, right, top + 54, CTR_TR("ABILITY", "HABILIDAD", "F\xC3\xA4higkeit"), d->ability);
    snprintf(text, sizeof(text), "%u/%u", d->hpCur, d->hpMax);
    SLine(x, right, top + 72, CTR_TR("HP", "PS", "KP"), text);
    KitHpBar(x, top + 90, right - x, d->hpCur, d->hpMax);
}

/* STATS: the HP with its bar and the five stats, then the experience and what the next level needs; or, switched, each
 * stat's IV and EV and Hidden Power's type and power. */
static void SStats(const CtrSummaryUiView *v, int x, int right, int top)
{
    static const char *const kLabel[5] = {CTR_TR("ATTACK", "ATAQUE", "Angriff"), CTR_TR("DEFENSE", "DEFENSA", "Vert."),
                                          CTR_TR("SP. ATK", "AT. ESP.", "Sp. Ang."), CTR_TR("SP. DEF", "DEF. ESP.", "Sp. Ver."),
                                          CTR_TR("SPEED", "VELOCIDAD", "Init.")};
    static const char *const kShort[6] = {CTR_TR("HP", "PS", "KP"), CTR_TR("ATTACK", "ATAQUE", "Angriff"),
                                          CTR_TR("DEFENSE", "DEFENSA", "Vert."), CTR_TR("SP. ATK", "AT. ESP.", "Sp. Ang."),
                                          CTR_TR("SP. DEF", "DEF. ESP.", "Sp. Ver."), CTR_TR("SPEED", "VELOCIDAD", "Init.")};
    const CtrSummaryView *d = &v->data;
    char text[16];

    if (v->ivView) {
        int ivRight = right - 40;

        KitTextOn("IV", ivRight - UiTextWidth("IV", 1), top, ivRight, 0);
        KitTextOn("EV", right - UiTextWidth("EV", 1), top, right, 0);
        for (int i = 0; i < 6; ++i) {
            int line = top + 14 + 14 * i;

            KitText(kShort[i], x, line, ivRight - 24);
            snprintf(text, sizeof(text), "%u", d->iv[i]);
            KitText(text, ivRight - UiTextWidth(text, 1), line, ivRight);
            snprintf(text, sizeof(text), "%u", d->ev[i]);
            KitText(text, right - UiTextWidth(text, 1), line, right);
        }
        KitTextOn(CTR_TR("HIDDEN POWER", "PODER OCULTO", "Kraftreserve"), x, top + 100, right - 70, 0);
        {
            CtrPokeTouchRect label = {(int16_t)(right - 66), (int16_t)(top + 100), 40, 13};

            KitTypeLabel(&label, d->hpType, d->hpTypeName);
        }
        snprintf(text, sizeof(text), "%u", d->hpPower);
        KitText(text, right - UiTextWidth(text, 1), top + 100, right);
        return;
    }
    snprintf(text, sizeof(text), "%u/%u", d->hpCur, d->hpMax);
    SLine(x, right, top, CTR_TR("HP", "PS", "KP"), text);
    KitHpBar(x, top + 15, right - x, d->hpCur, d->hpMax);
    for (int i = 0; i < 5; ++i)
        SLine(x, right, top + 20 + 14 * i, kLabel[i], d->stats[i]);
    SLine(x, right, top + 90, CTR_TR("EXP. POINTS", "PUNTOS EXP.", "Exp. Punkte"), d->exp);
    SLine(x, right, top + 104, CTR_TR("TO NEXT LV.", "PARA SUBIR", "N\xC3\xA4" "chst. Lv."), d->nextExp);
}

/* MOVES: the four as two-line rows (the name; its type and PP), each opening its details. */
static void SMoveCards(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;

    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect r = SumMoveCard(i);

        if (v->pressed == SUI_HIT_MOVE + i)
            Fill(r.x, r.y, r.w, r.h, KitInk(3));
        if (!d->moves[i]) {
            KitTextOn("-", r.x, r.y, r.x + r.w, 0);
            continue;
        }
        KitText(d->moveNames[i], r.x, r.y, r.x + r.w);
        {
            CtrPokeTouchRect label = {r.x, (int16_t)(r.y + 15), 44, 13};
            unsigned pp = SNumber(d->pp[i], 0), max = SNumber(d->pp[i], 1);
            int right = r.x + r.w;

            KitTypeLabel(&label, d->moveTypeIds[i], d->moveTypes[i]);
            UiText(d->pp[i], right - UiTextWidth(d->pp[i], 1), r.y + 15 + 3, SPpColor(pp, max), KitInk(3), 1, right);
            KitTextOn("PP", right - UiTextWidth(d->pp[i], 1) - 4 - UiTextWidth("PP", 1), r.y + 15, right, 0);
        }
    }
}

/* The details: the four moves and CANCEL as rows (FireRed's arrow on its cursor or the row held; the move being moved
 * on the light grey; CANCEL gone while one is), and under them the chosen move's type, power and accuracy. */
static void SMoveRows(const CtrSummaryUiView *v, int x, int right, int top)
{
    const CtrSummaryView *d = &v->data;
    int held = v->pressed >= SUI_HIT_MOVE && v->pressed <= SUI_HIT_CANCEL ? v->pressed - SUI_HIT_MOVE : -1;
    int arrow = held >= 0 ? held : d->cursor, sel = d->cursor;

    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = SumMoveRow(i);

        if (i == 4 && d->swapping)
            break;
        if (d->swapping && d->swapCursor == i && i < 4)
            Fill(r.x, r.y, r.w, r.h, KitInk(3));
        if (i == 4) {
            KitRow(&r, CTR_TR("CANCEL", "CANCELAR", "Zur\xC3\xBC" "ck"), arrow == 4, 1);
            continue;
        }
        if (!(d->swapping && d->swapCursor == i))
            Fill(r.x, r.y, r.w, r.h, KitInk(1));
        if (arrow == i)
            KitText(KIT_ARROW, r.x, r.y + 1, r.x + KIT_ARROW_W);
        if (!d->moves[i]) {
            KitTextOn("-", r.x + KIT_ARROW_W, r.y + 1, r.x + r.w, 0);
            continue;
        }
        KitText(d->moveNames[i], r.x + KIT_ARROW_W, r.y + 1, r.x + r.w - 40);
        UiText(d->pp[i], r.x + r.w - UiTextWidth(d->pp[i], 1), r.y + 1 + 3,
               SPpColor(SNumber(d->pp[i], 0), SNumber(d->pp[i], 1)), KitInk(3), 1, r.x + r.w);
    }
    {
        CtrPokeTouchRect last = SumMoveRow(4);
        int y = last.y + last.h + 2;

        Fill(x, y, right - x, 1, KitInk(3));
        if (sel >= 0 && sel < 4 && d->moves[sel]) {
            CtrPokeTouchRect label = {(int16_t)x, (int16_t)(y + 4), 44, 13};

            KitTypeLabel(&label, d->moveTypeIds[sel], d->moveTypes[sel]);
            SLine(x, right, y + 18, CTR_TR("POWER", "POTENCIA", "St\xC3\xA4rke"), d->power[sel]);
            SLine(x, right, y + 32, CTR_TR("ACCURACY", "PRECISI\xC3\x93N", "Genauigkeit"), d->accuracy[sel]);
        }
    }
    (void)top;
}

/* TRAINER: the original trainer, the ID, the nature and the ribbons, then the trainer memo. */
static void STrainer(const CtrSummaryUiView *v, int x, int right, int top)
{
    const CtrSummaryView *d = &v->data;
    char ribbons[8];
    int line = top;

    SLine(x, right, line, "OT", d->ot);
    SLine(x, right, line += 16, CTR_TR("ID No.", "N.\xC2\xBA ID", "ID-Nr."), d->id);
    if (!d->egg) {
        snprintf(ribbons, sizeof(ribbons), "%u", d->ribbons);
        SLine(x, right, line += 16, CTR_TR("NATURE", "NATURALEZA", "Wesen"), d->nature);
        SLine(x, right, line += 16, CTR_TR("RIBBONS", "CINTAS", "B\xC3\xA4nder"), ribbons);
    }
    KitTextOn(CTR_TR("TRAINER MEMO", "NOTAS ENTRENADOR", "Trainer-Memo"), x, line += 20, right, 0);
    KitWrap(d->memo, x, line + 14, right - x, d->egg ? 6 : 3);
}

static void SPageBody(const CtrSummaryUiView *v)
{
    const CtrPokeTouchRect *r = &kSumPage;
    int x = r->x + 8, right = r->x + r->w - 8, top = r->y + 8;

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    if (v->loading)
        return;
    switch (CtrSummaryUi_Tab(v)) {
    case SUI_TAB_INFO: SInfo(v, x, right, top); break;
    case SUI_TAB_STATS: SStats(v, x, right, top); break;
    case SUI_TAB_MOVES:
        if (v->data.page == SUI_PAGE_MOVES_INFO)
            SMoveRows(v, x, right, top);
        else
            SMoveCards(v);
        break;
    default: STrainer(v, x, right, top); break;
    }
}

/* The band: what the page describes (the ability on INFO, the chosen move in its details) or what a touch does there. */
static const char *SNoteText(const CtrSummaryUiView *v)
{
    const CtrSummaryView *d = &v->data;

    if (v->loading)
        return "";
    switch (CtrSummaryUi_Tab(v)) {
    case SUI_TAB_INFO:
        return d->egg ? "" : d->abilityDesc;
    case SUI_TAB_STATS:
        return v->ivView ? CTR_TR("Tap the stats for the EXP.", "Toca los datos para ver la EXP.", "Werte antippen: EP.")
                         : CTR_TR("Tap the stats for IV and EV.", "Toca los datos para ver IV y EV.", "Werte antippen: DV und FP.");
    case SUI_TAB_MOVES:
        if (d->page == SUI_PAGE_MOVES)
            return CTR_TR("Tap a move for its details.", "Toca un ataque para ver sus datos.", "Attacke antippen f\xC3\xBCr Details.");
        if (d->swapping)
            return CTR_TR("Tap where to move it.", "Toca d\xC3\xB3nde moverlo.", "Neuen Platz antippen.");
        return d->cursor >= 0 && d->cursor < 4 && d->moves[d->cursor] ? d->moveDesc
             : CTR_TR("Tap the selected move to move it.", "Toca el ataque elegido para moverlo.", "Ausgew\xC3\xA4hlte Attacke antippen zum Tauschen.");
    default:
        return d->egg ? "" : CTR_TR("UP / DOWN: another POK\xc3\xa9MON", "ARRIBA / ABAJO: otro POK\xc3\xa9MON", "Hoch / Runter: anderes Pok\xc3\xa9mon");
    }
}

static void SNote(const CtrSummaryUiView *v)
{
    const CtrPokeTouchRect *n = &kSumNote;

    Fill(n->x, n->y, n->w, n->h, BPageBg());
    KitPanel(n, KIT_FRAME_DIALOGUE, 0);
    KitWrap(SNoteText(v), n->x + 16, n->y + 10, n->w - 32, 2);
}

static void SBack(const CtrSummaryUiView *v)
{
    CtrPokeTouchRect in = BPageInside(&kSumBack);

    Fill(kSumBack.x, kSumBack.y, kSumBack.w, kSumBack.h, BPageBg());
    KitPanel(&kSumBack, KIT_FRAME_STD, v->frameType);
    KitRow(&in, CTR_TR("BACK", "ATR\xC3\x81S", "Zur\xC3\xBC" "ck"), v->pressed == SUI_HIT_BACK, 1);
}

static void SMonCells(const CtrSummaryUiView *v)
{
    int on = SSwitchEnabled(v);

    SArrowCell(v, &kSumMonPrev, 0, on, on && v->pressed == SUI_HIT_PREV);
    SArrowCell(v, &kSumMonNext, 1, on, on && v->pressed == SUI_HIT_NEXT);
}

void CtrSummaryUi_Draw(uint16_t *canvas, const CtrSummaryUiView *v)
{
    sCanvas = canvas;
    Fill(0, 0, W, H, BPageBg());
    STitle(v);
    SMon(v);
    SPageBody(v);
    SMonCells(v);
    SNote(v);
    SBack(v);
}

static void SMark(CtrDirtyList *dirty, const CtrPokeTouchRect *r)
{
    CtrDirty_Add(dirty, r->x, r->y, r->x + r->w, r->y + r->h);
}

/* A press or its release redraws its own control (a move row, the switch: the page); anything else, the screen. */
void CtrSummaryUi_Update(uint16_t *canvas, const CtrSummaryUiView *old, const CtrSummaryUiView *v, CtrDirtyList *dirty)
{
    CtrSummaryUiView a = *old;
    CtrPokeTouchRect all = {0, 0, W, H};

    sCanvas = canvas;
    CtrDirty_Clear(dirty);
    a.pressed = v->pressed;
    if (memcmp(&a, v, sizeof(a))) {
        CtrSummaryUi_Draw(canvas, v);
        SMark(dirty, &all);
        return;
    }
    if (old->pressed == v->pressed)
        return;
    if ((old->pressed == SUI_HIT_PAGE_PREV) != (v->pressed == SUI_HIT_PAGE_PREV)
        || (old->pressed == SUI_HIT_PAGE_NEXT) != (v->pressed == SUI_HIT_PAGE_NEXT)) {
        STitle(v);
        SMark(dirty, &kSumTitle);
        SMark(dirty, &kSumPagePrev);
        SMark(dirty, &kSumPageNext);
    }
    if ((old->pressed == SUI_HIT_PREV) != (v->pressed == SUI_HIT_PREV)
        || (old->pressed == SUI_HIT_NEXT) != (v->pressed == SUI_HIT_NEXT)) {
        SMonCells(v);
        SMark(dirty, &kSumMonPrev);
        SMark(dirty, &kSumMonNext);
    }
    if ((old->pressed == SUI_HIT_BACK) != (v->pressed == SUI_HIT_BACK)) {
        SBack(v);
        SMark(dirty, &kSumBack);
    }
    if ((old->pressed >= SUI_HIT_MOVE && old->pressed <= SUI_HIT_IVTOGGLE)
        || (v->pressed >= SUI_HIT_MOVE && v->pressed <= SUI_HIT_IVTOGGLE)) {
        SPageBody(v);
        SMark(dirty, &kSumPage);
    }
}
