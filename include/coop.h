#ifndef GUARD_COOP_H
#define GUARD_COOP_H

#include "global.h"
#include "coop_protocol.h"
#include "constants/coop.h"

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
// The rival twin standing beside the story's rival. Spawned in code.
#define OBJ_EVENT_ID_COOP_TWIN 0xF6

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
#define COOP_KIND_WILD      COOP_KIND_WILD_VALUE
#define COOP_KIND_PAIR      11
// Birch's Route 101 rescue. Both players already have a starter.
#define COOP_KIND_RESCUE    12
// Story bosses. The badge byte is the gate, not a gym number.
#define COOP_KIND_STORY     13
// The inviter gave up on an optional invite; the partner's open question
// for that sequence number becomes too late.
#define COOP_KIND_WITHDRAW  14

#define COOP_GATE_CHIMNEY  1
#define COOP_GATE_MAGMA    2
#define COOP_GATE_SEAFLOOR 3
#define COOP_GATE_SPACE    4
#define COOP_GATE_WEATHER  5
#define COOP_GATE_AQUA     6

#define COOP_RESCUE_RETURN_NONE  0
#define COOP_RESCUE_RETURN_HOST  1
#define COOP_RESCUE_RETURN_GUEST 2

#define COOP_REFUSE_NONE            0
#define COOP_REFUSE_SELF_NOT_READY  1
#define COOP_REFUSE_PARTNER_NOT_READY 2

// Why an ordinary fight stayed solo while linked with the Beacon on. Sent in
// a NACK's badge byte and shown under the battle's first message.
#define COOP_SOLO_NONE              0
#define COOP_SOLO_ELSEWHERE         1
#define COOP_SOLO_BUSY              2
#define COOP_SOLO_NO_ANSWER         3
#define COOP_SOLO_DECLINED          4
#define COOP_SOLO_PARTNER_NOT_READY 5
#define COOP_SOLO_SELF_NOT_READY    6
#define COOP_SOLO_RECOVERING        7
#define COOP_SOLO_PLACE             8
#define COOP_SOLO_KIND              9
#define COOP_SOLO_LEGEND_NOT_ELIGIBLE 10   // a shared legendary the partner's save cannot take part in
#define COOP_SOLO_COUNT             11

struct Pokemon;

void Coop_RecordLevelUp(u8 slot, u8 oldLevel);
void Coop_RestoreFieldPalettes(void);
u16 Coop_NextProgression(void);
void Coop_StartProgressionEvolution(void);

extern bool8 gCoopBattleLaunching;

// Finding #16 capture: a trainer battle that was about to launch with an
// opponent that cannot be built. Refused (never replaced by a stand-in) and
// recorded here for the harness and for a field report.
#define COOP_BADTR_START   1 // BattleSetup_StartTrainerBattle
#define COOP_BADTR_RESUME  2 // BattleSetup_ResumeSoloTrainerBattle (after an invite)
#define COOP_BADTR_INVITE  3 // a received invite named no valid trainer
struct CoopDiag
{
    u16 badTrainer;      // refusals so far this power-on
    u16 lastA;
    u16 lastB;
    u8 site;             // COOP_BADTR_*
    u8 approaching;      // gNoOfApproachingTrainers at the time
    u8 kind;             // co-op battle kind then
    u8 flags;            // bit0 inBattle, bit1 waitingAck, bit2 guestPairPending, bit3 script on
    u16 selectedObj;     // gSelectedObjectEvent
    u32 frame;           // gMain.vblankCounter1
    const u8 *script;    // trainer script pointer of the first approaching trainer
};
extern struct CoopDiag gCoopDiag;
void Coop_NoteBadTrainer(u8 site, u16 trainerA, u16 trainerB);
extern bool8 gCoopArmWild;
// Set while a co-op session should survive a one-off link glitch, such as a door.
extern bool8 gCoopHoldLink;
extern bool8 gCoopForceSolo;
// A friendly battle between the two partners (src/coop_deal.c).
extern bool8 gCoopFriendlyBattle;
// Battles that borrow the co-op cable and hand it back open afterwards.
#define COOP_SESSION_LINK_BATTLE() ((gBattleTypeFlags & BATTLE_TYPE_COOP) || gCoopFriendlyBattle)
void CoopDeal_AbortFriendlyBattle(void);

u16 Coop_PartnerOnSameMap(void);
u16 Coop_PartnerOnSameTile(void);
u16 Coop_SessionActive(void);
u16 Coop_CableUp(void);
u16 Coop_IsLevelCapOn(void);
u32 Coop_DisplayedLevelCap(void);
u16 Coop_CaseChosen(void);
u16 Coop_StartGymBattle(void);
u16 Coop_StartStoryBattle(void);
u16 Coop_ApplyGuestStory(void);
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
u16 Coop_PartnerChatOk(void);
const u8 *Coop_TakeSoloNote(void);
void Coop_NoteSoloOnlyEncounter(void);
// GetBlockReceivedStatus for link battle code: co-op blocks (invites,
// replies, story, deal and dex messages) that arrive during a fight are
// taken out of the way first, so they are never read as battle data.
u8 Coop_BattleBlockStatus(void);
// Test mailbox only: sends a reply block now; FALSE while the link is busy.
bool8 Coop_TestSendReply(u8 kind, u16 seq);
u16 Coop_BufferMenuExplain(void);
u16 Coop_TwinLineStage(void);
u16 Coop_ResetRivalScene(void);
u16 Nurse_RepeatVisit(void);
u16 Coop_MandatoryGate(void);
u16 Coop_SoloRivalTrainer(u8 stage);
void Coop_NoteSoloTrainerWin(void);
u16 Coop_StartSoloRescueBattle(void);
u8 Coop_PeekSoloNote(void);
void Coop_ClearSoloNote(void);
void Nurse_NoteHeal(void);
bool8 Coop_IsRivalGraphics(u16 gfx);
u8 Coop_PartnerSpriteId(u8 *gender);
bool8 Coop_PartnerMapPos(s16 *x, s16 *y);
bool8 Coop_PartnerFrameInfo(u8 *mapGroup, u8 *mapNum, u8 *activity);
void Coop_NoteCoordScene(s16 x, s16 y);
u16 Coop_WarpToCoordScene(void);
const u8 *GetCoordEventScriptAt(s16 x, s16 y, u8 elevation);
u16 Coop_BufferCapText(void);
u16 Coop_BufferUnlinkedMenu(void);
u16 Coop_BufferUnlinkName(void);
u16 Coop_BufferBeaconStatus(void);
u16 Coop_BufferBeaconAsk(void);
u16 Coop_PrepareTwinBeacon(void);
u16 Coop_BeaconAnswer(void);
u16 Coop_TakeGuestFollowup(void);
u16 Coop_TakeRefusal(void);
bool8 Coop_HasBattleReadyLead(void);
const u8 *Coop_UseTwinBeacon(void);
bool8 Coop_BeaconActive(void);

void Coop_OverworldFrame(void);
void Coop_ApplyPendingStory(void);
u16 Coop_AnnounceScene(void);
u16 Coop_PrepareRivalJoin(void);
u16 Coop_BufferRival(void);
void Coop_UpdateRivalTwin(void);
void Coop_FormatStatus(u8 *dest);
u16 Coop_BufferMenuStatus(void);
u16 Coop_MenuUseBeacon(void);
u16 Coop_ReopenStartMenu(void);
u16 Coop_TwinIdentity(void);
u16 Coop_BufferRivalTaunt(void);
extern const u8 Coop_EventScript_TalkTwin[];
void Coop_ResumeAfterTasksReset(void);
void Coop_YieldLinkToBattle(void);
void Coop_SettlePartner(void);
void Coop_LinkCallback(void);
void Coop_OnRecv(u32 playerId);
bool8 Coop_TryParseHeldKeys(u32 playerId);
bool8 Coop_FieldLinkCatchUp(void);
bool8 Coop_ShouldRefuseSolo(u16 trainerId);
bool8 Coop_BlockUnreadyMandatory(u16 trainerId);
bool8 Coop_InterceptRival(u16 trainerId);
bool8 Coop_TryQueueOrdinaryDouble(void);
void Coop_OnBattleLaunched(void);
bool8 Coop_BattleGuest(void);
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
u16 Coop_NextPartnerCatch(void);
// 0 means the stock target. Otherwise the chosen battler plus one.
extern u8 gCoopBallTarget;
extern u8 gCoopBallTargetFor[];
extern u16 gCoopLastBall;

// Stage 5: one optional trainer pair.
bool8 Coop_TryStartTrainerPair(void);
u16 Coop_PairHostAnswer(void);
u16 Coop_PairGuestAnswer(void);
u16 Coop_BufferJoinAsk(void);
u16 Coop_BufferPairNames(void);

extern const u8 Coop_EventScript_NeedPartner[];
extern const u8 Coop_EventScript_RivalWaits[];
extern const u8 Coop_EventScript_Menu[];
extern const u8 Coop_EventScript_TalkPartner[];
extern const u8 Coop_EventScript_WarpToLab[];
extern const u8 Coop_EventScript_ChooseCase[];
extern const u8 Coop_EventScript_Disconnected[];
extern const u8 Coop_EventScript_PartnerUnlinked[];
extern const u8 Coop_EventScript_BuildMismatch[];
extern const u8 Coop_EventScript_CapMismatch[];
extern const u8 Coop_EventScript_LegendConflict[];
extern const u8 Coop_EventScript_BattleAborted[];
extern const u8 Coop_EventScript_TrainerRefused[];
extern const u8 Coop_EventScript_TradeDone[];
extern const u8 Coop_EventScript_PartnerDeclined[];
extern const u8 Coop_EventScript_LegendAskGuest[];
extern const u8 Coop_EventScript_LegendDeliver[];
extern const u8 Coop_EventScript_LegendNoRoom[];
extern const u8 Coop_EventScript_LegendAckNote[];
extern const u8 Coop_EventScript_LegendGuestAway[];
extern const u8 Coop_EventScript_NotReady[];
extern const u8 Coop_EventScript_GuestAfterBattle[];
extern const u8 Coop_EventScript_BeaconAsk[];
extern const u8 Coop_EventScript_BeaconOn[];
extern const u8 Coop_EventScript_BeaconOff[];
extern const u8 Coop_EventScript_BeaconDeclined[];
extern const u8 Coop_EventScript_CaughtNickname[];
extern const u8 Coop_EventScript_PairAskHost[];
extern const u8 Coop_EventScript_PairAskGuest[];
extern const u8 Coop_EventScript_JoinAskGuest[];
extern const u8 Coop_EventScript_PairTooLate[];

#endif // GUARD_COOP_H
