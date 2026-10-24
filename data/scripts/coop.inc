.global Coop_EventScript_NeedPartner
	.global Coop_EventScript_PartnerDeclined
	.global Coop_EventScript_NotReady
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
	.global Coop_EventScript_BeaconAsk
	.global Coop_EventScript_BeaconOn
	.global Coop_EventScript_BeaconOff
	.global Coop_EventScript_BeaconDeclined
	.global Coop_EventScript_JoinRivalIntro
	.global Coop_EventScript_TalkTwin
	.global Coop_EventScript_RivalTaunt
	.global Coop_EventScript_JoinAskGuest

@ Coop_TakeRefusal: 1 means this player's first three are down,
@ 2 means the partner's are. 0 falls through to the usual message.
Coop_EventScript_NeedPartner::
	lockall
	specialvar VAR_RESULT, Coop_TakeRefusal
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_ShowNotReady
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_ShowPartnerNotReady
	msgbox Coop_Text_NeedPartner, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_PartnerDeclined::
	lockall
	specialvar VAR_RESULT, Coop_TakeRefusal
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_ShowNotReady
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_ShowPartnerNotReady
	msgbox Coop_Text_PartnerDeclined, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_NotReady::
	lockall
Coop_EventScript_ShowNotReady::
	msgbox Coop_Text_NotReady, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_ShowPartnerNotReady::
	msgbox Coop_Text_PartnerNotReady, MSGBOX_DEFAULT
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
	specialvar VAR_RESULT, Coop_CableUp
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_MenuLinked
	special Coop_BufferCapText
	msgbox Coop_Text_AskLink, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_AskToggleCap
	special Coop_BeginLink
	message Coop_Text_WaitingLink
	waitmessage
	specialvar VAR_RESULT, Coop_ArmLinkWait
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_LinkReady
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_MenuEnd
	goto_if_eq VAR_RESULT, 0, Coop_EventScript_LinkFailed
	goto_if_eq VAR_RESULT, 3, Coop_EventScript_LinkFailedExchange
	waitstate
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_LinkReady
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_MenuEnd
	goto_if_eq VAR_RESULT, 3, Coop_EventScript_LinkFailedExchange
Coop_EventScript_LinkFailed::
	msgbox Coop_Text_LinkFailed, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_LinkFailedExchange::
	msgbox Coop_Text_LinkFailedExchange, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_LinkReady::
	msgbox Coop_Text_LinkReady, MSGBOX_DEFAULT
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

@ Linked: a short list. Where the partner is, the Beacon, and your rival
@ are in the start menu's status window just before. B or BACK returns to
@ the start menu, like the other entries.
Coop_EventScript_MenuLinked::
	dynmultichoice 21, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Coop_Text_OptBeacon, Coop_Text_OptUnlink, Coop_Text_OptBack
	switch VAR_RESULT
	case 0, Coop_EventScript_MenuBeacon
	case 1, Coop_EventScript_MenuUnlink
	goto Coop_EventScript_MenuBack

Coop_EventScript_MenuBeacon::
	specialvar VAR_RESULT, Coop_MenuUseBeacon
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_MenuNoBeacon
	msgbox gStringVar4, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_MenuNoBeacon::
	msgbox Coop_Text_NoBeaconYet, MSGBOX_DEFAULT
	goto Coop_EventScript_MenuBack

Coop_EventScript_MenuUnlink::
	msgbox Coop_Text_AskUnlink, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_MenuBack
	special Coop_Disconnect
	msgbox Coop_Text_Disconnected, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_MenuBack::
	closemessage
	special Coop_ReopenStartMenu
	releaseall
	end

Coop_EventScript_MenuEnd::
	releaseall
	end

Coop_EventScript_PartnerChoosing::
	msgbox Coop_Text_PartnerChoosing, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_WarpToLab::
	lockall
	call Route101_EventScript_ApplyRescueState
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

Coop_EventScript_BeaconAsk::
	lockall
	special Coop_BufferBeaconAsk
	msgbox Coop_Text_BeaconAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_BeaconSayNo
	setvar VAR_0x8004, 1
	special Coop_BeaconAnswer
	releaseall
	end

Coop_EventScript_BeaconSayNo::
	setvar VAR_0x8004, 0
	special Coop_BeaconAnswer
	releaseall
	end

Coop_EventScript_BeaconOn::
	lockall
	msgbox Coop_Text_BeaconOn, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_BeaconOff::
	lockall
	msgbox Coop_Text_BeaconOff, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_BeaconDeclined::
	lockall
	msgbox Coop_Text_BeaconDeclined, MSGBOX_DEFAULT
	releaseall
	end

Coop_Text_NeedPartner:
	.string "You both have to be here.\n"
	.string "This isn't a fight you take alone.$"

Coop_Text_PartnerDeclined:
	.string "The link didn't hold.\n"
	.string "Nothing was won or lost.$"

Coop_Text_NotReady:
	.string "Only your first three POKéMON\n"
	.string "go into a co-op battle.\p"
	.string "None of them can fight right now.\n"
	.string "Heal one or move one up.$"

Coop_Text_PartnerNotReady:
	.string "Your partner's first three\n"
	.string "POKéMON can't battle right now.\p"
	.string "Nothing started.\n"
	.string "Try again once they've healed.$"

Coop_Text_OptBeacon:
	.string "BEACON$"

Coop_Text_OptUnlink:
	.string "UNLINK$"

Coop_Text_OptBack:
	.string "BACK$"

Coop_Text_AskUnlink:
	.string "Drop the link?\n"
	.string "The Twin Beacon turns off.$"

Coop_Text_NoBeaconYet:
	.string "PROF. BIRCH gives each of you a\n"
	.string "TWIN BEACON after the rescue.$"

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
	.string "Press B to cancel.$"

Coop_Text_LinkReady:
	.string "Linked with {STR_VAR_1}.$"

Coop_Text_LinkFailed:
	.string "The cable only saw this game.\n"
	.string "Use File, New Multiplayer Window.$"

Coop_Text_LinkFailedExchange:
	.string "Both games were on the cable,\n"
	.string "but the link didn't finish.$"

Coop_Text_AskToggleCap:
	.string "Change the level cap?\n"
	.string "You can't do that while linked.$"

Coop_Text_CapNow:
	.string "Level cap is now {STR_VAR_1}.$"

Coop_Text_PartnerWhere:
	.string "Your partner is on\n"
	.string "{STR_VAR_1}.\p"
	.string "Twin Beacon: {STR_VAR_2}.\p"
	.string "Drop the link?$"

Coop_Text_BeaconGift:
	.string "Use the Twin Beacon from the bag\n"
	.string "when you both want to fight together.$"

Coop_Text_BeaconAsk:
	.string "Turn the Twin Beacon {STR_VAR_1}?$"

Coop_Text_BeaconOn:
	.string "Twin Beacon is on.\n"
	.string "Ordinary fights can be together.$"

Coop_Text_BeaconOff:
	.string "Twin Beacon is off.\n"
	.string "Ordinary fights stay solo.$"

Coop_Text_BeaconDeclined:
	.string "Your partner said no.\n"
	.string "The Twin Beacon stays as it was.$"

Coop_Text_PartnerChoosing:
	.string "Your partner is already choosing\n"
	.string "a POKéMON from the bag.$"

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

@ The guest does not stay inside the leader's script. This applies the
@ story steps that script would have set, then opens a door or leaves
@ for the Hall of Fame when the room would otherwise trap them.
Coop_EventScript_GuestAfterBattle::
	lockall
	specialvar VAR_RESULT, Coop_TakeGuestFollowup
	switch VAR_RESULT
	case 1, Coop_EventScript_GuestRustboro
	case 2, Coop_EventScript_GuestDewford
	case 3, Coop_EventScript_GuestMauville
	case 4, Coop_EventScript_GuestLavaridge
	case 5, Coop_EventScript_GuestPetalburg
	case 6, Coop_EventScript_GuestFortree
	case 7, Coop_EventScript_GuestMossdeep
	case 8, Coop_EventScript_GuestSootopolis
	case 9, Coop_EventScript_GuestEliteFour
	case 10, Coop_EventScript_GuestHallOfFame
	case 11, Coop_EventScript_GuestCatchUp
	case 12, Coop_EventScript_GuestCatchUp
	case 13, Coop_EventScript_GuestCatchUp
	case 14, Coop_EventScript_GuestCatchUp
	case 15, Coop_EventScript_GuestCatchUp
	case 17, Coop_EventScript_GuestCatchUp
	case 18, Coop_EventScript_GuestCatchUp
	case 19, Coop_EventScript_GuestCatchUp
	case 20, Coop_EventScript_GuestCatchUp
	case 21, Coop_EventScript_GuestCatchUp
	case 22, Coop_EventScript_GuestCatchUp
	releaseall
	end

@ The talker keeps playing the cutscene. This only moves the guest's save
@ forward, and never backward. A 2 from the special warps to Route 128.
Coop_EventScript_GuestCatchUp::
	copyvar VAR_0x8004, VAR_RESULT
	specialvar VAR_RESULT, Coop_ApplyGuestStory
	goto_if_eq VAR_RESULT, 1, Common_EventScript_ShowBagIsFull
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_GuestRoute128
	goto_if_eq VAR_RESULT, 3, Coop_EventScript_SharedItemWaiting
	releaseall
	end

Coop_EventScript_GuestRoute128::
	warp MAP_ROUTE128, 38, 22
	waitstate
	releaseall
	end

Coop_EventScript_GuestCountBadge::
	goto_if_ge VAR_PETALBURG_GYM_STATE, 7, Coop_EventScript_GuestCountBadgeDone
	addvar VAR_PETALBURG_GYM_STATE, 1
	goto_if_eq VAR_PETALBURG_GYM_STATE, 6, Coop_EventScript_GuestReadyNorman
Coop_EventScript_GuestCountBadgeDone::
	return

Coop_EventScript_GuestReadyNorman::
	clearflag FLAG_HIDE_PETALBURG_GYM_GREETER
	setflag FLAG_PETALBURG_MART_EXPANDED_ITEMS
	return

Coop_EventScript_GuestRustboro::
	call Coop_EventScript_GuestCountBadge
	goto_if_ge VAR_RUSTBORO_CITY_STATE, 1, Coop_EventScript_GuestRustboroTrainers
	setvar VAR_RUSTBORO_CITY_STATE, 1
Coop_EventScript_GuestRustboroTrainers::
	setvar VAR_0x8008, 1
	call Common_EventScript_SetGymTrainers
	releaseall
	end

Coop_EventScript_GuestDewford::
	call Coop_EventScript_GuestCountBadge
	setvar VAR_0x8008, 2
	call Common_EventScript_SetGymTrainers
	setflag FLAG_ENABLE_BRAWLY_MATCH_CALL
	setflag FLAG_ENABLE_ROXANNE_FIRST_CALL
	setvar VAR_ROXANNE_CALL_STEP_COUNTER, 0
	releaseall
	end

Coop_EventScript_GuestMauville::
	call Coop_EventScript_GuestCountBadge
	goto_if_ge VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE, 3, Coop_EventScript_GuestMauvillePuzzle
	setvar VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE, 3
Coop_EventScript_GuestMauvillePuzzle::
	clearflag FLAG_HIDE_VERDANTURF_TOWN_SCOTT
	setvar VAR_0x8008, 3
	call Common_EventScript_SetGymTrainers
	special MauvilleGymDeactivatePuzzle
	special DrawWholeMapView
	releaseall
	end

Coop_EventScript_GuestLavaridge::
	call Coop_EventScript_GuestCountBadge
	setflag FLAG_WHITEOUT_TO_LAVARIDGE
	setflag FLAG_HIDE_VERDANTURF_TOWN_WANDAS_HOUSE_WALLY
	goto_if_ge VAR_LAVARIDGE_TOWN_STATE, 1, Coop_EventScript_GuestLavaridgeTrainers
	setvar VAR_LAVARIDGE_TOWN_STATE, 1
Coop_EventScript_GuestLavaridgeTrainers::
	setvar VAR_0x8008, 4
	call Common_EventScript_SetGymTrainers
	releaseall
	end

Coop_EventScript_GuestPetalburg::
	goto_if_ge VAR_PETALBURG_GYM_STATE, 7, Coop_EventScript_GuestPetalburgStory
	setvar VAR_PETALBURG_GYM_STATE, 7
Coop_EventScript_GuestPetalburgStory::
	special ResetHealLocationFromDewford
	call EventScript_HideMrBriney
	setflag FLAG_HIDE_MAUVILLE_GYM_WATTSON
	clearflag FLAG_HIDE_MAUVILLE_CITY_WATTSON
	clearflag FLAG_HIDE_DEWFORD_HALL_SLUDGE_BOMB_MAN
	setvar VAR_0x8008, 5
	call Common_EventScript_SetGymTrainers
	call PetalburgCity_Gym_EventScript_UnlockAllDoors
	special DrawWholeMapView
	releaseall
	end

Coop_EventScript_GuestFortree::
	setvar VAR_0x8008, 6
	call Common_EventScript_SetGymTrainers
	setflag FLAG_ENABLE_WINONA_MATCH_CALL
	setvar VAR_SCOTT_FORTREE_CALL_STEP_COUNTER, 0
	setflag FLAG_SCOTT_CALL_FORTREE_GYM
	releaseall
	end

Coop_EventScript_GuestMossdeep::
	setflag FLAG_HIDE_AQUA_HIDEOUT_GRUNTS
	clearflag FLAG_HIDE_SLATEPORT_CITY_STERNS_SHIPYARD_MR_BRINEY
	clearflag FLAG_HIDE_SLATEPORT_CITY_HARBOR_PATRONS
	clearflag FLAG_HIDE_MOSSDEEP_CITY_TEAM_MAGMA
	clearflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_1F_TEAM_MAGMA
	clearflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_2F_TEAM_MAGMA
	clearflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_2F_STEVEN
	setflag FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_1F_STEVEN
	goto_if_ge VAR_MOSSDEEP_CITY_STATE, 1, Coop_EventScript_GuestMossdeepTrainers
	setvar VAR_MOSSDEEP_CITY_STATE, 1
	setvar VAR_MOSSDEEP_SPACE_CENTER_STATE, 1
Coop_EventScript_GuestMossdeepTrainers::
	setvar VAR_0x8008, 7
	call Common_EventScript_SetGymTrainers
	releaseall
	end

Coop_EventScript_GuestSootopolis::
	setflag FLAG_HIDE_SOOTOPOLIS_CITY_RESIDENTS
	setflag FLAG_HIDE_SOOTOPOLIS_CITY_STEVEN
	setflag FLAG_HIDE_SOOTOPOLIS_CITY_WALLACE
	clearflag FLAG_HIDE_SOOTOPOLIS_CITY_MAN_1
	goto_if_ge VAR_SOOTOPOLIS_CITY_STATE, 6, Coop_EventScript_GuestSootopolisTrainers
	setvar VAR_SOOTOPOLIS_CITY_STATE, 6
Coop_EventScript_GuestSootopolisTrainers::
	setvar VAR_0x8008, 8
	call Common_EventScript_SetGymTrainers
	releaseall
	end

Coop_EventScript_GuestEliteFour::
	call PokemonLeague_EliteFour_EventScript_ResetAdvanceToNextRoom
	special DrawWholeMapView
	releaseall
	end

Coop_EventScript_GuestHallOfFame::
	warp MAP_EVER_GRANDE_CITY_HALL_OF_FAME, 7, 16
	waitstate
	releaseall
	end

@ Stage 4. Runs once per catch after a clean co-op wild fight, on the
@ catcher's game only.
Coop_EventScript_CaughtNickname::
	lockall
Coop_EventScript_PartnerCatchNext::
	specialvar VAR_RESULT, Coop_NextPartnerCatch
	goto_if_eq VAR_RESULT, SPECIES_NONE, Coop_EventScript_CaughtNicknameNext
	msgbox Coop_Text_PartnerCaught, MSGBOX_DEFAULT
	goto Coop_EventScript_PartnerCatchNext
Coop_EventScript_CaughtNicknameNext::
	specialvar VAR_RESULT, Coop_NextCaughtNickname
	goto_if_eq VAR_RESULT, SPECIES_NONE, Coop_EventScript_CaughtNicknameDone
	msgbox Coop_Text_YouCaught, MSGBOX_DEFAULT
	msgbox Coop_Text_NicknameCaught, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_CaughtNicknameNext
	fadescreen FADE_TO_BLACK
	special ChangePokemonNickname
	goto Coop_EventScript_CaughtNicknameNext

Coop_EventScript_CaughtNicknameDone::
	releaseall
	end

@ Stage 5. Called from inside the engaged trainer's own battle script, so
@ it returns there and the stock post-battle text runs after either fight.
Coop_EventScript_PairAskHost::
	msgbox Coop_Text_PairAskHost, MSGBOX_YESNO
	copyvar VAR_0x8004, VAR_RESULT
	closemessage
	special Coop_PairHostAnswer
	waitstate
	return

@ Coop_PairGuestAnswer: 1 launched the fight, 2 means the ask ran out.
Coop_EventScript_PairAskGuest::
	lockall
	special Coop_BufferPairNames
	msgbox Coop_Text_PairAskGuest, MSGBOX_YESNO
	closemessage
	specialvar VAR_RESULT, Coop_PairGuestAnswer
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_PairGuestWait
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_PairTooLate
	releaseall
	end

Coop_EventScript_PairGuestWait::
	waitstate
	releaseall
	end

Coop_EventScript_PairTooLate::
	msgbox Coop_Text_PairTooLate, MSGBOX_DEFAULT
	releaseall
	end

Coop_Text_PartnerCaught:
	.string "Your partner's ball caught the\n"
	.string "{STR_VAR_1}. It belongs to them.$"

Coop_Text_YouCaught:
	.string "Your ball caught the {STR_VAR_1}.\n"
	.string "It's yours to keep.$"

Coop_Text_NicknameCaught:
	.string "Give a nickname to the\n"
	.string "{STR_VAR_1} you caught?$"

Coop_Text_PairAskHost:
	.string "{STR_VAR_1} and {STR_VAR_2} could take\n"
	.string "you both on together.\p"
	.string "Ask your partner to join?$"

Coop_Text_PairAskGuest:
	.string "Your partner ran into {STR_VAR_1}\n"
	.string "and {STR_VAR_2} on this route.\p"
	.string "Join them for a 2-on-2 battle?$"

Coop_Text_PairTooLate:
	.string "Your partner couldn't wait,\n"
	.string "so that fight went ahead without you.$"

@ Resolve local progression before accepting another battle invite.
Coop_EventScript_Progression::
	lockall
Coop_EventScript_ProgressionNext::
	specialvar VAR_RESULT, Coop_NextProgression
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_ProgressionMove
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_ProgressionEvolution
	releaseall
	end
Coop_EventScript_ProgressionMove::
	special CanTeachLevelUpMove
	waitstate
	goto Coop_EventScript_ProgressionNext
Coop_EventScript_ProgressionEvolution::
	special Coop_StartProgressionEvolution
	waitstate
	goto Coop_EventScript_ProgressionNext

@ The partner is meeting the rival in this bedroom, or just has. This save
@ has not met the twins yet, so the rival turns to this player as well and
@ the introduction is not replayed later. The rival stands at the PC, the
@ spot the full scene ends on, so the stairs and the partner's path stay clear.
Coop_EventScript_JoinRivalIntro::
	lockall
	goto_if_ne VAR_LITTLEROOT_RIVAL_STATE, 2, Coop_EventScript_JoinRivalEnd
	special Coop_PrepareRivalJoin
	setobjectxyperm LOCALID_RIVALS_HOUSE_2F_RIVAL, VAR_0x8005, VAR_0x8006
	setobjectmovementtype LOCALID_RIVALS_HOUSE_2F_RIVAL, MOVEMENT_TYPE_FACE_UP
	addobject LOCALID_RIVALS_HOUSE_2F_RIVAL
	applymovement LOCALID_RIVALS_HOUSE_2F_RIVAL, Common_Movement_FacePlayer
	waitmovement 0
	playse SE_PIN
	applymovement LOCALID_RIVALS_HOUSE_2F_RIVAL, Common_Movement_ExclamationMark
	waitmovement 0
	checkplayergender
	call_if_eq VAR_RESULT, MALE, Coop_EventScript_JoinMira
	call_if_eq VAR_RESULT, FEMALE, Coop_EventScript_JoinSoren
	closemessage
	applymovement LOCALID_RIVALS_HOUSE_2F_RIVAL, Common_Movement_FaceUp
	waitmovement 0
	setvar VAR_LITTLEROOT_RIVAL_STATE, 3
	setvar VAR_LITTLEROOT_TOWN_STATE, 1
Coop_EventScript_JoinRivalEnd::
	releaseall
	end

Coop_EventScript_JoinMira::
	msgbox Coop_Text_JoinMira, MSGBOX_DEFAULT
	setflag FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_2F_POKE_BALL
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_RIVAL_BEDROOM
	return

Coop_EventScript_JoinSoren::
	msgbox Coop_Text_JoinSoren, MSGBOX_DEFAULT
	setflag FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F_POKE_BALL
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_RIVAL_BEDROOM
	return

Coop_Text_JoinMira:
	.string "MIRA: Oh! There are two of you?\p"
	.string "You must be {PLAYER}{KUN}.\n"
	.string "Dad said our new neighbors were\l"
	.string "moving in today.\p"
	.string "I'm MIRA. My twin brother SOREN\n"
	.string "is around here somewhere, too.\p"
	.string "I hope we can all be friends…\n"
	.string "and POKéMON friends, too!\p"
	.string "Oh, no! I was supposed to help Dad\n"
	.string "catch some wild POKéMON!\p"
	.string "I'll catch you both later!$"

Coop_Text_JoinSoren:
	.string "SOREN: Whoa, two new neighbors?\p"
	.string "You're {PLAYER}, right?\n"
	.string "Dad, PROF. BIRCH, said someone was\l"
	.string "moving in next door today.\p"
	.string "I'm SOREN. My twin sister MIRA's\n"
	.string "around here somewhere.\p"
	.string "Aw, darn, I'm supposed to be out\n"
	.string "helping Dad catch wild POKéMON.\p"
	.string "Catch you two later, okay?$"

@ The second twin beside the story's rival. Soren is brash and keen to
@ win; Mira is warm and curious. Their sibling is the one with the plan.
Coop_EventScript_TalkTwin::
	lock
	faceplayer
	specialvar VAR_RESULT, Coop_TwinIdentity
	goto_if_eq VAR_RESULT, COOP_RIVAL_SOREN, Coop_EventScript_TalkSoren
	msgbox Coop_Text_TalkMira, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_TalkSoren::
	msgbox Coop_Text_TalkSoren, MSGBOX_DEFAULT
	release
	end

@ Before the first rival fight the twin who answers this player's case
@ names their counter-pick. The other twin introduces themselves when the
@ player only met their sibling at home.
Coop_EventScript_RivalTaunt::
	specialvar VAR_RESULT, Coop_BufferRivalTaunt
	switch VAR_RESULT
	case 1, Coop_EventScript_TauntSorenIntro
	case 2, Coop_EventScript_TauntMiraIntro
	case 3, Coop_EventScript_TauntSorenHome
	case 4, Coop_EventScript_TauntMiraHome
	return

Coop_EventScript_TauntSorenIntro::
	msgbox Coop_Text_TauntSorenIntro, MSGBOX_DEFAULT
	return

Coop_EventScript_TauntMiraIntro::
	msgbox Coop_Text_TauntMiraIntro, MSGBOX_DEFAULT
	return

Coop_EventScript_TauntSorenHome::
	msgbox Coop_Text_MiraHello, MSGBOX_DEFAULT
	msgbox Coop_Text_TauntSoren, MSGBOX_DEFAULT
	return

Coop_EventScript_TauntMiraHome::
	msgbox Coop_Text_SorenHello, MSGBOX_DEFAULT
	msgbox Coop_Text_TauntMira, MSGBOX_DEFAULT
	return

Coop_Text_TalkSoren:
	.string "SOREN: Heh, {PLAYER}!\n"
	.string "MIRA does the planning.\p"
	.string "I do the winning!$"

Coop_Text_TalkMira:
	.string "MIRA: Hi, {PLAYER}{KUN}!\n"
	.string "SOREN gets so excited…\p"
	.string "Go on, he has something to say!$"

Coop_Text_TauntSorenIntro:
	.string "SOREN: And I'm SOREN, MIRA's twin!\p"
	.string "Dad let me take {STR_VAR_1}.\n"
	.string "It's got the edge on your\l"
	.string "{STR_VAR_2}, {PLAYER}!\p"
	.string "Heh, try to keep up!$"

Coop_Text_TauntMiraIntro:
	.string "MIRA: And I'm MIRA, SOREN's twin!\p"
	.string "Dad let me take {STR_VAR_1}.\n"
	.string "I want to see how it does against\l"
	.string "your {STR_VAR_2}, {PLAYER}{KUN}!\p"
	.string "Let's both do our best!$"

Coop_Text_MiraHello:
	.string "MIRA: I'm MIRA, SOREN's twin.\n"
	.string "Nice to meet you, {PLAYER}{KUN}!$"

Coop_Text_SorenHello:
	.string "SOREN: I'm SOREN, MIRA's twin.\n"
	.string "Good to meet you, {PLAYER}!$"

Coop_Text_TauntSoren:
	.string "SOREN: Dad let me take {STR_VAR_1}.\n"
	.string "It's got the edge on your\l"
	.string "{STR_VAR_2}, {PLAYER}!\p"
	.string "Heh, try to keep up!$"

Coop_Text_TauntMira:
	.string "MIRA: Dad let me take {STR_VAR_1}.\n"
	.string "I want to see how it does against\l"
	.string "your {STR_VAR_2}, {PLAYER}{KUN}!\p"
	.string "Let's both do our best!$"

@ A shared story item that did not fit. The save keeps it for later.
Coop_EventScript_SharedItemWaiting::
	msgbox Coop_Text_SharedItemWaiting, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_SharedItemArrived::
	lockall
	playfanfare MUS_OBTAIN_ITEM
	msgbox Coop_Text_SharedItemArrived, MSGBOX_DEFAULT
	waitfanfare
	releaseall
	end

Coop_Text_SharedItemWaiting:
	.string "Your partner's win earned you\n"
	.string "the {STR_VAR_1}, but the BAG is full.\p"
	.string "It will arrive once there's room.$"

Coop_Text_SharedItemArrived:
	.string "Your share from the shared win\n"
	.string "arrived: the {STR_VAR_1}!$"

@ The partner ran into a wild POKéMON or a TRAINER nearby. Joining is
@ optional and has to be answered in time; a NO, B, or a late YES leaves
@ the partner to fight alone. Coop_PairGuestAnswer: 1 launched, 2 too late.
Coop_EventScript_JoinAskGuest::
	lockall
	specialvar VAR_RESULT, Coop_BufferJoinAsk
	goto_if_eq VAR_RESULT, COOP_KIND_WILD_VALUE, Coop_EventScript_JoinAskWild
	msgbox Coop_Text_JoinAskTrainer, MSGBOX_YESNO
	goto Coop_EventScript_JoinAskAnswer

Coop_EventScript_JoinAskWild::
	msgbox Coop_Text_JoinAskWild, MSGBOX_YESNO
Coop_EventScript_JoinAskAnswer::
	closemessage
	specialvar VAR_RESULT, Coop_PairGuestAnswer
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_PairGuestWait
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_PairTooLate
	@ 3: a wild fight, launched once this script has ended, from the
	@ same idle field state the old automatic answer started it from.
	releaseall
	end

Coop_Text_JoinAskWild:
	.string "Your partner ran into a wild\n"
	.string "{STR_VAR_1} nearby. Join the battle?$"

Coop_Text_JoinAskTrainer:
	.string "{STR_VAR_1} challenged your partner\n"
	.string "nearby. Join the battle?$"
