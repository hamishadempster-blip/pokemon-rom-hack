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
// A duo Frontier battle (src/coop_frontier.c): trainerA the run id,
// trainerB the mode and opponent level, aux the battle number, badge bit 0
// a new run. Asked like a story fight, but no Beacon is needed.
#define COOP_KIND_DUO       15
#define COOP_KIND_LAST      COOP_KIND_DUO

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
#define COOP_REFUSE_PARTNER_BUSY    3   // the partner stayed in a menu screen
#define COOP_REFUSE_BEACON_OFF      4   // linked, together, but the Twin Beacon is OFF
#define COOP_REFUSE_PARTNER_DECLINED 5  // the partner answered NO to the shared fight
#define COOP_REFUSE_NO_ANSWER       6   // the partner did not answer in time
#define COOP_REFUSE_CANCELLED       7   // this player stopped waiting (B)
#define COOP_REFUSE_PREP_SELF       8   // this player stepped back during team preparation
#define COOP_REFUSE_PREP_PARTNER    9   // the partner stepped back (or stopped waiting)
#define COOP_REFUSE_PREP_TIMEOUT    10  // team preparation ran out of time
#define COOP_REFUSE_PREP_STOPPED    11  // interrupted (link, a changed party)
#define COOP_REFUSE_DUO_TEAM        12  // the partner has no three Pokemon allowed in the duo facility
#define COOP_REFUSE_DUO_OTHER_RUN   13  // the partner has a duo run with someone else
#define COOP_REFUSE_DUO_BEHIND      14  // the partner's save is at an earlier battle: continue from there
#define COOP_REFUSE_DUO_NO_RUN      15  // the partner's save has no such run (retired, or a new save)
#define COOP_REFUSE_DUO_SAME_TIME   16  // both asked for different duo runs at once: the partner's question comes next

// Invitations: how long the inviter waits for an answer and how long the
// partner's question stays open (shorter, so a YES arrives in time). Real
// seconds from the cartridge clock while it runs (so accelerated emulation
// does not shorten them), else 60 frames a second.
#define COOP_WAIT_SECONDS    30
#define COOP_ANSWER_SECONDS  26

// What this game is doing, for every request from the partner (a shared
// fight, a trade or gift, joining a conversation, following, preparing a
// team): one answer, checked when asked and again before committing.
enum {
    COOP_AVAIL_OK,
    COOP_AVAIL_BATTLE,       // in or entering a battle
    COOP_AVAIL_SCREEN,       // a menu screen: bag, party, PC, shop, summary, save, fade
    COOP_AVAIL_MESSAGE,      // a message box or a message queued to show
    COOP_AVAIL_PROMPT,       // already answering the partner's question
    COOP_AVAIL_TRANSACTION,  // a trade, gift, shared progress, or its own invitation
    COOP_AVAIL_SCENE,        // a script or cutscene has the field
};
#define COOP_PURPOSE_OPTIONAL  0  // optional fights, trades, gifts, conversations, following
#define COOP_PURPOSE_MANDATORY 1  // a shared story fight: its own waiting invitation counts as free
u8 Coop_LocalAvailability(u8 purpose);
extern u16 gCoopBusyRetries;

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
// Duo Frontier NACK reasons (badge byte; aux: the partner's battles won).
#define COOP_DUO_NACK_TEAM          0x40
#define COOP_DUO_NACK_OTHER_RUN     0x41
#define COOP_DUO_NACK_BEHIND        0x42
#define COOP_DUO_NACK_NO_RUN        0x43

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
u16 Coop_StartDuoBattle(void);
bool8 Coop_PrepDuoFields(bool8 host, u16 *runId, u16 *modeWord, u16 *battle, u8 *flags);
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
bool8 Coop_PartnerViewCoords(s16 *x, s16 *y);
bool8 Coop_PartnerLastSeen(s16 *x, s16 *y);
bool8 Coop_PartnerIsFollowing(void);
bool8 Coop_SendPing(void);
u16 Coop_PingCooldownSeconds(void);
bool8 Coop_PartnerTalkInfo(u8 *localId, bool8 *joinable);
bool8 Coop_PartnerWaiting(void);
bool8 Coop_PartnerMoveInfo(u8 *group, u8 *num, s16 *x, s16 *y, u8 *mode);
bool8 Coop_MapCoordsToLocal(u8 group, u8 num, s16 x, s16 y, s16 *outX, s16 *outY);
u8 Coop_PartnerActivity(void);
// The partner marker's icon: what the partner is doing (src/coop.c).
enum
{
    COOP_MARK_WALK,     // the order of the icons in src/data/coop_marker_icons.h
    COOP_MARK_TALK,
    COOP_MARK_MENU,
    COOP_MARK_BATTLE,
    COOP_MARK_INVITE,
    COOP_MARK_WAIT,
    COOP_MARK_BROKEN,
    COOP_MARK_NONE = 0xFF,
};
u8 Coop_PartnerMarkKind(void);
bool8 Coop_LocalAwaitingPartner(void);
#define COOP_PING_COOLDOWN 300   // five seconds between PINGs
#define COOP_PING_CUE_FRAMES 180 // a received PING's cue lasts three seconds
extern u16 gCoopPingCue;
extern u16 gCoopPingsReceived;
extern bool8 gCoopLocalWaiting;
// Follow mode and joined conversations (src/coop_partner.c, src/coop_talk.c).
bool8 CoopFollow_Active(void);
void CoopFollow_Stop(u8 reason);
u16 CoopFollow_Start(void);
void CoopFollow_FieldFrame(void);
void CoopFollow_FieldInput(u16 *newKeys, u16 *heldKeys);
void CoopFollow_NoteBattle(void);
u16 CoopPartner_Ping(void);
u16 CoopPartner_BufferBusyReason(void);
extern u8 gCoopFollowLastStop;
extern u16 gCoopFollowSteps;
extern u16 gCoopAppearanceFaults;
extern u8 gCoopAppearanceLastFault;
bool8 CoopTalk_LocalJoinable(void);
struct ScriptContext;
struct CoopTalkStats
{
    u8 hosted;          // conversations this game started
    u8 lastClass;       // how the last one was classified (0 joinable)
    u8 joinsAsked;
    u8 joinsAccepted;
    u8 checkpoints;     // host waits at movements and questions
    u8 readySent;
    u8 greetings;
    u8 busyNotes;
    u16 linesShown;     // lines presented to this game as a joiner
    u8 lastJoinEnd;     // how this game's last joined conversation ended (COOP_TALK_EV_*)
    u8 lastNote;        // and the note's reason, if one was shown
    u8 rxCount;         // talk messages received (the last eight ops below)
    u8 rxOps[8];
    u8 txCount;         // talk messages sent (the last eight ops below)
    u8 txOps[8];
    u8 busyClass;       // why the partner's last unshareable conversation was (CLASS_* in src/coop_talk.c: 1 gift, 2 shop, 3 battle, 4 warp, 5 other)
    u8 sideBySide;      // NPCs used while the partner used them too (shops, Exchange counters, gifts)
};
extern struct CoopTalkStats gCoopTalkStats;
const u8 *CoopTalk_OnInteract(u8 objectEventId, const u8 *script);
void CoopTalk_NoteText(const u8 *text);
bool8 CoopTalk_HostCheckpoint(struct ScriptContext *ctx, bool8 isQuestion);
void CoopTalk_HandleMsg(u8 op, u8 arg, u8 arg2, u16 serial, u16 total, const u8 *data);
void CoopTalk_FieldFrame(void);
void CoopTalk_Reset(void);
bool8 CoopTalk_Shared(void);
u8 CoopTalk_HostNpc(void);
u16 CoopTalk_RequestJoin(void);
void CoopTalk_WaitEvent(void);
u16 CoopTalk_BufferPartner(void);
u16 CoopTalk_PartnerJoinable(void);
void CoopTalk_ShowBusyNote(void);
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
bool8 Coop_HasBattleReadyMon(void);
u8 CoopKit_VialMax(void);
u8 CoopKit_VialCharges(void);
bool8 CoopKit_UseVialCharge(void);
void CoopKit_RefillVial(void);
u16 CoopKit_NeedsBoxLink(void);
u16 CoopKit_NeedsVial(void);
void CoopKit_MarkBoxLink(void);
void CoopKit_MarkVial(void);
const u8 *CoopKit_FieldUseBlocked(void);

// Battle start timing stages (gCoopStartTimes, frames).
enum
{
    COOP_START_QUEUED,        // the request was made (encounter, trainer, talk)
    COOP_START_ASKED,         // the partner's question opened (partner's game)
    COOP_START_ANSWERED,      // YES (partner) / the answer arrived (inviter)
    COOP_START_PREP,          // team preparation opened
    COOP_START_PREP_DONE,     // both READY, starting
    COOP_START_LAUNCH,        // the battle launch began
    COOP_START_INIT,          // CB2_InitBattle (after the transition)
    COOP_START_MAIN,          // parties exchanged, BattleMainCB2
    COOP_START_FIRST_ACTION,  // the first action menu
    COOP_START_COUNT,
};
extern u32 gCoopStartTimes[COOP_START_COUNT];
void Coop_NoteStartTime(u8 stage);

struct CoopWaitClock
{
    u32 startSecs;
    u16 frames;
    u8 rtc;        // 0 not known yet, 1 running, 2 not usable
    u8 secs;       // seconds from the clock
};
void Coop_WaitClockStart(struct CoopWaitClock *clock);
u8 Coop_WaitClockTick(struct CoopWaitClock *clock);
bool8 Coop_PrepHostCanStart(u16 session);
void Coop_PrepHostStarted(void);
void Coop_PrepHostEnd(u8 refusal);
void Coop_PrepGuestCommit(void);
void Coop_PrepSoloEnd(u8 refusal);
bool8 Coop_PrepIsHostSide(void);
bool8 Coop_PrepPartnerHere(void);
void Coop_BufferPrepOpponents(u8 *dst, bool8 host);
extern const u8 Coop_EventScript_PrepEnded[];
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
bool8 Coop_BlockMandatoryBeaconOff(u16 trainerId);
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
bool8 Coop_BattleItemsAllowed(void);
bool8 Coop_HasCatchRoom(void);
bool8 Coop_CanCatchTarget(u32 target);
void Coop_ClaimCatch(u32 thrower, u32 target);
void Coop_NoteCatch(u32 thrower, u32 target);
u16 Coop_NextCaughtNickname(void);
u16 Coop_NextPartnerCatch(void);
// 0 means the stock target. Otherwise the chosen battler plus one.
extern u8 gCoopBallTarget;
extern u8 gCoopBallTargetFor[];

// Changing a submitted battle action while the turn is open
// (src/coop_battle_turn.c).
struct CoopTurnState
{
    u8 turnId;                  // turns committed this battle (both games)
    u8 request[4];              // master: a pending change, turn + 1
    u8 waiting[4];              // this game's player: chose, waiting
    u8 asked[4];                // this game's player: asked, no answer yet
    u8 kind[4];                 // this game's player: the action chosen
    u8 lastResult[4];           // 1 changed, 2 too late (tests read it)
    u8 orders[4][3];            // master: party order at the turn's start
    u8 localAsk;
    u16 asks;
    u16 cancels;
    u16 lates;
};
extern struct CoopTurnState gCoopTurn;
void CoopTurn_Reset(void);
void CoopTurn_HostTurnStart(u32 battler);
void CoopTurn_HostProcess(u8 standbyState, u8 confirmedState, u8 beforeState);
void CoopTurn_HostCommit(void);
void CoopTurn_HandleMsg(u32 battler, const u8 *data);
void CoopTurn_NoteAction(u32 battler, u8 action);
const u8 *CoopTurn_StandbyText(u32 battler);
void CoopTurn_OnChooseAction(u32 battler);
void CoopTurn_ControllerIdle(u32 battler);
bool32 Coop_BattleSelectingActions(void);
u32 CoopEvo_PageCount(struct Pokemon *mon);
void CoopEvo_BufferHint(struct Pokemon *mon, u32 page, u8 *dest, bool32 own);
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
extern const u8 Coop_EventScript_StoryAskGuest[];
extern const u8 Coop_EventScript_JoinAskAnswer[];
void Coop_BufferStoryAsk(void);
extern const u8 Coop_EventScript_PairTooLate[];

// Co-op move target prompt (src/battle_controller_player.c).
extern bool8 gCoopTargetPromptWide;
extern u8 gCoopTargetKind;      // 1 foe, 2 ally, 3 self, 4 both foes, 5 your side, 6 all others, 7 everyone, 8 field, 9 foes' side
extern u8 gCoopTargetWarned;
extern u8 gCoopTargetPrompt[40];

// The Rustboro Trainer Exchange (src/coop_exchange.c).
struct CoopExchangeStats
{
    u8 shopsOpened;
    u8 lastCounter;
    u8 lastCount;       // items in the last stock opened
    u8 lastTier;
    u8 charges;
    u8 pad;
    u16 lastCharge;
};
extern struct CoopExchangeStats gCoopExchangeStats;
u32 CoopExchange_PriceOverride(u16 item);
u16 CoopExchange_Tier(void);
void CoopExchange_OpenShop(void);
u16 CoopExchange_CountStock(void);
u16 CoopExchange_BufferUpcoming(void);
u16 CoopExchange_BufferMegaHints(void);
void CoopExchange_RepairSavedPosition(void);
u16 CoopExchange_PushTutorTypes(void);
u16 CoopExchange_PushTutorMoves(void);
u16 CoopExchange_SelectTutorMove(void);
u16 CoopExchange_SetPrice(void);
u16 CoopExchange_Charge(void);
u16 CoopExchange_CheckEvReset(void);
void CoopExchange_ResetEvs(void);
u16 CoopExchange_CheckHyperTrain(void);
void CoopExchange_HyperTrain(void);

extern const u16 gCoopCaseStarters[3][2];

#endif // GUARD_COOP_H
