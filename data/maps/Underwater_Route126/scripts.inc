Underwater_Route126_MapScripts::
	.byte 0


Underwater_Route126_EventScript_WiglettScout::
	lock
	faceplayer
	goto_if_set FLAG_COOP_QUEST_WIGLETT, Underwater_Route126_EventScript_WiglettScoutDone
	msgbox Underwater_Route126_EventScript_WiglettScoutAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Underwater_Route126_EventScript_WiglettScoutDecline
	msgbox Underwater_Route126_EventScript_WiglettScoutWhich, MSGBOX_DEFAULT
	chooseboxmon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, Underwater_Route126_EventScript_WiglettScoutDecline
	specialvar VAR_RESULT, ScriptGetShownMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, Underwater_Route126_EventScript_WiglettScoutEgg
	goto_if_eq VAR_RESULT, SPECIES_NONE, Underwater_Route126_EventScript_WiglettScoutWrong
	goto_if_eq VAR_RESULT, SPECIES_WIGLETT, Underwater_Route126_EventScript_WiglettScoutReward
	goto Underwater_Route126_EventScript_WiglettScoutWrong

Underwater_Route126_EventScript_WiglettScoutReward::
	msgbox Underwater_Route126_EventScript_WiglettScoutYes, MSGBOX_DEFAULT
	giveitem ITEM_DIVE_BALL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_COOP_QUEST_WIGLETT
	msgbox Underwater_Route126_EventScript_WiglettScoutAfter, MSGBOX_DEFAULT
	release
	end

Underwater_Route126_EventScript_WiglettScoutWrong::
	msgbox Underwater_Route126_EventScript_WiglettScoutWrongText, MSGBOX_DEFAULT
	release
	end

Underwater_Route126_EventScript_WiglettScoutEgg::
	msgbox Underwater_Route126_EventScript_WiglettScoutEggText, MSGBOX_DEFAULT
	release
	end

Underwater_Route126_EventScript_WiglettScoutDecline::
	msgbox Underwater_Route126_EventScript_WiglettScoutDeclineText, MSGBOX_DEFAULT
	release
	end

Underwater_Route126_EventScript_WiglettScoutDone::
	msgbox Underwater_Route126_EventScript_WiglettScoutDoneText, MSGBOX_DEFAULT
	release
	end

Underwater_Route126_EventScript_WiglettScoutAsk::
	.string "A garden eel hides in\n"
	.string "the sand down here.\p"
	.string "Show me a Wiglett and\n"
	.string "this Dive Ball is yours.$"

Underwater_Route126_EventScript_WiglettScoutWhich::
	.string "Which one did you bring?$"

Underwater_Route126_EventScript_WiglettScoutYes::
	.string "It ducked, then came back up.$"

Underwater_Route126_EventScript_WiglettScoutWrongText::
	.string "I asked for a Wiglett.$"

Underwater_Route126_EventScript_WiglettScoutEggText::
	.string "I can't tell an Egg's\n"
	.string "species from the shell.$"

Underwater_Route126_EventScript_WiglettScoutDeclineText::
	.string "The sand down here is\n"
	.string "full of them.$"

Underwater_Route126_EventScript_WiglettScoutAfter::
	.string "Dive Balls like POKéMON\n"
	.string "met under the waves.$"

Underwater_Route126_EventScript_WiglettScoutDoneText::
	.string "Wugtrio is three eels\n"
	.string "in one hole.$"
