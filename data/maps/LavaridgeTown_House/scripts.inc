LavaridgeTown_House_MapScripts::
	.byte 0

LavaridgeTown_House_EventScript_OldMan::
	msgbox LavaridgeTown_House_Text_WifeWarmingEggInHotSprings, MSGBOX_NPC
	end

LavaridgeTown_House_EventScript_Zigzagoon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZIGZAGOON, CRY_MODE_NORMAL
	msgbox LavaridgeTown_House_Text_Zigzagoon, MSGBOX_DEFAULT
	waitmoncry
	release
	end

LavaridgeTown_House_Text_WifeWarmingEggInHotSprings:
	.string "My wife's warming an EGG in the hot\n"
	.string "springs. This is what she told me.\p"
	.string "She left two POKéMON with the DAY CARE.\n"
	.string "And they discovered that EGG!$"

LavaridgeTown_House_Text_Zigzagoon:
	.string "ZIGZAGOON: Pshoo!$"


@ One female SALANDIT per save. Only females evolve, and few hatch female.
LavaridgeTown_House_EventScript_SalanditKeeper::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_FEMALE_SALANDIT, LavaridgeTown_House_EventScript_SalanditHint
	msgbox LavaridgeTown_House_Text_SalanditOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, LavaridgeTown_House_EventScript_SalanditDeclined
	givemon SPECIES_SALANDIT, 15, gender=MON_FEMALE
	goto_if_eq VAR_RESULT, MON_GIVEN_TO_PARTY, LavaridgeTown_House_EventScript_SalanditParty
	goto_if_eq VAR_RESULT, MON_GIVEN_TO_PC, LavaridgeTown_House_EventScript_SalanditPC
	goto Common_EventScript_NoMoreRoomForPokemon
	end

LavaridgeTown_House_EventScript_SalanditParty::
	call LavaridgeTown_House_EventScript_SalanditFanfare
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, LavaridgeTown_House_EventScript_SalanditGiven
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	goto LavaridgeTown_House_EventScript_SalanditGiven
	end

LavaridgeTown_House_EventScript_SalanditPC::
	call LavaridgeTown_House_EventScript_SalanditFanfare
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, LavaridgeTown_House_EventScript_SalanditSentToPC
	call Common_EventScript_NameReceivedBoxMon
	goto LavaridgeTown_House_EventScript_SalanditSentToPC
	end

LavaridgeTown_House_EventScript_SalanditSentToPC::
	call Common_EventScript_TransferredToPC
	goto LavaridgeTown_House_EventScript_SalanditGiven
	end

LavaridgeTown_House_EventScript_SalanditFanfare::
	bufferspeciesname STR_VAR_1, SPECIES_SALANDIT
	playfanfare MUS_OBTAIN_ITEM
	message LavaridgeTown_House_Text_ReceivedSalandit
	waitmessage
	waitfanfare
	return

LavaridgeTown_House_EventScript_SalanditGiven::
	setflag FLAG_RECEIVED_FEMALE_SALANDIT
	goto LavaridgeTown_House_EventScript_SalanditHint
	end

LavaridgeTown_House_EventScript_SalanditDeclined::
	msgbox LavaridgeTown_House_Text_SalanditDeclined, MSGBOX_DEFAULT
	release
	end

LavaridgeTown_House_EventScript_SalanditHint::
	msgbox LavaridgeTown_House_Text_SalanditHint, MSGBOX_DEFAULT
	release
	end

LavaridgeTown_House_Text_SalanditOffer:
	.string "I raise SALANDIT from FIERY PATH.\n"
	.string "Only the females evolve, into SALAZZLE,\l"
	.string "and only one in eight is female.\p"
	.string "Would you like one of my females?$"

LavaridgeTown_House_Text_ReceivedSalandit:
	.string "{PLAYER} received a female\n"
	.string "SALANDIT!$"

LavaridgeTown_House_Text_SalanditDeclined:
	.string "Come back if you change your mind.\n"
	.string "I have one for each trainer who asks.$"

LavaridgeTown_House_Text_SalanditHint:
	.string "A female SALANDIT evolves into\n"
	.string "SALAZZLE at level 33.\l"
	.string "Males never evolve.\p"
	.string "Wild ones live in FIERY PATH and the\n"
	.string "MAGMA HIDEOUT.$"
