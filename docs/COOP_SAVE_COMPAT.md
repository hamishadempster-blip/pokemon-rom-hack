# Save compatibility (Emerald co-op)

How a save made on one build keeps working on later builds, what is reserved for future content (the archipelago), and how to check it. Written for build 0xC017013F; checked again on 0xC0170140 (no save change between them).

## The rule for players

- A save made on an **older** build loads on a newer one. Nothing is reset: party, PC, bag, money, Pokédex, badges, flags and variables come over unchanged; the co-op block gains new fields, empty at first.
- A save written by a **newer** build is not guaranteed to load correctly on an older build. Keep a copy of your `.sav` before updating, and do not go back to an older ROM with a save the newer one has written.
- Both players update to the same build (the link refuses mismatched builds). Each keeps their own `.sav`; the ROM can be replaced without touching it (see the handoff's release instructions).

## Where the save data lives

| Block | Used / capacity (bytes) | Free | Contents |
| --- | --- | --- | --- |
| SaveBlock1 | 15,568 / 15,872 | 304 | Position, party, money, bag, flags, variables, game stats, map objects, Pokédex flags |
| SaveBlock2 | 3,884 / 3,968 | 84 | Name, trainer id, options, play time, encryption key, Frontier records |
| SaveBlock3 | 564 / 1,624 | 1,060 | `struct CoopSave` (everything co-op), after the expansion's optional SaveBlock3 fields (all off) |
| PC storage | 34,144 / 35,712 | 1,568 | Boxes |

`struct CoopSave` (include/global.h) is versioned by its `layout` byte: 0 before versioning, then layouts 1–4. Layout 4 (0xC017013E) added the field kit, the duo Frontier records and the joint-outcome partner records. Each layout only **appends** fields; a save with an older layout gets the new fields zeroed on load and its `layout` raised (src/coop.c, the CoopSave migration). No field was ever moved, resized or reused.

## Identifiers that must not change

- Species, form, move, item, ability, trainer, map (group/number), flag and variable ids. Saves store them as numbers; renumbering changes what an existing save holds.
- `FLAGS_COUNT` and `VARS_COUNT`: the flag and variable arrays sit in the middle of SaveBlock1, so growing them moves every field after them.
- `POKEMON_SLOTS_NUMBER` (it sizes `dexSeen`/`dexCaught` in SaveBlock1) and `COOP_DEX_FLAG_BYTES` (132, the partner Pokédex snapshot).
- The expansion's optional save features that add SaveBlock3 fields before `CoopSave`: `OW_USE_FAKE_RTC`, `FNPC_ENABLE_NPC_FOLLOWERS`, `OW_SHOW_ITEM_DESCRIPTIONS` (first-time mode), `USE_DEXNAV_SEARCH_LEVELS`, `APRICORN_TREE_COUNT`. All are off; turning one on moves `CoopSave` and breaks every existing save.
- Trainer ids at or above `MAX_TRAINERS_COUNT` (864) have no defeated flag: setting one would write a system flag (SetTrainerFlag and the co-op win path both refuse them). New trainers that need a flag must be below 864; party-only variants may sit above.

## Free space for future content (the archipelago)

- **Flags:** 59 marked `FLAG_UNUSED_*`: 0x2C–0x4F, 0x54–0x55, 0x68, 0x71, 0xE9, 0x1AA–0x1AB, 0x1DA, 0x1DE–0x1E3, 0x264, 0x2D9, 0x468, 0x470, 0x472, 0x479, 0x4F9–0x4FA, 0x4FF (as of this build; check `include/constants/flags.h` before taking one).
- **Variables:** 0x404E and 0x40F8–0x40FF (9).
- **CoopSave:** 1,060 bytes after the layout-4 fields. This is the place for new persistent state (an area's progress bits, counters, records): append a block, bump `layout` to 5, zero it in the migration.
- Flags and variables are scarce; prefer a CoopSave bit field for anything new and keep the unused flags for scripts that need `setflag`/`checkflag`.

## Rules for a future update

1. Append, never reorder or resize existing fields. One new `layout` value per release that adds fields; the migration zeroes exactly the new fields and is safe to run twice (it checks `layout`).
2. New areas open through ordinary conditions an existing save can meet (a badge, a story flag already set at that point, an item), never a flag only a new game sets.
3. Never repair a save by deleting Pokémon, items or progress. If something must be corrected, record what changed and keep the player's data.
4. Before release, run the compatibility check below on representative saves from the previous release and from early, mid and late game.

## The compatibility check

`tools/coop_link_harness/compat_check.sh` loads each save on the previous build and on this one, saves on this one, power-cycles, links, and compares the save data with `compat.py` (field by field, encrypted fields decrypted). Fields rebuilt on every load (map object templates, the map view), the play-time clock and the values the game rerolls on a new day are reported but allowed; the relink step may change only the link bookkeeping (partner Pokédex snapshot, quest partner).

Run it from a built tree: `compat.py` compiles the field offsets against `include/`, which needs the headers `make` generates (`map_groups.h` and others); in a bare `git archive` copy the comparisons stop with "offset compile failed".

Result for 0xC0170140 (and 0xC017013F before it) against 0xC017013D (`1b26b863`, the last build before CoopSave layout 4), saves from checkpoints j06, j16, j44, j68, seg6, the lab saves and a save written by 0xC017013E: 21 of 21 comparisons pass; details in the test log (section *Save compatibility*).
