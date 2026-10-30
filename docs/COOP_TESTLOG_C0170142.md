# Test log: build 0xC0170142 (collection update, second delivery)

- **ROM:** `emerald-coop-C0170142.gba`, built from game code at `6f18870e`. Later commits change only the Makefile (`bbe18c3e`, a dependency, same ROM), scenarios, the harness's scenario files and docs. A clean clone at `6f18870e` with the `bbe18c3e` Makefile builds the same SHA-1.
  - SHA-1 `fcbb6be8b1ec270091db9b5805ded3126599b1f2`
  - SHA-256 `4f7e6ee558d8657b5b4a2961d6294e9a15cd3c0530838329b03757206e9a85b6`
- **Build id:** 0xC0170142. It links only with 0xC0170142: the WAIT HERE deal is a new partner request kind.
- **Harness:** the two-core mGBA 0.10.3 harness from `tools/coop_link_harness`, built against this ELF. `arm_cut N signed` was added during this round; the harness binary that ran the final suite already had it.

Words used here are as in earlier logs:
- **gameplay-tested**: played in the harness.
- **source-inspected**: read, not played.
- **fixture**: state set by the test.
- **driver**: the harness choosing inputs.

Harness results are not evidence for the mGBA GUI, real hardware or two-PC netplay, none of which were tested.

## What this delivery contains (and what it does not)

| Section | Status |
| --- | --- |
| Autosave (requested with this round) | **Implemented and gameplay-tested**. Separate slots: an autosave never overwrites your newest manual SAVE; after CONTINUE from an autosave the game offers your last SAVE instead. 16 scenarios ([COOP_AUTOSAVE.md](COOP_AUTOSAVE.md)) |
| 8 Waiting at a Pokémon Center | **Implemented and gameplay-tested**: solo, reload, linked (together, in the same Center) |
| 2 Encounter variety | **Implemented and gameplay-tested**: 21 visitor kinds, 57 appearances on 26 maps, by time of day and weather; the Center REPORT |
| 11 "Who can learn this?" | **Implemented and gameplay-tested** (party and PC, full boxes) |
| 4 FIELD NOTES and rewards | **Implemented and gameplay-tested**: 10 rewards, pending queue, catch-up on older saves |
| 3 Five hidden habitats | **Implemented and gameplay-tested**: all five entered, found, fished or met; MOSS HOLLOW's completion reward |
| 12 Town specialities | **Implemented and gameplay-tested**: six specialists and their tracks in one tour |
| 9 Expedition HQ | **Not implemented.** The staged design is in [COOP_HQ_PLAN.md](COOP_HQ_PLAN.md). Its core is a shared room for two linked games, which needs a new transaction kind with recovery records and an ownership rule. Building that halfway would risk saves and the link, so it is left for its own stage. Its 256 save bytes stay reserved and zero |

Save layout: unchanged from 0xC0170141 (layout 5). The new features use space that was already reserved.

## Defects found and fixed while building it

| Defect | How found | Fix | Evidence |
| --- | --- | --- | --- |
| **FIELD NOTES announced old rewards on the first steps.** On a save that had reached rewards before this build, each map entry showed a reward message. Older-save scenarios stopped at it (`coop_boulder_personal`, several roster runs), and a real player updating would get a string of messages | First suite run on ROM 6e4f0f4d3bc1 | Rewards reached before loading wait in CO-OP > JOURNAL > FIELD NOTES, which asks to collect them. New Game clears this record | `notes_catchup` (+`_collect`, `_reload`) |
| **An autosave stored a stale map view.** After a SAVE in one place and an autosave in another, CONTINUE pasted the first place's tiles, and their collision, around the player | Independent code review; reproduced on ROM 0006c5686410 (lab floor drawn over Route 101: 11,174 of 13,440 pixels differed) | The autosave stores the view (`SaveMapView`), as the start menu's SAVE does | `autosave_mapview` (+`_reload`): 0 pixels differ |
| **New Game was asked the autosave question.** With an autosave newer than the last SAVE on the cartridge, New Game asked "This is your AUTOSAVE…". Answering could load the old file over the new game | Review; reproduced on ROM 0006c5686410 | New Game clears the power-on note | `autosave_new_game_over` |
| **A second power cut could combine two saves.** Writing into a slot left incomplete by an earlier cut kept the old leftovers. A second cut with a matching sector order could leave a slot that looked complete but mixed two saves. Rare: it needs two cuts and about a 1-in-14 match | Review (source); not reproduced | Before writing to an incomplete slot, the game clears one signature byte of each leftover sector (no erase) | `autosave_power_cut_twice` (+`_load`): ids 0–9 new, leftovers invalid, the manual save loads |
| **Visitors replaced fixed test tables.** PURRLOIN, Route 116's night visitor, replaced the species that older scenarios had fixed for Route 116, so `battle_cancel` failed | First suite run | Test fixture `gCoopVisitorTestChance` 255 (no visitors) in the 35 scenarios that fix that table, and in `matrix/gen16.sh` | `battle_cancel` PASS |
| **Clean-clone build failure.** The visitor table generator ran before `map_groups.h` existed | Clean-clone build check | Makefile dependency | The clean clone builds the same SHA-1 |
| FIELD NOTES counted the Pokédex once per reward, costing about a frame | Profiling during development | Counted once per check | `notes_rewards` |

Earlier fixes in development (habitat marker positions, the Town ball list's first entry, map id byte order in the visitor code, autosave right after boot) are in the feature commits and their docs.

## Expectation changes not caused by this round

- `prep_win_j13` (+`_reload`): a benched Pokémon now gains EXP. This comes from whole-party EXP (D1500001, merged before this round). The checks now expect a gain instead of no change.
- `coop_bag_battle_tasks`: the catch timeout was raised from 20,000 to 40,000 frames. The battle had ended just past 20,000 frames even on 0xC0170141, and whole-party EXP evolutions now follow it.

## Save compatibility (`compat_check.sh`)

On ROM fcbb6be8b1ec, the 0xC0170141 build and this one loaded the same 11 save pairs:
- checkpoints j06, j16, j29, j44, j57 and j68;
- seg6;
- the 0xC0170140 and 0xC0170141 lab saves;
- the 0xC0170141 duo tower save;
- the 0xC017013C coast save.

Each pair was compared field by field after loading, and again after a reload: **33/33 comparisons PASS**. Output is in `/home/claude/journey/compat142c`.

## Final suite (0xC0170142)

`run_all.sh` with `LONG=1`, `SEG6`, `J06 J13 J16 J29 J37 J43 J44 J46 J48 J50 J51 J52 J57 J58 J59 J65 J66 J68` and `OLD13C_COAST`. ROM fcbb6be8b1ec; the harness and scenarios are from a snapshot of `6f18870e`. Output: `/home/claude/journey/final142c/suite`.

**350 scenarios: 337 PASS and 13 FAIL on the first run; 0 visual-check flags.**

| First-run failure | Cause | What followed |
| --- | --- | --- |
| `roster_b2_gifts`, `roster_b3_evo_place`, `roster_b3_evo_misc`, `breeding_coop`, `late_wild_tour`, `habitat_tour`, `legend_ray_ko`, `roster_b3_catch`, `caravan_solo_j68`, `twins_solo_j68`, `pp_solo_j68` (11) | **Scenario.** The journey saves have 9 kinds caught. Each run caught, hatched, evolved or was given a 10th, and the next map entry correctly paid "FIELD NOTES: 10 kinds caught! … 5 Great Balls". The scenario did not expect the message and timed out | Payouts switched off in these runs with `patch_ret CoopNotes_Poll` (labelled FIXTURE). The payout itself is covered by `notes_rewards` and `notes_catchup`. Reruns on the same ROM: all PASS except `late_wild_tour` (next row) |
| `late_wild_tour` | As above, then on reruns **driver luck**: twice no DRIFLOON was met in the hunt's budget (a 4% slot, made rarer at night by Route 121's visitors), and once six Poké Balls missed | The hunt got room for about 300 encounters (`MAX_FRAMES=500000`), 12 throws, and no visitors during it (fixture). Reruns 2–4 with the old settings: 1 PASS, 2 FAIL. After the change: 3/3 PASS |
| `roster_repair` | Also stopped at the payout message; underneath, the **known harness `hunt` regression** from 0xC0170140 (the global frame limit is reached at the same commands, pc 45 and 21, as on 0xC0170141) | With the payout fixture: still FAIL, same cause. **Not fixed**: this is the harness driver, not the game |
| `coop_bag_battle_tasks` | **Driver timing.** Eight throws at RAYQUAZA, then the battle ran past the 20,000-frame catch budget; once, Kai was in an evolution scene when time ran out. The test's own checks (one co-op session task) were never reached | Budget raised to 40,000 frames. Reruns: first, before the change, FAIL (still in battle at the limit); then 2/2 PASS |

**Changed after the suite, and rerun on the same ROM.**
- The 12 scenario files above.
- `run_all.sh`, for `late_wild_tour`'s budget.
- `pp_solo`, `caravan_solo` and `twins_solo` also run on the seg6 saves. Those runs, and the reloads and `pp_stagger` that depend on them, were rerun with the change: see the rerun section below.

**This is not one clean run.** The first-run failures above are the record.

Earlier suite runs, on candidate ROMs 6e4f0f4d3bc1 and 0006c5686410, were stopped when the code changed. Their findings are the first, fifth and sixth defects above. Their output is kept in `final142_oldrom` and `final142b_stale`.

## Reruns on the seg6 saves after the scenario change

On ROM fcbb6be8b1ec with the changed scenarios, the seg6 runs and their dependants: `pp_solo`, `pp_stagger`, `pp_stagger_reload`, `caravan_solo`, `twins_solo`, `caravan_reload`, `twins_reload`. **7/7 PASS.** Output: `/home/claude/journey/final142c/rerun_seg6`. The j-save reruns are in `/home/claude/journey/final142c/rerun`.

## New regressions in the suite (PASS on the delivered ROM)

- **Autosave:** `autosave_basic`, `autosave_reload`, `autosave_load_manual` (+`_reload`), `autosave_go_back_autosave` (+`_reload`), `autosave_power_cut` (+`_load`), `autosave_power_cut_twice` (+`_load`), `autosave_off`, `autosave_coop`, `autosave_new_game`, `autosave_new_game_over`, `autosave_mapview` (+`_reload`).
- **Waiting at a Center:** `centre_wait_solo` (+`_reload`), `centre_wait_coop`.
- **Visitors:** `visitors_time`, `visitors_weather_fish`.
- **"Who can learn this?":** `learn_who`, `learn_who_full`.
- **FIELD NOTES:** `notes_rewards` (+`_reload`), `notes_catchup` (+`_collect`, `_reload`).
- **Habitats and towns:** `habitat_moss`, `habitat_tour`, `towns_tour`.

## Clips (harness recordings, both screens, normal speed)

Recorded on the delivered ROM to `/home/claude/journey/clips142`, with the scenarios from the suite snapshot. These are not mGBA GUI footage.

| File | Scenario (saves) | Shows |
| --- | --- | --- |
| `raw_autosave.mp4` | `autosave_basic` (lab) | "Autosaving... Don't turn off the power." on Route 101; no second save right away; another autosave later |
| `raw_centre_wait.mp4` | `centre_wait_solo` (lab) | CO-OP > WAIT HERE in a Center at night ("It's already NIGHT"), then a rest until MORNING, and outside in morning colours |
| `raw_learn.mp4` | `learn_who` (j52) | BAG > TMs & HMs > CAN LEARN: the count, party and PC list |
| `raw_notes.mp4` | `notes_catchup_collect` (suite saves) | Two map entries with no payout; FIELD NOTES asks "2 reward(s) to collect"; both rewards handed over; the notes page |
| `raw_habitat.mp4` | `habitat_moss` (j16) | MOSS HOLLOW's entrance in Petalburg Woods, the find, its own encounter, the marker, the completion reward, and the FIELD NOTES page |
| `raw_towns.mp4` | `towns_tour` (j68) | The six town specialists and their first rewards |

Looking at sample frames turned up one visual fault, **not new in this build**. On the CONTINUE screen, "Build C0170142" runs into long location names such as RUSTURF TUNNEL and LITTLEROOT TOWN. The 0xC0170141 clips show the same overlap. It is not fixed here, so that the delivered ROM stays the one tested.

## Not covered

- **Expedition HQ:** not implemented (above).
- **Autosave:**
  - Union Room, Cable Club and facility exclusions: source-inspected only.
  - An invitation sent during a partner's autosave: not tested.
  - Save-counter wraparound (it would need billions of saves): not handled, not tested.
- **FIELD NOTES:**
  - Not tested: NO at the collect question; New Game after loading a save (source-inspected); the larger thresholds; the visitor rewards; four waiting rewards.
- **Visitors:**
  - Each visitor's map and condition is checked by the generator, but only a sample was met in play: Route 101 at night and in the morning, Route 113 in falling ash, and a SUPER ROD on Route 122 at night.
- **Habitats:** only MOSS HOLLOW's completion reward was played; the other four are the same code with other data.
- **Towns:** the long tracks were played to their first reward only, with fixtures for counts.
- **Balance:** reward sizes and visitor rates were not tuned with unassisted play.
- **Earlier open issues, unchanged:**
  - finding #16;
  - the partner drawn on tiles blocked by the other save;
  - the intermittent `partner_menu` FOLLOW failure, and `wild_simultaneous` (both passed in this suite);
  - `roster_repair` (harness `hunt`).
- **Not tested at all:** the mGBA GUI by a person, real hardware, two-PC netplay.
