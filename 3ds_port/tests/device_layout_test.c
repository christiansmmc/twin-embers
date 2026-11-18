/* The red device's rectangles (3ds_device_layout.h), the one source of what the B views draw and what they hit.
 *
 * Rules: every rectangle the header holds lies inside the 320x240 screen with its 3 px drop; a key is at least 20 px
 * tall and 28 px wide; the keys of one view state never overlap, their drops included. Keys of different states may
 * share space (the Bag's item menu keys lie where its CLOSE is in the list), so each state is a group of its own. Each
 * view adds its rectangles to `rects` and one group per state to `groups` (in main: the header's rectangles are
 * objects, not constants), and its own checks (where a view's pieces must lie).
 *
 * usage: device_layout_test (prints PASS device layout) */
#include <assert.h>
#include <stdio.h>
#include "3ds_device_layout.h"

typedef struct { const char *name; CtrPokeTouchRect r; } DevNamedRect;
typedef struct { const char *state; const CtrPokeTouchRect *keys; int count; } DevGroup;
#define DEV_GROUP(state, keys) {state, keys, (int)(sizeof(keys) / sizeof(keys[0]))}

static int Inside(CtrPokeTouchRect r)
{
    CtrPokeTouchRect a = DevKeyArea(&r);

    return a.x >= 0 && a.y >= 0 && a.w > 0 && a.h > 0 && a.x + a.w <= 320 && a.y + a.h <= 240;
}

static int Same(CtrPokeTouchRect a, CtrPokeTouchRect b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static int BigEnough(CtrPokeTouchRect r)
{
    return r.h >= 20 && r.w >= 28;
}

/* r (with its drop when `drop`) lies inside `outer`. */
static int Within(CtrPokeTouchRect r, CtrPokeTouchRect outer, int drop)
{
    CtrPokeTouchRect a = drop ? DevKeyArea(&r) : r;

    return a.x >= outer.x && a.y >= outer.y && a.x + a.w <= outer.x + outer.w && a.y + a.h <= outer.y + outer.h;
}

/* The paper of an embedded screen in `outer`: 6 px in (DevScreen's). */
static CtrPokeTouchRect PaperOf(CtrPokeTouchRect outer)
{
    CtrPokeTouchRect p = {(int16_t)(outer.x + 6), (int16_t)(outer.y + 6), (int16_t)(outer.w - 12),
                          (int16_t)(outer.h - 12)};

    return p;
}

/* Two keys with their drops share a pixel. */
static int Overlap(CtrPokeTouchRect a, CtrPokeTouchRect b)
{
    CtrPokeTouchRect p = DevKeyArea(&a), q = DevKeyArea(&b);

    return p.x < q.x + q.w && q.x < p.x + p.w && p.y < q.y + q.h && q.y < p.y + p.h;
}

/* The rules broken by one group, each reported; 0 when it keeps them all. */
static int GroupFaults(const DevGroup *g)
{
    int faults = 0;

    for (int i = 0; i < g->count; ++i) {
        CtrPokeTouchRect r = g->keys[i];

        if (!Inside(r)) {
            fprintf(stderr, "%s: key %d {%d,%d,%d,%d} leaves the screen with its drop\n", g->state, i, r.x, r.y, r.w, r.h);
            ++faults;
        }
        if (!BigEnough(r)) {
            fprintf(stderr, "%s: key %d {%d,%d,%d,%d} is under 28x20\n", g->state, i, r.x, r.y, r.w, r.h);
            ++faults;
        }
        for (int j = 0; j < i; ++j)
            if (Overlap(r, g->keys[j])) {
                fprintf(stderr, "%s: keys %d and %d overlap (drops included)\n", g->state, j, i);
                ++faults;
            }
    }
    return faults;
}

/* The rules themselves catch what they are for: a fit group passes, each kind of fault is found. */
static void RulesCatchFaults(void)
{
    static const CtrPokeTouchRect fit[] = {{8, 156, 96, 36}, {112, 156, 96, 36}, {8, 198, 96, 34}};
    static const CtrPokeTouchRect dropOver[] = {{8, 156, 96, 36}, {8, 194, 96, 30}};   /* the first's drop reaches 194 */
    static const CtrPokeTouchRect dropTouch[] = {{8, 156, 96, 36}, {8, 195, 96, 30}};  /* the drop ends at 194 */
    static const CtrPokeTouchRect low[] = {{8, 210, 96, 28}};                          /* 210 + 28 + 3 > 240 */
    static const CtrPokeTouchRect small[] = {{8, 8, 27, 24}, {48, 8, 40, 19}};
    static const DevGroup fitG = DEV_GROUP("fit", fit), dropOverG = DEV_GROUP("drop over", dropOver);
    static const DevGroup dropTouchG = DEV_GROUP("drop touch", dropTouch), lowG = DEV_GROUP("low", low);
    static const DevGroup smallG = DEV_GROUP("small", small);
    CtrPokeTouchRect r = {8, 156, 96, 36}, a = DevKeyArea(&r);

    assert(DEV_DROP == 3 && DEV_SINK == 2);
    assert(a.x == 8 && a.y == 156 && a.w == 96 && a.h == 39);
    fprintf(stderr, "(the faults below are expected: the rules' own check)\n");
    assert(GroupFaults(&fitG) == 0);
    assert(GroupFaults(&dropOverG) == 1);
    assert(GroupFaults(&dropTouchG) == 0);
    assert(GroupFaults(&lowG) == 1);
    assert(GroupFaults(&smallG) == 2);
}

/* HOME (board B · Início): eight keys of 73 under the screen in two rows (the six START entries, RUN and SPD at the
 * rows' ends), the panel's pieces on its paper, the map's keys in the window. */
static int HomeFaults(void)
{
    const CtrPokeTouchRect lower = {0, 150, 320, 90}, window = {PT_HOME_X, PT_HOME_Y, PT_HOME_W, PT_HOME_H};
    const CtrPokeTouchRect paper = {14, 14, 292, 128};
    int faults = 0;

    assert(Same(kDevHomeScreen, (CtrPokeTouchRect){8, 8, 304, 140}));
    for (int i = 0; i < 6; ++i)
        assert(Same(kDevHomeKey[i], DevAcross(4, i % 3, i < 3 ? 154 : 196, 34)));
    assert(Same(kDevHomeKey[0], (CtrPokeTouchRect){8, 154, 73, 34}) && Same(kDevHomeKey[1], (CtrPokeTouchRect){85, 154, 73, 34})
           && Same(kDevHomeKey[2], (CtrPokeTouchRect){162, 154, 73, 34}) && Same(kDevHomeKey[3], (CtrPokeTouchRect){8, 196, 73, 34})
           && Same(kDevHomeKey[4], (CtrPokeTouchRect){85, 196, 73, 34}) && Same(kDevHomeKey[5], (CtrPokeTouchRect){162, 196, 73, 34}));
    assert(Same(kDevHomeRun, DevAcross(4, 3, 154, 34)) && Same(kDevHomeSpeed, DevAcross(4, 3, 196, 34)));
    assert(Same(kDevHomeRun, (CtrPokeTouchRect){239, 154, 73, 34}) && Same(kDevHomeSpeed, (CtrPokeTouchRect){239, 196, 73, 34}));
    for (int i = 0; i < 6; ++i)
        assert(Same(DevHomeCell(i), (CtrPokeTouchRect){(int16_t)(210 + 30 * (i % 3)), (int16_t)(60 + 32 * (i / 3)), 30, 32}));
    assert(Same(kDevHomeName, (CtrPokeTouchRect){212, 16, 90, 26}) && kDevHomeName.h == 2 * 13);
    assert(kDevHomeRegion.x == 212 && kDevHomeRegion.y == 44 && Same(kDevHomeRule, (CtrPokeTouchRect){210, 57, 92, 1}));
    assert(Same(kDevHomeTime, (CtrPokeTouchRect){212, 128, 90, 10}));
    assert(kDevHomeMapKey.x == 16 && kDevHomeMapKey.y == 16 && kDevHomeMapKey.h == 14
           && Same(kDevHomeFly, (CtrPokeTouchRect){158, 16, 44, 16}));        /* moved off Cinnabar Island */
    assert(PT_HOME_X == 14 && PT_HOME_Y == 14 && PT_HOME_W == 192 && PT_HOME_H == 128 && PT_HOME_SRC_X == 24 && PT_HOME_SRC_Y == 28);
    for (int i = 0; i < 8; ++i) {
        CtrPokeTouchRect k = i < 6 ? kDevHomeKey[i] : i == 6 ? kDevHomeRun : kDevHomeSpeed;

        if (!Within(k, lower, 1)) {
            fprintf(stderr, "HOME: key %d leaves the lower part {0,150,320,90}\n", i);
            ++faults;
        }
    }
    if (!Within(kDevHomeMapKey, window, 1) || !Within(kDevHomeFly, window, 1)) {   /* with their drops */
        fprintf(stderr, "HOME: the region key or FLY leaves the map's window\n");
        ++faults;
    }
    if (!Within(window, paper, 0) || !Within(kDevHomeName, paper, 0) || !Within(kDevHomeRegion, paper, 0)
        || !Within(kDevHomeRule, paper, 0) || !Within(kDevHomeTime, paper, 0)) {
        fprintf(stderr, "HOME: a piece of the panel leaves the screen's paper\n");
        ++faults;
    }
    for (int i = 0; i < 6; ++i)
        if (!Within(DevHomeCell(i), paper, 0) || DevHomeCell(i).x < window.x + window.w
            || DevHomeCell(i).y < kDevHomeRule.y + kDevHomeRule.h) {
            fprintf(stderr, "HOME: the party's cell %d leaves the panel\n", i);
            ++faults;
        }
    if (DevHomeCell(5).y + DevHomeCell(5).h > kDevHomeTime.y) {
        fprintf(stderr, "HOME: the party's last row reaches into TIME\n");
        ++faults;
    }
    return faults;
}

/* The battle (boards def_fight and B · Golpes): the brief's numbers (the commands def_fight's, owner O1: a trainer's three
 * keys of 96, a wild battle's four of 73, at y 168 and 62 tall); each screen with its light row above the keys under it;
 * the quick ball's arrows on its key's face. */
static int BattleFaults(void)
{
    int faults = 0;

    assert(Same(kDevBatScreen, (CtrPokeTouchRect){8, 8, 304, 92}) && Same(kDevBatMoveScreen, (CtrPokeTouchRect){8, 8, 304, 56}));
    assert(Same(kDevBatFight, (CtrPokeTouchRect){8, 108, 304, 48}) && Same(kDevBatCancel, (CtrPokeTouchRect){236, 212, 76, 20}));
    for (int i = 0; i < 3; ++i)
        assert(Same(kDevBatRow3[i], (CtrPokeTouchRect){(int16_t)(8 + 104 * i), 168, 96, 62}));
    for (int i = 0; i < 4; ++i) {
        assert(Same(kDevBatRow4[i], (CtrPokeTouchRect){(int16_t)(8 + 77 * i), 168, 73, 62}));
        assert(Same(kDevBatMove[i], (CtrPokeTouchRect){(int16_t)(i & 1 ? 164 : 8), (int16_t)(i < 2 ? 72 : 142), 148, 62}));
    }
    for (int next = 0; next < 2; ++next) {
        const CtrPokeTouchRect *k = &kDevBatRow4[1];
        CtrPokeTouchRect a = DevBatBallArrow(k, next);

        assert(Same(a, (CtrPokeTouchRect){(int16_t)(next ? k->x + k->w - 22 : k->x + 2), (int16_t)(k->y + 38), 20, 20}));
        if (!Within(a, *k, 0)) {
            fprintf(stderr, "battle: the quick ball's %s arrow leaves its key\n", next ? "next" : "previous");
            ++faults;
        }
    }
    if (kDevBatScreen.y + kDevBatScreen.h + 1 > kDevBatFight.y || kDevBatMoveScreen.y + kDevBatMoveScreen.h + 1 > kDevBatMove[0].y) {
        fprintf(stderr, "battle: a screen (with its light row) reaches its keys\n");
        ++faults;
    }
    return faults;
}

/* The Party's action menu (def_party_menu; owner O2): one column at x 204 of DEV_PTY_MENU_ROWS keys, the board's 24 tall
 * and 28 apart, beside the slim bays and clear of them, the sixth's drop ending by y 175; the cue (▲ over the first key,
 * ▼ under the sixth: Call 10) clear of the keys, the screws and the grid's note and exit. The column is one for every
 * count, so the D-pad is FireRed's own list input (3ds_party_grid.h, recipe 0257: its grid answers -1). */
static void PartyMenuGrid(void)
{
    const CtrPokeTouchRect up = {DEV_PTY_CUE_X - 4, DEV_PTY_CUE_UP_Y - 2, 9, 5};
    const CtrPokeTouchRect down = {DEV_PTY_CUE_X - 4, DEV_PTY_CUE_DOWN_Y - 2, 9, 5};
    const CtrPokeTouchRect last = DevPtyMenuKey(DEV_PTY_MENU_ROWS - 1);

    assert(DEV_PTY_MENU_ROWS == 6 && Same(DevPtyMenuKey(5), (CtrPokeTouchRect){204, 148, 108, 24}));
    for (int k = 0; k < DEV_PTY_MENU_ROWS; ++k) {
        CtrPokeTouchRect r = DevPtyMenuKey(k);

        assert(Same(r, (CtrPokeTouchRect){204, (int16_t)(8 + 28 * k), 108, 24}) && Inside(r));
        for (int i = 0; i < 6; ++i)
            assert(!Overlap(r, DevPtySlim(i)));
        for (int j = 0; j < k; ++j)
            assert(!Overlap(r, DevPtyMenuKey(j)));
    }
    assert(last.y + last.h + DEV_DROP <= 175);
    assert(up.x == 254 && up.y == 2 && down.y == 180);                  /* Call 10: centred (258, 4) and (258, 182) */
    assert(up.y + up.h <= DevPtyMenuKey(0).y && down.y >= last.y + last.h + DEV_DROP);
    assert(down.y + down.h <= kDevGridNote.y && down.y + down.h <= kDevGridExit.y);
    assert(up.x > 2 + 5 && up.x + up.w < 313 && up.x >= DevPtyMenuKey(0).x && up.x + up.w <= DevPtyMenuKey(0).x + DevPtyMenuKey(0).w);
    for (int n = 1; n <= 8; ++n) {
        assert(CtrPartyGrid_Cols(n) == 1);
        for (int i = 0; i < n; ++i)
            for (int d = CTR_PGRID_UP; d <= CTR_PGRID_RIGHT; ++d)
                assert(CtrPartyGrid_Move(n, i, d) == -1);                /* one column: the game's own list input */
    }
}

/* The Party (boards def_party and def_party_menu): the brief's numbers; the bays two to a row over the grid's note and
 * exit (picking several: the small note, CONFIRM and the exit, 8 px apart); with a menu the slim bays (the board's, 188
 * wide) none reaching the grid's note (the keys beside them: PartyMenuGrid); each screen above nothing it would cover. */
static int PartyFaults(void)
{
    int faults = 0;

    assert(Same(kDevPtyConfirm, (CtrPokeTouchRect){168, 190, 68, 40}) && Same(kDevPtyLevelScreen, (CtrPokeTouchRect){8, 8, 304, 176}));
    assert(kDevSmallNote.x + kDevSmallNote.w + 8 == kDevPtyConfirm.x && kDevPtyConfirm.x + kDevPtyConfirm.w + 8 == kDevGridExit.x);
    for (int i = 0; i < 6; ++i) {
        int foot = DevPtyBay(i).y + DevPtyBay(i).h + DEV_DROP;

        assert(Same(DevPtyBay(i), (CtrPokeTouchRect){(int16_t)(i & 1 ? 164 : 8), (int16_t)(8 + 56 * (i >> 1)), 148, 50}));
        if (foot > kDevGridNote.y || foot > kDevSmallNote.y || foot > kDevPtyConfirm.y || foot > kDevGridExit.y) {
            fprintf(stderr, "Party: bay %d reaches the note, CONFIRM or CANCEL\n", i);
            ++faults;
        }
    }
    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect s = DevPtySlim(i);

        assert(Same(s, (CtrPokeTouchRect){8, (int16_t)(8 + 28 * i), 188, 24}));
        if (!Inside(s) || s.y + s.h + DEV_DROP > kDevGridNote.y) {
            fprintf(stderr, "Party: slim bay %d reaches the grid's note\n", i);
            ++faults;
        }
    }
    if (kDevPtyLevelScreen.y + kDevPtyLevelScreen.h + 1 > kDevGridNote.y) {
        fprintf(stderr, "Party: the level-up screen (with its light row) reaches the note\n");
        ++faults;
    }
    return faults;
}

/* The Summary (boards def_sum_info, def_sum_stats, def_sum_moves): the brief's numbers; on the grid's screen's paper the
 * left column with the picture's box (the 80x80 picture centred on it fits the column too), and from 4 px right of the
 * column the switch (ending at the page's right, x 256, inside the larger rectangle that takes it, which stays on the
 * paper) and the five rows, the grid's 24 tall and 26 apart. (The tabs, ▲▼, the screen, the note and BACK are the
 * grid's: GridFaults.) */
static int SummaryFaults(void)
{
    const CtrPokeTouchRect paper = PaperOf(kDevGridScreen), big = {22, 72, 80, 80};
    const int page = kDevSumColumn.x + kDevSumColumn.w + 4;
    int faults = 0;

    assert(Same(kDevSumColumn, (CtrPokeTouchRect){14, 46, 96, 132}) && Same(kDevSumPic, (CtrPokeTouchRect){30, 80, 64, 64}));
    assert(Same(kDevSumIvSwitch, (CtrPokeTouchRect){186, 49, 70, 13}) && Same(kDevSumIvSwitchHit, (CtrPokeTouchRect){182, 46, 78, 20}));
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevSumRow(i);

        assert(Same(r, (CtrPokeTouchRect){114, (int16_t)(48 + 26 * i), 146, 24}));
        if (!Within(r, paper, 0) || r.x < page || (i > 0 && r.y < DevSumRow(i - 1).y + DevSumRow(i - 1).h + 2)) {
            fprintf(stderr, "Summary: row %d leaves the paper right of the column, or meets the row above\n", i);
            ++faults;
        }
    }
    if (!Within(kDevSumIvSwitch, kDevSumIvSwitchHit, 0) || !Within(kDevSumIvSwitchHit, paper, 0) || kDevSumIvSwitch.x < page
        || kDevSumIvSwitch.x + kDevSumIvSwitch.w != 256) {
        fprintf(stderr, "Summary: the switch leaves its touch area, the touch area the paper, or the page\n");
        ++faults;
    }
    if (!Within(kDevSumColumn, paper, 0) || kDevSumColumn.x != paper.x || !Within(kDevSumPic, kDevSumColumn, 0)
        || !Within(big, kDevSumColumn, 0)) {
        fprintf(stderr, "Summary: the column leaves the paper's left, or a picture the column\n");
        ++faults;
    }
    return faults;
}

/* The Bag (boards B · BAG and B · BAG: menu do item): the brief's numbers; the pockets (the grid's three tabs) and a
 * case's key above the grid's screen, its five rows (the grid's) on the screen's paper and on the narrow screen's, one
 * under the other, the description's icon on the grid's note; the item menu's, YES's and how many's keys in the grid's
 * column (x 204, right of the narrow screen), their drops ending by the screen's foot and its drop (187); FireRed's last
 * row, NO and how many's CANCEL in the grid's exit. (The screen, ▲▼, the note and the exit are the grid's: GridFaults.) */
static int BagFaults(void)
{
    const CtrPokeTouchRect paper = PaperOf(kDevGridScreen), narrowPaper = PaperOf(kDevGridScreenNarrow);
    const CtrPokeTouchRect qty[3] = {kDevBagQtyUp, kDevBagQtyDown, kDevBagQtyOk};
    const int foot = kDevGridScreen.y + kDevGridScreen.h + DEV_DROP;
    int faults = 0;

    for (int i = 0; i < 3; ++i)
        assert(Same(DevGridTab(3, i), (CtrPokeTouchRect){(int16_t)(8 + 102 * i), 8, 98, 24}));
    assert(Same(kDevBagCaseKey, (CtrPokeTouchRect){8, 8, 304, 24}) && foot == 187);
    assert(Same(kDevBagDescIcon, (CtrPokeTouchRect){18, 199, 24, 24}) && PT_BAG_ROWS == DEV_GRID_ROWS && BUI_BAG_ROWS == DEV_GRID_ROWS);
    assert(Same(kDevBagQtyUp, (CtrPokeTouchRect){204, 40, 108, 30}) && Same(kDevBagQtyScreen, (CtrPokeTouchRect){204, 76, 108, 36}));
    assert(Same(kDevBagQtyDown, (CtrPokeTouchRect){204, 118, 108, 30}) && Same(kDevBagQtyOk, (CtrPokeTouchRect){204, 154, 108, 30}));
    assert(Same(DevBagMenuKey(5, 0), DevGridColumnKey(40, 36, 30, 0)) && Same(DevBagMenuKey(6, 4), (CtrPokeTouchRect){204, 152, 108, 24}));
    for (int i = 0; i < PT_BAG_ROWS; ++i) {
        CtrPokeTouchRect r = DevGridRow(i, 0), n = DevGridRow(i, 1);

        assert(Same(r, (CtrPokeTouchRect){16, (int16_t)(48 + 26 * i), 244, 24}) && Same(n, (CtrPokeTouchRect){16, r.y, 172, 24}));
        if (!Within(r, paper, 0) || !Within(n, narrowPaper, 0) || (i > 0 && r.y < DevGridRow(i - 1, 0).y + 24 + 2)) {
            fprintf(stderr, "Bag: row %d leaves its paper, or meets the row above\n", i);
            ++faults;
        }
    }
    for (int n = 2; n <= PT_BAG_MENU_MAX; ++n)  /* a menu of n rows: n - 1 keys in the column, the last row the exit */
        for (int i = 0; i < n - 1; ++i) {
            CtrPokeTouchRect k = DevBagMenuKey(n, i);

            assert(Same(k, n - 1 <= 4 ? DevGridColumnKey(40, 36, 30, i) : DevGridColumnKey(40, 28, 24, i)));
            if (k.x < kDevGridScreenNarrow.x + kDevGridScreenNarrow.w + 8 || k.y + k.h + DEV_DROP > foot) {
                fprintf(stderr, "Bag: menu key %d of %d reaches the narrow screen, or its drop passes y %d\n", i, n, foot);
                ++faults;
            }
        }
    for (int i = 0; i < 3; ++i)
        if (qty[i].x != DevGridColumnKey(40, 36, 30, 0).x || qty[i].w != DevGridColumnKey(40, 36, 30, 0).w
            || qty[i].y + qty[i].h + DEV_DROP > foot) {
            fprintf(stderr, "Bag: how many's key %d leaves the grid's column, or its drop passes y %d\n", i, foot);
            ++faults;
        }
    for (int i = 0; i < 3; ++i)
        if (DevGridTab(3, i).y + DevGridTab(3, i).h + DEV_DROP > kDevGridScreen.y) {
            fprintf(stderr, "Bag: pocket key %d reaches the list's screen\n", i);
            ++faults;
        }
    if (kDevBagCaseKey.y + kDevBagCaseKey.h + DEV_DROP > kDevGridScreen.y
        || !Within(kDevBagDescIcon, PaperOf(kDevGridNote), 0)
        || kDevBagQtyUp.y + kDevBagQtyUp.h + DEV_DROP > kDevBagQtyScreen.y || kDevBagQtyScreen.y + kDevBagQtyScreen.h + 1 > kDevBagQtyDown.y) {
        fprintf(stderr, "Bag: the case's key, the description's icon, or how many's column out of place\n");
        ++faults;
    }
    return faults;
}

/* The grid (def_spec): TOPO y 8-32 (the tabs, DevGridTab, or the title, kDevGridTitle), MEIO y 40-184 (the screen with
 * ▲▼ beside it, the wide screen, or a narrow screen with the key column at x 204: DevGridColumnKey), RODAPÉ y 190-232
 * (the note with the exit beside it, or the wide note); a list's five rows, 24 tall and 26 apart, on its screen's
 * paper. The tall note and BACK (the frame's, the Trainer Card's, the Pokedex page's and the move to forget's until
 * their tasks) keep their numbers. */
static int GridFaults(void)
{
    const CtrPokeTouchRect paper = PaperOf(kDevGridScreen), narrowPaper = PaperOf(kDevGridScreenNarrow);
    int faults = 0;

    assert(DEV_GRID_ROW_H == 24 && DEV_GRID_ROW_PITCH == 26 && DEV_GRID_ROWS == 5);
    assert(Same(kDevGridTitle, (CtrPokeTouchRect){8, 6, 304, 28}) && Same(kDevGridScreen, (CtrPokeTouchRect){8, 40, 260, 144}));
    assert(Same(kDevGridScreenNarrow, (CtrPokeTouchRect){8, 40, 188, 144}) && Same(kDevGridWide, (CtrPokeTouchRect){8, 40, 304, 144}));
    assert(Same(kDevGridUp, (CtrPokeTouchRect){276, 40, 36, 66}) && Same(kDevGridDown, (CtrPokeTouchRect){276, 114, 36, 66}));
    assert(Same(kDevGridNote, (CtrPokeTouchRect){8, 190, 228, 42}) && Same(kDevGridNoteWide, (CtrPokeTouchRect){8, 190, 304, 42}));
    assert(Same(kDevGridExit, (CtrPokeTouchRect){244, 190, 68, 40}));
    assert(Same(kDevTallNote, (CtrPokeTouchRect){8, 188, 228, 44}) && Same(kDevTallBack, (CtrPokeTouchRect){244, 188, 68, 42}));
    for (int i = 0; i < 4; ++i)
        assert(Same(DevGridTab(4, i), DevAcross(4, i, 8, 24)));
    assert(Same(DevGridTab(3, 0), (CtrPokeTouchRect){8, 8, 98, 24}) && Same(DevGridTab(3, 2), (CtrPokeTouchRect){212, 8, 98, 24}));
    assert(Same(DevGridRow(4, 0), (CtrPokeTouchRect){16, 152, 244, 24}));
    assert(Same(DevGridColumnKey(40, 36, 30, 3), (CtrPokeTouchRect){204, 148, 108, 30}));
    assert(kDevGridExit.x == kDevGridNote.x + kDevGridNote.w + 8 && kDevGridUp.x == kDevGridScreen.x + kDevGridScreen.w + 8);
    for (int i = 0; i < DEV_GRID_ROWS; ++i) {
        CtrPokeTouchRect r = DevGridRow(i, 0), n = DevGridRow(i, 1);

        assert(Same(r, (CtrPokeTouchRect){16, (int16_t)(48 + DEV_GRID_ROW_PITCH * i), 244, DEV_GRID_ROW_H})
               && Same(n, (CtrPokeTouchRect){16, r.y, 172, DEV_GRID_ROW_H}));
        if (!Within(r, paper, 0) || !Within(n, narrowPaper, 0)) {
            fprintf(stderr, "grid: row %d leaves its screen's paper\n", i);
            ++faults;
        }
    }
    for (int n = 3; n <= 4; ++n) {
        CtrPokeTouchRect last = DevGridTab(n, n - 1);

        for (int i = 1; i < n; ++i)
            assert(!Overlap(DevGridTab(n, i - 1), DevGridTab(n, i)));
        if (last.x + last.w > 312 || last.y + last.h + DEV_DROP > kDevGridScreen.y) {
            fprintf(stderr, "grid: %d tabs end past 312 or reach the screen\n", n);
            ++faults;
        }
    }
    if (kDevGridTitle.y + kDevGridTitle.h + 1 > kDevGridScreen.y || kDevGridScreen.y + kDevGridScreen.h + 1 > kDevGridNote.y
        || kDevGridUp.y + kDevGridUp.h + DEV_DROP > kDevGridDown.y || kDevGridDown.y + kDevGridDown.h + DEV_DROP > kDevGridExit.y
        || kDevGridScreenNarrow.x + kDevGridScreenNarrow.w + 8 > DevGridColumnKey(40, 36, 30, 0).x
        || DevGridColumnKey(40, 36, 30, 0).x + DevGridColumnKey(40, 36, 30, 0).w != 312
        || kDevGridWide.x + kDevGridWide.w != 312 || kDevGridNoteWide.x + kDevGridNoteWide.w != 312
        || kDevGridWide.y + kDevGridWide.h + 1 > kDevGridNoteWide.y) {
        fprintf(stderr, "grid: the title, the screens, ▲▼, the notes, the exit or the key column meet or leave x 312\n");
        ++faults;
    }
    return faults;
}

/* The move to forget's and the relearner's own rectangles: today's numbers, which were the Summary's and the Bag's (the
 * move to forget's slots right of its column on its screen's paper, one under the other; the relearner's YES and NO
 * right of its narrow screens). */
static int LearnFaults(void)
{
    const CtrPokeTouchRect paper = PaperOf(kDevLearnScreen);
    int faults = 0;

    assert(Same(kDevLearnScreen, (CtrPokeTouchRect){8, 40, 304, 140}) && Same(kDevLearnColumn, (CtrPokeTouchRect){14, 46, 104, 128}));
    assert(Same(kDevLearnPic, (CtrPokeTouchRect){34, 82, 64, 64}));
    assert(Same(kDevRlNarrow, (CtrPokeTouchRect){8, 40, 184, 144}) && Same(kDevRlDescNarrow, (CtrPokeTouchRect){8, 190, 184, 42}));
    assert(Same(DevRlKey(0), (CtrPokeTouchRect){200, 40, 112, 30}) && Same(DevRlKey(1), (CtrPokeTouchRect){200, 78, 112, 38}));
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevLearnSlot(i);

        assert(Same(r, (CtrPokeTouchRect){124, (int16_t)(52 + 24 * i), 176, 22}));
        if (!Within(r, paper, 0) || r.x < kDevLearnColumn.x + kDevLearnColumn.w || (i > 0 && r.y < DevLearnSlot(i - 1).y + 22)) {
            fprintf(stderr, "the move to forget: slot %d leaves the paper right of the column, or meets the one above\n", i);
            ++faults;
        }
    }
    if (!Within(kDevLearnColumn, paper, 0) || !Within(kDevLearnPic, kDevLearnColumn, 0)
        || kDevLearnScreen.y + kDevLearnScreen.h + 1 > kDevTallNote.y) {
        fprintf(stderr, "the move to forget: the column leaves the paper, the picture the column, or the screen meets the note\n");
        ++faults;
    }
    for (int i = 0; i < 2; ++i)
        if (DevRlKey(i).x < kDevRlNarrow.x + kDevRlNarrow.w || DevRlKey(i).x < kDevRlDescNarrow.x + kDevRlDescNarrow.w) {
            fprintf(stderr, "the relearner: key %d reaches the narrow screens\n", i);
            ++faults;
        }
    return faults;
}

/* The shapes the views without a board share (plan 3): DevAcross gives the boarded rows back (the battle's wild row
 * and its moves' top row) and fits n keys of 1-8 across; the tall rows (DevTallRow: the Pokedex's list, the relearner's)
 * on the list's paper and a narrow one; the small note with its two keys and CLOSE beside it, the tall screen over the
 * wide key, the question's note beside YES / NO. */
static int SharedFaults(void)
{
    const CtrPokeTouchRect paper = {14, 46, 248, 132}, narrowPaper = {14, 46, 172, 132};
    int faults = 0;

    for (int i = 0; i < 4; ++i)
        assert(Same(DevAcross(4, i, 168, 62), kDevBatRow4[i]));
    for (int i = 0; i < 2; ++i)
        assert(Same(DevAcross(2, i, 72, 62), kDevBatMove[i]));
    for (int n = 1; n <= 8; ++n) {
        CtrPokeTouchRect last = DevAcross(n, n - 1, 8, 24);

        assert(DevAcross(n, 0, 8, 24).x == 8);
        if (last.x + last.w > 312 || last.w < 28) {
            fprintf(stderr, "DevAcross(%d): the last key {%d,%d,%d,%d} ends past 312 or is under 28 wide\n", n, last.x,
                    last.y, last.w, last.h);
            ++faults;
        }
        for (int i = 1; i < n; ++i)
            assert(!Overlap(DevAcross(n, i - 1, 8, 24), DevAcross(n, i, 8, 24)));
    }
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevTallRow(i, 0), n = DevTallRow(i, 1);

        assert(Same(r, (CtrPokeTouchRect){16, (int16_t)(48 + 24 * i), 244, 24}) && Same(n, (CtrPokeTouchRect){16, r.y, 168, 24}));
        if (!Within(r, paper, 0) || !Within(n, narrowPaper, 0)) {
            fprintf(stderr, "a tall row %d leaves its paper\n", i);
            ++faults;
        }
    }
    assert(Same(kDevSmallNote, (CtrPokeTouchRect){8, 190, 152, 42}) && Same(kDevSmallKey[0], (CtrPokeTouchRect){168, 190, 32, 40})
           && Same(kDevSmallKey[1], (CtrPokeTouchRect){204, 190, 32, 40}));
    assert(Same(kDevTallScreen, (CtrPokeTouchRect){8, 8, 304, 172}) && Same(kDevWideKey, (CtrPokeTouchRect){8, 188, 304, 42}));
    assert(Same(kDevAskNote, (CtrPokeTouchRect){8, 156, 200, 76}) && Same(kDevAskYes, (CtrPokeTouchRect){216, 156, 96, 36})
           && Same(kDevAskNo, (CtrPokeTouchRect){216, 198, 96, 34}));
    if (kDevSmallNote.x + kDevSmallNote.w > kDevSmallKey[0].x || kDevGridScreen.y + kDevGridScreen.h + 1 > kDevSmallNote.y
        || kDevTallScreen.y + kDevTallScreen.h + 1 > kDevWideKey.y || kDevAskNote.x + kDevAskNote.w > kDevAskYes.x) {
        fprintf(stderr, "the small note meets its keys or the list's screen, the tall screen the wide key, or the "
                        "question's note YES / NO\n");
        ++faults;
    }
    return faults;
}

/* OPTIONS: the four section keys across the top, the rows on the Bag's list paper (seven fit), a row's zones (LABEL,
 * PREV, NEXT: the value and ▶) with room for a 15-byte label and an 11-byte value at 7 px a glyph, so no German row can
 * cut; FRAME's sample under five rows and the look's panel under four, both on the paper. */
static int OptionsFaults(void)
{
    const CtrPokeTouchRect paper = {14, 46, 248, 132};
    int faults = 0;

    assert(DEV_OPT_LABEL_W >= (PT_OPT_LABEL_LEN - 1) * 7 && 100 - 14 >= (PT_OPT_VALUE_LEN - 1) * 7);
    assert(DevOptRow(0).x + DEV_OPT_LABEL_W == DEV_OPT_PREV_X && DEV_OPT_PREV_X == 136 && DEV_OPT_NEXT_X == 160);
    for (int i = 0; i < PT_OPT_MAX; ++i) {
        CtrPokeTouchRect r = DevOptRow(i);

        assert(Same(r, (CtrPokeTouchRect){16, (int16_t)(48 + 18 * i), 244, 18}) && Same(r, DevListRow(i, 0, 18)));
        if (!Within(r, paper, 0)) {
            fprintf(stderr, "OPTIONS: row %d leaves the list's paper\n", i);
            ++faults;
        }
    }
    assert(Same(kDevOptSample, (CtrPokeTouchRect){202, 138, 56, 40}) && Same(kDevOptLook, (CtrPokeTouchRect){16, 122, 244, 54}));
    if (!Within(kDevOptSample, paper, 0) || kDevOptSample.y < DevOptRow(4).y + DevOptRow(4).h
        || !Within(kDevOptLook, paper, 0) || kDevOptLook.y < DevOptRow(3).y + DevOptRow(3).h) {
        fprintf(stderr, "OPTIONS: FRAME's sample or the look's panel leaves the paper or meets the rows over it\n");
        ++faults;
    }
    for (int s = 0; s < 4; ++s)
        if (DevAcross(4, s, 8, 24).y + 24 + DEV_DROP > kDevGridScreen.y) {
            fprintf(stderr, "OPTIONS: section key %d reaches the list's screen\n", s);
            ++faults;
        }
    return faults;
}

/* SAVE, the Trainer Card and the letter: SAVE's five rows and the player's icon on HOME's screen's paper, the rows one
 * under the other and clear of the icon; the card's front (its first line over the picture, its rows beside it, the
 * five stars under them, the eight badges' places side by side under the stars) and back (the rule under the name, the
 * five records under it) on the tall screen's paper, FLIP beside BACK; the letter's icon (centred), name, rule and
 * writer on the tall screen's paper, one under the other. */
static int ScreensFaults(void)
{
    const CtrPokeTouchRect homePaper = {14, 14, 292, 128}, tallPaper = {14, 14, 292, 160};
    const CtrPokeTouchRect stars = {DEV_CARD_STAR_X, DEV_CARD_STAR_Y, DEV_CARD_STAR_STEP * (DEV_CARD_STARS - 1) + 12, 12};
    const CtrPokeTouchRect letter[4] = {kDevMailIcon, kDevMailName, kDevMailRule, kDevMailFrom};
    int faults = 0;

    assert(Same(kDevSaveSprite, (CtrPokeTouchRect){224, 46, 64, 64}) && Same(kDevCardFlip, (CtrPokeTouchRect){8, 188, 228, 42})
           && Same(kDevCardPic, (CtrPokeTouchRect){226, 40, 64, 64}));
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DevSaveRow(i);

        if (!Within(r, homePaper, 0) || r.x + r.w > kDevSaveSprite.x || (i && DevSaveRow(i - 1).y + DevSaveRow(i - 1).h > r.y)) {
            fprintf(stderr, "SAVE: row %d leaves the paper, meets the icon or the row over it\n", i);
            ++faults;
        }
    }
    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect r = DevCardRow(i);

        if (!Within(r, tallPaper, 0) || (i ? r.x + r.w > kDevCardPic.x : r.y + r.h > kDevCardPic.y)
            || (i && DevCardRow(i - 1).y + DevCardRow(i - 1).h > r.y)) {
            fprintf(stderr, "card: front row %d leaves the paper, meets the picture or the row over it\n", i);
            ++faults;
        }
    }
    for (int i = 0; i < 8; ++i) {
        CtrPokeTouchRect b = DevCardBadge(i);

        assert(Same(b, (CtrPokeTouchRect){(int16_t)(20 + 34 * i), 132, 32, 32}));
        if (!Within(b, tallPaper, 0) || (i && DevCardBadge(i - 1).x + DevCardBadge(i - 1).w > b.x)) {
            fprintf(stderr, "card: badge %d leaves the paper or meets the one before\n", i);
            ++faults;
        }
    }
    if (!Within(kDevSaveSprite, homePaper, 0) || !Within(kDevCardPic, tallPaper, 0) || !Within(stars, tallPaper, 0)
        || stars.y < DevCardRow(3).y + DevCardRow(3).h || stars.y < kDevCardPic.y + kDevCardPic.h
        || stars.y + stars.h > DevCardBadge(0).y) {
        fprintf(stderr, "SAVE's icon, the card's picture or its stars leave the paper or meet the rows or badges\n");
        ++faults;
    }
    if (!Within(kDevCardRule, tallPaper, 0) || kDevCardRule.y < DevCardRow(0).y + DevCardRow(0).h) {
        fprintf(stderr, "card: the back's rule leaves the paper or meets the name\n");
        ++faults;
    }
    for (int i = 0; i < PT_CARD_ROWS; ++i) {
        CtrPokeTouchRect r = DevCardBackRow(i);

        if (!Within(r, tallPaper, 0) || r.y < (i ? DevCardBackRow(i - 1).y + DevCardBackRow(i - 1).h : kDevCardRule.y + 1)) {
            fprintf(stderr, "card: record %d leaves the paper or meets what is over it\n", i);
            ++faults;
        }
    }
    if (kDevCardFlip.x + kDevCardFlip.w > kDevTallBack.x || kDevCardFlip.y != kDevTallBack.y
        || kDevTallScreen.y + kDevTallScreen.h + 1 > kDevCardFlip.y) {
        fprintf(stderr, "card: FLIP meets BACK or the screen\n");
        ++faults;
    }
    assert(kDevMailIcon.x + kDevMailIcon.w / 2 == kDevTallScreen.x + kDevTallScreen.w / 2);
    for (int i = 0; i < 4; ++i)
        if (!Within(letter[i], tallPaper, 0) || (i && letter[i - 1].y + letter[i - 1].h > letter[i].y)) {
            fprintf(stderr, "the letter: piece %d leaves the paper or meets the one over it\n", i);
            ++faults;
        }
    return faults;
}

/* The Pokedex: its three tabs (today's places: three across, 8 px apart); the top menu's six keys a page (three across,
 * two rows) under the tabs and over the small note; a habitat's four slots under the title and over the small note; a
 * Pokemon's page on the tall screen's paper: the picture, the data beside it and short of the footprint, the rule under
 * them and the entry under the rule. */
static int DexFaults(void)
{
    const CtrPokeTouchRect tallPaper = {14, 14, 292, 160};
    const CtrPokeTouchRect page[5] = {kDevDexPic, kDevDexData, kDevDexFoot, kDevDexRule, kDevDexEntry};
    int faults = 0;

    for (int i = 0; i < 3; ++i)
        assert(Same(DevDexTabKey(i), DevAcross(3, i, 8, 24)) && Same(DevDexTabKey(i), (CtrPokeTouchRect){(int16_t)(8 + 104 * i), 8, 96, 24}));
    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect r = DevDexItem(i);

        assert(Same(r, DevAcross(3, i % 3, 40 + 72 * (i / 3), 68)));
        if (r.y < DevDexTabKey(0).y + DevDexTabKey(0).h + DEV_DROP || r.y + r.h + DEV_DROP > kDevSmallNote.y) {
            fprintf(stderr, "Pokedex: menu key %d meets the tabs or the small note\n", i);
            ++faults;
        }
    }
    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect r = DevDexSlot(i);

        assert(Same(r, (CtrPokeTouchRect){(int16_t)(8 + 156 * (i % 2)), (int16_t)(40 + 74 * (i / 2)), 148, 70}));
        if (r.y < kDevGridTitle.y + kDevGridTitle.h + 1 || r.y + r.h + DEV_DROP > kDevSmallNote.y) {
            fprintf(stderr, "Pokedex: slot %d meets the title or the small note\n", i);
            ++faults;
        }
    }
    assert(Same(kDevDexPic, (CtrPokeTouchRect){20, 20, 80, 80}) && kDevDexFoot.x == 282 && kDevDexFoot.y == 20
           && kDevDexEntry.w == 280 && kDevDexEntry.h >= 5 * 13);
    assert(Same(kDevDexArea, (CtrPokeTouchRect){8, 188, 110, 42}) && Same(kDevDexCry, (CtrPokeTouchRect){126, 188, 110, 42}));
    for (int i = 0; i < 5; ++i)
        if (!Within(page[i], tallPaper, 0)) {
            fprintf(stderr, "Pokedex: piece %d of a Pokemon's page leaves the paper\n", i);
            ++faults;
        }
    if (kDevDexData.x < kDevDexPic.x + kDevDexPic.w || kDevDexData.x + kDevDexData.w > kDevDexFoot.x
        || kDevDexRule.y < kDevDexPic.y + kDevDexPic.h || kDevDexRule.y < kDevDexData.y + kDevDexData.h
        || kDevDexEntry.y < kDevDexRule.y + kDevDexRule.h) {
        fprintf(stderr, "Pokedex: a Pokemon's page's pieces meet each other\n");
        ++faults;
    }
    if (kDevTallScreen.y + kDevTallScreen.h + 1 > kDevDexArea.y) {
        fprintf(stderr, "Pokedex: AREA and CRY meet the screen\n");
        ++faults;
    }
    return faults;
}

/* The field's choices on the grid (boards def_choice_yesno, def_choice_list, def_choice_grid): n keys (1-8) in
 * FireRed's one column (YES / NO two tall keys; up to four across the screen, 38 tall and 44 apart; five to eight in two
 * columns going down first, the left one always four) or FireRed's grid of 2 or 3 columns across; every key's drop ends
 * above the footer (the grid's note and its exit, BACK); a grid of one column is the one column's layout. (The move to
 * forget's and the relearner's own rectangles: LearnFaults.) */
static int ChoiceFaults(void)
{
    static const int kCols[4] = {0, 1, 2, 3};
    int faults = 0;

    assert(Same(DevChoiceKey(2, 0, 0), (CtrPokeTouchRect){8, 8, 304, 84}) && Same(DevChoiceKey(2, 0, 1), (CtrPokeTouchRect){8, 100, 304, 84}));
    assert(Same(DevChoiceKey(4, 0, 3), (CtrPokeTouchRect){8, 140, 304, 38}) && Same(DevChoiceKey(5, 0, 3), (CtrPokeTouchRect){8, 140, 148, 38}));
    assert(Same(DevChoiceKey(5, 0, 4), (CtrPokeTouchRect){164, 8, 148, 38}) && Same(DevChoiceKey(8, 0, 7), (CtrPokeTouchRect){164, 140, 148, 38}));
    assert(Same(DevChoiceKey(6, 3, 4), (CtrPokeTouchRect){112, 52, 96, 38}) && Same(DevChoiceKey(8, 3, 7), (CtrPokeTouchRect){112, 96, 96, 38}));
    assert(Same(DevChoiceKey(8, 2, 7), (CtrPokeTouchRect){164, 140, 148, 38}));
    for (int n = 1; n <= 8; ++n)
        for (int i = 0; i < n; ++i)
            if (!Same(DevChoiceKey(n, 1, i), DevChoiceKey(n, 0, i))) {
                fprintf(stderr, "choice: key %d of %d in a grid of one column is not the one column's\n", i, n);
                ++faults;
            }
    for (int c = 0; c < 4; ++c)
        for (int n = 1; n <= 8; ++n)
            for (int i = 0; i < n; ++i) {
                CtrPokeTouchRect k = DevChoiceKey(n, kCols[c], i);

                if (k.y + k.h + DEV_DROP > kDevGridNote.y || k.y + k.h + DEV_DROP > kDevGridExit.y) {
                    fprintf(stderr, "choice: key %d of %d (%d columns) reaches the footer\n", i, n, kCols[c]);
                    ++faults;
                }
            }
    return faults;
}

/* The naming keyboard: its screen across the top, the five keys across under it (the three pages, DELETE, OK), and
 * under them four rows of FireRed's 8 columns (34 wide) or of its symbols' 6 (47 wide). */
static int NamingFaults(void)
{
    int faults = 0;

    assert(Same(kDevNameScreen, (CtrPokeTouchRect){8, 8, 304, 40}));
    for (int i = 0; i < 5; ++i)
        assert(Same(DevNameTop(i), DevAcross(5, i, 56, 24)));
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 8; ++c)
            assert(Same(DevNameKey(8, r, c), DevAcross(8, c, 88 + 36 * r, 32)) && (c >= 6 || Same(DevNameKey(6, r, c), DevAcross(6, c, 88 + 36 * r, 32))));
    assert(DevNameKey(8, 0, 0).w == 34 && DevNameKey(6, 0, 0).w == 47);
    if (kDevNameScreen.y + kDevNameScreen.h + 1 > DevNameTop(0).y) {
        fprintf(stderr, "naming: the screen meets the keys under it\n");
        ++faults;
    }
    if (DevNameTop(0).y + DevNameTop(0).h + DEV_DROP > DevNameKey(8, 0, 0).y) {
        fprintf(stderr, "naming: the top keys meet the first row\n");
        ++faults;
    }
    return faults;
}

/* The PC box: its screen's paper is FireRed's 160x144 wallpaper at 1:1, the band and the thirty places (or the Party's
 * seven rows) on it; the right column (◀ ▶, PARTY, CLOSE BOX and the hand's screen; a menu's keys; the chooser's, its
 * CANCEL under the hand) beside it; the note under them all, narrow beside a menu's keys and the chooser's CANCEL. */
static int BoxFaults(void)
{
    const CtrPokeTouchRect paper = {(int16_t)(kDevBoxScreen.x + 6), (int16_t)(kDevBoxScreen.y + 6),
                                    (int16_t)(kDevBoxScreen.w - 12), (int16_t)(kDevBoxScreen.h - 12)};
    const CtrPokeTouchRect column = {188, 8, 124, 224};
    int faults = 0;

    assert(Same(paper, (CtrPokeTouchRect){14, 14, 160, 144}) && Same(kDevBoxTitle, (CtrPokeTouchRect){14, 14, 160, 24}));
    assert(Same(DevBoxCell(0), (CtrPokeTouchRect){22, 38, 24, 24}) && Same(DevBoxCell(29), (CtrPokeTouchRect){142, 134, 24, 24}));
    assert(Same(DevBoxPartyRow(0), (CtrPokeTouchRect){16, 16, 156, 20}) && Same(DevBoxPartyRow(6), (CtrPokeTouchRect){16, 136, 156, 20}));
    assert(Same(DevBoxMenuKey(5, 4), (CtrPokeTouchRect){188, 184, 124, 40}) && Same(DevBoxMenuKey(7, 6), (CtrPokeTouchRect){188, 200, 124, 28}));
    assert(Same(kDevBoxCancel, (CtrPokeTouchRect){188, 172, 124, 56}));
    for (int i = 0; i < 30; ++i) {
        CtrPokeTouchRect c = DevBoxCell(i);

        if (!Within(c, paper, 0) || c.y < kDevBoxTitle.y + kDevBoxTitle.h) {
            fprintf(stderr, "box: place %d leaves the paper or meets the band\n", i);
            ++faults;
        }
    }
    for (int i = 0; i < 7; ++i)
        if (!Within(DevBoxPartyRow(i), paper, 0)) {
            fprintf(stderr, "box: Party row %d leaves the paper\n", i);
            ++faults;
        }
    if (!Within(kDevBoxTitle, paper, 0) || !Within(kDevBoxHand, column, 0) || !Within(kDevBoxChoose, column, 1)
        || !Within(kDevBoxCancel, column, 1) || kDevBoxHand.y + kDevBoxHand.h + 1 > kDevBoxCancel.y
        || kDevBoxCancel.y != kDevBoxNoteNarrow.y) {
        fprintf(stderr, "box: the band, the hand's screen or the chooser's keys out of place\n");
        ++faults;
    }
    for (int n = 1; n <= 7; ++n)
        for (int i = 0; i < n; ++i)
            if (!Within(DevBoxMenuKey(n, i), column, 1)) {
                fprintf(stderr, "box: key %d of a menu of %d leaves the right column\n", i, n);
                ++faults;
            }
    if (kDevBoxScreen.x + kDevBoxScreen.w + 8 > column.x || kDevBoxNoteNarrow.x + kDevBoxNoteNarrow.w + 8 > column.x
        || kDevBoxClose.y + kDevBoxClose.h + DEV_DROP > kDevBoxHand.y || kDevBoxChoose.y + kDevBoxChoose.h + DEV_DROP > kDevBoxHand.y
        || kDevBoxScreen.y + kDevBoxScreen.h + 1 > kDevBoxNote.y || kDevBoxHand.y + kDevBoxHand.h + 1 > kDevBoxNote.y
        || kDevBoxNote.y != kDevBoxNoteNarrow.y) {
        fprintf(stderr, "box: the screen, the column, the hand's screen and the note meet\n");
        ++faults;
    }
    return faults;
}

/* Easy Chat: the phrase's screen over its slots (two rows of up to five), the mode keys and the buttons; the groups'
 * title over six rows of four, or the letters in FireRed's rows of 6, 6, 7, 7 of seven places, the toggle and BACK
 * under them; a list's title over the list's screen (the rows two a row, the rail on its paper right of them), ▲ ▼ and
 * the strip beside it (its letters on its paper), the bottom keys under them all; a question's note under the slots. */
static int EasyChatFaults(void)
{
    static const int kStart[4] = {0, 6, 12, 19};
    const CtrPokeTouchRect listPaper = {14, 58, 224, 124}, stripPaper = {290, 58, 16, 124};
    int faults = 0;

    assert(Same(kDevEcPhrase, (CtrPokeTouchRect){8, 8, 304, 36}) && Same(kDevEcListTitle, (CtrPokeTouchRect){8, 6, 304, 40}));
    assert(Same(kDevEcList, (CtrPokeTouchRect){8, 52, 236, 136}) && Same(kDevEcRail, (CtrPokeTouchRect){220, 58, 18, 124}));
    assert(Same(kDevEcUp, (CtrPokeTouchRect){252, 52, 28, 64}) && Same(kDevEcDown, (CtrPokeTouchRect){252, 120, 28, 64}));
    assert(Same(kDevEcStrip, (CtrPokeTouchRect){284, 52, 28, 136}));
    assert(DevEcListKey(2).x + DevEcListKey(2).w == 312 && DevEcToggle(1).x + DevEcToggle(1).w == 312);   /* BACK at the right */
    for (int n = 1; n <= 9; ++n)
        for (int i = 0; i < n; ++i)
            assert(Same(DevEcChip(n, i), DevAcross((n + 1) / 2, i % ((n + 1) / 2), 52 + 40 * (i / ((n + 1) / 2)), 34)));
    for (int i = 0; i < 3; ++i)
        assert(Same(DevEcFooter(i), DevAcross(3, i, 178, 52)) && Same(DevEcListKey(i), DevAcross(3, i, 196, 36))
               && (i == 2 || (Same(DevEcMode(i), DevAcross(2, i, 136, 30)) && Same(DevEcToggle(i), DevAcross(2, i, 206, 26)))));
    for (int i = 0; i < 21; ++i)
        assert(Same(DevEcCell(i), DevAcross(4, i % 4, 40 + 27 * (i / 4), 24)));
    for (int r = 0; r < 4; ++r)
        for (int i = kStart[r]; i < (r < 3 ? kStart[r + 1] : 26); ++i)
            assert(Same(DevEcLetter(i), DevAcross(7, i - kStart[r], 40 + 32 * r, 28)));
    for (int k = 0; k < 12; ++k) {
        CtrPokeTouchRect row = DevEcRow(k);

        assert(Same(row, (CtrPokeTouchRect){(int16_t)(16 + 102 * (k % 2)), (int16_t)(60 + 20 * (k / 2)), 100, 20}));
        if (!Within(row, listPaper, 0) || row.x + row.w > kDevEcRail.x) {
            fprintf(stderr, "Easy Chat: row place %d leaves the list's paper or meets the rail\n", k);
            ++faults;
        }
    }
    for (int i = 0; i < 26; ++i) {
        CtrPokeTouchRect c = DevEcStripCell(i);

        assert(c.x + 1 == 291 + 8 * (i / 13) && c.y + 1 == 60 + 9 * (i % 13));      /* the letter's capitals */
        if (!Within(c, stripPaper, 0)) {
            fprintf(stderr, "Easy Chat: the strip's letter %d leaves its paper\n", i);
            ++faults;
        }
    }
    if (!Within(kDevEcRail, listPaper, 0) || kDevEcPhrase.y + kDevEcPhrase.h + 1 > DevEcChip(9, 0).y
        || DevEcChip(9, 5).y + DevEcChip(9, 5).h + DEV_DROP > DevEcMode(0).y || DevEcMode(0).y + DevEcMode(0).h + DEV_DROP > DevEcFooter(0).y
        || DevEcCell(20).y + DevEcCell(20).h + DEV_DROP > DevEcToggle(0).y || DevEcLetter(25).y + DevEcLetter(25).h + DEV_DROP > DevEcToggle(0).y
        || kDevGridTitle.y + kDevGridTitle.h + 1 > DevEcCell(0).y || kDevEcListTitle.y + kDevEcListTitle.h + 1 > kDevEcList.y
        || kDevEcList.x + kDevEcList.w + 8 > kDevEcUp.x || kDevEcUp.y + kDevEcUp.h + DEV_DROP > kDevEcDown.y
        || kDevEcDown.x + kDevEcDown.w + 4 > kDevEcStrip.x || kDevEcList.y + kDevEcList.h + 1 > DevEcListKey(0).y
        || kDevEcStrip.y + kDevEcStrip.h + 1 > DevEcListKey(0).y || kDevEcDown.y + kDevEcDown.h + DEV_DROP > DevEcListKey(0).y
        || DevEcChip(9, 5).y + DevEcChip(9, 5).h + DEV_DROP > kDevAskNote.y) {
        fprintf(stderr, "Easy Chat: its screens and keys meet\n");
        ++faults;
    }
    return faults;
}

/* FireRed's picture: its screen centred across the top, the paper (6 px in) exactly where the GPU draws the picture
 * (PT_VIEW_*); under it the tall note beside BACK (kDevTallNote, kDevTallBack), or the note across in their place
 * without BACK. The idle screen: centred on the shell, its paper holding the 224x126 logo centred at (48, 57). */
static int FrameFaults(void)
{
    const CtrPokeTouchRect view = {PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H}, logo = {48, 57, 224, 126};
    const CtrPokeTouchRect paper = {(int16_t)(kDevViewScreen.x + 6), (int16_t)(kDevViewScreen.y + 6),
                                    (int16_t)(kDevViewScreen.w - 12), (int16_t)(kDevViewScreen.h - 12)};
    const CtrPokeTouchRect idlePaper = {(int16_t)(kDevIdleScreen.x + 6), (int16_t)(kDevIdleScreen.y + 6),
                                        (int16_t)(kDevIdleScreen.w - 12), (int16_t)(kDevIdleScreen.h - 12)};
    int faults = 0;

    assert(Same(kDevViewScreen, (CtrPokeTouchRect){34, 8, 252, 172}) && Same(kDevViewNoteWide, (CtrPokeTouchRect){8, 188, 304, 44})
           && Same(kDevIdleScreen, (CtrPokeTouchRect){40, 45, 240, 150}));
    assert(Same(paper, view) && Same(view, (CtrPokeTouchRect){40, 14, 240, 160}) && Same(idlePaper, (CtrPokeTouchRect){46, 51, 228, 138}));
    if (kDevViewScreen.x + kDevViewScreen.w / 2 != 160 || kDevViewScreen.y + kDevViewScreen.h + 1 > kDevTallNote.y
        || kDevTallNote.x + kDevTallNote.w > kDevTallBack.x || kDevViewNoteWide.x != kDevTallNote.x || kDevViewNoteWide.y != kDevTallNote.y
        || kDevViewNoteWide.h != kDevTallNote.h || kDevViewNoteWide.x + kDevViewNoteWide.w != kDevTallBack.x + kDevTallBack.w) {
        fprintf(stderr, "FireRed's picture: its screen is not centred or meets the note, the note BACK, or the note across "
                        "is not the note and BACK's place\n");
        ++faults;
    }
    if (kDevIdleScreen.x + kDevIdleScreen.w / 2 != 160 || kDevIdleScreen.y + kDevIdleScreen.h / 2 != 120
        || !Within(logo, idlePaper, 0) || logo.x - idlePaper.x != idlePaper.x + idlePaper.w - (logo.x + logo.w)
        || logo.y - idlePaper.y != idlePaper.y + idlePaper.h - (logo.y + logo.h)) {
        fprintf(stderr, "the idle screen is not centred, or the logo not centred on its paper\n");
        ++faults;
    }
    return faults;
}

int main(void)
{
    /* Every rectangle the header holds, by name, and the keys of each view state. */
    const CtrPokeTouchRect homeKeys[] = {kDevHomeKey[0], kDevHomeKey[1], kDevHomeKey[2], kDevHomeKey[3], kDevHomeKey[4],
                                         kDevHomeKey[5], kDevHomeRun, kDevHomeSpeed};
    const CtrPokeTouchRect batTrainer[] = {kDevBatFight, kDevBatRow3[0], kDevBatRow3[1], kDevBatRow3[2]};
    const CtrPokeTouchRect batWild[] = {kDevBatFight, kDevBatRow4[0], kDevBatRow4[1], kDevBatRow4[2], kDevBatRow4[3]};
    const CtrPokeTouchRect batMoves[] = {kDevBatMove[0], kDevBatMove[1], kDevBatMove[2], kDevBatMove[3], kDevBatCancel};
    const CtrPokeTouchRect ptyBays[] = {DevPtyBay(0), DevPtyBay(1), DevPtyBay(2), DevPtyBay(3), DevPtyBay(4), DevPtyBay(5),
                                        kDevGridExit};
    const CtrPokeTouchRect ptyMulti[] = {DevPtyBay(0), DevPtyBay(1), DevPtyBay(2), DevPtyBay(3), DevPtyBay(4), DevPtyBay(5),
                                         kDevPtyConfirm, kDevGridExit};
    const CtrPokeTouchRect ptyOk[] = {kDevGridExit};
    const CtrPokeTouchRect ptyMenu[] = {DevPtyMenuKey(0), DevPtyMenuKey(1), DevPtyMenuKey(2), DevPtyMenuKey(3),
                                        DevPtyMenuKey(4), DevPtyMenuKey(5), kDevGridExit};
    const CtrPokeTouchRect sumKeys[] = {DevGridTab(4, 0), DevGridTab(4, 1), DevGridTab(4, 2), DevGridTab(4, 3), kDevGridUp,
                                        kDevGridDown, kDevGridExit};
    const CtrPokeTouchRect bagList[] = {DevGridTab(3, 0), DevGridTab(3, 1), DevGridTab(3, 2), kDevGridUp, kDevGridDown, kDevGridExit};
    const CtrPokeTouchRect bagCase[] = {kDevBagCaseKey, kDevGridUp, kDevGridDown, kDevGridExit};
    const CtrPokeTouchRect bagQty[] = {kDevBagQtyUp, kDevBagQtyDown, kDevBagQtyOk, kDevGridExit};
    const CtrPokeTouchRect bagYesNo[] = {DevGridColumnKey(40, 36, 30, 0), kDevGridExit};
    const CtrPokeTouchRect optKeys[] = {DevAcross(4, 0, 8, 24), DevAcross(4, 1, 8, 24), DevAcross(4, 2, 8, 24),
                                        DevAcross(4, 3, 8, 24), kDevGridUp, kDevGridDown, kDevSmallKey[0], kDevSmallKey[1],
                                        kDevGridExit};
    const CtrPokeTouchRect askKeys[] = {kDevAskYes, kDevAskNo};
    const CtrPokeTouchRect saveAsk[] = {kDevAskYes, kDevAskNo}, saveResult[] = {kDevAskYes};
    const CtrPokeTouchRect cardKeys[] = {kDevCardFlip, kDevTallBack}, mailKeys[] = {kDevWideKey};
    const CtrPokeTouchRect dexMenu[] = {DevDexTabKey(0), DevDexTabKey(1), DevDexTabKey(2), DevDexItem(0), DevDexItem(1),
                                        DevDexItem(2), DevDexItem(3), DevDexItem(4), DevDexItem(5), kDevSmallKey[0],
                                        kDevSmallKey[1], kDevGridExit};
    const CtrPokeTouchRect dexList[] = {kDevGridUp, kDevGridDown, kDevGridExit};
    const CtrPokeTouchRect dexCategory[] = {DevDexSlot(0), DevDexSlot(1), DevDexSlot(2), DevDexSlot(3), kDevSmallKey[0],
                                            kDevSmallKey[1], kDevGridExit};
    const CtrPokeTouchRect dexPage[] = {kDevDexArea, kDevDexCry, kDevTallBack}, dexAdded[] = {kDevWideKey};
    const CtrPokeTouchRect learnKeys[] = {kDevTallBack};
    const CtrPokeTouchRect relearnList[] = {kDevGridUp, kDevGridDown, kDevGridExit};
    const CtrPokeTouchRect relearnYesNo[] = {DevRlKey(0), DevRlKey(1)};
    const CtrPokeTouchRect boxKeys[] = {kDevBoxPrev, kDevBoxNext, kDevBoxParty, kDevBoxClose};
    const CtrPokeTouchRect boxYesNo[] = {DevBoxMenuKey(2, 0), DevBoxMenuKey(2, 1)};
    const CtrPokeTouchRect boxChooser[] = {kDevBoxPrev, kDevBoxNext, kDevBoxChoose, kDevBoxCancel};
    const CtrPokeTouchRect boxMarks[] = {DevBoxMenuKey(6, 0), DevBoxMenuKey(6, 1), DevBoxMenuKey(6, 2), DevBoxMenuKey(6, 3),
                                         DevBoxMenuKey(6, 4), DevBoxMenuKey(6, 5)};
    const CtrPokeTouchRect ecList[] = {kDevEcUp, kDevEcDown, DevEcListKey(0), DevEcListKey(1), DevEcListKey(2)};
    const CtrPokeTouchRect ecAsk[] = {kDevAskYes, kDevAskNo};
    const CtrPokeTouchRect frameKeys[] = {kDevTallBack};
    CtrPokeTouchRect bagMenu[PT_BAG_MENU_MAX + 1][PT_BAG_MENU_MAX], choiceKeys[3][8][9], nameKeys[2][5 + 32];
    CtrPokeTouchRect boxMenu[7][7], ecEdit[9][9 + 5], ecGrid[2][26 + 2];
    DevGroup bagMenus[PT_BAG_MENU_MAX + 1], choiceGroups[3][8], nameGroups[2], boxMenus[7], ecEdits[9], ecGrids[2];
    static const char *const kEcEditName[9] = {"Easy Chat phrase of 1", "Easy Chat phrase of 2", "Easy Chat phrase of 3",
                                               "Easy Chat phrase of 4", "Easy Chat phrase of 5", "Easy Chat phrase of 6",
                                               "Easy Chat phrase of 7", "Easy Chat phrase of 8", "Easy Chat phrase of 9"};
    static const char *const kBoxMenuName[7] = {"box menu of 1", "box menu of 2", "box menu of 3", "box menu of 4",
                                                "box menu of 5", "box menu of 6", "box menu of 7"};
    static const char *const kNameGroup[2] = {"naming, 8 columns", "naming, 6 columns (symbols)"};
    static char choiceName[3][8][40];
    static const char *const kBagMenuName[PT_BAG_MENU_MAX + 1] = {NULL, NULL, "Bag menu of 2", "Bag menu of 3",
                                                                  "Bag menu of 4", "Bag menu of 5", "Bag menu of 6"};
    const DevNamedRect rects[] = {
        {"HOME screen", kDevHomeScreen}, {"HOME POKeDEX", kDevHomeKey[0]}, {"HOME POKeMON", kDevHomeKey[1]},
        {"HOME BAG", kDevHomeKey[2]}, {"HOME card", kDevHomeKey[3]}, {"HOME SAVE", kDevHomeKey[4]},
        {"HOME OPTIONS", kDevHomeKey[5]}, {"HOME RUN", kDevHomeRun}, {"HOME SPD", kDevHomeSpeed},
        {"HOME name", kDevHomeName}, {"HOME region line", kDevHomeRegion}, {"HOME rule", kDevHomeRule},
        {"HOME region key", kDevHomeMapKey}, {"HOME FLY", kDevHomeFly}, {"HOME TIME", kDevHomeTime},
        {"HOME cell 0", DevHomeCell(0)}, {"HOME cell 5", DevHomeCell(5)},
        {"battle screen", kDevBatScreen}, {"battle moves' screen", kDevBatMoveScreen}, {"battle FIGHT", kDevBatFight},
        {"battle BAG (3)", kDevBatRow3[0]}, {"battle POKeMON (3)", kDevBatRow3[1]}, {"battle RUN (3)", kDevBatRow3[2]},
        {"battle BAG (4)", kDevBatRow4[0]}, {"battle BALL", kDevBatRow4[1]}, {"battle POKeMON (4)", kDevBatRow4[2]},
        {"battle RUN (4)", kDevBatRow4[3]}, {"battle move 0", kDevBatMove[0]}, {"battle move 1", kDevBatMove[1]},
        {"battle move 2", kDevBatMove[2]}, {"battle move 3", kDevBatMove[3]}, {"battle CANCEL", kDevBatCancel},
        {"Party CONFIRM", kDevPtyConfirm}, {"Party slim bay 5", DevPtySlim(5)}, {"Party menu key 5", DevPtyMenuKey(5)},
        {"Party level-up screen", kDevPtyLevelScreen}, {"Party bay 0", DevPtyBay(0)}, {"Party bay 5", DevPtyBay(5)},
        {"Summary column", kDevSumColumn}, {"Summary picture", kDevSumPic}, {"Summary switch", kDevSumIvSwitch},
        {"Summary switch's touch", kDevSumIvSwitchHit}, {"Summary row 0", DevSumRow(0)}, {"Summary CANCEL row", DevSumRow(4)},
        {"Bag case key", kDevBagCaseKey}, {"Bag description's icon", kDevBagDescIcon}, {"Bag how many: up", kDevBagQtyUp},
        {"Bag how many: count", kDevBagQtyScreen}, {"Bag how many: down", kDevBagQtyDown}, {"Bag how many: OK", kDevBagQtyOk},
        {"Bag menu key 4 of 5", DevBagMenuKey(5, 3)}, {"Bag menu key 5 of 6", DevBagMenuKey(6, 4)},
        {"grid tab 1 of 3", DevGridTab(3, 0)}, {"grid narrow row 4", DevGridRow(4, 1)},
        {"grid title", kDevGridTitle}, {"grid screen", kDevGridScreen}, {"grid narrow screen", kDevGridScreenNarrow},
        {"grid wide screen", kDevGridWide}, {"grid up", kDevGridUp}, {"grid down", kDevGridDown}, {"grid note", kDevGridNote},
        {"grid wide note", kDevGridNoteWide}, {"grid exit", kDevGridExit}, {"grid tab 3 of 3", DevGridTab(3, 2)},
        {"grid tab 4 of 4", DevGridTab(4, 3)}, {"grid row 4", DevGridRow(4, 0)}, {"grid column key", DevGridColumnKey(40, 36, 30, 3)},
        {"small note", kDevSmallNote}, {"small key 0", kDevSmallKey[0]}, {"small key 1", kDevSmallKey[1]},
        {"tall screen", kDevTallScreen}, {"tall note", kDevTallNote}, {"tall BACK", kDevTallBack}, {"wide key", kDevWideKey},
        {"question's note", kDevAskNote},
        {"question's YES", kDevAskYes}, {"question's NO", kDevAskNo}, {"tall row 4", DevTallRow(4, 0)},
        {"OPTIONS row 0", DevOptRow(0)}, {"OPTIONS row 6", DevOptRow(PT_OPT_MAX - 1)}, {"OPTIONS FRAME sample", kDevOptSample},
        {"OPTIONS look panel", kDevOptLook},
        {"SAVE row 0", DevSaveRow(0)}, {"SAVE row 4", DevSaveRow(4)}, {"SAVE icon", kDevSaveSprite},
        {"card FLIP", kDevCardFlip}, {"card picture", kDevCardPic}, {"card first line", DevCardRow(0)},
        {"card row 3", DevCardRow(3)}, {"card badge 0", DevCardBadge(0)}, {"card badge 7", DevCardBadge(7)},
        {"card rule", kDevCardRule}, {"card record 0", DevCardBackRow(0)}, {"card record 4", DevCardBackRow(PT_CARD_ROWS - 1)},
        {"letter icon", kDevMailIcon}, {"letter name", kDevMailName}, {"letter rule", kDevMailRule},
        {"letter writer", kDevMailFrom},
        {"Pokedex tab 0", DevDexTabKey(0)}, {"Pokedex tab 2", DevDexTabKey(2)},
        {"Pokedex menu key 0", DevDexItem(0)}, {"Pokedex menu key 5", DevDexItem(5)}, {"Pokedex slot 0", DevDexSlot(0)},
        {"Pokedex slot 3", DevDexSlot(3)}, {"Pokedex picture", kDevDexPic}, {"Pokedex data", kDevDexData},
        {"Pokedex footprint", kDevDexFoot}, {"Pokedex rule", kDevDexRule}, {"Pokedex entry", kDevDexEntry},
        {"Pokedex AREA", kDevDexArea}, {"Pokedex CRY", kDevDexCry},
        {"move to forget's screen", kDevLearnScreen}, {"move to forget's column", kDevLearnColumn},
        {"move to forget's picture", kDevLearnPic}, {"move to forget's slot 4", DevLearnSlot(4)},
        {"relearner's narrow screen", kDevRlNarrow}, {"relearner's narrow description", kDevRlDescNarrow},
        {"relearner's YES", DevRlKey(0)}, {"relearner's NO", DevRlKey(1)},
        {"choice YES", DevChoiceKey(2, 0, 0)}, {"choice NO", DevChoiceKey(2, 0, 1)}, {"choice key 7 of 8", DevChoiceKey(8, 0, 7)},
        {"choice grid key 7 of 8", DevChoiceKey(8, 3, 7)},
        {"naming screen", kDevNameScreen}, {"naming OK", DevNameTop(4)}, {"naming key 3,7", DevNameKey(8, 3, 7)},
        {"naming symbol 3,5", DevNameKey(6, 3, 5)},
        {"box screen", kDevBoxScreen}, {"box band", kDevBoxTitle}, {"box place 0", DevBoxCell(0)},
        {"box place 29", DevBoxCell(29)}, {"box Party row 0", DevBoxPartyRow(0)}, {"box CANCEL row", DevBoxPartyRow(6)},
        {"box previous", kDevBoxPrev}, {"box next", kDevBoxNext}, {"box PARTY", kDevBoxParty}, {"box CLOSE BOX", kDevBoxClose},
        {"box hand", kDevBoxHand}, {"box note", kDevBoxNote}, {"box narrow note", kDevBoxNoteNarrow},
        {"box chooser", kDevBoxChoose}, {"box chooser's CANCEL", kDevBoxCancel}, {"box menu key 7 of 7", DevBoxMenuKey(7, 6)}, {"box menu key 5 of 5", DevBoxMenuKey(5, 4)},
        {"Easy Chat phrase", kDevEcPhrase}, {"Easy Chat slot 9 of 9", DevEcChip(9, 8)}, {"Easy Chat CONFIRM", DevEcFooter(2)},
        {"Easy Chat group 20", DevEcCell(20)}, {"Easy Chat letter Z", DevEcLetter(25)}, {"Easy Chat BACK", DevEcToggle(1)},
        {"Easy Chat list title", kDevEcListTitle}, {"Easy Chat list", kDevEcList}, {"Easy Chat row place 11", DevEcRow(11)},
        {"Easy Chat rail", kDevEcRail}, {"Easy Chat up", kDevEcUp}, {"Easy Chat down", kDevEcDown},
        {"Easy Chat strip", kDevEcStrip}, {"Easy Chat strip's Z", DevEcStripCell(25)}, {"Easy Chat USE WORD", DevEcListKey(0)},
        {"Easy Chat list BACK", DevEcListKey(2)},
        {"FireRed's picture's screen", kDevViewScreen}, {"FireRed's picture's note across", kDevViewNoteWide},
        {"idle screen", kDevIdleScreen},
    };
    const DevGroup groups[] = {DEV_GROUP("HOME", homeKeys), DEV_GROUP("battle, a trainer", batTrainer),
                               DEV_GROUP("battle, wild", batWild), DEV_GROUP("battle moves", batMoves),
                               DEV_GROUP("Party", ptyBays), DEV_GROUP("Party, several", ptyMulti),
                               DEV_GROUP("Party level up", ptyOk), DEV_GROUP("Party menu", ptyMenu),
                               DEV_GROUP("Summary", sumKeys),
                               DEV_GROUP("Bag list", bagList), DEV_GROUP("a case's list", bagCase),
                               DEV_GROUP("Bag how many", bagQty), DEV_GROUP("Bag YES / NO", bagYesNo),
                               DEV_GROUP("OPTIONS", optKeys), DEV_GROUP("a question", askKeys),
                               DEV_GROUP("SAVE asking", saveAsk), DEV_GROUP("SAVE's result", saveResult),
                               DEV_GROUP("Trainer Card", cardKeys), DEV_GROUP("a letter", mailKeys),
                               DEV_GROUP("Pokedex menu", dexMenu), DEV_GROUP("Pokedex list", dexList),
                               DEV_GROUP("Pokedex habitat", dexCategory), DEV_GROUP("Pokedex page", dexPage),
                               DEV_GROUP("Pokedex page added", dexAdded), DEV_GROUP("the move to forget", learnKeys),
                               DEV_GROUP("relearner list", relearnList), DEV_GROUP("relearner YES / NO", relearnYesNo),
                               DEV_GROUP("PC box", boxKeys), DEV_GROUP("box YES / NO", boxYesNo),
                               DEV_GROUP("box chooser", boxChooser), DEV_GROUP("box marks", boxMarks),
                               DEV_GROUP("Easy Chat list", ecList), DEV_GROUP("Easy Chat question", ecAsk),
                               DEV_GROUP("FireRed's picture", frameKeys)};
    int faults = 0;

    RulesCatchFaults();
    PartyMenuGrid();
    for (int n = 2; n <= PT_BAG_MENU_MAX; ++n) {    /* the Bag's item menu of each size: its keys and the exit */
        for (int i = 0; i < n - 1; ++i)
            bagMenu[n][i] = DevBagMenuKey(n, i);
        bagMenu[n][n - 1] = kDevGridExit;
        bagMenus[n] = (DevGroup){kBagMenuName[n], bagMenu[n], n};
        faults += GroupFaults(&bagMenus[n]);
    }
    for (int c = 0; c < 3; ++c)             /* a choice of each size, in one column and as grids of 2 and 3, with BACK */
        for (int n = 1; n <= 8; ++n) {
            int cols = c ? c + 1 : 0;

            for (int i = 0; i < n; ++i)
                choiceKeys[c][n - 1][i] = DevChoiceKey(n, cols, i);
            choiceKeys[c][n - 1][n] = kDevGridExit;
            snprintf(choiceName[c][n - 1], sizeof(choiceName[c][n - 1]), "choice of %d (%d columns)", n, cols);
            choiceGroups[c][n - 1] = (DevGroup){choiceName[c][n - 1], choiceKeys[c][n - 1], n + 1};
            faults += GroupFaults(&choiceGroups[c][n - 1]);
        }
    for (int g = 0; g < 2; ++g) {           /* the naming keyboard: the five keys across and a page of 8 columns, or 6 */
        int cols = g ? 6 : 8, n = 0;

        for (int i = 0; i < 5; ++i)
            nameKeys[g][n++] = DevNameTop(i);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < cols; ++c)
                nameKeys[g][n++] = DevNameKey(cols, r, c);
        nameGroups[g] = (DevGroup){kNameGroup[g], nameKeys[g], n};
        faults += GroupFaults(&nameGroups[g]);
    }
    for (int n = 1; n <= 7; ++n) {          /* the box's menu of each size */
        for (int i = 0; i < n; ++i)
            boxMenu[n - 1][i] = DevBoxMenuKey(n, i);
        boxMenus[n - 1] = (DevGroup){kBoxMenuName[n - 1], boxMenu[n - 1], n};
        faults += GroupFaults(&boxMenus[n - 1]);
    }
    for (int n = 1; n <= 9; ++n) {          /* Easy Chat's phrase of each size: its slots, the mode keys, the buttons */
        int c = 0;

        for (int i = 0; i < n; ++i)
            ecEdit[n - 1][c++] = DevEcChip(n, i);
        for (int i = 0; i < 2; ++i)
            ecEdit[n - 1][c++] = DevEcMode(i);
        for (int i = 0; i < 3; ++i)
            ecEdit[n - 1][c++] = DevEcFooter(i);
        ecEdits[n - 1] = (DevGroup){kEcEditName[n - 1], ecEdit[n - 1], c};
        faults += GroupFaults(&ecEdits[n - 1]);
    }
    for (int g = 0; g < 2; ++g) {           /* Easy Chat's groups (21) and letters (26), with the toggle and BACK */
        int n = g ? 26 : 21, c = 0;

        for (int i = 0; i < n; ++i)
            ecGrid[g][c++] = g ? DevEcLetter(i) : DevEcCell(i);
        ecGrid[g][c++] = DevEcToggle(0);
        ecGrid[g][c++] = DevEcToggle(1);
        ecGrids[g] = (DevGroup){g ? "Easy Chat letters" : "Easy Chat groups", ecGrid[g], c};
        faults += GroupFaults(&ecGrids[g]);
    }
    for (unsigned i = 0; i < sizeof(rects) / sizeof(rects[0]); ++i)
        if (!Inside(rects[i].r)) {
            const CtrPokeTouchRect *r = &rects[i].r;

            fprintf(stderr, "%s {%d,%d,%d,%d} leaves the screen with its drop\n", rects[i].name, r->x, r->y, r->w, r->h);
            ++faults;
        }
    for (unsigned i = 0; i < sizeof(groups) / sizeof(groups[0]); ++i)
        faults += GroupFaults(&groups[i]);
    faults += HomeFaults();
    faults += BattleFaults();
    faults += PartyFaults();
    faults += GridFaults();
    faults += SummaryFaults();
    faults += BagFaults();
    faults += SharedFaults();
    faults += OptionsFaults();
    faults += ScreensFaults();
    faults += DexFaults();
    faults += ChoiceFaults();
    faults += LearnFaults();
    faults += NamingFaults();
    faults += BoxFaults();
    faults += EasyChatFaults();
    faults += FrameFaults();
    assert(faults == 0);
    puts("PASS device layout");
    return 0;
}
