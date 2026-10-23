# Emerald Co-op

Two people, two saves, one Hoenn. Each game keeps its own party, bag, money, and catches. The host only hosts the link. Gyms, the rival pair, the Elite Four, the champion pair, and Groudon with Kyogre cannot be cleared alone.

This is a non-commercial fan patch. It does not include a ROM.

Based off RHH's pokeemerald-expansion 1.17.1
https://github.com/rh-hideout/pokeemerald-expansion/
Pinned commit `bb1093a7c72d3961d9a37d8e02ad2abc2c866682`.

Build id baked into the link handshake: `0xC017011A`.

## Clean ROM

`Pokemon - Emerald Version (USA, Europe).gba`

- Size: 16,777,216 bytes
- SHA-256: `a9dec84dfe7f62ab2220bafaef7479da0929d066ece16a6885f6226db19085af`
- SHA-1: `f3ae088181bf583e55daf962a92bb46f4f1d07b7`
- Title `POKEMON EMER`, code `BPEE`, revision 0, 128 KB flash

Keep that file on your PC, in `rom/` if you want `tools/coop_make_patch.sh` to find it. Expansion 1.17.1 does not need the ROM to compile. The ROM is only the base for the patch. It is gitignored. Do not commit it, and do not commit `pokeemerald.gba`.

## Build on Windows

Use WSL2 and the steps in [docs/install/windows/WSL.md](install/windows/WSL.md). Debian's `gcc-arm-none-eabi` 13.2 is the compiler this tree was checked with. From the repo root:

```sh
make -j$(nproc)
```

The ROM image is `pokeemerald.gba` in the repo root. Give each player their own copy and their own `.sav`. Two windows must not share one save file.

`tools/coop_protocol_test.c` checks the link frame on the host compiler, with no GBA tools:

```sh
gcc -DCOOP_HOST_TEST -iquote include tools/coop_protocol_test.c -o /tmp/coop_test && /tmp/coop_test
```

## Emulators

| Where | Emulator | What it is for |
| --- | --- | --- |
| One PC | mGBA 0.10.5 | Local cable. File, New Multiplayer Window. Two games, two saves. |
| Two PCs | RetroArch 1.22.2, gpSP core v1.1.0 | Internet. Netplay mode **Link Cable - Pokémon Gen3**. |

mGBA's internet link is not a release. Use RetroArch for that.

RetroArch netplay that locksteps one screen is a different feature. This hack needs the Gen 3 link-cable mode, which carries the Pokémon session (`MPK1`, 8 data words plus a checksum) between two independent games. Commands ride that session. `LINKCMD_COOP` (`0xC0A1`) is the normal command. If a core drops unknown commands, rebuild with `COOP_SEND_AS_HELD_KEYS` set to 1 in `include/coop.h`. Both layouts are accepted.

No custom server. If the public RetroArch relay is down, try UPnP, then a direct IP, then Tailscale. A room-code server is not part of this build.

## How a session works

1. Both games are the same build. Open the start menu, choose **CO-OP**, and say yes to the link. The other game does the same. mGBA has to already be in a multiplayer window. RetroArch has to already be in a Gen 3 link lobby.
2. The first exchange compares the build id and the level cap. Either mismatch aborts. Change the cap from CO-OP before linking. The cap is on unless you turn it off.
3. You can walk different maps. The partner sprite appears only on the same map, including through doors. A door is a new map, so the sprite drops and comes back when you return. The CO-OP menu names their map and says whether the Twin Beacon is on.
4. Birch gives each of you a Twin Beacon in the lab, before the starter case. It stays in the Key Items pocket. Using it asks the other player. Ordinary trainer battles become co-op only after both agree, and both games switch on that agreed exchange. Either person can ask to turn it off. A request made during a battle or trade waits until that fight or trade is over. Disconnecting turns the Beacon off. The item stays in the bag. Gyms, Soren and Mira, the Elite Four, the champion, and the legendary fights ignore the Beacon and still need both of you on the map.
5. Talk to the sprite to see where they are. From the party screen, while linked, **TRADE** offers one Pokémon. Both sides have to agree. Pulling the cable before both sides commit leaves both parties alone. You cannot trade your last conscious Pokémon.
6. Disconnect from the CO-OP menu, or from a dropped cable. Parties are not copied across. Reconnect exchanges badges earned together, the rival stage, and who owns Groudon, Kyogre, and Rayquaza. The Twin Beacon is off again until you both accept.

## Story rules

- Birch on Route 101 waits until both of you are on that map and linked. The lab will not open the cases until both of you are in the lab.
- One case, then you split the two Pokémon. Cinder is Torchic and Houndour. Tide is Mudkip and Wingull. Thorn is Treecko and Shroomish. If you both claim the same one, the host keeps the claim and the guest gets the other.
- Soren and Mira take the case that beats yours. They show up in the old Brendan and May fights. Both of you have to be there.
- Gyms are two leaders, three Pokémon each. You cannot start one alone. Ace levels are 16, 22, 28, 34, 40, 46, 52, 58. Rematches are a second pair around level 70.
- With the Twin Beacon off, ordinary trainers stay single even on the same map. With it on, most trainers lean doubles while you are together on their map. A trainer whose id is divisible by 4 stays a single. Magma and Aqua grunts are a pair. Leaders, the Elite Four, the champion, and rivals (Wally included) always ask for both of you, whether or not the Beacon is on.
- The Elite Four and the champion are the same 3+3 shape. The champion pair is Steven and Wallace. The guest does not watch the Hall of Fame cutscene. Both saves still set the champion flag.
- Terra Cave and Marine Cave both start Groudon and Kyogre together. Their weather overwrites itself. Sky Pillar is Rayquaza. One catch. The person whose ball lands keeps it. The other person can get it only by the party trade. A KO still clears the story flags.

### Party in co-op battles

- A co-op battle fields party slots 1–3 from each player, through the stock link-multi exchange (`COOP_BATTLE_SLOTS` in `include/coop_protocol.h`). Solo battles use all six. Enemy trainers keep their full teams.
- Slots 4–6 leave the party for the length of the battle. They cannot be switched in, they do not keep a fight going, and they never count toward a whiteout.
- When the battle finishes, slots 1–3 keep what happened: HP, PP, status, held items. Slots 4–6 come back exactly as they were. A Pokémon that joins during the battle goes into the next free party slot, or the PC if the party is full.
- A dropped cable mid-battle puts all six back the way they were when the battle started. No badge, no prize.
- If your first three are all fainted or eggs, a gym, rival, Elite Four, champion, or legendary fight refuses to start and tells you to heal or move one up. If your partner's first three are down, it refuses with a message about your partner. An ordinary trainer with the Twin Beacon on stays a solo fight for whoever's first three are down.
- When one player's three faint and the other player is still standing, the battle carries on. It ends in a loss only when both players' first threes are down.
- Link battles give no experience in this build, the same as stock link battles. Experience is unchanged by a co-op fight.
- A co-op battle hands the cable back open, so the session stays linked after the fight.

### Wild Pokémon together (Route 102)

This is the first route with shared wild battles. Other routes keep stock wild battles.

- A grass encounter on Route 102 becomes a co-op wild double when the Twin Beacon is on, both of you are on Route 102, and both of you have a battle-ready first three. The game that rolls the encounter picks two wild Pokémon and sends them to the other game. A partner who is in a menu, a script, or a trade answers no, and the wild battle starts as a stock solo fight for the player who stepped in the grass.
- The bag takes Poké Balls only. Choose a ball, then press left or right to pick which wild Pokémon it is for, and A to throw. B puts the ball back. If only one wild Pokémon is left, the ball goes to that one.
- The battle shows who threw. A caught Pokémon leaves the field and the fight goes on against the other one.
- If both of you throw at the same Pokémon on the same turn, the first ball to land keeps it. The second ball misses and is used up. No copy is made.
- Throwing at different Pokémon on the same turn can catch both, one each.
- A catch belongs to the person who threw the ball. It goes into their party, or their PC if the party is full. Only their Pokédex marks it caught, only their save counts the capture, and only they are asked for a nickname. That happens after the battle, back on the field.
- A full party and a full PC refuse the ball before it is thrown, and the fight continues.
- Pulling the cable during the shakes, or at any point before the battle ends, gives nobody the Pokémon.
- Running away needs both of you to choose RUN on the same turn. One player alone gets a message, and that turn is spent. A player whose first three are all down does not need to choose RUN. Roar, Whirlwind, and Red Card do not end the battle early. Your own Teleport switches out. A wild Pokémon can still flee with Teleport.
- No experience, the same as the other co-op battles.

### Rick and Allen (Route 102)

Rick (`TRAINER_RICK`, 615) and Allen (`TRAINER_ALLEN`, 333) on Route 102 are the optional pair. Rick is the left trainer and Allen is the right.

- Alone, or with the Twin Beacon off, each one is his normal solo fight. Beating one sets only that trainer's flag on your save.
- Walk into either one while linked, with the Beacon on and your partner on Route 102. If neither save has beaten either of them, you are asked whether to call your partner. Your partner gets a yes or no prompt. If both of you say yes, it is a 2-on-2 against Rick and Allen.
- A no, a busy partner, a partner who waits too long, or a partner whose save has beaten Rick or Allen turns it into a solo fight against the trainer you walked into.
- A joint win sets both trainer flags on both saves. A solo win sets only the flag of the trainer you fought. A loss or a pulled cable sets no flag.
- Flags never copy across the link. Each save checks its own two flags before it says yes.

Megas, Z-Moves, Dynamax, and Terastallization are off, including Primal Reversion.

A few Hoenn Pokémon were retuned for doubles. See [Roster changes](#roster-changes). When both players stand on the same tile, a wild double that rolls Plusle, Minun, Volbeat, Illumise, Lunatone, Solrock, Seviper, Zangoose, or either Indeedee brings that Pokémon's pair instead of a random second species. Gym leaders, their partners, the Elite Four, and the champion also switch more carefully and look for a two-hit knockout. Their ace levels are unchanged.

## Roster changes

Stats below are HP / Attack / Defense / Sp. Atk / Sp. Def / Speed. Abilities are listed as ability 1, ability 2, hidden.

| Pokémon | Before | After | What changed |
| --- | --- | --- | --- |
| Plusle | 60/50/40/85/75/95, Plus / Lightning Rod | 60/50/40/100/75/105, Plus / Lightning Rod | Special attack and speed. Already learns Helping Hand and Encore. |
| Minun | 60/40/50/75/85/95, Minus / Volt Absorb | 60/40/50/75/100/105, Minus / Volt Absorb | Special defense and speed. Same cheering moves as Plusle. |
| Castform | 70 across, Forecast | 80/70/70/100/100/90, Forecast | All four forms. Weather Ball and the weather moves were already there. |
| Luvdisc | 43/30/55/40/65/97, Swift Swim / Hydration | 70/30/65/70/95/105, Swift Swim / Hydration | Bulk so Swift Swim is worth a slot. Learns Rain Dance and Heal Pulse. |
| Delcatty | 70/65/65/55/55/90, Cute Charm / Normalize / Wonder Skin | 80/65/65/80/65/110, Cute Charm / Prankster / Normalize | Prankster support. Learns Thunder Wave and Assist. |
| Spinda | 60 across, Own Tempo / Tangled Feet / Contrary | 80/60/60/70/60/80, Prankster / Tangled Feet / Contrary | Contrary stays hidden. Learns Skill Swap and Assist. |
| Volbeat | 65/73/75/47/85/85, Illuminate / Swarm / Prankster | 75/73/85/47/85/95, Prankster / Swarm / Illuminate | Prankster is now the first ability. Learns Encore. Tail Glow stays. |
| Illumise | 65/47/75/73/85/85, Oblivious / Tinted Lens / Prankster | 75/47/85/80/85/95, Prankster / Tinted Lens / Oblivious | Learns Follow Me. Encore and Wish were already on the learnset. |
| Lunatone | 90/55/65/95/85/70, Levitate | 90/55/70/110/90/60, Levitate | Slower, so Trick Room still wants it. Learns Trick Room. |
| Solrock | 90/95/85/55/65/70, Levitate | 90/115/90/55/70/60, Levitate | Physical twin, still slow. Learns Trick Room. |
| Zangoose | 73/115/60/60/60/90, Immunity / Toxic Boost | 73/125/60/60/60/105, Toxic Boost / Immunity | Faster attacker. Learns Facade. Toxic Boost is the ability you see first. |
| Seviper | 73/100/60/100/60/65, Shed Skin / Infiltrator | 73/100/60/115/65/80, Shed Skin / Merciless / Infiltrator | Merciless crits a poisoned target. Learns Toxic. |
| Kecleon | 60/90/70/60/120/40, Color Change / Protean | 70/100/70/70/120/70, Color Change / Protean | Faster, and it learns Skill Swap. Protean stays hidden. |
| Tropius | 99/68/83/72/87/51, Chlorophyll / Solar Power / Harvest | 109/78/83/82/87/71, Chlorophyll / Solar Power / Harvest | Learns Tailwind. Wide Guard stays. |
| Chimecho | 75/50/80/95/90/65, Levitate | 85/50/85/110/100/85, Levitate / Telepathy | Telepathy is hidden, so Levitate is still the usual ability. Heal Bell and Heal Pulse stay. |
| Huntail | 55/104/105/94/75/52, Swift Swim / Water Veil | 55/120/105/94/75/80, Swift Swim / Water Veil | Learns Aqua Jet. Shell Smash and Rain Dance stay. |
| Gorebyss | 55/84/105/114/75/52, Swift Swim / Hydration | 55/84/105/125/80/80, Swift Swim / Hydration | Faster special attacker. Draining Kiss and Shell Smash stay. |
| Relicanth | 100/90/130/45/65/55, Swift Swim / Rock Head / Sturdy | 100/110/130/45/70/65, Swift Swim / Rock Head / Sturdy | Head Smash was already on the learnset. |
| Mawile | 50/85/85/55/55/50, Hyper Cutter / Intimidate / Sheer Force | 65/100/90/55/65/50, Intimidate / Hyper Cutter / Sheer Force | Intimidate is the first ability. Learns Play Rough at level 32. |
| Sableye | 50/75/75/65/65/50, Keen Eye / Stall / Prankster | 70/75/85/75/85/40, Prankster / Stall / Keen Eye | Prankster is the first ability. Learns Will-O-Wisp. Quash was already at level 30. |
| Nosepass | 30/45/135/45/90/30, Sturdy / Magnet Pull / Sand Force | 60/45/135/45/90/30, Sturdy / Magnet Pull / Sand Force | More HP. Sturdy still takes the first hit. |
| Clamperl | 35/64/85/74/55/32, Shell Armor / Rattled | 55/64/90/80/60/40, Shell Armor / Rattled | Enough bulk and speed to reach Huntail or Gorebyss. |
| Whiscash | 110/78/73/76/71/60, Oblivious / Anticipation / Hydration | 110/95/73/76/71/60, Oblivious / Anticipation / Hydration | Higher attack. Hydration stays hidden. Juan's Whiscash is set to Hydration. |

Together on the same tile, a wild double of any of these pairs is the pair itself: Plusle and Minun, Volbeat and Illumise, Lunatone and Solrock, Seviper and Zangoose, Indeedee male and female.

| New Pokémon | Where | Level |
| --- | --- | --- |
| Veluza | Route 106, surfing | 20–25 |
| Basculegion | Route 107, surfing | 25–30 |
| Dondozo | Route 109, surfing | 25–30 |
| Sandile | Route 111 desert | 19 and 21 |
| Helioptile | Route 111 desert | 20 and 22 |
| Gigalith | Desert Underpass | 42 |
| Greavard | Mt. Pyre 1F | 24 |
| Dreepy | Mt. Pyre 1F | 28 |
| Mimikyu | Mt. Pyre 1F | 29 |
| Politoed | Route 119 surf, and Route 120 surf | 26–30, and 25–30 |
| Abomasnow | Shoal Cave, low tide inner room | 32 |
| Lechonk | Route 102 | 3 |
| Tandemaus | Route 102 | 4 |
| Applin | Route 104 | 4 |
| Tarountula | Petalburg Woods | 5 |
| Shroodle | Petalburg Woods | 6 |
| Nacli | Granite Cave 1F | 7 |
| Glimmet | Granite Cave 1F | 10 |
| Pawmi | Route 110 | 13 |
| Bramblin | Route 113 | 16 |
| Noibat | Meteor Falls 1F | 20 |
| Charcadet | Jagged Pass | 22 |
| Toedscool | Route 119 | 24 |
| Indeedee (male) | Route 118 | 24 |
| Indeedee (female) | Route 118 | 26 |
| Fidough | Safari Zone, south | 27 |
| Cyclizar | Safari Zone, south | 29 |
| Cetoddle | Shoal Cave, low tide inner room | 30 |
| Finizen | Route 124, surfing | 25–30 |
| Tatsugiri | Route 128, surfing | 25–30 |
| Wiglett | Underwater Route 126 | 30–35 |
| Varoom | Magma Hideout 1F | 33 |
| Tinkatink | Victory Road 1F | 38 |
| Gimmighoul | Victory Road 1F | 36 |
| Frigibax | Victory Road 1F | 40 |
| Jangmo-o | Meteor Falls B1F | 40 |
| Rookidee | Route 104 | 4 and 5 |
| Wooloo | Route 116 | 6 and 7 |
| Nickit | Route 116 | 6 and 8 |
| Crabrawler | Route 106, surfing | 10–14 |
| Arrokuda | Route 108, surfing | 20–25 |
| Capsakid | Route 111 desert | 20 and 22 |
| Cufant | Jagged Pass | 22 and 23 |
| Flamigo | Route 115 | 23 and 26 |
| Tadbulb | New Mauville | 22 and 24 |
| Impidimp | Route 121 | 26 and 28 |
| Pawniard | Victory Road B1F | 38 and 40 |

Gigalith is on the Desert Underpass. The underpass encounter table already runs from 35 to 45, so level 42 is inside that table. Sandile and Helioptile stay at the desert's own levels. Tadbulb uses New Mauville's own levels, 22 and 24, because that cave's table runs from 22 to 26. Crabrawler is a surf encounter on Route 106, which has no grass.

Their evolution lines are in the Hoenn Pokédex beside the route where you first meet them: Corvisquire and Corviknight, Dubwool, Thievul, Crabominable, Barraskewda, Scovillain, Copperajah, Bellibolt, Morgrem and Grimmsnarl, Bisharp and Kingambit. Flamigo has no evolution.

Gym leaders, gym partners, rematches, the Elite Four, and both champions now have Smart Switching and Try To 2HKO on top of their old AI. The doubles-trick score bonuses are a bit higher, so they commit to Thunder Wave, Fake Out, Discharge, Eruption, Protect, Helping Hand, Tailwind, Trick Room, and the safe Earthquakes more often. Ace levels are the same.

The first clear of each gym was also edited so the two sides do not hit each other. Ace levels stay 16, 22, 28, 34, 40, 46, 52, and 58. Geodude and Onix use Protect. Lileep uses Mega Drain. Fake Out stays on Hariyama and Medicham, and Meditite is Telepathy. Voltorb uses Light Screen, and Joule sends Minun with Volt Absorb so Wattson's Discharge heals it. Camerupt uses Protect, and Cole sends Tropius with Solar Power beside Flannery's Eruption. Exploud uses Shadow Ball, Spinda uses Protect, and Zangoose holds a Toxic Orb. Altaria uses Dragon Pulse, Gyarados uses Ice Fang, and Winona's Pelipper is Drizzle with Hurricane. Claydol uses Protect, Xatu uses Trick Room, and Gardevoir uses Helping Hand. Whiscash is Hydration with Liquidation, and Kingdra is Swift Swim with Surf, Ice Beam, Dragon Pulse, and Protect. The level 68–70 rematches are a separate bracket and were left as they are.

## Hoenn Pokédex

Houndour and Houndoom sit with Poochyena, so the Cinder starter registers as soon as you take it. Every visitor above, and the rest of its evolution line, is in the Hoenn Pokédex after Deoxys. The new pages sit in roughly the order you can meet them. Seeing or catching one registers it before the National Dex is unlocked. The area screen reads the wild tables, so a seen species lights the route, cave, or water where that species is encountered. An evolution lights a place only when that evolved form is itself in a wild table. Politoed, Abomasnow, and Gigalith are. A pre-evolution you have not seen yet stays a blank slot until you do.

A species with no Hoenn number no longer counts as a Hoenn species for the summary screen or a link trade. Before the National Dex, a link trade still accepts anything in this regional list, including the visitors.

## People, trades, and errands

These work for one player. A link partner is not required, and none of them hand out a story item or a badge.

In-game trades copy the level of the Pokémon you give. Each trade happens once.

| Who | Where | They want | They give |
| --- | --- | --- | --- |
| Lass | Route 102 | Zigzagoon | Lechonk, nicknamed Oinker |
| Woman | Petalburg Woods | Wurmple | Tarountula, nicknamed Webby |
| Black Belt | Jagged Pass | Numel | Charcadet, nicknamed Cinder |
| Expert | Meteor Falls 1F | Bagon | Jangmo-o, nicknamed Scales |
| Swimmer | Route 124 | Wingull | Finizen, nicknamed Bubbles |
| Man | Route 110 | Electrike | Pawmi, nicknamed Spark |
| Black Belt | Granite Cave 1F | Aron | Nacli, nicknamed Salt |
| Camper | Route 113 | Spinda | Bramblin, nicknamed Tumble |
| Woman | Route 118 | Ralts | Indeedee, nicknamed Helper |

Show the person a matching Pokémon from your party or your PC. The errand is once each. A full Bag refuses the item, and they ask again.

| Who | Where | Show them | Reward |
| --- | --- | --- | --- |
| Expert | Granite Cave 1F | Nacli or Glimmet | Hard Stone |
| Lass | Route 110 | Pawmi | Magnet |
| Hex Maniac | Route 113 | Bramblin | Soft Sand |
| Psychic | Route 118 | Indeedee, either | Destiny Knot |
| Woman | Route 119 | Toedscool | Big Mushroom |
| Hex Maniac | Mt. Pyre 1F | Greavard, Mimikyu, or Dreepy | Spell Tag |
| Pokéfan | Safari Zone, south | Fidough or Cyclizar | Soothe Bell |
| Expert | Shoal Cave, low tide inner room | Cetoddle or Abomasnow | Never-Melt Ice |
| Expert | Victory Road 1F | Gimmighoul, Tinkatink, or Frigibax | Nugget |
| Bug Catcher | Petalburg Woods | Shroodle | Poison Barb |
| Woman | Route 104 | Applin | Sweet Apple |
| Fisherman | Route 106, on the sand | Crabrawler | Protective Pads |
| Boy | Mauville City | Plusle, then Minun | Cell Battery |
| Hiker | Meteor Falls 1F | Noibat | Sharp Beak |
| Camper | Route 111 desert | Sandile or Helioptile | Heat Rock |
| Expert | Magma Hideout 1F | Varoom | Air Balloon |
| Swimmer | Underwater Route 126 | Wiglett | Dive Ball |
| Fisherman | Lilycove City | Dondozo | Mystic Water |

The Mauville boy wants Plusle on the first visit and Minun on the next. The Cell Battery is given only after both. A full Bag on the second visit keeps the errand open.

Two people only talk. A girl in Oldale explains the Pokédex map. A scientist in Mauville explains the same-spot pairs, including Indeedee.

## Types and evolution items

Fairy is part of the battle rules. The type chart, the summary icons, and the Pokédex search all include it. Clefairy, Jigglypuff, Marill, Ralts, Mawile, and Impidimp are Fairy. Charm and Baby-Doll Eyes are Fairy moves, so an Eevee that knows one can become Sylveon by friendship.

Fire, Water, Thunder, Leaf, Moon, and Sun Stones were already placed in Hoenn. These later evolution items were not. Each one below is a single Poké Ball. Metal Coat, Dragon Scale, Prism Scale, and Reaper Cloth can be used from the bag. The same stones, apples, and held items are also sold by the evolution clerk on the third floor of the Lilycove Department Store.

Poké Marts sell more balls as you go east. Oldale adds Great Balls once the Pokédex is running. Petalburg and Rustboro add Nest Balls, and Rustboro also adds Net Balls. Mauville adds Ultra, Repeat, and Timer Balls, plus a Thunder Stone and a Moon Stone. Verdanturf sells a Leaf Stone and a Sun Stone. Fallarbor sells a Moon Stone. Lavaridge sells a Fire Stone. Fortree adds Dusk Balls and a Shiny Stone. Lilycove's second floor sells the full ball case, including Dive, Quick, Heal, Luxury, and Premier Balls. Mossdeep adds a Dawn Stone and an Ice Stone. Dewford and Pacifidlog have no mart, so a clerk in each Pokémon Center sells a small stock. Pacifidlog's clerk also sells Dive Balls, a Water Stone, and a Prism Scale.

| Item | Where | What it evolves |
| --- | --- | --- |
| Tart Apple | Route 104 | Applin into Flapple |
| Sweet Apple | Route 104 errand | Applin into Appletun |
| Syrupy Apple | Route 123 | Applin into Dipplin. The next level, after it learns Dragon Cheer, becomes Hydrapple |
| Ice Stone | Shoal Cave, low tide ice room | Cetoddle into Cetitan |
| Dawn Stone | Meteor Falls 1F | A male Kirlia into Gallade |
| Dawn Stone | Shoal Cave, low tide inner room | A female Snorunt into Froslass |
| Shiny Stone | Route 117 | Roselia into Roserade |
| Reaper Cloth | Mt. Pyre 1F | Dusclops into Dusknoir |
| Prism Scale | Route 119 | Feebas into Milotic |
| Metal Coat | Granite Cave B2F | Onix into Steelix |
| Dragon Scale | Route 124, on the sandbar | Seadra into Kingdra |

Crabrawler and Eevee can also become Crabominable and Glaceon by leveling in the Shoal Cave ice room. Eevee can become Leafeon by leveling in Petalburg Woods.

## Level cap

Default on. The steps are 16, 22, 28, 34, 40, 46, 52, 58, then 66 for the Elite Four, then 75 after the champion. EXP, Rare Candy, the day care, and EXP Share stop at the cap. A Pokémon already above the cap is not lowered. The party screen shows the cap next to the level. Both saves have to agree before a link comes up. Gym levels do not scale.

## Patch

After `make`:

```sh
sh tools/coop_make_patch.sh emerald-coop.bps
```

The script checks the clean ROM hash. It uses `flips` for BPS, or `beat` for UPS. If neither is installed it prints the command and does not invent a patch. Apply the patch to a clean ROM with the same tool. Do not put the patched ROM in this repo.

## Two-player script

This is the pass to run on two PCs. It was not executed in the environment that produced this tree. A passing host protocol test is not that pass.

1. Patch two copies from the same clean ROM. Put a different `.sav` next to each copy.
2. mGBA 0.10.5: open both, File, New Multiplayer Window. RetroArch: both sides on gpSP 1.1.0, Link Cable - Pokémon Gen3, relay or UPnP or a direct IP.
3. Cold boot. CO-OP on both games. Walk to different maps. Confirm no sprite. Meet on Route 101. Confirm the sprite and that the CO-OP menu names that map. Walk through a door. Confirm the sprite vanishes and comes back. The Twin Beacon is off after every reconnect. One player asks, the other says no, and an ordinary trainer stays solo. Both agree, and an ordinary trainer can be a co-op battle. Ask to turn it off during a gym. The gym finishes in the mode it started, then the Beacon clears. The item is still in the bag.
4. Rescue Birch together. Pick one case. Split the two starters. Fight the first rival pair.
5. Clear Rustboro together. Pull the cable during the gym. Confirm no badge and both parties restored. Reconnect. Clear it. Confirm both saves have the Stone Badge and the TMs stayed with whoever finished the speech.
6. First three: each player carries six. Start a co-op battle. Confirm only slots 1–3 appear in battle, on the party screen, and in the switch prompt. Faint all three on one side and confirm the partner keeps fighting. Win it. Confirm slots 1–3 kept their damage and PP, slots 4–6 match how they were before, and the CO-OP menu still shows the link. Run a solo battle and confirm all six can switch in. Faint all of one player's first three outside battle, with a healthy fifth slot, and confirm a gym refuses on both games with the right message. Pull the cable mid-battle and confirm all six come back as they were.
7. Trade one Pokémon from the party screen. Save both games. Reload. Confirm the trade stuck and the parties were not swapped twice.
8. Turn the cap off on one save, try to link, confirm it refuses. Turn it back on and link.
9. Build mismatch: one older patch against this one. Confirm the link stops.
10. Route 102 wild, Beacon on, both on the route. Step in grass and confirm both games enter one wild double with the same two Pokémon. Catch the left one and leave the right one, then finish. Confirm only the thrower has it, in the party or the PC, with a nickname prompt and a caught Pokédex entry. Both throw at the same Pokémon on one turn: one owner, the second ball is gone, no copy. Throw at different ones: both are caught, one each. Pull the cable during the shakes: nobody gets it. Fill the party and confirm the catch goes to the PC. Fill the PC as well and confirm the ball refuses and the fight goes on. Choose RUN on one game only and confirm the battle continues. Choose RUN on both and confirm it ends on both. Step in grass on Route 101 and confirm it stays a stock solo battle.
11. Rick and Allen. Fresh pair of saves. Beat Rick solo on one save with the cable off. Confirm Allen still fights. Link, Beacon on, both on Route 102. Walk into Allen on that save and confirm a solo fight with no partner prompt. On a second pair of fresh saves, walk into Rick, say yes on both, and win. Confirm both saves have both flags. Repeat and pull the cable mid-fight: no flags. Repeat with the partner in a menu: solo fight against the one you walked into.
12. Late game, on a second pass: Groudon and Kyogre, pull the link after the ball shakes and before the catch commits, confirm nobody has the Pokémon. Catch it on a retry and confirm only one save owns it.

## Known limits

- Route 101 across the internet was not proven here. The RetroArch steps above are the test, not a result.
- Coordinates in the link frame are 8-bit. A map wider than 255 tiles wraps the partner sprite.
- The acro bike is the extra bit in the facing nibble. Mach bike and acro bike can both be shown. Surf wins over a bike.
- After a shared gym or Elite Four win, the guest receives the same story flags and the way forward. The person who talked to the leader still gets the TM. The guest can ask the leader for that TM afterward.
- A shared champion win sends the guest to the Hall of Fame so their party is recorded too.
- A partner who is already inside a script can miss the lab warp. Stand on the overworld when the other person opens Birch's bag.
- Co-op battles field three Pokémon per player, through the stock link-multi exchange. Slots 4–6 sit out.
- gimmicks stay off, including primals.
- Id 865 is unused on purpose. It is the in-game partner id for Steven, and a normal trainer cannot live there.
- Invalid link frames are dropped. One bad packet does not disconnect you.

## Credit

Pokémon and Pokémon Emerald are Nintendo, Game Freak, and Creatures. This project does not ship their ROM.

Engine: pret pokeemerald, and RHH pokeemerald-expansion 1.17.1.
