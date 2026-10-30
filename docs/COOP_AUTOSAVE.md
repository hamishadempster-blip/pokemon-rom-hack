# Autosave

Added after build 0xC0170141. The game saves by itself now and then, so a crash, a frozen emulator or a closed window loses little. The player's own save is kept apart from it, so an autosave made at a bad moment (just before a softlock, say) can always be left behind.

## What the player sees

- **When.** On entering a map (a door, a map edge, a warp, the return from a battle), once at least **five minutes** have passed since the last save of any kind or since CONTINUE.
- **How.** "Autosaving... Don't turn off the power." shows while the save is written. A full save holds the game for about four seconds of the cartridge's flash time, as START > SAVE does, so the autosave is shown, not silent, and is not frequent.
- **Turning it off.** CO-OP > SETTINGS > AUTOSAVE (ON by default; each player's own setting).

## Two kinds of save

The cartridge has two save slots. One always holds the player's newest **manual save** (START > SAVE, and every save the game itself makes in trades, deals, the Hall of Fame and facilities). An autosave only ever goes into the **other** slot. Saving manually works as before.

**After CONTINUE**, if the newest slot is an autosave and the other is the manual save, the game asks on arrival:

> This is your AUTOSAVE. Your last SAVE is older. Keep playing from here?

- **YES** carries on from the autosave.
- **NO** asks "Go back to your last SAVE instead? The AUTOSAVE is replaced when you save." **YES** reloads the manual save, exactly as at power-on, and play continues from it. The autosave stays on the cartridge until the next save of either kind, which replaces it (never the manual save just loaded). **NO** carries on from the autosave.

A power cut during an autosave leaves the manual save; during a manual save, the other slot. In either case the next power-on says "The save file is corrupted. The previous save file will be loaded." as before.

A save into a slot that an earlier power cut left incomplete first clears a signature byte of each sector left there, so a second power cut can never combine the old leftovers and the new write into a slot that looks whole but mixes two saves. An autosave also stores the tiles around the player (the map view CONTINUE restores) as the start menu's SAVE does; and NEW GAME clears the "autosave newer than your last SAVE" note made at power-on, so the question is only asked after CONTINUE.

Save counters are compared without wraparound handling; the counter would need billions of saves to wrap (not handled, not tested).

## When it does not autosave

- In battle; during a script, a message, a menu or a fade; while moving.
- During any co-op deal or shared transaction, or while this player waits on the partner (an invitation, an acknowledgement, a co-op battle starting, a pending co-op message).
- On a new game, before its first manual save. (The first save of a new file replaces the old file and its Hall of Fame, and must be the player's own choice.)
- Where START has no SAVE, or a save mid-challenge could matter: Battle Frontier facilities and Battle Tents, the Battle Pike and Pyramid, Trainer Hill, the Safari Zone, the Cable Club rooms, the Union Room.
- Without flash, or with a save from a newer build (which is never written).
- If the field is not calm within ten seconds of entering a map, that entry is skipped; the next one tries again.

Each linked player autosaves on their own. The link survives a partner's four-second save (tested in the harness only).

## Implementation

- `src/coop_autosave.c`: the trigger (`CoopAutosave_NoteMapEntry` from the two map-load sites in `src/overworld.c`, `CoopAutosave_Poll` from `CB1_Overworld`), the conditions, the write, and going back to the manual save.
- `src/save.c`: the slot roles, the kind byte and the reload preference ([COOP_STORAGE_PLAN.md](COOP_STORAGE_PLAN.md#autosave-slot-roles-after-0xc0170141)).
- `data/scripts/coop.inc`: `Coop_EventScript_Autosave`, `Coop_EventScript_AutosaveLoaded`.
- `tools/coop_save_inspect.py` prints each slot's kind.

## Tests (harness)

The 30-second interval in the tests is a fixture (`gCoopAutosaveTestInterval`); the game uses five minutes.

| Scenario | Checks |
| --- | --- |
| `autosave_new_game` | Fresh flash, NEW GAME: no autosave before the first manual save; one after it |
| `autosave_basic` | No autosave within the interval; "Autosaving..." on a map entry after it; not again at once; again after another interval. Slot 0 autosave, slot 1 the manual save |
| `autosave_reload` | CONTINUE loads the autosave and asks; YES keeps Oldale Town |
| `autosave_load_manual` (+ `_reload`) | NO, YES: the lab's manual save loads; the next manual SAVE replaces the autosave slot (slot 0, counter 6), keeps the loaded save (slot 1, counter 1); CONTINUE then asks nothing |
| `autosave_go_back_autosave` (+ `_reload`) | After going back, the next autosave replaces the abandoned autosave, not the manual save; CONTINUE loads it and asks |
| `autosave_power_cut` (+ `_load`) | Power cut after 7 sectors of an autosave: the corrupted message, the manual save loads (no marker, PC as it was, no question), a manual save then writes |
| `autosave_power_cut_twice` (+ `_load`) | From the cut slot: a manual SAVE cut again after 10 new sectors (`arm_cut 10 signed`, past where the first cut stopped). The slot keeps its leftovers' signatures cleared and stays refused; the manual save loads (no marker, PC as it was). Fix found in review on build 0xC0170142's first suite candidate |
| `autosave_mapview` (+ `_reload`) | SAVE in the lab, then an autosave on Route 101. CONTINUE from it: the screen matches player 1 on the same tile (with the lab's stored view, the ground around the player was the lab's floor: reproduced on ROM 0006c5686410, fixed) |
| `autosave_new_game_over` | The newest slot an autosave; NEW GAME (developer start): no "This is your AUTOSAVE" question (reproduced on ROM 0006c5686410, where it was asked; fixed) |
| `autosave_off` | The setting line; OFF: no autosave; ON again: autosaves |
| `autosave_coop` | Linked: each player autosaves on entering Oldale Town while the other waits; both still linked and visible afterwards |

Not tested: the Union Room, Cable Club and facility exclusions (source-inspected); an invitation sent during a partner's autosave; the mGBA GUI by a person.
