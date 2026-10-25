@ Shoal Cave: The Missing Researcher (src/coop_quest.c, docs/COOP_QUESTS.md).
@ Personal: VAR_SHOAL_QUEST, the Vulpix, the reward. Shared: the clues
@ (OR-merged live) and "rescued" (ledger milestone, catch-up flag).

ShoalCave_EventScript_SetIslaVisibility::
	setflag FLAG_HIDE_SHOAL_ISLA_ENTRANCE
	setflag FLAG_HIDE_SHOAL_VULPIX
	goto_if_lt VAR_SHOAL_QUEST, SHOAL_RESCUED, ShoalCave_EventScript_IslaStillLost
	setflag FLAG_HIDE_SHOAL_ISLA_ICE
	clearflag FLAG_HIDE_SHOAL_ISLA_ENTRANCE
	goto_if_set FLAG_SHOAL_VULPIX_DONE, Common_EventScript_NopReturn
	clearflag FLAG_HIDE_SHOAL_VULPIX
	return

ShoalCave_EventScript_IslaStillLost::
	clearflag FLAG_HIDE_SHOAL_ISLA_ICE
	return

ShoalCave_EventScript_SetBriefed::
	setvar VAR_SHOAL_QUEST, SHOAL_BRIEFED
	return

ShoalCave_EventScript_Piers::
	lock
	faceplayer
	goto_if_ge VAR_SHOAL_QUEST, SHOAL_RESCUED, ShoalCave_EventScript_PiersAfter
	msgbox ShoalCave_Text_PiersBrief, MSGBOX_DEFAULT
	call_if_lt VAR_SHOAL_QUEST, SHOAL_BRIEFED, ShoalCave_EventScript_SetBriefed
	goto_if_set FLAG_SHOAL_GOT_ROPE, ShoalCave_EventScript_PiersTide
	checkitemspace ITEM_ESCAPE_ROPE
	goto_if_eq VAR_RESULT, FALSE, ShoalCave_EventScript_PiersTide
	msgbox ShoalCave_Text_PiersRope, MSGBOX_DEFAULT
	giveitem ITEM_ESCAPE_ROPE
	setflag FLAG_SHOAL_GOT_ROPE
ShoalCave_EventScript_PiersTide::
	msgbox ShoalCave_Text_PiersTide, MSGBOX_DEFAULT
	release
	end

ShoalCave_EventScript_PiersAfter::
	msgbox ShoalCave_Text_PiersAfter, MSGBOX_DEFAULT
	release
	end

@ The clues: one OR-merged bit each, so a clue the partner found is
@ already logged here and its object is gone at the next map load.
ShoalCave_EventScript_ClueThermo::
	lock
	setvar VAR_0x8005, SHOAL_CLUE_THERMO
	call ShoalCave_EventScript_TakeClue
	msgbox ShoalCave_Text_ClueThermo, MSGBOX_DEFAULT
	goto ShoalCave_EventScript_ClueCount

ShoalCave_EventScript_ClueNotes::
	lock
	setvar VAR_0x8005, SHOAL_CLUE_NOTES
	call ShoalCave_EventScript_TakeClue
	msgbox ShoalCave_Text_ClueNotes, MSGBOX_DEFAULT
	goto ShoalCave_EventScript_ClueCount

ShoalCave_EventScript_ClueRadio::
	lock
	setvar VAR_0x8005, SHOAL_CLUE_RADIO
	call ShoalCave_EventScript_TakeClue
	msgbox ShoalCave_Text_ClueRadio, MSGBOX_DEFAULT
	goto ShoalCave_EventScript_ClueCount

ShoalCave_EventScript_TakeClue::
	setvar VAR_0x8004, COOP_QUEST_SHOAL_CLUES
	special CoopQuest_SetBits
	call_if_lt VAR_SHOAL_QUEST, SHOAL_BRIEFED, ShoalCave_EventScript_SetBriefed
	playse SE_SELECT
	removeobject VAR_LAST_TALKED
	return

ShoalCave_EventScript_ClueCount::
	setvar VAR_0x8004, COOP_QUEST_SHOAL_CLUES
	specialvar VAR_RESULT, CoopQuest_CountBits
	msgbox ShoalCave_Text_ClueCount, MSGBOX_DEFAULT
	release
	end

@ ISLA, stranded in the B1F ice room's TM pocket.
ShoalCave_EventScript_IslaIce::
	lock
	faceplayer
	setvar VAR_0x8004, COOP_STORY_SHOAL_RESCUED
	specialvar VAR_RESULT, CoopQuest_MilestoneKnown
	goto_if_eq VAR_RESULT, TRUE, ShoalCave_EventScript_IslaPartnerCalled
	setvar VAR_0x8004, COOP_QUEST_SHOAL_CLUES
	setvar VAR_0x8005, SHOAL_CLUE_RADIO
	specialvar VAR_RESULT, CoopQuest_HasBits
	goto_if_eq VAR_RESULT, TRUE, ShoalCave_EventScript_IslaRescue
	call_if_lt VAR_SHOAL_QUEST, SHOAL_FOUND, ShoalCave_EventScript_SetFound
	msgbox ShoalCave_Text_IslaNeedRadio, MSGBOX_DEFAULT
	release
	end

ShoalCave_EventScript_SetFound::
	setvar VAR_SHOAL_QUEST, SHOAL_FOUND
	return

ShoalCave_EventScript_IslaPartnerCalled::
	msgbox ShoalCave_Text_IslaPartnerCalled, MSGBOX_DEFAULT
	goto ShoalCave_EventScript_IslaClimbOut

ShoalCave_EventScript_IslaRescue::
	msgbox ShoalCave_Text_IslaRescue, MSGBOX_DEFAULT
ShoalCave_EventScript_IslaClimbOut::
	setvar VAR_SHOAL_QUEST, SHOAL_RESCUED
	setflag FLAG_HIDE_SHOAL_ISLA_ICE
	clearflag FLAG_HIDE_SHOAL_ISLA_ENTRANCE
	fadescreen FADE_TO_BLACK
	removeobject VAR_LAST_TALKED
	warpsilent MAP_SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM, 5, 27
	waitstate
	release
	end

@ After the rescue: her thanks once per save (deferred while the bag is
@ full), then one ICE STONE a day.
ShoalCave_EventScript_IslaEntrance::
	lock
	faceplayer
	setvar VAR_0x8004, COOP_REWARD_SHOAL_RESCUE
	specialvar VAR_RESULT, CoopQuest_RewardClaimed
	goto_if_eq VAR_RESULT, FALSE, ShoalCave_EventScript_IslaThanks
	goto_if_set FLAG_DAILY_SHOAL_ICE_STONE, ShoalCave_EventScript_IslaDailyDone
	msgbox ShoalCave_Text_IslaDaily, MSGBOX_DEFAULT
	giveitem ITEM_ICE_STONE
	goto_if_eq VAR_RESULT, FALSE, ShoalCave_EventScript_IslaBagFull
	setflag FLAG_DAILY_SHOAL_ICE_STONE
	release
	end

ShoalCave_EventScript_IslaDailyDone::
	msgbox ShoalCave_Text_IslaDailyDone, MSGBOX_DEFAULT
	release
	end

ShoalCave_EventScript_IslaThanks::
	goto_if_set FLAG_SHOAL_CATCHUP, ShoalCave_EventScript_IslaThanksCatchup
	msgbox ShoalCave_Text_IslaThanks, MSGBOX_DEFAULT
	goto ShoalCave_EventScript_IslaGift

ShoalCave_EventScript_IslaThanksCatchup::
	msgbox ShoalCave_Text_IslaThanksCatchup, MSGBOX_DEFAULT
ShoalCave_EventScript_IslaGift::
	checkitemspace ITEM_ICE_STONE, 2
	goto_if_eq VAR_RESULT, FALSE, ShoalCave_EventScript_IslaBagFull
	checkitemspace ITEM_SNOWBALL, 2
	goto_if_eq VAR_RESULT, FALSE, ShoalCave_EventScript_IslaBagFull
	giveitem ITEM_ICE_STONE, 2
	giveitem ITEM_SNOWBALL, 2
	setvar VAR_0x8004, COOP_REWARD_SHOAL_RESCUE
	special CoopQuest_ClaimReward
	msgbox ShoalCave_Text_IslaAfterGift, MSGBOX_DEFAULT
	release
	end

ShoalCave_EventScript_IslaBagFull::
	msgbox ShoalCave_Text_IslaBagFull, MSGBOX_DEFAULT
	release
	end

@ This save's ALOLAN VULPIX, by the ice room ladder after the rescue.
ShoalCave_EventScript_Vulpix::
	lock
	faceplayer
	setwildbattle SPECIES_VULPIX_ALOLA, 32
	waitse
	playmoncry SPECIES_VULPIX_ALOLA, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, ShoalCave_EventScript_VulpixDone
	msgbox ShoalCave_Text_VulpixFled, MSGBOX_DEFAULT
	release
	end

ShoalCave_EventScript_VulpixDone::
	setflag FLAG_SHOAL_VULPIX_DONE
	release
	end

ShoalCave_EventScript_TryRemoveVulpix::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject VAR_LAST_TALKED
	return

ShoalCave_Text_PiersBrief:
	.string "PIERS: Are you a TRAINER? Please,\n"
	.string "I need help!\p"
	.string "Dr. ISLA went in at low tide to\n"
	.string "study the cave's ice POKéMON.\p"
	.string "Her radio went silent hours ago.\p"
	.string "She was heading for the ice room\n"
	.string "down on B1F. If you find her gear,\l"
	.string "her notes may show the way.$"

ShoalCave_Text_PiersRope:
	.string "Take this, just in case.$"

ShoalCave_Text_PiersTide:
	.string "Mind the tide. At high tide the\n"
	.string "inner pools flood.\p"
	.string "If the water cuts you off, wait\n"
	.string "for low tide or use an ESCAPE ROPE.$"

ShoalCave_Text_PiersAfter:
	.string "PIERS: Dr. ISLA is back at work,\n"
	.string "thanks to you. Her ankle's mending.$"

ShoalCave_Text_ClueThermo:
	.string "A frosted THERMOMETER, stamped\n"
	.string "“ISLA”. It still reads twelve below.\p"
	.string "She came through here.$"

ShoalCave_Text_ClueNotes:
	.string "Soggy field notes:\p"
	.string "“Pale Vulpix tracks lead to B1F.\n"
	.string "Following them to the ice room.\p"
	.string "The TM ledge sits in a pocket.\n"
	.string "Slide from the top to reach it.”$"

ShoalCave_Text_ClueRadio:
	.string "A dented RADIO, still crackling.\n"
	.string "It must be Dr. ISLA's.$"

ShoalCave_Text_ClueCount:
	.string "Dr. ISLA's gear found: {STR_VAR_1} of 3.$"

ShoalCave_Text_IslaNeedRadio:
	.string "ISLA: Oh! Someone came!\p"
	.string "I slid down here after an ALOLAN\n"
	.string "VULPIX and twisted my ankle.\p"
	.string "My RADIO fell as I slipped, near\n"
	.string "the ladder upstairs.\p"
	.string "Could you bring it? PIERS can\n"
	.string "lower a rope if I can call him.$"

ShoalCave_Text_IslaRescue:
	.string "ISLA: My RADIO! Thank you!\p"
	.string "…PIERS? I'm in the ice room's TM\n"
	.string "pocket. Drop the rope!\p"
	.string "Come on, let's climb out\n"
	.string "together.$"

ShoalCave_Text_IslaPartnerCalled:
	.string "ISLA: Your partner already got\n"
	.string "word to PIERS. The rope's here!\p"
	.string "Let's climb out together.$"

ShoalCave_Text_IslaThanks:
	.string "ISLA: You found me. Thank you!\p"
	.string "My samples gave me ICE STONES and\n"
	.string "SNOWBALLS. Please, take some.$"

ShoalCave_Text_IslaThanksCatchup:
	.string "ISLA: Your partner pulled me out,\n"
	.string "but you followed my trail too.\p"
	.string "Please, take a share of my\n"
	.string "samples.$"

ShoalCave_Text_IslaAfterGift:
	.string "An ICE STONE evolves ALOLAN\n"
	.string "VULPIX. Come back any day for\l"
	.string "another one from my samples.$"

ShoalCave_Text_IslaBagFull:
	.string "ISLA: Your bag is full. Come back\n"
	.string "when you have room, I'll be here.$"

ShoalCave_Text_IslaDaily:
	.string "ISLA: Today's ICE STONE, fresh\n"
	.string "from the ice room.$"

ShoalCave_Text_IslaDailyDone:
	.string "ISLA: That's all for today. The\n"
	.string "ice needs a day to make another.$"

ShoalCave_Text_VulpixFled:
	.string "The ALOLAN VULPIX scampered back\n"
	.string "into the ice.$"
