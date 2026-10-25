# Test log: build 0xC0170131

ROM built from `7bef09dd` on branch `coop-doorway-scenes-rivals`.

| Checksum | Value |
| --- | --- |
| SHA-1 | `5bce1e5fe6603ba6b1e9b04a71de69247c1b1175` |
| SHA-256 | `6101d18262c502ec1673fe8bee5e0f5a5db0c71bf5b31e9b0169f327b2a76574` |

All two-player results below come from two mGBA 0.10.3 cores joined by the emulated link cable (`tools/coop_link_harness`). They are not two PCs, and they are not netplay.

## 1. Data validation (tools over the built ROM and sources)

| Check | Result |
| --- | --- |
| `tools/coop_roster_audit/audit.py`: every added or revamped species, its habitats, its evolution items or methods and where they come from, its graphics, Pokédex text, and learnsets | 210 species, 0 issues |
| `tools/coop_trainers/validate.py` | 852 trainers, 0 errors, 0 warnings |
| `tools/coop_text_width.py` on the new and changed scripts (New Mauville, Mauville, field guide, partner menu, CO-OP) | No line wider than the message box. One older line in `coop.inc` (the legend-conflict message) is still too wide, and it is unchanged. |
| `tools/coop_protocol_test.c` (host) | All passed |
| Build | `make` clean; the only warnings are the stock RWX segment ones |

## 2. Source inspection only (not exercised in play)

- **Partner menu edge cases:**
  - an egg offered for trade or gift (refused in `Deal_CheckMonCanLeave`);
  - a full PC refusing a received Pokémon;
  - a full bag pocket refusing a gift;
  - an HM chosen as a gift (`CoopDeal_ItemGiftable`).
- **Lost save:** a save that loses its partner's save for good keeps its prepared record. The start menu note explains it; there is no way to cancel it by hand.
- **Power Problems race:** both players' live breaker messages cross on the same frame. Each takes the other's value, and the next change brings them back together.
- **The PORYGON line in play:** Up-Grade and Dubious Disc use from the bag (`I_USE_EVO_HELD_ITEMS_FROM_BAG`, `EVO_ITEM` entries checked).
- **POWER SHOP tiers:** only the base list was opened in play; the 6, 7 and 8 badge lists were read from the script.

## 3. Gameplay tests

`OUT=… SEG6=<story_seg6 saves> tools/coop_link_harness/run_all.sh` ran 76 scenarios in one pass. 74 passed. The other two failed once, then passed twice each when rerun on their own (details below the table). The run used fresh saves, lab saves made in the same run, and the story_seg6 saves. Those were made by the 0xC0170130 build, so every SEG6 scenario also loads a save from before this update (old co-op save layout).

| Area | Scenarios | Result |
| --- | --- | --- |
| Opening, rescue, bedroom, rival scenes (fresh saves) | littleroot, doors_stagger, rescue, guest_move, guest_evolution, target_navigation, spread_selection, mays_house_doorway (+p1bag), rival_join, rival_pending, rival_latejoin, rival_simultaneous, rival_reconnect, save_lab, solo_opening, new_species_evo | 17/17 pass |
| Lab-save regressions | reconnect, route102_reconnect, solo_six, trainer_reconnect, trainer_p1, coop_six, rival_story, rival_p1, invite_refuse, invite_busy, coop_menu, center_beacon, link_recovery, wild_route101 (+p1), battle_abort_recovery, connected_maps, wild_places, invite_cancel, wild_run, gym_teams, rival_late, qol, solo_rival, solo_gym, rotom_catalog, trainer_overhaul, e4_sidney, coop_levelup_boxes | 29/29 pass |
| Intermittent | invite_timeout_recovery, wild_simultaneous | Failed once in the suite; 2/2 passes each on rerun |
| Walking story chain | story_seg1, whiteout_home, story_seg2, bedroom_reload | 4/4 pass |
| Partner menu | deal_trade (+reload), deal_cut_before_partner (+reload), deal_cut_after_commit (+reload), deal_lost_ready, deal_gifts (+reload), deal_edge, deal_battle, deal_battle_doubles, deal_battle_rules | 13/13 pass |
| QoL | deal_dex, deal_dex_offline, deal_fast, practice, save_identity (screenshots: "Build C0170131" on CONTINUE and SAVE) | 5/5 pass |
| Power Problems | pp_solo, pp_coop, pp_reconnect, pp_stagger, pp_stagger_reload | 5/5 pass |
| Roster in play | roster_g (female SALANDIT gift evolves at 33; BUIZEL evolves at 26; unlinked LEVEL CAP toggle) | pass |

The two intermittent failures, in detail:
- **invite_timeout_recovery:** the second co-op wild battle was lost (level-5 leads, no strength fixture, so the outcome varies with the RNG seed).
- **wild_simultaneous:** expects "Solo: your partner is busy." when both games roll an encounter on the same frame. Once it got "didn't answer", which depends on which game's question lands first.

Both have the same shape as in earlier builds and do not touch code changed here.

What the Power Problems runs show:
- **pp_solo** (one player, not linked):
  - The log sets the order. CONTROL out of order trips all three.
  - ROLYCOLY and JOLTIK leave their panels after a battle.
  - VOLT fights solo, steps off the switch, and the switch still refuses with no breakers reset.
  - Once all three are reset, the switch stops the generator: crates gone, VOLT and FUSE gone.
  - ROTOM and PORYGON stay after being knocked out.
  - The three pages open the chamber only with the generator off, and the UP-GRADE is picked up.
  - WATTSON pays once (TM24 ×1 after a second talk). The POWER SHOP list opens.
  - The journal has four pages (screenshots), and the game is saved.
- **pp_coop** (linked):
  - A reset by player 0 shows on player 1's save within 4 seconds, with the "breakers 1/3" note. A wrong press by player 1 resets both.
  - The rest of the order is split between the players.
  - VOLT offers the duo, player 1 joins, and both saves record VOLT and FUSE. VOLT steps aside on both games.
  - Player 1 throws the switch. Player 0 keeps the running generator until a map change, then gets the catch-up state (`FLAG_PP_CATCHUP`, shop open).
  - Both are paid once, and the linked journal is shown.
- **pp_reconnect:** a reset made while unlinked survives relinking on both saves (the higher count is kept). A live trip resets both.
- **pp_stagger** (player 0 finished alone in pp_solo; player 1 never started):
  - Player 0's chamber is walkable after the reload.
  - After linking, player 1 gets "power restored" at its next map change, and WATTSON pays player 1 exactly once over two talks.
  - Player 0 is not paid again.
  - The player 1 save is kept.
- **pp_stagger_reload:** player 1's catch-up state and reward survive a reload. Player 1's own ROTOM is in the appliance room, and VOLT and the crates are gone.

## 4. Bugs found and fixed during testing

| Bug | How it showed | Fix |
| --- | --- | --- |
| A damaged deal block passed its checksum with a wild sequence number (seen as 0x213). Every later message then looked old and was ignored. | After a gift, the next GIVE POKéMON stalled: the partner never saw the request. 5 of 16 runs of `deal_gifts` with a diagnostic log showed it. | Accept only the partner's next sequence number, only from the partner's OT ID, and ignore acks beyond what was sent (`CoopDeal_HandleBlock`). 10/10 runs pass after the fix, and the whole suite passes. |
| Mashing A through a cutscene next to the partner opened the partner menu, and a few more presses sent a battle request. | The fresh-save doorway and rival scenarios fail in the TV scene | The partner menu ignores an A press within 20 frames of a script or message ending |
| VOLT used an overworld graphic Emerald does not load (`OBJ_EVENT_GFX_ROCKER`). He was invisible, and talking to him crashed to the title screen. | pp_solo | Guitarist graphic (`OBJ_EVENT_GFX_MAN_5`), and the POWER SHOP clerk uses the Mart clerk graphic |
| The unlinked CO-OP line printed an empty level cap (`messageinstant` does not expand `{STR_VAR_1}`) | pp_solo screenshot | `Coop_BufferUnlinkedMenu` builds the line |
| Harness: old scenarios' A-mash opened the partner menu when facing the partner | route102_reconnect and others | `autoKeys` skips A while facing the partner in the free field (test input only) |

## 5. Not tested

- Two real PCs, internet netplay, and physical cable pulls.
- New Mauville's own blue/green barrier maze: it is stock, and the quest runs warp between rooms.
- POWER SHOP stock beyond the base list, by badge count.
- Catching ROTOM and PORYGON. Only "stays after a knockout" was played; a catch uses the stock code.
- A partner menu exchange with a full PC or a full bag pocket.
- Practice as a co-op multi battle (not built).
- The five backlog quests (not built).
