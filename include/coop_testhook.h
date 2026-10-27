#ifndef GUARD_COOP_TESTHOOK_H
#define GUARD_COOP_TESTHOOK_H

// Harness-only test mailbox (see src/coop_testhook.c). Not reachable in play.
#define COOP_TEST_MAGIC 0x7E57C0DE

enum {
    COOP_TEST_NONE,
    COOP_TEST_SET_MON,    // slot, species, level, move1..move4 (0: level-up moves)
    COOP_TEST_SET_LEVEL,  // slot, level, 1 = also its level-up moves for that level
    COOP_TEST_HEAL,
    COOP_TEST_GIVE_ITEM,  // item, count
    COOP_TEST_SEND_REPLY, // kind, seq: a co-op reply block on the link (also in battle)
};

struct CoopTestMailbox
{
    u32 magic;
    u16 cmd;
    u16 arg[8];
    u16 result;
};

extern struct CoopTestMailbox gCoopTestMailbox;

void CoopTest_Poll(void);

#endif // GUARD_COOP_TESTHOOK_H
