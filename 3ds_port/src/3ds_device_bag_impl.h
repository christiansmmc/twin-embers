/* The Bag as the red device (style B; boards B · BAG and B · BAG: menu do item), included by 3ds_poketouch.c after the
 * kit, HOME and the Party and before the battle. One page for the field's Bag and the TM Case and Berry Pouch it opens,
 * a mart's BUY, the item PC's WITHDRAW ITEM and the mailbox (CtrPokeTouch_BagHit; CtrPokeTouch_Draw / Update draw it
 * whole while view->bag.on and there is no GBA picture: DevBagShown), and the battle's Bag (CtrBattleUi_* in BUI_BAG).
 * Each view is read into a DevBagPage (DevBagFromField, DevBagFromBattle), which is what is drawn and hit (DevBagDraw,
 * DevBagHit); every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Across the top the keys: the Bag's three pockets (FireRed's pocket sunk, or the one a touched pocket key asked for
 * while the Bag walks there), a case's one key with its name, or a title on a screen (a mart's with the player's money);
 * the battle's ITEMS and, when the Bag holds the Berry Pouch, BERRIES. The list's screen: the grid's five rows (the
 * item's icon at half size, its name, how many at the right; a mart's price; a key item's X or Y button), FireRed's
 * cursor inverted; ▲ and ▼ beside it, tall keys, their triangle grey when the list goes no further that way. Under it
 * the description's screen (the item's icon, a TM's type, how many the Bag holds of a mart's item, FireRed's
 * description) and CLOSE, a dark key, sunk while FireRed's cursor is on the list's CLOSE row. The item's menu (board B ·
 * BAG: menu do item): the list narrows and its words go grey, the actions stand as cream keys in the grid's column on
 * the right and FireRed's last row (CANCEL or EXIT) is the dark key in the exit corner; the footer says what is
 * selected. YES / NO: YES in the column, NO in the exit corner, the question in the footer. How many: ▲, the count's
 * screen (with what that many cost or fetch and the money, buying or selling), ▼ and OK in the column, CANCEL in the
 * exit corner, the question in the footer. Meanwhile the pockets but the one shown are beige and take no touch; the one
 * shown (a case's key) stays sunk, a title as it is. A message is on the list's screen with FireRed's arrow, the other
 * keys and the band across the top grey: a tap anywhere goes on.
 *
 * The chosen key or row is the one under a held touch, else the game's cursor (DevChosen); a touch on what is not drawn
 * or cannot be used chooses nothing (DevBagPressDrawn). Four screws (DevShell). Words: upper case (decision 5): FireRed's names and
 * the menu's through DevUpper (a mailbox's writers as typed), FireRed's descriptions, questions and messages as they
 * are, the port's words in capitals.
 *
 * Redraw (decision 7): the page is all that is drawn, so two pages that differ (or the touch as drawn, or the shell)
 * redraw the screen whole, and nothing else does. */

#define DEV_BAG_WORD        24                              /* a name as the page keeps it (in capitals, or as typed) */
#define DEV_BAG_TEXT        112                             /* a description, a question, a message */
#define DEV_BAG_MENU        PT_BAG_MENU_MAX
#define DEV_BAG_READ_X      48                              /* the description's and the narrow screen's words */
#define DEV_BAG_ICON_X      5                               /* a row's icon (12x12) and name, from the row's left */
#define DEV_BAG_NAME_X      22
#define DEV_BAG_ICON_Y      6                               /* a row's icon, from the row's top */
#define DEV_BAG_CAPS_Y      8                               /* a row's capitals (name, count, price), from its top */
#define DEV_BAG_CHIP_Y      7                               /* a key item's X / Y chip (10 tall), from the row's top */
#define DEV_BAG_PAD         6                               /* a row's right margin, and the gap before its count */

_Static_assert(PT_BAG_ROWS == BUI_BAG_ROWS, "the field's Bag and the battle's show as many rows");
_Static_assert(PT_BAG_ROWS == DEV_GRID_ROWS, "the Bag's lists show the grid's rows");

/* The pockets' words, in the game's pocket order (FireRed's 0 Items, 1 Key Items, 2 Poke Balls; test_bag_pockets.py). */
static const CtrUiString kDevBagPocketLabel[3] = {UIS_BAG_ITEMS, UIS_BAG_KEY_ITEMS, UIS_BAG_BALLS};

enum { DEV_BAG_LIST = 0, DEV_BAG_ACTIONS, DEV_BAG_YESNO, DEV_BAG_QUANTITY, DEV_BAG_MESSAGE };
enum { DEV_BAG_KEYS_POCKETS = 0, DEV_BAG_KEYS_CASE, DEV_BAG_KEYS_TITLE };
/* What the page shows under a point (DevBagHit), and what a held touch is on (DevBagDraw's `pressed`). */
enum { DEV_BAG_HIT_KEY = 0, DEV_BAG_HIT_ROW = 10, DEV_BAG_HIT_MENU = 20, DEV_BAG_HIT_UP = 30, DEV_BAG_HIT_DOWN,
       DEV_BAG_HIT_CLOSE, DEV_BAG_HIT_QTY_UP, DEV_BAG_HIT_QTY_DOWN, DEV_BAG_HIT_QTY_OK, DEV_BAG_HIT_QTY_CANCEL,
       DEV_BAG_HIT_MESSAGE };

/* All the page draws. Built in a zeroed struct, so two pages compare with memcmp. */
typedef struct {
    uint8_t keys;                       /* DEV_BAG_KEYS_*: pocket keys, a case's one key, or a title */
    uint8_t keyCount;                   /* the pocket keys drawn (0-3) */
    uint8_t keyTouch;                   /* bit i: pocket key i takes a touch */
    int8_t keyChosen;                   /* the pocket key sunk: the pocket asked for, else FireRed's */
    char key[3][DEV_BAG_WORD];          /* the pocket keys' words; key[0]: a case's name or the title */
    uint8_t showMoney;                  /* the title shows the player's money (a mart) */
    uint32_t money;
    uint8_t state;                      /* DEV_BAG_* */
    int16_t count, cursor, first;       /* the list: its items, FireRed's cursor (count: CLOSE), the first row's item */
    uint8_t rows;                       /* the rows drawn: the items from `first`, PT_BAG_ROWS at most */
    struct {
        uint16_t item;
        char name[DEV_BAG_WORD];
        char right[16];                 /* at the row's right: "×5", a mart's "¥200", or nothing */
        char button;                    /* a key item on X or Y: its button, else 0 */
    } row[PT_BAG_ROWS];
    uint16_t selItem;                   /* the selected item (its icon), 0 for none (CLOSE, an empty pocket) */
    int8_t moveType;                    /* a TM / HM: its move's type (-1 none) and the type's name */
    char typeName[12];
    uint8_t showOwned;                  /* a mart's item: how many the Bag holds, shown */
    uint16_t owned;
    char desc[DEV_BAG_TEXT];            /* FireRed's description of the selected item */
    char closeName[DEV_BAG_WORD];
    uint8_t menuCount;                  /* ACTIONS: the menu's keys; ACTIONS and YESNO: FireRed's cursor among them */
    int8_t menuCursor;
    char menuName[DEV_BAG_MENU][16];
    char read[DEV_BAG_TEXT];            /* the narrow screen's words (what is selected, the question), or the message */
    uint16_t qty;                       /* QUANTITY: how many */
    uint8_t showSale;                   /* buying or selling: what that many come to, and the player's money */
    uint32_t sale, held;
} DevBagPage;

/* ---- The page's codes as the field's (PT_BAG_HIT_*) and the battle's (BUI_BAG_*) ------------------------------------ */

/* A run of n codes, the page's and the field's and the battle's (-1: that one has none). */
static const struct {
    int16_t code[3];
    uint8_t n;
} kDevBagCodes[] = {
    {{DEV_BAG_HIT_KEY, PT_BAG_HIT_TAB, -1}, 3},
    {{DEV_BAG_HIT_KEY + 1, -1, BUI_BAG_BERRIES}, 1},        /* the battle's second key: BERRIES */
    {{DEV_BAG_HIT_ROW, PT_BAG_HIT_ROW, BUI_BAG_ROW}, PT_BAG_ROWS},
    {{DEV_BAG_HIT_MENU, PT_BAG_HIT_MENU, BUI_BAG_MENU}, DEV_BAG_MENU},
    {{DEV_BAG_HIT_UP, PT_BAG_HIT_UP, BUI_BAG_UP}, 1},
    {{DEV_BAG_HIT_DOWN, PT_BAG_HIT_DOWN, BUI_BAG_DOWN}, 1},
    {{DEV_BAG_HIT_CLOSE, PT_BAG_HIT_CLOSE, BUI_BAG_CANCEL}, 1},
    {{DEV_BAG_HIT_QTY_UP, PT_BAG_HIT_QTY_UP, -1}, 4},       /* UP, DOWN, OK, CANCEL: in the same order */
    {{DEV_BAG_HIT_MESSAGE, PT_BAG_HIT_MESSAGE, BUI_BAG_MESSAGE}, 1},
};
enum { DEV_BAG_PAGE = 0, DEV_BAG_FIELD, DEV_BAG_BATTLE };

/* `code` from one set (DEV_BAG_PAGE, _FIELD, _BATTLE) to another; -1 for a code the other has not. */
static int DevBagCode(int code, int from, int to)
{
    for (unsigned i = 0; code >= 0 && i < sizeof(kDevBagCodes) / sizeof(kDevBagCodes[0]); ++i) {
        int a = kDevBagCodes[i].code[from], b = kDevBagCodes[i].code[to];

        if (a >= 0 && b >= 0 && code >= a && code < a + kDevBagCodes[i].n)
            return b + code - a;
    }
    return -1;
}

/* ---- Reading the views ----------------------------------------------------------------------------------------------- */

/* The item's menu, YES / NO or how many open: the list narrow beside a column of keys, the footer what the page reads
 * (the description, its chips and CLOSE are not drawn, so not read). */
static int DevBagNarrow(const DevBagPage *p)
{
    return p->state == DEV_BAG_ACTIONS || p->state == DEV_BAG_YESNO || p->state == DEV_BAG_QUANTITY;
}

/* The narrow screen's words while the item's menu is open: FireRed's "... is selected." */
static void DevBagSelected(DevBagPage *p, const char *name)
{
    snprintf(p->read, sizeof(p->read), CTR_TR("%s is selected.", "Has elegido %s.", "%s ausgew\xC3\xA4hlt."), name);
}

/* A row's count at its right: "×5" (FireRed's own sign), 999 at most. */
static void DevBagCount(char *out, size_t size, unsigned qty)
{
    snprintf(out, size, "\xC3\x97%u", qty > 999 ? 999 : qty);
}

static void DevBagFromField(const CtrPokeTouchBag *b, DevBagPage *p)
{
    char name[DEV_BAG_WORD];

    memset(p, 0, sizeof(*p));
    p->state = b->state == PT_BAG_STATE_ACTIONS ? DEV_BAG_ACTIONS : b->state == PT_BAG_STATE_YESNO ? DEV_BAG_YESNO
             : b->state == PT_BAG_STATE_QUANTITY ? DEV_BAG_QUANTITY : b->state == PT_BAG_STATE_MESSAGE ? DEV_BAG_MESSAGE
             : DEV_BAG_LIST;
    if (b->kind == PT_BAG_KIND_BAG) {
        p->keys = DEV_BAG_KEYS_POCKETS;
        p->keyCount = 3;
        p->keyTouch = 7;
        p->keyChosen = (int8_t)(b->asked < 3 ? b->asked : b->pocket);
        for (int i = 0; i < 3; ++i)
            DevUpper(p->key[i], CtrUiText(kDevBagPocketLabel[i]), sizeof(p->key[i]));
    } else {
        p->keys = b->kind == PT_BAG_KIND_TMCASE || b->kind == PT_BAG_KIND_POUCH ? DEV_BAG_KEYS_CASE : DEV_BAG_KEYS_TITLE;
        DevUpper(p->key[0], b->title, sizeof(p->key[0]));
        if (b->kind == PT_BAG_KIND_SHOP) {
            p->showMoney = 1;
            p->money = b->money;
        }
    }
    p->count = (int16_t)(b->count < 0 ? 0 : b->count);
    p->cursor = b->cursor;
    p->first = (int16_t)(b->first < 0 ? 0 : b->first);
    for (int i = 0; i < PT_BAG_ROWS && p->first + i < p->count; ++i) {
        p->rows = (uint8_t)(i + 1);
        p->row[i].item = b->row[i].item;
        if (b->typed)
            snprintf(p->row[i].name, sizeof(p->row[i].name), "%s", b->row[i].name);
        else
            DevUpper(p->row[i].name, b->row[i].name, sizeof(p->row[i].name));
        if (b->kind == PT_BAG_KIND_SHOP)
            snprintf(p->row[i].right, sizeof(p->row[i].right), "\xC2\xA5%u", (unsigned)b->row[i].price);
        else if (b->kind != PT_BAG_KIND_BAG ? b->row[i].qty != 0 : b->pocket != 1)   /* key items, HMs: no count */
            DevBagCount(p->row[i].right, sizeof(p->row[i].right), b->row[i].qty);
        else if (b->row[i].item && (b->row[i].item == b->regX || b->row[i].item == b->regY))
            p->row[i].button = b->row[i].item == b->regX ? 'X' : 'Y';
    }
    p->moveType = -1;
    if (b->cursor >= 0 && b->cursor < p->count && b->selItem)
        p->selItem = b->selItem;
    if (!DevBagNarrow(p)) {                                 /* the list's (or a message's) footer and CLOSE */
        if (p->selItem) {
            p->moveType = b->moveType;
            if (b->moveType >= 0)
                DevUpper(p->typeName, b->typeName, sizeof(p->typeName));
            p->showOwned = b->showOwned;
            p->owned = b->showOwned ? b->owned : 0;
            snprintf(p->desc, sizeof(p->desc), "%s", b->selDesc);
        }
        if (b->closeName[0])
            DevUpper(p->closeName, b->closeName, sizeof(p->closeName));
        else
            snprintf(p->closeName, sizeof(p->closeName), "%s", DevCloseWord());
    }
    if (b->typed)
        snprintf(name, sizeof(name), "%s", b->selName);
    else
        DevUpper(name, b->selName, sizeof(name));
    switch (p->state) {
    case DEV_BAG_ACTIONS:
        p->menuCount = (uint8_t)(b->menuCount > DEV_BAG_MENU ? DEV_BAG_MENU : b->menuCount);
        p->menuCursor = b->menuCursor;
        for (int i = 0; i < p->menuCount; ++i)
            DevUpper(p->menuName[i], b->menuName[i], sizeof(p->menuName[i]));
        if (b->prompt[0])                                   /* the list's own question, when it has one */
            snprintf(p->read, sizeof(p->read), "%s", b->prompt);
        else if (p->selItem)
            DevBagSelected(p, name);
        break;
    case DEV_BAG_YESNO:
        p->menuCursor = b->menuCursor;
        snprintf(p->read, sizeof(p->read), "%s", b->prompt);
        break;
    case DEV_BAG_QUANTITY:
        p->qty = b->tossCount;
        if (b->kind == PT_BAG_KIND_SHOP || b->use == PT_BAG_USE_SELL) {
            p->showSale = 1;
            p->sale = b->sale;
            p->held = b->money;
        }
        snprintf(p->read, sizeof(p->read), "%s", CtrUiText(b->kind == PT_BAG_KIND_SHOP ? UIS_BAG_BUY_HOW_MANY
                 : b->kind == PT_BAG_KIND_ITEMPC ? UIS_BAG_WITHDRAW_HOW_MANY : b->use == PT_BAG_USE_SELL ? UIS_BAG_SELL_HOW_MANY
                 : b->use == PT_BAG_USE_DEPOSIT ? UIS_BAG_DEPOSIT_HOW_MANY : UIS_BAG_TOSS_HOW_MANY));
        break;
    case DEV_BAG_MESSAGE:
        snprintf(p->read, sizeof(p->read), "%s", b->prompt);
        break;
    default:
        break;
    }
}

/* The battle's Bag: the Items pocket (the balls have their own key on the commands; the key items stay out but for the
 * Berry Pouch, which BERRIES opens), FireRed's menu (USE, CANCEL) or its message. */
static void DevBagFromBattle(const CtrBattleBag *b, DevBagPage *p)
{
    char name[DEV_BAG_WORD];

    memset(p, 0, sizeof(*p));
    p->state = b->message[0] ? DEV_BAG_MESSAGE : b->menuCount > 0 ? DEV_BAG_ACTIONS : DEV_BAG_LIST;
    p->keys = DEV_BAG_KEYS_POCKETS;
    p->keyCount = (uint8_t)(b->berries ? 2 : 1);
    p->keyTouch = (uint8_t)(b->berries ? 2 : 0);
    DevUpper(p->key[0], CtrUiText(UIS_BAG_ITEMS), sizeof(p->key[0]));
    if (b->berries)
        snprintf(p->key[1], sizeof(p->key[1]), "%s", CTR_TR("BERRIES", "BAYAS", "BEEREN"));
    p->count = (int16_t)(b->count < 0 ? 0 : b->count);
    p->cursor = b->cursor;
    p->first = (int16_t)(b->first < 0 ? 0 : b->first);
    for (int i = 0; i < BUI_BAG_ROWS && p->first + i < p->count && b->row[i].item; ++i) {
        p->rows = (uint8_t)(i + 1);
        p->row[i].item = b->row[i].item;
        DevUpper(p->row[i].name, b->row[i].name, sizeof(p->row[i].name));
        if (b->pocket != 1)                                 /* key items are one of each */
            DevBagCount(p->row[i].right, sizeof(p->row[i].right), b->row[i].qty);
    }
    p->moveType = -1;
    if (b->cursor >= 0 && b->cursor < p->count && b->selItem)
        p->selItem = b->selItem;
    if (!DevBagNarrow(p)) {                                 /* the list's (or a message's) footer and CLOSE */
        if (p->selItem)
            snprintf(p->desc, sizeof(p->desc), "%s", b->selDesc);
        snprintf(p->closeName, sizeof(p->closeName), "%s", DevCloseWord());
    }
    if (p->state == DEV_BAG_ACTIONS) {
        p->menuCount = (uint8_t)(b->menuCount > 3 ? 3 : b->menuCount);
        p->menuCursor = b->menuCursor;
        for (int i = 0; i < p->menuCount; ++i)
            DevUpper(p->menuName[i], b->menuName[i], sizeof(p->menuName[i]));
        DevUpper(name, b->selName, sizeof(name));
        if (p->selItem)
            DevBagSelected(p, name);
    } else if (p->state == DEV_BAG_MESSAGE)
        snprintf(p->read, sizeof(p->read), "%s", b->message);
}

/* ---- Where a touch goes ---------------------------------------------------------------------------------------------- */

static int DevBagUpOn(const DevBagPage *p)
{
    return p->first > 0;
}

static int DevBagDownOn(const DevBagPage *p)
{
    return p->first + PT_BAG_ROWS < p->count;
}

/* The touch held as it is drawn: a code of what is drawn and can be used in the page's state, else -1 (a message, a key
 * the battle's ITEMS is, a row the list has not, ▲ at the top). Every key and row is chosen by it, not by the raw touch. */
static int DevBagPressDrawn(const DevBagPage *p, int press)
{
    switch (p->state) {
    case DEV_BAG_LIST:
        if (press >= DEV_BAG_HIT_KEY && press < DEV_BAG_HIT_KEY + p->keyCount)
            return (p->keyTouch >> (press - DEV_BAG_HIT_KEY)) & 1 ? press : -1;
        if (press >= DEV_BAG_HIT_ROW && press < DEV_BAG_HIT_ROW + p->rows)
            return press;
        if (press == DEV_BAG_HIT_UP || press == DEV_BAG_HIT_DOWN)
            return (press == DEV_BAG_HIT_UP ? DevBagUpOn(p) : DevBagDownOn(p)) ? press : -1;
        return press == DEV_BAG_HIT_CLOSE ? press : -1;
    case DEV_BAG_ACTIONS:
        return press >= DEV_BAG_HIT_MENU && press < DEV_BAG_HIT_MENU + p->menuCount ? press : -1;
    case DEV_BAG_YESNO:
        return press == DEV_BAG_HIT_MENU || press == DEV_BAG_HIT_MENU + 1 ? press : -1;
    case DEV_BAG_QUANTITY:
        return press >= DEV_BAG_HIT_QTY_UP && press <= DEV_BAG_HIT_QTY_CANCEL ? press : -1;
    default:
        return -1;
    }
}

/* The menu's rows (YES / NO: two): row i (i < n - 1) a key in the grid's column, the last FireRed's CANCEL or EXIT (NO)
 * in the grid's exit. */
static CtrPokeTouchRect DevBagMenuRect(int n, int i)
{
    return i == n - 1 ? kDevGridExit : DevBagMenuKey(n, i);
}

static int DevBagHit(const DevBagPage *p, int x, int y)
{
    static const CtrPokeTouchRect *const kQty[4] = {&kDevBagQtyUp, &kDevBagQtyDown, &kDevBagQtyOk, &kDevGridExit};

    switch (p->state) {
    case DEV_BAG_MESSAGE:
        return DEV_BAG_HIT_MESSAGE;                         /* anywhere: the message waits for A */
    case DEV_BAG_ACTIONS:
    case DEV_BAG_YESNO: {
        int n = p->state == DEV_BAG_YESNO ? 2 : p->menuCount;

        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r = DevBagMenuRect(n, i);

            if (DevOnKey(&r, x, y))
                return DEV_BAG_HIT_MENU + i;
        }
        return -1;
    }
    case DEV_BAG_QUANTITY:
        for (int i = 0; i < 4; ++i)
            if (DevOnKey(kQty[i], x, y))
                return DEV_BAG_HIT_QTY_UP + i;
        return -1;
    default:
        for (int i = 0; i < p->keyCount; ++i) {
            CtrPokeTouchRect r = DevGridTab(3, i);

            if (((p->keyTouch >> i) & 1) && DevOnKey(&r, x, y))
                return DEV_BAG_HIT_KEY + i;
        }
        if (DevOnKey(&kDevGridUp, x, y))
            return DevBagUpOn(p) ? DEV_BAG_HIT_UP : -1;
        if (DevOnKey(&kDevGridDown, x, y))
            return DevBagDownOn(p) ? DEV_BAG_HIT_DOWN : -1;
        if (DevOnKey(&kDevGridExit, x, y))
            return DEV_BAG_HIT_CLOSE;
        for (int i = 0; i < p->rows; ++i) {
            CtrPokeTouchRect r = DevGridRow(i, 0);

            if (UiIn(&r, x, y))
                return DEV_BAG_HIT_ROW + i;
        }
        return -1;
    }
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

/* The keys across the top: the pockets, a case's key (always sunk: it is the list shown) or the title's screen (and a
 * mart's money at its right). With the item's menu, YES / NO or how many open the pockets but the one shown are beige
 * (not hit), the one shown stays sunk; under a message the whole band is grey. */
static void DevBagKeys(const DevBagPage *p, int held)
{
    if (p->keys == DEV_BAG_KEYS_TITLE) {
        CtrPokeTouchRect paper = DevScreen(&kDevGridTitle);
        int top = paper.y + (paper.h - 8) / 2, right = paper.x + paper.w - 6;

        if (p->showMoney) {
            char money[32];

            snprintf(money, sizeof(money), "\xC2\xA5%lu", (unsigned long)p->money);
            DevText(money, right - DevTextWidth(money), top, right);
            right -= DevTextWidth(money) + 8;
        }
        DevText(p->key[0], paper.x + 6, top, right);
    } else if (p->keys == DEV_BAG_KEYS_CASE) {
        int faceY = DevKey(&kDevBagCaseKey, DEV_LOOK_KEY, 1, 5);

        DevKeyWord(&kDevBagCaseKey, faceY, DEV_LOOK_KEY, 1, p->key[0]);
    } else {
        int pressKey = held >= DEV_BAG_HIT_KEY && held < DEV_BAG_HIT_KEY + 3 ? held - DEV_BAG_HIT_KEY : -1;

        for (int i = 0; i < p->keyCount; ++i) {
            CtrPokeTouchRect r = DevGridTab(3, i);
            int sunk = DevChosen(i, p->keyChosen, pressKey), look = DevBagNarrow(p) && !sunk ? DEV_LOOK_FAINT : DEV_LOOK_KEY;
            int faceY = DevKey(&r, look, sunk, 5);

            DevKeyWord(&r, faceY, look, sunk, p->key[i]);
        }
    }
    if (p->state == DEV_BAG_MESSAGE)
        DevDim(&kDevGridTitle);                             /* the row's whole band: the title's, over the keys' too */
}

/* The list's rows on its screen (the grid's, or its narrow one beside a column of keys). In the list the chosen row is
 * the one under a held touch, else FireRed's cursor's; beside the keys FireRed's cursor's row stays inverted and the
 * words go grey. A row as the board's: its icon at half size, its name in capitals, how many (or a price) ending at the
 * right margin, a key item's X / Y chip there instead. An empty pocket says so. */
static void DevBagRows(const DevBagPage *p, int held)
{
    int narrow = DevBagNarrow(p), list = p->state == DEV_BAG_LIST, cursor = -1;
    int pressed = held == DEV_BAG_HIT_CLOSE || (held >= DEV_BAG_HIT_ROW && held < DEV_BAG_HIT_ROW + PT_BAG_ROWS) ? held : -1;
    CtrPokeTouchRect paper = DevScreen(narrow ? &kDevGridScreenNarrow : &kDevGridScreen);

    if (p->cursor >= p->count)
        cursor = DEV_BAG_HIT_CLOSE;
    else if (p->cursor - p->first >= 0 && p->cursor - p->first < p->rows)
        cursor = DEV_BAG_HIT_ROW + p->cursor - p->first;
    for (int i = 0; i < p->rows; ++i) {
        CtrPokeTouchRect r = DevGridRow(i, narrow);
        int chosen = list ? DevChosen(DEV_BAG_HIT_ROW + i, cursor, pressed) : cursor == DEV_BAG_HIT_ROW + i;
        uint16_t ink = chosen ? DEV_PAPER : list ? DEV_INK : DEV_INK_DIM, shadow = chosen || !list ? ink : DEV_INK_SH;
        int top = r.y + DEV_BAG_CAPS_Y, right = r.x + r.w - DEV_BAG_PAD, nameRight = right;

        DevRow(&r, chosen);
        DevItemIcon(p->row[i].item, r.x + DEV_BAG_ICON_X, r.y + DEV_BAG_ICON_Y, 1);
        if (p->row[i].right[0]) {
            int w = DevTextWidth(p->row[i].right);

            DevTextInk(p->row[i].right, right - w, top, right, ink, shadow);
            nameRight = right - w - DEV_BAG_PAD;
        } else if (p->row[i].button) {
            char key[2] = {p->row[i].button, 0};
            int w = DevSmallTextWidth(key) + 8;

            DevChip(right - w, r.y + DEV_BAG_CHIP_Y, w, 10, key, DEV_BLUE_INK, DevChipShadow(DEV_BLUE_INK));
            nameRight = right - w - DEV_BAG_PAD;
        }
        if (DevTextWidth(p->row[i].name) <= nameRight - (r.x + DEV_BAG_NAME_X))
            DevTextInk(p->row[i].name, r.x + DEV_BAG_NAME_X, top, nameRight, ink, shadow);
        else                                                /* a long name (a TM with its move): the small font */
            DevSmallText(p->row[i].name, r.x + DEV_BAG_NAME_X, top + 1, ink, nameRight);
    }
    if (!p->count)
        DevText2(CtrUiText(UIS_BAG_EMPTY), paper.x + 8, paper.y + 5, paper.x + paper.w - 8);
}

/* ▲ and ▼: tall keys, their triangle grey while the list goes no further that way (and then a touch is not taken);
 * `dim`: grey keys (a message). */
static void DevBagArrows(const DevBagPage *p, int held, int dim)
{
    DevArrowKeys(&kDevGridUp, &kDevGridDown, DevBagUpOn(p), DevBagDownOn(p), !dim && held == DEV_BAG_HIT_UP,
                 !dim && held == DEV_BAG_HIT_DOWN, 0);
    if (dim) {
        DevDim(&kDevGridUp);
        DevDim(&kDevGridDown);
    }
}

/* The footer, the grid's note: the selected item's icon and, in the list (or under a message), a TM's type and how many
 * the Bag holds of a mart's item in chips (9 tall, on the first line) before FireRed's description (the small font,
 * three lines DEV_SMALL_LINE_TIGHT apart: capitals 199, 208, 217, the first's accents and the third's descenders on the
 * paper; a longer one cut and counted), all from DEV_BAG_READ_X; with the item's menu, YES / NO or how many open, what
 * the page reads there ("... is selected.", the question) from the same x. */
static void DevBagDesc(const DevBagPage *p)
{
    CtrPokeTouchRect paper = DevScreen(&kDevGridNote);
    int x = DEV_BAG_READ_X, top = paper.y + 3, right = paper.x + paper.w - 4, first = x;
    char text[48];

    if (p->selItem)
        DevItemIcon(p->selItem, kDevBagDescIcon.x, kDevBagDescIcon.y, 0);
    if (DevBagNarrow(p)) {
        DevReadText(p->read, &paper, DEV_BAG_READ_X, right, top, DEV_SMALL_LINE_TIGHT);
        return;
    }
    if (!p->selItem)
        return;
    if (p->moveType >= 0 && p->typeName[0]) {
        uint16_t bg = UiTypeColor(p->moveType);
        int w = DevSmallTextWidth(p->typeName) + 8;

        DevChip(first, top, w, 9, p->typeName, bg, DevChipShadow(bg));
        first += w + 4;
    }
    if (p->showOwned) {
        char word[DEV_BAG_WORD];
        int w;

        DevUpper(word, CtrUiText(UIS_BAG_IN_BAG), sizeof(word));
        snprintf(text, sizeof(text), "%s %u", word, (unsigned)p->owned);
        w = DevSmallTextWidth(text) + 8;
        DevChip(first, top, w, 9, text, DEV_BLUE_INK, DevChipShadow(DEV_BLUE_INK));
        first += w + 4;
    }
    DevSmallFit(p->desc, first, x, top, right, 3, DEV_SMALL_LINE_TIGHT, DEV_INK);
}

/* The item's menu or YES / NO (DevBagMenuRect): the actions (YES) cream in the grid's column, their words from x + 10;
 * FireRed's last row (CANCEL, EXIT: its name in capitals) or NO the dark key in the exit corner. FireRed's cursor sunk,
 * or the key under a held touch. */
static void DevBagMenu(const DevBagPage *p, int held)
{
    int yesNo = p->state == DEV_BAG_YESNO, n = yesNo ? 2 : p->menuCount;
    int press = held >= DEV_BAG_HIT_MENU && held < DEV_BAG_HIT_MENU + DEV_BAG_MENU ? held - DEV_BAG_HIT_MENU : -1;
    char word[DEV_BAG_WORD];

    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = DevBagMenuRect(n, i);
        int last = i == n - 1, look = last ? DEV_LOOK_DARK : DEV_LOOK_KEY, sunk = DevChosen(i, p->menuCursor, press);
        int faceY = DevKey(&r, look, sunk, 6);

        if (yesNo)
            DevUpper(word, CtrUiText(i ? UIS_NO : UIS_YES), sizeof(word));
        DevKeyWordAt(&r, faceY, look, sunk, yesNo ? word : p->menuName[i], last ? 0 : 10);
    }
}

/* How many in the grid's column: ▲, the count's screen ("×3"; buying or selling, what they come to and the player's
 * money under it), ▼ and OK; CANCEL the dark key in the exit corner; a key sunk under a held touch. */
static void DevBagQuantity(const DevBagPage *p, int held)
{
    CtrPokeTouchRect paper;
    char count[16], text[32];
    int faceY, sunk;

    for (int i = 0; i < 2; ++i) {
        const CtrPokeTouchRect *r = i ? &kDevBagQtyDown : &kDevBagQtyUp;

        sunk = held == (i ? DEV_BAG_HIT_QTY_DOWN : DEV_BAG_HIT_QTY_UP);
        faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);
        DevTriangle(r->x + r->w / 2, faceY + r->h / 2, i, DevKeyInk(DEV_LOOK_KEY, sunk));
    }
    paper = DevScreen(&kDevBagQtyScreen);
    snprintf(count, sizeof(count), "\xC3\x97%u", (unsigned)p->qty);
    if (p->showSale) {
        int right = paper.x + paper.w - 6;

        DevText(count, paper.x + 6, paper.y + 3, right);
        snprintf(text, sizeof(text), "\xC2\xA5%lu", (unsigned long)p->sale);
        DevText(text, right - DevTextWidth(text), paper.y + 3, right);
        DevSmallText(CTR_TR("MONEY", "DINERO", "GELD"), paper.x + 6, paper.y + 14, DEV_INK2, right);
        snprintf(text, sizeof(text), "\xC2\xA5%lu", (unsigned long)p->held);
        DevSmallText(text, right - DevSmallTextWidth(text), paper.y + 14, DEV_INK2, right);
    } else
        DevText(count, paper.x + (paper.w - DevTextWidth(count)) / 2, paper.y + (paper.h - 8) / 2, paper.x + paper.w - 6);
    sunk = held == DEV_BAG_HIT_QTY_OK;
    faceY = DevKey(&kDevBagQtyOk, DEV_LOOK_KEY, sunk, 6);
    DevKeyWord(&kDevBagQtyOk, faceY, DEV_LOOK_KEY, sunk, CTR_TR("OK", "OK", "OK"));
    sunk = held == DEV_BAG_HIT_QTY_CANCEL;
    faceY = DevKey(&kDevGridExit, DEV_LOOK_DARK, sunk, 6);
    DevKeyWord(&kDevGridExit, faceY, DEV_LOOK_DARK, sunk, DevCancelWord());
}

/* CLOSE, a dark key: sunk by FireRed's cursor on the list's CLOSE row or a held touch; grey under a message. */
static void DevBagClose(const DevBagPage *p, int held)
{
    int message = p->state == DEV_BAG_MESSAGE;
    int sunk = !message && DevChosen(DEV_BAG_HIT_CLOSE, p->cursor >= p->count ? DEV_BAG_HIT_CLOSE : -1,
                                     held == DEV_BAG_HIT_CLOSE || (held >= DEV_BAG_HIT_ROW && held < DEV_BAG_HIT_ROW + PT_BAG_ROWS)
                                     ? held : -1);
    int faceY = DevKey(&kDevGridExit, DEV_LOOK_DARK, sunk, 6);

    DevKeyWord(&kDevGridExit, faceY, DEV_LOOK_DARK, sunk, p->closeName);
    if (message)
        DevDim(&kDevGridExit);
}

/* The page whole, `pressed` the page's code under a held touch (or -1). DevBegin is the caller's. */
static void DevBagDraw(const DevBagPage *p, int pressed)
{
    int held = DevBagPressDrawn(p, pressed);

    DevShell();
    DevBagKeys(p, held);
    if (p->state == DEV_BAG_MESSAGE) {
        CtrPokeTouchRect paper = DevScreen(&kDevGridScreen);

        DevWrap(p->read, paper.x + 8, paper.y + 8, paper.w - 28, (paper.h - 16) / DEV_LINE_H, DEV_INK, DEV_INK_SH);
        DevTriangle(paper.x + paper.w - 10, paper.y + paper.h - 8, 1, DEV_INK2);
        DevBagArrows(p, -1, 1);
        DevBagDesc(p);
        DevBagClose(p, -1);
        return;
    }
    DevBagRows(p, held);
    if (!DevBagNarrow(p)) {
        DevBagArrows(p, held, 0);
        DevBagDesc(p);
        DevBagClose(p, held);
        return;
    }
    DevBagDesc(p);
    if (p->state == DEV_BAG_QUANTITY)
        DevBagQuantity(p, held);
    else
        DevBagMenu(p, held);
}

/* ---- The field's Bag (CtrPokeTouch_*) and the battle's (CtrBattleUi_*) ------------------------------------------------ */

static int DevBagShown(const CtrPokeTouchView *v)
{
    return v->bag.on && !v->viewport;
}

int CtrPokeTouch_BagHit(const CtrPokeTouchView *v, int x, int y)
{
    DevBagPage p;

    if (!v->bag.on)
        return -1;
    DevBagFromField(&v->bag, &p);
    return DevBagCode(DevBagHit(&p, x, y), DEV_BAG_PAGE, DEV_BAG_FIELD);
}

/* The field's Bag, whole (sCanvas is CtrPokeTouch_Draw's). */
static void DevBagFieldDraw(const CtrPokeTouchView *v)
{
    DevBagPage p;

    DevBegin(sCanvas, v->shell);
    DevBagFromField(&v->bag, &p);
    DevBagDraw(&p, DevBagCode(v->bag.pressed, DEV_BAG_FIELD, DEV_BAG_PAGE));
}

/* From one Bag to the next: the screen whole when the pages, the touch as drawn or the shell differ. */
static void DevBagFieldUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)
{
    DevBagPage a, b;

    DevBagFromField(&old->bag, &a);
    DevBagFromField(&v->bag, &b);
    if (memcmp(&a, &b, sizeof(a)) || old->shell != v->shell
        || DevBagPressDrawn(&a, DevBagCode(old->bag.pressed, DEV_BAG_FIELD, DEV_BAG_PAGE))
           != DevBagPressDrawn(&b, DevBagCode(v->bag.pressed, DEV_BAG_FIELD, DEV_BAG_PAGE))) {
        DevBagFieldDraw(v);
        CtrDirty_Add(dirty, 0, 0, W, H);
    }
}

/* The battle's Bag (BUI_BAG): CtrBattleUi_Draw's (after DevBegin), Hit's and Update's. */
static void DevBagBattleDraw(const CtrBattleView *v)
{
    DevBagPage p;

    DevBagFromBattle(&v->bag, &p);
    DevBagDraw(&p, DevBagCode(v->pressed, DEV_BAG_BATTLE, DEV_BAG_PAGE));
}

static int DevBagBattleHit(const CtrBattleView *v, int x, int y)
{
    DevBagPage p;

    DevBagFromBattle(&v->bag, &p);
    return DevBagCode(DevBagHit(&p, x, y), DEV_BAG_PAGE, DEV_BAG_BATTLE);
}

/* Nothing the battle's Bag draws differs between `old` and `v` (the battle's other fields are not drawn). */
static int DevBagBattleSame(const CtrBattleView *old, const CtrBattleView *v)
{
    DevBagPage a, b;

    DevBagFromBattle(&old->bag, &a);
    DevBagFromBattle(&v->bag, &b);
    return !memcmp(&a, &b, sizeof(a)) && old->shell == v->shell
        && DevBagPressDrawn(&a, DevBagCode(old->pressed, DEV_BAG_BATTLE, DEV_BAG_PAGE))
           == DevBagPressDrawn(&b, DevBagCode(v->pressed, DEV_BAG_BATTLE, DEV_BAG_PAGE));
}
