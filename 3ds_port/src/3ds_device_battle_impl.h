/* The battle's lower screen as the red device (style B; boards B · Comandos and B · Golpes), included by
 * 3ds_poketouch.c after the kit and HOME. It owns CtrBattleUi_Draw / Update / Hit; the battle Party and Bag keep the
 * ember look for now (3ds_battle_ui_impl.h: BEmberDraw / BEmberUpdate / BEmberHit). Every rectangle is
 * 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * ACTION, the commands: the shell with its four screws; the screen with the lead (its icon at 2x, its name, gender and
 * level, the party's six balls, its HP) and FireRed's question; FIGHT across; under it BAG, POKeMON and RUN, and in a
 * wild battle the quick ball between BAG and POKeMON (the ball's icon, how many, the arrows to the other balls; grey,
 * saying why, when it cannot throw, and still taken: the game says why too). RUN is grey and takes no touch when there
 * is no running. IDLE: the same screen without the question, the keys grey, nothing taken. MOVE: the top screws; the
 * screen with the chosen move (name, type, category, power and accuracy, FireRed's description), the four move keys
 * (name, type, the matchup, the PP) or a hole for each slot without a move, the hint and CANCEL. TARGET: the keys are
 * the battlers the move can be aimed at (name, level, HP, foe or ally, the matchup). SAFARI: BALL, BAIT, ROCK, RUN.
 *
 * The chosen key is the one under a held touch, else the game's cursor: the cursor always counts (showCursor is not
 * drawn). Words: upper case, as the boards and FireRed write them; the game's names through DevUpper, a name the player
 * gave and FireRed's descriptions as they are. The keys' words are the port's, written in capitals.
 *
 * Redraw (decision 7): a change of anything drawn redraws the screen whole; the icon's bounce (activeFrame) and
 * showCursor are not drawn. */

#include "3ds_type_chart.h"     /* CTR_FX_*: a move's matchup */
#include "3ds_battle_nav.h"     /* CTR_ACTION_BALL, CTR_QUICK_BALL_*: the quick ball */

#define DEV_BAT_FIGHT_INK   UI_RGB(0xA0, 0x2A, 0x20)   /* FIGHT's word */
#define DEV_BAT_BALL_STATUS UI_RGB(0xE0, 0xB0, 0x20)   /* the party's balls: a status, fainted, none (a ring); able is DEV_INK */
#define DEV_BAT_BALL_FAINT  UI_RGB(0xC8, 0xC8, 0xC0)
#define DEV_BAT_BALL_NONE   UI_RGB(0xB0, 0xB0, 0xA8)
#define DEV_BAT_PIP_HALF    UI_RGB(0xE0, 0xB0, 0x20)   /* a move's PP pips, at half and at a quarter (the board's) */
#define DEV_BAT_PIP_QUARTER UI_RGB(0xE0, 0x6A, 0x1E)
#define DEV_BAT_ICON        18                          /* a row key's art (the board's), the gap to its word */
#define DEV_BAT_ICON_GAP    6
#define DEV_BAT_LINE        14                          /* a line of the board's text (its capitals 3 px in) */
#define DEV_BAT_LEAD_X      20                          /* the lead's icon: {20, 18, 48, 48}, at 2x from its frame's (4, 8) */
#define DEV_BAT_LEAD_Y      18
#define DEV_BAT_SHADOW_A    115                         /* the hint's shadow on the shell (the board's 0.45 black) */

/* ---- The keys' art: v0.6.0's battle icons (its BDiscIcon's, without the disc) ------------------------------------------
 * BAG is FireRed's own bag from the user's pack (the port's art without it), the Poke Ball and the shoe the flat icons of
 * tools/make_battle_icons.py (battle_flat_icons.h, included by 3ds_poketouch.c), BAIT and ROCK the port's art
 * (3ds_icons.h). The row keys draw them at the board's 18 px, Safari's wider keys as they are. */
enum { BX_BAG, BX_BALL, BX_SHOE, BX_BAIT, BX_ROCK };
#define BBAG_MAX 48
static struct {
    int ready, ok, w, h;
    uint16_t rgb[BBAG_MAX * BBAG_MAX];
    uint8_t a[BBAG_MAX * BBAG_MAX];
} sBBag;

/* FireRed's bag (the ITEMS frame of graphics/interface/bag_male) at most `maxH` tall, its edge blended. */
static void BBagPrepare(int maxH)
{
    static uint8_t tiles[64 * 256 / 2];
    uint8_t colors[32], px[64 * 64];
    uint16_t pal[16];
    uint32_t len = 0;
    uint8_t *packed;
    int n = 0, x0 = 64, y0 = 64, x1 = -1, y1 = -1, bw, bh, tw, th;
    float step;

    if (sBBag.ready)
        return;
    sBBag.ready = 1;
    packed = CtrData_Load("graphics/interface/bag_male.4bpp.lz", &len);
    if (packed) {
        n = PokeTouch_Lz77(packed, len, tiles, sizeof(tiles));
        free(packed);
    }
    if (n < 64 * 64 / 2 || PokeTouch_LoadLz("graphics/interface/bag.gbapal.lz", colors, sizeof(colors)) != 32)
        return;
    for (int i = 1; i < 16; ++i) {
        unsigned c = (unsigned)colors[i * 2] | (unsigned)colors[i * 2 + 1] << 8;
        pal[i] = GbaColor(c);
    }
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            uint8_t b = tiles[(y / 8 * 8 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2], v = (x & 1) ? b >> 4 : b & 15;

            px[y * 64 + x] = v;
            if (v) {
                x0 = x < x0 ? x : x0; x1 = x > x1 ? x : x1;
                y0 = y < y0 ? y : y0; y1 = y > y1 ? y : y1;
            }
        }
    if (x1 < 0)
        return;
    bw = x1 - x0 + 1;
    bh = y1 - y0 + 1;
    step = bh > maxH ? (float)bh / maxH : 1.0f;
    tw = (int)(bw / step + 0.5f);
    th = (int)(bh / step + 0.5f);
    if (tw > BBAG_MAX || th > BBAG_MAX)
        return;
    /* Area-averaged: each pixel the colours of the source pixels it covers (only the drawn ones), its cover as alpha. */
    for (int oy = 0; oy < th; ++oy)
        for (int ox = 0; ox < tw; ++ox) {
            float fx0 = ox * step, fx1 = fx0 + step, fy0 = oy * step, fy1 = fy0 + step, wsum = 0, wop = 0, r = 0, g = 0, b = 0;

            for (int sy = (int)fy0; sy < bh && sy < fy1; ++sy)
                for (int sx = (int)fx0; sx < bw && sx < fx1; ++sx) {
                    float wx = (sx + 1 < fx1 ? sx + 1 : fx1) - (sx > fx0 ? sx : fx0);
                    float wy = (sy + 1 < fy1 ? sy + 1 : fy1) - (sy > fy0 ? sy : fy0);
                    float w = wx * wy;
                    uint8_t v = px[(y0 + sy) * 64 + x0 + sx];

                    wsum += w;
                    if (v) {
                        wop += w;
                        r += w * (pal[v] >> 11);
                        g += w * ((pal[v] >> 5) & 63);
                        b += w * (pal[v] & 31);
                    }
                }
            sBBag.a[oy * tw + ox] = wop > 0 && wsum > 0 ? (uint8_t)(wop / wsum * 255.0f + 0.5f) : 0;
            sBBag.rgb[oy * tw + ox] = wop > 0 ? (uint16_t)((int)(r / wop + 0.5f) << 11 | (int)(g / wop + 0.5f) << 5
                                                           | (int)(b / wop + 0.5f)) : 0;
        }
    sBBag.w = tw;
    sBBag.h = th;
    sBBag.ok = 1;
}

static void BFlat(const uint16_t *px, int w, int h, int x, int y)
{
    for (int v = 0; v < h; ++v)
        for (int u = 0; u < w; ++u)
            if (px[v * w + u])
                Px(x + u, y + v, px[v * w + u]);
}

/* Art at half its size, its top-left at (x, y): each 2x2 block one pixel, the colours of its drawn pixels averaged and
 * blended over what is there by how many are drawn. v0.6.0's flat icons (`flat`, RGB565, 0 clear) or the port's
 * paletted art (`px` into `pal`, 0 clear), `stride` pixels a row. */
static void DevBatHalfArt(const uint16_t *flat, const uint8_t *px, const uint16_t *pal, int stride, int w, int h, int x, int y)
{
    for (int oy = 0; oy < (h + 1) / 2; ++oy)
        for (int ox = 0; ox < (w + 1) / 2; ++ox) {
            int n = 0, r = 0, g = 0, b = 0;

            for (int k = 0; k < 4; ++k) {
                int sx = 2 * ox + (k & 1), sy = 2 * oy + (k >> 1);
                uint16_t c;

                if (sx >= w || sy >= h || (flat ? !flat[sy * stride + sx] : !px[sy * stride + sx]))
                    continue;
                c = flat ? flat[sy * stride + sx] : pal[px[sy * stride + sx]];
                r += c >> 11;
                g += (c >> 5) & 63;
                b += c & 31;
                ++n;
            }
            if (n)
                Px(x + ox, y + oy, DevBlend(DevGet(x + ox, y + oy),
                                            (uint16_t)((r + n / 2) / n << 11 | (g + n / 2) / n << 5 | (b + n / 2) / n), n * 64));
        }
}

/* A row key's art at the board's 18 px, centred on cx in the rows y..y + 17: the bag, the Poke Ball, the shoe. */
static void DevBatRowArt(int art, int cx, int y)
{
    if (art == BX_BAG && (BBagPrepare(DEV_BAT_ICON), sBBag.ok)) {
        int x0 = cx - sBBag.w / 2, y0 = y + (DEV_BAT_ICON - sBBag.h) / 2;

        for (int v = 0; v < sBBag.h; ++v)
            for (int u = 0; u < sBBag.w; ++u) {
                int a = sBBag.a[v * sBBag.w + u];

                if (a)
                    Px(x0 + u, y0 + v, DevBlend(DevGet(x0 + u, y0 + v), sBBag.rgb[v * sBBag.w + u], a + (a >> 7)));
            }
    } else if (art == BX_BAG) {
        const PtImage *im = &sBImg[BI_BAG].image;   /* the port's bag, without the pack */

        BIconsPrepare();
        DevBatHalfArt(NULL, im->pixels, im->pal, im->w, im->w, im->h, cx - (im->w + 1) / 4,
                      y + (DEV_BAT_ICON - (im->h + 1) / 2) / 2);
    } else if (art == BX_BALL) {
        DevBatHalfArt(kFlatBall, NULL, NULL, FLATBALL_W, FLATBALL_W, FLATBALL_H, cx - (FLATBALL_W + 1) / 4,
                      y + (DEV_BAT_ICON - (FLATBALL_H + 1) / 2) / 2);
    } else {
        DevBatHalfArt(kFlatShoe, NULL, NULL, FLATSHOE_W, FLATSHOE_W, FLATSHOE_H, cx - (FLATSHOE_W + 1) / 4,
                      y + (DEV_BAT_ICON - (FLATSHOE_H + 1) / 2) / 2);
    }
}

/* Safari's art as v0.6.0 drew it (its size into *w, *h), its top-left at (x, y); `draw` 0 only measures. */
static void DevBatFullArt(int art, int x, int y, int *w, int *h, int draw)
{
    if (art == BX_BALL || art == BX_SHOE) {
        *w = art == BX_BALL ? FLATBALL_W : FLATSHOE_W;
        *h = art == BX_BALL ? FLATBALL_H : FLATSHOE_H;
        if (draw)
            BFlat(art == BX_BALL ? kFlatBall : kFlatShoe, *w, *h, x, y);
        return;
    }
    BIconsPrepare();
    *w = sBImg[art == BX_BAIT ? BI_BAIT : BI_ROCK].image.w * sBImg[art == BX_BAIT ? BI_BAIT : BI_ROCK].scale;
    *h = sBImg[art == BX_BAIT ? BI_BAIT : BI_ROCK].image.h * sBImg[art == BX_BAIT ? BI_BAIT : BI_ROCK].scale;
    if (draw)
        BIconAt(art == BX_BAIT ? BI_BAIT : BI_ROCK, x, y);
}

/* ---- Where a touch goes: the keys of each state --------------------------------------------------------------------- */

/* Action `action`'s key (0-3, or CTR_ACTION_BALL) on the screen v shows; NULL for the quick ball when there is none (a
 * trainer, the Safari Zone). Draw and hit alike. */
static const CtrPokeTouchRect *DevBatActKey(const CtrBattleView *v, int action)
{
    if (action == BUI_ACT_FIGHT)
        return &kDevBatFight;
    if (v->ball == CTR_QUICK_BALL_NONE)
        return action == CTR_ACTION_BALL ? NULL : &kDevBatRow3[action == BUI_ACT_BAG ? 0 : action == BUI_ACT_POKEMON ? 1 : 2];
    return &kDevBatRow4[action == BUI_ACT_BAG ? 0 : action == CTR_ACTION_BALL ? 1 : action == BUI_ACT_POKEMON ? 2 : 3];
}

/* The action key a hit code of the commands is on: the quick ball's three parts (the throw, its arrows) are its key. */
static int DevBatKeyOfHit(int hit)
{
    return hit >= BUI_ACT_BALL && hit <= BUI_ACT_BALL_NEXT ? CTR_ACTION_BALL : hit;
}

/* The commands in reading order. */
static const int kDevBatActs[5] = {BUI_ACT_FIGHT, BUI_ACT_BAG, CTR_ACTION_BALL, BUI_ACT_POKEMON, BUI_ACT_RUN};

int CtrBattleUi_Hit(const CtrBattleView *v, int x, int y)
{
    if (v->state == BUI_PARTY || v->state == BUI_BAG)
        return BEmberHit(v, x, y);
    if (v->state == BUI_ACTION) {
        for (int i = 0; i < 5; ++i) {
            const CtrPokeTouchRect *r = DevBatActKey(v, kDevBatActs[i]);
            CtrPokeTouchRect a;

            if (!r || (a = DevKeyArea(r), !UiIn(&a, x, y)))
                continue;
            if (kDevBatActs[i] == BUI_ACT_RUN && v->runBlocked)
                return -1;
            if (kDevBatActs[i] != CTR_ACTION_BALL)
                return kDevBatActs[i];
            /* The arrows answer a little round themselves, on the key: it is narrow. Only while there are other balls. */
            for (int next = 0; next < 2 && v->ball == CTR_QUICK_BALL_READY && v->ballCount > 1; ++next) {
                CtrPokeTouchRect arrow = DevBatBallArrow(r, next);

                arrow = UiInset(&arrow, -DEV_BAT_ARROW_SLOP);
                if (UiIn(&arrow, x, y))
                    return next ? BUI_ACT_BALL_NEXT : BUI_ACT_BALL_PREV;
            }
            return BUI_ACT_BALL;            /* grey too: the game then says why it cannot throw */
        }
        return -1;
    }
    if (v->state == BUI_MOVE || v->state == BUI_TARGET || v->state == BUI_SAFARI) {
        CtrPokeTouchRect a;

        for (int i = 0; i < 4; ++i) {
            int here = v->state == BUI_MOVE ? v->move[i].present : v->state == BUI_TARGET ? v->target[i].present : 1;

            a = DevKeyArea(&kDevBatMove[i]);
            if (here && UiIn(&a, x, y))
                return i;
        }
        a = DevKeyArea(&kDevBatCancel);
        if (v->state != BUI_SAFARI && UiIn(&a, x, y))
            return BUI_CANCEL;
    }
    return -1;
}

/* ---- Words on the keys ------------------------------------------------------------------------------------------------- */

/* A port word on a key, centred in x..x + room with its capitals from `top`: cut with "..." and counted in sDevKeyCut
 * when it is wider than the room (DevKeyLabel's rule, on a box of its own). */
static void DevBatWord(int sunk, int x, int room, int top, const char *s)
{
    CtrPokeTouchRect box = {(int16_t)(x - 4), 0, (int16_t)(room + 8), UI_TEXT_H};

    DevKeyLabel(&box, top, DEV_LOOK_KEY, sunk, s);
}

static uint16_t DevBatInk(int sunk)
{
    return sunk ? DEV_SUNK_INK : DEV_KEY_INK;
}

static uint16_t DevBatInkShadow(int sunk)
{
    return sunk ? DEV_SUNK_INK_SH : DEV_WHITE;
}

/* ---- The commands (board B · Comandos) and IDLE ---------------------------------------------------------------------- */

/* The lead's icon at 2x: its frame 0's 24x24 from (4, 8), where FireRed's icons sit in their 32x32. */
static void DevBatLeadIcon(const CtrBattleView *v)
{
    if (!v->activeIcon || v->activeSlot >= 6 || !BPartyIcon(v->activeSlot, v->activeIcon))
        return;
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 24; ++x) {
            uint8_t i = sBPIcon[v->activeSlot].pixels[(8 + y) * 32 + 4 + x];

            if (i)
                Fill(DEV_BAT_LEAD_X + 2 * x, DEV_BAT_LEAD_Y + 2 * y, 2, 2, sBPIcon[v->activeSlot].pal[i]);
        }
}

/* The screen of the commands: the lead (its icon, name, gender and level, HP) when there is one, the party's six balls
 * (able, a status, fainted, a ring for none) and, when `prompt`, FireRed's question. */
static void DevBatLead(const CtrBattleView *v, int prompt)
{
    char text[64];
    int x = 76, w;

    DevScreen(&kDevBatScreen);
    for (int i = 0; i < 6; ++i) {
        int bx = 228 + 12 * i;

        if (!v->party[i])
            DevHomeRing(bx, 20, 9, DEV_BAT_BALL_NONE);
        else
            DevDisc(bx, 20, 9, v->party[i] == 2 ? DEV_BAT_BALL_FAINT : v->party[i] == 3 ? DEV_BAT_BALL_STATUS : DEV_INK);
    }
    if (!v->activeName[0])
        return;
    DevBatLeadIcon(v);
    DevText(v->activeName, 76, 20, 224);
    if (v->activeGender < 2) {
        DevGender(v->activeGender, 76, 34);
        x = 87;
    }
    snprintf(text, sizeof(text), CTR_TR("Lv%u", "Nv%u", "Lv%u"), (unsigned)v->activeLevel);
    DevText(text, x, 34, 224);
    w = DevLabelChip(76, 52, "HP", DEV_HP_FRAME, DEV_HP_LABEL);
    DevHpBar(76 + w + 3, 53, 150, 6, v->activeHp, v->activeMaxHp);
    if (v->activeMaxHp) {
        snprintf(text, sizeof(text), "%u/%u", (unsigned)v->activeHp, (unsigned)v->activeMaxHp);
        DevTextInk(text, 300 - DevTextWidth(text), 51, 300, DEV_INK, DEV_INK_SH);
    }
    if (prompt) {
        snprintf(text, sizeof(text), CTR_TR("What will %s do?", "\xC2\xBF" "Qu\xC3\xA9 har\xC3\xA1 %s?", "Was soll %s tun?"),
                 v->activeName);
        DevText(text, 20, 74, 300);
    }
}

/* FIGHT: its word at twice the size, FireRed's red. */
static void DevBatFightKey(int sunk)
{
    const CtrPokeTouchRect *r = &kDevBatFight;
    const char *word = CTR_TR("FIGHT", "LUCHAR", "KAMPF");
    int faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6), w = UiTextWidth(word, 2);

    if (w > r->w - 8)
        ++sDevKeyCut;
    UiText(word, r->x + (r->w - w) / 2, faceY + (r->h - 2 * UI_TEXT_H) / 2, DEV_BAT_FIGHT_INK, DevBatInkShadow(sunk), 2,
           r->x + r->w - 4);
}

/* BAG, POKeMON, RUN: the art over the word, the pair centred as the board stacks them. */
static void DevBatRowKey(const CtrPokeTouchRect *r, int sunk, int art, const char *word)
{
    int faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6), top = faceY + (r->h - (DEV_BAT_ICON + DEV_BAT_ICON_GAP + DEV_BAT_LINE)) / 2;

    DevBatRowArt(art, r->x + r->w / 2, top);
    DevBatWord(sunk, r->x + 4, r->w - 8, top + DEV_BAT_ICON + DEV_BAT_ICON_GAP + 3, word);   /* capitals 3 px into the line */
}

/* The quick ball: the ball's icon over how many there are, between the arrows to the other balls (only when there are
 * others). Grey (the caller dims it): the ball that would be thrown, or a Poke Ball, over why it cannot be. */
static void DevBatBallKey(const CtrBattleView *v, const CtrPokeTouchRect *r, int sunk)
{
    int faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6), cx = r->x + r->w / 2, row = faceY + 38;
    char text[16];

    if (v->ballItem)
        BItemDraw(v->ballItem, cx - 12, faceY + 7, 1);
    else
        DevBatHalfArt(kFlatBall, NULL, NULL, FLATBALL_W, FLATBALL_W, FLATBALL_H, cx - (FLATBALL_W + 1) / 4,
                      faceY + 19 - (FLATBALL_H + 1) / 4);
    if (v->ball != CTR_QUICK_BALL_READY) {
        const char *why = v->ball == CTR_QUICK_BALL_FULL ? CTR_TR("BOX FULL", "CAJA LLENA", "BOX VOLL")
                        : v->ball == CTR_QUICK_BALL_AREA ? CTR_TR("AREA USED", "ZONA USADA", "GEBIET BELEGT")
                        : CTR_TR("NO BALLS", "SIN BALLS", "KEIN BALL");

        /* the normal font, or the small one where it does not fit (HOME's rule: a word that fits neither is counted cut) */
        DevHomeWord(why, DevHomeFont(why, r->w - 8, r->w - 8), r->x + 4, r->w - 8, row, 20, 1, DevBatInk(sunk),
                    DevBatInkShadow(sunk));
        return;
    }
    snprintf(text, sizeof(text), "x%u", (unsigned)v->ballQty);
    DevTextInk(text, cx - DevTextWidth(text) / 2, row + 6, r->x + r->w, DevBatInk(sunk), DevBatInkShadow(sunk));
    for (int next = 0; next < 2 && v->ballCount > 1; ++next) {
        CtrPokeTouchRect a = DevBatBallArrow(r, next);

        DevTriangle(a.x + a.w / 2, faceY - r->y + a.y + a.h / 2, next ? 3 : 2, DevBatInk(sunk));
    }
}

/* ACTION, or IDLE (`idle`: no question, every key grey and none chosen). */
static void DevBatCommands(const CtrBattleView *v, int idle)
{
    int cursor = idle ? -1 : v->cursor, pressed = idle ? -1 : DevBatKeyOfHit(v->pressed);

    DevShell(DEV_SCREWS_TOP | DEV_SCREWS_BOTTOM);
    DevBatLead(v, !idle);
    for (int i = 0; i < 5; ++i) {
        int act = kDevBatActs[i], sunk = DevChosen(act, cursor, pressed);
        const CtrPokeTouchRect *r = DevBatActKey(v, act);

        if (!r)
            continue;
        if (act == BUI_ACT_FIGHT)
            DevBatFightKey(sunk);
        else if (act == CTR_ACTION_BALL)
            DevBatBallKey(v, r, sunk);
        else if (act == BUI_ACT_BAG)
            DevBatRowKey(r, sunk, BX_BAG, CTR_TR("BAG", "MOCHILA", "BEUTEL"));
        else if (act == BUI_ACT_POKEMON)
            DevBatRowKey(r, sunk, BX_BALL, CTR_TR("POK\xC3\xA9MON", "POK\xC3\xA9MON", "POK\xC3\xA9MON"));
        else
            DevBatRowKey(r, sunk, BX_SHOE, CTR_TR("RUN", "HUIR", "FLUCHT"));
        if (idle || (act == BUI_ACT_RUN && v->runBlocked) || (act == CTR_ACTION_BALL && v->ball != CTR_QUICK_BALL_READY))
            DevDim(r);
    }
}

/* ---- The moves (board B · Golpes), the targets, Safari ------------------------------------------------------------------ */

/* A move's effectiveness against the foe as a chip at (x, y), in v0.6.0's words; none for CTR_FX_NONE. Returns whether
 * there is one. */
static int DevBatMatchup(uint8_t fx, int x, int y)
{
    const char *word;
    uint16_t bg, shadow;

    if (fx == CTR_FX_DOUBLE || fx == CTR_FX_QUADRUPLE) {
        word = fx == CTR_FX_DOUBLE ? CTR_TR("SUPER", "S\xC3\x9APER", "SEHR") : CTR_TR("SUPER x4", "S\xC3\x9APER x4", "SEHR x4");
        bg = DEV_FX_SUPER;
        shadow = DEV_FX_SUPER_SH;
    } else if (fx == CTR_FX_HALF || fx == CTR_FX_QUARTER) {
        word = CTR_TR("NOT VERY", "POCO EFIC.", "NICHT SEHR");
        bg = DEV_FX_NOTVERY;
        shadow = DEV_FX_NOTVERY_SH;
    } else if (fx == CTR_FX_NEUTRAL) {
        word = CTR_TR("EFFECTIVE", "EFICAZ", "EFFEKTIV");
        bg = DEV_FX_EFFECTIVE;
        shadow = DEV_FX_EFFECTIVE_SH;
    } else if (fx == CTR_FX_IMMUNE) {
        word = CTR_TR("NO EFFECT", "NO AFECTA", "WIRKUNGSLOS");
        bg = DEV_FX_NONE;
        shadow = DevChipShadow(DEV_FX_NONE);
    } else
        return 0;
    DevChip(x, y, 0, 12, word, bg, shadow);
    return 1;
}

/* A move with no power ("-") is a status move: no category, "STATUS" on its key when no matchup is shown. */
static int DevBatStatusMove(const CtrBattleMove *m)
{
    return !m->power[0] || m->power[0] == '-';
}

/* "LABEL value" (the label small in the secondary ink, the value in the paper's ink) ending at `right`, on the move
 * screen's first line; returns where it starts. */
static int DevBatStat(const char *label, const char *value, int right)
{
    int x = right - DevTextWidth(value);

    DevTextInk(value, x, 18, right, DEV_INK, DEV_INK_SH);
    x -= 3 + DevSmallTextWidth(label);
    DevSmallText(label, x, 19, DEV_INK2, right);
    return x;
}

/* The move screen: the chosen move's name, type and category chips, its power and accuracy; under them FireRed's
 * description in two lines, or without one STAB and the chance of its extra effect. */
static void DevBatMoveScreen(const CtrBattleView *v)
{
    int chosen = v->pressed >= 0 && v->pressed < 4 ? v->pressed : v->cursor, x, room, typeW, catW = 0, nameW;
    const CtrBattleMove *m;
    const char *cat = NULL;
    char name[BUI_NAME_LEN + 8], type[24], line[48];
    uint16_t typeBg;

    DevScreen(&kDevBatMoveScreen);
    if (chosen < 0 || chosen > 3 || !v->move[chosen].present)
        return;
    m = &v->move[chosen];
    DevUpper(name, m->name, sizeof(name));
    DevUpper(type, m->typeName, sizeof(type));
    typeBg = UiTypeColor(m->type);
    if (!DevBatStatusMove(m))
        cat = m->type < 10 ? CTR_TR("PHYSICAL", "F\xC3\x8DSICO", "PHYSISCH") : CTR_TR("SPECIAL", "ESPECIAL", "SPEZIAL");
    x = DevBatStat(CTR_TR("ACC", "PRE", "GEN"), m->accuracy[0] ? m->accuracy : "-", 300);
    x = DevBatStat(CTR_TR("POW", "POT", "ST\xC3\x84"), m->power[0] ? m->power : "-", x - 6);
    typeW = DevSmallTextWidth(type) + 8;
    if (cat)
        catW = DevSmallTextWidth(cat) + 8 + 4;
    room = x - 6 - 20 - (typeW + 4) - catW;
    nameW = DevTextWidth(name) < room ? DevTextWidth(name) : room > 0 ? room : 0;
    DevTextInk(name, 20, 18, 20 + nameW, DEV_INK, DEV_INK_SH);
    x = 20 + nameW + 4;
    DevChip(x, 16, 0, 12, type, typeBg, DevChipShadow(typeBg));
    if (cat)
        DevChip(x + typeW + 4, 16, 0, 12, cat, m->type < 10 ? DEV_CAT_PHYSICAL : DEV_CAT_SPECIAL,
                m->type < 10 ? DevChipShadow(DEV_CAT_PHYSICAL) : DEV_CAT_SPECIAL_SH);
    if (m->desc[0]) {
        DevWrap(m->desc, 20, 34, 280, 2, DEV_INK2, DEV_INK2);
        return;
    }
    x = 20;
    if (m->stab) {
        DevText2("STAB", x, 34, 300);
        x += DevTextWidth("STAB") + 8;
    }
    if (m->effectChance) {
        snprintf(line, sizeof(line), CTR_TR("EFFECT %u%%", "EFECTO %u%%", "EFFEKT %u%%"), (unsigned)m->effectChance);
        DevText2(line, x, 34, 300);
    }
}

/* PP as five pips (the board's): as many filled as the share left, rounded (at least one while any is left), green, or
 * at half and at a quarter FireRed's PP colours tell. */
static void DevBatPips(const CtrBattleMove *m, int x, int y)
{
    uint16_t level = DevPpInk(m->pp, m->maxPp, DEV_HP_GREEN);
    uint16_t fill = level == DEV_PP_HALF ? DEV_BAT_PIP_HALF : level == DEV_PP_QUARTER ? DEV_BAT_PIP_QUARTER : DEV_HP_GREEN;
    int n = m->maxPp ? (m->pp * 10 + m->maxPp) / (2 * m->maxPp) : 0;

    if (m->pp && n < 1)
        n = 1;
    for (int i = 0; i < 5; ++i)
        Fill(x + 6 * i, y, 5, 5, i < n ? fill : DEV_KEY_EDGE);
}

/* "PP n/m" ending at `right`, the numbers in FireRed's PP colours, capitals from `top`. */
static void DevBatPp(const CtrBattleMove *m, int right, int top, int sunk)
{
    char text[16];
    int x;

    snprintf(text, sizeof(text), "%u/%u", (unsigned)m->pp, (unsigned)m->maxPp);
    x = right - DevTextWidth(text);
    DevTextInk(text, x, top, right, DevPpInk(m->pp, m->maxPp, DevBatInk(sunk)), DevBatInkShadow(sunk));
    DevSmallText("PP", x - 3 - DevSmallTextWidth("PP"), top + 1, DEV_INK2, x);
}

/* A move key: its name, type chip, matchup chip (or "STATUS"), PP pips and PP; a hole for a slot without a move. */
static void DevBatMoveKey(const CtrBattleView *v, int i)
{
    const CtrPokeTouchRect *r = &kDevBatMove[i];
    const CtrBattleMove *m = &v->move[i];
    int sunk = DevChosen(i, v->cursor, v->pressed), faceY;
    char name[BUI_NAME_LEN + 8], type[24];
    uint16_t typeBg = UiTypeColor(m->type);

    if (!m->present) {
        DevHole(r, 6);
        return;
    }
    faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);
    DevUpper(name, m->name, sizeof(name));
    DevUpper(type, m->typeName, sizeof(type));
    DevTextInk(name, r->x + 10, faceY + 11, r->x + r->w - 18, DevBatInk(sunk), DevBatInkShadow(sunk));
    DevChip(r->x + 10, faceY + 25, 40, 12, type, typeBg, DevChipShadow(typeBg));
    if (!DevBatMatchup(m->matchup, r->x + 54, faceY + 25) && DevBatStatusMove(m))
        DevSmallText(CTR_TR("STATUS", "ESTADO", "STATUS"), r->x + 54, faceY + 27, DEV_INK2, r->x + r->w - 10);
    DevBatPips(m, r->x + 10, faceY + 46);
    DevBatPp(m, r->x + r->w - 10, faceY + 45, sunk);
}

/* A target key: the battler's name and level, foe or ally, the move's matchup against it, its HP bar (and the ally's
 * numbers, as FireRed shows them); a hole where the move cannot be aimed. */
static void DevBatTargetKey(const CtrBattleView *v, int i)
{
    const CtrPokeTouchRect *r = &kDevBatMove[i];
    const CtrBattleTarget *t = &v->target[i];
    int sunk = DevChosen(i, v->cursor, v->pressed), faceY, lvW;
    uint16_t side = t->side ? DEV_BLUE_INK : DEV_STAT_UP;
    char text[16];

    if (!t->present) {
        DevHole(r, 6);
        return;
    }
    faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6);
    snprintf(text, sizeof(text), CTR_TR("Lv%u", "Nv%u", "Lv%u"), (unsigned)t->level);
    lvW = DevTextWidth(text);
    DevTextInk(text, r->x + r->w - 20 - lvW, faceY + 11, r->x + r->w - 20, DevBatInk(sunk), DevBatInkShadow(sunk));
    DevTextInk(t->name, r->x + 10, faceY + 11, r->x + r->w - 24 - lvW, DevBatInk(sunk), DevBatInkShadow(sunk));
    DevChip(r->x + 10, faceY + 25, 40, 12, t->side ? CTR_TR("ALLY", "ALIADO", "PARTNER") : CTR_TR("FOE", "RIVAL", "GEGNER"),
            side, DevChipShadow(side));
    DevBatMatchup(t->matchup, r->x + 54, faceY + 25);
    DevHpBar(r->x + 10, faceY + 46, 72, 5, t->hp, t->maxHp);
    if (t->side) {
        snprintf(text, sizeof(text), "%u/%u", (unsigned)t->hp, (unsigned)t->maxHp);
        DevTextInk(text, r->x + r->w - 10 - DevTextWidth(text), faceY + 45, r->x + r->w - 10, DevBatInk(sunk),
                   DevBatInkShadow(sunk));
    }
}

/* MOVE and TARGET: the top screws, the screen, the four keys, the hint and CANCEL. */
static void DevBatMoves(const CtrBattleView *v)
{
    int sunk = DevChosen(BUI_CANCEL, v->cursor, v->pressed), faceY;

    DevShell(DEV_SCREWS_TOP);
    if (v->state == BUI_MOVE)
        DevBatMoveScreen(v);
    else {
        DevScreen(&kDevBatMoveScreen);
        DevText(CTR_TR("TARGET", "OBJETIVO", "ZIEL"), 20, 18, 300);
        DevWrap(CTR_TR("Which POK\xC3\xA9MON should the move hit?", "\xC2\xBF" "A qu\xC3\xA9 POK\xC3\xA9MON ataca?",
                       "Welches POK\xC3\xA9MON angreifen?"), 20, 34, 280, 2, DEV_INK2, DEV_INK2);
    }
    for (int i = 0; i < 4; ++i) {
        if (v->state == BUI_MOVE)
            DevBatMoveKey(v, i);
        else
            DevBatTargetKey(v, i);
    }
    /* the hint: the small font in white on the shell, its capitals at 218 (glyph row 0 four rows above them) */
    DevGlyphs(CTR_TR("A: USE   B: BACK", "A: USAR   B: VOLVER", "A: NUTZEN   B: ZUR\xC3\x9C" "CK"), 12, 218 - 4,
              kDevBatCancel.x - 4, DEV_WHITE, DevBlend(sDevShell, 0x0000, DEV_BAT_SHADOW_A), 1);
    faceY = DevKey(&kDevBatCancel, DEV_LOOK_DARK, sunk, 10);
    DevKeyLabel(&kDevBatCancel, faceY, DEV_LOOK_DARK, sunk, CTR_TR("CANCEL", "CANCELAR", "ZUR\xC3\x9C" "CK"));
}

/* SAFARI: FireRed's question and its four, BALL BAIT ROCK RUN, each v0.6.0's art beside the word. */
static void DevBatSafari(const CtrBattleView *v)
{
    static const int kArt[4] = {BX_BALL, BX_BAIT, BX_ROCK, BX_SHOE};
    const char *const word[4] = {CTR_TR("BALL", "BALL", "BALL"), CTR_TR("BAIT", "CEBO", "K\xC3\x96" "DER"),
                                 CTR_TR("ROCK", "ROCA", "STEIN"), CTR_TR("RUN", "HUIR", "FLUCHT")};
    CtrPokeTouchRect paper;

    DevShell(DEV_SCREWS_TOP | DEV_SCREWS_BOTTOM);
    paper = DevScreen(&kDevBatMoveScreen);
    DevText(CTR_TR("What will you do?", "\xC2\xBF" "Qu\xC3\xA9 vas a hacer?", "Was tun?"), 20,
            paper.y + (paper.h - UI_TEXT_H) / 2, 300);
    for (int i = 0; i < 4; ++i) {
        const CtrPokeTouchRect *r = &kDevBatMove[i];
        int sunk = DevChosen(i, v->cursor, v->pressed), faceY = DevKey(r, DEV_LOOK_KEY, sunk, 6), aw, ah, ww, x0;

        DevBatFullArt(kArt[i], 0, 0, &aw, &ah, 0);
        ww = DevTextWidth(word[i]);
        if (ww > r->w - 8 - aw - 8)
            ww = r->w - 8 - aw - 8;
        x0 = r->x + (r->w - (aw + 8 + ww)) / 2;
        DevBatFullArt(kArt[i], x0, faceY + (r->h - ah) / 2, &aw, &ah, 1);
        DevBatWord(sunk, x0 + aw + 8, ww, faceY + (r->h - UI_TEXT_H) / 2, word[i]);
    }
}

/* ---- The battle's lower screen ------------------------------------------------------------------------------------------ */

void CtrBattleUi_Draw(uint16_t *canvas, const CtrBattleView *v)
{
    if (v->state == BUI_PARTY || v->state == BUI_BAG) {
        BEmberDraw(canvas, v);
        return;
    }
    DevBegin(canvas, v->shell);
    if (v->state == BUI_MOVE || v->state == BUI_TARGET)
        DevBatMoves(v);
    else if (v->state == BUI_SAFARI)
        DevBatSafari(v);
    else
        DevBatCommands(v, v->state != BUI_ACTION);
}

/* From one Party to the next or one Bag to the next, the ember look's own; anything else: what a red device state draws
 * (all but the icon's bounce and showCursor) changed, the screen whole. */
void CtrBattleUi_Update(uint16_t *canvas, const CtrBattleView *old, const CtrBattleView *v, CtrDirtyList *dirty)
{
    CtrBattleView still;

    if ((old->state == BUI_PARTY && v->state == BUI_PARTY) || (old->state == BUI_BAG && v->state == BUI_BAG)) {
        BEmberUpdate(canvas, old, v, dirty);
        return;
    }
    CtrDirty_Clear(dirty);
    still = *old;
    still.activeFrame = v->activeFrame;
    still.showCursor = v->showCursor;
    if (memcmp(&still, v, sizeof(still))) {
        CtrBattleUi_Draw(canvas, v);
        CtrDirty_Add(dirty, 0, 0, W, H);
    }
}
