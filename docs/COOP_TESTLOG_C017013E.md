# Test log: build 0xC017013E (stability, team preparation, Duo Tower)

ROM `emerald-coop-C017013E.gba`, built from `60aee4f6` (no game-code change after it; later commits are harness, scenarios and docs). SHA-1 `096ffa376ca81c35b12cfc4636f7956814ffbb86`, SHA-256 `e1a43599a03bebc7a6ec4ef512933ded00dffcf0e3244b8471ed3639a408b52f`. Harness: mGBA 0.10.3 two-core harness from `tools/coop_link_harness` at `95b57eff` for the final suite and `94dba5fe` for the affected-subset run (below).

Words used here: **gameplay-tested** = played in the two-core harness; **source-inspected** = checked by reading code, not played; **fixture** = state set by the test, not reached by play; **test helper** = Pokémon, items or healing given by the test; **driver** = the harness choosing inputs (`smart_moves`, `weak_foes`, `strong_battlers` are driver aids or fixtures and are labelled in each scenario). Harness success is not evidence for the mGBA GUI, real hardware or two-PC netplay, none of which were tested.

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

## Final suite (this ROM and harness)

`run_all.sh` with `LONG=1`, `SEG6`, `J06 J13 J16 J37 J43 J44 J46 J48 J50 J51 J52 J57 J58 J59 J65 J66 J68`, `OLD13C_COAST`; ROM from `60aee4f6`, harness and scenarios at `95b57eff` (output `/home/claude/journey/final13E/pass3`): **281 scenarios, 279 PASS, 2 FAIL on the first run; 0 visual-check flags.** 11 of the 281 are deliberate fault/recovery tests (all PASS).

| First-run failure | Cause | Reruns and fix |
| --- | --- | --- |
| `breeding_coop` | Driver: Kai's one step to hatch the egg was blocked by the wandering Route 117 girl (she stood below him); the A that follows talked to her ("…that I didn't teach it."), so the egg never hatched in the 900-frame window | Same ROM and harness: 3/3 PASS. Scenario fix `94dba5fe`: the step goes up onto the partner's tile (the partner never blocks; the girl cannot reach it); PASS in the affected-subset run |
| `legend_ray_ko` | Driver: while Rayquaza was in the air (Fly) a turn took longer than the 20-frame change grace, and the `catch` driver's B (meant to close messages) landed on the co-op "Ready. Waiting… B: change your choice" note on both games, every turn (283 "Changing your choice" notes) until the 30,000-frame budget ran out. The game did what B asks; no game defect | Same ROM and harness: 2 of 3 PASS, 1 FAIL the same way (intermittent). Harness fix `94dba5fe`: `catch`/`hunt` never press B while that note is up. Then 5/5 PASS (affected run + 4 reruns), 0 "Changing your choice" notes |

**Affected-subset run after `94dba5fe`** (harness and scenarios only; same ROM; `/home/claude/journey/final13E/affected`): every `run_all` scenario that uses `catch` or `hunt`, `breeding_coop`, and the reloads that use their saves: **29 scenarios, 26 PASS, 3 FAIL**: `roster_b3_catch` and `roster_b1_play` (rare-slot hunts that timed out: the wanted species did not appear; no waiting note was involved, so the change did not touch them; both passed in the final suite) and `roster_b1_reload` (uses `roster_b1_play`'s saves). Reruns: 3/3 PASS.

Which results cover which revision: 252 scenarios are covered by the final suite at harness `95b57eff` (no later change touches them); the 29 affected scenarios by the `94dba5fe` run and its reruns. The ROM is the same for both (`60aee4f6`; a clean clone of `60aee4f6` builds the same SHA-1). This is not one uninterrupted green run of all 281: the two first-run failures above and the three affected-run failures are part of the record.

## Before the final suite (kept for the record)

Two earlier full runs on this build's code found test problems and three game defects; each was fixed before the final run.

**Pass 1** (`/home/claude/journey/final13E/pass1`, ROM from `e3c63803`, harness `c8a17759`): 281 scenarios, 276 PASS, 5 FAIL, 1 visual-check flag.

| Failure | Cause | Fix |
| --- | --- | --- |
| `coop_wally_vr_partner` | **Game:** assert screen "src/battle_message.c:3039: Opponent needs a valid name" on Rosa's game when Wally's Gallade, on the right-hand side, Mega Evolved; Kai's game printed "…is reacting to 's Mega Bracelet!" (empty name). A co-op fight against one trainer has no opponent B; the right-hand opponent's name was read from that empty slot | `60aee4f6`: both opposing battlers name opponent A when there is no opponent B. New read-only test hook `test_battle_name`; the scenario checks battlers 1 and 3 name WALLY on both games (on the old code battler 3 gives "" on both). Why only the partner's game asserted is not proven; 10 further runs did not put Gallade on the right again |
| `shoal_coop` (+ `_reload`, which uses its saves) | Test expected the pre-S8 rule (a clue found unlinked merged into the partner's save) | Expectations follow the ownership policy (`1f5291f7`); PASS, reload PASS |
| `duo_tower_behind` | Driver: `until_field_text` tapped A in a forced replacement and held the cursor on a fainted Pokémon | `until_field_text` uses the battle driver's keys in a battle (`5e5ecef9`); PASS |
| `duo_tower_set_reload` | Driver: an A mash after the loss talked to the desk again and began a new attempt (the result variable is cleared by a new attempt since S10) | B only after the loss message (`5e5ecef9`); PASS |
| `story_seg3` visual flag | `no-blank-name`: the empty healthbox of the absent fourth battler, redrawn after a move-learning screen at the battle's end (no "What will" prompt followed, so the S8 rule could not see it) | `visual_check.py` also accepts the blank when the same line shows that slot empty and flagged absent (a present battler with a blank name still fails: checked with a doctored log). Committed with `60aee4f6`, together with the harness's assert-screen dump of the battle attacker and positions |

**Pass 2** (`/home/claude/journey/final13E/pass2`, ROM from `60aee4f6`, harness `5e5ecef9`), stopped after 104 scenarios to start the final run on the fixed harness: 2 failures, both driver timing: `prep_slots456` (the DOWN on the team screen was not taken and A chose READY) and `revamp_solo_checks` (a critical U-turn knocked out the last wild Pokémon, so there was no pivot). Fixes (`95b57eff`): harness `menu_cursor N` moves the team-screen cursor until it is on the item; the wild Wurmple is Lv 20 (fixture). Both PASS on rerun, with `prep_cancel`, `prep_solo_lead`, `prep_win_j13`, `duo_tower_first`, `sustained_session`.

Earlier in this build's development: an interim full run after S2 (233 pass, 18 failures, all traced: stale scenarios for the new consent rules, the "not ready" script fall-through fixed in S9) and focused runs after each stage.

## Stage results (gameplay-tested in the final suite unless noted)

| Stage | Scenarios (all PASS in the final suite) | What they show |
| --- | --- | --- |
| §9 opening | `release_newgame_boys`, `_girls`, `catch_before_dex` | Real opening, names, van, two clocks set apart; Poké Balls in Oldale; a catch before the Pokédex is recorded |
| §2 C017013C fixes kept | `mandatory_busy_menu`, `_talk`, `_timeout`, `coop_held_items`, `coop_battle_items`, `coop_battle_items_wild`, `coop_secret_base_room`, `coop_boulder_personal`, `items_personal` (+ reload) | Busy partner never interrupted (menu, message, 30 s timeout); no item moves between the two players' battlers; personal rooms and boulders not shared |
| §2 finding #16 | `finding16_*`, `gen16_*` matrix (development, 24 runs) | 0 assertions, 0 bad-trainer diagnostics; not reproduced; not declared fixed |
| §3 relink policy | `relink_policy_setup`, `relink_policy`, `relink_policy_reload`, `rival_reconnect`, `pp_reconnect`, `pp_stagger` (+ reload), `shoal_coop` (+ reload), `quest_stone_catchup` | Joint outcomes restored only with the same partner (trainer id); solo badges, rival stages and quest clues stay on their save; reloads before and after |
| §4 availability, Beacon, timing | `story_consent`, `story_beacon_off`, `story_beacon_off_wally`, `story_cancel`, `story_unanswered`, `story_simultaneous`, `story_not_ready`, `invite_expiry_clock` (+ `_wall`), `invite_busy`, `invite_cancel` | Consent, Beacon OFF explained, cancel, no answer, simultaneous requests, no fighting Pokémon, expiry by the cartridge clock at emulated and wall-clock speed |
| §13 team preparation | `prep_win_j13` (+ reload), `prep_slots456`, `prep_cancel`, `prep_timeout`, `prep_link_stall`, `prep_solo_lead` | Both players choose (slots 4–6 included), commit only at a shared READY, cancel/timeout/stall start nothing, every Pokémon back in its slot with its results, solo lead choice |
| §7 visuals and timing | `battle_visual_wild_double`, `startup_timing_wild`, `startup_timing_gym` | All four battlers' displayed palettes match the unfaded ones; timings below |
| §8 menus | `appearance_menu`, `move_inspector` | APPEARANCE from the CO-OP menu (and nothing left behind); inspector states (var., sure, No PP, Disabled) |
| §10 field kit | `field_kit` | BOX LINK and HEALING VIAL, refusals in restricted places and during co-op transactions, no healing through storage |
| §6 FOLLOW | `partner_menu` | Through the Centre door and back, own steps counted once (Repel), stop on a button, no follow loop, stop before grass |
| §5 NPCs | `npc_town_order`, `npc_interior_talk`, `npc_reconnect` | One authority after leave/return and link loss, no jumps, busy NPC answer |
| §11 Duo Tower | `duo_tower_first`, `_set`, `_set_reload`, `_abort`, `_behind`, `_partners`, `_decline`, `duo_desks` | Pair clauses, same opponents on both games, BP once per set, loss ends the run, abort rolls back, saves that disagree, one partner per run, refused invite shown correctly, honest unavailable desks |
| §12 optional content | `opt_travel`, `opt_contest`, `opt_escape_dig`, `coop_wally_vr_partner` | Cable car and S.S. Tidal, a contest, Escape Rope and Dig while linked; one-trainer co-op fight names |
| §14 sustained session | `sustained_session` | See below |

## Battle start timing (§7)

Frames at 60 per second (normal speed); the harness reads them from `gCoopStartTimes` on both games (`startup_timing_wild`, `startup_timing_gym`, final suite). The partner answers and presses READY as soon as the driver can, so the question and the team screen are the driver's reaction time, not a person's; the team screen's time is listed separately and is intentional.

| Stage (§7) | Co-op wild double, before (S4 start) | after (final) | Co-op gym with team screen (final; no "before": the question and team screen are new) |
| --- | --- | --- | --- |
| 1. Encounter detection → request sent | same step (not separable) | same step | same step |
| 2. Invitation: request → partner's answer back (inviter) | 154 | 104 | 164 (to both team screens) |
| — partner's question on screen → YES (partner) | 134 | 86 | 146 |
| — team screen (intentional, driver presses READY at once) | — | — | 44 |
| 3./4. Launch → `CB2_InitBattle` (transition) | 90 | 90 | 140 |
| 3. Party/state exchange (`CB2_InitBattle` → battle main loop) | 226 | 226 | 224 |
| 5. Intro and send-outs (main loop → first action menu) | 860 | 878 | 1,318 |
| 6. Request → first action menu (total) | 1,330 (22.2 s) | 1,298 (21.6 s) | 1,890 (31.5 s), of which 44 team screen |

What changed: the join question is one page again (the partner's answer is 48 frames sooner at driver speed). The technical stages (transition, party exchange, link-battle intro exchanges) are the stock link-battle costs and were not shortened: no handshake, buffer check or readiness check was skipped. The intro stage varies by about ±20 frames between runs.

## Sustained session (§14)

`sustained_session` (j68, Battle Tower lobby, both games): the counts taken at the start (5 active tasks, 21 sprites, 8 sprite palette tags, 5 heap blocks / 11,648 bytes on each game) were identical at all seven checks: after 300 quiet frames, a cancelled team screen, a new duo run with two battles, a cancelled fixed-team screen, a third battle, the START and CO-OP menus with an outfit change and back, and FOLLOW round the lobby. The first version of this test found the wardrobe defect (sprite palettes 8 → 7 after APPEARANCE; fixed in S10); `appearance_menu` now checks the same counts around the wardrobe (fails on the old code: palettes 3 → 2).

## Party integrity

- Team screens: `prep_slots456` picks slots 6, 4, 5 and checks after the battle that slots 1-3 have the same personality, EXP and HP and slots 4-6 gained EXP, each in its own slot; `prep_win_j13` (+ reload) both players choose, EXP goes to the chosen Pokémon, a power cycle keeps it; `prep_cancel`, `prep_timeout`, `prep_link_stall`: nothing changes; `prep_solo_lead`: the chosen lead stays in front, nothing else moves.
- Duo Tower: `duo_tower_first`, `_set`, `_abort`, `_behind`, `_set_reload`, `sustained_session`: real parties byte for byte as before every battle (win, loss, link failure), money unchanged.
- Field kit: `field_kit` deposits a Pokémon at 5 HP and gets it back at 5 HP (no healing through storage).

## Clips (harness recordings, both screens side by side, normal speed unless the label says)

Recorded by the harness's `RECORD` option from the scenarios named; P1 is Kai (link player 0), P2 is Rosa. Builds: the first four from the S9 build (`69662143`), the last three from the S10 build (`e3c63803` ROM, harness `c8a17759`); later code changes do not touch what they show.

| File | Scenario | Shows |
| --- | --- | --- |
| `npc_reunion.mp4` | `npc_town_order` | Rustboro NPCs move the same on both screens; P1 goes into the Centre and back; the NPCs agree again without snapping |
| `follow_through_door.mp4` | `partner_menu` | FOLLOW chosen; P1 follows P2 into the Pokémon Centre and back out through the door |
| `battle_visuals_wild_double.mp4` | `battle_visual_wild_double` | A co-op wild double: both wild Pokémon lit on both screens through the battle (the dark left Pokémon before S4) |
| `busy_partner.mp4` | `mandatory_busy_menu` | P1 talks to Norman while P2 is in her bag: her bag stays open, P1 waits with a countdown; when she closes it she is asked, both pick teams, the fight starts |
| `team_from_slots_4_6.mp4` | `prep_slots456` | The team screen; P1 chooses slots 6, 4, 5; Zubat (slot 6) leads; a forced replacement offers the chosen team |
| `duo_tower_battle.mp4` | `duo_tower_first` | DUO DESK, the partner's question, the team screen with the species clause, a duo battle and its win (FIXTURE: foes held at 1 HP) |
| `duo_tower_set_bp.mp4` | `duo_tower_set` | Battle 7 (FIXTURE: run moved to 6 wins), fixed team, 3 BP once on each save, the partner asked again for battle 8 (FIXTURE: foes held at 1 HP) |

No clip shows a completed 7-battle set played from the start or any facility other than the Tower (none exists).

## Not covered

- Not played: berry planting and harvest, the Trick House, Illusion, Repel prompts around scripts and transitions, joined conversations with mismatched progress, postgame rematches and League returns with mismatched progress, team-screen battles with Mega Evolution or move learning from slots 4–6, the old-save remembered-selection case (no save before S3 has a remembered team).
- §5's explanation line for differing personal dialogue and §8's visual refinements (rival art, party/bag/battle menu restyling, type colour accents) were not implemented.
- §11: six facilities have no duo mode (the desks say so); no Mega-disabled bracket.
- mGBA GUI by a person at normal speed, real hardware and two-PC netplay: not tested. Clips are harness recordings.
- Balance: not assessed (driver aids and fixtures in every duo battle; see each scenario's header).
