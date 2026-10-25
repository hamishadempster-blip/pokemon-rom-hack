@ The Twin Beacon's History (src/coop_quest.c, docs/COOP_QUESTS.md).
@ Three inscriptions outside the old ruins (OR-merged live) open a passage
@ in the SCORCHED SLAB (ledger: shrine open). Inside: twin plates (partner,
@ or the idol stone with Strength), twin lanterns (lit together, or alone
@ within a few seconds), a riddle in two half-verses. The sanctum (ledger:
@ solved) holds the lore, the Beacon styles and compass, and the Twin
@ Trial: three duo rounds, co-op with the partner or a double alone.
@ None of this gates linking, invitations or reconnection.

Coop_EventScript_BeaconMark::
	setvar VAR_0x8004, COOP_QUEST_SHRINE_MARKS
	special CoopQuest_SetBits
	call_if_lt VAR_SHRINE_QUEST, SHRINE_STARTED, Coop_EventScript_ShrineStarted
	specialvar VAR_RESULT, CoopQuest_CountBits
	goto_if_ge VAR_RESULT, 3, Coop_EventScript_BeaconMarksDone
	msgbox Coop_Text_BeaconMarkCount, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_BeaconMarksDone::
	msgbox Coop_Text_BeaconMarksDone, MSGBOX_DEFAULT
	releaseall
	end

Coop_EventScript_ShrineStarted::
	setvar VAR_SHRINE_QUEST, SHRINE_STARTED
	return

Route111_EventScript_BeaconMark::
	lockall
	msgbox Coop_Text_BeaconMarkDesert, MSGBOX_DEFAULT
	setvar VAR_0x8005, SHRINE_MARK_DESERT
	goto Coop_EventScript_BeaconMark

Route105_EventScript_BeaconMark::
	lockall
	msgbox Coop_Text_BeaconMarkIsland, MSGBOX_DEFAULT
	setvar VAR_0x8005, SHRINE_MARK_ISLAND
	goto Coop_EventScript_BeaconMark

Route120_EventScript_BeaconMark::
	lockall
	msgbox Coop_Text_BeaconMarkTomb, MSGBOX_DEFAULT
	setvar VAR_0x8005, SHRINE_MARK_TOMB
	goto Coop_EventScript_BeaconMark

@ The Scorched Slab: the passage opens once all three are read (or the
@ partner opened it). Checked as the map loads and at the back wall.
ScorchedSlab_EventScript_CheckPassage::
	goto_if_set FLAG_SHRINE_FOUND, Common_EventScript_NopReturn
	setvar VAR_0x8004, COOP_QUEST_SHRINE_MARKS
	specialvar VAR_RESULT, CoopQuest_CountBits
	goto_if_lt VAR_RESULT, 3, Common_EventScript_NopReturn
	setflag FLAG_SHRINE_FOUND
	call_if_lt VAR_SHRINE_QUEST, SHRINE_OPENED, ScorchedSlab_EventScript_SetOpened
	return

ScorchedSlab_EventScript_SetOpened::
	setvar VAR_SHRINE_QUEST, SHRINE_OPENED
	return

ScorchedSlab_EventScript_TwinWall::
	lockall
	goto_if_set FLAG_SHRINE_FOUND, ScorchedSlab_EventScript_TwinWallOpen
	msgbox ScorchedSlab_Text_TwinWall, MSGBOX_DEFAULT
	releaseall
	end

ScorchedSlab_EventScript_TwinWallOpen::
	msgbox ScorchedSlab_Text_TwinWallOpen, MSGBOX_DEFAULT
	releaseall
	end

Coop_Text_BeaconMarkDesert:
	.string "Weathered letters by the sealed\n"
	.string "door:\p"
	.string "“Two keepers tended two lights,\n"
	.string "so no traveller walked alone.”$"

Coop_Text_BeaconMarkIsland:
	.string "Letters cut into the cave mouth:\p"
	.string "“When one light failed, the other\n"
	.string "called it back across the sea.”$"

Coop_Text_BeaconMarkTomb:
	.string "An inscription beside the tomb:\p"
	.string "“Their hall lies where the slab\n"
	.string "burns, behind the twin flames.”$"

Coop_Text_BeaconMarkCount:
	.string "Twin-light inscriptions read:\n"
	.string "{STR_VAR_1} of 3.$"

Coop_Text_BeaconMarksDone:
	.string "All three speak of a hall behind\n"
	.string "twin flames, at the SCORCHED SLAB.$"

ScorchedSlab_Text_TwinWall:
	.string "Twin flames are carved into the\n"
	.string "rock. It sounds hollow…$"

ScorchedSlab_Text_TwinWallOpen:
	.string "The carved wall has opened onto a\n"
	.string "passage.$"

@ ----------------------------------------------------------- the shrine
TwinBeaconShrine_EventScript_Plate::
	goto_if_set FLAG_SHRINE_GATE_PLATES, Common_EventScript_NopReturn
	specialvar VAR_RESULT, CoopQuest_PlatesHeld
	goto_if_eq VAR_RESULT, FALSE, TwinBeaconShrine_EventScript_PlateAlone
	lockall
	playse SE_SWITCH
	setflag FLAG_SHRINE_GATE_PLATES
	special CoopQuest_ShareGates
	msgbox TwinBeaconShrine_Text_PlatesOpen, MSGBOX_DEFAULT
	releaseall
	return

TwinBeaconShrine_EventScript_PlateAlone::
	goto_if_set FLAG_TEMP_1, Common_EventScript_NopReturn
	setflag FLAG_TEMP_1
	lockall
	msgbox TwinBeaconShrine_Text_PlateAlone, MSGBOX_DEFAULT
	releaseall
	return

TwinBeaconShrine_EventScript_PlateWest::
	setvar VAR_0x8004, 1
	call TwinBeaconShrine_EventScript_Plate
	end

TwinBeaconShrine_EventScript_PlateEast::
	setvar VAR_0x8004, 2
	call TwinBeaconShrine_EventScript_Plate
	end

TwinBeaconShrine_EventScript_LanternWest::
	lock
	setvar VAR_0x8004, 1
	goto TwinBeaconShrine_EventScript_Lantern

TwinBeaconShrine_EventScript_LanternEast::
	lock
	setvar VAR_0x8004, 2
TwinBeaconShrine_EventScript_Lantern::
	goto_if_set FLAG_SHRINE_GATE_LANTERNS, TwinBeaconShrine_EventScript_LanternsLit
	goto_if_unset FLAG_SHRINE_GATE_PLATES, TwinBeaconShrine_EventScript_LanternCold
	specialvar VAR_RESULT, CoopQuest_LightLantern
	playse SE_M_EMBER
	goto_if_eq VAR_RESULT, TRUE, TwinBeaconShrine_EventScript_LanternsSolved
	msgbox TwinBeaconShrine_Text_LanternOne, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_EventScript_LanternsSolved::
	setflag FLAG_SHRINE_GATE_LANTERNS
	special CoopQuest_ShareGates
	msgbox TwinBeaconShrine_Text_LanternsSolved, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_EventScript_LanternCold::
	msgbox TwinBeaconShrine_Text_LanternCold, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_EventScript_LanternsLit::
	msgbox TwinBeaconShrine_Text_LanternsLit, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_EventScript_VerseWest::
	msgbox TwinBeaconShrine_Text_VerseWest, MSGBOX_SIGN
	end

TwinBeaconShrine_EventScript_VerseEast::
	msgbox TwinBeaconShrine_Text_VerseEast, MSGBOX_SIGN
	end

@ The sanctum door: the riddle, once both gates are open.
TwinBeaconShrine_EventScript_Door::
	lockall
	goto_if_set FLAG_SHRINE_SOLVED, TwinBeaconShrine_EventScript_DoorOpen
	goto_if_unset FLAG_SHRINE_GATE_LANTERNS, TwinBeaconShrine_EventScript_DoorDark
	setflag FLAG_SHRINE_RIDDLE_SEEN
	msgbox TwinBeaconShrine_Text_Riddle, MSGBOX_DEFAULT
	dynmultichoice 17, 3, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, TwinBeaconShrine_Text_AnsShadow, TwinBeaconShrine_Text_AnsBeacon, TwinBeaconShrine_Text_AnsRiver, TwinBeaconShrine_Text_AnsSecret
	goto_if_ne VAR_RESULT, 1, TwinBeaconShrine_EventScript_RiddleWrong
	playse SE_DOOR
	setflag FLAG_SHRINE_SOLVED
	special CoopQuest_ShareGates
	call TwinBeaconShrine_EventScript_OpenSanctum
	special DrawWholeMapView
	msgbox TwinBeaconShrine_Text_DoorOpens, MSGBOX_DEFAULT
	releaseall
	end

TwinBeaconShrine_EventScript_RiddleWrong::
	msgbox TwinBeaconShrine_Text_RiddleWrong, MSGBOX_DEFAULT
	releaseall
	end

TwinBeaconShrine_EventScript_DoorDark::
	msgbox TwinBeaconShrine_Text_DoorDark, MSGBOX_DEFAULT
	releaseall
	end

TwinBeaconShrine_EventScript_DoorOpen::
	msgbox TwinBeaconShrine_Text_DoorOpen, MSGBOX_DEFAULT
	releaseall
	end

TwinBeaconShrine_EventScript_OpenSanctum::
	setmetatile 7, 19, METATILE_Cave_SealedChamberEntrance_TopLeft, TRUE
	setmetatile 8, 19, METATILE_Cave_SealedChamberEntrance_TopMid, TRUE
	setmetatile 9, 19, METATILE_Cave_SealedChamberEntrance_TopRight, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberEntrance_BottomLeft, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberEntrance_BottomMid, FALSE
	setmetatile 9, 20, METATILE_Cave_SealedChamberEntrance_BottomRight, TRUE
	return

TwinBeaconShrine_EventScript_Tablet::
	msgbox TwinBeaconShrine_Text_Tablet, MSGBOX_SIGN
	end

@ The keeper: lore (unlocks the Beacon styles and the compass), the style
@ choice, and the Twin Trial.
TwinBeaconShrine_EventScript_Keeper::
	lock
	faceplayer
	goto_if_set FLAG_SHRINE_LORE_READ, TwinBeaconShrine_EventScript_KeeperMenu
	msgbox TwinBeaconShrine_Text_KeeperLore, MSGBOX_DEFAULT
	setflag FLAG_SHRINE_LORE_READ
	msgbox TwinBeaconShrine_Text_KeeperGift, MSGBOX_DEFAULT
TwinBeaconShrine_EventScript_KeeperMenu::
	goto_if_unset FLAG_TOURNEY_WON, TwinBeaconShrine_EventScript_KeeperAsk
	setvar VAR_0x8004, COOP_REWARD_TOURNEY
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, FALSE, TwinBeaconShrine_EventScript_TrialFirstPrize
TwinBeaconShrine_EventScript_KeeperAsk::
	message TwinBeaconShrine_Text_KeeperAsk
	waitmessage
	dynmultichoice 17, 1, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, TwinBeaconShrine_Text_OptTrial, TwinBeaconShrine_Text_OptStyle, TwinBeaconShrine_Text_OptLore, Coop_Text_OptBack
	switch VAR_RESULT
	case 0, TwinBeaconShrine_EventScript_Trial
	case 1, TwinBeaconShrine_EventScript_Style
	case 2, TwinBeaconShrine_EventScript_Lore
	msgbox TwinBeaconShrine_Text_KeeperBye, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_EventScript_Lore::
	msgbox TwinBeaconShrine_Text_KeeperLore, MSGBOX_DEFAULT
	goto TwinBeaconShrine_EventScript_KeeperAsk

TwinBeaconShrine_EventScript_Style::
	message TwinBeaconShrine_Text_StyleAsk
	waitmessage
	dynmultichoice 18, 1, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, TwinBeaconShrine_Text_StyleClassic, TwinBeaconShrine_Text_StyleGold, TwinBeaconShrine_Text_StyleAzure, TwinBeaconShrine_Text_StyleRose
	goto_if_ge VAR_RESULT, BEACON_STYLE_COUNT, TwinBeaconShrine_EventScript_KeeperAsk
	copyvar VAR_BEACON_STYLE, VAR_RESULT
	msgbox TwinBeaconShrine_Text_StyleSet, MSGBOX_DEFAULT
	goto TwinBeaconShrine_EventScript_KeeperAsk

@ The Twin Trial: three rounds, each against a pair, one round per talk
@ (VAR_SHRINE_TOURNEY: the next round, 0 when no trial is under way; it
@ starts over when the shrine is entered again or a round is lost). With
@ the partner here (Beacon on, both agree) a round is a co-op multi battle;
@ alone, a double against both. Teams are restored before every round.
TwinBeaconShrine_EventScript_Trial::
	goto_if_ne VAR_SHRINE_TOURNEY, 0, TwinBeaconShrine_EventScript_TrialRound
	msgbox TwinBeaconShrine_Text_TrialRules, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, TwinBeaconShrine_EventScript_KeeperAsk
	setvar VAR_SHRINE_TOURNEY, 1
TwinBeaconShrine_EventScript_TrialRound::
	special CoopQuest_TrialHeal
	setvar VAR_TEMP_6, 0
	specialvar VAR_RESULT, CoopQuest_DuoReady
	goto_if_eq VAR_RESULT, FALSE, TwinBeaconShrine_EventScript_TrialPair
	special CoopDeal_BufferPartnerName
	msgbox TwinBeaconShrine_Text_TrialAskDuo, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, TwinBeaconShrine_EventScript_TrialPair
	setvar VAR_TEMP_6, 1
TwinBeaconShrine_EventScript_TrialPair::
	setvar VAR_0x8007, 0
	switch VAR_SHRINE_TOURNEY
	case 1, TwinBeaconShrine_EventScript_TrialRound1
	case 2, TwinBeaconShrine_EventScript_TrialRound2
	goto_if_set FLAG_TOURNEY_WON, TwinBeaconShrine_EventScript_FinalRepeat
	msgbox TwinBeaconShrine_Text_Final, MSGBOX_DEFAULT
	setvar VAR_0x8004, TRAINER_BT_FINAL_LUMEN
	setvar VAR_0x8005, TRAINER_BT_FINAL_UMBRA
	setvar VAR_0x8007, FLAG_TOURNEY_WON
	call TwinBeaconShrine_EventScript_TrialBattle
	setvar VAR_SHRINE_TOURNEY, 0
	setflag FLAG_TOURNEY_WON
	special CoopQuest_TrialHeal
	goto TwinBeaconShrine_EventScript_TrialFirstPrize

TwinBeaconShrine_EventScript_TrialRound1::
	msgbox TwinBeaconShrine_Text_Round1, MSGBOX_DEFAULT
	setvar VAR_0x8004, TRAINER_BT_ROUND1_SOL
	setvar VAR_0x8005, TRAINER_BT_ROUND1_LUNA
	goto TwinBeaconShrine_EventScript_TrialNext

TwinBeaconShrine_EventScript_TrialRound2::
	msgbox TwinBeaconShrine_Text_Round2, MSGBOX_DEFAULT
	setvar VAR_0x8004, TRAINER_BT_ROUND2_EMBER
	setvar VAR_0x8005, TRAINER_BT_ROUND2_FROST
TwinBeaconShrine_EventScript_TrialNext::
	call TwinBeaconShrine_EventScript_TrialBattle
	addvar VAR_SHRINE_TOURNEY, 1
	special CoopQuest_TrialHeal
	msgbox TwinBeaconShrine_Text_RoundWon, MSGBOX_DEFAULT
	release
	end

@ One round: VAR_0x8004/5 the pair, VAR_0x8007 a flag both saves get.
@ VAR_TEMP_6 1: with the partner (co-op multi; this script resumes after a
@ win); 0, or the partner can't join: alone, a double against both. A lost
@ round is the stock white-out.
TwinBeaconShrine_EventScript_TrialBattle::
	goto_if_eq VAR_TEMP_6, 0, TwinBeaconShrine_EventScript_TrialSoloBattle
	setvar VAR_0x8006, 0
	special Coop_StartStoryBattle
	goto_if_eq VAR_RESULT, TRUE, Common_EventScript_NopReturn
TwinBeaconShrine_EventScript_TrialSoloBattle::
	special CoopQuest_ForceSolo
	special CoopQuest_TrialSoloBattle
	return

TwinBeaconShrine_EventScript_FinalRepeat::
	msgbox TwinBeaconShrine_Text_Final, MSGBOX_DEFAULT
	setvar VAR_0x8004, TRAINER_BT_REPEAT_LUMEN
	setvar VAR_0x8005, TRAINER_BT_REPEAT_UMBRA
	call TwinBeaconShrine_EventScript_TrialBattle
	setvar VAR_SHRINE_TOURNEY, 0
	special CoopQuest_TrialHeal
	goto_if_set FLAG_DAILY_SHRINE_TOURNEY, TwinBeaconShrine_EventScript_TrialDoneToday
	checkitemspace ITEM_RARE_CANDY
	goto_if_eq VAR_RESULT, FALSE, TwinBeaconShrine_EventScript_TrialDoneToday
	msgbox TwinBeaconShrine_Text_RepeatPrize, MSGBOX_DEFAULT
	giveitem ITEM_RARE_CANDY
	setflag FLAG_DAILY_SHRINE_TOURNEY
	release
	end

TwinBeaconShrine_EventScript_TrialDoneToday::
	msgbox TwinBeaconShrine_Text_TrialDoneToday, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_EventScript_TrialFirstPrize::
	checkitemspace ITEM_ABILITY_CAPSULE
	goto_if_eq VAR_RESULT, FALSE, TwinBeaconShrine_EventScript_TrialBagFull
	msgbox TwinBeaconShrine_Text_FirstPrize, MSGBOX_DEFAULT
	giveitem ITEM_ABILITY_CAPSULE
	setvar VAR_0x8004, COOP_REWARD_TOURNEY
	special CoopQuest_ClaimReward
	msgbox TwinBeaconShrine_Text_TrialAfter, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_EventScript_TrialBagFull::
	msgbox TwinBeaconShrine_Text_TrialBagFull, MSGBOX_DEFAULT
	release
	end

TwinBeaconShrine_Text_PlatesOpen:
	.string "Both plates sink at once. Far off,\n"
	.string "stone grinds against stone.$"

TwinBeaconShrine_Text_PlateAlone:
	.string "The plate gives a little, then\n"
	.string "rises. Its twin across the hall\l"
	.string "needs weight too.$"

TwinBeaconShrine_Text_LanternOne:
	.string "The statue's lamp flares… and\n"
	.string "starts to fade. Its twin is dark.$"

TwinBeaconShrine_Text_LanternsSolved:
	.string "Both lamps burn bright together!\n"
	.string "The sanctum door glows.$"

TwinBeaconShrine_Text_LanternCold:
	.string "A cold stone lamp. Nothing here\n"
	.string "stirs yet.$"

TwinBeaconShrine_Text_LanternsLit:
	.string "The twin lamps burn steadily.$"

TwinBeaconShrine_Text_VerseWest:
	.string "“Two keepers stood on twin\n"
	.string "stones here. Apart, dim…”$"

TwinBeaconShrine_Text_VerseEast:
	.string "“…together they shine.\n"
	.string "Name what they share.”$"

TwinBeaconShrine_Text_Riddle:
	.string "The door asks in glowing letters:\n"
	.string "“What do the keepers share?”$"

TwinBeaconShrine_Text_AnsShadow:
	.string "A SHADOW$"
TwinBeaconShrine_Text_AnsBeacon:
	.string "A BEACON$"
TwinBeaconShrine_Text_AnsRiver:
	.string "A RIVER$"
TwinBeaconShrine_Text_AnsSecret:
	.string "A SECRET$"

TwinBeaconShrine_Text_RiddleWrong:
	.string "The letters dim. That was not it.$"

TwinBeaconShrine_Text_DoorOpens:
	.string "“A BEACON.” The door slides open!$"

TwinBeaconShrine_Text_DoorDark:
	.string "A sealed door. Twin lamps are\n"
	.string "carved above it, both unlit.$"

TwinBeaconShrine_Text_DoorOpen:
	.string "The sanctum door stands open.$"

TwinBeaconShrine_Text_Tablet:
	.string "“The first BEACON was two lamps.\n"
	.string "One keeper on each shore.\p"
	.string "When storms parted them, they\n"
	.string "followed each other's light home.”$"

TwinBeaconShrine_Text_KeeperLore:
	.string "KEEPER: Few find this hall. Hear\n"
	.string "its story.\p"
	.string "Two keepers once kept two lights,\n"
	.string "so travellers were never alone.\p"
	.string "Your TWIN BEACON carries that same\n"
	.string "light. It always has. Nothing here\l"
	.string "changes how it links you.$"

TwinBeaconShrine_Text_KeeperGift:
	.string "But it can shine in new colours,\n"
	.string "and point the way to your partner.\p"
	.string "Your partner's marker now glows in\n"
	.string "the BEACON STYLE you choose, and\l"
	.string "shows at the screen edge when they\l"
	.string "are out of sight on the same map.$"

TwinBeaconShrine_Text_KeeperAsk:
	.string "KEEPER: What do you seek?$"

TwinBeaconShrine_Text_OptTrial:
	.string "TWIN TRIAL$"
TwinBeaconShrine_Text_OptStyle:
	.string "BEACON STYLE$"
TwinBeaconShrine_Text_OptLore:
	.string "THE STORY$"

TwinBeaconShrine_Text_KeeperBye:
	.string "KEEPER: May your lights find each\n"
	.string "other.$"

TwinBeaconShrine_Text_StyleAsk:
	.string "Which colour for the marker?$"

TwinBeaconShrine_Text_StyleClassic:
	.string "CLASSIC$"
TwinBeaconShrine_Text_StyleGold:
	.string "GOLD$"
TwinBeaconShrine_Text_StyleAzure:
	.string "AZURE$"
TwinBeaconShrine_Text_StyleRose:
	.string "ROSE$"

TwinBeaconShrine_Text_StyleSet:
	.string "Your BEACON shines anew.$"

TwinBeaconShrine_Text_TrialRules:
	.string "KEEPER: The TWIN TRIAL: three\n"
	.string "rounds, each against a pair.\p"
	.string "With your partner beside you, fight\n"
	.string "as one. Alone, face both at once.\p"
	.string "Your team is restored before each\n"
	.string "round. Leave, or lose, and it\l"
	.string "starts over. Begin?$"

TwinBeaconShrine_Text_RoundWon:
	.string "KEEPER: Well fought! Speak to me\n"
	.string "when you are ready for the next.$"

TwinBeaconShrine_Text_TrialAskDuo:
	.string "KEEPER: {STR_VAR_1} is here too.\n"
	.string "Face the trial together?$"

TwinBeaconShrine_Text_Round1:
	.string "Round one: SOL and LUNA!$"
TwinBeaconShrine_Text_Round2:
	.string "Round two: EMBER and FROST!$"
TwinBeaconShrine_Text_Final:
	.string "The final round: LUMEN and UMBRA!$"

TwinBeaconShrine_Text_SolLose:
	.string "The sun sets on me.$"
TwinBeaconShrine_Text_LunaLose:
	.string "Eclipsed…$"
TwinBeaconShrine_Text_EmberLose:
	.string "Burnt out!$"
TwinBeaconShrine_Text_FrostLose:
	.string "Melted…$"
TwinBeaconShrine_Text_LumenLose:
	.string "Your light is the brighter.$"
TwinBeaconShrine_Text_UmbraLose:
	.string "Even the dark makes way.$"

TwinBeaconShrine_Text_FirstPrize:
	.string "KEEPER: The trial is yours! Take\n"
	.string "the keepers' prize.$"

TwinBeaconShrine_Text_TrialAfter:
	.string "KEEPER: Return any day. The trial\n"
	.string "is always open.$"

TwinBeaconShrine_Text_TrialBagFull:
	.string "KEEPER: Your bag is full. Your\n"
	.string "prize will wait for you here.$"

TwinBeaconShrine_Text_RepeatPrize:
	.string "KEEPER: Well fought. Today's\n"
	.string "token of the trial.$"

TwinBeaconShrine_Text_TrialDoneToday:
	.string "KEEPER: Well fought. The token\n"
	.string "is given once a day.$"
