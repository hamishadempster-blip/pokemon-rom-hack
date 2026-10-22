#include "global.h"
#include "coop.h"
#include "coop_protocol.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "caps.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "link.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "region_map.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "task.h"
#include "constants/battle.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/global.h"
#include "constants/opponents.h"
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

static EWRAM_DATA u8 sLinkState = COOP_LINK_IDLE;
static EWRAM_DATA u8 sTaskId = 0;
static EWRAM_DATA u8 sHandshakeSaved = FALSE;
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
static EWRAM_DATA u8 sLegendConflict = 0;

STATIC_ASSERT(sizeof(struct Pokemon) * 2 <= BLOCK_BUFFER_SIZE, CoopPartyChunkFitsLinkBlock);
STATIC_ASSERT(sizeof(struct CoopMonBlock) <= BLOCK_BUFFER_SIZE, CoopTradeBlockFitsLinkBlock);
STATIC_ASSERT(COOP_PARTY_SIZE <= PARTY_SIZE, CoopPartySizeFitsSave);

static void Task_CoopSession(u8 taskId);
static void Coop_Drop(const u8 *message);
static void Coop_LaunchBattle(bool8 isGuest);
static void Coop_SendInvite(void);
static u16 Coop_InviteChecksum(const struct CoopInvite *invite);

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
    return sPartner.mapGroup == gSaveBlock1Ptr->location.mapGroup
        && sPartner.mapNum == gSaveBlock1Ptr->location.mapNum;
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

static void Coop_UpdateSprite(void)
{
    u16 gfx;
    u8 objectId;
    struct ObjectEventTemplate template;

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
        template.x = sPartner.x;
        template.y = sPartner.y;
        template.elevation = PlayerGetElevation();
        template.movementType = MOVEMENT_TYPE_FACE_DOWN;
        template.trainerType = TRAINER_TYPE_NONE;
        template.script = Coop_EventScript_TalkPartner;
        template.flagId = 0;
        sSpriteGfx = gfx;
        SpawnSpecialObjectEvent(&template);
        return;
    }

    MoveObjectEventToMapCoords(&gObjectEvents[objectId], sPartner.x, sPartner.y);
    ObjectEventTurn(&gObjectEvents[objectId], (enum Direction)sPartner.facing);
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
    BattleSetup_StartTrainerBattle();
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

    if (invite->kind == COOP_KIND_WARP)
    {
        struct CoopInvite ack;

        if (!ScriptContext_IsEnabled())
            sPendingMessage = Coop_EventScript_WarpToLab;
        memset(&ack, 0, sizeof(ack));
        ack.magic = COOP_INVITE_MAGIC;
        ack.kind = COOP_KIND_ACK;
        ack.seq = invite->seq;
        ack.checksum = Coop_InviteChecksum(&ack);
        if (!gLinkTransferringData)
            SendBlock(BitmaskAllOtherLinkPlayers(), &ack, sizeof(ack));
        return;
    }

    if (sInBattle || gMain.inBattle)
        return;

    sBattleKind = invite->kind;
    sBattleBadge = invite->badge;
    sBattleFlag = invite->aux;
    sLegendKind = invite->kind == COOP_KIND_LEGEND ? (u8)invite->aux : 0;
    sRivalFightStage = invite->kind == COOP_KIND_RIVAL ? (u8)invite->badge : 0xFF;
    TRAINER_BATTLE_PARAM.opponentA = invite->trainerA;
    TRAINER_BATTLE_PARAM.opponentB = invite->trainerB;

    {
        struct CoopInvite ack;
        memset(&ack, 0, sizeof(ack));
        ack.magic = COOP_INVITE_MAGIC;
        ack.kind = COOP_KIND_ACK;
        ack.seq = invite->seq;
        ack.checksum = Coop_InviteChecksum(&ack);
        if (!gLinkTransferringData)
            SendBlock(BitmaskAllOtherLinkPlayers(), &ack, sizeof(ack));
    }
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
    sWaitingAck = FALSE;
    sWaitKind = 0;
    sRequest = COOP_REQ_NONE;
    gSpecialVar_Result = FALSE;
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
    Coop_SendInvite();
    if (++sInviteTimer >= COOP_ACK_TIMEOUT)
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

static void Coop_PollHandshake(void)
{
    if (GetLinkPlayerCount_2() >= 2)
    {
        if (!sHandshakeSaved)
        {
            SaveLinkPlayers(2);
            sHandshakeSaved = TRUE;
        }
        if (IsLinkMaster())
            CheckShouldAdvanceLinkState();
    }
    if (gReceivedRemoteLinkPlayers && IsLinkPlayerDataExchangeComplete())
    {
        sLinkState = COOP_LINK_UP;
        sTradeSlot = 0xFF;
        sRivalFightStage = 0xFF;
        sProposedCase = COOP_CASE_NONE;
        sClaim = COOP_CASE_NONE;
        gLinkCallback = Coop_LinkCallback;
        SetSuppressLinkErrorMessage(TRUE);
    }
    if (gLinkErrorOccurred)
        Coop_Drop(Coop_EventScript_Disconnected);
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

static void Coop_Drop(const u8 *message)
{
    sLinkState = COOP_LINK_IDLE;
    sHandshakeSaved = FALSE;
    sMerged = FALSE;
    sPartnerValid = FALSE;
    sWaitingAck = FALSE;
    sResumeScript = FALSE;
    sRequest = COOP_REQ_NONE;
    sTradeSlot = 0xFF;
    gLinkCallback = NULL;
    Coop_RemoveSprite();
    CloseLink();
    SetSuppressLinkErrorMessage(TRUE);
    sPendingMessage = message;
}

static void Task_CoopSession(u8 taskId)
{
    if (sLinkState == COOP_LINK_CONNECTING)
        Coop_PollHandshake();
    if (sLinkState == COOP_LINK_UP)
    {
        SetSuppressLinkErrorMessage(TRUE);
        Coop_MaintainCallback();
        Coop_UpdateSprite();
        Coop_PollBlocks();
        Coop_PollScriptWaits();
        Coop_PollTrade();
        Coop_PollInviteTimeout();
        if (gLinkErrorOccurred || (sMerged && !gReceivedRemoteLinkPlayers))
            Coop_Drop(Coop_EventScript_Disconnected);
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
    sHandshakeSaved = FALSE;
    sMerged = FALSE;
    sPartnerValid = FALSE;
    Coop_EnsureTask();
    return TRUE;
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

    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap())
        return FALSE;
    if (trainerA == TRAINER_NONE || trainerA >= TRAINERS_COUNT || IsSpecialTrainer(trainerA))
        return FALSE;
    if (Coop_RivalStageFromId(trainerA) != 0xFF)
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

u16 Coop_StartGymBattle(void)
{
    if (!Coop_PartnerOnSameMap())
    {
        gSpecialVar_Result = FALSE;
        CreateTask(Task_CoopEnableScript, 0);
        return FALSE;
    }
    sResumeScript = TRUE;
    sLegendKind = 0;
    Coop_QueueInvite((u8)VarGet(VAR_0x8008), VarGet(VAR_0x8004), VarGet(VAR_0x8005), (u8)VarGet(VAR_0x8006), VarGet(VAR_0x8007));
    gSpecialVar_Result = FALSE;
    return TRUE;
}

u16 Coop_StartLegendaryBattle(void)
{
    if (!Coop_PartnerOnSameMap())
    {
        gSpecialVar_Result = FALSE;
        CreateTask(Task_CoopEnableScript, 0);
        return FALSE;
    }
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

u16 Coop_RequestLabWarp(void)
{
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

void Coop_OnBattleLaunched(void)
{
    memcpy(sPartySnap, gParties[B_TRAINER_PLAYER], sizeof(sPartySnap));
    sHasSnapshot = TRUE;
    sAborting = FALSE;
    sInBattle = TRUE;
    gCoopBattleLaunching = FALSE;
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

bool8 Coop_FinishTrainerBattle(void)
{
    bool8 won = gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT;

    sInBattle = FALSE;
    sHasSnapshot = FALSE;
    if (won)
        Coop_GiveJointWin();
    gSpecialVar_Result = won ? TRUE : FALSE;
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

bool8 Coop_FinishWildBattle(void)
{
    bool8 won = gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT;

    sInBattle = FALSE;
    sHasSnapshot = FALSE;
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
    if (sHasSnapshot)
        memcpy(gParties[B_TRAINER_PLAYER], sPartySnap, sizeof(sPartySnap));
    sHasSnapshot = FALSE;
    sAborting = FALSE;
    sInBattle = FALSE;
    sWaitingAck = FALSE;
    gCoopBattleLaunching = FALSE;
    gCoopArmWild = FALSE;
    ScriptContext_Init();
    UnlockPlayerFieldControls();
    FlagClear(FLAG_SYS_CTRL_OBJ_DELETE);
    sPendingMessage = Coop_EventScript_BattleAborted;
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

bool8 Coop_CommitCatch(struct Pokemon *caughtMon)
{
    struct CoopSave *save = CoopSave();
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    bool8 local;
    u8 index;
    u16 species;

    if (!(gBattleTypeFlags & BATTLE_TYPE_COOP))
        return TRUE;
    local = caughtMon >= party && caughtMon < party + PARTY_SIZE;
    species = GetMonData(caughtMon, MON_DATA_SPECIES);
    index = Coop_LegendIndex(species);
    if (HasLinkErrorOccurred())
    {
        if (save != NULL)
            save->pendingLegendary = 0;
        return FALSE;
    }
    if (save != NULL && index != 0xFF)
    {
        save->pendingLegendary = index + 1;
        save->legendaryOwner[index] = local ? COOP_OWNER_LOCAL : COOP_OWNER_PARTNER;
        save->pendingLegendary = 0;
    }
    return local;
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
    Coop_QueueRival(stage);
    ScriptContext_Stop();
    return TRUE;
}
