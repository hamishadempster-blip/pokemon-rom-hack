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
	specialvar VAR_RESULT, Coop_SessionActive
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

Coop_EventScript_MenuLinked::
	special Coop_BufferPartnerLocation
	special Coop_BufferBeaconStatus
	msgbox Coop_Text_PartnerWhere, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_MenuEnd
	special Coop_Disconnect
	msgbox Coop_Text_Disconnected, MSGBOX_DEFAULT
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
Coop_EventScript_CaughtNicknameNext::
	specialvar VAR_RESULT, Coop_NextCaughtNickname
	goto_if_eq VAR_RESULT, SPECIES_NONE, Coop_EventScript_CaughtNicknameDone
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
