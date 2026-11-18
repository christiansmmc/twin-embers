/* The player's Trainer Card as the red device (no board; plan 3), included by 3ds_poketouch.c after SAVE: a row of the
 * field dispatcher (3ds_device_field_impl.h), shown while view->card.on and there is no GBA picture (DevCardShown; a
 * link card stays FireRed's picture). A remote for FireRed's own card (3ds_poketouch.h CtrPokeTouchCard).
 *
 * The tall screen. Its front: NAME and the name, IDNo. and the ID across the first line; MONEY, POKeDEX (with a Pokedex)
 * and TIME beside the trainer's front picture; up to five stars; the eight badges' places, a badge earned at 2x, one not
 * yet an empty place. Its back: the name, a rule and FireRed's records. Under the screen FLIP (a touch of it, or of the
 * screen, turns the card with the card's own key), sunk while the card is held, and BACK, a dark key. Every rectangle is
 * 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): FireRed's labels (which v0.6.0 title-cased) through DevUpper; the port's FLIP and BACK in capitals;
 * the name as typed; the numbers and the records' values as the card gives them.
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
    return DevCardShown(v) && (UiIn(&kDevTallScreen, x, y) || DevBagOnKey(&kDevCardFlip, x, y));
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

/* The eight badges' places: a badge earned (FireRed's 16x16) at 2x, one not yet the paper's second tone. */
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
            DevRect(r.x, r.y, r.w, r.h, DEV_PAPER2);
            continue;
        }
        DevArt(DEV_ART_BADGE, r.x, r.y, r.w, r.h);
        if (ok)
            UiImage(px + i * 16, pal565, 128, 16, 16, r.x, r.y, r.w / 16, 1);
    }
}

/* ---- Drawing --------------------------------------------------------------------------------------------------------- */

#define DEV_CARD_WORD 32                        /* a label as drawn */
#define DEV_CARD_GOLD UI_RGB(0xE8, 0xB8, 0x30)  /* the stars */

static int DevCardStars(const CtrPokeTouchCard *c)
{
    return c->stars < DEV_CARD_STARS ? c->stars : DEV_CARD_STARS;
}

static int DevCardRecords(const CtrPokeTouchCard *c)
{
    return c->rows < PT_CARD_ROWS ? c->rows : PT_CARD_ROWS;
}

/* The front's first line: NAME (FireRed's label without its ": ") and the name at its left, IDNo. (as FireRed writes
 * it) and the ID at its right; the labels small in the secondary ink. */
static void DevCardFirstLine(const CtrPokeTouchCard *c)
{
    CtrPokeTouchRect r = DevCardRow(0);
    int smallTop = r.y + (r.h - 7) / 2, top = r.y + (r.h - UI_TEXT_H) / 2, right = r.x + r.w, idX, idLabelX;
    char name[DEV_CARD_WORD], id[16];

    DevUpper(name, c->label[0], sizeof(name));
    for (int n = (int)strlen(name); n > 0 && (name[n - 1] == ' ' || name[n - 1] == ':'); --n)
        name[n - 1] = 0;
    snprintf(id, sizeof(id), "%05u", (unsigned)c->id);
    idX = right - DevTextWidth(id);
    idLabelX = idX - 6 - DevSmallTextWidth(c->label[1]);
    DevSmallText(name, r.x, smallTop, DEV_INK2, right);
    DevText(c->name, r.x + DevSmallTextWidth(name) + 6, top, idLabelX - 6);
    DevSmallText(c->label[1], idLabelX, smallTop, DEV_INK2, idX);
    DevText(id, idX, top, right);
}

/* The front: the first line, MONEY, POKeDEX (with a Pokedex; else TIME takes its row) and TIME beside the trainer's
 * picture, the stars (five at most) and the badges' places. */
static void DevCardFront(const CtrPokeTouchCard *c)
{
    char label[DEV_CARD_WORD], value[40];
    CtrPokeTouchRect r;
    int row = 1;

    DevCardFirstLine(c);
    DevUpper(label, c->label[2], sizeof(label));
    snprintf(value, sizeof(value), "%s%u", c->label[3], (unsigned)c->money);
    r = DevCardRow(row++);
    DevLabelValue(&r, label, value);
    if (c->hasDex) {
        DevUpper(label, c->label[4], sizeof(label));
        snprintf(value, sizeof(value), "%u", (unsigned)c->caught);
        r = DevCardRow(row++);
        DevLabelValue(&r, label, value);
    }
    DevUpper(label, c->label[5], sizeof(label));
    snprintf(value, sizeof(value), "%u:%02u", (unsigned)c->hours, (unsigned)c->minutes);
    r = DevCardRow(row);
    DevLabelValue(&r, label, value);
    DevCardTrainer();
    for (int i = 0; i < DevCardStars(c); ++i)
        FCStar(DEV_CARD_STAR_X + DEV_CARD_STAR_STEP * i, DEV_CARD_STAR_Y, DEV_CARD_GOLD, DEV_INK_SH);
    DevCardBadges(c);
}

/* The back: the name, the rule under it and FireRed's records (the label small at the left, the value at the right). */
static void DevCardBack(const CtrPokeTouchCard *c)
{
    CtrPokeTouchRect r = DevCardRow(0);
    char label[DEV_CARD_WORD];

    DevText(c->name, r.x, r.y + (r.h - UI_TEXT_H) / 2, r.x + r.w);
    DevRect(kDevCardRule.x, kDevCardRule.y, kDevCardRule.w, kDevCardRule.h, DEV_INK_SH);
    for (int i = 0; i < DevCardRecords(c); ++i) {
        r = DevCardBackRow(i);
        DevUpper(label, c->row[i].label, sizeof(label));
        DevLabelValue(&r, label, c->row[i].value);
    }
}

/* The card whole (after DevBegin: the dispatcher's): the screen with the side up, FLIP (sunk while the card is held) and
 * BACK. */
static void DevCardDraw(const CtrPokeTouchView *v)
{
    int sunk = v->card.pressed != 0, faceY;

    DevShell(0);
    DevScreen(&kDevTallScreen);
    if (v->card.back)
        DevCardBack(&v->card);
    else
        DevCardFront(&v->card);
    faceY = DevKey(&kDevCardFlip, DEV_LOOK_KEY, sunk, 6);
    DevKeyWord(&kDevCardFlip, faceY, DEV_LOOK_KEY, sunk, CTR_TR("FLIP", "GIRAR", "UMDREHEN"));
    DevFieldBack(v, &kDevSumBack, DevBackWord());
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the card draws differs between `a` and `b` (the shell is the dispatcher's): the side up, the hold, the name,
 * BACK, and the front's facts or the back's records. */
static int DevCardSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    const CtrPokeTouchCard *p = &a->card, *q = &b->card;

    if (p->back != q->back || !p->pressed != !q->pressed || strcmp(p->name, q->name) || !DevFieldBackSame(a, b))
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
        && p->hours == q->hours && p->minutes == q->minutes && DevCardStars(p) == DevCardStars(q) && p->badges == q->badges;
}

/* BACK as drawn now: the Summary's place while shown. */
static const CtrPokeTouchRect *DevCardBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevSumBack : NULL;
}
