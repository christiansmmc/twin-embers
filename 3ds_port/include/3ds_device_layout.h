#ifndef CTR_DEVICE_LAYOUT_H
#define CTR_DEVICE_LAYOUT_H

#include "3ds_poketouch.h"

/*
 * The red device (style B): the rectangles of the lower screen's B views, in screen pixels, taken from the owner's
 * boards. One source for what a view draws, what its touch hits and what device_layout_test checks (on the screen with
 * the drop, keys at least 28x20, the keys of one state apart). A key's rectangle is its resting face, as the boards
 * give it; its drop shadow takes DEV_DROP rows under it, and a chosen key is drawn DEV_SINK rows lower inside that
 * same area, so DevKeyArea is both where a key paints and where it is hit.
 *
 * Every rectangle here is `static const CtrPokeTouchRect kDev... __attribute__((unused))` or a static inline function
 * (a list's n-th row, a grid's cell).
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

#endif
