/* Part of 3ds_bottom.c, included there where it stood (not a separate translation unit; everything static): the battle
 * views' fills (the active slot, the battle Bag, the party, the battle panels) and the save view's. Split out so
 * 3ds_bottom.c stays under the source audit's per-file cap. */


/* The party slot of the Pokemon in the field. The party keeps its order in a battle (a switch changes only which
 * slot the battle points at), so the slot is found by what the battle knows of its left battler: its level, its
 * maximum HP and its name; the closest HP breaks a tie. Slot 0 when nothing matches. */
static int BattleActiveSlot(void)
{
    char name[32], mine[32];
    int level = 0, hp = 0, maxHp = 0, best = 0, bestScore = -1;

    for (int b = 0; b < CtrBattle_TargetCount() && b < 4; ++b)
        if (CtrBattle_TargetPosition(b) == 0)
            if (CtrBattle_TargetInfo(b, name, (int)sizeof(name), &level, &hp, &maxHp))
                break;
    if (!maxHp)
        return 0;
    for (int i = 0; i < 6; ++i) {
        int score, same = 0;

        if (CtrBottomParty_SlotEmpty(i) || CtrBottomParty_IsEgg(i)
            || (int)CtrBottomParty_Level(i) != level || (int)CtrBottomParty_MaxHP(i) != maxHp)
            continue;
        CtrBottomParty_Nickname(i, mine);
        same = strcmp(mine, name) == 0;
        score = 1000 + (same ? 500 : 0) - abs((int)CtrBottomParty_HP(i) - hp);
        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

/* The six ball slots and the touch held under a finger: what every battle screen shares. */
static void BattleCommon(CtrBattleView *v)
{
    memset(v, 0, sizeof(*v));
    v->pressed = (int16_t)(sBat.down && sBat.inside ? sBat.btn : -1);
    v->cursor = -1;
    PartyFill(v->party, v->partyHp, v->partyStatus, NULL);
    {                                   /* the Pokemon in the field: the action screen's footer and icon */
        int slot = BattleActiveSlot();

        if (v->party[slot]) {
            unsigned level = CtrBottomParty_Level(slot);

            CtrBottomParty_Nickname(slot, v->activeName);
            v->activeName[sizeof(v->activeName) - 1] = 0;
            v->activeLevel = (uint8_t)(level > 255 ? 255 : level);
            v->activeSlot = (uint8_t)slot;
            v->activeIcon = (uint16_t)CtrBottomParty_IconSpecies(slot);
            v->activeFrame = (uint8_t)((sBatAnim++ / 15) & 1);
            v->activeHp = (uint16_t)CtrBottomParty_HP(slot);
            {
                /* The types of the species in the field: kept while the species stays (the lookup is two Pokedex flag reads and two
                 * text conversions); only a found result is kept, so a species not yet in the Pokedex is asked again. */
                static unsigned sTypesSpecies;
                static int sTypes[2];
                static char sTypeName[2][12];
                unsigned species = CtrBottomParty_Species(slot);
                int ty[2];

                if (species != 0 && species == sTypesSpecies) {
                    ty[0] = sTypes[0];
                    ty[1] = sTypes[1];
                    memcpy(v->activeTypeName, sTypeName, sizeof(sTypeName));
                    v->activeType[0] = (int8_t)ty[0];
                    v->activeType[1] = (int8_t)ty[1];
                } else if (CtrBottomDex_Types(species, ty, v->activeTypeName[0], v->activeTypeName[1], 12)) {
                    sTypesSpecies = species;
                    sTypes[0] = ty[0];
                    sTypes[1] = ty[1];
                    memcpy(sTypeName, v->activeTypeName, sizeof(sTypeName));
                    v->activeType[0] = (int8_t)ty[0];
                    v->activeType[1] = (int8_t)ty[1];
                } else
                    v->activeType[0] = v->activeType[1] = -1;
            }
            v->activeMaxHp = (uint16_t)CtrBottomParty_MaxHP(slot);
        }
    }
}

/* The battle Bag as it is now: its pocket, its items (the save's own), the selected one's detail, the open menu or the
 * message it waits on. */
static void BattleBagFill(CtrBattleView *v)
{
    CtrBattleBag *b = &v->bag;
    int count, shown;

    BattleCommon(v);
    v->state = BUI_BAG;
    b->pocket = (uint8_t)CtrBottomBag_Pocket();
    b->bagState = (uint8_t)CtrBottomBag_State();
    b->count = (int16_t)(count = CtrBottomBag_Count());
    b->cursor = (int16_t)CtrBottomBag_Cursor();
    shown = b->cursor >= count ? count - 1 : b->cursor;
    b->first = (int16_t)(shown - 2 < 0 ? 0 : shown - 2);
    if (b->first > count - BUI_BAG_ROWS)
        b->first = (int16_t)(count - BUI_BAG_ROWS < 0 ? 0 : count - BUI_BAG_ROWS);
    for (int i = 0; i < BUI_BAG_ROWS; ++i) {
        unsigned item = CtrBottomBag_Item(b->first + i);

        b->row[i].item = (uint16_t)item;
        if (item) {
            b->row[i].qty = (uint16_t)CtrBottomBag_Quantity(b->first + i);
            CtrBottomBag_ItemName(item, b->row[i].name, sizeof(b->row[i].name));
        }
    }
    if (b->cursor >= 0 && b->cursor < count) {
        b->selItem = (uint16_t)CtrBottomBag_Item(b->cursor);
        if (b->selItem) {
            CtrBottomBag_ItemName(b->selItem, b->selName, sizeof(b->selName));
            CtrBottomBag_ItemDescription(b->selItem, b->selDesc, sizeof(b->selDesc));
        }
    }
    if (b->bagState == BAG_ACTIONS) {
        int n = CtrBottomBag_ActionCount();

        b->menuCount = (uint8_t)(n > 3 ? 3 : n);
        b->menuCursor = (int8_t)CtrBottomBag_MenuCursor();
        for (int i = 0; i < b->menuCount; ++i)
            CtrBottomBag_ActionName(i, b->menuName[i], sizeof(b->menuName[i]));
    }
    if (b->bagState == BAG_MESSAGE)
        CtrBottomBag_Message(b->message, sizeof(b->message));
    b->berries = (uint16_t)CtrBattle_BerryPouch();
}

static void BatCopy(char *dst, size_t size, const char *src)
{
    size_t n = strlen(src);

    if (n >= size)
        n = size - 1;
    memcpy(dst, src, n);
    dst[n] = 0;
}

/* The Party icons' clock: one tick a frame the Party is drawn (a snapshot is taken every frame). */
static unsigned sIconTick;

/* FireRed's icon animation (sMonIconAnim_*): two frames held 6, 8, 14 or 22 ticks (+1 each) by the HP bar's colour,
 * and a fainted one's only changes its hop. Bit 0 the frame, bit 1 the hop's phase. */
static uint8_t PartyIconFrame(unsigned tick, unsigned hp, unsigned maxHp, int egg)
{
    unsigned hold = 29, phase;

    if (hp != 0 || egg) {
        unsigned pct = maxHp ? hp * 100 / maxHp : 100;

        hold = hp == maxHp ? 6 : pct > 50 ? 8 : pct > 20 ? 14 : 22;
    }
    phase = (tick / (hold + 1)) & 1;
    return (uint8_t)((hp != 0 || egg ? phase : 0) | phase << 1);
}

/* What FireRed's card says in its description box when a TM, HM, tutor or stone is being offered to the Party. These
 * are the game's own words, so the Spanish build matches the Spanish FireRed's (which has no "!" forms). */
static uint8_t PartyTag(const char *text)
{
    static const char kNoUse[] = CTR_TR("No use", "No se usa", "Nutzlos");

    /* The text is the game's raw ALL CAPS (CtrBottomParty_Description does not case it), so compare without case. */
    if (CtrTextCase_Eq(text, CTR_TR("Able", "Puede", "O.K.")) || CtrTextCase_Eq(text, CTR_TR("Able!", "Puede!", "O.K.")))
        return BUI_PTY_TAG_ABLE;
    if (CtrTextCase_Eq(text, CTR_TR("Not able", "No puede", "NEIN")) || CtrTextCase_Eq(text, CTR_TR("Not able!", "No puede!", "NEIN")))
        return BUI_PTY_TAG_NOT_ABLE;
    if (CtrTextCase_Eq(text, CTR_TR("Learned", "Aprendido", "KENNT ATT.")))
        return BUI_PTY_TAG_LEARNED;
    if (CtrTextCase_EqN(text, kNoUse, sizeof(kNoUse) - 1))
        return BUI_PTY_TAG_NO_USE;
    return BUI_PTY_TAG_NONE;
}

/* Picking several: the page's arrow is on CONFIRM (0) or CANCEL (1) when the cursor, as echoed, is past the slots. */
static int8_t PartyButton(int multi, int cursor)
{
    return (int8_t)(multi && cursor >= PARTY_SIZE ? cursor - PARTY_SIZE : -1);
}

/* The battle Party as the Party is now (the lower screen's own Party snapshot `s`, which reads FireRed's Party): the six
 * slots, the Party's selection, what it was opened for, and its menu or message when it has one open. */
static void BattlePartyFill(Snapshot *s)
{
    CtrBattleView *v = &s->battle;
    CtrBattleParty *p = &v->pty;
    int action = CtrBottomParty_Action();
    int cursor;

    ++sIconTick;

    BattleCommon(v);
    v->state = BUI_PARTY;
    cursor = PartyEchoCursor(s->cursor);       /* a slot just touched, until the Party's cursor is there */
    p->cursor = (int8_t)(cursor >= 0 && cursor < PARTY_SIZE ? cursor : -1);
    /* SEND_OUT cannot be cancelled. Out of battle the Party says what it is for itself (its own prompt). */
    p->mode = s->chooseMultiple ? BUI_PTY_MULTI : !CtrBottomParty_InBattle() ? BUI_PTY_SWITCH
            : action == 1 ? BUI_PTY_FORCED : action == 3 ? BUI_PTY_TARGET : BUI_PTY_SWITCH;
    /* Picking several: FireRed's cursor may be on CONFIRM (slot 6) or CANCEL (7). */
    p->button = PartyButton(p->mode == BUI_PTY_MULTI, cursor);
    p->lead = (uint8_t)(CtrBottomParty_Layout() == 0 ? 1 : 2);
    p->caption[0] = 0;
    if (!CtrBottomParty_InBattle() && s->panel != PANEL_MESSAGE)
        BatCopy(p->caption, sizeof(p->caption), s->message);
    for (int i = 0; i < PARTY_SIZE; ++i) {
        p->present[i] = (uint8_t)((s->presentMask >> i) & 1);
        if (!p->present[i])
            continue;
        p->egg[i] = s->isEgg[i];
        p->status[i] = s->status[i];
        p->level[i] = s->level[i];
        p->hp[i] = s->hp[i];
        p->maxHp[i] = s->maxHp[i];
        p->icon[i] = s->icon[i];
        p->iconFrame[i] = PartyIconFrame(sIconTick, s->hp[i], s->maxHp[i], s->isEgg[i]);
        if (p->mode == BUI_PTY_MULTI) {
            int entry = CtrBottomParty_EntryState(i);

            p->entry[i] = (uint8_t)(entry < 0 ? BUI_PTY_ABLE : entry);
        }
        else
            p->tag[i] = PartyTag(s->description[i]);
        snprintf(p->name[i], sizeof(p->name[i]), "%s", s->nick[i]);
    }
    {
        int slot, level, stats[6], gains[6];

        /* A Rare Candy's level up: the card, not the message over the dimmed Party. */
        if (s->panel == PANEL_MESSAGE && CtrBottomParty_LevelUp(&slot, &level, stats, gains) && slot >= 0 && slot < PARTY_SIZE) {
            p->panel = BUI_PTY_LEVELUP;
            p->lvSlot = (uint8_t)slot;
            p->lvLevel = (uint8_t)level;
            for (int i = 0; i < 6; ++i) {
                p->lvStat[i] = (uint16_t)stats[i];
                p->lvGain[i] = (int16_t)gains[i];
            }
            BatCopy(p->message, sizeof(p->message), s->message);
            s->battleUi = 1;
            s->touch.viewport = 0;
            return;
        }
    }
    if (s->panel == PANEL_ACTIONS || s->panel == PANEL_YESNO) {
        int n = s->panel == PANEL_YESNO ? 2 : s->actionCount > 8 ? 8 : s->actionCount;

        p->panel = s->panel == PANEL_YESNO ? BUI_PTY_YESNO : BUI_PTY_ACTIONS;
        p->menuCount = (uint8_t)n;
        p->menuCursor = s->actionCursor;
        if (sPlan.kind == PLAN_MENU && sPlan.target < n)
            p->menuCursor = (int8_t)sPlan.target;      /* the row touched, not the cursor on its way there */
        for (int i = 0; i < n; ++i)
            BatCopy(p->menuName[i], sizeof(p->menuName[i]), s->panel == PANEL_YESNO ? (i == 0 ? CTR_TR("Yes", "S\xC3\xAD", "Ja") : CTR_TR("No", "No", "Nein")) : s->actionNames[i]);
    } else if (s->panel == PANEL_MESSAGE) {
        p->panel = BUI_PTY_TEXT;
        BatCopy(p->message, sizeof(p->message), s->message);
    }
    s->battleUi = 1;
    s->touch.viewport = 0;
}

/* The battle screen as the controller is now: nothing but what FireRed reports. */
static void BattleFill(CtrBattleView *v)
{
    int state = CtrBattle_State();

    BattleCommon(v);
    v->state = state == BATTLE_ACTION ? BUI_ACTION : state == BATTLE_MOVE ? BUI_MOVE
             : state == BATTLE_TARGET ? BUI_TARGET : state == BATTLE_SAFARI ? BUI_SAFARI : BUI_IDLE;
    if (v->state == BUI_TARGET) {
        /* The cards: opposing battlers on top (left, right), the player's below. Only the ones the controller's own
         * rule (the one its D-pad walks by) lets the move be aimed at are shown. */
        static const int8_t card[4] = {2, 0, 3, 1};     /* by FireRed's position: PLAYER_LEFT, OPPONENT_LEFT, PLAYER_RIGHT, OPPONENT_RIGHT */
        int cursor = CtrBattle_TargetCursor();

        v->cursor = -1;

        for (int b = 0; b < CtrBattle_TargetCount() && b < 4; ++b) {
            int position = CtrBattle_TargetPosition(b), i;
            CtrBattleTarget *t;
            int level = 0, hp = 0, maxHp = 0;

            if (position < 0 || position > 3 || !CtrBattle_TargetLegal(b))
                continue;
            i = card[position];
            t = &v->target[i];
            if (!CtrBattle_TargetInfo(b, t->name, (int)sizeof(t->name), &level, &hp, &maxHp))
                continue;
            t->present = 1;
            t->battler = (int8_t)b;
            t->side = (uint8_t)((position & 1) == 0);
            t->level = (uint8_t)(level > 255 ? 255 : level);
            t->hp = (uint16_t)hp;
            t->maxHp = (uint16_t)maxHp;
            t->matchup = (uint8_t)CtrBattle_TargetMatchup(b);
            if (b == cursor)
                v->cursor = (int8_t)i;
        }
        v->showCursor = !sBatTouchLast;
        return;
    }
    if (v->state == BUI_SAFARI) {
        v->cursor = (int8_t)CtrBattle_ActionCursor();
        v->showCursor = !sBatTouchLast;
        return;
    }
    if (v->state == BUI_ACTION) {
        unsigned item, qty;
        int place, count;

        v->cursor = (int8_t)CtrBattle_ActionCursor();
        v->showCursor = !sBatTouchLast;
        v->ball = (uint8_t)CtrBattle_QuickBall(&item, &qty, &place, &count);
        v->runBlocked = (uint8_t)!CtrBattle_CanRun();
        if (v->ball != CTR_QUICK_BALL_NONE) {
            v->ballItem = (uint16_t)item;
            v->ballQty = (uint16_t)(qty > 999 ? 999 : qty);
            v->ballPlace = (uint8_t)place;
            v->ballCount = (uint8_t)count;
            if (item)
                CtrBottomBag_ItemName(item, v->ballName, sizeof(v->ballName));
        }
    }
    if (v->state != BUI_MOVE)
        return;
    v->cursor = (int8_t)CtrBattle_MoveCursor();
    v->showCursor = !sBatTouchLast;
    for (int i = 0; i < 4; ++i) {
        unsigned move = CtrBattle_Move(i);
        CtrBattleMove *m = &v->move[i];
        unsigned power, acc;

        if (move == 0)
            continue;
        m->present = 1;
        m->type = (uint8_t)CtrBattle_MoveType(move);
        CtrBattle_MoveName(move, m->name, sizeof(m->name));
        CtrBattle_TypeName(m->type, m->typeName, sizeof(m->typeName));
        if (CtrOptions_MoveDetails())
            CtrBattle_MoveDescription(move, m->desc, sizeof(m->desc));
        else
            m->desc[0] = 0;
        {
            int pp, maxPp;

            if (CtrBattle_MovePP(i, &pp, &maxPp)) {
                m->pp = (uint8_t)(pp < 0 ? 0 : pp > 255 ? 255 : pp);
                m->maxPp = (uint8_t)(maxPp < 0 ? 0 : maxPp > 255 ? 255 : maxPp);
            }
        }
        if (CtrOptions_TypeMatchup())
            m->matchup = (uint8_t)CtrBattle_MoveMatchup(move);
        else
            m->matchup = CTR_FX_NONE;
        m->stab = (uint8_t)(CtrBattle_MoveStab(move) != 0);
        m->effectChance = (uint8_t)CtrBattle_MoveEffectChance(move);
        m->priority = (int8_t)CtrBattle_MovePriority(move);
        power = CtrBattle_MovePower(move);
        if (power)
            snprintf(m->power, sizeof(m->power), "%u", power > 999 ? 999 : power);
        else
            snprintf(m->power, sizeof(m->power), "-");
        acc = CtrBattle_MoveAccuracy(move);
        if (acc)
            snprintf(m->accuracy, sizeof(m->accuracy), "%u", acc > 999 ? 999 : acc);
        else
            snprintf(m->accuracy, sizeof(m->accuracy), "-");
    }
}

/* SAVE's view: FireRed's own save information (the facts SaveStatToString prints), the player's
 * icon key, and the dialogue's text for the state. No BACK: NO, B and the sidebar leave. */
static void SaveFill(CtrPokeTouchView *v)
{
    unsigned minutes = CtrSave_PlayMinutes();

    v->save = 1;
    v->saveState = (uint8_t)sSave.state;
    v->saveSel = (uint8_t)sSave.sel;
    v->savePressed = (uint8_t)(sSave.down && sSave.inside ? sSave.down : PT_SAVE_NONE);
    v->playerFemale = (uint8_t)(CtrPokeTouch_PlayerFemale() != 0);
    CtrPokeTouch_PlayerName(v->playerName, (int)sizeof(v->playerName));
    v->saveBadges = (uint8_t)CtrSave_BadgeCount();
    v->saveHasDex = (uint8_t)(CtrSave_HasPokedex() != 0);
    v->saveDex = (uint16_t)CtrSave_DexCaught();
    v->saveHours = (uint16_t)CtrSave_PlayHours();
    v->saveMinutes = (uint8_t)(minutes > 59 ? 59 : minutes);
    v->saveMoney = CtrSave_Money();
    switch (sSave.state) {
    case SAVE_WRITING:
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Saving...", "Guardando...", "Speichern..."));
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("Don't turn off the power.", "No apagues la consola.", "Nicht ausschalten."));
        break;
    case SAVE_SUCCESS:
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Game saved!", "\xc2\xa1Partida guardada!", "Spiel gespeichert!"));
        break;
    case SAVE_ERROR:
        snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Could not save.", "No se pudo guardar.", "Speichern fehlgeschlagen."));
        snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("Try again.", "Prueba otra vez.", "Bitte erneut versuchen."));
        break;
    default:
        if (CtrSave_IsDifferentFile()) {
            snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Another game is saved.", "Ya hay otra partida.", "Anderer Spielstand da."));
            snprintf(v->bar[1], PT_BAR_LEN, CTR_TR("Overwrite it?", "\xc2\xbfSobrescribirla?", "\xc3\x9c" "berschreiben?"));
        } else {
            snprintf(v->bar[0], PT_BAR_LEN, CTR_TR("Save your game?", "\xc2\xbfGuardar la partida?", "Spiel speichern?"));
            v->bar[1][0] = 0;
        }
        break;
    }
}
