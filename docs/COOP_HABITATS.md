# Hidden habitats

Section 3 of the collection update, added after build 0xC0170141.

## The five habitats

Each habitat is a small place off an ordinary map. The way in looks like nothing much: stand at the spot and press A, and the game asks whether to go through. Inside are its own wild Pokémon, a marker stone, and one find per save. The way out (an arrow, a ladder or a cave mouth) leads back to the spot.

| Habitat | Way in (stand at, face) | Ground | Residents (all three complete it) | Others there | Find | Completion reward |
| --- | --- | --- | --- | --- | --- | --- |
| MOSS HOLLOW | Petalburg Woods, west: (8,19), face the trees south | grass, Lv 7–10 | Sewaddle, Pikipek, Nymble | Combee, Blipbug, Skiddo, Buneary, Hatenna (1%) | Leaf Stone | 5 Nest Balls |
| TIDE GROTTO | Route 109's sandy islet: (32,31), face the water east (Surf to get there) | surfing and rods | Corsola, Clauncher, Cramorant | Lapras, Mantyke; rods: Luvdisc, Bruxish, Dhelmise (1%) | Water Stone | 5 Dive Balls |
| EMBER HOLLOW | Jagged Pass: (20,28), face the rock wall west | cave, Lv 20–24 | Heatmor, Turtonator, Litleo | Slugma, Magby, Capsakid, Numel | Fire Stone | 3 Luxury Balls |
| SHADE CHAMBER | Mt. Pyre's lower slope: (27,27), face the rocks east | cave, Lv 27–31 | Golett, Yamask, Sandygast | Sableye, Litwick, Duskull; Dreepy, Spiritomb (1% each) | Dusk Stone | Reaper Cloth |
| STARFALL CAVE | Mossdeep's eastern rocks: (63,30), face east | cave, Lv 35–40 | Clefairy, Elgyem, Bronzor | Solrock, Lunatone, Nosepass, Baltoy; Chingling, Beldum (1% each) | Moon Stone | Comet Shard |

**The maps.** They reuse existing layouts of places most players never see, the Southern-Island-style event islands and post-game caves: Faraway Island's interior, Marine Cave's entrance, Terra Cave's entrance, Navel Rock B1F and Steven's cave in Meteor Falls. Each is a new map: `MAP_HABITAT_*`, added at the end of the Dungeons group, so no existing map id changes. They show no map-name popup; the first visit names the place.

## Progress (this save's own)

Progress is kept in `CoopSaveV5.habitat` (reserved space; no layout change).

| Field | Meaning |
| --- | --- |
| found | Entered once: "FIELD NOTES: ASH found MOSS HOLLOW!" (with a fanfare), then "Three kinds of POKéMON live here. Catch all three to complete it." |
| cluesHeard | Its rumor was read: the WILDLIFE REPORT in the nearby town's Pokémon Center (Petalburg, Slateport, Lavaridge, Lilycove, Mossdeep) ends with "Travelers also tell of a hidden place near here…" and the clue |
| interactions | The marker stone was read (a line of lore) |
| claims | The find was picked up, once. With a full BAG it stays where it is |
| complete | Its three residents are all caught. Pokédex caught flags count, so one caught elsewhere counts too |
| rewarded | The completion reward was handed over by the FIELD NOTES ([COOP_NOTES.md](COOP_NOTES.md)): paid once, or kept waiting in the pending queue when the BAG is full |

- **FIELD NOTES pages.** A summary ("Hidden habitats found: 1 of 5. Complete: 0.") is followed by one page per habitat. A found habitat shows "MOSS HOLLOW: 2 of 3 residents caught. Something lies there."; one only heard of shows its clue.
- **Pokédex AREA and the manifest.** The AREA page shows the habitats like any map with wild Pokémon. The acquisition manifest gates them by their entrance (Moss Hollow from the start, Ember Hollow at 3 badges, Tide Grotto and Shade Chamber at 5 with Surf, Starfall Cave at 7).
  - Still 908 of 908 obtainable, 0 issues.
  - Changes the manifest shows: Luvdisc earlier (5 badges, was 8). Beldum and Chingling are now wild (1%, Starfall Cave, 7 badges); Beldum's dex hint is its area now, not "GIFT".
  - A manifest artifact: Chingling's earliest is listed as 7 (was 5 by breeding), because the manifest stops listing Day Care sources for species it finds renewable some other way. Breeding still works in the game.

## Co-op

- **Progress and finds are personal.** Each save has its own progress, and its own copy of the find.
- **The maps are ordinary maps.** A linked partner who goes through the same spot is seen there, and co-op wild battles work as anywhere else.
- **Not tested linked.**

## Tests (harness)

| Scenario | Checks |
| --- | --- |
| `habitat_moss` | j16 saves. Entrance question; first-visit message and found bit; a grass encounter (Buneary); marker (interactions bit); Leaf Stone once (claims bit, ball gone, A again does nothing); out by the arrow to the tree. FIXTURE: the three residents' caught flags. Then "MOSS HOLLOW complete! Kai got 5 Nest Balls!" once (complete and rewarded bits; a second check pays nothing). The FIELD NOTES habitat pages. Petalburg's WILDLIFE REPORT ends with the rumor (cluesHeard bit) |
| `habitat_tour` | j68 saves. The other four: entrance, found, marker, find (once), a wild Pokémon of their own (Tide Grotto: a SUPER ROD bite gave Corsola; Ember Hollow: Heatmor; Shade Chamber: Yamask; Starfall Cave: Bronzor), out again to the entrance |

**Not tested:**
- surfing in Tide Grotto;
- a full BAG at a find;
- habitats while linked;
- the habitats' completion rewards other than Moss Hollow's (the same code path).
