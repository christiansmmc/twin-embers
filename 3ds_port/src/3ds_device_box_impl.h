/* The PC box as the red device (def_box; owner decision O3), included by 3ds_poketouch.c after the naming keyboard,
 * whose FireRed text (FNText) it shares: a row of the field dispatcher (3ds_device_field_impl.h), shown while
 * view->box.on and there is no GBA picture (DevBoxShown). A remote for FireRed's own box (3ds_poketouch.h
 * CtrPokeTouchBox):
 *  - the box's screen: plain paper (no wallpaper: owner decision O3), the box's name as typed centred on the band at its
 *    top (inverted while FireRed's cursor is on it), the thirty places 24 px apart, each a square of the lighter paper
 *    with its Pokemon's icon (FireRed's, clipped to the paper), the cursor's place framed; MOVE ITEMS: a blue square at
 *    a place's lower right when its Pokemon holds an item. With the cursor in the Party the Party's rows on the same
 *    paper instead (half icon, nickname, level; CANCEL), the cursor's inverted, and the keys beside it grey (v0.6.0
 *    takes only the Party then);
 *  - the right column: ◀ ▶ (sunk while held); the hand's screen (IN HAND, and the Pokemon in hand with its name, or
 *    MOVE ITEMS' item in hand); PARTY POKéMON (cream, sunk by FireRed's cursor on its buttons or a held touch); how
 *    many Pokemon the box holds; CLOSE in the grid's exit (dark, sunk the same);
 *  - the footer (the grid's note): FireRed's message (its ▼ while it waits for A, and then a tap anywhere is its A: the
 *    keys grey); else MOVE ITEMS' item under the cursor; else the Pokemon under the cursor (its name, the gender's sign
 *    and level; its type chips and ITEM with its item); else the box's name;
 *  - a menu or the marks: their keys in the right column (the menu's rows; the four marks at 2x and OK), a menu's CANCEL
 *    row (FireRed's last, menuCancel: recipe 0268) and the marks' CANCEL the dark key in the grid's exit; a menu without
 *    CANCEL (the wallpaper menus) every row in the column and CANCEL in the exit the view's BACK (FireRed's B leaves
 *    it); YES / NO two tall keys in the column (def_choice_yesno) and BACK in the exit (B is NO); FireRed's cursor sunk;
 *  - the box chooser: ◀ ▶ and the chosen box's key in the hand's place (its name and how many it holds), always sunk;
 *    CANCEL in the grid's exit, a dark key: the view's BACK (FireRed's chooser has no CANCEL of its own; BACK sends its
 *    B, as v0.6.0's BACK did there), sunk while held, grey while the chooser takes no B;
 *  - in every state but the box's own and FireRed's busy moments (a menu, YES / NO, the marks, the chooser, a message)
 *    each icon but the cursor's place's (or Party row's) at half strength over what is under it; the paper, the name and
 *    the frame at full strength, the box never dimmed.
 * Under a held touch the key or row it holds is chosen instead of the cursor's (DevChosen, among the box's places, band
 * and keys, the Party's rows, a menu's keys). While FireRed is busy the box is drawn without the cursor's marks and
 * nothing is hit. BACK (DevBoxBack): the chooser's CANCEL, YES / NO's BACK and a CANCEL-less menu's CANCEL; none else
 * (CLOSE is FireRed's exit, a menu's CANCEL row and the marks' CANCEL FireRed's own). A touch on the box is the PC's own
 * controller (3ds_bottom.c ProcessStorageTouch, the 3ds_storage_touch.h codes: the cursor walked there and A); on BACK
 * it is BACK's (ProcessBack: FireRed's B). Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch
 * takes.
 *
 * Words (decision 5): FireRed's menu rows and items in capitals (DevUpper); a nickname, the box's name and the chosen
 * box's name as typed, a nickname that is its species' own name in capitals; FireRed's messages and descriptions as it
 * writes them; the port's words in capitals. Every word or line that can be cut is counted in sDevKeyCut.
 *
 * Redraw: the screen whole when the box as drawn differs (DevBoxSame), and nothing else. */
#include "3ds_storage_touch.h"

#define DEV_BOX_TEXT     200                        /* FireRed's codes as text: two bytes a code at most */
#define DEV_BOX_MARKS    6                          /* the marks' keys: the four marks, OK, CANCEL */

static int DevBoxShown(const CtrPokeTouchView *v)
{
    return v->box.on && !v->viewport;
}

/* The states with keys of their own instead of the right column's: a menu, YES / NO, the marks. */
static int DevBoxKeyed(const CtrPokeTouchBox *b)
{
    return b->state == PT_BOX_STATE_MENU || b->state == PT_BOX_STATE_YESNO || b->state == PT_BOX_STATE_MARKS;
}

/* The keys of the state: the menu's rows, YES and NO, the marks; none in the other states. */
static int DevBoxKeyCount(const CtrPokeTouchBox *b)
{
    switch (b->state) {
    case PT_BOX_STATE_MENU:
        return b->menuCount < PT_BOX_MENU_MAX ? b->menuCount : PT_BOX_MENU_MAX;
    case PT_BOX_STATE_YESNO:
        return 2;
    case PT_BOX_STATE_MARKS:
        return DEV_BOX_MARKS;
    default:
        return 0;
    }
}

/* The key in the grid's exit: a menu's last row when it is FireRed's CANCEL (menuCancel: recipe 0268) and the marks'
 * CANCEL; -1 for a menu without CANCEL (the wallpaper menus), YES / NO and the states without keys. */
static int DevBoxExitKey(const CtrPokeTouchBox *b)
{
    int n = DevBoxKeyCount(b);

    return n > 0 && (b->state == PT_BOX_STATE_MARKS || (b->state == PT_BOX_STATE_MENU && b->menuCancel)) ? n - 1 : -1;
}

/* The rows the right column is asked to hold: the state's keys but the exit's. More than DEV_BOX_COLUMN (none of
 * FireRed's menus) loses the rest, counted in sDevKeyCut by DevBoxMenu. */
static int DevBoxColumnRows(const CtrPokeTouchBox *b)
{
    return DevBoxKeyCount(b) - (DevBoxExitKey(b) >= 0);
}

/* The view's BACK in the grid's exit (FireRed's B): the chooser's CANCEL (its chooser has none), YES / NO's BACK (B is
 * NO) and a menu's CANCEL when FireRed's menu ends without one (the wallpaper menus: their B leaves them). */
static int DevBoxBack(const CtrPokeTouchBox *b)
{
    return b->state == PT_BOX_STATE_CHOOSE || b->state == PT_BOX_STATE_YESNO
        || (b->state == PT_BOX_STATE_MENU && !b->menuCancel);
}

/* Where key i of the state stands: YES and NO the column's two tall keys; the grid's exit (DevBoxExitKey); or the right
 * column's key i while i is one of the column's (a menu's rows but CANCEL, the marks and OK; DEV_BOX_COLUMN at most).
 * 0: a key not shown. */
static int DevBoxKeyRect(const CtrPokeTouchBox *b, int i, CtrPokeTouchRect *r)
{
    if (b->state == PT_BOX_STATE_YESNO) {
        if (i < 0 || i > 1)
            return 0;
        *r = i ? kDevBoxNo : kDevBoxYes;
        return 1;
    }
    if (i >= 0 && i == DevBoxExitKey(b)) {
        *r = kDevGridExit;
        return 1;
    }
    if (i < 0 || i >= DevBoxColumnRows(b) || i >= DEV_BOX_COLUMN)
        return 0;
    *r = DevBoxMenuKey(i);
    return 1;
}

int CtrPokeTouch_BoxHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchBox *b = &v->box;
    int n = DevBoxKeyCount(b);

    if (!DevBoxShown(v))
        return ST_HIT_NONE;
    switch (b->state) {
    case PT_BOX_STATE_MESSAGE:
        return PT_BOX_HIT_MESSAGE;                  /* anywhere: the message waits for A */
    case PT_BOX_STATE_MENU:
    case PT_BOX_STATE_YESNO:
    case PT_BOX_STATE_MARKS:
        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r;

            if (DevBoxKeyRect(b, i, &r) && DevOnKey(&r, x, y))
                return ST_HIT_MENU_BASE + i;
        }
        return ST_HIT_NONE;
    case PT_BOX_STATE_CHOOSE:
        return DevOnKey(&kDevBoxPrev, x, y) ? ST_HIT_PREV : DevOnKey(&kDevBoxNext, x, y) ? ST_HIT_NEXT
             : DevOnKey(&kDevBoxChoose, x, y) ? ST_HIT_TITLE : ST_HIT_NONE;
    case PT_BOX_STATE_BOX:
        break;
    default:
        return ST_HIT_NONE;                         /* busy */
    }
    if (b->area == ST_AREA_PARTY) {
        for (int i = 0; i < 7; ++i) {
            CtrPokeTouchRect r = DevBoxPartyRow(i);

            if (UiIn(&r, x, y))
                return ST_HIT_PARTY_BASE + i;
        }
        return ST_HIT_NONE;
    }
    if (DevOnKey(&kDevBoxPrev, x, y))
        return ST_HIT_PREV;
    if (DevOnKey(&kDevBoxNext, x, y))
        return ST_HIT_NEXT;
    if (DevOnKey(&kDevBoxParty, x, y))
        return ST_HIT_BUTTON_PARTY;
    if (DevOnKey(&kDevGridExit, x, y))
        return ST_HIT_BUTTON_CLOSE;
    if (UiIn(&kDevBoxTitle, x, y))
        return ST_HIT_TITLE;
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = DevBoxCell(i);

        if (UiIn(&r, x, y))
            return i;
    }
    return ST_HIT_NONE;
}

/* The touch held as it is drawn: a code of what is drawn and taken in the state shown (a place, the band, ◀ ▶, PARTY
 * POKéMON, CLOSE; a Party row; a key of the column or the exit; the chooser's ◀ ▶), else -1. */
static int DevBoxPress(const CtrPokeTouchBox *b)
{
    int p = b->pressed;
    CtrPokeTouchRect r;

    switch (b->state) {
    case PT_BOX_STATE_BOX:
        if (b->area == ST_AREA_PARTY)
            return p >= ST_HIT_PARTY_BASE && p <= ST_HIT_PARTY_BASE + 6 ? p : -1;
        return (p >= 0 && p < PT_BOX_SLOTS) || (p >= ST_HIT_TITLE && p <= ST_HIT_NEXT) ? p : -1;
    case PT_BOX_STATE_MENU:
    case PT_BOX_STATE_YESNO:
    case PT_BOX_STATE_MARKS:
        return p >= ST_HIT_MENU_BASE && DevBoxKeyRect(b, p - ST_HIT_MENU_BASE, &r) ? p : -1;
    case PT_BOX_STATE_CHOOSE:
        return p == ST_HIT_PREV || p == ST_HIT_NEXT ? p : -1;   /* the chosen box's key is sunk anyway */
    default:
        return -1;
    }
}

/* FireRed's cursor as the code of what it is on (a place, ST_HIT_TITLE, PARTY POKéMON, CLOSE, a Party row), or -1
 * (none; busy). */
static int DevBoxCursor(const CtrPokeTouchBox *b)
{
    if (b->state == PT_BOX_STATE_BUSY)
        return -1;
    switch (b->area) {
    case ST_AREA_BOX:
        return b->pos >= 0 && b->pos < PT_BOX_SLOTS ? b->pos : -1;
    case ST_AREA_TITLE:
        return ST_HIT_TITLE;
    case ST_AREA_BUTTONS:
        return b->pos == 0 ? ST_HIT_BUTTON_PARTY : b->pos == 1 ? ST_HIT_BUTTON_CLOSE : -1;
    case ST_AREA_PARTY:
        return b->pos >= 0 && b->pos <= 6 ? ST_HIT_PARTY_BASE + b->pos : -1;
    default:
        return -1;
    }
}

/* ---- FireRed's text and art --------------------------------------------------------------------------------------- */

/* FireRed's codes as text (FNText), its letters and digits as ASCII: they draw the same (GlyphCode) and DevUpper can
 * put them in capitals. */
static void DevBoxText(const uint8_t *raw, int n, char *out, int size)
{
    int i = 0, o = 0;

    FNText(raw, n, out, size);
    while (out[i]) {
        unsigned char c = (unsigned char)out[i], code = (unsigned char)out[i + 1];

        if ((c == 0x01 || c == 0x02) && code) {
            char a = c != 0x01 ? 0 : code >= 0xbb && code <= 0xd4 ? (char)('A' + code - 0xbb)
                   : code >= 0xd5 && code <= 0xee ? (char)('a' + code - 0xd5) : code >= 0xa1 && code <= 0xaa
                   ? (char)('0' + code - 0xa1) : 0;

            if (a) {
                out[o++] = a;
                i += 2;
                continue;
            }
            out[o++] = out[i++];
        }
        out[o++] = out[i++];
    }
    out[o] = 0;
}

/* The same in capitals (FireRed's strings that arrive title-cased: a menu row, an item). */
static void DevBoxUpper(const uint8_t *raw, int n, char *out, int size)
{
    char text[DEV_BOX_TEXT];

    DevBoxText(raw, n, text, (int)sizeof(text));
    DevUpper(out, text, (size_t)size);
}

/* How many codes `raw` holds before its end, the spaces padding it left out. */
static int DevBoxCodeLen(const uint8_t *raw, int n)
{
    int len = 0;

    while (len < n && raw[len] != 0xff)
        ++len;
    while (len > 0 && raw[len - 1] == 0x00)
        --len;
    return len;
}

/* The cursor's Pokemon's name (the one in hand while there is one: FireRed shows it): in capitals when it is its
 * species' own (FireRed's "/" line without the slash), else as typed. */
static void DevBoxMonName(const CtrPokeTouchBox *b, char *out, int size)
{
    const uint8_t *species = b->monText[1] + 1;
    int len = DevBoxCodeLen(b->monText[0], (int)sizeof(b->monText[0]));

    if (b->monText[1][0] == 0xba && len && len == DevBoxCodeLen(species, (int)sizeof(b->monText[1]) - 1)
        && !memcmp(b->monText[0], species, (size_t)len))
        DevBoxUpper(b->monText[0], (int)sizeof(b->monText[0]), out, size);
    else
        DevBoxText(b->monText[0], (int)sizeof(b->monText[0]), out, size);
}

/* `s` without the spaces round it (FireRed pads a held item's name with spaces, and writes eight for none). */
static void DevBoxTrim(char *s)
{
    size_t from = 0, n = strlen(s);

    while (n > 0 && s[n - 1] == ' ')
        s[--n] = 0;
    while (s[from] == ' ')
        ++from;
    memmove(s, s + from, n - from + 1);
}

/* Small text from x in `ink`, capitals at `top`, cut at `right` and then counted in sDevKeyCut. */
static void DevBoxSmall(const char *s, int x, int top, int right, uint16_t ink)
{
    if (DevSmallTextWidth(s) > right - x)
        ++sDevKeyCut;
    DevSmallText(s, x, top, ink, right);
}

#define DEV_A_BOX_FADE 128                          /* a faded icon: half strength over what is under it */

/* A Pokemon's icon (FireRed's, its first frame) with its top-left at (x, y): 32x32 clipped to `clip`, or (`half`) at
 * half size (DevIconPixels); `fade`: each of its pixels at half strength over what is under it (not the place the
 * box's menu, question or message is about). */
static void DevBoxIcon(const CtrPokeTouchBoxMon *m, int x, int y, const CtrPokeTouchRect *clip, int half, int fade)
{
    static unsigned char tiles[PT_MON_ICON_BYTES];  /* the bridge writes both frames; the first is drawn */
    static unsigned short pal[16];
    static uint8_t px[32 * 32];
    static uint16_t pal565[16], under[16 * 16];
    int x0 = x > clip->x ? x : clip->x, y0 = y > clip->y ? y : clip->y;
    int x1 = x + 32 < clip->x + clip->w ? x + 32 : clip->x + clip->w;
    int y1 = y + 32 < clip->y + clip->h ? y + 32 : clip->y + clip->h;

    if (!m->species)
        return;
    if (half)
        DevArt(DEV_ART_ICON, x, y, 16, 16);
    else if (x1 > x0 && y1 > y0)
        DevArt(DEV_ART_ICON, x0, y0, x1 - x0, y1 - y0);
    if (!CtrPokeTouch_MonIcon(m->species, m->personality, m->egg, tiles, pal))
        return;
    FDTiles(tiles, 32, 32, px);
    FDPal(pal, pal565);
    if (half) {                                     /* faded: what was under it, and the icon at half over it */
        for (int i = 0; fade && i < 16 * 16; ++i)
            under[i] = DevGet(x + i % 16, y + i / 16);
        DevIconPixels(px, pal565, x, y, 1, 0);
        for (int i = 0; fade && i < 16 * 16; ++i)
            Px(x + i % 16, y + i / 16, DevBlend(under[i], DevGet(x + i % 16, y + i / 16), DEV_A_BOX_FADE));
        return;
    }
    for (int py = y0; py < y1; ++py)
        for (int qx = x0; qx < x1; ++qx) {
            uint8_t c = px[(py - y) * 32 + qx - x];

            if (c)
                Px(qx, py, fade ? DevBlend(DevGet(qx, py), pal565[c], DEV_A_BOX_FADE) : pal565[c]);
        }
}

/* An item's icon (FireRed's 24x24) at (x, y). */
static void DevBoxItem(unsigned item, int x, int y)
{
    DevArt(DEV_ART_ITEM, x, y, 24, 24);
    DevItemIcon(item, x, y, 0);
}

/* ---- The box's screen ----------------------------------------------------------------------------------------------- */

/* A place's frame: 2 px in the bezel's colour round the inside of r (a place's square and the 1 px round it). */
static void DevBoxFrame(const CtrPokeTouchRect *r)
{
    DevRect(r->x, r->y, r->w, 2, DEV_BEZEL);
    DevRect(r->x, r->y + r->h - 2, r->w, 2, DEV_BEZEL);
    DevRect(r->x, r->y + 2, 2, r->h - 4, DEV_BEZEL);
    DevRect(r->x + r->w - 2, r->y + 2, 2, r->h - 4, DEV_BEZEL);
}

/* The box on its plain paper (owner decision O3): the second paper, each place's square of the first, the name centred
 * on the band (inverted when `cursor` or `press` chooses it), the icons (`fade`: all but the chosen place's at half
 * strength), MOVE ITEMS' squares, the chosen place's frame round its square. */
static void DevBoxPlaces(const CtrPokeTouchBox *b, const CtrPokeTouchRect *paper, int cursor, int press, int fade)
{
    int band = DevChosen(ST_HIT_TITLE, cursor, press);
    char name[DEV_BOX_TEXT];
    int w;

    DevRect(paper->x, paper->y, paper->w, paper->h, DEV_PAPER2);
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = DevBoxCell(i);

        DevRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, DEV_PAPER);
    }
    if (band)
        DevRow(&kDevBoxTitle, 1);
    DevBoxText(b->boxName, (int)sizeof(b->boxName), name, (int)sizeof(name));
    w = DevTextWidth(name);
    DevTextInk(name, kDevBoxTitle.x + (kDevBoxTitle.w - w) / 2, kDevBoxTitle.y + (kDevBoxTitle.h - UI_TEXT_H) / 2,
               kDevBoxTitle.x + kDevBoxTitle.w, band ? DEV_PAPER : DEV_INK, band ? DEV_PAPER : DEV_INK_SH);
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = DevBoxCell(i);

        DevBoxIcon(&b->slot[i], r.x - 4, r.y - 6, paper, 0, fade && !DevChosen(i, cursor, press));
    }
    for (int i = 0; i < PT_BOX_SLOTS; ++i) {
        CtrPokeTouchRect r = DevBoxCell(i), frame = {(int16_t)(r.x - 1), (int16_t)(r.y - 1), 26, 26};

        if (b->items && b->slot[i].species && b->slotItem[i])
            DevRect(r.x + r.w - 8, r.y + r.h - 8, 6, 6, DEV_BLUE_INK);
        if (DevChosen(i, cursor, press))
            DevBoxFrame(&frame);
    }
}

/* The Party on the box's paper: a row for each slot (its icon at half size, its nickname as typed, its level) and
 * CANCEL; the chosen row inverted; `fade`: the icons but the chosen row's at half strength; MOVE ITEMS: a blue square by
 * the icon of a Pokemon that holds an item. */
static void DevBoxParty(const CtrPokeTouchBox *b, int cursor, int press, int fade)
{
    char text[DEV_BOX_TEXT];

    for (int i = 0; i < 7; ++i) {
        CtrPokeTouchRect r = DevBoxPartyRow(i);
        int chosen = DevChosen(ST_HIT_PARTY_BASE + i, cursor, press);
        int top = r.y + (r.h - UI_TEXT_H) / 2, right = r.x + r.w - 6;
        uint16_t ink = DevRowInk(chosen), shadow = chosen ? ink : DEV_INK_SH;

        DevRow(&r, chosen);
        if (i == 6) {
            DevUpper(text, CtrUiText(UIS_CANCEL), sizeof(text));
            DevTextInk(text, r.x + 22, top, right, ink, shadow);
            continue;
        }
        if (!b->party[i].species)
            continue;
        DevBoxIcon(&b->party[i], r.x + 2, r.y + 2, &r, 1, fade && !chosen);
        if (b->items && b->partyItem[i])
            DevRect(r.x + 14, r.y + 12, 6, 6, DEV_BLUE_INK);
        if (!b->party[i].egg) {
            snprintf(text, sizeof(text), CTR_TR("Lv%u", "Nv%u", "Lv%u"), (unsigned)b->partyLevel[i]);
            right -= DevTextWidth(text);
            DevTextInk(text, right, top, r.x + r.w, ink, shadow);
            right -= 6;
        }
        DevBoxText(b->partyName[i], (int)sizeof(b->partyName[i]), text, (int)sizeof(text));
        DevTextInk(text, r.x + 22, top, right, ink, shadow);
    }
}

/* ---- The right column, the hand, the footer ----------------------------------------------------------------------- */

/* ◀ ▶, PARTY POKéMON (cream) and CLOSE (dark, in the grid's exit): sunk as chosen; grey (`dim`) while they are not
 * taken. Under PARTY POKéMON the box's count: how many of its places hold a Pokemon, small, centred on its screen. */
static void DevBoxSide(const CtrPokeTouchBox *b, int cursor, int press, int dim)
{
    CtrPokeTouchRect paper;
    char text[16];
    int sunk, faceY, n = 0;

    DevArrowKeys(&kDevBoxPrev, &kDevBoxNext, 1, 1, press == ST_HIT_PREV, press == ST_HIT_NEXT, 1);
    sunk = DevChosen(ST_HIT_BUTTON_PARTY, cursor, press);
    faceY = DevKey(&kDevBoxParty, DEV_LOOK_KEY, sunk, 6);
    DevKeyWord(&kDevBoxParty, faceY, DEV_LOOK_KEY, sunk,
               CTR_TR("PARTY POK\xC3\xA9MON", "EQUIPO POK\xC3\xA9MON", "POK\xC3\xA9MON-TEAM"));
    sunk = DevChosen(ST_HIT_BUTTON_CLOSE, cursor, press);
    faceY = DevKey(&kDevGridExit, DEV_LOOK_DARK, sunk, 6);
    DevKeyWord(&kDevGridExit, faceY, DEV_LOOK_DARK, sunk, DevCloseWord());
    paper = DevScreen(&kDevBoxCount);
    for (int i = 0; i < PT_BOX_SLOTS; ++i)
        n += b->slot[i].species != 0;
    snprintf(text, sizeof(text), "%u / %u", (unsigned)n, (unsigned)PT_BOX_SLOTS);
    DevSmallText(text, paper.x + (paper.w - DevSmallTextWidth(text)) / 2, paper.y + 3, DEV_INK2, paper.x + paper.w);
    if (!dim)
        return;
    DevDim(&kDevBoxPrev);
    DevDim(&kDevBoxNext);
    DevDim(&kDevBoxParty);
    DevDim(&kDevGridExit);
}

/* The hand's screen: IN HAND small at its top left; the Pokemon in hand (its icon at the screen's left, clipped to the
 * paper, its name small beside it), or MOVE ITEMS' item in hand; or IN HAND alone. */
static void DevBoxHand(const CtrPokeTouchBox *b)
{
    CtrPokeTouchRect paper = DevScreen(&kDevBoxHand);
    int right = paper.x + paper.w - 4;
    char word[DEV_BOX_TEXT];

    DevUpper(word, CtrUiText(UIS_BOX_IN_HAND), sizeof(word));
    DevBoxSmall(word, paper.x + 2, paper.y + 4, right, DEV_INK2);
    if (b->items && b->handItem) {
        DevBoxItem(b->handItem, paper.x + 4, paper.y + 18);
    } else if (b->holding) {
        DevBoxIcon(&b->held, paper.x, paper.y + 14, &paper, 0, 0);
        DevBoxMonName(b, word, (int)sizeof(word));
        DevBoxSmall(word, paper.x + 38, paper.y + 26, right, DEV_INK2);
    }
}

/* The Pokemon under the cursor on the footer's paper: line 1 its name, the gender's sign and level (FireRed's line);
 * line 2 (not for an egg) its type chips, 3 px apart, and ITEM with FireRed's item (its padding left out) or NONE, small
 * in the secondary ink 7 px after the chips. */
static void DevBoxMonNote(const CtrPokeTouchBox *b, const CtrPokeTouchRect *paper)
{
    int x = paper->x + 6, right = paper->x + paper->w - 6, top = paper->y + 3, at;
    char text[DEV_BOX_TEXT], line[DEV_BOX_TEXT + 16];
    const char *rest = text;

    DevBoxMonName(b, text, (int)sizeof(text));
    DevText(text, x, top, right);
    at = x + DevTextWidth(text) + 6;
    DevBoxText(b->monText[2], (int)sizeof(b->monText[2]), text, (int)sizeof(text));
    if (text[0] == 0x01 && (text[1] == (char)0xb5 || text[1] == (char)0xb6)) {
        DevGender(text[1] == (char)0xb6, at, top - 1);
        at += 12;
        rest += 2;
    }
    while (*rest == ' ')
        ++rest;
    if (at + DevTextWidth(rest) > right)
        ++sDevKeyCut;
    DevText(rest, at, top, right);
    if (b->mon.egg)
        return;
    at = x;
    for (int k = 0; k < 2; ++k)
        if (b->monType[k] >= 0)
            at += DevTypeChip(b->monTypeName[k], b->monType[k], at, paper->y + 16, 10) + 3;
    if (at > x)
        at += 4;                                    /* 7 px after the last chip */
    DevBoxUpper(b->monText[3], (int)sizeof(b->monText[3]), text, (int)sizeof(text));
    DevBoxTrim(text);
    snprintf(line, sizeof(line), "%s %s", CTR_TR("ITEM", "OBJETO", "ITEM"), text[0] ? text : CTR_TR("NONE", "NINGUNO", "KEINS"));
    DevBoxSmall(line, at, paper->y + 18, right, DEV_INK2);
}

/* MOVE ITEMS: the item the Pokemon under the cursor holds (its icon, its name, the Pokemon's ending at the paper's
 * right, FireRed's description small in two lines), or the Pokemon's name and that it holds none. */
static void DevBoxItemNote(const CtrPokeTouchBox *b, const CtrPokeTouchRect *paper)
{
    int x = paper->x + 34, right = paper->x + paper->w - 6, top = paper->y + 2;
    char text[DEV_BOX_TEXT], name[DEV_BOX_TEXT];
    int w;

    DevBoxMonName(b, name, (int)sizeof(name));
    if (!b->monItem) {
        DevText(name, paper->x + 6, paper->y + 3, right);
        DevUpper(text, CtrUiText(UIS_BOX_NO_ITEM), sizeof(text));
        DevBoxSmall(text, paper->x + 6, paper->y + 18, right, DEV_INK2);
        return;
    }
    DevBoxItem(b->monItem, paper->x + 4, paper->y + 3);
    w = DevTextWidth(name);
    DevText2(name, right - w, top, right);
    DevBoxUpper(b->itemName, (int)sizeof(b->itemName), text, (int)sizeof(text));
    if (DevTextWidth(text) > right - w - 6 - x)
        ++sDevKeyCut;
    DevText(text, x, top, right - w - 6);
    DevBoxText(b->itemDesc, (int)sizeof(b->itemDesc), text, (int)sizeof(text));
    DevSmallFit(text, x, x, paper->y + 13, right, 2, DEV_SMALL_LINE_TIGHT, DEV_INK);
}

/* The footer (the grid's note, in every state): FireRed's message while there is one (read on the paper; its ▼ while
 * it waits for A); else MOVE ITEMS' item, the Pokemon under the cursor, or the box's name centred down the paper. */
static void DevBoxNote(const CtrPokeTouchBox *b)
{
    CtrPokeTouchRect paper = DevScreen(&kDevGridNote);
    int x = paper.x + 6, right = paper.x + paper.w - 6;
    char text[DEV_BOX_TEXT];

    if (b->message[0] != 0xff) {
        int wait = b->state == PT_BOX_STATE_MESSAGE;

        DevBoxText(b->message, (int)sizeof(b->message), text, (int)sizeof(text));
        DevReadText(text, &paper, x, right - (wait ? 12 : 0), paper.y + 2, DEV_SMALL_LINE_TIGHT);
        if (wait)
            DevTriangle(paper.x + paper.w - 10, paper.y + paper.h - 6, 1, DEV_INK2);
        return;
    }
    if (!b->mon.species) {
        DevBoxText(b->boxName, (int)sizeof(b->boxName), text, (int)sizeof(text));
        DevText(text, x, paper.y + (paper.h - 8) / 2, right);
    } else if (b->items)
        DevBoxItemNote(b, &paper);
    else
        DevBoxMonNote(b, &paper);
}

/* A mark's shape (FireRed's 8x8: the mark and its edge) at 2x in `ink`, its top-left at (x, y). */
static void DevBoxMark(int mark, int x, int y, uint16_t ink)
{
    unsigned char px[64];

    DevArt(DEV_ART_MARK, x, y, 16, 16);
    if (!CtrPokeTouch_MarkShape(mark, px))
        return;
    for (int i = 0; i < 64; ++i)
        if (px[i] == 3 || px[i] == 4)
            DevRect(x + 2 * (i % 8), y + 2 * (i / 8), 2, 2, ink);
}

/* The state's keys (DevBoxKeyRect): in the right column the menu's rows (all of a menu without CANCEL) or the four
 * marks (on in the key's ink, off dim) and OK, their words from x + 10, or YES and NO, two tall keys, their words
 * centred; a menu's CANCEL row (FireRed's word) and the marks' CANCEL the dark key in the grid's exit; FireRed's cursor
 * sunk, or the key under a held touch. Rows past the column's DEV_BOX_COLUMN are counted in sDevKeyCut. */
static void DevBoxMenu(const CtrPokeTouchBox *b, int press)
{
    int n = DevBoxKeyCount(b), held = press >= 0 ? press - ST_HIT_MENU_BASE : -1;
    char word[DEV_BOX_TEXT];

    if (DevBoxColumnRows(b) > DEV_BOX_COLUMN)
        ++sDevKeyCut;                               /* a row the column cannot hold (none of FireRed's menus): loud */
    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r;
        int look = i == DevBoxExitKey(b) ? DEV_LOOK_DARK : DEV_LOOK_KEY, sunk, faceY;

        if (!DevBoxKeyRect(b, i, &r))
            continue;
        sunk = DevChosen(i, b->menuCursor, held);
        faceY = DevKey(&r, look, sunk, 6);
        if (b->state == PT_BOX_STATE_MARKS && i < 4) {
            DevBoxMark(i, r.x + (r.w - 16) / 2, faceY + (r.h - 16) / 2, (b->marks >> i) & 1 ? DEV_KEY_INK : DEV_INK_DIM);
            continue;
        }
        if (b->state == PT_BOX_STATE_MENU)
            DevBoxUpper(b->menu[i], (int)sizeof(b->menu[i]), word, (int)sizeof(word));
        else if (b->state == PT_BOX_STATE_YESNO)
            DevUpper(word, CtrUiText(i ? UIS_NO : UIS_YES), sizeof(word));
        else if (i == 4)
            DevUpper(word, CtrUiText(UIS_OK), sizeof(word));
        else
            snprintf(word, sizeof(word), "%s", DevCancelWord());
        DevKeyWordAt(&r, faceY, look, sunk, word, look == DEV_LOOK_KEY && b->state != PT_BOX_STATE_YESNO ? 10 : 0);
    }
}

/* The chooser: ◀ ▶ (sunk while held) and the chosen box's key in the hand's place, always sunk: its name as typed and
 * how many it holds, centred down it. */
static void DevBoxChooser(const CtrPokeTouchBox *b, int press)
{
    const CtrPokeTouchRect *r = &kDevBoxChoose;
    int faceY = DevKey(r, DEV_LOOK_KEY, 1, 6), w;
    char text[DEV_BOX_TEXT];

    DevArrowKeys(&kDevBoxPrev, &kDevBoxNext, 1, 1, press == ST_HIT_PREV, press == ST_HIT_NEXT, 1);
    DevBoxText(b->chooseName, (int)sizeof(b->chooseName), text, (int)sizeof(text));
    w = DevTextWidth(text);
    DevTextInk(text, r->x + (r->w - w) / 2, faceY + r->h / 2 - 14, r->x + r->w - 4, DEV_SUNK_INK, DEV_SUNK_INK_SH);
    snprintf(text, sizeof(text), "%u/%u", (unsigned)b->chooseCount, (unsigned)PT_BOX_SLOTS);
    w = DevSmallTextWidth(text);
    DevSmallText(text, r->x + (r->w - w) / 2, faceY + r->h / 2 + 6, DEV_SUNK_INK, r->x + r->w - 4);
}

/* The box whole (after DevBegin: the dispatcher's). A held touch chooses among the box's own pieces only while the box
 * takes it (BOX); a menu's keys, the chooser's ◀ ▶ take theirs. In every state but BOX and BUSY (FireRed's cursor
 * walking, a fade) the box's icons but the cursor's place's are faded, never the box dimmed: its paper, name and frame
 * at full strength. */
static void DevBoxDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBox *b = &v->box;
    int press = DevBoxPress(b), cursor = DevBoxCursor(b), boxPress = b->state == PT_BOX_STATE_BOX ? press : -1;
    int message = b->state == PT_BOX_STATE_MESSAGE;
    int fade = b->state != PT_BOX_STATE_BOX && b->state != PT_BOX_STATE_BUSY;
    CtrPokeTouchRect paper;

    DevShell();
    paper = DevScreen(&kDevBoxScreen);
    if (b->area == ST_AREA_PARTY)
        DevBoxParty(b, cursor, boxPress, fade);
    else
        DevBoxPlaces(b, &paper, cursor, boxPress, fade);
    DevBoxNote(b);
    if (DevBoxKeyed(b)) {
        DevBoxMenu(b, press);
        if (DevBoxBack(b))                          /* YES / NO's BACK, a menu without CANCEL's CANCEL: FireRed's B */
            DevFieldBack(v, &kDevGridExit, b->state == PT_BOX_STATE_YESNO ? DevBackWord() : DevCancelWord());
        return;
    }
    if (b->state == PT_BOX_STATE_CHOOSE) {
        DevBoxChooser(b, press);
        DevFieldBack(v, &kDevGridExit, DevCancelWord());
        return;
    }
    DevBoxSide(b, cursor, boxPress, message || (b->state == PT_BOX_STATE_BOX && b->area == ST_AREA_PARTY));
    DevBoxHand(b);
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* What of `b` is drawn: the box but its number and its wallpaper, the press as drawn, the state's keys (a menu's rows and
 * whether it ends in CANCEL) and the chooser only in their states, the Pokemon in hand while it is, the Pokemon's lines
 * while the footer or the hand shows them, its types while the footer's line 2 does (not MOVE ITEMS, not an egg), MOVE
 * ITEMS' item while the footer shows it. */
static void DevBoxDrawn(const CtrPokeTouchBox *b, CtrPokeTouchBox *out)
{
    int keys = DevBoxKeyed(b), lines = b->message[0] == 0xff && b->mon.species;
    int hand = !keys && b->state != PT_BOX_STATE_CHOOSE && b->holding && !(b->items && b->handItem);

    memcpy(out, b, sizeof(*out));
    out->box = 0;
    out->wallpaper = 0;
    out->pressed = (int16_t)DevBoxPress(b);
    if (!keys) {
        out->menuCount = 0;
        out->menuCursor = 0;
    }
    if (b->state != PT_BOX_STATE_MENU) {
        memset(out->menu, 0, sizeof(out->menu));
        out->menuCancel = 0;
    }
    if (b->state != PT_BOX_STATE_MARKS)
        out->marks = 0;
    if (b->state != PT_BOX_STATE_CHOOSE) {
        memset(out->chooseName, 0, sizeof(out->chooseName));
        out->chooseCount = 0;
    }
    if (!b->holding)
        memset(&out->held, 0, sizeof(out->held));
    if (!lines && !hand)
        memset(out->monText, 0, sizeof(out->monText));
    if (!lines)
        out->monItem = 0;
    if (!lines || b->items || b->mon.egg) {
        memset(out->monType, 0, sizeof(out->monType));
        memset(out->monTypeName, 0, sizeof(out->monTypeName));
    }
    if (!lines || !b->items || !b->monItem) {
        memset(out->itemName, 0, sizeof(out->itemName));
        memset(out->itemDesc, 0, sizeof(out->itemDesc));
    }
}

/* Nothing the box draws differs between `a` and `b` (the shell is the dispatcher's): the box as drawn, and BACK as
 * drawn whenever the box shows one (DevBoxBack), so a held BACK redraws. */
static int DevBoxSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    CtrPokeTouchBox p, q;

    DevBoxDrawn(&a->box, &p);
    DevBoxDrawn(&b->box, &q);
    return !memcmp(&p, &q, sizeof(p)) && (!DevBoxBack(&b->box) || DevFieldBackSame(a, b));
}

/* BACK in the grid's exit (FireRed's B, DevBoxBack): the chooser's CANCEL, YES / NO's BACK, a menu without CANCEL's
 * CANCEL; elsewhere none: CLOSE is FireRed's exit, a menu's CANCEL row and the marks' CANCEL are FireRed's own. */
static const CtrPokeTouchRect *DevBoxBackKey(const CtrPokeTouchView *v)
{
    return DevBoxBack(&v->box) ? &kDevGridExit : NULL;
}
