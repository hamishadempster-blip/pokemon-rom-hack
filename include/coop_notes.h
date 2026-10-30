#ifndef GUARD_COOP_NOTES_H
#define GUARD_COOP_NOTES_H

// FIELD NOTES and collection rewards (src/coop_notes.c).
void CoopNotes_NoteMapEntry(void);
bool8 CoopNotes_Poll(void);
bool8 CoopNotes_QueueReward(u16 item, u8 quantity, u8 source);

#endif // GUARD_COOP_NOTES_H
