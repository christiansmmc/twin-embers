/* The components the red device's views keep of the lower screen's first theme (3ds_ui_theme.h), included by
 * 3ds_poketouch.c after its pixel, fill and font helpers: a point in a rectangle, a rectangle inset, FireRed's text,
 * FireRed's type colours and an indexed image. Each draws only inside what it is given (the dirty-rectangle updates
 * rely on it). */

/* A helper of the views: not every build uses every one. */
#define UI_FN static __attribute__((unused))

UI_FN int UiIn(const CtrPokeTouchRect *r, int x, int y)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

UI_FN CtrPokeTouchRect UiInset(const CtrPokeTouchRect *r, int d)
{
    CtrPokeTouchRect i = {(int16_t)(r->x + d), (int16_t)(r->y + d), (int16_t)(r->w - 2 * d), (int16_t)(r->h - 2 * d)};
    return i;
}

/* Text: FireRed's own glyphs (latin_normal), `fg` with the `shadow` FireRed draws under it. `top` is where the capitals
 * start; nothing is drawn from the first glyph that would pass `limit`. */
UI_FN int UiTextWidth(const char *s, int scale)
{
    int w = 0;

    while (*s)
        w += Glyph(&s)->width;
    return w * scale;
}

UI_FN void UiText(const char *s, int x, int top, uint16_t fg, uint16_t shadow, int scale, int limit)
{
    int y = top - 3 * scale;

    while (*s) {
        const PtGlyph *g = Glyph(&s);

        if (x + g->width * scale > limit)
            break;
        for (int row = 0; row < 16; ++row)
            for (int col = 0; col < g->width; ++col) {
                unsigned v = (g->rows[row] >> (2 * col)) & 3;
                if (!v)
                    continue;
                if (scale == 1)
                    Px(x + col, y + row, v == 1 ? fg : shadow);
                else
                    Fill(x + col * scale, y + row * scale, scale, scale, v == 1 ? fg : shadow);
            }
        x += g->width * scale;
    }
}

/* FireRed's type colours (its type numbers 0-17), for the type chips. */
static const uint16_t sUiType[18] = {
    UI_RGB(150, 150, 112), UI_RGB(196, 48, 40), UI_RGB(146, 124, 226), UI_RGB(160, 64, 160), UI_RGB(216, 180, 96),
    UI_RGB(178, 152, 56), UI_RGB(160, 180, 32), UI_RGB(110, 88, 150), UI_RGB(170, 170, 196), UI_RGB(104, 104, 104),
    UI_RGB(232, 110, 40), UI_RGB(86, 136, 236), UI_RGB(110, 192, 72), UI_RGB(240, 200, 40), UI_RGB(240, 84, 130),
    UI_RGB(120, 206, 206), UI_RGB(104, 60, 236), UI_RGB(104, 80, 66),
};

UI_FN uint16_t UiTypeColor(int type)
{
    return sUiType[type >= 0 && type < 18 ? type : 0];
}

/* An indexed image (palette entries, 0 clear) at `scale`, toned grey when off. */
UI_FN void UiImage(const uint8_t *pixels, const uint16_t *pal, int stride, int w, int h, int x, int y, int scale, int on)
{
    for (int py = 0; py < h; ++py)
        for (int px = 0; px < w; ++px) {
            uint8_t v = pixels[py * stride + px];

            if (v)
                Fill(x + px * scale, y + py * scale, scale, scale, Tone(pal[v], on));
        }
}
