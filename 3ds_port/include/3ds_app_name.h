#ifndef GUARD_3DS_APP_NAME_H
#define GUARD_3DS_APP_NAME_H

/* This fork's name on the SD card and on screen, in one place. APP_NAME and APP_TITLE in the Makefile must match.
 * The original Twin Embers keeps sdmc:/3ds/twinembers/, so both games, packs, saves and settings can live on one
 * card side by side. */
#define CTR_APP_NAME "classicfirered3ds"
#define CTR_APP_TITLE "Classic FireRed 3DS"
#define CTR_APP_DIR "sdmc:/3ds/" CTR_APP_NAME
#define CTR_APP_FILE(name) CTR_APP_DIR "/" name

/* The original game's save, copied once on the first start while this fork has none of its own. The copy is
 * validated before it is written and the original is left in place. */
#define CTR_UPSTREAM_SAVE_PATH "sdmc:/3ds/twinembers/twinembers.sav"

#endif
