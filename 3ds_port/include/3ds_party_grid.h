#ifndef CTR_PARTY_GRID_H
#define CTR_PARTY_GRID_H

/* The Party's action menu (SUMMARY, SWITCH, ITEM, field moves, CANCEL; HEAL and REMEMBER with recipe 0264's
 * CTR_PARTY_EXTRAS) is ONE column, which the lower screen scrolls (the owner's O2: 3ds_device_layout.h DevPtyMenuKey,
 * its window following FireRed's cursor), so the D-pad is FireRed's own list input (UP / DOWN; it wraps beyond three
 * actions) for every count. The game's input (recipe 0257, Task_HandleSelectionMenuInput in party_menu.c) still asks
 * CtrPartyGrid_Move, which leaves it to that list input; 3ds_port/tests/party_grid_test.c and device_layout_test.c hold
 * the drawing and the D-pad to one column.
 *
 * Header-only and free of tables and switches: the game's objects include it (clean-release audit). */

#define CTR_PGRID_MAX 8
enum { CTR_PGRID_UP = 0, CTR_PGRID_DOWN = 1, CTR_PGRID_LEFT = 2, CTR_PGRID_RIGHT = 3 };

/* The actions counted: at most eight (FireRed's list holds eight). */
static inline int CtrPartyGrid_Count(int count)
{
    return count < 0 ? 0 : count > CTR_PGRID_MAX ? CTR_PGRID_MAX : count;
}

/* The menu's columns: one, whatever the count (O2). */
static inline int CtrPartyGrid_Cols(int count)
{
    (void)count;
    return 1;
}

/* The action the cursor goes to from `cur` on a D-pad direction: -1 for every count, cursor and direction (one column:
 * the game's own list input moves it). */
static inline int CtrPartyGrid_Move(int count, int cur, int dir)
{
    (void)count;
    (void)cur;
    (void)dir;
    return -1;
}

#endif
