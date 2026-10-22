ShoalCave_LowTideInnerRoom_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, ShoalCave_LowTideInnerRoom_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, ShoalCave_LowTideInnerRoom_OnTransition
	.byte 0

ShoalCave_LowTideInnerRoom_OnTransition:
	goto_if_set FLAG_SYS_SHOAL_TIDE, ShoalCave_LowTideInnerRoom_EventScript_SetHighTide
	goto ShoalCave_LowTideInnerRoom_EventScript_SetLowTide

ShoalCave_LowTideInnerRoom_EventScript_SetHighTide::
	setmaplayoutindex LAYOUT_SHOAL_CAVE_HIGH_TIDE_INNER_ROOM
	end

ShoalCave_LowTideInnerRoom_EventScript_SetLowTide::
	setmaplayoutindex LAYOUT_SHOAL_CAVE_LOW_TIDE_INNER_ROOM
	end

ShoalCave_LowTideInnerRoom_OnLoad:
	call ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles
	end

ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles::
	goto_if_set FLAG_RECEIVED_SHOAL_SALT_1, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles2
	goto_if_set FLAG_SYS_SHOAL_TIDE, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles2
	setmetatile 31, 8, METATILE_Cave_ShoalCave_DirtPile_Large, TRUE
	goto ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles2
	end

ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles2::
	goto_if_set FLAG_RECEIVED_SHOAL_SALT_2, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles3
	goto_if_set FLAG_SYS_SHOAL_TIDE, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles3
	setmetatile 14, 26, METATILE_Cave_ShoalCave_DirtPile_Large, TRUE
	goto ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles3
	end

ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles3::
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_1, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles4
	setmetatile 41, 20, METATILE_Cave_ShoalCave_BlueStone_Large, TRUE
	goto ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles4
	end

ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles4::
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_2, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles5
	setmetatile 41, 10, METATILE_Cave_ShoalCave_BlueStone_Large, TRUE
	goto ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles5
	end

ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles5::
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_3, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles6
	setmetatile 6, 9, METATILE_Cave_ShoalCave_BlueStone_Large, TRUE
	goto ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles6
	end

ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatiles6::
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_4, ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatilesEnd
	setmetatile 16, 13, METATILE_Cave_ShoalCave_BlueStone_Large, TRUE
	return

ShoalCave_LowTideInnerRoom_EventScript_SetShoalItemMetatilesEnd::
	return

ShoalCave_LowTideInnerRoom_EventScript_ShoalShell1::
	lockall
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_1, ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalShell
	giveitem ITEM_SHOAL_SHELL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setmetatile 41, 20, METATILE_Cave_ShoalCave_BlueStone_Small, FALSE
	special DrawWholeMapView
	setflag FLAG_RECEIVED_SHOAL_SHELL_1
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalShell::
	msgbox ShoalCave_Text_WasShoallShellNowNothing, MSGBOX_DEFAULT
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_ShoalShell2::
	lockall
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_2, ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalShell
	giveitem ITEM_SHOAL_SHELL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setmetatile 41, 10, METATILE_Cave_ShoalCave_BlueStone_Small, FALSE
	special DrawWholeMapView
	setflag FLAG_RECEIVED_SHOAL_SHELL_2
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_ShoalShell3::
	lockall
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_3, ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalShell
	giveitem ITEM_SHOAL_SHELL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setmetatile 6, 9, METATILE_Cave_ShoalCave_BlueStone_Small, FALSE
	special DrawWholeMapView
	setflag FLAG_RECEIVED_SHOAL_SHELL_3
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_ShoalShell4::
	lockall
	goto_if_set FLAG_RECEIVED_SHOAL_SHELL_4, ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalShell
	giveitem ITEM_SHOAL_SHELL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setmetatile 16, 13, METATILE_Cave_ShoalCave_BlueStone_Small, FALSE
	special DrawWholeMapView
	setflag FLAG_RECEIVED_SHOAL_SHELL_4
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_ShoalSalt1::
	lockall
	goto_if_set FLAG_RECEIVED_SHOAL_SALT_1, ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalSalt
	giveitem ITEM_SHOAL_SALT
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setmetatile 31, 8, METATILE_Cave_ShoalCave_DirtPile_Small, FALSE
	special DrawWholeMapView
	setflag FLAG_RECEIVED_SHOAL_SALT_1
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalSalt::
	msgbox ShoalCave_Text_WasShoalSaltNowNothing, MSGBOX_DEFAULT
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_ShoalSalt2::
	lockall
	goto_if_set FLAG_RECEIVED_SHOAL_SALT_2, ShoalCave_LowTideInnerRoom_EventScript_ReceivedShoalSalt
	giveitem ITEM_SHOAL_SALT
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setmetatile 14, 26, METATILE_Cave_ShoalCave_DirtPile_Small, FALSE
	special DrawWholeMapView
	setflag FLAG_RECEIVED_SHOAL_SALT_2
	releaseall
	end

ShoalCave_LowTideInnerRoom_EventScript_IceScout::
	lock
	faceplayer
	goto_if_set FLAG_COOP_QUEST_SHOAL, ShoalCave_LowTideInnerRoom_EventScript_IceScoutDone
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ShoalCave_LowTideInnerRoom_EventScript_IceScoutDecline
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutWhich, MSGBOX_DEFAULT
	chooseboxmon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, ShoalCave_LowTideInnerRoom_EventScript_IceScoutDecline
	specialvar VAR_RESULT, ScriptGetShownMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, ShoalCave_LowTideInnerRoom_EventScript_IceScoutEgg
	goto_if_eq VAR_RESULT, SPECIES_NONE, ShoalCave_LowTideInnerRoom_EventScript_IceScoutWrong
	goto_if_eq VAR_RESULT, SPECIES_CETODDLE, ShoalCave_LowTideInnerRoom_EventScript_IceScoutReward
	goto_if_eq VAR_RESULT, SPECIES_ABOMASNOW, ShoalCave_LowTideInnerRoom_EventScript_IceScoutReward
	goto ShoalCave_LowTideInnerRoom_EventScript_IceScoutWrong

ShoalCave_LowTideInnerRoom_EventScript_IceScoutReward::
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutYes, MSGBOX_DEFAULT
	giveitem ITEM_NEVER_MELT_ICE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_COOP_QUEST_SHOAL
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutAfter, MSGBOX_DEFAULT
	release
	end

ShoalCave_LowTideInnerRoom_EventScript_IceScoutWrong::
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutWrong, MSGBOX_DEFAULT
	release
	end

ShoalCave_LowTideInnerRoom_EventScript_IceScoutEgg::
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutEgg, MSGBOX_DEFAULT
	release
	end

ShoalCave_LowTideInnerRoom_EventScript_IceScoutDecline::
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutDecline, MSGBOX_DEFAULT
	release
	end

ShoalCave_LowTideInnerRoom_EventScript_IceScoutDone::
	msgbox ShoalCave_LowTideInnerRoom_Text_IceScoutDone, MSGBOX_DEFAULT
	release
	end

ShoalCave_LowTideInnerRoom_Text_IceScoutAsk::
	.string "The inner cave stays cold\n"
	.string "even when the tide is out.\p"
	.string "Show me a Cetoddle or\n"
	.string "an Abomasnow.\p"
	.string "This ice never melts.$"

ShoalCave_LowTideInnerRoom_Text_IceScoutWhich::
	.string "Which one braved the cold?$"

ShoalCave_LowTideInnerRoom_Text_IceScoutYes::
	.string "Your breath fogs the same way.$"

ShoalCave_LowTideInnerRoom_Text_IceScoutWrong::
	.string "I need a Cetoddle\n"
	.string "or an Abomasnow.$"

ShoalCave_LowTideInnerRoom_Text_IceScoutEgg::
	.string "Don't leave an Egg\n"
	.string "in a cave this cold.$"

ShoalCave_LowTideInnerRoom_Text_IceScoutDecline::
	.string "Low tide won't last.\n"
	.string "Come through while you can.$"

ShoalCave_LowTideInnerRoom_Text_IceScoutAfter::
	.string "Shoal Cave glows on\n"
	.string "the POKéDEX map.$"

ShoalCave_LowTideInnerRoom_Text_IceScoutDone::
	.string "Snover root in the same ice.\n"
	.string "The map knows the room.$"
