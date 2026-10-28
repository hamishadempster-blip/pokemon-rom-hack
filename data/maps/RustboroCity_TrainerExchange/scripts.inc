@ The Trainer Exchange (src/coop_exchange.c, stock in
@ tools/coop_exchange/stock.py). Five counters; each sells or teaches what
@ THIS save has unlocked with its own badges. VAR_TEMP_1 keeps the counter,
@ VAR_TEMP_2 a price across menus that may change the special variables.

RustboroCity_TrainerExchange_MapScripts::
	.byte 0

@ ---------------------------------------------------------------- common

RustboroCity_TrainerExchange_EventScript_ShopMenu::
	message RustboroCity_TrainerExchange_Text_HowCanIHelp
	waitmessage
	dynmultichoice 18, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, RustboroCity_TrainerExchange_Text_OptBuy, RustboroCity_TrainerExchange_Text_OptSoon, RustboroCity_TrainerExchange_Text_OptBack
	switch VAR_RESULT
	case 0, RustboroCity_TrainerExchange_EventScript_Buy
	case 1, RustboroCity_TrainerExchange_EventScript_Soon
	goto RustboroCity_TrainerExchange_EventScript_Bye
	end

RustboroCity_TrainerExchange_EventScript_Buy::
	copyvar VAR_0x8004, VAR_TEMP_1
	specialvar VAR_RESULT, CoopExchange_CountStock
	goto_if_eq VAR_RESULT, 0, RustboroCity_TrainerExchange_EventScript_NothingYet
	copyvar VAR_0x8004, VAR_TEMP_1
	special CoopExchange_OpenShop
	waitstate
	goto RustboroCity_TrainerExchange_EventScript_BackToMenu
	end

RustboroCity_TrainerExchange_EventScript_NothingYet::
	msgbox RustboroCity_TrainerExchange_Text_NothingYet, MSGBOX_DEFAULT
RustboroCity_TrainerExchange_EventScript_Soon::
	copyvar VAR_0x8004, VAR_TEMP_1
	specialvar VAR_RESULT, CoopExchange_BufferUpcoming
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_BackToMenu
	end

@ Back to the counter's own menu.
RustboroCity_TrainerExchange_EventScript_BackToMenu::
	goto_if_eq VAR_TEMP_1, COOP_EXCHANGE_TM, RustboroCity_TrainerExchange_EventScript_MovesMenu
	goto_if_eq VAR_TEMP_1, COOP_EXCHANGE_TRAIN, RustboroCity_TrainerExchange_EventScript_TrainMenu
	goto_if_eq VAR_TEMP_1, COOP_EXCHANGE_BREED, RustboroCity_TrainerExchange_EventScript_BreedMenu
	goto RustboroCity_TrainerExchange_EventScript_ShopMenu
	end

RustboroCity_TrainerExchange_EventScript_Bye::
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

RustboroCity_TrainerExchange_EventScript_TooPoor::
	msgbox RustboroCity_TrainerExchange_Text_TooPoor, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_BackToMenu
	end

@ ---------------------------------------------------------------- 1. moves

RustboroCity_TrainerExchange_EventScript_MovesClerk::
	lock
	faceplayer
	setvar VAR_TEMP_1, COOP_EXCHANGE_TM
	msgbox RustboroCity_TrainerExchange_Text_MovesIntro, MSGBOX_DEFAULT
RustboroCity_TrainerExchange_EventScript_MovesMenu::
	message RustboroCity_TrainerExchange_Text_HowCanIHelp
	waitmessage
	dynmultichoice 16, 1, FALSE, 6, 0, DYN_MULTICHOICE_CB_NONE, RustboroCity_TrainerExchange_Text_OptBuyTm, RustboroCity_TrainerExchange_Text_OptTeach, RustboroCity_TrainerExchange_Text_OptRelearn, RustboroCity_TrainerExchange_Text_OptEggMove, RustboroCity_TrainerExchange_Text_OptSoon, RustboroCity_TrainerExchange_Text_OptBack
	switch VAR_RESULT
	case 0, RustboroCity_TrainerExchange_EventScript_Buy
	case 1, RustboroCity_TrainerExchange_EventScript_TeachType
	case 2, RustboroCity_TrainerExchange_EventScript_Relearn
	case 3, RustboroCity_TrainerExchange_EventScript_EggMove
	case 4, RustboroCity_TrainerExchange_EventScript_Soon
	goto RustboroCity_TrainerExchange_EventScript_Bye
	end

RustboroCity_TrainerExchange_EventScript_TeachType::
	specialvar VAR_RESULT, CoopExchange_PushTutorTypes
	goto_if_eq VAR_RESULT, 0, RustboroCity_TrainerExchange_EventScript_MovesMenu
	message RustboroCity_TrainerExchange_Text_WhichType
	waitmessage
	dynmultistack 1, 1, FALSE, 7, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, RustboroCity_TrainerExchange_EventScript_MovesMenu
	copyvar VAR_TEMP_3, VAR_RESULT
RustboroCity_TrainerExchange_EventScript_TeachMove::
	copyvar VAR_0x8004, VAR_TEMP_3
	specialvar VAR_RESULT, CoopExchange_PushTutorMoves
	message RustboroCity_TrainerExchange_Text_WhichMove
	waitmessage
	dynmultistack 1, 1, FALSE, 7, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, RustboroCity_TrainerExchange_EventScript_TeachType
	copyvar VAR_0x8004, VAR_RESULT
	specialvar VAR_RESULT, CoopExchange_SelectTutorMove
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_TooPoor
	copyvar VAR_TEMP_2, VAR_0x8006
	msgbox RustboroCity_TrainerExchange_Text_TeachForPrice, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RustboroCity_TrainerExchange_EventScript_TeachMove
	msgbox RustboroCity_TrainerExchange_Text_TeachWhichMon, MSGBOX_DEFAULT
	call MoveTutor_EventScript_OpenBox
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_NotTaught
	copyvar VAR_0x8006, VAR_TEMP_2
	specialvar VAR_RESULT, CoopExchange_Charge
	playse SE_SHOP
	msgbox RustboroCity_TrainerExchange_Text_Taught, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_MovesMenu
	end

RustboroCity_TrainerExchange_EventScript_NotTaught::
	msgbox RustboroCity_TrainerExchange_Text_NoCharge, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_MovesMenu
	end

@ Level-up and evolution moves it could have learned by now.
RustboroCity_TrainerExchange_EventScript_Relearn::
	setvar VAR_0x8004, 1
	specialvar VAR_RESULT, CoopExchange_SetPrice
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_TooPoor
	copyvar VAR_TEMP_2, VAR_0x8006
	msgbox RustboroCity_TrainerExchange_Text_RelearnWhichMon, MSGBOX_DEFAULT
	setmoverelearnerstate MOVE_RELEARNER_LEVEL_UP_MOVES
	goto RustboroCity_TrainerExchange_EventScript_RelearnPick
	end

@ An Egg Move of its family (from 3 BADGES).
RustboroCity_TrainerExchange_EventScript_EggMove::
	specialvar VAR_RESULT, CoopExchange_Tier
	goto_if_lt VAR_RESULT, 2, RustboroCity_TrainerExchange_EventScript_EggMoveLocked
	setvar VAR_0x8004, 2
	specialvar VAR_RESULT, CoopExchange_SetPrice
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_TooPoor
	copyvar VAR_TEMP_2, VAR_0x8006
	msgbox RustboroCity_TrainerExchange_Text_EggMoveWhichMon, MSGBOX_DEFAULT
	setmoverelearnerstate MOVE_RELEARNER_EGG_MOVES
	goto RustboroCity_TrainerExchange_EventScript_RelearnPick
	end

RustboroCity_TrainerExchange_EventScript_EggMoveLocked::
	msgbox RustboroCity_TrainerExchange_Text_EggMoveLocked, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_MovesMenu
	end

RustboroCity_TrainerExchange_EventScript_RelearnPick::
	chooseboxmon SELECT_PC_MON_MOVE_RELEARNER
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, RustboroCity_TrainerExchange_EventScript_MovesMenu
	special IsSelectedMonEgg
	goto_if_eq VAR_RESULT, TRUE, RustboroCity_TrainerExchange_EventScript_RelearnEgg
	special Special_HasMoveToRelearn
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_RelearnNone
	msgbox RustboroCity_TrainerExchange_Text_RelearnWhichMove, MSGBOX_DEFAULT
	special TeachMoveRelearnerMove
	waitstate
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_NotTaught
	copyvar VAR_0x8006, VAR_TEMP_2
	specialvar VAR_RESULT, CoopExchange_Charge
	playse SE_SHOP
	msgbox RustboroCity_TrainerExchange_Text_Taught, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_MovesMenu
	end

RustboroCity_TrainerExchange_EventScript_RelearnEgg::
	msgbox RustboroCity_TrainerExchange_Text_NotAnEgg, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_MovesMenu
	end

RustboroCity_TrainerExchange_EventScript_RelearnNone::
	msgbox RustboroCity_TrainerExchange_Text_RelearnNone, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_MovesMenu
	end

@ ---------------------------------------------------------------- 2-3. items

RustboroCity_TrainerExchange_EventScript_HeldClerk::
	lock
	faceplayer
	setvar VAR_TEMP_1, COOP_EXCHANGE_HELD
	msgbox RustboroCity_TrainerExchange_Text_HeldIntro, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_ShopMenu
	end

RustboroCity_TrainerExchange_EventScript_EvoClerk::
	lock
	faceplayer
	setvar VAR_TEMP_1, COOP_EXCHANGE_EVO
	msgbox RustboroCity_TrainerExchange_Text_EvoIntro, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_ShopMenu
	end

@ ---------------------------------------------------------------- 4. training

RustboroCity_TrainerExchange_EventScript_TrainClerk::
	lock
	faceplayer
	setvar VAR_TEMP_1, COOP_EXCHANGE_TRAIN
	msgbox RustboroCity_TrainerExchange_Text_TrainIntro, MSGBOX_DEFAULT
RustboroCity_TrainerExchange_EventScript_TrainMenu::
	message RustboroCity_TrainerExchange_Text_HowCanIHelp
	waitmessage
	dynmultichoice 16, 1, FALSE, 6, 0, DYN_MULTICHOICE_CB_NONE, RustboroCity_TrainerExchange_Text_OptBuy, RustboroCity_TrainerExchange_Text_OptEvReset, RustboroCity_TrainerExchange_Text_OptHyper, RustboroCity_TrainerExchange_Text_OptHowTraining, RustboroCity_TrainerExchange_Text_OptSoon, RustboroCity_TrainerExchange_Text_OptBack
	switch VAR_RESULT
	case 0, RustboroCity_TrainerExchange_EventScript_Buy
	case 1, RustboroCity_TrainerExchange_EventScript_EvReset
	case 2, RustboroCity_TrainerExchange_EventScript_Hyper
	case 3, RustboroCity_TrainerExchange_EventScript_HowTraining
	case 4, RustboroCity_TrainerExchange_EventScript_Soon
	goto RustboroCity_TrainerExchange_EventScript_Bye
	end

RustboroCity_TrainerExchange_EventScript_HowTraining::
	msgbox RustboroCity_TrainerExchange_Text_HowTraining, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_EvReset::
	specialvar VAR_RESULT, CoopExchange_Tier
	goto_if_lt VAR_RESULT, 1, RustboroCity_TrainerExchange_EventScript_EvResetLocked
	setvar VAR_0x8004, 3
	specialvar VAR_RESULT, CoopExchange_SetPrice
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_TooPoor
	copyvar VAR_TEMP_2, VAR_0x8006
	msgbox RustboroCity_TrainerExchange_Text_EvResetWhich, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, RustboroCity_TrainerExchange_EventScript_TrainMenu
	specialvar VAR_RESULT, CoopExchange_CheckEvReset
	goto_if_eq VAR_RESULT, 1, RustboroCity_TrainerExchange_EventScript_TrainNotEgg
	goto_if_eq VAR_RESULT, 2, RustboroCity_TrainerExchange_EventScript_EvResetNone
	copyvar VAR_0x8006, VAR_TEMP_2
	msgbox RustboroCity_TrainerExchange_Text_EvResetConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RustboroCity_TrainerExchange_EventScript_TrainMenu
	specialvar VAR_RESULT, CoopExchange_Charge
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_TooPoor
	special CoopExchange_ResetEvs
	playse SE_SHOP
	msgbox RustboroCity_TrainerExchange_Text_EvResetDone, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_EvResetLocked::
	msgbox RustboroCity_TrainerExchange_Text_EvResetLocked, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_EvResetNone::
	msgbox RustboroCity_TrainerExchange_Text_EvResetNone, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_TrainNotEgg::
	msgbox RustboroCity_TrainerExchange_Text_NotAnEgg, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

@ Hyper Training: one stat from 7 BADGES, all six as CHAMPION; Lv. 50+.
RustboroCity_TrainerExchange_EventScript_Hyper::
	specialvar VAR_RESULT, CoopExchange_Tier
	goto_if_lt VAR_RESULT, 4, RustboroCity_TrainerExchange_EventScript_HyperLocked
	msgbox RustboroCity_TrainerExchange_Text_HyperWhich, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, RustboroCity_TrainerExchange_EventScript_TrainMenu
	copyvar VAR_TEMP_3, VAR_0x8004
	message RustboroCity_TrainerExchange_Text_HyperWhichStat
	waitmessage
	dynmultichoice 17, 1, FALSE, 7, 0, DYN_MULTICHOICE_CB_NONE, RustboroCity_TrainerExchange_Text_StatHp, RustboroCity_TrainerExchange_Text_StatAtk, RustboroCity_TrainerExchange_Text_StatDef, RustboroCity_TrainerExchange_Text_StatSpeed, RustboroCity_TrainerExchange_Text_StatSpAtk, RustboroCity_TrainerExchange_Text_StatSpDef, RustboroCity_TrainerExchange_Text_StatAll
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, RustboroCity_TrainerExchange_EventScript_TrainMenu
	copyvar VAR_0x8005, VAR_RESULT
	copyvar VAR_0x8004, VAR_TEMP_3
	goto_if_eq VAR_0x8005, 6, RustboroCity_TrainerExchange_EventScript_HyperAll
	setvar VAR_0x8004, 4
	goto RustboroCity_TrainerExchange_EventScript_HyperPrice
	end

RustboroCity_TrainerExchange_EventScript_HyperAll::
	specialvar VAR_RESULT, CoopExchange_Tier
	goto_if_lt VAR_RESULT, 5, RustboroCity_TrainerExchange_EventScript_HyperAllLocked
	setvar VAR_0x8004, 5
RustboroCity_TrainerExchange_EventScript_HyperPrice::
	specialvar VAR_RESULT, CoopExchange_SetPrice
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_TooPoor
	copyvar VAR_TEMP_2, VAR_0x8006
	copyvar VAR_0x8004, VAR_TEMP_3
	specialvar VAR_RESULT, CoopExchange_CheckHyperTrain
	goto_if_eq VAR_RESULT, 1, RustboroCity_TrainerExchange_EventScript_TrainNotEgg
	goto_if_eq VAR_RESULT, 2, RustboroCity_TrainerExchange_EventScript_HyperLowLevel
	goto_if_eq VAR_RESULT, 3, RustboroCity_TrainerExchange_EventScript_HyperNothing
	copyvar VAR_0x8006, VAR_TEMP_2
	msgbox RustboroCity_TrainerExchange_Text_HyperConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, RustboroCity_TrainerExchange_EventScript_TrainMenu
	specialvar VAR_RESULT, CoopExchange_Charge
	goto_if_eq VAR_RESULT, FALSE, RustboroCity_TrainerExchange_EventScript_TooPoor
	copyvar VAR_0x8004, VAR_TEMP_3
	special CoopExchange_HyperTrain
	playse SE_SHOP
	msgbox RustboroCity_TrainerExchange_Text_HyperDone, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_HyperLocked::
	msgbox RustboroCity_TrainerExchange_Text_HyperLocked, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_HyperAllLocked::
	msgbox RustboroCity_TrainerExchange_Text_HyperAllLocked, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_HyperLowLevel::
	msgbox RustboroCity_TrainerExchange_Text_HyperLowLevel, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

RustboroCity_TrainerExchange_EventScript_HyperNothing::
	msgbox RustboroCity_TrainerExchange_Text_HyperNothing, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_TrainMenu
	end

@ ---------------------------------------------------------------- 5. breeding

RustboroCity_TrainerExchange_EventScript_BreedClerk::
	lock
	faceplayer
	setvar VAR_TEMP_1, COOP_EXCHANGE_BREED
	msgbox RustboroCity_TrainerExchange_Text_BreedIntro, MSGBOX_DEFAULT
RustboroCity_TrainerExchange_EventScript_BreedMenu::
	message RustboroCity_TrainerExchange_Text_HowCanIHelp
	waitmessage
	dynmultichoice 16, 1, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, RustboroCity_TrainerExchange_Text_OptBuy, RustboroCity_TrainerExchange_Text_OptGuide, RustboroCity_TrainerExchange_Text_OptSoon, RustboroCity_TrainerExchange_Text_OptBack
	switch VAR_RESULT
	case 0, RustboroCity_TrainerExchange_EventScript_Buy
	case 1, RustboroCity_TrainerExchange_EventScript_BreedGuide
	case 2, RustboroCity_TrainerExchange_EventScript_Soon
	goto RustboroCity_TrainerExchange_EventScript_Bye
	end

RustboroCity_TrainerExchange_EventScript_BreedGuide::
	msgbox CoopBreeding_Text_Guide, MSGBOX_DEFAULT
	goto RustboroCity_TrainerExchange_EventScript_BreedMenu
	end

@ ---------------------------------------------------------------- guide

RustboroCity_TrainerExchange_EventScript_Guide::
	msgbox RustboroCity_TrainerExchange_Text_Guide, MSGBOX_NPC
	end

RustboroCity_TrainerExchange_EventScript_Directory::
	msgbox RustboroCity_TrainerExchange_Text_Directory, MSGBOX_SIGN
	end

@ ---------------------------------------------------------------- text

RustboroCity_TrainerExchange_Text_HowCanIHelp:
	.string "What would you like?$"

RustboroCity_TrainerExchange_Text_OptBuy:
	.string "BUY$"
RustboroCity_TrainerExchange_Text_OptBuyTm:
	.string "BUY TMs$"
RustboroCity_TrainerExchange_Text_OptTeach:
	.string "TEACH A MOVE$"
RustboroCity_TrainerExchange_Text_OptRelearn:
	.string "RELEARN A MOVE$"
RustboroCity_TrainerExchange_Text_OptEggMove:
	.string "EGG MOVE$"
RustboroCity_TrainerExchange_Text_OptSoon:
	.string "COMING SOON$"
RustboroCity_TrainerExchange_Text_OptBack:
	.string "BACK$"
RustboroCity_TrainerExchange_Text_OptEvReset:
	.string "RESET EVs$"
RustboroCity_TrainerExchange_Text_OptHyper:
	.string "HYPER TRAINING$"
RustboroCity_TrainerExchange_Text_OptHowTraining:
	.string "HOW IT WORKS$"
RustboroCity_TrainerExchange_Text_OptGuide:
	.string "BREEDING GUIDE$"
RustboroCity_TrainerExchange_Text_StatHp:
	.string "HP$"
RustboroCity_TrainerExchange_Text_StatAtk:
	.string "ATTACK$"
RustboroCity_TrainerExchange_Text_StatDef:
	.string "DEFENSE$"
RustboroCity_TrainerExchange_Text_StatSpeed:
	.string "SPEED$"
RustboroCity_TrainerExchange_Text_StatSpAtk:
	.string "SP. ATK$"
RustboroCity_TrainerExchange_Text_StatSpDef:
	.string "SP. DEF$"
RustboroCity_TrainerExchange_Text_StatAll:
	.string "ALL SIX$"

RustboroCity_TrainerExchange_Text_NothingYet:
	.string "Nothing here is for sale to you yet.\n"
	.string "Here's what opens next:$"

RustboroCity_TrainerExchange_Text_TooPoor:
	.string "That costs ¥{STR_VAR_2}.\n"
	.string "You don't have enough money.$"

RustboroCity_TrainerExchange_Text_NoCharge:
	.string "Nothing was taught, so there's\n"
	.string "no charge.$"

RustboroCity_TrainerExchange_Text_Taught:
	.string "Done! I've taken ¥{STR_VAR_2}.$"

RustboroCity_TrainerExchange_Text_NotAnEgg:
	.string "I can't do that for an EGG.$"

RustboroCity_TrainerExchange_Text_MovesIntro:
	.string "MOVES & TMs. TMs to buy, moves I\n"
	.string "teach for a fee, and moves to relearn.\p"
	.string "More moves open as you earn BADGES.\n"
	.string "Only POKéMON that can learn a move\l"
	.string "are offered it.$"

RustboroCity_TrainerExchange_Text_WhichType:
	.string "Which type of move?$"

RustboroCity_TrainerExchange_Text_WhichMove:
	.string "Which move?$"

RustboroCity_TrainerExchange_Text_TeachForPrice:
	.string "{STR_VAR_1} for ¥{STR_VAR_2}?\n"
	.string "You pay only once it's learned.$"

RustboroCity_TrainerExchange_Text_TeachWhichMon:
	.string "Which POKéMON should learn it?$"

RustboroCity_TrainerExchange_Text_RelearnWhichMon:
	.string "A move it learns by level or when\n"
	.string "it evolves, for ¥{STR_VAR_2}. Which one?$"

RustboroCity_TrainerExchange_Text_EggMoveWhichMon:
	.string "An EGG MOVE of its family, for\n"
	.string "¥{STR_VAR_2}. Which POKéMON?$"

RustboroCity_TrainerExchange_Text_EggMoveLocked:
	.string "EGG MOVES I teach to trainers with\n"
	.string "3 BADGES or more.$"

RustboroCity_TrainerExchange_Text_RelearnWhichMove:
	.string "Which move should it learn?$"

RustboroCity_TrainerExchange_Text_RelearnNone:
	.string "There's no move I can teach that\n"
	.string "POKéMON right now.$"

RustboroCity_TrainerExchange_Text_HeldIntro:
	.string "BATTLE ITEMS. Berries and items for\n"
	.string "your POKéMON to hold in battle.\p"
	.string "Stronger items open as you earn\n"
	.string "BADGES: CHOICE items and LIFE ORB\l"
	.string "from 5 BADGES.$"

RustboroCity_TrainerExchange_Text_EvoIntro:
	.string "EVOLUTION. Stones, held evolution\n"
	.string "items and regional items, for every\l"
	.string "POKéMON in this region.\p"
	.string "Early ones from 1 BADGE; the trade\n"
	.string "items and rare stones from 5.$"

RustboroCity_TrainerExchange_Text_TrainIntro:
	.string "TRAINING & NATURE. EXP. CANDIES,\n"
	.string "vitamins, EV berries and MINTS.\p"
	.string "I also reset EVs and, for strong\n"
	.string "trainers, do HYPER TRAINING.$"

RustboroCity_TrainerExchange_Text_HowTraining:
	.string "EVs (effort values) grow from battles.\n"
	.string "Vitamins add 10 EVs, feathers 1.\l"
	.string "Up to 252 a stat, 510 in all.\p"
	.string "Vitamins are not IV boosters.\n"
	.string "POMEG, KELPSY, QUALOT, HONDEW, GREPA\l"
	.string "and TAMATO Berries take 10 away,\l"
	.string "and I can reset them all.\p"
	.string "A MINT changes which stats its\n"
	.string "nature raises and lowers. Its own\l"
	.string "nature stays: that's what an\l"
	.string "EVERSTONE passes to an EGG.\p"
	.string "HYPER TRAINING (7 BADGES, Lv. 50+)\n"
	.string "makes a stat count as if its IV\l"
	.string "were perfect. The IV is unchanged;\l"
	.string "EGGS inherit the real IVs.\p"
	.string "CANDIES never go past your level\n"
	.string "cap. At the cap they aren't used;\l"
	.string "EXP. past the cap is lost.$"

RustboroCity_TrainerExchange_Text_EvResetLocked:
	.string "EV resets I do for trainers with\n"
	.string "a BADGE.$"

RustboroCity_TrainerExchange_Text_EvResetWhich:
	.string "Reset all EVs for ¥{STR_VAR_2}.\n"
	.string "Which POKéMON?$"

RustboroCity_TrainerExchange_Text_EvResetNone:
	.string "{STR_VAR_1} has no EVs to reset.$"

RustboroCity_TrainerExchange_Text_EvResetConfirm:
	.string "Reset all of {STR_VAR_1}'s EVs\n"
	.string "for ¥{STR_VAR_2}?$"

RustboroCity_TrainerExchange_Text_EvResetDone:
	.string "{STR_VAR_1}'s EVs are back to zero.\n"
	.string "I've taken ¥{STR_VAR_2}.$"

RustboroCity_TrainerExchange_Text_HyperLocked:
	.string "HYPER TRAINING I do for trainers\n"
	.string "with 7 BADGES or more.$"

RustboroCity_TrainerExchange_Text_HyperAllLocked:
	.string "All six at once I do for the\n"
	.string "CHAMPION only. One stat, then?$"

RustboroCity_TrainerExchange_Text_HyperWhich:
	.string "One stat for ¥15,000; all six for\n"
	.string "¥60,000 (CHAMPION). Which POKéMON?$"

RustboroCity_TrainerExchange_Text_HyperWhichStat:
	.string "Which stat?$"

RustboroCity_TrainerExchange_Text_HyperLowLevel:
	.string "{STR_VAR_1} needs to be at least\n"
	.string "Lv. 50 for HYPER TRAINING.$"

RustboroCity_TrainerExchange_Text_HyperNothing:
	.string "{STR_VAR_1} is already at its best\n"
	.string "there.$"

RustboroCity_TrainerExchange_Text_HyperConfirm:
	.string "HYPER TRAINING for {STR_VAR_1},\n"
	.string "¥{STR_VAR_2}?$"

RustboroCity_TrainerExchange_Text_HyperDone:
	.string "{STR_VAR_1} is in top form there now!\n"
	.string "I've taken ¥{STR_VAR_2}.$"

RustboroCity_TrainerExchange_Text_BreedIntro:
	.string "BREEDING. EVERSTONES, POWER items,\n"
	.string "the DESTINY KNOT and MIRROR HERB.\p"
	.string "The DAY CARE is on ROUTE 117.\n"
	.string "Ask me for the guide!$"

RustboroCity_TrainerExchange_Text_Guide:
	.string "This is the TRAINER EXCHANGE.\n"
	.string "Five counters: moves, battle items,\l"
	.string "evolution, training, breeding.\p"
	.string "What you can buy depends on your\n"
	.string "own BADGES. A partner's BADGES\l"
	.string "don't count here, but they can\l"
	.string "still give you what they've bought.$"

RustboroCity_TrainerExchange_Text_Directory:
	.string "TRAINER EXCHANGE\n"
	.string "LEFT: MOVES & TMs  RIGHT: BREEDING\p"
	.string "FRONT, left to right:\n"
	.string "BATTLE ITEMS, EVOLUTION, TRAINING$"
