# Audit checklist, build 0xC0170139

Taken from the source, the built ROM and gameplay runs, not from earlier documents. "Source" means checked from the code, data and ROM only; "played" means a harness scenario did it (test helpers and fixtures are named in each scenario and in the [test log](COOP_TESTLOG_C0170139.md)).

## 1. Pokémon: supported, registered, obtainable, verified

| Level | Count | How it was established |
| --- | --- | --- |
| Supported by the engine | All 1,025 National Dex species and their forms (pokeemerald-expansion 1.17.1); Pokédex flags for all of them are in the save | Source |
| Intended roster | 900 ordinary species: the National Dex minus the 125 deferred legendary, mythical, Ultra Beast and Paradox species (§2) | `roster.py` (the set matches the engine's species flags) |
| Obtainable in normal play | **908 species** (991 forms): **900 of 900 ordinary** + 8 Hoenn legendaries already in the game; every one solo-completable on one save | `tools/coop_roster_audit/roster.py` → `docs/coop_roster_manifest.json`, readable as [COOP_ROSTER_ALL.md](COOP_ROSTER_ALL.md) |
| Regional forms obtainable | **54**: Alolan 18, Galarian 16, Hisuian 16, Paldean 4 (7 before this build) | Source (manifest; [COOP_ROSTER_ALL.md](COOP_ROSTER_ALL.md#regional-forms-54-obtainable-not-counted-as-species)) |
| Registered in the Hoenn Pokédex | 910 entries, all unique: every obtainable species, plus Jirachi and Deoxys | `include/constants/pokedex.h`, `roster.py` |
| Not obtainable, in the Hoenn list | Jirachi, Deoxys (event tickets only, as in stock; deferred class) | `roster.py` |
| Obtained in gameplay | 112 species across the three batches (67 this build): caught, received, revived, hatched or evolved | `tools/coop_roster_audit/verified.json`, played |

Acquisition methods (species can have several): wild 414, partner Pokémon (daily) 24, Day Care egg 16, fossil 15, in-game trade 13, one-time static encounter 12, co-op wild pair 9, gift 8, legendary (once) 6, Birch's starter 3, roaming 2, gift egg 1; 431 species by evolution only.

## 2. Excluded (deferred) species: 125

No new encounters were added for these; existing content is unchanged. The 8 marked * were already obtainable (Hoenn legendaries) and stay so.

Articuno, Zapdos, Moltres, Mewtwo, Mew, Raikou, Entei, Suicune, Lugia, Ho-Oh, Celebi, Regirock*, Regice*, Registeel*, Latias*, Latios*, Kyogre*, Groudon*, Rayquaza*, Jirachi, Deoxys, Uxie, Mesprit, Azelf, Dialga, Palkia, Heatran, Regigigas, Giratina, Cresselia, Phione, Manaphy, Darkrai, Shaymin, Arceus, Victini, Cobalion, Terrakion, Virizion, Tornadus, Thundurus, Reshiram, Zekrom, Landorus, Kyurem, Keldeo, Meloetta, Genesect, Xerneas, Yveltal, Zygarde, Diancie, Hoopa, Volcanion, Type: Null, Silvally, Tapu Koko, Tapu Lele, Tapu Bulu, Tapu Fini, Cosmog, Cosmoem, Solgaleo, Lunala, Nihilego, Buzzwole, Pheromosa, Xurkitree, Celesteela, Kartana, Guzzlord, Necrozma, Magearna, Marshadow, Poipole, Naganadel, Stakataka, Blacephalon, Zeraora, Meltan, Melmetal, Zacian, Zamazenta, Eternatus, Kubfu, Urshifu, Zarude, Regieleki, Regidrago, Glastrier, Spectrier, Calyrex, Enamorus, Great Tusk, Scream Tail, Brute Bonnet, Flutter Mane, Slither Wing, Sandy Shocks, Iron Treads, Iron Bundle, Iron Hands, Iron Jugulis, Iron Moth, Iron Thorns, Wo-Chien, Chien-Pao, Ting-Lu, Chi-Yu, Roaring Moon, Iron Valiant, Koraidon, Miraidon, Walking Wake, Iron Leaves, Okidogi, Munkidori, Fezandipiti, Ogerpon, Gouging Fire, Raging Bolt, Iron Boulder, Iron Crown, Terapagos, Pecharunt.

Also not added: custom Megas (Mega Milotic); Galarian Darmanitan's Zen Mode and the Galarian legendary birds (legendary). The diplomas no longer require any of these (src/pokedex.c `IsDexEntryOptional`).

## 3. This build's additions

| Item | Before (0xC0170138) | After | Played |
| --- | --- | --- | --- |
| Batch 3 (230 candidates) | 3 obtainable (Meowth and Persian: a one-time Frontier trade; Bonsly: Day Care egg) | 230 of 230, all solo; plus Ursaluna | 9 catches (one per method and habitat), 57 evolutions and form changes (plus Runerigus, Archaludon and Oricorio's nectar in `roster_b3_services`), fossil, shop, co-op battles |
| Obtainable species | 680 | 908 (+228) | |
| Regional forms | 7 | 54 (+47) | Alolan Diglett, Galarian Stunfisk, Galarian Yamask, Hisuian Zorua caught; Alolan Raichu, Marowak, Exeggutor, Galarian Weezing, Mr. Mime, Slowbro, Slowking, Hisuian Typhlosion, Sliggoo, Goodra evolved |
| Hoenn Pokédex | 646 entries | 910 entries | LIST BOTTOM No 910 |
| AREA hints | 387 | 494 | Sinistcha's hint ("POLTCHAGEIST ARTISAN: MASTERPIECE TEACUP") on screen in `dex_probe` before the last Pokédex change; the final run shows Miltank's wild area |
| Wild placements | | 143 slots, 69 tables, 58 maps (grass, cave, surf, Old/Good/Super Rod, Rock Smash, Safari Zone incl. post-game areas, Altering Cave) | One catch per method/habitat |
| Evolution items for sale (Lilycove 3F) | Stones, apples, trade items | + Sweets, Soothe Bell, Oval Stone, Everstone, Up-Grade, Dubious Disc, Electirizer, Magmarizer, Protector, Razor Claw/Fang, pots, teacups, Sachet, Whipped Dream, Leek, Galarica Cuff/Wreath, Black Augurite, Peat Block, Metal Alloy, four nectars | Pink Nectar and Metal Alloy bought |

Earlier batches stay complete: BATCH1 114/114, BATCH2 114/114 (`roster.py`); no species became obtainable later than before, none lost its last wild source (comparison with 0xC0170138's manifest). Alolan Vulpix and Grimer stay obtainable after their quests: wild slots that do not depend on quest state (Shoal Cave low tide ice room and lower room; surfing on Route 110 and in Slateport).

## 4. Evolution and form mechanisms

| Mechanism | Examples | Source | Played |
| --- | --- | --- | --- |
| Regional branch by place | Alolan Raichu/Exeggutor/Marowak, Galarian Weezing/Mr. Mime, Hisuian starters, Lilligant, Braviary, Sliggoo, Avalugg | `GetEvolutionRegion` | Yes (Raichu both ways, Marowak both ways, Exeggutor, Weezing, Mr. Mime, Typhlosion, Sliggoo, Goodra) |
| Linking Cord with a partner species | Karrablast, Shelmet | data | Yes |
| Trade-item evolutions by use | Steelix, Electivire (and Scizor, Slowking, Magmortar…) | data | Steelix, Electivire, Gengar |
| Stats (Atk vs Def) | Tyrogue | stock | Yes, all three |
| Gender | Combee, Burmy, Espurr | stock | Yes |
| Cloak by battle place | Burmy → Wormadam | stock | Sandy, Trash (Plant is the default) |
| Time of day / held item by day | Happiny, Sneasler, Alolan Marowak, Obstagoon, Ursaluna | stock / changed (Ursaluna at night on Mt. Pyre) | Yes |
| Friendship | Chansey→Blissey, Munchlax, Buneary | stock | Yes |
| Known move | Yanma, Tangela, Lickitung, Mime Jr., Dunsparce, Hisuian Qwilfish | stock | Yes |
| Party member | Mantyke + Remoraid | stock | Yes |
| Weather (rain or fog) | Sliggoo → Goodra (both forms) | stock + fog places listed | Yes (Mt. Pyre summit fog) |
| Personality | Dudunsparce three-segment (1%) | stock | Yes (personality drawn by helper) |
| Move-use count | Primeape → Annihilape | stock; solo battles only | Yes (20th use) |
| Critical hits in one battle | Galarian Farfetch'd → Sirfetch'd | stock; solo battles only | Yes |
| Steps in the lead | Rellor → Rabsca | stock; counter not saved | Yes |
| Damage taken + special place | Galarian Yamask → Runerigus | carving in the Sandburied Ruins | Yes |
| Items: teacups, alloy, augurite, peat, cuff/wreath, sachet, whipped dream | | data | Yes |
| Fossil pairs | Dracozolt, Arctozolt, Dracovish, Arctovish | `src/coop_roster.c` | Dracozolt |
| Form change by item | Oricorio nectars | stock | Pink Nectar |
| Level only | Inkay (30, changed), Clodsire, Perrserker, Mr. Rime, Cursola, Malamar | data | Yes |

## 5. Battle abilities and forms (co-op, both screens)

| Ability | Result | How |
| --- | --- | --- |
| Schooling (Wishiwashi) | School Form on both screens (pixel-identical) | Played |
| Shields Down (Minior) | Core Form at half HP; both games' battle data | Played |
| Flower Gift (Cherrim, revamped Grass/Fairy) | Sunshine Form on both screens | Played |
| Dancer (Oricorio) | Copies the ally's Swords Dance, text on both screens | Played |
| Emergency Exit (Golisopod) | Flees; in a wild battle that ends the battle (engine rule) on both games together, link kept | Played |
| Receiver (Passimian) | Takes the fainted ally's ability (Drought), text on both screens | Played |

## 6. Pokédex

| Check | Result | How |
| --- | --- | --- |
| Unique regional mapping | 910 entries, no duplicates; every obtainable species included | Source (`roster.py --check` fails otherwise) |
| Name, types, description, sprites, footprint | No issue for any of the 908 | Source (read from the ROM) |
| Seen/caught registration | Catch, evolution, fossil, trade set caught | Played |
| Partner markers | Agree after UNLINK/LINK UP and after a power-on | Played (`roster_b3_play`, `_reload`) |
| Scrolling and reach | Hoenn No 910, National No 1025 | Played (`dex_probe`) |
| Search and sort | Type FAIRY, A to Z (type search lists caught Pokémon only, stock) | Played |
| Regional forms | Counted under their species; totals are National Dex species | Source |
| Completion | Diplomas skip legendary/mythical/UB/Paradox entries | Source |

## 7. ROM, tables, saves

- ROM 26,999,340 of 33,554,432 bytes (80.5%); +12,044 bytes since 0xC0170138.
- Wild tables keep their slot counts (each placement replaces a slot); the Hoenn order arrays are sized by `HOENN_DEX_COUNT`.
- Save layout unchanged; no new flags or variables; species ids unchanged; party stats of revamped species recalculated on CONTINUE as before.

## 8. Revamps

All earlier revamps are intact (328 species checked, 0 issues; `tools/coop_roster_audit/audit.py`). This build adds none. Completing the roster is not completing the balance plan: any proposals not yet in [COOP_REVAMPS_C0170137.md](COOP_REVAMPS_C0170137.md) or COOP.md's roster changes remain open.
