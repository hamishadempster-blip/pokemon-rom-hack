// Compile with the game's ARM ABI: fixtures must use this build's save layout.
#include "global.h"
#include "pokemon_storage_system.h"
#include "battle.h"
#include "constants/moves.h"
const u32 harnessSaveLayout[] = {
    offsetof(struct SaveBlock1, vars),
    offsetof(struct SaveBlock1, flags),
    offsetof(struct SaveBlock2, encryptionKey),
    offsetof(struct PokemonStorage, boxes),
    sizeof(struct BoxPokemon),
    offsetof(struct SaveBlock1, money),
    offsetof(struct SaveBlock3, coop) + offsetof(struct CoopSave, deal),
    offsetof(struct SaveBlock3, coop) + offsetof(struct CoopSave, partnerCaught),
    offsetof(struct SaveBlock1, dexCaught),
    offsetof(struct SaveBlock3, coop) + offsetof(struct CoopSave, partnerDexValid),
    offsetof(struct BattleStruct, expGetterMonId),
    MOVE_CUT, MOVE_FLY, MOVE_SURF, MOVE_STRENGTH, MOVE_FLASH, MOVE_ROCK_SMASH, MOVE_WATERFALL, MOVE_DIVE,
    sizeof(struct BattlePokemon),
    offsetof(struct BattlePokemon, types),
    sizeof(enum Type),
    NUMBER_OF_MON_TYPES,
    offsetof(struct BattlePokemon, hp),
    sizeof(uq4_12_t),
    offsetof(struct BattlePokemon, maxHP),
    offsetof(struct SaveBlock3, coop) + offsetof(struct CoopSave, legendClaimed),
    offsetof(struct SaveBlock3, coop) + offsetof(struct CoopSave, legendPending),
    offsetof(struct SaveBlock3, coop) + offsetof(struct CoopSave, legendOwed),
    // Moves smart_moves never picks: they fail unless a condition holds.
    MOVE_BELCH, MOVE_DREAM_EATER, MOVE_SNORE, MOVE_LAST_RESORT, MOVE_STEEL_ROLLER,
};
