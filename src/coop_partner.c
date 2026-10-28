// Partner menu extras (data/scripts/coop_deal.inc): FOLLOW, PING, VIEW TEAM.
//
// FOLLOW walks this player along the partner's own recent path, one tile at
// a time, by holding the direction a player would hold: the ordinary
// movement code still decides every step (collision, elevation, NPCs), so
// following can never pass through anything a player could not. It stops,
// with a short note, on any button press, before a tile that could start
// something (a warp, a scene trigger, a ledge, water, a forced-movement or
// cracking floor, grass or a cave floor with wild Pokemon), when this
// player's field is taken by a message, a menu or a battle, when the
// partner leaves or the link goes quiet, and when either player is not on
// foot. Two players never follow each other: the menu refuses, and if both
// start on the same frame the link guest stops.
#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_protocol.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "link.h"
#include "main.h"
#include "map_name_popup.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "bike.h"
#include "script.h"
#include "string_util.h"
#include "wild_encounter.h"
#include "constants/flags.h"

enum {
    FOLLOW_OFF,
    FOLLOW_ARMING,      // started from the menu; begins when the field is free
    FOLLOW_ON,
};

enum {
    STOP_NONE,
    STOP_MANUAL,
    STOP_WARP,
    STOP_SCENE,
    STOP_LEDGE,
    STOP_WILD,
    STOP_HAZARD,
    STOP_WATER,
    STOP_BUSY,          // this player: a message, a menu
    STOP_BATTLE,
    STOP_PARTNER_GONE,
    STOP_MODE,          // a bike or surfing on either side
    STOP_LOOP,
    STOP_BLOCKED,
    STOP_LOST,
};

#define TRAIL_SIZE      24
#define STUCK_FRAMES    90
#define RUN_BEHIND      3

struct TrailStep
{
    s16 x, y;
    u8 group, num;
    u8 mode;
    u8 pad;
};

static EWRAM_DATA struct TrailStep sTrail[TRAIL_SIZE] = {0};
static EWRAM_DATA u8 sTrailCount = 0;
static EWRAM_DATA u8 sFollow = FOLLOW_OFF;
static EWRAM_DATA u8 sStopNote = STOP_NONE;    // shown when the field is free
static EWRAM_DATA u16 sStuckFrames = 0;
static EWRAM_DATA s16 sLastX = 0;
static EWRAM_DATA s16 sLastY = 0;
EWRAM_DATA u8 gCoopFollowLastStop = 0;          // the harness reads why it stopped
EWRAM_DATA u16 gCoopFollowSteps = 0;            // tiles walked while following

bool8 CoopFollow_Active(void)
{
    return sFollow != FOLLOW_OFF;
}

static void Trail_Push(u8 group, u8 num, s16 x, s16 y, u8 mode)
{
    struct TrailStep *last;

    if (sTrailCount != 0)
    {
        last = &sTrail[sTrailCount - 1];
        if (last->group == group && last->num == num && last->x == x && last->y == y)
        {
            last->mode = mode;
            return;
        }
    }
    if (sTrailCount == TRAIL_SIZE)
    {
        memmove(&sTrail[0], &sTrail[1], sizeof(sTrail[0]) * (TRAIL_SIZE - 1));
        sTrailCount--;
    }
    last = &sTrail[sTrailCount++];
    last->group = group;
    last->num = num;
    last->x = x;
    last->y = y;
    last->mode = mode;
}

static void Trail_Drop(u8 n)
{
    if (n >= sTrailCount)
    {
        sTrailCount = 0;
        return;
    }
    memmove(&sTrail[0], &sTrail[n], sizeof(sTrail[0]) * (sTrailCount - n));
    sTrailCount -= n;
}

void CoopFollow_Stop(u8 reason)
{
    if (sFollow == FOLLOW_OFF)
        return;
    sFollow = FOLLOW_OFF;
    sTrailCount = 0;
    gCoopFollowLastStop = reason;
    sStopNote = reason;
}

static bool8 OnFoot(void)
{
    return TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_ON_FOOT) != 0;
}

// Special (partner menu FOLLOW). VAR_RESULT: COOP_FOLLOW_* (include/coop.h).
u16 CoopFollow_Start(void)
{
    u8 group, num, mode;
    s16 x, y, lx, ly;

    CoopDeal_BufferPartnerName();
    if (!Coop_PartnerMoveInfo(&group, &num, &x, &y, &mode)
     || !Coop_MapCoordsToLocal(group, num, x, y, &lx, &ly))
        return COOP_FOLLOW_GONE;
    if (Coop_PartnerIsFollowing())
        return COOP_FOLLOW_LOOP;
    if (!OnFoot())
        return COOP_FOLLOW_NOT_ON_FOOT;
    if (mode != 0)
        return COOP_FOLLOW_PARTNER_MODE;
    sTrailCount = 0;
    Trail_Push(group, num, x, y, mode);
    sFollow = FOLLOW_ARMING;
    sStuckFrames = 0;
    sStopNote = STOP_NONE;
    gCoopFollowLastStop = STOP_NONE;
    gCoopFollowSteps = 0;
    sLastX = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x;
    sLastY = gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y;
    return COOP_FOLLOW_OK;
}

static const u8 *StopText(u8 reason)
{
    switch (reason)
    {
    case STOP_MANUAL:       return COMPOUND_STRING("Stopped following.");
    case STOP_WARP:         return COMPOUND_STRING("Follow: door ahead");
    case STOP_SCENE:        return COMPOUND_STRING("Follow: event ahead");
    case STOP_LEDGE:        return COMPOUND_STRING("Follow: ledge ahead");
    case STOP_WILD:         return COMPOUND_STRING("Follow: wild area");
    case STOP_HAZARD:       return COMPOUND_STRING("Follow: tricky floor");
    case STOP_WATER:        return COMPOUND_STRING("Follow: water ahead");
    case STOP_BUSY:         return COMPOUND_STRING("Stopped following.");
    case STOP_BATTLE:       return COMPOUND_STRING("Follow: battle");
    case STOP_PARTNER_GONE: return COMPOUND_STRING("Follow: partner left");
    case STOP_MODE:         return COMPOUND_STRING("Follow: on foot only");
    case STOP_LOOP:         return COMPOUND_STRING("Follow: no loops");
    case STOP_BLOCKED:      return COMPOUND_STRING("Follow: way blocked");
    case STOP_LOST:         return COMPOUND_STRING("Follow: trail lost");
    }
    return NULL;
}

// The notes fit the map-name popup (about 20 characters).
// Why the next tile on the partner's path needs the player's own decision
// (STOP_NONE when it is an ordinary tile). Coordinates are this map's, with
// the map offset, as object events use them.
static u8 TileStop(s16 x, s16 y)
{
    u32 behavior = MapGridGetMetatileBehaviorAt(x, y);
    const struct MapEvents *events = gMapHeader.events;
    s16 ex = x - MAP_OFFSET, ey = y - MAP_OFFSET;
    u32 i;

    if (events != NULL)
    {
        for (i = 0; i < events->warpCount; i++)
        {
            if (events->warps[i].x == ex && events->warps[i].y == ey)
                return STOP_WARP;
        }
        for (i = 0; i < events->coordEventCount; i++)
        {
            const struct CoordEvent *coord = &events->coordEvents[i];

            if (coord->x != ex || coord->y != ey || coord->script == NULL)
                continue;
            if (coord->trigger == 0 || VarGet(coord->trigger) == coord->index)
                return STOP_SCENE;
        }
    }
    if (MetatileBehavior_IsWarpDoor(behavior) || MetatileBehavior_IsNonAnimDoor(behavior)
     || MetatileBehavior_IsLadder(behavior) || MetatileBehavior_IsEscalator(behavior)
     || MetatileBehavior_IsDeepSouthWarp(behavior) || MetatileBehavior_IsDirectionalStairWarp(behavior)
     || MetatileBehavior_IsEastArrowWarp(behavior) || MetatileBehavior_IsWestArrowWarp(behavior)
     || MetatileBehavior_IsNorthArrowWarp(behavior) || MetatileBehavior_IsSouthArrowWarp(behavior)
     || MetatileBehavior_IsAquaHideoutWarp(behavior) || MetatileBehavior_IsMossdeepGymWarp(behavior)
     || MetatileBehavior_IsMtPyreHole(behavior) || MetatileBehavior_IsLavaridgeB1FWarp(behavior)
     || MetatileBehavior_IsLavaridge1FWarp(behavior))
        return STOP_WARP;
    if (MetatileBehavior_IsJumpEast(behavior) || MetatileBehavior_IsJumpWest(behavior)
     || MetatileBehavior_IsJumpNorth(behavior) || MetatileBehavior_IsJumpSouth(behavior))
        return STOP_LEDGE;
    if (MetatileBehavior_IsSurfableWaterOrUnderwater(behavior) || MetatileBehavior_IsWaterfall(behavior))
        return STOP_WATER;
    if (MetatileBehavior_IsForcedMovementTile(behavior) || MetatileBehavior_IsIce(behavior)
     || MetatileBehavior_IsThinIce(behavior) || MetatileBehavior_IsCrackedIce(behavior)
     || MetatileBehavior_IsCrackedFloor(behavior) || MetatileBehavior_IsCrackedFloorHole(behavior)
     || MetatileBehavior_IsMuddySlope(behavior) || MetatileBehavior_IsSpinTile(behavior)
     || MetatileBehavior_IsTrickHouseSlipperyFloor(behavior)
     || MetatileBehavior_IsCyclingRoadPullDownTile(behavior))
        return STOP_HAZARD;
    if (MetatileBehavior_IsEncounterTile(behavior) && GetCurrentMapWildMonHeaderId() != HEADER_NONE)
        return STOP_WILD;
    return STOP_NONE;
}

// Every overworld frame (src/coop.c Coop_OverworldFrame): record the
// partner's path, end following when it can no longer work, and show why.
void CoopFollow_FieldFrame(void)
{
    u8 group, num, mode;
    s16 x, y, lx, ly;
    bool8 fieldBusy = ArePlayerFieldControlsLocked() || ScriptContext_IsEnabled() || gPaletteFade.active;

    if (sFollow == FOLLOW_OFF)
    {
        if (sStopNote != STOP_NONE && !fieldBusy && gMain.callback2 == CB2_Overworld)
        {
            const u8 *text = StopText(sStopNote);

            sStopNote = STOP_NONE;
            if (text != NULL)
            {
                StringCopy(gCoopPopupText, text);
                ShowCoopPopup();
            }
        }
        return;
    }
    if (!Coop_PartnerMoveInfo(&group, &num, &x, &y, &mode)
     || !Coop_MapCoordsToLocal(group, num, x, y, &lx, &ly))
    {
        CoopFollow_Stop(STOP_PARTNER_GONE);
        return;
    }
    // Both following: the guest gives way, so nobody walks in a circle.
    if (Coop_PartnerIsFollowing() && GetMultiplayerId() != 0)
    {
        CoopFollow_Stop(STOP_LOOP);
        return;
    }
    Trail_Push(group, num, x, y, mode);
    if (sFollow == FOLLOW_ARMING)
    {
        // The button that closed the menu's message must be let go first,
        // or it would read as the player taking over at once.
        if (!fieldBusy && !(gMain.heldKeys & (A_BUTTON | B_BUTTON | DPAD_ANY | START_BUTTON | SELECT_BUTTON)))
            sFollow = FOLLOW_ON;
        return;
    }
    if (fieldBusy)
    {
        CoopFollow_Stop(STOP_BUSY);
        return;
    }
    if (!OnFoot())
    {
        CoopFollow_Stop(STOP_MODE);
        return;
    }
}

// A battle took the field (src/coop.c, the solo-battle link callback).
void CoopFollow_NoteBattle(void)
{
    if (sFollow != FOLLOW_OFF)
        CoopFollow_Stop(STOP_BATTLE);
}

static u16 DirectionKey(s16 dx, s16 dy)
{
    if (dx == 1 && dy == 0)
        return DPAD_RIGHT;
    if (dx == -1 && dy == 0)
        return DPAD_LEFT;
    if (dx == 0 && dy == 1)
        return DPAD_DOWN;
    if (dx == 0 && dy == -1)
        return DPAD_UP;
    return 0;
}

// Before the field reads the buttons (src/overworld.c DoCB1_Overworld): any
// real button ends following and goes through as usual; otherwise the
// direction of the next step on the partner's path is held for the player.
void CoopFollow_FieldInput(u16 *newKeys, u16 *heldKeys)
{
    struct ObjectEvent *player;
    s16 cx, cy, tx, ty, px, py;
    s8 found = -1, near = -1;
    u8 i, stop;
    u16 key;

    if (sFollow != FOLLOW_ON)
        return;
    if ((*newKeys | *heldKeys) & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON
                                  | DPAD_ANY | L_BUTTON | R_BUTTON))
    {
        CoopFollow_Stop(STOP_MANUAL);
        return;
    }
    player = &gObjectEvents[gPlayerAvatar.objectEventId];
    // Mid-step: decide again at the next tile.
    if (gPlayerAvatar.tileTransitionState != T_NOT_MOVING && gPlayerAvatar.tileTransitionState != T_TILE_CENTER)
        return;
    cx = player->currentCoords.x;
    cy = player->currentCoords.y;
    if (cx != sLastX || cy != sLastY)
    {
        sLastX = cx;
        sLastY = cy;
        sStuckFrames = 0;
        if (gCoopFollowSteps < 0xFFFF)
            gCoopFollowSteps++;
    }
    for (i = 0; i < sTrailCount; i++)
    {
        if (!Coop_MapCoordsToLocal(sTrail[i].group, sTrail[i].num, sTrail[i].x, sTrail[i].y, &tx, &ty))
            continue;
        if (tx == cx && ty == cy)
            found = i;
        else if ((tx - cx) * (tx - cx) + (ty - cy) * (ty - cy) == 1)
            near = i;
    }
    if (found >= 0)
    {
        Trail_Drop(found);
        i = 1;
    }
    else if (near >= 0)
    {
        Trail_Drop(near);
        i = 0;
    }
    else
    {
        CoopFollow_Stop(STOP_LOST);
        return;
    }
    // The last step is where the partner stands now: stay a tile behind.
    if (i + 1 >= sTrailCount)
    {
        sStuckFrames = 0;
        return;
    }
    if (!Coop_MapCoordsToLocal(sTrail[i].group, sTrail[i].num, sTrail[i].x, sTrail[i].y, &tx, &ty))
    {
        CoopFollow_Stop(STOP_LOST);
        return;
    }
    if (Coop_PartnerViewCoords(&px, &py) && tx == px && ty == py)
        return;
    if (sTrail[i].mode != 0)
    {
        CoopFollow_Stop(STOP_MODE);
        return;
    }
    key = DirectionKey(tx - cx, ty - cy);
    if (key == 0)
    {
        // Two tiles at once: the partner jumped a ledge.
        CoopFollow_Stop(STOP_LEDGE);
        return;
    }
    stop = TileStop(tx, ty);
    if (stop != STOP_NONE)
    {
        CoopFollow_Stop(stop);
        return;
    }
    if (++sStuckFrames > STUCK_FRAMES)
    {
        CoopFollow_Stop(STOP_BLOCKED);
        return;
    }
    *heldKeys |= key;
    // Falling behind a running partner: run too, where running is allowed.
    if (sTrailCount - i > RUN_BEHIND && FlagGet(FLAG_SYS_B_DASH)
     && !IsRunningDisallowed(player->currentMetatileBehavior))
        *heldKeys |= B_BUTTON;
}

// Special (partner menu and CO-OP menu PING). VAR_RESULT: TRUE if sent;
// FALSE while the cooldown runs (STR_VAR_1: seconds left).
u16 CoopPartner_Ping(void)
{
    CoopDeal_BufferPartnerName();
    if (Coop_SendPing())
        return TRUE;
    ConvertIntToDecimalStringN(gStringVar2, Coop_PingCooldownSeconds(), STR_CONV_MODE_LEFT_ALIGN, 2);
    return FALSE;
}

// Special: why the partner menu can't open, in gStringVar4 (the partner's
// real state: a battle, a conversation, a menu, a question, the link).
u16 CoopPartner_BufferBusyReason(void)
{
    const u8 *why;
    u8 *str;

    CoopDeal_BufferPartnerName();
    if (!Coop_SessionActive())
    {
        str = StringCopy(gStringVar4, COMPOUND_STRING("The link to "));
        str = StringCopy(str, gStringVar1);
        StringCopy(str, COMPOUND_STRING(" is reconnecting.\nTry again in a moment."));
        return TRUE;
    }
    switch (Coop_PartnerMarkKind())
    {
    case COOP_MARK_BATTLE:
        why = COMPOUND_STRING(" is in a battle.\nTry again when it's over.");
        break;
    case COOP_MARK_TALK:
        why = COMPOUND_STRING(" is talking to\nsomeone right now.");
        break;
    case COOP_MARK_MENU:
        why = COMPOUND_STRING(" has a menu open.\nTry again in a moment.");
        break;
    case COOP_MARK_INVITE:
        why = COMPOUND_STRING(" is answering\nanother question.");
        break;
    case COOP_MARK_WAIT:
        why = COMPOUND_STRING(" is waiting for\nyou in a scene.");
        break;
    case COOP_MARK_NONE:
        why = COMPOUND_STRING(" isn't close enough\nfor that.");
        break;
    default:
        why = COMPOUND_STRING(" is busy right now.\nTry again in a moment.");
        break;
    }
    str = StringCopy(gStringVar4, gStringVar1);
    StringCopy(str, why);
    return TRUE;
}
