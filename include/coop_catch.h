#ifndef GUARD_COOP_CATCH_H
#define GUARD_COOP_CATCH_H

// Catch result summary (src/coop_catch.c).
void CoopCatch_BufferSummary(u8 *dst, struct Pokemon *mon, bool8 isNew);
bool8 CoopCatch_BufferBattleResult(struct Pokemon *mon);

#endif // GUARD_COOP_CATCH_H
