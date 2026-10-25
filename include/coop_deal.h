#ifndef GUARD_COOP_DEAL_H
#define GUARD_COOP_DEAL_H

// Partner interaction: friendly battles, trades and gifts between two linked
// saves. See src/coop_deal.c for the protocol and its recovery rules.

#define COOP_DEAL_MAGIC 0xC0D1

struct Pokemon;
struct CoopSave;

extern bool8 gCoopFriendlyBattle;
extern u8 gCoopFriendlyRules;
extern bool8 gCoopPracticeBattle;
extern u8 gCoopBattleSpeed;  // COOP_SPEED_* in force for the current battle

// Called by coop.c.
void CoopDeal_HandleBlock(u32 playerId, const u16 *words);
void CoopDeal_Poll(void);
void CoopDeal_OnSessionStart(void);
void CoopDeal_SendQuest(u8 quest, u8 value, bool8 live);
void CoopDeal_QueueLook(void);
bool8 CoopDeal_Busy(void);
bool8 CoopDeal_ItemGiftable(u16 item);
void CoopDeal_AbortFriendlyBattle(void);
void CoopDeal_FieldFrame(void);
void CoopDex_HandleBlock(u32 playerId, const u16 *words);
bool8 CoopDex_PartnerCaught(u16 nationalNum);
u16 CoopDex_BufferSummary(void);
u8 Coop_LocalBattleSpeed(void);
u8 Coop_BattleSpeed(void);
u16 CoopDeal_RequestPartnerTeam(void);
u8 CoopDeal_PartnerTeamReady(void);
void CoopDeal_CopyPartnerTeamMon(u8 index, struct Pokemon *dest);

// Practice battles (src/coop_practice.c).
u16 CoopPractice_AskPartnerTeam(void);
void CoopPractice_WaitPartnerTeam(void);
u16 CoopPractice_Prepare(void);
void CoopPractice_Start(void);
u16 CoopPractice_Outcome(void);
#define COOP_DEX_MAGIC 0xC0DE

// Called by coop.c, implemented there.
struct CoopSave *Coop_GetSave(void);
u16 Coop_PcFreeSlots(void);
bool8 Coop_DealPartnerHere(void);
bool8 Coop_OtherInteractionBusy(void);

// Team picker (party_menu.c).
bool8 CoopDeal_TeamClauses(void);

// Specials (data/scripts/coop_deal.inc).
u16 CoopDeal_CanStart(void);
u16 CoopDeal_BufferPartnerName(void);
u16 CoopDeal_CheckOfferMon(void);
u16 CoopDeal_CheckGiftItem(void);
u16 CoopDeal_BufferItemCount(void);
u16 CoopDeal_StartRequest(void);
void CoopDeal_Wait(void);
u16 CoopDeal_BufferIncoming(void);
u16 CoopDeal_BufferPartnerOffer(void);
u16 CoopDeal_BufferMyOffer(void);
u16 CoopDeal_Answer(void);
u16 CoopDeal_Confirm(void);
u16 CoopDeal_Prepare(void);
u16 CoopDeal_Commit(void);
u16 CoopDeal_TryTradeEvolution(void);
u16 CoopDeal_SaveAfterEvolution(void);
u16 CoopDeal_BufferRules(void);
u16 CoopDeal_TeamNeedsPicker(void);
void CoopDeal_OpenTeamPicker(void);
u16 CoopDeal_ValidateTeam(void);
u16 CoopDeal_TeamReady(void);
void CoopDeal_StartFriendlyBattle(void);
u16 CoopDeal_BattleOutcome(void);
u16 CoopDeal_Cancel(void);
u16 CoopDeal_BufferPending(void);
void CoopDeal_OpenGiftBag(void);
u16 CoopDeal_Settle(void);

#endif // GUARD_COOP_DEAL_H
