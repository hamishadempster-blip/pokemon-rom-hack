# Save storage plan (collection, base and polish update)

The storage and migration design for the update that adds the Trainer Bond, hidden habitats, town specialities and the expedition headquarters, written before any of their persistent fields. It records what the battery save is, what is free, what each new feature stores and where, and how a save moves to the new layout. Rules for later updates: [COOP_SAVE_COMPAT.md](COOP_SAVE_COMPAT.md).

Everything here was checked against this tree (pokeemerald-expansion 1.17.1 at build 0xC0170140) and against real saves; the evidence is listed with each fact.

## The battery save as it is

**Flash.** 128 KiB, 32 sectors of 4 KiB. Sectors 0–13 are save slot 1, 14–27 slot 2, 28–29 the Hall of Fame, 30 Trainer Hill, 31 the recorded battle (`include/save.h`).

**A sector** holds 3,968 bytes of data, then 116 bytes of SaveBlock3, then a 12-byte footer: sector id (u16), checksum (u16), signature `0x08012025` (u32), save counter (u32). The checksum is the folded 32-bit sum of the **data part only** (`CalculateChecksum(data, size)`): the 116 SaveBlock3 bytes are not covered by it.

**A slot** holds, by sector id: 0 SaveBlock2; 1–4 SaveBlock1; 5–13 the PC (`sSaveSlotLayout`, `src/save.c`). SaveBlock3 is cut into 14 chunks of 116 bytes, chunk *k* in the sector with id *k*.

| Block | Used / capacity (bytes) | Where |
| --- | --- | --- |
| SaveBlock2 | 3,884 / 3,968 | sector id 0 |
| SaveBlock1 | 15,568 / 15,872 | sector ids 1–4 |
| PC storage | 34,144 / 35,712 | sector ids 5–13 |
| SaveBlock3 | 564 / 1,624 | the 116-byte chunks of ids 0–13 |
| `struct CoopSave` | 560, at SaveBlock3 offset 4 | SaveBlock3 bytes 4–563, so chunks 0–4 (ids 0–4) |

(Sizes from `tools/coop_save_inspect.py`, which compiles `sizeof`/`offsetof` from these headers.)

**Writing a save.** A normal save (`SAVE_NORMAL`, and the Hall of Fame and incremental link saves `LinkFullSave_*` used by co-op deals and trades) writes all 14 sectors into the *other* slot with the save counter + 1. Each write rotates which physical sector gets id 0 by one more than last time (`gLastWrittenSector`), so the same slot is rotated by two between its consecutive writes.

**Reading.** `GetSaveValidStatus` accepts a slot only if every id 0–13 has a sector with the signature and a matching checksum; of two good slots it loads the higher counter. One good and one bad slot: it loads the good one and returns `SAVE_STATUS_ERROR`, and the main menu says "The save file is corrupted. The previous save file will be loaded."

**Interrupted writes (tested).** Because of the rotation by two, a slot cut off after *k* of its 14 sectors has overwritten two old sector ids that the new write has not reached yet (or, near the end, one): the slot is always incomplete and is never loaded half-new. Tested with the harness's new `arm_cut` (a power cut at an exact frame, the `.sav` kept as the flash was then): a save cut after 1, 2, … 14 changed sectors on one save, and after 7 completed sectors (`arm_cut 7 clean`) with the rotation at 13 different positions (1–13 extra saves first). In all 27 runs the next power-on showed the message above and loaded the previous complete save: the marker set before the cut save (variable 0x40F8) and the PC filled before it were both absent, never half there. Regression: `save_power_cut` + `save_power_cut_load` (j16 saves; the load also checks a normal save after the recovery). A cut before any sector changed loads the old save with no message. **Repeated on the layout-5 ROM** (after the save-path changes below, CoopSave grown to 940 bytes of SaveBlock3): cuts after 1–14 changed sectors, and after 7 at 13 rotation positions: 27 of 27 loads reported `SAVE_STATUS_ERROR` (the message) and the previous save (marker and PC absent, save counter unchanged). (The first attempt of those 27 loads failed at the title screen: my load script waited for the main menu without pressing a button. Fixed the script and reran the loads on the same cut files.)

**In-place writes.** Two older paths rewrite the *current* slot's first five sectors (SaveBlock2 and SaveBlock1) without touching the PC sectors: `SAVE_LINK` (Battle Frontier facilities, Pike, Pyramid, link contests) and `WriteSaveBlock2`/`WriteSaveBlock1Sector` (the linked Berry Blender). They also rewrite SaveBlock3 chunks 0–4 only. Today all of `CoopSave` lies in chunks 0–4, so it stays consistent. **A layout that grows `CoopSave` past SaveBlock3 byte 579 would leave its tail from the last full save after one of these writes** (the bag and party from now, a reward ledger from before: a reward could be paid twice). So this update routes both paths to full-slot writes (below) before `CoopSave` grows.

## What is free

- **Pokémon record bits.** pokeemerald-expansion leaves 21 bits of the encrypted, checksummed part of `struct BoxPokemon` unused: substruct 0 `unused_02` (6), `unused_04` (3), `unused_0A` (2); substruct 1 `unused_04` (5), `unused_06` (3); substruct 3 `unused_0B` (1); plus two bits outside the encryption (`unused_13`, `unused_1E`). Their positions were checked by compiling each field set alone. No code in this tree reads or writes them (searched). Every real Pokémon is created zeroed and copied whole, so they travel with the individual through party, PC, Day Care, evolution, form changes and trades. **Evidence they are zero:** `tools/coop_save_inspect.py` over 1,252 `.sav` files (every journey checkpoint, the seg6 and lab saves, every save of the 0xC017013E and 0xC017013F final suites; CoopSave layouts 1–4): 1,188 loadable, 6,170 Pokémon records (party, boxes, Day Care), 0 with any of these bits set, 0 with a bad record checksum. (The other 64 files were never saved: fresh-game scenarios.)
- **SaveBlock3:** 1,060 bytes after `CoopSave` (564 of 1,624 used).
- **SaveBlock1:** 304 bytes; SaveBlock2: 84; PC: 1,568. Not used by this update: growing those structures changes their sector sizes.
- **Flags:** 59 marked `FLAG_UNUSED_*` (list in COOP_SAVE_COMPAT.md). **Variables:** 0x404E, 0x40F8–0x40FF. This update takes none of them for persistent state (the marker in the power-cut test is a test fixture on a copy).

Occupied ranges that stay as they are: species, form, move, item, ability, map, trainer ids; trainer flags (ids below 864); `FLAGS_COUNT`, `VARS_COUNT`; the 88 Mega item-ball flags and the quest flags allocated earlier (`include/constants/flags.h`).

## Capacity budget for this update

| Feature | Stored in | Size |
| --- | --- | --- |
| Bond XP of each Pokémon (15 bits, 0–32,767) | its own record: substruct 0 `unused_02` (XP bits 0–5), substruct 1 `unused_04` (6–10) and `unused_06` (11–13), substruct 3 `unused_0B` (14) | 0 bytes in the save blocks; every owned Pokémon can have a history; nothing is evicted |
| "Has been a partner" (history at 0 XP) | its record: substruct 0 `unused_0A` bit 0 | 0 |
| Reserved for later per-Pokémon use | its record: substruct 0 `unused_04` (3 bits), `unused_0A` bit 1, the two plain bits | 0 |
| Active partner (personality + trainer id), bond ledgers (first major victories, walking and friendship tallies), notices | `CoopSave` layout 5 | 32 |
| Personal settings (low-HP alarm, easy fishing, shiny escape question, catch result) | `CoopSave` layout 5 | 2 |
| Encounter period shift (waiting at the Centre) | `CoopSave` layout 5 | 4 |
| Hidden habitats: entrances found, completions, rewards, journal clues | `CoopSave` layout 5 | 16 |
| Collection rewards ledger and pending-reward queue | `CoopSave` layout 5 | 24 |
| Town specialities (6 towns × progress and claims) | `CoopSave` layout 5 | 36 |
| Expedition headquarters: partnership identity, revisions, personal corner and shared-room layout, residents, aquarium, photo wall, services | `CoopSave` layout 5 | 256 |
| Layout-5 header (size, checksum of the new block) | `CoopSave` layout 5 | 8 |
| **Total new** | | **378** |
| **Left for the archipelago and later updates** | SaveBlock3 | **682** |

The headquarters figure is a cap: if a later stage needs more, it comes out of the reserve with a recorded decision, never by shrinking another feature's records.

## Migration design

- **Schema version:** `CoopSave.layout`, independent of the ROM build id. Known: 0 (before versioning) to 4 (0xC017013E). This update adds **5**.
- **On load** (`CoopSave()` in `src/coop.c`, before any reader uses the block): a layout from 0 to 4 is migrated step by step in RAM; each step zeroes exactly its own new fields and raises `layout`. Layout 5 zeroes the new block and sets the documented defaults (settings on their defaults, no partner, no history). Nothing outside the new fields changes; no Pokémon record is rewritten.
- **Validation:** before migrating, the known fields are range-checked; after, the layout-5 block's size word and checksum are set and checked. A layout above 5 (a newer build), or a `CoopSave` whose magic is neither 0 (a save from before co-op) nor the co-op magic, is **not** migrated or repaired: CONTINUE says the save comes from another build and does not start, and the file is left exactly as it was.
- **Commit:** the migrated block reaches flash only with the player's next ordinary save (or a deal or trade save), through the slot-alternating write above. Until then every power-on migrates the old save again, which is idempotent: same input, same result; no reward, conversion or id is produced twice because layout 5 starts empty.
- **No save is written to preview a feature.** Emulator save states are not a compatibility target; the battery `.sav` is.

## Changes to save paths in this update

- `SAVE_LINK` writes the full slot (new counter, rotated) instead of rewriting five sectors in place.
- `WriteSaveBlock2` / `WriteSaveBlock1Sector` (linked Berry Blender) become the incremental full-slot write the co-op deals already use (`LinkFullSave_*`), one sector per call.

Both keep every sector of a slot from the same moment, so `CoopSave` may grow past SaveBlock3 byte 579 safely, and an interruption in either leaves the previous complete slot.

## Autosave slot roles (after 0xC0170141)

No new storage: the flash has no free sectors for a third slot, so the autosave ([COOP_AUTOSAVE.md](COOP_AUTOSAVE.md)) uses the two existing slots with roles.

- **Kind byte.** `CoopSaveV5.saveKind` (layout-5 offset 2, formerly the first byte of a zero `u16 reserved`): 0 a manual save (any save that is not the autosave: START > SAVE, link, deal, trade, Hall of Fame, Frontier), 1 an autosave. It sits at SaveBlock3 offset 566, chunk 4, so it is in the sector with id 4 and inside the in-place paths' chunks 0–4 too. Older layouts and every layout-5 save before this change read as 0, a manual save. No layout change.
- **Choosing the slot** (`ChooseSaveSlotForWrite`, `src/save.c`; every full-slot write, including the incremental link writes). The two slots are read first (all 14 ids, signature, checksum).
  - Neither complete (a new file): the stock order.
  - One complete: the other.
  - A manual save: the slot that is not the newest (the stock rule).
  - An autosave: the slot that does not hold the newest manual save. So an autosave can never replace the player's newest manual save; two autosaves in a row replace each other.
  - After going back to the manual save (below): the next save of either kind replaces the abandoned autosave, never the save just loaded.
  - The counter becomes the larger of the two + 1, raised by one more if needed so its parity names the chosen slot (as `gSaveCounter % 2` does in the stock code); the chosen slot's own rotation advances by one.
- **Power cuts:** each write still goes to one slot with a nonzero rotation, so the earlier analysis holds: a cut autosave leaves the manual save, a cut manual save leaves the other slot (tested: `autosave_power_cut`, `autosave_power_cut_load`).
- **Writing into a slot a cut left incomplete** (0xC0170142, found in review): before the write, one signature byte of each leftover valid sector there is programmed to 0 (`ProgramFlashByte`, no erase; bits only clear). Otherwise a second cut, with leftovers in the same sector order as the new write, could leave all 14 ids present from two different saves. With the leftovers invalid, a cut write is always missing ids (tested: `autosave_power_cut_twice`, `autosave_power_cut_twice_load`, with the harness's `arm_cut N signed`).
- **Loading:** `GetSaveValidStatus` is unchanged and loads the newest complete slot. When that is an autosave and the other a complete manual save, the game asks on arrival whether to keep it; "go back" reloads with `gSaveLoadPreferManual` (IWRAM, which `ReloadSave` does not clear), which loads the manual slot and marks the autosave abandoned.
