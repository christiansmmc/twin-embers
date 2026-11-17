#ifndef CTR_DEVICE_LAYOUT_H
#define CTR_DEVICE_LAYOUT_H

#include "3ds_poketouch.h"
#include "3ds_party_grid.h"     /* the Party's menu: its grid */

/*
 * The red device (style B): the rectangles of the lower screen's B views, in screen pixels, taken from the owner's
 * boards. One source for what a view draws, what its touch hits and what device_layout_test checks (on the screen with
 * the drop, keys at least 28x20, the keys of one state apart). A key's rectangle is its resting face, as the boards
 * give it; its drop shadow takes DEV_DROP rows under it, and a chosen key is drawn DEV_SINK rows lower inside that
 * same area, so DevKeyArea is both where a key paints and where it is hit.
 *
 * Every rectangle here is `static const CtrPokeTouchRect kDev... __attribute__((unused))` or a static inline function
 * (a list's n-th row, a grid's cell, a key of a row across).
 */

#define DEV_DROP 3      /* rows of a key's drop shadow under its face */
#define DEV_SINK 2      /* rows a chosen (sunk) key's face is drawn lower */

/* Where a key paints and is hit: its face and the drop under it. */
static inline CtrPokeTouchRect DevKeyArea(const CtrPokeTouchRect *r)
{
    CtrPokeTouchRect a = {r->x, r->y, r->w, (int16_t)(r->h + DEV_DROP)};

    return a;
}

/* ---- HOME (board B · Início, 3ds_device_home_impl.h) ------------------------------------------------------------------
 * The screen: its paper holds the map's window on the left (PT_HOME_X/Y/W/H, 3ds_poketouch.h) and the panel on the
 * right: the place's name (two lines, DEV_LINE_H apart), the region line, a rule, the party's six cells, RUN and SPD
 * (paper buttons: no drop). Under the screen the six keys, in PT_* order. In the window, over the map: the region key
 * (its width is its text's: the most it may take is kDevHomeMapKey.w) and FLY. A text's rectangle is the board's box;
 * the capitals start 3 px into a box of the normal font, 2 px into one of the small font. */
static const CtrPokeTouchRect kDevHomeScreen __attribute__((unused)) = {8, 8, 304, 140};
static const CtrPokeTouchRect kDevHomeKey[6] __attribute__((unused)) = {
    {8, 156, 96, 36}, {112, 156, 96, 36}, {216, 156, 96, 36}, {8, 198, 96, 34}, {112, 198, 96, 34}, {216, 198, 96, 34},
};
static const CtrPokeTouchRect kDevHomeRun __attribute__((unused)) = {210, 123, 44, 16};
static const CtrPokeTouchRect kDevHomeSpeed __attribute__((unused)) = {258, 123, 44, 16};
static const CtrPokeTouchRect kDevHomeName __attribute__((unused)) = {212, 17, 90, 26};
static const CtrPokeTouchRect kDevHomeRegion __attribute__((unused)) = {212, 44, 90, 10};
static const CtrPokeTouchRect kDevHomeRule __attribute__((unused)) = {210, 58, 92, 1};
static const CtrPokeTouchRect kDevHomeMapKey __attribute__((unused)) = {16, 16, 96, 14};
/* FLY at the window's top right: the board's place (16, 122) covers Cinnabar Island (Kanto's cell 4,14); cells 17-21 of
 * rows 0-2 hold no place on any of the four maps. */
static const CtrPokeTouchRect kDevHomeFly __attribute__((unused)) = {158, 16, 44, 16};

/* The party's cell i (0-5), three to a row: its icon, its HP bar, and where a touch on it is taken. */
static inline CtrPokeTouchRect DevHomeCell(int i)
{
    CtrPokeTouchRect r = {(int16_t)(210 + 31 * (i % 3)), (int16_t)(62 + 28 * (i / 3)), 30, 28};

    return r;
}

/* ---- The battle (boards B · Comandos and B · Golpes, 3ds_device_battle_impl.h) -----------------------------------------
 * The commands (ACTION; IDLE shows them dimmed): the screen with the lead, FIGHT across, and under it a row of three
 * keys (BAG, POKeMON, RUN: a trainer) or of four (BAG, the quick ball, POKeMON, RUN: a wild battle). The moves (MOVE),
 * a double battle's targets (TARGET) and the Safari Zone's BALL BAIT ROCK RUN (SAFARI): the short screen and four keys
 * in the controller's 2x2 order; MOVE and TARGET add CANCEL, a dark pill. The quick ball's arrows are on its key. */
static const CtrPokeTouchRect kDevBatScreen __attribute__((unused)) = {8, 8, 304, 88};
static const CtrPokeTouchRect kDevBatMoveScreen __attribute__((unused)) = {8, 8, 304, 56};
static const CtrPokeTouchRect kDevBatFight __attribute__((unused)) = {8, 104, 304, 56};
static const CtrPokeTouchRect kDevBatRow3[3] __attribute__((unused)) = {{8, 170, 96, 60}, {112, 170, 96, 60}, {216, 170, 96, 60}};
static const CtrPokeTouchRect kDevBatRow4[4] __attribute__((unused)) = {
    {8, 170, 73, 60}, {85, 170, 73, 60}, {162, 170, 73, 60}, {239, 170, 73, 60},
};
static const CtrPokeTouchRect kDevBatMove[4] __attribute__((unused)) = {
    {8, 72, 148, 62}, {164, 72, 148, 62}, {8, 142, 148, 62}, {164, 142, 148, 62},
};
static const CtrPokeTouchRect kDevBatCancel __attribute__((unused)) = {236, 212, 76, 20};
#define DEV_BAT_ARROW_SLOP 3    /* the quick ball's arrows answer this much round themselves (on their key) */

/* The quick ball's arrow (previous, or `next`) on its key r: the key's lower corners, under the ball. */
static inline CtrPokeTouchRect DevBatBallArrow(const CtrPokeTouchRect *r, int next)
{
    CtrPokeTouchRect a = {(int16_t)(next ? r->x + r->w - 22 : r->x + 2), (int16_t)(r->y + 38), 20, 20};

    return a;
}

/* ---- The Party (boards B · POKeMON and B · POKeMON: menu de ações, 3ds_device_party_impl.h) ------------------------------
 * The field's and the battle's alike. The six bays two to a row in the game's order, the screen under them with what the
 * Party says, CANCEL beside it (picking several: CONFIRM over a shorter CANCEL). With the action menu or YES / NO open
 * (n rows) the bays become a column of slim ones and the rows keys beside them: one column of up to four as on the
 * board, two columns beyond with the slim bays narrowed, as the game's D-pad walks them (3ds_party_grid.h, recipe 0257);
 * the menu's screen takes the whole width under them. A Rare Candy's level up: a tall screen, OK in CANCEL's place. */
static const CtrPokeTouchRect kDevPtyNote __attribute__((unused)) = {8, 180, 222, 52};
static const CtrPokeTouchRect kDevPtyCancel __attribute__((unused)) = {238, 180, 74, 50};
static const CtrPokeTouchRect kDevPtyConfirm __attribute__((unused)) = {238, 180, 74, 22};
static const CtrPokeTouchRect kDevPtyCancelLow __attribute__((unused)) = {238, 206, 74, 24};
static const CtrPokeTouchRect kDevPtyMenuNote __attribute__((unused)) = {8, 182, 304, 50};
static const CtrPokeTouchRect kDevPtyLevelScreen __attribute__((unused)) = {8, 8, 304, 166};

/* Slot i's bay (0-5): left to right, then down. */
static inline CtrPokeTouchRect DevPtyBay(int i)
{
    CtrPokeTouchRect r = {(int16_t)(i & 1 ? 164 : 8), (int16_t)(8 + 56 * (i >> 1)), 148, 50};

    return r;
}

/* Slot i's slim bay while a menu of n rows is open: the board's 188 wide beside one column, 116 beside two. */
static inline CtrPokeTouchRect DevPtySlim(int n, int i)
{
    CtrPokeTouchRect r = {8, (int16_t)(8 + 28 * i), (int16_t)(n <= 4 ? 188 : 116), 24};

    return r;
}

/* Row i's key of a menu of n rows: its column and row are the game's grid's (CtrPartyGrid_Col / _Row). */
static inline CtrPokeTouchRect DevPtyMenuKey(int n, int i)
{
    int col = CtrPartyGrid_Col(n, i), row = CtrPartyGrid_Row(n, i);
    CtrPokeTouchRect r = {(int16_t)(CtrPartyGrid_Cols(n) == 2 ? 132 + 94 * col : 206), (int16_t)(8 + 28 * row),
                          (int16_t)(CtrPartyGrid_Cols(n) == 2 ? 86 : 106), 24};

    return r;
}

/* ---- The Summary (boards B · Resumo: INFO, STATS, MOVES; 3ds_device_summary_impl.h) -----------------------------------
 * The four tabs (INFO, STATS, MOVES, MEMO: SUI_TAB_* order) and the previous / next Pokemon (dark keys) across the top;
 * the screen, its paper's left column (DEV_PAPER2) with the Pokemon (its picture in kDevSumPic, the 80x80 one centred on
 * it) and the page's rows on the right; under it the note and BACK. MOVES and the move details list the moves in
 * DevSumRow(0..3), the details' CANCEL in DevSumRow(4) (rows: no drop). STATS' switch (STATS | IV/EV) is drawn in
 * kDevSumIvSwitch and taken in the larger kDevSumIvSwitchHit. */
static const CtrPokeTouchRect kDevSumTab[4] __attribute__((unused)) = {
    {8, 8, 56, 24}, {68, 8, 56, 24}, {128, 8, 56, 24}, {188, 8, 56, 24},
};
static const CtrPokeTouchRect kDevSumPrev __attribute__((unused)) = {252, 8, 28, 24};
static const CtrPokeTouchRect kDevSumNext __attribute__((unused)) = {284, 8, 28, 24};
static const CtrPokeTouchRect kDevSumScreen __attribute__((unused)) = {8, 40, 304, 140};
static const CtrPokeTouchRect kDevSumColumn __attribute__((unused)) = {14, 46, 104, 128};
static const CtrPokeTouchRect kDevSumPic __attribute__((unused)) = {34, 82, 64, 64};
static const CtrPokeTouchRect kDevSumNote __attribute__((unused)) = {8, 188, 228, 44};
static const CtrPokeTouchRect kDevSumBack __attribute__((unused)) = {244, 188, 68, 42};
static const CtrPokeTouchRect kDevSumIvSwitch __attribute__((unused)) = {228, 49, 70, 13};
static const CtrPokeTouchRect kDevSumIvSwitchHit __attribute__((unused)) = {224, 46, 78, 20};

/* Row i (0-3 a move, 4 the details' CANCEL) of MOVES and the move details. */
static inline CtrPokeTouchRect DevSumRow(int i)
{
    CtrPokeTouchRect r = {124, (int16_t)(52 + 24 * i), 176, 22};

    return r;
}

/* ---- The Bag (boards B · BAG and B · BAG: menu do item; 3ds_device_bag_impl.h) -----------------------------------------
 * One page for the field's Bag, its TM Case and Berry Pouch, a mart's BUY, the item PC's WITHDRAW ITEM, the mailbox and
 * the battle's Bag. Across the top the keys: the Bag's three pockets (the battle's: ITEMS and BERRIES), a case's one key
 * (kDevBagCaseKey), or a title on a screen (kDevBagTitle: a mart, the item PC, the mailbox). Under them the list's screen
 * with its rows (DevBagRow: no drop) and the tall ▲▼ beside it; under that the description's screen (the item's icon
 * in kDevBagDescIcon, the text from kDevBagDescText) and CLOSE, a dark key. With the item's menu, YES / NO or how many
 * open, the list's screen and the description's narrow (kDevBagListNarrow, kDevBagDescNarrow) and the keys stand in a
 * column on the right: the menu's (DevBagMenuKey), YES / NO (DevBagMenuKey(2, i)), or how many (kDevBagQty*). */
#define DEV_BAG_ROW_H 16
static const CtrPokeTouchRect kDevBagPocket[3] __attribute__((unused)) = {{8, 8, 96, 24}, {112, 8, 96, 24}, {216, 8, 96, 24}};
static const CtrPokeTouchRect kDevBagCaseKey __attribute__((unused)) = {8, 8, 304, 24};
static const CtrPokeTouchRect kDevBagTitle __attribute__((unused)) = {8, 6, 304, 28};
static const CtrPokeTouchRect kDevBagList __attribute__((unused)) = {8, 40, 260, 144};
static const CtrPokeTouchRect kDevBagListNarrow __attribute__((unused)) = {8, 40, 184, 144};
static const CtrPokeTouchRect kDevBagUp __attribute__((unused)) = {276, 40, 36, 66};
static const CtrPokeTouchRect kDevBagDown __attribute__((unused)) = {276, 114, 36, 66};
static const CtrPokeTouchRect kDevBagDesc __attribute__((unused)) = {8, 190, 228, 42};
static const CtrPokeTouchRect kDevBagDescNarrow __attribute__((unused)) = {8, 190, 184, 42};
static const CtrPokeTouchRect kDevBagDescIcon __attribute__((unused)) = {18, 199, 24, 24};
static const CtrPokeTouchRect kDevBagDescText __attribute__((unused)) = {46, 197, 180, 27};
static const CtrPokeTouchRect kDevBagClose __attribute__((unused)) = {244, 190, 68, 40};
/* How many: the count's screen between ▲ and ▼, OK and CANCEL under them. */
static const CtrPokeTouchRect kDevBagQtyUp __attribute__((unused)) = {200, 40, 112, 30};
static const CtrPokeTouchRect kDevBagQtyScreen __attribute__((unused)) = {200, 76, 112, 40};
static const CtrPokeTouchRect kDevBagQtyDown __attribute__((unused)) = {200, 122, 112, 30};
static const CtrPokeTouchRect kDevBagQtyOk __attribute__((unused)) = {200, 158, 112, 30};
static const CtrPokeTouchRect kDevBagQtyCancel __attribute__((unused)) = {200, 194, 112, 36};

/* Row i of a list h tall a row on kDevBagList's paper, the wide one or (`narrow`) kDevBagListNarrow's. */
static inline CtrPokeTouchRect DevListRow(int i, int narrow, int h)
{
    CtrPokeTouchRect r = {16, (int16_t)(48 + h * i), (int16_t)(narrow ? 168 : 244), (int16_t)h};

    return r;
}

/* Row i (0-7) of the list, on the wide screen or (`narrow`) the narrow one. */
static inline CtrPokeTouchRect DevBagRow(int i, int narrow)
{
    return DevListRow(i, narrow, DEV_BAG_ROW_H);
}

/* Row i (0-4) of a list of tall rows (24 px) on the same papers. */
static inline CtrPokeTouchRect DevTallRow(int i, int narrow)
{
    return DevListRow(i, narrow, 24);
}

/* Key i of a menu of n (1-6): one column, 38 apart up to five keys (the last, a dark one, 38 tall, the others 30), 32
 * apart and 26 tall for six. */
static inline CtrPokeTouchRect DevBagMenuKey(int n, int i)
{
    int p = n <= 5 ? 38 : 32, h = n <= 5 ? (i == n - 1 ? 38 : 30) : 26;
    CtrPokeTouchRect r = {200, (int16_t)(40 + p * i), 112, (int16_t)h};

    return r;
}

/* ---- The shapes the views without a board share (plan 3) ------------------------------------------------------------
 * No board draws these views: they reuse the boarded pages' shapes by name (the Bag's list page, the Summary's screen
 * and note, HOME's key places) and these few. Keys across the screen (DevAcross: the Bag's pockets, the battle's rows);
 * the small note (a screen) with two small keys and the Bag's CLOSE beside it; a tall screen over a wide key; a
 * question's note beside YES and NO in HOME's third and sixth key places. */
static const CtrPokeTouchRect kDevSmallNote __attribute__((unused)) = {8, 190, 152, 42};
static const CtrPokeTouchRect kDevSmallKey[2] __attribute__((unused)) = {{168, 190, 32, 40}, {204, 190, 32, 40}};
static const CtrPokeTouchRect kDevTallScreen __attribute__((unused)) = {8, 8, 304, 172};
static const CtrPokeTouchRect kDevWideKey __attribute__((unused)) = {8, 188, 304, 42};
static const CtrPokeTouchRect kDevAskNote __attribute__((unused)) = {8, 156, 200, 76};
static const CtrPokeTouchRect kDevAskYes __attribute__((unused)) = {216, 156, 96, 36};
static const CtrPokeTouchRect kDevAskNo __attribute__((unused)) = {216, 198, 96, 34};

/* Key i of n (1-8) across x 8..312, at y and h tall: 8 px apart up to three keys, 4 beyond. (n 3 at y 8 are the Bag's
 * pockets, n 4 at y 170 the battle's wild row, n 2 at y 72 its moves' top row.) */
static inline CtrPokeTouchRect DevAcross(int n, int i, int y, int h)
{
    int g = n <= 3 ? 8 : 4, w = (304 - g * (n - 1)) / n;
    CtrPokeTouchRect r = {(int16_t)(8 + (w + g) * i), (int16_t)y, (int16_t)w, (int16_t)h};

    return r;
}

/* ---- OPTIONS (no board; 3ds_device_options_impl.h) --------------------------------------------------------------------
 * The Bag's list page: the four sections across the top (DevAcross(4, s, 8, 24)), the rows on kDevBagList's paper
 * (DevOptRow) with ▲ ▼ (kDevBagUp, kDevBagDown) paging beside them, the description in kDevSmallNote, ◀ ▶ (kDevSmallKey)
 * stepping the row described, BACK in kDevBagClose. A row is three zones: LABEL (its words, from 6 px in), PREV (◀) and
 * NEXT (the value, centred in DEV_OPT_VALUE_W, and ▶); a row of levels shares PREV and NEXT among its levels. FRAME's
 * sample under the fifth row, the look's panel under the fourth. */
#define DEV_OPT_ROW_H     18
#define DEV_OPT_LABEL_W   120                               /* LABEL: x 16..135 */
#define DEV_OPT_PREV_X    136                               /* PREV: x 136..159 (◀ at its middle) */
#define DEV_OPT_NEXT_X    160                               /* NEXT: x 160..259 (the value in 160..243, ▶ at 252) */
#define DEV_OPT_VALUE_W   84
#define DEV_OPT_NEXT_ARROW 252
static const CtrPokeTouchRect kDevOptSample __attribute__((unused)) = {202, 138, 56, 40};
static const CtrPokeTouchRect kDevOptLook __attribute__((unused)) = {16, 122, 244, 54};

/* Section key s (0-3) of OPTIONS. */
static inline CtrPokeTouchRect DevOptSection(int s)
{
    return DevAcross(4, s, 8, 24);
}

/* Row i (0 to PT_OPT_MAX - 1) of OPTIONS. */
static inline CtrPokeTouchRect DevOptRow(int i)
{
    return DevListRow(i, 0, DEV_OPT_ROW_H);
}

/* Level k of a row of `count` levels on row i: PREV and NEXT shared out from x 136, a level taking the columns whose
 * (x - 136) * count / 124 is k (CtrPokeTouch_OptionSegment's). */
static inline CtrPokeTouchRect DevOptLevel(int i, int k, int count)
{
    CtrPokeTouchRect r = DevOptRow(i);
    int w = r.x + r.w - DEV_OPT_PREV_X, x0 = DEV_OPT_PREV_X + (w * k + count - 1) / count;

    r.w = (int16_t)(DEV_OPT_PREV_X + (w * (k + 1) + count - 1) / count - x0);
    r.x = (int16_t)x0;
    return r;
}

#endif
