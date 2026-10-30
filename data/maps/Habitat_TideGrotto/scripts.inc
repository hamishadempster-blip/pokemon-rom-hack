@ A hidden habitat (src/coop_habitat.c); shared scripts in data/scripts/coop.inc.
Habitat_TideGrotto_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, Coop_EventScript_HabitatOnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, Coop_EventScript_HabitatOnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Coop_EventScript_HabitatFrameTable
	.byte 0

@ The entrance on Route109 (its map.json bg_events).
Habitat_TideGrotto_EventScript_Entrance::
	msgbox Habitat_TideGrotto_Text_Entrance, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_HabitatStay
	closemessage
	playse SE_EXIT
	warp MAP_HABITAT_TIDE_GROTTO, 0
	waitstate
	end

Habitat_TideGrotto_Text_Entrance:
	.string "The tide is pulling east, toward\nthe rocks. There may be a cove there.\pLet the current carry you?$"
