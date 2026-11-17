/* The field dispatcher: the field's views without a board, drawn as the red device, in one table. Included last by
 * 3ds_poketouch.c (after every view's own file); CtrPokeTouch_Draw and _Update ask it after HOME and the Bag
 * (DevFieldDraw, DevFieldUpdate, declared before them), 3ds_bottom.c asks CtrPokeTouch_DeviceShown (no START column under
 * a view drawn as the device) and CtrPokeTouch_BackHit (BACK where the view draws it).
 *
 * A row: when the view is shown, how it draws whole (after DevBegin), whether two views draw the same (the fields it
 * draws, the press as drawn; the shell is the dispatcher's), and its BACK key as drawn now (NULL: none). The rows are in
 * MenuFrame's precedence: Easy Chat, the Pokedex, the card, the move to forget, the relearner, the letter, a choice, the
 * naming screen, the PC box, SAVE, OPTIONS, and the frame round FireRed's picture last; each view's task puts its row at
 * its place. A view drawn here draws whole when it changes (an Update is the full Draw), and nothing else. */

typedef struct {
    int (*shown)(const CtrPokeTouchView *);
    void (*draw)(const CtrPokeTouchView *);
    int (*same)(const CtrPokeTouchView *a, const CtrPokeTouchView *b);
    const CtrPokeTouchRect *(*back)(const CtrPokeTouchView *);
} DevFieldView;

static const DevFieldView kDevFieldViews[] = {
    {DevDexShown, DevDexDraw, DevDexSame, DevDexBackKey},           /* the Pokedex (3ds_device_dex_impl.h) */
    {DevCardShown, DevCardDraw, DevCardSame, DevCardBackKey},       /* the Trainer Card (3ds_device_card_impl.h) */
    {DevLearnShown, DevLearnDraw, DevLearnSame, DevLearnBackKey},   /* the move to forget (3ds_device_learn_impl.h) */
    {DevRelearnShown, DevRelearnDraw, DevRelearnSame, DevRelearnBackKey},   /* the relearner (_relearn_impl.h) */
    {DevMailShown, DevMailDraw, DevMailSame, DevMailBackKey},       /* the letter (3ds_device_mail_impl.h) */
    {DevChoiceShown, DevChoiceDraw, DevChoiceSame, DevChoiceBackKey},   /* a choice (3ds_device_choice_impl.h) */
    {DevNameShown, DevNameDraw, DevNameSame, DevNameBackKey},       /* the naming keyboard (3ds_device_naming_impl.h) */
    {DevBoxShown, DevBoxDraw, DevBoxSame, DevBoxBackKey},           /* the PC box (3ds_device_box_impl.h) */
    {DevSaveShown, DevSaveDraw, DevSaveSame, DevSaveBackKey},       /* SAVE (3ds_device_save_impl.h) */
    {DevOptShown, DevOptDraw, DevOptSame, DevOptBackKey},           /* OPTIONS (3ds_device_options_impl.h) */
};

/* The row that draws `v`, or -1 (an ember view, HOME, the Bag). */
static int DevFieldIndex(const CtrPokeTouchView *v)
{
    for (int i = 0; i < (int)(sizeof(kDevFieldViews) / sizeof(kDevFieldViews[0])); ++i)
        if (kDevFieldViews[i].shown(v))
            return i;
    return -1;
}

/* `v` whole when a row draws it (sCanvas is CtrPokeTouch_Draw's); 0 when none does. */
static int DevFieldDraw(const CtrPokeTouchView *v)
{
    int i = DevFieldIndex(v);

    if (i < 0)
        return 0;
    DevBegin(sCanvas, v->shell);
    kDevFieldViews[i].draw(v);
    return 1;
}

/* From `old` to `v` when either is a row's: the same row drawing the same in the same shell presents nothing, anything
 * else is drawn whole; 0 when neither is a row's. */
static int DevFieldUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)
{
    int a = DevFieldIndex(old), b = DevFieldIndex(v);

    if (a < 0 && b < 0)
        return 0;
    if (a != b || old->shell != v->shell || !kDevFieldViews[b].same(old, v)) {
        CtrPokeTouch_Draw(sCanvas, v);
        CtrDirty_Add(dirty, 0, 0, W, H);
    }
    return 1;
}

bool CtrPokeTouch_DeviceShown(const CtrPokeTouchView *v)
{
    return DevHomeShown(v) || DevBagShown(v) || DevFieldIndex(v) >= 0;
}

bool CtrPokeTouch_BackHit(const CtrPokeTouchView *v, int x, int y)
{
    int i = DevFieldIndex(v);
    const CtrPokeTouchRect *key;
    CtrPokeTouchRect area;

    if (i < 0)
        return UiIn(&gCtrPokeTouchBack, x, y);
    key = kDevFieldViews[i].back(v);
    if (!v->backVisible || !key)
        return false;
    area = DevKeyArea(key);
    return UiIn(&area, x, y);
}
