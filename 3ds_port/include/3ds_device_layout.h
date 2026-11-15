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

#endif
