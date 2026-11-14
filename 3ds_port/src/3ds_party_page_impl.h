/* POKeMON as a page (phase 2 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md, the owner's "1"):
 * the Party in the field and in battle drawn with the kit where 3ds_lower_layout.h puts it. Six framed cards in two
 * columns of three (the Party's D-pad walks them so, recipe 0167), each with the Pokemon's party icon (hopping while it
 * is the Party's selection, as FireRed's does), its name and gender sign, FireRed's HP bar, Lv or its ailment's tag,
 * and its HP numbers (or what an item or a move would do, or its place when several are picked). FireRed's message
 * under them in the dialogue frame, CANCEL beside it. FireRed's arrow marks the card or button held under a touch,
 * else the Party's own cursor. A menu is FireRed's window at the bottom right; a message that waits for A, and a Rare
 * Candy's level up with its stats' window, take the whole band. Included by 3ds_battle_ui_impl.h after the battle
 * pages. */

/* An ailment's tag colour and code, by GetMonAilment's number; 0 is FNT (a fainted one). */
static const uint16_t kPPageAilment[7] = {UI_RGB(90, 80, 90), UI_RGB(170, 80, 190), UI_RGB(214, 170, 30),
                                          UI_RGB(120, 128, 150), UI_RGB(80, 170, 220), UI_RGB(226, 96, 48),
                                          UI_RGB(200, 90, 160)};
static const char *const kPPageAilmentCode[7] = {CTR_TR("FNT", "DEB", "Bes"), CTR_TR("PSN", "ENV", "Gft"),
                                                 CTR_TR("PAR", "PAR", "Par"), CTR_TR("SLP", "DOR", "Slf"),
                                                 CTR_TR("FRZ", "CON", "Efr"), CTR_TR("BRN", "QUE", "Brn"), "PKRS"};
/* The gender signs (FireRed's charmap ♂ and ♀) and their colours, as FireRed's windows print them. */
static const uint16_t kPPageMale = UI_RGB(56, 104, 216), kPPageFemale = UI_RGB(224, 72, 64);
#define PPAGE_MALE "\x01\xB5"
#define PPAGE_FEMALE "\x01\xB6"
/* A card's lines: the name at y + 8, the HP bar at y + 26, Lv and the HP numbers at y + 31; the icon at (x + 16,
 * y + 12), the arrow left of it, the text from x + 52. */
#define PPAGE_TEXT_X 52
#define PPAGE_LINE3 31

/* A touch held on a card, CANCEL or CONFIRM: the arrow goes with it (a menu row's press moves only the menu's). */
static int PPagePressing(const CtrBattleView *v)
{
    return (v->pressed >= BUI_PTY_SLOT && v->pressed < BUI_PTY_SLOT + 6) || v->pressed == BUI_PTY_CANCEL
        || v->pressed == BUI_PTY_CONFIRM;
}

static int PPageCardChosen(const CtrBattleView *v, int i)
{
    return PPagePressing(v) ? v->pressed == BUI_PTY_SLOT + i : v->pty.cursor == i;
}

/* CANCEL (button 1) or CONFIRM (0): the one held, else the one the Party's cursor is on. */
static int PPageButtonChosen(const CtrBattleView *v, int button)
{
    return PPagePressing(v) ? v->pressed == (button ? BUI_PTY_CANCEL : BUI_PTY_CONFIRM) : v->pty.button == button;
}

static int PPageMenuOpen(const CtrBattleParty *p)
{
    return (p->panel == BUI_PTY_ACTIONS || p->panel == BUI_PTY_YESNO) && p->menuCount > 0;
}

static int PPageMenuRows(const CtrBattleParty *p)
{
    return p->menuCount > PARTY_MENU_ROWS ? PARTY_MENU_ROWS : p->menuCount;
}

static int PPageMenuChosen(const CtrBattleView *v, int i)
{
    int pressing = v->pressed >= BUI_PTY_MENU && v->pressed < BUI_PTY_MENU + PARTY_MENU_ROWS;

    return pressing ? v->pressed == BUI_PTY_MENU + i : v->pty.menuCursor == i;
}

/* A message that waits for A (alone, or with a Rare Candy's stats): the band takes the whole width, no buttons. */
static int PPageWaiting(const CtrBattleParty *p)
{
    return p->panel == BUI_PTY_TEXT || p->panel == BUI_PTY_LEVELUP;
}

/* A card toned: the item or move cannot be used on it, or it cannot be picked. */
static int PPageBarred(const CtrBattleParty *p, int i)
{
    return (p->mode == BUI_PTY_MULTI && p->entry[i] == BUI_PTY_NOT_ABLE)
        || (p->mode != BUI_PTY_MULTI && (p->tag[i] == BUI_PTY_TAG_NOT_ABLE || p->tag[i] == BUI_PTY_TAG_NO_USE));
}

/* A small flat tag with white text on lineTop's line: an ailment's code. */
static void PPageTag(int x, int lineTop, const char *text, uint16_t colour)
{
    int w = UiTextWidth(text, 1) + 8;

    Fill(x, lineTop + 2, w, KIT_LINE_H - 3, colour);
    UiText(text, x + 4, lineTop + 3, RGB(255, 255, 255), KitInk(2), 1, x + w);
}

/* The slot's party icon at (x, y), the frame the Party shows, between rows top and bottom (its hop stays in the card);
 * toned when fainted or barred. Without the icon, the port's ball mark. */
static void PPageIcon(const CtrBattleParty *p, int i, int x, int y, int toned, int top, int bottom)
{
    if (!p->icon[i] || !BPartyIcon(i, p->icon[i])) {
        BBall(x + 16, y + 16, toned ? 2 : 1, 0);
        return;
    }
    for (int py = 0; py < 32; ++py)
        for (int px = 0; px < 32; ++px) {
            uint8_t index = sBPIcon[i].pixels[(py + (p->iconFrame[i] & 1) * 32) * 32 + px];

            if (index && y + py >= top && y + py < bottom)
                Px(x + px, y + py, Tone(sBPIcon[i].pal[index], !toned));
        }
}

/* What the card's third line says at its right: its place when several are picked, the verdict on an item or a move,
 * else its HP. */
static void PPageCardRight(const CtrBattleParty *p, int i, char *out, size_t size)
{
    static const char *const kPlace[3] = {CTR_TR("1st", "1.\xC2\xBA", "1."), CTR_TR("2nd", "2.\xC2\xBA", "2."),
                                          CTR_TR("3rd", "3.\xC2\xBA", "3.")};
    int place = p->entry[i] - BUI_PTY_FIRST;

    if (p->mode == BUI_PTY_MULTI && p->entry[i] >= BUI_PTY_FIRST)
        snprintf(out, size, "%s", place < 3 ? kPlace[place] : "");
    else if (p->mode != BUI_PTY_MULTI && p->tag[i] > BUI_PTY_TAG_NONE && p->tag[i] <= BUI_PTY_TAG_NO_USE)
        snprintf(out, size, "%s", sBPartyTagText[p->tag[i]]);
    else
        snprintf(out, size, "%u/%u", p->hp[i], p->maxHp[i]);
}

static void PPageCard(const CtrBattleView *v, int i)
{
    const CtrBattleParty *p = &v->pty;
    const CtrPokeTouchRect *r = &kPartySlot[i];
    int x = r->x + PPAGE_TEXT_X, right = r->x + r->w - 8, line3 = r->y + PPAGE_LINE3;
    int fainted = !p->egg[i] && p->hp[i] == 0, on = !PPageBarred(p, i);
    char text[24];

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    if (!p->present[i])
        return;
    if (v->pressed == BUI_PTY_SLOT + i)
        Fill(r->x + 8, r->y + 8, r->w - 16, r->h - 16, KitInk(3));
    if (PPageCardChosen(v, i))
        KitText(KIT_ARROW, r->x + 8, r->y + 21, r->x + 16);
    PPageIcon(p, i, r->x + 16, r->y + 12 + BPartyBounce(p, i), fainted || !on, r->y + 8, r->y + r->h - 8);
    {
        const char *sign = p->egg[i] ? NULL : p->gender[i] == 0 ? PPAGE_MALE : p->gender[i] == 1 ? PPAGE_FEMALE : NULL;
        int limit = right - (sign ? UiTextWidth(sign, 1) + 2 : 0), nameEnd = x + UiTextWidth(p->name[i], 1);

        KitTextOn(p->name[i], x, r->y + 8, limit, on);
        if (sign) {
            uint16_t c = p->gender[i] == 0 ? kPPageMale : kPPageFemale;

            UiText(sign, (nameEnd < limit ? nameEnd : limit) + 2, r->y + 8 + 3, Tone(c, on), Tone(KitInk(3), on), 1,
                   right);
        }
    }
    if (p->egg[i])
        return;
    KitHpBar(x, r->y + 26, right - x, p->hp[i], p->maxHp[i]);
    if (fainted || (p->status[i] >= 1 && p->status[i] <= 5)) {
        int ailment = fainted ? 0 : p->status[i];

        PPageTag(x, line3, kPPageAilmentCode[ailment], kPPageAilment[ailment]);
    } else {
        snprintf(text, sizeof(text), "Lv%u", p->level[i]);   /* PKRS shows the level, as FireRed's Party does */
        KitTextOn(text, x, line3, right, on);
    }
    PPageCardRight(p, i, text, sizeof(text));
    KitTextOn(text, right - UiTextWidth(text, 1), line3, right, on);
}

/* CANCEL or CONFIRM: a framed cell with its label as a row, the arrow when chosen. */
static void PPageButton(const CtrBattleView *v, const CtrPokeTouchRect *r, const char *label, int chosen)
{
    CtrPokeTouchRect in = BPageInside(r);

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    KitRow(&in, label, chosen, 1);
}

/* The buttons the Party has now: none while a menu or a message has the band, CONFIRM and CANCEL picking several,
 * CANCEL unless a Pokemon must be sent out. */
static void PPageButtons(const CtrBattleView *v)
{
    const CtrBattleParty *p = &v->pty;

    if (PPageWaiting(p) || PPageMenuOpen(p))
        return;
    if (p->mode == BUI_PTY_MULTI)
        PPageButton(v, &kPartyConfirm, CtrUiText(UIS_PTY_CONFIRM), PPageButtonChosen(v, 0));
    if (p->mode != BUI_PTY_FORCED)
        PPageButton(v, &kPartyCancel, CTR_TR("CANCEL", "CANCELAR", "Zur\xC3\xBC" "ck"), PPageButtonChosen(v, 1));
}

static const CtrPokeTouchRect *PPageNoteRect(const CtrBattleParty *p)
{
    if (PPageWaiting(p))
        return &kPartyNoteWide;
    if (PPageMenuOpen(p))
        return &kPartyNoteMenu;
    return p->mode == BUI_PTY_MULTI ? &kPartyNoteMulti : &kPartyNote;
}

/* FireRed's message in the dialogue frame: the one that waits for A (with its continue mark), else what the Party says
 * (its own prompt in the field; in battle, what the Party is for). */
static void PPageNote(const CtrBattleView *v)
{
    const CtrBattleParty *p = &v->pty;
    const CtrPokeTouchRect *n = PPageNoteRect(p);
    const char *text = PPageWaiting(p) ? p->message
                     : p->caption[0] ? p->caption
                     : p->mode == BUI_PTY_FORCED ? CTR_TR("A POK\xC3\xA9MON must be sent out.", "Tienes que sacar un POK\xC3\xA9MON.", "Ein Pok\xC3\xA9mon muss antreten.")
                     : p->mode == BUI_PTY_TARGET ? CTR_TR("Use on which one?", "\xC2\xBF" "En cu\xC3\xA1l lo usas?", "Auf welches anwenden?")
                     : CTR_TR("Choose a POK\xC3\xA9MON.", "Elige un POK\xC3\xA9MON.", "W\xC3\xA4hle ein Pok\xC3\xA9mon.");

    Fill(kPartyNoteWide.x, kPartyNoteWide.y, kPartyNoteWide.w, kPartyNoteWide.h, BPageBg());
    KitPanel(n, KIT_FRAME_DIALOGUE, 0);
    KitWrap(text, n->x + 16, n->y + 10, n->w - 32, 2);
    if (PPageWaiting(p)) {
        int cx = n->x + n->w - 24, cy = n->y + n->h - 15;

        for (int row = 0; row < 4; ++row)
            Fill(cx + row, cy + row, 7 - 2 * row, 1, KitInk(2));
    }
}

/* FireRed's menu (the actions, YES / NO): its window at the bottom right, one row each, the arrow on the chosen one. */
static void PPageMenuRow(const CtrBattleView *v, int i)
{
    CtrPokeTouchRect r = PartyMenuRow(PPageMenuRows(&v->pty), i);

    KitRow(&r, v->pty.menuName[i], PPageMenuChosen(v, i), 1);
}

static void PPageMenu(const CtrBattleView *v)
{
    int n = PPageMenuRows(&v->pty);
    CtrPokeTouchRect w = PartyMenuWindow(n);

    Fill(w.x, w.y, w.w, w.h, BPageBg());
    KitPanel(&w, KIT_FRAME_STD, v->frameType);
    for (int i = 0; i < n; ++i)
        PPageMenuRow(v, i);
}

/* A Rare Candy's level up: the six stats in the Summary's order, each with its new value and what it gained, in a
 * window over the right column's first two cards (where FireRed shows them). */
static void PPageLevelUp(const CtrBattleView *v)
{
    static const char *const kLabel[6] = {CTR_TR("MAX HP", "PS M\xC3\x81X.", "Max. KP"), CTR_TR("ATTACK", "ATAQUE", "Angriff"),
                                          CTR_TR("DEFENSE", "DEFENSA", "Vert."), CTR_TR("SP. ATK", "AT. ESP.", "Sp. Ang."),
                                          CTR_TR("SP. DEF", "DEF. ESP.", "Sp. Ver."), CTR_TR("SPEED", "VELOCIDAD", "Init.")};
    const CtrPokeTouchRect *r = &kPartyLevelUp;
    int x = r->x + 8, right = r->x + r->w - 8, valueRight = right - 28, top = r->y + (r->h - 6 * 16) / 2;

    Fill(r->x, r->y, r->w, r->h, BPageBg());
    KitPanel(r, KIT_FRAME_STD, v->frameType);
    for (int i = 0; i < 6; ++i) {
        int line = top + 16 * i + 1;
        char value[8], gain[8];

        snprintf(value, sizeof(value), "%u", v->pty.lvStat[i]);
        snprintf(gain, sizeof(gain), "%+d", v->pty.lvGain[i]);
        KitText(kLabel[i], x, line, valueRight - UiTextWidth(value, 1) - 4);
        KitText(value, valueRight - UiTextWidth(value, 1), line, valueRight);
        UiText(gain, right - UiTextWidth(gain, 1), line + 3, UI_RGB(32, 144, 72), UI_RGB(196, 236, 204), 1, right);
    }
}

static void PPageScreen(const CtrBattleView *v)
{
    Fill(0, 0, W, H, BPageBg());
    for (int i = 0; i < 6; ++i)
        PPageCard(v, i);
    PPageNote(v);
    PPageButtons(v);
    if (PPageMenuOpen(&v->pty))
        PPageMenu(v);
    if (v->pty.panel == BUI_PTY_LEVELUP)
        PPageLevelUp(v);
}

static int PPageHit(const CtrBattleView *v, int x, int y)
{
    const CtrBattleParty *p = &v->pty;

    if (PPageWaiting(p))
        return BUI_PTY_MESSAGE;                         /* anywhere: the message waits for A */
    if (PPageMenuOpen(p)) {
        int n = PPageMenuRows(p);

        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r = PartyMenuRow(n, i);

            if (UiIn(&r, x, y))
                return BUI_PTY_MENU + i;
        }
        return -1;
    }
    if (p->mode == BUI_PTY_MULTI && UiIn(&kPartyConfirm, x, y))
        return BUI_PTY_CONFIRM;
    if (p->mode != BUI_PTY_FORCED && UiIn(&kPartyCancel, x, y))
        return BUI_PTY_CANCEL;
    for (int i = 0; i < 6; ++i)
        if (p->present[i] && UiIn(&kPartySlot[i], x, y))
            return BUI_PTY_SLOT + i;
    return -1;
}

static int PPageOverlap(const CtrPokeTouchRect *a, const CtrPokeTouchRect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

/* What a card's picture depends on beyond the Party's data: chosen, pressed, the icon's frame and hop. */
static int PPageCardLook(const CtrBattleView *v, int i)
{
    return PPageCardChosen(v, i) | (v->pressed == BUI_PTY_SLOT + i) << 1 | v->pty.iconFrame[i] << 2;
}

/* A move of the selection, a press, a menu's cursor or an icon's frame redraws what it changed (and a window over a
 * card drawn again); anything else, the page. */
static void PPageUpdate(const CtrBattleView *old, const CtrBattleView *v, CtrDirtyList *dirty)
{
    CtrBattleView a = *old;
    const CtrBattleParty *p = &v->pty;
    CtrPokeTouchRect all = {0, 0, W, H};
    int menu = PPageMenuOpen(p), level = p->panel == BUI_PTY_LEVELUP, coverMenu = 0, coverLevel = 0;
    CtrPokeTouchRect window = PartyMenuWindow(menu ? PPageMenuRows(p) : 1);

    a.pressed = v->pressed;
    a.pty.cursor = p->cursor;
    a.pty.button = p->button;
    a.pty.menuCursor = p->menuCursor;
    memcpy(a.pty.iconFrame, p->iconFrame, sizeof(a.pty.iconFrame));
    if (memcmp(&a, v, sizeof(a))) {
        PPageScreen(v);
        BPageMark(dirty, &all);
        return;
    }
    for (int i = 0; i < 6; ++i)
        if (PPageCardLook(old, i) != PPageCardLook(v, i)) {
            PPageCard(v, i);
            BPageMark(dirty, &kPartySlot[i]);
            coverMenu |= menu && PPageOverlap(&kPartySlot[i], &window);
            coverLevel |= level && PPageOverlap(&kPartySlot[i], &kPartyLevelUp);
        }
    if (!PPageWaiting(p) && !menu) {
        if (p->mode == BUI_PTY_MULTI && PPageButtonChosen(old, 0) != PPageButtonChosen(v, 0)) {
            PPageButton(v, &kPartyConfirm, CtrUiText(UIS_PTY_CONFIRM), PPageButtonChosen(v, 0));
            BPageMark(dirty, &kPartyConfirm);
        }
        if (p->mode != BUI_PTY_FORCED && PPageButtonChosen(old, 1) != PPageButtonChosen(v, 1)) {
            PPageButton(v, &kPartyCancel, CTR_TR("CANCEL", "CANCELAR", "Zur\xC3\xBC" "ck"), PPageButtonChosen(v, 1));
            BPageMark(dirty, &kPartyCancel);
        }
    }
    if (coverMenu) {
        PPageMenu(v);
        BPageMark(dirty, &window);
    } else if (menu) {
        for (int i = 0; i < PPageMenuRows(p); ++i)
            if (PPageMenuChosen(old, i) != PPageMenuChosen(v, i)) {
                CtrPokeTouchRect r = PartyMenuRow(PPageMenuRows(p), i);

                PPageMenuRow(v, i);
                BPageMark(dirty, &r);
            }
    }
    if (coverLevel) {
        PPageLevelUp(v);
        BPageMark(dirty, &kPartyLevelUp);
    }
}
