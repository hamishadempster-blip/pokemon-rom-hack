@ Mira and Soren: Separate Ambitions (src/coop_quest.c, docs/COOP_QUESTS.md).
@ SOREN trains to be stronger (Slateport, Fortree, Mossdeep, two spars);
@ MIRA studies habitats and odd events (Route 109, the caravan camp,
@ Shoal Cave). A meeting this save played is in VAR_TWIN_ARC; one the
@ partner played arrives live as VAR_TWIN_HEARD and gets the short version
@ here, never the full scene twice. The exhibition in Littleroot: alone it
@ is a double against both twins; with the partner (Beacon on, same map,
@ both agree) a co-op multi battle (Coop_StartStoryBattle) that sets
@ FLAG_TWIN_EXH_WON on both saves, so the partner collects the gifts later.

Coop_EventScript_TwinMark::
	special CoopQuest_TwinMark
	return

@ ---------------------------------------------------------------- SOREN
Coop_EventScript_TwinS1::
	lock
	faceplayer
	setvar VAR_0x8004, TWIN_ARC_S1
	specialvar VAR_RESULT, CoopQuest_TwinState
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TwinS1Spar
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TwinS1Short
	msgbox Coop_Text_TwinS1, MSGBOX_DEFAULT
	goto Coop_EventScript_TwinS1Seen
Coop_EventScript_TwinS1Short::
	msgbox Coop_Text_TwinS1Short, MSGBOX_DEFAULT
Coop_EventScript_TwinS1Seen::
	setvar VAR_0x8004, TWIN_ARC_S1
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinS1Spar::
	setvar VAR_0x8004, TWIN_ARC_SPAR_1
	specialvar VAR_RESULT, CoopQuest_TwinHas
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_TwinS1Prize
	msgbox Coop_Text_TwinSparAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_TwinSparLater
	special CoopQuest_ForceSolo
	trainerbattle_no_intro TRAINER_TWIN_SPAR_1, Coop_Text_TwinSpar1Lose
	setvar VAR_0x8004, TWIN_ARC_SPAR_1
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinS1Prize::
	setvar VAR_0x8004, COOP_REWARD_TWIN_SPAR_1
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_TwinS1After
	checkitemspace ITEM_PP_UP
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TwinBagFull
	msgbox Coop_Text_TwinSparPrize, MSGBOX_DEFAULT
	giveitem ITEM_PP_UP
	setvar VAR_0x8004, COOP_REWARD_TWIN_SPAR_1
	special CoopQuest_ClaimReward
Coop_EventScript_TwinS1After::
	msgbox Coop_Text_TwinS1After, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_TwinS2::
	lock
	faceplayer
	setvar VAR_0x8004, TWIN_ARC_S2
	specialvar VAR_RESULT, CoopQuest_TwinState
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TwinS2Spar
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TwinS2Short
	msgbox Coop_Text_TwinS2, MSGBOX_DEFAULT
	goto Coop_EventScript_TwinS2Seen
Coop_EventScript_TwinS2Short::
	msgbox Coop_Text_TwinS2Short, MSGBOX_DEFAULT
Coop_EventScript_TwinS2Seen::
	setvar VAR_0x8004, TWIN_ARC_S2
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinS2Spar::
	setvar VAR_0x8004, TWIN_ARC_SPAR_2
	specialvar VAR_RESULT, CoopQuest_TwinHas
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_TwinS2Prize
	msgbox Coop_Text_TwinSparAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_TwinSparLater
	special CoopQuest_ForceSolo
	trainerbattle_no_intro TRAINER_TWIN_SPAR_2, Coop_Text_TwinSpar2Lose
	setvar VAR_0x8004, TWIN_ARC_SPAR_2
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinS2Prize::
	setvar VAR_0x8004, COOP_REWARD_TWIN_SPAR_2
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_TwinS2After
	checkitemspace ITEM_MUSCLE_BAND
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TwinBagFull
	msgbox Coop_Text_TwinSparPrize, MSGBOX_DEFAULT
	giveitem ITEM_MUSCLE_BAND
	setvar VAR_0x8004, COOP_REWARD_TWIN_SPAR_2
	special CoopQuest_ClaimReward
Coop_EventScript_TwinS2After::
	msgbox Coop_Text_TwinS2After, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_TwinS3::
	lock
	faceplayer
	setvar VAR_0x8004, TWIN_ARC_S3
	specialvar VAR_RESULT, CoopQuest_TwinState
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TwinS3After
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TwinS3Short
	msgbox Coop_Text_TwinS3, MSGBOX_DEFAULT
	goto Coop_EventScript_TwinS3Seen
Coop_EventScript_TwinS3Short::
	msgbox Coop_Text_TwinS3Short, MSGBOX_DEFAULT
Coop_EventScript_TwinS3Seen::
	setvar VAR_0x8004, TWIN_ARC_S3
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinS3After::
	msgbox Coop_Text_TwinS3After, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_TwinSparLater::
	msgbox Coop_Text_TwinSparLater, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_TwinBagFull::
	msgbox Coop_Text_TwinBagFull, MSGBOX_DEFAULT
	release
	end

@ ----------------------------------------------------------------- MIRA
Coop_EventScript_TwinM1::
	lock
	faceplayer
	setvar VAR_0x8004, TWIN_ARC_M1
	specialvar VAR_RESULT, CoopQuest_TwinState
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TwinM1After
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TwinM1Short
	msgbox Coop_Text_TwinM1, MSGBOX_DEFAULT
	goto Coop_EventScript_TwinM1Seen
Coop_EventScript_TwinM1Short::
	msgbox Coop_Text_TwinM1Short, MSGBOX_DEFAULT
Coop_EventScript_TwinM1Seen::
	setvar VAR_0x8004, TWIN_ARC_M1
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinM1After::
	msgbox Coop_Text_TwinM1After, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_TwinM2::
	lock
	faceplayer
	setvar VAR_0x8004, TWIN_ARC_M2
	specialvar VAR_RESULT, CoopQuest_TwinState
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TwinM2After
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TwinM2Short
	msgbox Coop_Text_TwinM2, MSGBOX_DEFAULT
	goto Coop_EventScript_TwinM2Seen
Coop_EventScript_TwinM2Short::
	msgbox Coop_Text_TwinM2Short, MSGBOX_DEFAULT
Coop_EventScript_TwinM2Seen::
	setvar VAR_0x8004, TWIN_ARC_M2
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinM2After::
	msgbox Coop_Text_TwinM2After, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_TwinM3::
	lock
	faceplayer
	setvar VAR_0x8004, TWIN_ARC_M3
	specialvar VAR_RESULT, CoopQuest_TwinState
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_TwinM3After
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_TwinM3Short
	msgbox Coop_Text_TwinM3, MSGBOX_DEFAULT
	goto Coop_EventScript_TwinM3Seen
Coop_EventScript_TwinM3Short::
	msgbox Coop_Text_TwinM3Short, MSGBOX_DEFAULT
Coop_EventScript_TwinM3Seen::
	setvar VAR_0x8004, TWIN_ARC_M3
	call Coop_EventScript_TwinMark
Coop_EventScript_TwinM3After::
	msgbox Coop_Text_TwinM3After, MSGBOX_DEFAULT
	release
	end

@ ----------------------------------------------------------- exhibition
Coop_EventScript_Exhibition::
	lock
	faceplayer
	goto_if_set FLAG_TWIN_EXH_WON, Coop_EventScript_ExhibitionWon
	msgbox Coop_Text_ExhibitionAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_ExhibitionLater
	msgbox Coop_Text_ExhibitionStart, MSGBOX_DEFAULT
	specialvar VAR_RESULT, CoopQuest_DuoReady
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_ExhibitionSolo
	special CoopDeal_BufferPartnerName
	msgbox Coop_Text_ExhibitionAskDuo, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_ExhibitionSolo
	setvar VAR_0x8004, TRAINER_TWIN_EXH_SOREN
	setvar VAR_0x8005, TRAINER_TWIN_EXH_MIRA
	setvar VAR_0x8006, 0
	setvar VAR_0x8007, FLAG_TWIN_EXH_WON
	special Coop_StartStoryBattle
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_ExhibitionDuoWon
Coop_EventScript_ExhibitionSolo::
	special CoopQuest_ForceSolo
	trainerbattle_two_trainers TRAINER_TWIN_EXH_SOREN, Coop_Text_ExhSorenLose, TRAINER_TWIN_EXH_MIRA, Coop_Text_ExhMiraLose
	setflag FLAG_TWIN_EXH_WON
@ Won here or (the partner hosting) on this save too: record both twins.
Coop_EventScript_ExhibitionDuoWon::
Coop_EventScript_ExhibitionWon::
	settrainerflag TRAINER_TWIN_EXH_SOREN
	settrainerflag TRAINER_TWIN_EXH_MIRA
	setvar VAR_0x8004, TWIN_ARC_EXHIBITION
	call Coop_EventScript_TwinMark
	setvar VAR_0x8004, COOP_REWARD_TWIN_SOREN
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_ExhibitionMiraGift
	checkitemspace ITEM_EXPERT_BELT
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TwinBagFull
	msgbox Coop_Text_ExhSorenGift, MSGBOX_DEFAULT
	giveitem ITEM_EXPERT_BELT
	setvar VAR_0x8004, COOP_REWARD_TWIN_SOREN
	special CoopQuest_ClaimReward
Coop_EventScript_ExhibitionMiraGift::
	setvar VAR_0x8004, COOP_REWARD_TWIN_MIRA
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_ExhibitionAfter
	checkitemspace ITEM_SOOTHE_BELL
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_TwinBagFull
	msgbox Coop_Text_ExhMiraGift, MSGBOX_DEFAULT
	giveitem ITEM_SOOTHE_BELL
	setvar VAR_0x8004, COOP_REWARD_TWIN_MIRA
	special CoopQuest_ClaimReward
Coop_EventScript_ExhibitionAfter::
	msgbox Coop_Text_ExhibitionAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_ExhibitionLater::
	msgbox Coop_Text_ExhibitionLater, MSGBOX_DEFAULT
	release
	end

@ ---------------------------------------------------------------- texts
Coop_Text_TwinS1:
	.string "SOREN: Oh, it's you. I came to\n"
	.string "test myself at the BATTLE TENT.\p"
	.string "Rental POKéMON, no favourites.\n"
	.string "Only skill counts in there.\p"
	.string "If I can win with strangers, I can\n"
	.string "win with anyone. That's the plan.$"

Coop_Text_TwinS1Short:
	.string "SOREN: Your partner already heard\n"
	.string "my plan. Short version: the TENT\l"
	.string "first, then everyone else.$"

Coop_Text_TwinSparAsk:
	.string "SOREN: Got time for a spar? One\n"
	.string "quick battle, my new team.$"

Coop_Text_TwinSpar1Lose:
	.string "Tch. You read my moves. Again,\n"
	.string "next time.$"

Coop_Text_TwinSpar2Lose:
	.string "…Still not enough. I'll fix that\n"
	.string "before MOSSDEEP.$"

Coop_Text_TwinSparPrize:
	.string "SOREN: You earned this. Don't\n"
	.string "waste it.$"

Coop_Text_TwinSparLater:
	.string "SOREN: Suit yourself. I'll be\n"
	.string "right here, training.$"

Coop_Text_TwinBagFull:
	.string "Your bag's full. Come back when\n"
	.string "you've made room.$"

Coop_Text_TwinS1After:
	.string "SOREN: Next stop for me: wherever\n"
	.string "the strong trainers are.$"

Coop_Text_TwinS2:
	.string "SOREN: Six badges, huh? I've been\n"
	.string "training on ROUTE 119 in the rain.\p"
	.string "MIRA says I push my POKéMON too\n"
	.string "hard. My CORVISQUIRE evolved, so\l"
	.string "maybe she's wrong. Maybe.$"

Coop_Text_TwinS2Short:
	.string "SOREN: Your partner told you? Yeah,\n"
	.string "I'm training in the rain here now.$"

Coop_Text_TwinS2After:
	.string "SOREN: Meet me in MOSSDEEP later.\n"
	.string "I've got an idea brewing.$"

Coop_Text_TwinS3:
	.string "SOREN: Listen. MIRA and I talked.\p"
	.string "We've each gone our own way, so\n"
	.string "let's show Dad what we learned.\p"
	.string "An exhibition, in LITTLEROOT.\n"
	.string "Both of us against you. Once MIRA\l"
	.string "is done with her research, come.$"

Coop_Text_TwinS3Short:
	.string "SOREN: You know the plan: the\n"
	.string "exhibition in LITTLEROOT.$"

Coop_Text_TwinS3After:
	.string "SOREN: LITTLEROOT, once MIRA's\n"
	.string "ready. Don't keep us waiting.$"

Coop_Text_TwinM1:
	.string "MIRA: Shh! Look at that GRIMER.\n"
	.string "It's the ALOLAN kind, out here!\p"
	.string "Something in this water draws\n"
	.string "them. MARLIN thinks it's dumping.\p"
	.string "I'd rather study habitats than\n"
	.string "win trophies. Odd ones especially.$"

Coop_Text_TwinM1Short:
	.string "MIRA: Your partner saw my notes\n"
	.string "on the GRIMER already. Something's\l"
	.string "fouling this beach!$"

Coop_Text_TwinM1After:
	.string "MIRA: Clean water, clean habitats.\n"
	.string "That's how it should be.$"

Coop_Text_TwinM2:
	.string "MIRA: The caravan's MUDSDALE is\n"
	.string "amazing! I raised a MUDBRAY too.\p"
	.string "ZAHRA's map has a riddle about the\n"
	.string "SANDBURIED RUINS' glyphs:\p"
	.string "sun, then star, then moon. Glyphs\n"
	.string "that light in order. Fascinating!$"

Coop_Text_TwinM2Short:
	.string "MIRA: You heard about the glyphs\n"
	.string "from your partner? Sun, star, moon!$"

Coop_Text_TwinM2After:
	.string "MIRA: Desert POKéMON keep cool by\n"
	.string "burrowing. I could watch all day.$"

Coop_Text_TwinM3:
	.string "MIRA: I came for the ALOLAN VULPIX\n"
	.string "colony. Mine just evolved!\p"
	.string "Dr. ISLA's notes say they need an\n"
	.string "ICE STONE, and the cave keeps them.\p"
	.string "SOREN wants an exhibition in\n"
	.string "LITTLEROOT. I'm ready. Are you?$"

Coop_Text_TwinM3Short:
	.string "MIRA: Your partner heard: my\n"
	.string "NINETALES and I are ready for\l"
	.string "LITTLEROOT!$"

Coop_Text_TwinM3After:
	.string "MIRA: See you in LITTLEROOT!$"

Coop_Text_ExhibitionAsk:
	.string "SOREN: You made it.\n"
	.string "MIRA: Both of us, at once!\p"
	.string "Ready for the exhibition?$"

Coop_Text_ExhibitionStart:
	.string "SOREN: Strength, against…\n"
	.string "MIRA: …understanding. Go!$"

Coop_Text_ExhibitionAskDuo:
	.string "SOREN: {STR_VAR_1} too? Two on two,\n"
	.string "then! Battle together?$"

Coop_Text_ExhSorenLose:
	.string "SOREN: Ha… That was a real fight.$"

Coop_Text_ExhMiraLose:
	.string "MIRA: What a team you are!$"

Coop_Text_ExhSorenGift:
	.string "SOREN: Take this. It suits a\n"
	.string "trainer who hits that hard.$"

Coop_Text_ExhMiraGift:
	.string "MIRA: And this, for your POKéMON's\n"
	.string "happiness. They earned it.$"

Coop_Text_ExhibitionAfter:
	.string "SOREN: Next time I'll win.\n"
	.string "MIRA: And I'll take notes!$"

Coop_Text_ExhibitionLater:
	.string "MIRA: Whenever you're ready. We'll\n"
	.string "wait right here.$"
