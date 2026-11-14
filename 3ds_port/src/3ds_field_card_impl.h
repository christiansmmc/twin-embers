/* The player's Trainer Card as a page (3ds_poketouch.h CtrPokeTouchCard; phase 2), included at the end of 3ds_poketouch.c
 * after the Pokedex, whose picture helpers it shares. A remote for FireRed's own card: the card is one window with the
 * player's frame, its front (the title and stars with the ID, FireRed's NAME, MONEY, POKeDEX and TIME, the trainer's
 * front picture, the eight badges at 2x in their places) or its back (FireRed's records); a touch anywhere on it turns
 * it over with the card's own key, and the inside is the light grey while held. The band says so; BACK is the card's
 * own exit. Every word, number and picture is the card's (CtrBottomCard_*, CtrPokeTouch_Card*), but for the port's
 * title and hint (3ds_ui_strings.h). */

int CtrPokeTouch_CardHit(const CtrPokeTouchView *v, int x, int y)
{
    return v->card.on && UiIn(&kCard, x, y);
}

/* A flat five-pointed star, 11 pixels across, for the card's stars (the port's own shape). */
static void FCStar(int x, int y, uint16_t c, uint16_t sh)
{
    static const char *const kStar[11] = {
        "     #     ", "    ###    ", "    ###    ", "###########", " ######### ", "  #######  ",
        "  #######  ", " ######### ", " ###   ### ", "##       ##", "           ",
    };

    for (int pass = 0; pass < 2; ++pass)
        for (int r = 0; r < 11; ++r)
            for (int q = 0; q < 11; ++q)
                if (kStar[r][q] == '#')
                    Px(x + q + (pass ? 0 : 1), y + r + (pass ? 0 : 1), pass ? c : sh);
}

static void FCTrainer(int x, int y)
{
    static unsigned char tiles[0x800];
    static unsigned short pal[16];
    static uint8_t px[64 * 64];
    static uint16_t pal565[16];

    if (!CtrPokeTouch_CardTrainerPic(tiles, pal))
        return;
    FDTiles(tiles, 64, 64, px);
    FDPal(pal, pal565);
    UiImage(px, pal565, 64, 64, 64, x, y, 1, 1);
}

/* The eight badges at twice their size in a row along the card's bottom, each in its place: FireRed's badge once earned,
 * an empty place (the light grey) before. */
static void FCBadges(const CtrPokeTouchCard *c)
{
    static unsigned char tiles[0x400];
    static unsigned short pal[16];
    static uint8_t px[128 * 16];
    static uint16_t pal565[16];
    int ok = CtrPokeTouch_CardBadges(tiles, pal);
    int x0 = kCard.x + 8 + (kCard.w - 16 - (8 * CARD_BADGE + 7 * CARD_BADGE_GAP)) / 2;
    int y = kCard.y + kCard.h - 8 - 4 - CARD_BADGE;

    if (ok) {
        FDTiles(tiles, 128, 16, px);
        FDPal(pal, pal565);
    }
    for (int i = 0; i < 8; ++i) {
        int sx = x0 + i * (CARD_BADGE + CARD_BADGE_GAP);

        if (ok && (c->badges & (1u << i)))
            UiImage(px + i * 16, pal565, 128, 16, 16, sx, y, 2, 1);
        else
            Fill(sx + 6, y + 6, CARD_BADGE - 12, CARD_BADGE - 12, KitInk(3));
    }
}

/* The front: the title (its first word only when the stars and the ID would not fit after it) with the stars, the ID,
 * a rule; NAME, MONEY, POKeDEX (once the player has it) and TIME; the trainer's picture; the badges. */
static void FCFront(const CtrPokeTouchCard *c, int x, int right)
{
    char text[24], label[24], title[24];
    int stars = c->stars < 5 ? c->stars : 5, top = kCard.y + 8, idLeft, y = top + 26;
    int picX = right - 64 - 4, valueRight = picX - 12;

    snprintf(text, sizeof(text), "%s%05u", c->label[1], (unsigned)c->id);
    idLeft = right - UiTextWidth(text, 1);
    KitText(text, idLeft, top, right);
    snprintf(title, sizeof(title), "%s", CtrUiText(UIS_CARD_TITLE));
    if (x + UiTextWidth(title, 1) + 8 + stars * 13 > idLeft - 4 && strchr(title, ' '))
        *strchr(title, ' ') = 0;
    KitText(title, x, top, idLeft - 4);
    for (int i = 0; i < stars; ++i)
        FCStar(x + UiTextWidth(title, 1) + 8 + i * 13, top + 2, RGB(248, 200, 48), KitInk(3));
    Fill(x, top + 18, right - x, 1, KitInk(3));
    /* NAME: FireRed's label ends in ": " */
    snprintf(label, sizeof(label), "%s", c->label[0]);
    for (int n = (int)strlen(label); n > 0 && (label[n - 1] == ' ' || label[n - 1] == ':'); --n)
        label[n - 1] = 0;
    SLine(x, valueRight, y, label, c->name);
    snprintf(text, sizeof(text), "%s%u", c->label[3], (unsigned)c->money);
    SLine(x, valueRight, y += 18, c->label[2], text);
    if (c->hasDex) {
        snprintf(text, sizeof(text), "%u", (unsigned)c->caught);
        SLine(x, valueRight, y += 18, c->label[4], text);
    }
    snprintf(text, sizeof(text), "%u:%02u", (unsigned)c->hours, (unsigned)c->minutes);
    SLine(x, valueRight, y += 18, c->label[5], text);
    FCTrainer(picX, top + 24);
    FCBadges(c);
}

/* The back: the player's name, a rule and FireRed's records. */
static void FCBack(const CtrPokeTouchCard *c, int x, int right)
{
    int top = kCard.y + 8;

    KitText(c->name, x, top, right);
    Fill(x, top + 18, right - x, 1, KitInk(3));
    for (int i = 0; i < c->rows && i < PT_CARD_ROWS; ++i)
        SLine(x, right, top + 26 + 24 * i, c->row[i].label, c->row[i].value);
}

static void FCardPageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchCard *c = &v->card;
    const CtrPokeTouchRect *r = &kCard;
    int x = r->x + 8, right = r->x + r->w - 8;

    Fill(0, 0, W, H, BPageBg());
    FPageWindow(r, v->frameType);
    if (c->pressed)
        Fill(x, r->y + 8, r->w - 16, r->h - 16, KitInk(3));
    if (c->back)
        FCBack(c, x, right);
    else
        FCFront(c, x, right);
    FPageBand(&kCardNote, CtrUiText(UIS_CARD_TITLE), CtrUiText(UIS_CARD_TURN), 0);
    if (v->backVisible)
        FPageBack(v, &kCardBack);
}
