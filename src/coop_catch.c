// Catch result (collection update, section 6): a compact summary of a
// Pokemon just caught: species, shiny, a new Pokedex entry, nature, ability
// and where it went. Solo: in battle, before the stock nickname question
// (data/battle_scripts_2.s). Co-op: in the field after the shared battle,
// for each of this player's own catches (data/scripts/coop.inc). The
// CATCH RESULT setting (CO-OP > SETTINGS) turns it off.
#include "global.h"
#include "battle.h"
#include "coop.h"
#include "coop_catch.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "string_util.h"
#include "constants/characters.h"

// "Hoothoot★ – NEW in your POKéDEX!\nModest nature, Insomnia." into dst.
void CoopCatch_BufferSummary(u8 *dst, struct Pokemon *mon, bool8 isNew)
{
    u8 nature = GetMonData(mon, MON_DATA_HIDDEN_NATURE);

    dst = StringCopy(dst, GetSpeciesName(GetMonData(mon, MON_DATA_SPECIES)));
    if (IsMonShiny(mon))
        dst = StringCopy(dst, COMPOUND_STRING("{STAR}"));
    if (isNew)
        dst = StringCopy(dst, COMPOUND_STRING(": NEW in your POKéDEX!"));
    else
        dst = StringCopy(dst, COMPOUND_STRING(" is registered."));
    *dst++ = CHAR_NEWLINE;
    dst = StringCopy(dst, gNaturesInfo[nature < NUM_NATURES ? nature : 0].name);
    dst = StringCopy(dst, COMPOUND_STRING(" nature, "));
    dst = StringCopy(dst, gAbilitiesInfo[GetMonAbility(mon)].name);
    StringCopy(dst, COMPOUND_STRING("."));
}

// The solo battle script (BS_CoopCatchResult): FALSE when off; else
// gStringVar1 = the summary and gStringVar2 = where it is going (the stock
// steps that follow still do the moving, with the stock party-swap choice).
bool8 CoopCatch_BufferBattleResult(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);

    if (Coop_SettingOn(COOP_SET_NO_CATCH_RESULT) || species == SPECIES_NONE)
        return FALSE;
    CoopCatch_BufferSummary(gStringVar1, mon,
                            !GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT));
    if (CalculatePlayerPartyCount() < PARTY_SIZE)
        StringCopy(gStringVar2, COMPOUND_STRING("It will join your party."));
    else
        StringCopy(gStringVar2, COMPOUND_STRING("Your party is full: it can go to\nthe PC, or you can swap it in."));
    return TRUE;
}
