@ The SANDBURIED RUINS (Desert caravan quest). The outer hall holds the
@ caravan's crates and the raiders; three glyphs open the sealed chamber
@ (each save's own) with its lore tablets and this save's SIGILYPH.
SandburiedRuins_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, SandburiedRuins_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, SandburiedRuins_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, SandburiedRuins_OnResume
	.byte 0

SandburiedRuins_OnLoad:
	call_if_unset FLAG_CARAVAN_CHAMBER_OPEN, SandburiedRuins_EventScript_SealChamber
	end

SandburiedRuins_EventScript_SealChamber::
	setmetatile 7, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 8, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 9, 19, METATILE_Cave_EntranceCover, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	setmetatile 9, 20, METATILE_Cave_SealedChamberBraille_Mid, TRUE
	return

SandburiedRuins_OnTransition:
	special CoopQuest_ApplyFoundObjects
	setflag FLAG_HIDE_CARAVAN_SIGILYPH
	call_if_set FLAG_CARAVAN_CHAMBER_OPEN, SandburiedRuins_EventScript_MaybeShowSigilyph
	clearflag FLAG_HIDE_CARAVAN_RAIDERS
	call_if_set FLAG_CARAVAN_RAIDERS_DUO, SandburiedRuins_EventScript_RaidersDuoFlags
	call_if_ge VAR_CARAVAN_QUEST, CARAVAN_MERCHANT, SandburiedRuins_EventScript_HideRaiders
	call_if_defeated TRAINER_DC_RAIDER_DUNE, SandburiedRuins_EventScript_MaybeHideRaiders
	end

SandburiedRuins_EventScript_MaybeShowSigilyph::
	goto_if_set FLAG_CARAVAN_SIGILYPH_DONE, Common_EventScript_NopReturn
	clearflag FLAG_HIDE_CARAVAN_SIGILYPH
	return

SandburiedRuins_EventScript_MaybeHideRaiders::
	goto_if_not_defeated TRAINER_DC_RAIDER_MESA, Common_EventScript_NopReturn
SandburiedRuins_EventScript_HideRaiders::
	setflag FLAG_HIDE_CARAVAN_RAIDERS
	return

SandburiedRuins_OnResume:
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, SandburiedRuins_EventScript_TryRemoveSigilyph
	call SandburiedRuins_EventScript_ApplyRaidersDuo
	end

SandburiedRuins_EventScript_TryRemoveSigilyph::
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, Common_EventScript_NopReturn
	removeobject VAR_LAST_TALKED
	return

@ Crates (OR-merged with the partner).
SandburiedRuins_EventScript_Crate1::
	lock
	setvar VAR_0x8005, 1
	goto SandburiedRuins_EventScript_Crate

SandburiedRuins_EventScript_Crate2::
	lock
	setvar VAR_0x8005, 2
	goto SandburiedRuins_EventScript_Crate

SandburiedRuins_EventScript_Crate3::
	lock
	goto_if_not_defeated TRAINER_DC_RAIDER_DUNE, SandburiedRuins_EventScript_CrateGuarded
	goto_if_not_defeated TRAINER_DC_RAIDER_MESA, SandburiedRuins_EventScript_CrateGuarded
	setvar VAR_0x8005, 4
SandburiedRuins_EventScript_Crate::
	setvar VAR_0x8004, COOP_QUEST_CARAVAN_FOUND
	special CoopQuest_SetBits
	playse SE_SELECT
	removeobject VAR_LAST_TALKED
	specialvar VAR_RESULT, CoopQuest_CountBits
	goto_if_ge VAR_RESULT, 3, SandburiedRuins_EventScript_CratesDone
	msgbox SandburiedRuins_Text_Crate, MSGBOX_DEFAULT
	release
	end

SandburiedRuins_EventScript_CratesDone::
	call_if_lt VAR_CARAVAN_QUEST, CARAVAN_FOUND, SandburiedRuins_EventScript_SetFound
	msgbox SandburiedRuins_Text_CratesDone, MSGBOX_DEFAULT
	release
	end

SandburiedRuins_EventScript_SetFound::
	setvar VAR_CARAVAN_QUEST, CARAVAN_FOUND
	return

SandburiedRuins_EventScript_CrateGuarded::
	msgbox SandburiedRuins_Text_CrateGuarded, MSGBOX_DEFAULT
	release
	end

@ The raiders: a duo with the partner (Beacon on, here), else one by one.
SandburiedRuins_EventScript_Dune::
	lock
	faceplayer
	goto_if_defeated TRAINER_DC_RAIDER_DUNE, SandburiedRuins_EventScript_RaiderAfter
	specialvar VAR_RESULT, CoopQuest_DuoReady
	goto_if_eq VAR_RESULT, FALSE, SandburiedRuins_EventScript_DuneSolo
	goto_if_defeated TRAINER_DC_RAIDER_MESA, SandburiedRuins_EventScript_DuneSolo
	special CoopDeal_BufferPartnerName
	msgbox SandburiedRuins_Text_RaidersAskDuo, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, SandburiedRuins_EventScript_RaidersDuo
SandburiedRuins_EventScript_DuneSolo::
	special CoopQuest_ForceSolo
	trainerbattle_single TRAINER_DC_RAIDER_DUNE, SandburiedRuins_Text_DuneIntro, SandburiedRuins_Text_DuneDefeat, SandburiedRuins_EventScript_RaidersBeaten
	goto SandburiedRuins_EventScript_RaiderAfter

@ The duo: a co-op multi battle that marks both saves
@ (FLAG_CARAVAN_RAIDERS_DUO) and resumes here; solo if the partner can't.
SandburiedRuins_EventScript_RaidersDuo::
	setvar VAR_0x8004, TRAINER_DC_RAIDER_DUNE
	setvar VAR_0x8005, TRAINER_DC_RAIDER_MESA
	setvar VAR_0x8006, 0
	setvar VAR_0x8007, FLAG_CARAVAN_RAIDERS_DUO
	special Coop_StartStoryBattle
	goto_if_eq VAR_RESULT, FALSE, SandburiedRuins_EventScript_DuneSolo
	call SandburiedRuins_EventScript_RaidersDuoFlags
	goto SandburiedRuins_EventScript_RaidersBeaten

SandburiedRuins_EventScript_RaidersDuoFlags::
	settrainerflag TRAINER_DC_RAIDER_DUNE
	settrainerflag TRAINER_DC_RAIDER_MESA
	return

SandburiedRuins_EventScript_ApplyRaidersDuo::
	goto_if_unset FLAG_CARAVAN_RAIDERS_DUO, Common_EventScript_NopReturn
	goto_if_set FLAG_HIDE_CARAVAN_RAIDERS, Common_EventScript_NopReturn
	call SandburiedRuins_EventScript_RaidersDuoFlags
	setflag FLAG_HIDE_CARAVAN_RAIDERS
	removeobject LOCALID_RUINS_DUNE
	removeobject LOCALID_RUINS_MESA
	return

SandburiedRuins_EventScript_Mesa::
	lock
	faceplayer
	goto_if_defeated TRAINER_DC_RAIDER_MESA, SandburiedRuins_EventScript_RaiderAfter
	special CoopQuest_ForceSolo
	trainerbattle_single TRAINER_DC_RAIDER_MESA, SandburiedRuins_Text_MesaIntro, SandburiedRuins_Text_MesaDefeat, SandburiedRuins_EventScript_RaidersBeaten
	goto SandburiedRuins_EventScript_RaiderAfter

SandburiedRuins_EventScript_RaidersBeaten::
	goto_if_not_defeated TRAINER_DC_RAIDER_DUNE, SandburiedRuins_EventScript_RaiderOneDown
	goto_if_not_defeated TRAINER_DC_RAIDER_MESA, SandburiedRuins_EventScript_RaiderOneDown
	msgbox SandburiedRuins_Text_RaidersFlee, MSGBOX_DEFAULT
	closemessage
	setflag FLAG_HIDE_CARAVAN_RAIDERS
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_RUINS_DUNE
	removeobject LOCALID_RUINS_MESA
	fadescreen FADE_FROM_BLACK
	release
	end

SandburiedRuins_EventScript_RaiderOneDown::
	msgbox SandburiedRuins_Text_RaiderOneDown, MSGBOX_DEFAULT
	release
	end

SandburiedRuins_EventScript_RaiderAfter::
	msgbox SandburiedRuins_Text_RaiderAfter, MSGBOX_DEFAULT
	release
	end

@ The glyph wall: SUN, then STAR, then MOON (ZAHRA's old map). A wrong
@ glyph starts over. Progress: bits 4-5 of VAR_CARAVAN_FOUND (not shared).
SandburiedRuins_EventScript_GlyphSun::
	lockall
	setvar VAR_0x8005, 0
	goto SandburiedRuins_EventScript_Glyph

SandburiedRuins_EventScript_GlyphStar::
	lockall
	setvar VAR_0x8005, 1
	goto SandburiedRuins_EventScript_Glyph

SandburiedRuins_EventScript_GlyphMoon::
	lockall
	setvar VAR_0x8005, 2
SandburiedRuins_EventScript_Glyph::
	goto_if_set FLAG_CARAVAN_CHAMBER_OPEN, SandburiedRuins_EventScript_GlyphOpen
	specialvar VAR_RESULT, CoopQuest_PressGlyph
	goto_if_eq VAR_RESULT, 0, SandburiedRuins_EventScript_GlyphWrong
	goto_if_eq VAR_RESULT, 2, SandburiedRuins_EventScript_GlyphSolved
	playse SE_SWITCH
	msgbox SandburiedRuins_Text_GlyphGlow, MSGBOX_DEFAULT
	releaseall
	end

SandburiedRuins_EventScript_GlyphWrong::
	playse SE_FAILURE
	msgbox SandburiedRuins_Text_GlyphWrong, MSGBOX_DEFAULT
	releaseall
	end

SandburiedRuins_EventScript_GlyphSolved::
	playse SE_SWITCH
	msgbox SandburiedRuins_Text_GlyphGlow, MSGBOX_DEFAULT
	closemessage
	setflag FLAG_CARAVAN_CHAMBER_OPEN
	setmetatile 7, 19, METATILE_Cave_SealedChamberEntrance_TopLeft, TRUE
	setmetatile 8, 19, METATILE_Cave_SealedChamberEntrance_TopMid, TRUE
	setmetatile 9, 19, METATILE_Cave_SealedChamberEntrance_TopRight, TRUE
	setmetatile 7, 20, METATILE_Cave_SealedChamberEntrance_BottomLeft, TRUE
	setmetatile 8, 20, METATILE_Cave_SealedChamberEntrance_BottomMid, FALSE
	setmetatile 9, 20, METATILE_Cave_SealedChamberEntrance_BottomRight, TRUE
	special DrawWholeMapView
	playse SE_DOOR
	msgbox SandburiedRuins_Text_ChamberOpens, MSGBOX_DEFAULT
	releaseall
	end

SandburiedRuins_EventScript_GlyphOpen::
	msgbox SandburiedRuins_Text_ChamberOpen, MSGBOX_DEFAULT
	releaseall
	end

SandburiedRuins_EventScript_Tablet::
	lockall
	setflag FLAG_CARAVAN_TABLETS_READ
	msgbox SandburiedRuins_Text_Tablet, MSGBOX_DEFAULT
	releaseall
	end

@ This save's SIGILYPH, the chamber's old guardian. It stays until caught.
SandburiedRuins_EventScript_Sigilyph::
	lock
	faceplayer
	setwildbattle SPECIES_SIGILYPH, 34
	waitse
	playmoncry SPECIES_SIGILYPH, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	dowildbattle
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, SandburiedRuins_EventScript_SigilyphDone
	msgbox SandburiedRuins_Text_SigilyphStays, MSGBOX_DEFAULT
	release
	end

SandburiedRuins_EventScript_SigilyphDone::
	setflag FLAG_CARAVAN_SIGILYPH_DONE
	release
	end

SandburiedRuins_Text_Crate:
	.string "A caravan crate, stamped with a\n"
	.string "camel-and-sun mark!\p"
	.string "Crates found: {STR_VAR_1} of 3.$"

SandburiedRuins_Text_CratesDone:
	.string "That's all three crates!\n"
	.string "Better tell ZAHRA at the camp.$"

SandburiedRuins_Text_CrateGuarded:
	.string "The raiders are guarding this\n"
	.string "crate closely.$"

SandburiedRuins_Text_RaidersAskDuo:
	.string "DUNE: Two treasure hunters? Then\n"
	.string "MESA and I fight you both!\p"
	.string "Battle together with {STR_VAR_1}?$"

SandburiedRuins_Text_DuneIntro:
	.string "DUNE: Finders keepers! The sand\n"
	.string "dropped these crates on us.$"

SandburiedRuins_Text_DuneDefeat:
	.string "Sand in my eyes…$"

SandburiedRuins_Text_MesaIntro:
	.string "MESA: The last crate is ours!$"

SandburiedRuins_Text_MesaDefeat:
	.string "Fine, you dig it out then.$"

SandburiedRuins_Text_RaiderOneDown:
	.string "We're not done yet! The other one\n"
	.string "of us still has a say.$"

SandburiedRuins_Text_RaidersFlee:
	.string "DUNE: Keep the crate. These ruins\n"
	.string "give me the creeps anyway.$"

SandburiedRuins_Text_RaiderAfter:
	.string "Those glyphs on the wall… we never\n"
	.string "could make them light up.$"

SandburiedRuins_Text_GlyphGlow:
	.string "The glyph glows faintly…$"

SandburiedRuins_Text_GlyphWrong:
	.string "The glyphs flicker and go dark.\n"
	.string "Maybe the order matters.$"

SandburiedRuins_Text_ChamberOpens:
	.string "All three glyphs blaze! The wall\n"
	.string "grinds open.$"

SandburiedRuins_Text_ChamberOpen:
	.string "The chamber stands open.$"

SandburiedRuins_Text_Tablet:
	.string "An old tablet:\p"
	.string "“Caravans rested here when the\n"
	.string "dunes were young.\p"
	.string "A painted bird kept watch, and the\n"
	.string "travellers left it gifts of sand.”$"

SandburiedRuins_Text_SigilyphStays:
	.string "SIGILYPH hovers back to its post.$"

@ The cursed carving (Galarian YAMASK -> RUNERIGUS). The stock trigger is a
@ stone arch in Galar's Dusty Bowl, passed by a Galarian Yamask that has
@ lost at least 49 HP; here the carving by the east wall does the same.
SandburiedRuins_EventScript_CursedCarving::
	lockall
	msgbox SandburiedRuins_Text_CursedCarving, MSGBOX_DEFAULT
	setvar VAR_0x8005, EVO_TRIGGER_TABLET_CURSE
	pickpartymon SELECT_PC_MON_EVOLUTION
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, SandburiedRuins_EventScript_CursedCarvingNothing
	tryspecialevo EVO_TRIGGER_TABLET_CURSE, FALSE
	releaseall
	end
SandburiedRuins_EventScript_CursedCarvingNothing::
	msgbox SandburiedRuins_Text_CursedCarvingNothing, MSGBOX_DEFAULT
	releaseall
	end

SandburiedRuins_Text_CursedCarving:
	.string "A carving of a stone slab, cracked\n"
	.string "down the middle. It feels cold.$"

SandburiedRuins_Text_CursedCarvingNothing:
	.string "A sign beneath it reads: “A GALARIAN\n"
	.string "YAMASK hurt by 49 HP or more is\l"
	.string "drawn into the slab.”$"
