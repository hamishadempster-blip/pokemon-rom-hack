# Test log, build 0xC017013B (Trainer Exchange, breeding)

- Branch `coop-doorway-scenes-rivals` (pushed, not merged). ROM source commit `f6e902ee`; the commits after it change docs and test scenarios only.
- ROM `emerald-coop-C017013B.gba` (32 MB): SHA-1 `4833248dfa632bbb09f04ab289f9fea70cdecfef`, SHA-256 `db68ff86c3ba5207e1acd2bd429b6f8d7d2ceeb879ebac312794e3ea1aaa7c0a`.
- All gameplay is the two-core harness on libmgba 0.10.3 (`tools/coop_link_harness`), p0 = Kai (link master), p1 = Rosa. Fixtures and test helpers are named in each scenario's header and logged as `FIXTURE` / `TEST HELPER`. Not tried: the mGBA GUI, real hardware, netplay.

## Source checks (no gameplay)

| Check | Result |
| --- | --- |
| ROM build | A clean build of the source commit in a fresh worktree reproduces the tested ROM (same SHA-1) |
| `tools/coop_exchange/stock.py --check` | 286 items, 91 tutor moves; every one of the roster's 49 evolution items stocked (Poké Ball and the two Kubfu scrolls excluded); one price per item across counters; prices inside each tier's band; no signature move; no move both a TM and a tutor move; no key item, Mega Stone, Z-Crystal, Primal orb or Gem; all 18 types reachable through TMs and tutor moves; every tutor move learnable by at least 30 species (median 162) in the generated teachable learnsets |
| Prices | `GetItemPrice` consults the Exchange table, so every shop (Lilycove, Slateport, Mauville Power Shop, city marts, the Frontier mart) charges the same; selling is a quarter of that (`GetItemSellPrice`), so nothing bought sells at a profit |
| TMs | `I_REUSABLE_TMS` is FALSE: TMs are single-use; priced ₽2,000–12,000; no TM renumbered |
| Day Care rules read from `src/daycare.c` | Nature: `GetChildNature` uses the parent's personality (a mint's nature is not passed). IVs: raw IVs inherited (hyper training is a separate bit, not passed). Egg-move transfer (Gen 9): only a Mirror Herb holder with a free slot, at deposit. Babies without incense (`P_INCENSE_BREEDING` Gen 9). Egg cycle 128 steps. The guide text was corrected from the source before testing (it first said same-species parents share egg moves) |
| Obedience | `B_OBEDIENCE_MECHANICS` latest: own Pokémon by met level, traded ones by level; candies don't change that |
| Old saves | No save structure, flag or variable added. A save standing inside the building's footprint continues in front of its door; Rustboro's saved view is no longer restored (no script changes Rustboro's tiles) |

## Focused gameplay (new scenarios)

| Scenario | Saves | What it does | Result |
| --- | --- | --- | --- |
| `exchange_counters` | seg6 (1 badge), linked | Sign at the door, door in, guide ("own BADGES"), directory; HELD-ITEM counter: 39 for sale, ORAN BERRY bought for ₽200 (money −200, item in bag), QUIT, COMING SOON; BREEDING counter: the guide | PASS (fixture: warp to Rustboro) |
| `exchange_moves` | seg6 | TM stock 24, first TM (TM03) bought for ₽2,500; TEACH A MOVE: type list, Helping Hand ₽1,500 declined at the price (money unchanged), then taught to a Pikachu over Thunder Shock (money −1,500 once, receipt "taken ¥1500", move present, old one gone); RELEARN: Nuzzle for ₽1,000; EGG MOVE: refused below 3 BADGES; COMING SOON: "Unlocks with 3 BADGES: TM31 Brick Break, …, and 26 more" in pages | PASS (helper: the Pikachu) |
| `exchange_training` | j65 (8 badges) | Stock 47 at 8 badges (the Ability Capsule waits for the Twin Trial); RESET EVs: confirm shows ₽2,000, all six EVs from 600 to 0, money −2,000; HYPER TRAINING HP: ₽15,000, hyper bits = HP only, the IV itself stays 10; HP again: "already at its best", no charge; ALL SIX: "CHAMPION only"; a Lv. 30: "at least Lv. 50"; HOW IT WORKS; EXP. CANDY XL on a Lv. 65: stops at 66 (the cap) with "reached Lv. 66, the level cap. The rest of the EXP. was lost.", one candy used; a second at the cap: refused, kept | PASS (helpers: Pikachu at Lv. 60/30/65, EVs 100 each, IVs 10, two candies) |
| `exchange_coop` | j37 (3 badges), linked | Both press A at the HELD-ITEM clerk on the same frame: each gets their own shop (Kai 84 items, tier 2; Rosa 39, tier 1); each buys an ORAN BERRY and only their own money moves (−200 each); Kai, back on the field, sees Rosa's menu icon and A on her shows "Rosa is shopping" (no request sent, her shop untouched); Kai talks to the same clerk while Rosa is still in hers: his shop opens beside hers (side-by-side count 1); Rosa too poor (₽150): nothing bought, nothing taken; Kai's bag full at the EVOLUTION counter: MOON STONE not bought, money unchanged; Kai buys LEFTOVERS (a 3-badge item) and gives it to Rosa (2 badges): received, Kai has none; both save | PASS (fixtures: Rosa's third badge flag cleared, her money set, Kai's items pocket filled) |
| `exchange_coop_reload` | the saves above, not linked | After power-on Rosa still has the LEFTOVERS, Kai none; Rosa's stock is still her own 39 | PASS |
| `exchange_stock_t0` | j06 (no badge) | Every counter: TM 6, HELD 8, EVOLUTION nothing ("Nothing here is for sale to you yet" and what 1 badge opens), TRAINING 7, BREEDING 1; COMING SOON lines | PASS |
| `exchange_stock_t2`, `_t3`, `_t5` | j43 (4), j52 (6), j68 (Champion) | 38/84/35/23/7; 45/116/54/45/9 ("Everything at this counter is open"); 50/126/54/49/9 | PASS |
| `exchange_old_spot` + `_reload` | seg6 | FIXTURE: Kai warped onto (23,50), inside the new walls, saved; after CONTINUE he stands at (23,53) by the door, the building is drawn, he walks away and in | PASS |
| `breeding_coop` | j65, linked | Kai deposits; Rosa talks to the Day Care lady while Kai is at the chooser (side by side); each Day Care holds that save's own pair (species, personality, held items as deposited); Kai paces until his egg: his flag set, Rosa's not and her step counters unchanged; Kai's egg: ALOLAN VULPIX (mother's form with EVERSTONE), MODEST (her nature), FREEZE-DRY (mother's egg move), 5 perfect IVs (DESTINY KNOT on the DITTO); then Rosa's egg: VULPIX (no EVERSTONE: Hoenn form) with HYPNOSIS (father's egg move); Rosa's party full: the man keeps the egg; room made: received; GIVE POKéMON of the egg: "An EGG can't be traded or given."; Rosa presses A at Kai while his egg hatches: nothing opens, Kai's hatch runs on; VULPIX in Kai's Pokédex; withdrawals: each gets their own back (personality, species, held item kept); Rosa's full party: "Your POKéMON team is full", then hers back | PASS (helpers: the parents, egg and step counters, two Pikachu, a slot emptied; fixtures: warps) |
| `breeding_rules` + `_reload` | j65 | Two MARILL without incense: AZURILL egg; PICHU (Undiscovered) with DITTO: "The two prefer to play with other POKéMON than each other"; Rosa's VULPIX and DITTO, all IVs 0 and every stat hyper trained, DESTINY KNOT and POWER WEIGHT: egg left with the man over a save and power-on, then collected: HP IV 0, not hyper trained, at most one perfect IV; Kai's AZURILL egg still an egg after power-on, hatches, AZURILL in his Pokédex | PASS (helpers as named in the scenario) |

Breeding was tested on representative families (Vulpix and its Alolan form, Marill/Azurill, Pichu, Ditto) plus the source rules above; not every species' breeding was played.

### Found and fixed while testing

- COMING SOON lists ran off the message box (two names a line, a third line drawn below the frame). Names are now packed by width and scrolled a line at a time; TMs are named with their move ("TM31 Brick Break").
- The TM list was in stock order; it is now in TM number order.
- The tutor receipt ("I've taken ¥…") and the EV reset question showed a wrong price ("₽ 67"): the party and teaching screens reuse the string buffers. The price is written again before each message.
- The breeding guide said two Pokémon of a species share egg moves in the Day Care; in this engine (Gen 9) only a Mirror Herb holder with a free slot learns them, at deposit. The guide and COOP.md now say that.
- An old save inside the new building's footprint would load inside the walls, and any save near it showed the old ground until Rustboro was reloaded (repair above).

## Full regression on the final ROM

`run_all.sh` with `LONG=1` (story chain), `SEG6`, `J06`, `J16`, `J37`, `J43`, `J48`, `J52`, `J65`, `J68` on ROM `4833248d…`: **209 scenarios, 206 PASS, 3 FAIL**. Visual-check notes: 4, the same three as on 0xC017013A (`finding16_wild_teleport_karen`, `finding16_partner_wild_karen`, `coop_battle_items`: a battler with no name for a frame at a send-out, a known harness finding) and one in `roster_b3_catch`.

The three failures, and what was done (reruns on the same ROM):

| Scenario | What failed | Cause | After |
| --- | --- | --- | --- |
| `breeding_coop` | Its step-counter check and the hatch step | The scenario file in the suite was an older draft (a generator script rewrote it after the fixes; scenario fault, not the ROM) | Fixes re-applied; PASS on rerun (all its checks) |
| `rival_looks` | The early co-op rival battle was lost | The A-only driver (Leer and Tackle); timing between the cores decides the encounter's luck; this scenario only takes screenshots | PASS on rerun |
| `roster_b3_catch` | The surfing Galarian Stunfisk hunt ran out of budget (8 of 9 caught) | The hunting driver (as on 0xC017013A) | PASS on rerun (all nine caught) |

Regressions that matter for this build and all passed in the suite: the shops and economy (`shop_b2`, `quest_goods`, `caravan_*`, `pp_*` with the Power Shop), deals and gifts (`deal_*`, with power cuts and reloads), the roster and evolution runs (`roster_*`, `evo_hint`), the earlier Day Care egg (`roster_breeding`), link and save scenarios (`reconnect`, `save_identity`, `legend_*`), and the story chain.

