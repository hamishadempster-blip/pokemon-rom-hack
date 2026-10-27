# Test log, build 0xC0170136

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). Source commit `b69fc2e6`.
- ROM `emerald-coop-C0170136.gba` (32 MB): SHA-1 `2d0903031e252067aaf9b51c4199e015fd4a3ee8`, SHA-256 `5564b99ff36658781f82ccaf87f12c707d75ccf4c0375287f967a3a94dfdf169`.
- Changes since 0xC0170135: partner INDICATOR / NAME / OFF; shared legendaries (CoopSave layout 3, migration); fixes #27-#31 (handoff §3). Both players need this build.

## Source and data checks (no gameplay)

| Check | Result |
| --- | --- |
| ROM build (`make`) | Clean |
| `tools/coop_protocol_test.c` (host compiler) | 131 checks, all passed (legend field no longer merged or conflicted) |
| Legendary encounter scripts (Sky Pillar, Terra/Marine Cave, Desert Ruins, Island Cave, Ancient Tomb) | Read: each calls the shared encounter; caught / got away / lost paths; Kyogre, Regice, Registeel not played |
| Old-build bisect for `pp_coop` (0xC0170133 driver) | 0xC0170133: guest frozen (2 failures). 2004e0f6 alone: PASS. 0xC0170136: PASS |

## Gameplay: main journey (no fixtures)

`journey_70_shared_rayquaza` from checkpoint j68: Fly to Mossdeep, the Mart (10 ULTRA BALLs, 30 TIMER BALLs each, own money), Surf to the Sky Pillar, Mach Bike climb, the shared battle, catch, delivery, back to 5F, save.

| Run | ROM | Result |
| --- | --- | --- |
| 1-4 | 0xC0170136 before fix #29 | Both games stopped on Rayquaza's HYPER VOICE (move animation never ended; nine copies of the session task) |
| 5 | before fix #31 (fog) | PASS (Rosa's ULTRA BALL) |
| 6-11 | delivered | No catch (catch rate 3): Rayquaza knocked out 3x (flies off, nothing granted, back next visit), the pair lost 2x (whiteout, nothing granted, records unchanged), 1 driver stop (no PP) |
| 12 | delivered | **PASS**: Rosa's TIMER BALL after 26 throws; both own one Rayquaza (own OT, met Sky Pillar Lv70), Pokédex caught, claimed 1 / pending 0 / owed 0, one session task, both back on 5F, saved (checkpoint j70). Kai's reward identical (personality, IVs) to run 5's: no reroll |


## Gameplay: new regressions (two linked cores; fixtures named in each scenario)

| Scenario | Saves | What it checks | Result |
| --- | --- | --- | --- |
| `legend_shared_ray` | j68 | Kai starts, both agree, Kai's ball catches: one each, own OT, Pokédex, records, one session task | PASS |
| `legend_ray_rosa` | j68 | Rosa (link guest) starts and catches; her MASTER BALL used, Kai's bag untouched | PASS |
| `legend_ray_solo` | j68 | Unlinked stock battle; linking afterwards gives the partner nothing | PASS |
| `legend_ray_claimed` | j68 | Partner's record already claimed: refused without a prompt; Kai alone; Rosa's own Rayquaza just leaves | PASS |
| `legend_ray_story` | j68 | Partner's world not ready (Sky Pillar state 1): refused, her state untouched | PASS |
| `legend_ray_simul` | j68 | Both press A on the same frame: one battle, one each | PASS |
| `legend_ray_ko` | j68 | Knockout: nothing granted, it flies off on both screens, back next visit; retried and caught | PASS |
| `legend_ray_full` + `legend_ray_resume` | j68 | Party and PC full: pending, saved, ACKed; after reload still pending; room made: delivered once to the PC; relink adds nothing | PASS |
| `legend_ray_interrupt` + `legend_ray_reconnect` | j68 | Partner drops deal messages: catcher keeps the debt, "arrives later"; next link: delivered | PASS |
| `legend_ray_lostack` + `legend_ray_reconnect2` | j68 | Catcher drops the ACK: partner delivered, catcher's debt kept; next link: confirmed, no second Rayquaza | PASS |
| `legend_groudon` | j68 | Groudon shared in Terra Cave; weather ends on both | PASS |
| `legend_regirock` | j68 (puzzle flag fixture) | Regirock shared, Lv40 | PASS |
| `legend_migration` | j68 | Old record PARTNER -> pending, delivered once; LOCAL -> claimed, nothing delivered | PASS |
| `coop_bag_battle_tasks` | j68 | Six throws in a co-op battle: at most one session task | PASS (0xC0170135: FAIL, 9 tasks) |
| `indicator_tour` | j68, night and day clocks | INDICATOR through town, connection, door both ways, bike, dark cave | PASS (night, day) |
| `nametag` | SEG6 | INDICATOR default; NAME, OFF, INDICATOR; menus; Centre; unlink | PASS |
| `outfit_same_base`, `outfit_bases` (updated) | lab, SEG6 | The indicator marks the partner whatever the outfits; OFF at the dresser kept after reload | PASS |
| `coop_partner_mach_bike` (+ session task check) | SEG6 | One session task after the Bag while linked | PASS |
| `diag_master_move` | j68 | Plain A presses choose and confirm moves on both screens in the one-foe double | PASS |


## Gameplay: full suite and kept regressions on the final ROM

`run_all.sh` with `LONG=1`, `SEG6` (story_seg6), `J65`, `J68`: 134 scenarios.

| Result | Count |
| --- | --- |
| PASS | 131 (includes `pp_coop`, `centre_link`, `center_beacon`, the `beacon_*` and `*_coop` quests, `coop_one_trainer_faints`, `rival_simultaneous`, `coop_wally_vr_partner`, the deal and reload chains) |
| FAIL | 3: `outfit_same_base`, `outfit_same_base_reload` (expected the old AUTO marker; scenarios updated, rerun PASS); `wild_simultaneous` (once, with three harness lanes running: the encounters did not coincide; rerun twice alone: PASS) |
| Visual-check failures | 1: `caravan_coop` send-out order (the checker paired lines 1100 frames apart; both screens show Dune then Mesa); rerun: visual ok |

Kept regressions run separately (`final136/extra.sh`): `coop_host_replacement`, `coop_host_two_replacements`, `coop_host_team_down`, `coop_partner_wiped`, `coop_partner_wiped_wild`, `same_trainer_spot`, `same_trainer_spot_late` (j06), `partner_busy_trainer` (j16): PASS. `spot_after_coop_wild` (j16, the #16 diagnostic): FAIL twice at different points: Hiker Clark spots the players on its walk first, so its setup no longer holds; no assert screen; inconclusive.

`pp_coop` with the 0xC0170133 driver: 0xC0170133 FAIL (guest frozen in `Controller_DoMoveAnimation`), 2004e0f6 alone PASS, 0xC0170136 PASS.


## Checkpoint saves

CONTINUE to the field on both games: j62, j63, j64, j65, j66, j67c, j67d, j68, j70 (the new checkpoint): PASS. `journey_69_post_clear_again` (j68): PASS.

## Recordings (two screens side by side; not committed)

| File | Content |
| --- | --- |
| `shared_rayquaza_journey70.mp4` (3:12) | Journey 70, no fixtures: arrival at the top, resonance, both YES, the battle (at 8x), Rosa's catch, the caught note, Kai's delivery, the recorded note, both walking down to 5F, saving |
| `shared_rayquaza_fixture.mp4` (2:09) | `rec_legend_ray`: the same flow with a MASTER BALL and warp fixtures (Kai catches; Rosa receives hers), back down to 5F |
| `indicator_night.mp4`, `indicator_day.mp4` (about 1:05 each) | `indicator_tour` at night and by day |
| `partner_mark_menu.mp4` (0:53) | `nametag`: INDICATOR, NAME, OFF, INDICATOR from the CO-OP menu, status window, Centre, unlink |

