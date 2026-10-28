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
bool8 CoopQuest_IsRewardClaimed(u8 bit);
u16 CoopQuest_RewardClaimed(void);
u16 CoopQuest_PagesRead(void);
u16 CoopQuest_OpenChamber(void);
u16 CoopQuest_BufferJournal(void);

// Optional quests (shared rules: src/coop_quest.c).
struct WildPokemonInfo;
void CoopQuest_ApplyFoundObjects(void);
u16 CoopQuest_LocalMilestones(void);
bool8 CoopQuest_ApplyMilestone(u8 id);
const struct WildPokemonInfo *CoopQuest_FishingOverride(void);
u16 CoopQuest_SetBits(void);
u16 CoopQuest_CountBits(void);
u16 CoopQuest_HasBits(void);
u16 CoopQuest_PressGlyph(void);
u16 CoopQuest_TwinHas(void);
u16 CoopQuest_TwinUpdateFlags(void);
u16 CoopQuest_TrialHeal(void);
u16 CoopQuest_TrialSoloBattle(void);
u16 CoopQuest_ShareGates(void);
u16 CoopQuest_MilestoneKnown(void);
u16 CoopQuest_TwinState(void);
u16 CoopQuest_TwinMark(void);
u16 CoopQuest_PlatesHeld(void);
u16 CoopQuest_LightLantern(void);
u16 CoopQuest_DrawScorchedSlab(void);

#endif // GUARD_COOP_QUEST_H
