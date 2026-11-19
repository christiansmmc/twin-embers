/* The player's Trainer Card as the red device (def_card_front, def_card_back), included by 3ds_poketouch.c after SAVE:
 * a row of the field dispatcher (3ds_device_field_impl.h), shown while view->card.on and there is no GBA picture
 * (DevCardShown; a link card stays FireRed's picture). A remote for FireRed's own card (3ds_poketouch.h
 * CtrPokeTouchCard).
 *
 * The tall screen. On both sides the strip across its paper's top in the card's colour (FireRed's five Kanto colours
 * by its stars: blue, green, bronze, silver, gold), TRAINER CARD white in it and the four stars, those the card has in
 * gold, the rest the colour's light. Its front: NAME and the name, IDNo. and the ID across the first line; MONEY,
 * POKeDEX (with a Pokedex) and TIME left of the trainer's front picture; the eight badges' places, a badge earned at
 * 2x, one not yet a hole in the paper. Its back: the name, a rule and FireRed's records. Under the screen FLIP (a touch
 * of it, or of the screen, turns the card with the card's own key), sunk while the card is held, and BACK, a dark key.
 * Every rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): FireRed's labels (which v0.6.0 title-cased) and the port's title through DevUpper; the port's
 * FLIP and BACK in capitals; the name as typed; the numbers and the records' values as the card gives them. The title,
 * the name and IDNo. cut at their room are counted in sDevKeyCut.
 *
 * Redraw: the screen whole when what it draws differs (DevCardSame), and nothing else.
 *
 * FDTiles and FDPal (FireRed's 4bpp tiles and palettes as the canvas draws them) live here, the first of the B views to
 * need them; the Pokedex and the PC box, included after, use them too. */

static int DevCardShown(const CtrPokeTouchView *v)
{
    return v->card.on && !v->viewport;
}

int CtrPokeTouch_CardHit(const CtrPokeTouchView *v, int x, int y)
{
    return DevCardShown(v) && (UiIn(&kDevTallScreen, x, y) || DevOnKey(&kDevCardFlip, x, y));
}

/* ---- FireRed's pictures ---------------------------------------------------------------------------------------------- */

/* A w x h picture of 8x8 4bpp tiles in rows of w / 8 as palette indices, a row at a time. */
static void FDTiles(const unsigned char *tiles, int w, int h, uint8_t *px)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            unsigned byte = tiles[((y / 8) * (w / 8) + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2];
            px[y * w + x] = (uint8_t)((byte >> ((x & 1) * 4)) & 15);
        }
}

/* A GBA palette of 16 as the canvas's colours. */
static void FDPal(const unsigned short *in, uint16_t *out)
{
    for (int i = 0; i < 16; ++i) {
        unsigned c = in[i];
        out[i] = GbaColor(c);
    }
}

/* A flat five-pointed star, 11 pixels across (the port's), in c over its shadow sh 1 px right and down; each run of a
 * row a DevRect, so a board shows it. */
static void FCStar(int x, int y, uint16_t c, uint16_t sh)
{
    static const char *const kStar[11] = {
        "     #     ", "    ###    ", "    ###    ", "###########", " ######### ", "  #######  ",
        "  #######  ", " ######### ", " ###   ### ", "##       ##", "           ",
    };

    for (int pass = 0; pass < 2; ++pass)
        for (int r = 0; r < 11; ++r)
            for (int q = 0; q < 11;) {
                int n = 0;

                while (q + n < 11 && kStar[r][q + n] == '#')
                    ++n;
                if (n)
                    DevRect(x + q + !pass, y + r + !pass, n, 1, pass ? c : sh);
                q += n ? n : 1;
            }
}

/* The trainer's front picture (FireRed's 64x64) at its own size. */
static void DevCardTrainer(void)
{
    static unsigned char tiles[0x800];
    static unsigned short pal[16];
    static uint8_t px[64 * 64];
    static uint16_t pal565[16];
    const CtrPokeTouchRect *r = &kDevCardPic;

    DevArt(DEV_ART_PICTURE, r->x, r->y, r->w, r->h);
    if (!CtrPokeTouch_CardTrainerPic(tiles, pal))
        return;
    FDTiles(tiles, 64, 64, px);
    FDPal(pal, pal565);
    UiImage(px, pal565, 64, 64, 64, r->x, r->y, 1, 1);
}

/* The eight badges' places: a badge earned (FireRed's 16x16) at 2x, one not yet a hole in the paper. */
static void DevCardBadges(const CtrPokeTouchCard *c)
{
    static unsigned char tiles[0x400];
    static unsigned short pal[16];
    static uint8_t px[128 * 16];
    static uint16_t pal565[16];
    int ok = CtrPokeTouch_CardBadges(tiles, pal);

    if (ok) {
        FDTiles(tiles, 128, 16, px);
        FDPal(pal, pal565);
    }
    for (int i = 0; i < 8; ++i) {
        CtrPokeTouchRect r = DevCardBadge(i);

        if (!(c->badges & (1u << i))) {
            DevPaperHole(r.x, r.y, r.w);
            continue;
        }
        DevArt(DEV_ART_BADGE, r.x, r.y, r.w, r.h);
        if (ok)
            UiImage(px + i * 16, pal565, 128, 16, 16, r.x, r.y, r.w / 16, 1);
    }
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

#define DEV_CARD_WORD 32                            /* a label as drawn */
/* FireRed's Kanto card colours by the stars (0-4): blue, green, bronze, silver, gold (kDevCardColour; the harness's
 * colour table names them too). */
#define DEV_CARD_COLOURS UI_RGB(0x5A, 0x8A, 0xC8), UI_RGB(0x3E, 0x9A, 0x5A), UI_RGB(0xB8, 0x7A, 0x3A), \
                         UI_RGB(0x9A, 0x9A, 0xAA), UI_RGB(0xC8, 0xA0, 0x30)
#define DEV_CARD_STAR_ON UI_RGB(0xF8, 0xD0, 0x40)   /* a star the card has */
/* Alphas, of 256 (the board's 45 / 30 / 35 %): white over the colour for a star not had; black over it for the stars'
 * shadow and the title's. */
#define DEV_A_CARD_OFF      115
#define DEV_A_CARD_SH       77
#define DEV_A_CARD_TITLE_SH 90
#define DEV_CARD_TITLE_END  (DEV_CARD_STAR_X - 5)   /* 236: the title's room ends short of the stars */

static const uint16_t kDevCardColour[5] = {DEV_CARD_COLOURS};

static int DevCardStars(const CtrPokeTouchCard *c)
{
    return c->stars < DEV_CARD_STARS ? c->stars : DEV_CARD_STARS;
}

/* FireRed's card colour for `stars` (0-4; more counts as 4). */
static uint16_t DevCardColour(int stars)
{
    return kDevCardColour[stars < DEV_CARD_STARS ? stars : DEV_CARD_STARS];
}

static int DevCardRecords(const CtrPokeTouchCard *c)
{
    return c->rows < PT_CARD_ROWS ? c->rows : PT_CARD_ROWS;
}

/* The strip (either side): kDevCardStrip in the card's colour, the title white over the colour darkened from (22,
 * capitals 21) to DEV_CARD_TITLE_END, and the four stars: those the card has in gold, the rest the colour's light, each
 * over the colour darkened. */
static void DevCardStrip(const CtrPokeTouchCard *c)
{
    const CtrPokeTouchRect *r = &kDevCardStrip;
    uint16_t colour = DevCardColour(DevCardStars(c)), off = DevBlend(colour, DEV_WHITE, DEV_A_CARD_OFF);
    uint16_t sh = DevBlend(colour, 0, DEV_A_CARD_SH);
    char title[DEV_CARD_WORD];
    int titleX = r->x + 8;

    DevRect(r->x, r->y, r->w, r->h, colour);
    DevUpper(title, CtrUiText(UIS_CARD_TITLE), sizeof(title));
    if (DevTextWidth(title) > DEV_CARD_TITLE_END - titleX)
        ++sDevKeyCut;
    DevTextInk(title, titleX, r->y + 7, DEV_CARD_TITLE_END, DEV_WHITE, DevBlend(colour, 0, DEV_A_CARD_TITLE_SH));
    for (int k = 0; k < DEV_CARD_STARS; ++k) {
        int x = DEV_CARD_STAR_X + DEV_CARD_STAR_STEP * k;

        FCStar(x, DEV_CARD_STAR_Y, k < DevCardStars(c) ? DEV_CARD_STAR_ON : off, sh);
    }
}

/* The front's first line (DevCardRow(0), the labels' capitals a row under the values'): NAME (FireRed's label without
 * its ": ") small in the secondary ink, the name from DEV_CARD_NAME_X (or 6 past a wider NAME) cut 6 px before
 * IDNo., IDNo. (as FireRed writes it) small from DEV_CARD_ID_X, cut 6 px before the ID, and the ID ending at the line's
 * right. */
static void DevCardFirstLine(const CtrPokeTouchCard *c)
{
    CtrPokeTouchRect r = DevCardRow(0);
    int right = r.x + r.w, nameEnd = DEV_CARD_ID_X - 6, nameX, idX, idLabelEnd;
    char name[DEV_CARD_WORD], id[16];

    DevUpper(name, c->label[0], sizeof(name));
    for (int n = (int)strlen(name); n > 0 && (name[n - 1] == ' ' || name[n - 1] == ':'); --n)
        name[n - 1] = 0;
    snprintf(id, sizeof(id), "%05u", (unsigned)c->id);
    nameX = r.x + DevSmallTextWidth(name) + 6;
    if (nameX < DEV_CARD_NAME_X)
        nameX = DEV_CARD_NAME_X;
    idX = right - DevTextWidth(id);
    idLabelEnd = idX - 6;
    if (DevTextWidth(c->name) > nameEnd - nameX)
        ++sDevKeyCut;
    if (DevSmallTextWidth(c->label[1]) > idLabelEnd - DEV_CARD_ID_X)
        ++sDevKeyCut;
    DevSmallText(name, r.x, r.y + 4, DEV_INK2, nameEnd);
    DevText(c->name, nameX, r.y + 3, nameEnd);
    DevSmallText(c->label[1], DEV_CARD_ID_X, r.y + 4, DEV_INK2, idLabelEnd);
    DevText(id, idX, r.y + 3, right);
}

/* The front: the first line, MONEY, POKeDEX (with a Pokedex; else TIME takes its row) and TIME left of the trainer's
 * picture, and the badges' places. */
static void DevCardFront(const CtrPokeTouchCard *c)
{
    char label[DEV_CARD_WORD], value[40];
    CtrPokeTouchRect r;
    int row = 1;

    DevCardFirstLine(c);
    DevUpper(label, c->label[2], sizeof(label));
    snprintf(value, sizeof(value), "%s%u", c->label[3], (unsigned)c->money);
    r = DevCardRow(row++);
    DevColumnRow(r.y, label, value, r.x, r.x + r.w);
    if (c->hasDex) {
        DevUpper(label, c->label[4], sizeof(label));
        snprintf(value, sizeof(value), "%u", (unsigned)c->caught);
        r = DevCardRow(row++);
        DevColumnRow(r.y, label, value, r.x, r.x + r.w);
    }
    DevUpper(label, c->label[5], sizeof(label));
    snprintf(value, sizeof(value), "%u:%02u", (unsigned)c->hours, (unsigned)c->minutes);
    r = DevCardRow(row);
    DevColumnRow(r.y, label, value, r.x, r.x + r.w);
    DevCardTrainer();
    DevCardBadges(c);
}

/* The back: the name on the first line, the rule under it and FireRed's records (the label small at the left, the value
 * at the right). */
static void DevCardBack(const CtrPokeTouchCard *c)
{
    CtrPokeTouchRect r = DevCardRow(0);
    char label[DEV_CARD_WORD];

    DevText(c->name, r.x, r.y + 3, r.x + r.w);
    DevRect(kDevCardRule.x, kDevCardRule.y, kDevCardRule.w, kDevCardRule.h, DEV_RULE);
    for (int i = 0; i < DevCardRecords(c); ++i) {
        r = DevCardBackRow(i);
        DevUpper(label, c->row[i].label, sizeof(label));
        DevColumnRow(r.y, label, c->row[i].value, r.x, r.x + r.w);
    }
}

/* The card whole (after DevBegin: the dispatcher's): the screen with its strip and the side up, FLIP (sunk while the
 * card is held) and BACK. */
static void DevCardDraw(const CtrPokeTouchView *v)
{
    int sunk = v->card.pressed != 0, faceY;

    DevShell();
    DevScreen(&kDevTallScreen);
    DevCardStrip(&v->card);
    if (v->card.back)
        DevCardBack(&v->card);
    else
        DevCardFront(&v->card);
    faceY = DevKey(&kDevCardFlip, DEV_LOOK_KEY, sunk, 6);
    DevKeyWord(&kDevCardFlip, faceY, DEV_LOOK_KEY, sunk, CTR_TR("FLIP", "GIRAR", "UMDREHEN"));
    DevFieldBack(v, &kDevTallBack, DevBackWord());
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the card draws differs between `a` and `b` (the shell is the dispatcher's): the side up, the hold, the name,
 * the stars (the strip's colour and its stars, on either side), BACK, and the front's facts or the back's records. */
static int DevCardSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    const CtrPokeTouchCard *p = &a->card, *q = &b->card;

    if (p->back != q->back || !p->pressed != !q->pressed || strcmp(p->name, q->name)
        || DevCardStars(p) != DevCardStars(q) || !DevFieldBackSame(a, b))
        return 0;
    if (q->back) {
        if (DevCardRecords(p) != DevCardRecords(q))
            return 0;
        for (int i = 0; i < DevCardRecords(q); ++i)
            if (strcmp(p->row[i].label, q->row[i].label) || strcmp(p->row[i].value, q->row[i].value))
                return 0;
        return 1;
    }
    for (int i = 0; i < 6; ++i)
        if ((i != 4 || q->hasDex) && strcmp(p->label[i], q->label[i]))
            return 0;
    return p->id == q->id && p->money == q->money && p->hasDex == q->hasDex && (!q->hasDex || p->caught == q->caught)
        && p->hours == q->hours && p->minutes == q->minutes && p->badges == q->badges;
}

/* BACK as drawn now: the Summary's place while shown. */
static const CtrPokeTouchRect *DevCardBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevTallBack : NULL;
}
