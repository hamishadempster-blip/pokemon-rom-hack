# Gym and added-Pokémon audit (build 0xC017012E)

## How this was checked

- **ROM data audit (no gameplay):** `tools/coop_roster_audit/audit.py` reads the built ROM for every species in the "New Pokémon" table in [COOP.md](COOP.md) plus their evolution lines (107 entries, including regional forms): enabled entry, name, stats, types, abilities, front/back graphics (not the "?" placeholder), icon, cry, Pokédex text, learnset, evolutions. Obtainability comes from the wild tables, in-game trades, and where each evolution item is placed, hidden, sold, or given. Result: 0 issues.
- **Two-core gameplay, with fixtures:** `gym_teams.txt` fights Roxanne+Heath, Norman+Hana, and Juan+Wallace as co-op doubles. Fixtures: both players are warped to the leader, given level-70 leads, and Norman's gym state is set. It checks that each fight starts, the win sets the leader flag and badge on both saves, the prize is equal, and parties come back intact. It does not test difficulty.
- Gyms 2, 3, 4, 6, and 7 were not fought in the harness. Their teams were checked against the source only.

## Intended additions and how to get them

Only species in the COOP.md table and their lines count as added. Anything else the engine supports is out of scope and is not claimed to be obtainable.

| Line | How to get it |
| --- | --- |
| Lechonk → Oinkologne (both genders, Lv 18) | Route 102; in-game trade |
| Tandemaus → Maushold (Lv 25) | Route 102 |
| Applin → Flapple / Appletun / Dipplin → Hydrapple | Route 104; Tart Apple (Route 104 ball), Sweet Apple (Route 104 gift), Syrupy Apple (Route 123 ball), all also in Lilycove Dept. Store 3F. Hydrapple needs Dipplin to know Dragon Cheer (its level-1 move, from the Move Relearner). |
| Rookidee → Corvisquire → Corviknight | Route 104 |
| Tarountula → Spidops; Shroodle → Grafaiai | Petalburg Woods (Tarountula also by trade) |
| Nacli → Naclstack → Garganacl; Glimmet → Glimmora | Granite Cave 1F |
| Crabrawler → Crabominable | Route 106 surf; Ice Stone |
| Veluza | Route 106 surf |
| Basculegion (+ Basculin by breeding) | Route 107 surf; Day Care |
| Arrokuda → Barraskewda | Route 108 surf |
| Dondozo | Route 109 surf |
| Pawmi → Pawmo → Pawmot | Route 110; trade; Pawmot by level-up after 1,000 steps |
| Sandile line; Helioptile → Heliolisk; Capsakid → Scovillain | Route 111 desert; Sun Stone, Fire Stone |
| Bramblin → Brambleghast | Route 113; trade; level-up after 1,000 steps |
| Flamigo | Route 115 |
| Wooloo → Dubwool; Nickit → Thievul | Route 116 |
| Tadbulb → Bellibolt | New Mauville; Thunderstone |
| Charcadet → Armarouge / Ceruledge | Jagged Pass; trade; Auspicious / Malicious Armor sold at Lilycove 3F (added in this build) |
| Cufant → Copperajah | Jagged Pass |
| Indeedee (M and F) | Route 118 |
| Toedscool → Toedscruel | Route 119 |
| Politoed (+ Poliwag line by breeding) | Route 119/120 surf. Poliwag and Poliwhirl appear wild only in Kanto tables, so in Hoenn they come from Day Care breeding. Poliwrath needs a Water Stone. |
| Impidimp → Morgrem → Grimmsnarl | Route 121 |
| Finizen → Palafin | Route 124 surf; trade |
| Wiglett → Wugtrio | Underwater Route 126 |
| Tatsugiri | Route 128 surf |
| Greavard → Houndstone (Lv 30, night) | Mt. Pyre 1F |
| Dreepy line; Mimikyu | Mt. Pyre 1F |
| Noibat → Noivern | Meteor Falls 1F |
| Jangmo-o line | Meteor Falls B1F; trade |
| Roggenrola → Boldore → Gigalith | Gigalith on the Desert Underpass; Roggenrola by breeding. Boldore needs a trade or a Linking Cord (sold at Lilycove 3F, added in this build). |
| Snover → Abomasnow; Cetoddle → Cetitan | Shoal Cave; Ice Stone |
| Fidough → Dachsbun; Cyclizar | Safari Zone (single-player Safari rules) |
| Varoom → Revavroom | Magma Hideout |
| Tinkatink line; Frigibax line; Gimmighoul → Gholdengo | Victory Road 1F |
| Pawniard → Bisharp → Kingambit | Victory Road B1F |

### Gaps fixed in this build

- Gimmighoul needed 999 Gimmighoul Coins, which don't exist in this game. It now evolves at Lv 50.
- Bisharp needed to beat Bisharp holding a Leader's Crest, which doesn't exist in this game. It now evolves at Lv 64.
- The Auspicious Armor, Malicious Armor, and Linking Cord had no source. All three are now sold by the Lilycove evolution clerk.

### Remaining gaps and notes

- Pawmot and Brambleghast need 1,000 steps, and Houndstone needs night (RTC). These are stock methods and were left as they are.
- Kommo-o's audit row also lists the engine's Totem form. That form can't be reached and isn't an addition.
- None of the added species were caught or evolved in gameplay tests for this batch. Their battle behaviour was exercised only where one appeared on a gym or rival team in a harness fight (Nacli, Maushold, Veluza, Dondozo).

## Gym first-clear teams (final)

Every gym is a co-op double: the leader and a partner bring three Pokémon each. By the project's existing design, a Gym can't be fought alone, so there is no separate solo team. Each side's three Pokémon stand on their own, so either player's first three can face one of them. Ace levels follow the level cap: 16, 22, 28, 34, 40, 46, 52, 58. Rewards, badges, and the shared victory are unchanged, and both saves get the leader flag, the badge, and the stock prize. The Lv 68–70 rematches are a separate bracket and weren't changed.

Defaults chosen: one added Pokémon per leader, plus one per partner where it fits the type. Doubles support grows gradually:

- Gyms 1–2: Protect and Fake Out only.
- Gym 3: Volt Absorb / Discharge.
- Gym 4: Drought plus Chlorophyll.
- Gym 5: Follow Me / Friend Guard.
- Gym 6: Tailwind / Wide Guard.
- Gym 7: Psychic Surge, Trick Room, Follow Me.
- Gym 8: rain.

### Stone Badge (Rustboro)

- **Roxanne** (`TRAINER_ROXANNE_1`): Nosepass Lv15 (Sturdy) @ Oran Berry: Thunder Wave, Rock Tomb, Tackle, Harden; Nacli Lv14 (Purifying Salt): Smack Down, Mud Shot, Harden, Protect; Aron Lv16: Metal Claw, Rock Tomb, Tackle, Harden
- **Heath** (`TRAINER_HEATH_1`): Anorith Lv15: Water Gun, Scratch, Harden, Rock Tomb; Lileep Lv14: Acid, Constrict, Mega Drain, Rock Tomb; Onix Lv16: Rock Tomb, Tackle, Harden, Protect

### Knuckle Badge (Dewford)

- **Brawly** (`TRAINER_BRAWLY_1`): Crabrawler Lv20 (Hyper Cutter): Rock Smash, Bubble Beam, Leer, Protect; Machoke Lv21: Karate Chop, Seismic Toss, Rock Tomb, Bulk Up; Hariyama Lv22 (Guts) @ Flame Orb: Fake Out, Arm Thrust, Vital Throw, Bulk Up
- **Reed** (`TRAINER_REED_1`): Meditite Lv20 (Telepathy): Rock Tomb, Confusion, Detect, Meditate; Medicham Lv21: Fake Out, Brick Break, Confusion, Detect; Pawmo Lv21 (Volt Absorb): Nuzzle, Arm Thrust, Bite, Quick Attack

### Dynamo Badge (Mauville)

- **Wattson** (`TRAINER_WATTSON_1`): Magneton Lv28 (Magnet Pull): Discharge, Thunder Wave, Shock Wave, Sonic Boom; Voltorb Lv26: Spark, Rollout, Light Screen, Shock Wave; Tadbulb Lv27 (Static): Spark, Mud Shot, Charge, Water Gun
- **Joule** (`TRAINER_JOULE_1`): Lanturn Lv28 (Volt Absorb): Spark, Water Pulse, Confuse Ray, Thunder Wave; Manectric Lv27 (Lightning Rod): Shock Wave, Quick Attack, Howl, Thunder Wave; Minun Lv26 (Volt Absorb): Shock Wave, Helping Hand, Thunder Wave, Quick Attack

### Heat Badge (Lavaridge)

- **Flannery** (`TRAINER_FLANNERY_1`): Torkoal Lv34 (Drought) @ White Herb: Eruption, Body Slam, Protect, Attract; Camerupt Lv32: Overheat, Rock Slide, Tackle, Protect; Armarouge Lv33 (Flash Fire) @ Charcoal: Incinerate, Psybeam, Will O Wisp, Protect
- **Cole** (`TRAINER_COLE_1`): Houndoom Lv34 (Flash Fire): Flamethrower, Bite, Protect, Howl; Scovillain Lv33 (Chlorophyll): Flamethrower, Bullet Seed, Bite, Protect; Tropius Lv32 (Solar Power): Air Slash, Magical Leaf, Growth, Protect

### Balance Badge (Petalburg)

- **Norman** (`TRAINER_NORMAN_1`): Slaking Lv40 (Truant) @ Sitrus Berry: Facade, Yawn, Counter, Feint Attack; Maushold Lv38 (Friend Guard): Follow Me, Helping Hand, Super Fang, Encore; Exploud Lv39: Shadow Ball, Stomp, Howl, Supersonic
- **Hana** (`TRAINER_HANA_1`): Kecleon Lv40: Helping Hand, Skill Swap, Shadow Ball, Thief; Spinda Lv38: Protect, Facade, Psybeam, Encore; Zangoose Lv39 @ Toxic Orb: Slash, Swords Dance, Crush Claw, Facade

### Feather Badge (Fortree)

- **Winona** (`TRAINER_WINONA_1`): Altaria Lv46 @ Sitrus Berry: Dragon Pulse, Dragon Dance, Aerial Ace, Dragon Breath; Corviknight Lv44 (Mirror Armor) @ Leftovers: Drill Peck, Iron Head, Tailwind, Taunt; Pelipper Lv45 (Drizzle): Water Pulse, Aerial Ace, Protect, Hurricane
- **Gale** (`TRAINER_GALE_1`): Skarmory Lv46: Tailwind, Steel Wing, Aerial Ace, Spikes; Flamigo Lv44 (Scrappy): Wide Guard, Feint, Low Kick, Air Slash; Gyarados Lv45: Ice Fang, Waterfall, Dragon Dance, Bite

### Mind Badge (Mossdeep)

- **Tate** (`TRAINER_TATE_AND_LIZA_1`): Lunatone Lv52 @ Sitrus Berry: Trick Room, Psychic, Hypnosis, Calm Mind; Indeedee-F Lv50 (Psychic Surge) @ Sitrus Berry: Follow Me, Psychic, Helping Hand, Protect; Solrock Lv51: Flamethrower, Psychic, Rock Slide, Sunny Day
- **Liza** (`TRAINER_LIZA_1`): Xatu Lv50: Psychic, Trick Room, Confuse Ray, Calm Mind; Gardevoir Lv51: Psychic, Helping Hand, Thunderbolt, Magical Leaf; Slowking Lv52: Psychic, Surf, Ice Beam, Calm Mind

### Rain Badge (Sootopolis)

- **Juan** (`TRAINER_JUAN_1`): Veluza Lv56 (Sharpness) @ Mystic Water: Aqua Cutter, Psycho Cut, Night Slash, Protect; Whiscash Lv57 (Hydration): Liquidation, Surf, Amnesia, Rock Tomb; Kingdra Lv58 (Swift Swim): Surf, Ice Beam, Dragon Pulse, Protect
- **Wallace** (`TRAINER_WALLACE_GYM`): Pelipper Lv56 (Drizzle): Surf, Aerial Ace, Protect, Rain Dance; Milotic Lv57: Surf, Ice Beam, Recover, Attract; Dondozo Lv57 (Unaware) @ Leftovers: Order Up, Heavy Slam, Body Slam, Protect

## Rival twin teams (co-op rival fights)

Soren always leads with the starter line of his counter-pick, and Mira with the companion line. Neither carries the other's signature Pokémon, and the added Pokémon fill the remaining slots. The row is picked by the case's type.

| Fight | Soren (Fire / Water / Grass) | Mira (Fire / Water / Grass) |
| --- | --- | --- |
| Route 103 | Torchic 7 / Mudkip 7 / Treecko 7 | Houndour 7 / Wingull 7 / Shroomish 7 |
| Rustboro | Combusken 16, Numel 14 / Marshtomp 16, Nacli 14 / Grovyle 16, Applin 14 | Houndour 16, Slugma 14 / Pelipper 16, Lotad 14 / Shroomish 16, Lotad 14 |
| Route 110 | Combusken 22, Numel 21, Corvisquire 20 / Marshtomp 22, Nacli 21, Crabrawler 20 / Grovyle 22, Flapple 21, Pawmi 20 | Houndoom 22, Slugma 21, Tarountula 20 / Pelipper 22, Lombre 21, Tandemaus 20 / Breloom 22, Shroodle 21, Lotad 20 |
| Route 119 | Blaziken 36, Camerupt 35, Corvisquire 34 / Swampert 36, Naclstack 35, Lanturn 34 / Sceptile 36, Flapple 35, Pawmo 34 | Houndoom 36, Ninetales 35, Spidops 34 / Pelipper 36, Lanturn 35, Maushold 34 / Breloom 36, Grafaiai 35, Shiftry 34 |
| Lilycove | Blaziken 48, Camerupt 47, Corviknight 46 / Swampert 48, Garganacl 47, Lanturn 46 / Sceptile 48, Flapple 47, Pawmot 46 | Houndoom 48, Ninetales 47, Magcargo 46 / Pelipper 48, Gyarados 47, Sharpedo 46 / Breloom 48, Tropius 47, Shiftry 46 |
