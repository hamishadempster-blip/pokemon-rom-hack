#include "global.h"
#include "coop_bond.h"
#include "coop.h"
#include "coop_prep.h"
#include "coop_frontier.h"
#include "coop_teams.h"
#include "coop_npc.h"
#include "coop_protocol.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "trainer_see.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "constants/region_map_sections.h"
#include "constants/battle_pyramid.h"
#include "caps.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "item.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_message_box.h"
#include "trainer_hill.h"
#include "constants/layouts.h"
#include "constants/trainer_hill.h"
#include "link.h"
#include "main.h"
#include "union_room.h"
#include "overworld.h"
#include "field_weather.h"
#include "palette.h"
#include "pokeball.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "region_map.h"
#include "script.h"
#include "script_menu.h"
#include "start_menu.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "gpu_regs.h"
#include "menu.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/global.h"
#include "constants/opponents.h"
#include "constants/party_menu.h"
#include "constants/pokedex.h"
#include "constants/rgb.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/trainer_types.h"
#include "constants/vars.h"
#include "constants/coop.h"
#include "constants/maps.h"
#include "data.h"
#include "money.h"
#include "map_name_popup.h"
#include "field_effect_helpers.h"
#include "constants/field_effects.h"
#include "field_effect.h"
#include "coop_deal.h"
#include "sound.h"
#include "constants/songs.h"
#include "coop_legend.h"
#include "coop_outfit.h"
#include "coop_quest.h"
#include "rtc.h"
#include "constants/rtc.h"
#include "text_window.h"

#define COOP_INVITE_MAGIC 0xC0A1
#define COOP_SAVE_MAGIC   0xC0
#define COOP_ACK_TIMEOUT  600
// A wild encounter has the screen frozen while it waits, and the guest
// answers it without a prompt. The pair prompt waits on a human, so the
// guest stops accepting a yes well before the host gives up.
// Wild and ordinary-trainer invites now wait on a person: the partner sees
// a YES/NO. The partner's answer must arrive before the inviter gives up.
// (Wild, trainer, pair, shared-legend and story-fight invitations now wait
// COOP_WAIT_SECONDS of real time, include/coop.h; these frame counts are
// only the rescue's and the CO-OP menu's.)
#define COOP_PAIR_TIMEOUT 1800
// The rescue waits on a person who may still be closing a menu.
#define COOP_RESCUE_TIMEOUT 1800
// The CO-OP menu holds on the waiting text for this long. Both players
// need time to say yes. 30 seconds at 60 frames.
#define COOP_LINK_WAIT_LIMIT 1800
#define COOP_CASE_NONE    0xFF
// A ledger is re-sent this often while linked, so a dropped block heals.
#define COOP_STORY_RESEND 300

// Rival ids sit in a contiguous block: stage * 6 + person * 3 + type.
// Type order is fire, water, grass. Starter var order is grass, fire, water,
// and that lines up with the type that beats the player's case.
// 865 is TRAINER_PARTNER(PARTNER_STEVEN). Rival ids start after that hole.
#define COOP_RIVAL_BASE 866

struct CoopInvite
{
    u16 magic;
    u8 kind;
    u8 badge;
    u16 trainerA;
    u16 trainerB;
    u16 aux;
    u16 seq;
    u16 checksum;
    u16 teamKey;   // Coop_TeamKeyLocal of the sender (invite and every answer)
};

enum
{
    COOP_LINK_IDLE,
    COOP_LINK_CONNECTING,
    COOP_LINK_UP,
};

EWRAM_DATA bool8 gCoopBattleLaunching = FALSE;
EWRAM_DATA bool8 gCoopArmWild = FALSE;
EWRAM_DATA bool8 gCoopHoldLink = FALSE;
EWRAM_DATA struct CoopDiag gCoopDiag = {0};
EWRAM_DATA bool8 gCoopForceSolo = FALSE; // next trainer battle stays solo (coop_quest.c)

static EWRAM_DATA u8 sLinkState = COOP_LINK_IDLE;
static EWRAM_DATA u8 sTaskId = 0;
static EWRAM_DATA u8 sHandshakeSaved = FALSE;
static EWRAM_DATA u8 sLinkPhase = 0;
static EWRAM_DATA u16 sLinkPhaseTimer = 0;
static EWRAM_DATA bool8 sLinkWait = FALSE;
static EWRAM_DATA u16 sLinkWaitTimer = 0;
static EWRAM_DATA u8 sLinkPlayersSeen = 0;
static EWRAM_DATA u8 sLinkStatusCount = 0;
static EWRAM_DATA u16 sLinkErrorStreak = 0;
static EWRAM_DATA u8 sUnlinkTimer = 0;
static EWRAM_DATA u8 sBadFrameStreak = 0;
static EWRAM_DATA bool8 sBirchBag = FALSE;
static EWRAM_DATA u8 sRescueField = COOP_RESCUE_RETURN_NONE;
static EWRAM_DATA u8 sRescueResult = 0;
static EWRAM_DATA bool8 sRescueGuestFollow = FALSE;
// Both games still echo the rescue request when they come back from the
// fight. Without this latch that stale echo starts the fight a second time.
static EWRAM_DATA bool8 sRescueDone = FALSE;
static EWRAM_DATA u16 sRescueGuestTimer = 0;
static EWRAM_DATA u8 sTalkNpc = 0;
static EWRAM_DATA u8 sTalkNpcFacing = 0;
static EWRAM_DATA u8 sTalkNpcMapGroup = 0;
static EWRAM_DATA u8 sTalkNpcMapNum = 0;
static EWRAM_DATA u8 sMerged = FALSE;
static EWRAM_DATA u32 sSessionPartnerOt = 0;  // trainer id of this session's partner
static EWRAM_DATA u8 sSeq = 0;
static EWRAM_DATA u8 sRequest = 0;
static EWRAM_DATA u8 sSlot = 0;
static EWRAM_DATA u8 sPartnerValid = FALSE;
static EWRAM_DATA struct CoopFrame sPartner = {0};
static EWRAM_DATA u16 sSpriteGfx = OBJ_EVENT_GFX_BRENDAN_NORMAL;
static EWRAM_DATA u8 sPartnerStepWait = 0;
// The partner was drawn last frame, and on which of their maps.
static EWRAM_DATA bool8 sPartnerDrawn = FALSE;
static EWRAM_DATA u8 sPartnerDrawnGroup = 0;
static EWRAM_DATA u8 sPartnerDrawnNum = 0;
static EWRAM_DATA bool8 sPartnerResettle = FALSE;
static EWRAM_DATA bool8 sIsGuest = FALSE;
static EWRAM_DATA bool8 sInBattle = FALSE;
static EWRAM_DATA bool8 sAborting = FALSE;
static EWRAM_DATA bool8 sHasSnapshot = FALSE;
static EWRAM_DATA bool8 sResumeScript = FALSE;
static EWRAM_DATA bool8 sWaitingAck = FALSE;
static EWRAM_DATA bool8 sInviteSent = FALSE;
static EWRAM_DATA u16 sInviteTimer = 0;
// Frames before a mandatory invite is sent again to a partner who was busy
// in a menu screen; gCoopBusyRetries counts those (tests).
static EWRAM_DATA u8 sInviteRetryDelay = 0;
EWRAM_DATA u16 gCoopBusyRetries = 0;
#define COOP_BUSY_RETRY_FRAMES 30
// A partner who answers "busy" (a menu, a message) gets COOP_WAIT_SECONDS
// in all to become free (sBusyClock, from the first busy answer); once free
// and actually asked, the full COOP_WAIT_SECONDS to answer (sInviteClock
// restarts at every busy answer). sPartnerBusyNow: a busy answer came in
// the last couple of seconds.
static EWRAM_DATA bool8 sPartnerBusyNow = FALSE;
static EWRAM_DATA u16 sInviteSeq = 0;
static EWRAM_DATA u8 sBattleKind = 0;
static EWRAM_DATA u8 sBattleBadge = 0;
static EWRAM_DATA u16 sBattleFlag = 0;
static EWRAM_DATA u8 sLegendKind = 0;
static EWRAM_DATA u8 sRivalFightStage = 0;
static EWRAM_DATA u8 sProposedCase = 0;
static EWRAM_DATA u8 sClaim = 0;
static EWRAM_DATA u8 sWaitKind = 0;
static EWRAM_DATA u8 sRescueEcho = 0;
static EWRAM_DATA struct Pokemon sPartySnap[PARTY_SIZE] = {0};
static EWRAM_DATA struct CoopInvite sInvite = {0};
static EWRAM_DATA const u8 *sPendingMessage = NULL;
// Local level-up work survives battle resource cleanup, but never an abort.
static EWRAM_DATA u8 sProgressLevels[COOP_BATTLE_SLOTS] = {0};
static EWRAM_DATA u16 sProgressMoves[COOP_BATTLE_SLOTS] = {0};
static EWRAM_DATA bool8 sProgressGuest = FALSE;
static EWRAM_DATA bool8 sProgressPending = FALSE;
static EWRAM_DATA u8 sProgressSlot = 0;
static EWRAM_DATA u16 sProgressEvolution = SPECIES_NONE;
static EWRAM_DATA bool8 sProgressCanStop = FALSE;
extern const u8 CoopDuo_EventScript_AskGuest[];
extern const u8 CoopDuo_EventScript_GuestAfter[];
extern const u8 Coop_EventScript_Progression[];

static EWRAM_DATA u8 sGuestFollowup = 0;
static EWRAM_DATA u8 sLegendConflict = 0;
static EWRAM_DATA struct CoopBeaconState sBeacon = {0};
static EWRAM_DATA u8 sBeaconQueued = 0;
static EWRAM_DATA u8 sBeaconPrompted = 0;
static EWRAM_DATA u8 sBeaconBagWarned = 0;
static EWRAM_DATA u8 sRefusal = COOP_REFUSE_NONE;
static EWRAM_DATA u8 sSoloNote = COOP_SOLO_NONE;
static EWRAM_DATA u32 sMoneyAtLaunch = 0;
static EWRAM_DATA u16 sStorySeq = 0;
static EWRAM_DATA u16 sStoryLastSeq = 0;
static EWRAM_DATA bool8 sStorySeqKnown = FALSE;
static EWRAM_DATA u16 sStoryTimer = 0;
static EWRAM_DATA u16 sStorySent = 0;
static EWRAM_DATA bool8 sStorySentValid = FALSE;
static EWRAM_DATA u16 sStoryPartner = 0;
static EWRAM_DATA u8 sSceneAnnounce = COOP_SCENE_NONE;
static EWRAM_DATA u8 sSceneJoin = COOP_SCENE_NONE;
static EWRAM_DATA bool8 sHandlingStash = FALSE;
EWRAM_DATA u16 gCoopStaleInvites = 0; // test visibility: old or unknown requests refused
static EWRAM_DATA u16 sSceneJoinAge = 0; // frames a coord-scene join has waited
static EWRAM_DATA u8 sSceneJoinGroup = 0;
static EWRAM_DATA u8 sSceneJoinNum = 0;

// The partner marker's activity (CoopFrame.ext), PING and the last place the
// partner was drawn (for the broken-link icon while the link recovers).
static EWRAM_DATA u8 sPingCount = 0;            // this game's PING counter (2 bits)
static EWRAM_DATA u16 sPingCooldown = 0;        // frames before another PING
static EWRAM_DATA u8 sPartnerPingSeen = 0;
static EWRAM_DATA bool8 sPartnerPingKnown = FALSE;
EWRAM_DATA u16 gCoopPingCue = 0;                // frames left of a received PING's cue
EWRAM_DATA u16 gCoopPingsReceived = 0;          // test visibility
EWRAM_DATA bool8 gCoopLocalWaiting = FALSE;     // waiting for the partner at a shared checkpoint
static EWRAM_DATA s16 sLastSeenX = 0;
static EWRAM_DATA s16 sLastSeenY = 0;
static EWRAM_DATA u8 sLastSeenGroup = 0;
static EWRAM_DATA u8 sLastSeenNum = 0;
static EWRAM_DATA bool8 sLastSeenValid = FALSE;

struct CoopNickTarget
{
    u8 inParty;
    u8 slot;
    u8 box;
    u8 pos;
};

EWRAM_DATA u8 gCoopBallTarget = 0;
EWRAM_DATA u8 gCoopBallTargetFor[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA u16 gCoopLastBall = ITEM_NONE;
static EWRAM_DATA struct CoopCatchLedger sLedger = {0};
static EWRAM_DATA struct Pokemon sCatches[COOP_WILD_SLOTS] = {0};
static EWRAM_DATA u8 sCatchCount = 0;
static EWRAM_DATA u8 sLocalLegendMask = 0;
static EWRAM_DATA u8 sPartnerLegendMask = 0;
static EWRAM_DATA struct CoopNickTarget sNickTargets[COOP_WILD_SLOTS] = {0};
static EWRAM_DATA u8 sNickCount = 0;
static EWRAM_DATA u8 sNickIndex = 0;
// What the partner's balls caught this fight, to tell this player after.
static EWRAM_DATA u16 sPartnerCaught[COOP_WILD_SLOTS] = {0};
static EWRAM_DATA u8 sPartnerCaughtCount = 0;
static EWRAM_DATA u16 sPairEngaged = 0;
static EWRAM_DATA bool8 sGuestPairPending = FALSE;
static EWRAM_DATA u16 sGuestPairTimer = 0;   // seconds the question has been open; 0xFFFF: closed (too late)

// A wait measured in real seconds: the cartridge clock while it runs, so a
// player who speeds the emulator up still gets the full time; 60 frames a
// second when there is no clock (or it stands still). Frames alone also end
// it after ten times the limit, so a stuck clock can never hold it open.
static EWRAM_DATA struct CoopWaitClock sInviteClock = {0};
static EWRAM_DATA struct CoopWaitClock sGuestClock = {0};
static EWRAM_DATA struct CoopWaitClock sBusyClock = {0};
static EWRAM_DATA bool8 sGuestAskClosed = FALSE;   // the open question was closed by its time limit
static EWRAM_DATA u8 sWaitShown = 0;          // the seconds last drawn in the waiting note (0xFF: none)
static EWRAM_DATA u8 sWaitText[96] = {0};
EWRAM_DATA u8 gCoopWaitSecondsLeft = 0;
// Battle start timing (frames, gMain.vblankCounter1) for the startup
// measurements: COOP_START_* stages, written once each per request.
EWRAM_DATA u32 gCoopStartTimes[COOP_START_COUNT] = {0};       // test visibility: the inviter's countdown
static EWRAM_DATA struct CoopInvite sGuestPairInvite = {0};
// The partner's team key (src/coop_teams.c) from the invite this game joins,
// or from the answer to this game's invite.
static EWRAM_DATA u16 sRemoteTeamKey = 0;
// Test visibility: Megas this game turned back after a co-op battle.
EWRAM_DATA u8 gCoopMegaReverts = 0;
// A joined wild fight starts once the question's script has ended: a wild
// battle never resumes a script, so one left waiting would stay stuck.
static EWRAM_DATA bool8 sJoinLaunchPending = FALSE;
// Both players spotted by the same trainer: the partner's invite for a
// trainer whose approach and intro are still playing here. It is answered
// (joined) when this side reaches that battle, instead of a "busy" no.
static EWRAM_DATA struct CoopInvite sSameTrainerInvite = {0};
static EWRAM_DATA bool8 sSameTrainerPending = FALSE;

static const u8 sText_BeaconNeedLink[] = _("Link from the CO-OP menu first.{PAUSE_UNTIL_PRESS}");
static const u8 sText_BeaconWaiting[] = _("Still waiting on your partner.{PAUSE_UNTIL_PRESS}");
static const u8 sText_BeaconQueued[] = _("The request waits until this\nbattle or trade ends.{PAUSE_UNTIL_PRESS}");
static const u8 sText_BeaconSent[] = _("Asking your partner.\nBoth of you have to agree.{PAUSE_UNTIL_PRESS}");

STATIC_ASSERT(COOP_BATTLE_SLOTS == MULTI_PARTY_SIZE, CoopFieldsTheStockMultiHalf);
STATIC_ASSERT(COOP_ROSTER_SLOTS == PARTY_SIZE, CoopSnapshotCoversWholeParty);
STATIC_ASSERT(COOP_PAIR_LEFT_ID == TRAINER_RICK, CoopPairLeftIsRick);
STATIC_ASSERT(COOP_CONN_SOUTH == CONNECTION_SOUTH && COOP_CONN_NORTH == CONNECTION_NORTH
           && COOP_CONN_WEST == CONNECTION_WEST && COOP_CONN_EAST == CONNECTION_EAST, CoopConnectionsMatchEngine);
STATIC_ASSERT(COOP_PAIR_RIGHT_ID == TRAINER_ALLEN, CoopPairRightIsAllen);

static void Task_CoopSession(u8 taskId);
static void Coop_PollBeacon(void);
static void Coop_Drop(const u8 *message);
static void Coop_LaunchBattle(bool8 isGuest);
static void Coop_SendInvite(void);
static u16 Coop_InviteChecksum(const struct CoopInvite *invite);
static void Coop_FailWait(void);
static bool8 Coop_InteractionBusy(void);
static void Coop_RaiseVar(u16 varId, u16 floor);
static bool8 Coop_PairBeatenLocally(void);
static void Coop_GiveRescueStarter(u8 caseId, bool8 opener);
static bool8 Coop_PartnerInBirchLab(void);
static void Coop_HandleStoryMsg(u32 playerId, const struct CoopStoryMsg *msg);
static void Coop_PollSharedItems(void);
static u8 Coop_RivalId(void);
static u8 Coop_SoloReasonHere(void);
static void Coop_PollStory(void);
static void Coop_PollCoordJoin(void);
static bool8 Coop_GuestBusy(void);
static bool8 Coop_PlaceStaysSolo(void);
static void Coop_AcceptMandatory(const struct CoopInvite *invite);
static void Coop_ClockStart(struct CoopWaitClock *clock);
static u8 Coop_ClockTick(struct CoopWaitClock *clock);
static void Coop_SendReply(u8 kind, u16 seq);

// Frames before another "went inside" note may show.
static EWRAM_DATA u16 sPartnerPopupCooldown = 0;
#define COOP_PARTNER_POPUP_COOLDOWN (60 * 20)

#define COOP_FOLLOWUP_WOODS 30

// CoopFrame.talkLocalId while that player is in a (solo) battle.
#define COOP_TALK_IN_BATTLE 0xFE

// Field frames the partner has reported COOP_ACT_BUSY in a row.
static EWRAM_DATA u16 sPartnerBusyFrames = 0;
#define COOP_STATUS_WIDTH_PX (14 * 8)

// The partner who chose UNLINK, for the message after the link is gone.
static EWRAM_DATA u8 sUnlinkName[PLAYER_NAME_LENGTH + 1] = {0};

// The CoopSave layouts (docs/COOP_STORAGE_PLAN.md). Layout 4 ended at
// offset 560; layout 5 appends struct CoopSaveV5 and must stay within the
// budget, leaving the rest of SaveBlock3 for later updates.
STATIC_ASSERT(offsetof(struct CoopSave, v5) == 560, CoopSaveLayout4End);
STATIC_ASSERT(sizeof(struct CoopSaveV5) <= 380, CoopSaveV5Budget);
STATIC_ASSERT(sizeof(struct SaveBlock3) + 680 <= 1624, CoopSaveReserve);

// A CoopSave this build does not understand (a newer layout, or neither the
// co-op magic nor the zero of a save from before co-op): it is never
// migrated, repaired or rewritten. Readers get this default copy instead,
// and CONTINUE refuses the save (Coop_SaveSchemaSupported).
static EWRAM_DATA struct CoopSave sUnsupportedCoopSave = {0};

bool8 Coop_SaveSchemaSupported(void)
{
    const struct CoopSave *save;

    if (gSaveBlock3Ptr == NULL)
        return TRUE;
    save = &gSaveBlock3Ptr->coop;
    if (save->magic == 0)
        return TRUE;    // a save from before co-op (or a fresh one): set up below
    if (save->magic != COOP_SAVE_MAGIC)
        return FALSE;
    if (save->layout > COOP_SAVE_LAYOUT)
        return FALSE;
    // A layout-5 block records its own size: another size is a variant this
    // build does not know (a development build's save).
    if (save->layout == 5 && save->v5.size != sizeof(struct CoopSaveV5))
        return FALSE;
    return TRUE;
}

static struct CoopSave *CoopSave(void)
{
    struct CoopSave *save;

    if (gSaveBlock3Ptr == NULL)
        return NULL;
    if (!Coop_SaveSchemaSupported())
    {
        if (sUnsupportedCoopSave.magic != COOP_SAVE_MAGIC)
        {
            memset(&sUnsupportedCoopSave, 0, sizeof(sUnsupportedCoopSave));
            sUnsupportedCoopSave.magic = COOP_SAVE_MAGIC;
            sUnsupportedCoopSave.layout = COOP_SAVE_LAYOUT;
            sUnsupportedCoopSave.caseId = COOP_CASE_NONE;
            sUnsupportedCoopSave.rivalId = COOP_RIVAL_NONE;
            sUnsupportedCoopSave.deal.outSlot = 0xFF;
            sUnsupportedCoopSave.legendaryOwner[0] = COOP_OWNER_NONE;
            sUnsupportedCoopSave.legendaryOwner[1] = COOP_OWNER_NONE;
            sUnsupportedCoopSave.legendaryOwner[2] = COOP_OWNER_NONE;
        }
        return &sUnsupportedCoopSave;
    }
    save = &gSaveBlock3Ptr->coop;
    if (save->magic != COOP_SAVE_MAGIC)
    {
        save->magic = COOP_SAVE_MAGIC;
        save->levelCapOn = TRUE;
        save->jointBadges = 0;
        save->rivalStage = 0;
        save->legendaryOwner[0] = COOP_OWNER_NONE;
        save->legendaryOwner[1] = COOP_OWNER_NONE;
        save->legendaryOwner[2] = COOP_OWNER_NONE;
        save->pendingLegendary = 0;
        save->caseId = COOP_CASE_NONE;
        save->layout = 0;
    }
    // Saves from before the story ledger kept a zero here and arbitrary
    // bytes after it. Start the new fields clean.
    if (save->layout < 1)
    {
        save->layout = 1;
        save->storyDone = 0;
        save->storyApplied = 0;
        save->rivalId = COOP_RIVAL_NONE;
        save->sharedItems = 0;
        save->outfit = 0;
        save->partnerMarker = 0;
    }
    // Layout 2: escrow record, partner Pokedex cache, battle speed, rewards.
    if (save->layout < 2)
    {
        save->layout = 2;
        memset(&save->deal, 0, sizeof(save->deal));
        save->deal.outSlot = 0xFF;
        save->partnerDexOtId = 0;
        memset(save->partnerDexName, 0, sizeof(save->partnerDexName));
        save->partnerDexName[0] = EOS;
        save->partnerDexHours = 0;
        save->partnerDexMinutes = 0;
        save->partnerDexValid = FALSE;
        memset(save->partnerCaught, 0, sizeof(save->partnerCaught));
        save->battleSpeed = 0;
        save->questFlags = 0;
        memset(save->rewardsClaimed, 0, sizeof(save->rewardsClaimed));
        memset(save->dealHistory, 0, sizeof(save->dealHistory));
    }
    // Layout 3: shared legendary encounters. Records from the earlier
    // one-owner design (legendaryOwner: who of the pair kept Groudon, Kyogre
    // or Rayquaza) become per-save claims: the owner's catch is that save's
    // claim, and the partner's save, which took part but got nothing then,
    // is owed its own Pokémon (delivered once, like any pending reward).
    if (save->layout < 3)
    {
        static const u8 sOldLegendEnc[3] = { COOP_ENC_GROUDON, COOP_ENC_KYOGRE, COOP_ENC_RAYQUAZA };
        u32 i;

        save->layout = 3;
        save->legendClaimed = 0;
        save->legendPending = 0;
        save->legendOwed = 0;
        save->legendOwedOtId = 0;
        memset(save->legendPendingBall, 0, sizeof(save->legendPendingBall));
        memset(save->legendOwedBall, 0, sizeof(save->legendOwedBall));
        memset(save->legendFromOtId, 0, sizeof(save->legendFromOtId));
        for (i = 0; i < 3; i++)
        {
            u8 bit = 1 << (sOldLegendEnc[i] - 1);

            if (save->legendaryOwner[i] == COOP_OWNER_LOCAL)
                save->legendClaimed |= bit;
            else if (save->legendaryOwner[i] == COOP_OWNER_PARTNER)
                save->legendPending |= bit;
            save->legendaryOwner[i] = COOP_OWNER_NONE;
        }
        save->pendingLegendary = 0;
    }
    // Layout 4: field kit, duo Frontier records.
    if (save->layout < 4)
    {
        save->layout = 4;
        save->fieldKit = 0;
        save->vialCharges = 0;
        save->duoRunId = 0;
        save->duoFacility = 0;
        save->duoMode = 0;
        save->duoRound = 0;
        save->duoSeed = 0;
        save->duoState = 0;
        save->duoPaidRound = 0;
        save->duoPartnerOtId = 0;
        memset(save->duoBest, 0, sizeof(save->duoBest));
        // Joint wins of older builds have no partner on record: they stay
        // on this save and are never handed to anyone on a relink.
        memset(save->jointBadgeOtId, 0, sizeof(save->jointBadgeOtId));
        save->jointRivalOtId = 0;
        save->jointRivalStage = 0;
        memset(save->storyOtId, 0, sizeof(save->storyOtId));
        save->questPartnerOtId = 0;
        memset(save->questJoint, 0, sizeof(save->questJoint));
        save->questJointGates = 0;
        save->questJointBreakers = 0;
        save->duoPaid = 0;
        save->duoOppLevel = 0;
        save->duoSymbols = 0;
        memset(save->duoTeamPid, 0, sizeof(save->duoTeamPid));
        memset(save->duoBestOpen, 0, sizeof(save->duoBestOpen));
    }
    // Layout 5 (this update): Trainer Bond, settings, time, habitats,
    // towns, headquarters. Everything starts empty: no partner and no Bond
    // history (Pokémon records are not touched), settings on their
    // defaults, nothing found or claimed yet. Only these bytes change.
    if (save->layout < 5)
    {
        save->layout = 5;
        memset(&save->v5, 0, sizeof(save->v5));
        save->v5.size = sizeof(save->v5);
    }
    return save;
}

struct CoopSave *Coop_GetSave(void)
{
    return CoopSave();
}

u16 Coop_IsLevelCapOn(void)
{
    struct CoopSave *save = CoopSave();

    if (save == NULL)
        return TRUE;
    return save->levelCapOn ? TRUE : FALSE;
}

u32 Coop_DisplayedLevelCap(void)
{
    u32 cap;

    if (!Coop_IsLevelCapOn())
        return 0;
    cap = GetCurrentLevelCap();
    if (cap >= MAX_LEVEL)
        return 0;
    return cap;
}

u16 Coop_SessionActive(void)
{
    return sLinkState == COOP_LINK_UP && gReceivedRemoteLinkPlayers
        && sLinkErrorStreak < COOP_LINK_QUIET_LIMIT;
}

// The cable is up even when the stock link-player flag is clear. The bag
// must still read buttons in that state.
u16 Coop_CableUp(void)
{
    return sLinkState == COOP_LINK_UP;
}

// Maps whose room is built from each save's own data: a Secret Base (its
// owner's decorations), a Battle Pyramid floor and a Trainer Hill floor
// (their own layouts). Two players there share a map number, not a room, so
// the partner is neither drawn nor counted as being on this map.
static bool8 Coop_MapIsPersonalInstance(void)
{
    u8 hill = GetCurrentTrainerHillMapId();

    return gMapHeader.mapType == MAP_TYPE_SECRET_BASE
        || gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR
        || (hill != 0 && hill != TRAINER_HILL_ENTRANCE);
}

static bool8 Coop_SameRoom(u8 group, u8 num)
{
    return CoopProtocol_SameMap(group, num, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum)
        && !Coop_MapIsPersonalInstance();
}

u16 Coop_PartnerOnSameMap(void)
{
    if (!Coop_SessionActive() || !sPartnerValid)
        return FALSE;
    return Coop_SameRoom(sPartner.mapGroup, sPartner.mapNum);
}

u16 Coop_PartnerOnSameTile(void)
{
    s16 x, y;

    if (!Coop_PartnerOnSameMap())
        return FALSE;
    PlayerGetDestCoords(&x, &y);
    return sPartner.x == (u8)x && sPartner.y == (u8)y;
}

// Eligible to be asked: the same map, on any tile. Co-op already needs the
// partner on the map; a distance limit only turned a partner across the
// route into a solo fight they never heard about.
static bool8 Coop_PartnerNearby(void)
{
    return Coop_PartnerOnSameMap();
}

u16 Coop_CaseChosen(void)
{
    struct CoopSave *save = CoopSave();

    if (save == NULL || save->caseId > 2)
        return FALSE;
    return TRUE;
}

static u16 Coop_PartnerGfx(const struct CoopFrame *frame)
{
    bool8 female = frame->gender == FEMALE;

    // The partner's own palette (outfit), not this player's.
    if (frame->surf)
        return CoopOutfit_PartnerGfx(female, COOP_OUTFIT_POSE_SURFING);
    if (frame->bike && frame->acro)
        return CoopOutfit_PartnerGfx(female, COOP_OUTFIT_POSE_ACRO_BIKE);
    if (frame->bike)
        return CoopOutfit_PartnerGfx(female, COOP_OUTFIT_POSE_MACH_BIKE);
    return CoopOutfit_PartnerGfx(female, COOP_OUTFIT_POSE_NORMAL);
}

// The partner object keeps the map it was spawned on. Crossing a connection
// changes this game's map but keeps the object (the camera shifts it into the
// new grid), so look it up by id alone and adopt it into the current map. A
// second copy is never wanted.
static u8 Coop_FindPartnerObject(void)
{
    u8 i, found = OBJECT_EVENTS_COUNT;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *obj = &gObjectEvents[i];

        if (!obj->active || obj->localId != OBJ_EVENT_ID_COOP_PARTNER)
            continue;
        if (found < OBJECT_EVENTS_COUNT)
        {
            RemoveObjectEvent(obj);
            continue;
        }
        found = i;
        obj->mapNum = gSaveBlock1Ptr->location.mapNum;
        obj->mapGroup = gSaveBlock1Ptr->location.mapGroup;
    }
    return found;
}

// A surfing partner sits on a surf blob, as the player does; the sitting
// sprite alone looks like someone standing on the water. The blob belongs to
// the partner object through fieldEffectSpriteId. A battle or a map load
// frees every sprite, so a stored id is trusted only while that sprite is
// still this object's blob.
static bool8 Coop_PartnerBlobLive(u8 objectId)
{
    u8 id = gObjectEvents[objectId].fieldEffectSpriteId;

    return id != 0 && id < MAX_SPRITES && gSprites[id].inUse
        && gSprites[id].callback == UpdateSurfBlobFieldEffect
        && gSprites[id].data[2] == objectId;
}

static void Coop_DropPartnerBlob(u8 objectId)
{
    if (Coop_PartnerBlobLive(objectId))
        DestroySprite(&gSprites[gObjectEvents[objectId].fieldEffectSpriteId]);
    gObjectEvents[objectId].fieldEffectSpriteId = 0;
}

static void Coop_SyncPartnerBlob(u8 objectId)
{
    struct ObjectEvent *obj = &gObjectEvents[objectId];
    u8 id;

    if (!sPartner.surf)
    {
        if (obj->fieldEffectSpriteId != 0)
            Coop_DropPartnerBlob(objectId);
        return;
    }
    if (Coop_PartnerBlobLive(objectId))
        return;
    gFieldEffectArguments[0] = obj->currentCoords.x;
    gFieldEffectArguments[1] = obj->currentCoords.y;
    gFieldEffectArguments[2] = objectId;
    id = FieldEffectStart(FLDEFF_SURF_BLOB);
    if (id >= MAX_SPRITES)
    {
        obj->fieldEffectSpriteId = 0;
        return;
    }
    obj->fieldEffectSpriteId = id;
    SetSurfBlob_BobState(id, BOB_PLAYER_AND_MON);
}

static void Coop_RemoveSprite(void)
{
    u8 i;

    sPartnerStepWait = 0;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == OBJ_EVENT_ID_COOP_PARTNER)
        {
            Coop_DropPartnerBlob(i);
            RemoveObjectEvent(&gObjectEvents[i]);
        }
    }
}

// Where the partner stands in this game's map grid (MAP_OFFSET included).
// The same map is direct. A map joined to this one by a north, south, east,
// or west connection is translated with the camera's own crossing rule, and
// shown only inside the strip of it this game has loaded around the edge,
// which is all the screen can reach. Interiors, warps, and maps further away
// have no position here. Drawing only; battles still need the same map.
// A tile of map (group, num) in this map's coordinates: the same map, or a
// map joined to it by a connection (whose tiles are loaded around this one).
bool8 Coop_MapCoordsToLocal(u8 group, u8 num, s16 x, s16 y, s16 *outX, s16 *outY)
{
    const struct MapConnections *connections = gMapHeader.connections;
    const struct MapHeader *otherMap;
    int i, lx, ly;

    if (Coop_SameRoom(group, num))
    {
        *outX = x;
        *outY = y;
        return TRUE;
    }
    if (connections == NULL || connections->connections == NULL)
        return FALSE;
    otherMap = Overworld_GetMapHeaderByGroupAndId(group, num);
    if (otherMap == NULL || otherMap->mapLayout == NULL)
        return FALSE;
    for (i = 0; i < connections->count; i++)
    {
        const struct MapConnection *c = &connections->connections[i];

        if (c->mapGroup != group || c->mapNum != num)
            continue;
        if (!CoopProtocol_ConnectionToLocal(c->direction, c->offset,
                gMapHeader.mapLayout->width, gMapHeader.mapLayout->height,
                otherMap->mapLayout->width, otherMap->mapLayout->height,
                x, y, &lx, &ly))
            continue;
        if (lx < 0 || ly < 0 || lx >= gBackupMapLayout.width || ly >= gBackupMapLayout.height)
            return FALSE;
        *outX = lx;
        *outY = ly;
        return TRUE;
    }
    return FALSE;
}

static bool8 Coop_PartnerLocalCoords(s16 *x, s16 *y)
{
    if (!Coop_SessionActive() || !sPartnerValid)
        return FALSE;
    return Coop_MapCoordsToLocal(sPartner.mapGroup, sPartner.mapNum, sPartner.x, sPartner.y, x, y);
}

// A north, south, east, or west connection joins the two maps.
static bool8 Coop_MapsConnected(u8 groupA, u8 numA, u8 groupB, u8 numB)
{
    const struct MapHeader *header = Overworld_GetMapHeaderByGroupAndId(groupA, numA);
    int i;

    if (header == NULL || header->connections == NULL || header->connections->connections == NULL)
        return FALSE;
    for (i = 0; i < header->connections->count; i++)
    {
        const struct MapConnection *c = &header->connections->connections[i];

        if (c->mapGroup == groupB && c->mapNum == numB
         && c->direction >= CONNECTION_SOUTH && c->direction <= CONNECTION_EAST)
            return TRUE;
    }
    return FALSE;
}

// A short note when the partner leaves the part of the world drawn here:
// through a door, into a cave, or by a warp or Fly.
static void Coop_NotePartnerLeftView(void)
{
    const struct MapHeader *header;
    u8 *end;

    if (FlagGet(FLAG_HIDE_MAP_NAME_POPUP) || gMain.inBattle || !sPartnerValid)
        return;
    // In and out of a Centre or a shop is one note, not one per door.
    if (sPartnerPopupCooldown != 0)
        return;
    sPartnerPopupCooldown = COOP_PARTNER_POPUP_COOLDOWN;
    header = Overworld_GetMapHeaderByGroupAndId(sPartner.mapGroup, sPartner.mapNum);
    if (header == NULL)
        return;
    end = StringCopy(gCoopPopupText, gLinkPlayers[GetMultiplayerId() ^ 1].name);
    switch (header->mapType)
    {
    case MAP_TYPE_INDOOR:
    case MAP_TYPE_SECRET_BASE:
        StringCopy(end, COMPOUND_STRING(" went inside"));
        break;
    case MAP_TYPE_UNDERGROUND:
        StringCopy(end, COMPOUND_STRING(" went in"));
        break;
    case MAP_TYPE_UNDERWATER:
        StringCopy(end, COMPOUND_STRING(" dived"));
        break;
    default:
        *end++ = CHAR_COLON;
        *end++ = CHAR_SPACE;
        GetMapName(end, header->regionMapSectionId, 0);
        break;
    }
    ShowCoopPopup();
}

// Where the partner should be drawn. The link frame can share the player's
// tile; the sprite then stands one tile south so it stays on screen.
static bool8 Coop_PartnerTileFree(s16 x, s16 y)
{
    u8 objectId;

    if (MapGridGetCollisionAt(x, y))
        return FALSE;
    objectId = GetObjectEventIdByXY(x, y);
    return objectId >= OBJECT_EVENTS_COUNT || gObjectEvents[objectId].localId == OBJ_EVENT_ID_COOP_PARTNER;
}

// The shared tile is drawn beside the player on the first free side, south
// first as before. A wall, a counter, or an NPC there would hide the partner
// inside it, so those sides are skipped.
static void Coop_PartnerTarget(s16 *x, s16 *y)
{
    s16 px, py;
    u8 freeMask = 0;

    PlayerGetDestCoords(&px, &py);
    if (!Coop_PartnerLocalCoords(x, y))
    {
        *x = sPartner.x;
        *y = sPartner.y;
    }
    if (*x != px || *y != py)
        return;
    if (Coop_PartnerTileFree(px, py + 1))
        freeMask |= COOP_TILE_SOUTH;
    if (Coop_PartnerTileFree(px, py - 1))
        freeMask |= COOP_TILE_NORTH;
    if (Coop_PartnerTileFree(px - 1, py))
        freeMask |= COOP_TILE_WEST;
    if (Coop_PartnerTileFree(px + 1, py))
        freeMask |= COOP_TILE_EAST;
    // Nothing free: keep the old south rule so the sprite is never lost.
    if (freeMask == 0)
        freeMask = COOP_TILE_SOUTH;
    CoopProtocol_PickPartnerTile(px, py, x, y, freeMask);
}

// A cutscene can park a step on top of Mum or the player. That step never
// finishes, and the old code then ignored every later position.
static void Coop_NoteBadFrame(void)
{
    if (gMain.inBattle || sLinkState != COOP_LINK_UP)
        return;
    if (++sBadFrameStreak < 20)
        return;
    sBadFrameStreak = 0;
    Link_RecoverCoopCable();
    sPartnerResettle = TRUE;
    sSpriteGfx = 0xFFFF;
}

static void Coop_NoteGoodFrame(void)
{
    sBadFrameStreak = 0;
    sLinkErrorStreak = 0;
}

static void Coop_ShowPartner(struct ObjectEvent *obj)
{
    if (obj->spriteId >= MAX_SPRITES || !gSprites[obj->spriteId].inUse)
        return;
    obj->invisible = FALSE;
    gSprites[obj->spriteId].invisible = FALSE;
}

// 0 still stepping, 1 ready for a new step, 2 the last step was abandoned.
static u8 Coop_PartnerReadyForStep(struct ObjectEvent *obj)
{
    struct Sprite *sprite = &gSprites[obj->spriteId];

    if (obj->frozen)
        UnfreezeObjectEvent(obj);
    obj->movementType = MOVEMENT_TYPE_NONE;
    if (obj->heldMovementActive && !obj->heldMovementFinished)
        sprite->animPaused = FALSE;
    if (ObjectEventClearHeldMovementIfFinished(obj) != 0)
    {
        sPartnerStepWait = 0;
        return 1;
    }
    if (++sPartnerStepWait < 32)
        return 0;
    ObjectEventClearHeldMovementIfActive(obj);
    sPartnerStepWait = 0;
    return 2;
}

static enum Direction Coop_StepDir(s16 dx, s16 dy)
{
    s16 adx = dx < 0 ? -dx : dx;
    s16 ady = dy < 0 ? -dy : dy;

    if (adx >= ady && dx != 0)
        return dx > 0 ? DIR_EAST : DIR_WEST;
    if (dy > 0)
        return DIR_SOUTH;
    if (dy < 0)
        return DIR_NORTH;
    return DIR_NONE;
}

static void Coop_UpdateSprite(void)
{
    u16 gfx;
    u8 objectId;
    u8 ready;
    s16 dx, dy, adx, ady;
    u8 action;
    s16 tx, ty;
    enum Direction dir;
    struct ObjectEvent *obj;
    struct ObjectEventTemplate template;

    // Map loads and door fades reset the sprite list. Spawning during that
    // paints a character onto a blank screen and the next map wipe deletes it.
    if (gMain.callback2 != CB2_Overworld)
        return;
    if (gMain.inBattle || !Coop_PartnerLocalCoords(&tx, &ty))
    {
        // Judge only live frames: a quiet link or a battle says nothing about
        // where the partner went. Seen a moment ago and now on a map that is
        // not drawn here means a door, a cave, or a warp.
        if (!gMain.inBattle && sPartnerValid && Coop_SessionActive())
        {
            if (sPartnerDrawn && (sPartner.mapGroup != sPartnerDrawnGroup || sPartner.mapNum != sPartnerDrawnNum))
                Coop_NotePartnerLeftView();
            sPartnerDrawn = FALSE;
        }
        Coop_RemoveSprite();
        return;
    }
    sPartnerDrawn = TRUE;
    sPartnerDrawnGroup = sPartner.mapGroup;
    sPartnerDrawnNum = sPartner.mapNum;
    sLastSeenX = tx;
    sLastSeenY = ty;
    sLastSeenGroup = gSaveBlock1Ptr->location.mapGroup;
    sLastSeenNum = gSaveBlock1Ptr->location.mapNum;
    sLastSeenValid = TRUE;

    gfx = Coop_PartnerGfx(&sPartner);
    objectId = Coop_FindPartnerObject();
    if (objectId < OBJECT_EVENTS_COUNT
     && (gObjectEvents[objectId].spriteId >= MAX_SPRITES || !gSprites[gObjectEvents[objectId].spriteId].inUse))
    {
        Coop_RemoveSprite();
        objectId = OBJECT_EVENTS_COUNT;
    }
    if (objectId >= OBJECT_EVENTS_COUNT || sSpriteGfx != gfx)
    {
        Coop_RemoveSprite();
        memset(&template, 0, sizeof(template));
        template.localId = OBJ_EVENT_ID_COOP_PARTNER;
        template.graphicsId = gfx;
        template.kind = OBJ_KIND_NORMAL;
        // Spawn adds MAP_OFFSET itself. The link frame already includes it.
        Coop_PartnerTarget(&template.x, &template.y);
        // The tile's own height, so a partner across a connection or on a
        // ledge or bridge is drawn at their level, not at this player's.
        template.elevation = MapGridGetElevationAt(template.x, template.y);
        if (template.elevation == 0 || template.elevation == ELEVATION_MULTI_LEVEL)
            template.elevation = PlayerGetElevation();
        template.x -= MAP_OFFSET;
        template.y -= MAP_OFFSET;
        template.movementType = MOVEMENT_TYPE_NONE;
        template.trainerType = TRAINER_TYPE_NONE;
        template.script = Coop_EventScript_TalkPartner;
        template.flagId = 0;
        sSpriteGfx = gfx;
        objectId = SpawnSpecialObjectEvent(&template);
        if (objectId < OBJECT_EVENTS_COUNT)
        {
            gObjectEvents[objectId].fieldEffectSpriteId = 0;
            Coop_SyncPartnerBlob(objectId);
            Coop_ShowPartner(&gObjectEvents[objectId]);
            if (sPartner.facing != DIR_NONE)
                ObjectEventTurn(&gObjectEvents[objectId], (enum Direction)sPartner.facing);
        }
        return;
    }

    obj = &gObjectEvents[objectId];
    Coop_SyncPartnerBlob(objectId);
    Coop_PartnerTarget(&tx, &ty);
    // A released script only needs a resettle when the sprite is off its
    // tile or stuck mid-step. Re-placing a partner who is already standing
    // there made every closed message box snap and flicker the sprite.
    if (sPartnerResettle && obj->currentCoords.x == tx && obj->currentCoords.y == ty
     && !obj->heldMovementActive && !obj->frozen)
    {
        sPartnerResettle = FALSE;
        Coop_ShowPartner(obj);
    }
    if (sPartnerResettle)
    {
        sPartnerResettle = FALSE;
        ObjectEventClearHeldMovementIfActive(obj);
        ready = 2;
    }
    else
    {
        ready = Coop_PartnerReadyForStep(obj);
    }
    if (ready == 0)
        return;

    if (ready == 2)
    {
        MoveObjectEventToMapCoords(obj, tx, ty);
        Coop_ShowPartner(obj);
        if (sPartner.facing != DIR_NONE)
            ObjectEventTurn(obj, (enum Direction)sPartner.facing);
        return;
    }
    dx = tx - obj->currentCoords.x;
    dy = ty - obj->currentCoords.y;
    adx = dx < 0 ? -dx : dx;
    ady = dy < 0 ? -dy : dy;
    if (dx == 0 && dy == 0)
    {
        enum Direction face = (enum Direction)sPartner.facing;

        Coop_ShowPartner(obj);

        if (face == DIR_NONE)
            face = DIR_SOUTH;
        // Talking players stand still; do not invent repeated walking steps.
        if (obj->facingDirection != face)
            ObjectEventTurn(obj, face);
        return;
    }

    // Leaving a house is one tile past the mat, then normal walking.
    // A gap bigger than one tile is a warp, so place them instead of
    // stepping out of the doorway they already left.
    if (adx > 1 || ady > 1)
    {
        MoveObjectEventToMapCoords(obj, tx, ty);
        Coop_ShowPartner(obj);
        if (sPartner.facing != DIR_NONE)
            ObjectEventTurn(obj, (enum Direction)sPartner.facing);
        return;
    }

    dir = Coop_StepDir(dx, dy);
    if (dir == DIR_NONE)
        return;
    obj->singleMovementActive = FALSE;
    ObjectEventClearHeldMovementIfActive(obj);
    // Running frames exist only in the on-foot sprite's animation table; a
    // bike or surfing sprite given a run action reads past its own table
    // (the Mach Bike's: a jump to address 0 and a reset). Bikes and Surf
    // keep up at their own fast speeds instead.
    if (sPartner.bike)
        action = GetWalkFasterMovementAction(dir);
    else if (sPartner.surf)
        action = adx + ady > 1 ? GetWalkFastMovementAction(dir) : GetWalkNormalMovementAction(dir);
    else
        action = adx + ady > 1 ? GetPlayerRunMovementAction(dir) : GetWalkNormalMovementAction(dir);
    ObjectEventSetHeldMovement(obj, action);
}

static void Coop_ReleaseTalkNpc(void)
{
    if (sTalkNpc == 0)
        return;
    u8 objectId = GetObjectEventIdByLocalIdAndMap(sTalkNpc, sTalkNpcMapNum, sTalkNpcMapGroup);
    if (objectId < OBJECT_EVENTS_COUNT && !ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked())
    {
        struct ObjectEvent *npc = &gObjectEvents[objectId];
        if (!npc->heldMovementActive)
            ObjectEventTurn(npc, (enum Direction)sTalkNpcFacing);
    }
    sTalkNpc = 0;
}

// A remote conversation is cosmetic. Never clear or replace movement owned
// by a local conversation/cutscene, and never restore a same-id NPC on a new map.
static void Coop_AnimateTalkNpc(void)
{
    u8 localId = sPartner.talkLocalId;
    if (gMain.callback2 != CB2_Overworld || !Coop_PartnerOnSameMap()
     || ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked()
     || localId == 0 || localId == 0xFF || localId == COOP_TALK_IN_BATTLE || localId == OBJ_EVENT_ID_COOP_PARTNER)
    {
        Coop_ReleaseTalkNpc();
        return;
    }
    if (localId != sTalkNpc || sTalkNpcMapGroup != gSaveBlock1Ptr->location.mapGroup
     || sTalkNpcMapNum != gSaveBlock1Ptr->location.mapNum)
        Coop_ReleaseTalkNpc();
    u8 objectId = GetObjectEventIdByLocalIdAndMap(localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    if (objectId >= OBJECT_EVENTS_COUNT)
        return;
    struct ObjectEvent *npc = &gObjectEvents[objectId];
    if (npc->heldMovementActive || npc->singleMovementActive)
        return;
    s16 dx = (s16)sPartner.x - npc->currentCoords.x;
    s16 dy = (s16)sPartner.y - npc->currentCoords.y;
    if ((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) != 1)
    {
        Coop_ReleaseTalkNpc();
        return;
    }
    if (sTalkNpc == 0)
    {
        sTalkNpcFacing = npc->facingDirection;
        sTalkNpcMapGroup = gSaveBlock1Ptr->location.mapGroup;
        sTalkNpcMapNum = gSaveBlock1Ptr->location.mapNum;
    }
    sTalkNpc = localId;
    enum Direction face = dx ? (dx > 0 ? DIR_EAST : DIR_WEST) : (dy > 0 ? DIR_SOUTH : DIR_NORTH);
    ObjectEventTurn(npc, face);
}

static u8 Coop_LocalActivity(void)
{
    if (gMain.inBattle || sInBattle || gCoopBattleLaunching || Coop_InteractionBusy() || gPaletteFade.active)
        return COOP_ACT_BUSY;
    if (gMain.callback2 != CB2_Overworld)
        return COOP_ACT_MENU;
    if (ScriptContext_IsEnabled())
        return COOP_ACT_TALKING;
    // The start menu and other field windows lock the controls without a script.
    if (ArePlayerFieldControlsLocked())
        return COOP_ACT_MENU;
    return COOP_ACT_EXPLORING;
}

// A question between the two players is open on this game: an invitation
// sent or being answered, a partner-menu request, a Beacon change.
static bool8 Coop_LocalInvitePending(void)
{
    return sWaitingAck || sGuestPairPending || sSameTrainerPending
        || sBeacon.phase != COOP_BEACON_IDLE || CoopDeal_Busy();
}

// This game is held at a shared checkpoint until the partner is ready.
static bool8 Coop_LocalWaitingForPartner(void)
{
    return sWaitKind != 0 || sRequest == COOP_REQ_RESCUE || gCoopLocalWaiting;
}

// The local field is locked only because this game waits on the partner.
bool8 Coop_LocalAwaitingPartner(void)
{
    return sWaitingAck || Coop_LocalWaitingForPartner() || CoopDeal_WaitingForPartner() || CoopTalk_Shared();
}

static u8 Coop_LocalExt(void)
{
    u8 ext = (u8)((sPingCount & 3) << COOP_EXT_PING_SHIFT);

    if (Coop_LocalWaitingForPartner())
        ext |= COOP_EXT_WAITING;
    if (CoopFollow_Active())
        ext |= COOP_EXT_FOLLOWING;
    if (Coop_LocalInvitePending())
        ext |= COOP_EXT_INVITE;
    if (CoopTalk_LocalJoinable())
        ext |= COOP_EXT_JOINABLE;
    return ext;
}

EWRAM_DATA u8 gCoopJointRestored = 0;   // joint outcomes restored on relinks (tests)

static void Coop_RefreshLocalStoryWith(u32 partnerOt);

static u32 Coop_PartnerOtId(void)
{
    if (sLinkState != COOP_LINK_UP || !gReceivedRemoteLinkPlayers)
        return 0;
    return gLinkPlayers[GetMultiplayerId() ^ 1].trainerId;
}

// What this save offers the partner on a link: only the joint wins it
// earned with that same partner.
static u8 Coop_JointBadgesWithPartner(const struct CoopSave *save)
{
    u32 partnerOt = Coop_PartnerOtId();
    u8 i, bits = 0;

    if (partnerOt == 0)
        return 0;
    for (i = 0; i < 8; i++)
    {
        if ((save->jointBadges & (1 << i)) && save->jointBadgeOtId[i] == partnerOt)
            bits |= 1 << i;
    }
    return bits;
}

static u8 Coop_JointRivalWithPartner(const struct CoopSave *save)
{
    u32 partnerOt = Coop_PartnerOtId();

    if (partnerOt == 0 || save->jointRivalOtId != partnerOt)
        return 0;
    return save->jointRivalStage;
}

static void Coop_FillFrame(struct CoopFrame *frame)
{
    s16 x, y;
    struct CoopSave *save = CoopSave();
    u8 i;

    PlayerGetDestCoords(&x, &y);
    memset(frame, 0, sizeof(*frame));
    frame->buildXor = COOP_BUILD_XOR;
    frame->mapGroup = gSaveBlock1Ptr->location.mapGroup;
    frame->mapNum = gSaveBlock1Ptr->location.mapNum;
    frame->x = (u8)x;
    frame->y = (u8)y;
    frame->facing = (u8)GetPlayerFacingDirection();
    frame->capOn = Coop_IsLevelCapOn() ? 1 : 0;
    frame->request = sRequest;
    frame->gender = gSaveBlock2Ptr->playerGender;
    frame->bike = TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE) || TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_ACRO_BIKE) ? 1 : 0;
    frame->acro = TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_ACRO_BIKE) ? 1 : 0;
    frame->surf = TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) ? 1 : 0;
    frame->slot = sSlot;
    frame->seq = sSeq++;
    frame->talkLocalId = 0;
    if (sBirchBag)
        frame->talkLocalId = 0xFF;
    else if (CoopTalk_HostNpc() != 0)
        frame->talkLocalId = CoopTalk_HostNpc();
    else if (ScriptContext_IsEnabled()
          && gSpecialVar_LastTalked != 0
          && gSpecialVar_LastTalked != OBJ_EVENT_ID_COOP_PARTNER)
        frame->talkLocalId = gSpecialVar_LastTalked;
    if (save != NULL)
    {
        frame->badges = Coop_JointBadgesWithPartner(save);
        frame->rivalStage = Coop_JointRivalWithPartner(save);
    }
    // In a solo battle (frames keep coming, see Coop_LinkCallback).
    if (gMain.inBattle)
        frame->talkLocalId = COOP_TALK_IN_BATTLE;
    frame->activity = Coop_LocalActivity();
    frame->ext = Coop_LocalExt();
}

// What the partner's state means for an ordinary fight right now, from this
// game's side: COOP_SOLO_NONE when it would ask them to join.
static u8 Coop_FightOutlook(void)
{
    u8 why = Coop_SoloReasonHere();

    if (why != COOP_SOLO_NONE)
        return why;
    if (!sBeacon.on)
        return COOP_SOLO_NONE;
    if (sPartner.activity != COOP_ACT_EXPLORING)
        return COOP_SOLO_BUSY;
    return COOP_SOLO_NONE;
}

// The partner is talking to the nurse: their frame names the object, and
// their map's event table (in ROM) says what that object is.
static bool8 Coop_PartnerAtNurse(void)
{
    const struct MapHeader *header;
    u32 i;

    if (sPartner.activity != COOP_ACT_TALKING || sPartner.talkLocalId == 0 || sPartner.talkLocalId == 0xFF)
        return FALSE;
    header = Overworld_GetMapHeaderByGroupAndId(sPartner.mapGroup, sPartner.mapNum);
    if (header == NULL || header->events == NULL)
        return FALSE;
    for (i = 0; i < header->events->objectEventCount; i++)
    {
        if (header->events->objectEvents[i].localId == sPartner.talkLocalId)
            return header->events->objectEvents[i].graphicsId == OBJ_EVENT_GFX_NURSE;
    }
    return FALSE;
}

// Appends src only if the line (from lineStart) still fits the status
// window; returns the new end either way.
static u8 *Coop_AppendIfFits(u8 *lineStart, u8 *end, const u8 *src)
{
    u8 *tail = StringCopy(end, src);

    if (GetStringWidth(FONT_NARROW, lineStart, 0) <= COOP_STATUS_WIDTH_PX)
        return tail;
    *end = EOS;
    return end;
}

// Five short lines for the start menu: the link and Beacon, who the partner
// is and what they are doing, where they are, whether a fight here would be
// together (and if not, why), and this player's rival. Read fresh each time
// the menu opens.
void Coop_FormatStatus(u8 *dest)
{
    static const u8 *const sBusy[] = {
        [COOP_ACT_EXPLORING] = COMPOUND_STRING("Fights: solo"),
        [COOP_ACT_TALKING]   = COMPOUND_STRING("Solo: partner talking"),
        [COOP_ACT_MENU]      = COMPOUND_STRING("Solo: partner in menu"),
        [COOP_ACT_BUSY]      = COMPOUND_STRING("Solo: partner busy"),
    };
    u8 *end, *line;
    u8 outlook;
    const struct MapHeader *header;

    if (sLinkState != COOP_LINK_UP)
    {
        StringCopy(dest, COMPOUND_STRING("CO-OP: not linked"));
        return;
    }
    if (!CheckBagHasItem(ITEM_TWIN_BEACON, 1))
        end = StringCopy(dest, COMPOUND_STRING("LINKED · no BEACON yet"));
    else
        end = StringCopy(dest, sBeacon.on ? COMPOUND_STRING("LINKED · BEACON ON") : COMPOUND_STRING("LINKED · BEACON OFF"));
    *end++ = CHAR_NEWLINE;
    if (!Coop_SessionActive())
    {
        // Only real trouble: the cable has stalled for a few seconds.
        StringCopy(end, COMPOUND_STRING("Link trouble:\nretrying..."));
        return;
    }
    if (!sPartnerValid)
    {
        StringCopy(end, COMPOUND_STRING("Partner: joining…"));
        return;
    }

    // Who and what.
    if (gReceivedRemoteLinkPlayers && gLinkPlayers[GetMultiplayerId() ^ 1].name[0] != EOS)
    {
        u8 name[PLAYER_NAME_LENGTH + 1];

        StringCopyN(name, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
        name[PLAYER_NAME_LENGTH] = EOS;
        end = StringCopy(end, name);
    }
    else
    {
        end = StringCopy(end, COMPOUND_STRING("Partner"));
    }
    end = StringCopy(end, COMPOUND_STRING(": "));
    switch (sPartner.activity & 3)
    {
    case COOP_ACT_TALKING:
        end = StringCopy(end, Coop_PartnerAtNurse() ? COMPOUND_STRING("healing") : COMPOUND_STRING("talking"));
        break;
    case COOP_ACT_MENU:
        end = StringCopy(end, COMPOUND_STRING("in a menu"));
        break;
    case COOP_ACT_BUSY:
        // Doors and warps report busy for a moment too; a battle lasts.
        end = StringCopy(end, sPartnerBusyFrames >= 60 ? COMPOUND_STRING("battling") : COMPOUND_STRING("busy"));
        break;
    default:
        end = StringCopy(end, COMPOUND_STRING("exploring"));
        break;
    }
    *end++ = CHAR_NEWLINE;

    // Where: the map's name, and whether that is with this player.
    line = end;
    header = Overworld_GetMapHeaderByGroupAndId(sPartner.mapGroup, sPartner.mapNum);
    if (header != NULL)
    {
        end = GetMapName(end, header->regionMapSectionId, 0);
        if (Coop_PartnerOnSameMap())
            end = Coop_AppendIfFits(line, end, COMPOUND_STRING(", with you"));
        else if (header->mapType == MAP_TYPE_INDOOR || header->mapType == MAP_TYPE_SECRET_BASE)
            end = Coop_AppendIfFits(line, end, COMPOUND_STRING(", inside"));
    }
    else
    {
        end = StringCopy(end, COMPOUND_STRING("Somewhere else"));
    }
    *end++ = CHAR_NEWLINE;

    outlook = Coop_FightOutlook();
    if (!sBeacon.on)
        end = StringCopy(end, COMPOUND_STRING("Fights: solo"));
    else if (outlook == COOP_SOLO_NONE)
        end = StringCopy(end, COMPOUND_STRING("Fights: together"));
    else if (outlook == COOP_SOLO_BUSY && sPartner.activity != COOP_ACT_EXPLORING)
        end = StringCopy(end, sBusy[sPartner.activity & 3]);
    else if (outlook == COOP_SOLO_ELSEWHERE)
        end = StringCopy(end, COMPOUND_STRING("Fights: solo (apart)"));
    else if (outlook == COOP_SOLO_SELF_NOT_READY)
        end = StringCopy(end, COMPOUND_STRING("Solo: front 3 fainted"));
    else if (outlook == COOP_SOLO_PLACE)
        end = StringCopy(end, COMPOUND_STRING("Fights: solo here"));
    else
        end = StringCopy(end, COMPOUND_STRING("Fights: solo (busy)"));
    *end++ = CHAR_NEWLINE;
    end = StringCopy(end, COMPOUND_STRING("Your rival: "));
    if (Coop_RivalId() == COOP_RIVAL_SOREN)
        StringCopy(end, COMPOUND_STRING("SOREN"));
    else if (Coop_RivalId() == COOP_RIVAL_MIRA)
        StringCopy(end, COMPOUND_STRING("MIRA"));
    else
        StringCopy(end, COMPOUND_STRING("not yet"));
}

// The CO-OP menu's opening line in gStringVar4: what linking does, what the
// Beacon does, and why fights here would be solo. TRUE when the Beacon is on.
u16 Coop_BufferMenuExplain(void)
{
    const u8 *text;
    u8 outlook;

    if (!Coop_SessionActive() || !sPartnerValid)
        text = COMPOUND_STRING("The link is recovering. Wait,\nor choose UNLINK to stop.");
    else if (!CheckBagHasItem(ITEM_TWIN_BEACON, 1))
        text = COMPOUND_STRING("Linked: you explore together.\nNo TWIN BEACON yet.");
    else if (!sBeacon.on)
        text = COMPOUND_STRING("Linked: you explore together.\nBEACON OFF: optional fights solo.");
    else if ((outlook = Coop_FightOutlook()) == COOP_SOLO_NONE)
        text = COMPOUND_STRING("BEACON ON: fights here can be\nco-op with your partner.");
    else if (outlook == COOP_SOLO_ELSEWHERE)
        text = COMPOUND_STRING("BEACON ON, but you're on\ndifferent maps: fights are solo.");
    else if (outlook == COOP_SOLO_SELF_NOT_READY)
        text = COMPOUND_STRING("BEACON ON, but your first three\ncan't fight: fights are solo.");
    else if (outlook == COOP_SOLO_PLACE)
        text = COMPOUND_STRING("BEACON ON, but battles here\nare always one-player.");
    else
        text = COMPOUND_STRING("BEACON ON. Your partner is busy,\nso fights are solo for now.");
    StringCopy(gStringVar4, text);
    return sBeacon.on ? TRUE : FALSE;
}

// Field play with the cable up: link.c may read a receive backlog at once.
bool8 Coop_FieldLinkCatchUp(void)
{
    return sLinkState == COOP_LINK_UP && gMain.callback2 == CB2_Overworld
        && !gMain.inBattle && !gCoopBattleLaunching && !sInBattle;
}

void Coop_LinkCallback(void)
{
    struct CoopFrame frame;
    u16 words[8];

    // A link battle owns the cable (its callback is removed at setup). In a
    // solo battle keep sending the position frame (activity BUSY): silence
    // there used to run the partner's quiet limit out, so a long solo fight
    // read as "the link is recovering" on the other screen, and the
    // partner's sprite vanished until the fight ended.
    if (gMain.inBattle)
        CoopFollow_NoteBattle();
    if (!gReceivedRemoteLinkPlayers || gLinkTransferringData
     || (gMain.inBattle && (gBattleTypeFlags & BATTLE_TYPE_LINK)))
        return;
    // Shared NPC moves (src/coop_npc.c) take the slot at most every other
    // frame; the position frame keeps the rest.
    if (CoopNpc_TakeOutgoing(words))
    {
        memcpy(gSendCmd, words, sizeof(words));
        return;
    }
    Coop_FillFrame(&frame);
    CoopProtocol_Pack(words, &frame, COOP_SEND_AS_HELD_KEYS);
    memcpy(gSendCmd, words, sizeof(words));
}

// A relink restores only joint outcomes earned with this same partner (by
// trainer id, never by name): the badges of gyms won together and the rival
// stage of a rival fight won together. A badge, rival win or anything else
// the partner earned alone, or with someone else, stays on their save.
static void Coop_ApplyProgress(const struct CoopProgress *progress)
{
    struct CoopSave *save = CoopSave();
    u32 partnerOt = Coop_PartnerOtId();
    u8 i, add;

    if (save == NULL || partnerOt == 0)
        return;
    add = progress->badges & ~save->jointBadges;
    for (i = 0; i < 8; i++)
    {
        if (add & (1 << i))
        {
            save->jointBadges |= 1 << i;
            save->jointBadgeOtId[i] = partnerOt;
            FlagSet(FLAG_BADGE01_GET + i);
            if (gCoopJointRestored < 0xFF)
                gCoopJointRestored++;
        }
    }
    if (progress->rivalStage != 0)
    {
        if (progress->rivalStage > save->rivalStage)
        {
            save->rivalStage = progress->rivalStage;
            if (gCoopJointRestored < 0xFF)
                gCoopJointRestored++;
        }
        if (save->jointRivalOtId == partnerOt || progress->rivalStage >= save->jointRivalStage)
        {
            if (save->jointRivalOtId != partnerOt || progress->rivalStage > save->jointRivalStage)
                save->jointRivalStage = progress->rivalStage;
            save->jointRivalOtId = partnerOt;
        }
    }
}

static const u16 sLegendSpecies[] = { SPECIES_GROUDON, SPECIES_KYOGRE, SPECIES_RAYQUAZA };

// PING: the partner's counter moved. A short sound, a line in the map-name
// window and a pulse on the partner's marker or edge arrow. The first frame
// of a session only learns the counter.
static void Coop_NotePartnerPing(const struct CoopFrame *frame)
{
    u8 count = (frame->ext & COOP_EXT_PING_MASK) >> COOP_EXT_PING_SHIFT;
    const struct MapHeader *header;
    u8 *end;

    if (!sPartnerPingKnown)
    {
        sPartnerPingKnown = TRUE;
        sPartnerPingSeen = count;
        return;
    }
    if (count == sPartnerPingSeen)
        return;
    sPartnerPingSeen = count;
    if (gCoopPingsReceived < 0xFFFF)
        gCoopPingsReceived++;
    gCoopPingCue = COOP_PING_CUE_FRAMES;
    if (gMain.inBattle)
        return;
    PlaySE(SE_PIN);
    end = StringCopy(gCoopPopupText, gLinkPlayers[GetMultiplayerId() ^ 1].name);
    header = Overworld_GetMapHeaderByGroupAndId(frame->mapGroup, frame->mapNum);
    if (Coop_PartnerOnSameMap() || header == NULL)
    {
        StringCopy(end, COMPOUND_STRING(" pinged you!"));
    }
    else
    {
        // Elsewhere: where they are (the popup holds about 20 characters).
        end = StringCopy(gCoopPopupText, COMPOUND_STRING("Ping: "));
        GetMapName(end, header->regionMapSectionId, 0);
    }
    ShowCoopPopup();
}

static void Coop_AcceptPartner(const struct CoopFrame *frame)
{
    struct CoopSave *save;
    struct CoopProgress local, remote, merged;
    int merge;
    u8 i;

    // A house exit is a new map. The sprite has to be placed again, not
    // walked from the indoor doorway.
    if (sPartnerValid && (sPartner.mapGroup != frame->mapGroup || sPartner.mapNum != frame->mapNum
                       || sPartner.x != frame->x || sPartner.y != frame->y))
    {
        s16 dx = (s16)frame->x - (s16)sPartner.x;
        s16 dy = (s16)frame->y - (s16)sPartner.y;

        if (dx < 0)
            dx = -dx;
        if (dy < 0)
            dy = -dy;
        // Walking over a map connection changes the map but is one step;
        // the sprite keeps walking (Coop_PartnerLocalCoords translates it).
        if (sPartner.mapGroup != frame->mapGroup || sPartner.mapNum != frame->mapNum)
        {
            if (!Coop_MapsConnected(sPartner.mapGroup, sPartner.mapNum, frame->mapGroup, frame->mapNum))
                sPartnerResettle = TRUE;
        }
        else if (dx > 1 || dy > 1)
        {
            sPartnerResettle = TRUE;
        }
    }
    sPartner = *frame;
    sPartnerValid = TRUE;
    Coop_NoteGoodFrame();
    Coop_NotePartnerPing(frame);
    if (sMerged)
        return;

    save = CoopSave();
    if (save == NULL)
        return;
    // Whatever this save finished before the link came up was done alone.
    Coop_RefreshLocalStoryWith(0);
    memset(&local, 0, sizeof(local));
    memset(&remote, 0, sizeof(remote));
    local.badges = save->jointBadges;
    local.rivalStage = save->rivalStage;
    local.capOn = save->levelCapOn ? 1 : 0;
    remote.badges = frame->badges;
    remote.rivalStage = frame->rivalStage;
    remote.capOn = frame->capOn;
    for (i = 0; i < 3; i++)
    {
        local.legend[i] = save->legendaryOwner[i];
        remote.legend[i] = COOP_OWNER_NONE;   // each save's own (src/coop_legend.c)
    }
    merge = CoopProtocol_Merge(&local, &remote, &merged);
    if (merge == 1)
    {
        Coop_Drop(Coop_EventScript_CapMismatch);
        return;
    }
    if (merge == 2)
    {
        sLegendConflict = 0;
        for (i = 0; i < 3; i++)
        {
            if (local.legend[i] != COOP_OWNER_NONE && remote.legend[i] != COOP_OWNER_NONE && local.legend[i] == remote.legend[i])
            {
                sLegendConflict = i;
                break;
            }
        }
        Coop_Drop(Coop_EventScript_LegendConflict);
        return;
    }
    // Only what the partner's save offers as won together with this save.
    Coop_ApplyProgress(&remote);
    sSessionPartnerOt = Coop_PartnerOtId();
    sMerged = TRUE;
}

static void Coop_HandleFrame(u32 playerId, const u16 *words)
{
    struct CoopFrame frame;
    int status;

    if (playerId == GetMultiplayerId())
        return;
    status = CoopProtocol_Unpack(words, &frame, COOP_BUILD_XOR);
    if (status == COOP_UNPACK_DROP)
        return;
    if (status == COOP_UNPACK_BUILD)
    {
        Coop_Drop(Coop_EventScript_BuildMismatch);
        return;
    }
    if (frame.request == COOP_REQ_UNLINK)
    {
        // Deliberate unlink is distinct from a missing heartbeat.
        StringCopyN(sUnlinkName, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
        sUnlinkName[PLAYER_NAME_LENGTH] = EOS;
        Coop_Drop(sUnlinkTimer ? NULL : Coop_EventScript_PartnerUnlinked);
        return;
    }
    Coop_AcceptPartner(&frame);
}

void Coop_OnRecv(u32 playerId)
{
    u16 words[8];

    memcpy(words, gRecvCmds[playerId], sizeof(words));
    if (CoopProtocol_Checksum(words) != words[7])
    {
        Coop_NoteBadFrame();
        return;
    }
    Coop_HandleFrame(playerId, words);
}

bool8 Coop_TryParseHeldKeys(u32 playerId)
{
    u16 words[8];
    u16 probe[8];

    if (playerId == GetMultiplayerId())
        return FALSE;
    memcpy(words, gRecvCmds[playerId], sizeof(words));
    if (words[0] != COOP_HELD_KEYS_CMD)
        return FALSE;
    if (CoopNpc_TryHandleFrame(words))
        return TRUE;
    memcpy(probe, words, sizeof(probe));
    probe[0] = COOP_LINKCMD;
    if (CoopProtocol_Checksum(probe) != words[7])
    {
        Coop_NoteBadFrame();
        return FALSE;
    }
    Coop_HandleFrame(playerId, words);
    return TRUE;
}

static u16 Coop_InviteChecksum(const struct CoopInvite *invite)
{
    return (u16)(invite->magic ^ invite->kind ^ ((u16)invite->badge << 8) ^ invite->trainerA ^ invite->trainerB ^ invite->aux ^ invite->seq
               ^ (u16)(invite->teamKey * 7u) ^ COOP_INVITE_MAGIC);
}

static void Coop_SendInvite(void)
{
    if (sInviteRetryDelay != 0)
    {
        sInviteRetryDelay--;
        return;
    }
    if (gLinkTransferringData || sInviteSent)
        return;
    sInvite.checksum = Coop_InviteChecksum(&sInvite);
    if (SendBlock(BitmaskAllOtherLinkPlayers(), &sInvite, sizeof(sInvite)))
        sInviteSent = TRUE;
}

static void Coop_QueueInvite(u8 kind, u16 trainerA, u16 trainerB, u8 badge, u16 aux)
{
    memset(&sInvite, 0, sizeof(sInvite));
    sInvite.magic = COOP_INVITE_MAGIC;
    sInvite.kind = kind;
    sInvite.badge = badge;
    sInvite.trainerA = trainerA;
    sInvite.trainerB = trainerB;
    sInvite.aux = aux;
    sInvite.seq = ++sInviteSeq;
    Coop_NoteStartTime(COOP_START_QUEUED);
    sInvite.teamKey = Coop_TeamKeyLocal();
    sInviteRetryDelay = 0;
    sRefusal = COOP_REFUSE_NONE;
    sPartnerBusyNow = FALSE;
    sWaitingAck = TRUE;
    sInviteSent = FALSE;
    sInviteTimer = 0;
    Coop_ClockStart(&sInviteClock);
    sWaitShown = 0xFF;
    sBattleKind = kind;
    sBattleBadge = badge;
    sBattleFlag = aux;
    TRAINER_BATTLE_PARAM.opponentA = trainerA;
    TRAINER_BATTLE_PARAM.opponentB = trainerB;
}

static EWRAM_DATA bool8 sWaitOpenedBox = FALSE;   // the waiting note opened the message box itself

static void Coop_HideCallingMessage(void)
{
    // (IsFieldMessageBoxHidden() is TRUE once a line has finished printing,
    // with the box still on screen: a note this game drew is always closed.)
    if (sWaitOpenedBox
     || ((sBattleKind == COOP_KIND_WILD || sBattleKind == COOP_KIND_TRAINER) && !IsFieldMessageBoxHidden()))
        HideFieldMessageBox();
    sWaitOpenedBox = FALSE;
    sWaitShown = 0xFF;
}

// The inviter's note while the partner decides, with the seconds left:
// "Calling ROSA over… 27 / B: battle alone." for an optional fight, or
// "Waiting for ROSA to agree… 27 / B: stop waiting." for a shared story
// fight (written over the trainer's own message when that is still up).
static void Coop_DrawWaitNote(u8 left)
{
    u8 *end;
    const u8 *name = gLinkPlayers[GetMultiplayerId() ^ 1].name;
    bool8 optional = sBattleKind == COOP_KIND_WILD || sBattleKind == COOP_KIND_TRAINER;

    if (left == sWaitShown)
        return;
    if (!IsFieldMessageBoxHidden() && IsTextPrinterActiveOnWindow(0))
        return;   // the trainer's own line is still being printed
    if (optional)
        end = StringCopy(sWaitText, COMPOUND_STRING("Calling "));
    else
        end = StringCopy(sWaitText, COMPOUND_STRING("Waiting for "));
    end = StringCopyN(end, name, PLAYER_NAME_LENGTH);
    *end = EOS;
    if (optional)
        end = StringAppend(sWaitText, COMPOUND_STRING(" over… "));
    else if (sPartnerBusyNow)
        end = StringAppend(sWaitText, COMPOUND_STRING(" to be free… "));
    else
        end = StringAppend(sWaitText, COMPOUND_STRING(" to agree… "));
    end = ConvertIntToDecimalStringN(end, left, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringAppend(sWaitText, optional ? COMPOUND_STRING("\nB: battle alone.") : COMPOUND_STRING("\nB: stop waiting."));
    // The first note opens the box (or replaces the trainer's finished
    // line); later ones redraw it in place at once. "Hidden" here only means
    // that nothing is being printed: the box stays on screen after a line.
    if (!sWaitOpenedBox)
    {
        if (!ShowFieldMessage(sWaitText))
            return;
        sWaitOpenedBox = TRUE;
    }
    else
    {
        FillWindowPixelBuffer(0, PIXEL_FILL(1));
        AddTextPrinterParameterized(0, FONT_NORMAL, sWaitText, 0, 1, 0, NULL);
        CopyWindowToVram(0, COPYWIN_FULL);
    }
    sWaitShown = left;
}

static bool8 Coop_IsTwinTrialTrainer(u16 trainerId)
{
    switch (trainerId)
    {
    case TRAINER_BT_ROUND1_SOL:
    case TRAINER_BT_ROUND2_EMBER:
    case TRAINER_BT_FINAL_LUMEN:
    case TRAINER_BT_REPEAT_LUMEN:
        return TRUE;
    }
    return FALSE;
}

static void Coop_LaunchBattle(bool8 isGuest)
{
    // Both games pick the same parties: the lower progress tier of the two
    // and the inviter's starter case (Battle Guide v2 variants).
    if (isGuest)
        Coop_SetBattleTeamKey(Coop_TeamKeyMerge(sRemoteTeamKey, Coop_TeamKeyLocal()));
    else
        Coop_SetBattleTeamKey(Coop_TeamKeyMerge(Coop_TeamKeyLocal(), sRemoteTeamKey));
    gCoopPartnerHasBracelet = (sRemoteTeamKey & COOP_TEAMKEY_VALID) && (sRemoteTeamKey & COOP_TEAMKEY_BRACELET);
    if (!isGuest)
        Coop_HideCallingMessage();
    // A duo Frontier battle: the run's opponents (src/coop_frontier.c).
    if (sBattleKind == COOP_KIND_DUO)
    {
        const struct CoopInvite *duo = isGuest ? &sGuestPairInvite : &sInvite;

        sBattleBadge = 0;
        sBattleFlag = 0;
        CoopDuo_BeginBattle(duo->trainerA, duo->trainerB, duo->aux, duo->badge);
    }
    // The teams chosen before the fight move to the front (undone after).
    CoopPrep_OnLaunch();
    Coop_NoteStartTime(COOP_START_LAUNCH);
    sWaitingAck = FALSE;
    sIsGuest = isGuest;
    sInBattle = TRUE;
    gCoopBattleLaunching = TRUE;
    if (sBattleKind == COOP_KIND_LEGEND)
    {
        // One wild legendary against both players: the co-op double with the
        // opposing right battler absent (src/coop_legend.c).
        gCoopArmWild = TRUE;
        CreateScriptedWildMon(CoopLegend_Species(sLegendKind), CoopLegend_Level(sLegendKind), ITEM_NONE);
        BattleSetup_StartLegendaryBattle();
        return;
    }
    if (sBattleKind == COOP_KIND_WILD || sBattleKind == COOP_KIND_RESCUE)
    {
        if (sBattleKind == COOP_KIND_RESCUE)
        {
            sBirchBag = FALSE;
            sResumeScript = FALSE;
            CreateScriptedDoubleWildMon(SPECIES_POOCHYENA, 2, ITEM_NONE, SPECIES_POOCHYENA, 2, ITEM_ORAN_BERRY);
        }
        BattleSetup_StartCoopWildBattle();
        return;
    }
    if (isGuest)
    {
        LockPlayerFieldControls();
        FreezeObjectEvents();
        StopPlayerAvatar();
        // The Twin Trial restores both teams before every round. The host's
        // heal message can land while this side is still leaving the last
        // round, so the joining side heals here too, before its team is sent.
        if (Coop_IsTwinTrialTrainer(TRAINER_BATTLE_PARAM.opponentA))
            HealPlayerParty();
    }
    BattleSetup_StartTrainerBattle();
}

static bool8 Coop_IsOptionalKind(u8 kind)
{
    return kind == COOP_KIND_TRAINER || kind == COOP_KIND_WILD || kind == COOP_KIND_PAIR;
}

static bool8 Coop_IsMandatoryKind(u8 kind)
{
    return kind == COOP_KIND_GYM || kind == COOP_KIND_RIVAL || kind == COOP_KIND_E4
        || kind == COOP_KIND_CHAMPION || kind == COOP_KIND_STORY || kind == COOP_KIND_DUO;
}

// An optional invite never pulls a player out of a menu, a script, a trade,
// or a prompt that is already on screen.
// TRUE while this game is itself about to fight trainerId: its approach or
// intro is playing, or its own invite for that trainer is out.
static bool8 Coop_SameTrainerHere(u16 trainerId)
{
    if (trainerId == TRAINER_NONE || trainerId >= TRAINERS_COUNT || HasTrainerBeenFought(trainerId))
        return FALSE;
    if (sInBattle || gMain.inBattle || TRAINER_BATTLE_PARAM.opponentA != trainerId)
        return FALSE;
    if (sWaitingAck)
        return sBattleKind == COOP_KIND_TRAINER;
    return gNoOfApproachingTrainers != 0 && ScriptContext_IsEnabled();
}

static void Coop_JoinSameTrainer(const struct CoopInvite *invite)
{
    Coop_HideCallingMessage();
    sSameTrainerPending = FALSE;
    sBattleKind = COOP_KIND_TRAINER;
    sBattleBadge = 0;
    sBattleFlag = 0;
    sLegendKind = 0;
    sRivalFightStage = 0xFF;
    sResumeScript = FALSE;
    TRAINER_BATTLE_PARAM.opponentA = invite->trainerA;
    TRAINER_BATTLE_PARAM.opponentB = invite->trainerB;
    sRemoteTeamKey = invite->teamKey;
    Coop_SendReply(COOP_KIND_ACK, invite->seq);
    Coop_LaunchBattle(TRUE);
}

void Coop_NoteBadTrainer(u8 site, u16 trainerA, u16 trainerB)
{
    if (gCoopDiag.badTrainer != 0xFFFF)
        gCoopDiag.badTrainer++;
    gCoopDiag.lastA = trainerA;
    gCoopDiag.lastB = trainerB;
    gCoopDiag.site = site;
    gCoopDiag.approaching = gNoOfApproachingTrainers;
    gCoopDiag.kind = sBattleKind;
    gCoopDiag.flags = (sInBattle ? 1 : 0) | (sWaitingAck ? 2 : 0) | (sGuestPairPending ? 4 : 0)
                    | (ScriptContext_IsEnabled() ? 8 : 0);
    gCoopDiag.selectedObj = gSelectedObjectEvent;
    gCoopDiag.frame = gMain.vblankCounter1;
    gCoopDiag.script = gApproachingTrainers[0].trainerScriptPtr;
}

static bool8 Coop_GuestBusy(void)
{
    return Coop_LocalAvailability(COOP_PURPOSE_OPTIONAL) != COOP_AVAIL_OK;
}

// A reply that meets a block already on the cable used to be dropped, and
// the inviter then sat out its whole timeout. Replies now wait their turn.
static EWRAM_DATA u8 sReplyKind = 0;
static EWRAM_DATA u16 sReplySeq = 0;
static EWRAM_DATA u8 sReplyReason = 0;
static EWRAM_DATA u16 sReplyAux = 0;

// A NACK carries why (COOP_SOLO_*) in the badge byte, for the inviter's
// solo note.
static bool8 Coop_TrySendReply(u8 kind, u16 seq, u8 reason, u16 aux)
{
    struct CoopInvite reply;

    if (gLinkTransferringData)
        return FALSE;
    memset(&reply, 0, sizeof(reply));
    reply.magic = COOP_INVITE_MAGIC;
    reply.kind = kind;
    reply.seq = seq;
    reply.badge = reason;
    reply.aux = aux;
    reply.teamKey = Coop_TeamKeyLocal();
    reply.checksum = Coop_InviteChecksum(&reply);
    return SendBlock(BitmaskAllOtherLinkPlayers(), &reply, sizeof(reply));
}

static void Coop_SendReplyWhyAux(u8 kind, u16 seq, u8 reason, u16 aux)
{
    if (Coop_TrySendReply(kind, seq, reason, aux))
        return;
    sReplyKind = kind;
    sReplySeq = seq;
    sReplyReason = reason;
    sReplyAux = aux;
}

static void Coop_SendReplyWhy(u8 kind, u16 seq, u8 reason)
{
    Coop_SendReplyWhyAux(kind, seq, reason, 0);
}

static void Coop_SendReply(u8 kind, u16 seq)
{
    Coop_SendReplyWhy(kind, seq, COOP_SOLO_NONE);
}

static void Coop_FlushReply(void)
{
    if (sReplyKind != 0 && Coop_TrySendReply(sReplyKind, sReplySeq, sReplyReason, sReplyAux))
        sReplyKind = 0;
}

static void Coop_HandleInvite(u32 playerId, const struct CoopInvite *invite)
{
    if (playerId == GetMultiplayerId())
        return;
    if (invite->magic != COOP_INVITE_MAGIC || invite->checksum != Coop_InviteChecksum(invite))
        return;
    // A kind this build does not know: dropped, never acted on.
    if (invite->kind == 0 || invite->kind > COOP_KIND_LAST)
    {
        if (gCoopStaleInvites < 0xFFFF)
            gCoopStaleInvites++;
        return;
    }

    if (invite->kind == COOP_KIND_ACK)
    {
        // A team preparation's start (CoopPrep: the host asked, the partner
        // answered with this ACK and is launching): the block itself shows
        // the partner is there, even while its overworld heartbeat lags.
        if (sWaitingAck && invite->seq == sInvite.seq && Coop_SessionActive()
         && (sPartnerValid || CoopPrep_HostCommitted()))
        {
            Coop_NoteStartTime(COOP_START_ANSWERED);
            sRemoteTeamKey = invite->teamKey;
            Coop_LaunchBattle(FALSE);
        }
        return;
    }

    if (invite->kind == COOP_KIND_WITHDRAW)
    {
        if (sSameTrainerPending && invite->seq == sSameTrainerInvite.seq)
            sSameTrainerPending = FALSE;
        // The inviter stopped waiting; the open question becomes too late.
        if (sGuestPairPending && invite->seq == sGuestPairInvite.seq)
            sGuestPairTimer = 0xFFFF;
        CoopPrep_PartnerWithdrew(invite->seq);
        // A YES that crossed the withdrawal: the inviter is fighting alone.
        if (sJoinLaunchPending && invite->seq == sGuestPairInvite.seq)
        {
            sJoinLaunchPending = FALSE;
            sInBattle = FALSE;
            sPendingMessage = Coop_EventScript_PartnerDeclined;
        }
        return;
    }

    if (invite->kind == COOP_KIND_NACK)
    {
        if (sWaitingAck && invite->seq == sInvite.seq)
        {
            // The rescue is not optional. A busy partner just means try again.
            if (sBattleKind == COOP_KIND_RESCUE)
            {
                sInviteSent = FALSE;
                return;
            }
            // Neither is a gym, rival, Elite Four or story fight: a partner
            // in a menu screen (bag, party, PC, a shop) is asked again a
            // little later, until the invite's own time runs out.
            if (Coop_IsMandatoryKind(sBattleKind) && invite->badge == COOP_SOLO_BUSY)
            {
                if (sRefusal != COOP_REFUSE_PARTNER_BUSY)
                    Coop_ClockStart(&sBusyClock);
                sInviteSent = FALSE;
                sInviteRetryDelay = COOP_BUSY_RETRY_FRAMES;
                sRefusal = COOP_REFUSE_PARTNER_BUSY;
                sPartnerBusyNow = TRUE;
                Coop_ClockStart(&sInviteClock);
                if (gCoopBusyRetries < 0xFFFF)
                    gCoopBusyRetries++;
                return;
            }
            if (sBattleKind == COOP_KIND_DUO && invite->badge >= COOP_DUO_NACK_TEAM && invite->badge <= COOP_DUO_NACK_NO_RUN)
            {
                CoopDuo_OnPartnerRefused(invite->badge, invite->aux);
                sRefusal = invite->badge == COOP_DUO_NACK_TEAM ? COOP_REFUSE_DUO_TEAM
                         : invite->badge == COOP_DUO_NACK_OTHER_RUN ? COOP_REFUSE_DUO_OTHER_RUN
                         : invite->badge == COOP_DUO_NACK_BEHIND ? COOP_REFUSE_DUO_BEHIND
                         : COOP_REFUSE_DUO_NO_RUN;
                Coop_FailWait();
                return;
            }
            sRefusal = invite->badge == COOP_SOLO_DECLINED ? COOP_REFUSE_PARTNER_DECLINED
                     : invite->badge == COOP_SOLO_NO_ANSWER ? COOP_REFUSE_NO_ANSWER
                     : COOP_REFUSE_PARTNER_NOT_READY;
            sSoloNote = invite->badge < COOP_SOLO_COUNT && invite->badge != COOP_SOLO_NONE
                      ? invite->badge : COOP_SOLO_DECLINED;
            Coop_FailWait();
        }
        return;
    }

    if (invite->kind == COOP_KIND_WARP)
    {
        if (!ScriptContext_IsEnabled())
            sPendingMessage = Coop_EventScript_WarpToLab;
        Coop_SendReply(COOP_KIND_ACK, invite->seq);
        return;
    }

    if (sInBattle || gMain.inBattle)
        return;

    // A request that arrived during this player's battle (kept aside and
    // read now) is old: its sender has stopped waiting or will ask again.
    // Answer "busy" instead of asking about a fight that is over.
    if (sHandlingStash && invite->kind != COOP_KIND_NACK)
    {
        if (gCoopStaleInvites < 0xFFFF)
            gCoopStaleInvites++;
        Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, COOP_SOLO_BUSY);
        return;
    }

    if (CoopProtocol_KeepOwnInvite(GetMultiplayerId(), sWaitingAck, Coop_IsMandatoryKind(sBattleKind), Coop_IsMandatoryKind(invite->kind)))
        return;

    // A fight against trainers must name trainers this build can build. A
    // bad id (a damaged frame, another build's table) is refused here, so
    // neither game launches a battle against nobody (finding #16).
    if ((Coop_IsMandatoryKind(invite->kind) || invite->kind == COOP_KIND_TRAINER || invite->kind == COOP_KIND_PAIR)
     && invite->kind != COOP_KIND_DUO
     && (invite->trainerA == TRAINER_NONE || invite->trainerA >= TRAINERS_COUNT || invite->trainerB >= TRAINERS_COUNT))
    {
        Coop_NoteBadTrainer(COOP_BADTR_INVITE, invite->trainerA, invite->trainerB);
        Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, COOP_SOLO_KIND);
        return;
    }

    // A shared fight never starts from a menu screen (bag, party, PC, a
    // shop) or under a message box: the battle would replace that screen
    // without closing it (from a conversation the heap was left damaged and
    // the game stopped at the battle's start). The inviter asks again until
    // this player is back on the field.
    // Since 0xC017013E the shared availability check also covers a running
    // script or cutscene with nothing on screen (and an open question); this
    // game's own invitation for a shared fight does not count as busy.
    if (Coop_IsMandatoryKind(invite->kind) && Coop_LocalAvailability(COOP_PURPOSE_MANDATORY) != COOP_AVAIL_OK)
    {
        Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, COOP_SOLO_BUSY);
        return;
    }

    if (invite->kind == COOP_KIND_RESCUE)
    {
        if (Coop_GuestBusy() || invite->badge > 2)
        {
            Coop_SendReply(COOP_KIND_NACK, invite->seq);
            return;
        }
        Coop_GiveRescueStarter(invite->badge, FALSE);
        CreateScriptedDoubleWildMon(SPECIES_POOCHYENA, 2, ITEM_NONE, SPECIES_POOCHYENA, 2, ITEM_ORAN_BERRY);
        sBattleKind = COOP_KIND_RESCUE;
        sBattleBadge = 0;
        sBattleFlag = 0;
        sLegendKind = 0;
        sRivalFightStage = 0xFF;
        TRAINER_BATTLE_PARAM.opponentA = 0;
        TRAINER_BATTLE_PARAM.opponentB = 0;
        sRemoteTeamKey = invite->teamKey;
        Coop_SendReply(COOP_KIND_ACK, invite->seq);
        Coop_LaunchBattle(TRUE);
        return;
    }

    // A shared legendary: asked, like an optional fight, but never needs the
    // Beacon. A partner whose save cannot take part says so (the inviter is
    // offered the encounter alone). Both players talking to it at once: link
    // player 0's invite wins and the other game joins it.
    if (invite->kind == COOP_KIND_LEGEND)
    {
        u8 why = COOP_SOLO_NONE;

        if (sWaitingAck && sBattleKind == COOP_KIND_LEGEND && sLegendKind == invite->aux)
        {
            if (GetMultiplayerId() == 0)
                return;
            sWaitingAck = FALSE;
            sWaitKind = 0;
            sRequest = COOP_REQ_NONE;
            sResumeScript = FALSE;
            sBattleBadge = 0;
            sBattleFlag = 0;
            sRivalFightStage = 0xFF;
            TRAINER_BATTLE_PARAM.opponentA = 0;
            TRAINER_BATTLE_PARAM.opponentB = 0;
            sRemoteTeamKey = invite->teamKey;
            Coop_SendReply(COOP_KIND_ACK, invite->seq);
            Coop_LaunchBattle(TRUE);
            return;
        }
        if (Coop_GuestBusy() || sGuestPairPending)
            why = COOP_SOLO_BUSY;
        else if (!Coop_HasBattleReadyLead())
            why = COOP_SOLO_PARTNER_NOT_READY;
        else if (!Coop_PartnerOnSameMap())
            why = COOP_SOLO_ELSEWHERE;
        else if (!CoopLegend_Eligible((u8)invite->aux))
            why = COOP_SOLO_LEGEND_NOT_ELIGIBLE;
        if (why != COOP_SOLO_NONE)
        {
            Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, why);
            return;
        }
        sGuestPairInvite = *invite;
        sGuestPairPending = TRUE;
        sGuestAskClosed = FALSE;
        Coop_NoteStartTime(COOP_START_ASKED);
        sGuestPairTimer = 0;
        Coop_ClockStart(&sGuestClock);
        ScriptContext_SetupScript(Coop_EventScript_LegendAskGuest);
        return;
    }

    // Optional fights ask the partner first. Only a partner who is idle on
    // the field, nearby on the same map, and battle-ready is asked; anyone
    // else is a quiet no and the inviter fights alone.
    if (Coop_IsOptionalKind(invite->kind))
    {
        u8 why = COOP_SOLO_NONE;

        // The same trainer spotted both players. Both games run its approach
        // and intro, and each would ask the other, hear "busy", and fight
        // alone. Join instead: one fight, two players.
        if (invite->kind == COOP_KIND_TRAINER && Coop_SameTrainerHere(invite->trainerA))
        {
            if (sWaitingAck)
            {
                // Both games already asked: link player 0's invite wins.
                if (GetMultiplayerId() == 0)
                    return;
                Coop_JoinSameTrainer(invite);
                return;
            }
            sSameTrainerInvite = *invite;
            sSameTrainerPending = TRUE;
            return;
        }

        if (Coop_GuestBusy() || sGuestPairPending)
            why = COOP_SOLO_BUSY;
        else if (!Coop_HasBattleReadyLead())
            why = COOP_SOLO_PARTNER_NOT_READY;
        else if (!sBeacon.on)
            why = COOP_SOLO_DECLINED;
        else if (!Coop_PartnerNearby())
            why = COOP_SOLO_ELSEWHERE;
        else if ((invite->kind == COOP_KIND_PAIR && Coop_PairBeatenLocally())
         || (invite->kind == COOP_KIND_WILD && (invite->trainerA == SPECIES_NONE || invite->trainerA >= NUM_SPECIES
                                             || invite->trainerB == SPECIES_NONE || invite->trainerB >= NUM_SPECIES)))
            why = COOP_SOLO_KIND;
        if (why != COOP_SOLO_NONE)
        {
            Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, why);
            return;
        }
        sGuestPairInvite = *invite;
        sGuestPairPending = TRUE;
        sGuestAskClosed = FALSE;
        Coop_NoteStartTime(COOP_START_ASKED);
        sGuestPairTimer = 0;
        Coop_ClockStart(&sGuestClock);
        ScriptContext_SetupScript(invite->kind == COOP_KIND_PAIR ? Coop_EventScript_PairAskGuest : Coop_EventScript_JoinAskGuest);
        return;
    }

    // A duo Frontier battle (src/coop_frontier.c): this save's own run and
    // team decide; healed copies fight, so no Pokemon needs HP now.
    if (invite->kind == COOP_KIND_DUO)
    {
        u16 aux = 0;
        u8 why;

        if (sWaitingAck && sBattleKind == COOP_KIND_DUO)
        {
            // Both asked at once. The same battle of the same run: one
            // fight, as for a leader both talked to. Different requests:
            // this game stops waiting and hears the partner's next.
            if (sInvite.trainerA == invite->trainerA && sInvite.aux == invite->aux && !(invite->badge & 1))
                goto join;
            sRefusal = COOP_REFUSE_DUO_SAME_TIME;
            Coop_FailWait();
            Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, COOP_SOLO_BUSY);
            return;
        }
        why = CoopDuo_GuestCheck(invite->trainerA, invite->trainerB, invite->aux, invite->badge, &aux);
        if (why != 0)
        {
            Coop_SendReplyWhyAux(COOP_KIND_NACK, invite->seq, why, aux);
            return;
        }
        sGuestPairInvite = *invite;
        sGuestPairPending = TRUE;
        sGuestAskClosed = FALSE;
        Coop_NoteStartTime(COOP_START_ASKED);
        sGuestPairTimer = 0;
        Coop_ClockStart(&sGuestClock);
        CoopDuo_BufferAskText(invite->aux, invite->trainerB, invite->badge);
        ScriptContext_SetupScript(CoopDuo_EventScript_AskGuest);
        return;
    }

    // Story fights pick their team first (src/coop_prep.c): any one
    // Pokemon able to fight is enough to be asked.
    if (!Coop_HasBattleReadyMon())
    {
        Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, COOP_SOLO_PARTNER_NOT_READY);
        sPendingMessage = Coop_EventScript_NotReady;
        return;
    }

    // A shared story fight starts only when this player knowingly agrees:
    // a question, unless this game has itself just asked for the same fight
    // (both players talked to it). Never a silent pull.
    if (Coop_IsMandatoryKind(invite->kind) && !(sWaitingAck && Coop_IsMandatoryKind(sBattleKind)))
    {
        sGuestPairInvite = *invite;
        sGuestPairPending = TRUE;
        sGuestAskClosed = FALSE;
        Coop_NoteStartTime(COOP_START_ASKED);
        sGuestPairTimer = 0;
        Coop_ClockStart(&sGuestClock);
        ScriptContext_SetupScript(Coop_EventScript_StoryAskGuest);
        return;
    }
    // Both asked for the same fight: this game joins the partner's (link
    // player 0's request won, CoopProtocol_KeepOwnInvite), and both go on
    // to choose their teams.
join:
    Coop_HideCallingMessage();
    sWaitingAck = FALSE;
    sResumeScript = FALSE;
    sGuestPairInvite = *invite;
    CoopPrep_BeginGuest(invite->seq, playerId);
}

// The partner's shared story fight, accepted: the same battle on this game.
static void Coop_AcceptMandatory(const struct CoopInvite *invite)
{
    sBattleKind = invite->kind;
    sBattleBadge = invite->badge;
    sBattleFlag = invite->aux;
    sLegendKind = invite->kind == COOP_KIND_LEGEND ? (u8)invite->aux : 0;
    sRivalFightStage = invite->kind == COOP_KIND_RIVAL ? (u8)invite->badge : 0xFF;
    TRAINER_BATTLE_PARAM.opponentA = invite->trainerA;
    TRAINER_BATTLE_PARAM.opponentB = invite->trainerB;
    sRemoteTeamKey = invite->teamKey;

    Coop_SendReply(COOP_KIND_ACK, invite->seq);
    Coop_LaunchBattle(TRUE);
}

// Co-op blocks that reached a link battle's block reads (a reply flushed
// just before the fight, a story message in flight). Invites, replies and
// story messages are kept for after the fight; deal and dex messages are
// resent by their senders and are dropped.
#define COOP_STASH_COUNT 4
#define COOP_STASH_WORDS 8
STATIC_ASSERT(sizeof(struct CoopInvite) <= COOP_STASH_WORDS * 2, CoopInviteFitsStash);
STATIC_ASSERT(sizeof(struct CoopStoryMsg) <= COOP_STASH_WORDS * 2, CoopStoryFitsStash);
static EWRAM_DATA u16 sStash[COOP_STASH_COUNT][COOP_STASH_WORDS] = {0};
static EWRAM_DATA u8 sStashFrom[COOP_STASH_COUNT] = {0};
static EWRAM_DATA u8 sStashCount = 0;
// Test visibility: co-op blocks taken out of battle reads (kept or dropped).
EWRAM_DATA u16 gCoopStrayBattleBlocks = 0;

static bool8 Coop_IsCoopBlock(const u16 *words)
{
    switch (words[0])
    {
    case COOP_INVITE_MAGIC:
        return ((const struct CoopInvite *)words)->checksum == Coop_InviteChecksum((const struct CoopInvite *)words);
    case COOP_STORY_MAGIC:
        return ((const struct CoopStoryMsg *)words)->checksum == CoopProtocol_StoryChecksum((const struct CoopStoryMsg *)words);
    case COOP_DEAL_MAGIC:
    case COOP_DEX_MAGIC:
        return CoopDeal_IsLinkBlock(words);
    case COOP_PREP_MAGIC:
        return CoopPrep_IsLinkBlock(words);
    }
    return FALSE;
}

u8 Coop_BattleBlockStatus(void)
{
    u8 status = GetBlockReceivedStatus();
    u32 i;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (!(status & (1 << i)) || !Coop_IsCoopBlock(gBlockRecvBuffer[i]))
            continue;
        if ((gBlockRecvBuffer[i][0] == COOP_INVITE_MAGIC || gBlockRecvBuffer[i][0] == COOP_STORY_MAGIC)
         && sStashCount < COOP_STASH_COUNT)
        {
            memcpy(sStash[sStashCount], gBlockRecvBuffer[i], sizeof(sStash[0]));
            sStashFrom[sStashCount] = i;
            sStashCount++;
        }
        if (gCoopStrayBattleBlocks < 0xFFFF)
            gCoopStrayBattleBlocks++;
        ResetBlockReceivedFlag(i);
        status &= ~(1 << i);
    }
    return status;
}

bool8 Coop_TestSendReply(u8 kind, u16 seq)
{
    struct CoopInvite reply;

    if (sLinkState != COOP_LINK_UP || !IsLinkTaskFinished())
        return FALSE;
    memset(&reply, 0, sizeof(reply));
    reply.magic = COOP_INVITE_MAGIC;
    reply.kind = kind;
    reply.seq = seq;
    reply.checksum = Coop_InviteChecksum(&reply);
    return SendBlock(BitmaskAllOtherLinkPlayers(), &reply, sizeof(reply));
}

static void Coop_HandleBlock(u32 playerId, const u16 *words)
{
    if (words[0] == COOP_INVITE_MAGIC)
        Coop_HandleInvite(playerId, (const struct CoopInvite *)words);
    else if (words[0] == COOP_STORY_MAGIC)
        Coop_HandleStoryMsg(playerId, (const struct CoopStoryMsg *)words);
}

static void Coop_PollBlocks(void)
{
    u8 status;
    u32 i;

    if (sStashCount != 0)
    {
        u16 words[COOP_STASH_WORDS];
        u8 from = sStashFrom[0];

        memcpy(words, sStash[0], sizeof(words));
        sStashCount--;
        for (i = 0; i < sStashCount; i++)
        {
            memcpy(sStash[i], sStash[i + 1], sizeof(sStash[0]));
            sStashFrom[i] = sStashFrom[i + 1];
        }
        sHandlingStash = TRUE;
        Coop_HandleBlock(from, words);
        sHandlingStash = FALSE;
    }
    status = GetBlockReceivedStatus();

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (status & (1 << i))
        {
            u16 magic = gBlockRecvBuffer[i][0];
            if (magic == COOP_INVITE_MAGIC)
                Coop_HandleInvite(i, (const struct CoopInvite *)gBlockRecvBuffer[i]);
            else if (magic == COOP_DEAL_MAGIC)
                CoopDeal_HandleBlock(i, gBlockRecvBuffer[i]);
            else if (magic == COOP_DEX_MAGIC)
                CoopDex_HandleBlock(i, gBlockRecvBuffer[i]);
            else if (magic == COOP_PREP_MAGIC)
                CoopPrep_HandleBlock(i, gBlockRecvBuffer[i]);
            else if (magic == COOP_STORY_MAGIC)
                Coop_HandleStoryMsg(i, (const struct CoopStoryMsg *)gBlockRecvBuffer[i]);
            else
                continue;
            ResetBlockReceivedFlag(i);
        }
    }
}

static void Coop_FailWait(void)
{
    u8 kind = sBattleKind;

    Coop_HideCallingMessage();
    sPartnerBusyNow = FALSE;
    // Withdraw a question the partner may still be reading, so a late YES
    // gets "too late" instead of a fight that is not coming.
    if (sWaitingAck && (Coop_IsOptionalKind(kind) || Coop_IsMandatoryKind(kind)) && sInviteSent)
        Coop_SendReply(COOP_KIND_WITHDRAW, sInvite.seq);
    sWaitingAck = FALSE;
    sWaitKind = 0;
    sRescueEcho = 0;
    sRequest = COOP_REQ_NONE;
    gSpecialVar_Result = FALSE;
    if (kind == COOP_KIND_RESCUE)
    {
        sBirchBag = FALSE;
        sResumeScript = FALSE;
        sRescueField = COOP_RESCUE_RETURN_NONE;
        sRescueResult = 2;
        gSpecialVar_Result = 2;
        ScriptContext_Enable();
        return;
    }
    // Optional fights carry on alone: the first wild Pokémon, or the NPC
    // that was walked into. Nothing is shown, the stock battle just starts.
    if (kind == COOP_KIND_WILD)
    {
        ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][1]);
        BattleSetup_StartWildBattle();
        return;
    }
    if (kind == COOP_KIND_TRAINER || kind == COOP_KIND_PAIR)
    {
        if (kind == COOP_KIND_PAIR)
            TRAINER_BATTLE_PARAM.opponentA = sPairEngaged;
        sResumeScript = FALSE;
        BattleSetup_ResumeSoloTrainerBattle();
        return;
    }
    if (sResumeScript)
    {
        sResumeScript = FALSE;
        ScriptContext_Enable();
    }
    else if (kind == COOP_KIND_RIVAL)
    {
        // The rival scene walked the twins up: say why and put them back,
        // so the scene plays again in full later.
        sPendingMessage = Coop_EventScript_RivalWaits;
        ScriptContext_SetupScript(Coop_EventScript_RivalWaits);
    }
    else if (ScriptContext_IsEnabled())
    {
        sPendingMessage = Coop_EventScript_PartnerDeclined;
        ScriptContext_SetupScript(Coop_EventScript_PartnerDeclined);
    }
    else
    {
        sPendingMessage = Coop_EventScript_PartnerDeclined;
    }
}

static void Coop_PollInviteTimeout(void)
{
    u8 secs;

    if (!sWaitingAck || CoopPrep_Active())
        return;
    // B leaves the rescue wait so a partner stuck in a menu does not
    // freeze the person who opened the bag. For an optional fight it means
    // "fight alone now" instead of waiting out the partner's answer; for a
    // shared story fight, "stop waiting" (nothing starts, nothing is used).
    if ((sBattleKind == COOP_KIND_RESCUE || sBattleKind == COOP_KIND_WILD || sBattleKind == COOP_KIND_TRAINER)
     && JOY_NEW(B_BUTTON))
    {
        Coop_FailWait();
        return;
    }
    if (Coop_IsMandatoryKind(sBattleKind) && sInviteTimer >= 20 && JOY_NEW(B_BUTTON))
    {
        sRefusal = COOP_REFUSE_CANCELLED;
        Coop_FailWait();
        return;
    }
    Coop_SendInvite();
    if (sInviteTimer < 0xFFFF)
        sInviteTimer++;
    if (sBattleKind == COOP_KIND_RESCUE)
    {
        if (sInviteTimer >= COOP_RESCUE_TIMEOUT)
            Coop_FailWait();
        return;
    }
    secs = Coop_ClockTick(&sInviteClock);
    if (sRefusal == COOP_REFUSE_PARTNER_BUSY)
    {
        u8 busySecs = Coop_ClockTick(&sBusyClock);

        // No busy answer for two seconds: the partner is free and has the
        // question in front of them now.
        if (sPartnerBusyNow && secs >= 2)
            sPartnerBusyNow = FALSE;
        if (sPartnerBusyNow)
        {
            if (busySecs >= COOP_WAIT_SECONDS)
            {
                Coop_FailWait();   // stayed busy the whole time
                return;
            }
            secs = busySecs;
        }
    }
    gCoopWaitSecondsLeft = secs >= COOP_WAIT_SECONDS ? 0 : COOP_WAIT_SECONDS - secs;
    // Say why the field stopped while the partner decides, and how long.
    if ((Coop_IsOptionalKind(sBattleKind) && sBattleKind != COOP_KIND_PAIR && sBattleKind != COOP_KIND_LEGEND)
     || Coop_IsMandatoryKind(sBattleKind))
    {
        if (sInviteTimer >= 20)
            Coop_DrawWaitNote(gCoopWaitSecondsLeft);
    }
    if (secs >= COOP_WAIT_SECONDS)
    {
        if (Coop_IsOptionalKind(sBattleKind))
            sSoloNote = COOP_SOLO_NO_ANSWER;
            // (A partner who stayed busy was handled above.)
        if (Coop_IsMandatoryKind(sBattleKind) && !sPartnerBusyNow)
            sRefusal = COOP_REFUSE_NO_ANSWER;
        if (!Coop_IsMandatoryKind(sBattleKind) && sRefusal == COOP_REFUSE_PARTNER_BUSY)
            sRefusal = COOP_REFUSE_NONE;
        Coop_FailWait();
    }
}

// The rescue used to travel as a link block, then the other game refused
// it while any script or text box was open. That looked like "still in a
// menu" after the bag had already closed. The position frame is enough:
// the partner echoes the case once they are standing on the route.
static void Coop_EndRescueRequest(void)
{
    if (sRequest == COOP_REQ_RESCUE)
        sRequest = COOP_REQ_NONE;
    if (sWaitKind == COOP_REQ_RESCUE)
        sWaitKind = 0;
    sRescueEcho = 0;
    sRescueDone = TRUE;
}

static void Coop_PollRescue(void)
{
    if (gMain.inBattle || sInBattle || !sPartnerValid)
        return;

    if (sRescueDone)
    {
        if (sRequest == COOP_REQ_RESCUE)
            sRequest = COOP_REQ_NONE;
        return;
    }

    if (sWaitKind == COOP_REQ_RESCUE)
    {
        if (JOY_NEW(B_BUTTON) || ++sInviteTimer >= COOP_RESCUE_TIMEOUT)
        {
            Coop_FailWait();
            return;
        }
        if (sPartner.request == COOP_REQ_RESCUE && sPartner.slot == sSlot)
        {
            sRemoteTeamKey = 0;
            Coop_LaunchBattle(FALSE);
        }
        return;
    }

    if (sPartner.request != COOP_REQ_RESCUE || sPartner.slot > 2)
    {
        sRescueEcho = 0;
        return;
    }
    // This game is the one still in the bag, or a fade is up. Wait.
    if (sBirchBag || gMain.callback2 != CB2_Overworld)
        return;
    if (ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
        return;

    sRequest = COOP_REQ_RESCUE;
    sSlot = sPartner.slot;
    // Give the echo a few frames to reach the other game before this
    // one leaves the overworld and stops sending.
    if (++sRescueEcho < 8)
        return;

    Coop_GiveRescueStarter(sPartner.slot, FALSE);
    sBattleKind = COOP_KIND_RESCUE;
    sBattleBadge = 0;
    sBattleFlag = 0;
    sLegendKind = 0;
    sRivalFightStage = 0xFF;
    sBirchBag = FALSE;
    sRescueEcho = 0;
    sRemoteTeamKey = 0;
    Coop_LaunchBattle(TRUE);
}

static void Coop_PollScriptWaits(void)
{
    if (sWaitKind == COOP_REQ_CASE && sPartnerValid && sPartner.request == COOP_REQ_CASE)
    {
        gSpecialVar_Result = sPartner.slot == sProposedCase ? sProposedCase : COOP_CASE_NONE;
        sRequest = COOP_REQ_NONE;
        sWaitKind = 0;
        ScriptContext_Enable();
    }
    else if (sWaitKind == COOP_REQ_CLAIM && sPartnerValid && sPartner.request == COOP_REQ_CLAIM)
    {
        struct CoopSave *save = CoopSave();
        u8 mine = sClaim;
        u8 theirs = sPartner.slot;
        u8 index = mine;
        u8 caseId = save != NULL ? save->caseId : 0;

        if (mine == theirs)
            index = GetMultiplayerId() == 0 ? mine : (u8)(mine ^ 1);
        if (caseId > 2)
            caseId = sProposedCase <= 2 ? sProposedCase : 0;
        ZeroPlayerPartyMons();
        ScriptGiveMon(gCoopCaseStarters[caseId][index & 1], 5, ITEM_NONE);
        if (save != NULL)
            save->rivalId = CoopProtocol_RivalForSlot(index & 1);
        VarSet(VAR_STARTER_MON, caseId);
        if (save != NULL)
            save->caseId = caseId;
        sRequest = COOP_REQ_NONE;
        sWaitKind = 0;
        gSpecialVar_Result = TRUE;
        ScriptContext_Enable();
    }
}

static void Coop_MaintainCallback(void)
{
    if (!gMain.inBattle && gReceivedRemoteLinkPlayers && !gLinkTransferringData && gLinkCallback == NULL)
        gLinkCallback = Coop_LinkCallback;
}

static void Coop_FinishLinkWait(u16 result)
{
    sLinkWait = FALSE;
    gSpecialVar_Result = result;
    ScriptContext_Enable();
}

// Ends any fade and shows the field's real colours. Copying the unfaded
// palettes alone dropped the time-of-day tint and the weather colour map,
// so a night screen turned bright when the link came up or the CO-OP menu
// opened, and stayed bright until the next map load.
void Coop_RestoreFieldPalettes(void)
{
    gPaletteFade.active = FALSE;
    gPaletteFade.y = 0;
    gPaletteFade.targetY = 0;
    gPaletteFade.yDec = FALSE;
    UnfadePlttBuffer(PALETTES_ALL);
    if (gMain.callback2 == CB2_Overworld || gMain.callback2 == CB2_OverworldBasic)
    {
        UpdateTimeOfDay(TRUE);
        ApplyWeatherColorMapIfIdle(gWeatherPtr->colorMapIndex);
    }
}

static void Coop_ForceScreenOn(void)
{
    Coop_RestoreFieldPalettes();
    if ((REG_DISPCNT & DISPCNT_BG_ALL_ON) == 0)
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON);
}

static const u8 sText_LinkStatus[] = _("Cable players: {STR_VAR_1}.\nPress B to cancel.$");

static void Coop_RefreshLinkText(void)
{
    u8 count = GetLinkPlayerCount_2();

    if (count > sLinkPlayersSeen)
        sLinkPlayersSeen = count;
    if (count == sLinkStatusCount)
        return;
    sLinkStatusCount = count;
    Coop_ForceScreenOn();
    ConvertIntToDecimalStringN(gStringVar1, count, STR_CONV_MODE_LEFT_ALIGN, 1);
    StringExpandPlaceholders(gStringVar4, sText_LinkStatus);
    DrawDialogueFrame(0, TRUE);
    AddTextPrinterParameterized(0, FONT_NORMAL, gStringVar4, 0, 1, 0, NULL);
}

// Stock cable-club reconnect: wait until both GBAs are on the wire, let the
// master send one handshake pulse, then wait for the player exchange.
// Pulsing every frame skips the master/slave check and the link never comes up.
static void Coop_PollHandshake(void)
{
    if (sLinkState != COOP_LINK_CONNECTING)
        return;
    if (gLinkErrorOccurred)
    {
        Coop_Drop(sLinkWait ? NULL : Coop_EventScript_Disconnected);
        return;
    }
    switch (sLinkPhase)
    {
    case 0:
        Coop_RefreshLinkText();
        if (GetLinkPlayerCount_2() >= 2)
        {
            // The count arrives before the names and trainer ids. Saving here
            // stored two empty players, and the first link battle then closed
            // the cable as a communication error.
            sLinkPhaseTimer = 0;
            sLinkPhase = IsLinkMaster() ? 1 : 2;
        }
        break;
    case 1:
        if (++sLinkPhaseTimer > 30)
        {
            CheckShouldAdvanceLinkState();
            sLinkPhase = 2;
        }
        break;
    case 2:
        if (gReceivedRemoteLinkPlayers && IsLinkPlayerDataExchangeComplete())
        {
            if (!sHandshakeSaved && GetLinkPlayerCount_2() >= 2)
            {
                SaveLinkPlayers(GetLinkPlayerCount_2());
                sHandshakeSaved = TRUE;
            }
            sLinkState = COOP_LINK_UP;
            gCoopHoldLink = TRUE;
            sLinkErrorStreak = 0;
            sRescueDone = FALSE;
            sRivalFightStage = 0xFF;
            sProposedCase = COOP_CASE_NONE;
            sClaim = COOP_CASE_NONE;
            gLinkCallback = Coop_LinkCallback;
            SetSuppressLinkErrorMessage(TRUE);
            CoopDeal_OnSessionStart();
            CoopLegend_OnSessionStart();
        }
        break;
    }
}

static void Coop_PollLinkWait(void)
{
    if (!sLinkWait)
        return;
    Coop_ForceScreenOn();
    Coop_RefreshLinkText();
    if (sPendingMessage != NULL)
    {
        Coop_FinishLinkWait(2);
        return;
    }
    if (sLinkState == COOP_LINK_UP && sPartnerValid)
    {
        StringCopy(gStringVar1, gLinkPlayers[GetMultiplayerId() ^ 1].name);
        Coop_FinishLinkWait(1);
        return;
    }
    if (sLinkState == COOP_LINK_IDLE)
    {
        Coop_FinishLinkWait(0);
        return;
    }
    if (JOY_NEW(B_BUTTON) || ++sLinkWaitTimer >= COOP_LINK_WAIT_LIMIT)
    {
        u16 result = sLinkPlayersSeen >= 2 ? 3 : 0;

        if (sLinkState == COOP_LINK_CONNECTING)
            Coop_Drop(NULL);
        Coop_FinishLinkWait(result);
    }
}

static bool8 Coop_PlayerLost(void)
{
    switch (gBattleOutcome)
    {
    case B_OUTCOME_LOST:
    case B_OUTCOME_DREW:
    case B_OUTCOME_FORFEITED:
        return TRUE;
    default:
        return FALSE;
    }
}

static void Task_CoopEnableScript(u8 taskId)
{
    ScriptContext_Enable();
    DestroyTask(taskId);
}

static void Coop_NoteLinkHealth(void)
{
    if (sLinkState != COOP_LINK_UP)
        return;
    // A co-op fight still needs a real error to reach the battle abort.
    if (gMain.inBattle && COOP_SESSION_LINK_BATTLE())
        return;

    // Transport error bits and idle serial words are not a peer heartbeat.
    // Count time since an accepted co-op frame instead, including while a
    // menu/script is open. Keep session intent and Beacon through outages.
    Link_ConsumeErrors();
    gCoopHoldLink = TRUE;
    SetSuppressLinkErrorMessage(TRUE);

    if (sLinkErrorStreak < 0xFFFF)
        sLinkErrorStreak++;
    if (sLinkErrorStreak == 45 || sLinkErrorStreak % 60 == 0)
        Link_RecoverCoopCable();
    if (sLinkErrorStreak >= COOP_LINK_QUIET_LIMIT)
    {
        if (sPartnerValid)
            Coop_RemoveSprite();
        sPartnerValid = FALSE;
        sPartnerPingKnown = FALSE;
        // A late YES must not start a battle after its peer went away.
        if (sGuestPairPending)
            sGuestPairTimer = 0xFFFF;
    }
}

static void Coop_Drop(const u8 *message)
{
    // Milestones finished during the session (not yet read back at a map
    // change) were shared with this partner.
    if (sMerged && sSessionPartnerOt != 0)
        Coop_RefreshLocalStoryWith(sSessionPartnerOt);
    // A pending invite would otherwise leave the field frozen or the
    // trainer script waiting on an answer that can't arrive.
    gCoopHoldLink = FALSE;
    // The shared time of day ends with the session: re-tint from this
    // game's own clock on the next overworld frame.
    gTimeUpdateCounter = 0;
    sBirchBag = FALSE;
    sRescueField = COOP_RESCUE_RETURN_NONE;
    if (sRescueGuestFollow)
        message = Coop_EventScript_WarpToLab;
    sRescueGuestFollow = FALSE;
    sLinkErrorStreak = 0;
    sBadFrameStreak = 0;
    CoopPrep_LinkDown();
    if ((sWaitingAck || sWaitKind == COOP_REQ_RESCUE) && !gMain.inBattle)
        Coop_FailWait();
    sRescueEcho = 0;
    sGuestPairPending = FALSE;
    sLinkState = COOP_LINK_IDLE;
    sUnlinkTimer = 0;
    sLinkPhase = 0;
    sLinkPhaseTimer = 0;
    sHandshakeSaved = FALSE;
    sMerged = FALSE;
    sPartnerValid = FALSE;
    sPartnerPingKnown = FALSE;
    sLastSeenValid = FALSE;
    CoopTalk_Reset();
    sWaitingAck = FALSE;
    sResumeScript = FALSE;
    sRequest = COOP_REQ_NONE;
    memset(&sBeacon, 0, sizeof(sBeacon));
    sBeaconQueued = 0;
    sBeaconPrompted = 0;
    // The next session may be a rebooted partner whose numbers start over.
    sStorySeqKnown = FALSE;
    sStorySentValid = FALSE;
    sSceneAnnounce = COOP_SCENE_NONE;
    sSceneJoin = COOP_SCENE_NONE;
    if (sJoinLaunchPending)
    {
        sJoinLaunchPending = FALSE;
        sInBattle = FALSE;
    }
    gLinkCallback = NULL;
    Coop_RemoveSprite();
    CloseLink();
    SetSuppressLinkErrorMessage(TRUE);
    if (sPendingMessage != Coop_EventScript_WarpToLab)
        sPendingMessage = message;
}

// In a solo (non-link) fight the block buffers are free. An optional invite
// that crossed the start of this fight would otherwise wait unread until it
// ends, while the inviter sits out its timeout: answer "busy" now. Other
// blocks stay unread for after the fight.
static void Coop_RefuseInvitesInSoloBattle(void)
{
    u8 status;
    u32 i;

    if (sInBattle || (gBattleTypeFlags & BATTLE_TYPE_LINK) || gLinkTransferringData)
        return;
    status = GetBlockReceivedStatus();
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        const struct CoopInvite *invite = (const struct CoopInvite *)gBlockRecvBuffer[i];

        if (!(status & (1 << i)) || i == GetMultiplayerId())
            continue;
        if (invite->magic != COOP_INVITE_MAGIC || invite->checksum != Coop_InviteChecksum(invite))
            continue;
        if (!Coop_IsOptionalKind(invite->kind))
            continue;
        ResetBlockReceivedFlag(i);
        Coop_SendReplyWhy(COOP_KIND_NACK, invite->seq, COOP_SOLO_BUSY);
    }
    Coop_FlushReply();
}

static void Task_CoopSession(u8 taskId)
{
    if (sLinkState == COOP_LINK_CONNECTING)
        Coop_PollHandshake();
    Coop_PollLinkWait();
    if (sLinkState == COOP_LINK_UP)
    {
        SetSuppressLinkErrorMessage(TRUE);
        // Battle block transfers live in the same buffer. Reading them here
        // clears the flag, the fight never sees the party, and both screens
        // stay black while the music plays.
        if (gMain.inBattle)
        {
            Coop_RefuseInvitesInSoloBattle();
            return;
        }
        if (sUnlinkTimer != 0)
        {
            sRequest = COOP_REQ_UNLINK;
            Coop_MaintainCallback();
            if (--sUnlinkTimer == 0)
                Coop_Drop(NULL);
            return;
        }
        Coop_MaintainCallback();
        Coop_FlushReply();
        if (gMain.callback1 != CB1_Overworld)
            Coop_UpdateSprite();
        Coop_AnimateTalkNpc();
        Coop_PollBlocks();
        Coop_PollRescue();
        Coop_PollScriptWaits();
        CoopDeal_Poll();
        CoopPrep_Poll();
        Coop_PollBeacon();
        Coop_PollInviteTimeout();
        Coop_PollStory();
        if (sGuestPairPending && sGuestPairTimer < 0xFFFF)
            sGuestPairTimer = Coop_ClockTick(&sGuestClock);
        // Nobody answered the join question in time (or the inviter stopped
        // waiting): close it as a NO, so the prompt does not sit on screen
        // for a fight that has already gone ahead. The ask script then says
        // so ("couldn't wait") and tells the inviter.
        // A question still on its first page (nobody pressed anything) has
        // no YES/NO box to cancel yet: its script is replaced by the answer
        // step, which then sees the time ran out (the ask scripts do nothing
        // else before the answer).
        if (sGuestPairPending
         && sGuestPairTimer >= COOP_ANSWER_SECONDS
         && !ScriptMenu_CancelYesNo() && !sGuestAskClosed)
        {
            sGuestAskClosed = TRUE;
            HideFieldMessageBox();
            gSpecialVar_Result = FALSE;
            ScriptContext_SetupScript(Coop_EventScript_JoinAskAnswer);
        }
        if (sRescueGuestFollow && !gMain.inBattle)
        {
            if (sRescueGuestTimer < 0xFFFF)
                sRescueGuestTimer++;
            // Follow the partner into the lab once their speech is done.
            // If that never arrives, walk there anyway.
            if (!ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked() && !Coop_InteractionBusy()
             && (Coop_PartnerInBirchLab() || sRescueGuestTimer >= COOP_RESCUE_TIMEOUT))
            {
                sRescueGuestFollow = FALSE;
                sPendingMessage = Coop_EventScript_WarpToLab;
            }
        }
        Coop_NoteLinkHealth();
    }
    if (sProgressPending && !sInBattle && !gMain.inBattle
     && !ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked())
    {
        sProgressPending = FALSE;
        ScriptContext_SetupScript(Coop_EventScript_Progression);
    }
    if (sJoinLaunchPending && !ScriptContext_IsEnabled())
    {
        if (sReplyKind == 0 && (gLinkCallback == NULL || gLinkCallback == Coop_LinkCallback))
        {
            // A queued ACK has to leave the cable before battle code takes
            // ownership of its block buffer and stops flushing field replies.
            // Once it has left, the inviter is already starting the fight and
            // has stopped sending field frames, so a quiet partner here is
            // expected, not a reason to back out. (A withdrawn invite cancels
            // this in Coop_HandleInvite.)
            sJoinLaunchPending = FALSE;
            Coop_LaunchBattle(TRUE);
            return;
        }
        if (!Coop_SessionActive() || !sPartnerValid)
        {
            sJoinLaunchPending = FALSE;
            sInBattle = FALSE;
            sReplyKind = 0;
            sPendingMessage = Coop_EventScript_PartnerDeclined;
        }
    }
    if (sPendingMessage != NULL && !gMain.inBattle && !ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked())
    {
        const u8 *message = sPendingMessage;
        sPendingMessage = NULL;
        ScriptContext_SetupScript(message);
    }
    (void)taskId;
}

// Exactly one session task. Callers that expect a ResetTasks to have run
// (Coop_ResumeAfterTasksReset) are also reached when it did not (the bag, a
// battle's end, Flash): a second copy then piled up each time, every copy
// polled the link, and eight of them filled the task table so that a long
// battle's move animation could not create its tasks (CreateTask then hands
// back task 0) and waited forever (journey 70, Rayquaza's HYPER VOICE).
static void Coop_EnsureTask(void)
{
    u8 taskId = FindTaskIdByFunc(Task_CoopSession);

    if (taskId == TASK_NONE)
        taskId = CreateTask(Task_CoopSession, 80);
    sTaskId = taskId;
}

// Door transitions call ResetTasks, which deletes the session task.
// The link itself is still open. Put the task back before the fade ends.
void Coop_ResumeAfterTasksReset(void)
{
    if (sLinkState != COOP_LINK_UP)
        return;
    gCoopHoldLink = TRUE;
    SetSuppressLinkErrorMessage(TRUE);
    // Doors, the clock, and the starter bag all wipe the sprite list.
    // A half-sent frame from that moment also leaves the cable unable
    // to deliver the partner's new map, so the sprite never returns.
    Link_RecoverCoopCable();
    if (gLinkCallback == NULL && gReceivedRemoteLinkPlayers && !gMain.inBattle)
        gLinkCallback = Coop_LinkCallback;
    sTaskId = TASK_NONE;
    sSpriteGfx = 0xFFFF;
    sPartnerResettle = TRUE;
    sPartnerStepWait = 0;
    Coop_EnsureTask();
}

// The battle's block transfers own the cable until the fight returns.
void Coop_YieldLinkToBattle(void)
{
    gCoopHoldLink = TRUE;
    SetSuppressLinkErrorMessage(TRUE);
    if (gLinkCallback == Coop_LinkCallback)
        gLinkCallback = NULL;
}

// A script that walked the player has released. Place the partner again
// instead of leaving the last cutscene step running.
void Coop_SettlePartner(void)
{
    if (sLinkState != COOP_LINK_UP)
        return;
    sPartnerResettle = TRUE;
    sPartnerStepWait = 0;
}

// Frames the field has been free (no script, controls unlocked), and how
// long it had been free when the current script or menu began.
static EWRAM_DATA u8 sFieldFreeFrames = 0;
static EWRAM_DATA u8 sFreeBeforeScript = 0;
#define COOP_PARTNER_TALK_SETTLE 20

// The partner's position in this map's grid (with MAP_OFFSET, like
// currentCoords), when linked and on the same map.
// The partner's map and what they are doing (src/coop_npc.c).
bool8 Coop_PartnerFrameInfo(u8 *mapGroup, u8 *mapNum, u8 *activity)
{
    if (!Coop_SessionActive() || !sPartnerValid)
        return FALSE;
    *mapGroup = sPartner.mapGroup;
    *mapNum = sPartner.mapNum;
    *activity = sPartner.activity;
    return TRUE;
}

bool8 Coop_PartnerMapPos(s16 *x, s16 *y)
{
    if (!sPartnerValid || !Coop_PartnerOnSameMap())
        return FALSE;
    *x = sPartner.x;
    *y = sPartner.y;
    return TRUE;
}

// Where the partner stands in this map's grid, on this map or on a map
// joined to it by a connection (inside the strip loaded here), for the edge
// arrow. FALSE for other maps, interiors and anything not loaded.
bool8 Coop_PartnerViewCoords(s16 *x, s16 *y)
{
    return Coop_PartnerLocalCoords(x, y);
}

// While the link recovers: the last tile the partner was drawn on, if that
// was on this map.
bool8 Coop_PartnerLastSeen(s16 *x, s16 *y)
{
    if (!sLastSeenValid || sLastSeenGroup != gSaveBlock1Ptr->location.mapGroup
     || sLastSeenNum != gSaveBlock1Ptr->location.mapNum)
        return FALSE;
    *x = sLastSeenX;
    *y = sLastSeenY;
    return TRUE;
}

// The partner marker's icon (COOP_MARK_*): what the partner is doing, from
// their latest frame, so nothing outlives the state it shows. COOP_MARK_NONE
// when there is no partner to mark.
u8 Coop_PartnerMarkKind(void)
{
    if (sLinkState != COOP_LINK_UP)
        return COOP_MARK_NONE;
    if (!Coop_SessionActive())
        return COOP_MARK_BROKEN;
    if (!sPartnerValid)
        return COOP_MARK_NONE;
    if (sPartner.talkLocalId == COOP_TALK_IN_BATTLE)
        return COOP_MARK_BATTLE;
    if (sPartner.ext & COOP_EXT_INVITE)
        return COOP_MARK_INVITE;
    if (sPartner.ext & COOP_EXT_WAITING)
        return COOP_MARK_WAIT;
    switch (sPartner.activity & 3)
    {
    case COOP_ACT_TALKING:
        return COOP_MARK_TALK;
    case COOP_ACT_MENU:
    case COOP_ACT_BUSY:
        return COOP_MARK_MENU;
    }
    return COOP_MARK_WALK;
}

// The partner's own frame says they are following this player.
bool8 Coop_PartnerIsFollowing(void)
{
    return sPartnerValid && Coop_SessionActive() && (sPartner.ext & COOP_EXT_FOLLOWING);
}

// PING (partner menu): a cue on the partner's screen. FALSE while the
// cooldown runs.
bool8 Coop_SendPing(void)
{
    if (sPingCooldown != 0 || !Coop_SessionActive())
        return FALSE;
    sPingCount = (sPingCount + 1) & 3;
    sPingCooldown = COOP_PING_COOLDOWN;
    return TRUE;
}

u16 Coop_PingCooldownSeconds(void)
{
    return (sPingCooldown + 59) / 60;
}

// The partner's frame: whether their conversation can be joined, which NPC,
// and their map.
bool8 Coop_PartnerTalkInfo(u8 *localId, bool8 *joinable)
{
    // The talk id is sent only while that player's script runs, so it is
    // the signal; a script waiting on a checkpoint reads as busy, not talking.
    if (!Coop_PartnerOnSameMap() || (sPartner.activity & 3) == COOP_ACT_EXPLORING
     || sPartner.talkLocalId == 0 || sPartner.talkLocalId == 0xFF || sPartner.talkLocalId == COOP_TALK_IN_BATTLE)
        return FALSE;
    *localId = sPartner.talkLocalId;
    *joinable = (sPartner.ext & COOP_EXT_JOINABLE) != 0;
    return TRUE;
}

bool8 Coop_PartnerWaiting(void)
{
    return sPartnerValid && Coop_SessionActive() && (sPartner.ext & COOP_EXT_WAITING);
}

// The partner's map, tile and movement mode (0 on foot, 1 bike, 2 surf) from
// their latest live frame (follow mode's history).
bool8 Coop_PartnerMoveInfo(u8 *group, u8 *num, s16 *x, s16 *y, u8 *mode)
{
    if (!sPartnerValid || !Coop_SessionActive())
        return FALSE;
    *group = sPartner.mapGroup;
    *num = sPartner.mapNum;
    *x = sPartner.x;
    *y = sPartner.y;
    *mode = sPartner.surf ? 2 : sPartner.bike ? 1 : 0;
    return TRUE;
}

u8 Coop_PartnerActivity(void)
{
    return sPartnerValid ? (sPartner.activity & 3) : COOP_ACT_BUSY;
}

// The partner's sprite on this map (and gender), or MAX_SPRITES.
u8 Coop_PartnerSpriteId(u8 *gender)
{
    u8 i;

    *gender = sPartner.gender;
    if (!sPartnerValid || !sPartnerDrawn)
        return MAX_SPRITES;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *obj = &gObjectEvents[i];

        if (obj->active && obj->localId == OBJ_EVENT_ID_COOP_PARTNER
         && obj->spriteId < MAX_SPRITES && gSprites[obj->spriteId].inUse)
            return obj->spriteId;
    }
    return MAX_SPRITES;
}

// A self-check of the partner's picture, run after it is drawn each field
// frame: at most one partner object; drawn only where the partner really is
// (this map or a connected one); its graphics are the partner's current
// movement mode (on foot, bike, Surf); the palette is the partner's current
// outfit; the marker sits over the drawn sprite. Each failed frame counts
// in gCoopAppearanceFaults (a harness reads it across warps, bikes, Surf and
// outfit changes); nothing in play depends on it.
EWRAM_DATA u16 gCoopAppearanceFaults = 0;
EWRAM_DATA u8 gCoopAppearanceLastFault = 0;

static void Coop_AppearanceFault(u8 kind)
{
    if (gCoopAppearanceFaults < 0xFFFF)
        gCoopAppearanceFaults++;
    gCoopAppearanceLastFault = kind;
}

static void Coop_CheckAppearance(void)
{
    u8 i, count = 0, objectId = OBJECT_EVENTS_COUNT;
    s16 x, y;

    if (gMain.callback2 != CB2_Overworld)
        return;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == OBJ_EVENT_ID_COOP_PARTNER)
        {
            count++;
            objectId = i;
        }
    }
    if (count > 1)
        Coop_AppearanceFault(1);
    if (objectId == OBJECT_EVENTS_COUNT)
        return;
    if (sLinkState != COOP_LINK_UP)
    {
        Coop_AppearanceFault(2);
        return;
    }
    // A quiet link keeps the last picture until it mends or times out.
    if (!sPartnerValid || !Coop_SessionActive())
        return;
    if (!Coop_PartnerLocalCoords(&x, &y))
        Coop_AppearanceFault(3);
    else if (gObjectEvents[objectId].graphicsId != Coop_PartnerGfx(&sPartner))
        Coop_AppearanceFault(4);
    else if (!CoopOutfit_PartnerLookCurrent())
        Coop_AppearanceFault(5);
    else if (!CoopOutfit_MarkerOnSprite(gObjectEvents[objectId].spriteId))
        Coop_AppearanceFault(6);
}

void Coop_OverworldFrame(void)
{
    if (sPartnerPopupCooldown != 0)
        sPartnerPopupCooldown--;
    if (sPingCooldown != 0)
        sPingCooldown--;
    if (gCoopPingCue != 0)
        gCoopPingCue--;
    if (sPartnerValid && sPartner.activity == COOP_ACT_BUSY)
    {
        if (sPartnerBusyFrames < 0xFFFF)
            sPartnerBusyFrames++;
    }
    else
    {
        sPartnerBusyFrames = 0;
    }
    if (ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
    {
        if (sFieldFreeFrames != 0)
            sFreeBeforeScript = sFieldFreeFrames;
        sFieldFreeFrames = 0;
    }
    else if (sFieldFreeFrames < 255)
    {
        sFieldFreeFrames++;
    }
    CoopDeal_FieldFrame();
    CoopTalk_FieldFrame();
    CoopFollow_FieldFrame();
    // The partner's sprite, then its palette and marker, in the same frame:
    // a new map, a bike, Surf or an outfit change never shows one part a
    // frame before the others (the session task skips its own call then).
    if (sLinkState == COOP_LINK_UP && !gMain.inBattle && gMain.callback2 == CB2_Overworld)
        Coop_UpdateSprite();
    CoopOutfit_FieldFrame();
    Coop_CheckAppearance();
    CoopNpc_FieldFrame();
    Coop_PollCoordJoin();
    Coop_PollSharedItems();
    // Shared legendary rewards (src/coop_legend.c): resends, a waiting
    // delivery, the partner's confirmation. Only for an idle player; a
    // delivery never interrupts a script, a menu or a battle.
    CoopLegend_FieldFrame();
    if (sPendingMessage == NULL && sFieldFreeFrames >= 30 && !sInBattle && !gMain.inBattle && sWaitKind == 0
     && !sWaitingAck && !sGuestPairPending && !sJoinLaunchPending)
    {
        bool8 noRoom;

        if (CoopLegend_DeliveryDue(&noRoom))
            sPendingMessage = Coop_EventScript_LegendDeliver;
        else if (noRoom)
            sPendingMessage = Coop_EventScript_LegendNoRoom;
        else if (CoopLegend_NoteDue())
            sPendingMessage = Coop_EventScript_LegendAckNote;
    }
    if (sLinkState != COOP_LINK_IDLE || sPendingMessage != NULL)
        Coop_EnsureTask();
}

u16 Coop_BeginLink(void)
{
    if (sLinkState == COOP_LINK_UP)
        return TRUE;
    // A Pokemon Center's wireless Union Room scan (run while unlinked)
    // leaves the RFU serial and timer handlers installed and the comm type
    // wireless, even after leaving the Center. The cable handshake then
    // never gets a serial interrupt and LINK UP waits at "Cable players: 0".
    UnionRoom_StopCenterScan();
    RestoreSerialTimer3IntrHandlers();
    SetWirelessCommType0();
    gLinkType = LINKTYPE_COOP;
    SetSuppressLinkErrorMessage(TRUE);
    OpenLink();
    SetSuppressLinkErrorMessage(TRUE);
    ResetLinkPlayers();
    ResetLinkPlayerCount();
    sLinkState = COOP_LINK_CONNECTING;
    sLinkPhase = 0;
    sLinkPhaseTimer = 0;
    sLinkWait = FALSE;
    sLinkPlayersSeen = 0;
    sLinkStatusCount = 0xFF;
    sHandshakeSaved = FALSE;
    sMerged = FALSE;
    sPartnerValid = FALSE;
    sPartnerPingKnown = FALSE;
    sLastSeenValid = FALSE;
    CoopTalk_Reset();
    memset(&sBeacon, 0, sizeof(sBeacon));
    sBeaconQueued = 0;
    sBeaconPrompted = 0;
    sStorySeqKnown = FALSE;
    sStorySentValid = FALSE;
    sStoryPartner = 0;
    sReplyKind = 0;
    sSceneAnnounce = COOP_SCENE_NONE;
    sSceneJoin = COOP_SCENE_NONE;
    Coop_EnsureTask();
    return TRUE;
}

// 1 already linked, 2 a reason is waiting to be shown, 0 failed,
// 0xFF still waiting. The menu script waitstates on 0xFF.
u16 Coop_ArmLinkWait(void)
{
    sLinkWaitTimer = 0;
    if (sPendingMessage != NULL)
        return 2;
    if (sLinkState == COOP_LINK_UP && sPartnerValid)
    {
        StringCopy(gStringVar1, gLinkPlayers[GetMultiplayerId() ^ 1].name);
        return 1;
    }
    if (sLinkState != COOP_LINK_CONNECTING)
        return 0;
    sLinkWait = TRUE;
    return 0xFF;
}

u16 Coop_Disconnect(void)
{
    if (Coop_SessionActive() && sPartnerValid)
    {
        // Repeat over position frames before releasing the serial port.
        // The other player can distinguish this from a temporary outage.
        sRequest = COOP_REQ_UNLINK;
        sUnlinkTimer = 30;
        return TRUE;
    }
    Coop_Drop(Coop_EventScript_Disconnected);
    sPendingMessage = NULL;
    return TRUE;
}

u16 Coop_ToggleLevelCap(void)
{
    struct CoopSave *save = CoopSave();

    if (save == NULL || Coop_SessionActive())
        return FALSE;
    save->levelCapOn ^= 1;
    return save->levelCapOn;
}

// gStringVar4: the line under the CO-OP list when not linked.
// gStringVar1: the name of the partner who just chose UNLINK.
u16 Coop_BufferUnlinkName(void)
{
    if (sUnlinkName[0] == EOS)
        StringCopy(gStringVar1, COMPOUND_STRING("Your partner"));
    else
        StringCopy(gStringVar1, sUnlinkName);
    return TRUE;
}

u16 Coop_BufferUnlinkedMenu(void)
{
    u8 *end = StringCopy(gStringVar4, COMPOUND_STRING("Not linked. Level cap: "));

    Coop_BufferCapText();
    end = StringCopy(end, gStringVar1);
    StringCopy(end, COMPOUND_STRING("."));
    return TRUE;
}

u16 Coop_BufferCapText(void)
{
    if (Coop_IsLevelCapOn())
    {
        ConvertIntToDecimalStringN(gStringVar1, Coop_DisplayedLevelCap(), STR_CONV_MODE_LEFT_ALIGN, 3);
        StringCopy(gStringVar2, gStringVar1);
    }
    else
    {
        StringCopy(gStringVar1, COMPOUND_STRING("Off"));
    }
    return TRUE;
}

// Talking to the partner sprite is only a greeting. Not while a question,
// an invitation, or a battle start is pending on either side.
u16 Coop_PartnerChatOk(void)
{
    if (Coop_InteractionBusy() || sJoinLaunchPending || gCoopBattleLaunching)
        return FALSE;
    // An A press that lands as a scene or message closes (mashing through
    // text) does not open the partner menu; a deliberate press does.
    if (sFreeBeforeScript < COOP_PARTNER_TALK_SETTLE)
        return FALSE;
    if (sPartnerValid && sPartner.activity == COOP_ACT_BUSY)
        return FALSE;
    return TRUE;
}

u16 Coop_BufferPartnerLocation(void)
{
    const struct MapHeader *header;

    if (!sPartnerValid)
    {
        StringCopy(gStringVar1, COMPOUND_STRING("somewhere else"));
        return FALSE;
    }
    header = Overworld_GetMapHeaderByGroupAndId(sPartner.mapGroup, sPartner.mapNum);
    if (header == NULL)
    {
        StringCopy(gStringVar1, COMPOUND_STRING("an unknown place"));
        return TRUE;
    }
    GetMapName(gStringVar1, header->regionMapSectionId, 0);
    if (sLegendConflict < 3)
        StringCopy(gStringVar2, GetSpeciesName(sLegendSpecies[sLegendConflict]));
    return TRUE;
}

// STR_VAR_1 where the partner is, STR_VAR_2 the Beacon, STR_VAR_3 the rival.
u16 Coop_BufferMenuStatus(void)
{
    u8 rival = Coop_RivalId();

    if (Coop_PartnerOnSameMap())
        StringCopy(gStringVar1, COMPOUND_STRING("right here"));
    else
        Coop_BufferPartnerLocation();
    if (!CheckBagHasItem(ITEM_TWIN_BEACON, 1))
        StringCopy(gStringVar2, COMPOUND_STRING("not yet"));
    else
        StringCopy(gStringVar2, sBeacon.on ? COMPOUND_STRING("on") : COMPOUND_STRING("off"));
    if (rival == COOP_RIVAL_SOREN)
        StringCopy(gStringVar3, COMPOUND_STRING("SOREN"));
    else if (rival == COOP_RIVAL_MIRA)
        StringCopy(gStringVar3, COMPOUND_STRING("MIRA"));
    else
        StringCopy(gStringVar3, COMPOUND_STRING("?"));
    return TRUE;
}

// The Twin Beacon from the CO-OP menu, the same request as using the item.
// FALSE when this save has no Beacon yet. The answer text is in gStringVar4.
u16 Coop_MenuUseBeacon(void)
{
    if (!CheckBagHasItem(ITEM_TWIN_BEACON, 1))
        return FALSE;
    StringExpandPlaceholders(gStringVar4, Coop_UseTwinBeacon());
    return TRUE;
}

static void Task_CoopReopenStartMenu(u8 taskId)
{
    if (ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
        return;
    DestroyTask(taskId);
    ShowStartMenu();
}

// BACK and B in the CO-OP menu go back to the start menu, as the other
// start menu entries do.
u16 Coop_ReopenStartMenu(void)
{
    CreateTask(Task_CoopReopenStartMenu, 80);
    return 0;
}

u16 Coop_BufferBeaconStatus(void)
{
    StringCopy(gStringVar2, sBeacon.on ? COMPOUND_STRING("On") : COMPOUND_STRING("Off"));
    return sBeacon.on ? TRUE : FALSE;
}

u16 Coop_BufferBeaconAsk(void)
{
    StringCopy(gStringVar1, sBeacon.want ? COMPOUND_STRING("on") : COMPOUND_STRING("off"));
    return TRUE;
}

bool8 Coop_BeaconActive(void)
{
    return sBeacon.on ? TRUE : FALSE;
}

static u32 Coop_ClockNow(bool8 *ok)
{
    struct SiiRtcInfo rtc;

    *ok = FALSE;
    if (RtcGetErrorStatus() & RTC_ERR_FLAG_MASK)
        return 0;
    RtcGetInfo(&rtc);
    *ok = TRUE;
    return RtcGetDayCount(&rtc) * 86400u + ConvertBcdToBinary(rtc.hour) * 3600u
         + ConvertBcdToBinary(rtc.minute) * 60u + ConvertBcdToBinary(rtc.second);
}

static void Coop_ClockStart(struct CoopWaitClock *clock)
{
    bool8 ok;

    clock->startSecs = Coop_ClockNow(&ok);
    clock->rtc = ok ? 0 : 2;
    clock->frames = 0;
    clock->secs = 0;
}

// Once a frame: the seconds this wait has lasted.
static u8 Coop_ClockTick(struct CoopWaitClock *clock)
{
    u16 frameSecs;

    if (clock->frames < 0xFFFF)
        clock->frames++;
    if (clock->rtc != 2 && (clock->frames % 15) == 0)
    {
        bool8 ok;
        u32 now = Coop_ClockNow(&ok);

        if (!ok || now < clock->startSecs)
            clock->rtc = 2;
        else
        {
            u32 el = now - clock->startSecs;

            clock->secs = el > 250 ? 250 : el;
            if (el > 0)
                clock->rtc = 1;
            else if (clock->frames >= 60 * 60)
                clock->rtc = 2;   // a minute of frames and the clock never moved
        }
    }
    frameSecs = clock->frames / 60;
    if (clock->rtc == 2)
        return frameSecs > 250 ? 250 : frameSecs;
    return max(clock->secs, frameSecs / 10);
}

static bool8 Coop_IsMandatoryKind(u8 kind);

u8 Coop_LocalAvailability(u8 purpose)
{
    if (gMain.inBattle || sInBattle || gCoopBattleLaunching)
        return COOP_AVAIL_BATTLE;
    if (gMain.callback2 != CB2_Overworld)
        return COOP_AVAIL_SCREEN;
    if (sGuestPairPending || sJoinLaunchPending)
        return COOP_AVAIL_PROMPT;
    // Waiting on its own shared story fight (its script is stopped there):
    // the partner's invitation for that fight is the same fight.
    if (purpose == COOP_PURPOSE_MANDATORY && sWaitingAck && Coop_IsMandatoryKind(sBattleKind))
        return COOP_AVAIL_OK;
    if (!IsFieldMessageBoxHidden() || sPendingMessage != NULL)
        return COOP_AVAIL_MESSAGE;
    if (CoopDeal_Busy() || sProgressPending || sWaitKind != 0 || sWaitingAck)
        return COOP_AVAIL_TRANSACTION;
    if (ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
        return COOP_AVAIL_SCENE;
    return COOP_AVAIL_OK;
}

static bool8 Coop_InteractionBusy(void)
{
    return sProgressPending || sInBattle || gMain.inBattle || CoopDeal_Busy() || sWaitingAck || sWaitKind != 0 || sGuestPairPending;
}

static void Coop_SyncBeaconRequest(void)
{
    if (sBeacon.phase == COOP_BEACON_IDLE)
    {
        if (sRequest == COOP_REQ_BEACON || sRequest == COOP_REQ_BEACON_YES || sRequest == COOP_REQ_BEACON_NO)
            sRequest = COOP_REQ_NONE;
        return;
    }
    sRequest = sBeacon.request;
    sSlot = sBeacon.slot;
}

static void Coop_PollBeacon(void)
{
    u8 event;

    if (sBeaconQueued != 0 && sBeacon.phase == COOP_BEACON_IDLE && !Coop_InteractionBusy())
    {
        CoopProtocol_BeaconOffer(&sBeacon, (u8)(sBeaconQueued - 1));
        sBeaconQueued = 0;
        Coop_SyncBeaconRequest();
    }
    if (!sPartnerValid || !Coop_SessionActive())
        return;
    CoopProtocol_BeaconStep(&sBeacon, sPartner.request, sPartner.slot, Coop_InteractionBusy());
    event = sBeacon.event;
    Coop_SyncBeaconRequest();
    if (sPartner.request != COOP_REQ_BEACON)
        sBeaconPrompted = 0;
    if (event == COOP_BEACON_EVT_PROMPT && !sBeaconPrompted && sPendingMessage == NULL)
    {
        sBeaconPrompted = 1;
        sPendingMessage = Coop_EventScript_BeaconAsk;
    }
    else if (event == COOP_BEACON_EVT_APPLIED && sPendingMessage == NULL)
    {
        sPendingMessage = sBeacon.on ? Coop_EventScript_BeaconOn : Coop_EventScript_BeaconOff;
    }
    else if (event == COOP_BEACON_EVT_DECLINED && sPendingMessage == NULL)
    {
        sPendingMessage = Coop_EventScript_BeaconDeclined;
    }
}

const u8 *Coop_UseTwinBeacon(void)
{
    u8 want;

    if (!Coop_SessionActive())
        return sText_BeaconNeedLink;
    if (sBeacon.phase != COOP_BEACON_IDLE)
        return sText_BeaconWaiting;
    want = sBeacon.on ? 0 : 1;
    if (Coop_InteractionBusy())
    {
        sBeaconQueued = (u8)(want + 1);
        return sText_BeaconQueued;
    }
    CoopProtocol_BeaconOffer(&sBeacon, want);
    Coop_SyncBeaconRequest();
    return sText_BeaconSent;
}

u16 Coop_PrepareTwinBeacon(void)
{
    if (FlagGet(FLAG_RECEIVED_TWIN_BEACON))
        return 1;
    if (CheckBagHasSpace(ITEM_TWIN_BEACON, 1))
        return 2;
    if (sBeaconBagWarned)
        return 0;
    sBeaconBagWarned = 1;
    return 3;
}

u16 Coop_BeaconAnswer(void)
{
    CoopProtocol_BeaconAnswer(&sBeacon, VarGet(VAR_0x8004) != 0);
    Coop_SyncBeaconRequest();
    return TRUE;
}

static u8 Coop_RivalStageFromId(u16 trainerId)
{
    switch (trainerId)
    {
    case TRAINER_BRENDAN_ROUTE_103_MUDKIP:
    case TRAINER_BRENDAN_ROUTE_103_TREECKO:
    case TRAINER_BRENDAN_ROUTE_103_TORCHIC:
    case TRAINER_MAY_ROUTE_103_MUDKIP:
    case TRAINER_MAY_ROUTE_103_TREECKO:
    case TRAINER_MAY_ROUTE_103_TORCHIC:
        return 0;
    case TRAINER_BRENDAN_RUSTBORO_TREECKO:
    case TRAINER_BRENDAN_RUSTBORO_MUDKIP:
    case TRAINER_BRENDAN_RUSTBORO_TORCHIC:
    case TRAINER_MAY_RUSTBORO_MUDKIP:
    case TRAINER_MAY_RUSTBORO_TREECKO:
    case TRAINER_MAY_RUSTBORO_TORCHIC:
        return 1;
    case TRAINER_BRENDAN_ROUTE_110_MUDKIP:
    case TRAINER_BRENDAN_ROUTE_110_TREECKO:
    case TRAINER_BRENDAN_ROUTE_110_TORCHIC:
    case TRAINER_MAY_ROUTE_110_MUDKIP:
    case TRAINER_MAY_ROUTE_110_TREECKO:
    case TRAINER_MAY_ROUTE_110_TORCHIC:
        return 2;
    case TRAINER_BRENDAN_ROUTE_119_MUDKIP:
    case TRAINER_BRENDAN_ROUTE_119_TREECKO:
    case TRAINER_BRENDAN_ROUTE_119_TORCHIC:
    case TRAINER_MAY_ROUTE_119_MUDKIP:
    case TRAINER_MAY_ROUTE_119_TREECKO:
    case TRAINER_MAY_ROUTE_119_TORCHIC:
        return 3;
    case TRAINER_BRENDAN_LILYCOVE_MUDKIP:
    case TRAINER_BRENDAN_LILYCOVE_TREECKO:
    case TRAINER_BRENDAN_LILYCOVE_TORCHIC:
    case TRAINER_MAY_LILYCOVE_MUDKIP:
    case TRAINER_MAY_LILYCOVE_TREECKO:
    case TRAINER_MAY_LILYCOVE_TORCHIC:
        return 4;
    default:
        return 0xFF;
    }
}

// The optional quests' trainers (rival spars and exhibition, SLAG, the Twin
// Trial): never "meant for both", whatever their class, so each player can
// fight them alone while linked. Their duos are offered by the scripts.
static bool8 Coop_IsOptionalQuestTrainer(u16 trainerId)
{
    switch (trainerId)
    {
    case TRAINER_TWIN_SPAR_1:
    case TRAINER_TWIN_SPAR_2:
    case TRAINER_TWIN_EXH_SOREN:
    case TRAINER_TWIN_EXH_MIRA:
    case TRAINER_CC_CHIEF:
    case TRAINER_CC_GRUNT_SKIFF:
    case TRAINER_CC_GRUNT_BARREL:
    case TRAINER_DC_RAIDER_DUNE:
    case TRAINER_DC_RAIDER_MESA:
    case TRAINER_BT_ROUND1_SOL:
    case TRAINER_BT_ROUND1_LUNA:
    case TRAINER_BT_ROUND2_EMBER:
    case TRAINER_BT_ROUND2_FROST:
    case TRAINER_BT_FINAL_LUMEN:
    case TRAINER_BT_FINAL_UMBRA:
    case TRAINER_BT_REPEAT_LUMEN:
    case TRAINER_BT_REPEAT_UMBRA:
        return TRUE;
    }
    return FALSE;
}

static bool8 Coop_ClassRequiresPartner(u16 trainerId)
{
    enum TrainerClassID cls;

    if (trainerId == TRAINER_NONE || trainerId >= TRAINERS_COUNT || IsSpecialTrainer(trainerId))
        return FALSE;
    if (Coop_IsOptionalQuestTrainer(trainerId))
        return FALSE;
    cls = GetTrainerClassFromId(trainerId);
    return cls == TRAINER_CLASS_LEADER
        || cls == TRAINER_CLASS_ELITE_FOUR
        || cls == TRAINER_CLASS_CHAMPION
        || cls == TRAINER_CLASS_RIVAL
        || cls == TRAINER_CLASS_AQUA_LEADER
        || cls == TRAINER_CLASS_MAGMA_LEADER
        || cls == TRAINER_CLASS_AQUA_ADMIN
        || cls == TRAINER_CLASS_MAGMA_ADMIN;
}

bool8 Coop_ShouldRefuseSolo(u16 trainerId)
{
    // Not linked: every fight has a one-player path. Linked, a fight meant
    // for both waits for the partner (UNLINK first to face it alone).
    if (!Coop_SessionActive())
        return FALSE;
    if (Coop_PartnerOnSameMap())
        return FALSE;
    return Coop_ClassRequiresPartner(trainerId) || Coop_RivalStageFromId(trainerId) != 0xFF;
}

// A fight meant for both (leader, Elite Four, champion, Wally, villain
// bosses and admins) met while linked and together with the Twin Beacon
// OFF: say so and start nothing (turn it on, or UNLINK to face it alone).
bool8 Coop_BlockMandatoryBeaconOff(u16 trainerId)
{
    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap() || sBeacon.on)
        return FALSE;
    if (!Coop_ClassRequiresPartner(trainerId) || Coop_PlaceStaysSolo())
        return FALSE;
    sRefusal = COOP_REFUSE_BEACON_OFF;
    ScriptContext_SetupScript(Coop_EventScript_NeedPartner);
    return TRUE;
}

bool8 Coop_BlockUnreadyMandatory(u16 trainerId)
{
    if (!Coop_PartnerOnSameMap())
        return FALSE;
    if (!Coop_ClassRequiresPartner(trainerId))
        return FALSE;
    if (Coop_HasBattleReadyMon())
        return FALSE;
    ScriptContext_SetupScript(Coop_EventScript_NotReady);
    return TRUE;
}

static bool8 Coop_PlaceStaysSolo(void)
{
    u16 section = gMapHeader.regionMapSectionId;

    if (section == MAPSEC_BATTLE_FRONTIER || section == MAPSEC_SAFARI_ZONE || section == MAPSEC_TRAINER_HILL)
        return TRUE;
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        return TRUE;
    return FALSE;
}

static bool8 Coop_CoupleStaysSolo(u16 trainerId)
{
    enum TrainerClassID cls = GetTrainerClassFromId(trainerId);

    if (GetTrainerBattleType(trainerId) == TRAINER_BATTLE_TYPE_DOUBLES)
        return TRUE;
    switch (cls)
    {
    case TRAINER_CLASS_TWINS:
    case TRAINER_CLASS_SR_AND_JR:
    case TRAINER_CLASS_YOUNG_COUPLE:
    case TRAINER_CLASS_OLD_COUPLE:
    case TRAINER_CLASS_SIS_AND_BRO:
        return TRUE;
    default:
        return FALSE;
    }
}

static u8 Coop_StoryGate(u16 trainerId)
{
    switch (trainerId)
    {
    case TRAINER_MAXIE_MT_CHIMNEY:
        return COOP_GATE_CHIMNEY;
    case TRAINER_MAXIE_MAGMA_HIDEOUT:
        return COOP_GATE_MAGMA;
    case TRAINER_ARCHIE:
        return COOP_GATE_SEAFLOOR;
    case TRAINER_MAXIE_MOSSDEEP:
        return COOP_GATE_SPACE;
    case TRAINER_SHELLY_WEATHER_INSTITUTE:
        return COOP_GATE_WEATHER;
    case TRAINER_MATT:
        return COOP_GATE_AQUA;
    default:
        return 0;
    }
}

static void Coop_QueueRival(u8 stage)
{
    u8 rivalType = VarGet(VAR_STARTER_MON);
    u16 soren, mira;

    if (rivalType > 2)
        rivalType = 0;
    soren = COOP_RIVAL_BASE + stage * 6 + rivalType;
    mira = COOP_RIVAL_BASE + stage * 6 + 3 + rivalType;
    sRivalFightStage = stage;
    sResumeScript = FALSE;
    Coop_QueueInvite(COOP_KIND_RIVAL, soren, mira, stage, 0);
}

bool8 Coop_TryQueueOrdinaryDouble(void)
{
    u16 trainerA = TRAINER_BATTLE_PARAM.opponentA;
    u16 trainerB;
    int category;

    // A script that already offered the partner a duo and heard NO.
    if (gCoopForceSolo)
    {
        gCoopForceSolo = FALSE;
        sSoloNote = COOP_SOLO_NONE;
        return FALSE;
    }

    // Only ordinary trainers get a note; story fights have their own rules.
    sSoloNote = COOP_SOLO_NONE;
    if (trainerA != TRAINER_NONE && trainerA < TRAINERS_COUNT && !IsSpecialTrainer(trainerA)
     && Coop_RivalStageFromId(trainerA) == 0xFF && !Coop_ClassRequiresPartner(trainerA))
    {
        sSoloNote = Coop_SoloReasonHere();
        if (sSoloNote == COOP_SOLO_NONE && sBeacon.on && Coop_SessionActive() && Coop_CoupleStaysSolo(trainerA))
            sSoloNote = COOP_SOLO_KIND;
        // The partner is in a fight of their own (or the link is mending):
        // an invite would sit out its whole timeout and then read "didn't
        // answer". Fight alone at once with the real reason.
        if (sSoloNote == COOP_SOLO_BUSY || sSoloNote == COOP_SOLO_RECOVERING)
            return FALSE;
    }
    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap())
        return FALSE;
    if (trainerA == TRAINER_NONE || trainerA >= TRAINERS_COUNT || IsSpecialTrainer(trainerA))
        return FALSE;
    if (Coop_RivalStageFromId(trainerA) != 0xFF)
        return FALSE;
    // The Route 102 pair has its own ask, and stays solo when that is refused.
    if (trainerA == COOP_PAIR_LEFT_ID || trainerA == COOP_PAIR_RIGHT_ID)
        return FALSE;
    category = CoopProtocol_TrainerCategory(Coop_PlaceStaysSolo(), Coop_ClassRequiresPartner(trainerA), Coop_CoupleStaysSolo(trainerA));
    if (!CoopProtocol_QueuesCoop(category, sBeacon.on, TRUE))
        return FALSE;
    // Optional fights stay solo when the front three are down. Mandatory ones
    // already stopped in Coop_BlockUnreadyMandatory when nothing can fight
    // (their team is chosen first, src/coop_prep.c).
    if (category == COOP_CAT_MANDATORY ? !Coop_HasBattleReadyMon() : !Coop_HasBattleReadyLead())
        return FALSE;
    // The partner, spotted by this same trainer, already asked: join it.
    if (sSameTrainerPending)
    {
        sSameTrainerPending = FALSE;
        if (category == COOP_CAT_OPTIONAL && sSameTrainerInvite.trainerA == trainerA)
        {
            sSoloNote = COOP_SOLO_NONE;
            Coop_JoinSameTrainer(&sSameTrainerInvite);
            return TRUE;
        }
    }
    trainerB = CoopProtocol_DistinctPartner(trainerA, TRAINER_BATTLE_PARAM.opponentB);
    sResumeScript = FALSE;
    sLegendKind = 0;
    if (category == COOP_CAT_MANDATORY)
        Coop_QueueInvite(COOP_KIND_STORY, trainerA, trainerB, Coop_StoryGate(trainerA), 0);
    else
        Coop_QueueInvite(COOP_KIND_TRAINER, trainerA, trainerB, 0, 0);
    return TRUE;
}

// Scripts of fights meant for both players: TRUE when the fight can start
// here, together (partner on this map) or alone (not linked). FALSE while
// linked with the partner elsewhere: Coop_EventScript_NeedPartner explains.
u16 Coop_MandatoryGate(void)
{
    return Coop_PartnerOnSameMap() || !Coop_SessionActive();
}

// The solo versions of the rival fights: the stock rival trainer slots, one
// team per stage for each twin, picked by this save's starter (the case).
static const u16 sSoloRivalTrainers[5][2][3] = {
    { { TRAINER_BRENDAN_ROUTE_103_TREECKO, TRAINER_BRENDAN_ROUTE_103_TORCHIC, TRAINER_BRENDAN_ROUTE_103_MUDKIP },
      { TRAINER_MAY_ROUTE_103_TREECKO, TRAINER_MAY_ROUTE_103_TORCHIC, TRAINER_MAY_ROUTE_103_MUDKIP } },
    { { TRAINER_BRENDAN_RUSTBORO_TREECKO, TRAINER_BRENDAN_RUSTBORO_TORCHIC, TRAINER_BRENDAN_RUSTBORO_MUDKIP },
      { TRAINER_MAY_RUSTBORO_TREECKO, TRAINER_MAY_RUSTBORO_TORCHIC, TRAINER_MAY_RUSTBORO_MUDKIP } },
    { { TRAINER_BRENDAN_ROUTE_110_TREECKO, TRAINER_BRENDAN_ROUTE_110_TORCHIC, TRAINER_BRENDAN_ROUTE_110_MUDKIP },
      { TRAINER_MAY_ROUTE_110_TREECKO, TRAINER_MAY_ROUTE_110_TORCHIC, TRAINER_MAY_ROUTE_110_MUDKIP } },
    { { TRAINER_BRENDAN_ROUTE_119_TREECKO, TRAINER_BRENDAN_ROUTE_119_TORCHIC, TRAINER_BRENDAN_ROUTE_119_MUDKIP },
      { TRAINER_MAY_ROUTE_119_TREECKO, TRAINER_MAY_ROUTE_119_TORCHIC, TRAINER_MAY_ROUTE_119_MUDKIP } },
    { { TRAINER_BRENDAN_LILYCOVE_TREECKO, TRAINER_BRENDAN_LILYCOVE_TORCHIC, TRAINER_BRENDAN_LILYCOVE_MUDKIP },
      { TRAINER_MAY_LILYCOVE_TREECKO, TRAINER_MAY_LILYCOVE_TORCHIC, TRAINER_MAY_LILYCOVE_MUDKIP } },
};

static EWRAM_DATA u8 sSoloRivalStage = 0; // stage + 1, 0 = none

u16 Coop_SoloRivalTrainer(u8 stage)
{
    u8 starter = VarGet(VAR_STARTER_MON);
    u8 twin = Coop_RivalId() == COOP_RIVAL_MIRA ? 1 : 0;

    if (stage > 4)
        return TRAINER_NONE;
    if (starter > 2)
        starter = 0;
    return sSoloRivalTrainers[stage][twin][starter];
}

// CB2_EndTrainerBattle, after any won solo trainer fight.
void Coop_NoteSoloTrainerWin(void)
{
    struct CoopSave *save = CoopSave();

    if (sSoloRivalStage != 0 && save != NULL && save->rivalStage < sSoloRivalStage)
        save->rivalStage = sSoloRivalStage;
    sSoloRivalStage = 0;
}

static bool8 Coop_RefuseMandatory(void)
{
    if (!Coop_PartnerOnSameMap())
        sRefusal = COOP_REFUSE_NONE;
    else if (!Coop_HasBattleReadyMon())
        sRefusal = COOP_REFUSE_SELF_NOT_READY;
    // Linked and together, but the Beacon is OFF: a shared fight is asked
    // for with the Beacon on (the opening rescue is the one exception).
    else if (!sBeacon.on)
        sRefusal = COOP_REFUSE_BEACON_OFF;
    else
        return FALSE;
    gSpecialVar_Result = FALSE;
    CreateTask(Task_CoopEnableScript, 0);
    return TRUE;
}

u16 Coop_StartGymBattle(void)
{
    if (!Coop_SessionActive())
    {
        // Gym, Elite Four, champion, alone: both trainers, whole party. The
        // player may first choose the lead (src/coop_prep.c), which then
        // starts the battle, or step back.
        sSoloRivalStage = 0;
        gSpecialVar_Result = TRUE;
        if (CoopPrep_BeginSolo(VarGet(VAR_0x8004), VarGet(VAR_0x8005)))
            return TRUE;
        BattleSetup_StartSoloPairBattle(VarGet(VAR_0x8004), VarGet(VAR_0x8005));
        return TRUE;
    }
    if (Coop_RefuseMandatory())
        return FALSE;
    sResumeScript = TRUE;
    sLegendKind = 0;
    Coop_QueueInvite((u8)VarGet(VAR_0x8008), VarGet(VAR_0x8004), VarGet(VAR_0x8005), (u8)VarGet(VAR_0x8006), VarGet(VAR_0x8007));
    gSpecialVar_Result = FALSE;
    return TRUE;
}

// Special (waitstate), after CoopDuo_PrepareInvite: ask the partner on this
// map for the duo battle. No Beacon needed: the desk itself is the consent.
// VAR_RESULT TRUE once won; FALSE when lost or nothing started (the refusal:
// Coop_TakeRefusal; the outcome: CoopDuo_LastOutcome).
u16 Coop_StartDuoBattle(void)
{
    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap())
    {
        sRefusal = COOP_REFUSE_NONE;
        gSpecialVar_Result = FALSE;
        CreateTask(Task_CoopEnableScript, 0);
        return FALSE;
    }
    sResumeScript = TRUE;
    sLegendKind = 0;
    Coop_QueueInvite(COOP_KIND_DUO, CoopDuo_InviteRunId(), CoopDuo_InviteModeWord(), CoopDuo_InviteFlags(), CoopDuo_InviteBattle());
    // The invite's trainer fields carry the run, not trainers: while it
    // waits, this game's battle parameters name the duo placeholders.
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_DUO_A;
    TRAINER_BATTLE_PARAM.opponentB = TRAINER_DUO_B;
    sBattleBadge = 0;
    sBattleFlag = 0;
    gSpecialVar_Result = FALSE;
    return TRUE;
}

// The duo invite a team preparation belongs to (host: this game's own).
bool8 Coop_PrepDuoFields(bool8 host, u16 *runId, u16 *modeWord, u16 *battle, u8 *flags)
{
    const struct CoopInvite *inv = host ? &sInvite : &sGuestPairInvite;

    if (inv->kind != COOP_KIND_DUO)
        return FALSE;
    *runId = inv->trainerA;
    *modeWord = inv->trainerB;
    *battle = inv->aux;
    *flags = inv->badge;
    return TRUE;
}

u16 Coop_StartStoryBattle(void)
{
    if (Coop_RefuseMandatory())
        return FALSE;
    sResumeScript = TRUE;
    sLegendKind = 0;
    Coop_QueueInvite(COOP_KIND_STORY, VarGet(VAR_0x8004), VarGet(VAR_0x8005), (u8)VarGet(VAR_0x8006), VarGet(VAR_0x8007));
    gSpecialVar_Result = FALSE;
    return TRUE;
}

// Special (waitstate; VAR_0x8004 = COOP_ENC_*): ask the partner on this map
// to share the legendary encounter (src/coop_legend.c). The script has
// already offered it and this player said yes. VAR_RESULT: the battle
// outcome once fought, or 0 when it did not start (the reason is the solo
// note: CoopLegend_BufferDeclined), and the script offers it alone.
u16 Coop_StartLegendaryBattle(void)
{
    u8 enc = VarGet(VAR_0x8004);

    if (CoopLegend_Species(enc) == SPECIES_NONE || !Coop_SessionActive() || !Coop_PartnerOnSameMap()
     || !Coop_HasBattleReadyLead())
    {
        sSoloNote = !Coop_HasBattleReadyLead() ? COOP_SOLO_SELF_NOT_READY : COOP_SOLO_ELSEWHERE;
        gSpecialVar_Result = FALSE;
        CreateTask(Task_CoopEnableScript, 0);
        return FALSE;
    }
    sResumeScript = TRUE;
    sLegendKind = enc;
    Coop_QueueInvite(COOP_KIND_LEGEND, 0, 0, 0, enc);
    gSpecialVar_Result = FALSE;
    return TRUE;
}

u8 Coop_PeekSoloNote(void)
{
    return sSoloNote;
}

void Coop_ClearSoloNote(void)
{
    sSoloNote = COOP_SOLO_NONE;
}

u16 Coop_ProposeCase(void)
{
    struct CoopSave *save = CoopSave();
    u16 caseId = VarGet(VAR_0x8004);

    if (!Coop_PartnerOnSameMap() || caseId > 2)
    {
        gSpecialVar_Result = COOP_CASE_NONE;
        CreateTask(Task_CoopEnableScript, 0);
        return COOP_CASE_NONE;
    }
    if (save != NULL)
        save->caseId = (u8)caseId;
    sProposedCase = (u8)caseId;
    sRequest = COOP_REQ_CASE;
    sSlot = (u8)caseId;
    sWaitKind = COOP_REQ_CASE;
    gSpecialVar_Result = COOP_CASE_NONE;
    return TRUE;
}

// The duo starters of each case (Battle Guide v2), case player first.
// Case order is grass, fire, water (VAR_STARTER_MON). Saves made before
// 0xC017013D keep the Pokemon they were given (then Shroomish, Houndour,
// Wingull); only new rescues use this table.
const u16 gCoopCaseStarters[3][2] = {
    { SPECIES_TREECKO, SPECIES_CHIKORITA },
    { SPECIES_TORCHIC, SPECIES_FENNEKIN },
    { SPECIES_MUDKIP, SPECIES_TOTODILE },
};

// The player who opened the bag gets the Pokémon they highlighted.
// The partner gets the one named on that ball.
static bool8 Coop_PartnerInBirchLab(void)
{
    return sPartnerValid
        && sPartner.mapGroup == MAP_GROUP(MAP_LITTLEROOT_TOWN_PROFESSOR_BIRCHS_LAB)
        && sPartner.mapNum == MAP_NUM(MAP_LITTLEROOT_TOWN_PROFESSOR_BIRCHS_LAB);
}

static void Coop_GiveRescueStarter(u8 caseId, bool8 opener)
{
    struct CoopSave *save;
    u8 index;

    if (caseId > 2)
        caseId = 0;
    if (GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES) != SPECIES_NONE)
        return;
    save = CoopSave();
    if (save != NULL)
        save->caseId = caseId;
    VarSet(VAR_STARTER_MON, caseId);
    index = opener ? 0 : 1;
    if (save != NULL)
        save->rivalId = CoopProtocol_RivalForSlot(index);
    ScriptGiveMon(gCoopCaseStarters[caseId][index], 5, ITEM_NONE);
}

u16 Coop_StartRescueBattle(void)
{
    struct CoopSave *save = CoopSave();
    u16 caseId = VarGet(VAR_STARTER_MON);

    sBirchBag = FALSE;
    if (GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES) != SPECIES_NONE
     && save != NULL && save->caseId <= 2)
        caseId = save->caseId;
    if (caseId > 2)
        caseId = 0;
    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap())
    {
        sRescueResult = 0;
        gSpecialVar_Result = FALSE;
        return FALSE;
    }
    Coop_GiveRescueStarter((u8)caseId, TRUE);
    CreateScriptedDoubleWildMon(SPECIES_POOCHYENA, 2, ITEM_NONE, SPECIES_POOCHYENA, 2, ITEM_ORAN_BERRY);
    sRescueDone = FALSE;
    sRescueResult = 1;
    sRescueField = COOP_RESCUE_RETURN_NONE;
    sResumeScript = TRUE;
    sBattleKind = COOP_KIND_RESCUE;
    sRequest = COOP_REQ_RESCUE;
    sSlot = (u8)caseId;
    sWaitKind = COOP_REQ_RESCUE;
    sRescueEcho = 0;
    sInviteTimer = 0;
    gSpecialVar_Result = TRUE;
    return TRUE;
}

u16 Coop_TakeRescueResult(void)
{
    return sRescueResult;
}

u8 Coop_RescueReturn(void)
{
    u8 field = sRescueField;

    sRescueField = COOP_RESCUE_RETURN_NONE;
    return field;
}

u16 Coop_ClaimStarter(void)
{
    if (!Coop_PartnerOnSameMap())
    {
        gSpecialVar_Result = FALSE;
        CreateTask(Task_CoopEnableScript, 0);
        return FALSE;
    }
    sClaim = VarGet(VAR_0x8004) ? 0 : 1;
    sRequest = COOP_REQ_CLAIM;
    sSlot = sClaim;
    sWaitKind = COOP_REQ_CLAIM;
    return TRUE;
}

u16 Coop_BirchBagFree(void)
{
    if (sPartnerValid && sPartner.talkLocalId == 0xFF)
        return FALSE;
    sBirchBag = TRUE;
    return TRUE;
}

u16 Coop_RequestLabWarp(void)
{
    sBirchBag = FALSE;
    if (!Coop_SessionActive())
        return FALSE;
    sResumeScript = FALSE;
    Coop_QueueInvite(COOP_KIND_WARP, 0, 0, 0, 0);
    sWaitingAck = FALSE;
    Coop_SendInvite();
    return TRUE;
}

bool8 Coop_HasBattleReadyLead(void)
{
    struct CoopMonState party[COOP_BATTLE_SLOTS];
    u32 i;

    for (i = 0; i < COOP_BATTLE_SLOTS; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        party[i].species = GetMonData(mon, MON_DATA_SPECIES);
        party[i].hp = GetMonData(mon, MON_DATA_HP);
        party[i].isEgg = GetMonData(mon, MON_DATA_IS_EGG);
    }
    return CoopProtocol_LeadReady(party);
}

// Any Pokemon in the party that could fight (story fights choose their
// three before the battle, src/coop_prep.c).
bool8 Coop_HasBattleReadyMon(void)
{
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG)
         && GetMonData(mon, MON_DATA_HP) != 0)
            return TRUE;
    }
    return FALSE;
}

u16 Coop_TakeRefusal(void)
{
    u16 refusal = sRefusal;

    sRefusal = COOP_REFUSE_NONE;
    return refusal;
}

// Slots 4-6 leave the party for the battle so every engine count, switch
// prompt, and whiteout check sees exactly the three that were sent.
static void Coop_ClearCatchState(void)
{
    sPartnerCaughtCount = 0;
    memset(sPartnerCaught, 0, sizeof(sPartnerCaught));
    CoopProtocol_ClearCatches(&sLedger);
    memset(sCatches, 0, sizeof(sCatches));
    memset(gCoopBallTargetFor, 0, sizeof(gCoopBallTargetFor));
    CoopTurn_Reset();
    sCatchCount = 0;
    sLocalLegendMask = 0;
    sPartnerLegendMask = 0;
    gCoopBallTarget = 0;
    gCoopLastBall = ITEM_NONE;
}

bool8 Coop_BattleGuest(void)
{
    return sIsGuest;
}

void Coop_OnBattleLaunched(void)
{
    u32 i;

    sMoneyAtLaunch = GetMoney(&gSaveBlock1Ptr->money);
    Coop_ClearCatchState();
    memset(sProgressLevels, 0, sizeof(sProgressLevels));
    memset(sProgressMoves, 0, sizeof(sProgressMoves));
    sProgressPending = FALSE;
    sProgressSlot = 0;
    memcpy(sPartySnap, gParties[B_TRAINER_PLAYER], sizeof(sPartySnap));
    for (i = COOP_BATTLE_SLOTS; i < PARTY_SIZE; i++)
        ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
    CalculatePlayerPartyCount();
    sHasSnapshot = TRUE;
    sAborting = FALSE;
    sInBattle = TRUE;
    gCoopBattleLaunching = FALSE;
}

// The species a Mega form came from: the form of the same family whose own
// Mega Evolution leads to it (Meowstic's female line has its own Mega).
static u16 Coop_MegaBaseSpecies(u16 mega)
{
    const u16 *forms = gSpeciesInfo[mega].formSpeciesIdTable;
    u32 i, j;

    for (i = 0; forms != NULL && forms[i] != FORM_SPECIES_END; i++)
    {
        const struct FormChange *changes;

        if (forms[i] == mega || gSpeciesInfo[forms[i]].isMegaEvolution)
            continue;
        changes = GetSpeciesFormChanges(forms[i]);
        for (j = 0; changes != NULL && changes[j].method != FORM_CHANGE_TERMINATOR; j++)
        {
            if ((changes[j].method == FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM || changes[j].method == FORM_CHANGE_BATTLE_MEGA_EVOLUTION_MOVE)
             && changes[j].targetSpecies == mega)
                return forms[i];
        }
    }
    return SPECIES_NONE;
}

// A co-op battle's guest runs no battle engine, so the stock end-of-battle
// revert never reaches its own Pokemon: a Mega that the master's engine
// wrote into this party is turned back here (the master reverts its own).
// HP and everything else stay; Mega and base forms share base HP.
static void Coop_RevertBattleMegas(void)
{
    u32 i;

    for (i = 0; i < COOP_BATTLE_SLOTS; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u16 species = GetMonData(mon, MON_DATA_SPECIES);
        u16 base;
        u16 hp;

        if (species == SPECIES_NONE || species >= NUM_SPECIES || !gSpeciesInfo[species].isMegaEvolution)
            continue;
        base = Coop_MegaBaseSpecies(species);
        if (base == SPECIES_NONE)
            continue;
        hp = GetMonData(mon, MON_DATA_HP);
        SetMonData(mon, MON_DATA_SPECIES, &base);
        CalculateMonStats(mon);
        if (hp > GetMonData(mon, MON_DATA_MAX_HP))
            hp = GetMonData(mon, MON_DATA_MAX_HP);
        SetMonData(mon, MON_DATA_HP, &hp);
        if (gCoopMegaReverts < 0xFF)
            gCoopMegaReverts++;
    }
}

static void Coop_RestoreParty(bool8 aborted)
{
    struct Pokemon arrivals[PARTY_SIZE - COOP_BATTLE_SLOTS];
    u32 i, count = 0;

    // Duo Frontier: copies fought; the real party comes back as it was,
    // whatever happened (the copies' snapshot below must not be used).
    if (CoopDuo_RestoreRealParty())
    {
        if (aborted)
        {
            CoopDuo_AbortBattle();
            memset(sProgressLevels, 0, sizeof(sProgressLevels));
            sProgressPending = FALSE;
        }
        sHasSnapshot = FALSE;
        return;
    }
    if (aborted)
        CoopDuo_AbortBattle();
    if (!sHasSnapshot)
        return;
    if (!aborted)
        Coop_RevertBattleMegas();
    if (aborted)
    {
        memset(sProgressLevels, 0, sizeof(sProgressLevels));
        sProgressPending = FALSE;
    }
    // Anything sitting in 4-6 now joined during the battle, such as a catch.
    if (!aborted)
    {
        for (i = COOP_BATTLE_SLOTS; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) != SPECIES_NONE)
                arrivals[count++] = gParties[B_TRAINER_PLAYER][i];
        }
    }
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (CoopProtocol_RestoreFromSnapshot(i, aborted))
            gParties[B_TRAINER_PLAYER][i] = sPartySnap[i];
    }
    // Everyone back in their own slot (a team chosen from slots 4-6).
    CoopPrep_UndoTeam();
    CompactPartySlots();
    for (i = 0; i < count; i++)
    {
        u32 slot = CalculatePlayerPartyCount();

        if (slot < PARTY_SIZE)
            gParties[B_TRAINER_PLAYER][slot] = arrivals[i];
        else
            CopyMonToPC(&arrivals[i]);
    }
    CalculatePlayerPartyCount();
    sHasSnapshot = FALSE;
}

// The flags a shared win may set on both saves (the invite carries one).
// A flag outside this list, from a damaged or foreign invite, is ignored
// instead of being set on the partner's save.
static bool8 Coop_SharedWinFlag(u16 flag)
{
    static const u16 sFlags[] = {
        FLAG_DEFEATED_RUSTBORO_GYM, FLAG_DEFEATED_DEWFORD_GYM, FLAG_DEFEATED_MAUVILLE_GYM,
        FLAG_DEFEATED_LAVARIDGE_GYM, FLAG_DEFEATED_PETALBURG_GYM, FLAG_DEFEATED_FORTREE_GYM,
        FLAG_DEFEATED_MOSSDEEP_GYM, FLAG_DEFEATED_SOOTOPOLIS_GYM,
        FLAG_DEFEATED_ELITE_4_SIDNEY, FLAG_DEFEATED_ELITE_4_PHOEBE, FLAG_DEFEATED_ELITE_4_GLACIA,
        FLAG_DEFEATED_ELITE_4_DRAKE, FLAG_IS_CHAMPION, FLAG_DEFEATED_MAGMA_SPACE_CENTER,
        FLAG_CARAVAN_RAIDERS_DUO, FLAG_COAST_CREW_DUO, FLAG_TWIN_EXH_WON, FLAG_TOURNEY_WON,
    };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sFlags); i++)
    {
        if (sFlags[i] == flag)
            return TRUE;
    }
    return FALSE;
}

static void Coop_GiveJointWin(void)
{
    struct CoopSave *save = CoopSave();

    // The badge byte is a gym number only for gyms. Rivals store a stage
    // there, and story gates store a scene id. Either one used to set a badge.
    if (sBattleKind == COOP_KIND_GYM && sBattleBadge >= 1 && sBattleBadge <= 8)
    {
        FlagSet(FLAG_BADGE01_GET + sBattleBadge - 1);
        if (save != NULL)
        {
            save->jointBadges |= (u8)(1 << (sBattleBadge - 1));
            save->jointBadgeOtId[sBattleBadge - 1] = Coop_PartnerOtId();
        }
    }
    if (sBattleFlag != 0 && Coop_SharedWinFlag(sBattleFlag))
        FlagSet(sBattleFlag);
    if (sBattleKind == COOP_KIND_TRAINER || sBattleKind == COOP_KIND_STORY)
    {
        // Ordinary shared trainers have no story flag in the invite. The
        // talker's stock script only marks its own save, so mark both here.
        // The same for story fights (admins such as Tabitha have no gate
        // flag): the partner's save used to keep the trainer unbeaten, and
        // the admin stopped the partner for a second fight.
        u16 trainers[2] = { TRAINER_BATTLE_PARAM.opponentA, TRAINER_BATTLE_PARAM.opponentB };
        for (u32 i = 0; i < ARRAY_COUNT(trainers); i++)
            if (trainers[i] != TRAINER_NONE && trainers[i] < TRAINERS_COUNT && !IsSpecialTrainer(trainers[i]))
                SetTrainerFlag(trainers[i]);
    }
    // Wally at the Victory Road entrance: his line (a coord event) checks
    // VAR_VICTORY_ROAD_1F_STATE, which only the stepping player's script
    // sets. The partner's save kept it at 0, so Wally stopped the partner
    // again on the same line (journey 66), and with the other player gone
    // that fight waits for a partner who has moved on. Both saves record
    // the win (the talker's script then sets its own side).
    if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_WALLY_VR_1 && VarGet(VAR_VICTORY_ROAD_1F_STATE) == 0)
    {
        FlagSet(FLAG_DEFEATED_WALLY_VICTORY_ROAD);
        FlagClear(FLAG_HIDE_VICTORY_ROAD_ENTRANCE_WALLY);
        VarSet(VAR_VICTORY_ROAD_1F_STATE, 1);
    }
    if (sBattleKind == COOP_KIND_CHAMPION)
        FlagSet(FLAG_IS_CHAMPION);
    if (sBattleKind == COOP_KIND_RIVAL && save != NULL && sRivalFightStage != 0xFF)
    {
        if (save->rivalStage <= sRivalFightStage)
            save->rivalStage = sRivalFightStage + 1;
        save->jointRivalStage = sRivalFightStage + 1;
        save->jointRivalOtId = Coop_PartnerOtId();
    }
}

static void Coop_QueueGuestFollowup(void)
{
    if (sBattleKind == COOP_KIND_E4)
    {
        // Each room's walk-in leaves its own value; a walk-in cut short by
        // the invite would replay without it, and a lower value replays the
        // next room's walk-in on the wrong door.
        if (sBattleFlag == FLAG_DEFEATED_ELITE_4_SIDNEY)
            Coop_RaiseVar(VAR_ELITE_4_STATE, 1);
        else if (sBattleFlag == FLAG_DEFEATED_ELITE_4_PHOEBE)
            Coop_RaiseVar(VAR_ELITE_4_STATE, 2);
        else if (sBattleFlag == FLAG_DEFEATED_ELITE_4_GLACIA)
            Coop_RaiseVar(VAR_ELITE_4_STATE, 3);
        else if (sBattleFlag == FLAG_DEFEATED_ELITE_4_DRAKE)
            Coop_RaiseVar(VAR_ELITE_4_STATE, 4);
        sGuestFollowup = 9;
    }
    else if (sBattleKind == COOP_KIND_CHAMPION)
    {
        VarSet(VAR_TEMP_1, 1);
        sGuestFollowup = 10;
    }
    else if (sBattleKind == COOP_KIND_STORY && sBattleBadge >= COOP_GATE_CHIMNEY && sBattleBadge <= COOP_GATE_AQUA)
    {
        sGuestFollowup = 16 + sBattleBadge;
    }
    else if (sBattleKind == COOP_KIND_GYM && sBattleBadge >= 1 && sBattleBadge <= 8)
    {
        sGuestFollowup = sBattleBadge;
    }
    else if (sBattleKind == COOP_KIND_RIVAL && sRivalFightStage <= 4)
    {
        sGuestFollowup = 11 + sRivalFightStage;
    }
    else if (sBattleKind == COOP_KIND_TRAINER
          && TRAINER_BATTLE_PARAM.opponentA == TRAINER_GRUNT_PETALBURG_WOODS
          && gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_PETALBURG_WOODS)
          && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_PETALBURG_WOODS)
          && VarGet(VAR_PETALBURG_WOODS_STATE) == 0)
    {
        // The partner's Devon scene: this player helped beat the grunt, so
        // this save's scene is done too (its own GREAT BALL, its own var),
        // and the scene does not replay here. Story flags are not copied.
        sGuestFollowup = COOP_FOLLOWUP_WOODS;
    }
    else
    {
        return;
    }
    sPendingMessage = Coop_EventScript_GuestAfterBattle;
}

u16 Coop_TakeGuestFollowup(void)
{
    u16 followup = sGuestFollowup;

    sGuestFollowup = 0;
    return followup;
}

static void Coop_RaiseVar(u16 varId, u16 floor)
{
    u16 current = VarGet(varId);

    VarSet(varId, CoopProtocol_RaiseVar(current, floor));
}

// ---------------------------------------------------------------------------
// Shared story ledger.
//
// A milestone is story state both saves should agree on. Each save keeps the
// milestones it has (storyDone) and the ones it has written (storyApplied).
// Local progress is read back from the save's own vars, so a scene finished
// alone, on an older build, or before linking still counts. The partner's
// ledger only ever adds bits. Pending bits are written at a map transition,
// before objects load, so nobody is pulled into a scene or a cutscene.
// ---------------------------------------------------------------------------

static bool8 Coop_InRivalBedroom(void)
{
    u8 group = gSaveBlock1Ptr->location.mapGroup;
    u8 num = gSaveBlock1Ptr->location.mapNum;

    if (gSaveBlock2Ptr->playerGender == MALE)
        return group == MAP_GROUP(MAP_LITTLEROOT_TOWN_MAYS_HOUSE_2F) && num == MAP_NUM(MAP_LITTLEROOT_TOWN_MAYS_HOUSE_2F);
    return group == MAP_GROUP(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F) && num == MAP_NUM(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F);
}

static u16 Coop_LocalStory(void)
{
    u16 done = 0;

    if (VarGet(VAR_LITTLEROOT_RIVAL_STATE) >= 3)
        done |= 1 << COOP_STORY_RIVAL_MET;
    if (VarGet(VAR_NEW_MAUVILLE_STATE) >= 2)
        done |= 1 << COOP_STORY_POWER_RESTORED;
    done |= CoopQuest_LocalMilestones();
    return done;
}

// New local milestones are recorded as shared with partnerOt (0: done
// alone). Only shared ones travel to that same partner later.
static void Coop_RefreshLocalStoryWith(u32 partnerOt)
{
    struct CoopSave *save = CoopSave();
    u16 local, add;
    u8 i;

    if (save == NULL)
        return;
    local = Coop_LocalStory();
    add = local & ~save->storyDone & COOP_STORY_KNOWN_MASK;
    for (i = 0; i < COOP_STORY_COUNT; i++)
        if (add & (1 << i))
            save->storyOtId[i] = partnerOt;
    save->storyDone |= local;
    save->storyApplied |= local;
}

static void Coop_RefreshLocalStory(void)
{
    Coop_RefreshLocalStoryWith(Coop_PartnerOtId());
}

// The milestones this save offers the partner: those shared with them.
static u16 Coop_StoryForPartner(const struct CoopSave *save)
{
    u32 partnerOt = Coop_PartnerOtId();
    u16 bits = 0;
    u8 i;

    if (partnerOt == 0)
        return 0;
    for (i = 0; i < COOP_STORY_COUNT; i++)
        if ((save->storyDone & (1 << i)) && save->storyOtId[i] == partnerOt)
            bits |= 1 << i;
    return bits;
}

// The bedroom introduction, written without playing it. The rival stays in
// the bedroom only until Birch is rescued, as in the scene itself.
static bool8 Coop_ApplyRivalMet(void)
{
    // The opening still has to tell this player to go next door.
    if (VarGet(VAR_LITTLEROOT_INTRO_STATE) < 7)
        return FALSE;
    Coop_RaiseVar(VAR_LITTLEROOT_RIVAL_STATE, 3);
    Coop_RaiseVar(VAR_LITTLEROOT_TOWN_STATE, 1);
    if (gSaveBlock2Ptr->playerGender == MALE)
    {
        FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_2F_POKE_BALL);
        FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_MAY);
        if (FlagGet(FLAG_RESCUED_BIRCH))
            FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_RIVAL_BEDROOM);
        else
            FlagClear(FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_RIVAL_BEDROOM);
    }
    else
    {
        FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F_POKE_BALL);
        FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_BRENDAN);
        if (FlagGet(FLAG_RESCUED_BIRCH))
            FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_RIVAL_BEDROOM);
        else
            FlagClear(FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_RIVAL_BEDROOM);
    }
    return TRUE;
}

// Called from map loading before the transition script runs.
void Coop_ApplyPendingStory(void)
{
    struct CoopSave *save = CoopSave();
    u16 pending;
    u8 id;

    // FLAG_RESCUED_BIRCH is set before starter selection, so require the
    // completed route state as well. Do not touch Birch's later field NPC.
    if (FlagGet(FLAG_RESCUED_BIRCH) && VarGet(VAR_ROUTE101_STATE) >= 3)
    {
        FlagSet(FLAG_HIDE_ROUTE_101_ZIGZAGOON);
        FlagSet(FLAG_HIDE_ROUTE_101_BIRCH_ZIGZAGOON_BATTLE);
        if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ROUTE101)
         && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_ROUTE101))
            RemoveObjectEventByLocalIdAndMap(LOCALID_ROUTE101_ZIGZAGOON, MAP_NUM(MAP_ROUTE101), MAP_GROUP(MAP_ROUTE101));
    }
    if (save == NULL)
        return;
    Coop_RefreshLocalStory();
    pending = CoopProtocol_StoryPending(save->storyDone, save->storyApplied);
    if ((pending & (1 << COOP_STORY_RIVAL_MET)) && Coop_ApplyRivalMet())
        save->storyApplied |= 1 << COOP_STORY_RIVAL_MET;
    if ((pending & (1 << COOP_STORY_POWER_RESTORED)) && CoopQuest_ApplyPowerRestored())
        save->storyApplied |= 1 << COOP_STORY_POWER_RESTORED;
    for (id = COOP_STORY_SHOAL_RESCUED; id < COOP_STORY_COUNT; id++)
        if ((pending & (1 << id)) && CoopQuest_ApplyMilestone(id))
            save->storyApplied |= 1 << id;
}

// FALSE when the link could not take the block this frame.
static bool8 Coop_SendStory(u8 kind, u8 scene)
{
    struct CoopSave *save = CoopSave();
    struct CoopStoryMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.magic = COOP_STORY_MAGIC;
    msg.kind = kind;
    msg.scene = scene;
    msg.ledger = save != NULL ? Coop_StoryForPartner(save) : 0;
    msg.mapGroup = gSaveBlock1Ptr->location.mapGroup;
    msg.mapNum = gSaveBlock1Ptr->location.mapNum;
    msg.seq = ++sStorySeq;
    msg.checksum = CoopProtocol_StoryChecksum(&msg);
    if (!SendBlock(BitmaskAllOtherLinkPlayers(), &msg, sizeof(msg)))
    {
        sStorySeq--;
        return FALSE;
    }
    if (kind == COOP_STORY_MSG_LEDGER)
    {
        sStorySent = msg.ledger;
        sStorySentValid = TRUE;
    }
    return TRUE;
}

// A scene this game just started. A partner standing in the same room who
// still needs it joins from their own save instead of replaying it later.
u16 Coop_AnnounceScene(void)
{
    if (Coop_SessionActive() && Coop_PartnerOnSameMap())
        sSceneAnnounce = (u8)VarGet(VAR_0x8004);
    return 0;
}

// Story scenes started from a trigger tile that both players can be part
// of. Only one player fits on the tile (the other is blocked by them), so a
// partner who is nearby and free, with the scene still ahead on their own
// save, is taken to the tile and plays the scene too. Each save keeps its
// own state; nothing is copied across.
struct CoopCoordScene
{
    u16 map;
    s8 x;
    s8 y;
    u8 elevation;
};

static const struct CoopCoordScene sCoordScenes[] = {
    { MAP_METEOR_FALLS_1F_1R, 14, 18, 4 }, // Magma takes the meteorite (journey 38)
};

#define COOP_SCENE_COORD_BASE 16
#define COOP_SCENE_JOIN_RANGE 12
static EWRAM_DATA u8 sCoordJoinPending = 0; // scene index + 1, across the warp
EWRAM_DATA u16 gCoopSceneJoins = 0;           // test visibility

void Coop_NoteCoordScene(s16 x, s16 y)
{
    u16 map = (u16)(gSaveBlock1Ptr->location.mapNum | (gSaveBlock1Ptr->location.mapGroup << 8));
    u32 i;

    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap())
        return;
    for (i = 0; i < ARRAY_COUNT(sCoordScenes); i++)
    {
        if (sCoordScenes[i].map == map && sCoordScenes[i].x == x && sCoordScenes[i].y == y)
        {
            sSceneAnnounce = COOP_SCENE_COORD_BASE + i;
            return;
        }
    }
}

static bool8 Coop_CanJoinCoordScene(u8 index)
{
    const struct CoopCoordScene *scene = &sCoordScenes[index];
    s16 x, y;

    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(scene->map) || gSaveBlock1Ptr->location.mapNum != MAP_NUM(scene->map))
        return FALSE;
    PlayerGetDestCoords(&x, &y);
    x -= MAP_OFFSET;
    y -= MAP_OFFSET;
    if (abs(x - scene->x) + abs(y - scene->y) > COOP_SCENE_JOIN_RANGE)
        return FALSE;
    return GetCoordEventScriptAt(scene->x, scene->y, scene->elevation) != NULL;
}

// The joining player's game: over to the tile (a short fade); the scene
// starts once the map is back (Coop_PollCoordJoin).
u16 Coop_WarpToCoordScene(void)
{
    u8 index = gSpecialVar_0x8004;
    const struct CoopCoordScene *scene;

    if (index >= ARRAY_COUNT(sCoordScenes))
        return FALSE;
    scene = &sCoordScenes[index];
    sCoordJoinPending = index + 1;
    SetWarpDestination(MAP_GROUP(scene->map), MAP_NUM(scene->map), WARP_ID_NONE, scene->x, scene->y);
    DoWarp();
    ResetInitialPlayerAvatarState();
    return TRUE;
}

static void Coop_PollCoordJoin(void)
{
    const struct CoopCoordScene *scene;
    const u8 *script;
    s16 x, y;

    if (sCoordJoinPending == 0 || ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked()
     || gMain.callback2 != CB2_Overworld)
        return;
    scene = &sCoordScenes[sCoordJoinPending - 1];
    sCoordJoinPending = 0;
    PlayerGetDestCoords(&x, &y);
    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(scene->map) || gSaveBlock1Ptr->location.mapNum != MAP_NUM(scene->map)
     || x - MAP_OFFSET != scene->x || y - MAP_OFFSET != scene->y)
        return;
    script = GetCoordEventScriptAt(scene->x, scene->y, scene->elevation);
    if (script == NULL)
        return;
    if (gCoopSceneJoins < 0xFFFF)
        gCoopSceneJoins++;
    ScriptContext_SetupScript(script);
}

static bool8 Coop_CanJoinScene(u8 scene, u8 mapGroup, u8 mapNum)
{
    if (mapGroup != gSaveBlock1Ptr->location.mapGroup || mapNum != gSaveBlock1Ptr->location.mapNum)
        return FALSE;
    if (scene >= COOP_SCENE_COORD_BASE && scene < COOP_SCENE_COORD_BASE + ARRAY_COUNT(sCoordScenes))
        return Coop_CanJoinCoordScene(scene - COOP_SCENE_COORD_BASE);
    switch (scene)
    {
    case COOP_SCENE_RIVAL_INTRO:
        return Coop_InRivalBedroom() && VarGet(VAR_LITTLEROOT_RIVAL_STATE) == 2;
    default:
        return FALSE;
    }
}

static void Coop_HandleStoryMsg(u32 playerId, const struct CoopStoryMsg *msg)
{
    struct CoopSave *save = CoopSave();

    if (playerId == GetMultiplayerId() || msg->checksum != CoopProtocol_StoryChecksum(msg))
        return;
    if (sStorySeqKnown && !CoopProtocol_SeqIsNew(sStoryLastSeq, msg->seq))
        return;
    sStorySeqKnown = TRUE;
    sStoryLastSeq = msg->seq;
    sStoryPartner = msg->ledger;
    if (save != NULL)
    {
        u16 add = msg->ledger & ~save->storyDone & COOP_STORY_KNOWN_MASK;
        u8 i;

        for (i = 0; i < COOP_STORY_COUNT; i++)
            if (add & (1 << i))
                save->storyOtId[i] = Coop_PartnerOtId();
        save->storyDone = CoopProtocol_MergeStory(save->storyDone, msg->ledger);
    }
    if (msg->kind == COOP_STORY_MSG_SCENE && Coop_CanJoinScene(msg->scene, msg->mapGroup, msg->mapNum))
    {
        sSceneJoin = msg->scene;
        sSceneJoinAge = 0;
        sSceneJoinGroup = msg->mapGroup;
        sSceneJoinNum = msg->mapNum;
    }
}

extern const u8 Coop_EventScript_JoinRivalIntro[];
extern const u8 Coop_EventScript_JoinCoordScene[];

// Runs from the session task while linked, outside battle.
static void Coop_PollStory(void)
{
    struct CoopSave *save = CoopSave();
    bool8 idle;

    if (save == NULL)
        return;
    Coop_RefreshLocalStory();
    idle = !Coop_GuestBusy() && !sInBattle && !gCoopBattleLaunching;

    // Join now, or late: the partner finished the introduction while this
    // player was still in the bedroom with a menu or message open.
    if (idle && sSceneJoin == COOP_SCENE_NONE
     && (CoopProtocol_StoryPending(save->storyDone, save->storyApplied) & (1 << COOP_STORY_RIVAL_MET))
     && Coop_InRivalBedroom() && VarGet(VAR_LITTLEROOT_RIVAL_STATE) == 2)
    {
        sSceneJoin = COOP_SCENE_RIVAL_INTRO;
        sSceneJoinGroup = gSaveBlock1Ptr->location.mapGroup;
        sSceneJoinNum = gSaveBlock1Ptr->location.mapNum;
    }
    // A partner's trigger-tile scene is joined only soon after it began
    // (a long menu here means this player walks in on their own later).
    if (sSceneJoin >= COOP_SCENE_COORD_BASE && ++sSceneJoinAge > 600)
        sSceneJoin = COOP_SCENE_NONE;
    if (sSceneJoin != COOP_SCENE_NONE && idle)
    {
        u8 scene = sSceneJoin;

        sSceneJoin = COOP_SCENE_NONE;
        if (scene == COOP_SCENE_RIVAL_INTRO && Coop_CanJoinScene(scene, sSceneJoinGroup, sSceneJoinNum))
            ScriptContext_SetupScript(Coop_EventScript_JoinRivalIntro);
        else if (scene >= COOP_SCENE_COORD_BASE && Coop_CanJoinScene(scene, sSceneJoinGroup, sSceneJoinNum))
        {
            gSpecialVar_0x8004 = scene - COOP_SCENE_COORD_BASE;
            ScriptContext_SetupScript(Coop_EventScript_JoinCoordScene);
        }
    }

    if (gLinkTransferringData || sWaitingAck || gCoopBattleLaunching || gMain.callback2 != CB2_Overworld)
        return;
    if (sSceneAnnounce != COOP_SCENE_NONE)
    {
        // Kept until the link takes it (another block may be on its way,
        // such as a conversation note): a lost announcement would leave the
        // partner out of the scene.
        if (Coop_SendStory(COOP_STORY_MSG_SCENE, sSceneAnnounce))
            sSceneAnnounce = COOP_SCENE_NONE;
        sStoryTimer = 0;
        return;
    }
    // Send when the ledger changed; otherwise repeat slowly, and only while
    // the partner is not yet known to hold everything this save has.
    if (!sStorySentValid || Coop_StoryForPartner(save) != sStorySent
     || (++sStoryTimer >= COOP_STORY_RESEND && (Coop_StoryForPartner(save) & ~sStoryPartner) != 0))
    {
        sStoryTimer = 0;
        Coop_SendStory(COOP_STORY_MSG_LEDGER, COOP_SCENE_NONE);
    }
}

// Where the partner rival should stand in the joining player's bedroom: the
// PC, or the tile beside it when this player is standing there.
u16 Coop_PrepareRivalJoin(void)
{
    s16 x, y;

    PlayerGetDestCoords(&x, &y);
    x -= MAP_OFFSET;
    y -= MAP_OFFSET;
    gSpecialVar_0x8005 = 8;
    gSpecialVar_0x8006 = 2;
    if (x == 8 && y == 2)
        gSpecialVar_0x8005 = 7;
    return 0;
}

// The rival who answers this player's case. Set when the starter is handed
// out (from the role: the case player faces Soren, the companion Mira).
// 0 before the Route 103 fight, then one more for each rival fight won on
// this save (Rustboro, Route 110, Route 119, Lilycove), up to 5.
u16 Coop_TwinLineStage(void)
{
    struct CoopSave *save = CoopSave();

    if (save == NULL)
        return 0;
    return save->rivalStage > 5 ? 5 : save->rivalStage;
}

static u8 Coop_RivalId(void)
{
    struct CoopSave *save = CoopSave();

    if (save == NULL)
        return COOP_RIVAL_NONE;
    // Never from the party, its species, the player's gender or which
    // starter is owned: boxing, evolving or trading a starter does not
    // change the rivalry. A save that has a case but never recorded a role
    // (none since the role was saved at the rescue) is the case player,
    // once, and that is kept.
    if (save->rivalId == COOP_RIVAL_NONE && save->caseId <= 2 && FlagGet(FLAG_SYS_POKEMON_GET))
        save->rivalId = COOP_RIVAL_SOREN;
    return save->rivalId;
}

// Route 103: the twin who answers this player's case speaks up before the
// first fight. STR_VAR_1 is that twin's lead, STR_VAR_2 the player's lead.
// VAR_RESULT: 0 nothing to say (no case yet), 1 Soren, 2 Mira, plus 2 when
// that twin is the one this save already met at home, so the other twin
// introduces themselves instead.
u16 Coop_BufferRivalTaunt(void)
{
    struct CoopSave *save = CoopSave();
    u8 rival = Coop_RivalId();
    u8 home = gSaveBlock2Ptr->playerGender == MALE ? COOP_RIVAL_MIRA : COOP_RIVAL_SOREN;
    u8 type;

    if (save == NULL || rival == COOP_RIVAL_NONE || save->caseId > 2)
        return 0;
    // The counter case: grass faces fire, fire faces water, water faces
    // grass. Soren leads its case starter, Mira its companion.
    type = (save->caseId + 1) % 3;
    StringCopy(gStringVar1, GetSpeciesName(gCoopCaseStarters[type][rival == COOP_RIVAL_SOREN ? 0 : 1]));
    GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_NICKNAME, gStringVar2);
    StringGet_Nickname(gStringVar2);
    return rival + (rival == home ? 2 : 0);
}

// VAR_RESULT: COOP_RIVAL_* for this player. STR_VAR_1: that rival's name.
u16 Coop_BufferRival(void)
{
    static const u8 sText_Soren[] = _("SOREN");
    static const u8 sText_Mira[] = _("MIRA");
    u8 rival = Coop_RivalId();

    StringCopy(gStringVar1, rival == COOP_RIVAL_SOREN ? sText_Soren : sText_Mira);
    return rival;
}

// Shared progression items the partner's win hands this save. Each is
// delivered once: the save remembers a pending one until the bag has room,
// and an item already owned (or the HM flag) counts as delivered.
static const struct { u16 item; u16 flag; } sSharedItems[] = {
    [COOP_SHARED_DOWSING] = { ITEM_DOWSING_MACHINE, 0 },
    [COOP_SHARED_HM_FLY]  = { ITEM_HM_FLY, FLAG_RECEIVED_HM_FLY },
};

static bool8 Coop_SharedItemOwned(u8 id)
{
    if (sSharedItems[id].flag != 0 && FlagGet(sSharedItems[id].flag))
        return TRUE;
    return CheckBagHasItem(sSharedItems[id].item, 1);
}

// TRUE when the item is in the bag now. FALSE leaves it pending.
static bool8 Coop_GiveSharedItem(u8 id)
{
    struct CoopSave *save = CoopSave();

    if (Coop_SharedItemOwned(id) || AddBagItem(sSharedItems[id].item, 1))
    {
        if (sSharedItems[id].flag != 0)
            FlagSet(sSharedItems[id].flag);
        if (save != NULL)
            save->sharedItems &= ~(1 << id);
        return TRUE;
    }
    if (save != NULL)
        save->sharedItems |= 1 << id;
    StringCopy(gStringVar1, GetItemName(sSharedItems[id].item));
    return FALSE;
}

extern const u8 Coop_EventScript_SharedItemArrived[];

// A pending shared item arrives at a quiet moment on the field.
static void Coop_PollSharedItems(void)
{
    static EWRAM_DATA u8 sSharedItemTimer = 0;
    struct CoopSave *save = CoopSave();
    u8 id;

    if (save == NULL || save->sharedItems == 0 || gMain.callback2 != CB2_Overworld
     || ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked() || gMain.inBattle)
        return;
    if (++sSharedItemTimer < 60)
        return;
    sSharedItemTimer = 0;
    for (id = 0; id < ARRAY_COUNT(sSharedItems); id++)
    {
        if (!(save->sharedItems & (1 << id)))
            continue;
        if (Coop_SharedItemOwned(id))
        {
            save->sharedItems &= ~(1 << id);
            continue;
        }
        if (!CheckBagHasSpace(sSharedItems[id].item, 1))
            continue;
        AddBagItem(sSharedItems[id].item, 1);
        if (sSharedItems[id].flag != 0)
            FlagSet(sSharedItems[id].flag);
        save->sharedItems &= ~(1 << id);
        StringCopy(gStringVar1, GetItemName(sSharedItems[id].item));
        ScriptContext_SetupScript(Coop_EventScript_SharedItemArrived);
        return;
    }
}

static u16 Coop_GiveStoryItem(u16 item)
{
    if (CheckBagHasItem(item, 1))
        return 0;
    if (!AddBagItem(item, 1))
        return 1;
    return 0;
}

// VAR_0x8004 is the follow-up code. 0 means the guest can walk.
// 1 means a story item did not fit, and this scene was left in place.
// 2 means warp to Route 128, where the host arrives after Archie.
static u16 Coop_ApplyGuestStoryProgress(void)
{
    switch (VarGet(VAR_0x8004))
    {
    case 11:
        // The same state Route103_EventScript_RivalEnd writes for the talker.
        // Without the lab state Birch never handed the guest a Pokedex.
        FlagSet(FLAG_DEFEATED_RIVAL_ROUTE103);
        FlagSet(FLAG_HIDE_ROUTE_103_RIVAL);
        if (VarGet(VAR_BIRCH_LAB_STATE) < 4)
        {
            VarSet(VAR_BIRCH_LAB_STATE, 4);
            FlagClear(FLAG_HIDE_LITTLEROOT_TOWN_BIRCHS_LAB_RIVAL);
        }
        if (VarGet(VAR_OLDALE_RIVAL_STATE) < 1)
        {
            VarSet(VAR_OLDALE_RIVAL_STATE, 1);
            FlagClear(FLAG_HIDE_OLDALE_TOWN_RIVAL);
        }
        return 0;
    case 12:
        // The talker met the rival and registered them before the fight.
        FlagSet(FLAG_DEFEATED_RIVAL_RUSTBORO);
        FlagSet(FLAG_HIDE_RUSTBORO_CITY_RIVAL);
        FlagSet(FLAG_MET_RIVAL_RUSTBORO);
        FlagSet(FLAG_ENABLE_RIVAL_MATCH_CALL);
        if (VarGet(VAR_RUSTBORO_CITY_STATE) >= 7)
            Coop_RaiseVar(VAR_RUSTBORO_CITY_STATE, 8);
        if (VarGet(VAR_ROUTE104_STATE) >= 1)
            Coop_RaiseVar(VAR_ROUTE104_STATE, 2);
        return 0;
    case 13:
        // The story moves on even when the bag is full; the item waits in
        // the save and arrives once there is room (Coop_PollSharedItems).
        FlagSet(FLAG_HIDE_ROUTE_110_RIVAL);
        FlagSet(FLAG_HIDE_ROUTE_110_RIVAL_ON_BIKE);
        Coop_RaiseVar(VAR_ROUTE110_STATE, 1);
        return Coop_GiveSharedItem(COOP_SHARED_DOWSING) ? 0 : 3;
    case 14:
        FlagSet(FLAG_HIDE_ROUTE_119_RIVAL);
        FlagSet(FLAG_HIDE_ROUTE_119_RIVAL_ON_BIKE);
        if (VarGet(VAR_ROUTE119_STATE) < 1)
        {
            VarSet(VAR_ROUTE119_STATE, 1);
            VarSet(VAR_SCOTT_STATE, VarGet(VAR_SCOTT_STATE) + 1);
        }
        return Coop_GiveSharedItem(COOP_SHARED_HM_FLY) ? 0 : 3;
    case 15:
        // After Lilycove the rival is home again, as on the talker's save.
        if (!FlagGet(FLAG_MET_RIVAL_LILYCOVE))
        {
            if (gSaveBlock2Ptr->playerGender == MALE)
            {
                FlagClear(FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_RIVAL_BEDROOM);
                FlagClear(FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_2F_PICHU_DOLL);
            }
            else
            {
                FlagClear(FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_RIVAL_BEDROOM);
                FlagClear(FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F_SWABLU_DOLL);
            }
        }
        FlagSet(FLAG_MET_RIVAL_LILYCOVE);
        FlagSet(FLAG_HIDE_LILYCOVE_CITY_RIVAL);
        return 0;
    case 17:
        FlagSet(FLAG_HIDE_MT_CHIMNEY_TEAM_MAGMA);
        FlagSet(FLAG_HIDE_MT_CHIMNEY_TEAM_AQUA);
        FlagSet(FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY);
        FlagSet(FLAG_HIDE_METEOR_FALLS_1F_1R_COZMO);
        FlagClear(FLAG_HIDE_FALLARBOR_HOUSE_PROF_COZMO);
        FlagClear(FLAG_HIDE_MT_CHIMNEY_LAVA_COOKIE_LADY);
        return 0;
    case 18:
        if (VarGet(VAR_SLATEPORT_CITY_STATE) < 1)
        {
            FlagClear(FLAG_HIDE_SLATEPORT_CITY_CAPTAIN_STERN);
            FlagClear(FLAG_HIDE_SLATEPORT_CITY_GABBY_AND_TY);
            VarSet(VAR_SLATEPORT_CITY_STATE, 1);
            Coop_RaiseVar(VAR_SLATEPORT_HARBOR_STATE, 1);
        }
        FlagSet(FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT);
        FlagSet(FLAG_HIDE_MAGMA_HIDEOUT_GRUNTS);
        return 0;
    case 19:
        if (VarGet(VAR_SOOTOPOLIS_CITY_STATE) >= 6)
            return 0;
        Coop_RaiseVar(VAR_ROUTE128_STATE, 1);
        Coop_RaiseVar(VAR_SOOTOPOLIS_CITY_STATE, 1);
        Coop_RaiseVar(VAR_STEVENS_HOUSE_STATE, 2);
        Coop_RaiseVar(VAR_SEAFLOOR_CAVERN_STATE, 1);
        FlagClear(FLAG_HIDE_SOOTOPOLIS_CITY_STEVEN);
        FlagClear(FLAG_HIDE_SOOTOPOLIS_CITY_ARCHIE);
        FlagClear(FLAG_HIDE_SOOTOPOLIS_CITY_MAXIE);
        FlagClear(FLAG_HIDE_SOOTOPOLIS_CITY_RESIDENTS);
        FlagClear(FLAG_HIDE_SOOTOPOLIS_CITY_GROUDON);
        FlagClear(FLAG_HIDE_SOOTOPOLIS_CITY_KYOGRE);
        FlagSet(FLAG_HIDE_SOOTOPOLIS_CITY_MAN_1);
        FlagSet(FLAG_LEGENDARIES_IN_SOOTOPOLIS);
        FlagClear(FLAG_HIDE_ROUTE_128_ARCHIE);
        FlagClear(FLAG_HIDE_ROUTE_128_MAXIE);
        FlagSet(FLAG_SYS_WEATHER_CTRL);
        FlagSet(FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN);
        FlagSet(FLAG_HIDE_MOSSDEEP_CITY_STEVENS_HOUSE_STEVEN);
        FlagSet(FLAG_HIDE_SEAFLOOR_CAVERN_ROOM_9_ARCHIE);
        FlagSet(FLAG_HIDE_SEAFLOOR_CAVERN_ROOM_9_MAXIE);
        FlagSet(FLAG_HIDE_SEAFLOOR_CAVERN_ROOM_9_MAGMA_GRUNTS);
        FlagSet(FLAG_HIDE_SEAFLOOR_CAVERN_ROOM_9_KYOGRE);
        FlagSet(FLAG_HIDE_SEAFLOOR_CAVERN_AQUA_GRUNTS);
        return 2;
    case 20:
        Coop_RaiseVar(VAR_MOSSDEEP_CITY_STATE, 3);
        Coop_RaiseVar(VAR_MOSSDEEP_SPACE_CENTER_STATE, 3);
        Coop_RaiseVar(VAR_STEVENS_HOUSE_STATE, 1);
        FlagSet(FLAG_DEFEATED_MAGMA_SPACE_CENTER);
        FlagSet(FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_1F_TEAM_MAGMA);
        FlagSet(FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_2F_TEAM_MAGMA);
        FlagSet(FLAG_HIDE_MOSSDEEP_CITY_TEAM_MAGMA);
        FlagSet(FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_MAGMA_NOTE);
        if (VarGet(VAR_STEVENS_HOUSE_STATE) < 2)
            FlagClear(FLAG_HIDE_MOSSDEEP_CITY_STEVENS_HOUSE_STEVEN);
        return 0;
    case 21:
        FlagSet(FLAG_HIDE_ROUTE_119_TEAM_AQUA);
        FlagSet(FLAG_HIDE_WEATHER_INSTITUTE_2F_AQUA_GRUNT_M);
        if (VarGet(VAR_WEATHER_INSTITUTE_STATE) < 1)
        {
            VarSet(VAR_WEATHER_INSTITUTE_STATE, 1);
            FlagClear(FLAG_HIDE_WEATHER_INSTITUTE_2F_WORKERS);
        }
        return 0;
    case 22:
        FlagSet(FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE);
        FlagSet(FLAG_HIDE_LILYCOVE_CITY_AQUA_GRUNTS);
        FlagSet(FLAG_HIDE_AQUA_HIDEOUT_GRUNTS);
        return 0;
    default:
        return 0;
    }
}

// Story flags take effect on the current map immediately. Otherwise the
// guest can still see/talk to a rival or villain until the next map reload.
u16 Coop_ApplyGuestStory(void)
{
    u16 result = Coop_ApplyGuestStoryProgress();
    if (gMapHeader.events != NULL)
    {
        for (u32 i = 0; i < gMapHeader.events->objectEventCount; i++)
        {
            const struct ObjectEventTemplate *object = &gMapHeader.events->objectEvents[i];
            if (object->flagId != 0 && FlagGet(object->flagId))
                RemoveObjectEventByLocalIdAndMap(object->localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
        }
    }
    return result;
}

// The stock prize for one trainer (battle_script_commands.c), without the
// Amulet Coin: 4 x last Pokemon's level x class rate, doubled for a single
// trainer's double battle.
static u32 Coop_TrainerPrize(u16 trainerId, bool8 twoOpponents)
{
    const struct TrainerMon *party;
    u32 rate;

    if (trainerId == TRAINER_NONE || trainerId >= TRAINERS_COUNT || IsSpecialTrainer(trainerId))
        return 0;
    party = GetTrainerPartyFromId(trainerId);
    if (party == NULL || GetTrainerPartySizeFromId(trainerId) == 0)
        return 20;
    rate = gTrainerClasses[GetTrainerClassFromId(trainerId)].money ?: 5;
    return 4 * party[GetTrainerPartySizeFromId(trainerId) - 1].lvl * rate * (twoOpponents ? 1 : 2);
}

bool8 Coop_FinishTrainerBattle(void)
{
    bool8 won = gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT;
    bool8 firstClear = sBattleFlag == 0 || !FlagGet(sBattleFlag);

    // A duo Frontier battle: no prize, flags, EXP or white-out. Both games
    // put the real party back and record the result on their own run; the
    // inviter's desk script goes on, the partner gets its own note.
    if (sBattleKind == COOP_KIND_DUO && (gCoopDuoBattle || sHasSnapshot))
    {
        Coop_RestoreParty(FALSE);
        sInBattle = FALSE;
        CoopDuo_FinishBattle(won ? COOP_DUO_OUT_WON : COOP_DUO_OUT_LOST);
        gSpecialVar_Result = won ? TRUE : FALSE;
        if (sIsGuest)
        {
            sIsGuest = FALSE;
            ScriptContext_Init();
            sPendingMessage = CoopDuo_EventScript_GuestAfter;
            gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
            SetMainCallback2(CB2_ReturnToField);
            return TRUE;
        }
        sIsGuest = FALSE;
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        return TRUE;
    }

    Coop_RestoreParty(FALSE);
    sInBattle = FALSE;
    if (won)
        Coop_GiveJointWin();
    // A co-op fight is a link battle, and the link win script pays no prize,
    // so neither save earned one. Each pays itself the stock prize, once,
    // and only if nothing was paid during the battle.
    if (won && GetMoney(&gSaveBlock1Ptr->money) == sMoneyAtLaunch)
    {
        bool8 two = (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) != 0;
        u32 prize = Coop_TrainerPrize(TRAINER_BATTLE_PARAM.opponentA, two);

        if (two)
            prize += Coop_TrainerPrize(TRAINER_BATTLE_PARAM.opponentB, two);
        if (prize != 0)
            AddMoney(&gSaveBlock1Ptr->money, prize);
    }
    if (sBattleKind == COOP_KIND_PAIR)
    {
        u8 beat[2], fought[2];

        beat[0] = HasTrainerBeenFought(COOP_PAIR_LEFT_ID);
        beat[1] = HasTrainerBeenFought(COOP_PAIR_RIGHT_ID);
        fought[0] = TRAINER_BATTLE_PARAM.opponentA == COOP_PAIR_LEFT_ID || TRAINER_BATTLE_PARAM.opponentB == COOP_PAIR_LEFT_ID;
        fought[1] = TRAINER_BATTLE_PARAM.opponentA == COOP_PAIR_RIGHT_ID || TRAINER_BATTLE_PARAM.opponentB == COOP_PAIR_RIGHT_ID;
        CoopProtocol_PairFlagsAfter(beat, fought, won);
        if (beat[0])
            SetTrainerFlag(COOP_PAIR_LEFT_ID);
        if (beat[1])
            SetTrainerFlag(COOP_PAIR_RIGHT_ID);
    }
    gSpecialVar_Result = won ? TRUE : FALSE;
    if (sIsGuest && !Coop_PlayerLost() && won && firstClear)
        Coop_QueueGuestFollowup();
    if (sIsGuest && !Coop_PlayerLost())
    {
        ScriptContext_Init();
        gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
        SetMainCallback2(CB2_ReturnToField);
        sIsGuest = FALSE;
        return TRUE;
    }
    sIsGuest = FALSE;
    return FALSE;
}

// Catches stay off the save until the fight ends cleanly on this machine.
// A cable pull before then goes through Coop_CB2_AbortToField instead,
// which throws them away.
static void Coop_DeliverCatches(void)
{
    u32 i, slot;

    sNickCount = 0;
    sNickIndex = 0;
    for (i = 0; i < sCatchCount && i < COOP_WILD_SLOTS; i++)
    {
        struct Pokemon *mon = &sCatches[i];
        struct CoopNickTarget *target = &sNickTargets[sNickCount];
        u8 result;

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        for (slot = 0; slot < PARTY_SIZE; slot++)
        {
            if (GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_SPECIES) == SPECIES_NONE)
                break;
        }
        HandleSetPokedexFlagFromMon(mon, FLAG_SET_SEEN);
        HandleSetPokedexFlagFromMon(mon, FLAG_SET_CAUGHT);
        // The link master's game made both wild Pokémon, with its own OT id
        // in their encryption key; the stock capture rewrites only the field.
        SetBoxMonOtIdKeepData(&mon->box, READ_OTID_FROM_SAVE);
        result = GiveCapturedMonToPlayer(mon);
        if (result == MON_GIVEN_TO_PARTY)
        {
            target->inParty = TRUE;
            target->slot = slot;
        }
        else if (result == MON_GIVEN_TO_PC)
        {
            target->inParty = FALSE;
            target->box = gSpecialVar_MonBoxId;
            target->pos = gSpecialVar_MonBoxPos;
        }
        else
        {
            continue;
        }
        IncrementGameStat(GAME_STAT_POKEMON_CAPTURES);
        sNickCount++;
    }
    sCatchCount = 0;
    memset(sCatches, 0, sizeof(sCatches));
    // Each screen says who owns what: the partner's catches first, then
    // this player's own catches with their nickname prompts.
    if (sNickCount != 0 || sPartnerCaughtCount != 0)
        sPendingMessage = Coop_EventScript_CaughtNickname;
}

bool8 Coop_FinishWildBattle(void)
{
    bool8 won = gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT;
    u32 i;

    Coop_RestoreParty(FALSE);
    sInBattle = FALSE;
    for (i = 0; i < COOP_WILD_SLOTS; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES) != SPECIES_NONE)
            HandleSetPokedexFlagFromMon(&gParties[B_TRAINER_OPPONENT_A][i], FLAG_SET_SEEN);
    }
    Coop_DeliverCatches();
    if (sBattleKind == COOP_KIND_RESCUE)
    {
        Coop_EndRescueRequest();
        sRescueResult = 1;
        sBirchBag = FALSE;
        if (sIsGuest)
        {
            sRescueField = COOP_RESCUE_RETURN_GUEST;
            sRescueGuestFollow = TRUE;
            sRescueGuestTimer = 0;
        }
        else
        {
            sRescueField = COOP_RESCUE_RETURN_HOST;
        }
    }
    if (sBattleKind != COOP_KIND_LEGEND)
    {
        sIsGuest = FALSE;
        return FALSE;
    }
    {
        u8 bit = CoopLegend_Species(sLegendKind) != SPECIES_NONE ? 1 << (sLegendKind - 1) : 0;

        CoopLegend_OnSharedEnd(sLegendKind, (sLocalLegendMask & bit) != 0, (sPartnerLegendMask & bit) != 0, gCoopLastBall);
        if (((sLocalLegendMask | sPartnerLegendMask) & bit) && sPendingMessage == NULL)
            sPendingMessage = Coop_EventScript_CaughtNickname;
        // Not caught: on the guest's screen it flies off too, as it does on
        // the starter's (whose map script shows that); back next visit.
        else if (!((sLocalLegendMask | sPartnerLegendMask) & bit) && sIsGuest
              && gBattleOutcome != B_OUTCOME_LOST && gBattleOutcome != B_OUTCOME_DREW)
        {
            CoopLegend_OnSharedAway(sLegendKind, gBattleOutcome);
            if (sPendingMessage == NULL)
                sPendingMessage = Coop_EventScript_LegendGuestAway;
        }
    }
    (void)won;
    // 0 means the invite timed out and no battle outcome should be trusted.
    // A finished battle stores the real outcome so the cave script can tell
    // a KO, a catch, and a flee apart.
    gSpecialVar_Result = gBattleOutcome;
    if (sIsGuest && !Coop_PlayerLost())
    {
        ScriptContext_Init();
        gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
        SetMainCallback2(CB2_ReturnToField);
        sIsGuest = FALSE;
        return TRUE;
    }
    sIsGuest = FALSE;
    return FALSE;
}

static void Coop_CB2_AbortToField(void)
{
    bool8 rescue = sBattleKind == COOP_KIND_RESCUE;
    bool8 rescueGuest = rescue && sIsGuest;

    CoopBond_BattleAborted();   // no Bond XP from a dropped battle
    Coop_RestoreParty(TRUE);
    Coop_ClearCatchState();
    sAborting = FALSE;
    sInBattle = FALSE;
    sWaitingAck = FALSE;
    gCoopBattleLaunching = FALSE;
    gCoopArmWild = FALSE;
    sBirchBag = FALSE;
    UnlockPlayerFieldControls();
    FlagClear(FLAG_SYS_CTRL_OBJ_DELETE);
    if (rescue)
    {
        // The starters are already given. Both players still go to the lab.
        Coop_EndRescueRequest();
        sRescueResult = 1;
        if (rescueGuest)
        {
            ScriptContext_Init();
            sRescueGuestFollow = TRUE;
            sRescueGuestTimer = COOP_RESCUE_TIMEOUT;
        }
        else
        {
            ScriptContext_Enable();
        }
    }
    else
    {
        ScriptContext_Init();
        sPendingMessage = Coop_EventScript_BattleAborted;
    }
    gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
    SetMainCallback2(CB2_ReturnToField);
}

void Coop_AbortBattle(void)
{
    if (sAborting || !(gBattleTypeFlags & BATTLE_TYPE_COOP))
        return;
    sAborting = TRUE;
    gBattleOutcome = B_OUTCOME_LINK_BATTLE_RAN;
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gMain.savedCallback = Coop_CB2_AbortToField;
    SetMainCallback2(CB2_QuitRecordedBattle);
}

// Why an ordinary fight here would stay solo, from this game's side, when
// linked with the Beacon on (otherwise solo is simply expected: NONE).
static u8 Coop_SoloReasonHere(void)
{
    if (sLinkState != COOP_LINK_UP || !sBeacon.on)
        return COOP_SOLO_NONE;
    // Special facilities have their own encounter and party rules.
    if (Coop_PlaceStaysSolo())
        return COOP_SOLO_PLACE;
    if (!Coop_SessionActive() || !sPartnerValid)
        return COOP_SOLO_RECOVERING;
    if (!Coop_PartnerOnSameMap())
        return COOP_SOLO_ELSEWHERE;
    // The partner is in a fight of their own: no invite (it would wait out
    // its whole timeout, unanswered).
    if (sPartner.talkLocalId == COOP_TALK_IN_BATTLE)
        return COOP_SOLO_BUSY;
    if (!Coop_HasBattleReadyLead())
        return COOP_SOLO_SELF_NOT_READY;
    if (Coop_InteractionBusy() || sPendingMessage != NULL)
        return COOP_SOLO_BUSY;
    return COOP_SOLO_NONE;
}

bool8 Coop_WildPairReady(void)
{
    sSoloNote = Coop_SoloReasonHere();
    if (sSoloNote != COOP_SOLO_NONE)
        return FALSE;
    return Coop_SessionActive() && sBeacon.on;
}

// Fishing, Rock Smash, roamers, outbreaks, and scripted encounters keep
// their stock one-player path. Say so when co-op was otherwise possible.
void Coop_NoteSoloOnlyEncounter(void)
{
    sSoloNote = Coop_SoloReasonHere();
    if (sSoloNote == COOP_SOLO_NONE && Coop_SessionActive() && sBeacon.on)
        sSoloNote = COOP_SOLO_KIND;
}

// The note for the first battle message of a solo fight, read once.
const u8 *Coop_TakeSoloNote(void)
{
    static const u8 *const sNotes[COOP_SOLO_COUNT] = {
        [COOP_SOLO_ELSEWHERE]         = COMPOUND_STRING("Solo: partner is on another map."),
        [COOP_SOLO_BUSY]              = COMPOUND_STRING("Solo: your partner is busy."),
        [COOP_SOLO_NO_ANSWER]         = COMPOUND_STRING("Solo: your partner didn't answer."),
        [COOP_SOLO_DECLINED]          = COMPOUND_STRING("Solo: your partner said no."),
        [COOP_SOLO_PARTNER_NOT_READY] = COMPOUND_STRING("Solo: partner's team can't fight."),
        [COOP_SOLO_SELF_NOT_READY]    = COMPOUND_STRING("Solo: your front three can't fight."),
        [COOP_SOLO_RECOVERING]        = COMPOUND_STRING("Solo: the link is recovering."),
        [COOP_SOLO_PLACE]             = COMPOUND_STRING("Solo: no co-op battles here."),
        [COOP_SOLO_KIND]              = COMPOUND_STRING("Solo: a one-player encounter."),
    };
    u8 note = sSoloNote;

    sSoloNote = COOP_SOLO_NONE;
    if (note == COOP_SOLO_NONE || note >= COOP_SOLO_COUNT || (gBattleTypeFlags & BATTLE_TYPE_COOP))
        return NULL;
    return sNotes[note];
}

// The two wild Pokémon are already in the enemy party. The invite carries
// species and levels so the guest can build its own copy; the link master's
// copy is the one both games fight.
void Coop_QueueWildPair(void)
{
    struct Pokemon *wild = gParties[B_TRAINER_OPPONENT_A];

    sResumeScript = FALSE;
    sLegendKind = 0;
    Coop_QueueInvite(COOP_KIND_WILD,
                     GetMonData(&wild[0], MON_DATA_SPECIES),
                     GetMonData(&wild[1], MON_DATA_SPECIES),
                     GetMonData(&wild[0], MON_DATA_LEVEL),
                     GetMonData(&wild[1], MON_DATA_LEVEL));
    sBattleBadge = 0;
    sBattleFlag = 0;
    TRAINER_BATTLE_PARAM.opponentA = 0;
    TRAINER_BATTLE_PARAM.opponentB = 0;
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
}

// Co-op battles against the game (wild Pokémon, trainers): each player may
// use their own bag. Not in practice copies or the Twin Trial (a
// facility-style challenge), and not in friendly battles (no co-op flag).
bool8 Coop_BattleItemsAllowed(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_COOP) || gCoopPracticeBattle || gCoopDuoBattle)
        return FALSE;
    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER) && Coop_IsTwinTrialTrainer(TRAINER_BATTLE_PARAM.opponentA))
        return FALSE;
    return TRUE;
}

bool8 Coop_IsWildBattle(void)
{
    return (gBattleTypeFlags & BATTLE_TYPE_COOP) && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER);
}

static u8 Coop_WildSlot(u32 target)
{
    return GetBattlerPosition(target) == B_POSITION_OPPONENT_LEFT ? 0 : 1;
}

// 1 + the thrower's multiplayer id. The left human is always id 0.
static u8 Coop_ThrowerId(u32 thrower)
{
    return GetBattlerPosition(thrower) == B_POSITION_PLAYER_LEFT ? 1 : 2;
}

static bool8 Coop_TargetOnField(u32 target)
{
    return IsBattlerAlive(target) && !gBattleStruct->battlerState[target].notOnField;
}

bool8 Coop_CanCatchTarget(u32 target)
{
    return CoopProtocol_CanCatch(&sLedger, Coop_WildSlot(target), Coop_TargetOnField(target));
}

void Coop_ClaimCatch(u32 thrower, u32 target)
{
    CoopProtocol_ClaimCatch(&sLedger, Coop_WildSlot(target), Coop_ThrowerId(thrower), Coop_TargetOnField(target));
}

static u16 Coop_StorageFree(void)
{
    u32 box, pos;
    u16 free = 0;

    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            if (GetBoxMonDataAt(box, pos, MON_DATA_SPECIES) == SPECIES_NONE && ++free > COOP_WILD_SLOTS)
                return free;
        }
    }
    return free;
}

u16 Coop_PcFreeSlots(void)
{
    return Coop_StorageFree();
}

// The partner is linked, sending frames, and on this map.
bool8 Coop_DealPartnerHere(void)
{
    return sLinkState == COOP_LINK_UP && sPartnerValid && Coop_PartnerOnSameMap() && sUnlinkTimer == 0;
}

// Anything else in co-op that a trade or friendly battle must wait for.
bool8 Coop_OtherInteractionBusy(void)
{
    return sProgressPending || sInBattle || gMain.inBattle || sWaitingAck || sWaitKind != 0
        || sGuestPairPending || sJoinLaunchPending || gCoopBattleLaunching || sPendingMessage != NULL;
}

bool8 Coop_HasCatchRoom(void)
{
    struct Pokemon *party = sHasSnapshot ? sPartySnap : gParties[B_TRAINER_PLAYER];
    u8 count = 0;
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE)
            count++;
    }
    return CoopProtocol_CatchRoom(count, sCatchCount, Coop_StorageFree());
}

// Runs in the ball animation on both games. Only the thrower's game keeps a
// copy of the Pokémon; the other game only notes a legendary for ownership.
void Coop_NoteCatch(u32 thrower, u32 target)
{
    struct Pokemon *mon = GetBattlerMon(target);
    u8 enc = CoopLegend_EncForSpecies(GetMonData(mon, MON_DATA_SPECIES));
    u8 index = enc != 0 ? enc - 1 : 0xFF;

    if (BattlerIsPlayer(thrower))
    {
        if (sCatchCount < COOP_WILD_SLOTS)
        {
            struct Pokemon *kept = &sCatches[sCatchCount++];

            *kept = *mon;
            if (gCoopLastBall != ITEM_NONE && GetItemPocket(gCoopLastBall) == POCKET_POKE_BALLS)
            {
                enum PokeBall ballId = ItemIdToBallId(gCoopLastBall);
                SetMonData(kept, MON_DATA_POKEBALL, &ballId);
            }
        }
        if (index != 0xFF)
            sLocalLegendMask |= 1 << index;
    }
    else
    {
        // A shared legendary is not "theirs": each trainer receives one
        // (the after-battle note says so instead).
        if (sPartnerCaughtCount < COOP_WILD_SLOTS && !(sBattleKind == COOP_KIND_LEGEND && index != 0xFF))
            sPartnerCaught[sPartnerCaughtCount++] = GetMonData(mon, MON_DATA_SPECIES);
        if (index != 0xFF)
            sPartnerLegendMask |= 1 << index;
    }
}

// After a co-op wild fight: the partner's catches, one per call.
// VAR_RESULT is the species (STR_VAR_1 its name), or SPECIES_NONE when done.
u16 Coop_NextPartnerCatch(void)
{
    u16 species;

    if (sPartnerCaughtCount == 0)
        return SPECIES_NONE;
    species = sPartnerCaught[--sPartnerCaughtCount];
    sPartnerCaught[sPartnerCaughtCount] = SPECIES_NONE;
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return SPECIES_NONE;
    StringCopy(gStringVar1, GetSpeciesName(species));
    return species;
}

u16 Coop_NextCaughtNickname(void)
{
    while (sNickIndex < sNickCount)
    {
        struct CoopNickTarget *target = &sNickTargets[sNickIndex++];
        u16 species;

        if (target->inParty)
        {
            gSpecialVar_0x8004 = target->slot;
            species = GetMonData(&gParties[B_TRAINER_PLAYER][target->slot], MON_DATA_SPECIES);
        }
        else
        {
            gSpecialVar_0x8004 = PC_MON_CHOSEN;
            gSpecialVar_MonBoxId = target->box;
            gSpecialVar_MonBoxPos = target->pos;
            species = GetBoxMonDataAt(target->box, target->pos, MON_DATA_SPECIES);
        }
        if (species == SPECIES_NONE)
            continue;
        StringCopy(gStringVar1, GetSpeciesName(species));
        return species;
    }
    sNickCount = 0;
    sNickIndex = 0;
    return SPECIES_NONE;
}

static bool8 Coop_PairBeatenLocally(void)
{
    return HasTrainerBeenFought(COOP_PAIR_LEFT_ID) || HasTrainerBeenFought(COOP_PAIR_RIGHT_ID);
}

static int Coop_PairPlanFromHere(bool8 partnerFree, bool8 accepted)
{
    u8 local[2], remote[2] = { 0, 0 };

    // The other save checks its own two flags before it says yes.
    local[0] = HasTrainerBeenFought(COOP_PAIR_LEFT_ID);
    local[1] = HasTrainerBeenFought(COOP_PAIR_RIGHT_ID);
    return CoopProtocol_PairPlan(local, remote, partnerFree, accepted);
}

u16 Coop_BufferPairNames(void)
{
    StringCopy(gStringVar1, GetTrainerNameFromId(COOP_PAIR_LEFT_ID));
    StringCopy(gStringVar2, GetTrainerNameFromId(COOP_PAIR_RIGHT_ID));
    return TRUE;
}

// Called from BattleSetup_StartTrainerBattle when this player walked into
// Rick or Allen. Returning FALSE leaves the stock solo fight to run.
bool8 Coop_TryStartTrainerPair(void)
{
    u16 trainerId = TRAINER_BATTLE_PARAM.opponentA;

    if (trainerId != COOP_PAIR_LEFT_ID && trainerId != COOP_PAIR_RIGHT_ID)
        return FALSE;
    // Both spotted this player at once: that is the stock two-trainer fight.
    if (TRAINER_BATTLE_PARAM.opponentB != 0)
        return FALSE;
    if (!Coop_SessionActive() || !sBeacon.on || !Coop_PartnerOnSameMap())
        return FALSE;
    if (!Coop_HasBattleReadyLead() || sPendingMessage != NULL)
        return FALSE;
    if (Coop_PairPlanFromHere(!Coop_InteractionBusy(), TRUE) != COOP_PAIR_JOINT)
        return FALSE;
    sPairEngaged = trainerId;
    Coop_BufferPairNames();
    ScriptContext_CallScript(Coop_EventScript_PairAskHost);
    return TRUE;
}

// VAR_0x8004 holds the host's yes or no. Either way the trainer script waits
// here until a battle has run: the pair, or the engaged NPC alone.
u16 Coop_PairHostAnswer(void)
{
    bool8 accepted = VarGet(VAR_0x8004) != 0;

    if (accepted && Coop_SessionActive() && Coop_PartnerOnSameMap() && Coop_HasBattleReadyLead()
     && Coop_PairPlanFromHere(!Coop_InteractionBusy(), TRUE) == COOP_PAIR_JOINT)
    {
        sResumeScript = FALSE;
        sLegendKind = 0;
        Coop_QueueInvite(COOP_KIND_PAIR, COOP_PAIR_LEFT_ID, COOP_PAIR_RIGHT_ID, 0, 0);
        return TRUE;
    }
    TRAINER_BATTLE_PARAM.opponentA = sPairEngaged;
    BattleSetup_ResumeSoloTrainerBattle();
    return FALSE;
}

// 1 launched the pair fight, 2 means the ask ran out of time, 0 is a no.
u16 Coop_PairGuestAnswer(void)
{
    bool8 accepted = gSpecialVar_Result != 0;
    u16 seq = sGuestPairInvite.seq;

    u8 kind = sGuestPairInvite.kind;

    // Too late: an optional fight went ahead without this player (2); a
    // shared story fight did not start at all (4).
    if (!sGuestPairPending)
        return Coop_IsMandatoryKind(kind) ? 4 : 2;
    sGuestPairPending = FALSE;
    if (sGuestPairTimer >= COOP_ANSWER_SECONDS
     || !Coop_SessionActive() || !Coop_PartnerOnSameMap())
    {
        Coop_SendReplyWhy(COOP_KIND_NACK, seq, COOP_SOLO_NO_ANSWER);
        if (!Coop_IsMandatoryKind(kind))
            return 2;
        // This player's own time ran out (5), or the partner withdrew or left (4).
        return sGuestPairTimer != 0xFFFF && sGuestPairTimer >= COOP_ANSWER_SECONDS
            && Coop_SessionActive() && Coop_PartnerOnSameMap() ? 5 : 4;
    }
    if (accepted)
        Coop_NoteStartTime(COOP_START_ANSWERED);
    if (!accepted || !(kind == COOP_KIND_DUO || (Coop_IsMandatoryKind(kind) ? Coop_HasBattleReadyMon() : Coop_HasBattleReadyLead()))
     || (kind == COOP_KIND_PAIR && Coop_PairPlanFromHere(TRUE, accepted) != COOP_PAIR_JOINT))
    {
        Coop_SendReplyWhy(COOP_KIND_NACK, seq, !accepted ? COOP_SOLO_DECLINED : COOP_SOLO_PARTNER_NOT_READY);
        return 0;
    }
    if (Coop_IsMandatoryKind(kind))
    {
        // Agreed: both players now choose their team (src/coop_prep.c); the
        // fight starts when both are READY. This script waits meanwhile.
        CoopPrep_BeginGuest(seq, GetMultiplayerId() ^ 1);
        return 1;
    }
    if (kind == COOP_KIND_LEGEND && !CoopLegend_Eligible((u8)sGuestPairInvite.aux))
    {
        Coop_SendReplyWhy(COOP_KIND_NACK, seq, COOP_SOLO_LEGEND_NOT_ELIGIBLE);
        return 0;
    }
    sBattleKind = kind;
    sBattleBadge = 0;
    sBattleFlag = 0;
    sLegendKind = kind == COOP_KIND_LEGEND ? (u8)sGuestPairInvite.aux : 0;
    sRivalFightStage = 0xFF;
    sRemoteTeamKey = sGuestPairInvite.teamKey;
    if (kind == COOP_KIND_LEGEND)
    {
        TRAINER_BATTLE_PARAM.opponentA = 0;
        TRAINER_BATTLE_PARAM.opponentB = 0;
        Coop_SendReply(COOP_KIND_ACK, seq);
        Coop_LaunchBattle(TRUE);
        return 1;
    }
    if (kind == COOP_KIND_WILD)
    {
        TRAINER_BATTLE_PARAM.opponentA = 0;
        TRAINER_BATTLE_PARAM.opponentB = 0;
        CreateScriptedDoubleWildMon(sGuestPairInvite.trainerA, sGuestPairInvite.badge, ITEM_NONE,
                                    sGuestPairInvite.trainerB, (u8)sGuestPairInvite.aux, ITEM_NONE);
        Coop_SendReply(COOP_KIND_ACK, seq);
        sJoinLaunchPending = TRUE;
        sInBattle = TRUE;
        return 3;
    }
    else
    {
        sBattleBadge = sGuestPairInvite.badge;
        sBattleFlag = kind == COOP_KIND_PAIR ? 0 : sGuestPairInvite.aux;
        TRAINER_BATTLE_PARAM.opponentA = sGuestPairInvite.trainerA;
        TRAINER_BATTLE_PARAM.opponentB = sGuestPairInvite.trainerB;
    }
    Coop_SendReply(COOP_KIND_ACK, seq);
    Coop_LaunchBattle(TRUE);
    return 1;
}

// The shared story fight's question: STR_VAR_1 the trainers ("ROXANNE &
// HEATH"), STR_VAR_2 the partner.
void Coop_BufferStoryAsk(void)
{
    u8 *end = gStringVar1;

    *end = EOS;
    if (sGuestPairInvite.trainerA != TRAINER_NONE && sGuestPairInvite.trainerA < TRAINERS_COUNT)
        end = StringCopy(end, GetTrainerNameFromId(sGuestPairInvite.trainerA));
    if (sGuestPairInvite.trainerB != TRAINER_NONE && sGuestPairInvite.trainerB < TRAINERS_COUNT)
    {
        end = StringCopy(end, COMPOUND_STRING(" & "));
        StringCopy(end, GetTrainerNameFromId(sGuestPairInvite.trainerB));
    }
    StringCopyN(gStringVar2, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
    gStringVar2[PLAYER_NAME_LENGTH] = EOS;
}

// STR_VAR_1: what the partner ran into, for the join question.
u16 Coop_BufferJoinAsk(void)
{
    if (sGuestPairInvite.kind == COOP_KIND_WILD && sGuestPairInvite.trainerA < NUM_SPECIES)
    {
        StringCopy(gStringVar1, GetSpeciesName(sGuestPairInvite.trainerA));
        return COOP_KIND_WILD;
    }
    if (sGuestPairInvite.trainerA < TRAINERS_COUNT)
        StringCopy(gStringVar1, GetTrainerNameFromId(sGuestPairInvite.trainerA));
    return COOP_KIND_TRAINER;
}

// Called from BattleSetup when the rival script has already started a battle.
bool8 Coop_InterceptRival(u16 trainerId)
{
    u8 stage = Coop_RivalStageFromId(trainerId);

    if (stage == 0xFF || gCoopBattleLaunching)
        return FALSE;
    // Not linked: this player fights their own twin alone, with the solo
    // team for this stage. The stock scene then continues.
    if (!Coop_SessionActive())
    {
        u16 solo = Coop_SoloRivalTrainer(stage);

        if (solo != TRAINER_NONE)
            TRAINER_BATTLE_PARAM.opponentA = solo;
        TRAINER_BATTLE_PARAM.opponentB = TRAINER_NONE;
        sSoloRivalStage = stage + 1;
        return FALSE;
    }
    // Refused here, the scene has already walked the rival up to this player.
    // Coop_EventScript_RivalWaits says why, then puts the rival back on its
    // own tile with the map's music, so the scene can run again in full.
    if (!Coop_PartnerOnSameMap())
    {
        sRefusal = COOP_REFUSE_NONE;
        ScriptContext_SetupScript(Coop_EventScript_RivalWaits);
        return TRUE;
    }
    if (!Coop_HasBattleReadyMon())
    {
        sRefusal = COOP_REFUSE_SELF_NOT_READY;
        ScriptContext_SetupScript(Coop_EventScript_RivalWaits);
        return TRUE;
    }
    if (!sBeacon.on)
    {
        sRefusal = COOP_REFUSE_BEACON_OFF;
        ScriptContext_SetupScript(Coop_EventScript_RivalWaits);
        return TRUE;
    }
    Coop_QueueRival(stage);
    ScriptContext_Stop();
    return TRUE;
}

// After a refused rival scene: every rival-looking object on this map goes
// back to the tile and facing it was spawned with, and the twin re-forms.
extern const u8 gInitialMovementTypeFacingDirections[];

u16 Coop_ResetRivalScene(void)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *obj = &gObjectEvents[i];

        if (!obj->active || obj->isPlayer || obj->localId >= LOCALID_CAMERA
         || obj->localId == OBJ_EVENT_ID_COOP_PARTNER || obj->localId == OBJ_EVENT_ID_COOP_TWIN
         || !Coop_IsRivalGraphics(obj->graphicsId))
            continue;
        ObjectEventClearHeldMovementIfActive(obj);
        if (obj->currentCoords.x != obj->initialCoords.x || obj->currentCoords.y != obj->initialCoords.y)
            MoveObjectEventToMapCoords(obj, obj->initialCoords.x, obj->initialCoords.y);
        ObjectEventTurn(obj, gInitialMovementTypeFacingDirections[obj->movementType]);
    }
    return 0;
}

// The guest has no battle script engine. Its own controller records every
// crossed level; normal field move-learning UI handles replacements afterward.
void Coop_RecordLevelUp(u8 slot, u8 oldLevel)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_COOP) || slot >= COOP_BATTLE_SLOTS || gCoopDuoBattle)
        return;
    if (sProgressLevels[slot] == 0)
        sProgressLevels[slot] = oldLevel;
    sProgressGuest = !(gBattleTypeFlags & BATTLE_TYPE_IS_MASTER);
    sProgressPending = TRUE;
    if (sProgressGuest)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];
        for (u32 level = oldLevel; level < GetMonData(mon, MON_DATA_LEVEL); level++)
            AdjustFriendship(mon, FRIENDSHIP_EVENT_GROW_LEVEL);
        gLeveledUpInBattle |= 1u << slot;
    }
}

u16 Coop_NextProgression(void)
{
    for (; sProgressSlot < COOP_BATTLE_SLOTS; sProgressSlot++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][CoopPrep_PartySlotForBattleSlot(sProgressSlot)];
        u32 oldLevel = sProgressLevels[sProgressSlot];
        if (oldLevel == 0)
            continue;
        if (sProgressGuest)
        {
            const struct LevelUpMove *moves = GetSpeciesLevelUpLearnset(GetMonData(mon, MON_DATA_SPECIES));
            while (moves[sProgressMoves[sProgressSlot]].move != LEVEL_UP_MOVE_END)
            {
                const struct LevelUpMove *entry = &moves[sProgressMoves[sProgressSlot]++];
                if (entry->level > oldLevel && entry->level <= GetMonData(mon, MON_DATA_LEVEL))
                {
                    u32 i;
                    for (i = 0; i < MAX_MON_MOVES; i++)
                        if (GetMonData(mon, MON_DATA_MOVE1 + i) == entry->move)
                            break;
                    if (i < MAX_MON_MOVES)
                        continue;
                    gSpecialVar_0x8004 = CoopPrep_PartySlotForBattleSlot(sProgressSlot);
                    gSpecialVar_0x8005 = entry->move;
                    return 1;
                }
            }
        }
        sProgressLevels[sProgressSlot] = 0;
        bool32 canStop = TRUE;
        sProgressEvolution = GetEvolutionTargetSpecies(mon, EVO_MODE_BATTLE_ONLY, 0, NULL, &canStop, CHECK_EVO);
        sProgressCanStop = canStop;
        if (sProgressEvolution != SPECIES_NONE)
            return 2;
    }
    gLeveledUpInBattle = 0;
    return 0;
}

void Coop_StartProgressionEvolution(void)
{
    u8 slot = CoopPrep_PartySlotForBattleSlot(sProgressSlot);
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];
    bool32 canStop = sProgressCanStop;
    GetEvolutionTargetSpecies(mon, EVO_MODE_BATTLE_ONLY, 0, NULL, &canStop, DO_EVO);
    gCB2_AfterEvolution = CB2_ReturnToFieldContinueScript;
    BeginEvolutionScene(mon, sProgressEvolution, canStop, slot);
}

// Pokemon Center nurse: TRUE when the last heal this session was at this same
// Center, so the nurse skips her welcome and the YES/NO question. The first
// visit after a power-on, after declining, or after another Center asks.
static EWRAM_DATA u16 sLastNurseMap = 0; // map + 1, 0 = none yet

static u16 Nurse_HereId(void)
{
    return ((gSaveBlock1Ptr->location.mapGroup << 8) | gSaveBlock1Ptr->location.mapNum) + 1;
}

u16 Nurse_RepeatVisit(void)
{
    return sLastNurseMap == Nurse_HereId();
}

void Nurse_NoteHeal(void)
{
    sLastNurseMap = Nurse_HereId();
}

// Route 101 without a link: the opener's starter, then one POOCHYENA.
u16 Coop_StartSoloRescueBattle(void)
{
    u16 caseId = VarGet(VAR_STARTER_MON);

    if (caseId > 2)
        caseId = 0;
    Coop_GiveRescueStarter((u8)caseId, TRUE);
    CreateScriptedWildMon(SPECIES_POOCHYENA, 2, ITEM_NONE);
    BattleSetup_StartSoloRescueBattle();
    return TRUE;
}

// Terra/Marine Cave scripts: a co-op duo fight settles both legends; a solo
// fight settles only the one in this cave, so the other cave stays open.

void Coop_NoteStartTime(u8 stage)
{
    if (stage >= COOP_START_COUNT)
        return;
    if (stage == COOP_START_QUEUED || stage == COOP_START_ASKED)
    {
        // A new request: forget the last one's times.
        if (stage == COOP_START_QUEUED || gCoopStartTimes[COOP_START_QUEUED] == 0
         || gCoopStartTimes[COOP_START_FIRST_ACTION] != 0)
            memset(gCoopStartTimes, 0, sizeof(gCoopStartTimes));
    }
    if (gCoopStartTimes[stage] == 0)
        gCoopStartTimes[stage] = gMain.vblankCounter1 | 1;
}

// --- Team preparation (src/coop_prep.c) ----------------------------------

void Coop_WaitClockStart(struct CoopWaitClock *clock)
{
    Coop_ClockStart(clock);
}

u8 Coop_WaitClockTick(struct CoopWaitClock *clock)
{
    return Coop_ClockTick(clock);
}

// The host's own story invite is waiting for this session's answer.
bool8 Coop_PrepHostCanStart(u16 session)
{
    return sWaitingAck && sInviteSent && Coop_IsMandatoryKind(sBattleKind) && sInvite.seq == session
        && Coop_SessionActive() && !gMain.inBattle && !sInBattle;
}

// The partner agreed and is choosing: the waiting note and the trainer's
// line give way to the team screen.
void Coop_PrepHostStarted(void)
{
    Coop_HideCallingMessage();
    HideFieldMessageBox();
    gCoopWaitSecondsLeft = 0;
}

// The preparation ended without a fight: the host's story script resumes
// with "nothing started" and why.
void Coop_PrepHostEnd(u8 refusal)
{
    if (!sWaitingAck)
        return;
    sRefusal = refusal;
    Coop_FailWait();
}

// Unlinked lead choice stepped back from: the gym script resumes with
// "nothing started".
void Coop_PrepSoloEnd(u8 refusal)
{
    sRefusal = refusal;
    gSpecialVar_Result = FALSE;
    ScriptContext_Enable();
}

// Both READY and the host asked to start: the partner's fight, on this game.
void Coop_PrepGuestCommit(void)
{
    Coop_AcceptMandatory(&sGuestPairInvite);
}

bool8 Coop_PrepPartnerHere(void)
{
    return sPartnerValid && Coop_SessionActive();
}

bool8 Coop_PrepIsHostSide(void)
{
    return sWaitingAck && Coop_IsMandatoryKind(sBattleKind);
}

// "ROXANNE & HEATH": who the prepared fight is against.
void Coop_BufferPrepOpponents(u8 *dst, bool8 host)
{
    u16 a = host ? sInvite.trainerA : sGuestPairInvite.trainerA;
    u16 b = host ? sInvite.trainerB : sGuestPairInvite.trainerB;

    if ((host ? sInvite.kind : sGuestPairInvite.kind) == COOP_KIND_DUO)
    {
        CoopDuo_BufferPrepTitle(dst);
        return;
    }

    *dst = EOS;
    if (a != TRAINER_NONE && a < TRAINERS_COUNT)
        dst = StringCopy(dst, GetTrainerNameFromId(a));
    if (b != TRAINER_NONE && b < TRAINERS_COUNT)
    {
        dst = StringCopy(dst, COMPOUND_STRING(" & "));
        StringCopy(dst, GetTrainerNameFromId(b));
    }
}

// --- Field kit (build 0xC017013E) ----------------------------------------
// Each save gets a POKéMON BOX LINK (the storage system from the field) and
// a HEALING VIAL from the first POKéMON CENTER nurse it talks to once it has
// a Pokemon. The vial holds 1 charge plus 1 for every two badges, heals one
// Pokemon's HP and status outside battle (no revive, no PP), and is
// refilled by a Center's heal. Deposits do not heal (OW_PC_HEAL), so the
// BOX LINK is not a free Center.

#define COOP_KIT_BOX_LINK 1
#define COOP_KIT_VIAL     2

// Test visibility: the charges after the last change or read.
EWRAM_DATA u8 gCoopVialCharges = 0;

u8 CoopKit_VialMax(void)
{
    u32 i, badges = 0;

    for (i = 0; i < 8; i++)
    {
        if (FlagGet(FLAG_BADGE01_GET + i))
            badges++;
    }
    return 1 + badges / 2;
}

u8 CoopKit_VialCharges(void)
{
    struct CoopSave *save = CoopSave();

    if (save == NULL)
        return 0;
    if (save->vialCharges > CoopKit_VialMax())
        save->vialCharges = CoopKit_VialMax();
    gCoopVialCharges = save->vialCharges;
    return save->vialCharges;
}

bool8 CoopKit_UseVialCharge(void)
{
    struct CoopSave *save = CoopSave();

    if (save == NULL || CoopKit_VialCharges() == 0)
        return FALSE;
    save->vialCharges--;
    gCoopVialCharges = save->vialCharges;
    return TRUE;
}

// After a Center's heal.
void CoopKit_RefillVial(void)
{
    struct CoopSave *save = CoopSave();

    if (save != NULL && CheckBagHasItem(ITEM_HEALING_VIAL, 1))
        save->vialCharges = CoopKit_VialMax();
    if (save != NULL)
        gCoopVialCharges = save->vialCharges;
}

u16 CoopKit_NeedsBoxLink(void)
{
    struct CoopSave *save = CoopSave();

    return save != NULL && FlagGet(FLAG_SYS_POKEMON_GET) && !(save->fieldKit & COOP_KIT_BOX_LINK)
        && !CheckBagHasItem(ITEM_POKEMON_BOX_LINK, 1);
}

u16 CoopKit_NeedsVial(void)
{
    struct CoopSave *save = CoopSave();

    return save != NULL && FlagGet(FLAG_SYS_POKEMON_GET) && !(save->fieldKit & COOP_KIT_VIAL)
        && !CheckBagHasItem(ITEM_HEALING_VIAL, 1);
}

void CoopKit_MarkBoxLink(void)
{
    struct CoopSave *save = CoopSave();

    if (save != NULL)
        save->fieldKit |= COOP_KIT_BOX_LINK;
}

void CoopKit_MarkVial(void)
{
    struct CoopSave *save = CoopSave();

    if (save != NULL)
    {
        save->fieldKit |= COOP_KIT_VIAL;
        save->vialCharges = CoopKit_VialMax();
        gCoopVialCharges = save->vialCharges;
    }
}

// NULL when the BOX LINK and the HEALING VIAL may be used here.
const u8 *CoopKit_FieldUseBlocked(void)
{
    if (Coop_PlaceStaysSolo() || gMapHeader.regionMapSectionId == MAPSEC_BATTLE_FRONTIER
     || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InBattlePike())
        return COMPOUND_STRING("It can't be used here.");
    if (CoopDeal_Busy() || CoopPrep_Active() || sWaitingAck || sGuestPairPending)
        return COMPOUND_STRING("Finish with your partner first.");
    return NULL;
}
