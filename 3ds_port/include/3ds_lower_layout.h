#ifndef GUARD_3DS_LOWER_LAYOUT_H
#define GUARD_3DS_LOWER_LAYOUT_H

/* Where things are on the reworked lower screen (320x240, phase 1 of
 * docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md). Drawing and hit tests both read these, so a
 * layout change is a change here. The owner's picks on the design canvas: HOME "F" (a START-like menu window, RUN
 * and the game's speed under it, the party at the bottom), commands "2" (framed icon cells), moves "7" (the upstream
 * port's arrangement in FireRed's style). The rules they keep are tests/lower_layout_test.c. */
#include "3ds_poketouch.h"

#define LOWER_LAYOUT_PROVISIONAL 0

/* HOME. The map panel has the standard frame; its interior holds the 192x128 map at (x + 8, y + 8) and, under it,
 * the caption strip (the place on the left, the region on the right). */
static const CtrPokeTouchRect kHomeMapPanel = { 4, 4, 208, 160 };
static const CtrPokeTouchRect kHomeCaption = { 12, 140, 192, 16 };
/* The menu window, as FireRed's START menu: a row per entry, PT_POKEDEX .. PT_OPTIONS in that order. */
static const CtrPokeTouchRect kHomeMenu = { 216, 4, 96, 112 };
static const CtrPokeTouchRect kHomeEntry[6] = {
    { 224, 12, 80, 16 }, { 224, 28, 80, 16 }, { 224, 44, 80, 16 },
    { 224, 60, 80, 16 }, { 224, 76, 80, 16 }, { 224, 92, 80, 16 },
};
/* RUN and the game's speed (X and Y left the page; their buttons still work): a window 4 px under the menu whose bottom
 * is the map panel's, one text line each (the arrow, the label, the value on the right); a touch anywhere on a line's
 * half of the window is that line's. */
static const CtrPokeTouchRect kHomeSettings = { 216, 120, 96, 44 };
static const CtrPokeTouchRect kHomeRunLine = { 224, 128, 80, 14 };
static const CtrPokeTouchRect kHomeSpeedLine = { 224, 142, 80, 14 };
static const CtrPokeTouchRect kHomeRun = { 216, 120, 96, 22 };
static const CtrPokeTouchRect kHomeSpeed = { 216, 142, 96, 22 };
/* The party where the message band was: a framed window 4 px under the map, each slot a 32x32 party icon at
 * (x + 8, y + 7) over FireRed's HP bar at (x + 10, y + 40), at the battle party bar's x; the slot is its touch place.
 * While the map has a place picked, the Fly's hint (bar[2]) has the window instead. */
static const CtrPokeTouchRect kHomeParty = { 4, 168, 312, 68 };
static const CtrPokeTouchRect kHomePartySlot[6] = {
    { 16, 176, 48, 52 }, { 64, 176, 48, 52 }, { 112, 176, 48, 52 },
    { 160, 176, 48, 52 }, { 208, 176, 48, 52 }, { 256, 176, 48, 52 },
};

/* The party on top of every battle page: six slots 48 px apart, each Pokemon's own party icon (32x32) with FireRed's
 * HP bar under it (the owner's "B"). */
static const CtrPokeTouchRect kBatParty = { 16, 0, 288, 38 };
/* Battle commands: FIGHT, BAG, POKéMON, RUN; kBatActionBall is their place when the quick ball shows (a wild
 * battle), beside kBatBall and its two arrows. FIGHT sits 4 px over the row under it. */
static const CtrPokeTouchRect kBatAction[4] = {
    { 8, 44, 304, 88 }, { 8, 136, 96, 96 }, { 112, 136, 96, 96 }, { 216, 136, 96, 96 },
};
static const CtrPokeTouchRect kBatActionBall[4] = {
    { 8, 44, 304, 88 }, { 8, 136, 72, 96 }, { 84, 136, 72, 96 }, { 240, 136, 72, 96 },
};
static const CtrPokeTouchRect kBatBall = { 160, 136, 72, 64 };
static const CtrPokeTouchRect kBatBallArrow[2] = { { 160, 208, 24, 24 }, { 208, 208, 24, 24 } };
/* The Safari Zone: BALL, BAIT, ROCK, RUN. */
static const CtrPokeTouchRect kBatSafari[4] = {
    { 8, 44, 152, 88 }, { 160, 44, 152, 88 }, { 8, 136, 152, 96 }, { 160, 136, 152, 96 },
};

/* Moves (and, in a double battle, the targets): four cards 4 px apart under the party, the note with the
 * dialogue frame (the move's description in three lines, STAB at the first one's right), CANCEL. */
static const CtrPokeTouchRect kBatMove[4] = {
    { 6, 40, 152, 64 }, { 162, 40, 152, 64 }, { 6, 108, 152, 64 }, { 162, 108, 152, 64 },
};
static const CtrPokeTouchRect kBatMoveNote = { 6, 176, 240, 56 };
static const CtrPokeTouchRect kBatCancel = { 250, 176, 64, 56 };
static const CtrPokeTouchRect kBatTarget[4] = {
    { 6, 40, 152, 64 }, { 162, 40, 152, 64 }, { 6, 108, 152, 64 }, { 162, 108, 152, 64 },
};

/* POKeMON, in the field and in battle (phase 2, the owner's "1"): six framed cards in two columns of three, read left
 * to right and down as the Party's D-pad walks them (recipe 0167), FireRed's message under them in the dialogue frame
 * with CANCEL beside it. Picking several Pokemon, CONFIRM sits between a narrower message and CANCEL. A menu (the
 * actions, YES / NO) is a window at the bottom right over CANCEL, one 16 px row each from PARTY_MENU_BOTTOM up, the
 * message narrower beside it. A message that waits for A takes the whole width; a Rare Candy's level up adds its window
 * over the right column's first two cards. */
static const CtrPokeTouchRect kPartySlot[6] = {
    { 4, 4, 154, 57 }, { 162, 4, 154, 57 }, { 4, 65, 154, 57 }, { 162, 65, 154, 57 }, { 4, 126, 154, 57 }, { 162, 126, 154, 57 },
};
static const CtrPokeTouchRect kPartyNote = { 4, 188, 248, 48 };
static const CtrPokeTouchRect kPartyNoteMulti = { 4, 188, 184, 48 };
static const CtrPokeTouchRect kPartyNoteMenu = { 4, 188, 192, 48 };
static const CtrPokeTouchRect kPartyNoteWide = { 4, 188, 312, 48 };
static const CtrPokeTouchRect kPartyConfirm = { 192, 188, 60, 48 };
static const CtrPokeTouchRect kPartyCancel = { 256, 188, 60, 48 };
#define PARTY_MENU_X 200
#define PARTY_MENU_W 116
#define PARTY_MENU_BOTTOM 236
#define PARTY_MENU_ROWS 8
static const CtrPokeTouchRect kPartyLevelUp = { 162, 4, 154, 118 };

/* The Summary (phase 2, the owner's "3", FireRed's own arrangement): the page's title with its dots and the arrows that
 * turn the pages (INFO, STATS, MOVES, TRAINER), the Pokemon on the left (name, Lv, its picture, its types) and the page
 * on the right; under them the previous and next Pokemon, the description band (dialogue frame) and BACK. The moves
 * page's four rows (two lines each) and the details' five (the moves and CANCEL) are inside the page's frame, the
 * details' type, power and accuracy under their rows. */
static const CtrPokeTouchRect kSumTitle = { 4, 4, 248, 30 };
static const CtrPokeTouchRect kSumPagePrev = { 256, 4, 28, 30 };
static const CtrPokeTouchRect kSumPageNext = { 288, 4, 28, 30 };
static const CtrPokeTouchRect kSumMon = { 4, 38, 108, 146 };
static const CtrPokeTouchRect kSumPage = { 116, 38, 200, 146 };
static const CtrPokeTouchRect kSumMonPrev = { 4, 188, 28, 48 };
static const CtrPokeTouchRect kSumMonNext = { 36, 188, 28, 48 };
static const CtrPokeTouchRect kSumNote = { 68, 188, 184, 48 };
static const CtrPokeTouchRect kSumBack = { 256, 188, 60, 48 };

static inline CtrPokeTouchRect SumMoveCard(int i)
{
    CtrPokeTouchRect r = { (int16_t)(kSumPage.x + 8), (int16_t)(kSumPage.y + 8 + 32 * i), (int16_t)(kSumPage.w - 16), 30 };

    return r;
}

static inline CtrPokeTouchRect SumMoveRow(int i)
{
    CtrPokeTouchRect r = { (int16_t)(kSumPage.x + 8), (int16_t)(kSumPage.y + 8 + 16 * i), (int16_t)(kSumPage.w - 16), 16 };

    return r;
}

/* The BAG, the field's and the battle's (phase 2, the owner's "3"), and the lists drawn like it (the TM Case, the Berry
 * Pouch, a mart's BUY, the item PC, the mailbox): the pockets as three tabs across the top (a case, a mart or a PC: its
 * title across instead), the list's rows with each item's icon, name and count (or price, or its button X / Y), the
 * scroll cells beside it, the description band (dialogue frame, three lines) and CANCEL. A menu or YES / NO is the
 * Party's window at the bottom right (PartyMenuWindow), the band narrower beside it; the quantity box is a count window,
 * up and down, OK and CANCEL in that corner; a message that waits for A takes the whole band. */
static const CtrPokeTouchRect kBagTab[3] = { { 4, 4, 101, 30 }, { 109, 4, 102, 30 }, { 215, 4, 101, 30 } };
static const CtrPokeTouchRect kBagTitle = { 4, 4, 312, 30 };
static const CtrPokeTouchRect kBagList = { 4, 38, 284, 138 };
static const CtrPokeTouchRect kBagUp = { 292, 38, 24, 67 };
static const CtrPokeTouchRect kBagDown = { 292, 109, 24, 67 };
static const CtrPokeTouchRect kBagNote = { 4, 180, 248, 56 };
static const CtrPokeTouchRect kBagNoteMenu = { 4, 180, 192, 56 };
static const CtrPokeTouchRect kBagNoteWide = { 4, 180, 312, 56 };
static const CtrPokeTouchRect kBagCancel = { 256, 180, 60, 56 };
static const CtrPokeTouchRect kBagQtyCount = { 200, 92, 116, 40 };
static const CtrPokeTouchRect kBagQtyUp = { 200, 136, 56, 40 };
static const CtrPokeTouchRect kBagQtyDown = { 260, 136, 56, 40 };
static const CtrPokeTouchRect kBagQtyOk = { 200, 180, 56, 56 };
static const CtrPokeTouchRect kBagQtyCancel = { 260, 180, 56, 56 };
#define BAG_PAGE_ROWS 4
#define BAG_ROW_H 30

static inline CtrPokeTouchRect BagRow(int i)
{
    CtrPokeTouchRect r = { (int16_t)(kBagList.x + 8), (int16_t)(kBagList.y + 8 + BAG_ROW_H * i), (int16_t)(kBagList.w - 16), BAG_ROW_H };

    return r;
}

/* The POKeDEX (phase 2, drawn in the BAG's style). The top menu: its three sections as the BAG's tabs, the section's
 * rows in a list window (six a page, the scroll cells beside it), the row's FireRed icon and counts in a window on the
 * right; an ordered list: its order and counts in a title, five rows, the Pokemon's picture and types on the right. A
 * habitat page: its Pokemon in a window, two by two, the page arrows beside the band. A Pokemon's page: its picture,
 * its data, its entry in the dialogue frame, AREA and CRY. Every page: the band (three lines) and BACK. */
static const CtrPokeTouchRect kDexTab[3] = { { 4, 4, 101, 30 }, { 109, 4, 102, 30 }, { 215, 4, 101, 30 } };
static const CtrPokeTouchRect kDexTitle = { 4, 4, 312, 30 };
static const CtrPokeTouchRect kDexList = { 4, 38, 184, 138 };
static const CtrPokeTouchRect kDexUp = { 192, 38, 24, 67 };
static const CtrPokeTouchRect kDexDown = { 192, 109, 24, 67 };
static const CtrPokeTouchRect kDexSide = { 220, 38, 96, 138 };
static const CtrPokeTouchRect kDexNote = { 4, 180, 248, 56 };
static const CtrPokeTouchRect kDexBack = { 256, 180, 60, 56 };
static const CtrPokeTouchRect kDexHabitat = { 4, 4, 312, 172 };
static const CtrPokeTouchRect kDexPrev = { 4, 180, 28, 56 };
static const CtrPokeTouchRect kDexNext = { 36, 180, 28, 56 };
static const CtrPokeTouchRect kDexNoteNarrow = { 68, 180, 184, 56 };
static const CtrPokeTouchRect kDexPic = { 4, 4, 112, 112 };
static const CtrPokeTouchRect kDexInfo = { 120, 4, 196, 112 };
static const CtrPokeTouchRect kDexEntry = { 4, 120, 312, 56 };
static const CtrPokeTouchRect kDexArea = { 4, 180, 122, 56 };
static const CtrPokeTouchRect kDexCry = { 130, 180, 122, 56 };
static const CtrPokeTouchRect kDexNoteWide = { 4, 180, 312, 56 };
#define DEX_MENU_ROWS 6
#define DEX_MENU_ROW_H 20
#define DEX_LIST_ROW_H 24

static inline CtrPokeTouchRect DexMenuRow(int i)
{
    CtrPokeTouchRect r = { (int16_t)(kDexList.x + 8), (int16_t)(kDexList.y + 8 + DEX_MENU_ROW_H * i), (int16_t)(kDexList.w - 16), DEX_MENU_ROW_H };

    return r;
}

static inline CtrPokeTouchRect DexListRow(int i)
{
    CtrPokeTouchRect r = { (int16_t)(kDexList.x + 8), (int16_t)(kDexList.y + 8 + DEX_LIST_ROW_H * i), (int16_t)(kDexList.w - 16), DEX_LIST_ROW_H };

    return r;
}

/* A habitat page's place i (0-3), two by two inside its window. */
static inline CtrPokeTouchRect DexSlot(int i)
{
    CtrPokeTouchRect r = { (int16_t)(kDexHabitat.x + 8 + 148 * (i % 2)), (int16_t)(kDexHabitat.y + 8 + 78 * (i / 2)), 148, 78 };

    return r;
}

/* OPTIONS (phase 2, in FireRed's OPTION window's style): the title and the PAGE cell across the top, the page's rows in
 * a list window (the label, then the value between its two arrows, or a row's levels side by side), the look's preview
 * under a short page's rows, the band (the row's description; FRAME's sample at its right) and BACK. A row's touch
 * places: its label, "<" (the box's first OPT_ARROW_W), and the value or ">" (the rest of the box). */
static const CtrPokeTouchRect kOptTitle = { 4, 4, 216, 30 };
static const CtrPokeTouchRect kOptPage = { 224, 4, 92, 30 };
static const CtrPokeTouchRect kOptList = { 4, 38, 312, 138 };
static const CtrPokeTouchRect kOptNote = { 4, 180, 248, 56 };
static const CtrPokeTouchRect kOptBack = { 256, 180, 60, 56 };
static const CtrPokeTouchRect kOptSample = { 180, 188, 56, 40 };
#define OPT_ROW_H 17
#define OPT_BOX_W 124
#define OPT_ARROW_W 28

static inline CtrPokeTouchRect OptRowRect(int i)
{
    CtrPokeTouchRect r = { (int16_t)(kOptList.x + 8), (int16_t)(kOptList.y + 8 + OPT_ROW_H * i), (int16_t)(kOptList.w - 16), OPT_ROW_H };

    return r;
}

/* SAVE (phase 2): FireRed's save information in a window (the player, the badges, the Pokedex, the play time, the
 * money) and the player's own map icon at 4x in a window beside it; the dialogue in the band with YES / NO in the
 * Party's window, the band whole while writing and for the result (a tap anywhere is its OK). SAVE has no BACK: NO and
 * B leave it. */
static const CtrPokeTouchRect kSaveInfo = { 4, 4, 204, 172 };
static const CtrPokeTouchRect kSavePlayer = { 212, 4, 104, 172 };
static const CtrPokeTouchRect kSaveNote = { 4, 180, 192, 56 };
static const CtrPokeTouchRect kSaveNoteWide = { 4, 180, 312, 56 };

/* The Trainer Card (phase 2): the card as one window (a touch anywhere on it turns it over), its front (the title and
 * stars with the ID, the lines NAME, MONEY, POKeDEX and TIME, the trainer's picture, the eight badges at 2x) or its back
 * (FireRed's records), the band and BACK. */
static const CtrPokeTouchRect kCard = { 4, 4, 312, 172 };
static const CtrPokeTouchRect kCardNote = { 4, 180, 248, 56 };
static const CtrPokeTouchRect kCardBack = { 256, 180, 60, 56 };
#define CARD_BADGE 32
#define CARD_BADGE_GAP 4

/* The PC box (phase 2, FireRed's own arrangement at 1:1): the box's window holds FireRed's wallpaper, its 160x24 header
 * with the box's name between the arrows and its 160x120 body with the thirty places, 24 px each (the icons 32 px over
 * them, as FireRed draws them). PARTY and CLOSE BOX are cells beside it, over a window with the picture of the Pokemon
 * under the cursor; the band (what the box says, or that Pokemon's lines) and BACK under them. The Party's panel takes
 * the box's window (the lead on the left, five rows, CANCEL across); the box's menu, YES / NO and marks are the Party's
 * window at the bottom right; the box chooser is three cells over the wallpaper. */
static const CtrPokeTouchRect kBoxWin = { 4, 4, 176, 172 };
#define BOX_WALL_X 12
#define BOX_WALL_Y 18
static const CtrPokeTouchRect kBoxPrev = { 12, 18, 32, 24 };
static const CtrPokeTouchRect kBoxTitle = { 44, 18, 96, 24 };
static const CtrPokeTouchRect kBoxNext = { 140, 18, 32, 24 };
static const CtrPokeTouchRect kBoxPartyButton = { 184, 4, 132, 30 };
static const CtrPokeTouchRect kBoxClose = { 184, 38, 132, 30 };
static const CtrPokeTouchRect kBoxMon = { 184, 72, 132, 104 };
static const CtrPokeTouchRect kBoxNote = { 4, 180, 248, 56 };
static const CtrPokeTouchRect kBoxNoteMenu = { 4, 180, 192, 56 };
static const CtrPokeTouchRect kBoxNoteWide = { 4, 180, 312, 56 };
static const CtrPokeTouchRect kBoxBack = { 256, 180, 60, 56 };
static const CtrPokeTouchRect kBoxLead = { 12, 12, 72, 128 };
static const CtrPokeTouchRect kBoxPartyCancel = { 12, 144, 160, 24 };
static const CtrPokeTouchRect kBoxChoosePrev = { 12, 64, 32, 56 };
static const CtrPokeTouchRect kBoxChooseName = { 48, 64, 88, 56 };
static const CtrPokeTouchRect kBoxChooseNext = { 140, 64, 32, 56 };
#define BOX_CELL 24

/* The box's place `pos` (0-29), six to a row over the wallpaper's body. */
static inline CtrPokeTouchRect BoxCell(int pos)
{
    CtrPokeTouchRect r = { (int16_t)(BOX_WALL_X + 8 + BOX_CELL * (pos % 6)), (int16_t)(BOX_WALL_Y + 24 + BOX_CELL * (pos / 6)), BOX_CELL, BOX_CELL };

    return r;
}

/* The Party's panel: slots 1-5 as rows right of the lead's place. */
static inline CtrPokeTouchRect BoxPartyRow(int slot)
{
    CtrPokeTouchRect r = { (int16_t)(kBoxLead.x + kBoxLead.w + 4), (int16_t)(kBoxLead.y + 26 * (slot - 1)), 84, 26 };

    return r;
}

/* The party menu's window for `rows` rows (1 to PARTY_MENU_ROWS), and its row i: 8 px in from the frame, 16 px tall. */
static inline CtrPokeTouchRect PartyMenuWindow(int rows)
{
    CtrPokeTouchRect r = { PARTY_MENU_X, (int16_t)(PARTY_MENU_BOTTOM - 16 - 16 * rows), PARTY_MENU_W, (int16_t)(16 + 16 * rows) };

    return r;
}

static inline CtrPokeTouchRect PartyMenuRow(int rows, int i)
{
    CtrPokeTouchRect w = PartyMenuWindow(rows), r = { (int16_t)(w.x + 8), (int16_t)(w.y + 8 + 16 * i), (int16_t)(w.w - 16), 16 };

    return r;
}

#endif
