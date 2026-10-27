// Partner interaction between two linked saves: friendly battles, trades and
// gifts, started by pressing A on the partner (data/scripts/coop_deal.inc).
//
// Messages travel as link blocks (struct CoopDealMsg). Each carries a sequence
// number and the last sequence it received from the other game, so a lost
// block is resent and a repeated one is ignored.
//
// Transfers use an escrow record in the save (struct CoopDealRecord):
//   1. Both players confirm the exact transfer on screen.
//   2. Prepare: each game moves what it gives out of the party or bag into
//      the record, copies what it will receive into the record, marks it
//      PREPARED, saves, and then says READY.
//   3. Commit: on the partner's READY, each game delivers what it receives,
//      forgets the escrow, notes the deal as done, saves, and says DONE.
// A game never commits until the partner has prepared, and a partner that
// prepared never backs out on its own. If the cable drops between 2 and 3,
// the PREPARED record stays in the save and nothing is lost: the next time
// the same partner links, both games compare records (STATUS) and either
// both commit or the escrow returns to its owner. A deal id is applied once.

#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_quest.h"
#include "coop_outfit.h"
#include "coop_protocol.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_transition.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "field_player_avatar.h"
#include "item.h"
#include "item_menu.h"
#include "load_save.h"
#include "link.h"
#include "mail.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "constants/rgb.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "rtc.h"
#include "save.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "constants/battle.h"
#include "constants/battle_frontier.h"
#include "constants/coop.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/vars.h"

enum
{
    DMSG_ACK = 1,
    DMSG_REQUEST,
    DMSG_ANSWER,
    DMSG_CONFIRM,
    DMSG_READY,
    DMSG_DONE,
    DMSG_CANCEL,
    DMSG_TEAM,
    DMSG_STATUS,
    DMSG_PARTY_REQ,     // practice: send me a copy of your party
    DMSG_PARTY_MON,     // arg: index, arg2: count, mon: one Pokemon
    DMSG_QUEST,         // arg: COOP_QUEST_*, arg2: value, qty: 1 live change, 0 report
    DMSG_LOOK,          // arg: COOP_OUTFIT preset, arg2: gender (src/coop_outfit.c)
    DMSG_CLOCK,         // arg: the link master's local hours, arg2: minutes (shared time of day)
};

// The link master's time of day, received with DMSG_CLOCK (see Clock_Poll).
static EWRAM_DATA bool8 sPartnerClockValid = FALSE;
static EWRAM_DATA u8 sPartnerHours = 0;
static EWRAM_DATA u8 sPartnerMinutes = 0;

// Local deal states.
enum
{
    DS_IDLE,
    DS_ASK,          // initiator: request sent, waiting for the answer
    DS_INCOMING,     // recipient: a request is waiting for this player
    DS_NEGOTIATE,    // both agreed to go on; confirmations next
    DS_TEAM,         // friendly battle: choosing teams
    DS_PREPARED,     // escrow saved, READY sent
};

// What a waiting script waits for.
enum
{
    WAIT_NONE,
    WAIT_ANSWER,
    WAIT_CONFIRM,
    WAIT_READY,
    WAIT_TEAM,
    WAIT_QUIET,     // no deal block in flight, so none lands in the battle's
    WAIT_SAVE,      // this game's save has reached the flash
    WAIT_DONE,      // nothing to wait for: return sDoneResult next frame
};

// Settling a record left PREPARED by an earlier, interrupted exchange.
enum
{
    SETTLE_NONE,
    SETTLE_COMMIT,
    SETTLE_ABORT,
};

// Messages sent once a save has reached the flash.
enum
{
    AFTER_NONE,
    AFTER_READY,
    AFTER_DONE,
    AFTER_COMMITTED,
};

// The game saves a sector every other frame, as the cable trade does, so
// the co-op link keeps running. A save is never started while another
// is writing, and nothing the save covers changes while it writes.
enum
{
    SAVER_IDLE,
    SAVER_INIT,
    SAVER_SECTORS,
    SAVER_LAST,
    SAVER_SIGN,
};

struct DealAfter
{
    u8 type;
    u8 kind;
    u16 dealId;
};

struct DealSaver
{
    u8 state;
    u8 timer;
    bool8 dirty;        // the escrow record in RAM is newer than the flash
    bool8 pending;      // another save was asked for while this one ran
    struct DealAfter now;
    struct DealAfter next;
};

// Reconciliation states carried by STATUS.
#define TXN_REPORT_NONE      0
#define TXN_REPORT_PREPARED  1
#define TXN_REPORT_COMMITTED 2

#define DEAL_RESEND_FRAMES   45
#define DEAL_ANSWER_LIMIT    (60 * 90)  // the partner may be choosing a POKéMON
#define DEAL_CONFIRM_LIMIT   (60 * 60)
#define DEAL_READY_LIMIT     (60 * 25)
#define DEAL_TEAM_LIMIT      (60 * 120)
#define DEAL_PROMPT_LIMIT    (60 * 8)
#define DEAL_STATUS_FRAMES   120
#define DEAL_QUIET_FRAMES    75   // longer than one resend interval
#define DEAL_QUIET_LIMIT     (60 * 10)

struct CoopDealMsg
{
    u16 magic;
    u8 type;
    u8 kind;
    u16 dealId;
    u16 seq;
    u16 ack;
    u8 arg;
    u8 arg2;
    u16 item;
    u16 qty;
    u32 otId;
    u32 mon[20];
    u16 checksum;
    u16 pad;
};

STATIC_ASSERT(sizeof(struct BoxPokemon) == sizeof(((struct CoopDealRecord *)0)->outMon), CoopDealRecordHoldsBoxMon);
STATIC_ASSERT(sizeof(struct CoopDealMsg) <= BLOCK_BUFFER_SIZE, CoopDealMsgFitsBlock);
STATIC_ASSERT(COOP_DEX_FLAG_BYTES * 8 >= NATIONAL_DEX_COUNT, CoopDexCacheCoversNationalDex);

struct CoopDealState
{
    u8 state;
    u8 kind;
    u8 initiator;
    u8 rules;
    u16 dealId;
    u8 waitKind;
    u16 waitTimer;
    u16 promptTimer;
    bool8 prompted;
    bool8 myConfirmed;
    bool8 partnerConfirmed;
    bool8 partnerConfirmKnown;
    bool8 partnerReady;
    bool8 partnerTeam;
    bool8 answered;
    u8 answer;
    u8 mySlot;
    u16 myItem;
    u16 myQty;
    u32 myPersonality;
    bool8 partnerHasMon;
    u16 partnerItem;
    u16 partnerQty;
    u32 partnerOt;
    u8 cancelReason;
    bool8 deliveredInParty;
    u8 deliveredSlot;
    struct BoxPokemon partnerMon;
};

// The cable side, which outlives any one deal: messages go out in order,
// each resent until the partner acknowledges it.
#define DEAL_QUEUE_SIZE 8
struct CoopDealLink
{
    struct CoopDealMsg queue[DEAL_QUEUE_SIZE];
    u8 head;
    u8 count;
    u16 outSeq;
    u16 lastInSeq;
    bool8 inSeqKnown;
    bool8 needAck;
    u16 resendTimer;
    u16 statusTimer;
    u16 quietTimer;
};

EWRAM_DATA bool8 gCoopFriendlyBattle = FALSE;
EWRAM_DATA u8 gCoopFriendlyRules = 0;
EWRAM_DATA bool8 gCoopPracticeBattle = FALSE;
EWRAM_DATA u8 gCoopBattleSpeed = 0;

static EWRAM_DATA bool8 sLookDue = FALSE;
static EWRAM_DATA struct CoopDealState sDeal = {0};
static EWRAM_DATA struct CoopDealLink sLink = {0};
static EWRAM_DATA struct DealSaver sSaver = {0};
static EWRAM_DATA u8 sSettle = 0;
static EWRAM_DATA u16 sSettleDeal = 0;
static EWRAM_DATA struct Pokemon sFriendlySnap[PARTY_SIZE] = {0};
static EWRAM_DATA bool8 sFriendlyHasSnap = FALSE;
static EWRAM_DATA u8 sFriendlyOutcome = 0;
static EWRAM_DATA bool8 sFriendlyAborting = FALSE;
static EWRAM_DATA u8 sEarlyEnd = 0; // COOP_DEAL_* + 1 for a script not yet waiting
static EWRAM_DATA u8 sDoneResult = 0;
static EWRAM_DATA u16 sSavedFacility = 0;
static EWRAM_DATA const u8 *sDealMessage = NULL;
static EWRAM_DATA u8 sTeamOrder[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sTeamCount = 0;
static EWRAM_DATA struct Pokemon sTradeGiven = {0};
static EWRAM_DATA u8 sTradeEvoSlot = 0; // party slot + 1 of a traded-in Pokemon that may evolve
static EWRAM_DATA bool8 sNoRoomShown = FALSE;
static EWRAM_DATA u8 sSessionCount = 0; // cable sessions since power-on (diagnostics)

extern const u8 CoopDeal_EventScript_Incoming[];
extern const u8 CoopDeal_EventScript_Settle[];
extern const u8 CoopDeal_EventScript_NoRoomToSettle[];

static struct CoopDealMsg *Deal_Send(u8 type, u8 arg);
static void Deal_EndWait(u8 result);
static void Deal_Reset(void);
static void Dex_Poll(void);
static void Team_HandleMsg(const struct CoopDealMsg *msg);

// ---------------------------------------------------------------- helpers

static u16 Deal_Checksum(const struct CoopDealMsg *msg)
{
    const u16 *words = (const u16 *)msg;
    u32 i, sum = 0x5D11;

    for (i = 0; i < offsetof(struct CoopDealMsg, checksum) / 2; i++)
        sum = ((sum << 3) | (sum >> 13)) ^ words[i];
    return (u16)(sum ^ (sum >> 16));
}

static u8 Deal_PartnerLinkId(void)
{
    return GetMultiplayerId() ^ 1;
}

static u32 Deal_MyOtId(void)
{
    return READ_OTID_FROM_SAVE;
}

static u32 Deal_PartnerOtId(void)
{
    return gLinkPlayers[Deal_PartnerLinkId()].trainerId;
}

static struct CoopDealRecord *Deal_Record(void)
{
    struct CoopSave *save = Coop_GetSave();

    return save != NULL ? &save->deal : NULL;
}

static bool8 Deal_HasPending(void)
{
    struct CoopDealRecord *rec = Deal_Record();

    return rec != NULL && rec->state == COOP_TXN_PREPARED;
}

static bool8 Deal_LinkOk(void)
{
    return Coop_SessionActive() && gReceivedRemoteLinkPlayers;
}

bool8 CoopDeal_Busy(void)
{
    return sDeal.state != DS_IDLE || gCoopFriendlyBattle || gCoopPracticeBattle;
}

// Restricted items never leave a save: key items, HMs, mail, the e-Reader
// berry, and anything the game marks as important.
bool8 CoopDeal_ItemGiftable(u16 item)
{
    enum Pocket pocket;

    if (item == ITEM_NONE || item >= ITEMS_COUNT)
        return FALSE;
    if (GetItemImportance(item) != 0 || ItemIsMail(item) || item == ITEM_ENIGMA_BERRY_E_READER)
        return FALSE;
    pocket = GetItemPocket(item);
    if (pocket == POCKET_KEY_ITEMS)
        return FALSE;
    if (pocket == POCKET_TM_HM && IsMoveHM(GetItemTMHMMoveId(item)))
        return FALSE;
    return TRUE;
}

static u8 Deal_UsableExcept(u8 slot)
{
    u8 i, usable = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (i == slot || GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (!GetMonData(mon, MON_DATA_IS_EGG) && GetMonData(mon, MON_DATA_HP) != 0)
            usable++;
    }
    return usable;
}

// A party Pokemon can leave this save: not an egg, not the last one able to
// fight, and not holding something that must stay.
static u8 Deal_CheckMonCanLeave(u8 slot)
{
    struct Pokemon *mon;
    u16 held;

    if (slot >= PARTY_SIZE)
        return COOP_DEAL_INVALID;
    mon = &gParties[B_TRAINER_PLAYER][slot];
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return COOP_DEAL_INVALID;
    if (Deal_UsableExcept(slot) == 0)
        return COOP_DEAL_NO_ROOM;
    held = GetMonData(mon, MON_DATA_HELD_ITEM);
    if (held != ITEM_NONE && !CoopDeal_ItemGiftable(held))
        return COOP_DEAL_BUSY;
    return COOP_DEAL_OK;
}

static bool8 Deal_RoomForMon(void)
{
    return CalculatePlayerPartyCount() < PARTY_SIZE || Coop_PcFreeSlots() != 0;
}

static void Deal_BufferBoxMon(struct BoxPokemon *box, u8 *nameDest, u8 *infoDest)
{
    u8 nick[POKEMON_NAME_LENGTH + 1];
    u16 species = GetBoxMonData(box, MON_DATA_SPECIES);
    u16 held = GetBoxMonData(box, MON_DATA_HELD_ITEM);
    u8 level = GetLevelFromBoxMonExp(box);
    u8 gender = GetBoxMonGender(box);
    u8 *end;

    GetBoxMonData(box, MON_DATA_NICKNAME, nick);
    end = StringCopy_Nickname(nameDest, nick);
    if (StringCompare(nick, GetSpeciesName(species)) != 0)
    {
        end = StringCopy(end, COMPOUND_STRING(" ("));
        end = StringCopy(end, GetSpeciesName(species));
        end = StringCopy(end, COMPOUND_STRING(")"));
    }
    end = StringCopy(infoDest, COMPOUND_STRING("Lv. "));
    end = ConvertIntToDecimalStringN(end, level, STR_CONV_MODE_LEFT_ALIGN, 3);
    if (gender == MON_MALE)
        end = StringCopy(end, COMPOUND_STRING(" ♂"));
    else if (gender == MON_FEMALE)
        end = StringCopy(end, COMPOUND_STRING(" ♀"));
    if (held != ITEM_NONE)
    {
        end = StringCopy(end, COMPOUND_STRING(", holds "));
        CopyItemName(held, end);
    }
    else
    {
        StringCopy(end, COMPOUND_STRING(", no item"));
    }
}

static void Deal_BufferItem(u16 item, u16 qty, u8 *dest)
{
    u8 *end = ConvertIntToDecimalStringN(dest, qty, STR_CONV_MODE_LEFT_ALIGN, 3);

    end = StringCopy(end, COMPOUND_STRING(" × "));
    CopyItemName(item, end);
}

// ---------------------------------------------------------------- messages

static struct CoopDealMsg *Deal_Push(u8 type, u8 arg, u16 dealId, u8 kind)
{
    struct CoopDealMsg *msg;

    if (sLink.count == DEAL_QUEUE_SIZE)
    {
        // Full (never expected): the newest message replaces the last one.
        msg = &sLink.queue[(sLink.head + sLink.count - 1) % DEAL_QUEUE_SIZE];
    }
    else
    {
        msg = &sLink.queue[(sLink.head + sLink.count) % DEAL_QUEUE_SIZE];
        sLink.count++;
    }
    memset(msg, 0, sizeof(*msg));
    msg->magic = COOP_DEAL_MAGIC;
    msg->type = type;
    msg->kind = kind;
    msg->dealId = dealId;
    msg->seq = ++sLink.outSeq;
    msg->arg = arg;
    msg->arg2 = sDeal.rules;
    msg->otId = Deal_MyOtId();
    if (sLink.count == 1)
        sLink.resendTimer = 0;
    return msg;
}

static struct CoopDealMsg *Deal_Send(u8 type, u8 arg)
{
    return Deal_Push(type, arg, sDeal.dealId, sDeal.kind);
}

static void Deal_AttachMyOffer(struct CoopDealMsg *msg)
{
    if (sDeal.mySlot < PARTY_SIZE)
        memcpy(msg->mon, &gParties[B_TRAINER_PLAYER][sDeal.mySlot].box, sizeof(struct BoxPokemon));
    msg->item = sDeal.myItem;
    msg->qty = sDeal.myQty;
}

static void Deal_SendStatus(u16 dealId, u8 report)
{
    Deal_Push(DMSG_STATUS, report, dealId, COOP_DEAL_NONE);
}

// The partner has everything up to seq `ack`.
static void Deal_Acked(u16 ack)
{
    while (sLink.count != 0)
    {
        struct CoopDealMsg *head = &sLink.queue[sLink.head];

        if (head->seq != ack && CoopProtocol_SeqIsNew(ack, head->seq))
            break;
        sLink.head = (sLink.head + 1) % DEAL_QUEUE_SIZE;
        sLink.count--;
        sLink.resendTimer = 0;
    }
}

static bool8 Deal_LinkIdle(void)
{
    return sLink.count == 0 && !sLink.needAck;
}

static void Deal_PumpSend(void)
{
    struct CoopDealMsg ack;

    if (gLinkTransferringData)
        return;
    if (sLink.count != 0)
    {
        struct CoopDealMsg *head = &sLink.queue[sLink.head];

        if (sLink.resendTimer != 0)
        {
            sLink.resendTimer--;
            if (!sLink.needAck)
                return;
        }
        head->ack = sLink.lastInSeq;
        head->checksum = Deal_Checksum(head);
        if (SendBlock(BitmaskAllOtherLinkPlayers(), head, sizeof(*head)))
        {
            sLink.resendTimer = DEAL_RESEND_FRAMES;
            sLink.needAck = FALSE;
        }
        return;
    }
    if (sLink.needAck)
    {
        memset(&ack, 0, sizeof(ack));
        ack.magic = COOP_DEAL_MAGIC;
        ack.type = DMSG_ACK;
        ack.ack = sLink.lastInSeq;
        ack.otId = Deal_MyOtId();
        ack.checksum = Deal_Checksum(&ack);
        if (SendBlock(BitmaskAllOtherLinkPlayers(), &ack, sizeof(ack)))
            sLink.needAck = FALSE;
    }
}

// ---------------------------------------------------------------- escrow

static bool8 Deal_Saving(void)
{
    return sSaver.state != SAVER_IDLE || sSaver.pending;
}

// Save the game in the background, then send `after` (if any).
static void Deal_StartSave(u8 after, u16 dealId, u8 kind)
{
    struct DealAfter a = {after, kind, dealId};

    sSaver.dirty = TRUE;
    if (sSaver.state == SAVER_IDLE)
    {
        sSaver.now = a;
        sSaver.state = SAVER_INIT;
    }
    else
    {
        // The running save may have read the old data: write again after.
        sSaver.pending = TRUE;
        if (after != AFTER_NONE)
            sSaver.next = a;
    }
}

static void Deal_SendAfter(const struct DealAfter *a)
{
    switch (a->type)
    {
    case AFTER_READY:
        Deal_Push(DMSG_READY, 0, a->dealId, a->kind);
        break;
    case AFTER_DONE:
        Deal_Push(DMSG_DONE, 0, a->dealId, a->kind);
        break;
    case AFTER_COMMITTED:
        Deal_SendStatus(a->dealId, TXN_REPORT_COMMITTED);
        break;
    }
}

static void Deal_SaveStep(void)
{
    switch (sSaver.state)
    {
    case SAVER_IDLE:
        return;
    case SAVER_INIT:
        if (gFlashMemoryPresent != TRUE)
        {
            sSaver.state = SAVER_SIGN;
            break;
        }
        gSoftResetDisabled = TRUE;
        LinkFullSave_Init();
        sSaver.timer = 0;
        sSaver.state = SAVER_SECTORS;
        break;
    case SAVER_SECTORS:
        if (++sSaver.timer < 2)
            break;
        sSaver.timer = 0;
        if (LinkFullSave_WriteSector())
            sSaver.state = SAVER_LAST;
        break;
    case SAVER_LAST:
        LinkFullSave_ReplaceLastSector();
        sSaver.state = SAVER_SIGN;
        break;
    case SAVER_SIGN:
        if (gFlashMemoryPresent == TRUE)
            LinkFullSave_SetLastSectorSignature();
        gSoftResetDisabled = FALSE;
        Deal_SendAfter(&sSaver.now);
        sSaver.now.type = AFTER_NONE;
        if (sSaver.pending)
        {
            sSaver.pending = FALSE;
            sSaver.now = sSaver.next;
            sSaver.next.type = AFTER_NONE;
            sSaver.state = SAVER_INIT;
        }
        else
        {
            sSaver.dirty = FALSE;
            sSaver.state = SAVER_IDLE;
        }
        break;
    }
}

static u8 Deal_DeliverMon(struct BoxPokemon *box)
{
    struct Pokemon mon;
    u32 i;

    BoxMonToMon(box, &mon);
    CalculateMonStats(&mon);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == SPECIES_NONE)
            break;
    }
    if (i < PARTY_SIZE)
    {
        gParties[B_TRAINER_PLAYER][i] = mon;
        CalculatePlayerPartyCount();
        sDeal.deliveredInParty = TRUE;
        sDeal.deliveredSlot = i;
        HandleSetPokedexFlagFromMon(&gParties[B_TRAINER_PLAYER][i], FLAG_SET_SEEN);
        HandleSetPokedexFlagFromMon(&gParties[B_TRAINER_PLAYER][i], FLAG_SET_CAUGHT);
        return MON_GIVEN_TO_PARTY;
    }
    sDeal.deliveredInParty = FALSE;
    HandleSetPokedexFlagFromMon(&mon, FLAG_SET_SEEN);
    HandleSetPokedexFlagFromMon(&mon, FLAG_SET_CAUGHT);
    return CopyMonToPC(&mon);
}

// The escrow needs somewhere to land: the party or the PC for a Pokemon, the
// bag for an item.
static bool8 Deal_RecordFits(const struct CoopDealRecord *rec, bool8 incoming)
{
    u16 item = incoming ? rec->inItem : rec->outItem;
    u16 qty = incoming ? rec->inQty : rec->outQty;
    bool8 hasMon = incoming ? (rec->hasMons & 2) != 0 : (rec->hasMons & 1) != 0;

    if (hasMon && !Deal_RoomForMon())
        return FALSE;
    if (item != ITEM_NONE && !CheckBagHasSpace(item, qty))
        return FALSE;
    return TRUE;
}

static bool8 Deal_ApplyRecord(bool8 commit)
{
    struct CoopDealRecord *rec = Deal_Record();

    if (rec == NULL || rec->state != COOP_TXN_PREPARED)
        return FALSE;
    if (!Deal_RecordFits(rec, commit))
        return FALSE;
    if (commit)
    {
        if (rec->hasMons & 2)
            Deal_DeliverMon((struct BoxPokemon *)rec->inMon);
        if (rec->inItem != ITEM_NONE)
            AddBagItem(rec->inItem, rec->inQty);
        struct CoopSave *save = Coop_GetSave();

        if (rec->lastDoneId != 0)
        {
            save->dealHistory[2] = save->dealHistory[1];
            save->dealHistory[1] = save->dealHistory[0];
            save->dealHistory[0] = rec->lastDoneId;
        }
        rec->lastDoneId = rec->dealId;
        rec->lastDoneOtId = rec->partnerOtId;
    }
    else
    {
        if (rec->hasMons & 1)
            Deal_DeliverMon((struct BoxPokemon *)rec->outMon);
        if (rec->outItem != ITEM_NONE)
            AddBagItem(rec->outItem, rec->outQty);
    }
    rec->state = COOP_TXN_NONE;
    rec->hasMons = 0;
    rec->outItem = rec->inItem = ITEM_NONE;
    rec->outQty = rec->inQty = 0;
    rec->outSlot = 0xFF;
    memset(rec->outMon, 0, sizeof(rec->outMon));
    memset(rec->inMon, 0, sizeof(rec->inMon));
    return TRUE;
}

// ---------------------------------------------------------------- incoming

static void Deal_HandleRequest(const struct CoopDealMsg *msg)
{
    u8 refuse = 0xFF;

    if (sDeal.state != DS_IDLE)
    {
        // Both asked at once: link player 0 keeps its question; the other
        // game drops its own and answers player 0's.
        if (sDeal.state == DS_ASK && GetMultiplayerId() != 0)
        {
            sDeal.cancelReason = COOP_DEAL_BUSY;
            Deal_EndWait(COOP_DEAL_BUSY);
            Deal_Reset();
        }
        else
        {
            // Engaged in something else: say so, leave this deal alone.
            Deal_Push(DMSG_ANSWER, COOP_DEAL_BUSY, msg->dealId, msg->kind);
            return;
        }
    }
    sDeal.dealId = msg->dealId;
    sDeal.kind = msg->kind;
    sDeal.rules = msg->arg2;
    sDeal.initiator = FALSE;
    sDeal.partnerOt = msg->otId;
    sDeal.partnerItem = msg->item;
    sDeal.partnerQty = msg->qty;
    sDeal.partnerHasMon = (msg->kind == COOP_DEAL_TRADE || msg->kind == COOP_DEAL_GIVE_MON);
    if (sDeal.partnerHasMon)
        memcpy(&sDeal.partnerMon, msg->mon, sizeof(sDeal.partnerMon));
    sDeal.mySlot = 0xFF;
    sDeal.myItem = ITEM_NONE;
    sDeal.myQty = 0;

    if (Deal_HasPending())
        refuse = COOP_DEAL_PENDING;
    else if (Coop_OtherInteractionBusy() || gMain.inBattle || !Coop_DealPartnerHere())
        refuse = COOP_DEAL_BUSY;
    else if (msg->kind == COOP_DEAL_GIVE_MON && !Deal_RoomForMon())
        refuse = COOP_DEAL_NO_ROOM;
    else if (msg->kind == COOP_DEAL_GIVE_ITEM && (!CoopDeal_ItemGiftable(msg->item) || !CheckBagHasSpace(msg->item, msg->qty)))
        refuse = COOP_DEAL_NO_ROOM;

    if (refuse != 0xFF)
    {
        Deal_Send(DMSG_ANSWER, refuse);
        sDeal.state = DS_IDLE;
        return;
    }
    sDeal.state = DS_INCOMING;
    sDeal.prompted = FALSE;
    sDeal.promptTimer = 0;
}

// Said once per link session, not every time the partner asks again.
static void Deal_NoteNoRoom(void)
{
    if (sNoRoomShown)
        return;
    sNoRoomShown = TRUE;
    sDealMessage = CoopDeal_EventScript_NoRoomToSettle;
}

// Settled by a short locked script (CoopDeal_EventScript_Settle), so the
// player is not in a menu or saving while the record and party change.
static void Deal_RequestSettle(u16 dealId, u8 how)
{
    struct CoopDealRecord *rec = Deal_Record();

    // A commit never turns back into an abort.
    if (sSettle == SETTLE_COMMIT && sSettleDeal == dealId)
        return;
    // No room yet: say so once, and settle when the partner next asks.
    if (rec == NULL || !Deal_RecordFits(rec, how == SETTLE_COMMIT))
    {
        Deal_NoteNoRoom();
        return;
    }
    sSettle = how;
    sSettleDeal = dealId;
    if (sDealMessage == NULL)
        sDealMessage = CoopDeal_EventScript_Settle;
}

static bool8 Deal_WasCommitted(u16 dealId, u32 otId)
{
    struct CoopSave *save = Coop_GetSave();
    struct CoopDealRecord *rec = &save->deal;
    u32 i;

    if (dealId == 0)
        return FALSE;
    if (rec->lastDoneId == dealId && rec->lastDoneOtId == otId)
        return TRUE;
    for (i = 0; i < ARRAY_COUNT(save->dealHistory); i++)
    {
        if (save->dealHistory[i] == dealId)
            return TRUE;
    }
    return FALSE;
}

static void Deal_HandleStatus(const struct CoopDealMsg *msg)
{
    struct CoopDealRecord *rec = Deal_Record();

    if (rec == NULL)
        return;
    // In the live exchange, a partner that already timed out reports its
    // PREPARED record: that is its READY.
    if (sDeal.state == DS_PREPARED && msg->dealId == sDeal.dealId)
    {
        if (msg->arg != TXN_REPORT_NONE)
            sDeal.partnerReady = TRUE;
        return;
    }
    if (sDeal.state != DS_IDLE)
        return;
    if (rec->state == COOP_TXN_PREPARED && rec->dealId == msg->dealId && rec->partnerOtId == msg->otId)
    {
        Deal_RequestSettle(msg->dealId, msg->arg == TXN_REPORT_NONE ? SETTLE_ABORT : SETTLE_COMMIT);
        return;
    }
    // Only a partner that still holds a PREPARED record asks.
    if (msg->arg != TXN_REPORT_PREPARED)
        return;
    if (Deal_WasCommitted(msg->dealId, msg->otId))
        Deal_SendStatus(msg->dealId, TXN_REPORT_COMMITTED);
    else if (!(rec->state == COOP_TXN_PREPARED && rec->dealId == msg->dealId))
        Deal_SendStatus(msg->dealId, TXN_REPORT_NONE);
}

void CoopDeal_HandleBlock(u32 playerId, const u16 *words)
{
    struct CoopDealMsg msg;

    if (playerId == GetMultiplayerId())
        return;
    memcpy(&msg, words, sizeof(msg));
    if (msg.magic != COOP_DEAL_MAGIC || msg.checksum != Deal_Checksum(&msg))
        return;
    // A block can be overwritten in the receive buffer by the next one
    // before it is read; whatever does not belong to this exchange is
    // dropped rather than trusted.
    if (msg.otId != Deal_PartnerOtId())
        return;
    sLink.quietTimer = 0;
    // An ack can only cover messages this game has sent.
    if (!CoopProtocol_SeqIsNew(sLink.outSeq, msg.ack))
        Deal_Acked(msg.ack);
    if (msg.type == DMSG_ACK)
        return;
    if (sLink.inSeqKnown && !CoopProtocol_SeqIsNew(sLink.lastInSeq, msg.seq))
    {
        sLink.needAck = TRUE;
        return;
    }
    // The partner sends one message at a time and repeats it until it is
    // acknowledged, so a real one is always the next number. Anything else
    // is a damaged block: accepting it would make every later message look
    // old, and the exchange would stall.
    if (sLink.inSeqKnown && (u16)(msg.seq - sLink.lastInSeq) != 1)
        return;
    sLink.lastInSeq = msg.seq;
    sLink.inSeqKnown = TRUE;
    sLink.needAck = TRUE;

    if (msg.type == DMSG_REQUEST)
    {
        Deal_HandleRequest(&msg);
        return;
    }
    if (msg.type == DMSG_STATUS)
    {
        Deal_HandleStatus(&msg);
        return;
    }
    if (msg.type == DMSG_PARTY_REQ || msg.type == DMSG_PARTY_MON)
    {
        Team_HandleMsg(&msg);
        return;
    }
    if (msg.type == DMSG_QUEST)
    {
        CoopQuest_HandleSync(msg.arg, msg.arg2, msg.qty != 0);
        return;
    }
    if (msg.type == DMSG_LOOK)
    {
        CoopOutfit_OnPartnerLook(msg.arg, msg.arg2);
        return;
    }
    if (msg.type == DMSG_CLOCK)
    {
        if (msg.arg < 24 && msg.arg2 < 60)
        {
            bool8 changed = !sPartnerClockValid || sPartnerHours != msg.arg || sPartnerMinutes != msg.arg2;

            sPartnerHours = msg.arg;
            sPartnerMinutes = msg.arg2;
            sPartnerClockValid = TRUE;
            if (changed)
                gTimeUpdateCounter = 0; // re-tint on the next overworld frame
        }
        return;
    }
    if (msg.dealId != sDeal.dealId || sDeal.state == DS_IDLE)
    {
        // A READY for a deal this game has escrowed but already gave up
        // waiting for: settle it now.
        if (msg.type == DMSG_READY || msg.type == DMSG_DONE)
        {
            struct CoopDealRecord *rec = Deal_Record();
            if (sDeal.state == DS_IDLE && rec != NULL && rec->state == COOP_TXN_PREPARED
             && rec->dealId == msg.dealId && rec->partnerOtId == msg.otId)
                Deal_RequestSettle(msg.dealId, SETTLE_COMMIT);
        }
        return;
    }

    switch (msg.type)
    {
    case DMSG_ANSWER:
        if (sDeal.state != DS_ASK)
            break;
        sDeal.answered = TRUE;
        sDeal.answer = msg.arg;
        if (msg.arg == COOP_DEAL_OK)
        {
            sDeal.partnerItem = msg.item;
            sDeal.partnerQty = msg.qty;
            if (sDeal.kind == COOP_DEAL_TRADE)
            {
                memcpy(&sDeal.partnerMon, msg.mon, sizeof(sDeal.partnerMon));
                sDeal.partnerHasMon = TRUE;
            }
            // Accepting a gift is the recipient's confirmation.
            if (sDeal.kind == COOP_DEAL_GIVE_MON || sDeal.kind == COOP_DEAL_GIVE_ITEM)
            {
                sDeal.partnerConfirmed = TRUE;
                sDeal.partnerConfirmKnown = TRUE;
            }
            sDeal.state = sDeal.kind == COOP_DEAL_BATTLE ? DS_TEAM : DS_NEGOTIATE;
        }
        break;
    case DMSG_CONFIRM:
        sDeal.partnerConfirmKnown = TRUE;
        sDeal.partnerConfirmed = msg.arg == COOP_DEAL_OK;
        break;
    case DMSG_READY:
    case DMSG_DONE:
        sDeal.partnerReady = TRUE;
        break;
    case DMSG_TEAM:
        sDeal.partnerTeam = TRUE;
        break;
    case DMSG_CANCEL:
        if (sDeal.state == DS_PREPARED)
            break; // never after an escrow; the record settles it
        sDeal.cancelReason = COOP_DEAL_CANCELLED;
        if (sDeal.state == DS_INCOMING && sDeal.prompted)
        {
            // The question is still on screen; its answer will find the
            // deal gone.
            sDeal.state = DS_IDLE;
            break;
        }
        Deal_EndWait(COOP_DEAL_CANCELLED);
        Deal_Reset();
        break;
    }
}

// ---------------------------------------------------------------- waiting

static void Deal_Reset(void)
{
    // A script waiting for a save keeps waiting: the save finishes anyway.
    u8 waitKind = (sDeal.waitKind == WAIT_SAVE || sDeal.waitKind == WAIT_DONE) ? sDeal.waitKind : WAIT_NONE;

    memset(&sDeal, 0, sizeof(sDeal));
    sDeal.waitKind = waitKind;
    sDeal.state = DS_IDLE;
    sDeal.mySlot = 0xFF;
}

static void Deal_EndWait(u8 result)
{
    if (sDeal.waitKind == WAIT_NONE)
    {
        // The deal ended before its script reached the wait (it was still
        // printing): that wait returns this at once.
        sEarlyEnd = result + 1;
        return;
    }
    sDeal.waitKind = WAIT_NONE;
    gSpecialVar_Result = result;
    ScriptContext_Enable();
}

static bool8 Deal_WaitCancellable(void)
{
    return sDeal.waitKind == WAIT_ANSWER || sDeal.waitKind == WAIT_CONFIRM || sDeal.waitKind == WAIT_TEAM;
}

static void Deal_PollWait(void)
{
    u16 limit = 0;

    if (sDeal.waitKind == WAIT_NONE || sDeal.waitKind == WAIT_SAVE || sDeal.waitKind == WAIT_DONE)
        return;
    sDeal.waitTimer++;
    // The partner left the link or the map: nothing more can arrive.
    if (!Deal_LinkOk() || (!Coop_DealPartnerHere() && sDeal.waitKind != WAIT_READY))
    {
        Deal_EndWait(COOP_DEAL_LOST);
        Deal_Reset();
        return;
    }
    switch (sDeal.waitKind)
    {
    case WAIT_ANSWER:
        if (sDeal.answered)
        {
            u8 answer = sDeal.answer;
            Deal_EndWait(answer);
            if (answer != COOP_DEAL_OK)
                Deal_Reset();
            return;
        }
        limit = DEAL_ANSWER_LIMIT;
        break;
    case WAIT_CONFIRM:
        if (sDeal.partnerConfirmKnown)
        {
            if (sDeal.partnerConfirmed)
            {
                Deal_EndWait(COOP_DEAL_OK);
            }
            else
            {
                Deal_EndWait(COOP_DEAL_DECLINED);
                Deal_Reset();
            }
            return;
        }
        limit = DEAL_CONFIRM_LIMIT;
        break;
    case WAIT_READY:
        if (sDeal.partnerReady && !Deal_Saving())
        {
            Deal_EndWait(COOP_DEAL_OK);
            return;
        }
        limit = DEAL_READY_LIMIT;
        break;
    case WAIT_TEAM:
        if (sDeal.partnerTeam)
        {
            Deal_EndWait(COOP_DEAL_OK);
            return;
        }
        limit = DEAL_TEAM_LIMIT;
        break;
    case WAIT_QUIET:
        if (Deal_LinkIdle() && !gLinkTransferringData && ++sLink.quietTimer >= DEAL_QUIET_FRAMES)
        {
            Deal_EndWait(COOP_DEAL_OK);
            return;
        }
        limit = DEAL_QUIET_LIMIT;
        break;
    }
    // Checked after the answers above: a reply that just arrived wins.
    if (Deal_WaitCancellable() && JOY_NEW(B_BUTTON))
    {
        Deal_Send(DMSG_CANCEL, 0);
        Deal_EndWait(COOP_DEAL_CANCELLED);
        Deal_Reset();
        return;
    }
    if (limit != 0 && sDeal.waitTimer >= limit)
    {
        u8 kind = sDeal.waitKind;
        if (kind != WAIT_READY)
            Deal_Send(DMSG_CANCEL, 0);
        Deal_EndWait(kind == WAIT_READY ? COOP_DEAL_LOST : COOP_DEAL_TIMEOUT);
        // After an escrow the save record carries on and settles later.
        Deal_Reset();
    }
}

static bool8 Deal_PlayerFree(void)
{
    return !ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked()
        && gMain.callback2 == CB2_Overworld && !gPaletteFade.active;
}

// ------------------------------------------------------------ shared clock
// Shared-world time of day: while linked, both games show the link master's
// local time of day (outdoor tint, time-of-day encounters and evolutions).
// Only hours and minutes are shared and nothing is written to either save:
// each game's own clock, days, berries and daily events stay its own, and
// unlinking returns to the game's own time at once.
static EWRAM_DATA bool8 sClockSent = FALSE;
static EWRAM_DATA u8 sClockSentHours = 0;
static EWRAM_DATA u8 sClockSentMinutes = 0;
static EWRAM_DATA u8 sClockTimer = 0;
static EWRAM_DATA bool8 sClockWasShared = FALSE;

static void Clock_Poll(void)
{
    if (!IsLinkMaster())
        return;
    if (++sClockTimer < 30)
        return;
    sClockTimer = 0;
    RtcCalcLocalTime();
    if (sClockSent && gLocalTime.hours == sClockSentHours && gLocalTime.minutes == sClockSentMinutes)
        return;
    if (sLink.count + 1 >= DEAL_QUEUE_SIZE)
        return;
    {
        struct CoopDealMsg *msg = Deal_Push(DMSG_CLOCK, gLocalTime.hours, 0xFFFF, COOP_DEAL_NONE);
        msg->arg2 = gLocalTime.minutes;
    }
    sClockSent = TRUE;
    sClockSentHours = gLocalTime.hours;
    sClockSentMinutes = gLocalTime.minutes;
}

bool8 CoopDeal_SharedClock(s32 *hours, s32 *minutes)
{
    bool8 shared = Deal_LinkOk() && !IsLinkMaster() && sPartnerClockValid;

    if (shared != sClockWasShared)
    {
        sClockWasShared = shared;
        gTimeUpdateCounter = 0; // sharing began or ended: re-tint
    }
    if (!shared)
        return FALSE;
    *hours = sPartnerHours;
    *minutes = sPartnerMinutes;
    return TRUE;
}

void CoopDeal_Poll(void)
{
    struct CoopDealRecord *rec = Deal_Record();
    s32 hours, minutes;

    // Notices sharing start or stop (unlink, a dropped cable) at once.
    CoopDeal_SharedClock(&hours, &minutes);
    if (!Deal_LinkOk())
    {
        // Nothing can be sent or received. A waiting script must not wait
        // forever, and a question not yet shown is dropped.
        if (sDeal.waitKind != WAIT_NONE && sDeal.waitKind != WAIT_SAVE && sDeal.waitKind != WAIT_DONE)
        {
            Deal_EndWait(COOP_DEAL_LOST);
            Deal_Reset();
        }
        else if (sDeal.state == DS_INCOMING && !sDeal.prompted)
        {
            Deal_Reset();
        }
        return;
    }
    Clock_Poll();
    if (sLookDue && sLink.count + 1 < DEAL_QUEUE_SIZE)
    {
        struct CoopDealMsg *look = Deal_Push(DMSG_LOOK, CoopOutfit_Local(), 0xFFFF, COOP_DEAL_NONE);
        look->arg2 = gSaveBlock2Ptr->playerGender;
        sLookDue = FALSE;
    }
    Deal_PumpSend();
    Deal_PollWait();
    Dex_Poll();
    CoopQuest_Poll();

    if (sDeal.state == DS_INCOMING && !sDeal.prompted)
    {
        if (Deal_PlayerFree())
        {
            sDeal.prompted = TRUE;
            ScriptContext_SetupScript(CoopDeal_EventScript_Incoming);
        }
        else if (++sDeal.promptTimer >= DEAL_PROMPT_LIMIT)
        {
            Deal_Send(DMSG_ANSWER, COOP_DEAL_BUSY);
            Deal_Reset();
        }
    }

    // A record left PREPARED settles as soon as its partner is linked.
    if (sDeal.state == DS_IDLE && rec != NULL && rec->state == COOP_TXN_PREPARED
     && rec->partnerOtId == Deal_PartnerOtId() && sLink.count == 0 && !sSaver.dirty && !Deal_Saving())
    {
        if (++sLink.statusTimer >= DEAL_STATUS_FRAMES)
        {
            sLink.statusTimer = 0;
            Deal_SendStatus(rec->dealId, TXN_REPORT_PREPARED);
        }
    }

    if (sDealMessage != NULL && Deal_PlayerFree())
    {
        const u8 *script = sDealMessage;
        sDealMessage = NULL;
        ScriptContext_SetupScript(script);
    }
}

// ---------------------------------------------------------------- practice

// A copy of the partner's party for a practice battle (src/coop_practice.c).
// Asked for with PARTY_REQ; the partner answers with one PARTY_MON per
// Pokemon, eggs left out. Nothing of the partner's game changes.
static EWRAM_DATA struct BoxPokemon sTeamMons[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sPartnerTeamCount = 0;
static EWRAM_DATA u8 sTeamGot = 0;      // bit per index received
static EWRAM_DATA bool8 sTeamAsked = FALSE;

static void Team_HandleMsg(const struct CoopDealMsg *msg)
{
    u8 i, n = 0, order[PARTY_SIZE];

    if (msg->type == DMSG_PARTY_REQ)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
            if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG))
                order[n++] = i;
        }
        // All or nothing: the queue must take the whole team.
        if (DEAL_QUEUE_SIZE - sLink.count < n)
            return;
        for (i = 0; i < n; i++)
        {
            struct CoopDealMsg *out = Deal_Push(DMSG_PARTY_MON, i, 0xFFFF, COOP_DEAL_NONE);
            out->arg2 = n;
            memcpy(out->mon, &gParties[B_TRAINER_PLAYER][order[i]].box, sizeof(struct BoxPokemon));
        }
        return;
    }
    if (!sTeamAsked || msg->arg >= PARTY_SIZE || msg->arg2 == 0 || msg->arg2 > PARTY_SIZE)
        return;
    memcpy(&sTeamMons[msg->arg], msg->mon, sizeof(struct BoxPokemon));
    sPartnerTeamCount = msg->arg2;
    sTeamGot |= 1 << msg->arg;
}

// Starts asking. VAR_RESULT: COOP_DEAL_OK, or LOST when not linked.
u16 CoopDeal_RequestPartnerTeam(void)
{
    sTeamAsked = FALSE;
    sPartnerTeamCount = 0;
    sTeamGot = 0;
    if (!Deal_LinkOk() || !gReceivedRemoteLinkPlayers)
        return COOP_DEAL_LOST;
    CoopDeal_BufferPartnerName();
    sTeamAsked = TRUE;
    Deal_Push(DMSG_PARTY_REQ, 0, 0xFFFF, COOP_DEAL_NONE);
    return COOP_DEAL_OK;
}

// The whole team has arrived: its size, else 0.
u8 CoopDeal_PartnerTeamReady(void)
{
    if (!sTeamAsked || sPartnerTeamCount == 0 || sTeamGot != (1 << sPartnerTeamCount) - 1)
        return 0;
    return sPartnerTeamCount;
}

void CoopDeal_CopyPartnerTeamMon(u8 index, struct Pokemon *dest)
{
    BoxMonToMon(&sTeamMons[index], dest);
}

// ---------------------------------------------------------------- Pokedex

// Each game sends its own caught flags when a session starts, when they
// change, and now and then. The other game keeps them, with the sender's
// trainer id and name, in its save (struct CoopSave): the list then shows
// the partner's catches, live while linked to that partner and marked
// "last synced" otherwise. They never set this save's own flags.
#define DEX_FIRST_SEND     90
#define DEX_REFRESH     (60 * 30)

struct CoopDexMsg
{
    u16 magic;
    u16 count;
    u32 otId;
    u8 name[PLAYER_NAME_LENGTH + 1];
    u8 caught[COOP_DEX_FLAG_BYTES];
    u8 speed;       // the sender's COOP_SPEED_* battle options
    u8 pad;
    u16 checksum;
    u16 pad2;
};

STATIC_ASSERT(sizeof(struct CoopDexMsg) <= BLOCK_BUFFER_SIZE, CoopDexMsgFitsBlock);
STATIC_ASSERT(NUM_DEX_FLAG_BYTES <= COOP_DEX_FLAG_BYTES, CoopDexCacheHoldsCaughtFlags);

static EWRAM_DATA u32 sDexSentHash = 0;
static EWRAM_DATA bool8 sDexSent = FALSE;
static EWRAM_DATA u16 sDexTimer = 0;
static EWRAM_DATA u8 sPartnerSpeed = 0;     // this session's partner options
static EWRAM_DATA bool8 sPartnerSpeedKnown = FALSE;

// This save's own battle options: BATTLE SCENE OFF drops animations,
// FAST (CoopSave.battleSpeed) also shortens message pauses.
u8 Coop_LocalBattleSpeed(void)
{
    struct CoopSave *save = Coop_GetSave();
    u8 speed = 0;

    if (gSaveBlock2Ptr->optionsBattleSceneOff)
        speed |= COOP_SPEED_NO_ANIMS;
    if (save != NULL && (save->battleSpeed & COOP_SPEED_SHORT_TEXT) && (speed & COOP_SPEED_NO_ANIMS))
        speed |= COOP_SPEED_SHORT_TEXT;
    return speed;
}

// What a battle uses. A battle both games watch (co-op, friendly) is run
// by the link master, so it takes only what both players chose; a partner
// not heard from yet counts as normal speed.
u8 Coop_BattleSpeed(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
        return Coop_LocalBattleSpeed();
    if (!COOP_SESSION_LINK_BATTLE() || !sPartnerSpeedKnown)
        return 0;
    return Coop_LocalBattleSpeed() & sPartnerSpeed;
}

static u16 Dex_Checksum(const struct CoopDexMsg *msg)
{
    const u16 *words = (const u16 *)msg;
    u32 i, sum = 0xDE51;

    for (i = 0; i < offsetof(struct CoopDexMsg, checksum) / 2; i++)
        sum = ((sum << 5) | (sum >> 11)) ^ words[i];
    return (u16)(sum ^ (sum >> 16));
}

static u32 Dex_Hash(void)
{
    u32 i, h = 2166136261u;

    for (i = 0; i < NUM_DEX_FLAG_BYTES; i++)
        h = (h ^ gSaveBlock1Ptr->dexCaught[i]) * 16777619u;
    return h;
}

static void Dex_Poll(void)
{
    struct CoopDexMsg msg;
    u32 hash;

    if (sDexTimer < 0xFFFF)
        sDexTimer++;
    // Only between deals, with nothing else in flight.
    if (sDeal.state != DS_IDLE || sLink.count != 0 || sLink.needAck || gLinkTransferringData
     || Coop_OtherInteractionBusy() || sDexTimer < DEX_FIRST_SEND)
        return;
    hash = Dex_Hash() ^ (Coop_LocalBattleSpeed() * 0x9E3779B1u);
    if (sDexSent && hash == sDexSentHash && sDexTimer < DEX_REFRESH)
        return;
    memset(&msg, 0, sizeof(msg));
    msg.magic = COOP_DEX_MAGIC;
    msg.count = GetNationalPokedexCount(FLAG_GET_CAUGHT);
    msg.otId = Deal_MyOtId();
    StringCopy(msg.name, gSaveBlock2Ptr->playerName);
    memcpy(msg.caught, gSaveBlock1Ptr->dexCaught, NUM_DEX_FLAG_BYTES);
    msg.speed = Coop_LocalBattleSpeed();
    msg.checksum = Dex_Checksum(&msg);
    if (SendBlock(BitmaskAllOtherLinkPlayers(), &msg, sizeof(msg)))
    {
        sDexSent = TRUE;
        sDexSentHash = hash;
        sDexTimer = 0;
    }
}

void CoopDex_HandleBlock(u32 playerId, const u16 *words)
{
    struct CoopDexMsg msg;
    struct CoopSave *save = Coop_GetSave();

    if (playerId == GetMultiplayerId() || save == NULL)
        return;
    memcpy(&msg, words, sizeof(msg));
    if (msg.magic != COOP_DEX_MAGIC || msg.checksum != Dex_Checksum(&msg))
        return;
    sPartnerSpeed = msg.speed;
    sPartnerSpeedKnown = TRUE;
    // A different partner replaces the old one's catches entirely.
    save->partnerDexOtId = msg.otId;
    StringCopyN(save->partnerDexName, msg.name, PLAYER_NAME_LENGTH);
    save->partnerDexName[PLAYER_NAME_LENGTH] = EOS;
    memcpy(save->partnerCaught, msg.caught, NUM_DEX_FLAG_BYTES);
    save->partnerDexHours = gSaveBlock2Ptr->playTimeHours;
    save->partnerDexMinutes = gSaveBlock2Ptr->playTimeMinutes;
    save->partnerDexValid = TRUE;
}

// The cached catches apply: synced from this partner, or offline.
static bool8 Dex_CacheApplies(void)
{
    struct CoopSave *save = Coop_GetSave();

    if (save == NULL || !save->partnerDexValid)
        return FALSE;
    if (Deal_LinkOk() && gReceivedRemoteLinkPlayers && save->partnerDexOtId != Deal_PartnerOtId())
        return FALSE; // linked to someone else: not synced from them yet
    return TRUE;
}

bool8 CoopDex_PartnerCaught(u16 nationalNum)
{
    struct CoopSave *save;
    u32 i;

    if (nationalNum == 0 || nationalNum > NATIONAL_DEX_COUNT || !Dex_CacheApplies())
        return FALSE;
    save = Coop_GetSave();
    i = nationalNum - 1;
    return (save->partnerCaught[i / 8] >> (i % 8)) & 1;
}

// CO-OP menu: whose catches are cached and how fresh, with the counts.
// gStringVar4 gets the text. VAR_RESULT: 0 nothing cached, 1 linked to
// that partner now, 2 cached from earlier.
u16 CoopDex_BufferSummary(void)
{
    struct CoopSave *save = Coop_GetSave();
    u16 both = 0, mine = 0, theirs = 0, num;
    bool8 live;
    u8 *end;

    if (!Dex_CacheApplies())
    {
        StringCopy(gStringVar4, COMPOUND_STRING("No partner POKéDEX yet. It is shared\nonce you link with a partner.{PAUSE_UNTIL_PRESS}"));
        return 0;
    }
    for (num = 1; num <= NATIONAL_DEX_COUNT; num++)
    {
        bool8 me = GetSetPokedexFlag(num, FLAG_GET_CAUGHT);
        bool8 them = CoopDex_PartnerCaught(num);

        if (me && them)
            both++;
        else if (me)
            mine++;
        else if (them)
            theirs++;
    }
    live = Deal_LinkOk() && gReceivedRemoteLinkPlayers && save->partnerDexOtId == Deal_PartnerOtId();
    StringCopy(gStringVar1, save->partnerDexName);
    end = StringCopy(gStringVar4, gStringVar1);
    if (live)
    {
        end = StringCopy(end, COMPOUND_STRING("'s catches, linked now.\n"));
    }
    else
    {
        end = StringCopy(end, COMPOUND_STRING("'s catches, last synced at\n"));
        end = ConvertIntToDecimalStringN(end, save->partnerDexHours, STR_CONV_MODE_LEFT_ALIGN, 3);
        end = StringCopy(end, COMPOUND_STRING(":"));
        end = ConvertIntToDecimalStringN(end, save->partnerDexMinutes, STR_CONV_MODE_LEADING_ZEROS, 2);
        end = StringCopy(end, COMPOUND_STRING(" of your play time.\p"));
    }
    end = StringCopy(end, live ? COMPOUND_STRING("Both: ") : COMPOUND_STRING("Both: "));
    end = ConvertIntToDecimalStringN(end, both, STR_CONV_MODE_LEFT_ALIGN, 4);
    end = StringCopy(end, COMPOUND_STRING("  You only: "));
    end = ConvertIntToDecimalStringN(end, mine, STR_CONV_MODE_LEFT_ALIGN, 4);
    end = StringCopy(end, COMPOUND_STRING("\n"));
    end = StringCopy(end, gStringVar1);
    end = StringCopy(end, COMPOUND_STRING(" only: "));
    end = ConvertIntToDecimalStringN(end, theirs, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringCopy(end, COMPOUND_STRING("\pIn the POKéDEX list, a red ball is\nyours, a green one only your\lpartner's, red and green both.{PAUSE_UNTIL_PRESS}"));
    return live ? 1 : 2;
}

void CoopDeal_OnSessionStart(void)
{
    sClockSent = FALSE;
    sClockTimer = 30;
    sPartnerClockValid = FALSE;
    sDexSent = FALSE;
    sDexTimer = 0;
    sPartnerSpeedKnown = FALSE;
    // A new cable session restarts both sides' sequence numbers.
    memset(&sLink, 0, sizeof(sLink));
    sSessionCount++;
    sNoRoomShown = FALSE;
    sLink.statusTimer = DEAL_STATUS_FRAMES - 30;
    if (sDeal.state != DS_PREPARED && sDeal.waitKind == WAIT_NONE)
        Deal_Reset();
    CoopQuest_OnSessionStart();
    CoopOutfit_OnSessionStart();
}

// This save's outfit for the partner: sent at the next chance (new link, or
// a change at the dresser).
void CoopDeal_QueueLook(void)
{
    sLookDue = TRUE;
}

// Quest state for the partner (src/coop_quest.c), delivered in order and
// resent until acknowledged like every other deal block.
void CoopDeal_SendQuest(u8 quest, u8 value, bool8 live)
{
    struct CoopDealMsg *msg;

    if (!Deal_LinkOk())
        return;
    msg = Deal_Push(DMSG_QUEST, quest, 0, COOP_DEAL_NONE);
    msg->arg2 = value;
    msg->qty = live ? 1 : 0;
}

// ---------------------------------------------------------------- specials

// VAR_RESULT: COOP_DEAL_OK, or why not.
u16 CoopDeal_CanStart(void)
{
    sEarlyEnd = 0;
    if (Deal_HasPending())
        return COOP_DEAL_PENDING;
    if (!Deal_LinkOk() || !Coop_DealPartnerHere())
        return COOP_DEAL_LOST;
    if (CoopDeal_Busy() || Coop_OtherInteractionBusy())
        return COOP_DEAL_BUSY;
    return COOP_DEAL_OK;
}

u16 CoopDeal_BufferPartnerName(void)
{
    if (gReceivedRemoteLinkPlayers)
        StringCopy(gStringVar1, gLinkPlayers[Deal_PartnerLinkId()].name);
    else
        StringCopy(gStringVar1, COMPOUND_STRING("your partner"));
    return TRUE;
}

// VAR_0x8004: a party slot. VAR_RESULT: COOP_DEAL_OK, INVALID (egg or empty),
// NO_ROOM (last Pokemon able to fight), BUSY (holds an item that can't go).
u16 CoopDeal_CheckOfferMon(void)
{
    u8 result;

    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        return COOP_DEAL_CANCELLED;
    result = Deal_CheckMonCanLeave(gSpecialVar_0x8004);
    if (result == COOP_DEAL_OK)
        Deal_BufferBoxMon(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004].box, gStringVar2, gStringVar3);
    else if (result == COOP_DEAL_BUSY)
        CopyItemName(GetMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], MON_DATA_HELD_ITEM), gStringVar2);
    return result;
}

// gSpecialVar_ItemId from the gift bag. VAR_RESULT: COOP_DEAL_OK or INVALID.
u16 CoopDeal_CheckGiftItem(void)
{
    if (!CoopDeal_ItemGiftable(gSpecialVar_ItemId) || !CheckBagHasItem(gSpecialVar_ItemId, 1))
        return COOP_DEAL_INVALID;
    CopyItemName(gSpecialVar_ItemId, gStringVar2);
    return COOP_DEAL_OK;
}

// How many of gSpecialVar_ItemId this bag holds (VAR_RESULT, up to 999).
u16 CoopDeal_BufferItemCount(void)
{
    u16 count = CountTotalItemQuantityInBag(gSpecialVar_ItemId);

    ConvertIntToDecimalStringN(gStringVar3, count, STR_CONV_MODE_LEFT_ALIGN, 3);
    return count;
}

// VAR_0x8005: deal kind. Battle: VAR_0x8006 rules. Trade and Pokemon gift:
// VAR_0x8004 party slot. Item gift: gSpecialVar_ItemId, VAR_0x8007 count.
u16 CoopDeal_StartRequest(void)
{
    u8 can = CoopDeal_CanStart();
    u8 kind = gSpecialVar_0x8005;

    if (can != COOP_DEAL_OK)
        return can;
    Deal_Reset();
    sDeal.kind = kind;
    sDeal.initiator = TRUE;
    sDeal.dealId = (u16)((Random() & 0x7FFF) | ((GetMultiplayerId() & 1) << 15));
    if (sDeal.dealId == 0)
        sDeal.dealId = 1;
    sDeal.partnerOt = Deal_PartnerOtId();
    sDeal.mySlot = 0xFF;
    switch (kind)
    {
    case COOP_DEAL_BATTLE:
        sDeal.rules = gSpecialVar_0x8006;
        break;
    case COOP_DEAL_TRADE:
    case COOP_DEAL_GIVE_MON:
        if (Deal_CheckMonCanLeave(gSpecialVar_0x8004) != COOP_DEAL_OK)
            return COOP_DEAL_INVALID;
        sDeal.mySlot = gSpecialVar_0x8004;
        sDeal.myPersonality = GetMonData(&gParties[B_TRAINER_PLAYER][sDeal.mySlot], MON_DATA_PERSONALITY);
        break;
    case COOP_DEAL_GIVE_ITEM:
        if (!CoopDeal_ItemGiftable(gSpecialVar_ItemId) || gSpecialVar_0x8007 == 0
         || !CheckBagHasItem(gSpecialVar_ItemId, gSpecialVar_0x8007))
            return COOP_DEAL_INVALID;
        sDeal.myItem = gSpecialVar_ItemId;
        sDeal.myQty = gSpecialVar_0x8007;
        break;
    default:
        return COOP_DEAL_INVALID;
    }
    sDeal.state = DS_ASK;
    Deal_AttachMyOffer(Deal_Send(DMSG_REQUEST, 0));
    return COOP_DEAL_OK;
}

// VAR_0x8004: what to wait for (1 answer, 2 confirm, 3 ready, 4 teams). The
// script runs `waitstate`; VAR_RESULT then holds a COOP_DEAL_* result.
void CoopDeal_Wait(void)
{
    u8 kind = gSpecialVar_0x8004;
    u8 need;

    switch (kind)
    {
    case WAIT_ANSWER:
        need = DS_ASK;
        break;
    case WAIT_CONFIRM:
        need = DS_NEGOTIATE;
        break;
    case WAIT_READY:
        need = DS_PREPARED;
        break;
    case WAIT_TEAM:
    case WAIT_QUIET:
        need = DS_TEAM;
        break;
    default:
        need = 0xFF; // a save wait belongs to no deal state
        break;
    }
    if (need != 0xFF && sDeal.state != need)
    {
        // The deal already ended (or another took its place): no waiting.
        // `waitstate` follows, so the script resumes on the next frame.
        sDoneResult = sEarlyEnd ? sEarlyEnd - 1 : COOP_DEAL_CANCELLED;
        sEarlyEnd = 0;
        sDeal.waitKind = WAIT_DONE;
        return;
    }
    sEarlyEnd = 0;
    sDeal.waitKind = kind;
    sDeal.waitTimer = 0;
    ScriptContext_Stop();
    // Already there: finish on the next poll.
}

// For the incoming question. VAR_RESULT: the deal kind, or NONE when the
// request is gone. STR_VAR_1 partner, STR_VAR_2 and STR_VAR_3 the details.
u16 CoopDeal_BufferIncoming(void)
{
    sEarlyEnd = 0;
    CoopDeal_BufferPartnerName();
    if (sDeal.state != DS_INCOMING)
        return COOP_DEAL_NONE;
    switch (sDeal.kind)
    {
    case COOP_DEAL_BATTLE:
        gSpecialVar_0x8006 = sDeal.rules;
        CoopDeal_BufferRules();
        break;
    case COOP_DEAL_TRADE:
    case COOP_DEAL_GIVE_MON:
        Deal_BufferBoxMon(&sDeal.partnerMon, gStringVar2, gStringVar3);
        break;
    case COOP_DEAL_GIVE_ITEM:
        Deal_BufferItem(sDeal.partnerItem, sDeal.partnerQty, gStringVar2);
        gStringVar3[0] = EOS;
        break;
    }
    return sDeal.kind;
}

u16 CoopDeal_BufferPartnerOffer(void)
{
    CoopDeal_BufferPartnerName();
    if (!sDeal.partnerHasMon)
        return FALSE;
    Deal_BufferBoxMon(&sDeal.partnerMon, gStringVar2, gStringVar3);
    return TRUE;
}

u16 CoopDeal_BufferMyOffer(void)
{
    if (sDeal.mySlot >= PARTY_SIZE)
        return FALSE;
    Deal_BufferBoxMon(&gParties[B_TRAINER_PLAYER][sDeal.mySlot].box, gStringVar2, gStringVar3);
    return TRUE;
}

// Recipient's answer. VAR_0x8004: 0 no, 1 yes. A trade also needs VAR_0x8005,
// the party slot offered back. VAR_RESULT: COOP_DEAL_OK when the deal goes on,
// CANCELLED when the request had already gone, or INVALID.
u16 CoopDeal_Answer(void)
{
    if (sDeal.state != DS_INCOMING)
    {
        Deal_Reset();
        return COOP_DEAL_CANCELLED;
    }
    if (gSpecialVar_0x8004 == 0)
    {
        Deal_Send(DMSG_ANSWER, COOP_DEAL_DECLINED);
        Deal_Reset();
        return COOP_DEAL_DECLINED;
    }
    switch (sDeal.kind)
    {
    case COOP_DEAL_TRADE:
        if (Deal_CheckMonCanLeave(gSpecialVar_0x8005) != COOP_DEAL_OK)
            return COOP_DEAL_INVALID;
        sDeal.mySlot = gSpecialVar_0x8005;
        sDeal.myPersonality = GetMonData(&gParties[B_TRAINER_PLAYER][sDeal.mySlot], MON_DATA_PERSONALITY);
        Deal_AttachMyOffer(Deal_Send(DMSG_ANSWER, COOP_DEAL_OK));
        sDeal.state = DS_NEGOTIATE;
        break;
    case COOP_DEAL_GIVE_MON:
        if (!Deal_RoomForMon())
        {
            Deal_Send(DMSG_ANSWER, COOP_DEAL_NO_ROOM);
            Deal_Reset();
            return COOP_DEAL_NO_ROOM;
        }
        Deal_Send(DMSG_ANSWER, COOP_DEAL_OK);
        sDeal.myConfirmed = TRUE;
        sDeal.state = DS_NEGOTIATE;
        break;
    case COOP_DEAL_GIVE_ITEM:
        if (!CheckBagHasSpace(sDeal.partnerItem, sDeal.partnerQty))
        {
            Deal_Send(DMSG_ANSWER, COOP_DEAL_NO_ROOM);
            Deal_Reset();
            return COOP_DEAL_NO_ROOM;
        }
        Deal_Send(DMSG_ANSWER, COOP_DEAL_OK);
        sDeal.myConfirmed = TRUE;
        sDeal.state = DS_NEGOTIATE;
        break;
    case COOP_DEAL_BATTLE:
        gCoopFriendlyRules = sDeal.rules;
        Deal_Send(DMSG_ANSWER, COOP_DEAL_OK);
        sDeal.state = DS_TEAM;
        break;
    }
    return COOP_DEAL_OK;
}

// The final yes (VAR_0x8004 = 1) or no to the exact transfer on screen.
u16 CoopDeal_Confirm(void)
{
    if (sDeal.state != DS_NEGOTIATE)
        return COOP_DEAL_CANCELLED;
    if (gSpecialVar_0x8004 == 0)
    {
        Deal_Send(DMSG_CONFIRM, COOP_DEAL_DECLINED);
        Deal_Reset();
        return COOP_DEAL_DECLINED;
    }
    sDeal.myConfirmed = TRUE;
    Deal_Send(DMSG_CONFIRM, COOP_DEAL_OK);
    return COOP_DEAL_OK;
}

// Both confirmed: escrow, save, READY. VAR_RESULT: OK, or INVALID when the
// offer changed (nothing was moved and the partner is told).
u16 CoopDeal_Prepare(void)
{
    struct CoopDealRecord *rec = Deal_Record();
    bool8 giveMon, getMon;

    if (rec == NULL || sDeal.state != DS_NEGOTIATE || !sDeal.myConfirmed || !sDeal.partnerConfirmed)
        return COOP_DEAL_CANCELLED;
    // Nothing the save covers may change while a save is writing.
    if (Deal_Saving())
        return COOP_DEAL_BUSY;

    giveMon = (sDeal.kind == COOP_DEAL_TRADE) || (sDeal.kind == COOP_DEAL_GIVE_MON && sDeal.initiator);
    getMon = (sDeal.kind == COOP_DEAL_TRADE) || (sDeal.kind == COOP_DEAL_GIVE_MON && !sDeal.initiator);

    if (giveMon && (sDeal.mySlot >= PARTY_SIZE
                    || GetMonData(&gParties[B_TRAINER_PLAYER][sDeal.mySlot], MON_DATA_PERSONALITY) != sDeal.myPersonality
                    || Deal_CheckMonCanLeave(sDeal.mySlot) != COOP_DEAL_OK))
    {
        Deal_Send(DMSG_CANCEL, 0);
        Deal_Reset();
        return COOP_DEAL_INVALID;
    }
    if (sDeal.kind == COOP_DEAL_GIVE_ITEM && sDeal.initiator && !CheckBagHasItem(sDeal.myItem, sDeal.myQty))
    {
        Deal_Send(DMSG_CANCEL, 0);
        Deal_Reset();
        return COOP_DEAL_INVALID;
    }

    {
        u16 lastDoneId = rec->lastDoneId;
        u32 lastDoneOt = rec->lastDoneOtId;

        memset(rec, 0, sizeof(*rec));
        rec->lastDoneId = lastDoneId;
        rec->lastDoneOtId = lastDoneOt;
    }
    rec->state = COOP_TXN_NONE;
    rec->kind = sDeal.kind;
    rec->dealId = sDeal.dealId;
    rec->partnerOtId = sDeal.partnerOt;
    rec->outSlot = 0xFF;
    rec->hasMons = 0;
    rec->outItem = rec->inItem = ITEM_NONE;
    rec->outQty = rec->inQty = 0;
    memset(rec->outMon, 0, sizeof(rec->outMon));
    memset(rec->inMon, 0, sizeof(rec->inMon));
    if (giveMon)
    {
        memcpy(rec->outMon, &gParties[B_TRAINER_PLAYER][sDeal.mySlot].box, sizeof(struct BoxPokemon));
        rec->hasMons |= 1;
        rec->outSlot = sDeal.mySlot;
    }
    if (getMon && sDeal.partnerHasMon)
    {
        memcpy(rec->inMon, &sDeal.partnerMon, sizeof(struct BoxPokemon));
        rec->hasMons |= 2;
    }
    if (sDeal.kind == COOP_DEAL_GIVE_ITEM)
    {
        if (sDeal.initiator)
        {
            rec->outItem = sDeal.myItem;
            rec->outQty = sDeal.myQty;
        }
        else
        {
            rec->inItem = sDeal.partnerItem;
            rec->inQty = sDeal.partnerQty;
        }
    }

    // Move what this save gives into the record, then save both together.
    if (giveMon)
    {
        ZeroMonData(&gParties[B_TRAINER_PLAYER][sDeal.mySlot]);
        CompactPartySlots();
        CalculatePlayerPartyCount();
    }
    if (rec->outItem != ITEM_NONE)
        RemoveBagItem(rec->outItem, rec->outQty);
    rec->state = COOP_TXN_PREPARED;
    sDeal.state = DS_PREPARED;
    // READY goes out only once this save is on the flash: the partner
    // commits on READY, so it must never see one this game could forget.
    Deal_StartSave(AFTER_READY, sDeal.dealId, sDeal.kind);
    return COOP_DEAL_OK;
}

// The partner is READY: deliver, save, DONE. VAR_RESULT: OK, or NO_ROOM
// (the record stays and settles later).
u16 CoopDeal_Commit(void)
{
    struct CoopDealRecord *rec = Deal_Record();

    if (rec == NULL || sDeal.state != DS_PREPARED || !sDeal.partnerReady)
        return COOP_DEAL_LOST;
    sDeal.deliveredInParty = FALSE;
    ZeroMonData(&sTradeGiven);
    if (rec->hasMons & 1)
        BoxMonToMon((struct BoxPokemon *)rec->outMon, &sTradeGiven);
    if (Deal_Saving())
        return COOP_DEAL_BUSY;
    if (!Deal_ApplyRecord(TRUE))
        return COOP_DEAL_NO_ROOM;
    Deal_StartSave(AFTER_DONE, sDeal.dealId, sDeal.kind);
    // The deal is over; only a trade evolution may follow. Kept apart from
    // sDeal, which a new cable session or message may reset.
    sTradeEvoSlot = (sDeal.kind == COOP_DEAL_TRADE && sDeal.deliveredInParty) ? sDeal.deliveredSlot + 1 : 0;
    Deal_Reset();
    return COOP_DEAL_OK;
}

// After a Pokemon-for-Pokemon trade, the received Pokemon may evolve by
// trade. VAR_RESULT TRUE: the evolution scene starts (the script waits).
u16 CoopDeal_TryTradeEvolution(void)
{
    struct Pokemon *mon;
    u16 target;
    bool32 canStop = FALSE;

    u8 slot = sTradeEvoSlot - 1;

    sTradeEvoSlot = 0;
    if (slot >= PARTY_SIZE)
        return FALSE;
    mon = &gParties[B_TRAINER_PLAYER][slot];
    target = GetEvolutionTargetSpecies(mon, EVO_MODE_TRADE, ITEM_NONE, &sTradeGiven, &canStop, CHECK_EVO);
    if (target == SPECIES_NONE)
        return FALSE;
    GetEvolutionTargetSpecies(mon, EVO_MODE_TRADE, ITEM_NONE, &sTradeGiven, &canStop, DO_EVO);
    gCB2_AfterEvolution = CB2_ReturnToFieldContinueScript;
    BeginEvolutionScene(mon, target, FALSE, slot);
    return TRUE;
}

// Starts a save; the script waits for it (CoopDeal_Wait 6).
u16 CoopDeal_SaveAfterEvolution(void)
{
    Deal_StartSave(AFTER_NONE, 0, 0);
    return TRUE;
}

// The locked settle script: applies the requested outcome to the record
// left PREPARED, then saves (the script waits). VAR_RESULT: SETTLE_COMMIT,
// SETTLE_ABORT, 3 when there is no room yet, 0 when nothing is to be done.
u16 CoopDeal_Settle(void)
{
    struct CoopDealRecord *rec = Deal_Record();
    u8 how = sSettle;
    u16 dealId = sSettleDeal;

    sSettle = SETTLE_NONE;
    if (rec == NULL || how == SETTLE_NONE || rec->state != COOP_TXN_PREPARED || rec->dealId != dealId)
        return 0;
    if (Deal_Saving())
    {
        // Try again once the save is done.
        Deal_RequestSettle(dealId, how);
        return 0;
    }
    if (!Deal_ApplyRecord(how == SETTLE_COMMIT))
        return 0;
    Deal_StartSave(how == SETTLE_COMMIT ? AFTER_COMMITTED : AFTER_NONE, dealId, COOP_DEAL_NONE);
    return how;
}

// Every overworld frame, linked or not: background saving, and the end of
// a wait for it.
void CoopDeal_FieldFrame(void)
{
    Deal_SaveStep();
    if (sDeal.waitKind == WAIT_SAVE && !Deal_Saving())
        Deal_EndWait(COOP_DEAL_OK);
    else if (sDeal.waitKind == WAIT_DONE)
        Deal_EndWait(sDoneResult);
}

u16 CoopDeal_Cancel(void)
{
    if (sDeal.state != DS_IDLE && sDeal.state != DS_PREPARED)
        Deal_Send(DMSG_CANCEL, 0);
    if (sDeal.state != DS_PREPARED)
        Deal_Reset();
    gCoopFriendlyRules = 0;
    return TRUE;
}

// STR_VAR_1: the partner in the pending record (or "your partner").
u16 CoopDeal_BufferPending(void)
{
    struct CoopDealRecord *rec = Deal_Record();

    StringCopy(gStringVar1, COMPOUND_STRING("your last partner"));
    if (rec != NULL && rec->state == COOP_TXN_PREPARED && gReceivedRemoteLinkPlayers
     && rec->partnerOtId == Deal_PartnerOtId())
        StringCopy(gStringVar1, gLinkPlayers[Deal_PartnerLinkId()].name);
    return rec != NULL && rec->state == COOP_TXN_PREPARED;
}

void CoopDeal_OpenGiftBag(void)
{
    gSpecialVar_ItemId = ITEM_NONE;
    gSpecialVar_Result = FALSE;
    GoToBagMenu(ITEMMENULOCATION_COOP_GIFT, POCKETS_COUNT, CB2_ReturnToFieldContinueScript);
}

// ---------------------------------------------------------------- battles

// STR_VAR_2: format and level. STR_VAR_3: team size and clauses.
u16 CoopDeal_BufferRules(void)
{
    u8 rules = gSpecialVar_0x8006;
    u8 level = (rules >> COOP_RULE_LEVEL_SHIFT) & 3;
    u8 size = (rules >> COOP_RULE_SIZE_SHIFT) & 3;
    u8 *end;

    end = StringCopy(gStringVar2, (rules & COOP_RULE_DOUBLES) ? COMPOUND_STRING("DOUBLES") : COMPOUND_STRING("SINGLES"));
    if (level == COOP_RULE_LEVEL_50)
        StringCopy(end, COMPOUND_STRING(", all at Lv. 50"));
    else if (level == COOP_RULE_LEVEL_100)
        StringCopy(end, COMPOUND_STRING(", all at Lv. 100"));
    else
        StringCopy(end, COMPOUND_STRING(", own levels"));
    end = StringCopy(gStringVar3, size == COOP_RULE_SIZE_3 ? COMPOUND_STRING("3") : size == COOP_RULE_SIZE_4 ? COMPOUND_STRING("4") : COMPOUND_STRING("Up to 6"));
    end = StringCopy(end, COMPOUND_STRING(" POKéMON, "));
    StringCopy(end, (rules & COOP_RULE_OPEN) ? COMPOUND_STRING("UNRESTRICTED") : COMPOUND_STRING("no repeats"));
    return TRUE;
}

static u8 Deal_TeamSize(u8 rules)
{
    switch ((rules >> COOP_RULE_SIZE_SHIFT) & 3)
    {
    case COOP_RULE_SIZE_3:
        return 3;
    case COOP_RULE_SIZE_4:
        return 4;
    default:
        return PARTY_SIZE;
    }
}

static bool8 Deal_MonEligible(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG);
}

// The standard clauses: no species and no held item twice. TRUE when the
// chosen team (sTeamOrder) keeps them, or the rules are unrestricted.
bool8 CoopDeal_TeamClauses(void)
{
    u8 i, j;

    if (gCoopFriendlyRules & COOP_RULE_OPEN)
        return TRUE;
    for (i = 0; i < sTeamCount; i++)
    {
        struct Pokemon *a = &gParties[B_TRAINER_PLAYER][sTeamOrder[i]];
        u16 item = GetMonData(a, MON_DATA_HELD_ITEM);

        for (j = i + 1; j < sTeamCount; j++)
        {
            struct Pokemon *b = &gParties[B_TRAINER_PLAYER][sTeamOrder[j]];

            if (GetMonData(a, MON_DATA_SPECIES) == GetMonData(b, MON_DATA_SPECIES))
                return FALSE;
            if (item != ITEM_NONE && item == GetMonData(b, MON_DATA_HELD_ITEM))
                return FALSE;
        }
    }
    return TRUE;
}

// VAR_RESULT TRUE: the rules ask for 3 or 4 of a bigger party, so the
// picker opens. FALSE: the whole party, as arranged, is the team.
u16 CoopDeal_TeamNeedsPicker(void)
{
    u8 i, eligible = 0;

    gCoopFriendlyRules = sDeal.rules;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (Deal_MonEligible(&gParties[B_TRAINER_PLAYER][i]))
            eligible++;
    }
    return Deal_TeamSize(sDeal.rules) < PARTY_SIZE && eligible > Deal_TeamSize(sDeal.rules);
}

static void CB2_ReturnFromTeamPicker(void)
{
    VarSet(VAR_FRONTIER_FACILITY, sSavedFacility);
    gSpecialVar_Result = gSelectedOrderFromParty[0] != 0;
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

void CoopDeal_OpenTeamPicker(void)
{
    sSavedFacility = VarGet(VAR_FRONTIER_FACILITY);
    VarSet(VAR_FRONTIER_FACILITY, FACILITY_COOP_FRIENDLY);
    gSpecialVar_0x8005 = Deal_TeamSize(sDeal.rules);
    gMain.savedCallback = CB2_ReturnFromTeamPicker;
    InitChooseHalfPartyForBattle(0);
}

// Builds sTeamOrder from the picker (VAR_0x8004 = 1) or from the whole party
// (0). VAR_RESULT: COOP_DEAL_OK, INVALID (clauses), or NO_ROOM (too few).
u16 CoopDeal_ValidateTeam(void)
{
    u8 i, need = (sDeal.rules & COOP_RULE_DOUBLES) ? 2 : 1;

    sTeamCount = 0;
    if (gSpecialVar_0x8004)
    {
        for (i = 0; i < MAX_FRONTIER_PARTY_SIZE && i < Deal_TeamSize(sDeal.rules); i++)
        {
            if (gSelectedOrderFromParty[i] == 0)
                break;
            sTeamOrder[sTeamCount++] = gSelectedOrderFromParty[i] - 1;
        }
    }
    else
    {
        for (i = 0; i < PARTY_SIZE && sTeamCount < Deal_TeamSize(sDeal.rules); i++)
        {
            if (Deal_MonEligible(&gParties[B_TRAINER_PLAYER][i]))
                sTeamOrder[sTeamCount++] = i;
        }
    }
    if (sTeamCount < need)
        return COOP_DEAL_NO_ROOM;
    if (!CoopDeal_TeamClauses())
        return COOP_DEAL_INVALID;
    return COOP_DEAL_OK;
}

u16 CoopDeal_TeamReady(void)
{
    if (sDeal.state != DS_TEAM)
        return COOP_DEAL_CANCELLED;
    Deal_Send(DMSG_TEAM, 0);
    return COOP_DEAL_OK;
}

static void Deal_ScaleMon(struct Pokemon *mon, u8 level)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES);
    u32 exp = gExperienceTables[gSpeciesInfo[species].growthRate][level];

    SetMonData(mon, MON_DATA_EXP, &exp);
    CalculateMonStats(mon);
}

static void Deal_RestoreFriendlyParty(void)
{
    if (!sFriendlyHasSnap)
        return;
    memcpy(gParties[B_TRAINER_PLAYER], sFriendlySnap, sizeof(sFriendlySnap));
    CalculatePlayerPartyCount();
    sFriendlyHasSnap = FALSE;
}

static void CB2_EndFriendlyBattle(void)
{
    sFriendlyAborting = FALSE;
    sFriendlyOutcome = gBattleOutcome;
    gBattleTypeFlags &= ~BATTLE_TYPE_LINK_IN_BATTLE;
    Deal_RestoreFriendlyParty();
    gCoopFriendlyBattle = FALSE;
    gCoopFriendlyRules = 0;
    Deal_Reset();
    Overworld_ResetMapMusic();
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

// A cable error mid-fight: no result, the real party back, the field again.
void CoopDeal_AbortFriendlyBattle(void)
{
    // Called every frame while the error stands: start the exit once.
    if (!gCoopFriendlyBattle || sFriendlyAborting)
        return;
    sFriendlyAborting = TRUE;
    gBattleOutcome = B_OUTCOME_LINK_BATTLE_RAN;
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gMain.savedCallback = CB2_EndFriendlyBattle;
    SetMainCallback2(CB2_QuitRecordedBattle);
}

// The chosen team, copied, healed, and moved to the given level for the
// fight only. The real party comes back afterwards whatever happens.
void CoopDeal_StartFriendlyBattle(void)
{
    struct Pokemon team[PARTY_SIZE];
    u8 i, level = (sDeal.rules >> COOP_RULE_LEVEL_SHIFT) & 3;

    memcpy(sFriendlySnap, gParties[B_TRAINER_PLAYER], sizeof(sFriendlySnap));
    sFriendlyHasSnap = TRUE;
    memset(team, 0, sizeof(team));
    for (i = 0; i < sTeamCount; i++)
    {
        team[i] = sFriendlySnap[sTeamOrder[i]];
        if (level == COOP_RULE_LEVEL_50)
            Deal_ScaleMon(&team[i], 50);
        else if (level == COOP_RULE_LEVEL_100)
            Deal_ScaleMon(&team[i], MAX_LEVEL);
        HealPokemon(&team[i]);
    }
    memcpy(gParties[B_TRAINER_PLAYER], team, sizeof(team));
    CalculatePlayerPartyCount();

    gCoopFriendlyBattle = TRUE;
    gCoopFriendlyRules = sDeal.rules;
    sFriendlyOutcome = 0;
    gBattleTypeFlags = BATTLE_TYPE_LINK | BATTLE_TYPE_TRAINER;
    if (sDeal.rules & COOP_RULE_DOUBLES)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_LINK_OPPONENT;
    TRAINER_BATTLE_PARAM.opponentB = TRAINER_NONE;
    // Two players, one side each: battler ids follow the link ids, as in
    // the cable club. (Co-op fights reassign them for the two-human side.)
    gLinkPlayers[0].id = 0;
    gLinkPlayers[1].id = 1;
    gMain.savedCallback = CB2_EndFriendlyBattle;
    BattleSetup_StartCoopFriendlyBattle();
    ScriptContext_Stop();
}

// VAR_RESULT: 1 won, 2 lost, 3 draw, 0 called off.
u16 CoopDeal_BattleOutcome(void)
{
    switch (sFriendlyOutcome)
    {
    case B_OUTCOME_WON:
        return 1;
    case B_OUTCOME_LOST:
        return 2;
    case B_OUTCOME_DREW:
        return 3;
    default:
        return 0;
    }
}

// A deal or dex block, checked the way its handler checks it. Used to keep
// these out of a link battle's block reads (both are resent if lost).
bool8 CoopDeal_IsLinkBlock(const u16 *words)
{
    if (words[0] == COOP_DEAL_MAGIC)
        return ((const struct CoopDealMsg *)words)->checksum == Deal_Checksum((const struct CoopDealMsg *)words);
    if (words[0] == COOP_DEX_MAGIC)
        return ((const struct CoopDexMsg *)words)->checksum == Dex_Checksum((const struct CoopDexMsg *)words);
    return FALSE;
}
