# Whole-party EXP, full-team HEALING VIAL, move effectiveness — D1500001

Branch `feature/party-exp-vial-effectiveness`, started from `coop-doorway-scenes-rivals` at `ee2eb431` (the C0170140 line). Isolated like the D14 branches: it does not touch `coop-doorway-scenes-rivals` or `main`, and the build id `0xD1500001` is outside Claude's release sequence, so it will not link with any other build. No save structure, flag, variable, item id or persistent identifier changed.

## What changed

### 1. Every Pokémon in the party gets full experience

Solo and co-op, no item and no unlock. The Pokémon that fought are paid exactly as before (bar, level-up screen, move prompt). Everyone else (the whole bench, and anyone on the field who was never sent out) gets the **full** amount, not half, **after the battle**, silently, on the game that owns them (`src/party_exp.c`):

- The engine records each knockout: the defeated species and level, the full experience one Pokémon earns, and which party slots fought. In co-op the link master sends each knockout to the joining player's game in one small message (`B_COMM_COOP_BENCH`); each game then pays its **own** party, so nothing about a player's Pokémon crosses the cable.
- Same rules as a Pokémon that fought: soft/hard level cap, Lucky Egg, Exp. Charm (the player's own bag), unevolved/affection bonuses, effort values, friendship per level. A Pokémon that was fainted at the time of a knockout gets nothing for it. The engine's own "party shares EXP" flag (`I_EXP_SHARE_FLAG`) stays unused; a held Exp. Share does nothing extra.
- Co-op keeps slots 4–6 out of the party during the battle (`src/coop.c`), so they are paid in the pre-battle copy that is put back afterwards.
- Level-ups from the silent payout: a new move is learned when a slot is free; a full moveset keeps its moves (the Move Relearner in Fallarbor teaches what was skipped), except that the Pokémon on a co-op field get the usual new-move prompt after the battle. Evolution is offered after the battle for every Pokémon that levelled: solo through the stock check, co-op through Opus's progression script (`Coop_RecordSilentLevelUp`, and `Coop_NoteBenchLevelUp` for the benched three).
- Why not the engine's stock party share, which was dead behind a flag: it pays each Pokémon with its own controller command (a link round trip each in co-op), and a level-up there plays a fanfare, a message, a stats box and a move prompt. Measured on the same co-op wild double with six Pokémon per player (4 samples each, whole battle, frames):

| Build | Mean | |
| --- | --- | --- |
| Base (bench gets nothing) | 7,412 | |
| This build | 7,526 | +114, inside the run-to-run spread (6,523–9,243) |
| Stock party share switched on (throwaway build, not kept) | 8,929 | +1,517 (about 25 s) even though it only reaches part of a co-op party |

### 2. HEALING VIAL heals the whole team

One use (BAG, KEY ITEMS, USE; no party menu) is a Center's full heal for the whole party: HP, status, PP, and fainted Pokémon come back. It is much stronger than before, so it holds fewer charges: **1 to start, 2 after the first badge, 3 at four badges, 4 at seven**; a Center refills it. It refuses, keeping its charge, when the whole team is already in perfect shape, and says so when empty. Same places-it-can't-be-used rules as before (Battle Frontier, Safari, Trainer Hill, during a trade or team screen).

| Badges | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Before (1 + badges/2) | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 | 5 |
| Now | 1 | 2 | 2 | 2 | 3 | 3 | 3 | 4 | 4 |

An existing save keeps its charge count, clamped to the new maximum when read (so at 6 and 8 badges one charge fewer, refilled at any Center). The old party-menu callback (`ItemUseCB_HealingVial`) is removed; the item is now a field key item like the BOX LINK. The nurse's explanation and the item description are updated.

### 3. Move effectiveness is always shown

The engine's PP-row indicator (**○** normal, **◎** super effective, **★** ×4, **△** not very, **▽** barely, **✕** no effect; nothing for a status move) is now on in every fight, on both co-op games. It was set to "only for species seen in the Pokédex", which a link battle never records, so it never appeared (`B_SHOW_EFFECTIVENESS`, `include/config/battle.h`). In a double it shows the better result of the two foes and each target's own result while one is chosen. It uses the real types and abilities, so it can show "no effect" against an immunity ability. I tried a plain-words label in the L panel; the panel is 18 tiles wide with two description rows and a third would cover the move boxes, so it is not there. The symbols are the stock ones; the legend is in `docs/COOP.md`.

## Save safety

- Nothing in `SaveBlock1/2/3`, `CoopSave`, flags, variables or item ids changed (no `CoopSave` layout byte bump). `fieldKit` bits and `vialCharges` keep their meaning; only the vial's maximum changed (above).
- `compat_check.sh` on the final ROM, loading saves written by older builds: **PASS** on `lab_opus_head` (saves from `ee2eb431`) and `lab_d1400001` (saves from the D1400001 ROM): same data across builds apart from the allowed rebuilt fields (map templates, map view, play-time clocks), a save written by this build reads back the same, and linking changes only link bookkeeping. (My first run of the second one printed FAIL: the comparison script needs the ARM compiler on `PATH`, which my shell did not have; rerun with it, PASS.)
- **Not** tested: the user's own saves (`Before Bond Update`), which were not available in this session; and an old save that already holds more vial charges than the new maximum (the clamp is existing code, read, not run on such a save).

## Verification (D1500001, SHA-1 below)

- Final ROM, 32 scenarios: **31 pass, 1 fails, and that one fails the same way on the untouched `ee2eb431` ROM** (`debug_inputs_off` expects SELECT at the action menu to do nothing; `ee2eb431` made it open the conditions panel — an out-of-date scenario, not this change). Passing: the fresh-save `rescue`, `guest_move`, `guest_evolution`, `target_navigation`, `spread_selection`; lab-save `wild_route101`, `wild_route101_p1` (the guest starts the fight), `wild_simultaneous`, `wild_run`, `trainer_p1`, `trainer_reconnect`, `solo_six`, `coop_six`, `coop_fainted_lead`, `battle_abort_recovery`, `link_recovery`, `battle_visual_wild_double`, `move_inspector`, `battle_spare_buttons`, `prep_cancel`, `route102_reconnect`, `coop_levelup_boxes`, `startup_timing_gym`, `prep_timeout_host_first`, `prep_link_stall`; and the six new ones below.
- New scenarios (all pass, both roles where co-op):
  - `party_exp_wild`: six copies of the lead per player; every slot gains EXP (163 → 190) and slots 2–6 end with exactly slot 1's total, on both games, the benched three included.
  - `party_exp_levelup`: the whole team one point short of level 16; all six reach 16 and all six evolve, on both games.
  - `party_exp_solo` / `party_exp_solo_levelup`: the same in a solo fight.
  - `vial_team_heal`: a fainted, a poisoned, hurt ones and spent PP (PP 1 and 0 → 35); one use restores all, one charge; empty refuses; a Center refills it; a perfect team keeps the charge.
  - `move_effectiveness`: damaging move 1..6 (never 0), status move 255, each target a real result, on both games.
- Screenshots: the bag shows the new vial description inside its box; the move menu shows the indicator on both screens (Scratch ○, Ember ◎ against Weedle).
- Harness additions: `test_data SLOT pp VALUE` and `expect_mon SLOT pp VALUE MOVESLOT` (`src/coop_testhook.c`, `tools/coop_link_harness/harness.c`); `run_all.sh` and the README list the new scenarios; `field_kit.txt`'s vial section is updated for the new flow but **not rerun** (it needs the j13 checkpoint).

## Not done, not tested

- Not run: the full suite, a person playing in the mGBA window, hardware, two-PC netplay. Harness success is not netplay evidence.
- Not exercised on this build: catching a Pokémon (it goes through the same `getexp`, so the party is paid for the catch too), solo trainer and gym fights, doubles with an in-game partner, Duo Tower and other co-op trainer fights beyond `trainer_p1`/`coop_six`, Mega Evolution, the vial in a co-op session beyond `vial_team_heal` (unlinked).
- There is **no message or fanfare** for the silent payout. The party screen shows the new levels; a "your team gained EXP" line after the battle would be a small addition if wanted.
- The effectiveness hint is symbols only (above), and it reveals an immunity ability (stock engine behaviour once the Pokédex rule is out of the way).
- `docs/COOP_HANDOFF.md` and the older test logs still describe the old vial: that is Opus's live file, so it is left for the integration.

## Integrating

The patch (`include src data`) applies cleanly to `coop-doorway-scenes-rivals` at `ee2eb431` (it is that commit's child) and, separately, the three game files of `fixes/battle-intro-speed` (`0158f117`) apply cleanly on top of it (`git apply --check`, textual only, not rebuilt together). Where it touches code Opus edits: `battle_script_commands.c` (`Cmd_getexp` state 1, the multipliers function), `coop.c` (the progression functions, `Coop_RestoreParty`, `Coop_OnBattleLaunched`), `battle_main.c` (two lines), `battle_controllers.c/.h` (one message type and its receive case). Take the hunks, keep Claude's build id, and on the merged ROM rerun `party_exp_wild`, `party_exp_levelup`, `party_exp_solo`, `vial_team_heal`, `move_effectiveness`, `guest_evolution`, `guest_move`, `coop_levelup_boxes` and `compat_check.sh`. Leave main unmerged.

## Test ROM

Build id `D1500001`, `emerald-coop-D1500001.gba`, both players the same file. Built here with xPack GNU Arm Embedded GCC 13.2.1. SHA-1 `0a6c8e751b421fd146b617bec932cebdff684246`; SHA-256 `6ec1574189911a8c09935bae1e46c4a04739e73bf407a308667f979df540fd77`.

Before playtesting: close both emulators, back up both ROMs and both real `.sav` files, and put a COPY of each player's own save next to the ROM under the ROM's file name (mGBA looks for `name.sav` beside `name.gba`). Keep the originals for rollback.
