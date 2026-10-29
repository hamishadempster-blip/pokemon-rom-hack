# Co-op compatibility audit, build 0xC017013C

Branch `coop-doorway-scenes-rivals`, after 0xC017013B (Trainer Exchange). Mega Evolution and new overworld item placements were not touched.

Words used below: **inspected** = read in the source only; **tested** = played in the two-core harness on this ROM (scenario named); **fixed** = a confirmed fault, changed at its source, with a regression that fails on 0xC017013B and passes now; **not reproduced** = tried and not seen. Fixtures and test helpers are named in each scenario's header. Everything ran in the libmgba 0.10.3 two-core harness; no mGBA GUI, hardware or two-PC netplay was used. This audit does not show the game is free of bugs.

## State ownership

| Kind | What | How it crosses the link |
| --- | --- | --- |
| Shared events (both saves record them) | Co-op gym, Elite Four, champion, rival and story fights; the quest milestones (Shoal clues, caravan crates, coast clues, shrine marks, Power Problems breakers); shared legendary records; joint badges | Invite and story messages; each save writes its own copy of the result. A joint badge (won together) is restored on LINK UP to a save that lacks it. The flag a shared win sets is now limited to the known win flags (below) |
| Shared presentation only (RAM) | Partner sprite, outfit, activity icon, edge arrow, time of day tint, the partner's wandering NPC steps, conversation text | Per-frame and talk messages; nothing saved |
| Personal (each save alone) | Money, bag, party, PC, Pokédex (plus a partner-dex mark), Day Care, berries, Secret Base, puzzle state (gym switches and doors, boulders, ice, Trick House), Safari, Frontier, contests, RTC and daily events, repel | Only through a consented deal (trade, gift) or a shared win |
| Temporary battle state | Co-op battle parties (three each), held items, forms | Link battle engine on both games; each game restores its own party after |

No complete flag or variable table is ever copied. A more advanced save never overwrites the other: shared wins only add the win's own flags and the guest's catch-up steps for that event.

## Audit table

| Area | Intended behaviour (solo / co-op) | Inspected / tested | Result |
| --- | --- | --- | --- |
| **Gym puzzles** | Puzzle state per save; a shared gym fight starts for both; the guest gets the catch-up (doors, barriers, trainers) | Tested on this ROM from existing checkpoints: `journey_36_wattson` (Mauville switches and barriers), `journey_41_flannery` (Lavaridge), `journey_45_norman` (Petalburg room doors), `journey_51_winona` (Fortree rotating gates), `journey_58_tate_liza` (Mossdeep), `journey_64_juan` (Sootopolis ice); new `mandatory_busy_menu`, `mandatory_busy_talk`, `mandatory_busy_timeout` | **Fixed (A, B)**: a leader's shared fight pulled a partner out of an open bag into the battle, and from an open conversation the partner's game stopped with a heap check at the battle's start. Partner sprites can stand on tiles that are blocked in the other save's puzzle state (drawing only; the partner never collides with your own puzzle) |
| **Strength, Rock Smash, Cut** | Each save's own objects | Tested: `coop_boulder_personal`, `journey_60_seafloor` (Strength and Rock Smash rooms), `journey_43_rusturf` (Rock Smash) | **Fixed (E)**: Strength boulders were synced like wandering people; a push on one save moved the boulder on the other and blocked the partner's route (`journey_60_seafloor` failed on 0xC017013B and passes now). Rocks and trees were never synced (no wandering movement). Rock Smash encounters stay solo (inspected) |
| **Shoal Cave tides** | Tide per save's clock; low and high tide are different maps, so players in different tides never share a room | Tested (suite): `shoal_tide_a`, `shoal_tide_b`, `shoal_coop`, `shoal_coop_reload`, `shoal_vulpix_full_party` | No fault found |
| **Sky Pillar, bikes** | Personal floors; FOLLOW stops at cracked floors | Tested: `journey_63_sky_pillar` (Mach Bike climb, falls, both players); `partner_appearance` (Mach Bike sprite) | No game fault. The scenario failed because the second player started before the first had finished (a race in the test script); fixed in the scenario with a wait, then passed |
| **Dive, Surf, Waterfall** | Movement mode per save; partner sprite shows surfing/diving | Tested: `journey_60_seafloor` (Dive, Surf, currents), `journey_64_juan`, `partner_appearance` (Surf) | No fault found |
| **Transport** | Each player travels on their own save (no shared ride) | Tested: `journey_21_briney` (Mr. Briney with both). Inspected: cable car, ferries (stock warps, no co-op code; the partner sees the other leave the map) | No fault found; cable car and ferries not played |
| **Safari Zone** | Solo place: own fee, steps, balls; no invitations | Inspected (`Coop_PlaceStaysSolo`); tested: `roster_b3_catch` Safari hunt (suite) | No fault found |
| **Trick House** | Per save: puzzles, password, rewards | Inspected only (no co-op code) | Not tested |
| **Consecutive battles** | Each fight set up fresh; loss midway handled | Tested (suite): `twins_coop` (Twin Trial rounds), `quest_loss_retry` (loss midway, retry), `e4_sidney`, `coop_partner_wiped(_wild)`, `coop_host_two_replacements`; `journey_67_elite_four` (Sidney, Phoebe, Glacia, Drake, then Steven and Wallace, back to back, both players: all won, both at the Hall of Fame) | No fault found. `journey_67_elite_four` itself reports a failure: its last step allows 3,000 frames to enter the Champion's room and the whole Champion fight ran inside that step (a test budget; the original playthrough split the Champion into `journey_67c`) |
| **Whiteouts and escapes** | Each game applies its own loss once | Tested: `coop_wild_loss` (both lose: each back at its own heal place, healed, linked; a co-op loss costs no money, as stock link battles), `whiteout_home`, `coop_partner_wiped`, `finding16_wild_teleport_karen` (a wild Teleport) | No fault found. Escape Rope and Dig not played (stock field code) |
| **Battle Frontier, rentals** | Solo place; rentals restored by the stock code | Inspected (`Coop_PlaceStaysSolo`, the team picker's temporary `VAR_FRONTIER_FACILITY`) | Not played |
| **Contests** | Solo; never interrupted | Inspected: no co-op code; invitations need the same map and now never start under a menu screen or message (fix A/B); move contest data comes from the expansion's move table (every move has an entry) | Not played |
| **PC storage and release** | A Pokémon offered in a deal is fixed until the deal ends | Inspected: the offering player waits in the deal script (no PC access); tested (suite): `deal_trade`, `deal_cut_*` (power cuts), `legend_ray_full` (PC full) | Hardened: an incoming Pokémon is now checked (checksum, species, not an egg) before a trade or gift is accepted |
| **Held items** | Items stay with their own save; Gen 9 restore for your own losses | Tested: `coop_held_items` | **Fixed (C)**: TRICK on the ally swapped items and both saves kept the other player's item. Now Trick, Switcheroo, Bestow and Thief/Covet fail between the two players' Pokémon in co-op (and other link multi) battles |
| **Transformations** | Both screens agree; stored Pokémon unchanged | Tested (suite): `battleforms_coop` (Transform, Cramorant, Morpeko, Eiscue), `roster_b3_battle` | No fault found; Illusion not played |
| **Rematches, daily events** | RTC per save; shared tint display only | Inspected (clock messages are RAM only); tested (suite): `lighting_link` | No fault found |
| **Berries** | Personal patches; personal harvests are legitimate | Inspected: no co-op code | Not played |
| **Secret Bases** | Never shown as a shared room | Tested: `coop_secret_base_room` | **Fixed (D)**: two players in bases with the same map number saw each other inside their own base. The partner is now hidden (and not counted as nearby) in Secret Bases, Battle Pyramid floors and Trainer Hill floors, whose rooms are built from each save |
| **Repels and encounter modifiers** | Personal exploration uses your own Repel, lead ability and rod | Inspected: a step rolls on the stepping player's game with its own Repel; the partner is only invited (YES/NO) to a fight already rolled; the second wild Pokémon of a co-op double is generated without the partner's Repel | Defined (documented); not changed |
| **Saving during transactions** | Each stage recoverable once | Tested (suite): `deal_cut_before_partner(_reload)`, `deal_cut_after_commit(_reload)`, `deal_lost_ready`, `legend_ray_interrupt` / `_reconnect`, `legend_ray_lostack` / `_reconnect2`; purchases and Exchange services are single-save | No fault found |
| **Breeding follow-up** | Each save's Day Care; own steps | Tested: `breeding_coop` (both at once), `breeding_rules(_reload)` | No fault found |

## Confirmed bugs and fixes

| | Fault | First wrong state | Fix | Regression (fails on 0xC017013B, passes now) |
| --- | --- | --- | --- | --- |
| A | A shared gym, rival, Elite Four or story fight started while the partner had a menu screen open (bag, party, PC, shop): the battle replaced that screen without closing it | The guest's game accepted a mandatory invite whatever its own screen (`Coop_HandleInvite`) | The guest answers "busy"; the inviter asks again every half second for up to 30 seconds; if the partner stays busy: "<partner> stayed busy in a menu or a conversation. Nothing started." | `mandatory_busy_menu`, `mandatory_busy_timeout` |
| B | The same invite while the partner read an NPC's message: the partner's game stopped at the battle's start (malloc check, reset) | As A, under an open message box | Busy while a message box is shown | `mandatory_busy_talk` (log of the crash kept) |
| C | Trick between the two players' Pokémon: each save kept the other's item | `CanBattlerGetOrLoseItem` allowed item moves between link multi allies | Refused between the two players' battlers | `coop_held_items` |
| D | Partner drawn inside your own Secret Base | Same-map test used the map number only | Personal-instance maps (Secret Base, Pyramid floor, Trainer Hill floor) are never the same room | `coop_secret_base_room` |
| E | Strength boulders moved on both saves | Boulders (look-around movement, no trainer) passed the shared-NPC filter | Inanimate objects are never synced | `coop_boulder_personal`, `journey_60_seafloor` |

Hardening without a reproducible fault: an incoming deal Pokémon is validated (F); the flag a shared win sets on the partner's save is limited to the 18 win flags the scripts use (G; before, any flag id in the invite was set).

## Evidence kept

- Before-fix logs on 0xC017013B: `mandatory_busy_menu_before_fix`, `mandatory_busy_talk_before_fix` (the reset), `coop_held_items_before_fix`, `coop_secret_base_room_before_fix`, `coop_boulder_personal_before_fix`, and `journey_60_seafloor` failing at the same boulder on 0xC017013B (work environment: `/home/claude/journey/audit`, `/tmp/claude-0/runsB`).
- Two-screen clip of the five fixes: `clip_audit_fixes_C017013C.mp4`.

## Not reproduced / limits

- Finding #16 (two-trainer launch) is still not reproduced; its instrumentation stays.
- Not played: cable car, ferries, Trick House, Battle Frontier rentals, contests, berry planting, Escape Rope/Dig, Illusion, Knock Off between allies (now refused by the same rule as Trick).
- The harness is two emulator cores in one process; it does not establish desktop netplay or two-PC reliability.
