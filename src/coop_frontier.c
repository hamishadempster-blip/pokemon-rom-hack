// Duo Frontier (build 0xC017013E): Battle Frontier challenges for two
// linked players.
//
// DUO TOWER is the first facility. Two linked players at the DUO DESK in the
// Battle Tower lobby agree on a bracket (LV. 50 or OPEN LEVEL), each choose
// three Pokemon (team preparation, src/coop_prep.c), and fight a streak of
// co-op multi battles against generated pairs of Battle Frontier trainers:
// one battle at a time, each one asked and agreed again (the ready check).
//
// What both games share for a run, and how they agree on it:
// - the run id (picked by whoever starts the run), the bracket and the
//   battle number travel in every duo invite; the opponents and their teams
//   are generated on each game from those alone (a fixed generator, seeded
//   from the run id and battle number, with the global RNG put back after),
//   so both games build the same battle;
// - each save keeps its own copy of the run: partner (trainer id), battles
//   won, battles already paid, its own team (personalities). A save that
//   lost power before saving a win is at an earlier battle: the run goes on
//   from the earlier of the two (a win is never credited twice or without
//   both games having it, and BP already paid are never paid again).
//
// What never changes: the real party, levels, EXP, items and money. Each
// battle is fought by healed copies of the three chosen Pokemon (scaled to
// Lv. 50 in that bracket); the real party comes back after the battle
// whatever happened (win, loss, link failure). No EXP, no prize money, no
// bag in battle, no trainer flags, no catches. BP and the duo records are
// personal and paid once, per save.

#include "global.h"
#include "battle.h"
#include "battle_frontier.h"
#include "battle_setup.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_frontier.h"
#include "data.h"
#include "event_data.h"
#include "frontier_util.h"
#include "item.h"
#include "link.h"
#include "main.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "string_util.h"
#include "constants/battle_frontier.h"
#include "constants/characters.h"
#include "constants/battle_frontier_mons.h"
#include "constants/battle_frontier_trainers.h"
#include "constants/coop.h"
#include "constants/items.h"
#include "constants/opponents.h"

#define DUO_TEAM 3
#define DUO_MODE(word)     ((u8)((word) & 0xFF))
#define DUO_FACILITY(word) ((u8)((word) >> 8))
#define DUO_FLAG_NEW_RUN   1
#define DUO_OPEN_MIN_LEVEL 60

EWRAM_DATA bool8 gCoopDuoBattle = FALSE;
EWRAM_DATA u16 gCoopDuoTrainerIds[2] = {0};
EWRAM_DATA u16 gCoopDuoMonIds[6] = {0};
EWRAM_DATA u8 gCoopDuoLastOutcome = 0;
EWRAM_DATA u8 gCoopDuoLastBp = 0;

struct DuoBattle
{
    bool8 ready;     // trainers and teams generated for this battle
    u8 mode;
    u8 facility;
    u8 flags;
    u8 oppLevel;
    u16 runId;
    u16 battle;      // 1-based battle number
    u16 trainers[2];
    u16 mons[2][DUO_TEAM];
};

static EWRAM_DATA struct DuoBattle sDuo = {0};
static EWRAM_DATA struct Trainer sDuoTrainer[2] = {0};
static EWRAM_DATA struct Pokemon sDuoReal[PARTY_SIZE] = {0};
static EWRAM_DATA bool8 sDuoRealSaved = FALSE;
// The invite this game is about to send from the desk.
static EWRAM_DATA u16 sOutRunId = 0;
static EWRAM_DATA u16 sOutModeWord = 0;
static EWRAM_DATA u16 sOutBattle = 0;
static EWRAM_DATA u8 sOutFlags = 0;
// The duo invite a team preparation belongs to (src/coop_prep.c).
static EWRAM_DATA bool8 sPrepDuo = FALSE;
static EWRAM_DATA u16 sPrepRunId = 0;
static EWRAM_DATA u16 sPrepModeWord = 0;
static EWRAM_DATA u16 sPrepBattle = 0;
static EWRAM_DATA u8 sPrepFlags = 0;
// The last refusal from the partner, for the desk's message.
static EWRAM_DATA u16 sRefusedAux = 0;

static const u8 sText_DuoDefeat[] = _("Your teamwork wins the day!");

// The trainer pool by set of seven battles, as the stock Battle Tower.
static const u16 sDuoRanges[8][2] =
{
    {FRONTIER_TRAINER_BRADY,   FRONTIER_TRAINER_JILL},
    {FRONTIER_TRAINER_TREVIN,  FRONTIER_TRAINER_CHLOE},
    {FRONTIER_TRAINER_ERIK,    FRONTIER_TRAINER_SOFIA},
    {FRONTIER_TRAINER_NORTON,  FRONTIER_TRAINER_JAZLYN},
    {FRONTIER_TRAINER_BRADEN,  FRONTIER_TRAINER_ALISON},
    {FRONTIER_TRAINER_ZACHERY, FRONTIER_TRAINER_LAMAR},
    {FRONTIER_TRAINER_HANK,    FRONTIER_TRAINER_TESS},
    {FRONTIER_TRAINER_JAXON,   FRONTIER_TRAINER_GRETEL},
};

// Curated milestone pairs (battles 21 and 49): fixed top-tier trainers
// with perfect IVs. They are ordinary Battle Frontier trainers, not a
// Frontier Brain.
static const u16 sDuoMilestonePairs[2][2] =
{
    {FRONTIER_TRAINER_PAIGE, FRONTIER_TRAINER_ANYA},
    {FRONTIER_TRAINER_ABBY,  FRONTIER_TRAINER_GRETEL},
};

static struct CoopSave *Duo_Save(void)
{
    return Coop_GetSave();
}

static u32 Duo_PartnerOt(void)
{
    if (!Coop_SessionActive())
        return 0;
    return gLinkPlayers[GetMultiplayerId() ^ 1].trainerId;
}

static bool8 Duo_IsMilestone(u16 battle)
{
    return battle == COOP_DUO_MILESTONE_1 || battle == COOP_DUO_MILESTONE_2;
}

static u32 Duo_Seed(u16 runId, u16 battle, u8 salt)
{
    return (runId * 0x9E3779B1u) ^ (battle * 0x85EBCA77u) ^ (salt * 0xC2B2AE3Du) ^ 0x0D0E7105u;
}

// ---------------------------------------------------------------------------
// Opponents

static void Duo_PickPair(u16 runId, u16 battle, const u16 *used, u8 nUsed, u16 *out)
{
    rng_value_t rng;
    u16 set, lo, hi;
    u32 k, tries, j;

    if (Duo_IsMilestone(battle))
    {
        const u16 *pair = sDuoMilestonePairs[battle == COOP_DUO_MILESTONE_1 ? 0 : 1];

        out[0] = pair[0];
        out[1] = pair[1];
        return;
    }
    set = (battle - 1) / COOP_DUO_SET_LENGTH;
    if (set > 7)
        set = 7;
    lo = sDuoRanges[set][0];
    hi = sDuoRanges[set][1];
    rng = LocalRandomSeed(Duo_Seed(runId, battle, 1));
    for (k = 0; k < 2; k++)
    {
        u16 id = lo;

        for (tries = 0; tries < 100; tries++)
        {
            id = lo + LocalRandom(&rng) % (hi - lo + 1);
            for (j = 0; j < nUsed; j++)
            {
                if (used[j] == id)
                    break;
            }
            if (j == nUsed && (k == 0 || id != out[0]))
                break;
        }
        out[k] = id;
    }
}

// No trainer appears twice within a set of seven: earlier battles of the
// set are generated again (cheaply) to know who was met.
static void Duo_Trainers(u16 runId, u16 battle, u16 *out)
{
    u16 used[2 * COOP_DUO_SET_LENGTH];
    u16 pair[2];
    u16 first = ((battle - 1) / COOP_DUO_SET_LENGTH) * COOP_DUO_SET_LENGTH + 1;
    u8 n = 0;
    u16 b;

    for (b = first; b <= battle; b++)
    {
        Duo_PickPair(runId, b, used, n, pair);
        if (b < battle && n + 2 <= ARRAY_COUNT(used))
        {
            used[n++] = pair[0];
            used[n++] = pair[1];
        }
    }
    out[0] = pair[0];
    out[1] = pair[1];
}

static u8 Duo_MonSetCount(const u16 *monSet)
{
    u8 n = 0;

    while (monSet[n] != 0xFFFF && n < 0xFF)
        n++;
    return n;
}

// Three Pokemon per trainer from its Battle Frontier set: no species twice
// in the pair's six, no item twice within one trainer's three (the stock
// rule), and no "high tier" Pokemon in LV. 50.
static void Duo_PickMons(void)
{
    rng_value_t rng = LocalRandomSeed(Duo_Seed(sDuo.runId, sDuo.battle, 2));
    u32 side, i, j, tries;

    for (side = 0; side < 2; side++)
    {
        const u16 *monSet = gBattleFrontierTrainers[sDuo.trainers[side]].monSet;
        u8 count = Duo_MonSetCount(monSet);

        i = 0;
        for (tries = 0; tries < 600 && i < DUO_TEAM && count != 0; tries++)
        {
            u16 monId = monSet[LocalRandom(&rng) % count];
            bool8 relax = tries >= 400;   // a tiny set: allow repeats across the pair
            u16 species = gBattleFrontierMons[monId].species;
            u16 item = gBattleFrontierMons[monId].heldItem;
            bool8 ok = TRUE;

            if (sDuo.mode == COOP_DUO_LV50 && monId > FRONTIER_MONS_HIGH_TIER && !relax)
                continue;
            for (j = 0; j < side * DUO_TEAM + i && ok; j++)
            {
                u16 other = sDuo.mons[j / DUO_TEAM][j % DUO_TEAM];

                if (gBattleFrontierMons[other].species == species && (!relax || j / DUO_TEAM == side))
                    ok = FALSE;
                if (j / DUO_TEAM == side && item != ITEM_NONE && gBattleFrontierMons[other].heldItem == item)
                    ok = FALSE;
            }
            if (!ok)
                continue;
            sDuo.mons[side][i++] = monId;
        }
        // (Every Battle Frontier set has well over three choices.)
        for (; i < DUO_TEAM; i++)
            sDuo.mons[side][i] = monSet[i % (count ? count : 1)];
    }
}

static void Duo_SetupTrainers(void)
{
    u32 side;

    Duo_Trainers(sDuo.runId, sDuo.battle, sDuo.trainers);
    Duo_PickMons();
    for (side = 0; side < 2; side++)
    {
        const struct BattleFrontierTrainer *ft = &gBattleFrontierTrainers[sDuo.trainers[side]];
        struct Trainer *t = &sDuoTrainer[side];

        *t = gTrainers[DIFFICULTY_NORMAL][TRAINER_DUO_A + side];
        t->trainerClass = gFacilityClassToTrainerClass[ft->facilityClass];
        t->trainerPic = gFacilityClassToPicIndex[ft->facilityClass];
        memset(t->trainerName, EOS, sizeof(t->trainerName));
        StringCopyN(t->trainerName, ft->trainerName, PLAYER_NAME_LENGTH);
        t->trainerName[PLAYER_NAME_LENGTH] = EOS;
        t->partySize = DUO_TEAM;
        gCoopDuoTrainerIds[side] = sDuo.trainers[side];
        gCoopDuoMonIds[side * DUO_TEAM + 0] = sDuo.mons[side][0];
        gCoopDuoMonIds[side * DUO_TEAM + 1] = sDuo.mons[side][1];
        gCoopDuoMonIds[side * DUO_TEAM + 2] = sDuo.mons[side][2];
    }
    sDuo.ready = TRUE;
}

bool8 CoopDuo_InBattle(void)
{
    return gCoopDuoBattle;
}

const struct Trainer *CoopDuo_TrainerStruct(u16 trainerId)
{
    if (!sDuo.ready || (trainerId != TRAINER_DUO_A && trainerId != TRAINER_DUO_B))
        return NULL;
    return &sDuoTrainer[trainerId == TRAINER_DUO_B ? 1 : 0];
}

bool8 CoopDuo_CreateParty(struct Pokemon *party, u16 trainerId)
{
    rng_value_t saved;
    u8 side, level, fixedIV;
    u32 i, otId;

    if (!sDuo.ready || (trainerId != TRAINER_DUO_A && trainerId != TRAINER_DUO_B))
        return FALSE;
    side = trainerId == TRAINER_DUO_B ? 1 : 0;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    level = sDuo.mode == COOP_DUO_LV50 ? FRONTIER_MAX_LEVEL_50 : sDuo.oppLevel;
    fixedIV = Duo_IsMilestone(sDuo.battle) ? MAX_PER_STAT_IVS : GetFrontierTrainerFixedIvs(sDuo.trainers[side]);
    // CreateFacilityMon draws personality and ball from the global RNG:
    // seed it from the run so both games make the same Pokemon, then put
    // the game's own sequence back.
    saved = gRngValue;
    gRngValue = LocalRandomSeed(Duo_Seed(sDuo.runId, sDuo.battle, 3 + side));
    otId = Random32();
    for (i = 0; i < DUO_TEAM; i++)
        CreateFacilityMon(&gBattleFrontierMons[sDuo.mons[side][i]], level, fixedIV, otId, 0, &party[i]);
    gRngValue = saved;
    return TRUE;
}

// ---------------------------------------------------------------------------
// The run on this save

static u16 Duo_BestFor(const struct CoopSave *save, u8 mode)
{
    return mode == COOP_DUO_OPEN ? save->duoBestOpen[FRONTIER_FACILITY_TOWER] : save->duoBest[FRONTIER_FACILITY_TOWER];
}

static void Duo_NoteBest(struct CoopSave *save)
{
    u16 *best = save->duoMode == COOP_DUO_OPEN ? &save->duoBestOpen[FRONTIER_FACILITY_TOWER]
                                               : &save->duoBest[FRONTIER_FACILITY_TOWER];

    if (save->duoRound > *best)
        *best = save->duoRound;
}

static bool8 Duo_RunActive(const struct CoopSave *save)
{
    return save != NULL && save->duoState == COOP_DUO_STATE_ACTIVE && save->duoRunId != 0;
}

static void Duo_EndRun(struct CoopSave *save)
{
    Duo_NoteBest(save);
    save->duoState = COOP_DUO_STATE_NONE;
    save->duoRunId = 0;
    save->duoRound = 0;
    save->duoPaid = 0;
    memset(save->duoTeamPid, 0, sizeof(save->duoTeamPid));
}

// BP for the set of seven just completed (set 1 = battles 1-7), plus the
// milestone bonus. Duo BP are ordinary Battle Points, paid once per save.
static u16 Duo_SetBp(u16 battlesWon)
{
    u16 set = battlesWon / COOP_DUO_SET_LENGTH;
    u16 bp = 2 + set;

    if (bp > 10)
        bp = 10;
    if (battlesWon == COOP_DUO_MILESTONE_1)
        bp += 5;
    if (battlesWon == COOP_DUO_MILESTONE_2)
        bp += 10;
    return bp;
}

static void Duo_AddBp(u16 bp)
{
    u32 total = gSaveBlock2Ptr->frontier.battlePoints + bp;
    u32 card = gSaveBlock2Ptr->frontier.cardBattlePoints + bp;

    gSaveBlock2Ptr->frontier.battlePoints = total > MAX_BATTLE_FRONTIER_POINTS ? MAX_BATTLE_FRONTIER_POINTS : total;
    gSaveBlock2Ptr->frontier.cardBattlePoints = card > 0xFFFF ? 0xFFFF : card;
}

// ---------------------------------------------------------------------------
// Invites

u16 CoopDuo_InviteRunId(void)
{
    return sOutRunId;
}

u16 CoopDuo_InviteModeWord(void)
{
    return sOutModeWord;
}

u16 CoopDuo_InviteBattle(void)
{
    return sOutBattle;
}

u8 CoopDuo_InviteFlags(void)
{
    return sOutFlags;
}

static u8 Duo_CountAllowed(void)
{
    u32 i;
    u8 n = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (CoopDuo_MonAllowed(&gParties[B_TRAINER_PLAYER][i]))
            n++;
    }
    return n;
}

u8 CoopDuo_GuestCheck(u16 runId, u16 modeWord, u16 battle, u8 flags, u16 *aux)
{
    struct CoopSave *save = Duo_Save();
    u32 partner = Duo_PartnerOt();

    *aux = 0;
    if (save == NULL || runId == 0 || battle == 0 || DUO_MODE(modeWord) > COOP_DUO_OPEN
     || DUO_FACILITY(modeWord) != FRONTIER_FACILITY_TOWER)
        return COOP_DUO_NACK_NO_RUN;
    if (Duo_CountAllowed() < DUO_TEAM)
        return COOP_DUO_NACK_TEAM;
    // A run with someone else stays until this player retires it.
    if (Duo_RunActive(save) && save->duoPartnerOtId != partner)
        return COOP_DUO_NACK_OTHER_RUN;
    if (flags & DUO_FLAG_NEW_RUN)
        return 0;
    if (!Duo_RunActive(save) || save->duoRunId != runId)
        return COOP_DUO_NACK_NO_RUN;
    // Continue from the earlier of the two saves. This save behind: the
    // inviter goes back to this save's battle (and asks again).
    if (save->duoRound + 1 < battle)
    {
        *aux = save->duoRound;
        return COOP_DUO_NACK_BEHIND;
    }
    return 0;
}

void CoopDuo_OnPartnerRefused(u8 reason, u16 aux)
{
    struct CoopSave *save = Duo_Save();

    sRefusedAux = aux;
    if (reason == COOP_DUO_NACK_BEHIND && Duo_RunActive(save) && aux < save->duoRound)
        save->duoRound = aux;   // BP already paid stay paid (duoPaid)
}

void CoopDuo_BeginBattle(u16 runId, u16 modeWord, u16 battle, u8 flags)
{
    memset(&sDuo, 0, sizeof(sDuo));
    sDuo.runId = runId;
    sDuo.mode = DUO_MODE(modeWord);
    sDuo.facility = DUO_FACILITY(modeWord);
    sDuo.battle = battle;
    sDuo.flags = flags;
    gCoopDuoBattle = TRUE;
    gCoopDuoLastOutcome = COOP_DUO_OUT_NONE;
    gCoopDuoLastBp = 0;
    memset(&gTrainerBattleParameter, 0, sizeof(gTrainerBattleParameter));
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_DUO_A;
    TRAINER_BATTLE_PARAM.opponentB = TRAINER_DUO_B;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sText_DuoDefeat;
    TRAINER_BATTLE_PARAM.defeatTextB = (u8 *)sText_DuoDefeat;
}

// At the launch, on both games: the real party is put aside and the three
// chosen Pokemon fight as healed copies (Lv. 50 where the bracket says so).
// The run record is written now: a new run starts, a save ahead of the
// partner goes back to the agreed battle.
void CoopDuo_BuildBattleParty(const u8 *order, u8 count, u8 partnerMaxLevel)
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    struct CoopSave *save = Duo_Save();
    u8 ownMax = 0;
    u32 i;

    if (!gCoopDuoBattle)
        return;
    memcpy(sDuoReal, party, sizeof(sDuoReal));
    sDuoRealSaved = TRUE;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    for (i = 0; i < count && i < DUO_TEAM; i++)
    {
        struct Pokemon *mon = &party[i];
        u8 level;

        *mon = sDuoReal[order[i]];
        level = GetMonData(mon, MON_DATA_LEVEL);
        if (level > ownMax)
            ownMax = level;
        if (sDuo.mode == COOP_DUO_LV50 && level > FRONTIER_MAX_LEVEL_50)
        {
            u16 species = GetMonData(mon, MON_DATA_SPECIES);
            u32 exp = gExperienceTables[gSpeciesInfo[species].growthRate][FRONTIER_MAX_LEVEL_50];

            SetMonData(mon, MON_DATA_EXP, &exp);
            CalculateMonStats(mon);
        }
        HealPokemon(mon);
    }
    CalculatePlayerPartyCount();

    if (sDuo.mode == COOP_DUO_LV50)
        sDuo.oppLevel = FRONTIER_MAX_LEVEL_50;
    else
    {
        u8 lv = ownMax > partnerMaxLevel ? ownMax : partnerMaxLevel;
        sDuo.oppLevel = lv < DUO_OPEN_MIN_LEVEL ? DUO_OPEN_MIN_LEVEL : lv;
    }

    if (save != NULL)
    {
        if ((sDuo.flags & DUO_FLAG_NEW_RUN) || !Duo_RunActive(save) || save->duoRunId != sDuo.runId)
        {
            if (Duo_RunActive(save))
                Duo_EndRun(save);
            save->duoRunId = sDuo.runId;
            save->duoState = COOP_DUO_STATE_ACTIVE;
            save->duoFacility = sDuo.facility;
            save->duoMode = sDuo.mode;
            save->duoRound = sDuo.battle - 1;
            save->duoPaid = sDuo.battle - 1;
            save->duoPartnerOtId = Duo_PartnerOt();
        }
        else if (save->duoRound >= sDuo.battle)
        {
            // The partner's save is behind: both go on from its battle.
            save->duoRound = sDuo.battle - 1;
        }
        for (i = 0; i < DUO_TEAM; i++)
            save->duoTeamPid[i] = i < count ? GetMonData(&sDuoReal[order[i]], MON_DATA_PERSONALITY) : 0;
        save->duoOppLevel = sDuo.oppLevel;
    }
    Duo_SetupTrainers();
}

bool8 CoopDuo_RestoreRealParty(void)
{
    if (!sDuoRealSaved)
        return FALSE;
    memcpy(gParties[B_TRAINER_PLAYER], sDuoReal, sizeof(sDuoReal));
    CalculatePlayerPartyCount();
    sDuoRealSaved = FALSE;
    return TRUE;
}

// Both games, after the battle: the result goes on this save's run.
void CoopDuo_FinishBattle(u8 outcome)
{
    struct CoopSave *save = Duo_Save();

    CoopDuo_RestoreRealParty();
    gCoopDuoBattle = FALSE;
    sDuo.ready = FALSE;
    gCoopDuoLastOutcome = outcome;
    gCoopDuoLastBp = 0;
    if (save == NULL || !Duo_RunActive(save) || save->duoRunId != sDuo.runId)
        return;
    if (outcome == COOP_DUO_OUT_WON)
    {
        if (save->duoRound + 1 == sDuo.battle)
            save->duoRound = sDuo.battle;
        Duo_NoteBest(save);
        if (save->duoRound % COOP_DUO_SET_LENGTH == 0 && save->duoPaid < save->duoRound)
        {
            u16 bp = Duo_SetBp(save->duoRound);

            Duo_AddBp(bp);
            gCoopDuoLastBp = bp;
            save->duoPaid = save->duoRound;
        }
        if (save->duoRound == COOP_DUO_MILESTONE_1)
            save->duoSymbols |= 1;
        if (save->duoRound == COOP_DUO_MILESTONE_2)
            save->duoSymbols |= 2;
    }
    else if (outcome == COOP_DUO_OUT_LOST)
    {
        // The streak ends here (its length stays as the record).
        sDuo.battle = save->duoRound;   // for the message
        Duo_EndRun(save);
    }
}

void CoopDuo_AbortBattle(void)
{
    CoopDuo_RestoreRealParty();
    if (gCoopDuoBattle)
        gCoopDuoLastOutcome = COOP_DUO_OUT_ABORTED;
    gCoopDuoBattle = FALSE;
    sDuo.ready = FALSE;
}

// ---------------------------------------------------------------------------
// Team preparation rules

bool8 CoopDuo_MonAllowed(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES);

    if (species == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    return !gSpeciesInfo[GET_BASE_SPECIES_ID(species)].isFrontierBanned;
}

void CoopDuo_SetPrepContext(bool8 duo, u16 runId, u16 modeWord, u16 battle, u8 flags)
{
    sPrepDuo = duo;
    sPrepRunId = runId;
    sPrepModeWord = modeWord;
    sPrepBattle = battle;
    sPrepFlags = flags;
}

bool8 CoopDuo_PrepActive(void)
{
    return sPrepDuo;
}

// The run's team is fixed once its first battle has started.
bool8 CoopDuo_PrepTeamFixed(void)
{
    struct CoopSave *save = Duo_Save();

    return sPrepDuo && !(sPrepFlags & DUO_FLAG_NEW_RUN)
        && Duo_RunActive(save) && save->duoRunId == sPrepRunId && save->duoTeamPid[0] != 0;
}

bool8 CoopDuo_FixedTeamSlots(u8 *order)
{
    struct CoopSave *save = Duo_Save();
    u32 i, slot;

    if (save == NULL)
        return FALSE;
    for (i = 0; i < DUO_TEAM; i++)
    {
        for (slot = 0; slot < PARTY_SIZE; slot++)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];

            if (CoopDuo_MonAllowed(mon) && GetMonData(mon, MON_DATA_PERSONALITY) == save->duoTeamPid[i])
                break;
        }
        if (slot == PARTY_SIZE)
            return FALSE;
        order[i] = slot;
    }
    return TRUE;
}

u8 CoopDuo_CheckOwnTeam(const u8 *order, u8 count)
{
    u32 i, j;

    if (count != DUO_TEAM)
        return 1;
    for (i = 0; i < count; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][order[i]];

        if (!CoopDuo_MonAllowed(mon))
            return 2;
        for (j = 0; j < i; j++)
        {
            struct Pokemon *other = &gParties[B_TRAINER_PLAYER][order[j]];
            u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);

            if (GetMonData(other, MON_DATA_SPECIES) == GetMonData(mon, MON_DATA_SPECIES))
                return 3;
            if (item != ITEM_NONE && GetMonData(other, MON_DATA_HELD_ITEM) == item)
                return 4;
        }
    }
    return 0;
}

// The pair's clauses: no species twice and no held item twice among the
// six Pokemon of both teams.
u8 CoopDuo_CheckCombined(const u16 *mySpecies, const u16 *myItems, const u16 *theirSpecies, const u16 *theirItems)
{
    u32 i, j;

    for (i = 0; i < DUO_TEAM; i++)
    {
        for (j = 0; j < DUO_TEAM; j++)
        {
            if (mySpecies[i] != SPECIES_NONE && mySpecies[i] == theirSpecies[j])
            {
                StringCopy(gStringVar1, GetSpeciesName(mySpecies[i]));
                return 1;
            }
            if (myItems[i] != ITEM_NONE && myItems[i] == theirItems[j])
            {
                CopyItemName(myItems[i], gStringVar1);
                return 2;
            }
        }
    }
    return 0;
}

static const u8 *Duo_ModeName(u8 mode)
{
    return mode == COOP_DUO_OPEN ? COMPOUND_STRING("OPEN LEVEL") : COMPOUND_STRING("LV. 50");
}

void CoopDuo_BufferPrepTitle(u8 *dst)
{
    dst = StringCopy(dst, COMPOUND_STRING("DUO TOWER "));
    dst = StringCopy(dst, Duo_ModeName(DUO_MODE(sPrepModeWord)));
    dst = StringCopy(dst, COMPOUND_STRING(", battle "));
    ConvertIntToDecimalStringN(dst, sPrepBattle, STR_CONV_MODE_LEFT_ALIGN, 4);
}

// The partner's question: STR_VAR_1 the inviter, STR_VAR_2 the battle.
void CoopDuo_BufferAskText(u16 battle, u16 modeWord, u8 flags)
{
    u8 *end;

    StringCopyN(gStringVar1, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
    gStringVar1[PLAYER_NAME_LENGTH] = EOS;
    end = StringCopy(gStringVar2, (flags & DUO_FLAG_NEW_RUN) ? COMPOUND_STRING("a new DUO TOWER run (") : COMPOUND_STRING("DUO TOWER battle "));
    if (flags & DUO_FLAG_NEW_RUN)
    {
        end = StringCopy(end, Duo_ModeName(DUO_MODE(modeWord)));
        StringCopy(end, COMPOUND_STRING(")"));
    }
    else
    {
        end = ConvertIntToDecimalStringN(end, battle, STR_CONV_MODE_LEFT_ALIGN, 4);
        end = StringCopy(end, COMPOUND_STRING(" ("));
        end = StringCopy(end, Duo_ModeName(DUO_MODE(modeWord)));
        StringCopy(end, COMPOUND_STRING(")"));
    }
}

// ---------------------------------------------------------------------------
// The DUO DESK (data/scripts/coop_frontier.inc)

// VAR_RESULT: COOP_DUO_DESK_*.
u16 CoopDuo_DeskCheck(void)
{
    struct CoopSave *save = Duo_Save();

    if (!Coop_SessionActive() || !Coop_PartnerOnSameMap())
        return COOP_DUO_DESK_NOT_LINKED;
    if (Duo_RunActive(save) && save->duoPartnerOtId != Duo_PartnerOt())
        return COOP_DUO_DESK_OTHER_RUN;
    if (Duo_CountAllowed() < DUO_TEAM)
        return COOP_DUO_DESK_TEAM;
    return COOP_DUO_DESK_OK;
}

// VAR_RESULT: 0 no run, 1 a run (STR_VAR_1 the next battle, STR_VAR_2 the
// bracket, STR_VAR_3 the partner's name when linked to them).
u16 CoopDuo_BufferStatus(void)
{
    struct CoopSave *save = Duo_Save();

    if (!Duo_RunActive(save))
        return 0;
    ConvertIntToDecimalStringN(gStringVar1, save->duoRound + 1, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringCopy(gStringVar2, Duo_ModeName(save->duoMode));
    if (Coop_SessionActive() && save->duoPartnerOtId == Duo_PartnerOt())
    {
        StringCopyN(gStringVar3, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
        gStringVar3[PLAYER_NAME_LENGTH] = EOS;
    }
    else
    {
        StringCopy(gStringVar3, COMPOUND_STRING("another partner"));
    }
    return 1;
}

// VAR_0x8004: COOP_DUO_LV50 / OPEN for a new run, 0xFF to continue.
// Sets up the invite Coop_StartDuoBattle sends. VAR_RESULT: TRUE if there
// is something to send.
u16 CoopDuo_PrepareInvite(void)
{
    struct CoopSave *save = Duo_Save();
    u16 mode = gSpecialVar_0x8004;

    // A new attempt: the last battle's result must not be read as this
    // one's when the invite fails before any battle (refused, busy).
    gCoopDuoLastOutcome = COOP_DUO_OUT_NONE;
    gCoopDuoLastBp = 0;
    if (save == NULL)
        return FALSE;
    if (mode == 0xFF)
    {
        if (!Duo_RunActive(save))
            return FALSE;
        sOutRunId = save->duoRunId;
        sOutModeWord = save->duoMode | (FRONTIER_FACILITY_TOWER << 8);
        sOutBattle = save->duoRound + 1;
        sOutFlags = 0;
        return TRUE;
    }
    if (mode > COOP_DUO_OPEN)
        return FALSE;
    do
    {
        sOutRunId = Random();
    } while (sOutRunId == 0 || (Duo_RunActive(save) && sOutRunId == save->duoRunId));
    sOutModeWord = mode | (FRONTIER_FACILITY_TOWER << 8);
    sOutBattle = 1;
    sOutFlags = DUO_FLAG_NEW_RUN;
    return TRUE;
}

// After a duo battle (both games). VAR_RESULT: COOP_DUO_OUT_*.
// STR_VAR_1 battles won in the run (or the final streak), STR_VAR_2 BP
// earned now (0: none), STR_VAR_3 this save's BP.
u16 CoopDuo_LastOutcome(void)
{
    struct CoopSave *save = Duo_Save();
    u16 won = Duo_RunActive(save) ? save->duoRound : sDuo.battle;

    ConvertIntToDecimalStringN(gStringVar1, won, STR_CONV_MODE_LEFT_ALIGN, 4);
    ConvertIntToDecimalStringN(gStringVar2, gCoopDuoLastBp, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar3, gSaveBlock2Ptr->frontier.battlePoints, STR_CONV_MODE_LEFT_ALIGN, 4);
    gSpecialVar_0x8005 = gCoopDuoLastBp;
    gSpecialVar_0x8006 = won;
    return gCoopDuoLastOutcome;
}

// The partner refused the invite: STR_VAR_2 their battles won + 1.
u16 CoopDuo_BufferRefusal(void)
{
    ConvertIntToDecimalStringN(gStringVar2, sRefusedAux + 1, STR_CONV_MODE_LEFT_ALIGN, 4);
    return 0;
}

u16 CoopDuo_Retire(void)
{
    struct CoopSave *save = Duo_Save();

    if (!Duo_RunActive(save))
        return FALSE;
    ConvertIntToDecimalStringN(gStringVar1, save->duoRound, STR_CONV_MODE_LEFT_ALIGN, 4);
    Duo_EndRun(save);
    return TRUE;
}

// STR_VAR_1 best LV. 50 streak, STR_VAR_2 best OPEN LEVEL streak.
// VAR_RESULT: the duo milestone bits.
u16 CoopDuo_BufferRecords(void)
{
    struct CoopSave *save = Duo_Save();

    if (save == NULL)
        return 0;
    ConvertIntToDecimalStringN(gStringVar1, Duo_BestFor(save, COOP_DUO_LV50), STR_CONV_MODE_LEFT_ALIGN, 4);
    ConvertIntToDecimalStringN(gStringVar2, Duo_BestFor(save, COOP_DUO_OPEN), STR_CONV_MODE_LEFT_ALIGN, 4);
    return save->duoSymbols;
}

// Saves this game in the background; `call CoopDeal_EventScript_WaitSave`.
void CoopDuo_SaveNow(void)
{
    CoopDeal_SaveInBackground(0, 0);
}
