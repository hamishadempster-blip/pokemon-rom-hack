@ Shared legendary encounters (src/coop_legend.c).
@
@ A map's legendary script sets VAR_0x8004 to its COOP_ENC_* and calls
@ Coop_EventScript_LegendEncounter after its cry. On return VAR_RESULT is
@ 0 caught (the script removes the legendary), 1 it got away (a knockout or
@ a flight: it stays for another try), 2 the player lost, 3 not fought (the
@ player chose to leave).
	.global Coop_EventScript_LegendEncounter
	.global Coop_EventScript_LegendAskGuest
	.global Coop_EventScript_LegendDeliver
	.global Coop_EventScript_LegendNoRoom
	.global Coop_EventScript_LegendAckNote
	.global Coop_EventScript_LegendGuestAway

Coop_EventScript_LegendEncounter::
	specialvar VAR_RESULT, CoopLegend_Offer
	goto_if_eq VAR_RESULT, COOP_LEGEND_SOLO, Coop_EventScript_LegendSolo
	goto_if_eq VAR_RESULT, COOP_LEGEND_GONE, Coop_EventScript_LegendGone
	@ The Twin Beacons resonate: two worlds side by side.
	playse SE_ORB
	fadescreenspeed FADE_TO_WHITE, 2
	fadescreenspeed FADE_FROM_WHITE, 2
	waitse
	msgbox Coop_Text_LegendResonance, MSGBOX_DEFAULT
	msgbox Coop_Text_LegendShareAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_LegendAskAlone
	message Coop_Text_LegendAskingPartner
	waitmessage
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special Coop_StartLegendaryBattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	goto_if_eq VAR_RESULT, 0, Coop_EventScript_LegendDeclined
	specialvar VAR_RESULT, CoopLegend_AfterBattle
	return

Coop_EventScript_LegendDeclined::
	closemessage
	special CoopLegend_BufferDeclined
	msgbox gStringVar4, MSGBOX_DEFAULT
Coop_EventScript_LegendAskAlone::
	msgbox Coop_Text_LegendAlone, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_LegendLeave
Coop_EventScript_LegendSolo::
	closemessage
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special CoopLegend_StartSolo
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, CoopLegend_AfterBattle
	return

Coop_EventScript_LegendGone::
	msgbox Coop_Text_LegendGone, MSGBOX_DEFAULT
	closemessage
	setvar VAR_RESULT, 0
	return

Coop_EventScript_LegendLeave::
	closemessage
	setvar VAR_RESULT, 3
	return

@ The partner's game, on the invite: a YES/NO with the reward explained.
Coop_EventScript_LegendAskGuest::
	lockall
	playse SE_ORB
	msgbox Coop_Text_LegendAskGuest, MSGBOX_YESNO
	closemessage
	specialvar VAR_RESULT, Coop_PairGuestAnswer
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_PairGuestWait
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_PairTooLate
	releaseall
	end

@ A reward this save earned in a shared encounter (the partner's catch):
@ delivered when this player is free and has room.
Coop_EventScript_LegendDeliver::
	lockall
	specialvar VAR_RESULT, CoopLegend_DeliverNext
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, Coop_EventScript_LegendDeliverDone
	copyvar VAR_0x8005, VAR_RESULT
	special CoopLegend_WaitSaved
	playfanfare MUS_OBTAIN_ITEM
	msgbox Coop_Text_LegendReceived, MSGBOX_DEFAULT
	waitfanfare
	goto_if_eq VAR_0x8005, MON_GIVEN_TO_PC, Coop_EventScript_LegendDeliverPC
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_LegendDeliverDone
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	goto Coop_EventScript_LegendDeliverDone
Coop_EventScript_LegendDeliverPC::
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_LegendDeliverPCSent
	call Common_EventScript_NameReceivedBoxMon
Coop_EventScript_LegendDeliverPCSent::
	call Common_EventScript_TransferredToPC
Coop_EventScript_LegendDeliverDone::
	releaseall
	end

Coop_EventScript_LegendNoRoom::
	lockall
	msgbox Coop_Text_LegendNoRoom, MSGBOX_DEFAULT
	releaseall
	end

@ The catcher's game: the partner's save has (or had) its reward, or it
@ will arrive on a later link.
Coop_EventScript_LegendAckNote::
	lockall
	specialvar VAR_RESULT, CoopLegend_TakeAckNote
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_LegendAckRecorded
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_LegendAckAlready
	goto_if_eq VAR_RESULT, 3, Coop_EventScript_LegendAckLater
	releaseall
	end
Coop_EventScript_LegendAckRecorded::
	msgbox Coop_Text_LegendAckRecorded, MSGBOX_DEFAULT
	releaseall
	end
Coop_EventScript_LegendAckAlready::
	msgbox Coop_Text_LegendAckAlready, MSGBOX_DEFAULT
	releaseall
	end
Coop_EventScript_LegendAckLater::
	msgbox Coop_Text_LegendAckLater, MSGBOX_DEFAULT
	releaseall
	end

@ The guest's game after a shared battle without a catch: it flies off here
@ too (the object went when the field came back), back next visit.
Coop_EventScript_LegendGuestAway::
	lockall
	specialvar VAR_RESULT, CoopLegend_TakeAwayNote
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_LegendGuestAwayDone
	msgbox gText_LegendaryFlewAway, MSGBOX_DEFAULT
Coop_EventScript_LegendGuestAwayDone::
	releaseall
	end

Coop_Text_LegendResonance:
	.string "Your TWIN BEACON resonates with\n"
	.string "{STR_VAR_2}'s!\p"
	.string "Your two worlds touch here, and\n"
	.string "each holds its own {STR_VAR_1}.$"

Coop_Text_LegendShareAsk:
	.string "Face {STR_VAR_1} with {STR_VAR_2}?\p"
	.string "If it's caught, each of you keeps\n"
	.string "your own {STR_VAR_1}: no second catch\l"
	.string "and no extra BALL needed.$"

Coop_Text_LegendAskingPartner:
	.string "Asking {STR_VAR_2}…$"

Coop_Text_LegendGone:
	.string "This world's {STR_VAR_1} is already\n"
	.string "yours, from a shared catch.$"

Coop_Text_LegendAlone:
	.string "Face {STR_VAR_1} alone?$"

Coop_Text_LegendAskGuest:
	.string "Your TWIN BEACON resonates!\n"
	.string "Your partner faces {STR_VAR_1}.\p"
	.string "Join them? If it's caught, you each\n"
	.string "keep your own {STR_VAR_1}: no second\l"
	.string "catch and no extra BALL needed.$"

Coop_Text_LegendCaughtShared:
	.string "{STR_VAR_1} was caught! Both trainers\n"
	.string "can now receive their {STR_VAR_1}.$"

Coop_Text_LegendReceived:
	.string "From your world beside your\n"
	.string "partner's, your own {STR_VAR_1}\l"
	.string "came to you!$"

Coop_Text_LegendNoRoom:
	.string "Your {STR_VAR_1} is waiting.\n"
	.string "Make room in your party or PC,\l"
	.string "and it will come to you.$"

Coop_Text_LegendAckRecorded:
	.string "{STR_VAR_2}'s {STR_VAR_1} is safely\n"
	.string "recorded on their save.$"

Coop_Text_LegendAckAlready:
	.string "{STR_VAR_2} already had their own\n"
	.string "{STR_VAR_1}, so none was added.$"

Coop_Text_LegendAckLater:
	.string "{STR_VAR_2}'s {STR_VAR_1} will reach them\n"
	.string "the next time you link.$"
