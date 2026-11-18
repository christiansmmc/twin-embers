/* ---- mail: the letter being read as the red device, through the field dispatcher (no board; plan 3 Task 2) -------------
 * Included by device_render_test.c before its kSuites. */

/* The letter as 3ds_bottom.c gives it (MailSnapshot): ORANGE MAIL (FireRed's name, its icon) from a writer as typed;
 * CLOSE shown and usable. */
static CtrPokeTouchView MailBoard(void)
{
    CtrPokeTouchView v;

    memset(&v, 0, sizeof(v));
    v.pressed = PT_NONE;
    v.mail.on = 1;
    v.mail.item = 121;
    strcpy(v.mail.name, "ORANGE MAIL");
    strcpy(v.mail.from, "Ash");
    v.backVisible = v.backEnabled = 1;
    return v;
}

/* The columns of {x, y, w, h} holding ink `c`: the first and the last (-1 none). */
static void MailInkSpan(CtrPokeTouchRect r, uint16_t c, int *first, int *last)
{
    *first = *last = -1;
    for (int x = r.x; x < r.x + r.w; ++x)
        if (Count(x, r.y, 1, r.h, c)) {
            if (*first < 0)
                *first = x;
            *last = x;
        }
}

/* The plan's suite (the letter's screen closes it, CLOSE where drawn, a letter without a writer), then the pieces (the
 * paper's icon, its name centred, the rule, who wrote it), CLOSE's states, what is not drawn, the ways in and out, upper
 * case and the states for the eye. */
static void SuiteMail(void)
{
    const uint16_t blue = GbaColor(31 << 10);              /* the synthetic icon of an item past 2 */
    CtrPokeTouchView mv = MailBoard(), mo = mv, a, b;
    int first, last, inkFirst, inkLast;

    CtrPokeTouch_Draw(canvas, &mo);
    /* the plan's suite */
    mv = MailBoard(); DevCheckTouch(&mo, &mv); assert(DrawnBy(&mv, DevMailShown));
    /* mail */ assert(CtrPokeTouch_MailHit(&mv, 160, 90) && !CtrPokeTouch_MailHit(&mv, 160, 200)); AssertHits(BackV, &mv, kDevWideKey, 1); mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv);
    mv.mapGroup ^= 1; mv.speed ^= 1; assert(DevCheckTouch(&mo, &mv).count == 0);

    /* the screen closes the letter from its first pixel to its last; CLOSE is its own */
    mv = MailBoard(); DevCheckTouch(&mo, &mv);
    assert(CtrPokeTouch_MailHit(&mv, 8, 8) && CtrPokeTouch_MailHit(&mv, 311, 179) && !CtrPokeTouch_MailHit(&mv, 7, 8)
           && !CtrPokeTouch_MailHit(&mv, 8, 180) && !BackV(&mv, 160, 90));
    /* the paper's icon at its own size, centred; the name centred; the rule; FROM and the writer, the pair centred */
    assert(Count(kDevMailIcon.x, kDevMailIcon.y, kDevMailIcon.w, kDevMailIcon.h, blue) == 24 * 24);
    MailInkSpan(kDevMailName, DEV_INK, &first, &last);
    assert(first > kDevMailName.x && abs((first - kDevMailName.x) - (kDevMailName.x + kDevMailName.w - 1 - last)) <= 2);
    assert(Count(kDevMailRule.x, kDevMailRule.y, kDevMailRule.w, kDevMailRule.h, DEV_INK_SH) == kDevMailRule.w);
    MailInkSpan(kDevMailFrom, DEV_INK2, &first, &last);
    MailInkSpan(kDevMailFrom, DEV_INK, &inkFirst, &inkLast);
    assert(first > kDevMailFrom.x && last < inkFirst && inkLast > first
           && abs((first - kDevMailFrom.x) - (kDevMailFrom.x + kDevMailFrom.w - 1 - inkLast)) <= 2);
    AssertKey(kDevWideKey, DEV_LOOK_DARK, 0); assert(Count(8, 188, 304, 42, DEV_DARK_INK) > 0);
    /* no writer: no FROM; no item: no icon */
    mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv);
    assert(Count(kDevMailFrom.x, kDevMailFrom.y, kDevMailFrom.w, kDevMailFrom.h, DEV_INK2) == 0
           && Count(kDevMailFrom.x, kDevMailFrom.y, kDevMailFrom.w, kDevMailFrom.h, DEV_INK) == 0);
    mv.mail.item = 0; assert(DevWhole(DevCheckTouch(&mo, &mv)));
    assert(Count(kDevMailIcon.x, kDevMailIcon.y, kDevMailIcon.w, kDevMailIcon.h, DEV_PAPER) == 24 * 24);
    /* CLOSE: sunk under a touch, grey while it cannot be used, gone when not shown (the screen still closes it) */
    mv = MailBoard(); DevCheckTouch(&mo, &mv);
    mv.backPressed = 1; DevCheckTouch(&mo, &mv); AssertKey(kDevWideKey, DEV_LOOK_DARK, 1);
    mv.backPressed = 0; mv.backEnabled = 0; DevCheckTouch(&mo, &mv); assert(at(11, 210) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
    mv.backEnabled = 1; mv.backVisible = 0; DevCheckTouch(&mo, &mv);
    assert(at(11, 210) == DEV_SHELL_RED && !BackV(&mv, 11, 210) && CtrPokeTouch_MailHit(&mv, 160, 90));
    /* what the letter does not draw presents nothing: the bar, HOME's keys, the map */
    mv = MailBoard(); DevCheckTouch(&mo, &mv);
    strcpy(mv.bar[0], "Anything"); mv.enabled[PT_BAG] = 1; mv.playerX = 3;
    assert(DevCheckTouch(&mo, &mv).count == 0);
    mv.shell = 1; assert(DevWhole(DevCheckTouch(&mo, &mv)) && at(1, 100) == DEV_SHELL_BLUE);
    mv.shell = 0; DevCheckTouch(&mo, &mv);
    strcpy(mv.mail.name, "RETRO MAIL"); assert(DevWhole(DevCheckTouch(&mo, &mv)));
    /* the ways in and out: the GBA picture (in the frame's screen: nothing of the letter hit, the frame's BACK) */
    mv.viewport = 1; assert(DevWhole(DevCheckTouch(&mo, &mv)) && DrawnBy(&mv, DevFrameShown));
    assert(!CtrPokeTouch_MailHit(&mv, 160, 90) && CtrPokeTouch_BackHit(&mv, 278, 209) && !CtrPokeTouch_BackHit(&mv, 180, 210));
    mv = MailBoard(); mv.mail.on = 0; assert(!CtrPokeTouch_MailHit(&mv, 160, 90));
    mv = MailBoard(); assert(DevWhole(DevCheckTouch(&mo, &mv)));
    /* upper case (owner decision 5): the paper's name, which v0.6.0 title-cased, in capitals; the writer as typed */
    a = b = MailBoard();
    strcpy(a.mail.name, "Orange Mail"); TouchSamePixels(&a, &b, 1, "the paper's name in v0.6.0's case and in capitals");
    strcpy(b.mail.from, "ASH"); TouchSamePixels(&a, &b, 0, "the writer as typed and in capitals");
    CtrPokeTouch_Draw(canvas, &mo);

    /* The states for the eye, each through Update first. */
    mv = MailBoard(); DevCheckTouch(&mo, &mv); shot("from");
    mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv); shot("bare");
    assert(sDevKeyCut == 0);                      /* main prints PASS device mail */
}
