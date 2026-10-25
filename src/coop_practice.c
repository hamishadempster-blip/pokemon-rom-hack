// Practice battles (the COACH in several Pokemon Centers): an AI battle
// against a copy of your own team or of your linked partner's, singles or
// doubles, at your own levels or all at Lv. 50.
//
// Everything that fights is a copy. The real party and money are saved
// before the fight and put back after it, whatever the outcome; the bag is
// closed during the fight; no trainer flag, game stat or prize is given.

#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "battle.h"
#include "battle_setup.h"
#include "event_data.h"
#include "main.h"
#include "money.h"
#include "overworld.h"
#include "pokemon.h"
#include "script.h"
#include "string_util.h"
#include "task.h"
#include "constants/battle.h"
#include "constants/coop.h"
#include "constants/trainers.h"

#define PRACTICE_TEAM_WAIT (60 * 10)

static EWRAM_DATA struct Pokemon sSnap[PARTY_SIZE] = {0};
static EWRAM_DATA struct Pokemon sFoe[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sFoeCount = 0;
static EWRAM_DATA u32 sMoneySnap = 0;
static EWRAM_DATA bool8 sHasSnap = FALSE;
static EWRAM_DATA u8 sOutcome = 0;
static EWRAM_DATA u8 sRules = 0;    // bit 0 doubles, bit 1 Lv. 50

static const u8 sText_PracticeDefeat[] = _("That's practice done.\nYour POKéMON are as they were.");

static void ScaleAndHeal(struct Pokemon *mon, bool8 lv50)
{
    if (lv50)
    {
        u16 species = GetMonData(mon, MON_DATA_SPECIES);
        u32 exp = gExperienceTables[gSpeciesInfo[species].growthRate][50];

        SetMonData(mon, MON_DATA_EXP, &exp);
        CalculateMonStats(mon);
    }
    HealPokemon(mon);
}

static u8 CountFighters(struct Pokemon *party)
{
    u8 i, n = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&party[i], MON_DATA_IS_EGG))
            n++;
    }
    return n;
}

// Starts fetching the partner's team. VAR_RESULT: COOP_DEAL_OK or LOST.
u16 CoopPractice_AskPartnerTeam(void)
{
    return CoopDeal_RequestPartnerTeam();
}

static void Task_WaitPartnerTeam(u8 taskId)
{
    if (CoopDeal_PartnerTeamReady() != 0)
    {
        gSpecialVar_Result = TRUE;
    }
    else if (++gTasks[taskId].data[0] >= PRACTICE_TEAM_WAIT || !Coop_SessionActive())
    {
        gSpecialVar_Result = FALSE;
    }
    else
    {
        return;
    }
    DestroyTask(taskId);
    ScriptContext_Enable();
}

// `waitstate` follows. VAR_RESULT: TRUE once the whole team has arrived.
void CoopPractice_WaitPartnerTeam(void)
{
    CreateTask(Task_WaitPartnerTeam, 80);
}

// VAR_0x8004: 0 your team, 1 the partner's. VAR_0x8005: 1 doubles.
// VAR_0x8006: 1 all at Lv. 50. VAR_RESULT: 0 ready, 1 too few of yours,
// 2 too few on the other side.
u16 CoopPractice_Prepare(void)
{
    u8 i, need = gSpecialVar_0x8005 ? 2 : 1;

    sRules = (gSpecialVar_0x8005 ? 1 : 0) | (gSpecialVar_0x8006 ? 2 : 0);
    memset(sFoe, 0, sizeof(sFoe));
    sFoeCount = 0;
    if (CountFighters(gParties[B_TRAINER_PLAYER]) < need)
        return 1;
    if (gSpecialVar_0x8004 == 0)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
            if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG))
                sFoe[sFoeCount++] = *mon;
        }
    }
    else
    {
        u8 n = CoopDeal_PartnerTeamReady();

        for (i = 0; i < n; i++)
        {
            CoopDeal_CopyPartnerTeamMon(i, &sFoe[sFoeCount]);
            CalculateMonStats(&sFoe[sFoeCount]);
            sFoeCount++;
        }
    }
    if (sFoeCount < need)
        return 2;
    return 0;
}

static void RestoreReal(void)
{
    if (!sHasSnap)
        return;
    memcpy(gParties[B_TRAINER_PLAYER], sSnap, sizeof(sSnap));
    CalculatePlayerPartyCount();
    SetMoney(&gSaveBlock1Ptr->money, sMoneySnap);
    sHasSnap = FALSE;
}

static void CB2_EndPractice(void)
{
    sOutcome = gBattleOutcome;
    RestoreReal();
    gCoopPracticeBattle = FALSE;
    ZeroEnemyPartyMons();
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

// After CoopPractice_Prepare: the copies fight; `waitstate` follows.
void CoopPractice_Start(void)
{
    u8 i, n = 0;
    struct Pokemon team[PARTY_SIZE];

    memcpy(sSnap, gParties[B_TRAINER_PLAYER], sizeof(sSnap));
    sMoneySnap = GetMoney(&gSaveBlock1Ptr->money);
    sHasSnap = TRUE;
    memset(team, 0, sizeof(team));
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&sSnap[i], MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(&sSnap[i], MON_DATA_IS_EGG))
            continue;
        team[n] = sSnap[i];
        ScaleAndHeal(&team[n], sRules & 2);
        n++;
    }
    memcpy(gParties[B_TRAINER_PLAYER], team, sizeof(team));
    CalculatePlayerPartyCount();

    ZeroEnemyPartyMons();
    for (i = 0; i < sFoeCount; i++)
    {
        gParties[B_TRAINER_OPPONENT_A][i] = sFoe[i];
        ScaleAndHeal(&gParties[B_TRAINER_OPPONENT_A][i], sRules & 2);
    }
    CalculateEnemyPartyCount();

    gCoopPracticeBattle = TRUE;
    sOutcome = 0;
    memset(&gTrainerBattleParameter, 0, sizeof(gTrainerBattleParameter));
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_COOP_PRACTICE;
    TRAINER_BATTLE_PARAM.opponentB = TRAINER_NONE;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sText_PracticeDefeat;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    if (sRules & 1)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    gMain.savedCallback = CB2_EndPractice;
    BattleSetup_StartCoopFriendlyBattle();
    ScriptContext_Stop();
}

// VAR_RESULT: 1 won, 2 lost, 3 draw, 0 other.
u16 CoopPractice_Outcome(void)
{
    switch (sOutcome)
    {
    case B_OUTCOME_WON:
        return 1;
    case B_OUTCOME_LOST:
        return 2;
    case B_OUTCOME_DREW:
        return 3;
    default:
        return 0;
    }
}
