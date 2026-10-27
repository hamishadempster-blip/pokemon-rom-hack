#!/usr/bin/env python3
"""Generate data/scripts/coop_field_guide.inc: the field researcher's list of
where the added Pokemon live and how the unusual ones evolve.

    tools/coop_field_guide.py && tools/coop_text_width.py data/scripts/coop_field_guide.inc
"""
import os
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

# (menu name, where, how it evolves). \\n breaks a line, \\l scrolls.
GUIDE = [
    ('BUIZEL', 'ROUTE 104, in the grass and on\nthe water.', 'Level 26: FLOATZEL.'),
    ('SEWADDLE', 'PETALBURG WOODS.', 'Level 20: SWADLOON. SWADLOON\nevolves into LEAVANNY when it levels\\lup while very friendly.'),
    ('VENIPEDE', 'PETALBURG WOODS and RUSTURF\nTUNNEL.', 'Level 22: WHIRLIPEDE.\nLevel 30: SCOLIPEDE.'),
    ('LEDYBA', 'ROUTE 102 and ROUTE 117.', 'Level 18: LEDIAN, a punching fighter\nwith IRON FIST.'),
    ('SPINARAK', 'ROUTE 102 and ROUTE 119.', 'Level 22: ARIADOS, a sturdy trapper.'),
    ('HOOTHOOT', 'ROUTE 116 and ROUTE 121.', 'Level 20: NOCTOWL, with screens\nand ROOST.'),
    ('SUNKERN', 'ROUTE 116, ROUTE 123 and the\nSAFARI ZONE.', 'A SUN STONE: SUNFLORA, now\nGRASS and FIRE. VERDANTURF and\\lLILYCOVE sell SUN STONES.'),
    ('ROLYCOLY', 'RUSTURF TUNNEL and NEW MAUVILLE.', 'Level 18: CARKOL.\nLevel 34: COALOSSAL.'),
    ('SKIDDO', 'ROUTE 116 and ROUTE 118.', 'Level 32: GOGOAT.'),
    ('DRILBUR', 'GRANITE CAVE near DEWFORD.', 'Level 31: EXCADRILL.'),
    ('FOMANTIS', 'ROUTE 117 and ROUTE 123.', 'Level 34 between 6:00 and 19:59:\nLURANTIS. Not at night.'),
    ('DARUMAKA', 'FIERY PATH.', 'Level 35: DARMANITAN.'),
    ('SIZZLIPEDE', 'FIERY PATH.', 'Level 28: CENTISKORCH.'),
    ('SALANDIT', 'FIERY PATH and the MAGMA HIDEOUT.\nA woman in LAVARIDGE gives a female.', 'Only a female evolves: level 33,\nSALAZZLE. Males never evolve.'),
    ('SCRAGGY', 'The ROUTE 111 desert and the\nSANDBURIED RUINS.', 'Level 39: SCRAFTY.'),
    ('MUDBRAY', 'The ROUTE 111 desert, the SANDBURIED\nRUINS, ROUTE 112 and JAGGED PASS.', 'Level 30: MUDSDALE.'),
    ('RUFFLET', 'ROUTE 112 and JAGGED PASS.', 'Level 54: BRAVIARY.'),
    ('VULLABY', 'ROUTE 113 and JAGGED PASS.', 'Level 54: MANDIBUZZ.'),
    ('CLOBBOPUS', 'ROUTE 115, on the rocky beach.', 'Level up knowing TAUNT: GRAPPLOCT.\nIt learns TAUNT itself at level 35.'),
    ('CLAUNCHER', 'GOOD ROD on ROUTES 106 and 107, and\nin DEWFORD.', 'Level 37: CLAWITZER.'),
    ('SKRELP', 'Surf on ROUTES 105 and 109. Any rod\non ROUTE 109 and in DEWFORD once\\lROUTE 109 is cleaned up.', 'Level 48: DRAGALGE.'),
    ('SEEL', 'Surf on ROUTES 105 and 125, and in\nSHOAL CAVE.', 'Level 34: DEWGONG, a support that\nheals its partner.'),
    ('GRIMER', 'ALOLAN GRIMER: surf in SLATEPORT\nharbour and under ROUTE 110. Two\\lcrawl on ROUTE 109 until it is clean.', 'Level 38: ALOLAN MUK.'),
    ('PANCHAM', 'ROUTE 119.', 'Level 32 with a DARK-type POKéMON\nin the party: PANGORO.'),
    ('SOLOSIS', 'ROUTE 119 and ROUTE 120.', 'Level 32: DUOSION.\nLevel 41: REUNICLUS.'),
    ('GOTHITA', 'ROUTE 120.', 'Level 32: GOTHORITA.\nLevel 41: GOTHITELLE.'),
    ('VULPIX', 'ALOLAN VULPIX: the SHOAL CAVE ice\nrooms at low tide, and one by the\\lladder once Dr. ISLA is safe.', 'An ICE STONE: ALOLAN NINETALES.\nDr. ISLA gives one a day at SHOAL\\lCAVE. LILYCOVE also sells them.'),
    ('LARVESTA', 'The MAGMA HIDEOUT. It is rare.', 'Level 59: VOLCARONA.'),
    ('DEINO', 'METEOR FALLS, in the back room you\nreach with WATERFALL.', 'Level 50: ZWEILOUS.\nLevel 64: HYDREIGON.'),
    ('RIOLU', 'GRANITE CAVE 1F.', 'Level up while very friendly between\n6:00 and 19:59: LUCARIO.'),
    ('SNOM', 'SHOAL CAVE ice rooms at low tide.', 'Level up while very friendly at\nnight (20:00 to 5:59): FROSMOTH.'),
    ('ROCKRUFF', 'ROUTE 114.', 'Level 25: MIDDAY LYCANROC by day,\nMIDNIGHT at night. An OWN TEMPO\\lROCKRUFF becomes DUSK at any hour.'),
    ('ROTOM', 'NEW MAUVILLE, and in its APPLIANCE\nROOM once the power is fixed.', "The scientist at NEW MAUVILLE's door\ngives a ROTOM CATALOG to change it."),
    ('JOLTIK', 'NEW MAUVILLE.', 'Level 36: GALVANTULA.'),
    ('PORYGON', 'A sealed chamber in NEW MAUVILLE.\nOld log pages show the way.', 'An UP-GRADE: PORYGON2. A DUBIOUS\nDISC: PORYGON-Z. MAUVILLE sells both.'),
    ('SIGILYPH', 'The sealed chamber of the\nSANDBURIED RUINS. Its glyphs open it.', 'It does not evolve.'),
    ('COTTONEE', 'ROUTE 117. PETILIL: ROUTE 123.', 'A SUN STONE: WHIMSICOTT or\nLILLIGANT. VERDANTURF and LILYCOVE.'),
    ('LITWICK', 'MT. PYRE. HONEDGE: ALTERING CAVE.', 'LAMPENT and DOUBLADE need a\nDUSK STONE from LILYCOVE.'),
]


# The first roster expansion (tools/coop_roster/batch1.py).
GUIDE2 = [
    ('FLETCHLING', 'ROUTE 102.', 'Level 17: FLETCHINDER.\nLevel 35: TALONFLAME.'),
    ('STARLY', 'ROUTE 104.', 'Level 14: STARAVIA.\nLevel 34: STARAPTOR.'),
    ('HOPPIP', 'ROUTE 104, among the flowers.', 'Level 18: SKIPLOOM.\nLevel 27: JUMPLUFF.'),
    ('CUTIEFLY', 'PETALBURG WOODS.', 'Level 25: RIBOMBEE.'),
    ('TIMBURR', 'RUSTURF TUNNEL, with the diggers.', 'Level 25: GURDURR. A LINKING CORD\nor a trade: CONKELDURR. LILYCOVE\\land MAUVILLE sell LINKING CORDS.'),
    ('PARAS', 'RUSTURF TUNNEL.', 'Level 24: PARASECT, now a sturdy\nDRY SKIN fighter.'),
    ('MINCCINO', 'ROUTE 116.', 'A SHINY STONE: CINCCINO. FORTREE\nand LILYCOVE sell SHINY STONES.'),
    ('VENONAT', 'ROUTE 116.', 'Level 31: VENOMOTH.'),
    ('FERROSEED', 'GRANITE CAVE B1F.', 'Level 40: FERROTHORN.'),
    ('LARVITAR', 'GRANITE CAVE B2F. It is rare.', 'Level 30: PUPITAR.\nLevel 55: TYRANITAR.'),
    ('GRUBBIN', 'ROUTE 110.', 'Level 20: CHARJABUG. Level it up in\nNEW MAUVILLE, or use a THUNDER\\lSTONE: VIKAVOLT.'),
    ('BOUNSWEET', 'ROUTE 117.', 'Level 18: STEENEE. STEENEE learns\nSTOMP at 28; then a level up:\\lTSAREENA.'),
    ('DROWZEE', 'ROUTE 117.', 'Level 26: HYPNO, a bulky support\nwith TELEPATHY.'),
    ('DEWPIDER', 'Surf on ROUTES 117 and 120.', 'Level 22: ARAQUANID.'),
    ('TYNAMO', 'NEW MAUVILLE.', 'Level 39: EELEKTRIK. A THUNDER\nSTONE: EELEKTROSS.'),
    ('GROWLITHE', 'ROUTE 112 and FIERY PATH.', 'A FIRE STONE: ARCANINE. LAVARIDGE\nand LILYCOVE sell FIRE STONES.'),
    ('PONYTA', 'ROUTE 112.', 'Level 40: RAPIDASH.'),
    ('SKORUPI', 'The ROUTE 111 desert.', 'Level 40: DRAPION.'),
    ('GIBLE', 'The ROUTE 111 desert. It hides in\nthe sand and is rare.', 'Level 24: GABITE.\nLevel 48: GARCHOMP.'),
    ('SANDYGAST', 'The ROUTE 111 desert.', 'Level 42: PALOSSAND.'),
    ('AXEW', 'METEOR FALLS, first room.', 'Level 38: FRAXURE.\nLevel 48: HAXORUS.'),
    ('PHANTUMP', 'ROUTE 119.', 'A LINKING CORD or a trade:\nTREVENANT.'),
    ('DRATINI', 'SUPER ROD on ROUTE 119. It is rare.', 'Level 30: DRAGONAIR.\nLevel 55: DRAGONITE.'),
    ('CROAGUNK', 'ROUTE 120.', 'Level 37: TOXICROAK.'),
    ('ZORUA', 'ROUTE 120.', 'Level 30: ZOROARK.'),
    ('DRIFLOON', 'ROUTE 121, on the wind.', 'Level 28: DRIFBLIM.'),
    ('PUMPKABOO', 'MT. PYRE, outside. A huge one is\nvery rare.', 'A LINKING CORD or a trade:\nGOURGEIST, the same size.'),
    ('MURKROW', 'MT. PYRE, outside.', 'A DUSK STONE: HONCHKROW. LILYCOVE\nand MAUVILLE sell DUSK STONES.'),
    ('SWINUB', 'SHOAL CAVE at low tide.', 'Level 33: PILOSWINE. It needs ANCIENT\nPOWER (the MOVE REMINDER) to level\\lup into MAMOSWINE.'),
    ('SNEASEL', 'SHOAL CAVE ice rooms at low tide.', 'At night, a RAZOR CLAW used on it\nor held while it levels: WEAVILE.\\lMAUVILLE sells RAZOR CLAWS.'),
    ('SHELLOS', 'Surf on ROUTE 104 (pink, WEST\nSEA) and ROUTE 124 (blue, EAST SEA).', 'Level 30: GASTRODON of the same sea.'),
    ('MAREANIE', 'Surf on ROUTES 125 and 126.', 'Level 38: TOXAPEX.'),
    ('FRILLISH', 'SUPER ROD on ROUTES 124 and 126.', 'Level 40: JELLICENT.'),
]

# Partner Pokemon from other regions (src/coop_roster.c).
GUIDE3 = [
    ('KANTO', 'A visitor in the RUSTBORO POKéMON\nCENTER, after 1 BADGE: BULBASAUR,\\lCHARMANDER or SQUIRTLE, Lv 10.', 'One a day. They evolve at level\n16, then 32 or 36.'),
    ('SINNOH', 'A visitor in the LAVARIDGE POKéMON\nCENTER, after 4 BADGES: TURTWIG,\\lCHIMCHAR or PIPLUP, Lv 18.', 'One a day. They evolve at level\n14 to 18, then 32 or 36.'),
    ('UNOVA', 'A visitor in the FORTREE POKéMON\nCENTER, after 6 BADGES: SNIVY,\\lTEPIG or OSHAWOTT, Lv 25.', 'One a day. They evolve at level\n17, then 36.'),
    ('KALOS', 'A visitor in the MOSSDEEP POKéMON\nCENTER, after 7 BADGES: CHESPIN,\\lFENNEKIN or FROAKIE, Lv 30.', 'One a day. They evolve at level\n16, then 36.'),
]

GROUPS = [('EARLIER ARRIVALS', GUIDE), ('NEW ARRIVALS', GUIDE2), ('PARTNER POKéMON', GUIDE3)]


def strblock(label, text):
    lines = text.split('\n')
    out = [label + ':']
    for k, ln in enumerate(lines):
        out.append('\t.string "%s%s"' % (ln, '$' if k == len(lines) - 1 else '\\n'))
    return '\n'.join(out)


def main():
    L = ['@ Generated by tools/coop_field_guide.py. Where the added POKéMON live and',
         '@ how the unusual ones evolve; shared by four POKéMON CENTERS.',
         'Coop_EventScript_FieldResearcher::',
         '\tlock', '\tfaceplayer', '\tmsgbox Coop_Text_FieldGuideIntro, MSGBOX_DEFAULT',
         'Coop_EventScript_FieldGuideMenu::',
         '\tmessage Coop_Text_FieldGuideGroup', '\twaitmessage']
    L += ['\tdynmultipush Coop_Text_FG_Group%d, %d' % (g, g) for g in range(len(GROUPS))]
    L += ['\tdynmultipush gText_Cancel, %d' % len(GROUPS),
          '\tdynmultistack 15, 1, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE',
          '\tgoto_if_ge VAR_RESULT, %d, Coop_EventScript_FieldGuideBye' % len(GROUPS)]
    L += ['\tgoto_if_eq VAR_RESULT, %d, Coop_EventScript_FG_List%d' % (g, g) for g in range(len(GROUPS))]
    L += ['\tgoto Coop_EventScript_FieldGuideBye']
    for g, (title, entries) in enumerate(GROUPS):
        n = len(entries)
        L += ['Coop_EventScript_FG_List%d:' % g, '\tmessage Coop_Text_FieldGuideAsk', '\twaitmessage']
        L += ['\tdynmultipush Coop_Text_FG_Name%d_%d, %d' % (g, i, i) for i in range(n)]
        L += ['\tdynmultipush gText_Cancel, %d' % n,
              '\tdynmultistack 17, 1, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE',
              '\tgoto_if_ge VAR_RESULT, %d, Coop_EventScript_FieldGuideMenu' % n]
        L += ['\tgoto_if_eq VAR_RESULT, %d, Coop_EventScript_FG_%d_%d' % (i, g, i) for i in range(n)]
        L += ['\tgoto Coop_EventScript_FieldGuideMenu']
        for i in range(n):
            L += ['Coop_EventScript_FG_%d_%d:' % (g, i), '\tmsgbox Coop_Text_FG_Where%d_%d, MSGBOX_DEFAULT' % (g, i),
                  '\tmsgbox Coop_Text_FG_Evo%d_%d, MSGBOX_DEFAULT' % (g, i), '\tgoto Coop_EventScript_FG_List%d' % g]
    L += ['Coop_EventScript_FieldGuideBye::', '\tmsgbox Coop_Text_FieldGuideBye, MSGBOX_DEFAULT',
          '\trelease', '\tend', '']
    L.append(strblock('Coop_Text_FieldGuideIntro', 'I study the POKéMON that have\nsettled in HOENN lately.\\p'
                      'Day and night go by your clock.\nLinked, both of you share the\\l'
                      "host's time of day."))
    L.append(strblock('Coop_Text_FieldGuideGroup', 'Which POKéMON would you like to\nhear about?'))
    L.append(strblock('Coop_Text_FieldGuideAsk', 'Which one would you like to know\nabout?'))
    L.append(strblock('Coop_Text_FieldGuideBye', 'Each trainer catches their own.\nGood luck out there!'))
    for g, (title, entries) in enumerate(GROUPS):
        L.append('Coop_Text_FG_Group%d:\n\t.string "%s$"' % (g, title))
        for i, (name, where, evo) in enumerate(entries):
            L.append('Coop_Text_FG_Name%d_%d:\n\t.string "%s$"' % (g, i, name))
            L.append(strblock('Coop_Text_FG_Where%d_%d' % (g, i), where))
            L.append(strblock('Coop_Text_FG_Evo%d_%d' % (g, i), evo))
    open(os.path.join(ROOT, 'data/scripts/coop_field_guide.inc'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')


if __name__ == '__main__':
    main()
