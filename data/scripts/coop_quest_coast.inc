@ Coastal Cleanup, Route 109 (src/coop_quest.c, docs/COOP_QUESTS.md).
@ Personal: VAR_COAST_QUEST, the crew and SLAG fights, the Grimer, the
@ reward. Shared: the clues (OR-merged live) and "clean" (ledger milestone:
@ crew, drums and Grimer gone, fishers back, better fishing; catch-up flag).

Route109_EventScript_SetCoastObjects::
	setflag FLAG_HIDE_COAST_CLEAN_NPCS
	clearflag FLAG_HIDE_COAST_POLLUTION
	clearflag FLAG_HIDE_COAST_GRUNTS
	goto_if_ge VAR_COAST_QUEST, COAST_CLEAN, Route109_EventScript_CoastIsClean
	call_if_set FLAG_COAST_CREW_DUO, Route109_EventScript_CrewDuoFlags
	call_if_defeated TRAINER_CC_GRUNT_SKIFF, Route109_EventScript_MaybeHideCrew
	call_if_set FLAG_COAST_GRIMER_1_DONE, Route109_EventScript_HideGrimer1
	call_if_set FLAG_COAST_GRIMER_2_DONE, Route109_EventScript_HideGrimer2
	call_if_unset FLAG_COAST_GRIMER_1_DONE, Route109_EventScript_ShowGrimer1
	call_if_unset FLAG_COAST_GRIMER_2_DONE, Route109_EventScript_ShowGrimer2
	return

Route109_EventScript_MaybeHideCrew::
	goto_if_not_defeated TRAINER_CC_GRUNT_BARREL, Common_EventScript_NopReturn
	setflag FLAG_HIDE_COAST_GRUNTS
	call_if_lt VAR_COAST_QUEST, COAST_CREW_BEATEN, Route109_EventScript_SetCrewBeaten
	return

Route109_EventScript_HideGrimer1::
	setflag FLAG_HIDE_COAST_GRIMER_1
	return
Route109_EventScript_HideGrimer2::
	setflag FLAG_HIDE_COAST_GRIMER_2
	return
Route109_EventScript_ShowGrimer1::
	clearflag FLAG_HIDE_COAST_GRIMER_1
	return
Route109_EventScript_ShowGrimer2::
	clearflag FLAG_HIDE_COAST_GRIMER_2
	return

Route109_EventScript_CoastIsClean::
	setflag FLAG_HIDE_COAST_POLLUTION
	setflag FLAG_HIDE_COAST_GRUNTS
	setflag FLAG_HIDE_COAST_GRIMER_1
	setflag FLAG_HIDE_COAST_GRIMER_2
	clearflag FLAG_HIDE_COAST_CLEAN_NPCS
	return

Route109_EventScript_CoastBriefed::
	setvar VAR_COAST_QUEST, COAST_BRIEFED
	return

@ MARLIN, the fisher who noticed.
Route109_EventScript_Marlin::
	lock
	faceplayer
	goto_if_ge VAR_COAST_QUEST, COAST_CLEAN, Route109_EventScript_MarlinClean
	msgbox Route109_Text_MarlinBrief, MSGBOX_DEFAULT
	call_if_lt VAR_COAST_QUEST, COAST_BRIEFED, Route109_EventScript_CoastBriefed
	release
	end

Route109_EventScript_MarlinClean::
	setvar VAR_0x8004, COOP_REWARD_COAST
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, TRUE, Route109_EventScript_MarlinAfter
	goto_if_set FLAG_COAST_CATCHUP, Route109_EventScript_MarlinThanksCatchup
	msgbox Route109_Text_MarlinThanks, MSGBOX_DEFAULT
	goto Route109_EventScript_MarlinGift

Route109_EventScript_MarlinThanksCatchup::
	msgbox Route109_Text_MarlinThanksCatchup, MSGBOX_DEFAULT
Route109_EventScript_MarlinGift::
	checkitemspace ITEM_LURE_BALL, 5
	goto_if_eq VAR_RESULT, FALSE, Route109_EventScript_MarlinBagFull
	checkitemspace ITEM_MYSTIC_WATER
	goto_if_eq VAR_RESULT, FALSE, Route109_EventScript_MarlinBagFull
	giveitem ITEM_LURE_BALL, 5
	giveitem ITEM_MYSTIC_WATER
	setvar VAR_0x8004, COOP_REWARD_COAST
	special CoopQuest_ClaimReward
Route109_EventScript_MarlinAfter::
	msgbox Route109_Text_MarlinAfter, MSGBOX_DEFAULT
	release
	end

Route109_EventScript_MarlinBagFull::
	msgbox Route109_Text_MarlinBagFull, MSGBOX_DEFAULT
	release
	end

Route109_EventScript_CleanFisher::
	msgbox Route109_Text_CleanFisher, MSGBOX_NPC
	end

@ The three clues (OR-merged).
Route109_EventScript_ClueSheen::
	lockall
	setvar VAR_0x8005, COAST_CLUE_SHEEN
	goto_if_ge VAR_COAST_QUEST, COAST_CLEAN, Route109_EventScript_CleanWater
	msgbox Route109_Text_ClueSheen, MSGBOX_DEFAULT
	goto Route109_EventScript_CoastClue

Route109_EventScript_ClueDrum::
	lockall
	setvar VAR_0x8005, COAST_CLUE_DRUM
	msgbox Route109_Text_ClueDrum, MSGBOX_DEFAULT
	goto Route109_EventScript_CoastClue

Route109_EventScript_ClueTracks::
	lockall
	setvar VAR_0x8005, COAST_CLUE_TRACKS
	goto_if_ge VAR_COAST_QUEST, COAST_CLEAN, Route109_EventScript_CleanSand
	msgbox Route109_Text_ClueTracks, MSGBOX_DEFAULT
	goto Route109_EventScript_CoastClue

Route109_EventScript_CoastClue::
	setvar VAR_0x8004, COOP_QUEST_COAST_CLUES
	special CoopQuest_SetBits
	call_if_lt VAR_COAST_QUEST, COAST_BRIEFED, Route109_EventScript_CoastBriefed
	specialvar VAR_RESULT, CoopQuest_CountBits
	goto_if_ge VAR_RESULT, 3, Route109_EventScript_CoastCluesDone
	msgbox Route109_Text_ClueCount, MSGBOX_DEFAULT
	releaseall
	end

Route109_EventScript_CoastCluesDone::
	call_if_lt VAR_COAST_QUEST, COAST_CLUES, Route109_EventScript_SetCoastClues
	msgbox Route109_Text_CluesDone, MSGBOX_DEFAULT
	releaseall
	end

Route109_EventScript_SetCoastClues::
	setvar VAR_COAST_QUEST, COAST_CLUES
	return

Route109_EventScript_CleanWater::
	msgbox Route109_Text_CleanWater, MSGBOX_DEFAULT
	releaseall
	end

Route109_EventScript_CleanSand::
	msgbox Route109_Text_CleanSand, MSGBOX_DEFAULT
	releaseall
	end

@ The crew: a duo with the partner (Beacon on, here), otherwise one by
@ one. Both saves record a duo win (flagged trainers).
Route109_EventScript_CrewNotYet::
	msgbox Route109_Text_CrewShoo, MSGBOX_DEFAULT
	release
	end

Route109_EventScript_GruntSkiff::
	lock
	faceplayer
	call Route109_EventScript_CheckCluesFromPartner
	goto_if_lt VAR_COAST_QUEST, COAST_CLUES, Route109_EventScript_CrewNotYet
	goto_if_defeated TRAINER_CC_GRUNT_SKIFF, Route109_EventScript_CrewAfter
	specialvar VAR_RESULT, CoopQuest_DuoReady
	goto_if_eq VAR_RESULT, FALSE, Route109_EventScript_SkiffSolo
	goto_if_defeated TRAINER_CC_GRUNT_BARREL, Route109_EventScript_SkiffSolo
	special CoopDeal_BufferPartnerName
	msgbox Route109_Text_CrewAskDuo, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Route109_EventScript_CrewDuo
Route109_EventScript_SkiffSolo::
	special CoopQuest_ForceSolo
	trainerbattle_single TRAINER_CC_GRUNT_SKIFF, Route109_Text_SkiffIntro, Route109_Text_SkiffDefeat, Route109_EventScript_CrewBeaten
	goto Route109_EventScript_CrewAfter

@ The duo: a co-op multi battle that marks both saves (FLAG_COAST_CREW_DUO)
@ and resumes here. If the partner cannot join, the fight is solo.
Route109_EventScript_CrewDuo::
	setvar VAR_0x8004, TRAINER_CC_GRUNT_SKIFF
	setvar VAR_0x8005, TRAINER_CC_GRUNT_BARREL
	setvar VAR_0x8006, 0
	setvar VAR_0x8007, FLAG_COAST_CREW_DUO
	special Coop_StartStoryBattle
	goto_if_eq VAR_RESULT, FALSE, Route109_EventScript_SkiffSolo
	call Route109_EventScript_CrewDuoFlags
	goto Route109_EventScript_CrewBeaten

Route109_EventScript_CrewDuoFlags::
	settrainerflag TRAINER_CC_GRUNT_SKIFF
	settrainerflag TRAINER_CC_GRUNT_BARREL
	return

@ The partner's save after a duo it joined: both grunts beaten, gone.
Route109_EventScript_ApplyCrewDuo::
	goto_if_unset FLAG_COAST_CREW_DUO, Common_EventScript_NopReturn
	goto_if_set FLAG_HIDE_COAST_GRUNTS, Common_EventScript_NopReturn
	call Route109_EventScript_CrewDuoFlags
	call_if_lt VAR_COAST_QUEST, COAST_CREW_BEATEN, Route109_EventScript_SetCrewBeaten
	setflag FLAG_HIDE_COAST_GRUNTS
	removeobject LOCALID_ROUTE109_SKIFF
	removeobject LOCALID_ROUTE109_BARREL
	return

Route109_EventScript_GruntBarrel::
	lock
	faceplayer
	call Route109_EventScript_CheckCluesFromPartner
	goto_if_lt VAR_COAST_QUEST, COAST_CLUES, Route109_EventScript_CrewNotYet
	goto_if_defeated TRAINER_CC_GRUNT_BARREL, Route109_EventScript_CrewAfter
	special CoopQuest_ForceSolo
	trainerbattle_single TRAINER_CC_GRUNT_BARREL, Route109_Text_BarrelIntro, Route109_Text_BarrelDefeat, Route109_EventScript_CrewBeaten
	goto Route109_EventScript_CrewAfter

@ Clues the partner found count: three known means the crew will fight.
Route109_EventScript_CheckCluesFromPartner::
	goto_if_ge VAR_COAST_QUEST, COAST_CLUES, Common_EventScript_NopReturn
	setvar VAR_0x8004, COOP_QUEST_COAST_CLUES
	specialvar VAR_RESULT, CoopQuest_CountBits
	call_if_ge VAR_RESULT, 3, Route109_EventScript_SetCoastClues
	return

Route109_EventScript_CrewBeaten::
	goto_if_not_defeated TRAINER_CC_GRUNT_SKIFF, Route109_EventScript_CrewOneDown
	goto_if_not_defeated TRAINER_CC_GRUNT_BARREL, Route109_EventScript_CrewOneDown
	msgbox Route109_Text_CrewFlee, MSGBOX_DEFAULT
	closemessage
	call_if_lt VAR_COAST_QUEST, COAST_CREW_BEATEN, Route109_EventScript_SetCrewBeaten
	setflag FLAG_HIDE_COAST_GRUNTS
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_ROUTE109_SKIFF
	removeobject LOCALID_ROUTE109_BARREL
	fadescreen FADE_FROM_BLACK
	release
	end

Route109_EventScript_SetCrewBeaten::
	setvar VAR_COAST_QUEST, COAST_CREW_BEATEN
	return

Route109_EventScript_CrewOneDown::
	msgbox Route109_Text_CrewOneDown, MSGBOX_DEFAULT
	release
	end

Route109_EventScript_CrewAfter::
	msgbox Route109_Text_CrewAfter, MSGBOX_DEFAULT
	release
	end

@ SLAG, the chief. A personal fight (each save beats him once); the
@ first win anywhere cleans the coast for both saves.
Route109_EventScript_Slag::
	lock
	faceplayer
	setvar VAR_0x8004, COOP_STORY_COAST_CLEAN
	specialvar VAR_RESULT, CoopQuest_MilestoneKnown
	goto_if_eq VAR_RESULT, TRUE, Route109_EventScript_SlagLeaving
	msgbox Route109_Text_SlagIntro, MSGBOX_DEFAULT
	special CoopQuest_ForceSolo
	trainerbattle_no_intro TRAINER_CC_CHIEF, Route109_Text_SlagDefeat
	msgbox Route109_Text_SlagGivesUp, MSGBOX_DEFAULT
	goto Route109_EventScript_CleanUp

Route109_EventScript_SlagLeaving::
	msgbox Route109_Text_SlagLeaving, MSGBOX_DEFAULT
Route109_EventScript_CleanUp::
	closemessage
	setvar VAR_COAST_QUEST, COAST_CLEAN
	fadescreen FADE_TO_BLACK
	call Route109_EventScript_CoastIsClean
	warpsilent MAP_ROUTE109, 9, 6
	waitstate
	release
	end

Route109_EventScript_DumpDrums::
	msgbox Route109_Text_DumpDrums, MSGBOX_SIGN
	end

@ This save's two ALOLAN GRIMER, drawn to the sludge.
Route109_EventScript_Grimer1::
	lock
	faceplayer
	setvar VAR_0x8008, 1
	goto Route109_EventScript_Grimer

Route109_EventScript_Grimer2::
	lock
	faceplayer
	setvar VAR_0x8008, 2
Route109_EventScript_Grimer::
	setwildbattle SPECIES_GRIMER_ALOLA, 18
	waitse
	playmoncry SPECIES_GRIMER_ALOLA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	call_if_eq VAR_0x8008, 1, Route109_EventScript_Grimer1Done
	call_if_eq VAR_0x8008, 2, Route109_EventScript_Grimer2Done
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, Route109_EventScript_GrimerEnd
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, Route109_EventScript_GrimerGone
	msgbox Route109_Text_GrimerStays, MSGBOX_DEFAULT
	release
	end

Route109_EventScript_Grimer1Done::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, Common_EventScript_NopReturn
	goto_if_eq VAR_RESULT, B_OUTCOME_LOST, Common_EventScript_NopReturn
	setflag FLAG_COAST_GRIMER_1_DONE
	return

Route109_EventScript_Grimer2Done::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, Common_EventScript_NopReturn
	goto_if_eq VAR_RESULT, B_OUTCOME_LOST, Common_EventScript_NopReturn
	setflag FLAG_COAST_GRIMER_2_DONE
	return

Route109_EventScript_GrimerGone::
	removeobject VAR_LAST_TALKED
Route109_EventScript_GrimerEnd::
	release
	end

Route109_EventScript_TryRemoveGrimer::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject VAR_LAST_TALKED
	return

Route109_Text_MarlinBrief:
	.string "MARLIN: The fish are gone, and the\n"
	.string "water by this beach is slick.\p"
	.string "Even some odd GRIMER are crawling\n"
	.string "out of the sludge!\p"
	.string "Somebody's dumping something.\n"
	.string "Check the water's edge, the sand,\l"
	.string "and the end by the SEASHORE HOUSE.$"

Route109_Text_MarlinThanks:
	.string "MARLIN: You ran them off! The\n"
	.string "fish are already coming back.\p"
	.string "Here, these suit a fisher's\n"
	.string "friend.$"

Route109_Text_MarlinThanksCatchup:
	.string "MARLIN: Your partner chased that\n"
	.string "crew away. You helped them find\l"
	.string "the trail, so this is yours too.$"

Route109_Text_MarlinAfter:
	.string "MARLIN: With the water clean, SKRELP\n"
	.string "bite here and off DEWFORD again.$"

Route109_Text_MarlinBagFull:
	.string "MARLIN: Your bag's stuffed. Come\n"
	.string "back with some room.$"

Route109_Text_CleanFisher:
	.string "The water's clear again. First\n"
	.string "good catch in weeks!$"

Route109_Text_ClueSheen:
	.string "A rainbow sheen floats on the\n"
	.string "water. It smells of hot tar.$"

Route109_Text_ClueDrum:
	.string "A washed-up drum, leaking black\n"
	.string "sludge. A faded “M” is on its lid.$"

Route109_Text_ClueTracks:
	.string "Boot prints and drag marks lead\n"
	.string "north, behind the SEASHORE HOUSE.$"

Route109_Text_ClueCount:
	.string "Clues found: {STR_VAR_1} of 3.$"

Route109_Text_CluesDone:
	.string "All three clues point behind the\n"
	.string "SEASHORE HOUSE. TEAM MAGMA?$"

Route109_Text_CleanWater:
	.string "The water is clear. Small fish\n"
	.string "dart over the sand.$"

Route109_Text_CleanSand:
	.string "Just footprints of beachgoers now.$"

Route109_Text_CrewShoo:
	.string "GRUNT: Beat it, kid. We're just…\n"
	.string "cleaning the beach. Yeah.$"

Route109_Text_CrewAskDuo:
	.string "GRUNT: Two of you snooping? Fine,\n"
	.string "two of us!\p"
	.string "Battle together with {STR_VAR_1}?$"

Route109_Text_SkiffIntro:
	.string "Our boss said nobody comes past\n"
	.string "the SEASHORE HOUSE!$"

Route109_Text_SkiffDefeat:
	.string "The slag barrels were heavy\n"
	.string "anyway…$"

Route109_Text_BarrelIntro:
	.string "These drums won't empty\n"
	.string "themselves!$"

Route109_Text_BarrelDefeat:
	.string "Ugh, my clothes stink of tar.$"

Route109_Text_CrewOneDown:
	.string "GRUNT: My partner's still standing!\n"
	.string "You're not getting past.$"

Route109_Text_CrewFlee:
	.string "GRUNTS: Boss SLAG can deal with\n"
	.string "you! We're out!$"

Route109_Text_CrewAfter:
	.string "GRUNT: Go bother the boss.$"

Route109_Text_SlagIntro:
	.string "SLAG: Volcanic slag from our\n"
	.string "digs has to go somewhere.\p"
	.string "The sea's big. Who's it hurting?\n"
	.string "…You're about to find out!$"

Route109_Text_SlagDefeat:
	.string "Bah! Fine, fine!$"

Route109_Text_SlagGivesUp:
	.string "SLAG: We'll haul the drums back to\n"
	.string "the base. Don't follow us!$"

Route109_Text_SlagLeaving:
	.string "SLAG: Your friend already beat me.\n"
	.string "We're packing up, see?$"

Route109_Text_DumpDrums:
	.string "Drums of volcanic slag, stamped\n"
	.string "with TEAM MAGMA's mark.$"

Route109_Text_GrimerStays:
	.string "The ALOLAN GRIMER sloshes back\n"
	.string "into the sludge.$"
