NewMauville_Inside_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, NewMauville_Inside_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, NewMauville_Inside_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, NewMauville_Inside_OnLoad
	.byte 0

NewMauville_Inside_OnResume:
	call_if_eq VAR_TEMP_1, 1, NewMauville_Inside_EventScript_SetBarrierStateBlueButton
	call_if_eq VAR_TEMP_2, 1, NewMauville_Inside_EventScript_SetBarrierStateGreenButton
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, NewMauville_Inside_EventScript_TryRemoveVoltorb
	@ Power Problems: a beaten VOLT steps off the MAIN SWITCH, after a solo
	@ fight or a shared one alike.
	call_if_defeated TRAINER_PP_VOLT, NewMauville_Inside_EventScript_VoltStepAside
	special CoopQuest_DrawNewMauville
	end

NewMauville_Inside_EventScript_VoltStepAside::
	setobjectxy LOCALID_PP_VOLT, 32, 7
	setobjectxyperm LOCALID_PP_VOLT, 32, 7
	return

NewMauville_Inside_EventScript_TryRemoveVoltorb::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject VAR_LAST_TALKED
	return

NewMauville_Inside_OnTransition:
	setvar VAR_TEMP_1, 0
	setvar VAR_TEMP_2, 0
	call_if_unset FLAG_DEFEATED_VOLTORB_1_NEW_MAUVILLE, NewMauville_Inside_EventScript_ShowVoltorb1
	call_if_unset FLAG_DEFEATED_VOLTORB_2_NEW_MAUVILLE, NewMauville_Inside_EventScript_ShowVoltorb2
	call_if_unset FLAG_DEFEATED_VOLTORB_3_NEW_MAUVILLE, NewMauville_Inside_EventScript_ShowVoltorb3
	@ Power Problems: VOLT, FUSE, and the crates are here only while the
	@ generator runs on this save.
	call_if_lt VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_ShowPowerCrew
	call_if_ge VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_HidePowerCrew
	call_if_defeated TRAINER_PP_VOLT, NewMauville_Inside_EventScript_VoltAsidePerm
	end

NewMauville_Inside_EventScript_ShowPowerCrew::
	clearflag FLAG_HIDE_PP_CREW
	clearflag FLAG_HIDE_PP_SHUTTER
	return

NewMauville_Inside_EventScript_HidePowerCrew::
	setflag FLAG_HIDE_PP_CREW
	setflag FLAG_HIDE_PP_SHUTTER
	return

NewMauville_Inside_EventScript_VoltAsidePerm::
	setobjectxyperm LOCALID_PP_VOLT, 32, 7
	return

NewMauville_Inside_EventScript_ShowVoltorb1::
	clearflag FLAG_HIDE_NEW_MAUVILLE_VOLTORB_1
	return

NewMauville_Inside_EventScript_ShowVoltorb2::
	clearflag FLAG_HIDE_NEW_MAUVILLE_VOLTORB_2
	return

NewMauville_Inside_EventScript_ShowVoltorb3::
	clearflag FLAG_HIDE_NEW_MAUVILLE_VOLTORB_3
	return

NewMauville_Inside_OnLoad:
	call_if_eq VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_SetGeneratorOffMetatiles
	special CoopQuest_DrawNewMauville
	end

NewMauville_Inside_EventScript_BlueButton::
	lockall
	setvar VAR_TEMP_1, 1
	setvar VAR_TEMP_2, 0
	playse SE_PIN
	call NewMauville_Inside_EventScript_SetBarrierStateBlueButton
	special DrawWholeMapView
	releaseall
	end

NewMauville_Inside_EventScript_GreenButton::
	lockall
	setvar VAR_TEMP_1, 0
	setvar VAR_TEMP_2, 1
	playse SE_PIN
	call NewMauville_Inside_EventScript_SetBarrierStateGreenButton
	special DrawWholeMapView
	releaseall
	end

NewMauville_Inside_EventScript_SetBarrierStateBlueButton::
	setmetatile 23, 34, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 23, 35, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 23, 36, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 23, 37, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 10, 16, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 10, 17, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 10, 18, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 10, 19, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 10, 0, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 10, 1, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 10, 2, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 10, 3, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 37, 33, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 37, 34, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 37, 35, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 37, 36, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 28, 22, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 28, 23, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 28, 24, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 28, 25, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 10, 24, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 10, 25, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 10, 26, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 10, 27, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 21, 2, METATILE_BikeShop_Barrier_Green_Top, TRUE
	setmetatile 21, 3, METATILE_BikeShop_Barrier_Green_TopMid, TRUE
	setmetatile 21, 4, METATILE_BikeShop_Barrier_Green_BottomMid, TRUE
	setmetatile 21, 5, METATILE_BikeShop_Barrier_Green_Bottom, TRUE
	setmetatile 6, 11, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 13, 10, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 16, 22, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 4, 26, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 30, 38, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 2, 11, METATILE_BikeShop_Button_Green, FALSE
	setmetatile 17, 10, METATILE_BikeShop_Button_Green, FALSE
	setmetatile 25, 18, METATILE_BikeShop_Button_Green, FALSE
	setmetatile 18, 36, METATILE_BikeShop_Button_Green, FALSE
	return

NewMauville_Inside_EventScript_SetBarrierStateGreenButton::
	setmetatile 23, 34, METATILE_BikeShop_Barrier_Blue_Top, TRUE
	setmetatile 23, 35, METATILE_BikeShop_Barrier_Blue_TopMid, TRUE
	setmetatile 23, 36, METATILE_BikeShop_Barrier_Blue_BottomMid, TRUE
	setmetatile 23, 37, METATILE_BikeShop_Barrier_Blue_Bottom, TRUE
	setmetatile 10, 16, METATILE_BikeShop_Barrier_Blue_Top, TRUE
	setmetatile 10, 17, METATILE_BikeShop_Barrier_Blue_TopMid, TRUE
	setmetatile 10, 18, METATILE_BikeShop_Barrier_Blue_BottomMid, TRUE
	setmetatile 10, 19, METATILE_BikeShop_Barrier_Blue_Bottom, TRUE
	setmetatile 10, 0, METATILE_BikeShop_Barrier_Blue_Top, TRUE
	setmetatile 10, 1, METATILE_BikeShop_Barrier_Blue_TopMid, TRUE
	setmetatile 10, 2, METATILE_BikeShop_Barrier_Blue_BottomMid, TRUE
	setmetatile 10, 3, METATILE_BikeShop_Barrier_Blue_Bottom, TRUE
	setmetatile 37, 33, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 37, 34, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 37, 35, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 37, 36, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 28, 22, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 28, 23, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 28, 24, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 28, 25, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 10, 24, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 10, 25, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 10, 26, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 10, 27, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 21, 2, METATILE_BikeShop_Barrier_Hidden_Top, TRUE
	setmetatile 21, 3, METATILE_BikeShop_Barrier_Hidden_Bottom, TRUE
	setmetatile 21, 4, METATILE_BikeShop_Floor_Shadow_Top, FALSE
	setmetatile 21, 5, METATILE_BikeShop_Wall_Edge_Top, FALSE
	setmetatile 2, 11, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 17, 10, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 25, 18, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 18, 36, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 6, 11, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 13, 10, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 16, 22, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 4, 26, METATILE_BikeShop_Button_Blue, FALSE
	setmetatile 30, 38, METATILE_BikeShop_Button_Blue, FALSE
	return

@ The MAIN SWITCH works once all three breakers are reset. VOLT stands on
@ it until he is beaten.
NewMauville_Inside_EventScript_RedButton::
	lockall
	specialvar VAR_RESULT, CoopQuest_BreakerCount
	goto_if_lt VAR_RESULT, 3, NewMauville_Inside_EventScript_SwitchLocked
	msgbox NewMauville_Inside_Text_SteppedOnSwitchGeneratorStopped, MSGBOX_DEFAULT
	call NewMauville_Inside_EventScript_SetGeneratorOffMetatiles
	special CoopQuest_PowerRestored
	removeobject LOCALID_PP_VOLT
	removeobject LOCALID_PP_FUSE
	removeobject LOCALID_PP_CRATE_1
	removeobject LOCALID_PP_CRATE_2
	removeobject LOCALID_PP_CRATE_3
	special CoopQuest_DrawNewMauville
	special DrawWholeMapView
	playse SE_UNLOCK
	msgbox NewMauville_Inside_Text_ShutterOpened, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_SwitchLocked::
	msgbox NewMauville_Inside_Text_SwitchLocked, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_SetGeneratorOffMetatiles::
	setmetatile 33, 6, METATILE_BikeShop_Button_Pressed, FALSE
	setmetatile 32, 2, METATILE_BikeShop_Generator_Off_Tile0, TRUE
	setmetatile 33, 2, METATILE_BikeShop_Generator_Off_Tile1, TRUE
	setmetatile 34, 2, METATILE_BikeShop_Generator_Off_Tile2, TRUE
	setmetatile 35, 2, METATILE_BikeShop_Generator_Off_Tile3, TRUE
	setmetatile 32, 3, METATILE_BikeShop_Generator_Off_Tile4, TRUE
	setmetatile 33, 3, METATILE_BikeShop_Generator_Off_Tile5, TRUE
	setmetatile 34, 3, METATILE_BikeShop_Generator_Off_Tile6, TRUE
	setmetatile 35, 3, METATILE_BikeShop_Generator_Off_Tile7, TRUE
	special DrawWholeMapView
	return

NewMauville_Inside_EventScript_Generator::
	lockall
	goto_if_eq VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_GeneratorOff
	msgbox NewMauville_Inside_Text_GeneratorRadiatingHeat, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_GeneratorOff::
	msgbox NewMauville_Inside_Text_GeneratorQuietedDown, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_Voltorb1::
	lock
	faceplayer
	setwildbattle SPECIES_VOLTORB, 25
	waitse
	playmoncry SPECIES_VOLTORB, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, NewMauville_Inside_EventScript_DefeatedVoltorb1
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, NewMauville_Inside_EventScript_DefeatedVoltorb1
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, NewMauville_Inside_EventScript_DefeatedVoltorb1
	setflag FLAG_DEFEATED_VOLTORB_1_NEW_MAUVILLE
	release
	end

NewMauville_Inside_EventScript_DefeatedVoltorb1::
	setflag FLAG_DEFEATED_VOLTORB_1_NEW_MAUVILLE
	goto Common_EventScript_RemoveStaticPokemon
	end

NewMauville_Inside_EventScript_Voltorb2::
	lock
	faceplayer
	setwildbattle SPECIES_VOLTORB, 25
	waitse
	playmoncry SPECIES_VOLTORB, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, NewMauville_Inside_EventScript_DefeatedVoltorb2
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, NewMauville_Inside_EventScript_DefeatedVoltorb2
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, NewMauville_Inside_EventScript_DefeatedVoltorb2
	setflag FLAG_DEFEATED_VOLTORB_2_NEW_MAUVILLE
	release
	end

NewMauville_Inside_EventScript_DefeatedVoltorb2::
	setflag FLAG_DEFEATED_VOLTORB_2_NEW_MAUVILLE
	goto Common_EventScript_RemoveStaticPokemon
	end

NewMauville_Inside_EventScript_Voltorb3::
	lock
	faceplayer
	setwildbattle SPECIES_VOLTORB, 25
	waitse
	playmoncry SPECIES_VOLTORB, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, NewMauville_Inside_EventScript_DefeatedVoltorb3
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, NewMauville_Inside_EventScript_DefeatedVoltorb3
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, NewMauville_Inside_EventScript_DefeatedVoltorb3
	setflag FLAG_DEFEATED_VOLTORB_3_NEW_MAUVILLE
	release
	end

NewMauville_Inside_EventScript_DefeatedVoltorb3::
	setflag FLAG_DEFEATED_VOLTORB_3_NEW_MAUVILLE
	goto Common_EventScript_RemoveStaticPokemon
	end

NewMauville_Inside_Text_GeneratorRadiatingHeat:
	.string "The generator is radiating heat that\n"
	.string "can be felt even at a distance.\p"
	.string "It looks like it should be turned off\n"
	.string "as soon as possible.$"

NewMauville_Inside_Text_GeneratorQuietedDown:
	.string "The generator quieted down.$"

NewMauville_Inside_Text_SteppedOnSwitchGeneratorStopped:
	.string "{PLAYER} stepped on the switch.\p"
	.string "Click…\p"
	.string "… … … … … … … …\n"
	.string "… … … … … … … …\p"
	.string "The generator appears to have\n"
	.string "stopped…$"

@ ---------------------------------------------------------------------------
@ Power Problems (src/coop_quest.c). Three breakers, reset in the order the
@ maintenance log gives, let the MAIN SWITCH stop the generator.

NewMauville_Inside_EventScript_MaintenanceLog::
	lockall
	setflag FLAG_PP_LOG_READ
	msgbox NewMauville_Inside_Text_MaintenanceLog, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_BreakerStorage::
	lockall
	setvar VAR_0x8004, PP_BREAKER_STORAGE
	msgbox NewMauville_Inside_Text_BreakerStorage, MSGBOX_DEFAULT
	goto NewMauville_Inside_EventScript_Breaker

NewMauville_Inside_EventScript_BreakerService::
	lockall
	setvar VAR_0x8004, PP_BREAKER_SERVICE
	msgbox NewMauville_Inside_Text_BreakerService, MSGBOX_DEFAULT
	goto NewMauville_Inside_EventScript_Breaker

NewMauville_Inside_EventScript_BreakerControl::
	lockall
	setvar VAR_0x8004, PP_BREAKER_CONTROL
	msgbox NewMauville_Inside_Text_BreakerControl, MSGBOX_DEFAULT
	goto NewMauville_Inside_EventScript_Breaker

@ VAR_0x8004: which breaker.
NewMauville_Inside_EventScript_Breaker::
	goto_if_ge VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_BreakerPowerOff
	specialvar VAR_RESULT, CoopQuest_BreakerCount
	goto_if_gt VAR_RESULT, VAR_0x8004, NewMauville_Inside_EventScript_BreakerAlreadyOn
	msgbox NewMauville_Inside_Text_BreakerAskReset, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, NewMauville_Inside_EventScript_BreakerEnd
	specialvar VAR_RESULT, CoopQuest_PressBreaker
	goto_if_eq VAR_RESULT, PP_PRESS_TRIPPED, NewMauville_Inside_EventScript_BreakerTripped
	goto_if_ne VAR_RESULT, PP_PRESS_RESET, NewMauville_Inside_EventScript_BreakerAlreadyOn
	playse SE_SWITCH
	special CoopQuest_DrawNewMauville
	special DrawWholeMapView
	specialvar VAR_RESULT, CoopQuest_BreakerCount
	goto_if_eq VAR_RESULT, 3, NewMauville_Inside_EventScript_BreakersAllOn
	msgbox NewMauville_Inside_Text_BreakerReset, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_BreakersAllOn::
	msgbox NewMauville_Inside_Text_BreakersAllOn, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_BreakerTripped::
	playse SE_THUNDER
	special CoopQuest_DrawNewMauville
	special DrawWholeMapView
	msgbox NewMauville_Inside_Text_BreakerTripped, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_BreakerAlreadyOn::
	specialvar VAR_RESULT, CoopQuest_BreakerCount
	msgbox NewMauville_Inside_Text_BreakerAlreadyOn, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_BreakerPowerOff::
	msgbox NewMauville_Inside_Text_BreakerPowerOff, MSGBOX_DEFAULT
NewMauville_Inside_EventScript_BreakerEnd::
	releaseall
	end

@ The two wild POKéMON in the way of the first two breakers. Either way the
@ battle ends, they leave the panel.
NewMauville_Inside_EventScript_Rolycoly::
	lock
	faceplayer
	msgbox NewMauville_Inside_Text_Rolycoly, MSGBOX_DEFAULT
	setwildbattle SPECIES_ROLYCOLY, 26
	waitse
	playmoncry SPECIES_ROLYCOLY, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	setflag FLAG_PP_ROLYCOLY_DONE
	goto NewMauville_Inside_EventScript_ObstacleGone

NewMauville_Inside_EventScript_Joltik::
	lock
	faceplayer
	msgbox NewMauville_Inside_Text_Joltik, MSGBOX_DEFAULT
	setwildbattle SPECIES_JOLTIK, 26
	waitse
	playmoncry SPECIES_JOLTIK, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	setflag FLAG_PP_JOLTIK_DONE
NewMauville_Inside_EventScript_ObstacleGone::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, NewMauville_Inside_EventScript_StaticEnd
	goto Common_EventScript_RemoveStaticPokemon

NewMauville_Inside_EventScript_StaticEnd::
	release
	end

@ VOLT taps the generator for his amplifiers and stands on the MAIN SWITCH.
@ With the partner here, linked, and the Twin Beacon on, he calls FUSE and
@ both players can fight the pair together; otherwise, or on NO, he fights
@ alone. FUSE can also be fought on his own.
NewMauville_Inside_EventScript_Volt::
	lock
	faceplayer
	goto_if_defeated TRAINER_PP_VOLT, NewMauville_Inside_EventScript_VoltAfter
	specialvar VAR_RESULT, CoopQuest_DuoReady
	goto_if_eq VAR_RESULT, FALSE, NewMauville_Inside_EventScript_VoltSolo
	goto_if_defeated TRAINER_PP_FUSE, NewMauville_Inside_EventScript_VoltSolo
	special CoopDeal_BufferPartnerName
	msgbox NewMauville_Inside_Text_VoltAskDuo, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, NewMauville_Inside_EventScript_VoltDuo
	special CoopQuest_ForceSolo
NewMauville_Inside_EventScript_VoltSolo::
	trainerbattle_single TRAINER_PP_VOLT, NewMauville_Inside_Text_VoltIntro, NewMauville_Inside_Text_VoltDefeat, NewMauville_Inside_EventScript_VoltBeaten
	goto NewMauville_Inside_EventScript_VoltAfter

NewMauville_Inside_EventScript_VoltDuo::
	trainerbattle_two_trainers TRAINER_PP_VOLT, NewMauville_Inside_Text_VoltDefeat, TRAINER_PP_FUSE, NewMauville_Inside_Text_FuseDefeat
	goto NewMauville_Inside_EventScript_VoltAfter

NewMauville_Inside_EventScript_VoltBeaten::
	msgbox NewMauville_Inside_Text_VoltBeaten, MSGBOX_DEFAULT
	release
	end

NewMauville_Inside_EventScript_VoltAfter::
	msgbox NewMauville_Inside_Text_VoltAfter, MSGBOX_DEFAULT
	release
	end

NewMauville_Inside_EventScript_Fuse::
	trainerbattle_single TRAINER_PP_FUSE, NewMauville_Inside_Text_FuseIntro, NewMauville_Inside_Text_FuseDefeat
	msgbox NewMauville_Inside_Text_FuseAfter, MSGBOX_AUTOCLOSE
	end

NewMauville_Inside_EventScript_Crates::
	msgbox NewMauville_Inside_Text_Crates, MSGBOX_SIGN
	end

NewMauville_Inside_EventScript_Appliances::
	msgbox NewMauville_Inside_Text_Appliances, MSGBOX_SIGN
	end

@ This save's ROTOM, in the appliance room once the power is back. It stays
@ until it is caught.
NewMauville_Inside_EventScript_Rotom::
	lock
	faceplayer
	setwildbattle SPECIES_ROTOM, 32
	waitse
	playmoncry SPECIES_ROTOM, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, NewMauville_Inside_EventScript_RotomCaught
	msgbox NewMauville_Inside_Text_RotomFled, MSGBOX_DEFAULT
	release
	end

NewMauville_Inside_EventScript_RotomCaught::
	setflag FLAG_PP_ROTOM_DONE
	release
	end

@ The optional trail: three torn pages lead to a chamber sealed behind the
@ generator room's south wall. It opens only with the generator off.
NewMauville_Inside_EventScript_Page1::
	lockall
	goto_if_set FLAG_PP_PAGE_1, NewMauville_Inside_EventScript_PageGone
	setflag FLAG_PP_PAGE_1
	msgbox NewMauville_Inside_Text_Page1, MSGBOX_DEFAULT
	goto NewMauville_Inside_EventScript_PageCount

NewMauville_Inside_EventScript_Page2::
	lockall
	goto_if_set FLAG_PP_PAGE_2, NewMauville_Inside_EventScript_PageGone
	setflag FLAG_PP_PAGE_2
	msgbox NewMauville_Inside_Text_Page2, MSGBOX_DEFAULT
	goto NewMauville_Inside_EventScript_PageCount

NewMauville_Inside_EventScript_Page3::
	lockall
	goto_if_set FLAG_PP_PAGE_3, NewMauville_Inside_EventScript_PageGone
	setflag FLAG_PP_PAGE_3
	msgbox NewMauville_Inside_Text_Page3, MSGBOX_DEFAULT
NewMauville_Inside_EventScript_PageCount::
	specialvar VAR_RESULT, CoopQuest_PagesRead
	msgbox NewMauville_Inside_Text_PageCount, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_PageGone::
	msgbox NewMauville_Inside_Text_PageGone, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_Panel::
	lockall
	goto_if_set FLAG_PP_CHAMBER_OPEN, NewMauville_Inside_EventScript_PanelOpen
	specialvar VAR_RESULT, CoopQuest_PagesRead
	goto_if_lt VAR_RESULT, 3, NewMauville_Inside_EventScript_PanelPlain
	goto_if_lt VAR_NEW_MAUVILLE_STATE, 2, NewMauville_Inside_EventScript_PanelLive
	msgbox NewMauville_Inside_Text_PanelOpens, MSGBOX_DEFAULT
	playse SE_UNLOCK
	special CoopQuest_OpenChamber
	special DrawWholeMapView
	setvar VAR_0x8004, COOP_REWARD_PP_CHAMBER
	special CoopQuest_ClaimReward
	releaseall
	end

NewMauville_Inside_EventScript_PanelPlain::
	msgbox NewMauville_Inside_Text_PanelPlain, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_PanelLive::
	msgbox NewMauville_Inside_Text_PanelLive, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_PanelOpen::
	msgbox NewMauville_Inside_Text_PanelOpen, MSGBOX_DEFAULT
	releaseall
	end

NewMauville_Inside_EventScript_ChamberLog::
	msgbox NewMauville_Inside_Text_ChamberLog, MSGBOX_SIGN
	end

@ This save's PORYGON, left on standby in the chamber. It stays until caught.
NewMauville_Inside_EventScript_Porygon::
	lock
	faceplayer
	msgbox NewMauville_Inside_Text_Porygon, MSGBOX_DEFAULT
	setwildbattle SPECIES_PORYGON, 30
	waitse
	playmoncry SPECIES_PORYGON, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, NewMauville_Inside_EventScript_PorygonCaught
	msgbox NewMauville_Inside_Text_PorygonStandby, MSGBOX_DEFAULT
	release
	end

NewMauville_Inside_EventScript_PorygonCaught::
	setflag FLAG_PP_PORYGON_DONE
	release
	end

NewMauville_Inside_Text_SwitchLocked:
	.string "{PLAYER} stepped on the MAIN SWITCH.\p"
	.string "It won't budge. A red lamp blinks:\n"
	.string "BREAKERS RESET: {STR_VAR_1} OF 3.$"

NewMauville_Inside_Text_ShutterOpened:
	.string "With the power steady, something\n"
	.string "rattled open in the east wing.\p"
	.string "The APPLIANCE ROOM's shutter is\n"
	.string "no longer jammed!$"

NewMauville_Inside_Text_MaintenanceLog:
	.string "A MAINTENANCE LOG is pinned here.\p"
	.string "“Surges trip all three breakers.\n"
	.string "Reset them in this order:\p"
	.string "STORAGE, where the parts are kept,\n"
	.string "then SERVICE, where we fix them,\l"
	.string "then CONTROL, where we watch them.\p"
	.string "Out of order, they all trip again.\p"
	.string "With all three reset, the MAIN\n"
	.string "SWITCH by the generator shuts it\l"
	.string "down safely.”$"

NewMauville_Inside_Text_BreakerStorage:
	.string "Breaker panel: STORAGE ROOM.$"

NewMauville_Inside_Text_BreakerService:
	.string "Breaker panel: SERVICE ROOM.$"

NewMauville_Inside_Text_BreakerControl:
	.string "Breaker panel: CONTROL ROOM.$"

NewMauville_Inside_Text_BreakerAskReset:
	.string "The breaker is down. TRIPPED.\n"
	.string "Push it back up?$"

NewMauville_Inside_Text_BreakerReset:
	.string "Clunk! The breaker stayed up.\p"
	.string "Breakers reset: {STR_VAR_1} of 3.$"

NewMauville_Inside_Text_BreakersAllOn:
	.string "Clunk! The breaker stayed up.\p"
	.string "All three breakers are reset.\n"
	.string "Now for the MAIN SWITCH!$"

NewMauville_Inside_Text_BreakerTripped:
	.string "Bzzzt! The breaker snapped back\n"
	.string "down, and the others went with it!\p"
	.string "That must be the wrong order.$"

NewMauville_Inside_Text_BreakerAlreadyOn:
	.string "This breaker is already reset.\n"
	.string "Breakers reset: {STR_VAR_1} of 3.$"

NewMauville_Inside_Text_BreakerPowerOff:
	.string "The breaker is up. The generator\n"
	.string "is off, so it's quiet now.$"

NewMauville_Inside_Text_Rolycoly:
	.string "A ROLYCOLY has rolled up against\n"
	.string "the panel to soak up the heat!$"

NewMauville_Inside_Text_Joltik:
	.string "A JOLTIK is nibbling at the\n"
	.string "panel's wiring!$"

NewMauville_Inside_Text_VoltAskDuo:
	.string "VOLT: Two of you? FUSE! Get over\n"
	.string "here and back me up!\p"
	.string "Take on VOLT and FUSE together\n"
	.string "with {STR_VAR_1}?$"

NewMauville_Inside_Text_VoltIntro:
	.string "VOLT: Hey! This generator powers\n"
	.string "my amps for free!\p"
	.string "Nobody's touching that switch!$"

NewMauville_Inside_Text_VoltDefeat:
	.string "VOLT: Feedback! Total feedback!$"

NewMauville_Inside_Text_VoltBeaten:
	.string "VOLT: Fine, fine. The switch is\n"
	.string "yours. I'll find another outlet.$"

NewMauville_Inside_Text_VoltAfter:
	.string "VOLT: Go on, throw the switch.\n"
	.string "My amps can wait.$"

NewMauville_Inside_Text_FuseIntro:
	.string "FUSE: I just carry VOLT's cables.\n"
	.string "But I can still battle!$"

NewMauville_Inside_Text_FuseDefeat:
	.string "FUSE: Blown, like a fuse…$"

NewMauville_Inside_Text_FuseAfter:
	.string "FUSE: The surges were wrecking our\n"
	.string "gear anyway, honestly.$"

NewMauville_Inside_Text_Crates:
	.string "Crates are piled against a jammed\n"
	.string "shutter: APPLIANCE ROOM.\p"
	.string "Its lock flickers with each surge.\n"
	.string "Steady power might free it.$"

NewMauville_Inside_Text_Appliances:
	.string "Old appliances, still plugged in:\n"
	.string "a heater, a washer, a fridge, a fan,\l"
	.string "and a lawn mower.\p"
	.string "A ROTOM could slip into one with\n"
	.string "the ROTOM CATALOG.$"

NewMauville_Inside_Text_RotomFled:
	.string "ROTOM zipped back into the old\n"
	.string "appliances. It's still around.$"

NewMauville_Inside_Text_Page1:
	.string "A torn page is stuck behind the\n"
	.string "console…\p"
	.string "“…we sealed the old control chamber\n"
	.string "when the new generator went in…”$"

NewMauville_Inside_Text_Page2:
	.string "A torn page is tucked between the\n"
	.string "crates…\p"
	.string "“…its panel is in the south wall\n"
	.string "of the generator room, below the\l"
	.string "generator itself…”$"

NewMauville_Inside_Text_Page3:
	.string "A torn page is in the desk drawer…\p"
	.string "“…the panel's lock runs off the\n"
	.string "generator. Shut it down and the\l"
	.string "panel opens. -The old crew”$"

NewMauville_Inside_Text_PageCount:
	.string "Torn pages found: {STR_VAR_1} of 3.$"

NewMauville_Inside_Text_PageGone:
	.string "Nothing else is here.$"

NewMauville_Inside_Text_PanelPlain:
	.string "A plain wall panel, screwed shut.$"

NewMauville_Inside_Text_PanelLive:
	.string "The pages spoke of this panel.\n"
	.string "It hums with current.\p"
	.string "It won't open while the generator\n"
	.string "is running.$"

NewMauville_Inside_Text_PanelOpens:
	.string "The pages spoke of this panel.\n"
	.string "With the generator off…\p"
	.string "Click! The panel swung open!$"

NewMauville_Inside_Text_PanelOpen:
	.string "The panel hangs open.$"

NewMauville_Inside_Text_ChamberLog:
	.string "The last entry of a research log:\p"
	.string "“PORYGON prototype left on standby.\n"
	.string "Spare UP-GRADE in the cache, for\l"
	.string "whoever comes back for it.”$"

NewMauville_Inside_Text_Porygon:
	.string "A PORYGON flickers on standby\n"
	.string "beside the old console!$"

NewMauville_Inside_Text_PorygonStandby:
	.string "PORYGON went back on standby.$"
