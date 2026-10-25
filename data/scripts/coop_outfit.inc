@ The wardrobe in both Littleroot bedrooms: outfit colours
@ (src/coop_outfit.c). Free, any time, linked or not.
Coop_EventScript_Wardrobe::
	lockall
	msgbox Coop_Text_WardrobeAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Coop_EventScript_WardrobeEnd
	closemessage
	special CoopOutfit_OpenMenu
	waitstate
	goto_if_eq VAR_RESULT, FALSE, Coop_EventScript_WardrobeEnd
	msgbox Coop_Text_WardrobeDone, MSGBOX_DEFAULT
Coop_EventScript_WardrobeEnd::
	releaseall
	end

Coop_Text_WardrobeAsk:
	.string "A wardrobe full of clothes in all\n"
	.string "sorts of colours. Change outfit?$"

Coop_Text_WardrobeDone:
	.string "Looking sharp! Your partner will\n"
	.string "see the new colours too.$"
