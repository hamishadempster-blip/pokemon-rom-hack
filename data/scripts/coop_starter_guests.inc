@ Starter guests (src/coop_roster.c): a visitor from each region in a
@ POKéMON CENTER. Once the player has the badges, each gives one of their
@ region's three partner POKéMON a day.
	.global Coop_EventScript_StarterGuestKanto
	.global Coop_EventScript_StarterGuestSinnoh
	.global Coop_EventScript_StarterGuestUnova
	.global Coop_EventScript_StarterGuestKalos
	.global Coop_EventScript_StarterGuestAlola
	.global Coop_EventScript_StarterGuestGalar
	.global Coop_EventScript_StarterGuestPaldea
	.global Coop_EventScript_StarterGuestHoenn

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
Coop_EventScript_StarterGuestAlola::
	setvar VAR_0x8004, COOP_STARTER_GUEST_ALOLA
	goto Coop_EventScript_StarterGuest
Coop_EventScript_StarterGuestGalar::
	setvar VAR_0x8004, COOP_STARTER_GUEST_GALAR
	goto Coop_EventScript_StarterGuest
Coop_EventScript_StarterGuestPaldea::
	setvar VAR_0x8004, COOP_STARTER_GUEST_PALDEA
	goto Coop_EventScript_StarterGuest
Coop_EventScript_StarterGuestHoenn::
	setvar VAR_0x8004, COOP_STARTER_GUEST_HOENN
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
	.string "They'd love to see more of the\n"
	.string "world. Will you take one along?$"

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

@ Fossils. The FOSSIL MANIAC (Route 114 tunnel) digs up one fossil a day,
@ the player's choice of all eleven, so no fossil is an either/or pick. The
@ DEVON scientist (Rustboro, 2F) revives any of them: ROOT and CLAW by the
@ stock script, the other nine here (src/coop_roster.c), one at a time.
	.global Coop_EventScript_FossilManiacDaily
	.global Coop_EventScript_FossilScientistOffer
	.global Coop_EventScript_FossilMonReady

Coop_EventScript_FossilManiacDaily::
	goto_if_set FLAG_DAILY_FOSSIL_DIG, Coop_EventScript_FossilReturn
	msgbox Coop_Text_FossilManiacDug, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_FossilManiacLater
	message Coop_Text_FossilManiacWhich
	waitmessage
	dynmultipush Coop_Text_Fossil_HELIX_FOSSIL, ITEM_HELIX_FOSSIL
	dynmultipush Coop_Text_Fossil_DOME_FOSSIL, ITEM_DOME_FOSSIL
	dynmultipush Coop_Text_Fossil_OLD_AMBER, ITEM_OLD_AMBER
	dynmultipush Coop_Text_Fossil_ROOT_FOSSIL, ITEM_ROOT_FOSSIL
	dynmultipush Coop_Text_Fossil_CLAW_FOSSIL, ITEM_CLAW_FOSSIL
	dynmultipush Coop_Text_Fossil_SKULL_FOSSIL, ITEM_SKULL_FOSSIL
	dynmultipush Coop_Text_Fossil_ARMOR_FOSSIL, ITEM_ARMOR_FOSSIL
	dynmultipush Coop_Text_Fossil_COVER_FOSSIL, ITEM_COVER_FOSSIL
	dynmultipush Coop_Text_Fossil_PLUME_FOSSIL, ITEM_PLUME_FOSSIL
	dynmultipush Coop_Text_Fossil_JAW_FOSSIL, ITEM_JAW_FOSSIL
	dynmultipush Coop_Text_Fossil_SAIL_FOSSIL, ITEM_SAIL_FOSSIL
	dynmultipush Coop_Text_FossilPair0, 1001
	dynmultipush Coop_Text_FossilPair1, 1002
	dynmultipush Coop_Text_FossilPair2, 1003
	dynmultipush Coop_Text_FossilPair3, 1004
	dynmultipush gText_Cancel, 0
	dynmultistack 15, 1, FALSE, 6, FALSE, 999, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, 0, Coop_EventScript_FossilManiacLater
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Coop_EventScript_FossilManiacLater
	copyvar VAR_0x8004, VAR_RESULT
	specialvar VAR_RESULT, CoopRoster_GiveFossil
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_FossilManiacBagFull
	playfanfare MUS_OBTAIN_ITEM
	message Coop_Text_FossilManiacGot
	waitfanfare
	waitmessage
	setflag FLAG_DAILY_FOSSIL_DIG
	msgbox Coop_Text_FossilManiacDevon, MSGBOX_DEFAULT
	return
Coop_EventScript_FossilManiacBagFull::
	msgbox Coop_Text_FossilManiacBagFull, MSGBOX_DEFAULT
	return
Coop_EventScript_FossilManiacLater::
	msgbox Coop_Text_FossilManiacLater, MSGBOX_DEFAULT
Coop_EventScript_FossilReturn::
	return

@ Called by the DEVON scientist before the stock ROOT/CLAW checks. With one
@ of the nine other fossils in the bag: a list, then revival; otherwise
@ back to the stock script.
Coop_EventScript_FossilScientistOffer::
	setvar VAR_0x8009, 0
	checkitem ITEM_HELIX_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_DOME_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_OLD_AMBER
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_SKULL_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_ARMOR_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_COVER_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_PLUME_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_JAW_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	checkitem ITEM_SAIL_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	call Coop_EventScript_FossilPairsCount
	goto_if_eq VAR_0x8009, 0, Coop_EventScript_FossilReturn
	msgbox Coop_Text_FossilScientistNotice, MSGBOX_DEFAULT
	message Coop_Text_FossilScientistWhich
	waitmessage
	checkitem ITEM_HELIX_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_HELIX_FOSSIL
	checkitem ITEM_DOME_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_DOME_FOSSIL
	checkitem ITEM_OLD_AMBER
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_OLD_AMBER
	checkitem ITEM_ROOT_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_ROOT_FOSSIL
	checkitem ITEM_CLAW_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_CLAW_FOSSIL
	checkitem ITEM_SKULL_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_SKULL_FOSSIL
	checkitem ITEM_ARMOR_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_ARMOR_FOSSIL
	checkitem ITEM_COVER_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_COVER_FOSSIL
	checkitem ITEM_PLUME_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_PLUME_FOSSIL
	checkitem ITEM_JAW_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_JAW_FOSSIL
	checkitem ITEM_SAIL_FOSSIL
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPush_SAIL_FOSSIL
	setvar VAR_0x8004, 1001
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPushPair0
	setvar VAR_0x8004, 1002
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPushPair1
	setvar VAR_0x8004, 1003
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPushPair2
	setvar VAR_0x8004, 1004
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilPushPair3
	dynmultipush gText_Cancel, 0
	dynmultistack 16, 1, FALSE, 6, FALSE, 999, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, 0, Coop_EventScript_FossilScientistDecline
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Coop_EventScript_FossilScientistDecline
	copyvar VAR_0x8004, VAR_RESULT
	specialvar VAR_0x8005, CoopRoster_FossilId
	goto_if_eq VAR_0x8005, 1, RustboroCity_DevonCorp_2F_EventScript_GiveRootFossil
	goto_if_eq VAR_0x8005, 2, RustboroCity_DevonCorp_2F_EventScript_GiveClawFossil
	goto_if_eq VAR_0x8005, 0, Coop_EventScript_FossilScientistDecline
	special CoopRoster_TakeFossil
	msgbox Coop_Text_FossilScientistHandOver, MSGBOX_DEFAULT
	setvar VAR_FOSSIL_RESURRECTION_STATE, 1
	copyvar VAR_WHICH_FOSSIL_REVIVED, VAR_0x8005
	release
	end
Coop_EventScript_FossilScientistDecline::
	msgbox Coop_Text_FossilScientistDecline, MSGBOX_DEFAULT
	release
	end
Coop_EventScript_FossilCount::
	addvar VAR_0x8009, 1
	return
Coop_EventScript_FossilPairsCount::
	setvar VAR_0x8004, 1001
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	setvar VAR_0x8004, 1002
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	setvar VAR_0x8004, 1003
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	setvar VAR_0x8004, 1004
	specialvar VAR_RESULT, CoopRoster_FossilPairReady
	call_if_eq VAR_RESULT, TRUE, Coop_EventScript_FossilCount
	return
Coop_EventScript_FossilPushPair0::
	dynmultipush Coop_Text_FossilPair0, 1001
	return
Coop_EventScript_FossilPushPair1::
	dynmultipush Coop_Text_FossilPair1, 1002
	return
Coop_EventScript_FossilPushPair2::
	dynmultipush Coop_Text_FossilPair2, 1003
	return
Coop_EventScript_FossilPushPair3::
	dynmultipush Coop_Text_FossilPair3, 1004
	return
Coop_EventScript_FossilPush_HELIX_FOSSIL::
	dynmultipush Coop_Text_Fossil_HELIX_FOSSIL, ITEM_HELIX_FOSSIL
	return
Coop_EventScript_FossilPush_DOME_FOSSIL::
	dynmultipush Coop_Text_Fossil_DOME_FOSSIL, ITEM_DOME_FOSSIL
	return
Coop_EventScript_FossilPush_OLD_AMBER::
	dynmultipush Coop_Text_Fossil_OLD_AMBER, ITEM_OLD_AMBER
	return
Coop_EventScript_FossilPush_ROOT_FOSSIL::
	dynmultipush Coop_Text_Fossil_ROOT_FOSSIL, ITEM_ROOT_FOSSIL
	return
Coop_EventScript_FossilPush_CLAW_FOSSIL::
	dynmultipush Coop_Text_Fossil_CLAW_FOSSIL, ITEM_CLAW_FOSSIL
	return
Coop_EventScript_FossilPush_SKULL_FOSSIL::
	dynmultipush Coop_Text_Fossil_SKULL_FOSSIL, ITEM_SKULL_FOSSIL
	return
Coop_EventScript_FossilPush_ARMOR_FOSSIL::
	dynmultipush Coop_Text_Fossil_ARMOR_FOSSIL, ITEM_ARMOR_FOSSIL
	return
Coop_EventScript_FossilPush_COVER_FOSSIL::
	dynmultipush Coop_Text_Fossil_COVER_FOSSIL, ITEM_COVER_FOSSIL
	return
Coop_EventScript_FossilPush_PLUME_FOSSIL::
	dynmultipush Coop_Text_Fossil_PLUME_FOSSIL, ITEM_PLUME_FOSSIL
	return
Coop_EventScript_FossilPush_JAW_FOSSIL::
	dynmultipush Coop_Text_Fossil_JAW_FOSSIL, ITEM_JAW_FOSSIL
	return
Coop_EventScript_FossilPush_SAIL_FOSSIL::
	dynmultipush Coop_Text_Fossil_SAIL_FOSSIL, ITEM_SAIL_FOSSIL
	return

Coop_EventScript_FossilMonReady::
	specialvar VAR_0x8006, CoopRoster_FossilSpecies
	goto_if_eq VAR_0x8006, SPECIES_NONE, Coop_EventScript_FossilMonLost
	msgbox Coop_Text_FossilMonReady, MSGBOX_DEFAULT
	givemon VAR_0x8006, 20
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, Coop_EventScript_FossilMonNoRoom
	copyvar VAR_0x8008, VAR_RESULT
	setvar VAR_FOSSIL_RESURRECTION_STATE, 0
	setflag FLAG_RECEIVED_REVIVED_FOSSIL_MON
	playfanfare MUS_OBTAIN_ITEM
	msgbox Coop_Text_StarterGuestGot, MSGBOX_DEFAULT
	waitfanfare
	goto_if_eq VAR_0x8008, MON_GIVEN_TO_PC, Coop_EventScript_FossilMonPC
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_FossilMonDone
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	goto Coop_EventScript_FossilMonDone
Coop_EventScript_FossilMonPC::
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_FossilMonPCSent
	call Common_EventScript_NameReceivedBoxMon
Coop_EventScript_FossilMonPCSent::
	call Common_EventScript_TransferredToPC
Coop_EventScript_FossilMonDone::
	release
	end
Coop_EventScript_FossilMonNoRoom::
	msgbox Coop_Text_StarterGuestNoRoom, MSGBOX_DEFAULT
	release
	end
Coop_EventScript_FossilMonLost::
	setvar VAR_FOSSIL_RESURRECTION_STATE, 0
	release
	end

Coop_Text_FossilManiacDug:
	.string "I dig fossils out of these walls\n"
	.string "every day. I've more than I need.\p"
	.string "Want one? DEVON in RUSTBORO can\n"
	.string "bring it back to life.$"

Coop_Text_FossilManiacWhich:
	.string "Pick whichever you like.$"

Coop_Text_FossilManiacDevon:
	.string "Take it to the scientist on DEVON's\n"
	.string "second floor. Come back tomorrow\l"
	.string "for another.$"

Coop_Text_FossilManiacBagFull:
	.string "Your BAG is full. Make room and\n"
	.string "I'll dig you one.$"

Coop_Text_FossilManiacLater:
	.string "Suit yourself. Ask again today if\n"
	.string "you change your mind.$"

Coop_Text_FossilScientistNotice:
	.string "Oh! You have a fossil with you!\n"
	.string "Our machine can revive any kind.$"

Coop_Text_FossilScientistWhich:
	.string "Which one shall I revive?$"

Coop_Text_FossilScientistHandOver:
	.string "The {STR_VAR_1}, then. It will take\n"
	.string "a little while.\p"
	.string "Go for a walk and come back.$"

Coop_Text_FossilScientistDecline:
	.string "Bring it any time.$"

Coop_Text_FossilMonReady:
	.string "Your fossil came back to life!\n"
	.string "It's a {STR_VAR_1}!$"

Coop_Text_Fossil_HELIX_FOSSIL:
	.string "HELIX FOSSIL$"

Coop_Text_Fossil_DOME_FOSSIL:
	.string "DOME FOSSIL$"

Coop_Text_Fossil_OLD_AMBER:
	.string "OLD AMBER$"

Coop_Text_Fossil_ROOT_FOSSIL:
	.string "ROOT FOSSIL$"

Coop_Text_Fossil_CLAW_FOSSIL:
	.string "CLAW FOSSIL$"

Coop_Text_Fossil_SKULL_FOSSIL:
	.string "SKULL FOSSIL$"

Coop_Text_Fossil_ARMOR_FOSSIL:
	.string "ARMOR FOSSIL$"

Coop_Text_Fossil_COVER_FOSSIL:
	.string "COVER FOSSIL$"

Coop_Text_Fossil_PLUME_FOSSIL:
	.string "PLUME FOSSIL$"

Coop_Text_Fossil_JAW_FOSSIL:
	.string "JAW FOSSIL$"

Coop_Text_Fossil_SAIL_FOSSIL:
	.string "SAIL FOSSIL$"

Coop_Text_FossilPair0:
	.string "BIRD + DRAKE$"

Coop_Text_FossilPair1:
	.string "BIRD + DINO$"

Coop_Text_FossilPair2:
	.string "FISH + DRAKE$"

Coop_Text_FossilPair3:
	.string "FISH + DINO$"

Coop_Text_FossilManiacGot:
	.string "{PLAYER} got the fossil!$"
