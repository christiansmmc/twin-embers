# Rework 2, plan 2: the B kit and the views with a board — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** HOME, the battle's commands and moves, the Party, the Summary and the Bag are drawn as the red device (style B) by a new kit; the embers and their OPTIONS row give way to a Shell colour row; `dev.py test` stops rebuilding its host tree.

**Architecture:** A header-only kit (`3ds_device_kit_impl.h`) and a layout header (`3ds_device_layout.h`: the boards' rectangles, shared by drawers, hit tests and a layout test) are included by the unity TU `3ds_poketouch.c` after the battle drawer. Each B view is its own `3ds_device_<view>_impl.h` that takes over its public draw / update / hit functions and deletes the ember code it orphans; v0.6.0's fills, plans and touch handlers stay, with the few fields the boards need added. A host harness (`device_render_test.c`) checks every B view and, with the owner's pack, renders PNGs for the visual check. Two stacked PRs: kit + HOME + battle, then Party + Summary + Bag.

**Tech Stack:** C11 (devkitARM `-std=gnu11` and host gcc `-std=c11`, both `-Werror`), Python 3 tools, edit recipes (`patches/pokefirered/*.json`).

**Spec:** `docs/superpowers/specs/2026-11-14-lower-screen-rework2-design.md` (5.3, 5.4, 5.6 for the embers, 5.9, 7). Brief with the decisions taken as given: `.superpowers/sdd/plan2-brief.md`. Boards (git-ignored, read them for every sub-element this plan does not size): `.progress/design-b/spec-B*.txt` (one line per element: tag [x,y,w,h], colours, text) and `.progress/design-b/project/B*.dc.html`; legends `legendB`, `legendB2` in `project/canvas.json`. v0.6.0 map: `.progress/lower-screen-rework2-recon-v060-ui.md`.

## Global Constraints

- Commits: English Conventional Commits, each ending `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`; PR bodies end `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.
- Before every commit: `python tools/manifest_update.py && python tools/source_audit.py && python tools/source_audit.py --history-since 0afd7a2bdb75a7d1ec2d4d10fb9a84f6f23e08ab` → exit 0. Every file ≤ 262,144 bytes in every commit (`3ds_video.c` is at 258,680: add nothing to it; `3ds_bottom.c` is at 214,654).
- Nothing ROM-derived committed: the pack, `build/dev/src/text.c`'s widths and every render stay under `build/` (git-ignored); `.progress/` is never committed (no `git add -f`).
- `-Werror` on both compilers: a static function or static const table a task leaves unused is deleted in that task, unless a B drawer reuses it.
- Port-authored words through `CTR_TR(en, es, de)`; game strings drawn as the view gives them (cased by `CtrTextCase_*`). FireRed's pack gives two fonts: the normal one (`CtrTwinFontData`) for the boards' 10-12 px text and the small one (`CtrTwinSmallFontData`/`CtrTwinSmallFontWidth`, recipe 0120, today used only by the top screen's battle HUD, 3ds_battle_hud.c:131) for the boards' 7-9 px labels (POW, ACC, PP, Lv, SPECIES/OT/ITEM… labels, KANTO, SPD, the "A USE · B BACK" hint), in the secondary ink; the kit's text call takes the font as a parameter; ▲▼◀▶ are drawn shapes (`DevTriangle`), not glyphs.
- B redraw rule (decision 7): a B view is drawn whole when a field it draws changes (memcmp with the fields it does not draw copied over first); HOME's map window is the one partial case. `Update` must equal a full `Draw` pixel for pixel; every B key is hit exactly where it is drawn (the layout header is the one source of both).
- The chosen key is the game's cursor; a key under a held touch is sunk on the touch frame and is then the only sunk key of its group: `DevChosen(id, cursor, pressed) = pressed >= 0 ? id == pressed : id == cursor`.
- Views without a board in this plan (Dex, OPTIONS, SAVE, Card, PC box, choices, forget/learn, naming, relearn, mail, Easy Chat, idle, the GBA-picture frame) keep the ember look and their START column unchanged until plan 3. Each PR builds and plays.
- New game recipes (0262/0263 are plan 1's): 0264 `party-extras-switch` (Task 6) and 0265 `summary-reports-nature` (Task 7), in that order, each made with `tools/make_recipe.py` against a `build/dev` that already has the ones before it.
- Branches: PR A `feat/b-kit-home-battle` (base `fix/touch-response-v060`), PR B `feat/b-party-summary-bag` (base `feat/b-kit-home-battle`).

## Review Focus

1. German and Spanish labels on the narrowest keys (Summary tabs 56 px, battle row keys 73 px, two-column Party action keys 86 px, HOME keys with their icon): every port-authored key label fits uncut — `test_german_labels_fit` and `test_spanish_labels_fit` (Task 3) build the harness with `-DCTR_LANG_DE` / `-DCTR_LANG_ES`, glyphs 6 px wide, and every view suite ends with `assert(sDevKeyCut == 0)`.
2. The game's cursor on something the B page shows as a key, not a row: the Bag's cursor on CLOSE BAG (`cursor == count`), the Party's on CONFIRM / CANCEL when picking several, the Summary's on the details' CANCEL row — that key is drawn sunk with its LED (Tasks 6, 7, 8).
3. An old `settings.cfg` with `embers=` and no `shell=`: it loads every other key, the shell is red, and the next write has no `embers=` line (Task 2, `wilds_settings_test` cases 15-17).
4. Sparse data: a party of one (five holes), an egg (no HP bar; STATS and MOVES disabled in the Summary), an empty pocket (no rows, CLOSE sunk), a Pokémon with two moves (two move holes) — each drawn, update == draw, holes not hit (Tasks 5-8).
5. HOME's 1:1 map in every region: every town cell and the player fit the 192x128 window, the destination's cursor and its name window stay inside it at the grid's corners, and a blink or a step presents only the window (Task 4).


## Owner decisions after drafting (2026-11-15), applied in the tasks below

1. HOME's map is the game's own Region Map picture at 1:1, no zoom (Task 4).
2. The nature's ▲▼ marks on STATS: recipe 0265 approved (Task 7).
3. HEAL and REMEMBER leave the Party's action menu behind one compile-time switch, `CTR_PARTY_EXTRAS 0`, their code kept (recipe 0264, Task 6). The switch's recipe is 0264 and the nature's 0265 because Task 6 runs before Task 7 and recipes apply in number order.
4. Controller: the pack's small font for the boards' 7-9 px labels (`DevSmallText`, Task 3).
5. Owner (2026-11-15): B views use UPPER CASE like the boards and FireRed (POKéDEX, POKéMON, BAG, SAVE, OPTION, CANCEL, FIGHT, CHARMELEON, POTION, VERMILION CITY). Port-authored key and label words are written upper case (in every CTR_TR language); game strings on B views are shown upper case as FireRed has them — B views do not apply v0.6.0's title-casing (`CtrTextCase_Touch/Battle/Summary`), and if a string arrives already title-cased (e.g. via recipe 0251) the B view upper-cases it for display. FireRed's own spelling exceptions stay: the `é` of POKé/POKéMON/POKéDEX/POKé BALL stays lower case; umlauts and accents upper-case (Ä Ö Ü É …) where the game's charset has them. The top screen and the views still in the ember look are unchanged.

---

### Task 1: `dev.py test` syncs the host tree (decision 11)

**Files:**
- Modify: `tools/dev.py` (`sync`, `pristine`, `put`, `test`, `main`), `.github/workflows/source-audit.yml` (add `python3 -m unittest discover -s tools -p test_dev_tool.py -v` to the "Visible wilds glue text…" step), `docs/review-manifest.json`
- Create: `tools/test_dev_tool.py`

**Interfaces:**
- Produces: `sync(dest: Path = DEV) -> None` (state in `dest/'.dev-sync.json'`), `pristine(dest: Path, name: str) -> bytes | None`, `put(dest: Path, name: str, raw: bytes | None) -> int`; `python tools/dev.py test [SUITE ...]` — with names, only `python -m unittest SUITE ...` runs (MSYS shell, `tools/` as cwd), no sync and no verify-host. Every later task's single-suite runs use this form.

- [ ] **Step 1: Confirm the branch** — `git branch --show-current` → `feat/b-kit-home-battle` (it exists and is checked out; do not create it).
- [ ] **Step 2: Write the failing tests** (`tools/test_dev_tool.py`, `unittest.mock` only: no git, network or MSYS; they run on the Linux CI runner too)

```python
class DevSync(unittest.TestCase):
    def test_sync_writes_only_changed_files(self):
        # dest has .dev-sync.json {commit: lock, recipes: R, patched: [], repo: {'tools/a.py': 'old'}};
        # dev.inputs -> {'tools/a.py': 'new', **R}; ROOT/tools/a.py exists in a temp root (patch dev.ROOT)
        dev.sync(dest)
        self.assertEqual((dest / 'tools/a.py').read_bytes(), (root / 'tools/a.py').read_bytes())
        run.assert_not_called()                       # no bootstrap
    def test_sync_adopts_a_bootstrapped_tree(self):
        # no .dev-sync.json; .twin-bootstrap.json {'inputs': current}; `git rev-parse HEAD` (mocked) -> lock commit
        dev.sync(dest)
        self.assertTrue((dest / '.dev-sync.json').exists()); remove_tree.assert_not_called(); run.assert_not_called()
    def test_sync_rebuilds_another_commit_with_remove_tree(self):
        # .dev-sync.json with another commit; shutil.rmtree patched to raise
        dev.sync(dest)
        remove_tree.assert_called_once_with(dest)
        self.assertIn('--dir', run.call_args.args[0]); self.assertIn(str(dest), run.call_args.args[0])
    def test_test_syncs_the_host_tree_and_filters_suites(self):
        # dev.DEVKITPRO -> a temp tree holding empty msys2/usr/bin/bash.exe and gcc.exe (so the existence check passes on
        # Linux); dev.subprocess.run -> a mock returning returncode 0 that records each command; dev.sync -> a mock
        dev.test(argparse.Namespace(suites=[]));   sync.assert_called_once_with(dev.ROOT / 'build/host-tests')
        self.assertTrue(any('verify-host' in c[-1] for c in bash_commands))   # c = the recorded [bash, '-lc', command]
        bash_commands.clear(); sync.reset_mock()
        dev.test(argparse.Namespace(suites=['test_x'])); sync.assert_not_called()
        self.assertIn('-m unittest test_x', bash_commands[-1][-1]); self.assertFalse(any('verify-host' in c[-1] for c in bash_commands))
```

- [ ] **Step 3: Run** — `cd tools && python -m unittest test_dev_tool -v` → FAIL (`sync()` takes no `dest`).
- [ ] **Step 4: Implement** in `tools/dev.py`: `sync(dest)` uses `remove_tree` (never `shutil.rmtree`); a tree with no `.dev-sync.json` is adopted when its `.twin-bootstrap.json` recipe hashes equal the current ones and `git -C dest rev-parse HEAD` is the lock's commit (state = lock commit, current recipe hashes, `recipe_paths(current recipes)`, `repo` = its inputs minus `patches/`), else removed and bootstrapped as today. `test()` replaces the stale-check/rebuild block with `sync(hosts)`; `test` gets `nargs='*'` `suites`.
- [ ] **Step 5: Run** — the unit tests → PASS. Then `python tools/dev.py test` twice: the second prints `build/host-tests: 0 file(s) updated`, no bootstrap, all stages PASS; edit a comment in `tools/dev.py` → the next run prints `1 file(s) updated` and takes seconds, not ~8.5 min.
- [ ] **Step 6: Commit** `fix: dev.py test syncs the host tree instead of rebuilding it`.

### Task 2: A Shell colour row replaces the embers (decisions 5, 6)

**Files:**
- Modify: `3ds_port/src/3ds_poketouch_options.c` (rows, pages), `3ds_port/include/3ds_poketouch_options.h`, `3ds_port/src/3ds_settings.c`, `3ds_port/include/3ds_settings.h`, `3ds_port/src/main_game_full.c` (the `CtrOptions_EmbersInit` call and its log line go), `3ds_port/src/3ds_bottom.c` (`EmbersStep` and its two calls go; `shell` filled), `3ds_port/src/3ds_poketouch.c` (`CtrPokeTouch_Embers` goes), `3ds_port/src/3ds_touch_skin_impl.h` (the skin's `EmbersStep` and its spark state go), `3ds_port/include/3ds_poketouch.h` (the `CtrPokeTouch_Embers` declaration goes; `uint8_t shell;` added to `CtrPokeTouchView`, `CtrBattleView`, `CtrSummaryUiView`)
- Test: `3ds_port/tests/poketouch_options_test.c`, `3ds_port/tests/settings_test.c`, `3ds_port/tests/wilds_settings_test.c`, `3ds_port/tests/poketouch_render_test.c` (the `PT_WITH_OPTIONS` label list: "Embers" → "Shell colour")

**Interfaces:**
- Produces: `int CtrOptions_Shell(void)` → 0 red, 1 blue, 2 green, 3 grey; `int CtrSettings_Shell(void)`, `void CtrSettings_SetShell(int shell)` (0..3, anything else 0; key `shell=`, written only when non-zero); `CtrPokeTouchView.shell`, `CtrBattleView.shell`, `CtrSummaryUiView.shell`, set in `CtrBottom_Frame` beside the `CtrTextCase_*` calls (`!nav` branch): `state.touch.shell = state.battle.shell = state.sum.shell = (uint8_t)CtrOptions_Shell();`.
- Removed: `CTR_EMBERS_*`, `CtrOptions_Embers/EmbersLow/EmbersInit`, `CtrSettings_Embers/SetEmbers`, `CtrPokeTouch_Embers`. The row count stays 28 (`test_option_labels`).

- [ ] **Step 1: Write the failing tests.** New row order: Graphics page 1 = Frame 5, FPS 6, Day/Night 7, Hour 8, Companion 9; Looks page = Shell colour 10, Red/Leaf/Rival look 11-13 (the spec's "looks section"; look rows keep their indexes).

```c
/* poketouch_options_test.c (replaces every EMBERS check; COMPANION moves from 10 to 9, its levels from optSeg*[5] to [4]) */
CHECK(CtrOptions_RowCount() == 28, ...);
CHECK(strcmp(CtrOptions_Row(9)->label, "Companion") == 0 && strcmp(CtrOptions_Row(10)->label, "Shell colour") == 0, ...);
CHECK(CtrOptions_PageBase(1) == 5 && CtrOptions_PageRows(1) == 5 && CtrOptions_PageBase(2) == 10 && CtrOptions_PageRows(2) == 4
      && CtrOptions_PageBase(3) == 14, ...);                     /* the rest as before */
Text(10, "Red"); CtrOptions_Step(10, 1); Text(10, "Blue");
CHECK(CtrOptions_Shell() == 1 && CtrSettings_Shell() == 1, ...);
CtrOptions_Step(10, -1); CtrOptions_Step(10, -1); Text(10, "Grey");
memset(&view, 0, sizeof(view)); CtrOptions_Fill(&view, 10);
CHECK(view.optCount == 4 && strcmp(view.optLabel[0], "Shell colour") == 0 && view.optSegCount[0] == 0 && !view.optLook.on, ...);
CtrOptions_Fill(&view, 11); CHECK(view.optLook.on, ...);
/* wilds_settings_test.c: cases 15-17, loop bound 14 -> 17 */
case 15: PutS(kFull, 1);  /* the old file: embers=2 among the rest */
         assert(CtrSettings_Shell() == 0 && CtrSettings_Companion() == 2 && CtrSettings_ExpShare() == 2
                && CtrSettings_Turbo() == 3 && CtrSettings_Nuzlocke() == 1);
         CtrSettings_SetShell(2);  /* read the 512-byte block back: */
         assert(strstr(block, "shell=2\n") != NULL && strstr(block, "embers=") == NULL);
case 16: assert(CtrSettings_Shell() == 2);
case 17: PutS("shell=9\n", 1); assert(CtrSettings_Shell() == 0);
/* settings_test.c: the worst case sets CtrSettings_SetShell(3) instead of SetEmbers(2) and stays under 320 bytes */
```

- [ ] **Step 2: Run** — `python tools/dev.py test` → verify-host FAIL (`CtrOptions_Shell` undefined).
- [ ] **Step 3: Implement.** Row: label `CTR_TR("Shell colour", "Color carcasa", "Geh\xC3\xA4usefarbe")` (≤15 bytes), description `CTR_TR("The colour of the", "El color de la", "Farbe des")`, `CTR_TR("lower screen's shell.", "carcasa de abajo.", "unteren Geh\xC3\xA4uses.")`, values `{Red, Blue, Green, Grey}` = `CTR_TR("Red","Rojo","Rot")`, `("Blue","Azul","Blau")`, `("Green","Verde","Gr\xC3\xBCn")`, `("Grey","Gris","Grau")`, get/count/set over `CtrSettings_Shell` (4 values: a "<" value ">" row, not levels). `PAGE3 = 10`; `CtrOptions_IsLevels` keeps COMPANION and EXP SHARE only. Settings: drop `sEmbers` and its load/format lines (an `embers=` line is then skipped like any unknown key); add `sShell` loaded by `shell=%u` (≤ 3 else 0), formatted after `companion` when non-zero.
- [ ] **Step 4: Run** — `python tools/dev.py test` → all PASS; `python tools/dev.py build` → `classicfirered3ds.3dsx` built.
- [ ] **Step 5: Check in Azahar** — `python tools/dev.py run`: no sparks on any screen; OPTIONS → Looks page shows Shell colour (Red) above the three looks; the choice survives a restart.
- [ ] **Step 6: Commit** `feat: a shell colour option replaces the embers`.

### Task 3: The kit, the layout header and the device harness (decisions 5, 9)

**Files:**
- Create: `3ds_port/src/3ds_device_kit_impl.h`, `3ds_port/include/3ds_device_layout.h`, `3ds_port/tests/device_render_test.c`, `3ds_port/tests/device_layout_test.c`, `tools/test_device_views.py`, `tools/render_device_views.py`
- Modify: `3ds_port/src/3ds_poketouch.c` (`#include "3ds_device_kit_impl.h"` right after `#include "3ds_battle_ui_impl.h"`), `tools/dev.py` (`SUITES` gains `test_device_views`; new `render` subcommand), `.github/workflows/source-audit.yml` (step "B device views: draw, hits, layout": `python3 -m unittest discover -s tools -p test_device_views.py -v`), `docs/review-manifest.json`

**Interfaces:**
- Produces (kit; every function `UI_FN`, all colours `UI_RGB` of the spec 5.3 / board values):

| Name | Value |
|---|---|
| `DEV_SHELL_RED/BLUE/GREEN/GREY` | `C23A2E 3E4C6B 2E7A4E 5A5A64` (index = `shell & 3`) |
| `DEV_BEZEL`, `DEV_BEZEL_LINE`, `DEV_PAPER`, `DEV_PAPER2` | `2A2A30 4A4A54 F8F8F4 EFEEE4` |
| `DEV_INK`, `DEV_INK_SH`, `DEV_INK2`, `DEV_INK_DIM` | `626262 D5D5CD 747474 A8A8A0` |
| `DEV_KEY`, `DEV_KEY_EDGE`, `DEV_KEY_INK` (shadow white) | `F2ECE0 D6CCB8 4A4A4A` |
| `DEV_SUNK`, `DEV_SUNK_EDGE`, `DEV_SUNK_INK`, `DEV_SUNK_INK_SH`, `DEV_LED` | `E6DECE CFC4AE 3A3A3A F4EFE4 58E070` |
| `DEV_DARK_INK` | `F2ECE0` (the dark key's label and ▲▼; its face is `DEV_BEZEL`, its top line `DEV_BEZEL_LINE`) |
| `DEV_FAINT`, `DEV_FAINT_EDGE`, `DEV_FAINT_INK` | `D2CAB8 BCB29C 6E6E6E` |
| `DEV_HP_FRAME/TRACK/GREEN/YELLOW/RED`, `DEV_HP_LABEL` | `404850 D8D8D0 48C060 F0C830 E04030`, `F8C830` |
| `DEV_EXP`, `DEV_EXP_LABEL` | `48A0F8 78B8F8` |
| `DEV_FX_SUPER/NOTVERY/EFFECTIVE/NONE` | `289646 CC5418 646A84 7A7A7A` |
| `DEV_CAT_SPECIAL/PHYSICAL`, `DEV_PSN`, `DEV_FNT` | `5A78C0 C03828`, `9050A8`, `7A7A7A` |
| `DEV_LOW_INK`, `DEV_PP_QUARTER`, `DEV_PP_HALF`, `DEV_BLUE_INK`, `DEV_STAT_UP`, `DEV_MALE` | `B03020 B04A10 946E06 2E5AA0 C03828 4878C8` |
| alphas (of 256) `DEV_A_LIGHT/DARK/SCREW/DROP/HOLE/DIM/FAINT_ART` | `64 77 90 97 72 128 115` |

  - `uint16_t DevBlend(uint16_t under, uint16_t over, int alpha)` — per 565 channel `(u * (256 - a) + o * a + 128) >> 8`.
  - `void DevBegin(uint16_t *canvas, uint8_t shell)`; `uint16_t DevGet(int x, int y)` (reads the canvas).
  - `void DevRound(int x, int y, int w, int h, int r, uint16_t c)` — corners cut by the circle of radius r ≤ 10 (a pixel kept when its centre is inside).
  - `void DevShell(int screws)` — `DEV_SCREWS_TOP 1 | DEV_SCREWS_BOTTOM 2`: the screen in the shell colour, rows 0-1 `DevBlend(shell, white, A_LIGHT)`, rows 237-239 `DevBlend(shell, black, A_DARK)`, 5x5 screws (circle) at (2,2) (313,2) (2,232) (313,232) `DevBlend(shell, black, A_SCREW)` with a lighter pixel at their (+1,+1).
  - `CtrPokeTouchRect DevScreen(const CtrPokeTouchRect *outer)` — bezel radius 6 whose outer pixel ring is `DEV_BEZEL_LINE`, a 1 px row under it `DevBlend(under, white, A_LIGHT)`, square paper inset 6; returns the paper.
  - `enum { DEV_LOOK_KEY, DEV_LOOK_DARK, DEV_LOOK_FAINT };` `int DevKey(const CtrPokeTouchRect *r, int look, int sunk, int radius)` — `r` is the resting face (the boards' numbers); up: face at r.y; KEY: 1 px white top line and 2 inner bottom rows `DEV_KEY_EDGE`; DARK: 1 px `DEV_BEZEL_LINE` top line, no bottom edge; FAINT: 2 inner bottom rows `DEV_FAINT_EDGE`, no top line; then 3 drop rows; sunk (any look): face at r.y + 2, 2 inner top rows (`DEV_SUNK_EDGE` on a KEY; the face colour darkened by `A_DROP` on DARK and FAINT), 1 drop row, rows r.y..r.y+1 left to what is under; drop pixels `DevBlend(DevGet(x,y), black, A_DROP)`; LED when sunk: disc of s = h ≥ 30 ? 5 : 4 at (r.x + r.w − 7 − s, faceY + (h ≥ 40 ? 6 : 4)), 1 px glow `DevBlend(face, DEV_LED, 96)` (within 3 px of every board's LED). Returns faceY. Paints only {r.x, r.y, r.w, r.h + 3}.
  - `void DevKeyLabel(const CtrPokeTouchRect *r, int faceY, int look, int sunk, const char *s)` — centred, the look's ink and shadow; cut with "..." when wider than w − 8, and then `++sDevKeyCut` (`static int sDevKeyCut;`, read by the harness).
  - `void DevDim(const CtrPokeTouchRect *r)` — every pixel of {r, h + 3} → `DevBlend(shell, px, A_DIM)` (the group at half strength).
  - `void DevHole(const CtrPokeTouchRect *r, int radius)` — `DevBlend(under, black, A_HOLE)`, its 3 top inner rows a further `A_SCREW` darker, a light row under it (the board's empty bay).
  - `void DevRow(const CtrPokeTouchRect *r, int chosen)` (chosen: `DEV_BEZEL` fill) and `uint16_t DevRowInk(int chosen)` (`DEV_PAPER` / `DEV_INK`).
  - `void DevPaperButton(const CtrPokeTouchRect *r, int pressed)` — HOME's RUN/SPD: white, 1 px `B0B0A8` border, radius 2; pressed inverted (`DEV_BEZEL`).
  - `void DevText(const char *s, int x, int top, int limit)` (`DEV_INK` + `DEV_INK_SH`), `DevText2(...)` (`DEV_INK2`, no shadow), `DevTextRight(const char *s, int right, int top, uint16_t ink)`.
  - `void DevSmallText(const char *s, int x, int top, uint16_t ink, int limit)`, `int DevSmallTextWidth(const char *s)`: the pack's small font (`CtrTwinSmallFontData`/`CtrTwinSmallFontWidth`, declared in poketouch_assets.h) for the boards' 7-9 px labels; the harness's synthetic font stub gets a small-font stub too. (Controller ruling: the small font exists via recipe 0120.)
  - `int DevWrapFirst(const char *s, int x, int top, int firstWidth, int width, int maxLines, uint16_t ink, uint16_t shadow)` and `int DevWrap(const char *s, int x, int top, int width, int maxLines, uint16_t ink, uint16_t shadow)` — port of rework 1's `KitWrapFirst`/`KitWrap` (`git show archive/lower-screen-pages-v1:3ds_port/src/3ds_lower_kit_impl.h`, "\n" read as a space), lines `DEV_LINE_H 13` apart, returns the lines drawn.
  - `void DevTriangle(int cx, int cy, int dir, uint16_t c)` — port of `KitTriangle` (0 up, 1 down, 2 left, 3 right).
  - `void DevHpBar(int x, int y, int w, int h, unsigned hp, unsigned max)` and `uint16_t DevHpColour(unsigned hp, unsigned max)` — FireRed's thresholds (green when hp·2 > max, yellow when hp·5 > max, else red); 1 px frame, track, fill (≥ 1 px while hp > 0).
  - `void DevChip(int x, int y, int w, int h, const char *text, uint16_t bg, uint16_t shadow)` — the small font, ink always white with `shadow` under it (`DEV_WHITE`: none; PSN and FNT have none); `w == 0`: as wide as the text with 4 px padding each side, `w > 0`: that width, the text centred (BMoves' type chips: 40); the boards' fixed chips take `DEV_FX_SUPER_SH`, `DEV_FX_NOTVERY_SH`, `DEV_FX_EFFECTIVE_SH`, `DEV_CAT_SPECIAL_SH`, a chip whose colour comes from data (a type) `uint16_t DevChipShadow(uint16_t bg)`; `int DevLabelChip(int x, int y, const char *text, uint16_t bg, uint16_t ink)` (the "HP"/"EXP" tags).
  - `uint16_t DevPpInk(unsigned pp, unsigned max, uint16_t normal)` — 0 → `DEV_LOW_INK`, ≤ ¼ → `DEV_PP_QUARTER`, ≤ ½ → `DEV_PP_HALF`, else `normal`.
  - `int DevChosen(int id, int cursor, int pressed)`; `void DevGender(int gender, int x, int y)` (v0.6.0's `SGender` shapes, male `DEV_MALE`, female v0.6.0's red; 0 male, 1 female, 2 none as `CtrBottomParty_Gender`).
  - `void DevMonIcon(int slot, unsigned icon, int x, int y, int half, int faint)` — frame 0 of `BPartyIcon(slot, icon)`; `half` = a 2:1 sample (16x16: of each 2x2 block the first non-zero pixel in reading order); `faint` = `DevBlend(DEV_FAINT, px, A_FAINT_ART)`.
- Produces (layout header): `DEV_DROP 3`, `DEV_SINK 2`, `static inline CtrPokeTouchRect DevKeyArea(const CtrPokeTouchRect *r)` = {x, y, w, h + 3}; every later rect `static const CtrPokeTouchRect kDev... __attribute__((unused))` or a `static inline` function.
- Produces (harness): `device_render_test <suite>` (`kit`, later `home battle party summary bag`, and `all`), printing `PASS device <suite>`; env `DEVICE_OUT=dir` writes `dir/<suite>_<state>.ppm`, `FIRERED_TEST_PAK=pak` opens the pack as `poketouch_render_test.c` does (ABI 0x8f71cf3a and its ROM hash) and reads `graphics/fonts/latin_normal.fwlatfont` into the glyphs (missing or stored compressed: prints `NOTE device: synthetic font`), `DEVICE_WIDTHS=file` (256 glyph widths), `DEVICE_FONT_W=n` (stub width, default 5). Helpers: `CtrDirtyList DevCheckTouch(CtrPokeTouchView *old, const CtrPokeTouchView *v)`, `DevCheckBattle(...)`, `DevCheckSummary(...)` — Update into the canvas, copy, Draw, assert equal pixel for pixel, `*old = *v`, return the dirty list; `int DevWhole(CtrDirtyList d)` (one rect, 0,0,320,240); `void clear(uint16_t c)`; `void AssertKey(CtrPokeTouchRect r, int look, int sunk)` (the look's face colour at (x + 3, faceY + h/2); a dimmed key is checked by that pixel directly); `void AssertHits(DevHitFn hit, const void *view, CtrPokeTouchRect r, int code)` with `typedef int (*DevHitFn)(const void *view, int x, int y)` (a key with its drop: code at (x, y) and (x + w − 1, y + h + 2); not at (x − 1, y), (x, y − 1), (x + w, y), (x, y + h + 3)); `void AssertRowHits(DevHitFn hit, const void *view, CtrPokeTouchRect r, int code)` (a row, no drop: code at (x, y) and (x + w − 1, y + h − 1); not at (x − 1, y), (x + w, y)). Rects go by value, so `AssertHits(..., DevPtyBay(i), ...)` is valid C. Each view suite adds its adapter (`HomeHitV`, `BatHitV`, `SumHitV`, `BagHitV`). Stubs: `battle_ui_render_test.c`'s set plus `poketouch_render_test.c`'s map stubs (`richArt`, `iconArt`).
- Produces: `python tools/dev.py render [VIEW ...]` → runs `tools/render_device_views.py --pak build/pak/classicfirered3ds.pak --text build/dev/src/text.c --out build/renders VIEW...` in the MSYS shell; it parses `sFontNormalLatinGlyphWidths` from `text.c` into `build/renders/widths.bin`, builds the harness there, runs each view's suite, converts each PPM to `build/renders/<view>/<state>.png` (`ppm_to_png(ppm: Path, png: Path) -> None`, zlib + struct, RGB8) and prints the paths.

- [ ] **Step 1: Write the failing tests.** `device_layout_test.c` (rules for every rect the header holds: inside 320x240 with its drop, keys ≥ 20 tall and ≥ 28 wide; keys never overlap, drops included, *within one state's group* — the test lists one group per view state (`static const CtrPokeTouchRect kGroup...[]` built from the header) and keys of different states may share space (the Bag's menu keys lie where its CLOSE is in LIST); later tasks add each view's board values and groups) and the `kit` suite:

```c
assert(DevBlend(0xFFFF, 0x0000, 0) == 0xFFFF && DevBlend(0xFFFF, 0x0000, 256) == 0x0000);
assert(DevBlend(UI_RGB(248, 252, 248), 0x0000, 128) == UI_RGB(128, 128, 128));
for (s = 0; s < 4; ++s) {          /* kShell[] = the four shell colours */
    DevBegin(canvas, s); DevShell(DEV_SCREWS_TOP | DEV_SCREWS_BOTTOM);
    assert(at(160, 120) == kShell[s] && at(160, 0) == DevBlend(kShell[s], 0xFFFF, 64) && at(160, 1) == at(160, 0) && at(160, 2) == kShell[s]);
    assert(at(160, 237) == DevBlend(kShell[s], 0, 77) && at(160, 239) == at(160, 237) && at(160, 236) == kShell[s]);
    assert(at(4, 4) == DevBlend(kShell[s], 0, 90) && at(315, 234) == DevBlend(kShell[s], 0, 90)); }
DevBegin(canvas, 0); DevShell(0); assert(at(4, 4) == DEV_SHELL_RED);
p = DevScreen(&(CtrPokeTouchRect){8, 8, 304, 140});
assert(p.x == 14 && p.y == 14 && p.w == 292 && p.h == 128 && at(8, 70) == DEV_BEZEL_LINE && at(9, 70) == DEV_BEZEL
       && at(13, 70) == DEV_BEZEL && at(14, 70) == DEV_PAPER && at(8, 8) == DEV_SHELL_RED && at(160, 148) == DevBlend(DEV_SHELL_RED, 0xFFFF, 64));
k = (CtrPokeTouchRect){8, 156, 96, 36};                     /* HOME's POKeDEX key, on a fresh shell each time */
assert(DevKey(&k, DEV_LOOK_KEY, 0, 6) == 156 && at(11, 174) == DEV_KEY && at(56, 156) == 0xFFFF && at(56, 190) == DEV_KEY_EDGE
       && at(56, 191) == DEV_KEY_EDGE && at(56, 192) == DevBlend(DEV_SHELL_RED, 0, 97) && at(56, 194) == at(56, 192) && at(56, 195) == DEV_SHELL_RED && at(8, 156) == DEV_SHELL_RED);
assert(DevKey(&k, DEV_LOOK_KEY, 1, 6) == 158 && at(56, 157) == DEV_SHELL_RED && at(56, 158) == DEV_SUNK_EDGE && at(11, 176) == DEV_SUNK
       && at(56, 194) == DevBlend(DEV_SHELL_RED, 0, 97) && at(56, 195) == DEV_SHELL_RED && at(94, 164) == DEV_LED);
assert(DevKey(&k, DEV_LOOK_DARK, 0, 6) == 156 && at(11, 174) == DEV_BEZEL && at(56, 156) == DEV_BEZEL_LINE);
assert(DevKey(&k, DEV_LOOK_FAINT, 0, 6) == 156 && at(11, 174) == DEV_FAINT && at(56, 190) == DEV_FAINT_EDGE);
DevKey(&k, DEV_LOOK_KEY, 0, 6); DevDim(&k); assert(at(11, 174) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
DevHole(&(CtrPokeTouchRect){164, 120, 148, 50}, 6); assert(at(238, 150) == DevBlend(DEV_SHELL_RED, 0, 72));
DevRow(&(CtrPokeTouchRect){16, 48, 244, 16}, 1); assert(at(20, 56) == DEV_BEZEL && DevRowInk(1) == DEV_PAPER && DevRowInk(0) == DEV_INK);
DevHpBar(76, 52, 150, 6, 112, 112); assert(at(76, 52) == DEV_HP_FRAME && at(100, 54) == DEV_HP_GREEN);
assert(DevHpColour(50, 112) == DEV_HP_YELLOW && DevHpColour(20, 112) == DEV_HP_RED && DevHpColour(57, 112) == DEV_HP_GREEN);
assert(DevPpInk(15, 15, DEV_KEY_INK) == DEV_KEY_INK && DevPpInk(15, 35, 0) == DEV_PP_HALF && DevPpInk(3, 15, 0) == DEV_PP_QUARTER && DevPpInk(0, 15, 0) == DEV_LOW_INK);
assert(DevChosen(2, 2, -1) && !DevChosen(1, 2, -1) && DevChosen(1, 2, 1) && !DevChosen(2, 2, 1));
clear(0); DevRound(0, 0, 20, 20, 6, 0x1234); assert(at(0, 0) == 0 && at(1, 1) == 0 && at(2, 2) == 0x1234 && at(6, 0) == 0x1234 && at(0, 6) == 0x1234);
/* DevWrap / DevWrapFirst: rework 1's assertions (archive lower_kit_test.c:356-388) with KitWrap -> DevWrap, same 5 px stub font */
sDevKeyCut = 0; DevKeyLabel(&(CtrPokeTouchRect){0, 0, 30, 24}, 0, DEV_LOOK_KEY, 0, "A VERY LONG LABEL"); assert(sDevKeyCut == 1); sDevKeyCut = 0;
```

`tools/test_device_views.py` — `class DeviceViews` (`setUpClass` builds `device_render_test` with `[CC or gcc, -std=c11, -O1, -Wall, -Wextra, -Werror, -I include, -I src, tests/device_render_test.c, src/3ds_pak.c, -lm]` and `device_layout_test`): `test_layout`, `test_kit` (stdout contains `PASS device kit`), `test_german_labels_fit` and `test_spanish_labels_fit` (a build with `-DCTR_LANG_DE`, one with `-DCTR_LANG_ES`, each running `all` with `DEVICE_FONT_W=6` → `PASS device all`).

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views` → FAIL (kit missing).
- [ ] **Step 3: Implement** the kit, the header, the harness, `render_device_views.py` and the dev.py pieces.
- [ ] **Step 4: Run** — `python tools/dev.py test test_device_views` → PASS; `python tools/dev.py test` → all PASS; `python tools/dev.py build` OK.
- [ ] **Step 5: Render** — `python tools/dev.py render kit` → `build/renders/kit/kit_sheet.png` (every look, sunk and not, dark, faint, dim, hole, screen, row, bars, chips on the red shell); the controller compares colours and edges with `BHome.dc.html` / `BParty.dc.html`.
- [ ] **Step 6: Commit** `feat: the red device's drawing kit and its host harness`.

### Task 4: HOME (board B · Início; decision 4; owner decision 1: the game's map at 1:1, no zoom)

**Files:**
- Create: `3ds_port/src/3ds_device_home_impl.h` — defines `DevHomeShown`, `DevHomeDraw`, `DevHomeUpdate`, `DevHomeLabelRect`, `CtrPokeTouch_HomeHit` and the cell functions; `3ds_poketouch.c` includes it right after `#include "3ds_device_kit_impl.h"`, and forward-declares only the three `DevHome*` that `CtrPokeTouch_Draw/Update` (defined earlier in the file) call.
- Delete: `3ds_port/src/3ds_home_map_impl.h` (the 4x image builder: `sHmWorld`, its orbs, fields, bands, `HM_ZOOMS`/`HM_SCALE_*`, `HmDrawRect`).
- Modify: `3ds_port/include/3ds_poketouch.h`, `3ds_port/include/3ds_device_layout.h`, `3ds_port/src/3ds_poketouch.c` (`CtrPokeTouch_MapPrepare` composes the 1:1 window; `MapFeedPixels`, the worker `MapWorker`/`MapJoin`/`sMapThread`, `HomeSetView`, `MapRestore`, `CellView`, `ZoomRect`, `ZoomButton`, `PlayerRect`, `RingD`/`RingRect`/`RingDraw`, `MapRestoreRect`, `HomeShow`, `HomeOverlay`, HOME's branch of `MenuFrame` and `Bar`'s FLY go, then whatever the compiler reports unused; `Gba`, `MapColor`, `IconBuild`/`IconDraw` (re-based on the window), `sPlayer`, `RegionName` stay), `3ds_port/src/3ds_bottom.c`, `3ds_port/src/3ds_bottom_battle_fill_impl.h` (`BattleCommon`'s party loop → `PartyFill`), `3ds_port/include/3ds_bottom.h`, `3ds_port/src/3ds_game_bridge.c`
- Test: `3ds_port/tests/device_render_test.c` (`home`), `device_layout_test.c`, `tools/test_device_views.py` (`test_home`), `tools/test_field_presentation.py`, `3ds_port/tests/poketouch_render_test.c` (its HOME blocks go: the flat-map/ribbon check after the column sweep, the "HOME's pieces over the map" block with its `Hm*` asserts, the `home*` shots; the column `HitTest` and `ViewportToGba` sweeps stay)
- Replaces: v0.6.0's 4x map and its zoom, HOME's `MenuFrame`/`Bar` branches, the START column on HOME.

**Interfaces:**
- Produces in `3ds_poketouch.h`: `#define PT_HOME_X 14`, `PT_HOME_Y 14`, `PT_HOME_W 192`, `PT_HOME_H 128` (the window; `PT_VIEW_*` stay the GBA picture's); `#define PT_HOME_SRC_X 24`, `PT_HOME_SRC_Y 28` (the window's origin in the Region Map's 240x160 screen, rework 1's crop: it holds the whole 22x15 cell grid, screen x 32..207, y 32..151, so every town and the player are inside for every region). Crop or pan is the builder's call, stated in the report: this fixed crop, or a 16 px vertical pan following the destination / player (then `uint8_t mapTop` joins the view and is one of the window-only fields below). `enum { PT_HOME_SPEED = 0x40, PT_HOME_PARTY = 0x48 };` view fields `uint8_t party[6], partyHp[6]; uint16_t partyIcon[6]; uint8_t speed, speedEnabled;`; `mapZoom`, `mapCX`, `mapCY` removed (`mapReady` stays: the art was there). `int CtrPokeTouch_HomeHit(const CtrPokeTouchView *v, int x, int y)` → `PT_POKEDEX..PT_OPTIONS`, `PT_RUN`, `PT_HOME_SPEED`, `PT_HOME_PARTY + slot` (present slots), else `PT_NONE`; `bool CtrPokeTouch_HomeWindowPoint(int x, int y, int *vx, int *vy)`; `bool CtrPokeTouch_HomeCellPoint(const CtrPokeTouchView *v, int cx, int cy, int *x, int *y)` (a cell's centre on the lower screen: x = `PT_HOME_X + 36 + 8 cx − PT_HOME_SRC_X`, y = `PT_HOME_Y + 36 + 8 cy − PT_HOME_SRC_Y`; false off the 22x15 grid); `bool CtrPokeTouch_HomeMapCell(const CtrPokeTouchView *v, int x, int y, int *cx, int *cy)` (the 8x8 cell under a lower-screen point; false off the grid or the window). Removed: `CtrPokeTouch_HomeCellWorld/HomeClamp/HomePick/HomeScale/HomeReveal/HomeZoomHit`.
- `bool CtrPokeTouch_MapPrepare(int group, int version)` (kept): once per (group, version) composes `static uint16_t sHomeMapCols[PT_HOME_W][PT_HOME_H]` (column-major, as the canvas) from `CtrPokeTouch_MapArt` with `Gba(gba[MapColor(..., PT_HOME_SRC_X + dx, PT_HOME_SRC_Y + dy)])` (rework 1's `HomeMapBuild`, `git show archive/lower-screen-pages-v1:3ds_port/src/3ds_home_page_impl.h`); synchronous, no thread; returns whether the art was there.
- Produces in the layout header: `kDevHomeScreen {8,8,304,140}`; `kDevHomeKey[6]` in `PT_*` order `{8,156,96,36} {112,156,96,36} {216,156,96,36} {8,198,96,34} {112,198,96,34} {216,198,96,34}`; `kDevHomeRun {210,123,44,16}`, `kDevHomeSpeed {258,123,44,16}`; `DevHomeCell(i)` = {210 + 31·(i%3), 62 + 28·(i/3), 30, 28}; name at (212,17) 2 lines wide 90, region line (212,44), rule {210,58,92,1}; inside the window: region key {16,16,w,14} (w = text + 14, + 9 with ▶) and FLY {16,122,44,16} (the builder may move either off a town at the render check, reported).
- Produces in bottom.c (port of `fd55b62`): `static void PartyFill(uint8_t kind[6], uint8_t hp[6], uint8_t status[6], uint16_t icon[6])` (icon may be NULL), `static int HomeTouchEnabled(int id)`, `static int HomeTouchActivate(int id)`, `bool CtrBottom_TakeTurboTap(void)` (declared in `3ds_bottom.h`); `static void HomeTap(int cx, int cy)` now takes a cell.
- Produces in the home impl: `static int DevHomeShown(const CtrPokeTouchView *v)` (= `v->home && !v->viewport`), `static void DevHomeDraw(const CtrPokeTouchView *v)`, `static void DevHomeUpdate(const CtrPokeTouchView *old, const CtrPokeTouchView *v, CtrDirtyList *dirty)`, `static int DevHomeLabelRect(const CtrPokeTouchView *v, CtrPokeTouchRect *r)`. Task 8 repeats the same `CtrPokeTouch_Draw/Update` dispatch for the Bag (`DevHomeShown` is tested first; the two never hold together).

- [ ] **Step 1: Write the failing tests.** `test_field_presentation.py`: `test_home_speed_and_party_taps` verbatim from `git show fd55b62 -- tools/test_field_presentation.py`; `test_turbo_takes_the_speed_tap` — `3ds_game_bridge.c` contains `bool turboTap = CtrBottom_TakeTurboTap();` and `(CtrInput_Get()->physicalDown & CTR_KEY_START) != 0 || turboTap,`. Layout test: the board values above, the six keys inside {0,150,320,90}, the region key and FLY inside {14,14,192,128}; group: {six keys, RUN, SPD}. `home` suite (fixture = the board's: "VERMILION CITY" / "KANTO", five Pokémon and a hole, HP 100/66/34/0/8 %, speed 2, `richArt` map prepared, `mapOpen = 3`, no player, no destination):

```c
static const int ids[6] = {PT_POKEDEX, PT_POKEMON, PT_BAG, PT_CARD, PT_SAVE, PT_OPTIONS};
for (i = 0; i < 6; ++i) { AssertHits(HomeHitV, &v, kDevHomeKey[i], ids[i]); AssertKey(kDevHomeKey[i], DEV_LOOK_KEY, 0); }
assert(CtrPokeTouch_HomeHit(&v, 108, 174) == PT_NONE && at(108, 174) == DEV_SHELL_RED);      /* the gap */
assert(CtrPokeTouch_HomeHit(&v, 232, 131) == PT_RUN && CtrPokeTouch_HomeHit(&v, 280, 131) == PT_HOME_SPEED);
assert(CtrPokeTouch_HomeHit(&v, 225, 75) == PT_HOME_PARTY && CtrPokeTouch_HomeHit(&v, 256, 103) == PT_HOME_PARTY + 4
       && CtrPokeTouch_HomeHit(&v, 287, 103) == PT_NONE && CtrPokeTouch_HomeHit(&v, 100, 80) == PT_NONE);
assert(CtrPokeTouch_HomeWindowPoint(14, 14, &vx, &vy) && vx == 0 && vy == 0 && CtrPokeTouch_HomeWindowPoint(205, 141, &vx, &vy)
       && vx == 191 && vy == 127 && !CtrPokeTouch_HomeWindowPoint(206, 80, &vx, &vy) && !CtrPokeTouch_HomeWindowPoint(13, 80, &vx, &vy));
/* the map at 1:1: window pixel (dx, dy) is the Region Map screen's (24 + dx, 28 + dy) (below the region key's rows) */
CtrPokeTouch_MapArt(0, tm, tl, gba, btm, btl);
for (dy = 20; dy < 128; dy += 9) for (dx = 0; dx < 192; dx += 7)
    assert(at(14 + dx, 14 + dy) == Gba(gba[MapColor(tm, tl, btm, btl, 24 + dx, 28 + dy)]));
/* every cell of every region inside the window with room for its 16x16 icon; a tap on it gives it back */
for (g = 0; g < PT_MAP_COUNT; ++g) {
    v.mapGroup = g; DevCheckTouch(&old, &v);
    for (cy = 0; cy < 15; ++cy) for (cx = 0; cx < 22; ++cx) {
        assert(CtrPokeTouch_HomeCellPoint(&v, cx, cy, &x, &y) && x - 8 >= 14 && y - 8 >= 14 && x + 8 <= 206 && y + 8 <= 142);
        assert(CtrPokeTouch_HomeMapCell(&v, x, y, &gx, &gy) && gx == cx && gy == cy); } }
assert(CtrPokeTouch_HomeCellPoint(&v, 0, 0, &x, &y) && x == 26 && y == 22 && !CtrPokeTouch_HomeCellPoint(&v, 22, 0, &x, &y));
v.mapGroup = 0;
/* the destination's name window stays inside the window, at the corners and the middle */
strcpy(v.bar[0], "VERMILION CITY");
static const int corner[5][2] = {{0, 0}, {21, 0}, {0, 14}, {21, 14}, {10, 7}};
for (i = 0; i < 5; ++i) { v.cursorX = corner[i][0]; v.cursorY = corner[i][1]; DevCheckTouch(&old, &v);
    assert(DevHomeLabelRect(&v, &r) && r.x >= 14 && r.y >= 14 && r.x + r.w <= 206 && r.y + r.h <= 142); }
/* a blink or a step: the window alone */
v.cursorFrame ^= 1; d = DevCheckTouch(&old, &v);
assert(d.count == 1 && d.rects[0].x0 == 14 && d.rects[0].y0 == 14 && d.rects[0].x1 == 206 && d.rects[0].y1 == 142);
v.playerX = 21; v.playerY = 14; d = DevCheckTouch(&old, &v); assert(d.count == 1 && d.rects[0].x1 == 206 && d.rects[0].y1 == 142);
v.pressed = PT_POKEMON; assert(DevWhole(DevCheckTouch(&old, &v))); AssertKey(kDevHomeKey[1], DEV_LOOK_KEY, 1); assert(at(198, 164) == DEV_LED);
v.pressed = PT_NONE; v.enabled[PT_SAVE] = 0; DevCheckTouch(&old, &v); assert(at(115, 215) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
v.flyOk = 1; DevCheckTouch(&old, &v); assert(CtrPokeTouch_FlyHit(38, 130));
v.flyOk = 0; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_FlyHit(38, 130));
assert(CtrPokeTouch_HomeRibbonHit(20, 20)); v.mapOpen = 1; DevCheckTouch(&old, &v); assert(!CtrPokeTouch_HomeRibbonHit(20, 20));
v.shell = 2; assert(DevWhole(DevCheckTouch(&old, &v)) && at(160, 152) == DEV_SHELL_GREEN);
assert(sDevKeyCut == 0); puts("PASS device home");
```

With the 16 px pan instead of the fixed crop, the cell loop runs with the player and the destination put on each cell row in turn (the pan follows them), and `mapTop` changes join the window-only case.

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views.DeviceViews.test_home test_field_presentation` → FAIL.
- [ ] **Step 3: Implement.**
  - Draw: `DevShell(TOP | BOTTOM)`, `DevScreen(&kDevHomeScreen)`; the window = `sHomeMapCols` copied column by column (or `C_MAP_WAIT` without art), then its overlays: the player's icon (FireRed's 16x16, `sPlayer`) centred on its cell, the destination as FireRed's own cursor icon (`CtrPokeTouch_MapCursorIcon`, frame `cursorFrame`, as rework 1's `sCursor`) centred on its cell, its name window (`DevHomeLabelRect`: v0.6.0's `LabelRect` rule — 3 px above the 16x16 cursor, else below, kept inside the window — paper with a 1 px `DEV_BEZEL` border, `DevText`), the region key (shown map's `RegionName`, ▶ when `mapOpen` has another map; remembered in `sHomeRibbon`), FLY key while `flyOk` (remembered in `sFlyButton`); `sHomeShown = 1`. Panel: `bar[0]` wrapped in 2 lines; the region line (small font, `DEV_INK2`) `bar[1]`, or `bar[2]` when a destination is chosen and `!flyOk` (the reason); rule; six minis (`DevMonIcon(i, partyIcon[i], cx − 8, y + 3, 1, kind == 2)` and a 22x4 `DevHpBar` at y + 22; an empty slot an 18 px ring `C4C4C4`; a pressed mini a 1 px `DEV_BEZEL` ring); RUN (`DevPaperButton`, `CTR_TR("Run", "Correr", "Rennen")` + a 6x6 lamp: on `48C060`/`1F6A32`, off `D8D8D0`/`A0A098`, `DEV_INK_DIM` text when `!enabled[PT_RUN]`); SPD (`CTR_TR("SPD", "VEL", "TMP")` in the small font, `DEV_INK2`, + "`speed`x", `DEV_INK_DIM` when `!speedEnabled`). Keys: icon = the pack's soft 16x16 `PokeTouch_SoftIcon(sMenuItemArt[id])`, else `sTwinIconRows[id]` in key ink, then the label (`playerName` for the card; v0.6.0's `Button` words for the others), icon + 6 px + label centred; sunk + LED when `pressed == id`; `DevDim` when `!enabled[id]`. All keys `DEV_LOOK_KEY`, radius 6.
  - Update: (1) only `playerX/playerY/playerFemale/cursorFrame` (+ `mapTop` with the pan) differ → the window again (map copy + overlays), dirty `{14,14,192,128}`; (2) else `DevHomeDraw`, whole screen. `CtrPokeTouch_Update`: both old and new HOME → `DevHomeUpdate`; one of them → `CtrPokeTouch_Draw` + whole screen.
  - bottom.c: `sHome` loses `zoom`, `panX`, `panY`, `centered`, `prevX`, `prevY`; `HomeZoom`, `HOME_ZOOMS`, `HOME_MID_X/Y`, `HomeMapState`'s centring and the `CtrPokeTouch_HomeReveal` call go. `ProcessPokeTouch`: hits with `NativeActive() ? CtrPokeTouch_HitTest(x, y) : CtrPokeTouch_HomeHit(&sShown.touch, x, y)` (at touch-down and while held); the zoom branch goes; a touch that begins in the window (`CtrPokeTouch_HomeWindowPoint`) and ends within `MAP_TAP_SLOP` → `CtrPokeTouch_HomeMapCell(&sShown.touch, downX, downY, &cx, &cy)` → `HomeTap(cx, cy)` (a moved touch does nothing: nothing pans); release of a key: `if (inside && FieldEnabled(id) && !HomeTouchActivate(id)) PokeTouchActivate(id);`; `FieldEnabled` asks `HomeTouchEnabled` first (as `fd55b62`); `HomeMapState` fills `party/partyHp/partyIcon` (`PartyFill`) and `speed = CtrSettings_Turbo()` in 2..4 else 1, `speedEnabled = CtrBottom_TurboCanCycle()`. `3ds_game_bridge.c`: `bool turboTap = CtrBottom_TakeTurboTap();` and `|| turboTap` on the START argument of `CtrFastForward_Turbo`.
- [ ] **Step 4: Run** — the Step 2 command → PASS; `python tools/dev.py test` → all PASS (incl. `test_poketouch_render`, `test_column_labels`); build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render home` → `build/renders/home/home_{plain,pressed,selection,flypick,sevii,fainted}.png`; compare with `BHome.dc.html`.
- [ ] **Step 6: Check in Azahar** — each key opens its screen (OPTIONS/SAVE still ember, with their column); SPD cycles the turbo the top screen shows; RUN toggles; a tap picks a town (cursor and name), a second tap or FLY flies, the region key changes map; a mini opens POKéMON; X and Y buttons still work.
- [ ] **Step 7: Commit** `feat: HOME as the red device, with the game's map at 1:1`.

### Task 5: The battle: commands, quick ball, moves, target, Safari, idle (boards B · Comandos, B · Golpes; decisions 1, 10) — then PR A

**Files:**
- Create: `3ds_port/src/3ds_device_battle_impl.h` (defines the public `CtrBattleUi_Draw/Update/Hit`)
- Modify: `3ds_port/src/3ds_battle_ui_impl.h` (v0.6.0's public three renamed `static BEmberDraw/BEmberUpdate/BEmberHit`, kept only for `BUI_PARTY`/`BUI_BAG` until Tasks 6/8; `BScreen`, `BActCard`, `BBallCard`, `BMoveCard`, `BMoveNote`, `BTargetCard`, `BTargetNote`, `BSafariCard` deleted, then whatever the compiler reports unused — the ember Party and Bag still use some shared pieces, e.g. `BEmCancel`, which go in Task 8), `3ds_port/include/3ds_poketouch.h` (`CtrBattleView.activeGender`), `3ds_port/src/3ds_bottom_battle_fill_impl.h` (`activeGender = CtrBottomParty_Gender(slot)`), `3ds_port/include/3ds_device_layout.h`, `3ds_port/src/3ds_poketouch.c` (include)
- Test: `device_render_test.c` (`battle`), `device_layout_test.c` (groups: {FIGHT, three row keys}, {FIGHT, four row keys}, {four move keys, CANCEL}), `tools/test_device_views.py` (`test_battle`), `3ds_port/tests/battle_ui_render_test.c` (its IDLE/ACTION/quick ball/MOVE/matchup/TARGET/SAFARI parts go; PARTY, BAG, SUMMARY stay; the navy check covers the ember states left)

**Interfaces:**
- Consumes: the kit; `CtrBattleView` as filled today (+ `activeGender`).
- Produces (layout): `kDevBatScreen {8,8,304,88}` (ACTION, SAFARI, IDLE), `kDevBatMoveScreen {8,8,304,56}` (MOVE, TARGET); `kDevBatFight {8,104,304,56}`; `kDevBatRow3[3] {8|112|216,170,96,60}` (BAG, POKéMON, RUN), `kDevBatRow4[4] {8|85|162|239,170,73,60}` (BAG, BALL, POKéMON, RUN); ball arrows of a key r: prev {r.x+2, r.y+38, 20, 20}, next {r.x+r.w−22, r.y+38, 20, 20} (hit 3 px larger, as v0.6.0); `kDevBatMove[4] {8|164,72|142,148,62}` (moves, targets, Safari's BALL BAIT ROCK RUN); `kDevBatCancel {236,212,76,20}` (dark pill, radius 10); balls at (228 + 12i, 20), 9x9.
- Produces: `static const CtrPokeTouchRect *DevBatActKey(const CtrBattleView *v, int action)` (action 0-3 or `CTR_ACTION_BALL`) and `static int DevBatKeyOfHit(int hit)` (8/9/10 → `CTR_ACTION_BALL`), used by draw and hit alike.

- [ ] **Step 1: Write the failing tests** (`battle` suite; fixture = the boards': CHARMELEON ♂ Lv36 112/112, balls able/able/status/fainted/able/none, FLAMETHROWER / SCRATCH 15/35 / GROWL / BRICK BREAK 3/15):

```c
v.state = BUI_ACTION; v.cursor = 0; DevCheckBattle(&old, &v);            /* a trainer: three keys */
AssertHits(BatHitV, &v, kDevBatFight, BUI_ACT_FIGHT); AssertKey(kDevBatFight, DEV_LOOK_KEY, 1); assert(at(302, 114) == DEV_LED);
AssertHits(BatHitV, &v, kDevBatRow3[0], BUI_ACT_BAG); AssertHits(BatHitV, &v, kDevBatRow3[1], BUI_ACT_POKEMON); AssertHits(BatHitV, &v, kDevBatRow3[2], BUI_ACT_RUN);
assert(CtrBattleUi_Hit(&v, 108, 200) == -1 && CtrBattleUi_Hit(&v, 160, 50) == -1);
v.runBlocked = 1; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 264, 200) == -1 && at(219, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
v.runBlocked = 0; v.ball = CTR_QUICK_BALL_READY; v.ballCount = 4; v.ballItem = 4; v.ballQty = 21; DevCheckBattle(&old, &v);   /* wild: four keys */
assert(CtrBattleUi_Hit(&v, 121, 180) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, 97, 218) == BUI_ACT_BALL_PREV && CtrBattleUi_Hit(&v, 151, 218) == BUI_ACT_BALL_NEXT
       && CtrBattleUi_Hit(&v, 44, 200) == BUI_ACT_BAG && CtrBattleUi_Hit(&v, 198, 200) == BUI_ACT_POKEMON && CtrBattleUi_Hit(&v, 275, 200) == BUI_ACT_RUN);
v.ballCount = 1; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 97, 218) == BUI_ACT_BALL);
v.ball = CTR_QUICK_BALL_FULL; v.ballCount = 4; DevCheckBattle(&old, &v);       /* grey: dimmed, the throw still hit (the game says why), no arrows */
assert(CtrBattleUi_Hit(&v, 121, 180) == BUI_ACT_BALL && CtrBattleUi_Hit(&v, 97, 218) == BUI_ACT_BALL && at(88, 200) == DevBlend(DEV_SHELL_RED, DEV_KEY, 128));
v.ball = CTR_QUICK_BALL_READY; v.ballCount = 4; v.pressed = BUI_ACT_BALL_NEXT; DevCheckBattle(&old, &v);
AssertKey(kDevBatRow4[1], DEV_LOOK_KEY, 1); AssertKey(kDevBatFight, DEV_LOOK_KEY, 0);                 /* the touch, not the cursor */
v.pressed = -1; DevCheckBattle(&old, &v); v.activeFrame ^= 1; v.showCursor ^= 1; assert(DevCheckBattle(&old, &v).count == 0);   /* not drawn: no redraw */
v.state = BUI_MOVE; v.cursor = 1; assert(DevWhole(DevCheckBattle(&old, &v)));
for (i = 0; i < 4; ++i) AssertHits(BatHitV, &v, kDevBatMove[i], i);
AssertHits(BatHitV, &v, kDevBatCancel, BUI_CANCEL); AssertKey(kDevBatMove[1], DEV_LOOK_KEY, 1);
v.move[2].present = v.move[3].present = 0; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 82, 173) == -1);   /* two moves: holes */
v.state = BUI_TARGET; /* target[0], target[1] foes, target[2] the ally */ DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, kDevBatMove[1], 1); AssertHits(BatHitV, &v, kDevBatCancel, BUI_CANCEL);
v.state = BUI_SAFARI; v.cursor = 2; DevCheckBattle(&old, &v); for (i = 0; i < 4; ++i) AssertHits(BatHitV, &v, kDevBatMove[i], i);
v.state = BUI_IDLE; DevCheckBattle(&old, &v); for (y = 0; y < 240; y += 3) for (x = 0; x < 320; x += 3) assert(CtrBattleUi_Hit(&v, x, y) == -1);
assert(sDevKeyCut == 0); puts("PASS device battle");
```

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views.DeviceViews.test_battle` → FAIL.
- [ ] **Step 3: Implement.**
  - ACTION (BFight): shell with four screws; screen: the lead's icon at 2x from its frame-0 window {4,8,24,24} into {20,18,48,48}; name (76,17); `DevGender` + "Lv%u" (v0.6.0's `CTR_TR`) at (76,31); balls (able `626262`, status `E0B020`, fainted `C8C8C0`, none a 1 px `B0B0A8` ring); `DevLabelChip("HP")` at (76,52) + `DevHpBar` 150x6 after it; "hp/max" right-aligned to 300 at y 48; at (20,71) `CTR_TR("What will %s do?", "\xC2\xBF" "Qu\xC3\xA9 har\xC3\xA1 %s?", "Was soll %s tun?")`. FIGHT key: label at scale 2 in `A02A20` (no icon); row keys: icon + label (BAG = pack `bag_male` as v0.6.0 `BBagPrepare`, POKéMON = `kFlatBall`, RUN = `kFlatShoe` — v0.6.0's `BDiscIcon` art without its disc, kept in this file); BALL key: the ball's item icon (`BItemDraw`), "×qty", ◀ ▶ (`DevTriangle`) in the arrow rects. Hits as v0.6.0's `CtrBattleUi_Hit`: `READY` → the key is `BUI_ACT_BALL`, its arrows `BUI_ACT_BALL_PREV/NEXT` when `ballCount > 1` (drawn only then); `EMPTY`, `FULL`, `AREA` → DIM with v0.6.0's reason word, no arrows, but the whole key is still `BUI_ACT_BALL` (the grey quick ball stays hittable: the game then says why it cannot throw). RUN blocked: DIM and not hit. Sunk key: `DevChosen(key, v->cursor, DevBatKeyOfHit(v->pressed))` — the cursor always counts (`showCursor` is not drawn).
  - MOVE (BMoves): top screws only; screen line 1: name, type chip (`UiTypeColor`), category chip (power "-" → none; `type < 10` PHYSICAL `DEV_CAT_PHYSICAL` else SPECIAL), "POW n ACC n" right-aligned; lines 2-3: `desc` wrapped (2 lines), or without it "STAB" and `CTR_TR("Effect %u%%", "Efecto %u%%", "Effekt %u%%")` in `DEV_INK2`. Move key: name (+10,+8), type chip (+10,+25) 40x12, matchup chip (+54,+25): SUPER for DOUBLE/QUADRUPLE ("Super x4"), NOT VERY for HALF/QUARTER, EFFECTIVE for NEUTRAL, NO EFFECT `DEV_FX_NONE` for IMMUNE (v0.6.0's `BFxBadge` words), or "STATUS" in `DEV_INK2` when power is "-" and there is no chip; "PP n/m" right-aligned at +42 in `DevPpInk`. Absent move: `DevHole`. CANCEL pill; hint at (12,216) in white `CTR_TR("A: use   B: back", "A: usar   B: volver", "A: nutzen   B: zur\xC3\xBC" "ck")`.
  - TARGET: keys in `kDevBatMove`: name, Lv, `DevHpBar` 72x5 (ally: hp/max), side chip FOE `DEV_STAT_UP` / ALLY `DEV_BLUE_INK`, matchup chip; screen: v0.6.0's `BTargetNote` words. SAFARI: keys BALL BAIT ROCK RUN with v0.6.0's art; screen `CTR_TR("What will you do?", "\xC2\xBF" "Qu\xC3\xA9 vas a hacer?", "Was tun?")`. IDLE: ACTION's screen without the prompt, FIGHT and the row DIM, nothing hit.
  - `CtrBattleUi_Update`: old and new both PARTY or both BAG → `BEmberUpdate`; else copy old, copy `activeFrame` and `showCursor` from new, memcmp → if different `CtrBattleUi_Draw` + whole screen.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS (`test_battle_ui` keeps PARTY/BAG/SUMMARY); build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render battle` → `build/renders/battle/battle_{action,action_ball,action_runblocked,moves,moves_two,target,safari,idle}.png`; compare with `BFight.dc.html` and `BMoves.dc.html`.
- [ ] **Step 6: Commit** `feat: the battle's commands and moves on the red device`.
- [ ] **Step 7: PR A boundary — full check.** `python tools/dev.py test` (all PASS), `python tools/dev.py build`, the audit line, `python tools/dev.py render` (every view so far; the controller compares each PNG with its board), `python tools/dev.py run`: HOME (Task 4's list); a wild battle: FIGHT → moves → CANCEL, BALL ◀ ▶ by tap and swipe, a throw; a trainer battle: three keys, RUN grey; BAG and POKéMON still open the ember Bag and Party and come back; OPTIONS → Shell colour recolours HOME and the battle.
- [ ] **Step 8: Push and open PR A** — `git push -u origin feat/b-kit-home-battle`; `gh pr create --base fix/touch-response-v060 --title "feat: the red device's kit, HOME and battle"`; body: the tooling fix, embers → Shell colour, the kit and harness, HOME, battle; test results; the Step 7 check list; ends with the 🤖 line.

### Task 6: The Party: bays, action menu, YES/NO, message, level-up, several (boards B · POKéMON and its action menu; decisions 2, 10; owner decision 3: the HEAL/REMEMBER switch, recipe 0264)

**Files:**
- Create: `3ds_port/src/3ds_device_party_impl.h`, `patches/pokefirered/0264-party-extras-switch.json` (game file `src/party_menu.c`: HEAL and REMEMBER leave the menu behind `CTR_PARTY_EXTRAS 0`, owner decision 3)
- Modify: `3ds_port/include/3ds_poketouch.h` (`CtrBattleParty`: `uint8_t menuBlue, menuDark;` bit i per menu row), `3ds_port/src/3ds_bottom_battle_fill_impl.h` (`PartyMenuMarks`, called by `BattlePartyFill` for `PANEL_ACTIONS`; 0 for YES/NO), `3ds_port/src/3ds_battle_ui_impl.h` (ember `BParty`, `BPanel`, `BPMenuRow`, `BPartyPanel`, `BPartyKeepRing`, `BPartyEntry`, `BPCancelButton`, `BPNoteCard`, `BPartyOverlay`, `BPLevelUp` and the PARTY branches of `BEmber*` deleted, then whatever the compiler reports unused; `BPNoteCard`'s per-mode words move to the B Party), `3ds_port/include/3ds_party_grid.h` and `3ds_port/tests/party_grid_test.c` (comments: `BPMenuRow` → `DevPtyMenuKey`), layout header, pt.c (include)
- Test: `device_render_test.c` (`party`), `device_layout_test.c` (the grid check; groups: {six bays, CANCEL}, {six bays, CONFIRM, CANCEL low}, {menu keys of n} for n = 1..8, {OK}), `tools/test_device_views.py` (`test_party`), `tools/test_field_presentation.py` (`test_party_extras_switch`, `test_party_menu_marks`), `battle_ui_render_test.c` (PARTY section and `partyMenuGrid` go)

**Interfaces:**
- Produces (layout): `DevPtyBay(i)` = {i&1 ? 164 : 8, 8 + 56·(i>>1), 148, 50}; `kDevPtyNote {8,180,222,52}`, `kDevPtyCancel {238,180,74,50}`, several: `kDevPtyConfirm {238,180,74,22}` + `kDevPtyCancelLow {238,206,74,24}`; with a menu (n = `menuCount`, YES/NO n = 2): `DevPtySlim(n, i)` = {8, 8 + 28i, n ≤ 4 ? 188 : 116, 24}, `DevPtyMenuKey(n, i)` = 1 column {206, 8 + 28·row, 106, 24}, 2 columns {132 + 94·col, 8 + 28·row, 86, 24} (col/row from `CtrPartyGrid_Col/Row`), `kDevPtyMenuNote {8,182,304,50}`; level-up `kDevPtyLevelScreen {8,8,304,166}` with OK in `kDevPtyCancel`'s place.
- Produces (fill): `static void PartyMenuMarks(const unsigned char *ids, int n, uint8_t *blue, uint8_t *dark)` — blue: id ≥ 18 (`CURSOR_OPTION_FIELD_MOVES`: the field moves in blue), dark: id 2 or 9 (`CURSOR_OPTION_CANCEL1/2`), as `#define`s naming `party_menu.h`'s constants.
- Produces: `static void DevPartyDraw(const CtrBattleView *v)`, `static int DevPartyHit(const CtrBattleView *v, int x, int y)` (the `BUI_PTY_*` codes as today), dispatched by `CtrBattleUi_*` for `BUI_PARTY`.

- [ ] **Step 1: Branch** — `git checkout -b feat/b-party-summary-bag feat/b-kit-home-battle`.
- [ ] **Step 2: Pin the switch — write the failing test** `test_party_extras_switch` (`tools/test_field_presentation.py`, beside the other recipe tests; CI already runs that suite). It reads the recipe's added lines only (`additions('0264-party-extras-switch.json', 'src/party_menu.c')`; unchanged lines are stored as spans, so the two action adds must be port text — they move into a helper):

```python
text = additions('0264-party-extras-switch.json', 'src/party_menu.c')
self.assertIn('#define CTR_PARTY_EXTRAS 0', text)
helper = function(text, 'CtrAppendPartyExtras')
self.assertIn('CURSOR_OPTION_RELEARN);', helper); self.assertIn('CURSOR_OPTION_HEAL);', helper)
blocks = re.findall(r'#if CTR_PARTY_EXTRAS\n(.*?)#endif', text, re.S)
self.assertTrue(any('static void CtrAppendPartyExtras(void)' in b for b in blocks))   # the helper: compiled only with the switch
self.assertTrue(any('CtrAppendPartyExtras();' in b for b in blocks))                   # its one call, likewise
doc = json.loads((ROOT / 'patches/pokefirered/0264-party-extras-switch.json').read_text())
self.assertEqual([f['path'] for f in doc['files']], ['src/party_menu.c'])
```

- [ ] **Step 3: Make recipe 0264** — copy `build/dev/src/party_menu.c` (it has 0235 HEAL, 0238 REMEMBER and 0257 applied) to `build/work/src/`; there: above `SetPartyMonFieldSelectionActions` add `#ifndef CTR_PARTY_EXTRAS` / `#define CTR_PARTY_EXTRAS 0 /* 1 brings HEAL and REMEMBER back to the Party's menu; their code stays compiled */` / `#endif` and, under `#if CTR_PARTY_EXTRAS`, `static void CtrAppendPartyExtras(void)` holding the two `if (sPartyMenuInternal->numActions < 7) AppendToList(..., CURSOR_OPTION_RELEARN / CURSOR_OPTION_HEAL)` adds; in `SetPartyMonFieldSelectionActions` replace those two adds (inside its `#ifdef PLATFORM_3DS`) with `#if CTR_PARTY_EXTRAS` `CtrAppendPartyExtras();` `#endif`. Everything else of HEAL and REMEMBER (`CursorCB_Heal`, `CursorCB_Relearn`, their texts, the dispatch by id) stays as it is (still referenced: -Werror quiet). Then `python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/0264-party-extras-switch.json src/party_menu.c`.
- [ ] **Step 4: Run and commit** — `python tools/dev.py build` (sync applies 0264), `python tools/dev.py test test_field_presentation` → PASS; manifest + audit; commit `feat: HEAL and REMEMBER leave the Party's menu, behind one switch`.
- [ ] **Step 5: Write the failing tests.** `test_party_menu_marks` (extracts `PartyMenuMarks` with `function(battle, ...)` and runs `run_c`): `static const unsigned char a[] = {0, 1, 3, 19, 20, 2}, c[] = {10, 0, 2}; PartyMenuMarks(a, 6, &b, &d); assert(b == 0x18 && d == 0x20); PartyMenuMarks(c, 3, &b, &d); assert(b == 0 && d == 0x04);`. Layout: `partyMenuGrid()` ported from `battle_ui_render_test.c` onto `DevPtyMenuKey` (pitch 28, h 24, columns at x 206 or 132/226). `party` suite (fixture = the board's five + a hole: CHARMELEON 112/112 Lv36, PIDGEOTTO 62/94, RATICATE PSN, NIDORINO fainted, ONIX 5/64):

```c
for (i = 0; i < 5; ++i) AssertHits(BatHitV, &v, DevPtyBay(i), BUI_PTY_SLOT + i);
assert(CtrBattleUi_Hit(&v, 238, 145) == -1 && at(238, 150) == DevBlend(DEV_SHELL_RED, 0, 72));      /* the hole */
AssertKey(DevPtyBay(0), DEV_LOOK_KEY, 1); AssertKey(DevPtyBay(3), DEV_LOOK_FAINT, 0); AssertHits(BatHitV, &v, kDevPtyCancel, BUI_PTY_CANCEL);
v.pty.mode = BUI_PTY_FORCED; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 275, 205) == -1 && at(275, 205) == DEV_SHELL_RED);
v.pty.mode = BUI_PTY_MULTI; v.pty.button = 1; DevCheckBattle(&old, &v);
AssertHits(BatHitV, &v, kDevPtyConfirm, BUI_PTY_CONFIRM); AssertHits(BatHitV, &v, kDevPtyCancelLow, BUI_PTY_CANCEL); AssertKey(kDevPtyCancelLow, DEV_LOOK_DARK, 1);
v.pty.mode = BUI_PTY_SWITCH; v.pty.panel = BUI_PTY_ACTIONS; v.pty.menuCount = 3; v.pty.menuCursor = 0; v.pty.menuDark = 0x04; DevCheckBattle(&old, &v);
for (i = 0; i < 3; ++i) AssertHits(BatHitV, &v, DevPtyMenuKey(3, i), BUI_PTY_MENU + i);
AssertKey(DevPtyMenuKey(3, 2), DEV_LOOK_DARK, 0); assert(CtrBattleUi_Hit(&v, 100, 20) == -1);           /* bays: not while a menu is open */
v.pty.menuCount = 6; v.pty.menuBlue = 0x18; v.pty.menuDark = 0x20; DevCheckBattle(&old, &v);
assert(CtrBattleUi_Hit(&v, 175, 20) == BUI_PTY_MENU && CtrBattleUi_Hit(&v, 175, 76) == BUI_PTY_MENU + 2 && CtrBattleUi_Hit(&v, 269, 20) == BUI_PTY_MENU + 3 && CtrBattleUi_Hit(&v, 269, 76) == BUI_PTY_MENU + 5);
v.pty.menuCount = 8; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 269, 104) == BUI_PTY_MENU + 7);
v.pty.panel = BUI_PTY_YESNO; v.pty.menuCount = 2; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 259, 20) == BUI_PTY_MENU && CtrBattleUi_Hit(&v, 259, 48) == BUI_PTY_MENU + 1);
v.pty.panel = BUI_PTY_TEXT; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 10, 10) == BUI_PTY_MESSAGE && CtrBattleUi_Hit(&v, 300, 230) == BUI_PTY_MESSAGE);
v.pty.panel = BUI_PTY_LEVELUP; DevCheckBattle(&old, &v); assert(CtrBattleUi_Hit(&v, 160, 100) == BUI_PTY_MESSAGE); AssertKey(kDevPtyCancel, DEV_LOOK_KEY, 1);
v.pty.panel = BUI_PTY_NONE; DevCheckBattle(&old, &v); v.pty.iconFrame[0] ^= 1; assert(DevCheckBattle(&old, &v).count == 0);   /* icons are not animated in B */
v.pty.egg[1] = 1; DevCheckBattle(&old, &v); AssertHits(BatHitV, &v, DevPtyBay(1), BUI_PTY_SLOT + 1);   /* an egg: no bar, still a bay */
for (i = 1; i < 6; ++i) v.pty.present[i] = 0; DevCheckBattle(&old, &v);                                /* a party of one */
for (i = 1; i < 6; ++i) { CtrPokeTouchRect r = DevPtyBay(i); assert(CtrBattleUi_Hit(&v, r.x + 74, r.y + 40) == -1 && at(r.x + 74, r.y + 40) == DevBlend(DEV_SHELL_RED, 0, 72)); }
assert(sDevKeyCut == 0); puts("PASS device party");
```

- [ ] **Step 6: Run** — `python tools/dev.py test test_device_views.DeviceViews.test_party test_field_presentation` → FAIL.
- [ ] **Step 7: Implement.** No screws. Bay: `DevKey` (KEY, FAINT when fainted or `entry == BUI_PTY_NOT_ABLE`; sunk = `DevChosen(i, p->cursor, pressed − BUI_PTY_SLOT)`); icon 1:1 at (+4, faceY + 9) (faint 45 %), egg: icon and name only; name (+38, +8); `DevLabelChip("HP")` + 56x5 bar at (+38, +29) (grey chip `8A8A8A`/`E0E0E0` when fainted); hp/max right at +26 (`DEV_LOW_INK` in the red zone); right at +38: "Lv%u" in `DEV_INK2`, or a status chip (v0.6.0's `BPartyPanel` words; PSN `DEV_PSN`, FNT `DEV_FNT`, the rest v0.6.0's tints), or the TM/item verdict chip (v0.6.0's `sBPartyTagText`), or when picking several the order "1"/"2"/"3" chip. Screen: `p->message` (TEXT) or `p->caption` or v0.6.0's `BPNoteCard` words per mode, wrapped in 3 lines; CANCEL dark (not drawn when FORCED); several: CONFIRM key + CANCEL dark, sunk by `p->button`. Menu: slim bays (icon 2:1, name, a 72x4 bar; Lv and numbers only when 188 wide), keys sunk by `DevChosen(i, menuCursor, pressed − BUI_PTY_MENU)`, blue rows `DEV_BLUE_INK`, dark rows `DEV_LOOK_DARK`; screen `kDevPtyMenuNote` with `p->caption`. Level-up: the screen with name, "Lv n", the six stats and "+gain", the message in `kDevPtyNote`, OK `CTR_TR("OK","OK","OK")` sunk. Update: masked `iconFrame`, else whole.
- [ ] **Step 8: Run** — Step 6 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 9: Render** — `python tools/dev.py render party` → `build/renders/party/party_{bays,menu3,menu6,menu8,yesno,message,levelup,multi,forced,one}.png`; compare with `BParty.dc.html` and `BPartyMenu.dc.html` (the board's six-in-a-column menu becomes two columns, decision 2).
- [ ] **Step 10: Check in Azahar** — field Party: SWITCH, SUMMARY, ITEM, a field move (blue), CANCEL by touch and D-pad (≥ 5 actions: LEFT/RIGHT across); no HEAL or REMEMBER in the menu, and the Two Island Move Relearner still opens its screen; battle Party: switch, FORCED after a faint, an item's target; Rare Candy's card.
- [ ] **Step 11: Commit** `feat: the Party on the red device`.

### Task 7: The Summary: INFO, STATS (IV/EV), MOVES, move details, MEMO (boards B · Resumo; recipe 0265)

**Files:**
- Create: `3ds_port/src/3ds_device_summary_impl.h` (defines `CtrSummaryUi_Draw/Update/Hit/Tab`; keeps, moved from the deleted file, v0.6.0's `SNumber`, `SIcon`, `SPic`, `SExpAt`, `SDigits`, `SExpPercent`, `SHint` and the IV/EV data logic), `3ds_port/include/3ds_nature.h`, `patches/pokefirered/0265-summary-reports-nature.json`
- Delete: `3ds_port/src/3ds_summary_ui_impl.h` (pt.c includes the new file in its place)
- Modify: `3ds_port/include/3ds_summary.h` (`unsigned char natureId;` after `nature`), layout header
- Test: `device_render_test.c` (`summary`), `device_layout_test.c` (group: {four tabs, ▲, ▼, BACK}), `tools/test_device_views.py` (`test_summary`), `tools/test_field_presentation.py` (`test_summary_reports_the_nature`), `battle_ui_render_test.c` (SUMMARY section and its navy check go)

**Interfaces:**
- Produces (layout): `kDevSumTab[4]` = {8 + 60i, 8, 56, 24} (INFO, STATS, MOVES, MEMO = `SUI_TAB_TRAINER`), `kDevSumPrev {252,8,28,24}` (▲), `kDevSumNext {284,8,28,24}` (▼), `kDevSumScreen {8,40,304,140}` (left column {14,46,104,128} in `DEV_PAPER2`), `kDevSumNote {8,188,228,44}`, `kDevSumBack {244,188,68,42}`, `DevSumRow(i)` = {124, 52 + 24i, 176, 22} (row 4 = the details' CANCEL), `kDevSumIvSwitch {228,49,70,13}` (hit {224,46,78,20}); picture box {34,82,64,64} (`SPic` centred there, the 80x80 one included).
- Produces: `static inline int CtrNature_Up(int nature)` / `CtrNature_Down(int nature)` → the Summary's stat index 0..4 (ATTACK, DEFENSE, SP. ATK, SP. DEF, SPEED) or −1 (neutral, or ≥ 25): the third generation's rule, raised `n / 5`, lowered `n % 5` in the order ATK, DEF, SPEED, SP. ATK, SP. DEF, mapped `k == 2 ? 4 : k > 2 ? k − 1 : k` (arithmetic, no table: clean-release audit).

- [ ] **Step 1: Write the failing tests.** `test_summary_reports_the_nature`: `additions('0265-summary-reports-nature.json', 'src/pokemon_summary_screen.c')` contains `out->natureId = GetNature(&screen->currentMon);` and the recipe edits only that file. `summary` suite (fixture = the boards': CHARMELEON ♂ Lv36, ADAMANT = 3, BLAZE, FLAMETHROWER ...):

```c
assert(CtrNature_Up(3) == 0 && CtrNature_Down(3) == 2 && CtrNature_Up(15) == 2 && CtrNature_Down(15) == 0
       && CtrNature_Up(10) == 4 && CtrNature_Up(0) == -1 && CtrNature_Down(24) == -1 && CtrNature_Up(25) == -1);
for (i = 0; i < 4; ++i) AssertHits(SumHitV, &sv, kDevSumTab[i], SUI_HIT_TAB + i);
AssertHits(SumHitV, &sv, kDevSumPrev, SUI_HIT_PREV); AssertHits(SumHitV, &sv, kDevSumNext, SUI_HIT_NEXT); AssertHits(SumHitV, &sv, kDevSumBack, SUI_HIT_BACK);
AssertKey(kDevSumTab[0], DEV_LOOK_KEY, 1); AssertKey(kDevSumPrev, DEV_LOOK_DARK, 0);
sv.data.page = SUI_PAGE_SKILLS; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 263, 55) == SUI_HIT_IVTOGGLE); AssertKey(kDevSumTab[1], DEV_LOOK_KEY, 1);
sv.ivView = 1; DevCheckSummary(&so, &sv);
sv.data.page = SUI_PAGE_MOVES; DevCheckSummary(&so, &sv);
for (i = 0; i < 4; ++i) AssertRowHits(SumHitV, &sv, DevSumRow(i), SUI_HIT_MOVE + i);
assert(CtrSummaryUi_Hit(&sv, 263, 55) != SUI_HIT_IVTOGGLE && CtrSummaryUi_Hit(&sv, 212, 159) == -1);
sv.data.page = SUI_PAGE_MOVES_INFO; sv.data.cursor = 4; DevCheckSummary(&so, &sv);
AssertRowHits(SumHitV, &sv, DevSumRow(4), SUI_HIT_CANCEL); assert(at(130, 159) == DEV_BEZEL);        /* the cursor on CANCEL: its row inverted */
assert(CtrSummaryUi_Hit(&sv, 266, 20) == -1 && CtrSummaryUi_Hit(&sv, 298, 20) == -1);                 /* ▲▼ off in the details */
sv.data.swapping = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 212, 159) == -1);
sv.data.swapping = 0; sv.trainer = 1; sv.data.page = SUI_PAGE_INFO; DevCheckSummary(&so, &sv); AssertKey(kDevSumTab[3], DEV_LOOK_KEY, 1);
sv.data.egg = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 96, 20) == -1 && CtrSummaryUi_Hit(&sv, 156, 20) == -1 && CtrSummaryUi_Hit(&sv, 216, 20) == SUI_HIT_TAB + 3);
sv.loading = 1; DevCheckSummary(&so, &sv); assert(CtrSummaryUi_Hit(&sv, 36, 20) == -1 && CtrSummaryUi_Hit(&sv, 278, 209) == SUI_HIT_BACK);
assert(sDevKeyCut == 0); puts("PASS device summary");
```

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views.DeviceViews.test_summary test_field_presentation` → FAIL.
- [ ] **Step 3: Make the recipe** — copy `build/dev/src/pokemon_summary_screen.c` to `build/work/src/`, add `out->natureId = GetNature(&screen->currentMon);` after the `COPY(out->nature, ...)` line, then `python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/0265-summary-reports-nature.json src/pokemon_summary_screen.c` (it confirms the recipe reproduces the edit).
- [ ] **Step 4: Implement.** No screws. Tabs (`CTR_TR`: "Info"; "Stats"/"Stats"/"Status"; "Moves"/"Ataques"/"Attacken"; "Memo"/"Notas"/"Memo"), sunk on `CtrSummaryUi_Tab(v)` or the pressed one, DIM when disabled (egg, loading); ▲▼ dark keys (`DevTriangle` in `DEV_DARK_INK`), DIM in the details or loading. Left column: INFO/STATS/MEMO = name, `DevGender` + "Lv", picture, type chips; MOVES = 32x32 icon, name, Lv, types; details = the cursor move's type + category chips, POWER, ACCURACY (no CONTACT: the bridge does not report it). Right: INFO = SPECIES "No.%s species", OT, ITEM, NATURE, ABILITY, HP rows (label `DEV_INK2`, value right-aligned), 172x6 HP bar at (126,150), EXP chip + bar at (126,159); STATS (`spec-BSumStats.txt` / `BSumStats.dc.html`) = the nature's name (small font, `DEV_INK2`) and the switch (STATS | IV/EV, the shown half in `3E4C6B` white) on the line at y 49; six rows HP..SPEED from y 65, **13 tall, 14 apart** (65, 79, 93, 107, 121, 135; the last ends at 148): the label 48 wide in the small font (raised `DEV_STAT_UP` with ▲, lowered `DEV_BLUE_INK` with ▼, from `CtrNature_*(natureId)`, else `DEV_INK2`) at x 126, a 76x4 bar (`8A94A8` on `E2E2DA`, the stat over the largest of the six) after it, the value right-aligned to 298; the IV/EV view = v0.6.0's IVs (bar = IV / 31), EVs and Hidden Power in these rows; the EXP line at y 152 (chip, a 92x4 bar, "NEXT n"); MOVES/details = `DevSumRow` rows (name, "pp" right in `DevPpInk`), the cursor's row `DevRow` inverted in the details, row 4 "CANCEL" unless swapping; MEMO = OT, ID No., Nature, Ribbons rows and the memo (egg: the hatch text). Bottom screen: INFO/STATS ability name in `DEV_SUNK_INK` then `abilityDesc` (`DevWrapFirst`, 2 lines); MOVES v0.6.0's `SHint`; details `moveDesc` (swapping: v0.6.0's hint); MEMO v0.6.0's "Up / down" hint. BACK dark. Update: whole on any difference.
- [ ] **Step 5: Run** — `python tools/dev.py build` (sync applies 0265), Step 2 → PASS, `python tools/dev.py test` → all PASS.
- [ ] **Step 6: Render** — `python tools/dev.py render summary` → `build/renders/summary/summary_{info,stats,ivev,moves,details,details_cancel,memo,egg}.png`; compare with `BSumInfo`, `BSumStats`, `BSumMoves`.
- [ ] **Step 7: Check in Azahar** — from the field Party and the battle Party: tabs by touch and L/R/LEFT/RIGHT (TRAINER→MEMO stays the last), ▲▼ and UP/DOWN, a move's details, a swap, IV/EV, BACK; ADAMANT shows ATTACK ▲ SP. ATK ▼.
- [ ] **Step 8: Commit** two commits: `feat: the Summary reports the nature's number` (recipe, `3ds_summary.h`, its test), then `feat: the Summary on the red device`.

### Task 8: The Bag: field Bag, TM Case, Berry Pouch, shop BUY, item PC, mailbox and the battle Bag (boards B · BAG and its item menu; decision 3) — then PR B

**Files:**
- Create: `3ds_port/src/3ds_device_bag_impl.h`
- Modify: `3ds_port/include/3ds_poketouch.h` (`PT_BAG_ROWS 8`, `BUI_BAG_ROWS 8`, `PT_BAG_HIT_CLOSE = 630`; `PT_BAG_HIT_RAIL`, `BUI_BAG_RAIL`, `CtrPokeTouch_BagRailTarget`, `CtrBattleUi_BagRailTarget` removed), `3ds_port/src/3ds_field_bag_impl.h` (keeps `FBWindow`, `FBLlButton` and what they need — plan-3 views call them: `3ds_field_box_impl.h`, `3ds_field_relearn_impl.h`, `3ds_field_choice_impl.h`; the Bag drawer, `CtrPokeTouch_BagHit`, its rects and the rest go, then whatever the compiler reports unused, `FBCard`/`FBButton` included if nothing else calls them), `3ds_port/src/3ds_battle_ui_impl.h` (ember `BBag`, `BTabs`, `BBagRow`, `BBagList`, `BBagDetail`, `BBagMessage`, `BRow`, `BMenuRow` and the `BEmber*` scaffolding go, then whatever the compiler reports unused, e.g. `BEmCancel`; the helpers B uses — `BPartyIcon`, `BItemIcon`/`BItemDraw`, the battle art — stay), `3ds_port/src/3ds_bottom_field_fill_impl.h` (`FieldBagFill`: `first = clamp(cursor − 3, 0, max(0, count − PT_BAG_ROWS))`, rows only below `count`), `3ds_port/src/3ds_bottom_battle_fill_impl.h` (`BattleBagFill`: the same `first`), `3ds_port/src/3ds_bottom.c` (`ProcessTouch`: a mode with `FieldListFor(mode) != NULL` goes to `ProcessFieldBagTouch` before `ProcessRouteTouch`/`ProcessBack`, and the SHOP/ITEMPC/MAILBOX branch drops `ProcessBack` — B's CLOSE replaces BACK, and the column's rects lie over B's ▲▼ and CLOSE; `ProcessFieldBagTouch`: `PT_BAG_HIT_CLOSE` → `BagPlanStart(list, BAGPLAN_PRESS, 0, 0, K_B)`, `#define BAG_DRAG_ROW 16` for the drag, the rail branch goes; `ProcessBattleBagTouch`: the rail branch goes), `3ds_port/src/3ds_poketouch.c` (`CtrPokeTouch_Draw/Update` branch for `DevBagShown(v)` = `v->bag.on && !v->viewport`, as HOME's), layout header, `tools/dev.py` (`SUITES` loses `test_battle_ui`)
- Delete: `3ds_port/tests/battle_ui_render_test.c`, `tools/test_battle_ui.py` (every state they drew is B now)
- Test: `device_render_test.c` (`bag`), `device_layout_test.c` (groups: LIST {three pockets, ▲, ▼, CLOSE}, a case's LIST {case key, ▲, ▼, CLOSE}, ACTIONS {menu keys of n} for n = 1..6, QUANTITY {up, down, OK, CANCEL}, YES/NO {two keys}), `tools/test_device_views.py` (`test_bag`), `tools/test_bag_pockets.py` (reads `kDevBagPocketLabel[3] = {UIS_BAG_ITEMS, UIS_BAG_KEY_ITEMS, UIS_BAG_BALLS}` from `3ds_device_bag_impl.h`), `tools/test_field_presentation.py` (the `CtrBattleUi_BagRailTarget` stub line goes), `poketouch_render_test.c` (its field Bag block goes)

**Interfaces:**
- Produces (layout): `kDevBagPocket[3]` = {8 + 104i, 8, 96, 24}; `kDevBagCaseKey {8,8,304,24}`; `kDevBagTitle {8,6,304,28}` (a screen); list screen `kDevBagList {8,40,260,144}`, with a menu / quantity / YES-NO `kDevBagListNarrow {8,40,184,144}`; `DevBagRow(i, narrow)` = {16, 48 + 16i, narrow ? 168 : 244, 16}; `kDevBagUp {276,40,36,66}`, `kDevBagDown {276,114,36,66}`; `kDevBagDesc {8,190,228,42}` (icon {18,199,24,24}, text at (46,197) width 180, 2 lines), narrow `kDevBagDescNarrow {8,190,184,42}`; `kDevBagClose {244,190,68,40}`; `DevBagMenuKey(n, i)` = {200, 40 + p·i, 112, h}, p = n ≤ 5 ? 38 : 32, h = n ≤ 5 ? (i == n − 1 ? 38 : 30) : 26, the last one dark; quantity: up {200,40,112,30}, count screen {200,76,112,40}, down {200,122,112,30}, OK {200,158,112,30}, CANCEL {200,194,112,36}.
- Produces: `typedef struct DevBagPage` (keys: count 0-3 with names, chosen and asked / a case's one key / a title and its money; rows; cursor, first, count; `closeChosen` = `cursor == count`; the selected item, its icon and text; state LIST/ACTIONS/YESNO/QUANTITY/MESSAGE; menu names and cursor; count and sale; the message), `static void DevBagFromField(const CtrPokeTouchBag *b, DevBagPage *p)`, `static void DevBagFromBattle(const CtrBattleBag *b, DevBagPage *p)`, `static void DevBagDraw(const DevBagPage *p, int pressed)`, `static int DevBagHit(const DevBagPage *p, int x, int y)` returning `DEV_BAG_HIT_*`, mapped by `CtrPokeTouch_BagHit` to `PT_BAG_HIT_*` and by `CtrBattleUi_Hit` to `BUI_BAG_*`.

- [ ] **Step 1: Write the failing tests** (`bag` suite; fixture = the board's eight items POTION ×5 … MAX REPEL ×3 of nine, `cursor = 1`, `asked = 0xFF`, desc "A spray-type wound medicine…"; `bv` = a `BUI_BAG` view with the same rows, `pocket = 0`, `berries = 0`):

```c
for (i = 0; i < 3; ++i) AssertHits(BagHitV, &v, kDevBagPocket[i], PT_BAG_HIT_TAB + i);
AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 1);
v.bag.asked = 2; DevCheckTouch(&old, &v); AssertKey(kDevBagPocket[2], DEV_LOOK_KEY, 1); AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 0); v.bag.asked = 0xFF;
for (i = 0; i < 8; ++i) AssertRowHits(BagHitV, &v, DevBagRow(i, 0), PT_BAG_HIT_ROW + i);
assert(at(20, 72) == DEV_BEZEL);                                                                  /* row 1, the cursor's, inverted */
assert(CtrPokeTouch_BagHit(&v, 294, 73) == -1 && CtrPokeTouch_BagHit(&v, 294, 147) == PT_BAG_HIT_DOWN);   /* nothing above yet */
AssertHits(BagHitV, &v, kDevBagClose, PT_BAG_HIT_CLOSE);
v.bag.cursor = v.bag.count; DevCheckTouch(&old, &v); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1);     /* the game's cursor on CLOSE BAG */
v.bag.count = 0; v.bag.cursor = 0; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 138, 56) == -1); AssertKey(kDevBagClose, DEV_LOOK_DARK, 1);
v.bag.count = 9; v.bag.cursor = 1;
/* the item menu: USE GIVE TOSS REGISTER CANCEL */
v.bag.state = PT_BAG_STATE_ACTIONS; v.bag.menuCount = 5; v.bag.menuCursor = 0; DevCheckTouch(&old, &v);
for (i = 0; i < 5; ++i) AssertHits(BagHitV, &v, DevBagMenuKey(5, i), PT_BAG_HIT_MENU + i);
AssertKey(DevBagMenuKey(5, 4), DEV_LOOK_DARK, 0); assert(CtrPokeTouch_BagHit(&v, 56, 20) == -1 && CtrPokeTouch_BagHit(&v, 196, 56) == -1);
assert(at(11, 20) == DevBlend(DEV_SHELL_RED, DEV_SUNK, 128));                                     /* the pockets at half strength */
v.bag.menuCount = 6; DevCheckTouch(&old, &v); AssertHits(BagHitV, &v, DevBagMenuKey(6, 5), PT_BAG_HIT_MENU + 5);
v.bag.state = PT_BAG_STATE_QUANTITY; DevCheckTouch(&old, &v);
assert(CtrPokeTouch_BagHit(&v, 256, 55) == PT_BAG_HIT_QTY_UP && CtrPokeTouch_BagHit(&v, 256, 137) == PT_BAG_HIT_QTY_DOWN
       && CtrPokeTouch_BagHit(&v, 256, 173) == PT_BAG_HIT_QTY_OK && CtrPokeTouch_BagHit(&v, 256, 212) == PT_BAG_HIT_QTY_CANCEL);
v.bag.state = PT_BAG_STATE_YESNO; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 256, 55) == PT_BAG_HIT_MENU && CtrPokeTouch_BagHit(&v, 256, 93) == PT_BAG_HIT_MENU + 1);
v.bag.state = PT_BAG_STATE_MESSAGE; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 5, 5) == PT_BAG_HIT_MESSAGE && CtrPokeTouch_BagHit(&v, 315, 235) == PT_BAG_HIT_MESSAGE);
v.bag.state = PT_BAG_STATE_LIST; v.bag.kind = PT_BAG_KIND_TMCASE; strcpy(v.bag.title, "TM CASE"); DevCheckTouch(&old, &v);
AssertKey(kDevBagCaseKey, DEV_LOOK_KEY, 1); assert(CtrPokeTouch_BagHit(&v, 160, 20) == -1);
v.bag.kind = PT_BAG_KIND_SHOP; strcpy(v.bag.title, "POKe MART"); v.bag.money = 3000; DevCheckTouch(&old, &v); assert(CtrPokeTouch_BagHit(&v, 160, 20) == -1);   /* the title */
/* the battle Bag */
DevCheckBattle(&bo, &bv); AssertKey(kDevBagPocket[0], DEV_LOOK_KEY, 1);
assert(CtrBattleUi_Hit(&bv, 56, 20) == -1 && CtrBattleUi_Hit(&bv, 160, 20) == -1);                 /* no Berry Pouch: no BERRIES */
bv.bag.berries = 365; DevCheckBattle(&bo, &bv); AssertHits(BatHitV, &bv, kDevBagPocket[1], BUI_BAG_BERRIES);
AssertRowHits(BatHitV, &bv, DevBagRow(0, 0), BUI_BAG_ROW); AssertHits(BatHitV, &bv, kDevBagClose, BUI_BAG_CANCEL);
bv.bag.menuCount = 2; DevCheckBattle(&bo, &bv); AssertHits(BatHitV, &bv, DevBagMenuKey(2, 0), BUI_BAG_MENU); AssertHits(BatHitV, &bv, DevBagMenuKey(2, 1), BUI_BAG_MENU + 1);
bv.bag.menuCount = 0; strcpy(bv.bag.message, "..."); DevCheckBattle(&bo, &bv); assert(CtrBattleUi_Hit(&bv, 5, 5) == BUI_BAG_MESSAGE);
assert(sDevKeyCut == 0); puts("PASS device bag");
```

- [ ] **Step 2: Run** — `python tools/dev.py test test_device_views.DeviceViews.test_bag test_bag_pockets test_field_presentation` → FAIL.
- [ ] **Step 3: Implement.** No screws. Keys row: the Bag's three pockets (`CtrUiText(kDevBagPocketLabel[i])`, sunk = `asked != 0xFF ? asked : pocket`), a case's one key with `title` sunk + LED, shop / item PC / mailbox a title screen (`title`; the shop's money right-aligned), the battle Bag ITEMS sunk + BERRIES when `berries`; DIM outside LIST. List: rows `DevRow` + name and "×qty" right-aligned (a shop's price as v0.6.0's row prints it), the cursor's row inverted, `DEV_INK_DIM` text outside LIST; ▲▼ tall keys with `DevTriangle`, ink `B8B0A0` when nothing that way. Below: item icon + `selDesc` (a TM/HM: its type chip first; a shop: "In Bag n"), CLOSE dark (`closeName`, else `CTR_TR("Close", "Cerrar", "Schlie\xC3\x9f" "en")`), sunk when `cursor == count`. ACTIONS: narrow list, menu keys (`menuName`, sunk by `menuCursor`/pressed, the last dark), the prompt (or "%s is selected." from `selName`) on the narrow screen. QUANTITY: the column of the interface block, "×count" (and the sale / money when buying or selling) on its screen. YESNO: two keys, `prompt` on the narrow screen. MESSAGE: the message wrapped in the list screen. Update: whole on any difference.
- [ ] **Step 4: Run** — Step 2 → PASS; `python tools/dev.py test` → all PASS; build OK.
- [ ] **Step 5: Render** — `python tools/dev.py render bag` → `build/renders/bag/bag_{items,close,empty,menu,menu6,quantity,yesno,message,tmcase,pouch,shop,itempc,mailbox,battle,battle_berries}.png`; compare with `BBag.dc.html` and `BBagMenu.dc.html`.
- [ ] **Step 6: Commit** `feat: the Bag on the red device`.
- [ ] **Step 7: PR B boundary — full check.** As Task 5 Step 7 plus: field Bag (pockets by touch and D-pad, rows, drag, ▲▼, CLOSE, USE/GIVE/TOSS/REGISTER, a toss quantity), TM Case and Berry Pouch from the Bag, a mart's BUY, the item PC's WITHDRAW, the mailbox, the battle Bag (an item, BERRIES and back, CANCEL), GIVE from the Party; and the Party / Summary lists of Tasks 6-7.
- [ ] **Step 8: Push and open PR B** — `git push -u origin feat/b-party-summary-bag`; `gh pr create --base feat/b-kit-home-battle --title "feat: the Party, the Summary and the Bag on the red device"`; body: the three views, recipes 0264 (HEAL/REMEMBER switch) and 0265 (the nature), the tests that left with the ember states (`battle_ui_render_test.c`, `test_battle_ui.py`), the Step 7 list; ends with the 🤖 line.
- [ ] **Step 9: Owner's console check** (spec 5.9, end of 5.4) — ask the owner to run PR B on the 3DS with the Step 7 list and every board's view.

## Not in this plan

- Plan 3: the views without a board, the GBA-picture frame's position constant, the START column and route code (spec 7.2), the ember shell/skin/backdrop and their generated assets and tools (spec 5.6). Plan 4: speed (5.7). The "B — como ficou" boards (5.5) start with plan 3's views.
