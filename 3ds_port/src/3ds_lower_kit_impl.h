/* The lower screen's kit (phase 1 of the rework, docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md):
 * FireRed's own window frames, read from the player's pack, drawn the way the game draws them. Included by
 * 3ds_poketouch.c after its pixel helpers and the skin (EM_DARK, EM_DARK_SH). Nothing of the game is compiled in:
 * the tiles and palettes come from graphics/text_window/ in the pack, and without them a panel is a plain outline.
 *
 * Two frames, as FireRed uses them: the dialogue box's (menu_message, palette stdpal_0) for message and description
 * panels, and the standard frame the player picks in OPTIONS -> FRAME (typeN, 10 types) for lists and menus. */

enum { KIT_FRAME_DIALOGUE, KIT_FRAME_STD };

#define KIT_STD_TYPES 10

static int FrameLoadFile(const char *base, const char *ext, uint8_t *out, uint32_t cap, uint32_t want);

typedef struct {
    int type;                   /* the loaded std type, -1 for the dialogue frame; -2 nothing tried yet */
    int ok;
    uint16_t pal[16];           /* RGB565 */
    uint8_t tiles[18 * 32];     /* 4bpp, 8x8 tiles in the file's order */
} KitFrame;

static KitFrame sKitDialogue = { -2, 0, {0}, {0} }, sKitStd = { -2, 0, {0}, {0} };

static void KitPalette(const uint8_t *bytes, uint16_t *pal)
{
    for (int i = 0; i < 16; ++i)
        pal[i] = GbaColor((uint16_t)(bytes[i * 2] | bytes[i * 2 + 1] << 8));
}

/* The text colours of FireRed's windows: stdpal_0's 1 (white, the interior), 2 (the text), 3 (its shadow). */
UI_FN uint16_t KitInk(int index)
{
    static int tried, ok;
    static uint16_t pal[16];

    if (!tried) {
        uint8_t bytes[32];

        tried = 1;
        ok = FrameLoadFile("stdpal_0", "gbapal", bytes, sizeof(bytes), sizeof(bytes)) != 0;
        if (ok)
            KitPalette(bytes, pal);
    }
    if (ok && index >= 0 && index < 16)
        return pal[index];
    return index == 2 ? EM_DARK : index == 3 ? EM_DARK_SH : RGB(255, 255, 255);
}

UI_FN int KitFrameReady(int kind, int stdType)
{
    uint8_t bytes[32];

    if (kind == KIT_FRAME_DIALOGUE) {
        if (sKitDialogue.type == -2) {
            sKitDialogue.type = -1;
            sKitDialogue.ok = FrameLoadFile("menu_message", "4bpp", sKitDialogue.tiles, 18 * 32, 18 * 32)
                && FrameLoadFile("stdpal_0", "gbapal", bytes, sizeof(bytes), sizeof(bytes));
            if (sKitDialogue.ok)
                KitPalette(bytes, sKitDialogue.pal);
        }
        return sKitDialogue.ok;
    }
    if (stdType < 0 || stdType >= KIT_STD_TYPES)
        stdType = 0;                    /* FireRed resets an out-of-range frame to the first */
    if (sKitStd.type != stdType) {
        char base[16];

        sKitStd.type = stdType;
        snprintf(base, sizeof(base), "type%d", stdType + 1);
        sKitStd.ok = FrameLoadFile(base, "4bpp", sKitStd.tiles, 9 * 32, 9 * 32)
            && FrameLoadFile(base, "gbapal", bytes, sizeof(bytes), sizeof(bytes));
        if (sKitStd.ok)
            KitPalette(bytes, sKitStd.pal);
    }
    return sKitStd.ok;
}

/* A type's standard frame (OPTIONS -> FRAME, 0..9; out of range is type1) as the pack has it, for the top screen's
 * healthboxes (CtrHud_SetFrame): its nine 4bpp tiles, its GBA colours and stdpal_0's. False without the pack. */
bool CtrPokeTouch_FrameData(int type, uint8_t tiles[9 * 32], uint16_t framePal[16], uint16_t textPal[16])
{
    char base[16];
    uint8_t bytes[32];

    if (type < 0 || type >= KIT_STD_TYPES)
        type = 0;
    snprintf(base, sizeof(base), "type%d", type + 1);
    if (!FrameLoadFile(base, "4bpp", tiles, 9 * 32, 9 * 32) || !FrameLoadFile(base, "gbapal", bytes, 32, 32))
        return false;
    for (int i = 0; i < 16; ++i)
        framePal[i] = (uint16_t)(bytes[i * 2] | bytes[i * 2 + 1] << 8);
    if (!FrameLoadFile("stdpal_0", "gbapal", bytes, 32, 32))
        return false;
    for (int i = 0; i < 16; ++i)
        textPal[i] = (uint16_t)(bytes[i * 2] | bytes[i * 2 + 1] << 8);
    return true;
}

/* One 8x8 tile at (x, y), optionally flipped vertically; index 0 is left as it is. */
static void KitTile(const KitFrame *f, int tile, int vflip, int x, int y)
{
    const uint8_t *src = f->tiles + tile * 32;

    for (int py = 0; py < 8; ++py) {
        const uint8_t *row = src + (vflip ? 7 - py : py) * 4;

        for (int px = 0; px < 8; ++px) {
            uint8_t b = row[px / 2];
            uint8_t v = (px & 1) ? b >> 4 : b & 15;

            if (v)
                Px(x + px, y + py, f->pal[v]);
        }
    }
}

/* The standard frame: a ring one tile wide, FireRed's DrawStdFrameWithCustomTileAndPalette. */
static void KitStdFrame(const KitFrame *f, int x, int y, int cols, int rows)
{
    for (int c = 0; c < cols; ++c) {
        int edge = c == 0 ? 0 : c == cols - 1 ? 2 : 1;

        KitTile(f, edge, 0, x + c * 8, y);
        KitTile(f, 6 + edge, 0, x + c * 8, y + (rows - 1) * 8);
    }
    for (int r = 1; r < rows - 1; ++r) {
        KitTile(f, 3, 0, x, y + r * 8);
        KitTile(f, 5, 0, x + (cols - 1) * 8, y + r * 8);
    }
}

/* The dialogue frame: two tile columns on each side, FireRed's WindowFunc_DrawDialogueFrame. Its height there is six
 * tile rows; a taller panel repeats the upper middle row in its upper half and the lower middle row in its lower. */
static void KitDialogueFrame(const KitFrame *f, int x, int y, int cols, int rows)
{
    int middle = rows - 4, upper = (middle + 1) / 2;

    for (int r = 0; r < rows; ++r) {
        int base, vflip;

        if (r == 0) { base = 0; vflip = 0; }
        else if (r == 1) { base = 5; vflip = 0; }
        else if (r == rows - 1) { base = 0; vflip = 1; }
        else if (r == rows - 2) { base = 5; vflip = 1; }
        else { base = 10; vflip = r - 2 >= upper; }
        for (int c = 0; c < cols; ++c) {
            int tile;

            if (c == 0) tile = base;
            else if (c == 1) tile = base + 1;
            else if (c == cols - 2) tile = base == 0 ? 3 : base + (base == 5 ? 3 : 2);
            else if (c == cols - 1) tile = base == 0 ? 4 : base + (base == 5 ? 4 : 3);
            else if (base == 0) tile = 2;
            else continue;              /* the interior */
            KitTile(f, tile, vflip, x + c * 8, y + r * 8);
        }
    }
}

/* A panel whose outer edge is r (its width and height multiples of 8), the interior in palette index 1. */
UI_FN void KitPanel(const CtrPokeTouchRect *r, int kind, int stdType)
{
    int cols = r->w / 8, rows = r->h / 8;
    const KitFrame *f = kind == KIT_FRAME_DIALOGUE ? &sKitDialogue : &sKitStd;
    int side = kind == KIT_FRAME_DIALOGUE ? 2 : 1;

    if (!KitFrameReady(kind, stdType) || cols < side * 2 + 1 || rows < (kind == KIT_FRAME_DIALOGUE ? 4 : 3)) {
        Fill(r->x, r->y, r->w, r->h, KitInk(2));
        Fill(r->x + 1, r->y + 1, r->w - 2, r->h - 2, RGB(255, 255, 255));
        return;
    }
    Fill(r->x + side * 8, r->y + 8, r->w - side * 16, r->h - 16, f->pal[1]);
    if (kind == KIT_FRAME_DIALOGUE)
        KitDialogueFrame(f, r->x, r->y, cols, rows);
    else
        KitStdFrame(f, r->x, r->y, cols, rows);
}

/* Text the way FireRed's windows print it: its font, text colour 2 with shadow 3 on the white interior, a 14 px line
 * box, and its menu arrow (charmap 0xEF, 8 px wide) left of the chosen item. */
#define KIT_ARROW "\x01\xEF"
#define KIT_ARROW_W 8
#define KIT_LINE_H 14

/* One line whose glyph cells start at lineTop (UiText's `top` is the capitals' top, three rows lower). */
UI_FN void KitText(const char *s, int x, int lineTop, int limitX)
{
    UiText(s, x, lineTop + 3, KitInk(2), KitInk(3), 1, limitX);
}

static void KitTextOn(const char *s, int x, int lineTop, int limitX, int enabled)
{
    UiText(s, x, lineTop + 3, Tone(KitInk(2), enabled), Tone(KitInk(3), enabled), 1, limitX);
}

static int KitLineTop(const CtrPokeTouchRect *r)
{
    return r->y + (r->h - KIT_LINE_H) / 2;
}

/* A list row: the arrow at its left edge when chosen, the label 8 px to the right of it, cut at the row's end. */
UI_FN void KitRow(const CtrPokeTouchRect *r, const char *label, int selected, int enabled)
{
    int top = KitLineTop(r), limit = r->x + r->w;

    Fill(r->x, r->y, r->w, r->h, KitInk(1));
    if (selected)
        KitText(KIT_ARROW, r->x, top, limit);
    KitTextOn(label, r->x + KIT_ARROW_W, top, limit, enabled);
}

/* A cell with an icon at 1:1 (indexed pixels, 0 clear, like Art) centred above a one-line label; the arrow left of
 * the label when chosen, the shadow grey behind everything while pressed. */
UI_FN void KitIconCell(const CtrPokeTouchRect *r, const uint8_t *pixels, const uint16_t *pal, int iconW, int iconH,
                       const char *label, int selected, int pressed, int enabled)
{
    int lineTop = label ? r->y + r->h - 2 - KIT_LINE_H : r->y + r->h;

    Fill(r->x, r->y, r->w, r->h, pressed ? KitInk(3) : KitInk(1));
    if (pixels)
        Art(pixels, pal, iconW, 0, 0, iconW, iconH, r->x + (r->w - iconW) / 2, r->y + (lineTop - r->y - iconH) / 2,
            enabled);
    if (label) {
        int x = r->x + (r->w - UiTextWidth(label, 1)) / 2;

        if (x < r->x + KIT_ARROW_W)
            x = r->x + KIT_ARROW_W;
        if (selected)
            KitText(KIT_ARROW, x - KIT_ARROW_W, lineTop, r->x + r->w);
        KitTextOn(label, x, lineTop, r->x + r->w, enabled);
    }
}

/* One line with `left` from 4 px in and `right` ending 4 px before the end; `right` stays whole and `left` stops
 * 4 px before it. */
UI_FN void KitCaption(const CtrPokeTouchRect *r, const char *left, const char *right)
{
    int top = KitLineTop(r), end = r->x + r->w - 4, rightX = end;

    Fill(r->x, r->y, r->w, r->h, KitInk(1));
    if (right) {
        rightX = end - UiTextWidth(right, 1);
        if (rightX < r->x + 4)
            rightX = r->x + 4;
        KitText(right, rightX, top, end);
    }
    if (left)
        KitText(left, r->x + 4, top, right ? rightX - 4 : end);
}

/* FireRed's HP bar: a 4 px track, 2 px of colour by its thresholds on a 48 px scale (over 24 green, over 9 yellow,
 * else red). */
UI_FN void KitHpBar(int x, int y, int w, int hp, int maxHp)
{
    int inner = w - 2, level = maxHp > 0 ? hp * 48 / maxHp : 0;
    int fill = maxHp > 0 ? hp * inner / maxHp : 0;
    uint16_t c = level > 24 ? UI_HP_HIGH : level > 9 ? UI_HP_MID : UI_HP_LOW;

    if (hp > 0 && fill < 1)
        fill = 1;
    if (fill > inner)
        fill = inner;
    if (fill < 0)
        fill = 0;
    Fill(x, y, w, 4, KitInk(2));
    Fill(x + 1, y + 1, inner, 2, KitInk(3));
    if (fill)
        Fill(x + 1, y + 1, fill, 2, c);
}

/* A move's type: a flat block of the type's colour with its name in white. */
UI_FN void KitTypeLabel(const CtrPokeTouchRect *r, int type, const char *name)
{
    Fill(r->x, r->y, r->w, r->h, UiTypeColor(type));
    UiTextCentered(name, r, -1, RGB(255, 255, 255), KitInk(2), 1);
}

/* Text wrapped at spaces into at most maxLines lines `width` wide from lineTop, KIT_LINE_H apart; a word wider than
 * a line is cut at its end. Breaking only at spaces keeps UTF-8 and the raw-code escapes whole. Returns the lines
 * drawn. */
UI_FN int KitWrap(const char *s, int x, int lineTop, int width, int maxLines)
{
    char line[128];
    int lines = 0;

    while (lines < maxLines) {
        int fit = 0, end = 0;

        while (*s == ' ')
            ++s;
        if (!*s)
            break;
        for (;;) {
            while (s[end] && s[end] != ' ' && end < (int)sizeof(line) - 1)
                ++end;
            memcpy(line, s, (size_t)end);
            line[end] = 0;
            if (fit > 0 && UiTextWidth(line, 1) > width)
                break;
            fit = end;
            if (!s[end] || end >= (int)sizeof(line) - 1)
                break;
            ++end;                          /* past the space, to the next word */
        }
        memcpy(line, s, (size_t)fit);
        line[fit] = 0;
        KitText(line, x, lineTop + lines * KIT_LINE_H, x + width);
        ++lines;
        s += fit;
    }
    return lines;
}
