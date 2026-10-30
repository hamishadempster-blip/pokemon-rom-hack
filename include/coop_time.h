#ifndef GUARD_COOP_TIME_H
#define GUARD_COOP_TIME_H

// Waiting at a Pokémon Center for another time of day (src/coop_time.c).
#define COOP_WAIT_REAL_TIME 4   // VAR_0x8006 / deal rules: back to the clock's own time

void CoopTime_AdjustClock(s32 *hours, s32 *minutes);
bool8 CoopTime_InCentre(void);
bool8 CoopTime_TargetValid(u8 target);
void CoopTime_BufferTarget(u8 target, u8 *dest);
u16 CoopTime_Shift(void);

#endif // GUARD_COOP_TIME_H
