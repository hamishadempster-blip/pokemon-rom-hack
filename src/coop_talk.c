// Joining the partner's conversation with an NPC.
//
// One game runs the NPC's script: the player who talked first (the host).
// The other player (the joiner) never runs it. The host's game sends each
// line it shows, as it shows it, and the joiner's game presents the lines
// in its own message box, at its own reading speed (each player presses A
// for their own text; nothing times out while reading).
//
// Shared checkpoints: before a movement in the script (applymovement) and
// before a question (yes/no), the host's script waits until the joiner has
// read every earlier line. The joiner's game reports READY when it gets
// there; meanwhile each screen marks the other player (a tick over a player
// who is ready and waiting). The host can go on alone with B; a joiner
// leaving (B while waiting for a line) or a lost link releases every wait.
// Choices stay the host's: the joiner sees the question and which answer
// the host gave. Only the host's save changes.
//
// Which conversations can be joined is decided when the host starts one,
// by following the NPC's script through every path the host's save could
// take (flags and variables read live; the answers to questions are
// followed both ways). Ordinary dialogue (text, questions, flags and
// variables, sounds, NPC movement) can be joined. Gifts, shops, battles,
// warps and anything run by special code (naming screens, scenes,
// menus) cannot: the joiner gets a short note instead.
#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_npc.h"
#include "coop_protocol.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "link.h"
#include "main.h"
#include "map_name_popup.h"
#include "overworld.h"
#include "rtc.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "constants/characters.h"
#include "constants/coop.h"
#include "constants/script_commands.h"
#include "constants/songs.h"

extern const u8 *gStdScripts[];

// ---------------------------------------------------------------- classify

enum {
    CLASS_OK,
    CLASS_GIFT,
    CLASS_SHOP,
    CLASS_BATTLE,
    CLASS_WARP,
    CLASS_SCENE,
};

#define WALK_STATES     12
#define WALK_STEPS      900
#define WALK_STACK      6
#define WALK_OVERLAY    8
#define COMPARE_UNKNOWN 3

struct WalkState
{
    const u8 *pc;
    const u8 *stack[WALK_STACK];
    u8 depth;
    u8 cmp;                      // 0 <, 1 =, 2 >, 3 unknown
    u8 nOverlay;
    u8 unknownResult;            // VAR_RESULT was set by a question
    u16 overlayId[WALK_OVERLAY]; // flags: id | 0x8000 marks a flag
    u16 overlayValue[WALK_OVERLAY];
};

static EWRAM_DATA struct WalkState sWalk[WALK_STATES] = {0};

static u16 Rd16(const u8 *p)
{
    return p[0] | (p[1] << 8);
}

static const u8 *RdPtr(const u8 *p)
{
    return (const u8 *)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static bool8 Overlay_Get(struct WalkState *w, u16 id, u16 *value)
{
    u8 i;

    for (i = 0; i < w->nOverlay; i++)
    {
        if (w->overlayId[i] == id)
        {
            *value = w->overlayValue[i];
            return TRUE;
        }
    }
    return FALSE;
}

static void Overlay_Set(struct WalkState *w, u16 id, u16 value)
{
    u8 i;

    for (i = 0; i < w->nOverlay; i++)
    {
        if (w->overlayId[i] == id)
        {
            w->overlayValue[i] = value;
            return;
        }
    }
    if (w->nOverlay < WALK_OVERLAY)
    {
        w->overlayId[w->nOverlay] = id;
        w->overlayValue[w->nOverlay] = value;
        w->nOverlay++;
    }
}

static bool8 Walk_Flag(struct WalkState *w, u16 flag)
{
    u16 v;

    if (Overlay_Get(w, flag | 0x8000, &v))
        return v != 0;
    return FlagGet(flag);
}

// FALSE when the value can't be known (the answer to a question).
static bool8 Walk_Var(struct WalkState *w, u16 var, u16 *out)
{
    u16 *ptr;

    if (var == VAR_RESULT && w->unknownResult)
        return FALSE;
    if (Overlay_Get(w, var & 0x7FFF, out))
        return TRUE;
    if (var < VARS_START)
        return FALSE;
    ptr = GetVarPointer(var);
    if (ptr == NULL)
        return FALSE;
    *out = *ptr;
    return TRUE;
}

static u8 Cmp(u16 a, u16 b)
{
    return a < b ? 0 : a == b ? 1 : 2;
}

static const u8 sCondition[6][3] = {
    {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 0}, {0, 1, 1}, {1, 0, 1},
};

// What std script n means for joining (msgbox kinds only are dialogue).
static u8 StdClass(u8 n)
{
    switch (n)
    {
    case 2: case 3: case 4: case 5: case 6:     // MSGBOX_NPC .. AUTOCLOSE
        return CLASS_OK;
    case 0: case 1: case 7: case 11: case 12:   // items, decorations
        return CLASS_GIFT;
    }
    return CLASS_SCENE;
}

static u8 OpClass(u8 op)
{
    switch (op)
    {
    case SCR_OP_ADDITEM: case SCR_OP_REMOVEITEM: case SCR_OP_ADDPCITEM:
    case SCR_OP_ADDDECORATION: case SCR_OP_REMOVEDECORATION:
    case SCR_OP_GIVEMON: case SCR_OP_GIVEEGG: case SCR_OP_ADDMONEY:
    case SCR_OP_REMOVEMONEY: case SCR_OP_ADDCOINS: case SCR_OP_REMOVECOINS:
    case SCR_OP_SETMONMOVE:
        return CLASS_GIFT;
    case SCR_OP_POKEMART: case SCR_OP_POKEMARTDECORATION: case SCR_OP_POKEMARTDECORATION2:
        return CLASS_SHOP;
    case SCR_OP_TRAINERBATTLE: case SCR_OP_DOTRAINERBATTLE: case SCR_OP_SETWILDBATTLE:
    case SCR_OP_DOWILDBATTLE:
        return CLASS_BATTLE;
    case SCR_OP_WARP: case SCR_OP_WARPSILENT: case SCR_OP_WARPDOOR: case SCR_OP_WARPHOLE:
    case SCR_OP_WARPTELEPORT: case SCR_OP_WARPSPINENTER: case SCR_OP_WARPMOSSDEEPGYM:
    case SCR_OP_WARPWHITEFADE:
        return CLASS_WARP;
    }
    return CLASS_SCENE;
}

// Bytes taken by a dialogue-safe command, or 0 for any other command.
static u8 OpLength(u8 op)
{
    switch (op)
    {
    case SCR_OP_NOP: case SCR_OP_NOP1: case SCR_OP_WAITSE: case SCR_OP_WAITFANFARE:
    case SCR_OP_LOCK: case SCR_OP_LOCKALL: case SCR_OP_RELEASE: case SCR_OP_RELEASEALL:
    case SCR_OP_FACEPLAYER: case SCR_OP_WAITMESSAGE: case SCR_OP_CLOSEMESSAGE:
    case SCR_OP_WAITBUTTONPRESS: case SCR_OP_CHECKPLAYERGENDER: case SCR_OP_WAITMONCRY:
    case SCR_OP_SIGNMSG: case SCR_OP_NORMALMSG: case SCR_OP_GETPARTYSIZE:
        return 1;
    case SCR_OP_TEXTCOLOR: case SCR_OP_BUFFERLEADMONSPECIESNAME: case SCR_OP_INCREMENTGAMESTAT:
        return 2;
    case SCR_OP_LOAD_BYTE: case SCR_OP_DELAY: case SCR_OP_PLAYSE: case SCR_OP_PLAYFANFARE:
    case SCR_OP_YESNOBOX: case SCR_OP_WAITMOVEMENT: case SCR_OP_RANDOM:
        return 3;
    case SCR_OP_TURNOBJECT: case SCR_OP_BUFFERSPECIESNAME: case SCR_OP_BUFFERITEMNAME:
    case SCR_OP_BUFFERNUMBERSTRING:
        return 4;
    case SCR_OP_MESSAGE: case SCR_OP_CHECKITEM: case SCR_OP_PLAYMONCRY: case SCR_OP_CHECKMONEY:
        return 5;
    case SCR_OP_LOAD_WORD: case SCR_OP_BUFFERSTRING:
        return 6;
    case SCR_OP_APPLYMOVEMENT:
        return 7;
    }
    return 0;
}

// Follows every path of `script` the host's save could take. CLASS_OK when
// all of them are dialogue only; otherwise the first thing found on the way.
static u8 Talk_Classify(const u8 *script)
{
    struct WalkState w;
    u8 count = 1;
    u16 steps = 0;

    if (script == NULL)
        return CLASS_SCENE;
    memset(&sWalk[0], 0, sizeof(sWalk[0]));
    sWalk[0].pc = script;
    sWalk[0].cmp = 1;
    while (count != 0)
    {
        w = sWalk[--count];
        while (TRUE)
        {
            const u8 *pc = w.pc, *dest;
            u8 op = *pc, len, cond, n;
            u16 a, b, va, vb;

            if (++steps > WALK_STEPS)
                return CLASS_SCENE;
            if (op == SCR_OP_END || (op == SCR_OP_RETURN && w.depth == 0))
                break;
            switch (op)
            {
            case SCR_OP_RETURN:
                w.pc = w.stack[--w.depth];
                continue;
            case SCR_OP_GOTO:
            case SCR_OP_CALL:
                if (op == SCR_OP_CALL)
                {
                    if (w.depth >= WALK_STACK)
                        return CLASS_SCENE;
                    w.stack[w.depth++] = pc + 5;
                }
                w.pc = RdPtr(pc + 1);
                continue;
            case SCR_OP_GOTO_IF:
            case SCR_OP_CALL_IF:
                cond = pc[1];
                dest = RdPtr(pc + 2);
                if (cond >= 6)
                    return CLASS_SCENE;
                if (w.cmp == COMPARE_UNKNOWN)
                {
                    // Both ways: the branch taken becomes a new path.
                    if (count >= WALK_STATES)
                        return CLASS_SCENE;
                    sWalk[count] = w;
                    if (op == SCR_OP_CALL_IF)
                    {
                        if (w.depth >= WALK_STACK)
                            return CLASS_SCENE;
                        sWalk[count].stack[sWalk[count].depth++] = pc + 6;
                    }
                    sWalk[count].pc = dest;
                    count++;
                    w.pc = pc + 6;
                    continue;
                }
                if (sCondition[cond][w.cmp])
                {
                    if (op == SCR_OP_CALL_IF)
                    {
                        if (w.depth >= WALK_STACK)
                            return CLASS_SCENE;
                        w.stack[w.depth++] = pc + 6;
                    }
                    w.pc = dest;
                }
                else
                {
                    w.pc = pc + 6;
                }
                continue;
            case SCR_OP_GOTO_STD:
            case SCR_OP_CALL_STD:
                n = StdClass(pc[1]);
                if (n != CLASS_OK)
                    return n;
                if (pc[1] == 5)
                    w.unknownResult = TRUE;
                if (op == SCR_OP_GOTO_STD)
                {
                    // The std script returns to whoever called this one.
                    if (w.depth == 0)
                        goto path_done;
                    w.pc = w.stack[--w.depth];
                    continue;
                }
                w.pc = pc + 2;
                continue;
            case SCR_OP_GOTO_STD_IF:
            case SCR_OP_CALL_STD_IF:
                n = StdClass(pc[2]);
                if (n != CLASS_OK)
                    return n;
                w.pc = pc + 3;
                continue;
            case SCR_OP_CHECKFLAG:
                w.cmp = Walk_Flag(&w, Rd16(pc + 1)) ? 1 : 0;
                w.pc = pc + 3;
                continue;
            case SCR_OP_SETFLAG:
            case SCR_OP_CLEARFLAG:
                Overlay_Set(&w, Rd16(pc + 1) | 0x8000, op == SCR_OP_SETFLAG);
                w.pc = pc + 3;
                continue;
            case SCR_OP_COMPARE_VAR_TO_VALUE:
            case SCR_OP_COMPARE_VAR_TO_VAR:
                a = Rd16(pc + 1);
                b = Rd16(pc + 3);
                if (!Walk_Var(&w, a, &va))
                    w.cmp = COMPARE_UNKNOWN;
                else if (op == SCR_OP_COMPARE_VAR_TO_VALUE)
                    w.cmp = Cmp(va, b);
                else if (!Walk_Var(&w, b, &vb))
                    w.cmp = COMPARE_UNKNOWN;
                else
                    w.cmp = Cmp(va, vb);
                w.pc = pc + 5;
                continue;
            case SCR_OP_SETVAR:
            case SCR_OP_ADDVAR:
            case SCR_OP_SUBVAR:
            case SCR_OP_COPYVAR:
            case SCR_OP_SETORCOPYVAR:
                a = Rd16(pc + 1);
                b = Rd16(pc + 3);
                if (op == SCR_OP_SETVAR)
                {
                    Overlay_Set(&w, a & 0x7FFF, b);
                    if (a == VAR_RESULT)
                        w.unknownResult = FALSE;
                }
                else if (op == SCR_OP_COPYVAR || op == SCR_OP_SETORCOPYVAR)
                {
                    if (Walk_Var(&w, b, &vb))
                        Overlay_Set(&w, a & 0x7FFF, vb);
                    else if (a == VAR_RESULT)
                        w.unknownResult = TRUE;
                }
                else if (Walk_Var(&w, a, &va))
                {
                    Overlay_Set(&w, a & 0x7FFF, op == SCR_OP_ADDVAR ? va + b : va - b);
                }
                w.pc = pc + 5;
                continue;
            case SCR_OP_YESNOBOX:
            case SCR_OP_RANDOM:
            case SCR_OP_CHECKITEM:
            case SCR_OP_CHECKMONEY:
            case SCR_OP_CHECKPLAYERGENDER:
            case SCR_OP_GETPARTYSIZE:
                // Results the walk doesn't evaluate: VAR_RESULT unknown.
                w.unknownResult = TRUE;
                break;
            }
            len = OpLength(op);
            if (len == 0)
                return OpClass(op);
            w.pc = pc + len;
        }
    path_done:
        ;
    }
    return CLASS_OK;
}

// ---------------------------------------------------------------- shared

enum {
    TALK_OP_JOIN = 1,       // joiner -> host
    TALK_OP_ACCEPT,         // host -> joiner
    TALK_OP_REFUSE,         // host -> joiner, arg: reason
    TALK_OP_TEXT,           // host -> joiner, arg: chunk, arg2: chunks, serial: line
    TALK_OP_CHECK,          // host -> joiner, serial: checkpoint
    TALK_OP_READY,          // joiner -> host, serial: checkpoint
    TALK_OP_END,            // host -> joiner, arg: reason
    TALK_OP_LEAVE,          // joiner -> host
    TALK_OP_CLASS,          // host -> partner, arg: why it can't be joined (CLASS_*), arg2: the NPC
};

// Why the partner's conversation can't be shared (TALK_OP_CLASS) and with
// whom: the corner note says what they are doing.
static EWRAM_DATA u8 sPartnerClass = 0;
static EWRAM_DATA u8 sPartnerClassNpc = 0;

enum {
    END_DONE,
    END_WENT_AHEAD,
    END_PRIVATE,
    END_GONE,
};

#define TALK_TEXT_MAX   1000
#define TALK_CHUNK      80
#define TALK_LINES      3

struct TalkLine
{
    u8 op;
    u8 arg;
    u16 serial;
    u16 len;
    u8 sent;            // chunks sent (TEXT)
    u8 text[TALK_TEXT_MAX];
};

struct TalkHost
{
    bool8 active;
    bool8 joinable;
    bool8 joiner;
    u8 class;
    u8 npcLocalId;
    u8 choicePending;   // a yes/no answer to report with the next line
    u8 waitPopup;
    bool8 joinerLost;   // dropped while the link was quiet: tell it when back
    u16 lineSerial;
    u16 checkSent;
    u16 checkReady;
    const u8 *passOp;   // the checkpoint command let through next
    u16 lastLen;
    u8 last[TALK_TEXT_MAX];
    u8 outCount;
    struct TalkLine out[TALK_LINES];
};

enum {
    JOIN_NONE,
    JOIN_ASKING,
    JOIN_JOINED,
};

enum {
    EV_TEXT,
    EV_CHECK,
    EV_END,
    EV_REFUSE,
    EV_ACCEPT,
};

struct TalkEvent
{
    u8 kind;
    u8 arg;
    u16 serial;
    u16 len;
    u8 text[TALK_TEXT_MAX];
};

struct TalkJoin
{
    u8 state;
    u8 npcLocalId;
    u16 askFrames;
    u16 assembleSerial;
    u16 assembleLen;
    u8 assemble[TALK_TEXT_MAX];
    u8 count;
    u8 head;
    struct TalkEvent events[TALK_LINES];
    bool8 greeted;
    u8 greetedNpc;
};

static EWRAM_DATA struct TalkHost sHost = {0};
static EWRAM_DATA struct TalkJoin sJoin = {0};
static EWRAM_DATA u8 sBusyPopupCooldown = 0;
EWRAM_DATA struct CoopTalkStats gCoopTalkStats = {0};

static void PartnerName(u8 *dest)
{
    if (gReceivedRemoteLinkPlayers && GetMultiplayerId() <= 1)
    {
        StringCopyN(dest, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
        dest[PLAYER_NAME_LENGTH] = EOS;
    }
    else
    {
        StringCopy(dest, COMPOUND_STRING("your partner"));
    }
}

static void Popup(const u8 *text)
{
    StringCopy(gCoopPopupText, text);
    ShowCoopPopup();
}

// ---------------------------------------------------------------- host

bool8 CoopTalk_LocalJoinable(void)
{
    return sHost.active && sHost.joinable;
}

static struct TalkLine *Host_Queue(u8 op, u8 arg, u16 serial)
{
    struct TalkLine *line;

    if (sHost.outCount == TALK_LINES)
        return NULL;
    line = &sHost.out[sHost.outCount++];
    line->op = op;
    line->arg = arg;
    line->serial = serial;
    line->len = 0;
    line->sent = 0;
    return line;
}

static void Host_QueueText(const u8 *text, u16 len)
{
    struct TalkLine *line = Host_Queue(TALK_OP_TEXT, 0, ++sHost.lineSerial);

    if (line == NULL)
        return;
    if (len > TALK_TEXT_MAX - 1)
        len = TALK_TEXT_MAX - 1;
    memcpy(line->text, text, len);
    line->text[len] = EOS;
    line->len = len + 1;
}

// The host's own yes/no answer, reported as a line of its own.
static void Host_ReportChoice(void)
{
    u8 text[40];
    u8 *str;

    if (!sHost.choicePending)
        return;
    sHost.choicePending = FALSE;
    str = StringCopy(text, gSaveBlock2Ptr->playerName);
    StringCopy(str, gSpecialVar_Result ? COMPOUND_STRING(" answered YES.") : COMPOUND_STRING(" answered NO."));
    Host_QueueText(text, StringLength(text));
}

static void Host_Pump(void)
{
    while (sHost.outCount != 0 && CoopDeal_TalkRoom() != 0)
    {
        struct TalkLine *line = &sHost.out[0];

        if (line->op == TALK_OP_TEXT)
        {
            u8 chunks = (line->len + TALK_CHUNK - 1) / TALK_CHUNK;
            u16 off = line->sent * TALK_CHUNK;
            u16 n = line->len - off;

            if (n > TALK_CHUNK)
                n = TALK_CHUNK;
            if (!CoopDeal_SendTalk(TALK_OP_TEXT, line->sent, chunks, line->serial, line->len, line->text + off, n))
                return;
            if (++line->sent < chunks)
                continue;
        }
        else if (!CoopDeal_SendTalk(line->op, line->arg, 0, line->serial, 0, NULL, 0))
        {
            return;
        }
        sHost.outCount--;
        memmove(&sHost.out[0], &sHost.out[1], sizeof(sHost.out[0]) * sHost.outCount);
    }
}

static void Host_DropJoiner(u8 reason)
{
    if (!sHost.joiner)
        return;
    sHost.joiner = FALSE;
    sHost.outCount = 0;
    Host_Queue(TALK_OP_END, reason, 0);
}

static void Host_End(void)
{
    if (!sHost.active)
        return;
    if (sHost.joiner)
    {
        Host_ReportChoice();
        Host_Queue(TALK_OP_END, END_DONE, 0);
    }
    sHost.active = FALSE;
    sHost.joiner = FALSE;
    sHost.passOp = NULL;
    gCoopLocalWaiting = FALSE;
}

// A player starts a script with an NPC (src/field_control_avatar.c).
// Returns the script to run: the NPC's own, or a join offer / busy note
// when the partner is already talking to this NPC.
// NPCs that serve each player on their own game, side by side.
extern const u8 RustboroCity_TrainerExchange_EventScript_MovesClerk[];
extern const u8 RustboroCity_TrainerExchange_EventScript_HeldClerk[];
extern const u8 RustboroCity_TrainerExchange_EventScript_EvoClerk[];
extern const u8 RustboroCity_TrainerExchange_EventScript_TrainClerk[];
extern const u8 RustboroCity_TrainerExchange_EventScript_BreedClerk[];
extern const u8 Route117_PokemonDayCare_EventScript_DaycareWoman[];
extern const u8 Route117_EventScript_DaycareMan[];

static bool8 IsIndependentService(const u8 *script)
{
    return script == RustboroCity_TrainerExchange_EventScript_MovesClerk
        || script == RustboroCity_TrainerExchange_EventScript_HeldClerk
        || script == RustboroCity_TrainerExchange_EventScript_EvoClerk
        || script == RustboroCity_TrainerExchange_EventScript_TrainClerk
        || script == RustboroCity_TrainerExchange_EventScript_BreedClerk
        // The Day Care: each save's own deposits, eggs and fees.
        || script == Route117_PokemonDayCare_EventScript_DaycareWoman
        || script == Route117_EventScript_DaycareMan;
}

const u8 *CoopTalk_OnInteract(u8 objectEventId, const u8 *script)
{
    struct ObjectEvent *obj;
    u8 partnerNpc;
    bool8 joinable;

    if (objectEventId >= OBJECT_EVENTS_COUNT || script == NULL || !Coop_SessionActive())
        return script;
    obj = &gObjectEvents[objectEventId];
    if (obj->localId == OBJ_EVENT_ID_COOP_PARTNER || obj->localId == OBJ_EVENT_ID_COOP_TWIN)
        return script;
    // The partner is talking to this NPC. A shop, a counter of the Trainer
    // Exchange, the Day Care or a personal gift serves each player on their own game
    // (their own money, bag and rewards): this player gets the NPC as usual.
    // Anything else is joined (dialogue) or left to the partner (a note).
    if (Coop_PartnerTalkInfo(&partnerNpc, &joinable) && partnerNpc == obj->localId)
    {
        u8 own = IsIndependentService(script) ? CLASS_SHOP : Talk_Classify(script);

        if (own == CLASS_SHOP || own == CLASS_GIFT)
        {
            if (gCoopTalkStats.sideBySide < 0xFF)
                gCoopTalkStats.sideBySide++;
            return script;
        }
        sJoin.npcLocalId = obj->localId;
        if (joinable)
            return CoopTalk_EventScript_JoinAsk;
        return CoopTalk_EventScript_BusyNote;
    }
    if (script == Coop_EventScript_NpcBusy)
        return CoopTalk_EventScript_BusyNote;
    // This player hosts: classify once, with this save's state.
    Host_End();
    {
        bool8 lost = sHost.joinerLost;
        u8 pending = sHost.outCount;

        memset(&sHost, 0, offsetof(struct TalkHost, out));
        sHost.joinerLost = lost;
        sHost.outCount = pending;   // an END still on its way
    }
    sHost.active = TRUE;
    sHost.npcLocalId = obj->localId;
    sHost.class = IsIndependentService(script) ? CLASS_SHOP : Talk_Classify(script);
    sHost.joinable = (sHost.class == CLASS_OK);
    // Tell the partner why this one can't be shared, for their note: only
    // for a reason the note names (a shop, a gift, a battle, a warp; scenes
    // and special code get the plain note anyway) and only when they are
    // here to press A. Every message waits for an answer, so needless ones
    // would delay others (a quest's progress) on a busy link.
    if (!sHost.joinable && sHost.class != CLASS_SCENE && Coop_PartnerOnSameMap())
        CoopDeal_SendTalk(TALK_OP_CLASS, sHost.class, obj->localId, 0, 0, NULL, 0);
    if (gCoopTalkStats.hosted < 0xFF)
        gCoopTalkStats.hosted++;
    gCoopTalkStats.lastClass = sHost.class;
    return script;
}

// Every message the field shows (src/field_message_box.c), expanded.
void CoopTalk_NoteText(const u8 *text)
{
    u16 len;

    if (!sHost.active)
        return;
    len = StringLength(text);
    if (len > TALK_TEXT_MAX - 1)
        len = TALK_TEXT_MAX - 1;
    memcpy(sHost.last, text, len);
    sHost.last[len] = EOS;
    sHost.lastLen = len;
    if (sHost.joiner)
    {
        Host_ReportChoice();
        Host_QueueText(text, len);
    }
}

static bool8 Host_WaitReady(void)
{
    if (!sHost.active || !sHost.joiner || sHost.checkReady >= sHost.checkSent || !Coop_SessionActive())
    {
        if (sHost.joiner && !Coop_SessionActive())
        {
            sHost.joiner = FALSE;
            sHost.joinerLost = TRUE;
        }
        gCoopLocalWaiting = FALSE;
        return TRUE;
    }
    // Go on alone: the partner is told and stops listening.
    if (JOY_NEW(B_BUTTON))
    {
        Host_DropJoiner(END_WENT_AHEAD);
        gCoopLocalWaiting = FALSE;
        return TRUE;
    }
    gCoopLocalWaiting = TRUE;
    if (!sHost.waitPopup)
    {
        u8 *str;

        sHost.waitPopup = TRUE;
        str = StringCopy(gCoopPopupText, COMPOUND_STRING("Waiting for "));
        PartnerName(str);
        ShowCoopPopup();
    }
    return FALSE;
}

// Shared checkpoints (src/scrcmd.c, at the start of applymovement and
// yesnobox): with a joiner, the host's script waits until the joiner has
// read every line before it. TRUE: the command waits (it runs again after).
bool8 CoopTalk_HostCheckpoint(struct ScriptContext *ctx, bool8 isQuestion)
{
    const u8 *op = ctx->scriptPtr - 1;

    if (!sHost.active || !sHost.joiner || Script_IsAnalyzingEffects())
        return FALSE;
    if (sHost.passOp == op)
    {
        sHost.passOp = NULL;
        if (isQuestion)
            sHost.choicePending = TRUE;
        return FALSE;
    }
    Host_ReportChoice();
    Host_Queue(TALK_OP_CHECK, 0, ++sHost.checkSent);
    sHost.passOp = op;
    sHost.waitPopup = FALSE;
    ctx->scriptPtr = op;
    if (gCoopTalkStats.checkpoints < 0xFF)
        gCoopTalkStats.checkpoints++;
    SetupNativeScript(ctx, Host_WaitReady);
    return TRUE;
}

static void Host_HandleJoin(void)
{
    u8 text[48];
    u8 *str;

    if (!sHost.active || !sHost.joinable || !ScriptContext_IsEnabled())
    {
        Host_Queue(TALK_OP_REFUSE, sHost.active ? END_PRIVATE : END_DONE, 0);
        return;
    }
    sHost.joiner = TRUE;
    sHost.outCount = 0;
    Host_Queue(TALK_OP_ACCEPT, sHost.npcLocalId, 0);
    // The line on screen now, so the joiner starts where the host is.
    if (sHost.lastLen != 0)
        Host_QueueText(sHost.last, sHost.lastLen);
    PartnerName(text);
    str = text + StringLength(text);
    StringCopy(str, COMPOUND_STRING(" joined in."));
    Popup(text);
    if (gCoopTalkStats.joinsAccepted < 0xFF)
        gCoopTalkStats.joinsAccepted++;
}

// ---------------------------------------------------------------- joiner

static struct TalkEvent *Join_Push(u8 kind)
{
    struct TalkEvent *ev;

    if (sJoin.count == TALK_LINES)
        return NULL;
    ev = &sJoin.events[(sJoin.head + sJoin.count) % TALK_LINES];
    sJoin.count++;
    ev->kind = kind;
    ev->arg = 0;
    ev->serial = 0;
    ev->len = 0;
    ev->text[0] = EOS;
    return ev;
}

static void Join_Pop(void)
{
    if (sJoin.count == 0)
        return;
    sJoin.head = (sJoin.head + 1) % TALK_LINES;
    sJoin.count--;
}

// Deal messages of type TALK (src/coop_deal.c).
void CoopTalk_HandleMsg(u8 op, u8 arg, u8 arg2, u16 serial, u16 total, const u8 *data)
{
    struct TalkEvent *ev;
    u16 off, n;

    gCoopTalkStats.rxOps[gCoopTalkStats.rxCount++ & 7] = (op << 4) | (arg & 0xF);

    switch (op)
    {
    case TALK_OP_JOIN:
        Host_HandleJoin();
        return;
    case TALK_OP_READY:
        if (serial > sHost.checkReady)
            sHost.checkReady = serial;
        return;
    case TALK_OP_LEAVE:
        sHost.joiner = FALSE;
        sHost.outCount = 0;
        return;
    case TALK_OP_CLASS:
        sPartnerClass = arg;
        sPartnerClassNpc = arg2;
        return;
    }
    if (sJoin.state == JOIN_NONE)
        return;
    switch (op)
    {
    case TALK_OP_ACCEPT:
        sJoin.state = JOIN_JOINED;
        Join_Push(EV_ACCEPT);
        break;
    case TALK_OP_REFUSE:
        if ((ev = Join_Push(EV_REFUSE)) != NULL)
            ev->arg = arg;
        break;
    case TALK_OP_END:
        if ((ev = Join_Push(EV_END)) != NULL)
            ev->arg = arg;
        break;
    case TALK_OP_CHECK:
        if ((ev = Join_Push(EV_CHECK)) != NULL)
            ev->serial = serial;
        break;
    case TALK_OP_TEXT:
        if (arg == 0)
        {
            sJoin.assembleSerial = serial;
            sJoin.assembleLen = 0;
        }
        if (serial != sJoin.assembleSerial || total > TALK_TEXT_MAX)
            return;
        off = arg * TALK_CHUNK;
        n = total > off ? total - off : 0;
        if (n > TALK_CHUNK)
            n = TALK_CHUNK;
        if (off + n > TALK_TEXT_MAX)
            return;
        memcpy(sJoin.assemble + off, data, n);
        sJoin.assembleLen = off + n;
        if (arg + 1 == arg2 && (ev = Join_Push(EV_TEXT)) != NULL)
        {
            memcpy(ev->text, sJoin.assemble, sJoin.assembleLen);
            ev->text[TALK_TEXT_MAX - 1] = EOS;
            ev->len = sJoin.assembleLen;
        }
        break;
    }
}

// VAR_RESULT for the joiner's presentation script.
#define TALK_EV_TEXT    COOP_TALK_EV_TEXT
#define TALK_EV_END     COOP_TALK_EV_END
#define TALK_EV_NOTE    COOP_TALK_EV_NOTE
#define TALK_EV_LEFT    COOP_TALK_EV_LEFT

// A line that greets both players, once per NPC and session, only when the
// joiner arrives at the start of a conversation: what an NPC says on seeing
// a second trainer join in. Morning, day, evening and night differ.
static void Join_Greeting(void)
{
    static const u8 *const sLines[] = {
        COMPOUND_STRING("Oh! Two of you now?\nHello there, {STR_VAR_1}!"),
        COMPOUND_STRING("Ah, {STR_VAR_1}'s here too!\nI'll keep it short for both of you."),
        COMPOUND_STRING("Oh, {STR_VAR_1}! Come and listen,\nyou two."),
    };
    u8 *str;

    if (sJoin.greeted && sJoin.greetedNpc == sJoin.npcLocalId)
        return;
    sJoin.greeted = TRUE;
    sJoin.greetedNpc = sJoin.npcLocalId;
    StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
    str = gStringVar4;
    switch (GetTimeOfDay())
    {
    case TIME_MORNING:
        str = StringCopy(str, COMPOUND_STRING("Good morning! "));
        break;
    case TIME_EVENING:
        str = StringCopy(str, COMPOUND_STRING("Good evening! "));
        break;
    case TIME_NIGHT:
        str = StringCopy(str, COMPOUND_STRING("Up late, you two? "));
        break;
    default:
        break;
    }
    StringExpandPlaceholders(str, sLines[(sJoin.npcLocalId + gSaveBlock1Ptr->location.mapNum) % ARRAY_COUNT(sLines)]);
}

static void Join_Note(u8 reason)
{
    gCoopTalkStats.lastNote = reason + 1;
    u8 name[PLAYER_NAME_LENGTH + 1];
    u8 *str;

    PartnerName(name);
    str = StringCopy(gStringVar4, name);
    switch (reason)
    {
    case END_WENT_AHEAD:
        StringCopy(str, COMPOUND_STRING(" went on without\nwaiting."));
        break;
    case END_PRIVATE:
        StringCopy(str, COMPOUND_STRING("'s conversation\ncan't be shared."));
        break;
    case END_GONE:
        StringCopy(gStringVar4, COMPOUND_STRING("The link was lost, so the\nconversation ended here."));
        break;
    default:
        StringCopy(str, COMPOUND_STRING("'s conversation\nhas already ended."));
        break;
    }
}

static void Join_Finish(u8 result)
{
    gCoopTalkStats.lastJoinEnd = result + 1;
    sJoin.state = JOIN_NONE;
    sJoin.count = 0;
    gCoopLocalWaiting = FALSE;
    gSpecialVar_Result = result;
}

static void Task_JoinWait(u8 taskId)
{
    while (sJoin.count != 0)
    {
        struct TalkEvent *ev = &sJoin.events[sJoin.head];

        switch (ev->kind)
        {
        case EV_ACCEPT:
            Join_Pop();
            // A greeting for both, once per NPC this session.
            if (!(sJoin.greeted && sJoin.greetedNpc == sJoin.npcLocalId))
            {
                Join_Greeting();
                if (gCoopTalkStats.greetings < 0xFF)
                    gCoopTalkStats.greetings++;
                gSpecialVar_Result = TALK_EV_TEXT;
                goto finished;
            }
            continue;
        case EV_CHECK:
            // Every earlier line has been read: tell the host.
            if (!CoopDeal_SendTalk(TALK_OP_READY, 0, 0, ev->serial, 0, NULL, 0))
                return;
            if (gCoopTalkStats.readySent < 0xFF)
                gCoopTalkStats.readySent++;
            Join_Pop();
            continue;
        case EV_TEXT:
            StringCopy(gStringVar4, ev->text);
            Join_Pop();
            if (gCoopTalkStats.linesShown < 0xFFFF)
                gCoopTalkStats.linesShown++;
            gSpecialVar_Result = TALK_EV_TEXT;
            goto finished;
        case EV_END:
            if (ev->arg == END_DONE)
            {
                Join_Finish(TALK_EV_END);
            }
            else
            {
                Join_Note(ev->arg);
                Join_Finish(TALK_EV_NOTE);
            }
            goto finished;
        case EV_REFUSE:
            Join_Note(ev->arg);
            Join_Finish(TALK_EV_NOTE);
            goto finished;
        default:
            Join_Pop();
            continue;
        }
    }
    if (!Coop_SessionActive())
    {
        Join_Note(END_GONE);
        Join_Finish(TALK_EV_NOTE);
        goto finished;
    }
    if (sJoin.state == JOIN_ASKING)
    {
        // Only the request itself has a limit, never anyone's reading.
        if (++sJoin.askFrames > 600)
        {
            Join_Note(END_DONE);
            Join_Finish(TALK_EV_NOTE);
            goto finished;
        }
        return;
    }
    // The host stopped talking without a word (a link outage ended it on
    // their side): once their frames have said so for a while, end here.
    {
        u8 npc;
        bool8 joinable;

        if (Coop_PartnerTalkInfo(&npc, &joinable) && npc == sJoin.npcLocalId)
            gTasks[taskId].data[2] = 0;
        else if (++gTasks[taskId].data[2] > 180)
        {
            Join_Note(END_GONE);
            Join_Finish(TALK_EV_NOTE);
            goto finished;
        }
    }
    // Caught up, waiting for the host's next line: ready. B leaves.
    gCoopLocalWaiting = TRUE;
    if (++gTasks[taskId].data[1] > 20 && JOY_NEW(B_BUTTON))
    {
        CoopDeal_SendTalk(TALK_OP_LEAVE, 0, 0, 0, 0, NULL, 0);
        Join_Finish(TALK_EV_LEFT);
        goto finished;
    }
    return;
finished:
    gCoopLocalWaiting = FALSE;
    DestroyTask(taskId);
    ScriptContext_Enable();
}

// Special, `waitstate` follows: the next thing to show. VAR_RESULT:
// TALK_EV_TEXT (the line is in gStringVar4), TALK_EV_END, TALK_EV_NOTE (a
// closing note in gStringVar4), TALK_EV_LEFT.
void CoopTalk_WaitEvent(void)
{
    CreateTask(Task_JoinWait, 80);
}

// Special: ask the host to let this player in. VAR_RESULT: TRUE if sent.
u16 CoopTalk_RequestJoin(void)
{
    sJoin.state = JOIN_NONE;
    sJoin.count = 0;
    sJoin.head = 0;
    sJoin.askFrames = 0;
    if (!Coop_SessionActive() || !CoopDeal_SendTalk(TALK_OP_JOIN, 0, 0, 0, 0, NULL, 0))
        return FALSE;
    sJoin.state = JOIN_ASKING;
    if (gCoopTalkStats.joinsAsked < 0xFF)
        gCoopTalkStats.joinsAsked++;
    return TRUE;
}

// Special: STR_VAR_1 the partner's name.
u16 CoopTalk_BufferPartner(void)
{
    PartnerName(gStringVar1);
    return TRUE;
}

// A on the partner while they talk to an NPC: TRUE when that conversation
// can be joined (data/scripts/coop.inc).
// VAR_RESULT: 0 the partner isn't talking to an NPC, 1 they are and it can
// be joined, 2 they are and it can't.
u16 CoopTalk_PartnerJoinable(void)
{
    u8 npc;
    bool8 joinable;

    if (!Coop_PartnerTalkInfo(&npc, &joinable))
        return 0;
    sJoin.npcLocalId = npc;
    return joinable ? 1 : 2;
}

// The partner's conversation can't be joined: a short note in the map-name
// popup, never a message box, and not again for a few seconds.
static EWRAM_DATA u8 sBusyNoteWait = 0;    // frames left to show a held note

static bool8 PopupFits(void)
{
    return GetStringWidth(GetFontIdToFit(gCoopPopupText, FONT_NORMAL, -1, 68), gCoopPopupText, -1) <= 68;
}

static bool8 ShowBusyNoteNow(void)
{
    u8 *str = gCoopPopupText;

    // The popup shows 68 pixels of text (src/map_name_popup.c): the longest
    // wording that fits the partner's name.
    u8 npc;
    bool8 joinable;
    u8 class = CLASS_SCENE;

    if (Coop_PartnerTalkInfo(&npc, &joinable) && npc == sPartnerClassNpc)
        class = sPartnerClass;
    gCoopTalkStats.busyClass = class;
    if (class == CLASS_GIFT)
    {
        str = StringCopy(str, COMPOUND_STRING("A gift for "));
        PartnerName(str);
        if (PopupFits())
            return ShowCoopPopup();
        str = gCoopPopupText;
    }
    PartnerName(str);
    str += StringLength(str);
    if (class == CLASS_SHOP)
        StringCopy(str, COMPOUND_STRING(" is shopping"));
    else if (class == CLASS_BATTLE)
        StringCopy(str, COMPOUND_STRING(" is battling"));
    else if (class == CLASS_WARP)
        StringCopy(str, COMPOUND_STRING(" is moving on"));
    else
        StringCopy(str, COMPOUND_STRING(" is talking to them"));
    if (PopupFits() || class != CLASS_SCENE)
        return ShowCoopPopup();
    if (!PopupFits())
        StringCopy(str, COMPOUND_STRING("'s talking to them"));
    if (!PopupFits())
        StringCopy(str, COMPOUND_STRING(" is talking"));
    return ShowCoopPopup();
}

void CoopTalk_ShowBusyNote(void)
{
    if (sBusyPopupCooldown != 0)
        return;
    sBusyPopupCooldown = 180;
    if (gCoopTalkStats.busyNotes < 0xFF)
        gCoopTalkStats.busyNotes++;
    // The map name may still be showing after a door: the note waits a
    // moment for it rather than being lost.
    if (!ShowBusyNoteNow())
        sBusyNoteWait = 240;
}

// Every overworld frame (src/coop.c).
void CoopTalk_FieldFrame(void)
{
    if (sBusyPopupCooldown != 0)
        sBusyPopupCooldown--;
    if (sBusyNoteWait != 0 && (--sBusyNoteWait == 0 || ShowBusyNoteNow()))
        sBusyNoteWait = 0;
    if (sHost.active || sHost.joinerLost)
    {
        if (!Coop_SessionActive())
        {
            if (sHost.joiner)
                sHost.joinerLost = TRUE;
            sHost.joiner = FALSE;
            sHost.outCount = 0;
        }
        else if (sHost.joinerLost && sHost.outCount == 0)
        {
            sHost.joinerLost = FALSE;
            Host_Queue(TALK_OP_END, END_GONE, 0);
        }
    }
    if (sHost.active)
    {
        // The script is over (or never ran): the conversation ends.
        if (!ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked())
            Host_End();
    }
    if (sHost.outCount != 0)
        Host_Pump();
}

// The link went down or a session started over.
void CoopTalk_Reset(void)
{
    sHost.active = FALSE;
    sHost.joiner = FALSE;
    sHost.outCount = 0;
    sHost.passOp = NULL;
    if (sJoin.state != JOIN_NONE)
        sJoin.state = JOIN_NONE;
    sJoin.count = 0;
}

// A shared conversation is on (host with a joiner, or joined): the partner
// marker stays visible over the message box (their tick or bubble).
bool8 CoopTalk_Shared(void)
{
    return (sHost.active && sHost.joiner) || sJoin.state == JOIN_JOINED;
}

// The NPC this game is talking to as host (0: none). The frame's talk id
// stays on it through a question menu, where the script itself is paused.
u8 CoopTalk_HostNpc(void)
{
    return sHost.active ? sHost.npcLocalId : 0;
}
