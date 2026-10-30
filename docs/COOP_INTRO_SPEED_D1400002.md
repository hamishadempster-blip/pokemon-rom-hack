# Faster co-op battle intro — D1400002

Branch `fixes/battle-intro-speed`, continued from `fixes/move-info-intro-0140` (`9ceb6080`, D1400001). Isolated like that branch: it does not touch `coop-doorway-scenes-rivals`, `main`, any save structure, persistent identifier, party record or content table. Do not merge it over Claude's active tree; bring the hunks over (see "Integrating").

Only co-op battles (`BATTLE_TYPE_COOP`) change. Stock link battles, solo battles and friendly co-op battles (no co-op flag) run the old code.

## Result

Frames from the host's request to its first action menu, three runs each, same lab saves, two-core mGBA 0.10.2 harness, 60 frames per second. Intro-only measurements, not a performance guarantee: they depend on the player's text speed and on how fast the partner answers.

| Fight | D1400001 (before) | D1400002 (this) | Saved |
| --- | --- | --- | --- |
| Wild double (`startup_timing_wild`) | 1,256–1,266 (~21.0 s) | 900–920 (~15.2 s) | ~350 frames (~5.9 s) |
| Gym fight (`startup_timing_gym`, team screen not counted) | 1,846–1,870 (~30.9 s) | 1,620–1,626 (~27.0 s) | ~230 frames (~3.9 s) |

Wild double by stage (host): transition 90 → 90, party exchange 226 → 116, main loop to menu 844 → ~600. What is left in the main loop is presentation (message text, the two ball throws, the cries, the health boxes) plus the stock transition; it was left alone.

## Changes (all in `BATTLE_TYPE_COOP` only)

1. **The starting battlers' data is read locally.** `DoBattleIntro` used to ask each of the four battlers' controllers for its starting data over the cable, one after another (about 40 frames each). Both games already hold the exchanged parties, so the host now builds the same reply with the existing `GetBattlerMonData` conversion (new `BtlController_FillIntroMonDataLocally`, `src/battle_controllers.c`) and writes it to `bufferB` itself. Only the battle engine (the host) reads those buffers; `GetMonData` changes no controller state on either game. About 160 frames.
2. **A wild fight sends only its first enemy block.** The party exchange sent opponent A in three blocks and opponent B in three more. A wild fight has one or two enemy Pokémon, all in the first block, so after it both games skip straight to the end of the exchange (`FinishMultiPartnerPartyExchange`, the old Shedinja pass). The decision is `Coop_IsWildBattle()`, the flag both games settle together in state 2, so both skip the same blocks (a one-sided skip would deadlock the lockstep exchange). The game that did not build the parties clears the slots it would have received (enemy slots 3–6 of side A, all of side B). About 110 frames. Trainer fights still exchange every block: skipping some there would need the host's party size told to the guest, which was not done.
3. **Trainer and wild-Pokémon slide-ins move 6 px a frame instead of 2** (`INTRO_SLIDE_SPEED`, `include/battle_controllers.h`). Every start offset is 240 px and each slide stops on `x2 == 0`, so the speed must divide it; 6 does. 120 frames become 40.
4. **Build ID `0xD1400002`.** It will not link with D1400001, with C0170140 or with Claude's builds.

Also in this branch, from before: the D1400001 inspector sprite and field-marker fixes, the shorter VS animation and the shorter link send-task grace.

## Harness changes

- `rescue_battle.inc` and `rescue_after_boot.inc` waited a fixed 500/60/200 frames with blind A presses, timed for the old intro; with the shorter intro those presses landed on the action menu and chose FIGHT, so six scenarios (`rescue`, `guest_move`, `guest_evolution`, `target_navigation`, `spread_selection`, `inspector_first_guest`) failed at the first action-menu wait. They now wait by state. The revised driver also passes on the D1400001 ROM (`rescue`, `inspector_first_guest`).
- New `wild_enemy_slots` (in `run_all.sh`'s lab list). FIXTURE: stale non-zero data is written into the guest's enemy slots 4–5 (side A) and 2–3 (side B) just before the exchange; at the first action menu both wild Pokémon must be present on both games and every other enemy slot empty on both. Negative control: with the guest-side clearing removed it fails on exactly those four seeded slots; with it, it passes.

## Verification (D1400002, SHA-1 below)

- Equivalence of change 1: a throwaway diagnostic ROM (not committed) ran the old cable request and then the new local fill for every battler and compared them field by field (every field `GetBattlerMonData` sets; a plain byte compare differs in fields it never sets, such as stat stages, which `DoBattleIntro` overwrites). Wild double and gym fight: 4 battlers each, 0 mismatches.
- 26 scenarios on the final ROM, 0 failures: the six fresh-save ones above and 20 lab-save ones (`wild_enemy_slots`, `startup_timing_wild`, `startup_timing_gym`, `wild_route101`, `wild_route101_p1` (guest starts the fight), `wild_simultaneous`, `wild_run`, `trainer_p1`, `trainer_reconnect`, `solo_six`, `coop_six`, `coop_fainted_lead`, `battle_abort_recovery`, `link_recovery`, `battle_visual_wild_double`, `move_inspector`, `debug_inputs_off`, `battle_spare_buttons`, `prep_cancel`, `route102_reconnect`). The first run of the six fresh-save ones failed for the driver reason above; they passed after the driver fix. `wild_enemy_slots` failed once on its own first draft (the guest never answered the invitation); fixed in the scenario.
- Protocol host tests: 132 checks pass.
- Screenshots of the intro on both screens at 20-frame steps: slide-ins land, health boxes, text and menu correct. The VS card is now up for roughly 2.3 s instead of roughly 6.8 s (estimated from the stage times and these frames, not timed directly), since it used to sit through the removed waits.
- The lab saves were made by the D1400001 ROM and loaded by this one.

## Not done, not tested

- Not run: the full suite, a person playing in the mGBA GUI, hardware, two-PC netplay. Harness success is not netplay evidence; the cable there is slower than the emulated one, so real times will differ.
- The user's own saves (`Before Bond Update`) were not available in this session, so `compat_load` / `compat_reload` were not rerun. No save code changed, but that is an inference, not a test: rerun them before relying on this build with those saves.
- Not run on this build: Duo Tower and other COOP trainer fights outside the gym, co-op legendary fights, Mega Evolution, evolution. They use the same code paths, but they were not played.
- Trainer-fight party exchange (up to 8 blocks) and the presentation phases are unchanged.
- Autosave: still not implemented, for the reasons in `COOP_VISUAL_FIX_D1400001.md`.

## Integrating

The three game files (`include/battle_controllers.h`, `src/battle_controllers.c`, `src/battle_main.c`) apply without conflicts to `coop-doorway-scenes-rivals` at `ee2eb431` (checked with `git apply --check` in a scratch tree, then removed; textual only, not rebuilt or retested there). Take the hunks, leave `COOP_BUILD_ID` on Claude's sequence, bring the two `.inc` driver fixes and `wild_enemy_slots`, rebuild, and rerun `wild_enemy_slots`, `startup_timing_wild`/`_gym`, `wild_route101_p1`, `battle_spare_buttons` and the six fresh-save scenarios on the merged ROM. Leave main unmerged.

## Test ROM

Build ID `D1400002`, file `emerald-coop-D1400002.gba`. Both players must use this same file. Built here with xPack GNU Arm Embedded GCC 13.2.1; it is not byte-identical to the D1400001 ROM that was built elsewhere. SHA-1 `b4d00b44199335dcabe52d5a15605b3ca2bb6750`; SHA-256 `d14da3f3cbd229a137e066f9e02d7dcba4a7affee998aaceb93dcd6e58aba72e`.

Before playtesting: close both emulators, back up both ROMs and both real `.sav` files, and put a COPY of each player's own save next to the ROM under the ROM's file name (mGBA looks for `name.sav` beside `name.gba`). Do not use it with a save already migrated by Claude's newer build.
