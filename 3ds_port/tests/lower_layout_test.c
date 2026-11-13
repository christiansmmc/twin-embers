/* The lower screen's layout table (3ds_lower_layout.h) keeps the rules of the design: everything on the 320x240
 * screen, touch sizes, frames on the 8 px grid, room for the map, and nothing on top of anything else in a view. */
#include <assert.h>
#include <stdio.h>
#include "3ds_poketouch.h"
#include "3ds_lower_layout.h"

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

static void Inside(const CtrPokeTouchRect *r)
{
    assert(r->x >= 0 && r->y >= 0 && r->w > 0 && r->h > 0 && r->x + r->w <= 320 && r->y + r->h <= 240);
}

static void Button(const CtrPokeTouchRect *r)
{
    Inside(r);
    assert(r->w >= 24 && r->h >= 24);
}

static void Row(const CtrPokeTouchRect *r)
{
    Inside(r);
    assert(r->h >= 16);
}

static void Framed(const CtrPokeTouchRect *r)
{
    Inside(r);
    assert(r->w % 8 == 0 && r->h % 8 == 0);
}

static int Overlap(const CtrPokeTouchRect *a, const CtrPokeTouchRect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

/* No two rectangles of one view overlap. */
static void Apart(const CtrPokeTouchRect *const *rects, int n)
{
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            assert(!Overlap(rects[i], rects[j]));
}

static void home(void)
{
    const CtrPokeTouchRect *view[6 + 5];
    int n = 0;

    Framed(&kHomeMapPanel);
    assert(kHomeMapPanel.w - 16 >= 192 && kHomeMapPanel.h - 16 >= 128 + 16);
    Row(&kHomeCaption);
    assert(kHomeCaption.y >= kHomeMapPanel.y + 8 + 128 && kHomeCaption.x >= kHomeMapPanel.x + 8);
    assert(kHomeCaption.x + kHomeCaption.w <= kHomeMapPanel.x + kHomeMapPanel.w - 8);
    assert(kHomeCaption.y + kHomeCaption.h <= kHomeMapPanel.y + kHomeMapPanel.h - 8);
    Framed(&kHomeMessage);
    assert(COUNT(kHomeEntry) == PT_OPTIONS - PT_POKEDEX + 1);
    for (int i = 0; i < COUNT(kHomeEntry); ++i) {
        Button(&kHomeEntry[i]);
        view[n++] = &kHomeEntry[i];
    }
    Button(&kHomeRegistered);
    Button(&kHomeBike);
    Button(&kHomeRun);
    view[n++] = &kHomeRegistered;
    view[n++] = &kHomeBike;
    view[n++] = &kHomeRun;
    view[n++] = &kHomeMapPanel;
    view[n++] = &kHomeMessage;
    Apart(view, n);
}

static void battle(void)
{
    const CtrPokeTouchRect *view[8];
    int n;

    Row(&kBatParty);
    /* The commands, the quick ball and its arrows, the Safari's four are framed cells. */
    for (int i = 0; i < 4; ++i) {
        Framed(&kBatAction[i]);
        Framed(&kBatActionBall[i]);
        Framed(&kBatSafari[i]);
    }
    Framed(&kBatBall);
    for (int i = 0; i < 4; ++i) {
        Framed(&kBatMove[i]);
        Framed(&kBatTarget[i]);
    }
    Framed(&kBatCancel);
    Framed(&kBatBallArrow[0]);
    Framed(&kBatBallArrow[1]);
    n = 0;
    for (int i = 0; i < 4; ++i) {
        Button(&kBatAction[i]);
        view[n++] = &kBatAction[i];
    }
    view[n++] = &kBatParty;
    Apart(view, n);
    n = 0;
    for (int i = 0; i < 4; ++i) {
        Button(&kBatActionBall[i]);
        view[n++] = &kBatActionBall[i];
    }
    Button(&kBatBall);
    Button(&kBatBallArrow[0]);
    Button(&kBatBallArrow[1]);
    view[n++] = &kBatBall;
    view[n++] = &kBatBallArrow[0];
    view[n++] = &kBatBallArrow[1];
    view[n++] = &kBatParty;
    Apart(view, n);
    n = 0;
    for (int i = 0; i < 4; ++i) {
        Button(&kBatSafari[i]);
        view[n++] = &kBatSafari[i];
    }
    view[n++] = &kBatParty;
    Apart(view, n);
    n = 0;
    for (int i = 0; i < 4; ++i) {
        Button(&kBatMove[i]);
        view[n++] = &kBatMove[i];
    }
    Framed(&kBatMoveNote);
    Button(&kBatCancel);
    view[n++] = &kBatMoveNote;
    view[n++] = &kBatCancel;
    Apart(view, n);
    n = 0;
    for (int i = 0; i < 4; ++i) {
        Button(&kBatTarget[i]);
        view[n++] = &kBatTarget[i];
    }
    view[n++] = &kBatCancel;
    Apart(view, n);
}

int main(void)
{
    home();
    battle();
    printf("PASS lower layout%s\n", LOWER_LAYOUT_PROVISIONAL ? " (provisional values)" : "");
    return 0;
}
