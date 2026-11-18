/* The letter being read as the red device (no board; plan 3), included by 3ds_poketouch.c after the Trainer Card: a row
 * of the field dispatcher (3ds_device_field_impl.h), shown while view->mail.on and there is no GBA picture
 * (DevMailShown). The upper screen keeps drawing the letter on its own stationery, as the game does
 * (3ds_poketouch.h CtrPokeTouchMail).
 *
 * The tall screen: the paper's item icon at its own size near the top, its name centred under it, a rule and who wrote
 * it (FROM and the writer, the pair centred; left out when the game does not know). A touch of the screen closes the
 * letter (CtrPokeTouch_MailHit; 3ds_bottom.c sends what BACK does); CLOSE, a dark key under it, is its BACK. Every
 * rectangle is 3ds_device_layout.h's, so what is drawn is what a touch takes.
 *
 * Words (decision 5): the paper's name (FireRed's, which v0.6.0 title-cased) through DevUpper; the port's FROM and CLOSE
 * in capitals; the writer as typed.
 *
 * Redraw: the screen whole when what it draws differs (DevMailSame), and nothing else. */

static int DevMailShown(const CtrPokeTouchView *v)
{
    return v->mail.on && !v->viewport;
}

bool CtrPokeTouch_MailHit(const CtrPokeTouchView *v, int x, int y)
{
    return DevMailShown(v) && UiIn(&kDevTallScreen, x, y);
}

/* The letter whole (after DevBegin: the dispatcher's). */
static void DevMailDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchMail *m = &v->mail;
    const CtrPokeTouchRect *n = &kDevMailName, *f = &kDevMailFrom;
    char name[sizeof(m->name)];
    int w;

    DevShell(0);
    DevScreen(&kDevTallScreen);
    if (m->item) {
        DevArt(DEV_ART_ITEM, kDevMailIcon.x, kDevMailIcon.y, kDevMailIcon.w, kDevMailIcon.h);
        DevItemIcon(m->item, kDevMailIcon.x, kDevMailIcon.y, 0);
    }
    DevUpper(name, m->name, sizeof(name));
    w = DevTextWidth(name);
    DevText(name, w < n->w ? n->x + (n->w - w) / 2 : n->x, n->y + (n->h - UI_TEXT_H) / 2, n->x + n->w);
    DevRect(kDevMailRule.x, kDevMailRule.y, kDevMailRule.w, kDevMailRule.h, DEV_INK_SH);
    if (m->from[0]) {
        const char *label = CTR_TR("FROM", "DE", "VON");
        int lw = DevSmallTextWidth(label) + 6, x;

        w = lw + DevTextWidth(m->from);
        x = w < f->w ? f->x + (f->w - w) / 2 : f->x;
        DevSmallText(label, x, f->y + (f->h - 7) / 2, DEV_INK2, f->x + f->w);
        DevText(m->from, x + lw, f->y + (f->h - UI_TEXT_H) / 2, f->x + f->w);
    }
    DevFieldBack(v, &kDevWideKey, DevCloseWord());          /* CLOSE */
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the letter draws differs between `a` and `b` (the shell is the dispatcher's). */
static int DevMailSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return a->mail.item == b->mail.item && !strcmp(a->mail.name, b->mail.name) && !strcmp(a->mail.from, b->mail.from)
        && DevFieldBackSame(a, b);
}

/* CLOSE as drawn now: the wide key while shown. */
static const CtrPokeTouchRect *DevMailBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevWideKey : NULL;
}
