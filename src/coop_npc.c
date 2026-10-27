// Shared NPC movement for two linked players on the same map.
//
// One game is the movement authority for the map: its random walkers and
// turners run as usual, and every step or turn they start is sent as a small
// frame (map, local id, start tile, movement action). The other game stops
// those objects' own movement and replays the moves on the same objects,
// found by their stable local id on that map (never by object slot).
//
// Authority: whoever was on the map first keeps it until they leave; the
// other player then carries on from where the objects stand (nothing is
// reset). Two arrivals at once, or a link that comes up with both already
// there, go to link player 0. A lost link hands every object back to its
// own game.
//
// Not synced (each game runs its own): trainers, objects scripts move or
// hide, followers, copy-the-player and in-place movers, objects that only
// one save has. A script running on either side pauses replay; moves that
// arrive meanwhile are replayed afterwards, or the object is placed where
// the authority has it when too many were missed.
//
// Talking: the player who talks to a synced NPC reserves it. The authority
// holds it still, and the other player gets a short "busy" answer.
//
// Frames use the field heartbeat's slot, at most every other frame, so the
// block channel (battles, trades, invites) is never used or delayed.
#include "global.h"
#include "coop.h"
#include "coop_npc.h"
#include "coop_protocol.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "link.h"
#include "main.h"
#include "overworld.h"
#include "script.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/trainer_types.h"

#define NPC_MARK        0x4E50 // 'NP', xored into the build word
#define NPC_MSG_STEP    1
#define NPC_MSG_POS     2
#define NPC_MSG_HELLO   3
#define NPC_MSG_RESERVE 4
#define NPC_MSG_RELEASE 5

#define NPC_OUT_SIZE    24
#define NPC_PENDING     4
#define NPC_REFRESH     40  // frames between authority refreshes (one object each)
#define NPC_BEACON      120 // authority beacon when no object is synced
#define NPC_HELLO_RETRY 180
#define NPC_ARRIVE_TIE  12  // arrivals this close count as simultaneous
#define NPC_BLOCK_WAIT  90  // frames a replay waits for the local player to move

struct NpcMsg
{
    u8 type;
    u8 localId;
    u8 x;
    u8 y;
    u8 action;
    u8 facing;
};

struct NpcRemote
{
    u8 localId;    // 0: this slot runs its own movement
    u8 count;
    u8 wait;
    bool8 heldByUs;
    struct NpcMsg pending[NPC_PENDING];
};

struct NpcState
{
    u8 role;
    u8 mapGroup;
    u8 mapNum;
    bool8 shared;
    u8 partnerGroup;
    u8 partnerNum;
    bool8 partnerKnown;
    u8 parity;
    u16 frame;
    u16 arriveFrame;
    u16 partnerArriveFrame;
    u16 sendSeq;
    u16 recvSeq;
    bool8 recvSeqKnown;
    u8 outHead;
    u8 outCount;
    u8 refreshSlot;
    u16 refreshTimer;
    u16 helloTimer;
    u8 partnerTalk;
    u16 partnerTalkIdle;
    u8 myTalk;
    struct NpcMsg out[NPC_OUT_SIZE];
    struct NpcRemote remote[OBJECT_EVENTS_COUNT];
};

static EWRAM_DATA struct NpcState sNpc = {0};
EWRAM_DATA struct CoopNpcStats gCoopNpcStats = {0};

static void ReleaseAll(void);

static bool8 IsSyncMovementType(u8 type)
{
    if (type >= MOVEMENT_TYPE_LOOK_AROUND && type <= MOVEMENT_TYPE_WANDER_RIGHT_AND_LEFT)
        return TRUE;
    if (type >= MOVEMENT_TYPE_FACE_DOWN_AND_UP && type <= MOVEMENT_TYPE_WALK_SEQUENCE_RIGHT_DOWN_LEFT_UP)
        return TRUE;
    return type == MOVEMENT_TYPE_WANDER_AROUND_SLOWER;
}

static bool8 IsSyncObject(const struct ObjectEvent *obj)
{
    return obj->active && !obj->isPlayer && obj->localId != 0
        && obj->localId < OBJ_EVENT_ID_DYNAMIC_BASE
        && obj->trainerType == TRAINER_TYPE_NONE
        && IsSyncMovementType(obj->movementType);
}

// Face and one-tile walk actions only: what the synced movement types use.
static bool8 ActionIsFace(u8 action)
{
    return action <= MOVEMENT_ACTION_FACE_RIGHT;
}

static s8 ActionWalkDir(u8 action)
{
    if (action >= MOVEMENT_ACTION_WALK_SLOW_DOWN && action <= MOVEMENT_ACTION_WALK_NORMAL_RIGHT)
        return (action - MOVEMENT_ACTION_WALK_SLOW_DOWN) % 4 + 1;
    if (action >= MOVEMENT_ACTION_WALK_FAST_DOWN && action <= MOVEMENT_ACTION_WALK_FAST_RIGHT)
        return action - MOVEMENT_ACTION_WALK_FAST_DOWN + 1;
    if (action >= MOVEMENT_ACTION_WALK_SLOW_STAIRS_DOWN && action <= MOVEMENT_ACTION_WALK_SLOW_STAIRS_RIGHT)
        return action - MOVEMENT_ACTION_WALK_SLOW_STAIRS_DOWN + 1;
    return 0;
}

static u16 NpcChecksum(const u16 *w)
{
    u16 sum = 0x5EED;
    u32 i;

    for (i = 1; i <= 6; i++)
        sum = (u16)((sum + w[i]) ^ (u16)(w[i] << 3) ^ (u16)i);
    return sum;
}

static void Enqueue(u8 type, u8 localId, u8 x, u8 y, u8 action, u8 facing)
{
    struct NpcMsg *msg;

    if (sNpc.outCount >= NPC_OUT_SIZE)
    {
        // Oldest goes; the refresh and the receiver's realignment cover it.
        sNpc.outHead = (sNpc.outHead + 1) % NPC_OUT_SIZE;
        sNpc.outCount--;
        if (gCoopNpcStats.dropped < 0xFFFF)
            gCoopNpcStats.dropped++;
    }
    msg = &sNpc.out[(sNpc.outHead + sNpc.outCount) % NPC_OUT_SIZE];
    msg->type = type;
    msg->localId = localId;
    msg->x = x;
    msg->y = y;
    msg->action = action;
    msg->facing = facing;
    sNpc.outCount++;
}

static void EnqueuePos(const struct ObjectEvent *obj)
{
    Enqueue(NPC_MSG_POS, obj->localId, obj->currentCoords.x, obj->currentCoords.y,
            MOVEMENT_ACTION_NONE, obj->facingDirection);
}

static void ClearOutgoing(void)
{
    sNpc.outHead = 0;
    sNpc.outCount = 0;
}

static void SetRole(u8 role)
{
    if (sNpc.role == role)
        return;
    if (sNpc.role == COOP_NPC_ROLE_FOLLOW)
        ReleaseAll();
    sNpc.role = role;
    gCoopNpcStats.role = role;
    if (gCoopNpcStats.roleChanges < 0xFFFF)
        gCoopNpcStats.roleChanges++;
    sNpc.refreshTimer = 0;
    sNpc.refreshSlot = 0;
    sNpc.helloTimer = 0;
    if (role == COOP_NPC_ROLE_FOLLOW)
    {
        ClearOutgoing();
        Enqueue(NPC_MSG_HELLO, 0, 0, 0, 0, 0);
    }
    else if (role == COOP_NPC_ROLE_AUTH)
    {
        u32 i;

        // Announce every object as it stands now.
        for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        {
            if (IsSyncObject(&gObjectEvents[i]))
                EnqueuePos(&gObjectEvents[i]);
        }
    }
}

// ---- follower side -------------------------------------------------------

static void ReleaseSlot(u32 slot)
{
    struct NpcRemote *r = &sNpc.remote[slot];
    struct ObjectEvent *obj = &gObjectEvents[slot];

    if (r->localId != 0 && obj->active && obj->localId == r->localId)
    {
        if (r->heldByUs && obj->heldMovementActive)
            ObjectEventClearHeldMovementIfFinished(obj);
        // The object's own movement restarts from its first step, where it
        // stands: no jump back to where it was before.
        if (!obj->heldMovementActive)
            ClearObjectEventMovement(obj, &gSprites[obj->spriteId]);
    }
    memset(r, 0, sizeof(*r));
}

static void ReleaseAll(void)
{
    u32 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        ReleaseSlot(i);
    gCoopNpcStats.synced = 0;
}

static u8 SlotForLocalId(u8 localId)
{
    u8 slot;

    if (localId == 0 || localId >= OBJ_EVENT_ID_DYNAMIC_BASE)
        return OBJECT_EVENTS_COUNT;
    if (TryGetObjectEventIdByLocalIdAndMap(localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &slot))
        return OBJECT_EVENTS_COUNT;
    if (!IsSyncObject(&gObjectEvents[slot]))
        return OBJECT_EVENTS_COUNT;
    return slot;
}

static void AddPending(struct NpcRemote *r, const struct NpcMsg *msg)
{
    if (r->count >= NPC_PENDING)
    {
        // Too far behind: keep only where the authority says it is now.
        r->pending[0] = *msg;
        if (msg->type == NPC_MSG_STEP)
            r->pending[0].type = NPC_MSG_POS;
        r->count = 1;
        if (gCoopNpcStats.dropped < 0xFFFF)
            gCoopNpcStats.dropped++;
        return;
    }
    r->pending[r->count++] = *msg;
}

static void PopPending(struct NpcRemote *r)
{
    u32 i;

    for (i = 1; i < r->count; i++)
        r->pending[i - 1] = r->pending[i];
    if (r->count != 0)
        r->count--;
}

static bool8 LocalPlayerAt(s16 x, s16 y)
{
    struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];

    // currentCoords is already the destination while the player steps.
    return player->currentCoords.x == x && player->currentCoords.y == y;
}

static bool8 StartHeld(struct ObjectEvent *obj, struct NpcRemote *r, u8 action)
{
    if (obj->heldMovementActive)
    {
        if (!obj->heldMovementFinished)
            return FALSE;
        ObjectEventClearHeldMovementIfFinished(obj);
    }
    if (ObjectEventSetHeldMovement(obj, action))
        return FALSE;
    r->heldByUs = TRUE;
    return TRUE;
}

static s8 DirectionToward(s16 fromX, s16 fromY, s16 toX, s16 toY)
{
    if (toX < fromX)
        return DIR_WEST;
    if (toX > fromX)
        return DIR_EAST;
    if (toY < fromY)
        return DIR_NORTH;
    if (toY > fromY)
        return DIR_SOUTH;
    return 0;
}

static void PlaceAt(struct ObjectEvent *obj, const struct NpcMsg *msg)
{
    if (obj->heldMovementActive)
        ObjectEventClearHeldMovementIfFinished(obj);
    MoveObjectEventToMapCoords(obj, msg->x, msg->y);
    if (msg->facing >= DIR_SOUTH && msg->facing <= DIR_EAST)
        ObjectEventTurn(obj, msg->facing);
    if (gCoopNpcStats.snapped < 0xFFFF)
        gCoopNpcStats.snapped++;
}

// One step of replay for one object. Returns when it started something or
// has to wait.
static void ReplayObject(u32 slot)
{
    struct NpcRemote *r = &sNpc.remote[slot];
    struct ObjectEvent *obj = &gObjectEvents[slot];
    struct NpcMsg *msg;
    s16 dist;
    s8 dir;

    if (r->localId == 0)
        return;
    if (!obj->active || obj->localId != r->localId || !IsSyncObject(obj))
    {
        memset(r, 0, sizeof(*r));
        return;
    }
    // A script on this side owns the objects for now (a talk, a cutscene).
    if (obj->frozen || ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
    {
        if (r->count != 0 && gCoopNpcStats.deferred < 0xFFFF)
            gCoopNpcStats.deferred++;
        return;
    }
    if (obj->heldMovementActive && !obj->heldMovementFinished)
        return;
    if (obj->heldMovementActive && !r->heldByUs)
        return;
    if (r->count == 0)
        return;

    msg = &r->pending[0];
    dist = abs(obj->currentCoords.x - msg->x) + abs(obj->currentCoords.y - msg->y);
    if (dist != 0)
    {
        // Missed moves: walk back into line when it is close, otherwise place
        // it (a long gap or a reload).
        dir = DirectionToward(obj->currentCoords.x, obj->currentCoords.y, msg->x, msg->y);
        if (dist <= 2 && dir != 0)
        {
            s16 nx = obj->currentCoords.x, ny = obj->currentCoords.y;

            MoveCoords(dir, &nx, &ny);
            if (LocalPlayerAt(nx, ny))
            {
                if (++r->wait < NPC_BLOCK_WAIT)
                    return;
                r->wait = 0;
                PlaceAt(obj, msg);
                return;
            }
            r->wait = 0;
            if (StartHeld(obj, r, GetWalkNormalMovementAction(dir)) && gCoopNpcStats.corrected < 0xFFFF)
                gCoopNpcStats.corrected++;
            return;
        }
        PlaceAt(obj, msg);
        if (msg->type == NPC_MSG_POS)
            PopPending(r);
        return;
    }

    if (msg->type == NPC_MSG_POS)
    {
        if (msg->facing >= DIR_SOUTH && msg->facing <= DIR_EAST && obj->facingDirection != msg->facing)
            StartHeld(obj, r, GetFaceDirectionMovementAction(msg->facing));
        PopPending(r);
        return;
    }

    dir = ActionWalkDir(msg->action);
    if (dir != 0)
    {
        s16 nx = obj->currentCoords.x, ny = obj->currentCoords.y;

        MoveCoords(dir, &nx, &ny);
        if (LocalPlayerAt(nx, ny))
        {
            // This player is standing there here (the authority had not seen
            // it yet). Wait for them to move on, then carry on.
            if (gCoopNpcStats.deferred < 0xFFFF)
                gCoopNpcStats.deferred++;
            if (++r->wait < NPC_BLOCK_WAIT)
                return;
            // Still there: skip the step; the refresh realigns later.
            r->wait = 0;
            PopPending(r);
            return;
        }
    }
    r->wait = 0;
    if (StartHeld(obj, r, msg->action))
    {
        PopPending(r);
        if (gCoopNpcStats.applied < 0xFFFF)
            gCoopNpcStats.applied++;
    }
}

static void TakeOver(u32 slot, const struct NpcMsg *msg)
{
    struct NpcRemote *r = &sNpc.remote[slot];
    struct ObjectEvent *obj = &gObjectEvents[slot];

    if (r->localId == obj->localId)
    {
        AddPending(r, msg);
        return;
    }
    // Wait for a step of its own to finish; the object is never stopped
    // halfway between tiles. Keep the message for when it has.
    memset(r, 0, sizeof(*r));
    r->localId = obj->localId;
    r->pending[0] = *msg;
    r->count = 1;
}

static void HandleFollowerMsg(const struct NpcMsg *msg)
{
    u8 slot = SlotForLocalId(msg->localId);

    if (slot >= OBJECT_EVENTS_COUNT)
        return;
    if (msg->type == NPC_MSG_STEP && !ActionIsFace(msg->action) && ActionWalkDir(msg->action) == 0)
        return;
    TakeOver(slot, msg);
}

static void FollowerFrame(void)
{
    u32 i;
    u8 synced = 0;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct NpcRemote *r = &sNpc.remote[i];
        struct ObjectEvent *obj = &gObjectEvents[i];

        if (r->localId == 0)
            continue;
        if (!obj->active || obj->localId != r->localId)
        {
            memset(r, 0, sizeof(*r));
            continue;
        }
        // Taking over: only once its own step is done.
        if (obj->singleMovementActive)
            continue;
        if (!r->heldByUs && obj->heldMovementActive)
            continue; // a script is moving it
        synced++;
        ReplayObject(i);
    }
    gCoopNpcStats.synced = synced;
    // No snapshot yet: ask again.
    if (++sNpc.helloTimer >= NPC_HELLO_RETRY && synced == 0)
    {
        sNpc.helloTimer = 0;
        Enqueue(NPC_MSG_HELLO, 0, 0, 0, 0, 0);
    }
}

// ---- authority side ------------------------------------------------------

static void AuthorityFrame(void)
{
    u32 i, n;

    if (sNpc.refreshTimer++ < NPC_REFRESH)
        return;
    sNpc.refreshTimer = 0;
    // Round-robin: one object per refresh. It doubles as the authority's
    // beacon; with nothing to sync, a bare beacon goes out less often.
    for (n = 0; n < OBJECT_EVENTS_COUNT; n++)
    {
        i = (sNpc.refreshSlot + n) % OBJECT_EVENTS_COUNT;
        if (IsSyncObject(&gObjectEvents[i]) && !gObjectEvents[i].singleMovementActive)
        {
            EnqueuePos(&gObjectEvents[i]);
            sNpc.refreshSlot = i + 1;
            return;
        }
    }
    if ((sNpc.frame % NPC_BEACON) < NPC_REFRESH)
        Enqueue(NPC_MSG_POS, 0, 0, 0, 0, 0);
}

void CoopNpc_NoteSingleMovement(struct ObjectEvent *obj)
{
    u8 action;

    if (sNpc.role != COOP_NPC_ROLE_AUTH || !IsSyncObject(obj))
        return;
    action = obj->movementActionId;
    if (!ActionIsFace(action) && ActionWalkDir(action) == 0)
        return;
    Enqueue(NPC_MSG_STEP, obj->localId, obj->currentCoords.x, obj->currentCoords.y,
            action, obj->facingDirection);
}

bool8 CoopNpc_BlocksLocalMovement(struct ObjectEvent *obj)
{
    u32 slot;

    if (sNpc.role == COOP_NPC_ROLE_NONE)
        return FALSE;
    slot = obj - gObjectEvents;
    if (slot >= OBJECT_EVENTS_COUNT)
        return FALSE;
    if (sNpc.role == COOP_NPC_ROLE_FOLLOW)
        return sNpc.remote[slot].localId != 0 && sNpc.remote[slot].localId == obj->localId
            && !obj->singleMovementActive;
    // Authority: the NPC the partner is talking to waits for them.
    return sNpc.partnerTalk != 0 && obj->localId == sNpc.partnerTalk && IsSyncObject(obj)
        && !obj->singleMovementActive;
}

// ---- talking -------------------------------------------------------------

static void FacePartner(u8 localId)
{
    u8 slot = SlotForLocalId(localId);
    s16 px, py;
    struct ObjectEvent *obj;
    s8 dir;

    if (slot >= OBJECT_EVENTS_COUNT || !Coop_PartnerMapPos(&px, &py))
        return;
    obj = &gObjectEvents[slot];
    if (abs(obj->currentCoords.x - px) + abs(obj->currentCoords.y - py) != 1)
        return;
    dir = DirectionToward(obj->currentCoords.x, obj->currentCoords.y, px, py);
    if (dir != 0 && !obj->singleMovementActive && !obj->heldMovementActive)
        ObjectEventTurn(obj, dir);
}

const u8 *CoopNpc_InteractionScript(u8 objectEventId, const u8 *script)
{
    struct ObjectEvent *obj;

    if (objectEventId >= OBJECT_EVENTS_COUNT || sNpc.role == COOP_NPC_ROLE_NONE)
        return script;
    obj = &gObjectEvents[objectEventId];
    if (sNpc.partnerTalk != 0 && obj->localId == sNpc.partnerTalk && IsSyncObject(obj))
    {
        if (gCoopNpcStats.busyAnswers < 0xFFFF)
            gCoopNpcStats.busyAnswers++;
        return Coop_EventScript_NpcBusy;
    }
    return script;
}

void CoopNpc_NoteTalkStart(u8 objectEventId)
{
    struct ObjectEvent *obj;

    if (objectEventId >= OBJECT_EVENTS_COUNT || sNpc.role == COOP_NPC_ROLE_NONE)
        return;
    obj = &gObjectEvents[objectEventId];
    if (!IsSyncObject(obj) || obj->localId == sNpc.partnerTalk)
        return;
    sNpc.myTalk = obj->localId;
    gCoopNpcStats.myTalk = sNpc.myTalk;
    Enqueue(NPC_MSG_RESERVE, obj->localId, obj->currentCoords.x, obj->currentCoords.y, 0, obj->facingDirection);
}

u16 CoopNpc_BufferBusy(void)
{
    u8 id = GetMultiplayerId() ^ 1;

    StringCopyN(gStringVar1, gLinkPlayers[id].name, PLAYER_NAME_LENGTH);
    gStringVar1[PLAYER_NAME_LENGTH] = EOS;
    return TRUE;
}

static void EndMyTalkIfDone(void)
{
    u8 slot;

    if (sNpc.myTalk == 0 || ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
        return;
    slot = SlotForLocalId(sNpc.myTalk);
    if (slot < OBJECT_EVENTS_COUNT)
        Enqueue(NPC_MSG_RELEASE, sNpc.myTalk, gObjectEvents[slot].currentCoords.x,
                gObjectEvents[slot].currentCoords.y, 0, gObjectEvents[slot].facingDirection);
    else
        Enqueue(NPC_MSG_RELEASE, sNpc.myTalk, 0, 0, 0, 0);
    sNpc.myTalk = 0;
    gCoopNpcStats.myTalk = 0;
}

// ---- link ----------------------------------------------------------------

bool8 CoopNpc_TakeOutgoing(u16 w[8])
{
    struct NpcMsg *msg;

    // Every other frame at most: the position heartbeat keeps its rhythm.
    sNpc.parity ^= 1;
    if (sNpc.outCount == 0 || sNpc.parity == 0 || !sNpc.shared)
        return FALSE;
    msg = &sNpc.out[sNpc.outHead];
    sNpc.outHead = (sNpc.outHead + 1) % NPC_OUT_SIZE;
    sNpc.outCount--;
    w[0] = COOP_HELD_KEYS_CMD;
    w[1] = COOP_BUILD_XOR ^ NPC_MARK;
    w[2] = (u16)((sNpc.mapGroup << 8) | sNpc.mapNum);
    w[3] = (u16)((msg->type << 12) | (sNpc.role << 10) | msg->localId);
    w[4] = (u16)((msg->x << 8) | msg->y);
    w[5] = (u16)((msg->action << 8) | (msg->facing & 0xF));
    w[6] = sNpc.sendSeq++;
    w[7] = NpcChecksum(w);
    if (gCoopNpcStats.sent < 0xFFFF)
        gCoopNpcStats.sent++;
    return TRUE;
}

bool8 CoopNpc_TryHandleFrame(const u16 w[8])
{
    struct NpcMsg msg;
    u8 senderRole;

    if (w[0] != COOP_HELD_KEYS_CMD || w[1] != (u16)(COOP_BUILD_XOR ^ NPC_MARK))
        return FALSE;
    if (NpcChecksum(w) != w[7])
    {
        if (gCoopNpcStats.dropped < 0xFFFF)
            gCoopNpcStats.dropped++;
        return TRUE;
    }
    if (gCoopNpcStats.received < 0xFFFF)
        gCoopNpcStats.received++;
    if (sNpc.recvSeqKnown && w[6] != (u16)(sNpc.recvSeq + 1) && gCoopNpcStats.seqGaps < 0xFFFF)
        gCoopNpcStats.seqGaps++;
    sNpc.recvSeq = w[6];
    sNpc.recvSeqKnown = TRUE;

    // Another map's frame (one of us just warped): not for this one.
    if (!sNpc.shared || (w[2] >> 8) != sNpc.mapGroup || (w[2] & 0xFF) != sNpc.mapNum)
    {
        if (gCoopNpcStats.dropped < 0xFFFF)
            gCoopNpcStats.dropped++;
        return TRUE;
    }
    msg.type = w[3] >> 12;
    senderRole = (w[3] >> 10) & 3;
    msg.localId = w[3] & 0xFF;
    msg.x = w[4] >> 8;
    msg.y = w[4] & 0xFF;
    msg.action = w[5] >> 8;
    msg.facing = w[5] & 0xF;

    switch (msg.type)
    {
    case NPC_MSG_HELLO:
        // The partner wants a snapshot. Two followers: link player 0 leads.
        if (sNpc.role == COOP_NPC_ROLE_FOLLOW && GetMultiplayerId() == 0)
            SetRole(COOP_NPC_ROLE_AUTH);
        else if (sNpc.role == COOP_NPC_ROLE_AUTH)
        {
            u32 i;
            for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
            {
                if (IsSyncObject(&gObjectEvents[i]))
                    EnqueuePos(&gObjectEvents[i]);
            }
        }
        break;
    case NPC_MSG_POS:
    case NPC_MSG_STEP:
        if (senderRole == COOP_NPC_ROLE_AUTH && sNpc.role == COOP_NPC_ROLE_AUTH)
        {
            // Both took the map at once: link player 0 keeps it.
            if (GetMultiplayerId() == 0)
                break;
            SetRole(COOP_NPC_ROLE_FOLLOW);
        }
        if (sNpc.role == COOP_NPC_ROLE_FOLLOW && senderRole == COOP_NPC_ROLE_AUTH && msg.localId != 0)
        {
            HandleFollowerMsg(&msg);
            sNpc.helloTimer = 0;
        }
        break;
    case NPC_MSG_RESERVE:
        sNpc.partnerTalk = msg.localId;
        sNpc.partnerTalkIdle = 0;
        gCoopNpcStats.partnerTalk = msg.localId;
        if (sNpc.role == COOP_NPC_ROLE_AUTH)
            FacePartner(msg.localId);
        break;
    case NPC_MSG_RELEASE:
        if (sNpc.partnerTalk == msg.localId)
        {
            sNpc.partnerTalk = 0;
            gCoopNpcStats.partnerTalk = 0;
        }
        // The authority's object after its talk: where it faces now.
        if (sNpc.role == COOP_NPC_ROLE_FOLLOW && senderRole == COOP_NPC_ROLE_AUTH && msg.x != 0)
        {
            msg.type = NPC_MSG_POS;
            HandleFollowerMsg(&msg);
        }
        break;
    }
    return TRUE;
}

// ---- per frame -----------------------------------------------------------

void CoopNpc_Reset(void)
{
    ReleaseAll();
    memset(&sNpc, 0, sizeof(sNpc));
    gCoopNpcStats.role = COOP_NPC_ROLE_NONE;
    gCoopNpcStats.partnerTalk = 0;
    gCoopNpcStats.myTalk = 0;
}

static void LeaveShared(void)
{
    if (sNpc.role == COOP_NPC_ROLE_FOLLOW)
        ReleaseAll();
    if (sNpc.role != COOP_NPC_ROLE_NONE)
    {
        sNpc.role = COOP_NPC_ROLE_NONE;
        gCoopNpcStats.role = COOP_NPC_ROLE_NONE;
        if (gCoopNpcStats.roleChanges < 0xFFFF)
            gCoopNpcStats.roleChanges++;
    }
    sNpc.shared = FALSE;
    sNpc.partnerTalk = 0;
    sNpc.myTalk = 0;
    gCoopNpcStats.partnerTalk = 0;
    gCoopNpcStats.myTalk = 0;
    ClearOutgoing();
}

void CoopNpc_FieldFrame(void)
{
    u8 group, num, activity;
    u8 myGroup = gSaveBlock1Ptr->location.mapGroup;
    u8 myNum = gSaveBlock1Ptr->location.mapNum;
    bool8 partner = Coop_PartnerFrameInfo(&group, &num, &activity);

    sNpc.frame++;
    if (myGroup != sNpc.mapGroup || myNum != sNpc.mapNum)
    {
        if (sNpc.shared)
            LeaveShared();
        sNpc.mapGroup = myGroup;
        sNpc.mapNum = myNum;
        sNpc.arriveFrame = sNpc.frame;
        memset(sNpc.remote, 0, sizeof(sNpc.remote));
    }
    if (!partner)
    {
        if (sNpc.shared)
            LeaveShared();
        sNpc.partnerKnown = FALSE;
        return;
    }
    if (!sNpc.partnerKnown || group != sNpc.partnerGroup || num != sNpc.partnerNum)
    {
        sNpc.partnerGroup = group;
        sNpc.partnerNum = num;
        // First sight of the partner (a new link) counts as arriving now.
        sNpc.partnerArriveFrame = sNpc.frame;
        sNpc.partnerKnown = TRUE;
    }

    if (group != myGroup || num != myNum)
    {
        if (sNpc.shared)
            LeaveShared();
        return;
    }

    if (!sNpc.shared)
    {
        s16 lead = (s16)(sNpc.partnerArriveFrame - sNpc.arriveFrame);

        sNpc.shared = TRUE;
        sNpc.recvSeqKnown = FALSE;
        ClearOutgoing();
        if (lead > NPC_ARRIVE_TIE)
            SetRole(COOP_NPC_ROLE_AUTH);            // here first
        else if (lead < -NPC_ARRIVE_TIE)
            SetRole(COOP_NPC_ROLE_FOLLOW);          // the partner was here first
        else
            SetRole(GetMultiplayerId() == 0 ? COOP_NPC_ROLE_AUTH : COOP_NPC_ROLE_FOLLOW);
    }

    // A reservation outlives a lost RELEASE only while the partner is busy.
    if (sNpc.partnerTalk != 0)
    {
        if (activity == COOP_ACT_EXPLORING)
        {
            if (++sNpc.partnerTalkIdle > 90)
            {
                sNpc.partnerTalk = 0;
                gCoopNpcStats.partnerTalk = 0;
            }
        }
        else
        {
            sNpc.partnerTalkIdle = 0;
        }
    }
    EndMyTalkIfDone();

    if (sNpc.role == COOP_NPC_ROLE_AUTH)
        AuthorityFrame();
    else if (sNpc.role == COOP_NPC_ROLE_FOLLOW)
        FollowerFrame();
}
