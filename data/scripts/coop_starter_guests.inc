@ Starter guests (src/coop_roster.c): a visitor from each region in a
@ POKéMON CENTER. Once the player has the badges, each gives one of their
@ region's three partner POKéMON a day.
	.global Coop_EventScript_StarterGuestKanto
	.global Coop_EventScript_StarterGuestSinnoh
	.global Coop_EventScript_StarterGuestUnova
	.global Coop_EventScript_StarterGuestKalos

Coop_EventScript_StarterGuestKanto::
	setvar VAR_0x8004, COOP_STARTER_GUEST_KANTO
	goto Coop_EventScript_StarterGuest
Coop_EventScript_StarterGuestSinnoh::
	setvar VAR_0x8004, COOP_STARTER_GUEST_SINNOH
	goto Coop_EventScript_StarterGuest
Coop_EventScript_StarterGuestUnova::
	setvar VAR_0x8004, COOP_STARTER_GUEST_UNOVA
	goto Coop_EventScript_StarterGuest
Coop_EventScript_StarterGuestKalos::
	setvar VAR_0x8004, COOP_STARTER_GUEST_KALOS
	goto Coop_EventScript_StarterGuest

Coop_EventScript_StarterGuest::
	lock
	faceplayer
	specialvar VAR_RESULT, CoopRoster_StarterGuestOffer
	goto_if_eq VAR_RESULT, 0, Coop_EventScript_StarterGuestNotYet
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_StarterGuestToday
	msgbox Coop_Text_StarterGuestIntro, MSGBOX_DEFAULT
	special CoopRoster_BufferStarterNames
	message Coop_Text_StarterGuestWhich
	waitmessage
	dynmultipush gStringVar1, 0
	dynmultipush gStringVar2, 1
	dynmultipush gStringVar3, 2
	dynmultipush gText_Cancel, 3
	dynmultistack 19, 1, FALSE, 4, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_ge VAR_RESULT, 3, Coop_EventScript_StarterGuestBye
	copyvar VAR_0x8005, VAR_RESULT
	specialvar VAR_0x8006, CoopRoster_StarterChoice
	givemon VAR_0x8006, VAR_0x8007
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, Coop_EventScript_StarterGuestNoRoom
	copyvar VAR_0x8008, VAR_RESULT
	special CoopRoster_StarterTaken
	playfanfare MUS_OBTAIN_ITEM
	msgbox Coop_Text_StarterGuestGot, MSGBOX_DEFAULT
	waitfanfare
	goto_if_eq VAR_0x8008, MON_GIVEN_TO_PC, Coop_EventScript_StarterGuestPC
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_StarterGuestAfter
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	goto Coop_EventScript_StarterGuestAfter
Coop_EventScript_StarterGuestPC::
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_StarterGuestPCSent
	call Common_EventScript_NameReceivedBoxMon
Coop_EventScript_StarterGuestPCSent::
	call Common_EventScript_TransferredToPC
Coop_EventScript_StarterGuestAfter::
	msgbox Coop_Text_StarterGuestAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_StarterGuestNotYet::
	msgbox Coop_Text_StarterGuestNotYet, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_StarterGuestToday::
	msgbox Coop_Text_StarterGuestToday, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_StarterGuestNoRoom::
	msgbox Coop_Text_StarterGuestNoRoom, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_StarterGuestBye::
	msgbox Coop_Text_StarterGuestBye, MSGBOX_DEFAULT
	release
	end

Coop_Text_StarterGuestIntro:
	.string "I raise the partner POKéMON that\n"
	.string "new trainers in {STR_VAR_1} start with.\p"
	.string "They're curious about HOENN. Will\n"
	.string "you take one along?$"

Coop_Text_StarterGuestWhich:
	.string "Which one?$"

Coop_Text_StarterGuestGot:
	.string "{PLAYER} received {STR_VAR_1}!$"

Coop_Text_StarterGuestAfter:
	.string "Raise it well. Another will be\n"
	.string "ready tomorrow.$"

Coop_Text_StarterGuestNotYet:
	.string "I raise partner POKéMON from\n"
	.string "{STR_VAR_1}.\p"
	.string "Come back with {STR_VAR_2} GYM BADGES and\n"
	.string "I'll trust you with one.$"

Coop_Text_StarterGuestToday:
	.string "That's all for today. The next\n"
	.string "one is ready tomorrow.$"

Coop_Text_StarterGuestNoRoom:
	.string "Your party and boxes are full.\n"
	.string "Make room and come back.$"

Coop_Text_StarterGuestBye:
	.string "Come back any time.$"
