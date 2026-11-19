/* The naming keyboard as the red device (board def_naming), included by 3ds_poketouch.c after a choice: a row of the
 * field dispatcher (3ds_device_field_impl.h), shown while view->name.on and there is no GBA picture (DevNameShown). A
 * remote for FireRed's own keyboard (3ds_poketouch.h CtrPokeTouchName): the name's screen (the icon of a Pokemon being
 * named in its box, FireRed's question and the counter "3 / 7" over a slot for each character the name takes: a
 * character typed, an underline under every slot, the next's blue, the gender sign after them; the slots where they
 * are with or without an icon); five keys across under it, the three pages (ABC, abc, 123), DELETE and OK; the page's
 * keys in four rows of FireRed's 8 columns (6 for its symbols), every place of them a key
 * (a short row ends in space keys: FireRed's rows are zero-filled, and 0x00 is its space), a place given as EOS (0xff)
 * neither drawn nor hit. The page shown is sunk, or the page key held. FireRed's cursor sinks its key, or on
 * its button column DELETE (row 1) or OK (row 2), or (row 0, the page button) lights the LED of the page key its A turns
 * to, that key up; a held touch sinks what it holds instead (DevChosen). A key moves FireRed's cursor there and presses
 * its A, a page key turns to its page, DELETE is its B, OK its OK (3ds_bottom.c ProcessNameTouch); no BACK. Every
 * rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes; the codes are v0.6.0's (PT_NAME_HIT_*).
 *
 * Words (decision 5): FireRed's question and characters as FireRed has them, the name as typed; ABC, abc and 123 as they
 * are (abc names the small letters), DELETE in capitals, OK.
 *
 * Redraw: the screen whole when the keyboard as drawn differs (DevNameSame), and nothing else.
 *
 * FireRed's codes as text (FNChar, FNText with FNSymbol and FNEffectArgs) and the space key's mark (FNSpaceMark) live
 * here; the PC box, included after, uses them too. */

static int DevNameShown(const CtrPokeTouchView *v)
{
    return v->name.on && !v->viewport;
}

/* The page each page key turns to (FireRed's: 0 the symbols, 1 the capitals, 2 the small letters), and its word. */
static const uint8_t kDevNamePage[3] = {1, 2, 0};
static const int kDevNamePageWord[3] = {UIS_NAME_UPPER, UIS_NAME_LOWER, UIS_NAME_SYMBOLS};

/* The page's columns drawn: FireRed's, PT_NAME_COLS at most. */
static int DevNameCols(const CtrPokeTouchName *n)
{
    return n->cols < PT_NAME_COLS ? n->cols : PT_NAME_COLS;
}

int CtrPokeTouch_NameHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchName *n = &v->name;

    if (!DevNameShown(v))
        return -1;
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevNameTop(i);

        if (DevOnKey(&r, x, y))
            return i < 3 ? PT_NAME_HIT_TAB + kDevNamePage[i] : i == 3 ? PT_NAME_HIT_DELETE : PT_NAME_HIT_OK;
    }
    for (int row = 0; row < PT_NAME_ROWS; ++row)
        for (int col = 0; col < DevNameCols(n); ++col) {
            CtrPokeTouchRect r = DevNameKey(n->cols, row, col);

            if (n->key[row][col] != 0xff && DevOnKey(&r, x, y))
                return PT_NAME_HIT_KEY + row * PT_NAME_COLS + col;
        }
    return -1;
}

/* ---- FireRed's codes as text ------------------------------------------------------------------------------------------ */

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

/* The space key's mark in c: a flat open bracket 13 wide, its arms 3 tall, its top middle at (cx, y + 3). */
static void FNSpaceMark(int cx, int y, uint16_t c)
{
    DevRect(cx - 6, y + 6, 13, 1, c);
    DevRect(cx - 6, y + 3, 1, 3, c);
    DevRect(cx + 6, y + 3, 1, 3, c);
}

/* ---- The keyboard ------------------------------------------------------------------------------------------------------ */

/* A Pokemon's icon (FireRed's 32x32, its first frame) at 1:1 from (16, 10), its pixels outside kDevNameIcon not drawn
 * (the board's 24x24 box: the Pokemon, not the icon's empty rows and columns). */
static void DevNameIcon(unsigned species)
{
    static unsigned char tiles[PT_MON_ICON_BYTES];  /* the bridge writes the icon's two frames */
    static unsigned short pal[16];
    static uint8_t px[32 * 32];
    static uint16_t pal565[16];
    const CtrPokeTouchRect *box = &kDevNameIcon;
    const int x = 16, y = 10;

    DevArt(DEV_ART_ICON, box->x, box->y, box->w, box->h);
    if (!CtrPokeTouch_MonIcon(species, 0, 0, tiles, pal))
        return;
    FDTiles(tiles, 32, 32, px);
    FDPal(pal, pal565);
    for (int py = box->y; py < box->y + box->h; ++py)
        for (int qx = box->x; qx < box->x + box->w; ++qx) {
            uint8_t c = px[(py - y) * 32 + qx - x];

            if (c)
                Px(qx, py, pal565[c]);
        }
}

/* The name's screen (def_naming): a Pokemon's icon in its box at the left (one being named; the box the paper's
 * otherwise), FireRed's question small in the secondary ink from DEV_NAME_TITLE_X and, ending at DEV_NAME_COUNT_RIGHT,
 * the counter of the characters typed and the most the name takes ("3 / 7"; both capitals 17: the question cut 8 px
 * before the counter, and counted in sDevKeyCut); under them a slot for each character the name takes (DevNameSlot,
 * where they are with or without an icon): a character typed in the title's ink from 2 px into its slot, capitals 28;
 * an underline under every slot, in the paper's line, the mark's blue under the next to be typed; the gender sign after
 * them. */
static void DevNameEntry(const CtrPokeTouchName *n)
{
    const int smallTop = 17;                            /* the question's and the counter's capitals */
    int count = n->maxChars < PT_NAME_MAX ? n->maxChars : PT_NAME_MAX, from;
    char text[66], counter[12], g[3];

    DevScreen(&kDevNameScreen);
    if (n->species)
        DevNameIcon(n->species);
    snprintf(counter, sizeof(counter), "%u / %u", (unsigned)n->len, (unsigned)n->maxChars);
    from = DEV_NAME_COUNT_RIGHT - DevSmallTextWidth(counter);
    DevSmallText(counter, from, smallTop, DEV_INK2, DEV_NAME_COUNT_RIGHT);
    FNText(n->title, (int)sizeof(n->title), text, (int)sizeof(text));
    if (DevSmallTextWidth(text) > from - 8 - DEV_NAME_TITLE_X)
        ++sDevKeyCut;                                   /* cut before the counter */
    DevSmallText(text, DEV_NAME_TITLE_X, smallTop, DEV_INK2, from - 8);
    for (int k = 0; k < count; ++k) {
        CtrPokeTouchRect s = DevNameSlot(k);

        DevRect(s.x, s.y + s.h - 1, s.w, 1, k == n->len ? DEV_MARK_ON : DEV_PAPER_LINE);
        if (k >= n->len)
            continue;
        if (DevTextWidth(FNChar(n->text[k], g)) > s.w)
            ++sDevKeyCut;                               /* wider than its slot and the gap after it: left out */
        DevTextInk(g, s.x + 2, s.y + 2, s.x + 2 + s.w, DEV_TITLE_INK, DEV_INK_SH);
    }
    if (n->gender)
        DevGender(n->gender == 2, DevNameSlot(count).x + 2, DevNameSlot(count).y + 1);
}

/* The five keys across: the page keys (the page shown sunk, or the one held; FireRed's cursor on its page button lights
 * the LED of the one its A turns to, where a sunk key has its own, the key up), DELETE and OK (sunk by the cursor on
 * FireRed's button column, rows 1 and 2, or held). */
static void DevNameTops(const CtrPokeTouchName *n)
{
    int onButtons = n->curCol >= n->cols;
    int tab = n->pressed >= PT_NAME_HIT_TAB && n->pressed < PT_NAME_HIT_TAB + 3 ? n->pressed - PT_NAME_HIT_TAB : -1;
    int button = !onButtons ? -1 : n->curRow == 1 ? PT_NAME_HIT_DELETE : n->curRow == 2 ? PT_NAME_HIT_OK : -1;
    int next = onButtons && n->curRow == 0 && n->pressed < 0 ? (n->page + 1) % 3 : -1;
    char word[24];

    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevNameTop(i);
        int sunk = i < 3 ? DevChosen(kDevNamePage[i], n->page, tab)
                         : DevChosen(i == 3 ? PT_NAME_HIT_DELETE : PT_NAME_HIT_OK, button, n->pressed);
        int faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);

        if (i < 3 && !sunk && kDevNamePage[i] == next)
            DevLed(r.x + r.w - 7 - 4, faceY + 4, 4, DEV_KEY);
        if (i == 3)
            DevUpper(word, CtrUiText(UIS_NAME_DELETE), sizeof(word));
        DevKeyWord(&r, faceY, DEV_LOOK_KEY, sunk, i < 3 ? CtrUiText(kDevNamePageWord[i]) : i == 3 ? word : CtrUiText(UIS_NAME_OK));
    }
}

/* The page's keys: FireRed's character centred on each, the space key's mark; the key under a held touch sunk, else
 * FireRed's cursor's (on the keys). */
static void DevNameKeys(const CtrPokeTouchName *n)
{
    int cursor = n->curCol < n->cols ? n->curRow * PT_NAME_COLS + n->curCol : -1;
    char g[3];

    for (int row = 0; row < PT_NAME_ROWS; ++row)
        for (int col = 0; col < DevNameCols(n); ++col) {
            CtrPokeTouchRect r = DevNameKey(n->cols, row, col);
            uint8_t code = n->key[row][col];
            int sunk, faceY;

            if (code == 0xff)
                continue;
            sunk = DevChosen(row * PT_NAME_COLS + col, cursor, n->pressed - PT_NAME_HIT_KEY);
            faceY = DevKey(&r, DEV_LOOK_KEY, sunk, 6);
            if (code == 0)
                FNSpaceMark(r.x + r.w / 2, faceY + (r.h - UI_TEXT_H) / 2, DevKeyInk(DEV_LOOK_KEY, sunk));
            else
                DevKeyWord(&r, faceY, DEV_LOOK_KEY, sunk, FNChar(code, g));
        }
}

/* The keyboard whole (after DevBegin: the dispatcher's). */
static void DevNameDraw(const CtrPokeTouchView *v)
{
    DevShell();
    DevNameEntry(&v->name);
    DevNameTops(&v->name);
    DevNameKeys(&v->name);
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the keyboard draws differs between `a` and `b` (the shell is the dispatcher's): the keyboard as filled. */
static int DevNameSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return !memcmp(&a->name, &b->name, sizeof(a->name));
}

/* No BACK: DELETE is FireRed's B. */
static const CtrPokeTouchRect *DevNameBackKey(const CtrPokeTouchView *v)
{
    (void)v;
    return NULL;
}
