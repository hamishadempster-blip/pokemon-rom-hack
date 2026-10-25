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
#include "fieldmap.h"
#include "field_camera.h"
#include "link.h"
#include "main.h"
#include "map_name_popup.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "string_util.h"
#include "constants/coop.h"
#include "constants/flags.h"
#include "constants/maps.h"
#include "constants/opponents.h"
#include "constants/vars.h"

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

static EWRAM_DATA bool8 sReportPending = FALSE;
static EWRAM_DATA bool8 sRedraw = FALSE;
static EWRAM_DATA u8 sPartnerNote = 0; // 1 + breakers, or 0xFF for a trip

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

void CoopQuest_HandleSync(u8 quest, u8 value, bool8 live)
{
    u8 now, next;

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

// Every frame while linked (src/coop_deal.c).
void CoopQuest_Poll(void)
{
    if (sReportPending)
    {
        sReportPending = FALSE;
        if (!PowerIsOff() && Breakers() != 0)
            CoopDeal_SendQuest(COOP_QUEST_PP_BREAKERS, Breakers(), FALSE);
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

// gStringVar4: this save's quests, a page each step. VAR_RESULT: unused.
u16 CoopQuest_BufferJournal(void)
{
    u8 *end = gStringVar4;
    bool8 first = TRUE;
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
    return 0;
}
