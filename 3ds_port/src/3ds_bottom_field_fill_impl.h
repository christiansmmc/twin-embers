/* Part of 3ds_bottom.c, included there where it stood (everything static, the same translation unit): the field
 * views' bars and fills, what each lower-screen view of the field (BAG and its cases, shops, the Trainer Card, the
 * forget-a-move view, choices, naming, the PC box, the Pokedex) gets from the game. Split out so 3ds_bottom.c stays
 * under the source audit's 256 KiB per file. */

/* The bar under the Bag: the pocket, the item under the cursor, and the
 * X shortcut when it is the registered item (else the money). */
static void BagBar(CtrPokeTouchView *v)
{
    unsigned item = CtrBottomBag_Item(CtrBottomBag_Cursor());

    CtrBottomBag_PocketName(v->bar[0], PT_BAR_LEN);
    if (item != 0)
        CtrBottomBag_ItemName(item, v->bar[1], PT_BAR_LEN);
    if (item != 0 && item == v->registeredItem)
        snprintf(v->bar[2], PT_BAR_LEN, CTR_TR("On the X button", "En el bot\xc3\xb3n X", "Auf Taste X"));
    else if (item != 0 && item == v->bikeItem)
        snprintf(v->bar[2], PT_BAR_LEN, CTR_TR("On the Y button", "En el bot\xc3\xb3n Y", "Auf Taste Y"));
    else
        snprintf(v->bar[2], PT_BAR_LEN, CTR_TR("Money  %u", "Dinero  %u", "Geld  %u"), CtrBottomBag_Money());
}

/* The field Bag drawn by the lower screen (3ds_field_bag_impl.h) rather than sampled from the GBA picture: the field's
 * own Bag, Party's GIVE Bag, a shop's (selling) and the item PC's (depositing), outside the Pokemon PC. The battle's
 * and the Old Man's demonstration are drawn by the battle screen (BattleBagFill). */
static int FieldBagNative(void)
{
    return CtrBottomBag_TouchDrawn() && !CtrBottomBag_InBattle() && !sStorageSession;
}

/* What the lower screen draws a list screen from: the Bag's bridge, or a case's. A NULL question has no answer there
 * (the TM Case has no quantity box nor YES/NO). */
typedef struct {
    int mode, kind;
    const ListScreen *list;
    unsigned (*item)(int index);
    unsigned (*quantity)(int index);
    int (*actionCount)(void);
    void (*actionName)(int index, char *out, int size);
    int (*tossCount)(void);
    int (*tossMax)(void);
    int (*prompt)(char *out, int size);
    int (*message)(char *out, int size);
    /* A row's name when it is not its item's (the mailbox: the mail's writer), or NULL. */
    void (*name)(int index, char *out, int size);
} FieldList;

/* Defined with the list screens' touch plans below. */
static const ListScreen sBagScreen, sTMCaseScreen, sPouchScreen, sShopScreen, sItemPcScreen, sMailboxScreen;

static const FieldList sFieldBagList = {
    MODE_BAG, PT_BAG_KIND_BAG, &sBagScreen, CtrBottomBag_Item, CtrBottomBag_Quantity, CtrBottomBag_ActionCount,
    CtrBottomBag_ActionName, CtrBottomBag_TossCount, CtrBottomBag_TossMax, CtrBottomBag_Prompt, CtrBottomBag_Message, NULL,
};
static const FieldList sFieldTMCaseList = {
    MODE_TMCASE, PT_BAG_KIND_TMCASE, &sTMCaseScreen, CtrBottomTMCase_Item, CtrBottomTMCase_Quantity,
    CtrBottomTMCase_ActionCount, CtrBottomTMCase_ActionName, NULL, NULL, NULL, CtrBottomTMCase_Message, NULL,
};
static const FieldList sFieldPouchList = {
    MODE_POUCH, PT_BAG_KIND_POUCH, &sPouchScreen, CtrBottomPouch_Item, CtrBottomPouch_Quantity,
    CtrBottomPouch_ActionCount, CtrBottomPouch_ActionName, CtrBottomPouch_TossCount, CtrBottomPouch_TossMax,
    CtrBottomPouch_Prompt, CtrBottomPouch_Message, NULL,
};
/* A mart's BUY list: its rows' "quantity" is their price; it has no menu of actions. */
static int ShopNoActions(void) { return 0; }
static void ShopNoActionName(int index, char *out, int size) { (void)index; if (size > 0) out[0] = 0; }
static const FieldList sFieldShopList = {
    MODE_SHOP, PT_BAG_KIND_SHOP, &sShopScreen, CtrBottomShop_Item, CtrBottomShop_Price, ShopNoActions,
    ShopNoActionName, CtrBottomShop_Quantity, CtrBottomShop_QuantityMax, CtrBottomShop_Text, CtrBottomShop_Text, NULL,
};
/* The item PC's WITHDRAW ITEM list: its menu (WITHDRAW, GIVE, CANCEL), how many, and its messages; no YES/NO. */
static const FieldList sFieldItemPcList = {
    MODE_ITEMPC, PT_BAG_KIND_ITEMPC, &sItemPcScreen, CtrBottomItemPc_Item, CtrBottomItemPc_Quantity,
    CtrBottomItemPc_ActionCount, CtrBottomItemPc_ActionName, CtrBottomItemPc_TossCount, CtrBottomItemPc_TossMax, NULL,
    CtrBottomItemPc_Message, NULL,
};
/* The PC's mailbox: a row per mail (its item, its writer, no count), a mail's menu (READ, MOVE TO BAG, GIVE, EXIT),
 * MOVE TO BAG's YES/NO and the messages. Drawn as the item PC's list. */
static const FieldList sFieldMailboxList = {
    MODE_MAILBOX, PT_BAG_KIND_ITEMPC, &sMailboxScreen, CtrBottomMailbox_Item, CtrBottomMailbox_Quantity,
    CtrBottomMailbox_ActionCount, CtrBottomMailbox_ActionName, NULL, NULL, CtrBottomMailbox_Message,
    CtrBottomMailbox_Message, CtrBottomMailbox_Name,
};

/* The list the lower screen draws itself in this mode (the field's Bag and the cases it opens), or NULL. */
static const FieldList *FieldListFor(int mode)
{
    if (mode == MODE_SHOP)
        return &sFieldShopList;
    if (mode == MODE_ITEMPC)
        return &sFieldItemPcList;
    if (mode == MODE_MAILBOX)
        return &sFieldMailboxList;
    if (sStorageSession)
        return NULL;
    if (mode == MODE_BAG)
        return FieldBagNative() ? &sFieldBagList : NULL;
    return mode == MODE_TMCASE ? &sFieldTMCaseList : mode == MODE_POUCH ? &sFieldPouchList : NULL;
}

/* A row's name: the item's, and for a TM/HM its move's after it ("TM01 FOCUS PUNCH"). */
static void FieldListName(const FieldList *f, unsigned item, char *out, int size)
{
    CtrBottomBag_ItemName(item, out, size);
    /* Only a TM/HM has a move: for any other item the move table would be read out of range. */
    if (f->kind == PT_BAG_KIND_TMCASE || (f->kind == PT_BAG_KIND_SHOP && item >= 289 /* ITEM_TM01 */ && item <= 346 /* ITEM_HM08 */)) {
        char move[16];
        int n = (int)strlen(out);

        CtrBottomTMCase_MoveName(item, move, sizeof(move));
        if (move[0] && n + 1 < size)
            snprintf(out + n, (size_t)(size - n), " %s", move);
    }
}

/* The field Bag (or a case) as it is now: its pocket, the rows around the cursor (CLOSE the row after the last item),
 * the selected item's detail, and what it has open. While it is busy (a pocket sliding in, a message printing) the
 * picture last shown stays. */
static void FieldBagFill(CtrPokeTouchView *v, int pressed, const FieldList *f)
{
    CtrPokeTouchBag *b = &v->bag;
    const ListScreen *list = f->list;
    int count, total, state = list->state();

    if (state == BAG_BUSY && sHaveShown && sShown.mode == f->mode && sShown.touch.bag.on) {
        *b = sShown.touch.bag;
        b->pressed = -1;
        return;
    }
    memset(b, 0, sizeof(*b));
    v->frameType = (uint8_t)CtrPokeTouch_OptionGet(PT_OPT_FRAME);     /* the page's windows: the player's FRAME */
    b->on = 1;
    b->kind = (uint8_t)f->kind;
    b->moveType = -1;
    b->pressed = (int16_t)pressed;
    b->state = (uint8_t)(state == BAG_BUSY ? BAG_LIST : state);
    if (f->kind == PT_BAG_KIND_BAG) {
        b->use = (uint8_t)CtrBottomBag_Use();
        if (b->use == PT_BAG_USE_SELL) {
            b->money = CtrBottomBag_Money();
            b->sale = state == BAG_QUANTITY ? CtrBottomBag_SalePrice() : 0;
        }
    }
    if (f->kind == PT_BAG_KIND_TMCASE) {
        CtrBottomTMCase_Title(b->title, sizeof(b->title));
        CtrBottomTMCase_CloseLabel(b->closeName, sizeof(b->closeName));
        b->titleItem = (uint16_t)CtrBottomTMCase_CaseItem();
    } else if (f->kind == PT_BAG_KIND_POUCH) {
        CtrBottomPouch_Title(b->title, sizeof(b->title));
        CtrBottomPouch_CloseLabel(b->closeName, sizeof(b->closeName));
        b->titleItem = (uint16_t)CtrBottomPouch_PouchItem();
    } else if (f->kind == PT_BAG_KIND_SHOP) {
        CtrBottomShop_Title(b->title, sizeof(b->title));
        snprintf(b->closeName, sizeof(b->closeName), "%s", CtrUiText(UIS_CANCEL));
        b->money = CtrBottomShop_Money();
        b->sale = state == BAG_QUANTITY ? CtrBottomShop_Total() : 0;
    } else if (f->mode == MODE_MAILBOX) {
        CtrBottomMailbox_Title(b->title, sizeof(b->title));
        CtrBottomMailbox_CloseLabel(b->closeName, sizeof(b->closeName));
    } else if (f->kind == PT_BAG_KIND_ITEMPC) {
        CtrBottomItemPc_Title(b->title, sizeof(b->title));
        CtrBottomItemPc_CloseLabel(b->closeName, sizeof(b->closeName));
    } else
    {
        b->pocket = (uint8_t)CtrBottomBag_Pocket();
        b->regX = (uint16_t)CtrPokeTouch_RegisteredItem();
        b->regY = (uint16_t)CtrPokeTouch_BikeItem();
    }
    b->count = (int16_t)(count = list->count());
    b->cursor = (int16_t)list->cursor();
    total = count + 1;
    b->first = (int16_t)(b->cursor - (PT_BAG_ROWS - 1) / 2 < 0 ? 0 : b->cursor - (PT_BAG_ROWS - 1) / 2);
    if (b->first > total - PT_BAG_ROWS)
        b->first = (int16_t)(total - PT_BAG_ROWS < 0 ? 0 : total - PT_BAG_ROWS);
    for (int i = 0; i < PT_BAG_ROWS; ++i) {
        unsigned item = f->item(b->first + i);

        b->row[i].item = (uint16_t)item;
        if (item) {
            if (f->kind == PT_BAG_KIND_SHOP)
                b->row[i].price = (uint16_t)f->quantity(b->first + i);
            else
                b->row[i].qty = (uint16_t)f->quantity(b->first + i);
            if (f->name != NULL)
                f->name(b->first + i, b->row[i].name, sizeof(b->row[i].name));
            else
                FieldListName(f, item, b->row[i].name, sizeof(b->row[i].name));
        }
    }
    if (b->cursor >= 0 && b->cursor < count) {
        b->selItem = (uint16_t)f->item(b->cursor);
        if (b->selItem) {
            int type = -1, power, accuracy, pp;

            if (f->kind == PT_BAG_KIND_TMCASE)
                CtrBottomTMCase_MoveName(b->selItem, b->selName, sizeof(b->selName));
            else if (f->name != NULL)
                f->name(b->cursor, b->selName, sizeof(b->selName));
            else
                CtrBottomBag_ItemName(b->selItem, b->selName, sizeof(b->selName));
            CtrBottomBag_ItemDescription(b->selItem, b->selDesc, sizeof(b->selDesc));
            if (f->kind == PT_BAG_KIND_SHOP) {
                b->showOwned = 1;
                b->owned = BagGetQuantityByItemId((unsigned short)b->selItem);
            }
            if ((f->kind == PT_BAG_KIND_TMCASE || f->kind == PT_BAG_KIND_SHOP)
                && CtrBottomTMCase_MoveInfo(b->selItem, &type, &power, &accuracy, &pp, b->typeName, sizeof(b->typeName)))
                b->moveType = (int8_t)type;
        }
    }
    if (state == BAG_ACTIONS || state == BAG_YESNO) {
        int n = state == BAG_YESNO ? 2 : f->actionCount();

        b->menuCount = (uint8_t)(n > PT_BAG_MENU_MAX ? PT_BAG_MENU_MAX : n);
        b->menuCursor = (int8_t)CtrBottomBag_MenuCursor();
        for (int i = 0; state == BAG_ACTIONS && i < b->menuCount; ++i)
            f->actionName(i, b->menuName[i], sizeof(b->menuName[i]));
    }
    /* The count chosen last in this list's quantity box, for the YES/NO that confirms it: a message printed between
     * them (a mart's "and you want 17. That will be...") left the frame shown just before without one, and the
     * confirmation said x1. A new pick from the list or its menu forgets it. */
    static int chosenMode = -1;
    static uint16_t chosen;

    if (state == BAG_LIST || state == BAG_ACTIONS || chosenMode != f->mode) {
        chosenMode = f->mode;
        chosen = 0;
    }
    if (state == BAG_QUANTITY && f->tossCount != NULL) {
        b->tossCount = (uint16_t)f->tossCount();
        b->tossMax = (uint16_t)f->tossMax();
        chosen = b->tossCount;
    }
    if (state == BAG_YESNO && f->prompt != NULL) {
        f->prompt(b->prompt, sizeof(b->prompt));
        b->tossCount = chosen ? chosen : 1;   /* one when none was asked */
    }
    if (state == BAG_MESSAGE)
        f->message(b->prompt, sizeof(b->prompt));
}

/* The player's Trainer Card as the card shows it: its side, FireRed's labels and numbers, and its back's records. */
static void FieldCardFill(CtrPokeTouchView *v, int pressed)
{
    CtrPokeTouchCard *c = &v->card;
    unsigned n[8];
    int rows = 0;

    memset(c, 0, sizeof(*c));
    c->on = 1;
    c->back = (uint8_t)(CtrBottomCard_OnBack() != 0);
    c->pressed = (uint8_t)(pressed != 0);
    CtrBottomCard_Name(c->name, sizeof(c->name));
    for (int i = 0; i < 6; ++i)
        CtrBottomCard_Label(i, c->label[i], sizeof(c->label[i]));
    CtrBottomCard_Numbers(n);
    c->id = n[0];
    c->money = n[1];
    c->hasDex = (uint8_t)n[2];
    c->caught = (uint16_t)n[3];
    c->hours = (uint16_t)n[4];
    c->minutes = (uint8_t)n[5];
    c->stars = (uint8_t)n[6];
    c->badges = (uint8_t)n[7];
    while (rows < PT_CARD_ROWS && CtrBottomCard_BackRow(rows, c->row[rows].label, sizeof(c->row[rows].label),
                                                         c->row[rows].value, sizeof(c->row[rows].value)))
        ++rows;
    c->rows = (uint8_t)rows;
}

/* The Summary choosing the move to forget, as it is now. While it does not take a key the picture last shown stays. */
static void FieldLearnFill(CtrPokeTouchView *v, int pressed)
{
    CtrPokeTouchLearn *l = &v->learn;

    if (!CtrBottomLearn_Ready() && sHaveShown && sShown.mode == MODE_PARTY_CHILD && sShown.touch.learn.on) {
        *l = sShown.touch.learn;
        l->pressed = -1;
        return;
    }
    memset(l, 0, sizeof(*l));
    l->on = 1;
    l->pressed = (int16_t)pressed;
    l->cursor = (int8_t)CtrBottomLearn_Cursor();
    l->species = (uint16_t)CtrBottomLearn_Species();
    CtrBottomLearn_Name(l->nickname, sizeof(l->nickname), l->level, sizeof(l->level));
    CtrBottomLearn_Question(l->question, sizeof(l->question));
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        int type = -1;

        l->row[i].move = (uint16_t)CtrBottomLearn_Move(i, &type, l->row[i].name, sizeof(l->row[i].name), l->row[i].pp,
                                                       sizeof(l->row[i].pp), l->row[i].power, sizeof(l->row[i].power),
                                                       l->row[i].accuracy, sizeof(l->row[i].accuracy));
        l->row[i].type = (int8_t)type;
        l->row[i].locked = (uint8_t)(CtrBottomLearn_Locked(i) != 0);
        CtrBottomLearn_TypeName(type, l->row[i].typeName, sizeof(l->row[i].typeName));
    }
    if (l->cursor >= 0 && l->cursor < PT_LEARN_ROWS)
        CtrBottomLearn_Description(l->row[l->cursor].move, l->desc, sizeof(l->desc));
}

/* The move to forget, when that is the child screen: drawn here, BACK is the Summary's B. */
static void LearnSnapshot(Snapshot *s, int mode)
{
    if (!CtrBottomLearn_IsOpen())
        return;
    FieldLearnFill(&s->touch, sLearnTap.down && sLearnTap.inside ? sLearnTap.code : -1);
    s->touch.viewport = 0;
    s->touch.backVisible = 1;
    s->touch.backEnabled = (uint8_t)BackEnabled(mode);
    s->touch.backPressed = (uint8_t)(sBack.inside != 0);
}

/* The field's choice as FireRed's menu has it: its rows' labels, the cursor, the message box's question. */
static void ChoiceSnapshot(Snapshot *s, int mode)
{
    CtrPokeTouchChoice *c = &s->touch.choice;
    int n = CtrFieldChoice_Count();

    memset(c, 0, sizeof(*c));
    c->on = 1;
    c->count = (uint8_t)(n > PT_CHOICE_MAX ? PT_CHOICE_MAX : n);
    c->cols = (uint8_t)CtrFieldChoice_Columns();
    c->cursor = (int8_t)CtrFieldChoice_Cursor();
    c->pressed = (int16_t)(sChoiceTap.down && sChoiceTap.inside ? sChoiceTap.code : -1);
    for (int i = 0; i < c->count; ++i)
        CtrFieldChoice_Label(i, c->label[i], sizeof(c->label[i]));
    if (CtrBottomStorage_PCMenu())
        CtrBottomStorage_PCMessage(c->prompt, sizeof(c->prompt));
    else
        CtrFieldChoice_Prompt(c->prompt, sizeof(c->prompt));
    s->touch.viewport = 0;
    s->touch.backVisible = 1;
    s->touch.backEnabled = (uint8_t)BackEnabled(mode);
    s->touch.backPressed = (uint8_t)(sBack.inside != 0);
}

/* The naming screen as it is now. While it does not take a key (a page turning, the cursor going to OK) the picture
 * last shown stays. */
static void FieldNameFill(CtrPokeTouchView *v, int pressed)
{
    CtrPokeTouchName *n = &v->name;
    int col, row;

    if (!CtrBottomNaming_Ready() && sHaveShown && sShown.mode == MODE_NAMING && sShown.touch.name.on) {
        *n = sShown.touch.name;
        n->pressed = -1;
        return;
    }
    memset(n, 0, sizeof(*n));
    n->on = 1;
    n->pressed = (int16_t)pressed;
    n->page = (uint8_t)CtrBottomNaming_Page();
    n->cols = (uint8_t)CtrBottomNaming_Columns();
    n->maxChars = (uint8_t)CtrBottomNaming_MaxChars();
    if (n->maxChars > PT_NAME_MAX)
        n->maxChars = PT_NAME_MAX;
    for (int r = 0; r < PT_NAME_ROWS; ++r)
        for (int c = 0; c < PT_NAME_COLS; ++c)
            n->key[r][c] = (uint8_t)CtrBottomNaming_Key(r, c);
    memset(n->text, 0xff, sizeof(n->text));
    CtrBottomNaming_Text(n->text, (int)sizeof(n->text));
    while (n->len < n->maxChars && n->text[n->len] != 0xff)
        ++n->len;
    CtrBottomNaming_Title(n->title, (int)sizeof(n->title));
    n->species = (uint16_t)CtrBottomNaming_Species();
    n->gender = (uint8_t)CtrBottomNaming_Gender();
    CtrBottomNaming_Cursor(&col, &row);
    n->curCol = (int8_t)col;
    n->curRow = (int8_t)row;
}

/* A PC box, drawn here while FireRed's box is in a state the port draws (MOVE ITEMS too; not carrying several or
 * marking one). While it does not take a key the picture last shown stays. */
static void BoxSnapshot(Snapshot *s)
{
    CtrPokeTouchBox *b = &s->touch.box;

    if (!CtrBottomStorage_BoxFill(b))
        return;
    if (b->state == PT_BOX_STATE_BUSY && sHaveShown && sShown.mode == MODE_STORAGE_BOX && sShown.touch.box.on)
        *b = sShown.touch.box;
    b->pressed = (int16_t)(sStoreTouch.active && sStoreTouch.inside ? sStoreTouch.hit : -1);
    s->touch.viewport = 0;
}

/* A mart's BUY list or the item PC's WITHDRAW ITEM: drawn here, modal; BACK is its B (CANCEL, or back from how many). */
static void ShopSnapshot(Snapshot *s, int mode)
{
    FieldBagFill(&s->touch, sFieldBag.down && sFieldBag.inside ? sFieldBag.code : -1, FieldListFor(mode));
    s->touch.viewport = 0;
    s->touch.backVisible = 1;
    s->touch.backEnabled = (uint8_t)BackEnabled(mode);
    s->touch.backPressed = (uint8_t)(sBack.inside != 0);
}

/* The naming screen: drawn here, modal (no BACK: its own DELETE and OK). */
static void NameSnapshot(Snapshot *s)
{
    FieldNameFill(&s->touch, sNameTap.down && sNameTap.inside ? sNameTap.code : -1);
    s->touch.viewport = 0;
}

/* A Pokemon as the Pokedex knows it: number, flags and (once seen) name. */
static void FieldDexMon(CtrPokeTouchDexMon *m, unsigned species)
{
    memset(m, 0, sizeof(*m));
    if (species == 0)
        return;
    m->species = (uint16_t)species;
    m->number = (uint16_t)CtrBottomDex_Number(species);
    m->flags = (uint8_t)CtrBottomDex_Flags(species);
    if (m->flags & 1)
        CtrBottomDex_Name(species, m->name, sizeof(m->name));
}

/* The selected Pokemon's types; on its page, its category, size, weight and entry too (FireRed tells them only for a
 * caught one). */
static void FieldDexDetail(CtrPokeTouchDex *d, int page)
{
    unsigned species = d->sel.species;
    int types[2];

    d->type[0] = d->type[1] = -1;
    if (CtrBottomDex_Types(species, types, d->typeName[0], d->typeName[1], sizeof(d->typeName[0]))) {
        d->type[0] = (int8_t)types[0];
        d->type[1] = (int8_t)types[1];
    }
    if (!page)
        return;
    CtrBottomDex_Category(species, d->kind, sizeof(d->kind));
    d->height = (uint16_t)CtrBottomDex_Height(species);
    d->weight = (uint16_t)CtrBottomDex_Weight(species);
    CtrBottomDex_Description(species, d->desc, sizeof(d->desc));
}

/* The Pokedex as it is now, for the lower screen to draw: 0 for its AREA page, which FireRed draws (the viewport
 * shows it). While the Pokedex is busy (a fade, a page flip) the picture last shown stays. */
static int FieldDexFill(CtrPokeTouchView *v, int pressed)
{
    CtrPokeTouchDex *d = &v->dex;
    int state = CtrBottomDex_State();
    unsigned counts[4];

    if (state == DEX_PAGE && CtrBottomDex_MonArea()) {
        memset(d, 0, sizeof(*d));
        return 0;
    }
    if (state == DEX_BUSY) {
        if (sHaveShown && sShown.mode == MODE_DEX && sShown.touch.dex.on)
            *d = sShown.touch.dex;
        else if (sHaveShown && sShown.mode == MODE_DEX && sShown.touch.viewport)
            return 0;                   /* leaving the AREA page: FireRed's picture until the page is back */
        else if (CtrBottomDex_Registering())
            return 0;                   /* a catch's page coming in (its habitat, the zoom): FireRed's picture */
        else {
            memset(d, 0, sizeof(*d));
            d->on = 1;
        }
        d->pressed = -1;
        return 1;
    }
    memset(d, 0, sizeof(*d));
    d->on = 1;
    d->pressed = (int16_t)pressed;
    d->state = (uint8_t)state;
    d->national = (uint8_t)(CtrBottomDex_National() != 0);
    CtrBottomDex_Counts(counts);
    d->seen[0] = (uint16_t)counts[0];
    d->owned[0] = (uint16_t)counts[1];
    d->seen[1] = (uint16_t)counts[2];
    d->owned[1] = (uint16_t)counts[3];
    d->type[0] = d->type[1] = -1;
    if (state == DEX_MENU) {
        int n = CtrBottomDex_MenuCount();

        d->menuCount = (uint8_t)(n > PT_DEX_MENU_MAX ? PT_DEX_MENU_MAX : n);
        for (int i = 0; i < d->menuCount; ++i) {
            int id = CtrBottomDex_MenuItem(i, d->menu[i].name, sizeof(d->menu[i].name));

            d->menu[i].id = (int8_t)id;
            d->menu[i].locked = (uint8_t)(!CtrBottomDex_MenuUnlocked(id));
        }
        d->menuCursor = (int8_t)CtrBottomDex_Cursor();
    } else if (state == DEX_LIST) {
        int count = CtrBottomDex_ListCount();

        d->order = (uint8_t)CtrBottomDex_ListOrder();
        d->listCount = (int16_t)count;
        d->listCursor = (int16_t)CtrBottomDex_Cursor();
        d->listFirst = (int16_t)(d->listCursor - 2 < 0 ? 0 : d->listCursor - 2);
        if (d->listFirst > count - PT_DEX_ROWS)
            d->listFirst = (int16_t)(count - PT_DEX_ROWS < 0 ? 0 : count - PT_DEX_ROWS);
        for (int i = 0; i < PT_DEX_ROWS && d->listFirst + i < count; ++i)
            FieldDexMon(&d->row[i], CtrBottomDex_ListEntry(d->listFirst + i) & 0xFFFFu);
        FieldDexMon(&d->sel, CtrBottomDex_ListEntry(d->listCursor) & 0xFFFFu);
        FieldDexDetail(d, 0);
    } else if (state == DEX_CATEGORY) {
        int category, page, pages, slots, slot;

        if (CtrBottomDex_CategoryPage(&category, &page, &pages, &slots, &slot)) {
            d->category = (uint8_t)category;
            d->page = (uint8_t)page;
            d->pages = (uint8_t)pages;
            d->slots = (uint8_t)(slots > 4 ? 4 : slots);
            d->slot = (uint8_t)slot;
            d->canPrev = (uint8_t)(CtrBottomDex_PageCanFlip(-1) != 0);
            d->canNext = (uint8_t)(CtrBottomDex_PageCanFlip(1) != 0);
            for (int i = 0; i < d->slots; ++i)
                FieldDexMon(&d->mon[i], CtrBottomDex_CategorySpecies(i));
            if (slot >= 0 && slot < d->slots)
                d->sel = d->mon[slot];
            FieldDexDetail(d, 0);
        }
    } else if (state == DEX_PAGE) {
        FieldDexMon(&d->sel, CtrBottomDex_MonSpecies());
        FieldDexDetail(d, 1);
        d->added = (uint8_t)(CtrBottomDex_Registering() != 0);
    }
    return 1;
}

/* The bar under the TM Case: its title, the TM/HM under the cursor and the
 * move it teaches. */
static void TMCaseBar(CtrPokeTouchView *v)
{
    unsigned item = CtrBottomTMCase_Item(CtrBottomTMCase_Cursor());

    CtrBottomTMCase_Title(v->bar[0], PT_BAR_LEN);
    if (item != 0) {
        CtrBottomBag_ItemName(item, v->bar[1], PT_BAR_LEN);
        CtrBottomTMCase_MoveName(item, v->bar[2], PT_BAR_LEN);
    }
}

/* The bar under the Berry Pouch: its name, the berry under the cursor and how
 * many there are. */
static void PouchBar(CtrPokeTouchView *v)
{
    int cursor = CtrBottomPouch_Cursor();
    unsigned item = CtrBottomPouch_Item(cursor);

    CtrBottomPouch_Title(v->bar[0], PT_BAR_LEN);
    if (item != 0) {
        CtrBottomBag_ItemName(item, v->bar[1], PT_BAR_LEN);
        snprintf(v->bar[2], PT_BAR_LEN, CTR_TR("Quantity  %u", "Cantidad  %u", "Anzahl  %u"), CtrBottomPouch_Quantity(cursor));
    }
}
