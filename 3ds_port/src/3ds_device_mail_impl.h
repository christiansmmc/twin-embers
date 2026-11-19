/* The letter being read as the red device (def_mail), included by 3ds_poketouch.c after the Trainer Card: a row of the
 * field dispatcher (3ds_device_field_impl.h), shown while view->mail.on and there is no GBA picture (DevMailShown). The
 * upper screen keeps drawing the letter on its own stationery, as the game does (3ds_poketouch.h CtrPokeTouchMail).
 *
 * Its screen (kDevMailScreen): the paper's item icon at its own size centred near the top, its name centred under it,
 * a rule and who wrote it ("FROM" and the writer as one line, centred; left out when the game does not know). Under it
 * the grid's note says where the letter is and how it closes (DevMailNote, small, three lines at most), and CLOSE, a
 * dark key in the exit corner, is its BACK. A touch of the screen or of the note closes the letter
 * (CtrPokeTouch_MailHit; 3ds_bottom.c sends what BACK does). Every rectangle is 3ds_device_layout.h's, so what is drawn
 * is what a touch takes.
 *
 * Words (decision 5): the paper's name (FireRed's, which v0.6.0 title-cased) through DevUpper; the port's FROM, the
 * note and CLOSE in capitals; the writer as typed.
 *
 * Redraw: the screen whole when what it draws differs (DevMailSame), and nothing else (the note is the same words
 * whatever the letter). */

static int DevMailShown(const CtrPokeTouchView *v)
{
    return v->mail.on && !v->viewport;
}

/* The note under the screen (the coordinator's ruling: the board's words, with "TAP HERE OR CLOSE"). */
static const char *DevMailNote(void)
{
    return CTR_TR("THE LETTER IS ON THE UPPER SCREEN. TAP HERE OR CLOSE.",
                  "LA CARTA EST\xC3\x81 EN LA PANTALLA DE ARRIBA. TOCA AQU\xC3\x8D O CERRAR.",
                  "DER BRIEF IST AUF DEM OBEREN BILDSCHIRM. HIER TIPPEN ODER SCHLIE\xC3\x9F" "EN.");
}

bool CtrPokeTouch_MailHit(const CtrPokeTouchView *v, int x, int y)
{
    return DevMailShown(v) && (UiIn(&kDevMailScreen, x, y) || UiIn(&kDevGridNote, x, y));
}

/* The grid's note with `note` small in the paper's ink from x 18 to 226, capitals 198 / 208 / 218 (Call 4: the third's
 * descenders end on the paper's last row, 225); a note that needs a fourth line is cut there and counted
 * (DevSmallFit). */
static void DevMailNoteDraw(const char *note)
{
    DevScreen(&kDevGridNote);
    DevSmallFit(note, 18, 18, 198, 226, 3, DEV_SMALL_LINE, DEV_INK);
}

/* `s` in the normal font across r, centred, its capitals centred down r; one wider than r is cut at its right and
 * counted in sDevKeyCut (FireRed's names and a typed writer are far narrower). */
static void DevMailLine(const CtrPokeTouchRect *r, const char *s)
{
    int w = DevTextWidth(s);

    if (w > r->w)
        ++sDevKeyCut;
    DevText(s, w < r->w ? r->x + (r->w - w) / 2 : r->x, r->y + (r->h - UI_TEXT_H) / 2, r->x + r->w);
}

/* The letter whole (after DevBegin: the dispatcher's). */
static void DevMailDraw(const CtrPokeTouchView *v)
{
    const CtrPokeTouchMail *m = &v->mail;
    char name[sizeof(m->name)], from[sizeof(m->from) + 8];

    DevShell();
    DevScreen(&kDevMailScreen);
    if (m->item) {
        DevArt(DEV_ART_ITEM, kDevMailIcon.x, kDevMailIcon.y, kDevMailIcon.w, kDevMailIcon.h);
        DevItemIcon(m->item, kDevMailIcon.x, kDevMailIcon.y, 0);
    }
    DevUpper(name, m->name, sizeof(name));
    DevMailLine(&kDevMailName, name);
    DevRect(kDevMailRule.x, kDevMailRule.y, kDevMailRule.w, kDevMailRule.h, DEV_RULE);
    if (m->from[0]) {
        snprintf(from, sizeof(from), "%s %s", CTR_TR("FROM", "DE", "VON"), m->from);
        DevMailLine(&kDevMailFrom, from);
    }
    DevMailNoteDraw(DevMailNote());
    DevFieldBack(v, &kDevGridExit, DevCloseWord());         /* CLOSE */
}

/* ---- The dispatcher's row: whether two views draw the same, where BACK is ------------------------------------------- */

/* Nothing the letter draws differs between `a` and `b` (the shell is the dispatcher's). */
static int DevMailSame(const CtrPokeTouchView *a, const CtrPokeTouchView *b)
{
    return a->mail.item == b->mail.item && !strcmp(a->mail.name, b->mail.name) && !strcmp(a->mail.from, b->mail.from)
        && DevFieldBackSame(a, b);
}

/* CLOSE as drawn now: the grid's exit while shown. */
static const CtrPokeTouchRect *DevMailBackKey(const CtrPokeTouchView *v)
{
    return v->backVisible ? &kDevGridExit : NULL;
}
