#include "global.h"
#include "battle.h"
#include "coop.h"
#include "coop_teams.h"
#include "battle_setup.h"
#include "data.h"
#include "constants/rematches.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "constants/opponents.h"

// Battle Guide v2 team selection (tools/coop_trainers/guide.py).
//
// A trainer id keeps its trainer flag, name, class and intro; only the party
// it fields can depend on how far the player is (pre-Winona arrivals, Gabby
// and Ty's tiers) or on the saved starter case (the rival exhibition).
// The choice is made once, when the battle's parties are built
// (DoTrainerBattle), never during the battle. Solo: this save. Co-op: the
// lower progress tier of the two players and the inviter's starter case,
// carried by the invite and its answer (Coop_TeamKeyLocal), so both games
// build the same parties.

#define COOP_TEAMSEL_ALWAYS     0
#define COOP_TEAMSEL_BELOW_TIER 1
#define COOP_TEAMSEL_CASE       2
#define COOP_TEAMSEL_CLAMP      3

struct CoopTeamVariant
{
    u16 base;
    u8 how;
    u8 arg;
    u16 party;
};

#include "data/coop_team_variants.h"

static const u8 sTierCap[COOP_TIER_COUNT] = { 18, 24, 30, 36, 42, 48, 54, 60, 66, 76, 80 };

// The key of the co-op battle being started (0 = none: use this save).
EWRAM_DATA u16 gCoopBattleTeamKey = 0;
// The co-op partner owns a Mega Bracelet (from the same invite or answer).
EWRAM_DATA u8 gCoopPartnerHasBracelet = 0;
// Test visibility: co-op Mega requests the master refused (stale or invalid).
EWRAM_DATA u8 gCoopMegaRejected = 0;
// Test visibility: the last party choice made (base, party, key).
EWRAM_DATA u16 gCoopTeamLastBase = 0;
EWRAM_DATA u16 gCoopTeamLastParty = 0;
EWRAM_DATA u16 gCoopTeamLastKey = 0;

static const u16 sBadgeFlags[8] = {
    FLAG_BADGE01_GET, FLAG_BADGE02_GET, FLAG_BADGE03_GET, FLAG_BADGE04_GET,
    FLAG_BADGE05_GET, FLAG_BADGE06_GET, FLAG_BADGE07_GET, FLAG_BADGE08_GET,
};

// 0 before the first badge, 1-8 by badges (in order, as the caps), 9 once
// the League has admitted this player (kept after a loss), 10 Champion.
u8 Coop_ProgressTier(void)
{
    u32 i;

    if (FlagGet(FLAG_IS_CHAMPION))
        return COOP_TIER_CHAMPION;
    for (i = 0; i < ARRAY_COUNT(sBadgeFlags); i++)
    {
        if (!FlagGet(sBadgeFlags[i]))
            return i;
    }
    if (FlagGet(FLAG_ENTERED_ELITE_FOUR))
        return COOP_TIER_LEAGUE;
    return 8;
}

u8 Coop_TierLevelCap(u8 tier)
{
    if (tier >= COOP_TIER_COUNT)
        tier = COOP_TIER_COUNT - 1;
    return sTierCap[tier];
}

static u8 Coop_SavedCase(void)
{
    u16 c = VarGet(VAR_STARTER_MON);

    return c > 2 ? 0 : c;
}

u16 Coop_TeamKeyLocal(void)
{
    return COOP_TEAMKEY_VALID | (Coop_ProgressTier() & COOP_TEAMKEY_TIER_MASK)
         | ((Coop_SavedCase() & 3) << COOP_TEAMKEY_CASE_SHIFT)
         | (CheckBagHasItem(ITEM_MEGA_RING, 1) ? COOP_TEAMKEY_BRACELET : 0);
}

// Both games call this with the same two keys: the inviter's and the
// guest's. Tier: the lower of the two. Case: the inviter's.
u16 Coop_TeamKeyMerge(u16 inviter, u16 guest)
{
    u16 tier;

    if (!(inviter & COOP_TEAMKEY_VALID))
        return guest;
    if (!(guest & COOP_TEAMKEY_VALID))
        return inviter;
    tier = inviter & COOP_TEAMKEY_TIER_MASK;
    if ((guest & COOP_TEAMKEY_TIER_MASK) < tier)
        tier = guest & COOP_TEAMKEY_TIER_MASK;
    return COOP_TEAMKEY_VALID | tier | (inviter & (3 << COOP_TEAMKEY_CASE_SHIFT));  // no bracelet bit: per player
}

void Coop_SetBattleTeamKey(u16 key)
{
    gCoopBattleTeamKey = key;
}

static u16 Coop_ActiveTeamKey(void)
{
    if ((gBattleTypeFlags & BATTLE_TYPE_COOP) && (gCoopBattleTeamKey & COOP_TEAMKEY_VALID))
        return gCoopBattleTeamKey;
    return Coop_TeamKeyLocal();
}

static bool8 RuleMatches(const struct CoopTeamVariant *v, u16 key)
{
    switch (v->how)
    {
    case COOP_TEAMSEL_ALWAYS:
        return TRUE;
    case COOP_TEAMSEL_BELOW_TIER:
        return (key & COOP_TEAMKEY_TIER_MASK) < v->arg;
    case COOP_TEAMSEL_CASE:
        return ((key >> COOP_TEAMKEY_CASE_SHIFT) & 3) == v->arg;
    default:
        return FALSE;
    }
}

static u8 PartyMaxLevel(u16 trainerId)
{
    const struct Trainer *trainer = GetTrainerStructFromId(trainerId);
    u8 top = 0;
    u32 i;

    for (i = 0; i < trainer->partySize; i++)
    {
        if (trainer->party[i].lvl > top)
            top = trainer->party[i].lvl;
    }
    return top;
}

// Ordinary trainers' rematch tiers (gRematchTable, before Wally): the stock
// game moves to the next tier each time one is beaten, so repeating fights
// could bring out a team above the players' cap. The tier fielded is the
// highest one, up to the one asked for, whose levels fit the cap. The tier
// asked for still gets its trainer flag (the repeat count moves on).
static u16 Coop_GateRematchTier(u16 trainerId, u16 key)
{
    u8 cap = Coop_TierLevelCap(key & COOP_TEAMKEY_TIER_MASK);
    s32 i, j;

    for (i = 0; i < REMATCH_SPECIAL_TRAINER_START; i++)
    {
        for (j = 1; j < REMATCHES_COUNT; j++)
        {
            if (gRematchTable[i].trainerIds[j] == trainerId)
            {
                while (j > 0 && PartyMaxLevel(gRematchTable[i].trainerIds[j]) > cap)
                    j--;
                return gRematchTable[i].trainerIds[j];
            }
        }
    }
    return trainerId;
}

u16 Coop_TeamPartyIdForKey(u16 trainerId, u16 key)
{
    u32 step, i;

    trainerId = Coop_GateRematchTier(trainerId, key);

    for (step = 0; step < 8; step++)
    {
        for (i = 0; i < ARRAY_COUNT(sCoopTeamVariants); i++)
        {
            if (sCoopTeamVariants[i].base == trainerId && RuleMatches(&sCoopTeamVariants[i], key))
                break;
        }
        if (i == ARRAY_COUNT(sCoopTeamVariants) || sCoopTeamVariants[i].party == trainerId)
            break;
        trainerId = sCoopTeamVariants[i].party;
    }
    return trainerId;
}

u16 Coop_TeamPartyId(u16 trainerId)
{
    u16 key = Coop_ActiveTeamKey();
    u16 party = Coop_TeamPartyIdForKey(trainerId, key);

    gCoopTeamLastBase = trainerId;
    gCoopTeamLastParty = party;
    gCoopTeamLastKey = key;
    return party;
}

// A team whose lowest tier the players have not reached is fielded at their
// cap instead (Gabby and Ty's first tier; generated as COOP_TEAMSEL_CLAMP).
void Coop_ClampTeamLevels(struct Pokemon *party, u16 partyId)
{
    u16 key = Coop_ActiveTeamKey();
    u8 tier = key & COOP_TEAMKEY_TIER_MASK;
    u32 i, j;
    u8 cap;

    for (i = 0; i < ARRAY_COUNT(sCoopTeamVariants); i++)
    {
        if (sCoopTeamVariants[i].base == partyId && sCoopTeamVariants[i].how == COOP_TEAMSEL_CLAMP && tier < sCoopTeamVariants[i].arg)
            break;
    }
    if (i == ARRAY_COUNT(sCoopTeamVariants))
        return;
    cap = Coop_TierLevelCap(tier);
    for (j = 0; j < PARTY_SIZE; j++)
    {
        u16 species = GetMonData(&party[j], MON_DATA_SPECIES);

        if (species == SPECIES_NONE)
            continue;
        if (GetMonData(&party[j], MON_DATA_LEVEL) > cap)
        {
            u32 exp = gExperienceTables[gSpeciesInfo[species].growthRate][cap];

            SetMonData(&party[j], MON_DATA_EXP, &exp);
            CalculateMonStats(&party[j]);
        }
    }
}
