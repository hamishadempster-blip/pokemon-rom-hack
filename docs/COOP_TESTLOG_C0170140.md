# Test log: build 0xC0170140 (release-readiness pass)

ROM `emerald-coop-C0170140.gba`, built from `4ac1d749` (no game-code change after it; later commits are harness, scenarios and docs). A clone checked out at `4ac1d749` builds the same SHA-1. SHA-1 `a20130970e1caed23b0d7dbfe48bad0134ab29d5`, SHA-256 `6f0f67866deb3dfd31d69b23e0ed632b593a5b8c5e9fe5bbfb6156eb13ec4fc5`. Harness: mGBA 0.10.3 two-core harness from `tools/coop_link_harness`.

This pass produced two builds. **0xC017013F** (`38d764f1`, developer menus off) ran the full suite; its one game-side first-run failure was a wrong message, fixed in **0xC0170140** (`4ac1d749`, one change in `src/coop_prep.c`), which then ran every affected scenario. Which result covers which build is stated in each section. 0xC017013F was never delivered.

Words used here: **gameplay-tested** = played in the two-core harness; **source-inspected** = checked by reading code, not played; **fixture** = state set by the test, not reached by play; **test helper** = Pokémon, items or healing given by the test; **driver** = the harness choosing inputs (`smart_moves`, `weak_foes`, `strong_battlers` are driver aids or fixtures and are labelled in each scenario). Harness success is not evidence for the mGBA GUI, real hardware or two-PC netplay, none of which were tested.

## What this pass did

1. **Audit.** Started from the 0xC017013E open-issue list and the test record, then probed inputs a player can press that no scenario pressed: every spare button (R, L, START, SELECT, L+R, R+START) at the battle action menu, the move menu, the field and the summary screens, in both roles of a co-op battle. This found the first defect below (developer menus left on); the final suite found the second (a wrong partner message after a team-screen timeout). Nothing else new was found: no other crash, freeze, save damage, invalid name or progression blocker appeared in the probes, the suites or the compatibility runs. The 0xC017013E open issues are unchanged and listed under *Not covered*.
2. **Fix and regressions** for the first defect (`38d764f1`, build 0xC017013F).
3. **Save compatibility check** (new tools, `cd700e3b`): older saves on this build, field by field.
4. **Difficulty sample** without fixtures (not pass/fail).
5. **Final suite** on 0xC017013F. Its first-run failures: three driver timeouts (rerun) and one wrong partner message after a team-screen timeout (fixed in 0xC0170140, with a regression).
6. **Affected-subset run** on 0xC0170140: every scenario that uses the team screen or a story invite, and reruns of the first-run failures.
7. **Clips** (recorded on 0xC017013F; the 0xC0170140 change does not touch what they show).

## Automated validation (final tree)

| Check | Result |
| --- | --- |
| `tools/coop_protocol_test.c` (host build) | all passed |
| `tools/coop_trainers/guide.py --check` | 189 cards (252 ids), 57 selection rules, 0 errors |
| `tools/coop_trainers/validate.py` | 0 errors, 1 warning (Brenden's Crabrawler, kept from the guide) |
| `tools/coop_mega/rewards.py --check` | 83 Mega forms covered by 82 stones (+1 by a move); 84 item balls; 0 errors |
| `tools/coop_exchange/stock.py --check` | ok; Mega Stones only at the MEGA STONE counter |
| `tools/coop_roster_audit/roster.py --check` | 0 issues |
| `tools/coop_text_width.py` on `coop.inc`, `coop_frontier.inc` | every line fits |

## Defects fixed in this pass

| Defect | First wrong state | Fix | Regression |
| --- | --- | --- | --- |
| SELECT at the action menu of a co-op battle crashed the game that pressed it (the partner's game in the probe: `pc=0x1a8`, `cb2=CB2_BattleDebugMenu`); R+START on the field opened the developer menu (set flags, variables, give Pokémon); SELECT on a summary page other than INFO opened the sprite visualizer | Every ROM so far was built without `RELEASE`, so pokeemerald-expansion's developer menus (`DISABLED_ON_RELEASE`) were compiled in and reachable by ordinary buttons | `include/config/debug.h`: `DEBUG_OVERWORLD_MENU`, `DEBUG_BATTLE_MENU`, `DEBUG_POKEMON_SPRITE_VISUALIZER` FALSE in every build. Asserts stay on (the build is still not `RELEASE`): an assert shows a crash screen that START continues past, which is how the harness reports them | `debug_inputs_off`: SELECT at the action menu on both games, R+START on the field. On 0xC017013E: crash (`CB2_BattleDebugMenu`); on this build: PASS. `battle_spare_buttons`: R, START, SELECT, L, L+R at the action menu and L, R, START, SELECT, L+R in the move menu on both games, then B back to the action menu and the battle finished. PASS |
| After nobody finished the team screen, the partner could read "RED stepped back from the battle. Nothing started." although nobody stepped back: the time had run out (0xC017013F final suite, `prep_timeout`, first run). Nothing started, no badge, still linked: a wrong message only | The two cartridge clocks tick their seconds at different moments, so either game's two minutes can end first. When the host's ended first, its withdrawal of the story invite (sent at once) reached the partner before the team-screen block that carries the reason (sent on the next poll); the partner took the withdrawal to mean the host stepped back | `src/coop_prep.c` (0xC0170140): after a withdrawal the partner waits up to 60 frames for the block and uses its reason, and cannot commit meanwhile; with no block it ends as before ("stepped back") | `prep_timeout_host_first` (FIXTURE: the host's team-screen clock moved on to 115 s): on 0xC017013F the partner reads "RED stepped back" (FAIL, reproduces the suite failure every time); on 0xC0170140 both read "took too long" (PASS); part 2, the host's real CANCEL, still gives "RED stepped back" on the partner |

## Final suite (0xC017013F) and affected run (0xC0170140)

**1. Full suite on 0xC017013F** (`run_all.sh` with `LONG=1`, `SEG6`, `J06 J13 J16 J37 J43 J44 J46 J48 J50 J51 J52 J57 J58 J59 J65 J66 J68`, `OLD13C_COAST`; ROM from `38d764f1`, SHA-1 `1ffd618e…`; harness and scenarios at `cd700e3b`; output `/home/claude/journey/final13F/suite_run`): **283 scenarios, 279 PASS, 4 FAIL on the first run; 0 visual-check flags.** 11 of the 283 are deliberate fault/recovery tests (all PASS).

| First-run failure | Cause | What followed |
| --- | --- | --- |
| `prep_timeout` | **Game (wrong message):** the host's two minutes ran out first; the partner read "RED stepped back from the battle" (expected "took too long"). Nothing started, still linked | Fixed in 0xC0170140 (above). Regression `prep_timeout_host_first` reproduces it on 0xC017013F every time. On 0xC0170140: `prep_timeout` 4/4 PASS (affected run + 3 reruns), `prep_timeout_host_first` PASS |
| `roster_b3_battle` | Driver: SUPER FANG (90% accuracy) missed Minior and the scenario waited for the Core form without another turn | Scenario repeats SUPER FANG until it lands (`e26aea1d`). On 0xC0170140: 3/3 PASS |
| `late_wild_tour` | Driver: six Poké Ball throws did not catch the one DRIFLOON met (it then fainted to Surf); no second DRIFLOON in the 120,000-frame budget | Same scenario on 0xC0170140: 3/3 PASS (reruns), then PASS with the final harness |
| `roster_b3_catch` | Driver: Kai's hunt for FLABÉBÉ (a 20% slot on Route 117) drifted north out of the grass over many battles, into two trainers' sight; after those fights he stood between a beaten trainer and the trees and met nothing more | Reruns on 0xC0170140 with the same harness: 0/3 PASS. Reruns 1 and 3: the same drift (rerun 3 also tripped a visual-check false alarm on a half-printed Safari prompt). Rerun 2: Rosa's twenty ULTRA BALLs ran out before her last hunt (HISUIAN ZORUA), and the driver then kept choosing a move TAUNT blocked. Harness fixes `f878f3d8`, `4fe33bc9`, `9a359ae3`: `hunt` stays within a tile of its starting row (and re-anchors when the way back is blocked, e.g. by a ledge); 40 ULTRA BALLs; `visual_check` skips a half-printed prompt; `hunt` leaves the bag when a ball is refused (with only the first fix, `roster_b3_play` failed: Rosa jumped a ledge below her spot and could not get back, fixed by the re-anchor and a new spot; with the first two fixes, `roster_b3_catch` passed once and then failed when Rosa's GALARIAN STUNFISK used BOUNCE: a ball cannot be thrown at a Pokémon in the air, and the driver pressed A in the bag until its timeout, as on 0xC017013D). Results below |

**2. Affected-subset run on 0xC0170140** (ROM `4ac1d749`, harness and scenarios at `4ac1d749`; `/home/claude/journey/final140/affected`): a fresh `save_lab`, then every `run_all` scenario that opens a team screen or a story invite (story consent, Beacon off, cancel, no answer, simultaneous, not ready, expiry by game clock and by wall clock, team-screen cancel/timeout/stall/slots 4–6/win and reload/solo lead, busy partner in a menu/message/timeout, gym teams, boss pair, guide team selection, level-up boxes, rival looks, Calvin co-op, story segment 1, Wally, Tate & Liza Mega, every Duo Tower scenario, the desks, the sustained session), `debug_inputs_off`, `battle_spare_buttons`, the new regression, and 3 reruns each of the four first-run failures: **57 runs, 54 PASS, 3 FAIL** (the three `roster_b3_catch` reruns above). 0 visual flags apart from that false alarm.

**3. Hunt scenarios with the final harness** (ROM `4ac1d749`, harness and scenarios at `9a359ae3`; `/home/claude/journey/final140/hunt`; two earlier attempts with intermediate harness fixes are kept in `hunt_old` and `hunt_b`): every `run_all` scenario that uses `hunt`, with the reloads that use their saves, and `roster_b3_catch` three times: **10 runs, 10 PASS**: `late_wild_tour`, `roster_b1_play` (+ reload), `roster_b2_play` (+ reload), `roster_b3_play` (+ reload), `roster_b3_catch` three times. No Pokémon went into the air in these runs, so the refused-ball path of `hunt` was not reached (source-inspected only).

**Which results cover what.** The ROM delivered is 0xC0170140. Its only game-code difference from 0xC017013F is the team-screen change in `src/coop_prep.c` (and the build id); the 0xC017013F suite covers the rest of the game. Every scenario that reaches the changed code ran again on 0xC0170140 (run 2). The harness changes after `cd700e3b` touch only `hunt`, `visual_check.py`'s partial-prompt rule and four scenarios (`roster_b3_battle`, `roster_b3_catch`, `roster_b3_play`, the new `prep_timeout_host_first`); every scenario using `hunt` ran again with them (run 3), and every 0xC017013F suite log still passes the changed `visual_check.py`. This is not one uninterrupted green run of all 284 scenarios: the first-run failures above are part of the record.

## Save compatibility

`tools/coop_link_harness/compat_check.sh` with `compat.py` (both new; rules in [COOP_SAVE_COMPAT.md](COOP_SAVE_COMPAT.md)). The old build is 0xC017013D (`1b26b863`, SHA-1 `7ec98060…`), the last build before CoopSave layout 4, rebuilt from a clean clone; its harness was built from this harness source against its ELF. Run on 0xC017013F (`/home/claude/journey/compat13F_b`) and again on 0xC0170140 (`/home/claude/journey/compat140`, comparisons in `compat140_cmp.txt`).

Saves: the journey checkpoints j06 (Rustboro, before Roxanne), j16 (Rustboro and Rusturf Tunnel, after the first gym), j44 (on the way to Petalburg, Strength learned), j68 (after the League), the story-segment-6 save, the lab saves (a new game at the lab), and e13: `duo_tower_set`'s saves from the 0xC017013E final suite (already layout 4, with Duo Tower records). Each pair (both players) was:

1. loaded on 0xC017013D and on the new build (CONTINUE, dump), then saved on the new build (START → SAVE, dump);
2. power-cycled on the new build (CONTINUE, dump), then linked (LINK UP, dump);
3. compared field by field, with money, coins, game stats, bag quantities and berry powder decrypted (their key changes on every map load).

| Comparison (7 saves × 2 players) | 0xC017013F | 0xC0170140 |
| --- | --- | --- |
| Old build's load vs new build's load | 7/7 PASS | 7/7 PASS |
| New build's save vs power-cycled reload | 7/7 PASS | 7/7 PASS |
| Reload vs relinked | 7/7 PASS | 7/7 PASS |

What that means: party, PC boxes, bag, money, coins, Pokédex, flags and variables are identical across the builds on every save. The only change on load is the CoopSave `layout` byte raised to 4 on the six layout-3 saves, with 148 bytes of new fields appended, zero (e13 was already layout 4). A save written by the new build reads back the same after a power cycle. Linking changes only the partner Pokédex snapshot and the quest partner.

Allowed differences, reported but not failures: map object templates and the map view (rebuilt from the ROM on every load), play time, the berry-tree clock, and on saves that crossed a day the daily seed and the two lottery variables (0x404B, 0x404C). First 0xC017013F run: j16 player 2 differed in `lastBerryTreeUpdate` (1 byte: the berry clock, set from the cartridge clock at load); it was added to the allowed clock fields and the stored dumps re-compared: 21/21 PASS. The 0xC0170140 run's comparisons first stopped with "offset compile failed" (run from a bare `git archive` copy, which lacks the generated headers `compat.py` compiles against); they were run again from the built tree on the same dumps: 21/21 PASS. The docs now say to run it from a built tree.

Not covered: saves from before 0xC017013D were not loaded in this check (layouts 1–3 are migrated by the same append-only code and were tested in their own releases); a save written by the new build was not loaded on an older ROM (not supported).

## Difficulty sample (not pass/fail; no fixtures; driver picks each battler's most effective move, uses no items and never switches)

| Battle | Teams | Runs | Result |
| --- | --- | --- | --- |
| Roxanne + Heath (co-op gym 1: Dwebble 16, Archen 17, Nosepass 18 / Geodude 16, Lileep 17, Nacli 18, Oran Berries, smart AI) | Checkpoint j13 as played: each player's first three (Lv 5–16) | 3 | 3 losses; 0–1 of 6 opponents fainted |
| same | Each player's best three from the j13 boxes (team screen): Kai Shroomish, Buizel, Combusken Lv 16; Rosa Skiddo Lv 13, Shroomish, Marill Lv 16 | 3 | 3 losses; 1, 3 and 4 of 6 fainted |
| same | Same six raised to Lv 18, the level cap before the first badge (`test_level`, with their level-up moves) | 3 | 3 losses; 2 of 6 fainted each time |
| Duo Tower LV50 battle 1 | j68 parties as played | 3 | 3 wins |

Reading: the first co-op gym beat the driver nine times out of nine, including at the level cap with the best team on hand. The driver plays worse than a person (no Oran Berries or potions, no switching out of bad match-ups, no setup moves), so this does not show the gym is unwinnable; it does show the first gym is a steep step for a pair at or below the cap, and it needs a human playtest. Late-game duo battles with realistic parties were won.

## Player-experience coverage (0xC017013F final suite; those reaching the team screen also on 0xC0170140)

| Requested | Scenarios in `run_all.sh` | Notes |
| --- | --- | --- |
| Fresh opening and reload | `release_newgame_boys`, `release_newgame_girls`, `solo_opening`, `catch_before_dex`, `bedroom_reload`, `save_identity` | The real NEW GAME path on this ROM |
| Centre, shops, PC | `centre_link`, `center_beacon`, `shop_b2`, `field_kit`, `items_personal` (+ reload), `exchange_coop` (+ reload) | |
| Wild and trainer battles | `wild_route101`, `wild_route101_p1`, `wild_simultaneous`, `battle_visual_wild_double`, `coop_six`, `solo_six`, `trainer_p1`, `coop_wally_vr_partner`, `battle_spare_buttons`, `debug_inputs_off`, `story_seg1`–`6` | Driver aids and fixtures are labelled in each scenario |
| Boss team from slots 4–6 | `prep_slots456`, `prep_win_j13` (+ reload), `prep_cancel`, `prep_timeout`, `prep_timeout_host_first`, `prep_link_stall`, `prep_solo_lead` | Party identity and EXP checked slot by slot |
| Loss, whiteout, escape | `coop_wild_loss`, `whiteout_home`, `wild_run`, `quest_loss_retry`, `duo_tower_behind`, `opt_escape_dig` | |
| Staggered cutscenes | `doors_stagger`, `center_beacon`, `pp_stagger` (+ reload), `scene_meteor_join`, `rival_latejoin`, `story_*` | |
| Catching with a full party or PC | `legend_ray_full` (PC filled by fixture), `shoal_vulpix_full_party` (party filled by test helper) | |
| Quests, legendaries, Frontier | `quest_goods`, `quest_stone_catchup`, `quest_busy_partner`, `shoal_coop` (+ reload), `legend_ray_*`, `legend_groudon`, `legend_regirock`, `duo_tower_*`, `duo_desks` | Six Frontier facilities have no duo mode |
| Save, reload, relink | every `*_reload`, `relink_policy*`, `rival_reconnect`, `npc_reconnect`, `link_recovery`, `reconnect`; `compat_check.sh` (above) | |

## Clips (harness recordings on 0xC017013F, both screens side by side, normal speed)

| File | Scenario | Shows |
| --- | --- | --- |
| `exploration.mp4` | `indicator_tour` | Both players through Littleroot, the Route 101 map connection, a house door and back, the Mach Bike and a dark cave, with the partner indicator |
| `npc_interaction.mp4` | `talk_join` (j68 saves, Lilycove) | Kai talks to the POKéFAN; Rosa joins the conversation; each reads at their own text speed |
| `coop_battle_complete.mp4` | `coop_six` (lab saves) | A co-op battle against Calvin on Route 102 from the start to the win and back on the field. FIXTURE: the lab party copied into six slots (`fixture_six`) |

The lab saves' Pokémon carry the nickname "Aaaaaaaaaaaa": the test's naming step presses A on the keyboard (a test save, not a defect). None of the three scenarios reaches the 0xC0170140 change (no team screen ends early in them; Calvin is an ordinary trainer, no team screen). Recorded by the harness's `RECORD` option with captions added; not mGBA GUI footage.

## Not covered

- **Finding #16** (invalid opponent-name assertion after a co-op wild battle into trainer detection): still not reproduced; not declared fixed.
- **The one partner-side assert in a one-trainer co-op fight (0xC017013E pass 1)**: the name lookup was fixed in 0xC017013E; why only the partner's game asserted is not proven. With the battle developer menu removed since 0xC017013F, one possible route (the partner's game reading debug-battle state) is gone, but that is not shown to be the cause.
- Partner drawn on tiles blocked by the other save's puzzle state; no explanation line when a joined conversation's personal dialogue differs; rival art and menu restyling; six Frontier facilities without a duo mode (unchanged from 0xC017013E).
- Not played: berry planting and harvest, the Trick House, Illusion, Repel prompts around scripts and transitions, joined conversations with mismatched progress, postgame rematches with mismatched progress, team-screen battles with Mega Evolution or move learning from slots 4–6.
- mGBA GUI by a person, real hardware, two-PC netplay: not tested. Clips are harness recordings.
- Balance: the sample above is the only unassisted evidence; the early-gym result needs a human playtest.
- The harness's battle driver can loop when TAUNT blocks the move it keeps choosing (seen once in a `roster_b3_catch` rerun after the balls ran out); a driver limit, not a game fault, not fixed (the scenario now has enough balls).
