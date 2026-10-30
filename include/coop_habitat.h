#ifndef GUARD_COOP_HABITAT_H
#define GUARD_COOP_HABITAT_H

// Five hidden habitats (src/coop_habitat.c).
#define COOP_HABITAT_COUNT 5

s8 CoopHabitat_Here(void);
bool8 CoopHabitat_PayDue(u16 *item, u8 *quantity, u8 *why, u8 skip);
u8 CoopHabitat_CompleteMask(void);
u8 CoopHabitat_CountDue(void);
void CoopHabitat_ConfirmPaid(void);
bool8 CoopHabitat_ClueForTown(u16 mapsec, u8 *dest);
bool8 CoopHabitat_NotesPage(u8 page, u8 *dest);

#endif // GUARD_COOP_HABITAT_H
