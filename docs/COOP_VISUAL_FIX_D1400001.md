# Isolated inspector/field-marker fixes — D1400001

Base: `3ea4b93d` on `coop-doorway-scenes-rivals` (C0170140). Work is isolated on `fixes/move-info-intro-0140`. Do not replace Claude's active tree or reset its branch to this older baseline. No save structures, persistent identifiers, party records or content tables changed.

## Findings and changes

1. **Guest move inspector uses a stale sprite ID.** `gCategoryIconSpriteId` starts at zero and was reset only by `BattleStartClearSetData`; the controller-only guest never runs it. Opening L in its first battle reuses sprite 0 as a category icon, and closing can destroy that unrelated sprite. Before-fix screenshots show corrupt opposing names/health boxes. The new regression detects guest ID 0 versus the required empty sentinel. Reset the ID alongside `ResetSpriteData` on both clients. Validate sprite bounds, in-use state and template before reuse/destruction; handle `CreateSprite` exhaustion without indexing past the array. This removes the demonstrated wrong-sprite path; the user's exact fatal crash was not independently captured as a CPU/assert trace.
2. **Wrong move category after a dynamic move.** The inspector used the engine's last resolved dynamic category for the currently highlighted move; that is also not authoritative on the guest. Use the selected move's data category for this static information panel, consistently including power/status annotations. No battle calculation changed.
3. **Field marker during transitions.** `CB1_Overworld` continues calling `Coop_OverworldFrame` during screen changes; `CoopOutfit_FieldFrame` lacked a callback/battle guard and could recreate a field indicator or reload field palettes on another screen. Hide markers outside `CB2_Overworld` and during battles, before any palette refresh. Patched first-battle screenshots contain no tick. The user's exact persistent tick was not reproduced in the mixed-gender original-code fixture, so this is a lifecycle correction with focused coverage, not a claim of exhaustive reproduction.
4. **Intro pacing: partial improvement only.** Shorten the co-op VS angle animation and link intro presentation delays. Reduce the established co-op connection's fixed send-task grace period from 100 to 8 frames while retaining player-count/block readiness/party exchange. Ordinary PvP timing is unchanged. The overall effect is modest: host request to action menu 1,276 -> 1,254 frames (21.27 -> 20.90 s); party exchange remains 226 frames. Main-loop-to-menu 856 -> 832 frames. One controlled timing sample each, not a performance guarantee. The slow intro remains open; do not describe it as solved or broadly eliminate synchronization to improve it.

## Focused verification

Final-source two-core mGBA 0.10.2 harness scenarios:

- `inspector_first_guest`: fresh opening rescue, guest first, host second, close both panels, finish fight and return linked; PASS. Original-source baseline fails the guest-empty-ID assertion (0 rather than 255).
- `battle_spare_buttons`: ordinary co-op wild battle; action/move spare buttons and inspectors on both roles; complete fight and return linked; PASS.
- `wild_route101_p1`: guest-initiated wild double, win, Beacon/link/partner appearance retained; PASS.
- `startup_timing_wild`: PASS, timing above.
- `startup_timing_gym`: PASS through both action menus (not a gym-victory/balance test).
- `compat_load`: copies of both uploaded real saves loaded and explicitly saved; PASS.
- `compat_reload`: those saved copies power-cycled and relinked; PASS.
- Protocol host tests: 132 checks PASS.
- Field-by-field comparison of the user's save loads against the rebuilt baseline: PASS; party/PC/bag/progression unchanged. Expected map-template/view rebuilds and clock/daily fields excluded by the existing compatibility policy. Round-trip comparison also required before delivery.

The user's files are `emerald-coop-Before Bond Update.sav` and `emerald-coop-Before Bond Update - Copy.sa2` (131,088 bytes each, including appended emulator data). Originals were never modified. Baseline and patched gameplay checkpoints are separate from these files. Both uploaded ROMs SHA-1: `a20130970e1caed23b0d7dbfe48bad0134ab29d5`, matching the documented C0170140 release. The local baseline was rebuilt with GCC 13.2.1/newlib 4.4 and differs byte-for-byte from that delivered release; tests used each local ROM's matching ELF, never uploaded-ROM offsets borrowed from a different build.

Some exploratory driver runs failed: first inspector script needed explicit synchronization; a hand-made post-rescue checkpoint did not finish both dialogue chains or write both battery saves; downstream wild tests from it consequently failed. These outputs were quarantined. Final ordinary-wild tests use the passing existing `save_lab` checkpoint. No failed checkpoint was accepted as verification. The new first-inspector regression is committed; failed exploratory scripts are not.

No full campaign/full regression suite, normal mGBA GUI, hardware or two-PC netplay validation. This is a focused test build, not a replacement claim for Claude's full release verification. No unrelated visual/crash finding is claimed fixed.

## Autosave — deliberately not implemented in this isolated patch

Claude is actively changing save layout/migration. `include/save.h` provides two alternating 14-sector save generations plus four occupied special sectors in a 32-sector flash. These are recovery generations, not independent manual/autosave slots. Calling SAVE_NORMAL periodically would eventually replace the last manual save; it cannot honestly promise an independent manual fallback.

Integrate autosave with Claude's storage preflight, rather than changing save allocation concurrently. Requirements: default OFF; explicit safe-point policy; no saves during battles, scripts, warps/fades, menus mutating data, deals, joint-event commits, unresolved invitations, or rentals without a durable restoration checkpoint. Preserve transaction recovery records; never treat a partially committed operation as idle. Rate-limit and queue a request until safe instead of forcibly interrupting either client. A reconnect timeout must not force a save. Show save success/failure honestly. An independent recoverable manual fallback needs proven storage/emulator backup support and interrupted-write tests before being advertised. Document whether this is same-slot autosave or separate fallback; user must know the difference. Do not relabel the alternating flash generations as a permanent manual backup.

## Integrating after Claude reaches a checkpoint

Inspect the isolated commit and bring the necessary hunks into the current branch. Resolve overlaps against Claude's newer inspector/conditions UI; do not replace its files wholesale. Preserve its layout, bond state and settings. Choose a fresh build ID in its release sequence; the D-prefix here is intentionally isolated. Rerun the first-inspector, ordinary wild, guest-initiated, gym-start and user's-save compatibility cases on the integrated ROM. Leave main unmerged.

## Test ROM

Build ID: `D1400001`. Both players must use this same file; C0170140 or Claude's other builds refuse to link with it. SHA-1: `7557b35c69ed9c4cf990c0ad3148c4b548f52878`; SHA-256: `c496d6fc8bd082f5e2a764546d413fa666dcf0825494d679cf7040badbd70f68`.

For playtesting, close both emulators and back up the old ROM and both actual battery saves first. Associate a COPY of each player's own save with that player's ROM according to mGBA's save naming, including its multiplayer suffix. Continue normally; do not restore a cross-build save state. Do not restore these earlier backups over later gameplay progress. Keep the original old ROM/save pairs for rollback. Do not install this older-baseline test build over a save already migrated by Claude's newer update.
