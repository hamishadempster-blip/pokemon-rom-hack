# Test log, build 0xC0170135

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). ROM built from source commit `d9502d9e` (no `src/` or `include/` change since).
- ROM `emerald-coop-C0170135.gba` (32 MB): SHA-1 `6fc520fe8fceffe1fa2c973fcf38a1a2df4c3da8`, SHA-256 `5ce145ad780ffbfe93a5d6dbef2030b1443e4e83d50ee15e08119e0690bf1b25`.
- Changes since 0xC0170133: fix #24 (0xC0170134: a partner on a bike or surfing never gets on-foot run frames; the Mach Bike case reset the other game) and fix #25 (0xC0170135: the partner's save records the Victory Road Wally fight). Saves load unchanged.

## Source checks (no gameplay)

| Check | Result |
| --- | --- |
| ROM build (`make`) | Clean |
| `tools/coop_protocol_test.c` on the host compiler | All passed |

## Gameplay tests (two linked mGBA cores, the harness)

### Main journey (continuous saves, no fixtures)

Journeys 62 to 69 in `docs/COOP_PLAYTHROUGH_LOG.md`: Sky Pillar, Sootopolis, Juan (badge 8), Ever Grande, Victory Road, the Elite Four, the Champions, the Hall of Fame, the credits and two power-on reloads after the clear: all passed on both saves. Journeys 66 on ran on this build; 62-65 on 0xC0170133/0xC0170134 (same save format).

### Full regression suite on the final ROM

`run_all.sh` with `LONG=1`, `SEG6` (story_seg6 saves) and `J65` (journey checkpoint 65): 118 scenarios.

| Result | Count |
| --- | --- |
| PASS | 117 |
| FAIL | 1: `deal_battle_doubles` |
| Visual-check failures | 0 |

`deal_battle_doubles` failed on a driver limitation, not the game (finding #26): both of the guest's battlers fainted, and the driver picked the same replacement twice. The harness was fixed and the ROM left unchanged. It was then rerun together with `deal_battle`, `deal_battle_rules`, `coop_one_trainer_faints` and `battle_stray_block`: all PASS.

`pp_coop`, the known failing scenario on 0xC0170133, passed this time. No change targeted it, so it may only be timing; it is not claimed fixed.

### New regressions this build

| Scenario | Before | After |
| --- | --- | --- |
| `coop_partner_mach_bike` (SEG6) | 0xC0170133: Rosa's game resets | PASS on 0xC0170134 and 0xC0170135 |
| `coop_wally_vr_partner` (J65) | 0xC0170134: Rosa's var stays 0; Wally fights the pair again | PASS on 0xC0170135 |
| `coop_field_move_fly` (SEG6, from #23) | 0xC0170133: Fly refused | PASS |

## Recordings (two screens side by side; not committed)

- `partner_mach_bike.mp4`: fix #24, both players on the Mach Bike.
- `wally_victory_road.mp4`: fix #25, Wally once, Rosa crosses the line.
- `champion_hof_credits.mp4`: from Drake's room: Steven + Wallace, the Champion scenes, the Hall of Fame, the credits and the reset.
- `post_clear_reload.mp4`: power-on after the clear, both relink in Littleroot.

## Open

- Finding #16 ("Opponent needs a valid name") was not reproduced; its cause is unknown.
- Driver notes:
  - `until_cb2` can miss a callback that lasts one frame (the boot callback after the credits).
  - A scenario keeps running after a failed check, so a later `save_again` can save a failed state. This happened in Elite Four attempt 1; the checkpoint in use was not affected.
