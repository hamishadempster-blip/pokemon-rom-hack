// Trainer Bond (docs/COOP_BOND.md).
//
// One Partner Pokémon per player, chosen at the Bond Mentor in Rustboro once
// the Stone Badge is on the save. Bond XP lives in each Pokémon's own record
// (MON_DATA_BOND_XP: reserved bits of the encrypted substructs), so every
// Pokémon that was ever a partner keeps its own history wherever it goes;
// nothing is evicted. The active partner's record carries
// COOP_BOND_MON_ACTIVE and CoopSave v5.bond names it by personality and
// original trainer id (never by slot, nickname or species).
//
// Bonus: a flat +5/+10/+15 on each calculated stat (HP except fixed-HP
// species) by rank, added once in CalculateMonStatsCont after the ordinary
// level/nature/IV/EV values. MON_DATA_BOND_APPLIED records which rank's
// bonus the stored stats include, so a change of eligibility maps the
// current HP proportionally (rounded down, never healing; full stays full,
// fainted stays fainted) instead of the stock max-HP rule, which would heal
// on each toggle.
//
// Contexts: no bonus and no Bond XP in Frontier facilities and tents, PvP
// link battles, practice battles and duo Frontier copies. The rule reads
// only shared battle state in battle, so both games of a co-op battle
// compute the same stats for each other's Pokémon.
#include "global.h"
#include "coop_bond.h"
#include "battle.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_frontier.h"
#include "event_data.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "string_util.h"
#include "map_name_popup.h"
#include "palette.h"
#include "daycare.h"
#include "script.h"
#include "field_screen_effect.h"
#include "constants/characters.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/maps.h"

// Initial tuning (configurable here): cumulative Bond XP for each rank, and
// the bonus to each stat at that rank. Bonuses replace each other.
const u16 gCoopBondThresholds[COOP_BOND_RANK_COUNT] = { 0, 2500, 10000, 25000 };
const u8 gCoopBondBonus[COOP_BOND_RANK_COUNT] = { 0, 5, 10, 15 };

// Award amounts and per-battle caps (docs/COOP_BOND.md, "Pacing").
#define BOND_KO_XP              40   // per opposing knockout while on the field
#define BOND_KO_CAP             4    // knockouts counted per battle
#define BOND_WIN_TRAINER_XP     60
#define BOND_WIN_WILD_XP        30
#define BOND_MAJOR_XP           400  // a first major victory (one-time each)
#define BOND_DAMAGE_PCT_PER_XP  2    // 1 XP per 2% of a foe's max HP dealt
#define BOND_DAMAGE_CAP         60
#define BOND_CATCH_XP           40
#define BOND_SUPPORT_XP         10
#define BOND_SUPPORT_CAP        3
#define BOND_STEPS_PER_XP       32
#define BOND_FRIENDSHIP_XP      2    // per friendship point gained
#define BOND_FRIENDSHIP_CAP     10   // per event

// Per-battle tracking on this game, for this player's own partner.
struct BondBattle
{
    bool8 pending;          // a battle started; its end not handled yet
    bool8 eligible;         // a partner in the party at the start, context allows XP
    u32 pid;
    u32 otId;
    u8 kos;
    u8 supports;
    bool8 participated;     // on the field at some point
    bool8 dealtDamage;
    u16 damagePct;          // sum of % of foes' max HP dealt (capped)
    u16 majorsBefore;       // major-victory flags before the battle
};

static EWRAM_DATA struct BondBattle sBattle = {0};

static struct CoopBondSave *Bond_Save(void)
{
    struct CoopSave *save = Coop_GetSave();

    return save != NULL ? &save->v5.bond : NULL;
}

u8 CoopBond_RankFromXp(u32 xp)
{
    u8 rank = COOP_BOND_RANK_NEW;

    while (rank + 1 < COOP_BOND_RANK_COUNT && xp >= gCoopBondThresholds[rank + 1])
        rank++;
    return rank;
}

u32 CoopBond_MonXp(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_BOND_XP);
}

u8 CoopBond_MonRank(struct Pokemon *mon)
{
    return CoopBond_RankFromXp(GetMonData(mon, MON_DATA_BOND_XP));
}

bool8 CoopBond_HasHistory(struct BoxPokemon *boxMon)
{
    return (GetBoxMonData(boxMon, MON_DATA_BOND_FLAGS) & COOP_BOND_MON_HISTORY) != 0;
}

static bool8 Bond_KeyMatches(struct BoxPokemon *boxMon)
{
    struct CoopBondSave *bond = Bond_Save();

    return bond != NULL && bond->active
        && GetBoxMonData(boxMon, MON_DATA_PERSONALITY) == bond->pid
        && GetBoxMonData(boxMon, MON_DATA_OT_ID) == bond->otId;
}

// This player's designated partner (record flag and save key agree).
bool8 CoopBond_IsActivePartner(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    if (!(GetMonData(mon, MON_DATA_BOND_FLAGS) & COOP_BOND_MON_ACTIVE))
        return FALSE;
    return Bond_KeyMatches(&mon->box);
}

static bool8 Bond_InParty(struct Pokemon *mon, u8 trainer)
{
    return mon >= &gParties[trainer][0] && mon < &gParties[trainer][PARTY_SIZE];
}

// Battle facilities and rental challenges (outside battle: by map).
static bool8 Bond_MapSuppresses(void)
{
    u16 map = (gSaveBlock1Ptr->location.mapGroup << 8) | gSaveBlock1Ptr->location.mapNum;

    switch (map)
    {
    case MAP_BATTLE_FRONTIER_BATTLE_TOWER_LOBBY:
    case MAP_BATTLE_FRONTIER_BATTLE_TOWER_ELEVATOR:
    case MAP_BATTLE_FRONTIER_BATTLE_TOWER_CORRIDOR:
    case MAP_BATTLE_FRONTIER_BATTLE_TOWER_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_PARTNER_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_CORRIDOR:
    case MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_DOME_LOBBY:
    case MAP_BATTLE_FRONTIER_BATTLE_DOME_CORRIDOR:
    case MAP_BATTLE_FRONTIER_BATTLE_DOME_PRE_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_DOME_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_PALACE_LOBBY:
    case MAP_BATTLE_FRONTIER_BATTLE_PALACE_CORRIDOR:
    case MAP_BATTLE_FRONTIER_BATTLE_PALACE_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY:
    case MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR:
    case MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_TOP:
    case MAP_BATTLE_FRONTIER_BATTLE_ARENA_LOBBY:
    case MAP_BATTLE_FRONTIER_BATTLE_ARENA_CORRIDOR:
    case MAP_BATTLE_FRONTIER_BATTLE_ARENA_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_FACTORY_LOBBY:
    case MAP_BATTLE_FRONTIER_BATTLE_FACTORY_PRE_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_FACTORY_BATTLE_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_PIKE_LOBBY:
    case MAP_BATTLE_FRONTIER_BATTLE_PIKE_CORRIDOR:
    case MAP_BATTLE_FRONTIER_BATTLE_PIKE_THREE_PATH_ROOM:
    case MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_NORMAL:
    case MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_FINAL:
    case MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS:
    case MAP_FALLARBOR_TOWN_BATTLE_TENT_LOBBY:
    case MAP_FALLARBOR_TOWN_BATTLE_TENT_CORRIDOR:
    case MAP_FALLARBOR_TOWN_BATTLE_TENT_BATTLE_ROOM:
    case MAP_VERDANTURF_TOWN_BATTLE_TENT_LOBBY:
    case MAP_VERDANTURF_TOWN_BATTLE_TENT_CORRIDOR:
    case MAP_VERDANTURF_TOWN_BATTLE_TENT_BATTLE_ROOM:
    case MAP_SLATEPORT_CITY_BATTLE_TENT_LOBBY:
    case MAP_SLATEPORT_CITY_BATTLE_TENT_CORRIDOR:
    case MAP_SLATEPORT_CITY_BATTLE_TENT_BATTLE_ROOM:
        return TRUE;
    }
    return FALSE;
}

// In battle: only state both games of a link battle share.
static bool8 Bond_BattleSuppresses(void)
{
    if (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_RECORDED | BATTLE_TYPE_RECORDED_LINK
                          | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_HILL))
        return TRUE;
    if ((gBattleTypeFlags & BATTLE_TYPE_LINK) && !(gBattleTypeFlags & BATTLE_TYPE_COOP))
        return TRUE;    // PvP
    if (gCoopPracticeBattle || gCoopDuoBattle)
        return TRUE;
    return FALSE;
}

bool8 CoopBond_ContextSuppressed(void)
{
    if (gMain.inBattle)
        return Bond_BattleSuppresses();
    return Bond_MapSuppresses() || gCoopDuoBattle || gCoopPracticeBattle;
}

static EWRAM_DATA bool8 sForBattle = FALSE;

u8 CoopBond_EffectiveRank(struct Pokemon *mon)
{
    u32 flags;

    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return 0;
    flags = GetMonData(mon, MON_DATA_BOND_FLAGS);
    if (!(flags & COOP_BOND_MON_ACTIVE))
        return 0;
    if (sForBattle ? Bond_BattleSuppresses() : CoopBond_ContextSuppressed())
        return 0;
    if (Bond_InParty(mon, B_TRAINER_PLAYER))
    {
        // Our own: the save must name it too (a stale flag gives nothing).
        if (!Bond_KeyMatches(&mon->box))
            return 0;
    }
    else if (!Bond_InParty(mon, B_TRAINER_PARTNER))
    {
        // Outside battle, a copy of this save's own partner (the PC's
        // moving Pokémon, a summary of a boxed one) keeps its bonus: the PC
        // copies a withdrawn Pokémon through such a copy, and mapping its
        // HP down and back up would lose a point to rounding each move.
        if (!gMain.inBattle && !sForBattle && Bond_KeyMatches(&mon->box))
            return CoopBond_MonRank(mon);
        // Other copies (rentals, practice foes, display sprites, a copy in
        // battle): never a real partner.
        return 0;
    }
    // The partner's Pokémon in a co-op battle: its record says so (its
    // owner set the flag; both games apply the same rule).
    return CoopBond_MonRank(mon);
}

static void Bond_RefreshMon(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return;
    if (GetMonData(mon, MON_DATA_BOND_APPLIED) != CoopBond_EffectiveRank(mon))
        CalculateMonStats(mon);
}

// A flag on a Pokémon this save does not name (it arrived from elsewhere,
// or the save names another) is cleared; the history stays.
static void Bond_ClearStaleFlags(void)
{
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u32 flags;

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        flags = GetMonData(mon, MON_DATA_BOND_FLAGS);
        if ((flags & COOP_BOND_MON_ACTIVE) && !Bond_KeyMatches(&mon->box))
        {
            flags &= ~COOP_BOND_MON_ACTIVE;
            SetMonData(mon, MON_DATA_BOND_FLAGS, &flags);
        }
    }
}

// Still on this save (party, PC or Day Care)? Personality and trainer id
// are outside the record's encryption, so this is a cheap scan.
static bool8 Bond_KeyOwned(u32 pid, u32 otId)
{
    u32 i, j;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct BoxPokemon *b = &gParties[B_TRAINER_PLAYER][i].box;

        if (GetBoxMonData(b, MON_DATA_PERSONALITY) == pid && GetBoxMonData(b, MON_DATA_OT_ID) == otId
         && GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE)
            return TRUE;
    }
    for (i = 0; i < DAYCARE_MON_COUNT; i++)
    {
        struct BoxPokemon *b = &gSaveBlock1Ptr->daycare.mons[i].mon;

        if (GetBoxMonData(b, MON_DATA_PERSONALITY) == pid && GetBoxMonData(b, MON_DATA_OT_ID) == otId
         && GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE)
            return TRUE;
    }
    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
    {
        for (j = 0; j < IN_BOX_COUNT; j++)
        {
            struct BoxPokemon *b = GetBoxedMonPtr(i, j);

            if (GetBoxMonData(b, MON_DATA_PERSONALITY) == pid && GetBoxMonData(b, MON_DATA_OT_ID) == otId
             && GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE)
                return TRUE;
        }
    }
    // Offered in a deal that is not settled yet: a rollback returns it.
    return CoopDeal_EscrowHolds(pid, otId);
}

// The partner left this save (traded, given in a deal, released, given to
// an in-game trade): the designation ends. Its history went with it; if it
// ever comes back it is not the partner until chosen again.
static void Bond_CheckDesignationOwned(void)
{
    struct CoopBondSave *bond = Bond_Save();

    if (bond == NULL || !bond->active)
        return;
    if (Bond_KeyOwned(bond->pid, bond->otId))
        return;
    bond->active = FALSE;
    bond->pid = 0;
    bond->otId = 0;
}

void CoopBond_RefreshParty(void)
{
    u32 i;

    if (gMain.inBattle)
        return;
    Bond_CheckDesignationOwned();
    Bond_ClearStaleFlags();
    for (i = 0; i < PARTY_SIZE; i++)
        Bond_RefreshMon(&gParties[B_TRAINER_PLAYER][i]);
    CoopBond_PublishTest();
}

void CoopBond_RefreshPartyForBattle(void)
{
    u32 i;

    Bond_ClearStaleFlags();
    sForBattle = TRUE;
    for (i = 0; i < PARTY_SIZE; i++)
        Bond_RefreshMon(&gParties[B_TRAINER_PLAYER][i]);
    sForBattle = FALSE;
    CoopBond_PublishTest();
}

s8 CoopBond_ActiveSlot(void)
{
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (CoopBond_IsActivePartner(&gParties[B_TRAINER_PLAYER][i]))
            return i;
    }
    return -1;
}

// Any other owned Pokémon with the same personality and trainer id (only
// copies made outside the game can share both): refuse to designate it.
static bool8 Bond_KeyIsUnique(u32 pid, u32 otId, struct BoxPokemon *self)
{
    u32 i, j;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct BoxPokemon *b = &gParties[B_TRAINER_PLAYER][i].box;

        if (b != self && GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE
         && GetBoxMonData(b, MON_DATA_PERSONALITY) == pid && GetBoxMonData(b, MON_DATA_OT_ID) == otId)
            return FALSE;
    }
    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
    {
        for (j = 0; j < IN_BOX_COUNT; j++)
        {
            struct BoxPokemon *b = GetBoxedMonPtr(i, j);

            if (GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE
             && GetBoxMonData(b, MON_DATA_PERSONALITY) == pid && GetBoxMonData(b, MON_DATA_OT_ID) == otId)
                return FALSE;
        }
    }
    return TRUE;
}

static void Bond_SetActiveFlag(struct BoxPokemon *boxMon, bool8 active)
{
    u32 flags = GetBoxMonData(boxMon, MON_DATA_BOND_FLAGS);

    if (active)
        flags |= COOP_BOND_MON_ACTIVE | COOP_BOND_MON_HISTORY;
    else
        flags &= ~COOP_BOND_MON_ACTIVE;
    SetBoxMonData(boxMon, MON_DATA_BOND_FLAGS, &flags);
}

// Clears the active flag wherever the old partner is now (party or PC).
static void Bond_ClearOldFlag(u32 pid, u32 otId)
{
    u32 i, j;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct BoxPokemon *b = &gParties[B_TRAINER_PLAYER][i].box;

        if (GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE
         && GetBoxMonData(b, MON_DATA_PERSONALITY) == pid && GetBoxMonData(b, MON_DATA_OT_ID) == otId)
            Bond_SetActiveFlag(b, FALSE);
    }
    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
    {
        for (j = 0; j < IN_BOX_COUNT; j++)
        {
            struct BoxPokemon *b = GetBoxedMonPtr(i, j);

            if (GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE
             && GetBoxMonData(b, MON_DATA_PERSONALITY) == pid && GetBoxMonData(b, MON_DATA_OT_ID) == otId)
                Bond_SetActiveFlag(b, FALSE);
        }
    }
}

bool8 CoopBond_Designate(u8 partySlot)
{
    struct CoopBondSave *bond = Bond_Save();
    struct Pokemon *mon;
    u32 pid, otId;

    if (bond == NULL || partySlot >= PARTY_SIZE || gMain.inBattle)
        return FALSE;
    mon = &gParties[B_TRAINER_PLAYER][partySlot];
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    pid = GetMonData(mon, MON_DATA_PERSONALITY);
    otId = GetMonData(mon, MON_DATA_OT_ID);
    if (!Bond_KeyIsUnique(pid, otId, &mon->box))
        return FALSE;
    if (bond->active)
        Bond_ClearOldFlag(bond->pid, bond->otId);
    bond->pid = pid;
    bond->otId = otId;
    bond->active = TRUE;
    Bond_SetActiveFlag(&mon->box, TRUE);
    CoopBond_RefreshParty();
    return TRUE;
}

void CoopBond_ClearDesignation(void)
{
    struct CoopBondSave *bond = Bond_Save();

    if (bond == NULL || !bond->active)
        return;
    Bond_ClearOldFlag(bond->pid, bond->otId);
    bond->active = FALSE;
    bond->pid = 0;
    bond->otId = 0;
    CoopBond_RefreshParty();
}

void CoopBond_MonLeaving(struct BoxPokemon *boxMon)
{
    struct CoopBondSave *bond = Bond_Save();

    // The history travels with it; the designation does not.
    Bond_SetActiveFlag(boxMon, FALSE);
    if (bond != NULL && bond->active
     && GetBoxMonData(boxMon, MON_DATA_PERSONALITY) == bond->pid
     && GetBoxMonData(boxMon, MON_DATA_OT_ID) == bond->otId)
    {
        bond->active = FALSE;
        bond->pid = 0;
        bond->otId = 0;
    }
}

void CoopBond_MonArrived(struct BoxPokemon *boxMon)
{
    if (!Bond_KeyMatches(boxMon))
        Bond_SetActiveFlag(boxMon, FALSE);
}

// Partner menu offer (VAR_0x8004 the party slot): is it the active
// partner? Its nickname goes to gStringVar1 for the question.
u16 CoopBond_ScriptSlotIsPartner(void)
{
    u16 slot = gSpecialVar_0x8004;

    if (slot >= PARTY_SIZE || CoopBond_ActiveSlot() != (s8)slot)
        return FALSE;
    GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
    return TRUE;
}

// --- Bond XP -------------------------------------------------------------

static EWRAM_DATA u16 sNoticeGain = 0;
static EWRAM_DATA u8 sNoticeRank = 0;

static struct Pokemon *Bond_PartyPartner(void)
{
    s8 slot = CoopBond_ActiveSlot();

    return slot >= 0 ? &gParties[B_TRAINER_PLAYER][slot] : NULL;
}

void CoopBond_Award(u8 source, u32 amount)
{
    struct Pokemon *mon;
    u32 xp, before;
    u8 oldRank, newRank;

    if (amount == 0 || source >= COOP_BOND_SRC_COUNT)
        return;
    if (CoopBond_ContextSuppressed() && source != COOP_BOND_SRC_MAJOR)
        return;
    mon = Bond_PartyPartner();
    if (mon == NULL)
        return;    // boxed or absent: earns nothing
    before = xp = GetMonData(mon, MON_DATA_BOND_XP);
    xp += amount;
    if (xp > COOP_BOND_XP_MAX)
        xp = COOP_BOND_XP_MAX;
    if (xp == before)
        return;
    SetMonData(mon, MON_DATA_BOND_XP, &xp);
    oldRank = CoopBond_RankFromXp(before);
    newRank = CoopBond_RankFromXp(xp);
    // Battle and capture awards are announced; walking and friendship are
    // not (a line after every few steps would be noise).
    if (source != COOP_BOND_SRC_STEPS && source != COOP_BOND_SRC_FRIENDSHIP
     && sNoticeGain + (xp - before) < 0xFFFF)
        sNoticeGain += xp - before;
    if (newRank > oldRank)
    {
        struct CoopBondSave *bond = Bond_Save();

        sNoticeRank = newRank + 1;
        if (bond != NULL)
            bond->noticeRank = newRank + 1;
        // The new bonus: at a safe point only (never mid-move).
        if (!gMain.inBattle)
            CoopBond_RefreshParty();
    }
    CoopBond_PublishTest();
}

bool8 CoopBond_TakeNotice(u8 *rank, u16 *gain)
{
    struct CoopBondSave *bond = Bond_Save();

    if (sNoticeGain == 0 && sNoticeRank == 0)
        return FALSE;
    *rank = sNoticeRank;
    *gain = sNoticeGain;
    sNoticeRank = 0;
    sNoticeGain = 0;
    if (bond != NULL)
        bond->noticeRank = 0;
    return TRUE;
}

// --- Battle --------------------------------------------------------------

// Major victories: badges, the Elite Four and the champion.
static u16 Bond_MajorFlags(void)
{
    static const u16 sFlags[] = {
        FLAG_BADGE01_GET, FLAG_BADGE02_GET, FLAG_BADGE03_GET, FLAG_BADGE04_GET,
        FLAG_BADGE05_GET, FLAG_BADGE06_GET, FLAG_BADGE07_GET, FLAG_BADGE08_GET,
        FLAG_DEFEATED_ELITE_4_SIDNEY, FLAG_DEFEATED_ELITE_4_PHOEBE,
        FLAG_DEFEATED_ELITE_4_GLACIA, FLAG_DEFEATED_ELITE_4_DRAKE, FLAG_IS_CHAMPION,
    };
    u16 bits = 0;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sFlags); i++)
    {
        if (FlagGet(sFlags[i]))
            bits |= 1 << i;
    }
    return bits;
}

static void Bond_TestSupports(u8 n);

static EWRAM_DATA bool8 sMajorWatch = FALSE;
static EWRAM_DATA u16 sMajorBefore = 0;

void CoopBond_BattleStart(void)
{
    struct Pokemon *mon;

    // The last won battle's flags are settled before a new one starts.
    CoopBond_SettleMajors();
    sMajorWatch = FALSE;
    memset(&sBattle, 0, sizeof(sBattle));
    Bond_TestSupports(0);
    sBattle.pending = TRUE;
    if (Bond_BattleSuppresses())
        return;
    mon = Bond_PartyPartner();
    if (mon == NULL)
        return;
    sBattle.eligible = TRUE;
    sBattle.pid = GetMonData(mon, MON_DATA_PERSONALITY);
    sBattle.otId = GetMonData(mon, MON_DATA_OT_ID);
    sBattle.majorsBefore = Bond_MajorFlags();
}

// This player's battler holding the partner now, or MAX_BATTLERS_COUNT.
static u8 Bond_PartnerBattler(void)
{
    u32 b;

    if (!sBattle.eligible)
        return MAX_BATTLERS_COUNT;
    for (b = 0; b < gBattlersCount; b++)
    {
        struct Pokemon *mon;

        if (!IsOnPlayerSide(b))
            continue;
        if (gAbsentBattlerFlags & (1u << b))
            continue;
        mon = GetBattlerMon(b);
        if (mon != NULL && Bond_InParty(mon, B_TRAINER_PLAYER)
         && GetMonData(mon, MON_DATA_PERSONALITY) == sBattle.pid
         && GetMonData(mon, MON_DATA_OT_ID) == sBattle.otId)
        {
            sBattle.participated = TRUE;
            return b;
        }
    }
    return MAX_BATTLERS_COUNT;
}

void CoopBond_BattleDamage(u8 attacker, u8 target, s32 damage)
{
    u8 partner = Bond_PartnerBattler();
    struct Pokemon *mon;
    s32 maxHP, curHP;

    if (partner == MAX_BATTLERS_COUNT || attacker != partner || target >= MAX_BATTLERS_COUNT)
        return;
    mon = GetBattlerMon(target);
    if (mon == NULL)
        return;
    maxHP = GetMonData(mon, MON_DATA_MAX_HP);
    curHP = GetMonData(mon, MON_DATA_HP);
    if (maxHP <= 0)
        return;
    if (damage > 0 && !IsOnPlayerSide(target))
    {
        // Actual damage only: never past the HP it had (no overkill).
        if (damage > curHP)
            damage = curHP;
        if (damage <= 0)
            return;
        sBattle.dealtDamage = TRUE;
        sBattle.damagePct += (damage * 100) / maxHP;
        if (sBattle.damagePct > BOND_DAMAGE_CAP * BOND_DAMAGE_PCT_PER_XP)
            sBattle.damagePct = BOND_DAMAGE_CAP * BOND_DAMAGE_PCT_PER_XP;
    }
    else if (damage < 0 && IsOnPlayerSide(target) && target != attacker)
    {
        // Healed an injured ally (not itself, never past full).
        if (curHP < maxHP && sBattle.supports < BOND_SUPPORT_CAP)
            sBattle.supports++;
    }
}

void CoopBond_BattleSupport(u8 battler, u16 what)
{
    // A support effect the engine confirmed for this player's partner
    // (Reflect, Light Screen, Aurora Veil, Tailwind, Trick Room: their
    // success messages), counted per use, capped per battle.
    if (Bond_PartnerBattler() == battler && sBattle.supports < BOND_SUPPORT_CAP)
    {
        sBattle.supports++;
        Bond_TestSupports(sBattle.supports);
    }
}

void CoopBond_BattleKnockout(u8 target)
{
    if (IsOnPlayerSide(target))
        return;
    if (Bond_PartnerBattler() == MAX_BATTLERS_COUNT)
        return;
    if (sBattle.kos < BOND_KO_CAP)
        sBattle.kos++;
}

void CoopBond_BattleEnd(u8 outcome)
{
    u32 xp = 0;

    if (!sBattle.eligible)
        return;
    sBattle.eligible = FALSE;
    // Still in the party (not lost through a release or trade mid-battle).
    if (Bond_PartyPartner() == NULL)
        return;
    // Was it ever out? (Checked once more now for a partner still on the field.)
    if (!sBattle.participated)
        return;
    xp += sBattle.kos * BOND_KO_XP;
    xp += (sBattle.damagePct / BOND_DAMAGE_PCT_PER_XP);
    xp += sBattle.supports * BOND_SUPPORT_XP;
    if (outcome == B_OUTCOME_WON)
        xp += (gBattleTypeFlags & BATTLE_TYPE_TRAINER) ? BOND_WIN_TRAINER_XP : BOND_WIN_WILD_XP;
    if (outcome == B_OUTCOME_CAUGHT && (sBattle.dealtDamage || sBattle.supports))
        xp += BOND_CATCH_XP;
    // Losses, escapes and interrupted battles earn only what was done
    // (knockouts, damage, support), never a victory.
    if (outcome != B_OUTCOME_WON && outcome != B_OUTCOME_CAUGHT && outcome != B_OUTCOME_LOST
     && outcome != B_OUTCOME_DREW && outcome != B_OUTCOME_RAN && outcome != B_OUTCOME_PLAYER_TELEPORTED
     && outcome != B_OUTCOME_MON_FLED && outcome != B_OUTCOME_MON_TELEPORTED && outcome != B_OUTCOME_FORFEITED)
        return;    // an aborted (link) battle: nothing
    sForBattle = FALSE;
    CoopBond_Award(COOP_BOND_SRC_KO, xp);
    // First major victories: the script that follows a gym leader, an Elite
    // Four member or the champion sets the badge or flag after the battle,
    // so watch for it until the next battle (Bond_SettleMajors).
    if (outcome == B_OUTCOME_WON && (gBattleTypeFlags & BATTLE_TYPE_TRAINER))
    {
        sMajorWatch = TRUE;
        sMajorBefore = sBattle.majorsBefore;
    }
}

// A badge, Elite Four or champion flag set since the start of the battle
// the partner won: its one-time bonus (each bit once per save, ever).
void CoopBond_SettleMajors(void)
{
    struct CoopBondSave *bond = Bond_Save();
    u16 newMajors;
    u32 i, count = 0;

    if (!sMajorWatch || bond == NULL)
        return;
    newMajors = Bond_MajorFlags() & ~sMajorBefore;
    for (i = 0; i < 16; i++)
    {
        if ((newMajors & (1 << i)) && !(bond->firstWins & (1 << i)))
        {
            bond->firstWins |= 1 << i;
            count++;
        }
    }
    if (count != 0)
        CoopBond_Award(COOP_BOND_SRC_MAJOR, count * BOND_MAJOR_XP);
}

void CoopBond_CatchHelped(void)
{
    // (Counted in CoopBond_BattleEnd from the outcome.)
}

// A co-op battle dropped (the party goes back to its snapshot): nothing.
void CoopBond_BattleAborted(void)
{
    sBattle.pending = FALSE;
    sBattle.eligible = FALSE;
}

// The field again (after a battle, a menu, the PC): settle the battle that
// just ended, then put the right bonus on the party.
void CoopBond_FieldResume(void)
{
    if (sBattle.pending)
    {
        sBattle.pending = FALSE;
        CoopBond_BattleEnd(gBattleOutcome);
    }
    CoopBond_RefreshParty();
}

// --- Field ---------------------------------------------------------------

void CoopBond_OnStep(void)
{
    struct CoopBondSave *bond = Bond_Save();
    struct Pokemon *lead = &gParties[B_TRAINER_PLAYER][0];
    u32 hp, maxHP;

    if (bond == NULL || !CoopBond_IsActivePartner(lead) || CoopBond_ContextSuppressed())
        return;
    hp = GetMonData(lead, MON_DATA_HP);
    maxHP = GetMonData(lead, MON_DATA_MAX_HP);
    // Healthy: more than half its HP and no status.
    if (hp * 2 <= maxHP || GetMonData(lead, MON_DATA_STATUS) != 0)
        return;
    if (++bond->stepTally >= BOND_STEPS_PER_XP)
    {
        bond->stepTally = 0;
        CoopBond_Award(COOP_BOND_SRC_STEPS, 1);
    }
}

void CoopBond_OnFriendshipGain(struct Pokemon *mon, s32 delta)
{
    if (delta <= 0 || !CoopBond_IsActivePartner(mon))
        return;
    delta *= BOND_FRIENDSHIP_XP;
    if (delta > BOND_FRIENDSHIP_CAP)
        delta = BOND_FRIENDSHIP_CAP;
    CoopBond_Award(COOP_BOND_SRC_FRIENDSHIP, delta);
}

// --- Text and the Bond Mentor (data/scripts/coop_bond.inc) -----------------

static const u8 *const sRankNames[COOP_BOND_RANK_COUNT] = {
    COMPOUND_STRING("New Partner"),
    COMPOUND_STRING("Companion"),
    COMPOUND_STRING("Trusted Partner"),
    COMPOUND_STRING("Ace Partner"),
};

const u8 *CoopBond_RankName(u8 rank)
{
    return sRankNames[rank < COOP_BOND_RANK_COUNT ? rank : 0];
}

// "5,500"
u8 *CoopBond_FormatNumber(u8 *dst, u32 value)
{
    if (value >= 1000)
    {
        dst = ConvertIntToDecimalStringN(dst, value / 1000, STR_CONV_MODE_LEFT_ALIGN, 2);
        *dst++ = CHAR_COMMA;
        return ConvertIntToDecimalStringN(dst, value % 1000, STR_CONV_MODE_LEADING_ZEROS, 3);
    }
    return ConvertIntToDecimalStringN(dst, value, STR_CONV_MODE_LEFT_ALIGN, 3);
}

// The bond summary of a Pokémon into dst. Summary page (forMentor FALSE),
// four short lines: "ACTIVE PARTNER: Companion" (or PREVIOUS PARTNER),
// "Bond XP 5,500", "Next: Trusted Partner at 10,000" (or MAX RANK),
// "4,500 XP needed. +5 each stat" (or why the bonus is off). The mentor's
// version is one paged message.
void CoopBond_DescribeMon(u8 *dst, struct Pokemon *mon, u8 mode)
{
    bool8 forMentor = (mode == COOP_BOND_DESC_MENTOR);
    u32 xp = GetMonData(mon, MON_DATA_BOND_XP);
    u8 rank = CoopBond_RankFromXp(xp);
    bool8 flagged = (GetMonData(mon, MON_DATA_BOND_FLAGS) & COOP_BOND_MON_ACTIVE) != 0;
    bool8 active = flagged && (Bond_KeyMatches(&mon->box) || Bond_InParty(mon, B_TRAINER_PARTNER));
    u8 *p = dst;

    if (!forMentor)
    {
        p = StringCopy(p, active ? COMPOUND_STRING("ACTIVE PARTNER: ") : COMPOUND_STRING("PREVIOUS PARTNER: "));
        p = StringCopy(p, sRankNames[rank]);
        *p++ = CHAR_NEWLINE;
        p = StringCopy(p, COMPOUND_STRING("Bond XP "));
        p = CoopBond_FormatNumber(p, xp);
        *p++ = CHAR_NEWLINE;
    }
    else
    {
        p = StringCopy(p, sRankNames[rank]);
        p = StringCopy(p, COMPOUND_STRING(", Bond XP "));
        p = CoopBond_FormatNumber(p, xp);
        *p++ = CHAR_PROMPT_SCROLL;
    }
    if (rank + 1 < COOP_BOND_RANK_COUNT)
    {
        p = StringCopy(p, COMPOUND_STRING("Next: "));
        p = StringCopy(p, sRankNames[rank + 1]);
        p = StringCopy(p, COMPOUND_STRING(" at "));
        p = CoopBond_FormatNumber(p, gCoopBondThresholds[rank + 1]);
        *p++ = forMentor ? CHAR_NEWLINE : CHAR_NEWLINE;
        p = CoopBond_FormatNumber(p, gCoopBondThresholds[rank + 1] - xp);
        p = StringCopy(p, COMPOUND_STRING(" XP needed. "));
    }
    else
    {
        p = StringCopy(p, COMPOUND_STRING("MAX RANK"));
        *p++ = CHAR_NEWLINE;
    }
    if (forMentor)
        *p++ = CHAR_PROMPT_SCROLL;
    if (!active)
        p = StringCopy(p, COMPOUND_STRING("Bonus paused"));
    else if (mode == COOP_BOND_DESC_BOXED)
        p = StringCopy(p, COMPOUND_STRING("Bonus in the party"));
    else if (CoopBond_ContextSuppressed())
        p = StringCopy(p, COMPOUND_STRING("Bonus inactive here"));
    else if (gCoopBondBonus[rank] == 0)
        p = StringCopy(p, COMPOUND_STRING("No bonus yet"));
    else
    {
        p = StringCopy(p, COMPOUND_STRING("+"));
        p = ConvertIntToDecimalStringN(p, gCoopBondBonus[rank], STR_CONV_MODE_LEFT_ALIGN, 2);
        p = StringCopy(p, forMentor ? COMPOUND_STRING(" to each stat") : COMPOUND_STRING(" each stat"));
    }
    *p = EOS;
}

// Progress within the current rank (the summary bar): *done of *span.
void CoopBond_RankProgress(struct Pokemon *mon, u32 *done, u32 *span)
{
    u32 xp = GetMonData(mon, MON_DATA_BOND_XP);
    u8 rank = CoopBond_RankFromXp(xp);

    if (rank + 1 >= COOP_BOND_RANK_COUNT)
    {
        *done = 1;
        *span = 1;
        return;
    }
    *done = xp - gCoopBondThresholds[rank];
    *span = gCoopBondThresholds[rank + 1] - gCoopBondThresholds[rank];
}

// First conversation heard? (sets it)
u16 CoopBond_ScriptMetMentor(void)
{
    struct CoopBondSave *bond = Bond_Save();
    u16 met;

    if (bond == NULL)
        return TRUE;
    met = (bond->flags & COOP_BOND_F_MET_MENTOR) != 0;
    bond->flags |= COOP_BOND_F_MET_MENTOR;
    return met;
}

// Busy with the partner (a deal, an answered invitation, a launch).
u16 CoopBond_ScriptBusy(void)
{
    return CoopDeal_Busy() || Coop_OtherInteractionBusy();
}

// The current partner, if any: STR_VAR_1 its name. 0 none, 1 in the
// party, 2 elsewhere (PC or Day Care).
u16 CoopBond_ScriptCurrent(void)
{
    struct CoopBondSave *bond = Bond_Save();
    s8 slot = CoopBond_ActiveSlot();
    u32 i, j;

    if (bond == NULL || !bond->active)
        return 0;
    if (slot >= 0)
    {
        GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_NICKNAME, gStringVar1);
        StringGet_Nickname(gStringVar1);
        return 1;
    }
    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
    {
        for (j = 0; j < IN_BOX_COUNT; j++)
        {
            struct BoxPokemon *b = GetBoxedMonPtr(i, j);

            if (GetBoxMonData(b, MON_DATA_SPECIES) != SPECIES_NONE
             && GetBoxMonData(b, MON_DATA_PERSONALITY) == bond->pid
             && GetBoxMonData(b, MON_DATA_OT_ID) == bond->otId)
            {
                GetBoxMonData(b, MON_DATA_NICKNAME, gStringVar1);
                StringGet_Nickname(gStringVar1);
                return 2;
            }
        }
    }
    StringCopy(gStringVar1, COMPOUND_STRING("your partner"));
    return 2;
}

// The Pokémon chosen in VAR_0x8004 (party slot): 0 can be the partner,
// 1 an egg, 2 already the partner, 3 shares its identity with another
// Pokémon (cannot be told apart). STR_VAR_1 its name; gStringVar4 the
// preview: its Bond XP, rank and the bonus it would bring.
u16 CoopBond_ScriptCheckChoice(void)
{
    u16 slot = gSpecialVar_0x8004;
    struct Pokemon *mon;
    u32 xp;
    u8 rank, *p;

    if (slot >= PARTY_SIZE)
        return 1;
    mon = &gParties[B_TRAINER_PLAYER][slot];
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return 1;
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
    if (CoopBond_IsActivePartner(mon))
        return 2;
    if (!Bond_KeyIsUnique(GetMonData(mon, MON_DATA_PERSONALITY), GetMonData(mon, MON_DATA_OT_ID), &mon->box))
        return 3;
    xp = GetMonData(mon, MON_DATA_BOND_XP);
    rank = CoopBond_RankFromXp(xp);
    p = StringCopy(gStringVar4, gStringVar1);
    p = StringCopy(p, COMPOUND_STRING(": "));
    p = StringCopy(p, sRankNames[rank]);
    p = StringCopy(p, COMPOUND_STRING(",\nBond XP "));
    p = CoopBond_FormatNumber(p, xp);
    p = StringCopy(p, COMPOUND_STRING(". Bonus as partner: +"));
    p = ConvertIntToDecimalStringN(p, gCoopBondBonus[rank], STR_CONV_MODE_LEFT_ALIGN, 2);
    p = StringCopy(p, COMPOUND_STRING(".\pMake it your Partner POKéMON?"));
    *p = EOS;
    return 0;
}

u16 CoopBond_ScriptDesignate(void)
{
    return CoopBond_Designate(gSpecialVar_0x8004);
}

// CHECK BOND: gStringVar4 = the active partner's bond (0: none chosen).
u16 CoopBond_ScriptCheckBond(void)
{
    s8 slot = CoopBond_ActiveSlot();
    u8 *p;

    if (slot < 0)
        return CoopBond_ScriptCurrent() != 0 ? 2 : 0;
    p = StringCopy(gStringVar4, COMPOUND_STRING("Your partner "));
    GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
    p = StringCopy(p, gStringVar1);
    p = StringCopy(p, COMPOUND_STRING(":\n"));
    CoopBond_DescribeMon(p, &gParties[B_TRAINER_PLAYER][slot], COOP_BOND_DESC_MENTOR);
    return 1;
}

u16 CoopBond_ScriptCheckChoiceName(void)
{
    if (gSpecialVar_0x8004 < PARTY_SIZE)
    {
        GetMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], MON_DATA_NICKNAME, gStringVar1);
        StringGet_Nickname(gStringVar1);
    }
    return 0;
}

// Test visibility (the harness reads it, +33 supports): per party slot its Bond XP,
// flags, applied rank and effective rank; the save's designation.
struct CoopBondTest
{
    u16 xp[PARTY_SIZE];
    u8 flags[PARTY_SIZE];
    u8 applied[PARTY_SIZE];
    u8 effective[PARTY_SIZE];
    u8 active;
    u8 activeSlot;
    u8 suppressed;
    u8 supports;    // support uses counted in the current/last battle
    u32 pid;
};
EWRAM_DATA struct CoopBondTest gCoopBondTest = {0};

static void Bond_TestSupports(u8 n)
{
    gCoopBondTest.supports = n;
}

void CoopBond_PublishTest(void)
{
    struct CoopBondSave *bond = Bond_Save();
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        gCoopBondTest.xp[i] = GetMonData(mon, MON_DATA_BOND_XP);
        gCoopBondTest.flags[i] = GetMonData(mon, MON_DATA_BOND_FLAGS);
        gCoopBondTest.applied[i] = GetMonData(mon, MON_DATA_BOND_APPLIED);
        gCoopBondTest.effective[i] = CoopBond_EffectiveRank(mon);
    }
    gCoopBondTest.active = bond != NULL ? bond->active : 0;
    gCoopBondTest.pid = bond != NULL ? bond->pid : 0;
    gCoopBondTest.activeSlot = (u8)CoopBond_ActiveSlot();
    gCoopBondTest.suppressed = CoopBond_ContextSuppressed();
}

// A compact "Bond +N" or rank-up line in the map-name popup, once the field
// is quiet (never over a battle, a script, a fade or a message). Walking
// and friendship awards are not announced.
void CoopBond_FieldPoll(void)
{
    struct Pokemon *mon;
    u8 *p;

    if (sMajorWatch && gMain.callback2 == CB2_Overworld && !ScriptContext_IsEnabled())
        CoopBond_SettleMajors();
    if (sNoticeGain == 0 && sNoticeRank == 0)
        return;
    if (gMain.callback2 != CB2_Overworld || ArePlayerFieldControlsLocked() || ScriptContext_IsEnabled()
     || gPaletteFade.active)
        return;
    mon = Bond_PartyPartner();
    if (sNoticeRank != 0)
    {
        p = StringCopy(gCoopPopupText, COMPOUND_STRING("Bond: "));
        p = StringCopy(p, sRankNames[(sNoticeRank - 1) % COOP_BOND_RANK_COUNT]);
        StringCopy(p, COMPOUND_STRING("!"));
    }
    else if (mon != NULL)
    {
        // (The popup fits about 18 characters: no room for a long name.)
        p = StringCopy(gCoopPopupText, COMPOUND_STRING("Partner: Bond +"));
        ConvertIntToDecimalStringN(p, sNoticeGain, STR_CONV_MODE_LEFT_ALIGN, 4);
    }
    else
    {
        sNoticeGain = 0;
        return;
    }
    if (ShowCoopPopup())
    {
        struct CoopBondSave *bond = Bond_Save();

        sNoticeRank = 0;
        sNoticeGain = 0;
        if (bond != NULL)
            bond->noticeRank = 0;
    }
}
