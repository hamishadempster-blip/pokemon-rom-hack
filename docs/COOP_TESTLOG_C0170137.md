# Test log, build 0xC0170137

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). ROM source commit `c880f0cb`; this log and the handoff are in the commit after it (docs and one test scenario only; the ROM is unchanged by them).
- ROM `emerald-coop-C0170137.gba` (32 MB): SHA-1 `c1971178c7aee0beefc253fa171bddc10fe382bc`, SHA-256 `162b51c75ae75cf051575b2eb4af1fc9922d6ba8bf231d0a588cf242782a00c7`. ROM space used: 26,967,124 of 33,554,432 bytes (80.4%).
- Commits since 0xC0170136: stability `38ab7398`, synchronization `65549f03`, transitions `abc6a576`, roster `45493603`, balancing `99a97e65`, build id `c880f0cb`.
- All gameplay is the two-core harness on libmgba 0.10.3 (`tools/coop_link_harness`), p0 = Kai (link master), p1 = Rosa. Fixtures and test helpers are named in each scenario's header and logged as `TEST HELPER` / `DRIVER`.

## Earlier results, restated

- 0xC0170135 final suite: **117 of 118 passed**; the failure (`deal_battle_doubles`) was a driver limitation, fixed in the harness, and that scenario plus four related ones then passed on rerun.
- 0xC0170136 final suite: 134 scenarios, 131 passed; 3 failed and passed on rerun (two outfit scenarios whose expectations were updated; `wild_simultaneous` once under load).

## Source and data checks (no gameplay)

| Check | Result |
| --- | --- |
| ROM build | Clean; the final rebuild reproduces the tested ROM (same SHA-1) |
| `tools/coop_protocol_test.c` (host compiler) | 131 checks, all passed |
| `tools/coop_roster/audit.py` (batch 1, sources) | Before the batch 0 of 114 obtainable; after 114 of 114 (45 direct: 33 in wild tables + 12 partner Pokémon; 69 by evolution). 0 were already obtainable; 0 repaired |
| `tools/coop_roster_audit/audit.py` (built ROM) | 328 species (all added species and their lines): 0 issues; every revamp in `revamps.py` (32 entries, Cherrim's two forms separately; pre-evolution ability slots included) matches the ROM; every evolution item has a source |
| Evolution supplies | Fire, Thunder, Shiny, Dusk Stone, Linking Cord, Razor Claw: all sold (Mauville POWER SHOP, Lilycove Dept. Store, local Marts); listed in `docs/COOP_ROSTER_BATCH1.md` |

## Stage 1: stability (focused gameplay)

| Test | Result |
| --- | --- |
| Finding #16 matrix (`matrix/gen16.sh`, 25 runs on j16: Kai, Rosa, both initiating; crossing into Karen's lane during the partner's encounter; declined invite; the recorded order: co-op wild Abra pair, Teleport, Karen at once; three timings) | 0 asserts, 0 refused launches, `gCoopDiag` empty. **Not reproduced; cause unknown.** Instrumentation kept (refusal + capture). Earlier evidence (journey 17b log): the partner's battle teardown was still running and the solo Karen battle had opponent id 0 |
| `pp_coop` timing matrix (`matrix/genpp.sh`: both roles, delayed choices, replacements, simultaneous faints, heartbeat loss, UNLINK/LINK UP before the fight) | Run 1 (ordinary battlers): 9 pass; 4 lost the fight with both games agreeing (not hangs); 1 scenario error (relink expected the old Beacon). Run 2 (strong battlers): 5/5 pass. No hang or divergence found; wait handling not changed |
| Harness reliability | cb2 history (one-frame callbacks visible), assert/unexpected reset = failure and stop, failed runs' saves quarantined (never promoted), recovery tests tagged `KIND: recovery` |

## Stage 2: shared world (focused gameplay)

| Scenario | Saves | Checks | Result |
| --- | --- | --- | --- |
| `npc_town_order` | j16 | Both arrival orders; authority hand-over when the first player leaves and returns; an NPC only one save has (different story progress) left alone; no phantom partner after a door | PASS |
| `npc_interior_talk` | j16 | Centre visitor agrees on both screens; talking reserves an NPC, the partner gets the "busy" answer, then can talk normally | PASS |
| `npc_reconnect` (recovery) | j16 | Heartbeat muted 10 s: each game runs its own NPCs; after the link mends one authority again and both agree | PASS |
| `scene_meteor_join` | j37 | Kai on the Meteor Falls trigger tile, Rosa nearby: she is taken to the scene and plays it on her save; actors removed on both | PASS |
| `scene_meteor_menu` | j37 | Partner in a menu is not pulled in; offer lapses after 10 s, she walks in herself | PASS |

## Stage 3: transitions and battle items (focused gameplay)

| Scenario | Saves | Checks | Result |
| --- | --- | --- | --- |
| `coop_battle_items` | j16 | Co-op trainer battle: each player's own Potion, Revive on a benched Pokémon, Full Heal, X Attack; bags and both saves checked | PASS |
| `coop_battle_items_wild` | j16 | Both use a Potion in the same turn of a co-op wild battle | PASS |
| `transitions_tour` | seg6 | Centre door and nurse (staggered), START menu, SAVE, Mach Bike, FLY, a solo battle on another map, a co-op battle, UNLINK, LINK UP (new session, Beacon agreed again), co-op battle | PASS |

## Stage 4: roster (focused gameplay)

| Scenario | Saves | Checks | Result |
| --- | --- | --- | --- |
| `roster_b1_evolutions` | j65 (Kai midday, Rosa 23:00) | Staravia (level), Arcanine (Fire Stone), Trevenant (Linking Cord, no trade), Tsareena (learns Stomp, then evolves), Mamoswine (Ancient Power), Sneasel + Razor Claw at midday: no effect; Weavile at night, Vikavolt inside New Mauville, Gastrodon East, Gourgeist Super, Cinccino (Shiny Stone) — all through the Bag, party menu and evolution scene | PASS (11 checks) |
| `roster_b1_play` | seg6 | Catch Minccino and Venonat in Route 116 grass (normal encounters), trade them, Kanto partner Pokémon to the party (Kai) and to the PC with a full party (Rosa), refused a second time that day, save | PASS |
| `roster_b1_reload` | from `roster_b1_play` | After power-on: traded catches, party gift and PC gift present, intact, Pokédex caught, no copies | PASS |
| `roster_b1_gate` | seg6 | Sinnoh partner with one badge: "not yet", nothing given | PASS |
| `late_wild_tour` | j52 (random encounters on) | Route 121 grass: two battles in a row, Drifloon caught, a run away, Abra's Teleport (switches out, Gen 8+ rule), Lilycove Centre then another grass battle, Meteor Falls cave battle; Rosa: Surf battle onto Route 122, Super Rod bite | PASS (3 runs after the driver fixes below) |

Driver fixes found here (not game bugs): pacing on water headed back to land; a PokéNav Match Call held the field while the driver pressed only directions (now answered); `fish` pressed A during the dots (reels in nothing).

## Stage 5: revamps (focused gameplay)

| Scenario | Saves | Checks | Result |
| --- | --- | --- | --- |
| `revamp_coop_support` | j16, co-op doubles | Butterfree's Pollen Puff and Hypno's Heal Pulse on the partner's Pokémon (both healed, both saves); Hypno's Telepathy vs ally Lanturn's Discharge ("It doesn't affect Hypno"); Cherrim + Sunny Day → Sunshine form on both games | PASS |
| `revamp_solo_checks` | j43, solo | Flower Gift form change; Beedrill's U-turn returns and switches | PASS |
| `revamp_owned_stats` + `_reload` | seg6 | A party Arbok and Parasect stored with old (too low) stats are recalculated on CONTINUE; same individuals, no copies | PASS |
| `revamp_trainers_early/mid/late` | j43, j48, j52 | Showcase trainers at route level (results in `docs/COOP_REVAMPS_C0170137.md`): all nine fights won, from no losses to 4 of 6 fainted | PASS (results recorded) |

Found while testing: Telepathy also blocks an ally's Pollen Puff heal. This is the official rule (Bulbapedia) and was kept; Hypno's first ability is Insomnia.

## Full regression on the final ROM and harness

`run_all.sh` with `LONG=1` (story chain), `SEG6`, `J16`, `J37`, `J43`, `J48`, `J52`, `J65`, `J68`: **158 scenarios, 157 PASS, 1 FAIL.**

| Result | Detail |
| --- | --- |
| FAIL: `connected_maps` | Kai's Lv 5 starter lost a solo wild battle (critical hit) that the scenario only uses to show "partner on another map"; the whiteout sent the walk off course and 4 later checks failed. Not a game fault. The scenario now raises battle stats for that one fight (TEST HELPER, logged); rerun twice on the same ROM and the suite's lab saves: PASS, PASS |
| Visual-check flags (2): `coop_battle_items`, `finding16_partner_wild_karen` | The checker saw a blank Pokémon name printed when the healthboxes are redrawn after the Bag; it belongs to the absent fourth battler. Screenshots of that moment (both screens) show three healthboxes and nothing blank: not visible. Not changed |
| PASS includes | `pp_coop`, `pp_reconnect`, `wild_simultaneous`, `center_beacon`, `deal_battle_doubles`, story segments 1-6, all legend, deal, quest and reload chains, the recovery tests (9), and every new scenario above |

## Clips (two screens side by side; this ROM)

| File | Content |
| --- | --- |
| `clip1_npc_sync_C0170137.mp4` (0:40) | `npc_town_order`: both in Rustboro, the walkers and turners moving the same on both screens, the partner marker |
| `clip2_unlink_relink_battle_C0170137.mp4` (1:39) | `transitions_tour`: after a co-op battle, UNLINK ("playing solo"), LINK UP, the Beacon agreed again, then a complete co-op wild battle |
| `clip3_new_pokemon_evolutions_C0170137.mp4` (1:02) | `roster_b1_evolutions`: Kai's Phantump + Linking Cord → Trevenant, Rosa's Shellos (East) → Gastrodon and others, through the Bag |

## Coverage gaps

- Finding #16 remains unreproduced; the validation refuses an invalid opponent and records it, but the cause is not known.
- NPC sync: same-frame talk by both players to one NPC was not produced (the busy case was tested with one player already talking); only Meteor Falls is a joinable trigger-tile scene; Birch's rescue, Mira/Soren, Wally and reward NPCs use their earlier co-op handling and were covered only by the existing story and rival regressions. Unsupported movement types (not synced): trainers, followers, copy-the-player and in-place movers.
- Roster: every distinct evolution method was played once; not every species was caught in play (encounter placements are source-checked by the audit). Starter guests for Lavaridge, Fortree and Mossdeep were checked by the gate test and source, not given in play.
- Revamps: Storm Drain, Water Absorb, Poison Touch, Stakeout and Technician were not exercised in play; Roland (Lanturn) and Tanya (Lumineon) were not fought. Realistic battles are single samples, not win rates; the driver picks the strongest damaging move, not a human strategy.
- Not tested: mGBA GUI, real hardware, netplay.
