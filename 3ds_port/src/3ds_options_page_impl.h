/* OPTIONS and SAVE as pages (phase 2 of docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md): the port's own
 * screens (3ds_poketouch_options.c holds OPTIONS' rows; SAVE's facts are FireRed's save window's), drawn in the kit's
 * look where 3ds_lower_layout.h puts them. OPTIONS is FireRed's OPTION window: the title and PAGE, the page's rows (the
 * label, the value in FireRed's red between its arrows, or a row's levels side by side, the chosen one red), the look's
 * preview under a short page, the band with the row's description (and FRAME's sample) and BACK. SAVE is FireRed's
 * save window beside the player's own map icon, the dialogue in the band with YES / NO in the Party's window. Included
 * by 3ds_poketouch.c after the field pages' pieces. */

/* A row of levels: its values share the box, OPT_SEG_W wide in all (they need no arrows). */
#define OPT_SEG_X(boxX) ((boxX) + 2)
#define OPT_SEG_W (OPT_BOX_W - 4)

static int OptBoxX(void)
{
    CtrPokeTouchRect r = OptRowRect(0);

    return r.x + r.w - OPT_BOX_W;
}

bool CtrPokeTouch_OptionHit(int x, int y, int rows, int *row, int *zone)
{
    CtrPokeTouchRect first = OptRowRect(0);
    int boxX = OptBoxX(), r;

    if (x < first.x || x >= first.x + first.w || y < first.y)
        return false;
    r = (y - first.y) / OPT_ROW_H;
    if (r >= rows || r >= PT_OPT_MAX)
        return false;
    *row = r;
    *zone = x < boxX ? PT_OPT_ZONE_LABEL : x < boxX + OPT_ARROW_W ? PT_OPT_ZONE_PREV : PT_OPT_ZONE_NEXT;
    return true;
}

int CtrPokeTouch_OptionSegment(int x, int count)
{
    int at = x - OPT_SEG_X(OptBoxX()), k;

    if (count <= 0)
        return 0;
    k = at < 0 ? 0 : at * count / OPT_SEG_W;
    return k >= count ? count - 1 : k;
}

bool CtrPokeTouch_HitOptPage(int x, int y)
{
    return UiIn(&kOptPage, x, y);
}

/* FireRed's red text (its options' values) and its shadow. */
static void OptRedText(const char *s, int x, int lineTop, int limit)
{
    UiText(s, x, lineTop + 3, KitInk(4), KitInk(5), 1, limit);
}

/* One row: the light grey under the place held, FireRed's arrow on the row the band describes, the label, and in the
 * box the value between "<" and ">" (or the levels side by side, the chosen one red). */
static void OptPageRow(const CtrPokeTouchView *v, int i)
{
    CtrPokeTouchRect r = OptRowRect(i);
    int boxX = OptBoxX(), top = r.y + (r.h - KIT_LINE_H) / 2;
    int zone = v->optPressed == i ? v->optZone : PT_OPT_ZONE_NONE;

    Fill(r.x, r.y, r.w, r.h, KitInk(1));
    if (zone == PT_OPT_ZONE_LABEL)
        Fill(r.x, r.y, boxX - r.x, r.h, KitInk(3));
    if (v->optSelected == i)
        KitText(KIT_ARROW, r.x, top, r.x + KIT_ARROW_W);
    KitText(v->optLabel[i], r.x + KIT_ARROW_W, top, boxX - 4);
    if (v->optSegCount[i]) {
        int count = v->optSegCount[i], x0 = OPT_SEG_X(boxX);

        for (int k = 0; k < count && k < PT_OPT_SEGS; ++k) {
            int sx = x0 + k * OPT_SEG_W / count, sw = x0 + (k + 1) * OPT_SEG_W / count - sx;
            const char *name = v->optSegName[i][k];
            int tx = sx + (sw - UiTextWidth(name, 1)) / 2;

            if (zone == PT_OPT_ZONE_SEG + k)
                Fill(sx, r.y, sw, r.h, KitInk(3));
            if (k == v->optSegValue[i])
                OptRedText(name, tx, top, sx + sw);
            else
                KitTextOn(name, tx, top, sx + sw, 0);
        }
        return;
    }
    if (zone == PT_OPT_ZONE_PREV)
        Fill(boxX, r.y, OPT_ARROW_W, r.h, KitInk(3));
    if (zone == PT_OPT_ZONE_NEXT)
        Fill(boxX + OPT_ARROW_W, r.y, OPT_BOX_W - OPT_ARROW_W, r.h, KitInk(3));
    KitTriangle(boxX + OPT_ARROW_W / 2, r.y + r.h / 2, 2, KitInk(2));
    KitTriangle(boxX + OPT_BOX_W - OPT_ARROW_W / 2, r.y + r.h / 2, 3, KitInk(2));
    OptRedText(v->optValue[i], boxX + (OPT_BOX_W - UiTextWidth(v->optValue[i], 1)) / 2, top,
               boxX + OPT_BOX_W - OPT_ARROW_W);
}

/* Under a short page's rows (the looks), the look chosen for the selected one: its walking picture facing down, twice as
 * large (as it walks in the field), its name and its artists; left out where there is no room for the picture. */
static void OptLook(const CtrPokeTouchView *v)
{
    const CtrPokeTouchLook *l = &v->optLook;
    CtrPokeTouchRect last = OptRowRect(v->optCount);
    int x = last.x, right = last.x + last.w, y = last.y + 4, bottom = kOptList.y + kOptList.h - 8, textX = x + 72;

    if (bottom - y < 60)
        return;
    Fill(x, last.y + 1, last.w, 1, KitInk(3));
    if (l->has) {
        static uint8_t pixels[32 * 32];
        uint16_t pal[16];

        for (int py = 0; py < 32; ++py)
            for (int px = 0; px < 32; ++px)
                pixels[py * 32 + px] = (uint8_t)((l->tiles[(py / 8 * 4 + px / 8) * 32 + py % 8 * 4 + px % 8 / 2] >> (px & 1) * 4) & 15);
        for (int i = 0; i < 16; ++i)
            pal[i] = GbaColor(l->palette[i]);
        /* The first rows of a person are empty: the other 30 at twice the size. */
        UiImage(pixels + 2 * 32, pal, 32, 32, 30, x + 4, y + (bottom - y - 60) / 2, 2, 1);
    }
    KitText(l->name, textX, y + 2, right);
    KitWrap(l->credit, textX, y + 2 + KIT_LINE_H, right - textX, 3);
}

/* The band: the row's name and its description; FRAME's sample at its right while FRAME is the row (a window with that
 * frame and FireRed's YES). */
static void OptBand(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *n = &kOptNote;
    int x = n->x + 16, right = v->optFrameShow ? kOptSample.x - 4 : n->x + n->w - 16;
    char body[2 * PT_BAR_LEN + 2];

    snprintf(body, sizeof(body), "%s%s%s", v->bar[1], v->bar[1][0] && v->bar[2][0] ? " " : "", v->bar[2]);
    Fill(n->x, n->y, n->w, n->h, BPageBg());
    KitPanel(n, KIT_FRAME_DIALOGUE, 0);
    KitText(v->bar[0], x, n->y + 6, right);
    KitWrap(body, x, n->y + 6 + KIT_LINE_H, right - x, 2);
    if (v->optFrameShow) {
        const CtrPokeTouchRect *s = &kOptSample;
        int top = s->y + (s->h - KIT_LINE_H) / 2;

        KitPanel(s, KIT_FRAME_STD, v->optFrameType);
        KitText(KIT_ARROW, s->x + 8, top, s->x + s->w - 8);
        KitText(CtrUiText(UIS_YES), s->x + 8 + KIT_ARROW_W, top, s->x + s->w - 8);
    }
}

static void OptionsPageDraw(const CtrPokeTouchView *v)
{
    char page[24];

    Fill(0, 0, W, H, BPageBg());
    FPageWindow(&kOptTitle, v->frameType);
    KitText(CTR_TR("OPTION", "AJUSTES", "Option"), kOptTitle.x + 8, kOptTitle.y + 8, kOptTitle.x + kOptTitle.w - 8);
    snprintf(page, sizeof(page), CTR_TR("PAGE %u/%u", "P\xC3\x81G. %u/%u", "Seite %u/%u"), (unsigned)(v->optPage + 1),
             (unsigned)(v->optPageCount ? v->optPageCount : 1));
    BagCell(&kOptPage, v->frameType, page, v->optPagePressed, v->optPagePressed, v->optPageCount > 1);
    FPageWindow(&kOptList, v->frameType);
    for (int i = 0; i < v->optCount && i < PT_OPT_MAX; ++i)
        OptPageRow(v, i);
    if (v->optLook.on)
        OptLook(v);
    OptBand(v);
    if (v->backVisible)
        FPageBack(v, &kOptBack);
}

/* ---- SAVE ---- */
enum { SF_NAME, SF_BADGES, SF_DEX, SF_TIME, SF_MONEY, SF_COUNT };
static const char *const sSaveLabel[SF_COUNT] = {CTR_TR("PLAYER", "JUGADOR", "Spieler"), CTR_TR("BADGES", "MEDALLAS", "Orden"),
                                                 CTR_TR("POK\xc3\xa9" "DEX", "POK\xc3\xa9" "DEX", "Pok\xc3\xa9" "dex"),
                                                 CTR_TR("TIME", "TIEMPO", "Zeit"), CTR_TR("MONEY", "DINERO", "Geld")};

/* A line's value as FireRed's save window writes it (the name in capitals but in German). */
static void SaveValue(int f, const CtrPokeTouchView *v, char *text, int size)
{
    switch (f) {
    case SF_NAME:   snprintf(text, (size_t)size, "%s", v->playerName); break;
    case SF_BADGES: snprintf(text, (size_t)size, "%u", (unsigned)v->saveBadges); break;
    case SF_DEX:
        if (v->saveHasDex)
            snprintf(text, (size_t)size, "%u", (unsigned)v->saveDex);
        else
            snprintf(text, (size_t)size, "---");
        break;
    case SF_TIME:   snprintf(text, (size_t)size, "%u:%02u", (unsigned)(v->saveHours > 999 ? 999 : v->saveHours),
                             (unsigned)(v->saveMinutes > 59 ? 59 : v->saveMinutes)); break;
    default:        snprintf(text, (size_t)size, "\xc2\xa5%u", (unsigned)(v->saveMoney > 999999u ? 999999u : v->saveMoney)); break;
    }
#if !CTR_LANG_DE
    for (char *c = text; f == SF_NAME && *c; ++c)
        if (*c >= 'a' && *c <= 'z')
            *c = (char)(*c - ('a' - 'A'));
#endif
}

int CtrPokeTouch_SaveHit(int x, int y, int state)
{
    if (state == SAVE_WRITING)
        return PT_SAVE_NONE;
    if (state != SAVE_CONFIRM)          /* the result waits for its OK: anywhere */
        return x >= 0 && y >= 0 && x < W && y < H ? PT_SAVE_FIRST : PT_SAVE_NONE;
    for (int i = 0; i < 2; ++i) {
        CtrPokeTouchRect r = PartyMenuRow(2, i);

        if (UiIn(&r, x, y))
            return PT_SAVE_FIRST + i;
    }
    return PT_SAVE_NONE;
}

/* The player's own map icon (Red or Leaf) at four times its size, in the middle of r. */
static void SavePlayer(const CtrPokeTouchView *v, const CtrPokeTouchRect *r)
{
    const int scale = 4, x = r->x + (r->w - MARKER * scale) / 2, y = r->y + (r->h - MARKER * scale) / 2;

    if (sPlayer.key != v->playerFemale)
        IconBuild(&sPlayer, v->playerFemale, CtrPokeTouch_MapPlayerIcon);
    if (!sPlayer.ok)
        return;
    for (int py = 0; py < MARKER; ++py)
        for (int px = 0; px < MARKER; ++px) {
            uint8_t index = sPlayer.pixels[0][py * MARKER + px];

            if (index)
                Fill(x + px * scale, y + py * scale, scale, scale, sPlayer.pal[index]);
        }
}

static void SavePageDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchRect *info = &kSaveInfo;
    int x = info->x + 12, right = info->x + info->w - 12, confirm = v->saveState == SAVE_CONFIRM;
    char text[2 * PT_BAR_LEN + 2];

    Fill(0, 0, W, H, BPageBg());
    FPageWindow(info, v->frameType);
    for (int f = 0; f < SF_COUNT; ++f) {
        SaveValue(f, v, text, sizeof(text));
        KitText(sSaveLabel[f], x, info->y + 14 + 30 * f, right - UiTextWidth(text, 1) - 6);
        KitText(text, right - UiTextWidth(text, 1), info->y + 14 + 30 * f, right);
    }
    FPageWindow(&kSavePlayer, v->frameType);
    SavePlayer(v, &kSavePlayer);
    snprintf(text, sizeof(text), "%s%s%s", v->bar[0], v->bar[0][0] && v->bar[1][0] ? " " : "", v->bar[1]);
    FPageBand(confirm ? &kSaveNote : &kSaveNoteWide, NULL, text,
              v->saveState == SAVE_SUCCESS || v->saveState == SAVE_ERROR);
    if (confirm) {
        const char *names[2] = {CtrUiText(UIS_YES), CtrUiText(UIS_NO)};
        int held = v->savePressed == PT_SAVE_FIRST ? 0 : v->savePressed == PT_SAVE_SECOND ? 1 : -1;

        FPageMenu(v->frameType, names, 2, held >= 0 ? held : v->saveSel, held);
    }
}
