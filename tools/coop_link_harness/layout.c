// Compile with the game's ARM ABI: fixtures must use this build's save layout.
#include "global.h"
#include "pokemon_storage_system.h"
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
};
