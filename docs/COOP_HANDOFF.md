# Bug-fixing pass: build 0xC0170130

Branch `coop-doorway-scenes-rivals`, on top of the 0xC017012F work. Keep it
unmerged. Both games need this build (0xC017012F refuses to link with it).
The save layout is unchanged; existing saves load, and saves with the
respawn problem below are repaired on load. Full log with evidence:
[COOP_TESTLOG_C0170130.md](COOP_TESTLOG_C0170130.md).

## Fixed

- **Whiteout sent the player to Petalburg's corner.** The co-op TEST START
  skipped the truck script, the only place that sets the respawn point, so
  `lastHealLocation` stayed all zero (Petalburg, tile 0,0). TEST START now
  sets the player's bedroom; `Coop_RepairUnsetHealLocation` (overworld.c)
  fixes an all-zero heal location on Continue and before a whiteout or
  Teleport, so affected saves wake at home with Mum's heal.
- **Co-op level-ups on the wrong health box.** Both leads are slot 0 of
  different parties; the level-up flash and new level went to the ally's
  box, and the guest never heard about the host's level-ups.
  `IsMonOutOnAllyOfSameParty` (battle_controller_player.c) also checks the
  party; `LinkPartnerHandleExpUpdate` (battle_controller_link_partner.c,
  guest only) mirrors the host's exp step into its copy; the host redraws
  the guest's box after the partner-exp path (battle_script_commands.c).
- **"gained a boosted … Exp." on the partner's Pokémon** (label only; the
  amount was already right).

## Test harness additions

Story runs now walk real maps (`walk`, `walk_battle`, `cross`, `until_map`),
with `no_wild`, `strong_battlers` (in-battle stats only), `smart_moves`,
`until_task`, `expect_mirror`, `hexp`, and `edges.py`. See the harness
README. New scenarios: `story_seg1`–`story_seg6` (a chain from the lab to
Rustboro after the rival, each continuing the last one's saves),
`whiteout_home`, `coop_levelup_boxes`, `catch_walk`. Final suite on this
build: 60 runs, 58 pass, 2 check artifacts fixed and rerun (see the log).
`run_all.sh` runs the
long chain with `LONG=1`; `HEAL_AFFECTED=dir` adds `whiteout_home` on
pre-fix saves.

## Behaviour confirmed, not changed

- The Twin Beacon is per session: off again after a reload.
- An idle partner never answers an invitation; the fight goes solo after
  the timeout and the prompt is withdrawn.
- A partner standing in the 7-tile strip of a connected map is drawn (and
  counts as visible) even when off screen.
- The Match Call tutorial after Devon accepts only POKéNAV (stock).

## Known gaps

- Walking story coverage stops after the Rustboro rival; later story is
  covered only by the older fixture scenarios.
- One starter pairing and one-Pokémon parties in the walking chain.
- Physical cable loss is not exercised (application stall and injected
  battle abort only).
- One harness segfault was seen in a single recording run of `catch_walk`;
  it did not reproduce (same scenario passed under gdb and with recording).
- Two games booted in the same second get the same RTC RNG seed and can
  roll matching personality/IVs for different Pokémon. Harmless (no co-op
  code uses personality) and unlikely outside the harness; the harness copy
  checks now also compare OT ID.

# Roster, revamps, trainers, bosses, solo paths: build 0xC017012F

Branch `coop-doorway-scenes-rivals`, on top of `03e29fd8` (C017012E). Keep it
unmerged. Both games need this build; 0xC017012E refuses to link with it.
The save layout is unchanged, so existing saves load.

The reference tables for this build (new Pokémon, revamps, trainer and boss
changes, co-op and solo coverage) are in
[COOP_ROSTER_F.md](COOP_ROSTER_F.md). Exact test results are in
[COOP_REGRESSION_C017012F.md](COOP_REGRESSION_C017012F.md).

## What changed

- **No fight needs a second player** (`3119da59`). A game that is not
  linked plays the whole story alone:
  - Birch's rescue: one starter from the case you highlight, against one
    POOCHYENA.
  - Gyms, Elite Four, and champion: a double battle against both trainers.
  - Rivals: a single battle against your own twin's solo team.
  - Space Center: the stock multi battle with Steven.
  - Legendaries: stock single battles; a solo catch settles only that
    cave's legend.

  While linked, these fights still wait for the partner, and the message
  now names UNLINK as the way to face them alone. A solo win counts only on
  that save. Linking does not copy it, because `jointBadges` holds co-op
  wins only.
- **Fourteen new families** (`5103ae26`):
  - Wild slots via `place_new_families.py`, plus Hoenn Pokédex entries.
  - The New Mauville scientist gives each save a Rotom Catalog.
  - Own Tempo Rockruff becomes Dusk Lycanroc at level 25 at any hour.
  - The roster audit now covers forms and reports excluded ones.
- **Eleven revamps** (`029b55fe`) via `revamps.py`, checked against the ROM
  by the audit.
- **Ordinary trainers** (`1d742b0c`): 728 rebuilt by
  `tools/coop_trainers/overhaul.py` from a fixed base commit (rerunnable),
  and validated by `validate.py`.
- **Bosses** (`20c296c3`), via `bosses.py`:
  - Solo rival teams in the stock BRENDAN and MAY slots.
  - Villains and Wally rebuilt.
  - One added Pokémon per Elite Four and champion side.
  - Smarter AI for bosses, without omniscience.
- **Harness**:
  - `expect_range`.
  - `mash_field` now handles a forced switch in battle (steps to a healthy
    slot, backs out of summaries).
  - New scenarios: `solo_opening`, `solo_rival`, `solo_gym`,
    `new_species_evo`, `rotom_catalog`, `trainer_overhaul`, `e4_sidney`.

## Decisions taken

- **Linked games keep waiting for the partner in mandatory fights.** The
  way to fight alone is UNLINK. This keeps one player from clearing a gym
  or rival scene while the partner is on the way. A game that is not linked
  never waits.
- **The solo rival fight is your own twin, in a single battle.** Soren for
  the case-starter holder, Mira for the companion holder, so the
  counter-pick stays meaningful.
- **Ordinary trainers' levels rise only modestly** (+0 to +5). Most of the
  extra difficulty comes from team composition, partners, items, and AI,
  as asked. Wild levels are unchanged.
- **Couples and twins stay solo-only**, as before. A one-Pokémon trainer
  stays one Pokémon as a co-op opponent; nothing is duplicated to pad a
  double battle.

## Known gaps

- Not exercised in an emulator: the solo legendaries, the solo Space
  Center multi battle, solo Elite Four and champion fights (they use the
  same special as `solo_gym`), and the solo Rustboro, Route 110/119, and
  Lilycove rival fights. Their source paths are shared with tested ones.
- The added Pokémon have not been caught in play, except through random
  wild doubles. Only Shinx → Luxio was evolved in play.
- Ordinary trainer teams were validated by the tool. Only Tiana was fought
  in play, plus the stage-0 trainers already in older tests. Difficulty
  was not play-tested.
- Unchanged from earlier builds: the partner's open YES/NO is not closed
  automatically when the inviter gives up; a physical cable pull and
  internet play are untested.

---

# Seamless overworld, battles, Beacon, rivals, roster: build 0xC017012E

Branch `coop-doorway-scenes-rivals`, on top of `672331ea` (C017012D). Keep it
unmerged. Both games need this build. The save layout is unchanged, so
existing saves load.

## What changed

- **Connected maps** (`544d5a95`):
  - The partner is drawn through north/south/east/west connections, including
    offset ones, while their real tile falls in the strip of the neighbouring
    map this screen has loaded. Movement is a normal step across the boundary,
    with no snap.
  - One sprite, no collision, elevation taken from the tile.
  - Doors, caves, warps, and Fly hide the partner and put a short note in the
    map-name window ("NAME went inside", "NAME: PETALBURG CITY").
  - Interiors and unrelated maps never draw them.
  - Visibility does not imply eligibility: battles still need the same map.
  - The field loop now drains the co-op receive backlog each frame. Before,
    it read one command per frame, which could leave the partner up to 50
    frames behind.
- **Battles** (`544d5a95`, `bcad5418`, `8504de10`):
  - A committed wild join launches even if the inviter goes quiet, and a
    withdrawn invite cancels it.
  - A guest's catch was encrypted with the master's OT id and decrypted as
    garbage. It is now re-keyed to the guest's id with its data intact.
  - Shared trainer wins paid no prize to either save. Each save now pays
    itself the stock prize once.
  - RUN needs both players on the same turn.
- **Beacon clarity** (`bcad5418`):
  - The start-menu status says whether fights here would be together, and
    why not.
  - The CO-OP menu explains Linked, Beacon on, and Beacon off in one line and
    names the switch by what it does.
  - A solo fight while linked with the Beacon on says why under its first
    message: other map, busy, said no, no answer, team not ready, recovering,
    facility, or one-player encounter.
  - A surfing partner rides a surf blob.
- **Rivals** (`38211b66`):
  - Soren always leads with the starter line and Mira with the companion
    line, and the added Pokémon fill their teams.
  - The twin's talk line follows the rival stage.
  - The twin never takes the partner's tile, so both screens place it the
    same.
  - A rival scene started alone explains why, returns the rival to its tile
    and the map music, and plays in full later.
- **Roster and gyms** (`675d528e`): see
  [COOP_GYM_AND_ROSTER_AUDIT.md](COOP_GYM_AND_ROSTER_AUDIT.md) for the 107
  audited species, how each is obtained, the evolution fixes (Gimmighoul,
  Bisharp, Charcadet armors, Linking Cord), and every gym's final team.
- **Small conveniences** (`b6dea32d`):
  - The party screen reopens on the last highlighted Pokémon.
  - Returning to the nurse who last healed you this session heals at once.
  - Bag position memory and the expired-Repel renewal were already stock
    features and are unchanged.
- **Simultaneous encounters**: when both players roll a wild encounter on
  the same frame, each answers the other "busy", and both fight solo with
  "Solo: your partner is busy." Nothing is left waiting, and the next co-op
  double works. This was already the behaviour; this batch adds a test for
  it.

## Decisions taken

- Gyms remain co-op only, as the project already specified. There is no
  separate solo gym team; each side's three Pokémon form a self-contained
  team.
- Visible across a boundary means "on another map" for battles, invitations,
  and scenes.
- Gimmighoul now evolves at level 50 and Bisharp at level 64, because the
  stock items they need don't exist in this game.

## Verification

Exact results are in [COOP_REGRESSION_C017012E.md](COOP_REGRESSION_C017012E.md).

---

# Previous: regression fixes, build 0xC017012D

Branch: `coop-doorway-scenes-rivals`, based on `66719f6d`. Keep unmerged.
Both players need the new build; existing save layout is unchanged.

## Causes and changes

- **Guest-only rescue Zigzagoon:** `removeobject` set Zigzagoon's own hide
  flag on the opener's save. The shared rescue-state script only hid Birch,
  so the waiting player's flag `0x2EE` stayed clear. The shared script now
  writes both flags. Completed rescues also repair the flag and remove the
  active Zigzagoon on map entry/resume. The repair requires both
  `FLAG_RESCUED_BIRCH` and route state >= 3, because the former is set before
  starter selection. Birch's later field actor is untouched.
- **Affected saves loaded in place:** Continue restores active object events
  and bypasses transition scripts. `ResumeMap` now applies pending story
  before restored actors respawn. This is exercised on real old-build saves
  with the guest's Zigzagoon visibly present before saving.
- **Beacon wild doubles outside Route 102:** `Coop_WildPairReady` explicitly
  rejected every other map. Ordinary land encounters now use the same
  invitation flow across eligible maps; ordinary Surf encounters are wired
  into it too. Safari/Frontier/Trainer Hill/Pyramid stay excluded. Fishing,
  Rock Smash, roamers, outbreaks, and special scripted encounters retain
  their existing paths. Late answers cannot launch from an unavailable or
  different-map partner; a deferred wild ACK leaves the cable before the
  guest yields the block buffer to the battle.
- **Pokemon Center unlinks:** `CableClub_OnResume` calls `InitUnionRoom`,
  which switched to RFU, called `OpenLink`, and replaced the cable session.
  `ShouldCheckForUnionRoom` now refuses this background scanner while co-op
  owns the transport. This also protects the nurse's scanner call.
- **Quiet timeout erased session intent:** The old 240-frame timeout called
  `Coop_Drop`, clearing Beacon. It now tracks accepted application heartbeats,
  hides an unavailable partner, and attempts transport recovery without
  clearing the session/Beacon. Valid frames restore availability. The start
  menu reports unavailable/recovering; its CO-OP menu remains accessible so
  the user can explicitly unlink during an outage.
- **Deliberate unlink:** a new repeated position-frame request tells the peer
  this is intentional. Compatibility uses build D. A unilateral unlink ends
  both reachable sessions; an unreachable peer cannot receive notification.
  Explicit unlink and incompatible builds may still terminate a session.

## Verification and limits

The final results table is in `COOP_REGRESSION_C017012D.md`. Tests use two
mGBA 0.10.3 cores and actual game/controller execution.

The old build reproduces both the Center disconnect and the guest-only
Zigzagoon. The recovery save is generated by the old build, not by writing
its story flags. Lab-save navigation scenarios use the already documented
Oldale adventure-gate fixture. Both doorway-role scenarios use legitimate
opening gameplay. Rival scenarios retain their documented strong-stat
fixture to isolate story handoff from difficulty.

`mute_coop` temporarily patches the emulated sender callback to return, then
restores its original instruction. It simulates missing application
heartbeats while the serial cable runs; it is NOT a physical cable pull or
internet test. `battle_abort_recovery` injects the engine's error flag and
checks rollback/return/retry, not physical-loss detection. A closed emulator
cannot communicate. Resuming an existing transport recovers automatically;
restarting an emulator may require linking on that game again.

Not verified: actual cable removal, internet/netplay or emulator restart
reconnection, Surf encounters, late-game rival/story sequences, or every
map/facility. mGBA 0.10.2 was not rerun for this batch. No claim is made that
all battles or every bug is fixed.

## Delivery

ROM: `emerald-coop-C017012D.gba`, game code BPEE, 32 MiB.
SHA256: `08f050c098999a04beb675142edf850999de21473fb1a67262cd324c7732cb8d`.
Update both ROM copies, retaining each existing ROM filename so its own
`.sav` still matches. Back up saves first. ROMs and personal saves are not
committed.

---

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

## Test results

Host protocol tests pass. `tools/coop_link_harness/run_all.sh` on two linked
mGBA 0.10.2 cores: 27 scenarios pass (see the harness README for the one
Route 102 rerun after making its pacing deterministic). A 12-scenario set,
covering the doorway in both roles, reload and recovery, the rival join,
Route 103, the Route 102 wild double, a refused invitation, the CO-OP menu,
and the Calvin co-op fight, passes on mGBA 0.10.3 cores. The doorway scenario
fails on the pre-fix build exactly as reported.

Fixtures: the rival introduction and doorway checks use legitimate play
only. Route 103, the invitations, and the menu start from `save_lab.txt`
saves with the documented Oldale gate fixture. Recovery used saves that the
pre-fix build produced at the moment of the bug, not a save from a player's
own game.

## Known gaps and remaining work

- Later rival scenes (lab, Rustboro, Route 110/119, Lilycove) were not run in the harness; they need late-game fixtures or a long play-through.
- Only the link master can show "Partner ready."; the guest keeps the stock prompt.
- With a boy and a girl, Littleroot's houses differ between screens (stock gender rule).
- The partner's open YES/NO is not closed automatically when the inviter gives up; answering then says it is too late.
- Prize money and the guest's battle economy were not changed or audited.
- Internet (RetroArch/gpSP) play was not run.
