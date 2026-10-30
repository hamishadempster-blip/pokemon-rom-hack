#ifndef GUARD_PARTY_EXP_H
#define GUARD_PARTY_EXP_H

// Whole-party experience. A Pokémon that fought is paid by the stock getexp
// flow, with its bar and level-up screen. Every other Pokémon in the party
// (the whole bench, and anyone on the field who was never sent out) gets the
// same full amount from a short record of the knockouts, once the battle is
// over, on the game that owns it: no command and no screen per Pokémon.

// Battle start, on every game.
void PartyExp_Reset(void);

// The battle engine (the link master, or a solo game): one knockout, with the
// full experience one Pokémon earns for it and who fought. In co-op the
// joining player's game hears of it over the cable.
void PartyExp_NoteKo(u16 species, u8 level, u32 baseExp, u8 ownSent, u8 partnerSent);

// Co-op, the joining player's game: a knockout message arrived.
void PartyExp_HandleMsg(const u8 *data);

// Battle over, no co-op: pay the party (before the stock evolution check).
void PartyExp_ApplySolo(void);

// Co-op, battle over (not aborted): pay the live slots 1-3 and the benched
// slots 4-6 held in the pre-battle copy. Slots are in battle order here.
struct Pokemon;
void PartyExp_ApplyCoop(struct Pokemon *benchCopy, int isGuest);

// Throw the record away (an aborted battle).
void PartyExp_Drop(void);

// Pokémon that gained a level in the last payout and are on the field
// (unit tests / harness read this).
extern u8 gPartyExpLeveled;

#endif // GUARD_PARTY_EXP_H
