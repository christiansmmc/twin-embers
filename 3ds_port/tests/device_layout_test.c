/* The red device's rectangles (3ds_device_layout.h), the one source of what the B views draw and what they hit.
 *
 * Rules: every rectangle the header holds lies inside the 320x240 screen with its 3 px drop; a key is at least 20 px
 * tall and 28 px wide; the keys of one view state never overlap, their drops included. Keys of different states may
 * share space (the Bag's item menu keys lie where its CLOSE is in the list), so each state is a group of its own.
 * Each view adds its rectangles to kRects and one group per state to kGroups, built from the header.
 *
 * usage: device_layout_test (prints PASS device layout) */
#include <assert.h>
#include <stdio.h>
#include "3ds_device_layout.h"

typedef struct { const char *name; CtrPokeTouchRect r; } DevNamedRect;
typedef struct { const char *state; const CtrPokeTouchRect *keys; int count; } DevGroup;
#define DEV_GROUP(state, keys) {state, keys, (int)(sizeof(keys) / sizeof(keys[0]))}

/* Every rectangle the header holds, by name (the list ends with a NULL name). */
static const DevNamedRect kRects[] = {
    {NULL, {0, 0, 0, 0}},
};

/* The keys of each view state (the list ends with a NULL state). */
static const DevGroup kGroups[] = {
    {NULL, NULL, 0},
};

static int Inside(CtrPokeTouchRect r)
{
    CtrPokeTouchRect a = DevKeyArea(&r);

    return a.x >= 0 && a.y >= 0 && a.w > 0 && a.h > 0 && a.x + a.w <= 320 && a.y + a.h <= 240;
}

static int BigEnough(CtrPokeTouchRect r)
{
    return r.h >= 20 && r.w >= 28;
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

int main(void)
{
    int faults = 0;

    RulesCatchFaults();
    for (const DevNamedRect *n = kRects; n->name; ++n)
        if (!Inside(n->r)) {
            fprintf(stderr, "%s {%d,%d,%d,%d} leaves the screen with its drop\n", n->name, n->r.x, n->r.y, n->r.w, n->r.h);
            ++faults;
        }
    for (const DevGroup *g = kGroups; g->state; ++g)
        faults += GroupFaults(g);
    assert(faults == 0);
    puts("PASS device layout");
    return 0;
}
