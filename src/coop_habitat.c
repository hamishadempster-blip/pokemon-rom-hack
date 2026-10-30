// Five hidden habitats (collection update §3).
//
// Each is a small place off an ordinary map, entered through a spot that
// looks like nothing much (a gap behind trees, a crack in a rock wall, a
// current off a sandy islet, a cliff path): standing at it and pressing A
// asks whether to go through. Each has its own wild Pokémon, a marker stone
// with a little lore, and one thing to find once per save.
//
//   MOSS HOLLOW    behind Petalburg Woods     grass, Lv 7-10 (from the start)
//   TIDE GROTTO    off Route 109's islet      surfing and rods (Surf)
//   EMBER HOLLOW   in Jagged Pass's walls     cave, Lv 20-24
//   SHADE CHAMBER  under Mt. Pyre's slope     cave, Lv 27-31
//   STARFALL CAVE  below Mossdeep's cliffs    cave, Lv 35-40
//
// Progress is this save's own (CoopSaveV5.habitat):
//   found        entered at least once ("FIELD NOTES: You found ...!")
//   cluesHeard   its rumor read in the nearby Pokémon Center's WILDLIFE REPORT
//   interactions its marker stone read
//   claims       its find picked up (an item, once)
//   complete     its three resident Pokémon all caught (Pokédex caught flags,
//                so ones caught elsewhere count too)
//   rewarded     its completion reward handed over, by the FIELD NOTES
//                (src/coop_notes.c: once, in the same moment as the item, or
//                waiting for room in the BAG)
// The FIELD NOTES list them; the Pokédex AREA page shows the habitats like
// any map with wild Pokémon. In co-op each player has their own progress and
// their own copy of the find; the maps are ordinary maps, so a linked
// partner who goes through the same spot is seen there.
#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_habitat.h"
#include "event_data.h"
#include "item.h"
#include "overworld.h"
#include "pokedex.h"
#include "string_util.h"
#include "strings.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/region_map_sections.h"
#include "constants/species.h"

struct Habitat
{
    u16 map;                    // MAP_HABITAT_*
    u16 hostTown;               // MAPSEC whose Center's report has the rumor
    u16 residents[3];           // catching all three completes it
    u16 find;                   // the find (once per save)
    u16 reward;                 // the completion reward
    u8 rewardQuantity;
    const u8 *name;
    const u8 *clue;
    const u8 *lore;
};

static const struct Habitat sHabitats[COOP_HABITAT_COUNT] =
{
    {
        .map = MAP_HABITAT_MOSS_HOLLOW,
        .hostTown = MAPSEC_PETALBURG_CITY,
        .residents = { SPECIES_SEWADDLE, SPECIES_PIKIPEK, SPECIES_NYMBLE },
        .find = ITEM_LEAF_STONE,
        .reward = ITEM_NEST_BALL, .rewardQuantity = 5,
        .name = COMPOUND_STRING("MOSS HOLLOW"),
        .clue = COMPOUND_STRING("A narrow way behind the trees\nin PETALBURG WOODS's west."),
        .lore = COMPOUND_STRING("“Moss Hollow. Small POKéMON grow\nup here, away from the paths.”"),
    },
    {
        .map = MAP_HABITAT_TIDE_GROTTO,
        .hostTown = MAPSEC_SLATEPORT_CITY,
        .residents = { SPECIES_CORSOLA, SPECIES_CLAUNCHER, SPECIES_CRAMORANT },
        .find = ITEM_WATER_STONE,
        .reward = ITEM_DIVE_BALL, .rewardQuantity = 5,
        .name = COMPOUND_STRING("TIDE GROTTO"),
        .clue = COMPOUND_STRING("The tide pulls east from\nROUTE 109's sandy islet."),
        .lore = COMPOUND_STRING("“Tide Grotto. The sea comes in at\nevery tide, and so do visitors.”"),
    },
    {
        .map = MAP_HABITAT_EMBER_HOLLOW,
        .hostTown = MAPSEC_LAVARIDGE_TOWN,
        .residents = { SPECIES_HEATMOR, SPECIES_TURTONATOR, SPECIES_LITLEO },
        .find = ITEM_FIRE_STONE,
        .reward = ITEM_LUXURY_BALL, .rewardQuantity = 3,
        .name = COMPOUND_STRING("EMBER HOLLOW"),
        .clue = COMPOUND_STRING("Warm air seeps from a crack in\nJAGGED PASS's rock walls."),
        .lore = COMPOUND_STRING("“Ember Hollow. The mountain's heat\nkeeps this cave warm all year.”"),
    },
    {
        .map = MAP_HABITAT_SHADE_CHAMBER,
        .hostTown = MAPSEC_LILYCOVE_CITY,
        .residents = { SPECIES_GOLETT, SPECIES_YAMASK, SPECIES_SANDYGAST },
        .find = ITEM_DUSK_STONE,
        .reward = ITEM_REAPER_CLOTH, .rewardQuantity = 1,
        .name = COMPOUND_STRING("SHADE CHAMBER"),
        .clue = COMPOUND_STRING("A cold draft blows from the\nrocks of MT. PYRE's lower slope."),
        .lore = COMPOUND_STRING("“Shade Chamber. Those who rest on\nthe mountain sometimes wander here.”"),
    },
    {
        .map = MAP_HABITAT_STARFALL_CAVE,
        .hostTown = MAPSEC_MOSSDEEP_CITY,
        .residents = { SPECIES_CLEFAIRY, SPECIES_ELGYEM, SPECIES_BRONZOR },
        .find = ITEM_MOON_STONE,
        .reward = ITEM_COMET_SHARD, .rewardQuantity = 1,
        .name = COMPOUND_STRING("STARFALL CAVE"),
        .clue = COMPOUND_STRING("A cliff path leads down from\nMOSSDEEP's eastern rocks."),
        .lore = COMPOUND_STRING("“Starfall Cave. A star fell here\nlong ago. Some say it still sings.”"),
    },
};

static struct CoopHabitatSave *Habitat_Save(void)
{
    struct CoopSave *save = Coop_GetSave();

    if (save == NULL || save->v5.size != sizeof(struct CoopSaveV5) || !Coop_SaveSchemaSupported())
        return NULL;
    return &save->v5.habitat;
}

static u16 Habitat_CurrentMap(void)
{
    return gSaveBlock1Ptr->location.mapNum | (gSaveBlock1Ptr->location.mapGroup << 8);
}

// The habitat the player is in, or -1.
s8 CoopHabitat_Here(void)
{
    u16 map = Habitat_CurrentMap();
    u32 i;

    for (i = 0; i < COOP_HABITAT_COUNT; i++)
    {
        if (sHabitats[i].map == map)
            return i;
    }
    return -1;
}

static u8 Habitat_ResidentsCaught(u32 i)
{
    u8 n = 0;
    u32 r;

    for (r = 0; r < ARRAY_COUNT(sHabitats[i].residents); r++)
        n += GetSetPokedexFlag(SpeciesToNationalPokedexNum(sHabitats[i].residents[r]), FLAG_GET_CAUGHT) ? 1 : 0;
    return n;
}

// ON_TRANSITION of a habitat map. VAR_RESULT: TRUE when this is the first
// visit (the ON_FRAME script then says so). STR_VAR_1: its name.
u16 CoopHabitat_OnEnter(void)
{
    struct CoopHabitatSave *h = Habitat_Save();
    s8 i = CoopHabitat_Here();

    if (i < 0)
        return FALSE;
    StringCopy(gStringVar1, sHabitats[i].name);
    if (h == NULL || (h->found & (1 << i)))
        return FALSE;
    h->found |= 1 << i;
    h->newNotes |= 1 << i;
    return TRUE;
}

// STR_VAR_1: the name of the habitat the player is in.
void CoopHabitat_BufferName(void)
{
    s8 i = CoopHabitat_Here();

    StringCopy(gStringVar1, i >= 0 ? sHabitats[i].name : gText_EmptyString2);
}

// ON_LOAD: hides the find (object flag FLAG_TEMP_1) once it is claimed.
void CoopHabitat_HideClaimed(void)
{
    struct CoopHabitatSave *h = Habitat_Save();
    s8 i = CoopHabitat_Here();

    if (i >= 0 && h != NULL && (h->claims & (1 << i)))
        FlagSet(FLAG_TEMP_1);
}

// The find. VAR_RESULT: 0 picked up (STR_VAR_2: the item), 1 no room
// (nothing changes), 2 already claimed. gSpecialVar_0x8005: the item.
u16 CoopHabitat_ClaimFind(void)
{
    struct CoopHabitatSave *h = Habitat_Save();
    s8 i = CoopHabitat_Here();

    if (i < 0 || h == NULL || (h->claims & (1 << i)))
        return 2;
    gSpecialVar_0x8005 = sHabitats[i].find;
    CopyItemName(sHabitats[i].find, gStringVar2);
    if (!CheckBagHasSpace(sHabitats[i].find, 1))
        return 1;
    AddBagItem(sHabitats[i].find, 1);
    h->claims |= 1 << i;
    FlagSet(FLAG_TEMP_1);
    return 0;
}

// The marker stone: its lore in gStringVar4; noted in the FIELD NOTES.
void CoopHabitat_ReadMarker(void)
{
    struct CoopHabitatSave *h = Habitat_Save();
    s8 i = CoopHabitat_Here();

    if (i < 0)
    {
        gStringVar4[0] = EOS;
        return;
    }
    StringExpandPlaceholders(gStringVar4, sHabitats[i].lore);
    if (h != NULL)
        h->interactions |= 1 << i;
}

// For the FIELD NOTES payout (src/coop_notes.c): a habitat whose three
// residents are caught is complete; its reward is due once. Marks it paid
// only when the caller says so (CoopHabitat_PayDue is asked first, then the
// item goes in or waits, then sHabitatPaying is confirmed).
static EWRAM_DATA s8 sHabitatPaying = 0;   // set by CoopHabitat_PayDue before use

// Habitats whose three residents are caught (and found): bit per habitat.
u8 CoopHabitat_CompleteMask(void)
{
    struct CoopHabitatSave *h = Habitat_Save();
    u8 mask = 0;
    u32 i;

    if (h == NULL)
        return 0;
    for (i = 0; i < COOP_HABITAT_COUNT; i++)
    {
        if ((h->found & (1 << i)) && Habitat_ResidentsCaught(i) == ARRAY_COUNT(sHabitats[i].residents))
            mask |= 1 << i;
    }
    return mask;
}

// Completion rewards due (complete, not yet handed over).
u8 CoopHabitat_CountDue(void)
{
    struct CoopHabitatSave *h = Habitat_Save();
    u8 mask = CoopHabitat_CompleteMask(), n = 0;
    u32 i;

    if (h == NULL)
        return 0;
    for (i = 0; i < COOP_HABITAT_COUNT; i++)
        n += (mask & (1 << i)) && !(h->rewarded & (1 << i));
    return n;
}

// `skip`: habitats not to pay now (complete already when the save was
// loaded: they wait in the FIELD NOTES, src/coop_notes.c).
bool8 CoopHabitat_PayDue(u16 *item, u8 *quantity, u8 *why, u8 skip)
{
    struct CoopHabitatSave *h = Habitat_Save();
    u32 i;

    sHabitatPaying = -1;
    if (h == NULL)
        return FALSE;
    for (i = 0; i < COOP_HABITAT_COUNT; i++)
    {
        if (!(h->found & (1 << i)) || Habitat_ResidentsCaught(i) < ARRAY_COUNT(sHabitats[i].residents))
            continue;
        h->complete |= 1 << i;
        if ((h->rewarded & (1 << i)) || (skip & (1 << i)))
            continue;
        *item = sHabitats[i].reward;
        *quantity = sHabitats[i].rewardQuantity;
        StringCopy(why, sHabitats[i].name);
        StringAppend(why, COMPOUND_STRING(" complete!"));
        sHabitatPaying = i;
        return TRUE;
    }
    return FALSE;
}

void CoopHabitat_ConfirmPaid(void)
{
    struct CoopHabitatSave *h = Habitat_Save();

    if (h != NULL && sHabitatPaying >= 0)
        h->rewarded |= 1 << sHabitatPaying;
    sHabitatPaying = -1;
}

// The WILDLIFE REPORT's last page in that town's Center (src/
// coop_encounters.c): the rumor. Marks it heard.
bool8 CoopHabitat_ClueForTown(u16 mapsec, u8 *dest)
{
    struct CoopHabitatSave *h = Habitat_Save();
    u32 i;

    for (i = 0; i < COOP_HABITAT_COUNT; i++)
    {
        if (sHabitats[i].hostTown != mapsec)
            continue;
        if (h != NULL)
            h->cluesHeard |= 1 << i;
        StringCopy(dest, sHabitats[i].clue);
        return TRUE;
    }
    return FALSE;
}

// FIELD NOTES pages about habitats (page 0: the summary; then one per
// habitat found or heard of). FALSE after the last.
bool8 CoopHabitat_NotesPage(u8 page, u8 *dest)
{
    struct CoopHabitatSave *h = Habitat_Save();
    u8 *p = dest;
    u32 i, found = 0, complete = 0, shown = 0;

    if (h == NULL)
        return FALSE;
    for (i = 0; i < COOP_HABITAT_COUNT; i++)
    {
        found += (h->found >> i) & 1;
        complete += (h->complete >> i) & 1;
    }
    if (page == 0)
    {
        p = StringCopy(p, COMPOUND_STRING("Hidden habitats found: "));
        p = ConvertIntToDecimalStringN(p, found, STR_CONV_MODE_LEFT_ALIGN, 1);
        p = StringCopy(p, COMPOUND_STRING(" of 5.\nComplete: "));
        p = ConvertIntToDecimalStringN(p, complete, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringCopy(p, COMPOUND_STRING(". CENTER REPORTS have rumors."));
        return TRUE;
    }
    for (i = 0; i < COOP_HABITAT_COUNT; i++)
    {
        if (!((h->found | h->cluesHeard) & (1 << i)))
            continue;
        if (++shown != page)
            continue;
        h->newNotes &= ~(1 << i);
        if (h->found & (1 << i))
        {
            p = StringCopy(p, sHabitats[i].name);
            p = StringCopy(p, COMPOUND_STRING(": "));
            p = ConvertIntToDecimalStringN(p, Habitat_ResidentsCaught(i), STR_CONV_MODE_LEFT_ALIGN, 1);
            p = StringCopy(p, COMPOUND_STRING(" of 3 residents\ncaught"));
            p = StringCopy(p, (h->claims & (1 << i)) ? COMPOUND_STRING(", its find found.") : COMPOUND_STRING(". Something lies there."));
        }
        else
        {
            StringCopy(p, sHabitats[i].clue);
        }
        return TRUE;
    }
    return FALSE;
}
