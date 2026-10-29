@ Battle Guide v2: the Mega Bracelet (ITEM_MEGA_RING, shown as MEGA BRACELET).
@ Each save claims its own, once, after the fifth badge: from Steven on
@ Route 118 (the scene after Norman), or, for a save that has already had
@ that scene or had no room, from the MEGA STONE counter of the Trainer
@ Exchange in Rustboro. Nothing is shared with the partner's save.

@ Steven's part of the Route 118 scene (called after his first lines).
Coop_EventScript_StevenMegaBracelet::
	goto_if_set FLAG_RECEIVED_MEGA_BRACELET, Coop_EventScript_MegaBraceletDone
	goto_if_unset FLAG_BADGE05_GET, Coop_EventScript_MegaBraceletDone
	msgbox Coop_Text_StevenMegaIntro, MSGBOX_DEFAULT
	call Coop_EventScript_GiveMegaBracelet
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StevenBraceletLater
	msgbox Coop_Text_StevenMegaExplain, MSGBOX_DEFAULT
Coop_EventScript_MegaBraceletDone::
	return

Coop_EventScript_StevenBraceletLater::
	msgbox Coop_Text_StevenBraceletLater, MSGBOX_DEFAULT
	return

@ VAR_RESULT TRUE when the bracelet is in the bag now (also when it already
@ was), FALSE when the Key Items pocket had no room.
Coop_EventScript_GiveMegaBracelet::
	checkitem ITEM_MEGA_RING, 1
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_MegaBraceletOwned
	giveitem ITEM_MEGA_RING
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_MegaBraceletNoRoom
Coop_EventScript_MegaBraceletOwned::
	setflag FLAG_RECEIVED_MEGA_BRACELET
	setvar VAR_RESULT, TRUE
	return

Coop_EventScript_MegaBraceletNoRoom::
	setvar VAR_RESULT, FALSE
	return

Coop_Text_StevenMegaIntro:
	.string "STEVEN: One more thing.\p"
	.string "You beat NORMAN. Then you're ready\n"
	.string "for this. It's a MEGA BRACELET.\p"
	.string "The KEY STONE in it answers a\n"
	.string "MEGA STONE that a POKéMON holds.$"

Coop_Text_StevenMegaExplain:
	.string "STEVEN: Let it hold its MEGA STONE.\n"
	.string "On the move list, press START to\l"
	.string "light the trigger, then pick a move.\p"
	.string "Each TRAINER can do it once in a\n"
	.string "battle. It ends with the battle.\p"
	.string "The TRAINER EXCHANGE in RUSTBORO\n"
	.string "sells MEGA STONES as you earn\l"
	.string "BADGES. Some lie out in HOENN too.$"

Coop_Text_StevenBraceletLater:
	.string "STEVEN: Your bag is full. I'll leave\n"
	.string "it at the TRAINER EXCHANGE in\l"
	.string "RUSTBORO. Ask at the MEGA counter.$"

@ A quest's Mega Stone, once per save, after its reward (also for a save
@ that finished the quest before this build). A full bag leaves it for the
@ next talk (checked first, so nothing is announced and lost); the Exchange sells it again from 5 BADGES once the reward is in.
Coop_EventScript_StoneCoast::
	goto_if_set FLAG_ITEM_COOP_MEGA_DRAGALGITE, Coop_EventScript_StoneDone
	checkitemspace ITEM_DRAGALGITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneNoRoom
	msgbox Coop_Text_QuestStone, MSGBOX_DEFAULT
	giveitem ITEM_DRAGALGITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneDone
	setflag FLAG_ITEM_COOP_MEGA_DRAGALGITE
	return

Coop_EventScript_StoneCaravan::
	goto_if_set FLAG_ITEM_COOP_MEGA_GARCHOMPITE, Coop_EventScript_StoneDone
	checkitemspace ITEM_GARCHOMPITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneNoRoom
	msgbox Coop_Text_QuestStone, MSGBOX_DEFAULT
	giveitem ITEM_GARCHOMPITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneDone
	setflag FLAG_ITEM_COOP_MEGA_GARCHOMPITE
	return

Coop_EventScript_StoneWattson::
	goto_if_set FLAG_ITEM_COOP_MEGA_EELEKTROSSITE, Coop_EventScript_StoneDone
	checkitemspace ITEM_EELEKTROSSITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneNoRoom
	msgbox Coop_Text_QuestStone, MSGBOX_DEFAULT
	giveitem ITEM_EELEKTROSSITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneDone
	setflag FLAG_ITEM_COOP_MEGA_EELEKTROSSITE
	return

Coop_EventScript_StoneShoal::
	goto_if_set FLAG_ITEM_COOP_MEGA_FROSLASSITE, Coop_EventScript_StoneDone
	checkitemspace ITEM_FROSLASSITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneNoRoom
	msgbox Coop_Text_QuestStone, MSGBOX_DEFAULT
	giveitem ITEM_FROSLASSITE
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_StoneDone
	setflag FLAG_ITEM_COOP_MEGA_FROSLASSITE
Coop_EventScript_StoneDone::
	return

Coop_EventScript_StoneNoRoom::
	msgbox Coop_Text_QuestStoneNoRoom, MSGBOX_DEFAULT
	return

Coop_Text_QuestStone:
	.string "This turned up while you were at it.\n"
	.string "A MEGA STONE, I think. It's yours.$"

Coop_Text_QuestStoneNoRoom:
	.string "I've got a MEGA STONE for you, but\n"
	.string "your BAG is full. Come back for it.$"
