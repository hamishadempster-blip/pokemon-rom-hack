#ifndef GUARD_COOP_BOND_H
#define GUARD_COOP_BOND_H

// Trainer Bond (src/coop_bond.c, docs/COOP_BOND.md): one Partner Pokémon per
// player, chosen at the Bond Mentor in Rustboro once the Stone Badge is on
// the save. Bond XP lives in each Pokémon's own record (MON_DATA_BOND_XP),
// so every Pokémon keeps its history; the active partner's record carries
// COOP_BOND_MON_ACTIVE, and CoopSave layout 5 names it (personality and
// original trainer id). The active partner gets a flat bonus on each
// calculated stat by rank, outside the Frontier, rental and PvP battles.

struct Pokemon;
struct BoxPokemon;

enum
{
    COOP_BOND_RANK_NEW,         // New Partner
    COOP_BOND_RANK_COMPANION,   // Companion
    COOP_BOND_RANK_TRUSTED,     // Trusted Partner
    COOP_BOND_RANK_ACE,         // Ace Partner
    COOP_BOND_RANK_COUNT,
};

// Bond XP sources (caps and amounts in src/coop_bond.c).
enum
{
    COOP_BOND_SRC_KO,           // took part in a knockout
    COOP_BOND_SRC_WIN,          // a battle won
    COOP_BOND_SRC_MAJOR,        // a first major victory (one-time bit)
    COOP_BOND_SRC_DAMAGE,       // damage dealt to opponents
    COOP_BOND_SRC_CATCH,        // helped a capture
    COOP_BOND_SRC_STEPS,        // walked leading the party, healthy
    COOP_BOND_SRC_FRIENDSHIP,   // a genuine friendship gain
    COOP_BOND_SRC_SUPPORT,      // useful support in battle
    COOP_BOND_SRC_COUNT,
};

// COOP_BOND_F_* (CoopSave v5.bond.flags)
#define COOP_BOND_F_MET_MENTOR   (1 << 0)   // first conversation heard
#define COOP_BOND_F_NEWS_READ    (1 << 1)   // the "new features" note read

extern const u16 gCoopBondThresholds[COOP_BOND_RANK_COUNT];
extern const u8 gCoopBondBonus[COOP_BOND_RANK_COUNT];

u8 CoopBond_RankFromXp(u32 xp);
u32 CoopBond_MonXp(struct Pokemon *mon);
u8 CoopBond_MonRank(struct Pokemon *mon);
bool8 CoopBond_IsActivePartner(struct Pokemon *mon);
bool8 CoopBond_HasHistory(struct BoxPokemon *boxMon);
// The bond rank whose bonus this Pokémon's calculated stats should carry
// now (0: none), from its record, its owner's designation and the context.
u8 CoopBond_EffectiveRank(struct Pokemon *mon);
// The bonus is switched off here (Frontier and tents, PvP link, practice,
// duo copies): the summary says "Bonus inactive here".
bool8 CoopBond_ContextSuppressed(void);
// A Battle Frontier facility or Battle Tent map (also used by the autosave).
bool8 CoopBond_InFacilityMap(void);
// Recalculate the party where the bonus baked into its stats is not the
// one that applies now (safe points: field entry, battle start, after
// designation, rank-up, withdrawal). Never heals: see CalculateMonStatsCont.
void CoopBond_RefreshParty(void);
void CoopBond_RefreshPartyForBattle(void);
// The active partner in the party (slot), or -1 (boxed, absent or none).
s8 CoopBond_ActiveSlot(void);
// Choose / clear the designation (the mentor; never in battle or scenes).
bool8 CoopBond_Designate(u8 partySlot);
void CoopBond_ClearDesignation(void);
// The designated Pokémon left this save (traded, gifted, released).
void CoopBond_MonLeaving(struct BoxPokemon *boxMon);
// A Pokémon arrived from elsewhere (trade, gift, partner deal): it keeps
// its history but is not anyone's active partner here.
void CoopBond_MonArrived(struct BoxPokemon *boxMon);
// Award Bond XP to the active partner (if it is in the party and the
// context allows), deduplicated and capped per source.
void CoopBond_Award(u8 source, u32 amount);
// Battle hooks.
void CoopBond_BattleStart(void);
void CoopBond_BattleDamage(u8 attacker, u8 target, s32 damage);
void CoopBond_BattleKnockout(u8 target);
void CoopBond_BattleSupport(u8 battler, u16 what);
void CoopBond_BattleEnd(u8 outcome);
void CoopBond_CatchHelped(void);
// First major victories (badges, Elite Four, champion) after a won battle:
// polled in the field and called by GameClear (the champion's flag).
void CoopBond_SettleMajors(void);
void CoopBond_BattleAborted(void);
void CoopBond_FieldResume(void);
// Field hooks.
void CoopBond_OnStep(void);
void CoopBond_OnFriendshipGain(struct Pokemon *mon, s32 delta);
// Text (summary BOND page, mentor).
const u8 *CoopBond_RankName(u8 rank);
u8 *CoopBond_FormatNumber(u8 *dst, u32 value);
#define COOP_BOND_DESC_SUMMARY 0
#define COOP_BOND_DESC_MENTOR  1
#define COOP_BOND_DESC_BOXED   2
void CoopBond_DescribeMon(u8 *dst, struct Pokemon *mon, u8 mode);
void CoopBond_RankProgress(struct Pokemon *mon, u32 *done, u32 *span);
// A pending "Bond +N" / rank-up notice to show at a safe point.
bool8 CoopBond_TakeNotice(u8 *rank, u16 *gain);
void CoopBond_PublishTest(void);
u16 CoopBond_ScriptSlotIsPartner(void);
void CoopBond_FieldPoll(void);

#endif // GUARD_COOP_BOND_H
