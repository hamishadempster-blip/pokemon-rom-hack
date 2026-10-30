@ The Bond Mentor (Rustboro Pokémon Center): Trainer Bond, src/coop_bond.c,
@ docs/COOP_BOND.md. Talking to her checks the Stone Badge on this save each
@ time, so a save that already has it (any later point in the story) can use
@ her at once; the first conversation is told once and can be heard again.
Coop_EventScript_BondMentor::
	lock
	faceplayer
	goto_if_unset FLAG_BADGE01_GET, Coop_EventScript_BondMentorNoBadge
	specialvar VAR_RESULT, CoopBond_ScriptMetMentor
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_BondMentorMenu
	msgbox Coop_Text_BondMentorIntro, MSGBOX_DEFAULT
Coop_EventScript_BondMentorMenu::
	message Coop_Text_BondMentorAsk
	waitmessage
	dynmultichoice 15, 1, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, Coop_Text_BondOptChoose, Coop_Text_BondOptCheck, Coop_Text_BondOptHow, Coop_Text_BondOptBack
	switch VAR_RESULT
	case 0, Coop_EventScript_BondChoose
	case 1, Coop_EventScript_BondCheck
	case 2, Coop_EventScript_BondHow
	goto Coop_EventScript_BondMentorBye
	end

Coop_EventScript_BondMentorNoBadge::
	msgbox Coop_Text_BondMentorNoBadge, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_BondMentorBye::
	msgbox Coop_Text_BondMentorBye, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_BondChoose::
	specialvar VAR_RESULT, CoopBond_ScriptBusy
	goto_if_eq VAR_RESULT, TRUE, Coop_EventScript_BondBusy
	msgbox Coop_Text_BondChooseWhich, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, Coop_EventScript_BondMentorMenu
	specialvar VAR_RESULT, CoopBond_ScriptCheckChoice
	goto_if_eq VAR_RESULT, 1, Coop_EventScript_BondNotEgg
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_BondAlready
	goto_if_eq VAR_RESULT, 3, Coop_EventScript_BondTwin
	msgbox gStringVar4, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_BondMentorMenu
	specialvar VAR_RESULT, CoopBond_ScriptCurrent
	goto_if_eq VAR_RESULT, 0, Coop_EventScript_BondDoChoose
	msgbox Coop_Text_BondOldKeeps, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_BondMentorMenu
Coop_EventScript_BondDoChoose::
	specialvar VAR_RESULT, CoopBond_ScriptDesignate
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_BondBusy
	playfanfare MUS_LEVEL_UP
	specialvar VAR_RESULT, CoopBond_ScriptCheckChoiceName
	msgbox Coop_Text_BondChosen, MSGBOX_DEFAULT
	waitfanfare
	goto Coop_EventScript_BondMentorMenu

Coop_EventScript_BondNotEgg::
	msgbox Coop_Text_BondNotEgg, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu

Coop_EventScript_BondAlready::
	msgbox Coop_Text_BondAlready, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu

Coop_EventScript_BondTwin::
	msgbox Coop_Text_BondTwin, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu

Coop_EventScript_BondBusy::
	msgbox Coop_Text_BondBusy, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu

Coop_EventScript_BondCheck::
	specialvar VAR_RESULT, CoopBond_ScriptCheckBond
	goto_if_eq VAR_RESULT, 0, Coop_EventScript_BondCheckNone
	goto_if_eq VAR_RESULT, 2, Coop_EventScript_BondCheckAway
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu
Coop_EventScript_BondCheckNone::
	msgbox Coop_Text_BondNone, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu
Coop_EventScript_BondCheckAway::
	msgbox Coop_Text_BondAway, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu

Coop_EventScript_BondHow::
	msgbox Coop_Text_BondHow, MSGBOX_DEFAULT
	goto Coop_EventScript_BondMentorMenu

Coop_Text_BondOptChoose:
	.string "CHOOSE/CHANGE PARTNER$"
Coop_Text_BondOptCheck:
	.string "CHECK BOND$"
Coop_Text_BondOptHow:
	.string "HOW IT WORKS$"
Coop_Text_BondOptBack:
	.string "BACK$"

Coop_Text_BondMentorNoBadge:
	.string "I teach TRAINERS to build a bond with\n"
	.string "one POKéMON, a Partner POKéMON.\p"
	.string "Win RUSTBORO's STONE BADGE first,\n"
	.string "then come and see me.$"

Coop_Text_BondMentorIntro:
	.string "That STONE BADGE says you're ready.\n"
	.string "I'm the BOND MENTOR.\p"
	.string "Choose one POKéMON as your Partner\n"
	.string "POKéMON. Battles and travels together\l"
	.string "earn it BOND XP.\p"
	.string "As its rank grows it gains a few\n"
	.string "points on every stat. You can change\l"
	.string "partners whenever you visit me.$"

Coop_Text_BondMentorAsk:
	.string "What can I do for you?$"

Coop_Text_BondMentorBye:
	.string "Look after each other.$"

Coop_Text_BondChooseWhich:
	.string "Which POKéMON will be your partner?$"

Coop_Text_BondOldKeeps:
	.string "{STR_VAR_1} keeps its BOND XP, but its\n"
	.string "bonus pauses while another POKéMON\l"
	.string "is your partner. Choosing it again\l"
	.string "brings everything back. Go ahead?$"

Coop_Text_BondChosen:
	.string "{STR_VAR_1} is your Partner POKéMON now!$"

Coop_Text_BondNotEgg:
	.string "An EGG can't be a partner. Choose a\n"
	.string "POKéMON that has hatched.$"

Coop_Text_BondAlready:
	.string "{STR_VAR_1} is already your partner.$"

Coop_Text_BondTwin:
	.string "Another of your POKéMON can't be told\n"
	.string "apart from {STR_VAR_1}, so I can't\l"
	.string "choose it. Choose another.$"

Coop_Text_BondBusy:
	.string "Finish what you're doing with your\n"
	.string "partner first, then come back.$"

Coop_Text_BondNone:
	.string "You haven't chosen a Partner POKéMON\n"
	.string "yet.$"

Coop_Text_BondAway:
	.string "Your partner {STR_VAR_1} isn't with you.\n"
	.string "Its BOND XP is kept; bring it along\l"
	.string "to use its bonus and earn more.$"

Coop_Text_BondHow:
	.string "Your partner earns BOND XP for helping\n"
	.string "knock out foes, damage dealt, wins,\l"
	.string "catches it helped with, and walking\l"
	.string "at the front.\p"
	.string "Support counts too: healing an ally,\n"
	.string "screens such as REFLECT, TAILWIND\l"
	.string "or TRICK ROOM.\p"
	.string "Each badge, ELITE FOUR win and the\n"
	.string "title gives a big bonus once.\p"
	.string "Ranks: COMPANION at 2,500 XP gives\n"
	.string "+5 to each stat; TRUSTED PARTNER at\l"
	.string "10,000, +10; ACE PARTNER at 25,000,\l"
	.string "+15. They don't add up.\p"
	.string "The points are on top of training:\n"
	.string "EVs, vitamins and items work as usual.\p"
	.string "No bonus or XP in the BATTLE FRONTIER,\n"
	.string "BATTLE TENTS, practice or link battles\l"
	.string "against friends. Nothing is lost there.\p"
	.string "A former partner keeps its BOND XP.\n"
	.string "Its summary shows it with SELECT.$"
