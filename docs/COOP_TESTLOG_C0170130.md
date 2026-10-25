# Bug-fixing pass: test log, build 0xC0170130

Branch `coop-doorway-scenes-rivals` (unmerged). Two mGBA 0.10.3 cores linked
in one process (`tools/coop_link_harness`), each with its own save, both
screens checked and recorded side by side. Every row below is an executed
gameplay run unless it says otherwise. "Fixture" marks test setup that is not
gameplay (listed per scenario in its header comment).

Builds named below: **F** = 0xC017012F as handed over; **G** = F + respawn
fix (same build id); **I** = 0xC0170130, the delivered ROM (G + co-op
level-up display fix + "boosted" label fix).

## Bugs found, fixed, rerun

| # | Bug (how found) | Cause | Fix | Rerun evidence |
|---|---|---|---|---|
| 1 | Losing a battle left the player at the top-left corner of Petalburg (map 0.0, tile 0,0) instead of home. Found in `story_seg2` on F: player 1 lost a Route 102 trainer fight. | The co-op TEST START skips the truck, whose script is the only thing that sets the respawn point; `lastHealLocation` stayed all zero (= Petalburg 0,0). Confirmed by dumping SaveBlock1 +0x1C on F saves: `00 00 00 00 00 00 00 00`. | `Coop_ApplyClockStart` sets the gendered bedroom respawn. `Coop_RepairUnsetHealLocation` repairs an all-zero heal location on Continue and before any whiteout or Teleport, so existing saves recover. | `whiteout_home` PASS on pre-fix saves (player 1 wakes in May's house 1F, Mum's heal, HP > 0, link kept). Fresh TEST START shows `01 03 ff 00 04 00 02 00` (May 2F) / `01 01 ...` (Brendan 2F) at +0x1C. Two real wipe-outs in the Rustboro gym during development sent player 0 to Brendan's house and player 1 to May's. |
| 2 | Co-op battle health boxes disagreed between screens (seen in the `story_seg4` recording at Roxanne: host showed Lv12/Lv14, guest Lv11/Lv10). | Both leads are party slot 0 of different parties. `Task_LaunchLvlUpAnim`/`Task_UpdateLvlInHealthbox` matched the slot index alone, so a player's level-up flash and new level went to the ally's box. The guest's link-partner controller ignored exp updates (`BtlController_Empty`), so the host's level-ups never reached the guest, and the host never redrew the guest's box. Data on G: guest's copy of the host lead stayed Lv10 while the host reached Lv12. | `IsMonOutOnAllyOfSameParty` also requires the same party. `LinkPartnerHandleExpUpdate` (guest only) mirrors the host's exp step into its copy and redraws that box. The host redraws the guest's box after the partner-exp path changes level. | `coop_levelup_boxes` PASS on H and I: after real level-ups (host 5→10, guest 5→8) each game's copy of the partner lead equals the partner's level and max HP, both ways. Both screens show the same boxes (`levelup-boxes` shots). |
| 3 | "gained a **boosted** … Exp." printed for the partner's Pokémon. | The amount already skipped the traded bonus for the partner; the label did not. | Same partner test for the label. | OCR of every battle message in the `coop_levelup_boxes` recording (both screens, H): only "gained N Exp. Points!", no "boosted"; the `story_seg4` recording on G showed "gained a boosted 290". |

Build id bumped to 0xC0170130 for fix 2 (it changes link-battle behaviour).
Save layout unchanged; F/G saves load (the story chain below runs on saves
made by earlier builds).

## Not game bugs (test-driver or scenario issues found and fixed)

- The driver always chose move 1. A status move looped "There's no PP left"
  (Route 102). Added `smart_moves`, now reading the move menu's own data so
  it also works on the guest, whose `gBattleMons` is empty by design.
- Party stat fixtures were wiped by level-up recalculation (both players lost
  in the Rustboro gym). Replaced by `strong_battlers` (in-battle only).
- Arrow-warp exits (Petalburg Woods north, Rusturf west) need the direction
  held: `cross`, not `until_map`.
- The Match Call tutorial menu only accepts POKéNAV; scripted with
  `until_task`.
- Personality-only copy checks misfired when both cores booted in the same
  second (see the final suite below); they now compare personality and OT.
- One harness segfault in a single recorded `catch_walk` run (empty log);
  not reproduced in four later runs (with and without recording, and under
  gdb).
- A partner inside the 7-tile strip of a connected map is drawn by design;
  "apart" checks moved further away. The path-finder can jump a one-way
  ledge into a pocket on Route 104 north; the split uses the Mart instead.
- An idle partner never answers an invitation, so the fight goes solo
  ("Solo: your partner didn't answer") and the prompt is withdrawn. That is
  the designed recovery, observed in a Woods run on G, not a hang.

## Long progression runs (real walking, no warps or story flags)

Each segment continues the saves the previous one made. Runs on G unless
marked; the whole chain is rerun on I in the final suite below.

| Scenario | Result (G) | What happened on both saves |
|---|---|---|
| `story_seg1` | PASS | Lab saves → Mom's Running Shoes → Routes 101/103 → co-op twins won → Oldale "MISTY! Over here!" scene → staggered lab entry → Pokédex (flag 0x74) → save |
| `story_seg2` | PASS | Footprint man lets them west; Route 102 with the Beacon OFF: each player's spotted trainer is a solo win; Petalburg; staggered gym entry; Norman/Wally tutorial played on each save (0x4085 = 2); save |
| `story_seg3` | PASS | Twin Beacon ON; Route 104 trainer spots the pair → co-op multi battle; Petalburg Woods: player 0's Devon scene → grunt invite → player 1 joins → win (0x4098: p0 1, p1 still 0); then player 1's scene → player 0 joins → win (both 1); north exit; Rustboro; save |
| `story_seg4` | PASS | Reload → Beacon OFF (per-session by design); gym trainers solo; Roxanne co-op double won; flags 0x4F0 and badge 0x867 on both; Torchic → Combusken after the win; save |
| `story_seg5` | PASS | Thief (0x405A 1→2), employee (→3), Briney (0x406F 2), Rusturf grunt solo per save (0x409A 3, Devon Parts), goods returned (→5, warp to Devon 3F), Mr. Stone letter and PokéNav (→6), Match Call tutorial (→7), Rustboro rival as co-op (→8 on both; player 1 crossing the trigger afterwards gets no second fight); save |
| `catch_walk` (I) | PASS | Continue from segment 6; player 0 walks to Route 104 grass alone (Beacon off), throws one of its own Poké Balls (5→4) and catches; party 2; player 1 unaffected; link up; save |
| `story_seg6` | PASS | Leads at 1 HP; player 0 heals in the Centre while player 1 waits outside (partner hidden), then the reverse; UNLINK; player 1 into the Mart while player 0 walks; relink from different maps (no partner shown); rejoin (partner shown); save |

Roles alternated: player 0 opened the Route 103 fight, the Woods scene and
Roxanne; player 1 opened its own Woods scene and accepted invitations;
each player's own story state was checked separately. One starter pairing
(Torchic / Houndour) and one-Pokémon parties throughout the chain.

## Final suite on the delivered ROM (build I)

`run_all.sh` with `LONG=1`, `AFFECTED`, `ZIG_AFFECTED`, `HEAL_AFFECTED`, ROM
SHA-1 `3d09a38ecf8be2116712e901b8b010233ccbb082`: 60 runs, 58 PASS, 2 FAIL.

The two failures were test-check artifacts, not game bugs. In that run the
two starters rolled the same personality value (0xE333F5A9) for different
Pokémon (Torchic vs Houndour, different OT IDs). `BUGFIX` seeds the RNG from
the RTC seconds at boot, and the harness boots both cores in the same
second. `save_lab`'s roster check and `wild_places`' no-copies check (which
continued from those saves) compared personality alone. A real copy keeps
personality and OT ID, so both checks now compare both. Rerun with the final
harness: `save_lab` PASS, `wild_places` PASS on the same coincident saves
("none of 7 Pokemon copied"), and every other scenario using those checks
PASS (`rescue`, `guest_move`, `guest_evolution`, `target_navigation`,
`spread_selection`, `mays_house_doorway`, `mays_house_doorway_p1bag`,
`new_species_evo`). No co-op code uses personality.

<details><summary>Suite summary (build I)</summary>

```
littleroot                   PASS  frames p0=4934 p1=5219 failures=0  (6s)
doors_stagger                PASS  frames p0=5241 p1=5385 failures=0  (6s)
rescue                       PASS  frames p0=18478 p1=18261 failures=0  (37s)
guest_move                   PASS  frames p0=18211 p1=18382 failures=0  (39s)
guest_evolution              PASS  frames p0=20116 p1=20429 failures=0  (35s)
target_navigation            PASS  frames p0=17848 p1=18134 failures=0  (32s)
spread_selection             PASS  frames p0=21088 p1=21249 failures=0  (39s)
mays_house_doorway           PASS  frames p0=25826 p1=25826 failures=0  (48s)
mays_house_doorway_p1bag     PASS  frames p0=35016 p1=35222 failures=0  (59s)
rival_join                   PASS  frames p0=9888 p1=9318 failures=0  (17s)
rival_pending                PASS  frames p0=9653 p1=9828 failures=0  (18s)
rival_latejoin               PASS  frames p0=9849 p1=9831 failures=0  (18s)
rival_simultaneous           PASS  frames p0=9771 p1=9964 failures=0  (17s)
rival_reconnect              PASS  frames p0=10528 p1=10259 failures=0  (16s)
save_lab                     FAIL  frames p0=19198 p1=19171 failures=2  (34s)
solo_opening                 PASS  frames p0=15014 p1=14483 failures=0  (25s)
new_species_evo              PASS  frames p0=19340 p1=19503 failures=0  (33s)
reconnect                    PASS  frames p0=3102 p1=3267 failures=0  (7s)
route102_reconnect           PASS  frames p0=14521 p1=14522 failures=0  (24s)
solo_six                     PASS  frames p0=9127 p1=9127 failures=0  (16s)
trainer_reconnect            PASS  frames p0=12100 p1=12142 failures=0  (21s)
trainer_p1                   PASS  frames p0=12039 p1=12065 failures=0  (20s)
coop_six                     PASS  frames p0=11913 p1=11893 failures=0  (22s)
rival_story                  PASS  frames p0=13213 p1=13205 failures=0  (23s)
rival_p1                     PASS  frames p0=11884 p1=11927 failures=0  (21s)
invite_refuse                PASS  frames p0=9504 p1=9558 failures=0  (17s)
invite_busy                  PASS  frames p0=9556 p1=9551 failures=0  (15s)
coop_menu                    PASS  frames p0=4965 p1=5023 failures=0  (7s)
center_beacon                PASS  frames p0=22548 p1=22591 failures=0  (37s)
link_recovery                PASS  frames p0=22498 p1=22536 failures=0  (38s)
wild_route101                PASS  frames p0=17339 p1=17392 failures=0  (37s)
wild_route101_p1             PASS  frames p0=14407 p1=14428 failures=0  (27s)
battle_abort_recovery        PASS  frames p0=17672 p1=17586 failures=0  (33s)
invite_timeout_recovery      PASS  frames p0=19354 p1=19243 failures=0  (44s)
connected_maps               PASS  frames p0=21768 p1=21766 failures=0  (51s)
wild_places                  FAIL  frames p0=13599 p1=13544 failures=2  (36s)
invite_cancel                PASS  frames p0=17299 p1=17296 failures=0  (32s)
wild_run                     PASS  frames p0=10775 p1=10784 failures=0  (21s)
wild_simultaneous            PASS  frames p0=11012 p1=10933 failures=0  (22s)
gym_teams                    PASS  frames p0=78170 p1=78272 failures=0  (151s)
rival_late                   PASS  frames p0=48169 p1=47880 failures=0  (114s)
qol                          PASS  frames p0=7913 p1=8017 failures=0  (14s)
solo_rival                   PASS  frames p0=7136 p1=6957 failures=0  (6s)
solo_gym                     PASS  frames p0=15815 p1=15484 failures=0  (12s)
rotom_catalog                PASS  frames p0=3162 p1=2677 failures=0  (3s)
trainer_overhaul             PASS  frames p0=18052 p1=18094 failures=0  (39s)
e4_sidney                    PASS  frames p0=41204 p1=41049 failures=0  (93s)
coop_levelup_boxes           PASS  frames p0=27543 p1=27570 failures=0  (61s)
story_seg1                   PASS  frames p0=25097 p1=25110 failures=0  (55s)
whiteout_home                PASS  frames p0=9637 p1=8557 failures=0  (17s)
story_seg2                   PASS  frames p0=37121 p1=37145 failures=0  (81s)
story_seg3                   PASS  frames p0=26787 p1=26758 failures=0  (53s)
story_seg4                   PASS  frames p0=37044 p1=37061 failures=0  (89s)
story_seg5                   PASS  frames p0=50895 p1=50338 failures=0  (131s)
story_seg6                   PASS  frames p0=7596 p1=6846 failures=0  (15s)
catch_walk                   PASS  frames p0=9519 p1=9281 failures=0  (21s)
bedroom_reload               PASS  frames p0=3706 p1=3875 failures=0  (4s)
mays_house_recovery          PASS  frames p0=4206 p1=3691 failures=0  (5s)
whiteout_home                PASS  frames p0=9317 p1=8775 failures=0  (19s)
zigzagoon_reload             PASS  frames p0=1703 p1=1587 failures=0  (1s)
failed scenarios: 2
```
</details>

Recordings made on build I, both screens side by side (2x speed unless
noted): whiteout at home (1x), Route 104 + both Woods grunt fights, Roxanne,
Rustboro rival, Centre/Unlink/relink, co-op level-up boxes, catch.

## Not executed

- Story after the Rustboro rival (Dewford onward) was not played in a walking
  run. Later gyms, the Elite Four and bosses are covered only by the older
  fixture-warp scenarios (`gym_teams`: Roxanne, Norman, Juan; `e4_sidney`).
- Physical cable loss: only the application-level stall (`mute_coop`) and an
  injected battle abort are exercised.
- Different starters in the long chain; party sizes above one in the long
  chain (six-Pokémon parties only in `coop_six`/`solo_six`).
- Item and trade evolutions in a walking run (only `new_species_evo`,
  `guest_evolution` with fixtures).
- No run on real hardware or the mGBA GUI; all on the two-core harness.
