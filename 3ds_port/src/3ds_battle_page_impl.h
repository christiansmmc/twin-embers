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

/* A party strip, six slots 48 px apart from (x, y) (kind, hp, status and icon as CtrBattleView's party fields): in
 * each slot the Pokemon's own party icon (frame 0, from the pack), toned when it has fainted, with an amber dot at its
 * corner for a status condition, and FireRed's HP bar under it (y + 33). Without its icon a slot shows the port's ball
 * mark instead; an empty slot shows the empty ring. The battle's party bar and HOME's party window. */
static void PartyStrip(int x, int y, const uint8_t kind[6], const uint8_t hp[6], const uint8_t status[6],
                       const uint16_t icon[6])
{
    for (int i = 0; i < 6; ++i) {
        int sx = x + 48 * i, k = kind[i];
        int sick = k == 3 || (status[i] >= 1 && status[i] <= 6);

        if (k && icon[i] && BPartyIcon(i, icon[i])) {
            for (int py = 0; py < 32; ++py)
                for (int px = 0; px < 32; ++px) {
                    uint8_t index = sBPIcon[i].pixels[py * 32 + px];

                    if (index)
                        Px(sx + px, y + py, Tone(sBPIcon[i].pal[index], k != 2));
                }
            if (sick) {
                UiDisc(sx + 28, y + 4, 3, UI_RGB(110, 40, 6));
                UiDisc(sx + 28, y + 4, 2, UI_RGB(255, 200, 60));
            }
        } else {
            BBall(sx + 16, y + 16, k, sick);
        }
        if (k)
            KitHpBar(sx + 2, y + 33, 28, k == 2 ? 0 : hp[i], 100);
    }
}

/* The party on top of every battle page. */
static void BPageParty(const CtrBattleView *v)
{
    const CtrPokeTouchRect *s = &kBatParty;

    Fill(s->x, s->y, s->w, s->h, BPageBg());
    PartyStrip(s->x + 8, s->y, v->party, v->partyHp, v->partyStatus, v->partyIcon);
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

/* ---- The moves and a double battle's targets ---- */
/* What a move does to the foe, as today's tags (the port's own colours: x2 and x4 green, x1/2 orange, x0 and a plain hit
 * grey), flat now. */
static const uint16_t kBFxColor[] = {0, UI_RGB(110, 100, 110), UI_RGB(204, 84, 24), UI_RGB(214, 112, 30),
                                     UI_RGB(40, 150, 70), UI_RGB(30, 170, 60), UI_RGB(100, 106, 132)};

/* The note's STAB tag: the frames' slate, flat. */
static const uint16_t kBStabColor = UI_RGB(90, 100, 120);

static const char *BFxText(uint8_t matchup)
{
    static const char *const kText[] = {"", CTR_TR("NO EFFECT", "NO AFECTA", "Wirkungslos"),
                                        CTR_TR("NOT VERY", "POCO EFIC.", "Nicht sehr"), CTR_TR("NOT VERY", "POCO EFIC.", "Nicht sehr"),
                                        CTR_TR("SUPER", "S\xC3\x9APER", "Sehr"), CTR_TR("SUPER x4", "S\xC3\x9APER x4", "Sehr x4"),
                                        CTR_TR("EFFECTIVE", "EFICAZ", "Effektiv")};

    return matchup <= CTR_FX_NEUTRAL ? kText[matchup] : "";
}

/* The tag on a line, ending at `right`; nothing for CTR_FX_NONE (the option off, or a status move). */
static void BPageFxTag(uint8_t matchup, int right, int lineTop)
{
    const char *text;
    int w;

    if (matchup == CTR_FX_NONE || matchup > CTR_FX_NEUTRAL)
        return;
    text = BFxText(matchup);
    w = UiTextWidth(text, 1) + 8;
    Fill(right - w, lineTop, w, KIT_LINE_H - 1, kBFxColor[matchup]);
    UiText(text, right - w + 4, lineTop + 3, RGB(255, 255, 255), KitInk(2), 1, right);
}

/* The three lines of a move or target cell, 16 px apart from the top of its interior. */
#define BPAGE_LINE(in, n) ((in).y + 1 + 16 * (n))

/* A label in the toned ink, then its value in the text colour a space later; returns where the value ends. */
static int BPageField(const char *label, const char *value, int x, int lineTop, int limit)
{
    int vx = x + UiTextWidth(label, 1) + UiTextWidth(" ", 1);

    KitTextOn(label, x, lineTop, limit, 0);
    KitText(value, vx, lineTop, limit);
    return vx + UiTextWidth(value, 1);
}

/* PP as five pips from x, a fifth of the move's PP each (rounded up), in FireRed's HP colours by what is left (over
 * half, down to a quarter, below); the unlit ones the light grey. */
static void BPagePips(int x, int lineTop, unsigned pp, unsigned maxPp)
{
    int lit = maxPp ? (int)((pp * 5 + maxPp - 1) / maxPp) : 0;
    uint16_t on = pp * 4 <= maxPp ? UI_HP_LOW : pp * 2 <= maxPp ? UI_HP_MID : UI_HP_HIGH;

    if (lit > 5)
        lit = 5;
    for (int k = 0; k < 5; ++k) {
        uint16_t c = k < lit ? on : KitInk(3);
        int px = x + 6 * k, py = lineTop + 6;

        Fill(px + 1, py, 3, 1, c);
        Fill(px, py + 1, 5, 3, c);
        Fill(px + 1, py + 4, 3, 1, c);
    }
}

/* A move card, three lines: the arrow, its name and its type; POW and what it does to the foe (STATUS for a status move
 * when no tag says it); its PP as pips, ACC and PP (grey at none left). Labels in the toned ink. Where ACC's label does
 * not fit between the pips and PP, only its number shows. An empty slot is the page only. */
static void BPageMove(const CtrBattleView *v, int i)
{
    const CtrPokeTouchRect *r = &kBatMove[i];
    const CtrBattleMove *m = &v->move[i];
    CtrPokeTouchRect in = BPageInside(r);
    int right = in.x + in.w, x = in.x + KIT_ARROW_W, line;
    const char *acc = m->accuracy[0] && m->accuracy[0] != '-' ? m->accuracy : "--";
    const char *accLabel = CTR_TR("ACC", "PRE", "Gen");
    char pp[16];

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    if (!m->present)
        return;
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    Fill(in.x, in.y, in.w, in.h, v->pressed == i ? KitInk(3) : KitInk(1));

    line = BPAGE_LINE(in, 0);
    if (BPageChosen(v, i, i))
        KitText(KIT_ARROW, in.x, line, right);
    {
        CtrPokeTouchRect type = {(int16_t)(right - 40), (int16_t)line, 40, KIT_LINE_H - 1};

        KitTypeLabel(&type, m->type, m->typeName);
    }
    KitText(m->name, x, line, right - 40 - 4);

    line = BPAGE_LINE(in, 1);
    BPageField(CTR_TR("POW", "POT", "St\xC3\xA4"), m->power[0] ? m->power : "-", x, line, right);
    if (m->matchup != CTR_FX_NONE) {
        BPageFxTag(m->matchup, right, line);
    } else if (!m->power[0] || m->power[0] == '-') {
        const char *status = CTR_TR("STATUS", "ESTADO", "Status");

        KitTextOn(status, right - UiTextWidth(status, 1), line, right, 0);
    }

    line = BPAGE_LINE(in, 2);
    BPagePips(x, line, m->pp, m->maxPp);
    snprintf(pp, sizeof(pp), "%u/%u", m->pp, m->maxPp);
    {
        int ppw = UiTextWidth(pp, 1), space = UiTextWidth(" ", 1);
        int ppLabelX = right - ppw - space - UiTextWidth("PP", 1);
        int accRight = ppLabelX - 8, accW = UiTextWidth(acc, 1);
        int accStart = accRight - accW - space - UiTextWidth(accLabel, 1);

        UiText(pp, right - ppw, line + 3, Tone(KitInk(2), m->pp > 0), Tone(KitInk(3), m->pp > 0), 1, right);
        KitTextOn("PP", ppLabelX, line, right, 0);
        if (accStart >= x + 29 + 4)
            BPageField(accLabel, acc, accStart, line, accRight);
        else
            KitText(acc, accRight - accW, line, accRight);
    }
}

/* The note under the moves, in the dialogue frame: the description of the move under the controller's cursor in three
 * lines (or whether it is physical or special), its STAB tag at the first line's right, that line stopping 4 px short
 * of it; the accuracy is on the card (the owner's pick: every English description fits). Three 14 px lines from 6 px
 * in keep the glyphs inside the frame's 40 px interior. */
static void BPageMoveNote(const CtrBattleView *v)
{
    const CtrPokeTouchRect *n = &kBatMoveNote;
    int sel = v->cursor >= 0 && v->cursor < 4 && v->move[v->cursor].present ? v->cursor : 0;
    const CtrBattleMove *m = &v->move[sel];
    int x = n->x + 16, top = n->y + 6, right = n->x + n->w - 16, tag = 0;

    Fill(n->x, n->y, n->w, n->h, BPageBg());
    KitPanel(n, KIT_FRAME_DIALOGUE, 0);
    if (!m->present)
        return;
    if (m->stab) {
        tag = UiTextWidth("STAB", 1) + 8;
        Fill(right - tag, top + 2, tag, KIT_LINE_H - 3, kBStabColor);
        UiText("STAB", right - tag + 4, top + 3, RGB(255, 255, 255), KitInk(2), 1, right);
    }
    KitWrapFirst(m->desc[0] ? m->desc
                            : m->type < 10 ? CTR_TR("Physical move", "Ataque f\xC3\xADsico", "Physischer Angriff")
                                           : CTR_TR("Special move", "Ataque especial", "Spezialangriff"),
                 x, top, right - x - (tag ? tag + 4 : 0), right - x, 3);
}

/* A target: the arrow and its name, FOE or ALLY; its level and what the move does to it; its HP (and the numbers for
 * the player's own). */
static void BPageTarget(const CtrBattleView *v, int i)
{
    const CtrPokeTouchRect *r = &kBatTarget[i];
    const CtrBattleTarget *t = &v->target[i];
    CtrPokeTouchRect in = BPageInside(r);
    const char *side = t->side ? CTR_TR("ALLY", "ALIADO", "Partner") : CTR_TR("FOE", "RIVAL", "Gegner");
    int right = in.x + in.w, sw = UiTextWidth(side, 1);
    char text[20];

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    if (!t->present)
        return;
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    Fill(in.x, in.y, in.w, in.h, v->pressed == i ? KitInk(3) : KitInk(1));
    if (BPageChosen(v, i, i))
        KitText(KIT_ARROW, in.x, BPAGE_LINE(in, 0), right);
    KitText(side, right - sw, BPAGE_LINE(in, 0), right);
    KitText(t->name, in.x + KIT_ARROW_W, BPAGE_LINE(in, 0), right - sw - 4);
    snprintf(text, sizeof(text), "Lv%u", t->level);
    KitText(text, in.x + KIT_ARROW_W, BPAGE_LINE(in, 1), right);
    BPageFxTag(t->matchup, right, BPAGE_LINE(in, 1));
    KitHpBar(in.x + KIT_ARROW_W, BPAGE_LINE(in, 2) + 5, 64, t->hp, t->maxHp);
    if (t->side) {
        snprintf(text, sizeof(text), "%u/%u", t->hp, t->maxHp);
        KitText(text, right - UiTextWidth(text, 1), BPAGE_LINE(in, 2), right);
    }
}

static void BPageTargetNote(void)
{
    const CtrPokeTouchRect *n = &kBatMoveNote;

    Fill(n->x, n->y, n->w, n->h, BPageBg());
    KitPanel(n, KIT_FRAME_DIALOGUE, 0);
    KitWrap(CTR_TR("Which POK\xC3\xA9MON should the move hit?", "\xC2\xBF" "A qu\xC3\xA9 POK\xC3\xA9MON ataca?",
                   "Welches Pok\xC3\xA9mon angreifen?"),
            n->x + 16, n->y + 8, n->w - 32, (n->h - 16) / KIT_LINE_H);
}

/* CANCEL: a framed cell with one row, the arrow on it while it is held. */
static void BPageCancel(const CtrBattleView *v)
{
    const CtrPokeTouchRect *r = &kBatCancel;
    CtrPokeTouchRect in = BPageInside(r);

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    KitRow(&in, CTR_TR("CANCEL", "CANCELAR", "Zur\xC3\xBC" "ck"), v->pressed == BUI_CANCEL, 1);
}

static void BPageScreen(const CtrBattleView *v)
{
    Fill(0, 0, W, H, BPageBg());
    BPageParty(v);                      /* the party's balls on every battle page, in one place */
    if (v->state == BUI_MOVE || v->state == BUI_TARGET) {
        for (int i = 0; i < 4; ++i) {
            if (v->state == BUI_MOVE)
                BPageMove(v, i);
            else
                BPageTarget(v, i);
        }
        if (v->state == BUI_MOVE)
            BPageMoveNote(v);
        else
            BPageTargetNote();
        BPageCancel(v);
        return;
    }
    if (v->state == BUI_IDLE)
        return;                         /* a message or an animation: the party's balls only */
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
    if (v->state == BUI_IDLE)
        return;                         /* the balls change only with the screen's data (BScreenChanged) */
    if (v->state == BUI_MOVE || v->state == BUI_TARGET) {
        for (int i = 0; i < 4; ++i)
            if (BPageLook(old, i) != BPageLook(v, i)) {
                if (v->state == BUI_MOVE)
                    BPageMove(v, i);
                else
                    BPageTarget(v, i);
                BPageMark(dirty, v->state == BUI_MOVE ? &kBatMove[i] : &kBatTarget[i]);
            }
        if (v->state == BUI_MOVE && old->cursor != v->cursor) {
            BPageMoveNote(v);
            BPageMark(dirty, &kBatMoveNote);
        }
        if ((old->pressed == BUI_CANCEL) != (v->pressed == BUI_CANCEL)) {
            BPageCancel(v);
            BPageMark(dirty, &kBatCancel);
        }
        return;
    }
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
