# Build C017012E regression results

Date: 2026-10-25. Two linked mGBA 0.10.3 cores running this exact ROM,
driven by `tools/coop_link_harness`. Host protocol tests
(`tools/coop_protocol_test.c`) also pass.

ROM SHA256: `26ef6c4ae4eb3fff5602169c0283c0264657284525bf90e53e5d18ee9d3f8343` (32 MiB, BPEE).

## `run_all.sh` (41 scenarios, all passed)

`AFFECTED=/tmp/affected` adds `mays_house_recovery`, using saves the pre-fix
build produced at the moment of the doorway bug.

| Scenario | Frames P0 / P1 | Result |
| --- | --- | --- |
| `littleroot` | 4941 / 4940 | Pass |
| `doors_stagger` | 5464 / 5236 | Pass |
| `rescue` | 18247 / 18413 | Pass |
| `guest_move` | 18351 / 18276 | Pass |
| `guest_evolution` | 20169 / 20218 | Pass |
| `target_navigation` | 18443 / 18411 | Pass |
| `spread_selection` | 20311 / 20286 | Pass |
| `mays_house_doorway` | 26678 / 26670 | Pass |
| `mays_house_doorway_p1bag` | 35347 / 35325 | Pass |
| `rival_join` | 9867 / 9313 | Pass |
| `rival_pending` | 9806 / 9724 | Pass |
| `rival_latejoin` | 9855 / 9831 | Pass |
| `rival_simultaneous` | 9686 / 9741 | Pass |
| `rival_reconnect` | 10406 / 10291 | Pass |
| `save_lab` | 19244 / 19184 | Pass |
| `reconnect` | 3011 / 3032 | Pass |
| `route102_reconnect` | 16797 / 16706 | Pass |
| `solo_six` | 10265 / 10253 | Pass |
| `trainer_reconnect` | 12242 / 12173 | Pass |
| `trainer_p1` | 12146 / 12088 | Pass |
| `coop_six` | 11985 / 12007 | Pass |
| `rival_story` | 12830 / 12826 | Pass |
| `rival_p1` | 12971 / 12857 | Pass |
| `invite_refuse` | 8688 / 8690 | Pass |
| `invite_busy` | 9760 / 9785 | Pass |
| `coop_menu` | 4970 / 4963 | Pass |
| `center_beacon` | 21203 / 21095 | Pass |
| `link_recovery` | 20130 / 20129 | Pass |
| `wild_route101` | 14252 / 14279 | Pass |
| `wild_route101_p1` | 14349 / 14355 | Pass |
| `battle_abort_recovery` | 18496 / 18446 | Pass |
| `invite_timeout_recovery` | 21186 / 21146 | Pass |
| `connected_maps` | 24928 / 24765 | Pass |
| `wild_places` | 13379 / 13439 | Pass |
| `invite_cancel` | 20016 / 20009 | Pass |
| `wild_run` | 11424 / 11424 | Pass |
| `gym_teams` | 79514 / 79516 | Pass |
| `rival_late` | 46235 / 46213 | Pass |
| `qol` | 8023 / 8021 | Pass |
| `bedroom_reload` | 3650 / 3528 | Pass |
| `mays_house_recovery` | 3836 / 3752 | Pass |

## Run separately on the same ROM (2 scenarios, both passed)

| Scenario | Frames P0 / P1 | Result |
| --- | --- | --- |
| `wild_simultaneous` (new; now in `run_all.sh`) | 11539 / 11536 | Pass |
| `zigzagoon_reload` | 1571 / 1582 | Pass |

For `zigzagoon_reload`, the affected saves were made fresh by rebuilding
C017012C from `66719f6d`. That rebuild is byte-identical to the delivered
C017012C ROM (sha1 `76ff54b5…`). On it, `save_lab` was run, and then
`zigzagoon_affected_save.txt` reproduced the guest-only Zigzagoon.

## What kind of test each is

- **Gameplay from a fresh save** (no fixtures): `littleroot`,
  `doors_stagger`, `rescue`, `guest_move`, `guest_evolution`,
  `target_navigation`, `spread_selection`, both doorway roles, the five
  `rival_*` introduction scenarios, and `save_lab`.
- **Lab saves with documented fixtures**:
  - `save_lab` and everything after it uses the Oldale gate fixture.
  - Teleports with `fixture_warp`: `wild_places` (Granite Cave, Route 103
    Surf) and `gym_teams` (Roxanne, Norman, Juan). `rival_late` (Route 110,
    Lilycove) also teleports and marks earlier rival stages as won.
  - Strong-stat leads isolate the flow being tested from difficulty:
    `wild_places`, `gym_teams`, `rival_late`, `wild_simultaneous`, and the
    rival fights.
  - `wild_simultaneous` patches `WildEncounterCheck` so that two encounters
    land on the same frame.
  - `mute_coop` and `battle_abort_recovery` simulate stalls and errors. They
    are not physical cable pulls.
- **Saves made by older builds**: `mays_house_recovery`, `zigzagoon_reload`.
- **Source and ROM-data audit, no gameplay**: the 107-species roster audit
  and the Gym teams for badges 2, 3, 4, 6, and 7.

## Coverage of this batch's requests

- **Connected maps**: `connected_maps` covers both roles and both
  directions:
  - East/west at offset 0 (Oldale and Route 102).
  - North/south (Oldale and Route 101).
  - The offset −10 connection (Route 102 and Petalburg).
  - Crossing first, stopping, turning, and repeated crossings.
  - A house just inside the boundary (hidden, with a note).
  - A solo wild fight with the partner visible across the boundary, which
    sends no invitation.
  - A co-op double after all of that.
- **Battle lifecycle**:
  - Both initiators: `wild_route101`/`_p1`, `trainer_reconnect`/`trainer_p1`.
  - Simultaneous encounters: `wild_simultaneous`.
  - Declined, expired, cancelled, and busy invitations: `invite_refuse`,
    `invite_timeout_recovery`, `invite_cancel`, `invite_busy`.
  - Stalls and errors: `link_recovery`, `battle_abort_recovery`.
  - Center transitions: `center_beacon`.
  - Targeting, spread moves, and party indicators: `target_navigation`,
    `spread_selection`.
  - RUN: `wild_run`.
  - Captures to party and PC, ball ownership, no copies: `wild_places`.
  - Guest move learning and evolution: `guest_move`, `guest_evolution`.
  - Party limits: `solo_six`, `coop_six`.
  - Prize parity: `trainer_*`, `gym_teams`.
- **Wild doubles in several places**: Route 101, Route 102, Route 103 Surf,
  and Granite Cave 1F.
- **Rivals**:
  - Bedroom introduction (five variants).
  - Route 103 in both roles.
  - Route 110 alone, then together.
  - Lilycove.
  - The guest's Dowsing Machine.

## Not tested

- Rival scenes at Birch's lab (Pokédex), Rustboro, Route 119, Lavaridge,
  the champion's room, and post-League Littleroot. Only their source was
  checked.
- Gyms 2, 3, 4, 6, and 7 fights, rematches, and difficulty or balance for any
  gym.
- Catching or evolving any added species. Only the ROM data and how each
  species is obtained were checked.
- Repel renewal and Bag memory. These are stock features and weren't changed.
- West/east connections other than Oldale–Route 102 and Route 102–Petalburg.
  Dive maps are excluded by design, which a host test checks.
- mGBA 0.10.2 cores, a physical cable pull, internet or RetroArch play,
  restarting an emulator, and your own saves.
