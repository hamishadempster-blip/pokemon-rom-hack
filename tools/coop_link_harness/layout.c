// Compile with the game's ARM ABI: fixtures must use this build's save layout.
#include "global.h"
const u32 harnessSaveLayout[] = {
    offsetof(struct SaveBlock1, vars),
    offsetof(struct SaveBlock1, flags),
};
