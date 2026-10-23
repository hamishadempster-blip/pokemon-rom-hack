#ifndef GUARD_COOP_H
#define GUARD_COOP_H

#include "global.h"
#include "coop_protocol.h"

// Megas, Z-Moves, Dynamax, and Terastallization (including Primal Reversion)
// are turned off at the Can* checks. Species tables stay intact.
#define COOP_GIMMICKS_OFF 1

// Each human fields party slots 1-3 (COOP_BATTLE_SLOTS) in a co-op battle,
// through the stock link multi exchange. Slots 4-6 leave the party for the
// battle and come back untouched. Solo battles and enemy parties keep six.

// 0 sends LINKCMD_COOP. 1 packs the same payload into LINKCMD_SEND_HELD_KEYS
// for a core that drops unknown commands. Both layouts are accepted.
#define COOP_SEND_AS_HELD_KEYS 0

#define OBJ_EVENT_ID_COOP_PARTNER 0xFC

extern const u8 Coop_EventScript_TalkPartner[];

#define COOP_KIND_GYM       1
#define COOP_KIND_RIVAL     2
#define COOP_KIND_E4        3
#define COOP_KIND_CHAMPION  4
#define COOP_KIND_LEGEND    5
#define COOP_KIND_TRAINER   6
#define COOP_KIND_WARP      7
#define COOP_KIND_ACK       8
#define COOP_KIND_NACK      9
#define COOP_KIND_WILD      10
#define COOP_KIND_PAIR      11
// Birch's Route 101 rescue. Both players already have a starter.
#define COOP_KIND_RESCUE    12

#define COOP_RESCUE_RETURN_NONE  0
#define COOP_RESCUE_RETURN_HOST  1
#define COOP_RESCUE_RETURN_GUEST 2

#define COOP_REFUSE_NONE            0
#define COOP_REFUSE_SELF_NOT_READY  1
#define COOP_REFUSE_PARTNER_NOT_READY 2

#define COOP_LEGEND_DUO 0
#define COOP_LEGEND_RAY 1

struct Pokemon;

extern bool8 gCoopBattleLaunching;
extern bool8 gCoopArmWild;
// Set while a co-op session should survive a one-off link glitch, such as a door.
extern bool8 gCoopHoldLink;

u16 Coop_PartnerOnSameMap(void);
u16 Coop_PartnerOnSameTile(void);
u16 Coop_SessionActive(void);
u16 Coop_IsLevelCapOn(void);
u32 Coop_DisplayedLevelCap(void);
u16 Coop_CaseChosen(void);
u16 Coop_StartGymBattle(void);
u16 Coop_StartLegendaryBattle(void);
u16 Coop_BeginLink(void);
u16 Coop_ArmLinkWait(void);
u16 Coop_BirchBagFree(void);
u16 Coop_Disconnect(void);
u16 Coop_ToggleLevelCap(void);
u16 Coop_ProposeCase(void);
u16 Coop_ClaimStarter(void);
u16 Coop_RequestLabWarp(void);
u16 Coop_StartRescueBattle(void);
u16 Coop_TakeRescueResult(void);
u8 Coop_RescueReturn(void);
u16 Coop_BufferPartnerLocation(void);
u16 Coop_BufferCapText(void);
u16 Coop_BufferBeaconStatus(void);
u16 Coop_BufferBeaconAsk(void);
u16 Coop_PrepareTwinBeacon(void);
u16 Coop_BeaconAnswer(void);
u16 Coop_TakeGuestFollowup(void);
u16 Coop_TakeRefusal(void);
bool8 Coop_HasBattleReadyLead(void);
const u8 *Coop_UseTwinBeacon(void);
bool8 Coop_BeaconActive(void);
bool8 Coop_OfferTrade(u8 slot);

void Coop_OverworldFrame(void);
void Coop_ResumeAfterTasksReset(void);
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

// Stage 4: one-route wild catch.
bool8 Coop_WildPairReady(void);
void Coop_QueueWildPair(void);
bool8 Coop_IsWildBattle(void);
bool8 Coop_HasCatchRoom(void);
bool8 Coop_CanCatchTarget(u32 target);
void Coop_ClaimCatch(u32 thrower, u32 target);
void Coop_NoteCatch(u32 thrower, u32 target);
u16 Coop_NextCaughtNickname(void);
// 0 means the stock target. Otherwise the chosen battler plus one.
extern u8 gCoopBallTarget;
extern u8 gCoopBallTargetFor[];
extern u16 gCoopLastBall;

// Stage 5: one optional trainer pair.
bool8 Coop_TryStartTrainerPair(void);
u16 Coop_PairHostAnswer(void);
u16 Coop_PairGuestAnswer(void);
u16 Coop_BufferPairNames(void);

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
extern const u8 Coop_EventScript_NotReady[];
extern const u8 Coop_EventScript_GuestAfterBattle[];
extern const u8 Coop_EventScript_BeaconAsk[];
extern const u8 Coop_EventScript_BeaconOn[];
extern const u8 Coop_EventScript_BeaconOff[];
extern const u8 Coop_EventScript_BeaconDeclined[];
extern const u8 Coop_EventScript_CaughtNickname[];
extern const u8 Coop_EventScript_PairAskHost[];
extern const u8 Coop_EventScript_PairAskGuest[];
extern const u8 Coop_EventScript_PairTooLate[];

#endif // GUARD_COOP_H
