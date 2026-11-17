#ifndef CTR_POKETOUCH_H
#define CTR_POKETOUCH_H

#include <stdbool.h>
#include <stdint.h>
#include "3ds_dirty_rect.h"
#include "3ds_summary.h"

/*
 * PokeTouch: the bottom screen (320x240). A view over FireRed's own menu
 * and shortcuts: every button maps to the game's START menu entry or key
 * item use (3ds_bottom.c issues them); this module only lays out, draws and
 * hit-tests. The frame on the left holds the viewport, where the GBA picture
 * of the open menu is shown 1:1 (240x160) by the GPU, and the contextual bar
 * under it. Drawing goes into the bottom canvas (RGB565,
 * column-major, each column bottom-to-top) when the view changes.
 */
enum {
    PT_POKEDEX, PT_POKEMON, PT_BAG, PT_CARD, PT_SAVE, PT_OPTIONS, /* side column (the map is HOME, no button) */
    PT_REGISTERED,  /* shortcut 1, X: the registered item (SELECT) */
    PT_BIKE,        /* shortcut 2, Y: the Bicycle */
    PT_RUN,         /* Running Shoes: the running mode on and off (B runs as ever) */
    PT_COUNT,
    PT_CLOSE = PT_COUNT, /* START menu exit pick for the bridge; not a button */
    PT_FLY,              /* the field command that Flies to the selected map section; not a button */
    PT_NONE = 0xff
};

/* HOME's own touch places beyond the keys (CtrPokeTouch_HomeHit, a view's `pressed`): the game's speed (a tap is a
 * START press for the turbo) and a Pokemon of the party (PT_HOME_PARTY + its slot, a tap opens POKeMON). Never START
 * menu entries or field commands. */
enum { PT_HOME_SPEED = 0x40, PT_HOME_PARTY = 0x48 };

/* What Fly does at a map section (CtrPokeTouch_FlyStatus): not a destination, or
 * one, and why it cannot be used (unvisited, no party member that knows Fly, no
 * badge for it, not outdoors, the field busy), or can. */
enum { PT_FLY_NONE, PT_FLY_UNVISITED, PT_FLY_NO_MON, PT_FLY_NO_BADGE, PT_FLY_NO_PLACE, PT_FLY_BUSY, PT_FLY_OK };

typedef struct { int16_t x, y, w, h; } CtrPokeTouchRect;

/* The viewport: the GBA picture's 240x160, pixel for pixel. */
#define PT_VIEW_X 4
#define PT_VIEW_Y 4
#define PT_VIEW_W 240
#define PT_VIEW_H 160
/* The HOME map: FireRed's own Region Map art, drawn passively over the field's
 * live state (CtrPokeTouch_Map* in the game, patch 0064). The maps are its
 * REGIONMAP_* values. */
enum { PT_MAP_KANTO, PT_MAP_SEVII123, PT_MAP_SEVII45, PT_MAP_SEVII67, PT_MAP_COUNT };
/* HOME's map window (the red device, 3ds_device_home_impl.h): the Region Map's own picture at 1:1, its 192x128 from
 * (PT_HOME_SRC_X, PT_HOME_SRC_Y) of the game's 240x160 screen, at (PT_HOME_X, PT_HOME_Y) of the lower screen. That crop
 * holds the whole 22x15 grid of 8x8 cells (screen x 32..207, y 32..151) with room for a 16x16 icon on every cell, so
 * every town and the player are in it on every map. (PT_VIEW_* stay the GBA picture's.) */
#define PT_HOME_X 14
#define PT_HOME_Y 14
#define PT_HOME_W 192
#define PT_HOME_H 128
#define PT_HOME_SRC_X 24
#define PT_HOME_SRC_Y 28
bool CtrPokeTouch_MapPrepare(int group, int version);                  /* the window's picture of a map; false: no art */
bool CtrPokeTouch_HomeRibbonHit(int x, int y);                         /* the region key, when it leads to another map */
/* The native OPTIONS screen (3ds_poketouch_options.c holds the rows; the red device draws it,
 * 3ds_device_options_impl.h): up to PT_OPT_MAX rows, each a label on the left and, on the right,
 * ◀ value ▶ (3ds_device_layout.h DevOptRow). On a row, the label selects it (its description goes to
 * the small note), ◀ steps the value back and the value or ▶ steps it forward; the ◀ ▶ keys under
 * the rows step the row described. A row of levels (COMPANION's OFF / FOLLOW / BE IT) shows all its
 * values side by side instead, the chosen one boxed: a touch on one picks it (zone PT_OPT_ZONE_SEG +
 * its index). The keys (PT_OPT_KEY_*): a section's (its first page) across the top, ▲ ▼ (the page
 * before, after). */
#define PT_OPT_MAX 7
#define PT_OPT_LABEL_LEN 16
#define PT_OPT_VALUE_LEN 12
#define PT_OPT_SEGS 3
#define PT_OPT_SEG_LEN 8
#define PT_OPT_SECTIONS 4
#define PT_OPT_SECTION_LEN 12
enum { PT_OPT_ZONE_NONE, PT_OPT_ZONE_LABEL, PT_OPT_ZONE_PREV, PT_OPT_ZONE_NEXT, PT_OPT_ZONE_SEG };
enum { PT_OPT_KEY_SECTION = 0x10, PT_OPT_KEY_UP = 0x18, PT_OPT_KEY_DOWN = 0x19 };
/* FireRed's own options (the game bridge, ctr_poketouch_options.c), in the order of its menu. */
enum { PT_OPT_TEXT_SPEED, PT_OPT_BATTLE_SCENE, PT_OPT_BATTLE_STYLE, PT_OPT_SOUND, PT_OPT_BUTTON_MODE, PT_OPT_FRAME,
       PT_OPT_FPS, PT_OPT_GAME_COUNT };
/* The preview OPTIONS shows under the look rows (RED LOOK, LEAF LOOK, RIVAL LOOK) while one is selected: the look chosen for it,
 * its name and its artists, and its walking picture facing down (32x32 at 4 bits, 4x4 tiles of 32 bytes, row by row of tiles)
 * drawn twice as large. `has` is 0 when the choice is the default (no picture, the name says so) or the pack has no picture. */
#define PT_LOOK_NAME_LEN 12
#define PT_LOOK_CREDIT_LEN 128
typedef struct {
    uint8_t on;                     /* a look row is selected: the panel under the rows is drawn */
    uint8_t has;
    char name[PT_LOOK_NAME_LEN];
    char credit[PT_LOOK_CREDIT_LEN];
    uint8_t tiles[512];
    uint16_t palette[16];           /* 15-bit colours, 0 clear */
} CtrPokeTouchLook;
/* The SAVE screen (the facts come from FireRed's save window, ctr_poketouch.h CtrSave_*), the red device's
 * (3ds_device_save_impl.h): the game's save information on the screen, the dialogue (bar[]) on the note under
 * it and beside the note YES / NO (SAVE_CONFIRM), nothing (SAVE_WRITING) or OK (SAVE_SUCCESS, SAVE_ERROR). */
enum { SAVE_CONFIRM, SAVE_WRITING, SAVE_SUCCESS, SAVE_ERROR };
enum { PT_SAVE_NONE, PT_SAVE_FIRST, PT_SAVE_SECOND };  /* the button: YES or OK, NO */
/* Contextual bar text: three lines of FireRed's font inside 228 pixels. */
#define PT_BAR_LINES 3
#define PT_BAR_LEN 40

/* The field Bag drawn by the port as the red device (3ds_device_bag_impl.h), a remote for FireRed's own Bag: its
 * pockets as keys, the rows of the list it shows (its items; FireRed's CLOSE BAG row is the CLOSE key), the item's
 * description, and the menu, the quantity box, the question or the message it has open. Everything is the Bag's own
 * state. The TM Case and the Berry Pouch it opens, a mart's BUY, the item PC's WITHDRAW ITEM and the mailbox are drawn
 * the same way, with their name or title in place of the pockets. */
#define PT_BAG_ROWS 8
#define PT_BAG_MENU_MAX 6
enum { PT_BAG_KIND_BAG = 0, PT_BAG_KIND_TMCASE, PT_BAG_KIND_POUCH, PT_BAG_KIND_SHOP,   /* SHOP: a mart's BUY list */
       PT_BAG_KIND_ITEMPC };                                                          /* ITEMPC: the PC's WITHDRAW ITEM */
enum { PT_BAG_USE_FIELD = 0, PT_BAG_USE_SELL, PT_BAG_USE_DEPOSIT };   /* the field's Bag, a shop's, the item PC's */
enum { PT_BAG_STATE_BUSY = 0, PT_BAG_STATE_LIST, PT_BAG_STATE_ACTIONS, PT_BAG_STATE_YESNO, PT_BAG_STATE_QUANTITY,
       PT_BAG_STATE_MESSAGE };
/* Hit codes (CtrPokeTouch_BagHit). */
enum { PT_BAG_HIT_TAB = 100, PT_BAG_HIT_ROW = 200, PT_BAG_HIT_MENU = 300, PT_BAG_HIT_MESSAGE = 500, PT_BAG_HIT_UP = 600,
       PT_BAG_HIT_DOWN, PT_BAG_HIT_QTY_UP = 610, PT_BAG_HIT_QTY_DOWN, PT_BAG_HIT_QTY_OK, PT_BAG_HIT_QTY_CANCEL,
       PT_BAG_HIT_CLOSE = 630 };
typedef struct {
    uint8_t on;                     /* the viewport shows this Bag (no GBA picture) */
    uint8_t kind;                   /* PT_BAG_KIND_*: the Bag, or a case it opens */
    uint16_t regX, regY;            /* the items on the X and Y buttons (0 none), tagged on their key-item rows */
    uint16_t titleItem;             /* a case: its own item (the title's icon), its name and its CLOSE row */
    char title[20];
    char closeName[16];
    uint8_t asked;                  /* the pocket a touch on a tab asked for, 0xFF none (the Bag is being walked to it) */
    uint8_t pocket;                 /* FireRed's pocket: 0 Items, 1 Key Items, 2 Poke Balls */
    uint8_t state;                  /* PT_BAG_STATE_*: the Bag's own input state */
    int16_t count;                  /* items in the pocket; FireRed's row `count` is CLOSE BAG (the CLOSE key) */
    int16_t cursor;                 /* the Bag's selection (count: CLOSE BAG) */
    int16_t first;                  /* the list index of the first row shown */
    struct {
        uint16_t item;
        uint16_t qty;
        uint16_t price;             /* a mart's row: what one costs */
        char name[20];
    } row[PT_BAG_ROWS];
    uint8_t typed;                  /* the rows' and the selection's names are ones the player typed (the mailbox's writers) */
    uint16_t selItem;               /* the selected item, its name and description */
    char selName[20];
    char selDesc[112];
    int8_t moveType;                /* a TM/HM: the move's type (-1: none) and its name */
    char typeName[10];
    uint8_t menuCount;              /* the open menu's rows (USE, GIVE, TOSS ... CANCEL) */
    int8_t menuCursor;
    char menuName[PT_BAG_MENU_MAX][14];
    uint16_t tossCount, tossMax;    /* the quantity box: how many, of how many */
    uint8_t use;                    /* PT_BAG_USE_*: what the Bag is open for */
    uint32_t sale, money;           /* selling or buying: what that many come to, and the player's money */
    uint8_t showOwned;              /* a mart's selected item: how many the Bag holds (owned), shown */
    uint16_t owned;
    char prompt[96];                /* the Bag's question (YES/NO) or the message it waits on */
    int16_t pressed;                /* the hit code under a held touch, or -1 */
} CtrPokeTouchBag;

/* The Pokedex drawn by the port (3ds_device_dex_impl.h), a remote for FireRed's own Pokedex like the Bag: its top menu
 * (lists, habitats, search) as tabs over its rows' keys, an ordered list as rows, a habitat page as its Pokemon, and a
 * Pokemon's page. Its AREA page stays FireRed's picture in the viewport. */
#define PT_DEX_MENU_MAX 24
#define PT_DEX_ROWS 5
enum { PT_DEX_STATE_BUSY = 0, PT_DEX_STATE_MENU, PT_DEX_STATE_LIST, PT_DEX_STATE_CATEGORY, PT_DEX_STATE_PAGE };
/* Menu ids as FireRed's (a habitat 0-8, a list 9-14, CANCEL -2, a header -3). */
#define PT_DEX_ID_KANTO 9
#define PT_DEX_ID_NATIONAL 14
#define PT_DEX_ID_HEADER (-3)
#define PT_DEX_ID_CANCEL (-2)
/* Hit codes (CtrPokeTouch_DexHit). */
enum { PT_DEX_HIT_TAB = 100, PT_DEX_HIT_ITEM = 200, PT_DEX_HIT_ROW = 300, PT_DEX_HIT_SLOT = 400, PT_DEX_HIT_UP = 500,
       PT_DEX_HIT_DOWN, PT_DEX_HIT_PREV, PT_DEX_HIT_NEXT, PT_DEX_HIT_AREA = 510, PT_DEX_HIT_CRY,
       PT_DEX_HIT_CONTINUE };
typedef struct {
    uint16_t species, number;
    uint8_t flags;                  /* seen (1) and caught (2) */
    char name[12];
} CtrPokeTouchDexMon;
/* The choice of the move to forget for a new one, drawn by the port as the red device (3ds_device_learn_impl.h), a
 * remote for FireRed's Summary in that mode: the Pokemon, its four moves and the new one (row 4), the selected move's
 * numbers and description. A row is walked to with the Summary's UP / DOWN and taken with its A; BACK is its B. */
#define PT_LEARN_ROWS 5
enum { PT_LEARN_HIT_ROW = 100 };
typedef struct {
    uint8_t on;
    int8_t cursor;
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    uint16_t species;
    uint8_t nameDefault;            /* the nickname is its species' own name (drawn in capitals), not one the player gave */
    char nickname[13], level[8];   /* nickname: six kana, two bytes each */
    char question[48];
    struct {
        uint16_t move;
        int8_t type;
        uint8_t locked;             /* an HM: FireRed will not forget it */
        char name[16], pp[12], power[8], accuracy[8], typeName[10];
    } row[PT_LEARN_ROWS];
    char desc[160];
} CtrPokeTouchLearn;

/* The move relearner (REMEMBER, the Two Island man), drawn by the port as the red device (3ds_device_relearn_impl.h), a
 * remote for FireRed's screen: the list (a window of PT_RL_ROWS rows of the list: the moves it can remember, then
 * FireRed's CANCEL row, the cursor and the selected move's numbers and description), a question with YES and NO, or a
 * message. A row is walked to with the list's own UP / DOWN and taken with its A; FireRed's CANCEL row is the CANCEL key
 * (PT_RL_HIT_CANCEL), taken as that row is. */
#define PT_RL_ROWS 5
enum { PT_RL_HIT_ROW = 200, PT_RL_HIT_UP = 230, PT_RL_HIT_DOWN = 231, PT_RL_HIT_YES = 232, PT_RL_HIT_NO = 233,
       PT_RL_HIT_MESSAGE = 234, PT_RL_HIT_CANCEL = 235 };
/* What the screen shows: the list; a message over it; a question with YES / NO over it; nothing but the backdrop (being set up). */
enum { PT_RL_PANEL_LIST = 0, PT_RL_PANEL_MESSAGE, PT_RL_PANEL_YESNO, PT_RL_PANEL_BLANK };
typedef struct {
    uint8_t on;
    uint8_t panel;                  /* PT_RL_PANEL_* */
    uint8_t menuCursor;             /* YES (0) or NO (1) while the panel is a question */
    uint8_t tap;                    /* the message waits for A: a tap on it goes on (and says so) */
    char message[200];              /* the question or message FireRed has printed */
    int8_t cursor;                  /* list index of the selected row (count: CANCEL) */
    uint8_t count;                  /* the moves; the list has one more row, CANCEL */
    uint8_t first;                  /* list index of the first row shown */
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    uint16_t species;
    char nickname[13], level[8];
    char question[48];
    struct {
        uint16_t move;              /* 0: CANCEL or no row */
        int8_t type;
        char name[16], pp[8], power[8], accuracy[8], typeName[10];
    } row[PT_RL_ROWS];
    char desc[160];
} CtrPokeTouchRelearn;

/* The letter being read (3ds_device_mail_impl.h): the paper's item (its icon), its name and who wrote it. */
typedef struct {
    uint8_t on;
    uint16_t item;                  /* 0: no icon */
    char name[32], from[24];        /* from: as typed, "" when unknown */
} CtrPokeTouchMail;

/* Easy Chat (3ds_field_easychat_impl.h), drawn by the port over the whole lower screen (the upper one keeps FireRed's own): a remote
 * for the game's screen. The phrase as chips, the group menu (or the letters of the A-Z order), a group's word list with its rail and
 * a letter strip, and the game's Yes/No questions over the dimmed phrase. Everything is the game's state (CtrBottomEasyChat_*); the
 * touches are walked to with its own keys (3ds_bottom_easychat_impl.h). */
#define PT_EC_WORDS 9
#define PT_EC_WORD_LEN 28
#define PT_EC_GROUPS 21
#define PT_EC_GROUP_LEN 20
#define PT_EC_ROWS 6                /* the list's rows shown, two words each */
#define PT_EC_LETTERS 26
#define PT_EC_ASK_LEN 48
enum { PT_EC_EDIT, PT_EC_GROUP, PT_EC_LETTER, PT_EC_LIST };         /* the screen: the phrase, the groups, the letters, a list */
enum { PT_EC_ASK_NONE, PT_EC_ASK_QUIT, PT_EC_ASK_DELETE, PT_EC_ASK_CONFIRM };
#define PT_EC_CELL_TOGGLE 24        /* the group and letter screens' A-Z / groups button and BACK, as cells */
#define PT_EC_CELL_BACK 25
/* Hit codes (CtrPokeTouch_EasyHit). */
enum { PT_EC_HIT_CHIP = 100, PT_EC_HIT_MODE_GROUPS = 120, PT_EC_HIT_MODE_ALPHA, PT_EC_HIT_DELETE = 130, PT_EC_HIT_CANCEL,
       PT_EC_HIT_CONFIRM, PT_EC_HIT_CELL = 200, PT_EC_HIT_ROW = 300, PT_EC_HIT_UP = 320, PT_EC_HIT_DOWN, PT_EC_HIT_RAIL,
       PT_EC_HIT_STRIP, PT_EC_HIT_BACK, PT_EC_HIT_TOGGLE, PT_EC_HIT_PLACE, PT_EC_HIT_YES = 340, PT_EC_HIT_NO };
typedef struct {
    uint8_t on;
    uint8_t screen;                 /* PT_EC_* */
    uint8_t ask;                    /* PT_EC_ASK_*: a question is open over the phrase */
    uint8_t askCursor;              /* 0 YES, 1 NO: the game's cursor on it */
    uint8_t alpha;                  /* the A-Z order is on */
    uint8_t wantAlpha;              /* the editor's mode buttons: the order the next word is picked in */
    uint8_t numWords;               /* the phrase's slots */
    int8_t cursor;                  /* the editor's selected chip, -1 none */
    int8_t footer;                  /* the editor's selected button (0 DELETE ALL, 1 QUIT, 2 OK), -1 none */
    int8_t cell;                    /* the group / letter screen's selected cell (PT_EC_CELL_*), -1 none */
    uint8_t groupCount;
    uint8_t letter;                 /* the strip's lit letter 0..25, 255 none */
    uint16_t listCount;             /* the list: its words, the first row shown, the selected word */
    uint16_t first;
    int16_t word;
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    char title[PT_EC_ASK_LEN];      /* the game's title of the screen */
    char listTitle[PT_EC_GROUP_LEN];/* the open group, or the letter */
    char ask1[PT_EC_ASK_LEN], ask2[PT_EC_ASK_LEN];
    char chip[PT_EC_WORDS][PT_EC_WORD_LEN];
    char group[PT_EC_GROUPS][PT_EC_GROUP_LEN];
    char list[2 * PT_EC_ROWS][PT_EC_WORD_LEN];
} CtrPokeTouchEasy;

/* A choice of the field (a script's multichoice, YES/NO or grid, the PC's menus) drawn by the port as the red device
 * (3ds_device_choice_impl.h), a remote for FireRed's menu: its rows as keys (cols > 0: a grid of that many columns),
 * the cursor's, and the question of the message box. A row is asked of the menu and taken with its A; BACK is its B. */
#define PT_CHOICE_MAX 8
enum { PT_CHOICE_HIT_ROW = 100 };
typedef struct {
    uint8_t on;
    uint8_t count, cols;
    int8_t cursor;
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    char label[PT_CHOICE_MAX][24];
    char prompt[160];
} CtrPokeTouchChoice;

/* FireRed's naming screen drawn by the port (3ds_device_naming_impl.h), a remote for it: the name so far, the keys of
 * its page (FireRed's own character codes, 0xff for no key), tabs for its three pages, DELETE and OK. A key moves
 * FireRed's cursor there and presses its A; a tab is its SELECT, DELETE its B. */
#define PT_NAME_ROWS 4
#define PT_NAME_COLS 8
#define PT_NAME_MAX 12
enum { PT_NAME_HIT_KEY = 200, PT_NAME_HIT_TAB = 240, PT_NAME_HIT_DELETE = 250, PT_NAME_HIT_OK = 251 };
typedef struct {
    uint8_t on;
    uint8_t page;                   /* FireRed's: 0 symbols, 1 capitals, 2 small letters */
    uint8_t cols, maxChars, len;
    int8_t curCol, curRow;          /* FireRed's cursor; curCol == cols is its button column */
    uint8_t gender;                 /* 1 male, 2 female, 0 none */
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    uint16_t species;               /* the Pokemon being named, or 0 */
    uint8_t key[PT_NAME_ROWS][PT_NAME_COLS];
    uint8_t text[PT_NAME_MAX];
    uint8_t title[32];              /* FireRed's question, its codes, 0xff ended */
} CtrPokeTouchName;

/* A PC box drawn by the port (3ds_device_box_impl.h), a remote for FireRed's own box: its name on the wallpaper's band,
 * its thirty places with their icons, PARTY and CLOSE BOX, the Pokemon in hand, the Party when the cursor is in it, the
 * box's menu, its YES/NO, its box chooser, its marks or its message. Texts are FireRed's own codes, 0xff ended.
 * A touch is the PC's own controller (3ds_storage_touch.h hit codes): the cursor walked there and A. */
#define PT_BOX_SLOTS 30
#define PT_BOX_MENU_MAX 7
enum { PT_BOX_STATE_BUSY = 0, PT_BOX_STATE_BOX, PT_BOX_STATE_MENU, PT_BOX_STATE_YESNO, PT_BOX_STATE_CHOOSE,
       PT_BOX_STATE_MESSAGE, PT_BOX_STATE_MARKS };
enum { PT_BOX_HIT_MESSAGE = 300 };  /* besides the ST_HIT_* codes: a message that waits for A */
typedef struct {
    uint16_t species;               /* 0: empty */
    uint8_t egg;
    uint32_t personality;
} CtrPokeTouchBoxMon;
typedef struct {
    uint8_t on, state;
    int8_t area, pos;               /* FireRed's cursor (ST_AREA_*) */
    uint8_t holding;                /* a Pokemon in hand (held) */
    uint8_t box;
    uint8_t wallpaper;              /* the box's wallpaper (FireRed's WALLPAPER_*) */
    uint8_t boxName[12];
    CtrPokeTouchBoxMon slot[PT_BOX_SLOTS];
    CtrPokeTouchBoxMon party[6];
    uint8_t partyName[6][12];
    uint8_t partyLevel[6];
    CtrPokeTouchBoxMon held;
    CtrPokeTouchBoxMon mon;         /* the Pokemon under the cursor, and FireRed's lines about it */
    uint8_t monText[4][20];         /* nickname, species, gender and level, item */
    uint8_t menuCount;              /* MARKS: the four marks, OK and CANCEL */
    int8_t menuCursor;
    uint8_t marks;                  /* MARKS: the marks on so far, bit 0 the circle .. bit 3 the heart */
    uint8_t menu[PT_BOX_MENU_MAX][16];
    uint8_t chooseName[12];
    uint8_t chooseCount;
    uint8_t message[48];            /* FireRed's line in the box's window, while it shows */
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    /* MOVE ITEMS: the items the Pokemon hold (a tag on each), the item in hand, and the one under the cursor with
     * its name and description for the note. */
    uint8_t items;
    uint16_t slotItem[PT_BOX_SLOTS];
    uint16_t partyItem[6];
    uint16_t handItem;
    uint16_t monItem;
    uint8_t itemName[16];
    uint8_t itemDesc[96];
} CtrPokeTouchBox;

/* The player's Trainer Card drawn by the port (3ds_device_card_impl.h), a remote for FireRed's own card: its front
 * (name, ID, money, Pokedex, time, the trainer's picture, stars and badges) or its back (FireRed's records). A touch
 * on it or on FLIP turns it over with the card's own key. A link card stays FireRed's picture. */
#define PT_CARD_ROWS 5
typedef struct {
    uint8_t on;                     /* the viewport shows this card (no GBA picture) */
    uint8_t back;                   /* its back is up */
    uint8_t pressed;                /* the card is held */
    char name[16];
    char label[6][16];              /* FireRed's NAME, IDNo., MONEY, yen sign, POKeDEX, TIME */
    uint32_t id, money;
    uint8_t hasDex, minutes, stars, badges;   /* badges: bit i, badge i */
    uint16_t caught, hours;
    uint8_t rows;                   /* the back's records */
    struct {
        char label[28];
        char value[20];
    } row[PT_CARD_ROWS];
} CtrPokeTouchCard;

typedef struct {
    uint8_t on;                     /* the viewport shows this Pokedex (no GBA picture) */
    uint8_t state;                  /* PT_DEX_STATE_*: the Pokedex's own input state */
    uint8_t national;               /* the National Pokedex is on */
    uint16_t seen[2], owned[2];     /* KANTO, NATIONAL */
    uint8_t menuCount;              /* the top menu's rows, headers included */
    int8_t menuCursor;              /* its selected row */
    struct {
        int8_t id;                  /* PT_DEX_ID_* or a habitat */
        uint8_t locked;             /* a habitat with nothing seen yet */
        char name[28];              /* FireRed's label */
    } menu[PT_DEX_MENU_MAX];
    uint8_t order;                  /* the ordered list's order (FireRed's DEX_ORDER_*) */
    int16_t listCount, listCursor, listFirst;
    CtrPokeTouchDexMon row[PT_DEX_ROWS];
    uint8_t category, page, pages, slots, slot, canPrev, canNext;
    CtrPokeTouchDexMon mon[4];      /* a habitat page's Pokemon */
    CtrPokeTouchDexMon sel;         /* the selected Pokemon (list, habitat page) or the one whose page is open */
    char kind[28];                  /* its category, height in decimeters, weight in hectograms, types and entry: */
    uint16_t height, weight;        /* only for a caught one */
    int8_t type[2];
    char typeName[2][10];
    char desc[200];
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    uint8_t added;                  /* the page a catch has just added (from the battle): no AREA or CRY */
} CtrPokeTouchDex;

typedef struct {
    uint8_t enabled[PT_COUNT];  /* usable now: drawn normally, else dimmed */
    uint8_t pressed;            /* element under a held touch, or PT_NONE */
    uint8_t selected;           /* the active destination, or PT_NONE */
    uint8_t running;            /* the running mode is on (Running Shoes) */
    uint8_t viewport;           /* the GPU shows a GBA picture in the viewport */
    uint8_t home;               /* HOME is the lower screen (the red device's; with no GBA picture) */
    uint8_t mapGroup;           /* which map: PT_MAP_KANTO..PT_MAP_SEVII67 */
    uint8_t mapVersion;         /* what else its art depends on (CtrPokeTouch_MapVersion) */
    uint8_t mapReady;           /* the map's art was there (CtrPokeTouch_MapPrepare) */
    uint8_t mapOpen;            /* the maps the region key leads to, a bit each (PT_MAP_*): with more than one, it says so */
    uint8_t playerFemale;       /* Leaf's map icon rather than Red's */
    int8_t playerX, playerY;    /* the player's cell on the map (22x15), -1: none */
    int8_t cursorX, cursorY;    /* the selected cell (FireRed's cursor icon), -1: none */
    uint8_t cursorFrame;        /* which of the cursor's two blink frames (0 with none) */
    uint8_t flyOk;              /* the selected destination can be Flown to now: HOME shows FLY */
    uint8_t party[6];           /* HOME's party: 0 no Pokemon, 1 able, 2 fainted, 3 a status (as CtrBattleView) */
    uint8_t partyHp[6];         /* HP left as a percentage (an egg: 100) */
    uint16_t partyIcon[6];      /* each slot's party icon (a species number, as CtrBottomParty_IconSpecies) */
    uint8_t partyEgg;           /* the slots that hold an egg, a bit each: no HP bar (FireRed shows none) */
    uint8_t speed;             /* the turbo chosen (3ds_fast_forward.h): 1 off, 2..4 the multiplier */
    uint8_t speedEnabled;       /* a tap on SPD can change it now (where START could) */
    uint8_t options;            /* the OPTIONS screen fills the viewport (no GBA picture, no map) */
    uint8_t optPage;            /* 0-based page index in OPTIONS */
    uint8_t optPageCount;       /* total pages in OPTIONS */
    uint8_t optSection, optSectionCount;   /* the section the page is in (Settings, Graphics, QoL, Nuzlocke) and how many there are */
    char optSectionName[PT_OPT_SECTIONS][PT_OPT_SECTION_LEN];   /* the section keys' words: the shown section's is its page's name */
    int8_t optKey;              /* the key (PT_OPT_KEY_*) under a held touch, or -1 */
    uint8_t optCount;           /* its rows on the current page */
    uint8_t optSelected;        /* the row the bar describes (within the current page) */
    int8_t optPressed;          /* the row under a held touch, or -1, and where on it: PT_OPT_ZONE_* */
    uint8_t optZone;
    uint8_t optFrameShow;       /* the selected row is FRAME: the bar shows a sample of its type ... */
    uint8_t optFrameType;       /* ... 0..9 */
    CtrPokeTouchLook optLook;   /* a look row is selected: the preview panel */
    char optLabel[PT_OPT_MAX][PT_OPT_LABEL_LEN];
    char optValue[PT_OPT_MAX][PT_OPT_VALUE_LEN];
    uint8_t optSwitch[PT_OPT_MAX];      /* an ON/OFF row, drawn as a switch: 0 not one, 1 off, 2 on */
    uint8_t optSegCount[PT_OPT_MAX];    /* a row of levels: how many (0: the "<" value ">" row) ... */
    uint8_t optSegValue[PT_OPT_MAX];    /* ... the one chosen, and their names */
    char optSegName[PT_OPT_MAX][PT_OPT_SEGS][PT_OPT_SEG_LEN];
    uint8_t save;               /* the SAVE screen fills the viewport (no GBA picture, no map) */
    uint8_t saveState;          /* SAVE_* */
    uint8_t saveSel;            /* the selected button: 0 YES, 1 NO */
    uint8_t savePressed;        /* PT_SAVE_* under a held touch */
    uint8_t saveBadges;         /* badges earned, 0-8 */
    uint8_t saveHasDex;         /* the Pokedex line is shown */
    uint8_t saveMinutes;        /* the play clock */
    uint16_t saveHours;
    uint16_t saveDex;           /* Pokemon caught */
    uint32_t saveMoney;
    uint8_t backVisible;        /* the contextual BACK control is on the bar (a screen with no touch exit of its own) */
    uint8_t backEnabled;        /* it can be used now: drawn normally, else dimmed */
    uint8_t backPressed;        /* it is under a held touch */
    uint16_t registeredItem;    /* 0: nothing registered */
    uint16_t bikeItem;          /* the Bicycle's item id, 0 when not owned */
    char bar[PT_BAR_LINES][PT_BAR_LEN]; /* contextual text, "" for no line */
    char playerName[16];        /* the Trainer Card button's label (UTF-8) */
    CtrPokeTouchBag bag;        /* the field Bag, when bag.on */
    CtrPokeTouchDex dex;        /* the Pokedex, when dex.on */
    CtrPokeTouchCard card;      /* the Trainer Card, when card.on */
    CtrPokeTouchLearn learn;    /* the move to forget, when learn.on */
    CtrPokeTouchRelearn relearn;/* the move relearner's list, when relearn.on */
    CtrPokeTouchMail mail;      /* the letter being read, when mail.on */
    CtrPokeTouchEasy ec;        /* Easy Chat over the whole screen, when ec.on */
    CtrPokeTouchName name;      /* the naming screen, when name.on */
    CtrPokeTouchBox box;        /* a PC box, when box.on */
    CtrPokeTouchChoice choice;  /* a choice of the field, when choice.on */
    uint8_t shell;          /* the lower screen's shell colour: 0 red, 1 blue, 2 green, 3 grey (CtrOptions_Shell) */
} CtrPokeTouchView;

extern const CtrPokeTouchRect gCtrPokeTouchRects[PT_COUNT];
/* BACK: a shell control in the bar's bottom-right corner, outside the viewport. It
 * stands for the B button of the open screen (3ds_bottom.c sends the real B). */
extern const CtrPokeTouchRect gCtrPokeTouchBack;
/* Whether the view is drawn as the red device (HOME, the Bag, or a field view of the dispatcher,
 * 3ds_device_field_impl.h): no START column under it. */
bool CtrPokeTouch_DeviceShown(const CtrPokeTouchView *view);
/* Whether a bottom-screen pixel is on BACK as the view draws it: a field view of the dispatcher's
 * BACK key while it is shown (backVisible), an ember view's gCtrPokeTouchBack. */
bool CtrPokeTouch_BackHit(const CtrPokeTouchView *view, int x, int y);

/* The OPTIONS row and zone (PT_OPT_ZONE_*) under a bottom-screen pixel of the view shown (OPTIONS,
 * no GBA picture); false when none. On a row of levels any zone but LABEL is the level there
 * (PT_OPT_ZONE_SEG + CtrPokeTouch_OptionSegment); ◀ ▶ under the rows are the row described's
 * PREV / NEXT. */
bool CtrPokeTouch_OptionHit(const CtrPokeTouchView *view, int x, int y, int *row, int *zone);
/* On a row of `count` levels, the one under bottom-screen column `x` (the zone was not LABEL). */
int CtrPokeTouch_OptionSegment(int x, int count);
/* The OPTIONS key under a bottom-screen pixel: PT_OPT_KEY_SECTION + a section, PT_OPT_KEY_UP,
 * PT_OPT_KEY_DOWN (with more than one page), else -1. */
int CtrPokeTouch_OptionKey(const CtrPokeTouchView *view, int x, int y);

/* SAVE's key under a bottom-screen pixel (PT_SAVE_*) for the dialogue state: YES and NO while
 * confirming, OK (PT_SAVE_FIRST, in YES's place) after a result, none while writing. */
int CtrPokeTouch_SaveHit(int x, int y, int state);

/* The START column's PT_* under a bottom-screen pixel, or PT_NONE (every view but HOME). */
int CtrPokeTouch_HitTest(int x, int y);
bool CtrPokeTouch_FlyHit(int x, int y);          /* HOME's FLY key, while it is drawn */
/* HOME (view->home, no GBA picture) under lower-screen point (x, y): its keys PT_POKEDEX..PT_OPTIONS, PT_RUN,
 * PT_HOME_SPEED, PT_HOME_PARTY + a slot that holds a Pokemon, else PT_NONE (the map window is not a key). */
int CtrPokeTouch_HomeHit(const CtrPokeTouchView *view, int x, int y);
/* A lower-screen point in HOME's map window, as the window's own pixel (vx, vy); false outside it. */
bool CtrPokeTouch_HomeWindowPoint(int x, int y, int *vx, int *vy);
/* The centre of the map's cell (cx, cy) of the 22x15 grid on the lower screen; false off the grid. */
bool CtrPokeTouch_HomeCellPoint(const CtrPokeTouchView *view, int cx, int cy, int *x, int *y);
/* The 8x8 cell under a lower-screen point; false off the grid or outside the window. */
bool CtrPokeTouch_HomeMapCell(const CtrPokeTouchView *view, int x, int y, int *cx, int *cy);
/* What the field Bag (view->bag.on) shows under lower-screen point (x, y): a PT_BAG_HIT_* code, or -1. */
int CtrPokeTouch_BagHit(const CtrPokeTouchView *view, int x, int y);
/* What the Pokedex (view->dex.on) shows under lower-screen point (x, y): a PT_DEX_HIT_* code, or -1. */
int CtrPokeTouch_DexHit(const CtrPokeTouchView *view, int x, int y);
/* The top menu row a tab (0-2) leads to, and the first row of the habitat/search page before (-1) or after (1) the
 * one shown; -1 when there is none. */
int CtrPokeTouch_DexTabRow(const CtrPokeTouchView *view, int tab);
int CtrPokeTouch_DexGridPageRow(const CtrPokeTouchView *view, int dir);
/* The Pokedex's pictures, from FireRed's tables (pokedex_screen.c): a species' front picture (64x64 4bpp, 8x8 tiles
 * in rows of eight) and palette, a top menu row's icon (64x48, the same layout), a footprint (16x16 1bpp, four 8x8
 * tiles) and the caught marker (one 4bpp tile). 0 when there is none. */
/* The row of the move to forget (view->learn.on) under lower-screen point (x, y): PT_LEARN_HIT_ROW + row, or -1. */
int CtrPokeTouch_LearnHit(const CtrPokeTouchView *view, int x, int y);
/* The row, scroll arrow or CANCEL of the relearner's list, YES / NO, or the message (view->relearn.on) under (x, y):
 * PT_RL_HIT_*, or -1. */
int CtrPokeTouch_RelearnHit(const CtrPokeTouchView *view, int x, int y);
/* What Easy Chat (view->ec.on) shows under lower-screen point (x, y): a PT_EC_HIT_* code (a chip's is PT_EC_HIT_CHIP + slot, a cell's
 * PT_EC_HIT_CELL + cell, a list row's PT_EC_HIT_ROW + its place among the rows shown), or -1. */
int CtrPokeTouch_EasyHit(const CtrPokeTouchView *view, int x, int y);
int CtrPokeTouch_EasyRailTarget(const CtrPokeTouchEasy *ec, int y);   /* the first row the rail's thumb at y stands for (-1: no rail) */
int CtrPokeTouch_EasyStripLetter(int x, int y);                       /* the letter 0..25 under the strip at (x, y), or -1 */
/* The row of the field's choice (view->choice.on) under lower-screen point (x, y): PT_CHOICE_HIT_ROW + row, or -1. */
int CtrPokeTouch_ChoiceHit(const CtrPokeTouchView *view, int x, int y);
/* The key, tab or button of the naming screen (view->name.on) under lower-screen point (x, y): a PT_NAME_HIT_* code
 * (a key's is PT_NAME_HIT_KEY + row * PT_NAME_COLS + column, a tab's PT_NAME_HIT_TAB + page), or -1. */
int CtrPokeTouch_NameHit(const CtrPokeTouchView *view, int x, int y);
/* What of the PC box (view->box.on) is under lower-screen point (x, y): a 3ds_storage_touch.h ST_HIT_* code, or
 * PT_BOX_HIT_MESSAGE, or ST_HIT_NONE. */
int CtrPokeTouch_BoxHit(const CtrPokeTouchView *view, int x, int y);
/* The game's side: a Pokemon's icon (its first frame, 4bpp 32x32) and palette; and the box as it is now (0 while the
 * port should show FireRed's own picture). */
int CtrPokeTouch_MonIcon(unsigned species, unsigned personality, int egg, unsigned char *tiles, unsigned short *palette);
/* A box wallpaper's 160x144 picture, as FireRed draws it: BGR555, 0 when there is none. */
int CtrPokeTouch_Wallpaper(unsigned id, unsigned short *rgb555);
/* A mark's 8x8 shape (0 circle .. 3 heart) from FireRed's marking sheet: 0 none, 3 the mark, 4 its edge. */
int CtrPokeTouch_MarkShape(int mark, unsigned char *indexes);
int CtrBottomStorage_BoxFill(CtrPokeTouchBox *box);
/* Whether the Trainer Card (view->card.on, no GBA picture) is under lower-screen point (x, y): its screen or FLIP. */
int CtrPokeTouch_CardHit(const CtrPokeTouchView *view, int x, int y);
/* Whether the letter being read (view->mail.on, no GBA picture) is under lower-screen point (x, y): its screen,
 * which closes it as CLOSE does. */
bool CtrPokeTouch_MailHit(const CtrPokeTouchView *view, int x, int y);
/* The card's pictures, from FireRed's (trainer_card.c): the trainer's front picture (64x64 4bpp, 8x8 tiles in rows of
 * eight) and the badges (128x16 4bpp: badge i at tiles 2i, 2i + 1 and 2i + 16, 2i + 17). 0 when there is none. */
int CtrPokeTouch_CardTrainerPic(unsigned char *tiles, unsigned short *palette);
int CtrPokeTouch_CardBadges(unsigned char *tiles, unsigned short *palette);
int CtrPokeTouch_DexMonPic(unsigned species, unsigned char *tiles, unsigned short *palette);
/* The species' HeartGold picture whole: 80x80 4bpp in rows of 40 bytes, and its palette. 0 without it. */
int CtrPokeTouch_DexMonPic80(unsigned species, unsigned char *pixels, unsigned short *palette);
int CtrPokeTouch_DexMenuIcon(int id, unsigned char *tiles, unsigned short *palette);
int CtrPokeTouch_DexFootprint(unsigned species, unsigned char *bits);
int CtrPokeTouch_DexCaughtMarker(unsigned char *tiles, unsigned short *palette);
/* A bottom-screen pixel inside the viewport as GBA coordinates; false
 * outside the viewport. */
bool CtrPokeTouch_ViewportToGba(int x, int y, int *gx, int *gy);
/* Draw the whole screen for `view` into the bottom canvas. */
void CtrPokeTouch_Draw(uint16_t *canvas, const CtrPokeTouchView *view);
/* The canvas holds `old` drawn: redraw only the elements that look different
 * in `view` (a press, a state, the bar). Each element draws only inside its
 * rectangle, and that rectangle (a changed map icon: its old and new place)
 * is added to `dirty` to be presented; nothing is added when nothing changed. */
void CtrPokeTouch_Update(uint16_t *canvas, const CtrPokeTouchView *old, const CtrPokeTouchView *view,
                         CtrDirtyList *dirty);
/* The lower screen with nothing to show for FireRed's screen above (its title, naming, a shop, an evolution, a trade):
 * the ember backdrop, and the project's logo when `logo`. */
void CtrPokeTouch_DrawIdle(uint16_t *canvas, int logo);

/*
 * The battle screen (3ds_device_battle_impl.h, CtrBattleUi_*): the whole lower screen while FireRed's battle is on
 * the upper one, drawn as the red device. While the player's controller waits at the action menu: a screen with the
 * lead and the party's six balls, FIGHT across, BAG, POKeMON and RUN below; at the move list, the chosen move on the
 * screen (type, category, power, accuracy, description) and the four move keys and CANCEL; at any other moment the
 * action screen with its keys grey.
 * Everything shown is what the controller reports: the screen has no state of its own. In a wild battle a fourth button
 * between BAG and POKeMON throws a ball at once (the quick ball: the ball's own icon and count, arrows or a swipe
 * across it for the others, CTR_ACTION_BALL as the controller's cursor). In the battle Bag (BUI_BAG, the Bag FireRed
 * opens from BAG; 3ds_device_bag_impl.h, the field Bag's page) it is a remote for that Bag: its items (the Items pocket:
 * the balls have their button, the key items stay out but for the Berry Pouch, which BERRIES opens) with icon and
 * quantity, the selected item's description, its menu or message, and CLOSE; what it shows is the Bag's own state and
 * save data. In the Party (BUI_PARTY, the field's and the battle's: a
 * switch, the replacement for a Pokemon that fainted, the target of an item; 3ds_device_party_impl.h) it is a remote for
 * FireRed's Party, drawn as the red device: the six Pokemon as bays in two columns (icon, name, HP bar and numbers, level
 * or condition), the screen with what the Party says, its menu or message when it has one open, and CANCEL unless the
 * Party cannot be cancelled.
 */
enum { BUI_IDLE = 0, BUI_ACTION, BUI_MOVE, BUI_BAG, BUI_PARTY, BUI_TARGET, BUI_SAFARI };
/* Buttons (CtrBattleUi_Hit): the controller's action numbers while choosing an action, the move slots 0-3 and
 * BUI_CANCEL while choosing a move; in the Bag the codes below. */
enum { BUI_ACT_FIGHT = 0, BUI_ACT_BAG = 1, BUI_ACT_POKEMON = 2, BUI_ACT_RUN = 3, BUI_CANCEL = 4 };
/* The quick ball's parts (hit codes of the action screen): the button (a throw), its arrows (the other balls). */
enum { BUI_ACT_BALL = 8, BUI_ACT_BALL_PREV = 9, BUI_ACT_BALL_NEXT = 10 };
#define BUI_NAME_LEN 18
#define BUI_DESC_LEN 112
typedef struct {
    uint8_t present;                /* a real move in the slot */
    uint8_t type;                   /* FireRed's type number (the card's colours) */
    char name[BUI_NAME_LEN];
    char typeName[10];
    char power[5];                  /* "90", or "-" for a move with no power */
    char accuracy[5];               /* "100", or "-" for a move that does not check accuracy */
    uint8_t pp, maxPp;              /* what the controller's own move list holds (CtrBattle_MovePP) */
    uint8_t matchup;                /* against the foe: a CTR_FX_* (3ds_type_chart.h), CTR_FX_NONE draws nothing */
    uint8_t stab;                   /* the move is of one of the attacker's own types (same-type attack bonus) */
    uint8_t effectChance;           /* percent chance of its extra effect, 0 for none */
    int8_t priority;                /* the move's priority bracket, 0 for most */
    char desc[BUI_DESC_LEN];        /* FireRed's description, wrapped when drawn */
} CtrBattleMove;
enum { BUI_BAG_TAB = 100, BUI_BAG_ROW = 200, BUI_BAG_MENU = 300, BUI_BAG_CANCEL = 400, BUI_BAG_MESSAGE = 500,
       BUI_BAG_UP = 600, BUI_BAG_DOWN = 601, BUI_BAG_BERRIES = 602 };
#define BUI_BAG_ROWS 8
#define BUI_ITEM_LEN 20
typedef struct {
    uint8_t pocket;                 /* FireRed's pocket: 0 Items, 1 Key Items, 2 Poke Balls */
    uint8_t bagState;               /* the Bag's own input state (CTR_BAG_*: list, menu, message, busy ...) */
    int16_t count;                  /* items in the pocket */
    int16_t cursor;                 /* the selected item (count is the list's own CANCEL row: the CLOSE key) */
    int16_t first;                  /* the item of the first row shown */
    struct {
        uint16_t item;              /* 0: no item in the row */
        uint16_t qty;
        char name[BUI_ITEM_LEN];
    } row[BUI_BAG_ROWS];
    uint16_t selItem;               /* the selected item, its name and description */
    char selName[BUI_ITEM_LEN];
    char selDesc[BUI_DESC_LEN];
    uint8_t menuCount;              /* the open context menu's rows (USE, CANCEL ...) */
    int8_t menuCursor;
    char menuName[3][12];
    char message[96];               /* what the Bag printed and waits on */
    uint16_t berries;               /* the Berry Pouch when the Bag has it (BERRIES opens it), else 0 */
} CtrBattleBag;
enum { BUI_PTY_SLOT = 700, BUI_PTY_CANCEL = 710, BUI_PTY_CONFIRM = 711, BUI_PTY_MENU = 720, BUI_PTY_MESSAGE = 730 };
/* BUI_PTY_MULTI: the picks of several Pokemon (a link battle, the Union Room), with CONFIRM over CANCEL. */
enum { BUI_PTY_SWITCH = 0, BUI_PTY_TARGET, BUI_PTY_FORCED, BUI_PTY_MULTI };
enum { BUI_PTY_NOT_ABLE = 0, BUI_PTY_ABLE, BUI_PTY_FIRST };
/* What a card says about teaching or using the item picked (FireRed's own box text): the pill on the card. */
enum { BUI_PTY_TAG_NONE = 0, BUI_PTY_TAG_ABLE, BUI_PTY_TAG_NOT_ABLE, BUI_PTY_TAG_LEARNED, BUI_PTY_TAG_NO_USE };
/* BUI_PTY_LEVELUP: a Rare Candy's level up, a card over the slots with the new stats and their gains, its message
 * below and OK in CANCEL's place. */
enum { BUI_PTY_NONE = 0, BUI_PTY_ACTIONS, BUI_PTY_TEXT, BUI_PTY_YESNO, BUI_PTY_LEVELUP };
typedef struct {
    uint8_t present[6];             /* a Pokemon in the slot */
    uint8_t egg[6];
    uint8_t status[6];              /* GetMonAilment: 1 PSN, 2 PAR, 3 SLP, 4 FRZ, 5 BRN, 6 PKRS */
    uint8_t level[6];
    uint16_t hp[6], maxHp[6];
    uint16_t icon[6];               /* FireRed's icon species of the slot (the icon is read through the bridge) */
    uint8_t iconFrame[6];           /* bit 0: which of the icon's two frames it shows (0 when fainted); bit 1: the hop's phase
                                     * (not drawn: the bays show frame 0) */
    char name[6][11];
    uint8_t nameIsSpecies[6];       /* the name is its species' own (shown in capitals), not one the player gave */
    int8_t cursor;                  /* the Party's own selection: a slot 0-5 */
    uint8_t mode;                   /* BUI_PTY_SWITCH, BUI_PTY_TARGET (an item's target), BUI_PTY_FORCED (CANCEL grey) */
    uint8_t panel;                  /* BUI_PTY_*: what the Party has open over the slots */
    uint8_t menuCount;              /* its menu's rows (SHIFT, SUMMARY, CANCEL; YES, NO; the field's up to 8) */
    int8_t menuCursor;
    char menuName[8][14];
    uint8_t menuBlue, menuDark;     /* bit i for menu row i: a field move (its name in blue), CANCEL (a dark key); 0 for YES / NO */
    char message[96];
    char caption[64];               /* what the Party says under the slots ("" for the mode's own words) */
    uint8_t lead;                   /* the slots in the left column, as FireRed lays them out (1, or 2 in a double; not drawn) */
    uint8_t entry[6];               /* BUI_PTY_MULTI: BUI_PTY_NOT_ABLE, BUI_PTY_ABLE, or BUI_PTY_FIRST + its place in the order */
    int8_t button;                  /* the Party's cursor on CONFIRM (0, BUI_PTY_MULTI only) or CANCEL (1), else -1 */
    uint8_t tag[6];                 /* BUI_PTY_TAG_*: ABLE / NOT ABLE / LEARNED (a TM, HM or tutor move), NO USE (a stone) */
    uint8_t lvSlot, lvLevel;        /* BUI_PTY_LEVELUP: the Pokemon's slot and its new level */
    uint16_t lvStat[6];             /* its new stats, in the Summary's order (HP, ATTACK, DEFENSE, SP. ATK, SP. DEF, SPEED) */
    int16_t lvGain[6];              /* what each one gained */
} CtrBattleParty;
/* BUI_TARGET: a double battle's move being aimed. The four cards are the opposing battlers on top and the player's
 * below (left, right); only the ones FireRed lets the move be aimed at are `present`. The hit codes are the card
 * (0-3) or BUI_CANCEL; `cursor` is the card of the controller's own target cursor. BUI_SAFARI: BALL, BAIT, ROCK,
 * RUN, FireRed's own four, in the controller's 2x2 order (codes 0-3, `cursor` its cursor). */
typedef struct {
    uint8_t present;
    int8_t battler;                 /* the battle's battler number: what the controller's cursor is on */
    uint8_t side;                   /* 0 opposing, 1 the player's */
    uint8_t level;
    uint8_t matchup;                /* CTR_FX_* effectiveness of the chosen move against this battler */
    uint16_t hp, maxHp;
    char name[BUI_NAME_LEN];
    uint8_t nameIsSpecies;          /* the name is its species' own (shown in capitals), not one the player gave */
} CtrBattleTarget;
typedef struct {
    uint8_t state;                  /* BUI_* */
    int16_t pressed;                /* the button held under a touch, or -1 */
    int8_t cursor;                  /* the controller's own cursor: the chosen key, unless a touch holds another */
    uint8_t showCursor;             /* the last input was a button (not drawn: the red device always shows the cursor) */
    uint8_t party[6];               /* 0 no Pokemon, 1 able, 2 fainted, 3 status condition */
    uint8_t partyHp[6];             /* HP left as a percentage (an egg: 100) */
    uint8_t partyStatus[6];         /* 0 none, else 1..6 PSN PAR SLP FRZ BRN PKRS */
    char activeName[BUI_NAME_LEN];  /* the Pokemon in the field: the action screen's lead and question */
    uint8_t activeNameIsSpecies;    /* its name is its species' own (shown in capitals), not one the player gave */
    uint8_t activeLevel;
    uint8_t activeGender;           /* 0 male, 1 female, 2 none (CtrBottomParty_Gender) */
    uint8_t activeSlot;             /* its slot in the party */
    uint16_t activeIcon;            /* its party icon (a species number, as CtrBottomParty_IconSpecies gives it) */
    int8_t activeType[2];           /* its types (FireRed's numbers), -1 for none (not drawn) */
    char activeTypeName[2][12];
    uint8_t activeFrame;            /* 0 or 1, flips about four times a second (not drawn: the icon is its frame 0) */
    uint16_t activeHp, activeMaxHp;
    uint8_t ball;                   /* the quick ball: CTR_QUICK_BALL_* (3ds_battle_nav.h), NONE for no button */
    uint8_t ballPlace, ballCount;   /* the ball shown among the Bag's balls (its place is not drawn; others: the arrows) */
    uint16_t ballItem, ballQty;
    uint8_t runBlocked;             /* BUI_ACTION: fleeing is impossible now (a trainer, a trapping ability or move): RUN is grey and takes no tap */
    char ballName[BUI_ITEM_LEN];    /* (not drawn) */
    uint8_t safari;                 /* BUI_IDLE in the Safari Zone: its own keys, grey */
    CtrBattleMove move[4];
    CtrBattleBag bag;
    CtrBattleParty pty;
    CtrBattleTarget target[4];
    uint8_t shell;          /* the lower screen's shell colour: 0 red, 1 blue, 2 green, 3 grey (CtrOptions_Shell) */
} CtrBattleView;

void CtrBattleUi_Draw(uint16_t *canvas, const CtrBattleView *view);
/* The canvas holds `old` drawn: redraw only what differs in `view` and mark its dirty rectangles. */
void CtrBattleUi_Update(uint16_t *canvas, const CtrBattleView *old, const CtrBattleView *view, CtrDirtyList *dirty);
/* The button under a canvas point of the screen `view` shows, or -1. */
int CtrBattleUi_Hit(const CtrBattleView *view, int x, int y);


/*
 * The Pokemon Summary (3ds_device_summary_impl.h, CtrSummaryUi_*): the whole lower screen while FireRed's Summary of the
 * party runs hidden behind it, from the field's Party, the battle's or a PC box (the same screen for all). It shows what
 * the Summary reports (CtrSummaryView) as the red device: the tabs INFO / STATS / MOVES / MEMO and the keys for the
 * previous and next Pokemon, the screen with the Pokemon and the page, the note and BACK. INFO, STATS and MOVES are
 * FireRed's three pages (its move details are MOVES with a move selected); MEMO (SUI_TAB_TRAINER) is the trainer memo
 * of its first page, shown on its own.
 */
enum { SUI_TAB_INFO = 0, SUI_TAB_STATS, SUI_TAB_MOVES, SUI_TAB_TRAINER, SUI_TABS };
/* Hit codes (CtrSummaryUi_Hit): the tabs, the previous / next Pokemon, BACK, a move card or row, the details' CANCEL. */
enum { SUI_HIT_TAB = 0, SUI_HIT_PREV = 10, SUI_HIT_NEXT = 11, SUI_HIT_BACK = 12, SUI_HIT_MOVE = 20, SUI_HIT_CANCEL = 24,
       SUI_HIT_IVTOGGLE = 30 /* the STATS page's STATS | IV/EV switch */ };
/* FireRed's Summary pages (pokemon_summary_screen.h PSS_PAGE_*). */
enum { SUI_PAGE_INFO = 0, SUI_PAGE_SKILLS, SUI_PAGE_MOVES, SUI_PAGE_MOVES_INFO };
typedef struct {
    CtrSummaryView data;            /* what the Summary reports (the last complete report while it loads a Pokemon) */
    uint16_t icon;                  /* FireRed's icon species of the Pokemon shown (the icon cache's key) */
    int16_t pressed;                /* the hit code under a held touch, or -1 */
    uint8_t trainer;                /* MEMO (SUI_TAB_TRAINER) is shown (FireRed is on its first page) */
    uint8_t ivView;                 /* STATS shows the IVs, EVs and Hidden Power instead of the stats and the experience */
    uint8_t loading;                /* nothing reported yet: the frame only */
    uint8_t shell;          /* the lower screen's shell colour: 0 red, 1 blue, 2 green, 3 grey (CtrOptions_Shell) */
} CtrSummaryUiView;

void CtrSummaryUi_Draw(uint16_t *canvas, const CtrSummaryUiView *view);
/* The canvas holds `old` drawn: redraw only the parts that differ in `view` and mark their dirty rectangles. */
void CtrSummaryUi_Update(uint16_t *canvas, const CtrSummaryUiView *old, const CtrSummaryUiView *view, CtrDirtyList *dirty);
/* The hit code under a canvas point of the screen `view` shows, or -1 (a disabled control is not hit). */
int CtrSummaryUi_Hit(const CtrSummaryUiView *view, int x, int y);
/* The tab `view` shows (SUI_TAB_*). */
int CtrSummaryUi_Tab(const CtrSummaryUiView *view);

#endif
