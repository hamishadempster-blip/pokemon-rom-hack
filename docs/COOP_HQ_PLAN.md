# Expedition HQ: plan (not implemented)

Section 9 of the collection update. **Nothing of the HQ is in the game yet.** Its 256 bytes of `CoopSaveV5.hq` are reserved and zero, so it can be added later without a save-format change ([COOP_STORAGE_PLAN.md](COOP_STORAGE_PLAN.md)). This page records the design worked out so far, so the next stage starts from it.

## Why it was not started in this round

The other sections of this round (autosave, waiting at a Center, encounter variety, "Who can learn this?", FIELD NOTES and rewards, hidden habitats, town specialities) each stand alone on one save. The HQ's core is a **shared** room for two linked players, so it needs:

- a new kind of transaction, with request, accept, commit and recovery records, for every change both games must agree on;
- a rule for which game owns the room (the host's? both?) and what happens when the partner is offline or a different partner links;
- care with Secret Bases, whose stock data is mixed between saves by record mixing.

Doing that halfway would risk the save and the link. It is better done as its own stage with its own tests.

## Staged design

1. **Opt-in upgrade.** At your own Secret Base's PC: EXPEDITION HQ → "Make this base your HQ?". It only sets a flag and records the date. Turning it off keeps everything. Without the upgrade the Secret Base behaves exactly as now.
2. **Personal corner (one save, no link).**
   - Your existing base decorations stay yours.
   - A terminal shows your expedition record: the FIELD NOTES totals, habitats found and complete, town tracks, Trainer Bond rank, visitors caught.
   - A photo wall shows your partner as last linked: name, trainer id, the date first linked, joint badges. The trainer id is already recorded with joint outcomes.
3. **Residents (one save).** Up to three Pokémon chosen from your PC appear walking in the HQ, by personality and trainer id, never copies. They leave if released or traded. The aquarium is the same idea, for Water types, behind glass.
4. **Shared main room (linked).**
   - The room's layout is the HOST's.
   - The guest sees it while linked, sent as a checked packet with the build id, session and a checksum.
   - Each player can change only their own corner. Changes to shared furniture need a request and accept, then are committed on both saves with a revision number. A crash during a commit is settled by the revision on the next link, as deals are settled now.
   - A different partner sees only the host's own corner.
5. **Services.** A daily expedition order: catch a kind of Pokémon, visit a habitat. It is chosen from what the save can reach, so it never asks for something impossible, and pays once per day per save, through the FIELD NOTES ledger rules.

## Storage budget (within the reserved 256 bytes)

| Part | Bytes |
| --- | --- |
| Flags, upgrade date, revision | 8 |
| Partnership identity (trainer id, name, first-link day) | 16 |
| Photo wall records | 32 |
| Residents (3 × personality + trainer id) and aquarium (3) | 48 |
| Shared-room layout (16 furniture slots × 4) | 64 |
| Daily orders and ledger | 16 |
| Reserve | 72 |

## Tests the stages will need

- Opt-in, then a power cut during the upgrade save.
- Residents surviving release, trade and PC moves.
- Shared room:
  - both roles;
  - simultaneous edits;
  - a link drop mid-commit;
  - relinking with the same partner and with a different one;
  - a partner on an older build (refused).
- Reloads before and after each commit.
