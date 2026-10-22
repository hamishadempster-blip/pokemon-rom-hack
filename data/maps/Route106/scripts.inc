Route106_MapScripts::
	.byte 0

Route106_EventScript_TrainerTipsSign::
	msgbox Route106_Text_TrainerTips, MSGBOX_SIGN
	end

Route106_EventScript_Douglas::
	trainerbattle_single TRAINER_DOUGLAS, Route106_Text_DouglasIntro, Route106_Text_DouglasDefeated
	msgbox Route106_Text_DouglasPostBattle, MSGBOX_AUTOCLOSE
	end

Route106_EventScript_Kyla::
	trainerbattle_single TRAINER_KYLA, Route106_Text_KylaIntro, Route106_Text_KylaDefeated
	msgbox Route106_Text_KylaPostBattle, MSGBOX_AUTOCLOSE
	end

Route106_EventScript_Elliot::
	trainerbattle_single TRAINER_ELLIOT_1, Route106_Text_ElliotIntro, Route106_Text_ElliotDefeated, Route106_EventScript_ElliotRegisterMatchCallAfterBattle
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, Route106_EventScript_ElliotRematch
	msgbox Route106_Text_ElliotPostBattle, MSGBOX_DEFAULT
	release
	end

Route106_EventScript_ElliotRegisterMatchCallAfterBattle::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox Route106_Text_ElliotRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_ELLIOT_1
	release
	end

Route106_EventScript_ElliotRematch::
	trainerbattle_rematch TRAINER_ELLIOT_1, Route106_Text_ElliotRematchIntro, Route106_Text_ElliotRematchDefeated
	msgbox Route106_Text_ElliotRematchPostBattle, MSGBOX_AUTOCLOSE
	end

Route106_EventScript_Ned::
	trainerbattle_single TRAINER_NED, Route106_Text_NedIntro, Route106_Text_NedDefeated
	msgbox Route106_Text_NedPostBattle, MSGBOX_AUTOCLOSE
	end

Route106_Text_TrainerTips:
	.string "TRAINER TIPS\p"
	.string "Advice on catching POKéMON with a ROD:\n"
	.string "Press the A Button if you get a bite.$"


Route106_EventScript_CrabScout::
	lock
	faceplayer
	goto_if_set FLAG_COOP_QUEST_CRABRAWLER, Route106_EventScript_CrabScoutDone
	msgbox Route106_EventScript_CrabScoutAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Route106_EventScript_CrabScoutDecline
	msgbox Route106_EventScript_CrabScoutWhich, MSGBOX_DEFAULT
	chooseboxmon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, Route106_EventScript_CrabScoutDecline
	specialvar VAR_RESULT, ScriptGetShownMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, Route106_EventScript_CrabScoutEgg
	goto_if_eq VAR_RESULT, SPECIES_NONE, Route106_EventScript_CrabScoutWrong
	goto_if_eq VAR_RESULT, SPECIES_CRABRAWLER, Route106_EventScript_CrabScoutReward
	goto Route106_EventScript_CrabScoutWrong

Route106_EventScript_CrabScoutReward::
	msgbox Route106_EventScript_CrabScoutYes, MSGBOX_DEFAULT
	giveitem ITEM_PROTECTIVE_PADS
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_COOP_QUEST_CRABRAWLER
	msgbox Route106_EventScript_CrabScoutAfter, MSGBOX_DEFAULT
	release
	end

Route106_EventScript_CrabScoutWrong::
	msgbox Route106_EventScript_CrabScoutWrongText, MSGBOX_DEFAULT
	release
	end

Route106_EventScript_CrabScoutEgg::
	msgbox Route106_EventScript_CrabScoutEggText, MSGBOX_DEFAULT
	release
	end

Route106_EventScript_CrabScoutDecline::
	msgbox Route106_EventScript_CrabScoutDeclineText, MSGBOX_DEFAULT
	release
	end

Route106_EventScript_CrabScoutDone::
	msgbox Route106_EventScript_CrabScoutDoneText, MSGBOX_DEFAULT
	release
	end

Route106_EventScript_CrabScoutAsk::
	.string "A boxing crab swims this\n"
	.string "strait between the waves.\p"
	.string "Show me a Crabrawler and\n"
	.string "these pads are yours.$"

Route106_EventScript_CrabScoutWhich::
	.string "Which one did you bring?$"

Route106_EventScript_CrabScoutYes::
	.string "It tried to punch the pads.\n"
	.string "They held.$"

Route106_EventScript_CrabScoutWrongText::
	.string "I asked for a Crabrawler.$"

Route106_EventScript_CrabScoutEggText::
	.string "I can't tell an Egg's\n"
	.string "species from the shell.$"

Route106_EventScript_CrabScoutDeclineText::
	.string "Surf the strait if you\n"
	.string "want to find one.$"

Route106_EventScript_CrabScoutAfter::
	.string "Crabominable waits somewhere\n"
	.string "cold. The pads will do for now.$"

Route106_EventScript_CrabScoutDoneText::
	.string "The strait stays busy.\n"
	.string "Surf if you want another.$"
