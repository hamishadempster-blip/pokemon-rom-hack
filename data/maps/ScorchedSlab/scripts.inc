ScorchedSlab_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ScorchedSlab_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ScorchedSlab_OnLoad
	.byte 0

@ The Twin Beacon shrine passage, once the inscriptions point to it.
ScorchedSlab_OnLoad:
	call ScorchedSlab_EventScript_CheckPassage
	special CoopQuest_DrawScorchedSlab
	end

ScorchedSlab_OnTransition:
	setflag FLAG_LANDMARK_SCORCHED_SLAB
	end

