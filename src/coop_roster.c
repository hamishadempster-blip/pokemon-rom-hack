// Roster additions that need code: the eight starter guests (one visitor per
// region in a POKéMON CENTER, each giving one partner POKéMON a day once the
// player has enough badges), and fossil revival for every fossil.
#include "global.h"
#include "coop_roster.h"
#include "event_data.h"
#include "pokemon.h"
#include "item.h"
#include "string_util.h"
#include "constants/flags.h"
#include "constants/species.h"
#include "constants/items.h"
#include "constants/vars.h"

struct StarterGuest
{
    const u8 *region;
    u8 badges;
    u8 level;
    u16 flag;
    u16 species[3];
};

static const struct StarterGuest sStarterGuests[COOP_STARTER_GUEST_COUNT] = {
    [COOP_STARTER_GUEST_KANTO]  = { COMPOUND_STRING("KANTO"),  1, 10, FLAG_DAILY_STARTER_KANTO,  { SPECIES_BULBASAUR, SPECIES_CHARMANDER, SPECIES_SQUIRTLE } },
    [COOP_STARTER_GUEST_SINNOH] = { COMPOUND_STRING("SINNOH"), 4, 18, FLAG_DAILY_STARTER_SINNOH, { SPECIES_TURTWIG, SPECIES_CHIMCHAR, SPECIES_PIPLUP } },
    [COOP_STARTER_GUEST_UNOVA]  = { COMPOUND_STRING("UNOVA"),  6, 25, FLAG_DAILY_STARTER_UNOVA,  { SPECIES_SNIVY, SPECIES_TEPIG, SPECIES_OSHAWOTT } },
    [COOP_STARTER_GUEST_KALOS]  = { COMPOUND_STRING("KALOS"),  7, 30, FLAG_DAILY_STARTER_KALOS,  { SPECIES_CHESPIN, SPECIES_FENNEKIN, SPECIES_FROAKIE } },
    [COOP_STARTER_GUEST_ALOLA]  = { COMPOUND_STRING("ALOLA"),  2, 14, FLAG_DAILY_STARTER_ALOLA,  { SPECIES_ROWLET, SPECIES_LITTEN, SPECIES_POPPLIO } },
    [COOP_STARTER_GUEST_GALAR]  = { COMPOUND_STRING("GALAR"),  5, 22, FLAG_DAILY_STARTER_GALAR,  { SPECIES_GROOKEY, SPECIES_SCORBUNNY, SPECIES_SOBBLE } },
    [COOP_STARTER_GUEST_PALDEA] = { COMPOUND_STRING("PALDEA"), 8, 35, FLAG_DAILY_STARTER_PALDEA, { SPECIES_SPRIGATITO, SPECIES_FUECOCO, SPECIES_QUAXLY } },
    [COOP_STARTER_GUEST_HOENN]  = { COMPOUND_STRING("HOENN"),  3, 16, FLAG_DAILY_STARTER_HOENN,  { SPECIES_TREECKO, SPECIES_TORCHIC, SPECIES_MUDKIP } },
};

static u8 CountBadges(void)
{
    u8 i, n = 0;

    for (i = 0; i < 8; i++)
    {
        if (FlagGet(FLAG_BADGE01_GET + i))
            n++;
    }
    return n;
}

// VAR_0x8004: the guest. STR_VAR_1 the region, STR_VAR_2 the badges needed,
// STR_VAR_3 nothing. VAR_RESULT 0: not enough badges yet, 1: given today,
// 2: one is ready (STR_VAR_1..3 then hold the three names, region in
// gStringVar4's place is not used).
u16 CoopRoster_StarterGuestOffer(void)
{
    u16 guest = gSpecialVar_0x8004;
    const struct StarterGuest *g;

    if (guest >= COOP_STARTER_GUEST_COUNT)
        return 0;
    g = &sStarterGuests[guest];
    StringCopy(gStringVar1, g->region);
    ConvertIntToDecimalStringN(gStringVar2, g->badges, STR_CONV_MODE_LEFT_ALIGN, 1);
    if (CountBadges() < g->badges)
        return 0;
    if (FlagGet(g->flag))
        return 1;
    return 2;
}

// After the region text: the three names for the menu.
u16 CoopRoster_BufferStarterNames(void)
{
    u16 guest = gSpecialVar_0x8004;
    const struct StarterGuest *g;

    if (guest >= COOP_STARTER_GUEST_COUNT)
        return 0;
    g = &sStarterGuests[guest];
    StringCopy(gStringVar1, GetSpeciesName(g->species[0]));
    StringCopy(gStringVar2, GetSpeciesName(g->species[1]));
    StringCopy(gStringVar3, GetSpeciesName(g->species[2]));
    return 0;
}

// VAR_0x8005: the choice. VAR_0x8007 gets the level. Returns the species.
u16 CoopRoster_StarterChoice(void)
{
    u16 guest = gSpecialVar_0x8004;

    if (guest >= COOP_STARTER_GUEST_COUNT || gSpecialVar_0x8005 > 2)
        return SPECIES_NONE;
    gSpecialVar_0x8007 = sStarterGuests[guest].level;
    StringCopy(gStringVar1, GetSpeciesName(sStarterGuests[guest].species[gSpecialVar_0x8005]));
    return sStarterGuests[guest].species[gSpecialVar_0x8005];
}

void CoopRoster_StarterTaken(void)
{
    if (gSpecialVar_0x8004 < COOP_STARTER_GUEST_COUNT)
        FlagSet(sStarterGuests[gSpecialVar_0x8004].flag);
}

// Fossils the Devon scientist revives. VAR_WHICH_FOSSIL_REVIVED keeps the
// stock values 1 (Root) and 2 (Claw); the rest follow this table from 3.
static const u16 sFossils[][2] = {
    { ITEM_HELIX_FOSSIL, SPECIES_OMANYTE },
    { ITEM_DOME_FOSSIL,  SPECIES_KABUTO },
    { ITEM_OLD_AMBER,    SPECIES_AERODACTYL },
    { ITEM_SKULL_FOSSIL, SPECIES_CRANIDOS },
    { ITEM_ARMOR_FOSSIL, SPECIES_SHIELDON },
    { ITEM_COVER_FOSSIL, SPECIES_TIRTOUGA },
    { ITEM_PLUME_FOSSIL, SPECIES_ARCHEN },
    { ITEM_JAW_FOSSIL,   SPECIES_TYRUNT },
    { ITEM_SAIL_FOSSIL,  SPECIES_AMAURA },
};

#define FOSSIL_FIRST_EXTRA 3

// The Galar fossils revive in pairs (one upper and one lower half). In the
// Fossil Maniac's and the scientist's lists a pair is a single entry with a
// script value from FOSSIL_COMBO_VALUE; its revival number follows sFossils.
#define FOSSIL_COMBO_VALUE 1001
static const u16 sFossilPairs[][3] = {
    { ITEM_FOSSILIZED_BIRD, ITEM_FOSSILIZED_DRAKE, SPECIES_DRACOZOLT },
    { ITEM_FOSSILIZED_BIRD, ITEM_FOSSILIZED_DINO,  SPECIES_ARCTOZOLT },
    { ITEM_FOSSILIZED_FISH, ITEM_FOSSILIZED_DRAKE, SPECIES_DRACOVISH },
    { ITEM_FOSSILIZED_FISH, ITEM_FOSSILIZED_DINO,  SPECIES_ARCTOVISH },
};
#define FOSSIL_FIRST_PAIR (FOSSIL_FIRST_EXTRA + ARRAY_COUNT(sFossils))

static s32 FossilPairIndex(u16 value)
{
    if (value >= FOSSIL_COMBO_VALUE && value < FOSSIL_COMBO_VALUE + ARRAY_COUNT(sFossilPairs))
        return value - FOSSIL_COMBO_VALUE;
    return -1;
}

// VAR_0x8004: a fossil item or a pair value. Gives it (both halves for a
// pair). VAR_RESULT-style return: TRUE if it fit in the bag.
u16 CoopRoster_GiveFossil(void)
{
    s32 pair = FossilPairIndex(gSpecialVar_0x8004);

    if (pair < 0)
        return AddBagItem(gSpecialVar_0x8004, 1);
    if (!CheckBagHasSpace(sFossilPairs[pair][0], 1) || !CheckBagHasSpace(sFossilPairs[pair][1], 1))
        return FALSE;
    AddBagItem(sFossilPairs[pair][0], 1);
    AddBagItem(sFossilPairs[pair][1], 1);
    return TRUE;
}

// VAR_0x8004: a pair value. TRUE if both halves are in the bag.
u16 CoopRoster_FossilPairReady(void)
{
    s32 pair = FossilPairIndex(gSpecialVar_0x8004);

    return pair >= 0 && CheckBagHasItem(sFossilPairs[pair][0], 1) && CheckBagHasItem(sFossilPairs[pair][1], 1);
}

// VAR_0x8004: a fossil item or a pair value. Removes it (both halves) and
// buffers its name in STR_VAR_1.
u16 CoopRoster_TakeFossil(void)
{
    s32 pair = FossilPairIndex(gSpecialVar_0x8004);

    if (pair < 0)
    {
        CopyItemName(gSpecialVar_0x8004, gStringVar1);
        RemoveBagItem(gSpecialVar_0x8004, 1);
        return 0;
    }
    {
        static const u8 *const sPairNames[] = {
            COMPOUND_STRING("BIRD and DRAKE"), COMPOUND_STRING("BIRD and DINO"),
            COMPOUND_STRING("FISH and DRAKE"), COMPOUND_STRING("FISH and DINO"),
        };
        StringCopy(gStringVar1, sPairNames[pair]);
    }
    RemoveBagItem(sFossilPairs[pair][0], 1);
    RemoveBagItem(sFossilPairs[pair][1], 1);
    return 0;
}

// VAR_0x8004: a fossil item. Returns its VAR_WHICH_FOSSIL_REVIVED value (0
// for anything that is not a fossil) and buffers its name in STR_VAR_1.
u16 CoopRoster_FossilId(void)
{
    u32 i;

    if (gSpecialVar_0x8004 == ITEM_ROOT_FOSSIL)
        return 1;
    if (gSpecialVar_0x8004 == ITEM_CLAW_FOSSIL)
        return 2;
    for (i = 0; i < ARRAY_COUNT(sFossils); i++)
        if (sFossils[i][0] == gSpecialVar_0x8004)
            return FOSSIL_FIRST_EXTRA + i;
    if (FossilPairIndex(gSpecialVar_0x8004) >= 0)
        return FOSSIL_FIRST_PAIR + FossilPairIndex(gSpecialVar_0x8004);
    return 0;
}

// VAR_WHICH_FOSSIL_REVIVED (3 and up): the species, its name in STR_VAR_1.
u16 CoopRoster_FossilSpecies(void)
{
    u16 which = VarGet(VAR_WHICH_FOSSIL_REVIVED);

    if (which >= FOSSIL_FIRST_PAIR && which < FOSSIL_FIRST_PAIR + ARRAY_COUNT(sFossilPairs))
    {
        StringCopy(gStringVar1, GetSpeciesName(sFossilPairs[which - FOSSIL_FIRST_PAIR][2]));
        return sFossilPairs[which - FOSSIL_FIRST_PAIR][2];
    }
    if (which < FOSSIL_FIRST_EXTRA || which >= FOSSIL_FIRST_EXTRA + ARRAY_COUNT(sFossils))
        return SPECIES_NONE;
    StringCopy(gStringVar1, GetSpeciesName(sFossils[which - FOSSIL_FIRST_EXTRA][1]));
    return sFossils[which - FOSSIL_FIRST_EXTRA][1];
}
