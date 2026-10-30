@ Town specialities (src/coop_town.c): one service in each of six towns,
@ and a small track that pays a reward once. Each player's own.

@ VAR_0x8004: the town, VAR_0x8005: TRUE for a daily service. Counts the
@ use and pays the track's reward when it is reached.
Coop_EventScript_TownTrack::
	specialvar VAR_RESULT, CoopTown_CountUse
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TownTrackGiven
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TownTrackQueued
	return
Coop_EventScript_TownTrackGiven::
	playfanfare MUS_OBTAIN_ITEM
	message Coop_Text_TownTrackGiven
	waitmessage
	waitfanfare
	waitbuttonpress
	return
Coop_EventScript_TownTrackQueued::
	msgbox Coop_Text_TownTrackQueued, MSGBOX_DEFAULT
	return

Coop_Text_TownTrackGiven:
	.string "You keep coming back! Here, this is\n"
	.string "for you. {PLAYER} got {STR_VAR_2}!$"
Coop_Text_TownTrackQueued:
	.string "This is for you, but your BAG is\n"
	.string "full. It waits ({STR_VAR_2}).$"

@ ------------------------------------------------------------ DEWFORD
	.global Coop_EventScript_TownDewford
Coop_EventScript_TownDewford::
	lock
	faceplayer
	setvar VAR_0x8004, 0
	specialvar VAR_RESULT, CoopTown_FreeToday
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TownDewfordTomorrow
	msgbox Coop_Text_TownDewfordAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_TownEnd
	specialvar VAR_RESULT, CoopTown_BeachRest
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TownDewfordNoPartner
	closemessage
	fadescreen FADE_TO_BLACK
	playse SE_EXIT
	delay 40
	fadescreen FADE_FROM_BLACK
	msgbox Coop_Text_TownDewfordRested, MSGBOX_DEFAULT
	setvar VAR_0x8004, 0
	setvar VAR_0x8005, TRUE
	call Coop_EventScript_TownTrack
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownDewfordTomorrow::
	msgbox Coop_Text_TownDewfordTomorrow, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownDewfordNoPartner::
	msgbox Coop_Text_TownDewfordNoPartner, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd

Coop_Text_TownDewfordAsk:
	.string "Welcome to the BEACH HUT! You and your\n"
	.string "partner POKéMON can rest here a while.\p"
	.string "Once a day it does you both good.\n"
	.string "Will you rest?$"
Coop_Text_TownDewfordRested:
	.string "{PLAYER} and {STR_VAR_1} rested on the\n"
	.string "beach. Your bond grew stronger.$"
Coop_Text_TownDewfordTomorrow:
	.string "You've rested here today. Come back\n"
	.string "tomorrow, the beach will still be here.$"
Coop_Text_TownDewfordNoPartner:
	.string "Rest is for a TRAINER and a partner\n"
	.string "POKéMON. The BOND MENTOR in RUSTBORO\l"
	.string "can help you choose one.$"

@ ------------------------------------------------------------ FALLARBOR
	.global Coop_EventScript_TownFallarbor
Coop_EventScript_TownFallarbor::
	lock
	faceplayer
	msgbox Coop_Text_TownFallarborAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_TownEnd
	msgbox Coop_Text_TownFallarborWhich, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, Coop_EventScript_TownEnd
	specialvar VAR_RESULT, CoopTown_CheckReBall
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TownFallarborEgg
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TownFallarborNotYours
	message Coop_Text_TownFallarborWhichBall
	waitmessage
	setvar VAR_0x8007, 0
	setvar VAR_0x8009, ITEM_NONE
Coop_EventScript_TownFallarborPush::
	specialvar VAR_RESULT, CoopTown_BufferBall
	goto_if_eq VAR_RESULT, ITEM_NONE, Coop_EventScript_TownFallarborList
	call_if_eq VAR_0x8007, 0, Coop_EventScript_TownFallarborFirst
	dynmultipush Coop_Text_TownStrVar1, VAR_RESULT
	addvar VAR_0x8007, 1
	goto_if_lt VAR_0x8007, 30, Coop_EventScript_TownFallarborPush
Coop_EventScript_TownFallarborList::
	goto_if_eq VAR_0x8007, 0, Coop_EventScript_TownFallarborNoBalls
	dynmultipush gText_Cancel, ITEM_NONE
	dynmultistack 15, 1, FALSE, 6, FALSE, VAR_0x8009, DYN_MULTICHOICE_CB_NONE
	closemessage
	goto_if_eq VAR_RESULT, ITEM_NONE, Coop_EventScript_TownEnd
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Coop_EventScript_TownEnd
	copyvar VAR_0x8006, VAR_RESULT
	specialvar VAR_RESULT, CoopTown_ReBall
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TownFallarborSame
	playse SE_BALL_OPEN
	msgbox Coop_Text_TownFallarborDone, MSGBOX_DEFAULT
	setvar VAR_0x8004, 1
	setvar VAR_0x8005, FALSE
	call Coop_EventScript_TownTrack
	goto Coop_EventScript_TownEnd
@ The list opens on the first BALL (its id), not on CANCEL.
Coop_EventScript_TownFallarborFirst::
	copyvar VAR_0x8009, VAR_RESULT
	return
Coop_EventScript_TownFallarborEgg::
	msgbox Coop_Text_TownFallarborEgg, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownFallarborNotYours::
	msgbox Coop_Text_TownFallarborNotYours, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownFallarborNoBalls::
	msgbox Coop_Text_TownFallarborNoBalls, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownFallarborSame::
	msgbox Coop_Text_TownFallarborSame, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd

Coop_Text_TownStrVar1:
	.string "{STR_VAR_1}$"
Coop_Text_TownFallarborAsk:
	.string "I'm a BALL ARTISAN. I can move one of\n"
	.string "your POKéMON into another BALL.\p"
	.string "You bring the BALL. Shall we?$"
Coop_Text_TownFallarborWhich:
	.string "Which POKéMON?$"
Coop_Text_TownFallarborWhichBall:
	.string "Into which BALL?$"
Coop_Text_TownFallarborDone:
	.string "There! {STR_VAR_1} is in a\n"
	.string "{STR_VAR_2} now.$"
Coop_Text_TownFallarborEgg:
	.string "That's an EGG, or nobody at all.$"
Coop_Text_TownFallarborNotYours:
	.string "{STR_VAR_1} came from another\n"
	.string "TRAINER. Its BALL is theirs to keep.$"
Coop_Text_TownFallarborNoBalls:
	.string "You don't have any BALLS with you.$"
Coop_Text_TownFallarborSame:
	.string "It's in that BALL already.$"

@ ------------------------------------------------------------ VERDANTURF
	.global Coop_EventScript_TownVerdanturf
Coop_EventScript_TownVerdanturf::
	lock
	faceplayer
	setvar VAR_0x8004, 2
	specialvar VAR_RESULT, CoopTown_FreeToday
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TownVerdanturfTomorrow
	msgbox Coop_Text_TownVerdanturfAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_TownEnd
	msgbox Coop_Text_TownFallarborWhich, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, Coop_EventScript_TownEnd
	specialvar VAR_RESULT, CoopTown_Groom
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TownFallarborEgg
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TownVerdanturfHappy
	msgbox Coop_Text_TownVerdanturfDone, MSGBOX_DEFAULT
	goto Coop_EventScript_TownVerdanturfCount
Coop_EventScript_TownVerdanturfHappy::
	msgbox Coop_Text_TownVerdanturfHappy, MSGBOX_DEFAULT
Coop_EventScript_TownVerdanturfCount::
	setvar VAR_0x8004, 2
	setvar VAR_0x8005, TRUE
	call Coop_EventScript_TownTrack
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownVerdanturfTomorrow::
	msgbox Coop_Text_TownVerdanturfTomorrow, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd

Coop_Text_TownVerdanturfAsk:
	.string "Verdanturf's clean air makes for the\n"
	.string "best grooming. One POKéMON a day.\p"
	.string "Shall I groom one of yours?$"
Coop_Text_TownVerdanturfDone:
	.string "All brushed! {STR_VAR_1} looks\n"
	.string "very pleased with you.$"
Coop_Text_TownVerdanturfHappy:
	.string "{STR_VAR_1} couldn't be any happier\n"
	.string "with you. It enjoyed it all the same.$"
Coop_Text_TownVerdanturfTomorrow:
	.string "That's today's grooming done.\n"
	.string "Come back tomorrow!$"

@ ------------------------------------------------------------ FORTREE
	.global Coop_EventScript_TownFortree
Coop_EventScript_TownFortree::
	lock
	faceplayer
	setvar VAR_0x8004, 3
	specialvar VAR_RESULT, CoopTown_FreeToday
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TownFortreeTomorrow
	msgbox Coop_Text_TownFortreeGive, MSGBOX_DEFAULT
	specialvar VAR_0x8006, CoopTown_TodaysBerry
	giveitem VAR_0x8006
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TownFortreeNoRoom
	setvar VAR_0x8004, 3
	setvar VAR_0x8005, TRUE
	call Coop_EventScript_TownTrack
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownFortreeNoRoom::
	msgbox Coop_Text_TownFortreeNoRoom, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd
Coop_EventScript_TownFortreeTomorrow::
	msgbox Coop_Text_TownFortreeTomorrow, MSGBOX_DEFAULT
	goto Coop_EventScript_TownEnd

Coop_Text_TownFortreeGive:
	.string "Up here the BERRIES ripen in the\n"
	.string "sun. Take today's pick!$"
Coop_Text_TownFortreeNoRoom:
	.string "No room for it? I'll keep it for\n"
	.string "you until you come back today.$"
Coop_Text_TownFortreeTomorrow:
	.string "That was today's pick. A different\n"
	.string "BERRY ripens tomorrow!$"

@ ------------------------------------------------------------ MOSSDEEP
	.global Coop_EventScript_TownMossdeep
Coop_EventScript_TownMossdeep::
	lock
	faceplayer
	msgbox Coop_Text_TownMossdeepAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_TownEnd
	msgbox Coop_Text_TownFallarborWhich, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, Coop_EventScript_TownEnd
	specialvar VAR_RESULT, CoopTown_JudgeIVs
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TownFallarborEgg
	msgbox gStringVar4, MSGBOX_DEFAULT
	setvar VAR_0x8004, 4
	setvar VAR_0x8005, FALSE
	call Coop_EventScript_TownTrack
	goto Coop_EventScript_TownEnd

Coop_Text_TownMossdeepAsk:
	.string "Like stars, every POKéMON shines in\n"
	.string "its own way. I can read how.\p"
	.string "Shall I judge one of yours?$"

@ ------------------------------------------------------------ PACIFIDLOG
	.global Coop_EventScript_TownPacifidlog
Coop_EventScript_TownPacifidlog::
	lock
	faceplayer
	msgbox Coop_Text_TownPacifidlogIntro, MSGBOX_DEFAULT
	special CoopTown_ReadTides
	msgbox gStringVar4, MSGBOX_DEFAULT
	setvar VAR_0x8004, 5
	setvar VAR_0x8005, TRUE
	call Coop_EventScript_TownTrack
	goto Coop_EventScript_TownEnd

Coop_Text_TownPacifidlogIntro:
	.string "I read the sea for the whole town.\n"
	.string "Here's today's word.$"

Coop_EventScript_TownEnd::
	release
	end
