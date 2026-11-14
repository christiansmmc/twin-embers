/* The pieces the field's pages share (phase 2 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md): a
 * window with the player's frame on the page's colour, a list row, the band (the dialogue frame, a title line over
 * wrapped text) and BACK's cell. Included by 3ds_poketouch.c after the BAG's page, whose cells it uses, and before the
 * field's views. */

#define FPAGE_BACK CTR_TR("BACK", "ATR\xC3\x81S", "Zur\xC3\xBC" "ck")

/* A window: the page's colour under it (the frame's clear pixels show it) and the player's standard frame. */
static void FPageWindow(const CtrPokeTouchRect *r, int frameType)
{
    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, frameType);
}

/* A row of a list: the light grey under it while held, FireRed's arrow when chosen, the label after it (toned when it
 * cannot be used), cut at the row's end. */
static void FPageRow(const CtrPokeTouchRect *r, const char *label, int chosen, int pressed, int on)
{
    int top = r->y + (r->h - KIT_LINE_H) / 2;

    Fill(r->x, r->y, r->w, r->h, pressed ? KitInk(3) : KitInk(1));
    if (chosen)
        KitText(KIT_ARROW, r->x, top, r->x + KIT_ARROW_W);
    KitTextOn(label, r->x + KIT_ARROW_W, top, r->x + r->w, on);
}

/* The band: `title` on its first line and `body` wrapped on the lines left (two in a 56 px band, one in 48), 16 px in;
 * no title: the body from the first line. While a message waits for A, the continue mark. */
static void FPageBand(const CtrPokeTouchRect *n, const char *title, const char *body, int waiting)
{
    int x = n->x + 16, w = n->w - 32, lines = n->h >= 56 ? 3 : 2, top = n->y + (lines == 3 ? 6 : 10);

    Fill(n->x, n->y, n->w, n->h, BPageBg());
    KitPanel(n, KIT_FRAME_DIALOGUE, 0);
    if (title && title[0]) {
        KitText(title, x, top, x + w);
        top += KIT_LINE_H;
        --lines;
    }
    if (body && body[0])
        KitWrap(body, x, top, w, lines);
    if (waiting) {
        int cx = n->x + n->w - 20, cy = n->y + n->h - 13;

        for (int row = 0; row < 4; ++row)
            Fill(cx - 3 + row, cy + row, 7 - 2 * row, 1, KitInk(2));
    }
}

/* A menu or YES / NO: the Party's window at the bottom right with `count` rows (PARTY_MENU_ROWS at most), FireRed's
 * arrow on the chosen one, the light grey under the one held (-1: none). */
static void FPageMenu(int frameType, const char *const *names, int count, int chosen, int held)
{
    CtrPokeTouchRect w;

    if (count > PARTY_MENU_ROWS)
        count = PARTY_MENU_ROWS;
    w = PartyMenuWindow(count);
    FPageWindow(&w, frameType);
    for (int i = 0; i < count; ++i) {
        CtrPokeTouchRect r = PartyMenuRow(count, i);

        FPageRow(&r, names[i], chosen == i, held == i, 1);
    }
}

/* BACK's cell, the view's B: FireRed's arrow and the light grey while held, toned while it cannot be used. */
static void FPageBack(const CtrPokeTouchView *v, const CtrPokeTouchRect *r)
{
    int held = v->backEnabled && v->backPressed;

    BagCell(r, v->frameType, FPAGE_BACK, held, held, v->backEnabled);
}
