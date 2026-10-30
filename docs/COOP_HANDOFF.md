# Collection update, second delivery: build 0xC0170142

Branch `coop-doorway-scenes-rivals`, pushed, not merged. It also includes the D1400001/D1400002 and D1500001/D1500002 fixes, which were merged before this round. **Both players need this build:** it links only with 0xC0170142.

Saves from 0xC0170141 and older load unchanged. The save layout is the same (layout 5); the new features use its reserved space. Compatibility: 33/33 comparisons PASS on 11 save pairs.

Docs:
- Test log: [COOP_TESTLOG_C0170142.md](COOP_TESTLOG_C0170142.md)
- Autosave: [COOP_AUTOSAVE.md](COOP_AUTOSAVE.md)
- Waiting at a Center: [COOP_TIME.md](COOP_TIME.md)
- Visitors: [COOP_ENCOUNTERS.md](COOP_ENCOUNTERS.md)
- CAN LEARN: [COOP_LEARN.md](COOP_LEARN.md)
- FIELD NOTES: [COOP_NOTES.md](COOP_NOTES.md)
- Habitats: [COOP_HABITATS.md](COOP_HABITATS.md)
- Towns: [COOP_TOWNS.md](COOP_TOWNS.md)
- HQ plan (not implemented): [COOP_HQ_PLAN.md](COOP_HQ_PLAN.md)

## The build

- **ROM:** `emerald-coop-C0170142.gba`, from game code `6f18870e`. A clean clone builds the same SHA-1, given the Makefile dependency fix `bbe18c3e`.
  - SHA-1 `fcbb6be8b1ec270091db9b5805ded3126599b1f2`
  - SHA-256 `4f7e6ee558d8657b5b4a2961d6294e9a15cd3c0530838329b03757206e9a85b6`
- **Commits:**
  - autosave `e7132728`
  - Center wait `e36a9412`
  - visitors `a8b3268e`
  - CAN LEARN `7a2af586`
  - FIELD NOTES `c0dcefe0`, `6a705e04`, `1c5545f7`
  - habitats `54adb312`
  - towns `e5ab831d`
  - build id `331ed831`
  - review fixes `6f18870e`
  - Makefile `bbe18c3e`
  - scenario fixes and docs after the suite
- **Testing:**
  - Final suite on the delivered ROM: 350 scenarios, **337 PASS and 13 FAIL on the first run**:
    - 11 of the failures were the new 10-kind FIELD NOTES reward appearing in unrelated scenarios on journey saves. The game behaved correctly; the scenarios now switch the payout off and pass on rerun.
    - `late_wild_tour` and `coop_bag_battle_tasks` failed on luck or timing; both were made robust and pass on rerun.
    - `roster_repair` still fails: the known harness `hunt` problem from 0xC0170140.
  - Power cuts: once and twice in a row; the previous save loaded each time.
  - Compatibility: 33/33 comparisons PASS.
- **Readiness:** ready for a player playthrough, with the stated limitations.
  - The Expedition HQ is not implemented.
  - No crash, freeze, save damage or progression blocker is known in this build.
  - Four defects were found and fixed before delivery:
    - stale map view after an autosave;
    - New Game asked the autosave question;
    - a rare double-power-cut mixture;
    - old rewards announced on the first steps.
  - Not bug-free; nothing here claims so.

## Status of the collection request (by section)

| Section | Status |
| --- | --- |
| Autosave (added to this round) | **Implemented and tested**: separate slots; an autosave never overwrites the newest manual SAVE; the question after CONTINUE; going back to the last SAVE; setting; linked; power cuts |
| 2 Encounter variety | **Implemented and tested** (visitors: 21 kinds, 57 appearances, 26 maps; the Center REPORT). The engine's own time-of-day tables stay off (reason in COOP_ENCOUNTERS.md) |
| 3 Five hidden habitats | **Implemented and tested** (all five entered; MOSS HOLLOW's completion reward played) |
| 4 Notebook and collection rewards | **Implemented and tested** (FIELD NOTES; 10 rewards; queue; catch-up on older saves) |
| 8 Waiting for a time of day | **Implemented and tested** (solo, reload, linked) |
| 9 Expedition HQ | **Not implemented**: staged design in COOP_HQ_PLAN.md; 256 bytes stay reserved |
| 11 "Who can learn this?" | **Implemented and tested** |
| 12 Town specialities | **Implemented and tested** (six specialists, first track rewards) |
| 14, 5, 6, 7, 10, 15, 1 | As in 0xC0170141 (below) |

## Open issues (new or still open)

- **Expedition HQ:** not implemented.
- **CONTINUE screen:** "Build C0170142" runs into long location names (pre-existing since save identification; seen in 0xC0170141 clips). Not fixed in this delivery, so that the delivered ROM is the one tested.
- **Harness:**
  - `roster_repair` (`hunt` regression since 0xC0170140);
  - `wild_simultaneous` and the `partner_menu` FOLLOW failure are intermittent (both passed in this suite).
- **Save counters:** compared without wraparound handling. A wrap would need billions of saves.
- **Balance:** visitor rates and reward sizes are not tuned with unassisted play.
- **Not tested at all:** the mGBA GUI by a person, real hardware, two-PC netplay.

---

# Previous: collection update, first delivery: build 0xC0170141

Branch `coop-doorway-scenes-rivals` (continued from 0xC0170140), pushed, not merged. Both players need this build: 0xC0170140 and older refuse to link with it.

Saves from earlier builds load unchanged. The co-op block gains layout 5 (376 bytes, zero), checked field by field on 11 save pairs (CoopSave layouts 1–4). A save from a newer build is refused and left untouched.

Docs:
- Test log: [COOP_TESTLOG_C0170141.md](COOP_TESTLOG_C0170141.md)
- Storage design: [COOP_STORAGE_PLAN.md](COOP_STORAGE_PLAN.md)
- Trainer Bond: [COOP_BOND.md](COOP_BOND.md)
- Settings, shiny, catch result and battle conditions: [COOP_POLISH.md](COOP_POLISH.md)
- Save rules: [COOP_SAVE_COMPAT.md](COOP_SAVE_COMPAT.md)

## The build

- **ROM:** `emerald-coop-C0170141.gba` from source `712d7dc3`. No game-code change after it, and a clean clone checked out at `712d7dc3` builds the same SHA-1.
  - SHA-1 `dc7c31b978c6b3572a752a3a9793e401c2ff78ac`
  - SHA-256 `3334223a8868bc1815e500303ffe1bd236edc336a7f24798e239389f384880b6`
- **Commits:**
  - storage preflight, layout 5, refusal of newer saves, full-slot link saves: `913f7663`
  - Trainer Bond: `a7e8b0f2`, `612b5a66`, `ee98ec4b`, `51258ff4`
  - settings, low-HP alarm, easy fishing, shiny, catch result: `c7a5e813`
  - battle conditions panel: `ee2eb431`
  - `debug_inputs_off` update
  - build id: `712d7dc3`
  - after the build: roster manifest, two scenario fixes, docs
- **Testing:**
  - Final suite on the delivered ROM: 307 scenarios, **301 PASS, 6 FAIL on the first run**:
    - two of my new Bond scenarios had observed values that vary (fixed, rerun PASS);
    - `twins_solo` lost a lucky battle (rerun PASS);
    - `partner_menu`'s FOLLOW-through-door failed once (3/3 reruns PASS; intermittent, open);
    - `roster_repair` failed because of a harness `hunt` regression that also fails on 0xC0170140 (not fixed).
  - 27/27 power-cut loads recovered the previous save.
  - Compatibility: 33/33 comparisons PASS.
- **Readiness: the delivered features are ready for a player playthrough, with stated limitations.**
  - No crash, freeze, save damage or progression blocker was found in this build.
  - Most of the request is **not implemented yet** (below).
  - The known open issues of 0xC0170140 remain, and one intermittent FOLLOW failure is new in the record.
  - Not bug-free; nothing here claims so.

## Status of the collection request (by section)

| Section | Status |
| --- | --- |
| 15 Existing saves (release gate) | **Done for this delivery:** storage plan; layout 5 with migration; refusal of newer saves; interrupted-write tests; late unlock (the Bond Mentor checks the badge on any save: `bond_mentor` on a pre-update save); compatibility matrix; update guide. The user's own saves were not supplied, so they were not tested |
| 14 Trainer Bond | **Implemented and tested**; gaps listed in COOP_BOND.md (no follower or greetings, Transform untested, award rates not tuned by unassisted play) |
| 5 Shiny visibility and SHINY CHECK | **Implemented and tested** (solo and co-op guest); no overworld shiny appearances to cover |
| 6 Catch result | **Implemented**; solo tested, co-op field path source-inspected only |
| 7 Low-HP alarm, easy fishing | **Implemented and tested** (EASY FISHING B-cancel and other rods untested) |
| 10 Battle conditions panel | **Implemented and tested** (both co-op roles, expiry) |
| 1 Roster manifest | **Regenerated:** 908/908 obtainable, 0 issues; no roster change in this update |
| 2 Encounter variety | Not started (time-of-day encounter tables off) |
| 3 Five hidden habitats | Not started (save space reserved) |
| 4 Notebook and collection rewards | Not started (ledger space reserved) |
| 8 Waiting for a time of day | Not started (period-shift space reserved) |
| 9 Expedition HQ | Not started (256 bytes reserved) |
| 11 "Who can learn this?" | Not started |
| 12 Town specialities | Not started (space reserved) |
| 13 Verification and delivery | This handoff, the test log, ROM and hashes, four clips |

## Updating both ROMs (keep your saves)

1. Each player backs up their `.sav` (copy it somewhere safe).
2. Each player replaces their ROM with `emerald-coop-C0170141.gba` **under the same file name as before** (mGBA pairs `name.gba` with `name.sav`), or renames the save to match the new name.
3. Open both games and CONTINUE on each. The CONTINUE screen should say *Build C0170141*.
4. CO-OP → LINK UP on both. A partner still on 0xC0170140 or older is refused: both must update.
5. If CONTINUE says "This save is from a newer build", that save was written by a later ROM. It is left unchanged; use that ROM.
6. If the game ever says "The save file is corrupted. The previous save file will be loaded.", the last save was interrupted. The previous complete save loads, and nothing is half-written.
7. Do not go back to an older ROM with a save this one has written (layout 5 is not known there).

# Previous build: 0xC0170140

## Release-readiness pass: build 0xC0170140

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013E), pushed, not merged. A stability pass, no new features. Both players need this build: 0xC017013E and older refuse to link with it. Saves from earlier builds load unchanged (checked field by field, below). Test log: [COOP_TESTLOG_C0170140.md](COOP_TESTLOG_C0170140.md). Save rules for future updates: [COOP_SAVE_COMPAT.md](COOP_SAVE_COMPAT.md).

- ROM `emerald-coop-C0170140.gba` from source `4ac1d749` (no game-code change after it): SHA-1 `a20130970e1caed23b0d7dbfe48bad0134ab29d5`, SHA-256 `6f0f67866deb3dfd31d69b23e0ed632b593a5b8c5e9fe5bbfb6156eb13ec4fc5`. A clone checked out at `4ac1d749` builds the same SHA-1.
- Commits: developer menus off, build 0xC017013F `38d764f1`; save compatibility tools and docs `cd700e3b`; `roster_b3_battle` retry `e26aea1d`; team-screen message fix, build 0xC0170140 `4ac1d749`; harness `hunt` and `visual_check` fixes `f878f3d8`, `4fe33bc9`, `9a359ae3`; docs after them. 0xC017013F was an intermediate build, never delivered.
- Testing: full suite on 0xC017013F (283 scenarios, 279 PASS on the first run, 4 FAIL: one game message, three driver); every scenario reaching the 0xC0170140 change rerun on it (57 runs, 54 PASS; the 3 FAIL were driver, fixed in the harness); every `hunt` scenario rerun with the final harness (10 of 10 PASS). Details in the test log.
- **Readiness: ready for a player playthrough with stated limitations.** No crash, freeze, save damage or progression blocker is known in this build: the full suite, the affected and hunt reruns and the save checks found none that remain (the two game defects found in this pass are fixed, with regressions). The limitations: (1) the first co-op gym beat the driver 9 times in 9, even at the level cap with the best team on hand, so it may be a difficulty wall for two players; only a person can tell; (2) nothing here was played in the mGBA GUI by a person, on hardware or over two-PC netplay; (3) the known open issues below (finding #16 not reproduced; the partner drawn on a tile the other save has blocked; no explanation line for differing personal dialogue; six Frontier facilities without a duo mode). It is not bug-free, and nothing here claims so.

## Updating both ROMs (keep your saves)

1. Each player backs up their `.sav` (copy it somewhere safe).
2. Each player replaces their ROM file with `emerald-coop-C0170140.gba` **under the same file name as before** (mGBA looks for the save next to the ROM with the ROM's name: `name.gba` uses `name.sav`). Or keep the new name and rename your save to match it.
3. Open both games (mGBA: one window, File → New Multiplayer Window for the second; two-PC netplay is untested), CONTINUE on each, and check the CONTINUE screen says *Build C0170140*.
4. CO-OP → LINK UP on both. A partner still on an older build is refused (build mismatch): both must update.
5. Each player keeps their own save; nothing is copied between them.
6. Do not go back to an older ROM with a save the new one has written (not guaranteed to work).

## Fixed in this pass

| Defect | First wrong state | Fix | Regression |
| --- | --- | --- | --- |
| SELECT at a battle's action menu crashed the game that pressed it in a co-op battle (seen on the partner's game, pc 0x1a8 in `CB2_BattleDebugMenu`); R+START on the field opened the developer menu (flags, variables, Pokémon); SELECT on a summary page other than INFO opened the sprite visualizer | Every ROM was built without `RELEASE`, so pokeemerald-expansion's developer menus (`DISABLED_ON_RELEASE`) were live | `DEBUG_OVERWORLD_MENU`, `DEBUG_BATTLE_MENU`, `DEBUG_POKEMON_SPRITE_VISUALIZER` are FALSE in every build; asserts stay (a crash screen START continues past) | `debug_inputs_off` (crashes on 0xC017013E, passes), `battle_spare_buttons` (R, START, SELECT, L, L+R at the action and move menus, both roles) |
| When the team screen timed out on the host's game first, the partner read "RED stepped back from the battle" (nothing started; wrong reason only) | The host's withdrawal of its invite reached the partner before the team-screen block carrying the reason; the two cartridge clocks tick at different moments, so either game can time out first | The partner waits up to 60 frames after a withdrawal for that block and uses its reason (and cannot commit meanwhile); with no block it says "stepped back" as before | `prep_timeout_host_first` (fails on 0xC017013F, passes; part 2: a real host CANCEL still reads "stepped back") |

Harness only (no game change): `hunt` stays near its starting row (it drifted out of the grass over many battles); `roster_b3_battle` repeats SUPER FANG until it lands; `roster_b3_catch` gives 40 balls; `roster_b3_play` hunts away from a ledge; `hunt` leaves the bag when a ball is refused (a Pokémon in the air); `visual_check.py` skips a half-printed battle prompt.

## Save compatibility (checked)

`compat_check.sh` (new): saves from checkpoints j06, j16, j44, j68, seg6, the lab saves and a save written by 0xC017013E, loaded on 0xC017013D and on this build, saved here, power-cycled and relinked, compared field by field (on 0xC017013F and again on 0xC0170140: 21/21 comparisons pass each time). Party, PC, bag, money, flags and variables are identical across builds; the only change is the CoopSave `layout` byte (migration to layout 4, new fields zero); a save written here reads back the same; linking changes only the partner Pokédex snapshot and quest partner. Allowed differences: map object templates and the map view (rebuilt from the ROM on every load), the play-time and berry clocks, the daily seed and the two lottery variables the game rerolls on a new day. Free flags, variables and CoopSave space for future content, and the rules for future updates: [COOP_SAVE_COMPAT.md](COOP_SAVE_COMPAT.md).

## Difficulty sample (no fixtures, driver picks each battler's most effective move)

- **First co-op gym (Roxanne + Heath, Lv 16–18, Oran Berries, smart AI): 9 losses in 9 runs** with the j13 checkpoint teams: as played (3 runs, 0–1 of 6 foes fainted), each player's best three from the boxes chosen on the team screen (3 runs, 1–4 fainted), and the same six raised to the Lv 18 cap (3 runs, 2 fainted each). The driver uses no items, never switches and has no strategy, so this does not show the gym is unwinnable, but it is a steep first step for two players at or below the cap. **Needs a human playtest before release to players.**
- **Duo Tower LV50 battle 1 with the j68 parties: 3 wins in 3.**
- One earlier data point: `prep_slots456`'s team lost to Norman unassisted.

## Remaining issues (not intended)

- **Early difficulty** (above): possible wall at the first co-op gym; untested by a person.
- **Finding #16** (an invalid opponent-name assertion after a co-op wild battle into trainer detection): still not reproduced; not declared fixed.
- **Assert on the partner's game in a one-trainer co-op fight** (0xC017013E pass 1, once): the name lookup was fixed in 0xC017013E; why only the partner's game asserted is not proven. The battle developer menu is gone now, which removes one conceivable route, but that is not shown to be the cause.
- **Partner drawn on tiles blocked by the other save's puzzle state** (a boulder pushed on one save): unchanged.
- **Shared dialogue with different personal progress**: still no explanation line when the partner's version differs.
- **Visual refinements not done:** rival art; party, bag and battle menu restyling.
- **Duo Frontier:** six facilities have no duo mode (each desk says so); no Mega-disabled bracket; no duo Frontier Brains.
- **Not played:** berry planting/harvest, Trick House, Illusion, Repel prompts around scripts and transitions, joined conversations with mismatched progress, postgame rematches with mismatched progress, team-screen battles with Mega Evolution or move learning from slots 4–6.
- **Not tested at all:** mGBA GUI at normal speed by a person, real hardware, two-PC netplay. Harness success is not netplay evidence. The clips are harness recordings.

## Intended differences (not bugs)

- A badge, win or quest earned unlinked stays on that save after relinking.
- The partner's own puzzle objects (boulders) and personal rooms (Secret Bases, Pyramid and Trainer Hill floors) are not shared.
- Wild Pokémon and ordinary trainers use your first three Pokémon (no team screen); slots 4–6 sit out there.
- FOLLOW stops before grass, cave floors, other warps and scene tiles by design.
- Duo Tower battles give no EXP, prize money, bag use or catches; healed copies fight.
- SELECT, R+START and L+R do nothing developer-side any more (the developer menus are off). Since 0xC0170141, SELECT at the battle action menu opens the read-only battle conditions panel.

## Where things are (work environment, not committed)

- Final suite (0xC017013F) `/home/claude/journey/final13F/suite_run`; affected run (0xC0170140) `/home/claude/journey/final140/affected`; hunt run `/home/claude/journey/final140/hunt`; compatibility runs `/home/claude/journey/compat13F_b`, `/home/claude/journey/compat140` (+ `compat140_cmp.txt`); difficulty sample `/home/claude/journey/diff13F` (scenarios in `/tmp/claude-0/zzkeep`); clips `/home/claude/journey/clips13F`.
- Full suite: as for 0xC017013E (`OUT=… ROM=… HARNESS_OUT=… LONG=1 SEG6=… J06=… … J68=… OLD13C_COAST=… tools/coop_link_harness/run_all.sh`); compatibility check: `OLD_ROM=… OLD_HB=… NEW_HB=… OUT=… tools/coop_link_harness/compat_check.sh NAME:SAVEDIR …` from a built tree.

# Previous build: 0xC017013E

## Co-op stability, team preparation and Duo Tower: build 0xC017013E

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013D), pushed, not merged. Both players need this build: 0xC017013D and older refuse to link with it. Older saves load: the co-op save block grows (layout 4); the new fields start empty and nothing already saved moves. Test log: [COOP_TESTLOG_C017013E.md](COOP_TESTLOG_C017013E.md). Player-facing rules: [COOP.md](COOP.md) (Asking each other, Playing apart and relinking, Team preparation, Duo Frontier, Field kit).

- ROM `emerald-coop-C017013E.gba` from source `60aee4f6` (no game-code change after it; a clean clone of it builds the same SHA-1): SHA-1 `096ffa376ca81c35b12cfc4636f7956814ffbb86`, SHA-256 `e1a43599a03bebc7a6ec4ef512933ded00dffcf0e3244b8471ed3639a408b52f`.
- Commits, oldest first: release NEW GAME and early Poké Balls `328b1320`; availability, Beacon rules, clock-based invite timing (S2) `2f7c26f6`; team preparation (S3) `ccd71c3c`; wild double palettes and startup timing (S4) `3db87847`; APPEARANCE and move inspector (S5) `a674b770`; field kit and save layout 4 (S6) `5d7f3f35`; FOLLOW through doors, safer NPC corrections (S7) `41faccec`; relink ownership policy (S8) `e9421e14`; DUO TOWER (S9) `69662143`; sustained-session checks, outfit preview and duo result fixes (S10) `e3c63803`; harness timing fixes `d0852b53`, `c8a17759`; right-hand opponent's trainer name in one-trainer co-op fights `60aee4f6`; test and driver fixes after the earlier full runs `1f5291f7`, `5e5ecef9`, `95b57eff` (the final suite's harness) and after it `94dba5fe`; docs after them.
- Final suite: 281 scenarios, 279 PASS on the first run at harness `95b57eff`; the 2 failures were driver problems (a wandering NPC blocking a step; the catch driver pressing B on the co-op waiting note), fixed in the harness (`94dba5fe`); the 29 scenarios that change touches were rerun: 26 PASS, 3 FAIL (two rare-slot hunts and a reload that depends on one), all PASS on rerun. Details and revision coverage in the test log.

## Release instructions

1. Both players use the same file, `emerald-coop-C017013E.gba` (SHA-1 above), each with their own copy and their own `.sav`. A game on 0xC017013D or older will not link with it.
2. Back up both `.sav` files first. Older saves load (the co-op block gains new fields, empty at first); nothing in the party, PC or bag changes on loading.
3. mGBA: open one copy, File → New Multiplayer Window, open the other copy there; CO-OP → LINK UP on both. Two-PC netplay (RetroArch gpSP, Link Cable – Pokémon Gen3) was not tested for this build.
4. What to expect, in plain words: COOP.md, sections *Asking each other* (Beacon and consent), *Playing apart and relinking* (what stays personal), *Team preparation* (choosing three before story fights) and *Duo Frontier* (DUO TOWER rules, BP, records).

## What changed, by stage of the request

- **§9 Opening.** NEW GAME runs the real opening (Birch, boy/girl, name, van, room). The developer "skip to the clock" start is offered only when the harness arms it. The Oldale Mart sells Poké Balls from the start; catches before the Pokédex are recorded (`catch_before_dex`). Fresh paired saves with different clocks: `release_newgame_boys/_girls`.
- **§4 Availability and Beacon.** One availability check (`Coop_LocalAvailability`): battle, screen, message, question, transaction, scene or locked script (with or without text). Shared story fights need the Twin Beacon ON, the partner on the map, and the partner's YES; Beacon OFF says so and starts nothing; the opening rescue is the exception. A busy partner is never interrupted: the inviter asks again for up to 30 s. Simultaneous requests: link player 0 wins, the other joins. Invite and answer windows are about 30 s / 25 s of cartridge-clock time (frame fallback), with a countdown; B stops waiting; late answers start nothing.
- **§13 Team preparation.** Shared story fights (gyms, Elite Four, champion, rivals, Wally, villain bosses, quest bosses) open a team screen on both games after the YES: up to three from any slot (4–6 included), LEAD/SECOND/THIRD, READY / CHANGE TEAM / CANCEL; a revisioned, idempotent ready/commit exchange; no auto-confirm, no healing; every Pokémon returns to its own slot afterwards with its battle results. Unlinked gym/E4/champion: lead choice only.
- **§7 Battle visuals.** The persistent dark left wild Pokémon in co-op wild doubles was two overlapping intro fades (a link wild double was not treated as a wild double); fixed. Startup timing probe `gCoopStartTimes`; the join question is one page again. Measurements below.
- **§8 Menus.** CO-OP menu explains the current state in one line and has APPEARANCE (the wardrobe from anywhere). Move inspector (L in the move menu): category, PWR (var. with what decides it), ACC (sure), target, No PP / Disabled, description. Rival-name audit: no stale names in script text; the "DAWN" seen in testing came from the old developer start's name list.
- **§10 Field kit.** POKéMON BOX LINK (storage from the field, no healing) and HEALING VIAL (one Pokémon's HP and status, no revive, no PP; 1 charge + 1 per two badges; refilled at Centers). Both refused in Frontier facilities, Safari Zone, Trainer Hill, and while a trade, gift, invitation or team screen is open.
- **§6 FOLLOW** goes through ordinary doors, door mats, stairs, ladders and escalators when that warp leads to the partner's map; the follower's game warps itself; stops on any button, other warps, scene tiles, hazards, grass and cave floors, battles, menus. Each game counts only its own steps.
- **§5 NPCs.** Shared NPC corrections never place an object on a player, another object, a wall or a warp (dropped and repeated by the authority's next refresh). Authority after leave/return, interiors, link loss: `npc_town_order`, `npc_interior_talk`, `npc_reconnect`.
- **§3 Relink policy.** Personal: party, PC, money, bag, catches, purchases, pickups. Earned alone stays alone (badges, trainer/rival wins, quest steps). Earned together: recorded on both saves with the partner's trainer id; a relink with the same partner restores a joint outcome one save lost; never handed to another partner; no flag tables copied, nothing rolled back.
- **§11 Duo Frontier.** DUO TOWER is implemented (LV. 50 / OPEN LEVEL, three each, pair species/item clauses on both games, generated opponent pairs from the run id and battle number, healed copies, real party restored after every battle including aborts, BP per set paid once per save, separate duo records, milestone pairs at 21 and 49, resume from the earlier save, one partner per run). The other six facilities have a DUO DESK that says their duo mode is not available in this version.
- **§14 Sustained session.** Harness `leak_note`/`leak_check` count tasks, sprites, sprite palette tags and heap. `sustained_session` runs cancelled team screens, three duo battles, menus, an outfit change and FOLLOW in one session: all seven checks equal the first note on both games.

## Confirmed and fixed in this build

| Defect | First wrong state | Fix | Regression |
|---|---|---|---|
| Left wild Pokémon dark all battle in co-op wild doubles (both screens) | Two intro fades; the second dropped while the first ran (palette 3 faded ≠ unfaded) | Any non-trainer double uses the wild-double intro fade | `battle_visual_wild_double` (16 palette mismatches on old code, 0 now) |
| Story fights could start from a closing conversation without the partner's consent | Busy-retry launched when the partner's message closed | Story fights ask; Beacon rule; availability check | `story_consent`, `story_beacon_off`, `mandatory_busy_*` |
| "Not ready" note ran on into "The link dropped" (since S2) | `Coop_EventScript_NotReady` fell through | Script restored | `story_not_ready` |
| Duo battle abort could leave the battle copies in the real party (found while building S9, never released) | Restore order on abort | Real party restored first on every path | `duo_tower_abort` |
| Wardrobe preview wrote its palette over the player's field sprite and freed the player's palette tag | `tPicSprite` 0 (the player's sprite) during `ListMenuInit`'s first cursor callback | Preview starts as "no sprite"; 0xFFFF checked before s16 task data | `appearance_menu` leak check, `sustained_session` (fails on old code) |
| A refused DUO invite after a won battle showed "battle won!" again | Last outcome never cleared before a new attempt | Cleared in `CoopDuo_PrepareInvite` | `duo_tower_decline` (fails on old code) |
| In a co-op fight against one trainer, messages about the right-hand opponent named nobody ("…reacting to 's Mega Bracelet!"); the partner's game once stopped on "Opponent needs a valid name" at that message | Name lookup for the right-hand opponent read the empty second-trainer slot (opponentB = TRAINER_NONE) | Both opposing battlers name opponent A when there is no opponent B | `coop_wally_vr_partner` names both opposing battlers WALLY on both games (fails on old code: "") |
| Relink handed solo progress to the partner and merged joint wins with any partner | No provenance for joint outcomes | Partner trainer id on joint badges, rival stage, story ledger, quest bits | `relink_policy*`, `rival_reconnect`, `pp_reconnect`, `pp_stagger*` |

## Startup timing (§7)

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

## Remaining issues (not intended)

- **Finding #16** (an invalid opponent-name assertion after a co-op wild battle into trainer detection): not reproduced. Matrix of 24 runs (Teleport then trainer sight, simultaneous approaches, partner wild then Karen, three wait lengths, both declining and accepting partners) gave 0 assertions and 0 bad-trainer diagnostics; 5 of them ended in driver timeouts in the decline variants (the driver waited for a battle the partner had declined). The invite-side id validation stays. Not declared fixed.
- **Assert on the partner's game in a one-trainer co-op fight** (pass-1 suite, `coop_wally_vr_partner`, once): the message's name lookup is fixed (above), but why the partner's game asserted while the engine's game printed an empty name is not proven; 10 more runs of the scenario did not put Wally's Mega Gallade on the right-hand side again. The check now names both opposing battlers on both games.
- **Partner drawn on tiles blocked by the other save's puzzle state** (a boulder pushed on one save): unchanged; the partner sprite never collides, so it can appear on a tile that is solid on the viewer's save.
- **Shared dialogue with different personal progress**: joining a conversation still uses the initiator's script state; there is no explanation line when the partner's version differs (§5 asked for one). Not implemented.
- **§8 visual refinements not done:** rival hats, hair, portraits and movement frames; party, bag and battle menu restyling and move-type colour accents. Only APPEARANCE, the CO-OP menu explanation, the move inspector and the team screen changed.
- **§11:** DUO RENTALS, DUO DOME, DUO PALACE, DUO ARENA, DUO PIKE and DUO EXPEDITION are not implemented (each desk says so). No Mega-disabled bracket (Megas follow each player's own bracelet). No duo Frontier Brains; the 21/49 milestones are two fixed top Tower trainers.
- **Not played:** berry planting/harvest, Trick House, Illusion, Repel prompts around scripts and transitions, joined conversations with mismatched progress, postgame rematches and League returns with mismatched progress, prep battles with Mega Evolution or move learning from slots 4–6.
- **Not tested at all:** mGBA GUI at normal speed by a person, real hardware, two-PC netplay. Harness success is not netplay evidence.
- **Balance:** not established (assisted wins only; duo battles use `weak_foes`/`strong_battlers` fixtures).

## Intended differences (not bugs)

- A badge, win or quest earned unlinked stays on that save after relinking.
- The partner's own puzzle objects (boulders) and personal rooms (Secret Bases, Pyramid and Trainer Hill floors) are not shared.
- Wild Pokémon and ordinary trainers use your first three Pokémon (no team screen); slots 4–6 sit out there.
- FOLLOW stops before grass, cave floors, other warps and scene tiles by design.
- Duo Tower battles give no EXP, prize money, bag use or catches; healed copies fight.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN`; final suite `/home/claude/journey/final13E/pass3/suite`; clips `/home/claude/journey/clips13E/out` (raw recordings in `raw/`).
- Full suite: `OUT=dir ROM=… HARNESS_OUT=dir LONG=1 SEG6=… J06=… J13=… J16=… J37=… J43=… J44=… J46=… J48=… J50=… J51=… J52=… J57=… J58=… J59=… J65=… J66=… J68=… OLD13C_COAST=<0xC017013C suite>/coast_solo_j68 tools/coop_link_harness/run_all.sh`.

# Previous build: 0xC017013D

## Hoenn Co-op Battle Guide v2: build 0xC017013D

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013C), pushed, not merged. Both players need this build: 0xC017013C and older refuse to link with it (the invite carries a team key now). Older saves load; nothing in the save layout moved. Test log: [COOP_TESTLOG_C017013D.md](COOP_TESTLOG_C017013D.md). Team spec and every substitution: [COOP_TEAM_SPEC.md](COOP_TEAM_SPEC.md). Stones, sources and item balls: [COOP_MEGA_MANIFEST.md](COOP_MEGA_MANIFEST.md). Player-facing: [COOP.md](COOP.md) (Story rules, Level cap, Mega Evolution).

- ROM `emerald-coop-C017013D.gba` from source `1b26b863` (no game-code change after it): SHA-1 `7ec98060cb241e0749fbc4d73e43c93f4cf05527`, SHA-256 `1ef8450dbe64423bfa1868c08392504ef80c4c88726ccf775e3a5c0b0143f71b`.
- Commits: project instructions `104eab18`; team spec, progress-based selection, caps, duo starters `982e48a7`; Mega Evolution in co-op battles `bea2ac70`; bracelet, stones, personal item balls `026d3894`; boss pairs `84f76907`; harness and first scenarios `720845e3`; ball placement rules and quest-stone room check `8fe80b9a`; scenarios and fixture updates `1b26b863`; harness pace fix `c59a587e`; docs after it.
- Final suite: 241 scenarios, 237 pass on the first run. Of the 4 failures, 3 (an egg-roll frame budget; a harness START lost mid-step and the reload that depends on it) pass on rerun after the harness fix `c59a587e`; `roster_b3_catch` failed again (driver: a wild Stunfisk in the air after Bounce, the throw refused). Details in the test log.

## What changed

- **One team spec.** `tools/coop_trainers/battle_plan.json` (the guide's embedded plan) → `guide.py` writes the parties in `src/data/trainers.party` and `battle_partners.party`, the variant rules `src/data/coop_team_variants.h` and `docs/COOP_TEAM_SPEC.md`. `overhaul.py` skips the ids the guide owns; `bosses.py` is retired. 13 new party-only ids (926-938): Lilycove and hideout early variants and the exhibition case variants. Trainer flags stay on the base ids.
- **Team selection.** `src/coop_teams.c`: progress tier (0 before badge 1, 1-8 badges, 9 League admission = `FLAG_ENTERED_ELITE_FOUR`, 10 Champion) and the starter case make a team key. A linked fight uses the lower tier of the two players and the inviter's case (sent in the invite and every answer). Ordinary rematch tiers step down while their levels pass the cap.
- **Caps** 18 / 24-60 / 66 / 76 (League admission, kept) / 80 (Champion); a Pokémon at or below the cap obeys.
- **Duo starters** Torchic+Fennekin, Mudkip+Totodile, Treecko+Chikorita with their own level-up moves; rivals use the counter type; Soren owns the case line, Mira the companion line, from the save's recorded rival (never the party).
- **Mega Evolution** on in co-op and solo (Z-Moves, Dynamax, Tera stay off). The guest's move selection says whether a Mega is allowed and already used; the master takes a Mega only if still allowed; a withdrawn choice gives it back. The partner slot needs the partner's own bracelet. The guest's healthbox shows the Mega indicator. Party Megas revert after a co-op battle with HP kept. Mega Banette, Beedrill, Chimecho, Mawile and Sableye keep the revamped base forms' HP. `ITEM_MEGA_RING` is shown as the Mega Bracelet.
- **Bracelet** from Steven on Route 118 after badge 5 (`FLAG_RECEIVED_MEGA_BRACELET` 0x4A2), each save on its own; the Exchange's MEGA STONE clerk gives it to a 5-badge save without it.
- **Stones and item balls.** `tools/coop_mega/rewards.py` (+ `place.py`): 82 stones (Exchange tiers 5 badges / 7 badges / Champion, four quest rewards, 53 balls) and 31 other balls; 88 flags from unused ranges (0x21-0x4F, 0x4A3-0x4EF). Found stones are sold again; WHERE TO FIND gives hints.
- **Boss pairs.** The second trainer stands beside each leader, Elite Four member and the Champion (`data/scripts/coop_boss_pairs.inc`); talking to either starts the same fight.

## Migration notes

- No save layout change. New flags were unused before (listed in `include/constants/flags.h` with the generator's comment).
- Existing Pokémon are untouched; saves from before keep their old companion starters (Houndour, Wingull, Shroomish); the rival is still read from the save.
- A save past Route 118's scene gets its bracelet from the Exchange clerk; a save that finished a quest before gets that quest's stone the next time it talks to the quest's person (a full bag is told to come back).
- Trainer flags are unchanged (base ids keep them), so beaten trainers stay beaten.

## Known gaps

- **Balance is not established.** Unassisted runs with the main journey's parties lost to the Petalburg Woods grunt and to Rustboro's third gym trainer, and later fights were lost with the journey's level-boosted, early-move parties (test log, Balance). No balance claim.
- Played: Roxanne/Heath as a boss pair (solo and co-op), Lilycove early/late, Route 103 all three cases, exhibitions all three cases, Megas (simultaneous, cancel, abort, bracelet ownership, HP, MEGA SOL, DRAGONIZE). Not played: the other 11 boss-pair NPCs, Elite Four/Champion pairs, Gabby and Ty, rematch tiers and hideout variants in battle, the other Z-A abilities, most of the 84 balls.
- Reed at Dewford (3,3) stands where the old journey script walked to talk to Brawly from the side; Brawly is still reached from the front.
- Mega Milotic and other custom Megas deferred (as asked).
- Harness weak points: rare-slot hunts, the visual checker's absent-battler and Safari false positives, the driver cannot pick a replacement in the stock solo-with-partner party menu. mGBA GUI, hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN`; final suite `/home/claude/journey/final13D/suite`; harness reruns `/home/claude/journey/final13D/harness_rerun`; earlier full run `/home/claude/journey/pre13D`; clips `/home/claude/journey/clips13D`; balance probes (copies of journey scenarios) `/tmp/claude-0/zz`.
- Full suite: `OUT=dir ROM=… HARNESS_OUT=dir LONG=1 SEG6=… J06=… J13=… J16=… J37=… J43=… J44=… J46=… J48=… J50=… J51=… J52=… J57=… J58=… J59=… J65=… J66=… J68=… OLD13C_COAST=<0xC017013C suite>/coast_solo_j68 tools/coop_link_harness/run_all.sh`.

# Previous build: 0xC017013C

## Co-op compatibility audit: build 0xC017013C

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013B), pushed, not merged. Both players need this build; 0xC017013B and older refuse to link with it. Older saves load (no save layout change). Audit table: [COOP_AUDIT_C017013C.md](COOP_AUDIT_C017013C.md). Test log: [COOP_TESTLOG_C017013C.md](COOP_TESTLOG_C017013C.md).

- ROM `emerald-coop-C017013C.gba` from source `cfc98e64`: SHA-1 `68a24090dd7cfb2fbba8f58b2c4abe47895c958f`, SHA-256 `42277453eaea1f574b2d8bfe76a420a3b6257327e768be1bc780b6390a8059db`; a clean build gives the same SHA-1.
- Commits: busy partner, personal rooms, win flags `148fab0a`; item moves between allies `38d818f0`; boulders not synced `36c7a370`; deal Pokémon checked `8f9c09f6`; build id `cfc98e64`; harness and regressions `ca8436f1`; docs after it.
- Final suite: 215 scenarios, 211 pass; the 4 failures are test-driver luck or timing and one reload that depends on a failed run; all pass on rerun. Details in the test log.

## Confirmed and fixed

- **A.** A gym/rival/Elite Four/story fight started on a partner with a menu screen open (bag, party, PC, shop), replacing it without closing it. Now the partner answers busy; the inviter retries for 30 seconds, then explains.
- **B.** The same with a message on the partner's screen stopped the partner's game at the battle's start (heap check). Same fix.
- **C.** Trick between the two players' Pokémon left each save holding the other's item. Item moves between link multi allies now fail.
- **D.** In Secret Bases (and Pyramid / Trainer Hill floors) the partner was drawn in your own room. Now hidden there.
- **E.** Strength boulders were synced like wandering NPCs: a push on one save moved the other's boulder (it blocked `journey_60_seafloor`). Things are never synced now.
- Hardening: incoming deal Pokémon are validated; a shared win's flag must be one of the known win flags.

## Known gaps

- Not played: cable car, ferries, Trick House, Battle Frontier and rentals, contests, berry planting, Escape Rope/Dig, Illusion, Knock Off between allies (refused by the same rule), Repel prompts around scenes. See the audit table.
- A co-op loss costs no money (as stock link battles); documented, not changed.
- Partner sprites can stand on tiles blocked in your own puzzle state (drawing only).
- Harness weak points: rare-slot hunts (`roster_b3_catch`), A-only early battles, walks that meet a wandering NPC. Finding #16 still unreproduced. mGBA GUI, hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN`; before-fix logs `/home/claude/journey/audit`; 0xC017013B runs `/tmp/claude-0/runsB`; final suite `/home/claude/journey/final13C/suite`; clips `/home/claude/journey/clips13C`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J06=… J13=… J16=… J37=… J43=… J44=… J48=… J52=… J59=… J65=… J68=… tools/coop_link_harness/run_all.sh`.

# Previous build: 0xC017013B

## Trainer Exchange and breeding: build 0xC017013B

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013A), pushed, not merged. Both players need this build; 0xC017013A and older refuse to link with it. Older saves load (no save layout change; a save standing where the new building is continues in front of its door). Test log: [COOP_TESTLOG_C017013B.md](COOP_TESTLOG_C017013B.md). Checklist: [COOP_CHECKLIST_C017013B.md](COOP_CHECKLIST_C017013B.md). Stock, prices and unlocks: [COOP_EXCHANGE_STOCK.md](COOP_EXCHANGE_STOCK.md). Player-facing: [COOP.md](COOP.md) (Trainer Exchange, Breeding and the Day Care, Level cap, Known limits).

- ROM `emerald-coop-C017013B.gba` from source `f6e902ee`: SHA-1 `4833248dfa632bbb09f04ab289f9fea70cdecfef`, SHA-256 `db68ff86c3ba5207e1acd2bd429b6f8d7d2ceeb879ebac312794e3ea1aaa7c0a`. A clean build in a fresh worktree gives the same SHA-1.
- Commits: Exchange, breeding guide, Day Care side by side, old-save repair `e0eef008`; EXP candy cap message `536b98ec`; build id `0f4e0d0c`; harness and scenarios `f6e902ee` (includes the ROM's inert test hook additions); docs after it.
- Final suite on this ROM: 209 scenarios, 206 pass. The 3 failures: one scenario file overwritten by its generator (fixed, passes), one early battle lost by the A-only driver (passes on rerun), the surf hunt in `roster_b3_catch` (driver; passes on rerun).

## What changed

- **Trainer Exchange** in the south of Rustboro (door (23,52), seven steps east and seven south of the Mart's door), open before Roxanne: MOVES & TMs (TMs, 91 tutor moves by type with species compatibility, relearning ₽1,000, egg moves ₽5,000 from 3 badges), BATTLE ITEMS, EVOLUTION (every roster evolution item), TRAINING & NATURE (candies, feathers, vitamins, EV berries, mints, PP, Ability Capsule/Patch; EV reset ₽2,000; Hyper Training ₽15,000 a stat from 7 badges, ₽60,000 all six as Champion), BREEDING (Everstone, Power items, Destiny Knot, Mirror Herb, guide). Stock by the buyer's own badges (0/1/3/5/7/Champion), COMING SOON per counter. Data in `tools/coop_exchange/stock.py` → `src/data/coop_exchange_stock.h`, `docs/COOP_EXCHANGE_STOCK.md`, the `extraTutors` list.
- **One price per item** in every shop (`GetItemPrice` → `CoopExchange_PriceOverride`).
- **Co-op**: both players can use one counter (and the Day Care lady and man) at the same time, each on their own save; a shopper shows the menu icon and the partner menu says they're busy.
- **EXP candies** that reach the level cap say so; at the cap they are refused and kept.
- **Breeding guide** at the Exchange and in the Day Care.

## Move access

TMs stay single-use (as before) and are renumbered nowhere. The tutor adds 91 moves (9 from the start, 28 with 1 badge, 48 with 3, 72 with 5, 88 with 7, 91 as Champion) across all 18 types with the TMs; signature moves excluded (checked against `signatureTeachables`); each is offered only to species whose teachable learnset has it (at least 30 species each, median 162). RELEARN covers level-up and evolution moves (`MOVE_RELEARNER_LEVEL_UP_MOVES`); EGG MOVE teaches any egg move of the family (`MOVE_RELEARNER_EGG_MOVES`).

## Breeding, mints, Hyper Training (this engine)

Egg = mother's first stage (Ditto: the other parent's); babies without incense; Undiscovered can't breed. Everstone passes the holder's personality nature, not a mint's. Three IVs inherited, five with Destiny Knot; Power items pass their stat; Hyper Training is a separate bit and is not inherited (played: hyper-trained parents with IVs 0 gave an egg with IVs 0 and no hyper bits). Egg moves from either parent; Mirror Herb holder with a free slot learns the other's at deposit. Regional form: the mother's with an Everstone, otherwise the Hoenn form. Eggs can't battle, be traded or be given. Ditto: Desert Underpass after the Hall of Fame.

## Known gaps

- Breeding was played with representative families (Vulpix/Alolan Vulpix, Marill/Azurill, Pichu, Ditto) and checked from the source; not every species.
- A partner at an Exchange counter or the Day Care is reported as "shopping".
- A badge won together in a co-op gym battle is restored to a save that lacks it on LINK UP (the joint-badge record); that is the only way a partner can add to your badges and so to your stock.
- Not played: the Day Care across a reconnect (nothing of it crosses the link; source), buying with both players' bags full, gifts of every item kind.
- Harness weak points, not game faults: hunts for rare slots and A-only early battles. mGBA GUI, real hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN` (p0 Kai, p1 Rosa; j06 before Roxanne), story_seg6 `/tmp/claude-0/seg6`.
- Final suite output `/home/claude/journey/final13B/suite`; clips `/home/claude/journey/clips13B`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J06=… J16=… J37=… J43=… J48=… J52=… J65=… J68=… tools/coop_link_harness/run_all.sh`.

## Previous build: 0xC017013A

## Co-op polish: build 0xC017013A

Branch `coop-doorway-scenes-rivals` (continued from 0xC0170139), pushed, not merged. Both players need this build; 0xC0170139 and older refuse to link with it. Older saves load (no save layout change). Test log: [COOP_TESTLOG_C017013A.md](COOP_TESTLOG_C017013A.md). Checklist: [COOP_CHECKLIST_C017013A.md](COOP_CHECKLIST_C017013A.md). Player-facing description: [COOP.md](COOP.md) (How a session works, Party in co-op battles, With your partner, Quality of life, Known limits).

- ROM `emerald-coop-C017013A.gba` from source `a714c7f3`: SHA-1 `cb64e3cf311cc0cc4efae804da55b41e828137e8`, SHA-256 `0231ef9b2c1804e37e8550b1d91910a7d0a52cb9d2e8e36e329c47588c8dc154`.
- Commits: activity icons, arrow, PING, VIEW TEAM, FOLLOW `f9d07377`; shared conversations, battle targets and changes, evolution hints `4805ee12`; build id `88a6aa23`; scenarios `d327521f`; popup width `cda8c960`; busy-note reasons `6d871d34`; scene-announcement retry and B grace `ef024478`; harness `5d0460bd`; busy reason only when needed `a714c7f3`; harness budgets `2d1f1d2b`; docs after it.
- Final suite on this ROM: 194 scenarios, 189 pass. The 5 failures are test-driver luck and one timing-sensitive comparison (two early battles lost by the A-only driver, a bobbing sprite compared, a teleporting Abra, a rare Safari hunt); the scenarios were adjusted where that made sense and rerun on the same ROM: four pass; `roster_b3_catch` still caught 8 of 9 on rerun (details in the test log). Earlier: 0xC0170139 188 scenarios, 187 pass.

## What changed

- **Activity icons** over the partner (walk, talk, menu, battle, "?" invitation, waiting tick, broken link), from the partner's own state, debounced, never blinking; a steady **edge arrow** to a nearby off-screen partner (standard now; the shrine lore still unlocks BEACON STYLE colours).
- **Partner menu:** VIEW TEAM (read-only snapshot, age shown when the partner can't answer), PING (five-second cooldown; also in CO-OP), FOLLOW (walks the partner's trail on foot; stops on any button and before doors, scene tiles, ledges, water, hazards and wild terrain; no loops).
- **Joining a conversation:** A on the person the partner talks to (or on the partner) offers to join; each screen reads at its own pace; questions and movements wait for both; the host answers and the joiner sees the answer; B leaves or goes ahead; a lost link frees both. Shops, gifts, battles, warps and special scripts aren't shared; a short corner note says why.
- **Battle:** target prompts (FOE (LEFT/RIGHT), ALLY, YOURSELF, BOTH FOES, ALL OTHER POKéMON, EVERYONE, YOUR SIDE, THE FOES' SIDE, THE WHOLE FIELD) with an ally warning for damaging moves; B on the waiting message takes back a move, switch or RUN while the turn is open (master decides, both games agree; late requests refused; items can't be taken back).
- **Summary screen:** SELECT on the INFO page pages through evolution hints (target, method, conditions, met now or not, where to get the item).
- **Fixes found in testing:** scene announcements could be lost when the link was busy; long popups clipped their last letter; B right after choosing could request a change; the memo's SELECT cue overflowed; the busy-note reason was sent too often and delayed a quest's progress message.

## Known gaps

- A joined conversation follows the host's story state; the joiner isn't told when their own save would get different lines.
- Not played: the edge arrow on a connected map; FOLLOW stops at ledges, water, ice and scene tiles (source only); joining across a power-off (nothing to resume: session state is not saved).
- Items chosen in battle can't be taken back. FOLLOW is on foot only and stops at wild terrain.
- Harness weak points, not game faults: hunts for rare slots (`roster_b3_catch` caught 8 of 9 in this build's runs; its surf hunt paces only up and down and can stall in an east-west channel); A-only battles in a few early scenarios (now `smart_moves`).
- Finding #16 still unreproduced. mGBA GUI, real hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN` (p0 Kai, p1 Rosa), story_seg6 `/tmp/claude-0/seg6`.
- Final suite output `/home/claude/journey/final13A/suite`; clips `/home/claude/journey/clips13A`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J16=… J37=… J43=… J48=… J52=… J65=… J68=… tools/coop_link_harness/run_all.sh`.

# Previous build: 0xC0170139

## Ordinary roster complete, Pokédex 910: build 0xC0170139

Branch `coop-doorway-scenes-rivals` (continued from 0xC0170138), pushed, not merged. Both players need this build; 0xC0170138 and older refuse to link with it. Older saves load (save layout and species ids unchanged). Test log: [COOP_TESTLOG_C0170139.md](COOP_TESTLOG_C0170139.md). Checklist: [COOP_CHECKLIST_C0170139.md](COOP_CHECKLIST_C0170139.md). Whole-roster manifest: [COOP_ROSTER_ALL.md](COOP_ROSTER_ALL.md) (this batch alone: [COOP_ROSTER_BATCH3.md](COOP_ROSTER_BATCH3.md); data: `docs/coop_roster_manifest.json`).

- ROM `emerald-coop-C0170139.gba` from source `d4d5a863`: SHA-1 `a2456d1f99c13a852d1d8134e997e6a54a5f6978`, SHA-256 `bb8da58d8400d5cff39d2f7c5b498e227265b9f099b4cb69beab005e12846bac`.
- Commits: roster batch 3 `367e36e1`, harness and scenarios `44d6eedb`, build id `d911ba53`, Hoenn Pokédex `d4d5a863`, harness fixes `a47896d5` (test tools only; the ROM is unchanged), docs after it.
- Final suite on this ROM: 188 scenarios, 187 pass. The failure, `late_wild_tour`, is a hunt that did not catch a 4% Drifloon in time (luck), then a test-driver loop on a move with no PP; with the driver fixed it passed three reruns. Earlier: 0xC0170138 180 scenarios, 179 pass.

## 1. Totals

| | Count |
| --- | --- |
| Obtainable species, one save, alone | **908**: the whole ordinary roster (**900 of 900**: every National Dex species except the 125 deferred legendary, mythical, Ultra Beast and Paradox ones) plus the 8 Hoenn legendaries already in the game |
| Obtainable before this build (0xC0170138) | 680 (3 of this batch's 230 candidates were among them: Meowth and Persian by a one-time trade, Bonsly by Day Care egg) |
| Newly obtainable this build | **228** species: the other 227 candidates and Ursaluna (Hisuian evolution of Ursaring, not on the list) |
| Regional forms obtainable | **54** (Alolan 18, Galarian 16, Hisuian 16, Paldean 4): 7 before (Alolan Vulpix, Ninetales, Grimer, Muk; Galarian Corsola, Zigzagoon, Linoone) + 47 new. Not counted as species |
| Other forms | 991 forms in all across the 908 species (cosmetic and battle forms included) |
| Hoenn Pokédex | 910 entries (was 646): every obtainable species, plus Jirachi and Deoxys (event-only, as in stock) |
| New wild placements | 143 slots in 69 tables on 58 maps; 10 already-obtainable species also gained a wild source (mostly through their regional forms) |
| Excluded (deferred) | 125 National Dex species, listed in the checklist §2; the 8 Hoenn legendaries among them keep their existing encounters. Custom Megas (Mega Milotic) deferred |

## 2. How the hard cases work (all solo, all renewable)

- **Regional branches by place** (`GetEvolutionRegion`, src/pokemon.c): Alola = Dewford Town, Granite Cave, Routes 106 and 107; Galar = Route 114; Hisui = Mt. Pyre. Same evolution elsewhere gives the ordinary form. Played: Alolan Raichu in Dewford and Raichu on Route 114 from the same item; Alolan Marowak in Dewford, Marowak on Route 114; Hisuian Typhlosion, Sliggoo and Goodra on Mt. Pyre; Galarian Weezing and Mr. Mime on Route 114.
- **Trade evolutions:** Linking Cord (sold), or the held item used. Karrablast and Shelmet: Linking Cord with the other in the party (played both ways).
- **Galar fossils:** the Fossil Maniac offers the four pairs daily; DEVON revives (Dracozolt played).
- **Runerigus:** a Galarian Yamask down 49 HP touches the cracked carving in the Sandburied Ruins (played; the carving explains itself with no such Pokémon).
- **Battle conditions:** Annihilape (Rage Fist ×20), Sirfetch'd (3 critical hits in a battle): played in solo battles. Co-op battles are link battles and, like stock link battles, do not count them; the field guide says "solo battles".
- **Steps:** Rabsca after 1000 steps leading (played). The counter is not saved (power-off restarts it) and restarts on a lead change.
- **Other mechanisms played:** Tyrogue's three branches (stats via nature), Burmy's Sandy and Trash cloaks then Wormadam, male Burmy → Mothim, female Combee, both Meowstic, Happiny with Oval Stone by day, friendship (Blissey, Snorlax, Lopunny), known moves (Yanmega, Tangrowth, Lickilicky, Overqwil, both Dudunsparce), Mantyke with Remoraid, items (Gengar, Steelix, Electivire, Aromatisse, Slurpuff, both Sinistcha, Archaludon, Kleavor, Sneasler by day, both Galarian Slowpoke evolutions), Ursaluna (Peat Block, night, Mt. Pyre), Inkay at 30, Clodsire, Perrserker, Mr. Rime, Obstagoon, Cursola, Oricorio's nectar.
- **Changed from stock, explained in game:** Inkay at level 30 (no upside-down console); Ursaluna at night on Mt. Pyre (no full moon); Goodra in rain or fog (Route 119, Mt. Pyre summit, Rusturf Tunnel).

## 3. Pokédex

- 910 unique Hoenn entries; every obtainable species has one (now checked by `roster.py --check`); 494 AREA acquisition hints, each fits its line.
- Diplomas: legendary, mythical, Ultra Beast and Paradox entries are optional (a save with one of Latias and Latios can finish).
- Played: LIST BOTTOM reaches Hoenn No 910 (Miltank) and National No 1025; entry and AREA pages; type search; seen/caught from catches, evolutions, fossil, trade; partner markers after UNLINK/LINK UP and after a power-on.

## 4. Known gaps

- Forms: Vivillon Icy Snow only; Flabébé line Red only; Furfrou untrimmed; Minior red core; Squawkabilly Green; Dudunsparce three-segment 1 in 100.
- Not played this build: a real co-op trade evolving Karrablast/Shelmet (Linking Cord path played); breeding a regional form with an Everstone; the other three Galar fossil pairs and nectars (same code as the one played); most placements by hand (11 species caught in play, at least one per encounter method and habitat; all placements source-checked by the audit). 112 species across the three batches are now obtained in play (`verified.json`).
- Revamps: all earlier ones preserved (328 species checked, 0 differences). No new revamps this build; the wider balance plan is not part of it.
- Finding #16 still unreproduced. mGBA GUI, real hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN` (p0 Kai, p1 Rosa), story_seg6 `/tmp/claude-0/seg6`.
- Final suite output `/home/claude/journey/final139/suite`; clips `/home/claude/journey/clips139`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J16=… J37=… J43=… J48=… J52=… J65=… J68=… tools/coop_link_harness/run_all.sh`.
