@ Duo Frontier (src/coop_frontier.c), build 0xC017013E. The DUO DESK in
@ each Battle Frontier lobby. Only the DUO TOWER is open; the other desks
@ say so plainly.

CoopDuo_EventScript_TowerDesk::
	lock
	faceplayer
	msgbox CoopDuo_Text_Welcome, MSGBOX_DEFAULT
CoopDuo_EventScript_TowerMenu::
	specialvar VAR_RESULT, CoopDuo_BufferStatus
	goto_if_eq VAR_RESULT, 1, CoopDuo_EventScript_RunMenu
	message CoopDuo_Text_ChooseBracket
	waitmessage
	dynmultichoice 15, 1, FALSE, 5, 0, DYN_MULTICHOICE_CB_NONE, CoopDuo_Text_OptLv50, CoopDuo_Text_OptOpen, CoopDuo_Text_OptRules, CoopDuo_Text_OptRecords, CoopDuo_Text_OptExit
	switch VAR_RESULT
	case 0, CoopDuo_EventScript_NewLv50
	case 1, CoopDuo_EventScript_NewOpen
	case 2, CoopDuo_EventScript_Rules
	case 3, CoopDuo_EventScript_Records
	goto CoopDuo_EventScript_Bye

CoopDuo_EventScript_RunMenu::
	message CoopDuo_Text_RunStatus
	waitmessage
	dynmultichoice 15, 1, FALSE, 5, 0, DYN_MULTICHOICE_CB_NONE, CoopDuo_Text_OptContinue, CoopDuo_Text_OptRetire, CoopDuo_Text_OptRules, CoopDuo_Text_OptRecords, CoopDuo_Text_OptExit
	switch VAR_RESULT
	case 0, CoopDuo_EventScript_Continue
	case 1, CoopDuo_EventScript_Retire
	case 2, CoopDuo_EventScript_Rules
	case 3, CoopDuo_EventScript_Records
	goto CoopDuo_EventScript_Bye

CoopDuo_EventScript_Rules::
	msgbox CoopDuo_Text_Rules, MSGBOX_DEFAULT
	goto CoopDuo_EventScript_TowerMenu

CoopDuo_EventScript_Records::
	specialvar VAR_RESULT, CoopDuo_BufferRecords
	msgbox CoopDuo_Text_Records, MSGBOX_DEFAULT
	goto_if_eq VAR_RESULT, 0, CoopDuo_EventScript_TowerMenu
	msgbox CoopDuo_Text_Milestones, MSGBOX_DEFAULT
	goto CoopDuo_EventScript_TowerMenu

CoopDuo_EventScript_Retire::
	msgbox CoopDuo_Text_RetireAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDuo_EventScript_TowerMenu
	specialvar VAR_RESULT, CoopDuo_Retire
	msgbox CoopDuo_Text_Retired, MSGBOX_DEFAULT
	call CoopDuo_EventScript_Save
	goto CoopDuo_EventScript_TowerMenu

CoopDuo_EventScript_NewLv50::
	setvar VAR_0x8004, COOP_DUO_LV50
	goto CoopDuo_EventScript_Start

CoopDuo_EventScript_NewOpen::
	setvar VAR_0x8004, COOP_DUO_OPEN
	goto CoopDuo_EventScript_Start

CoopDuo_EventScript_Continue::
	setvar VAR_0x8004, 0xFF
CoopDuo_EventScript_Start::
	copyvar VAR_0x8008, VAR_0x8004
	specialvar VAR_RESULT, CoopDuo_DeskCheck
	goto_if_eq VAR_RESULT, COOP_DUO_DESK_NOT_LINKED, CoopDuo_EventScript_NeedPartner
	goto_if_eq VAR_RESULT, COOP_DUO_DESK_TEAM, CoopDuo_EventScript_NeedTeam
	goto_if_eq VAR_RESULT, COOP_DUO_DESK_OTHER_RUN, CoopDuo_EventScript_OtherRun
	copyvar VAR_0x8004, VAR_0x8008
	specialvar VAR_RESULT, CoopDuo_PrepareInvite
	goto_if_eq VAR_RESULT, FALSE, CoopDuo_EventScript_Bye
	closemessage
	special Coop_StartDuoBattle
	specialvar VAR_RESULT, CoopDuo_LastOutcome
	goto_if_eq VAR_RESULT, COOP_DUO_OUT_WON, CoopDuo_EventScript_Won
	goto_if_eq VAR_RESULT, COOP_DUO_OUT_LOST, CoopDuo_EventScript_Lost
	specialvar VAR_RESULT, Coop_TakeRefusal
	call Coop_EventScript_RefusalNote
	release
	end

CoopDuo_EventScript_Won::
	msgbox CoopDuo_Text_Won, MSGBOX_DEFAULT
	call_if_ne VAR_0x8005, 0, CoopDuo_EventScript_ShowBp
	call CoopDuo_EventScript_Save
	specialvar VAR_RESULT, CoopDuo_BufferStatus
	msgbox CoopDuo_Text_NextBattle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, CoopDuo_EventScript_Continue
	msgbox CoopDuo_Text_ComeBack, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_ShowBp::
	msgbox CoopDuo_Text_Bp, MSGBOX_DEFAULT
	return

CoopDuo_EventScript_Lost::
	msgbox CoopDuo_Text_Lost, MSGBOX_DEFAULT
	call CoopDuo_EventScript_Save
	release
	end

CoopDuo_EventScript_NeedPartner::
	msgbox CoopDuo_Text_NeedPartner, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_NeedTeam::
	msgbox CoopDuo_Text_NeedTeam, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_OtherRun::
	specialvar VAR_RESULT, CoopDuo_BufferStatus
	msgbox CoopDuo_Text_OtherRun, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDuo_EventScript_Bye
	specialvar VAR_RESULT, CoopDuo_Retire
	msgbox CoopDuo_Text_Retired, MSGBOX_DEFAULT
	call CoopDuo_EventScript_Save
	goto CoopDuo_EventScript_TowerMenu

CoopDuo_EventScript_Bye::
	msgbox CoopDuo_Text_Bye, MSGBOX_DEFAULT
	release
	end

@ The run is written to this save in the background (the link stays up).
CoopDuo_EventScript_Save::
	message CoopDuo_Text_Saving
	waitmessage
	special CoopDuo_SaveNow
	call CoopDeal_EventScript_WaitSave
	closemessage
	return

@ The partner's question (src/coop.c): STR_VAR_1 who asks, STR_VAR_2 what.
CoopDuo_EventScript_AskGuest::
	lockall
	msgbox CoopDuo_Text_Ask, MSGBOX_YESNO
	goto Coop_EventScript_JoinAskAnswer

@ The partner's side after a duo battle: the result on this save, then save.
CoopDuo_EventScript_GuestAfter::
	lockall
	specialvar VAR_RESULT, CoopDuo_LastOutcome
	goto_if_eq VAR_RESULT, COOP_DUO_OUT_WON, CoopDuo_EventScript_GuestWon
	goto_if_eq VAR_RESULT, COOP_DUO_OUT_LOST, CoopDuo_EventScript_GuestLost
	releaseall
	end

CoopDuo_EventScript_GuestWon::
	msgbox CoopDuo_Text_Won, MSGBOX_DEFAULT
	call_if_ne VAR_0x8005, 0, CoopDuo_EventScript_ShowBp
	call CoopDuo_EventScript_Save
	special CoopDeal_BufferPartnerName
	msgbox CoopDuo_Text_GuestNext, MSGBOX_DEFAULT
	releaseall
	end

CoopDuo_EventScript_GuestLost::
	msgbox CoopDuo_Text_Lost, MSGBOX_DEFAULT
	call CoopDuo_EventScript_Save
	releaseall
	end

@ The other facilities: no duo mode in this build.
CoopDuo_EventScript_DomeDesk::
	lock
	faceplayer
	msgbox CoopDuo_Text_DomeUnavailable, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_FactoryDesk::
	lock
	faceplayer
	msgbox CoopDuo_Text_FactoryUnavailable, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_PalaceDesk::
	lock
	faceplayer
	msgbox CoopDuo_Text_PalaceUnavailable, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_ArenaDesk::
	lock
	faceplayer
	msgbox CoopDuo_Text_ArenaUnavailable, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_PikeDesk::
	lock
	faceplayer
	msgbox CoopDuo_Text_PikeUnavailable, MSGBOX_DEFAULT
	release
	end

CoopDuo_EventScript_PyramidDesk::
	lock
	faceplayer
	msgbox CoopDuo_Text_PyramidUnavailable, MSGBOX_DEFAULT
	release
	end

CoopDuo_Text_Welcome:
	.string "This is the DUO DESK. In the DUO\n"
	.string "TOWER, two linked trainers battle\l"
	.string "side by side against TOWER pairs.$"

CoopDuo_Text_ChooseBracket:
	.string "Start a DUO TOWER run?$"

CoopDuo_Text_RunStatus:
	.string "DUO run ({STR_VAR_2}) with\n"
	.string "{STR_VAR_3}: next is battle {STR_VAR_1}.$"

CoopDuo_Text_OptLv50:
	.string "LV. 50$"

CoopDuo_Text_OptOpen:
	.string "OPEN LEVEL$"

CoopDuo_Text_OptRules:
	.string "RULES$"

CoopDuo_Text_OptRecords:
	.string "RECORDS$"

CoopDuo_Text_OptExit:
	.string "EXIT$"

CoopDuo_Text_OptContinue:
	.string "CONTINUE$"

CoopDuo_Text_OptRetire:
	.string "RETIRE$"

CoopDuo_Text_Rules:
	.string "DUO TOWER: two linked trainers,\n"
	.string "three POKéMON each, against two\l"
	.string "TOWER trainers at once.\p"
	.string "No two of your six POKéMON may be\n"
	.string "the same species or hold the same\l"
	.string "item. Some POKéMON can't enter.\p"
	.string "LV. 50: POKéMON above Lv. 50\n"
	.string "fight at Lv. 50. OPEN LEVEL: foes\l"
	.string "match your best (at least Lv. 60).\p"
	.string "Your POKéMON never change: healed\n"
	.string "copies fight. No EXP, no prize\l"
	.string "money, no bag in battle.\p"
	.string "MEGA EVOLUTION works with your own\n"
	.string "MEGA BRACELET.\p"
	.string "Before each battle you're both\n"
	.string "asked again. Every 7 wins pay BP to\l"
	.string "each of you, once.\p"
	.string "Battles 21 and 49 are DUO\n"
	.string "MILESTONE pairs, with bonus BP.\p"
	.string "A lost battle ends the run. If the\n"
	.string "link fails, nothing is won or lost:\l"
	.string "talk to me again to go on.$"

CoopDuo_Text_Records:
	.string "Your best DUO TOWER streaks:\n"
	.string "LV. 50 {STR_VAR_1}, OPEN LEVEL {STR_VAR_2}.$"

CoopDuo_Text_Milestones:
	.string "You've beaten a DUO MILESTONE\n"
	.string "pair. Your partner keeps their own.$"

CoopDuo_Text_RetireAsk:
	.string "Retire this DUO run? Your record\n"
	.string "stays. It can't be continued.$"

CoopDuo_Text_Retired:
	.string "The run ended after {STR_VAR_1} wins.$"

CoopDuo_Text_Won:
	.string "DUO TOWER: battle won!\n"
	.string "Wins in this run: {STR_VAR_1}.$"

CoopDuo_Text_Bp:
	.string "You earned {STR_VAR_2} BP!\n"
	.string "You have {STR_VAR_3} BP.$"

CoopDuo_Text_NextBattle:
	.string "Go on to battle {STR_VAR_1}? Your\n"
	.string "partner is asked again.$"

CoopDuo_Text_ComeBack:
	.string "Your run is saved. Come back\n"
	.string "together to go on.$"

CoopDuo_Text_GuestNext:
	.string "{STR_VAR_1} decides at the DUO DESK\n"
	.string "whether you go on.$"

CoopDuo_Text_Lost:
	.string "The DUO run ends with {STR_VAR_1} wins.\n"
	.string "Your POKéMON are as they were.$"

CoopDuo_Text_NeedPartner:
	.string "The DUO TOWER is for two linked\n"
	.string "trainers. Link up and come here\l"
	.string "together.$"

CoopDuo_Text_NeedTeam:
	.string "You need three POKéMON that can\n"
	.string "enter (no EGGS, no banned ones).$"

CoopDuo_Text_OtherRun:
	.string "Your save has a DUO run with\n"
	.string "{STR_VAR_3}. Retire it?$"

CoopDuo_Text_Bye:
	.string "We hope to see you both again!$"

CoopDuo_Text_Saving:
	.string "Recording your DUO run…$"

CoopDuo_Text_Ask:
	.string "DUO TOWER: {STR_VAR_1} is ready.\n"
	.string "{STR_VAR_2}. Join? (25 s)$"

CoopDuo_Text_DomeUnavailable:
	.string "This is the DUO DESK for the\n"
	.string "BATTLE DOME.\p"
	.string "DUO DOME is not open in this\n"
	.string "version. Only the DUO TOWER takes\l"
	.string "two linked trainers so far.\p"
	.string "The usual challenges are at the\n"
	.string "counter.$"

CoopDuo_Text_FactoryUnavailable:
	.string "This is the DUO DESK for the\n"
	.string "BATTLE FACTORY.\p"
	.string "DUO RENTALS are not open in this\n"
	.string "version. Only the DUO TOWER takes\l"
	.string "two linked trainers so far.\p"
	.string "The usual challenges are at the\n"
	.string "counter.$"

CoopDuo_Text_PalaceUnavailable:
	.string "This is the DUO DESK for the\n"
	.string "BATTLE PALACE.\p"
	.string "DUO PALACE is not open in this\n"
	.string "version. Only the DUO TOWER takes\l"
	.string "two linked trainers so far.\p"
	.string "The usual challenges are at the\n"
	.string "counter.$"

CoopDuo_Text_ArenaUnavailable:
	.string "This is the DUO DESK for the\n"
	.string "BATTLE ARENA.\p"
	.string "DUO ARENA is not open in this\n"
	.string "version. Only the DUO TOWER takes\l"
	.string "two linked trainers so far.\p"
	.string "The usual challenge is at the\n"
	.string "counter.$"

CoopDuo_Text_PikeUnavailable:
	.string "This is the DUO DESK for the\n"
	.string "BATTLE PIKE.\p"
	.string "DUO PIKE is not open in this\n"
	.string "version. Only the DUO TOWER takes\l"
	.string "two linked trainers so far.\p"
	.string "The usual challenge is at the\n"
	.string "counter.$"

CoopDuo_Text_PyramidUnavailable:
	.string "This is the DUO DESK for the\n"
	.string "BATTLE PYRAMID.\p"
	.string "DUO EXPEDITIONS are not open in\n"
	.string "this version. Only the DUO TOWER\l"
	.string "takes two linked trainers so far.\p"
	.string "The usual challenge is at the\n"
	.string "counter.$"
