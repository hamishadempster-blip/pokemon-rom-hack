.global Coop_EventScript_NeedPartner
	.global Coop_EventScript_PartnerDeclined
	.global Coop_EventScript_Disconnected
	.global Coop_EventScript_BuildMismatch
	.global Coop_EventScript_CapMismatch
	.global Coop_EventScript_LegendConflict
	.global Coop_EventScript_BattleAborted
	.global Coop_EventScript_TradeDone
	.global Coop_EventScript_TalkPartner
	.global Coop_EventScript_Menu
	.global Coop_EventScript_WarpToLab
	.global Coop_EventScript_ChooseCase

Coop_EventScript_NeedPartner::
	lockall
	msgbox Coop_Text_NeedPartner, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_PartnerDeclined::
	lockall
	msgbox Coop_Text_PartnerDeclined, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_Disconnected::
	lockall
	msgbox Coop_Text_Disconnected, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_BuildMismatch::
	lockall
	msgbox Coop_Text_BuildMismatch, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_CapMismatch::
	lockall
	msgbox Coop_Text_CapMismatch, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_LegendConflict::
	lockall
	special Coop_BufferPartnerLocation
	msgbox Coop_Text_LegendConflict, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_BattleAborted::
	lockall
	msgbox Coop_Text_BattleAborted, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_TradeDone::
	lockall
	msgbox Coop_Text_TradeDone, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_TalkPartner::
	lockall
	faceplayer
	special Coop_BufferPartnerLocation
	msgbox Coop_Text_PartnerHello, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_Menu::
	lockall
	specialvar VAR_RESULT, Coop_SessionActive
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_MenuLinked
	special Coop_BufferCapText
	msgbox Coop_Text_AskLink, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_AskToggleCap
	special Coop_BeginLink
	msgbox Coop_Text_WaitingLink, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_AskToggleCap::
	msgbox Coop_Text_AskToggleCap, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_MenuEnd
	special Coop_ToggleLevelCap
	special Coop_BufferCapText
	msgbox Coop_Text_CapNow, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_MenuLinked::
	special Coop_BufferPartnerLocation
	msgbox Coop_Text_PartnerWhere, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_MenuEnd
	special Coop_Disconnect
	msgbox Coop_Text_Disconnected, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_MenuEnd::
	releaseall
	end

Coop_EventScript_WarpToLab::
	lockall
	setflag FLAG_SYS_POKEMON_GET
	setflag FLAG_RESCUED_BIRCH
	setflag FLAG_HIDE_ROUTE_101_BIRCH_ZIGZAGOON_BATTLE
	setflag FLAG_HIDE_ROUTE_101_BIRCH_STARTERS_BAG
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_BIRCHS_LAB_BIRCH
	setvar VAR_BIRCH_LAB_STATE, 2
	setvar VAR_ROUTE101_STATE, 3
	warp MAP_LITTLEROOT_TOWN_PROFESSOR_BIRCHS_LAB, 6, 5
	waitstate
	releaseall
	end

Coop_EventScript_ChooseCase::
	lockall
	specialvar VAR_RESULT, Coop_PartnerOnSameMap
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_NeedPartner
	msgbox Coop_Text_ChooseCase, MSGBOX_DEFAULT
	msgbox Coop_Text_CaseCinder, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Coop_EventScript_CaseCinder
	msgbox Coop_Text_CaseTide, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Coop_EventScript_CaseTide
	msgbox Coop_Text_CaseThorn, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Coop_EventScript_CaseThorn
	goto Coop_EventScript_ChooseCase

Coop_EventScript_CaseCinder::
	setvar VAR_0x8004, 1
	goto Coop_EventScript_ProposeCase

Coop_EventScript_CaseTide::
	setvar VAR_0x8004, 2
	goto Coop_EventScript_ProposeCase

Coop_EventScript_CaseThorn::
	setvar VAR_0x8004, 0
	goto Coop_EventScript_ProposeCase

Coop_EventScript_ProposeCase::
	special Coop_ProposeCase
	goto_if_eq VAR_RESULT, 0xFF, Coop_EventScript_CaseMismatch
	msgbox Coop_Text_TakeFirst, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Coop_EventScript_ClaimFirst
	setvar VAR_0x8004, 0
	goto Coop_EventScript_Claim

Coop_EventScript_ClaimFirst::
	setvar VAR_0x8004, 1
	goto Coop_EventScript_Claim

Coop_EventScript_Claim::
	special Coop_ClaimStarter
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_NeedPartner
	goto LittlerootTown_ProfessorBirchsLab_EventScript_GiveStarterEvent

Coop_EventScript_CaseMismatch::
	msgbox Coop_Text_CaseMismatch, MSGBOX_DEFAULT
	goto Coop_EventScript_ChooseCase

Coop_Text_NeedPartner:
	.string "You both have to be here.\n"
	.string "This isn't a fight you take alone.$"

Coop_Text_PartnerDeclined:
	.string "The link didn't hold.\n"
	.string "Nothing was won or lost.$"

Coop_Text_Disconnected:
	.string "The link dropped.\n"
	.string "Your party is still yours.$"

Coop_Text_BuildMismatch:
	.string "These two games aren't the same build.\n"
	.string "Patch both from the same ROM.$"

Coop_Text_CapMismatch:
	.string "The level cap doesn't match.\n"
	.string "Set it the same way, then link again.$"

Coop_Text_LegendConflict:
	.string "The saves disagree about {STR_VAR_2}.\n"
	.string "Linking would overwrite a catch, so it stops here.$"

Coop_Text_BattleAborted:
	.string "The link broke mid-battle.\n"
	.string "No badge, no prize, and your party\l"
	.string "is back to how it was.$"

Coop_Text_TradeDone:
	.string "The trade went through.\n"
	.string "That POKéMON is yours now.$"

Coop_Text_PartnerHello:
	.string "Your partner is on\n"
	.string "{STR_VAR_1}.$"

Coop_Text_AskLink:
	.string "Level cap: {STR_VAR_1}.\n"
	.string "Link up with your partner?$"

Coop_Text_WaitingLink:
	.string "Waiting for the other game.\n"
	.string "Open CO-OP there too.$"

Coop_Text_AskToggleCap:
	.string "Change the level cap?\n"
	.string "You can't do that while linked.$"

Coop_Text_CapNow:
	.string "Level cap is now {STR_VAR_1}.$"

Coop_Text_PartnerWhere:
	.string "Your partner is on\n"
	.string "{STR_VAR_1}.\p"
	.string "Drop the link?$"

Coop_Text_ChooseCase:
	.string "Three cases. You share one,\n"
	.string "then each of you takes a POKéMON.$"

Coop_Text_CaseCinder:
	.string "CINDER: TORCHIC and HOUNDOUR.\n"
	.string "Take that case?$"

Coop_Text_CaseTide:
	.string "TIDE: MUDKIP and WINGULL.\n"
	.string "Take that case?$"

Coop_Text_CaseThorn:
	.string "THORN: TREECKO and SHROOMISH.\n"
	.string "Take that case?$"

Coop_Text_TakeFirst:
	.string "The case is agreed.\n"
	.string "Take the first POKéMON?$"

Coop_Text_CaseMismatch:
	.string "You picked different cases.\n"
	.string "Choose again.$"
