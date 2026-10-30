// Autosave (co-op update after 0xC0170141). On entering a map, once at least
// five minutes have passed since the last save of any kind, the game saves
// by itself: "Autosaving... Don't turn off the power." shows while the flash
// is written (a full save takes about four seconds of the cartridge's flash
// time, as the stock SAVE does; the game cannot move meanwhile, so it is not
// done silently or often).
//
// It never replaces the player's own save: src/save.c gives the two flash
// slots roles, and an autosave always goes to the slot that does NOT hold
// the newest manual save. If CONTINUE loads an autosave newer than the last
// manual save, the game says so on arriving and offers the manual save, so a
// softlock or a bad moment caught by an autosave can be left behind. A power cut during an
// autosave leaves the manual save (and during a manual save, the autosave).
//
// It does not save where the stock game would not, or where a save in the
// middle of something could matter: in battle, during a script, a fade or a
// menu, while moving, in a Battle Frontier facility, Battle Tent, the
// Battle Pyramid, Trainer Hill, the Safari Zone, a Cable Club room or the
// Union Room, during any co-op deal,
// invitation, team screen or shared transaction, and on a new game until
// its first manual save (the first save of a new file erases the old file's
// Hall of Fame, and must be the player's). CO-OP > SETTINGS > AUTOSAVE
// turns it off.
#include "global.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "coop.h"
#include "coop_autosave.h"
#include "coop_bond.h"
#include "coop_deal.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_specials.h"
#include "link.h"
#include "main.h"
#include "map_name_popup.h"
#include "new_game.h"
#include "overworld.h"
#include "palette.h"
#include "safari_zone.h"
#include "save.h"
#include "script.h"
#include "string_util.h"
#include "trainer_hill.h"
#include "load_save.h"
#include "event_data.h"
#include "event_scripts.h"
#include "reload_save.h"
#include "constants/battle_pyramid.h"

#define AUTOSAVE_MIN_FRAMES   (5 * 60 * 60)  // at least 5 minutes since the last save
#define AUTOSAVE_WAIT_FRAMES  600            // a map entry waits this long for a safe moment

// Test fixture: the harness may write a shorter interval here (frames);
// 0 is the real one. Nothing in the game writes it.
EWRAM_DATA u32 gCoopAutosaveTestInterval = 0;

EWRAM_DATA bool8 gCoopAutosavePending = FALSE;   // (test visibility too)
static EWRAM_DATA u16 sPendingFrames = 0;
EWRAM_DATA u8 gCoopAutosaveCount = 0;   // test visibility: autosaves written this session

void CoopAutosave_NoteMapEntry(void)
{
    gCoopAutosavePending = TRUE;
    sPendingFrames = 0;
}

// Everything but the time since the last save: a safe moment to write.
// Also the moment the FIELD NOTES pay a reward (src/coop_notes.c).
bool8 CoopField_Calm(void)
{
    if (gMain.callback2 != CB2_Overworld || gMain.inBattle)
        return FALSE;
    if (ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked() || gPaletteFade.active)
        return FALSE;
    if (gPlayerAvatar.tileTransitionState != T_NOT_MOVING)
        return FALSE;
    if (CoopDeal_Busy() || Coop_OtherInteractionBusy())
        return FALSE;
    return TRUE;
}

// An ordinary place: not where START has no SAVE (src/start_menu.c), a
// Trainer Bond facility (src/coop_bond.c) or Trainer Hill. The FIELD NOTES
// pay rewards only here too (the Battle Pyramid has its own bag).
bool8 CoopField_OrdinaryPlace(void)
{
    return !(IsOverworldLinkActive() || InUnionRoom() || GetSafariZoneFlag() || InBattlePike()
          || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InMultiPartnerRoom()
          || CoopBond_InFacilityMap() || InTrainerHill());
}

// Places and states where an autosave is never made.
static bool8 Autosave_Allowed(void)
{
    if (gFlashMemoryPresent != TRUE || gDifferentSaveFile)
        return FALSE;
    if (!Coop_SaveSchemaSupported() || Coop_SettingOn(COOP_SET_NO_AUTOSAVE))
        return FALSE;
    return CoopField_OrdinaryPlace();
}

void CoopAutosave_Poll(void)
{
    u32 interval = gCoopAutosaveTestInterval != 0 ? gCoopAutosaveTestInterval : AUTOSAVE_MIN_FRAMES;

    // CONTINUE loaded an autosave newer than the last manual save: say so
    // and offer the manual save, once, as soon as the field is calm.
    if (gSaveLoadedAutosaveOverManual)
    {
        if (!CoopField_Calm())
            return;
        gSaveLoadedAutosaveOverManual = FALSE;
        LockPlayerFieldControls();
        ScriptContext_SetupScript(Coop_EventScript_AutosaveLoaded);
        return;
    }

    if (!gCoopAutosavePending)
        return;
    if (!Autosave_Allowed() || gMain.vblankCounter1 - gSaveLastWriteVBlank < interval)
    {
        gCoopAutosavePending = FALSE;   // until the next map
        return;
    }
    if (!CoopField_Calm())
    {
        if (++sPendingFrames >= AUTOSAVE_WAIT_FRAMES)
            gCoopAutosavePending = FALSE;
        return;
    }
    gCoopAutosavePending = FALSE;
    LockPlayerFieldControls();
    ScriptContext_SetupScript(Coop_EventScript_Autosave);
}

// The script's write (data/scripts/coop.inc, Coop_EventScript_Autosave),
// once "Autosaving..." is on screen. VAR_RESULT: TRUE when saved.
void CoopAutosave_Write(void)
{
    gSaveWritingAutosave = TRUE;
    gSpecialVar_Result = (TrySavingData(SAVE_NORMAL) == SAVE_STATUS_OK);
    gSaveWritingAutosave = FALSE;
    if (gSpecialVar_Result)
        gCoopAutosaveCount++;
}

static void CB2_CoopReloadManualSave(void)
{
    gSaveLoadPreferManual = TRUE;
    ReloadSave();
}

// The script chose the last manual save (the screen has faded out): load
// it as at power-on and continue from it. The autosave stays on the flash
// until the next save of either kind replaces it (src/save.c).
void CoopAutosave_LoadManual(void)
{
    SetMainCallback2(CB2_CoopReloadManualSave);
}
