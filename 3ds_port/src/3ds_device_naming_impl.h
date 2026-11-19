/* The naming keyboard as the red device (no board; plan 3), included by 3ds_poketouch.c after a choice: a row of the
 * field dispatcher (3ds_device_field_impl.h), shown while view->name.on and there is no GBA picture (DevNameShown). A
 * remote for FireRed's own keyboard (3ds_poketouch.h CtrPokeTouchName): the name's screen (the icon of a Pokemon being
 * named, FireRed's question over a slot for each character the name takes: a character typed, an underline under an
 * empty one, darker under the next, the gender sign after them); five keys across under it, the three pages (ABC, abc,
 * 123), DELETE and OK; the page's keys in four rows of FireRed's 8 columns (6 for its symbols), every place of them a key
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

/* A Pokemon's icon (FireRed's, its first frame) at half size, its top-left at (x, y). */
static void DevNameIcon(unsigned species, int x, int y)
{
    static unsigned char tiles[PT_MON_ICON_BYTES];  /* the bridge writes the icon's two frames */
    static unsigned short pal[16];
    static uint8_t px[32 * 32];
    static uint16_t pal565[16];

    DevArt(DEV_ART_ICON, x, y, 16, 16);
    if (!CtrPokeTouch_MonIcon(species, 0, 0, tiles, pal))
        return;
    FDTiles(tiles, 32, 32, px);
    FDPal(pal, pal565);
    DevIconPixels(px, pal565, x, y, 1, 0);
}

/* The name's screen: a Pokemon's icon at the left (one being named), FireRed's question small in the secondary ink, under
 * it a slot for each character the name takes (16 apart at most, 248 shared out): a character typed centred on its
 * slot, an underline under an empty one, in the paper's ink under the next to be typed; the gender sign after them. */
static void DevNameEntry(const CtrPokeTouchName *n)
{
    CtrPokeTouchRect paper = DevScreen(&kDevNameScreen);
    int x = paper.x + (n->species ? 24 : 6), top = paper.y + 14, pitch = n->maxChars ? 248 / n->maxChars : 16;
    char text[66], g[3];

    if (pitch > 16)
        pitch = 16;
    if (n->species)
        DevNameIcon(n->species, paper.x + 4, paper.y + 6);
    FNText(n->title, (int)sizeof(n->title), text, (int)sizeof(text));
    DevSmallText(text, x, paper.y + 3, DEV_INK2, paper.x + paper.w - 4);
    for (int i = 0; i < n->maxChars && i < PT_NAME_MAX; ++i) {
        int sx = x + pitch * i, sw = pitch - 2;

        if (i < n->len) {
            FNChar(n->text[i], g);
            DevText(g, sx + (sw - DevTextWidth(g)) / 2, top, sx + pitch);
        } else
            DevRect(sx, paper.y + 25, sw, 1, i == n->len ? DEV_INK : DEV_INK_SH);
    }
    if (n->gender)
        DevGender(n->gender == 2, x + pitch * n->maxChars + 2, top);
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
