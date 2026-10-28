# Test log, build 0xC0170138

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). ROM source commit `69904380`; this log, the handoff and the checklist are in the commit after it (docs only; the ROM is unchanged by them).
- ROM `emerald-coop-C0170138.gba` (32 MB): SHA-1 `929fe33a5105517faabb480957d0978c532c00c5`, SHA-256 `97bd927b156845d91d17fbdd70c6ca5b2cb08e04755b806c8dcd8cbbb93bc3ae`. ROM space used: 26,987,296 of 33,554,432 bytes (80.4%).
- Commits since 0xC0170137: roster batch 2 and Pokédex `1337897b`, harness `b7d28cb4`, quests `a446e12f`, build id `69904380`.
- All gameplay is the two-core harness on libmgba 0.10.3 (`tools/coop_link_harness`), p0 = Kai (link master), p1 = Rosa. Fixtures and test helpers are named in each scenario's header and logged as `FIXTURE` / `TEST HELPER`. Where a Pokémon was made by a helper (to reach an evolution quickly), only the evolution itself counts as played.

## Source and data checks (no gameplay)

| Check | Result |
| --- | --- |
| ROM build | A clean rebuild (build directory removed) of `69904380` reproduces the tested ROM (same SHA-1) |
| `tools/coop_protocol_test.c` (host compiler) | 131 checks, all passed |
| `tools/coop_roster/audit.py batch2` on 0xC0170137's source | 0 of 114 obtainable before this build |
| `tools/coop_roster_audit/roster.py --check` (built ROM + sources) | 680 obtainable species (712 forms), 680 solo-completable, 0 issues; BATCH1 114/114, BATCH2 114/114; regional Pokédex 646 unique entries, 2 without a path (Jirachi, Deoxys: event-only); 387 AREA hints, none missing; only the legendaries are not renewable |
| Tide geometry (Shoal Cave layouts) | Every quest object and the rescue warp is on a dry tile at both tides; the exit is reachable from the rescue spot at both tides (by Surf at high tide) |
| Field guide width check (`tools/coop_field_guide.py`) | Passed |

## Roster and Pokédex (focused gameplay)

| Scenario | Saves | Checks | Result |
| --- | --- | --- | --- |
| `roster_b2_gifts` | j65 | Alola, Galar, Paldea guests (Rowlet, Scorbunny, Quaxly; daily flags; Pokédex); Fossil Maniac's Helix Fossil, once a day; DEVON's list and revival (Omanyte) | PASS |
| `roster_b2_evolutions` | j65 (Kai noon, Rosa 23:00) | 17 evolutions through the Bag, party menu and evolution scene: Eevee to Vaporeon, Leafeon, Glaceon (stones), Espeon by day and Umbreon at night (friendship), Sylveon (friendship and a Fairy move); Togetic, Togekiss, Clefairy, Clefable, Swoobat (friendship, stones); Musharna; Sawsbuck; Toxtricity (nature); Polteageist with each pot; Alcremie by spinning in place with a Sweet | PASS (pre-evolutions made by a logged helper) |
| `roster_b2_play` + `_reload` | seg6, linked | Lillipup and Skwovet caught (normal encounters), traded, stored and withdrawn, saved; after a power-on: parties, own Pokédex and the partner Pokédex intact, no copies | PASS |
| `roster_repair` | j65 | HOENN guest in Fallarbor (Mudkip; once a day); Surskit (Route 102), Galarian Zigzagoon and Budew (Route 103) caught in normal encounters | PASS |
| `roster_breeding` | j65 (Rosa) | Politoed and Ditto (helpers) left at the Day Care through the PC/party chooser; egg found by walking; it is a Poliwag egg; hatched (hatch counter set to 0 by a helper); Pokédex #60 | PASS |
| `battleforms_coop` | j16, co-op doubles | Cramorant's Gulping form, Morpeko's Hangry Mode and Eiscue's Noice Face on both screens (pixel-identical on the guest) | PASS |
| `dex_probe` | j68 (Kai) | LIST BOTTOM: National No 1025 and, after a HOENN search, regional No 646 (Sigilyph) with its entry and AREA hint; type FAIRY, A to Z finds Sylveon; counters; a seen-only entry concealed | PASS (fixtures: Sigilyph, Pecharunt, Sylveon flags) |
| `fieldguide_probe` | j65 | Researcher's guide: 42 newest arrivals scroll to the last (FOSSILS), the 8 partner regions include HOENN | PASS |
| `shop_b2` | j65 | POWER SHOP at eight badges sells a Strawberry Sweet and a Cracked Pot (bought with Kai's money) | PASS |

## Quests (focused gameplay)

| Scenario | Saves | Checks | Result |
| --- | --- | --- | --- |
| `shoal_solo`, `caravan_solo`, `coast_solo`, `twins_solo`, `beacon_solo`, `pp_solo` | j68 (post-League) | Each quest started and finished on a save that has cleared the League | PASS (6/6) |
| `shoal_solo` at noon and at 16:00 | seg6 | The whole quest at high tide and at low tide | PASS, PASS |
| `shoal_tide_a` → `shoal_tide_b` | seg6, then its save | High tide: briefing and two clues, leave for Mossdeep, save. Power on at low tide: the progress is kept, radio, Isla, rescue, gift once | PASS |
| `shoal_vulpix_full_party` | `shoal_solo`'s save | Alolan Vulpix caught with six in the party: to the PC, gone from the map, still gone after a reload of the map | PASS |
| `quest_loss_retry` | seg6 | Run from an Alolan Grimer (it stays); lose to SLAG (quest unchanged, no reward); win on the retry; MARLIN pays once | PASS |
| `quest_busy_partner` | seg6, linked | Same crate on the same frame (counted once, gone on both); player 1 in the party menu and then in a wild battle while player 0 progresses: player 1's save has every crate afterwards | PASS |

## Full regression on the final ROM and harness

`run_all.sh` with `LONG=1` (story chain), `SEG6`, `J16`, `J37`, `J43`, `J48`, `J52`, `J65`, `J68`: **180 scenarios, 179 PASS, 1 FAIL.** (`shop_b2` was added to the list after the suite started; it passed on its own run on the same ROM.)

| Result | Detail |
| --- | --- |
| FAIL: `wild_simultaneous` | The test needs both games to roll a wild encounter on the same frame (each then answers the other "busy" and both fight solo). Rerun three times on the same ROM with no other load: PASS, PASS, FAIL. In both failures Kai's game did not roll: Rosa's invitation reached it first and opened the join question; Rosa's game waited about three seconds, said "Solo: your partner didn't answer" and fought alone; Kai's game then said "Your partner couldn't wait, so that fight went ahead without you" and waited for a button. Both games kept the link (state 2) and the Beacon, nothing was left waiting, no data diverged. That is the documented invitation-timeout path, not the collision the test asserts. The cause is the two cores not stepping on the same frame; the game code in this area did not change this build. It also failed once under load in 0xC0170136. Not fixed. |
| Visual-check flags (4): `story_seg3`, `finding16_wild_teleport_karen`, `finding16_partner_wild_karen`, `coop_battle_items` | All "no-blank-name": a blank name printed when the healthboxes are redrawn, belonging to the absent fourth battler (the same kind as in 0xC0170137, where screenshots of that moment showed nothing blank). A recorded rerun of `finding16_wild_teleport_karen` raised no flag; not looked at frame by frame this time |
| PASS includes | `pp_coop` (VOLT + FUSE duo), `pp_reconnect`, `pp_stagger`(+reload), every quest's solo, co-op and reload runs, the six quests on the post-League save, the new quest and roster scenarios above, `center_beacon`, `deal_battle_doubles`, story segments 1-6, the legend chains, the recovery tests |

## Clips (two screens side by side; this ROM)

| File | Content |
| --- | --- |
| `clip1_quest_busy_partner_C0170138.mp4` (1:15) | Desert Caravan: both press A on one crate on the same frame; Rosa in the party menu; Rosa in a wild battle while Kai beats the raiders and takes the last crate |
| `clip2_new_encounters_C0170138.mp4` (1:29) | The Fallarbor HOENN guest (Mudkip); Galarian Zigzagoon and Budew on Route 103; Surskit on Route 102 (normal encounters) |
| `clip3_unusual_evolutions_C0170138.mp4` (1:06) | Milcery holding a Strawberry Sweet, spun in place: Alcremie; a friendly Eevee knowing a Fairy move: Sylveon |

## Coverage gaps

- Placements are source-checked by the audit; 45 species were obtained in play across both batches (`verified.json`), not all 228.
- Not played: Cramorant's Gorging form; Castform's forms; a quest step during a heartbeat loss; a quest Pokémon with a full PC; a loss in the Twin Trial or to VOLT; the Kanto, Sinnoh, Unova, Kalos, Alola, Galar and Paldea starters beyond their first gifts (gate tests and source only for most).
- Finding #16 remains unreproduced.
- Not tested: mGBA GUI, real hardware, netplay.
