#ifndef GUARD_COOP_QUEST_H
#define GUARD_COOP_QUEST_H

// Quest foundations and New Mauville "Power Problems" (src/coop_quest.c).

void CoopQuest_HandleSync(u8 quest, u8 value, bool8 live);
void CoopQuest_OnSessionStart(void);
void CoopQuest_Poll(void);
bool8 CoopQuest_ApplyPowerRestored(void);

// Specials.
u16 CoopQuest_DrawNewMauville(void);
u16 CoopQuest_PressBreaker(void);
u16 CoopQuest_BreakerCount(void);
u16 CoopQuest_PowerRestored(void);
u16 CoopQuest_DuoReady(void);
u16 CoopQuest_ForceSolo(void);
u16 CoopQuest_ClaimReward(void);
u16 CoopQuest_RewardClaimed(void);
u16 CoopQuest_PagesRead(void);
u16 CoopQuest_OpenChamber(void);
u16 CoopQuest_BufferJournal(void);

#endif // GUARD_COOP_QUEST_H
