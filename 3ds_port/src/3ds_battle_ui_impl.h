/* The battle screen (included at the end of 3ds_poketouch.c, whose drawing helpers and design system it uses).
 * The lower screen while FireRed's battle is on the upper one: the commands with the quick ball, the moves, the targets
 * and the Safari Zone's four are pages in the kit's look (3ds_battle_page_impl.h), and so is the Party, the field's
 * too (3ds_party_page_impl.h), and the battle Bag (3ds_bag_page_impl.h). Everything shown is what the controller
 * reports. */

#include "3ds_icons.h"
#include "3ds_ui_strings.h"
#include "3ds_type_chart.h"
#include "3ds_battle_nav.h"

/* The party's balls, on their pill. */
static const CtrPokeTouchRect sBStrip = {70, 3, 180, 28};
/* The commands, FireRed's cursors 0-3 (FIGHT, BAG, POKeMON, RUN), and in a wild battle the quick ball
 * (CTR_ACTION_BALL) with its arrows: where 3ds_lower_layout.h puts them, the row with the quick ball apart. */
static const CtrPokeTouchRect *BActRect(const CtrBattleView *v, int i)
{
    if (i == CTR_ACTION_BALL)
        return &kBatBall;
    return &(v->ball == CTR_QUICK_BALL_NONE ? kBatAction : kBatActionBall)[i];
}
/* The battle Bag's pocket order once shown as tabs (CtrBattleUi_TabPocket). */
static const int sBTabPocket[3] = {0, 2, 1};            /* FireRed's pockets in the order the tabs once showed them */
/* The Party and the Bag are pages (3ds_party_page_impl.h, and BBagHit after the BAG's pieces further down). */
static int PPageHit(const CtrBattleView *v, int x, int y);
static int BBagHit(const CtrBattleView *v, int x, int y);

static int BIn(const CtrPokeTouchRect *r, int x, int y)
{
    return UiIn(r, x, y);
}

int CtrBattleUi_TabPocket(int tab)
{
    return tab >= 0 && tab < 3 ? sBTabPocket[tab] : -1;
}


int CtrBattleUi_Hit(const CtrBattleView *v, int x, int y)
{
    if (v->state == BUI_ACTION) {
        for (int i = 0; i < 4; ++i)
            if (BIn(BActRect(v, i), x, y))
                return i;
        if (v->ball != CTR_QUICK_BALL_NONE) {
            /* The arrows only while there is another ball to go to. */
            if (v->ball == CTR_QUICK_BALL_READY && v->ballCount > 1 && BIn(&kBatBallArrow[0], x, y))
                return BUI_ACT_BALL_PREV;
            if (v->ball == CTR_QUICK_BALL_READY && v->ballCount > 1 && BIn(&kBatBallArrow[1], x, y))
                return BUI_ACT_BALL_NEXT;
            if (BIn(&kBatBall, x, y))
                return BUI_ACT_BALL;
        }
    }
    if (v->state == BUI_MOVE || v->state == BUI_TARGET) {
        for (int i = 0; i < 4; ++i)
            if (v->state == BUI_MOVE ? v->move[i].present && BIn(&kBatMove[i], x, y)
                                     : v->target[i].present && BIn(&kBatTarget[i], x, y))
                return i;
        if (BIn(&kBatCancel, x, y))
            return BUI_CANCEL;
        return -1;
    }
    if (v->state == BUI_SAFARI) {
        for (int i = 0; i < 4; ++i)
            if (BIn(&kBatSafari[i], x, y))
                return i;
        return -1;
    }
    if (v->state == BUI_PARTY)
        return PPageHit(v, x, y);
    if (v->state == BUI_BAG)
        return BBagHit(v, x, y);
    return -1;
}

/* The Party's icons (the Party page, the battle pages' party strip, HOME's party window). */
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

/* The selected slot's icon hops with its frames, as FireRed's Party does: up on the first, down on the second. */
static int BPartyBounce(const CtrBattleParty *p, int i)
{
    return p->cursor == i ? (p->iconFrame[i] & 2 ? 1 : -3) : 0;
}

static const char *const sBPartyTagText[5] = {"", CTR_TR("ABLE", "PUEDE", "Kann"), CTR_TR("NOT ABLE", "NO PUEDE", "Nein"), CTR_TR("LEARNED", "APRENDIDO", "Kennt Att."), CTR_TR("NO USE", "NO SIRVE", "Nutzlos")};

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

/* The port's flat shoe for RUN (tools/make_battle_icons.py; FireRed has no icon for running away). */
#include "battle_flat_icons.h"

/* A Poke Ball of the party strip, 15 pixels across: red and white for a Pokemon that can fight, grey for one that
 * fainted (kind 2), a hollow ring for no Pokemon (kind 0); an amber dot for a status condition. */
static void BBall(int cx, int cy, int kind, int status)
{
    for (int dy = -7; dy <= 7; ++dy)
        for (int dx = -7; dx <= 7; ++dx) {
            int d2 = dx * dx + dy * dy;
            uint16_t c;

            if (d2 > 52)
                continue;
            if (kind == 0) {
                if (d2 >= 36)
                    Px(cx + dx, cy + dy, UI_RGB(120, 56, 52));
                continue;
            }
            if (d2 >= 40 || dy == 0)
                c = UI_RGB(40, 24, 28);
            else if (d2 <= 5)
                c = d2 <= 2 ? EM_WHITE : UI_RGB(40, 24, 28);
            else if (dy < 0)
                c = kind == 2 ? UI_RGB(150, 146, 156) : dx < -1 && dy < -3 ? UI_RGB(255, 140, 120) : UI_RGB(230, 64, 52);
            else
                c = kind == 2 ? UI_RGB(196, 192, 200) : UI_RGB(244, 242, 248);
            Px(cx + dx, cy + dy, c);
        }
    if (kind && kind != 2 && status) {
        UiDisc(cx + 6, cy - 6, 3, UI_RGB(110, 40, 6));
        UiDisc(cx + 6, cy - 6, 2, UI_RGB(255, 200, 60));
    }
}

/* ---- The action menu ---- */
/* A hit code of the quick ball: the button or one of its arrows. */
static int BBallPart(int code)
{
    return code >= BUI_ACT_BALL && code <= BUI_ACT_BALL_NEXT;
}

/* ---- The Safari Zone: BALL, BAIT, ROCK, RUN (the words of FireRed's gText_SafariZoneMenu) ---- */
/* A screen with the party's balls on top. */
#include "3ds_battle_page_impl.h"
/* POKeMON, field and battle, as a page. */
#include "3ds_party_page_impl.h"
/* The BAG's pieces as a page (the field's Bag and the battle's). */
#include "3ds_bag_page_impl.h"

/* ---- The battle Bag as a page (phase 2, the owner's "3"): the Items pocket's tab (and BERRIES beside it, which opens
 * the Berry Pouch), the list's rows with their icons, the scroll cells, the band and CANCEL; FireRed's menu in the
 * Party's window, a message in the whole band. ---- */
static int BBagScrolls(const CtrBattleBag *b)
{
    return b->count > BUI_BAG_ROWS;
}

static int BBagHit(const CtrBattleView *v, int x, int y)
{
    const CtrBattleBag *b = &v->bag;

    if (b->message[0])
        return BUI_BAG_MESSAGE;                         /* anywhere: the message waits for A */
    if (b->menuCount > 0) {
        int n = b->menuCount > 3 ? 3 : b->menuCount;
        CtrPokeTouchRect w = PartyMenuWindow(n);

        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r = PartyMenuRow(n, i);

            if (UiIn(&r, x, y))
                return BUI_BAG_MENU + i;
        }
        return UiIn(&w, x, y) ? -1 : BUI_BAG_CANCEL;     /* off the menu: B */
    }
    if (UiIn(&kBagCancel, x, y))
        return BUI_BAG_CANCEL;
    if (b->berries && UiIn(&kBagTab[1], x, y))
        return BUI_BAG_BERRIES;
    if (BBagScrolls(b)) {
        if (UiIn(&kBagUp, x, y))
            return b->first > 0 ? BUI_BAG_UP : -1;
        if (UiIn(&kBagDown, x, y))
            return b->first + BUI_BAG_ROWS < b->count ? BUI_BAG_DOWN : -1;
    }
    for (int i = 0; i < BUI_BAG_ROWS; ++i) {
        CtrPokeTouchRect r = BagRow(i);

        if (b->row[i].item && UiIn(&r, x, y))
            return BUI_BAG_ROW + i;
    }
    return -1;
}

static void BBagPage(const CtrBattleView *v)
{
    const CtrBattleBag *b = &v->bag;
    int held = v->pressed >= BUI_BAG_ROW && v->pressed < BUI_BAG_ROW + BUI_BAG_ROWS ? v->pressed - BUI_BAG_ROW : -1;
    int scrolls = BBagScrolls(b);
    char text[128];

    Fill(0, 0, W, H, BPageBg());
    BagCell(&kBagTab[0], v->frameType, CTR_TR("ITEMS", "OBJETOS", "Items"), 1, 0, 1);
    if (b->berries)
        BagCell(&kBagTab[1], v->frameType, CTR_TR("BERRIES", "BAYAS", "Beeren"), 0, v->pressed == BUI_BAG_BERRIES, 1);
    Fill(kBagList.x, kBagList.y, kBagList.w, kBagList.h, BPageBg());
    KitPanel(&kBagList, KIT_FRAME_STD, v->frameType);
    for (int i = 0; i < BUI_BAG_ROWS; ++i) {
        CtrPokeTouchRect r = BagRow(i);
        char qty[8] = "";

        if (!b->row[i].item)
            continue;
        if (b->pocket != 1)                             /* key items are one of each: no count */
            snprintf(qty, sizeof(qty), "x%u", b->row[i].qty > 999 ? 999 : b->row[i].qty);
        BagRowDraw(&r, b->row[i].item, b->row[i].name, qty, held >= 0 ? held == i : b->first + i == b->cursor, held == i);
    }
    BagArrowCell(&kBagUp, v->frameType, 0, scrolls && b->first > 0, v->pressed == BUI_BAG_UP);
    BagArrowCell(&kBagDown, v->frameType, 1, scrolls && b->first + BUI_BAG_ROWS < b->count, v->pressed == BUI_BAG_DOWN);
    if (b->message[0]) {
        BagNote(&kBagNoteWide, b->message, NULL, -1, NULL, 1);
        return;
    }
    if (b->menuCount > 0) {
        const char *names[3];
        int n = b->menuCount > 3 ? 3 : b->menuCount;
        int menuHeld = v->pressed >= BUI_BAG_MENU && v->pressed < BUI_BAG_MENU + n ? v->pressed - BUI_BAG_MENU : -1;

        snprintf(text, sizeof(text), CTR_TR("%s is selected.", "%s seleccionado.", "%s gew\xC3\xA4hlt."), b->selName);
        BagNote(&kBagNoteMenu, text, NULL, -1, NULL, 0);
        for (int i = 0; i < n; ++i)
            names[i] = b->menuName[i];
        BagMenu(v->frameType, names, n, menuHeld >= 0 ? menuHeld : b->menuCursor);
        return;
    }
    BagNote(&kBagNote, b->selItem ? b->selDesc : CTR_TR("Close the Bag", "Cerrar la MOCHILA", "Beutel schlie\xC3\x9F" "en"),
            NULL, -1, NULL, 0);
    BagCell(&kBagCancel, v->frameType, CTR_TR("CANCEL", "CANCELAR", "Zur\xC3\xBC" "ck"), 0, v->pressed == BUI_BAG_CANCEL, 1);
}

/* Every state but the Bag and the Party: a page (3ds_battle_page_impl.h). */
static void BScreen(const CtrBattleView *v)
{
    BPageScreen(v);
}

void CtrBattleUi_Draw(uint16_t *canvas, const CtrBattleView *v)
{
    sCanvas = canvas;
    EmSurface();
    if (v->state == BUI_PARTY)
        PPageScreen(v);
    else if (v->state == BUI_BAG)
        BBagPage(v);
    else
        BScreen(v);
    sUiS = &sUiNavy;
}

/* Each changed battle element is presented through the shared dirty-rectangle path. */
static void BMark(CtrDirtyList *dirty, const CtrPokeTouchRect *r)
{
    CtrDirty_Add(dirty, r->x, r->y, r->x + r->w, r->y + r->h);
}

/* Whether a screen with the balls must be drawn again whole: another screen, other balls, other cards. */
static int BScreenChanged(const CtrBattleView *a, const CtrBattleView *b)
{
    if (a->state != b->state || a->frameType != b->frameType || memcmp(a->party, b->party, sizeof(b->party)) || memcmp(a->partyHp, b->partyHp, sizeof(b->partyHp))
        || memcmp(a->partyStatus, b->partyStatus, sizeof(b->partyStatus)) || memcmp(a->partyIcon, b->partyIcon, sizeof(b->partyIcon)))
        return 1;
    if (b->state == BUI_ACTION)
        return strcmp(a->activeName, b->activeName) || a->activeLevel != b->activeLevel || a->activeIcon != b->activeIcon
            || a->activeSlot != b->activeSlot || a->ball != b->ball || a->ballItem != b->ballItem
            || a->ballQty != b->ballQty || a->ballPlace != b->ballPlace || a->ballCount != b->ballCount
            || strcmp(a->ballName, b->ballName);
    if (b->state == BUI_MOVE)
        return memcmp(a->move, b->move, sizeof(b->move)) != 0;
    if (b->state == BUI_TARGET)
        return memcmp(a->target, b->target, sizeof(b->target)) != 0;
    return 0;
}

void CtrBattleUi_Update(uint16_t *canvas, const CtrBattleView *old, const CtrBattleView *v, CtrDirtyList *dirty)
{
    CtrPokeTouchRect all = {0, 0, W, H};

    sCanvas = canvas;
    EmSurface();
    CtrDirty_Clear(dirty);
    if (v->state == BUI_PARTY && old->state == BUI_PARTY) {
        PPageUpdate(old, v, dirty);
    } else if (v->state == BUI_PARTY || old->state == BUI_PARTY) {
        CtrBattleUi_Draw(canvas, v);
        EmSurface();
        BMark(dirty, &all);
    } else if (v->state == BUI_BAG || old->state == BUI_BAG) {
        if (old->state != v->state || old->frameType != v->frameType || memcmp(&old->bag, &v->bag, sizeof(v->bag))
            || old->pressed != v->pressed) {
            CtrBattleUi_Draw(canvas, v);
            EmSurface();
            BMark(dirty, &all);
        }
    } else if (BScreenChanged(old, v)) {
        CtrBattleUi_Draw(canvas, v);
        EmSurface();
        BMark(dirty, &all);
    } else {
        BPageUpdate(old, v, dirty);
    }
    sUiS = &sUiNavy;
}
