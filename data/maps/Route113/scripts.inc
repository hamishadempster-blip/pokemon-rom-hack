Route113_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, Route113_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, Route113_OnTransition
	.byte 0

Route113_OnResume:
	setstepcallback STEP_CB_ASH
	end

Route113_OnTransition:
	clearflag FLAG_FORCE_MIRAGE_TOWER_VISIBLE
	call Route113_EventScript_CheckSetAshWeather
	end

Route113_EventScript_CheckSetAshWeather::
	getplayerxy VAR_TEMP_0, VAR_TEMP_1
	goto_if_lt VAR_TEMP_0, 19, Route113_EventScript_DontSetAshWeather
	goto_if_gt VAR_TEMP_0, 84, Route113_EventScript_DontSetAshWeather
	setweather WEATHER_VOLCANIC_ASH
	return

Route113_EventScript_DontSetAshWeather::
	return

Route113_EventScript_Gentleman::
	msgbox Route113_Text_AshCanBeFashionedIntoGlass, MSGBOX_NPC
	end

Route113_EventScript_NinjaBoy::
	msgbox Route113_Text_FunWalkingThroughAsh, MSGBOX_NPC
	end

Route113_EventScript_RouteSign111::
	msgbox Route113_Text_RouteSign111, MSGBOX_SIGN
	end

Route113_EventScript_RouteSignFallarbor::
	msgbox Route113_Text_RouteSignFallarbor, MSGBOX_SIGN
	end

Route113_EventScript_GlassWorkshopSign::
	msgbox Route113_Text_GlassWorkshopSign, MSGBOX_SIGN
	end

Route113_EventScript_TrainerTipsRegisterKeyItems::
	msgbox Route113_Text_TrainerTipsRegisterKeyItems, MSGBOX_SIGN
	end

Route113_EventScript_Jaylen::
	trainerbattle_single TRAINER_JAYLEN, Route113_Text_JaylenIntro, Route113_Text_JaylenDefeat
	msgbox Route113_Text_JaylenPostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Dillon::
	trainerbattle_single TRAINER_DILLON, Route113_Text_DillonIntro, Route113_Text_DillonDefeat
	msgbox Route113_Text_DillonPostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Madeline::
	trainerbattle_single TRAINER_MADELINE_1, Route113_Text_MadelineIntro, Route113_Text_MadelineDefeat, Route113_EventScript_RegisterMadeline
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, Route113_EventScript_RematchMadeline
	msgbox Route113_Text_MadelinePostBattle, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_RegisterMadeline::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox Route113_Text_MadelineRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_MADELINE_1
	release
	end

Route113_EventScript_RematchMadeline::
	trainerbattle_rematch TRAINER_MADELINE_1, Route113_Text_MadelineRematchIntro, Route113_Text_MadelineRematchDefeat
	msgbox Route113_Text_MadelinePostRematch, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Lao::
	trainerbattle_single TRAINER_LAO_1, Route113_Text_LaoIntro, Route113_Text_LaoDefeat, Route113_EventScript_RegisterLao
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, Route113_EventScript_RematchLao
	msgbox Route113_Text_LaoPostBattle, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_RegisterLao::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox Route113_Text_LaoRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_LAO_1
	release
	end

Route113_EventScript_RematchLao::
	trainerbattle_rematch TRAINER_LAO_1, Route113_Text_LaoRematchIntro, Route113_Text_LaoRematchDefeat
	msgbox Route113_Text_LaoPostRematch, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Lung::
	trainerbattle_single TRAINER_LUNG, Route113_Text_LungIntro, Route113_Text_LungDefeat
	msgbox Route113_Text_LungPostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Tori::
	trainerbattle_double TRAINER_TORI_AND_TIA, Route113_Text_ToriIntro, Route113_Text_ToriDefeat, Route113_Text_ToriNotEnoughMons
	msgbox Route113_Text_ToriPostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Tia::
	trainerbattle_double TRAINER_TORI_AND_TIA, Route113_Text_TiaIntro, Route113_Text_TiaDefeat, Route113_Text_TiaNotEnoughMons
	msgbox Route113_Text_TiaPostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Sophie::
	trainerbattle_single TRAINER_SOPHIE, Route113_Text_SophieIntro, Route113_Text_SophieDefeat
	msgbox Route113_Text_SophiePostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Coby::
	trainerbattle_single TRAINER_COBY, Route113_Text_CobyIntro, Route113_Text_CobyDefeat
	msgbox Route113_Text_CobyPostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Lawrence::
	trainerbattle_single TRAINER_LAWRENCE, Route113_Text_LawrenceIntro, Route113_Text_LawrenceDefeat
	msgbox Route113_Text_LawrencePostBattle, MSGBOX_AUTOCLOSE
	end

Route113_EventScript_Wyatt::
	trainerbattle_single TRAINER_WYATT, Route113_Text_WyattIntro, Route113_Text_WyattDefeat
	msgbox Route113_Text_WyattPostBattle, MSGBOX_AUTOCLOSE
	end

Route113_Text_AshCanBeFashionedIntoGlass:
	.string "Wahahaha! Today's technology is a\n"
	.string "wondrous thing!\p"
	.string "Take this volcanic ash here.\n"
	.string "It can be fashioned into glass.$"

Route113_Text_FunWalkingThroughAsh:
	.string "It's fun walking through the volcano's\n"
	.string "ashes on the ground and grass.\p"
	.string "You can see where you walked--it's\n"
	.string "really neat!$"

Route113_Text_RouteSign111:
	.string "ROUTE 113\n"
	.string "{RIGHT_ARROW} ROUTE 111$"

Route113_Text_RouteSignFallarbor:
	.string "ROUTE 113\n"
	.string "{LEFT_ARROW} FALLARBOR TOWN$"

Route113_Text_TrainerTipsRegisterKeyItems:
	.string "TRAINER TIPS\p"
	.string "You may register one of the KEY ITEMS\n"
	.string "in your BAG as SELECT.\p"
	.string "Simply press SELECT to use\n"
	.string "the registered item conveniently.$"

Route113_Text_GlassWorkshopSign:
	.string "GLASS WORKSHOP\n"
	.string "“Turning Volcanic Ash into Glass Items”$"


Route113_EventScript_BramblinScout::
	lock
	faceplayer
	goto_if_set FLAG_COOP_QUEST_BRAMBLIN, Route113_EventScript_BramblinScoutDone
	msgbox Route113_Text_BramblinScoutAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Route113_EventScript_BramblinScoutDecline
	msgbox Route113_Text_BramblinScoutWhich, MSGBOX_DEFAULT
	chooseboxmon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, Route113_EventScript_BramblinScoutDecline
	specialvar VAR_RESULT, ScriptGetShownMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, Route113_EventScript_BramblinScoutEgg
	goto_if_eq VAR_RESULT, SPECIES_NONE, Route113_EventScript_BramblinScoutWrong
	goto_if_eq VAR_RESULT, SPECIES_BRAMBLIN, Route113_EventScript_BramblinScoutReward
	goto Route113_EventScript_BramblinScoutWrong

Route113_EventScript_BramblinScoutReward::
	msgbox Route113_Text_BramblinScoutYes, MSGBOX_DEFAULT
	giveitem ITEM_SOFT_SAND
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_COOP_QUEST_BRAMBLIN
	msgbox Route113_Text_BramblinScoutAfter, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_BramblinScoutWrong::
	msgbox Route113_Text_BramblinScoutWrong, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_BramblinScoutEgg::
	msgbox Route113_Text_BramblinScoutEgg, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_BramblinScoutDecline::
	msgbox Route113_Text_BramblinScoutDecline, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_BramblinScoutDone::
	msgbox Route113_Text_BramblinScoutDone, MSGBOX_DEFAULT
	release
	end

Route113_Text_BramblinScoutAsk::
	.string "The ash rolls itself into\n"
	.string "a tumbleweed some days.\p"
	.string "Show me a Bramblin.\n"
	.string "I saved some Soft Sand.$"

Route113_Text_BramblinScoutWhich::
	.string "Is that the tumbleweed?$"

Route113_Text_BramblinScoutYes::
	.string "It even smells like the pass.$"

Route113_Text_BramblinScoutWrong::
	.string "That isn't a Bramblin.$"

Route113_Text_BramblinScoutEgg::
	.string "That's an Egg, not a\n"
	.string "tumble of dried brush.$"

Route113_Text_BramblinScoutDecline::
	.string "Don't breathe too deep\n"
	.string "while you search the ash.$"

Route113_Text_BramblinScoutAfter::
	.string "Route 113 glows on the\n"
	.string "POKéDEX map for Bramblin.$"

Route113_Text_BramblinScoutDone::
	.string "The glass workers hate the ash.\n"
	.string "Bramblin doesn't mind it.$"

Route113_EventScript_BramblinTrade::
	lock
	faceplayer
	goto_if_set FLAG_COOP_TRADE_BRAMBLIN, Route113_EventScript_BramblinTradeDone
	ingame_trade INGAME_TRADE_BRAMBLIN, Route113_EventScript_BramblinTradeAsk, Route113_EventScript_BramblinTradeDecline, Route113_EventScript_BramblinTradeWrong, Route113_EventScript_BramblinTradeComplete
	setflag FLAG_COOP_TRADE_BRAMBLIN
	release
	end

Route113_EventScript_BramblinTradeDecline::
	msgbox Route113_EventScript_BramblinTradeDeclineText, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_BramblinTradeWrong::
	bufferspeciesname STR_VAR_1, VAR_0x8009
	msgbox Route113_EventScript_BramblinTradeWrongText, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_BramblinTradeDone::
	msgbox Route113_EventScript_BramblinTradeDoneText, MSGBOX_DEFAULT
	release
	end

Route113_EventScript_BramblinTradeAsk::
	.string "A tumbleweed ghost rolls\n"
	.string "through the ash with Spinda.\p"
	.string "I'll trade you my {STR_VAR_2}\n"
	.string "for a {STR_VAR_1}.$"

Route113_EventScript_BramblinTradeDeclineText::
	.string "That's all right.\n"
	.string "Tumble likes the wind.$"

Route113_EventScript_BramblinTradeWrongText::
	.string "Hmm. That isn't a {STR_VAR_1}.$"

Route113_EventScript_BramblinTradeComplete::
	.string "Please be gentle with Tumble.\p"
	.string "It doesn't like to sit still.$"

Route113_EventScript_BramblinTradeDoneText::
	.string "Brambleghast is the big one.\n"
	.string "Wind carries it.$"
