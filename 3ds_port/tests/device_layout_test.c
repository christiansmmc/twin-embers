/* The red device's rectangles (3ds_device_layout.h), the one source of what the B views draw and what they hit.
 *
 * Rules: every rectangle the header holds lies inside the 320x240 screen with its 3 px drop; a key is at least 20 px
 * tall and 28 px wide (a paper button, HOME's RUN and SPD on the screen's paper, at least 40x16: the board's); the keys
 * of one view state never overlap, their drops included. Keys of different states may share space (the Bag's item menu
 * keys lie where its CLOSE is in the list), so each state is a group of its own. Each view adds its rectangles to
 * `rects` and one group per state to `groups` (in main: the header's rectangles are objects, not constants), and its own
 * checks (where a view's pieces must lie).
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

/* A paper button: drawn on a screen's paper, no drop (HOME's RUN and SPD). */
static int Paper(CtrPokeTouchRect r)
{
    return Same(r, kDevHomeRun) || Same(r, kDevHomeSpeed);
}

static int BigEnough(CtrPokeTouchRect r)
{
    return Paper(r) ? r.h >= 16 && r.w >= 40 : r.h >= 20 && r.w >= 28;
}

/* r (with its drop when `drop`) lies inside `outer`. */
static int Within(CtrPokeTouchRect r, CtrPokeTouchRect outer, int drop)
{
    CtrPokeTouchRect a = drop ? DevKeyArea(&r) : r;

    return a.x >= outer.x && a.y >= outer.y && a.x + a.w <= outer.x + outer.w && a.y + a.h <= outer.y + outer.h;
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
    assert(Paper(kDevHomeRun) && BigEnough(kDevHomeRun) && !BigEnough((CtrPokeTouchRect){210, 123, 44, 15}));
}

/* HOME (board B · Início): the six keys under the screen, the panel's pieces on its paper, the map's keys in the
 * window. */
static int HomeFaults(void)
{
    const CtrPokeTouchRect lower = {0, 150, 320, 90}, window = {PT_HOME_X, PT_HOME_Y, PT_HOME_W, PT_HOME_H};
    const CtrPokeTouchRect paper = {14, 14, 292, 128};
    int faults = 0;

    assert(Same(kDevHomeScreen, (CtrPokeTouchRect){8, 8, 304, 140}));
    assert(Same(kDevHomeKey[0], (CtrPokeTouchRect){8, 156, 96, 36}) && Same(kDevHomeKey[1], (CtrPokeTouchRect){112, 156, 96, 36})
           && Same(kDevHomeKey[2], (CtrPokeTouchRect){216, 156, 96, 36}) && Same(kDevHomeKey[3], (CtrPokeTouchRect){8, 198, 96, 34})
           && Same(kDevHomeKey[4], (CtrPokeTouchRect){112, 198, 96, 34}) && Same(kDevHomeKey[5], (CtrPokeTouchRect){216, 198, 96, 34}));
    assert(Same(kDevHomeRun, (CtrPokeTouchRect){210, 123, 44, 16}) && Same(kDevHomeSpeed, (CtrPokeTouchRect){258, 123, 44, 16}));
    for (int i = 0; i < 6; ++i)
        assert(Same(DevHomeCell(i), (CtrPokeTouchRect){(int16_t)(210 + 31 * (i % 3)), (int16_t)(62 + 28 * (i / 3)), 30, 28}));
    assert(kDevHomeName.x == 212 && kDevHomeName.y == 17 && kDevHomeName.w == 90 && kDevHomeName.h == 2 * 13);
    assert(kDevHomeRegion.x == 212 && kDevHomeRegion.y == 44 && Same(kDevHomeRule, (CtrPokeTouchRect){210, 58, 92, 1}));
    assert(kDevHomeMapKey.x == 16 && kDevHomeMapKey.y == 16 && kDevHomeMapKey.h == 14
           && Same(kDevHomeFly, (CtrPokeTouchRect){158, 16, 44, 16}));        /* moved off Cinnabar Island */
    assert(PT_HOME_X == 14 && PT_HOME_Y == 14 && PT_HOME_W == 192 && PT_HOME_H == 128 && PT_HOME_SRC_X == 24 && PT_HOME_SRC_Y == 28);
    for (int i = 0; i < 6; ++i)
        if (!Within(kDevHomeKey[i], lower, 1)) {
            fprintf(stderr, "HOME: key %d leaves the lower part {0,150,320,90}\n", i);
            ++faults;
        }
    if (!Within(kDevHomeMapKey, window, 1) || !Within(kDevHomeFly, window, 1)) {   /* with their drops */
        fprintf(stderr, "HOME: the region key or FLY leaves the map's window\n");
        ++faults;
    }
    if (!Within(window, paper, 0) || !Within(kDevHomeRun, paper, 0) || !Within(kDevHomeSpeed, paper, 0)
        || !Within(kDevHomeName, paper, 0) || !Within(kDevHomeRegion, paper, 0) || !Within(kDevHomeRule, paper, 0)) {
        fprintf(stderr, "HOME: a piece of the panel leaves the screen's paper\n");
        ++faults;
    }
    for (int i = 0; i < 6; ++i)
        if (!Within(DevHomeCell(i), paper, 0) || DevHomeCell(i).x < window.x + window.w) {
            fprintf(stderr, "HOME: the party's cell %d leaves the panel\n", i);
            ++faults;
        }
    return faults;
}

/* The battle (boards B · Comandos and B · Golpes): the brief's numbers; each screen with its light row above the keys
 * under it; the quick ball's arrows on its key's face. */
static int BattleFaults(void)
{
    int faults = 0;

    assert(Same(kDevBatScreen, (CtrPokeTouchRect){8, 8, 304, 88}) && Same(kDevBatMoveScreen, (CtrPokeTouchRect){8, 8, 304, 56}));
    assert(Same(kDevBatFight, (CtrPokeTouchRect){8, 104, 304, 56}) && Same(kDevBatCancel, (CtrPokeTouchRect){236, 212, 76, 20}));
    for (int i = 0; i < 3; ++i)
        assert(Same(kDevBatRow3[i], (CtrPokeTouchRect){(int16_t)(8 + 104 * i), 170, 96, 60}));
    for (int i = 0; i < 4; ++i) {
        assert(Same(kDevBatRow4[i], (CtrPokeTouchRect){(int16_t)(8 + 77 * i), 170, 73, 60}));
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

int main(void)
{
    /* Every rectangle the header holds, by name, and the keys of each view state. */
    const CtrPokeTouchRect homeKeys[] = {kDevHomeKey[0], kDevHomeKey[1], kDevHomeKey[2], kDevHomeKey[3], kDevHomeKey[4],
                                         kDevHomeKey[5], kDevHomeRun, kDevHomeSpeed};
    const CtrPokeTouchRect batTrainer[] = {kDevBatFight, kDevBatRow3[0], kDevBatRow3[1], kDevBatRow3[2]};
    const CtrPokeTouchRect batWild[] = {kDevBatFight, kDevBatRow4[0], kDevBatRow4[1], kDevBatRow4[2], kDevBatRow4[3]};
    const CtrPokeTouchRect batMoves[] = {kDevBatMove[0], kDevBatMove[1], kDevBatMove[2], kDevBatMove[3], kDevBatCancel};
    const DevNamedRect rects[] = {
        {"HOME screen", kDevHomeScreen}, {"HOME POKeDEX", kDevHomeKey[0]}, {"HOME POKeMON", kDevHomeKey[1]},
        {"HOME BAG", kDevHomeKey[2]}, {"HOME card", kDevHomeKey[3]}, {"HOME SAVE", kDevHomeKey[4]},
        {"HOME OPTIONS", kDevHomeKey[5]}, {"HOME RUN", kDevHomeRun}, {"HOME SPD", kDevHomeSpeed},
        {"HOME name", kDevHomeName}, {"HOME region line", kDevHomeRegion}, {"HOME rule", kDevHomeRule},
        {"HOME region key", kDevHomeMapKey}, {"HOME FLY", kDevHomeFly},
        {"HOME cell 0", DevHomeCell(0)}, {"HOME cell 5", DevHomeCell(5)},
        {"battle screen", kDevBatScreen}, {"battle moves' screen", kDevBatMoveScreen}, {"battle FIGHT", kDevBatFight},
        {"battle BAG (3)", kDevBatRow3[0]}, {"battle POKeMON (3)", kDevBatRow3[1]}, {"battle RUN (3)", kDevBatRow3[2]},
        {"battle BAG (4)", kDevBatRow4[0]}, {"battle BALL", kDevBatRow4[1]}, {"battle POKeMON (4)", kDevBatRow4[2]},
        {"battle RUN (4)", kDevBatRow4[3]}, {"battle move 0", kDevBatMove[0]}, {"battle move 1", kDevBatMove[1]},
        {"battle move 2", kDevBatMove[2]}, {"battle move 3", kDevBatMove[3]}, {"battle CANCEL", kDevBatCancel},
    };
    const DevGroup groups[] = {DEV_GROUP("HOME", homeKeys), DEV_GROUP("battle, a trainer", batTrainer),
                               DEV_GROUP("battle, wild", batWild), DEV_GROUP("battle moves", batMoves)};
    int faults = 0;

    RulesCatchFaults();
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
    assert(faults == 0);
    puts("PASS device layout");
    return 0;
}
