// Town specialities (collection update §12): one service in each of six
// towns, and a small track that pays a reward once.
//
//   DEWFORD     BEACH HUT     once a day, rest on the beach with your
//                             Trainer Bond partner: +120 Bond XP, friendship
//               track: 5 days -> SHELL BELL
//   FALLARBOR   BALL ARTISAN  move one of your own Pokémon (not traded,
//                             not an egg) into another ball from your BAG;
//                             the ball is used up
//               track: 3 times -> 3 LUXURY BALLS
//   VERDANTURF  GROOMER       once a day, groom a Pokémon: friendship +20
//               track: 5 days -> SOOTHE BELL
//   FORTREE     BERRY KEEPER  once a day, a berry (it changes with the day)
//               track: 7 days -> STARF BERRY
//   MOSSDEEP    STAR JUDGE    reads a Pokémon's IVs (read only)
//               track: 5 times -> BOTTLE CAP
//   PACIFIDLOG  TIDE READER   today's tides in Shoal Cave and the weather
//                             of Routes 119 and 123 (read only)
//               track: 3 days -> MYSTIC WATER
//
// Everything is this save's own (CoopSaveV5.town[6][6]; reserved space):
//   [0] uses (days, for the daily ones), saturating at 255
//   [1] the track's reward handed over (bit 0)
//   [2..3] the last day used, by the cartridge clock's day count (daily
//          services: waiting at a Pokémon Center changes the time of day,
//          never the day, so it repeats nothing)
// A track reward goes into the BAG in the same moment its bit is set, or
// waits in CoopSaveV5.pending (src/coop_notes.c) when there is no room.
#include "global.h"
#include "coop.h"
#include "coop_bond.h"
#include "coop_deal.h"
#include "coop_notes.h"
#include "event_data.h"
#include "item.h"
#include "party_menu.h"
#include "pokeball.h"
#include "pokemon.h"
#include "rtc.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/coop.h"
#include "constants/items.h"
#include "constants/party_menu.h"
#include "constants/pokeball.h"

enum
{
    TOWN_DEWFORD,
    TOWN_FALLARBOR,
    TOWN_VERDANTURF,
    TOWN_FORTREE,
    TOWN_MOSSDEEP,
    TOWN_PACIFIDLOG,
    TOWN_COUNT,
};

struct TownTrack
{
    u8 need;
    u16 item;
    u8 quantity;
    const u8 *name;
};

static const struct TownTrack sTracks[TOWN_COUNT] =
{
    [TOWN_DEWFORD]    = { 5, ITEM_SHELL_BELL, 1, COMPOUND_STRING("BEACH HUT") },
    [TOWN_FALLARBOR]  = { 3, ITEM_LUXURY_BALL, 3, COMPOUND_STRING("BALL ARTISAN") },
    [TOWN_VERDANTURF] = { 5, ITEM_SOOTHE_BELL, 1, COMPOUND_STRING("GROOMER") },
    [TOWN_FORTREE]    = { 7, ITEM_STARF_BERRY, 1, COMPOUND_STRING("BERRY KEEPER") },
    [TOWN_MOSSDEEP]   = { 5, ITEM_BOTTLE_CAP, 1, COMPOUND_STRING("STAR JUDGE") },
    [TOWN_PACIFIDLOG] = { 3, ITEM_MYSTIC_WATER, 1, COMPOUND_STRING("TIDE READER") },
};

#define BOND_REST_XP 120

static u8 *Town_Data(u8 town)
{
    struct CoopSave *save = Coop_GetSave();

    if (town >= TOWN_COUNT || save == NULL || save->v5.size != sizeof(struct CoopSaveV5) || !Coop_SaveSchemaSupported())
        return NULL;
    return save->v5.town[town];
}

static u16 Town_Today(void)
{
    RtcCalcLocalTime();
    return (u16)gLocalTime.days;
}

// Special. VAR_0x8004: the town. VAR_RESULT: TRUE when its daily service
// has not been used today.
u16 CoopTown_FreeToday(void)
{
    u8 *t = Town_Data(gSpecialVar_0x8004);
    u16 today = Town_Today();

    if (t == NULL)
        return FALSE;
    if (t[0] == 0)
        return TRUE;
    return (t[2] | (t[3] << 8)) != today;
}

enum
{
    TRACK_NONE,
    TRACK_GIVEN,    // STR_VAR_2: the reward, in the BAG
    TRACK_QUEUED,   // no room: it waits
};

// Special, after a service was used. VAR_0x8004: the town; VAR_0x8005: TRUE
// for a daily service (records today). Counts the use and pays the track's
// reward once when it is reached. VAR_RESULT: TRACK_*; STR_VAR_2: the
// reward.
u16 CoopTown_CountUse(void)
{
    u8 town = gSpecialVar_0x8004;
    u8 *t = Town_Data(town);
    u16 today = Town_Today();
    const struct TownTrack *track;
    u8 *p;

    if (t == NULL)
        return TRACK_NONE;
    if (gSpecialVar_0x8005)
    {
        if (t[0] != 0 && (t[2] | (t[3] << 8)) == today)
            return TRACK_NONE;   // already counted today
        t[2] = today & 0xFF;
        t[3] = today >> 8;
    }
    if (t[0] < 255)
        t[0]++;
    track = &sTracks[town];
    if ((t[1] & 1) || t[0] < track->need)
        return TRACK_NONE;
    p = ConvertIntToDecimalStringN(gStringVar2, track->quantity, STR_CONV_MODE_LEFT_ALIGN, 2);
    *p++ = CHAR_SPACE;
    CopyItemNameHandlePlural(track->item, p, track->quantity);
    if (CheckBagHasSpace(track->item, track->quantity))
    {
        AddBagItem(track->item, track->quantity);
        t[1] |= 1;
        return TRACK_GIVEN;
    }
    if (CoopNotes_QueueReward(track->item, track->quantity, COOP_REWARD_SRC_TOWN))
    {
        t[1] |= 1;
        return TRACK_QUEUED;
    }
    return TRACK_NONE;    // the queue is full too: paid on a later use
}

// Special. VAR_0x8004: the town. STR_VAR_1: uses so far; STR_VAR_3: how
// many the track needs. VAR_RESULT: TRUE when its reward is already given.
u16 CoopTown_BufferTrack(void)
{
    u8 *t = Town_Data(gSpecialVar_0x8004);

    if (t == NULL)
        return TRUE;
    ConvertIntToDecimalStringN(gStringVar1, t[0], STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar3, sTracks[gSpecialVar_0x8004].need, STR_CONV_MODE_LEFT_ALIGN, 2);
    return (t[1] & 1) != 0;
}

static struct Pokemon *Town_ChosenMon(void)
{
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        return NULL;
    if (GetMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], MON_DATA_SPECIES) == SPECIES_NONE
     || GetMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], MON_DATA_IS_EGG))
        return NULL;
    return &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
}

// ---------------------------------------------------------------- DEWFORD

// Special. VAR_RESULT: FALSE without a Trainer Bond partner in the party
// (STR_VAR_1 empty); TRUE after resting with it (STR_VAR_1: its name).
u16 CoopTown_BeachRest(void)
{
    s8 slot = CoopBond_ActiveSlot();
    struct Pokemon *mon;
    u16 friendship;

    if (slot < 0)
        return FALSE;
    mon = &gParties[B_TRAINER_PLAYER][slot];
    GetMonNickname(mon, gStringVar1);
    friendship = GetMonData(mon, MON_DATA_FRIENDSHIP) + 5;
    if (friendship > MAX_FRIENDSHIP)
        friendship = MAX_FRIENDSHIP;
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
    CoopBond_Award(COOP_BOND_SRC_REST, BOND_REST_XP);
    return TRUE;
}

// ---------------------------------------------------------------- FALLARBOR

// Special. VAR_0x8004: party slot. VAR_RESULT: 0 can change, 1 an egg or
// nothing, 2 caught or hatched by another trainer. STR_VAR_1: its name.
u16 CoopTown_CheckReBall(void)
{
    struct Pokemon *mon = Town_ChosenMon();

    if (mon == NULL)
        return 1;
    GetMonNickname(mon, gStringVar1);
    if (GetMonData(mon, MON_DATA_OT_ID) != READ_OTID_FROM_SAVE)
        return 2;
    return 0;
}

// Special. VAR_0x8007: the nth ball in the BAG's POKé BALLS pocket (0
// first). VAR_RESULT: its item (STR_VAR_1: "NAME x3"), or 0 after the last.
u16 CoopTown_BufferBall(void)
{
    struct BagPocket *pocket = &gBagPockets[POCKET_POKE_BALLS];
    u32 i, n = 0;

    for (i = 0; i < pocket->capacity; i++)
    {
        struct ItemSlot slot = BagPocket_GetSlotData(pocket, i);
        u8 *p;

        if (slot.itemId == ITEM_NONE || slot.quantity == 0 || ItemIdToBallId(slot.itemId) == BALL_STRANGE)
            continue;
        if (n++ != gSpecialVar_0x8007)
            continue;
        p = StringCopy(gStringVar1, GetItemName(slot.itemId));
        p = StringCopy(p, COMPOUND_STRING(" ×"));
        ConvertIntToDecimalStringN(p, slot.quantity, STR_CONV_MODE_LEFT_ALIGN, 3);
        return slot.itemId;
    }
    return ITEM_NONE;
}

// Special. VAR_0x8004: party slot; VAR_0x8006: the ball item. Moves the
// Pokémon into it and uses one up. VAR_RESULT: TRUE when done (FALSE: the
// ball is gone, or it is already in that ball). STR_VAR_2: the ball.
u16 CoopTown_ReBall(void)
{
    struct Pokemon *mon = Town_ChosenMon();
    u16 item = gSpecialVar_0x8006;
    u32 ball;

    if (mon == NULL || item == ITEM_NONE || !CheckBagHasItem(item, 1))
        return FALSE;
    ball = ItemIdToBallId(item);
    if (ball == BALL_STRANGE || GetMonData(mon, MON_DATA_OT_ID) != READ_OTID_FROM_SAVE)
        return FALSE;
    CopyItemName(item, gStringVar2);
    GetMonNickname(mon, gStringVar1);   // the BALL list used STR_VAR_1
    if (GetMonData(mon, MON_DATA_POKEBALL) == ball)
        return FALSE;
    SetMonData(mon, MON_DATA_POKEBALL, &ball);
    RemoveBagItem(item, 1);
    return TRUE;
}

// ---------------------------------------------------------------- VERDANTURF

// Special. VAR_0x8004: party slot. VAR_RESULT: 0 groomed (friendship
// +20), 1 nothing to groom, 2 already as happy as can be (still counts as
// today's grooming). STR_VAR_1: its name.
u16 CoopTown_Groom(void)
{
    struct Pokemon *mon = Town_ChosenMon();
    u16 friendship;

    if (mon == NULL)
        return 1;
    GetMonNickname(mon, gStringVar1);
    friendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
    if (friendship >= MAX_FRIENDSHIP)
        return 2;
    friendship = min(friendship + 20, MAX_FRIENDSHIP);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
    return 0;
}

// ---------------------------------------------------------------- FORTREE

static const u16 sDailyBerries[] =
{
    ITEM_ORAN_BERRY, ITEM_SITRUS_BERRY, ITEM_LUM_BERRY, ITEM_LEPPA_BERRY,
    ITEM_PECHA_BERRY, ITEM_CHESTO_BERRY, ITEM_RAWST_BERRY,
};

// Special. VAR_RESULT: today's berry (the script gives it with giveitem,
// then counts the day only when it was received).
u16 CoopTown_TodaysBerry(void)
{
    return sDailyBerries[Town_Today() % ARRAY_COUNT(sDailyBerries)];
}

// ---------------------------------------------------------------- MOSSDEEP

static const u8 *const sStatNames[NUM_STATS] =
{
    [STAT_HP] = COMPOUND_STRING("HP"),
    [STAT_ATK] = COMPOUND_STRING("ATTACK"),
    [STAT_DEF] = COMPOUND_STRING("DEFENSE"),
    [STAT_SPEED] = COMPOUND_STRING("SPEED"),
    [STAT_SPATK] = COMPOUND_STRING("SP. ATK"),
    [STAT_SPDEF] = COMPOUND_STRING("SP. DEF"),
};

// Special. VAR_0x8004: party slot. VAR_RESULT: FALSE for an egg or nothing.
// gStringVar4: two pages, "{name}'s potential is outstanding.\nIts best
// is SPEED." and the six values.
u16 CoopTown_JudgeIVs(void)
{
    struct Pokemon *mon = Town_ChosenMon();
    u8 name[POKEMON_NAME_LENGTH + 1];
    u8 iv[NUM_STATS];
    u32 i, total = 0, best = 0;
    u8 *p = gStringVar4;
    const u8 *verdict;

    if (mon == NULL)
        return FALSE;
    for (i = 0; i < NUM_STATS; i++)
    {
        iv[i] = GetMonData(mon, MON_DATA_HP_IV + i);
        total += iv[i];
        if (iv[i] > iv[best])
            best = i;
    }
    if (total >= 151)
        verdict = COMPOUND_STRING("outstanding");
    else if (total >= 121)
        verdict = COMPOUND_STRING("relatively superior");
    else if (total >= 91)
        verdict = COMPOUND_STRING("above average");
    else
        verdict = COMPOUND_STRING("decent");
    GetMonNickname(mon, name);
    p = StringCopy(p, name);
    p = StringCopy(p, COMPOUND_STRING("'s potential is\n"));
    p = StringCopy(p, verdict);
    p = StringCopy(p, COMPOUND_STRING(". Its best: "));
    p = StringCopy(p, sStatNames[best]);
    p = StringCopy(p, COMPOUND_STRING(".\p"));
    for (i = 0; i < NUM_STATS; i++)
    {
        p = StringCopy(p, sStatNames[i]);
        *p++ = CHAR_SPACE;
        p = ConvertIntToDecimalStringN(p, iv[i], STR_CONV_MODE_LEFT_ALIGN, 2);
        if (i == 2)
            *p++ = CHAR_NEWLINE;
        else if (i != NUM_STATS - 1)
            p = StringCopy(p, COMPOUND_STRING(", "));
    }
    *p++ = CHAR_PERIOD;
    *p = EOS;
    return TRUE;
}

// ---------------------------------------------------------------- PACIFIDLOG

// Shoal Cave's tide by hour (UpdateShoalTideFlag, src/time_events.c): 1 high.
static const u8 sShoalTide[24] =
{
    1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1,
};

// The Route 119 and 123 weather cycles (src/field_weather_effect.c): rain?
static const bool8 sRoute119Rain[4] = { FALSE, TRUE, TRUE, TRUE };
static const bool8 sRoute123Rain[4] = { FALSE, FALSE, TRUE, FALSE };

// Special. gStringVar4: today's tides and weather, two pages.
void CoopTown_ReadTides(void)
{
    u8 *p = gStringVar4;
    u32 hour, next;
    u8 stage = gSaveBlock1Ptr->weatherCycleStage % 4;
    bool8 high;

    RtcCalcLocalTime();
    hour = gLocalTime.hours % 24;   // the real clock: tides do not follow a rest
    high = sShoalTide[hour];
    for (next = hour + 1; next < hour + 24; next++)
    {
        if (sShoalTide[next % 24] != high)
            break;
    }
    p = StringCopy(p, COMPOUND_STRING("SHOAL CAVE's tide is "));
    p = StringCopy(p, high ? COMPOUND_STRING("HIGH") : COMPOUND_STRING("LOW"));
    p = StringCopy(p, COMPOUND_STRING(" now,\nuntil "));
    p = ConvertIntToDecimalStringN(p, next % 24, STR_CONV_MODE_LEADING_ZEROS, 2);
    p = StringCopy(p, COMPOUND_STRING(":00.\p"));
    p = StringCopy(p, COMPOUND_STRING("Today ROUTE 119 is "));
    p = StringCopy(p, sRoute119Rain[stage] ? COMPOUND_STRING("rainy") : COMPOUND_STRING("clear"));
    p = StringCopy(p, COMPOUND_STRING(",\nand ROUTE 123 is "));
    p = StringCopy(p, sRoute123Rain[stage] ? COMPOUND_STRING("rainy.") : COMPOUND_STRING("clear."));
}
