# Lower screen rework 2: the red device (style B), on v0.6.0

Date: 2026-11-14. Status: draft for the owner's approval.

Design canvas: https://claude.ai/artifact/9cx85k3qavedLkTyKEsAKm, rows "B — Aparelho vermelho" and "B — Telas restantes".
Local copies of the boards and one-line-per-element layout dumps: `.progress/design-b/` (git-ignored; regenerate with
the Artifact read of `project/B*.dc.html`).
Recon behind this document (git-ignored, `.progress/`): `lower-screen-rework2-recon-v060-ui.md` (map of v0.6.0's lower
screen), `lower-screen-rework2-recon-port.md` (what of our old PRs still applies), `lower-screen-rework2-recon-perf.md`
(why opening and closing views is slow).

## 1. Intent

The owner forked Twin Embers to take the 3DS lower screen's look in their own direction. Rework 1 (FireRed-style pages,
PRs #1 to #5) is kept, untouched, on `archive/lower-screen-pages-v1`. Rework 2 redraws every lower-screen view as the
canvas's style B, "the red device": the lower screen becomes a piece of red plastic hardware with screws, an embedded
screen with a dark bezel for whatever you read, and cream plastic keys that sink when chosen (with a green LED), dark
keys for leaving.

It is built on upstream's v0.6.0, taken once. After that the fork no longer follows upstream. The owner wants
upstream's features, fixes and speed work, but not its look.

Once the look is complete, the second goal is speed: opening and closing views (BAG, POKéMON, Summary...) feels slow
today.

## 2. Goals, non-goals, anti-goals

Goals:
1. `main` = upstream v0.6.0 plus what makes the fork ours: the `classicfirered3ds` name, the local dev loop, the
   touch-response fixes from PR #4 that still apply. Top screen exactly as v0.6.0, minus the turbo badge.
2. Every lower-screen view drawn in style B. No view keeps the ember look, and the ember drawing code is deleted.
3. Everything v0.6.0 does keeps working: every view, touch, D-pad and buttons, sessions, every OPTIONS row, EN/ES/DE
   text. One exception: jumping from inside a view straight to another through the START column (see 7.2).
4. Opening and closing views feels fast. Measured from the tap to the new page and from closing to the player having
   control again, lower than v0.6.0's own numbers, and confirmed by the owner on the console.

Non-goals: changing the top screen (beyond the turbo badge; the owner will handle the rest later); new gameplay
features; taking upstream's later releases; changing game logic beyond what the latency work needs.

Anti-goals (a failure even if the rest works):
- any view falling back to the ember look, or to a blank screen;
- touch or D-pad regressions (lost taps, a tap hitting the wrong thing, a page that does not follow the game's cursor);
- a lower screen that costs more per frame than v0.6.0's;
- breaking the clean-release audit (nothing derived from the ROM committed).

## 3. Constraints (measured in the recon unless marked)

- The lower screen is a CPU-drawn RGB565 canvas, 320x240, stored column-major bottom-up. All drawers live in one
  translation unit (`3ds_poketouch.c` includes every `*_impl.h`) and are pure functions of plain view structs filled by
  `3ds_bottom.c` (`CurrentMode` → `SnapshotState` → `Render` / `*_Update`). Routing, touch and the game bridges sit
  above the drawers and stay.
- The boards are 320x240 drawn at 2x. Coordinates in the layout dumps are already lower-screen pixels.
- The boards use Pixelify Sans as a stand-in. The game draws with FireRed's own font from the player's pack. Striped or
  dashed boxes in the boards are art from the pack (Pokémon and item icons, the region map, pictures).
- Source governance stays: 262,144 bytes per file including every past version of it (`tools/source_audit.py`), every
  file listed in `docs/review-manifest.json`, `-Werror`, no long hex runs. `3ds_video.c` has 3.3 KB left and
  `3ds_bottom.c` 9.6 KB, so new code goes in new files and `3ds_bottom.c` gets split first.
- Text is chosen at compile time with `CTR_TR(en, es, de)`.
- Tests that pin the ember look (`test_poketouch_render`, `test_battle_ui`, `test_column_labels`) get rewritten for B,
  keeping their invariants (a partial update must equal a full draw; hit rectangles match drawn rectangles).
- Game-side edit recipes continue from 0262 (upstream used up to 0261).
- Nacho's project has no licence: ideas only, no code.

## 4. Approaches (decided by the owner)

1. **Chosen:** v0.6.0 as the base, taken once; the fork owns the look and stops syncing. Gets every upstream feature,
   fix and per-frame speedup; frees us to delete the ember code and restructure. Cost: later upstream fixes would have to
   be brought by hand.
2. Port upstream's features one by one onto our v0.5 base. Rejected: upstream's history is braided (integration
   branches merging ~15 work branches), its features are tied to lower-screen views (Wilds and Nuzlocke in OPTIONS,
   REMEMBER, Easy Chat, mail), its speedups are in its drawing code, and it would repeat with every release.
3. Merge upstream into our PR stack. Rejected: 40 files in conflict, ending where option 1 starts, messier.

The owner also decided: the top screen stays v0.6.0's except the turbo badge, which goes.

## 5. Design

### 5.1 Base (first PR)

- Fast-forward `main` (local and `origin`) to upstream `0afd7a2` (v0.6.0). `main` has nothing upstream lacks.
- Re-apply the fork identity: cherry-pick `a808ae4` (rename to `classicfirered3ds`, `tools/dev.py`, `.dev.local.json`
  ignored); bring `tools/make_recipe.py` and `tools/manifest_update.py` (fix: v0.6.0's manifest ends in a newline);
  fix the stale `sdmc:/3ds/twinembers/debug.txt` comment (`3ds_game_bridge.c:921`); regenerate the manifest.
- Our crash fixes (recipes 0237/0238) are dropped: upstream's 0256/0259 are the same fixes.
- Remove the turbo badge from the top screen.

### 5.2 Touch-response fixes (second PR)

- Split `3ds_bottom.c`'s fill functions into `*_impl.h` headers included where they stood (about 35 KB can move), leaving
  in place the text ranges that tests cut out of it.
- Port PR #4: `6ea0499` (the Summary turns its pages on the press; drop the `SUI_HIT_PAGE_*` codes v0.6.0 lacks),
  `65b3274` (show a touched Summary page when FireRed takes it), `9201498` rewritten for v0.6.0's functions (a touched
  tab or Pokémon stays chosen), and the two game recipes regenerated as 0262 (the bridge reports the Bag's and the
  Party's destination) and 0263 (the Party's cards change when the switch's place is picked). Each with a test that fails
  before its fix.
- Port the SPEED-tap bridge (`CtrBottom_TakeTurboTap`), which B's HOME needs for its SPD key.

### 5.3 The B kit

A new drawing kit in its own file, included by `3ds_poketouch.c`. Its parts, with the boards' values:

| Part | Look |
|---|---|
| Shell | whole screen in the shell colour (default `#C23A2E`), 2 px light top edge, 3 px dark bottom edge; four 5x5 screws in the corners (on the boards that have room) |
| Embedded screen | bezel `#2A2A30` with a 1 px `#4A4A54` inner line, radius 6, 6 px to the paper; paper `#F8F8F4`; ink `#626262` with a `#D5D5CD` shadow; secondary ink `#747474` |
| Key | cream `#F2ECE0`, radius 5-6, 3 px drop shadow, `#D6CCB8` inner bottom edge, white top line; ink `#4A4A4A` with a white shadow |
| Chosen key | sunk: `#E6DECE`, 1 px drop, `#CFC4AE` inner top edge, drawn 2 px lower; green LED (`#58E070`, 4-5 px with glow) at its top right |
| Dark key | `#2A2A30`, ink `#F2ECE0`: CANCEL, BACK, CLOSE, ▲▼ in the Summary |
| List row, chosen | inverted like a display: `#2A2A30` behind `#F8F8F4` text |
| Empty party bay | a hole, black at 28% over the shell |
| Disabled | the group at half strength (the Bag's pocket keys under its item menu); a fainted Pokémon's key `#D2CAB8` |
| Bars and chips | HP bar (`#404850` frame, `#D8D8D0` track, green `#48C060` / yellow `#F0C830` / red `#E04030`), PP pips, type chips, status chips, effectiveness chips (SUPER `#289646`, NOT VERY `#CC5418`, EFFECTIVE `#646A84`) as on the boards |

The chosen key is the game's cursor. A touch sinks the key on the touch frame, before the game answers.

Shell colour: an OPTIONS row (looks section) with the canvas's four colours, red `#C23A2E` (default), blue `#3E4C6B`,
green `#2E7A4E`, grey `#5A5A64`, saved in `settings.cfg`.

### 5.4 The views with a board

| Board | v0.6.0 view(s) |
|---|---|
| B · Início | HOME (field). The embedded screen holds the region map window (192x128) with v0.6.0's map behaviour inside it (drag, zoom, region ribbon, FLY), the place name, the region, the party as 3x2 icons with HP bars, the RUN switch and the SPD key. Six keys below: POKéDEX, POKéMON, BAG, the player's name (Trainer Card), SAVE, OPTION. This replaces v0.6.0's START column. |
| B · Comandos | battle action menu: the embedded screen shows the active Pokémon (icon, name, gender, level, HP, party balls) and "What will X do?"; FIGHT is a wide key. Below, four keys about 73 px wide (owner's choice, the board had three): BAG, BALL, POKéMON, RUN. BALL is v0.6.0's quick ball (recipe 0228, the game's fifth cursor spot): the chosen ball's icon and count with ◀ ▶ to change it, as v0.6.0 does. |
| B · Golpes | battle moves: the embedded screen shows the chosen move's detail (type, category, power, accuracy, description); four move keys with type, effectiveness and PP; CANCEL dark key; "A USE · B BACK" hint |
| B · POKéMON | Party, in the field and in battle, including picking several (CONFIRM): six bays, an empty one is a hole; the message on an embedded screen; CANCEL dark key |
| B · POKéMON: menu de ações | the party's action menu: the party as a column of slim bays, the actions as keys beside it; field moves (CUT, FLY...) with their name in blue. The keys follow the game's own grid (owner's choice): up to four actions, one column as on the board; five or more (v0.6.0 allows up to eight: HEAL, REMEMBER, field moves), two columns of keys, with the party column narrowed to make room. The D-pad stays as v0.6.0 has it (`3ds_party_grid.h`, recipe 0257). |
| B · Resumo: INFO / STATS / detalhes do golpe | Summary: page keys INFO, STATS, MOVES, MEMO on top, ▲▼ dark keys for the previous and next Pokémon; the page in the middle; the ability or move description below; STATS has its IV/EV switch on the screen; BACK dark key |
| B · BAG / BAG: menu do item | field Bag, TM Case and Berry Pouch (their own pocket names on the keys), and the battle Bag. Item list on the embedded screen with the chosen row inverted, ▲▼ tall keys, description below, CLOSE dark key. Item menu: the list dims and USE, GIVE, TOSS, REGISTER, CANCEL rise as keys on the right. Shop BUY, Item PC and Mailbox use the same page with a title in place of the pocket keys. The battle Bag shows only items in v0.6.0 (recipe 0261), with a BERRIES button for the Berry Pouch: its pocket keys are ITEMS and BERRIES. |

### 5.5 The views without a board

They follow the canvas's rule (legend "B — Telas restantes"): what you choose is a key, what you read is on the
embedded screen, what leaves is a dark key; the chosen key is sunk with its LED; lists invert the chosen row; ▲▼ keys
page or scroll.

The views: OPTIONS (sections as keys across the top like the Bag's pockets, rows on the embedded screen, ◀ ▶ to change a
value, BACK), SAVE, Trainer Card (the card on the embedded screen, a FLIP key), Pokédex (menu, list, category, page),
PC box, field choices and YES/NO, forget a move, REMEMBER, mail, Easy Chat, the naming keyboard (the keyboard as keys),
the Rare Candy level-up card, battle target and Safari menus, the idle screen (the shell with the logo on its embedded
screen), and the frame round FireRed's own picture (the picture inside an embedded screen; its position, hard-coded at
(4,4) in three places, is moved to one constant first).

Each one, once built, gets a board on the canvas in a new row, "B — como ficou", for the owner to review after the fact;
changes the owner asks for come back as follow-up work. These boards are generated from the drawn layout's rectangles,
with stand-ins for text and art like the existing boards, as rework 1 did: no pixel from the player's pack leaves the
machine.

### 5.6 Removing the ember look

Delete the ember shell, skin, START column, backdrop, the embers animation and its OPTIONS row, and the generated
assets and tools that only they use. The tests that pinned them are rewritten for B.

### 5.7 Speed

The recon found the slowness is mostly waiting, not drawing: on open, the lower screen holds until FireRed's invisible
fade-in and window animation end and then 8 more frames (`ROUTE_STABLE`); on close, about 0.6-0.7 s pass at normal
speed with both screens frozen; nothing changes on screen while it waits; taps made while walking are dropped.

1. Measure first. A probe stamps, from the tap: each game callback change, fade edges, readiness, the first lower-screen
   redraw, the present, the top's release and the player's control returning, in game frames, shown frames and ms, into
   `port.log`. Baseline in Azahar, and by the owner on the console.
2. Then, in order, measuring each: show the page as soon as the game's menu runs (keep input gated separately, drop
   `ROUTE_STABLE`); run the closing phase at the hidden fast speed or release the top screen earlier; queue a tap made
   while walking until the player is free; give feedback on the tap frame (B's sinking key does this); skip frames at
   normal speed when behind.
3. Re-measure; the owner confirms on the console.

### 5.8 Branches and PRs

Stacked like rework 1: `main` ← base (5.1) ← touch fixes (5.2) ← B in three or four PRs (kit with HOME and battle;
Party, Summary and Bag; the views without a board; ember removal) ← speed (5.7). The old PRs #1 to #5 get closed as
superseded once the new ones are up, after asking the owner.

### 5.9 Verification, every step

`tools/dev.py` build (`-Werror`), `make verify-host`, the Python suites, the source audit (cap, history, manifest), a
host render of each view with the real pack to PNG, compared side by side with its board, and a run in Azahar. The
owner tests on the console at the end of 5.4, 5.6 and 5.7.

## 6. Left to the builder

- Exact pixel placement where FireRed's font metrics differ from the stand-in font, and how long ES/DE text fits.
- Which pack art fills each placeholder, and the icons on HOME's keys.
- Where the map's zoom, region and FLY controls sit inside HOME's map window.
- How files are split to stay under the cap.

## 7. Calls made without the owner (check these)

1. The 15 or so views without a board are designed from the rules and reviewed afterwards on the canvas, not before.
2. The START column goes everywhere; HOME's six keys replace it. The on-screen X/Y registered-item tiles go too (the X
   and Y buttons keep working). With the column goes the jump from inside a view straight to another one (v0.6.0's
   `ProcessRouteTouch` → `RequestDestination`, which closes the current screen with B presses and opens the new one):
   every board shows only a dark key to leave, and rework 1 had dropped it too. To go from the Bag to the Pokédex, you
   leave the Bag and tap POKéDEX on HOME. The route code is deleted.
3. The embers animation and its OPTIONS row go: there is no backdrop for them on the plastic shell.
4. The shell colour becomes an OPTIONS row with the canvas's four colours.
5. HOME's map keeps v0.6.0's drag, zoom, region ribbon and FLY, inside the 192x128 window.
6. The battle Bag's pocket keys are ITEMS and BERRIES.
7. The "como ficou" boards are made from the layout's rectangles with stand-ins, not from screenshots with the pack's
   art.
