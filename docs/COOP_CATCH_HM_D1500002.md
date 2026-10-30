# Build 0xD1500002: catch swap safety, HMs without a Pokémon

Branch `fixes/catch-swap-hm`, based on `feature/party-exp-vial-effectiveness`
(D1500001). Opus's branch `coop-doorway-scenes-rivals` and `main` are untouched.
Save layout unchanged; no migration.

## User reports

1. A wild catch with a full party (six different Pokémon). The in-battle
   "Send which POKéMON to the PC?" menu listed six copies of the lead
   (Marshtomp "Flopper"). The game then froze on "It was placed in BOX 'BOX1'",
   with a YES/NO box still on screen, and mGBA reported "Unimplemented BIOS
   call". The linked partner's game dropped to 7.9 fps.
2. Rock Smash could not be removed from Marshtomp ("HM moves can't be
   forgotten"). The user wants HMs to work without a Pokémon knowing the move.

## Findings

- The in-battle swap menu is only used in ordinary (non-co-op) battles. Co-op
  wild catches skip it: `BattleScript_CoopSuccessBallThrow` places the catch on
  the catcher's own save after the fight.
- That menu runs `UpdatePartyToBattleOrder()`, which rewrites the REAL party from
  the battler's packed order (`gBattlePartyCurrentOrder`, from
  `battlerPartyOrders[gBattlerAttacker]`). If the order is not a permutation of
  slots 0-5 (an all-zero order, for example), every slot gets a copy of one
  Pokémon. That matches six copies of the lead. Closing the menu with
  `UpdatePartyToFieldOrder()` would make those copies permanent.
- The YES/NO box of "Add it to your party?" was never cleared, so it stayed
  over the next message. That is the box in the second screenshot.
- NOT REPRODUCED: the six-copies menu and the freeze. Tried with six different
  Pokémon (one knowing Rock Smash): solo, solo while linked (Beacon on), after
  fighting first, with the catch EXP causing a level-up (Marshtomp 17 to 18),
  and after switching Pokémon in battle. In every case the menu and the party
  were correct. The first incorrect state could not be found, so the root cause
  is **not confirmed**. The user's build ID is also unknown.

## Changes

- `src/party_menu.c`: `IsBattlePartyOrderValid()` (also in `party_menu.h`).
  `UpdatePartyToBattleOrder()` and `UpdatePartyToFieldOrder()` leave the party
  alone when the order is not a permutation. An invalid order can no longer copy
  one Pokémon over the others.
- `src/battle_script_commands.c` (`Cmd_givecaughtmon`): the swap menu rebuilds
  the catcher's order with `BufferBattlePartyCurrentOrderBySide()` if it is
  invalid; the YES/NO box is cleared on A and B; an empty slot chosen counts as
  cancel.
- HMs: `P_CAN_FORGET_HIDDEN_MOVE TRUE` (forget HM moves at level-up, on
  evolution, in the summary's move list and at the Move Deleter: all four checks
  go through `CannotForgetMove`). `B_CATCH_SWAP_CHECK_HMS FALSE` (a Pokémon that
  knows an HM can go to the PC from the swap menu).
- HMs without a Pokémon: `IsHmFieldMoveUsableWithoutMon()` (`field_move.c`) is
  TRUE for the eight HM field moves when the HM item is in the bag and the
  move's badge is held (the existing badge rule is unchanged):
  - `checkfieldmove` (Cut, Rock Smash, Strength, Surf, Waterfall, Dive scripts):
    if nobody knows the move, the first healthy non-egg Pokémon stands in for
    the field effect.
  - `PartyHasMonWithSurf()`: TRUE with the HM and badge (A at water, party menu).
  - Party menu: the field-move list also offers those HM moves (Fly and Flash
    are only reachable here), within the menu's eight entries (room is kept for
    SWITCH, ITEM and CANCEL; a move already listed is not repeated).
- `COOP_BUILD_ID` 0xD1500002. Both players need this build.

## Harness

- `catch ... [SWAP]`: the 7th argument (slot 1-6) answers YES to the swap prompt
  and picks that slot. `CATCH_SHOTS=1` saves a screenshot at each throw.
- New scenarios (lab saves, in `run_all.sh`): `catch_full_party_solo`,
  `catch_full_party_linked_solo`, `hm_no_mon`, `hm_party_menu`.

## Evidence (D1500002, final source and harness, lab saves)

| Scenario | Result |
| --- | --- |
| catch_full_party_solo, catch_full_party_linked_solo | PASS |
| hm_no_mon (rock refused without HM06, refused without badge, smashed with both) | PASS, screenshots checked |
| hm_party_menu (FLASH/FLY listed only with HMs and badges) | PASS, screenshots checked |
| catch_before_dex, solo_six | PASS |
| party_exp_wild, party_exp_levelup, party_exp_solo, party_exp_solo_levelup, vial_team_heal, move_effectiveness | PASS |
| `tools/coop_protocol_test.c` | all passed |
| compat_check.sh, lab saves, Opus head ee2eb431 vs D1500002 | PASS (the first try printed FAIL because the toolchain was not on PATH; the rerun with it passed) |

## Not tested / gaps

- The original six-copies bug and the freeze were not reproduced. The fix is
  defensive: the party can no longer be overwritten, and the swap uses a rebuilt
  order. If it happens again, note the build ID (title or CO-OP screen) and
  whether the players were linked, and keep the save.
- Forgetting an HM at a level-up was checked in source only: all four forget
  checks call `CannotForgetMove`, which now returns FALSE. There was no lab-save
  scenario for it.
- Cut, Strength, Surf, Waterfall, Dive and the Fly/Flash field effects without
  a knower: same code path as Rock Smash (`checkfieldmove`), but not played.
  Fly and Flash were checked as menu entries only.
- `field_kit` needs the j13 checkpoint (five Pokémon), which is not available.
  On lab saves it fails on empty slots. Not counted.
- Co-op wild doubles: the harness's ball throws did not catch in a quick
  co-op attempt (not investigated). Co-op catches do not use the changed menu.
- Not checked on mGBA GUI, two-PC netplay or hardware.
