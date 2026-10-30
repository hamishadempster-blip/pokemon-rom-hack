@ A hidden habitat (src/coop_habitat.c); shared scripts in data/scripts/coop.inc.
Habitat_MossHollow_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, Coop_EventScript_HabitatOnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, Coop_EventScript_HabitatOnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Coop_EventScript_HabitatFrameTable
	.byte 0

@ The entrance on PetalburgWoods (its map.json bg_events).
Habitat_MossHollow_EventScript_Entrance::
	msgbox Habitat_MossHollow_Text_Entrance, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_HabitatStay
	closemessage
	playse SE_EXIT
	warp MAP_HABITAT_MOSS_HOLLOW, 0
	waitstate
	end

Habitat_MossHollow_Text_Entrance:
	.string "Behind these trees, a narrow way\nleads deeper into the woods.\pGo through?$"
