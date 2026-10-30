#ifndef GUARD_COOP_AUTOSAVE_H
#define GUARD_COOP_AUTOSAVE_H

// Autosave (src/coop_autosave.c; the slot rules are in src/save.c).
void CoopAutosave_NoteMapEntry(void);
void CoopAutosave_Poll(void);
extern u8 gCoopAutosaveCount;
extern u32 gCoopAutosaveTestInterval;
void CoopAutosave_Write(void);
void CoopAutosave_LoadManual(void);

#endif // GUARD_COOP_AUTOSAVE_H
