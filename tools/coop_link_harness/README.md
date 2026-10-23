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
tools/coop_link_harness/run_scenario.sh tools/coop_link_harness/scenarios/opening.txt
```

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
| `dump [tag]` | Print callbacks, map, position, partner sprite, and co-op state |
| `expect_partner [tag]` | The sprite for the other player is drawn on their real tile (same map only) |
| `expect_no_partner [tag]` | No partner sprite (other map) |
| `hex SYM [len] [off]` | Print raw bytes |
| `shot NAME` | Write `NAME_p0.png` and `NAME_p1.png` |
| `include FILE` | Inline another script |

`coop.<name>` resolves the statics in `src/coop.c`.
