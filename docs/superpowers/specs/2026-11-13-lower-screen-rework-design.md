# Lower screen rework: an extension of FireRed

Status: draft for the owner's approval (2026-11-13). Fork: classicfirered3ds.

## Intent

The lower screen (PokéTouch, the battle screen and the menus that open on it) does not look like the game on the
upper screen. Today it feels like a second game running beside FireRed: a dark red backdrop with rising embers,
glossy raised buttons with gradients and large rounded corners, cream and amber cards, a separate 5x7 button font.
The upper screen is FireRed's pixel art at 1:1, soft colours and the game's own framed text box.

The owner wants the lower screen to read as **an extension of FireRed**, with the simpler, roomier structure of the
NachoGonzalez2001 Pokemon-FireRed-3DS port (light panels, the whole 320x240 used, nothing cramped), and the spirit
of the DS Pokémon games, but built from the game's own materials rather than a modern app look.

## Goals

- Every lower-screen view looks like it belongs to FireRed: the game's window frame, font, selection arrow, colours
  and icons, at the game's pixel scale.
- Roomier layouts that use the full 320x240, instead of a 240x160 viewport plus a side column plus a bar.
- **Every view is a full-screen page** (owner's choice, "a more original Pokémon experience"): HOME holds the menu,
  each view opened from it takes the whole 320x240, and BACK (or B) returns to HOME. Switching straight from one
  view to another (today's side column) goes; it costs one more tap. The owner would also accept a persistent bar,
  but chose pages.
- One small kit of components, built once from the game's materials and reused by every view.
- Same functions as today: nothing the player can do now is lost.

## Non-goals

- New features or new game logic. This is a visual and layout rework.
- The upper screen. (The battle message box on the upper screen was also restyled upstream; the owner may add it
  later, it is not in this work.)
- The HOME Menu forwarder, the Builder, the web page.

## Anti-goals (a failure even if it works)

- A modern app look: glow, gradients, drop shadows, large rounded corners, phone-style toggle switches, animated
  particles.
- Game art resized by a non-integer factor, or redrawn by hand to imitate the game.
- Game art (sprites, frames, fonts, palettes) copied into the repository or into the design mockups. It comes from
  the player's own pack at run time, as everything else in this port.
- Slower menus on an Old 3DS than today.

## What "an extension of the game" means (agreed with the owner)

- **Frame**: panels use FireRed's own window frames, with FireRed's own rule (owner's choice): message and
  description panels use the dialogue box's fixed frame (`menu_message`, the one the upper screen shows all the
  time); lists and menus use the frame picked in OPTIONS -> FRAME (10 `typeN` frames, as FireRed's START menu,
  YES/NO and multichoice windows do) and redraw when the option changes.
- **Selection**: the chosen row or button is marked with FireRed's selection arrow (the ▶ glyph of latin_normal,
  in the window's text colour, dark grey, as in its own menus), instead of lighting the whole button.
- **Text**: FireRed's font everywhere; the 5x7 button font goes.
- **Colours and icons**: FireRed's own menu colours and its item and Pokémon icons, pixel for pixel with the upper
  screen.
- **Out**: the embers, glow, gradients, shadows, rounded pills and the toggle switches.

## Scope and order

Phase 1, refined until the owner approves the style:

1. The component kit (below).
2. HOME (the field's home view: region map and menu).
3. Battle commands (FIGHT / BAG / POKéMON / RUN, the quick ball, the Safari Zone's commands).
4. Battle moves, and the double-battle target choice (it shares the move cards' rectangles and code).

Builder's call: views not yet reworked keep their whole current look (frame, column, bar, BACK, backdrop) until
their turn. A mixed look is accepted between phases, but every intermediate build stays fully usable, since the
owner plays local builds throughout. No battle shows two styles at once (hence target and Safari in phase 1).

Phase 2, one view at a time with the approved style: party list and its action menu, Summary (field and battle),
Bag (field and battle), battle party switch, Pokédex, Trainer Card, SAVE, OPTIONS pages, PC menu and boxes, field
choices (YES/NO and multichoice lists), the naming keyboard, the forget-a-move view, the idle view shown while the
upper screen has nothing for it (title, shop, evolution, trade).

## Approaches considered

1. **Whole screens in Claude Design first, then code.** The owner keeps full control, but the game's materials do
   not exist in Claude Design and may not be pasted there, so mockups would use imitations and the result in the
   game would differ from them.
2. **Straight in code.** What the owner sees is what runs, but the owner loses direct control of the layout and
   every idea costs code.
3. **Hybrid (picked by the owner).** The owner designs layout in Claude Design: what is on each view, where, how
   big, the spacing. Game materials appear there as labelled placeholder blocks ("FireRed frame", "item icon 24x24",
   "selection arrow"). In parallel the kit is built in code from the real materials. Layouts are then implemented
   with the kit and refined on the running game. Picked because the owner's two complaints split this way: "cramped"
   is layout (the owner's call, best explored visually), "another game" is material (only real in code).

## Workflow

1. Brief for Claude Design (Portuguese, for the owner): canvas, pixel grid, the kit's pieces with their real sizes,
   the game font's measured metrics, minimum touch sizes, the placeholder convention, and the required elements of
   each phase 1 view (inventory below).
2. Kit in code, shown to the owner as host-renderer captures and in Azahar.
3. Owner's layouts for HOME, battle commands, battle moves, handed off from Claude Design.
4. Implementation with the kit; review on the host renderer, Azahar, then the console (`tools/dev.py`).
5. Refinement rounds until the owner approves the style. Then phase 2.

## Functional inventory (phase 1: must stay available)

**HOME**
- FireRed's region map with the player's position; the current map group (Kanto and the Sevii Islands). Drawn at
  1:1, the whole region visible (about 192x128; owner's choice over a 2x crop), in a framed panel whose thicker
  bottom strip holds the location's name on the left and the region on the right (the owner's reference: the
  NachoGonzalez2001 port's HOME map panel).
- Tap a place twice to Fly there once FLY can be used, as today (targets are the map's 8 px cells).
- The contextual message ("Tap a destination" and the like).
- Entries: POKéDEX (once obtained), POKéMON, BAG, the Trainer Card (labelled with the player's name), SAVE, OPTIONS.
  Each opens its view as a full-screen page; BACK or B returns to HOME.
- The X and Y shortcuts showing the registered items' icons, and the Running Shoes (running mode) switch.

**Battle commands**
- FIGHT, BAG, POKéMON, RUN; the active Pokémon's icon, name and level.
- In wild battles, the quick ball: a fourth button with previous/next arrows and a swipe to change the ball.
- In the Safari Zone, BALL / BAIT / ROCK / RUN.
- The party's six balls (fainted, status, empty).
- Double battles: the commands for each of the player's Pokémon in turn, as today.
- A tap continues a battle message that waits for A.

**Battle moves**
- Four moves with type, power, accuracy and PP; empty and no-PP states.
- In a single battle the effectiveness against the foe's types, STAB, the effect chance, the Physical/Special
  fallback text and the selected move's description, each governed by the existing OPTIONS rows, as today.
- Back to the commands.
- Double battles: the target choice.

No new states: FireRed lets the player pick RUN in a trainer battle or a disabled move and answers with its own
message, and the rework adds nothing for them (no new game logic, no new bridge reads).

**Changed by the owner after this inventory (2026-11-13/14, on the layout canvas): phase 2 does not bring these back.**
- HOME: the X and Y cells left the page (the physical buttons still use the registered items); RUN moved to a
  RUN / SPEED window with the turbo's speed, a SPEED tap stepping it like START; the party's six with their HP bars
  have the window the contextual message had. The idle "Tap a destination" is gone; the map's other hints (a place
  picked, the Party's FLY) take the party's window until a tap on it or a button gives the party back.
- Battle moves: the note shows accuracy, STAB and the description; the effect chance is no longer shown.
- The top screen's turbo badge is gone: HOME's SPEED line shows the speed.

Both battle views keep the D-pad and A/B in step with the touch screen (the arrow follows the game's cursor).

## Component kit (phase 1)

- **Framed panel**: any size on the 8 px grid, drawn from the player's FRAME type (graphics/text_window/typeN.4bpp
  and its palette, 8x8 tiles at 1:1), redrawn when the option changes.
- **List row**: text in the game font with the selection arrow; selected, pressed and unavailable states.
- **Icon button**: a framed or plain cell with a 24x24 item icon (or a Pokémon icon) and an optional label.
- **Header band**: one line of text (a location, a title) on a FireRed colour.
- **HP bar and status**: FireRed's colours for green, yellow and red HP and the status labels.
- **Type label**: a move's type in the game's type colours.
- **Text**: the game's latin_normal font (it holds the selection arrow glyph) and, where space is short, latin_small,
  both from the pack.

## Constraints

- **Canvas**: 320x240 RGB565, software-drawn, column-major (bottom to top); views redraw only when they change, and
  each component draws only inside its rectangle (the dirty-rectangle updates depend on it).
- **Pixel scale**: game art at 1:1, or an integer factor where the owner wants larger icons; never fractional. HOME
  currently enlarges the region map's 8 px cells to 10 px; the rework draws it at an integer scale.
- **Materials from the pack only**: frames, fonts, icons, the map and palettes are read from the player's pack at
  run time. No game pixels or palettes in the source (the clean-release audit counts any read-only table in a game
  object as FireRed data); port-authored pieces (a toggle mark, a shoe glyph) stay original.
- **The GBA picture viewport**: some states still show FireRed's own 240x160 picture on the lower screen, 1:1 by the
  GPU (`PT_VIEW_*`, the `viewport` flag in `3ds_poketouch.h`). With full-screen pages it gets a page of its own (the
  picture 1:1 inside a frame), or each such state is listed and given a view.
- **Phase transition with pages**: today Bag, Pokédex, Trainer Card, SAVE, OPTIONS, choices, naming, PC box and
  forget-a-move are drawn inside HOME's frame (`sMenuFrame`, with the bar, BACK and the side column). Until each one
  is reworked it keeps that old frame and column when opened from the new HOME, so it stays fully usable.
- **File size**: the source audit caps every file at 256 KiB. `3ds_video.c` is 59 bytes under it and
  `3ds_bottom.c` is at 234 KiB, so the kit and the new views go in new files.
- **Performance**: no slower than today on an Old 3DS; the ember backdrop and its four 150 KiB tables go away.
- **Settings**: the EMBERS option row goes; an old settings.cfg with an `embers=` line must still load.
- **Tests that lock today's look** change on purpose: `tools/test_poketouch_render.py` (HOME and viewport pixels equal
  to a baseline commit, OPTIONS/SAVE on the red backdrop) and `tools/test_battle_ui.py` (hit masks equal to the
  previous version). They are re-baselined with the new look, and keep checking redraw consistency and hit areas.

## Decided by the builder (calls, open to the owner)

- The lower screen is not tinted at night (legibility); the upper screen keeps its day/night tint.
- Minimum touch size 24x24 px for buttons and 16 px tall for list rows (the 3DS stylus is precise).
- The selection arrow is always shown on the current choice, as FireRed does, and moves to whatever was last
  touched or reached with the D-pad (today the battle cursor hides after a touch).
- The kit and views live in new files beside `3ds_poketouch.c`; the old ember pieces are removed once no view uses
  them.

## Left to the builder

- The exact list of states that use the GBA picture viewport, and how each is handled.
- How the kit loads and caches frames and fonts from the pack.
- The split of the work into steps and the order of the old code's removal.

## Verification

- Host renderer captures (`3ds_port/tests/poketouch_render_test.c`, `battle_ui_render_test.c`) for each view and
  state, shown to the owner as images.
- Azahar with the owner's save (`tools/dev.py run`), then the console (`tools/dev.py send`).
- The re-baselined render and hit tests, the source audit, the 3DSX checks of the build.
