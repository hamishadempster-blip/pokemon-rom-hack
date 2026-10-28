// Changing a submitted action in a co-op battle while the turn is open.
//
// The link master runs the battle engine; each player's game runs its own
// player controller. After a player submits an action, their controller
// shows "Ready. Waiting for …" until the turn starts. B there asks to
// change the choice:
//
//   * the guest's game sends CANCEL (with the number of turns it has seen
//     committed); the master's own player asks locally;
//   * the master's engine handles the request in its action-selection loop,
//     before deciding whether every action is in: if the turn is still
//     open and the action has been taken in, it is withdrawn (its side
//     effects undone: running, a ball throw, a switch's party order, a
//     Mega-style trigger), CANCEL_OK is sent, and the action menu is sent
//     to that player again; the menu itself only reopens on that command;
//   * once the master commits the turn it sends COMMIT; a request it gets
//     for a committed turn, or while the turn runs, gets LATE, and the
//     action stands on both games.
//
// Messages travel in the battle link's own ordered buffers (a fourth
// buffer kind beside the controller and engine buffers), so a CANCEL can't
// overtake the action it withdraws and an answer can't overtake the menu
// it announces. Nothing is consumed by submitting a move, a switch or
// running (PP and Mega-style gimmicks are spent when the turn runs). Items
// leave the bag when chosen (the stock rule), so an item can't be taken
// back: the prompt doesn't offer B then.
#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "coop.h"
#include "sound.h"
#include "constants/battle.h"
#include "constants/songs.h"

enum {
    TURN_CANCEL_REQ = 1,    // player -> master: arg turn seen
    TURN_CANCEL_OK,         // master -> player: withdrawn; the menu follows
    TURN_CANCEL_LATE,       // master -> player: the turn already started
    TURN_COMMIT,            // master -> players: arg the new turn count
};

EWRAM_DATA struct CoopTurnState gCoopTurn = {0};

void CoopTurn_Reset(void)
{
    memset(&gCoopTurn, 0, sizeof(gCoopTurn));
}

static bool32 IsCoop(void)
{
    return (gBattleTypeFlags & BATTLE_TYPE_COOP) != 0;
}

static bool32 IsMaster(void)
{
    return (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER) != 0;
}

static void Send(enum BattlerId battler, u8 op, u8 arg)
{
    u8 data[4];

    data[0] = op;
    data[1] = arg;
    data[2] = battler;
    data[3] = 0;
    PrepareBufferDataTransferLink(battler, B_COMM_COOP_TURN, sizeof(data), data);
}

static void PrintTurnNote(const u8 *text)
{
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    BattleStringExpandPlaceholdersToDisplayedString(text);
    BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
}

// ---------------------------------------------------------------- engine

// A battler's turn begins (master): the party order a switch may change.
void CoopTurn_HostTurnStart(u32 battler)
{
    if (!IsCoop() || battler >= MAX_BATTLERS_COUNT)
        return;
    memcpy(gCoopTurn.orders[battler], gBattleStruct->battlerPartyOrders[battler], sizeof(gCoopTurn.orders[battler]));
}

static void HostWithdraw(enum BattlerId battler)
{
    enum BattlerId other = GetPartnerBattler(battler);
    u8 action = gChosenActionByBattler[battler];

    if (action == B_ACTION_RUN && gChosenActionByBattler[other] != B_ACTION_RUN)
        gHitMarker &= ~HITMARKER_RUN;
    if ((action == B_ACTION_THROW_BALL || action == B_ACTION_USE_ITEM)
     && gChosenActionByBattler[other] != B_ACTION_THROW_BALL && gChosenActionByBattler[other] != B_ACTION_USE_ITEM)
        gBattleStruct->throwingPokeBall = FALSE;
    gBattleStruct->gimmick.toActivate &= ~(1u << battler);
    gBattleStruct->monToSwitchIntoId[battler] = PARTY_SIZE;
    memcpy(gBattleStruct->battlerPartyOrders[battler], gCoopTurn.orders[battler], sizeof(gCoopTurn.orders[battler]));
    gChosenActionByBattler[battler] = B_ACTION_NONE;
}

// The master's own player asked: the answer is local.
static void HostLocalResult(enum BattlerId battler, bool32 changed)
{
    if (gBattlerBattleController[battler] != BATTLE_CONTROLLER_PLAYER)
        return;
    gCoopTurn.asked[battler] = FALSE;
    gCoopTurn.waiting[battler] = FALSE;
    gCoopTurn.lastResult[battler] = changed ? 1 : 2;
    if (!changed)
        PrintTurnNote(COMPOUND_STRING("Too late to change: the turn\nhas already begun."));
}

// The master's action-selection loop, before it counts confirmed actions.
// `confirmedState` is the selection state of an action taken in and
// confirmed; `beforeState` the state that asks for an action again.
void CoopTurn_HostProcess(u8 standbyState, u8 confirmedState, u8 beforeState)
{
    enum BattlerId battler;

    if (!IsCoop() || !IsMaster())
        return;
    for (battler = 0; battler < gBattlersCount; battler++)
    {
        u8 req = gCoopTurn.request[battler];

        if (req == 0)
            continue;
        if (req - 1 != gCoopTurn.turnId)
        {
            // Asked about a turn that is already over.
            gCoopTurn.request[battler] = 0;
            gCoopTurn.lates++;
            Send(battler, TURN_CANCEL_LATE, gCoopTurn.turnId);
            HostLocalResult(battler, FALSE);
            continue;
        }
        if (gBattleCommunication[battler] == confirmedState
         || gBattleCommunication[battler] == standbyState)
        {
            // Not while its controller still shows the waiting note.
            if (IsBattleControllerActiveOrPendingSyncAnywhere(battler))
                continue;
            HostWithdraw(battler);
            gBattleCommunication[battler] = beforeState;
            gCoopTurn.request[battler] = 0;
            gCoopTurn.cancels++;
            Send(battler, TURN_CANCEL_OK, gCoopTurn.turnId);
            HostLocalResult(battler, TRUE);
            continue;
        }
        if (gBattleCommunication[battler] == beforeState)
        {
            // Already open again (a repeated request).
            gCoopTurn.request[battler] = 0;
            continue;
        }
        // Still being taken in (waiting for its move or target, or a
        // selection script): decide once it is in.
    }
}

// The master commits the turn: every action is in and confirmed.
void CoopTurn_HostCommit(void)
{
    enum BattlerId battler;

    if (!IsCoop() || !IsMaster())
        return;
    gCoopTurn.turnId++;
    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (gCoopTurn.request[battler] != 0)
        {
            gCoopTurn.request[battler] = 0;
            gCoopTurn.lates++;
            Send(battler, TURN_CANCEL_LATE, gCoopTurn.turnId);
            HostLocalResult(battler, FALSE);
        }
        gCoopTurn.waiting[battler] = FALSE;
        gCoopTurn.asked[battler] = FALSE;
    }
    Send(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT), TURN_COMMIT, gCoopTurn.turnId);
}

// ---------------------------------------------------------------- link

// A TURN message from the battle link (src/battle_controllers.c). Both
// games see every message, their own included.
void CoopTurn_HandleMsg(u32 battler, const u8 *data)
{
    u8 op = data[0], arg = data[1];

    if (!IsCoop() || battler >= MAX_BATTLERS_COUNT)
        return;
    if (IsMaster())
    {
        if (op != TURN_CANCEL_REQ)
            return;
        // The turn is already running: the action stands.
        if (!Coop_BattleSelectingActions() || arg != gCoopTurn.turnId)
        {
            gCoopTurn.lates++;
            Send(battler, TURN_CANCEL_LATE, gCoopTurn.turnId);
            return;
        }
        gCoopTurn.request[battler] = arg + 1;
        return;
    }
    switch (op)
    {
    case TURN_COMMIT:
        gCoopTurn.turnId = arg;
        for (battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
            gCoopTurn.waiting[battler] = FALSE;
        break;
    case TURN_CANCEL_OK:
        if (!gCoopTurn.asked[battler])
            break;
        gCoopTurn.asked[battler] = FALSE;
        gCoopTurn.waiting[battler] = FALSE;
        gCoopTurn.lastResult[battler] = 1;
        gCoopTurn.cancels++;
        break;
    case TURN_CANCEL_LATE:
        if (!gCoopTurn.asked[battler])
            break;
        gCoopTurn.asked[battler] = FALSE;
        gCoopTurn.waiting[battler] = FALSE;
        gCoopTurn.lastResult[battler] = 2;
        gCoopTurn.lates++;
        PrintTurnNote(COMPOUND_STRING("Too late to change: the turn\nhas already begun."));
        break;
    }
}

// ---------------------------------------------------------------- controller

// The local player submitted an action (B_ACTION_*), from the action menu.
void CoopTurn_NoteAction(u32 battler, u8 action)
{
    if (!IsCoop() || battler >= MAX_BATTLERS_COUNT)
        return;
    gCoopTurn.kind[battler] = action;
}

static bool32 KindCanChange(u8 kind)
{
    return kind == B_ACTION_USE_MOVE || kind == B_ACTION_SWITCH || kind == B_ACTION_RUN;
}

// The waiting note after an action was taken in (PrintLinkStandbyMsg).
const u8 *CoopTurn_StandbyText(u32 battler)
{
    if (IsCoop() && battler < MAX_BATTLERS_COUNT)
    {
        gCoopTurn.waiting[battler] = TRUE;
        gCoopTurn.asked[battler] = FALSE;
        if (KindCanChange(gCoopTurn.kind[battler]))
            return COMPOUND_STRING("Ready. Waiting for {B_LINK_PARTNER_NAME}…\nB: change your choice");
    }
    return COMPOUND_STRING("Ready. Waiting for {B_LINK_PARTNER_NAME}…");
}

// A new action menu: the earlier choice is gone.
void CoopTurn_OnChooseAction(u32 battler)
{
    if (battler >= MAX_BATTLERS_COUNT)
        return;
    gCoopTurn.waiting[battler] = FALSE;
    gCoopTurn.asked[battler] = FALSE;
}

// The local player's controller has nothing to do: B asks to change.
void CoopTurn_ControllerIdle(u32 battler)
{
    if (!IsCoop() || battler >= MAX_BATTLERS_COUNT || !gCoopTurn.waiting[battler] || gCoopTurn.asked[battler])
        return;
    if (!JOY_NEW(B_BUTTON))
        return;
    if (!KindCanChange(gCoopTurn.kind[battler]))
    {
        PrintTurnNote(COMPOUND_STRING("An item can't be taken back\nonce it's chosen."));
        gCoopTurn.waiting[battler] = FALSE;
        return;
    }
    PlaySE(SE_SELECT);
    gCoopTurn.asked[battler] = TRUE;
    gCoopTurn.asks++;
    PrintTurnNote(COMPOUND_STRING("Changing your choice…"));
    if (IsMaster())
    {
        // The master's own player: the engine sees it on its next pass.
        gCoopTurn.request[battler] = gCoopTurn.turnId + 1;
        gCoopTurn.localAsk = battler + 1;
    }
    else
    {
        Send(battler, TURN_CANCEL_REQ, gCoopTurn.turnId);
    }
}
