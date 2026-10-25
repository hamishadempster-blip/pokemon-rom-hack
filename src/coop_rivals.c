#include "global.h"
#include "coop.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "main.h"
#include "overworld.h"
#include "script.h"
#include "sprite.h"
#include "string_util.h"
#include "constants/coop.h"
#include "constants/event_objects.h"
#include "constants/maps.h"
#include "constants/trainer_types.h"

// Birch's twins, Soren and Mira, are both rivals. The story scripts place one
// rival object in each scene: Mira on a boy's save and Soren on a girl's,
// from the stock gender rule. This file puts the other twin beside that
// rival on the overworld, so both are present wherever the rival is, and
// both are the pair the player fights.
//
// The second twin is a picture plus a short line. It walks the rival's trail
// one step behind, never collides with anything, and never stands on a warp,
// so a scene cannot be blocked by it. It disappears with the rival. It is
// spawned in code, so no map file has to change, and it works linked or
// alone.

#define TWIN_TRAIL_LIMIT 2

static EWRAM_DATA s16 sTwinPrimaryX = 0;
static EWRAM_DATA s16 sTwinPrimaryY = 0;
static EWRAM_DATA s16 sTwinPendingX = 0;
static EWRAM_DATA s16 sTwinPendingY = 0;
static EWRAM_DATA bool8 sTwinPending = FALSE;
static EWRAM_DATA u8 sTwinPrimaryLocalId = 0;
static EWRAM_DATA u8 sTwinRetry = 0;

extern const u8 Coop_EventScript_TalkTwin[];

static bool8 Coop_IsRivalGfx(u16 gfx)
{
    return (gfx >= OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL && gfx <= OBJ_EVENT_GFX_RIVAL_BRENDAN_FIELD_MOVE)
        || (gfx >= OBJ_EVENT_GFX_RIVAL_MAY_NORMAL && gfx <= OBJ_EVENT_GFX_RIVAL_MAY_FIELD_MOVE);
}

bool8 Coop_IsRivalGraphics(u16 gfx)
{
    return Coop_IsRivalGfx(gfx);
}

// Same pose, other twin: Brendan's look is Soren, May's look is Mira.
static u16 Coop_TwinGfx(u16 primaryGfx)
{
    if (primaryGfx >= OBJ_EVENT_GFX_RIVAL_MAY_NORMAL && primaryGfx <= OBJ_EVENT_GFX_RIVAL_MAY_FIELD_MOVE)
        return OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL + (primaryGfx - OBJ_EVENT_GFX_RIVAL_MAY_NORMAL);
    return OBJ_EVENT_GFX_RIVAL_MAY_NORMAL + (primaryGfx - OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL);
}

// The bedrooms and the downstairs meeting introduce one twin at home; the
// other is out helping Birch, as the dialogue there says.
static bool8 Coop_TwinExcludedMap(void)
{
    u8 group = gSaveBlock1Ptr->location.mapGroup;
    u8 num = gSaveBlock1Ptr->location.mapNum;

#define IS_MAP(m) (group == MAP_GROUP(m) && num == MAP_NUM(m))
    return IS_MAP(MAP_LITTLEROOT_TOWN_MAYS_HOUSE_1F) || IS_MAP(MAP_LITTLEROOT_TOWN_MAYS_HOUSE_2F)
        || IS_MAP(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F) || IS_MAP(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F);
#undef IS_MAP
}

static bool8 Coop_IsWarpTile(s16 x, s16 y)
{
    u32 i;

    if (gMapHeader.events == NULL)
        return FALSE;
    for (i = 0; i < gMapHeader.events->warpCount; i++)
    {
        if (gMapHeader.events->warps[i].x + MAP_OFFSET == x && gMapHeader.events->warps[i].y + MAP_OFFSET == y)
            return TRUE;
    }
    return FALSE;
}

static EWRAM_DATA u8 sTwinOverlap = 0;

static bool8 Coop_PartnerStandsOn(s16 x, s16 y)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *obj = &gObjectEvents[i];

        if (obj->active && obj->localId == OBJ_EVENT_ID_COOP_PARTNER
         && obj->currentCoords.x == x && obj->currentCoords.y == y && !obj->heldMovementActive)
            return TRUE;
    }
    return FALSE;
}

static bool8 Coop_TwinTileFree(s16 x, s16 y)
{
    s16 px, py;
    u8 objectId;

    if (MapGridGetCollisionAt(x, y) || Coop_IsWarpTile(x, y))
        return FALSE;
    PlayerGetDestCoords(&px, &py);
    if (x == px && y == py)
        return FALSE;
    // The partner's sprite is not solid, but a twin standing on it hides
    // one of them, and each game would then place the twin differently.
    objectId = GetObjectEventIdByXY(x, y);
    return objectId >= OBJECT_EVENTS_COUNT;
}

static u8 Coop_FindTwin(void)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && gObjectEvents[i].localId == OBJ_EVENT_ID_COOP_TWIN)
            return i;
    }
    return OBJECT_EVENTS_COUNT;
}

static u8 Coop_FindPrimaryRival(void)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *obj = &gObjectEvents[i];

        if (!obj->active || obj->isPlayer || obj->invisible)
            continue;
        if (obj->localId == OBJ_EVENT_ID_COOP_TWIN || obj->localId == OBJ_EVENT_ID_COOP_PARTNER || obj->localId >= LOCALID_CAMERA)
            continue;
        if (obj->mapGroup != gSaveBlock1Ptr->location.mapGroup || obj->mapNum != gSaveBlock1Ptr->location.mapNum)
            continue;
        if (Coop_IsRivalGfx(obj->graphicsId))
            return i;
    }
    return OBJECT_EVENTS_COUNT;
}

static void Coop_RemoveTwin(void)
{
    u8 twin = Coop_FindTwin();

    if (twin < OBJECT_EVENTS_COUNT)
        RemoveObjectEvent(&gObjectEvents[twin]);
    sTwinPending = FALSE;
    sTwinPrimaryLocalId = 0;
}

static void Coop_SpawnTwin(struct ObjectEvent *primary)
{
    static const s8 sOffsets[4][2] = { { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 } };
    struct ObjectEventTemplate template;
    s16 x = primary->currentCoords.x, y = primary->currentCoords.y;
    s16 tx, ty;
    s16 bx, by;
    u8 i, objectId;

    // Behind the rival first, then either side, then in front.
    bx = x;
    by = y;
    MoveCoords(GetOppositeDirection(primary->facingDirection), &bx, &by);
    if (Coop_TwinTileFree(bx, by))
    {
        tx = bx;
        ty = by;
    }
    else
    {
        tx = ty = -1;
        for (i = 0; i < 4; i++)
        {
            if (Coop_TwinTileFree(x + sOffsets[i][0], y + sOffsets[i][1]))
            {
                tx = x + sOffsets[i][0];
                ty = y + sOffsets[i][1];
                break;
            }
        }
        if (tx < 0)
            return;
    }

    memset(&template, 0, sizeof(template));
    template.localId = OBJ_EVENT_ID_COOP_TWIN;
    template.graphicsId = Coop_TwinGfx(primary->graphicsId);
    template.kind = OBJ_KIND_NORMAL;
    template.x = tx - MAP_OFFSET;
    template.y = ty - MAP_OFFSET;
    template.elevation = primary->currentElevation;
    template.movementType = MOVEMENT_TYPE_NONE;
    template.trainerType = TRAINER_TYPE_NONE;
    template.script = Coop_EventScript_TalkTwin;
    objectId = SpawnSpecialObjectEvent(&template);
    if (objectId >= OBJECT_EVENTS_COUNT)
        return;
    ObjectEventTurn(&gObjectEvents[objectId], primary->facingDirection);
    sTwinPrimaryX = x;
    sTwinPrimaryY = y;
    sTwinPrimaryLocalId = primary->localId;
    sTwinPending = FALSE;
}

static enum Direction Coop_DirTo(s16 dx, s16 dy)
{
    if (dx > 0)
        return DIR_EAST;
    if (dx < 0)
        return DIR_WEST;
    if (dy > 0)
        return DIR_SOUTH;
    if (dy < 0)
        return DIR_NORTH;
    return DIR_NONE;
}

// Take one trail step toward (x, y): a walk for a neighbour, a jump for a
// ledge two tiles away, and a re-place for anything else.
static void Coop_TwinStepTo(struct ObjectEvent *twin, struct ObjectEvent *primary, s16 x, s16 y)
{
    s16 dx = x - twin->currentCoords.x;
    s16 dy = y - twin->currentCoords.y;
    s16 adx = dx < 0 ? -dx : dx;
    s16 ady = dy < 0 ? -dy : dy;
    enum Direction dir = Coop_DirTo(dx, dy);

    if (dx == 0 && dy == 0)
        return;
    twin->singleMovementActive = FALSE;
    ObjectEventClearHeldMovementIfActive(twin);
    if (adx + ady == 1)
    {
        // Keep the rival's pace: a run or a bike is quicker than a walk.
        u8 action = primary->movementActionId;
        if (action >= MOVEMENT_ACTION_WALK_FASTER_DOWN && action <= MOVEMENT_ACTION_WALK_FASTER_RIGHT)
            ObjectEventSetHeldMovement(twin, GetWalkFasterMovementAction(dir));
        else if (action >= MOVEMENT_ACTION_WALK_FAST_DOWN && action <= MOVEMENT_ACTION_WALK_FAST_RIGHT)
            ObjectEventSetHeldMovement(twin, GetWalkFastMovementAction(dir));
        else
            ObjectEventSetHeldMovement(twin, GetWalkNormalMovementAction(dir));
    }
    else if ((adx == 2 && ady == 0) || (adx == 0 && ady == 2))
    {
        ObjectEventSetHeldMovement(twin, GetJump2MovementAction(dir));
    }
    else
    {
        MoveObjectEventToMapCoords(twin, x, y);
        ObjectEventTurn(twin, primary->facingDirection);
    }
}

// Idle: look where the rival looks, or at the player while the rival talks.
static void Coop_TwinFace(struct ObjectEvent *twin, struct ObjectEvent *primary)
{
    s16 px, py;
    s16 dx, dy;
    enum Direction face = primary->facingDirection;

    PlayerGetDestCoords(&px, &py);
    dx = px - primary->currentCoords.x;
    dy = py - primary->currentCoords.y;
    if ((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) == 1 && Coop_DirTo(dx, dy) == primary->facingDirection)
    {
        dx = px - twin->currentCoords.x;
        dy = py - twin->currentCoords.y;
        if ((dx < 0 ? -dx : dx) >= (dy < 0 ? -dy : dy))
            face = dx > 0 ? DIR_EAST : DIR_WEST;
        else
            face = dy > 0 ? DIR_SOUTH : DIR_NORTH;
    }
    if (face != DIR_NONE && twin->facingDirection != face)
        ObjectEventTurn(twin, face);
}

void Coop_UpdateRivalTwin(void)
{
    u8 primaryId, twinId;
    struct ObjectEvent *primary, *twin;

    if (gMain.callback2 != CB2_Overworld || gMain.inBattle)
        return;

    twinId = Coop_FindTwin();
    primaryId = Coop_TwinExcludedMap() ? OBJECT_EVENTS_COUNT : Coop_FindPrimaryRival();
    if (primaryId >= OBJECT_EVENTS_COUNT)
    {
        if (twinId < OBJECT_EVENTS_COUNT)
            Coop_RemoveTwin();
        return;
    }
    primary = &gObjectEvents[primaryId];
    if (twinId >= OBJECT_EVENTS_COUNT
     || gObjectEvents[twinId].spriteId >= MAX_SPRITES || !gSprites[gObjectEvents[twinId].spriteId].inUse
     || sTwinPrimaryLocalId != primary->localId)
    {
        if (twinId < OBJECT_EVENTS_COUNT)
            Coop_RemoveTwin();
        // A crowded spot: try again a little later instead of every frame.
        if (sTwinRetry > 0)
        {
            sTwinRetry--;
            return;
        }
        Coop_SpawnTwin(primary);
        if (Coop_FindTwin() >= OBJECT_EVENTS_COUNT)
            sTwinRetry = 30;
        return;
    }

    twin = &gObjectEvents[twinId];
    // Cutscenes freeze every object they do not move. The twin follows the
    // rival through the cutscene, so it cannot stay frozen.
    if (twin->frozen)
        UnfreezeObjectEvent(twin);
    twin->movementType = MOVEMENT_TYPE_NONE;
    twin->invisible = primary->invisible;

    // The rival left a tile: that tile is the next step on the trail.
    if (primary->currentCoords.x != sTwinPrimaryX || primary->currentCoords.y != sTwinPrimaryY)
    {
        sTwinPendingX = primary->previousCoords.x;
        sTwinPendingY = primary->previousCoords.y;
        // previousCoords equals currentCoords once a step has settled.
        if (sTwinPendingX == primary->currentCoords.x && sTwinPendingY == primary->currentCoords.y)
        {
            sTwinPendingX = sTwinPrimaryX;
            sTwinPendingY = sTwinPrimaryY;
        }
        sTwinPending = TRUE;
        sTwinPrimaryX = primary->currentCoords.x;
        sTwinPrimaryY = primary->currentCoords.y;
    }

    if (twin->heldMovementActive && !ObjectEventClearHeldMovementIfFinished(twin))
    {
        // Still walking. A rival two steps ahead means the twin fell behind.
        s16 dx = sTwinPendingX - twin->currentCoords.x;
        s16 dy = sTwinPendingY - twin->currentCoords.y;
        if (sTwinPending && (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) > TWIN_TRAIL_LIMIT)
        {
            ObjectEventClearHeldMovementIfActive(twin);
            MoveObjectEventToMapCoords(twin, sTwinPendingX, sTwinPendingY);
            sTwinPending = FALSE;
        }
        return;
    }
    if (sTwinPending)
    {
        sTwinPending = FALSE;
        Coop_TwinStepTo(twin, primary, sTwinPendingX, sTwinPendingY);
        return;
    }
    // The twin was placed before the partner arrived and the partner has
    // stopped on its tile: one would hide the other, and the two screens
    // would disagree. Outside scenes, move it to another free side.
    if (!ScriptContext_IsEnabled() && Coop_PartnerStandsOn(twin->currentCoords.x, twin->currentCoords.y))
    {
        if (++sTwinOverlap >= 30)
        {
            sTwinOverlap = 0;
            Coop_RemoveTwin();
            return;
        }
    }
    else
    {
        sTwinOverlap = 0;
    }
    Coop_TwinFace(twin, primary);
}

// VAR_RESULT is 1 when the twin beside the rival is Soren, 2 for Mira.
u16 Coop_TwinIdentity(void)
{
    u8 twin = Coop_FindTwin();

    if (twin >= OBJECT_EVENTS_COUNT)
        return COOP_RIVAL_NONE;
    if (gObjectEvents[twin].graphicsId >= OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL
     && gObjectEvents[twin].graphicsId <= OBJ_EVENT_GFX_RIVAL_BRENDAN_FIELD_MOVE)
        return COOP_RIVAL_SOREN;
    return COOP_RIVAL_MIRA;
}
