NewMauville_Entrance_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, NewMauville_Entrance_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, NewMauville_Entrance_OnTransition
	.byte 0

NewMauville_Entrance_OnLoad:
	call_if_eq VAR_NEW_MAUVILLE_STATE, 0, NewMauville_Entrance_EventScript_CloseDoor
	end

NewMauville_Entrance_EventScript_CloseDoor::
	setmetatile 3, 0, METATILE_Facility_NewMauvilleDoor_Closed_Tile0, TRUE
	setmetatile 4, 0, METATILE_Facility_NewMauvilleDoor_Closed_Tile1, TRUE
	setmetatile 5, 0, METATILE_Facility_NewMauvilleDoor_Closed_Tile2, TRUE
	setmetatile 3, 1, METATILE_Facility_NewMauvilleDoor_Closed_Tile3, TRUE
	setmetatile 4, 1, METATILE_Facility_NewMauvilleDoor_Closed_Tile4, TRUE
	setmetatile 5, 1, METATILE_Facility_NewMauvilleDoor_Closed_Tile5, TRUE
	return

NewMauville_Entrance_OnTransition:
	setflag FLAG_LANDMARK_NEW_MAUVILLE
	end

NewMauville_Entrance_EventScript_Door::
	lockall
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox NewMauville_Entrance_Text_DoorIsLocked, MSGBOX_DEFAULT
	checkitem ITEM_BASEMENT_KEY
	goto_if_eq VAR_RESULT, FALSE, NewMauville_Entrance_EventScript_DontOpenDoor
	msgbox NewMauville_Entrance_Text_UseBasementKey, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, NewMauville_Entrance_EventScript_DontOpenDoor
	msgbox NewMauville_Entrance_Text_UsedBasementKey, MSGBOX_DEFAULT
	setmetatile 3, 0, METATILE_Facility_NewMauvilleDoor_Open_Tile0, FALSE
	setmetatile 4, 0, METATILE_Facility_NewMauvilleDoor_Open_Tile1, FALSE
	setmetatile 5, 0, METATILE_Facility_NewMauvilleDoor_Open_Tile2, FALSE
	setmetatile 3, 1, METATILE_Facility_NewMauvilleDoor_Open_Tile3, TRUE
	setmetatile 4, 1, METATILE_Facility_NewMauvilleDoor_Open_Tile4, FALSE
	setmetatile 5, 1, METATILE_Facility_NewMauvilleDoor_Open_Tile5, TRUE
	special DrawWholeMapView
	playse SE_BANG
	setvar VAR_NEW_MAUVILLE_STATE, 1
	releaseall
	end

NewMauville_Entrance_EventScript_DontOpenDoor::
	releaseall
	end

NewMauville_Entrance_Text_DoorIsLocked:
	.string "The door is locked.$"

NewMauville_Entrance_Text_UseBasementKey:
	.string "Use the BASEMENT KEY?$"

NewMauville_Entrance_Text_UsedBasementKey:
	.string "{PLAYER} used the BASEMENT KEY.\p"
	.string "The door opened!$"


@ Rotom's appliances: each save gets its own catalog here.
NewMauville_Entrance_EventScript_RotomScientist::
	lock
	faceplayer
	checkitem ITEM_ROTOM_CATALOG
	goto_if_eq VAR_RESULT, TRUE, NewMauville_Entrance_EventScript_RotomExplain
	msgbox NewMauville_Entrance_Text_RotomIntro, MSGBOX_DEFAULT
	giveitem ITEM_ROTOM_CATALOG
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
NewMauville_Entrance_EventScript_RotomExplain::
	msgbox NewMauville_Entrance_Text_RotomExplain, MSGBOX_DEFAULT
	release
	end

NewMauville_Entrance_Text_RotomIntro:
	.string "The old appliances down here are\n"
	.string "full of ROTOM. They love machines!\p"
	.string "Take this catalog of appliances.$"

NewMauville_Entrance_Text_RotomExplain:
	.string "With a ROTOM in your party, use the\n"
	.string "ROTOM CATALOG from your bag.\p"
	.string "It can slip into a heater, a washer,\n"
	.string "a fridge, a fan, or a mower, and\l"
	.string "learns that appliance's move.\p"
	.string "Pick the light bulb to bring it\n"
	.string "back to its own form.$"
