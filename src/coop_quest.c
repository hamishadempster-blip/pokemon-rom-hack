// Quest foundations, and the first quest built on them: New Mauville,
// "Power Problems".
//
// What each save keeps for itself (personal):
//   - its journal progress and the breakers it reset while not linked
//     (VAR_PP_BREAKERS),
//   - trainer flags (VOLT, FUSE), static encounters (ROLYCOLY, JOLTIK, ROTOM,
//     PORYGON), item balls, the chamber's log pages and panel,
//   - rewards, recorded once each in CoopSave.rewardsClaimed and in the stock
//     flags (FLAG_GOT_TM_THUNDERBOLT_FROM_WATTSON).
// What the two saves share:
//   - "the generator is off" (COOP_STORY_POWER_RESTORED). It travels on the
//     story ledger in src/coop.c, which only ever adds, and is written at a
//     map change, so nobody is moved or interrupted. A save that receives it
//     first gets FLAG_PP_CATCHUP: the crates are gone, VOLT has left, and
//     WATTSON still pays that player's own reward.
//   - breaker progress while both games are linked. Each change is sent at
//     once (deal blocks: ordered, resent until acknowledged). On a new link
//     both sides report and keep the higher count; a trip sent live resets
//     both. A partner elsewhere in the world still receives it.
// Nothing here waits for the partner, so the whole quest can be done alone.

#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_quest.h"
#include "battle_setup.h"
#include "constants/characters.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "field_camera.h"
#include "link.h"
#include "main.h"
#include "map_name_popup.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "constants/coop.h"
#include "constants/flags.h"
#include "constants/maps.h"
#include "constants/opponents.h"
#include "constants/vars.h"
#include "constants/metatile_labels.h"
#include "constants/species.h"
#include "wild_encounter.h"

// Breaker consoles: orange screen while tripped, green once reset.
#define CONSOLE_TOP_OFF  (0x2D7 | MAPGRID_IMPASSABLE)
#define CONSOLE_BOT_OFF  0x2DF
#define CONSOLE_TOP_ON   (0x2D6 | MAPGRID_IMPASSABLE)
#define CONSOLE_BOT_ON   0x2DE
// The sealed chamber's panel, south of the generator.
#define PANEL_X          34
#define PANEL_Y          9
#define PANEL_OPEN       0x27D

static const u8 sBreakerPos[PP_BREAKER_COUNT][2] = {
    [PP_BREAKER_STORAGE] = { 3, 22 },
    [PP_BREAKER_SERVICE] = { 25, 14 },
    [PP_BREAKER_CONTROL] = { 37, 1 },
};

static u8 CountBits(u16 v);
void CoopQuest_ApplyFoundObjects(void);

static EWRAM_DATA bool8 sReportPending = FALSE;
static EWRAM_DATA bool8 sRedraw = FALSE;
static EWRAM_DATA u8 sPartnerNote = 0; // 1 + breakers, or 0xFF for a trip
static EWRAM_DATA u8 sPartnerNoteQuest = 0; // a quest value the partner added to

static bool8 InNewMauvilleInside(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_NEW_MAUVILLE_INSIDE)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_NEW_MAUVILLE_INSIDE);
}

static bool8 PowerIsOff(void)
{
    return VarGet(VAR_NEW_MAUVILLE_STATE) >= 2;
}

static u8 Breakers(void)
{
    u16 n = VarGet(VAR_PP_BREAKERS);

    return n > PP_BREAKER_COUNT ? PP_BREAKER_COUNT : n;
}

static void DrawBreakers(void)
{
    u8 i, n = PowerIsOff() ? PP_BREAKER_COUNT : Breakers();

    for (i = 0; i < PP_BREAKER_COUNT; i++)
    {
        s32 x = sBreakerPos[i][0] + MAP_OFFSET;
        s32 y = sBreakerPos[i][1] + MAP_OFFSET;

        MapGridSetMetatileIdAt(x, y, i < n ? CONSOLE_TOP_ON : CONSOLE_TOP_OFF);
        MapGridSetMetatileIdAt(x, y + 1, i < n ? CONSOLE_BOT_ON : CONSOLE_BOT_OFF);
    }
}

// ON_LOAD of New Mauville: the consoles and the chamber panel as this save
// has them. The caller redraws the view when the map is already shown.
u16 CoopQuest_DrawNewMauville(void)
{
    DrawBreakers();
    if (FlagGet(FLAG_PP_CHAMBER_OPEN))
        MapGridSetMetatileIdAt(PANEL_X + MAP_OFFSET, PANEL_Y + MAP_OFFSET, PANEL_OPEN);
    return 0;
}

static void SetBreakers(u8 value, bool8 tell)
{
    VarSet(VAR_PP_BREAKERS, value);
    if (tell && Coop_SessionActive())
        CoopDeal_SendQuest(COOP_QUEST_PP_BREAKERS, value, TRUE);
}

// VAR_0x8004: PP_BREAKER_*. VAR_RESULT: PP_PRESS_*. The log's order is
// STORAGE, SERVICE, CONTROL; any other order trips them all.
u16 CoopQuest_PressBreaker(void)
{
    u8 which = gSpecialVar_0x8004;
    u8 n = Breakers();

    if (PowerIsOff())
        return PP_PRESS_DONE;
    if (which < n)
        return PP_PRESS_ALREADY;
    if (which == n)
    {
        SetBreakers(n + 1, TRUE);
        return PP_PRESS_RESET;
    }
    SetBreakers(0, TRUE);
    return PP_PRESS_TRIPPED;
}

// VAR_RESULT and STR_VAR_1: breakers reset so far.
u16 CoopQuest_BreakerCount(void)
{
    u8 n = PowerIsOff() ? PP_BREAKER_COUNT : Breakers();

    ConvertIntToDecimalStringN(gStringVar1, n, STR_CONV_MODE_LEFT_ALIGN, 1);
    return n;
}

// The main switch was thrown on this save.
u16 CoopQuest_PowerRestored(void)
{
    VarSet(VAR_NEW_MAUVILLE_STATE, 2);
    FlagSet(FLAG_HIDE_PP_CREW);
    FlagSet(FLAG_HIDE_PP_SHUTTER);
    FlagClear(FLAG_HIDE_PP_SHOP);
    SetBreakers(PP_BREAKER_COUNT, TRUE);
    return 0;
}

// The partner's save has the generator off and this one does not: write the
// world as it is now. Called at a map change, before objects load.
bool8 CoopQuest_ApplyPowerRestored(void)
{
    if (!PowerIsOff())
    {
        VarSet(VAR_NEW_MAUVILLE_STATE, 2);
        VarSet(VAR_PP_BREAKERS, PP_BREAKER_COUNT);
        FlagSet(FLAG_PP_CATCHUP);
        FlagSet(FLAG_HIDE_PP_CREW);
        FlagSet(FLAG_HIDE_PP_SHUTTER);
        FlagClear(FLAG_HIDE_PP_SHOP);
    }
    return TRUE;
}

// Quest values merged with OR: whatever either player found counts for both.
static const struct { u8 quest; u16 var; } sOrValues[] = {
    { COOP_QUEST_SHOAL_CLUES,   VAR_SHOAL_CLUES },
    { COOP_QUEST_CARAVAN_FOUND, VAR_CARAVAN_FOUND },
    { COOP_QUEST_COAST_CLUES,   VAR_COAST_CLUES },
    { COOP_QUEST_SHRINE_MARKS,  VAR_SHRINE_MARKS },
};

// Which bits of each value travel (the caravan's glyph progress does not).
static u16 SharedBits(u8 quest, u16 value)
{
    if (quest == COOP_QUEST_CARAVAN_FOUND)
        return value & CARAVAN_CRATE_ALL;
    return value & 0xFF;
}

static bool8 InShrine(void);
#define LANTERN_WINDOW  (60 * 12)   // frames: about 12 seconds
static EWRAM_DATA u8 sMyLantern = 0;           // 1 or 2: the lantern this player lit last
static EWRAM_DATA u32 sMyLanternTime = 0;
static EWRAM_DATA bool8 sLanternNote = FALSE;  // show "both lanterns" when the field is free
static EWRAM_DATA u8 sPartnerLantern = 0;      // 1 or 2: the lantern the partner lit
static EWRAM_DATA u32 sPartnerLanternTime = 0;  // gMain.vblankCounter1 when it arrived

static void ApplyOrValue(u8 quest, u16 var, u8 value)
{
    u16 now = VarGet(var);
    u16 add = SharedBits(quest, value) & ~now;

    if (add == 0)
        return;
    VarSet(var, now | add);
    CoopQuest_ApplyFoundObjects();
    sPartnerNoteQuest = quest;
}

static u8 ShrineGates(void)
{
    return (FlagGet(FLAG_SHRINE_GATE_PLATES) ? 1 : 0) | (FlagGet(FLAG_SHRINE_GATE_LANTERNS) ? 2 : 0)
         | (FlagGet(FLAG_SHRINE_SOLVED) ? 4 : 0);
}


// The sanctum door, as TwinBeaconShrine_EventScript_OpenSanctum draws it:
// for a partner who answered the riddle while this player is in the hall.
static void OpenSanctumNow(void)
{
    if (!InShrine() || gMain.callback2 != CB2_Overworld)
        return;
    MapGridSetMetatileIdAt(7 + MAP_OFFSET, 19 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_TopLeft | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(8 + MAP_OFFSET, 19 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_TopMid | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(9 + MAP_OFFSET, 19 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_TopRight | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(7 + MAP_OFFSET, 20 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_BottomLeft | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(8 + MAP_OFFSET, 20 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_BottomMid);
    MapGridSetMetatileIdAt(9 + MAP_OFFSET, 20 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_BottomRight | MAPGRID_IMPASSABLE);
    DrawWholeMapView();
}

// After a shrine gate opens here: the partner's save opens it too.
u16 CoopQuest_ShareGates(void)
{
    if (Coop_SessionActive())
        CoopDeal_SendQuest(COOP_QUEST_SHRINE_GATES, ShrineGates(), TRUE);
    return 0;
}

void CoopQuest_HandleSync(u8 quest, u8 value, bool8 live)
{
    u8 now, next, i;

    for (i = 0; i < ARRAY_COUNT(sOrValues); i++)
    {
        if (sOrValues[i].quest == quest)
        {
            ApplyOrValue(quest, sOrValues[i].var, value);
            if (!live && (SharedBits(quest, VarGet(sOrValues[i].var)) & ~value) != 0)
                sReportPending = TRUE;
            return;
        }
    }
    if (quest == COOP_QUEST_TWIN_HEARD)
    {
        VarSet(VAR_TWIN_HEARD, VarGet(VAR_TWIN_HEARD) | value);
        return;
    }
    if (quest == COOP_QUEST_TRIAL_HEAL)
    {
        if (live && InShrine())
            HealPlayerParty();
        return;
    }
    if (quest == COOP_QUEST_SHRINE_GATES)
    {
        // The twin gates are one world: opened together (or by one player),
        // they stand open for both. A later link merges them too.
        if ((value & 1) && !FlagGet(FLAG_SHRINE_GATE_PLATES))
            FlagSet(FLAG_SHRINE_GATE_PLATES);
        if ((value & 2) && !FlagGet(FLAG_SHRINE_GATE_LANTERNS))
            FlagSet(FLAG_SHRINE_GATE_LANTERNS);
        if ((value & 4) && !FlagGet(FLAG_SHRINE_SOLVED))
        {
            FlagSet(FLAG_SHRINE_SOLVED);
            OpenSanctumNow();
        }
        if (!live && (ShrineGates() & ~value) != 0)
            sReportPending = TRUE;
        return;
    }
    if (quest == COOP_QUEST_SHRINE_LANTERN)
    {
        if (live && (value == 1 || value == 2))
        {
            sPartnerLantern = value;
            sPartnerLanternTime = gMain.vblankCounter1;
            // Lit at the same moment: each side lit before hearing of the
            // other, so the later message opens the gate here as well.
            if (InShrine() && !FlagGet(FLAG_SHRINE_GATE_LANTERNS) && FlagGet(FLAG_SHRINE_GATE_PLATES)
             && sMyLantern != 0 && sMyLantern != value
             && gMain.vblankCounter1 - sMyLanternTime <= LANTERN_WINDOW)
            {
                FlagSet(FLAG_SHRINE_GATE_LANTERNS);
                CoopDeal_SendQuest(COOP_QUEST_SHRINE_GATES, ShrineGates(), TRUE);
                sLanternNote = TRUE;
            }
        }
        return;
    }
    if (quest != COOP_QUEST_PP_BREAKERS)
        return;
    if (value > PP_BREAKER_COUNT)
        value = PP_BREAKER_COUNT;
    if (PowerIsOff())
        return;
    now = Breakers();
    next = live ? value : (value > now ? value : now);
    if (!live && now > value)
        sReportPending = TRUE;
    if (next == now)
        return;
    VarSet(VAR_PP_BREAKERS, next);
    sRedraw = TRUE;
    sPartnerNote = next < now ? 0xFF : next + 1;
}

void CoopQuest_OnSessionStart(void)
{
    sReportPending = TRUE;
    sPartnerNote = 0;
}

// The partner found a clue or crate on this map: take it away now, not only
// at the next map load (the flags were set when the value merged).
static void HidePartnerFoundObjects(void)
{
    static const u16 sFlags[] = {
        FLAG_SHOAL_CLUE_THERMO_TAKEN, FLAG_HIDE_CARAVAN_CRATE_1,
        FLAG_HIDE_CARAVAN_CRATE_2, FLAG_HIDE_CARAVAN_CRATE_3,
    };
    u8 i, j;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *obj = &gObjectEvents[i];
        u16 flag;

        if (!obj->active || obj->isPlayer || obj->mapNum != gSaveBlock1Ptr->location.mapNum
         || obj->mapGroup != gSaveBlock1Ptr->location.mapGroup)
            continue;
        flag = GetObjectEventFlagIdByLocalIdAndMap(obj->localId, obj->mapNum, obj->mapGroup);
        for (j = 0; j < ARRAY_COUNT(sFlags); j++)
        {
            if (flag == sFlags[j] && FlagGet(flag))
            {
                RemoveObjectEventByLocalIdAndMap(obj->localId, obj->mapNum, obj->mapGroup);
                break;
            }
        }
    }
}

// Every frame while linked (src/coop_deal.c).
void CoopQuest_Poll(void)
{
    if (sReportPending)
    {
        u8 i;

        sReportPending = FALSE;
        if (!PowerIsOff() && Breakers() != 0)
            CoopDeal_SendQuest(COOP_QUEST_PP_BREAKERS, Breakers(), FALSE);
        for (i = 0; i < ARRAY_COUNT(sOrValues); i++)
        {
            u16 v = SharedBits(sOrValues[i].quest, VarGet(sOrValues[i].var));
            if (v != 0)
                CoopDeal_SendQuest(sOrValues[i].quest, v, FALSE);
        }
        if ((VarGet(VAR_TWIN_ARC) & 0xFF) != 0)
            CoopDeal_SendQuest(COOP_QUEST_TWIN_HEARD, VarGet(VAR_TWIN_ARC) & 0xFF, FALSE);
        if (ShrineGates() != 0)
            CoopDeal_SendQuest(COOP_QUEST_SHRINE_GATES, ShrineGates(), FALSE);
    }
    if (sLanternNote && gMain.callback2 == CB2_Overworld && !gMain.inBattle && !gPaletteFade.active
     && !ScriptContext_IsEnabled())
    {
        sLanternNote = FALSE;
        StringCopy(gCoopPopupText, COMPOUND_STRING("Both lanterns burn together!"));
        ShowCoopPopup();
    }
    if (sPartnerNoteQuest != 0 && gMain.callback2 == CB2_Overworld && !gMain.inBattle && !gPaletteFade.active)
    {
        u8 *end = StringCopy(gCoopPopupText, gLinkPlayers[GetMultiplayerId() ^ 1].name);

        switch (sPartnerNoteQuest)
        {
        case COOP_QUEST_SHOAL_CLUES:   StringCopy(end, COMPOUND_STRING(" found a clue (SHOAL)")); break;
        case COOP_QUEST_CARAVAN_FOUND: StringCopy(end, COMPOUND_STRING(" found a crate (RUINS)")); break;
        case COOP_QUEST_COAST_CLUES:   StringCopy(end, COMPOUND_STRING(" found a clue (COAST)")); break;
        default:                       StringCopy(end, COMPOUND_STRING(" read an inscription")); break;
        }
        sPartnerNoteQuest = 0;
        HidePartnerFoundObjects();
        ShowCoopPopup();
    }
    if (!sRedraw && sPartnerNote == 0)
        return;
    if (gMain.callback2 != CB2_Overworld || gMain.inBattle || gPaletteFade.active)
        return;
    if (sRedraw)
    {
        sRedraw = FALSE;
        if (InNewMauvilleInside())
        {
            DrawBreakers();
            DrawWholeMapView();
        }
    }
    if (sPartnerNote != 0)
    {
        u8 *end = StringCopy(gCoopPopupText, gLinkPlayers[GetMultiplayerId() ^ 1].name);

        if (sPartnerNote == 0xFF)
        {
            StringCopy(end, COMPOUND_STRING(": breakers tripped"));
        }
        else
        {
            end = StringCopy(end, COMPOUND_STRING(": breakers "));
            end = ConvertIntToDecimalStringN(end, sPartnerNote - 1, STR_CONV_MODE_LEFT_ALIGN, 1);
            StringCopy(end, COMPOUND_STRING("/3"));
        }
        sPartnerNote = 0;
        ShowCoopPopup();
    }
}

// TRUE when the partner can join a duo fight here: linked, on this map, the
// Twin Beacon on, and both sides able to fight.
u16 CoopQuest_DuoReady(void)
{
    return Coop_SessionActive() && Coop_PartnerOnSameMap() && Coop_BeaconActive()
        && Coop_HasBattleReadyLead() && !Coop_OtherInteractionBusy();
}

// The next trainer battle is this player's alone (the duo was declined).
u16 CoopQuest_ForceSolo(void)
{
    gCoopForceSolo = TRUE;
    return 0;
}

// VAR_0x8004: COOP_REWARD_*. VAR_RESULT: TRUE if it was not yet claimed on
// this save (and now is).
u16 CoopQuest_ClaimReward(void)
{
    struct CoopSave *save = Coop_GetSave();
    u8 bit = gSpecialVar_0x8004;

    if (save == NULL || bit >= 8 * sizeof(save->rewardsClaimed))
        return FALSE;
    if (save->rewardsClaimed[bit / 8] & (1 << (bit % 8)))
        return FALSE;
    save->rewardsClaimed[bit / 8] |= 1 << (bit % 8);
    return TRUE;
}

static bool8 RewardClaimed(u8 bit)
{
    struct CoopSave *save = Coop_GetSave();

    return save != NULL && bit < 8 * sizeof(save->rewardsClaimed)
        && (save->rewardsClaimed[bit / 8] & (1 << (bit % 8)));
}

u16 CoopQuest_RewardClaimed(void)
{
    return RewardClaimed(gSpecialVar_0x8004);
}

static u8 PagesRead(void)
{
    return FlagGet(FLAG_PP_PAGE_1) + FlagGet(FLAG_PP_PAGE_2) + FlagGet(FLAG_PP_PAGE_3);
}

// VAR_RESULT and STR_VAR_1: log pages found.
u16 CoopQuest_PagesRead(void)
{
    u8 n = PagesRead();

    ConvertIntToDecimalStringN(gStringVar1, n, STR_CONV_MODE_LEFT_ALIGN, 1);
    return n;
}

u16 CoopQuest_OpenChamber(void)
{
    FlagSet(FLAG_PP_CHAMBER_OPEN);
    MapGridSetMetatileIdAt(PANEL_X + MAP_OFFSET, PANEL_Y + MAP_OFFSET, PANEL_OPEN);
    return 0;
}

// ---------------------------------------------------------------- journal

static u8 *AddLine(u8 *end, const u8 *text, bool8 *first)
{
    if (!*first)
    {
        *end++ = CHAR_PROMPT_CLEAR;
    }
    *first = FALSE;
    return StringCopy(end, text);
}

static u8 *JournalPowerProblems(u8 *end, bool8 *firstp)
{
    bool8 first = *firstp;
    bool8 started = FlagGet(FLAG_GOT_BASEMENT_KEY_FROM_WATTSON) || FlagGet(FLAG_PP_BRIEFED);
    bool8 paid = FlagGet(FLAG_GOT_TM_THUNDERBOLT_FROM_WATTSON);
    u8 n;

    if (!PowerIsOff() && !started)
    {
        end = AddLine(end, COMPOUND_STRING("POWER PROBLEMS: not started.\nWATTSON in MAUVILLE may need help."), &first);
    }
    else if (!PowerIsOff())
    {
        n = Breakers();
        end = AddLine(end, COMPOUND_STRING("POWER PROBLEMS: NEW MAUVILLE.\nBreakers reset: "), &first);
        end = ConvertIntToDecimalStringN(end, n, STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" of 3."));
        if (FlagGet(FLAG_PP_LOG_READ))
            end = AddLine(end, COMPOUND_STRING("The log's order: STORAGE, then\nSERVICE, then CONTROL."), &first);
        else
            end = AddLine(end, COMPOUND_STRING("A maintenance log by the stairs\nshould say how to reset them."), &first);
        if (n >= PP_BREAKER_COUNT && !HasTrainerBeenFought(TRAINER_PP_VOLT))
            end = AddLine(end, COMPOUND_STRING("Someone is standing on the\nMAIN SWITCH by the generator."), &first);
        else if (n >= PP_BREAKER_COUNT)
            end = AddLine(end, COMPOUND_STRING("The MAIN SWITCH is clear.\nStep on it to stop the generator."), &first);
    }
    else if (!paid)
    {
        end = AddLine(end, COMPOUND_STRING("POWER PROBLEMS: the generator\nis off. Tell WATTSON in MAUVILLE."), &first);
        if (FlagGet(FLAG_PP_CATCHUP))
            end = AddLine(end, COMPOUND_STRING("Your partner stopped it first.\nYour reward is still waiting."), &first);
    }
    else
    {
        end = AddLine(end, COMPOUND_STRING("POWER PROBLEMS: complete.\nWATTSON's reward: received."), &first);
    }

    if (PowerIsOff())
    {
        end = AddLine(end, COMPOUND_STRING("APPLIANCE ROOM: open.\nROTOM: "), &first);
        end = StringCopy(end, FlagGet(FLAG_PP_ROTOM_DONE) ? COMPOUND_STRING("met.") : COMPOUND_STRING("still inside."));
        end = AddLine(end, COMPOUND_STRING("POWER SHOP: open in MAUVILLE.\nMore stock as you earn badges."), &first);
    }
    if (FlagGet(FLAG_PP_CHAMBER_OPEN))
    {
        end = AddLine(end, COMPOUND_STRING("SEALED CHAMBER: open.\nCache: "), &first);
        end = StringCopy(end, FlagGet(FLAG_ITEM_PP_CHAMBER_UPGRADE) ? COMPOUND_STRING("found.") : COMPOUND_STRING("not found yet."));
    }
    else if (PagesRead() != 0)
    {
        end = AddLine(end, COMPOUND_STRING("Old log pages found: "), &first);
        end = ConvertIntToDecimalStringN(end, PagesRead(), STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" of 3.\nThey mention a sealed chamber."));
    }
    *firstp = first;
    return end;
}


static bool8 Claimed(u8 bit)
{
    return RewardClaimed(bit);
}

static u8 *JournalShoal(u8 *end, bool8 *first)
{
    u16 state = VarGet(VAR_SHOAL_QUEST);
    u8 clues = CountBits(VarGet(VAR_SHOAL_CLUES) & SHOAL_CLUE_ALL);

    if (state == SHOAL_NONE && clues == 0)
        return AddLine(end, COMPOUND_STRING("MISSING RESEARCHER: not started.\nAsk at SHOAL CAVE on ROUTE 125."), first);
    if (state < SHOAL_RESCUED)
    {
        end = AddLine(end, COMPOUND_STRING("MISSING RESEARCHER: ISLA is\nlost somewhere in SHOAL CAVE."), first);
        end = AddLine(end, COMPOUND_STRING("Her dropped gear found: "), first);
        end = ConvertIntToDecimalStringN(end, clues, STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" of 3.\nShared with your partner."));
        if (!(VarGet(VAR_SHOAL_CLUES) & SHOAL_CLUE_NOTES))
            end = AddLine(end, COMPOUND_STRING("Hint: her notes would say where\nshe went. Try the inner cave."), first);
        else
            end = AddLine(end, COMPOUND_STRING("Notes: she slid down the ice\nby the TM ledge in the B1F ice room."), first);
        // The lower rooms are only dry at low tide (UpdateShoalTideFlag).
        end = AddLine(end, COMPOUND_STRING("Low tide uncovers the paths down:\n3:00-8:59 and 15:00-20:59."), first);
        if (state == SHOAL_FOUND && !(VarGet(VAR_SHOAL_CLUES) & SHOAL_CLUE_RADIO))
            end = AddLine(end, COMPOUND_STRING("Found her! She needs her RADIO.\nIt fell near the ice room ladder."), first);
        else if (state == SHOAL_FOUND)
            end = AddLine(end, COMPOUND_STRING("You have her RADIO.\nTake it to her in the ice room."), first);
        return end;
    }
    end = AddLine(end, COMPOUND_STRING("MISSING RESEARCHER: ISLA is safe\nat SHOAL CAVE's entrance."), first);
    if (FlagGet(FLAG_SHOAL_CATCHUP) && !Claimed(COOP_REWARD_SHOAL_RESCUE))
        end = AddLine(end, COMPOUND_STRING("Your partner found her first.\nShe still has a gift for you."), first);
    else if (!Claimed(COOP_REWARD_SHOAL_RESCUE))
        end = AddLine(end, COMPOUND_STRING("Her thank-you gift is waiting.\nMake room in your bag."), first);
    end = AddLine(end, COMPOUND_STRING("ISLA gives one ICE STONE a day.\nALOLAN VULPIX live in the ice room."), first);
    return end;
}

static u8 *JournalCaravan(u8 *end, bool8 *first)
{
    u16 state = VarGet(VAR_CARAVAN_QUEST);
    u8 crates = CountBits(VarGet(VAR_CARAVAN_FOUND) & CARAVAN_CRATE_ALL);

    if (state == CARAVAN_NONE)
        return AddLine(end, COMPOUND_STRING("DESERT CARAVAN: not started.\nA camp waits south of the desert."), first);
    if (state < CARAVAN_MERCHANT)
    {
        end = AddLine(end, COMPOUND_STRING("DESERT CARAVAN: supply crates are\nin the SANDBURIED RUINS."), first);
        end = AddLine(end, COMPOUND_STRING("Crates found: "), first);
        end = ConvertIntToDecimalStringN(end, crates, STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" of 3.\nShared with your partner."));
        end = AddLine(end, COMPOUND_STRING("The ruins rise from the sand\neast of the MIRAGE TOWER."), first);
        if (crates >= 3)
            end = AddLine(end, COMPOUND_STRING("All crates found! Report to\nZAHRA at the caravan camp."), first);
        return end;
    }
    end = AddLine(end, COMPOUND_STRING("DESERT CARAVAN: resupplied.\nTheir MERCHANT trades every day."), first);
    end = AddLine(end, COMPOUND_STRING("Camp: ROUTE 111, just south\nof the desert. Stock grows with badges."), first);
    if (FlagGet(FLAG_CARAVAN_CATCHUP) && !Claimed(COOP_REWARD_CARAVAN))
        end = AddLine(end, COMPOUND_STRING("Your partner beat you to it.\nZAHRA still has a gift for you."), first);
    else if (!Claimed(COOP_REWARD_CARAVAN))
        end = AddLine(end, COMPOUND_STRING("ZAHRA's thank-you gift is\nwaiting. Make room in your bag."), first);
    if (FlagGet(FLAG_CARAVAN_CHAMBER_OPEN))
    {
        end = AddLine(end, COMPOUND_STRING("RUINS CHAMBER: open.\nIts guardian: "), first);
        end = StringCopy(end, FlagGet(FLAG_HIDE_CARAVAN_SIGILYPH) ? COMPOUND_STRING("met.") : COMPOUND_STRING("waiting."));
    }
    else
        end = AddLine(end, COMPOUND_STRING("Three glyph stones in the ruins\nhint at a sealed chamber."), first);
    return end;
}

static u8 *JournalTwins(u8 *end, bool8 *first)
{
    u16 arc = VarGet(VAR_TWIN_ARC);
    u16 heard = VarGet(VAR_TWIN_HEARD);
    u16 seen = arc | heard;
    u8 s = CountBits(seen & (TWIN_ARC_S1 | TWIN_ARC_S2 | TWIN_ARC_S3));
    u8 m = CountBits(seen & (TWIN_ARC_M1 | TWIN_ARC_M2 | TWIN_ARC_M3));

    end = AddLine(end, COMPOUND_STRING("SEPARATE AMBITIONS: SOREN chases\nstrength, MIRA chases mysteries."), first);
    end = AddLine(end, COMPOUND_STRING("SOREN meetings: "), first);
    end = ConvertIntToDecimalStringN(end, s, STR_CONV_MODE_LEFT_ALIGN, 1);
    end = StringCopy(end, COMPOUND_STRING(" of 3.\nMIRA meetings: "));
    end = ConvertIntToDecimalStringN(end, m, STR_CONV_MODE_LEFT_ALIGN, 1);
    end = StringCopy(end, COMPOUND_STRING(" of 3."));
    if (!(seen & TWIN_ARC_S1))
        end = AddLine(end, COMPOUND_STRING("SOREN: training near the\nBATTLE TENT in SLATEPORT."), first);
    else if (!(seen & TWIN_ARC_S2))
        end = AddLine(end, COMPOUND_STRING("SOREN: next at FORTREE's west\nedge, with six badges."), first);
    else if (!(seen & TWIN_ARC_S3))
        end = AddLine(end, COMPOUND_STRING("SOREN: next at MOSSDEEP,\nwith seven badges."), first);
    if (!(seen & TWIN_ARC_M1))
        end = AddLine(end, COMPOUND_STRING("MIRA: studying the shore on\nROUTE 109 near SLATEPORT."), first);
    else if (!(seen & TWIN_ARC_M2))
        end = AddLine(end, COMPOUND_STRING("MIRA: next at the caravan camp\non ROUTE 111, with four badges."), first);
    else if (!(seen & TWIN_ARC_M3))
        end = AddLine(end, COMPOUND_STRING("MIRA: next at SHOAL CAVE's\nentrance, with seven badges."), first);
    if (s == 3 && m == 3 && !(arc & TWIN_ARC_EXHIBITION))
        end = AddLine(end, COMPOUND_STRING("Both twins wait in LITTLEROOT\nfor an exhibition battle."), first);
    else if (arc & TWIN_ARC_EXHIBITION)
    {
        end = AddLine(end, COMPOUND_STRING("Exhibition: won. The twins'\ngifts: "), first);
        end = StringCopy(end, Claimed(COOP_REWARD_TWIN_SOREN) && Claimed(COOP_REWARD_TWIN_MIRA) ? COMPOUND_STRING("received.") : COMPOUND_STRING("waiting (bag full?)."));
    }
    return end;
}

static u8 *JournalCoast(u8 *end, bool8 *first)
{
    u16 state = VarGet(VAR_COAST_QUEST);
    u8 clues = CountBits(VarGet(VAR_COAST_CLUES) & COAST_CLUE_ALL);

    if (state == COAST_NONE && clues == 0)
        return AddLine(end, COMPOUND_STRING("COASTAL CLEANUP: not started.\nFishers on ROUTE 109 seem worried."), first);
    if (state < COAST_CLEAN)
    {
        end = AddLine(end, COMPOUND_STRING("COASTAL CLEANUP: the water by\nROUTE 109's west beach is fouled."), first);
        end = AddLine(end, COMPOUND_STRING("Clues found: "), first);
        end = ConvertIntToDecimalStringN(end, clues, STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" of 3.\nShared with your partner."));
        if (clues < 3)
            end = AddLine(end, COMPOUND_STRING("Look at the water's edge, the\nsand, and the rocks at the far end."), first);
        else if (state < COAST_CREW_BEATEN)
            end = AddLine(end, COMPOUND_STRING("A TEAM MAGMA crew is dumping\nat the beach's south point."), first);
        else
            end = AddLine(end, COMPOUND_STRING("Their chief SLAG is still\nthere. Make him stop."), first);
        return end;
    }
    end = AddLine(end, COMPOUND_STRING("COASTAL CLEANUP: the beach is\nclean again."), first);
    end = AddLine(end, COMPOUND_STRING("Better fishing at ROUTE 109 and\nDEWFORD: SKRELP bite now."), first);
    if (FlagGet(FLAG_COAST_CATCHUP) && !Claimed(COOP_REWARD_COAST))
        end = AddLine(end, COMPOUND_STRING("Your partner finished it.\nMARLIN still has a gift for you."), first);
    else if (!Claimed(COOP_REWARD_COAST))
        end = AddLine(end, COMPOUND_STRING("MARLIN's thank-you gift is\nwaiting. Make room in your bag."), first);
    return end;
}

static u8 *JournalBeacon(u8 *end, bool8 *first)
{
    u8 marks = CountBits(VarGet(VAR_SHRINE_MARKS) & SHRINE_MARK_ALL);

    if (marks == 0 && !FlagGet(FLAG_SHRINE_FOUND))
        return AddLine(end, COMPOUND_STRING("BEACON HISTORY: not started.\nOld ruins may mention twin lights."), first);
    if (!FlagGet(FLAG_SHRINE_FOUND))
    {
        end = AddLine(end, COMPOUND_STRING("BEACON HISTORY: twin-light\ninscriptions read: "), first);
        end = ConvertIntToDecimalStringN(end, marks, STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" of 3."));
        end = AddLine(end, COMPOUND_STRING("Look in the DESERT RUINS, the\nISLAND CAVE and the ANCIENT TOMB."), first);
        if (marks == 3)
            end = AddLine(end, COMPOUND_STRING("All three point to a hidden\npassage in the SCORCHED SLAB."), first);
        return end;
    }
    end = AddLine(end, COMPOUND_STRING("BEACON HISTORY: the shrine is\nopen, behind the SCORCHED SLAB."), first);
    if (!FlagGet(FLAG_SHRINE_SOLVED))
    {
        end = AddLine(end, COMPOUND_STRING("Twin plates: "), first);
        end = StringCopy(end, FlagGet(FLAG_SHRINE_GATE_PLATES) ? COMPOUND_STRING("open.") : COMPOUND_STRING("both need weight.\nA partner, or the idol stone."));
        end = AddLine(end, COMPOUND_STRING("Twin lanterns: "), first);
        end = StringCopy(end, FlagGet(FLAG_SHRINE_GATE_LANTERNS) ? COMPOUND_STRING("lit.") : COMPOUND_STRING("light both close\ntogether, apart or alone."));
        end = AddLine(end, COMPOUND_STRING("The sanctum door asks a riddle.\nTwo half-verses hold the answer."), first);
        return end;
    }
    end = AddLine(end, COMPOUND_STRING("Sanctum: open. "), first);
    end = StringCopy(end, FlagGet(FLAG_SHRINE_LORE_READ) ? COMPOUND_STRING("Lore read:\nBEACON STYLE and COMPASS on.") : COMPOUND_STRING("Hear the\nKEEPER's story."));
    end = AddLine(end, COMPOUND_STRING("TWIN TRIAL: three duo rounds.\nFirst win: "), first);
    end = StringCopy(end, Claimed(COOP_REWARD_TOURNEY) ? COMPOUND_STRING("done.") : COMPOUND_STRING("not yet."));
    if (VarGet(VAR_SHRINE_TOURNEY) != 0)
    {
        end = AddLine(end, COMPOUND_STRING("Trial under way: round "), first);
        end = ConvertIntToDecimalStringN(end, VarGet(VAR_SHRINE_TOURNEY), STR_CONV_MODE_LEFT_ALIGN, 1);
        end = StringCopy(end, COMPOUND_STRING(" next.\nSpeak to the KEEPER."));
    }
    return end;
}

// gStringVar4: one quest's journal, a page each step. VAR_0x8004:
// COOP_JOURNAL_*. VAR_RESULT: unused.
u16 CoopQuest_BufferJournal(void)
{
    u8 *end = gStringVar4;
    bool8 first = TRUE;

    switch (gSpecialVar_0x8004)
    {
    default:
    case COOP_JOURNAL_PP:      end = JournalPowerProblems(end, &first); break;
    case COOP_JOURNAL_SHOAL:   end = JournalShoal(end, &first); break;
    case COOP_JOURNAL_CARAVAN: end = JournalCaravan(end, &first); break;
    case COOP_JOURNAL_TWINS:   end = JournalTwins(end, &first); break;
    case COOP_JOURNAL_COAST:   end = JournalCoast(end, &first); break;
    case COOP_JOURNAL_BEACON:  end = JournalBeacon(end, &first); break;
    }
    return 0;
}

// ------------------------------------------------------- optional quests
//
// Five optional quests share these rules (docs/COOP_QUESTS.md):
//  - World outcomes are story ledger milestones (src/coop.c): append only,
//    applied to the other save at its next map change, with a catch-up flag
//    so that player still gets their own reward and a short line instead of
//    the full scene.
//  - Clues, crates and inscriptions are OR-merged quest values: whatever
//    either player finds counts for both, live while linked and merged on
//    every new link.
//  - Rewards are per save (CoopSave.rewardsClaimed), given only when the bag
//    has room, so a full bag defers them.

// Hide the clue and crate objects whose bit is in the value (found by this
// player or the partner). The object stays drawn until the next map load.
void CoopQuest_ApplyFoundObjects(void)
{
    u16 clues = VarGet(VAR_SHOAL_CLUES);
    u16 crates = VarGet(VAR_CARAVAN_FOUND);

    if (clues & SHOAL_CLUE_THERMO)
        FlagSet(FLAG_SHOAL_CLUE_THERMO_TAKEN);
    if (clues & SHOAL_CLUE_NOTES)
        FlagSet(FLAG_SHOAL_CLUE_NOTES_TAKEN);
    if (clues & SHOAL_CLUE_RADIO)
        FlagSet(FLAG_SHOAL_CLUE_RADIO_TAKEN);
    if (crates & 1)
        FlagSet(FLAG_HIDE_CARAVAN_CRATE_1);
    if (crates & 2)
        FlagSet(FLAG_HIDE_CARAVAN_CRATE_2);
    if (crates & 4)
        FlagSet(FLAG_HIDE_CARAVAN_CRATE_3);
}

static u16 OrVar(u8 quest)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sOrValues); i++)
        if (sOrValues[i].quest == quest)
            return sOrValues[i].var;
    return 0;
}

static u8 CountBits(u16 v)
{
    u8 n = 0;

    while (v)
    {
        n += v & 1;
        v >>= 1;
    }
    return n;
}

// VAR_0x8004: COOP_QUEST_* (an OR value), VAR_0x8005: bits found now.
// VAR_RESULT: TRUE if any bit is new. STR_VAR_1: how many are found.
u16 CoopQuest_SetBits(void)
{
    u8 quest = gSpecialVar_0x8004;
    u16 var = OrVar(quest);
    u16 now, add;

    if (var == 0)
        return FALSE;
    now = VarGet(var);
    add = gSpecialVar_0x8005 & ~now;
    VarSet(var, now | gSpecialVar_0x8005);
    CoopQuest_ApplyFoundObjects();
    ConvertIntToDecimalStringN(gStringVar1, CountBits(SharedBits(quest, VarGet(var))), STR_CONV_MODE_LEFT_ALIGN, 1);
    if (add != 0 && Coop_SessionActive())
        CoopDeal_SendQuest(quest, SharedBits(quest, VarGet(var)), TRUE);
    return add != 0;
}

// VAR_0x8004: COOP_QUEST_*, VAR_0x8005: bits. TRUE when all are found.
u16 CoopQuest_HasBits(void)
{
    u16 var = OrVar(gSpecialVar_0x8004);

    return var != 0 && (VarGet(var) & gSpecialVar_0x8005) == gSpecialVar_0x8005;
}

// VAR_0x8004: COOP_QUEST_*. VAR_RESULT and STR_VAR_1: bits found.
u16 CoopQuest_CountBits(void)
{
    u16 var = OrVar(gSpecialVar_0x8004);
    u8 n = var ? CountBits(SharedBits(gSpecialVar_0x8004, VarGet(var))) : 0;

    ConvertIntToDecimalStringN(gStringVar1, n, STR_CONV_MODE_LEFT_ALIGN, 1);
    return n;
}

// VAR_0x8004: COOP_STORY_*. TRUE when this save knows the milestone, from
// its own play or the partner's ledger (even before it is applied here).
u16 CoopQuest_MilestoneKnown(void)
{
    struct CoopSave *save = Coop_GetSave();

    return save != NULL && gSpecialVar_0x8004 < COOP_STORY_COUNT
        && (save->storyDone & (1 << gSpecialVar_0x8004)) != 0;
}

// Milestones this save has from its own play (read back from its vars).
u16 CoopQuest_LocalMilestones(void)
{
    u16 done = 0;

    if (VarGet(VAR_SHOAL_QUEST) >= SHOAL_RESCUED)
        done |= 1 << COOP_STORY_SHOAL_RESCUED;
    if (VarGet(VAR_CARAVAN_QUEST) >= CARAVAN_MERCHANT)
        done |= 1 << COOP_STORY_CARAVAN_SUPPLIED;
    if (VarGet(VAR_COAST_QUEST) >= COAST_CLEAN)
        done |= 1 << COOP_STORY_COAST_CLEAN;
    if (FlagGet(FLAG_SHRINE_FOUND))
        done |= 1 << COOP_STORY_SHRINE_OPEN;
    if (FlagGet(FLAG_SHRINE_SOLVED))
        done |= 1 << COOP_STORY_SHRINE_SOLVED;
    return done;
}

// The partner's milestone, written into this save at a map change.
bool8 CoopQuest_ApplyMilestone(u8 id)
{
    switch (id)
    {
    case COOP_STORY_SHOAL_RESCUED:
        if (VarGet(VAR_SHOAL_QUEST) < SHOAL_RESCUED)
        {
            VarSet(VAR_SHOAL_QUEST, SHOAL_RESCUED);
            FlagSet(FLAG_SHOAL_CATCHUP);
        }
        FlagSet(FLAG_HIDE_SHOAL_ISLA_ICE);
        return TRUE;
    case COOP_STORY_CARAVAN_SUPPLIED:
        if (VarGet(VAR_CARAVAN_QUEST) < CARAVAN_MERCHANT)
        {
            VarSet(VAR_CARAVAN_QUEST, CARAVAN_MERCHANT);
            FlagSet(FLAG_CARAVAN_CATCHUP);
        }
        return TRUE;
    case COOP_STORY_COAST_CLEAN:
        if (VarGet(VAR_COAST_QUEST) < COAST_CLEAN)
        {
            VarSet(VAR_COAST_QUEST, COAST_CLEAN);
            FlagSet(FLAG_COAST_CATCHUP);
        }
        return TRUE;
    case COOP_STORY_SHRINE_OPEN:
        FlagSet(FLAG_SHRINE_FOUND);
        if (VarGet(VAR_SHRINE_QUEST) < SHRINE_OPENED)
            VarSet(VAR_SHRINE_QUEST, SHRINE_OPENED);
        return TRUE;
    case COOP_STORY_SHRINE_SOLVED:
        FlagSet(FLAG_SHRINE_FOUND);
        FlagSet(FLAG_SHRINE_GATE_PLATES);
        FlagSet(FLAG_SHRINE_GATE_LANTERNS);
        FlagSet(FLAG_SHRINE_SOLVED);
        if (VarGet(VAR_SHRINE_QUEST) < SHRINE_OPENED)
            VarSet(VAR_SHRINE_QUEST, SHRINE_OPENED);
        return TRUE;
    }
    return FALSE;
}

// ---------------------------------------------------------------- twins

// VAR_0x8004: a TWIN_ARC_* bit. VAR_RESULT: 0 new to this save, 1 the
// partner already played it (short version), 2 played here.
u16 CoopQuest_TwinState(void)
{
    u16 bit = gSpecialVar_0x8004;

    if (VarGet(VAR_TWIN_ARC) & bit)
        return 2;
    if (VarGet(VAR_TWIN_HEARD) & bit)
        return 1;
    return 0;
}

// VAR_0x8004: a TWIN_ARC_* bit this save just played.
u16 CoopQuest_TwinMark(void)
{
    u16 arc = VarGet(VAR_TWIN_ARC) | gSpecialVar_0x8004;

    VarSet(VAR_TWIN_ARC, arc);
    if (Coop_SessionActive() && (gSpecialVar_0x8004 & 0xFF))
        CoopDeal_SendQuest(COOP_QUEST_TWIN_HEARD, arc & 0xFF, TRUE);
    return 0;
}

// ---------------------------------------------------------------- shrine

// The twin plates in the shrine's hall, and where the idol boulder starts.
#define PLATE_A_X   5
#define PLATE_A_Y   25
#define PLATE_B_X   11
#define PLATE_B_Y   25

static bool8 InShrine(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_TWIN_BEACON_SHRINE)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_TWIN_BEACON_SHRINE);
}

static bool8 ObjectOrPartnerOn(s16 x, s16 y)
{
    u8 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent *obj = &gObjectEvents[i];

        if (!obj->active || obj->isPlayer)
            continue;
        if (obj->currentCoords.x - MAP_OFFSET == x && obj->currentCoords.y - MAP_OFFSET == y)
            return TRUE; // the idol boulder, or the partner's figure
    }
    return FALSE;
}

// VAR_0x8004: the plate the player stands on (1 west, 2 east). TRUE when
// the other plate is held down: the idol boulder (solo, Strength) or the
// partner standing there (co-op).
u16 CoopQuest_PlatesHeld(void)
{
    if (!InShrine())
        return FALSE;
    if (gSpecialVar_0x8004 == 1)
        return ObjectOrPartnerOn(PLATE_B_X, PLATE_B_Y);
    return ObjectOrPartnerOn(PLATE_A_X, PLATE_A_Y);
}


// VAR_0x8004: the lantern lit now (1 or 2). TRUE when the other lantern
// burned within the window: lit by this player (solo: run) or the partner.
u16 CoopQuest_LightLantern(void)
{
    u8 which = gSpecialVar_0x8004;
    u32 now = gMain.vblankCounter1;
    bool8 solved = FALSE;

    if (sMyLantern != 0 && sMyLantern != which && now - sMyLanternTime <= LANTERN_WINDOW)
        solved = TRUE;
    if (sPartnerLantern != 0 && sPartnerLantern != which && now - sPartnerLanternTime <= LANTERN_WINDOW)
        solved = TRUE;
    sMyLantern = which;
    sMyLanternTime = now;
    if (Coop_SessionActive())
        CoopDeal_SendQuest(COOP_QUEST_SHRINE_LANTERN, which, TRUE);
    return solved;
}

// ON_LOAD of the Scorched Slab: the passage to the shrine, once found.
u16 CoopQuest_DrawScorchedSlab(void)
{
    if (!FlagGet(FLAG_SHRINE_FOUND))
        return 0;
    MapGridSetMetatileIdAt(6 + MAP_OFFSET, 1 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_TopLeft | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(7 + MAP_OFFSET, 1 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_TopMid | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(8 + MAP_OFFSET, 1 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_TopRight | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(6 + MAP_OFFSET, 2 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_BottomLeft | MAPGRID_IMPASSABLE);
    MapGridSetMetatileIdAt(7 + MAP_OFFSET, 2 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_BottomMid);
    MapGridSetMetatileIdAt(8 + MAP_OFFSET, 2 + MAP_OFFSET, METATILE_Cave_SealedChamberEntrance_BottomRight | MAPGRID_IMPASSABLE);
    return 0;
}

// ---------------------------------------------------------------- coast

// Route 109 and Dewford after the cleanup: Skrelp, Clauncher and Lure-
// friendly rates for each rod (old 0-1, good 2-4, super 5-9).
static const struct WildPokemon sCleanCoastFishing[] = {
    { 5, 10, SPECIES_MAGIKARP },
    { 5, 10, SPECIES_SKRELP },
    { 10, 30, SPECIES_SKRELP },
    { 10, 30, SPECIES_CLAUNCHER },
    { 10, 30, SPECIES_WAILMER },
    { 25, 30, SPECIES_SKRELP },
    { 30, 35, SPECIES_CLAUNCHER },
    { 25, 30, SPECIES_LANTURN },
    { 35, 40, SPECIES_DRAGALGE },
    { 35, 40, SPECIES_SHARPEDO },
};
static const struct WildPokemonInfo sCleanCoastFishingInfo = { 30, sCleanCoastFishing };

const struct WildPokemonInfo *CoopQuest_FishingOverride(void)
{
    u8 group = gSaveBlock1Ptr->location.mapGroup;
    u8 num = gSaveBlock1Ptr->location.mapNum;

    if (VarGet(VAR_COAST_QUEST) < COAST_CLEAN)
        return NULL;
    if ((group == MAP_GROUP(MAP_ROUTE109) && num == MAP_NUM(MAP_ROUTE109))
     || (group == MAP_GROUP(MAP_DEWFORD_TOWN) && num == MAP_NUM(MAP_DEWFORD_TOWN)))
        return &sCleanCoastFishingInfo;
    return NULL;
}

// ---------------------------------------------------------------- caravan

// VAR_0x8005: the glyph touched (0 SUN, 1 STAR, 2 MOON). The order is SUN,
// STAR, MOON. VAR_RESULT: 0 wrong (start over), 1 right, 2 solved.
u16 CoopQuest_PressGlyph(void)
{
    u16 found = VarGet(VAR_CARAVAN_FOUND);
    u8 step = (found >> CARAVAN_GLYPH_SHIFT) & 3;

    found &= ~(3 << CARAVAN_GLYPH_SHIFT);
    if (gSpecialVar_0x8005 != step)
    {
        VarSet(VAR_CARAVAN_FOUND, found);
        return 0;
    }
    if (step == 2)
    {
        VarSet(VAR_CARAVAN_FOUND, found);
        return 2;
    }
    VarSet(VAR_CARAVAN_FOUND, found | ((step + 1) << CARAVAN_GLYPH_SHIFT));
    return 1;
}

// ---------------------------------------------------------------- twins

static u8 BadgeCount(void)
{
    u8 i, n = 0;

    for (i = 0; i < 8; i++)
        n += FlagGet(FLAG_BADGE01_GET + i);
    return n;
}

static bool8 TwinKnown(u16 bits)
{
    return ((VarGet(VAR_TWIN_ARC) | VarGet(VAR_TWIN_HEARD)) & bits) == bits;
}

static bool8 TwinDone(u16 bits)
{
    return (VarGet(VAR_TWIN_ARC) & bits) == bits;
}

// VAR_0x8004: TWIN_ARC_* bits. TRUE when this save played all of them.
u16 CoopQuest_TwinHas(void)
{
    return TwinDone(gSpecialVar_0x8004);
}

// Who stands where: each meeting shows from its badge count once the one
// before it is known (played here or by the partner), and stays until this
// save played it (and, for SOREN's spars, won and took the prize).
u16 CoopQuest_TwinUpdateFlags(void)
{
    u8 badges = BadgeCount();
    bool8 s1 = badges >= 2 && !(TwinDone(TWIN_ARC_S1 | TWIN_ARC_SPAR_1) && RewardClaimed(COOP_REWARD_TWIN_SPAR_1));
    bool8 s2 = badges >= 6 && TwinKnown(TWIN_ARC_S1) && !(TwinDone(TWIN_ARC_S2 | TWIN_ARC_SPAR_2) && RewardClaimed(COOP_REWARD_TWIN_SPAR_2));
    bool8 s3 = badges >= 7 && TwinKnown(TWIN_ARC_S2) && !TwinDone(TWIN_ARC_S3);
    bool8 m1 = badges >= 2 && !TwinDone(TWIN_ARC_M1);
    bool8 m2 = badges >= 4 && TwinKnown(TWIN_ARC_M1) && !TwinDone(TWIN_ARC_M2);
    bool8 m3 = badges >= 7 && TwinKnown(TWIN_ARC_M2) && !TwinDone(TWIN_ARC_M3);
    bool8 all = TwinKnown(TWIN_ARC_S1 | TWIN_ARC_S2 | TWIN_ARC_S3 | TWIN_ARC_M1 | TWIN_ARC_M2 | TWIN_ARC_M3);
    bool8 exh = all && !(TwinDone(TWIN_ARC_EXHIBITION) && RewardClaimed(COOP_REWARD_TWIN_SOREN) && RewardClaimed(COOP_REWARD_TWIN_MIRA));
    static const u16 sFlags[] = {
        FLAG_HIDE_TWIN_S1, FLAG_HIDE_TWIN_S2, FLAG_HIDE_TWIN_S3,
        FLAG_HIDE_TWIN_M1, FLAG_HIDE_TWIN_M2, FLAG_HIDE_TWIN_M3, FLAG_HIDE_TWIN_EXHIBITION,
    };
    bool8 show[] = { s1, s2, s3, m1, m2, m3, exh };
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sFlags); i++)
    {
        if (show[i])
            FlagClear(sFlags[i]);
        else
            FlagSet(sFlags[i]);
    }
    return 0;
}

// Before each Twin Trial round: this team, and the partner's if they are
// in the shrine (a co-op round uses both teams).
u16 CoopQuest_TrialHeal(void)
{
    HealPlayerParty();
    if (Coop_SessionActive())
        CoopDeal_SendQuest(COOP_QUEST_TRIAL_HEAL, 1, TRUE);
    return 0;
}

// A Twin Trial round alone: VAR_0x8004/5 the pair, as one double battle
// against both (the script waits and resumes after a win).
u16 CoopQuest_TrialSoloBattle(void)
{
    BattleSetup_StartSoloPairBattle(gSpecialVar_0x8004, gSpecialVar_0x8005);
    return 0;
}
