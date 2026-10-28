# Test log, build 0xC017013A (co-op polish)

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). ROM source commit `a714c7f3`; the harness changes and the docs come after it (test tools and text only; the ROM is unchanged by them).
- ROM `emerald-coop-C017013A.gba` (32 MB): SHA-1 `cb64e3cf311cc0cc4efae804da55b41e828137e8`, SHA-256 `0231ef9b2c1804e37e8550b1d91910a7d0a52cb9d2e8e36e329c47588c8dc154`.
- Commits since 0xC0170139: activity icons, edge arrow, PING, VIEW TEAM, FOLLOW `f9d07377`; shared conversations, battle targets and choice changes, evolution hints `4805ee12`; build id `88a6aa23`; harness and scenarios `d327521f`; popup width `cda8c960`; busy-note reasons `6d871d34`; scene-announcement retry and B grace `ef024478`; harness `5d0460bd`; busy reason only when needed `a714c7f3`; harness budgets `2d1f1d2b`.
- All gameplay is the two-core harness on libmgba 0.10.3 (`tools/coop_link_harness`), p0 = Kai (link master), p1 = Rosa. Fixtures and test helpers are named in each scenario's header and logged as `FIXTURE` / `TEST HELPER`. Not tried: the mGBA GUI, real hardware, netplay.

## Source checks (no gameplay)

| Check | Result |
| --- | --- |
| ROM build | A clean build of `a714c7f3` in a fresh worktree (no build directory) reproduces the tested ROM (same SHA-1) |
| `tools/coop_protocol_test.c` (host compiler, `-iquote include`) | 132 checks, all passed |
| Link frame layout | Six activity bits replace the three 2-bit legendary owners (unused since layout 3); frames from 0xC0170139 are refused by the build id, as every build change is |
| Save compatibility | No save structure, flag or variable added; everything new (follow trail, conversation state, team snapshot, turn revisions) lives in RAM for the session only. Every checkpoint save the suite loads was written by an older build |
| Evolution item sources | `src/data/coop_evo_item_hints.h` generated from the map scripts by `tools/coop_roster_audit/evo_item_hints.py` |

## Focused gameplay (new and changed scenarios)

| Scenario | Saves | What it does | Result |
| --- | --- | --- | --- |
| `partner_activity` | seg6, Rustboro, linked | Kai reads Rosa's icon: walk, menu (START), talk (partner menu), "?" while Kai is asked, back to walk; Rosa walks east off screen: arrow at the right edge, the dot again in view; heartbeat muted: broken-link icon, cleared on recovery; a solo wild battle on Route 116: swords | PASS (fixtures: no random wild battles, the mute) |
| `partner_menu` | seg6 | VIEW TEAM: Rosa's party in the summary screen, read only, nothing on her screen; the evolution hint on her Pokémon ("EVOLVES INTO Houndoom At Lv. 24 … (Your partner's POKéMON.)"); with her deal messages dropped, the last team with its age. PING: sound and note, second one refused for the cooldown. FOLLOW: east, north, west along her trail; no loop; stops outside the Centre door; manual stop; stops before Route 101 grass | PASS (fixtures: dropped deal messages for 400 frames, warps) |
| `partner_appearance` | j68 | Picture self-check stays at 0 faults: both through May's door on the same frames, walking through the partner, Mach Bike, Surf on the Petalburg pond | PASS (fixtures: warps, a Mach Bike) |
| `talk_join` | j68, Lilycove and Dewford, linked | 1: Kai (fast text) talks to the POKéFAN, Rosa (slow text) joins: greeting, Kai waits at the yes/no until Rosa's READY, tick over Kai on her screen, "Kai answered YES.", the turn-back movement is a second checkpoint. 2: roles swapped, Rosa goes ahead with B, Kai gets "went on without waiting". 3: the joiner leaves with B, the host continues alone. 4: Rosa's heartbeat muted mid-wait: Kai released, Rosa's conversation ends with the link note. 5: Dewford Hall man (special code): Rosa's A gives "Kai is talking", no box; a second A within the cooldown shows nothing. 6: Petalburg Mart clerk: "Kai is shopping" | PASS (fixtures: warps, text speeds, the mute) |
| `battle_cancel` | j16, Route 116, co-op wild double | Target prompts: ALL OTHER POKéMON with "! This hits your ALLY too" (Earthquake), FOE (LEFT) Lotad, ALLY Zigzagoon with "! This attacks your ALLY" (Tackle), BOTH FOES (Tail Whip). Rosa (guest) takes back Tail Whip: "Changing your choice…", the menu reopens only after the master's answer, Growl is used. Kai (master) changes his own choice. Commit boundary: Rosa's B on the frame of Kai's last A: both games agree (cancel counts and turn ids); then both RUN | PASS (helpers: leads by `test_mon`, no-damage moves; fixtures: all-Lotad grass, a forced encounter) |
| `evo_hint` | j65, not linked, clock at night | Charmander (Lv 16, "6 levels to go"), Pikachu (Raichu not near Dewford / Alolan Raichu near Dewford, "None in BAG. Sold: Mauville Mart"), Eevee (1/10), Chansey (friendship), Happiny (held Oval Stone not at night; with the stone held: "Not now: it's night."), Charizard (none); the SELECT cue on the memo | PASS (helpers: the Pokémon, a held item) |
| `coop_menu` | save_lab | Reads the partner's activity at its new frame offset | PASS after the scenario fix |
| Five field scenarios (`indicator_tour`, `outfit_bases`, `mays_house_doorway`, `doors_stagger`, `nametag`) | | Also assert 0 appearance faults | see the suite |

Results across runs, by outcome: the commit boundary is decided by timing on two cores. Over this build's runs all three outcomes were seen, each consistent on both games: the change accepted (master's cancel count 3, Rosa's menu reopened, Kai's move kept), refused as late (both games count a late request, the first choice used), and B arriving after the commit (ignored; the turn runs).

### Found and fixed while testing

- A scene announcement (the partner joining the rival's introduction) was dropped when the link was busy with another block at that moment; the new conversation note made it happen in `rival_join` and `mays_house_doorway` (both failed in the first suite run). It is now kept until sent.
- Long co-op popups lost their last letter under the frame; they fit 68 px now.
- B right after choosing a move could be read as a change request; 20 frames of grace.
- The summary memo's SELECT cue overflowed the window.
- The busy-note reason was sent for every unshareable conversation; each deal message waits for an answer, so in `quest_busy_partner` the Desert Caravan's third crate reached the partner only after the check that follows their battle (first full suite run). The reason is now sent only for shops, gifts, battles and warps with the partner on the same map.

## Full regression on the final ROM

`run_all.sh` with `LONG=1` (story chain), `SEG6`, `J16`, `J37`, `J43`, `J48`, `J52`, `J65`, `J68` on ROM `cb64e3cf…` (harness at `2d1f1d2b`): **194 scenarios, 189 PASS, 5 FAIL**. Visual-check failures: 3, the same three as on 0xC0170139 (`finding16_wild_teleport_karen`, `finding16_partner_wild_karen`, `coop_battle_items`: a battler drawn with no name for a frame or two at a send-out, a known harness finding).

The five failures, and what was done (all reruns on the same ROM):

| Scenario | What failed | Cause | After |
| --- | --- | --- | --- |
| `link_recovery`, `invite_timeout_recovery` | The co-op wild battle was lost | The A-only driver: one lead repeats Leer; the encounter drawn decides whether the other lead wins alone | Scenarios now use the driver's `smart_moves` (`54f3b1c8`); both PASS on rerun; `center_beacon` (same pattern, lost in the first suite run) also PASS |
| `battleforms_coop` | Morpeko's picture differed by 205 px between the screens | Rosa's Morpeko was bobbing at her action menu when compared | Compared after she has chosen (`05158b26`); PASS, 0 px, both screens show the Hangry form |
| `late_wild_tour` | An Abra fainted instead of teleporting | Luck (this scenario also failed on 0xC0170139 and passed in this build's first suite run) | PASS on rerun |
| `roster_b3_catch` | The Safari Zone Furfrou hunt ran out of its budget | Luck (rare slot; 8 of 9 species caught) | Not resolved. Rerun (500000 frames): 8 of 9 again, this time the surfing Galarian Stunfisk hunt: the driver paces only up and down and was left in an east-west water channel with no steps for 260000 frames. On the previous ROM (`265ec238`, same game code apart from the busy-note change) with 600000 frames all 9 were caught. The scenario is not linked, so none of this build's co-op code runs in it; its hunting driver is the weak point |

An earlier full run (ROM `265ec238…`, before `a714c7f3`) had 194 scenarios with 4 failures: `quest_busy_partner` (a real regression, fixed in `a714c7f3` and passing above), `roster_b3_catch` and `roster_b1_play` (hunts that ran past the 200000-frame budget; budgets raised in `2d1f1d2b`; `roster_b1_play` passes above) and `roster_b1_reload` (continues `roster_b1_play`'s saves); `late_wild_tour` passed there; one extra visual-check note (`story_seg3`, a nameless battler for one frame) did not recur. The very first run, on `6d871d34`, was stopped after `rival_join`, `mays_house_doorway`, `coop_menu` and `center_beacon` failed (the scene-announcement bug, fixed in `ef024478`, and the scenario offset).

## Two-screen clips (from this ROM, harness recordings)

- `clip1_activity_icons_C017013A.mp4`: menu, talk, invitation, edge arrow, broken link, battle icons.
- `clip2_shared_conversation_C017013A.mp4`: a joined conversation at two reading speeds, the shared question, the corner notes.
- `clip3_battle_targets_cancel_C017013A.mp4`: target prompts, the guest's and the master's changes, the commit boundary, both running.
