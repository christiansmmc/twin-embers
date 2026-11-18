/* Original neutral geometric icons and pack-backed glyph storage.
 * No ROM-derived pixels are initialized in this header. */
typedef struct { uint8_t width; uint32_t rows[16]; } PtGlyph;
typedef struct { uint8_t w, h; const uint8_t *pixels; uint16_t pal[16]; } PtImage;
/* FireRed's latin_normal glyphs, then its japanese_normal ones (a Japanese Pokemon's name): 256 each. */
static PtGlyph sPtFont[512];
static bool sPtFontReady[512];
/* Port-authored 12x12 silhouettes (not restored game assets): HOME's six START entries without the pack
 * (3ds_device_home_impl.h). One bit per pixel, left-to-right, from high to low in each 12-bit row. */
static const uint16_t sTwinIconRows[6][12] = {
    /* Pokedex */
    {0x000, 0x3fc, 0x284, 0x2b4, 0x2b4, 0x284, 0x2b4, 0x284, 0x284, 0x3fc, 0x000, 0x000},
    /* Pokemon */
    {0x060, 0x1f8, 0x30c, 0x606, 0x462, 0xf0f, 0xf0f, 0x462, 0x606, 0x30c, 0x1f8, 0x060},
    /* Bag */
    {0x0f0, 0x108, 0x108, 0x3fc, 0x204, 0x264, 0x264, 0x204, 0x204, 0x204, 0x3fc, 0x000},
    /* Trainer Card */
    {0x000, 0x7fe, 0x402, 0x5de, 0x552, 0x5de, 0x402, 0x5fe, 0x402, 0x7fe, 0x000, 0x000},
    /* Save */
    {0x000, 0x3fc, 0x204, 0x2f4, 0x2f4, 0x204, 0x2f4, 0x294, 0x294, 0x2f4, 0x3fc, 0x000},
    /* Options */
    {0x060, 0x1f8, 0x36c, 0x606, 0x666, 0xe97, 0xe97, 0x666, 0x606, 0x36c, 0x1f8, 0x060},
};
extern const uint8_t *CtrTwinFontData(void);
extern unsigned CtrTwinFontWidth(unsigned code);
/* FireRed's japanese_normal glyph for a code, laid out as a latin_normal one (64 bytes), and its width. */
extern const uint8_t *CtrTwinJapaneseGlyph(unsigned code, uint8_t out[64]);
extern unsigned CtrTwinJapaneseFontWidth(unsigned code);
extern const uint8_t *CtrTwinSmallFontData(void);
extern unsigned CtrTwinSmallFontWidth(unsigned code);
