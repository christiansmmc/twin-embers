/* The lower screen kit (3ds_lower_kit_impl.h) drawn by the real renderer, with synthetic text-window
 * entries in place of the pack: no ROM-derived fixtures. Run with no argument for the kit's tests, with
 * "fallback" for a pack that has no text-window entries at all (a fresh process, so nothing is cached). */
#include <assert.h>
#ifndef PT_SOURCE
#define PT_SOURCE "3ds_poketouch.c"
#endif
#include PT_SOURCE

static int fixtureFail;
static uint8_t font[256 * 64], fontWidth[256];
const uint8_t *CtrTwinFontData(void) { return font; }
unsigned CtrTwinFontWidth(unsigned code) { return code < 256 && fontWidth[code] ? fontWidth[code] : 5; }

/* One pixel of a synthetic glyph, in the game's layout (TwinGlyphDecode reads it back). */
static void GlyphPixel(unsigned code, int x, int y, unsigned value)
{
    uint8_t *src = font + code * 64 + ((y / 8) * 2 + x / 8) * 16 + (y % 8) * 2;
    unsigned bits = (unsigned)src[1] << 8 | src[0];

    bits |= value << ((7 - x % 8) * 2);
    src[0] = (uint8_t)bits;
    src[1] = (uint8_t)(bits >> 8);
}

/* The arrow (0xEF) a solid 8x14 block of text ink; every letter, digit and sign 6 wide with ink in columns 0-4 on
 * the capitals' rows 3-11. */
static void SyntheticFont(void)
{
    for (unsigned code = 0xA1; code < 0xEF; ++code) {
        fontWidth[code] = 6;
        for (int y = 3; y <= 11; ++y)
            for (int x = 0; x <= 4; ++x)
                GlyphPixel(code, x, y, 1);
    }
    fontWidth[0xEF] = 8;
    for (int y = 0; y < 14; ++y)
        for (int x = 0; x < 8; ++x)
            GlyphPixel(0xEF, x, y, 1);
}
const uint8_t *CtrTwinJapaneseGlyph(unsigned code, uint8_t out[64]) { (void)code; memset(out, 0, 64); return out; }
unsigned CtrTwinJapaneseFontWidth(unsigned code) { (void)code; return 8; }
static uint8_t smallFont[256 * 32];
const uint8_t *CtrTwinSmallFontData(void) { return smallFont; }
unsigned CtrTwinSmallFontWidth(unsigned code) { (void)code; return 5; }

/* Tile t of a synthetic frame: index 2 + t % 13 in its top four rows, 3 + t % 13 in its bottom four. Never 0
 * (every pixel drawn) or 1 (the interior), so a tile, its flip and the interior are told apart. */
static int TopIndex(int t) { return 2 + t % 13; }
static int BottomIndex(int t) { return 3 + t % 13; }

static void SyntheticTiles(uint8_t *out, int count)
{
    for (int t = 0; t < count; ++t)
        for (int row = 0; row < 8; ++row) {
            int v = row < 4 ? TopIndex(t) : BottomIndex(t);
            memset(out + t * 32 + row * 4, v | v << 4, 4);
        }
}

/* Palette "set": 1..10 for type1..10, 0 for stdpal_0. Distinct for every index and set. */
static uint16_t SyntheticColor(int set, int index) { return (uint16_t)(index | set << 5 | (index + set) % 32 << 10); }

/* 1..10 when path is exactly graphics/text_window/typeN.<ext>, else 0. */
static int TypeOf(const char *path, const char *ext)
{
    char want[64];

    for (int type = 1; type <= 10; ++type) {
        snprintf(want, sizeof(want), "graphics/text_window/type%d.%s", type, ext);
        if (!strcmp(path, want))
            return type;
    }
    return 0;
}

void *CtrData_Load(const char *path, uint32_t *len)
{
    uint8_t *out;

    if (fixtureFail)
        return NULL;
    if (!strcmp(path, "graphics/text_window/menu_message.4bpp")) {
        out = malloc(576); assert(out);
        SyntheticTiles(out, 18);
        *len = 576;
        return out;
    }
    if (TypeOf(path, "4bpp")) {
        out = malloc(288); assert(out);
        SyntheticTiles(out, 9);
        *len = 288;
        return out;
    }
    int set = !strcmp(path, "graphics/text_window/stdpal_0.gbapal") ? 0 : TypeOf(path, "gbapal") ? TypeOf(path, "gbapal") : -1;
    if (set < 0)
        return NULL;
    out = malloc(32); assert(out);
    for (int i = 0; i < 16; ++i) {
        uint16_t c = SyntheticColor(set, i);
        out[i * 2] = (uint8_t)c;
        out[i * 2 + 1] = (uint8_t)(c >> 8);
    }
    *len = 32;
    return out;
}
int CtrPokeTouch_DexMonPic(unsigned species, unsigned char *tiles, unsigned short *palette)
{ (void)species; (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_DexMonPic80(unsigned species, unsigned char *pixels, unsigned short *palette)
{ (void)species; (void)pixels; (void)palette; return 0; }
int CtrPokeTouch_DexMenuIcon(int id, unsigned char *tiles, unsigned short *palette)
{ (void)id; (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_DexFootprint(unsigned species, unsigned char *bits)
{ (void)species; (void)bits; return 0; }
int CtrPokeTouch_DexCaughtMarker(unsigned char *tiles, unsigned short *palette)
{ (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_CardTrainerPic(unsigned char *tiles, unsigned short *palette)
{ (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_CardBadges(unsigned char *tiles, unsigned short *palette)
{ (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_MonIcon(unsigned species, unsigned personality, int egg, unsigned char *tiles, unsigned short *palette)
{ (void)species; (void)personality; (void)egg; (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_ItemIcon(unsigned item, unsigned char *tiles, unsigned short *palette)
{ (void)item; (void)tiles; (void)palette; return 0; }
int CtrPokeTouch_MapArt(int group, uint16_t *map, uint8_t *tiles, uint16_t *pal, uint16_t *bgMap, uint8_t *bgTiles)
{ (void)group; (void)map; (void)tiles; (void)pal; (void)bgMap; (void)bgTiles; return 0; }
int CtrPokeTouch_MapPlayerIcon(uint8_t *tiles, uint16_t *pal) { (void)tiles; (void)pal; return 0; }
int CtrPokeTouch_MapCursorIcon(uint8_t *tiles, uint16_t *pal) { (void)tiles; (void)pal; return 0; }
int CtrBottomParty_Icon(int slot, unsigned char *tiles, unsigned short *pal) { (void)slot; (void)tiles; (void)pal; return 0; }
unsigned CtrBottomSummary_IconSpecies(void) { return 0; }
int CtrBottomSummary_Icon(unsigned char *tiles, unsigned short *pal) { (void)tiles; (void)pal; return 0; }
int CtrBottomSummary_Pic(unsigned char *tiles, unsigned short *pal, int *flip) { (void)tiles; (void)pal; (void)flip; return 0; }
int CtrBottomSummary_Pic80(unsigned char *pixels, unsigned short *pal, int *flip) { (void)pixels; (void)pal; (void)flip; return 0; }
int CtrPokeTouch_Wallpaper(unsigned id, unsigned short *rgb555) { (void)id; (void)rgb555; return 0; }
int CtrPokeTouch_MarkShape(int mark, unsigned char *indexes) { (void)mark; (void)indexes; return 0; }

#define SENTINEL 0x1234
static uint16_t canvas[W * H];
static uint16_t at(int x, int y) { return canvas[x * H + H - 1 - y]; }
static void Clear(void)
{
    sCanvas = canvas;
    for (int i = 0; i < W * H; ++i)
        canvas[i] = SENTINEL;
}
static uint16_t Color(int set, int index) { return GbaColor(SyntheticColor(set, index)); }
/* Every pixel outside r still the sentinel. */
static void Untouched(int x, int y, int w, int h)
{
    for (int px = 0; px < W; ++px)
        for (int py = 0; py < H; ++py)
            if (px < x || py < y || px >= x + w || py >= y + h)
                assert(at(px, py) == SENTINEL);
}

static void frame_std_layout(void)
{
    Clear();
    KitPanel(&(CtrPokeTouchRect){16, 16, 64, 40}, KIT_FRAME_STD, 0);
    assert(at(16, 16) == Color(1, TopIndex(0)));
    assert(at(24, 16) == Color(1, TopIndex(1)) && at(64, 16) == Color(1, TopIndex(1)));
    assert(at(72, 16) == Color(1, TopIndex(2)));
    assert(at(16, 24) == Color(1, TopIndex(3)) && at(72, 24) == Color(1, TopIndex(5)));
    assert(at(40, 32) == Color(1, 1));
    assert(at(16, 48) == Color(1, TopIndex(6)) && at(16, 55) == Color(1, BottomIndex(6)));
    assert(at(40, 48) == Color(1, TopIndex(7)) && at(72, 55) == Color(1, BottomIndex(8)));
    assert(at(15, 15) == SENTINEL && at(80, 56) == SENTINEL);
    Untouched(16, 16, 64, 40);
}

static void frame_dialogue_flip(void)
{
    Clear();
    KitPanel(&(CtrPokeTouchRect){0, 0, 64, 48}, KIT_FRAME_DIALOGUE, 0);
    assert(at(0, 2) == Color(0, TopIndex(0)) && at(8, 2) == Color(0, TopIndex(1)));
    assert(at(16, 2) == Color(0, TopIndex(2)) && at(48, 2) == Color(0, TopIndex(3)) && at(56, 2) == Color(0, TopIndex(4)));
    assert(at(0, 10) == Color(0, TopIndex(5)) && at(56, 10) == Color(0, TopIndex(9)));
    assert(at(0, 18) == Color(0, TopIndex(10)));         /* tile row 2: tile 10 as it is */
    assert(at(0, 26) == Color(0, BottomIndex(10)));      /* tile row 3: tile 10 flipped vertically */
    assert(at(0, 34) == Color(0, BottomIndex(5)));       /* V5 */
    assert(at(16, 47) == Color(0, TopIndex(2)));         /* V2: its last row is tile 2's first */
    assert(at(24, 24) == Color(0, 1));                   /* the interior */
    assert(at(0, 48) == SENTINEL);
    Untouched(0, 0, 64, 48);
}

static void frame_dialogue_tall(void)
{
    Clear();
    KitPanel(&(CtrPokeTouchRect){0, 0, 64, 80}, KIT_FRAME_DIALOGUE, 0);
    for (int row = 2; row <= 4; ++row)
        assert(at(0, row * 8 + 2) == Color(0, TopIndex(10)));
    for (int row = 5; row <= 7; ++row)
        assert(at(0, row * 8 + 2) == Color(0, BottomIndex(10)));
    assert(at(0, 8 * 8 + 2) == Color(0, BottomIndex(5)) && at(0, 9 * 8 + 2) == Color(0, BottomIndex(0)));
    Untouched(0, 0, 64, 80);
}

static void frame_type_change_redraws(void)
{
    Clear();
    KitPanel(&(CtrPokeTouchRect){16, 16, 64, 40}, KIT_FRAME_STD, 0);
    assert(at(16, 16) == Color(1, TopIndex(0)));
    KitPanel(&(CtrPokeTouchRect){16, 16, 64, 40}, KIT_FRAME_STD, 3);
    assert(at(16, 16) == Color(4, TopIndex(0)));
    KitPanel(&(CtrPokeTouchRect){16, 16, 64, 40}, KIT_FRAME_STD, 12);  /* out of range: type1, as FireRed */
    assert(at(16, 16) == Color(1, TopIndex(0)));
}

static void frame_fallback_without_pack(void)
{
    fixtureFail = 1;
    Clear();
    assert(!KitFrameReady(KIT_FRAME_STD, 0) && !KitFrameReady(KIT_FRAME_DIALOGUE, 0));
    assert(KitInk(2) == EM_DARK && KitInk(3) == EM_DARK_SH && KitInk(1) == RGB(255, 255, 255));
    for (int kind = KIT_FRAME_DIALOGUE; kind <= KIT_FRAME_STD; ++kind) {
        Clear();
        KitPanel(&(CtrPokeTouchRect){16, 16, 64, 40}, kind, 0);
        assert(at(16, 16) == EM_DARK && at(79, 55) == EM_DARK && at(40, 16) == EM_DARK && at(16, 40) == EM_DARK);
        assert(at(17, 17) == RGB(255, 255, 255) && at(40, 32) == RGB(255, 255, 255));
        Untouched(16, 16, 64, 40);
    }
}

/* Pixels of colour c inside the rectangle. */
static int Count(int x, int y, int w, int h, uint16_t c)
{
    int n = 0;

    for (int px = x; px < x + w; ++px)
        for (int py = y; py < y + h; ++py)
            n += at(px, py) == c;
    return n;
}

static void row_arrow_when_selected(void)
{
    Clear();
    KitRow(&(CtrPokeTouchRect){8, 8, 96, 16}, "BAG", 1, 1);
    assert(Count(8, 8, 8, 16, KitInk(2)) == 8 * 14);    /* the arrow, whole, left of the label */
    assert(Count(16, 8, 18, 16, KitInk(2)) > 0);        /* the label 8 px to its right */
    Untouched(8, 8, 96, 16);
    Clear();
    KitRow(&(CtrPokeTouchRect){8, 8, 96, 16}, "BAG", 0, 1);
    assert(Count(8, 8, 8, 16, KitInk(2)) == 0 && Count(8, 8, 8, 16, KitInk(1)) == 8 * 16);
    assert(Count(16, 8, 18, 16, KitInk(2)) > 0);
}

static void text_clipped_to_rect(void)
{
    Clear();
    KitRow(&(CtrPokeTouchRect){8, 8, 64, 16}, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCD", 1, 1);
    Untouched(8, 8, 64, 16);
    Clear();
    KitCaption(&(CtrPokeTouchRect){8, 8, 120, 16}, "SEAFOAM ISLANDS AND MORE", "SEVII ISLANDS");
    Untouched(8, 8, 120, 16);
}

static void caption_no_overlap(void)
{
    Clear();
    KitCaption(&(CtrPokeTouchRect){8, 8, 120, 16}, "SEAFOAM ISLANDS AND MORE", "SEVII");
    /* SEVII is 30 px wide and ends 4 px inside the strip: x 94..123. */
    assert(Count(94, 8, 5, 16, KitInk(2)) > 0 && Count(118, 8, 5, 16, KitInk(2)) > 0);
    assert(Count(90, 8, 4, 16, KitInk(2)) == 0);        /* the left text stops at least 4 px before it */
    assert(Count(12, 8, 6, 16, KitInk(2)) > 0);         /* and starts 4 px inside */
}

static void icon_cell_1to1(void)
{
    static uint8_t pixels[24 * 24];
    uint16_t pal[16] = {0};

    memset(pixels, 1, sizeof(pixels));
    pal[1] = RGB(16, 200, 32);
    Clear();
    KitIconCell(&(CtrPokeTouchRect){100, 100, 48, 56}, pixels, pal, 24, 24, "BAG", 0, 0, 1);
    /* The label's line starts at 100 + 56 - 2 - 14 = 140; the icon is centred above it: (112, 108). */
    assert(Count(112, 108, 24, 24, pal[1]) == 24 * 24);
    assert(Count(100, 100, 48, 56, pal[1]) == 24 * 24);
    assert(Count(100, 140, 48, 16, KitInk(2)) > 0);
    Untouched(100, 100, 48, 56);
    Clear();
    KitIconCell(&(CtrPokeTouchRect){100, 100, 48, 56}, pixels, pal, 24, 24, "BAG", 1, 1, 1);
    assert(Count(100, 100, 48, 56, KitInk(3)) > 0);     /* pressed: the shadow grey */
    assert(Count(100, 140, 48, 16, KitInk(2)) >= 8 * 14 / 2);
}

static void hp_thresholds(void)
{
    static const struct { unsigned hp; uint16_t color; } cases[] = {
        {25, UI_HP_HIGH}, {10, UI_HP_MID}, {9, UI_HP_LOW},
    };

    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Clear();
        KitHpBar(8, 8, 50, (int)cases[i].hp, 48);
        assert(at(9, 9) == cases[i].color && at(9, 10) == cases[i].color);
        Untouched(8, 8, 50, 4);
    }
    Clear();
    KitHpBar(8, 8, 50, 0, 48);
    assert(Count(8, 8, 50, 4, UI_HP_LOW) == 0);
}

static void disabled_row_toned(void)
{
    Clear();
    KitRow(&(CtrPokeTouchRect){8, 8, 96, 16}, "BAG", 0, 0);
    assert(Count(16, 8, 18, 16, Tone(KitInk(2), 0)) > 0 && Count(8, 8, 96, 16, KitInk(2)) == 0);
}

/* Words that do not fit go to the next 14 px line; no more than maxLines; nothing outside x..x+width. */
static void wrap_lines(void)
{
    Clear();
    /* "AAA" is 18 px, "AAA BBB" 41: in 40 px each word takes its own line. */
    assert(KitWrap("AAA BBB CCC", 8, 8, 40, 3) == 3);
    assert(Count(8, 8, 40, 14, KitInk(2)) > 0 && Count(8, 22, 40, 14, KitInk(2)) > 0 && Count(8, 36, 40, 14, KitInk(2)) > 0);
    Untouched(8, 8, 40, 3 * 14);
    Clear();
    assert(KitWrap("AAA BBB CCC", 8, 8, 40, 2) == 2);
    assert(Count(8, 36, 40, 14, KitInk(2)) == 0);
    Clear();
    assert(KitWrap("AA BB", 8, 8, 80, 3) == 1);              /* fits on one line */
    Clear();
    assert(KitWrap("", 8, 8, 80, 3) == 0);
    Untouched(0, 0, 0, 0);
}

static void type_label_flat(void)
{
    Clear();
    KitTypeLabel(&(CtrPokeTouchRect){8, 8, 40, 14}, 3, "FIRE");
    assert(at(8, 8) == UiTypeColor(3) && at(47, 21) == UiTypeColor(3));
    assert(Count(8, 8, 40, 14, RGB(255, 255, 255)) > 0);
    Untouched(8, 8, 40, 14);
}

int main(int argc, char **argv)
{
    SyntheticFont();
    if (argc > 1 && !strcmp(argv[1], "fallback")) {
        frame_fallback_without_pack();
        printf("PASS lower kit fallback\n");
        return 0;
    }
    assert(KitFrameReady(KIT_FRAME_STD, 0) && KitFrameReady(KIT_FRAME_DIALOGUE, 0));
    assert(KitInk(2) == Color(0, 2) && KitInk(1) == Color(0, 1));
    frame_std_layout();
    frame_dialogue_flip();
    frame_dialogue_tall();
    frame_type_change_redraws();
    row_arrow_when_selected();
    text_clipped_to_rect();
    caption_no_overlap();
    icon_cell_1to1();
    hp_thresholds();
    disabled_row_toned();
    wrap_lines();
    type_label_flat();
    printf("PASS lower kit\n");
    return 0;
}
