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
     * asked), the note in the dialogue frame with three lines (the description, STAB at the first one's right) and
     * CANCEL under them, lined up with the cards' outer edges. */
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

/* POKeMON: the six cards 4 px apart in two columns of three, the message band 5 px under them (dialogue frame, two
 * lines), CANCEL beside it ending with the cards' right edge; CONFIRM between the narrower message and CANCEL; the
 * menu's window over CANCEL with the message narrower beside it, its rows inside it; the level-up window over the right
 * column's first two cards. */
static void party(void)
{
    const CtrPokeTouchRect *view[10];
    int n = 0;

    for (int i = 0; i < 6; ++i) {
        Framed(&kPartySlot[i]);
        assert(kPartySlot[i].w == kPartySlot[0].w && kPartySlot[i].h == kPartySlot[0].h);
        assert(kPartySlot[i].h - 16 >= 32 + 8);                 /* the icon and its hop inside the frame */
        view[n++] = &kPartySlot[i];
    }
    assert(kPartySlot[1].x - (kPartySlot[0].x + kPartySlot[0].w) == 4 && kPartySlot[2].y - Bottom(&kPartySlot[0]) == 4);
    assert(kPartySlot[0].x == 4 && kPartySlot[1].x + kPartySlot[1].w == 316 && kPartySlot[0].y == 4);
    for (int i = 2; i < 6; ++i)
        assert(kPartySlot[i].x == kPartySlot[i & 1].x && kPartySlot[i].y == kPartySlot[i - 2].y + kPartySlot[0].h + 4);
    Dialogue(&kPartyNote);
    assert(kPartyNote.h - 16 >= 2 * 14 && kPartyNote.y >= Bottom(&kPartySlot[5]) + 4 && Bottom(&kPartyNote) <= 236);
    Button(&kPartyCancel);
    assert(kPartyCancel.x == kPartyNote.x + kPartyNote.w + 4 && kPartyCancel.x + kPartyCancel.w == 316);
    assert(kPartyCancel.y == kPartyNote.y && kPartyCancel.h == kPartyNote.h);
    view[n++] = &kPartyNote;
    view[n++] = &kPartyCancel;
    Apart(view, n);
    /* picking several */
    Dialogue(&kPartyNoteMulti);
    Button(&kPartyConfirm);
    assert(kPartyConfirm.x == kPartyNoteMulti.x + kPartyNoteMulti.w + 4 && kPartyCancel.x == kPartyConfirm.x + kPartyConfirm.w + 4);
    assert(kPartyConfirm.y == kPartyNote.y && kPartyConfirm.h == kPartyNote.h);
    n = 6;
    view[n++] = &kPartyNoteMulti;
    view[n++] = &kPartyConfirm;
    view[n++] = &kPartyCancel;
    Apart(view, n);
    /* a menu: every size from one row to the most, where CANCEL is (not drawn while a menu shows), the message
     * beside it */
    Dialogue(&kPartyNoteMenu);
    for (int rows = 1; rows <= PARTY_MENU_ROWS; ++rows) {
        CtrPokeTouchRect w = PartyMenuWindow(rows);

        Framed(&w);
        assert(w.x == kPartyNoteMenu.x + kPartyNoteMenu.w + 4 && w.x + w.w == 316 && Bottom(&w) == Bottom(&kPartyNote));
        assert(w.y >= 4);
        for (int i = 0; i < rows; ++i) {
            CtrPokeTouchRect r = PartyMenuRow(rows, i);

            Row(&r);
            InFrame(&r, &w);
        }
    }
    Dialogue(&kPartyNoteWide);
    assert(kPartyNoteWide.x == 4 && kPartyNoteWide.x + kPartyNoteWide.w == 316 && kPartyNoteWide.y == kPartyNote.y);
    /* the level-up window: the right column's first two cards exactly */
    Framed(&kPartyLevelUp);
    assert(kPartyLevelUp.x == kPartySlot[1].x && kPartyLevelUp.y == kPartySlot[1].y && kPartyLevelUp.w == kPartySlot[1].w);
    assert(Bottom(&kPartyLevelUp) == Bottom(&kPartySlot[3]) && kPartyLevelUp.h - 16 >= 6 * 16);
}

/* The Summary: the title and its two arrows across the top, the Pokemon and the page under them 4 px apart, the
 * bottom row (previous, next, the band, BACK) 4 px under those, all 4 px apart and within the screen's 4 px margin;
 * the moves page's rows and the details' rows inside the page's frame, the details leaving room for three more lines. */
static void summary(void)
{
    const CtrPokeTouchRect *view[10] = {&kSumTitle, &kSumPagePrev, &kSumPageNext, &kSumMon, &kSumPage,
                                        &kSumMonPrev, &kSumMonNext, &kSumNote, &kSumBack};
    int n = 9;

    Framed(&kSumTitle);
    assert(kSumTitle.h - 16 >= 14);
    Button(&kSumPagePrev);
    Button(&kSumPageNext);
    Framed(&kSumMon);
    Framed(&kSumPage);
    Button(&kSumMonPrev);
    Button(&kSumMonNext);
    Dialogue(&kSumNote);
    assert(kSumNote.h - 16 >= 2 * 14);
    Button(&kSumBack);
    Apart(view, n);
    assert(kSumPagePrev.x == kSumTitle.x + kSumTitle.w + 4 && kSumPageNext.x == kSumPagePrev.x + kSumPagePrev.w + 4);
    assert(kSumPageNext.x + kSumPageNext.w == 316 && kSumTitle.x == 4 && kSumTitle.y == 4);
    assert(kSumMon.y == Bottom(&kSumTitle) + 4 && kSumPage.y == kSumMon.y && kSumPage.x == kSumMon.x + kSumMon.w + 4);
    assert(kSumPage.x + kSumPage.w == 316 && Bottom(&kSumPage) == Bottom(&kSumMon));
    assert(kSumMon.w - 16 >= 80 && kSumMon.h - 16 >= 2 * 14 + 64 + 14);   /* name, Lv, the picture, the types */
    assert(kSumMonPrev.y == Bottom(&kSumMon) + 4 && Bottom(&kSumBack) == 236);
    assert(kSumMonNext.x == kSumMonPrev.x + kSumMonPrev.w + 4 && kSumNote.x == kSumMonNext.x + kSumMonNext.w + 4);
    assert(kSumBack.x == kSumNote.x + kSumNote.w + 4 && kSumBack.x + kSumBack.w == 316);
    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect c = SumMoveCard(i);

        Row(&c);
        assert(c.h >= 2 * 14);
        InFrame(&c, &kSumPage);
    }
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = SumMoveRow(i);

        Row(&r);
        InFrame(&r, &kSumPage);
    }
    {
        CtrPokeTouchRect last = SumMoveRow(4);

        assert(kSumPage.y + kSumPage.h - 8 - Bottom(&last) >= 4 + 14 + 2 * 14);   /* a rule, the type, power, accuracy */
    }
}

/* The BAG: three tabs across (or the title), the list and its two scroll cells 4 px under them, the band and CANCEL
 * 4 px under those; the rows inside the list's frame, each tall enough for an item icon; the quantity box's five in the
 * bottom right corner, apart, over where the menu goes; the band's widths for a menu (beside the Party's window) and
 * a message. */
static void bag(void)
{
    const CtrPokeTouchRect *view[10];
    int n = 0;

    for (int i = 0; i < 3; ++i) {
        Button(&kBagTab[i]);
        assert(kBagTab[i].y == 4 && kBagTab[i].h - 16 >= 14);
        if (i)
            assert(kBagTab[i].x == kBagTab[i - 1].x + kBagTab[i - 1].w + 4);
        view[n++] = &kBagTab[i];
    }
    assert(kBagTab[0].x == 4 && kBagTab[2].x + kBagTab[2].w == 316);
    Framed(&kBagTitle);
    assert(kBagTitle.x == 4 && kBagTitle.x + kBagTitle.w == 316 && kBagTitle.h == kBagTab[0].h);
    Framed(&kBagList);
    Button(&kBagUp);
    Button(&kBagDown);
    assert(kBagList.y == Bottom(&kBagTab[0]) + 4 && kBagUp.x == kBagList.x + kBagList.w + 4 && kBagUp.x + kBagUp.w == 316);
    assert(kBagUp.y == kBagList.y && kBagDown.y == Bottom(&kBagUp) + 4 && Bottom(&kBagDown) == Bottom(&kBagList));
    view[n++] = &kBagList;
    view[n++] = &kBagUp;
    view[n++] = &kBagDown;
    Dialogue(&kBagNote);
    assert(kBagNote.h >= 56 && kBagNote.y == Bottom(&kBagList) + 4 && Bottom(&kBagNote) == 236);  /* three lines, as the moves' note */
    Button(&kBagCancel);
    assert(kBagCancel.x == kBagNote.x + kBagNote.w + 4 && kBagCancel.x + kBagCancel.w == 316 && kBagCancel.y == kBagNote.y);
    view[n++] = &kBagNote;
    view[n++] = &kBagCancel;
    Apart(view, n);
    for (int i = 0; i < BAG_PAGE_ROWS; ++i) {
        CtrPokeTouchRect r = BagRow(i);

        Row(&r);
        InFrame(&r, &kBagList);
        assert(r.h >= 24 + 4);                          /* an item's 24x24 icon */
    }
    /* a menu: the band beside the Party's window */
    Dialogue(&kBagNoteMenu);
    assert(kBagNoteMenu.x + kBagNoteMenu.w + 4 == PARTY_MENU_X && kBagNoteMenu.y == kBagNote.y);
    Dialogue(&kBagNoteWide);
    assert(kBagNoteWide.x == 4 && kBagNoteWide.x + kBagNoteWide.w == 316 && kBagNoteWide.y == kBagNote.y);
    /* the quantity box */
    {
        const CtrPokeTouchRect *qty[6] = {&kBagQtyCount, &kBagQtyUp, &kBagQtyDown, &kBagQtyOk, &kBagQtyCancel, &kBagNoteMenu};

        Framed(&kBagQtyCount);
        assert(kBagQtyCount.h - 16 >= 14);
        for (int i = 1; i < 5; ++i) {
            Button(qty[i]);
            assert(qty[i]->x >= PARTY_MENU_X && qty[i]->x + qty[i]->w <= 316);
        }
        assert(kBagQtyOk.y == kBagNote.y && Bottom(&kBagQtyOk) == 236);
        Apart(qty, 6);
    }
}

/* The bottom row every field page shares: the band (dialogue frame, three lines) 4 px under what is above it, BACK
 * beside it at the bottom right, both on the screen's 4 px margin. */
static void BandAndBack(const CtrPokeTouchRect *band, const CtrPokeTouchRect *back, int above)
{
    Dialogue(band);
    assert(band->h >= 56 && Bottom(band) == 236 && band->y == above + 4);
    Button(back);
    assert(back->x == band->x + band->w + 4 && back->x + back->w == 316 && back->y == band->y && back->h == band->h);
}

/* The POKeDEX: the BAG's tabs over the list window, its scroll cells and the side window, 4 px apart; six menu rows or
 * five list rows inside the list's frame; a habitat page's window with its four places, the page arrows and the
 * narrower band under it; a Pokemon's page: the picture (80x80 whole) and the data side by side, the entry under them
 * (three lines), AREA, CRY and BACK under that. */
static void dex(void)
{
    const CtrPokeTouchRect *view[12];
    int n = 0;

    for (int i = 0; i < 3; ++i) {
        assert(kDexTab[i].x == kBagTab[i].x && kDexTab[i].y == kBagTab[i].y && kDexTab[i].w == kBagTab[i].w);
        Button(&kDexTab[i]);
        view[n++] = &kDexTab[i];
    }
    Framed(&kDexTitle);
    assert(kDexTitle.x == 4 && kDexTitle.x + kDexTitle.w == 316 && kDexTitle.h == kDexTab[0].h);
    Framed(&kDexList);
    Button(&kDexUp);
    Button(&kDexDown);
    Framed(&kDexSide);
    assert(kDexList.x == 4 && kDexList.y == Bottom(&kDexTab[0]) + 4 && kDexUp.x == kDexList.x + kDexList.w + 4);
    assert(kDexUp.y == kDexList.y && kDexDown.y == Bottom(&kDexUp) + 4 && Bottom(&kDexDown) == Bottom(&kDexList));
    assert(kDexSide.x == kDexUp.x + kDexUp.w + 4 && kDexSide.x + kDexSide.w == 316 && kDexSide.y == kDexList.y);
    assert(Bottom(&kDexSide) == Bottom(&kDexList) && kDexSide.w - 16 >= 80 && kDexSide.h - 16 >= 80 + 2 * 16);
    view[n++] = &kDexList;
    view[n++] = &kDexUp;
    view[n++] = &kDexDown;
    view[n++] = &kDexSide;
    BandAndBack(&kDexNote, &kDexBack, Bottom(&kDexList));
    view[n++] = &kDexNote;
    view[n++] = &kDexBack;
    Apart(view, n);
    for (int i = 0; i < DEX_MENU_ROWS; ++i) {
        CtrPokeTouchRect r = DexMenuRow(i);

        Row(&r);
        InFrame(&r, &kDexList);
    }
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = DexListRow(i);

        Row(&r);
        assert(r.h >= 24);
        InFrame(&r, &kDexList);
    }
    /* a habitat page */
    Framed(&kDexHabitat);
    assert(kDexHabitat.x == 4 && kDexHabitat.y == 4 && kDexHabitat.x + kDexHabitat.w == 316);
    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect r = DexSlot(i);

        Button(&r);
        InFrame(&r, &kDexHabitat);
        assert(r.h >= 64 + 8 && r.w >= 64 + 8 + 64);       /* the picture and the number and name beside it */
    }
    Button(&kDexPrev);
    Button(&kDexNext);
    Dialogue(&kDexNoteNarrow);
    assert(kDexPrev.x == 4 && kDexNext.x == kDexPrev.x + kDexPrev.w + 4 && kDexNoteNarrow.x == kDexNext.x + kDexNext.w + 4);
    assert(kDexPrev.y == kDexNote.y && kDexNext.y == kDexNote.y && kDexNoteNarrow.y == kDexNote.y);
    assert(kDexNoteNarrow.x + kDexNoteNarrow.w + 4 == kDexBack.x && kDexPrev.y == Bottom(&kDexHabitat) + 4);
    /* a Pokemon's page */
    Framed(&kDexPic);
    Framed(&kDexInfo);
    assert(kDexPic.w - 16 >= 80 && kDexPic.h - 16 >= 80);
    assert(kDexInfo.x == kDexPic.x + kDexPic.w + 4 && kDexInfo.x + kDexInfo.w == 316 && kDexInfo.h == kDexPic.h);
    Dialogue(&kDexEntry);
    assert(kDexEntry.x == 4 && kDexEntry.w == 312 && kDexEntry.y == Bottom(&kDexPic) + 4 && kDexEntry.h >= 56);
    Button(&kDexArea);
    Button(&kDexCry);
    assert(kDexArea.x == 4 && kDexArea.y == Bottom(&kDexEntry) + 4 && kDexCry.x == kDexArea.x + kDexArea.w + 4);
    assert(kDexCry.x + kDexCry.w + 4 == kDexBack.x && kDexArea.y == kDexBack.y && Bottom(&kDexArea) == 236);
    Dialogue(&kDexNoteWide);
    assert(kDexNoteWide.x == 4 && kDexNoteWide.x + kDexNoteWide.w == 316 && kDexNoteWide.y == kDexNote.y);
}

/* OPTIONS: the title and PAGE across the top, the list 4 px under them with a page's rows inside its frame (each with
 * room for its label and the value box, whose arrows' touch places are 24 px wide at least), the band and BACK; FRAME's
 * sample inside the band's text area, clear of its frame. SAVE: the two windows side by side, the band beside the
 * Party's two-row window, or whole. */
static void options(void)
{
    const CtrPokeTouchRect *view[6] = {&kOptTitle, &kOptPage, &kOptList, &kOptNote, &kOptBack};

    Framed(&kOptTitle);
    Button(&kOptPage);
    assert(kOptTitle.x == 4 && kOptTitle.y == 4 && kOptPage.x == kOptTitle.x + kOptTitle.w + 4 && kOptPage.x + kOptPage.w == 316);
    assert(kOptPage.h == kOptTitle.h && kOptTitle.h - 16 >= 14);
    Framed(&kOptList);
    assert(kOptList.x == 4 && kOptList.x + kOptList.w == 316 && kOptList.y == Bottom(&kOptTitle) + 4);
    BandAndBack(&kOptNote, &kOptBack, Bottom(&kOptList));
    Apart(view, 5);
    for (int i = 0; i < PT_OPT_MAX; ++i) {
        CtrPokeTouchRect r = OptRowRect(i);

        Row(&r);
        InFrame(&r, &kOptList);
        assert(r.w - OPT_BOX_W >= 8 + 80 && OPT_ARROW_W >= 24 && OPT_BOX_W - OPT_ARROW_W >= 24);
    }
    Within(&kOptSample, &kOptNote);
    assert(kOptSample.y >= kOptNote.y + 8 && Bottom(&kOptSample) <= Bottom(&kOptNote) - 8);
    assert(kOptSample.x + kOptSample.w <= kOptNote.x + kOptNote.w - 16 && kOptSample.w >= 24 && kOptSample.h >= 24);
    {
        const CtrPokeTouchRect *save[4] = {&kSaveInfo, &kSavePlayer, &kSaveNote, NULL};
        CtrPokeTouchRect yesNo = PartyMenuWindow(2);

        Framed(&kSaveInfo);
        Framed(&kSavePlayer);
        assert(kSaveInfo.x == 4 && kSaveInfo.y == 4 && kSavePlayer.x == kSaveInfo.x + kSaveInfo.w + 4);
        assert(kSavePlayer.x + kSavePlayer.w == 316 && kSavePlayer.h == kSaveInfo.h && kSaveInfo.h - 16 >= 5 * 14);
        assert(kSavePlayer.w - 16 >= 64 && kSavePlayer.h - 16 >= 64);           /* the map icon at 4x */
        Dialogue(&kSaveNote);
        assert(kSaveNote.x == 4 && kSaveNote.y == Bottom(&kSaveInfo) + 4 && Bottom(&kSaveNote) == 236);
        assert(kSaveNote.x + kSaveNote.w + 4 == yesNo.x && Bottom(&yesNo) == 236);
        save[3] = &yesNo;
        Apart(save, 4);
        Dialogue(&kSaveNoteWide);
        assert(kSaveNoteWide.x == 4 && kSaveNoteWide.x + kSaveNoteWide.w == 316 && kSaveNoteWide.y == kSaveNote.y);
    }
}

/* The Trainer Card: one window across the page, the band and BACK under it; inside its frame, the eight badges at 2x
 * in a row and the trainer's 64x64 picture beside four lines. */
static void card(void)
{
    const CtrPokeTouchRect *view[3] = {&kCard, &kCardNote, &kCardBack};

    Framed(&kCard);
    assert(kCard.x == 4 && kCard.y == 4 && kCard.x + kCard.w == 316);
    BandAndBack(&kCardNote, &kCardBack, Bottom(&kCard));
    Apart(view, 3);
    assert(8 * CARD_BADGE + 7 * CARD_BADGE_GAP <= kCard.w - 16 && CARD_BADGE == 2 * 16);
    assert(kCard.h - 16 >= 16 + 4 * 18 + 8 + CARD_BADGE && kCard.w - 16 >= 200 + 64);
}

/* The PC box: the wallpaper (160x144) whole inside the box window's frame, its header's three places and the thirty
 * places inside it at 24 px; PARTY, CLOSE BOX and the picture's window beside it, 4 px apart; the band and BACK; the
 * Party's panel and the chooser inside the box window's frame; the menu's window beside the narrower band. */
static void box(void)
{
    const CtrPokeTouchRect *view[8] = {&kBoxWin, &kBoxPartyButton, &kBoxClose, &kBoxMon, &kBoxNote, &kBoxBack};
    CtrPokeTouchRect wall = {BOX_WALL_X, BOX_WALL_Y, 160, 144}, body = {BOX_WALL_X, BOX_WALL_Y + 24, 160, 120};

    Framed(&kBoxWin);
    assert(kBoxWin.x == 4 && kBoxWin.y == 4);
    InFrame(&wall, &kBoxWin);
    Button(&kBoxPrev);
    Button(&kBoxTitle);
    Button(&kBoxNext);
    assert(kBoxPrev.y == wall.y && kBoxPrev.h == 24 && kBoxTitle.x == kBoxPrev.x + kBoxPrev.w && kBoxNext.x == kBoxTitle.x + kBoxTitle.w);
    assert(kBoxPrev.x == wall.x && kBoxNext.x + kBoxNext.w == wall.x + wall.w);
    for (int i = 0; i < 30; ++i) {
        CtrPokeTouchRect c = BoxCell(i);

        Button(&c);
        Within(&c, &body);
    }
    Button(&kBoxPartyButton);
    Button(&kBoxClose);
    Framed(&kBoxMon);
    assert(kBoxPartyButton.x == kBoxWin.x + kBoxWin.w + 4 && kBoxPartyButton.x + kBoxPartyButton.w == 316 && kBoxPartyButton.y == 4);
    assert(kBoxClose.x == kBoxPartyButton.x && kBoxClose.y == Bottom(&kBoxPartyButton) + 4 && kBoxMon.y == Bottom(&kBoxClose) + 4);
    assert(kBoxMon.x == kBoxClose.x && kBoxMon.w == kBoxClose.w && Bottom(&kBoxMon) == Bottom(&kBoxWin) && kBoxMon.h - 16 >= 64);
    BandAndBack(&kBoxNote, &kBoxBack, Bottom(&kBoxWin));
    Apart(view, 6);
    Dialogue(&kBoxNoteMenu);
    assert(kBoxNoteMenu.x + kBoxNoteMenu.w + 4 == PARTY_MENU_X && kBoxNoteMenu.y == kBoxNote.y && PT_BOX_MENU_MAX <= PARTY_MENU_ROWS);
    Dialogue(&kBoxNoteWide);
    assert(kBoxNoteWide.x == 4 && kBoxNoteWide.x + kBoxNoteWide.w == 316 && kBoxNoteWide.y == kBoxNote.y);
    /* the Party's panel */
    {
        const CtrPokeTouchRect *panel[7];
        CtrPokeTouchRect rows[5];

        Button(&kBoxLead);
        InFrame(&kBoxLead, &kBoxWin);
        panel[0] = &kBoxLead;
        for (int i = 1; i < 6; ++i) {
            rows[i - 1] = BoxPartyRow(i);
            Button(&rows[i - 1]);
            InFrame(&rows[i - 1], &kBoxWin);
            panel[i] = &rows[i - 1];
        }
        Button(&kBoxPartyCancel);
        InFrame(&kBoxPartyCancel, &kBoxWin);
        panel[6] = &kBoxPartyCancel;
        Apart(panel, 7);
    }
    /* the chooser */
    {
        const CtrPokeTouchRect *choose[3] = {&kBoxChoosePrev, &kBoxChooseName, &kBoxChooseNext};

        Button(&kBoxChoosePrev);
        Framed(&kBoxChooseName);
        Button(&kBoxChooseNext);
        for (int i = 0; i < 3; ++i)
            InFrame(choose[i], &kBoxWin);
        assert(kBoxChooseName.h - 16 >= 2 * 14 && kBoxChooseName.x == kBoxChoosePrev.x + kBoxChoosePrev.w + 4);
        assert(kBoxChooseNext.x == kBoxChooseName.x + kBoxChooseName.w + 4);
        Apart(choose, 3);
    }
}

/* A choice of the field: for every size and width, the window framed on the screen, its rows inside its frame; one
 * column: the Party's window corner (its bottom and right), the band beside it on whole tiles; a grid: across the
 * page, 4 px over the whole band. */
static void choice(void)
{
    static const int widths[5] = {0, 100, 130, 180, 400};

    for (int cols = 0; cols <= 3; ++cols)
        for (int count = 1; count <= PT_CHOICE_MAX; ++count)
            for (int k = 0; k < 5; ++k) {
                CtrPokeTouchRect w = ChoiceWindow(count, cols, widths[k]), n = ChoiceNote(count, cols, widths[k]);
                const CtrPokeTouchRect *view[2] = {&w, &n};

                Framed(&w);
                Dialogue(&n);
                assert(n.y == 180 && n.h == 56 && n.x == 4);
                Apart(view, 2);
                for (int i = 0; i < count; ++i) {
                    CtrPokeTouchRect r = ChoiceRow(count, cols, widths[k], i);

                    Row(&r);
                    InFrame(&r, &w);
                }
                if (cols > 1) {
                    assert(w.x == 4 && w.x + w.w == 316 && Bottom(&w) + 4 == n.y && n.w == 312);
                } else {
                    assert(w.x + w.w == 316 && Bottom(&w) == 236 && n.x + n.w + 4 == w.x);
                    assert(w.w >= CHOICE_MIN_W && w.w <= CHOICE_MAX_W && (w.w >= widths[k] || w.w == CHOICE_MAX_W));
                }
            }
    {
        CtrPokeTouchRect party = PartyMenuWindow(3), w = ChoiceWindow(3, 1, 0);

        assert(party.x == w.x && party.y == w.y && party.w == w.w && party.h == w.h);   /* the Party's window itself */
    }
}

/* Forgetting a move: the question across the top 4 px over the Summary's two windows, the band and BACK 4 px under
 * them; five move rows inside the page's window with room for a rule, POWER and ACCURACY under them. */
static void learn(void)
{
    const CtrPokeTouchRect *view[5] = {&kLearnTitle, &kSumMon, &kSumPage, &kLearnNote, &kLearnBack};
    CtrPokeTouchRect last = SumMoveRow(4);

    Framed(&kLearnTitle);
    assert(kLearnTitle.x == 4 && kLearnTitle.y == 4 && kLearnTitle.x + kLearnTitle.w == 316 && kLearnTitle.h - 16 >= 14);
    assert(kSumMon.y == Bottom(&kLearnTitle) + 4 && kSumPage.y == kSumMon.y);
    Dialogue(&kLearnNote);
    assert(kLearnNote.h - 16 >= 2 * 14 && kLearnNote.y == Bottom(&kSumMon) + 4 && Bottom(&kLearnNote) == 236);
    Button(&kLearnBack);
    assert(kLearnBack.x == kLearnNote.x + kLearnNote.w + 4 && kLearnBack.x + kLearnBack.w == 316 && kLearnBack.y == kLearnNote.y);
    Apart(view, 5);
    assert(kSumPage.y + kSumPage.h - 8 - Bottom(&last) >= 4 + 2 * 14);
}

/* The naming keyboard: the name's window and the tabs' window side by side, the keys' window 4 px under them, the band,
 * DELETE and OK 4 px under that (as the quantity box's OK and CANCEL); the tabs inside their window's frame; every key
 * of a page of six or eight across inside the keys' window's frame, 24 px or more each way. */
static void naming(void)
{
    const CtrPokeTouchRect *view[6] = {&kNameEntry, &kNamePages, &kNameKeys, &kNameNote, &kNameDelete, &kNameOk};

    Framed(&kNameEntry);
    Framed(&kNamePages);
    Framed(&kNameKeys);
    assert(kNameEntry.x == 4 && kNameEntry.y == 4 && kNamePages.x == kNameEntry.x + kNameEntry.w + 4);
    assert(kNamePages.x + kNamePages.w == 316 && kNamePages.h == kNameEntry.h && kNameEntry.h - 16 >= 32);
    assert(kNameKeys.x == 4 && kNameKeys.x + kNameKeys.w == 316 && kNameKeys.y == Bottom(&kNameEntry) + 4);
    Dialogue(&kNameNote);
    assert(kNameNote.x == 4 && kNameNote.y == Bottom(&kNameKeys) + 4 && Bottom(&kNameNote) == 236 && kNameNote.h >= 56);
    Button(&kNameDelete);
    Button(&kNameOk);
    assert(kNameDelete.x == kNameNote.x + kNameNote.w + 4 && kNameOk.x == kNameDelete.x + kNameDelete.w + 4);
    assert(kNameOk.x + kNameOk.w == 316 && kNameDelete.y == kNameNote.y && kNameOk.h == kNameNote.h);
    assert(kNameDelete.x == kBagQtyOk.x && kNameOk.x == kBagQtyCancel.x);
    Apart(view, 6);
    for (int i = 0; i < 3; ++i) {
        CtrPokeTouchRect t = NameTab(i);

        Button(&t);
        InFrame(&t, &kNamePages);
    }
    for (int cols = 6; cols <= PT_NAME_COLS; cols += 2)
        for (int row = 0; row < PT_NAME_ROWS; ++row)
            for (int col = 0; col < cols; ++col) {
                CtrPokeTouchRect k = NameKey(cols, row, col);

                Button(&k);
                InFrame(&k, &kNameKeys);
            }
}

/* The idle screen: the logo's panel in the middle of the screen, whole tiles, the logo (224x126) inside its frame. */
static void idle(void)
{
    Dialogue(&kIdlePanel);
    assert(kIdlePanel.x * 2 + kIdlePanel.w == 320 && kIdlePanel.y * 2 + kIdlePanel.h == 240);
    assert(kIdlePanel.w - 32 >= 224 && kIdlePanel.h - 16 >= 126);
}

/* FireRed's picture: 240x160 inside the frame's window, which sits in the middle of the page; the band 4 px under it
 * (two lines), BACK beside it; the whole band without BACK. */
static void viewport(void)
{
    const CtrPokeTouchRect pic = {PT_VIEW_X, PT_VIEW_Y, PT_VIEW_W, PT_VIEW_H};
    const CtrPokeTouchRect *view[3] = {&kViewFrame, &kViewNote, &kViewBack};

    assert(PT_VIEW_W == 240 && PT_VIEW_H == 160);
    Framed(&kViewFrame);
    InFrame(&pic, &kViewFrame);
    assert(kViewFrame.x * 2 + kViewFrame.w == 320 && kViewFrame.y == 4);
    assert(pic.x - kViewFrame.x == 8 && kViewFrame.x + kViewFrame.w - (pic.x + pic.w) == 8);
    assert(pic.y - kViewFrame.y - 8 == Bottom(&kViewFrame) - 8 - Bottom(&pic));   /* centred in it */
    Dialogue(&kViewNote);
    assert(kViewNote.x == 4 && kViewNote.y == Bottom(&kViewFrame) + 4 && Bottom(&kViewNote) == 236 && kViewNote.h - 16 >= 2 * 14);
    Button(&kViewBack);
    assert(kViewBack.x == kViewNote.x + kViewNote.w + 4 && kViewBack.x + kViewBack.w == 316 && kViewBack.y == kViewNote.y);
    Apart(view, 3);
    Dialogue(&kViewNoteWide);
    assert(kViewNoteWide.x == 4 && kViewNoteWide.x + kViewNoteWide.w == 316 && kViewNoteWide.y == kViewNote.y);
}

int main(void)
{
    idle();
    viewport();
    home();
    battle();
    party();
    summary();
    bag();
    dex();
    options();
    card();
    box();
    choice();
    learn();
    naming();
    printf("PASS lower layout%s\n", LOWER_LAYOUT_PROVISIONAL ? " (provisional values)" : "");
    return 0;
}
