@ The breeding guide (Trainer Exchange breeding counter, and the guide in
@ the Route 117 Day Care). The rules are this engine's: src/daycare.c.

CoopBreeding_EventScript_Guide::
	msgbox CoopBreeding_Text_Guide, MSGBOX_NPC
	end

CoopBreeding_Text_Guide:
	.string "Leave two POKéMON at the DAY CARE:\n"
	.string "a male and a female of one EGG\l"
	.string "GROUP, or one with DITTO. EGGS come\l"
	.string "as you walk; only your own steps\l"
	.string "count, and they hatch the same way.\p"
	.string "The EGG is the mother's first stage\n"
	.string "(with DITTO, the other parent's).\l"
	.string "Babies like AZURILL hatch with no\l"
	.string "INCENSE needed.\p"
	.string "It usually gets the mother's\n"
	.string "ability. An EVERSTONE passes the\l"
	.string "holder's own nature, not a MINT's.\p"
	.string "Three IVs come from the parents;\n"
	.string "five with a DESTINY KNOT. A POWER\l"
	.string "item passes its stat's IV. HYPER\l"
	.string "TRAINING isn't passed on.\p"
	.string "EGG MOVES known by either parent\n"
	.string "pass down. A parent holding a\l"
	.string "MIRROR HERB, with a free move slot,\l"
	.string "learns the other's EGG MOVES when\l"
	.string "the two are left together.\p"
	.string "A regional form passes on when the\n"
	.string "mother holds an EVERSTONE; without\l"
	.string "one it hatches in its usual form.\p"
	.string "EGGS can't battle and can't be\n"
	.string "traded or given. DITTO lives in the\l"
	.string "DESERT UNDERPASS after the HALL OF\l"
	.string "FAME.$"
