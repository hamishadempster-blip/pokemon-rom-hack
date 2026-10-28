# Audit checklist, build 0xC0170138

Taken from the source, the built ROM and gameplay runs, not from earlier documents. "Source" means checked from the code, data and ROM only; "played" means a harness scenario did it (test helpers and fixtures are named in each scenario and in the [test log](COOP_TESTLOG_C0170138.md)).

## 1. Pokémon: supported, registered, obtainable, verified

| Level | Count | How it was established |
| --- | --- | --- |
| Supported by the engine | All 1,025 National Dex species and their forms (pokeemerald-expansion 1.17.1); Pokédex flags for all of them are in the save | Source |
| Registered in the regional (Hoenn) Pokédex | 646 entries, all unique; 644 obtainable | `include/constants/pokedex.h`, `roster.py` |
| Registered in the National Pokédex | 1,025 | Stock |
| Obtainable in normal play | **680 species** (712 forms), every one solo-completable | `tools/coop_roster_audit/roster.py` (wild tables on reachable maps, gifts, static and legendary encounters, roamers, Feebas tiles, in-game trades, partner Pokémon, fossils, co-op wild pairs, Day Care eggs, and evolutions whose items, moves, places and times are reachable) → `docs/coop_roster_manifest.json` |
| Not obtainable, in the regional list | Jirachi, Deoxys (event tickets only, as in stock Emerald; no new legendary encounters were added) | `roster.py` |
| Obtained in gameplay (caught, received, hatched or evolved) | 45 species this build and the last (`tools/coop_roster_audit/verified.json`), plus the quest Pokémon fought or caught in the quest scenarios | Played |

Acquisition methods in the manifest (species can have several): wild 293, partner Pokémon (daily) 24, Day Care egg 17, in-game trade 13, one-time static encounter 12, fossil 11, co-op wild pair 9, gift 8, legendary (once) 6, Birch's starter 3, roaming 2, gift egg 1; 326 species only by evolution.

## 2. This build's additions

| Item | Before | After | Played |
| --- | --- | --- | --- |
| Batch 2 (114 species) | 0 obtainable (audit of 0xC0170137's source) | 114 obtainable, 114 solo; 52 with a direct source, 62 by evolution | Guests (Rowlet, Scorbunny, Quaxly), fossil (Helix → Omanyte), catches (Lillipup, Skwovet), trade/PC/reload, every distinct evolution method (17 evolutions: Eevee by stone, by friendship by day and at night, and Sylveon; Toxtricity; both Polteageist pots; Milcery's spin; friendship and stone lines); Jolteon and Flareon (stones, the same path as Vaporeon) and Leafeon/Glaceon by place were not played, battle forms on both screens (Cramorant, Morpeko, Eiscue), POWER SHOP stock (Strawberry Sweet, Cracked Pot) |
| Regional entries that were trade-only in stock Emerald | Surskit, Masquerain, Meditite, Medicham, Budew, Roselia, Roserade, Obstagoon, Cursola: none obtainable; Zangoose and Lunatone only as co-op wild pairs | All obtainable solo: one duplicate wild slot each (Surskit Route 102, Budew and Galarian Zigzagoon Route 103, Zangoose Route 114, Lunatone Meteor Falls, Meditite Mt. Pyre outside, Galarian Corsola in the Abandoned Ship's flooded rooms) | Surskit, Galarian Zigzagoon, Budew caught (normal encounters) |
| Hoenn starters | One per save (Birch's bag); the other two only by trade | A Fallarbor guest gives Treecko, Torchic or Mudkip, one a day, from 3 badges | Mudkip received |
| Day Care eggs | Not counted by the audit (stock mechanic) | Counted: Poliwag, Snover, Roggenrola, Igglybuff, Azurill, Chingling, Bonsly and others come from their evolved forms | Poliwag egg from Politoed + Ditto (fixtures), hatched |
| **Newly obtainable species** | | **123** (114 batch + 9 repaired) | |

Fossils: all eleven (Root, Claw, Helix, Dome, Old Amber, Skull, Armor, Cover, Plume, Jaw, Sail) come from the Route 114 Fossil Maniac, one a day, any kind; DEVON revives each. No permanent either/or remains.

Renewable: every evolution item is sold (POWER SHOP, Lilycove, local Marts) or given daily (Ice Stone, Shoal Cave); every first-stage species is renewable except the legendaries.

ROM capacity: 26,987,296 of 33,554,432 bytes (80.4%). No species, item or map was added; the Hoenn order (646) fits the `u16` regional numbers; 387 AREA hints; one more daily flag (0x937; 23 of the 64 are in use).

## 3. Pokédex

| Check | Result | How |
| --- | --- | --- |
| Unique regional mapping | 646 entries, no duplicates | Source |
| Name, types, description, sprites, footprint | No issue for any of the 680 | `roster.py` reads them from the ROM |
| Seen/caught registration | Catch, gift, fossil, trade, evolution and hatch set caught | Played (roster scenarios) |
| Partner distinction | The partner's catches are a separate cached list (never the save's own); kept across a reload | Played (`deal_dex`, `roster_b2_reload`) |
| Scrolling and reach | LIST BOTTOM reaches Hoenn No 646 (Sigilyph) and National No 1025; the entry pages open | Played (`dex_probe`) |
| Search and sort | Mode HOENN, numerical; type FAIRY, A to Z finds Sylveon (type search lists caught Pokémon only, as in stock Emerald) | Played (`dex_probe`) |
| Counters | SEEN/OWN for regional and National agree with the flags (a National count is never below the regional one) | Played (`dex_probe`) |
| AREA page | Wild areas from the tables; for the 387 species without one, an acquisition hint (verified on screen: "ONE ENCOUNTER: SANDBURIED RUINS", "DAILY GIFT, SLATEPORT CENTER: 2 BADGES") | Played (`dex_probe`, earlier gift probe) |
| Unseen entries | Stay concealed (a seen-only entry shows "?????" for category, height and weight) | Played (`dex_probe`) |

## 4. Quests: implemented and verified

| Quest | Implemented (scripts) | Verified in gameplay |
| --- | --- | --- |
| A. Power Problems | Every stage: Wattson's briefing, log, three breakers with Rolycoly and Joltik, VOLT (and FUSE as a duo), main switch, appliance room with Rotom, POWER SHOP, optional sealed chamber | Solo, co-op, staggered, reconnect, reload (regression); solo on a post-League save |
| B. Shoal Cave researcher | Every stage: briefing, three clues, Isla, rescue, Ice Stones (daily), Alolan Vulpix; this build adds the low-tide times to the journal | Solo at high and low tide; left half done, reloaded, finished at the other tide; co-op; full party catch of the Vulpix; post-League |
| C. Desert Caravan | Every stage: camp, ruins puzzle, raiders (duo), glyph wall, chamber with Sigilyph, merchant | Solo, co-op, a busy partner (menu, wild battle), same-frame crate; post-League |
| D. Mira and Soren | Six meetings, two spars (Soren's team grows: Lv 26-28, then 38-40, then 49-50), exhibition double or co-op multi | Solo, co-op, reload; post-League |
| E. Coastal Cleanup | Every stage: clues, crew (duo), SLAG, Alolan Grimer, cleaned beach, fishing, Grimer kept on Route 110 and in Slateport | Solo, co-op, reload; run, loss and retry; post-League |
| F. Twin Beacon history | Every stage: three inscriptions, passage, twin plates (partner or boulder), lanterns, riddle, lore (cosmetics), Twin Trial (co-op multi or solo double) | Solo, co-op, reload; post-League |

Shared systems: the journal (objective, place, count, pending reward) and the field researcher's guide (now 42 newest arrivals and 8 partner regions) were checked on screen; the POWER SHOP grows with badges and finished quests (played); once-per-player rewards are recorded per save (played in every quest scenario).

The full matrix (initiator, maps, menu or battle, simultaneous, progress, unlink/relink, reload, full bag/party, loss/retry, post-League) is in [COOP_QUESTS.md](COOP_QUESTS.md#4-audit-and-test-matrix-build-0xc0170138).
