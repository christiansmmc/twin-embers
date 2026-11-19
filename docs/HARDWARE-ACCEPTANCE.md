# Real Nintendo 3DS acceptance checklist

For the owner, on real hardware, with the exact candidate `twinembers.3dsx` (record
its SHA-256) and a pack built by the Builder from your own supported ROM. Keep a
copy of every save you use. Record the model (Old/New 3DS, 2DS), system version and
Homebrew Launcher entry point. Mark each line PASS or FAIL with a short note; a
FAIL should include the screen text or `/3ds/twinembers/port.log` lines.

## Setup

- [ ] `/3ds/twinembers/` holds only `twinembers.3dsx` and `twinembers.pak`.
- [ ] Builder printed ABI `8f71cf3a` and `verify-pak` passed.
- [ ] `/3ds/dspfirm.cdc` exists (made once by DSP1); without it the game runs silently.

## Starting and saving

- [ ] New game (no save anywhere): intro, title, Oak speech and naming screen run.
- [ ] First save of that new game asks "Save your game?"
      (no "different game file" warning on a blank card).
- [ ] Save in the field, power off, restart: Continue resumes at the saved spot.
- [ ] Existing save: copy a known FireRed save to `/3ds/pokefirered/firered.sav`
      (no `twinembers.sav`); the game copies it once and shows your progress; the
      original file is unchanged.
- [ ] With both `/3ds/twinembers/twinembers.sav` and an old save present, the
      Twin Embers save is used.
- [ ] SAVE screen reports success only after writing; a write-protected card
      shows a save error instead.
- [ ] After a start with a save, `/3ds/twinembers/twinembers.sav.bak` exists and is
      131072 bytes; no `.bak.tmp` is left behind.
- [ ] With the card's write-protect switch on, starting the game shows the save error
      on the lower screen until A (the app does not just close).

## Interface

- [ ] PokéTouch lower screen: Party, Bag, Summary, Pokédex, Trainer Card, Town
      Map, OPTIONS and SAVE open, respond to touch and close cleanly.
- [ ] OPTIONS shows seven rows; every change survives a save and restart.
- [ ] PC storage: move, withdraw and deposit Pokémon by touch.

## The lower screen (the red device)

Touch each view on the console and compare with the Azahar run. Rows follow the design canvas.

- [ ] HOME: every key, RUN on and off (the light), SPD 1x-4x, the play time ticking a minute, the party cells.
- [ ] Party: the field's Party, a battle's switch, a forced send-out, a Rare Candy's level up, an item's target.
- [ ] Party actions: a Pokémon with CUT and FLY. The D-pad walks the column with wrap, the window follows to the 7th action and to CANCEL, CANCEL works by the exit key, SWITCH's YES / NO.
- [ ] Summary: each tab, ▲▼ through the party (grey in the move details), a move's details and CANCEL, a swap, the IV/EV switch, an egg, BACK, a battle's Summary. CONTACT: SCRATCH says YES; a status move and FLAMETHROWER say NO.
- [ ] Bag: the three pockets, the TM Case, the Berry Pouch, a mart's BUY, the item PC's WITHDRAW ITEM, the mailbox, the battle's Bag. Tap rows, ▲▼, drag (one item per 26 px), CLOSE at the end.
- [ ] Bag menu: USE / GIVE / TOSS (how many, YES / NO) / REGISTER. CANCEL by the exit key and by B. The Berry Pouch's CHECK TAG. The battle's USE.
- [ ] Battle commands: a wild battle (the quick ball, its arrows and swipe), a trainer battle, RUN blocked.
- [ ] Moves: FIGHT and each move (the footer follows the cursor), BACK, a double battle's target, the Safari Zone.
- [ ] Choices: a nurse's YES / NO, the PC's menu (4 + 1), the Viridian school's grid, BACK on each. Prices: the Celadon roof's drinks, a Game Corner prize counter, the coin seller, the Cerulean bike shop (its price shows).
- [ ] Forget a move: a Rare Candy at four moves. Tap an old move twice (forgotten), the new one twice (skipped), an HM (refused), BACK.
- [ ] Relearner (Two Island): a row, ▲▼, a move's footer, YES / NO, CANCEL with one tap.
- [ ] OPTIONS: ▲▼ through the six pages (the tabs never rename), the page line and dots on GRAPHICS' three pages, FRAME's sample, a look's panel. COMPANION and EXP SHARE: a tap on each word, between two words, at the row's right end (EN/DE COMPANION is drawn small).
- [ ] SAVE from HOME: YES / NO by touch and by ← → ↑ ↓ (no wrap), B; a save (the writing screen, then "RED SAVED THE GAME." and OK); an overwrite question starts on NO and → and ↓ keep NO; after a failed overwrite it reopens on NO.
- [ ] Trainer Card: the stars' colour, FLIP by the key and by a tap on the card, BACK.
- [ ] Pokédex: the three tabs (▲▼ grey on LISTS and SEARCH), HABITATS' pages, a habitat's page with ◀ ▶, CLOSE; KANTO's list (top, bottom, tap twice, ▲▼, a drag); a page's AREA, CRY, BACK; a new catch (REGISTERED, OK). ES/DE ROM: height and weight in m and kg; is the decimal separator a comma ("1,0 m")? Today it prints "1.0 m".
- [ ] Letter: read one from the Bag's mail. CLOSE, a tap on the screen, a tap on the note.
- [ ] FireRed's picture and idle: the Fame Checker and a Pokédex AREA map (BACK); the idle screen after a while.
- [ ] Naming: the player's name at a new game (or a Pokémon's nickname with its icon), the three pages, DELETE, OK, the counter.
- [ ] PC box: move a Pokémon across boxes (IN HAND), PARTY POKéMON, CLOSE, MOVE ITEMS, the chooser. The cursor into the Party (rows, CANCEL in the corner), the chooser (JUMP), a message (a tap anywhere).
- [ ] PC menus: a Pokémon's menu (CANCEL by the corner and by B), the box title's menu, WALLPAPER's scenery menus (the corner sends B), MARK (CANCEL), RELEASE's YES / NO and BACK.
- [ ] Easy Chat: a profile's phrase (CLEAR with its question, CONFIRM, CANCEL by touch and by B, the D-pad on the footer); a group's words (rows, the rail, ▲▼ at both ends, USE WORD, A TO Z, BACK); BY GROUP (the window by ▲▼ and by the D-pad to the last groups, a group, BACK; A TO Z's letters and BACK).

## Gameplay

- [ ] Several wild and trainer battles: menus, moves, switching, Pokémon cries.
- [ ] Fly to two towns; Surf across water; Flash inside Rock Tunnel.
- [ ] Enter and leave buildings, caves and route connections without glitches.
- [ ] Map edges at Pallet Town, Route 1 and a coastal route show the game's own border
      (no fill), with no holes or flicker.

## Overworld

The visible wild Pokémon, the day and night tint and the companion are implemented. These are the checks not yet recorded as
done (host tests do not count here).

- [ ] Old 3DS with 2 visible wilds: frame time and the `wilds:` summary lines in `port.log`.
- [ ] A crowded map (Celadon, Saffron) with 6 visible wilds on a New 3DS: every NPC appears, no wild takes the last free object events.
- [ ] Save with a visible wild on screen, reload in the same session and from a cold start: no leftover object, no blocked tile.
- [ ] Edge cases: caves, Viridian Forest, Pokémon Tower silhouettes, Safari Zone and Randomlocke (step encounters stay or come back).
- [ ] Day and night outdoors at night with the clock set, entering a house, a battle and a door fade, on a console.
- [ ] The companion follows through doors, Surf and the bike, on a console.

## Audio and performance

- [ ] Music and effects sound clean and stay in sync for 15+ minutes.
- [ ] FPS counter stays at about 60 in towns, routes and battles; no audio dropouts.
- [ ] Turbo speeds play up and returns to normal speed cleanly.
- [ ] Close the lid during play and reopen: game, audio and touch resume.

## Data pack errors

- [ ] Rename `twinembers.pak`: the missing-pack screen is readable (separated,
      whole-word lines) and A exits.
- [ ] Use a pack from a different release or a damaged copy: the mismatch screen
      appears and A exits; no save is touched.

Hardware results are evidence only when recorded with the candidate SHA-256. They do
not change `binary_release_approved` or the separate rights review.
