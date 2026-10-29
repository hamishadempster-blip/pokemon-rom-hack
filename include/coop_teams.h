#ifndef GUARD_COOP_TEAMS_H
#define GUARD_COOP_TEAMS_H

// Progress tiers (src/coop_teams.c): 0 before badge 1, 1-8 badges,
// 9 League admission, 10 Champion. Level caps 18/24/30/36/42/48/54/60/66/76/80.
#define COOP_TIER_LEAGUE   9
#define COOP_TIER_CHAMPION 10
#define COOP_TIER_COUNT    11

// Team key carried by co-op invites and their answers.
#define COOP_TEAMKEY_TIER_MASK   0x000F
#define COOP_TEAMKEY_CASE_SHIFT  4
#define COOP_TEAMKEY_VALID       0x0080
#define COOP_TEAMKEY_BRACELET    0x0040   // the sender owns a Mega Bracelet

extern u16 gCoopBattleTeamKey;
extern u8 gCoopPartnerHasBracelet;
extern u8 gCoopMegaRejected;
extern u16 gCoopTeamLastBase;
extern u16 gCoopTeamLastParty;
extern u16 gCoopTeamLastKey;

u8 Coop_ProgressTier(void);
u8 Coop_TierLevelCap(u8 tier);
u16 Coop_TeamKeyLocal(void);
u16 Coop_TeamKeyMerge(u16 inviter, u16 guest);
void Coop_SetBattleTeamKey(u16 key);
u16 Coop_TeamPartyIdForKey(u16 trainerId, u16 key);
u16 Coop_TeamPartyId(u16 trainerId);
void Coop_ClampTeamLevels(struct Pokemon *party, u16 partyId);

#endif // GUARD_COOP_TEAMS_H
