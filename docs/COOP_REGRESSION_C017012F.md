# Build C017012F regression results

Date: 2026-10-25. Two linked mGBA 0.10.3 cores running this exact ROM, from
commit `e5d76080`, driven by `tools/coop_link_harness/run_all.sh` with
`AFFECTED` (pre-fix doorway saves) and `ZIG_AFFECTED` (saves made by
C017012C).

ROM SHA256: `49b03afa5982ca9271446c4735c77351f1ffa31367fc3afb225a8fbe100956ce` (32 MiB, BPEE).

All 50 scenarios passed. The host protocol tests pass.

## Data checks (no emulator)

- **Roster:** `tools/coop_roster_audit/audit.py` on this ROM checks 146
  species with 0 issues.
  - Covered: names, stats, types, abilities, graphics, cries, Pokédex
    text, learnsets, evolutions, Hoenn Pokédex numbers, and where each is
    obtained.
  - Also checked: the evolution items, the Rotom forms via the Catalog,
    and the 11 revamps against the docs.
- **Trainers:** `tools/coop_trainers/validate.py` checks 850 Hoenn
  trainers with 0 errors and 0 warnings.
  - Covered: species, items, moves, abilities, and that each moveset is
    legal; party sizes; double battles; gym trainers below their leader;
    roster stages.
  - Twin Beacon split: 692 co-op-capable, 36 solo couples and doubles,
    122 mandatory with a solo path.

## Scenarios

New in this build: `e4_sidney`, `new_species_evo`, `rotom_catalog`, `solo_gym`, `solo_opening`, `solo_rival`, `trainer_overhaul`.

| Scenario | Frames P0 / P1 | Result |
| --- | --- | --- |
| `littleroot` | 4956 / 4939 | Pass |
| `doors_stagger` | 5303 / 5235 | Pass |
| `rescue` | 18183 / 18227 | Pass |
| `guest_move` | 18225 / 18221 | Pass |
| `guest_evolution` | 20263 / 20260 | Pass |
| `target_navigation` | 18465 / 18460 | Pass |
| `spread_selection` | 20315 / 20313 | Pass |
| `mays_house_doorway` | 26427 / 26400 | Pass |
| `mays_house_doorway_p1bag` | 34986 / 34904 | Pass |
| `rival_join` | 9308 / 9239 | Pass |
| `rival_pending` | 9724 / 9776 | Pass |
| `rival_latejoin` | 9758 / 9756 | Pass |
| `rival_simultaneous` | 9703 / 9693 | Pass |
| `rival_reconnect` | 10336 / 10471 | Pass |
| `save_lab` | 19194 / 19192 | Pass |
| `solo_opening` | 14832 / 14796 | Pass |
| `new_species_evo` | 19326 / 19300 | Pass |
| `reconnect` | 3061 / 3043 | Pass |
| `route102_reconnect` | 16801 / 16811 | Pass |
| `solo_six` | 10343 / 10361 | Pass |
| `trainer_reconnect` | 12246 / 12335 | Pass |
| `trainer_p1` | 12181 / 12159 | Pass |
| `coop_six` | 11840 / 11834 | Pass |
| `rival_story` | 11926 / 11927 | Pass |
| `rival_p1` | 12263 / 12302 | Pass |
| `invite_refuse` | 8699 / 8713 | Pass |
| `invite_busy` | 10661 / 10671 | Pass |
| `coop_menu` | 5037 / 4989 | Pass |
| `center_beacon` | 21565 / 21520 | Pass |
| `link_recovery` | 19598 / 19564 | Pass |
| `wild_route101` | 14372 / 14363 | Pass |
| `wild_route101_p1` | 15715 / 15714 | Pass |
| `battle_abort_recovery` | 18143 / 18139 | Pass |
| `invite_timeout_recovery` | 20405 / 20365 | Pass |
| `connected_maps` | 24703 / 24565 | Pass |
| `wild_places` | 13314 / 13336 | Pass |
| `invite_cancel` | 20064 / 20060 | Pass |
| `wild_run` | 10709 / 10505 | Pass |
| `wild_simultaneous` | 11443 / 11413 | Pass |
| `gym_teams` | 78067 / 78102 | Pass |
| `rival_late` | 51966 / 51972 | Pass |
| `qol` | 7864 / 7903 | Pass |
| `solo_rival` | 6610 / 6571 | Pass |
| `solo_gym` | 14665 / 14540 | Pass |
| `rotom_catalog` | 2820 / 2689 | Pass |
| `trainer_overhaul` | 17267 / 17291 | Pass |
| `e4_sidney` | 32447 / 32430 | Pass |
| `bedroom_reload` | 3600 / 3577 | Pass |
| `mays_house_recovery` | 3727 / 3687 | Pass |
| `zigzagoon_reload` | 1620 / 1603 | Pass |

## What kind of test each is

- **Actual play from new saves:**
  - `littleroot`, `doors_stagger`, `rescue`, `guest_move`,
    `guest_evolution`, `target_navigation`, `spread_selection`, both
    doorway roles, the five `rival_*` introductions, and `save_lab`.
  - `solo_opening`: never linked, rescue to lab, no fixtures.
- **Fixtures:**
  - `new_species_evo`: a guest Shinx placed one EXP point below level 15.
- **From lab saves, with the documented Oldale gate fixture:**
  - Unlinked, with strong leads: `solo_rival` (both rival assignments)
    and `solo_gym` (warped to the leader).
  - `rotom_catalog` (warped into the New Mauville entrance).
  - `trainer_overhaul` (Calvin marked beaten for the walk).
  - `e4_sidney` (warped into the room; six strong copies of the lead).
  - `wild_places`, `gym_teams`, and `rival_late`, as before.
- **Saves made by older builds:** `mays_house_recovery`,
  `zigzagoon_reload`.

## Not tested in an emulator

- Solo: Elite Four, champion, legendaries, the Space Center multi battle,
  and the Rustboro, Route 110/119, and Lilycove rival fights. They share
  the code paths of the tested `solo_gym` and `solo_rival`.
- Catching each new species in play. Evolution methods other than Shinx →
  Luxio: friendship and time (Riolu, Snom), stones, and the Rotom Catalog
  menu.
- Ordinary trainers other than Tiana and the unchanged early ones;
  difficulty and balance of any trainer, gym, or boss.
- mGBA 0.10.2, a physical cable pull, internet play, and your own saves.
