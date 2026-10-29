@ Battle Guide v2: the second boss of each gym, Elite Four room and the
@ Champions' room stands beside the leader (maps' object events). Talking
@ to either starts the same shared encounter: the partner says a line and
@ hands over to the leader's own script (gate, invite, rewards). After the
@ win the partner has its own words; rematches start from the leader.

Coop_EventScript_PairHeath::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_RUSTBORO_GYM, Coop_EventScript_PairHeathAfter
	msgbox Coop_Text_PairHeathIntro, MSGBOX_DEFAULT
	release
	goto RustboroCity_Gym_EventScript_Roxanne
	end

Coop_EventScript_PairHeathAfter::
	msgbox Coop_Text_PairHeathAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairReed::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_DEWFORD_GYM, Coop_EventScript_PairReedAfter
	msgbox Coop_Text_PairReedIntro, MSGBOX_DEFAULT
	release
	goto DewfordTown_Gym_EventScript_Brawly
	end

Coop_EventScript_PairReedAfter::
	msgbox Coop_Text_PairReedAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairJoule::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_MAUVILLE_GYM, Coop_EventScript_PairJouleAfter
	msgbox Coop_Text_PairJouleIntro, MSGBOX_DEFAULT
	release
	goto MauvilleCity_Gym_EventScript_Wattson
	end

Coop_EventScript_PairJouleAfter::
	msgbox Coop_Text_PairJouleAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairCole::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_LAVARIDGE_GYM, Coop_EventScript_PairColeAfter
	msgbox Coop_Text_PairColeIntro, MSGBOX_DEFAULT
	release
	goto LavaridgeTown_Gym_1F_EventScript_Flannery
	end

Coop_EventScript_PairColeAfter::
	msgbox Coop_Text_PairColeAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairHana::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_PETALBURG_GYM, Coop_EventScript_PairHanaAfter
	msgbox Coop_Text_PairHanaIntro, MSGBOX_DEFAULT
	release
	goto PetalburgCity_Gym_EventScript_Norman
	end

Coop_EventScript_PairHanaAfter::
	msgbox Coop_Text_PairHanaAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairGale::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_FORTREE_GYM, Coop_EventScript_PairGaleAfter
	msgbox Coop_Text_PairGaleIntro, MSGBOX_DEFAULT
	release
	goto FortreeCity_Gym_EventScript_Winona
	end

Coop_EventScript_PairGaleAfter::
	msgbox Coop_Text_PairGaleAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairWallaceGym::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_SOOTOPOLIS_GYM, Coop_EventScript_PairWallaceGymAfter
	msgbox Coop_Text_PairWallaceGymIntro, MSGBOX_DEFAULT
	release
	goto SootopolisCity_Gym_1F_EventScript_Juan
	end

Coop_EventScript_PairWallaceGymAfter::
	msgbox Coop_Text_PairWallaceGymAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairNoir::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_ELITE_4_SIDNEY, Coop_EventScript_PairNoirAfter
	msgbox Coop_Text_PairNoirIntro, MSGBOX_DEFAULT
	release
	goto EverGrandeCity_SidneysRoom_EventScript_Sidney
	end

Coop_EventScript_PairNoirAfter::
	msgbox Coop_Text_PairNoirAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairWraith::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_ELITE_4_PHOEBE, Coop_EventScript_PairWraithAfter
	msgbox Coop_Text_PairWraithIntro, MSGBOX_DEFAULT
	release
	goto EverGrandeCity_PhoebesRoom_EventScript_Phoebe
	end

Coop_EventScript_PairWraithAfter::
	msgbox Coop_Text_PairWraithAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairFrost::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_ELITE_4_GLACIA, Coop_EventScript_PairFrostAfter
	msgbox Coop_Text_PairFrostIntro, MSGBOX_DEFAULT
	release
	goto EverGrandeCity_GlaciasRoom_EventScript_Glacia
	end

Coop_EventScript_PairFrostAfter::
	msgbox Coop_Text_PairFrostAfter, MSGBOX_DEFAULT
	release
	end

Coop_EventScript_PairWyrm::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_ELITE_4_DRAKE, Coop_EventScript_PairWyrmAfter
	msgbox Coop_Text_PairWyrmIntro, MSGBOX_DEFAULT
	release
	goto EverGrandeCity_DrakesRoom_EventScript_Drake
	end

Coop_EventScript_PairWyrmAfter::
	msgbox Coop_Text_PairWyrmAfter, MSGBOX_DEFAULT
	release
	end

@ The Champions' room fight starts on entry; STEVEN stands beside WALLACE.
Coop_EventScript_PairStevenChampion::
	lock
	faceplayer
	msgbox Coop_Text_PairStevenChampion, MSGBOX_DEFAULT
	release
	end

Coop_Text_PairHeathIntro:
	.string "HEATH: ROXANNE and I battle as a pair.\nTalk to either of us.$"

Coop_Text_PairHeathAfter:
	.string "HEATH: Rock stands firm, and so do\nyou. Good luck on the road!$"

Coop_Text_PairReedIntro:
	.string "REED: BRAWLY and I ride the same\nwave. Ready for both of us?$"

Coop_Text_PairReedAfter:
	.string "REED: That was a big swell. You\nrode it out together.$"

Coop_Text_PairJouleIntro:
	.string "JOULE: WATTSON and I are wired in\nseries. Two of us, one battle!$"

Coop_Text_PairJouleAfter:
	.string "JOULE: You blew our fuse. Wahahah!\nWATTSON's laugh is catching.$"

Coop_Text_PairColeIntro:
	.string "COLE: FLANNERY brings the heat and\nI keep the sun up. Both of us!$"

Coop_Text_PairColeAfter:
	.string "COLE: Even the sun cools off\nsometimes. Well fought.$"

Coop_Text_PairHanaIntro:
	.string "HANA: NORMAN asked me to stand\nbeside him for this one.$"

Coop_Text_PairHanaAfter:
	.string "HANA: NORMAN is proud of you.\nHe just won't say it twice.$"

Coop_Text_PairGaleIntro:
	.string "GALE: WINONA and I fly in\nformation. Watch the wind!$"

Coop_Text_PairGaleAfter:
	.string "GALE: You flew right through our\ntailwind. Impressive.$"

Coop_Text_PairWallaceGymIntro:
	.string "WALLACE: My mentor JUAN and I will\nface you together.$"

Coop_Text_PairWallaceGymAfter:
	.string "WALLACE: Elegant battling. The\nLEAGUE awaits you both.$"

Coop_Text_PairNoirIntro:
	.string "NOIR: SIDNEY and I play dirty,\nbut we play fair. Both of us!$"

Coop_Text_PairNoirAfter:
	.string "NOIR: Nice. SIDNEY is still\ngrinning about that one.$"

Coop_Text_PairWraithIntro:
	.string "WRAITH: PHOEBE and I share the\nspirits of MT. PYRE. Shall we?$"

Coop_Text_PairWraithAfter:
	.string "WRAITH: The spirits are quiet\nnow. Go on ahead.$"

Coop_Text_PairFrostIntro:
	.string "FROST: GLACIA and I bring the\nsnow. Two of us, two of you.$"

Coop_Text_PairFrostAfter:
	.string "FROST: Your hearts burned hot\nenough to melt it all.$"

Coop_Text_PairWyrmIntro:
	.string "WYRM: DRAKE and I command dragons.\nFace us both!$"

Coop_Text_PairWyrmAfter:
	.string "WYRM: You understood your\nPOKéMON. Go to the CHAMPIONS.$"

Coop_Text_PairStevenChampion:
	.string "STEVEN: WALLACE and I hold this\nroom together. You earned it.$"
