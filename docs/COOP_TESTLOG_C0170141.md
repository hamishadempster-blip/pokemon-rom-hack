# Test log: build 0xC0170141 (collection update, first delivery)

- **ROM:** `emerald-coop-C0170141.gba`, built from `712d7dc3`. No game-code change after it: later commits touch only docs, the roster manifest, the harness and two scenarios. A clean clone checked out at `712d7dc3` builds the same SHA-1.
  - SHA-1 `dc7c31b978c6b3572a752a3a9793e401c2ff78ac`
  - SHA-256 `3334223a8868bc1815e500303ffe1bd236edc336a7f24798e239389f384880b6`
- **Harness:** the two-core mGBA 0.10.3 harness from `tools/coop_link_harness`, built against this ELF.

Words used here are as in earlier logs:
- **gameplay-tested**: played in the harness.
- **source-inspected**: read, not played.
- **fixture**: state set by the test.
- **test helper**: Pokémon or items given by the test.
- **driver**: the harness choosing inputs.

Harness results are not evidence for the mGBA GUI, real hardware or two-PC netplay, none of which were tested.

## What this delivery contains (and what it does not)

The request has 15 sections. This delivery implements and tests:

- **§15 / storage preflight and migration:** the storage plan (`COOP_STORAGE_PLAN.md`).
  - CoopSave layout 5: 376 bytes appended to SaveBlock3, leaving 684 bytes in reserve.
  - Bond XP lives in each Pokémon's record, in reserved bits proven zero on 6,170 existing Pokémon.
  - Full-slot writes for the link and Berry Blender save paths.
  - Honest refusal of newer saves.
  - Power-cut tests and the compatibility check.
- **§14 Trainer Bond:** complete, with documented gaps (`COOP_BOND.md`).
- **§5 Shiny visibility and SHINY CHECK, §6 catch result, §7 low-HP alarm and easy fishing**, with CO-OP → SETTINGS (`COOP_POLISH.md`).
- **§10 Battle conditions panel** (`COOP_POLISH.md`).
- **§1 manifest:** regenerated on this build. 908 of 908 species obtainable; 900 of 900 in the ordinary roster; 0 issues. It found one Moon Stone source the stale manifest had missed.

**Not started in this delivery:**
- §2 encounter variety: time-of-day tables, weather visitors and reports. `OW_TIME_OF_DAY_ENCOUNTERS` is still off.
- §3 five hidden habitats.
- §4 notebook and collection rewards.
- §8 waiting for a time of day at the Centre.
- §9 Expedition HQ.
- §11 "Who can learn this?".
- §12 town specialities.

Their save space is reserved (zeroed) in layout 5. Nothing for them is in the game yet.

## Defects found and fixed while building it

| Defect | Found by | Fix |
| --- | --- | --- |
| A stale `#define COOP_SAVE_LAYOUT 4` in `coop.c` made every layout-5 save look "unsupported": writes went to a dummy block, and the Duo Tower records were lost | `duo_tower_first` during development | Define removed. `duo_tower_*` pass |
| Trading the partner away did not end the designation until the next map change | `bond_trade` first run | The deal commit ends it. A Pokémon held in a pending deal still counts as owned (a rollback returns it) |
| Withdrawing the partner from the PC lost 1 HP to rounding each time (the PC copies it through a non-party Pokémon, which lost and regained the bonus) | `bond_box` (10/29 → 9/29) | A copy of the save's own partner outside battle keeps its bonus |
| SHEDINJA copied NINCADA's Bond XP and active flag and became a second partner | `bond_evolve` (first run on a ROM before the fix) | SHEDINJA starts with no bond |
| The first-victory bonus for a badge never paid: the check ran at battle end, before the gym script sets the badge | Source review; `bond_major` written after the fix | Won trainer battles watch for new badge, Elite Four and champion flags until the next battle (the champion via GameClear) |
| The SHINY CHECK question was recoloured red: the YES/NO box was drawn while the question was still printing (text printers share one colour table) | `shiny_run` screenshot | The box is drawn after the question finishes |
| SHINY CHECK did not ask on the co-op guest (it read `gBattleMons`, which the guest does not keep) | `shiny_coop` first run | It reads party data |

## Power cuts and save formats

- **Power cuts** (harness `arm_cut`; cut `.sav` files on the layout-5 ROM):
  - Cuts after 1–14 changed sectors, and after 7 at 13 rotation positions: **27 of 27** loads showed the "save file is corrupted, previous save loaded" message and the previous save (marker and PC absent, save counter unchanged).
  - First attempt: all 27 loads failed at the title screen because my load script waited for the main menu without pressing a button. Fixed the script and reran the loads on the same files.
  - Regression in the suite: `save_power_cut` + `save_power_cut_load`.
- **Newer save:** `save_schema_newer` (a copy with layout 6 and a copy with an unknown magic). CONTINUE refuses with "This save is from a newer build. It was not changed. Use that ROM." and returns to the title. Both files are byte-identical afterwards.

## Save compatibility (`compat_check.sh`)

- **Method:** old build 0xC0170140 (SHA-1 `a2013097…`) against this build. Each pair was:
  1. loaded on both builds;
  2. saved on this build;
  3. power-cycled;
  4. relinked;
  5. compared field by field.

| Saves (CoopSave layout) | Old vs new load | Saved vs reloaded | Reloaded vs relinked |
| --- | --- | --- | --- |
| j06, j16, j29, j44, j57, j68 (journey checkpoints, layout 2) | 6/6 PASS | 6/6 | 6/6 |
| seg6 (layout 1) | PASS | PASS | PASS |
| final13C `coast_solo_j68` (layout 3) | PASS | PASS | PASS |
| lab saves, `duo_tower_set` from 0xC017013E and from 0xC0170140 (layout 4) | 3/3 PASS | 3/3 | 3/3 |
| **Total** | **11/11** | **11/11** | **11/11** (33 comparisons, both players each) |

- **What changes on load:** only the `layout` byte (→ 5) and the 376 appended bytes. In every saved file those are zero except the block's size word (`0x0178`).
- **Allowed differences, as before:** map object templates and view, play-time clocks.
- **The user's own two saves were not supplied** and were not tested.
- **Layout 0** (a save from before co-op versioning) was not available and was not tested.

## Final suite (0xC0170141)

`run_all.sh` with `LONG=1`, `SEG6`, `J06 J13 J16 J29 J37 J43 J44 J46 J48 J50 J51 J52 J57 J58 J59 J65 J66 J68` and `OLD13C_COAST`. ROM `712d7dc3`; harness and scenarios at `712d7dc3` (snapshot); output `/home/claude/journey/final141/suite`.

**307 scenarios: 301 PASS, 6 FAIL on the first run; 0 visual-check flags.**

| First-run failure | Cause | What followed |
| --- | --- | --- |
| `bond_tent` | **Scenario:** an exact Bond XP check (25,000) after walking; a random walking friendship gain added 2 outside the tent. Inside, nothing was added, as intended | Check made a range, 25,000–25,010 (`bond_mega`/`bond_tent` commit after `712d7dc3`). Reruns on the same ROM: 2/2 PASS |
| `bond_mega` | **Scenario:** Rosa's Mega CHIMECHO Attack expected 67, a value observed in development. Her nature is random, and it was 82 this time | Natures fixed to HARDY; both Attacks now calculated: Kai 204 = 199 + 5, Rosa 75. Rerun PASS |
| `twins_solo` (and `twins_reload`, which uses its save) | **Driver/RNG:** the Littleroot exhibition double was lost (MEGA FERALIGATR's WATERFALL OHKO'd both 63-HP leads). The run's timing differed by a frame from the 0xC017013F run, so the battle's random rolls differed. The scenario's `strong_battlers 600` fixture does not guarantee the win | Rerun: PASS (no Mega Evolution that time); `twins_reload` on its saves: PASS. Not a game fault, but this scenario's win depends on luck |
| `partner_menu` | **Intermittent:** FOLLOW did not take Kai through the Pokémon Center door after Rosa (he stayed outside; nothing else failed) | Reruns 3/3 PASS. Not reproduced; the first-run log is kept. One pass later is not proof it is fixed. Not seen in the 0xC017013F suite. Possibly a timing-sensitive FOLLOW race; open |
| `roster_repair` | **Driver:** Rosa's `hunt` left the Route 103 grass after one or two battles and met nothing more. This scenario last passed on 0xC017013F with the older `hunt` | Reproduced with the same harness on **0xC0170140** (`hb140b`): a harness `hunt` regression from the 0xC0170140 pass's hunt fixes, which never ran this scenario. Not a game fault; **not fixed** |

**Which results cover what.**
- The ROM delivered is the one the suite ran, `712d7dc3`.
- After the suite, only two scenario files changed (`bond_mega`, `bond_tent`); both were rerun on the delivered ROM and pass.
- This is not one clean run: the first-run failures above are the record.

## New regressions in the suite (all PASS on the delivered ROM)

`save_power_cut`, `save_power_cut_load`, `save_schema_newer`, `bond_mentor`, `bond_ranks`, `bond_ranks_reload`, `bond_xp_wild`, `bond_coop_wild`, `bond_support`, `bond_coop_support`, `bond_major`, `bond_trade`, `bond_practice`, `bond_box`, `bond_tent` (after the change), `bond_evolve`, `bond_mega` (after the change), `shiny_run`, `shiny_coop`, `catch_result`, `lowhp_beeps`, `battle_panel_coop`, `easy_fishing`.

`debug_inputs_off` was updated: SELECT now opens the battle conditions panel, and B closes it.

## Clips (harness recordings, both screens, normal speed)

Recorded on the delivered ROM in `/home/claude/journey/clips141`. These are not mGBA GUI footage.

| File | Scenario | Shows |
| --- | --- | --- |
| `raw_bond_mentor.mp4` | `bond_mentor` (j16) | The Bond Mentor's first conversation, the menu, choosing a partner with the preview |
| `raw_battle_panel.mp4` | `battle_panel_coop` (lab) | REFLECT and TAILWIND, then the guest's and the master's battle conditions panels, expiry |
| `raw_shiny.mp4` | `shiny_coop` (lab) | Shiny stars on both screens; the guest's SHINY CHECK question |
| `raw_catch_result.mp4` | `catch_result` (lab) | The catch result pages; CATCH RESULT turned off in CO-OP → SETTINGS |

## Not covered

- **Bond:**
  - No preferred follower, resting spot or rank greetings.
  - Rank-reached times are not recorded.
  - Speed control other than TAILWIND and TRICK ROOM is not counted.
  - PC release uses the PC's own confirmation.
  - Not tested: TRANSFORM, Cable Club trades, full PC delivery after a trade.
  - Award rates are not tuned with unassisted play.
- **Co-op catch result** in the field: source-inspected only. The harness cannot throw a ball in a two-target co-op double.
- **Polish not tested:** EASY FISHING B-to-cancel and the OLD and GOOD RODS; SHINY CHECK with YES in a co-op double.
- **Earlier open issues, unchanged:** finding #16; the partner drawn on tiles blocked by the other save; the explanation line for differing personal dialogue; six Frontier facilities without a duo mode; the first co-op gym's difficulty.
- **Not tested at all:** the mGBA GUI by a person, real hardware, two-PC netplay.
