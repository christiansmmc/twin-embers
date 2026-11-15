/* The Party as the red device (style B; boards B · POKeMON and B · POKeMON: menu de ações), the field's and the
 * battle's alike (BUI_PARTY), included by 3ds_poketouch.c after the kit and HOME and before the battle, whose
 * CtrBattleUi_Draw / Update / Hit hand it this state. Every rectangle is 3ds_device_layout.h's, so what is drawn is what
 * a touch takes.
 *
 * The six Pokemon are bays in the shell, two to a row in the game's order: a key with the icon (1:1, frame 0), the name,
 * the HP tag and bar with the numbers and, at the right, the level or what matters more (picking several its place in
 * the order, an item's or a move's verdict, a condition); a faint key when it fainted or may not enter; an egg shows its
 * icon and name only; a slot without a Pokemon is a hole. Under them the screen says what the Party says (its own
 * prompt, or the mode's words) and CANCEL, a dark key, leaves (grey and not taken when a Pokemon must be sent out);
 * picking several, CONFIRM over a shorter CANCEL. A message is on the screen with FireRed's arrow, the keys grey: a tap
 * anywhere goes on.
 * The action menu (and YES / NO): the bays become a column of slim ones and the actions keys beside them, one column of
 * up to four, two beyond with the slim bays narrowed (the game's D-pad walks the same grid, 3ds_party_grid.h); a field
 * move's name in blue, CANCEL dark; the menu's screen under them with the Party's prompt. A Rare Candy's level up: the
 * new level and the six stats with their gains on a tall screen, the message under it, OK.
 *
 * The chosen key is the one under a held touch, else the game's cursor: a slot, CONFIRM or CANCEL (FireRed's slots 6
 * and 7), a menu row. No screws. Words: upper case (decision 5): the menu's names through DevUpper, a Pokemon's name in
 * capitals when it is its species' own (the fill compares it, as the battle's does), as typed when the player gave it;
 * FireRed's prompts and messages as they are; the port's words written in capitals.
 *
 * Redraw (decision 7): a change of anything drawn redraws the screen whole; what is not drawn (the icons' frames: they
 * are not animated here; the battle's own fields; what the open panel does not show: DevPartyUndrawn) is copied over
 * before the comparison. */

#define DEV_PTY_GREY      UI_RGB(0x8A, 0x8A, 0x8A)  /* a faint bay's HP tag and bar frame, and the tag's ink */
#define DEV_PTY_GREY_INK  UI_RGB(0xE0, 0xE0, 0xE0)
#define DEV_PTY_ORDER     DEV_BLUE_INK              /* picking several: a Pokemon's place in the order */
#define DEV_PTY_GAIN      DEV_FX_SUPER              /* the level up's gains */
#define DEV_PTY_BAR_W     56                        /* a bay's HP bar (the board's), less what wide numbers need */
#define DEV_PTY_SLIM_BAR  72                        /* a slim bay's */

/* Words at a bay's right: v0.6.0's (its party cards'), in capitals. The conditions are GetMonAilment's 1-6. */
static const char *const kDevPtyStatus[7] = {
    "", CTR_TR("PSN", "ENV", "GFT"), CTR_TR("PAR", "PAR", "PAR"), CTR_TR("SLP", "DOR", "SLF"), CTR_TR("FRZ", "CON", "EFR"),
    CTR_TR("BRN", "QUE", "BRN"), CTR_TR("PKRS", "PKRS", "PKRS"),
};
static const char *const kDevPtyTag[5] = {
    "", CTR_TR("ABLE", "PUEDE", "KANN"), CTR_TR("NOT ABLE", "NO PUEDE", "NEIN"), CTR_TR("LEARNED", "APRENDIDO", "KENNT ATT."),
    CTR_TR("NO USE", "NO SIRVE", "NUTZLOS"),
};

/* The menu's rows shown (at most the grid's eight). */
static int DevPtyMenuCount(const CtrBattleParty *p)
{
    return CtrPartyGrid_Count(p->menuCount);
}

static int DevPtyMenuOpen(const CtrBattleParty *p)
{
    return p->panel == BUI_PTY_ACTIONS || p->panel == BUI_PTY_YESNO;
}

/* The key the game's cursor is on (a hit code): CANCEL or, picking several, CONFIRM when FireRed's cursor is past the
 * slots; else its slot; -1 for none. */
static int DevPtyCursor(const CtrBattleParty *p)
{
    if (p->button == 0 && p->mode == BUI_PTY_MULTI)
        return BUI_PTY_CONFIRM;
    if (p->button == 1)
        return BUI_PTY_CANCEL;
    return p->cursor >= 0 && p->cursor < 6 ? BUI_PTY_SLOT + p->cursor : -1;
}

/* ---- Where a touch goes --------------------------------------------------------------------------------------------- */

static int DevPartyHit(const CtrBattleView *v, int x, int y)
{
    const CtrBattleParty *p = &v->pty;
    CtrPokeTouchRect a;

    if (p->panel == BUI_PTY_TEXT || p->panel == BUI_PTY_LEVELUP)
        return BUI_PTY_MESSAGE;                             /* anywhere: the message waits for A */
    if (DevPtyMenuOpen(p)) {
        int n = DevPtyMenuCount(p);

        for (int i = 0; i < n; ++i) {
            CtrPokeTouchRect r = DevPtyMenuKey(n, i);

            a = DevKeyArea(&r);
            if (UiIn(&a, x, y))
                return BUI_PTY_MENU + i;
        }
        return -1;                                          /* the bays: not while a menu is open */
    }
    if (p->mode == BUI_PTY_MULTI) {
        a = DevKeyArea(&kDevPtyConfirm);
        if (UiIn(&a, x, y))
            return BUI_PTY_CONFIRM;
        a = DevKeyArea(&kDevPtyCancelLow);
        if (UiIn(&a, x, y))
            return BUI_PTY_CANCEL;
    } else if (p->mode != BUI_PTY_FORCED) {
        a = DevKeyArea(&kDevPtyCancel);
        if (UiIn(&a, x, y))
            return BUI_PTY_CANCEL;
    }
    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect r = DevPtyBay(i);

        a = DevKeyArea(&r);
        if (p->present[i] && UiIn(&a, x, y))
            return BUI_PTY_SLOT + i;
    }
    return -1;
}

/* ---- A Pokemon's bay -------------------------------------------------------------------------------------------------- */

/* A faint key: it fainted, or picking several it may not enter. */
static int DevPtyLook(const CtrBattleParty *p, int i)
{
    return (!p->egg[i] && p->hp[i] == 0) || (p->mode == BUI_PTY_MULTI && p->entry[i] == BUI_PTY_NOT_ABLE)
         ? DEV_LOOK_FAINT : DEV_LOOK_KEY;
}

/* Slot i's name: its species' own in capitals, a nickname as typed, an egg's ("EGG") as the game's word. */
static const char *DevPtyName(const CtrBattleParty *p, int i, char *out, size_t size)
{
    return DevMonName(p->name[i], p->nameIsSpecies[i] || p->egg[i], out, size);
}

/* An HP bar; grey (a faint key's) its frame in the faint grey. */
static void DevPtyBar(int x, int y, int w, int h, unsigned hp, unsigned max, int grey)
{
    DevHpBar(x, y, w, h, hp, max);
    if (!grey || w < 2 || h < 2)
        return;
    Fill(x, y, w, 1, DEV_PTY_GREY);
    Fill(x, y + h - 1, w, 1, DEV_PTY_GREY);
    Fill(x, y, 1, h, DEV_PTY_GREY);
    Fill(x + w - 1, y, 1, h, DEV_PTY_GREY);
}

/* "hp/max" in the small font ending at `right`, capitals from `top`: the key's ink, the low ink in the HP bar's red, a
 * faint key's grey. */
static void DevPtyNumbers(const CtrBattleParty *p, int i, int right, int top, int grey, uint16_t ink, uint16_t shadow)
{
    char text[16];

    snprintf(text, sizeof(text), "%u/%u", (unsigned)p->hp[i], (unsigned)p->maxHp[i]);
    if (grey)
        ink = shadow = DEV_FAINT_INK;
    else if (DevHpColour(p->hp[i], p->maxHp[i]) == DEV_HP_RED)
        ink = DEV_LOW_INK;
    DevGlyphs(text, right - DevSmallTextWidth(text), top - 4, right, ink, shadow, 1);
}

/* What a bay shows at its right, ending at `right`: picking several, a picked Pokemon's place in the order; else the
 * item's or the move's verdict (FireRed's own box text); else FNT or a condition; else the level. A chip at `chipY`,
 * chipH tall, or the level's capitals at `lvTop`. */
static void DevPtyRight(const CtrBattleParty *p, int i, int right, int chipY, int chipH, int lvTop)
{
    static const uint16_t kTagBg[5] = {0, UI_RGB(56, 168, 88), UI_RGB(112, 112, 128), UI_RGB(70, 132, 214),
                                       UI_RGB(112, 112, 128)};
    static const uint16_t kStatusBg[7] = {0, DEV_PSN, UI_RGB(216, 168, 24), UI_RGB(120, 128, 150), UI_RGB(80, 170, 220),
                                          UI_RGB(226, 96, 48), UI_RGB(200, 90, 160)};
    int multi = p->mode == BUI_PTY_MULTI, w;
    const char *word = NULL;
    uint16_t bg = 0, shadow = DEV_WHITE;
    char text[12];

    if (multi && p->entry[i] >= BUI_PTY_FIRST) {
        int place = p->entry[i] - BUI_PTY_FIRST + 1;

        text[0] = (char)('0' + (place > 9 ? 9 : place));
        text[1] = 0;
        word = text;
        bg = DEV_PTY_ORDER;
        shadow = DevChipShadow(bg);
    } else if (!multi && p->tag[i] > BUI_PTY_TAG_NONE && p->tag[i] <= BUI_PTY_TAG_NO_USE) {
        word = kDevPtyTag[p->tag[i]];
        bg = kTagBg[p->tag[i]];
        shadow = DevChipShadow(bg);
    } else if (p->hp[i] == 0) {
        word = CTR_TR("FNT", "DEB", "BES");
        bg = DEV_FNT;                                       /* PSN and FNT: the board's, no shadow */
    } else if (p->status[i] >= 1 && p->status[i] <= 6) {
        word = kDevPtyStatus[p->status[i]];
        bg = kStatusBg[p->status[i]];
        shadow = p->status[i] == 1 ? DEV_WHITE : DevChipShadow(bg);
    }
    if (word) {
        w = DevSmallTextWidth(word) + 6;                    /* the board's 3 px each side */
        DevChip(right - w, chipY, w, chipH, word, bg, shadow);
        return;
    }
    snprintf(text, sizeof(text), CTR_TR("Lv%u", "Nv%u", "Lv%u"), (unsigned)p->level[i]);
    DevSmallText(text, right - DevSmallTextWidth(text), lvTop, DEV_INK2, right);
}

/* Slot i's bay (a key, sunk when chosen): the icon at 1:1, the name, the HP tag and bar and the numbers over the bar's
 * end, the right column's level or chip. The bar is the board's 56 px, less what FireRed's widest numbers ("999/999")
 * need with 3 px to spare: its small digits are wider than the board's font, and every bay's bar is the same. */
static void DevPtyBayKey(const CtrBattleParty *p, int i, int sunk)
{
    CtrPokeTouchRect r = DevPtyBay(i);
    int look = DevPtyLook(p, i), grey = look == DEV_LOOK_FAINT, faceY = DevKey(&r, look, sunk, 6), right = r.x + r.w - 8;
    uint16_t ink = DevKeyInk(look, sunk), shadow = DevKeyInkShadow(look, sunk);
    char name[24];
    int tag, barX, barW = DEV_PTY_BAR_W;

    DevMonIcon(i, p->icon[i], r.x + 4, faceY + 9, 0, grey);
    DevTextInk(DevPtyName(p, i, name, sizeof(name)), r.x + 38, faceY + 11, r.x + r.w - 14, ink, shadow);
    if (p->egg[i])
        return;
    tag = DevLabelChip(r.x + 38, faceY + 29, "HP", grey ? DEV_PTY_GREY : DEV_HP_FRAME, grey ? DEV_PTY_GREY_INK : DEV_HP_LABEL);
    barX = r.x + 38 + tag + 2;
    if (barX + barW + 3 > right - DevSmallTextWidth("999/999"))
        barW = right - DevSmallTextWidth("999/999") - 3 - barX;
    DevPtyBar(barX, faceY + 31, barW, 5, p->hp[i], p->maxHp[i], grey);
    DevPtyNumbers(p, i, right, faceY + 28, grey, ink, shadow);
    DevPtyRight(p, i, right, faceY + 38, 10, faceY + 40);
}

/* Slot i's slim bay beside a menu of n rows (sunk when it is the Party's selection; not a key a touch takes): the icon
 * at half size, the name, a 72 px bar; the level (or its chip) and the numbers only beside one column, 188 wide, ending
 * 14 px in (the board's 8 would put the level under the LED of the sunk one). */
static void DevPtySlimKey(const CtrBattleParty *p, int n, int i)
{
    CtrPokeTouchRect r = DevPtySlim(n, i);
    int look, grey, sunk = p->cursor == i, faceY, wide = r.w >= 188;
    uint16_t ink, shadow;
    char name[24];

    if (!p->present[i]) {
        DevHole(&r, 5);
        return;
    }
    look = DevPtyLook(p, i);
    grey = look == DEV_LOOK_FAINT;
    ink = DevKeyInk(look, sunk);
    shadow = DevKeyInkShadow(look, sunk);
    faceY = DevKey(&r, look, sunk, 5);
    DevMonIcon(i, p->icon[i], r.x + 6, faceY + 4, 1, grey);
    DevTextInk(DevPtyName(p, i, name, sizeof(name)), r.x + 29, faceY + 4, wide ? r.x + 140 : r.x + r.w - 14, ink, shadow);
    if (p->egg[i])
        return;
    DevPtyBar(r.x + 29, faceY + 15, DEV_PTY_SLIM_BAR, 4, p->hp[i], p->maxHp[i], grey);
    if (!wide)
        return;
    DevPtyRight(p, i, r.x + r.w - 14, faceY + 2, 9, faceY + 3);
    DevPtyNumbers(p, i, r.x + r.w - 14, faceY + 13, grey, ink, shadow);
}

/* ---- The screens ------------------------------------------------------------------------------------------------------ */

/* `text` on the screen in `outer`: from 8 px into the paper, wrapped in three lines at most, the lines centred down the
 * paper; `more`: FireRed's arrow at its foot (a tap goes on). */
static void DevPtyScreen(const CtrPokeTouchRect *outer, const char *text, int more)
{
    CtrPokeTouchRect paper = DevScreen(outer);
    int width = paper.w - 16 - (more ? 12 : 0), lines = DevWrapLines(text, width, 3);

    if (lines > 0)
        DevWrap(text, paper.x + 8, paper.y + (paper.h - (8 + DEV_LINE_H * (lines - 1))) / 2, width, 3, DEV_INK, DEV_INK_SH);
    if (more)
        DevTriangle(paper.x + paper.w - 10, paper.y + paper.h - 8, 1, DEV_INK2);
}

/* What the Party says under its bays when it has no prompt of its own (a battle's): v0.6.0's words for the mode. */
static const char *DevPtyModeWords(const CtrBattleParty *p)
{
    if (p->mode == BUI_PTY_FORCED)
        return CTR_TR("A POK\xC3\xA9MON must be sent out.", "Tienes que sacar un POK\xC3\xA9MON.", "Ein POK\xC3\xA9MON muss antreten.");
    if (p->mode == BUI_PTY_TARGET)
        return CTR_TR("Use on which one?", "\xC2\xBF" "En cu\xC3\xA1l lo usas?", "Auf welches anwenden?");
    return CTR_TR("Choose a POK\xC3\xA9MON.", "Elige un POK\xC3\xA9MON.", "W\xC3\xA4hle ein POK\xC3\xA9MON.");
}

/* CANCEL (and CONFIRM picking several) under the bays: `cursor` and `pressed` choose the sunk one; `dim` greys them.
 * When a Pokemon must be sent out CANCEL is grey (DevPartyHit does not take it), but FireRed's cursor can still go
 * to it (SEND OUT then fails), and then it is the chosen key all the same. */
static void DevPtyLeaveKeys(const CtrBattleParty *p, int cursor, int pressed, int dim)
{
    const char *cancel = CTR_TR("CANCEL", "CANCELAR", "ZUR\xC3\x9C" "CK");
    int sunk, faceY;

    if (p->mode == BUI_PTY_MULTI) {
        sunk = DevChosen(BUI_PTY_CONFIRM, cursor, pressed);
        faceY = DevKey(&kDevPtyConfirm, DEV_LOOK_KEY, sunk, 6);
        DevKeyWord(&kDevPtyConfirm, faceY, DEV_LOOK_KEY, sunk, CTR_TR("CONFIRM", "CONFIRMAR", "BEST\xC3\x84TIGEN"));
        sunk = DevChosen(BUI_PTY_CANCEL, cursor, pressed);
        faceY = DevKey(&kDevPtyCancelLow, DEV_LOOK_DARK, sunk, 6);
        DevKeyWord(&kDevPtyCancelLow, faceY, DEV_LOOK_DARK, sunk, cancel);
        if (dim) {
            DevDim(&kDevPtyConfirm);
            DevDim(&kDevPtyCancelLow);
        }
    } else {
        sunk = DevChosen(BUI_PTY_CANCEL, cursor, pressed);
        faceY = DevKey(&kDevPtyCancel, DEV_LOOK_DARK, sunk, 6);
        DevKeyWord(&kDevPtyCancel, faceY, DEV_LOOK_DARK, sunk, cancel);
        if (dim || p->mode == BUI_PTY_FORCED)
            DevDim(&kDevPtyCancel);
    }
}

/* The bays, the screen and CANCEL; with a message (BUI_PTY_TEXT) the message on the screen, every key grey and none
 * chosen. */
static void DevPtyBays(const CtrBattleView *v)
{
    const CtrBattleParty *p = &v->pty;
    int message = p->panel == BUI_PTY_TEXT, cursor = message ? -1 : DevPtyCursor(p), pressed = message ? -1 : v->pressed;

    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect r = DevPtyBay(i);

        if (!p->present[i]) {
            DevHole(&r, 6);
            continue;
        }
        DevPtyBayKey(p, i, DevChosen(BUI_PTY_SLOT + i, cursor, pressed));
        if (message)
            DevDim(&r);
    }
    DevPtyScreen(&kDevPtyNote, message ? p->message : p->caption[0] ? p->caption : DevPtyModeWords(p), message);
    DevPtyLeaveKeys(p, cursor, pressed, message);
}

/* The action menu or YES / NO: the slim bays, the rows' keys (a field move's name in blue, CANCEL dark), the menu's
 * screen with the Party's prompt (a battle's has none: FireRed's question). */
static void DevPtyMenu(const CtrBattleView *v)
{
    const CtrBattleParty *p = &v->pty;
    int n = DevPtyMenuCount(p);
    char word[32];

    for (int i = 0; i < 6; ++i)
        DevPtySlimKey(p, n, i);
    for (int i = 0; i < n; ++i) {
        CtrPokeTouchRect r = DevPtyMenuKey(n, i);
        int dark = (p->menuDark >> i) & 1, look = dark ? DEV_LOOK_DARK : DEV_LOOK_KEY;
        int sunk = DevChosen(i, p->menuCursor, v->pressed - BUI_PTY_MENU), faceY = DevKey(&r, look, sunk, 5);

        DevUpper(word, p->menuName[i], sizeof(word));
        if (!dark && ((p->menuBlue >> i) & 1))
            DevKeyWordInk(&r, faceY, sunk, word, DEV_BLUE_INK, DevKeyInkShadow(look, sunk));
        else
            DevKeyWord(&r, faceY, look, sunk, word);
    }
    DevPtyScreen(&kDevPtyMenuNote, p->caption[0] || p->panel == BUI_PTY_YESNO ? p->caption
                 : CTR_TR("Do what with this POK\xC3\xA9MON?", "\xC2\xBF" "Qu\xC3\xA9 hacer con este POK\xC3\xA9MON?",
                          "Was tun mit diesem POK\xC3\xA9MON?"), 0);
}

/* A Rare Candy's level up: on the tall screen the Pokemon's name and new level, the six stats (two columns, in the
 * Summary's order) with their new values and gains; the message on the screen under it; OK, the key that goes on. */
static void DevPtyLevelUp(const CtrBattleView *v)
{
    static const char *const kLabel[6] = {
        CTR_TR("MAX HP", "PS M\xC3\x81X.", "MAX. KP"), CTR_TR("ATTACK", "ATAQUE", "ANGRIFF"), CTR_TR("DEFENSE", "DEFENSA", "VERT."),
        CTR_TR("SP. ATK", "AT. ESP.", "SP. ANG."), CTR_TR("SP. DEF", "DEF. ESP.", "SP. VER."), CTR_TR("SPEED", "VELOCIDAD", "INIT."),
    };
    const CtrBattleParty *p = &v->pty;
    int slot = p->lvSlot < 6 ? p->lvSlot : 0, faceY;
    CtrPokeTouchRect paper = DevScreen(&kDevPtyLevelScreen);
    char text[24];

    snprintf(text, sizeof(text), CTR_TR("Lv%u", "Nv%u", "Lv%u"), (unsigned)p->lvLevel);
    DevText(text, paper.x + paper.w - 8 - DevTextWidth(text), paper.y + 12, paper.x + paper.w - 8);
    DevText(DevPtyName(p, slot, text, sizeof(text)), paper.x + 8, paper.y + 12, paper.x + paper.w - 60);
    for (int k = 0; k < 6; ++k) {
        int x = paper.x + 8 + 142 * (k & 1), y = paper.y + 36 + 36 * (k >> 1), right;

        Fill(x, y, 134, 28, DEV_PAPER2);
        DevText2(kLabel[k], x + 6, y + 10, x + 78);
        snprintf(text, sizeof(text), "%u", (unsigned)p->lvStat[k]);
        right = x + 98;
        DevText(text, right - DevTextWidth(text), y + 10, right);
        snprintf(text, sizeof(text), "+%d", (int)p->lvGain[k]);
        right = x + 128;
        DevTextInk(text, right - DevTextWidth(text), y + 10, right, DEV_PTY_GAIN, DEV_PTY_GAIN);
    }
    DevPtyScreen(&kDevPtyNote, p->message, 0);
    faceY = DevKey(&kDevPtyCancel, DEV_LOOK_KEY, 1, 6);
    DevKeyWord(&kDevPtyCancel, faceY, DEV_LOOK_KEY, 1, CTR_TR("OK", "OK", "OK"));
}

/* The Party, whole (canvas and shell already set by CtrBattleUi_Draw). */
static void DevPartyDraw(const CtrBattleView *v)
{
    DevShell(0);
    if (v->pty.panel == BUI_PTY_LEVELUP)
        DevPtyLevelUp(v);
    else if (DevPtyMenuOpen(&v->pty))
        DevPtyMenu(v);
    else
        DevPtyBays(v);
}

/* What the Party does not draw, from v into `still` (a copy of the view drawn last), so that only a change of what it
 * draws redraws (the plan's rule): the battle's own fields; the icons' frames and the column layout; the level up's
 * fields but in it, and in it nothing else of the Party but the Pokemon's name; the menu's fields but with a menu open,
 * and its rows past the open menu's; the message but with one; a touch and the cursor with a message (none is chosen) and
 * a touch in the level up (OK is always the key); the prompt under a message; the order picking several, the verdicts
 * otherwise. The state, the shell and the touch otherwise are compared. */
static void DevPartyUndrawn(CtrBattleView *still, const CtrBattleView *v)
{
    const CtrBattleParty *p = &v->pty;
    CtrBattleParty drawn = still->pty, *s;
    uint8_t state = still->state, shell = still->shell;
    int16_t pressed = still->pressed;
    int n = DevPtyMenuCount(p), low = (1 << n) - 1;

    *still = *v;
    still->state = state;
    still->shell = shell;
    still->pressed = pressed;
    s = &still->pty;
    if (p->panel == BUI_PTY_LEVELUP) {
        int slot = p->lvSlot < 6 ? p->lvSlot : 0;

        still->pressed = v->pressed;
        s->panel = drawn.panel;
        s->lvSlot = drawn.lvSlot;
        s->lvLevel = drawn.lvLevel;
        memcpy(s->lvStat, drawn.lvStat, sizeof(s->lvStat));
        memcpy(s->lvGain, drawn.lvGain, sizeof(s->lvGain));
        memcpy(s->message, drawn.message, sizeof(s->message));
        memcpy(s->name[slot], drawn.name[slot], sizeof(s->name[slot]));
        s->nameIsSpecies[slot] = drawn.nameIsSpecies[slot];
        s->egg[slot] = drawn.egg[slot];
        return;
    }
    *s = drawn;
    memcpy(s->iconFrame, p->iconFrame, sizeof(s->iconFrame));
    s->lead = p->lead;
    s->lvSlot = p->lvSlot;
    s->lvLevel = p->lvLevel;
    memcpy(s->lvStat, p->lvStat, sizeof(s->lvStat));
    memcpy(s->lvGain, p->lvGain, sizeof(s->lvGain));
    if (DevPtyMenuOpen(p)) {
        for (int i = n; i < 8; ++i)
            memcpy(s->menuName[i], p->menuName[i], sizeof(s->menuName[i]));
        s->menuBlue = (uint8_t)((s->menuBlue & low) | (p->menuBlue & ~low));
        s->menuDark = (uint8_t)((s->menuDark & low) | (p->menuDark & ~low));
    } else {
        s->menuCount = p->menuCount;
        s->menuCursor = p->menuCursor;
        memcpy(s->menuName, p->menuName, sizeof(s->menuName));
        s->menuBlue = p->menuBlue;
        s->menuDark = p->menuDark;
    }
    if (p->panel == BUI_PTY_TEXT) {
        still->pressed = v->pressed;
        s->cursor = p->cursor;
        s->button = p->button;
        memcpy(s->caption, p->caption, sizeof(s->caption));
    } else
        memcpy(s->message, p->message, sizeof(s->message));
    if (p->mode == BUI_PTY_MULTI)
        memcpy(s->tag, p->tag, sizeof(s->tag));
    else
        memcpy(s->entry, p->entry, sizeof(s->entry));
}
