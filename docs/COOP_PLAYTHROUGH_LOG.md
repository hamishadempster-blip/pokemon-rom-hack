# Co-op playthrough log (main journey)

A continuous two-player run of the main story on one ROM build, driven by the
harness (`tools/coop_link_harness`), scenario files `journey_NN_*.txt`. Each
segment CONTINUEs the saves the previous segment wrote. No story flags, badge
grants, warps, stat boosts or forced wins in these segments; the only driver
aids are named in each file (for example `smart_moves`, which keeps the move
cursor on the strongest damaging move, and `grind`, which paces in grass and
fights until a battle count or an HP floor).

Players: player 0 **Kai** (boy), player 1 **Rosa** (girl). Checkpoint saves
live outside the repository (`/home/claude/journey/<segment>/p0.sav, p1.sav`
in the work environment); they are not committed.

## Status

| Segment | Scenario | Build / commit | Result | Furthest point |
| --- | --- | --- | --- | --- |
| 01 | journey_01_opening | 2004e0f6 | PASS | New games (real intro, no test start), truck, clock, link, Route 101 rescue, starters (Kai: Torchic, Rosa: Houndour), lab, both saved |

## Findings

| # | Where | What | Kind | Status |
| --- | --- | --- | --- | --- |
| 1 | pp_coop (VOLT+FUSE), any link battle | A special move hitting through Light Screen froze the scoring screen (NULL trap in battle TV) | Game bug | Fixed in 2004e0f6 |

## Next action

See the bottom of the latest segment row.
