# Town specialities

Section 12 of the collection update, added after build 0xC0170141. Six towns each get a specialist with one service and a small track, a reward paid once when the service has been used enough. Rustboro's Trainer Exchange already covers move tutoring, the Egg move relearner, EV resets, Hyper Training and mints, so these services are different on purpose.

| Town (specialist, where) | Service | Track (reward, once) |
| --- | --- | --- |
| DEWFORD: BEACH HUT keeper, south of the Pokémon Center | Once a day, rest on the beach with your Trainer Bond partner: +120 Bond XP (announced like other Bond gains) and friendship +5. Without a partner it points to Rustboro's BOND MENTOR | 5 days: Shell Bell |
| FALLARBOR: BALL ARTISAN, by the Pokémon Center | Moves one of your own Pokémon into another BALL from your BAG; the BALL is used up. Not for an Egg, or a Pokémon from another trainer (its BALL is theirs). The list opens on the first BALL, and the BALL it is already in says so | 3 times: 3 Luxury Balls |
| VERDANTURF: GROOMER, by the Pokémon Center | Once a day, groom a Pokémon: friendship +20 (a happy-as-can-be one enjoys it all the same) | 5 days: Soothe Bell |
| FORTREE: BERRY KEEPER, by the Pokémon Center | Once a day, today's BERRY (Oran, Sitrus, Lum, Leppa, Pecha, Chesto, Rawst in turn). With no room it is not counted, so it can be collected later that day | 7 days: Starf Berry |
| MOSSDEEP: STAR JUDGE, south of the Pokémon Center | Reads a Pokémon's IVs: a verdict (decent / above average / relatively superior / outstanding), its best stat, then all six values. Read only | 5 times: Bottle Cap |
| PACIFIDLOG: TIDE READER, by the Pokémon Center | Shoal Cave's tide now and when it turns, and whether Routes 119 and 123 are rainy today. Read only | 3 days: Mystic Water |

## Rules

- **Personal.** Everything is this save's own: `CoopSaveV5.town[6][6]`, reserved space, so there is no layout change. Per town:
  - `[0]` uses (days for the daily services);
  - `[1]` bit 0, its reward handed over;
  - `[2..3]` the last day used.
- **Days** are the cartridge clock's day count. Resting at a Pokémon Center changes the time of day, never the day ([COOP_TIME.md](COOP_TIME.md)), so it repeats nothing here. The tides use the real hour too.
- **Paid once.** A reward's bit is set in the same moment it goes into the BAG. With no room it waits in `CoopSaveV5.pending` and the FIELD NOTES hand it over later ([COOP_NOTES.md](COOP_NOTES.md)). With that queue full too, it is not claimed and is paid on a later use.
- **The NPCs.** They are ordinary map objects added to each town's map, placed off the paths.

Code: `src/coop_town.c`, `data/scripts/coop_town.inc`.

## Tests (harness)

| Scenario | Checks |
| --- | --- |
| `towns_tour` | j68 saves. FIXTURES: party slot 0 designated as the Bond partner; Dewford's count set to 4 and its last day to another day. Dewford: rest once ("Your bond grew stronger"), then "You've rested here today"; the 5th day pays the Shell Bell once (bit set, item in the BAG). Fallarbor: slot 0 into a Great Ball (the second listed), one Great Ball used. Verdanturf: groomed, then "today's grooming done". Fortree: today's BERRY, then "That was today's pick". Mossdeep: the IV reading. Pacifidlog: tides and weather. Each town's use count |

Found and fixed while testing:
- The BALL list opened on CANCEL (the menu's initial id 0 was CANCEL's): it now opens on the first BALL.
- "There! Nest Ball ×3 is in a…": the BALL list had reused STR_VAR_1. The Pokémon's name is buffered again.

**Not tested:**
- the Fallarbor refusal for another trainer's Pokémon;
- a full BAG at a track reward;
- the other towns' track rewards (the same code);
- linked play next to these NPCs.
