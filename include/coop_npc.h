#ifndef GUARD_COOP_NPC_H
#define GUARD_COOP_NPC_H

// Shared NPC movement (src/coop_npc.c). On a map both players are on, one
// game (the movement authority) runs the random walkers and turners and the
// other replays their moves by stable local id.

#define COOP_NPC_ROLE_NONE   0
#define COOP_NPC_ROLE_AUTH   1
#define COOP_NPC_ROLE_FOLLOW 2

struct CoopNpcStats
{
    u8 role;
    u8 synced;       // objects this game drives from the partner's moves
    u8 partnerTalk;  // local id the partner is talking to (0 none)
    u8 myTalk;       // local id this player is talking to (0 none)
    u16 sent;
    u16 received;
    u16 applied;     // remote moves replayed here
    u16 corrected;   // small walks to realign after a missed move
    u16 snapped;     // larger realignments (placed directly)
    u16 dropped;     // frames for another map, bad checksums, overflow
    u16 roleChanges;
    u16 busyAnswers; // "busy" replies shown to this player
    u16 seqGaps;
    u16 deferred;    // frames a replay waited (local player in the way, a script)
};
extern struct CoopNpcStats gCoopNpcStats;

void CoopNpc_FieldFrame(void);
void CoopNpc_Reset(void);
bool8 CoopNpc_BlocksLocalMovement(struct ObjectEvent *objectEvent);
void CoopNpc_NoteSingleMovement(struct ObjectEvent *objectEvent);
bool8 CoopNpc_TakeOutgoing(u16 words[8]);
bool8 CoopNpc_TryHandleFrame(const u16 words[8]);
const u8 *CoopNpc_InteractionScript(u8 objectEventId, const u8 *script);
void CoopNpc_NoteTalkStart(u8 objectEventId);
u16 CoopNpc_BufferBusy(void);

extern const u8 Coop_EventScript_NpcBusy[];

#endif // GUARD_COOP_NPC_H
