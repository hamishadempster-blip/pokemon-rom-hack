// Roster additions that need code: the eight starter guests (one visitor per
// region in a POKéMON CENTER, each giving one partner POKéMON a day once the
// player has enough badges), and fossil revival for every fossil.
#include "global.h"
#include "coop_roster.h"
#include "event_data.h"
#include "pokemon.h"
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
    return 0;
}

// VAR_WHICH_FOSSIL_REVIVED (3 and up): the species, its name in STR_VAR_1.
u16 CoopRoster_FossilSpecies(void)
{
    u16 which = VarGet(VAR_WHICH_FOSSIL_REVIVED);

    if (which < FOSSIL_FIRST_EXTRA || which >= FOSSIL_FIRST_EXTRA + ARRAY_COUNT(sFossils))
        return SPECIES_NONE;
    StringCopy(gStringVar1, GetSpeciesName(sFossils[which - FOSSIL_FIRST_EXTRA][1]));
    return sFossils[which - FOSSIL_FIRST_EXTRA][1];
}
