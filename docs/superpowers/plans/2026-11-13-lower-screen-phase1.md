# Lower Screen Rework, Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the ember look of HOME and the battle's command, move, target and Safari views with a FireRed-made kit (its frames, font, arrow, icons) in full-screen layouts designed by the owner, keeping every function.

**Architecture:** The lower screen is one translation unit: `3ds_port/src/3ds_poketouch.c` includes the theme, skin, battle, summary and field `*_impl.h` headers, all `static`, sharing `sCanvas`, `Px`, `Fill`, `Glyph`, `Art`. Phase 1 adds three impl headers (the kit, the HOME page, the battle pages) and one layout header whose rectangles come from the owner's Claude Design handoff. Old views keep their old drawing until phase 2. Game materials are read from the player's pack at run time with `CtrData_Load`.

**Tech Stack:** C (gnu11, devkitARM, `-Wall -Wextra -Werror`), host tests in C compiled with gcc (msys2's on this machine), Python 3 test runners, `tools/dev.py` for builds, Azahar and the console.

**Spec:** `docs/superpowers/specs/2026-11-13-lower-screen-rework-design.md`

## Global Constraints

- Canvas 320x240 RGB565, column-major, bottom to top: `sCanvas[(size_t)x * H + (H - 1 - (size_t)y)]`; views redraw only when they change; every component draws only inside the rectangle it is given.
- Game art at 1:1 or an integer factor, never fractional.
- No game pixels, palettes or tables in the source: frames, fonts, icons, the map and palettes come from the pack at run time (the clean-release audit counts any read-only table in a game object as FireRed data).
- The source audit caps every file at 256 KiB: `3ds_video.c` is 59 bytes under it, `3ds_bottom.c` is at 234 KiB; new code goes in new files.
- Minimum touch size 24x24 px for buttons, 16 px tall for list rows.
- Frames, FireRed's rule: message and description panels use the dialogue frame (`graphics/text_window/menu_message.4bpp` + `stdpal_0.gbapal`); lists and menus use the OPTIONS -> FRAME type (`graphics/text_window/type1..10.4bpp` + `.gbapal`, index = `optFrameType` 0..9 + 1).
- Selection: FireRed's arrow, charmap byte `0xEF` (written `"\x01\xEF"` in port strings), 8 px wide, text 8 px to its right, in the window's text colours (stdpal index 2 on index 1, shadow index 3); always shown on the current choice.
- Every view is a full-screen page opened from HOME; BACK or B returns to HOME. Until a view is reworked it keeps its old frame, column and bar.
- The lower screen is not tinted at night. No new game states (no "RUN unavailable", no "disabled move").
- Every commit updates `docs/review-manifest.json` for the files it adds or changes (`python tools/manifest_update.py`, Task 1) and passes `python tools/source_audit.py`; CI audits each commit (`--history`).
- Commits: English, Conventional Commits, ending with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Never push to main; one PR into the fork (`gh pr create --repo christiansmmc/twin-embers`).
- Host tests on this machine: `PATH="/c/devkitPro/msys2/usr/bin:$PATH" CC=C:/devkitPro/msys2/usr/bin/gcc.exe`.

## Review Focus

1. OPTIONS -> FRAME changed while HOME is shown: every list/menu panel redraws with the new frame, no stale tiles (Task 2 test `frame_type_change_redraws`).
2. A pack without the text-window entries (old or damaged pack, `CtrData_Load` returns NULL): panels fall back to a 1 px outline on white instead of garbage or a crash (Task 2 test `frame_fallback_without_pack`).
3. Long text (a 7-letter player name, the longest location names) in captions, buttons and rows: clipped at the rectangle, never drawn outside it (Task 3 test `text_clipped_to_rect`).
4. A tap on the map panel outside the 22x15 grid, on the caption strip, or between buttons: no Fly target and no action (Task 5 tests `home_map_cell_bounds`, `home_hit_gaps`).
5. Arrow and game cursor out of step after mixing touch and D-pad in battle: the arrow is always on FireRed's current choice, or on the pressed button while a touch is held (Task 6 test `arrow_follows_cursor_and_press`).

---

### Task 1: Claude Design brief and the manifest helper

**Files:**
- Create: `docs/superpowers/specs/2026-11-13-lower-screen-claude-design-brief.md` (pt-BR, for the owner)
- Create: `tools/manifest_update.py`
- Modify: `docs/review-manifest.json`

**Interfaces:**
- Produces: `python tools/manifest_update.py [--basis TEXT]`: for every added, modified or untracked file in `git status --porcelain -uall` except the manifest, sets `sha256` (of LF-normalised bytes) or appends an entry with `review_status: reviewed`, credit `christiansmmc (fork changes with disclosed AI assistance)`, licence `MIT only for original owned contributions; third-party rights unchanged`; appends the fork credit to an existing entry's credit once. Writes the manifest with `json.dumps(d, indent=1, ensure_ascii=False)` and no trailing newline (the file's current format). Deleted files are removed from the manifest.

- [ ] **Step 1: Write the brief** with these sections and values (all from the spec and the measured code):
  - Plataforma: tela de baixo do 3DS, 320x240, toque com caneta; desenhada pixel a pixel pelo jogo (sem HTML/CSS em tempo de execução); projetar em 320x240 a 1x e visualizar ampliado 2x/3x sem suavização.
  - Grade de 8 px (as molduras do FireRed são feitas de blocos 8x8); larguras e alturas de painéis com moldura em múltiplos de 8.
  - Peças do jogo como blocos marcados (nunca colar arte do jogo): "MOLDURA DIÁLOGO" (borda visível ~5 px em cima e embaixo, ~11 px nos lados, mínimo 40x32 px), "MOLDURA FRAME" (anel de 8 px, interior branco), "SETA ▶" (8x14), "ÍCONE ITEM 24x24", "ÍCONE POKÉMON 32x32", "MAPA 192x128".
  - Texto: fonte do jogo, linha de 14 px, maiúsculas com 9 px de altura, letras até 10 px de largura; fonte pequena com linha de 13 px; texto cinza escuro com sombra cinza clara sobre branco (para o mockup: #626262 / #D5D5CD / #FFFFFF).
  - Toque: botões ≥ 24x24 px, linhas de lista ≥ 16 px.
  - Modelo de páginas: cada tela ocupa 320x240; BACK/B volta para a inicial.
  - Fora: degradê, brilho, sombra, cantos muito arredondados, chaves liga/desliga, partículas, escala quebrada.
  - Elementos obrigatórios por tela (copiar o "Functional inventory" do spec): HOME; comandos da batalha (com a Poké Ball rápida e a variante do Safari); golpes (com a escolha de alvo em batalha dupla).
  - Entrega: o zip de handoff do Claude Design (README + .dc.html) com x, y, largura e altura em px a 1x de cada elemento.
- [ ] **Step 2: Write `tools/manifest_update.py`** with the interface above.
- [ ] **Step 3: Register and audit**

Run: `python tools/manifest_update.py && python tools/source_audit.py`
Expected: JSON with `"reviewed_files": 686` (684 + the brief + the helper).

- [ ] **Step 4: Commit**

```bash
git add docs/superpowers/specs/2026-11-13-lower-screen-claude-design-brief.md tools/manifest_update.py docs/review-manifest.json
git commit -m "docs: Claude Design brief for the lower screen; manifest helper"
```

- [ ] **Step 5: Hand the brief to the owner** (link the file; they start designing while Tasks 2-8 proceed).

### Task 2: Kit frames from the pack, with the kit's host test

**Files:**
- Create: `3ds_port/src/3ds_lower_kit_impl.h`
- Modify: `3ds_port/src/3ds_poketouch.c` (include the kit after the skin include at :322)
- Create: `3ds_port/tests/lower_kit_test.c`
- Create: `tools/test_lower_kit.py`
- Modify: `.github/workflows/source-audit.yml` (run `python3 tools/test_lower_kit.py` next to `test_poketouch_render.py`)

**Interfaces:**
- Consumes: `FrameLoadFile(const char *base, const char *ext, uint8_t *out, uint32_t cap, uint32_t want)` (PT.c:984), `GbaColor`, `Px`, `Fill`.
- Produces (static, in the kit header):
  - `enum { KIT_FRAME_DIALOGUE, KIT_FRAME_STD };`
  - `static int KitFrameReady(int kind, int stdType)`: loads and caches the frame (`stdType` 0..9; dialogue ignores it): dialogue = `menu_message.4bpp` 576 B + `stdpal_0.gbapal` 32 B; std = `type<stdType+1>.4bpp` 288 B + `.gbapal` 32 B. Reloads the std cache when `stdType` changes. Returns 1 when the materials are loaded.
  - `static void KitPanel(const CtrPokeTouchRect *r, int kind, int stdType)`: draws the frame so its outer edge is `r` (`r->w`, `r->h` multiples of 8), the interior filled with palette index 1; index 0 pixels are not drawn (the caller paints the page first). Without materials: a 1 px outline in `KitInk(2)` and a white interior.
  - `static uint16_t KitInk(int index)`: stdpal_0 colour `index` (1 white, 2 text, 3 shadow) as RGB565 via `GbaColor`; when the pack has no stdpal_0, the port's own constants (`EM_DARK`, `EM_DARK_SH`, white).
- Tile placement (from FireRed, `src/menu.c` and `src/new_menu_helpers.c`; tiles are 8x8 4bpp, 32 B each, row-major in the file):
  - Std (9 tiles): top row `0, 1 x n, 2`; side rows `3 … 5`; bottom row `6, 7 x n, 8`; tile 4 never placed; no flips.
  - Dialogue (18 tiles), columns A B interior C D, `V` = vertical flip: row 1 `0 1 2xn 3 4`; row 2 `5 6 … 8 9`; upper middle rows `10 11 … 12 13`; lower middle rows `V10 V11 … V12 V13`; row −2 `V5 V6 … V8 V9`; last row `V0 V1 V2xn V3 V4`. FireRed's height is 6 tile rows; a taller panel repeats the upper middle row in its upper half and the lower middle row in its lower half. Minimum 5x4 tiles.

- [ ] **Step 1: Write `lower_kit_test.c`**: unity build like `poketouch_render_test.c` (`#include PT_SOURCE`, the same bridge and font stubs copied from it); its own `CtrData_Load` serves synthetic entries: each tile `t` of `typeN.4bpp`/`menu_message.4bpp` filled with index `(t % 14) + 1` in its top half and `(t % 14) + 2` in its bottom half (so flips are visible), palettes with 16 distinct colours; a flag makes it return NULL. Tests, each a function called from `main`, asserting with a sentinel colour painted over the whole canvas first:
  - `frame_std_layout`: `KitPanel(&(CtrPokeTouchRect){16,16,64,40}, KIT_FRAME_STD, 0)`: pixel (16,16) has tile 0's top colour; (24,16) and (64,16) tile 1's; (72,16) tile 2's; (16,24) tile 3's; (40,32) palette index 1; (16,48) tile 6's bottom colour; (15,15) and (80,56) still the sentinel.
  - `frame_dialogue_flip`: `KitPanel(&(CtrPokeTouchRect){0,0,64,48}, KIT_FRAME_DIALOGUE, 0)`: row 2 of the column A tile in tile row 2 has tile 10's top colour, the same position in tile row 3 has tile 10's bottom colour (vertical flip); total height 48 px.
  - `frame_dialogue_tall`: an 80 px tall dialogue panel repeats the upper middle row in tile rows 2-4 and the lower middle row in rows 5-7.
  - `frame_type_change_redraws`: draw with `stdType` 0, then 3: the edge pixel colour changes to type4's.
  - `frame_fallback_without_pack`: with the NULL flag set and caches cleared, `KitPanel` draws a 1 px `KitInk(2)` outline and a white interior, nothing outside.
- [ ] **Step 2: Write `tools/test_lower_kit.py`**: compiles `lower_kit_test.c` + `3ds_pak.c` like `test_poketouch_render.py` (same flags, `CC` env) and runs it; asserts exit 0 and `PASS lower kit` in stdout.
- [ ] **Step 3: Run it to see it fail**

Run: `PATH="/c/devkitPro/msys2/usr/bin:$PATH" CC=C:/devkitPro/msys2/usr/bin/gcc.exe python tools/test_lower_kit.py`
Expected: FAIL, compile error on `KitPanel` undeclared.

- [ ] **Step 4: Implement the kit header's frame part** (interfaces above) and include it from `3ds_poketouch.c` after `3ds_touch_skin_impl.h`. Mark helpers `UI_FN` until a view uses them (`-Werror` on unused statics).
- [ ] **Step 5: Run the test and the existing render tests**

Run: the command of Step 3, then `python tools/test_poketouch_render.py` and `python tools/test_battle_ui.py` with the same environment.
Expected: `PASS lower kit`; the two existing runners still pass (their baseline comparisons skip in this checkout).

- [ ] **Step 6: Commit** (`python tools/manifest_update.py` first) — `feat: FireRed window frames for the lower screen kit`.

### Task 3: Kit text, rows, icon cells, caption strip, HP bar, type label

**Files:**
- Modify: `3ds_port/src/3ds_lower_kit_impl.h`
- Modify: `3ds_port/tests/lower_kit_test.c`

**Interfaces:**
- Consumes: Task 2's `KitInk`; `UiText(const char *s, int x, int top, uint16_t fg, uint16_t shadow, int scale, int limit)` (draws the glyph cell at `top - 3`), `UiTextWidth`, `UiTextRight`, `Art`, `Tone`, `UiTypeColor`, `UiHpColor`.
- Produces:
  - `#define KIT_ARROW "\x01\xEF"`, `#define KIT_ARROW_W 8`, `#define KIT_LINE_H 14`
  - `static void KitText(const char *s, int x, int lineTop, int limitX)`: FireRed's line box: glyph cell at `lineTop`, so `UiText(s, x, lineTop + 3, KitInk(2), KitInk(3), 1, limitX)`.
  - `static void KitRow(const CtrPokeTouchRect *r, const char *label, int selected, int enabled)`: erases `r` with `KitInk(1)`, arrow at `r->x` when `selected`, label at `r->x + KIT_ARROW_W`, vertically centred on a 14 px line, clipped at `r->x + r->w`; disabled text through `Tone(c, 0)`.
  - `static void KitIconCell(const CtrPokeTouchRect *r, const uint8_t *tiles, const uint16_t *pal, int iconW, int iconH, const char *label, int selected, int pressed, int enabled)`: interior `KitInk(1)` (pressed: `KitInk(3)`), icon 1:1 centred in the space above the label, label one line at the bottom, arrow left of the label when `selected`.
  - `static void KitCaption(const CtrPokeTouchRect *r, const char *left, const char *right)`: one line, `left` from `r->x + 4`, `right` right-aligned at `r->x + r->w - 4`, both clipped so they never overlap (the left text is cut first).
  - `static void KitHpBar(int x, int y, int w, int hp, int maxHp)`: 2 px tall bar on a 4 px track; FireRed's thresholds on a 48 px scale (`hp * 48 / maxHp`: > 24 green, > 9 yellow, else red); colours from `UiHpColor`.
  - `static void KitTypeLabel(const CtrPokeTouchRect *r, int type)`: flat fill `UiTypeColor(type)`, name in white centred, no rounding.

- [ ] **Step 1: Add the failing tests** (the font stub gets a solid 8x14 ink block for code `0xEF` and 6 px wide blocks for letters):
  - `row_arrow_when_selected`: `KitRow({8,8,96,16}, "BAG", 1, 1)` has ink pixels in x 8..15; with `selected = 0` none there.
  - `text_clipped_to_rect`: a 30-letter label in `KitRow({8,8,64,16}, …)`, and the longest caption pair in `KitCaption({8,8,120,16}, …)`: no pixel changes outside the rect.
  - `caption_no_overlap`: left and right texts wider than the strip: the right text is whole, the left text ends at least 4 px before it.
  - `icon_cell_1to1`: a 24x24 synthetic icon appears as a 24x24 block (no scaling) centred above the label.
  - `hp_thresholds`: bar colour at hp/max 25/48, 10/48, 9/48 equals `UiHpColor` green, yellow, red.
  - `disabled_row_toned`: disabled label pixels equal `Tone(KitInk(2), 0)`.
- [ ] **Step 2: Run to see them fail** (command of Task 2 Step 3). Expected: compile error on `KitRow`.
- [ ] **Step 3: Implement** the interfaces above.
- [ ] **Step 4: Run** Task 2 Step 5's commands. Expected: all pass.
- [ ] **Step 5: Commit** — `feat: lower screen kit rows, icon cells, captions and bars`.

### Task 4: Layout header with its invariants

**Files:**
- Create: `3ds_port/include/3ds_lower_layout.h`
- Create: `3ds_port/tests/lower_layout_test.c`
- Modify: `tools/test_lower_kit.py` (also builds and runs the layout test)

**Interfaces:**
- Produces (`static const CtrPokeTouchRect`, `CtrPokeTouchRect` from `3ds_poketouch.h`):
  - HOME: `kHomeMapPanel` (std frame; interior holds the 192x128 map at `kHomeMapPanel.x + 8, .y + 8` and the caption strip under it), `kHomeCaption`, `kHomeEntry[6]` (index = `PT_POKEDEX..PT_OPTIONS`), `kHomeRegistered`, `kHomeBike`, `kHomeRun`, `kHomeMessage`.
  - Battle: `kBatAction[4]` (FIGHT, BAG, POKéMON, RUN), `kBatBall` and `kBatBallArrow[2]`, `kBatSafari[4]`, `kBatParty` (the six balls), `kBatMove[4]`, `kBatMoveNote`, `kBatCancel`, `kBatTarget[4]`.
  - `#define LOWER_LAYOUT_PROVISIONAL 1` until Task 9 sets it to 0.
- Values: provisional ones chosen by the implementer that pass the invariants; Task 9 replaces them with the owner's handoff.

- [ ] **Step 1: Write `lower_layout_test.c`** (plain C, includes only the two headers): every rect inside 320x240; every button ≥ 24x24 and every row ≥ 16 tall; framed rects (`kHomeMapPanel`, `kHomeMessage`, `kBatMoveNote`) have `w` and `h` multiples of 8; `kHomeMapPanel` interior ≥ 192x(128 + 16); the HOME entries, X, Y and RUN do not overlap one another or the map panel; the battle rects of one state do not overlap. Prints `PASS lower layout`.
- [ ] **Step 2: Run to see it fail.** Expected: missing header.
- [ ] **Step 3: Write the header** with provisional values.
- [ ] **Step 4: Run** `tools/test_lower_kit.py`. Expected: `PASS lower kit` and `PASS lower layout`.
- [ ] **Step 5: Commit** — `feat: lower screen layout table with provisional values`.

### Task 5: HOME as a full-screen page with the map at 1:1

**Files:**
- Create: `3ds_port/src/3ds_home_page_impl.h`
- Modify: `3ds_port/src/3ds_poketouch.c` (`CtrPokeTouch_Draw` :1207, `CtrPokeTouch_Update` :1234, `MenuFrame` :897; include the new header after the kit)
- Modify: `3ds_port/include/3ds_poketouch.h` (declarations below)
- Modify: `3ds_port/src/3ds_bottom.c` (`ProcessPokeTouch` :3641, `HomeTap` :1491)
- Modify: `3ds_port/tests/poketouch_render_test.c`

**Interfaces:**
- Consumes: the kit (Tasks 2-3), the layout (Task 4), `CtrPokeTouch_MapArt`, `CtrPokeTouch_MapPlayerIcon`, `CtrPokeTouch_MapCursorIcon`, `ItemArt`'s icon cache, `CtrPokeTouch_ItemIcon`.
- Produces:
  - `uint8_t frameType;` in `CtrPokeTouchView` (0..9), filled in `SnapshotState` (`3ds_bottom.c` :1994) from `CtrPokeTouch_OptionGet(PT_OPT_FRAME)` for every field view (today `optFrameType` is only filled while OPTIONS is shown); the HOME page draws its std frames with it, and `CtrPokeTouch_Update` redraws them when it changes.
  - `bool CtrPokeTouch_HomePageShown(const CtrPokeTouchView *view)`: `view->home && !view->viewport` and no sub-view on (`bag.on`, `dex.on`, `card.on`, `learn.on`, `name.on`, `box.on`, `choice.on`, `options`, `save` all 0).
  - `int CtrPokeTouch_HomeHit(const CtrPokeTouchView *view, int x, int y)`: the `PT_*` id of the HOME entry, `PT_REGISTERED`, `PT_BIKE` or `PT_RUN` under (x, y), or -1; disabled entries still report their id (the caller already checks `FieldEnabled`).
  - `bool CtrPokeTouch_HomeMapCell(int x, int y, int *cx, int *cy)`: with `mx = kHomeMapPanel.x + 8`, `my = kHomeMapPanel.y + 8` (the map's top-left), the 1:1 crop of the 240x160 art at (24, 28) puts cell (cx, cy) at `(mx + 8 + 8 cx, my + 4 + 8 cy)`; so `cx = (x - mx - 8) / 8`, `cy = (y - my - 4) / 8` for `x - mx - 8 >= 0` and `y - my - 4 >= 0`; true only for `0 <= cx < 22`, `0 <= cy < 15`.
  - In `3ds_bottom.c`: `static void HomeTapCell(int cx, int cy)` holds `HomeTap`'s logic after its cell conversion; `HomeTap(gx, gy)` keeps converting the old 5/4 coordinates and calls it.
- Drawing: the page background (provisional: `KitInk(1)`), `kHomeMapPanel` as `KIT_FRAME_STD` with `view->frameType`, the map at 1:1 from the same `MapArt` data `MapBuild` uses (crop 192x128 at (24,28), no scaling; a 192x128 column cache of its own), the player and cursor markers 16x16 at `(mx + 4 + 8 cx, my + 8 cy)`, `KitCaption(&kHomeCaption, location, region)`, entries with `KitIconCell` (the six item icons the column uses today, labels as today, the Trainer Card labelled with the player's name), X/Y with the registered items' icons, RUN with the port's own shoe mark and an on/off text (no toggle switch), `kHomeMessage` as `KIT_FRAME_DIALOGUE` with `view->bar` lines.

- [ ] **Step 1: Update `poketouch_render_test.c`**: the exact column rectangles stay asserted for views other than HOME; add:
  - `home_hit_centres`: for every entry and X/Y/RUN, the rect centre gives its id through `CtrPokeTouch_HomeHit`.
  - `home_hit_gaps`: a point between two entries and a point on the map panel give -1.
  - `home_map_cell_bounds`: the centre of cell (0,0) and of (21,14) map back to themselves; a point left of cell 0, below row 14, and on `kHomeCaption` give false.
  - `home_map_1to1`: the HOME map interior pixel at (mx + 8, my + 4) equals `Gba(0x5AB3)` (the stub map colour); replaces the old viewport colour check.
  - `home_update_equals_draw`: HOME with selection, press, frame type, location and bar changes: `CtrPokeTouch_Update` from the previous view equals a fresh `CtrPokeTouch_Draw` (the file's existing equality helper).
  - `old_views_keep_old_frame`: with `bag.on`, `CtrPokeTouch_HitTest` still returns the column ids at their old rectangles.
- [ ] **Step 2: Run to see them fail.** Run: `python tools/test_poketouch_render.py` (with the environment). Expected: compile error on `CtrPokeTouch_HomeHit`.
- [ ] **Step 3: Implement** the page, the three functions and the `3ds_bottom.c` wiring: in `ProcessPokeTouch`, when `CtrPokeTouch_HomePageShown(&sShown.touch)`, use `CtrPokeTouch_HomeHit` for presses and releases and `CtrPokeTouch_HomeMapCell` + `HomeTapCell` for map taps; otherwise the old path.
- [ ] **Step 4: Run** the render, kit and battle runners, then `python tools/dev.py run`. Expected: all pass; in Azahar HOME shows the new page with the owner's save, Fly works by tapping a town twice, every entry opens its (old-style) view, BACK returns to the new HOME.
- [ ] **Step 5: Commit** — `feat: HOME as a full-screen page with FireRed's frames and the map at 1:1`.

### Task 6: Battle commands, quick ball and Safari

**Files:**
- Create: `3ds_port/src/3ds_battle_page_impl.h`
- Modify: `3ds_port/src/3ds_battle_ui_impl.h` (`BActRect` :38, `BBallArrow` :48, `sBSafari` :60, `BScreen` :1426, `CtrBattleUi_Hit` :129; include the page header before `BScreen`)
- Modify: `3ds_port/src/3ds_bottom.c` (`BattleFill` :1845: `showCursor` = 1 for `BUI_ACTION`, `BUI_SAFARI`, `BUI_MOVE`, `BUI_TARGET`)
- Modify: `3ds_port/tests/battle_ui_render_test.c`

**Interfaces:**
- Consumes: kit, layout (`kBatAction`, `kBatBall`, `kBatBallArrow`, `kBatSafari`, `kBatParty`), `BPartyIcon`, `BItemDraw`, `CtrBattleView` fields as today.
- Produces: `uint8_t frameType;` in `CtrBattleView`, filled in `BattleFill` from `CtrPokeTouch_OptionGet(PT_OPT_FRAME)`; `static void BPageAction(const CtrBattleView *v)`, `static void BPageSafari(const CtrBattleView *v)`; `BActRect`/`BBallArrow`/`sBSafari` return the layout's rects, so `CtrBattleUi_Hit` and drawing share them. The arrow is drawn on `v->pressed` while a touch is held, else on `v->cursor`.
- Drawing: each command is a `KitIconCell` inside a `KIT_FRAME_STD` panel: FIGHT with the active Pokémon's icon (`BPartyIcon`), name and level; BAG with the Bag art the battle Bag already loads (`graphics/interface/bag_male.4bpp.lz`); POKéMON with the Poké Ball item icon; RUN with the port's own shoe mark (FireRed has no icon for it). The quick ball cell shows its item icon, quantity and the two arrows; the party's six balls keep their fainted/status/empty marks from the existing data; Safari's four commands are the same cells. No ember backdrop on these states.

- [ ] **Step 1: Update `battle_ui_render_test.c`**: the backdrop check at (1,100) becomes the page background for `BUI_ACTION`/`BUI_SAFARI`; add:
  - `action_hit_centres`: the centre of every `kBatAction`, `kBatBall`, `kBatBallArrow`, `kBatSafari` rect gives its hit code.
  - `arrow_follows_cursor_and_press`: with `cursor = 2`, `pressed = -1` the arrow's ink is left of `kBatAction[2]`'s label; with `pressed = 1` it moves to `kBatAction[1]`.
  - `quick_ball_states`: `CTR_QUICK_BALL_NONE` draws no ball button, `READY`/`EMPTY`/`FULL` draw it (EMPTY toned).
  - The existing incremental-vs-full and legibility checks keep running for these states.
- [ ] **Step 2: Run to see them fail.** Run: `python tools/test_battle_ui.py`. Expected: FAIL on the new checks.
- [ ] **Step 3: Implement** the page functions, rect sources and `showCursor`.
- [ ] **Step 4: Run** all runners, then `python tools/dev.py run`: a wild battle (quick ball), a trainer battle, the Safari Zone if reachable in the owner's save. Expected: all pass; touch and D-pad pick the same commands.
- [ ] **Step 5: Commit** — `feat: battle commands, quick ball and Safari in the FireRed kit`.

### Task 7: Battle moves and the double-battle target

**Files:**
- Modify: `3ds_port/src/3ds_battle_page_impl.h`
- Modify: `3ds_port/src/3ds_battle_ui_impl.h` (`sBCard` :56, `sBNote` :57, `sBCancel` :58, `BScreen`)
- Modify: `3ds_port/tests/battle_ui_render_test.c`

**Interfaces:**
- Consumes: kit, `kBatMove`, `kBatMoveNote`, `kBatCancel`, `kBatTarget`; `CtrBattleMove` and `CtrBattleTarget` as today; `BDescribeN` for wrapping.
- Produces: `static void BPageMove(const CtrBattleView *v)`, `static void BPageTarget(const CtrBattleView *v)`; `sBCard`/`sBNote`/`sBCancel` replaced by the layout's rects for both drawing and hits.
- Drawing: each move as a framed list cell: name, `KitTypeLabel`, power, accuracy, PP (no-PP toned); the note as `KIT_FRAME_DIALOGUE` with the description, effectiveness, STAB, effect chance and the Physical/Special fallback text, each still governed by the existing OPTIONS rows; CANCEL as a `KitRow`; targets as framed cells with icon and name.

- [ ] **Step 1: Add tests**: `move_hit_centres` (each `kBatMove`, `kBatCancel`, `kBatTarget` centre gives its code), `no_pp_toned` (a move with 0 PP draws its PP digits with `Tone(KitInk(2), 0)`), `empty_move_slot` (an empty slot draws nothing but the page and is not hit), the arrow test of Task 6 for `BUI_MOVE` and `BUI_TARGET`.
- [ ] **Step 2: Run to see them fail.** Expected: FAIL on the new checks.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run** all runners, then `python tools/dev.py run` with a single and (if reachable) a double battle. Expected: all pass; move details as today.
- [ ] **Step 5: Commit** — `feat: battle moves and targets in the FireRed kit`.

### Task 8: No more embers

**Files:**
- Modify: `3ds_port/src/3ds_bottom.c` (`EmbersStep` :5337 and its calls at :5569, :5603)
- Modify: `3ds_port/src/3ds_poketouch.c` (`CtrPokeTouch_Embers` :1373), `3ds_port/include/3ds_poketouch.h` (its declaration)
- Modify: `3ds_port/src/3ds_poketouch_options.c` (EMBERS row :133, `CtrOptions_EmbersInit` :87-108), `3ds_port/include/3ds_poketouch_options.h`
- Modify: `3ds_port/src/main_game_full.c` (the `CtrOptions_EmbersInit` call and its log line)
- Modify: `3ds_port/tests/poketouch_options_test.c`, `3ds_port/tests/settings_test.c`

**Interfaces:**
- Produces: OPTIONS without the EMBERS row; `settings.cfg` keeps loading files that contain `embers=` (the key is ignored on read and no longer written). The ember backdrop tables stay for the views not yet reworked.

- [ ] **Step 1: Tests**: `poketouch_options_test.c`: the port page has no EMBERS row and its other rows keep their order; `settings_test.c`: a file with `embers=2` and `turbo=2` loads `turbo` as 2.
- [ ] **Step 2: Run to see them fail.** Run: `python tools/dev.py build` (syncs `build/dev`), then `MSYSTEM=MSYS /c/devkitPro/msys2/usr/bin/bash.exe -lc 'make -C /c/Users/csequ/projects/twin-embers/build/dev/3ds_port verify-host'`. Expected: FAIL on the EMBERS row.
- [ ] **Step 3: Implement** the removals.
- [ ] **Step 4: Run** all runners and `python tools/dev.py build`. Expected: pass; the 3DSX checks pass.
- [ ] **Step 5: Commit** — `refactor: drop the rising embers and their option`.

### Task 9: The owner's layouts

**Files:**
- Modify: `3ds_port/include/3ds_lower_layout.h` (values; `LOWER_LAYOUT_PROVISIONAL 0`)
- Modify: `3ds_port/src/3ds_home_page_impl.h`, `3ds_port/src/3ds_battle_page_impl.h` only where the handoff places an element differently (order, alignment, which kit piece)

**Interfaces:**
- Consumes: the owner's Claude Design handoff zip (README + `.dc.html`); the README is the specification, the prototype's runtime is not reused.

- [ ] **Step 1: Read the handoff** and list every element with its 1x rect; map each to a layout constant. An element outside the spec's inventory, or an inventory element missing from the handoff: stop and ask the owner before continuing.
- [ ] **Step 2: Snap** framed rects to multiples of 8 and buttons to ≥ 24x24; list each change of more than 2 px for the owner.
- [ ] **Step 3: Update the header and run** `tools/test_lower_kit.py`, the render and battle runners. Expected: all pass (the invariants hold for the owner's values).
- [ ] **Step 4: Show the owner** host-renderer captures of HOME, commands (wild, trainer, Safari), moves (single, double target) as PNGs, then `python tools/dev.py run`.
- [ ] **Step 5: Refine** with the owner's notes; repeat Steps 3-4 until they approve the style.
- [ ] **Step 6: Commit** — `feat: the owner's lower screen layouts`.

### Task 10: Verify on the console and open the PR

**Files:**
- Modify: `docs/review-manifest.json` (through the helper, per commit)

- [ ] **Step 1: Full checks**: all runners, `python tools/source_audit.py`, `python tools/dev.py build` (validate_3dsx and verify_m4a_arm11 pass).
- [ ] **Step 2: Console**: `python tools/dev.py install` (the owner backs up the SD card first), then the owner plays: HOME, Fly, every entry and BACK, a wild and a trainer battle, moves, quick ball.
- [ ] **Step 3: Push and open the PR into the fork** (base `main` if PR 1 is merged, else `feat/fork-setup`):

```bash
git push -u origin feat/lower-screen-rework
gh pr create --repo christiansmmc/twin-embers --base main --head feat/lower-screen-rework
```
