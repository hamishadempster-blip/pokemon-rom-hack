# Test log: build 0xC017013D (Hoenn Co-op Battle Guide v2)

ROM `emerald-coop-C017013D.gba`, built from `1b26b863` (no game-code change after it; later commits are harness, scenarios and docs). SHA-1 `7ec98060cb241e0749fbc4d73e43c93f4cf05527`, SHA-256 `1ef8450dbe64423bfa1868c08392504ef80c4c88726ccf775e3a5c0b0143f71b`. Harness: mGBA 0.10.3 two-core harness from `tools/coop_link_harness` (final suite at `1b26b863`; the pace fix in `c59a587e` and its reruns below).

Words used here: **gameplay-tested** = played in the two-core harness; **source/validator** = checked by reading code or by the generators' checks, not played; **fixture** = state set by the test, not reached by play; **test helper** = Pokémon, items or healing given by the test; **driver** = the harness choosing inputs.

## Automated validation (all on the final tree)

| Check | Result |
| --- | --- |
| `tools/coop_trainers/guide.py --check` (the single team spec from `battle_plan.json`) | 189 cards (252 trainer ids), 57 selection rules, 0 errors: species, gender, abilities, Mega stone and ability, every move legal at its level (level-up, egg, teachable), evolution levels, caps, one Mega per trainer, the 57 gym trainers on their gym maps, rival routing. Every forced change is listed in `docs/COOP_TEAM_SPEC.md` (Substitutions). Re-running the generator leaves the files unchanged |
| `tools/coop_trainers/validate.py` | 0 errors, 1 warning (Brenden's Crabrawler in Dewford Gym is above its evolution stage's usual level; kept from the guide) |
| `tools/coop_mega/rewards.py --check` | 83 Mega forms of obtainable species covered by 82 stones (+ Rayquaza by Dragon Ascent); Mega base HP equal to the base form where it differed; 84 item balls (53 stones, 31 items), each tile reachable from a real entrance, not on the shortest way between two entrances, not within four tiles of an edge into another map, not next to a warp, trigger or person, and not cutting the walkable area; 0 errors |
| `tools/coop_exchange/stock.py --check` | stock ok (368 items, 91 tutor moves, 49 evolution items); Mega Stones only at the MEGA STONE counter |
| `tools/coop_roster_audit/roster.py --check` | 0 issues (900/900 roster species obtainable; Birch's bag follows the new case starters) |
| `tools/coop_protocol_test.c` (host build) | all passed (the invite now carries the 16-bit team key) |
| `tools/coop_text_width.py` on the new and changed scripts | fits; one older overflow in `Coop_Text_LegendConflict` fixed |

## Final suite (this ROM)

`run_all.sh` with `LONG=1`, `SEG6`, `J06 J13 J16 J37 J43 J44 J46 J48 J50 J51 J52 J57 J58 J59 J65 J66 J68` and `OLD13C_COAST` (output `/home/claude/journey/final13D/suite`): **241 scenarios, 237 PASS, 4 FAIL** on the first run; 3 visual-check flags.

| First-run failure | Cause | After |
| --- | --- | --- |
| `breeding_coop` | Hit the 200,000-frame budget: Kai's egg came after 11,263 steps (one random roll per 256 steps; earlier runs 511 and 1,535) | `run_all.sh` gives it 400,000 frames (`c59a587e`); rerun PASS |
| `breeding_rules` (+ `_reload`, which uses its saves) | Harness: Rosa's pace stopped mid-step and the next START (save) was lost, so the menu cursor never moved. Reproduced with `RTC_EPOCH` set near the suite's start time (the unmodified scenario failed in 3 of 6 runs) | Harness `pace_until_*` now waits for the player to stand still (`c59a587e`); 4/4 PASS at the failing clock values; suite-style rerun PASS (+ reload PASS) |
| `roster_b3_catch` | Rare-slot hunt timed out (known harness weak point; also failed in the 0xC017013C suite) | rerun below |

Visual-check flags, all known false positives: `finding16_wild_teleport_karen` and `coop_battle_items` "no-blank-name" (the healthbox name of the absent fourth battler, as in every suite since 0xC0170137); `roster_b3_catch` "no-phantom" (a Safari Zone battle, where the player has no Pokémon out).

Reruns with the `c59a587e` harness on the same ROM (`/home/claude/journey/final13D/harness_rerun`): every scenario that uses `pace_until_flag`/`pace_until_mem`: `invite_cancel` PASS, `breeding_coop` PASS, `breeding_rules` PASS, `breeding_rules_reload` PASS, `roster_breeding` PASS, `roster_b3_evo_misc` PASS; `roster_b3_catch` FAIL again: Rosa's Galarian Stunfisk (found after 184 encounters) used Bounce after the first ball; with the target in the air the game refuses the throw and the driver stayed in the Bag until the hunt timed out (driver weak point; the same hunt failed in the 0xC017013C suite). The other 230 results are from the `1b26b863` harness, whose only later change is that pace behaviour.

## Before the final suite (development runs, kept for the record)

A first full run on an earlier ROM of this build (`/home/claude/journey/pre13D`, 216 scenarios) had 17 failures; every one was traced:

- The rescue companion is now Fennekin (Scratch, Tail Whip, Ember): `target_navigation`, `spread_selection` and `coop_six` pressed the old move slots. Scenarios updated.
- `connected_maps` walked into a new item ball on Route 102 at (2,4). That ball and 19 others stood near map edges or on the main way; `place.py` gained both rules and the 20 moved (same flags and object order).
- Flow tests whose cached-stat fixtures lose to the guide's teams once a level-up recalculates the stats: `gym_teams` (Juan and Wallace), `rival_late` (Route 110 twins), `story_seg3` (Petalburg Woods grunt; seg4-6 and `catch_walk` depend on it), `coop_wally_vr_partner` (Wally), `twins_coop` (+ reload; exhibition). They now use `strong_battlers` or 999-HP fixtures, labelled in each file. These are flow tests, not balance results (see Balance below).
- `breeding_rules` and `coop_bag_battle_tasks`: intermittent (the pace issue above; eight balls without a catch). Both passed on rerun.

New-scenario first runs that failed and why: `items_personal` (the placement check counted the Terra Cave entrance as a way into Route 116's east pocket: fixed in `place.py`; the pocket is reached through Rusturf Tunnel), `mega_exchange` (one B after buying), `boss_pair_coop` (the partner's figure is solid; Rosa walks round), `mega_za_abilities` and `mega_cancel` (scenario deadlock, a two-turn move, a test Pokémon fainting), `exh_case_grass` (fixture HP), `quest_stone_catchup` (first pointed at saves that already had the stone; then the stone was announced before the bag check, fixed in `coop_mega.inc`), `space_center_solo` (the stock choose-three screen; the driver cannot pick a replacement there).

## Battle Guide v2 checks (gameplay-tested, final suite)

| Scenario | Saves | Result | What was checked |
| --- | --- | --- | --- |
| `guide_team_select` | lab | PASS | 27 selections through the ROM's own `Coop_TeamPartyIdForKey` (read-only test hook): Lilycove and hideout early variants by tier, exhibitions by case, rematch tiers stepped down to the cap, Gabby and Ty by tier |
| `rival_case_grass`, `_fire`, `_water` | lab, FIXTURE `VAR_STARTER_MON` | PASS ×3 | Route 103 co-op: Soren (opponent A, ids 866-868) leads Torchic / Mudkip / Treecko and Mira (B) Fennekin / Totodile / Chikorita, the type that beats the case; Kai talks in two, Rosa in one; win, story flag on both |
| `rival_lilycove_early` | j50 (5 badges, Devon Scope: Route 120 open before Winona) | PASS | Twins' early teams (slot levels 44/44/46, no stone) |
| `rival_lilycove_late` | j51 (6 badges) | PASS | Regular teams (50-52); Soren's Mega Swampert and Mira's Z-A Mega Feraligatr Mega Evolve |
| `exh_case_grass`, `_fire`, `_water` | seg6, FIXTURES | PASS ×3 | Exhibition aces Blaziken+Delphox / Swampert+Feraligatr / Sceptile+Meganium by the inviter's case; co-op win recorded |
| `boss_pair_solo` | lab, not linked | PASS | Talking to HEATH (beside Roxanne): his line, then the same solo double against Roxanne and Heath; his own after-words |
| `boss_pair_coop` | lab, linked | PASS | Kai talks to Heath: co-op battle for both, same prize on both saves; Rosa then gets Heath's after-words |
| `caps_league` | j66 | PASS | Cap 66 before the door guards (Rare Candy on a Lv 75 refused, kept); the guards' real script sets admission; cap 76 (one candy to 76, the next refused) |
| `caps_champion` | j68 | PASS | Cap 80 |
| `mega_bracelet` (+ reload) | j46 | PASS | Steven's Route 118 scene gives each save its own bracelet; power-on keeps both |
| `mega_exchange` | j57 | PASS | Exchange clerk's bracelet for a save that passed the scene earlier (once); 12 stones from 5 badges; a purchase; WHERE TO FIND; a found stone added to the stock |
| `mega_coop_tate` | j57, test helpers | PASS | Both players Mega Evolve the same turn; Z-A Mega Chimecho and Mega Gallade on the enemy side, one per trainer; nothing rejected; afterwards each stored Pokémon is its base species with the same personality, moves, EXP, held stone |
| `mega_cancel` | j57 | PASS | Guest turns the trigger on, picks a move, presses B at "Ready. Waiting": the menu reopens with the trigger off; no Mega that turn; next turn both Mega Evolve |
| `mega_abort` | j57 | PASS (KIND: recovery) | Link error flag set after both Megas: battle ends, no badge, link and Beacon up, base species, identity, moves, EXP and held stones kept |
| `mega_no_bracelet` | j57 | PASS | Rosa without a bracelet presses START: no Mega, nothing rejected; Kai's works |
| `mega_za_abilities` | j57 | PASS | Mega Meganium has MEGA SOL (Solar Beam charged and hit in the same turn, from the battle log), Mega Feraligatr DRAGONIZE |
| `mega_hp_same` | j57 | PASS | Mega Banette (160/160) and Mega Chimecho (172/172; the enemy's 192/192) keep the base form's max HP; party max HP the same before, during and after |
| `space_center_solo` | j58, not linked | PASS (FIXTURE `strong_battlers`) | Real path through the Space Center; stock choose-three; Steven's Skarmory beside the player; flag 0x75 |
| `items_personal` (+ reload) | j16 | PASS | Both at the Route 116 GREAT BALL on one frame: each save gets one, own flag; Rosa's full ITEMS pocket leaves the STARAPTITE ball (flag clear), then she takes it; Kai takes his own; power-on |
| `quest_stone_catchup` | 0xC017013C `coast_solo_j68` saves | PASS | A quest finished before Mega Stones: full bag "come back for it", nothing set; then DRAGALGITE once |

Existing scenarios that meet the guide's teams and pass in the final suite: `gym_teams` (Roxanne+Heath, Norman+Hana, Juan+Wallace), `rival_late` (Route 110; Lilycove at tier 1 = early team), `rival_story`, `rival_p1`, `solo_rival`, `solo_gym`, `story_seg1-6`, `twins_coop`, `twins_solo`, `coop_wally_vr_partner`, `revamp_trainers_early/mid/late`, `e4_sidney`.

## Balance (no claim of balance)

Unassisted runs with the main journey's own parties (driver: `smart_moves`; no fixtures):

| Fight | Players' teams | Result |
| --- | --- | --- |
| Petalburg Woods grunt, co-op (j03) | Kai Torchic Lv 12 + Poochyena; Rosa Houndour Lv 10 + Wurmple | **Lost.** Carvanha (Lv 13, Rough Skin, Aqua Jet first) knocked out Houndour on turn 1 and was never damaged |
| Rustboro Gym trainers (j06) | Kai Combusken Lv 16, Buizel 12, Poochyena 5; Rosa led by Marill Lv 13 | Josh (co-op) won; Tommy won by Kai alone with 9 HP left; **lost** to Marc before reaching Roxanne |
| Dewford Gym (j27) | levels 15-22 | Beat Edmond & Hailey together; the old journey script then walked into Reed's new tile (3,3), so the leader fight was not reached: inconclusive |
| Flannery + Cole (j40) | Lv 45 trios raised by an earlier test helper, early moves only | **Lost** (Scovillain, Ninetales and Houndoom) |
| Maxie + Tabitha with Steven, solo (j58) | Lv 65 teams from earlier helpers | Tabitha's Mega Scovillain knocked out Steven's Metagross and both players' leads; the driver then could not choose a replacement: inconclusive |
| Wally, co-op (j65) | Lv 80 trios from earlier helpers, early moves | **Lost** |

Reading: the journey parties are weaker than the guide assumes (low levels early; level-boosted Pokémon with first-route moves later), so these are not a fair test of the guide's intended parties either. What they do show is that the guide's teams are much harder than the previous teams for the same parties, from the first Aqua grunt on. No balance claim is made; a run with parties built to the caps (levels near the cap, moves and items available by then) is still needed.

## Source-inspected or validator-only (not played)

- Z-A abilities other than MEGA SOL and DRAGONIZE: SPICY SPRAY, EELEVATE, PIERCING DRILL, STALWART, Mega Starmie's HUGE POWER (the expansion's code paths read; the abilities are assigned by the species data).
- The other 52 stone balls and 30 item balls (placement validator); three other quest stones (same script shape as the coast one).
- Boss pairs other than Heath (11 more NPCs placed; scripts share one shape); the Elite Four and Champion pairs in play.
- Gabby and Ty, rematch tiers and the hideout early variants in battle (selection function tested only).
- 54 of the 57 gym trainers' teams in battle (validator; Rustboro's three met in play).
- AI coordination, friendly fire and Protect loops: not assessed.
- mGBA GUI, real hardware and two-PC netplay: not tested.

## Clips (two screens side by side, this ROM)

| File | Content | Fixtures in it |
| --- | --- | --- |
| `clip1_simultaneous_megas_C017013D.mp4` (1:29) | Co-op vs Tate & Liza: Rosa and Kai press START; Mega Gardevoir and Mega Blaziken the same turn; Liza's Z-A Mega Chimecho; Tate's Mega Gallade | test-helper Lv 54 Blaziken/Gardevoir with stones and bracelets; warp; `strong_battlers` after the Megas |
| `clip2_rival_lilycove_za_ace_C017013D.mp4` (1:13) | Lilycove at 6 badges: Soren (case line) and Mira (companion line); Soren's Mega Swampert; Mira's Z-A Mega Feraligatr | j51 parties (raised by earlier helpers); warp; strong stats and 999 HP on the leads |
| `clip3_rival_branch_route103_C017013D.mp4` (0:34) | Route 103, Grass case: the twins use Fire (Soren's Torchic, Mira's Fennekin); the win | lab saves with the case set by fixture; strong leads |
| `clip4_boss_pair_heath_C017013D.mp4` (1:04) | Rustboro Gym: Heath beside Roxanne; Kai talks to Heath; one co-op battle on both screens; won; Heath's own after-words | lab saves; warp; strong leads |
| `clip5_personal_item_balls_C017013D.mp4` (0:46) | Route 116: both take the same GREAT BALL (one each); Rosa's full bag leaves the STARAPTITE ball, then each takes their own; both save | warp; Rosa's ITEMS pocket filled by fixture |
