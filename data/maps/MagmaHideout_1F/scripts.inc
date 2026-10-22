MagmaHideout_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MagmaHideout_1F_OnTransition
	.byte 0

MagmaHideout_1F_OnTransition:
	setvar VAR_JAGGED_PASS_ASH_WEATHER, 0
	end

MagmaHideout_1F_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_1, MagmaHideout_1F_Text_Grunt1Intro, MagmaHideout_1F_Text_Grunt1Defeat
	msgbox MagmaHideout_1F_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_1F_EventScript_Grunt2::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_2, MagmaHideout_1F_Text_Grunt2Intro, MagmaHideout_1F_Text_Grunt2Defeat
	msgbox MagmaHideout_1F_Text_Grunt2PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_1F_Text_Grunt1Intro:
	.string "When TEAM MAGMA has roll call, we get\n"
	.string "important guarding assignments in\l"
	.string "the order that we line up.\p"
	.string "That's why I'm stuck off in this corner.\n"
	.string "I'm always late to roll call!$"

MagmaHideout_1F_Text_Grunt1Defeat:
	.string "I'm always late for training sessions,\n"
	.string "too!\p"
	.string "I hate to say it, but I'm wimpy…$"

MagmaHideout_1F_Text_Grunt1PostBattle:
	.string "Okay, I'll try to put a little more\n"
	.string "effort into things from now on…$"

MagmaHideout_1F_Text_Grunt2Intro:
	.string "Our leader told us to dig into\n"
	.string "MT. CHIMNEY, so we dug and dug.\p"
	.string "And in the course of digging, we came\n"
	.string "across something that blew our minds!\p"
	.string "What did we find?\p"
	.string "Fuhahaha!\n"
	.string "I'll tell you if you beat me!$"

MagmaHideout_1F_Text_Grunt2Defeat:
	.string "Arrgh!\n"
	.string "Taken down!$"

MagmaHideout_1F_Text_Grunt2PostBattle:
	.string "I won't tell you after all.\n"
	.string "You'll find out when you get there!\p"
	.string "It'd be better if you saved surprises\n"
	.string "to the end, don't you think?$"


MagmaHideout_1F_EventScript_VaroomScout::
	lock
	faceplayer
	goto_if_set FLAG_COOP_QUEST_VAROOM, MagmaHideout_1F_EventScript_VaroomScoutDone
	msgbox MagmaHideout_1F_EventScript_VaroomScoutAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MagmaHideout_1F_EventScript_VaroomScoutDecline
	msgbox MagmaHideout_1F_EventScript_VaroomScoutWhich, MSGBOX_DEFAULT
	chooseboxmon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, MagmaHideout_1F_EventScript_VaroomScoutDecline
	specialvar VAR_RESULT, ScriptGetShownMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, MagmaHideout_1F_EventScript_VaroomScoutEgg
	goto_if_eq VAR_RESULT, SPECIES_NONE, MagmaHideout_1F_EventScript_VaroomScoutWrong
	goto_if_eq VAR_RESULT, SPECIES_VAROOM, MagmaHideout_1F_EventScript_VaroomScoutReward
	goto MagmaHideout_1F_EventScript_VaroomScoutWrong

MagmaHideout_1F_EventScript_VaroomScoutReward::
	msgbox MagmaHideout_1F_EventScript_VaroomScoutYes, MSGBOX_DEFAULT
	giveitem ITEM_AIR_BALLOON
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_COOP_QUEST_VAROOM
	msgbox MagmaHideout_1F_EventScript_VaroomScoutAfter, MSGBOX_DEFAULT
	release
	end

MagmaHideout_1F_EventScript_VaroomScoutWrong::
	msgbox MagmaHideout_1F_EventScript_VaroomScoutWrongText, MSGBOX_DEFAULT
	release
	end

MagmaHideout_1F_EventScript_VaroomScoutEgg::
	msgbox MagmaHideout_1F_EventScript_VaroomScoutEggText, MSGBOX_DEFAULT
	release
	end

MagmaHideout_1F_EventScript_VaroomScoutDecline::
	msgbox MagmaHideout_1F_EventScript_VaroomScoutDeclineText, MSGBOX_DEFAULT
	release
	end

MagmaHideout_1F_EventScript_VaroomScoutDone::
	msgbox MagmaHideout_1F_EventScript_VaroomScoutDoneText, MSGBOX_DEFAULT
	release
	end

MagmaHideout_1F_EventScript_VaroomScoutAsk::
	.string "A steel engine has started\n"
	.string "rolling these tunnels.\p"
	.string "Show me a Varoom and\n"
	.string "this Air Balloon is yours.$"

MagmaHideout_1F_EventScript_VaroomScoutWhich::
	.string "Which one did you bring?$"

MagmaHideout_1F_EventScript_VaroomScoutYes::
	.string "It revved at the balloon.$"

MagmaHideout_1F_EventScript_VaroomScoutWrongText::
	.string "I asked for a Varoom.$"

MagmaHideout_1F_EventScript_VaroomScoutEggText::
	.string "I can't tell an Egg's\n"
	.string "species from the shell.$"

MagmaHideout_1F_EventScript_VaroomScoutDeclineText::
	.string "The tunnels rumble either\n"
	.string "way. I'll hold the balloon.$"

MagmaHideout_1F_EventScript_VaroomScoutAfter::
	.string "The balloon pops on the\n"
	.string "first hit. Until then, the\n"
	.string "ground can't reach you.$"

MagmaHideout_1F_EventScript_VaroomScoutDoneText::
	.string "Revavroom is louder.\n"
	.string "You'll hear it coming.$"
