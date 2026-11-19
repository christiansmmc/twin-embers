# Rework 2, plan 04b: the Definitiva system views, naming, the PC box and Easy Chat — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rows 3-5 of the canvas row "B — Definitiva" (OPTIONS, SAVE, the Trainer Card; the Pokédex, the letter, FireRed's picture, the idle screen; the naming keyboard, the PC box and its menus, Easy Chat) are drawn as their Definitiva boards on plan 04a's grid and kit, with the PC box's game-side recipe 0268; the "como ficou" boards are made again and the README, CHANGELOG and a console checklist describe rows 1-5.

**Architecture:** Plan 04a has landed (the grid's names `kDevGrid*`, `DevGridTab/Row/ColumnKey`; the kit rules and helpers `DevSmallFit`, `DevReadText`, `DevColumnRow`, `DevTypeChip`, `DevArrowKeys`, `DevCloseWord`, `DevCancelWord`, `DEV_RULE`, `DEV_TITLE_INK`; rows 1-2). This plan adds the kit pieces rows 3-5 need (Task 1), then one task per view or state group in the canvas's order, each moving only its own rects and helpers; fills, hit codes and handlers stay unless Interfaces change them. Then an optional trace of rows 1-2, the boards, the docs.

**Tech Stack:** C11 (devkitARM `-std=gnu11`, host gcc `-std=c11`, both `-Werror`), Python 3 tools, the pokefirered edit recipes (`tools/make_recipe.py`).

**Spec:** the canvas row "B — Definitiva": rules and row notes `.progress/design-b/def/NOTES.md` (def_spec, def_row3-5); boards dumped one element per line in `.progress/design-b/def-spec/def_*.txt` (screen px; a text box's capitals start +3 in the 11 px font, +2 in the 9 px one). Binding decisions: `.progress/lower-screen-rework2-definitive.md` § Decisions. Per-view differences: `.progress/rework2-04-recon-C.md` (row 3), `-D.md` (row 4; § 2 the kit), `-E.md` (row 5). Plan 04a (`docs/superpowers/plans/2026-11-18-rework2-04a-definitive-kit-field-battle.md`, with its pre-flight rulings: the kit this plan consumes), plan 3 and its ledger hold where nothing here overrides them.

## Calls made while drafting (for the controller)

1. Recipe **0268** `box-menu-reports-cancel` (Task 13, `src/pokemon_storage_system_tasks.c` only), against a `build/dev` holding 0266 and 0267.
2. Built on 04a as amended by its pre-flight rulings: `DevOnKey` (the kit's key-area hit test), the Pokédex's tab rect `DevDexTabKey` (deleted here for `DevGridTab(3, i)`; the drawer's `DevDexTab` keeps its name), `DevBagShown` staying in the Bag file (the frame's predicate), Easy Chat's non-static `kButton`, SAVE as the grid rule's named `kDevHomeScreen` borrower until Task 4.
3. Rows 3-5's kit pieces are one first task (Task 1), so every view task only consumes.
4. A small note of up to three lines on a 30 px footer paper (OPTIONS, the letter) has capitals 198 / 208 / 218: one row above the boards' 199 so a third (DE) line ends on the paper, as 04a's Bag description.
5. OPTIONS: the page line says PAGE through the existing `UIS_DEX_PAGE`; `·` is FireRed's 0xAF; the look's credit two small lines at most; the levels' box rule, small-font fallback and hit split as Task 3 (`CtrPokeTouch_OptionSegment` goes).
6. SAVE: the place on `bar[0]`, the dialogue in two halves on `bar[1]` / `bar[2]` (`PT_BAR_LEN` 40 cannot hold the ES overwrite sentence), one normal line when it fits 280 px, else two small lines; the bar's fill is the port's steps taken of two (`saveStep`): the screen shown during the blocking write has it half full.
7. The card's stars and colours by FireRed's rule (0-4 stars, five colours); the board's 45 / 30 / 35 % blends as alphas 115 / 77 / 90; NAME's value at x 56 and IDNo. at x 206 are fixed columns.
8. Pokédex: the habitat page's slots on the menu's rows (66 tall); the entry four lines at most, a fifth counted; the new catch's OK sunk while `pressed == PT_DEX_HIT_CONTINUE`.
9. FireRed's picture's note title at capitals 198, not the board's 197 (an accent stays off the bezel).
10. Naming: the name's column stays at the board's x with or without an icon (the player's, rival's, PC's icons come later, O4: nothing moves then); the Pokémon's icon 1:1 clipped to 24x24.
11. PC box: the wallpaper code, its declaration and its harness stub go; recipe 0158 stays inert (plan 3's 0126 precedent); `PT_BOX_MENU_MAX` stays 7, the column holds five keys, the harness's `menu7` becomes FireRed's six. PARTY POKéMON is a port word (`UIS_BOX_PARTY`, `UIS_BOX_CLOSE` go); "ITEM <item>" / "ITEM NONE"; an empty hand shows IN HAND only; the pointed Pokémon's types are filled port-side (`CtrBottomDex_Types`: none for an egg or one not caught); chips 10 tall as the board.
12. PC box states without a board (Tasks 13-14): the Party in six 24 px rows with FireRed's CANCEL in the exit corner; the chooser's key in the hand's place; MESSAGE greys the cream keys beige and dims CLOSE; icons at half (not `DevDim`) in every state but BOX; a menu without FireRed's CANCEL row (the wallpaper menus) sends B from the exit corner; YES / NO two tall keys in the column and BACK (B = NO).
13. Easy Chat: the list's ▲▼ grey and not hit at the window's ends (the legend; plan 3 kept them taken); the question (no board) keeps plan 3's note and keys; the groups' ▲▼ walk FireRed's cursor one row of three, the window following it; the letters' footer says CHOOSE A LETTER.
14. Task 18 (boards of HOME, the battle, the Party, the Summary, the Bag) is optional; skipping it changes no other task. The boards are made again by the controller after any fix wave (plan 3's ruling). README and CHANGELOG cover rows 1-5 (04a left its docs here); the console checklist goes in PR F2's body and in `docs/HARDWARE-ACCEPTANCE.md`.

## Global Constraints

Plan 04a's Global Constraints hold unchanged; in short:

- Commits: English Conventional Commits ending `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`; PR bodies end `🤖 Generated with [Claude Code](https://claude.com/claude-code)`. No force-push (stacked branches are updated by merging). Files added by name; never `.dev.local.json`, `.progress/`, `.superpowers/`, `build/`.
- Before every commit: `python tools/manifest_update.py && python tools/source_audit.py && python tools/source_audit.py --history-since 0afd7a2bdb75a7d1ec2d4d10fb9a84f6f23e08ab` → exit 0; every file ≤ 262,144 bytes; `3ds_video.c` (~258.7 KB) not edited; `device_render_test.c` kept under ~200 KB (suites live in `device_suite_<view>.h`). Nothing ROM-derived committed.
- `-Werror`: what a task orphans (rect, static function, colour, view field, declaration) it deletes; `device_render_test.c` includes all of `3ds_poketouch.c` and compiles at every commit.
- The legend: a cream key is chosen (sunk 2 px, LED 4x4 at (x + w − 11, faceY + 4)); one reads an embedded screen; one leaves by the dark key in the exit corner (BACK a level, CLOSE an app, CANCEL an action); an unused cream key is beige (`DEV_LOOK_FAINT`, ink `#8A8478`), a disabled dark exit key dimmed; a lone OK up, sunk only under a touch; four screws; list rows 24 tall, 26 apart, five shown; ▲▼ grey (`DEV_ARROW_OFF`) at the ends and then not hit.
- The grid: TOPO y 8-32 (`DevGridTab`, `kDevGridTitle`), MEIO y 40-184 (`kDevGridScreen` + `kDevGridUp`/`Down`, `kDevGridWide`, or a view's own screen), RODAPÉ y 190-232 (`kDevGridNote` + `kDevGridExit`, or `kDevGridNoteWide`); the card and the Pokédex page keep `kDevTallBack {244,188,68,42}`. Every drawn or hit rect is the layout header's, no view borrows another's (`test_views_borrow_no_view_rects`); keys hit on `DevKeyArea`, rows and screens on themselves.
- Words: port words UPPER CASE in all three `CTR_TR` languages with the board's wording; FireRed's strings as FireRed has them (`DevUpper` where they arrive title-cased); typed names as typed; key words in the normal font; exits `DevBackWord()`, `DevCloseWord()`, `DevCancelWord()`; FireRed's type colours.
- Redraw: a view draws whole when a field it draws changes; `Update` == `Draw`; an undrawn field's change presents nothing (`v.mapGroup ^= 1; v.speed ^= 1;` and the view's own).
- Keys ≥ 28x20, a state's keys apart with their drops (layout groups); every suite ends `assert(sDevKeyCut == 0)` and passes the ES / DE label-fit builds. Touch keeps its codes, handlers and timing unless Interfaces say otherwise; every moved key and row is pinned by `AssertHits` / `AssertRowHits`. Art through `DevArt`, fills through `DevRect`, discs through `DevSpot` / `DevPaperHole`.
- **The task close** (every task): `python tools/dev.py build`; `python tools/dev.py test` (all PASS; it also runs `verify-host`, where `poketouch_options_test` lives; one python suite: `python tools/dev.py test <module>`); the audit line; sizes; `python tools/dev.py render <views>` → read each PNG beside its board's def-spec lines and fix what the task decided that is off by more than a pixel; `python tools/dev.py run` (builds, copies to Azahar, starts it) and the task's Azahar check; commit.
- PRs: **F1** `feat/b-def-system-views` (base 04a's E2 `feat/b-def-battle-questions`) = Tasks 1-10 (rows 3-4); **F2** `feat/b-def-naming-box-easychat` (base F1) = Tasks 11-20 (row 5, boards, docs).

## Review Focus

1. The PC box's menus with and without FireRed's CANCEL row (the Pokémon's menu of six, the box title's menu, a wallpaper menu of four, the marks, YES / NO): the exit key hits CANCEL's row or sends B, no row is lost or hit twice, FireRed's cursor sinks the right key — `box` suite's menu blocks and `test_box_menu_reports_cancel` (Task 13).
2. OPTIONS' rows of levels in DE and ES (COMPANION's three words, EXP SHARE's): every column 136-259 of the row hits a level, split at the gaps' middles, a too-wide set drawn small and counted — `options` suite's levels block (Task 3).
3. Easy Chat's groups with 21 groups and with 14: the window follows FireRed's cursor, ▲▼ grey and not hit at the ends, a tap on a shown cell walks to its group (`cell − 3·first`), BACK sends B — `easychat` suite's groups block and `easychat_plan_test`'s `test_group_window` (Task 17).
4. SAVE with an overwrite and an error in ES and DE: the question in one line or two small ones, the keys never move between states, OK wide and up, ← → ↑ ↓ toggling YES / NO — `save` suite (Task 4).
5. The Pokédex list at both ends with five 24 px rows: the fifth row hit, the 2 px gaps hit nothing, a drag one row per 26 px, ▼ grey and not hit at the end — `dex` suite and `test_dex_list_drags_a_row_per_grid_row` (Task 7).

## Decisions applied (binding; where)

O3 PC box plain paper, screen 176 tall, places at y 35 — Task 12. O4 recipe 0268, naming icons later — Tasks 11, 13. Coordinator rulings: Easy Chat footer CLEAR / CONFIRM / CANCEL — Task 15; lone OK up — Tasks 4, 8; SAVE bar of the port's steps, error OK wide — Task 4; OPTIONS ▲▼ walk the six pages, sample at [160,140,56,36], word-sized level boxes split at the gaps — Tasks 2-3; mail note "…UPPER SCREEN. TAP HERE OR CLOSE." and a tap on it closes — Task 9; unused cream keys beige, a disabled dark exit dimmed — Tasks 4, 14; Pokédex habitat page kept on the grid, ▲▼ grey on one-page tabs, entry 3 lines (a 4th allowed), REGISTERED ending at 278 — Tasks 6-8; box YES / NO like def_choice_yesno — Task 13; Easy Chat's letter strip and the groups' A TO Z key dropped, the letters screen keeps FireRed's keys with the groups' footer — Tasks 16-17; screws, LED, beige ink, DE ABBRECHEN, type colours, key words' font — 04a, consumed everywhere.

---

### Task 1: The kit's pieces for rows 3-5

**Files:**
- Modify: `3ds_port/src/3ds_device_kit_impl.h`, `3ds_port/src/3ds_poketouch.c` (`LatinCode`), `tools/make_device_boards.py`
- Test: `device_render_test.c` (`kit`; the trace sink's `kOp` and its DISC line), `tools/test_make_device_boards.py`

**Interfaces:**
- Consumes: 04a's kit.
- Produces:
  - Colours: `DEV_PAPER_LINE UI_RGB(0xC8, 0xC8, 0xC0)` (an off page dot, a paper hole's rim, a name slot's underline); `DEV_PAPER_HOLE UI_RGB(0xE2, 0xE2, 0xDA)` (a badge not earned); `DEV_MARK_ON UI_RGB(0x3E, 0x4C, 0x6B)` (the page shown's dot, the next name slot's underline).
  - `DEV_TRACE_DISC` after `DEV_TRACE_TRI` (a its colour, b its 2 px top inset's colour, `DEV_WHITE` none); the harness's `kOp` gains `"disc"` with `"fill"` and, unless none, `"rim"`. `UI_FN void DevSpot(int x, int y, int s, uint16_t c)` — `DevDisc` told as a DISC. `UI_FN void DevPaperHole(int x, int y, int s)` — `DevDisc(x, y, s, DEV_PAPER_LINE)`, then the pixels of the same disc at (x, y + 2) that lie inside the first in `DEV_PAPER_HOLE`; one DISC (fill `DEV_PAPER_HOLE`, rim `DEV_PAPER_LINE`). The boards tool's `disc` → a span with `border-radius: 50%`, the fill, and `box-shadow: inset 0 2px 0 {rim}` when it has one.
  - `UI_FN void DevBar(int x, int y, int w, int h, int fill, uint16_t colour)` — `DevHpBar`'s 1 px `DEV_HP_FRAME` frame and `DEV_HP_TRACK` track, `fill` columns (clamped 0 … w − 2) of `colour`, told as BAR; `DevHpBar` becomes its fill rule + `DevBar(…, DevHpColour(hp, max))` (same pixels).
  - `LatinCode`: `case 0xb7: return 0xaf;   /* · */`.
  - `DevWrapRun` gains `int pitch` before `draw` (its lines `pitch` apart); `DevWrapFirst`, `DevWrapAfter`, `DevWrap`, `DevWrapLines` pass `DEV_LINE_H`; `UI_FN int DevWrapPitch(const char *s, int x, int top, int width, int maxLines, int pitch, uint16_t ink, uint16_t shadow)` (returns the lines drawn).
  - `UI_FN void DevTitleLine(const CtrPokeTouchRect *screen, const char *left, const char *right)` — `DevScreen(screen)`; `right` small `DEV_INK2` ending at paper.x + paper.w − 8, capitals paper.y + (paper.h − 8) / 2 + 1; `left` normal `DEV_INK` over `DEV_INK_SH` from paper.x + 6, capitals paper.y + (paper.h − 8) / 2, cut with "..." 8 px before `right` (not counted: FireRed's text or a phrase).
  - `UI_FN void DevKeySmallWord(const CtrPokeTouchRect *r, int top, int look, int sunk, const char *s)` — `s` in the small font centred on the key, capitals at `top`, in `DevKeyInk(look, sunk)` over `DevKeyInkShadow(look, sunk)` (`DevGlyphs(…, small 1)`), cut at r->x + 4 … r->x + r->w − 4 and counted in `sDevKeyCut` when wider than r->w − 8.

- [ ] **Step 0: Branch** — `git checkout -b feat/b-def-system-views` from 04a's last tip (`feat/b-def-battle-questions`, PR E2 open); Tasks 1-10 commit on it.
- [ ] **Step 1: Write the failing tests.** `kit` suite:

```c
DevBegin(canvas, 0); DevShell();
DevSpot(226, 164, 5, DEV_MARK_ON); assert(at(228, 166) == DEV_MARK_ON && at(226, 164) != DEV_MARK_ON);    /* a disc: no corner */
DevPaperHole(125, 134, 32); assert(at(141, 150) == DEV_PAPER_HOLE && at(141, 135) == DEV_PAPER_LINE && at(125, 134) != DEV_PAPER_HOLE);
DevBar(196, 122, 100, 6, 49, DEV_EXP);
assert(at(196, 122) == DEV_HP_FRAME && at(197, 124) == DEV_EXP && at(245, 124) == DEV_EXP && at(246, 124) == DEV_HP_TRACK);
DevSmallText("\xC2\xB7", 20, 20, DEV_INK, 60); assert(Count(20, 20, 8, 7, DEV_INK) > 0);                  /* · is a glyph */
assert(DevWrapPitch(kLongWords, 20, 100, 120, 3, 14, DEV_INK, DEV_INK_SH) == 3 && Count(20, 128, 120, 9, DEV_INK) > 0);
CtrPokeTouchRect t = {8, 6, 304, 28}, k = {8, 40, 96, 66};
DevTitleLine(&t, "KANTO", "SEEN 1"); assert(Count(280, 17, 19, 7, DEV_INK2) > 0 && Count(299, 12, 7, 16, DEV_INK2) == 0 && Count(20, 16, 30, 9, DEV_INK) > 0);
DevKey(&k, DEV_LOOK_KEY, 0, 6); DevKeySmallWord(&k, 96, DEV_LOOK_KEY, 0, "FOREST"); assert(Count(9, 96, 94, 7, DEV_KEY_INK) > 0);
assert(sDevKeyCut == 0);
```

`test_make_device_boards.py`: `test_every_piece_has_an_element` gains `{'op': 'disc', 'x': 125, 'y': 134, 'w': 32, 'h': 32, 'fill': '#e2e2da', 'rim': '#c8c8c0'}` → its element holds `border-radius: 50%` and `inset 0 2px 0 #c8c8c0`; a disc without `rim` has no `box-shadow`.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_make_device_boards` → FAIL.
- [ ] **Step 3: Implement** per Interfaces.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS (no view's pixels change: `DevHpBar` and `DevWrap*` draw as before).
- [ ] **Step 5: Task close** — `python tools/dev.py render`: no PNG changes; Azahar: HOME's and a battle's HP bars. Commit `feat: the kit's discs, progress bar, title line and small key words`.

### Task 2: OPTIONS' sections keep their names; the page line

**Files:**
- Modify: `3ds_port/include/3ds_poketouch.h` (view fields; the comment at ~434), `3ds_port/src/3ds_poketouch_options.c` (`CtrOptions_Fill`), `3ds_device_layout.h` (`DevOptRow`, `kDevOptSample`, `kDevOptLook`, `DevOptDot`, `DEV_OPT_PAGE_Y`, `DEV_OPT_VALUE_X/W`; delete `DevOptSection`), `3ds_device_options_impl.h`
- Test: `3ds_port/tests/poketouch_options_test.c`, `device_suite_options.h`, `device_layout_test.c` (`OptionsFaults`, the OPTIONS group)

**Interfaces:**
- Consumes: `DevGridTab`, `kDevGridScreen`, `kDevGridUp/Down`, `kDevGridExit`, `kDevSmallNote`, `kDevSmallKey`, `DevArrowKeys`, `DevSmallFit`, `DevSpot`, `DEV_MARK_ON`, `DEV_PAPER_LINE`, `DEV_SMALL_LINE`, `DEV_SMALL_LINE_TIGHT`.
- Produces:
  - View: `uint8_t optSectionPage, optSectionPages;` (the page shown within its section, 1-based, and the section's pages) and `char optPageName[PT_OPT_SECTION_LEN];` (the page shown's own name: Graphics, Looks, Wilds…). `CtrOptions_Fill`: `optSectionName[s]` = `CtrOptions_PageName(CtrOptions_SectionFirstPage(s))` for every section (no key is renamed by its page); `optSectionPage = CtrOptions_SectionPage(page, &of)`, `optSectionPages = of`, `optPageName` = `CtrOptions_PageName(page)`.
  - Layout: `DevOptRow(int i)` = {16, 48 + 18i, 244, 18} written out (no `DevListRow`); sections `DevGridTab(4, s)`; `kDevOptSample {160,140,56,36}`; `kDevOptLook {16,122,244,36}`; `#define DEV_OPT_PAGE_Y 164`; `DevOptDot(int k)` = {226 + 8k, 164, 5, 5}; `#define DEV_OPT_VALUE_X 152`, `DEV_OPT_VALUE_W 96` (the value centred in 152-247); `DEV_OPT_LABEL_W`, `DEV_OPT_PREV_X`, `DEV_OPT_NEXT_X`, `DEV_OPT_NEXT_ARROW` stay.
  - Drawer: with `optSectionPages > 1` the page line `"%s %u/%u \xC2\xB7 %s"` of `DevUpper(CtrUiText(UIS_DEX_PAGE))`, the page, the pages and `DevUpper(optPageName)`, small `DEV_INK2` at (22, `DEV_OPT_PAGE_Y`); its dots `DevSpot(DevOptDot(k)…, 5, k == page − 1 ? DEV_MARK_ON : DEV_PAPER_LINE)` for k < pages. The look's panel: `DEV_PAPER2` over `kDevOptLook`, the picture at (22, 124), the name normal from x 62 capitals 128 (to 254), the credit `DevSmallFit(credit, 62, 62, 142, 254, 2, DEV_SMALL_LINE_TIGHT, DEV_INK2)`. FRAME's sample: YES capitals y + 6, NO y + 18 (x + 14). The note: `DevSmallFit(upper, 18, 18, 198, 150, 3, DEV_SMALL_LINE, DEV_INK)`. ▲▼ `DevArrowKeys(&kDevGridUp, &kDevGridDown, 1, 1, …)` (the six pages wrap, Decision). `DevOptSame` compares `optSectionPage`, `optSectionPages` and `optPageName` (while pages > 1).

- [ ] **Step 1: Write the failing tests.** `poketouch_options_test.c` (replacing the "named by its page" checks at ~205-228, ~359):

```c
CtrOptions_Fill(&view, CtrOptions_PageBase(2));                    /* the looks page */
CHECK(strcmp(view.optSectionName[1], "Graphics") == 0 && view.optSectionPage == 2 && view.optSectionPages == 3
      && strcmp(view.optPageName, "Looks") == 0, "the Graphics key keeps its name; the page is 2 of 3, Looks");
CtrOptions_Fill(&view, CtrOptions_PageBase(3));
CHECK(strcmp(view.optSectionName[1], "Graphics") == 0 && view.optSectionPage == 3 && strcmp(view.optPageName, "Wilds") == 0, "Wilds: 3 of 3");
CtrOptions_Fill(&view, 0); CHECK(view.optSectionPage == 1 && view.optSectionPages == 1, "SETTINGS: one page");
```

`options` suite: on a GRAPHICS page 1 fixture (`optSectionPage 1`, `optSectionPages 3`, `optPageName "Graphics"`):

```c
assert(Count(22, DEV_OPT_PAGE_Y, 140, 7, DEV_INK2) > 0 && at(DevOptDot(0).x + 2, DevOptDot(0).y + 2) == DEV_MARK_ON
       && at(DevOptDot(2).x + 2, DevOptDot(2).y + 2) == DEV_PAPER_LINE && Count(250, 160, 12, 12, DEV_PAPER_LINE) == 0);
v.optSectionPage = 2; assert(DevWhole(DevCheckTouch(&old, &v)));
v.optSectionPages = 1; DevCheckTouch(&old, &v); assert(Count(16, 158, 244, 18, DEV_INK2) == 0 && Count(16, 158, 244, 18, DEV_PAPER_LINE) == 0);
for (s = 0; s < 4; ++s) AssertHits(OptHitV, &v, DevGridTab(4, s), 0x100 + PT_OPT_KEY_SECTION + s);
v = OptBoard(); v.optCount = 4; v.optLook.on = 1; DevCheckTouch(&old, &v);
assert(at(200, 160) == DEV_PAPER && Count(62, 128, 190, 9, DEV_INK) > 0);               /* the panel ends at 157 */
```

(the suite's `OptPage` second-name argument and its "Looks" comment go; `Count(64, 122, 190, 54, DEV_INK)` follows the panel). Layout `OptionsFaults`: the new values; `kDevOptSample` inside the paper, under `DevOptRow(4)`, ending left of `DevOptDot(0)`; `kDevOptLook.y + kDevOptLook.h ≤ DEV_OPT_PAGE_Y − 2`; `DevOptDot(2)` inside the paper; the group "OPTIONS" = {`DevGridTab(4, s)`, `kDevGridUp`, `kDevGridDown`, `kDevSmallKey[0..1]`, `kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test` → FAIL (`verify-host`'s `poketouch_options_test`, `test_device_views`).
- [ ] **Step 3: Implement** per Interfaces.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `options_{settings,graphics_frame,looks,qol_levels,nuzlocke,pressed_key}` vs `def_opt_settings.txt`, `def_opt_graphics.txt`, `def_opt_looks.txt` (the Interfaces' rects and capitals). Azahar: OPTIONS — ▲▼ through the six pages (the tabs never rename), the page line and dots on GRAPHICS' three pages, FRAME's sample, a look's panel. Commit `feat: OPTIONS' sections keep their names; the page line and its dots`.

### Task 3: OPTIONS' levels as word-sized boxes

**Files:**
- Modify: `3ds_port/include/3ds_poketouch.h` (delete `CtrPokeTouch_OptionSegment` and its comment lines), `3ds_device_layout.h` (delete `DevOptLevel`, its comment), `3ds_device_options_impl.h`
- Test: `device_suite_options.h` (the levels block; the `DevOptLevel` / `OptionSegment` loop at ~154, ~181-184 goes)

**Interfaces:**
- Consumes: `DevOptRow`, `DevFitFont`, `DevFitWord`.
- Produces (drawer): `static int DevOptLevelSmall(const CtrPokeTouchView *v, int i)` — 1 when row i's levels in the normal font need more than 114 px (Σ (word width + 8) + 4 (n − 1)); then each box is its small word + 8 and, if those need more than 114 too, `++sDevKeyCut` when drawn. `static CtrPokeTouchRect DevOptLevelBox(const CtrPokeTouchView *v, int i, int k)` — {x, row.y + 2, word width + 8, 14}, packed right 4 apart, the last ending at row.x + row.w − 10 (250). `static int DevOptLevelAt(const CtrPokeTouchView *v, int i, int x)` — level k from the middle of the gap before it (box(k).x − 2; the first from `DEV_OPT_PREV_X`) to the middle of the gap after it (the last to 259). `CtrPokeTouch_OptionHit` returns `PT_OPT_ZONE_SEG + DevOptLevelAt(…)` on a level row's PREV / NEXT. Each word (`DevUpper`'d) centred in its box (`DevFitWord`, the row's ink); the chosen level (or the held one) outlined 1 px on its box's own edge (`DevOptBox` draws on r's edge).

- [ ] **Step 1: Write the failing tests** (`options` suite; Review Focus 2; row 4 COMPANION: OFF, FOLLOW, BE IT, FOLLOW chosen):

```c
CtrPokeTouchRect b[3]; int y = DevOptRow(4).y + 9;
for (k = 0; k < 3; ++k) b[k] = DevOptLevelBox(&v, 4, k);
assert(b[2].x + b[2].w == 250 && b[1].x == b[0].x + b[0].w + 4 && b[2].x == b[1].x + b[1].w + 4
       && b[0].y == DevOptRow(4).y + 2 && b[0].h == 14 && b[1].w == DevTextWidth("FOLLOW") + 8);
for (x = 136; x < 260; ++x) assert(OptHitV(&v, x, y) >= 4 * 16 + PT_OPT_ZONE_SEG);
assert(OptHitV(&v, b[1].x - 3, y) == 4 * 16 + PT_OPT_ZONE_SEG && OptHitV(&v, b[1].x - 2, y) == 4 * 16 + PT_OPT_ZONE_SEG + 1
       && OptHitV(&v, 259, y) == 4 * 16 + PT_OPT_ZONE_SEG + 2);
assert(at(b[1].x, b[1].y + 7) == DEV_INK && at(b[0].x, b[0].y + 7) != DEV_INK);       /* FOLLOW outlined */
v.optSegValue[4] = 2; assert(DevWhole(DevCheckTouch(&old, &v))); assert(at(b[2].x, b[2].y + 7) == DEV_INK);
for (k = 0; k < 3; ++k) strcpy(v.optSegName[4][k], "Abcdefg");                        /* too wide even small */
DevCheckTouch(&old, &v); assert(DevOptLevelSmall(&v, 4) && sDevKeyCut == 1); sDevKeyCut = 0;
```

On the selected (inverted) row the outline is in the paper's ink: `at(b.x, b.y + 7) == DEV_PAPER` for the chosen one.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS (ES / DE: EXP SHARE's and COMPANION's words fit, or go small uncut).
- [ ] **Step 5: Task close** — renders `options_{graphics_frame,qol_levels}` vs `def_opt_graphics.txt` (the Interfaces' rects and capitals). Azahar: COMPANION and EXP SHARE — a tap on each word, between two words, at the row's right end. Commit `feat: OPTIONS' levels in word-sized boxes, hit to the gaps' middles`.

### Task 4: SAVE as the Definitiva

**Files:**
- Modify: `3ds_device_layout.h` (`kDevSaveScreen`, `kDevSaveRule`, `DevSaveRow`, `kDevSaveSprite`, `kDevSaveYes/No/Ok`, `kDevSaveBar`), `3ds_device_save_impl.h`, `3ds_port/include/3ds_poketouch.h` (`saveStep`, `PT_SAVE_STEPS`, the SAVE comment at ~97), `3ds_bottom_battle_fill_impl.h` (`SaveFill`), `3ds_bottom.c` (`ProcessSaveInput`)
- Test: `device_suite_save.h` (rewritten), `device_layout_test.c` (`ScreensFaults`' SAVE part, the SAVE groups), `tools/test_field_presentation.py`

**Interfaces:**
- Consumes: `DevColumnRow`, `DevSmallFit`, `DevBar`, `DEV_RULE`, `DEV_TITLE_INK`, `DEV_EXP`.
- Produces:
  - Layout: `kDevSaveScreen {8,8,304,132}`; `kDevSaveRule[2] = {{20,30,280,1}, {20,114,280,1}}`; `DevSaveRow(int i)` = {20, 36 + 15i, 188, 15}; `kDevSaveSprite {228,38,64,64}`; `kDevSaveYes {8,148,228,82}`, `kDevSaveNo {244,148,68,82}`, `kDevSaveOk {8,148,304,82}`; `kDevSaveBar {196,122,100,6}`. `kDevAskNote/Yes/No` stay (Easy Chat's question); SAVE stops borrowing `kDevHomeScreen` (04a's grid rule names it until this task).
  - View: `uint8_t saveStep;` and `#define PT_SAVE_STEPS 2`. `SaveFill`: `CtrPokeTouch_Location(v->bar[0], PT_BAR_LEN);` `v->saveStep = (uint8_t)(sSave.state == SAVE_WRITING ? PT_SAVE_STEPS - sSave.wait : 0);` the dialogue on `bar[1]` / `bar[2]` (port words, the board's wording):
    - CONFIRM: `CTR_TR("WOULD YOU LIKE TO SAVE THE GAME?", "\xC2\xBFQUIERES GUARDAR LA PARTIDA?", "M\xC3\x96" "CHTEST DU SPEICHERN?")` / empty; another game saved: `CTR_TR("ANOTHER GAME IS SAVED.", "YA HAY OTRA PARTIDA.", "ANDERER SPIELSTAND DA.")` / `CTR_TR("OVERWRITE IT?", "\xC2\xBFSOBRESCRIBIRLA?", "\xC3\x9C" "BERSCHREIBEN?")`;
    - WRITING: `CTR_TR("SAVING...", "GUARDANDO...", "SPEICHERN...")` / `CTR_TR("DON'T TURN OFF THE POWER.", "NO APAGUES LA CONSOLA.", "NICHT AUSSCHALTEN.")`;
    - SUCCESS: `CTR_TR("%s SAVED THE GAME.", "%s GUARD\xC3\x93 LA PARTIDA.", "%s HAT GESPEICHERT.")` of `v->playerName` (as typed) / empty;
    - ERROR: `CTR_TR("COULD NOT SAVE.", "NO SE PUDO GUARDAR.", "SPEICHERN FEHLGESCHLAGEN.")` / `CTR_TR("TRY AGAIN.", "PRUEBA OTRA VEZ.", "BITTE ERNEUT VERSUCHEN.")`.
  - `ProcessSaveInput`: `K_LEFT | K_RIGHT` join `K_UP | K_DOWN` in the physical mask and toggle `sel` while confirming.
  - Drawer: the place `DevUpper(bar[0])` `DEV_TITLE_INK` over `DEV_INK_SH` from (20, capitals 17) to 300; the rules `DEV_RULE`; facts `DevColumnRow(DevSaveRow(i).y, label, value, 20, 208)` (today's words); the sprite. The dialogue `"%s %s"` of bar[1], bar[2]: WRITING `DevSmallFit(text, 20, 20, 118, 190, 2, 8, DEV_INK)` and `DevBar(196, 122, 100, 6, 98 * saveStep / PT_SAVE_STEPS, DEV_EXP)`; else one normal line at (20, capitals 121) when it fits 280 px, else `DevSmallFit(text, 20, 20, 118, 300, 2, 8, DEV_INK)`. Keys: CONFIRM — YES cream in `kDevSaveYes`, NO dark in `kDevSaveNo`, sunk by `DevSaveSunk`; WRITING — both `DEV_LOOK_FAINT` in the same rects, not hit; SUCCESS / ERROR — OK cream in `kDevSaveOk`, up, sunk only while `savePressed == PT_SAVE_FIRST`. `CtrPokeTouch_SaveHit` follows the keys drawn. `DevSaveSame` adds `saveStep` and the OK's press.

- [ ] **Step 1: Write the failing tests.** `save` suite (Review Focus 4; `SaveDialogue` writes the new words and the place, "PALLET TOWN"):

```c
v = SaveBoard(); DevCheckTouch(&old, &v);
AssertHits(SaveHitV, &v, kDevSaveYes, PT_SAVE_FIRST); AssertHits(SaveHitV, &v, kDevSaveNo, PT_SAVE_SECOND);
AssertKey(kDevSaveYes, DEV_LOOK_KEY, 1); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 0);
assert(at(100, 30) == DEV_RULE && at(100, 114) == DEV_RULE && Count(20, 17, 200, 9, DEV_TITLE_INK) > 0 && Count(20, 121, 280, 9, DEV_INK) > 0);
assert(Count(150, 39, 58, 9, DEV_INK) > 0 && Count(209, 36, 18, 76, DEV_INK) == 0);                /* values end at 208 */
v.saveSel = 1; DevCheckTouch(&old, &v); AssertKey(kDevSaveNo, DEV_LOOK_DARK, 1);
SaveDialogue(&v, SAVE_CONFIRM, 1); DevCheckTouch(&old, &v); assert(Count(14, 115, 292, 19, DEV_INK) > 0); /* one line or two small */
v.saveState = SAVE_WRITING; v.saveStep = 1; SaveDialogue(&v, SAVE_WRITING, 0); DevCheckTouch(&old, &v);
AssertKey(kDevSaveYes, DEV_LOOK_FAINT, 0); AssertKey(kDevSaveNo, DEV_LOOK_FAINT, 0);
assert(CtrPokeTouch_SaveHit(100, 180, SAVE_WRITING) == PT_SAVE_NONE && at(197, 124) == DEV_EXP && at(294, 124) == DEV_HP_TRACK
       && Count(20, 118, 170, 15, DEV_INK) > 0 && Count(191, 118, 4, 15, DEV_INK) == 0);
v.saveStep = 2; assert(DevWhole(DevCheckTouch(&old, &v))); assert(at(294, 124) == DEV_EXP);
SaveDialogue(&v, SAVE_SUCCESS, 0); v.savePressed = PT_SAVE_NONE; DevCheckTouch(&old, &v);
AssertKey(kDevSaveOk, DEV_LOOK_KEY, 0); AssertHits(SaveHitV, &v, kDevSaveOk, PT_SAVE_FIRST);
v.savePressed = PT_SAVE_FIRST; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevSaveOk, DEV_LOOK_KEY, 1);
SaveDialogue(&v, SAVE_ERROR, 0); v.savePressed = PT_SAVE_NONE; DevCheckTouch(&old, &v); AssertKey(kDevSaveOk, DEV_LOOK_KEY, 0);
v.mapGroup ^= 1; v.speed ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
```

`test_field_presentation.py`:

```python
def test_save_fill_says_the_place_and_the_steps(self):
    """SAVE's screen is titled with the place (HOME's CtrPokeTouch_Location) and its writing bar shows the port's own
    steps; YES / NO side by side toggle on LEFT / RIGHT too (plan 04b Task 4)."""
    fill = function((ROOT / '3ds_port/src/3ds_bottom_battle_fill_impl.h').read_text(encoding='utf-8'), 'SaveFill')
    self.assertIn('CtrPokeTouch_Location(v->bar[0], PT_BAR_LEN);', fill)
    self.assertIn('v->saveStep = (uint8_t)(sSave.state == SAVE_WRITING ? PT_SAVE_STEPS - sSave.wait : 0);', fill)
    self.assertIn('K_LEFT | K_RIGHT', function((ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8'), 'ProcessSaveInput'))
```

Layout `ScreensFaults` (SAVE): the rows inside `kDevSaveScreen`'s paper between the rules, clear of the sprite; groups "SAVE asking" {`kDevSaveYes`, `kDevSaveNo`} (8 apart) and "SAVE result" {`kDevSaveOk`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_field_presentation` → FAIL.
- [ ] **Step 3: Implement** per Interfaces.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS (ES / DE: the overwrite falls back to two small lines uncut).
- [ ] **Step 5: Task close** — renders `save_{confirm,no,overwrite,writing,saved,error}` vs `def_save_confirm.txt`, `def_save_writing.txt`, `def_save_saved.txt` (the Interfaces' rects and capitals). Azahar: SAVE from HOME — YES / NO by touch and by ← → ↑ ↓, B; save (the writing screen, then "RED SAVED THE GAME." and OK); a second save file's overwrite question. Commit `feat: SAVE as the Definitiva: the place, YES wide, NO in the corner, a bar`.

### Task 5: The Trainer Card's colour strip and stars

**Files:**
- Modify: `3ds_device_layout.h` (`kDevCardStrip`, `DevCardRow`, `kDevCardPic`, `DevCardBadge`, `kDevCardRule`, `DevCardBackRow`, `DEV_CARD_*`), `3ds_device_card_impl.h`
- Test: `device_suite_card.h`, `device_layout_test.c` (`ScreensFaults`' card part), `tools/test_make_device_boards.py` (`test_real_traces_pin_the_pieces`)

**Interfaces:**
- Consumes: `DevColumnRow`, `DevPaperHole`, `DEV_RULE`, `kDevTallScreen`, `kDevCardFlip`, `kDevTallBack`.
- Produces:
  - Layout: `kDevCardStrip {14,14,292,22}`; `DevCardRow(int i)` = i ? {22, 44 + 18i, 186, 18} : {22, 42, 276, 18}; `kDevCardPic {226,60,64,64}`; `DevCardBadge(int i)` = {20 + 35i, 134, 32, 32}; `kDevCardRule {22,58,276,1}`; `DevCardBackRow(int i)` = {22, 66 + 21i, 276, 21}; `DEV_CARD_STARS 4`, `DEV_CARD_STAR_X 241`, `DEV_CARD_STAR_Y 19`, `DEV_CARD_STAR_STEP 13`; `#define DEV_CARD_NAME_X 56`, `#define DEV_CARD_ID_X 206`.
  - Drawer: `static uint16_t DevCardColour(int stars)` — FireRed's card colour by its stars (0-4, more counts as 4): `UI_RGB(0x5A,0x8A,0xC8)`, `(0x3E,0x9A,0x5A)`, `(0xB8,0x7A,0x3A)`, `(0x9A,0x9A,0xAA)`, `(0xC8,0xA0,0x30)`. Both sides: the strip `DevRect` in that colour; `DevUpper(CtrUiText(UIS_CARD_TITLE))` `DEV_WHITE` over `DevBlend(colour, 0, 90)` from (22, capitals 21) to 236; four stars `FCStar(241 + 13k, 19, k < stars ? UI_RGB(0xF8, 0xD0, 0x40) : DevBlend(colour, DEV_WHITE, 115), DevBlend(colour, 0, 77))`. Front: NAME small `DEV_INK2` at (22, capitals 46), the name from x max(56, 22 + label + 6) capitals 45 cut before 200, IDNo. small at (206, 46), the ID right at 298 capitals 45; MONEY / POKéDEX / TIME `DevColumnRow(DevCardRow(r).y, label, value, 22, 208)`; the picture; a badge earned at 2x, one not `DevPaperHole(r.x, r.y, 32)`. Back: the name normal at (22, capitals 45), `kDevCardRule` in `DEV_RULE`, records `DevColumnRow(DevCardBackRow(i).y, label, value, 22, 298)`. `DevCardSame`'s back branch compares the stars too.

- [ ] **Step 1: Write the failing tests** (`card` suite; `kColour[5]` the suite's copy of the five colours):

```c
for (s = 0; s <= 5; ++s) { v.card.stars = (uint8_t)s; DevCheckTouch(&old, &v); assert(at(150, 16) == kColour[s > 4 ? 4 : s]); }
v.card.stars = 2; DevCheckTouch(&old, &v);
assert(Count(241, 19, 12, 12, UI_RGB(0xF8, 0xD0, 0x40)) > 0 && Count(267, 19, 12, 12, UI_RGB(0xF8, 0xD0, 0x40)) == 0
       && Count(267, 19, 12, 12, DevBlend(kColour[2], DEV_WHITE, 115)) > 0 && Count(22, 21, 120, 9, DEV_WHITE) > 0);
r = DevCardBadge(7); assert(at(r.x + 16, r.y + 16) == DEV_PAPER_HOLE && at(r.x + 16, r.y + 1) == DEV_PAPER_LINE);
assert(Count(206, 46, 30, 7, DEV_INK2) > 0 && Count(56, 45, 30, 9, DEV_INK) > 0 && Count(209, 62, 16, 50, DEV_INK) == 0);
v.card.back = 1; DevCheckTouch(&old, &v); assert(at(100, 58) == DEV_RULE && at(150, 16) == kColour[2]);
v.card.stars = 3; assert(DevWhole(DevCheckTouch(&old, &v)));
AssertHits(CardHitV, &v, kDevCardFlip, 1); AssertHits(BackV, &v, kDevTallBack, 1);
```

`test_real_traces_pin_the_pieces` gains `'card_front': {'disc': 'border-radius: 50%'}` (its badges not earned). Layout `ScreensFaults` (card): rows, picture, badges (3 apart) inside `kDevTallScreen`'s paper; the stars inside `kDevCardStrip`; rows 1-3 left of the picture; the back's records under the rule.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_make_device_boards` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DEV_CARD_GOLD` goes).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `card_{front,pressed,back}` vs `def_card_front.txt`, `def_card_back.txt` (the Interfaces' rects and capitals). Azahar: the card with its stars' colour, FLIP by the key and by a tap on the card, BACK. Commit `feat: the Trainer Card's colour strip, stars and badge holes`.

### Task 6: The Pokédex's menu and habitat page on the grid

**Files:**
- Modify: `3ds_device_layout.h` (`DevDexItem`, `DevDexSlot`; delete `DevDexTabKey`), `3ds_device_dex_impl.h`
- Test: `device_suite_dex.h` (menu and habitat blocks), `device_layout_test.c` (`DexFaults`, the menu and habitat groups)

**Interfaces:**
- Consumes: `DevGridTab`, `kDevGridTitle`, `kDevGridExit`, `kDevSmallNote`, `kDevSmallKey`, `DevArrowKeys`, `DevTitleLine`, `DevKeySmallWord`, `DevCloseWord`.
- Produces: tabs `DevGridTab(3, i)` (for 04a's `DevDexTabKey`); `DevDexItem(int i)` = `DevAcross(3, i % 3, 40 + 72(i / 3), 66)`; `DevDexSlot(int i)` = {8 + 156(i % 2), 40 + 72(i / 2), 148, 66}. A menu key: FireRed's 64x48 icon at (r.x + 16, faceY + 3) (`DevArt`), its label `DevKeySmallWord(&r, faceY + 56, look, sunk, word)`; a locked row (MOUNTAIN, nothing seen) `DEV_LOOK_FAINT` with the icon at half over the face (`DevDexIcon`'s off branch: each colour `DevBlend(DEV_FAINT, c, 128)`, pixel by pixel). SEEN / OWNED rows {paper.x + 6, paper.y + 2 + 14i, paper.w − 12, 14} (`DevLabelValue`). ▲▼ always drawn: `DevArrowKeys(&kDevSmallKey[0], &kDevSmallKey[1], page > 0, page < pages − 1, …, 0)` (both grey on a one-page tab), hit only where on. CLOSE `kDevGridExit` (`DevCloseWord()`). The habitat page: `DevTitleLine(&kDevGridTitle, habitat, "PAGE n/m")`; a slot's picture at (r.x + 4, faceY + 1), the mark at (r.x + 6, faceY + 4), number and name from r.x + 74 at capitals faceY + 18 and faceY + 36; ◀ ▶ `DevArrowKeys(…, canPrev, canNext, …, 1)`; the note `kDevSmallNote` as today; BACK `kDevGridExit`. `DevDexTitle` and `DevDexKeySmall` go.

- [ ] **Step 1: Write the failing tests** (`dex` suite):

```c
for (i = 0; i < 3; ++i) AssertHits(DexHitV, &v, DevGridTab(3, i), PT_DEX_HIT_TAB + i);
for (i = 0; i < 6; ++i) AssertHits(DexHitV, &v, DevDexItem(i), PT_DEX_HIT_ITEM + ItemRow(&v, i));
r = DevDexItem(5); AssertKey(r, DEV_LOOK_FAINT, 0); assert(Count(r.x + 4, r.y + 56, r.w - 8, 7, DEV_FAINT_INK) > 0);  /* MOUNTAIN */
r = DevDexItem(1); AssertKey(r, DEV_LOOK_KEY, 1); assert(Count(r.x + 4, r.y + 59, r.w - 8, 8, DEV_SUNK_INK_SH) > 0);   /* its label's shadow */
assert(Count(20, 201, 40, 7, DEV_INK2) > 0 && Count(110, 200, 38, 9, DEV_INK) > 0 && Count(149, 196, 5, 30, DEV_INK) == 0);
AssertHits(DexHitV, &v, kDevGridExit, PT_DEX_HIT_ITEM + DevDexCancelRow(&v.dex));
/* lv: the suite's LISTS fixture, one page */ assert(Count(168, 190, 68, 40, DEV_ARROW_OFF) > 0 && DexHitV(&lv, 184, 210) == -1 && DexHitV(&lv, 220, 210) == -1);
for (i = 0; i < 4; ++i) AssertHits(DexHitV, &cv, DevDexSlot(i), PT_DEX_HIT_SLOT + i);
AssertHits(BackV, &cv, kDevGridExit, 1); assert(Count(280, 17, 19, 7, DEV_INK2) > 0);   /* "PAGE 1/2" ends at 298 */
```

Layout `DexFaults`: `DevDexItem(i)` and `DevDexSlot(i)` drops end ≤ 187, apart; groups "Pokédex menu" {3 tabs, 6 keys, `kDevSmallKey[0..1]`, `kDevGridExit`} and "Pokédex habitat" {4 slots, `kDevSmallKey[0..1]`, `kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevDexMenu`, `DevDexCounts`, `DevDexCategory`, `DevDexSlotDraw`, `CtrPokeTouch_DexHit`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `dex_{lists,habitats,habitats_page2,search,category}` vs `def_dex_menu.txt` (the Interfaces' rects and capitals). Azahar: the Pokédex's three tabs (▲▼ grey on LISTS and SEARCH), HABITATS' pages, a habitat's page with ◀ ▶, CLOSE. Commit `feat: the Pokédex's menu and habitat page on the grid`.

### Task 7: The Pokédex's list: five rows of 24, 26 apart

**Files:**
- Modify: `3ds_device_layout.h` (delete `DevTallRow`, `DevListRow`), `3ds_device_dex_impl.h`, `3ds_bottom.c` (`LIST_DRAG_ROW 26` and its comment at ~744)
- Test: `device_suite_dex.h` (list blocks: `DevTallRow` → `DevGridRow`, the mark band), `device_layout_test.c` (the `DevTallRow` pins at ~412-414), `tools/test_field_presentation.py`

**Interfaces:**
- Consumes: `DevGridRow`, `kDevGridTitle`, `kDevGridScreen`, `kDevGridUp/Down`, `kDevGridNote`, `kDevGridExit`, `DevArrowKeys`, `DevTitleLine`, `DevTypeChip`, `DEV_GRID_ROW_PITCH`.
- Produces: rows `DevGridRow(i, 0)`, hit on themselves (`UiIn`; the gaps hit nothing); a row: "No." number from r.x + 6 and the name from r.x + 52 at capitals r.y + 8 (the name cut 4 px before the mark), a caught one's 8x8 mark at (r.x + r.w − 16, r.y + 8). The title `DevTitleLine(&kDevGridTitle, order, counts)`. The footer `kDevGridNote`: a caught one's type chips (`DevTypeChip(name, type, x, 205, 12)`, 3 apart from x 20), its kind small `DEV_INK2` 3 px after them at capitals 208; else "?????" as today. ▲▼ `DevArrowKeys(&kDevGridUp, &kDevGridDown, …)`; BACK `kDevGridExit`. `#define LIST_DRAG_ROW 26` (= `DEV_GRID_ROW_PITCH`).

- [ ] **Step 1: Write the failing tests.** `dex` suite (Review Focus 5):

```c
for (i = 0; i < PT_DEX_ROWS; ++i) AssertRowHits(DexHitV, &v, DevGridRow(i, 0), PT_DEX_HIT_ROW + i);
r = DevGridRow(1, 0); assert(DexHitV(&v, 100, r.y - 1) == -1 && DexHitV(&v, 100, r.y - 2) == -1);      /* the gap */
for (i = 0; i < PT_DEX_ROWS; ++i) { r = DevGridRow(i, 0); assert((Count(r.x + r.w - 16, r.y + 8, 8, 8, STUB_MARK) > 0) == ((v.dex.row[i].flags & 2) != 0)); }
r = DevGridRow(0, 0); assert(Count(22, r.y + 8, 36, 9, DEV_INK) > 0 && Count(22, r.y + 7, 36, 1, DEV_INK) == 0);
assert(Count(250, 17, 48, 7, DEV_INK2) > 0 && Count(299, 12, 7, 16, DEV_INK2) == 0);
assert(Count(20, 205, 40, 12, UiTypeColor(v.dex.type[0])) > 0 && Count(109, 208, 60, 7, DEV_INK2) > 0);
v.dex.listFirst = (uint16_t)(v.dex.listCount - PT_DEX_ROWS); v.dex.listCursor = (uint16_t)(v.dex.listCount - 1); DevCheckTouch(&old, &v);
assert(DexHitV(&v, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0); AssertHits(DexHitV, &v, kDevGridUp, PT_DEX_HIT_UP);
AssertHits(BackV, &v, kDevGridExit, 1);
```

`test_field_presentation.py`:

```python
def test_dex_list_drags_a_row_per_grid_row(self):
    """The Pokédex list's rows are the grid's, 26 px apart (3ds_device_layout.h): a drag moves one row per row."""
    layout = (ROOT / '3ds_port/include/3ds_device_layout.h').read_text(encoding='utf-8')
    pitch = int(re.search(r'#define DEV_GRID_ROW_PITCH (\d+)', layout).group(1))
    bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
    self.assertEqual(int(re.search(r'#define LIST_DRAG_ROW (\d+)', bottom).group(1)), pitch)
```

Layout: the group "Pokédex list" {`kDevGridUp`, `kDevGridDown`, `kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_field_presentation` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevDexRow`, `DevDexList`, `DevDexTypes` on `DevTypeChip`, `CtrPokeTouch_DexHit`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `dex_{list_top,list_end}` vs `def_dex_list.txt` (the Interfaces' rects and capitals). Azahar: KANTO's list — top and bottom, tap twice to open, ▲▼, a drag (one row per 26 px). Commit `feat: the Pokédex's list in five 24 px rows on the grid`.

### Task 8: The Pokédex's page and a new catch

**Files:**
- Modify: `3ds_device_layout.h` (`kDevDexRule`, `kDevDexEntry`; add `DevDexDataRow`), `3ds_device_dex_impl.h`
- Test: `device_suite_dex.h` (page and added blocks), `device_layout_test.c` (`DexFaults`' page part)

**Interfaces:**
- Consumes: `DevWrapPitch`, `DevWrapLines`, `DevChip`, `DevChipShadow`, `DEV_RULE`, `DEV_TAG_NEW`, `kDevTallScreen`, `kDevTallBack`, `kDevWideKey`.
- Produces: `kDevDexRule {20,106,280,1}` (`DEV_RULE`); `kDevDexEntry {20,112,280,62}`; `DevDexDataRow(int i)` = {110, 74 + 14i, 130, 14} (HT, WT: `DevLabelValue`, the value ending at 240). The entry `DevWrapPitch(desc, 20, 115, 280, 4, 14, DEV_INK, DEV_INK_SH)`, `++sDevKeyCut` when a fifth line is needed (`DevWrapLines(desc, 280, 5) == 5`). A new catch (`d->added`): `CTR_TR("REGISTERED", "REGISTRADO", "REGISTRIERT")` in a chip 14 tall, small width + 8 wide, ending at x 278, at y 20 (`DEV_TAG_NEW`, `DevChipShadow(DEV_TAG_NEW)`); the name cut 4 px before it; OK in `kDevWideKey` cream, up, sunk while `d->pressed == PT_DEX_HIT_CONTINUE` (the fill gives that code while OK or the screen is held).

- [ ] **Step 1: Write the failing tests** (`dex` suite):

```c
assert(at(100, 106) == DEV_RULE && Count(20, 115, 280, 9, DEV_INK) > 0 && Count(110, 77, 20, 7, DEV_INK2) > 0
       && Count(200, 76, 40, 9, DEV_INK) > 0 && Count(241, 72, 40, 30, DEV_INK) == 0);
strcpy(v.dex.desc, kFiveLines); DevCheckTouch(&old, &v); assert(Count(20, 157, 280, 9, DEV_INK) > 0 && sDevKeyCut == 1); sDevKeyCut = 0;
av.dex.added = 1; DevCheckTouch(&ao, &av); w = DevSmallTextWidth("REGISTERED") + 8;
assert(Count(278 - w, 20, w, 14, DEV_TAG_NEW) > 0 && at(279, 27) != DEV_TAG_NEW); AssertKey(kDevWideKey, DEV_LOOK_KEY, 0);
av.dex.pressed = PT_DEX_HIT_CONTINUE; assert(DevWhole(DevCheckTouch(&ao, &av))); AssertKey(kDevWideKey, DEV_LOOK_KEY, 1);
AssertHits(DexHitV, &av, kDevWideKey, PT_DEX_HIT_CONTINUE);
```

(`kFiveLines` a port-written sentence that wraps into five lines at 280 px.) Layout: the HT / WT rows and `kDevDexEntry` inside `kDevTallScreen`'s paper, the entry under the rule.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevDexPage`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `dex_{page,page_seen,added}` vs `def_dex_page.txt`, `def_dex_added.txt` (the Interfaces' rects and capitals). Azahar: a page's AREA, CRY, BACK; a catch's new page (REGISTERED, OK and a tap on the screen continue). Commit `feat: the Pokédex's page and a new catch as the Definitiva`.

### Task 9: The letter

**Files:**
- Modify: `3ds_device_layout.h` (`kDevMailScreen`, `kDevMailIcon`, `kDevMailName`, `kDevMailRule`, `kDevMailFrom`), `3ds_device_mail_impl.h`
- Test: `device_suite_mail.h`, `device_layout_test.c` (`ScreensFaults`' letter part, the letter's group)

**Interfaces:**
- Consumes: `kDevGridNote`, `kDevGridExit`, `DevSmallFit`, `DevItemIcon`, `DevCloseWord`, `DEV_RULE`, `DEV_SMALL_LINE`.
- Produces: `kDevMailScreen {8,8,304,176}`, `kDevMailIcon {148,44,24,24}`, `kDevMailName {22,77,276,18}`, `kDevMailRule {80,100,160,1}` (`DEV_RULE`), `kDevMailFrom {22,109,276,18}`. Who wrote it: one string `"%s %s"` of `CTR_TR("FROM", "DE", "VON")` and the writer (as typed), normal `DEV_INK` over `DEV_INK_SH`, centred (left out when unknown). The note on `kDevGridNote`: `DevSmallFit(CTR_TR("THE LETTER IS ON THE UPPER SCREEN. TAP HERE OR CLOSE.", "LA CARTA EST\xC3\x81 EN LA PANTALLA DE ARRIBA. TOCA AQU\xC3\x8D O CERRAR.", "DER BRIEF IST AUF DEM OBEREN BILDSCHIRM. HIER TIPPEN ODER SCHLIE\xC3\x9F" "EN."), 18, 18, 198, 226, 3, DEV_SMALL_LINE, DEV_INK)`. CLOSE `DevFieldBack(v, &kDevGridExit, DevCloseWord())`; `DevMailBackKey` → `&kDevGridExit`. `CtrPokeTouch_MailHit`: the screen or the note (`kDevMailScreen`, `kDevGridNote`).

- [ ] **Step 1: Write the failing tests** (`mail` suite):

```c
assert(CtrPokeTouch_MailHit(&mv, 8, 8) && CtrPokeTouch_MailHit(&mv, 311, 183) && !CtrPokeTouch_MailHit(&mv, 8, 185)
       && CtrPokeTouch_MailHit(&mv, 20, 210) && !CtrPokeTouch_MailHit(&mv, 240, 210) && !CtrPokeTouch_MailHit(&mv, 278, 209));
AssertHits(BackV, &mv, kDevGridExit, 1); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(!BackV(&mv, 160, 90));
assert(at(100, 100) == DEV_RULE && Count(18, 198, 208, 17, DEV_INK) > 0 && Count(22, 113, 276, 9, DEV_INK2) == 0 && Count(22, 113, 276, 9, DEV_INK) > 0);
mv.mail.from[0] = 0; DevCheckTouch(&mo, &mv); assert(Count(22, 109, 276, 18, DEV_INK) == 0);
```

Layout: the icon centred on `kDevMailScreen`, name / rule / writer in its paper one under the other; the group "the letter" {`kDevGridExit`} (`mailKeys {kDevWideKey}` goes).
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS (DE's note in three small lines, uncut).
- [ ] **Step 5: Task close** — renders `mail_{from,bare}` vs `def_mail.txt` (the Interfaces' rects and capitals). Azahar: read a letter from the Bag's mail: CLOSE, a tap on the screen, a tap on the note. Commit `feat: the letter's note and CLOSE in the corner`.

### Task 10: FireRed's picture and the idle screen — then PR F1

**Files:**
- Modify: `3ds_device_layout.h` (delete `kDevTallNote`, `kDevViewNoteWide`), `3ds_device_frame_impl.h`
- Test: `device_suite_frame.h` (`kFrameNotePaper {14,196,216,30}`, `kFrameWidePaper {14,196,292,30}`, `FRAME_TITLE_Y 198`, the hints' rows, BACK), `device_layout_test.c` (`FrameFaults`, the frame's group)

**Interfaces:**
- Consumes: `kDevGridNote`, `kDevGridNoteWide`, `kDevGridExit`, `DevSmallFit`, `DEV_SMALL_LINE_TIGHT`.
- Produces: the note `DevScreen(v->backVisible ? &kDevGridNote : &kDevGridNoteWide)`; the title at capitals paper.y + 2 (198), alone centred down the paper (capitals 206); the hints small `DEV_INK2` at capitals paper.y + 13 and + 22 (209 / 218), one a line while each fits, else one text `DevSmallFit(…, 2, DEV_SMALL_LINE_TIGHT, DEV_INK2)`; BACK `kDevGridExit` (`DevFrameBackKey`). The idle screen unchanged (it matches def_idle).

- [ ] **Step 1: Write the failing tests** (`frame` suite):

```c
AssertHits(BackV, &v, kDevGridExit, 1); v.backPressed = 1; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
assert(Count(20, 198, 120, 9, DEV_INK) > 0 && Count(20, 197, 120, 1, DEV_INK) == 0 && Count(20, 209, 200, 7, DEV_INK2) > 0 && Count(20, 218, 200, 7, DEV_INK2) > 0);
v.backVisible = 0; DevCheckTouch(&old, &v); assert(Count(14, 196, 292, 30, DEV_PAPER) > 0 && !BackV(&v, 278, 209));
```

Layout `FrameFaults`: `kDevGridNoteWide` spans `kDevGridNote` and `kDevGridExit`; the group "FireRed's picture" {`kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `frame_{back,back_off,back_pressed,empty,wide}` vs `def_frame.txt` (note [8,190,228,42], title caps 198, hints 209 / 218, BACK [244,190,68,40]); `idle_*` vs `def_idle.txt` (the Interfaces' rects and capitals). Azahar: the Fame Checker and a Pokédex AREA map (BACK), the idle screen after a while. Commit `feat: FireRed's picture's note and BACK on the grid`.
- [ ] **Step 6: PR F1** — full check (`python tools/dev.py test`, build, audit, `python tools/dev.py render` every view, `python tools/dev.py run`); `git push -u origin feat/b-def-system-views`; `gh pr create --base feat/b-def-battle-questions --title "feat: the Definitiva system views (OPTIONS, SAVE, card, Pokédex, letter, picture)"`; body: each view's change, the kit pieces, the owner's checklist (Tasks 2-10's Azahar lines), the 🤖 line.

### Task 11: The naming keyboard's name strip

**Files:**
- Modify: `3ds_device_layout.h` (`kDevNameIcon`, `DevNameSlot`, `DEV_NAME_TITLE_X`, `DEV_NAME_COUNT_RIGHT`), `3ds_device_naming_impl.h`
- Test: `device_suite_naming.h` (`NAME_SLOT` / `NAME_LINE`, the underline and icon checks at ~203-211, ~244-248), `device_layout_test.c` (`NamingFaults`)

**Interfaces:**
- Consumes: `DEV_PAPER_LINE`, `DEV_MARK_ON`, `DEV_TITLE_INK`.
- Produces: `kDevNameIcon {20,16,24,24}`; `DevNameSlot(int k)` = {54 + 16k, 26, 14, 14} (k ≤ `PT_NAME_MAX`: the character's place, its underline the last row, y 39); `#define DEV_NAME_TITLE_X 52`, `#define DEV_NAME_COUNT_RIGHT 298`. The icon (a Pokémon's, `n->species`): `DevArt(DEV_ART_ICON, 20, 16, 24, 24)` and FireRed's 32x32 at 1:1 from (16, 10), its pixels outside `kDevNameIcon` not drawn; none otherwise (the box stays paper). FireRed's question small `DEV_INK2` from (52, capitals 17), cut before the counter. Each slot k < maxChars: its underline {s.x, 39, 14, 1} in `DEV_MARK_ON` for the next to type (k == len < maxChars), else `DEV_PAPER_LINE`; a typed character `DEV_TITLE_INK` over `DEV_INK_SH` from s.x + 2 at capitals 28. The gender sign at (`DevNameSlot(maxChars).x` + 2, 27). The counter `"%u / %u"` of len and maxChars, small `DEV_INK2`, right-aligned ending at 298, capitals 17.

- [ ] **Step 0: Branch** — `git checkout -b feat/b-def-naming-box-easychat` from Task 10's tip (PR F1 open); Tasks 11-20 commit on it.
- [ ] **Step 1: Write the failing tests** (`naming` suite; `NAME_SLOT(i)` becomes `DevNameSlot(i).x`; the fixture "RED", 7 places):

```c
for (i = 0; i < 7; ++i) assert(Count(NAME_SLOT(i), NAME_LINE, 14, 1, i == 3 ? DEV_MARK_ON : DEV_PAPER_LINE) == 14);
assert(Count(NAME_SLOT(0) + 2, 28, 12, 9, DEV_TITLE_INK) > 0 && Count(52, 17, 50, 7, DEV_INK2) > 0);
assert(Count(262, 17, 37, 7, DEV_INK2) > 0 && Count(299, 14, 7, 28, DEV_INK2) == 0);                  /* "3 / 7" */
v.name.len = 7; DevCheckTouch(&gNameOld, &v); assert(Count(14, 39, 292, 1, DEV_MARK_ON) == 0);          /* full */
monArt = 2; v.name.species = 4; DevCheckTouch(&gNameOld, &v);                                          /* a whole 32x32 square */
assert(CountIn(kDevNameIcon, STUB_ICON) == 24 * 24 && Count(16, 10, 4, 32, STUB_ICON) == 0 && Count(44, 14, 4, 28, STUB_ICON) == 0);
assert(Count(52, 17, 50, 7, DEV_INK2) > 0);                                                            /* the column does not move */
v.name.len ^= 1; assert(DevWhole(DevCheckTouch(&gNameOld, &v)));
```

Layout `NamingFaults`: `kDevNameIcon` inside `kDevNameScreen`'s paper and left of `DevNameSlot(0)`; `DevNameSlot(PT_NAME_MAX − 1)` plus 2 + the sign's 8 ends ≤ 262. `tools/test_make_device_boards.py` (naming_upper's top row, naming_cursor_page's lone LED) stays green.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevNameIcon`, `DevNameEntry`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `naming_{upper,lower,symbols,full,pokemon,cursor_delete,cursor_page}` vs `def_naming.txt` (the Interfaces' rects and capitals). Azahar: the player's name at a new game (or a Pokémon's nickname with its icon), the three pages, DELETE, OK, the counter. Commit `feat: the naming keyboard's name strip as the Definitiva`.

### Task 12: The PC box on plain paper, its column and footer

**Files:**
- Modify: `3ds_device_layout.h` (`kDevBoxScreen`, `kDevBoxTitle`, `DevBoxCell`, `kDevBoxHand`, `kDevBoxParty`, `kDevBoxCount`, `kDevBoxChoose`, `DevBoxMenuKey`; delete `kDevBoxClose`, `kDevBoxNote`, `kDevBoxNoteNarrow`, `kDevBoxCancel`), `3ds_device_box_impl.h`, `3ds_port/include/3ds_poketouch.h` (`CtrPokeTouchBox.monType`, `monTypeName`; delete `CtrPokeTouch_Wallpaper`), `3ds_port/include/3ds_ui_strings.h` (delete `UIS_BOX_PARTY`, `UIS_BOX_CLOSE`), `3ds_bottom_field_fill_impl.h` (`BoxSnapshot`)
- Test: `device_suite_box.h`, `device_render_test.c` (the wallpaper stub and `wallArt` go), `device_layout_test.c` (`BoxFaults`, the box's groups), `tools/test_field_presentation.py`

**Interfaces:**
- Consumes: `kDevGridNote`, `kDevGridExit`, `DevArrowKeys`, `DevReadText`, `DevSmallFit`, `DevTypeChip`, `DevKeyWordAt`, `DevCloseWord`, `DevCancelWord`, `DEV_SMALL_LINE_TIGHT`.
- Produces:
  - Layout: `kDevBoxScreen {8,8,172,176}` (paper 14,14,160,164); `kDevBoxTitle {14,14,160,20}`; `DevBoxCell(int i)` = {22 + 24(i % 6), 34 + 24(i / 6), 24, 24}; ◀ ▶ as today; `kDevBoxHand {188,40,124,60}`; `kDevBoxParty {188,108,124,40}`; `kDevBoxCount {188,156,124,28}`; `kDevBoxChoose {188,40,124,60}` (the hand's place: the chooser draws it instead of the hand); `DevBoxMenuKey(int i)` = {188, 8 + 36i, 124, 30}, i 0-4, `#define DEV_BOX_COLUMN 5`. CLOSE, the chooser's CANCEL and a menu's last row in `kDevGridExit`; every state's footer `kDevGridNote`.
  - View: at the end of `CtrPokeTouchBox`, `int8_t monType[2];` and `char monTypeName[2][12];` — the pointed Pokémon's types (−1 none), filled by the port: `BoxSnapshot` after `CtrBottomStorage_BoxFill`, for `b->mon.species && !b->mon.egg`, `CtrBottomDex_Types(b->mon.species, types, b->monTypeName[0], b->monTypeName[1], sizeof(b->monTypeName[0]))`, else both −1. `DevBoxDrawn` masks them with the Pokémon's lines, and masks `wallpaper` always.
  - The box: paper `DEV_PAPER2`; each place's square `DevRect(cell.x + 1, cell.y + 1, 22, 22, DEV_PAPER)`; the name on the band centred, capitals 19, `DEV_INK` over `DEV_INK_SH` (the band inverted when chosen); the icons `DevBoxIcon(&slot, cell.x − 4, cell.y − 6, &paper, 0)`; MOVE ITEMS' squares as today; the frame 2 px `DEV_BEZEL` round {cell.x − 1, cell.y − 1, 26, 26}. `sDevBoxWall`, `DevBoxWallpaper`, `DEV_BOX_TITLE_SH` go.
  - The column (BOX): ◀ ▶ `DevArrowKeys(…, 1, 1, …, 1)`; the hand: IN HAND small `DEV_INK2` at (196, capitals 50), the held Pokémon's icon at (194, 60) clipped to the paper and its name (`DevBoxMonName` while `holding`) small `DEV_INK2` from x 232 capitals 72, cut at 302; MOVE ITEMS' item in hand its icon at (198, 64). PARTY POKéMON cream, `CTR_TR("PARTY POK\xC3\xA9MON", "EQUIPO POK\xC3\xA9MON", "POK\xC3\xA9MON-TEAM")`, sunk by FireRed's cursor on its buttons (pos 0) or held; the count `"%u / %u"` of the box's Pokémon and `PT_BOX_SLOTS`, small `DEV_INK2` centred at capitals 165; CLOSE dark `DevCloseWord()`, sunk by pos 1 or held, hit `ST_HIT_BUTTON_CLOSE`.
  - The footer (`DevBoxNote(b)`, paper 14,196,216,30): FireRed's message `DevReadText(text, &paper, 20, right − (waiting ? 12 : 0), paper.y + 2, DEV_SMALL_LINE_TIGHT)` and ▼ at (paper.x + paper.w − 10, paper.y + paper.h − 6) while it waits; MOVE ITEMS' item: its icon at (18, 199), its name normal from x 48 capitals 198 (the Pokémon's name `DEV_INK2` right at 224), FireRed's description `DevSmallFit(…, 48, 48, 209, 224, 2, DEV_SMALL_LINE_TIGHT, DEV_INK)`, or the name and `UIS_BOX_NO_ITEM`; a Pokémon: line 1 normal from x 20 capitals 199 (the name, the gender's sign, the level: today's `DevBoxMonNote` line), line 2 the type chips `DevTypeChip(name, type, x, 212, 10)` 3 apart from x 20, then `"%s %s"` of `CTR_TR("ITEM", "OBJETO", "ITEM")` and FireRed's item (`monText[3]` upper) or `CTR_TR("NONE", "NINGUNO", "KEINS")`, small `DEV_INK2` 7 px after the chips at capitals 214 (an egg: line 1 only); no Pokémon: the box's name centred down the paper.
  - The other states on the grid (Tasks 13-14 refine them): a menu's rows in `DevBoxMenuKey(i)` (at most five; words from x + 12, `DevKeyWordAt`), its last row dark in `kDevGridExit` (hit `ST_HIT_MENU_BASE + n − 1`) — so the marks' CANCEL too; YES / NO the first two column keys; the chooser's key in `kDevBoxChoose` (no hand), its CANCEL `DevFieldBack(v, &kDevGridExit, DevCancelWord())` (`DevBoxBackKey`); the box's `DevDim` kept for now.

- [ ] **Step 1: Write the failing tests.** `box` suite (`menu7` becomes FireRed's six rows MOVE SUMMARY WITHDRAW MARK RELEASE CANCEL, shot `menu6`; `wallArt` and its checks go):

```c
for (i = 0; i < PT_BOX_SLOTS; ++i) AssertRowHits(BoxHitV, &v, DevBoxCell(i), i);
AssertHits(BoxHitV, &v, kDevBoxParty, ST_HIT_BUTTON_PARTY); AssertHits(BoxHitV, &v, kDevGridExit, ST_HIT_BUTTON_CLOSE);
AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(BoxHitV(&v, 250, 170) == ST_HIT_NONE && BoxHitV(&v, 250, 70) == ST_HIT_NONE);
r = DevBoxCell(29); assert(at(r.x - 1, r.y + 12) == DEV_BEZEL && at(r.x + r.w, r.y + 12) == DEV_BEZEL && at(r.x + 1, r.y + 12) != DEV_BEZEL);
r = DevBoxCell(28); assert(at(r.x, r.y + 12) == DEV_PAPER2 && at(r.x + 1, r.y + 12) == DEV_PAPER);    /* an empty place's square */
assert(Count(20, 199, 120, 9, DEV_INK) > 0 && Count(20, 212, 40, 10, UiTypeColor(v.box.monType[0])) > 0 && Count(110, 214, 100, 7, DEV_INK2) > 0);
assert(Count(194, 165, 112, 7, DEV_INK2) > 0 && Count(196, 50, 40, 7, DEV_INK2) > 0);
v.box.holding = 1; DevCheckTouch(&old, &v); assert(Count(194, 46, 112, 48, STUB_ICON) > 0 && Count(232, 72, 70, 7, DEV_INK2) > 0);
v.box.monType[1] ^= 1; assert(DevWhole(DevCheckTouch(&old, &v))); v.box.wallpaper ^= 1; assert(DevCheckTouch(&old, &v).count == 0);
/* menu6 */ for (i = 0; i < 5; ++i) AssertHits(BoxHitV, &m6, DevBoxMenuKey(i), ST_HIT_MENU_BASE + i);
AssertHits(BoxHitV, &m6, kDevGridExit, ST_HIT_MENU_BASE + 5); assert(Count(14, 196, 216, 30, DEV_INK) > 0);
/* chooser */ AssertHits(BoxHitV, &cv, kDevBoxChoose, ST_HIT_TITLE); AssertHits(BackV, &cv, kDevGridExit, 1);
```

`test_field_presentation.py` `test_box_fill_adds_the_types`: `function(fieldFill, 'BoxSnapshot')` contains `CtrBottomDex_Types(b->mon.species, types, b->monTypeName[0], b->monTypeName[1], sizeof(b->monTypeName[0]))`. Layout `BoxFaults`: the places inside the paper; `kDevBoxHand`, `kDevBoxParty`, `kDevBoxCount` one under the other, apart, ending ≤ 184; groups "PC box" {◀, ▶, PARTY, `kDevGridExit`}, "box menu of n" (n 2-6: `min(n − 1, 5)` column keys and `kDevGridExit`), "box chooser" {◀, ▶, `kDevBoxChoose`, `kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_field_presentation` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevBoxPlaces`, `DevBoxSide`, `DevBoxHand`, `DevBoxMonNote`, `DevBoxItemNote`, `DevBoxNote`, `DevBoxMenu`, `DevBoxChooser`, `DevBoxDraw`, `CtrPokeTouch_BoxHit`, `DevBoxBackKey`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `box_{full,empty,held,buttons,title,items,choose}` vs `def_box.txt` (the Interfaces' rects and capitals). Azahar: a box — move a Pokémon across boxes (IN HAND), PARTY POKéMON, CLOSE, MOVE ITEMS, the chooser. Commit `feat: the PC box on plain paper with IN HAND, the count and the types (owner O3)`.

### Task 13: The box's menus in the column, CANCEL in the corner (recipe 0268)

**Files:**
- Create: `patches/pokefirered/0268-box-menu-reports-cancel.json`
- Modify: `3ds_port/include/3ds_poketouch.h` (`CtrPokeTouchBox.menuCancel`), `3ds_device_layout.h` (`kDevBoxYes`, `kDevBoxNo`), `3ds_device_box_impl.h`
- Test: `device_suite_box.h` (menu, wallpaper menu, marks, YES / NO), `device_layout_test.c` (box groups), `tools/test_field_presentation.py`, `tools/test_make_device_boards.py` (the `box_choose` dim pin becomes `box_message`'s)

**Interfaces:**
- Consumes: Task 12's column, `kDevGridExit`, `DevFieldBack`, `DevBackWord`, `DevCancelWord`.
- Produces:
  - `uint8_t menuCancel;` right after `uint8_t menu[PT_BOX_MENU_MAX][16];` — 1 when the menu's last row is FireRed's CANCEL. Recipe 0268 (`src/pokemon_storage_system_tasks.c` only): in `CtrBottomStorage_BoxFill`'s MENU branch after the copy loop, `b->menuCancel = b->menuCount > 0 && gStorage->menuItems[b->menuCount - 1].textId == MENU_TEXT_CANCEL;`.
  - MENU: the column holds the rows but the CANCEL row (at most `DEV_BOX_COLUMN`); with `menuCancel` that row is the dark exit key in `kDevGridExit` with FireRed's word (`DevBoxUpper`), hit `ST_HIT_MENU_BASE + n − 1`, sunk by FireRed's cursor on it; without it (the wallpaper menus) every row is a column key and the exit key is the view's BACK, `DevFieldBack(v, &kDevGridExit, DevCancelWord())`, sending B (`DevBoxBackKey` → `&kDevGridExit` while `state == MENU && !menuCancel`).
  - YES / NO: `kDevBoxYes {188,8,124,84}`, `kDevBoxNo {188,100,124,84}`, cream, sunk by `menuCursor` or held, hits `ST_HIT_MENU_BASE`, `+ 1`; BACK `DevFieldBack(v, &kDevGridExit, DevBackWord())` (B = NO; `DevBoxBackKey` while YESNO).
  - In every state but BOX: no `DevDim` of the box; each icon but the cursor's place's drawn at half over what is under it (`DevBoxIcon` gains `int fade`: a pixel `DevBlend(DevGet(px, py), c, 128)`); the panel, the name and the frame at full strength. The footer is FireRed's message (Task 12's).

- [ ] **Step 1: Write the failing tests.** `test_field_presentation.py` (Review Focus 1):

```python
def test_box_menu_reports_cancel(self):
    """A PC box menu's CANCEL goes to the exit corner (def_box_menu) only when FireRed's last row is CANCEL: the
    wallpaper menus end in a scenery. The bridge reports it from the row's text id (recipe 0268, the tasks' file only)."""
    name = '0268-box-menu-reports-cancel.json'
    self.assertIn('b->menuCancel = b->menuCount > 0 && gStorage->menuItems[b->menuCount - 1].textId == MENU_TEXT_CANCEL;',
                  additions(name, 'src/pokemon_storage_system_tasks.c'))
    doc = json.loads((ROOT / 'patches/pokefirered' / name).read_text())
    self.assertEqual([f['path'] for f in doc['files']], ['src/pokemon_storage_system_tasks.c'])
    self.assertRegex((ROOT / '3ds_port/include/3ds_poketouch.h').read_text(encoding='utf-8'),
                     r'uint8_t menu\[PT_BOX_MENU_MAX\]\[16\];\s*uint8_t menuCancel;')
```

`box` suite:

```c
/* menu6, menuCancel 1, cursor on MOVE */ AssertKey(DevBoxMenuKey(0), DEV_LOOK_KEY, 1); AssertHits(BoxHitV, &m6, kDevGridExit, ST_HIT_MENU_BASE + 5);
assert(!BackV(&m6, 278, 209)); m6.box.menuCursor = 5; DevCheckTouch(&mo, &m6); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
r = DevBoxCell(m6.box.pos); assert(Count(r.x - 4, r.y - 6, 32, 32, STUB_ICON) > 0);                         /* the cursor's icon whole */
r = DevBoxCell(0); assert(Count(r.x, r.y, 24, 24, STUB_ICON) == 0 && Count(r.x, r.y, 24, 24, DevBlend(DEV_PAPER, STUB_ICON, 128)) > 0);
/* wv, a new shot `menu_wallpaper`: SCENERY 1-3, ETCETERA, menuCancel 0 */ for (i = 0; i < 4; ++i) AssertHits(BoxHitV, &wv, DevBoxMenuKey(i), ST_HIT_MENU_BASE + i);
AssertHits(BackV, &wv, kDevGridExit, 1); AssertKey(DevBoxMenuKey(3), DEV_LOOK_KEY, 0);
/* YES / NO */ AssertHits(BoxHitV, &yv, kDevBoxYes, ST_HIT_MENU_BASE); AssertHits(BoxHitV, &yv, kDevBoxNo, ST_HIT_MENU_BASE + 1); AssertHits(BackV, &yv, kDevGridExit, 1);
/* marks */ for (i = 0; i < 5; ++i) AssertHits(BoxHitV, &kv, DevBoxMenuKey(i), ST_HIT_MENU_BASE + i); AssertHits(BoxHitV, &kv, kDevGridExit, ST_HIT_MENU_BASE + 5);
m6.box.menuCancel ^= 1; assert(DevWhole(DevCheckTouch(&mo, &m6)));
```

(`DevBoxCell(0)` holds a Pokémon in the fixture; its faded pixels are the stub's over the place's square.) `test_real_traces_pin_the_pieces`: `'box_choose': {'dim': …}` becomes `'box_message': {'dim': 'opacity: 0.5', 'screen': …}` (Task 12's MESSAGE still dims its column). Layout: groups "box YES / NO" {`kDevBoxYes`, `kDevBoxNo`, `kDevGridExit`}, "box wallpaper menu" {4 column keys, `kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_field_presentation test_device_views test_make_device_boards` → FAIL.
- [ ] **Step 3: Make recipe 0268** — copy `build/dev/src/pokemon_storage_system_tasks.c` (0145, 0158, 0163, 0166, 0184, 0188, 0199 applied; `build/dev` holds 0266 and 0267) to `build/work/src/`, add the line; `python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/0268-box-menu-reports-cancel.json src/pokemon_storage_system_tasks.c`; then the header field and the drawer.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py build` (the recipe applies); `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `box_{menu5,menu6,menu_wallpaper,yesno,marks}` vs `def_box_menu.txt` (the Interfaces' rects and capitals). Azahar: a Pokémon's menu (CANCEL by the corner and by B), the box title's menu, WALLPAPER's scenery menus (the corner sends B), MARK (CANCEL), RELEASE's YES / NO and BACK. Commit `feat: the PC box's menus in the column, CANCEL in the corner (recipe 0268)`.

### Task 14: The box's Party, chooser and message

**Files:**
- Modify: `3ds_device_layout.h` (`DevBoxPartyRow`), `3ds_device_box_impl.h`
- Test: `device_suite_box.h` (party, choose, message blocks), `device_layout_test.c` (box groups)

**Interfaces:**
- Consumes: Tasks 12-13's box, `kDevGridExit`, `DevCancelWord`.
- Produces: `DevBoxPartyRow(int i)` = {16, 16 + 26i, 156, 24}, i 0-5 (rows, hit on themselves); a row: the half icon at (r.x + 2, r.y + 4), the nickname (as typed) from r.x + 22 and "Lv n" right at r.x + r.w − 6, capitals r.y + 8; MOVE ITEMS' square at (r.x + 14, r.y + 14). FireRed's seventh row CANCEL: `kDevGridExit` dark `DevCancelWord()`, hit `ST_HIT_PARTY_BASE + 6`, sunk by the cursor on it (pos 6) or held; in the Party ◀ ▶ and PARTY POKéMON are beige (`DEV_LOOK_FAINT`, not hit). The chooser: its key sunk in `kDevBoxChoose`: the box's name normal centred at capitals faceY + 15, `"%u/%u"` small centred at capitals faceY + 34; PARTY POKéMON beige; the count's screen as in BOX. MESSAGE (a tap anywhere is its A): ◀ ▶ and PARTY beige, CLOSE dimmed (`DevDim(&kDevGridExit)`).

- [ ] **Step 1: Write the failing tests** (`box` suite):

```c
for (i = 0; i < 6; ++i) AssertRowHits(BoxHitV, &pv, DevBoxPartyRow(i), ST_HIT_PARTY_BASE + i);
AssertHits(BoxHitV, &pv, kDevGridExit, ST_HIT_PARTY_BASE + 6); AssertKey(kDevBoxParty, DEV_LOOK_FAINT, 0);
assert(BoxHitV(&pv, 250, 125) == ST_HIT_NONE && BoxHitV(&pv, 210, 20) == ST_HIT_NONE);
pv.box.pos = 6; DevCheckTouch(&po, &pv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
r = DevBoxPartyRow(1); assert(Count(r.x + 22, r.y + 8, 60, 9, DEV_INK) > 0 && Count(r.x + r.w - 30, r.y + 8, 24, 9, DEV_INK) > 0);   /* name, Lv */
AssertKey(kDevBoxChoose, DEV_LOOK_KEY, 1); AssertKey(kDevBoxParty, DEV_LOOK_FAINT, 0);                            /* cv: CHOOSE */
assert(at(250, 200) == DIM_DARK && BoxHitV(&xv, 100, 100) == PT_BOX_HIT_MESSAGE); AssertKey(kDevBoxPrev, DEV_LOOK_FAINT, 0); /* xv: MESSAGE */
```

Layout: `DevBoxPartyRow(0..5)` inside the box's paper; the group "box Party" {`kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevBoxParty`, `DevBoxSide`, `DevBoxChooser`, `CtrPokeTouch_BoxHit`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `box_{party,choose,message}` (no board: the legend). Azahar: the cursor into the Party (rows, CANCEL in the corner), the chooser (JUMP), a message (a tap anywhere). Commit `feat: the PC box's Party, chooser and messages by the legend`.

### Task 15: Easy Chat's phrase: CLEAR / CONFIRM / CANCEL

**Files:**
- Modify: `3ds_device_layout.h` (`DevEcBottom`; delete `DevEcFooter`), `3ds_device_easychat_impl.h`
- Test: `device_suite_easychat.h` (edit block), `device_layout_test.c` (`EasyChatFaults`, the phrase groups)

**Interfaces:**
- Consumes: `DevTitleLine`, `DevCancelWord`.
- Produces: `DevEcBottom(int i, int y, int h)` = i 0 {8, y, 108, h}, 1 {124, y, 112, h}, 2 {244, y, 68, h}. The phrase's footer `DevEcBottom(i, 178, 52)`: 0 CLEAR (`PT_EC_HIT_DELETE`), 1 CONFIRM (`PT_EC_HIT_CONFIRM`), 2 CANCEL dark (`DevCancelWord()`, `PT_EC_HIT_CANCEL`); `kButton` (non-static since 04a) holds the words in the drawn order {CLEAR, CONFIRM, `DevCancelWord()`}; `static const int16_t kDevEcFooterCode[3]` maps the drawn place to its code and `static const int8_t kDevEcFooterPlace[3] = {0, 2, 1}` FireRed's footer column (`e->footer`: 0 DELETE ALL, 1 QUIT, 2 OK) to the place it sinks (the D-pad still walks FireRed's order). The phrase's title `DevTitleLine(&kDevEcPhrase, e->title, count)` (the count ending at 298).

- [ ] **Step 1: Write the failing tests** (`easychat` suite):

```c
static const int kCode[3] = {PT_EC_HIT_DELETE, PT_EC_HIT_CONFIRM, PT_EC_HIT_CANCEL};
for (i = 0; i < 3; ++i) AssertHits(EcHitV, &v, DevEcBottom(i, 178, 52), kCode[i]);
AssertKey(DevEcBottom(2, 178, 52), DEV_LOOK_DARK, 0); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 0);
v.ec.footer = 1; DevCheckTouch(&old, &v); AssertKey(DevEcBottom(2, 178, 52), DEV_LOOK_DARK, 1);      /* QUIT: the corner */
v.ec.footer = 2; DevCheckTouch(&old, &v); AssertKey(DevEcBottom(1, 178, 52), DEV_LOOK_KEY, 1);       /* OK: the middle */
assert(Count(250, 23, 48, 7, DEV_INK2) > 0 && Count(299, 14, 7, 24, DEV_INK2) == 0);
```

Layout `EasyChatFaults`: `Same(DevEcBottom(0, 178, 52), {8,178,108,52})`, `(1, …) {124,178,112,52}`, `(2, …) {244,178,68,52}`; the groups "Easy Chat phrase of n" use them.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevEcEdit`, `DevEcTitle` goes for `DevTitleLine`, `CtrPokeTouch_EasyHit`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS (`easychat_plan_test` unchanged: the codes are the same).
- [ ] **Step 5: Task close** — renders `easychat_{edit,edit_four,ask,ask_delete}` vs `def_easychat_edit.txt` (the Interfaces' rects and capitals). Azahar: a profile's phrase — CLEAR (its question), CONFIRM, CANCEL by touch and by B; the D-pad on the footer. Commit `feat: Easy Chat's phrase footer CLEAR / CONFIRM / CANCEL`.

### Task 16: Easy Chat's word list without the letter strip

**Files:**
- Modify: `3ds_device_layout.h` (`kDevEcUp`, `kDevEcDown`; delete `kDevEcStrip`, `DevEcStripCell`, `DevEcListKey`), `3ds_device_easychat_impl.h`, `3ds_port/include/3ds_poketouch.h` (delete `PT_EC_HIT_STRIP`, `CtrPokeTouchEasy.letter`, `CtrPokeTouch_EasyStripLetter`), `3ds_bottom_easychat_impl.h` (the strip's branch, `EasyLetterOf`, the fill's `letter`)
- Test: `device_suite_easychat.h` (list blocks; the strip's ~147-173), `device_layout_test.c` (`EasyChatFaults`' strip checks), `3ds_port/tests/easychat_plan_test.c` (the strip's part of its touch test, ~421-464; the rail's stays)

**Interfaces:**
- Consumes: `DevEcBottom`, `DevArrowKeys`.
- Produces: `kDevEcUp {252,52,60,62}`, `kDevEcDown {252,122,60,62}` (`DevArrowKeys(&kDevEcUp, &kDevEcDown, first > 0, first + PT_EC_ROWS < rows, …, 0)`, grey and not hit at the window's ends); the list's bottom `DevEcBottom(i, 196, 34)`: USE WORD (`PT_EC_HIT_PLACE`), the toggle (`PT_EC_HIT_TOGGLE`), BACK dark (`PT_EC_HIT_BACK`). A row's word at capitals r.y + 6. `DevEcPhraseLine` writes `"____"` for an empty slot (the list's and the groups' phrase line). The strip, its hit code, its field, its letter function and its touch branch go; the rail stays a slider.

- [ ] **Step 1: Write the failing tests** (`easychat` suite):

```c
static const int kList[3] = {PT_EC_HIT_PLACE, PT_EC_HIT_TOGGLE, PT_EC_HIT_BACK};
for (i = 0; i < 3; ++i) AssertHits(EcHitV, &lv, DevEcBottom(i, 196, 34), kList[i]);
lv.ec.first = 0; DevCheckTouch(&lo, &lv); assert(EcHitV(&lv, 282, 80) == -1 && Count(252, 52, 60, 62, DEV_ARROW_OFF) > 0);
AssertHits(EcHitV, &lv, kDevEcDown, PT_EC_HIT_DOWN); lv.ec.first = 1; DevCheckTouch(&lo, &lv); AssertHits(EcHitV, &lv, kDevEcUp, PT_EC_HIT_UP);
r = DevEcRow(0); assert(Count(r.x + 4, r.y + 6, 40, 9, DEV_INK) > 0 && Count(r.x + 4, r.y + 5, 40, 1, DEV_INK) == 0);
char line[PT_EC_WORDS * PT_EC_WORD_LEN]; lv.ec.numWords = 4; strcpy(lv.ec.chip[0], "Hello"); strcpy(lv.ec.chip[1], "Great");
lv.ec.chip[2][0] = 0; strcpy(lv.ec.chip[3], "Ember"); DevEcPhraseLine(&lv.ec, line, sizeof(line)); assert(!strcmp(line, "HELLO GREAT ____ EMBER"));
```

`easychat_plan_test.c`: the strip's taps go from `test_touch`; the rail's scrub at both ends stays.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_easychat_plan` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevEcList`, `DevEcRowDraw`, `DevEcPhraseLine`, `CtrPokeTouch_EasyHit`, `ProcessEasyTouch`, `EasyFill`; `DevEcArrow` goes for `DevArrowKeys`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `easychat_{list,list_alpha,list_end,list_short}` vs `def_easychat_list.txt` (the Interfaces' rects and capitals). Azahar: a group's words — rows, the rail, ▲▼ at both ends, USE WORD, A TO Z, BACK. Commit `feat: Easy Chat's word list on the Definitiva, the letter strip gone`.

### Task 17: Easy Chat's groups in a window of three columns; the letters

**Files:**
- Modify: `3ds_device_layout.h` (`DevEcCell`, `DEV_EC_GROUP_ROWS`; delete `DevEcToggle`), `3ds_device_easychat_impl.h`, `3ds_port/include/3ds_poketouch.h` (`CtrPokeTouchEasy.groupFirst`, `CtrPokeTouch_EasyGroupStep`), `3ds_bottom_easychat_impl.h` (`sEcGroupFirst`, `EasyReset`, `EasyFill`, `ProcessEasyTouch`)
- Test: `device_suite_easychat.h` (groups, groups14, letters), `device_layout_test.c` (`EasyChatFaults`, the grid groups), `easychat_plan_test.c` (`test_group_window`)

**Interfaces:**
- Consumes: `kDevGridTitle`, `kDevGridUp/Down`, `kDevGridNote`, `kDevGridExit`, `DevArrowKeys`, `DevTitleLine`, `DevKeySmallWord`, `DevReadText`, `DevBackWord`.
- Produces:
  - `DevEcCell(int i)` = {8 + 88(i % 3), 40 + 29(i / 3), 84, 24}, i 0-14 (a place in the window); `#define DEV_EC_GROUP_ROWS 5`. `DevEcLetter` unchanged.
  - `uint8_t groupFirst;` in `CtrPokeTouchEasy` — the window's first row of three. `static int sEcGroupFirst` in 3ds_bottom_easychat_impl.h, 0 in `EasyReset` and on entering the groups; `EasyFill`: rows = (groupCount + 2) / 3; while FireRed's cursor is on a group, the least move that keeps its row (cell / 3) within [first, first + 4]; clamped to [0, max(0, rows − 5)]; stored back and filled.
  - `int CtrPokeTouch_EasyGroupStep(const CtrPokeTouchEasy *e, int down)` (drawer TU, declared in 3ds_poketouch.h) — the group ▲ (down 0) / ▼ walks FireRed's cursor to: from base = `cell` when it is a group, else `3 · groupFirst`; ▲ base − 3 when base / 3 > 0, ▼ min(base + 3, groupCount − 1) when base / 3 < (groupCount − 1) / 3; else −1.
  - Groups: the title `DevTitleLine(&kDevGridTitle, phrase line, count)`; group i of [3·first, min(groupCount, 3·first + 15)) at `DevEcCell(i − 3·first)`, its word `DevKeySmallWord` (capitals faceY + 9), hit `PT_EC_HIT_CELL + i`; ▲▼ `DevArrowKeys(&kDevGridUp, &kDevGridDown, step(0) ≥ 0, step(1) ≥ 0, …)`, hit `PT_EC_HIT_UP` / `_DOWN` only when on; the footer `kDevGridNote`, `DevReadText(…, &paper, 20, 224, paper.y + 2, DEV_SMALL_LINE)` of `CTR_TR("CHOOSE A GROUP.", "ELIGE UN GRUPO.", "W\xC3\x84HLE EINE GRUPPE.")`; BACK `kDevGridExit` (`PT_EC_HIT_CELL + PT_EC_CELL_BACK`). Letters: `DevEcLetter`, the same footer with `CTR_TR("CHOOSE A LETTER.", "ELIGE UNA LETRA.", "W\xC3\x84HLE EINEN BUCHSTABEN.")`, BACK. The toggle is neither drawn nor hit (`PT_EC_CELL_TOGGLE` stays the fill's value for FireRed's blue box; its SELECT branch in the touch goes).
  - `ProcessEasyTouch` (groups): `PT_EC_HIT_UP` / `PT_EC_HIT_DOWN` → `EasyPlanStart(EP_GROUP, CtrPokeTouch_EasyGroupStep(e, code == PT_EC_HIT_DOWN), 0)` when ≥ 0.

- [ ] **Step 1: Write the failing tests.** `easychat` suite (Review Focus 3; 21 groups):

```c
v.ec.cell = 0; DevCheckTouch(&old, &v);
for (i = 0; i < 15; ++i) AssertHits(EcHitV, &v, DevEcCell(i), PT_EC_HIT_CELL + i);
assert(EcHitV(&v, 294, 73) == -1 && Count(276, 40, 36, 66, DEV_ARROW_OFF) > 0); AssertHits(EcHitV, &v, kDevGridDown, PT_EC_HIT_DOWN);
AssertHits(EcHitV, &v, kDevGridExit, PT_EC_HIT_CELL + PT_EC_CELL_BACK); assert(Count(20, 206, 200, 9, DEV_INK) > 0);
assert(CtrPokeTouch_EasyGroupStep(&v.ec, 1) == 3 && CtrPokeTouch_EasyGroupStep(&v.ec, 0) == -1);
v.ec.cell = 19; v.ec.groupFirst = 2; DevCheckTouch(&old, &v);
AssertHits(EcHitV, &v, DevEcCell(13), PT_EC_HIT_CELL + 19); AssertKey(DevEcCell(13), DEV_LOOK_KEY, 1);
assert(CtrPokeTouch_EasyGroupStep(&v.ec, 1) == -1 && CtrPokeTouch_EasyGroupStep(&v.ec, 0) == 16 && EcHitV(&v, 294, 147) == -1);
v.ec.groupFirst ^= 1; assert(DevWhole(DevCheckTouch(&old, &v)));
/* groups14: one window */ assert(Count(276, 40, 36, 140, DEV_ARROW_OFF) > 0 && EcHitV(&gv, 294, 73) == -1 && EcHitV(&gv, 294, 147) == -1);
/* letters */ for (i = 0; i < PT_EC_LETTERS; ++i) AssertHits(EcHitV, &tv, DevEcLetter(i), PT_EC_HIT_CELL + i);
AssertHits(EcHitV, &tv, kDevGridExit, PT_EC_HIT_CELL + PT_EC_CELL_BACK);
```

`easychat_plan_test.c` `test_group_window`: `game_open(21, …)`; the D-pad (`game_group_key`) down to group 18 → the fill's `groupFirst` 2; up to group 0 → 0; `tap(PT_EC_HIT_DOWN)` at group 0 walks FireRed's cursor to group 3 and nothing is chosen (still the groups' state); `tap(PT_EC_HIT_UP)` at group 0 does nothing. Layout `EasyChatFaults`: `DevEcCell(14)`'s drop ends ≤ 187, cells apart (gap 4 across, 5 down); groups "Easy Chat groups" {15 cells, `kDevGridUp`, `kDevGridDown`, `kDevGridExit`}, "Easy Chat letters" {26 letters, `kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_easychat_plan` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevEcGrid`, `DevEcCells`, `DevEcCellKey`, `CtrPokeTouch_EasyHit`, `EasyFill`, `EasyReset`, `ProcessEasyTouch`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `easychat_{groups,groups14,letters}` vs `def_easychat_groups.txt` (the Interfaces' rects and capitals). Azahar: BY GROUP — the window by ▲▼ and by the D-pad to the last groups, a group, BACK; A TO Z's letters and BACK. Commit `feat: Easy Chat's groups in a window of three columns with ▲▼`.

### Task 18 (optional): Boards of HOME, the battle, the Party, the Summary and the Bag

**Files:**
- Modify: `3ds_device_{home,battle,party,summary,bag}_impl.h` (raw `Fill` / `Px` → `DevRect`, or behind `DevArt`), `3ds_device_kit_impl.h` (`DEV_ART_MAP` appended), `device_render_test.c` (`kArt` gains "map"; `TRACE_MAX` if a state loses pieces), `tools/make_device_boards.py` (`SUITES`, `VIEWS`, `ART['map'] = 'MAPA'`, the docstring), `tools/test_make_device_boards.py`, `tools/test_device_views.py`

**Interfaces:**
- Produces: every pixel those five draw is a traced piece: a plain fill a `DevRect`; art (HOME's map and icons, a Pokémon's icon or picture, an item's icon) announced with `DevArt` in the function that draws it (`DEV_ART_MAP` for HOME's map). `SUITES` gains `home`, `battle`, `party`, `summary`, `bag` (`VIEWS`: Início, Batalha, POKéMON, Resumo, BAG).

- [ ] **Step 1: Write the failing tests.** `test_device_views.py`:

```python
def test_traced_views_paint_through_the_kit(self):
    """HOME, the battle, the Party, the Summary and the Bag paint only through traced pieces: a plain fill is a DevRect,
    a pixel or an icon belongs to art its function announces with DevArt (plan 04b Task 18)."""
    for view in ('home', 'battle', 'party', 'summary', 'bag'):
        text = (ROOT / f'3ds_port/src/3ds_device_{view}_impl.h').read_text(encoding='utf-8')
        self.assertNotRegex(text, r'\bFill\(', view)
        for body in text.split('\n}\n'):
            if re.search(r'\bPx\(|UiImage\(|DevMonIcon\(|DevItemIcon\(', body):
                self.assertIn('DevArt(', body, view)
```

`test_untraced_suites_are_refused`: `SUITES` is the 18, and only `kit` and `nosuch` are refused; `test_every_boarded_state_is_traced` then covers the five.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_make_device_boards` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (renders unchanged: `DevRect` paints what `Fill` did).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS; `python tools/dev.py render` → no PNG differs; `python tools/dev.py boards` → boards of the five.
- [ ] **Step 5: Task close** — Azahar: HOME, a battle, the Party, the Summary, the Bag as before. Commit `feat: boards of HOME, the battle, the Party, the Summary and the Bag`.

### Task 19: "B — como ficou" made again

**Files:**
- Modify: only what the comparison finds (a fix in the view's task's files, with its test).

- [ ] **Step 1: Make the boards** — `python tools/dev.py boards` → one `.dc.html` per state of every suite in `SUITES` under `build/boards/` and `build/boards/index.json`.
- [ ] **Step 2: Compare** — dump them (`python -I .progress/design-b/board_spec.py build/boards/*.dc.html` into a scratch folder) and, for each Definitiva board of rows 3-5 (and rows 1-2 with Task 18), set its state's lines beside the def-spec lines; any decided rect or text off by more than a pixel is fixed in its view with a test pinning it, then Step 1 again.
- [ ] **Step 3: Hand over** — list the boards and the comparison (view, board, what differs and why: a Call above, a pack-width text) in the report. The controller, not the implementer, publishes them (artifact 9cx85k3qavedLkTyKEsAKm, row "B — como ficou").
- [ ] **Step 4: Task close** (when Step 2 changed code) — commit `fix: <view> matches its Definitiva board`.

### Task 20: README, CHANGELOG and the console checklist — then PR F2

**Files:**
- Modify: `README.md`, `CHANGELOG.md`, `docs/HARDWARE-ACCEPTANCE.md`

- [ ] **Step 1: Write** — README (EN): ~151 (OPTIONS' sections keep their names; the page line under the rows, "PAGE 2/3 · LOOKS", with dots; ▲▼ walk all the pages), ~35 and ~88 (the red device as the Definitiva: HOME's eight keys with RUN, SPD and the play time; the PC box on plain paper without its wallpaper, IN HAND, the box's count, the pointed Pokémon's types; Easy Chat's groups in three columns, no letter strip); screenshots stay (the owner's call). CHANGELOG `## Unreleased` `### Changed`: one bullet per area of rows 1-5 (HOME; the Party's one scrolling column of actions; the Summary on four fixed tabs with CONTACT, recipe 0266; the Bag's five rows; the battle's commands, moves with their details in the footer, Safari; the choices with prices, recipe 0267; forget a move; the relearner; OPTIONS; SAVE with the place and a bar; the Trainer Card's colour strip and stars; the Pokédex; the letter; naming's counter; the PC box and its menus, recipe 0268; Easy Chat's footer, groups and words); `### Removed`: the PC box's wallpaper on the lower screen; Easy Chat's letter strip and the groups' A TO Z key (A TO Z stays on the phrase and the word list). `docs/HARDWARE-ACCEPTANCE.md`: a section "## The lower screen (the red device)" with one `- [ ]` line per view of rows 1-5 — what to touch and what must happen (the Azahar lines of 04a's Tasks 3-15 and this plan's Tasks 2-17, in the canvas's order).
- [ ] **Step 2: Check** — `git grep -n -i -E 'wallpaper at 1:1|letter strip|names the page shown|GRAPHICS, LOOKS or WILDS' README.md CHANGELOG.md docs/` shows only history.
- [ ] **Step 3: Task close** — commit `docs: the Definitiva layouts in the README, the changelog and the console checklist`.
- [ ] **Step 4: PR F2** — full check (`python tools/dev.py test`, build, audit, `python tools/dev.py render` every view, `python tools/dev.py boards`, `python tools/dev.py run`); `git push -u origin feat/b-def-naming-box-easychat`; `gh pr create --base feat/b-def-system-views --title "feat: the Definitiva naming, PC box and Easy Chat; boards and docs"`; body: each view's change, recipe 0268, the boards (Task 19's comparison), the docs, the owner's console checklist (the HARDWARE-ACCEPTANCE section), the 🤖 line.

## Not in this plan

- Plan 4's speed (spec 5.7): the box still redraws whole and decodes its 30 icons on each cursor step; Easy Chat's group window adds one redraw per scroll; the Pokédex menu redraws whole on a cursor move.
- The naming keyboard's icons for the player, the rival and the PC (O4: later, a recipe).
- A board for the states drawn by the legend alone (OPTIONS' QOL and NUZLOCKE, SAVE's overwrite and error, the habitat page, the letters, the box's Party / chooser / message / YES / NO, Easy Chat's question): the owner checks them in "B — como ficou".
- Not taken: the board's type colours (FireRed's, Decision); a 14 px key font; the boards' sample prop colours (the card's colour follows its stars).
