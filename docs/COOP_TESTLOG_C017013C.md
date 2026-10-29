# Test log, build 0xC017013C (co-op compatibility audit)

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). ROM source commit `cfc98e64`; the commits after it change the harness, scenarios and docs only.
- ROM `emerald-coop-C017013C.gba` (32 MB): SHA-1 `68a24090dd7cfb2fbba8f58b2c4abe47895c958f`, SHA-256 `42277453eaea1f574b2d8bfe76a420a3b6257327e768be1bc780b6390a8059db`. A clean build of `cfc98e64` in a fresh worktree gives the same SHA-1.
- All gameplay is the two-core harness on libmgba 0.10.3, p0 = Kai (link master), p1 = Rosa, from existing checkpoints (no new playthrough). Fixtures and test helpers are named in each scenario's header and logged. Not tried: the mGBA GUI, real hardware, netplay. The audit table is [COOP_AUDIT_C017013C.md](COOP_AUDIT_C017013C.md).

## Fixes and their regressions

Each regression was run on 0xC017013B (a worktree build of `361a67ae` with its own harness) and on 0xC017013C.

| Fix | Scenario (checkpoint) | 0xC017013B | 0xC017013C |
| --- | --- | --- | --- |
| A: a shared gym fight while the partner's BAG is open | `mandatory_busy_menu` (j44, Petalburg Gym) | FAIL: Rosa's game left the bag for the battle without closing it (`coop.sInBattle` 1 while she never pressed B) | PASS: Rosa's game answers busy (7 retries counted), the bag stays open; she closes it (back to her start menu) and the fight starts for both |
| A: the partner stays in a menu | `mandatory_busy_timeout` (j44) | (not run: the retry did not exist) | PASS: no battle; after 30 seconds Kai: "Rosa stayed busy in a menu or a conversation. Nothing started."; Rosa's party screen never closed; Kai asks again once she is back and the fight starts |
| B: the partner reading an NPC's message | `mandatory_busy_talk` (j13, Rustboro Gym) | Rosa's game stopped at the battle's start (malloc check, reset; first seen in an earlier draft of this build, same code as 0xC017013B) | PASS: busy (11 retries), she reads on; when her message closes the fight starts for both |
| C: TRICK on the ally | `coop_held_items` (j16, Route 116 wild double) | FAIL: after the battle Kai's Pokémon held Rosa's LEFTOVERS and Rosa's held Kai's CHARCOAL | PASS: "But it failed!" (TRICK, then THIEF on the ally); each keeps its own item, no copy in either bag |
| D: two players in Secret Bases on one map number | `coop_secret_base_room` (seg6) | FAIL: each screen drew the partner in its own base | PASS: not drawn in the base; drawn again back in Rustboro |
| E: Strength boulders | `coop_boulder_personal` (j59, Seafloor Cavern room 8) | FAIL: Kai's push moved Rosa's boulder from (4,7) to (4,6) | PASS: hers stays at (4,7) until she pushes it |
| E: the same, in play | `journey_60_seafloor` (j59, main playthrough segment, no fixtures) | FAIL: Rosa blocked at (5,5) in room 8 | PASS (Seafloor Cavern, Archie, both to Route 128) |

F (incoming deal Pokémon checked) and G (shared-win flag list) are hardening: no reproduction through play (a normal game never offers a Bad Egg; scripts only send the listed flags). Regressions for the deal paths: `deal_trade`, `deal_gifts` and the rest of `deal_*` pass.

## Existing checkpoints replayed for the audit (0xC017013C)

| Segment | Covers | Result |
| --- | --- | --- |
| `journey_21_briney` | Mr. Briney's boat with both players | PASS |
| `journey_36_wattson` | Mauville Gym switches and barriers, co-op Wattson | PASS |
| `journey_41_flannery` | Lavaridge Gym | PASS |
| `journey_43_rusturf` | Rock Smash | PASS |
| `journey_45_norman` | Petalburg Gym room doors | PASS |
| `journey_51_winona` | Fortree rotating gates | PASS |
| `journey_58_tate_liza` | Mossdeep Gym | PASS |
| `journey_60_seafloor` | Dive, currents, Strength, Rock Smash, a co-op story fight | PASS (failed before fix E) |
| `journey_63_sky_pillar` | Mach Bike floors and falls, Rayquaza, Sootopolis | FAIL on 0xC017013B and first on 0xC017013C: Rosa's commands started while Kai talked to Maxie, so Kai got the busy note (a race in the script); with a wait added: PASS |
| `journey_64_juan` | Sootopolis Gym ice | PASS |
| `journey_67_elite_four` | Four Elite Four fights and the Champion back to back, both players | All five won, both at the Hall of Fame; the scenario still reports a failure: its last step's 3,000-frame budget covered the whole Champion fight (the original playthrough split it off) |

New focused scenario: `coop_wild_loss` (j16): both lose a co-op wild double: each wakes at its own heal place, healed, still linked; money unchanged on both (a co-op loss costs nothing). PASS.

## Full regression on the final ROM

`run_all.sh` with `LONG=1`, `SEG6`, `J06`, `J13`, `J16`, `J37`, `J43`, `J44`, `J48`, `J52`, `J59`, `J65`, `J68` on ROM `68a24090…` (harness at `ca8436f1`): **215 scenarios, 211 PASS, 4 FAIL**. Visual-check note: 1 (`coop_battle_items`, the known nameless battler at a send-out).

| Scenario | What failed | Cause | After (same ROM) |
| --- | --- | --- | --- |
| `roster_b3_battle` | Super Fang missed ("Minior avoided the attack!"), so the form change never came | Luck (90% accuracy) | PASS on rerun |
| `roster_b3_catch` | The surfing Galarian Stunfisk hunt ran out of budget (8 of 9) | The hunting driver, as on the two previous builds | PASS on rerun (all nine caught) |
| `twins_coop` | Rosa's walk to (13,18) found a wandering villager standing on it for its whole budget | Test driver timing | PASS on rerun |
| `twins_coop_reload` | Not run: it continues `twins_coop`'s saves, which were quarantined | Follows the above | PASS on the rerun's saves |
