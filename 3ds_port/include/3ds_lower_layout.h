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
 * dialogue frame (ACC and STAB, then two lines of the move's description), CANCEL. */
static const CtrPokeTouchRect kBatMove[4] = {
    { 6, 40, 152, 64 }, { 162, 40, 152, 64 }, { 6, 108, 152, 64 }, { 162, 108, 152, 64 },
};
static const CtrPokeTouchRect kBatMoveNote = { 6, 176, 240, 56 };
static const CtrPokeTouchRect kBatCancel = { 250, 176, 64, 56 };
static const CtrPokeTouchRect kBatTarget[4] = {
    { 6, 40, 152, 64 }, { 162, 40, 152, 64 }, { 6, 108, 152, 64 }, { 162, 108, 152, 64 },
};

#endif
