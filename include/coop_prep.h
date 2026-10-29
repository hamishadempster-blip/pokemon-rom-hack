#ifndef GUARD_COOP_PREP_H
#define GUARD_COOP_PREP_H

// Pre-battle team preparation (src/coop_prep.c): before a shared story fight
// (gym, Elite Four, champion, rival, villain, quest boss) each player picks
// up to three Pokemon from any party slot and their order; the battle uses
// them, and afterwards everyone is back in their own slot. Solo (unlinked)
// gym, Elite Four and champion fights offer the lead only.

#define COOP_PREP_MAGIC 0xC0E7

enum
{
    COOP_PREP_NONE,
    COOP_PREP_REVIEW,     // the team and READY / CHANGE TEAM / CANCEL
    COOP_PREP_PICKING,    // the party screen is open
    COOP_PREP_READY,      // waiting for the partner
    COOP_PREP_COMMIT,     // host: both ready, asked the partner to start
    COOP_PREP_COMMITTED,  // guest: started (the battle is launching)
    COOP_PREP_CANCELLED,
};

enum
{
    COOP_PREP_END_NONE,
    COOP_PREP_END_SELF,       // this player chose CANCEL
    COOP_PREP_END_PARTNER,    // the partner chose CANCEL (or stopped waiting)
    COOP_PREP_END_TIMEOUT,    // the preparation ran out of time
    COOP_PREP_END_LINK,       // the link went down
    COOP_PREP_END_INVALID,    // the chosen team changed under the choice
};

// Seconds of game clock the whole preparation may take before it is
// cancelled (never confirmed on anyone's behalf).
#define COOP_PREP_SECONDS 120

bool8 CoopPrep_Active(void);
bool8 CoopPrep_HostCommitted(void);
bool8 CoopPrep_IsLinkBlock(const u16 *words);
void CoopPrep_HandleBlock(u32 playerId, const u16 *words);
void CoopPrep_Poll(void);
void CoopPrep_BeginGuest(u16 session, u8 hostId);
void CoopPrep_OnLaunch(void);
void CoopPrep_ApplyTeam(void);
void CoopPrep_UndoTeam(void);
u8 CoopPrep_PartySlotForBattleSlot(u8 battleSlot);
void CoopPrep_PartnerWithdrew(u16 session);
void CoopPrep_LinkDown(void);
bool8 CoopPrep_BeginSolo(u16 trainerA, u16 trainerB);
u16 CoopPrep_TakeEndReason(void);

extern u8 gCoopPrepPhase;
extern u8 gCoopPrepPartnerPhase;
extern u8 gCoopPrepOrder[3];
extern u8 gCoopPrepApplied;
extern u8 gCoopPrepMaxEntries;

#endif // GUARD_COOP_PREP_H
