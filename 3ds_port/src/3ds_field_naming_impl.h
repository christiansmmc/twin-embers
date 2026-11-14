/* FireRed's naming screen as a page (3ds_poketouch.h CtrPokeTouchName; phase 2), included at the end of 3ds_poketouch.c
 * after the Pokedex, whose picture helpers it shares. A remote for FireRed's own keyboard: the name so far in a window
 * (the Pokemon's icon, a place per letter it takes, the gender sign), the three pages' tabs in a window beside it (the
 * page shown underlined), the page's keys (FireRed's own characters) in a window under them, FireRed's question and the
 * hint in the band, DELETE and OK beside it. FireRed's arrow marks its cursor (a key, the tab its page button turns to,
 * DELETE or OK) or what is held. Every character is FireRed's (CtrBottomNaming_*), but for the tabs' and buttons' labels
 * (3ds_ui_strings.h). Its FireRed text helpers (FNChar, FNText) are shared with the PC box. */

/* One FireRed code as text for UiText: its glyph as it is (GlyphCode's 0x01 escape), the space as a space. */
static const char *FNChar(uint8_t code, char out[3])
{
    out[0] = code ? 0x01 : ' ';
    out[1] = (char)code;
    out[2] = 0;
    if (!code)
        out[1] = 0;
    return out;
}

/* How many argument bytes FireRed's text effect `code` (after 0xfc) takes, as GetStringWidth in text.c counts them. */
static int FNEffectArgs(uint8_t code)
{
    switch (code) {
    case 0x04:                          /* colour, highlight and shadow */
        return 3;
    case 0x0b: case 0x10:               /* play a song or a sound */
        return 2;
    case 0x01: case 0x02: case 0x03: case 0x05: case 0x06: case 0x08: case 0x0c: case 0x0d: case 0x0e:
    case 0x11: case 0x12: case 0x13: case 0x14:
        return 1;
    default:
        return 0;
    }
}

/* FireRed's extra symbol `code` (after 0xf9) as text for UiText, or NULL when it has no glyph here. */
static const char *FNSymbol(uint8_t code)
{
    switch (code) {
    case 0x04: return "\x01\x2e";          /* + */
    case 0x05: return "\x01\x34";          /* Lv, FireRed's own glyph */
    case 0x06: return "PP";
    case 0x07: return "ID";
    case 0x08: return "No";
    case 0x13: return "(";
    case 0x14: return ")";
    default: return NULL;
    }
}

/* FireRed codes (0xff ended) as text for UiText: its control codes are left out, a line break is a space. */
static void FNText(const uint8_t *raw, int n, char *out, int size)
{
    int o = 0, jp = 0;

    for (int i = 0; i < n && raw[i] != 0xff && o + 3 <= size; ++i) {
        uint8_t c = raw[i];

        if (c == 0xfc && i + 1 < n && (raw[i + 1] == 0x15 || raw[i + 1] == 0x16)) {
            jp = raw[i + 1] == 0x15;    /* a Japanese Pokemon's name, between JPN and ENG: its own glyphs */
            ++i;
            continue;
        }
        if (c == 0xfc) {                /* a text effect: its code and arguments are not glyphs */
            i += 1 + (i + 1 < n ? FNEffectArgs(raw[i + 1]) : 0);
            continue;
        }
        if (c == 0xf9) {                /* an extra symbol: Lv in the box's note, PP, ID... */
            const char *sym = i + 1 < n ? FNSymbol(raw[i + 1]) : NULL;

            for (; sym && *sym && o + 1 < size; ++sym)
                out[o++] = *sym;
            ++i;
            continue;
        }
        if (c == 0xf7 || c == 0xf8 || c == 0xfd) {     /* a buffer, a button icon or a placeholder: one more byte */
            ++i;
            continue;
        }
        if (c == 0xfe || c == 0)
            out[o++] = ' ';
        else if (c < 0xf7) {
            out[o++] = jp ? 0x02 : 0x01;
            out[o++] = (char)c;
        }
    }
    out[o] = 0;
}

/* The space key's mark: a flat open bracket under the middle of the key. */
static void FNSpaceMark(int cx, int y, uint16_t c)
{
    Fill(cx - 6, y + 6, 13, 1, c);
    Fill(cx - 6, y + 3, 1, 3, c);
    Fill(cx + 6, y + 3, 1, 3, c);
}

/* Each page's tab, left to right: capitals, small letters, then FireRed's symbols page (FireRed's page numbers). */
static const int8_t kFNPageSlot[3] = {2, 0, 1};
static const int kFNPageLabel[3] = {UIS_NAME_SYMBOLS, UIS_NAME_UPPER, UIS_NAME_LOWER};

int CtrPokeTouch_NameHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchName *n = &v->name;

    if (!n->on)
        return -1;
    for (int p = 0; p < 3; ++p) {
        CtrPokeTouchRect r = NameTab(kFNPageSlot[p]);

        if (UiIn(&r, x, y))
            return PT_NAME_HIT_TAB + p;
    }
    if (UiIn(&kNameDelete, x, y))
        return PT_NAME_HIT_DELETE;
    if (UiIn(&kNameOk, x, y))
        return PT_NAME_HIT_OK;
    for (int row = 0; row < PT_NAME_ROWS; ++row)
        for (int col = 0; col < n->cols && col < PT_NAME_COLS; ++col) {
            CtrPokeTouchRect r = NameKey(n->cols, row, col);

            if (n->key[row][col] != 0xff && UiIn(&r, x, y))
                return PT_NAME_HIT_KEY + row * PT_NAME_COLS + col;
        }
    return -1;
}

/* The arrow's place: what is held under a touch, else FireRed's cursor (`cursorHere`). */
static int FNChosen(const CtrPokeTouchName *n, int code, int cursorHere)
{
    return n->pressed >= 0 ? n->pressed == code : cursorHere;
}

/* The Pokemon being named: its icon (FireRed's, as its naming screen shows it) at (x, y). */
static void FNIcon(unsigned species, int x, int y)
{
    static unsigned char tiles[1024];
    static unsigned short pal[16];
    static uint8_t px[32 * 32];
    static uint16_t pal565[16];

    if (!species || !CtrPokeTouch_MonIcon(species, 0, 0, tiles, pal))
        return;
    FDTiles(tiles, 32, 32, px);
    FDPal(pal, pal565);
    UiImage(px, pal565, 32, 32, 32, x, y, 1, 1);
}

/* The name so far: the Pokemon's icon, a place per letter it takes (the one the next letter takes underlined in ink),
 * the gender sign after them in blue or red. */
static void FNEntry(const CtrPokeTouchView *v)
{
    const CtrPokeTouchName *n = &v->name;
    const CtrPokeTouchRect *r = &kNameEntry;
    int slots = n->maxChars ? n->maxChars : 1, x0 = r->x + 8 + (n->species ? 40 : 0), right = r->x + r->w - 8;
    int pitch = (right - x0 - 12) / slots, top = r->y + 8 + (32 - KIT_LINE_H) / 2 - 2;
    char g[3];

    if (pitch > 10)
        pitch = 10;
    FPageWindow(r, v->frameType);
    if (n->species)
        FNIcon(n->species, r->x + 8, r->y + 8);
    for (int i = 0; i < slots; ++i) {
        int sx = x0 + i * pitch;

        if (i < n->len && n->text[i]) {
            FNChar(n->text[i], g);
            KitText(g, sx + (pitch - UiTextWidth(g, 1)) / 2, top, sx + pitch);
        }
        Fill(sx + 1, top + KIT_LINE_H + 2, pitch - 2, 2, i == n->len ? KitInk(2) : KitInk(3));
    }
    if (n->gender) {
        char sign[3];

        FNChar(n->gender == 2 ? 0xb6 : 0xb5, sign);
        UiText(sign, x0 + slots * pitch + 4, top + 3, n->gender == 2 ? kPPageFemale : kPPageMale, KitInk(3), 1, right);
    }
}

/* The tabs: the page shown underlined; FireRed's arrow on the one held, or, while FireRed's cursor is on its page button,
 * on the page its A turns to. */
static void FNTabs(const CtrPokeTouchView *v)
{
    const CtrPokeTouchName *n = &v->name;
    int onButtons = n->curCol >= n->cols;

    FPageWindow(&kNamePages, v->frameType);
    for (int p = 0; p < 3; ++p) {
        CtrPokeTouchRect t = NameTab(kFNPageSlot[p]);
        const char *label = CtrUiText(kFNPageLabel[p]);
        int top = t.y + (t.h - KIT_LINE_H) / 2, x = t.x + KIT_ARROW_W + 1;

        Fill(t.x, t.y, t.w, t.h, n->pressed == PT_NAME_HIT_TAB + p ? KitInk(3) : KitInk(1));
        if (FNChosen(n, PT_NAME_HIT_TAB + p, onButtons && n->curRow == 0 && p == (n->page + 1) % 3))
            KitText(KIT_ARROW, t.x + 1, top, x);
        KitText(label, x, top, t.x + t.w);
        if (n->page == p)
            Fill(x, t.y + t.h - 5, UiTextWidth(label, 1), 2, KitInk(2));
    }
}

/* The page's keys (FireRed's own characters; the space key's mark), FireRed's arrow on its cursor or the key held. */
static void FNKeys(const CtrPokeTouchView *v)
{
    const CtrPokeTouchName *n = &v->name;
    int onButtons = n->curCol >= n->cols;
    char g[3];

    FPageWindow(&kNameKeys, v->frameType);
    for (int row = 0; row < PT_NAME_ROWS; ++row)
        for (int col = 0; col < n->cols && col < PT_NAME_COLS; ++col) {
            CtrPokeTouchRect k = NameKey(n->cols, row, col);
            uint8_t code = n->key[row][col];
            int hit = PT_NAME_HIT_KEY + row * PT_NAME_COLS + col, top = k.y + (k.h - KIT_LINE_H) / 2;

            if (code == 0xff)
                continue;
            Fill(k.x, k.y, k.w, k.h, n->pressed == hit ? KitInk(3) : KitInk(1));
            if (FNChosen(n, hit, !onButtons && n->curCol == col && n->curRow == row))
                KitText(KIT_ARROW, k.x + 2, top, k.x + 2 + KIT_ARROW_W);
            if (code == 0)
                FNSpaceMark(k.x + k.w / 2, top + 2, KitInk(2));
            else {
                FNChar(code, g);
                KitText(g, k.x + (k.w - UiTextWidth(g, 1)) / 2, top, k.x + k.w);
            }
        }
}

static void FNamePageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchName *n = &v->name;
    int onButtons = n->curCol >= n->cols;
    char title[66];

    Fill(0, 0, W, H, BPageBg());
    FNEntry(v);
    FNTabs(v);
    FNKeys(v);
    FNText(n->title, (int)sizeof(n->title), title, (int)sizeof(title));
    FPageBand(&kNameNote, title, CtrUiText(UIS_NAME_HINT), 0);
    BagCell(&kNameDelete, v->frameType, CtrUiText(UIS_NAME_DELETE),
            FNChosen(n, PT_NAME_HIT_DELETE, onButtons && n->curRow == 1), n->pressed == PT_NAME_HIT_DELETE, 1);
    BagCell(&kNameOk, v->frameType, CtrUiText(UIS_NAME_OK),
            FNChosen(n, PT_NAME_HIT_OK, onButtons && n->curRow == 2), n->pressed == PT_NAME_HIT_OK, 1);
}
