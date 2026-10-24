#ifndef GUARD_COOP_PROTOCOL_H
#define GUARD_COOP_PROTOCOL_H

// Host-testable 8-word co-op frame. The GBA link already checksums the cable
// frame. Word 7 is this layer's checksum so a bad command can be dropped
// without tearing the session down.
//
// Word 0: COOP_LINKCMD, or LINKCMD_SEND_HELD_KEYS when packed as the fallback.
// Word 1: build id folded to 16 bits.
// Word 2: map group in the high byte, map number in the low byte.
// Word 3: x in the high byte, y in the low byte.
// Word 4: facing (bits 0-2), acro bike (bit 3), level cap (bit 4),
//         request (bits 5-8), gender (bit 9), bike (bit 10), surf (bit 11),
//         slot (bits 12-15).
// Word 5: joint badges in the low byte, rival stage in bits 8-11,
//         sequence in bits 12-15.
// Word 6: the NPC being talked to in the low byte (0xFF means the Birch bag
//         screen), three 2-bit legendary owners in bits 8-13.
// Word 7: checksum of words 1-6.

#ifdef COOP_HOST_TEST
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
#else
#include "global.h"
#endif

#define COOP_BUILD_ID  0xC017012Bu
// gpSP's Pokémon Gen 3 cable (serial_proto.c MAX_FRAME_TIMEOUT) drops a peer
// after 240 frames with no packet. The overworld uses the same count.
#define COOP_LINK_QUIET_LIMIT 240

#define COOP_CAT_SOLO 0
#define COOP_CAT_OPTIONAL 1
#define COOP_CAT_MANDATORY 2
#define COOP_BUILD_XOR ((u16)(((COOP_BUILD_ID) & 0xFFFFu) ^ ((COOP_BUILD_ID) >> 16)))
#define COOP_LINKCMD   0xC0A1
#define COOP_HELD_KEYS_CMD 0xCAFE

#define COOP_REQ_NONE      0
#define COOP_REQ_TRADE     2
#define COOP_REQ_TRADE_YES 3
#define COOP_REQ_TALK      4
#define COOP_REQ_ACCEPT    5
#define COOP_REQ_CANCEL    6
#define COOP_REQ_CASE      7
#define COOP_REQ_CLAIM     8
#define COOP_REQ_BEACON    9
#define COOP_REQ_BEACON_YES 10
#define COOP_REQ_BEACON_NO  11
#define COOP_REQ_RESCUE     12

#define COOP_BEACON_IDLE    0
#define COOP_BEACON_OFFER   1
#define COOP_BEACON_CONFIRM 2
#define COOP_BEACON_DECLINE 3

#define COOP_BEACON_EVT_NONE     0
#define COOP_BEACON_EVT_PROMPT   1
#define COOP_BEACON_EVT_APPLIED  2
#define COOP_BEACON_EVT_DECLINED 3

#define COOP_UNPACK_DROP  0
#define COOP_UNPACK_OK    1
#define COOP_UNPACK_BUILD 2

#define COOP_BATTLE_SLOTS  3
#define COOP_ROSTER_SLOTS  6

#define COOP_OWNER_NONE    0
#define COOP_OWNER_LOCAL   1
#define COOP_OWNER_PARTNER 2

struct CoopFrame
{
    u16 buildXor;
    u8 mapGroup;
    u8 mapNum;
    u8 x;
    u8 y;
    u8 facing;
    u8 acro;
    u8 capOn;
    u8 request;
    u8 gender;
    u8 bike;
    u8 surf;
    u8 slot;
    u8 badges;
    u8 rivalStage;
    u8 seq;
    u8 talkLocalId;
    u8 legend[3];
};

struct CoopProgress
{
    u8 badges;
    u8 rivalStage;
    u8 capOn;
    u8 legend[3];
};

struct CoopMonState
{
    u16 species;
    u16 hp;
    u8 isEgg;
};

struct CoopBeaconState
{
    u8 phase;
    u8 on;
    u8 want;
    u8 request;
    u8 slot;
    u8 event;
};

static inline u16 CoopProtocol_Checksum(const u16 *words)
{
    u16 sum = COOP_LINKCMD;
    u16 i;

    for (i = 1; i <= 6; i++)
        sum = (u16)((sum + words[i]) ^ (u16)(words[i] << 1));
    return sum;
}

static inline int CoopProtocol_MapOk(u8 mapGroup, u8 mapNum)
{
    return mapGroup < 64 && mapNum < 128;
}

static inline void CoopProtocol_Pack(u16 out[8], const struct CoopFrame *frame, int asHeldKeys)
{
    u16 flags = 0;
    u16 legend = 0;

    flags |= (u16)(frame->facing & 7);
    flags |= (u16)(frame->acro & 1) << 3;
    flags |= (u16)(frame->capOn & 1) << 4;
    flags |= (u16)(frame->request & 0xF) << 5;
    flags |= (u16)(frame->gender & 1) << 9;
    flags |= (u16)(frame->bike & 1) << 10;
    flags |= (u16)(frame->surf & 1) << 11;
    flags |= (u16)(frame->slot & 0xF) << 12;

    legend |= (u16)(frame->legend[0] & 3);
    legend |= (u16)(frame->legend[1] & 3) << 2;
    legend |= (u16)(frame->legend[2] & 3) << 4;

    out[0] = asHeldKeys ? COOP_HELD_KEYS_CMD : COOP_LINKCMD;
    out[1] = frame->buildXor;
    out[2] = (u16)((frame->mapGroup << 8) | frame->mapNum);
    out[3] = (u16)((frame->x << 8) | frame->y);
    out[4] = flags;
    out[5] = (u16)(frame->badges | ((frame->rivalStage & 0xF) << 8) | ((frame->seq & 0xF) << 12));
    out[6] = (u16)(frame->talkLocalId | (legend << 8));
    out[7] = CoopProtocol_Checksum(out);
}

static inline int CoopProtocol_Unpack(const u16 in[8], struct CoopFrame *frame, u16 expectedBuild)
{
    u16 words[8];
    u16 flags;
    u16 legend;
    int i;

    if (in[0] != COOP_LINKCMD && in[0] != COOP_HELD_KEYS_CMD)
        return COOP_UNPACK_DROP;

    for (i = 0; i < 8; i++)
        words[i] = in[i];
    if (in[0] == COOP_HELD_KEYS_CMD)
        words[0] = COOP_LINKCMD;

    if (CoopProtocol_Checksum(words) != in[7])
        return COOP_UNPACK_DROP;

    frame->buildXor = in[1];
    frame->mapGroup = (u8)(in[2] >> 8);
    frame->mapNum = (u8)(in[2] & 0xFF);
    frame->x = (u8)(in[3] >> 8);
    frame->y = (u8)(in[3] & 0xFF);
    flags = in[4];
    frame->facing = (u8)(flags & 7);
    frame->acro = (u8)((flags >> 3) & 1);
    frame->capOn = (u8)((flags >> 4) & 1);
    frame->request = (u8)((flags >> 5) & 0xF);
    frame->gender = (u8)((flags >> 9) & 1);
    frame->bike = (u8)((flags >> 10) & 1);
    frame->surf = (u8)((flags >> 11) & 1);
    frame->slot = (u8)((flags >> 12) & 0xF);
    frame->badges = (u8)(in[5] & 0xFF);
    frame->rivalStage = (u8)((in[5] >> 8) & 0xF);
    frame->seq = (u8)((in[5] >> 12) & 0xF);
    frame->talkLocalId = (u8)(in[6] & 0xFF);
    legend = (u16)(in[6] >> 8);
    frame->legend[0] = (u8)(legend & 3);
    frame->legend[1] = (u8)((legend >> 2) & 3);
    frame->legend[2] = (u8)((legend >> 4) & 3);

    if (!CoopProtocol_MapOk(frame->mapGroup, frame->mapNum))
        return COOP_UNPACK_DROP;
    if (frame->buildXor != expectedBuild)
        return COOP_UNPACK_BUILD;
    return COOP_UNPACK_OK;
}

// Remote owner 1 means the other save has the Pokémon, which is owner 2 here.
static inline int CoopProtocol_Merge(const struct CoopProgress *local, const struct CoopProgress *remote, struct CoopProgress *out)
{
    int i;

    if (local->capOn != remote->capOn)
        return 1;

    out->capOn = local->capOn;
    out->badges = (u8)(local->badges | remote->badges);
    out->rivalStage = local->rivalStage > remote->rivalStage ? local->rivalStage : remote->rivalStage;

    for (i = 0; i < 3; i++)
    {
        u8 mine = local->legend[i];
        u8 theirs = remote->legend[i];
        u8 theirsHere = theirs == COOP_OWNER_LOCAL ? COOP_OWNER_PARTNER : theirs == COOP_OWNER_PARTNER ? COOP_OWNER_LOCAL : COOP_OWNER_NONE;

        if (mine == COOP_OWNER_NONE)
            out->legend[i] = theirsHere;
        else if (theirsHere == COOP_OWNER_NONE || mine == theirsHere)
            out->legend[i] = mine;
        else
            return 2;
    }
    return 0;
}

// Elite Four and champion rooms walk both players up to the same NPC, so
// both games can ask for the same mandatory fight at once. Link player 0
// keeps its own invite; the other game takes player 0's invite as guest.
static inline int CoopProtocol_KeepOwnInvite(u8 myLinkId, u8 waitingAck, u8 ownMandatory, u8 incomingMandatory)
{
    return myLinkId == 0 && waitingAck && ownMandatory && incomingMandatory;
}

// A story var only moves forward. A guest who is already past this scene
// keeps the later state.
static inline u16 CoopProtocol_RaiseVar(u16 current, u16 floor)
{
    return current > floor ? current : floor;
}

// placeSolo: Battle Frontier, Safari, Trainer Hill, Battle Pyramid.
// couple: a trainer who is already two opponents on one save.
// mandatory: gyms, rivals, villains, Elite Four, champion, legendaries.
static inline int CoopProtocol_TrainerCategory(int placeSolo, int mandatory, int couple)
{
    if (placeSolo || couple)
        return COOP_CAT_SOLO;
    if (mandatory)
        return COOP_CAT_MANDATORY;
    return COOP_CAT_OPTIONAL;
}

// Mandatory fights ignore the Twin Beacon. Optional fights need it.
// Nothing queues across maps. Solo never queues.
static inline int CoopProtocol_QueuesCoop(int category, int beaconOn, int sameMap)
{
    if (!sameMap)
        return 0;
    if (category == COOP_CAT_MANDATORY)
        return 1;
    if (category == COOP_CAT_OPTIONAL && beaconOn)
        return 1;
    return 0;
}

// Stock link setup stamps BATTLE_TYPE_TRAINER onto every link battle.
// A co-op wild fight must stay a wild fight, or the two games disagree
// about catches, parties, and who the opponents are.
static inline u32 CoopProtocol_KeepWildBattle(u32 flagsBefore, u32 flagsAfter)
{
    // Bit 14 is BATTLE_TYPE_COOP. Bit 3 is BATTLE_TYPE_TRAINER.
    if ((flagsBefore & (1u << 14)) && !(flagsBefore & (1u << 3)))
        return flagsAfter & ~(1u << 3);
    return flagsAfter;
}

// Mum's TV scene walks both players onto one tile. Drawn there, the partner
// is hidden inside the local player. Step them one tile south instead.
static inline void CoopProtocol_VisiblePartnerTile(s16 playerX, s16 playerY, s16 *x, s16 *y)
{
    if (*x == playerX && *y == playerY)
        (*y)++;
}

// One trainer id never stands in for two. A repeated id is dropped.
static inline int CoopProtocol_DistinctPartner(int trainerA, int trainerB)
{
    if (trainerB == 0 || trainerB == 0xFFFF || trainerB == trainerA)
        return 0;
    return trainerB;
}

// Stage 1. The partner sprite exists only while this is true. A door changes
// the map number or group, so the sprite drops until both players are inside.
static inline int CoopProtocol_SameMap(u8 groupA, u8 numA, u8 groupB, u8 numB)
{
    return groupA == groupB && numA == numB;
}

static inline void CoopProtocol_BeaconOffer(struct CoopBeaconState *state, u8 want)
{
    if (state->phase != COOP_BEACON_IDLE)
        return;
    state->want = want ? 1 : 0;
    state->phase = COOP_BEACON_OFFER;
    state->request = COOP_REQ_BEACON;
    state->slot = state->want;
    state->event = COOP_BEACON_EVT_NONE;
}

static inline void CoopProtocol_BeaconAnswer(struct CoopBeaconState *state, int yes)
{
    if (yes)
    {
        state->phase = COOP_BEACON_CONFIRM;
        state->request = COOP_REQ_BEACON_YES;
        state->slot = state->want;
    }
    else
    {
        state->phase = COOP_BEACON_DECLINE;
        state->request = COOP_REQ_BEACON_NO;
        state->event = COOP_BEACON_EVT_DECLINED;
    }
}

// Beacon flips only after both sides are already sending YES. A local yes,
// and a yes that has not been echoed yet, leave `on` alone. `busy` freezes
// the state so a gym or trade finishes under the mode it started with.
static inline void CoopProtocol_BeaconStep(struct CoopBeaconState *state, u8 partnerRequest, u8 partnerSlot, int busy)
{
    state->event = COOP_BEACON_EVT_NONE;
    if (busy)
        return;

    if (state->phase == COOP_BEACON_OFFER && partnerRequest == COOP_REQ_BEACON_NO)
    {
        state->phase = COOP_BEACON_IDLE;
        state->request = COOP_REQ_NONE;
        state->event = COOP_BEACON_EVT_DECLINED;
        return;
    }
    if (state->phase == COOP_BEACON_OFFER
        && partnerRequest == COOP_REQ_BEACON
        && partnerSlot != state->want)
    {
        state->phase = COOP_BEACON_DECLINE;
        state->request = COOP_REQ_BEACON_NO;
        state->event = COOP_BEACON_EVT_DECLINED;
        return;
    }
    if (state->phase == COOP_BEACON_OFFER
        && ((partnerRequest == COOP_REQ_BEACON && partnerSlot == state->want)
            || (partnerRequest == COOP_REQ_BEACON_YES && partnerSlot == state->want)))
    {
        state->phase = COOP_BEACON_CONFIRM;
        state->request = COOP_REQ_BEACON_YES;
        state->slot = state->want;
        return;
    }
    if (state->phase == COOP_BEACON_CONFIRM
        && partnerRequest == COOP_REQ_BEACON_YES
        && partnerSlot == state->want)
    {
        state->on = state->want;
        state->phase = COOP_BEACON_IDLE;
        state->request = COOP_REQ_NONE;
        state->event = COOP_BEACON_EVT_APPLIED;
        return;
    }
    if (state->phase == COOP_BEACON_DECLINE && partnerRequest != COOP_REQ_BEACON)
    {
        state->phase = COOP_BEACON_IDLE;
        state->request = COOP_REQ_NONE;
        return;
    }
    if (state->phase == COOP_BEACON_IDLE && partnerRequest == COOP_REQ_BEACON)
    {
        state->want = partnerSlot ? 1 : 0;
        state->event = COOP_BEACON_EVT_PROMPT;
    }
}

// Stage 3. A co-op battle needs one conscious, hatched Pokémon in slots 1-3.
// A healthy fourth to sixth slot does not count; it never reaches the field.
static inline int CoopProtocol_LeadReady(const struct CoopMonState *party)
{
    int i;

    for (i = 0; i < COOP_BATTLE_SLOTS; i++)
    {
        if (party[i].species != 0 && !party[i].isEgg && party[i].hp != 0)
            return 1;
    }
    return 0;
}

// A finished battle keeps the outcome for slots 1-3 and puts the pre-battle
// copy back in slots 4-6. An aborted battle puts the pre-battle copy back
// everywhere.
static inline int CoopProtocol_RestoreFromSnapshot(u8 slot, int aborted)
{
    if (slot >= COOP_ROSTER_SLOTS)
        return 0;
    return aborted || slot >= COOP_BATTLE_SLOTS;
}

// Stage 4. One owner per wild slot. A ball at a slot that already has an
// owner, or that has left the field, is spent and catches nothing.
#define COOP_WILD_SLOTS 2
#define COOP_THROWER_NONE 0

struct CoopCatchLedger
{
    u8 owner[COOP_WILD_SLOTS];
};

static inline void CoopProtocol_ClearCatches(struct CoopCatchLedger *ledger)
{
    int i;

    for (i = 0; i < COOP_WILD_SLOTS; i++)
        ledger->owner[i] = COOP_THROWER_NONE;
}

static inline int CoopProtocol_CanCatch(const struct CoopCatchLedger *ledger, u8 wildSlot, int targetOnField)
{
    if (wildSlot >= COOP_WILD_SLOTS || !targetOnField)
        return 0;
    return ledger->owner[wildSlot] == COOP_THROWER_NONE;
}

// `thrower` is 1 + the thrower's multiplayer id, so 0 stays "nobody".
static inline int CoopProtocol_ClaimCatch(struct CoopCatchLedger *ledger, u8 wildSlot, u8 thrower, int targetOnField)
{
    if (thrower == COOP_THROWER_NONE || !CoopProtocol_CanCatch(ledger, wildSlot, targetOnField))
        return 0;
    ledger->owner[wildSlot] = thrower;
    return 1;
}

static inline int CoopProtocol_CatchCount(const struct CoopCatchLedger *ledger, u8 thrower)
{
    int i, count = 0;

    for (i = 0; i < COOP_WILD_SLOTS; i++)
    {
        if (thrower != COOP_THROWER_NONE && ledger->owner[i] == thrower)
            count++;
    }
    return count;
}

// A co-op wild fight ends by running only when every human who still has
// a Pokémon up chose Run this turn. One Run alone is spent and the fight goes on.
static inline int CoopProtocol_FleeAgreed(const u8 *choseRun, const u8 *canAct, int count)
{
    int i, acting = 0;

    for (i = 0; i < count; i++)
    {
        if (!canAct[i])
            continue;
        acting++;
        if (!choseRun[i])
            return 0;
    }
    return acting != 0;
}

// The party left the battle at three, so room counts against the full party
// from before the fight. Catches already made this fight hold their space.
static inline int CoopProtocol_CatchRoom(u8 partyCount, u8 pendingCatches, u16 storageFree)
{
    u32 free = storageFree;

    if (partyCount < COOP_ROSTER_SLOTS)
        free += (u32)(COOP_ROSTER_SLOTS - partyCount);
    return pendingCatches < free;
}

// Stage 5. A trainer pair fights both humans only when neither id is beaten
// on either save, the partner is free, and both players said yes. Any other
// case is the stock solo fight against the NPC that was walked into.
#define COOP_PAIR_SOLO  0
#define COOP_PAIR_JOINT 1

// Route 102: Rick stands on the left of the pair and Allen on the right.
// coop.c checks these against the trainer constants.
#define COOP_PAIR_LEFT_ID  615
#define COOP_PAIR_RIGHT_ID 333

static inline int CoopProtocol_PairPlan(const u8 localBeat[2], const u8 remoteBeat[2], int partnerFree, int bothAccept)
{
    if (localBeat[0] || localBeat[1] || remoteBeat[0] || remoteBeat[1])
        return COOP_PAIR_SOLO;
    if (!partnerFree || !bothAccept)
        return COOP_PAIR_SOLO;
    return COOP_PAIR_JOINT;
}

// The flags a finished pair fight leaves on one save. Only a win marks the
// ids that were in the fight; a loss or a dropped cable marks nothing.
static inline void CoopProtocol_PairFlagsAfter(u8 beat[2], const u8 fought[2], int won)
{
    int i;

    if (!won)
        return;
    for (i = 0; i < 2; i++)
    {
        if (fought[i])
            beat[i] = 1;
    }
}

#endif // GUARD_COOP_PROTOCOL_H
