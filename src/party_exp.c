// Whole-party experience (see include/party_exp.h).
//
// Why a record and a late payout: a stock "party shares EXP" pays every
// Pokémon through the battle engine, one controller command each, and every
// level-up plays a fanfare, a message and a stats box. Over the co-op cable
// each command is also a round trip, so six benched Pokémon made one
// knockout take seconds. In co-op the benched slots 4-6 are not even in the
// party during the battle (src/coop.c keeps them in a copy).
//
// So the engine notes each knockout (who was defeated, the full experience
// one Pokémon earns, and which party slots fought). When the battle is over,
// each game pays its OWN party from that record: the same formula, the same
// level cap and multipliers as a Pokémon that fought, with no screen. The
// joining player's game, which runs no battle engine, hears each knockout in
// one small message.
#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "caps.h"
#include "coop.h"
#include "coop_protocol.h"
#include "party_exp.h"
#include "pokemon.h"
#include "constants/battle.h"
#include "constants/pokemon.h"

void ApplyExperienceMultipliersToMon(s32 *expAmount, struct Pokemon *mon, bool32 partner, u32 faintedLevel);

#define MAX_KOS 16

// From this game's point of view: "mine" is this game's own party.
struct PartyExpKo
{
    u16 species;    // the defeated species (effort values)
    u16 baseExp;    // the full experience for one Pokémon, before its own multipliers
    u8 level;       // the defeated level (scaled experience)
    u8 mineSent;    // my party slots that fought in this knockout
    u8 mineAlive;   // my live slots that could fight when it happened
};

static EWRAM_DATA struct PartyExpKo sKos[MAX_KOS] = {0};
static EWRAM_DATA u8 sKoCount = 0;
EWRAM_DATA u8 gPartyExpLeveled = 0;

// Message from the link master to the joining player's game.
#define MSG_KO 1
#define MSG_LEN 10

void PartyExp_Reset(void)
{
    memset(sKos, 0, sizeof(sKos));
    sKoCount = 0;
    gPartyExpLeveled = 0;
}

void PartyExp_Drop(void)
{
    PartyExp_Reset();
}

static u8 AliveBits(struct Pokemon *party, u32 count)
{
    u8 bits = 0;
    u32 i;

    for (i = 0; i < count; i++)
    {
        if (IsValidForBattle(&party[i]))
            bits |= 1u << i;
    }
    return bits;
}

void PartyExp_NoteKo(u16 species, u8 level, u32 baseExp, u8 ownSent, u8 partnerSent)
{
    bool32 coop = (gBattleTypeFlags & BATTLE_TYPE_COOP) != 0;
    struct PartyExpKo *ko;
    u8 partnerAlive = 0;

    if (sKoCount >= MAX_KOS)
        return;
    if (baseExp > 0xFFFF)
        baseExp = 0xFFFF;
    ko = &sKos[sKoCount++];
    ko->species = species;
    ko->level = level;
    ko->baseExp = baseExp;
    ko->mineSent = ownSent;
    ko->mineAlive = AliveBits(gParties[B_TRAINER_PLAYER], coop ? COOP_BATTLE_SLOTS : PARTY_SIZE);

    if (coop && (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER))
    {
        u8 msg[MSG_LEN + 2];

        partnerAlive = AliveBits(gParties[B_TRAINER_PARTNER], COOP_BATTLE_SLOTS);
        msg[0] = MSG_KO;
        msg[1] = species & 0xFF;
        msg[2] = species >> 8;
        msg[3] = level;
        msg[4] = baseExp & 0xFF;
        msg[5] = baseExp >> 8;
        msg[6] = partnerSent;   // the joining player's own slots
        msg[7] = partnerAlive;
        msg[8] = 0;
        msg[9] = 0;
        msg[10] = 0;
        msg[11] = 0;
        PrepareBufferDataTransferLink(0, B_COMM_COOP_BENCH, sizeof(msg), msg);
    }
}

void PartyExp_HandleMsg(const u8 *data)
{
    struct PartyExpKo *ko;

    // The master hears its own message back; it recorded the knockout itself.
    if (!(gBattleTypeFlags & BATTLE_TYPE_COOP) || (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER))
        return;
    if (data[0] != MSG_KO || sKoCount >= MAX_KOS)
        return;
    ko = &sKos[sKoCount++];
    ko->species = data[1] | (data[2] << 8);
    ko->level = data[3];
    ko->baseExp = data[4] | (data[5] << 8);
    ko->mineSent = data[6];
    ko->mineAlive = data[7];
}

// The new level-up moves that fit. A full moveset is left alone: the Move
// Relearner teaches what was skipped, and a field prompt (src/coop.c) covers
// the Pokémon on the co-op field.
static void LearnFreeMoves(struct Pokemon *mon, u32 oldLevel, u32 newLevel)
{
    const struct LevelUpMove *moves = GetSpeciesLevelUpLearnset(GetMonData(mon, MON_DATA_SPECIES));
    u32 i;

    for (i = 0; moves[i].move != LEVEL_UP_MOVE_END; i++)
    {
        if (moves[i].level > oldLevel && moves[i].level <= newLevel)
            GiveMoveToMon(mon, moves[i].move);
    }
}

// One knockout's experience for one Pokémon that did not fight: the same
// rules as a Pokémon that did (soft/hard level cap, its own multipliers, EVs).
static void PayOne(struct Pokemon *mon, const struct PartyExpKo *ko)
{
    s32 reward;
    u32 level = GetMonData(mon, MON_DATA_LEVEL);
    u32 exp;

    if (level >= MAX_LEVEL)
    {
        if (B_MAX_LEVEL_EV_GAINS >= GEN_5)
            MonGainEVs(mon, ko->species);
        return;
    }

    reward = GetSoftLevelCapExpValue(level, ko->baseExp);
    ApplyExperienceMultipliersToMon(&reward, mon, FALSE, ko->level);

    if (B_EXP_CAP_TYPE == EXP_CAP_HARD && reward != 0)
    {
        enum GrowthRate growthRate = gSpeciesInfo[GetMonData(mon, MON_DATA_SPECIES)].growthRate;
        u32 currentExp = GetMonData(mon, MON_DATA_EXP);
        u32 levelCap = GetCurrentLevelCap();

        if (level >= levelCap)
            reward = 0;
        else if (gExperienceTables[growthRate][levelCap] < currentExp + reward)
            reward = gExperienceTables[growthRate][levelCap] - currentExp;
    }
    if (reward <= 0)
        return;

    MonGainEVs(mon, ko->species);
    exp = GetMonData(mon, MON_DATA_EXP) + reward;
    SetMonData(mon, MON_DATA_EXP, &exp);
    CalculateMonStats(mon);
}

// All the recorded knockouts for one Pokémon (slotBit: its party slot in battle
// order; a benched copy has no recorded "alive" bits, so it is checked now).
// Returns its level before the payout when it gained a level, else 0.
static u32 PayMon(struct Pokemon *mon, bool32 useRecordedAlive, u32 slotBit)
{
    u32 startLevel = GetMonData(mon, MON_DATA_LEVEL);
    u32 endLevel, level, k;
    bool32 paid = FALSE;

    for (k = 0; k < sKoCount; k++)
    {
        const struct PartyExpKo *ko = &sKos[k];

        if (ko->mineSent & slotBit)
            continue;
        if (useRecordedAlive ? !(ko->mineAlive & slotBit) : !IsValidForBattle(mon))
            continue;
        PayOne(mon, ko);
        paid = TRUE;
    }
    if (!paid)
        return 0;

    endLevel = GetMonData(mon, MON_DATA_LEVEL);
    if (endLevel <= startLevel)
        return 0;
    for (level = startLevel; level < endLevel; level++)
        AdjustFriendship(mon, FRIENDSHIP_EVENT_GROW_LEVEL);
    LearnFreeMoves(mon, startLevel, endLevel);
    return startLevel;
}

void PartyExp_ApplySolo(void)
{
    u32 i;

    if (sKoCount == 0 || (gBattleTypeFlags & BATTLE_TYPE_COOP))
        return;
    gPartyExpLeveled = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (PayMon(mon, TRUE, 1u << i) != 0)
        {
            // The stock check after the battle evolves what is flagged here.
            gLeveledUpInBattle |= 1u << i;
            gPartyExpLeveled |= 1u << i;
        }
    }
    PartyExp_Reset();
}

void PartyExp_ApplyCoop(struct Pokemon *benchCopy, int isGuest)
{
    u32 i;

    if (sKoCount == 0)
        return;
    (void)isGuest;
    gPartyExpLeveled = 0;
    // The three that were on the field (slots in battle order).
    for (i = 0; i < COOP_BATTLE_SLOTS; i++)
    {
        u32 oldLevel = PayMon(&gParties[B_TRAINER_PLAYER][i], TRUE, 1u << i);

        if (oldLevel != 0)
        {
            Coop_RecordSilentLevelUp(i, oldLevel);
            gPartyExpLeveled |= 1u << i;
        }
    }
    // The benched Pokémon: only in the pre-battle copy. They were never sent
    // out, so every knockout pays them.
    for (i = COOP_BATTLE_SLOTS; i < PARTY_SIZE; i++)
    {
        if (PayMon(&benchCopy[i], FALSE, 1u << i) != 0)
            Coop_NoteBenchLevelUp(GetMonData(&benchCopy[i], MON_DATA_PERSONALITY));
    }
    PartyExp_Reset();
}
