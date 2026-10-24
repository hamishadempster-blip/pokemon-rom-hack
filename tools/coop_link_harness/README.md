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
| `rival_reconnect.txt` | Player 0 meets the rival while unlinked; after relinking the ledger reaches player 1, who does not replay the introduction. |
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
