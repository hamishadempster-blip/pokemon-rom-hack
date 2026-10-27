#ifndef GUARD_COOP_LEGEND_H
#define GUARD_COOP_LEGEND_H

// Shared legendary encounters (src/coop_legend.c).

// DMSG_LEGEND sub-types (carried over the deal link).
#define COOP_LEGEND_MSG_EARN 1   // the sender caught a shared encounter: the receiver's reward
#define COOP_LEGEND_MSG_ACK  2   // the receiver's save has recorded it (result 1) or had it (2)

u16 CoopLegend_Species(u8 enc);
u8 CoopLegend_Level(u8 enc);
u8 CoopLegend_EncForSpecies(u16 species);
bool8 CoopLegend_Eligible(u8 enc);
void CoopLegend_OnSharedEnd(u8 enc, bool8 caughtHere, bool8 caughtThere, u16 ball);
void CoopLegend_HandleMsg(u8 sub, u8 enc, u16 ball, u16 result, u32 fromOtId);
void CoopLegend_OnSessionStart(void);
void CoopLegend_FieldFrame(void);
bool8 CoopLegend_DeliveryDue(bool8 *noRoomNote);
bool8 CoopLegend_NoteDue(void);

// Specials.
u16 CoopLegend_Offer(void);
u16 CoopLegend_BufferDeclined(void);
void CoopLegend_StartSolo(void);
u16 CoopLegend_AfterBattle(void);
u16 CoopLegend_TakeSharedNote(void);
u16 CoopLegend_DeliverNext(void);
void CoopLegend_WaitSaved(void);
u16 CoopLegend_TakeAckNote(void);
u16 CoopLegend_TakeAwayNote(void);
void CoopLegend_OnSharedAway(u8 enc, u8 outcome);

#endif // GUARD_COOP_LEGEND_H
