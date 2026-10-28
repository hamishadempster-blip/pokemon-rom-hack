// Compiled with the game's ARM ABI so the audit reads this build's tables.
#include "global.h"
#include "pokemon.h"
#define O(f) offsetof(struct SpeciesInfo, f)
const u32 rosterLayout[] = {
    sizeof(struct SpeciesInfo),
    O(baseHP), O(types), O(abilities), O(speciesName), O(description),
    O(frontPic), O(backPic), O(palette), O(shinyPalette), O(iconSprite),
    O(levelUpLearnset), O(teachableLearnset), O(evolutions), O(catchRate),
    sizeof(struct Evolution), offsetof(struct Evolution, method),
    offsetof(struct Evolution, param), offsetof(struct Evolution, targetSpecies),
    sizeof(struct LevelUpMove), O(height),
#if P_FOOTPRINTS
    O(footprint),
#endif
};
