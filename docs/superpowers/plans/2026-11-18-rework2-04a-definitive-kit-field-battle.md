# Rework 2, plan 04a: the Definitiva kit, the field's views, battle and questions — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The lower screen's kit, grid and rows 1-2 of the canvas row "B — Definitiva" (HOME, the Party and its menu, the Summary, the Bag and its menu; the battle's commands and moves, the choices, forget a move, the relearner) are drawn as their Definitiva boards, with the two game-side recipes they need (Summary CONTACT, choice prices); every other view keeps working on the same kit.

**Architecture:** First the shared names: the grid of def_spec (TOPO / MEIO / RODAPÉ) as named rects in `3ds_device_layout.h`, and every view that borrows another view's rect or helper moved onto a grid name, its own copy or a kit function, with no pixel changing (Task 1). Then the Definitiva's kit rules in one place (LED, screws, faint ink, colours, words, footer text: Task 2). Then one task per view or state, each moving only its own rects, in recon A's and recon B's order. Fills, hit codes and handlers stay unless a task says otherwise.

**Tech Stack:** C11 (devkitARM `-std=gnu11`, host gcc `-std=c11`, both `-Werror`), Python 3 tools, the pokefirered edit recipes (`tools/make_recipe.py`).

**Spec:** the canvas row "B — Definitiva": rules and row notes `.progress/design-b/def/NOTES.md` (def_spec, def_row1, def_row2), boards dumped one element per line in `.progress/design-b/def-spec/def_*.txt` (screen px; a text box's capitals start +3 in the 11 px font, +2 in the 9 px font). Binding decisions (owner O1-O4, coordinator rulings): `.progress/lower-screen-rework2-definitive.md` § Decisions. Per-view differences: `.progress/rework2-04-recon-D.md` § 2 (the kit), `-A.md` (row 1), `-B.md` (row 2). Plan 3 (`docs/superpowers/plans/2026-11-17-rework2-03-boardless-views-and-ember-removal.md`) and its ledger still hold where nothing here overrides them. Code contract: `3ds_device_layout.h`, `3ds_device_kit_impl.h`.

## Calls made while drafting (for the controller)

1. Recipe numbers follow landing order: **0266** `summary-reports-contact` (E1, Task 6), **0267** `field-choice-prices` (E2, Task 11); plan 04b's box-menu flag becomes 0268.
2. Safari is in the moves task (Task 13): it shares `kDevBatMove` and the moves' top screen, which that task deletes.
3. The Summary's frame and its pages are one task (Task 5): the 260 screen alone would leave every page overflowing it; CONTACT and its recipe are Task 6.
4. Bag quantity (no board): ▲ `{204,40,108,30}`, the count's screen `{204,76,108,36}`, ▼ `{204,118,108,30}`, OK `{204,154,108,30}`, CANCEL in the exit corner.
5. Bag menu states: only the pocket keys go beige; a case's key stays sunk (it is the list shown), a title screen is not dimmed; a message still dims the band.
6. List rows are hit on their own 24 px (plan 3's rule "rows hit on themselves"); the 2 px gaps take nothing (recon D's 26 px band not taken).
7. TARGET (no board): the moves' four keys (the HP bar where the pips are), "Which POKéMON should the move hit?" in the footer, BACK in the exit corner.
8. Party level up (no board): screen `{8,8,304,176}`, the message in the footer, OK cream in the exit corner, up, sunk while a touch is held (the lone-OK rule).
9. Party YES / NO: both keys in the action column, no exit key; picking several per the Decisions.
10. Party menu cue: ▲ centred (258,4), ▼ centred (258,182), cream (`DEV_DARK_INK`) when actions are hidden that way, `DEV_ARROW_OFF` otherwise, drawn only with more than six actions, never hit.
11. HOME: RUN's unlit LED is a 6 px disc in `DEV_KEY_EDGE`; TIME reuses the view's play clock (`saveHours`, `saveMinutes`), now filled for HOME too.
12. Kept, not on the Decisions: `DevLabelChip` stays 9 px tall (no 7 px font); chip text shadows stay `DevChipShadow` (recon D 2.7's single-alpha rule not taken); type colours FireRed's (Decision).
13. Footer chip lines (moves, relearner): chips 10 tall at paper.y + 2, small text from paper.y + 13 in two lines; footers without chips start their small lines at paper.y + 2.
14. DE CANCEL: `DevCancelWord()` and `UIS_CANCEL`'s German both "Abbrechen", so the PC box's CANCEL follows (a word only).
15. Learn hints for the NEW and HM rows (words in Task 14); MEMO rows: NATURE, RIBBONS, then FireRed's memo.
16. `DevShell` takes no argument: four screws on every view, kept so by the compiler.
17. STATS' switch keeps today's 70 px, ending at 256: `kDevSumIvSwitch {186,49,70,13}` (the board's starts at 196, auto width).
18. The move to forget: two description lines (capitals 198, 208) and the hint at 218, as Call 13 (the board: one line at 199, the hint at 211).

## Pre-flight rulings (2026-11-18)

From `.superpowers/sdd/2026-11-18-rework2-04a-definitive-kit-field-battle/preflight.md`, all accepted:

1. `DevBagOnKey` moves to the kit as `DevOnKey` (naming, save added to Files); the borrow test's regex exempts `DevBagShown(` — Task 1.
2. The Pokédex's tab rect is `DevDexTabKey(int i)` (the drawer's `DevDexTab` stays) — Task 1.
3. `test_battle_bag_drags_its_rows`: `b->count = 7`, `gCursor = 6` for 26 px rows — Task 3.
4. The bag suite's row loops run to `PT_BAG_ROWS` / `BUI_BAG_ROWS`, the HM row is `row[4]` — Task 3.
5. The layout test's SharedFaults / BattleFaults lines are named, and L's DevAcross comment follows — Tasks 3, 12, 13.
6. The dex suite leaves `kDevBagPocket` and the Bag's names in Task 1 — Task 1.
7. The Summary's category chip ends by 106 with words that fit (`DevCategoryWord`) — Task 5 (and 13).
8. The priced cut is forced by widened stub glyphs — Task 11.
9. POW / ACC never overlap the footer's chips: past them, or the first small line — Task 13.
10. Easy Chat's `kButton` loses `static` to take `DevCancelWord()` — Task 2.
11. The relearner's description keeps capitals 199 (`paper.y + 3`) — Task 1.
12. "Bag how many" group: ▲, ▼, OK and the exit — Task 4.
13. Calls 17 (STATS' switch) and 18 (the move to forget's footer) — Tasks 5, 14.
14. The party grid keeps `Count`, `Cols`, `Move`; `DevPtyMenuFirst` uses `DEV_PTY_MENU_ROWS` — Task 8.
15. The Bag-narrow and learn fixtures fit three small lines once `DevReadText` counts — Task 2.
16. The move to forget draws on `kDevGridWide` (`kDevLearnScreen` goes); its 22 / 24 rows are the board's exception — Task 14, Global Constraints.
17. The relearner's YES / NO cites the Decision, not "def_bag_menu's pattern" — Task 15.
18. Kit `DevCategoryWord` and `DevPowAcc`, used by the Summary, the battle's footer and the relearner — Tasks 2, 5, 13, 15.
19. The grid rule names SAVE's `kDevHomeScreen` (04b's) and the dispatcher's `…Shown` predicates — Global Constraints.
20. Wording: Task 7's test reads `3ds_bottom_battle_fill_impl.h`; naming's LED needs no change (Task 2); the recipe hints say "every recipe" — Tasks 2, 6, 7, 11.

## Global Constraints

- Commits: English Conventional Commits ending with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`; PR bodies end with `🤖 Generated with [Claude Code](https://claude.com/claude-code)`. No force-push (stacked branches are updated by merging). Files added by name; never `.dev.local.json`, `.progress/`, `.superpowers/`, `build/`.
- Before every commit: `python tools/manifest_update.py && python tools/source_audit.py && python tools/source_audit.py --history-since 0afd7a2bdb75a7d1ec2d4d10fb9a84f6f23e08ab` → exit 0; every file ≤ 262,144 bytes; `3ds_video.c` (258,680) is not edited; `device_render_test.c` (149,963) may move a suite it rewrites into its own `device_suite_<view>.h` (plan 3's pattern) rather than grow past ~200 KB.
- Nothing ROM-derived committed: renders stay under `build/`; test fixtures carry port-written sentences, not FireRed's descriptions.
- `-Werror`: what a task orphans (a rect, a static function, a colour, a view field) it deletes; `device_render_test.c` includes all of `3ds_poketouch.c` and compiles at every commit.
- The legend (def_spec + Decisions): a cream key is chosen (sunk 2 px, its LED 4x4 at (x + w − 11, faceY + 4)); one reads an embedded screen; one leaves by the dark key in the exit corner (BACK a level, CLOSE an app, CANCEL an action); an unused cream key is beige (`DEV_LOOK_FAINT`, ink `#8A8478`), a disabled dark exit key is dimmed (`DevDim`); a lone OK is up, sunk only under a touch; four screws on every view; a list's rows 24 tall, 26 apart, five shown, the chosen inverted (the move to forget: the board's 22 / 24); ▲▼ grey (`DEV_ARROW_OFF`) at the ends and then not hit.
- The grid (def_spec): TOPO y 8-32 (`DevGridTab`, `kDevGridTitle`), MEIO y 40-184 (`kDevGridScreen` + `kDevGridUp`/`kDevGridDown`, or `kDevGridWide`, or a narrow screen with the key column at x 204), RODAPÉ y 190-232 (`kDevGridNote` + `kDevGridExit`, or `kDevGridNoteWide`). Every drawn or hit rectangle is the layout header's; a view never draws with another view's rect or helper (`test_views_borrow_no_view_rects`, Task 1; SAVE's `kDevHomeScreen` until 04b's SAVE task; the dispatcher's `…Shown` predicates are not borrowing); keys hit on `DevKeyArea`, rows and screens on themselves.
- Words (decision 5): port words UPPER CASE in all three `CTR_TR` languages with the board's wording; FireRed's strings as FireRed has them (`DevUpper` for those that arrive title-cased); key words in the normal font (no 14 px); exits `DevBackWord()`, `DevCloseWord()`, `DevCancelWord()` (DE "ABBRECHEN"). Type colours: FireRed's (`UiTypeColor`).
- Redraw (decision 5): a view draws whole when a field it draws changes; `Update` == `Draw` pixel for pixel; a change of an undrawn field presents nothing (each suite asserts it: `v.mapGroup ^= 1; v.speed ^= 1;` on touch views, the view's own undrawn fields on battle and Summary views).
- Every key ≥ 28x20 and the keys of one state apart with their drops (`device_layout_test.c`); every suite ends `assert(sDevKeyCut == 0)` and passes in the ES and DE label-fit builds (`test_german_labels_fit`, `test_spanish_labels_fit`).
- Touch keeps doing what it does: hit codes, handlers, press/release timing stay unless a task's Interfaces change them; every moved key and row is pinned by `AssertHits` / `AssertRowHits`.
- Art a view draws goes through `DevArt`, a drawn fill through `DevRect` (the boards show both).
- Recipes (O4, the owner was told): 0266 (Task 6) and 0267 (Task 11), each made with `python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/<NNNN-name>.json <files>` against a `build/dev` that holds every earlier recipe, then `python tools/dev.py build`.
- **The task close** (every task, after its own steps): `python tools/dev.py build`; `python tools/dev.py test` (all PASS; one python suite: `python tools/dev.py test <module>`); the audit line; sizes; `python tools/dev.py render <views>` → read each PNG named in the task beside its board's def-spec lines and fix what the task decided that is off by more than a pixel; `python tools/dev.py run` (builds, copies to Azahar, starts it) and the task's Azahar check; commit with the task's message.
- PRs: **E1** `feat/b-def-kit-field` (base `feat/b-ember-removal`, PR #12) = Tasks 1-9; **E2** `feat/b-def-battle-questions` (base E1) = Tasks 10-15.

## Review Focus

1. Every Bag-page list with five rows at its top, its end and on CLOSE (the field Bag, TM Case, Berry Pouch, a mart's BUY, the item PC, the mailbox, the battle's Bag): the window FireRed's cursor gives, the fifth row hit, ▼ grey and not hit at the end, CLOSE sunk on its row, a drag one item per 26 px — `BagEveryList` and `test_bag_list_drags_a_row_per_grid_row` (Task 3).
2. The Party's menu with seven actions and CANCEL (HEAL / REMEMBER on, two field moves): the window follows FireRed's cursor to the seventh action and to CANCEL, CANCEL in the exit corner hits the dark row, the cue shows what is hidden, YES / NO has no exit key, the D-pad walks FireRed's own list — `party` suite's `menu8` block and `party_grid_test` (Task 8).
3. Priced choices: a label that starts with `{FONT_SMALL}` and has a `CLEAR_TO` (the Game Corner's coins), a six-row priced prize menu in two columns, an unpriced CANCEL row, a label and price too wide in ES/DE (cut, counted) — `test_choice_prices_split` and the `choice` suite's priced block (Task 11).
4. The commands of a wild battle with the quick ball (four keys of 73) and of a trainer (three of 96) at y 168, the ball's arrows on the 62-tall key, RUN blocked, IDLE — `battle` suite's ACTION block (Task 12).
5. The Summary's move details: ▲▼ grey and not hit, the CANCEL row chosen, CONTACT on a status move and on a physical one, an egg's INFO — `summary` suite (Tasks 5, 6).

## Decisions applied (binding; where)

O1 battle row 4 keys wild / 3 trainer, y 168 h 62 — Task 12. O2 Party menu one scrolling column, `CtrPartyGrid_Cols()` → 1 — Task 8. O3 PC box paper — plan 04b. O4 recipes now — Tasks 6, 11 (box flag 04b). Coordinator rulings: screws everywhere, lone OK up, normal-font key words, FireRed's type colours, lists 24/26/5, faint keys beige / dark exit dimmed, DE ABBRECHEN — Tasks 1-2; HOME TIME = play clock — Task 9; moves hint dropped, Safari footer across, choices' grids / 5-8 / prices / 2-line descriptions — Tasks 10-13; learn hint per row, relearn YES/NO as def_bag_menu — Tasks 14-15; Party picking several, Bag YES/NO and quantity's CANCEL — Tasks 4, 7; Summary ▲▼ grey in details, "No." beside / under chips, CANCEL row kept, MEMO = NATURE, RIBBONS, memo, RUN LED off = dim disc, party menu cue — Tasks 5, 8, 9.

---

### Task 1: The grid's names, and every borrowed rect and helper moved (no pixel changes)

**Files:**
- Modify: `3ds_port/include/3ds_device_layout.h`, `3ds_port/src/3ds_device_kit_impl.h`, `3ds_device_{bag,summary,dex,options,relearn,learn,frame,card,mail,box,choice,easychat,naming,save}_impl.h`
- Test: `3ds_port/tests/device_layout_test.c`, `device_render_test.c` (`kit`, `summary`, `bag`), `device_suite_{dex,options,relearn,learn,frame,card,easychat}.h`, `tools/test_device_views.py`

**Interfaces:**
- Produces (layout, the grid; values from def_spec):
  - `#define DEV_GRID_ROW_H 24`, `#define DEV_GRID_ROW_PITCH 26`, `#define DEV_GRID_ROWS 5` (the pitch is also the drag constants' value: `BAG_DRAG_ROW` in Task 3; `LIST_DRAG_ROW` with the Pokédex in 04b).
  - `kDevGridTitle {8,6,304,28}`, `kDevGridScreen {8,40,260,144}`, `kDevGridScreenNarrow {8,40,188,144}`, `kDevGridWide {8,40,304,144}`, `kDevGridUp {276,40,36,66}`, `kDevGridDown {276,114,36,66}`, `kDevGridNote {8,190,228,42}`, `kDevGridNoteWide {8,190,304,42}`, `kDevGridExit {244,190,68,40}`.
  - `static inline CtrPokeTouchRect DevGridTab(int n, int i)` = {8 + (w + 4)·i, 8, w, 24}, w = (304 − 4(n − 1)) / n (n 3: 98 wide, the last ending at 310; n 4: 73, = `DevAcross(4, i, 8, 24)`).
  - `static inline CtrPokeTouchRect DevGridRow(int i, int narrow)` = {16, 48 + 26i, narrow ? 172 : 244, 24}, i 0-4.
  - `static inline CtrPokeTouchRect DevGridColumnKey(int top, int pitch, int h, int i)` = {204, top + pitch·i, 108, h}.
  - Renamed, same values: `kDevTallNote {8,188,228,44}` and `kDevTallBack {244,188,68,42}` (were `kDevSumNote` / `kDevSumBack`: the card's and the Pokédex page's BACK by their boards; the frame, the move to forget and the Summary until their tasks).
  - Own copies with today's values: the move to forget `kDevLearnScreen {8,40,304,140}`, `kDevLearnColumn {14,46,104,128}`, `kDevLearnPic {34,82,64,64}`, `DevLearnSlot(int i)` = {124, 52 + 24i, 176, 22}; the relearner `kDevRlNarrow {8,40,184,144}`, `kDevRlDescNarrow {8,190,184,42}`, `DevRlKey(int i)` = i ? {200,78,112,38} : {200,40,112,30}, and `DevRlDesc`'s top `paper.y + 3` (was `kDevBagDescText.y + 2`, capitals 199); the Pokédex `DevDexTabKey(int i)` = `DevAcross(3, i, 8, 24)` (the drawer's `DevDexTab(const CtrPokeTouchDex *)`, which tab is shown, keeps its name).
  - Deleted (every user on a grid name): `kDevBagTitle`, `kDevBagList`, `kDevBagUp`, `kDevBagDown`, `kDevBagDesc`, `kDevBagClose`, `kDevSumNote`, `kDevSumBack`. The Bag's own stay the Bag's (`kDevBagPocket`, `kDevBagCaseKey`, `kDevBagListNarrow`, `kDevBagDescNarrow`, `kDevBagDescIcon`, `kDevBagDescText`, `kDevBagQty*`, `DevBagRow`, `DevBagMenuKey`, `DEV_BAG_ROW_H`); the Summary's own (`kDevSumTab`, `kDevSumPrev/Next`, `kDevSumScreen/Column/Pic`, `kDevSumIvSwitch*`, `DevSumRow`) the Summary's. `kDevAskNote/Yes/No` keep their literal values (no longer equal to `kDevHomeKey` by test).
- Produces (kit, moved from the view files, bodies and pixels unchanged; callers pass today's values):
  - `#define DEV_ARROW_OFF UI_RGB(0xB8, 0xB0, 0xA0)` (was `DEV_BAG_ARROW_OFF`); `#define DEV_SMALL_LINE_TIGHT 9` (was `DEV_BAG_SMALL_LINE`: the frame's hints, OPTIONS and the PC box keep it).
  - `UI_FN const char *DevCloseWord(void)` (was `DevBagCloseWord`); `UI_FN void DevItemIcon(unsigned item, int x, int y, int half)` (was `DevBagIcon`).
  - `UI_FN int DevOnKey(const CtrPokeTouchRect *r, int x, int y)` (was `DevBagOnKey`: `UiIn` of `DevKeyArea(r)`; its callers in bag, box, card, choice, dex, easychat, naming, options, relearn, save).
  - `UI_FN int DevSmallWrapRun(const char *s, int firstX, int x, int top, int right, int maxLines, int pitch, uint16_t ink, int draw)` and `UI_FN int DevSmallWrap(const char *s, int firstX, int x, int top, int right, int maxLines, int pitch, uint16_t ink)` (were `DevBagSmallWrap(Run)` at pitch 9: callers pass `DEV_SMALL_LINE_TIGHT`).
  - `UI_FN void DevReadText(const char *text, const CtrPokeTouchRect *paper, int x, int right, int smallTop, int pitch)` (was `DevBagRead`: two normal lines centred down the paper, else three small lines from `smallTop`; the Bag passes `kDevBagDescText.y + 2`, the move to forget `paper.y + 5`, both `DEV_SMALL_LINE_TIGHT`).
  - `UI_FN void DevKeyWordAtInk(const CtrPokeTouchRect *r, int faceY, int sunk, const char *s, int inset, uint16_t ink, uint16_t shadow)` and `UI_FN void DevKeyWordAt(const CtrPokeTouchRect *r, int faceY, int look, int sunk, const char *s, int inset)`: inset 0 is `DevKeyWord(Ink)`; else the word from r->x + inset when it fits before the LED's box, else `DevKeyWord(Ink)` (was `DevBagWord(…, left)` with x + 12: callers pass 12).
  - `UI_FN void DevArrowKeys(const CtrPokeTouchRect *r0, const CtrPokeTouchRect *r1, int on0, int on1, int held0, int held1, int across)` (was `DevDexArrows`; the Bag's `DevBagArrows` becomes it plus `DevDim` of both when dimmed).
  - `UI_FN int DevTypeChipWidth(const char *typeName)` and `UI_FN int DevTypeChip(const char *typeName, int type, int x, int y, int h)` (were `DevMoveChipWidth` / `DevMoveChip`: at least 40 wide).
  - `UI_FN void DevColumnRow(int y, const char *label, const char *value, int x, int right)` (was `DevSumColumnRow`, x 20, right 112: label small `DEV_INK2` capitals y + 4, value right-aligned at `right`, capitals y + 3).

- [ ] **Step 1: Confirm the branch** — `git branch --show-current` → `feat/b-def-kit-field` (the controller created it from `feat/b-ember-removal` and committed this plan on it). Then the baseline: `python tools/dev.py render` and `sha256sum build/renders/*/*.png > <scratchpad>/renders-before.txt`.
- [ ] **Step 2: Write the failing tests.** Layout (`SharedFaults` or a new `GridFaults`): `Same(kDevGridScreen, (CtrPokeTouchRect){8,40,260,144})` and each grid rect's value as listed; `for (i = 0; i < 4; ++i) assert(Same(DevGridTab(4, i), DevAcross(4, i, 8, 24)));` `Same(DevGridTab(3, 2), (CtrPokeTouchRect){212,8,98,24})`; `Same(DevGridRow(4, 0), (CtrPokeTouchRect){16,152,244,24})` and every `DevGridRow(i, n)` inside its screen's paper (`kDevGridScreen` / `kDevGridScreenNarrow` less 6 px a side); `Same(DevGridColumnKey(40, 36, 30, 3), (CtrPokeTouchRect){204,148,108,30})`; `kDevGridExit.x == kDevGridNote.x + kDevGridNote.w + 8` and `kDevGridUp.x == kDevGridScreen.x + kDevGridScreen.w + 8`; `kDevAskYes`/`kDevAskNo` pinned to `{216,156,96,36}` / `{216,198,96,34}` (line 423's equality to `kDevHomeKey` goes); the learn and relearn copies equal the values above. `tools/test_device_views.py`:

```python
def test_views_borrow_no_view_rects(self):
    """A view draws with the grid's names, the shared shapes, the kit or its own: the Bag's and the Summary's rects and
    helpers are theirs alone, so one view's task cannot move another view (plan 04a, Task 1). The dispatcher's question
    "is the Bag shown" (DevBagShown) is not a rect or a drawing helper."""
    for path in sorted((ROOT / '3ds_port/src').glob('3ds_device_*_impl.h')):
        text = path.read_text(encoding='utf-8')
        if path.name != '3ds_device_bag_impl.h':
            self.assertNotRegex(text, r'\bkDevBag\w+|\bDevBag(?!Battle|Shown\()[A-Z]\w*\(|\bDEV_BAG_\w+', path.name)
        if path.name != '3ds_device_summary_impl.h':
            self.assertNotRegex(text, r'\bkDevSum\w+|\bDevSum[A-Z]\w*\(', path.name)
```

- [ ] **Step 3: Run** — `python tools/dev.py test test_device_views` → FAIL (compile errors on the new names; the source test).
- [ ] **Step 4: Implement** the names and moves above: rename each user in the drawers and suites (bag, dex, options, relearn on the grid names; learn on its own rects, `kDevGridTitle`, `kDevTallNote`/`kDevTallBack`, `DevColumnRow`, `DevReadText`, `DevTypeChip`; frame, card, dex page on `kDevTall*`; box, mail, choice, easychat, relearn on the moved kit helpers; every `DevBagOnKey` on `DevOnKey`, naming and save included; the dex tabs on `DevDexTabKey`). The suites' expectations change by name only; `device_suite_dex.h` leaves the Bag's names now (`kDevBagPocket[i]` → `DevDexTabKey(i)`, `kDevBagClose/Up/Down` → `kDevGridExit/Up/Down`, `DEV_BAG_ARROW_OFF` → `DEV_ARROW_OFF`), so Task 3 can delete `kDevBagPocket`.
- [ ] **Step 5: Run** — `python tools/dev.py test` → all PASS; `python tools/dev.py render` and `sha256sum -c <scratchpad>/renders-before.txt` → every PNG unchanged.
- [ ] **Step 6: Task close** — renders: none differ (Step 5); Azahar: open the Bag, OPTIONS, the Pokédex, a relearner, a move to forget, FireRed's picture; all as before. Commit `refactor: the grid's names, and no view borrows another's rects`.

### Task 2: The Definitiva's kit rules

**Files:**
- Modify: `3ds_device_kit_impl.h`, every drawer calling `DevShell` (bag, battle, box, card, choice, dex, easychat, frame, home, learn, mail, naming, options, party, relearn, save, summary), `3ds_device_naming_impl.h` (its page-key LED, ~214: already 4 px at (x + w − 11, faceY + 4), no change; only its suite's probe is checked), `3ds_device_home_impl.h` / `_summary_impl.h` (`DEV_HOME_RULE`, `DEV_SUM_RULE` → `DEV_RULE`), the CANCEL literals in `_bag_impl.h` (how many), `_battle_impl.h`, `_party_impl.h`, `_summary_impl.h`, `_easychat_impl.h`, `3ds_port/include/3ds_ui_strings.h` (`UIS_CANCEL`), `tools/make_device_boards.py`
- Test: `device_render_test.c` (`kit`; every suite probing an LED pixel, a corner, `DEV_FAINT_INK` or a DE CANCEL), `device_suite_*.h` likewise, `tools/test_make_device_boards.py`, `tools/test_device_views.py`

**Interfaces:**
- Consumes: Task 1's kit helpers.
- Produces:
  - `DevKey`: a sunk key's LED is 4x4 at (x + w − 11, faceY + 4) whatever its height; `DevKeyWordInk` / `DevKeyWordAtInk` keep clear of that box (LED x + w − 11 … + 3, its glow one more each side); the naming page key's lone LED and the boards tool's sunk-key LED (`make_device_boards.py`: size 4 at x + w − 11, y + SINK + 4) the same.
  - `UI_FN void DevShell(void)`: the four screws always; its trace `a` is `DEV_SCREWS_TOP | DEV_SCREWS_BOTTOM`.
  - Colours: `DEV_FAINT_INK UI_RGB(0x8A, 0x84, 0x78)`; `DEV_RULE UI_RGB(0xD8, 0xD8, 0xD0)`; `DEV_TAG_STAB UI_RGB(0x5A, 0x64, 0x78)`; `DEV_TAG_NEW UI_RGB(0x2A, 0x7A, 0x3E)`; `DEV_KEY_INK2 UI_RGB(0x6A, 0x6A, 0x6A)` (secondary text on a key: a price, "PP", "STATUS", SPD, a bay's level); `DEV_TITLE_INK UI_RGB(0x3A, 0x3A, 0x3A)` (a footer's title line); `#define DEV_SMALL_LINE 10`.
  - `UI_FN const char *DevCancelWord(void)` = `CTR_TR("CANCEL", "CANCELAR", "ABBRECHEN")`, replacing every `CTR_TR("CANCEL", "CANCELAR", "ZUR\xC3\x9C" "CK")` and Easy Chat's literal (Easy Chat's `kButton` loses `static` to take `DevCancelWord()`: CTR_TR is compile-time, a call is not); `UIS_CANCEL`'s German becomes `"Abbrechen"`.
  - Words more than one drawer builds: `UI_FN const char *DevCategoryWord(int physical)` = physical ? `CTR_TR("PHYSICAL", "F\xC3\x8DSICO", "PHYS.")` : `CTR_TR("SPECIAL", "ESPEC.", "SPEZIAL")` (each ≤ 43 px with its chip's padding at 6 px), replacing the Summary's and the battle's literals; `UI_FN void DevPowAcc(char *out, size_t size, const char *power, const char *accuracy)` = `"%s %s  %s %s"` of `CTR_TR("POW", "POT", "ST\xC3\x84")`, power (or "-"), `CTR_TR("ACC", "PRE", "GEN")`, accuracy (or "-"), replacing the relearner's `snprintf` (Tasks 5, 13, 15 use both).
  - Footer text: `UI_FN void DevNoteTitle(const CtrPokeTouchRect *paper, int x, const char *s)` — `s` in the normal font, capitals from paper->y + 2, `DEV_TITLE_INK` over `DEV_INK_SH`, cut with "..." at paper->x + paper->w − 6 and counted in `sDevKeyCut`; `UI_FN int DevSmallFit(const char *s, int firstX, int x, int top, int right, int maxLines, int pitch, uint16_t ink)` — `DevSmallWrap`, and when the text needs more than `maxLines` lines `++sDevKeyCut`; returns the lines drawn. `DevReadText`'s small branch uses `DevSmallFit` (counts, same pixels).

- [ ] **Step 1: Write the failing tests.** `kit` suite (replacing the `DevShell(0)` corner check and the 5x5 LED probes):

```c
for (int s = 0; s < 4; ++s) { DevBegin(canvas, (uint8_t)s); DevShell();
    assert(at(4, 4) == DevBlend(kShell[s], 0, 90) && at(315, 4) == at(4, 4) && at(4, 234) == at(4, 4) && at(315, 234) == at(4, 4)); }
/* the LED: a 4 px disc (no corner pixels) at (x + w − 11, faceY + 4) */
k = (CtrPokeTouchRect){8, 156, 96, 36}; DevShell(); DevKey(&k, DEV_LOOK_KEY, 1, 6);                  /* faceY 158 */
assert(at(94, 162) == DEV_LED && at(95, 165) == DEV_LED && at(93, 162) != DEV_LED && at(92, 163) != DEV_LED && at(97, 164) != DEV_LED);
k = (CtrPokeTouchRect){8, 8, 304, 84}; DevShell(); DevKey(&k, DEV_LOOK_KEY, 1, 6); assert(at(302, 14) == DEV_LED && at(303, 17) == DEV_LED && at(300, 15) != DEV_LED);
k = (CtrPokeTouchRect){244, 190, 68, 40}; DevShell(); DevKey(&k, DEV_LOOK_DARK, 1, 6); assert(at(302, 196) == DEV_LED);
assert(DevKeyInk(DEV_LOOK_FAINT, 0) == UI_RGB(0x8A, 0x84, 0x78));
static const char kLongWords[] = "Port-written words, more than one small line of this footer's paper can hold at once.";
sDevKeyCut = 0; assert(DevSmallFit(kLongWords, 20, 20, 198, 224, 1, DEV_SMALL_LINE, DEV_INK) == 1 && sDevKeyCut == 1); sDevKeyCut = 0;
```

`test_make_device_boards.py`: a traced sunk key `{'op': 'key', 'x': 8, 'y': 8, 'w': 96, 'h': 40, 'look': 'key', 'sunk': 1, 'radius': 6}` gives its LED span at left 93, top 14, 4x4. `test_device_views.py` `test_cancel_word_is_the_kits`: no drawer holds `CTR_TR("CANCEL", "CANCELAR", "ZUR`; the kit holds `CTR_TR("CANCEL", "CANCELAR", "ABBRECHEN")`; `3ds_ui_strings.h` has `[UIS_CANCEL] = CTR_TR("Cancel", "Cancelar", "Abbrechen")`.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_make_device_boards` → FAIL.
- [ ] **Step 3: Implement** as in Interfaces. Then every suite that probes an LED, a corner or a DE CANCEL follows the kit (known: HOME (198,164), battle (302,114), Party (146,18) and its menu (209,44), Bag (94,15) and (302,48), the naming page key, and the corner pixels of views that had no screws); a suite assertion that fails because of a real drawing bug is fixed in the code, not the test. `DevReadText` now counts a cut: check that the Bag-narrow and move-to-forget fixtures' texts fit three small lines (shorten a port-written fixture that does not).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS (ES and DE builds too).
- [ ] **Step 5: Task close** — `python tools/dev.py render` (every view): four screws on every PNG; every LED 4x4; beige keys' words `#8A8478`; compare `kit/*.png` with `def_spec.txt` (TECLAS: NORMAL, ESCOLHIDA's LED at [455,24,4,4], BACK, OFF). Azahar: HOME, a battle's moves (four screws now), the Bag, a DE build's Party CANCEL reads ABBRECHEN. Commit `feat: the Definitiva's kit: 4 px LED, screws everywhere, beige ink, footer text, CANCEL`.

### Task 3: The Bag's rows: five of 24, 26 apart, on every Bag-page list

**Files:**
- Modify: `3ds_port/include/3ds_poketouch.h` (`PT_BAG_ROWS 5`, `BUI_BAG_ROWS 5`, `CtrPokeTouch_ListFirst`), `3ds_device_layout.h` (delete `kDevBagPocket`, `kDevBagListNarrow`, `kDevBagDescText`, `DevBagRow`, `DEV_BAG_ROW_H`), `3ds_device_bag_impl.h`, `3ds_bottom_field_fill_impl.h` (~174), `3ds_bottom_battle_fill_impl.h` (~162), `3ds_bottom.c` (`BAG_DRAG_ROW 26`)
- Test: `device_render_test.c` (`bag`), `device_layout_test.c` (`BagFaults`, `SharedFaults`, rects, groups), `tools/test_field_presentation.py` (a new test; `test_battle_bag_drags_its_rows`'s fixture)

**Interfaces:**
- Consumes: `DevGridTab`, `DevGridRow`, `kDevGridScreen(Narrow)`, `kDevGridUp/Down`, `kDevGridNote`, `kDevGridExit`, `DevArrowKeys`, `DevSmallWrap`, `DevReadText`, `DEV_SMALL_LINE`.
- Produces: `static inline int CtrPokeTouch_ListFirst(int cursor, int count, int rows)` (3ds_poketouch.h) = clamp(cursor − rows / 2, 0, max(0, count − rows)), used by both Bag fills (`first = CtrPokeTouch_ListFirst(cursor, count, PT_BAG_ROWS)`); `_Static_assert(PT_BAG_ROWS == DEV_GRID_ROWS, …)` in the Bag file. Pockets `DevGridTab(3, i)` (the battle's ITEMS and BERRIES in places 0 and 1); rows `DevGridRow(i, narrow)` on `kDevGridScreen` or `kDevGridScreenNarrow` (the column keys at x 200 stay until Task 4). A row: its icon 12x12 at (r.x + 5, r.y + 6), the name from r.x + 22 capitals r.y + 8 (a long one small, r.y + 9), the count / price right-aligned at r.x + r.w − 6 capitals r.y + 8, an X/Y chip 10 tall at r.y + 7. The description: icon `kDevBagDescIcon`, chips (a TM's type, IN BAG) 9 tall at paper.y + 2, FireRed's text small from x 48 (`DEV_BAG_READ_X`), capitals 198 / 208 / 218 (`DevSmallWrap(…, 3, DEV_SMALL_LINE, DEV_INK)`); the narrow screen's words `DevReadText(…, paper.y + 2, DEV_SMALL_LINE)`. ▲▼ page `PT_BAG_ROWS − 1` = 4 rows (unchanged code); `BAG_DRAG_ROW 26`.

- [ ] **Step 1: Write the failing tests.** `bag` suite, `BagEveryList` (Review Focus 1) — for each of the suite's list states (items, tmcase, pouch, shop, itempc, mailbox; then `BagBattle` through `BatHitV` with `BUI_BAG_ROW`), with nine items:

```c
assert(CtrPokeTouch_ListFirst(0, 9, 5) == 0 && CtrPokeTouch_ListFirst(4, 9, 5) == 2 && CtrPokeTouch_ListFirst(8, 9, 5) == 4
       && CtrPokeTouch_ListFirst(9, 9, 5) == 4 && CtrPokeTouch_ListFirst(1, 3, 5) == 0);
v.bag.first = 0; v.bag.cursor = 0; DevCheckTouch(&old, &v);
for (i = 0; i < 5; ++i) AssertRowHits(BagHitV, &v, DevGridRow(i, 0), PT_BAG_HIT_ROW + i);
assert(CtrPokeTouch_BagHit(&v, 100, 179) == -1 && CtrPokeTouch_BagHit(&v, 294, 73) == -1);                 /* ▲ at the top */
v.bag.first = 4; v.bag.cursor = 8; DevCheckTouch(&old, &v); assert(at(DevGridRow(4, 0).x + 1, DevGridRow(4, 0).y + 12) == DEV_BEZEL);
assert(CtrPokeTouch_BagHit(&v, 294, 147) == -1 && Count(276, 114, 36, 66, DEV_ARROW_OFF) > 0); AssertHits(BagHitV, &v, kDevGridUp, PT_BAG_HIT_UP);
v.bag.cursor = 9; DevCheckTouch(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);                          /* on CLOSE */
```

and the board's pieces on `items`: `AssertHits(BagHitV, &v, DevGridTab(3, i), PT_BAG_HIT_TAB + i)` for i < 3; the description's three lines from x 48 (`Count(48, 198, 176, 7, DEV_INK) > 0 && Count(48, 218, 176, 7, DEV_INK) > 0` with the suite's long description). `test_field_presentation.py`:

```python
def test_bag_list_drags_a_row_per_grid_row(self):
    """The Bag-page lists' rows are the grid's, 26 px apart (3ds_device_layout.h): a drag moves one item per row."""
    bottom = (ROOT / '3ds_port/src/3ds_bottom.c').read_text(encoding='utf-8')
    layout = (ROOT / '3ds_port/include/3ds_device_layout.h').read_text(encoding='utf-8')
    pitch = int(re.search(r'#define DEV_GRID_ROW_PITCH (\d+)', layout).group(1))
    self.assertEqual(int(re.search(r'#define BAG_DRAG_ROW (\d+)', bottom).group(1)), pitch)
```

`test_battle_bag_drags_its_rows`: its fixture's `b->count = 10` becomes `7` and the later `gCursor = 9` becomes `6` (from y 150 a drag to y 0 is six rows of 26, so "far past the end" still reaches `count - 1`); its other numbers are in `BAG_DRAG_ROW` and stay.

Layout: `BagFaults` pins the pockets to `DevGridTab(3, i)` and the narrow rows inside `kDevGridScreenNarrow`'s paper; in `SharedFaults` the line 391 `DevAcross(3, i, 8, 24) == kDevBagPocket[i]` and the loop `DevBagRow(i, narrow) == DevListRow(i, narrow, DEV_BAG_ROW_H)` (~409) go; the group "Bag list" is {3 tabs, `kDevGridUp`, `kDevGridDown`, `kDevGridExit`}. `3ds_device_layout.h`'s `DevAcross` comment stops calling n 3 at y 8 the Bag's pockets (the Pokédex's tabs, `DevDexTabKey`).
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_field_presentation` → FAIL.
- [ ] **Step 3: Implement** per Interfaces; the struct sizes shrink with the constants (`CtrPokeTouchBag.row`, `CtrBattleBag.row`, `DevBagPage.row`), and so do the bag suite's fixtures: every loop over `row[]` (`BagBoard` ~2398, `BagBattle` ~2423, the TM Case 2597, the pouch 2603, the mart 2608, the item PC 2617, the rows' hits 2515) runs to `PT_BAG_ROWS` / `BUI_BAG_ROWS` and the TM Case's HM row is `row[4]` (`kBagItem`'s eight names still cover the five rows) — at `-O1` nothing warns when rows 5-7 overwrite `typed`, `selItem`, ….
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `bag_{items,tmcase,pouch,shop,itempc,mailbox,battle,battle_berries,empty,close,message}` vs `def_bag.txt` (rows [16,48+26i,244,24], icon [21,54,12,12], name x 38, count right 254, description 48 / 198 / 208 / 218). Azahar: each list — the Bag's three pockets, the TM Case, the Berry Pouch, a mart's BUY, the item PC's WITHDRAW ITEM, the mailbox, the battle's Bag: tap rows, ▲▼, drag (one item per 26 px), CLOSE at the end. Commit `feat: the Bag's lists show five 24 px rows on the grid`.

### Task 4: The Bag's item menu, YES / NO and how many

**Files:**
- Modify: `3ds_device_layout.h` (`DevBagMenuKey`, `kDevBagQty*`; delete `kDevBagDescNarrow`, `kDevBagQtyCancel`), `3ds_device_bag_impl.h`
- Test: `device_render_test.c` (`bag`: menu, menu6, yesno, quantity, battle menu), `device_layout_test.c`

**Interfaces:**
- Consumes: `DevGridColumnKey`, `kDevGridExit`, `kDevGridNote`, `DevKeyWordAt`, `DevReadText`, `DevCancelWord`.
- Produces: `DevBagMenuKey(int n, int i)` — key i (< n − 1) of a menu of n rows (FireRed's last row, CANCEL / EXIT, is `kDevGridExit`): n − 1 ≤ 4 → `DevGridColumnKey(40, 36, 30, i)`; n − 1 = 5 → `DevGridColumnKey(40, 28, 24, i)`. The exit key draws FireRed's last row (its name through `DevUpper`, dark) and hits `DEV_BAG_HIT_MENU + n − 1`. YES / NO: YES `DevGridColumnKey(40, 36, 30, 0)` cream, NO `kDevGridExit` dark (`UIS_NO` upper); hits `DEV_BAG_HIT_MENU`, `+ 1`. How many: `kDevBagQtyUp {204,40,108,30}`, `kDevBagQtyScreen {204,76,108,36}`, `kDevBagQtyDown {204,118,108,30}`, `kDevBagQtyOk {204,154,108,30}`, CANCEL `kDevGridExit` (`DevCancelWord()`, hit `DEV_BAG_HIT_QTY_CANCEL`). Action words from x + 10 (`DevKeyWordAt`, inset 10). In ACTIONS / YESNO / QUANTITY the pocket keys but the chosen one are `DEV_LOOK_FAINT` (not hit), the chosen stays sunk with its LED, a case's key stays sunk, a title is not dimmed; MESSAGE keeps today's dim band. The footer is always `kDevGridNote`: the icon and, in a menu state, `read` from x 48 (`DevReadText(…, paper.y + 2, DEV_SMALL_LINE)`).

- [ ] **Step 1: Write the failing tests** (`bag` suite; the menu fixture USE GIVE TOSS REGISTER CANCEL, cursor on USE):

```c
for (i = 0; i < 4; ++i) AssertHits(BagHitV, &v, DevBagMenuKey(5, i), PT_BAG_HIT_MENU + i);
AssertHits(BagHitV, &v, kDevGridExit, PT_BAG_HIT_MENU + 4); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); AssertKey(DevBagMenuKey(5, 0), DEV_LOOK_KEY, 1);
AssertKey(DevGridTab(3, 0), DEV_LOOK_KEY, 1); AssertKey(DevGridTab(3, 1), DEV_LOOK_FAINT, 0); assert(CtrPokeTouch_BagHit(&v, 150, 20) == -1);
assert(Count(16, 48, 172, 130, DEV_INK) == 0 && Count(48, 198, 176, 27, DEV_INK) > 0);       /* the list's words grey; "… is selected." */
/* menu6 */ assert(Same(DevBagMenuKey(6, 4), (CtrPokeTouchRect){204, 152, 108, 24})); AssertHits(BagHitV, &v6, kDevGridExit, PT_BAG_HIT_MENU + 5);
/* YES / NO */ AssertHits(BagHitV, &yv, DevGridColumnKey(40, 36, 30, 0), PT_BAG_HIT_MENU); AssertHits(BagHitV, &yv, kDevGridExit, PT_BAG_HIT_MENU + 1);
/* how many */ AssertHits(BagHitV, &qv, kDevBagQtyOk, PT_BAG_HIT_QTY_OK); AssertHits(BagHitV, &qv, kDevGridExit, PT_BAG_HIT_QTY_CANCEL);
/* the battle's menu (USE, CANCEL) */ AssertHits(BatHitV, &bv, DevBagMenuKey(2, 0), BUI_BAG_MENU); AssertHits(BatHitV, &bv, kDevGridExit, BUI_BAG_MENU + 1);
```

Layout: groups "Bag menu of n" (n 2-6: its keys and `kDevGridExit`), "Bag YES / NO", "Bag how many" ({`kDevBagQtyUp`, `kDevBagQtyDown`, `kDevBagQtyOk`, `kDevGridExit`}); every column key's drop ends ≤ 187.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevBagMenu`, `DevBagQuantity`, `DevBagKeys`, `DevBagDesc`, `DevBagHit`).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `bag_{menu,menu6,yesno,quantity,battle}` vs `def_bag_menu.txt` (USE [204,40,108,30] sunk with LED [301,46], pockets beige, CANCEL [244,190,68,40], footer icon + "… is selected." at 48). Azahar: USE / GIVE / TOSS (how many, YES / NO) / REGISTER, CANCEL by the exit key and by B, the Berry Pouch's CHECK TAG (five actions), the battle's USE. Commit `feat: the Bag's item menu in the column, CANCEL in the exit corner`.

### Task 5: The Summary on the grid: frame, INFO, STATS, MOVES, MEMO

**Files:**
- Modify: `3ds_device_layout.h` (delete `kDevSumTab`, `kDevSumPrev/Next`, `kDevSumScreen`; change `kDevSumColumn`, `kDevSumPic`, `kDevSumIvSwitch(Hit)`, `DevSumRow`), `3ds_device_summary_impl.h`
- Test: `device_render_test.c` (`summary`), `device_layout_test.c` (`SummaryFaults`, rects, group)

**Interfaces:**
- Consumes: `DevGridTab`, `kDevGridScreen`, `kDevGridUp/Down`, `kDevGridNote`, `kDevGridExit`, `DevArrowKeys`, `DevColumnRow`, `DevNoteTitle`, `DevSmallFit`, `DevReadText`, `DevCancelWord`, `DevCategoryWord`, `DEV_RULE`.
- Produces (layout): tabs `DevGridTab(4, i)`; ▲ ▼ `kDevGridUp` / `kDevGridDown` (cream, `DevArrowKeys`; grey and not hit while `DevSumArrowsOn` is 0: the move details, an egg's …); screen `kDevGridScreen`; `kDevSumColumn {14,46,96,132}`; `kDevSumPic {30,80,64,64}` (the 80x80 picture centred on it); `kDevSumIvSwitch {186,49,70,13}` (Call 17), `kDevSumIvSwitchHit {182,46,78,20}`; `DevSumRow(int i)` = {114, 48 + 26i, 146, 24}, i 0-4 (4 the details' CANCEL); the note `kDevGridNote`; BACK `kDevGridExit`.
- Produces (drawer): `DEV_SUM_LEFT 116`, `DEV_SUM_RIGHT 256`, `DEV_SUM_ROW_TOP 53`, `DEV_SUM_ROW_DY 15`, `DEV_SUM_STAT_Y 69` (rows 13 tall, 14 apart).
  - Column (INFO, STATS, MEMO): the name capitals 52 from x 20 (to 106), the gender and "Lv" capitals 66, the picture, type chips 40x12 at (20, 152) (`DevSumTypes`, ending by 106); "No.%s" small `DEV_INK2` at (64, capitals 155) beside a single chip, at (20, capitals 167) under two.
  - INFO (values right at 256, capitals 53 + 15k; labels small at 116, capitals + 1): SPECIES (the species, upper), OT, `CTR_TR("IDNo.", "N.\xC2\xBA ID", "ID-Nr.")` (`d->id`), ITEM, NATURE, ABILITY, HP; `DevHpBar(116, 157, 140, 5, …)`; `DevSumExp(d, 116, 165, 0)`. An egg: the hatch text from (116, 53), 140 wide.
  - STATS: the nature small at (116, capitals 52), the switch; six rows from 69 (labels at 116, bar at 116 + col with col ≥ 48, `barW = min(64, 256 − 28 − barX)`, IV view `− 46`); `DevSumExp(d, 116, 158, 74)`, "NEXT n" small right at 256 capitals 158; the IV view's Hidden Power line at capitals 158.
  - MEMO: NATURE, RIBBONS rows (k 0, 1), then FireRed's memo wrapped from capitals 85, 140 wide, to the paper's foot; an egg as today at the new width.
  - MOVES and the details: the column without the icon — the name capitals 52 (small font past 86 px), "Lv" small `DEV_INK2` capitals 66, `DevRect(20, 78, 84, 1, DEV_RULE)`; on a move in the details its type chip 40x12 at (20, 84) and the category chip after it, `DevSumChip(x, 84, 106, 0, DevCategoryWord(physical), …)` (ending by the column's 106; the kit's words fit there at 6 px, so ES / DE count no cut), `DevColumnRow(102, POWER, …, 20, 104)`, `DevColumnRow(118, ACCURACY, …, 20, 104)`; else the Pokémon's types at y 84. Rows: the type's band `DevRect(r.x, r.y, 4, 24, …)` (on the inverted row too), the name from r.x + 8 capitals r.y + 8, the PP right at r.x + r.w − 4 capitals r.y + 8 (`DevPpInk`), the CANCEL row `DevCancelWord()` from r.x + 8.
  - Footer (`DevScreen(&kDevGridNote)`, paper (14,196,216,30)): INFO / STATS `DevNoteTitle(&paper, 20, ability)` + `DevSmallFit(abilityDesc, 20, 20, 212, 224, 1, DEV_SMALL_LINE, DEV_INK)`; the details' text (description, hint or nothing, today's rule) `DevSmallFit(…, 198, 224, 2, …)`; MOVES and MEMO the hint `DevReadText(…, 20, 224, paper.y + 2, DEV_SMALL_LINE)`.

- [ ] **Step 1: Write the failing tests** (`summary` suite):

```c
for (i = 0; i < 4; ++i) AssertHits(SumHitV, &v, DevGridTab(4, i), SUI_HIT_TAB + i);
AssertHits(SumHitV, &v, kDevGridUp, SUI_HIT_PREV); AssertHits(SumHitV, &v, kDevGridDown, SUI_HIT_NEXT); AssertHits(SumHitV, &v, kDevGridExit, SUI_HIT_BACK);
AssertKey(kDevGridUp, DEV_LOOK_KEY, 0); assert(Count(276, 40, 36, 66, DEV_KEY_INK) > 0 && Count(257, 46, 5, 132, DEV_INK) == 0);   /* nothing past 256 */
assert(Count(64, 155, 40, 7, DEV_INK2) > 0);                                                     /* No. beside one chip */
v.data.typeIds[1] = 2; strcpy(v.data.types[1], "Flying"); DevCheckSummary(&old, &v); assert(Count(20, 167, 40, 7, DEV_INK2) > 0);
/* Review Focus 5: the details — ▲▼ grey, not hit; the CANCEL row chosen; the rows hit */
v.data.page = SUI_PAGE_MOVES_INFO; v.data.cursor = 4; DevCheckSummary(&old, &v);
assert(CtrSummaryUi_Hit(&v, 294, 73) == -1 && Count(276, 40, 36, 66, DEV_ARROW_OFF) > 0 && at(DevSumRow(4).x + 1, DevSumRow(4).y + 12) == DEV_BEZEL);
for (i = 0; i < 5; ++i) AssertRowHits(SumHitV, &v, DevSumRow(i), SUI_HIT_MOVE + i);
assert(at(21, 78) == DEV_RULE);
v.data.egg = 1; v.data.page = SUI_PAGE_INFO; DevCheckSummary(&old, &v); assert(Count(116, 53, 140, 60, DEV_INK) > 0);
```

Layout `SummaryFaults`: `kDevSumColumn` and `DevSumRow(0..4)` inside `kDevGridScreen`'s paper, `DevSumRow(i).x ≥ kDevSumColumn.x + kDevSumColumn.w + 4`, `kDevSumIvSwitchHit` inside the paper; group "Summary" {4 tabs, ▲, ▼, BACK}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevSumKeys`, `DevSumScreen`, `DevSumColumn`, `DevSumMoveColumn`, `DevSumInfo`, `DevSumStats`, `DevSumMemo`, `DevSumMoveRow`, `DevSumNote`, `CtrSummaryUi_Hit`); the arrows' former dark-key code and `DEV_SUM_NOTE_*` go.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `summary_{info,stats,ivev,moves,details,details_cancel,memo,swap,egg,egg_info}` vs `def_sum_info.txt`, `def_sum_stats.txt`, `def_sum_moves.txt`. Azahar: each tab, ▲▼ through the party (grey in the details), a move's details and CANCEL, a swap, the switch, an egg, BACK; a battle's Summary. Commit `feat: the Summary on the grid, its pages as the Definitiva`.

### Task 6: The Summary's CONTACT (recipe 0266)

**Files:**
- Create: `patches/pokefirered/0266-summary-reports-contact.json`
- Modify: `3ds_port/include/3ds_summary.h`, `3ds_device_summary_impl.h`
- Test: `device_render_test.c` (`summary`), `tools/test_field_presentation.py`

**Interfaces:**
- Consumes: Task 5's details column, `DevColumnRow`.
- Produces: `CtrSummaryView.contact` — `unsigned char contact;` right after `ribbons`: bit i, move i makes contact. Recipe 0266 (`src/pokemon_summary_screen.c` only): in `CtrBottomSummary_GetView`'s moves loop, after `out->moveTypeIds[i] = screen->moveTypes[i];`, the line `if (gBattleMoves[out->moves[i]].flags & FLAG_MAKES_CONTACT) out->contact |= (unsigned char)(1u << i);`. The details on a move: `DevColumnRow(134, CTR_TR("CONTACT", "CONTACTO", "KONTAKT"), (d->contact >> i) & 1 ? CTR_TR("YES", "S\xC3\x8D", "JA") : CTR_TR("NO", "NO", "NEIN"), 20, 104)`, a status move too. `DevSumUndrawn` copies `contact` outside the details (only the details draw it).

- [ ] **Step 1: Write the failing tests.** `test_field_presentation.py`:

```python
def test_summary_reports_contact(self):
    """The Summary's move details say whether the move makes contact (def_sum_moves): the bridge reports a bit per move,
    read from the move's own flags (recipe 0266, the summary screen's file only), after the ribbons."""
    name = '0266-summary-reports-contact.json'
    self.assertIn('if (gBattleMoves[out->moves[i]].flags & FLAG_MAKES_CONTACT) out->contact |= (unsigned char)(1u << i);',
                  additions(name, 'src/pokemon_summary_screen.c'))
    doc = json.loads((ROOT / 'patches/pokefirered' / name).read_text())
    self.assertEqual([f['path'] for f in doc['files']], ['src/pokemon_summary_screen.c'])
    self.assertRegex((ROOT / '3ds_port/include/3ds_summary.h').read_text(encoding='utf-8'), r'unsigned char ribbons;\s*unsigned char contact;')
```

`summary` suite (Review Focus 5): on the details with the cursor on move 0, `v.data.contact ^= 1` → `DevWhole(DevCheckSummary(&old, &v))` and `Count(20, 137, 84, 8, DEV_INK) > 0` (the CONTACT row); the cursor on a status move (power "-") still draws the row; on INFO `v.data.contact ^= 1` → `DevCheckSummary(&old, &v).count == 0`.
- [ ] **Step 2: Run** — `python tools/dev.py test test_field_presentation test_device_views` → FAIL.
- [ ] **Step 3: Make recipe 0266** — copy `build/dev/src/pokemon_summary_screen.c` (every earlier recipe applied, 0265 the last) to `build/work/src/`, add the line; `python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/0266-summary-reports-contact.json src/pokemon_summary_screen.c`; then the header field and the drawer.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py build` (the recipe applies; `FLAG_MAKES_CONTACT` is `include/constants/pokemon.h`'s); `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — render `summary_details` vs `def_sum_moves.txt` (CONTACT label [20,136], value right at 104). Azahar: SCRATCH (YES), a status move (NO), FLAMETHROWER (NO). Commit `feat: the Summary's move details say CONTACT (recipe 0266)`.

### Task 7: The Party's bays on the grid

**Files:**
- Modify: `3ds_port/include/3ds_poketouch.h` (`CtrBattleParty.gender`), `3ds_bottom_battle_fill_impl.h` (`BattlePartyFill`), `3ds_device_layout.h` (delete `kDevPtyNote`, `kDevPtyCancel`, `kDevPtyCancelLow`; change `kDevPtyConfirm`, `kDevPtyLevelScreen`), `3ds_device_party_impl.h`
- Test: `device_render_test.c` (`party`), `device_layout_test.c` (`PartyFaults`, groups), `tools/test_field_presentation.py`

**Interfaces:**
- Consumes: `kDevGridNote`, `kDevGridExit`, `kDevSmallNote`, `DevReadText`, `DevCancelWord`, `DEV_KEY_INK2`.
- Produces: `uint8_t gender[6]` in `CtrBattleParty` (0 male, 1 female, 2 none), copied in `BattlePartyFill` beside the name: `p->gender[i] = s->gender[i];` (the snapshot's, 3ds_bottom.c:1418). `kDevPtyConfirm {168,190,68,40}`; `kDevPtyLevelScreen {8,8,304,176}`.
  - A bay (`DevPtyBay` unchanged): the name from r.x + 38 capitals faceY + 10, its gender sign (`DevGender`) 4 px after it at faceY + 9 (none for an egg; the name cut to leave it room); `DevHpBar(r.x + 38, faceY + 29, 56, 5, …)` with no "HP" chip (`DevLabelChip` here, `DEV_PTY_GREY*`, `DEV_PTY_BAR_W` and the bar's grey frame go); the numbers small right at r.x + r.w − 8 capitals faceY + 27; under them the level small `DEV_KEY_INK2` capitals faceY + 39, or the status chip 10 tall at faceY + 38; a fainted bay `DEV_LOOK_FAINT` (its ink the kit's `#8A8478`).
  - Under the bays: the words (caption, the mode's words, a message with ▼ at (paper.x + paper.w − 10, paper.y + paper.h − 6)) on `kDevGridNote` via `DevReadText(…, paper.x + 8, right, paper.y + 2, DEV_SMALL_LINE)`; CANCEL `kDevGridExit` dark `DevCancelWord()` (dimmed when FORCED or a message); picking several: the words on `kDevSmallNote`, CONFIRM `kDevPtyConfirm` cream, CANCEL `kDevGridExit`. `DevPtyScreen` stays for the menu's screen until Task 8.
  - The level up: the stats on `kDevPtyLevelScreen` as today, the message on `kDevGridNote`, OK cream in `kDevGridExit`, up, sunk while `v->pressed == BUI_PTY_MESSAGE` (`DevPartyUndrawn` stops ignoring the touch in the level up).
  - `DevPartyHit`: CANCEL on `kDevGridExit`, CONFIRM on `kDevPtyConfirm`.

- [ ] **Step 1: Write the failing tests** (`party` suite; the board's party, bay 0 sunk):

```c
for (i = 0; i < 5; ++i) AssertHits(BatHitV, &v, DevPtyBay(i), BUI_PTY_SLOT + i);
AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_CANCEL); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
assert(at(47, 41) == DEV_HP_GREEN && at(46, 41) == DEV_HP_FRAME);                                   /* the bar from x+38 */
assert(Count(8, 8, 304, 170, DEV_HP_LABEL) == 0);                                                   /* no "HP" tag on any bay */
assert(Count(46, 18, 110, 11, DEV_MALE) > 0 && Count(202, 14, 110, 11, DEV_FEMALE) > 0);           /* CHARMELEON ♂, PIDGEOTTO ♀ */
assert(Count(14, 196, 216, 30, DEV_INK) > 0 && Count(8, 180, 304, 9, DEV_INK) == 0);               /* the words on the footer */
v.pty.mode = BUI_PTY_FORCED; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 278, 210) == -1);
v.pty.mode = BUI_PTY_MULTI; DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, kDevPtyConfirm, BUI_PTY_CONFIRM);
AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_CANCEL); assert(CtrBattleUi_Hit(&v, 162, 210) == -1);
lv.pty.panel = BUI_PTY_LEVELUP; DevCheckBattle(&lo, &lv); AssertKey(kDevGridExit, DEV_LOOK_KEY, 0);
lv.pressed = BUI_PTY_MESSAGE; assert(DevWhole(DevCheckBattle(&lo, &lv))); AssertKey(kDevGridExit, DEV_LOOK_KEY, 1);
v.pty.gender[2] ^= 1; assert(DevWhole(DevCheckBattle(&old, &v)));
```

`test_field_presentation.py` `test_party_fill_copies_gender`: `function(fill, 'BattlePartyFill')`, `fill` read from `3ds_port/src/3ds_bottom_battle_fill_impl.h`, contains `p->gender[i] = s->gender[i];`. Layout: groups "Party" {6 bays, `kDevGridExit`}, "Party, several" {6 bays, `kDevPtyConfirm`, `kDevGridExit`}, "Party level up" {`kDevGridExit`}; `kDevSmallNote.x + kDevSmallNote.w + 8 == kDevPtyConfirm.x` and `kDevPtyConfirm.x + kDevPtyConfirm.w + 8 == kDevGridExit.x`.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_field_presentation` → FAIL.
- [ ] **Step 3: Implement** per Interfaces.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `party_{bays,one,forced,forced_cancel,multi,message,teach,levelup}` vs `def_party.txt` (name + sign, bar [46,39,56,5], numbers / Lv at the right, faint bay beige, note [8,190,228,42] "Choose a POKéMON." at 22, CANCEL [244,190,68,40]). Azahar: the field's Party, a battle's switch, a forced send-out, a Rare Candy's level up, an item's target, a link-style pick if reachable. Commit `feat: the Party's bays on the grid, with the gender sign`.

### Task 8: The Party's action menu: one scrolling column (O2)

**Files:**
- Modify: `3ds_port/include/3ds_party_grid.h`, `3ds_device_layout.h` (`DevPtySlim`, `DevPtyMenuKey`; delete `kDevPtyMenuNote`), `3ds_device_party_impl.h`
- Test: `3ds_port/tests/party_grid_test.c`, `device_layout_test.c` (`PartyMenuGrid`, `PartyFaults`, the menu groups), `device_render_test.c` (`party`: menu3, menu6, menu8, yesno)

**Interfaces:**
- Consumes: `kDevGridNote`, `kDevGridExit`, `DevKeyWordAt(Ink)`, `DevReadText`, `DEV_ARROW_OFF`, `DEV_DARK_INK`.
- Produces: `CtrPartyGrid_Cols(int count)` returns 1 for every count; `CtrPartyGrid_Move(...)` returns −1 (FireRed's own list input: UP / DOWN, wrapping); the two-column arithmetic and its comment go, and with it `CtrPartyGrid_Rows`, `_Col`, `_Row`, `_ColLen` (no caller left once `DevPtyMenuKey` stops using them): the header keeps `CtrPartyGrid_Count`, `_Cols` (O2) and `_Move`, and says one column the lower screen scrolls (recipe 0257 and `test_party_relearn_recipes.py` unchanged). `DevPtySlim(int i)` = {8, 8 + 28i, 188, 24}; `DevPtyMenuKey(int k)` = {204, 8 + 28k, 108, 24}, k 0-5; `#define DEV_PTY_MENU_ROWS 6`. In the party file `static int DevPtyMenuKeys(const CtrBattleParty *p)` = the menu's rows less the last when its `menuDark` bit is set, and `static int DevPtyMenuFirst(const CtrBattleParty *p)` = keys > `DEV_PTY_MENU_ROWS` ? clamp(menuCursor − (`DEV_PTY_MENU_ROWS` − 1), 0, keys − `DEV_PTY_MENU_ROWS`) : 0. Column key k is row first + k (hit `BUI_PTY_MENU + first + k`), its word from x + 10 (a field move in `DEV_BLUE_INK`, `DevKeyWordAtInk`); the dark last row is `kDevGridExit` (hit `BUI_PTY_MENU + n − 1`: `RunPlan` already makes it B); YES / NO: two column keys, no exit key. The cue (keys > 6 only): `DevTriangle(258, 4, 0, first > 0 ? DEV_DARK_INK : DEV_ARROW_OFF)`, `DevTriangle(258, 182, 1, first + 6 < keys ? DEV_DARK_INK : DEV_ARROW_OFF)`, not hit. The question / caption on `kDevGridNote` (`DevReadText`, from paper.x + 8); `DevPtyScreen` goes.

- [ ] **Step 1: Write the failing tests.** `party_grid_test.c` (its two-column checks go): `for (n = 1; n <= 8; ++n) { assert(CtrPartyGrid_Cols(n) == 1 && CtrPartyGrid_Count(n) == n); for (i = 0; i < n; ++i) for (d = 0; d < 4; ++d) assert(CtrPartyGrid_Move(n, i, d) == -1); } assert(CtrPartyGrid_Count(9) == 8);`. Layout `PartyMenuGrid`: `Same(DevPtyMenuKey(5), (CtrPokeTouchRect){204,148,108,24})`, every `DevPtySlim(i)` and `DevPtyMenuKey(k)` apart, the sixth key's drop ending ≤ 175; group "Party menu" {6 keys, `kDevGridExit`}. `party` suite, `menu8` (Review Focus 2: SUMMARY SWITCH ITEM CUT FLY HEAL REMEMBER CANCEL, CANCEL dark):

```c
v.pty.menuCursor = 0; DevCheckBattle(&old, &v);
for (k = 0; k < 6; ++k) AssertHits(BatHitV, &v, DevPtyMenuKey(k), BUI_PTY_MENU + k);
AssertHits(BatHitV, &v, kDevGridExit, BUI_PTY_MENU + 7); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0);
assert(Count(254, 180, 9, 5, DEV_DARK_INK) > 0 && Count(254, 2, 9, 5, DEV_ARROW_OFF) > 0);               /* more below */
v.pty.menuCursor = 6; DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, DevPtyMenuKey(5), BUI_PTY_MENU + 6); AssertKey(DevPtyMenuKey(5), DEV_LOOK_KEY, 1);
v.pty.menuCursor = 7; DevCheckBattle(&old, &v); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1); AssertHits(BatHitV, &v, DevPtyMenuKey(0), BUI_PTY_MENU + 1);
/* menu3 (SHIFT SUMMARY CANCEL): two keys and the exit; no cue */ assert(Count(254, 180, 9, 5, DEV_DARK_INK) == 0);
/* YES / NO */ AssertHits(BatHitV, &yv, DevPtyMenuKey(0), BUI_PTY_MENU); AssertHits(BatHitV, &yv, DevPtyMenuKey(1), BUI_PTY_MENU + 1); assert(CtrBattleUi_Hit(&yv, 278, 210) == -1);
```

- [ ] **Step 2: Run** — `python tools/dev.py test` → FAIL (party_grid_test, layout, party).
- [ ] **Step 3: Implement** per Interfaces (`DevPtyMenu`, `DevPtySlimKey` always 188 wide with level and numbers, `DevPartyHit`).
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `party_{menu3,menu6,menu8,yesno}` vs `def_party_menu.txt` (slim bays [8,8+28i,188,24], keys [204,8+28k,108,24] words from x 214, CANCEL [244,190,68,40], note "Do what with this POKéMON?" at 22). Azahar: a Pokémon with CUT and FLY (and HEAL / REMEMBER with `CTR_PARTY_EXTRAS 1` in a local build): the D-pad walks the column with wrap, the window follows to the seventh action and to CANCEL, CANCEL by the exit key, SWITCH's YES / NO. Commit `feat: the Party's actions in one scrolling column (owner O2)`.

### Task 9: HOME as the Definitiva — then PR E1

**Files:**
- Modify: `3ds_device_layout.h` (`kDevHomeKey`, `kDevHomeRun`, `kDevHomeSpeed`, `kDevHomeName`, `kDevHomeRule`, `DevHomeCell`; add `kDevHomeTime`), `3ds_device_home_impl.h`, `3ds_device_kit_impl.h` (delete `DevPaperButton`), `3ds_bottom.c` (HOME's fill)
- Test: `device_render_test.c` (`home`, `kit`), `device_layout_test.c` (`HomeFaults`, `Paper()` and `RulesCatchFaults`' line on it, the HOME group)

**Interfaces:**
- Consumes: `DevAcross`, `DevKeyWord`, `DevLedPixels`, `DevDisc`, `DEV_KEY_INK2`, `DEV_RULE`.
- Produces: `kDevHomeKey[6]` (PT order) = `DevAcross(4, id % 3, id < 3 ? 154 : 196, 34)` written out: {8,154,73,34} {85,154,73,34} {162,154,73,34} {8,196,73,34} {85,196,73,34} {162,196,73,34}; `kDevHomeRun {239,154,73,34}`, `kDevHomeSpeed {239,196,73,34}` (keys now: hit on `DevKeyArea`); `kDevHomeName {212,16,90,26}`, `kDevHomeRule {210,57,92,1}`, `kDevHomeTime {212,128,90,10}`; `DevHomeCell(int i)` = {210 + 30(i % 3), 60 + 32(i / 3), 30, 32}: the half icon at (x + 7, y + 4), `DevHpBar(x + 4, y + 25, 22, 4, …)`, a hole's ring 16 at (x + 7, y + 4).
  - A START key: the 16x16 icon, 6 px, the word, the pair centred (room 73 − 8 − 22 = 43, cut and counted past it). RUN: a cream key, the word (`DevUpper(CTR_TR("Run", "Corre", "Lauf"))`), 6 px, a 6 px LED, the pair centred, the LED's top at faceY + 14: lit `DevLedPixels(x, y, 6, face)` while `running`, else `DevDisc(x, y, 6, DEV_KEY_EDGE)`; sunk under a held touch, `DevDim` when `!enabled[PT_RUN]`. SPD: `CTR_TR("SPD", "VEL", "TMP")` small `DEV_KEY_INK2`, 6 px, "2x" normal, centred; sunk under a touch, `DevDim` when `!speedEnabled`. TIME: `"%s %u:%02u"` of `CTR_TR("TIME", "TIEMPO", "ZEIT")`, `saveHours`, `saveMinutes`, small `DEV_INK2` at `kDevHomeTime` (capitals 130); `DevHomeLook` gains both; HOME's fill sets them as `SaveFill` does (`CtrSave_PlayHours()`, `CtrSave_PlayMinutes()` ≤ 59). `DEV_HOME_LAMP*` and `DevPaperButton` go.

- [ ] **Step 1: Write the failing tests.** `home` suite:

```c
for (id = PT_POKEDEX; id <= PT_OPTIONS; ++id) AssertHits(HomeHitV, &v, kDevHomeKey[id], id);
AssertHits(HomeHitV, &v, kDevHomeRun, PT_RUN); AssertHits(HomeHitV, &v, kDevHomeSpeed, PT_HOME_SPEED);
for (i = 0; i < 6; ++i) if (v.party[i]) AssertRowHits(HomeHitV, &v, DevHomeCell(i), PT_HOME_PARTY + i);
v.running = 1; DevCheckTouch(&old, &v); int lit = Count(239, 154, 73, 37, DEV_KEY_EDGE); assert(Count(239, 154, 73, 37, DEV_LED) >= 20);
v.running = 0; DevCheckTouch(&old, &v); assert(Count(239, 154, 73, 37, DEV_LED) == 0 && Count(239, 154, 73, 37, DEV_KEY_EDGE) - lit >= 20);  /* the dim disc */
v.pressed = PT_RUN; DevCheckTouch(&old, &v); AssertKey(kDevHomeRun, DEV_LOOK_KEY, 1); v.pressed = PT_NONE;
v.saveMinutes = 34; assert(DevWhole(DevCheckTouch(&old, &v))); assert(Count(212, 130, 90, 7, DEV_INK2) > 0);
```

Layout `HomeFaults`: `kDevHomeKey[i]` = `DevAcross(4, i % 3, i < 3 ? 154 : 196, 34)`; `DevHomeCell(5)` ends ≤ `kDevHomeTime.y`; group HOME {6 keys, RUN, SPD} with no paper exception (`Paper()` goes). `kit`: the `DevPaperButton` probes go.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevHomeKey`, `DevHomeRun`, `DevHomeSpeed`, `DevHomeParty`, `DevHomePanel`, `DevHomeDraw` draws RUN and SPD with the keys, `CtrPokeTouch_HomeHit`).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `home_{plain,pressed,fainted,selection,sevii,flypick}` vs `def_home.txt` (keys [8|85|162|239, 154|196, 73, 34], icons [215+30c, 62+32r], bars [214+30c, 85+32r, 22, 4], ring [277,96,16,16], TIME at [212,128]). Azahar: every key, RUN on and off (the LED), SPD 1x-4x, a party icon, TIME ticking a minute.
- [ ] **Step 6: Commit** `feat: HOME's eight keys, RUN's LED and the play time`.
- [ ] **Step 7: PR E1** — full check (`python tools/dev.py test`, build, audit, `python tools/dev.py render` every view, `python tools/dev.py run`); `git push -u origin feat/b-def-kit-field`; `gh pr create --base feat/b-ember-removal --title "feat: the Definitiva kit and the field's views (HOME, Party, Summary, Bag)"`; body: the grid and the kit rules (every view gets the LED, the screws, beige ink, DE ABBRECHEN), each view's change, recipe 0266, the owner's checklist (Tasks 3-9's Azahar lines), the 🤖 line.

### Task 10: The choices on the grid

**Files:**
- Modify: `3ds_device_layout.h` (`DevChoiceKey`; delete `kDevChoiceNote`, `kDevChoiceBack`), `3ds_device_choice_impl.h`
- Test: `device_suite_choice.h`, `device_layout_test.c` (`ChoiceFaults`, the choice groups)

**Interfaces:**
- Consumes: `kDevGridNote`, `kDevGridExit`, `DevKeyWordAt`, `DevReadText`, `DevBackWord`.
- Produces: `DevChoiceKey(int n, int cols, int i)`: cols ≥ 2 (FireRed's grid) → `DevAcross(cols, i % cols, 8 + 44(i / cols), 38)`; else n = 2 → {8, 8 + 92i, 304, 84}; n ≤ 4 → {8, 8 + 44i, 304, 38}; n 5-8 → `DevAcross(2, i / 4, 8 + 44(i % 4), 38)` (the left column always four). A key's word: on a 304-wide key from x + 12 (`DevKeyWordAt`, inset 12), else centred; the normal font everywhere. The question on `kDevGridNote`: `DevReadText(prompt, &paper, paper.x + 6, paper.x + paper.w − 6, paper.y + 2, DEV_SMALL_LINE)`; BACK `kDevGridExit` (`DevFieldBack`, `DevChoiceBackKey`). `CtrPokeTouch_ChoiceHit` and its codes unchanged.

- [ ] **Step 0: Branch** — `git checkout -b feat/b-def-battle-questions` (from Task 9's tip, PR E1 open); Tasks 10-15 commit on it.
- [ ] **Step 1: Write the failing tests** (`choice` suite):

```c
v.choice.count = 2; v.choice.cols = 0; DevCheckTouch(&old, &v);
AssertHits(ChoiceHitV, &v, (CtrPokeTouchRect){8, 8, 304, 84}, PT_CHOICE_HIT_ROW); AssertHits(ChoiceHitV, &v, (CtrPokeTouchRect){8, 100, 304, 84}, PT_CHOICE_HIT_ROW + 1);
for (n = 1; n <= 8; ++n) { v.choice.count = n; DevCheckTouch(&old, &v); for (i = 0; i < n; ++i) AssertHits(ChoiceHitV, &v, DevChoiceKey(n, 0, i), PT_CHOICE_HIT_ROW + i); }
assert(Same(DevChoiceKey(4, 0, 3), (CtrPokeTouchRect){8, 140, 304, 38}) && Same(DevChoiceKey(5, 0, 3), (CtrPokeTouchRect){8, 140, 148, 38})
       && Same(DevChoiceKey(5, 0, 4), (CtrPokeTouchRect){164, 8, 148, 38}) && Same(DevChoiceKey(6, 3, 4), (CtrPokeTouchRect){112, 52, 96, 38}));
AssertHits(BackV, &v, kDevGridExit, 1); assert(CtrPokeTouch_ChoiceHit(&v, 160, 186) == -1 && Count(14, 196, 216, 30, DEV_INK) > 0);
```

The long-prompt state: two normal lines, else three small (`DevSmallFit` counts a fourth). Layout: for n 1-8 and cols {0, 2, 3} the keys and `kDevGridExit` keep the rules and every key's drop ends above the footer (< 190).
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevChoiceDraw`, `DevChoicePrompt` goes for `DevReadText`).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `choice_{yesno,long,six,eight,grid}` vs `def_choice_yesno.txt`, `def_choice_list.txt`, `def_choice_grid.txt`. Azahar: a nurse's YES / NO, the PC's menu (five: 4 + 1), the Celadon roof's drinks, the Viridian school's grid, BACK on each. Commit `feat: the field's choices on the grid`.

### Task 11: Prices on the choices (recipe 0267)

**Files:**
- Create: `patches/pokefirered/0267-field-choice-prices.json`
- Modify: `3ds_port/include/3ds_poketouch.h` (`CtrPokeTouchChoice.price`), `3ds_bottom.c` (extern), `3ds_bottom_field_fill_impl.h` (`ChoiceSnapshot`), `3ds_device_choice_impl.h`
- Test: `device_suite_choice.h`, `tools/test_field_presentation.py`; any host test that stubs `CtrFieldChoice_Label` gains `CtrFieldChoice_Price` (`git grep CtrFieldChoice_Label -- 3ds_port/tests tools`)

**Interfaces:**
- Consumes: Task 10's keys, `DEV_KEY_INK2`.
- Produces: recipe 0267 (`src/menu.c` only): `static void CtrFieldChoice_Split(const u8 *text, u8 *label, u8 *price, int size)` — the label is the text before `EXT_CTRL_CODE_BEGIN EXT_CTRL_CODE_CLEAR_TO`, the price what follows it (its argument skipped); in both every other control code is skipped (`GetExtCtrlCodeLength`), leading spaces (0x00) are dropped, each ends with `EOS` within `size` bytes; no `CLEAR_TO`: the price is empty. `CtrFieldChoice_Label` gives the label; new `void CtrFieldChoice_Price(int index, char *out, int size)` gives the price (both through `CtrBattle_FireRedText`; `out[0] = 0` out of range). Port: `extern void CtrFieldChoice_Price(int index, char *out, int size);`; `char price[PT_CHOICE_MAX][12];` after `label`; `ChoiceSnapshot` fills it. Draw: a 304-wide key: the price right-aligned at x + w − 12 in `DEV_KEY_INK2`, no shadow, the label from x + 12 ending 8 px before it (cut and counted past that); a narrower key: the label centred, capitals faceY + 10, the price small `DEV_KEY_INK2` centred, capitals faceY + 21; the YES / NO keys draw no price.

- [ ] **Step 1: Write the failing tests.** `test_field_presentation.py` (Review Focus 3):

```python
def test_choice_prices_split(self):
    """A priced multichoice row (FireRed's "FRESH WATER{CLEAR_TO 0x57}{FONT_SMALL}¥200", the Game Corner's
    "{FONT_SMALL} 50 COINS{CLEAR_TO 0x45}¥1,000") reaches the lower screen as its label and its price (recipe 0267)."""
    name = '0267-field-choice-prices.json'
    added = additions(name, 'src/menu.c')
    self.assertIn('void CtrFieldChoice_Price(int index, char *out, int size)', added)
    self.assertEqual([f['path'] for f in json.loads((ROOT / 'patches/pokefirered' / name).read_text())['files']], ['src/menu.c'])
    self.run_c('''#include <assert.h>
#include <string.h>
typedef unsigned char u8;
#define EOS 0xFF
#define EXT_CTRL_CODE_BEGIN 0xFC
#define EXT_CTRL_CODE_CLEAR_TO 0x13
static u8 GetExtCtrlCodeLength(u8 code) { return code == 0x06 || code == 0x13 ? 2 : 1; }
''' + function(added, 'CtrFieldChoice_Split') + '''
int main(void) {
    static const u8 a[] = {0xC0, 0xC1, 0xFC, 0x13, 0x57, 0xFC, 0x06, 0x00, 0xA1, 0xA2, 0xFF};
    static const u8 b[] = {0xFC, 0x06, 0x00, 0x00, 0xC0, 0xFC, 0x13, 0x45, 0xA1, 0xFF}, c[] = {0xC0, 0xFF};
    u8 l[8], p[8];
    CtrFieldChoice_Split(a, l, p, 8); assert(!memcmp(l, "\\xC0\\xC1\\xFF", 3) && !memcmp(p, "\\xA1\\xA2\\xFF", 3));
    CtrFieldChoice_Split(b, l, p, 8); assert(!memcmp(l, "\\xC0\\xFF", 2) && !memcmp(p, "\\xA1\\xFF", 2));
    CtrFieldChoice_Split(c, l, p, 8); assert(!memcmp(l, "\\xC0\\xFF", 2) && p[0] == 0xFF);
    CtrFieldChoice_Split(a, l, p, 2); assert(l[0] == 0xC0 && l[1] == 0xFF);
    return 0; }''')
```

`choice` suite, priced block: the drinks (three priced rows and CANCEL) — `Count(k.x + k.w - 52, k.y + 12, 40, 12, DEV_KEY_INK2) > 0` on each priced key and `== 0` on CANCEL; a six-row priced prize menu — the price under the label (`Count(k.x, k.y + 21, k.w, 8, DEV_KEY_INK2) > 0`) on the 148-wide keys; a 23-character label with a price on a 304-wide key counts one cut, forced as `TouchSamePixels` does by widening the stub glyphs for that one draw (`fontWidths[c] = 12` for the label's capitals, 23 × 12 = 276 past the ≈ 242 px before the price, restored after; at 5 or 6 px no 23-byte label reaches any key's room) — `assert(sDevKeyCut == 1); sDevKeyCut = 0;`; `v.choice.price[1][0] ^= 1` redraws (`DevWhole`).
- [ ] **Step 2: Run** — `python tools/dev.py test test_field_presentation test_device_views` → FAIL.
- [ ] **Step 3: Make recipe 0267** — copy `build/dev/src/menu.c` (every earlier recipe applied, 0266 included) to `build/work/src/`, add the split and the price, have `CtrFieldChoice_Label` take the label; `python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/0267-field-choice-prices.json src/menu.c`; then the port side.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py build`; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — render `choice_long` (or a new `choice_priced`) vs `def_choice_list.txt` (label at x 20, price right at 300 in #6A6A6A). Azahar: the Celadon roof's drinks, a Game Corner prize counter, the coin seller. Commit `feat: prices on the choices' keys (recipe 0267)`.

### Task 12: The battle's commands (O1)

**Files:**
- Modify: `3ds_device_layout.h` (`kDevBatScreen`, `kDevBatFight`, `kDevBatRow3`, `kDevBatRow4`), `3ds_device_battle_impl.h`
- Test: `device_render_test.c` (`battle`: ACTION, IDLE), `device_layout_test.c` (`BattleFaults`, `SharedFaults`, groups)

**Interfaces:**
- Consumes: the kit's `DevKey` (LED), `DevDim`.
- Produces: `kDevBatScreen {8,8,304,92}`; `kDevBatFight {8,108,304,48}`; `kDevBatRow3[3]` {8,168,96,62} {112,168,96,62} {216,168,96,62} (a trainer, the board); `kDevBatRow4[i]` = `DevAcross(4, i, 168, 62)` {8|85|162|239, 168, 73, 62} (a wild battle with the quick ball: BAG, BALL, POKéMON, RUN); `DevBatBallArrow` unchanged (r.y + 38 stays on the 62-tall key). Drawer: `DEV_BAT_SWORDS 22`, `DEV_BAT_SWORDS_GAP 6`, `DEV_BAT_ICON_GAP 5`; the party's balls 9x9 at x 230 + 12i, y 20; `DevHpBar(89, 54, 160, 6, …)`; the question's capitals at 79; the rest as today (no exit key, B does nothing).

- [ ] **Step 1: Write the failing tests** (`battle` suite; Review Focus 4):

```c
static const int kWild[4] = {BUI_ACT_BAG, BUI_ACT_BALL, BUI_ACT_POKEMON, BUI_ACT_RUN}, kTrainer[3] = {BUI_ACT_BAG, BUI_ACT_POKEMON, BUI_ACT_RUN};
AssertHits(BatHitV, &v, kDevBatFight, BUI_ACT_FIGHT);
for (i = 0; i < 4; ++i) if (i != 1) AssertHits(BatHitV, &v, kDevBatRow4[i], kWild[i]);
r = DevBatBallArrow(&kDevBatRow4[1], 0); assert(CtrBattleUi_Hit(&v, r.x + 10, r.y + 10) == BUI_ACT_BALL_PREV);
r = DevBatBallArrow(&kDevBatRow4[1], 1); assert(CtrBattleUi_Hit(&v, r.x + 10, r.y + 10) == BUI_ACT_BALL_NEXT);
assert(CtrBattleUi_Hit(&v, 121, 180) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, 160, 233) == -1);
v.ball = CTR_QUICK_BALL_NONE; DevCheckBattle(&old, &v); for (i = 0; i < 3; ++i) AssertHits(BatHitV, &v, kDevBatRow3[i], kTrainer[i]);
v.runBlocked = 1; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 260, 200) == -1 && at(219, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
assert(at(234, 24) == DEV_INK && Count(89, 54, 160, 6, DEV_HP_FRAME) > 0);
```

(the ball's probes on the board's balls; IDLE: every key dimmed, none sunk). Layout `BattleFaults` pins the five rects; `SharedFaults:393` becomes `DevAcross(4, i, 168, 62) == kDevBatRow4[i]` (and `3ds_device_layout.h`'s `DevAcross` comment says y 168); groups "battle, a trainer" and "battle, wild" keep the rules (FIGHT's drop 159 < 168).
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevBatLead`, `DevBatFightKey`, `DevBatRowKey`, `DevBatBallKey` on the new rects).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `battle_{action,action_ball,action_ballfull,action_runblocked,idle}` vs `def_fight.txt` (screen [8,8,304,92], balls [230+12i,20,9,9], HP bar [89,54,160,6], FIGHT [8,108,304,48], row keys [..,168,..,62]). Azahar: a wild battle (the quick ball, its arrows and swipe), a trainer battle, RUN blocked. Commit `feat: the battle's commands as the Definitiva (owner O1)`.

### Task 13: The battle's moves, TARGET and Safari

**Files:**
- Modify: `3ds_device_layout.h` (`kDevBatMove`; delete `kDevBatMoveScreen`, `kDevBatCancel`), `3ds_device_battle_impl.h`
- Test: `device_render_test.c` (`battle`: MOVE, TARGET, SAFARI, IDLE Safari), `device_layout_test.c` (`BattleFaults`, `SharedFaults`, the named-rect table)

**Interfaces:**
- Consumes: `kDevGridNote`, `kDevGridNoteWide`, `kDevGridExit`, `DevTypeChip`, `DevSmallFit`, `DevReadText`, `DevBackWord`, `DevCategoryWord`, `DevPowAcc`, `DEV_TAG_STAB`, `DEV_KEY_INK2`.
- Produces: `kDevBatMove[i]` = `DevAcross(2, i % 2, 8 + 90(i / 2), 84)`: {8,8,148,84} {164,8,148,84} {8,98,148,84} {164,98,148,84}.
  - A move key: the name from x + 10 capitals faceY + 11; the type chip (`DevTypeChip`, 12 tall) at (x + 10, faceY + 28); the matchup 4 px after it at faceY + 28, or "STATUS" small `DEV_KEY_INK2` capitals faceY + 29; the pips at (x + 10, faceY + 64); "PP" small `DEV_KEY_INK2` and the numbers right at x + w − 10 capitals faceY + 63; a hole for no move.
  - The footer (MOVE: the move under a held touch, else the cursor's; none: empty): chips 10 tall at paper.y + 2 — type from x 20, then category (`DevCategoryWord`), then "STAB" (`DEV_TAG_STAB`) when `stab`; POW / ACC (`DevPowAcc`) small `DEV_INK` right-aligned at right = paper.x + paper.w − 4, capitals paper.y + 2, starting no further left than the chips' end + 6; when chipsEnd + 6 + its width passes right (ES / DE with a physical STAB move), it is instead the first small line at capitals paper.y + 13 and the description keeps one line. FireRed's description small `DEV_INK2` from capitals paper.y + 13 (+ 10 under a moved POW / ACC), two lines (`DevSmallFit`, x 20 to 224; one when POW / ACC moved), else "EFFECT n%" there.
  - TARGET: the same keys (the HP bar `DevHpBar(x + 10, faceY + 64, 72, 5, …)`, an ally's numbers right capitals faceY + 63); the footer `DevReadText` of today's "Which POKéMON should the move hit?" words.
  - MOVE and TARGET: BACK `kDevGridExit`, dark, `DevBackWord()`, sunk by `DevChosen(BUI_CANCEL, cursor, pressed)`, hit `BUI_CANCEL`; the hint and `DEV_BAT_SHADOW_A` go.
  - SAFARI: the four keys at `kDevBatMove` (art and word as today), "What will you do?" (today's words) on `kDevGridNoteWide` via `DevReadText`; no exit key; IDLE Safari: no question, the keys dimmed.

- [ ] **Step 1: Write the failing tests** (`battle` suite):

```c
for (i = 0; i < 4; ++i) AssertHits(BatHitV, &mv, kDevBatMove[i], i);
AssertHits(BatHitV, &mv, kDevGridExit, BUI_CANCEL); AssertKey(kDevGridExit, DEV_LOOK_DARK, 0); assert(Count(8, 210, 236, 12, DEV_WHITE) == 0);   /* no hint */
mv.move[0].stab = 1; mv.cursor = 0; DevCheckBattle(&mo, &mv); assert(Count(14, 196, 216, 12, DEV_TAG_STAB) > 0);
mv.move[0].stab = 0; DevCheckBattle(&mo, &mv); assert(Count(14, 196, 216, 12, DEV_TAG_STAB) == 0 && Count(20, 209, 204, 17, DEV_INK2) > 0);
mv.move[3].present = 0; DevCheckBattle(&mo, &mv); assert(CtrBattleUi_Hit(&mv, 238, 140) == -1);
mv.cursor = BUI_CANCEL; DevCheckBattle(&mo, &mv); AssertKey(kDevGridExit, DEV_LOOK_DARK, 1);
/* TARGET */ AssertHits(BatHitV, &tv, kDevGridExit, BUI_CANCEL); tv.target[1].present = 0; DevCheckBattle(&to, &tv); assert(CtrBattleUi_Hit(&tv, 238, 50) == -1);
/* SAFARI */ for (i = 0; i < 4; ++i) AssertHits(BatHitV, &sv, kDevBatMove[i], i); assert(CtrBattleUi_Hit(&sv, 278, 210) == -1 && Count(14, 196, 292, 30, DEV_INK) > 0);
```

The MOVE block adds a physical STAB move as the cursor's (its footer drawn in the ES / DE fit runs: no chip overlapped, no cut). Layout: `kDevBatMove[i]` pinned; `SharedFaults:395` becomes `DevAcross(2, i % 2, 8 + 90 * (i / 2), 84) == kDevBatMove[i]` for i < 4 (and `3ds_device_layout.h`'s `DevAcross` comment says y 8, 84 tall); `BattleFaults`' moves-screen clearance (:173) and the named-rect table's "battle moves' screen" and "battle CANCEL" (848-852) go (BACK is `kDevGridExit`); groups "battle moves" {4 keys, `kDevGridExit`} and "Safari" {4 keys}; the lower keys' drops end ≤ 186.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevBatMoveKey`, `DevBatTargetKey`, a footer function in place of `DevBatMoveScreen`, `DevBatMoves`, `DevBatSafari`, `CtrBattleUi_Hit`, `DevBatUndrawn` for what the footer now draws).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `battle_{moves,moves_two,target,safari,idle_safari}` vs `def_moves.txt` (keys 148x84, chips at faceY + 28, pips at faceY + 64, footer chips / POW ACC / description, BACK [244,190,68,40]). Azahar: FIGHT and each move (the footer follows the cursor), BACK, a double battle's target, the Safari Zone. Commit `feat: the battle's moves, targets and Safari as the Definitiva`.

### Task 14: Forget a move

**Files:**
- Modify: `3ds_device_layout.h` (the learn rects; delete `kDevLearnScreen`; add `kDevLearnRule`), `3ds_device_learn_impl.h`
- Test: `device_suite_learn.h`, `device_layout_test.c`

**Interfaces:**
- Consumes: `kDevGridTitle`, `kDevGridWide`, `kDevGridNote`, `kDevGridExit`, `DevColumnRow`, `DevTypeChip`, `DevSmallFit`, `DEV_TAG_NEW`, `DEV_RULE`.
- Produces: the screen on `kDevGridWide` {8,40,304,144} (`kDevLearnScreen` goes: no view-named copy of a grid rect), `kDevLearnColumn {14,46,104,132}`, `kDevLearnPic {34,76,64,64}`, `DevLearnSlot(int i)` = i < 4 ? {124, 48 + 24i, 176, 22} : {124, 150, 176, 22}, `kDevLearnRule {124,146,176,1}` (`DEV_RULE`). The column: the name capitals 52, "Lv" small capitals 66, the picture, `DevColumnRow(146, POWER, …, 20, 110)`, `DevColumnRow(160, ACCURACY, …, 20, 110)`. A row: the name from x 130 capitals r.y + 7; on row 4 first a "NEW" chip (`CTR_TR("NEW", "NUEVO", "NEU")`, `DEV_TAG_NEW`, 12 tall at (130, r.y + 5)) and the name 3 px after it; the type chip (`DevTypeChip`, 12 tall) at (216, r.y + 5); the PP right at 298 capitals r.y + 7, no shadow; the name cut before 212. The footer (paper (14,196,216,30); Call 18): the description small `DEV_INK`, capitals 198 and 208 (`DevSmallFit`, 2 lines, x 20 to 224); the hint small `DEV_INK2` capitals 218 for the cursor's row: rows 0-3 `CTR_TR("TAP AGAIN TO FORGET IT.", "TOCA OTRA VEZ PARA OLVIDARLO.", "NOCHMAL TIPPEN ZUM VERGESSEN.")`, a locked (HM) row `CTR_TR("HM MOVES CAN'T BE FORGOTTEN.", "LAS MO NO SE PUEDEN OLVIDAR.", "VM K\xC3\x96NNEN NICHT VERGESSEN WERDEN.")`, row 4 `CTR_TR("TAP AGAIN TO SKIP IT.", "TOCA OTRA VEZ PARA NO APRENDERLO.", "NOCHMAL TIPPEN ZUM AUSLASSEN.")`. BACK `kDevGridExit` (`DevLearnBackKey`). `DevLearnSame` compares the cursor row's `locked` (the hint draws it), still not the others'.

- [ ] **Step 1: Write the failing tests** (`learn` suite; four moves and SLASH new, the cursor on row 1):

```c
for (i = 0; i < 5; ++i) AssertRowHits(LearnHitV, &lv, DevLearnSlot(i), PT_LEARN_HIT_ROW + i);
AssertHits(BackV, &lv, kDevGridExit, 1); assert(at(160, 146) == DEV_RULE && Count(130, 155, 30, 12, DEV_TAG_NEW) > 0);
assert(Count(216, DevLearnSlot(0).y + 5, 40, 12, UiTypeColor(lv.learn.row[0].type)) > 0 && Count(20, 218, 204, 7, DEV_INK2) > 0);
lv.learn.row[1].locked = 1; assert(DevWhole(DevCheckTouch(&lo, &lv)));
lv.learn.row[2].locked ^= 1; lv.mapGroup ^= 1; lv.speed ^= 1; assert(DevCheckTouch(&lo, &lv).count == 0);
```

Layout: the column and `DevLearnSlot(0..4)` inside `kDevGridWide`'s paper (rows 22 tall, 24 apart: the board's exception to the list rule); group "the move to forget" {`kDevGridExit`}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevLearnColumn`, `DevLearnRow`, `DevLearnDraw`, `CtrPokeTouch_LearnHit`, `DevLearnSame`).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS (ES/DE: the hints fit 204 px).
- [ ] **Step 5: Task close** — renders `learn_{move,new,two}` vs `def_learn.txt`. Azahar: a Rare Candy at four moves — tap an old move twice (forgotten), the new one twice (skipped), an HM (refused), BACK. Commit `feat: forget a move as the Definitiva, with a hint per row`.

### Task 15: The relearner — then PR E2

**Files:**
- Modify: `3ds_device_layout.h` (delete `kDevRlNarrow`, `kDevRlDescNarrow`, `DevRlKey`), `3ds_device_relearn_impl.h`
- Test: `device_suite_relearn.h`, `device_layout_test.c`; `tools/test_make_device_boards.py` stays green (relearn_list keeps a chip and ▼)

**Interfaces:**
- Consumes: `DevGridRow`, `kDevGridScreen(Narrow)`, `kDevGridUp/Down`, `kDevGridNote`, `kDevGridExit`, `DevGridColumnKey`, `DevArrowKeys`, `DevTypeChip`, `DevSmallFit`, `DevCancelWord`, `DevPowAcc`.
- Produces: LIST — rows `DevGridRow(i, 0)`: the name from r.x + 6 capitals r.y + 8 (cut before 166), the type chip (`DevTypeChip`, 12 tall) at (170, r.y + 6), `"PP %s"` of `row[i].pp` right at r.x + r.w − 6 capitals r.y + 8 in the row's ink, no shadow; ▲▼ `DevArrowKeys(&kDevGridUp, &kDevGridDown, …)`; the footer: the type chip 10 tall at (20, paper.y + 2), "POW n  ACC n" (`DevPowAcc`) small `DEV_INK2` 4 px after it capitals paper.y + 2, the description small `DEV_INK` from capitals paper.y + 13, two lines (`DevSmallFit`, x 20 to 224); CANCEL `kDevGridExit` dark `DevCancelWord()` (code 235, one tap = B, unchanged). YESNO (the Decision "Relearn YES/NO … keys in the column": unlike the Bag's YES / NO of Task 4, no key in the exit corner) — the question on `kDevGridScreenNarrow` (as today on the narrow screen), YES `DevGridColumnKey(40, 36, 30, 0)` and NO `DevGridColumnKey(40, 36, 30, 1)`, both cream, sunk by `menuCursor`; the move (chip and name) on `kDevGridNote`; no CANCEL. MESSAGE and BLANK as today on the grid names.

- [ ] **Step 1: Write the failing tests** (`relearn` suite):

```c
for (i = 0; i < 5; ++i) AssertRowHits(RlHitV, &rv, DevGridRow(i, 0), PT_RL_HIT_ROW + i);
r = DevGridRow(1, 0);                                                                  /* a row not under the cursor */
assert(Count(170, r.y + 6, 40, 12, UiTypeColor(rv.relearn.row[1].type)) > 0 && Count(214, r.y + 8, 40, 8, DEV_INK) > 0);
AssertHits(RlHitV, &rv, kDevGridExit, PT_RL_HIT_CANCEL); assert(Count(20, 209, 204, 17, DEV_INK) > 0);
rv.relearn.panel = PT_RL_PANEL_YESNO; DevCheckTouch(&ro, &rv);
AssertHits(RlHitV, &rv, DevGridColumnKey(40, 36, 30, 0), PT_RL_HIT_YES); AssertHits(RlHitV, &rv, DevGridColumnKey(40, 36, 30, 1), PT_RL_HIT_NO);
assert(CtrPokeTouch_RelearnHit(&rv, 278, 210) == -1 && CtrPokeTouch_RelearnHit(&rv, 60, 60) == -1);
```

(the suite's `r.x + r.w − 60` chip band moves to x 170). Layout: groups "relearner list" {▲, ▼, `kDevGridExit`} and "relearner YES / NO" {the two column keys}.
- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views test_make_device_boards` → FAIL.
- [ ] **Step 3: Implement** per Interfaces (`DevRlRowDraw`, `DevRlDesc`, `DevRlCancel`, `DevRlList`, `DevRlYesNo`, `CtrPokeTouch_RelearnHit`).
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS.
- [ ] **Step 5: Task close** — renders `relearn_{list,scroll,cancel,yesno,message,message_tap,blank}` vs `def_relearn.txt` (rows [16,48+26i,244,24], chip at 170, "PP n" right at 254, footer chip [20,198,40,10], CANCEL [244,190,68,40]); `python tools/dev.py boards` builds (choice, learn, relearn traced). Azahar: the Two Island relearner — rows, ▲▼, a teach with YES / NO, CANCEL by one tap.
- [ ] **Step 6: Commit** `feat: the relearner as the Definitiva`.
- [ ] **Step 7: PR E2** — full check (as Task 9 Step 7); `git push -u origin feat/b-def-battle-questions`; `gh pr create --base feat/b-def-kit-field --title "feat: the Definitiva battle and questions (commands, moves, choices, forget, relearn)"`; body: each view's change, recipe 0267, the owner's checklist (Tasks 10-15's Azahar lines), the 🤖 line.

## Not in this plan

- Plan 04b: rows 3-5 (OPTIONS, SAVE, the Trainer Card, the Pokédex, the letter, FireRed's picture, idle, naming, the PC box and its recipe 0268, Easy Chat), the "como ficou" boards, README / CHANGELOG. Those views get here only the kit-wide changes (LED, screws, beige ink, DE "ABBRECHEN", the renames of Task 1) and keep their tests green; `DevTallRow` / `LIST_DRAG_ROW 24` (the Pokédex), `kDevTallNote` (the frame), `DEV_SMALL_LINE_TIGHT`'s users, Easy Chat's own ▲▼ copy and the PC box's `UIS_CANCEL` key words are theirs to move.
- Not taken: the board's type colours (Decision), a 7 px HP / EXP tag, a single shadow rule for tags, a drag on the Party's menu, tracing HOME / Party / Summary / Bag / battle for boards, plan-4 speed.
