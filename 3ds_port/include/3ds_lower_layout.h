#ifndef GUARD_3DS_LOWER_LAYOUT_H
#define GUARD_3DS_LOWER_LAYOUT_H

/* Where things are on the reworked lower screen (320x240, phase 1 of
 * docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md). Drawing and hit tests both read these, so a
 * layout change is a change here. The values are PROVISIONAL placeholders that keep the design's rules
 * (tests/lower_layout_test.c) until the owner's Claude Design handoff replaces them; they are not a design. */
#include "3ds_poketouch.h"

#define LOWER_LAYOUT_PROVISIONAL 1

/* HOME. The map panel has the standard frame; its interior holds the 192x128 map at (x + 8, y + 8) and, under it,
 * the caption strip (the place on the left, the region on the right). */
static const CtrPokeTouchRect kHomeMapPanel = { 0, 0, 208, 160 };
static const CtrPokeTouchRect kHomeCaption = { 8, 136, 192, 16 };
/* PT_POKEDEX .. PT_OPTIONS, in that order. */
static const CtrPokeTouchRect kHomeEntry[6] = {
    { 4, 168, 48, 64 }, { 57, 168, 48, 64 }, { 110, 168, 48, 64 },
    { 163, 168, 48, 64 }, { 216, 168, 48, 64 }, { 269, 168, 48, 64 },
};
static const CtrPokeTouchRect kHomeRegistered = { 216, 8, 48, 40 };
static const CtrPokeTouchRect kHomeBike = { 268, 8, 48, 40 };
static const CtrPokeTouchRect kHomeRun = { 216, 52, 100, 24 };
static const CtrPokeTouchRect kHomeMessage = { 216, 80, 104, 80 };

/* Battle commands: FIGHT, BAG, POKéMON, RUN; kBatActionBall is their place when the quick ball shows (a wild
 * battle), beside kBatBall and its two arrows. The party's six balls are on every command view. */
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

/* Moves (and, in a double battle, the targets): four cells, the move's note with the dialogue frame, CANCEL. */
static const CtrPokeTouchRect kBatMove[4] = {
    { 8, 8, 148, 52 }, { 164, 8, 148, 52 }, { 8, 64, 148, 52 }, { 164, 64, 148, 52 },
};
static const CtrPokeTouchRect kBatMoveNote = { 8, 120, 232, 112 };
static const CtrPokeTouchRect kBatCancel = { 248, 120, 64, 112 };
static const CtrPokeTouchRect kBatTarget[4] = {
    { 8, 8, 148, 52 }, { 164, 8, 148, 52 }, { 8, 64, 148, 52 }, { 164, 64, 148, 52 },
};

#endif
