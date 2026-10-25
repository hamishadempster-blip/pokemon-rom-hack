# Visual correctness and co-op polish: build 0xC0170133

Branch `coop-doorway-scenes-rivals`, not merged. Both games need this build (0xC0170132 refuses to link with it). Save layout unchanged; the new NAME TAG setting uses spare bits of `CoopSave.partnerMarker` (0 = ON on older saves). Test log: [COOP_TESTLOG_C0170133.md](COOP_TESTLOG_C0170133.md).

## 1. Patterned blocks after a level-up (host screen)

- **Cause, confirmed.** The link VS screen leaves BG1 set to 512x256 in the BG library and its frame in the BG1/BG2 tilemap buffers; `BattleIntroSlideLink` (battle_intro.c) then writes the battle's own BG1CNT/BG2CNT straight to the registers, so library and registers disagree. Stock link battles never draw on BG1 again. When the co-op level-up stat box redraws BG1, the library re-applies the stale 512x256 size (BG1CNT 0x5C00/0x5C01) and copies the stale buffer, so the VS frame comes back as slate blocks for the rest of the battle. Only the host draws level-up boxes in a co-op battle (stock link battles award no Exp.), so only Player 1's screen showed it.
- **Fix.** Where the slide restores the registers, it also restores the library's screen sizes (BG1 256x512, BG2 512x256) and clears both tilemap buffers, so the two agree and the next window copy starts from empty maps. Level-up boxes, move learning and animations are unchanged.
- **Evidence.** 0xC0170132, story_seg5 from seg4 saves: BG1CNT 0x5C00/0x5C01 in 159 of 1133 battle snapshots, blocks in the frames after the Rustboro rival level-up box. 0xC0170133, same saves and inputs: 0 of 1167; the box draws and clears (clip `rival_levelup_before_after.mp4`).

## 2. One trainer in a co-op battle

- **Cause.** A co-op fight against one trainer is a multi battle (two players) without `BATTLE_TYPE_TWO_OPPONENTS`, but the intro, the VS screen, the send-out and the victory text assumed a second opponent: a second trainer picture, a send-out for an uninitialised fourth battler (the Lv0 box and the nameless send-out), "You defeated GRUNT and !", and the replacement message read the right flank's buffer ("SOREN sent out Marshtomp!" on Player 2 while Nacli came out).
- **Fix.** `IsCoopSingleTrainerBattle()` (battle.h) steers the intro (one centred trainer pic, no second send-out animation), `TwoMonsAtSendOut` (the trainer's own two Pokémon), the VS screen, the trainer pic positions, INTROSENDOUT, BATTLEEND ("You defeated TEAM AQUA GRUNT!") and SWITCHINMON (each flank's own name buffer).
- **Found by this pass's own regression: a stall.** With the second trainer gone, a single trainer's two Pokémon come from one party. When both fainted on the same turn with one left in reserve (story_seg3, Bug Catcher Lyle), both slots asked for that last Pokémon and the battle waited forever. `HasNoMonsToSwitch` (battle_util.c) now lets only the left slot send it out, as the stock 2-vs-1 case does. `coop_one_trainer_faints` reproduces it (stalls with the check disabled, passes with it).
- **Empty-field actions after a KO, investigated from the battle log, not the picture.** With `BATTLE_LOG`, every case found was the stock doubles rule: when both foes faint before a slower ally acts, that ally's move has no target and fails ("But it failed!"); replacements come at the end of the turn. Example: story_seg4 Roxanne/Heath, Lileep fainted to Emberdash, then Sootfang's Smog failed with both foe slots absent. Not changed. The phantom fourth battler of the old build is gone, so no action resolves against an uninitialised slot.

## 3. Day and night

- **Cause, confirmed: palette restoration, plus two clocks.** The CO-OP menu callback and the link wait called `UnfadePlttBuffer`, which copies the untinted palettes over the tinted ones, so the outdoor night tint vanished after the menu or a link. Separately, each game tinted by its own clock, so two linked players could see night and noon side by side.
- **Fix.** `Coop_RestoreFieldPalettes()` ends the fade, unfades, and re-applies the time-of-day tint and the weather colour map. While linked, the host sends hours and minutes (`DMSG_CLOCK`, on change) and the guest's `UpdateTimeOfDay` uses them; nothing is written to either clock or save, so berries and daily events keep each game's own clock. Lighting, time-based evolutions and their hints, encounters and night music follow the time shown. Unlinking (or a drop) re-tints from the game's own clock.
- **Evidence.** lighting_link with the two clocks at 23:00 and 12:00: 11 failed checks on 0xC0170132 (tint lost in the menu, two times of day when linked), 0 on 0xC0170133.

## 4. Battle presentation

- The "Partner choosing…" line that overlapped "What will … do?" is gone. After choosing, the screen says "Ready. Waiting for <partner>…".
- Only the link host's Pokémon learn moves inside the battle. The other screen used to show the host's YES/NO question ("Should another move be forgotten…?"); it now says "<host> is choosing whether <Pokémon> learns <move>…".
- Target selection already names the slot and species ("Left foe Nacli", "Partner …", spread moves, "Hits partner!"); unchanged here.
- Test runs can give distinct, realistic nicknames (`fixture_nick`), so both screens' names are checkable; max-length (12-character) names stay covered by the story saves.

## 5. Partner visibility and status

- **Name tag**: the partner's name in small text on a dark pill above their head (`coop_outfit.c`), shared palette with the marker, one 64x32 sprite. Shown only while this player can move (hidden in menus, messages, scenes, warps), only for a partner drawn on this map and on screen; rebuilt after a map load, removed when the partner leaves or the link ends (no ghost). Drawn on the partner's layer, in front of characters, so the name stays readable when the local player stands just north. CO-OP menu > NAME TAG cycles ON (default), NEARBY (three steps), OFF; saved. The head arrow is suppressed while the tag shows; the edge compass is unchanged.
- **Status window** (start menu, five lines): link and Beacon ("LINKED · no BEACON yet" before the item), "<name>: exploring / talking / healing / in a menu / busy / battling" ("healing" when the partner's talk target is a nurse in their map's event table; "battling" after a second of BUSY, so doors do not read as battles), "<map>, with you" or "<map>, inside", fights together or solo and why, rival. "Link trouble: retrying..." only when the cable has stalled.
- **Door notes**: one "<name> went inside" per 20 seconds, not one per door.

## 6. Story flow

- Defined: a fight won together is shared, everything else is personal (state, reward, catch-up). Story flags are never copied wholesale (COOP.md, "Shared and personal outcomes").
- Petalburg Woods: a partner who joined the grunt fight and won gets a short catch-up from their own researcher, their own GREAT BALL and their own `VAR_PETALBURG_WOODS_STATE`, instead of replaying the scene and the fight (`Coop_QueueGuestFollowup`, `Coop_EventScript_GuestWoods`). A partner who did not join still meets the scene. Ordinary trainers stay independent.
- UNLINK: "You unlinked. You're playing solo." for the one who chose it, "<name> unlinked. …" for the other; "The link dropped." only for a real fault.

## 7. Known gaps

- **pp_coop stops in the VOLT + FUSE duo battle** (New Mauville) with the regression's seg6 saves: the host waits for the guest's battler to finish a move and it never does. Build 0xC0170132 stops at the same point with the same saves, so it predates this pass; not fixed. Repro: `pp_coop` on story_seg6 saves; the harness's stop dump now includes animation, healthbox and task state.
- No in-battle state for a partner whose link stalls mid-battle; the stock link-error handling still applies.
- The tag can be clipped at the top screen edge; when the local player stands just north of the partner it covers the player's legs (readability chosen over layering).
- Status "healing" covers the nurse only (not a Centre PC or Mum); "battling" is inferred from a lasting BUSY.
- Only the Woods scene has a joined-fight catch-up; other optional story fights with a scene (none found up to Rustboro) would need their own.
- Unmodified-party runs: segment 3 passes; segment 4 loses to the first gym trainer (one Torchic/Houndour each against Glimmet's Rock Throw) and whites out home. That is a balance finding of the one-Pokémon walking chain, not changed here.
- mGBA's GUI could not be run in this environment (no display); all recordings come from the two-core harness built on libmgba 0.10.3. Real hardware, netplay and two PCs were not tested.

## 8. Results on the delivered ROM

- Full regression (`run_all.sh`, `SEG6`, `LONG=1`): 110 runs, 109 pass, `pp_coop` fails (pre-existing, §7). Visual checks on all 110 logs: 0 failures; the same checks fail the 0xC0170132 logs where the user's recordings showed the bugs.
- New: `nametag`, `lighting_link`, `coop_one_trainer_faints` pass; `story_seg3` now covers the Woods catch-up; `story_seg3_real` passes without stat fixtures; `story_seg4_real` loses (§7).
- Clips (both screens, same build per side): Woods grunt, Rustboro rival level-up and replacement, lighting across link/menu/Centre/unlink, partner tag and status.

# Rival looks, outfits, five optional quests: build 0xC0170132

Branch `coop-doorway-scenes-rivals`, not merged. Test log: [COOP_TESTLOG_C0170132.md](COOP_TESTLOG_C0170132.md). Quest walkthroughs and data: [COOP_QUESTS.md](COOP_QUESTS.md). Player text: [COOP.md](COOP.md).

## 1. Mira and Soren look like themselves

- New overworld sheets (walking, running, Mach Bike) and trainer front pics, made by `tools/coop_art/rivals.py`: Emerald's May/Brendan bodies and frames, with redrawn heads (Mira: wide-brim sun hat; Soren: spiked hair and a gold headband) and new palettes, so the silhouettes differ, not only the colours.
- Gfx ids `OBJ_EVENT_GFX_SOREN_NORMAL/_MACH_BIKE`, `OBJ_EVENT_GFX_MIRA_NORMAL/_MACH_BIKE`; palette tags 0x1134/0x1135; `TRAINER_PIC_SOREN/MIRA`. All 60 rival trainer parties use them; `rival_graphics.inc`, the Littleroot houses, `coop_rivals.c` (`Coop_IsRivalGfx`, `Coop_TwinGfx`), and the PokéNav match-call list point to them. Assignments, teams, dialogue and flags are unchanged.
- Fix found on the way: the gfx-info pointers must sit after `#endif // IS_FRLG` (they were invisible and crashed on reset).

## 2. Player outfits

- Wardrobe bg event at (4,1) in both Littleroot bedrooms → `CoopOutfit_OpenMenu` (`src/coop_outfit.c`): six presets per base (`src/data/coop_outfits.h`, from `tools/coop_art/outfits.py`), live preview on the trainer pic and the player sprite, plus the partner MARKER setting (AUTO/ON/OFF).
- Stored per save in `CoopSave.outfit` and `CoopSave.partnerMarker` (the two former unused bytes; 0 = CLASSIC/AUTO, so older saves are safe). Sent to the partner with `DMSG_LOOK` and re-sent on every link.
- Overworld palettes go through `ObjectEventSpritePalette()`; the partner uses its own gfx ids and palette tag 0x1136, so two players with the same base wear different colours at once and nobody else is recoloured. Battle back/front pics (`GetTrainerFront/BackPicPalette`, `CoopOutfit_BattleTrainerSprite`) and the trainer card use the same colours.
- Marker: its own palette 0xC0B5; after the shrine lore it uses `VAR_BEACON_STYLE`, and doubles as the edge compass for an off-screen partner on the same map.

## 3. Five optional quests

Shoal Cave, the Desert Caravan, Mira and Soren, Coastal Cleanup, and the Twin Beacon's history; details in COOP_QUESTS.md §3. Foundations added for them:

- `battle_setup.c`: trainer-flag helpers ignore ids at or above `MAX_TRAINERS_COUNT`, so party-only trainers (917–925) never write system flags.
- `coop_quest.c`: OR-merged live values with a partner popup and live removal of a partner-found object; specials for bits, milestones, twins, plates, lanterns, glyphs, the Slab passage, trial heal/solo battle, shared shrine gates, and the clean-coast fishing table.
- `coop.c`: five new ledger milestones; `Coop_IsOptionalQuestTrainer` (Rival/Magma-Admin-class quest trainers are not refused while linked); the joining side heals before a Twin Trial round.
- Maps `SANDBURIED_RUINS` and `TWIN_BEACON_SHRINE`; Route 111 mound and entrance; Scorched Slab passage; journal with six quests; POWER SHOP stalls.

## 4. Decisions taken

- **Co-op story battles resume only the host's script.** The guest returns to the field with no script, so every guest-side result is applied by flags (the battle's flag on both saves, then `ON_RESUME`/`ON_TRANSITION` or the next talk). The exhibition records the guest's trainer flags when the guest next talks to the twins; the trial's guest gets `FLAG_TOURNEY_WON` from the battle (nothing reads the final pair's trainer flags).
- **The Twin Trial is one round per talk**, not three chained battles from one script: each round starts from a fresh, idle field on both sides, teams are restored each time, and leaving or losing starts over.
- **Shrine gates are world state** shared live (`COOP_QUEST_SHRINE_GATES`); the sanctum door is redrawn for a partner standing in the hall.
- **Lanterns lit at the same moment** solve on both: the later message opens the gate on the side that lit first.
- **Inscriptions are outside the Regi doors** (Desert Ruins, Island Cave, Ancient Tomb stay closed until the braille puzzle).

## 5. Known gaps

- The shrine's twin plates are not drawn; the verse signs beside them, the "plate gives a little" message and the journal are the hints.
- Two harness intermittents remain: `wild_simultaneous` (pre-existing wording race) and the harness's forced-switch handling, which was fixed for the summary-screen case found here.
- Fishing on the clean coast, the 6-badge merchant stock, the other three POWER SHOP stalls, and daily resets were checked in source only.
- Real hardware, netplay and two physical PCs were not tested.

# Roster, partner menu, QoL, Power Problems: build 0xC0170131

Branch `coop-doorway-scenes-rivals`, on top of `060e0121` (0xC0170130). Keep
it unmerged. Both games need this build: 0xC0170130 refuses to link with it.

Existing saves load:
- The co-op save block grows into its spare bytes (layout 2), and older saves get the new fields zeroed on load.
- A save that already finished New Mauville counts as "power restored".

**Delivered ROM:** built from `7bef09dd`; later commits change only docs.
- SHA-1 `5bce1e5fe6603ba6b1e9b04a71de69247c1b1175`
- SHA-256 `6101d18262c502ec1673fe8bee5e0f5a5db0c71bf5b31e9b0169f327b2a76574`

Full test evidence: [COOP_TESTLOG_C0170131.md](COOP_TESTLOG_C0170131.md).
Quest design and the backlog: [COOP_QUESTS.md](COOP_QUESTS.md).

## 1. Roster: 24 new families

Placed by `tools/coop_roster_audit/place_families_g.py`. Each family is in the Hoenn Pokédex after Frosmoth and is catchable on each save. Levels per spot are in [COOP.md](COOP.md#roster-changes). The field researcher (Petalburg, Mauville, Fortree and Lilycove Pokémon Centers) gives the same hints in game.

| Family | Where | Evolution access |
| --- | --- | --- |
| Buizel | Route 104, grass and water | Lv 26 Floatzel |
| Sewaddle | Petalburg Woods | Lv 20 Swadloon; level up with high friendship: Leavanny |
| Venipede | Petalburg Woods, Rusturf Tunnel | Lv 22, Lv 30 |
| Rolycoly | Rusturf Tunnel, New Mauville | Lv 18, Lv 34 |
| Skiddo | Routes 116, 118 | Lv 32 |
| Drilbur | Granite Cave | Lv 31 |
| Fomantis | Routes 117, 123 | Lv 34 by day (6:00–19:59) |
| Darumaka | Fiery Path | Lv 35 |
| Sizzlipede | Fiery Path | Lv 28 |
| Salandit | Fiery Path, Magma Hideout; a woman in Lavaridge gives one female per save | Females only: Lv 33 Salazzle |
| Scraggy | Route 111 desert | Lv 39 |
| Mudbray | Route 111 desert, Route 112, Jagged Pass | Lv 30 |
| Rufflet | Route 112, Jagged Pass | Lv 54 |
| Vullaby | Route 113, Jagged Pass | Lv 54 |
| Clobbopus | Route 115 | Level up knowing Taunt (learned at 35) |
| Clauncher | Good Rod: Routes 106, 107, Dewford | Lv 37 |
| Skrelp | Surf: Routes 105, 109; Super Rod: Routes 108, 109 | Lv 48 |
| Grimer (Alolan) | Surf: Slateport harbour, Route 110 | Lv 38 |
| Pancham | Route 119 | Lv 32 with a Dark type in the party |
| Solosis | Routes 119, 120 | Lv 32, Lv 41 |
| Gothita | Route 120 | Lv 32, Lv 41 |
| Vulpix (Alolan) | Shoal Cave ice rooms, low tide | Ice Stone (Lilycove, Mossdeep, POWER SHOP from 7 badges) |
| Larvesta | Magma Hideout (rare) | Lv 59 |
| Deino | Meteor Falls back room (Waterfall) | Lv 50, Lv 64 |

New Hoenn habitats for existing lines: Ledyba (Routes 102, 117), Spinarak (Routes 102, 119), Hoothoot (Routes 116, 121), Sunkern (Routes 116, 123, Safari Zone), Seel (surf: Routes 105, 125, Shoal Cave). New Mauville also gives PORYGON (per save, sealed chamber): its Up-Grade is in the chamber and the POWER SHOP sells both Up-Grade (6 badges) and Dubious Disc (7 badges), each usable from the bag.

## 2. Revamps

Stats are HP/Atk/Def/SpA/SpD/Spe. Full move notes are in [COOP.md](COOP.md#roster-changes).

| Pokémon | Before | After | Role |
| --- | --- | --- | --- |
| Ledian | 55/35/50/55/110/85 | 70/105/65/35/95/95, Iron Fist first | Physical puncher: Mach Punch, elemental punches, Drain Punch |
| Ariados | 70/90/70/60/70/40 | 90/100/85/50/85/40, Stakeout | Trapper: Toxic Thread, Sticky Web, Knock Off |
| Sunflora | 75/75/55/105/85/30 | 85/50/70/125/90/30, Grass/Fire, hidden Drought | Sun attacker: Weather Ball, Heat Wave, Earth Power |
| Magcargo | 60/50/120/90/80/30 | 85/45/120/85/105/30, Solid Rock first | Wall: Stealth Rock, Recover 40, Will-O-Wisp, Body Press |
| Dewgong | 90/70/80/70/95/70 | 100/60/90/80/105/60 | Doubles support: Life Dew, Fake Out, Icy Wind, Encore |
| Noctowl | 100/50/50/86/96/70 | 110/40/60/95/110/70 | Screens: Light Screen, Reflect, Roost, Hypnosis |

## 3. Features

| Feature | Where | Notes |
| --- | --- | --- |
| Partner menu | A facing the linked partner | BATTLE, TRADE, GIVE ITEM, GIVE POKéMON, CANCEL. Replaces the party screen's TRADE. Escrow in the save for trades and gifts (`src/coop_deal.c`). An A press within 20 frames of a message closing is ignored, so mashing through text does not open it. |
| Friendly battles | Partner menu, BATTLE | Singles or doubles; own levels, Lv 50 or Lv 100; up to 6, 4 or 3; STANDARD (no repeated species or items) or UNRESTRICTED. No rewards; parties restored; a link error ends it for both. |
| Trades and gifts | Partner menu | Both confirm the exact exchange. Restrictions: last usable Pokémon, eggs, key items, HMs, Mail. Trade evolutions. Saved on both sides before completing; interrupted exchanges settle on the next link of the same saves. |
| Partner Pokédex | Pokédex list; CO-OP, PARTNER DEX | You, partner, both, or neither; partner name and play time of the last sync; kept offline. |
| BATTLE SCENE: FAST | OPTION | Skips animations, halves message pauses; shared battles use what both allow. |
| Save identification | CONTINUE, SAVE window | Location and "Build C0170131". |
| Practice COACH | Nine Pokémon Centers | AI battle against a copy of your or your partner's team; singles or doubles; own levels or Lv 50; nothing awarded, everything restored. |
| Quest journal | CO-OP, JOURNAL | Linked and unlinked. The unlinked CO-OP list is now LINK UP / JOURNAL / PARTNER DEX / LEVEL CAP / BACK. |
| New Mauville: Power Problems | Mauville, New Mauville | Breakers, ROLYCOLY and JOLTIK, VOLT (and FUSE as a duo), MAIN SWITCH, APPLIANCE ROOM ROTOM, sealed chamber, WATTSON's reward, POWER SHOP. Solo or co-op. See [COOP_QUESTS.md](COOP_QUESTS.md). |
| POWER SHOP | Mauville, by the Gym | Repeatable evolution items, more at 6, 7 and 8 badges. |
| Duo trainers | VOLT and FUSE | Duo with partner + Beacon on, solo otherwise or on NO. |
| Per-save rewards and shared milestones | `CoopSave.rewardsClaimed`, story ledger | Defined in [COOP_QUESTS.md](COOP_QUESTS.md#shared-and-personal-state). |

## 4. Deferred quests (backlog)

These are not in this build and are not needed for this update's Pokémon or items. Outlines are in [COOP_QUESTS.md](COOP_QUESTS.md#3-backlog-five-quests-for-later).

| Quest | Place | Core idea |
| --- | --- | --- |
| Shoal Cave: the missing researcher | Shoal Cave | Tide-split search; notes shared live |
| Desert caravan | Route 111 desert, Desert Underpass | Scout and escort; desert duo trainers with a solo option |
| Mira and Soren: separate ambitions | After Lilycove | Each player follows their own twin; joint finale |
| Coastal cleanup | Slateport, Routes 105, 109, Dewford | Pooled pickups, per-save tiers |
| The Twin Beacon's history | Birch's lab, Mossdeep, Sky Pillar | Two beacon sites lit together, or one after the other alone |

## 5. Tests

All play results are from two mGBA 0.10.3 cores on the emulated cable. Details and exact runs are in [COOP_TESTLOG_C0170131.md](COOP_TESTLOG_C0170131.md).

| Kind | What | Result |
| --- | --- | --- |
| Data validation | Roster audit (habitats, evolution access, graphics, dex text, learnsets) | 210 species, 0 issues |
| Data validation | Trainer validator | 852 trainers, 0 errors |
| Data validation | Text width on new scripts; host protocol test | Pass |
| Gameplay, fresh saves | Opening, rescue, bedroom, rival scenes (17 scenarios) | 17/17 pass |
| Gameplay, lab saves | Sessions, wild and trainer co-op, gyms, E4, solo paths (31 scenarios) | 29 pass; 2 timing- or RNG-dependent ones failed once and passed 2/2 on rerun |
| Gameplay, walking story | story_seg1, whiteout_home, story_seg2, bedroom_reload | 4/4 pass |
| Gameplay, existing saves (from 0xC0170130) | Partner menu: trades, gifts, interruptions and reloads, friendly battles (13) | 13/13 pass |
| Gameplay, existing saves | QoL: partner dex (+offline), FAST, practice, save identity (5) | 5/5 pass |
| Gameplay, both roles | Power Problems: solo, linked split + duo + catch-up, reconnect, staggered, staggered reload (5) | 5/5 pass |
| Gameplay | Roster: female Salandit gift evolves at 33, Buizel at 26, unlinked level cap toggle | Pass |
| Source only | Partner menu: eggs, full PC, full pocket, HM gifts; POWER SHOP tiers 6–8; Porygon item evolutions | Not played |

Bugs found by these runs and fixed:
- A damaged deal block could stall the next exchange.
- Mashing A through a scene next to the partner opened the partner menu.
- VOLT's overworld graphic did not exist.
- The unlinked CO-OP line printed an empty level cap.

## 6. Not tested, and known limits

- **Not run here:**
  - Real two-PC play: all two-player results are two mGBA 0.10.3 cores on one emulated cable.
  - Internet netplay.
  - Physical cable pulls: link loss was tested only by injected errors and stalled frames.
- **Partner menu, source only:**
  - eggs offered;
  - a full PC refusing a received Pokémon;
  - a full bag pocket refusing a gift;
  - HM items.
- **Not built:** practice against AI copies as a co-op multi (both players together against copies).
- **Escrow:** if a partner's save holding a prepared exchange is lost for good, your side stays pending (the start menu says so).
- **Both asking at once:** player 1 is told "<name> is busy right now", then asked.
- **Power Problems:**
  - The breakers sync whenever both games are linked, even when one player is far from New Mauville.
  - Partner stops the generator while you are in the same room: it stays running on your screen until you leave the map, and you can also throw the switch yourself.
  - The sealed chamber can be seen across the void from the generator room before it is opened.
- **Harness fixtures (in the scenarios):**
  - New Mauville's stock blue/green barrier maze is skipped with `fixture_warp`.
  - The evolution checks set EXP one point below the level.
- **Test input changes:**
  - The harness's A-mash no longer presses A while facing the partner in the free field (it would open the partner menu).
  - The unlinked CO-OP list replaced the old link question; `relink.inc` and `deal_dex_offline.txt` follow it.

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
