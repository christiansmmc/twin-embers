/* The BAG's pieces as a page (phase 2 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md, the owner's
 * "3"), for the field's Bag and the lists drawn like it (3ds_field_bag_impl.h) and for the battle's (BBag): framed tabs
 * and cells with their label centred, the list's rows with each item's icon at 1:1, the scroll cells, the band in the
 * dialogue frame, the Party's menu window, the quantity box. Where they go is 3ds_lower_layout.h's. Included by
 * 3ds_battle_ui_impl.h after the battle pages. */

/* A framed cell (a tab, CANCEL, OK) with its label centred, FireRed's arrow before it when chosen, the light grey
 * inside while held, the label toned when it cannot be used. */
static void BagCell(const CtrPokeTouchRect *r, int frameType, const char *label, int chosen, int pressed, int on)
{
    CtrPokeTouchRect in = BPageInside(r);
    int w = UiTextWidth(label, 1), x = in.x + (in.w - w) / 2, top = in.y + (in.h - KIT_LINE_H) / 2;

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, frameType);
    if (pressed)
        Fill(in.x, in.y, in.w, in.h, KitInk(3));
    if (x < in.x + KIT_ARROW_W)
        x = in.x + KIT_ARROW_W;
    if (chosen)
        KitText(KIT_ARROW, x - KIT_ARROW_W, top, in.x + in.w);
    KitTextOn(label, x, top, in.x + in.w, on);
}

/* A framed cell with a triangle (0 up, 1 down): the list's scroll; toned while the list cannot go that way. */
static void BagArrowCell(const CtrPokeTouchRect *r, int frameType, int dir, int on, int pressed)
{
    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, frameType);
    if (pressed && on)
        Fill(r->x + 8, r->y + 8, r->w - 16, r->h - 16, KitInk(3));
    KitTriangle(r->x + r->w / 2, r->y + r->h / 2, dir, Tone(KitInk(2), on));
}

/* A row of the list: FireRed's arrow when chosen, the item's icon at 1:1 (none for a CLOSE row: item 0), its name, and
 * what goes on its right (a count, a price, its button X / Y); the light grey under it while held. */
static void BagRowDraw(const CtrPokeTouchRect *r, unsigned item, const char *name, const char *right, int chosen,
                       int pressed)
{
    int top = r->y + (r->h - KIT_LINE_H) / 2, end = r->x + r->w;

    Fill(r->x, r->y, r->w, r->h, pressed ? KitInk(3) : KitInk(1));
    if (chosen)
        KitText(KIT_ARROW, r->x, top, r->x + KIT_ARROW_W);
    if (item)
        BItemDraw(item, r->x + KIT_ARROW_W, r->y + (r->h - 24) / 2, 1);
    if (right && right[0]) {
        KitText(right, end - UiTextWidth(right, 1), top, end);
        end -= UiTextWidth(right, 1) + 6;
    }
    KitText(name, r->x + KIT_ARROW_W + 30, top, end);
}

/* The band: `text` in up to three lines from 6 px in; on its first line's right a move's type (typeId >= 0) and,
 * left of it, `aside` (how many the Bag holds, the money); while a message waits for A, the continue mark. */
static void BagNote(const CtrPokeTouchRect *n, const char *text, const char *aside, int typeId, const char *typeName,
                    int waiting)
{
    int x = n->x + 16, right = n->x + n->w - 16, top = n->y + 6, first = right;

    Fill(n->x, n->y, n->w, n->h, BPageBg());
    KitPanel(n, KIT_FRAME_DIALOGUE, 0);
    if (typeId >= 0) {
        CtrPokeTouchRect label = {(int16_t)(right - 44), (int16_t)(top + 1), 44, 13};

        KitTypeLabel(&label, typeId, typeName);
        first = label.x - 6;
    }
    if (aside && aside[0]) {
        KitText(aside, first - UiTextWidth(aside, 1), top, first);
        first -= UiTextWidth(aside, 1) + 6;
    }
    KitWrapFirst(text, x, top, first - x, right - x, 3);
    if (waiting) {
        int cx = right - 4, cy = n->y + n->h - 13;

        for (int row = 0; row < 4; ++row)
            Fill(cx - 3 + row, cy + row, 7 - 2 * row, 1, KitInk(2));
    }
}

/* The Party's menu window at the bottom right with `count` rows, FireRed's arrow on the chosen one. */
static void BagMenu(int frameType, const char *const *names, int count, int chosen)
{
    CtrPokeTouchRect w = PartyMenuWindow(count);

    Fill(w.x, w.y, w.w, w.h, BPageBg());
    KitPanel(&w, KIT_FRAME_STD, frameType);
    for (int i = 0; i < count; ++i) {
        CtrPokeTouchRect r = PartyMenuRow(count, i);

        KitRow(&r, names[i], chosen == i, 1);
    }
}

/* The quantity box: the count (and what it comes to, `price`, when there is one) in its window, up and down, OK and
 * CANCEL; each cell's light grey while held (`pressed`: 0 none, 1 up, 2 down, 3 OK, 4 CANCEL). */
static void BagQuantity(int frameType, unsigned count, const char *price, int pressed)
{
    const CtrPokeTouchRect *w = &kBagQtyCount;
    char text[12];
    int top = w->y + (w->h - KIT_LINE_H) / 2, right = w->x + w->w - 8;

    Fill(w->x, w->y, w->w, w->h, BPageBg());
    KitPanel(w, KIT_FRAME_STD, frameType);
    snprintf(text, sizeof(text), "x%02u", count);
    KitText(text, w->x + 8, top, right);
    if (price && price[0])
        KitText(price, right - UiTextWidth(price, 1), top, right);
    BagArrowCell(&kBagQtyUp, frameType, 0, 1, pressed == 1);
    BagArrowCell(&kBagQtyDown, frameType, 1, 1, pressed == 2);
    BagCell(&kBagQtyOk, frameType, CtrUiText(UIS_OK), 0, pressed == 3, 1);
    BagCell(&kBagQtyCancel, frameType, CtrUiText(UIS_CANCEL), 0, pressed == 4, 1);
}
