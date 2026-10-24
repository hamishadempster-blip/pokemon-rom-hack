# Co-op handoff: doorway fix, shared scenes, rival twins, usability

Branch `coop-doorway-scenes-rivals`, on top of `b4edadc0`. Build id
`0xC017012C` (both games must run this build). Player-facing rules are in
[COOP.md](COOP.md); harness scenarios are in
[tools/coop_link_harness/README.md](../tools/coop_link_harness/README.md).

## Root causes found

| Symptom | Cause | Fix |
| --- | --- | --- |
| May stands in the upstairs doorway on one screen only, after the opening fight and the Twin Beacon; unlinking does not help. | The player who opens Birch's bag runs Route 101's script, which sets `FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_RIVAL_BEDROOM` (Brendan's for a girl). The partner reaches the lab through `Coop_EventScript_WarpToLab`, which skipped it. May's 2F transition script repositions her only before the rescue, so on the partner's save she spawned at her template tile `(1,1)`: the stair warp. The state lived in the save, not the link. | One `Route101_EventScript_ApplyRescueState` for both roles. Both rival bedrooms re-hide the rival from the rescue until Lilycove on transition and on resume (a save made in the room), so affected saves recover at load. |
| (Found in the audit) The guest never gets Birch's Pokédex after Route 103. | Guest follow-up 11 set the defeat flag but not `VAR_BIRCH_LAB_STATE = 4`, nor un-hid the lab and Oldale rival, which the talker's `Route103_EventScript_RivalEnd` does. | Follow-ups 11, 12 (Rustboro: met, match call, raised city states) and 15 (Lilycove: rival home again) now mirror the talker, raised only. |
| (Found in the audit) A guest with a full bag keeps a rival standing on Route 110/119 after the shared win. | Follow-ups 13/14 returned early on a full bag, leaving the hide flags unset. | Story state is always written; the item is held as pending in the save and delivered once there is room. |
| (Found while testing) A partner's YES/NO answer could be lost. | `Coop_SendReply` dropped the reply if a block was mid-transfer; the inviter sat out its timeout and the partner could see a stale question later. | Replies queue and retry; an inviter who gives up sends `COOP_KIND_WITHDRAW` so a late YES is "too late". |
| (Found while testing) After joining a wild battle by prompt, the partner's script stayed parked. | Wild battles never resume a script, unlike trainer battles. | The joined wild battle launches after the question script ends. |

## What changed, by stage

**1. Doorway** — `data/maps/Route101`, `data/scripts/coop.inc`, both `LittlerootTown_*House_2F` scripts.

**2. Scene lifecycle** — `src/coop.c` (story ledger, scene join, partner tile, reply queue), `include/constants/coop.h` (stable ids), `src/overworld.c` (apply at transition), `include/global.h` (`CoopSave` gains `layout`, `storyDone`, `storyApplied`, `rivalId`, `sharedItems`; old saves are upgraded in place on first access).

**3. Rival twins** — `src/coop_rivals.c` (the second twin), renamed rival dialogue (May → MIRA, Brendan → SOREN) across the rival scenes and match call, `{RIVAL}` expansion, Route 103 counter-pick lines, guest follow-ups.

**4. Usability** — start menu status window (`src/start_menu.c`), CO-OP list, nearby invitations, battle prompt and spread warning (`src/battle_controller_player.c`, `src/battle_main.c`), catch messages, shared items.

Defaults chosen: the twins are Birch's children; the bedroom twin follows the stock gender rule; a player's rival follows the case (starter holder → Soren, companion holder → Mira); "nearby" means the same map, on any tile; the join answer window is 480 frames inside a 600-frame invite.

## Rival encounter audit

| Encounter | Both twins present | Pair fight | Guest state | Exercised by harness |
| --- | --- | --- | --- | --- |
| Bedroom introduction (2F Poké Ball) | Home twin only, by design; the other is named | — | Ledger + join | Yes: join, late join, pending, simultaneous, reconnect |
| Downstairs introduction (1F trigger) | Home twin only | — | Ledger (read from the save's own state) | No |
| Route 103 | Yes | Soren + Mira | Fixed (lab state 4, lab/Oldale rival) | Yes (`rival_story.txt`, from lab fixture saves) |
| Birch's lab, Pokédex scene | Yes (twin follows) | — | Unchanged | No |
| Oldale Town | Yes | — | Unchanged | No |
| Route 104 / Rustboro | Yes | Soren + Mira | Fixed (met, registered) | No |
| Lavaridge (Go-Goggles) | Yes | — | Each save plays its own scene | No |
| Route 110 | Yes | Soren + Mira | Dowsing Machine as shared item | No |
| Route 119 | Yes | Soren + Mira | HM Fly as shared item | No |
| Lilycove | Yes | Soren + Mira | Fixed (rival home again) | No |
| Champion's room, Littleroot after the League | Yes | — | Unchanged | No |

The second twin is generic: it appears next to any visible object using a
rival graphic outside the two Littleroot houses. Its own dialogue is the
Route 103 introduction and its talk line; later scenes still speak through
the scene's rival.

## Known gaps and remaining work

- Later rival scenes (lab, Rustboro, Route 110/119, Lilycove) were not run in the harness; they need late-game fixtures or a long play-through.
- Only the link master can show "Partner ready."; the guest keeps the stock prompt.
- With a boy and a girl, Littleroot's houses differ between screens (stock gender rule).
- The partner's open YES/NO is not closed automatically when the inviter gives up; answering then says it is too late.
- Prize money and the guest's battle economy were not changed or audited.
- Internet (RetroArch/gpSP) play was not run.
