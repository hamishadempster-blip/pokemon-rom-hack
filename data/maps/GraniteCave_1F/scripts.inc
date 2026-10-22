GraniteCave_1F_MapScripts::
	.byte 0

GraniteCave_1F_EventScript_Hiker::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM_FLASH, GraniteCave_1F_EventScript_ReceivedFlash
	msgbox GraniteCave_1F_Text_GetsDarkAheadHereYouGo, MSGBOX_DEFAULT
	giveitem ITEM_HM_FLASH
	setflag FLAG_RECEIVED_HM_FLASH
	msgbox GraniteCave_1F_Text_ExplainFlash, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_ReceivedFlash::
	msgbox GraniteCave_1F_Text_ExplainFlash, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_Text_GetsDarkAheadHereYouGo:
	.string "Hey, you.\n"
	.string "It gets awfully dark ahead.\l"
	.string "It'll be tough trying to explore.\p"
	.string "That guy who came by earlier…\n"
	.string "STEVEN, I think it was.\p"
	.string "He knew how to use FLASH, so he ought\n"
	.string "to be all right, but…\p"
	.string "Well, for us HIKERS, helping out those\n"
	.string "that we meet is our motto.\p"
	.string "Here you go, I'll pass this on to you.$"

GraniteCave_1F_Text_ExplainFlash:
	.string "Teach that hidden move FLASH to\n"
	.string "a POKéMON and use it.\p"
	.string "It lights up even the inky darkness\n"
	.string "of caves.\p"
	.string "But, to use it, you need the GYM BADGE\n"
	.string "from DEWFORD's POKéMON GYM.$"


GraniteCave_1F_EventScript_RockScout::
	lock
	faceplayer
	goto_if_set FLAG_COOP_QUEST_GRANITE, GraniteCave_1F_EventScript_RockScoutDone
	msgbox GraniteCave_1F_Text_RockScoutAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GraniteCave_1F_EventScript_RockScoutDecline
	msgbox GraniteCave_1F_Text_RockScoutWhich, MSGBOX_DEFAULT
	chooseboxmon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, GraniteCave_1F_EventScript_RockScoutDecline
	specialvar VAR_RESULT, ScriptGetShownMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, GraniteCave_1F_EventScript_RockScoutEgg
	goto_if_eq VAR_RESULT, SPECIES_NONE, GraniteCave_1F_EventScript_RockScoutWrong
	goto_if_eq VAR_RESULT, SPECIES_NACLI, GraniteCave_1F_EventScript_RockScoutReward
	goto_if_eq VAR_RESULT, SPECIES_GLIMMET, GraniteCave_1F_EventScript_RockScoutReward
	goto GraniteCave_1F_EventScript_RockScoutWrong

GraniteCave_1F_EventScript_RockScoutReward::
	msgbox GraniteCave_1F_Text_RockScoutYes, MSGBOX_DEFAULT
	giveitem ITEM_HARD_STONE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_COOP_QUEST_GRANITE
	msgbox GraniteCave_1F_Text_RockScoutAfter, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_RockScoutWrong::
	msgbox GraniteCave_1F_Text_RockScoutWrong, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_RockScoutEgg::
	msgbox GraniteCave_1F_Text_RockScoutEgg, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_RockScoutDecline::
	msgbox GraniteCave_1F_Text_RockScoutDecline, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_RockScoutDone::
	msgbox GraniteCave_1F_Text_RockScoutDone, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_Text_RockScoutAsk::
	.string "Nacli and Glimmet both live\n"
	.string "in this cave.\p"
	.string "Show me either one and\n"
	.string "this Hard Stone is yours.$"

GraniteCave_1F_Text_RockScoutWhich::
	.string "Which one did you bring?$"

GraniteCave_1F_Text_RockScoutYes::
	.string "That's the one.\n"
	.string "The cave has been kind to you.$"

GraniteCave_1F_Text_RockScoutWrong::
	.string "I asked for a Nacli\n"
	.string "or a Glimmet.$"

GraniteCave_1F_Text_RockScoutEgg::
	.string "I can't tell an Egg's\n"
	.string "species from the shell.$"

GraniteCave_1F_Text_RockScoutDecline::
	.string "The salt and the crystals\n"
	.string "will still be here.$"

GraniteCave_1F_Text_RockScoutAfter::
	.string "Your POKéDEX map should\n"
	.string "glow on Granite Cave now.$"

GraniteCave_1F_Text_RockScoutDone::
	.string "Steven used to study these\n"
	.string "stones. The map remembers.$"

GraniteCave_1F_EventScript_NacliTrade::
	lock
	faceplayer
	goto_if_set FLAG_COOP_TRADE_NACLI, GraniteCave_1F_EventScript_NacliTradeDone
	ingame_trade INGAME_TRADE_NACLI, GraniteCave_1F_EventScript_NacliTradeAsk, GraniteCave_1F_EventScript_NacliTradeDecline, GraniteCave_1F_EventScript_NacliTradeWrong, GraniteCave_1F_EventScript_NacliTradeComplete
	setflag FLAG_COOP_TRADE_NACLI
	release
	end

GraniteCave_1F_EventScript_NacliTradeDecline::
	msgbox GraniteCave_1F_EventScript_NacliTradeDeclineText, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_NacliTradeWrong::
	bufferspeciesname STR_VAR_1, VAR_0x8009
	msgbox GraniteCave_1F_EventScript_NacliTradeWrongText, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_NacliTradeDone::
	msgbox GraniteCave_1F_EventScript_NacliTradeDoneText, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_NacliTradeAsk::
	.string "A little salt rock sits\n"
	.string "with the Aron in this cave.\p"
	.string "I'll trade you my {STR_VAR_2}\n"
	.string "for a {STR_VAR_1}.$"

GraniteCave_1F_EventScript_NacliTradeDeclineText::
	.string "That's all right.\n"
	.string "Salt keeps.$"

GraniteCave_1F_EventScript_NacliTradeWrongText::
	.string "Hmm. That isn't a {STR_VAR_1}.$"

GraniteCave_1F_EventScript_NacliTradeComplete::
	.string "Please be gentle with Salt.\p"
	.string "It is heavier than\n"
	.string "it looks.$"

GraniteCave_1F_EventScript_NacliTradeDoneText::
	.string "Naclstack and Garganacl\n"
	.string "come later. Salt stays salt.$"
