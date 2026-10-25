# Build 0xC017012F: roster, revamps, trainers, co-op reference

Stats are HP / Atk / Def / SpA / SpD / Spe.

- **Data checks** run on the built ROM and sources:
  - `tools/coop_roster_audit/audit.py` checks 146 species, with 0 issues.
  - `tools/coop_trainers/validate.py` checks 850 trainers, with 0 errors.
- **Gameplay results** are in
  [COOP_REGRESSION_C017012F.md](COOP_REGRESSION_C017012F.md).

## 1. New Pokémon families

Every family is catchable on each save independently. Every required evolution item is sold every day, so neither player can permanently miss one. Each family has its own Hoenn Pokédex page after Gholdengo.

| Family | Where (rate, level) | Evolution access |
| --- | --- | --- |
| Hatenna → Hattrem → Hatterene | Petalburg Woods (10%, 5–6) | Lv 32, Lv 42 |
| Wattrel → Kilowattrel | Route 104 (8%, 4–5); Route 110 (8%, 12–13) | Lv 25 |
| Riolu → Lucario | Granite Cave 1F, Dewford (15%, 9–10) | High friendship, level up outside night (06:00–19:59) |
| Shinx → Luxio → Luxray | Route 110 (30%, 12–13) | Lv 15, Lv 30. Luxray is Electric/Dark here (see revamps). |
| Cottonee → Whimsicott | Route 117 (14%, 13–14) | Sun Stone (Verdanturf Mart, Lilycove 3F) |
| Smoliv → Dolliv → Arboliva | Route 117 (14%, 13–14) | Lv 25, Lv 35 |
| Rockruff → Lycanroc (Midday, Midnight) | Route 114 (15%, 16–17) | Lv 25: Midday form by day, Midnight form at night (20:00–05:59) |
| Rockruff (Own Tempo) → Lycanroc (Dusk) | Route 114 (4%, 16) | Lv 25 at any hour (changed; see below) |
| Joltik → Galvantula | New Mauville (30%, 23–24) | Lv 36 |
| Rotom and five appliance forms | New Mauville (10%, 24–25) | Rotom Catalog from the scientist in New Mauville's entrance (one per save): Heat, Wash, Frost, Fan, Mow, or back to Rotom |
| Litwick → Lampent → Chandelure | Mt. Pyre 1F (10%, 25), 2F (20%, 25–26), 3F (20%, 26–27) | Lv 41, then Dusk Stone (Lilycove 3F) |
| Foongus → Amoonguss | Route 119 (15%, 25–27) | Lv 39 |
| Honedge → Doublade → Aegislash | Altering Cave (39%, 26–30; Route 103, by Surf) | Lv 35, then Dusk Stone |
| Petilil → Lilligant | Route 123 (15%, 26–28) | Sun Stone |
| Snom → Frosmoth | Shoal Cave low-tide ice room (14%, 28–30), inner room (5%, 32) | High friendship, level up at night |

Placements are in `tools/coop_roster_audit/place_new_families.py`. Each one replaces specific encounter slots and keeps the native species in their other slots.

### Choices

- **Dusk Lycanroc:** stock Own Tempo Rockruff needs evening, which this clock defines as 19:00–19:59. That window is too narrow for play, so it now evolves at level 25 at any hour. The note on the POOCHYENA on Route 114 explains the three forms.
- **Riolu and Snom:** they keep their stock friendship and time methods. Lucario needs any hour except night; Frosmoth needs night. Both rely on the cartridge clock, which mGBA keeps.
- **Excluded forms:**
  - Hisuian Lilligant needs the Hisui region and is left out.
  - The totem Kommo-o has no evolution method.
  - Both are listed by the audit.
- **Earlier additions:** the audit rechecked these too. Their gaps were fixed in 0xC017012E: Gimmighoul, Bisharp, Charcadet's armors, and the Linking Cord.

## 2. Revamps

Pre-evolutions keep their stock types and stats. Ability slots line up across each family, so an evolved Pokémon keeps the slot it had.

| Pokémon | Before | After | Typing | Notable moves added |
| --- | --- | --- | --- | --- |
| Mightyena | 70/90/70/60/60/70, Intimidate / Quick Feet / Moxie | 80/110/75/50/65/90, Intimidate / Strong Jaw / Moxie | Dark | Throat Chop 32, Psychic Fangs 40, Play Rough 46, Parting Shot 56; keeps Snarl, Taunt, Yawn |
| Masquerain | 70/60/62/100/82/80, Intimidate / Unnerve | 75/40/72/105/92/90, same | Bug/Flying → Bug/Water (like Surskit) | Sticky Web 1, Bubble Beam 24, Scald 29, Tailwind 34, Hydro Pump 48; keeps Stun Spore, Quiver Dance |
| Banette | 64/115/65/83/63/65, Insomnia / Frisk / Cursed Body | 74/125/75/60/73/80, Insomnia / Frisk / Prankster | Ghost | Taunt 24, Shadow Claw 28, Poltergeist 38, Play Rough 44, Swords Dance 50 |
| Cacturne | 70/115/60/115/60/55, Sand Veil / Water Absorb | 80/115/75/110/75/55, Water Absorb / Sand Veil | Grass/Dark | Drain Punch 28, Seed Bomb 32, Dark Pulse 41, Leaf Storm 47 |
| Flygon | 80/100/80/80/80/100, Levitate | 80/100/85/100/80/100, Levitate | Ground/Dragon | Dragon Pulse 36, Scorching Sands 42, Draco Meteor 56 |
| Swalot | 100/73/83/73/83/55, Liquid Ooze / Sticky Hold / Gluttony | 115/70/95/73/100/45, same | Poison | Clear Smog 22, Knock Off 33, Pain Split 45 |
| Delibird | 45/55/45/65/45/75, Vital Spirit / Hustle / Insomnia | 65/75/55/75/65/110, Vital Spirit / Hustle / Snow Warning | Ice/Flying | New learnset: Fake Out, Ice Shard 8, Icy Wind 13, Helping Hand 18, Tailwind 30, Freeze-Dry 35, Aurora Veil 40, Brave Bird 45, Spikes 50 |
| Beautifly | 60/70/50/100/50/65, Swarm / Rivalry | 65/50/60/115/65/100, Swarm / Tinted Lens / Rivalry | Bug/Flying | Psychic 37, Hurricane 44 (Protect removed) |
| Dustox | 60/50/70/50/90/65, Shield Dust / Compound Eyes | 80/40/90/60/105/55, Shield Dust / Compound Eyes / Healer | Bug/Poison | Rage Powder 24, Toxic Spikes 28, Sludge Bomb 40, Safeguard 45 (Quiver Dance removed) |
| Armaldo | 75/125/100/70/80/45, Battle Armor / Swift Swim | 85/130/110/45/90/50, same | Rock/Bug | Rapid Spin 1, Knock Off 30, Stone Edge 40, Swords Dance 52 |
| Luxray | 80/120/79/95/79/70, Rivalry / Intimidate / Guts | 80/125/79/75/79/88, same | Electric → Electric/Dark | Fire/Ice/Thunder Fang 1, Crunch 42, Wild Charge 50, Psychic Fangs 58, Supercell Slam 68 (Swagger removed) |

Why these numbers:
- Each total rises by 3 (Luxray) to 115 (Delibird, which was the weakest at 330).
- Each one moves stats toward its role and away from points it never uses: Masquerain's and Beautifly's Attack, Banette's and Armaldo's Special Attack.
- Only Banette (Prankster, the ability of its stock Mega form) and Delibird (Snow Warning, for its Aurora Veil) gain strong new abilities, and both are hidden.
- No two of them share a support kit:
  - Swalot supports with Clear Smog, Pain Split and Yawn.
  - Dustox redirects with Rage Powder and cures its partner with Healer.
  - Delibird sets Tailwind and Aurora Veil.
  - Masquerain sets Sticky Web and Tailwind behind Intimidate.

## 3. Trainers and bosses

### Ordinary trainers (728, rebuilt by `tools/coop_trainers/overhaul.py`)

The scope covers route, cave, building, optional-area and gym trainers, Aqua and Magma grunts, and every rematch tier. The full before/after list is in [COOP_TRAINERS_F.tsv](COOP_TRAINERS_F.tsv).

| Rule | Result |
| --- | --- |
| Levels | +0 to +5 by stage, never above the next gym ace − 2. Results: +0 for 74, +1 for 65, +2 for 206, +4 for 256, +5 for 127. Gym trainers stay below their own leader (validated). |
| Party sizes | Before: 277 ones, 297 twos, 115 threes, 18 fours, 2 fives, 19 sixes. After: 60, 460, 150, 37, 2, 19. A lone Pokémon gets a themed partner from stage 2 on. Double-battle pairs keep their count. |
| Expanded roster | 443 teams include an added species. Each is only used from the stage where players can catch it, and it fits the trainer's class theme (gym trainers use their gym's type). |
| Evolutions | A Pokémon at least 4 levels past its level evolution (2 levels from level 30 on) is evolved. Stone, friendship and trade evolutions happen from level 32 (36 for trade). Place-, move- and party-specific evolutions are left to players. |
| Held items | 222 teams give their ace an item: about one in four from stage 2 gets a berry, about one in six from stage 5 gets a type-boosting item. |
| AI | Check Bad Move / Try To Faint; plus Check Viability from stage 2 and HP Aware from stage 5. Never omniscient. |
| Rematches | Each rematch group keeps the species it gained across every tier, adding it where the team has room. |
| Kept as is | Calvin is kept as the one-Pokémon co-op test case. The Route 102 pair Allen and Rick keep their teams for the pair-ask feature. |

### Rivals

**Linked (co-op):** both twins fight both players. These teams were unchanged from 0xC017012E except for the AI below.

**Solo:** a game that is not linked fights its own twin in a single battle:
- Soren uses the stock BRENDAN slots and Mira the stock MAY slots.
- Both rival assignments are tested.

| Fight | Soren solo (case Treecko / Torchic / Mudkip) | Mira solo (case Treecko / Torchic / Mudkip) |
| --- | --- | --- |
| Route 103 | Torchic / Mudkip / Treecko, Lv 5 | Houndour / Wingull / Shroomish, Lv 5 |
| Rustboro | Numel 14 + Combusken 16 / Nacli 14 + Marshtomp 16 / Applin 14 + Grovyle 16 | Slugma 14 + Houndour 16 / Lotad 14 + Pelipper 16 / Lotad 14 + Shroomish 16 |
| Route 110 | the co-op three (Lv 20–22), ace last | the co-op three, ace last |
| Route 119 | Lucario 34 + the co-op three (Blaziken / Swampert / Sceptile 36 last) | Hattrem 34 + the co-op three (Houndoom / Pelipper / Breloom 36 last) |
| Lilycove | Lucario 46 + the co-op three (ace 48) | Hatterene 46 + the co-op three (ace 48) |

### Villains, Wally, Elite Four, champions (`tools/coop_trainers/bosses.py`)

The full list is in [COOP_BOSSES_F.tsv](COOP_BOSSES_F.tsv).

| Fight | Before | After |
| --- | --- | --- |
| Tabitha, Mt. Chimney | Numel 18, Poochyena 20, Numel 22, Zubat 22 | Numel 26, Charcadet 26, Mightyena 27, Golbat 28 |
| Maxie, Mt. Chimney | Mightyena 24, Zubat 24, Camerupt 25 | Mightyena 29, Golbat 29, Krokorok 30, Camerupt 31 |
| Shelly, Weather Institute | Carvanha 28, Mightyena 28 | Mightyena 34, Barraskewda 34, Sharpedo 36 |
| Tabitha, Magma Hideout | Numel 26, Mightyena 28, Zubat 30, Camerupt 33 | Camerupt 40, Mightyena 40, Armarouge 41, Crobat 42 |
| Maxie, Magma Hideout | Mightyena 37, Crobat 38, Camerupt 39 | Mightyena 44, Crobat 44, Revavroom 45, Camerupt 46 |
| Matt, Aqua Hideout | Mightyena 34, Golbat 34 | Mightyena 41, Crobat 41, Crabominable 43 |
| Maxie and Tabitha, Space Center | Mightyena 42, Crobat 43, Camerupt 44 / Camerupt 36, Mightyena 38, Golbat 40 | Mightyena 47, Crobat 47, Krookodile 48, Camerupt 49 / Mightyena 45, Ceruledge 46, Camerupt 47 |
| Shelly, Seafloor Cavern | Sharpedo 37, Mightyena 37 | Mightyena 48, Basculegion 49, Sharpedo 50 |
| Archie | Mightyena 41, Crobat 41, Sharpedo 43 | Mightyena 52, Crobat 52, Dondozo 53, Sharpedo 54 |
| Wally, Mauville | Ralts 16 | Ralts 18 |
| Wally, Victory Road (tiers +3 each) | Altaria, Delcatty, Roselia, Magneton, Gardevoir 41–45 | Altaria 58, Delcatty 57, Roserade 58, Magneton 57, Whimsicott 58, Gardevoir 60 |
| Sidney + Noir | Shiftry; Crawdaunt | Kingambit 65; Grimmsnarl 64 (Prankster screens) |
| Phoebe + Wraith | Wraith's Banette | Chandelure 64 (Heat Wave) |
| Glacia + Frost | two Sealeo; Glalie lead | Frosmoth 64 and ace Walrein 66; Abomasnow 64 (Snow Warning) with Slush Rush Cetitan 65, Glalie 66 |
| Drake + Wyrm | Shelgon, Altaria (Wyrm also has Altaria) | Dragapult 64, Kommo-o 65 |
| Steven + Wallace (champions) | Skarmory; Wailord | Aegislash 64; Basculegion 64 (Swift Swim) |

Every villain level sits between the trainers around that fight and the next gym ace.

The first-clear Gym teams are unchanged from 0xC017012E (see [COOP_GYM_AND_ROSTER_AUDIT.md](COOP_GYM_AND_ROSTER_AUDIT.md)).

**Boss AI** (Gym leaders and partners, Elite Four, champions, admins and leaders, both twins' teams, Wally) is the 0xC017012E set plus PP Stall Prevention and Weigh Ability Prediction, plus Ace Pokémon where the ace is last in the party. It never uses Omniscient or any move, item or ability omniscience, so it never reads a player's hidden choices.

## 4. Co-op and solo coverage

| Encounter | Linked, partner on the map | Linked, partner elsewhere | Not linked |
| --- | --- | --- | --- |
| Ordinary trainer, Beacon on | Invitation; co-op battle on YES | Solo, with "partner is on another map" | Stock solo |
| Ordinary trainer, Beacon off | Solo | Solo | Stock solo |
| Double-battle couples and twins (36) | Solo, with "a one-player encounter" | Solo | Stock solo |
| Battle Frontier, Safari, Trainer Hill, Pyramid | Solo (facility rules) | Solo | Stock |
| Wild, ordinary land or Surf, Beacon on | Invitation; co-op double on YES | Solo | Stock |
| Fishing, Rock Smash, roamers, scripted wild | Solo (one-player encounter) | Solo | Stock |
| Gyms, Elite Four, champion | Co-op double | Waits ("come back together, or UNLINK") | Double against both trainers |
| Rival twins | Both twins against both players | Scene resets and waits | Single battle against your own twin |
| Aqua and Magma bosses, Wally | Co-op | Waits | Stock single battle |
| Maxie and Tabitha, Space Center | Co-op double | Waits | Stock multi battle with Steven |
| Groudon and Kyogre | One co-op double against both | Waits | Each cave's legend alone, Lv 70 |
| Rayquaza | Co-op | Waits | Stock single battle, Lv 70 |
| Birch's rescue | Both players, two POOCHYENA | Waits | One POOCHYENA |

**Exceptions:**
- **Couples and twins stay solo.** They already battle as a pair, and adding a second human would need two more opposing Pokémon that their stock teams don't have.
- **Facilities keep their own rules.**
- **Fishing, Rock Smash, roamers and scripted encounters keep their one-player paths.**
- **No fight requires a second player.** A save that is not linked can finish the game alone.
