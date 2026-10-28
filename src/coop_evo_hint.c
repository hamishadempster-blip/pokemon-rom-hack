// The summary screen's evolution hint (INFO page, SELECT): each way the
// Pokemon can evolve, from the species' real evolution data as this build
// has it (its own adaptations included: items in place of trades, the
// Linking Cord, the places that stand in for Alola, Galar and Hisui), what
// it needs, whether this Pokemon meets that now, and where a needed item
// can be had. Nothing is guessed: a condition the hint can't put in words
// is shown as "a special condition", and a Pokemon that can't evolve says
// so. For a Pokemon of the partner's (VIEW TEAM) only the needs are shown.
#include "global.h"
#include "coop.h"
#include "battle_main.h"
#include "event_data.h"
#include "item.h"
#include "move.h"
#include "overworld.h"
#include "pokemon.h"
#include "region_map.h"
#include "rtc.h"
#include "string_util.h"
#include "text.h"
#include "constants/characters.h"
#include "strings.h"
#include "constants/items.h"
#include "constants/regions.h"
#include "constants/region_map_sections.h"
#include "constants/weather.h"
#include "data/coop_evo_item_hints.h"


static const u8 *ItemWhere(u16 item)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sEvoItemHints); i++)
    {
        if (sEvoItemHints[i].item == item)
            return sEvoItemHints[i].where;
    }
    return NULL;
}

static u8 *AddText(u8 *dst, const u8 *src)
{
    return StringCopy(dst, src);
}

static u8 *AddItem(u8 *dst, u16 item)
{
    CopyItemName(item, dst);
    return dst + StringLength(dst);
}

static u8 *AddNum(u8 *dst, u32 n)
{
    return ConvertIntToDecimalStringN(dst, n, STR_CONV_MODE_LEFT_ALIGN, 5);
}

static const u8 *TimeName(u16 time)
{
    switch (time)
    {
    case TIME_MORNING: return COMPOUND_STRING("in the morning");
    case TIME_DAY:     return COMPOUND_STRING("by day");
    case TIME_EVENING: return COMPOUND_STRING("in the evening");
    default:           return COMPOUND_STRING("at night");
    }
}

// This build's stand-in places for other regions (src/pokemon.c,
// GetEvolutionRegion).
static const u8 *RegionPlace(u16 region)
{
    switch (region)
    {
    case REGION_ALOLA: return COMPOUND_STRING("near DEWFORD (Alola)");
    case REGION_GALAR: return COMPOUND_STRING("on ROUTE 114 (Galar)");
    case REGION_HISUI: return COMPOUND_STRING("on MT. PYRE (Hisui)");
    }
    return NULL;
}

// One condition in words, appended; FALSE for one it can't word.
static bool32 AddCondition(u8 **pp, const struct EvolutionParam *c)
{
    u8 *p = *pp;

    switch (c->condition)
    {
    case IF_GENDER:
        p = AddText(p, c->arg1 == MON_MALE ? COMPOUND_STRING("male") : COMPOUND_STRING("female"));
        break;
    case IF_TIME:
        p = AddText(p, TimeName(c->arg1));
        break;
    case IF_NOT_TIME:
        p = AddText(p, COMPOUND_STRING("not "));
        p = AddText(p, TimeName(c->arg1));
        break;
    case IF_MIN_FRIENDSHIP:
        p = AddText(p, COMPOUND_STRING("high friendship"));
        break;
    case IF_ATK_GT_DEF:
        p = AddText(p, COMPOUND_STRING("ATTACK > DEFENSE"));
        break;
    case IF_ATK_EQ_DEF:
        p = AddText(p, COMPOUND_STRING("ATTACK = DEFENSE"));
        break;
    case IF_ATK_LT_DEF:
        p = AddText(p, COMPOUND_STRING("ATTACK < DEFENSE"));
        break;
    case IF_HOLD_ITEM:
        p = AddText(p, COMPOUND_STRING("holding "));
        p = AddItem(p, c->arg1);
        break;
    case IF_SPECIES_IN_PARTY:
        p = AddText(p, GetSpeciesName(c->arg1));
        p = AddText(p, COMPOUND_STRING(" in party"));
        break;
    case IF_IN_MAPSEC:
        p = AddText(p, COMPOUND_STRING("at "));
        p = GetMapName(p, c->arg1, 0);
        break;
    case IF_IN_MAP:
    {
        const struct MapHeader *header = Overworld_GetMapHeaderByGroupAndId(c->arg1 >> 8, c->arg1 & 0xFF);
        if (header == NULL)
            return FALSE;
        p = AddText(p, COMPOUND_STRING("at "));
        p = GetMapName(p, header->regionMapSectionId, 0);
        break;
    }
    case IF_KNOWS_MOVE:
        p = AddText(p, COMPOUND_STRING("knowing "));
        p = AddText(p, GetMoveName(c->arg1));
        break;
    case IF_KNOWS_MOVE_TYPE:
        p = AddText(p, COMPOUND_STRING("knowing a "));
        p = AddText(p, gTypesInfo[c->arg1].name);
        p = AddText(p, COMPOUND_STRING(" move"));
        break;
    case IF_TYPE_IN_PARTY:
        p = AddText(p, COMPOUND_STRING("a "));
        p = AddText(p, gTypesInfo[c->arg1].name);
        p = AddText(p, COMPOUND_STRING(" type in party"));
        break;
    case IF_WEATHER:
        if (c->arg1 == WEATHER_RAIN)
            p = AddText(p, COMPOUND_STRING("in rain"));
        else if (c->arg1 == WEATHER_FOG)
            p = AddText(p, COMPOUND_STRING("in rain or fog"));
        else
            p = AddText(p, COMPOUND_STRING("in special weather"));
        break;
    case IF_NATURE:
        p = AddText(p, gNaturesInfo[c->arg1].name);
        p = AddText(p, COMPOUND_STRING(" nature"));
        break;
    case IF_AMPED_NATURE:
        p = AddText(p, COMPOUND_STRING("by its nature (Amped)"));
        break;
    case IF_LOW_KEY_NATURE:
        p = AddText(p, COMPOUND_STRING("by its nature (Low Key)"));
        break;
    case IF_RECOIL_DAMAGE_GE:
        p = AddText(p, COMPOUND_STRING("after "));
        p = AddNum(p, c->arg1);
        p = AddText(p, COMPOUND_STRING(" recoil HP"));
        break;
    case IF_CURRENT_DAMAGE_GE:
        p = AddText(p, COMPOUND_STRING("down "));
        p = AddNum(p, c->arg1);
        p = AddText(p, COMPOUND_STRING("+ HP"));
        break;
    case IF_CRITICAL_HITS_GE:
        p = AddNum(p, c->arg1);
        p = AddText(p, COMPOUND_STRING(" critical hits, one solo battle"));
        break;
    case IF_USED_MOVE_X_TIMES:
        p = AddText(p, COMPOUND_STRING("using "));
        p = AddText(p, GetMoveName(c->arg1));
        p = AddText(p, COMPOUND_STRING(" "));
        p = AddNum(p, c->arg2);
        p = AddText(p, COMPOUND_STRING("x (solo)"));
        break;
    case IF_MIN_OVERWORLD_STEPS:
        p = AddNum(p, c->arg1);
        p = AddText(p, COMPOUND_STRING(" steps leading"));
        break;
    case IF_BAG_ITEM_COUNT:
        p = AddNum(p, c->arg2);
        p = AddText(p, COMPOUND_STRING(" "));
        p = AddItem(p, c->arg1);
        p = AddText(p, COMPOUND_STRING(" in bag"));
        break;
    case IF_REGION:
        if (RegionPlace(c->arg1) == NULL)
            return FALSE;
        p = AddText(p, RegionPlace(c->arg1));
        break;
    case IF_NOT_REGION:
        if (RegionPlace(c->arg1) == NULL)
            return FALSE;
        p = AddText(p, COMPOUND_STRING("not "));
        p = AddText(p, RegionPlace(c->arg1));
        break;
    case IF_PID_UPPER_MODULO_10_GT:
    case IF_PID_UPPER_MODULO_10_EQ:
    case IF_PID_UPPER_MODULO_10_LT:
    case IF_PID_MODULO_100_GT:
    case IF_PID_MODULO_100_EQ:
    case IF_PID_MODULO_100_LT:
        p = AddText(p, COMPOUND_STRING("by its personality"));
        break;
    case IF_MIN_BEAUTY:
        p = AddText(p, COMPOUND_STRING("high BEAUTY"));
        break;
    default:
        return FALSE;
    }
    *pp = p;
    return TRUE;
}

// Hidden conditions (personality) stay hidden: "met now" for them is
// shown only as a whole, never per condition.
static bool32 CondMet(struct Pokemon *mon, const struct EvolutionParam *c)
{
    struct EvolutionParam one[2];

    one[0] = *c;
    one[1].condition = CONDITIONS_END;
    return DoesMonMeetAdditionalConditions(mon, one, NULL, PARTY_SIZE, NULL, CHECK_EVO);
}

// Number of evolution entries worth showing (targets that exist in this
// build; EVO_NONE and split clones left out).
static u32 CountEntries(const struct Evolution *evos)
{
    u32 i, n = 0;

    for (i = 0; evos != NULL && evos[i].method != EVOLUTIONS_END; i++)
    {
        if (evos[i].method == EVO_NONE || evos[i].method == EVO_SPLIT_FROM_EVO)
            continue;
        if (SanitizeSpeciesId(evos[i].targetSpecies) == SPECIES_NONE)
            continue;
        n++;
    }
    return n;
}

static const struct Evolution *EntryAt(const struct Evolution *evos, u32 page)
{
    u32 i, n = 0;

    for (i = 0; evos[i].method != EVOLUTIONS_END; i++)
    {
        if (evos[i].method == EVO_NONE || evos[i].method == EVO_SPLIT_FROM_EVO)
            continue;
        if (SanitizeSpeciesId(evos[i].targetSpecies) == SPECIES_NONE)
            continue;
        if (n++ == page)
            return &evos[i];
    }
    return NULL;
}

// The number of pages (evolutions) this Pokemon's hint has (0: none).
u32 CoopEvo_PageCount(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return 0;
    return CountEntries(GetSpeciesEvolutions(GetMonData(mon, MON_DATA_SPECIES)));
}

#define HINT_WIDTH 142    // the memo window, small narrow font

EWRAM_DATA u8 gCoopEvoHintText[260] = {0};   // the last hint built (tests read it)

// Cuts `line` (EOS-terminated) to the window width, with "…" when cut.
static void FitLine(u8 *line)
{
    u32 len = StringLength(line);

    if (GetStringWidth(FONT_SMALL_NARROW, line, 0) <= HINT_WIDTH)
        return;
    while (len > 1)
    {
        line[--len] = EOS;
        line[len - 1] = CHAR_ELLIPSIS;
        if (GetStringWidth(FONT_SMALL_NARROW, line, 0) <= HINT_WIDTH)
            return;
        line[len - 1] = EOS;
    }
}

// "None in BAG. " and where to get it; just where, if both don't fit.
static void AddNoneInBag(u8 *line, const u8 *where)
{
    u8 *p = AddText(line, COMPOUND_STRING("None in BAG. "));

    if (where == NULL)
        return;
    AddText(p, where);
    if (GetStringWidth(FONT_SMALL_NARROW, line, 0) > HINT_WIDTH)
        AddText(line, where);
}

// Four short lines for page `page` of `mon`'s hint, in `dest`: where to,
// the method, its conditions, and where this Pokemon stands. `own`: this
// player's Pokemon (whether it meets the needs is shown; else only needs).
void CoopEvo_BufferHint(struct Pokemon *mon, u32 page, u8 *dest, bool32 own)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    const struct Evolution *evos = GetSpeciesEvolutions(species);
    u32 count = CountEntries(evos), level = GetMonData(mon, MON_DATA_LEVEL);
    const struct Evolution *e;
    const struct EvolutionParam *c;
    u8 lines[4][64];
    u8 *p;
    bool32 mainMet = FALSE, condsMet, unworded = FALSE, any = FALSE;
    const struct EvolutionParam *firstUnmet = NULL;
    u16 needItem = ITEM_NONE;
    u32 i;

    if (GetMonData(mon, MON_DATA_IS_EGG) || count == 0)
    {
        StringCopy(dest, COMPOUND_STRING("EVOLUTION\nThis POKéMON does not\nevolve any further."));
        StringCopy(gCoopEvoHintText, dest);
        return;
    }
    if (page >= count)
        page = 0;
    e = EntryAt(evos, page);
    memset(lines, EOS, sizeof(lines));

    // 1: where to, and which page.
    p = AddText(lines[0], COMPOUND_STRING("EVOLVES INTO "));
    p = AddText(p, GetSpeciesName(e->targetSpecies));
    if (count > 1)
    {
        p = AddText(p, COMPOUND_STRING(" ("));
        p = AddNum(p, page + 1);
        p = AddText(p, COMPOUND_STRING("/"));
        p = AddNum(p, count);
        p = AddText(p, COMPOUND_STRING(")"));
    }

    // 2: the method.
    p = lines[1];
    switch (e->method)
    {
    case EVO_LEVEL:
    case EVO_LEVEL_BATTLE_ONLY:
        if (e->param > 1)
        {
            p = AddText(p, COMPOUND_STRING("At Lv. "));
            p = AddNum(p, e->param);
        }
        else
        {
            p = AddText(p, COMPOUND_STRING("At a level-up"));
        }
        if (e->method == EVO_LEVEL_BATTLE_ONLY)
            p = AddText(p, COMPOUND_STRING(" in battle"));
        mainMet = level >= e->param;
        break;
    case EVO_ITEM:
        p = AddText(p, COMPOUND_STRING("Use a "));
        p = AddItem(p, e->param);
        needItem = e->param;
        mainMet = CheckBagHasItem(e->param, 1);
        break;
    case EVO_TRADE:
        p = AddText(p, COMPOUND_STRING("Trade it (partner menu: TRADE)"));
        break;
    case EVO_BATTLE_END:
        p = AddText(p, COMPOUND_STRING("At the end of a battle"));
        mainMet = TRUE;
        break;
    case EVO_SPIN:
        p = AddText(p, COMPOUND_STRING("Spin on the spot (field)"));
        mainMet = TRUE;
        break;
    default:
        p = AddText(p, COMPOUND_STRING("At a special place"));
        break;
    }

    // 3: the conditions.
    p = lines[2];
    for (c = e->params; c != NULL && c->condition != CONDITIONS_END; c++)
    {
        u8 *before = p;

        if (any)
            p = AddText(p, COMPOUND_STRING(", "));
        if (!AddCondition(&p, c))
        {
            p = before;
            *p = EOS;
            unworded = TRUE;
            continue;
        }
        any = TRUE;
        if ((c->condition == IF_HOLD_ITEM || c->condition == IF_BAG_ITEM_COUNT) && needItem == ITEM_NONE)
            needItem = c->arg1;
        if (own && firstUnmet == NULL && !CondMet(mon, c))
            firstUnmet = c;
        if (p - lines[2] > 48)
            break;
    }
    if (unworded)
        p = AddText(p, any ? COMPOUND_STRING(", a special condition") : COMPOUND_STRING("A special condition"));
    if (!any && !unworded)
        p = AddText(p, COMPOUND_STRING("No other conditions."));
    // Too long for one line: the first condition moves up beside the
    // method when it fits there ("At a level-up, not at night" /
    // "holding Oval Stone").
    if (GetStringWidth(FONT_SMALL_NARROW, lines[2], 0) > HINT_WIDTH)
    {
        u8 *comma = lines[2];

        while (*comma != EOS && !(comma[0] == CHAR_COMMA && comma[1] == CHAR_SPACE))
            comma++;
        if (*comma != EOS)
        {
            u8 joined[64];
            u32 head = comma - lines[2], len = StringLength(lines[1]);

            if (len + 2 + head < sizeof(joined))
            {
                memcpy(joined, lines[1], len);
                joined[len] = CHAR_COMMA;
                joined[len + 1] = CHAR_SPACE;
                memcpy(joined + len + 2, lines[2], head);
                joined[len + 2 + head] = EOS;
                if (GetStringWidth(FONT_SMALL_NARROW, joined, 0) <= HINT_WIDTH)
                {
                    StringCopy(lines[1], joined);
                    memmove(lines[2], comma + 2, StringLength(comma + 2) + 1);
                }
            }
        }
    }

    // 4: where this one stands now.
    p = lines[3];
    if (!own)
    {
        p = AddText(p, COMPOUND_STRING("(Your partner's POKéMON.)"));
    }
    else
    {
        condsMet = DoesMonMeetAdditionalConditions(mon, e->params, NULL, PARTY_SIZE, NULL, CHECK_EVO);
        if ((e->method == EVO_LEVEL || e->method == EVO_LEVEL_BATTLE_ONLY) && !mainMet)
        {
            p = AddText(p, COMPOUND_STRING("Now Lv. "));
            p = AddNum(p, level);
            p = AddText(p, COMPOUND_STRING(": "));
            p = AddNum(p, e->param - level);
            p = AddText(p, e->param - level == 1 ? COMPOUND_STRING(" level to go.") : COMPOUND_STRING(" levels to go."));
        }
        else if (e->method == EVO_ITEM && !mainMet)
        {
            AddNoneInBag(p, ItemWhere(e->param));
        }
        else if (needItem != ITEM_NONE && !CheckBagHasItem(needItem, 1)
              && GetMonData(mon, MON_DATA_HELD_ITEM) != needItem && ItemWhere(needItem) != NULL)
        {
            AddNoneInBag(p, ItemWhere(needItem));
        }
        else if (e->method == EVO_TRADE)
        {
            p = AddText(p, COMPOUND_STRING("Needs your partner's game."));
        }
        else if (firstUnmet != NULL && (firstUnmet->condition == IF_TIME || firstUnmet->condition == IF_NOT_TIME))
        {
            // The game's time of day (the shared one while linked, as the
            // evolution itself uses).
            static const u8 *const sNow[TIMES_OF_DAY_COUNT] = {
                [TIME_MORNING] = COMPOUND_STRING("Not now: it's morning."),
                [TIME_DAY]     = COMPOUND_STRING("Not now: it's daytime."),
                [TIME_EVENING] = COMPOUND_STRING("Not now: it's evening."),
                [TIME_NIGHT]   = COMPOUND_STRING("Not now: it's night."),
            };
            enum TimeOfDay now = GetTimeOfDay();
            p = AddText(p, now < TIMES_OF_DAY_COUNT ? sNow[now] : COMPOUND_STRING("Not now."));
        }
        else if (firstUnmet != NULL)
        {
            p = AddText(p, COMPOUND_STRING("Not yet: "));
            AddCondition(&p, firstUnmet);
        }
        else if (condsMet)
        {
            if (e->method == EVO_ITEM)
                p = AddText(p, COMPOUND_STRING("Ready: use it from the BAG."));
            else if (e->method == EVO_LEVEL || e->method == EVO_LEVEL_BATTLE_ONLY)
                p = AddText(p, COMPOUND_STRING("Ready at its next level-up."));
            else
                p = AddText(p, COMPOUND_STRING("Its needs are met now."));
        }
        else
        {
            p = AddText(p, COMPOUND_STRING("Not yet."));
        }
    }

    p = dest;
    for (i = 0; i < 4; i++)
    {
        lines[i][63] = EOS;
        FitLine(lines[i]);
        p = StringCopy(p, lines[i]);
        if (i != 3)
            *p++ = CHAR_NEWLINE;
    }
    *p = EOS;
    for (i = 0; i < sizeof(gCoopEvoHintText) - 1 && dest[i] != EOS; i++)
        gCoopEvoHintText[i] = dest[i];
    gCoopEvoHintText[i] = EOS;
}
