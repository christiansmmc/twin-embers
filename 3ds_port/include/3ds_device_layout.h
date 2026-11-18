#ifndef CTR_DEVICE_LAYOUT_H
#define CTR_DEVICE_LAYOUT_H

#include "3ds_poketouch.h"
#include "3ds_party_grid.h"     /* the Party's menu: its count, one column */

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

/* ---- The grid (def_spec: the owner's final design, "B — Definitiva") -------------------------------------------------
 * Three bands. TOPO, y 8-32: the tabs (DevGridTab) or a title on a screen (kDevGridTitle). MEIO, y 40-184: the screen
 * with ▲▼ beside it (kDevGridScreen, kDevGridUp, kDevGridDown), the wide screen (kDevGridWide), or a narrow screen
 * with a column of keys at x 204 (kDevGridScreenNarrow, DevGridColumnKey); a list's rows on the screen's paper,
 * DEV_GRID_ROWS shown, DEV_GRID_ROW_H tall and DEV_GRID_ROW_PITCH apart (DevGridRow; the pitch is the drag constants'
 * too). RODAPÉ, y 190-232: the note with the exit beside it (kDevGridNote, kDevGridExit: BACK, CLOSE or CANCEL, a dark
 * key) or the wide note (kDevGridNoteWide). */
#define DEV_GRID_ROW_H     24
#define DEV_GRID_ROW_PITCH 26
#define DEV_GRID_ROWS      5
static const CtrPokeTouchRect kDevGridTitle __attribute__((unused)) = {8, 6, 304, 28};
static const CtrPokeTouchRect kDevGridScreen __attribute__((unused)) = {8, 40, 260, 144};
static const CtrPokeTouchRect kDevGridScreenNarrow __attribute__((unused)) = {8, 40, 188, 144};
static const CtrPokeTouchRect kDevGridWide __attribute__((unused)) = {8, 40, 304, 144};
static const CtrPokeTouchRect kDevGridUp __attribute__((unused)) = {276, 40, 36, 66};
static const CtrPokeTouchRect kDevGridDown __attribute__((unused)) = {276, 114, 36, 66};
static const CtrPokeTouchRect kDevGridNote __attribute__((unused)) = {8, 190, 228, 42};
static const CtrPokeTouchRect kDevGridNoteWide __attribute__((unused)) = {8, 190, 304, 42};
static const CtrPokeTouchRect kDevGridExit __attribute__((unused)) = {244, 190, 68, 40};

/* Tab i of n across x 8..312 at the top, 4 px apart (n 3: 98 wide, the last ending at 310). */
static inline CtrPokeTouchRect DevGridTab(int n, int i)
{
    int w = (304 - 4 * (n - 1)) / n;
    CtrPokeTouchRect r = {(int16_t)(8 + (w + 4) * i), 8, (int16_t)w, 24};

    return r;
}

/* Row i (0 to DEV_GRID_ROWS - 1) of a list on kDevGridScreen's paper, or (`narrow`) kDevGridScreenNarrow's. */
static inline CtrPokeTouchRect DevGridRow(int i, int narrow)
{
    CtrPokeTouchRect r = {16, (int16_t)(48 + DEV_GRID_ROW_PITCH * i), (int16_t)(narrow ? 172 : 244), DEV_GRID_ROW_H};

    return r;
}

/* Key i of the column beside the narrow screen: from `top`, `pitch` apart, h tall. */
static inline CtrPokeTouchRect DevGridColumnKey(int top, int pitch, int h, int i)
{
    CtrPokeTouchRect r = {204, (int16_t)(top + pitch * i), 108, (int16_t)h};

    return r;
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

/* ---- The Party (boards def_party and def_party_menu, 3ds_device_party_impl.h) -----------------------------------------
 * The field's and the battle's alike. The six bays two to a row in the game's order; under them the grid's note with
 * what the Party says and CANCEL, the dark key in the exit corner (kDevGridNote, kDevGridExit); picking several (the
 * Decisions) the small note (kDevSmallNote), CONFIRM 8 px after it and CANCEL in the exit corner. With the action menu or
 * YES / NO open the bays become a column of slim ones (DevPtySlim) and the actions keys in ONE column beside them
 * (DevPtyMenuKey; the owner's O2), DEV_PTY_MENU_ROWS shown, a window that follows FireRed's cursor, the game's D-pad its
 * own list input (3ds_party_grid.h); FireRed's CANCEL is the dark key in the exit corner (kDevGridExit), YES / NO two
 * keys in the column and no exit key; with more actions than the column shows, ▲▼ over and under it say where more are
 * (DEV_PTY_CUE_*: drawn, never hit); the Party's words on the grid's note. A Rare Candy's level up: a tall screen over
 * the grid's note, OK in the exit corner. */
static const CtrPokeTouchRect kDevPtyConfirm __attribute__((unused)) = {168, 190, 68, 40};
static const CtrPokeTouchRect kDevPtyLevelScreen __attribute__((unused)) = {8, 8, 304, 176};
#define DEV_PTY_MENU_ROWS  6    /* the action keys the column shows */
#define DEV_PTY_CUE_X      258  /* the cue's ▲ and ▼ centred over and under the column (Call 10) */
#define DEV_PTY_CUE_UP_Y   4
#define DEV_PTY_CUE_DOWN_Y 182

/* Slot i's bay (0-5): left to right, then down. */
static inline CtrPokeTouchRect DevPtyBay(int i)
{
    CtrPokeTouchRect r = {(int16_t)(i & 1 ? 164 : 8), (int16_t)(8 + 56 * (i >> 1)), 148, 50};

    return r;
}

/* Slot i's slim bay (0-5) while a menu is open: the board's, 188 wide, 28 apart. */
static inline CtrPokeTouchRect DevPtySlim(int i)
{
    CtrPokeTouchRect r = {8, (int16_t)(8 + 28 * i), 188, 24};

    return r;
}

/* Key k (0 to DEV_PTY_MENU_ROWS - 1) of the menu's column: the board's, 108 wide at x 204, 28 apart. */
static inline CtrPokeTouchRect DevPtyMenuKey(int k)
{
    CtrPokeTouchRect r = {204, (int16_t)(8 + 28 * k), 108, 24};

    return r;
}

/* ---- The Summary (boards def_sum_info, def_sum_stats, def_sum_moves; 3ds_device_summary_impl.h) -----------------------
 * On the grid: the four tabs (INFO, STATS, MOVES, MEMO: SUI_TAB_* order) are DevGridTab(4, i); the previous and next
 * Pokemon the grid's ▲▼ (kDevGridUp, kDevGridDown) beside its screen (kDevGridScreen); the note and BACK the grid's
 * (kDevGridNote, kDevGridExit). On the screen's paper the left column (DEV_PAPER2) with the Pokemon (its picture in
 * kDevSumPic, the 80x80 one centred on it) and the page right of it. MOVES and the move details list the moves in
 * DevSumRow(0..3), the details' CANCEL in DevSumRow(4) (rows: no drop). STATS' switch (STATS | IV/EV) is drawn in
 * kDevSumIvSwitch (70 px, ending at the page's right, x 256: Call 17) and taken in the larger kDevSumIvSwitchHit. */
static const CtrPokeTouchRect kDevSumColumn __attribute__((unused)) = {14, 46, 96, 132};
static const CtrPokeTouchRect kDevSumPic __attribute__((unused)) = {30, 80, 64, 64};
static const CtrPokeTouchRect kDevSumIvSwitch __attribute__((unused)) = {186, 49, 70, 13};
static const CtrPokeTouchRect kDevSumIvSwitchHit __attribute__((unused)) = {182, 46, 78, 20};

/* Row i (0-3 a move, 4 the details' CANCEL) of MOVES and the move details: the grid's rows (24 tall, 26 apart), 4 px
 * right of the column. */
static inline CtrPokeTouchRect DevSumRow(int i)
{
    CtrPokeTouchRect r = {114, (int16_t)(48 + DEV_GRID_ROW_PITCH * i), 146, DEV_GRID_ROW_H};

    return r;
}

/* ---- The Bag (boards B · BAG and B · BAG: menu do item; 3ds_device_bag_impl.h) -----------------------------------------
 * One page for the field's Bag, its TM Case and Berry Pouch, a mart's BUY, the item PC's WITHDRAW ITEM, the mailbox and
 * the battle's Bag. Across the top the keys: the Bag's three pockets (the grid's tabs, DevGridTab(3, i); the battle's:
 * ITEMS and BERRIES in the first two), a case's one key (kDevBagCaseKey), or a title on a screen (kDevGridTitle: a mart,
 * the item PC, the mailbox). Under them the grid's screen with the list's five rows (DevGridRow: no drop) and the tall
 * ▲▼ beside it (kDevGridScreen, kDevGridUp, kDevGridDown); under that the description on the grid's note (the item's
 * icon in kDevBagDescIcon, the text beside it) and CLOSE, a dark key, in the grid's exit (kDevGridNote, kDevGridExit).
 * With the item's menu, YES / NO or how many open (board B · BAG: menu do item), the list's screen narrows
 * (kDevGridScreenNarrow and its rows), the note stays the grid's, and the keys stand in the grid's column at x 204: the
 * menu's actions (DevBagMenuKey), YES (DevGridColumnKey(40, 36, 30, 0)), or how many (kDevBagQty*); FireRed's last row
 * (CANCEL, EXIT), NO and how many's CANCEL are the dark key in the grid's exit (kDevGridExit). */
static const CtrPokeTouchRect kDevBagCaseKey __attribute__((unused)) = {8, 8, 304, 24};
static const CtrPokeTouchRect kDevBagDescIcon __attribute__((unused)) = {18, 199, 24, 24};
/* How many, in the grid's column: ▲, the count's screen, ▼ and OK (CANCEL: the grid's exit). */
static const CtrPokeTouchRect kDevBagQtyUp __attribute__((unused)) = {204, 40, 108, 30};
static const CtrPokeTouchRect kDevBagQtyScreen __attribute__((unused)) = {204, 76, 108, 36};
static const CtrPokeTouchRect kDevBagQtyDown __attribute__((unused)) = {204, 118, 108, 30};
static const CtrPokeTouchRect kDevBagQtyOk __attribute__((unused)) = {204, 154, 108, 30};

/* Key i (i < n - 1) of the item's menu of n rows (2-6) in the grid's column: up to four keys 30 tall and 36 apart, five
 * 24 tall and 28 apart. FireRed's last row (CANCEL, EXIT) is the grid's exit, kDevGridExit. */
static inline CtrPokeTouchRect DevBagMenuKey(int n, int i)
{
    return n - 1 <= 4 ? DevGridColumnKey(40, 36, 30, i) : DevGridColumnKey(40, 28, 24, i);
}

/* ---- The shapes the views without a board share (plan 3) ------------------------------------------------------------
 * No board draws these views: they draw on the grid's names and these few. Keys across the screen (DevAcross: the
 * battle's rows); a list's rows h tall on the grid's screen's paper (DevListRow; DevTallRow: the Pokedex's list, the
 * relearner's); the small note (a screen) with two small keys and the grid's exit beside it (the Party picking several:
 * its CONFIRM, kDevPtyConfirm, in the keys' place); a tall screen over a wide
 * key; the tall note with BACK beside it (kDevTallNote, kDevTallBack: the frame's, the move to forget's, the Trainer
 * Card's and the Pokedex page's BACK until their boards' tasks); a question's note beside YES and
 * NO (where HOME's third and sixth keys are). */
static const CtrPokeTouchRect kDevSmallNote __attribute__((unused)) = {8, 190, 152, 42};
static const CtrPokeTouchRect kDevSmallKey[2] __attribute__((unused)) = {{168, 190, 32, 40}, {204, 190, 32, 40}};
static const CtrPokeTouchRect kDevTallScreen __attribute__((unused)) = {8, 8, 304, 172};
static const CtrPokeTouchRect kDevTallNote __attribute__((unused)) = {8, 188, 228, 44};
static const CtrPokeTouchRect kDevTallBack __attribute__((unused)) = {244, 188, 68, 42};
static const CtrPokeTouchRect kDevWideKey __attribute__((unused)) = {8, 188, 304, 42};
static const CtrPokeTouchRect kDevAskNote __attribute__((unused)) = {8, 156, 200, 76};
static const CtrPokeTouchRect kDevAskYes __attribute__((unused)) = {216, 156, 96, 36};
static const CtrPokeTouchRect kDevAskNo __attribute__((unused)) = {216, 198, 96, 34};

/* Row i of a list h tall a row on kDevGridScreen's paper, or (`narrow`) on the paper of a screen 184 wide (the
 * relearner's kDevRlNarrow). */
static inline CtrPokeTouchRect DevListRow(int i, int narrow, int h)
{
    CtrPokeTouchRect r = {16, (int16_t)(48 + h * i), (int16_t)(narrow ? 168 : 244), (int16_t)h};

    return r;
}

/* Row i (0-4) of a list of tall rows (24 px, no gap) on the same papers: the Pokedex's list, the relearner's. */
static inline CtrPokeTouchRect DevTallRow(int i, int narrow)
{
    return DevListRow(i, narrow, 24);
}

/* Key i of n (1-8) across x 8..312, at y and h tall: 8 px apart up to three keys, 4 beyond. (n 3 at y 8 are the
 * Pokedex's tabs, DevDexTabKey; n 4 at y 170 the battle's wild row, n 2 at y 72 its moves' top row.) */
static inline CtrPokeTouchRect DevAcross(int n, int i, int y, int h)
{
    int g = n <= 3 ? 8 : 4, w = (304 - g * (n - 1)) / n;
    CtrPokeTouchRect r = {(int16_t)(8 + (w + g) * i), (int16_t)y, (int16_t)w, (int16_t)h};

    return r;
}

/* ---- OPTIONS (no board; 3ds_device_options_impl.h) --------------------------------------------------------------------
 * The grid's list page: the four sections across the top (DevAcross(4, s, 8, 24)), the rows on kDevGridScreen's paper
 * (DevOptRow) with ▲ ▼ (kDevGridUp, kDevGridDown) paging beside them, the description in kDevSmallNote, ◀ ▶
 * (kDevSmallKey) stepping the row described, BACK in kDevGridExit. A row is three zones: LABEL (its words, from 6 px
 * in), PREV (◀) and NEXT (the value, centred in DEV_OPT_VALUE_W, and ▶); a row of levels shares PREV and NEXT among its
 * levels. FRAME's sample under the fifth row, the look's panel under the fourth. */
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

/* ---- SAVE, the Trainer Card, the letter (no board; 3ds_device_save_impl.h, _card_impl.h, _mail_impl.h) ---------------
 * SAVE: HOME's screen holds the save's facts, a row each (DevSaveRow: the label small at its left, the value at its
 * right) and the player's map icon at 4x (kDevSaveSprite); under it the question's note, YES and NO beside it (a result's
 * OK alone in YES's place). The Trainer Card: the tall screen, FLIP under it and BACK beside FLIP (kDevTallBack). Its
 * front: the first line (DevCardRow(0): NAME and the name, IDNo. and the ID), the rows beside the trainer's picture
 * (DevCardRow(1-3): MONEY, POKeDEX, TIME; kDevCardPic), the stars (DEV_CARD_STAR_*) and the eight badges' places
 * (DevCardBadge); its back: the name on the first line, the rule under it (kDevCardRule) and the records
 * (DevCardBackRow). The letter: the tall screen (a touch of it closes the letter) with the paper's icon (kDevMailIcon),
 * its name (kDevMailName), a rule (kDevMailRule) and who wrote it (kDevMailFrom); CLOSE, the wide key, under it. */
static const CtrPokeTouchRect kDevSaveSprite __attribute__((unused)) = {224, 46, 64, 64};
static const CtrPokeTouchRect kDevCardFlip __attribute__((unused)) = {8, 188, 228, 42};
static const CtrPokeTouchRect kDevCardPic __attribute__((unused)) = {226, 40, 64, 64};
static const CtrPokeTouchRect kDevCardRule __attribute__((unused)) = {22, 40, 276, 1};
static const CtrPokeTouchRect kDevMailIcon __attribute__((unused)) = {148, 40, 24, 24};
static const CtrPokeTouchRect kDevMailName __attribute__((unused)) = {22, 74, 276, 18};
static const CtrPokeTouchRect kDevMailRule __attribute__((unused)) = {80, 98, 160, 1};
static const CtrPokeTouchRect kDevMailFrom __attribute__((unused)) = {22, 106, 276, 18};
#define DEV_CARD_STARS     5    /* the most a card has: the first's top-left at (X, Y), each STEP right of the one before */
#define DEV_CARD_STAR_X    22
#define DEV_CARD_STAR_Y    108
#define DEV_CARD_STAR_STEP 13

/* Row i (0-4: PLAYER, BADGES, POKeDEX, TIME, MONEY) of SAVE's facts. */
static inline CtrPokeTouchRect DevSaveRow(int i)
{
    CtrPokeTouchRect r = {22, (int16_t)(18 + 24 * i), 192, 24};

    return r;
}

/* The card's front: its first line (i 0, across the screen) and rows 1-3 beside the picture. */
static inline CtrPokeTouchRect DevCardRow(int i)
{
    CtrPokeTouchRect r = {22, (int16_t)(18 + 22 * i), (int16_t)(i ? 196 : 276), 22};

    return r;
}

/* The card's back: record i (0 to PT_CARD_ROWS - 1) under the rule. */
static inline CtrPokeTouchRect DevCardBackRow(int i)
{
    CtrPokeTouchRect r = {22, (int16_t)(46 + 24 * i), 276, 24};

    return r;
}

/* The place of badge i (0-7) on the card's front: the badge at 2x, or the place empty. */
static inline CtrPokeTouchRect DevCardBadge(int i)
{
    CtrPokeTouchRect r = {(int16_t)(20 + 34 * i), 132, 32, 32};

    return r;
}

/* ---- The Pokedex (no board; 3ds_device_dex_impl.h) ------------------------------------------------------------------
 * The top menu: its three sections as tabs (DevDexTabKey), the shown section's rows as keys six to a page (DevDexItem:
 * FireRed's menu icon over the row's name), the counts in the small note with ▲ ▼ (kDevSmallKey) turning the pages,
 * CLOSE (FireRed's CLOSE POKeDEX row) in the grid's exit (kDevGridExit). An ordered list: its order on the grid's
 * title, the grid's screen with five tall rows (DevTallRow) and ▲ ▼, the selected Pokemon's types and kind on the
 * grid's note, BACK in the exit. A habitat's page: the habitat and the page on the title, its Pokemon in four slots
 * (DevDexSlot: the picture beside the number and the name), the selected one in the small note, ◀ ▶ (the small keys)
 * and BACK. A Pokemon's page: the tall screen (the picture in kDevDexPic, the number, name, kind, types, height and
 * weight in kDevDexData, the footprint in kDevDexFoot, the entry in kDevDexEntry under kDevDexRule), AREA, CRY and the
 * BACK (kDevTallBack) under it; the page a catch has just added: OK, the wide key, instead. */
static const CtrPokeTouchRect kDevDexPic __attribute__((unused)) = {20, 20, 80, 80};
static const CtrPokeTouchRect kDevDexData __attribute__((unused)) = {110, 20, 168, 80};
static const CtrPokeTouchRect kDevDexFoot __attribute__((unused)) = {282, 20, 16, 16};
static const CtrPokeTouchRect kDevDexRule __attribute__((unused)) = {20, 104, 280, 1};
static const CtrPokeTouchRect kDevDexEntry __attribute__((unused)) = {20, 108, 280, 65};
static const CtrPokeTouchRect kDevDexArea __attribute__((unused)) = {8, 188, 110, 42};
static const CtrPokeTouchRect kDevDexCry __attribute__((unused)) = {126, 188, 110, 42};

/* Tab i (0-2) of the top menu: three across, 8 px apart (today's places). */
static inline CtrPokeTouchRect DevDexTabKey(int i)
{
    return DevAcross(3, i, 8, 24);
}

/* Key i (0-5) of the top menu's page: three across, two rows. */
static inline CtrPokeTouchRect DevDexItem(int i)
{
    return DevAcross(3, i % 3, 40 + 72 * (i / 3), 68);
}

/* Slot i (0-3) of a habitat's page: two across, two rows. */
static inline CtrPokeTouchRect DevDexSlot(int i)
{
    CtrPokeTouchRect r = {(int16_t)(8 + 156 * (i % 2)), (int16_t)(40 + 74 * (i / 2)), 148, 70};

    return r;
}

/* ---- The field's choices, the move to forget, the relearner (no board; 3ds_device_choice_impl.h, _learn_impl.h,
 * _relearn_impl.h) ---------------------------------------------------------------------------------------------------
 * A choice: FireRed's rows as keys across the screen (DevChoiceKey), the message box's question on the note under them
 * (kDevChoiceNote) and BACK beside it. The move to forget: FireRed's question on the grid's title, its screen
 * (kDevLearnScreen: the Pokemon in its column, kDevLearnColumn, its picture in kDevLearnPic; the four moves and the new
 * one in DevLearnSlot(0..4)), the tall note (the description) and BACK. The relearner is the grid's list page: the
 * question on the title, five tall rows with ▲ ▼, the description on the note and CANCEL (FireRed's CANCEL row) in the
 * exit; YES / NO beside the narrow screens (kDevRlNarrow, kDevRlDescNarrow) in DevRlKey(i). Each its own copy of the
 * numbers it drew with (the Summary's and the Bag's), so neither moves with them. */
static const CtrPokeTouchRect kDevChoiceNote __attribute__((unused)) = {8, 160, 228, 72};
static const CtrPokeTouchRect kDevChoiceBack __attribute__((unused)) = {244, 160, 68, 70};
static const CtrPokeTouchRect kDevLearnScreen __attribute__((unused)) = {8, 40, 304, 140};
static const CtrPokeTouchRect kDevLearnColumn __attribute__((unused)) = {14, 46, 104, 128};
static const CtrPokeTouchRect kDevLearnPic __attribute__((unused)) = {34, 82, 64, 64};
static const CtrPokeTouchRect kDevRlNarrow __attribute__((unused)) = {8, 40, 184, 144};
static const CtrPokeTouchRect kDevRlDescNarrow __attribute__((unused)) = {8, 190, 184, 42};

/* Slot i (0-3 a move, 4 the new one) of the move to forget. */
static inline CtrPokeTouchRect DevLearnSlot(int i)
{
    CtrPokeTouchRect r = {124, (int16_t)(52 + 24 * i), 176, 22};

    return r;
}

/* The relearner's YES (0) and NO (1) beside its narrow screens. */
static inline CtrPokeTouchRect DevRlKey(int i)
{
    CtrPokeTouchRect r = {200, (int16_t)(i ? 78 : 40), 112, (int16_t)(i ? 38 : 30)};

    return r;
}

/* Key i of a choice of n (1-8): FireRed's one column (cols 0, or a grid of one) in one column up to four keys and in two
 * beyond, going down first as FireRed's list does; a grid of `cols` columns across, as FireRed's grid goes. Up to three
 * rows 40 tall, four 32, 4 px apart from the top. */
static inline CtrPokeTouchRect DevChoiceKey(int n, int cols, int i)
{
    int c, rows, h, col, row;

    if (cols < 2)
        cols = 0;
    c = cols > 0 ? cols : n > 4 ? 2 : 1;
    rows = (n + c - 1) / c;
    if (rows < 1)
        rows = 1;
    col = cols > 0 ? i % c : i / rows;
    row = cols > 0 ? i / c : i % rows;
    h = rows <= 3 ? 40 : 32;
    return DevAcross(c, col, 8 + (h + 4) * row, h);
}

/* ---- The naming keyboard (no board; 3ds_device_naming_impl.h) --------------------------------------------------------
 * The name's screen across the top (the Pokemon's icon, FireRed's question, the name's slots and the gender sign); under
 * it five keys across (DevNameTop: ABC, abc and 123 turning to FireRed's pages, DELETE, OK); under them the page's keys
 * in four rows (DevNameKey: FireRed's 8 columns, 34 wide, or its symbols' 6, 47 wide). */
static const CtrPokeTouchRect kDevNameScreen __attribute__((unused)) = {8, 8, 304, 40};

/* Key i (0-4) across under the screen: ABC (FireRed's page 1, the capitals), abc (page 2, the small letters), 123 (page
 * 0, the symbols), DELETE, OK. */
static inline CtrPokeTouchRect DevNameTop(int i)
{
    return DevAcross(5, i, 56, 24);
}

/* The key of row r (0-3) and column c of a page of `cols` columns (8, or 6 for the symbols). */
static inline CtrPokeTouchRect DevNameKey(int cols, int r, int c)
{
    return DevAcross(cols, c, 88 + 36 * r, 32);
}

/* ---- The PC box (no board; 3ds_device_box_impl.h) --------------------------------------------------------------------
 * The box's screen on the left: its paper is FireRed's 160x144 wallpaper at 1:1 (owner decision 2), the box's name on
 * the wallpaper's own band at its top (kDevBoxTitle, a row), the thirty places 24 px apart under it (DevBoxCell, rows),
 * or with the cursor in the Party the Party's rows on the same paper (DevBoxPartyRow, 6 its CANCEL). The right column:
 * ◀ ▶, PARTY, CLOSE BOX (dark) and the hand's screen; the note across under them all. A menu, YES / NO or the marks:
 * the keys stand in the right column (DevBoxMenuKey) and the note is narrow (kDevBoxNoteNarrow). The box chooser: ◀ ▶
 * and the chosen box's key (kDevBoxChoose) under them, the note narrow and CANCEL (kDevBoxCancel, dark) beside it. */
static const CtrPokeTouchRect kDevBoxScreen __attribute__((unused)) = {8, 8, 172, 156};
static const CtrPokeTouchRect kDevBoxTitle __attribute__((unused)) = {14, 14, 160, 24};
static const CtrPokeTouchRect kDevBoxPrev __attribute__((unused)) = {188, 8, 58, 24};
static const CtrPokeTouchRect kDevBoxNext __attribute__((unused)) = {254, 8, 58, 24};
static const CtrPokeTouchRect kDevBoxParty __attribute__((unused)) = {188, 40, 124, 32};
static const CtrPokeTouchRect kDevBoxClose __attribute__((unused)) = {188, 80, 124, 32};
static const CtrPokeTouchRect kDevBoxHand __attribute__((unused)) = {188, 120, 124, 44};
static const CtrPokeTouchRect kDevBoxNote __attribute__((unused)) = {8, 172, 304, 60};
static const CtrPokeTouchRect kDevBoxNoteNarrow __attribute__((unused)) = {8, 172, 172, 60};
static const CtrPokeTouchRect kDevBoxChoose __attribute__((unused)) = {188, 40, 124, 72};
static const CtrPokeTouchRect kDevBoxCancel __attribute__((unused)) = {188, 172, 124, 56};

/* Place i (0-29) of the box: six across, five down, on the wallpaper where FireRed puts its icons. */
static inline CtrPokeTouchRect DevBoxCell(int i)
{
    CtrPokeTouchRect r = {(int16_t)(22 + 24 * (i % 6)), (int16_t)(38 + 24 * (i / 6)), 24, 24};

    return r;
}

/* The Party's row i (0-5 a slot, 6 its CANCEL) on the box's paper. */
static inline CtrPokeTouchRect DevBoxPartyRow(int i)
{
    CtrPokeTouchRect r = {16, (int16_t)(16 + 20 * i), 156, 20};

    return r;
}

/* Key i of n (1-7) in the right column: 44 apart and 40 tall up to five, 32 apart and 28 tall beyond (the box's menu
 * of up to seven, YES / NO, the marks' six). */
static inline CtrPokeTouchRect DevBoxMenuKey(int n, int i)
{
    int p = n <= 5 ? 44 : 32, h = n <= 5 ? 40 : 28;
    CtrPokeTouchRect r = {188, (int16_t)(8 + p * i), 124, (int16_t)h};

    return r;
}

/* ---- Easy Chat (no board; 3ds_device_easychat_impl.h) ----------------------------------------------------------------
 * The phrase: FireRed's title and the count on the phrase's screen (kDevEcPhrase), the phrase's n slots as keys in two
 * rows (DevEcChip), BY GROUP and A TO Z across under them (DevEcMode), CLEAR, CANCEL (dark) and CONFIRM across the
 * bottom (DevEcFooter). The groups: the title on the grid's title, the groups' names as keys four across (DevEcCell); the
 * letters: A..Z as keys in FireRed's rows of 6, 6, 7 and 7 (DevEcLetter); both with the toggle and BACK (dark) across the
 * bottom (DevEcToggle). A list: its title over the phrase on two lines (kDevEcListTitle), the words two a row
 * (DevEcRow: rows) on the list's paper (kDevEcList) with the rail on the same paper (kDevEcRail), ▲ ▼ (kDevEcUp,
 * kDevEcDown), the letter strip (kDevEcStrip, a screen; its letters in DevEcStripCell), USE WORD, the toggle and BACK
 * (dark, at the right) across the bottom (DevEcListKey). A question: the phrase's screen and slots dimmed, the question's note, YES and
 * NO (kDevAskNote, kDevAskYes, kDevAskNo). */
static const CtrPokeTouchRect kDevEcPhrase __attribute__((unused)) = {8, 8, 304, 36};
static const CtrPokeTouchRect kDevEcListTitle __attribute__((unused)) = {8, 6, 304, 40};
static const CtrPokeTouchRect kDevEcList __attribute__((unused)) = {8, 52, 236, 136};
static const CtrPokeTouchRect kDevEcRail __attribute__((unused)) = {220, 58, 18, 124};
static const CtrPokeTouchRect kDevEcUp __attribute__((unused)) = {252, 52, 28, 64};
static const CtrPokeTouchRect kDevEcDown __attribute__((unused)) = {252, 120, 28, 64};
static const CtrPokeTouchRect kDevEcStrip __attribute__((unused)) = {284, 52, 28, 136};

/* Slot i of a phrase of n slots (1-9): (n + 1) / 2 across, two rows. */
static inline CtrPokeTouchRect DevEcChip(int n, int i)
{
    int per = (n + 1) / 2;

    return DevAcross(per, i % per, 52 + 40 * (i / per), 34);
}

/* The phrase's mode keys: 0 BY GROUP, 1 A TO Z. */
static inline CtrPokeTouchRect DevEcMode(int i)
{
    return DevAcross(2, i, 136, 30);
}

/* The phrase's buttons: 0 CLEAR, 1 CANCEL, 2 CONFIRM. */
static inline CtrPokeTouchRect DevEcFooter(int i)
{
    return DevAcross(3, i, 178, 52);
}

/* Group i (0-20): four across, six rows at most. */
static inline CtrPokeTouchRect DevEcCell(int i)
{
    return DevAcross(4, i % 4, 40 + 27 * (i / 4), 24);
}

/* Letter i (0-25, A..Z) where FireRed's A-Z keys lie: rows of 6, 6, 7 and 7 (from letters 0, 6, 12, 19), seven places
 * across. */
static inline CtrPokeTouchRect DevEcLetter(int i)
{
    int r = i < 6 ? 0 : i < 12 ? 1 : i < 19 ? 2 : 3;

    return DevAcross(7, i - (6 * r + (r == 3)), 40 + 32 * r, 28);
}

/* The groups' and the letters' bottom keys: 0 the toggle to the other order, 1 BACK. */
static inline CtrPokeTouchRect DevEcToggle(int i)
{
    return DevAcross(2, i, 206, 26);
}

/* Row place k (0-11) of the list: two words a row, six rows, left of the rail. */
static inline CtrPokeTouchRect DevEcRow(int k)
{
    CtrPokeTouchRect r = {(int16_t)(16 + 102 * (k % 2)), (int16_t)(60 + 20 * (k / 2)), 100, 20};

    return r;
}

/* A list's bottom keys: 0 USE WORD, 1 the toggle, 2 BACK (at the right, as every view's and the groups' BACK). */
static inline CtrPokeTouchRect DevEcListKey(int i)
{
    return DevAcross(3, i, 196, 36);
}

/* Letter i (0-25) of the strip, thirteen down in two columns: its place, the lit letter's box; the letter's capitals
 * from 1 px in (x 291 + 8 * column, y 60 + 9 * row). */
static inline CtrPokeTouchRect DevEcStripCell(int i)
{
    CtrPokeTouchRect r = {(int16_t)(290 + 8 * (i / 13)), (int16_t)(59 + 9 * (i % 13)), 8, 9};

    return r;
}

/* ---- FireRed's picture and the idle screen (no board; 3ds_device_frame_impl.h) ----------------------------------------
 * FireRed's picture: an embedded screen centred across the top (kDevViewScreen), its paper (6 px in) the picture's
 * 240x160 at 1:1 where the GPU draws it (PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H, 3ds_poketouch.h); under it the
 * tall note (kDevTallNote) beside BACK (kDevTallBack), or without BACK the note across both (kDevViewNoteWide). The
 * idle screen: an embedded screen centred on the shell (kDevIdleScreen), the project's logo centred on its paper. */
static const CtrPokeTouchRect kDevViewScreen __attribute__((unused)) = {34, 8, 252, 172};
static const CtrPokeTouchRect kDevViewNoteWide __attribute__((unused)) = {8, 188, 304, 44};
static const CtrPokeTouchRect kDevIdleScreen __attribute__((unused)) = {40, 45, 240, 150};

#endif
