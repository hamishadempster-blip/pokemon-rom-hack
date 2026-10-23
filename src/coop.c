#include "global.h"
#include "coop.h"
#include "coop_protocol.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "caps.h"
#include "event_data.h"
#include "item.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "link.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "pokeball.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "region_map.h"
#include "script.h"
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
#include "data.h"

#define COOP_INVITE_MAGIC 0xC0A1
#define COOP_MON_MAGIC    0xC0B0
#define COOP_SAVE_MAGIC   0xC0
#define COOP_ACK_TIMEOUT  600
// A wild encounter has the screen frozen while it waits, and the guest
// answers it without a prompt. The pair prompt waits on a human, so the
// guest stops accepting a yes well before the host gives up.
#define COOP_WILD_TIMEOUT 180
#define COOP_PAIR_TIMEOUT 1800
// The rescue waits on a person who may still be closing a menu.
#define COOP_RESCUE_TIMEOUT 1800
#define COOP_PAIR_ANSWER_LIMIT 1500
// The CO-OP menu holds on the waiting text for this long. Both players
// need time to say yes. 30 seconds at 60 frames.
#define COOP_LINK_WAIT_LIMIT 1800
#define COOP_CASE_NONE    0xFF

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
};

struct CoopMonBlock
{
    u16 magic;
    u8 slot;
    u8 pad;
    struct Pokemon mon;
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

static EWRAM_DATA u8 sLinkState = COOP_LINK_IDLE;
static EWRAM_DATA u8 sTaskId = 0;
static EWRAM_DATA u8 sHandshakeSaved = FALSE;
static EWRAM_DATA u8 sLinkPhase = 0;
static EWRAM_DATA u16 sLinkPhaseTimer = 0;
static EWRAM_DATA bool8 sLinkWait = FALSE;
static EWRAM_DATA u16 sLinkWaitTimer = 0;
static EWRAM_DATA u8 sLinkPlayersSeen = 0;
static EWRAM_DATA u8 sLinkStatusCount = 0;
static EWRAM_DATA u8 sLinkErrorStreak = 0;
static EWRAM_DATA bool8 sBirchBag = FALSE;
static EWRAM_DATA u8 sRescueField = COOP_RESCUE_RETURN_NONE;
static EWRAM_DATA u8 sRescueResult = 0;
static EWRAM_DATA bool8 sRescueGuestFollow = FALSE;
static EWRAM_DATA u16 sRescueGuestTimer = 0;
static EWRAM_DATA u8 sTalkNpc = 0;
static EWRAM_DATA u8 sMerged = FALSE;
static EWRAM_DATA u8 sSeq = 0;
static EWRAM_DATA u8 sRequest = 0;
static EWRAM_DATA u8 sSlot = 0;
static EWRAM_DATA u8 sPartnerValid = FALSE;
static EWRAM_DATA struct CoopFrame sPartner = {0};
static EWRAM_DATA u16 sSpriteGfx = OBJ_EVENT_GFX_BRENDAN_NORMAL;
static EWRAM_DATA bool8 sIsGuest = FALSE;
static EWRAM_DATA bool8 sInBattle = FALSE;
static EWRAM_DATA bool8 sAborting = FALSE;
static EWRAM_DATA bool8 sHasSnapshot = FALSE;
static EWRAM_DATA bool8 sResumeScript = FALSE;
static EWRAM_DATA bool8 sWaitingAck = FALSE;
static EWRAM_DATA bool8 sInviteSent = FALSE;
static EWRAM_DATA u16 sInviteTimer = 0;
static EWRAM_DATA u16 sInviteSeq = 0;
static EWRAM_DATA u8 sBattleKind = 0;
static EWRAM_DATA u8 sBattleBadge = 0;
static EWRAM_DATA u16 sBattleFlag = 0;
static EWRAM_DATA u8 sLegendKind = 0;
static EWRAM_DATA u8 sRivalFightStage = 0;
static EWRAM_DATA u8 sProposedCase = 0;
static EWRAM_DATA u8 sClaim = 0;
static EWRAM_DATA u8 sWaitKind = 0;
static EWRAM_DATA u8 sTradeSlot = 0;
static EWRAM_DATA u8 sTradeSent = FALSE;
static EWRAM_DATA u8 sTradeGot = FALSE;
static EWRAM_DATA struct Pokemon sTradeMon = {0};
static EWRAM_DATA struct Pokemon sPartySnap[PARTY_SIZE] = {0};
static EWRAM_DATA struct CoopInvite sInvite = {0};
static EWRAM_DATA const u8 *sPendingMessage = NULL;
static EWRAM_DATA u8 sGuestFollowup = 0;
static EWRAM_DATA u8 sLegendConflict = 0;
static EWRAM_DATA struct CoopBeaconState sBeacon = {0};
static EWRAM_DATA u8 sBeaconQueued = 0;
static EWRAM_DATA u8 sBeaconPrompted = 0;
static EWRAM_DATA u8 sBeaconBagWarned = 0;
static EWRAM_DATA u8 sRefusal = COOP_REFUSE_NONE;

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
static EWRAM_DATA u16 sPairEngaged = 0;
static EWRAM_DATA bool8 sGuestPairPending = FALSE;
static EWRAM_DATA u16 sGuestPairTimer = 0;
static EWRAM_DATA struct CoopInvite sGuestPairInvite = {0};

static const u8 sText_BeaconNeedLink[] = _("Link from the CO-OP menu first.{PAUSE_UNTIL_PRESS}");
static const u8 sText_BeaconWaiting[] = _("Still waiting on your partner.{PAUSE_UNTIL_PRESS}");
static const u8 sText_BeaconQueued[] = _("The request waits until this\nbattle or trade ends.{PAUSE_UNTIL_PRESS}");
static const u8 sText_BeaconSent[] = _("Asking your partner.\nBoth of you have to agree.{PAUSE_UNTIL_PRESS}");

STATIC_ASSERT(sizeof(struct CoopMonBlock) <= BLOCK_BUFFER_SIZE, CoopTradeBlockFitsLinkBlock);
STATIC_ASSERT(COOP_BATTLE_SLOTS == MULTI_PARTY_SIZE, CoopFieldsTheStockMultiHalf);
STATIC_ASSERT(COOP_ROSTER_SLOTS == PARTY_SIZE, CoopSnapshotCoversWholeParty);
STATIC_ASSERT(COOP_PAIR_LEFT_ID == TRAINER_RICK, CoopPairLeftIsRick);
STATIC_ASSERT(COOP_PAIR_RIGHT_ID == TRAINER_ALLEN, CoopPairRightIsAllen);

static void Task_CoopSession(u8 taskId);
static void Coop_PollBeacon(void);
static void Coop_Drop(const u8 *message);
static void Coop_LaunchBattle(bool8 isGuest);
static void Coop_SendInvite(void);
static u16 Coop_InviteChecksum(const struct CoopInvite *invite);
static void Coop_FailWait(void);
static bool8 Coop_InteractionBusy(void);
static bool8 Coop_PairBeatenLocally(void);
static void Coop_GiveRescueStarter(u8 caseId, bool8 opener);
static bool8 Coop_PartnerInBirchLab(void);

static struct CoopSave *CoopSave(void)
{
    struct CoopSave *save;

    if (gSaveBlock3Ptr == NULL)
        return NULL;
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
        save->reserved = 0;
    }
    return save;
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
    return sLinkState == COOP_LINK_UP && gReceivedRemoteLinkPlayers;
}

u16 Coop_PartnerOnSameMap(void)
{
    if (!Coop_SessionActive() || !sPartnerValid)
        return FALSE;
    return CoopProtocol_SameMap(sPartner.mapGroup, sPartner.mapNum,
        gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
}

u16 Coop_PartnerOnSameTile(void)
{
    s16 x, y;

    if (!Coop_PartnerOnSameMap())
        return FALSE;
    PlayerGetDestCoords(&x, &y);
    return sPartner.x == (u8)x && sPartner.y == (u8)y;
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

    if (frame->surf)
        return female ? OBJ_EVENT_GFX_MAY_SURFING : OBJ_EVENT_GFX_BRENDAN_SURFING;
    if (frame->bike && frame->acro)
        return female ? OBJ_EVENT_GFX_MAY_ACRO_BIKE : OBJ_EVENT_GFX_BRENDAN_ACRO_BIKE;
    if (frame->bike)
        return female ? OBJ_EVENT_GFX_MAY_MACH_BIKE : OBJ_EVENT_GFX_BRENDAN_MACH_BIKE;
    return female ? OBJ_EVENT_GFX_MAY_NORMAL : OBJ_EVENT_GFX_BRENDAN_NORMAL;
}

static void Coop_RemoveSprite(void)
{
    RemoveObjectEventByLocalIdAndMap(OBJ_EVENT_ID_COOP_PARTNER, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
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
    u8 status;
    s16 dx, dy, adx, ady;
    enum Direction dir;
    struct ObjectEvent *obj;
    struct ObjectEventTemplate template;

    // Map loads and door fades reset the sprite list. Spawning during that
    // paints a character onto a blank screen and the next map wipe deletes it.
    if (gMain.callback2 != CB2_Overworld)
        return;
    if (!Coop_PartnerOnSameMap() || gMain.inBattle)
    {
        Coop_RemoveSprite();
        return;
    }

    gfx = Coop_PartnerGfx(&sPartner);
    objectId = GetObjectEventIdByLocalIdAndMap(OBJ_EVENT_ID_COOP_PARTNER, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    if (objectId >= OBJECT_EVENTS_COUNT || sSpriteGfx != gfx)
    {
        Coop_RemoveSprite();
        memset(&template, 0, sizeof(template));
        template.localId = OBJ_EVENT_ID_COOP_PARTNER;
        template.graphicsId = gfx;
        template.kind = OBJ_KIND_NORMAL;
        // Spawn adds MAP_OFFSET itself. The link frame already includes it.
        template.x = (s16)sPartner.x - MAP_OFFSET;
        template.y = (s16)sPartner.y - MAP_OFFSET;
        template.elevation = PlayerGetElevation();
        template.movementType = MOVEMENT_TYPE_FACE_DOWN;
        template.trainerType = TRAINER_TYPE_NONE;
        template.script = Coop_EventScript_TalkPartner;
        template.flagId = 0;
        sSpriteGfx = gfx;
        objectId = SpawnSpecialObjectEvent(&template);
        if (objectId < OBJECT_EVENTS_COUNT && sPartner.facing != DIR_NONE)
            ObjectEventTurn(&gObjectEvents[objectId], (enum Direction)sPartner.facing);
        return;
    }

    obj = &gObjectEvents[objectId];
    status = ObjectEventClearHeldMovementIfFinished(obj);
    if (status == 0)
        return;

    dx = (s16)sPartner.x - obj->currentCoords.x;
    dy = (s16)sPartner.y - obj->currentCoords.y;
    adx = dx < 0 ? -dx : dx;
    ady = dy < 0 ? -dy : dy;
    if (dx == 0 && dy == 0)
    {
        enum Direction face = (enum Direction)sPartner.facing;

        if (face == DIR_NONE)
            face = DIR_SOUTH;
        // Standing still while talking: bob in place. Otherwise just turn.
        if (sPartner.talkLocalId != 0 && sPartner.talkLocalId != 0xFF)
        {
            if (ObjectEventClearHeldMovementIfFinished(obj) != 0)
            {
                obj->singleMovementActive = FALSE;
                ObjectEventClearHeldMovementIfActive(obj);
                ObjectEventSetHeldMovement(obj, GetWalkInPlaceNormalMovementAction(face));
            }
        }
        else if (obj->facingDirection != face)
        {
            obj->singleMovementActive = FALSE;
            ObjectEventClearHeldMovementIfActive(obj);
            ObjectEventSetHeldMovement(obj, GetFaceDirectionMovementAction(face));
        }
        return;
    }

    // A door or a missed packet can jump more than a couple of tiles.
    // Anything that far is a place, not a step.
    if (adx > 2 || ady > 2)
    {
        MoveObjectEventToMapCoords(obj, sPartner.x, sPartner.y);
        if (sPartner.facing != DIR_NONE)
            ObjectEventTurn(obj, (enum Direction)sPartner.facing);
        return;
    }

    dir = Coop_StepDir(dx, dy);
    if (dir == DIR_NONE)
        return;
    obj->singleMovementActive = FALSE;
    ObjectEventClearHeldMovementIfActive(obj);
    ObjectEventSetHeldMovement(obj, (sPartner.bike || adx + ady > 1)
        ? GetPlayerRunMovementAction(dir)
        : GetWalkNormalMovementAction(dir));
}

static void Coop_ReleaseTalkNpc(void)
{
    u8 objectId;

    if (sTalkNpc == 0)
        return;
    objectId = GetObjectEventIdByLocalIdAndMap(sTalkNpc, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    if (objectId < OBJECT_EVENTS_COUNT)
        ObjectEventClearHeldMovementIfActive(&gObjectEvents[objectId]);
    sTalkNpc = 0;
}

// The NPC your partner is talking to turns toward them and does the
// little talk bob, instead of standing in whatever pose the map left them.
static void Coop_AnimateTalkNpc(void)
{
    u8 localId = sPartner.talkLocalId;
    u8 objectId;
    s16 dx, dy, adx, ady;
    enum Direction face;
    struct ObjectEvent *npc;

    if (gMain.callback2 != CB2_Overworld || !Coop_PartnerOnSameMap()
     || localId == 0 || localId == 0xFF || localId == OBJ_EVENT_ID_COOP_PARTNER)
    {
        Coop_ReleaseTalkNpc();
        return;
    }
    if (localId != sTalkNpc)
        Coop_ReleaseTalkNpc();

    objectId = GetObjectEventIdByLocalIdAndMap(localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    if (objectId >= OBJECT_EVENTS_COUNT)
        return;

    npc = &gObjectEvents[objectId];
    if (ObjectEventClearHeldMovementIfFinished(npc) == 0)
    {
        sTalkNpc = localId;
        return;
    }

    dx = (s16)sPartner.x - npc->currentCoords.x;
    dy = (s16)sPartner.y - npc->currentCoords.y;
    adx = dx < 0 ? -dx : dx;
    ady = dy < 0 ? -dy : dy;
    if (dx == 0 && dy == 0)
        face = (enum Direction)sPartner.facing;
    else if (adx > ady)
        face = dx > 0 ? DIR_EAST : DIR_WEST;
    else
        face = dy > 0 ? DIR_SOUTH : DIR_NORTH;
    if (face == DIR_NONE)
        face = DIR_SOUTH;

    npc->singleMovementActive = FALSE;
    ObjectEventClearHeldMovementIfActive(npc);
    ObjectEventSetHeldMovement(npc, GetWalkInPlaceNormalMovementAction(face));
    sTalkNpc = localId;
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
    else if (ScriptContext_IsEnabled()
          && gSpecialVar_LastTalked != 0
          && gSpecialVar_LastTalked != OBJ_EVENT_ID_COOP_PARTNER)
        frame->talkLocalId = gSpecialVar_LastTalked;
    if (save != NULL)
    {
        frame->badges = save->jointBadges;
        frame->rivalStage = save->rivalStage;
        for (i = 0; i < 3; i++)
            frame->legend[i] = save->legendaryOwner[i];
    }
}

void Coop_LinkCallback(void)
{
    struct CoopFrame frame;
    u16 words[8];

    if (!gReceivedRemoteLinkPlayers || gLinkTransferringData || gMain.inBattle)
        return;
    Coop_FillFrame(&frame);
    CoopProtocol_Pack(words, &frame, COOP_SEND_AS_HELD_KEYS);
    memcpy(gSendCmd, words, sizeof(words));
}

static void Coop_ApplyProgress(const struct CoopProgress *progress)
{
    struct CoopSave *save = CoopSave();
    u8 i;

    if (save == NULL)
        return;
    save->jointBadges |= progress->badges;
    if (progress->rivalStage > save->rivalStage)
        save->rivalStage = progress->rivalStage;
    for (i = 0; i < 8; i++)
    {
        if (save->jointBadges & (1 << i))
            FlagSet(FLAG_BADGE01_GET + i);
    }
    for (i = 0; i < 3; i++)
    {
        if (progress->legend[i] != COOP_OWNER_NONE)
            save->legendaryOwner[i] = progress->legend[i];
    }
}

static const u16 sLegendSpecies[] = { SPECIES_GROUDON, SPECIES_KYOGRE, SPECIES_RAYQUAZA };

static void Coop_AcceptPartner(const struct CoopFrame *frame)
{
    struct CoopSave *save;
    struct CoopProgress local, remote, merged;
    int merge;
    u8 i;

    sPartner = *frame;
    sPartnerValid = TRUE;
    if (sMerged)
        return;

    save = CoopSave();
    if (save == NULL)
        return;
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
        remote.legend[i] = frame->legend[i];
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
    Coop_ApplyProgress(&merged);
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
    Coop_AcceptPartner(&frame);
}

void Coop_OnRecv(u32 playerId)
{
    Coop_HandleFrame(playerId, gRecvCmds[playerId]);
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
    memcpy(probe, words, sizeof(probe));
    probe[0] = COOP_LINKCMD;
    if (CoopProtocol_Checksum(probe) != words[7])
        return FALSE;
    Coop_HandleFrame(playerId, words);
    return TRUE;
}

static u16 Coop_InviteChecksum(const struct CoopInvite *invite)
{
    return (u16)(invite->magic ^ invite->kind ^ ((u16)invite->badge << 8) ^ invite->trainerA ^ invite->trainerB ^ invite->aux ^ invite->seq ^ COOP_INVITE_MAGIC);
}

static void Coop_SendInvite(void)
{
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
    sRefusal = COOP_REFUSE_NONE;
    sWaitingAck = TRUE;
    sInviteSent = FALSE;
    sInviteTimer = 0;
    sBattleKind = kind;
    sBattleBadge = badge;
    sBattleFlag = aux;
    TRAINER_BATTLE_PARAM.opponentA = trainerA;
    TRAINER_BATTLE_PARAM.opponentB = trainerB;
}

static void Coop_LaunchBattle(bool8 isGuest)
{
    sWaitingAck = FALSE;
    sIsGuest = isGuest;
    sInBattle = TRUE;
    gCoopBattleLaunching = TRUE;
    if (sBattleKind == COOP_KIND_LEGEND)
    {
        gCoopArmWild = TRUE;
        if (sLegendKind == COOP_LEGEND_RAY)
            CreateScriptedWildMon(SPECIES_RAYQUAZA, 70, ITEM_NONE);
        else
            CreateScriptedDoubleWildMon(SPECIES_GROUDON, 70, ITEM_NONE, SPECIES_KYOGRE, 70, ITEM_NONE);
        BattleSetup_StartLegendaryBattle();
        return;
    }
    if (sBattleKind == COOP_KIND_WILD || sBattleKind == COOP_KIND_RESCUE)
    {
        if (sBattleKind == COOP_KIND_RESCUE)
        {
            sBirchBag = FALSE;
            sResumeScript = FALSE;
            CreateScriptedDoubleWildMon(SPECIES_POOCHYENA, 2, ITEM_NONE, SPECIES_POOCHYENA, 2, ITEM_NONE);
        }
        BattleSetup_StartCoopWildBattle();
        return;
    }
    if (isGuest)
    {
        LockPlayerFieldControls();
        FreezeObjectEvents();
        StopPlayerAvatar();
    }
    BattleSetup_StartTrainerBattle();
}

static bool8 Coop_IsOptionalKind(u8 kind)
{
    return kind == COOP_KIND_TRAINER || kind == COOP_KIND_WILD || kind == COOP_KIND_PAIR;
}

// An optional invite never pulls a player out of a menu, a script, a trade,
// or a prompt that is already on screen.
static bool8 Coop_GuestBusy(void)
{
    return ScriptContext_IsEnabled()
        || ArePlayerFieldControlsLocked()
        || Coop_InteractionBusy()
        || gMain.callback2 != CB2_Overworld
        || sPendingMessage != NULL;
}

static u16 Coop_InviteTimeout(u8 kind)
{
    if (kind == COOP_KIND_WILD)
        return COOP_WILD_TIMEOUT;
    if (kind == COOP_KIND_RESCUE)
        return COOP_RESCUE_TIMEOUT;
    if (kind == COOP_KIND_PAIR)
        return COOP_PAIR_TIMEOUT;
    return COOP_ACK_TIMEOUT;
}

static void Coop_SendReply(u8 kind, u16 seq)
{
    struct CoopInvite reply;

    memset(&reply, 0, sizeof(reply));
    reply.magic = COOP_INVITE_MAGIC;
    reply.kind = kind;
    reply.seq = seq;
    reply.checksum = Coop_InviteChecksum(&reply);
    if (!gLinkTransferringData)
        SendBlock(BitmaskAllOtherLinkPlayers(), &reply, sizeof(reply));
}

static void Coop_HandleInvite(u32 playerId, const struct CoopInvite *invite)
{
    if (playerId == GetMultiplayerId())
        return;
    if (invite->magic != COOP_INVITE_MAGIC || invite->checksum != Coop_InviteChecksum(invite))
        return;

    if (invite->kind == COOP_KIND_ACK)
    {
        if (sWaitingAck && invite->seq == sInvite.seq)
            Coop_LaunchBattle(FALSE);
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
            sRefusal = COOP_REFUSE_PARTNER_NOT_READY;
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

    if (invite->kind == COOP_KIND_RESCUE)
    {
        if (Coop_GuestBusy() || invite->badge > 2)
        {
            Coop_SendReply(COOP_KIND_NACK, invite->seq);
            return;
        }
        Coop_GiveRescueStarter(invite->badge, FALSE);
        CreateScriptedDoubleWildMon(SPECIES_POOCHYENA, 2, ITEM_NONE, SPECIES_POOCHYENA, 2, ITEM_NONE);
        sBattleKind = COOP_KIND_RESCUE;
        sBattleBadge = 0;
        sBattleFlag = 0;
        sLegendKind = 0;
        sRivalFightStage = 0xFF;
        TRAINER_BATTLE_PARAM.opponentA = 0;
        TRAINER_BATTLE_PARAM.opponentB = 0;
        Coop_SendReply(COOP_KIND_ACK, invite->seq);
        Coop_LaunchBattle(TRUE);
        return;
    }

    if (Coop_IsOptionalKind(invite->kind))
    {
        if (Coop_GuestBusy() || !Coop_HasBattleReadyLead()
         || ((invite->kind == COOP_KIND_WILD || invite->kind == COOP_KIND_PAIR) && (!sBeacon.on || !Coop_PartnerOnSameMap()))
         || (invite->kind == COOP_KIND_PAIR && Coop_PairBeatenLocally())
         || (invite->kind == COOP_KIND_WILD && (invite->trainerA == SPECIES_NONE || invite->trainerA >= NUM_SPECIES
                                             || invite->trainerB == SPECIES_NONE || invite->trainerB >= NUM_SPECIES)))
        {
            Coop_SendReply(COOP_KIND_NACK, invite->seq);
            return;
        }
        if (invite->kind == COOP_KIND_PAIR)
        {
            sGuestPairInvite = *invite;
            sGuestPairPending = TRUE;
            sGuestPairTimer = 0;
            ScriptContext_SetupScript(Coop_EventScript_PairAskGuest);
            return;
        }
        if (invite->kind == COOP_KIND_WILD)
        {
            sBattleKind = COOP_KIND_WILD;
            sBattleBadge = 0;
            sBattleFlag = 0;
            sLegendKind = 0;
            sRivalFightStage = 0xFF;
            TRAINER_BATTLE_PARAM.opponentA = 0;
            TRAINER_BATTLE_PARAM.opponentB = 0;
            CreateScriptedDoubleWildMon(invite->trainerA, invite->badge, ITEM_NONE, invite->trainerB, (u8)invite->aux, ITEM_NONE);
            Coop_SendReply(COOP_KIND_ACK, invite->seq);
            Coop_LaunchBattle(TRUE);
            return;
        }
    }

    if (!Coop_HasBattleReadyLead())
    {
        Coop_SendReply(COOP_KIND_NACK, invite->seq);
        sPendingMessage = Coop_EventScript_NotReady;
        return;
    }

    sBattleKind = invite->kind;
    sBattleBadge = invite->badge;
    sBattleFlag = invite->aux;
    sLegendKind = invite->kind == COOP_KIND_LEGEND ? (u8)invite->aux : 0;
    sRivalFightStage = invite->kind == COOP_KIND_RIVAL ? (u8)invite->badge : 0xFF;
    TRAINER_BATTLE_PARAM.opponentA = invite->trainerA;
    TRAINER_BATTLE_PARAM.opponentB = invite->trainerB;

    Coop_SendReply(COOP_KIND_ACK, invite->seq);
    Coop_LaunchBattle(TRUE);
}

static void Coop_HandleMonBlock(u32 playerId, const struct CoopMonBlock *block)
{
    if (playerId == GetMultiplayerId() || block->magic != COOP_MON_MAGIC)
        return;
    sTradeMon = block->mon;
    sTradeGot = TRUE;
}

static void Coop_PollBlocks(void)
{
    u8 status = GetBlockReceivedStatus();
    u32 i;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (status & (1 << i))
        {
            u16 magic = gBlockRecvBuffer[i][0];
            if (magic == COOP_INVITE_MAGIC)
                Coop_HandleInvite(i, (const struct CoopInvite *)gBlockRecvBuffer[i]);
            else if (magic == COOP_MON_MAGIC)
                Coop_HandleMonBlock(i, (const struct CoopMonBlock *)gBlockRecvBuffer[i]);
            ResetBlockReceivedFlag(i);
        }
    }
}

static void Coop_FailWait(void)
{
    u8 kind = sBattleKind;

    sWaitingAck = FALSE;
    sWaitKind = 0;
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
    if (!sWaitingAck)
        return;
    // B leaves the rescue wait so a partner stuck in a menu does not
    // freeze the person who opened the bag.
    if (sBattleKind == COOP_KIND_RESCUE && JOY_NEW(B_BUTTON))
    {
        Coop_FailWait();
        return;
    }
    Coop_SendInvite();
    if (++sInviteTimer >= Coop_InviteTimeout(sBattleKind))
        Coop_FailWait();
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
        static const u16 species[3][2] = {
            { SPECIES_TREECKO, SPECIES_SHROOMISH },
            { SPECIES_TORCHIC, SPECIES_HOUNDOUR },
            { SPECIES_MUDKIP, SPECIES_WINGULL },
        };
        u8 mine = sClaim;
        u8 theirs = sPartner.slot;
        u8 index = mine;
        u8 caseId = save != NULL ? save->caseId : 0;

        if (mine == theirs)
            index = GetMultiplayerId() == 0 ? mine : (u8)(mine ^ 1);
        if (caseId > 2)
            caseId = sProposedCase <= 2 ? sProposedCase : 0;
        ZeroPlayerPartyMons();
        ScriptGiveMon(species[caseId][index & 1], 5, ITEM_NONE);
        VarSet(VAR_STARTER_MON, caseId);
        if (save != NULL)
            save->caseId = caseId;
        sRequest = COOP_REQ_NONE;
        sWaitKind = 0;
        gSpecialVar_Result = TRUE;
        ScriptContext_Enable();
    }
}

static void Coop_PollTrade(void)
{
    struct CoopMonBlock block;

    if (sTradeSlot == 0xFF || !Coop_SessionActive())
    {
        sTradeSent = FALSE;
        sTradeGot = FALSE;
        return;
    }
    if (!sPartnerValid || sPartner.request != COOP_REQ_TRADE_YES || sRequest != COOP_REQ_TRADE_YES)
        return;
    if (!sTradeSent && !gLinkTransferringData)
    {
        memset(&block, 0, sizeof(block));
        block.magic = COOP_MON_MAGIC;
        block.slot = sTradeSlot;
        block.mon = gParties[B_TRAINER_PLAYER][sTradeSlot];
        if (SendBlock(BitmaskAllOtherLinkPlayers(), &block, sizeof(block)))
            sTradeSent = TRUE;
    }
    if (sTradeSent && sTradeGot && GetMonData(&sTradeMon, MON_DATA_SPECIES) != SPECIES_NONE)
    {
        gParties[B_TRAINER_PLAYER][sTradeSlot] = sTradeMon;
        sTradeSlot = 0xFF;
        sTradeSent = FALSE;
        sTradeGot = FALSE;
        sRequest = COOP_REQ_NONE;
        sPendingMessage = Coop_EventScript_TradeDone;
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

static void Coop_ForceScreenOn(void)
{
    gPaletteFade.active = FALSE;
    gPaletteFade.y = 0;
    gPaletteFade.targetY = 0;
    gPaletteFade.yDec = FALSE;
    UnfadePlttBuffer(PALETTES_ALL);
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
            if (!sHandshakeSaved)
            {
                SaveLinkPlayers(2);
                sHandshakeSaved = TRUE;
            }
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
            sLinkState = COOP_LINK_UP;
            gCoopHoldLink = TRUE;
            sLinkErrorStreak = 0;
            sTradeSlot = 0xFF;
            sRivalFightStage = 0xFF;
            sProposedCase = COOP_CASE_NONE;
            sClaim = COOP_CASE_NONE;
            gLinkCallback = Coop_LinkCallback;
            SetSuppressLinkErrorMessage(TRUE);
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
    if (gMain.inBattle && (gBattleTypeFlags & BATTLE_TYPE_COOP))
        return;

    // Checksums and a full send queue show up whenever a message box, a
    // door, or Birch's bag copies a lot of tiles. That is not the cable
    // coming out. Forget those blips. The session ends only when the other
    // game is actually gone.
    Link_ConsumeErrors();
    gCoopHoldLink = TRUE;
    SetSuppressLinkErrorMessage(TRUE);

    if (ScriptContext_IsEnabled() || sBirchBag || gMain.inBattle || gMain.callback2 != CB2_Overworld)
    {
        sLinkErrorStreak = 0;
        return;
    }
    if (!gReceivedRemoteLinkPlayers || EXTRACT_RECEIVED_NOTHING(gLinkStatus))
    {
        if (++sLinkErrorStreak >= 180)
            Coop_Drop(Coop_EventScript_Disconnected);
    }
    else
    {
        sLinkErrorStreak = 0;
    }
}

static void Coop_Drop(const u8 *message)
{
    // A pending invite would otherwise leave the field frozen or the
    // trainer script waiting on an answer that can't arrive.
    gCoopHoldLink = FALSE;
    sBirchBag = FALSE;
    sRescueField = COOP_RESCUE_RETURN_NONE;
    if (sRescueGuestFollow)
        message = Coop_EventScript_WarpToLab;
    sRescueGuestFollow = FALSE;
    sLinkErrorStreak = 0;
    if (sWaitingAck && !gMain.inBattle)
        Coop_FailWait();
    sGuestPairPending = FALSE;
    sLinkState = COOP_LINK_IDLE;
    sLinkPhase = 0;
    sLinkPhaseTimer = 0;
    sHandshakeSaved = FALSE;
    sMerged = FALSE;
    sPartnerValid = FALSE;
    sWaitingAck = FALSE;
    sResumeScript = FALSE;
    sRequest = COOP_REQ_NONE;
    sTradeSlot = 0xFF;
    memset(&sBeacon, 0, sizeof(sBeacon));
    sBeaconQueued = 0;
    sBeaconPrompted = 0;
    gLinkCallback = NULL;
    Coop_RemoveSprite();
    CloseLink();
    SetSuppressLinkErrorMessage(TRUE);
    if (sPendingMessage != Coop_EventScript_WarpToLab)
        sPendingMessage = message;
}

static void Task_CoopSession(u8 taskId)
{
    if (sLinkState == COOP_LINK_CONNECTING)
        Coop_PollHandshake();
    Coop_PollLinkWait();
    if (sLinkState == COOP_LINK_UP)
    {
        SetSuppressLinkErrorMessage(TRUE);
        Coop_MaintainCallback();
        Coop_UpdateSprite();
        Coop_AnimateTalkNpc();
        Coop_PollBlocks();
        Coop_PollScriptWaits();
        Coop_PollTrade();
        Coop_PollBeacon();
        Coop_PollInviteTimeout();
        if (sGuestPairPending && sGuestPairTimer < 0xFFFF)
            sGuestPairTimer++;
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
    if (sPendingMessage != NULL && !gMain.inBattle && !ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked())
    {
        const u8 *message = sPendingMessage;
        sPendingMessage = NULL;
        ScriptContext_SetupScript(message);
    }
    (void)taskId;
}

static void Coop_EnsureTask(void)
{
    if (sTaskId == TASK_NONE || !FuncIsActiveTask(Task_CoopSession))
        sTaskId = CreateTask(Task_CoopSession, 80);
}

// Door transitions call ResetTasks, which deletes the session task.
// The link itself is still open. Put the task back before the fade ends.
void Coop_ResumeAfterTasksReset(void)
{
    if (sLinkState != COOP_LINK_UP)
        return;
    gCoopHoldLink = TRUE;
    SetSuppressLinkErrorMessage(TRUE);
    if (gLinkCallback == NULL && gReceivedRemoteLinkPlayers)
        gLinkCallback = Coop_LinkCallback;
    sTaskId = TASK_NONE;
    Coop_EnsureTask();
}

void Coop_OverworldFrame(void)
{
    if (sLinkState != COOP_LINK_IDLE || sPendingMessage != NULL)
        Coop_EnsureTask();
}

u16 Coop_BeginLink(void)
{
    if (sLinkState == COOP_LINK_UP)
        return TRUE;
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
    memset(&sBeacon, 0, sizeof(sBeacon));
    sBeaconQueued = 0;
    sBeaconPrompted = 0;
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

static bool8 Coop_InteractionBusy(void)
{
    return sInBattle || gMain.inBattle || sTradeSlot != 0xFF || sTradeSent || sWaitingAck || sWaitKind != 0 || sGuestPairPending;
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
    if (!sPartnerValid)
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

static bool8 Coop_ClassRequiresPartner(u16 trainerId)
{
    enum TrainerClassID cls;

    if (trainerId == TRAINER_NONE || trainerId >= TRAINERS_COUNT || IsSpecialTrainer(trainerId))
        return FALSE;
    cls = GetTrainerClassFromId(trainerId);
    return cls == TRAINER_CLASS_LEADER
        || cls == TRAINER_CLASS_ELITE_FOUR
        || cls == TRAINER_CLASS_CHAMPION
        || cls == TRAINER_CLASS_RIVAL;
}

bool8 Coop_ShouldRefuseSolo(u16 trainerId)
{
    if (Coop_PartnerOnSameMap())
        return FALSE;
    return Coop_ClassRequiresPartner(trainerId) || Coop_RivalStageFromId(trainerId) != 0xFF;
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
    enum TrainerClassID cls;
    bool8 grunt;
    bool8 mandatory;
    u16 trainerB;

    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap() || !sBeacon.on)
        return FALSE;
    // Ordinary fights are optional, so a fainted front three just means this one stays solo.
    if (!Coop_HasBattleReadyLead())
        return FALSE;
    if (trainerA == TRAINER_NONE || trainerA >= TRAINERS_COUNT || IsSpecialTrainer(trainerA))
        return FALSE;
    if (Coop_RivalStageFromId(trainerA) != 0xFF)
        return FALSE;
    // The Route 102 pair has its own ask, and stays solo when that is refused.
    if (trainerA == COOP_PAIR_LEFT_ID || trainerA == COOP_PAIR_RIGHT_ID)
        return FALSE;
    cls = GetTrainerClassFromId(trainerA);
    grunt = cls == TRAINER_CLASS_TEAM_AQUA || cls == TRAINER_CLASS_TEAM_MAGMA;
    mandatory = Coop_ClassRequiresPartner(trainerA);
    if (!grunt && !mandatory && (trainerA % 4) == 0)
        return FALSE;
    trainerB = grunt ? trainerA : 0;
    sResumeScript = FALSE;
    Coop_QueueInvite(COOP_KIND_TRAINER, trainerA, trainerB, 0, 0);
    return TRUE;
}

static bool8 Coop_RefuseMandatory(void)
{
    if (!Coop_PartnerOnSameMap())
        sRefusal = COOP_REFUSE_NONE;
    else if (!Coop_HasBattleReadyLead())
        sRefusal = COOP_REFUSE_SELF_NOT_READY;
    else
        return FALSE;
    gSpecialVar_Result = FALSE;
    CreateTask(Task_CoopEnableScript, 0);
    return TRUE;
}

u16 Coop_StartGymBattle(void)
{
    if (Coop_RefuseMandatory())
        return FALSE;
    sResumeScript = TRUE;
    sLegendKind = 0;
    Coop_QueueInvite((u8)VarGet(VAR_0x8008), VarGet(VAR_0x8004), VarGet(VAR_0x8005), (u8)VarGet(VAR_0x8006), VarGet(VAR_0x8007));
    gSpecialVar_Result = FALSE;
    return TRUE;
}

u16 Coop_StartLegendaryBattle(void)
{
    if (Coop_RefuseMandatory())
        return FALSE;
    sResumeScript = TRUE;
    sLegendKind = VarGet(VAR_0x8004) == COOP_LEGEND_RAY ? COOP_LEGEND_RAY : COOP_LEGEND_DUO;
    Coop_QueueInvite(COOP_KIND_LEGEND, 0, 0, 0, sLegendKind);
    gSpecialVar_Result = FALSE;
    return TRUE;
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

static const u16 sRescueSpecies[3][2] = {
    { SPECIES_TREECKO, SPECIES_SHROOMISH },
    { SPECIES_TORCHIC, SPECIES_HOUNDOUR },
    { SPECIES_MUDKIP, SPECIES_WINGULL },
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
    ScriptGiveMon(sRescueSpecies[caseId][index], 5, ITEM_NONE);
}

u16 Coop_StartRescueBattle(void)
{
    struct CoopSave *save = CoopSave();
    u16 caseId = VarGet(VAR_STARTER_MON);

    sBirchBag = TRUE;
    if (GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES) != SPECIES_NONE
     && save != NULL && save->caseId <= 2)
        caseId = save->caseId;
    if (caseId > 2)
        caseId = 0;
    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap())
    {
        sBirchBag = FALSE;
        sRescueResult = 0;
        gSpecialVar_Result = FALSE;
        return FALSE;
    }
    Coop_GiveRescueStarter((u8)caseId, TRUE);
    CreateScriptedDoubleWildMon(SPECIES_POOCHYENA, 2, ITEM_NONE, SPECIES_POOCHYENA, 2, ITEM_NONE);
    sRescueResult = 1;
    sRescueField = COOP_RESCUE_RETURN_NONE;
    sResumeScript = TRUE;
    Coop_QueueInvite(COOP_KIND_RESCUE, 0, 0, (u8)caseId, 0);
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

bool8 Coop_OfferTrade(u8 slot)
{
    u8 i, usable = 0;
    struct Pokemon *mon;

    if (!Coop_SessionActive() || slot >= PARTY_SIZE)
        return FALSE;
    mon = &gParties[B_TRAINER_PLAYER][slot];
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) != SPECIES_NONE
         && !GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG)
         && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP) != 0)
            usable++;
    }
    if (usable <= 1 && GetMonData(mon, MON_DATA_HP) != 0)
        return FALSE;
    sTradeSlot = slot;
    sTradeSent = FALSE;
    sTradeGot = FALSE;
    sRequest = COOP_REQ_TRADE_YES;
    sSlot = slot;
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
    CoopProtocol_ClearCatches(&sLedger);
    memset(sCatches, 0, sizeof(sCatches));
    memset(gCoopBallTargetFor, 0, sizeof(gCoopBallTargetFor));
    sCatchCount = 0;
    sLocalLegendMask = 0;
    sPartnerLegendMask = 0;
    gCoopBallTarget = 0;
    gCoopLastBall = ITEM_NONE;
}

void Coop_OnBattleLaunched(void)
{
    u32 i;

    Coop_ClearCatchState();
    memcpy(sPartySnap, gParties[B_TRAINER_PLAYER], sizeof(sPartySnap));
    for (i = COOP_BATTLE_SLOTS; i < PARTY_SIZE; i++)
        ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
    CalculatePlayerPartyCount();
    sHasSnapshot = TRUE;
    sAborting = FALSE;
    sInBattle = TRUE;
    gCoopBattleLaunching = FALSE;
}

static void Coop_RestoreParty(bool8 aborted)
{
    struct Pokemon arrivals[PARTY_SIZE - COOP_BATTLE_SLOTS];
    u32 i, count = 0;

    if (!sHasSnapshot)
        return;
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

static void Coop_GiveJointWin(void)
{
    struct CoopSave *save = CoopSave();

    if (sBattleBadge >= 1 && sBattleBadge <= 8)
    {
        FlagSet(FLAG_BADGE01_GET + sBattleBadge - 1);
        if (save != NULL)
            save->jointBadges |= (u8)(1 << (sBattleBadge - 1));
    }
    if (sBattleFlag != 0)
        FlagSet(sBattleFlag);
    if (sBattleKind == COOP_KIND_CHAMPION)
        FlagSet(FLAG_IS_CHAMPION);
    if (sBattleKind == COOP_KIND_RIVAL && save != NULL && sRivalFightStage != 0xFF && save->rivalStage <= sRivalFightStage)
        save->rivalStage = sRivalFightStage + 1;
}

static void Coop_QueueGuestFollowup(void)
{
    if (sBattleKind == COOP_KIND_E4)
    {
        // Stop the room's walk-in from playing again before the door opens.
        VarSet(VAR_ELITE_4_STATE, 1);
        sGuestFollowup = 9;
    }
    else if (sBattleKind == COOP_KIND_CHAMPION)
    {
        VarSet(VAR_TEMP_1, 1);
        sGuestFollowup = 10;
    }
    else if (sBattleKind == COOP_KIND_GYM && sBattleBadge >= 1 && sBattleBadge <= 8)
    {
        sGuestFollowup = sBattleBadge;
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

bool8 Coop_FinishTrainerBattle(void)
{
    bool8 won = gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT;
    bool8 firstClear = sBattleFlag == 0 || !FlagGet(sBattleFlag);

    Coop_RestoreParty(FALSE);
    sInBattle = FALSE;
    if (won)
        Coop_GiveJointWin();
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

static u8 Coop_LegendIndex(u16 species)
{
    if (species == SPECIES_GROUDON)
        return 0;
    if (species == SPECIES_KYOGRE)
        return 1;
    if (species == SPECIES_RAYQUAZA)
        return 2;
    return 0xFF;
}

static void Coop_ApplyLegendOutcome(bool8 won)
{
    static const u16 defeatedFlags[3] = { FLAG_DEFEATED_GROUDON, FLAG_DEFEATED_KYOGRE, FLAG_DEFEATED_RAYQUAZA };
    struct CoopSave *save = CoopSave();
    u8 caught = sLocalLegendMask | sPartnerLegendMask;
    u32 i;

    if (won)
    {
        if (sLegendKind == COOP_LEGEND_RAY)
            FlagSet(FLAG_DEFEATED_RAYQUAZA);
        else
        {
            FlagSet(FLAG_DEFEATED_GROUDON);
            FlagSet(FLAG_DEFEATED_KYOGRE);
        }
    }
    for (i = 0; i < 3; i++)
    {
        if (!(caught & (1 << i)))
            continue;
        FlagSet(defeatedFlags[i]);
        if (save != NULL)
            save->legendaryOwner[i] = (sLocalLegendMask & (1 << i)) ? COOP_OWNER_LOCAL : COOP_OWNER_PARTNER;
    }
    if (save != NULL)
        save->pendingLegendary = 0;
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
    if (sNickCount != 0)
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
    Coop_ApplyLegendOutcome(won);
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

bool8 Coop_WildPairReady(void)
{
    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(MAP_ROUTE102)
     || gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_ROUTE102))
        return FALSE;
    if (!Coop_SessionActive() || !sBeacon.on || !Coop_PartnerOnSameMap())
        return FALSE;
    if (!Coop_HasBattleReadyLead() || Coop_InteractionBusy() || sPendingMessage != NULL)
        return FALSE;
    return TRUE;
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
    u8 index = Coop_LegendIndex(GetMonData(mon, MON_DATA_SPECIES));

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
    else if (index != 0xFF)
    {
        sPartnerLegendMask |= 1 << index;
    }
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

    if (!sGuestPairPending)
        return 2;
    sGuestPairPending = FALSE;
    if (sGuestPairTimer >= COOP_PAIR_ANSWER_LIMIT || !Coop_SessionActive())
    {
        Coop_SendReply(COOP_KIND_NACK, seq);
        return 2;
    }
    if (!Coop_HasBattleReadyLead() || Coop_PairPlanFromHere(TRUE, accepted) != COOP_PAIR_JOINT)
    {
        Coop_SendReply(COOP_KIND_NACK, seq);
        return 0;
    }
    sBattleKind = COOP_KIND_PAIR;
    sBattleBadge = 0;
    sBattleFlag = 0;
    sLegendKind = 0;
    sRivalFightStage = 0xFF;
    TRAINER_BATTLE_PARAM.opponentA = sGuestPairInvite.trainerA;
    TRAINER_BATTLE_PARAM.opponentB = sGuestPairInvite.trainerB;
    Coop_SendReply(COOP_KIND_ACK, seq);
    Coop_LaunchBattle(TRUE);
    return 1;
}

// Called from BattleSetup when the rival script has already started a battle.
bool8 Coop_InterceptRival(u16 trainerId)
{
    u8 stage = Coop_RivalStageFromId(trainerId);

    if (stage == 0xFF || gCoopBattleLaunching)
        return FALSE;
    if (!Coop_PartnerOnSameMap())
    {
        ScriptContext_SetupScript(Coop_EventScript_NeedPartner);
        return TRUE;
    }
    if (!Coop_HasBattleReadyLead())
    {
        ScriptContext_SetupScript(Coop_EventScript_NotReady);
        return TRUE;
    }
    Coop_QueueRival(stage);
    ScriptContext_Stop();
    return TRUE;
}
