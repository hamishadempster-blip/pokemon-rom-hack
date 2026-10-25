@ The Desert Caravan, Route 111 (src/coop_quest.c, docs/COOP_QUESTS.md).
@ Personal: VAR_CARAVAN_QUEST, the raiders, the glyph chamber, SIGILYPH,
@ the reward. Shared: the crates (OR-merged live) and "resupplied"
@ (ledger milestone: the merchant opens on both saves; catch-up flag).

Route111_EventScript_SetCaravanCamp::
	setflag FLAG_HIDE_CARAVAN_CAMP
	goto_if_lt VAR_CARAVAN_QUEST, CARAVAN_MERCHANT, Common_EventScript_NopReturn
	clearflag FLAG_HIDE_CARAVAN_CAMP
	return

Route111_EventScript_CaravanBriefed::
	setvar VAR_CARAVAN_QUEST, CARAVAN_BRIEFED
	return

@ ZAHRA, the caravan's leader.
Route111_EventScript_Zahra::
	lock
	faceplayer
	goto_if_ge VAR_CARAVAN_QUEST, CARAVAN_MERCHANT, Route111_EventScript_ZahraMerchant
	setvar VAR_0x8004, COOP_QUEST_CARAVAN_FOUND
	specialvar VAR_RESULT, CoopQuest_CountBits
	goto_if_ge VAR_RESULT, 3, Route111_EventScript_ZahraSupplied
	goto_if_ge VAR_CARAVAN_QUEST, CARAVAN_BRIEFED, Route111_EventScript_ZahraWaiting
	msgbox Route111_Text_ZahraBrief, MSGBOX_DEFAULT
	call Route111_EventScript_CaravanBriefed
	release
	end

Route111_EventScript_ZahraWaiting::
	msgbox Route111_Text_ZahraWaiting, MSGBOX_DEFAULT
	release
	end

Route111_EventScript_ZahraSupplied::
	msgbox Route111_Text_ZahraSupplied, MSGBOX_DEFAULT
	setvar VAR_CARAVAN_QUEST, CARAVAN_MERCHANT
	clearflag FLAG_HIDE_CARAVAN_CAMP
	addobject LOCALID_ROUTE111_CARAVAN_MERCHANT
	goto Route111_EventScript_ZahraGift

Route111_EventScript_ZahraMerchant::
	setvar VAR_0x8004, COOP_REWARD_CARAVAN
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, TRUE, Route111_EventScript_ZahraAfter
	goto_if_unset FLAG_CARAVAN_CATCHUP, Route111_EventScript_ZahraGift
	msgbox Route111_Text_ZahraCatchup, MSGBOX_DEFAULT
Route111_EventScript_ZahraGift::
	checkitemspace ITEM_SAFETY_GOGGLES
	goto_if_eq VAR_RESULT, FALSE, Route111_EventScript_ZahraBagFull
	msgbox Route111_Text_ZahraGift, MSGBOX_DEFAULT
	giveitem ITEM_SAFETY_GOGGLES
	setvar VAR_0x8004, COOP_REWARD_CARAVAN
	special CoopQuest_ClaimReward
Route111_EventScript_ZahraAfter::
	msgbox Route111_Text_ZahraAfter, MSGBOX_DEFAULT
	release
	end

Route111_EventScript_ZahraBagFull::
	msgbox Route111_Text_ZahraBagFull, MSGBOX_DEFAULT
	release
	end

@ The caravan's MERCHANT: open every day once resupplied. Essentials are
@ always stocked; desert goods and stronger supplies come with badges.
Route111_EventScript_CaravanMerchant::
	lock
	faceplayer
	message Route111_Text_MerchantHello
	waitmessage
	goto_if_set FLAG_BADGE06_GET, Route111_EventScript_CaravanMart2
	pokemart Route111_Pokemart_Caravan1
	goto Route111_EventScript_CaravanMartEnd
Route111_EventScript_CaravanMart2::
	pokemart Route111_Pokemart_Caravan2
Route111_EventScript_CaravanMartEnd::
	msgbox Route111_Text_MerchantBye, MSGBOX_DEFAULT
	release
	end

	.align 2
Route111_Pokemart_Caravan1:
	.2byte ITEM_POKE_BALL
	.2byte ITEM_GREAT_BALL
	.2byte ITEM_POTION
	.2byte ITEM_SUPER_POTION
	.2byte ITEM_ANTIDOTE
	.2byte ITEM_PARALYZE_HEAL
	.2byte ITEM_REPEL
	.2byte ITEM_SOFT_SAND
	.2byte ITEM_SMOOTH_ROCK
	.2byte ITEM_HEAT_ROCK
	pokemartlistend

	.align 2
Route111_Pokemart_Caravan2:
	.2byte ITEM_POKE_BALL
	.2byte ITEM_GREAT_BALL
	.2byte ITEM_ULTRA_BALL
	.2byte ITEM_DUSK_BALL
	.2byte ITEM_SUPER_POTION
	.2byte ITEM_HYPER_POTION
	.2byte ITEM_FULL_HEAL
	.2byte ITEM_REVIVE
	.2byte ITEM_SUPER_REPEL
	.2byte ITEM_SOFT_SAND
	.2byte ITEM_SMOOTH_ROCK
	.2byte ITEM_HEAT_ROCK
	.2byte ITEM_SAFETY_GOGGLES
	pokemartlistend

Route111_EventScript_CaravanMudsdale::
	lock
	faceplayer
	playmoncry SPECIES_MUDSDALE, CRY_MODE_NORMAL
	msgbox Route111_Text_CaravanMudsdale, MSGBOX_DEFAULT
	waitmoncry
	release
	end

Route111_EventScript_CaravanHand::
	lock
	faceplayer
	goto_if_ge VAR_CARAVAN_QUEST, CARAVAN_MERCHANT, Route111_EventScript_CaravanHandAfter
	msgbox Route111_Text_CaravanHand, MSGBOX_DEFAULT
	release
	end

Route111_EventScript_CaravanHandAfter::
	msgbox Route111_Text_CaravanHandAfter, MSGBOX_DEFAULT
	release
	end

Route111_Text_ZahraBrief:
	.string "ZAHRA: Welcome to our camp,\n"
	.string "traveller. Though it's a poor one.\p"
	.string "The sandstorm scattered our supply\n"
	.string "crates, and raiders hauled three\l"
	.string "of them into the SANDBURIED RUINS.\p"
	.string "The ruins stand in the desert,\n"
	.string "east of the MIRAGE TOWER.\p"
	.string "If you find them, tell me. Oh, and\n"
	.string "our old map says of those ruins:\p"
	.string "“The sun sets, the star rises,\n"
	.string "then the moon opens the way.”$"

Route111_Text_ZahraWaiting:
	.string "ZAHRA: Three crates, in the ruins\n"
	.string "east of the MIRAGE TOWER.\p"
	.string "“The sun sets, the star rises,\n"
	.string "then the moon opens the way.”$"

Route111_Text_ZahraSupplied:
	.string "ZAHRA: All three crates! The\n"
	.string "caravan can trade again.\p"
	.string "Our MERCHANT will set up shop\n"
	.string "right here, every day.$"

Route111_Text_ZahraCatchup:
	.string "ZAHRA: Your partner brought the\n"
	.string "last crates. You're both caravan\l"
	.string "friends now.$"

Route111_Text_ZahraGift:
	.string "Take these. No sandstorm will\n"
	.string "sting your POKéMON's eyes again.$"

Route111_Text_ZahraAfter:
	.string "ZAHRA: We stay at this camp now.\n"
	.string "The MERCHANT trades every day.\p"
	.string "SCRAGGY and MUDBRAY roam the dunes\n"
	.string "and the ruins, if you're curious.$"

Route111_Text_ZahraBagFull:
	.string "ZAHRA: Your bag is full. Come back\n"
	.string "for your gift when you have room.$"

Route111_Text_MerchantHello:
	.string "Caravan goods, fresh off the dunes!\n"
	.string "Open every day.$"

Route111_Text_MerchantBye:
	.string "Safe travels across the sand!$"

Route111_Text_CaravanMudsdale:
	.string "MUDSDALE: Hrrrumph!\p"
	.string "It carries the caravan's packs.$"

Route111_Text_CaravanHand:
	.string "Those raiders! If we had our\n"
	.string "supplies, we'd be trading by now.$"

Route111_Text_CaravanHandAfter:
	.string "Business is good! Buy something\n"
	.string "from our MERCHANT, won't you?$"
