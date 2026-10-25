# Test log: build 0xC0170132

ROM built on branch `coop-doorway-scenes-rivals` from the commit that adds this file (the source tree tested is that commit; the docs in it do not change the ROM).

| Checksum | Value |
| --- | --- |
| SHA-1 | `3f196cfb7ffcd432a1ac08447965e9384d853a22` |
| SHA-256 | `cbd33407a5bd0593577c0457c777fa9f7af735e90002703b5c22a50375c7a47c` |

All two-player results come from two mGBA 0.10.3 cores joined by the emulated link cable (`tools/coop_link_harness`). They are not two PCs and not netplay. "Gameplay test" below means the scenario drives the real ROM with button presses and reads game memory; fixtures (warps, flags, a strong test team) are named in each scenario's header. "Source check" means reading code or data only.

## 1. Data validation

| Check | Result |
| --- | --- |
| `tools/coop_roster_audit/audit.py` (after `build_layout.sh`) | 210 species, 0 issues |
| Build | `make` clean; ROM 80.3% of 32 MB |

## 2. Full regression (`run_all.sh`, `SEG6` set): 102 scenarios

99 passed on the first run, 3 failed:

| Scenario | First run | Cause | Rerun |
| --- | --- | --- | --- |
| twins_solo | FAIL | Test driver: during the exhibition a forced switch opened the summary screen; the harness's "back out of the summary screen" used the wrong `MainCB2` (several files have a static of that name). Fixed in `harness.c` (`autoKeys`). | PASS |
| twins_reload | FAIL | Depends on twins_solo's save (it had not saved). | PASS |
| wild_simultaneous | FAIL | Pre-existing race, already logged for 0xC0170131: when both games roll a wild encounter on the same frame, one gets "Solo: your partner didn't answer." instead of "...is busy." Both still get their solo battle. | 3 of 6 reruns pass; unchanged from the previous build's behaviour |

## 3. New scenarios (gameplay tests, all PASS in the suite run above unless noted)

| Feature | Scenarios | What they prove |
| --- | --- | --- |
| Rival looks | rival_looks | Route 103 twins with the new overworld sprites; the co-op battle intro shows the SOREN and MIRA trainer pics (screenshots). Plus the rival regressions: rival_join, rival_pending, rival_latejoin, rival_simultaneous, rival_reconnect, rival_story, rival_p1, rival_late, solo_rival, story_seg1, story_seg2, bedroom_reload, mays_house_doorway. |
| Outfits, different bases | outfit_bases, outfit_bases_reload | Wardrobe preview and choice; each partner sees the other's colours; doors and map changes; trainer card after a power cycle. |
| Outfits, same base | outfit_same_base, outfit_same_base_reload | Two Brendans in different colours at once (overworld, surf, cave, co-op battle back sprites); marker AUTO/OFF; survives reload. |
| Shoal Cave | shoal_solo, shoal_reload, shoal_coop, shoal_coop_reload | Solo end to end; deferred gift on a full bag; daily stone; clues shared live and merged after an offline find; catch-up for an absent partner; both talking at once paid once each; persistence. |
| Desert Caravan | caravan_solo, caravan_reload, caravan_coop, caravan_coop_reload | Crates (partner's crate removed live), raiders solo and as a co-op duo, glyph order with a wrong press, chamber and SIGILYPH (knocked out, stays), deferred reward, merchant, catch-up, partner's chamber stays its own, persistence. |
| Mira and Soren | twins_solo (rerun), twins_reload, twins_coop, twins_coop_reload | All six meetings with badge fixtures, both spars and prizes; met twins leave on the next visit; partner's meeting gives the short version; exhibition solo (double) and co-op (multi), gifts deferred by a full bag then given once; persistence. |
| Coastal Cleanup | coast_solo, coast_reload, coast_coop, coast_coop_reload | Clues gate the crew; Grimer; crew solo and as a duo; SLAG; clean beach; deferred reward; catch-up while the partner is in Slateport; persistence. |
| Twin Beacon | beacon_solo, beacon_reload, beacon_coop, beacon_coop_reload | Inscriptions (shared live), Slab passage, plates with the boulder (solo) and with the partner (co-op), lanterns alone and together, wrong and right riddle, door opening for the partner on the spot, lore, GOLD style, compass when the partner is off screen, Twin Trial solo (three doubles) and co-op (three multi rounds), one capsule each, persistence. |
| POWER SHOP stalls | quest_goods | With all quests done the four stalls appear and the BEACON stall sells its stock. |

## 4. Source checks only (not exercised in play)

- Clean-coast fishing table (`CoopQuest_FishingOverride`) for Route 109 and Dewford: code read, not fished in a test.
- Caravan merchant's 6-badge stock and the POWER SHOP's ICE/SEA/DESERT stalls: lists read; only the BEACON stall was opened in play.
- The daily RARE CANDY for repeat trial wins and the daily ICE STONE reset at midnight (daily flags are the stock mechanism).
- Journal pages for each quest at every step: the first and the finished pages were shown in play; intermediate pages by code reading.
- Real hardware, netplay and two PCs: not tested.

## 5. Harness changes

- `fixture_gender`, `fixture_fill_pocket`, `fixture_unfill_pocket`, and the `coop_outfit` and `text` statics.
- `smart_moves 2`: a later move wins a tie on power (Ember over Scratch, so a test team doesn't loop on a Ghost type it can't hit).
- `autoKeys`: anything in the summary screen's code during a battle backs out with B.
- `run_all.sh` runs the outfit, rival-look and quest scenarios and their reloads.
