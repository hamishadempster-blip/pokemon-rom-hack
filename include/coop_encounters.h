#ifndef GUARD_COOP_ENCOUNTERS_H
#define GUARD_COOP_ENCOUNTERS_H

// Wild visitors by time of day and weather (src/coop_encounters.c; the data
// is src/data/coop_visitors.json).
enum
{
    COOP_VISIT_LAND,
    COOP_VISIT_SURF,
    COOP_VISIT_FISH,
    COOP_VISIT_ROCK,
};

// The first four are TIME_* (include/constants/rtc.h).
enum
{
    COOP_WHEN_MORNING,
    COOP_WHEN_DAY,
    COOP_WHEN_EVENING,
    COOP_WHEN_NIGHT,
    COOP_WHEN_RAIN,
    COOP_WHEN_SAND,
    COOP_WHEN_ASH,
};

enum
{
    COOP_ROD_ANY,
    COOP_ROD_OLD,   // OLD_ROD + 1, and so on
    COOP_ROD_GOOD,
    COOP_ROD_SUPER,
};

struct CoopVisitor
{
    u16 map;            // MAP_* (num | group << 8)
    u16 species;
    u8 area;            // COOP_VISIT_*
    u8 when;            // COOP_WHEN_*
    u8 rod;             // COOP_ROD_* (fishing only)
    const u8 *hint;     // what the report says before the species is seen
};

struct CoopVisitorReport
{
    u16 mapsec;
    u16 maps[4];        // MAP_UNDEFINED after the last
};

u16 CoopVisitor_Species(u8 area, u8 rod, u16 species);
bool8 CoopVisitor_MapHasSpecies(u8 mapGroup, u8 mapNum, u16 species);
u16 CoopVisitor_CountCaught(u16 *total);

#endif // GUARD_COOP_ENCOUNTERS_H
