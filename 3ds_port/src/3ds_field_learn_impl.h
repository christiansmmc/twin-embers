/* Forgetting a move for a new one as a page (3ds_poketouch.h CtrPokeTouchLearn; phase 2), included at the end of
 * 3ds_poketouch.c after the Pokedex, whose picture helpers it shares. A remote for FireRed's Summary in that mode, so it
 * takes the Summary page's places: FireRed's question across the top, the Pokemon (its name, level and picture) on the
 * left, its four moves and the new one (after a rule) as rows with their types and PP, FireRed's arrow on the Summary's
 * cursor or the row held, an HM toned (FireRed will not forget it), the chosen move's power and accuracy under them; its
 * description in the band, BACK giving the new move up (the Summary's B). Every word but the labels is FireRed's
 * (CtrBottomLearn_*). */

int CtrPokeTouch_LearnHit(const CtrPokeTouchView *v, int x, int y)
{
    const CtrPokeTouchLearn *l = &v->learn;

    if (!l->on)
        return -1;
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        CtrPokeTouchRect r = SumMoveRow(i);

        if (l->row[i].move && UiIn(&r, x, y))
            return PT_LEARN_HIT_ROW + i;
    }
    return -1;
}

/* The Pokemon: its name, its level, its front picture. */
static void FLMon(const CtrPokeTouchView *v)
{
    const CtrPokeTouchLearn *l = &v->learn;
    const CtrPokeTouchRect *r = &kSumMon;
    int x = r->x + 8, right = r->x + r->w - 8, top = r->y + 8;

    FPageWindow(r, v->frameType);
    KitText(l->nickname, x, top, right);
    KitText(l->level, x, top + KIT_LINE_H, right);
    FDPicDraw(l->species, r->x + (r->w - 64) / 2, top + 2 * KIT_LINE_H + 16, 0);
}

/* The moves: a row each (the type, the name, the PP in FireRed's colours), the new one after a rule; under them the
 * chosen move's power and accuracy. */
static void FLMoves(const CtrPokeTouchView *v)
{
    const CtrPokeTouchLearn *l = &v->learn;
    const CtrPokeTouchRect *p = &kSumPage;
    int held = l->pressed >= PT_LEARN_HIT_ROW && l->pressed < PT_LEARN_HIT_ROW + PT_LEARN_ROWS ? l->pressed - PT_LEARN_HIT_ROW : -1;
    int c = l->cursor >= 0 && l->cursor < PT_LEARN_ROWS ? l->cursor : 0, x = p->x + 8, right = p->x + p->w - 8;
    CtrPokeTouchRect last = SumMoveRow(PT_LEARN_ROWS - 1);
    int y = last.y + last.h + 2;

    FPageWindow(p, v->frameType);
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        CtrPokeTouchRect r = SumMoveRow(i);
        int top = r.y + (r.h - KIT_LINE_H) / 2, nameX = r.x + KIT_ARROW_W + 48;

        Fill(r.x, r.y, r.w, r.h, held == i ? KitInk(3) : KitInk(1));
        if (i == PT_LEARN_ROWS - 1)
            Fill(r.x, r.y, r.w, 1, KitInk(3));
        if (held >= 0 ? held == i : l->cursor == i)
            KitText(KIT_ARROW, r.x, top, r.x + KIT_ARROW_W);
        if (!l->row[i].move) {
            KitTextOn("-", nameX, top, r.x + r.w, 0);
            continue;
        }
        if (l->row[i].type >= 0) {
            CtrPokeTouchRect t = {(int16_t)(r.x + KIT_ARROW_W), (int16_t)(r.y + 2), 44, 13};

            KitTypeLabel(&t, l->row[i].type, l->row[i].typeName);
        }
        KitTextOn(l->row[i].name, nameX, top, r.x + r.w - UiTextWidth(l->row[i].pp, 1) - 4, !l->row[i].locked);
        UiText(l->row[i].pp, r.x + r.w - UiTextWidth(l->row[i].pp, 1), top + 3,
               SPpColor(SNumber(l->row[i].pp, 0), SNumber(l->row[i].pp, 1)), KitInk(3), 1, r.x + r.w);
    }
    Fill(x, y, right - x, 1, KitInk(3));
    if (l->row[c].move) {
        SLine(x, right, y + 4, CtrUiText(UIS_MOVE_POWER), l->row[c].power);
        SLine(x, right, y + 4 + KIT_LINE_H, CtrUiText(UIS_MOVE_ACCURACY), l->row[c].accuracy);
    }
}

static void FLearnPageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchLearn *l = &v->learn;
    int c = l->cursor >= 0 && l->cursor < PT_LEARN_ROWS ? l->cursor : 0;

    Fill(0, 0, W, H, BPageBg());
    FPageWindow(&kLearnTitle, v->frameType);
    KitText(l->question, kLearnTitle.x + 8, kLearnTitle.y + 8, kLearnTitle.x + kLearnTitle.w - 8);
    FLMon(v);
    FLMoves(v);
    FPageBand(&kLearnNote, NULL, l->row[c].move ? l->desc : "", 0);
    if (v->backVisible)
        FPageBack(v, &kLearnBack);
}
