/* The lower screen's layout table (3ds_lower_layout.h) keeps the rules of the design: everything on the 320x240
 * screen, touch sizes, dialogue frames on the 8 px grid, room for the map, and nothing on top of anything else in a
 * view. */
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

/* A window with the standard frame: at least three tiles each way (its corners and an edge between them); its sides
 * need not be whole tiles (the kit cuts the last edge tile short). */
static void Framed(const CtrPokeTouchRect *r)
{
    Inside(r);
    assert(r->w >= 24 && r->h >= 24);
}

/* A panel with the dialogue frame: whole tiles (its tiles flip and repeat by tile rows). */
static void Dialogue(const CtrPokeTouchRect *r)
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

/* r lies inside the window f, clear of its 8 px frame. */
static void InFrame(const CtrPokeTouchRect *r, const CtrPokeTouchRect *f)
{
    assert(r->x >= f->x + 8 && r->y >= f->y + 8 && r->x + r->w <= f->x + f->w - 8 && r->y + r->h <= f->y + f->h - 8);
}

/* r lies inside f (touch places may cover a window's frame). */
static void Within(const CtrPokeTouchRect *r, const CtrPokeTouchRect *f)
{
    assert(r->x >= f->x && r->y >= f->y && r->x + r->w <= f->x + f->w && r->y + r->h <= f->y + f->h);
}

static int Bottom(const CtrPokeTouchRect *r)
{
    return r->y + r->h;
}

static void home(void)
{
    const CtrPokeTouchRect *view[4], *slots[6];
    int n = 0;

    Framed(&kHomeMapPanel);
    assert(kHomeMapPanel.w - 16 >= 192 && kHomeMapPanel.h - 16 >= 128 + 16);
    Row(&kHomeCaption);
    assert(kHomeCaption.y >= kHomeMapPanel.y + 8 + 128 && kHomeCaption.x >= kHomeMapPanel.x + 8);
    assert(kHomeCaption.x + kHomeCaption.w <= kHomeMapPanel.x + kHomeMapPanel.w - 8);
    assert(kHomeCaption.y + kHomeCaption.h <= kHomeMapPanel.y + kHomeMapPanel.h - 8);
    /* The entries are rows of one menu window (FireRed's START menu), top to bottom in that order; RUN has left it. */
    Framed(&kHomeMenu);
    assert(kHomeMenu.y == kHomeMapPanel.y);
    assert(COUNT(kHomeEntry) == PT_OPTIONS - PT_POKEDEX + 1);
    for (int i = 0; i < COUNT(kHomeEntry); ++i) {
        Row(&kHomeEntry[i]);
        InFrame(&kHomeEntry[i], &kHomeMenu);
        assert(i == 0 || kHomeEntry[i].y >= kHomeEntry[i - 1].y + kHomeEntry[i - 1].h);
    }
    /* RUN and the game's speed (the owner's "F"): a window 4 px under the menu whose bottom is the map panel's, a line
     * each inside its frame, each line's touch place the half of the window it is in (RUN on top). */
    Framed(&kHomeSettings);
    assert(kHomeSettings.x == kHomeMenu.x && kHomeSettings.w == kHomeMenu.w);
    assert(kHomeSettings.y - Bottom(&kHomeMenu) == 4 && Bottom(&kHomeSettings) == Bottom(&kHomeMapPanel));
    InFrame(&kHomeRunLine, &kHomeSettings);
    InFrame(&kHomeSpeedLine, &kHomeSettings);
    assert(kHomeSpeedLine.y >= Bottom(&kHomeRunLine));
    Row(&kHomeRun);
    Row(&kHomeSpeed);
    Within(&kHomeRun, &kHomeSettings);
    Within(&kHomeSpeed, &kHomeSettings);
    Within(&kHomeRunLine, &kHomeRun);
    Within(&kHomeSpeedLine, &kHomeSpeed);
    assert(kHomeRun.y == kHomeSettings.y && kHomeSpeed.y == Bottom(&kHomeRun) && Bottom(&kHomeSpeed) == Bottom(&kHomeSettings));
    /* The party where the message was: a window 4 px under the map and 4 px over the screen's bottom, as wide as the
     * page; six touch places side by side inside its frame, each holding a 32 px icon over FireRed's HP bar, at the
     * battle's party bar's x. */
    Framed(&kHomeParty);
    assert(kHomeParty.x == kHomeMapPanel.x && Bottom(&kHomeParty) == 236 && kHomeParty.x + kHomeParty.w == 316);
    assert(kHomeParty.y - Bottom(&kHomeMapPanel) == 4);
    assert(COUNT(kHomePartySlot) == 6);
    for (int i = 0; i < 6; ++i) {
        Button(&kHomePartySlot[i]);
        InFrame(&kHomePartySlot[i], &kHomeParty);
        assert(kHomePartySlot[i].h >= 32 + 6 && kHomePartySlot[i].x + 8 == kBatParty.x + 48 * i + 8);
        slots[i] = &kHomePartySlot[i];
    }
    Apart(slots, 6);
    view[n++] = &kHomeMapPanel;
    view[n++] = &kHomeMenu;
    view[n++] = &kHomeSettings;
    view[n++] = &kHomeParty;
    Apart(view, n);
}

static void battle(void)
{
    const CtrPokeTouchRect *view[8];
    int n;

    /* The party: six slots, each a 32 px party icon with FireRed's HP bar under it. */
    Row(&kBatParty);
    assert(kBatParty.h >= 32 + 6 && kBatParty.w >= 6 * 40);
    /* The commands, the quick ball and its arrows, the Safari's four are framed cells. */
    for (int i = 0; i < 4; ++i) {
        Framed(&kBatAction[i]);
        Framed(&kBatActionBall[i]);
        Framed(&kBatSafari[i]);
    }
    Framed(&kBatBall);
    /* FIGHT (and the Safari's top row) sit close over the row under them, 4 px (owner), with more room above, under the
     * party. */
    for (int i = 0; i < 2; ++i) {
        const CtrPokeTouchRect *top = i ? &kBatSafari[0] : &kBatAction[0], *under = i ? &kBatSafari[2] : &kBatAction[1];

        assert(under->y - (top->y + top->h) == 4);
        assert(top->y - (kBatParty.y + kBatParty.h) > 4);
    }
    assert(kBatActionBall[0].y == kBatAction[0].y && kBatActionBall[0].h == kBatAction[0].h && kBatSafari[1].y == kBatSafari[0].y);
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
    /* The moves: the party's balls on top as on the commands, four cards a little apart (4 to 7 px, as the owner
     * asked), the note in the dialogue frame with three lines (ACC and STAB, two of description) and CANCEL under them,
     * lined up with the cards' outer edges. */
    n = 0;
    for (int i = 0; i < 4; ++i) {
        Button(&kBatMove[i]);
        view[n++] = &kBatMove[i];
    }
    {
        int across = kBatMove[1].x - (kBatMove[0].x + kBatMove[0].w), down = kBatMove[2].y - (kBatMove[0].y + kBatMove[0].h);

        assert(across >= 4 && across < 8 && down >= 4 && down < 8);
        assert(kBatMove[3].x - (kBatMove[2].x + kBatMove[2].w) == across && kBatMove[3].y - (kBatMove[1].y + kBatMove[1].h) == down);
        assert(kBatCancel.x - (kBatMoveNote.x + kBatMoveNote.w) == across);
    }
    assert(kBatMoveNote.x == kBatMove[0].x && kBatCancel.x + kBatCancel.w == kBatMove[1].x + kBatMove[1].w);
    Dialogue(&kBatMoveNote);
    assert(kBatMoveNote.h >= 56);
    Button(&kBatCancel);
    view[n++] = &kBatMoveNote;
    view[n++] = &kBatCancel;
    view[n++] = &kBatParty;
    Apart(view, n);
    n = 0;
    for (int i = 0; i < 4; ++i) {
        Button(&kBatTarget[i]);
        view[n++] = &kBatTarget[i];
    }
    view[n++] = &kBatMoveNote;
    view[n++] = &kBatCancel;
    view[n++] = &kBatParty;
    Apart(view, n);
}

int main(void)
{
    home();
    battle();
    printf("PASS lower layout%s\n", LOWER_LAYOUT_PROVISIONAL ? " (provisional values)" : "");
    return 0;
}
