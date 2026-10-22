MtPyre_1F_MapScripts::
	.byte 0

MtPyre_1F_EventScript_CleanseTagWoman::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_CLEANSE_TAG, MtPyre_1F_EventScript_ReceivedCleanseTag
	msgbox MtPyre_1F_Text_TakeThisForYourOwnGood, MSGBOX_DEFAULT
	giveitem ITEM_CLEANSE_TAG
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_CLEANSE_TAG
	release
	end

MtPyre_1F_EventScript_ReceivedCleanseTag::
	msgbox MtPyre_1F_Text_ExplainCleanseTag, MSGBOX_DEFAULT
	release
	end

MtPyre_1F_EventScript_PokefanF::
	msgbox MtPyre_1F_Text_ComeToPayRespects, MSGBOX_NPC
	end

MtPyre_1F_EventScript_Man::
	msgbox MtPyre_1F_Text_RestingPlaceOfZigzagoon, MSGBOX_NPC
	end

MtPyre_1F_Text_TakeThisForYourOwnGood:
	.string "All sorts of beings wander the slopes\n"
	.string "of MT. PYRE…\p"
	.string "There is no telling what may happen.\n"
	.string "Take this. It's for your own good.$"

MtPyre_1F_Text_ExplainCleanseTag:
	.string "Have a POKéMON hold that\n"
	.string "CLEANSE TAG.\p"
	.string "It will help ward off wild POKéMON.$"

MtPyre_1F_Text_ComeToPayRespects:
	.string "Did you come to pay your respect\n"
	.string "to the spirits of departed POKéMON?\p"
	.string "You must care for your POKéMON a lot.$"

MtPyre_1F_Text_RestingPlaceOfZigzagoon:
	.string "This is the final resting place of my\n"
	.string "ZIGZAGOON. I cherished it…$"


MtPyre_1F_EventScript_GhostScout::
	lock
	faceplayer
	goto_if_set FLAG_COOP_QUEST_PYRE, MtPyre_1F_EventScript_GhostScoutDone
	msgbox MtPyre_1F_Text_GhostScoutAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MtPyre_1F_EventScript_GhostScoutDecline
	msgbox MtPyre_1F_Text_GhostScoutWhich, MSGBOX_DEFAULT
	chooseboxmon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, MtPyre_1F_EventScript_GhostScoutDecline
	specialvar VAR_RESULT, ScriptGetShownMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, MtPyre_1F_EventScript_GhostScoutEgg
	goto_if_eq VAR_RESULT, SPECIES_NONE, MtPyre_1F_EventScript_GhostScoutWrong
	goto_if_eq VAR_RESULT, SPECIES_GREAVARD, MtPyre_1F_EventScript_GhostScoutReward
	goto_if_eq VAR_RESULT, SPECIES_MIMIKYU, MtPyre_1F_EventScript_GhostScoutReward
	goto_if_eq VAR_RESULT, SPECIES_DREEPY, MtPyre_1F_EventScript_GhostScoutReward
	goto MtPyre_1F_EventScript_GhostScoutWrong

MtPyre_1F_EventScript_GhostScoutReward::
	msgbox MtPyre_1F_Text_GhostScoutYes, MSGBOX_DEFAULT
	giveitem ITEM_SPELL_TAG
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_COOP_QUEST_PYRE
	msgbox MtPyre_1F_Text_GhostScoutAfter, MSGBOX_DEFAULT
	release
	end

MtPyre_1F_EventScript_GhostScoutWrong::
	msgbox MtPyre_1F_Text_GhostScoutWrong, MSGBOX_DEFAULT
	release
	end

MtPyre_1F_EventScript_GhostScoutEgg::
	msgbox MtPyre_1F_Text_GhostScoutEgg, MSGBOX_DEFAULT
	release
	end

MtPyre_1F_EventScript_GhostScoutDecline::
	msgbox MtPyre_1F_Text_GhostScoutDecline, MSGBOX_DEFAULT
	release
	end

MtPyre_1F_EventScript_GhostScoutDone::
	msgbox MtPyre_1F_Text_GhostScoutDone, MSGBOX_DEFAULT
	release
	end

MtPyre_1F_Text_GhostScoutAsk::
	.string "New spirits wander the pyre.\n"
	.string "Greavard, Mimikyu, Dreepy.\p"
	.string "Show me any one of them.\n"
	.string "This Spell Tag is for you.$"

MtPyre_1F_Text_GhostScoutWhich::
	.string "Which spirit did you meet?$"

MtPyre_1F_Text_GhostScoutYes::
	.string "It trusts you.\n"
	.string "That matters in this place.$"

MtPyre_1F_Text_GhostScoutWrong::
	.string "I asked for a Greavard,\n"
	.string "a Mimikyu, or a Dreepy.$"

MtPyre_1F_Text_GhostScoutEgg::
	.string "Please don't show an Egg\n"
	.string "to the resting places.$"

MtPyre_1F_Text_GhostScoutDecline::
	.string "Walk softly if you go up.\n"
	.string "They startle easily.$"

MtPyre_1F_Text_GhostScoutAfter::
	.string "Mt. Pyre glows on the\n"
	.string "POKéDEX map for them.$"

MtPyre_1F_Text_GhostScoutDone::
	.string "The old families here\n"
	.string "would have liked the company.$"
