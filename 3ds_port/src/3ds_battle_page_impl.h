/* The battle's reworked views (phase 1 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md): the
 * commands with the quick ball, and the Safari Zone's four, drawn with the kit where 3ds_lower_layout.h puts them.
 * FireRed's arrow marks the cell held under a touch, else the controller's own cursor. The icons are the ones the game
 * reads from the player's pack (item icons, the lead's party icon) and the port's own shoe for RUN. Included by
 * 3ds_battle_ui_impl.h before BScreen. */

static const char *const kBPageAction[4] = {CTR_TR("FIGHT", "LUCHAR", "Kampf"), CTR_TR("BAG", "MOCHILA", "Beutel"),
                                           CTR_TR("POK\xC3\xA9MON", "POK\xC3\xA9MON", "Pok\xC3\xA9mon"),
                                           CTR_TR("RUN", "HUIR", "Flucht")};
static const char *const kBPageSafari[4] = {CTR_TR("BALL", "BALL", "Ball"), CTR_TR("BAIT", "CEBO", "K\xC3\xB6" "der"),
                                           CTR_TR("ROCK", "ROCA", "Stein"), CTR_TR("RUN", "HUIR", "Flucht")};

/* The page's colour behind its cells (provisional, with the layout). */
static uint16_t BPageBg(void)
{
    return KitInk(3);
}

static void BPageMark(CtrDirtyList *dirty, const CtrPokeTouchRect *r)
{
    CtrDirty_Add(dirty, r->x, r->y, r->x + r->w, r->y + r->h);
}

/* The cell under the arrow: the one held under a touch (its hit code), else the controller's cursor. */
static int BPageChosen(const CtrBattleView *v, int cursor, int hit)
{
    return v->pressed >= 0 ? v->pressed == hit : v->cursor == cursor;
}

/* What a command cell's picture depends on, apart from the screen's data: chosen, pressed. */
static int BPageLook(const CtrBattleView *v, int i)
{
    return BPageChosen(v, i, i) | (v->pressed == i) << 1;
}

static int BPageBallChosen(const CtrBattleView *v)
{
    return v->pressed >= 0 ? BBallPart(v->pressed) : v->cursor == CTR_ACTION_BALL;
}

static int BPageBallLook(const CtrBattleView *v)
{
    return BPageBallChosen(v) | (v->pressed == BUI_ACT_BALL) << 1 | (v->pressed == BUI_ACT_BALL_PREV) << 2
        | (v->pressed == BUI_ACT_BALL_NEXT) << 3;
}

static CtrPokeTouchRect BPageInside(const CtrPokeTouchRect *r)
{
    CtrPokeTouchRect in = {(int16_t)(r->x + 8), (int16_t)(r->y + 8), (int16_t)(r->w - 16), (int16_t)(r->h - 16)};

    return in;
}

/* A framed cell: the page under it (the frame's clear pixels show it), the FRAME type's frame, an icon cell inside. */
static void BPageCell(const CtrPokeTouchRect *r, const CtrBattleView *v, const uint8_t *pixels, const uint16_t *pal,
                      const char *label, int chosen, int pressed, int on)
{
    CtrPokeTouchRect in = BPageInside(r);

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    KitIconCell(&in, pixels, pal, 24, 24, label, chosen, pressed, on);
}

/* The port's own shoe where a cell's icon goes (FireRed has no icon for running away). */
static void BPageShoe(const CtrPokeTouchRect *r, int on)
{
    CtrPokeTouchRect in = BPageInside(r);
    int lineTop = in.y + in.h - 2 - KIT_LINE_H;
    int x = in.x + (in.w - FLATSHOESMALL_W) / 2, y = in.y + (lineTop - in.y - FLATSHOESMALL_H) / 2;

    for (int py = 0; py < FLATSHOESMALL_H; ++py)
        for (int px = 0; px < FLATSHOESMALL_W; ++px)
            if (kFlatShoeSmall[py * FLATSHOESMALL_W + px])
                Px(x + px, y + py, Tone(kFlatShoeSmall[py * FLATSHOESMALL_W + px], on));
}

/* FIGHT: the lead's party icon at twice its size (bouncing as before), FIGHT, its name and level. */
static void BPageFight(const CtrBattleView *v, const CtrPokeTouchRect *r)
{
    CtrPokeTouchRect in = BPageInside(r);
    int tx = in.x + 8 + 64 + 16, top = in.y + (in.h - 2 * KIT_LINE_H - 4) / 2;

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    Fill(in.x, in.y, in.w, in.h, v->pressed == 0 ? KitInk(3) : KitInk(1));
    if (v->activeIcon && BPartyIcon(v->activeSlot, v->activeIcon)) {
        int x0 = in.x + 8, y0 = in.y + (in.h - 64) / 2 - 2 * v->activeFrame;

        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                uint8_t index = sBPIcon[v->activeSlot].pixels[y * 32 + x];

                if (index)
                    Fill(x0 + 2 * x, y0 + 2 * y, 2, 2, sBPIcon[v->activeSlot].pal[index]);
            }
    }
    if (BPageChosen(v, 0, 0))
        KitText(KIT_ARROW, tx - KIT_ARROW_W, top, in.x + in.w);
    KitText(kBPageAction[0], tx, top, in.x + in.w);
    if (v->activeName[0]) {
        char line[32];

        snprintf(line, sizeof(line), "%s Lv%u", v->activeName, (unsigned)v->activeLevel);
        KitText(line, tx, top + KIT_LINE_H + 4, in.x + in.w);
    }
}

static void BPageAction(const CtrBattleView *v, int i)
{
    const CtrPokeTouchRect *r = BActRect(v, i);
    const uint16_t *pal = NULL;
    const uint8_t *pixels = NULL;

    if (i == 0) {
        BPageFight(v, r);
        return;
    }
    /* The same FireRed item icons as HOME's BAG and POKeMON entries. */
    if (i == 1)
        pixels = HomeEntryIcon(PT_BAG - PT_POKEDEX, &pal);
    else if (i == 2)
        pixels = HomeEntryIcon(PT_POKEMON - PT_POKEDEX, &pal);
    BPageCell(r, v, pixels, pal, kBPageAction[i], BPageChosen(v, i, i), v->pressed == i, 1);
    if (i == 3)
        BPageShoe(r, 1);
}

/* The quick ball: the ball's own icon and how many there are; without one to throw the cell is grey and says why. */
static void BPageBall(const CtrBattleView *v)
{
    int ready = v->ball == CTR_QUICK_BALL_READY, slot = BItemIcon(v->ballItem);
    const uint16_t *pal = slot >= 0 ? sBIcon[slot].pal : NULL;
    const uint8_t *pixels = slot >= 0 ? sBIcon[slot].pixels : NULL;
    char label[16];

    if (ready)
        snprintf(label, sizeof(label), "x%u", (unsigned)v->ballQty);
    else
        snprintf(label, sizeof(label), "%s", v->ball == CTR_QUICK_BALL_FULL ? CTR_TR("BOX FULL", "CAJA LLENA", "Box voll")
                                                                           : CTR_TR("NO BALLS", "SIN BALLS", "Kein Ball"));
    if (!pixels)
        pixels = HomeEntryIcon(PT_POKEMON - PT_POKEDEX, &pal);    /* no ball named: FireRed's Poke Ball icon */
    BPageCell(&kBatBall, v, pixels, pal, label, BPageBallChosen(v), ready && v->pressed == BUI_ACT_BALL, ready);
}

/* An arrow to the previous or next ball, only while there is another; a chevron in the text colour. */
static void BPageBallArrow(const CtrBattleView *v, int next)
{
    const CtrPokeTouchRect *r = &kBatBallArrow[next];
    uint16_t ink = KitInk(2);

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    if (v->ball != CTR_QUICK_BALL_READY || v->ballCount <= 1)
        return;
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    Fill(r->x + 8, r->y + 8, 8, 8, v->pressed == (next ? BUI_ACT_BALL_NEXT : BUI_ACT_BALL_PREV) ? KitInk(3) : KitInk(1));
    for (int k = 0; k < 4; ++k) {
        int x = next ? r->x + 10 + k : r->x + 13 - k;

        Px(x, r->y + 8 + k, ink);
        Px(x, r->y + 15 - k, ink);
    }
}

/* The party's six balls (the port's own marks, as on the old strip) and an HP bar under each. */
static void BPageParty(const CtrBattleView *v)
{
    const CtrPokeTouchRect *s = &kBatParty;

    Fill(s->x, s->y, s->w, s->h, BPageBg());
    for (int i = 0; i < 6; ++i) {
        int cx = s->x + 16 + 32 * i, kind = v->party[i];

        BBall(cx, s->y + 10, kind, kind == 3 || (v->partyStatus[i] >= 1 && v->partyStatus[i] <= 6));
        if (kind)
            KitHpBar(cx - 10, s->y + 19, 20, kind == 2 ? 0 : v->partyHp[i], 100);
    }
}

/* The Safari Zone's BALL: FireRed's Safari Ball icon, read once. */
static const uint8_t *BPageSafariBall(const uint16_t **pal)
{
    static int tried, ok;
    static uint8_t pixels[24 * 24];
    static uint16_t colors[16];

    if (!tried) {
        tried = 1;
        ok = PokeTouch_PackIcon24("safari_ball", pixels, colors);
    }
    *pal = colors;
    return ok ? pixels : NULL;
}

static void BPageSafari(const CtrBattleView *v, int i)
{
    const uint16_t *pal = NULL;
    const uint8_t *pixels = i == 0 ? BPageSafariBall(&pal) : NULL;

    BPageCell(&kBatSafari[i], v, pixels, pal, kBPageSafari[i], BPageChosen(v, i, i), v->pressed == i, 1);
    if (i == 3)
        BPageShoe(&kBatSafari[i], 1);
}

static void BPageScreen(const CtrBattleView *v)
{
    Fill(0, 0, W, H, BPageBg());
    BPageParty(v);
    if (v->state == BUI_SAFARI) {
        for (int i = 0; i < 4; ++i)
            BPageSafari(v, i);
        return;
    }
    for (int i = 0; i < 4; ++i)
        BPageAction(v, i);
    if (v->ball != CTR_QUICK_BALL_NONE) {
        BPageBall(v);
        BPageBallArrow(v, 0);
        BPageBallArrow(v, 1);
    }
}

/* Within one screen whose data did not change (BScreenChanged): the cells whose arrow or press changed, the lead's
 * bounce, the quick ball and its arrows. Each repaints its whole rectangle, so the result equals BPageScreen. */
static void BPageUpdate(const CtrBattleView *old, const CtrBattleView *v, CtrDirtyList *dirty)
{
    for (int i = 0; i < 4; ++i) {
        if (BPageLook(old, i) == BPageLook(v, i) && !(v->state == BUI_ACTION && i == 0 && old->activeFrame != v->activeFrame))
            continue;
        if (v->state == BUI_SAFARI) {
            BPageSafari(v, i);
            BPageMark(dirty, &kBatSafari[i]);
        } else {
            BPageAction(v, i);
            BPageMark(dirty, BActRect(v, i));
        }
    }
    if (v->state == BUI_ACTION && v->ball != CTR_QUICK_BALL_NONE && BPageBallLook(old) != BPageBallLook(v)) {
        BPageBall(v);
        BPageBallArrow(v, 0);
        BPageBallArrow(v, 1);
        BPageMark(dirty, &kBatBall);
        BPageMark(dirty, &kBatBallArrow[0]);
        BPageMark(dirty, &kBatBallArrow[1]);
    }
}
