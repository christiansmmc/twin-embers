/* What is left of the lower screen's ember skin once every view became a page (phase 2): the battle screen's surface
 * colours (EmSurface, 3ds_battle_ui_impl.h) and the kit's fallback ink (3ds_lower_kit_impl.h) read these. The backdrop,
 * the raised buttons, the note card, the bezel and the column's icons went with the old frame. Included by
 * 3ds_poketouch.c after its Px / Fill and the theme. */

/* The palettes of the raised pieces: the face from `top` to `bottom`, its lip and outline, and the ink of a label. */
typedef struct { uint16_t top, bottom, lip, edge, ink, inkSh; } EmPal;

#define EM_DARK UI_RGB(98, 98, 98)                  /* FireRed's text grey and its shadow, on the light pieces */
#define EM_DARK_SH UI_RGB(213, 213, 205)
#define EM_DIM UI_RGB(136, 120, 112)
#define EM_WHITE UI_RGB(255, 255, 255)
