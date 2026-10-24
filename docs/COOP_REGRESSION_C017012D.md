# Build C017012D regression results

Date: 2026-10-24. Two local mGBA 0.10.3 cores. All 20 scenarios passed on
this exact ROM; host protocol tests also passed.

ROM SHA256: `08f050c098999a04beb675142edf850999de21473fb1a67262cd324c7732cb8d` (32 MiB, BPEE).

| Scenario | Frames P0 / P1 | Result |
| --- | --- | --- |
| `zigzagoon_reload` | 1583 / 1598 | Pass |
| `battle_abort_recovery` | 18637 / 18345 | Pass |
| `invite_timeout_recovery` | 22468 / 22452 | Pass |
| `center_beacon` | 21622 / 21760 | Pass |
| `link_recovery` | 18983 / 18945 | Pass |
| `wild_route101` | 14435 / 14443 | Pass |
| `wild_route101_p1` | 16089 / 16060 | Pass |
| `trainer_reconnect` | 12224 / 12240 | Pass |
| `trainer_p1` | 12180 / 12103 | Pass |
| `rival_story` | 11902 / 11882 | Pass |
| `rival_p1` | 12789 / 12784 | Pass |
| `mays_house_doorway` | 25980 / 25944 | Pass |
| `mays_house_doorway_p1bag` | 34692 / 34746 | Pass |
| `target_navigation` | 18548 / 18523 | Pass |
| `guest_move` | 18483 / 18389 | Pass |
| `guest_evolution` | 20230 / 20209 | Pass |
| `solo_six` | 9515 / 9518 | Pass |
| `coop_six` | 11881 / 11858 | Pass |
| `invite_refuse` | 8767 / 8792 | Pass |
| `invite_busy` | 9867 / 9867 | Pass |

## Scope

- Both ordinary trainer and Route 103 rival initiators are exercised. Rival
  tests use the documented strong-stat fixtures, not a balance assessment.
- Wild double victories cover Route 101 and Route 102. Both roles start a
  Route 101 fight. Center transitions/healing and application outages are
  followed by a completed shared battle; explicit unilateral unlink is tested.
- The expired invitation falls back to solo, rejects the late answer, then
  successfully starts a later joint battle. A partner in a menu stays there.
- The abort test injects the engine's link-error flag on both games, checks
  return/recovery, then completes a later double. It does not detect or
  simulate physical cable removal.
- Heartbeat outages patch each emulated sender callback temporarily, while
  serial hardware continues. No internet, physical cable, or emulator restart
  recovery test was performed.
- A genuine C017012C rescue/lab save pair was walked back to Route 101 and
  saved with guest object 4 present, guest hide flag 0, opener hide flag 1.
  Build D removes it immediately on Continue, keeps route state 3, the original
  species/levels/EXP and save counter. The initial recovery attempt exposed
  the missing Continue path; the final run includes that repair.
- Both fresh doorway-role scenarios use legitimate opening gameplay. Lab
  navigation fixtures retain the documented Oldale adventure gate edits.
- No Surf encounter, late-game campaign, real user save, or mGBA 0.10.2 run
  was performed for this batch. Full original ROM/party mechanics are not
  exhaustively covered.

Old-build controls reproduced the Pokemon Center session drop and the
persisted guest-only Zigzagoon. New tests are included in the harness suite;
`zigzagoon_affected_save.txt` is an old-build fixture producer only.
