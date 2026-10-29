#ifndef GUARD_COOP_FRONTIER_H
#define GUARD_COOP_FRONTIER_H

// Duo Frontier (src/coop_frontier.c): Battle Frontier challenges for two
// linked players. Build 0xC017013E has the DUO TOWER; the other facilities'
// duo desks say plainly that their duo mode is not available.

struct Trainer;
struct Pokemon;

extern bool8 gCoopDuoBattle;

// A duo battle is being fought (or launched) on this game.
bool8 CoopDuo_InBattle(void);
// The generated trainer for TRAINER_DUO_A / _B while a duo battle is set up.
const struct Trainer *CoopDuo_TrainerStruct(u16 trainerId);
// Builds the party of TRAINER_DUO_A / _B; FALSE for any other trainer.
bool8 CoopDuo_CreateParty(struct Pokemon *party, u16 trainerId);

// Invite fields (src/coop.c).
u16 CoopDuo_InviteRunId(void);
u16 CoopDuo_InviteModeWord(void);
u16 CoopDuo_InviteBattle(void);
u8 CoopDuo_InviteFlags(void);
// The partner's duo invite, checked on this save: 0 to ask the player,
// else a COOP_DUO_NACK_* reason (*aux: this save's battles won).
u8 CoopDuo_GuestCheck(u16 runId, u16 modeWord, u16 battle, u8 flags, u16 *aux);
// A duo battle is starting on this game (host or guest).
void CoopDuo_BeginBattle(u16 runId, u16 modeWord, u16 battle, u8 flags);
// The partner refused: adopt its round (BEHIND) and remember why.
void CoopDuo_OnPartnerRefused(u8 reason, u16 aux);
// After the battle (won, lost) or an aborted launch.
void CoopDuo_FinishBattle(u8 outcome);
void CoopDuo_AbortBattle(void);
void CoopDuo_BufferAskText(u16 battle, u16 modeWord, u8 flags);
void CoopDuo_BufferPrepTitle(u8 *dst);

// Party copies for the battle (CoopPrep_OnLaunch) and the real party after.
void CoopDuo_BuildBattleParty(const u8 *order, u8 count, u8 partnerMaxLevel);
bool8 CoopDuo_RestoreRealParty(void);

// Team preparation rules (src/coop_prep.c).
void CoopDuo_SetPrepContext(bool8 duo, u16 runId, u16 modeWord, u16 battle, u8 flags);
bool8 CoopDuo_PrepActive(void);
bool8 CoopDuo_PrepTeamFixed(void);
bool8 CoopDuo_MonAllowed(struct Pokemon *mon);
// 0 valid; else 1 not three, 2 banned, 3 same species twice, 4 same item twice
u8 CoopDuo_CheckOwnTeam(const u8 *order, u8 count);
// 0 valid; else 1 same species, 2 same item (STR_VAR_1: which)
u8 CoopDuo_CheckCombined(const u16 *mySpecies, const u16 *myItems, const u16 *theirSpecies, const u16 *theirItems);
// The run team's party slots (fixed rounds); FALSE if one is missing.
bool8 CoopDuo_FixedTeamSlots(u8 *order);

// Test visibility.
extern u16 gCoopDuoTrainerIds[2];
extern u16 gCoopDuoMonIds[6];
extern u8 gCoopDuoLastOutcome;
extern u8 gCoopDuoLastBp;

#endif // GUARD_COOP_FRONTIER_H
