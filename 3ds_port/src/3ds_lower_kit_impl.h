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
