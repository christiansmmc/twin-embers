/* The battle Bag and the battle Party in the ember "Llama" look (included in 3ds_poketouch.c, whose drawing helpers and
 * design system it uses; the owner's mockup, llama2_mockup.html; the pieces are 3ds_ember2_impl.h's Ll*): the dark ember
 * backdrop, raised glass cards (sky blue for the party, flame for what is chosen, slate for CANCEL), FireRed's text
 * window for the notes, its red cursor and slanted red ribbons. The rest of the battle's lower screen (the commands,
 * the moves, the targets, the Safari Zone, idle) is the red device's, 3ds_device_battle_impl.h, which owns the public
 * CtrBattleUi_Draw / Update / Hit and hands these two states to BEmberDraw / BEmberUpdate / BEmberHit; it also uses
 * the item and party icon caches here. Everything shown is what the controller reports. A card is drawn in a rectangle
 * of its own plus the 2 px its select ring and shadow use (the canvas redraws only what changed). */

#include "3ds_icons.h"
#include "3ds_text_case.h"
#include "3ds_ui_strings.h"
#include "3ds_party_grid.h"

/* The battle Bag: its title (the Items pocket, the only one it shows) and BERRIES for the Berry Pouch, the item list
 * on the left with its scroll arrows, the selected item's card on the right, the description's note and CANCEL. */
static const CtrPokeTouchRect sBTitle = {6, 4, 204, 26}, sBTitleWide = {6, 4, 308, 26}, sBBerries = {214, 4, 100, 26};
static const CtrPokeTouchRect sBList = {6, 36, 190, 138};
static const CtrPokeTouchRect sBDetail = {200, 36, 114, 138};
static const CtrPokeTouchRect sBUse = {210, 136, 94, 30};
static const CtrPokeTouchRect sBUpArrow = {176, 36, 20, 66}, sBDownArrow = {176, 108, 20, 66};
/* The Bag's text window and CANCEL. */
static const CtrPokeTouchRect sBNote = {4, 176, 236, 60};
static const CtrPokeTouchRect sBBagCancel = {244, 180, 72, 52};
#define B_DEPTH 3

static CtrPokeTouchRect BRow(int i)
{
    CtrPokeTouchRect r = {6, (int16_t)(36 + 28 * i), 166, 26};
    return r;
}

static CtrPokeTouchRect BMenuRow(int i)
{
    CtrPokeTouchRect r = {208, (int16_t)(84 + 30 * i), 98, 27};
    return r;
}
/* The Party (in battle and on the field): the six Pokemon as sky-blue cards in two columns of three on the embers (the mockup's 150 x 48
 * and the lip), and under them the text window with what the Party says and CANCEL. */
static const CtrPokeTouchRect sBPNote = {7, 169, 228, 65};
static const CtrPokeTouchRect sBPCancel = {240, 170, 72, 63};
/* Picking several Pokemon: CONFIRM over a shorter CANCEL. */
static const CtrPokeTouchRect sBPConfirm = {240, 170, 72, 30}, sBPCancelLow = {240, 203, 72, 30};
#define BP_DEPTH 3
#define BP_CARD_H 51

/* A slot's card: two columns of three, the slots reading left to right and down, as the Party's D-pad walks them (the
 * patched UpdatePartySelectionGrid). Vertically the cards are 2 px apart: the select ring of one fills the gap. */
static CtrPokeTouchRect BPanel(const CtrBattleParty *p, int i)
{
    CtrPokeTouchRect r = {(int16_t)(i & 1 ? 162 : 8), (int16_t)(6 + 53 * (i >> 1)), 150, BP_CARD_H};

    (void)p;
    return r;
}

/* A row of the Party's menu: one column of up to four, two columns beyond. Which column and row an action is in is
 * 3ds_party_grid.h's, the same arithmetic the game's D-pad walks it by (recipe 0257). */
static CtrPokeTouchRect BPMenuRow(int count, int i)
{
    int cols = CtrPartyGrid_Cols(count), rows = CtrPartyGrid_Rows(count), col = CtrPartyGrid_Col(count, i), row = CtrPartyGrid_Row(count, i);
    CtrPokeTouchRect r = {(int16_t)(cols == 2 ? 44 + 118 * col : 100), (int16_t)(88 - 15 * rows + 30 * row),
                          (int16_t)(cols == 2 ? 112 : 120), 27};
    return r;
}

static int BIn(const CtrPokeTouchRect *r, int x, int y)
{
    return UiIn(r, x, y);
}

/* The row of the Bag's selected item when it is on screen (USE stands for it), else -1. */
static int BBagSelRow(const CtrBattleBag *b)
{
    int i = b->cursor - b->first;

    return i >= 0 && i < BUI_BAG_ROWS && b->row[i].item ? i : -1;
}

/* What the battle Party or Bag shows under a point (CtrBattleUi_Hit's for those two states). */
static int BEmberHit(const CtrBattleView *v, int x, int y)
{
    if (v->state == BUI_PARTY) {
        const CtrBattleParty *p = &v->pty;

        if (p->panel == BUI_PTY_TEXT || p->panel == BUI_PTY_LEVELUP)
            return BUI_PTY_MESSAGE;                         /* anywhere: the message waits for A */
        if (p->panel == BUI_PTY_ACTIONS || p->panel == BUI_PTY_YESNO) {
            int n = p->menuCount > 8 ? 8 : p->menuCount;

            for (int i = 0; i < n; ++i) {
                CtrPokeTouchRect r = BPMenuRow(n, i);
                if (BIn(&r, x, y))
                    return BUI_PTY_MENU + i;
            }
            return -1;
        }
        if (p->mode == BUI_PTY_MULTI && BIn(&sBPConfirm, x, y))
            return BUI_PTY_CONFIRM;
        if (p->mode != BUI_PTY_FORCED && BIn(p->mode == BUI_PTY_MULTI ? &sBPCancelLow : &sBPCancel, x, y))
            return BUI_PTY_CANCEL;
        for (int i = 0; i < 6; ++i) {
            CtrPokeTouchRect r = BPanel(p, i);
            if (p->present[i] && BIn(&r, x, y))
                return BUI_PTY_SLOT + i;
        }
        return -1;
    }
    if (v->state == BUI_BAG) {
        const CtrBattleBag *b = &v->bag;

        if (b->message[0])
            return BUI_BAG_MESSAGE;                         /* anywhere: the message waits for A */
        if (BIn(&sBBagCancel, x, y))
            return BUI_BAG_CANCEL;
        if (b->berries && BIn(&sBBerries, x, y))
            return BUI_BAG_BERRIES;
        if (b->menuCount > 0 && BIn(&sBDetail, x, y)) {
            for (int i = 0; i < b->menuCount && i < 3; ++i) {
                CtrPokeTouchRect r = BMenuRow(i);
                if (BIn(&r, x, y))
                    return BUI_BAG_MENU + i;
            }
            return -1;
        }
        if (b->menuCount == 0 && BBagSelRow(b) >= 0 && BIn(&sBUse, x, y))
            return BUI_BAG_ROW + BBagSelRow(b);             /* USE: the selected row again, which uses it */
        if (b->count > BUI_BAG_ROWS) {
            /* The arrows are their 22 px buttons; between them the rail is a slider. */
            CtrPokeTouchRect up = {sBUpArrow.x, sBUpArrow.y, sBUpArrow.w, 22};
            CtrPokeTouchRect down = {sBDownArrow.x, sBDownArrow.y + sBDownArrow.h - 22, sBDownArrow.w, 22};
            CtrPokeTouchRect rail = {sBUpArrow.x, sBUpArrow.y + 22, sBUpArrow.w, sBDownArrow.y + sBDownArrow.h - 22 - (sBUpArrow.y + 22)};

            if (BIn(&up, x, y))
                return b->first > 0 ? BUI_BAG_UP : -1;
            if (BIn(&down, x, y))
                return b->first + BUI_BAG_ROWS < b->count ? BUI_BAG_DOWN : -1;
            if (BIn(&rail, x, y))
                return BUI_BAG_RAIL;
        }
        for (int i = 0; i < BUI_BAG_ROWS; ++i) {
            CtrPokeTouchRect r = BRow(i);
            if (b->row[i].item && BIn(&r, x, y))
                return BUI_BAG_ROW + i;
        }
    }
    return -1;
}

/* Running text wrapped to `width` pixels, `maxLines` at most (FireRed's line breaks are spaces here): the last line ends in "..." when
 * there is more (Em2Wrap). */
static void BDescribeSh(const char *text, int x, int y, int width, int maxLines, uint16_t color, uint16_t shadow)
{
    Em2Wrap(text, x, y, width, maxLines, UI_LINE_H, color, shadow);
}

static void BDescribeN(const char *text, int x, int y, int width, int maxLines, uint16_t color)
{
    BDescribeSh(text, x, y, width, maxLines, color, UIS_TEXT_SH);
}


/* The PP colour FireRed's move list uses: the usual ink, then yellow at half, orange at a quarter, red at none. */
static uint16_t BPpColor(unsigned pp, unsigned max)
{
    return pp == 0 ? UI_HP_LOW : pp * 4 <= max ? UI_WARN : pp * 2 <= max ? UI_HP_MID : UI_TEXT;
}


/* The battle Party: a header, the six Pokemon in two columns of three, CANCEL. */
extern int CtrBottomParty_Icon(int slot, unsigned char *tilesOut, unsigned short *palOut);
/* Party icons: FireRed's 32x32 icon of a slot with both its frames (the second under the first), decoded once per
 * species and slot. */
static struct {
    unsigned icon;
    int ok;
    uint8_t pixels[32 * 64];
    uint16_t pal[16];
} sBPIcon[6];

static int BPartyIcon(int slot, unsigned icon)
{
    static unsigned char tiles[1024];
    static unsigned short pal[16];

    if (sBPIcon[slot].icon == icon + 1)
        return sBPIcon[slot].ok;
    memset(tiles, 0, sizeof(tiles));
    sBPIcon[slot].icon = icon + 1;
    sBPIcon[slot].ok = CtrBottomParty_Icon(slot, tiles, pal);
    if (!sBPIcon[slot].ok)
        return 0;
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 32; ++x) {
            unsigned byte = tiles[(y / 8 * 4 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2];
            sBPIcon[slot].pixels[y * 32 + x] = (uint8_t)((byte >> (x & 1) * 4) & 15);
        }
    for (int i = 0; i < 16; ++i) {
        unsigned c = pal[i];
        sBPIcon[slot].pal[i] = GbaColor(c);
    }
    return 1;
}

/* Text wrapped into `maxLines` lines of `width`, in `ink` over `shadow`, 16 apart; the last ends in "..." when there is more. */
static void BPWrap(const char *text, int x, int y, int width, int maxLines, uint16_t ink, uint16_t shadow)
{
    Em2Wrap(text, x, y, width, maxLines, 16, ink, shadow);
}

/* The selected slot's icon hops with its frames, as FireRed's Party does: up on the first, down on the second. */
static int BPartyBounce(const CtrBattleParty *p, int i)
{
    return p->cursor == i ? (p->iconFrame[i] & 2 ? 1 : -3) : 0;
}

/* ---- the Llama pieces the battle screens share ---- */

/* A raised card of the battle: the backdrop back over its touch rectangle and 2 px round it, then its select ring while the controller's
 * cursor is on it (the mockup's .c.sel) or else, when `shadow`, the shade under its lip; `h` is the height drawn (the 3 px lip included, at
 * most the rectangle's). Returns where the face starts: a pressed card sinks onto its lip. */
static int BLCard(const CtrPokeTouchRect *r, int h, const EmPal *p, int focused, int pressed, int shadow)
{
    TsBgRect(r->x - 2, r->y - 2, r->w + 4, r->h + 4);
    if (focused)
        LlSelectRing(r->x, r->y, r->w, h);
    else if (shadow && !pressed)
        Em2CardShadow(r->x, r->y, r->w, h);
    return EmButton(r->x, r->y, r->w, h, p, B_DEPTH, pressed);
}

/* The faint ring of light behind a card's art (the mockup's border of 8 or 10 px in white at 10 to 14 percent, a circle that the card clips): over
 * the card's face (x, y, w, h, outline included) the pixels whose centre is between rIn and rOut from (cx, cy), `a`/255 of white. */
static void BRingDecor(int x, int y, int w, int h, int cx, int cy, int rIn, int rOut, int a)
{
    int x0 = cx - rOut > x + 1 ? cx - rOut : x + 1, x1 = cx + rOut < x + w - 1 ? cx + rOut : x + w - 1;
    int y0 = cy - rOut > y + 1 ? cy - rOut : y + 1, y1 = cy + rOut < y + h - 1 ? cy + rOut : y + h - 1;
    int in2 = 4 * rIn * rIn, out2 = 4 * rOut * rOut;

    for (int py = y0; py < y1; ++py) {
        int dy = 2 * py + 1 - 2 * cy, row = py - y, cutY = LlCut(row, h);

        for (int px = x0; px < x1; ++px) {
            int dx = 2 * px + 1 - 2 * cx, d2 = dx * dx + dy * dy, col = px - x;

            if (d2 < in2 || d2 >= out2 || col < 1 + cutY || col > w - 2 - cutY)
                continue;
            Px(px, py, Em2Lerp(TsGet(px, py), EM_WHITE, a, 255));
        }
    }
}

/* A slot with nothing in it: a hollow in the backdrop (the mockup's rgba(20,0,0,.35) with a pale hairline and the shade of its top edge), `word`
 * dim in the middle when there is one. */
static void BHollow(const CtrPokeTouchRect *r, int h, const char *word)
{
    TsBgRect(r->x - 2, r->y - 2, r->w + 4, r->h + 4);
    for (int row = 0; row < h; ++row) {
        int cut = LlCut(row, h);

        for (int col = cut; col < r->w - cut; ++col) {
            int cx = r->x + col, cy = r->y + row;
            uint16_t c = UiMix(TsGet(cx, cy), UI_RGB(20, 0, 0), 3);

            if (row < 2)
                c = UiMix(c, UI_RGB(0, 0, 0), 2);
            if (row == 0 || row == h - 1 || col == cut || col == r->w - cut - 1)
                c = UiMix(c, UI_RGB(255, 200, 180), 1);
            Px(cx, cy, c);
        }
    }
    if (word)
        UiText(word, r->x + (r->w - UiTextWidth(word, 1)) / 2, r->y + (h - UI_TEXT_H) / 2, UI_RGB(176, 128, 120), UI_RGB(52, 12, 12), 1, r->x + r->w - 4);
}

static const char *const sBPartyTagText[5] = {"", CTR_TR("Able", "Puede", "Kann"), CTR_TR("Not able", "No puede", "Nein"), CTR_TR("Learned", "Aprendido", "Kennt Att."), CTR_TR("No use", "No sirve", "Nutzlos")};

/* The width of the card's verdict badge (0: none), so the status badge beside it leaves it room. */
static int BPartyTagWidth(const CtrBattleParty *p, int i)
{
    int tag = p->tag[i];

    if (tag <= BUI_PTY_TAG_NONE || tag > BUI_PTY_TAG_NO_USE || p->egg[i] || p->mode == BUI_PTY_MULTI)
        return 0;
    return UiTextWidth(sBPartyTagText[tag], 1) + 12;
}

/* The card's verdict on the item or move being taught, as a badge at the right of the row where its HP numbers would be (cut with "..." where it
 * would run into the status badge, which ends at `left`). Returns whether there is one. */
static int BPartyTagBadge(const CtrBattleParty *p, int i, const CtrPokeTouchRect *r, int y, int left)
{
    static const uint16_t tint[5] = {0, UI_RGB(56, 168, 88), UI_RGB(112, 112, 128), UI_RGB(70, 132, 214), UI_RGB(112, 112, 128)};
    char text[24];
    int tag = p->tag[i], w = BPartyTagWidth(p, i), room = r->x + 144 - left;

    if (w == 0)
        return 0;
    Em2Fit(sBPartyTagText[tag], (room < w ? room : w) - 12, text, sizeof(text));
    w = UiTextWidth(text, 1) + 12;
    LlTypeBadge(r->x + 144 - w + 1, y, text, tint[tag]);
    return 1;
}

/* Whether the card holds a Pokemon that cannot be chosen now (a link battle's pick that is not able, or the item or move's verdict). */
static int BPartyBarred(const CtrBattleParty *p, int i)
{
    return (p->mode == BUI_PTY_MULTI && p->entry[i] == BUI_PTY_NOT_ABLE) || p->tag[i] == BUI_PTY_TAG_NOT_ABLE || p->tag[i] == BUI_PTY_TAG_NO_USE;
}

/* A Party card: a sky-blue card (red when the Pokemon fainted, slate when it cannot be chosen) with the icon on its ringed plate, name and level,
 * the HP capsule, a status badge and the numbers, a "1st" ribbon on the first and FireRed's ring of light behind the plate. An empty slot is a hollow. */
static void BPartyPanel(int i, const CtrBattleView *v)
{
    static const char *const code[7] = {"", CTR_TR("Psn", "Env", "Gft"), CTR_TR("Par", "Par", "Par"), CTR_TR("Slp", "Dor", "Slf"), CTR_TR("Frz", "Con", "Efr"), CTR_TR("Brn", "Que", "Brn"), CTR_TR("PKRS", "PKRS", "PKRS")};
    static const uint16_t tint[7] = {0, UI_RGB(170, 80, 190), UI_RGB(216, 168, 24), UI_RGB(120, 128, 150),
                                     UI_RGB(80, 170, 220), UI_RGB(226, 96, 48), UI_RGB(200, 90, 160)};
    const CtrBattleParty *p = &v->pty;
    CtrPokeTouchRect r = BPanel(p, i);
    int fainted = p->present[i] && !p->egg[i] && p->hp[i] == 0, selected = p->cursor == i;
    const EmPal *pal = fainted ? &kEmPartyRed : BPartyBarred(p, i) ? &kEmSlate : &kEmPartyBlue;
    int fy, fh = BP_CARD_H - B_DEPTH, y0, right = r.x + 144, levelLeft = right;
    char text[24];

    if (!p->present[i]) {
        BHollow(&r, BP_CARD_H, CTR_TR("- empty -", "- vac\xC3\xADo -", "- leer -"));
        return;
    }
    fy = BLCard(&r, BP_CARD_H, pal, selected, v->pressed == BUI_PTY_SLOT + i, 0);
    BRingDecor(r.x, fy, r.w, fh, r.x + 20, fy + 18, 30, 38, 36);
    LlPlate(r.x + 5, fy + 4, 40, pal->inkSh, UI_RGB(232, 248, 255), 1);
    y0 = fy + 7 + BPartyBounce(p, i);
    if (BPartyIcon(i, p->icon[i]))
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                uint8_t index = sBPIcon[i].pixels[(y + (p->iconFrame[i] & 1) * 32) * 32 + x];

                if (index)
                    Px(r.x + 9 + x, y0 + y, fainted ? Tone(sBPIcon[i].pal[index], 0) : sBPIcon[i].pal[index]);
            }
    if (i == 0 && p->mode != BUI_PTY_MULTI)                   /* the first, as FireRed's Party draws it big: its ribbon over the plate's foot */
        LlRibbonBox(r.x + 1, fy + 36, UiTextWidth(CTR_TR("1st", "1\xC2\xBA", "1."), 1) + 18, 11, 7, LL_BL, LL_RIB_TOP, LL_RIB_BOT, CTR_TR("1st", "1\xC2\xBA", "1."));
    if (!p->egg[i]) {
        snprintf(text, sizeof(text), CTR_TR("Lv%u", "Nv%u", "Lv%u"), p->level[i]);
        levelLeft = right - UiTextWidth(text, 1);
        UiTextRight(text, right, fy + 5, EM_WHITE, pal->inkSh, 1);
    }
    Em2Text(p->name[i], r.x + 52, fy + 5, EM_WHITE, pal->inkSh, levelLeft - 4);
    if (p->egg[i])
        return;
    LlHpBar(r.x + 52, fy + 19, 92, p->hp[i], p->maxHp[i]);
    {
        int badgeRight = r.x + 52;

        if (fainted || (p->status[i] >= 1 && p->status[i] <= 6)) {
            const char *name = fainted ? CTR_TR("Fnt", "Deb", "Bes") : code[p->status[i]];

            /* The verdict badge is the more important of the two: the condition's badge makes way where both would not fit. */
            if (52 + UiTextWidth(name, 1) + 12 + 4 + BPartyTagWidth(p, i) <= 145 || !BPartyTagWidth(p, i))
                badgeRight += LlTypeBadge(r.x + 52, fy + 32, name, fainted ? UI_RGB(192, 48, 40) : tint[p->status[i]]);
        }
        if (!BPartyTagBadge(p, i, &r, fy + 32, badgeRight + 4)) {
            snprintf(text, sizeof(text), "%u/%u", p->hp[i], p->maxHp[i]);
            UiTextRight(text, right, fy + 34, EM_WHITE, pal->inkSh, 1);
        }
    }
}

/* The select ring of the card under the Party's cursor, drawn again after another card's own redraw: the cards are only 2 px apart down the
 * columns, so the backdrop a card restores round itself takes the neighbour's ring with it. */
static void BPartyKeepRing(const CtrBattleView *v, int i)
{
    const CtrBattleParty *p = &v->pty;

    if (p->cursor >= 0 && p->cursor < 6 && p->cursor != i && p->present[p->cursor]) {
        CtrPokeTouchRect r = BPanel(p, p->cursor);

        LlSelectRing(r.x, r.y, r.w, BP_CARD_H);
    }
}

/* Picking several Pokemon: a slot's place in the order (1-3) on a flame disc at the foot of its icon plate. */
static void BPartyEntry(int i, const CtrBattleView *v)
{
    const CtrBattleParty *p = &v->pty;
    CtrPokeTouchRect r = BPanel(p, i);
    int fy = r.y + (v->pressed == BUI_PTY_SLOT + i ? BP_DEPTH : 0), place = p->entry[i] - BUI_PTY_FIRST;
    int cx = r.x + 11, cy = fy + 37;
    char digit[2] = {(char)('1' + place), 0};

    if (p->mode != BUI_PTY_MULTI || !p->present[i] || p->entry[i] < BUI_PTY_FIRST)
        return;
    UiDisc(cx, cy, 8, kEmFlame.edge);
    UiDisc(cx, cy, 7, kEmFlame.lip);
    UiDisc(cx, cy - 1, 6, kEmFlame.top);
    UiText(digit, cx - UiTextWidth(digit, 1) / 2, cy - 5, EM_WHITE, kEmFlame.inkSh, 1, cx + 8);
}

/* The round back arrow, a word under it: the label of a tall CANCEL (a slate button whose face starts at `fy`: the disc `discY` and the word
 * `wordY` below it). */
static void BBackLabel(const CtrPokeTouchRect *r, int fy, int discY, int wordY)
{
    LlBackDisc(r->x + (r->w - 24) / 2, fy + discY);
    EmLabel(CTR_TR("Cancel", "Cancelar", "Zur\xC3\xBC" "ck"), r->x, r->w, fy + wordY, UI_TEXT_H, &kEmSlate);
}

/* CANCEL: slate with its back disc, as BACK. Picking several, the shorter one under CONFIRM (flame), the select ring on the one the Party's
 * cursor is on. */
static void BPCancelButton(const CtrBattleView *v)
{
    int multi = v->pty.mode == BUI_PTY_MULTI, fy;
    const CtrPokeTouchRect *r = multi ? &sBPCancelLow : &sBPCancel;

    TsBgRect(sBPCancel.x - 2, sBPCancel.y - 2, sBPCancel.w + 4, sBPCancel.h + 4);
    if (multi) {
        if (v->pty.button == 0)
            LlSelectRing(sBPConfirm.x, sBPConfirm.y, sBPConfirm.w, sBPConfirm.h);
        if (v->pty.button == 1)
            LlSelectRing(r->x, r->y, r->w, r->h);
        fy = EmButton(sBPConfirm.x, sBPConfirm.y, sBPConfirm.w, sBPConfirm.h, &kEmFlame, BP_DEPTH, v->pressed == BUI_PTY_CONFIRM);
        EmLabel(CtrUiText(UIS_PTY_CONFIRM), sBPConfirm.x, sBPConfirm.w, fy, sBPConfirm.h - BP_DEPTH, &kEmFlame);
        fy = EmButton(r->x, r->y, r->w, r->h, &kEmSlate, BP_DEPTH, v->pressed == BUI_PTY_CANCEL);
        EmLabel(CTR_TR("Cancel", "Cancelar", "Zur\xC3\xBC" "ck"), r->x, r->w, fy, r->h - BP_DEPTH, &kEmSlate);
        return;
    }
    Em2CardShadow(r->x, r->y, r->w, r->h);
    fy = EmButton(r->x, r->y, r->w, r->h, &kEmSlate, BP_DEPTH, v->pressed == BUI_PTY_CANCEL);
    BBackLabel(r, fy, 7, 37);
}

/* The text window of the mockup under the cards: what the Party says (its question or hint), or, with no CANCEL, that one must be sent out, a
 * hairline and, dim, how to answer. */
static void BPNoteCard(const CtrBattleView *v)
{
    const CtrPokeTouchRect *n = &sBPNote;
    const char *text = v->pty.caption[0] ? v->pty.caption
                     : v->pty.mode == BUI_PTY_FORCED ? CTR_TR("A Pok\xC3\xA9mon must be sent out.", "Tienes que sacar un Pok\xC3\xA9mon.", "Ein Pok\xC3\xA9mon muss antreten.")
                     : v->pty.mode == BUI_PTY_TARGET ? CTR_TR("Use on which one?", "\xC2\xBF" "En cu\xC3\xA1l lo usas?", "Auf welches anwenden?") : CTR_TR("Choose a Pok\xC3\xA9mon.", "Elige un Pok\xC3\xA9mon.", "W\xC3\xA4hle ein Pok\xC3\xA9mon.");

    TsBgRect(n->x - 2, n->y - 2, n->w + 4, n->h + 4);
    LlWindow(n->x, n->y, n->w, n->h);
    Em2Text(text, n->x + 13, n->y + 11, EM_DARK, EM_DARK_SH, n->x + n->w - 14);
    LlRule(n->x + 13, n->y + 26, n->w - 26);
    Em2Wrap(CTR_TR("Tap a card or use the D-pad and A.", "Toca una tarjeta o usa la cruceta y A.", "Tippe eine Karte an oder nutze Kreuz und A."),
            n->x + 13, n->y + 31, n->w - 26, 2, 14, EM_DIM, EM_DARK_SH);
}

/* A message in FireRed's text window over the dimmed screen, with "tap to continue" and its arrow (the Party's and the Bag's). */
static void BMessageWindow(const char *message)
{
    CtrPokeTouchRect all = {0, 0, W, H}, box = {20, 70, 280, 76};
    const char *tap = CTR_TR("Tap to continue", "Toca para seguir", "Tippen zum Fortfahren");

    UiDim(&all);
    LlWindow(box.x, box.y, box.w, box.h);
    Em2Wrap(message, box.x + 14, box.y + 11, box.w - 28, 3, 14, EM_DARK, EM_DARK_SH);
    UiText(tap, box.x + box.w - 30 - UiTextWidth(tap, 1), box.y + box.h - 19, EM_DIM, EM_DARK_SH, 1, box.x + box.w - 18);
    LlArrow(box.x + box.w - 26, box.y + box.h - 18, 8, 5, 1, LL_CURSOR);
}

/* What the Party has open over its slots, over the dimmed Party: its menu (the selected row flame with the select ring, the last slate), or
 * the message it waits on in the text window. */
static void BPartyOverlay(const CtrBattleView *v)
{
    const CtrBattleParty *p = &v->pty;
    CtrPokeTouchRect all = {0, 0, W, H};

    if (p->panel == BUI_PTY_TEXT) {
        BMessageWindow(p->message);
    } else if (p->panel == BUI_PTY_ACTIONS || p->panel == BUI_PTY_YESNO) {
        int n = p->menuCount > 8 ? 8 : p->menuCount;

        UiDim(&all);
        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r = BPMenuRow(n, i);
            const EmPal *pal = p->menuCursor == i ? &kEmFlame : i == n - 1 ? &kEmSlate : &kEmCream;
            int fy;

            if (p->menuCursor == i)
                LlSelectRing(r.x, r.y, r.w, r.h);
            fy = EmButton(r.x, r.y, r.w, r.h, pal, BP_DEPTH, v->pressed == BUI_PTY_MENU + i);
            EmLabel(p->menuName[i], r.x, r.w, fy, r.h - BP_DEPTH, pal);
        }
    }
}

/* A Rare Candy's level up (BUI_PTY_LEVELUP): over the slots FireRed's text window, its flame header with the Pokemon's icon, name and
 * "LvN > LvN+1", and the six stats in sunk wells, the new value and its gain in green; the message in the text window and OK, flame, in
 * CANCEL's place. Anywhere on the screen goes on (A). */
static void BPLevelUp(const CtrBattleView *v)
{
    static const char *const sLabels[6] = {CTR_TR("Max HP", "PS m\xC3\xA1x.", "Max. KP"), CTR_TR("Attack", "Ataque", "Angriff"), CTR_TR("Defense", "Defensa", "Vert."), CTR_TR("Sp. atk", "At. esp.", "Sp. Ang."), CTR_TR("Sp. def", "Def. esp.", "Sp. Ver."), CTR_TR("Speed", "Velocidad", "Init.")};
    const CtrBattleParty *p = &v->pty;
    const EmPal *flame = &kEmFlame;
    const CtrPokeTouchRect *n = &sBPNote, *ok = &sBPCancel;
    int slot = p->lvSlot < 6 ? p->lvSlot : 0, fy, x;
    char now[8], before[8];

    LlWindow(6, 6, 308, 160);
    fy = EmButton(12, 12, 296, 36, flame, BP_DEPTH, 0);
    if (BPartyIcon(slot, p->icon[slot]))
        for (int y = 2; y < 31; ++y)
            for (int ix = 0; ix < 32; ++ix) {
                uint8_t index = sBPIcon[slot].pixels[y * 32 + ix];
                if (index)
                    Px(18 + ix, fy - 1 + y, sBPIcon[slot].pal[index]);
            }
    UiText(p->name[slot], 56, fy + (33 - UI_TEXT_H) / 2, flame->ink, flame->inkSh, 1, 170);
    snprintf(now, sizeof(now), CTR_TR("Lv%u", "Nv%u", "Lv%u"), p->lvLevel);
    snprintf(before, sizeof(before), CTR_TR("Lv%u", "Nv%u", "Lv%u"), p->lvLevel > 1 ? p->lvLevel - 1 : 1);
    x = 298 - UiTextWidth(now, 1);
    UiText(now, x, fy + (33 - UI_TEXT_H) / 2, flame->ink, flame->inkSh, 1, 304);
    LlArrow(x - 12, fy + 12, 6, 10, 3, flame->ink);
    x -= 20 + UiTextWidth(before, 1);
    UiText(before, x, fy + (33 - UI_TEXT_H) / 2, UiMix(flame->ink, flame->bottom, 3), flame->inkSh, 1, x + 40);
    for (int i = 0; i < 6; ++i) {
        int cx = 14 + (i & 1) * 150, cy = 54 + (i >> 1) * 36;
        char value[8], gain[8];

        LlWell(cx, cy, 142, 30);
        UiText(sLabels[i], cx + 8, cy + (30 - UI_TEXT_H) / 2, EM_DARK, EM_DARK_SH, 1, cx + 80);
        snprintf(value, sizeof(value), "%u", p->lvStat[i]);
        UiTextRight(value, cx + 102, cy + (30 - UI_TEXT_H) / 2, EM_DARK, EM_DARK_SH, 1);
        snprintf(gain, sizeof(gain), "%+d", p->lvGain[i]);
        UiTextRight(gain, cx + 134, cy + (30 - UI_TEXT_H) / 2, UI_RGB(32, 144, 72), UI_RGB(196, 236, 204), 1);
    }
    LlWindow(n->x, n->y, n->w, n->h);
    Em2Wrap(p->message, n->x + 13, n->y + 10, n->w - 26, 3, 14, EM_DARK, EM_DARK_SH);
    Em2CardShadow(ok->x, ok->y, ok->w, ok->h);
    fy = EmButton(ok->x, ok->y, ok->w, ok->h, flame, BP_DEPTH, v->pressed == BUI_PTY_MESSAGE);
    EmLabel("OK", ok->x, ok->w, fy, ok->h - BP_DEPTH, flame);
}

static void BParty(const CtrBattleView *v)
{
    TsBackdrop();
    if (v->pty.panel == BUI_PTY_LEVELUP) {
        BPLevelUp(v);
        return;
    }
    for (int i = 0; i < 6; ++i) {
        BPartyPanel(i, v);
        BPartyEntry(i, v);
        BPartyKeepRing(v, i);
    }
    BPNoteCard(v);
    if (v->pty.mode != BUI_PTY_FORCED)
        BPCancelButton(v);
    BPartyOverlay(v);
}

/* Item icons: FireRed's 24x24 icon of an item, decoded once into a collision-free 12-slot FIFO cache. */
#define BICONS 12
static struct {
    unsigned item;
    int ok;
    uint8_t pixels[24 * 24];
    uint16_t pal[16];
} sBIcon[BICONS];
static int sBIconNext;

static int BItemIcon(unsigned item)
{
    static unsigned char tiles[0x200];
    static unsigned short pal[16];
    int slot = -1;

    if (item == 0)
        return -1;
    for (int i = 0; i < BICONS; ++i)
        if (sBIcon[i].item == item)
            return sBIcon[i].ok ? i : -1;
    for (int i = 0; i < BICONS; ++i)
        if (sBIcon[i].item == 0) {
            slot = i;
            break;
        }
    if (slot < 0) {
        slot = sBIconNext;
        sBIconNext = (sBIconNext + 1) % BICONS;
    }
    memset(tiles, 0, sizeof(tiles));
    sBIcon[slot].item = item;
    sBIcon[slot].ok = CtrPokeTouch_ItemIcon(item, tiles, pal);
    if (!sBIcon[slot].ok)
        return -1;
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 24; ++x) {
            unsigned byte = tiles[(y / 8 * 3 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2];
            sBIcon[slot].pixels[y * 24 + x] = (uint8_t)((byte >> (x & 1) * 4) & 15);
        }
    for (int i = 0; i < 16; ++i) {
        unsigned c = pal[i];
        sBIcon[slot].pal[i] = GbaColor(c);
    }
    return slot;
}

/* An item's icon at `scale` times its size, its top-left corner at (x, y). */
static void BItemDraw(unsigned item, int x, int y, int scale)
{
    int slot = BItemIcon(item);

    if (slot >= 0)
        UiImage(sBIcon[slot].pixels, sBIcon[slot].pal, 24, 24, 24, x, y, scale, 1);
}


/* The ember surface: PokeTouch's red theme with the shadows and outlines of the light pieces (the Summary's too). */
static UiSurface sUiEmber;

static void EmSurface(void)
{
    sUiEmber = sUiRed;
    sUiEmber.textSh = EM_DARK_SH;
    sUiEmber.textDim = EM_DIM;
    sUiEmber.outline = UI_RGB(64, 72, 104);
    sUiEmber.track = UI_RGB(64, 72, 104);
    sUiS = &sUiEmber;
}

/* A pill with `text` on it, 15 pixels tall; BPillW is its width, x its left edge. */
UI_FN int BPillW(const char *text)
{
    return UiTextWidth(text, 1) + 14;
}

/* A sunk pill (the mockup's ipill): outline, a flat fill, a shade under its top edge and a light line under it. */
UI_FN void BPill(int x, int y, const char *text, uint16_t bg, uint16_t ink, uint16_t shadow)
{
    int w = BPillW(text);

    UiRoundFill(x, y, w, 15, 5, shadow);
    UiRoundFill(x + 1, y + 1, w - 2, 13, 4, bg);
    Fill(x + 3, y + 1, w - 6, 1, UiMix(bg, UI_RGB(0, 0, 0), 3));
    for (int cx = x + 4; cx < x + w - 4; ++cx)
        EmBlend(cx, y + 15, EM_WHITE, 56);
    UiText(text, x + 7, y + 3, ink, shadow, 1, x + w);
}

/* The Bag's CANCEL: slate (the mockup's .sl) with its round back arrow and the word under it; a tall one has the arrow and
 * the word as the mockup draws them, a short one only the word. */
static void BEmCancel(const CtrPokeTouchRect *r, int pressed)
{
    int fy;

    TsBgRect(r->x - 2, r->y - 2, r->w + 4, r->h + 4);
    if (!pressed)
        Em2CardShadow(r->x, r->y, r->w, r->h);
    fy = EmButton(r->x, r->y, r->w, r->h, &kEmSlate, B_DEPTH, pressed);
    if (r->h >= 40)
        BBackLabel(r, fy, r->h >= 70 ? 13 : 6, r->h >= 70 ? 47 : 34);
    else
        EmLabel(CTR_TR("Cancel", "Cancelar", "Zur\xC3\xBC" "ck"), r->x, r->w, fy, r->h - B_DEPTH, &kEmSlate);
}

/* ---- The battle Bag ---- */
/* Its title (the items: the balls have their button on the action screen) in flame and, with the Berry Pouch, BERRIES. */
static void BTabs(const CtrBattleView *v)
{
    const CtrPokeTouchRect *t = v->bag.berries ? &sBTitle : &sBTitleWide;
    int fy = EmButton(t->x, t->y, t->w, t->h, &kEmFlame, 2, 0);

    EmLabel(CTR_TR("Items", "Objetos", "Items"), t->x, t->w, fy, t->h - 2, &kEmFlame);
    if (v->bag.berries) {
        const CtrPokeTouchRect *r = &sBBerries;

        fy = EmButton(r->x, r->y, r->w, r->h, &kEmCream, 2, v->pressed == BUI_BAG_BERRIES);
        EmLabel(CTR_TR("Berries", "Bayas", "Beeren"), r->x, r->w, fy, r->h - 2, &kEmCream);
    }
}

/* A list row: the icon, the name and (but for key items) the quantity in a sunk pill, in blue as FireRed's "x12"; the Bag's selection flame with
 * FireRed's cursor and the select ring (the rows are 2 px apart: the ring fills the gap). */
static void BBagRow(int i, const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;
    CtrPokeTouchRect r = BRow(i);
    int chosen = b->first + i == b->cursor;
    const EmPal *p = chosen ? &kEmFlame : &kEmCream;
    int fy;

    if (!b->row[i].item)
        return;
    if (chosen)
        LlSelectRing(r.x, r.y, r.w, r.h);
    fy = EmButton(r.x, r.y, r.w, r.h, p, 2, v->pressed == BUI_BAG_ROW + i);
    BItemDraw(b->row[i].item, r.x + 3, fy, 1);
    if (chosen)
        LlCursor(r.x + 29, fy + 7, EM_WHITE);
    if (b->pocket != 1) {                       /* key items are one of each: no count */
        char qty[8];

        snprintf(qty, sizeof(qty), "x%u", b->row[i].qty > 999 ? 999 : b->row[i].qty);
        LlWell(r.x + r.w - 42, fy + 4, 34, 15);
        UiText(qty, r.x + r.w - 42 + (34 - UiTextWidth(qty, 1)) / 2, fy + 7, EM2_COUNT_INK, EM2_COUNT_SH, 1, r.x + r.w - 8);
    }
    Em2Text(b->row[i].name, r.x + (chosen ? 39 : 30), fy + 8, p->ink, p->inkSh, r.x + r.w - (b->pocket != 1 ? 46 : 8));
}

/* A scroll arrow: a small cream button with a red triangle (FireRed's), the triangle grey while the list cannot go that way. The button sits at the
 * top of the arrow's area going up, at its foot going down. */
/* The slider: the cursor index that puts the thumb, centred on y, where the finger is. */
int CtrBattleUi_BagRailTarget(const CtrBattleBag *b, int y)
{
    int railY = sBUpArrow.y + 22, railH = sBDownArrow.y + sBDownArrow.h - 22 - railY, left = b->count - BUI_BAG_ROWS, thumbH, first, t;

    if (left <= 0)
        return -1;
    thumbH = railH * BUI_BAG_ROWS / b->count;
    if (thumbH < 14)
        thumbH = 14;
    t = y - railY - thumbH / 2;
    t = t < 0 ? 0 : t > railH - thumbH ? railH - thumbH : t;
    first = (t * left * 2 + (railH - thumbH)) / (2 * (railH - thumbH));
    first = first > left ? left : first;
    t = first + (y < railY ? 0 : y >= railY + railH ? BUI_BAG_ROWS - 1 : (y - railY) * (BUI_BAG_ROWS - 1) / railH);
    return t > b->count - 1 ? b->count - 1 : t;
}

static void BBagArrow(const CtrPokeTouchRect *r, int down, int on)
{
    int fy = EmButton(r->x, down ? r->y + r->h - 20 : r->y, 20, 20, &kEmCream, 2, 0);

    LlArrow(r->x + 6, fy + (down ? 7 : 6), 8, 5, down ? 1 : 0, on ? LL_CURSOR : UI_RGB(184, 176, 168));
}

static void BBagList(const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;

    for (int i = 0; i < BUI_BAG_ROWS; ++i)
        BBagRow(i, v);
    if (b->count > BUI_BAG_ROWS) {
        int railY = sBUpArrow.y + 22, railH = sBDownArrow.y + sBDownArrow.h - 22 - railY, left = b->count - BUI_BAG_ROWS;
        int thumbH = railH * BUI_BAG_ROWS / b->count, thumbY;

        if (thumbH < 14)
            thumbH = 14;
        thumbY = (railH - thumbH) * (b->first < 0 ? 0 : b->first > left ? left : b->first) / left;
        BBagArrow(&sBUpArrow, 0, b->first > 0);
        BBagArrow(&sBDownArrow, 1, b->first + BUI_BAG_ROWS < b->count);
        LlScrollRail(sBUpArrow.x + 5, railY, 10, railH, thumbY, thumbH);
    }
}

/* The selected item's window: its icon on a plate, name, quantity and USE; with FireRed's menu open, the menu. */
static void BBagDetail(const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;
    const CtrPokeTouchRect *d = &sBDetail;
    CtrPokeTouchRect name = {(int16_t)(d->x + 7), 0, (int16_t)(d->w - 14), 13};
    char fit[BUI_ITEM_LEN + 4];

    LlWindow(d->x, d->y, d->w, d->h);
    if (b->menuCount > 0) {
        if (b->selItem) {
            BItemDraw(b->selItem, d->x + 9, d->y + 8, 1);
            Em2Text(b->selName, d->x + 37, d->y + 14, EM_DARK, EM_DARK_SH, d->x + d->w - 8);
        }
        for (int i = 0; i < b->menuCount && i < 3; ++i) {
            CtrPokeTouchRect r = BMenuRow(i);
            const EmPal *p = b->menuCursor == i ? &kEmFlame : i == b->menuCount - 1 ? &kEmSlate : &kEmCream;
            int fy;

            if (b->menuCursor == i)
                LlSelectRing(r.x, r.y, r.w, r.h);
            fy = EmButton(r.x, r.y, r.w, r.h, p, B_DEPTH, v->pressed == BUI_BAG_MENU + i);
            EmLabel(b->menuName[i], r.x, r.w, fy, r.h - B_DEPTH, p);
        }
        return;
    }
    if (!b->selItem) {
        Em2Fit(CTR_TR("Close the Bag", "Cerrar la mochila", "Beutel schlie\xC3\x9F" "en"), name.w, fit, sizeof(fit));
        UiTextCentered(fit, &name, d->y + 60, EM_DIM, EM_DARK_SH, 1);
        return;
    }
    LlPlate(d->x + d->w / 2 - 27, d->y + 7, 54, LL_SLATE, 0, 0);
    BItemDraw(b->selItem, d->x + d->w / 2 - 24, d->y + 10, 2);
    Em2Fit(b->selName, name.w, fit, sizeof(fit));
    UiTextCentered(fit, &name, d->y + 66, EM_DARK, EM_DARK_SH, 1);
    if (b->pocket != 1) {
        char qty[8];

        snprintf(qty, sizeof(qty), "x%u", b->selItem && BBagSelRow(b) >= 0 ? b->row[BBagSelRow(b)].qty : 0);
        if (BBagSelRow(b) >= 0)
            UiTextCentered(qty, &name, d->y + 81, EM2_COUNT_INK, EM2_COUNT_SH, 1);
    }
    if (BBagSelRow(b) >= 0) {
        int fy = EmButton(sBUse.x, sBUse.y, sBUse.w, sBUse.h, &kEmFlame, B_DEPTH, v->pressed == BUI_BAG_ROW + BBagSelRow(b));

        EmLabel(CTR_TR("Use", "Usar", "Benutzen"), sBUse.x, sBUse.w, fy, sBUse.h - B_DEPTH, &kEmFlame);
    }
}

/* The Bag's message over the dimmed Bag, in the text window (A, or a tap anywhere, goes on). */
static void BBagMessage(const CtrBattleView *v)
{
    BMessageWindow(v->bag.message);
}

static void BBag(const CtrBattleView *v)
{
    const CtrPokeTouchRect *n = &sBNote;

    TsBackdrop();
    BTabs(v);
    BBagList(v);
    BBagDetail(v);
    LlWindow(n->x, n->y, n->w, n->h);
    if (v->bag.selItem && v->bag.menuCount == 0)
        Em2Wrap(v->bag.selDesc, n->x + 13, n->y + 11, n->w - 26, 4, UI_LINE_H, EM_DIM, EM_DARK_SH);
    BEmCancel(&sBBagCancel, v->pressed == BUI_BAG_CANCEL);
    if (v->bag.message[0])
        BBagMessage(v);
}

/* The battle Party or Bag, whole (CtrBattleUi_Draw's for those two states). */
static void BEmberDraw(uint16_t *canvas, const CtrBattleView *v)
{
    sCanvas = canvas;
    EmSurface();
    if (v->state == BUI_PARTY)
        BParty(v);
    else
        BBag(v);
    sUiS = &sUiNavy;
}

/* Each changed battle element is presented through the shared dirty-rectangle path. */
static void BMark(CtrDirtyList *dirty, const CtrPokeTouchRect *r)
{
    CtrDirty_Add(dirty, r->x, r->y, r->x + r->w, r->y + r->h);
}

/* Whether two Party views differ only by `cursor` (with no modal open), so only the affected slot panels need redraw. */
static int BPartySlotsOnly(const CtrBattleParty *a, const CtrBattleParty *b)
{
    CtrBattleParty copy = *a;

    if (a->panel != BUI_PTY_NONE || b->panel != BUI_PTY_NONE)
        return 0;
    copy.cursor = b->cursor;
    /* A card's icon changes frames by itself (FireRed's icon animation): its own card is drawn again, not all of them. */
    memcpy(copy.iconFrame, b->iconFrame, sizeof(copy.iconFrame));
    return memcmp(&copy, b, sizeof(copy)) == 0;
}

/* Whether two Party views with the same menu or YES / NO open differ only by its cursor row and the row pressed. */
static int BPartyMenuOnly(const CtrBattleView *a, const CtrBattleView *b)
{
    CtrBattleView copy = *a;

    if (a->state != BUI_PARTY || b->state != BUI_PARTY || a->pty.panel != b->pty.panel
        || (b->pty.panel != BUI_PTY_ACTIONS && b->pty.panel != BUI_PTY_YESNO))
        return 0;
    copy.pty.menuCursor = b->pty.menuCursor;
    copy.pressed = b->pressed;
    memcpy(copy.pty.iconFrame, b->pty.iconFrame, sizeof(copy.pty.iconFrame));
    return memcmp(&copy, b, sizeof(copy)) == 0;
}

/* From one Party to the next, or one Bag to the next (CtrBattleUi_Update's for those): only what changed. */
static void BEmberUpdate(uint16_t *canvas, const CtrBattleView *old, const CtrBattleView *v, CtrDirtyList *dirty)
{
    CtrPokeTouchRect all = {0, 0, W, H};

    sCanvas = canvas;
    EmSurface();
    CtrDirty_Clear(dirty);
    if (v->state == BUI_PARTY) {
        /* The Party: a move of the selection or a press redraws its cards, anything else all of it. */
        CtrBattleView still = *old;

        memcpy(still.pty.iconFrame, v->pty.iconFrame, sizeof(still.pty.iconFrame));
        if (v->pty.panel != BUI_PTY_NONE && memcmp(&still, v, sizeof(still)) == 0) {
            /* Only the icons' frames: with a menu or a message over the cards they stay as they were (a full compose,
             * ~35 ms, every few frames for a hop nobody is looking at). */
        } else if (BPartyMenuOnly(old, v)) {
            /* A menu's cursor or press: the picture is composed in the canvas, but only the rows that changed are presented
             * (the whole screen written at once to the shown buffer tears). */
            int n = v->pty.menuCount > 8 ? 8 : v->pty.menuCount;

            BEmberDraw(canvas, v);
            EmSurface();
            for (int i = 0; i < n; ++i)
                if ((old->pty.menuCursor == i) != (v->pty.menuCursor == i)
                    || (old->pressed == BUI_PTY_MENU + i) != (v->pressed == BUI_PTY_MENU + i)) {
                    CtrPokeTouchRect r = BPMenuRow(n, i);

                    r = UiInset(&r, -3);
                    BMark(dirty, &r);
                }
        } else if (!BPartySlotsOnly(&old->pty, &v->pty) || old->pty.lead != v->pty.lead
            || ((old->pressed >= BUI_PTY_MENU && old->pressed < BUI_PTY_MESSAGE) != (v->pressed >= BUI_PTY_MENU && v->pressed < BUI_PTY_MESSAGE))
            || (v->pty.panel != BUI_PTY_NONE && old->pressed != v->pressed)) {
            BEmberDraw(canvas, v);
            EmSurface();
            BMark(dirty, &all);
        } else {
            for (int i = 0; i < 6; ++i)
                if ((old->pty.cursor == i) != (v->pty.cursor == i)
                    || old->pty.iconFrame[i] != v->pty.iconFrame[i]
                    || (old->pressed == BUI_PTY_SLOT + i) != (v->pressed == BUI_PTY_SLOT + i)) {
                    CtrPokeTouchRect r = BPanel(&v->pty, i);

                    r = UiInset(&r, -2);
                    BPartyPanel(i, v);
                    BPartyEntry(i, v);
                    BPartyKeepRing(v, i);
                    BMark(dirty, &r);
                }
            if (v->pty.mode != BUI_PTY_FORCED && ((old->pressed == BUI_PTY_CANCEL) != (v->pressed == BUI_PTY_CANCEL)
                                                 || (old->pressed == BUI_PTY_CONFIRM) != (v->pressed == BUI_PTY_CONFIRM))) {
                CtrPokeTouchRect r = UiInset(&sBPCancel, -2);

                BPCancelButton(v);
                BMark(dirty, &r);
            }
        }
    } else if (memcmp(&old->bag, &v->bag, sizeof(v->bag)) || old->pressed != v->pressed) {
        BEmberDraw(canvas, v);           /* the Bag: any change draws all of it */
        EmSurface();
        BMark(dirty, &all);
    }
    sUiS = &sUiNavy;
}
