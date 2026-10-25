# Test log: build 0xC0170133

ROM built on branch `coop-doorway-scenes-rivals` from the commit that adds this file (the docs in it do not change the ROM).

| Checksum | Value |
| --- | --- |
| SHA-1 | `e2734dbb3e94c193f5142e49a5194dfd87214a4c` |
| SHA-256 | `0c6c5c9322a7faf658f0930d978e20a2643087b9f8217d52f897ef433da38b85` |

All two-player results come from two mGBA 0.10.3 cores joined by the emulated link cable (`tools/coop_link_harness`). They are not two PCs, not netplay, and not mGBA's GUI: this environment has no display, so normal mGBA GUI playback could not be checked. "Gameplay test" means the scenario drives the real ROM with button presses and reads game memory; fixtures are named in each scenario's header. "Source check" means reading code only. Every recording and screenshot below was made with both players on the same build.

## 1. What the user's recordings showed, and what the logs say now

The checks in `visual_check.py` read the battle text each screen showed (`BATTLE_LOG`) and, with snapshots, the BG registers. Same saves and inputs on both builds:

| Check | 0xC0170132 | 0xC0170133 |
| --- | --- | --- |
| story_seg3 (Woods grunt): victory text | FAIL "You defeated GRUNT and !" (3) | PASS "You defeated TEAM AQUA GRUNT!" |
| story_seg3: nameless send-out (Lv0 box) | FAIL (3) | PASS |
| story_seg3: "Partner choosing" inside "What will … do?" | FAIL (3) | PASS |
| story_seg5 (Rustboro rival): replacement send-out on both screens | FAIL P1 "Nacli", P2 "Marshtomp" | PASS "Nacli" on both |
| story_seg5: BG1 layout under the battle windows (blocks) | FAIL 159 of 1133 snapshots | PASS 0 of 1167 |
| lighting_link (clocks 23:00 / 12:00): tint and time-of-day checks | 11 FAIL | PASS |

Clips (both screens, same build each side): `woods_grunt_before_after.mp4`, `rival_levelup_before_after.mp4`, `lighting_link_before_after.mp4`, `partner_tag_status_0133.mp4`. The "after" halves were recorded on the delivered ROM. Recording provenance: the harness writes the two cores side by side, every frame (`RECORD_ALL=1`), with a frame counter; the clips are cut from those files by frame number and labelled. Fades at doors and battle starts are ordinary fades, not freezes.

## 2. Full regression (`run_all.sh` with `SEG6` and `LONG=1`)

110 runs on this ROM: **109 pass, 1 fails** (`pp_coop`, below). The LONG story chain (story_seg1–6, catch_walk, whiteout_home, bedroom_reload) passes end to end from the lab saves; the seg6-based runs used the existing seg6 saves.

Visual checks: 63 of the runs contain battles. `visual_check.py` was tightened once during this run: its send-out comparison paired lines from two players' simultaneous solo fights and reported `caravan_coop` falsely. After the fix (compare only four-battler send-outs, by nearest line of the same kind), all 110 logs were re-checked: **0 visual failures**; the same checker still fails the 0xC0170132 logs in §1.

| Scenario | Result | Cause |
| --- | --- | --- |
| pp_coop | FAIL (and 2 of 2 reruns) | The VOLT + FUSE duo battle in New Mauville stops after a move: the host waits for the guest's battler to finish, and the guest never completes that move's animation step. **Pre-existing**: build 0xC0170132 stops at the same point with the same saves and inputs. It passed in 0xC0170132's own suite with that run's seg6 saves. Not fixed in this pass; the harness now prints the animation state, healthbox flags, tasks and sprite count at such a stop (`banim`, `bhealthbox` lines) for the next investigation. |
| coop_one_trainer_faints | PASS | New; see §3 |

During the first attempt at this regression, story_seg3 stalled in a co-op battle against Bug Catcher Lyle alone. That was a bug introduced by this pass's single-trainer change (both of his slots asked for his last Pokémon). It was fixed, `coop_one_trainer_faints` was added, and the whole suite was rerun on the fixed ROM (the numbers above).

## 3. Focused gameplay tests added in this pass

| Scenario | Result | What it shows |
| --- | --- | --- |
| nametag | PASS | Tag over the partner on both screens (also with the partner just south), hidden while the start menu is open, status window "BLUE: exploring / RUSTBORO CITY, with you", NAME TAG → NEARBY hides it at four rows away while the partner's side (ON) still shows it, partner into the Centre: no sprite or tag outside, one "went inside" note and none for out-and-in-again, status "RUSTBORO CITY, inside"; UNLINK: "You unlinked…" / "MISTY unlinked…"; no tag after |
| lighting_link | PASS | Own tint alone (night vs day); kept with the CO-OP menu open and closed; linked: both night (host's time); indoors untinted; after the Centre visit outdoor night again; after UNLINK player 1 back to its own midday; relinked: shared again |
| coop_one_trainer_faints | PASS | Lyle alone vs the pair, both of his Pokémon down on turn 1 with one left: one send-out, right slot absent, win. With the fix disabled the same run stalls after the send-out (reproduced, then restored) |
| story_seg3 | PASS | Player 1 joined player 0's Woods grunt fight: catch-up text, GREAT BALL x1 on both saves, scene state 1 on both, the researcher gone on player 1's map, walking the trigger tiles starts nothing |
| story_seg4 (move learning) | PASS | Roxanne co-op: player 0's Emberdash learns Sand Attack; player 0 gets the question, player 1 reads "RED is choosing whether Emberdash learns Sand Attack…" (RED is player 0 in the lab-save chain) |

## 4. Unmodified parties (balance), not in the suite

| Scenario | Result |
| --- | --- |
| story_seg3_real (seg2 saves) | Passes without the stat fixture: the Woods grunt is beaten together (Torchic/Houndour line, one Pokémon each), catch-up as above |
| story_seg4_real (seg3 saves) | Both players lose their solo fight with Youngster Josh (Glimmet's Rock Throw, Toxic Debris) and white out to their homes in Littleroot, still linked; the scenario then times out waiting for Roxanne. Outcome recorded: whiteout → respawn at home → field control back. A balance finding for the one-Pokémon walking chain |

## 5. Investigated, not changed

- "Actions against an empty field after a KO": every case in the logs is the stock doubles rule (both foes down before a slower ally acts; that move fails; replacements at the end of the turn). Example: story_seg4, Lileep fainted, then Sootfang's Smog "But it failed!" with both foe slots absent.
- In the logs, the guest's battler table (`gBattleMons`) trails the host's: the guest fills it from the host's controller messages. The log's state column is therefore read from the host's lines; the text checks use both screens.

## 6. Source checks only

- Status "healing" reads the partner's map event table (nurse graphics id); seen as "talking"/"exploring" in play, the nurse case was not captured in a screenshot.
- `DMSG_CLOCK` is resent only when the minute changes; the hour-change path at a time-of-day boundary was exercised by the starting clocks, not by waiting for a boundary to pass.
- Real hardware, netplay and two physical PCs: not tested.
