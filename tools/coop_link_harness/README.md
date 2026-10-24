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
