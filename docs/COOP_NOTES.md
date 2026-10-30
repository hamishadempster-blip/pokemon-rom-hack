# FIELD NOTES and collection rewards

Section 4 of the collection update, added after build 0xC0170141.

## What the player sees

- **CO-OP > JOURNAL > FIELD NOTES** (after the six quests) shows a page each:
  - "Caught: 26 kinds of POKéMON. Next reward at 50."
  - "Visitors caught: 3 of 21 kinds. CENTER REPORTS tell where." (see [COOP_ENCOUNTERS.md](COOP_ENCOUNTERS.md))
  - "Hidden habitats found: N of 5." (see [COOP_HABITATS.md](COOP_HABITATS.md))
  - "Waiting for room in the BAG: …", when a reward is waiting.
- **Rewards are paid once per save, automatically,** at the first calm moment in the field after entering a map (as the autosave waits for one). The fanfare plays and "FIELD NOTES: 25 kinds caught! ASH got 5 Ultra Balls!".

| Reward (ledger bit) | For | Item |
| --- | --- | --- |
| 0 | 10 kinds caught | 5 Great Balls |
| 1 | 25 kinds caught | 5 Ultra Balls |
| 2 | 50 kinds caught | 3 Exp. Candy M |
| 3 | 100 kinds caught | Ability Capsule |
| 4 | 150 kinds caught | Bottle Cap |
| 5 | 200 kinds caught | Master Ball |
| 6 | 300 kinds caught | Ability Patch |
| 7 | 400 kinds caught | Shiny Charm |
| 8 | 5 visitor kinds caught | 5 Dusk Balls |
| 9 | every visitor kind caught (21) | 10 Quick Balls |

"Kinds caught" is the National Pokédex caught count, which is what the Hoenn Pokédex of this build covers too.

## Rules

- **Personal:** each save has its own ledger. Nothing is sent to the partner.
- **When it is never paid:** in a battle, menu, script, fade, co-op deal or invitation, or a facility or link room. In those places the next map entry checks again.
- **Paid once:**
  - Each reward is one bit of `CoopSaveV5.collectionClaimed`. The bit is set in the same moment the item goes into the BAG, so both reach the flash with the same save.
  - A reload before saving pays it again exactly once (neither was saved); a reload after saving pays nothing.
- **No room in the BAG:**
  - The reward is claimed and waits in `CoopSaveV5.pending` ("No room: 5 Ultra Balls wait."). It has 4 places, which habitat and town rewards will share.
  - At a later calm moment with room, it is handed over ("There's room now: ASH got the waiting 5 Ultra Balls!").
  - With all 4 places taken, a new reward is not claimed and is offered again later.
- **Damaged queue entries:** an entry with an impossible item or quantity is dropped, never paid.
- **The table is append-only:** a reward's bit is its place in `sRewards` (`src/coop_notes.c`).

## Tests (harness)

| Scenario | Checks |
| --- | --- |
| `notes_rewards` | Lab saves. FIXTURES: caught flags (`dex_caught`), the POKé BALLS pocket filled and emptied, and the map-entry check requested twice through `gCoopNotesCheck`; then real map entries. Checks: 10 kinds gives 5 Great Balls and bit 0. 25 kinds with no room: claimed, waiting (`pending[0]` = Ultra Ball ×5), "No room". After room is made, the next map entry delivers it and empties the queue. A later entry pays nothing. The FIELD NOTES pages |
| `notes_rewards_reload` | After a power cycle: the ledger and empty queue are as saved; a map entry pays nothing again |

Not tested: the larger thresholds (the same code with other numbers); the visitor rewards; four waiting rewards.
