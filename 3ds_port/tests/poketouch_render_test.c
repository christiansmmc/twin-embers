/* Real renderer with synthetic pack/game bridges; no ROM-derived fixtures. */
#include <assert.h>
#ifndef PT_SOURCE
#define PT_SOURCE "3ds_poketouch.c"
#endif
#include PT_SOURCE
#include "3ds_pak.h"

static uint8_t fixtureTiles[512], fixtureColors[32];
static int fixtureSize = 288, fixtureFail;
static uint8_t font[256 * 64];
static FILE *realPack;
static CtrPakHeader realHeader;
static CtrPakEntry *realIndex;
const uint8_t *CtrTwinFontData(void) { return font; }
unsigned CtrTwinFontWidth(unsigned code) { (void)code; return 5; }
const uint8_t *CtrTwinJapaneseGlyph(unsigned code, uint8_t out[64]) { (void)code; memset(out, 0, 64); return out; }
unsigned CtrTwinJapaneseFontWidth(unsigned code) { (void)code; return 8; }
static uint8_t smallFont[256 * 32];
const uint8_t *CtrTwinSmallFontData(void) { return smallFont; }
unsigned CtrTwinSmallFontWidth(unsigned code) { (void)code; return 5; }

/* graphics/text_window/ for home_frame_change, raw: typeN.4bpp's pixels cycle through indexes 0..2 shifted by N, so
 * each type has clear (0) pixels where the type before it drew; the palettes' index 2 differs by type. */
static int kitFixture;
static void *KitFixture(const char *path, uint32_t *len)
{
    const char *name = path + strlen("graphics/text_window/");
    int type = 0, n = strstr(name, "menu_message") ? 576 : strstr(name, ".4bpp") ? 288 : 32;
    uint8_t *out;

    if (strstr(name, ".lz"))
        return NULL;
    if (sscanf(name, "type%d.", &type) != 1)
        type = 0;
    out = calloc((size_t)n, 1); assert(out);
    if (n != 32)
        for (int i = 0; i < n * 2; ++i)
            out[i / 2] |= (uint8_t)(((i + type) % 3) << (i & 1 ? 4 : 0));
    else
        for (int i = 0; i < 16; ++i) {
            uint16_t c = (uint16_t)(i == 1 ? 0x7fff : (i * 7 + type * 3) | type << 10);

            out[i * 2] = (uint8_t)c;
            out[i * 2 + 1] = (uint8_t)(c >> 8);
        }
    *len = (uint32_t)n;
    return out;
}

/* Literal-only GBA LZ stream, freshly synthesized from the fixture bytes. */
void *CtrData_Load(const char *path, uint32_t *len)
{
    if (kitFixture && !strncmp(path, "graphics/text_window/", strlen("graphics/text_window/")))
        return KitFixture(path, len);
    if (realPack) {
        const CtrPakEntry *entry=CtrPak_Find(realIndex,realHeader.entryCount,path);
        assert(entry && !entry->flags && entry->storedSize==entry->rawSize);
        uint8_t *out=malloc(entry->rawSize); assert(out);
        assert(!fseek(realPack,(long)entry->offset,SEEK_SET));
        assert(fread(out,1,entry->rawSize,realPack)==entry->rawSize);
        assert(CtrPak_Crc32(0,out,entry->rawSize)==entry->crc32);
        *len=entry->rawSize; return out;
    }
    if (fixtureFail) return NULL;
    int palette = strstr(path, "icon_palettes/") != NULL;
    const uint8_t *src = palette ? fixtureColors : fixtureTiles;
    unsigned size = palette ? 32u : (unsigned)fixtureSize;
    *len = 4 + size + (size + 7) / 8;
    uint8_t *out = malloc(*len); assert(out);
    out[0] = 0x10; out[1] = size; out[2] = size >> 8; out[3] = 0;
    unsigned pos = 4;
    for (unsigned i = 0; i < size; ++i) {
        if (!(i % 8)) out[pos++] = 0;
        out[pos++] = src[i];
    }
    assert(pos == *len);
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
/* The card's badges: none, or (cardArtOk) a sheet of solid badges in palette colour 1, pure green. */
static int cardArtOk;
int CtrPokeTouch_CardBadges(unsigned char *tiles, unsigned short *palette)
{
    if (!cardArtOk)
        return 0;
    memset(tiles, 0x11, 0x400); memset(palette, 0, 32); palette[1] = 0x03E0;
    return 1;
}
/* A Pokemon's box icon: none, or (monIconOk) a solid 32x32 one in palette colour 1, yellow. */
static int monIconOk;   /* yellow: apart from the item icons' red and blue */
int CtrPokeTouch_MonIcon(unsigned species, unsigned personality, int egg, unsigned char *tiles, unsigned short *palette)
{
    (void)personality; (void)egg;
    if (!monIconOk || !species)
        return 0;
    memset(tiles, 0x11, 512); memset(palette, 0, 32); palette[1] = 0x03FF;
    return 1;
}
int CtrPokeTouch_ItemIcon(unsigned item, unsigned char *tiles, unsigned short *palette)
{
    memset(tiles, 0x11, 288); memset(palette, 0, 32);
    palette[1] = item == 1 ? 31 : 31 << 10;
    return item != 0;
}
int CtrPokeTouch_MapArt(int group, uint16_t *map, uint8_t *tiles, uint16_t *pal,
                      uint16_t *bgMap, uint8_t *bgTiles)
{
    (void)group;
    memset(map, 0, 30 * 20 * 2); memset(bgMap, 0, 30 * 20 * 2);
    memset(tiles, 0x11, MAP_TILES * 32); memset(bgTiles, 0, MAP_TILES * 32);
    memset(pal, 0, MAP_COLORS * 2); pal[1] = 0x5ab3;
    return 1;
}
int CtrPokeTouch_MapPlayerIcon(uint8_t *tiles, uint16_t *pal)
{ (void)tiles; (void)pal; return 0; }
int CtrPokeTouch_MapCursorIcon(uint8_t *tiles, uint16_t *pal)
{ (void)tiles; (void)pal; return 0; }
/* A party icon: none, or (partyIconOk) a solid one in palette colour 1, pure red. */
static int partyIconOk;
int CtrBottomParty_Icon(int slot, unsigned char *tiles, unsigned short *pal)
{
    (void)slot;
    if (!partyIconOk)
        return 0;
    memset(tiles, 0x11, 1024); memset(pal, 0, 32); pal[1] = 0x001F;
    return 1;
}

unsigned CtrBottomSummary_IconSpecies(void) { return 0; }
int CtrBottomSummary_Icon(unsigned char *tiles, unsigned short *pal)
{ (void)tiles; (void)pal; return 0; }
int CtrBottomSummary_Pic(unsigned char *tiles, unsigned short *pal, int *flip)
{ (void)tiles; (void)pal; (void)flip; return 0; }
int CtrBottomSummary_Pic80(unsigned char *pixels, unsigned short *pal, int *flip)
{ (void)pixels; (void)pal; (void)flip; return 0; }
/* A box wallpaper: none, or (wallOk) one flat colour. */
static int wallOk;
int CtrPokeTouch_Wallpaper(unsigned id, unsigned short *rgb555)
{
    (void)id;
    if (!wallOk)
        return 0;
    for (int i = 0; i < 160 * 144; ++i) rgb555[i] = 0x5294;
    return 1;
}
int CtrPokeTouch_MarkShape(int mark, unsigned char *indexes)
{ (void)mark; (void)indexes; return 0; }

static uint16_t canvas[W * H], reference[W * H];
static uint16_t at(int x, int y) { return canvas[x * H + H - 1 - y]; }
static void update(CtrPokeTouchView *old, const CtrPokeTouchView *v)
{
    CtrDirtyList dirty; CtrDirty_Clear(&dirty);
    CtrPokeTouch_Update(canvas, old, v, &dirty);
    memcpy(reference, canvas, sizeof(canvas));
    CtrPokeTouch_Draw(canvas, v);
#ifndef PT_BASELINE
    if (memcmp(reference, canvas, sizeof(canvas))) {
        for(int x=0;x<W;++x) for(int y=0;y<H;++y) {
            unsigned n=x*H+H-1-y;
            if(reference[n]!=canvas[n]) {
                fprintf(stderr,"redraw mismatch at %d,%d selected=%u pressed=%u save=%u/%u back=%u\n",
                        x,y,v->selected,v->pressed,v->save,v->saveState,v->backVisible);
                assert(reference[n]==canvas[n]);
            }
        }
    }
#endif
    *old = *v;
}
static void decoder(void)
{
#ifndef PT_BASELINE
    uint8_t out[512], pixels[144], saved[144]; uint16_t pal[16];
    const uint8_t literals[] = {0x10,3,0,0,0,1,2,3};
    assert(PokeTouch_Lz77(literals,sizeof(literals),out,3)==3);
    for (unsigned n=0;n<sizeof(literals);++n) assert(!PokeTouch_Lz77(literals,n,out,3));
    assert(!PokeTouch_Lz77(literals,sizeof(literals),out,2));
    const uint8_t repeated[] = {0x10,6,0,0,0x10,1,2,3,0,2};
    assert(PokeTouch_Lz77(repeated,sizeof(repeated),out,6)==6);
    assert(!memcmp(out,"\1\2\3\1\2\3",6));
    const uint8_t invalid[] = {0x10,3,0,0,0x80,0,0};
    assert(!PokeTouch_Lz77(invalid,sizeof(invalid),out,512));
    const uint8_t overrun[] = {0x10,2,0,0,0x40,1,0,0};
    assert(!PokeTouch_Lz77(overrun,sizeof(overrun),out,512));
    assert(!strcmp(sMenuItemArt[3],"card_key"));
    assert(!strcmp(sMenuItemArt[5],"teachy_tv"));
    for (int pitch=3;pitch<=4;++pitch) {
        fixtureSize=pitch==3?288:512;
        memset(fixtureTiles,0,sizeof(fixtureTiles));
        memset(fixtureColors,0,sizeof(fixtureColors));
        fixtureColors[2]=0xff; fixtureColors[3]=0x7f; /* index 1 white */
        /* Odd x/y, across both tile boundaries: upper-left sampling loses it. */
        fixtureTiles[(9/8*pitch+9/8)*32+9%8*4+9%8/2]=0x10;
        assert(CtrPokeTouch_PackMenuIcon(0,pixels,pal));
        assert(pixels[4*12+4]==1 && pixels[0]==0 && pal[0]==0 && pal[1]==0xffff);
        /* Opaque black must remain an opaque index. */
        fixtureColors[2]=fixtureColors[3]=0;
        assert(CtrPokeTouch_PackMenuIcon(0,pixels,pal) && pal[1]==0);
        memcpy(saved,pixels,sizeof(saved)); fixtureFail=1;
        assert(!CtrPokeTouch_PackMenuIcon(0,pixels,pal));
        assert(!memcmp(saved,pixels,sizeof(saved))); fixtureFail=0;
    }
    {
        /* The side menu's soft icons: the pack's 24x24 reduced by area to 16x16, colour over the drawn pixels only. */
        static PtSoft soft;

        fixtureSize=288; memset(fixtureColors,0,sizeof(fixtureColors)); fixtureColors[2]=0xff; fixtureColors[3]=0x7f;
        memset(fixtureTiles,0x11,288);
        assert(PokeTouch_SoftIcon("fame_checker",&soft) && soft.ok);
        for (int i=0;i<PT_SOFT*PT_SOFT;++i) assert(soft.alpha[i]==255 && soft.rgb[i]==0xffff);
        /* Opaque only left of x=11 in every row: a whole output column (x 0-10.5), then a third of one, then none. */
        memset(fixtureTiles,0,288);
        for (int tr=0;tr<3;++tr) for (int row=0;row<8;++row) {
            memset(&fixtureTiles[(tr*3+0)*32+row*4],0x11,4);
            fixtureTiles[(tr*3+1)*32+row*4+0]=0x11; fixtureTiles[(tr*3+1)*32+row*4+1]=0x01;
        }
        assert(PokeTouch_SoftIcon("fame_checker",&soft));
        assert(soft.alpha[5]==255 && soft.alpha[7]>70 && soft.alpha[7]<100 && soft.alpha[8]==0 && soft.alpha[15]==0);
        assert(soft.rgb[7]==0xffff);                              /* a half-covered pixel keeps the colour, not a darker mix */
        memset(fixtureTiles,0,sizeof(fixtureTiles)); assert(!PokeTouch_SoftIcon("fame_checker",&soft));   /* nothing drawn */
        fixtureFail=1; assert(!PokeTouch_SoftIcon("fame_checker",&soft)); fixtureFail=0;
    }
    fixtureSize=287; assert(!CtrPokeTouch_PackMenuIcon(0,pixels,pal));
    fixtureSize=288; memset(fixtureTiles,0,sizeof(fixtureTiles));
    assert(!CtrPokeTouch_PackMenuIcon(0,pixels,pal));
    assert(!CtrPokeTouch_PackMenuIcon(6,pixels,pal));
#endif
}
#ifndef PT_BASELINE
/* FireRed's menu arrow (raw code 0xEF) as a solid glyph, so where it is drawn shows in the text colour. */
static void ArrowGlyph(void)
{
    for (int y = 0; y < 14; ++y)
        for (int x = 0; x < 8; ++x) {
            uint8_t *src = font + 0xEF * 64 + ((y / 8) * 2 + x / 8) * 16 + (y % 8) * 2;
            unsigned bits = ((unsigned)src[1] << 8 | src[0]) | 1u << ((7 - x % 8) * 2);

            src[0] = (uint8_t)bits;
            src[1] = (uint8_t)(bits >> 8);
        }
}

/* A glyph inked in the text colour in columns 0-4 over the capitals' rows 3-11, so where it is drawn shows. */
static void SolidGlyph(unsigned code)
{
    for (int y = 3; y <= 11; ++y)
        for (int x = 0; x < 5; ++x) {
            uint8_t *src = font + code * 64 + ((y / 8) * 2 + x / 8) * 16 + (y % 8) * 2;
            unsigned bits = ((unsigned)src[1] << 8 | src[0]) | 1u << ((7 - x % 8) * 2);

            src[0] = (uint8_t)bits;
            src[1] = (uint8_t)(bits >> 8);
        }
}

/* Pixels of colour c in the w x h at (x, y). */
static int ColorIn(int x, int y, int w, int h, uint16_t c)
{
    int n = 0;

    for (int i = x; i < x + w; ++i)
        for (int j = y; j < y + h; ++j)
            n += at(i, j) == c;
    return n;
}

/* HOME as a full-screen page (3ds_home_page_impl.h): its hits, the map at 1:1 and its cells, the page only when
 * nothing else is open, and every partial redraw equal to a full one. `v` is the HOME view just drawn. */
static void home_page(CtrPokeTouchView *v)
{
    const int mx = kHomeMapPanel.x + 8, my = kHomeMapPanel.y + 8;
    const CtrPokeTouchRect *buttons[PT_COUNT] = {0};
    int cx, cy;

    /* home_map_1to1: the stub map is one colour; at 1:1 it covers exactly the 192x128 at (mx, my). */
    for (int x = mx; x < mx + 192; ++x) for (int y = my; y < my + 128; ++y) assert(at(x, y) == Gba(0x5ab3));
    assert(at(mx - 1, my + 10) != Gba(0x5ab3) && at(mx + 192, my + 10) != Gba(0x5ab3));
    /* home_hit_centres: the entries; RUN and SPEED, each the half of their window it is in */
    for (int i = PT_POKEDEX; i <= PT_OPTIONS; ++i) buttons[i] = &kHomeEntry[i - PT_POKEDEX];
    buttons[PT_RUN] = &kHomeRun;
    for (int i = 0; i < PT_COUNT; ++i) {
        const CtrPokeTouchRect *r = buttons[i];
        if (!r)
            continue;
        assert(CtrPokeTouch_HomeHit(v, r->x + r->w / 2, r->y + r->h / 2) == i);
        assert(CtrPokeTouch_HomeHit(v, r->x, r->y) == i && CtrPokeTouch_HomeHit(v, r->x + r->w - 1, r->y + r->h - 1) == i);
    }
    assert(CtrPokeTouch_HomeHit(v, kHomeSpeed.x, kHomeSpeed.y) == PT_HOME_SPEED);
    assert(CtrPokeTouch_HomeHit(v, kHomeSpeed.x + kHomeSpeed.w - 1, kHomeSpeed.y + kHomeSpeed.h - 1) == PT_HOME_SPEED);
    /* home_no_xy: X and Y have left the page (their buttons still work): nothing on it answers with them */
    for (int x = 0; x < W; x += 2)
        for (int y = 0; y < H; y += 2) {
            int hit = CtrPokeTouch_HomeHit(v, x, y);
            assert(hit != PT_REGISTERED && hit != PT_BIKE);
        }
    /* home_hit_gaps: the menu window's frame beside a row and above the first, the gap under it, on the map, off the
     * screen */
    assert(CtrPokeTouch_HomeHit(v, kHomeEntry[0].x + kHomeEntry[0].w, kHomeEntry[0].y + 4) == -1);
    assert(CtrPokeTouch_HomeHit(v, kHomeMenu.x + 2, kHomeMenu.y + 2) == -1);
    assert(CtrPokeTouch_HomeHit(v, kHomeMenu.x + 20, kHomeMenu.y + kHomeMenu.h + 1) == -1);
    assert(CtrPokeTouch_HomeHit(v, mx + 50, my + 50) == -1 && CtrPokeTouch_HomeHit(v, -1, -1) == -1);
    /* home_menu_window: the entries are rows of one framed window, each row the window's white; RUN and SPEED's window
     * and the party's are framed too, the first ending on the map's last row. The outline checks hold without the pack
     * (the kit's outline); with FIRERED_TEST_PAK the frames are the pack's tiles and home_frame_change covers them. */
    if (!KitFrameReady(KIT_FRAME_STD, v->frameType)) {
        assert(at(kHomeMenu.x, kHomeMenu.y + 20) == KitInk(2) && at(kHomeMenu.x + kHomeMenu.w - 1, kHomeMenu.y + 60) == KitInk(2));
        assert(at(kHomeSettings.x, kHomeSettings.y + 20) == KitInk(2));
        assert(at(kHomeSettings.x + 20, kHomeSettings.y + kHomeSettings.h - 1) == KitInk(2));
        assert(at(kHomeParty.x, kHomeParty.y + 20) == KitInk(2) && at(kHomeParty.x + 20, kHomeParty.y + kHomeParty.h - 1) == KitInk(2));
    }
    for (int i = 0; i < 6; ++i)
        assert(at(kHomeEntry[i].x + 2, kHomeEntry[i].y + 1) == KitInk(1));
    assert(at(kHomeSettings.x + 20, kHomeSettings.y + kHomeSettings.h) != KitInk(2));
    assert(at(kHomeRunLine.x + 2, kHomeRunLine.y + 1) == KitInk(1) && at(kHomeSpeedLine.x + 2, kHomeSpeedLine.y + 1) == KitInk(1));
    /* home_speed_label: the multiplier the game runs the field at, "1x" with the turbo off */
    {
        char text[8];

        HomeSpeedLabel(0, text); assert(!strcmp(text, "1x"));
        HomeSpeedLabel(1, text); assert(!strcmp(text, "1x"));
        HomeSpeedLabel(2, text); assert(!strcmp(text, "2x"));
        HomeSpeedLabel(4, text); assert(!strcmp(text, "4x"));
    }
    /* home_setting_margins: RUN's and SPEED's values ("OFF", "1x" here) end the arrow's 8 px before their line's end,
     * as the labels start 8 px after its start, so the text sits in the middle of the window (the owner saw "ON" and
     * "2x" against the frame) */
    {
        static uint8_t saved[sizeof(font)];
        const CtrPokeTouchRect *lines[2] = {&kHomeRunLine, &kHomeSpeedLine};

        memcpy(saved, font, sizeof(font));
        for (const char *c = "OF1x"; *c; ++c) {
            const char *one = c;

            SolidGlyph(GlyphCode(&one));
        }
        memset(sPtFontReady, 0, sizeof(sPtFontReady));
        CtrPokeTouch_Draw(canvas, v);
        for (int i = 0; i < 2; ++i) {
            int end = lines[i]->x + lines[i]->w;

            assert(!ColorIn(end - KIT_ARROW_W, lines[i]->y, KIT_ARROW_W, lines[i]->h, KitInk(2)));
            assert(ColorIn(end - KIT_ARROW_W - 5, lines[i]->y, 5, lines[i]->h, KitInk(2)) > 0);
        }
        /* home_setting_label_room: a label too long for its line (SPEED is "VELOCIDAD" in Spanish) stops 4 px short
         * of the value, never runs under it */
        {
            const char *one = "W";
            const CtrPokeTouchRect *l = &kHomeSpeedLine;
            int valueX = l->x + l->w - KIT_ARROW_W - UiTextWidth("1x", 1);

            SolidGlyph(GlyphCode(&one));
            memset(sPtFontReady, 0, sizeof(sPtFontReady));
            HomeSettingLine(l, "WWWWWWWWWWWWWWWWWW", "1x", 0, 1);
            assert(ColorIn(l->x + KIT_ARROW_W, l->y, 8, l->h, KitInk(2)) > 0);
            assert(!ColorIn(valueX - 4, l->y, 4, l->h, KitInk(2)) && !ColorIn(valueX - 4, l->y, 4, l->h, KitInk(3)));
            assert(ColorIn(valueX, l->y, 5, l->h, KitInk(2)) > 0);
        }
        memcpy(font, saved, sizeof(font));
        memset(sPtFontReady, 0, sizeof(sPtFontReady));
        CtrPokeTouch_Draw(canvas, v);
    }
    /* home_arrow_on_held_row: no arrow while nothing is held; FireRed's arrow at the left of the row held, RUN's and
     * SPEED's on their line */
    for (int i = 0; i < 6; ++i)
        assert(!ColorIn(kHomeEntry[i].x, kHomeEntry[i].y, 8, 16, KitInk(2)));
    assert(!ColorIn(kHomeRunLine.x, kHomeRunLine.y, 8, kHomeRunLine.h, KitInk(2)));
    {
        CtrPokeTouchView old = *v;

        v->pressed = PT_BAG; update(&old, v);
        assert(ColorIn(kHomeEntry[2].x, kHomeEntry[2].y, 8, 16, KitInk(2)) > 20);
        assert(!ColorIn(kHomeEntry[1].x, kHomeEntry[1].y, 8, 16, KitInk(2)));
        v->pressed = PT_RUN; update(&old, v);
        assert(ColorIn(kHomeRunLine.x, kHomeRunLine.y, 8, kHomeRunLine.h, KitInk(2)) > 20);
        assert(!ColorIn(kHomeEntry[2].x, kHomeEntry[2].y, 8, 16, KitInk(2)));
        v->pressed = PT_HOME_SPEED; update(&old, v);
        assert(ColorIn(kHomeSpeedLine.x, kHomeSpeedLine.y, 8, kHomeSpeedLine.h, KitInk(2)) > 20);
        assert(!ColorIn(kHomeRunLine.x, kHomeRunLine.y, 8, kHomeRunLine.h, KitInk(2)));
        v->running = 1; update(&old, v);
        v->speed = 3; update(&old, v);
        v->speedEnabled = 0; update(&old, v);
        v->enabled[PT_SAVE] = 0; update(&old, v);
        v->pressed = PT_REGISTERED; update(&old, v);
        v->pressed = PT_NONE; v->running = 0; v->speed = 1; v->speedEnabled = 1; v->enabled[PT_SAVE] = 1; update(&old, v);
    }
    /* home_party: each Pokemon's party icon at (slot x + 8, slot y + 7) over its HP bar at (slot x + 10, slot y + 40),
     * as the battle's bar: toned when fainted, the empty ring and no bar for an empty slot. A touch on a Pokemon is
     * PT_HOME_PARTY + its slot; on an empty slot nothing. */
    {
        CtrPokeTouchView old = *v;
        const CtrPokeTouchRect *s0 = &kHomePartySlot[0], *s1 = &kHomePartySlot[1], *s2 = &kHomePartySlot[2];

        partyIconOk = 1;
        v->party[0] = 1; v->partyHp[0] = 100; v->partyIcon[0] = 25;
        v->party[1] = 2; v->partyHp[1] = 0; v->partyIcon[1] = 26;
        update(&old, v);
        assert(at(s0->x + 8 + 16, s0->y + 7 + 16) == GbaColor(0x001F));
        assert(at(s1->x + 8 + 16, s1->y + 7 + 16) == Tone(GbaColor(0x001F), 0));
        assert(at(s0->x + 11, s0->y + 41) == UI_HP_HIGH && at(s0->x + 10, s0->y + 40) == KitInk(2));
        assert(at(s1->x + 11, s1->y + 41) == KitInk(3));
        assert(at(s2->x + 11, s2->y + 41) == KitInk(1) && at(s2->x + 10, s2->y + 40) == KitInk(1));
        assert(CtrPokeTouch_HomeHit(v, s0->x + s0->w / 2, s0->y + s0->h / 2) == PT_HOME_PARTY + 0);
        assert(CtrPokeTouch_HomeHit(v, s1->x, s1->y) == PT_HOME_PARTY + 1);
        assert(CtrPokeTouch_HomeHit(v, s2->x + 10, s2->y + 10) == -1);
        /* home_hint_over_party: while the map has a place picked (the Fly's hints), the hint has the window and the party
         * waits: no icon, no bar, no touch on it; then the party comes back */
        v->homeHint = 1; strcpy(v->bar[2], "Tap again to Fly"); update(&old, v);
        assert(at(s0->x + 8 + 16, s0->y + 7 + 16) == KitInk(1) && at(s0->x + 11, s0->y + 41) == KitInk(1));
        /* home_hint_tap: a touch anywhere on the hint's window is PT_HOME_HINT (it puts the party back) */
        assert(CtrPokeTouch_HomeHit(v, s0->x + s0->w / 2, s0->y + s0->h / 2) == PT_HOME_HINT);
        assert(CtrPokeTouch_HomeHit(v, kHomeParty.x, kHomeParty.y) == PT_HOME_HINT);
        assert(CtrPokeTouch_HomeHit(v, kHomeParty.x + kHomeParty.w - 1, kHomeParty.y + kHomeParty.h - 1) == PT_HOME_HINT);
        assert(CtrPokeTouch_HomeHit(v, kHomeParty.x + 20, kHomeParty.y - 1) == -1);
        strcpy(v->bar[2], "Not visited yet"); update(&old, v);
        v->homeHint = 0; update(&old, v);
        assert(at(s0->x + 8 + 16, s0->y + 7 + 16) == GbaColor(0x001F));
        /* every change of the party redraws it as a full draw would */
        v->partyHp[0] = 30; update(&old, v);
        v->party[0] = 3; v->partyStatus[0] = 1; update(&old, v);
        v->partyIcon[0] = 27; update(&old, v);
        v->party[2] = 1; v->partyHp[2] = 5; v->partyIcon[2] = 28; update(&old, v);
        v->pressed = PT_HOME_PARTY + 2; update(&old, v);
        v->pressed = PT_NONE; memset(v->party, 0, sizeof(v->party)); update(&old, v);
        partyIconOk = 0;
    }
    /* home_map_cell_bounds: cell (cx, cy) is the 8x8 at (mx + 8 + 8 cx, my + 4 + 8 cy) */
    assert(CtrPokeTouch_HomeMapCell(mx + 8 + 3, my + 4 + 3, &cx, &cy) && cx == 0 && cy == 0);
    assert(CtrPokeTouch_HomeMapCell(mx + 8 + 8 * 21 + 7, my + 4 + 8 * 14 + 7, &cx, &cy) && cx == 21 && cy == 14);
    assert(!CtrPokeTouch_HomeMapCell(mx + 7, my + 10, &cx, &cy));                 /* left of cell 0 */
    assert(!CtrPokeTouch_HomeMapCell(mx + 20, my + 4 + 8 * 15, &cx, &cy));        /* below row 14 */
    assert(!CtrPokeTouch_HomeMapCell(mx + 8 + 8 * 22, my + 20, &cx, &cy));        /* right of column 21 */
    assert(!CtrPokeTouch_HomeMapCell(kHomeCaption.x + 20, kHomeCaption.y + 8, &cx, &cy));
    /* old_views_keep_old_frame: the page is HOME alone; any other view keeps the old frame and column */
    assert(CtrPokeTouch_HomePageShown(v));
    {
        CtrPokeTouchView w = *v;
        w.bag.on = 1; assert(!CtrPokeTouch_HomePageShown(&w));
        w = *v; w.viewport = 1; assert(!CtrPokeTouch_HomePageShown(&w));
        w = *v; w.options = 1; assert(!CtrPokeTouch_HomePageShown(&w));
        w = *v; w.home = 0; assert(!CtrPokeTouch_HomePageShown(&w));
    }
    /* home_update_equals_draw: frame type, the caption's and the message's text, the map, the player */
    {
        CtrPokeTouchView old = *v;
        v->frameType = 4; update(&old, v);
        strcpy(v->bar[0], "SEAFOAM ISLANDS"); update(&old, v);
        strcpy(v->bar[2], "Tap a destination"); update(&old, v);
        v->mapGroup = PT_MAP_SEVII123; update(&old, v);
        v->playerX = 3; v->playerY = 4; update(&old, v);
        v->cursorX = 5; v->cursorY = 6; v->cursorFrame = 1; update(&old, v);
        v->mapGroup = PT_MAP_KANTO; v->frameType = 0; v->cursorX = v->cursorY = v->playerX = v->playerY = -1;
        strcpy(v->bar[0], "TEST MAP"); v->bar[2][0] = 0; update(&old, v);
    }
}

/* home_frame_change (Review Focus 1): with frames that differ in every window and have clear pixels where the type
 * before drew, each OPTIONS -> FRAME change while HOME shows repaints every window as a full draw would, nothing of the
 * old frame left. The outline without the pack is the same for every type, so this needs the fixture's frames. */
static void home_frame_change(CtrPokeTouchView *v)
{
    static uint16_t first[W * H];
    const CtrPokeTouchRect *windows[] = {&kHomeMapPanel, &kHomeMenu, &kHomeSettings, &kHomeParty};
    int type = v->frameType;
    CtrPokeTouchView old;

    kitFixture = 1;
    sKitStd.type = sKitDialogue.type = -2;
    v->frameType = 0;
    CtrPokeTouch_Draw(canvas, v);
    assert(KitFrameReady(KIT_FRAME_STD, 0));
    memcpy(first, canvas, sizeof(first));
    v->frameType = 1;
    CtrPokeTouch_Draw(canvas, v);
    for (size_t i = 0; i < sizeof(windows) / sizeof(windows[0]); ++i) {
        const CtrPokeTouchRect *r = windows[i];
        int differs = 0;

        for (int x = r->x; x < r->x + 8; ++x)
            for (int y = r->y; y < r->y + r->h; ++y)
                differs |= first[x * H + H - 1 - y] != canvas[x * H + H - 1 - y];
        assert(differs);
    }
    v->frameType = 0;
    CtrPokeTouch_Draw(canvas, v);
    old = *v;
    for (int t = 1; t < KIT_STD_TYPES; ++t) {
        v->frameType = t;
        update(&old, v);
    }
    v->frameType = 0;
    update(&old, v);
    kitFixture = 0;
    sKitStd.type = sKitDialogue.type = -2;
    v->frameType = type;
    CtrPokeTouch_Draw(canvas, v);
}

/* The field BAG as a page (phase 2, the owner's "3"): the pockets' tabs, the rows with their items' icons, the scroll
 * cells, CANCEL; a case's title; the menu, YES / NO, the quantity box and a message; every step an update equal to a full
 * draw. The item icon stub is a solid square, red for item 1 and blue for the others; the font is blank but for the
 * arrow. */
static int BagInk(int x, int y, int w, int h)
{
    int n = 0;

    for (int px = x; px < x + w; ++px)
        for (int py = y; py < y + h; ++py)
            n += at(px, py) == KitInk(2);
    return n;
}

static void bag_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchBag *b = &v.bag;
    static const char *const names[4] = {"POTION", "ANTIDOTE", "REPEL", "ETHER"};

    v.home = 0;
    memset(b, 0, sizeof(*b));
    b->on = 1; b->kind = PT_BAG_KIND_BAG; b->pocket = 0; b->state = PT_BAG_STATE_LIST; b->count = 6; b->cursor = 1;
    b->first = 0; b->moveType = -1; b->pressed = -1;
    for (int i = 0; i < 4; ++i) {
        b->row[i].item = (uint16_t)(i + 1); b->row[i].qty = (uint16_t)(5 - i);
        snprintf(b->row[i].name, sizeof(b->row[i].name), "%s", names[i]);
    }
    b->selItem = 2; snprintf(b->selName, sizeof(b->selName), "ANTIDOTE");
    snprintf(b->selDesc, sizeof(b->selDesc), "A spray-type medicine. It heals one POKEMON from a poisoning.");
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    assert(CtrPokeTouch_BagPageShown(&v) && !CtrPokeTouch_HomePageShown(&v));
    /* the outline checks hold without the pack (the kit's outline); with FIRERED_TEST_PAK the frames are its tiles */
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    /* bag_page_bg: the page's colour round the windows */
    for (int y = 0; y < H; ++y)
        assert(at(0, y) == KitInk(3) && at(3, y) == KitInk(3));
    /* bag_tabs: three framed tabs, FireRed's arrow in the pocket shown; each a hit */
    for (int i = 0; i < 3; ++i) {
        const CtrPokeTouchRect *t = &kBagTab[i];

        assert(!outline || at(t->x, t->y + 15) == KitInk(2));
        assert((BagInk(t->x + 8, t->y + 8, t->w - 16, t->h - 16) > 0) == (i == 0));
        assert(CtrPokeTouch_BagHit(&v, t->x + t->w / 2, t->y + t->h / 2) == PT_BAG_HIT_TAB + i);
    }
    /* bag_rows: each item's icon at 1:1 (red for item 1, blue for the others), the arrow on the Bag's selection; each row a
     * hit */
    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect r = BagRow(i);

        assert(at(r.x + 8 + 12, r.y + 3 + 12) == GbaColor(i == 0 ? 31 : 31 << 10));
        assert((BagInk(r.x, r.y + 8, 8, 14) > 20) == (i == 1));     /* the arrow: 5 px wide in this font */
        assert(CtrPokeTouch_BagHit(&v, r.x + r.w / 2, r.y + r.h / 2) == PT_BAG_HIT_ROW + i);
    }
    /* bag_scroll_and_cancel: up has nothing above the first row, down has more; CANCEL is the Bag's B */
    assert(CtrPokeTouch_BagHit(&v, kBagUp.x + 12, kBagUp.y + 30) == -1);
    assert(CtrPokeTouch_BagHit(&v, kBagDown.x + 12, kBagDown.y + 30) == PT_BAG_HIT_DOWN);
    assert(CtrPokeTouch_BagHit(&v, kBagCancel.x + 30, kBagCancel.y + 28) == PT_BAG_HIT_BACK);
    assert(CtrPokeTouch_BagHit(&v, kBagNote.x + 30, kBagNote.y + 28) == -1);
    assert(!outline || (at(kBagCancel.x, kBagCancel.y + 20) == KitInk(2) && at(kBagList.x, kBagList.y + 20) == KitInk(2)));
    /* a row held: the light grey under it and the arrow with it; the selection moves; the list scrolls */
    b->pressed = PT_BAG_HIT_ROW + 2; update(&old, &v);
    {
        CtrPokeTouchRect r = BagRow(2);

        assert(at(r.x + 120, r.y + 2) == KitInk(3) && BagInk(r.x, r.y + 8, 8, 14) > 20);
        assert(BagInk(BagRow(1).x, BagRow(1).y + 8, 8, 14) == 0);
    }
    b->pressed = -1; b->cursor = 2; update(&old, &v);
    b->first = 1; b->cursor = 3; update(&old, &v);
    assert(CtrPokeTouch_BagHit(&v, kBagUp.x + 12, kBagUp.y + 30) == PT_BAG_HIT_UP);
    /* CLOSE BAG, the row after the last item */
    b->first = 3; b->cursor = 6; b->row[3].item = 0; update(&old, &v);
    assert(CtrPokeTouch_BagHit(&v, BagRow(3).x + 40, BagRow(3).y + 10) == PT_BAG_HIT_ROW + 3);
    b->first = 0; b->cursor = 1; b->row[3].item = 4; update(&old, &v);
    /* a tab's pocket on the way (asked): that tab chosen, not the one the Bag is still in */
    b->asked = 3; update(&old, &v);
    assert(BagInk(kBagTab[2].x + 8, kBagTab[2].y + 8, kBagTab[2].w - 16, 14) > 0);
    assert(BagInk(kBagTab[0].x + 8, kBagTab[0].y + 8, kBagTab[0].w - 16, 14) == 0);
    b->asked = 0; update(&old, &v);
    /* a key item registered on X: its button in its row */
    b->pocket = 1; b->regX = 3; update(&old, &v);
    b->pocket = 0; b->regX = 0; update(&old, &v);
    /* a case: its title across, no tabs */
    b->kind = PT_BAG_KIND_TMCASE; snprintf(b->title, sizeof(b->title), "TM CASE"); b->moveType = 10;
    snprintf(b->typeName, sizeof(b->typeName), "FIRE"); update(&old, &v);
    assert(!outline || (at(kBagTitle.x, kBagTitle.y + 15) == KitInk(2) && at(kBagTab[1].x - 2, kBagTab[1].y + 15) != BPageBg()));
    assert(CtrPokeTouch_BagHit(&v, kBagTab[1].x + 40, kBagTab[1].y + 15) == -1);
    assert(ColorIn(kBagNote.x, kBagNote.y, kBagNote.w, kBagNote.h, UiTypeColor(10)) > 0);   /* the TM's move type */
    b->kind = PT_BAG_KIND_SHOP; snprintf(b->title, sizeof(b->title), "POKe MART"); b->money = 3000; b->moveType = -1;
    b->row[0].price = 200; update(&old, &v);
    b->kind = PT_BAG_KIND_BAG; update(&old, &v);
    /* the menu: the Party's window at the bottom right, the band narrower; a row, or B off the window */
    b->state = PT_BAG_STATE_ACTIONS; b->menuCount = 4; b->menuCursor = 0;
    for (int i = 0; i < 4; ++i)
        snprintf(b->menuName[i], sizeof(b->menuName[i]), "%s", (const char *[]){"USE", "GIVE", "TOSS", "CANCEL"}[i]);
    update(&old, &v);
    {
        CtrPokeTouchRect w = PartyMenuWindow(4);

        assert((!outline || at(w.x, w.y + 20) == KitInk(2)) && at(kBagNoteMenu.x + kBagNoteMenu.w + 1, kBagNote.y + 20) == BPageBg());
        for (int i = 0; i < 4; ++i) {
            CtrPokeTouchRect r = PartyMenuRow(4, i);

            assert(CtrPokeTouch_BagHit(&v, r.x + r.w / 2, r.y + r.h / 2) == PT_BAG_HIT_MENU + i);
            assert((BagInk(r.x, r.y, 8, 16) > 20) == (i == 0));
        }
        assert(CtrPokeTouch_BagHit(&v, BagRow(0).x + 30, BagRow(0).y + 10) == PT_BAG_HIT_BACK);
    }
    b->pressed = PT_BAG_HIT_MENU + 2; update(&old, &v);
    b->pressed = -1; update(&old, &v);
    /* YES / NO */
    b->state = PT_BAG_STATE_YESNO; b->menuCount = 2; b->menuCursor = 1;
    snprintf(b->prompt, sizeof(b->prompt), "Throw away 2 POTIONS?"); update(&old, &v);
    assert(CtrPokeTouch_BagHit(&v, PartyMenuRow(2, 0).x + 20, PartyMenuRow(2, 0).y + 8) == PT_BAG_HIT_MENU);
    assert(CtrPokeTouch_BagHit(&v, PartyMenuRow(2, 1).x + 20, PartyMenuRow(2, 1).y + 8) == PT_BAG_HIT_MENU + 1);
    /* the quantity box */
    b->state = PT_BAG_STATE_QUANTITY; b->tossCount = 2; b->tossMax = 5; update(&old, &v);
    assert(!outline || at(kBagQtyCount.x, kBagQtyCount.y + 20) == KitInk(2));
    assert(CtrPokeTouch_BagHit(&v, kBagQtyUp.x + 28, kBagQtyUp.y + 20) == PT_BAG_HIT_QTY_UP);
    assert(CtrPokeTouch_BagHit(&v, kBagQtyDown.x + 28, kBagQtyDown.y + 20) == PT_BAG_HIT_QTY_DOWN);
    assert(CtrPokeTouch_BagHit(&v, kBagQtyOk.x + 28, kBagQtyOk.y + 28) == PT_BAG_HIT_QTY_OK);
    assert(CtrPokeTouch_BagHit(&v, kBagQtyCancel.x + 28, kBagQtyCancel.y + 28) == PT_BAG_HIT_QTY_CANCEL);
    b->pressed = PT_BAG_HIT_QTY_UP; update(&old, &v);
    b->pressed = -1; b->tossCount = 3; update(&old, &v);
    /* a message that waits for A: the whole band, CANCEL gone, anywhere goes on */
    b->state = PT_BAG_STATE_MESSAGE; snprintf(b->prompt, sizeof(b->prompt), "Threw away 2 POTIONS."); update(&old, &v);
    assert(at(kBagCancel.x + 30, kBagCancel.y + 28) == KitInk(1));
    assert(CtrPokeTouch_BagHit(&v, 10, 10) == PT_BAG_HIT_MESSAGE && CtrPokeTouch_BagHit(&v, 300, 230) == PT_BAG_HIT_MESSAGE);
    b->state = PT_BAG_STATE_LIST; b->menuCount = 0; update(&old, &v);
    /* the FRAME the player chose */
    v.frameType = 6; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    /* leaving: the page goes and the field's view is drawn whole */
    CtrPokeTouch_Draw(canvas, base);
}

/* For a look at the pages (a local viewing aid, never a ROM screenshot): with PT_SHOTS naming a directory, Shot writes
 * the canvas there as <name>.ppm, and PT_SHOTS_FONT names a 256x64-byte glyph file to draw the text with. */
static void Shot(const char *name)
{
    const char *dir = getenv("PT_SHOTS");
    char path[512];
    FILE *f;

    if (!dir)
        return;
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n320 240\n255\n");
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            uint16_t c = at(x, y);
            uint8_t p[3] = {(uint8_t)((c >> 11) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63), (uint8_t)((c & 31) * 255 / 31)};

            fwrite(p, 1, 3, f);
        }
    fclose(f);
}

static void ShotOf(const CtrPokeTouchView *v, const char *name)
{
    CtrPokeTouch_Draw(canvas, v);
    Shot(name);
}

/* The dex page's states with names and numbers, for Shot. */
static void DexSample(CtrPokeTouchDex *d)
{
    static const int8_t ids[19] = {PT_DEX_ID_HEADER, PT_DEX_ID_KANTO, PT_DEX_ID_NATIONAL, PT_DEX_ID_HEADER,
                                   0, 1, 2, 3, 4, 5, 6, 7, 8, PT_DEX_ID_HEADER, 10, 11, 12, 13, PT_DEX_ID_CANCEL};
    static const char *const names[19] = {"POKeMON LIST", "NUMERICAL MODE", "NUMERICAL MODE", "POKeMON HABITATS",
        "GRASSLAND POKeMON", "FOREST POKeMON", "WATERS-EDGE POKeMON", "SEA POKeMON", "CAVE POKeMON",
        "MOUNTAIN POKeMON", "ROUGH-TERRAIN POKeMON", "URBAN POKeMON", "RARE POKeMON", "SEARCH", "A TO Z MODE",
        "TYPE MODE", "LIGHTEST MODE", "SMALLEST MODE", "CLOSE POKeDEX"};
    static const char *const mons[5] = {"BULBASAUR", "IVYSAUR", "VENUSAUR", "CHARMANDER", "CHARMELEON"};

    memset(d, 0, sizeof(*d));
    d->on = 1; d->state = PT_DEX_STATE_MENU; d->pressed = -1; d->menuCount = 19; d->menuCursor = 1;
    d->type[0] = d->type[1] = -1; d->seen[0] = 40; d->owned[0] = 12; d->seen[1] = 60; d->owned[1] = 20;
    for (int i = 0; i < 19; ++i) {
        d->menu[i].id = ids[i];
        snprintf(d->menu[i].name, sizeof(d->menu[i].name), "%s", names[i]);
    }
    d->menu[8].locked = 1;
    d->listCount = 151; d->listCursor = 2; d->listFirst = 0;
    for (int i = 0; i < PT_DEX_ROWS; ++i) {
        d->row[i].species = (uint16_t)(i + 1); d->row[i].number = (uint16_t)(i + 1); d->row[i].flags = (uint8_t)(i < 3 ? 3 : 1);
        snprintf(d->row[i].name, sizeof(d->row[i].name), "%s", mons[i]);
        d->mon[i < 4 ? i : 3] = d->row[i];
    }
    d->sel = d->row[2]; d->type[0] = 11; d->type[1] = 3;
    snprintf(d->typeName[0], sizeof(d->typeName[0]), "GRASS"); snprintf(d->typeName[1], sizeof(d->typeName[1]), "POISON");
    d->slots = 4; d->slot = 1; d->page = 1; d->pages = 3; d->canNext = 1;
    d->height = 20; d->weight = 1000;
    snprintf(d->kind, sizeof(d->kind), "SEED");
    snprintf(d->desc, sizeof(d->desc), "There is a large flower on VENUSAUR's back. The flower is said to take on vivid "
             "colors if it gets plenty of nutrition and sunlight.");
}

static void BoxText(uint8_t *out, int size, const char *s);

static void preview_pages(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base;
    CtrPokeTouchDex *d = &v.dex;

    v.home = 0; v.backVisible = 1; v.backEnabled = 1;
    DexSample(d);
    ShotOf(&v, "dex_1_menu");
    d->menuCursor = 6; ShotOf(&v, "dex_2_habitats");
    d->menuCursor = 15; d->pressed = PT_DEX_HIT_ITEM + 16; ShotOf(&v, "dex_3_search_held");
    d->pressed = -1; d->state = PT_DEX_STATE_LIST; ShotOf(&v, "dex_4_list");
    d->state = PT_DEX_STATE_CATEGORY; d->sel = d->mon[1]; ShotOf(&v, "dex_5_habitat_page");
    d->state = PT_DEX_STATE_PAGE; d->sel = d->row[2]; ShotOf(&v, "dex_6_mon_page");
    d->added = 1; ShotOf(&v, "dex_7_added");
    memset(d, 0, sizeof(*d));
    {
        static const char *const labels[7] = {"TEXT SPEED", "BATTLE SCENE", "BATTLE STYLE", "SOUND", "BUTTON MODE", "FRAME",
                                              "FPS COUNTER"};
        static const char *const values[7] = {"MID", "ON", "SHIFT", "MONO", "HELP", "TYPE1", "OFF"};

        v.options = 1; v.optPage = 0; v.optPageCount = 3; v.optCount = 7; v.optSelected = 5; v.optPressed = -1;
        for (int i = 0; i < 7; ++i) {
            snprintf(v.optLabel[i], sizeof(v.optLabel[i]), "%s", labels[i]);
            snprintf(v.optValue[i], sizeof(v.optValue[i]), "%s", values[i]);
        }
        snprintf(v.bar[0], sizeof(v.bar[0]), "FRAME"); snprintf(v.bar[1], sizeof(v.bar[1]), "The frame of menu");
        snprintf(v.bar[2], sizeof(v.bar[2]), "and YES/NO boxes.");
        v.optFrameShow = 1; v.optFrameType = (uint8_t)v.frameType;
        ShotOf(&v, "opt_1_page1");
        v.optFrameShow = 0; v.optPage = 1; v.optCount = 7; v.optSelected = 5;
        snprintf(v.optLabel[5], sizeof(v.optLabel[5]), "COMPANION"); v.optSegCount[5] = 3; v.optSegValue[5] = 1;
        snprintf(v.optSegName[5][0], 8, "OFF"); snprintf(v.optSegName[5][1], 8, "FOLLOW"); snprintf(v.optSegName[5][2], 8, "BE IT");
        v.optPressed = 3; v.optZone = PT_OPT_ZONE_PREV;
        ShotOf(&v, "opt_2_page2_held");
        v.optPressed = -1; v.optSegCount[5] = 0; v.optPage = 2; v.optCount = 3; v.optSelected = 0; v.optLook.on = 1;
        snprintf(v.optLabel[0], 16, "RED LOOK"); snprintf(v.optValue[0], 12, "ASH HGSS");
        snprintf(v.optLook.name, sizeof(v.optLook.name), "ASH HGSS");
        snprintf(v.optLook.credit, sizeof(v.optLook.credit), "Made by PKMNTrainerRick, with thanks to the HGSS team");
        ShotOf(&v, "opt_3_page3_look");
        memset(&v.optLook, 0, sizeof(v.optLook));
        v.options = 0; v.save = 1; v.saveState = SAVE_CONFIRM; v.saveSel = 0; v.savePressed = PT_SAVE_NONE;
        v.saveBadges = 3; v.saveHasDex = 1; v.saveDex = 25; v.saveHours = 12; v.saveMinutes = 34; v.saveMoney = 3000;
        snprintf(v.playerName, sizeof(v.playerName), "Red");
        snprintf(v.bar[0], sizeof(v.bar[0]), "Would you like to save"); snprintf(v.bar[1], sizeof(v.bar[1]), "your adventure?");
        v.backVisible = 0;
        ShotOf(&v, "save_1_confirm");
        v.saveState = SAVE_SUCCESS; snprintf(v.bar[0], sizeof(v.bar[0]), "Your game has been saved!"); v.bar[1][0] = 0;
        ShotOf(&v, "save_2_done");
        v.save = 0; v.backVisible = 1;
    }
    {
        CtrPokeTouchCard *c = &v.card;
        static const char *const labels[6] = {"NAME: ", "IDNo.", "MONEY", "\xc2\xa5", "POKeDEX", "TIME"};
        static const char *const records[5][2] = {{"HALL OF FAME DEBUT", "0:00"}, {"POKeMON TRADES", "3"},
                                                  {"BERRY CRUSH PAIRS", "0"}, {"LINK BATTLES", "W1 L2"}, {"UNION TRADES", "0"}};

        memset(c, 0, sizeof(*c));
        c->on = 1; c->id = 12345; c->money = 3000; c->hasDex = 1; c->caught = 25; c->hours = 12; c->minutes = 34;
        c->stars = 2; c->badges = 0x0B; snprintf(c->name, sizeof(c->name), "RED");
        for (int i = 0; i < 6; ++i)
            snprintf(c->label[i], sizeof(c->label[i]), "%s", labels[i]);
        c->rows = 5;
        for (int i = 0; i < 5; ++i) {
            snprintf(c->row[i].label, sizeof(c->row[i].label), "%s", records[i][0]);
            snprintf(c->row[i].value, sizeof(c->row[i].value), "%s", records[i][1]);
        }
        cardArtOk = 1;
        ShotOf(&v, "card_1_front");
        c->back = 1; ShotOf(&v, "card_2_back");
        cardArtOk = 0;
        memset(c, 0, sizeof(*c));
    }
    {
        CtrPokeTouchBox *b = &v.box;

        memset(b, 0, sizeof(*b));
        b->on = 1; b->state = PT_BOX_STATE_BOX; b->area = ST_AREA_BOX; b->pos = 7; b->pressed = -1;
        BoxText(b->boxName, sizeof(b->boxName), "BOX A");
        for (int i = 0; i < 30; i += 4)
            b->slot[i].species = (uint16_t)(i + 1);
        b->party[0].species = 25; b->party[1].species = 7; b->party[2].species = 1;
        b->mon = b->slot[8];
        BoxText(b->monText[0], sizeof(b->monText[0]), "PIDGEY");
        BoxText(b->monText[1], sizeof(b->monText[1]), "PIDGEY");
        BoxText(b->monText[2], sizeof(b->monText[2]), "LV");
        BoxText(b->monText[3], sizeof(b->monText[3]), "NO ITEM");
        memset(b->message, 0xff, sizeof(b->message));
        wallOk = monIconOk = 1; sFXWallId = -1;
        ShotOf(&v, "box_1_box");
        b->holding = 1; b->held.species = 3; b->pos = 9; ShotOf(&v, "box_2_holding");
        b->holding = 0; b->area = ST_AREA_PARTY; b->pos = 1; ShotOf(&v, "box_3_party");
        b->area = ST_AREA_BOX; b->pos = 8; b->state = PT_BOX_STATE_MENU; b->menuCount = 5; b->menuCursor = 0;
        for (int i = 0; i < 5; ++i)
            BoxText(b->menu[i], sizeof(b->menu[i]), (const char *[]){"MOVE", "SUMMARY", "WITHDRAW", "MARK", "CANCEL"}[i]);
        BoxText(b->message, sizeof(b->message), "PIDGEY IS SELECTED");
        ShotOf(&v, "box_4_menu");
        b->state = PT_BOX_STATE_CHOOSE; BoxText(b->chooseName, sizeof(b->chooseName), "BOX B"); b->chooseCount = 12;
        BoxText(b->message, sizeof(b->message), "JUMP TO WHICH BOX");
        ShotOf(&v, "box_5_choose");
        b->state = PT_BOX_STATE_MARKS; b->menuCount = 6; b->menuCursor = 1; b->marks = 5;
        ShotOf(&v, "box_6_marks");
        b->state = PT_BOX_STATE_MESSAGE; BoxText(b->message, sizeof(b->message), "PIDGEY WAS RELEASED");
        ShotOf(&v, "box_7_message");
        wallOk = monIconOk = 0; sFXWallId = -1;
        memset(b, 0, sizeof(*b));
    }
    {
        CtrPokeTouchChoice *c = &v.choice;
        static const char *const pc[4] = {"SOMEONE'S PC", "BILL'S PC", "PROF. OAK'S PC", "LOG OFF"};

        memset(c, 0, sizeof(*c));
        c->on = 1; c->count = 2; c->cursor = 0; c->pressed = -1;
        snprintf(c->label[0], sizeof(c->label[0]), "YES"); snprintf(c->label[1], sizeof(c->label[1]), "NO");
        snprintf(c->prompt, sizeof(c->prompt), "Would you like to give a nickname to the received BULBASAUR?");
        ShotOf(&v, "choice_1_yesno");
        c->count = 4; c->cursor = 2;
        for (int i = 0; i < 4; ++i)
            snprintf(c->label[i], sizeof(c->label[i]), "%s", pc[i]);
        snprintf(c->prompt, sizeof(c->prompt), "Which PC should be accessed?");
        ShotOf(&v, "choice_2_pc");
        c->cols = 3; c->count = 6;
        for (int i = 0; i < 6; ++i)
            snprintf(c->label[i], sizeof(c->label[i]), "GRID %d", i);
        ShotOf(&v, "choice_3_grid");
        memset(c, 0, sizeof(*c));
    }
    {
        CtrPokeTouchLearn *l = &v.learn;
        static const char *const moves[5] = {"THUNDERSHOCK", "GROWL", "THUNDER WAVE", "QUICK ATTACK", "THUNDERBOLT"};
        static const int8_t types[5] = {13, 0, 13, 0, 13};

        memset(l, 0, sizeof(*l));
        l->on = 1; l->cursor = 1; l->pressed = -1; l->species = 25;
        snprintf(l->nickname, sizeof(l->nickname), "PIKACHU"); snprintf(l->level, sizeof(l->level), "Lv26");
        snprintf(l->question, sizeof(l->question), "Which move should be forgotten?");
        for (int i = 0; i < 5; ++i) {
            l->row[i].move = (uint16_t)(i + 1); l->row[i].type = types[i];
            snprintf(l->row[i].name, sizeof(l->row[i].name), "%s", moves[i]);
            snprintf(l->row[i].pp, sizeof(l->row[i].pp), "%d/30", 30 - 7 * i);
            snprintf(l->row[i].power, sizeof(l->row[i].power), "40");
            snprintf(l->row[i].accuracy, sizeof(l->row[i].accuracy), "100");
            snprintf(l->row[i].typeName, sizeof(l->row[i].typeName), "%s", types[i] ? "ELECTR" : "NORMAL");
        }
        snprintf(l->desc, sizeof(l->desc), "Loudly roars to reduce the foe's ATTACK rating.");
        ShotOf(&v, "learn_1");
        l->pressed = PT_LEARN_HIT_ROW + 4; ShotOf(&v, "learn_2_held_new");
        memset(l, 0, sizeof(*l));
    }
    {
        CtrPokeTouchName *n = &v.name;

        memset(n, 0, sizeof(*n));
        n->on = 1; n->page = 1; n->cols = 8; n->maxChars = 10; n->len = 3; n->curCol = 2; n->curRow = 1; n->pressed = -1;
        n->gender = 1; n->species = 25;
        memset(n->text, 0xff, sizeof(n->text));
        n->text[0] = 0xCA; n->text[1] = 0xC3; n->text[2] = 0xC5;
        for (int r = 0; r < PT_NAME_ROWS; ++r)
            for (int c = 0; c < PT_NAME_COLS; ++c)
                n->key[r][c] = (uint8_t)(r * 8 + c < 26 ? 0xBB + r * 8 + c : r == 3 && c == 2 ? 0 : 0xff);
        memset(n->title, 0xff, sizeof(n->title));
        BoxText(n->title, sizeof(n->title), "PIKACHU NICKNAME");
        ShotOf(&v, "name_1");
        n->curCol = 8; n->curRow = 0; ShotOf(&v, "name_2_page_button");
        memset(n, 0, sizeof(*n));
    }
    CtrPokeTouch_DrawIdle(canvas, 1);
    Shot("idle");
    v.viewport = 1; v.backVisible = 1; v.backEnabled = 1;
    snprintf(v.bar[0], sizeof(v.bar[0]), "FAME CHECKER"); snprintf(v.bar[1], sizeof(v.bar[1]), "Use the game's own controls.");
    snprintf(v.bar[2], sizeof(v.bar[2]), "BACK is the B button.");
    ShotOf(&v, "view_1_gba");
    v.viewport = 0;
}

/* The page colour round every window: the screen's 4 px margin. */
static void PageMargin(void)
{
    for (int y = 0; y < H; ++y)
        assert(at(0, y) == KitInk(3) && at(3, y) == KitInk(3) && at(W - 1, y) == KitInk(3));
    for (int x = 0; x < W; ++x)
        assert(at(x, 0) == KitInk(3) && at(x, H - 1) == KitInk(3));
}

/* The arrow (5 px wide in this font) at the left of r, under a 14 px line centred in it. */
static int ArrowAt(const CtrPokeTouchRect *r)
{
    return BagInk(r->x, r->y, 8, r->h) > 20;
}

/* The POKeDEX as a page: the top menu's tabs and rows (six a page, CLOSE in LISTS), the side window, the scroll cells;
 * an ordered list; a habitat page with its arrows; a Pokemon's page with AREA and CRY, and the one a catch adds; BACK
 * everywhere but there. Every step an update equal to a full draw. */
static void dex_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchDex *d = &v.dex;
    static const int8_t ids[19] = {PT_DEX_ID_HEADER, PT_DEX_ID_KANTO, PT_DEX_ID_NATIONAL, PT_DEX_ID_HEADER,
                                   0, 1, 2, 3, 4, 5, 6, 7, 8, PT_DEX_ID_HEADER, 10, 11, 12, 13, PT_DEX_ID_CANCEL};

    v.home = 0;
    v.backVisible = 1; v.backEnabled = 1; v.backPressed = 0;
    memset(d, 0, sizeof(*d));
    d->on = 1; d->state = PT_DEX_STATE_MENU; d->pressed = -1; d->menuCount = 19; d->menuCursor = 1;
    d->type[0] = d->type[1] = -1;
    d->seen[0] = 40; d->owned[0] = 12; d->seen[1] = 60; d->owned[1] = 20;
    for (int i = 0; i < 19; ++i) {
        d->menu[i].id = ids[i];
        snprintf(d->menu[i].name, sizeof(d->menu[i].name), "ROW %d", i);
    }
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(!CtrPokeTouch_HomePageShown(&v) && !CtrPokeTouch_BagPageShown(&v));
    PageMargin();
    /* dex_tabs: the BAG's three tabs, the arrow in the section shown; each a hit */
    for (int i = 0; i < 3; ++i) {
        const CtrPokeTouchRect *t = &kDexTab[i];

        assert(!outline || at(t->x, t->y + 15) == KitInk(2));
        assert((BagInk(t->x + 8, t->y + 8, t->w - 16, t->h - 16) > 0) == (i == 0));
        assert(CtrPokeTouch_DexHit(&v, t->x + t->w / 2, t->y + t->h / 2) == PT_DEX_HIT_TAB + i);
    }
    /* dex_lists_rows: KANTO, NATIONAL and CLOSE as rows of the list window; the arrow on the cursor's */
    {
        static const int rows[3] = {1, 2, 18};

        for (int i = 0; i < 3; ++i) {
            CtrPokeTouchRect r = DexMenuRow(i);

            assert(CtrPokeTouch_DexHit(&v, r.x + r.w / 2, r.y + r.h / 2) == PT_DEX_HIT_ITEM + rows[i]);
            assert(ArrowAt(&r) == (i == 0));
        }
        assert(CtrPokeTouch_DexHit(&v, DexMenuRow(3).x + 20, DexMenuRow(3).y + 10) == -1);
        assert(!outline || (at(kDexList.x, kDexList.y + 20) == KitInk(2) && at(kDexSide.x, kDexSide.y + 20) == KitInk(2)));
        assert(CtrPokeTouch_DexHit(&v, kDexUp.x + 12, kDexUp.y + 30) == -1);
        assert(CtrPokeTouch_DexHit(&v, kDexDown.x + 12, kDexDown.y + 30) == -1);
    }
    /* dex_back: BACK's cell at the bottom right is the Pokedex's B (ProcessBack), the band is nothing */
    assert(CtrPokeTouch_BackHit(&v, kDexBack.x + 30, kDexBack.y + 28));
    assert(!CtrPokeTouch_BackHit(&v, kDexNote.x + 30, kDexNote.y + 28) && !CtrPokeTouch_BackHit(&v, 200, 220));
    assert(!outline || at(kDexBack.x, kDexBack.y + 20) == KitInk(2));
    assert(CtrPokeTouch_DexHit(&v, kDexBack.x + 30, kDexBack.y + 28) == -1);
    v.backPressed = 1; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){(int16_t)(kDexBack.x + 8 + (kDexBack.w - 16 - UiTextWidth(CTR_TR("BACK", "ATR\xC3\x81S", "Zur\xC3\xBC" "ck"), 1)) / 2 - 8),
                                       (int16_t)(kDexBack.y + 8), 8, (int16_t)(kDexBack.h - 16)}));
    v.backPressed = 0; v.backEnabled = 0; update(&old, &v);
    v.backEnabled = 1; update(&old, &v);
    /* dex_habitats: the section's second page (rows 10-12), the arrow on the cursor; up has a page, down none */
    d->menuCursor = 10; update(&old, &v);
    assert(BagInk(kDexTab[1].x + 8, kDexTab[1].y + 8, kDexTab[1].w - 16, 14) > 0);
    assert(BagInk(kDexTab[0].x + 8, kDexTab[0].y + 8, kDexTab[0].w - 16, 14) == 0);
    for (int i = 0; i < DEX_MENU_ROWS; ++i) {
        CtrPokeTouchRect r = DexMenuRow(i);

        assert(CtrPokeTouch_DexHit(&v, r.x + r.w / 2, r.y + r.h / 2) == (i < 3 ? PT_DEX_HIT_ITEM + 10 + i : -1));
        assert(ArrowAt(&r) == (i == 0));
    }
    assert(CtrPokeTouch_DexHit(&v, kDexUp.x + 12, kDexUp.y + 30) == PT_DEX_HIT_UP);
    assert(CtrPokeTouch_DexHit(&v, kDexDown.x + 12, kDexDown.y + 30) == -1);
    assert(CtrPokeTouch_DexGridPageRow(&v, -1) == 4 && CtrPokeTouch_DexGridPageRow(&v, 1) == -1);
    assert(CtrPokeTouch_DexTabRow(&v, 0) == 1 && CtrPokeTouch_DexTabRow(&v, 1) == 4 && CtrPokeTouch_DexTabRow(&v, 2) == 14);
    d->menuCursor = 5; update(&old, &v);
    assert(CtrPokeTouch_DexHit(&v, kDexDown.x + 12, kDexDown.y + 30) == PT_DEX_HIT_DOWN);
    assert(CtrPokeTouch_DexGridPageRow(&v, 1) == 10 && CtrPokeTouch_DexGridPageRow(&v, -1) == -1);
    assert(CtrPokeTouch_DexHit(&v, DexMenuRow(5).x + 20, DexMenuRow(5).y + 10) == PT_DEX_HIT_ITEM + 9);
    /* a locked habitat is toned (still a hit: the touch path refuses it); a row held: the arrow with it, the grey */
    d->menu[6].locked = 1; update(&old, &v);
    d->pressed = PT_DEX_HIT_ITEM + 7; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){DexMenuRow(3).x, DexMenuRow(3).y, 8, DEX_MENU_ROW_H}));
    assert(!ArrowAt(&(CtrPokeTouchRect){DexMenuRow(1).x, DexMenuRow(1).y, 8, DEX_MENU_ROW_H}));
    assert(at(DexMenuRow(3).x + 100, DexMenuRow(3).y + 1) == KitInk(3));
    d->pressed = PT_DEX_HIT_TAB + 2; update(&old, &v);
    assert(BagInk(kDexTab[2].x + 8, kDexTab[2].y + 8, kDexTab[2].w - 16, 14) > 0);
    d->pressed = -1; update(&old, &v);
    /* CLOSE: the cursor on it shows LISTS */
    d->menuCursor = 18; update(&old, &v);
    assert(BagInk(kDexTab[0].x + 8, kDexTab[0].y + 8, kDexTab[0].w - 16, 14) > 0);
    assert(ArrowAt(&(CtrPokeTouchRect){DexMenuRow(2).x, DexMenuRow(2).y, 8, DEX_MENU_ROW_H}));
    /* dex_list: the order's title, five rows (the arrow on the cursor's), the scroll cells; the Pokemon's picture */
    d->state = PT_DEX_STATE_LIST; d->listCount = 151; d->listCursor = 3; d->listFirst = 1; d->order = 0;
    for (int i = 0; i < PT_DEX_ROWS; ++i) {
        d->row[i].species = (uint16_t)(i + 2); d->row[i].number = (uint16_t)(i + 2); d->row[i].flags = (uint8_t)(i & 1 ? 3 : 1);
        snprintf(d->row[i].name, sizeof(d->row[i].name), "MON %d", i);
    }
    d->sel = d->row[2]; d->type[0] = 11; snprintf(d->typeName[0], sizeof(d->typeName[0]), "GRASS");
    update(&old, &v);
    PageMargin();
    assert(!outline || at(kDexTitle.x, kDexTitle.y + 15) == KitInk(2));
    for (int i = 0; i < PT_DEX_ROWS; ++i) {
        CtrPokeTouchRect r = DexListRow(i);

        assert(CtrPokeTouch_DexHit(&v, r.x + r.w / 2, r.y + r.h / 2) == PT_DEX_HIT_ROW + i);
        assert(ArrowAt(&r) == (i == 2));
    }
    assert(CtrPokeTouch_DexHit(&v, kDexUp.x + 12, kDexUp.y + 30) == PT_DEX_HIT_UP);
    assert(CtrPokeTouch_DexHit(&v, kDexDown.x + 12, kDexDown.y + 30) == PT_DEX_HIT_DOWN);
    assert(ColorIn(kDexSide.x + 8, kDexSide.y + 8, kDexSide.w - 16, kDexSide.h - 16, UiTypeColor(11)) > 0);
    assert(CtrPokeTouch_BackHit(&v, kDexBack.x + 30, kDexBack.y + 28));
    d->pressed = PT_DEX_HIT_ROW + 4; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){DexListRow(4).x, DexListRow(4).y, 8, DEX_LIST_ROW_H}));
    d->pressed = -1; d->listFirst = 0; d->listCursor = 0; d->sel.flags = 0; update(&old, &v);
    assert(CtrPokeTouch_DexHit(&v, kDexUp.x + 12, kDexUp.y + 30) == -1);
    d->order = 5; update(&old, &v);
    /* dex_habitat_page: four places two by two, the arrow on the page's cursor; PREV and NEXT beside the band */
    d->state = PT_DEX_STATE_CATEGORY; d->slots = 3; d->slot = 1; d->page = 2; d->pages = 3; d->canPrev = 1; d->canNext = 0;
    for (int i = 0; i < 4; ++i)
        d->mon[i] = d->row[i];
    d->sel = d->mon[1]; update(&old, &v);
    PageMargin();
    assert(!outline || at(kDexHabitat.x, kDexHabitat.y + 20) == KitInk(2));
    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect r = DexSlot(i);

        assert(CtrPokeTouch_DexHit(&v, r.x + r.w / 2, r.y + r.h / 2) == (i < 3 ? PT_DEX_HIT_SLOT + i : -1));
    }
    assert(CtrPokeTouch_DexHit(&v, kDexPrev.x + 14, kDexPrev.y + 28) == PT_DEX_HIT_PREV);
    assert(CtrPokeTouch_DexHit(&v, kDexNext.x + 14, kDexNext.y + 28) == -1);
    assert(CtrPokeTouch_BackHit(&v, kDexBack.x + 30, kDexBack.y + 28));
    d->canNext = 1; d->pressed = PT_DEX_HIT_SLOT + 2; update(&old, &v);
    assert(CtrPokeTouch_DexHit(&v, kDexNext.x + 14, kDexNext.y + 28) == PT_DEX_HIT_NEXT);
    d->pressed = PT_DEX_HIT_NEXT; update(&old, &v);
    d->pressed = -1; update(&old, &v);
    /* dex_mon_page: the picture and the data, the entry, AREA and CRY */
    d->state = PT_DEX_STATE_PAGE; d->sel = d->row[1]; d->height = 7; d->weight = 69; d->type[1] = 3;
    snprintf(d->typeName[1], sizeof(d->typeName[1]), "POISON");
    snprintf(d->kind, sizeof(d->kind), "SEED"); snprintf(d->desc, sizeof(d->desc), "A strange seed was planted on its back at birth.");
    update(&old, &v);
    PageMargin();
    assert(!outline || (at(kDexPic.x, kDexPic.y + 20) == KitInk(2) && at(kDexInfo.x, kDexInfo.y + 20) == KitInk(2)));
    assert(ColorIn(kDexInfo.x, kDexInfo.y, kDexInfo.w, kDexInfo.h, UiTypeColor(3)) > 0);
    assert(CtrPokeTouch_DexHit(&v, kDexArea.x + 60, kDexArea.y + 28) == PT_DEX_HIT_AREA);
    assert(CtrPokeTouch_DexHit(&v, kDexCry.x + 60, kDexCry.y + 28) == PT_DEX_HIT_CRY);
    assert(CtrPokeTouch_DexHit(&v, kDexEntry.x + 60, kDexEntry.y + 28) == -1);
    assert(CtrPokeTouch_BackHit(&v, kDexBack.x + 30, kDexBack.y + 28));
    d->pressed = PT_DEX_HIT_CRY; update(&old, &v);
    d->pressed = -1; update(&old, &v);
    /* dex_added: the page a catch adds: the whole band, no BACK, anywhere goes on */
    d->added = 1; update(&old, &v);
    assert(CtrPokeTouch_DexHit(&v, 10, 10) == PT_DEX_HIT_CONTINUE && CtrPokeTouch_DexHit(&v, 300, 230) == PT_DEX_HIT_CONTINUE);
    assert(!CtrPokeTouch_BackHit(&v, kDexBack.x + 30, kDexBack.y + 28));
    assert(at(kDexBack.x + 30, kDexBack.y + 28) == KitInk(1));
    d->added = 0; update(&old, &v);
    /* the FRAME the player chose; leaving */
    v.frameType = 5; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    CtrPokeTouch_Draw(canvas, base);
}

/* OPTIONS as a page: the title, PAGE, the rows' touch places (label, "<", the value and ">"; a row of levels by its
 * thirds), the arrow on the row the band describes, a held place's grey, the look's preview under a short page, FRAME's
 * sample in the band, BACK. */
static void options_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    int row, zone;

    v.home = 0; v.options = 1; v.optPage = 0; v.optPageCount = 3; v.optCount = 7; v.optSelected = 2; v.optPressed = -1;
    v.backVisible = 1; v.backEnabled = 1; v.backPressed = 0;
    for (int i = 0; i < 7; ++i) {
        snprintf(v.optLabel[i], sizeof(v.optLabel[i]), "ROW %d", i);
        snprintf(v.optValue[i], sizeof(v.optValue[i]), "VALUE");
        v.optSwitch[i] = 0; v.optSegCount[i] = 0;
    }
    v.optSwitch[1] = 2;
    v.optSegCount[5] = 3; v.optSegValue[5] = 1;
    for (int k = 0; k < 3; ++k)
        snprintf(v.optSegName[5][k], sizeof(v.optSegName[5][k]), "L%d", k);
    snprintf(v.bar[0], sizeof(v.bar[0]), "TEXT SPEED"); snprintf(v.bar[1], sizeof(v.bar[1]), "How fast dialogue");
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(CtrPokeTouch_FieldPageShown(&v));
    PageMargin();
    assert(!outline || (at(kOptTitle.x, kOptTitle.y + 15) == KitInk(2) && at(kOptPage.x, kOptPage.y + 15) == KitInk(2)
                        && at(kOptList.x, kOptList.y + 20) == KitInk(2)));
    assert(CtrPokeTouch_HitOptPage(kOptPage.x + 40, kOptPage.y + 15) && !CtrPokeTouch_HitOptPage(kOptTitle.x + 40, kOptTitle.y + 15));
    /* opt_rows: each row's three places; nothing past the page's rows */
    for (int i = 0; i < 7; ++i) {
        CtrPokeTouchRect r = OptRowRect(i);
        int box = r.x + r.w - OPT_BOX_W;

        assert(CtrPokeTouch_OptionHit(r.x + 20, r.y + 8, 7, &row, &zone) && row == i && zone == PT_OPT_ZONE_LABEL);
        assert(CtrPokeTouch_OptionHit(box + 4, r.y + 8, 7, &row, &zone) && row == i && zone == PT_OPT_ZONE_PREV);
        assert(CtrPokeTouch_OptionHit(box + 60, r.y + 8, 7, &row, &zone) && row == i && zone == PT_OPT_ZONE_NEXT);
        assert(CtrPokeTouch_OptionHit(r.x + r.w - 2, r.y + r.h - 1, 7, &row, &zone) && zone == PT_OPT_ZONE_NEXT);
        assert(ArrowAt(&r) == (i == 2));
    }
    assert(!CtrPokeTouch_OptionHit(OptRowRect(3).x + 20, OptRowRect(3).y + 8, 3, &row, &zone));
    assert(!CtrPokeTouch_OptionHit(kOptList.x + 2, OptRowRect(0).y + 8, 7, &row, &zone));
    {
        int box = OptRowRect(5).x + OptRowRect(5).w - OPT_BOX_W;

        assert(CtrPokeTouch_OptionSegment(box + 6, 3) == 0 && CtrPokeTouch_OptionSegment(box + OPT_BOX_W / 2, 3) == 1);
        assert(CtrPokeTouch_OptionSegment(box + OPT_BOX_W - 6, 3) == 2 && CtrPokeTouch_OptionSegment(box - 10, 3) == 0);
    }
    /* BACK closes OPTIONS (ProcessBack); the band is nothing */
    assert(CtrPokeTouch_BackHit(&v, kOptBack.x + 30, kOptBack.y + 28) && !CtrPokeTouch_BackHit(&v, kOptNote.x + 30, kOptNote.y + 28));
    /* a held place: its grey; another row selected; a held level */
    v.optPressed = 3; v.optZone = PT_OPT_ZONE_PREV; update(&old, &v);
    assert(at(OptRowRect(3).x + OptRowRect(3).w - OPT_BOX_W + 2, OptRowRect(3).y + 1) == KitInk(3));
    v.optZone = PT_OPT_ZONE_LABEL; update(&old, &v);
    assert(at(OptRowRect(3).x + 60, OptRowRect(3).y + 1) == KitInk(3));
    v.optZone = PT_OPT_ZONE_NEXT; update(&old, &v);
    v.optPressed = 5; v.optZone = PT_OPT_ZONE_SEG + 2; update(&old, &v);
    v.optPressed = -1; v.optSelected = 4; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){OptRowRect(4).x, OptRowRect(4).y, 8, OPT_ROW_H}));
    v.optPagePressed = 1; update(&old, &v);
    v.optPagePressed = 0; v.optPage = 1; update(&old, &v);
    /* FRAME's sample in the band */
    v.optFrameShow = 1; v.optFrameType = 0; update(&old, &v);
    assert(!outline || at(kOptSample.x, kOptSample.y + 20) == KitInk(2));
    v.optFrameShow = 0; update(&old, &v);
    /* a short page with a look row selected: the preview under its rows */
    v.optPage = 2; v.optCount = 3; v.optSelected = 1; v.optLook.on = 1; v.optLook.has = 1;
    strcpy(v.optLook.name, "ASH HGSS"); strcpy(v.optLook.credit, "Made by someone");
    for (int i = 0; i < 512; ++i) v.optLook.tiles[i] = (uint8_t)(i * 7);
    for (int i = 1; i < 16; ++i) v.optLook.palette[i] = (uint16_t)(0x001F);
    update(&old, &v);
    assert(ColorIn(kOptList.x + 8, OptRowRect(3).y + 2, 80, kOptList.y + kOptList.h - 8 - OptRowRect(3).y - 2, GbaColor(0x001F)) > 0);
    v.optLook.has = 0; update(&old, &v);
    v.optLook.on = 0; v.optCount = 7; update(&old, &v);
    v.backPressed = 1; update(&old, &v);
    v.backPressed = 0; v.frameType = 3; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    CtrPokeTouch_Draw(canvas, base);
}

/* SAVE as a page: the information and the player's windows; YES / NO in the Party's window beside the band (waiting
 * for one of its two), the arrow on the selection or the one held; the band whole while writing (no touch) and for the
 * result (anywhere is OK). No BACK. */
static void save_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchRect yes = PartyMenuRow(2, 0), no = PartyMenuRow(2, 1);

    v.home = 0; v.save = 1; v.saveState = SAVE_CONFIRM; v.saveSel = 0; v.savePressed = PT_SAVE_NONE;
    v.saveBadges = 3; v.saveHasDex = 1; v.saveDex = 25; v.saveHours = 12; v.saveMinutes = 34; v.saveMoney = 3000;
    v.backVisible = 0;
    snprintf(v.bar[0], sizeof(v.bar[0]), "Would you like to save"); snprintf(v.bar[1], sizeof(v.bar[1]), "your adventure?");
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(CtrPokeTouch_FieldPageShown(&v));
    PageMargin();
    assert(!outline || (at(kSaveInfo.x, kSaveInfo.y + 20) == KitInk(2) && at(kSavePlayer.x, kSavePlayer.y + 20) == KitInk(2)));
    assert(CtrPokeTouch_SaveHit(yes.x + 20, yes.y + 8, SAVE_CONFIRM) == PT_SAVE_FIRST);
    assert(CtrPokeTouch_SaveHit(no.x + 20, no.y + 8, SAVE_CONFIRM) == PT_SAVE_SECOND);
    assert(CtrPokeTouch_SaveHit(kSaveNote.x + 30, kSaveNote.y + 20, SAVE_CONFIRM) == PT_SAVE_NONE);
    assert(CtrPokeTouch_SaveHit(10, 10, SAVE_CONFIRM) == PT_SAVE_NONE);
    assert(ArrowAt(&yes) && !ArrowAt(&no));
    assert(!CtrPokeTouch_BackHit(&v, 290, 220));
    v.saveSel = 1; update(&old, &v);
    assert(!ArrowAt(&yes) && ArrowAt(&no));
    v.savePressed = PT_SAVE_FIRST; update(&old, &v);
    assert(ArrowAt(&yes) && !ArrowAt(&no) && at(yes.x + 60, yes.y + 1) == KitInk(3));
    v.savePressed = PT_SAVE_NONE; v.saveMinutes = 35; update(&old, &v);
    v.playerFemale = 1; update(&old, &v);
    /* writing: the whole band, nothing to touch */
    v.saveState = SAVE_WRITING; snprintf(v.bar[0], sizeof(v.bar[0]), "Saving..."); update(&old, &v);
    assert(CtrPokeTouch_SaveHit(yes.x + 20, yes.y + 8, SAVE_WRITING) == PT_SAVE_NONE);
    assert(at(yes.x + 20, yes.y + 8) == KitInk(1) || at(yes.x + 20, yes.y + 8) == KitInk(2));
    /* the result: anywhere is its OK */
    for (int state = SAVE_SUCCESS; state <= SAVE_ERROR; ++state) {
        v.saveState = (uint8_t)state; update(&old, &v);
        assert(CtrPokeTouch_SaveHit(10, 10, state) == PT_SAVE_FIRST && CtrPokeTouch_SaveHit(300, 230, state) == PT_SAVE_FIRST);
        v.savePressed = PT_SAVE_FIRST; update(&old, &v);
        v.savePressed = PT_SAVE_NONE; update(&old, &v);
    }
    v.saveHasDex = 0; update(&old, &v);
    v.frameType = 7; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    CtrPokeTouch_Draw(canvas, base);
}

/* The Trainer Card as a page: the card one window (a touch anywhere on it is the card's own turn), the earned badges at
 * 2x in their places, the grey inside while held, its back; the band and BACK. */
static void card_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchCard *c = &v.card;
    const int bx = kCard.x + 8 + (kCard.w - 16 - (8 * CARD_BADGE + 7 * CARD_BADGE_GAP)) / 2;

    v.home = 0; v.backVisible = 1; v.backEnabled = 1; v.backPressed = 0;
    memset(c, 0, sizeof(*c));
    c->on = 1; c->id = 12345; c->money = 3000; c->hasDex = 1; c->caught = 25; c->hours = 12; c->minutes = 34;
    c->stars = 2; c->badges = 0x05;
    snprintf(c->name, sizeof(c->name), "RED");
    for (int i = 0; i < 6; ++i)
        snprintf(c->label[i], sizeof(c->label[i]), "L%d", i);
    c->rows = 3;
    for (int i = 0; i < 3; ++i) {
        snprintf(c->row[i].label, sizeof(c->row[i].label), "RECORD %d", i);
        snprintf(c->row[i].value, sizeof(c->row[i].value), "%d", i * 10);
    }
    cardArtOk = 1;
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(CtrPokeTouch_FieldPageShown(&v));
    PageMargin();
    assert(!outline || (at(kCard.x, kCard.y + 20) == KitInk(2) && at(kCardBack.x, kCardBack.y + 20) == KitInk(2)));
    /* card_hit: the card's window, nothing else; BACK is the card's own exit */
    assert(CtrPokeTouch_CardHit(&v, kCard.x + 1, kCard.y + 1) && CtrPokeTouch_CardHit(&v, kCard.x + kCard.w - 1, kCard.y + kCard.h - 1));
    assert(!CtrPokeTouch_CardHit(&v, kCardNote.x + 30, kCardNote.y + 20) && !CtrPokeTouch_CardHit(&v, kCardBack.x + 30, kCardBack.y + 20));
    assert(CtrPokeTouch_BackHit(&v, kCardBack.x + 30, kCardBack.y + 28) && !CtrPokeTouch_BackHit(&v, kCard.x + 30, kCard.y + 28));
    /* card_badges: badges 0 and 2 earned, at 2x in their places; the others empty */
    for (int i = 0; i < 8; ++i) {
        int x = bx + i * (CARD_BADGE + CARD_BADGE_GAP), y = kCard.y + kCard.h - 8 - 4 - CARD_BADGE;

        assert((ColorIn(x, y, CARD_BADGE, CARD_BADGE, GbaColor(0x03E0)) == CARD_BADGE * CARD_BADGE) == (i == 0 || i == 2));
    }
    /* held: the card's inside grey */
    c->pressed = 1; update(&old, &v);
    assert(at(kCard.x + 10, kCard.y + kCard.h - 10) == KitInk(3));
    c->pressed = 0; update(&old, &v);
    assert(at(kCard.x + 10, kCard.y + kCard.h - 10) == KitInk(1));
    /* its back: the records; no badges */
    c->back = 1; update(&old, &v);
    assert(!ColorIn(kCard.x, kCard.y, kCard.w, kCard.h, GbaColor(0x03E0)));
    c->hasDex = 0; c->back = 0; c->stars = 5; update(&old, &v);
    v.backPressed = 1; update(&old, &v);
    v.backPressed = 0; v.frameType = 2; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    cardArtOk = 0;
    CtrPokeTouch_Draw(canvas, base);
}

/* FireRed codes for a text field: the letters A-Z, 0xff ended (the rest 0xff). */
static void BoxText(uint8_t *out, int size, const char *s)
{
    memset(out, 0xff, (size_t)size);
    for (int i = 0; s[i] && i < size - 1; ++i)
        out[i] = (uint8_t)(s[i] == ' ' ? 0 : 0xbb + s[i] - 'A');
}

/* The PC box as a page: FireRed's wallpaper at 1:1 in the box's window, its header's places and the thirty places
 * (24 px), the icons over them, PARTY and CLOSE BOX, the Pokemon in hand over the cursor; the Party's panel; the menu,
 * YES / NO and marks in the Party's window; the chooser; a message; BACK where the box takes its B. */
static void box_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchBox *b = &v.box;
    const uint16_t wall = GbaColor(0x5294), icon = GbaColor(0x03FF);

    v.home = 0; v.backVisible = 1; v.backEnabled = 1; v.backPressed = 0;
    memset(b, 0, sizeof(*b));
    b->on = 1; b->state = PT_BOX_STATE_BOX; b->area = ST_AREA_BOX; b->pos = 7; b->pressed = -1;
    BoxText(b->boxName, sizeof(b->boxName), "BOX A");
    b->slot[0].species = 1; b->slot[8].species = 4; b->party[0].species = 25; b->party[1].species = 7;
    b->mon = b->slot[0];
    BoxText(b->monText[0], sizeof(b->monText[0]), "BULBASAUR");
    BoxText(b->monText[1], sizeof(b->monText[1]), "BULBASAUR");
    BoxText(b->monText[2], sizeof(b->monText[2]), "LV");
    BoxText(b->monText[3], sizeof(b->monText[3]), "");
    memset(b->message, 0xff, sizeof(b->message));
    wallOk = monIconOk = 1;
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(CtrPokeTouch_FieldPageShown(&v));
    PageMargin();
    /* box_wallpaper_1to1: its 160x144 exactly at (BOX_WALL_X, BOX_WALL_Y) */
    assert(at(BOX_WALL_X, BOX_WALL_Y + 30) == wall && at(BOX_WALL_X + 159, BOX_WALL_Y + 143) == wall);
    assert(at(BOX_WALL_X - 1, BOX_WALL_Y + 30) != wall && at(BOX_WALL_X + 160, BOX_WALL_Y + 30) != wall);
    assert(at(BOX_WALL_X + 80, BOX_WALL_Y - 1) != wall && at(BOX_WALL_X + 80, BOX_WALL_Y + 144) != wall);
    assert(!outline || (at(kBoxWin.x, kBoxWin.y + 20) == KitInk(2) && at(kBoxMon.x, kBoxMon.y + 20) == KitInk(2)));
    /* box_hits: the places, the header's three, PARTY and CLOSE BOX; the picture's window nothing */
    for (int i = 0; i < 30; ++i) {
        CtrPokeTouchRect c = BoxCell(i);

        assert(CtrPokeTouch_BoxHit(&v, c.x + 12, c.y + 12) == i && CtrPokeTouch_BoxHit(&v, c.x, c.y) == i);
    }
    assert(CtrPokeTouch_BoxHit(&v, kBoxPrev.x + 16, kBoxPrev.y + 12) == ST_HIT_PREV);
    assert(CtrPokeTouch_BoxHit(&v, kBoxTitle.x + 48, kBoxTitle.y + 12) == ST_HIT_TITLE);
    assert(CtrPokeTouch_BoxHit(&v, kBoxNext.x + 16, kBoxNext.y + 12) == ST_HIT_NEXT);
    assert(CtrPokeTouch_BoxHit(&v, kBoxPartyButton.x + 60, kBoxPartyButton.y + 15) == ST_HIT_BUTTON_PARTY);
    assert(CtrPokeTouch_BoxHit(&v, kBoxClose.x + 60, kBoxClose.y + 15) == ST_HIT_BUTTON_CLOSE);
    assert(CtrPokeTouch_BoxHit(&v, kBoxMon.x + 60, kBoxMon.y + 50) == ST_HIT_NONE);
    assert(CtrPokeTouch_BoxHit(&v, kBoxBack.x + 30, kBoxBack.y + 28) == ST_HIT_NONE);
    assert(CtrPokeTouch_BackHit(&v, kBoxBack.x + 30, kBoxBack.y + 28));
    /* box_icons: each Pokemon's icon over its place; the cursor's empty place marked */
    assert(at(BoxCell(0).x + 12, BoxCell(0).y + 12) == icon && at(BoxCell(1).x + 12, BoxCell(1).y + 12) == wall);
    assert(at(BoxCell(7).x + 12, BoxCell(7).y + 12) != wall);
    /* a place held: its grey; the Pokemon in hand drawn over the cursor */
    b->pressed = 3; update(&old, &v);
    assert(at(BoxCell(3).x + 12, BoxCell(3).y + 12) == KitInk(3));
    b->pressed = -1; b->holding = 1; b->held.species = 9; update(&old, &v);
    assert(at(BoxCell(7).x + 12, BoxCell(7).y + 2) == icon);
    b->holding = 0; b->area = ST_AREA_TITLE; b->pos = 0; update(&old, &v);
    b->area = ST_AREA_BUTTONS; b->pos = 1; update(&old, &v);
    /* the Party's panel: the lead, five rows, CANCEL; the box's places are under it */
    b->area = ST_AREA_PARTY; b->pos = 1; b->mon = b->party[1]; update(&old, &v);
    assert(CtrPokeTouch_BoxHit(&v, kBoxLead.x + 36, kBoxLead.y + 60) == ST_HIT_PARTY_BASE);
    for (int i = 1; i < 6; ++i) {
        CtrPokeTouchRect r = BoxPartyRow(i);

        assert(CtrPokeTouch_BoxHit(&v, r.x + 40, r.y + 13) == ST_HIT_PARTY_BASE + i);
    }
    assert(CtrPokeTouch_BoxHit(&v, kBoxPartyCancel.x + 80, kBoxPartyCancel.y + 12) == ST_HIT_PARTY_BASE + 6);
    assert(ArrowAt(&(CtrPokeTouchRect){BoxPartyRow(1).x, BoxPartyRow(1).y, 8, BoxPartyRow(1).h}));
    assert(at(BoxCell(0).x + 12, BoxCell(0).y + 12) != icon || at(kBoxLead.x + 36, kBoxLead.y + 60) == icon);
    b->pos = 6; b->pressed = ST_HIT_PARTY_BASE + 6; update(&old, &v);
    b->pressed = -1; b->area = ST_AREA_BOX; b->pos = 0; b->mon = b->slot[0]; update(&old, &v);
    /* the menu: the Party's window at the bottom right, the band narrower; no BACK under it */
    b->state = PT_BOX_STATE_MENU; b->menuCount = 4; b->menuCursor = 1;
    for (int i = 0; i < 4; ++i)
        BoxText(b->menu[i], sizeof(b->menu[i]), (const char *[]){"MOVE", "SUMMARY", "MARK", "CANCEL"}[i]);
    update(&old, &v);
    for (int i = 0; i < 4; ++i) {
        CtrPokeTouchRect r = PartyMenuRow(4, i);

        assert(CtrPokeTouch_BoxHit(&v, r.x + 30, r.y + 8) == ST_HIT_MENU_BASE + i);
        assert(ArrowAt(&r) == (i == 1));
    }
    assert(CtrPokeTouch_BoxHit(&v, BoxCell(0).x + 12, BoxCell(0).y + 12) == ST_HIT_NONE);
    assert(!CtrPokeTouch_BackHit(&v, kBoxBack.x + 30, kBoxBack.y + 50));
    b->pressed = ST_HIT_MENU_BASE + 3; update(&old, &v);
    b->pressed = -1; b->menuCount = 7; update(&old, &v);
    /* YES / NO */
    b->state = PT_BOX_STATE_YESNO; b->menuCount = 2; b->menuCursor = 0;
    BoxText(b->message, sizeof(b->message), "RELEASE IT");
    update(&old, &v);
    assert(CtrPokeTouch_BoxHit(&v, PartyMenuRow(2, 1).x + 30, PartyMenuRow(2, 1).y + 8) == ST_HIT_MENU_BASE + 1);
    /* the chooser */
    b->state = PT_BOX_STATE_CHOOSE; BoxText(b->chooseName, sizeof(b->chooseName), "BOX B"); b->chooseCount = 12;
    update(&old, &v);
    assert(CtrPokeTouch_BoxHit(&v, kBoxChoosePrev.x + 16, kBoxChoosePrev.y + 28) == ST_HIT_PREV);
    assert(CtrPokeTouch_BoxHit(&v, kBoxChooseName.x + 40, kBoxChooseName.y + 28) == ST_HIT_TITLE);
    assert(CtrPokeTouch_BoxHit(&v, kBoxChooseNext.x + 16, kBoxChooseNext.y + 28) == ST_HIT_NEXT);
    assert(CtrPokeTouch_BackHit(&v, kBoxBack.x + 30, kBoxBack.y + 28));
    assert(!outline || at(kBoxChooseName.x, kBoxChooseName.y + 20) == KitInk(2));
    /* the marks: four marks, OK and CANCEL in the Party's window */
    b->state = PT_BOX_STATE_MARKS; b->menuCount = 6; b->menuCursor = 4; b->marks = 0x5; update(&old, &v);
    for (int i = 0; i < 6; ++i) {
        CtrPokeTouchRect r = PartyMenuRow(6, i);

        assert(CtrPokeTouch_BoxHit(&v, r.x + 30, r.y + 8) == ST_HIT_MENU_BASE + i);
        assert(ArrowAt(&r) == (i == 4));
    }
    /* a message that waits for A: the whole band, anywhere goes on, no BACK */
    b->state = PT_BOX_STATE_MESSAGE; update(&old, &v);
    assert(CtrPokeTouch_BoxHit(&v, 10, 10) == PT_BOX_HIT_MESSAGE && CtrPokeTouch_BoxHit(&v, 300, 230) == PT_BOX_HIT_MESSAGE);
    assert(!CtrPokeTouch_BackHit(&v, kBoxBack.x + 30, kBoxBack.y + 28));
    /* MOVE ITEMS: a tag on a Pokemon that holds an item, the item in hand over the cursor, the item's lines */
    b->state = PT_BOX_STATE_BOX; memset(b->message, 0xff, sizeof(b->message));
    b->items = 1; b->slotItem[0] = 1; b->handItem = 2; b->monItem = 1; b->pos = 7;
    BoxText(b->itemName, sizeof(b->itemName), "POTION"); BoxText(b->itemDesc, sizeof(b->itemDesc), "HEALS");
    update(&old, &v);
    assert(ColorIn(BoxCell(7).x, BoxCell(7).y - 12, 24, 24, GbaColor(31 << 10)) > 0);
    b->items = 0; b->handItem = b->monItem = 0; b->slotItem[0] = 0; update(&old, &v);
    /* no wallpaper: the box's window white; BUSY */
    wallOk = 0; b->wallpaper = 3; update(&old, &v);    /* another wallpaper, which the pack lacks */
    b->state = PT_BOX_STATE_BUSY; update(&old, &v);
    v.frameType = 1; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    wallOk = monIconOk = 0; sFXWallId = -1;
    CtrPokeTouch_Draw(canvas, base);
}

/* A choice of the field as a page: FireRed's menu window at the bottom right as wide as its rows need, the question in
 * the band beside it, the arrow on the cursor or the row held, a tap off the window its B; a grid across the page. */
static void choice_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchChoice *c = &v.choice;
    int width;

    v.home = 0; v.backVisible = 1; v.backEnabled = 1;
    memset(c, 0, sizeof(*c));
    c->on = 1; c->count = 3; c->cols = 0; c->cursor = 1; c->pressed = -1;
    snprintf(c->label[0], sizeof(c->label[0]), "YES"); snprintf(c->label[1], sizeof(c->label[1]), "NO");
    snprintf(c->label[2], sizeof(c->label[2]), "INFO");
    snprintf(c->prompt, sizeof(c->prompt), "Would you like to battle?");
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    width = 0;
    {
        const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
        CtrPokeTouchRect w = ChoiceWindow(3, 0, width), n = ChoiceNote(3, 0, width);

        assert(CtrPokeTouch_FieldPageShown(&v));
        PageMargin();
        assert(!outline || at(w.x, w.y + 20) == KitInk(2));
        for (int i = 0; i < 3; ++i) {
            CtrPokeTouchRect r = ChoiceRow(3, 0, width, i);

            assert(CtrPokeTouch_ChoiceHit(&v, r.x + r.w / 2, r.y + 8) == PT_CHOICE_HIT_ROW + i);
            assert(ArrowAt(&r) == (i == 1));
        }
        /* a tap off the window is the menu's B; the window's frame nothing; no BACK cell */
        assert(CtrPokeTouch_ChoiceHit(&v, 40, 40) == PT_CHOICE_HIT_BACK);
        assert(CtrPokeTouch_ChoiceHit(&v, n.x + 30, n.y + 20) == PT_CHOICE_HIT_BACK);
        assert(CtrPokeTouch_ChoiceHit(&v, w.x + 2, w.y + 2) == -1);
        assert(!CtrPokeTouch_BackHit(&v, 290, 220) && !CtrPokeTouch_BackHit(&v, 40, 40));
    }
    c->pressed = PT_CHOICE_HIT_ROW + 2; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){ChoiceRow(3, 0, 0, 2).x, ChoiceRow(3, 0, 0, 2).y, 8, 16}));
    c->pressed = -1; c->cursor = 0; update(&old, &v);
    /* a long row widens the window, the band narrower */
    snprintf(c->label[2], sizeof(c->label[2]), "WWWWWWWWWWWWWWWWWWWWWWW"); update(&old, &v);
    width = UiTextWidth(c->label[2], 1) + KIT_ARROW_W + 16;
    assert(CtrPokeTouch_ChoiceHit(&v, ChoiceRow(3, 0, width, 2).x + 4, ChoiceRow(3, 0, width, 2).y + 8) == PT_CHOICE_HIT_ROW + 2);
    /* eight rows; a grid */
    c->count = 8;
    for (int i = 3; i < 8; ++i)
        snprintf(c->label[i], sizeof(c->label[i]), "ROW %d", i);
    snprintf(c->label[2], sizeof(c->label[2]), "INFO"); update(&old, &v);
    c->cols = 2; c->count = 5; update(&old, &v);
    for (int i = 0; i < 5; ++i) {
        CtrPokeTouchRect r = ChoiceRow(5, 2, 0, i);

        assert(CtrPokeTouch_ChoiceHit(&v, r.x + r.w / 2, r.y + 8) == PT_CHOICE_HIT_ROW + i);
    }
    assert(CtrPokeTouch_ChoiceHit(&v, 40, 200) == PT_CHOICE_HIT_BACK);
    c->pressed = PT_CHOICE_HIT_BACK; update(&old, &v);
    c->pressed = -1; v.frameType = 8; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    CtrPokeTouch_Draw(canvas, base);
}

/* Forgetting a move as a page: FireRed's question, the Pokemon, its four moves and the new one as rows (their types),
 * the arrow on the Summary's cursor or the row held, the chosen move's numbers and description; BACK gives up. */
static void learn_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchLearn *l = &v.learn;

    v.home = 0; v.backVisible = 1; v.backEnabled = 1; v.backPressed = 0;
    memset(l, 0, sizeof(*l));
    l->on = 1; l->cursor = 1; l->pressed = -1; l->species = 25;
    snprintf(l->nickname, sizeof(l->nickname), "PIKACHU"); snprintf(l->level, sizeof(l->level), "Lv12");
    snprintf(l->question, sizeof(l->question), "Which move should be forgotten?");
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        l->row[i].move = (uint16_t)(i + 1); l->row[i].type = (int8_t)(i == 4 ? 12 : 0);
        snprintf(l->row[i].name, sizeof(l->row[i].name), "MOVE %d", i);
        snprintf(l->row[i].pp, sizeof(l->row[i].pp), "10/10");
        snprintf(l->row[i].power, sizeof(l->row[i].power), "40");
        snprintf(l->row[i].accuracy, sizeof(l->row[i].accuracy), "100");
        snprintf(l->row[i].typeName, sizeof(l->row[i].typeName), "TYPE");
    }
    l->row[2].locked = 1;
    snprintf(l->desc, sizeof(l->desc), "A move that may cause paralysis.");
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(CtrPokeTouch_FieldPageShown(&v));
    PageMargin();
    assert(!outline || (at(kLearnTitle.x, kLearnTitle.y + 15) == KitInk(2) && at(kSumMon.x, kSumMon.y + 20) == KitInk(2)
                        && at(kSumPage.x, kSumPage.y + 20) == KitInk(2) && at(kLearnBack.x, kLearnBack.y + 20) == KitInk(2)));
    for (int i = 0; i < PT_LEARN_ROWS; ++i) {
        CtrPokeTouchRect r = SumMoveRow(i);

        assert(CtrPokeTouch_LearnHit(&v, r.x + r.w / 2, r.y + 8) == PT_LEARN_HIT_ROW + i);
        assert(ArrowAt(&r) == (i == 1));
    }
    assert(ColorIn(kSumPage.x, kSumPage.y, kSumPage.w, kSumPage.h, UiTypeColor(12)) > 0);   /* the new move's type */
    assert(CtrPokeTouch_LearnHit(&v, kSumMon.x + 50, kSumMon.y + 50) == -1);
    assert(CtrPokeTouch_LearnHit(&v, kLearnBack.x + 30, kLearnBack.y + 20) == -1);
    assert(CtrPokeTouch_BackHit(&v, kLearnBack.x + 30, kLearnBack.y + 20) && !CtrPokeTouch_BackHit(&v, kLearnNote.x + 30, kLearnNote.y + 20));
    /* a row held: the arrow with it and the grey; the cursor on the new move; an empty slot is no hit */
    l->pressed = PT_LEARN_HIT_ROW + 3; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){SumMoveRow(3).x, SumMoveRow(3).y, 8, 16}) && !ArrowAt(&(CtrPokeTouchRect){SumMoveRow(1).x, SumMoveRow(1).y, 8, 16}));
    assert(at(SumMoveRow(3).x + SumMoveRow(3).w - 60, SumMoveRow(3).y + 1) == KitInk(3));
    l->pressed = -1; l->cursor = 4; update(&old, &v);
    l->row[3].move = 0; update(&old, &v);
    assert(CtrPokeTouch_LearnHit(&v, SumMoveRow(3).x + 40, SumMoveRow(3).y + 8) == -1);
    v.backPressed = 1; update(&old, &v);
    v.backPressed = 0; v.frameType = 9; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    CtrPokeTouch_Draw(canvas, base);
}

/* The idle screen: the page's colour; with the logo, the logo in its panel (the dialogue frame) in the middle, the
 * page's colour round it. */
static void idle_page(const CtrPokeTouchView *base)
{
    int drawn = 0;

    CtrPokeTouch_DrawIdle(canvas, 0);
    for (int x = 0; x < W; ++x)
        for (int y = 0; y < H; ++y)
            assert(at(x, y) == KitInk(3));
    CtrPokeTouch_DrawIdle(canvas, 1);
    PageMargin();
    for (int x = 0; x < kIdlePanel.x; ++x)
        assert(at(x, H / 2) == KitInk(3) && at(W - 1 - x, H / 2) == KitInk(3));
    for (int x = kIdlePanel.x + 16; x < kIdlePanel.x + kIdlePanel.w - 16; ++x)
        for (int y = kIdlePanel.y + 8; y < kIdlePanel.y + kIdlePanel.h - 8; ++y)
            drawn += at(x, y) != KitInk(1) && at(x, y) != KitInk(3);
    assert(drawn > 1000);                                      /* the logo */
    assert(at(kIdlePanel.x + 40, kIdlePanel.y + 8) == KitInk(1));    /* the panel's white, over the logo's first row */
    CtrPokeTouch_Draw(canvas, base);
}

/* FireRed's own picture as a page (the Fame Checker, mail, the Pokedex's AREA...): the frame's window round the picture
 * in the middle of the page (the picture's place black under the GPU's picture), the band with what the screen is and
 * BACK, the band whole without BACK; any other view without a page of its own (nothing to show) gets the same frame. */
static void frame_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    int gx, gy;

    v.home = 0; v.viewport = 1; v.backVisible = 1; v.backEnabled = 1; v.backPressed = 0;
    snprintf(v.bar[0], sizeof(v.bar[0]), "FAME CHECKER"); snprintf(v.bar[1], sizeof(v.bar[1]), "Use the game's own controls.");
    snprintf(v.bar[2], sizeof(v.bar[2]), "BACK is the B button.");
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(CtrPokeTouch_FieldPageShown(&v) && !CtrPokeTouch_HomePageShown(&v));
    PageMargin();
    assert(!outline || (at(kViewFrame.x, kViewFrame.y + 20) == KitInk(2) && at(kViewBack.x, kViewBack.y + 20) == KitInk(2)));
    assert(at(PT_VIEW_X, PT_VIEW_Y) == 0 && at(PT_VIEW_X + PT_VIEW_W - 1, PT_VIEW_Y + PT_VIEW_H - 1) == 0);
    assert(at(PT_VIEW_X - 1, PT_VIEW_Y + 20) != 0 && at(PT_VIEW_X + 20, PT_VIEW_Y + PT_VIEW_H) != 0);
    assert(CtrPokeTouch_ViewportToGba(PT_VIEW_X, PT_VIEW_Y, &gx, &gy) && gx == 0 && gy == 0);
    assert(CtrPokeTouch_BackHit(&v, kViewBack.x + 30, kViewBack.y + 24) && !CtrPokeTouch_BackHit(&v, kViewNote.x + 30, kViewNote.y + 24));
    assert(!CtrPokeTouch_BackHit(&v, PT_VIEW_X + 100, PT_VIEW_Y + 80));
    v.backPressed = 1; update(&old, &v);
    assert(at(kViewBack.x + kViewBack.w - 10, kViewBack.y + 10) == KitInk(3));
    v.backPressed = 0; v.backEnabled = 0; update(&old, &v);
    /* no BACK: the band whole */
    v.backVisible = 0; update(&old, &v);
    assert(!CtrPokeTouch_BackHit(&v, kViewBack.x + 30, kViewBack.y + 24) && at(kViewBack.x + 30, kViewBack.y + 24) != KitInk(3));
    snprintf(v.bar[0], sizeof(v.bar[0]), "POKeMON STORAGE SYSTEM"); update(&old, &v);
    /* a view with nothing of its own to show: the frame's window empty */
    v.viewport = 0; update(&old, &v);
    assert(at(PT_VIEW_X + 10, PT_VIEW_Y + 10) == KitInk(1));
    v.frameType = 3; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    CtrPokeTouch_Draw(canvas, base);
}

/* The naming keyboard as a page: the name's window, the three tabs (capitals, small letters, symbols, left to right;
 * the page shown underlined), the page's keys, DELETE and OK; FireRed's arrow on its cursor (a key, the tab its page
 * button turns to, DELETE or OK) or what is held. No BACK: DELETE is its B. */
static void name_page(const CtrPokeTouchView *base)
{
    CtrPokeTouchView v = *base, old;
    CtrPokeTouchName *n = &v.name;
    static const int slotPage[3] = {1, 2, 0};

    v.home = 0; v.backVisible = 0;
    memset(n, 0, sizeof(*n));
    n->on = 1; n->page = 1; n->cols = 8; n->maxChars = 10; n->len = 3; n->curCol = 2; n->curRow = 1; n->pressed = -1;
    n->gender = 1; n->species = 25;
    memset(n->text, 0xff, sizeof(n->text));
    n->text[0] = 0xBB; n->text[1] = 0xBC; n->text[2] = 0xBD;
    for (int r = 0; r < PT_NAME_ROWS; ++r)
        for (int c = 0; c < PT_NAME_COLS; ++c)
            n->key[r][c] = (uint8_t)(r == 3 && c == 7 ? 0xff : r == 3 && c == 6 ? 0 : 0xBB + r * 8 + c);
    memset(n->title, 0xff, sizeof(n->title));
    n->title[0] = 0xD3; n->title[1] = 0xC9;
    CtrPokeTouch_Draw(canvas, &v);
    old = v;
    const int outline = !KitFrameReady(KIT_FRAME_STD, v.frameType);
    assert(CtrPokeTouch_FieldPageShown(&v));
    PageMargin();
    assert(!outline || (at(kNameEntry.x, kNameEntry.y + 20) == KitInk(2) && at(kNamePages.x, kNamePages.y + 20) == KitInk(2)
                        && at(kNameKeys.x, kNameKeys.y + 20) == KitInk(2) && at(kNameOk.x, kNameOk.y + 20) == KitInk(2)));
    for (int s = 0; s < 3; ++s) {
        CtrPokeTouchRect t = NameTab(s);

        assert(CtrPokeTouch_NameHit(&v, t.x + 16, t.y + 16) == PT_NAME_HIT_TAB + slotPage[s]);
        assert((BagInk(t.x + 8, t.y + t.h - 6, t.w - 8, 3) > 0) == (s == 0));     /* the page shown underlined */
    }
    assert(CtrPokeTouch_NameHit(&v, kNameDelete.x + 28, kNameDelete.y + 28) == PT_NAME_HIT_DELETE);
    assert(CtrPokeTouch_NameHit(&v, kNameOk.x + 28, kNameOk.y + 28) == PT_NAME_HIT_OK);
    assert(CtrPokeTouch_NameHit(&v, kNameNote.x + 28, kNameNote.y + 28) == -1);
    for (int r = 0; r < PT_NAME_ROWS; ++r)
        for (int c = 0; c < 8; ++c) {
            CtrPokeTouchRect k = NameKey(8, r, c);

            assert(CtrPokeTouch_NameHit(&v, k.x + k.w / 2, k.y + k.h / 2) == (r == 3 && c == 7 ? -1 : PT_NAME_HIT_KEY + r * PT_NAME_COLS + c));
            assert(ArrowAt(&k) == (r == 1 && c == 2));
        }
    assert(!CtrPokeTouch_BackHit(&v, kNameOk.x + 28, kNameOk.y + 28) && !CtrPokeTouch_BackHit(&v, 40, 40));
    /* a key held: its grey and the arrow; the cursor on FireRed's buttons: OK, DELETE, the tab the page button turns to */
    n->pressed = PT_NAME_HIT_KEY + 2 * PT_NAME_COLS + 5; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){NameKey(8, 2, 5).x, NameKey(8, 2, 5).y, 8, NAME_KEY_H}));
    assert(at(NameKey(8, 2, 5).x + NameKey(8, 2, 5).w - 3, NameKey(8, 2, 5).y + 2) == KitInk(3));
    n->pressed = -1; n->curCol = 8; n->curRow = 2; update(&old, &v);
    assert(BagInk(kNameOk.x + 8, kNameOk.y + 8, kNameOk.w - 16, kNameOk.h - 16) > 20);
    n->curRow = 1; update(&old, &v);
    assert(BagInk(kNameDelete.x + 8, kNameDelete.y + 8, kNameDelete.w - 16, kNameDelete.h - 16) > 20);
    n->curRow = 0; update(&old, &v);
    assert(ArrowAt(&(CtrPokeTouchRect){NameTab(1).x, NameTab(1).y, 8, 32}));
    n->pressed = PT_NAME_HIT_TAB; update(&old, &v);
    n->pressed = PT_NAME_HIT_OK; update(&old, &v);
    /* the symbols' page, six across; a full name, a female sign; no Pokemon */
    n->pressed = -1; n->page = 0; n->cols = 6; n->curCol = 0; n->curRow = 0; update(&old, &v);
    assert(CtrPokeTouch_NameHit(&v, NameKey(6, 0, 5).x + 10, NameKey(6, 0, 5).y + 10) == PT_NAME_HIT_KEY + 5);
    n->len = 10; n->gender = 2; n->species = 0; update(&old, &v);
    v.frameType = 4; update(&old, &v);
    v.frameType = base->frameType; update(&old, &v);
    CtrPokeTouch_Draw(canvas, base);
}
#endif

int main(int argc, char **argv)
{
    decoder(); fixtureFail=1; /* Exercise original fallbacks for the UI. */
#ifndef PT_BASELINE
    const char *packPath=getenv("FIRERED_TEST_PAK");
    if(packPath) {
        realPack=fopen(packPath,"rb"); assert(realPack);
        uint8_t header[64]; assert(fread(header,1,64,realPack)==64);
        static const uint8_t romHash[20]={0x41,0xcb,0x23,0xd8,0xdc,0xcc,0x8e,0xbd,0x7c,0x64,
            0x9c,0xd8,0xfb,0xb5,0x8e,0xea,0xce,0x6e,0x2f,0xdc};
        assert(CtrPak_ParseHeader(header,0x8f71cf3a,romHash,&realHeader)==CTR_PAK_OK);
        size_t size=realHeader.entryCount*40;
        uint8_t *index=malloc(size); realIndex=calloc(realHeader.entryCount,sizeof(*realIndex));
        assert(index && realIndex && fread(index,1,size,realPack)==size);
        assert(CtrPak_ParseIndex(index,&realHeader,realIndex)==CTR_PAK_OK); free(index);
        for(unsigned i=0;i<6;++i) {
            uint8_t pixels[144]; uint16_t palette[16];
            assert(CtrPokeTouch_PackMenuIcon(i,pixels,palette));
        }
        puts("PASS six production icon decoders with real user-owned pack, header/index/payload CRCs");
    }
#endif
    for (int y=-1;y<=H;++y) for (int x=-1;x<=W;++x) {
#ifdef PT_BASELINE
        /* The old frame's START column (gone with the old shell: every view is a page). */
        static const CtrPokeTouchRect expected[PT_COUNT] = {
            {250,4,68,21},{250,29,68,21},{250,54,68,21},{250,79,68,21},
            {250,104,68,21},{250,129,68,21},{250,158,32,40},{286,158,32,40},{250,204,68,30}};
        int hit=PT_NONE;
        for(int i=0;i<PT_COUNT;++i) if(UiIn(&expected[i],x,y)) hit=i;
        assert(CtrPokeTouch_HitTest(x,y)==hit);
#endif
        int gx=-1,gy=-1;
        int inside=x>=PT_VIEW_X&&x<PT_VIEW_X+PT_VIEW_W&&y>=PT_VIEW_Y&&y<PT_VIEW_Y+PT_VIEW_H;
        assert(CtrPokeTouch_ViewportToGba(x,y,&gx,&gy)==inside);
        if(inside) assert(gx==x-PT_VIEW_X&&gy==y-PT_VIEW_Y);
    }
    CtrPokeTouchView v={0}; v.selected=v.pressed=PT_NONE;
    v.home=1; v.playerX=v.playerY=v.cursorX=v.cursorY=-1;
    v.registeredItem=1; v.bikeItem=2;
    strcpy(v.playerName,"Trades"); strcpy(v.bar[0],"TEST MAP");
    for(int i=0;i<PT_COUNT;++i) v.enabled[i]=1;
    v.speed=1; v.speedEnabled=1;
#ifndef PT_BASELINE
    ArrowGlyph();
    if (getenv("PT_SHOTS")) {
        const char *fp = getenv("PT_SHOTS_FONT");

        if (fp) { FILE *ff = fopen(fp, "rb"); assert(ff); assert(fread(font, 1, sizeof(font), ff) == sizeof(font)); fclose(ff); }
        memset(sPtFontReady, 0, sizeof(sPtFontReady));
        preview_pages(&v);
        return 0;
    }
#endif
    CtrPokeTouch_Draw(canvas,&v);
#ifndef PT_BASELINE
    home_page(&v);
    home_frame_change(&v);
    bag_page(&v);
    dex_page(&v);
    options_page(&v);
    save_page(&v);
    card_page(&v);
    box_page(&v);
    choice_page(&v);
    learn_page(&v);
    name_page(&v);
    idle_page(&v);
    frame_page(&v);
#else
    for(int x=4;x<244;++x) for(int y=4;y<164;++y) assert(at(x,y)==Gba(0x5ab3));
#endif
    if(argc>1) { /* Synthetic render for visual QA, never a ROM screenshot. */
        FILE *f=fopen(argv[1],"wb"); assert(f);
        fprintf(f,"P6\n320 240\n255\n");
        for(int y=0;y<H;++y) for(int x=0;x<W;++x) {
            uint16_t c=at(x,y); uint8_t p[3]={(c>>11)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};
            fwrite(p,1,3,f);
        }
        fclose(f);
    }
    FILE *viewports=argc>2?fopen(argv[2],"wb"):NULL;
    CtrPokeTouchView old=v;
    for(int i=0;i<PT_COUNT;++i) {
        v.pressed=i; update(&old,&v); v.pressed=PT_NONE; v.selected=i; update(&old,&v);
        v.enabled[i]=0; v.selected=PT_NONE; update(&old,&v); v.enabled[i]=1; update(&old,&v);
    }
    v.running=1; update(&old,&v); v.registeredItem=2; v.bikeItem=1; update(&old,&v);
    strcpy(v.playerName,"Alex"); update(&old,&v);
    for(int screen=0;screen<4;++screen) {
        v.home=screen==0; v.viewport=screen==1; v.options=screen==2; v.save=screen==3;
        v.optCount=2; strcpy(v.optLabel[0],"SOUND"); strcpy(v.optValue[0],"MONO");
        v.savePressed=PT_SAVE_NONE;
        update(&old,&v);
        if(viewports) for(int x=PT_VIEW_X;x<PT_VIEW_X+PT_VIEW_W;++x) for(int y=PT_VIEW_Y;y<PT_VIEW_Y+PT_VIEW_H;++y) {
            uint16_t c=at(x,y); fwrite(&c,2,1,viewports);
        }
    }
    for(int state=SAVE_CONFIRM;state<=SAVE_ERROR;++state) {
        v.saveState=state; update(&old,&v);
        v.saveSel=1; update(&old,&v); v.savePressed=PT_SAVE_FIRST; update(&old,&v);
#ifndef PT_BASELINE
        assert(CtrPokeTouch_SaveHit(state==SAVE_CONFIRM?PartyMenuRow(2,0).x+20:10,state==SAVE_CONFIRM?PartyMenuRow(2,0).y+8:10,state)==
               (state==SAVE_WRITING?PT_SAVE_NONE:PT_SAVE_FIRST));
#else
        assert(CtrPokeTouch_SaveHit(180,state==SAVE_CONFIRM?180:200,state)==
               (state==SAVE_WRITING?PT_SAVE_NONE:PT_SAVE_FIRST));
#endif
        v.savePressed=PT_SAVE_NONE; update(&old,&v);
    }
#ifndef PT_BASELINE
    /* The preview under OPTIONS' look rows: a note card in the rows the page does not use, with the look's walking picture
     * (here a pattern and a palette), its name and its artists; it follows the look chosen, and goes with the row. */
    {
        v.home=0; v.viewport=0; v.options=1; v.save=0; v.optCount=4; v.optSelected=1;
        v.optLook.on=1; v.optLook.has=1;
        strcpy(v.optLook.name,"ASH HGSS"); strcpy(v.optLook.credit,"Made by PKMNTrainerRick");
        for(int i=0;i<512;++i) v.optLook.tiles[i]=(uint8_t)(i*7);
        for(int i=1;i<16;++i) v.optLook.palette[i]=(uint16_t)(i*0x0421);
        update(&old,&v);
        {   /* For a look at the screen: PT_PREVIEW_PPM names the file to write, PT_PREVIEW_WALK a walk.bin to show, with its
             * name and artists in PT_PREVIEW_NAME and PT_PREVIEW_CREDIT. */
            const char *out=getenv("PT_PREVIEW_PPM"), *walk=getenv("PT_PREVIEW_WALK");
            if(out && walk) {
                const char *fp=getenv("PT_PREVIEW_FONT");   /* a 256x64-byte glyph file, a local viewing aid */
                if(fp) { FILE *ff=fopen(fp,"rb"); assert(ff); assert(fread(font,1,sizeof(font),ff)==sizeof(font)); fclose(ff); }
                FILE *w=fopen(walk,"rb"); assert(w);
                uint8_t entry[40+512]; assert(fread(entry,1,sizeof(entry),w)==sizeof(entry)); fclose(w);
                memcpy(v.optLook.tiles,entry+40,512);
                for(int i=0;i<16;++i) v.optLook.palette[i]=(uint16_t)(entry[8+i*2]|entry[9+i*2]<<8);
                snprintf(v.optLook.name,sizeof(v.optLook.name),"%s",getenv("PT_PREVIEW_NAME")?getenv("PT_PREVIEW_NAME"):"LOOK");
                snprintf(v.optLook.credit,sizeof(v.optLook.credit),"%s",getenv("PT_PREVIEW_CREDIT")?getenv("PT_PREVIEW_CREDIT"):"");
                strcpy(v.optLabel[0],"EXP SHARE"); strcpy(v.optValue[0],"OFF");
                strcpy(v.optLabel[1],"RED LOOK"); strcpy(v.optValue[1],v.optLook.name);
                strcpy(v.optLabel[2],"LEAF LOOK"); strcpy(v.optValue[2],"DEFAULT");
                strcpy(v.optLabel[3],"RIVAL LOOK"); strcpy(v.optValue[3],"DEFAULT");
                update(&old,&v);
                FILE *f=fopen(out,"wb"); assert(f);
                fprintf(f,"P6\n320 240\n255\n");
                for(int y=0;y<H;++y) for(int x=0;x<W;++x) {
                    uint16_t c=at(x,y); uint8_t p[3]={(uint8_t)((c>>11)*255/31),(uint8_t)(((c>>5)&63)*255/63),(uint8_t)((c&31)*255/31)};
                    fwrite(p,1,3,f);
                }
                fclose(f);
            }
        }
        strcpy(v.optLook.name,"GARY"); strcpy(v.optLook.credit,"Made by Alan_Flygon, a credit long enough to need a second and a third line of the note card");
        update(&old,&v);
        v.optLook.has=0; strcpy(v.optLook.name,"DEFAULT"); update(&old,&v);       /* the default: no picture */
        v.optCount=7; update(&old,&v);                                            /* a page of seven rows has no room: no panel */
        v.optCount=4; v.optLook.on=0; update(&old,&v);                           /* another row selected: the panel goes */
        memset(&v.optLook,0,sizeof(v.optLook)); v.optCount=2; v.optSelected=0; v.options=0; v.home=1; update(&old,&v);
    }
#endif
    v.save=0; v.home=1; v.backVisible=1; update(&old,&v);
    v.backEnabled=1; update(&old,&v); v.backPressed=1; update(&old,&v);
    if(viewports) fclose(viewports);
    if(realPack) fclose(realPack);
    free(realIndex);
    puts("PASS real renderer: decoder, 4bpp, geometry, viewport, incremental states, X/Y/RUN, save/back hit-tests");
    return 0;
}
