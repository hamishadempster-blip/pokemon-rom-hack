# Test log, build 0xC0170139

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). ROM source commit `d4d5a863`; this log, the handoff and the checklist are in the commit after it (docs only; the ROM is unchanged by them).
- ROM `emerald-coop-C0170139.gba` (32 MB): SHA-1 `a2456d1f99c13a852d1d8134e997e6a54a5f6978`, SHA-256 `bb8da58d8400d5cff39d2f7c5b498e227265b9f099b4cb69beab005e12846bac`. ROM space used: 26,999,340 of 33,554,432 bytes (80.5%; 0xC0170138 used 26,987,296).
- Commits since 0xC0170138: roster batch 3 `367e36e1`, harness and scenarios `44d6eedb`, build id `d911ba53`, Hoenn Pokédex `d4d5a863`; harness fixes `a47896d5` (test tools only; the ROM is unchanged).
- All gameplay is the two-core harness on libmgba 0.10.3 (`tools/coop_link_harness`), p0 = Kai (link master), p1 = Rosa. Fixtures and test helpers are named in each scenario's header and logged as `FIXTURE` / `TEST HELPER`. Where a Pokémon was made by a helper (to reach an evolution quickly), only the evolution itself counts as played.

## Source and data checks (no gameplay)

| Check | Result |
| --- | --- |
| ROM build | A clean build of `d4d5a863` in a fresh worktree (no build directory) reproduces the tested ROM (same SHA-1) |
| `tools/coop_protocol_test.c` (host compiler) | 131 checks, all passed |
| `tools/coop_roster_audit/roster.py --check` (built ROM + sources) | 908 obtainable species (991 forms), 908 solo-completable, 0 issues. Ordinary roster: 900 of 900 (National Dex minus 125 deferred). BATCH1 114/114, BATCH2 114/114, BATCH3 230/230. Hoenn Pokédex 910 unique entries, 2 without a path (Jirachi, Deoxys); obtainable species without a Hoenn number: 0. 494 AREA hints, none missing, all fit their line (small narrow font, 224 px). Only the Hoenn legendaries are not renewable |
| Names, types, descriptions, sprites, footprints (read from the ROM by `roster.py`) | No issue for any of the 908 |
| Acquisition chains | Every evolution's resources (items, moves, places, times, party members, weather) traced to a renewable source; no resource only from a quest (`quest_gated` check). No circular dependency: the audit's fixed point starts only from wild slots, gifts, fossils, eggs and trades, so a species reachable only through a cycle could not count as obtainable |
| Comparison with 0xC0170138's manifest | No species lost all wild sources; none became obtainable later; 5 have fewer wild tables (Poochyena 8 → 3, Zigzagoon 6 → 4, Whismur and Carvanha 4 → 3, Shuppet 10 → 9) |
| Revamps (`tools/coop_roster_audit/audit.py` on the built ROM) | 328 species checked, 0 issues: every earlier revamp (stats, types, abilities, learnsets) intact |
| Save compatibility | No save structure, flag or variable added; header changes are the Hoenn order (not stored), prototypes and the build id; species ids unchanged (no species added). Every checkpoint save the suite loads was written by an older build |
| Field guide width check | Passed |
| Excluded set | Matches the engine's legendary/mythical/Ultra Beast/Paradox flags (forms-keyed species checked by family) |

## Roster batch 3 (focused gameplay)

| Scenario | Saves | Checks | Result |
| --- | --- | --- | --- |
| `roster_b3_catch` | j68 (post-League), not linked | One catch per encounter method and habitat, ordinary wild battles: Flabébé (Route 117 grass), Karrablast (Altering Cave), Rellor (Rock Smash, Route 111), Furfrou (Safari Zone North-East, entered through the gate, Safari Balls); Alolan Diglett (Granite Cave B1F), Krabby (Super Rod, Route 104), Galarian Stunfisk (surfing, Route 114), Galarian Yamask (Sandburied Ruins), Hisuian Zorua (Mt. Pyre 6F). Pokédex caught set for each | PASS (fixtures: warps, Ultra Balls, a registered Super Rod, a Pokéblock Case) |
| `roster_b3_evo_place` | j65 (Kai noon, Rosa 23:00) | 24 evolutions: regional branches by place (Alolan Raichu in Dewford, Raichu on Route 114; Galarian Weezing, Mr. Mime, Mr. Rime; Hisuian Typhlosion, Sliggoo, Goodra on Mt. Pyre and its foggy summit; Goodra; Alolan Marowak and Exeggutor, Marowak on Route 114); Sneasler, Kleavor, Overqwil, Clodsire, Galarian Slowbro and Slowking, Perrserker, Escavalier and Accelgor (Linking Cord, partner in party), Ursaluna (night), Obstagoon, Cursola | PASS (pre-evolutions by logged helper) |
| `roster_b3_evo_misc` | j65 | 31 evolutions, 2 cloak changes and 1 non-evolution: Tyrogue ×3, Vespiquen, a male Combee that stays, both Meowstic, Chansey (Oval Stone by day), Blissey, Snorlax, Lopunny, Yanmega, Tangrowth, Lickilicky, both Dudunsparce, Mantine (Remoraid in party), Gengar, Steelix, Electivire, Aromatisse, Slurpuff, both Sinistcha, Archaludon, Malamar, Rabsca (1000 steps), Burmy's Sandy cloak (Granite Cave battle) → Wormadam (sandy), Trash cloak (Mt. Pyre battle) → Wormadam (trash), Mothim; Annihilape (the 20th Rage Fist in a wild battle); Sirfetch'd (3+ critical hits with a Leek in one wild battle) | PASS (helpers: the Pokémon, gender/nature/personality draws, Rage Fist count 19, step counter set to 0, items) |
| `roster_b3_services` | j65 | Lilycove clerk sells Pink Nectar and Metal Alloy (bought); the nectar turns Oricorio Pa'u, the alloy evolves Duraludon; Fossil Maniac's BIRD + DRAKE, DEVON's list shows the pair, Dracozolt revived; the Sandburied carving's hint with no such Pokémon, then Runerigus | PASS |
| `roster_b3_battle` | j16, linked, Beacon ON, co-op doubles | Schooling (School Form on both screens), Shields Down (Core Form after half HP, both games' data), Flower Gift (Sunshine Cherrim, both screens pixel-identical), Dancer (Oricorio copies Swords Dance on both screens), Emergency Exit (Golisopod flees: the wild battle ends for both, outcome 5 on both, link kept), Receiver (Passimian takes Torkoal's Drought; text on both screens) | PASS (leads by helper; force_move) |
| `roster_b3_play` + `_reload` | seg6, linked, Beacon OFF | Buneary (Route 102) and Yungoos (Route 103) caught in solo battles while linked; traded through the partner menu; deposit and withdraw at the PC; UNLINK, LINK UP: both partner Pokédex marks agree; saved; after a power-on: parties, own and partner Pokédex intact, no copies | PASS |
| `dex_probe` | j68 | LIST BOTTOM: Hoenn No 910 (Miltank) with entry and AREA (Safari Zone); National No 1025; type FAIRY A–Z; counters; seen-only entry concealed | PASS (fixtures: Miltank, Pecharunt, Sylveon flags) |
| `fieldguide_probe` | j65 | FINAL ARRIVALS (25 entries) scrolls to SPRITZEE, both pages; NEWEST (42) as before; PARTNER (now the fifth group) lists HOENN | PASS |

Harness fixes found while writing these: `hunt` and `catch` pressed B in an evolution scene that follows a battle, which cancelled it (the first Sirfetch'd runs "failed" for that reason; the game was right); `hunt` now fights trainers who spot the player and wild Pokémon that cannot be escaped (Arena Trap), fishes and uses Rock Smash.

## Full regression on the final ROM and harness

`run_all.sh` with `LONG=1` (story chain), `SEG6`, `J16`, `J37`, `J43`, `J48`, `J52`, `J65`, `J68`: **188 scenarios, 187 PASS, 1 FAIL** (the 8 new batch-3 scenarios and the updated `dex_probe` and `fieldguide_probe` included; `wild_simultaneous` passed this time).

| Result | Detail |
| --- | --- |
| FAIL: `late_wild_tour` | Its hunt for a Drifloon (4% of Route 121, unchanged by this build) met three in 120,000 frames and each broke free of six Poké Balls; the hunt timed out. Rerun alone on the same ROM: PASS, then FAIL in its Meteor Falls battle: the battle driver kept choosing a move with no PP left against a new wild Minior (its Shields Down form takes long to wear down), a test-driver limit, not a game fault. The driver now moves the cursor to a move with PP (as a player would); three reruns with it: PASS, PASS, PASS. `roster_b3_battle` rerun with the final driver: PASS. `roster_b3_evo_misc` rerun: FAIL, then fixed in the scenario: a wild Litwick on Mt. Pyre burned the helper Burmy (Bug Bite only, which Litwick resists) until it fainted, and a fainted Burmy keeps its cloak (the game's rule). The helper Burmy now also knows Hidden Power (a TM move it can learn): two reruns PASS |
| Visual-check flags (4) | `finding16_wild_teleport_karen`, `finding16_partner_wild_karen`, `coop_battle_items`: "no-blank-name", the absent fourth battler's healthbox name, as in 0xC0170137 and 0xC0170138 (not visible on screen then). `roster_b3_catch`: "no-phantom" at the Safari Zone's "What will KAI do?", where the player has no Pokémon in battle by design; the checker now skips that prompt (re-checked: pass) |
| PASS includes | Story segments 1-6, every quest's solo, co-op, reload and post-League runs (`pp_*`, `shoal_*`, `caravan_*`, `coast_*`, `twins_*`, `beacon_*`), the earlier roster scenarios (`roster_b1_*`, `roster_b2_*`, `roster_repair`, `roster_breeding`, `battleforms_coop`, `shop_b2`), the legend chains, the recovery tests, `center_beacon`, `deal_battle_doubles`, `wild_simultaneous` |

Earlier quest rewards and unlocks still supply their Pokémon and items: the quest scenarios above (Rotom and the POWER SHOP, Alolan Vulpix and the Ice Stones, Sigilyph and the merchant, Alolan Grimer, the Twin Trial rewards) all pass on this ROM.

## Clips (two screens side by side; this ROM)

| File | Content |
| --- | --- |
| `clip1_coop_battle_abilities_C0170139.mp4` (1:45) | `roster_b3_battle`: School Form and Super Fang → Core Minior; Sunny Day → Sunshine Cherrim, Oricorio's Dancer; Emergency Exit ends the battle on both screens; Receiver takes Drought |
| `clip2_fossils_carving_shop_C0170139.mp4` (0:44) | `roster_b3_services`: the Lilycove clerk's new stock and Pink Nectar on Oricorio; the Galar fossil pair at DEVON; the Sandburied carving evolving Galarian Yamask |
| `clip3_regional_branches_C0170139.mp4` (1:40) | `roster_b3_evo_place`: Thunder Stone in Dewford (Alolan Raichu) and on Route 114 (Raichu); Peat Block at night (Ursaluna); Mt. Pyre's Hisuian Typhlosion and Sliggoo, Hisuian Goodra in the summit fog |

## Coverage gaps

- 11 species caught in play this build (9 for the methods and habitats, 2 while linked), 67 obtained in play in total this build; 112 across the three batches (`verified.json`). Every placement is source-checked.
- Not played: a real co-op trade evolution of Karrablast/Shelmet; Everstone breeding of a regional form; three of the four Galar pairs and three of the four nectars (same code paths as the ones played); Plant cloak (Burmy's default); every Hisuian starter evolution other than Typhlosion (same rule).
- Rage Fist counts and battle-end critical hits do not count in co-op (link) battles: source-checked and documented, not played.
- Finding #16 remains unreproduced. Not tested: mGBA GUI, real hardware, netplay.
