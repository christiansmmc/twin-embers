/* The Party's action menu (3ds_party_grid.h): one column, which the lower screen scrolls (owner O2), so the D-pad is
 * FireRed's own list input for every count (recipe 0257's grid takes no part: CtrPartyGrid_Move answers -1) and at most
 * eight actions are counted, as the drawing clamps them. The drawn column is checked in device_layout_test.c
 * (DevPtyMenuKey), so the layout and the D-pad cannot drift apart.
 *
 * usage: party_grid_test */
#include <assert.h>
#include <stdio.h>
#include "3ds_party_grid.h"

int main(void)
{
    int n, i, d;

    for (n = 1; n <= 8; ++n) {
        assert(CtrPartyGrid_Cols(n) == 1 && CtrPartyGrid_Count(n) == n);
        for (i = 0; i < n; ++i)
            for (d = 0; d < 4; ++d)
                assert(CtrPartyGrid_Move(n, i, d) == -1);
    }
    assert(CtrPartyGrid_Count(9) == 8 && CtrPartyGrid_Count(-1) == 0);

    printf("PASS party grid\n");
    return 0;
}
