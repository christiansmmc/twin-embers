# Rework 2, plan 1: v0.6.0 base and touch fixes — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `main` becomes upstream v0.6.0, the fork's identity and dev loop are back on it, the top screen loses the turbo badge, and the five touch-response fixes of the old PR #4 work on v0.6.0's lower screen.

**Architecture:** Two stacked branches. `chore/v060-fork-setup` (base `main`): cherry-picks and small tool ports, no behaviour change except the badge. `fix/touch-response-v060` (base the first): first splits `3ds_bottom.c` (cap), then ports each fix as its own commit with its own failing-first test. The two game-side fixes are edit recipes regenerated against v0.6.0's bootstrapped tree.

**Tech Stack:** C11 (devkitARM for the 3DS, host gcc for tests, `-Werror`), Python 3 tooling under `tools/`, edit recipes in `patches/pokefirered/*.json`.

**Spec:** `docs/superpowers/specs/2026-11-14-lower-screen-rework2-design.md` (sections 5.1 and 5.2).
Recon this plan relies on (git-ignored): `.progress/lower-screen-rework2-recon-port.md` (sections 3.2, 3.3, 4.1, 5, 6).

## Global Constraints

- Commit messages in English, Conventional Commits, ending with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Every source file ≤ 262,144 bytes in every commit (`tools/source_audit.py` LIMIT, checked with `--history`).
- Every committed file listed in `docs/review-manifest.json` with its current hash: run `python tools/manifest_update.py` before each commit that adds or changes files.
- Nothing derived from the ROM is committed. `.progress/` is never committed (globally ignored; never `git add -f`).
- Game edit recipes are never cherry-picked across bases: regenerate them with `tools/make_recipe.py` against a v0.6.0 `build/dev`, numbered from 0262.
- Host tests use `$CC` or `gcc` (`/c/devkitPro/msys2/usr/bin/gcc` on this machine).
- The fixes keep upstream's rule that the Party's `p->button` is set only when picking several Pokémon (`3ds_bottom.c` ~:1909 in v0.6.0).

## Review Focus

1. A tab or Pokémon touched while the game is still sliding/animating: the touched target stays shown until the game arrives, then follows the game (no flash back to the old cursor) — pinned in Task 8's test.
2. The Summary's prev/next and RIGHT/LEFT page turns acting on the press while another key is held: physical keys still win over a touch plan — pinned in Task 5's test.
3. A first build on a clean `build/dev` after the base change: the bootstrap applies all recipes 0001..0261 then 0262/0263 in order and their before-hashes match — checked in Tasks 2, 7 and 9.
4. Old save and settings paths (`sdmc:/3ds/twinembers/...`) still migrate after the rename on v0.6.0 — checked in Task 2 (`data_runtime_test`, `test_data_runtime.py`).
5. A Party SWITCH in battle (not only in the field): the cards change at the pick in both — pinned in Task 9's test, and in the owner's check (Task 10).

---

### Task 1: Base branch

**Files:**
- Create (commit): `docs/superpowers/specs/2026-11-14-lower-screen-rework2-design.md`, `docs/superpowers/plans/2026-11-14-rework2-01-base-and-touch-fixes.md` (both already written, untracked in the working tree)

- [ ] **Step 1: Fast-forward main and push it**

```bash
git checkout main && git merge --ff-only upstream/main && git push origin main
```
Expected: `main` at `0afd7a2`; push succeeds (fast-forward).

- [ ] **Step 2: Create the branch** — `git checkout -b chore/v060-fork-setup`. The design and this plan (untracked) are committed in Task 2, once `tools/manifest_update.py` exists to give them manifest rows.

### Task 2: Fork identity and dev tooling

**Files:**
- Cherry-pick: `a808ae4` (rename to `classicfirered3ds`, `tools/dev.py`, `/.dev.local.json` in `.gitignore`, test fixture)
- Create: `tools/make_recipe.py` (from `4ec46f5`), `tools/manifest_update.py` (from `317b879`)
- Modify: `3ds_port/src/3ds_game_bridge.c:921` (comment `sdmc:/3ds/twinembers/debug.txt` → the `CTR_APP_DIR` path the rest of the file uses), `docs/review-manifest.json`

- [ ] **Step 1: Cherry-pick and resolve**

```bash
git cherry-pick a808ae4
# conflict only in docs/review-manifest.json: take v0.6.0's
git checkout --ours docs/review-manifest.json   # "ours" = the branch (v0.6.0) side during a cherry-pick
git checkout 4ec46f5 -- tools/make_recipe.py
git checkout 317b879 -- tools/manifest_update.py
```

- [ ] **Step 2: Make `manifest_update.py` keep v0.6.0's trailing newline**

v0.6.0's `docs/review-manifest.json` ends with `\n`; ours did not. The tool must write the file ending with exactly one `\n` (check: regenerate on an unchanged tree → `git diff docs/review-manifest.json` is empty except for the new rows).

- [ ] **Step 3: Fix the stale comment at `3ds_game_bridge.c:921`**, then `grep -rn "3ds/twinembers" 3ds_port tools` — expected: only the deliberate migration/old-path spots the rename kept (compare with `git grep -n "3ds/twinembers" a808ae4 -- 3ds_port tools`).

- [ ] **Step 4: Regenerate the manifest, run the audit**

Run: `python tools/manifest_update.py && python tools/source_audit.py --history`
Expected: exit 0.

- [ ] **Step 5: Fresh build and host tests**

```bash
rm -rf build/dev && python tools/dev.py build
python tools/bootstrap.py --dir build/host-tests && make -C build/host-tests/3ds_port verify-host
cd tools && python -m unittest test_data_runtime -v
```
Expected: the build ends with `classicfirered3ds.3dsx` validated (validate_3dsx + verify_m4a_arm11 pass); verify-host all pass (incl. `data_error_text_test`); `test_data_runtime` OK.

- [ ] **Step 6: Smoke run** — `python tools/dev.py run` starts Azahar with the existing pack (`build/pak/classicfirered3ds.pak`); the title screen shows and a save loads. If the pack is refused, rebuild it with `python tools/dev.py pak` (engine ABI is expected unchanged, 0x8f71cf3a).

- [ ] **Step 7: Commit** (amend the cherry-pick's resolution into it; the tools and the comment in a second commit)

```bash
git add tools/make_recipe.py tools/manifest_update.py 3ds_port/src/3ds_game_bridge.c docs/review-manifest.json docs/superpowers/specs/2026-11-14-lower-screen-rework2-design.md docs/superpowers/plans/2026-11-14-rework2-01-base-and-touch-fixes.md
git commit -m "chore: recipe and manifest tools, rework 2 design and plan"
```

### Task 3: Drop the top screen's turbo badge

**Files:**
- Modify: `3ds_port/src/3ds_video.c` (the `>>N` badge and the FPS counter's step-aside for it), `README.md` (the line about the speed badge)
- Reference: `88c4aa5` did this on the old base (`git show 88c4aa5`)

- [ ] **Step 1: Try `git cherry-pick 88c4aa5`**; on conflict, redo it by hand on v0.6.0's `3ds_video.c`: remove the badge drawing and its state, the FPS counter keeps its normal place, speed changes still go to the log.
- [ ] **Step 2: Build and audit** — `python tools/dev.py build && python tools/manifest_update.py && python tools/source_audit.py --history` → exit 0; `3ds_video.c` smaller than before.
- [ ] **Step 3: Check in Azahar** — `python tools/dev.py run`, press START in the field to cycle the turbo: no `>>N` in the top screen's corner; the FPS counter (if enabled in OPTIONS) stays at its place.
- [ ] **Step 4: Commit** `feat: drop the top screen's turbo badge`.
- [ ] **Step 5: Push and open the PR** (base `main`), title `chore: fork setup on v0.6.0`, body: what was ported, what was dropped (our 0237/0238 = upstream 0256/0259), the badge, the test results.

### Task 4: Split `3ds_bottom.c`'s fills out (branch `fix/touch-response-v060`)

**Files:**
- Create: `3ds_port/src/3ds_bottom_battle_fill_impl.h` (`BattleActiveSlot`, `BattleCommon`, `BattleBagFill`, `BatCopy`, `PartyIconFrame`, `PartyTag`, `BattlePartyFill`, `BattleFill`, `SaveFill`), `3ds_port/src/3ds_bottom_field_fill_impl.h` (`BagBar`, `FieldBagNative`, `ShopNoActions`, `ShopNoActionName`, `FieldListFor`, `FieldListName`, `FieldBagFill`, `FieldCardFill`, `FieldLearnFill`, `LearnSnapshot`, `ChoiceSnapshot`, `FieldNameFill`, `BoxSnapshot`, `ShopSnapshot`, `NameSnapshot`, `FieldDexMon`, `FieldDexDetail`, `FieldDexFill`, `TMCaseBar`, `PouchBar`)
- Modify: `3ds_port/src/3ds_bottom.c` (each moved run replaced by one `#include` at the place it stood)
- Must stay in `3ds_bottom.c` (tests cut them out by text): `StoreDirKey`..`ProcessTouch`, `enum { MODE_OFF`, `enum { ST_PLAN_NONE`..`BeginSessionPending`, `SnapshotState`'s opening `memset`..`if (mode == MODE_PARTY_CHILD`, `StorageBridge`, `UpdateStorageOwner`, `StorageHoldTop`, `CurrentMode`, `BeginSession`, `UpdateSession`, `CtrBattleAction_Next`, `CtrBottom_FilterGameKeys`..`/* The naming keyboard`, `BattleBagVisiblePocketStep`, `sBatBag.pocketTarget = 1;`.

- [ ] **Step 1: Branch** — `git checkout -b fix/touch-response-v060 chore/v060-fork-setup`.
- [ ] **Step 2: Move the functions** (contiguous runs only; a run that contains a must-stay anchor is split around it).
- [ ] **Step 3: Byte-identical check** — a scratch script substitutes each `#include "3ds_bottom_*_fill_impl.h"` line with the header's body; the result must equal `git show HEAD:3ds_port/src/3ds_bottom.c` byte for byte.
- [ ] **Step 4: Build and tests**

```bash
python tools/dev.py build && make -C build/host-tests/3ds_port verify-host
cd tools && python -m unittest test_storage_controller test_field_presentation test_bag_pockets test_first_battle test_poketouch_render -v
```
Expected: all pass; `3ds_bottom.c` ≤ ~218 KB.
- [ ] **Step 5: Commit** `refactor: the bottom screen's fills in their own headers` (manifest updated, audit exit 0).

### Task 5: The Summary turns its pages on the press (port of `6ea0499`)

**Files:**
- Modify: `3ds_port/src/3ds_bottom.c` (`RunSumPlan` ~:2843, `ProcessSummaryTouch` ~:2976)
- Test: `tools/test_field_presentation.py` — add `test_summary_turns_on_press`

- [ ] **Step 1: Write the failing test** — port `test_summary_turns_on_press` from `git show archive/lower-screen-pages-v1:tools/test_field_presentation.py`, dropping every `SUI_HIT_PAGE_PREV`/`SUI_HIT_PAGE_NEXT` assertion (v0.6.0 has only `SUI_HIT_PREV/NEXT` and the tab hits). It asserts: prev/next and tab hits start their plan on the touch-down; RIGHT on MOVES → TRAINER and LEFT back act on the key press, not the release; `RunSumPlan` cancels only on a newly pressed physical key, not on any held one.
- [ ] **Step 2: Run it** — `cd tools && python -m unittest test_field_presentation.FieldPresentation.test_summary_turns_on_press -v` (use the test class name the file defines) → FAIL.
- [ ] **Step 3: Implement** — `git cherry-pick -n 6ea0499`, keep its `3ds_bottom.c` changes, remove the `SUI_HIT_PAGE_*` references, extend the press rule to the tab hits (`SUI_HIT_TAB + n`).
- [ ] **Step 4: Run it** → PASS; `python tools/dev.py build` OK.
- [ ] **Step 5: Commit** `fix: the Summary turns its pages on the press`.

### Task 6: Show a touched Summary page when FireRed takes it (port of `65b3274`)

**Files:**
- Modify: `3ds_port/src/3ds_bottom.c` (`NavPending` ~:5568: condition becomes `CtrBottomSummary_Page() != sSumPlan.target`; `SummaryFill` ~:2918: `v->data.ready = 0;`)
- Test: `tools/test_field_presentation.py` — add `test_summary_page_shown_when_taken` and its readiness assert, from the archive branch.

- [ ] **Step 1: Port the test** → **Step 2: run → FAIL** → **Step 3: `git cherry-pick -n 65b3274`** (auto-merges) → **Step 4: run → PASS, build OK** → **Step 5: commit** `fix: show a touched Summary page when FireRed takes it`.

### Task 7: The bridge reports the Bag's and the Party's destination (recipe 0262, redo of `f2418c4`)

**Files:**
- Create: `patches/pokefirered/0262-lower-screen-shows-destination.json`
- Modify: `tools/test_field_presentation.py` (`test_bridge_reports_the_destination`, recipe file name `0262-lower-screen-shows-destination.json`), `docs/review-manifest.json`
- Game files the recipe edits (in `build/dev`): `src/item_menu.c` (`CtrBottomBag_Count`, `MaxShowed`, `Cursor`, `Serial`, `Item`, `PocketName`, `Pocket`, `Quantity` report the pocket being switched to), `src/party_menu.c` (`CtrBottomParty_ActiveSlot` reports the switch's picked place during `PARTY_ACTION_SWITCHING`)

- [ ] **Step 1: Port the test** from the archive branch with the new file name → run → FAIL (file missing).
- [ ] **Step 2: Make the edited copies** — copy `build/dev/src/item_menu.c` and `build/dev/src/party_menu.c` to `build/work/src/`, apply the same edits `f2418c4`'s recipe 0239 makes (read them with `git show f2418c4:patches/pokefirered/0239-lower-screen-shows-destination.json` and the old bootstrapped functions).
- [ ] **Step 3: Write the recipe**

```bash
python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/0262-lower-screen-shows-destination.json src/item_menu.c src/party_menu.c
```
Expected: the tool applies it and confirms it reproduces the edit.
- [ ] **Step 4: Rebuild from scratch and test** — `rm -rf build/dev && python tools/dev.py build`; the test → PASS; manifest + audit exit 0.
- [ ] **Step 5: Commit** `fix: the bridge reports the Bag's and the Party's destination`.

### Task 8: A touched tab or Pokémon stays chosen until the game gets there (rewrite of `9201498`)

**Files:**
- Modify: `3ds_port/src/3ds_bottom.c` (touch state: `sPartyEcho`, `PartyEchoCursor()`, `FieldBagHold()`; `ProcessBattlePartyTouch` ~:4303), `3ds_port/src/3ds_bottom_battle_fill_impl.h` (`BattlePartyFill`: `p->cursor` from `PartyEchoCursor()`), `3ds_port/src/3ds_bottom_field_fill_impl.h` (`FieldBagFill`: the BAG_BUSY hold goes through `FieldBagHold()`), `3ds_port/include/3ds_poketouch.h` (`uint8_t asked;` in `CtrPokeTouchBag`: the pocket a touch asked for, 0xFF none)
- Test: `tools/test_field_presentation.py` — `test_touch_shows_where_it_goes` from the archive branch

**Interfaces:**
- Produces: `static int PartyEchoCursor(int gameCursor)` → the touched slot while `CtrBottomParty_TouchSlot` is pending, else `gameCursor`; `static int FieldBagHold(const CtrPokeTouchBag *last)` → 1 while the picture must hold (the pocket switch in flight shows the pocket 0262 reports). `CtrPokeTouchBag.asked` is read by plan 2's B Bag to sink the asked pocket key; the ember drawer ignores it.

- [ ] **Step 1: Port the test** → run → FAIL.
- [ ] **Step 2: Implement** as in `9201498` (read it with `git show 9201498`), against v0.6.0's functions; keep upstream's MULTI-only `p->button`.
- [ ] **Step 3: Run** → PASS; build; `cd tools && python -m unittest test_poketouch_render test_battle_ui -v` still pass (the Update == Draw invariant).
- [ ] **Step 4: Commit** `fix: a touched tab or Pokemon stays chosen until the game gets there`.

### Task 9: The Party's cards change when the switch's place is picked (recipe 0263, redo of `3264f20`)

**Files:**
- Create: `patches/pokefirered/0263-party-switch-shows-new-places.json` (game files: `src/party_menu.c` adds `CtrBottomParty_ShownMon`, chained on 0262's output; `src/ctr_bottom_party.c`'s ten getters read through it)
- Modify: `tools/test_field_presentation.py` (`test_party_switch_shows_the_new_places`, file name `0263-party-switch-shows-new-places.json`), `docs/review-manifest.json`

- [ ] **Step 1: Port the test** → FAIL. **Step 2:** edited copies in `build/work` from a `build/dev` that already has 0262, same edits as `3264f20`'s 0240. **Step 3:** `python tools/make_recipe.py --before build/dev --after build/work --out patches/pokefirered/0263-party-switch-shows-new-places.json src/party_menu.c src/ctr_bottom_party.c`. **Step 4:** fresh `build/dev` build, test PASS, manifest + audit exit 0. **Step 5:** commit `fix: the Party's cards change when the switch's place is picked`.

### Task 10: Full check and PR

- [ ] **Step 1: Full suite**

```bash
python tools/source_audit.py --history
python tools/bootstrap.py --dir build/host-tests && make -C build/host-tests/3ds_port verify-host
cd tools && python -m unittest test_poketouch_render test_battle_ui test_column_labels test_option_labels test_storage_touch test_storage_controller test_easychat_plan test_field_presentation test_bag_pockets test_first_battle test_data_runtime -v
```
Expected: all OK.
- [ ] **Step 2: Copy to Azahar** — `python tools/dev.py run`.
- [ ] **Step 3: Push and open the PR** (base `chore/v060-fork-setup`), title `fix: lower screen touch response on v0.6.0`, body listing each fix, its test, and the owner's check list: the Summary's page turns and TRAINER by touch and by RIGHT/LEFT; the field Bag's tabs; a Party SWITCH in the field and in battle.

## Not in this plan

- The SPEED-tap bridge (`CtrBottom_TakeTurboTap`, spec 5.2): it only has a caller once B's HOME has its SPD key, so it moves to plan 2's HOME task (no dead code in between).
- Closing the old PRs #1–#5: after both new PRs are up, ask the owner.
