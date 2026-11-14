/* The field Bag as a page (3ds_poketouch.h CtrPokeTouchBag; phase 2, the owner's "3"), included at the end of
 * 3ds_poketouch.c after the battle screen, whose BAG pieces (3ds_bag_page_impl.h) and item icons it shares. A remote for
 * FireRed's own Bag: the pockets as tabs (a case, a mart, the item PC or the mailbox: its title across), the list's rows
 * with each item's icon, name and count (a mart: its price; a key item on X or Y: that button), the scroll cells, the
 * selected item's description in the band (a TM/HM: its move's type beside it) and CANCEL. What the Bag has open goes
 * where the Party's does: its menu or YES / NO in the window at the bottom right with the band narrower, the quantity
 * box in that corner, a message in the whole band. Every row, cursor and count is the Bag's own (3ds_bottom.c walks
 * them with its keys). */
#include "3ds_ui_strings.h"

/* The old views' pieces that other field views still use until their turn (the box, the Pokedex, the choices, the
 * forget-a-move view, the naming keyboard). */
static const CtrPokeTouchRect sFBArea = {PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H};
#define FB_DEPTH 2

/* A light card over the dimmed list. */
static void FBCard(const CtrPokeTouchRect *r)
{
    UiRoundFill(r->x, r->y, r->w, r->h, 5, kEmCream.edge);
    EmGradFill(r->x + 1, r->y + 1, r->w - 2, r->h - 2, 4, kEmCream.top, kEmCream.bottom);
}

static void FBButton(const CtrPokeTouchRect *r, const EmPal *p, const char *label, int pressed)
{
    int fy = EmButton(r->x, r->y, r->w, r->h, p, FB_DEPTH, pressed);

    EmLabel(label, r->x, r->w, fy, r->h - FB_DEPTH, p);
}

/* ---- The page ---- */
static const CtrUiString sFBTabLabel[3] = {UIS_BAG_ITEMS, UIS_BAG_KEY_ITEMS, UIS_BAG_BALLS};

bool CtrPokeTouch_BagPageShown(const CtrPokeTouchView *v)
{
    return v->bag.on && !v->viewport;
}

static int FBMenuRows(const CtrPokeTouchBag *b)
{
    return b->state == PT_BAG_STATE_YESNO ? 2 : b->menuCount > PT_BAG_MENU_MAX ? PT_BAG_MENU_MAX : b->menuCount;
}

static int FBScrolls(const CtrPokeTouchBag *b)
{
    return b->count + 1 > PT_BAG_ROWS;
}

int CtrPokeTouch_BagHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchBag *b = &v->bag;

    if (!CtrPokeTouch_BagPageShown(v))
        return -1;
    switch (b->state) {
    case PT_BAG_STATE_MESSAGE:
        return PT_BAG_HIT_MESSAGE;                      /* anywhere: it waits for A */
    case PT_BAG_STATE_ACTIONS:
    case PT_BAG_STATE_YESNO:
        for (int i = 0; i < FBMenuRows(b); ++i) {
            CtrPokeTouchRect r = PartyMenuRow(FBMenuRows(b), i);

            if (UiIn(&r, x, y))
                return PT_BAG_HIT_MENU + i;
        }
        {
            CtrPokeTouchRect w = PartyMenuWindow(FBMenuRows(b));

            /* The menu: a tap off its window is B (FireRed closes it); YES / NO waits for one of its two. */
            return b->state == PT_BAG_STATE_ACTIONS && !UiIn(&w, x, y) ? PT_BAG_HIT_BACK : -1;
        }
    case PT_BAG_STATE_QUANTITY:
        return UiIn(&kBagQtyUp, x, y) ? PT_BAG_HIT_QTY_UP : UiIn(&kBagQtyDown, x, y) ? PT_BAG_HIT_QTY_DOWN
             : UiIn(&kBagQtyOk, x, y) ? PT_BAG_HIT_QTY_OK : UiIn(&kBagQtyCancel, x, y) ? PT_BAG_HIT_QTY_CANCEL : -1;
    default:
        break;
    }
    for (int i = 0; i < 3 && b->kind == PT_BAG_KIND_BAG; ++i)
        if (UiIn(&kBagTab[i], x, y))
            return PT_BAG_HIT_TAB + i;
    if (UiIn(&kBagCancel, x, y))
        return PT_BAG_HIT_BACK;
    if (FBScrolls(b)) {
        if (UiIn(&kBagUp, x, y))
            return b->first > 0 ? PT_BAG_HIT_UP : -1;
        if (UiIn(&kBagDown, x, y))
            return b->first + PT_BAG_ROWS <= b->count ? PT_BAG_HIT_DOWN : -1;
    }
    for (int i = 0; i < PT_BAG_ROWS; ++i) {
        CtrPokeTouchRect r = BagRow(i);

        if (b->first + i <= b->count && UiIn(&r, x, y))
            return PT_BAG_HIT_ROW + i;
    }
    return -1;
}

/* The pockets as tabs (the arrow on the one shown, on the tab held, or on the one the Bag is being walked to), or a
 * case's, a mart's or a PC's title across (a mart: the player's money at its right). */
static void FBHeader(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBag *b = &v->bag;

    if (b->kind == PT_BAG_KIND_BAG) {
        int held = b->pressed >= PT_BAG_HIT_TAB && b->pressed < PT_BAG_HIT_TAB + 3 ? b->pressed - PT_BAG_HIT_TAB : -1;

        for (int i = 0; i < 3; ++i)
            BagCell(&kBagTab[i], v->frameType, CtrUiText(sFBTabLabel[i]),
                    held >= 0 ? held == i : b->asked ? b->asked - 1 == i : b->pocket == i, held == i, 1);
        return;
    }
    {
        const CtrPokeTouchRect *t = &kBagTitle;
        int right = t->x + t->w - 8;

        Fill(t->x, t->y, t->w, t->h, BPageBg());
        KitPanel(t, KIT_FRAME_STD, v->frameType);
        if (b->kind == PT_BAG_KIND_SHOP) {
            char money[16];

            snprintf(money, sizeof(money), "\xc2\xa5%u", (unsigned)b->money);
            KitText(money, right - UiTextWidth(money, 1), t->y + 8, right);
            right -= UiTextWidth(money, 1) + 8;
        }
        KitText(b->title, t->x + 8, t->y + 8, right);
    }
}

/* The rows: an item (its icon, name and count; key items and HMs have none; a mart's row its price; a key item on X
 * or Y that button), then the CLOSE row; the arrow on the Bag's selection, or on the row held. */
static void FBRows(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBag *b = &v->bag;
    int held = b->pressed >= PT_BAG_HIT_ROW && b->pressed < PT_BAG_HIT_ROW + PT_BAG_ROWS ? b->pressed - PT_BAG_HIT_ROW : -1;

    Fill(kBagList.x, kBagList.y, kBagList.w, kBagList.h, BPageBg());
    KitPanel(&kBagList, KIT_FRAME_STD, v->frameType);
    for (int i = 0; i < PT_BAG_ROWS; ++i) {
        CtrPokeTouchRect r = BagRow(i);
        int index = b->first + i, chosen = held >= 0 ? held == i : index == b->cursor;
        char right[16] = "";

        if (index > b->count)
            break;
        if (index == b->count) {
            BagRowDraw(&r, 0, b->kind != PT_BAG_KIND_BAG ? b->closeName : CtrUiText(UIS_BAG_CLOSE), NULL, chosen, held == i);
            continue;
        }
        if (b->kind == PT_BAG_KIND_SHOP)
            snprintf(right, sizeof(right), "\xc2\xa5%u", (unsigned)b->row[i].price);
        else if (b->kind == PT_BAG_KIND_BAG && b->pocket == 1 && b->row[i].item
                 && (b->row[i].item == b->regX || b->row[i].item == b->regY))
            snprintf(right, sizeof(right), "%s", b->row[i].item == b->regX ? "X" : "Y");
        else if (b->kind != PT_BAG_KIND_BAG ? b->row[i].qty != 0 : b->pocket != 1)
            snprintf(right, sizeof(right), "x%u", b->row[i].qty > 999 ? 999 : b->row[i].qty);
        BagRowDraw(&r, b->row[i].item, b->row[i].name, right, chosen, held == i);
    }
}

/* The band: what the Bag says now. */
static void FBNote(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBag *b = &v->bag;
    char text[128], aside[24] = "";
    const CtrPokeTouchRect *n = b->state == PT_BAG_STATE_MESSAGE ? &kBagNoteWide
                              : b->state == PT_BAG_STATE_LIST ? &kBagNote : &kBagNoteMenu;
    int type = -1;

    switch (b->state) {
    case PT_BAG_STATE_ACTIONS:
        snprintf(text, sizeof(text), CTR_TR("%s is selected.", "%s seleccionado.", "%s gew\xC3\xA4hlt."), b->selName);
        break;
    case PT_BAG_STATE_QUANTITY:
        snprintf(text, sizeof(text), "%s", CtrUiText(b->kind == PT_BAG_KIND_SHOP ? UIS_BAG_BUY_HOW_MANY
                                                     : b->kind == PT_BAG_KIND_ITEMPC ? UIS_BAG_WITHDRAW_HOW_MANY
                                                     : b->use == PT_BAG_USE_SELL ? UIS_BAG_SELL_HOW_MANY
                                                     : b->use == PT_BAG_USE_DEPOSIT ? UIS_BAG_DEPOSIT_HOW_MANY
                                                     : UIS_BAG_TOSS_HOW_MANY));
        break;
    case PT_BAG_STATE_YESNO:
    case PT_BAG_STATE_MESSAGE:
        snprintf(text, sizeof(text), "%s", b->prompt);
        break;
    default:
        if (b->selItem) {
            snprintf(text, sizeof(text), "%s", b->selDesc);
            type = b->moveType;
            if (b->showOwned)
                snprintf(aside, sizeof(aside), "%s %u", CtrUiText(UIS_BAG_IN_BAG), (unsigned)b->owned);
            else if (b->use == PT_BAG_USE_SELL)
                snprintf(aside, sizeof(aside), "\xc2\xa5%u", (unsigned)b->money);
        } else
            snprintf(text, sizeof(text), "%s", b->kind != PT_BAG_KIND_BAG ? b->closeName
                                               : CtrUiText(b->count ? UIS_BAG_CLOSE : UIS_BAG_EMPTY));
        break;
    }
    BagNote(n, text, aside, type, b->typeName, b->state == PT_BAG_STATE_MESSAGE);
}

static void BagPageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchBag *b = &v->bag;
    int scrolls = FBScrolls(b);

    Fill(0, 0, W, H, BPageBg());
    FBHeader(v);
    FBRows(v);
    BagArrowCell(&kBagUp, v->frameType, 0, scrolls && b->first > 0, b->pressed == PT_BAG_HIT_UP);
    BagArrowCell(&kBagDown, v->frameType, 1, scrolls && b->first + PT_BAG_ROWS <= b->count, b->pressed == PT_BAG_HIT_DOWN);
    FBNote(v);
    if (b->state == PT_BAG_STATE_LIST)
        BagCell(&kBagCancel, v->frameType, CtrUiText(UIS_CANCEL), 0, b->pressed == PT_BAG_HIT_BACK, 1);
    if (b->state == PT_BAG_STATE_ACTIONS || b->state == PT_BAG_STATE_YESNO) {
        const char *names[PT_BAG_MENU_MAX];
        int n = FBMenuRows(b), held = b->pressed >= PT_BAG_HIT_MENU && b->pressed < PT_BAG_HIT_MENU + n
                                    ? b->pressed - PT_BAG_HIT_MENU : -1;

        for (int i = 0; i < n; ++i)
            names[i] = b->state == PT_BAG_STATE_YESNO ? CtrUiText(i ? UIS_NO : UIS_YES) : b->menuName[i];
        BagMenu(v->frameType, names, n, held >= 0 ? held : b->menuCursor);
    } else if (b->state == PT_BAG_STATE_QUANTITY) {
        char price[16] = "";

        if (b->use == PT_BAG_USE_SELL || b->kind == PT_BAG_KIND_SHOP)
            snprintf(price, sizeof(price), "\xc2\xa5%u", (unsigned)b->sale);
        BagQuantity(v->frameType, b->tossCount, price,
                    b->pressed == PT_BAG_HIT_QTY_UP ? 1 : b->pressed == PT_BAG_HIT_QTY_DOWN ? 2
                    : b->pressed == PT_BAG_HIT_QTY_OK ? 3 : b->pressed == PT_BAG_HIT_QTY_CANCEL ? 4 : 0);
    }
}
