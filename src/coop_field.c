// Battle conditions panel (collection update, section 10). SELECT at the
// action menu shows, read-only, what is on the field: weather, terrain,
// rooms and Gravity; each side's screens, Safeguard, Mist, Tailwind and
// hazards; each Pokemon's stat stages. B, SELECT or A closes it.
//
// Where the data comes from: the engine fills a snapshot when it asks a
// battler for its action (BtlController_EmitChooseAction appends it to that
// command). The command reaches the asked player's controller on every game
// of a link battle, so the co-op guest, which does not run the engine, sees
// the master's real state as of this turn's choice. Nothing is sent back;
// opening it chooses nothing, consumes no RNG and does not stop link
// processing.
//
// Truthfulness: durations are shown only where they are known without
// hidden information. Rooms, Gravity, Tailwind, Safeguard and Mist have
// fixed lengths. A foe's REFLECT/LIGHT SCREEN/AURORA VEIL may be lengthened
// by its unrevealed LIGHT CLAY, and weather and terrain by an item whose
// owner is not tracked, so those say "?". Weather set with no end (an
// ability under older rules, the primal weathers) says "lasting".
#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_util.h"
#include "coop_field.h"
#include "menu.h"
#include "string_util.h"
#include "text.h"
#include "window.h"
#include "constants/battle.h"
#include "constants/characters.h"
#include "constants/songs.h"
#include "sound.h"

EWRAM_DATA struct CoopFieldSnap gCoopFieldSnap = {0};

void CoopField_Fill(struct CoopFieldSnap *snap)
{
    u32 side, b, s;

    memset(snap, 0, sizeof(*snap));
    snap->valid = TRUE;
    snap->weather = gBattleWeather;
    snap->weatherTurns = gBattleStruct->weatherDuration;
    snap->terrain = gFieldTimers.terrain;
    snap->terrainTurns = gFieldTimers.terrainTimer;
    snap->trickRoom = (gFieldStatuses & STATUS_FIELD_TRICK_ROOM) ? gFieldTimers.trickRoomTimer : 0;
    snap->gravity = (gFieldStatuses & STATUS_FIELD_GRAVITY) ? gFieldTimers.gravityTimer : 0;
    snap->magicRoom = (gFieldStatuses & STATUS_FIELD_MAGIC_ROOM) ? gFieldTimers.magicRoomTimer : 0;
    snap->wonderRoom = (gFieldStatuses & STATUS_FIELD_WONDER_ROOM) ? gFieldTimers.wonderRoomTimer : 0;
    for (side = 0; side < NUM_BATTLE_SIDES; side++)
    {
        struct CoopFieldSide *dst = &snap->side[side];
        u32 st = gSideStatuses[side];

        dst->reflect = (st & SIDE_STATUS_REFLECT) ? gSideTimers[side].reflectTimer : 0;
        dst->lightScreen = (st & SIDE_STATUS_LIGHTSCREEN) ? gSideTimers[side].lightscreenTimer : 0;
        dst->auroraVeil = (st & SIDE_STATUS_AURORA_VEIL) ? gSideTimers[side].auroraVeilTimer : 0;
        dst->safeguard = (st & SIDE_STATUS_SAFEGUARD) ? gSideTimers[side].safeguardTimer : 0;
        dst->mist = (st & SIDE_STATUS_MIST) ? gSideTimers[side].mistTimer : 0;
        dst->tailwind = (st & SIDE_STATUS_TAILWIND) ? gSideTimers[side].tailwindTimer : 0;
        dst->spikes = IsHazardOnSide(side, HAZARDS_SPIKES) ? gSideTimers[side].spikesAmount : 0;
        dst->toxicSpikes = IsHazardOnSide(side, HAZARDS_TOXIC_SPIKES) ? gSideTimers[side].toxicSpikesAmount : 0;
        dst->stealthRock = IsHazardOnSide(side, HAZARDS_STEALTH_ROCK);
        dst->stickyWeb = IsHazardOnSide(side, HAZARDS_STICKY_WEB);
        dst->steelsurge = IsHazardOnSide(side, HAZARDS_STEELSURGE);
    }
    for (b = 0; b < MAX_BATTLERS_COUNT && b < gBattlersCount; b++)
    {
        struct CoopFieldBattler *dst = &snap->battler[b];

        if (!IsBattlerAlive(b) || (gAbsentBattlerFlags & (1u << b)))
            continue;
        dst->present = TRUE;
        dst->side = GetBattlerSide(b);
        // The species shown in battle (an Illusion shows its disguise).
        dst->species = GetMonData(GetIllusionMonPtr(b) != NULL ? GetIllusionMonPtr(b) : GetBattlerMon(b), MON_DATA_SPECIES);
        for (s = 0; s < NUM_BATTLE_STATS - 1; s++)
            dst->stages[s] = gBattleMons[b].statStages[s + 1] - DEFAULT_STAT_STAGE;
    }
}

// --- Text ------------------------------------------------------------------

#define PANEL_LINE_WIDTH 200
#define PANEL_LINE0_WIDTH 160   // leaves room for the page number
#define PANEL_MAX_PAGES  10

static EWRAM_DATA u8 sPages[PANEL_MAX_PAGES][2][64] = {0};
static EWRAM_DATA u8 sPageCount = 0;
static EWRAM_DATA u8 sPage = 0;
static EWRAM_DATA u8 sLine = 0;   // filling sPages[sPageCount - 1][sLine]

static void Panel_NewPage(const u8 *title)
{
    if (sPageCount >= PANEL_MAX_PAGES)
        return;
    StringCopy(sPages[sPageCount][0], title);
    sPages[sPageCount][1][0] = EOS;
    sPageCount++;
    sLine = 0;
}

// Add ", item" to the current line, or start the next line / page.
static void Panel_Add(const u8 *title, const u8 *item)
{
    u8 tmp[96];
    u8 *line;

    if (sPageCount == 0 || sPageCount > PANEL_MAX_PAGES)
        return;
    line = sPages[sPageCount - 1][sLine];
    StringCopy(tmp, line);
    if (line[0] != EOS && !(sLine == 0 && StringLength(line) == StringLength(title)))
        StringAppend(tmp, COMPOUND_STRING(", "));
    StringAppend(tmp, item);
    if (StringLength(tmp) < 63 && GetStringWidth(FONT_NORMAL, tmp, 0) <= (sLine == 0 ? PANEL_LINE0_WIDTH : PANEL_LINE_WIDTH))
    {
        StringCopy(line, tmp);
        return;
    }
    if (sLine == 0)
    {
        sLine = 1;
        StringCopy(sPages[sPageCount - 1][1], item);
        return;
    }
    Panel_NewPage(title);
    StringAppend(sPages[sPageCount - 1][0], item);
}

static void Panel_Turns(u8 *dst, const u8 *name, u8 turns, bool8 known)
{
    dst = StringCopy(dst, name);
    *dst++ = CHAR_SPACE;
    if (!known)
        StringCopy(dst, COMPOUND_STRING("(? turns)"));
    else if (turns == 0)
        StringCopy(dst, COMPOUND_STRING("(lasting)"));
    else
    {
        *dst++ = CHAR_LEFT_PAREN;
        dst = ConvertIntToDecimalStringN(dst, turns, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringCopy(dst, turns == 1 ? COMPOUND_STRING(" turn)") : COMPOUND_STRING(" turns)"));
    }
}

static const u8 *Panel_WeatherName(u16 weather)
{
    if (weather & B_WEATHER_RAIN)       return COMPOUND_STRING("Rain");
    if (weather & B_WEATHER_SUN)        return COMPOUND_STRING("Harsh sunlight");
    if (weather & B_WEATHER_SANDSTORM)  return COMPOUND_STRING("Sandstorm");
    if (weather & B_WEATHER_HAIL)       return COMPOUND_STRING("Hail");
    if (weather & B_WEATHER_SNOW)       return COMPOUND_STRING("Snow");
    if (weather & B_WEATHER_FOG)        return COMPOUND_STRING("Fog");
    if (weather & B_WEATHER_STRONG_WINDS) return COMPOUND_STRING("Strong winds");
    return NULL;
}

static const u8 *const sTerrainNames[B_TERRAIN_COUNT] = {
    [B_TERRAIN_GRASSY]   = COMPOUND_STRING("Grassy Terrain"),
    [B_TERRAIN_MISTY]    = COMPOUND_STRING("Misty Terrain"),
    [B_TERRAIN_ELECTRIC] = COMPOUND_STRING("Electric Terrain"),
    [B_TERRAIN_PSYCHIC]  = COMPOUND_STRING("Psychic Terrain"),
};

static const u8 *const sStatShort[NUM_BATTLE_STATS - 1] = {
    COMPOUND_STRING("Atk"), COMPOUND_STRING("Def"), COMPOUND_STRING("Spe"),
    COMPOUND_STRING("SpA"), COMPOUND_STRING("SpD"), COMPOUND_STRING("Acc"),
    COMPOUND_STRING("Eva"),
};

static void Panel_Side(const struct CoopFieldSnap *snap, u32 side, bool8 ours)
{
    const struct CoopFieldSide *s = &snap->side[side];
    const u8 *title = ours ? COMPOUND_STRING("OUR SIDE: ") : COMPOUND_STRING("FOE SIDE: ");
    u8 item[40];
    bool8 any = FALSE;

    Panel_NewPage(title);
    if (s->reflect)     { Panel_Turns(item, COMPOUND_STRING("Reflect"), s->reflect, ours); Panel_Add(title, item); any = TRUE; }
    if (s->lightScreen) { Panel_Turns(item, COMPOUND_STRING("Light Screen"), s->lightScreen, ours); Panel_Add(title, item); any = TRUE; }
    if (s->auroraVeil)  { Panel_Turns(item, COMPOUND_STRING("Aurora Veil"), s->auroraVeil, ours); Panel_Add(title, item); any = TRUE; }
    if (s->safeguard)   { Panel_Turns(item, COMPOUND_STRING("Safeguard"), s->safeguard, TRUE); Panel_Add(title, item); any = TRUE; }
    if (s->mist)        { Panel_Turns(item, COMPOUND_STRING("Mist"), s->mist, TRUE); Panel_Add(title, item); any = TRUE; }
    if (s->tailwind)    { Panel_Turns(item, COMPOUND_STRING("Tailwind"), s->tailwind, TRUE); Panel_Add(title, item); any = TRUE; }
    if (s->spikes)
    {
        u8 *p = StringCopy(item, COMPOUND_STRING("Spikes x"));
        ConvertIntToDecimalStringN(p, s->spikes, STR_CONV_MODE_LEFT_ALIGN, 1);
        Panel_Add(title, item);
        any = TRUE;
    }
    if (s->toxicSpikes)
    {
        u8 *p = StringCopy(item, COMPOUND_STRING("Toxic Spikes x"));
        ConvertIntToDecimalStringN(p, s->toxicSpikes, STR_CONV_MODE_LEFT_ALIGN, 1);
        Panel_Add(title, item);
        any = TRUE;
    }
    if (s->stealthRock) { Panel_Add(title, COMPOUND_STRING("Stealth Rock")); any = TRUE; }
    if (s->stickyWeb)   { Panel_Add(title, COMPOUND_STRING("Sticky Web")); any = TRUE; }
    if (s->steelsurge)  { Panel_Add(title, COMPOUND_STRING("Steelsurge")); any = TRUE; }
    if (!any)
        Panel_Add(title, COMPOUND_STRING("nothing"));
}

static void Panel_Build(const struct CoopFieldSnap *snap)
{
    const u8 *title = COMPOUND_STRING("FIELD: ");
    const u8 *weather = Panel_WeatherName(snap->weather);
    u8 item[48];
    u32 b, s;
    bool8 any = FALSE;

    sPageCount = 0;
    Panel_NewPage(title);
    if (weather != NULL)
    {
        // Rocks lengthen weather and nothing records whose they were.
        Panel_Turns(item, weather, snap->weatherTurns, snap->weatherTurns == 0);
        Panel_Add(title, item);
        any = TRUE;
    }
    if (snap->terrain != B_TERRAIN_NONE && snap->terrain < B_TERRAIN_COUNT)
    {
        Panel_Turns(item, sTerrainNames[snap->terrain], snap->terrainTurns, snap->terrainTurns == 0);
        Panel_Add(title, item);
        any = TRUE;
    }
    if (snap->trickRoom)  { Panel_Turns(item, COMPOUND_STRING("Trick Room"), snap->trickRoom, TRUE); Panel_Add(title, item); any = TRUE; }
    if (snap->gravity)    { Panel_Turns(item, COMPOUND_STRING("Gravity"), snap->gravity, TRUE); Panel_Add(title, item); any = TRUE; }
    if (snap->magicRoom)  { Panel_Turns(item, COMPOUND_STRING("Magic Room"), snap->magicRoom, TRUE); Panel_Add(title, item); any = TRUE; }
    if (snap->wonderRoom) { Panel_Turns(item, COMPOUND_STRING("Wonder Room"), snap->wonderRoom, TRUE); Panel_Add(title, item); any = TRUE; }
    if (!any)
        Panel_Add(title, COMPOUND_STRING("nothing"));

    Panel_Side(snap, B_SIDE_PLAYER, TRUE);
    Panel_Side(snap, B_SIDE_OPPONENT, FALSE);

    // Stat stages: modifiers only, never the foe's hidden stats.
    for (b = 0; b < MAX_BATTLERS_COUNT; b++)
    {
        const struct CoopFieldBattler *m = &snap->battler[b];
        u8 name[POKEMON_NAME_LENGTH + 12];
        u8 *p;
        bool8 changed = FALSE;

        if (!m->present || m->species == SPECIES_NONE || m->species >= NUM_SPECIES)
            continue;
        // With a partner on our side, say whose: YOUR (this player's
        // party) or ALLY; otherwise OUR. Decided on this game, from its own
        // view of each battler's party.
        if (m->side != B_SIDE_PLAYER)
            p = StringCopy(name, COMPOUND_STRING("FOE "));
        else if (!(gBattleTypeFlags & (BATTLE_TYPE_COOP | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI)))
            p = StringCopy(name, COMPOUND_STRING("OUR "));
        else if (GetBattlerParty(b) == gParties[B_TRAINER_PLAYER])
            p = StringCopy(name, COMPOUND_STRING("YOUR "));
        else
            p = StringCopy(name, COMPOUND_STRING("ALLY "));
        p = StringCopy(p, GetSpeciesName(m->species));
        StringCopy(p, COMPOUND_STRING(": "));
        Panel_NewPage(name);
        for (s = 0; s < NUM_BATTLE_STATS - 1; s++)
        {
            if (m->stages[s] == 0)
                continue;
            p = StringCopy(item, sStatShort[s]);
            *p++ = m->stages[s] > 0 ? CHAR_PLUS : CHAR_HYPHEN;
            ConvertIntToDecimalStringN(p, m->stages[s] > 0 ? m->stages[s] : -m->stages[s], STR_CONV_MODE_LEFT_ALIGN, 1);
            Panel_Add(name, item);
            changed = TRUE;
        }
        if (!changed)
            Panel_Add(name, COMPOUND_STRING("no stat changes"));
    }
}

static void Panel_Draw(void)
{
    static const u8 sColors[3] = {15, 1, 6};
    u8 text[150];
    u8 *p;

    p = StringCopy(text, sPages[sPage][0]);
    *p++ = CHAR_NEWLINE;
    p = StringCopy(p, sPages[sPage][1]);
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    FillWindowPixelBuffer(B_WIN_MSG, PIXEL_FILL(0xF));
    AddTextPrinterParameterized4(B_WIN_MSG, FONT_NORMAL, 0, 1, 0, 0, sColors, TEXT_SKIP_DRAW, text);
    // The page number and the keys, right-aligned on the first line.
    {
        u8 hint[24];
        u8 *h = ConvertIntToDecimalStringN(hint, sPage + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
        *h++ = CHAR_SLASH;
        h = ConvertIntToDecimalStringN(h, sPageCount, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringCopy(h, COMPOUND_STRING(" {LEFT_ARROW}{RIGHT_ARROW}"));
        AddTextPrinterParameterized4(B_WIN_MSG, FONT_SMALL, 208 - GetStringWidth(FONT_SMALL, hint, 0), 1, 0, 0, sColors, TEXT_SKIP_DRAW, hint);
    }
    PutWindowTilemap(B_WIN_MSG);
    CopyWindowToVram(B_WIN_MSG, COPYWIN_FULL);
}

bool8 CoopField_PanelAvailable(void)
{
    // PvP link battles: the sides differ per game; not offered there.
    if ((gBattleTypeFlags & BATTLE_TYPE_LINK) && !(gBattleTypeFlags & BATTLE_TYPE_COOP))
        return FALSE;
    return gCoopFieldSnap.valid;
}

void CoopField_OpenPanel(void)
{
    Panel_Build(&gCoopFieldSnap);
    sPage = 0;
    Panel_Draw();
}

// TRUE when closed (B, A or SELECT).
bool8 CoopField_PanelInput(void)
{
    if (JOY_NEW(B_BUTTON | A_BUTTON | SELECT_BUTTON))
    {
        PlaySE(SE_SELECT);
        FillWindowPixelBuffer(B_WIN_MSG, PIXEL_FILL(0xF));
        CopyWindowToVram(B_WIN_MSG, COPYWIN_FULL);
        return TRUE;
    }
    if (JOY_NEW(DPAD_RIGHT | DPAD_DOWN) && sPageCount > 1)
    {
        PlaySE(SE_SELECT);
        sPage = (sPage + 1) % sPageCount;
        Panel_Draw();
    }
    else if (JOY_NEW(DPAD_LEFT | DPAD_UP) && sPageCount > 1)
    {
        PlaySE(SE_SELECT);
        sPage = (sPage + sPageCount - 1) % sPageCount;
        Panel_Draw();
    }
    return FALSE;
}
