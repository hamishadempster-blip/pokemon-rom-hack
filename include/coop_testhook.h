#ifndef GUARD_COOP_TESTHOOK_H
#define GUARD_COOP_TESTHOOK_H

// Harness-only test mailbox (see src/coop_testhook.c). Not reachable in play.
#define COOP_TEST_MAGIC 0x7E57C0DE

enum {
    COOP_TEST_NONE,
    COOP_TEST_SET_MON,    // slot, species, level, move1..move4 (0: level-up moves)
    COOP_TEST_SET_LEVEL,  // slot, level, 1 = also its level-up moves for that level, met level (0: keep)
    COOP_TEST_HEAL,
    COOP_TEST_GIVE_ITEM,  // item, count
    COOP_TEST_SEND_REPLY, // kind, seq: a co-op reply block on the link (also in battle)
    COOP_TEST_SET_DATA,   // slot, friendship (test_friendship)
    COOP_TEST_GET_DATA,   // slot, field (COOP_TEST_FIELD_*), arg, source (0 party, 1 Day Care, 2/3 the opposing trainers' parties): value
    COOP_TEST_TEAM_FOR,   // trainer id, team key: the party id Coop_TeamPartyIdForKey picks (read-only)
    COOP_TEST_BATTLE_NAME, // battler: "{B_ATK_TRAINER_NAME}" for that battler into gStringVar4 (in battle, read-only)
};

struct CoopTestMailbox
{
    u32 magic;
    u16 cmd;
    u16 arg[8];
    u16 result;
    u32 value;            // COOP_TEST_GET_DATA's answer
};

enum {
    COOP_TEST_FIELD_SPECIES, COOP_TEST_FIELD_LEVEL, COOP_TEST_FIELD_NATURE, COOP_TEST_FIELD_MINT_NATURE,
    COOP_TEST_FIELD_ABILITY_NUM, COOP_TEST_FIELD_EV_TOTAL, COOP_TEST_FIELD_HYPER_MASK, COOP_TEST_FIELD_IV,
    COOP_TEST_FIELD_HELD, COOP_TEST_FIELD_EGG, COOP_TEST_FIELD_PERSONALITY, COOP_TEST_FIELD_MOVE,
    COOP_TEST_FIELD_FRIENDSHIP, COOP_TEST_FIELD_GENDER, COOP_TEST_FIELD_EV, COOP_TEST_FIELD_OTID,
    COOP_TEST_FIELD_PERFECT_IVS, COOP_TEST_FIELD_EXP, COOP_TEST_FIELD_HAS_MOVE, COOP_TEST_FIELD_STAT,
    COOP_TEST_FIELD_DAYCARE_STEPS, // daycare: slot 0/1 that Pokemon's steps, 2 the egg step counter, 3 eggs waiting (offspring personality != 0)
    COOP_TEST_FIELD_DEX_CAUGHT,    // this save's Pokedex caught flag for that Pokemon's species
    COOP_TEST_FIELD_HP,            // current HP (party Pokemon)
    COOP_TEST_FIELD_PP,            // current PP of move slot `arg`
};

extern struct CoopTestMailbox gCoopTestMailbox;

// The developer start (skip to the clock in your room) is offered on NEW
// GAME only while the harness has written this value here. A release
// player never sees it: nothing in the game writes it.
#define COOP_TEST_START_ARMED 0x57A27ED0
extern u32 gCoopTestStartArmed;

// Test fixture: while the harness has written COOP_TEST_FORCE_SHINY here,
// newly made wild and gift Pokemon are shiny (src/pokemon.c). Nothing in the
// game writes it.
#define COOP_TEST_FORCE_SHINY 0x5A1A7E57
extern u32 gCoopTestForceShiny;

void CoopTest_Poll(void);

#endif // GUARD_COOP_TESTHOOK_H
