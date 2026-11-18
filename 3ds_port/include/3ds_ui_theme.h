#ifndef CTR_UI_THEME_H
#define CTR_UI_THEME_H

#include <stdint.h>

/*
 * What the red device's views keep of the lower screen's first theme: a colour written as RGB888 and the height of
 * FireRed's capitals. Its few components are 3ds_ui_theme_impl.h; the views' own kit is 3ds_device_kit_impl.h.
 */

/* RGB888 -> RGB565, at compile time. */
#define UI_RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

enum {
    UI_TEXT_H = 9,            /* FireRed's capitals, in pixels */
};

#endif
