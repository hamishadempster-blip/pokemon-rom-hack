// Test helper for the two-player harness (tools/coop_link_harness).
//
// Inert in play: nothing in the game writes the mailbox, there is no menu or
// item for it, and it only acts when the harness pokes the magic value into
// EWRAM. The harness uses it for "combat assistance" in long playthrough
// runs (stronger parties, healing, ordinary battle items), always through
// the game's own creation, stat and move functions. It never touches story
// flags, badges, trainers or key items.
#include "global.h"
#include "coop_testhook.h"
#include "item.h"
#include "main.h"
#include "random.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "constants/items.h"

EWRAM_DATA struct CoopTestMailbox gCoopTestMailbox = {0};

static bool8 TestHook_SetMon(void)
{
    struct Pokemon mon;
    u32 slot = gCoopTestMailbox.arg[0], species = gCoopTestMailbox.arg[1], level = gCoopTestMailbox.arg[2];
    u32 i;

    if (slot >= PARTY_SIZE || species == SPECIES_NONE || species >= NUM_SPECIES || level < 1 || level > MAX_LEVEL)
        return FALSE;
    CreateMonWithIVs(&mon, species, level, Random32(), OTID_STRUCT_PLAYER_ID, MAX_PER_STAT_IVS);
    GiveMonInitialMoveset(&mon);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gCoopTestMailbox.arg[3 + i] != MOVE_NONE && gCoopTestMailbox.arg[3 + i] < MOVES_COUNT)
            SetMonMoveSlot(&mon, gCoopTestMailbox.arg[3 + i], i);
    }
    CalculateMonStats(&mon);
    gPlayerParty[slot] = mon;
    CalculatePlayerPartyCount();
    return TRUE;
}

static bool8 TestHook_SetLevel(void)
{
    struct Pokemon *mon;
    u32 slot = gCoopTestMailbox.arg[0], level = gCoopTestMailbox.arg[1], species, exp;

    if (slot >= PARTY_SIZE || level < 1 || level > MAX_LEVEL)
        return FALSE;
    mon = &gPlayerParty[slot];
    species = GetMonData(mon, MON_DATA_SPECIES);
    if (species == SPECIES_NONE)
        return FALSE;
    exp = gExperienceTables[gSpeciesInfo[species].growthRate][level];
    SetMonData(mon, MON_DATA_EXP, &exp);
    CalculateMonStats(mon);
    // arg 2 = 1: the moves it would know at that level (its last four
    // level-up moves), as for a freshly met Pokemon of that level.
    if (gCoopTestMailbox.arg[2] == 1)
        GiveMonInitialMoveset(mon);
    return TRUE;
}

void CoopTest_Poll(void)
{
    bool8 ok = FALSE;

    if (gCoopTestMailbox.magic != COOP_TEST_MAGIC || gMain.inBattle)
        return;
    switch (gCoopTestMailbox.cmd)
    {
    case COOP_TEST_SET_MON:
        ok = TestHook_SetMon();
        break;
    case COOP_TEST_SET_LEVEL:
        ok = TestHook_SetLevel();
        break;
    case COOP_TEST_HEAL:
        HealPlayerParty();
        ok = TRUE;
        break;
    case COOP_TEST_GIVE_ITEM:
        ok = gCoopTestMailbox.arg[0] != ITEM_NONE && gCoopTestMailbox.arg[0] < ITEMS_COUNT
          && AddBagItem(gCoopTestMailbox.arg[0], gCoopTestMailbox.arg[1]);
        break;
    }
    gCoopTestMailbox.result = ok ? 1 : 2;
    gCoopTestMailbox.magic = 0;
}
