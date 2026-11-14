#ifndef GUARD_3DS_LOWER_LAYOUT_H
#define GUARD_3DS_LOWER_LAYOUT_H

/* Where things are on the reworked lower screen (320x240, phase 1 of
 * docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md). Drawing and hit tests both read these, so a
 * layout change is a change here. The owner's picks on the design canvas: HOME "A" (a START-like menu window),
 * commands "2" (framed icon cells), moves "7" (the upstream port's arrangement in FireRed's style). The rules they
 * keep are tests/lower_layout_test.c. */
#include "3ds_poketouch.h"

#define LOWER_LAYOUT_PROVISIONAL 0

/* HOME. The map panel has the standard frame; its interior holds the 192x128 map at (x + 8, y + 8) and, under it,
 * the caption strip (the place on the left, the region on the right). */
static const CtrPokeTouchRect kHomeMapPanel = { 4, 4, 208, 160 };
static const CtrPokeTouchRect kHomeCaption = { 12, 140, 192, 16 };
/* The menu window, as FireRed's START menu: a row per entry, PT_POKEDEX .. PT_OPTIONS in that order, then RUN. */
static const CtrPokeTouchRect kHomeMenu = { 216, 4, 96, 128 };
static const CtrPokeTouchRect kHomeEntry[6] = {
    { 224, 12, 80, 16 }, { 224, 28, 80, 16 }, { 224, 44, 80, 16 },
    { 224, 60, 80, 16 }, { 224, 76, 80, 16 }, { 224, 92, 80, 16 },
};
static const CtrPokeTouchRect kHomeRun = { 224, 108, 80, 16 };
/* X and Y: framed cells with the item's icon, the key's letter on the frame's top edge. */
static const CtrPokeTouchRect kHomeRegistered = { 216, 136, 40, 40 };
static const CtrPokeTouchRect kHomeBike = { 264, 136, 40, 40 };
/* The message band at the bottom, in the dialogue frame. */
static const CtrPokeTouchRect kHomeMessage = { 4, 180, 312, 56 };

/* Battle commands: FIGHT, BAG, POKéMON, RUN; kBatActionBall is their place when the quick ball shows (a wild
 * battle), beside kBatBall and its two arrows. The party's six balls are on every battle page. */
static const CtrPokeTouchRect kBatParty = { 64, 4, 192, 24 };
static const CtrPokeTouchRect kBatAction[4] = {
    { 8, 32, 304, 96 }, { 8, 136, 96, 96 }, { 112, 136, 96, 96 }, { 216, 136, 96, 96 },
};
static const CtrPokeTouchRect kBatActionBall[4] = {
    { 8, 32, 304, 96 }, { 8, 136, 72, 96 }, { 84, 136, 72, 96 }, { 240, 136, 72, 96 },
};
static const CtrPokeTouchRect kBatBall = { 160, 136, 72, 64 };
static const CtrPokeTouchRect kBatBallArrow[2] = { { 160, 208, 24, 24 }, { 208, 208, 24, 24 } };
/* The Safari Zone: BALL, BAIT, ROCK, RUN. */
static const CtrPokeTouchRect kBatSafari[4] = {
    { 8, 32, 152, 96 }, { 160, 32, 152, 96 }, { 8, 136, 152, 96 }, { 160, 136, 152, 96 },
};

/* Moves (and, in a double battle, the targets): four cards 4 px apart under the party's balls, the note with the
 * dialogue frame (ACC and STAB, then two lines of the move's description), CANCEL. */
static const CtrPokeTouchRect kBatMove[4] = {
    { 6, 32, 152, 64 }, { 162, 32, 152, 64 }, { 6, 100, 152, 64 }, { 162, 100, 152, 64 },
};
static const CtrPokeTouchRect kBatMoveNote = { 6, 176, 240, 56 };
static const CtrPokeTouchRect kBatCancel = { 250, 176, 64, 56 };
static const CtrPokeTouchRect kBatTarget[4] = {
    { 6, 32, 152, 64 }, { 162, 32, 152, 64 }, { 6, 100, 152, 64 }, { 162, 100, 152, 64 },
};

#endif
