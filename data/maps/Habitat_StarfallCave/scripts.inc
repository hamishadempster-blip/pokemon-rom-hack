@ A hidden habitat (src/coop_habitat.c); shared scripts in data/scripts/coop.inc.
Habitat_StarfallCave_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, Coop_EventScript_HabitatOnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, Coop_EventScript_HabitatOnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Coop_EventScript_HabitatFrameTable
	.byte 0

@ The entrance on MossdeepCity (its map.json bg_events).
Habitat_StarfallCave_EventScript_Entrance::
	msgbox Habitat_StarfallCave_Text_Entrance, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_HabitatStay
	closemessage
	playse SE_EXIT
	warp MAP_HABITAT_STARFALL_CAVE, 0
	waitstate
	end

Habitat_StarfallCave_Text_Entrance:
	.string "A narrow path leads down the cliff\ninto the rocks.\pFollow it?$"
