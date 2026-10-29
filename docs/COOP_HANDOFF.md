# Hoenn Co-op Battle Guide v2: build 0xC017013D

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013C), pushed, not merged. Both players need this build: 0xC017013C and older refuse to link with it (the invite carries a team key now). Older saves load; nothing in the save layout moved. Test log: [COOP_TESTLOG_C017013D.md](COOP_TESTLOG_C017013D.md). Team spec and every substitution: [COOP_TEAM_SPEC.md](COOP_TEAM_SPEC.md). Stones, sources and item balls: [COOP_MEGA_MANIFEST.md](COOP_MEGA_MANIFEST.md). Player-facing: [COOP.md](COOP.md) (Story rules, Level cap, Mega Evolution).

- ROM `emerald-coop-C017013D.gba` from source `1b26b863` (no game-code change after it): SHA-1 `7ec98060cb241e0749fbc4d73e43c93f4cf05527`, SHA-256 `1ef8450dbe64423bfa1868c08392504ef80c4c88726ccf775e3a5c0b0143f71b`.
- Commits: project instructions `104eab18`; team spec, progress-based selection, caps, duo starters `982e48a7`; Mega Evolution in co-op battles `bea2ac70`; bracelet, stones, personal item balls `026d3894`; boss pairs `84f76907`; harness and first scenarios `720845e3`; ball placement rules and quest-stone room check `8fe80b9a`; scenarios and fixture updates `1b26b863`; harness pace fix `c59a587e`; docs after it.
- Final suite: 241 scenarios, 237 pass on the first run. Of the 4 failures, 3 (an egg-roll frame budget; a harness START lost mid-step and the reload that depends on it) pass on rerun after the harness fix `c59a587e`; `roster_b3_catch` failed again (driver: a wild Stunfisk in the air after Bounce, the throw refused). Details in the test log.

## What changed

- **One team spec.** `tools/coop_trainers/battle_plan.json` (the guide's embedded plan) → `guide.py` writes the parties in `src/data/trainers.party` and `battle_partners.party`, the variant rules `src/data/coop_team_variants.h` and `docs/COOP_TEAM_SPEC.md`. `overhaul.py` skips the ids the guide owns; `bosses.py` is retired. 13 new party-only ids (926-938): Lilycove and hideout early variants and the exhibition case variants. Trainer flags stay on the base ids.
- **Team selection.** `src/coop_teams.c`: progress tier (0 before badge 1, 1-8 badges, 9 League admission = `FLAG_ENTERED_ELITE_FOUR`, 10 Champion) and the starter case make a team key. A linked fight uses the lower tier of the two players and the inviter's case (sent in the invite and every answer). Ordinary rematch tiers step down while their levels pass the cap.
- **Caps** 18 / 24-60 / 66 / 76 (League admission, kept) / 80 (Champion); a Pokémon at or below the cap obeys.
- **Duo starters** Torchic+Fennekin, Mudkip+Totodile, Treecko+Chikorita with their own level-up moves; rivals use the counter type; Soren owns the case line, Mira the companion line, from the save's recorded rival (never the party).
- **Mega Evolution** on in co-op and solo (Z-Moves, Dynamax, Tera stay off). The guest's move selection says whether a Mega is allowed and already used; the master takes a Mega only if still allowed; a withdrawn choice gives it back. The partner slot needs the partner's own bracelet. The guest's healthbox shows the Mega indicator. Party Megas revert after a co-op battle with HP kept. Mega Banette, Beedrill, Chimecho, Mawile and Sableye keep the revamped base forms' HP. `ITEM_MEGA_RING` is shown as the Mega Bracelet.
- **Bracelet** from Steven on Route 118 after badge 5 (`FLAG_RECEIVED_MEGA_BRACELET` 0x4A2), each save on its own; the Exchange's MEGA STONE clerk gives it to a 5-badge save without it.
- **Stones and item balls.** `tools/coop_mega/rewards.py` (+ `place.py`): 82 stones (Exchange tiers 5 badges / 7 badges / Champion, four quest rewards, 53 balls) and 31 other balls; 88 flags from unused ranges (0x21-0x4F, 0x4A3-0x4EF). Found stones are sold again; WHERE TO FIND gives hints.
- **Boss pairs.** The second trainer stands beside each leader, Elite Four member and the Champion (`data/scripts/coop_boss_pairs.inc`); talking to either starts the same fight.

## Migration notes

- No save layout change. New flags were unused before (listed in `include/constants/flags.h` with the generator's comment).
- Existing Pokémon are untouched; saves from before keep their old companion starters (Houndour, Wingull, Shroomish); the rival is still read from the save.
- A save past Route 118's scene gets its bracelet from the Exchange clerk; a save that finished a quest before gets that quest's stone the next time it talks to the quest's person (a full bag is told to come back).
- Trainer flags are unchanged (base ids keep them), so beaten trainers stay beaten.

## Known gaps

- **Balance is not established.** Unassisted runs with the main journey's parties lost to the Petalburg Woods grunt and to Rustboro's third gym trainer, and later fights were lost with the journey's level-boosted, early-move parties (test log, Balance). No balance claim.
- Played: Roxanne/Heath as a boss pair (solo and co-op), Lilycove early/late, Route 103 all three cases, exhibitions all three cases, Megas (simultaneous, cancel, abort, bracelet ownership, HP, MEGA SOL, DRAGONIZE). Not played: the other 11 boss-pair NPCs, Elite Four/Champion pairs, Gabby and Ty, rematch tiers and hideout variants in battle, the other Z-A abilities, most of the 84 balls.
- Reed at Dewford (3,3) stands where the old journey script walked to talk to Brawly from the side; Brawly is still reached from the front.
- Mega Milotic and other custom Megas deferred (as asked).
- Harness weak points: rare-slot hunts, the visual checker's absent-battler and Safari false positives, the driver cannot pick a replacement in the stock solo-with-partner party menu. mGBA GUI, hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN`; final suite `/home/claude/journey/final13D/suite`; harness reruns `/home/claude/journey/final13D/harness_rerun`; earlier full run `/home/claude/journey/pre13D`; clips `/home/claude/journey/clips13D`; balance probes (copies of journey scenarios) `/tmp/claude-0/zz`.
- Full suite: `OUT=dir ROM=… HARNESS_OUT=dir LONG=1 SEG6=… J06=… J13=… J16=… J37=… J43=… J44=… J46=… J48=… J50=… J51=… J52=… J57=… J58=… J59=… J65=… J66=… J68=… OLD13C_COAST=<0xC017013C suite>/coast_solo_j68 tools/coop_link_harness/run_all.sh`.

# Previous build: 0xC017013C

## Co-op compatibility audit: build 0xC017013C

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013B), pushed, not merged. Both players need this build; 0xC017013B and older refuse to link with it. Older saves load (no save layout change). Audit table: [COOP_AUDIT_C017013C.md](COOP_AUDIT_C017013C.md). Test log: [COOP_TESTLOG_C017013C.md](COOP_TESTLOG_C017013C.md).

- ROM `emerald-coop-C017013C.gba` from source `cfc98e64`: SHA-1 `68a24090dd7cfb2fbba8f58b2c4abe47895c958f`, SHA-256 `42277453eaea1f574b2d8bfe76a420a3b6257327e768be1bc780b6390a8059db`; a clean build gives the same SHA-1.
- Commits: busy partner, personal rooms, win flags `148fab0a`; item moves between allies `38d818f0`; boulders not synced `36c7a370`; deal Pokémon checked `8f9c09f6`; build id `cfc98e64`; harness and regressions `ca8436f1`; docs after it.
- Final suite: 215 scenarios, 211 pass; the 4 failures are test-driver luck or timing and one reload that depends on a failed run; all pass on rerun. Details in the test log.

## Confirmed and fixed

- **A.** A gym/rival/Elite Four/story fight started on a partner with a menu screen open (bag, party, PC, shop), replacing it without closing it. Now the partner answers busy; the inviter retries for 30 seconds, then explains.
- **B.** The same with a message on the partner's screen stopped the partner's game at the battle's start (heap check). Same fix.
- **C.** Trick between the two players' Pokémon left each save holding the other's item. Item moves between link multi allies now fail.
- **D.** In Secret Bases (and Pyramid / Trainer Hill floors) the partner was drawn in your own room. Now hidden there.
- **E.** Strength boulders were synced like wandering NPCs: a push on one save moved the other's boulder (it blocked `journey_60_seafloor`). Things are never synced now.
- Hardening: incoming deal Pokémon are validated; a shared win's flag must be one of the known win flags.

## Known gaps

- Not played: cable car, ferries, Trick House, Battle Frontier and rentals, contests, berry planting, Escape Rope/Dig, Illusion, Knock Off between allies (refused by the same rule), Repel prompts around scenes. See the audit table.
- A co-op loss costs no money (as stock link battles); documented, not changed.
- Partner sprites can stand on tiles blocked in your own puzzle state (drawing only).
- Harness weak points: rare-slot hunts (`roster_b3_catch`), A-only early battles, walks that meet a wandering NPC. Finding #16 still unreproduced. mGBA GUI, hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN`; before-fix logs `/home/claude/journey/audit`; 0xC017013B runs `/tmp/claude-0/runsB`; final suite `/home/claude/journey/final13C/suite`; clips `/home/claude/journey/clips13C`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J06=… J13=… J16=… J37=… J43=… J44=… J48=… J52=… J59=… J65=… J68=… tools/coop_link_harness/run_all.sh`.

# Previous build: 0xC017013B

## Trainer Exchange and breeding: build 0xC017013B

Branch `coop-doorway-scenes-rivals` (continued from 0xC017013A), pushed, not merged. Both players need this build; 0xC017013A and older refuse to link with it. Older saves load (no save layout change; a save standing where the new building is continues in front of its door). Test log: [COOP_TESTLOG_C017013B.md](COOP_TESTLOG_C017013B.md). Checklist: [COOP_CHECKLIST_C017013B.md](COOP_CHECKLIST_C017013B.md). Stock, prices and unlocks: [COOP_EXCHANGE_STOCK.md](COOP_EXCHANGE_STOCK.md). Player-facing: [COOP.md](COOP.md) (Trainer Exchange, Breeding and the Day Care, Level cap, Known limits).

- ROM `emerald-coop-C017013B.gba` from source `f6e902ee`: SHA-1 `4833248dfa632bbb09f04ab289f9fea70cdecfef`, SHA-256 `db68ff86c3ba5207e1acd2bd429b6f8d7d2ceeb879ebac312794e3ea1aaa7c0a`. A clean build in a fresh worktree gives the same SHA-1.
- Commits: Exchange, breeding guide, Day Care side by side, old-save repair `e0eef008`; EXP candy cap message `536b98ec`; build id `0f4e0d0c`; harness and scenarios `f6e902ee` (includes the ROM's inert test hook additions); docs after it.
- Final suite on this ROM: 209 scenarios, 206 pass. The 3 failures: one scenario file overwritten by its generator (fixed, passes), one early battle lost by the A-only driver (passes on rerun), the surf hunt in `roster_b3_catch` (driver; passes on rerun).

## What changed

- **Trainer Exchange** in the south of Rustboro (door (23,52), seven steps east and seven south of the Mart's door), open before Roxanne: MOVES & TMs (TMs, 91 tutor moves by type with species compatibility, relearning ₽1,000, egg moves ₽5,000 from 3 badges), BATTLE ITEMS, EVOLUTION (every roster evolution item), TRAINING & NATURE (candies, feathers, vitamins, EV berries, mints, PP, Ability Capsule/Patch; EV reset ₽2,000; Hyper Training ₽15,000 a stat from 7 badges, ₽60,000 all six as Champion), BREEDING (Everstone, Power items, Destiny Knot, Mirror Herb, guide). Stock by the buyer's own badges (0/1/3/5/7/Champion), COMING SOON per counter. Data in `tools/coop_exchange/stock.py` → `src/data/coop_exchange_stock.h`, `docs/COOP_EXCHANGE_STOCK.md`, the `extraTutors` list.
- **One price per item** in every shop (`GetItemPrice` → `CoopExchange_PriceOverride`).
- **Co-op**: both players can use one counter (and the Day Care lady and man) at the same time, each on their own save; a shopper shows the menu icon and the partner menu says they're busy.
- **EXP candies** that reach the level cap say so; at the cap they are refused and kept.
- **Breeding guide** at the Exchange and in the Day Care.

## Move access

TMs stay single-use (as before) and are renumbered nowhere. The tutor adds 91 moves (9 from the start, 28 with 1 badge, 48 with 3, 72 with 5, 88 with 7, 91 as Champion) across all 18 types with the TMs; signature moves excluded (checked against `signatureTeachables`); each is offered only to species whose teachable learnset has it (at least 30 species each, median 162). RELEARN covers level-up and evolution moves (`MOVE_RELEARNER_LEVEL_UP_MOVES`); EGG MOVE teaches any egg move of the family (`MOVE_RELEARNER_EGG_MOVES`).

## Breeding, mints, Hyper Training (this engine)

Egg = mother's first stage (Ditto: the other parent's); babies without incense; Undiscovered can't breed. Everstone passes the holder's personality nature, not a mint's. Three IVs inherited, five with Destiny Knot; Power items pass their stat; Hyper Training is a separate bit and is not inherited (played: hyper-trained parents with IVs 0 gave an egg with IVs 0 and no hyper bits). Egg moves from either parent; Mirror Herb holder with a free slot learns the other's at deposit. Regional form: the mother's with an Everstone, otherwise the Hoenn form. Eggs can't battle, be traded or be given. Ditto: Desert Underpass after the Hall of Fame.

## Known gaps

- Breeding was played with representative families (Vulpix/Alolan Vulpix, Marill/Azurill, Pichu, Ditto) and checked from the source; not every species.
- A partner at an Exchange counter or the Day Care is reported as "shopping".
- A badge won together in a co-op gym battle is restored to a save that lacks it on LINK UP (the joint-badge record); that is the only way a partner can add to your badges and so to your stock.
- Not played: the Day Care across a reconnect (nothing of it crosses the link; source), buying with both players' bags full, gifts of every item kind.
- Harness weak points, not game faults: hunts for rare slots and A-only early battles. mGBA GUI, real hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN` (p0 Kai, p1 Rosa; j06 before Roxanne), story_seg6 `/tmp/claude-0/seg6`.
- Final suite output `/home/claude/journey/final13B/suite`; clips `/home/claude/journey/clips13B`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J06=… J16=… J37=… J43=… J48=… J52=… J65=… J68=… tools/coop_link_harness/run_all.sh`.

## Previous build: 0xC017013A

## Co-op polish: build 0xC017013A

Branch `coop-doorway-scenes-rivals` (continued from 0xC0170139), pushed, not merged. Both players need this build; 0xC0170139 and older refuse to link with it. Older saves load (no save layout change). Test log: [COOP_TESTLOG_C017013A.md](COOP_TESTLOG_C017013A.md). Checklist: [COOP_CHECKLIST_C017013A.md](COOP_CHECKLIST_C017013A.md). Player-facing description: [COOP.md](COOP.md) (How a session works, Party in co-op battles, With your partner, Quality of life, Known limits).

- ROM `emerald-coop-C017013A.gba` from source `a714c7f3`: SHA-1 `cb64e3cf311cc0cc4efae804da55b41e828137e8`, SHA-256 `0231ef9b2c1804e37e8550b1d91910a7d0a52cb9d2e8e36e329c47588c8dc154`.
- Commits: activity icons, arrow, PING, VIEW TEAM, FOLLOW `f9d07377`; shared conversations, battle targets and changes, evolution hints `4805ee12`; build id `88a6aa23`; scenarios `d327521f`; popup width `cda8c960`; busy-note reasons `6d871d34`; scene-announcement retry and B grace `ef024478`; harness `5d0460bd`; busy reason only when needed `a714c7f3`; harness budgets `2d1f1d2b`; docs after it.
- Final suite on this ROM: 194 scenarios, 189 pass. The 5 failures are test-driver luck and one timing-sensitive comparison (two early battles lost by the A-only driver, a bobbing sprite compared, a teleporting Abra, a rare Safari hunt); the scenarios were adjusted where that made sense and rerun on the same ROM: four pass; `roster_b3_catch` still caught 8 of 9 on rerun (details in the test log). Earlier: 0xC0170139 188 scenarios, 187 pass.

## What changed

- **Activity icons** over the partner (walk, talk, menu, battle, "?" invitation, waiting tick, broken link), from the partner's own state, debounced, never blinking; a steady **edge arrow** to a nearby off-screen partner (standard now; the shrine lore still unlocks BEACON STYLE colours).
- **Partner menu:** VIEW TEAM (read-only snapshot, age shown when the partner can't answer), PING (five-second cooldown; also in CO-OP), FOLLOW (walks the partner's trail on foot; stops on any button and before doors, scene tiles, ledges, water, hazards and wild terrain; no loops).
- **Joining a conversation:** A on the person the partner talks to (or on the partner) offers to join; each screen reads at its own pace; questions and movements wait for both; the host answers and the joiner sees the answer; B leaves or goes ahead; a lost link frees both. Shops, gifts, battles, warps and special scripts aren't shared; a short corner note says why.
- **Battle:** target prompts (FOE (LEFT/RIGHT), ALLY, YOURSELF, BOTH FOES, ALL OTHER POKéMON, EVERYONE, YOUR SIDE, THE FOES' SIDE, THE WHOLE FIELD) with an ally warning for damaging moves; B on the waiting message takes back a move, switch or RUN while the turn is open (master decides, both games agree; late requests refused; items can't be taken back).
- **Summary screen:** SELECT on the INFO page pages through evolution hints (target, method, conditions, met now or not, where to get the item).
- **Fixes found in testing:** scene announcements could be lost when the link was busy; long popups clipped their last letter; B right after choosing could request a change; the memo's SELECT cue overflowed; the busy-note reason was sent too often and delayed a quest's progress message.

## Known gaps

- A joined conversation follows the host's story state; the joiner isn't told when their own save would get different lines.
- Not played: the edge arrow on a connected map; FOLLOW stops at ledges, water, ice and scene tiles (source only); joining across a power-off (nothing to resume: session state is not saved).
- Items chosen in battle can't be taken back. FOLLOW is on foot only and stops at wild terrain.
- Harness weak points, not game faults: hunts for rare slots (`roster_b3_catch` caught 8 of 9 in this build's runs; its surf hunt paces only up and down and can stall in an east-west channel); A-only battles in a few early scenarios (now `smart_moves`).
- Finding #16 still unreproduced. mGBA GUI, real hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN` (p0 Kai, p1 Rosa), story_seg6 `/tmp/claude-0/seg6`.
- Final suite output `/home/claude/journey/final13A/suite`; clips `/home/claude/journey/clips13A`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J16=… J37=… J43=… J48=… J52=… J65=… J68=… tools/coop_link_harness/run_all.sh`.

# Previous build: 0xC0170139

## Ordinary roster complete, Pokédex 910: build 0xC0170139

Branch `coop-doorway-scenes-rivals` (continued from 0xC0170138), pushed, not merged. Both players need this build; 0xC0170138 and older refuse to link with it. Older saves load (save layout and species ids unchanged). Test log: [COOP_TESTLOG_C0170139.md](COOP_TESTLOG_C0170139.md). Checklist: [COOP_CHECKLIST_C0170139.md](COOP_CHECKLIST_C0170139.md). Whole-roster manifest: [COOP_ROSTER_ALL.md](COOP_ROSTER_ALL.md) (this batch alone: [COOP_ROSTER_BATCH3.md](COOP_ROSTER_BATCH3.md); data: `docs/coop_roster_manifest.json`).

- ROM `emerald-coop-C0170139.gba` from source `d4d5a863`: SHA-1 `a2456d1f99c13a852d1d8134e997e6a54a5f6978`, SHA-256 `bb8da58d8400d5cff39d2f7c5b498e227265b9f099b4cb69beab005e12846bac`.
- Commits: roster batch 3 `367e36e1`, harness and scenarios `44d6eedb`, build id `d911ba53`, Hoenn Pokédex `d4d5a863`, harness fixes `a47896d5` (test tools only; the ROM is unchanged), docs after it.
- Final suite on this ROM: 188 scenarios, 187 pass. The failure, `late_wild_tour`, is a hunt that did not catch a 4% Drifloon in time (luck), then a test-driver loop on a move with no PP; with the driver fixed it passed three reruns. Earlier: 0xC0170138 180 scenarios, 179 pass.

## 1. Totals

| | Count |
| --- | --- |
| Obtainable species, one save, alone | **908**: the whole ordinary roster (**900 of 900**: every National Dex species except the 125 deferred legendary, mythical, Ultra Beast and Paradox ones) plus the 8 Hoenn legendaries already in the game |
| Obtainable before this build (0xC0170138) | 680 (3 of this batch's 230 candidates were among them: Meowth and Persian by a one-time trade, Bonsly by Day Care egg) |
| Newly obtainable this build | **228** species: the other 227 candidates and Ursaluna (Hisuian evolution of Ursaring, not on the list) |
| Regional forms obtainable | **54** (Alolan 18, Galarian 16, Hisuian 16, Paldean 4): 7 before (Alolan Vulpix, Ninetales, Grimer, Muk; Galarian Corsola, Zigzagoon, Linoone) + 47 new. Not counted as species |
| Other forms | 991 forms in all across the 908 species (cosmetic and battle forms included) |
| Hoenn Pokédex | 910 entries (was 646): every obtainable species, plus Jirachi and Deoxys (event-only, as in stock) |
| New wild placements | 143 slots in 69 tables on 58 maps; 10 already-obtainable species also gained a wild source (mostly through their regional forms) |
| Excluded (deferred) | 125 National Dex species, listed in the checklist §2; the 8 Hoenn legendaries among them keep their existing encounters. Custom Megas (Mega Milotic) deferred |

## 2. How the hard cases work (all solo, all renewable)

- **Regional branches by place** (`GetEvolutionRegion`, src/pokemon.c): Alola = Dewford Town, Granite Cave, Routes 106 and 107; Galar = Route 114; Hisui = Mt. Pyre. Same evolution elsewhere gives the ordinary form. Played: Alolan Raichu in Dewford and Raichu on Route 114 from the same item; Alolan Marowak in Dewford, Marowak on Route 114; Hisuian Typhlosion, Sliggoo and Goodra on Mt. Pyre; Galarian Weezing and Mr. Mime on Route 114.
- **Trade evolutions:** Linking Cord (sold), or the held item used. Karrablast and Shelmet: Linking Cord with the other in the party (played both ways).
- **Galar fossils:** the Fossil Maniac offers the four pairs daily; DEVON revives (Dracozolt played).
- **Runerigus:** a Galarian Yamask down 49 HP touches the cracked carving in the Sandburied Ruins (played; the carving explains itself with no such Pokémon).
- **Battle conditions:** Annihilape (Rage Fist ×20), Sirfetch'd (3 critical hits in a battle): played in solo battles. Co-op battles are link battles and, like stock link battles, do not count them; the field guide says "solo battles".
- **Steps:** Rabsca after 1000 steps leading (played). The counter is not saved (power-off restarts it) and restarts on a lead change.
- **Other mechanisms played:** Tyrogue's three branches (stats via nature), Burmy's Sandy and Trash cloaks then Wormadam, male Burmy → Mothim, female Combee, both Meowstic, Happiny with Oval Stone by day, friendship (Blissey, Snorlax, Lopunny), known moves (Yanmega, Tangrowth, Lickilicky, Overqwil, both Dudunsparce), Mantyke with Remoraid, items (Gengar, Steelix, Electivire, Aromatisse, Slurpuff, both Sinistcha, Archaludon, Kleavor, Sneasler by day, both Galarian Slowpoke evolutions), Ursaluna (Peat Block, night, Mt. Pyre), Inkay at 30, Clodsire, Perrserker, Mr. Rime, Obstagoon, Cursola, Oricorio's nectar.
- **Changed from stock, explained in game:** Inkay at level 30 (no upside-down console); Ursaluna at night on Mt. Pyre (no full moon); Goodra in rain or fog (Route 119, Mt. Pyre summit, Rusturf Tunnel).

## 3. Pokédex

- 910 unique Hoenn entries; every obtainable species has one (now checked by `roster.py --check`); 494 AREA acquisition hints, each fits its line.
- Diplomas: legendary, mythical, Ultra Beast and Paradox entries are optional (a save with one of Latias and Latios can finish).
- Played: LIST BOTTOM reaches Hoenn No 910 (Miltank) and National No 1025; entry and AREA pages; type search; seen/caught from catches, evolutions, fossil, trade; partner markers after UNLINK/LINK UP and after a power-on.

## 4. Known gaps

- Forms: Vivillon Icy Snow only; Flabébé line Red only; Furfrou untrimmed; Minior red core; Squawkabilly Green; Dudunsparce three-segment 1 in 100.
- Not played this build: a real co-op trade evolving Karrablast/Shelmet (Linking Cord path played); breeding a regional form with an Everstone; the other three Galar fossil pairs and nectars (same code as the one played); most placements by hand (11 species caught in play, at least one per encounter method and habitat; all placements source-checked by the audit). 112 species across the three batches are now obtained in play (`verified.json`).
- Revamps: all earlier ones preserved (328 species checked, 0 differences). No new revamps this build; the wider balance plan is not part of it.
- Finding #16 still unreproduced. mGBA GUI, real hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN` (p0 Kai, p1 Rosa), story_seg6 `/tmp/claude-0/seg6`.
- Final suite output `/home/claude/journey/final139/suite`; clips `/home/claude/journey/clips139`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J16=… J37=… J43=… J48=… J52=… J65=… J68=… tools/coop_link_harness/run_all.sh`.
