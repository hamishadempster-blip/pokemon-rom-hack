# Two-player link harness

Runs two copies of `pokeemerald.gba` in mGBA 0.10.2 cores joined by the emulated
link cable (the same lockstep code as mGBA's Multiplayer window), headless, with
scripted input. It reads game state from RAM through `pokeemerald.elf` symbols,
so a script can wait for a callback, a position, or a co-op variable instead of
guessing frame counts.

## Build

```sh
make -j4                          # the ROM, elf, and build/emerald/src/coop.o
tools/coop_link_harness/build.sh  # clones and builds libmgba 0.10.2 on first run
```

Needs `gcc`, `g++`, `cmake`, and `zlib1g-dev`. The binary and `syms.txt` go to
`/tmp/coop_harness_bin` (`HARNESS_OUT` overrides it). Rebuild the symbols after
every ROM build.

## Run

```sh
tools/coop_link_harness/run_scenario.sh tools/coop_link_harness/scenarios/littleroot.txt
tools/coop_link_harness/run_scenario.sh tools/coop_link_harness/scenarios/rescue.txt
```

`littleroot.txt` links two fresh saves and walks both players out of their
houses. `rescue.txt` continues through the twin block, Birch's bag, the
Poochyena double, and the warp into the lab.

Player 0 is the cable master. Screenshots and failure dumps go to the output
directory. The exit code is non-zero when any `until_*`, `goto`, or `expect`
step fails.

## Script

One command per line, `#` starts a comment. The first word is the player:
`0`, `1`, or `b` for both. A `b` line other than `press` and `wait` first waits
for both players to reach it.

| Command | Meaning |
| --- | --- |
| `press KEYS [frames] [repeat]` | Hold `A+B+UP...` for frames (default 2), then release 6 frames |
| `wait N` | Idle N frames |
| `goto X Y [timeout]` | Walk to map-local tile X,Y |
| `until_field [timeout]` | Wait for the overworld with controls free and no script |
| `mash_field [timeout] [KEYS]` | Same, tapping A (or KEYS) every 24 frames |
| `until_cb2 SYMBOL [timeout]` | Wait for `gMain.callback2` |
| `until_mem SYM[+off][:size] VALUE [timeout]` | Wait for a RAM value |
| `expect SYM[+off][:size] VALUE` | Check a RAM value now |
| `dump [tag]` | Print callbacks, map, position, partner sprite, co-op state, and CPU pc/lr (plus battle and link state in a battle) |
| `expect_partner [tag]` | Partner sprite is on the other player's tile. A shared tile is drawn one step south, the same rule as `CoopProtocol_VisiblePartnerTile` |
| `expect_battle [moves|hp]` | Roster, distinct move slots, or mirrored party HP. The guest does not run the battle engine, so `gBattleMons` is only filled on the link master |
| `menu_to SAVE\|COOP` | Move the start-menu cursor onto that entry, whatever the current list is |
| `expect_nz SYM[:size]` | The value is not zero |
| `expect_no_partner [tag]` | No partner sprite (other map) |
| `hex SYM [len] [off]` | Print raw bytes |
| `shot NAME` | Write `NAME_p0.png` and `NAME_p1.png` |
| `include FILE` | Inline another script |

`coop.<name>` resolves the statics in `src/coop.c`.

## Progression and reconnect regressions

Run `save_lab.txt` once, then supply its two saves to the reload tests:

```sh
SAVE0=/tmp/coop_harness/save_lab/p0.sav SAVE1=/tmp/coop_harness/save_lab/p1.sav \
  tools/coop_link_harness/run_scenario.sh tools/coop_link_harness/scenarios/route102_reconnect.txt
SAVE0=/tmp/coop_harness/save_lab/p0.sav SAVE1=/tmp/coop_harness/save_lab/p1.sav \
  tools/coop_link_harness/run_scenario.sh tools/coop_link_harness/scenarios/solo_six.txt
tools/coop_link_harness/run_scenario.sh tools/coop_link_harness/scenarios/guest_move.txt
tools/coop_link_harness/run_scenario.sh tools/coop_link_harness/scenarios/guest_evolution.txt
```

`route102_reconnect.txt` continues both saves, reconnects, uses the Beacon,
walks to Route 102, wins a wild double, and checks the session and Beacon remain
on. It marks the opening adventure flag and Oldale state complete to bypass
the Route 103 story gate; it does not test that rival sequence.

`guest_move.txt` seeds the guest's real encrypted party data and the master's
matching partner copy immediately below level 16, with four moves. The actual
rescue battle awards EXP. The guest must learn Bite, replace Scratch, reach
level 16, and return to the lab. `guest_evolution.txt` uses a Torchic at the
level-16 boundary and checks that the guest finishes as Combusken. Cached
pre-level stats are deliberately unchanged by these fixtures; they are not
balance or damage tests. Production EXP, stat recalculation, menu prompts,
evolution, link traffic, and field callbacks execute normally.

`solo_six.txt` fills the local party with six copies of the lead as a fixture,
starts a real solo wild battle on Route 101 while the Beacon is on, chooses
slot six through the party UI, wins, and checks all six remain in the party.

Additional script commands:

| Command | Meaning |
| --- | --- |
| `fixture_guest SPECIES LEVEL EXP` | On both cores, seed the guest lead and its mirrored partner copy; use `b` |
| `fixture_lead SPECIES LEVEL EXP` | Seed this core's own lead |
| `fixture_six` | Fill all six local slots with copies of the lead |
| `fixture_flag ID VALUE` | Set/clear a saved story flag |
| `fixture_var ID VALUE` | Set a saved variable (0x4000 range) |
| `write SYM[+off][:size] VALUE` | Write a test setup value to RAM |
| `expect_lead SPECIES LEVEL` | Check local lead species and level |
| `expect_move MOVE` / `expect_no_move MOVE` | Check local lead's decrypted moves |

`until_mem` accepts optional mash keys after its timeout. Save offsets for
fixtures are compiled from this tree's `SaveBlock1` using the ARM ABI during
`build.sh`; they are not copied from vanilla Emerald's layout.

## Verified 2026-10-24

ROM build and host protocol tests pass. Two mGBA 0.10.2 cores completed the
opening/rescue/save scenario, guest Bite replacement, guest Torchic-to-Combusken
evolution, reload/reconnect/Beacon/Route 102 wild win, and a solo win using
party slot six, with zero harness failures. These are local emulated cable
tests. Internet netplay, move refusal, evolution cancellation, special
battle-condition evolutions, and a cable pull during a level-up remain untested.

## Target navigation and story regressions (build 0xC017012B)

The guest now receives a compact controller snapshot of current battler HP,
types, species, stats, party slots, and the master's legal-target masks.
Its battle engine still does not run. Navigation is bounded to four candidates,
even if a future packet contains no legal target. The snapshot is statically
checked against the actual 256-byte transfer buffer, not just the larger receive
buffer. Legal masks are keyed by move ID so reordering moves cannot mismatch them.

The earlier rescue test confirmed moves immediately. `target_navigation.txt`
now lets the host confirm first, delays the guest, cycles all four directions,
backs out, chooses Ember again, verifies Ember reached the master, and wins.
Its intro check reads the real VS task data (one Pokémon per player), rather
than inferring indicators from the party. Captures 90 idle frames apart have
identical PP-panel pixels. This scenario also passes with mGBA 0.10.3, matching
the reported desktop emulator version.

Additional scenarios:

| Scenario | Verified behavior |
| --- | --- |
| `spread_selection.txt` | Leer explicitly targets both foes; B restores move names; the fight completes |
| `trainer_reconnect.txt` | One-foe Calvin co-op battle, target cycling, both saves' defeated flag |
| `coop_six.txt` | Six Pokémon each before battle, three each in battle/intro, all six restored after victory |
| `rival_story.txt` | Mandatory Route 103 pair; both story flags set; rival removed on both games |
| `doors_stagger.txt` | Partner hidden across different maps and restored after meeting |

The trainer, six-party, rival, reconnect, solo, guest replacement/evolution,
and opening scenarios pass on two local 0.10.2 cores. The delayed Ember scenario
also passes on two 0.10.3 cores. `rival_story.txt` uses level/strong-stat fixtures
to isolate story handoff from battle balance; it does not validate rival difficulty.
The shared lab-save fixtures and bypassed Oldale adventure gate retain the
scope described above. These runs do not prove every late-game trainer, catch,
trade, or internet-netplay path.

`expect_vs_party P0_COUNT P1_COUNT` checks the intro's actual indicators.
`expect_flag ID VALUE` and `expect_var ID VALUE` check persistent state.
`expect_no_object LOCAL_ID` checks that a completed story NPC is gone from
the current map. The `oldale_reconnect.inc` and `route102_reconnect_start.inc`
files share setup without duplicating the opening route traversal.

## Doorway, scene, rival, and invitation regressions (build 0xC017012C)

`run_all.sh` runs every scenario below in dependency order and writes
`summary.txt` (`ROM`, `HARNESS_OUT`, `OUT`, `ONLY`, and `AFFECTED` select the
image, the harness build, the output folder, a subset, and an affected-save
folder for the recovery check).

| Scenario | What it checks |
| --- | --- |
| `mays_house_doorway.txt` | The reported bug from legitimate play: two boys, opening fight, Twin Beacon, then May's bedroom. Player 0 opens the bag. Both saves keep the rival away; both players take the stairs down and back twice, unlink, repeat, relink, repeat, and save in the bedroom. No flag or variable is edited. |
| `mays_house_doorway_p1bag.txt` | The same with player 1 opening the bag, and player 0 reading text at slow speed against player 1 at fast speed from the rival meeting on. |
| `bedroom_reload.txt` | Continue the two bedroom saves from `mays_house_doorway.txt`; rival still away, story values unchanged, stairs both ways, relink. |
| `mays_house_recovery.txt` | Continue a pair of saves made by the **pre-fix** build at the moment of the bug (the guest saved in the bedroom with May on the stair warp). The rival is gone on load, the story values are unchanged, and both players can use the stairs and relink. |
| `rival_join.txt` | Player 0 starts the bedroom introduction; player 1, idle in the room, joins from their own save. |
| `rival_latejoin.txt` | Player 1 has the start menu open during player 0's introduction; nothing interrupts it, and player 1 joins when the menu closes. |
| `rival_pending.txt` | Player 1 is downstairs; on entering the bedroom the finished introduction is applied at the transition: rival at the PC, short line, no replay. |
| `rival_simultaneous.txt` | Both press A on the Poké Ball on the same frame; each save plays its own introduction once, no join, no lock, both leave by the stairs. |
| `rival_reconnect.txt` | Player 0 meets the rival while unlinked; after relinking player 1 does NOT receive it (0xC017013E policy: unlinked progress is personal) and meets May on their own save. |
| `rival_story.txt` | (extended) Both twins stand on Route 103 on both screens before the fight; the second twin leaves with the rival; the guest also gets lab state 4 and the lab rival. |
| `invite_refuse.txt` | Nearby wild invitation answered NO: the inviter fights alone, the link stays up. |
| `invite_busy.txt` | Wild invitation while the partner has the start menu open: no prompt, the menu stays usable, the inviter fights alone. |
| `coop_menu.txt` | Start menu status window, partner activity in the frame, the CO-OP list with B back to the start menu, and the Beacon turned off from the menu once both agree. |

`route102_reconnect.txt` now has player 1 answer the join question with A.

Commands added for these: `expect_map G N`, `expect_object ID X Y`,
`expect_any_object ID`, and `text_speed N` (0 slow, 1 mid, 2 fast, a
player option, not story state). The shared-tile partner check accepts any
free neighbouring tile.

Fixture scope: `rival_story.txt`, `invite_*.txt`, and `coop_menu.txt` start
from `save_lab.txt` saves and the Oldale adventure fixture described above.
The bedroom and rival-introduction scenarios use only legitimate play.
The recovery scenario uses saves the old build produced, not a save from a
player's own game.

## Verified 2026-10-24 (build 0xC017012C)

ROM build, host protocol tests (`tools/coop_protocol_test.c`), and
`run_all.sh` on two linked mGBA 0.10.2 cores: 27 scenarios, all passing,
including `mays_house_recovery.txt` on saves the pre-fix build produced.
`route102_reconnect.txt` first failed once because no wild Pokémon rolled
during its fixed pacing (neither game left the field); it now paces until a
battle starts and passed four further runs (two per emulator version).
On two mGBA 0.10.3 cores: both doorway roles, bedroom reload, recovery, rival
join and simultaneous press, the lab save, Route 103 rivals, the Route 102 wild
double, the refused invitation, the CO-OP menu, and the Calvin co-op fight,
all passing.

The same doorway scenario fails on the pre-fix build exactly as reported:
the guest's save has May on the stair warp and the guest cannot go down.

Not run here: internet netplay, late-game rival scenes (lab Pokédex scene,
Rustboro, Routes 110/119, Lilycove), a real affected save from a player's
own game, and a cable pull during the new prompts.


## Persistent session regressions (build 0xC017012D)

New lab-save scenarios: `center_beacon`, `link_recovery`, `wild_route101`,
`wild_route101_p1`, `trainer_p1`, `rival_p1`, `battle_abort_recovery`, and
`invite_timeout_recovery`. They cover both initiators, Center entry/healing/
exit with staggered players, application heartbeat outages, expired invites,
explicit unilateral unlink, and a completed double after recovery.

`force_encounter 1|0` (a fixture) patches this core's `WildEncounterCheck` to
return TRUE, so its next grass step rolls an encounter; `0` restores the
original code. `wild_simultaneous` uses it on both cores to make two
encounters land on the same frame. `qol` covers the party cursor memory and
the quick repeat heal at a nurse.

Build 0xC017012F scenarios: `solo_opening` (fresh saves, never linked:
rescue, lab), `solo_rival` (unlinked Route 103 fight against each save's own
twin, both assignments), `solo_gym` (unlinked Roxanne + Heath double),
`new_species_evo` (a guest's Shinx evolves into Luxio after a co-op win),
`rotom_catalog` (the New Mauville scientist, one catalog per save),
`trainer_overhaul` (Tiana's rebuilt team as a co-op battle), and `e4_sidney`
(co-op Elite Four double against Sidney and Noir). `expect_range SYM:size LO
HI` checks a range. `mash_field` in a battle's party menu steps the cursor to
a healthy Pokemon and backs out of a summary screen, so forced switches
after a faint no longer stall a scenario.

`mute_coop FRAMES` suppresses this core's application frame sender by
patching its first instruction to `bx lr`, then restores the original
instruction automatically. It does not disconnect the serial cable. The
battle-abort scenario injects an engine error flag; it does not validate
physical cable-loss detection. Field messages are dismissed using
`mash_field TIMEOUT B`; `until_field` does not press buttons.

To recreate a persisted Zigzagoon bug, run `zigzagoon_affected_save.txt`
against build C017012C with old `save_lab` saves (player 0 opened the bag).
The guest must show object 4 and hide flag 0, while the opener has flag 1.
This saves on Route 101. Then run `zigzagoon_reload.txt` on build D with
those saved files. Both must hide the actor immediately on Continue, retain
route state 3, the original level-5 leads and save counter 2. Set
`ZIG_AFFECTED=/path/to/old-route101-saves` to add this check to `run_all.sh`.
The affected-save producer is deliberately not part of the new-ROM suite.

Final run results and fixture limits: `docs/COOP_REGRESSION_C017012E.md` and
`docs/COOP_HANDOFF.md`.

## Story runs on real walking (build 0xC0170130)

Long progression runs walk the maps instead of warping. New verbs:

| Command | Meaning |
|---|---|
| `walk X Y [timeout]` | Path-find (BFS over collision, elevation, ledges, water, objects) to map tile X,Y and walk there; a door/warp goal tile is allowed. Off the field (a scene, a trainer who spotted the player) it taps A. The timeout counts overworld frames only. |
| `walk_battle X Y [timeout]` | The same, but succeeds as soon as a battle starts on the way. |
| `cross DIR GROUP NUM [timeout]` | Hold DIR until on map GROUP.NUM with field control (edges, arrow warps, doors). |
| `until_map GROUP NUM [timeout]` | Wait (no keys) until on that map with field control. |
| `until_task FUNC [timeout] [KEYS]` | Wait for an active task with that function (e.g. `Task_HandleMultichoiceInput`). |
| `no_wild 1\|0` | FIXTURE: hold `sWildEncountersDisabled` while walking. |
| `strong_battlers N` | FIXTURE: in battle, set attack..spDefense of this game's battlers (0 and 2) in `gBattleMons` to N every frame. Nothing reaches the party or the save (a level-up recalculates party stats, which is why party stat writes do not survive a run). |
| `smart_moves 1\|0` | Test driver only: in battle, keep the move cursor on the strongest damaging move with PP, read from the move menu's own data (`bufferA[b]+4`), so mashing A attacks on the guest too. |
| `expect_mirror SYM[+off]:size SYM[+off]` | A value on this game equals one on the other game (after a `sync`). |
| `hexp PTRSYM LEN OFF` | Hex dump through a pointer (e.g. `gSaveBlock1Ptr`). |

`tools/coop_link_harness/edges.py MapName` prints a map's passable edge
tiles and warps, for choosing `walk`/`cross` targets.

Story chain (each from the saves the one before made; `run_all.sh` runs
segments 1–2 always and 3–6 plus `catch_walk` with `LONG=1`):

| Scenario | Covers |
|---|---|
| `story_seg1` | Lab saves: Mom's Running Shoes, Routes 101/103, co-op twins, Oldale rival scene, staggered lab entry, Pokédex, save |
| `whiteout_home` | Lose a real Route 102 trainer fight: wake at home with Mum's heal (also on pre-fix TEST START saves via `HEAL_AFFECTED`) |
| `story_seg2` | Route 102 with the Beacon off (solo trainers), Petalburg, staggered gym entry, Norman/Wally on each save, save |
| `story_seg3` | Twin Beacon on, Route 104 co-op trainer, Petalburg Woods grunt on each save (partner joins from the invite), Rustboro, save |
| `story_seg4` | Reload (Beacon off), Rustboro gym trainers solo, Roxanne co-op, badge and flag on both, save |
| `story_seg5` | Thief, employee, Briney, Rusturf grunt per save, goods, Mr. Stone (PokéNav), Match Call tutorial, co-op Rustboro rival (no replay on the second save), save |
| `story_seg6` | Staggered Pokémon Centre heal, UNLINK, split maps (Mart), relink apart, rejoin, save |
| `catch_walk` | (LONG chain) Walk to Route 104 grass alone, catch with an own Poké Ball, save |
| `coop_levelup_boxes` | Co-op level-ups: each game's copy of the partner's lead matches the partner's level and max HP |

## Partner menu, QoL, quest, and roster runs (build 0xC0170131)

These start from `story_seg6` saves (Rustboro, linked play). `run_all.sh`
runs them with `SEG6=/dir/with/p0.sav,p1.sav`, then the reload scenarios on
the saves they wrote.

| Scenario | Covers |
|---|---|
| `deal_trade` (+`_reload`) | Partner menu TRADE with a held-item trade evolution, then reload: both saved, no copies |
| `deal_cut_before_partner` (+`_reload`) | Power cut before the partner escrowed: the held Pokémon returns on reload |
| `deal_cut_after_commit` (+`_reload`) | Power cut after one side committed: the other completes on reload |
| `deal_lost_ready` | A lost READY recovered live by STATUS |
| `deal_gifts` (+`_reload`) | GIVE ITEM / GIVE POKéMON, and the refusals (last Pokémon, key item, held Mail) |
| `deal_edge` | Busy partner, both asking at once, partner leaving mid-question |
| `deal_battle`, `deal_battle_doubles`, `deal_battle_rules` | Friendly 6v6 singles; Lv. 50 doubles with the team picker; STANDARD refusal, UNRESTRICTED Lv. 100, link-error abort. Parties and money unchanged |
| `deal_dex` (+`deal_dex_offline`) | Partner catches synced, the list marks, and the offline copy |
| `deal_fast` | BATTLE SCENE FAST shared only when both chose it |
| `practice` | The COACH: own team singles, partner's team doubles at Lv. 50; party and money unchanged |
| `save_identity` | Screenshots of CONTINUE and SAVE with the build id (visual check only) |
| `pp_solo` | Power Problems alone, unlinked: log, trip, ROLYCOLY/JOLTIK, VOLT, switch, appliance room, pages, chamber, WATTSON once, POWER SHOP, journal; saves |
| `pp_coop` | Linked: breaker changes and a trip seen live on the other save, split tasks, VOLT + FUSE duo (co-op multi), switch by player 1, catch-up for player 0 at a map change, both paid once, linked journal |
| `pp_reconnect` | Breakers reset while unlinked stay on that save after relinking (0xC017013E policy); a live trip resets both |
| `pp_stagger` (+`_reload`) | Player 0 finished alone earlier (pp_solo's save); after linking and a map change player 1's New Mauville is untouched and WATTSON pays player 1 nothing (0xC017013E policy); player 0 not paid again; survives reload |
| `roster_g` | Unlinked LEVEL CAP toggle; the Lavaridge female SALANDIT evolves into SALAZZLE at 33; a BUIZEL evolves into FLOATZEL at 26 |

New verbs: `fixture_swap A B` (exchange two party slots, keeping each
Pokémon's personality), `fixture_slot SLOT SPECIES [HELD]`, `expect_species
SLOT SPECIES`, `note_party`/`expect_party_same`, `expect_txn STATE`,
`expect_money_same`, `patch_ret SYM FRAMES`, `dex_caught NUM VALUE`,
`expect_partner_dex NUM VALUE`, `menu_to POKEDEX|OPTION`.

Fixtures in these runs: `fixture_warp` between rooms (New Mauville's
blue/green barrier maze is stock and not walked), flags for Wattson's key
step and for an obstacle a player does not need, strong battlers, and
EXP set one point below an evolution level. The quest logic, breakers, link
messages, battles, story ledger, rewards, and saves are the real code.

The partner menu ignores an A press that lands within 20 frames of a scene
or message closing (mashing through text). Scenarios that press A at the
partner right after a menu closed wait 30 frames first.

## Visual checks and polish runs (build 0xC0170133)

Passing a functional scenario does not prove the screens look right, so
`run_all.sh` now runs every scenario with `BATTLE_LOG=1` and then
`visual_check.py` on its log (summary column "visual ok" / "VISUAL FAIL";
details in `<name>.visual.txt`).

| Aid | Meaning |
|---|---|
| `BATTLE_LOG=1` | In battle, print each new battle message on each screen with every battler's species, HP and absent bit: `[fNNN pX battle] text \| b0=species:hp/max ...`. Who acted on what can then be read, not guessed from the picture. |
| `SNAP_EVERY=N SNAP_DIR=dir` | Every N battle frames, dump VRAM, OAM, palettes and I/O to `dir/p<id>_<frame>.bin` and a PNG. |
| `RTC_EPOCH0/1=unix seconds` | FIXTURE: that core's cartridge clock starts at that moment (the RTC reads it as local time; 1699963200 is 23:00, 1699923600 is 12:00 AEDT). The clock always runs with emulated time (60 frames a second); without RTC_EPOCH it starts at 1793268000 (since build 0xC017013E; before that it was the host's clock). |
| `RTC_WALL0/1=1` | That core's clock is the host's wall clock instead, as in an emulator running faster than real time (tests of real-time waits). |
| `expect_tint 1\|0 LABEL` | The outdoor time-of-day tint is (1) or is not (0) applied: BG palettes 2–6 of `gPlttBufferFaded` against `gPlttBufferUnfaded`. |
| `writep PTRSYM+off:size VALUE` | Write through a pointer. |
| `writep_other PTRSYM+off:size SRCPTRSYM+off` | FIXTURE: write through a pointer a value read (byte by byte) through a pointer on the OTHER game, e.g. the partner's trainer id at `gSaveBlock2Ptr+10`. |
| `expectp PTRSYM+off:size VALUE` | Check a value read through a pointer (e.g. a `struct CoopSave` field in SaveBlock3). |
| `expectp_same PTRSYM+off:size` | The value read through the same pointer is the same on both games (e.g. the duo run id). |
| `weak_foes 1\|0` | FIXTURE: on this game (use it on the battle engine's, player 0), the opposing battlers (1 and 3) are held at 1 HP in battle: any hit knocks them out (a flow test's win, not balance). |
| `expect_not_noted` | After `note_text WORDS`: those words were NOT in any message since. |
| `menu_cursor N [FRAMES]` | Press DOWN/UP until the open standard menu's cursor (`menu.sMenu.cursorPos`) is on item N (team screens: 1 CHANGE TEAM, 2 CANCEL), so a press the game did not take is repeated instead of A landing on READY |
| `test_battle_name BATTLER` | In battle, the ROM writes the trainer name a battle message gives that battler (`{B_ATK_TRAINER_NAME}`) into `gStringVar4` (read-only test hook; check with `expect_text gStringVar4 NAME`). `coop_wally_vr_partner` checks both opposing battlers name WALLY on both games (the right one was empty before 0xC017013E). |
| `leak_note [LABEL]` / `leak_check [LABEL]` | Count this game's active tasks, sprites in use, sprite palette tags held and heap blocks/bytes allocated (walking `gHeap`); `leak_check` fails if any differ from the note. Take both on the field at a quiet point. `LEAK_SPRITES=1` also lists each sprite's palette slot and tag. |
| `fixture_nick SLOT NAME` | FIXTURE: a nickname for this player's party slot (letters, digits, `_` for a space, 9 at most). Names only; stats untouched. |
| `until_field_text WORDS [timeout]` | Tap A every 24 frames until a field message containing WORDS (`_` for spaces) is expanded into `gStringVar4`. |

`visual_check.py LOG [SNAPDIR]` checks: `victory-text` (no "You defeated X
and !"), `send-out-match` (each shared battle announces the same Pokémon on
both screens), `no-phantom` (no species-0 battler left present at a turn
start), `no-blank-name` (no message about a nameless Pokémon), 
`prompt-overlap` (no "Partner choosing" inside "What will … do?"),
`learn-prompt` (the move-forget question on one screen only), and with
snaps `bg1-layout` (BG1 never keeps the VS intro's 512x256 layout under the
battle's own windows: the patterned blocks after level-ups). On build
0xC0170132 these fail exactly where the user's recordings showed the bugs
(story_seg3: victory-text, no-blank-name, prompt-overlap; story_seg5:
send-out-match "Nacli / Marshtomp", prompt-overlap, bg1-layout).

| Scenario | Covers |
|---|---|
| `nametag` | seg6 saves: the partner's name tag on both screens, hidden in the start menu, the status window (name, activity, map, "with you"/"inside"), NAME TAG → NEARBY, partner into the Centre (no tag or sprite left outside, one note for in-out-in), UNLINK messages on both sides |
| `lighting_link` | seg6 saves with the clocks set apart (23:00 / 12:00): own tint alone, kept across the CO-OP menu, shared (host's) when linked, indoor untinted, after the Centre, own again after UNLINK, shared again after relinking |
| `story_seg3` | now also: the partner who joined the Woods grunt fight gets the catch-up (own GREAT BALL, own scene state) and the scene does not replay |
| `coop_one_trainer_faints` | seg6 saves: co-op against Bug Catcher Lyle alone; both his Pokémon faint on one turn with one left, which only the left slot sends out; the battle ends normally (FIXTURES: warp, his trainer flag, strong battlers, his fourth Wurmple at 0 HP) |
| `story_seg3_real`, `story_seg4_real` | UNMODIFIED PARTIES: segments 3 and 4 without the stat fixtures, as balance checks. A loss is a finding, not a harness fault |

Scenarios marked `DEBUG-ONLY` in their first line (`pre_rival`,
`pre_woods`) only make saves just before a fight for debugging; they are
not in `run_all.sh`.

## Shared legendaries and the partner mark (build 0xC0170136)

Saves: the main journey's checkpoint 68 (`J68=dir run_all.sh`), warped to
the Sky Pillar top (`legend_top.inc`). Each run plays the real encounter,
battle, catch, link messages, saves and delivery on both cores.

| Scenario | What it checks |
| --- | --- |
| `legend_shared_ray` | Kai starts it, both agree, Kai's ball catches it: one Rayquaza each (own OT), records once, Pokédex, partner seen after |
| `legend_ray_rosa` | Rosa (link guest) starts it and catches it with her ball (used up; Kai's bag untouched) |
| `legend_ray_solo` | Not linked: the stock battle; linking afterwards gives the partner nothing |
| `legend_ray_claimed` | A partner whose record already says claimed is refused by their game (no prompt); Kai faces it alone; Rosa then talks to hers: no battle, it goes |
| `legend_ray_story` | The partner's world hasn't woken Rayquaza: refused; her Sky Pillar state untouched |
| `legend_ray_simul` | Both press A at Rayquaza on the same frame: one battle, Rosa's ball, one each |
| `legend_ray_ko` | Knocked out: nothing granted, it flies off on both screens and comes back; retried and caught |
| `legend_ray_full` (+`_resume`) | Party and PC full: the reward waits (recorded, confirmed); after a reload, still waiting; room made, delivered once; relinking adds nothing |
| `legend_ray_interrupt` (+`legend_ray_reconnect`) | The partner's game drops deal messages: no reward, the catcher keeps the debt and is told it comes later; on the next link it is sent again and delivered |
| `legend_ray_lostack` (+`legend_ray_reconnect2`) | The catcher's game drops the confirmation: the partner has hers, the catcher's debt stays; on the next link the repeat notice is confirmed without a second Rayquaza |
| `legend_groudon`, `legend_regirock` | The same flow for Groudon (Terra Cave: its weather ends on both saves) and Regirock (puzzle flag as a fixture) |
| `legend_migration` | An old one-owner record written into the running game: PARTNER becomes a pending reward delivered once, LOCAL a claim with nothing delivered |
| `coop_bag_battle_tasks` | Kai throws six balls in a co-op battle: at most one co-op session task at any frame (finding #29) |
| `indicator_tour` | The default partner INDICATOR through a town, a map connection, a house door both ways, the Mach Bike and a dark cave (day or night from `RTC_EPOCH0/1`) |
| `nametag` | INDICATOR by default; PARTNER MARK cycles NAME, OFF, INDICATOR; hidden in menus; partner inside a Centre; UNLINK |
| `journey_70_shared_rayquaza` | Main journey, no fixtures: from checkpoint 68, Fly, Mart, sea route, bike climb, the shared catch with the players' own balls, delivery, back down to 5F, save |

New verbs: `expect_legend FIELD VALUE` and `fixture_legend FIELD VALUE`
(FIELD claimed, pending or owed; one bit per COOP_ENC_*), `legend_owned
SPECIES MIN [MAX]` (party and PC, own OT counted; prints OT, met location
and level, IVs), `expect_dex_caught NUM VALUE`, `expect_tasks FUNC N` (live tasks running FUNC; `expect_tasks MAX N`: the most co-op session tasks seen in any frame), `catch N [T] [BALL] [FIGHT]` (pick a ball; fight the first FIGHT turns, or with FIGHT 100+P fight while the foe has more than P% HP; reads the cursor to reach BAG; skips a move with no PP), `fixture_fill_pc` (every
empty PC slot gets a copy of party slot 1) and `fixture_unfill_pc`. A
crashed core now prints its PC, LR and the code addresses on its stack.

Driver limits found here: `smart_moves` makes the link master's move menu
never confirm in the one-foe co-op double (the legend scenarios leave it
off; plain A presses work on both screens, `diag_master_move`), and the
Sky Pillar top's cracked floor is not avoided by `walk` (the scenarios walk
the solid tiles by waypoints).

## Third roster batch (build 0xC0170139)

| Scenario | What it checks |
| --- | --- |
| `roster_b3_catch` (j68) | One catch per encounter method and habitat: grass, cave, Rock Smash, Safari Zone (through the gate, Safari Balls), Super Rod, surfing, ruins, Mt. Pyre; regional forms among them |
| `roster_b3_evo_place` (j65, Kai noon, Rosa 23:00) | Regional branches by place (Dewford = Alola, Route 114 = Galar, Mt. Pyre = Hisui) and the item, time and party evolutions |
| `roster_b3_evo_misc` (j65) | Stats, gender, cloak, friendship, known-move, party-member, personality, item, step, move-use and critical-hit evolutions |
| `roster_b3_services` (j65) | Lilycove clerk's new stock, Oricorio's nectar, the Galar fossil pair, the Sandburied carving (Runerigus) |
| `roster_b3_battle` (j16, co-op doubles) | Schooling, Shields Down, Flower Gift, Dancer, Emergency Exit, Receiver on both screens |
| `roster_b3_play` + `_reload` (seg6) | Catch, trade, PC, UNLINK/LINK UP and power-on with batch-3 species; partner Pokédex marks |

New or extended verbs: `hunt THROWS LIST T fish` (the registered rod) and
`hunt THROWS LIST T smash G N X Y DIR` (warp before a rock, face it, Rock
Smash; the map load brings the rock back); `hunt` throws Safari Balls in the
Safari Zone, fights trainers who spot the player and wild Pokémon that RUN
cannot leave; `hunt` and `catch` press A, never B, in an evolution scene at
a battle's end (B stopped Sirfetch'd's evolution in the first runs);
`test_mon ... ARG7` (low nibble an ability slot + 1; bits 4-5: 1 male, 2
female, 3 a personality divisible by 100; bits 8-12: a nature + 1);
`test_data SLOT hp|tracker VALUE` (current HP; the evolution tracker, e.g.
Rage Fist uses). The battle driver moves the cursor off a move with no PP.
`visual_check.py` skips the Safari Zone's "What will KAI do?" (the player
has no Pokémon in battle there).

## Trainer Exchange and Day Care (build 0xC017013B)

| Scenario | Saves | What it checks |
| --- | --- | --- |
| `exchange_counters` | seg6 | Door sign, guide, directory; HELD-ITEM stock at 1 BADGE (39), a purchase, COMING SOON; the breeding guide |
| `exchange_moves` | seg6 | TM stock (24), TM bought; TEACH A MOVE declined (no charge) and taught (charged once, receipt); RELEARN; EGG MOVE refused below 3 BADGES; COMING SOON pages |
| `exchange_training` | j65 | Stock at 8 BADGES; RESET EVs; HYPER TRAINING one stat; same stat again and all six refused; Lv. 30 refused; HOW IT WORKS; EXP. CANDY XL stops at the cap with a message and is kept when at the cap |
| `exchange_coop` (+`_reload`) | j37 | Both at one counter on the same frame; each player's own stock (fixture: Rosa 2 BADGES) and money; partner icon; partner menu "busy"; side by side when the partner is already there; too poor; bag full; a 3-BADGE item given to the 2-BADGE partner; after power-on |
| `exchange_stock_t0/t2/t3/t5` | j06, j43, j52, j68 | Every counter's stock at 0, 4, 6 BADGES and CHAMPION, and the COMING SOON line |
| `exchange_old_spot` (+`_reload`) | seg6 | A save standing where the building now is (fixture warp) continues in front of its door |
| `breeding_coop` | j65 | Both players at the Day Care; own deposits; own steps; Everstone regional form and nature; Destiny Knot; egg moves from either parent; full party; an egg can't be given; A at a partner whose egg is hatching; Pokédex; withdrawals with identity kept |
| `breeding_rules` (+`_reload`) | j65 | Baby without incense (Azurill); Undiscovered group with Ditto; Power item; hyper training not inherited; a pending egg across power-on; hatch and Pokédex |

New verbs: `expect_mon SLOT FIELD VALUE [ARG] [daycare]` (read-only, through
the ROM's test hook: species, level, nature, mint, ability, evtotal, hyper,
iv, held, egg, personality, move, friendship, gender, ev, otid, perfect,
exp, hasmove, stat, dcsteps; VALUE `-` prints, `LO..HI` a range, `@K`
remembers then compares), `test_data SLOT evs|ivs|hyper|clear|dcsteps V`
(TEST HELPERS: all EVs, all IVs, all hyper-trained bits, empty a party slot,
the Day Care step counter), `note_text WORDS` / `expect_noted` (a message
seen in any frame since), `expect_cb2 FUNC`, `expect_money_change DELTA`
and `fixture_money N`.

## Battle Guide v2 scenarios (build 0xC017013D)

| Scenario | Saves | Checks |
| --- | --- | --- |
| `guide_team_select` | any | `Coop_TeamPartyIdForKey` for each selection rule (tiers, Lilycove/hideout early variants, exhibitions by case, rematch gating) |
| `rival_case_grass/fire/water` | lab | Route 103 co-op rival fight per starter case (FIXTURE: `VAR_STARTER_MON`); Soren (opponent A) leads the case line, Mira (B) the companion line; both talker roles |
| `rival_lilycove_early` / `_late` | j50 / j51 | Lilycove twins: the pre-Winona team (levels 44-46) with five badges, the regular Mega team with six |
| `exh_case_grass/fire/water` | seg6 | Littleroot exhibition aces follow the case |
| `boss_pair_solo` / `boss_pair_coop` | lab | Heath beside Roxanne: talking to him starts the same solo or co-op fight; his own after-words |
| `caps_league` / `caps_champion` | j66 / j68 | Cap 66 before the door guards, 76 after them (League admission), 80 as Champion (Rare Candy refused and kept at the cap) |
| `mega_bracelet` (+`_reload`) | j46 | Steven's Route 118 scene gives each save its own bracelet |
| `mega_exchange` | j57 | Bracelet fallback at the Exchange, MEGA STONE counter stock, a purchase, WHERE TO FIND, a found stone added to the stock |
| `mega_coop_tate` | j57 | Both players Mega Evolve the same turn; enemy Megas (Z-A Chimecho, Gallade); one per trainer; base forms after the battle |
| `mega_cancel` | j57 | The guest turns the trigger on, then changes the choice: no Mega that turn, the allowance kept for the next |
| `mega_abort` | j57 | Link error after both Megas: base forms, identity, moves, EXP, held stones kept |
| `mega_no_bracelet` | j57 | No trigger without your own bracelet in co-op |
| `mega_za_abilities` | j57 | Mega Meganium has MEGA SOL (Solar Beam fires on its first turn), Mega Feraligatr DRAGONIZE |
| `mega_hp_same` | j57 | Mega Banette and Mega Chimecho keep the base form's max HP |
| `space_center_solo` | j58 | Not linked: Maxie and Tabitha with Steven as the ally (stock choose-three) |
| `items_personal` (+`_reload`) | j16 | Item balls are per save; both at one ball; full bag leaves it |
| `quest_stone_catchup` | 13C coast_solo_j68 | A quest finished before this build gives its Mega Stone once; a full bag is told to come back |

New verbs: `expect_team TRAINER KEY PARTY` (read-only: the party id the
ROM picks for a trainer and team key), `expect_mon ... enemyA|enemyB` (the
opponent parties, in battle), `mega_next N` (DRIVER: press START once at
the next N move menus, the game's own Mega trigger).

Build 0xC017013E scenarios:

| Scenario | Saves | Checks |
|---|---|---|
| `release_newgame_boys` / `_girls` | fresh | Release NEW GAME without the dev shortcut: real opening, default names, the van, the clock; different RTC start times per player |
| `catch_before_dex` | lab | Poké Balls sold in Oldale before Route 103; a catch before the Pokédex counts as caught |
| `story_consent` | lab | A gym fight asks the partner; NO starts nothing (said why); YES starts it |
| `story_beacon_off` | lab | Beacon OFF: a gym fight says so and starts nothing; the partner is never asked |
| `story_cancel` | lab | The inviter stops waiting (B); the partner's late YES starts nothing |
| `story_unanswered` | lab | Nobody answers: the question closes on its own (~26 s), the inviter is told (~30 s) |
| `story_beacon_off_wally` | j65 | Beacon OFF: Wally (a trainer's own battle meant for both) says so and starts nothing |
| `story_simultaneous` | lab | Both talk to the leaders at once: player 0's request wins, one fight |
| `invite_expiry_clock` (+`_wall`) | lab | A wild invitation expires after ~30 s of the cartridge clock: ~1,800 frames with the emulated RTC; many more frames with `RTC_WALL0/1=1` at harness speed |
| `battle_visual_wild_double` | lab | Co-op wild double: all four battlers' displayed palettes equal their loaded colours at the first action menu (the left wild Pokémon used to stay dark) |
| `startup_timing_wild` / `_gym` | lab | MEASUREMENT: frames from the request to the first action menu by stage (`gCoopStartTimes`); team-screen time separate |
| `appearance_menu` | lab | CO-OP menu APPEARANCE opens the outfit list, unlinked and linked; the partner sees the change |
| `relink_policy_setup` → `relink_policy` → `relink_policy_reload` | seg6 | Relink ownership policy: progress made unlinked (a crate found alone; FIXTURE: a solo badge and rival stage) stays on its save; joint outcomes recorded with this same partner by trainer id (FIXTURE: a joint badge, rival stage and crate whose message was lost) come back once; a joint badge recorded with another partner does not; checked after linking, after a map change, after a power cycle and a second link (nothing restored twice) |
| `continue_quiet.inc` | — | CONTINUE without tapping A in the field (continue.inc's A mash can talk to whoever the save faces) |
| `duo_tower_first` | j68 | DUO TOWER: Kai at the DUO DESK (LV. 50), Rosa says YES; both default teams have SHROOMISH, both screens say so and nothing starts; Rosa changes her team; battle 1 (FIXTURE weak_foes) against two generated Tower trainers, identical on both games; the same run at 1 win on both saves; real parties byte for byte, money and BP unchanged; NO to battle 2 |
| `duo_tower_set` → `duo_tower_set_reload` | duo_tower_first | FIXTURE run at 6 wins: CONTINUE offers READY/CANCEL and the run's team; battle 7 pays 3 BP once on each save, best streak 7; battle 8 pays nothing; after a power cycle battle 9 is lost (FIXTURE strong_battlers 1): the run ends on both, no white-out, record 8, BP and parties unchanged |
| `duo_tower_abort` | duo_tower_first | KIND recovery: link error during battle 2: nothing won or lost, parties byte for byte, run still at 1; CONTINUE fights battle 2 again (2 wins on both) |
| `duo_tower_behind` | duo_tower_first | One save behind (FIXTURE): Rosa behind - Kai is told her battle and goes back; Kai behind - Rosa's save goes back at the launch; both end at the same battle, no BP change |
| `duo_tower_partners` | duo_tower_first | A run belongs to one partner (FIXTURE: Rosa's run with another trainer id): refused with a message; Rosa retires it at the desk; then "no such DUO run" |
| `duo_desks` | j68 | The six other DUO DESKS say their duo mode is not open; the Tower desk unlinked asks for a partner; the stock singles counter still opens its menu |
| `duo_tower_decline` | duo_tower_first | FIXTURE gCoopDuoLastOutcome = WON (as a won duo battle leaves it); Kai CONTINUEs, Rosa says NO: Kai is told nothing started and is NOT shown "battle won!" (the stale result fixed in 0xC017013E); 1 win on both saves |
| `debug_inputs_off` | lab | SELECT at the action menu of a co-op battle (both roles) and R+START on the field do nothing developer-side: both games stay in the battle and finish it; R+START opens the ordinary START menu (0xC017013F; before, SELECT crashed the game that pressed it) |
| `sustained_session` | j68 | One session at the Battle Tower desk: a cancelled team screen, a new duo run (battles 1-2), a cancelled fixed-team screen, battle 3, START/CO-OP menus, an outfit change and back, FOLLOW round the lobby; after each, tasks, sprites, sprite palettes and heap equal the first note on both games (0xC017013E). FIXTURES: fixture_warp, no_wild; driver aid smart_moves, weak_foes 1, strong_battlers 400 |
| `story_not_ready` | lab | Regression (S2 script fall-through): no Pokemon able to fight gives "Heal one first" and never "The link dropped", for the talker and for the asked partner |
| `opt_escape_dig` | j68 | ESCAPE ROPE (a reusable key item since Gen 8) and DIG out of Granite Cave while linked; the other player unaffected; link up |
| `opt_travel` | j68 | Cable car up and down (Kai) and boarding the S.S. TIDAL in Lilycove (Rosa; this save's route is to Slateport) while linked; link up; the other game unaffected |
| `opt_contest` | j68 | A whole POKeMON CONTEST (Lilycove, A through it) while linked: the hall, the appeals, the results, back in the lobby; link up; the other game unaffected |
| `move_inspector` | lab | L in the move menu: category, PWR ("var." when it depends), ACC ("sure"), who the move hits, then the description (screens) |
| `prep_cancel` | lab | Team screen: READY then B, partner READY: nothing starts; CANCEL tells both; asking again works |
| `prep_timeout` | lab | Nobody finishes the team screen: closes after ~2 min of clock, nothing starts |
| `prep_link_stall` | lab | The partner's heartbeat stops during the team screen: nothing starts until it is back, then one fight |
| `prep_win_j13` (+`_reload`) | j13 | Both choose from slots 3–5 in their own order; ASSISTED win; everyone back in their own slot on both saves; EXP to the chosen; badge; power-on reload |
| `prep_slots456` | j44 | Kai picks 6, 4, 5 in the party screen; those three fight; order restored afterwards (the fight is lost: unassisted) |
| `field_kit` | j13 | The first nurse gives the BOX LINK and HEALING VIAL; the vial heals HP and status, 1 charge, empty message; a deposit at 5 HP comes back at 5 HP; a Center refills; neither works in the Battle Frontier |
| `partner_menu` (updated) | seg6 | FOLLOW now goes with the partner into the Pokémon Centre and back out (2 doors); each save's Repel goes down by its own 8 steps |
| `prep_solo_lead` | j13 | Unlinked gym: lead screen, CANCEL starts nothing, CHANGE LEAD to slot 5, the battle starts with it |

New verbs: `expect_mon SLOT hp VALUE`, `expect_mon ... >@K` (more than note
K), `expect_equal SYM[+off]:size SYM[+off]` (two places on the same game). New harness variables: `RTC_EPOCH<i>` (start time) and `RTC_WALL<i>=1`
(the host's wall clock instead of emulated time).
