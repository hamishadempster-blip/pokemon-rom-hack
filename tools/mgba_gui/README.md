# Graphical two-window link check

Launches the Ubuntu `mgba-qt` build (0.10.2) on this machine's X display and
checks that one process can open two linked multiplayer windows of the co-op ROM.

```sh
python3 tools/mgba_gui/link_check.py
python3 tools/mgba_gui/link_check.py --keep   # leave the windows open
```

The check copies the ROM to disposable filenames under `/tmp/mgba-coop/roms`,
points saves at `/tmp/mgba-coop/saves`, and uses its own config directory.
It does not write a save next to `pokeemerald.gba`.

What it checks:

- both window titles say Player 1 of 2 and Player 2 of 2
- both cores are drawing (fps above 1, screenshot is not a blank frame)
- keyboard focus can be moved to player 2
- after player 1 is paused, Start changes only player 2's frame

mGBA links windows inside one process. Two separate `mgba-qt` processes are not
a link session. Audio sync is off because a failed SDL device stalled the
second core at 0 fps on this VM. The software display driver is selected
because a second OpenGL window did not paint.

Story regression (doors, the TV scene, the Poochyena fight) stays in the
headless harness, `tools/coop_link_harness`. This script only proves the
graphical windows.
