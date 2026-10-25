@ The Twin Beacon shrine (behind the Scorched Slab). Scripts are in
@ data/scripts/coop_quest_beacon.inc.
TwinBeaconShrine_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, TwinBeaconShrine_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, TwinBeaconShrine_OnTransition
	.byte 0

TwinBeaconShrine_OnLoad:
	call_if_unset FLAG_SHRINE_SOLVED, TwinBeaconShrine_EventScript_SealSanctum
	end

TwinBeaconShrine_OnTransition:
	setflag FLAG_LANDMARK_SCORCHED_SLAB
	setvar VAR_TEMP_5, 0
	setvar VAR_SHRINE_TOURNEY, 0
	end

TwinBeaconShrine_EventScript_SealSanctum::
	setmetatile 7, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 8, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 9, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 9, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	return
