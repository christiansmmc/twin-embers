# Rework 2, plan 3: the views without a board, the ember look removed — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every lower-screen view still in the ember look (OPTIONS, SAVE, the Trainer Card, the letter, the Pokédex, field choices and YES/NO, forget a move, REMEMBER, the naming keyboard, the PC box, Easy Chat, the frame round FireRed's picture, the idle screen) is drawn as the red device and gets a "B — como ficou" board made from its drawn rectangles; the START column, the X/Y tiles, the route code and the ember look are deleted; README and CHANGELOG say so.

**Architecture:** A dispatcher (`3ds_device_field_impl.h`, included last by `3ds_poketouch.c`) holds a table of the B field views — when each is shown, how it draws whole, whether two views draw the same, where its BACK key is — so `CtrPokeTouch_Draw/Update` and `ProcessBack` stop knowing the views one by one. Each view is its own `3ds_device_<view>_impl.h` that takes over its public draw/update/hit functions and deletes the ember code it orphans (plan 2's pattern); new B files are included in task order after `3ds_device_summary_impl.h`, before the `3ds_field_*` files still there. v0.6.0's fills, hit codes and handlers stay unless a task says otherwise. The boarded pages' shapes (the Bag's list page, the Summary's screen and note, HOME's key places) are reused by name; the few new ones go in `3ds_device_layout.h`. Then the boards tool, then the ember shell goes.

**Tech Stack:** C11 (devkitARM `-std=gnu11`, host gcc `-std=c11`, both `-Werror`), Python 3 tools.

**Spec:** `docs/superpowers/specs/2026-11-14-lower-screen-rework2-design.md` (5.5, 5.6, 5.8, 5.9, 7.1-7.3, 7.7). Brief, its decisions taken as given ("decision N"): `.superpowers/sdd/plan3-brief.md`. Plan 2 (`docs/superpowers/plans/2026-11-15-rework2-02-b-kit-and-boarded-views.md`): its Global Constraints, Review Focus and owner decisions 1-5 still apply. Code contract: `3ds_device_kit_impl.h`, `3ds_device_layout.h`, `3ds_device_bag_impl.h`, `3ds_device_summary_impl.h`. No view here has a board: every rectangle is decided below; text placement inside a rectangle follows the boarded pages' habits.

## Global Constraints

- Commits: English Conventional Commits ending with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>` and `Claude-Session: https://claude.ai/code/session_01W753ukip7Xr7Uc89wPXquM`; PR bodies end with `🤖 Generated with [Claude Code](https://claude.com/claude-code)` and the session URL. No force-push (stacked branches are updated by merging). Files added by name; never `.dev.local.json`, `.progress/`, `.superpowers/`, `build/`.
- Before every commit: `python tools/manifest_update.py && python tools/source_audit.py && python tools/source_audit.py --history-since 0afd7a2bdb75a7d1ec2d4d10fb9a84f6f23e08ab` → exit 0; every file ≤ 262,144 bytes. `3ds_video.c` (258,680) is not edited (it already draws the picture at `PT_VIEW_X/Y`); `3ds_bottom.c` (215,421) only shrinks overall.
- Nothing ROM-derived committed; boards are made from the synthetic harness (no pack) and stay under `build/`.
- `-Werror`: what a task orphans (static function, table, enum value, view field) it deletes; `poketouch_render_test.c` and `device_render_test.c` include all of `3ds_poketouch.c` and keep compiling in every commit.
- The legend rule (decision 1): you choose a key, you read an embedded screen, you leave by a dark key; the chosen key is the game's cursor, sunk with its LED (`DevChosen(id, cursor, pressed)`, a touch sinks it on the touch frame); a list inverts its chosen row (`DevRow`); ▲▼ keys page or scroll. No screws except the frame and the idle screen.
- Words (decision 2 = plan 2's owner decision 5): port words UPPER CASE in all three `CTR_TR` languages; game strings as FireRed has them (`DevUpper` only for those that arrive title-cased), POKé's é stays; typed names as typed; a name equal to its species' own upper case (`DevMonName`). Each view task deletes its fields from `CtrTextCase_Touch` (`3ds_text_case_views.h`). BACK's word is everywhere `DevBackWord()` (Task 1).
- New files use the `Dev*` kit and only `UI_FN`, `UI_RGB`, `UI_TEXT_H`, `UiIn`, `UiInset`, `UiText`, `UiTextWidth`, `UiTypeColor`, `UiImage` of the old theme; no `Em*`, `Ll*`, `Ts*`, `kEm*`, `FB*`, `BDescribe*`, `BPWrap`, `EmSurface`. Every drawn or hit rectangle is the layout header's; keys hit on `DevKeyArea`, rows and screens on themselves.
- Redraw (decision 5): a B field view draws whole when a field it draws changes (`same`: the fields it draws, `shell`, the press as drawn); `Update` == full `Draw` pixel for pixel; an unchanged view, or a change of a field it does not draw, presents nothing (every suite asserts it with `v.mapGroup ^= 1; v.speed ^= 1;`, HOME's fields, which survive Task 10).
- Every new key ≥ 28x20 (`device_layout_test.c`); every suite ends `assert(sDevKeyCut == 0)` and runs in the ES and DE label-fit builds (6 px glyphs); a cut port word on a key or a row is counted in `sDevKeyCut`.
- Touch keeps doing what it does in v0.6.0 (spec goal 3): hit codes, handlers, press/release timing stay unless a task's Interfaces change them, and every changed geometry is pinned by a test hitting each drawn key and row.
- Art a view draws (picture, icon, item, badge, wallpaper, mark, sprite, frame sample, logo, FireRed's picture) is announced with `DevArt(kind, x, y, w, h)` (Task 1; nothing on the console) so a board can show a stand-in.
- Recipes: none expected; if one is needed it is 0266 (`tools/make_recipe.py` against a current `build/dev`), and the owner is told.
- Every task ends with `python tools/dev.py build`, `python tools/dev.py test` (all PASS; single suites `python tools/dev.py test <suite>`), the audit line, `python tools/dev.py render <its views>` (PNGs under `build/renders/<view>/` for the controller), `python tools/dev.py run` (copies to Azahar) and its Azahar check.
- Branches (decision 12, split proposed): PR C1 `feat/b-boardless-views` (base `feat/b-party-summary-bag`) Tasks 1-4; PR C2 `feat/b-boardless-views-2` (base C1) Tasks 5-8; PR D `feat/b-ember-removal` (base C2) Tasks 9-12. **Why split PR C:** thirteen views is more than PR A and PR B together (their review diff was ~1 MB); six then seven views are reviewable, and the stack merges together anyway. If the owner wants one PR C, Tasks 5-8 stay on `feat/b-boardless-views` and Task 4 Step 8 only runs the check.

## Review Focus

1. The naming keyboard on its three pages (8 columns with short rows, 6 columns of symbols), in the ES and DE builds: every key FireRed has is hit exactly where drawn, a `0xff` place is neither drawn nor hit, the page / DELETE / OK words are uncut — `NameEveryKey` (Task 5).
2. The Pokédex list at its top and its bottom, with a seen-not-caught entry: ▲ (top) and ▼ (bottom) grey and not hit, the seen-only row has no caught mark, its page no type chips — `dex` suite (Task 3).
3. The PC box full, empty, and with a Pokémon in hand: every place hit where drawn in both, the cursor's frame on place 29, the held Pokémon in the hand's screen, nothing of the box hit under a menu — `box` suite (Task 6).
4. Easy Chat's word list at its first and last rows: the rail and the letter strip give the rows v0.6.0's plan expects at both ends, the last row's words are hit and nothing after them — `easychat` suite (Task 7).
5. A B view while the START column still exists in code (PR C): no tap on the Pokédex's or the card's right side reaches the route or the column, whose rectangles lie under B's ▲▼ and BACK; after PR D the route and the column are gone and X / Y still use the registered item and the Bicycle — `test_column_only_under_ember_views` (Task 1), `test_route_and_column_gone` (Task 10).

## Owner decisions (2026-11-17), applied in the tasks below

1. Plan approved with three PRs: C1 (Tasks 1-4), C2 (Tasks 5-8), D (Tasks 9-12).
2. PC box places are 24 px, the wallpaper at 1:1 (not v0.6.0's stretched 28x25) — Task 6.
3. The idle logo sits on a dark embedded screen — Task 8.
4. The relearner's CANCEL is a dark key (new code 235, like the Bag's CLOSE), no BACK key — Task 4.

---

### Task 1: The field dispatcher, the shared shapes, and OPTIONS

**Files:**
- Create: `3ds_port/src/3ds_device_field_impl.h`, `3ds_port/src/3ds_device_options_impl.h`, `3ds_port/tests/device_suite_options.h` (included by `device_render_test.c` before `kSuites`)
- Modify: `3ds_poketouch.c` (Draw/Update call the dispatcher after the Bag; OPTIONS' ember code goes — `OptRect`, `OptPageColor`, `OptRow`, `OptLookRect`, `OptLookPanel`, `OptPageButton`, `gCtrPokeTouchOptPage`, `CtrPokeTouch_HitOptPage`, `CtrPokeTouch_HitBack`, the OPTIONS branches of `MenuFrame`/`Bar`/`Update`, then what the compiler reports; `FrameLoadFile`/`FrameSample` move to the options file), `3ds_poketouch.h`, `3ds_poketouch_options.c/.h`, `3ds_bottom.c` (`ProcessOptionsTouch`, `ProcessBack`, `ProcessTouch`, `PokeTouchHit`), `3ds_device_kit_impl.h`, `3ds_device_layout.h`
- Test: `device_render_test.c` (`options`), `device_layout_test.c`, `tools/test_device_views.py` (`test_options`), `tools/render_device_views.py` (`VIEWS` gains each new suite as it lands), `tools/test_field_presentation.py`, `3ds_port/tests/poketouch_options_test.c`, `poketouch_render_test.c` (its OPTIONS blocks: the options part of 361-369, 377-426, 464-514, 714-730), `tools/test_poketouch_render.py` (the OPTIONS "differs and is red" check)

**Interfaces:**
- Produces (dispatcher): `typedef struct { int (*shown)(const CtrPokeTouchView *); void (*draw)(const CtrPokeTouchView *); int (*same)(const CtrPokeTouchView *a, const CtrPokeTouchView *b); const CtrPokeTouchRect *(*back)(const CtrPokeTouchView *); } DevFieldView;` (`draw` after `DevBegin(sCanvas, v->shell)`; `back` = the BACK key as drawn now, or NULL). `static const DevFieldView kDevFieldViews[]` in MenuFrame's precedence (Easy Chat, Pokédex, card, learn, relearn, mail, choice, naming, box, SAVE, OPTIONS; the frame last in Task 8), each task adding its row. `static int DevFieldIndex(const CtrPokeTouchView *v)` (−1 none); `static int DevFieldDraw(const CtrPokeTouchView *v)` (1 when drawn); `static int DevFieldUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)` (same row, `same`, same shell → nothing; else `CtrPokeTouch_Draw` and the whole screen; 0 when neither is a dispatcher view). Public: `bool CtrPokeTouch_DeviceShown(const CtrPokeTouchView *v)` (HOME, the Bag or a dispatcher view draws `v`); `bool CtrPokeTouch_BackHit(const CtrPokeTouchView *v, int x, int y)` (a dispatcher view: `DevKeyArea(*back(v))` while `backVisible`; else the ember `gCtrPokeTouchBack`) replacing `CtrPokeTouch_HitBack`.
- Produces (kit, so every view file sees them): `UI_FN const char *DevBackWord(void)` = `CTR_TR("BACK", "ATR\xC3\x81S", "ZUR\xC3\x9C" "CK")`; `#ifndef DEV_TRACE` / `#define DEV_TRACE(op, x, y, w, h, a, b, s)` that only `(void)`s its arguments; `enum { DEV_ART_ICON, DEV_ART_PICTURE, DEV_ART_ITEM, DEV_ART_BADGE, DEV_ART_WALLPAPER, DEV_ART_MARK, DEV_ART_SPRITE, DEV_ART_FRAME, DEV_ART_LOGO, DEV_ART_GBA };` `enum { DEV_TRACE_ART = 1 };` `UI_FN void DevArt(int kind, int x, int y, int w, int h)` → `DEV_TRACE(DEV_TRACE_ART, x, y, w, h, kind, 0, NULL)`.
- Produces (shared layout): `static inline CtrPokeTouchRect DevAcross(int n, int i, int y, int h)` — key i of n (1-8) across x 8..312: g = n ≤ 3 ? 8 : 4, w = (304 − g(n − 1)) / n, x = 8 + (w + g)·i; `DevTallRow(int i, int narrow)` = {16, 48 + 24i, narrow ? 168 : 244, 24}, i 0-4, on `kDevBagList`'s paper; `kDevSmallNote {8,190,152,42}` (a screen) with `kDevSmallKey[2]` {168,190,32,40}, {204,190,32,40} and `kDevBagClose` beside it; `kDevTallScreen {8,8,304,172}`; `kDevWideKey {8,188,304,42}`; `kDevAskNote {8,156,200,76}`, `kDevAskYes {216,156,96,36}`, `kDevAskNo {216,198,96,34}` (HOME's third and sixth key places).
- Produces (OPTIONS layout): sections `DevAcross(4, s, 8, 24)`; rows `DevOptRow(i)` = {16, 48 + 18i, 244, 18}, i < `PT_OPT_MAX`; on a row LABEL x 16..135 (`DEV_OPT_LABEL_W 120`), PREV x 136..159, NEXT x 160..259, a level row's segments x 136..259 split in `count`; pages ▲ `kDevBagUp`, ▼ `kDevBagDown`; the description `kDevSmallNote`; ◀ ▶ `kDevSmallKey`; BACK `kDevBagClose`; FRAME's sample `kDevOptSample {202,138,56,40}`; the look's panel `kDevOptLook {16,122,244,54}`.
- Produces (OPTIONS API): `bool CtrPokeTouch_OptionHit(const CtrPokeTouchView *v, int x, int y, int *row, int *zone)` (gains `v`; ◀ / ▶ = row `optSelected`, zone PREV / NEXT); `int CtrPokeTouch_OptionSegment(int x, int count)` (new geometry); `int CtrPokeTouch_OptionKey(const CtrPokeTouchView *v, int x, int y)` → `PT_OPT_KEY_SECTION + s` (s < `optSectionCount`), `PT_OPT_KEY_UP`, `PT_OPT_KEY_DOWN` (only when `optPageCount > 1`), else −1, `enum { PT_OPT_KEY_SECTION = 0x10, PT_OPT_KEY_UP = 0x18, PT_OPT_KEY_DOWN = 0x19 };`. View: `int8_t optKey` (key under a held touch, −1) replaces `optPagePressed`; `char optSectionName[4][12]` (by `CtrOptions_Fill`: the shown section's = the shown page's name, the others' = their first page's). `int CtrOptions_SectionFirstPage(int section)`.

- [ ] **Step 1: Confirm the branch** — `git branch --show-current` → `feat/b-boardless-views` (the controller created it from `feat/b-party-summary-bag` and committed this plan on it; do not create it).
- [ ] **Step 2: Write the failing tests.** Layout: `Same(DevAcross(3, i, 8, 24), kDevBagPocket[i])`, `Same(DevAcross(4, i, 170, 60), kDevBatRow4[i])`, `Same(DevAcross(2, i, 72, 62), kDevBatMove[i])`; for n 1-8 the last key ends ≤ 312 and w ≥ 28; `DEV_OPT_LABEL_W ≥ (PT_OPT_LABEL_LEN − 1) * 7` and `100 − 14 ≥ (PT_OPT_VALUE_LEN − 1) * 7` (a 15-byte label and an 11-byte value fit at 7 px a glyph: no German row can cut); group {4 sections, ▲, ▼, ◀, ▶, BACK}. `poketouch_options_test.c`: `CtrOptions_SectionFirstPage(0..3)` = 0, 1, 4, 5; `CtrOptions_Fill(&view, 10)` gives `optSectionName` "Settings", "Looks", "QoL", "Nuzlocke". `test_field_presentation.py` (and its :1003 test now expects `PokeTouchHit` → `PT_NONE` while a B view is shown):

```python
def test_column_only_under_ember_views(self):
    self.assertIn('if (!sHaveShown || !CtrPokeTouch_DeviceShown(&sShown.touch))\n        ProcessRouteTouch(mode, in);', function(bottom, 'ProcessTouch'))
    self.assertIn('CtrPokeTouch_DeviceShown(&sShown.touch)', function(bottom, 'PokeTouchHit'))
    self.assertIn('CtrPokeTouch_BackHit(&sShown.touch,', function(bottom, 'ProcessBack'))
    self.assertNotIn('CtrPokeTouch_HitOptPage', bottom)
```

`options` suite (`OptBoard()`: page 0 of 6, sections "Settings" "Graphics" "QoL" "Nuzlocke", five rows — two "<>" rows, a switch, a 3-level row at 4, row 0 a 15-`W` label and an 11-`W` value — `optSelected = 1`, `backVisible = backEnabled = 1`; `OptHitV` = `OptionKey` + 0x100 if any, else `row * 16 + zone`, else −1; `BackV` = `CtrPokeTouch_BackHit`, shared by the later suites, each of which adds its own `DevHitFn` adapter as plan 2's did):

```c
v = OptBoard(); DevCheckTouch(&old, &v); assert(CtrPokeTouch_DeviceShown(&v));
for (s = 0; s < 4; ++s) AssertHits(OptHitV, &v, DevAcross(4, s, 8, 24), 0x100 + PT_OPT_KEY_SECTION + s);
AssertKey(DevAcross(4, 0, 8, 24), DEV_LOOK_KEY, 1); AssertHits(OptHitV, &v, kDevBagUp, 0x100 + PT_OPT_KEY_UP); AssertHits(OptHitV, &v, kDevBagDown, 0x100 + PT_OPT_KEY_DOWN);
for (i = 0; i < 4; ++i) { y = DevOptRow(i).y + 9;
    assert(OptHitV(&v, 16, y) == i * 16 + PT_OPT_ZONE_LABEL && OptHitV(&v, 135, y) == i * 16 + PT_OPT_ZONE_LABEL && OptHitV(&v, 136, y) == i * 16 + PT_OPT_ZONE_PREV
           && OptHitV(&v, 160, y) == i * 16 + PT_OPT_ZONE_NEXT && OptHitV(&v, 259, y) == i * 16 + PT_OPT_ZONE_NEXT && OptHitV(&v, 15, y) == -1 && OptHitV(&v, 260, y) == -1); }
for (k = 0; k < 3; ++k) assert(OptHitV(&v, 137 + 41 * k, DevOptRow(4).y + 9) == 4 * 16 + PT_OPT_ZONE_SEG + k && CtrPokeTouch_OptionSegment(137 + 41 * k, 3) == k);
AssertHits(OptHitV, &v, kDevSmallKey[0], 16 + PT_OPT_ZONE_PREV); AssertHits(OptHitV, &v, kDevSmallKey[1], 16 + PT_OPT_ZONE_NEXT); AssertHits(BackV, &v, kDevBagClose, 1);
assert(OptHitV(&v, 20, DevOptRow(5).y + 9) == -1 && at(20, DevOptRow(1).y + 9) == DEV_BEZEL);
v.optPressed = 3; v.optZone = PT_OPT_ZONE_LABEL; DevCheckTouch(&old, &v); assert(at(20, DevOptRow(3).y + 9) == DEV_BEZEL && at(20, DevOptRow(1).y + 9) != DEV_BEZEL);
v.optPressed = -1; v.optKey = PT_OPT_KEY_DOWN; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevBagDown, DEV_LOOK_KEY, 1);
v.optKey = -1; DevCheckTouch(&old, &v); v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
v.optPageCount = 1; DevCheckTouch(&old, &v); assert(CtrPokeTouch_OptionKey(&v, 294, 73) == -1 && at(294, 73) == DEV_SHELL_RED);
v.backVisible = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_BackHit(&v, 278, 210));
v = OptBoard(); v.optCount = 4; v.optLook.on = 1; DevCheckTouch(&old, &v); assert(at(200, 173) == DEV_PAPER2);
assert(sDevKeyCut == 0); puts("PASS device options");
```

- [ ] **Step 3: Run** — `python tools/dev.py test test_device_views test_field_presentation` and `python tools/dev.py test` (verify-host runs `poketouch_options_test`) → FAIL.
- [ ] **Step 4: Implement.**
  - Dispatcher and touch: as in Interfaces. `ProcessBack` asks `CtrPokeTouch_BackHit(&sShown.touch, ...)`; `ProcessTouch` runs `ProcessRouteTouch` only under `!sHaveShown || !CtrPokeTouch_DeviceShown(&sShown.touch)`; `PokeTouchHit` returns `PT_NONE` for a native screen drawn in B.
  - OPTIONS (shown `options && !viewport`): section keys `optSectionName` through `DevUpper`, sunk `DevChosen(s, optSection, optKey − PT_OPT_KEY_SECTION)`. Rows on `kDevBagList`'s screen, chosen `DevChosen(i, optSelected, optPressed)` inverted: label (`DevUpper`), a ◀ triangle in PREV, the value (`DevUpper`) centred in 160..244 and ▶ at 252; a switch's value `CTR_TR("ON","ON","AN")` / `CTR_TR("OFF","OFF","AUS")`; a level row's names side by side, the chosen one boxed. ▲▼ when `optPageCount > 1`, sunk by `optKey`. Description: `bar[1]` + `bar[2]`, small font, up to three lines (`DevBagSmallWrap`). ◀ ▶ sunk while `optPressed == optSelected` in that zone. BACK `DevBackWord()` dark (`backVisible`; sunk `backPressed`; `DevDim` when `!backEnabled`). FRAME's sample (`FrameSample`, its YES / NO in `DEV_INK`) in `kDevOptSample` when `optFrameShow`; the look's panel in `kDevOptLook` (`DEV_PAPER2`) when `optLook.on && optCount ≤ 4`: the walking picture 1:1 (`DevArt(DEV_ART_SPRITE, ...)`), its name, the credit small in three lines. `same`: the `opt*` fields, `optKey`, `optSectionName`, `bar[1..2]`, `back*`.
  - `ProcessOptionsTouch`: PAGE's press/release becomes the keys' (`sOpt.key` armed by `CtrPokeTouch_OptionKey`): UP → page `(p − 1 + n) % n`, DOWN → `(p + 1) % n`, SECTION s → `CtrOptions_SectionFirstPage(s)`, each then `sOpt.sel = CtrOptions_PageBase(page)` as PAGE did; rows, zones, steps, switches and levels as today through the new `OptionHit`.
- [ ] **Step 5: Run** — Step 3 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 6: Render** — `python tools/dev.py render options` → `options_{settings,graphics_frame,looks,qol_levels,nuzlocke,pressed_key}.png`.
- [ ] **Step 7: Azahar** — every section key, ▲▼ through all six pages, a row's three zones, ◀ ▶, a switch, a level row, FRAME's sample, a look's preview, Shell colour recolouring at once, BACK; no tap on the right side opens another screen.
- [ ] **Step 8: Commit** `feat: OPTIONS on the red device, through one field dispatcher`.

### Task 2: SAVE, the Trainer Card and the letter

**Files:**
- Create: `3ds_port/src/3ds_device_save_impl.h`, `3ds_device_card_impl.h`, `3ds_device_mail_impl.h`, `3ds_port/tests/device_suite_screens.h`
- Delete: `3ds_field_card_impl.h`, `3ds_field_mail_impl.h`; SAVE's ember code in `3ds_poketouch.c` (`SaveField` … `SaveButtons`, `sSavePanel`, `sSaveButtons`) and the SAVE / card / mail branches of `MenuFrame`, `Bar`, `Update`
- Modify: `3ds_poketouch.h` (`CtrPokeTouch_MailHit`), `3ds_bottom.c` (the letter's tap at ~4369-4372 asks `CtrPokeTouch_MailHit(&sShown.touch, x, y)` instead of `PT_VIEW_*`), `3ds_text_case_views.h`, layout header
- Test: `device_render_test.c` (`save`, `card`, `mail`), `device_layout_test.c` (SAVE {YES, NO}, {OK}; card {FLIP, BACK}; mail {CLOSE}), `test_device_views.py`, `poketouch_render_test.c` (370-376, 731-742, 581-590, the card part of 673-712), `test_poketouch_render.py` (the SAVE check)

**Interfaces:**
- Produces: SAVE = info on `kDevHomeScreen`, the dialogue (`bar[0..2]`) on `kDevAskNote`, YES `kDevAskYes` / NO `kDevAskNo`, OK (SUCCESS, ERROR) in `kDevAskYes`, no key while WRITING, no BACK. Card = `kDevTallScreen`, FLIP `kDevCardFlip {8,188,228,42}`, BACK `kDevSumBack`. Letter = `kDevTallScreen`, CLOSE `kDevWideKey` (its `back`). `CtrPokeTouch_SaveHit(int x, int y, int state)` and `CtrPokeTouch_CardHit(v, x, y)` keep their signatures (the card: 1 on its screen or FLIP); `bool CtrPokeTouch_MailHit(const CtrPokeTouchView *v, int x, int y)` (1 on the letter's screen while `mail.on`).

- [ ] **Step 1: Write the failing tests** (`save`: 8 badges, a Pokédex, 123:45, ¥999999, `saveSel` 0; `card`: front with 8 badges and 3 stars, then the back's 5 rows; `mail`: ORANGE MAIL from a writer, then none):

```c
AssertHits(SaveHitV, &v, kDevAskYes, PT_SAVE_FIRST); AssertHits(SaveHitV, &v, kDevAskNo, PT_SAVE_SECOND); AssertKey(kDevAskYes, DEV_LOOK_KEY, 1);
v.saveSel = 1; DevCheckTouch(&old, &v); AssertKey(kDevAskNo, DEV_LOOK_KEY, 1); v.savePressed = PT_SAVE_FIRST; DevCheckTouch(&old, &v); AssertKey(kDevAskYes, DEV_LOOK_KEY, 1);
v.saveState = SAVE_WRITING; DevCheckTouch(&old, &v); assert(CtrPokeTouch_SaveHit(260, 170, SAVE_WRITING) == PT_SAVE_NONE && at(260, 175) == DEV_SHELL_RED);
v.saveState = SAVE_SUCCESS; DevCheckTouch(&old, &v); AssertHits(SaveHitV, &v, kDevAskYes, PT_SAVE_FIRST); assert(CtrPokeTouch_SaveHit(260, 215, SAVE_SUCCESS) == PT_SAVE_NONE);
v.saveMinutes++; assert(DevWhole(DevCheckTouch(&old, &v))); v.mapGroup ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
AssertHits(CardHitV, &cv, kDevCardFlip, 1); assert(CtrPokeTouch_CardHit(&cv, 160, 90) == 1 && CtrPokeTouch_CardHit(&cv, 240, 200) == 0); AssertHits(BackV, &cv, kDevSumBack, 1);
cv.card.pressed = 1; DevCheckTouch(&co, &cv); AssertKey(kDevCardFlip, DEV_LOOK_KEY, 1); cv.card.back = 1; cv.card.pressed = 0; assert(DevWhole(DevCheckTouch(&co, &cv)));
assert(CtrPokeTouch_MailHit(&mv, 160, 90) && !CtrPokeTouch_MailHit(&mv, 160, 200)); AssertHits(BackV, &mv, kDevWideKey, 1); mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv);
assert(sDevKeyCut == 0); puts("PASS device screens");
```

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement.**
  - SAVE (`save && !viewport`): rows PLAYER, BADGES, POKéDEX, TIME, MONEY (`CTR_TR` upper: "JUGADOR"/"SPIELER", "MEDALLAS"/"ORDEN", "POK\xC3\xA9" "DEX", "TIEMPO"/"ZEIT", "DINERO"/"GELD"): small label, value right-aligned (name as typed, "---" without a Pokédex, v0.6.0's caps); the player's map icon (HOME's `sPlayer`) at 4x in {224,46,64,64} (`DevArt(DEV_ART_SPRITE, ...)`). The dialogue: `bar[0]` normal, `bar[1..2]` normal in `DEV_INK2`. Keys `UIS_YES`/`UIS_NO`/`UIS_OK` through `DevUpper`, sunk `DevChosen(k, saveSel, savePressed − 1)`. `same`: `save*`, `playerName`, `playerFemale`, `bar`.
  - Card (`card.on && !viewport`): front — NAME (`label[0]` without ": ") and the name as typed, IDNo. and the ID on the first line; MONEY, POKéDEX (when `hasDex`), TIME rows; the trainer's picture {226,40,64,64}; up to five stars (the port's star bitmap from `FCStar`, gold `UI_RGB(0xE8,0xB8,0x30)`) from (22,108); eight badge places {20 + 34i, 132, 32, 32}: the badge at 2x (`DevArt(DEV_ART_BADGE, ...)`) or a `DEV_PAPER2` rounded square. Back — the name, a `DEV_INK_SH` rule, the records (label small, value right). Labels through `DevUpper`. FLIP `CTR_TR("FLIP","GIRAR","UMDREHEN")` sunk while `card.pressed`; BACK `DevBackWord()` dark.
  - Letter (`mail.on && !viewport`): the item's icon 1:1 centred near the top (`DevBagIcon`, `DevArt(DEV_ART_ITEM, ...)`), the name (`DevUpper`) centred, a rule, `CTR_TR("FROM","DE","VON")` small and the writer as typed (left out when empty); CLOSE `DevBagCloseWord()` dark.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render save card mail` → `save_{confirm,no,overwrite,writing,saved,error}`, `card_{front,pressed,back}`, `mail_{from,bare}`.
- [ ] **Step 6: Azahar** — SAVE by touch and D-pad through writing and saved; the card: FLIP, a tap on it, BACK from both sides; a letter from the mailbox closes by a tap on it and by CLOSE.
- [ ] **Step 7: Commit** `feat: SAVE, the Trainer Card and the letter on the red device`.

### Task 3: The Pokédex

**Files:**
- Create: `3ds_port/src/3ds_device_dex_impl.h` (defines `CtrPokeTouch_DexHit`, `CtrPokeTouch_DexTabRow`, `CtrPokeTouch_DexGridPageRow`; holds, moved unchanged, `FDTiles`, `FDPal`, `FDPicSlot`, `FDPicDraw`, `FDPicDrawBig`, still called by the ember learn, relearn, naming and box until Tasks 4-6), `3ds_port/tests/device_suite_dex.h`
- Delete: `3ds_field_dex_impl.h`; `sFBArea` (`3ds_field_bag_impl.h`, its last user)
- Modify: `3ds_bottom.c` (`LIST_DRAG_ROW 26` → `24`, the B list's row), `3ds_text_case_views.h`, layout header
- Test: `device_render_test.c` (`dex`), `device_layout_test.c` (MENU {3 tabs, 6 items, ▲, ▼, CLOSE}; LIST {▲, ▼, BACK}; CATEGORY {4 slots, ◀, ▶, BACK}; PAGE {AREA, CRY, BACK}; added {OK}), `test_device_views.py`, `poketouch_render_test.c` (the dex part of 673-712)

**Interfaces:**
- Produces: MENU — tabs `kDevBagPocket[0..2]` (LISTS, HABITATS, SEARCH); items `DevDexItem(i)` = `DevAcross(3, i % 3, 40 + 72(i / 3), 68)`, six to a grid page; `kDevSmallNote` (SEEN, OWNED), ▲ ▼ `kDevSmallKey` (grid pages), CLOSE `kDevBagClose` (= FireRed's CANCEL row: `back` NULL in MENU). LIST — `kDevBagTitle`, `kDevBagList` + `DevTallRow(i, 0)`, `kDevBagUp/Down`, `kDevBagDesc`, BACK `kDevBagClose`. CATEGORY — `kDevBagTitle`, `DevDexSlot(i)` = {8 + 156(i % 2), 40 + 74(i / 2), 148, 70}, ◀ ▶ `kDevSmallKey`, `kDevSmallNote`, BACK `kDevBagClose`. PAGE — `kDevTallScreen` (picture {20,20,80,80}), AREA `kDevDexArea {8,188,110,42}`, CRY `kDevDexCry {126,188,110,42}`, BACK `kDevSumBack`; added — OK in `kDevWideKey`, the screen CONTINUE too, no BACK. Codes and `DexTabRow`/`DexGridPageRow` as today.

- [ ] **Step 1: Write the failing tests** (`dex`: MENU on HABITATS, 9 habitats, the 6th locked, the cursor on the 2nd; LIST of 151 from `listFirst = 0`, row 2 seen-not-caught, `richArt` on; CATEGORY of 3 with `canNext`; PAGE caught, then seen-only; added; `ItemRow`/`CancelRow` = the suite's reading of the fixture's rows):

```c
for (i = 0; i < 3; ++i) AssertHits(DexHitV, &v, kDevBagPocket[i], PT_DEX_HIT_TAB + i); AssertKey(kDevBagPocket[1], DEV_LOOK_KEY, 1);
for (i = 0; i < 6; ++i) AssertHits(DexHitV, &v, DevDexItem(i), PT_DEX_HIT_ITEM + ItemRow(&v, i)); AssertKey(DevDexItem(1), DEV_LOOK_KEY, 1); AssertKey(DevDexItem(5), DEV_LOOK_FAINT, 0);
AssertHits(DexHitV, &v, kDevSmallKey[1], PT_DEX_HIT_DOWN); AssertHits(DexHitV, &v, kDevBagClose, PT_DEX_HIT_ITEM + CancelRow(&v)); assert(!CtrPokeTouch_BackHit(&v, 278, 210));
v.dex.state = PT_DEX_STATE_LIST; DevCheckTouch(&old, &v); for (i = 0; i < 5; ++i) AssertRowHits(DexHitV, &v, DevTallRow(i, 0), PT_DEX_HIT_ROW + i);
assert(CtrPokeTouch_DexHit(&v, 294, 73) == -1 && Count(276, 40, 36, 66, DEV_BAG_ARROW_OFF) > 0); AssertHits(DexHitV, &v, kDevBagDown, PT_DEX_HIT_DOWN); AssertHits(BackV, &v, kDevBagClose, 1);
r0 = DevTallRow(0, 0); r2 = DevTallRow(2, 0); assert(Count(r0.x + r0.w - 14, r0.y, 14, r0.h, STUB_MARK) > 0 && Count(r2.x + r2.w - 14, r2.y, 14, r2.h, STUB_MARK) == 0);
v.dex.listFirst = 146; v.dex.listCursor = 150; DevCheckTouch(&old, &v); assert(CtrPokeTouch_DexHit(&v, 294, 147) == -1); AssertHits(DexHitV, &v, kDevBagUp, PT_DEX_HIT_UP);
v.dex.state = PT_DEX_STATE_CATEGORY; DevCheckTouch(&old, &v); for (i = 0; i < 3; ++i) AssertHits(DexHitV, &v, DevDexSlot(i), PT_DEX_HIT_SLOT + i);
assert(CtrPokeTouch_DexHit(&v, 238, 150) == -1 && CtrPokeTouch_DexHit(&v, 184, 210) == -1); AssertHits(DexHitV, &v, kDevSmallKey[1], PT_DEX_HIT_NEXT);
v.dex.state = PT_DEX_STATE_PAGE; DevCheckTouch(&old, &v); AssertHits(DexHitV, &v, kDevDexArea, PT_DEX_HIT_AREA); AssertHits(DexHitV, &v, kDevDexCry, PT_DEX_HIT_CRY);
v.dex.sel.flags = 1; DevCheckTouch(&old, &v); assert(Count(110, 56, 196, 14, UiTypeColor(v.dex.type[0])) == 0);        /* seen only: no type chips */
v.dex.added = 1; DevCheckTouch(&old, &v); AssertHits(DexHitV, &v, kDevWideKey, PT_DEX_HIT_CONTINUE); assert(CtrPokeTouch_DexHit(&v, 160, 90) == PT_DEX_HIT_CONTINUE && !CtrPokeTouch_BackHit(&v, 278, 209));
v.dex.national ^= 1; v.dex.category ^= 1; v.mapGroup ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
assert(sDevKeyCut == 0); puts("PASS device dex");
```

(`STUB_MARK`: the colour the harness's caught-marker stub paints with `richArt`.)
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** (`dex.on && !viewport`; BUSY draws the kept struct). MENU: tab sunk = the section holding `menuCursor` (v0.6.0's rule) or the pressed tab; the shown section's rows but headers and CANCEL, six to a page by v0.6.0's paging: the 64x48 menu icon (`DevArt(DEV_ART_ICON, ...)`) over the label (small, FireRed's name through `DevUpper`, then " POKéMON" / " MODE" stripped — fixing v0.6.0's case-sensitive strip), sunk `DevChosen(row, menuCursor, pressed − PT_DEX_HIT_ITEM)`, locked `DEV_LOOK_FAINT` (hit; the handler ignores it as today); ▲▼ only with more than one page; the note `UIS_DEX_SEEN`/`_OWNED` (upper) and the counts; CLOSE `DevBagCloseWord()` dark, sunk while `menuCursor` is on CANCEL. LIST: title = the order's word (`UIS_DEX_KANTO/NATIONAL/ATOZ/TYPE/LIGHTEST/SMALLEST`, upper; SEEN / OWNED small at its right for orders 0 and 5); rows "No.%03u", the name (`DevUpper`, "-----" unseen), the caught mark at the right (`DevArt(DEV_ART_MARK, ...)`); ▲▼ on and hit as v0.6.0's arrows (grey `DEV_BAG_ARROW_OFF` otherwise); the description: type chips and kind when caught, "?????" otherwise. CATEGORY: title = the habitat and `UIS_DEX_PAGE` "n/m"; a slot: the 64x64 picture when seen (`DevArt(DEV_ART_PICTURE, ...)`), "No." and the name ("?????" unseen), sunk `DevChosen(i, slot, pressed − PT_DEX_HIT_SLOT)`; ◀ ▶ by `canPrev`/`canNext`; the note: the slot's Pokémon. PAGE: the picture (80x80 when the pack has it, else 64x64 centred), No., name, kind, type chips (caught only), HT and WT (v0.6.0's formats), the footprint at (282,20), `desc` in five lines across 280 px; AREA / CRY (`UIS_DEX_AREA`/`_CRY`, upper). `same`: the dex struct but `national`, `category`, plus `back*`.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render dex` → `dex_{lists,habitats,habitats_page2,search,list_top,list_end,category,page,page_seen,added}`.
- [ ] **Step 6: Azahar** — each tab, a habitat page by ▼ and by drag, the list by rows, ▲▼ and drag (24 px a row), slots and ◀ ▶, a page's AREA (FireRed's picture returns) and CRY, BACK a level at a time, CLOSE from HABITATS, a catch's page and OK.
- [ ] **Step 7: Commit** `feat: the Pokedex on the red device`.

### Task 4: Field choices and YES/NO, forget a move, REMEMBER — then PR C1

**Files:**
- Create: `3ds_port/src/3ds_device_choice_impl.h`, `3ds_device_learn_impl.h`, `3ds_device_relearn_impl.h`, `3ds_port/tests/device_suite_lists.h`
- Delete: `3ds_field_choice_impl.h`, `3ds_field_learn_impl.h`, `3ds_field_relearn_impl.h`; `FBWindow` (`FBLlButton` stays for the box until Task 6)
- Modify: `3ds_poketouch.h` (`PT_RL_HIT_CANCEL = 235`; `CtrPokeTouchLearn.nameDefault`), `3ds_bottom_relearn_impl.h` (`ProcessRelearnTouch`), `3ds_bottom_field_fill_impl.h` (`FieldLearnFill` sets `nameDefault`), `3ds_text_case_views.h`, layout header
- Test: `device_render_test.c` (`choice`, `learn`, `relearn`), `device_layout_test.c` (choice n 1-8 in one column and as 2- and 3-column grids, each with BACK; learn {BACK}; relearn LIST {▲, ▼, CANCEL}, YESNO {2}), `test_device_views.py`, `tools/test_field_presentation.py` (`test_relearn_cancel_key`), `poketouch_render_test.c` (515-580, 743-751)

**Interfaces:**
- Produces (choice): `DevChoiceKey(int n, int cols, int i)`: c = cols > 0 ? cols : (n > 4 ? 2 : 1), rows = ⌈n / c⌉ ≤ 4; FireRed's one-column menu shown in two columns goes down first (col = i / rows, row = i % rows), a grid across (col = i % c, row = i / c); h = rows ≤ 3 ? 40 : 32; key = `DevAcross(c, col, 8 + (h + 4)·row, h)`. The prompt `kDevChoiceNote {8,160,228,72}`, BACK `kDevChoiceBack {244,160,68,70}`.
- Produces (learn): `kDevBagTitle` (FireRed's question), `kDevSumScreen` with `kDevSumColumn` and `kDevSumPic`, rows `DevSumRow(0..4)` (4 = the new move), `kDevSumNote`, BACK `kDevSumBack`; `uint8_t nameDefault` (the nickname is its species' own name, compared as the battle fill's `BatSameName` does).
- Produces (relearn): `kDevBagTitle`, `kDevBagList` + `DevTallRow`, `kDevBagUp/Down`, `kDevBagDesc`, CANCEL `kDevBagClose` → `PT_RL_HIT_CANCEL` (FireRed's CANCEL row, no longer drawn as a row; `ProcessRelearnTouch` takes it as v0.6.0 takes that row: the cursor on it → A, else the walk plan to `count`, then A; `back` NULL); YESNO: `kDevBagListNarrow` (the question), `DevBagMenuKey(2, i)`, `kDevBagDescNarrow` (the move). `CtrPokeTouch_ChoiceHit`, `_LearnHit`, `_RelearnHit` keep their signatures and codes.

- [ ] **Step 1: Write the failing tests.** `test_relearn_cancel_key` (compiles `ProcessRelearnTouch` with the file's existing stubs): a release on `PT_RL_HIT_CANCEL` with the cursor on `count` sends A; with the cursor on 0 starts the plan with target `count`. Suites (`choice`: YES/NO, six, eight, a 3-column grid of 8, a 24-character label; `learn`: four moves and one, the cursor on the new one, then two moves; `relearn`: eight moves at `first` 0 and 4, cursor on CANCEL, YESNO, MESSAGE with and without `tap`, BLANK):

```c
for (n = 1; n <= 8; ++n) { v.choice.count = n; v.choice.cols = 0; DevCheckTouch(&old, &v); for (i = 0; i < n; ++i) AssertHits(ChoiceHitV, &v, DevChoiceKey(n, 0, i), PT_CHOICE_HIT_ROW + i); }
assert(DevChoiceKey(6, 0, 3).x == 164 && DevChoiceKey(6, 0, 3).y == 8);
v.choice.count = 8; v.choice.cols = 3; DevCheckTouch(&old, &v); AssertHits(ChoiceHitV, &v, DevChoiceKey(8, 3, 7), PT_CHOICE_HIT_ROW + 7);
AssertKey(DevChoiceKey(8, 3, v.choice.cursor), DEV_LOOK_KEY, 1); AssertHits(BackV, &v, kDevChoiceBack, 1); assert(CtrPokeTouch_ChoiceHit(&v, 160, 200) == -1);
for (i = 0; i < 5; ++i) AssertRowHits(LearnHitV, &lv, DevSumRow(i), PT_LEARN_HIT_ROW + i); assert(at(130, DevSumRow(4).y + 11) == DEV_BEZEL); AssertHits(BackV, &lv, kDevSumBack, 1);
lv.learn.row[2].move = lv.learn.row[3].move = 0; DevCheckTouch(&lo, &lv); assert(CtrPokeTouch_LearnHit(&lv, 130, DevSumRow(2).y + 5) == -1);
lv.learn.row[0].locked ^= 1; lv.mapGroup ^= 1; assert(DevCheckTouch(&lo, &lv).count == 0);
for (i = 0; i < 5; ++i) AssertRowHits(RlHitV, &rv, DevTallRow(i, 0), PT_RL_HIT_ROW + i);
assert(CtrPokeTouch_RelearnHit(&rv, 294, 73) == -1); AssertHits(RlHitV, &rv, kDevBagDown, PT_RL_HIT_DOWN); AssertHits(RlHitV, &rv, kDevBagClose, PT_RL_HIT_CANCEL);
rv.relearn.first = 4; rv.relearn.cursor = 8; DevCheckTouch(&ro, &rv); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1); assert(CtrPokeTouch_RelearnHit(&rv, 130, DevTallRow(4, 0).y + 5) == -1);
rv.relearn.panel = PT_RL_PANEL_YESNO; DevCheckTouch(&ro, &rv); AssertHits(RlHitV, &rv, DevBagMenuKey(2, 0), PT_RL_HIT_YES); AssertHits(RlHitV, &rv, DevBagMenuKey(2, 1), PT_RL_HIT_NO); assert(CtrPokeTouch_RelearnHit(&rv, 60, 60) == -1);
rv.relearn.panel = PT_RL_PANEL_MESSAGE; rv.relearn.tap = 1; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == PT_RL_HIT_MESSAGE);
rv.relearn.tap = 0; DevCheckTouch(&ro, &rv); assert(CtrPokeTouch_RelearnHit(&rv, 5, 5) == -1);
rv.relearn.panel = PT_RL_PANEL_BLANK; DevCheckTouch(&ro, &rv); for (y = 0; y < 240; y += 4) for (x = 0; x < 320; x += 4) assert(CtrPokeTouch_RelearnHit(&rv, x, y) == -1);
assert(sDevKeyCut == 0); puts("PASS device lists");
```

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_field_presentation` → FAIL.
- [ ] **Step 3: Implement.**
  - Choice (`choice.on && !viewport`): keys = FireRed's labels (`DevUpper`, `DevKeyWord`), sunk `DevChosen(i, cursor, pressed − PT_CHOICE_HIT_ROW)`; the prompt normal up to four lines, else small up to six; BACK `DevBackWord()` dark; the upper screen's hide and hold unchanged.
  - Learn (`learn.on && !viewport`): title `question`; column: `DevMonName(nickname, nameDefault, ...)`, the level small, the picture in `kDevSumPic` (`FDPicDraw`, `DevArt(DEV_ART_PICTURE, ...)`), the cursor move's POWER / ACCURACY (`UIS_MOVE_*`, upper) at its foot; rows: name, a 40-wide type chip, `pp` right; a `DEV_INK_SH` rule over row 4; chosen `DevChosen(i, cursor, pressed − PT_LEARN_HIT_ROW)` inverted; empty rows not drawn; the note: `desc` (as `DevBagRead`); BACK dark.
  - Relearn (`relearn.on && !viewport`): LIST — title `question`; rows for moves only (`move == 0` is CANCEL: not a row); chosen = the cursor's row; ▲▼ on and hit by v0.6.0's rules (▲ when `first > 0 || cursor > 0`, ▼ when `cursor < count`); the description: type chip, "POW n ACC n" small, `desc` small in three lines; CANCEL (`UIS_CANCEL`, upper) dark, sunk when `cursor == count` or pressed. YESNO — the narrow list screen shows `message` (up to nine lines), YES / NO sunk by `menuCursor`, the narrow description the cursor move; ▲▼ and CANCEL not drawn. MESSAGE — `message` on the list's screen, a ▼ when `tap`, the rest `DevDim`; anywhere hits only when `tap`. BLANK — empty screens, keys dim, nothing hit.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render choice learn relearn` → `choice_{yesno,six,eight,grid,long}`, `learn_{move,new,two}`, `relearn_{list,scroll,cancel,yesno,message,message_tap,blank}`.
- [ ] **Step 6: Azahar** — a YES/NO (a nurse), the PC's menu, a grid multichoice (the Celadon roof's drinks), BACK on each; Rare Candy at four moves: forget one, and give up by BACK; the Two Island relearner: rows, ▲▼, CANCEL by touch and D-pad, YES/NO, messages.
- [ ] **Step 7: Commit** `feat: the field's choices, the move to forget and the relearner on the red device`.
- [ ] **Step 8: PR C1** — full check (`python tools/dev.py test`, build, audit, `python tools/dev.py render` for every view so far, `python tools/dev.py run` with Tasks 1-4's Azahar lists); `git push -u origin feat/b-boardless-views`; `gh pr create --base feat/b-party-summary-bag --title "feat: OPTIONS, SAVE, the card, the Pokedex and the field's lists on the red device"`; body: the dispatcher, each view's states, the touch changes (OPTIONS' section keys and ◀▶, the relearner's CANCEL key, the Pokédex's 24 px drag), the ember tests that left, the check list; the 🤖 line and session URL.

### Task 5: The naming keyboard

**Files:**
- Create: `3ds_port/src/3ds_device_naming_impl.h` (holds, moved, `FNChar`, `FNText`, `FNSymbol`, `FNEffectArgs`, `FNSpaceMark`: FireRed's codes in the kit's ink; the box uses them until Task 6), `3ds_port/tests/device_suite_naming.h`
- Delete: `3ds_field_naming_impl.h`
- Test: `device_render_test.c` (`naming`), `device_layout_test.c` (the 5 top keys + 32 keys of 8 columns; + 24 of 6), `test_device_views.py`, `poketouch_render_test.c` (naming's part of 438-462)

**Interfaces:**
- Produces: `kDevNameScreen {8,8,304,40}`; `DevNameTop(i)` = `DevAcross(5, i, 56, 24)`: ABC (page 1), abc (page 2), 123 (page 0), DELETE, OK; `DevNameKey(int cols, int r, int c)` = `DevAcross(cols, c, 88 + 36r, 32)` (8 columns 34 wide, 6 columns 47). `CtrPokeTouch_NameHit` and its codes as today; no BACK (DELETE is B).

- [ ] **Step 1: Write the failing tests** — `NameEveryKey` (Review Focus 1; the pages as the fill gives FireRed's `sKeyboardChars`: "ABCDEF ." / "GHIJKL ," / "MNOPQRS" / "TUVWXYZ" in 8 columns with `0xff` after short rows, lower the same, symbols in 6):

```c
static void NameEveryKey(CtrPokeTouchView v)
{
    static const uint8_t page[3] = {1, 2, 0};
    for (int p = 0; p < 3; ++p) {
        NameFill(&v, page[p]); DevCheckTouch(&gNameOld, &v);
        for (int r = 0; r < 4; ++r) for (int c = 0; c < v.name.cols; ++c) {
            CtrPokeTouchRect k = DevNameKey(v.name.cols, r, c);
            if (v.name.key[r][c] == 0xff) assert(CtrPokeTouch_NameHit(&v, k.x + k.w / 2, k.y + k.h / 2) == -1 && at(k.x + 3, k.y + k.h / 2) == DEV_SHELL_RED);
            else AssertHits(NameHitV, &v, k, PT_NAME_HIT_KEY + r * PT_NAME_COLS + c);
        }
        for (int t = 0; t < 3; ++t) AssertHits(NameHitV, &v, DevNameTop(t), PT_NAME_HIT_TAB + page[t]);
        AssertHits(NameHitV, &v, DevNameTop(3), PT_NAME_HIT_DELETE); AssertHits(NameHitV, &v, DevNameTop(4), PT_NAME_HIT_OK); AssertKey(DevNameTop(p), DEV_LOOK_KEY, 1);
    }
}
```

and in the suite: the cursor on key (2, 3) sinks it alone; on DELETE (`curCol == cols`, row 1) / OK (row 2) sinks it; on the page button (row 0) the next page's key shows its LED unsunk (`at(r.x + r.w − 9, r.y + 6) == DEV_LED`, `AssertKey(r, DEV_LOOK_KEY, 0)`); a held key sinks over the cursor; `len` 0 and `maxChars` draw; a gender sign; an undrawn change presents nothing; `sDevKeyCut == 0`.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** (`name.on && !viewport`). The name's screen: the Pokémon's half-size icon when `species` (`DevMonIcon`, `DevArt(DEV_ART_ICON, ...)`), FireRed's question small in `DEV_INK2`, the slots (pitch `min(16, 248 / maxChars)`): each typed character centred, a 1 px underline `DEV_INK_SH` under the empty ones and `DEV_INK` under slot `len`, the gender sign after them. Top keys: `UIS_NAME_UPPER/LOWER/SYMBOLS` as they are ("abc" names the small letters), `UIS_NAME_DELETE` upper, `UIS_NAME_OK`; the page shown sunk (the held tab instead); DELETE / OK sunk by the cursor on FireRed's button column (rows 1, 2) or a held touch; the cursor on the page button (row 0): `DevLed` on the key of the page its A turns to, the key up. Keys: FireRed's code centred (`FNChar`), the space key's mark, sunk `DevChosen(r * 8 + c, curCol < cols ? curRow * 8 + curCol : -1, pressed − PT_NAME_HIT_KEY)`.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render naming` → `naming_{upper,lower,symbols,cursor_delete,cursor_page,full,pokemon}`.
- [ ] **Step 6: Azahar** — the player's name, a nickname after a catch, a box name: each page by key and by SELECT, keys by touch and D-pad, DELETE, OK.
- [ ] **Step 7: Commit** `feat: the naming keyboard on the red device`.

### Task 6: The PC box

**Files:**
- Create: `3ds_port/src/3ds_device_box_impl.h` (defines `CtrPokeTouch_BoxHit`; keeps the wallpaper cache), `3ds_port/tests/device_suite_box.h`
- Delete: `3ds_field_box_impl.h`, `3ds_field_bag_impl.h` (no user left), `ItemArt`/`ItemIcon`/`ICON_BOX_*` if orphaned
- Test: `device_render_test.c` (`box`), `device_layout_test.c` (BOX {◀, ▶, PARTY, CLOSE BOX}; MENU {n} n 1-7; YESNO {2}; CHOOSE {◀, ▶, name}; MARKS {6}), `test_device_views.py`, `poketouch_render_test.c` (the rest of 438-462); `storage_controller_test.c` / `test_storage_controller.py` unchanged (GBA coordinates)

**Interfaces:**
- Produces: `kDevBoxScreen {8,8,172,156}` (paper = the 160x144 wallpaper at (14,14), 1:1); `kDevBoxTitle {14,14,160,24}` (the name band, `ST_HIT_TITLE`, a row); `DevBoxCell(i)` = {22 + 24(i % 6), 38 + 24(i / 6), 24, 24}; `kDevBoxPrev {188,8,58,24}`, `kDevBoxNext {254,8,58,24}`; `kDevBoxParty {188,40,124,32}`; `kDevBoxClose {188,80,124,32}` (dark); `kDevBoxHand {188,120,124,44}` (a screen); `kDevBoxNote {8,172,304,60}`, `kDevBoxNoteNarrow {8,172,172,60}`; `DevBoxPartyRow(i)` = {16, 16 + 20i, 156, 20}, i 0-6 (6 CANCEL); `DevBoxMenuKey(int n, int i)` = {188, 8 + p·i, 124, h}, n ≤ 5: p 44, h 40, else p 32, h 28, the last dark; `kDevBoxChoose {188,40,124,72}`. `CtrPokeTouch_BoxHit` and its codes as today on this geometry; no BACK (`back` NULL): CLOSE BOX is FireRed's exit and every menu has its CANCEL.

- [ ] **Step 1: Write the failing tests** (`box`, Review Focus 3: a full box of 30, the cursor on 29, `holding` with `iconArt` on; then empty; PARTY; MENU of 7; YESNO; CHOOSE; MARKS with two on; MESSAGE; MOVE ITEMS on places 0 and 7; BUSY):

```c
for (i = 0; i < 30; ++i) AssertRowHits(BoxHitV, &v, DevBoxCell(i), i);
AssertRowHits(BoxHitV, &v, kDevBoxTitle, ST_HIT_TITLE); AssertHits(BoxHitV, &v, kDevBoxPrev, ST_HIT_PREV); AssertHits(BoxHitV, &v, kDevBoxNext, ST_HIT_NEXT);
AssertHits(BoxHitV, &v, kDevBoxParty, ST_HIT_BUTTON_PARTY); AssertHits(BoxHitV, &v, kDevBoxClose, ST_HIT_BUTTON_CLOSE);
assert(at(DevBoxCell(29).x, DevBoxCell(29).y + 12) == DEV_BEZEL && Count(188, 120, 124, 44, STUB_ICON) > 0);   /* the cursor's frame; the held icon */
assert(CtrPokeTouch_BoxHit(&v, 250, 140) == ST_HIT_NONE && !CtrPokeTouch_BackHit(&v, 278, 210));
memset(v.box.slot, 0, sizeof(v.box.slot)); v.box.holding = 0; DevCheckTouch(&old, &v); for (i = 0; i < 30; ++i) AssertRowHits(BoxHitV, &v, DevBoxCell(i), i);
v.box.area = ST_AREA_PARTY; v.box.pos = 2; DevCheckTouch(&old, &v); for (i = 0; i < 7; ++i) AssertRowHits(BoxHitV, &v, DevBoxPartyRow(i), ST_HIT_PARTY_BASE + i);
v.box.state = PT_BOX_STATE_MENU; v.box.menuCount = 7; DevCheckTouch(&old, &v); for (i = 0; i < 7; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(7, i), ST_HIT_MENU_BASE + i);
AssertKey(DevBoxMenuKey(7, 6), DEV_LOOK_DARK, 0); assert(CtrPokeTouch_BoxHit(&v, 60, 60) == ST_HIT_NONE);
v.box.state = PT_BOX_STATE_YESNO; DevCheckTouch(&old, &v); AssertHits(BoxHitV, &v, DevBoxMenuKey(2, 1), ST_HIT_MENU_BASE + 1);
v.box.state = PT_BOX_STATE_CHOOSE; DevCheckTouch(&old, &v); AssertHits(BoxHitV, &v, kDevBoxChoose, ST_HIT_TITLE); AssertHits(BoxHitV, &v, kDevBoxNext, ST_HIT_NEXT);
v.box.state = PT_BOX_STATE_MARKS; v.box.menuCount = 6; DevCheckTouch(&old, &v); for (i = 0; i < 6; ++i) AssertHits(BoxHitV, &v, DevBoxMenuKey(6, i), ST_HIT_MENU_BASE + i);
v.box.state = PT_BOX_STATE_MESSAGE; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BoxHit(&v, 5, 5) == PT_BOX_HIT_MESSAGE);
v.box.state = PT_BOX_STATE_BOX; v.box.box ^= 1; v.mapGroup ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
assert(sDevKeyCut == 0); puts("PASS device box");
```

(`STUB_ICON`: the colour the harness's Pokémon-icon stub paints with `iconArt`.)

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** (`box.on && !viewport`; BUSY draws the kept box without the cursor's frame). The box's screen: the wallpaper 1:1 (`DevArt(DEV_ART_WALLPAPER, ...)`; none: `DEV_PAPER2` with a lighter place grid); the box's name (`FNText`, as typed) on its band, the band inverted while the cursor is on the title; icons 32x32 at (cell.x − 4, cell.y − 6) clipped to the paper (`DevArt(DEV_ART_ICON, ...)`); the cursor's place framed 2 px `DEV_BEZEL`; MOVE ITEMS: a 6x6 `DEV_BLUE_INK` square at a place's lower right when its Pokémon holds an item. ◀ ▶ sunk while held; PARTY (`UIS_BOX_PARTY`) and CLOSE BOX (`UIS_BOX_CLOSE`, dark), upper, sunk by the cursor on FireRed's buttons or a held touch; the hand's screen: `UIS_BOX_IN_HAND` small and the held icon (or MOVE ITEMS' held item). The note: the cursor's Pokémon (`monText`; the nickname upper when it equals the species line), or MOVE ITEMS' item and description, or the box's name; MESSAGE: `message` and a ▼, anywhere taps. PARTY in BOX: the party rows on the box's paper (half icon, nickname, "Lv n"; row 6 `UIS_CANCEL`), the cursor's inverted. MENU / YESNO / MARKS: the box's screen and the note `DevDim`, the narrow note, the right column's keys (menu rows `DevUpper`; YES / NO; marks at 2x in `DEV_KEY_INK` when on, `DEV_INK_DIM` off, OK, CANCEL dark) sunk by `menuCursor`. CHOOSE: ◀ ▶ and the name key (`chooseName`, "n/30" small), always sunk.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` (incl. `test_storage_controller`) → all PASS; build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render box` → `box_{full,empty,held,title,buttons,party,menu5,menu7,yesno,choose,marks,message,items}`.
- [ ] **Step 6: Azahar** — MOVE POKéMON (carry one across boxes by ◀ ▶), WITHDRAW / DEPOSIT, the box's menu (JUMP, WALLPAPER, NAME), marks, MOVE ITEMS, CLOSE BOX; note whether 24 px places miss taps.
- [ ] **Step 7: Commit** `feat: the PC box on the red device`.

### Task 7: Easy Chat

**Files:**
- Create: `3ds_port/src/3ds_device_easychat_impl.h` (defines `CtrPokeTouch_EasyHit`, `_EasyRailTarget`, `_EasyStripLetter`), `3ds_port/tests/device_suite_easychat.h`
- Delete: `3ds_field_easychat_impl.h`; `EasyCase` (`3ds_bottom_easychat_impl.h`: FireRed's words as they are)
- Modify: `3ds_poketouch.c` (the `ec.on` branches of Draw/Update: Easy Chat is the dispatcher's first row), `3ds_port/tests/easychat_plan_test.c` (its casing assertion goes), layout header
- Test: `device_render_test.c` (`easychat`), `device_layout_test.c` (EDIT {chips n 1-9, 2 modes, 3 footer}; GROUP {21, toggle, BACK}; LETTER {26, toggle, BACK}; LIST {▲, ▼, BACK, toggle, USE WORD}; ASK {2}), `test_device_views.py`, `test_easychat_plan.py`, `poketouch_render_test.c` (591-671)

**Interfaces:**
- Produces: EDIT — `kDevEcPhrase {8,8,304,36}`; chips `DevEcChip(int n, int i)` = `DevAcross(per, i % per, 52 + 40(i / per), 34)`, per = (n + 1) / 2; modes `DevAcross(2, i, 136, 30)`; footer `DevAcross(3, i, 178, 52)` (CLEAR, CANCEL dark, CONFIRM). GROUP — `kDevBagTitle`, `DevEcCell(i)` = `DevAcross(4, i % 4, 40 + 27(i / 4), 24)`, i < 21; LETTER — `DevEcLetter(i)` = `DevAcross(7, i − start[r], 40 + 32r, 28)`, start {0, 6, 12, 19}; both: toggle `DevAcross(2, 0, 206, 26)`, BACK `DevAcross(2, 1, 206, 26)` dark. LIST — `kDevEcListTitle {8,6,304,40}` (two lines), `kDevEcList {8,52,236,136}`, `DevEcRow(k)` = {16 + 102(k % 2), 60 + 20(k / 2), 100, 20}, k 0-11, the rail `kDevEcRail {220,58,18,124}` (on the list's paper), ▲ `kDevEcUp {252,52,28,64}`, ▼ `kDevEcDown {252,120,28,64}`, the letters `kDevEcStrip {284,52,28,136}` (letter i at (291 + 8(i / 13), 60 + 9(i % 13))), bottom `DevAcross(3, i, 196, 36)` (BACK dark, toggle, USE WORD). ASK — EDIT's phrase and chips `DevDim`, `kDevAskNote`, `kDevAskYes`, `kDevAskNo`.
- Produces: `PT_EC_HIT_*` as today; `CtrPokeTouch_EasyRailTarget(ec, y)` = v0.6.0's mapping on the rail's 124 px (thumb `max(14, 124·6 / rows)`); `CtrPokeTouch_EasyStripLetter(x, y)` = `13·((x − 290) / 8) + clamp((y − 60) / 9, 0, 12)` inside the strip, else −1.

- [ ] **Step 1: Write the failing tests** (`easychat`, Review Focus 4: EDIT 9 slots, 4 filled; GROUP of 21 and of 14; LETTER; LIST of 60 words (30 rows) at `first` 0 and 24, a 5-word list, A-Z on; ASK QUIT):

```c
for (i = 0; i < 9; ++i) AssertHits(EcHitV, &v, DevEcChip(9, i), PT_EC_HIT_CHIP + i); assert(CtrPokeTouch_EasyHit(&v, DevEcChip(9, 9).x + 3, 100) == -1);
AssertHits(EcHitV, &v, DevAcross(2, 1, 136, 30), PT_EC_HIT_MODE_ALPHA); AssertHits(EcHitV, &v, DevAcross(3, 1, 178, 52), PT_EC_HIT_CANCEL); AssertKey(DevAcross(3, 1, 178, 52), DEV_LOOK_DARK, 0);
v.ec.screen = PT_EC_GROUP; v.ec.groupCount = 21; DevCheckTouch(&old, &v); for (i = 0; i < 21; ++i) AssertHits(EcHitV, &v, DevEcCell(i), PT_EC_HIT_CELL + i);
AssertHits(EcHitV, &v, DevAcross(2, 0, 206, 26), PT_EC_HIT_CELL + PT_EC_CELL_TOGGLE); AssertHits(EcHitV, &v, DevAcross(2, 1, 206, 26), PT_EC_HIT_CELL + PT_EC_CELL_BACK);
v.ec.groupCount = 14; DevCheckTouch(&old, &v); assert(CtrPokeTouch_EasyHit(&v, DevEcCell(14).x + 3, DevEcCell(14).y + 3) == -1);
v.ec.screen = PT_EC_LETTER; v.ec.alpha = 1; DevCheckTouch(&old, &v); for (i = 0; i < 26; ++i) AssertHits(EcHitV, &v, DevEcLetter(i), PT_EC_HIT_CELL + i);
v.ec.screen = PT_EC_LIST; v.ec.alpha = 0; v.ec.listCount = 60; v.ec.first = 0; DevCheckTouch(&old, &v); for (k = 0; k < 12; ++k) AssertRowHits(EcHitV, &v, DevEcRow(k), PT_EC_HIT_ROW + k);
assert(CtrPokeTouch_EasyRailTarget(&v.ec, 58) == 0 && CtrPokeTouch_EasyRailTarget(&v.ec, 181) == 24);
assert(CtrPokeTouch_EasyStripLetter(291, 60) == 0 && CtrPokeTouch_EasyStripLetter(305, 181) == 25 && CtrPokeTouch_EasyStripLetter(283, 60) == -1);
AssertHits(EcHitV, &v, kDevEcUp, PT_EC_HIT_UP); AssertHits(EcHitV, &v, kDevEcDown, PT_EC_HIT_DOWN); AssertRowHits(EcHitV, &v, kDevEcRail, PT_EC_HIT_RAIL); AssertRowHits(EcHitV, &v, kDevEcStrip, PT_EC_HIT_STRIP);
v.ec.first = 24; DevCheckTouch(&old, &v); AssertRowHits(EcHitV, &v, DevEcRow(11), PT_EC_HIT_ROW + 11);
v.ec.listCount = 5; v.ec.first = 0; DevCheckTouch(&old, &v); assert(CtrPokeTouch_EasyHit(&v, DevEcRow(5).x + 3, DevEcRow(5).y + 3) == -1);
v.ec.alpha = 1; DevCheckTouch(&old, &v); assert(CtrPokeTouch_EasyHit(&v, 300, 100) == -1);
v.ec.screen = PT_EC_EDIT; v.ec.ask = PT_EC_ASK_QUIT; DevCheckTouch(&old, &v); AssertHits(EcHitV, &v, kDevAskYes, PT_EC_HIT_YES); AssertHits(EcHitV, &v, kDevAskNo, PT_EC_HIT_NO);
assert(CtrPokeTouch_EasyHit(&v, DevEcChip(9, 0).x + 3, 60) == -1); v.mapGroup ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
assert(sDevKeyCut == 0); puts("PASS device easychat");
```

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_easychat_plan` → FAIL.
- [ ] **Step 3: Implement** (`ec.on`, the whole screen). EDIT: `title` and `CTR_TR("WORDS %d OF %d","PALABRAS %d DE %d","W\xC3\x96RTER %d VON %d")` small; chips = the words, an empty slot a cream key with a short `DEV_INK_DIM` rule; modes `CTR_TR("BY GROUP","POR GRUPOS","NACH GRUPPEN")` / `CTR_TR("A TO Z","DE LA A A LA Z","VON A BIS Z")`, the one `wantAlpha` names sunk; footer `CTR_TR("CLEAR","BORRAR","LEEREN")`, `CTR_TR("CANCEL","CANCELAR","ABBRECHEN")`, `CTR_TR("CONFIRM","CONFIRMAR","BEST\xC3\x84TIGEN")`. GROUP / LETTER: title and count; the groups' names or A..Z; toggle (the A TO Z word / `CTR_TR("GROUPS","GRUPOS","GRUPPEN")`); BACK `DevBackWord()`. LIST: `listTitle` (+ "A-Z" small when `alpha`), then the phrase in one line (v0.6.0's preview, small); two words a row, the selected inverted; the rail a 4 px `DEV_INK_DIM` track with a `DEV_BEZEL` thumb; the strip's letters small, the lit one boxed, the strip `DevDim` and not hit under A-Z; bottom BACK, the toggle, `CTR_TR("USE WORD","PONER PALABRA","WORT SETZEN")`. ASK: `ask1` and `ask2` (or v0.6.0's QUIT line triple) on the question's screen, YES / NO sunk by `askCursor`. Keys sunk by FireRed's cursor (`cursor`, `footer`, `cell`, `askCursor`; `word` inverts its row) or `pressed`. `same`: the `ec` struct.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render easychat` → `easychat_{edit,edit_four,groups,groups14,letters,list,list_end,list_short,list_alpha,ask}`.
- [ ] **Step 6: Azahar** — a profile phrase: a slot, a group, a word (a second tap places it), ▲▼, rail and strip by drag, A TO Z, CLEAR with its YES/NO, CANCEL, CONFIRM.
- [ ] **Step 7: Commit** `feat: Easy Chat on the red device`.

### Task 8: FireRed's picture in an embedded screen, and the idle screen — then PR C2

**Files:**
- Create: `3ds_port/src/3ds_device_frame_impl.h` (the frame: the dispatcher's last row, shown for any field view no other row draws; `CtrPokeTouch_DrawIdle` with `#include "idle_logo.h"` and its comment), `3ds_port/tests/device_suite_frame.h`
- Delete: `MenuFrame`'s viewport branch (`HomeFrame`) and the old `CtrPokeTouch_DrawIdle` in `3ds_poketouch.c`; `3ds_text_case_views.h` (only the bars are left in it: port words and names the frame upper-cases), its call in `3ds_bottom.c` and its include in `device_render_test.c`
- Modify: `3ds_poketouch.h` (`PT_VIEW_X 40`, `PT_VIEW_Y 14`; the stale OPTIONS comment at :62-67; `CtrPokeTouch_DrawIdle(uint16_t *canvas, int logo, uint8_t shell)`), `3ds_bottom.c` (`Render` passes `s->touch.shell`, filled for `MODE_OFF` too so a shell change redraws the idle screen), layout header
- Test: `device_render_test.c` (`frame`, `idle`), `device_layout_test.c` (frame {BACK}; `kDevViewScreen`'s paper == the `PT_VIEW_*` rectangle), `test_device_views.py`, `poketouch_render_test.c`, `tools/test_poketouch_render.py` (the GBA-viewport baseline comparison goes: the picture moved)

**Interfaces:**
- Produces: `kDevViewScreen {34,8,252,172}` (paper {40,14,240,160} = `PT_VIEW_*`); the bar on `kDevSumNote` + BACK `kDevSumBack`, or without BACK `kDevViewNoteWide {8,188,304,44}`; idle `kDevIdleScreen {40,45,240,150}` (paper {46,51,228,138}, the 224x126 logo at (48,57)). `PT_VIEW_X/Y` stay the one place of FireRed's picture: `3ds_video.c:5735`, `CtrPokeTouch_ViewportToGba`, the storage tests and the frame use them.

- [ ] **Step 1: One constant first** — `poketouch_render_test.c:334-336, 366` use `PT_VIEW_*` instead of their literal 4 / 244 / 164 (the port's code already does); `git grep -n -E '\b244\b|x - 4|y - 4' -- 3ds_port` shows nothing else tied to (4,4); `python tools/dev.py test` → PASS; commit `refactor: FireRed's picture's place is PT_VIEW_X/Y everywhere`.
- [ ] **Step 2: Write the failing tests** (`frame`: a GBA view with a title and two hint lines, BACK; `idle`: logo off and on, each shell; `kShell[]` as the kit suite's):

```c
v.viewport = 1; v.backVisible = v.backEnabled = 1; DevCheckTouch(&old, &v); assert(CtrPokeTouch_DeviceShown(&v));
assert(at(PT_VIEW_X, PT_VIEW_Y) == 0 && at(PT_VIEW_X + PT_VIEW_W - 1, PT_VIEW_Y + PT_VIEW_H - 1) == 0 && at(PT_VIEW_X - 1, PT_VIEW_Y + 20) == DEV_BEZEL);
for (y = -1; y <= H; ++y) for (x = -1; x <= W; ++x) {                                       /* moved from poketouch_render_test */
    int in = x >= PT_VIEW_X && x < PT_VIEW_X + PT_VIEW_W && y >= PT_VIEW_Y && y < PT_VIEW_Y + PT_VIEW_H, gx, gy;
    assert(CtrPokeTouch_ViewportToGba(x, y, &gx, &gy) == in && (!in || (gx == x - PT_VIEW_X && gy == y - PT_VIEW_Y))); }
AssertHits(BackV, &v, kDevSumBack, 1); v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevSumBack, DEV_LOOK_DARK, 1);
v.backEnabled = 0; v.backPressed = 0; DevCheckTouch(&old, &v); assert(at(247, 209) == DevBlend(DEV_SHELL_RED, DEV_BEZEL, 128));
v.backVisible = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_BackHit(&v, 278, 209) && at(278, 209) != DEV_SHELL_RED);
v.viewport = 0; DevCheckTouch(&old, &v); assert(at(160, 90) == DEV_PAPER);                   /* nothing to show: an empty screen, never the embers */
v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
for (s = 0; s < 4; ++s) { CtrPokeTouch_DrawIdle(canvas, 0, s); assert(at(160, 20) == kShell[s] && at(160, 120) == DEV_BEZEL && at(4, 4) == DevBlend(kShell[s], 0, 90)); }
CtrPokeTouch_DrawIdle(canvas, 1, 0); assert(Count(48, 57, 224, 126, DEV_BEZEL) < 224 * 126);
puts("PASS device frame");
```

- [ ] **Step 3: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 4: Implement.** `PT_VIEW_X 40`, `PT_VIEW_Y 14`. Frame (four screws): the bezel round the picture, its paper black while `viewport` (`DevArt(DEV_ART_GBA, ...)`: the GPU's picture covers it), `DEV_PAPER` without one; the note `bar[0]` (`DevUpper`) and `bar[1]` + `bar[2]` small in `DEV_INK2`, two lines; BACK `DevBackWord()` dark, sunk `backPressed`, `DevDim` when `!backEnabled` (its `back` while `backVisible`). `same`: `viewport`, `bar`, `back*`. Idle: `DevBegin(canvas, shell)`, four screws, `kDevIdleScreen` with a `DEV_BEZEL` paper (the screen asleep; the logo was made over the dark ember backdrop), the logo blended over it as today (`DevGet` for the colour under; `DevArt(DEV_ART_LOGO, 48, 57, 224, 126)`). After this task no field view reaches `Frame`, `MenuFrame`, `Bar` or `Button`; Task 10 deletes them.
- [ ] **Step 5: Run** — Step 3 → PASS; `python tools/dev.py test` (incl. the storage suites) → all PASS; build OK.
- [ ] **Step 6: Render** — `python tools/dev.py render frame idle` → `frame_{back,back_pressed,back_off,wide,empty}`, `idle_{shell,logo,blue,green,grey}`.
- [ ] **Step 7: Azahar** — FireRed's picture centred in its screen (the Pokédex's AREA, a link card, the GBA Summary, a shop, an evolution) with taps inside it landing where they did; BACK; the idle screen before the title in each shell colour.
- [ ] **Step 8: Commit** `feat: FireRed's picture in an embedded screen, and the idle screen`.
- [ ] **Step 9: PR C2** — as Task 4 Step 8 for Tasks 5-8; `git push -u origin feat/b-boardless-views-2`; `gh pr create --base feat/b-boardless-views --title "feat: the naming keyboard, the PC box, Easy Chat and FireRed's picture on the red device"`.

### Task 9: "B — como ficou": boards made from what is drawn (decision 6)

**Files:**
- Branch: `git checkout -b feat/b-ember-removal feat/b-boardless-views-2`
- Create: `tools/make_device_boards.py`, `tools/test_make_device_boards.py`
- Modify: `3ds_device_kit_impl.h` (`DEV_TRACE` in `DevShell`, `DevScreen`, `DevKey`, `DevDim`, `DevHole`, `DevRow`, `DevGlyphs`, `DevChip`, `DevHpBar`, `DevTriangle`), `device_render_test.c` (the trace sink), `tools/dev.py` (`boards`), `.github/workflows/source-audit.yml` (the new test in the "B device views" step), `docs/review-manifest.json`

**Interfaces:**
- Produces: `enum { DEV_TRACE_ART = 1, DEV_TRACE_SHELL, DEV_TRACE_SCREEN, DEV_TRACE_KEY, DEV_TRACE_DIM, DEV_TRACE_HOLE, DEV_TRACE_ROW, DEV_TRACE_TEXT, DEV_TRACE_CHIP, DEV_TRACE_BAR, DEV_TRACE_TRI };` (a = look / colour / art kind, b = sunk / small / dir, s = text). With `DEVICE_TRACE=dir` the harness defines `DEV_TRACE` as a buffer append before including `3ds_poketouch.c` (`DevBegin` empties it: a state keeps its last full draw) and `shot(state)` writes `dir/<suite>_<state>.json` = `{"suite", "state", "shell": "#c23a2e", "items": [{"op": "key", "x", "y", "w", "h", "look": "key|dark|faint", "sunk"}, {"op": "text", "x", "y", "w", "font": "normal|small", "ink", "text"}, {"op": "art", "kind", ...}, ...]}` in draw order.
- Produces: `python tools/make_device_boards.py --trace DIR --out build/boards [SUITE ...]`, `board_html(trace: dict) -> str` → `build/boards/<suite>_<state>.dc.html` in the structure of `.progress/design-b/project/B*.dc.html` (`<x-dc>`, the helmet's Pixelify Sans link, a 640x480 box with the 320x240 one at `scale(2)` in the shell colour; keys as the boards' `<button>`s with the LED span when sunk; a screen's bezel and paper divs; rows; text in the font's size and ink; art a striped `repeating-linear-gradient(45deg, #ececec 0 3px, #f6f6f6 3px 6px)` box; chips, bars, triangles as the boards) and `build/boards/index.json` (`[{"file", "title": "B · <VIEW>: <state>"}]`); refuses `--out` outside `ROOT/build`. `python tools/dev.py boards [SUITE ...]` builds the harness without a pack (no ROM pixel can reach a board), runs the suites with `DEVICE_TRACE`, then the tool (default: options, save, card, mail, dex, choice, learn, relearn, naming, box, easychat, frame, idle).

- [ ] **Step 1: Write the failing tests** (`tools/test_make_device_boards.py`, Linux too):

```python
def test_rect_becomes_an_element(self):
    html = tool.board_html({'suite': 'options', 'state': 'x', 'shell': '#c23a2e', 'items': [
        {'op': 'key', 'x': 8, 'y': 8, 'w': 73, 'h': 24, 'look': 'key', 'sunk': 1},
        {'op': 'text', 'x': 20, 'y': 12, 'w': 40, 'font': 'normal', 'ink': '#3a3a3a', 'text': 'SETTINGS'},
        {'op': 'art', 'x': 20, 'y': 124, 'w': 32, 'h': 32, 'kind': 'sprite'}]})
    self.assertIn('left: 8px; top: 10px; width: 73px; height: 24px', html)          # sunk: 2 px lower, as the kit draws it
    for want in ('SETTINGS', 'repeating-linear-gradient', '#58e070'): self.assertIn(want, html)
    for banned in ('data:', 'base64', '<img', '<canvas'): self.assertNotIn(banned, html)
def test_harness_trace_matches_the_layout(self):   # harness built and run on "options" with DEVICE_TRACE=tmp
    keys = [i for i in json.loads((tmp / 'options_settings.json').read_text())['items'] if i['op'] == 'key']
    self.assertIn({'x': 8, 'y': 8, 'w': 73, 'h': 24}, [{k: i[k] for k in 'xywh'} for i in keys])
def test_out_must_be_under_build(self): ...   # SystemExit for a path outside ROOT/build
```

- [ ] **Step 2: Run** — `cd tools && python -m unittest test_make_device_boards -v` → FAIL.
- [ ] **Step 3: Implement** the trace calls, the sink, the tool, `dev.py boards`; the console build is unchanged (the default `DEV_TRACE` compiles to nothing).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK; `python tools/dev.py boards` → one `.dc.html` per state of each suite above.
- [ ] **Step 5: Hand over** — list the files under `build/boards/` and `index.json` in the report. The controller, not the implementer, publishes them (https://claude.ai/artifact/9cx85k3qavedLkTyKEsAKm, new row "B — como ficou", a pt-BR yellow legend per row).
- [ ] **Step 6: Commit** `feat: boards of the B views made from what the harness draws`.

### Task 10: The START column, the X/Y tiles and the route go (decision 3)

**Files:**
- Modify: `3ds_poketouch.c` (delete `gCtrPokeTouchRects`, `sMenuFrame`, `sBar`, `gCtrPokeTouchBack`, `Frame`, `FrameShown`, the `F*Frame`/`F*Note` declarations, `MenuFrame`, `Back`, `Bar`, `Area`, `ColumnIcon`, `Button`, `Changed`, `Mark`, `CtrPokeTouch_HitTest`, `HomeFrame`, `COL_DEPTH`, `ICON_REGISTERED`/`ICON_BIKE`, the ember tails of Draw/Update; `ColumnLabel` moves to `3ds_device_home_impl.h` as `DevHomeWord`), `3ds_poketouch.h` (those declarations; the view's `selected`, `registeredItem`, `bikeItem` — HOME keeps `enabled[]` and `pressed`; `PT_*` ids stay for HOME and `CtrPokeTouch_RequestFieldCommand`), `3ds_bottom.c` (delete `RequestDestination`, `ProcessRouteTouch` and its call, `RouteTargetMode`, `RouteAbort`, `OpenDestination`, the route part of `UpdateDestinationRoute` — what ends `sRouteHold` for `BeginSessionPending` stays as `UpdateSessionHold` — `CanRouteDestination` and the column's `enabled[]` fill (~1351-1356, ~1493-1498), `sRouteWait`, `sRouteMode`, `sRouteTries`, the route-only `ROUTE_*`, `sPendingScreen` and its readers if nothing sets it any more, `PokeTouchActivate`'s X / Y tile branch, `PokeTouchHit`'s column branch, the externs `CtrMenu_SetSwitchTarget`, `CtrMenu_TakeSwitched`, `CtrBottomSummary_RedirectExit` — recipe 0126 stays), `3ds_device_field_impl.h` (`CtrPokeTouch_BackHit` without the ember fallback)
- Delete: `tools/test_column_labels.py`, its `dev.py` SUITES entry and CI line (its invariant — each START word fits its key in EN/ES/DE — is the home suite's `sDevKeyCut == 0` in the label-fit builds; one new home assert checks the five words come from `DevHomeWord`)
- Test: `tools/test_field_presentation.py` (`test_route_and_column_gone`; its :91 and :1003 cuts follow the code), `tools/test_storage_controller.py` (its `BeginSessionPending` cut), `device_render_test.c` (the "column draws nothing on HOME/Bag" asserts at ~944-948, ~2094-2104), `poketouch_render_test.c` (the `HitTest` sweep 326-333, the column block 353-360)

- [ ] **Step 1: Write the failing test:**

```python
def test_route_and_column_gone(self):
    for name in ('ProcessRouteTouch', 'RequestDestination', 'CanRouteDestination', 'CtrPokeTouch_HitTest', 'gCtrPokeTouchRects'):
        self.assertNotIn(name, bottom + poketouch)
    frame = function(bottom, 'CtrBottom_Frame')            # X and Y keep the registered item and the Bicycle
    self.assertIn('CtrPokeTouch_RequestFieldCommand(PT_BIKE);', frame); self.assertIn('CtrPokeTouch_RequestFieldCommand(PT_REGISTERED);', frame)
```

- [ ] **Step 2: Run** — `python tools/dev.py test test_field_presentation` → FAIL.
- [ ] **Step 3: Implement** the deletions, then what the compiler reports unused, with the tests that cut the deleted text.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 5: Azahar** — HOME's six keys, each view's dark key back to HOME, X and Y in the field, Party → Summary → back, the PC; nothing blank or ember.
- [ ] **Step 6: Commit** `refactor: the START column, its X/Y tiles and the route go`.

### Task 11: The ember look deleted, its tests rewritten (decision 4, spec 5.6)

**Files:**
- Delete: `3ds_port/src/3ds_touch_skin_impl.h`, `3ds_port/src/3ds_ember2_impl.h` (with `LlHpBox`, `LlHpBar`, `LlBallTray`, `LlLeadFrame`, `LlTraySlot`, `LlTrayBar`, `LlBackDisc`, `LlSubPill`, `kEmGreen`), `3ds_port/include/ember_skin.h`, `ember_skin_bg0..3.h`, `tools/make_ember_skin.py`, `3ds_port/tests/poketouch_render_test.c`, `tools/test_poketouch_render.py`
- Modify: `3ds_poketouch.c` (includes; `LhPill`, `LhSwitch`, `LhValue`, `Under`, `ItemArt`/`ItemIcon`, then what the compiler reports; stale comments at :12-13, :331, :1538), `3ds_ui_theme_impl.h` + `3ds_ui_theme.h` (keep `UI_FN`, `UI_RGB`, `UI_TEXT_H`, `UiIn`, `UiInset`, `UiText`, `UiTextWidth`, `UiTypeColor`/`sUiType`, `UiImage`), `3ds_battle_ui_impl.h` (`BDescribeSh`, `BDescribeN`, `BPWrap`, `EmSurface`, `sUiEmber` go; `BPartyIcon`, `BItemIcon`, `BItemDraw`, the battle art stay), `battle_flat_icons.h` + `tools/make_battle_icons.py` (only if `kFlatShoeSmall` is reported unused: drop it and regenerate), `tools/dev.py` (SUITES), `.github/workflows/source-audit.yml`, `docs/review-manifest.json` (also its line 847: `battle_flat_icons.h` is `make_battle_icons.py`'s)
- Test: `device_render_test.c` (`kit` gains the old test's decoder checks that still have production code: `PokeTouch_Lz77`, `CtrPokeTouch_PackMenuIcon`), `tools/test_device_views.py`

- [ ] **Step 1: Inventory** — `git grep -n -E '\b(Em|Ll|Ts|kEm|EM_|EM2_|LL_)[A-Za-z0-9_]*' -- 3ds_port tools` lists only files this task deletes or edits; report any other hit and keep what it needs.
- [ ] **Step 2: Write the failing test** — the `kit` suite's moved decoder checks (a known LZ77 block decodes to its bytes; a synthetic pack entry gives its 12x12 icon) → FAIL until moved.
- [ ] **Step 3: Implement** the deletions and the move. Then the unused-kit check (plan 2's candidates are all used today — `DevText2` by the Bag, battle and Party, `DevPaperButton` by HOME, `DevTriangle` 2/3 by the battle and HOME, `DevLabelChip` by the battle, Party and Summary): `git grep` each kit function and delete any nothing calls.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS; build OK; the 3dsx smaller (four 154 KB backdrops gone).
- [ ] **Step 5: Render** — `python tools/dev.py render` (every view): no PNG differs from Tasks 8-9's.
- [ ] **Step 6: Commit** `refactor: the ember look is deleted`.

### Task 12: README and CHANGELOG, PR D, the owner's console check (decisions 7, 13)

**Files:**
- Modify: `README.md` (EN only; its ember mentions at ~15 and ~153 (alt texts), ~35, ~105, ~125, ~151, ~159, and the v0.5.2 list's HEAL at ~45 — say the red device, the Shell colour option and its four colours, HOME's six keys, a view's dark key back to HOME, X and Y on their buttons, HEAL / REMEMBER behind `CTR_PARTY_EXTRAS` (recipe 0264); screenshots stay: new ones are ROM-derived renders, the owner's call), `CHANGELOG.md` (`## Unreleased`: `### Changed` — every lower-screen view drawn as the red device; OPTIONS' Shell colour row (red, blue, green, grey) replaces EMBERS; HEAL and REMEMBER leave the Party's menu behind `CTR_PARTY_EXTRAS` (recipe 0264); the Summary's nature marks (recipe 0265); `### Removed` gains the ember look, the START column and its in-view jumps (leave a view, then HOME), the on-screen X/Y tiles (the buttons still work))

- [ ] **Step 1: Write** both files; `git grep -n -i -E 'ember|start column|X and Y hold' README.md CHANGELOG.md` shows only history (v0.6.0 and older) and the project's name.
- [ ] **Step 2: Commit** `docs: the red device in the README and the changelog`.
- [ ] **Step 3: PR D** — full check (`python tools/dev.py test`, build, audit, `python tools/dev.py render`, `python tools/dev.py boards`, `python tools/dev.py run`); `git push -u origin feat/b-ember-removal`; `gh pr create --base feat/b-boardless-views-2 --title "feat: the ember look removed, boards of the B views, docs"`; body: the deletions (files, bytes), the tests rewritten and what each kept, the boards tool, the docs, the owner's checklist below; the 🤖 line and session URL.
- [ ] **Step 4: Owner's console check** (spec 5.9, end of 5.6) — the owner runs PR D on the 3DS: OPTIONS (sections, ▲▼, ◀▶, a row's zones, Shell colour), SAVE, the card, a letter, the Pokédex (tabs, list ends, a habitat, a page, AREA, CRY), a YES/NO and a grid choice, forget-a-move, the relearner, naming (three pages), the PC box (carry across boxes, menus, marks, MOVE ITEMS), Easy Chat (rail, strip, a phrase), FireRed's picture in its screen (taps in it), the idle screen, X and Y; nothing ember, nothing blank.

## Not in this plan

- Plan 4 (speed, spec 5.7). Costs noted for it: every B field view redraws and presents the whole screen on any drawn change (each cursor step in the Pokédex, a list, the keyboard, the box, Easy Chat); Easy Chat's rail and strip scrubs redraw once per row passed; SAVE's clock once a minute; the wallpaper decodes once per box. Plan 2's perf minors stay there.
- The Rare Candy level-up card, the battle's targets and the Safari menu: already B (plan 2 Tasks 5-6: `BUI_PTY_LEVELUP`, `BUI_TARGET`, `BUI_SAFARI`).
- Owner checks still open in PRs #8/#9; README screenshots; the canvas publication (the controller's, after Task 9).
- Plan 2's deferred minors already done on `feat/b-party-summary-bag` (checked 2026-11-17): the duplicate SHOP/ITEMPC/MAILBOX branch, `DevBagKeys`' `kRow`, `party_grid_test.c:58`, the idle_logo comment (Task 8 writes its new one); `LlHpBar` goes in Task 11.
