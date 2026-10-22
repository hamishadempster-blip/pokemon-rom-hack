#ifndef GUARD_COOP_H
#define GUARD_COOP_H

#include "global.h"

// Megas, Z-Moves, Dynamax, and Terastallization (including Primal Reversion)
// are turned off at the Can* checks. Species tables stay intact.
#define COOP_GIMMICKS_OFF 1

// Each human brings a party of 6. Stock link multis send 3. The battle start
// path sends the extra Pokémon in two-Pokémon blocks. If that path has to be
// reverted, set this to 3. Enemy gyms stay 6 either way.
#define COOP_PARTY_SIZE 6

// 0 sends LINKCMD_COOP. 1 packs the same payload into LINKCMD_SEND_HELD_KEYS
// for a core that drops unknown commands. Both layouts are accepted.
#define COOP_SEND_AS_HELD_KEYS 0

#define OBJ_EVENT_ID_COOP_PARTNER 0xFC

#define COOP_KIND_GYM       1
#define COOP_KIND_RIVAL     2
#define COOP_KIND_E4        3
#define COOP_KIND_CHAMPION  4
#define COOP_KIND_LEGEND    5
#define COOP_KIND_TRAINER   6
#define COOP_KIND_WARP      7
#define COOP_KIND_ACK       8

#define COOP_LEGEND_DUO 0
#define COOP_LEGEND_RAY 1

struct Pokemon;

extern bool8 gCoopBattleLaunching;
extern bool8 gCoopArmWild;

u16 Coop_PartnerOnSameMap(void);
u16 Coop_PartnerOnSameTile(void);
u16 Coop_SessionActive(void);
u16 Coop_IsLevelCapOn(void);
u32 Coop_DisplayedLevelCap(void);
u16 Coop_CaseChosen(void);
u16 Coop_StartGymBattle(void);
u16 Coop_StartLegendaryBattle(void);
u16 Coop_BeginLink(void);
u16 Coop_Disconnect(void);
u16 Coop_ToggleLevelCap(void);
u16 Coop_ProposeCase(void);
u16 Coop_ClaimStarter(void);
u16 Coop_RequestLabWarp(void);
u16 Coop_BufferPartnerLocation(void);
u16 Coop_BufferCapText(void);
bool8 Coop_OfferTrade(u8 slot);

void Coop_OverworldFrame(void);
void Coop_LinkCallback(void);
void Coop_OnRecv(u32 playerId);
bool8 Coop_TryParseHeldKeys(u32 playerId);
bool8 Coop_ShouldRefuseSolo(u16 trainerId);
bool8 Coop_InterceptRival(u16 trainerId);
bool8 Coop_TryQueueOrdinaryDouble(void);
void Coop_OnBattleLaunched(void);
bool8 Coop_FinishTrainerBattle(void);
bool8 Coop_FinishWildBattle(void);
void Coop_AbortBattle(void);
bool8 Coop_CommitCatch(struct Pokemon *caughtMon);

extern const u8 Coop_EventScript_NeedPartner[];
extern const u8 Coop_EventScript_Menu[];
extern const u8 Coop_EventScript_TalkPartner[];
extern const u8 Coop_EventScript_WarpToLab[];
extern const u8 Coop_EventScript_ChooseCase[];
extern const u8 Coop_EventScript_Disconnected[];
extern const u8 Coop_EventScript_BuildMismatch[];
extern const u8 Coop_EventScript_CapMismatch[];
extern const u8 Coop_EventScript_LegendConflict[];
extern const u8 Coop_EventScript_BattleAborted[];
extern const u8 Coop_EventScript_TradeDone[];
extern const u8 Coop_EventScript_PartnerDeclined[];

#endif // GUARD_COOP_H
