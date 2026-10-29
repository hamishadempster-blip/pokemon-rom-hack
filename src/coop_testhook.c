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
#include "coop.h"
#include "coop_teams.h"
#include "battle.h"
#include "item.h"
#include "main.h"
#include "random.h"
#include "pokemon.h"
#include "pokedex.h"
#include "script_pokemon_util.h"
#include "daycare.h"
#include "pokemon_storage_system.h"
#include "constants/items.h"

EWRAM_DATA struct CoopTestMailbox gCoopTestMailbox = {0};
EWRAM_DATA u32 gCoopTestStartArmed = 0;

#define TEST_HOOK_MET_LEVEL 5

static bool8 TestHook_SetMon(void)
{
    struct Pokemon mon;
    u32 slot = gCoopTestMailbox.arg[0], species = gCoopTestMailbox.arg[1], level = gCoopTestMailbox.arg[2];
    u32 i;

    if (slot >= PARTY_SIZE || species == SPECIES_NONE || species >= NUM_SPECIES || level < 1 || level > MAX_LEVEL)
        return FALSE;
    // arg 7, bits 4-5: 1 male, 2 female, 3 a personality divisible by 100
    // (Dunsparce's rare evolution); bits 8-12: a nature + 1. Personalities
    // are drawn until one fits, as a wild Pokemon's would be.
    {
        u32 want = (gCoopTestMailbox.arg[7] >> 4) & 3, nature = (gCoopTestMailbox.arg[7] >> 8) & 0x1F;
        u32 tries, personality = Random32();

        for (tries = 0; tries < 200000; tries++)
        {
            bool32 fits = TRUE;
            if (want == 1 && GetGenderFromSpeciesAndPersonality(species, personality) != MON_MALE)
                fits = FALSE;
            if (want == 2 && GetGenderFromSpeciesAndPersonality(species, personality) != MON_FEMALE)
                fits = FALSE;
            if (want == 3 && personality % 100 != 0)
                fits = FALSE;
            if (nature != 0 && GetNatureFromPersonality(personality) != nature - 1)
                fits = FALSE;
            if (fits)
                break;
            personality = Random32();
        }
        CreateMonWithIVs(&mon, species, level, personality, OTID_STRUCT_PLAYER_ID, MAX_PER_STAT_IVS);
    }
    GiveMonInitialMoveset(&mon);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gCoopTestMailbox.arg[3 + i] != MOVE_NONE && gCoopTestMailbox.arg[3 + i] < MOVES_COUNT)
            SetMonMoveSlot(&mon, gCoopTestMailbox.arg[3 + i], i);
    }
    // Met at a low level, as a Pokemon the player raised: newer obedience
    // rules compare the met level with the badges.
    {
        u8 metLevel = TEST_HOOK_MET_LEVEL;
        SetMonData(&mon, MON_DATA_MET_LEVEL, &metLevel);
    }
    // arg 7: an ability slot + 1 (0 keeps the one the personality gave).
    if ((gCoopTestMailbox.arg[7] & 0xF) >= 1 && (gCoopTestMailbox.arg[7] & 0xF) <= NUM_ABILITY_SLOTS)
    {
        u8 abilityNum = (gCoopTestMailbox.arg[7] & 0xF) - 1;
        SetMonData(&mon, MON_DATA_ABILITY_NUM, &abilityNum);
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
    // arg 3: a met level to record (a Pokemon made by SET_MON before it
    // recorded a low one).
    if (gCoopTestMailbox.arg[3] != 0 && gCoopTestMailbox.arg[3] <= MAX_LEVEL)
    {
        u8 metLevel = gCoopTestMailbox.arg[3];
        SetMonData(mon, MON_DATA_MET_LEVEL, &metLevel);
    }
    return TRUE;
}

// COOP_TEST_GET_DATA: one field of a party Pokemon or a Day Care Pokemon,
// read with the game's own accessors (nothing written).
static bool8 TestHook_GetData(void)
{
    u32 slot = gCoopTestMailbox.arg[0], field = gCoopTestMailbox.arg[1], arg = gCoopTestMailbox.arg[2];
    struct BoxPokemon *box;
    struct Pokemon *mon = NULL;
    u32 v = 0, i;

    if (field == COOP_TEST_FIELD_DAYCARE_STEPS)
    {
        struct DayCare *daycare = &gSaveBlock1Ptr->daycare;
        if (slot < DAYCARE_MON_COUNT)
            v = daycare->mons[slot].steps;
        else if (slot == 2)
            v = daycare->stepCounter;
        else
            v = daycare->offspringPersonality != 0;
        gCoopTestMailbox.value = v;
        return TRUE;
    }
    if (gCoopTestMailbox.arg[3] == 1)
    {
        if (slot >= DAYCARE_MON_COUNT)
            return FALSE;
        box = &gSaveBlock1Ptr->daycare.mons[slot].mon;
    }
    else if (gCoopTestMailbox.arg[3] == 2 || gCoopTestMailbox.arg[3] == 3)
    {
        // The opposing trainers' parties as built for the battle (read-only).
        if (slot >= PARTY_SIZE)
            return FALSE;
        mon = &gParties[gCoopTestMailbox.arg[3] == 2 ? B_TRAINER_OPPONENT_A : B_TRAINER_OPPONENT_B][slot];
        box = &mon->box;
    }
    else
    {
        if (slot >= PARTY_SIZE)
            return FALSE;
        mon = &gPlayerParty[slot];
        box = &mon->box;
    }
    if (field != COOP_TEST_FIELD_SPECIES && GetBoxMonData(box, MON_DATA_SPECIES) == SPECIES_NONE)
        return FALSE;
    switch (field)
    {
    case COOP_TEST_FIELD_SPECIES:     v = GetBoxMonData(box, MON_DATA_SPECIES); break;
    case COOP_TEST_FIELD_LEVEL:       v = mon != NULL ? GetMonData(mon, MON_DATA_LEVEL) : GetLevelFromBoxMonExp(box); break;
    case COOP_TEST_FIELD_NATURE:      v = GetNatureFromPersonality(GetBoxMonData(box, MON_DATA_PERSONALITY)); break;
    case COOP_TEST_FIELD_MINT_NATURE: v = GetBoxMonData(box, MON_DATA_HIDDEN_NATURE); break;
    case COOP_TEST_FIELD_ABILITY_NUM: v = GetBoxMonData(box, MON_DATA_ABILITY_NUM); break;
    case COOP_TEST_FIELD_EV_TOTAL:
        for (i = 0; i < NUM_STATS; i++)
            v += GetBoxMonData(box, MON_DATA_HP_EV + i);
        break;
    case COOP_TEST_FIELD_HYPER_MASK:
        for (i = 0; i < NUM_STATS; i++)
            if (GetBoxMonData(box, MON_DATA_HYPER_TRAINED_HP + i))
                v |= 1 << i;
        break;
    case COOP_TEST_FIELD_IV:          v = arg < NUM_STATS ? GetBoxMonData(box, MON_DATA_HP_IV + arg) : 0; break;
    case COOP_TEST_FIELD_HELD:        v = GetBoxMonData(box, MON_DATA_HELD_ITEM); break;
    case COOP_TEST_FIELD_EGG:         v = GetBoxMonData(box, MON_DATA_IS_EGG); break;
    case COOP_TEST_FIELD_PERSONALITY: v = GetBoxMonData(box, MON_DATA_PERSONALITY); break;
    case COOP_TEST_FIELD_MOVE:        v = arg < MAX_MON_MOVES ? GetBoxMonData(box, MON_DATA_MOVE1 + arg) : 0; break;
    case COOP_TEST_FIELD_FRIENDSHIP:  v = GetBoxMonData(box, MON_DATA_FRIENDSHIP); break;
    case COOP_TEST_FIELD_GENDER:      v = GetBoxMonGender(box); break;
    case COOP_TEST_FIELD_EV:          v = arg < NUM_STATS ? GetBoxMonData(box, MON_DATA_HP_EV + arg) : 0; break;
    case COOP_TEST_FIELD_OTID:        v = GetBoxMonData(box, MON_DATA_OT_ID); break;
    case COOP_TEST_FIELD_PERFECT_IVS:
        for (i = 0; i < NUM_STATS; i++)
            if (GetBoxMonData(box, MON_DATA_HP_IV + i) == MAX_PER_STAT_IVS)
                v++;
        break;
    case COOP_TEST_FIELD_EXP:         v = GetBoxMonData(box, MON_DATA_EXP); break;
    case COOP_TEST_FIELD_HAS_MOVE:
        for (i = 0; i < MAX_MON_MOVES; i++)
            if (GetBoxMonData(box, MON_DATA_MOVE1 + i) == arg)
                v = 1;
        break;
    case COOP_TEST_FIELD_STAT:        v = (mon != NULL && arg < NUM_STATS) ? GetMonData(mon, MON_DATA_MAX_HP + arg) : 0; break;
    case COOP_TEST_FIELD_DEX_CAUGHT:
        v = GetSetPokedexFlag(SpeciesToNationalPokedexNum(GetBoxMonData(box, MON_DATA_SPECIES)), FLAG_GET_CAUGHT) ? 1 : 0;
        break;
    default:
        return FALSE;
    }
    gCoopTestMailbox.value = v;
    return TRUE;
}

void CoopTest_Poll(void)
{
    bool8 ok = FALSE;

    if (gCoopTestMailbox.magic != COOP_TEST_MAGIC)
        return;
    // Link fixture, served in battles too: a co-op reply block sent as the
    // game sends one (waits while another block is in flight).
    if (gCoopTestMailbox.cmd == COOP_TEST_SEND_REPLY)
    {
        if (!Coop_TestSendReply(gCoopTestMailbox.arg[0], gCoopTestMailbox.arg[1]))
            return;
        gCoopTestMailbox.result = 1;
        gCoopTestMailbox.magic = 0;
        return;
    }
    // Read-only questions are answered in battles too.
    if (gCoopTestMailbox.cmd == COOP_TEST_GET_DATA || gCoopTestMailbox.cmd == COOP_TEST_TEAM_FOR)
    {
        if (gCoopTestMailbox.cmd == COOP_TEST_TEAM_FOR)
        {
            gCoopTestMailbox.value = Coop_TeamPartyIdForKey(gCoopTestMailbox.arg[0], gCoopTestMailbox.arg[1]);
            ok = TRUE;
        }
        else
        {
            ok = TestHook_GetData();
        }
        gCoopTestMailbox.result = ok ? 1 : 2;
        gCoopTestMailbox.magic = 0;
        return;
    }
    if (gMain.inBattle)
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
    case COOP_TEST_SET_DATA:
        // Friendship (and similar plain fields) for evolution tests.
        if (gCoopTestMailbox.arg[0] >= PARTY_SIZE)
            break;
        if (gCoopTestMailbox.arg[2] == 1 && gCoopTestMailbox.arg[1] < ITEMS_COUNT)
        {
            u16 item = gCoopTestMailbox.arg[1];   // held item (test_held)
            SetMonData(&gPlayerParty[gCoopTestMailbox.arg[0]], MON_DATA_HELD_ITEM, &item);
            ok = TRUE;
        }
        else if (gCoopTestMailbox.arg[2] == 2)
        {
            // test_data: arg 3 = 1 current HP (a hurt Pokemon), 2 the
            // evolution tracker (moves used, as counted in battle).
            struct Pokemon *mon = &gPlayerParty[gCoopTestMailbox.arg[0]];
            u32 v = gCoopTestMailbox.arg[1];
            if (gCoopTestMailbox.arg[3] == 1 && v >= 1 && v <= GetMonData(mon, MON_DATA_MAX_HP))
            {
                SetMonData(mon, MON_DATA_HP, &v);
                ok = TRUE;
            }
            else if (gCoopTestMailbox.arg[3] == 2 && v <= 0x3FF)
            {
                SetMonData(mon, MON_DATA_EVOLUTION_TRACKER, &v);
                ok = TRUE;
            }
            else if (gCoopTestMailbox.arg[3] == 7 && v <= 1)
            {
                // 7: every stat HYPER TRAINED (1) or not (0).
                u32 k;
                for (k = 0; k < NUM_STATS; k++)
                    SetMonData(mon, MON_DATA_HYPER_TRAINED_HP + k, &v);
                CalculateMonStats(mon);
                ok = TRUE;
            }
            else if (gCoopTestMailbox.arg[3] == 6 && v < 0x10000)
            {
                // 6: the Day Care's step counter (the next egg cycle sooner).
                gSaveBlock1Ptr->daycare.stepCounter = v;
                ok = TRUE;
            }
            else if (gCoopTestMailbox.arg[3] == 5 && gPlayerPartyCount > 1)
            {
                // 5: empty the slot (room in a full party), as a deposit would.
                ZeroMonData(mon);
                CompactPartySlots();
                CalculatePlayerPartyCount();
                ok = TRUE;
            }
            else if ((gCoopTestMailbox.arg[3] == 3 && v <= MAX_PER_STAT_EVS) || (gCoopTestMailbox.arg[3] == 4 && v <= MAX_PER_STAT_IVS))
            {
                // 3: every EV to v (an EV spread to correct); 4: every IV to v.
                u32 k, base = gCoopTestMailbox.arg[3] == 3 ? MON_DATA_HP_EV : MON_DATA_HP_IV;
                for (k = 0; k < NUM_STATS; k++)
                    SetMonData(mon, base + k, &v);
                CalculateMonStats(mon);
                ok = TRUE;
            }
        }
        else if (gCoopTestMailbox.arg[2] == 0 && gCoopTestMailbox.arg[1] <= MAX_FRIENDSHIP)
        {
            u8 v = gCoopTestMailbox.arg[1];       // friendship (test_friendship)
            SetMonData(&gPlayerParty[gCoopTestMailbox.arg[0]], MON_DATA_FRIENDSHIP, &v);
            ok = TRUE;
        }
        break;
    case COOP_TEST_GET_DATA:
        ok = TestHook_GetData();
        break;
    case COOP_TEST_GIVE_ITEM:
        ok = gCoopTestMailbox.arg[0] != ITEM_NONE && gCoopTestMailbox.arg[0] < ITEMS_COUNT
          && AddBagItem(gCoopTestMailbox.arg[0], gCoopTestMailbox.arg[1]);
        break;
    }
    gCoopTestMailbox.result = ok ? 1 : 2;
    gCoopTestMailbox.magic = 0;
}
