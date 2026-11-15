/* Part of 3ds_video.c (included where the top screen's overlays are drawn): the turbo's speed log and the FPS counter, with the
 * 3x5 glyph helper it uses. They use the file's statics (sStats, Blend, BlendForget, sFastForward is defined here). */
/*
 * The turbo's speed, for the log only: the top screen has no badge for it (the owner's call).
 */
static unsigned sFastForward = 1;

void CtrVideo_SetFastForward(unsigned speed)
{
    if (speed != sFastForward)
        CtrLog_Write(CTR_LOG_VIDEO, "fast-forward %ux at frame %lu", speed, (unsigned long)sStats.frames);
    sFastForward = speed;
}

/* A 3x5 glyph: one rectangle per run of pixels in a row, not one per pixel (same picture). */
static void DrawGlyphRuns(const char *b, float x, float y, float p)
{
    for (unsigned r = 0, c; r < 5; ++r)
        for (c = 0; c < 3; ) {
            unsigned n = 0;
            while (c + n < 3 && b[r * 3 + c + n] == '1') ++n;
            if (n) C2D_DrawRectSolid(x + c * p, y + r * p, 0, n * p, p, C2D_Color32(255, 255, 255, 255));
            c += n ? n : 1;
        }
}

#ifndef CTR_SHOW_FPS
#define CTR_SHOW_FPS 1
#endif
#if CTR_SHOW_FPS
/*
 * The FPS counter (SHOW_FPS=1): a 3x5 pixel font, each pixel a 1x1 solid
 * rectangle, over a translucent box in the top-right corner (the 2 pixel margin
 * it had on the left, from the box's own width). Drawn last, over whatever the
 * frame composed, and at zero parallax in both eyes. The PokeTouch option turns
 * it off: it is then not drawn at all.
 */
static bool sFpsCounter = true;

void CtrVideo_SetFpsCounter(bool on)
{
    sFpsCounter = on;
}
#define FPS_PIXEL 1.0f
#define FPS_ADVANCE (4 * FPS_PIXEL)

static const char *const sFpsGlyphs[] =
{
    "111101101101111", "010110010010111", "111001111100111", "111001111001111",
    "101101111001001", "111100111001111", "111100111101111", "111001001001001",
    "111101111101111", "111101111001111",
    "111100111100100", /* F */
    "111101111100100", /* P */
};

static void DrawFps(C3D_RenderTarget *target)
{
    unsigned fps = (unsigned)(sStats.fps + 0.5f);
    unsigned glyphs[8], count = 0;
    char digits[4];
    int length = snprintf(digits, sizeof(digits), "%u", fps > 999 ? 999 : fps);

    glyphs[count++] = 10;   /* F */
    glyphs[count++] = 11;   /* P */
    glyphs[count++] = 5;    /* S, which is a 5 */
    glyphs[count++] = ~0u;  /* space */
    for (int i = 0; i < length; ++i)
        glyphs[count++] = (unsigned)(digits[i] - '0');

    /* Whatever path composed the frame, the counter draws with the 2D
     * program and no depth test, as RenderVoxel's UI pass does. */
    C2D_Prepare();
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    BlendForget();
    C2D_SceneBegin(target);
    C2D_ViewReset();
    Blend(5, false, false);
    float boxWidth = count * FPS_ADVANCE + FPS_PIXEL * 3, margin = 2.0f;
    float origin = CTR_GAME_WIDTH - margin - boxWidth;
    C2D_DrawRectSolid(origin, 2, 0, boxWidth, 5 * FPS_PIXEL + FPS_PIXEL * 4,
                      C2D_Color32(0, 0, 0, 160));
    for (unsigned i = 0; i < count; ++i)
    {
        if (glyphs[i] == ~0u) continue;
        const char *bits = sFpsGlyphs[glyphs[i]];
        DrawGlyphRuns(bits, origin + FPS_PIXEL * 2 + i * FPS_ADVANCE, 2 + FPS_PIXEL * 2, FPS_PIXEL);
    }
    C2D_Flush();
}
#else
void CtrVideo_SetFpsCounter(bool on)
{
    (void)on;
}
#endif
