@ Joining the partner's conversation with an NPC (src/coop_talk.c). The
@ partner's game runs the NPC; this game shows each line as it arrives, at
@ this player's own pace, and never runs the NPC's script itself.

	.global CoopTalk_EventScript_JoinAsk
	.global CoopTalk_EventScript_BusyNote
	.global CoopTalk_EventScript_JoinPartner

@ A on the NPC (or on the partner) while the partner talks to that NPC.
CoopTalk_EventScript_JoinAsk::
	lockall
	special CoopTalk_BufferPartner
	msgbox CoopTalk_Text_JoinAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopTalk_EventScript_Close
	specialvar VAR_RESULT, CoopTalk_RequestJoin
	goto_if_eq VAR_RESULT, FALSE, CoopTalk_EventScript_NoAnswer
	message CoopTalk_Text_Joining
	waitmessage
CoopTalk_EventScript_Loop::
	special CoopTalk_WaitEvent
	waitstate
	switch VAR_RESULT
	case COOP_TALK_EV_TEXT, CoopTalk_EventScript_Line
	case COOP_TALK_EV_NOTE, CoopTalk_EventScript_Note
	goto CoopTalk_EventScript_Close

CoopTalk_EventScript_Line::
	message gStringVar4
	waitmessage
	waitbuttonpress
	goto CoopTalk_EventScript_Loop

CoopTalk_EventScript_Note::
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto CoopTalk_EventScript_Close

CoopTalk_EventScript_NoAnswer::
	msgbox CoopTalk_Text_NoAnswer, MSGBOX_DEFAULT
CoopTalk_EventScript_Close::
	closemessage
	releaseall
	end

@ The partner's conversation can't be shared: a short note in the corner,
@ no message box, nothing locked.
CoopTalk_EventScript_BusyNote::
	special CoopTalk_ShowBusyNote
	end

CoopTalk_Text_JoinAsk:
	.string "{STR_VAR_1} is talking with them.\n"
	.string "Join the conversation?$"

CoopTalk_Text_Joining:
	.string "…$"

CoopTalk_Text_NoAnswer:
	.string "The conversation can't be joined\n"
	.string "right now.$"
