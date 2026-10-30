@ Pressing A on the co-op partner: BATTLE, TRADE, GIVE ITEM, GIVE POKéMON,
@ VIEW TEAM, PING, FOLLOW (the last three in src/coop_partner.c and the
@ VIEW TEAM snapshot in src/coop_deal.c).
@ The protocol, the escrow and the recovery rules are in src/coop_deal.c.
@ Every wait below ends: on an answer, on B (where the text says so), on a
@ timeout, or when the partner leaves the link or the map.

	.global CoopDeal_EventScript_PartnerMenu
	.global CoopDeal_EventScript_Incoming
	.global CoopDeal_EventScript_Settle
	.global CoopDeal_EventScript_NoRoomToSettle

CoopDeal_EventScript_PartnerMenu::
	lockall
	faceplayer
	special CoopDeal_BufferPartnerName
	specialvar VAR_RESULT, CoopDeal_CanStart
	goto_if_eq VAR_RESULT, COOP_DEAL_PENDING, CoopDeal_EventScript_PendingNotice
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_PartnerBusy
	message CoopDeal_Text_WhatToDo
	waitmessage
	dynmultichoice 17, 1, FALSE, 6, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptBattle, CoopDeal_Text_OptTrade, CoopDeal_Text_OptGiveItem, CoopDeal_Text_OptGiveMon, CoopDeal_Text_OptViewTeam, CoopDeal_Text_OptPing, CoopDeal_Text_OptFollow, CoopDeal_Text_OptCancel
	closemessage
	switch VAR_RESULT
	case 0, CoopDeal_EventScript_AskBattle
	case 1, CoopDeal_EventScript_AskTrade
	case 2, CoopDeal_EventScript_AskGiveItem
	case 3, CoopDeal_EventScript_AskGiveMon
	case 4, CoopDeal_EventScript_ViewTeam
	case 5, CoopDeal_EventScript_Ping
	case 6, CoopDeal_EventScript_Follow
	goto CoopDeal_EventScript_End

@ The partner's real state: a battle, a conversation, a menu, a question,
@ the link reconnecting (src/coop_partner.c).
CoopDeal_EventScript_PartnerBusy::
	special CoopPartner_BufferBusyReason
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

@ A read-only look at the partner's party. The partner's game answers by
@ itself (nothing opens on their screen); with no answer, the last team
@ they sent is shown with its age, or the reason it can't be shown.
CoopDeal_EventScript_ViewTeam::
	specialvar VAR_RESULT, CoopTeam_Ask
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_ViewTeamOld
	message CoopDeal_Text_AskingTeam
	waitmessage
	special CoopTeam_Wait
	waitstate
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_ViewTeamOld
CoopDeal_EventScript_ViewTeamShow::
	special CoopTeam_BufferNote
	msgbox gStringVar4, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	special CoopTeam_Show
	waitstate
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_ViewTeamOld::
	specialvar VAR_RESULT, CoopTeam_HaveSnapshot
	goto_if_eq VAR_RESULT, TRUE, CoopDeal_EventScript_ViewTeamShow
	msgbox CoopDeal_Text_TeamUnavailable, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

@ A sound and a note on the partner's screen, and their marker hops (or
@ its edge arrow shows) for a few seconds. One every five seconds.
CoopDeal_EventScript_Ping::
	specialvar VAR_RESULT, CoopPartner_Ping
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_PingWait
	msgbox CoopDeal_Text_Pinged, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_PingWait::
	msgbox CoopDeal_Text_PingWait, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

@ Walk the partner's path on foot until a button is pressed or something
@ needs this player's own decision (src/coop_partner.c).
CoopDeal_EventScript_Follow::
	specialvar VAR_RESULT, CoopFollow_Start
	switch VAR_RESULT
	case COOP_FOLLOW_OK, CoopDeal_EventScript_FollowOn
	case COOP_FOLLOW_LOOP, CoopDeal_EventScript_FollowLoop
	case COOP_FOLLOW_NOT_ON_FOOT, CoopDeal_EventScript_FollowNotOnFoot
	case COOP_FOLLOW_PARTNER_MODE, CoopDeal_EventScript_FollowPartnerMode
	msgbox CoopDeal_Text_FollowGone, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_FollowOn::
	msgbox CoopDeal_Text_FollowOn, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_FollowLoop::
	msgbox CoopDeal_Text_FollowLoop, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_FollowNotOnFoot::
	msgbox CoopDeal_Text_FollowNotOnFoot, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_FollowPartnerMode::
	msgbox CoopDeal_Text_FollowPartnerMode, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_PendingNotice::
	special CoopDeal_BufferPending
	msgbox CoopDeal_Text_Pending, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_End::
	releaseall
	end

@ ------------------------------------------------------------ battle rules

CoopDeal_EventScript_AskBattle::
	setvar VAR_0x8006, 0
	message CoopDeal_Text_ChooseFormat
	waitmessage
	dynmultichoice 18, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptSingles, CoopDeal_Text_OptDoubles, CoopDeal_Text_OptCancel
	goto_if_eq VAR_RESULT, 1, CoopDeal_EventScript_RuleDoubles
	goto_if_ne VAR_RESULT, 0, CoopDeal_EventScript_CloseEnd
	goto CoopDeal_EventScript_AskLevel
CoopDeal_EventScript_RuleDoubles::
	addvar VAR_0x8006, COOP_RULE_DOUBLES
CoopDeal_EventScript_AskLevel::
	message CoopDeal_Text_ChooseLevel
	waitmessage
	dynmultichoice 18, 1, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptOwnLevels, CoopDeal_Text_OptLv50, CoopDeal_Text_OptLv100, CoopDeal_Text_OptCancel
	goto_if_eq VAR_RESULT, 1, CoopDeal_EventScript_RuleLv50
	goto_if_eq VAR_RESULT, 2, CoopDeal_EventScript_RuleLv100
	goto_if_ne VAR_RESULT, 0, CoopDeal_EventScript_CloseEnd
	goto CoopDeal_EventScript_AskSize
CoopDeal_EventScript_RuleLv50::
	addvar VAR_0x8006, (COOP_RULE_LEVEL_50 << COOP_RULE_LEVEL_SHIFT)
	goto CoopDeal_EventScript_AskSize
CoopDeal_EventScript_RuleLv100::
	addvar VAR_0x8006, (COOP_RULE_LEVEL_100 << COOP_RULE_LEVEL_SHIFT)
CoopDeal_EventScript_AskSize::
	message CoopDeal_Text_ChooseSize
	waitmessage
	dynmultichoice 18, 1, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptUpTo6, CoopDeal_Text_Opt4, CoopDeal_Text_Opt3, CoopDeal_Text_OptCancel
	goto_if_eq VAR_RESULT, 0, CoopDeal_EventScript_RuleSize6
	goto_if_eq VAR_RESULT, 1, CoopDeal_EventScript_RuleSize4
	goto_if_eq VAR_RESULT, 2, CoopDeal_EventScript_AskClauses
	goto CoopDeal_EventScript_CloseEnd
CoopDeal_EventScript_RuleSize6::
	addvar VAR_0x8006, (COOP_RULE_SIZE_6 << COOP_RULE_SIZE_SHIFT)
	goto CoopDeal_EventScript_AskClauses
CoopDeal_EventScript_RuleSize4::
	addvar VAR_0x8006, (COOP_RULE_SIZE_4 << COOP_RULE_SIZE_SHIFT)
CoopDeal_EventScript_AskClauses::
	message CoopDeal_Text_ChooseClauses
	waitmessage
	dynmultichoice 18, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptStandard, CoopDeal_Text_OptUnrestricted, CoopDeal_Text_OptCancel
	goto_if_eq VAR_RESULT, 1, CoopDeal_EventScript_RuleOpen
	goto_if_ne VAR_RESULT, 0, CoopDeal_EventScript_CloseEnd
	goto CoopDeal_EventScript_ConfirmRules
CoopDeal_EventScript_RuleOpen::
	addvar VAR_0x8006, COOP_RULE_OPEN
CoopDeal_EventScript_ConfirmRules::
	closemessage
	special CoopDeal_BufferPartnerName
	special CoopDeal_BufferRules
	msgbox CoopDeal_Text_AskBattleRules, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_End
	setvar VAR_0x8005, COOP_DEAL_BATTLE
	specialvar VAR_RESULT, CoopDeal_StartRequest
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	call CoopDeal_EventScript_WaitAnswer
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto CoopDeal_EventScript_ChooseTeam

CoopDeal_EventScript_CloseEnd::
	closemessage
	goto CoopDeal_EventScript_End

@ ------------------------------------------------------------ teams (both)

CoopDeal_EventScript_ChooseTeam::
	specialvar VAR_RESULT, CoopDeal_TeamNeedsPicker
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_WholeParty
	msgbox CoopDeal_Text_PickTeam, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	special CoopDeal_OpenTeamPicker
	waitstate
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_TeamCancelled
	setvar VAR_0x8004, 1
	goto CoopDeal_EventScript_CheckTeam
CoopDeal_EventScript_WholeParty::
	setvar VAR_0x8004, 0
CoopDeal_EventScript_CheckTeam::
	specialvar VAR_RESULT, CoopDeal_ValidateTeam
	goto_if_eq VAR_RESULT, COOP_DEAL_NO_ROOM, CoopDeal_EventScript_TeamTooSmall
	goto_if_eq VAR_RESULT, COOP_DEAL_INVALID, CoopDeal_EventScript_TeamClauses
	specialvar VAR_RESULT, CoopDeal_TeamReady
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	special CoopDeal_BufferPartnerName
	message CoopDeal_Text_WaitTeam
	waitmessage
	setvar VAR_0x8004, 4
	special CoopDeal_Wait
	waitstate
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_CloseResult
	message CoopDeal_Text_Starting
	waitmessage
	setvar VAR_0x8004, 5
	special CoopDeal_Wait
	waitstate
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_CloseResult
	closemessage
	special CoopDeal_StartFriendlyBattle
	waitstate
	specialvar VAR_RESULT, CoopDeal_BattleOutcome
	special CoopDeal_BufferPartnerName
	switch VAR_RESULT
	case 1, CoopDeal_EventScript_BattleWon
	case 2, CoopDeal_EventScript_BattleLost
	case 3, CoopDeal_EventScript_BattleDrew
	msgbox CoopDeal_Text_BattleCalledOff, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_BattleWon::
	msgbox CoopDeal_Text_BattleWon, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_BattleLost::
	msgbox CoopDeal_Text_BattleLost, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_BattleDrew::
	msgbox CoopDeal_Text_BattleDrew, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_TeamCancelled::
	special CoopDeal_Cancel
	msgbox CoopDeal_Text_YouCalledOff, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_TeamTooSmall::
	special CoopDeal_Cancel
	msgbox CoopDeal_Text_TeamTooSmall, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_TeamClauses::
	special CoopDeal_Cancel
	msgbox CoopDeal_Text_TeamClauses, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

@ ------------------------------------------------------------ trade

CoopDeal_EventScript_AskTrade::
	msgbox CoopDeal_Text_ChooseOffer, MSGBOX_DEFAULT
	call CoopDeal_EventScript_PickMon
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_End
	special CoopDeal_BufferPartnerName
	msgbox CoopDeal_Text_AskOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_End
	setvar VAR_0x8005, COOP_DEAL_TRADE
	specialvar VAR_RESULT, CoopDeal_StartRequest
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	call CoopDeal_EventScript_WaitAnswer
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto CoopDeal_EventScript_ConfirmTrade

@ Both sides: the exact exchange, then the final yes.
CoopDeal_EventScript_ConfirmTrade::
	special CoopDeal_BufferPartnerOffer
	msgbox CoopDeal_Text_YouReceive, MSGBOX_DEFAULT
	special CoopDeal_BufferMyOffer
	msgbox CoopDeal_Text_YouGiveTrade, MSGBOX_YESNO
	copyvar VAR_0x8004, VAR_RESULT
	specialvar VAR_RESULT, CoopDeal_Confirm
	goto_if_eq VAR_RESULT, COOP_DEAL_DECLINED, CoopDeal_EventScript_YouCalledOff
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	call CoopDeal_EventScript_WaitConfirm
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto CoopDeal_EventScript_Transfer

@ VAR_0x8004: a party slot that can leave, or VAR_RESULT is not OK.
CoopDeal_EventScript_PickMon::
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, CoopDeal_EventScript_PickMonNone
	specialvar VAR_RESULT, CoopDeal_CheckOfferMon
	goto_if_eq VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_PickMonPartner
	goto_if_eq VAR_RESULT, COOP_DEAL_NO_ROOM, CoopDeal_EventScript_PickMonLast
	goto_if_eq VAR_RESULT, COOP_DEAL_BUSY, CoopDeal_EventScript_PickMonItem
	msgbox CoopDeal_Text_CantOfferEgg, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_PickMon
CoopDeal_EventScript_PickMonLast::
	msgbox CoopDeal_Text_KeepOneUsable, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_PickMon
CoopDeal_EventScript_PickMonItem::
	msgbox CoopDeal_Text_HeldItemStays, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_PickMon
CoopDeal_EventScript_PickMonNone::
	setvar VAR_RESULT, COOP_DEAL_CANCELLED
CoopDeal_EventScript_PickMonDone::
	return
@ Trainer Bond: offering the Partner Pokemon asks first (it stops being
@ the partner if it leaves; its Bond XP goes with it).
CoopDeal_EventScript_PickMonPartner::
	specialvar VAR_RESULT, CoopBond_ScriptSlotIsPartner
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_PickMonOk
	msgbox CoopDeal_Text_OfferPartner, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_PickMon
CoopDeal_EventScript_PickMonOk::
	setvar VAR_RESULT, COOP_DEAL_OK
	return

@ ------------------------------------------------------------ gifts

CoopDeal_EventScript_AskGiveMon::
	msgbox CoopDeal_Text_ChooseGift, MSGBOX_DEFAULT
	call CoopDeal_EventScript_PickMon
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_End
	special CoopDeal_BufferPartnerName
	msgbox CoopDeal_Text_AskGiveMon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_End
	setvar VAR_0x8005, COOP_DEAL_GIVE_MON
	goto CoopDeal_EventScript_SendGift

CoopDeal_EventScript_AskGiveItem::
	msgbox CoopDeal_Text_ChooseItem, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	special CoopDeal_OpenGiftBag
	waitstate
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_End
	specialvar VAR_RESULT, CoopDeal_CheckGiftItem
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_End
	buffernumberstring STR_VAR_3, VAR_0x8007
	special CoopDeal_BufferPartnerName
	msgbox CoopDeal_Text_AskGiveItem, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_End
	setvar VAR_0x8005, COOP_DEAL_GIVE_ITEM
CoopDeal_EventScript_SendGift::
	specialvar VAR_RESULT, CoopDeal_StartRequest
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	call CoopDeal_EventScript_WaitAnswer
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	@ Asking was the giver's confirmation; accepting was the receiver's.
	setvar VAR_0x8004, 1
	specialvar VAR_RESULT, CoopDeal_Confirm
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto CoopDeal_EventScript_Transfer

@ ------------------------------------------------------------ the exchange

CoopDeal_EventScript_Transfer::
	special CoopDeal_BufferPartnerName
	message CoopDeal_Text_Saving
	waitmessage
	call CoopDeal_EventScript_WaitSave
	specialvar VAR_RESULT, CoopDeal_Prepare
	goto_if_eq VAR_RESULT, COOP_DEAL_INVALID, CoopDeal_EventScript_CloseOfferChanged
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_CloseResult
	setvar VAR_0x8004, 3
	special CoopDeal_Wait
	waitstate
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Interrupted
	specialvar VAR_RESULT, CoopDeal_Commit
	goto_if_eq VAR_RESULT, COOP_DEAL_NO_ROOM, CoopDeal_EventScript_CloseNoRoomLater
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Interrupted
	call CoopDeal_EventScript_WaitSave
	closemessage
	playfanfare MUS_OBTAIN_ITEM
	special CoopDeal_BufferPartnerName
	msgbox CoopDeal_Text_ExchangeDone, MSGBOX_DEFAULT
	waitfanfare
	specialvar VAR_RESULT, CoopDeal_TryTradeEvolution
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_End
	waitstate
	specialvar VAR_RESULT, CoopDeal_SaveAfterEvolution
	call CoopDeal_EventScript_WaitSave
	goto CoopDeal_EventScript_End

@ Until this game's background save is on the flash.
CoopDeal_EventScript_WaitSave::
	setvar VAR_0x8004, 6
	special CoopDeal_Wait
	waitstate
	return

CoopDeal_EventScript_Interrupted::
	closemessage
	special CoopDeal_BufferPartnerName
	msgbox CoopDeal_Text_Interrupted, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_CloseNoRoomLater::
	closemessage
	msgbox CoopDeal_Text_NoRoomToSettle, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_CloseOfferChanged::
	closemessage
	msgbox CoopDeal_Text_OfferChanged, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_YouCalledOff::
	msgbox CoopDeal_Text_YouCalledOff, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

@ ------------------------------------------------------------ waits

CoopDeal_EventScript_WaitAnswer::
	special CoopDeal_BufferPartnerName
	message CoopDeal_Text_WaitAnswer
	waitmessage
	setvar VAR_0x8004, 1
	special CoopDeal_Wait
	waitstate
	closemessage
	return

CoopDeal_EventScript_WaitConfirm::
	special CoopDeal_BufferPartnerName
	message CoopDeal_Text_WaitConfirm
	waitmessage
	setvar VAR_0x8004, 2
	special CoopDeal_Wait
	waitstate
	closemessage
	return

CoopDeal_EventScript_CloseResult::
	closemessage
@ VAR_RESULT: why the deal ended (COOP_DEAL_*).
CoopDeal_EventScript_Result::
	special CoopDeal_BufferPartnerName
	switch VAR_RESULT
	case COOP_DEAL_DECLINED, CoopDeal_EventScript_ResDeclined
	case COOP_DEAL_BUSY, CoopDeal_EventScript_ResBusy
	case COOP_DEAL_NO_ROOM, CoopDeal_EventScript_ResNoRoom
	case COOP_DEAL_PENDING, CoopDeal_EventScript_ResPending
	case COOP_DEAL_INVALID, CoopDeal_EventScript_ResInvalid
	case COOP_DEAL_TIMEOUT, CoopDeal_EventScript_ResTimeout
	case COOP_DEAL_LOST, CoopDeal_EventScript_ResLost
	msgbox CoopDeal_Text_ResCancelled, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_ResDeclined::
	msgbox CoopDeal_Text_ResDeclined, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_ResBusy::
	msgbox CoopDeal_Text_ResBusy, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_ResNoRoom::
	msgbox CoopDeal_Text_ResNoRoom, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_ResPending::
	msgbox CoopDeal_Text_ResPending, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_ResInvalid::
	msgbox CoopDeal_Text_OfferChanged, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_ResTimeout::
	msgbox CoopDeal_Text_ResTimeout, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_ResLost::
	msgbox CoopDeal_Text_ResLost, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

@ ------------------------------------------------------------ recipient

CoopDeal_EventScript_Incoming::
	lockall
	playse SE_PIN
	specialvar VAR_RESULT, CoopDeal_BufferIncoming
	switch VAR_RESULT
	case COOP_DEAL_BATTLE, CoopDeal_EventScript_InBattle
	case COOP_DEAL_TRADE, CoopDeal_EventScript_InTrade
	case COOP_DEAL_GIVE_MON, CoopDeal_EventScript_InGiveMon
	case COOP_DEAL_GIVE_ITEM, CoopDeal_EventScript_InGiveItem
	case COOP_DEAL_WAIT, CoopDeal_EventScript_InWait
	msgbox CoopDeal_Text_ResCancelled, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_InBattle::
	msgbox CoopDeal_Text_InBattle, MSGBOX_YESNO
	copyvar VAR_0x8004, VAR_RESULT
	specialvar VAR_RESULT, CoopDeal_Answer
	goto_if_eq VAR_RESULT, COOP_DEAL_DECLINED, CoopDeal_EventScript_End
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto CoopDeal_EventScript_ChooseTeam

CoopDeal_EventScript_InTrade::
	msgbox CoopDeal_Text_InTrade, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_InDecline
	msgbox CoopDeal_Text_ChooseOfferBack, MSGBOX_DEFAULT
	call CoopDeal_EventScript_PickMon
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_InDecline
	copyvar VAR_0x8005, VAR_0x8004
	setvar VAR_0x8004, 1
	specialvar VAR_RESULT, CoopDeal_Answer
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto CoopDeal_EventScript_ConfirmTrade

CoopDeal_EventScript_InGiveMon::
	msgbox CoopDeal_Text_InGiveMon, MSGBOX_YESNO
	goto CoopDeal_EventScript_InGift
CoopDeal_EventScript_InGiveItem::
	msgbox CoopDeal_Text_InGiveItem, MSGBOX_YESNO
CoopDeal_EventScript_InGift::
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_InDecline
	setvar VAR_0x8004, 1
	specialvar VAR_RESULT, CoopDeal_Answer
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	call CoopDeal_EventScript_WaitConfirm
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto CoopDeal_EventScript_Transfer

@ Rest at the Pokemon Center together (src/coop_time.c). VAR_0x8006: the
@ time of day asked for (4: back to the clock's own time).
CoopDeal_EventScript_InWait::
	goto_if_eq VAR_0x8006, 4, CoopDeal_EventScript_InWaitReal
	msgbox CoopDeal_Text_InWait, MSGBOX_YESNO
	goto CoopDeal_EventScript_InWaitAnswer
CoopDeal_EventScript_InWaitReal::
	msgbox CoopDeal_Text_InWaitReal, MSGBOX_YESNO
CoopDeal_EventScript_InWaitAnswer::
	goto_if_eq VAR_RESULT, NO, CoopDeal_EventScript_InDecline
	setvar VAR_0x8004, 1
	specialvar VAR_RESULT, CoopDeal_Answer
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_Result
	goto Coop_EventScript_WaitApply

CoopDeal_EventScript_InDecline::
	setvar VAR_0x8004, 0
	specialvar VAR_RESULT, CoopDeal_Answer
	goto CoopDeal_EventScript_End

@ ------------------------------------------------------------ settling

@ A record left PREPARED by an interrupted exchange settles now.
CoopDeal_EventScript_Settle::
	lockall
	specialvar VAR_RESULT, CoopDeal_Settle
	goto_if_eq VAR_RESULT, 0, CoopDeal_EventScript_End
	goto_if_eq VAR_RESULT, 3, CoopDeal_EventScript_SettleNoRoom
	copyvar VAR_0x8005, VAR_RESULT
	message CoopDeal_Text_Settling
	waitmessage
	call CoopDeal_EventScript_WaitSave
	closemessage
	goto_if_eq VAR_0x8005, 2, CoopDeal_EventScript_SettledAbort
	playfanfare MUS_OBTAIN_ITEM
	msgbox CoopDeal_Text_SettledCommit, MSGBOX_DEFAULT
	waitfanfare
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_SettledAbort::
	msgbox CoopDeal_Text_SettledAbort, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End
CoopDeal_EventScript_SettleNoRoom::
	msgbox CoopDeal_Text_NoRoomToSettle, MSGBOX_DEFAULT
	goto CoopDeal_EventScript_End

CoopDeal_EventScript_NoRoomToSettle::
	lockall
	msgbox CoopDeal_Text_NoRoomToSettle, MSGBOX_DEFAULT
	releaseall
	end

@ ------------------------------------------------------------ text

CoopDeal_Text_InWait:
	.string "{STR_VAR_1} wants to rest here\n"
	.string "until {STR_VAR_2}. Rest together?$"
CoopDeal_Text_InWaitReal:
	.string "{STR_VAR_1} wants the time of day\n"
	.string "to follow the clock again. OK?$"

CoopDeal_Text_WhatToDo:
	.string "What would you like to do\n"
	.string "with {STR_VAR_1}?$"

CoopDeal_Text_OptBattle:
	.string "BATTLE$"
CoopDeal_Text_OptTrade:
	.string "TRADE$"
CoopDeal_Text_OptGiveItem:
	.string "GIVE ITEM$"
CoopDeal_Text_OptGiveMon:
	.string "GIVE POKéMON$"
CoopDeal_Text_OptCancel:
	.string "CANCEL$"
CoopDeal_Text_OptViewTeam:
	.string "VIEW TEAM$"
CoopDeal_Text_OptPing:
	.string "PING$"
CoopDeal_Text_OptFollow:
	.string "FOLLOW$"

CoopDeal_Text_AskingTeam:
	.string "Asking {STR_VAR_1} for a look\n"
	.string "at their team…$"

CoopDeal_Text_TeamUnavailable:
	.string "{STR_VAR_1}'s team can't be seen\n"
	.string "right now. Try again in a moment.$"

CoopDeal_Text_Pinged:
	.string "You pinged {STR_VAR_1}.$"

CoopDeal_Text_PingWait:
	.string "You pinged a moment ago.\n"
	.string "Wait {STR_VAR_2} more seconds.$"

CoopDeal_Text_FollowOn:
	.string "Following {STR_VAR_1}.\n"
	.string "Press any button to stop.$"

CoopDeal_Text_FollowLoop:
	.string "{STR_VAR_1} is following you\n"
	.string "already.$"

CoopDeal_Text_FollowNotOnFoot:
	.string "You can only follow on foot.$"

CoopDeal_Text_FollowPartnerMode:
	.string "{STR_VAR_1} isn't on foot.\n"
	.string "You can only follow on foot.$"

CoopDeal_Text_FollowGone:
	.string "{STR_VAR_1} can't be followed\n"
	.string "from here.$"
CoopDeal_Text_OptSingles:
	.string "SINGLES$"
CoopDeal_Text_OptDoubles:
	.string "DOUBLES$"
CoopDeal_Text_OptOwnLevels:
	.string "OWN LEVELS$"
CoopDeal_Text_OptLv50:
	.string "ALL LV. 50$"
CoopDeal_Text_OptLv100:
	.string "ALL LV. 100$"
CoopDeal_Text_OptUpTo6:
	.string "UP TO 6$"
CoopDeal_Text_Opt4:
	.string "4 POKéMON$"
CoopDeal_Text_Opt3:
	.string "3 POKéMON$"
CoopDeal_Text_OptStandard:
	.string "STANDARD$"
CoopDeal_Text_OptUnrestricted:
	.string "UNRESTRICTED$"

CoopDeal_Text_PartnerBusy:
	.string "{STR_VAR_1} is busy right now.\n"
	.string "Try again in a moment.$"

CoopDeal_Text_Pending:
	.string "An exchange with {STR_VAR_1}\n"
	.string "hasn't finished settling.\p"
	.string "It finishes by itself while you\n"
	.string "are linked with that partner.$"

CoopDeal_Text_ChooseFormat:
	.string "A friendly battle. Nothing is won\n"
	.string "or lost. Which format?$"

CoopDeal_Text_ChooseLevel:
	.string "Which levels? Lv. 50 and 100\n"
	.string "last only for this battle.$"

CoopDeal_Text_ChooseSize:
	.string "How many POKéMON each?$"

CoopDeal_Text_ChooseClauses:
	.string "STANDARD: no two of one species\n"
	.string "or held item. UNRESTRICTED: any.$"

CoopDeal_Text_AskBattleRules:
	.string "Ask {STR_VAR_1} to battle?\n"
	.string "{STR_VAR_2}.\p"
	.string "{STR_VAR_3}.$"

CoopDeal_Text_InBattle:
	.string "{STR_VAR_1} asks for a friendly\n"
	.string "battle! {STR_VAR_2}.\p"
	.string "{STR_VAR_3}.\n"
	.string "No prize, EXP. or losses. Accept?$"

CoopDeal_Text_PickTeam:
	.string "Choose the POKéMON for this\n"
	.string "battle, in order.$"

CoopDeal_Text_TeamTooSmall:
	.string "You need more POKéMON able to\n"
	.string "battle for those rules.\p"
	.string "The battle was called off.$"

CoopDeal_Text_TeamClauses:
	.string "Under STANDARD rules no two\n"
	.string "POKéMON may share a species\l"
	.string "or a held item.\p"
	.string "The battle was called off.$"

CoopDeal_Text_WaitTeam:
	.string "Waiting for {STR_VAR_1} to\n"
	.string "choose a team… B: call off$"

CoopDeal_Text_Starting:
	.string "Both teams are ready!$"

CoopDeal_Text_BattleWon:
	.string "You won the friendly battle!\p"
	.string "Your POKéMON are back just as\n"
	.string "they were before it.$"

CoopDeal_Text_BattleLost:
	.string "{STR_VAR_1} won the friendly\n"
	.string "battle!\p"
	.string "Your POKéMON are back just as\n"
	.string "they were before it.$"

CoopDeal_Text_BattleDrew:
	.string "The friendly battle was a draw!\p"
	.string "Your POKéMON are back just as\n"
	.string "they were before it.$"

CoopDeal_Text_BattleCalledOff:
	.string "The friendly battle ended early.\p"
	.string "Your POKéMON are back just as\n"
	.string "they were before it.$"

CoopDeal_Text_ChooseOffer:
	.string "Choose a POKéMON to offer.$"

CoopDeal_Text_ChooseOfferBack:
	.string "Choose a POKéMON to offer back.$"

CoopDeal_Text_ChooseGift:
	.string "Choose a POKéMON to give.$"

CoopDeal_Text_ChooseItem:
	.string "Choose an item to give.$"

CoopDeal_Text_CantOfferEgg:
	.string "An EGG can't be traded or given.$"

CoopDeal_Text_KeepOneUsable:
	.string "Keep at least one other POKéMON\n"
	.string "that can battle.$"

CoopDeal_Text_HeldItemStays:
	.string "It's holding {STR_VAR_2}, which has\n"
	.string "to stay with you. Take it first.$"

CoopDeal_Text_AskOffer:
	.string "Offer {STR_VAR_2}\n"
	.string "{STR_VAR_3}?$"

CoopDeal_Text_AskGiveMon:
	.string "{STR_VAR_2}\n"
	.string "{STR_VAR_3}.\p"
	.string "Give it to {STR_VAR_1}?$"

CoopDeal_Text_AskGiveItem:
	.string "Give {STR_VAR_3} × {STR_VAR_2}\n"
	.string "to {STR_VAR_1}?$"

CoopDeal_Text_InTrade:
	.string "{STR_VAR_1} offers a trade:\n"
	.string "{STR_VAR_2}\p"
	.string "{STR_VAR_3}.\n"
	.string "Offer a POKéMON back?$"

CoopDeal_Text_InGiveMon:
	.string "{STR_VAR_1} wants to give you\n"
	.string "{STR_VAR_2}\p"
	.string "{STR_VAR_3}.\n"
	.string "Accept it?$"

CoopDeal_Text_InGiveItem:
	.string "{STR_VAR_1} wants to give you\n"
	.string "{STR_VAR_2}. Accept?$"

CoopDeal_Text_YouReceive:
	.string "You would receive\n"
	.string "{STR_VAR_2}\p"
	.string "{STR_VAR_3}.$"

CoopDeal_Text_OfferPartner:
	.string "{STR_VAR_1} is your Partner POKéMON.\n"
	.string "If it goes, it stops being your\l"
	.string "partner; its BOND XP goes with it.\l"
	.string "Offer it?$"

CoopDeal_Text_YouGiveTrade:
	.string "for your {STR_VAR_2}\n"
	.string "{STR_VAR_3}.\p"
	.string "Make this trade?$"

CoopDeal_Text_WaitAnswer:
	.string "Asking {STR_VAR_1}…\n"
	.string "B: call off$"

CoopDeal_Text_WaitConfirm:
	.string "Waiting for {STR_VAR_1} to\n"
	.string "confirm… B: call off$"

CoopDeal_Text_Saving:
	.string "Saving both games…\n"
	.string "Don't turn off the power.$"

CoopDeal_Text_ExchangeDone:
	.string "The exchange with {STR_VAR_1}\n"
	.string "is complete, and both games\l"
	.string "are saved.$"

CoopDeal_Text_Interrupted:
	.string "The link was interrupted.\p"
	.string "Nothing is lost. The exchange\n"
	.string "finishes or is called off the\l"
	.string "next time you link with\l"
	.string "{STR_VAR_1}.$"

CoopDeal_Text_OfferChanged:
	.string "That offer is no longer valid,\n"
	.string "so nothing was exchanged.$"

CoopDeal_Text_YouCalledOff:
	.string "You called it off.$"

CoopDeal_Text_ResDeclined:
	.string "{STR_VAR_1} said no.$"

CoopDeal_Text_ResBusy:
	.string "{STR_VAR_1} is busy right now.$"

CoopDeal_Text_ResNoRoom:
	.string "{STR_VAR_1} has no room for it.$"

CoopDeal_Text_ResPending:
	.string "{STR_VAR_1} has an earlier\n"
	.string "exchange still settling.$"

CoopDeal_Text_ResTimeout:
	.string "No answer from {STR_VAR_1}.$"

CoopDeal_Text_ResLost:
	.string "{STR_VAR_1} isn't here anymore.$"

CoopDeal_Text_ResCancelled:
	.string "The request was called off.$"

CoopDeal_Text_Settling:
	.string "Finishing an earlier exchange…\n"
	.string "Don't turn off the power.$"

CoopDeal_Text_SettledCommit:
	.string "An earlier exchange with your\n"
	.string "partner is now complete.$"

CoopDeal_Text_SettledAbort:
	.string "An unfinished exchange was\n"
	.string "called off. Everything you\l"
	.string "offered is back with you.$"

CoopDeal_Text_NoRoomToSettle:
	.string "An exchange is waiting for room\n"
	.string "in your party, BOXES or BAG.$"

@ ------------------------------------------------------------ practice

@ The COACH in several Pokemon Centers: an AI battle against a copy of your
@ team or of your linked partner's (src/coop_practice.c). Nothing changes.
	.global Coop_EventScript_PracticeCoach
Coop_EventScript_PracticeCoach::
	lock
	faceplayer
	msgbox CoopDeal_Text_CoachHello, MSGBOX_DEFAULT
	message CoopDeal_Text_CoachWhichTeam
	waitmessage
	dynmultichoice 15, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptYourTeam, CoopDeal_Text_OptPartnerTeam, CoopDeal_Text_OptCancel
	switch VAR_RESULT
	case 0, CoopDeal_EventScript_CoachOwn
	case 1, CoopDeal_EventScript_CoachPartner
	goto CoopDeal_EventScript_CoachBye
CoopDeal_EventScript_CoachOwn::
	setvar VAR_0x8004, 0
	goto CoopDeal_EventScript_CoachFormat
CoopDeal_EventScript_CoachPartner::
	specialvar VAR_RESULT, CoopPractice_AskPartnerTeam
	goto_if_ne VAR_RESULT, COOP_DEAL_OK, CoopDeal_EventScript_CoachNoPartner
	message CoopDeal_Text_CoachAsking
	waitmessage
	special CoopPractice_WaitPartnerTeam
	waitstate
	goto_if_eq VAR_RESULT, FALSE, CoopDeal_EventScript_CoachNoTeam
	setvar VAR_0x8004, 1
CoopDeal_EventScript_CoachFormat::
	message CoopDeal_Text_ChooseFormat2
	waitmessage
	dynmultichoice 18, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptSingles, CoopDeal_Text_OptDoubles, CoopDeal_Text_OptCancel
	goto_if_gt VAR_RESULT, 1, CoopDeal_EventScript_CoachBye
	copyvar VAR_0x8005, VAR_RESULT
	message CoopDeal_Text_ChooseLevel2
	waitmessage
	dynmultichoice 18, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, CoopDeal_Text_OptOwnLevels, CoopDeal_Text_OptLv50, CoopDeal_Text_OptCancel
	goto_if_gt VAR_RESULT, 1, CoopDeal_EventScript_CoachBye
	copyvar VAR_0x8006, VAR_RESULT
	specialvar VAR_RESULT, CoopPractice_Prepare
	goto_if_eq VAR_RESULT, 1, CoopDeal_EventScript_CoachTooFewMine
	goto_if_eq VAR_RESULT, 2, CoopDeal_EventScript_CoachTooFewFoe
	msgbox CoopDeal_Text_CoachGo, MSGBOX_DEFAULT
	closemessage
	special CoopPractice_Start
	waitstate
	specialvar VAR_RESULT, CoopPractice_Outcome
	goto_if_eq VAR_RESULT, 1, CoopDeal_EventScript_CoachWon
	msgbox CoopDeal_Text_CoachAfter, MSGBOX_DEFAULT
	release
	end
CoopDeal_EventScript_CoachWon::
	msgbox CoopDeal_Text_CoachWon, MSGBOX_DEFAULT
	release
	end
CoopDeal_EventScript_CoachNoPartner::
	msgbox CoopDeal_Text_CoachNoPartner, MSGBOX_DEFAULT
	release
	end
CoopDeal_EventScript_CoachNoTeam::
	msgbox CoopDeal_Text_CoachNoTeam, MSGBOX_DEFAULT
	release
	end
CoopDeal_EventScript_CoachTooFewMine::
	msgbox CoopDeal_Text_CoachTooFewMine, MSGBOX_DEFAULT
	release
	end
CoopDeal_EventScript_CoachTooFewFoe::
	msgbox CoopDeal_Text_CoachTooFewFoe, MSGBOX_DEFAULT
	release
	end
CoopDeal_EventScript_CoachBye::
	msgbox CoopDeal_Text_CoachBye, MSGBOX_DEFAULT
	release
	end

CoopDeal_Text_CoachHello:
	.string "I'm a battle COACH. I set copies of\n"
	.string "a team against yours.\p"
	.string "Only copies fight: your POKéMON,\n"
	.string "items and money stay as they are.$"

CoopDeal_Text_CoachWhichTeam:
	.string "Whose team should you face?$"

CoopDeal_Text_OptYourTeam:
	.string "YOUR TEAM$"
CoopDeal_Text_OptPartnerTeam:
	.string "PARTNER'S$"

CoopDeal_Text_ChooseFormat2:
	.string "Which format?$"

CoopDeal_Text_ChooseLevel2:
	.string "Which levels?$"

CoopDeal_Text_CoachAsking:
	.string "Getting a copy of your partner's\n"
	.string "team…$"

CoopDeal_Text_CoachNoPartner:
	.string "Link with your partner first, and\n"
	.string "I can copy their team.$"

CoopDeal_Text_CoachNoTeam:
	.string "Your partner's team didn't come\n"
	.string "through. Try again in a moment.$"

CoopDeal_Text_CoachTooFewMine:
	.string "You need two POKéMON that can\n"
	.string "battle for a double battle.$"

CoopDeal_Text_CoachTooFewFoe:
	.string "That team has only one POKéMON;\n"
	.string "a double battle needs two.$"

CoopDeal_Text_CoachGo:
	.string "Here come the copies!$"

CoopDeal_Text_CoachWon:
	.string "Well fought! That's the real thing\n"
	.string "next time.$"

CoopDeal_Text_CoachAfter:
	.string "Practice is for learning. Come\n"
	.string "back any time.$"

CoopDeal_Text_CoachBye:
	.string "Come back for practice any time.$"
