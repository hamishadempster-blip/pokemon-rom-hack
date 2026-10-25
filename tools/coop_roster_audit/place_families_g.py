#!/usr/bin/env python3
"""Wild placements for the 24 families added in build 0xC0170131, and Hoenn
homes for the revamped Ledyba, Spinarak, Hoothoot, Sunkern and Seel lines.

Each entry replaces one slot of a map's base table; the native species keep
their other slots. Slot rates:
  land  (12): 20 20 10 10 10 10 5 5 4 4 1 1
  water  (5): 60 30 5 4 1
  fishing(10): old 70 30 | good 60 20 20 | super 40 40 15 4 1
Idempotent: rerunning writes the same table. Run after place_new_families.py.
"""
import json, os
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
PATH = os.path.join(ROOT, 'src/data/wild_encounters.json')

MAGMA = ['gMagmaHideout_1F', 'gMagmaHideout_2F_1R', 'gMagmaHideout_2F_2R', 'gMagmaHideout_2F_3R',
         'gMagmaHideout_3F_1R', 'gMagmaHideout_3F_2R', 'gMagmaHideout_3F_3R']

# map label -> [(kind, slot, species, minLevel, maxLevel)]
PLACEMENTS = {
    # Early coast: Buizel on Route 104's grass (20%) and its water (30%).
    'gRoute104': [('land', 2, 'BUIZEL', 5, 5), ('land', 5, 'BUIZEL', 4, 4),
                  ('water', 1, 'BUIZEL', 15, 20)],
    # Forest: Sewaddle and Venipede take the Silcoon/Cascoon slots (Wurmple
    # still evolves into both).
    'gPetalburgWoods': [('land', 4, 'SEWADDLE', 5, 5), ('land', 5, 'VENIPEDE', 5, 5)],
    # Mining tunnel: Rolycoly and Venipede join the Whismur.
    'gRusturfTunnel': [('land', 1, 'ROLYCOLY', 7, 7), ('land', 4, 'ROLYCOLY', 6, 6),
                       ('land', 3, 'VENIPEDE', 6, 6), ('land', 7, 'VENIPEDE', 8, 8)],
    'gRoute116': [('land', 4, 'SKIDDO', 7, 7), ('land', 8, 'SKIDDO', 8, 8)],
    # Dewford's cave: Drilbur on every floor.
    'gGraniteCave_1F': [('land', 9, 'DRILBUR', 9, 9)],
    'gGraniteCave_B1F': [('land', 2, 'DRILBUR', 10, 10)],
    'gGraniteCave_B2F': [('land', 2, 'DRILBUR', 11, 11)],
    'gRoute117': [('land', 4, 'FOMANTIS', 13, 13)],
    # Fiery Path: Darumaka, Sizzlipede, Salandit.
    'gFieryPath': [('land', 2, 'DARUMAKA', 15, 15), ('land', 7, 'DARUMAKA', 16, 16),
                   ('land', 3, 'SIZZLIPEDE', 15, 15),
                   ('land', 6, 'SALANDIT', 15, 15), ('land', 8, 'SALANDIT', 16, 16)],
    # Desert and mountain paths.
    'gRoute111': [('land', 2, 'SCRAGGY', 20, 20), ('land', 5, 'MUDBRAY', 21, 21)],
    'gRoute112': [('land', 2, 'MUDBRAY', 15, 15), ('land', 3, 'RUFFLET', 14, 14), ('land', 6, 'RUFFLET', 16, 16)],
    'gRoute113': [('land', 3, 'VULLABY', 14, 14), ('land', 6, 'VULLABY', 16, 16)],
    'gJaggedPass': [('land', 3, 'MUDBRAY', 21, 21), ('land', 6, 'RUFFLET', 21, 21), ('land', 7, 'VULLABY', 21, 21)],
    'gRoute115': [('land', 8, 'CLOBBOPUS', 23, 23), ('land', 9, 'CLOBBOPUS', 24, 24)],
    'gRoute118': [('land', 6, 'SKIDDO', 25, 25), ('land', 7, 'SKIDDO', 26, 26)],
    # Near Fortree.
    'gRoute119': [('land', 3, 'PANCHAM', 25, 25), ('land', 5, 'SOLOSIS', 25, 25)],
    'gRoute120': [('land', 3, 'GOTHITA', 25, 25), ('land', 5, 'SOLOSIS', 26, 26), ('land', 6, 'GOTHITA', 27, 27)],
    'gRoute123': [('land', 8, 'FOMANTIS', 26, 26), ('land', 9, 'FOMANTIS', 27, 27)],
    # Optional late cave (Waterfall): Deino in Meteor Falls' back room.
    'gMeteorFalls_B1F_2R': [('land', 3, 'DEINO', 32, 32), ('land', 7, 'DEINO', 35, 35)],
    # Shoal Cave: Alolan Vulpix in the ice rooms.
    'gShoalCave_LowTideIceRoom': [('land', 2, 'VULPIX_ALOLA', 28, 28)],
    'gShoalCave_LowTideLowerRoom': [('land', 4, 'VULPIX_ALOLA', 30, 30)],
    # New Mauville's maintenance areas: Rolycoly and Carkol.
    'gNewMauville_Inside': [('land', 4, 'ROLYCOLY', 23, 23), ('land', 8, 'CARKOL', 24, 24)],
    'gNewMauville_Entrance': [('land', 4, 'ROLYCOLY', 22, 22), ('land', 5, 'ROLYCOLY', 23, 23)],
    # Coast: Clauncher on the Good Rod, Skrelp on the water and Super Rod.
    'gRoute106': [('fishing', 4, 'CLAUNCHER', 15, 25)],
    'gRoute107': [('fishing', 4, 'CLAUNCHER', 15, 25)],
    'gDewfordTown': [('fishing', 4, 'CLAUNCHER', 15, 25)],
    'gRoute105': [('water', 2, 'SKRELP', 20, 25)],
    'gRoute108': [('fishing', 7, 'SKRELP', 25, 30)],
    'gRoute109': [('water', 2, 'SKRELP', 25, 30), ('fishing', 7, 'SKRELP', 25, 30)],
    # Industrial waterways: Slateport's harbour and the water under Route 110.
    'gSlateportCity': [('water', 1, 'GRIMER_ALOLA', 25, 30)],
    'gRoute110': [('water', 2, 'GRIMER_ALOLA', 20, 25)],
    # Volcanic: Salandit (4%) on every Magma Hideout floor, Larvesta rare.
    'gMagmaHideout_4F': [('land', 8, 'SALANDIT', 30, 30), ('land', 9, 'LARVESTA', 30, 30)],
}
# Revamped families from other regions get Hoenn homes (they had none).
PLACEMENTS.update({
    'gRoute102': [('land', 3, 'LEDYBA', 4, 4), ('land', 5, 'SPINARAK', 4, 4)],
    'gRoute125': [('water', 1, 'SEEL', 25, 30)],
    'gShoalCave_LowTideEntranceRoom': [('water', 2, 'SEEL', 25, 30), ('water', 4, 'DEWGONG', 34, 35)],
})
PLACEMENTS['gRoute116'] += [('land', 0, 'HOOTHOOT', 6, 6), ('land', 9, 'SUNKERN', 6, 6)]
PLACEMENTS['gRoute105'] += [('water', 1, 'SEEL', 15, 25)]
PLACEMENTS['gRoute123'] += [('land', 7, 'SUNKERN', 26, 26)]
PLACEMENTS['gRoute119'] += [('land', 1, 'SPINARAK', 24, 24)]
PLACEMENTS['gRoute121'] = [('land', 0, 'HOOTHOOT', 26, 26)]
PLACEMENTS['gRoute117'] += [('land', 11, 'LEDYBA', 13, 13)]

for label in MAGMA:
    PLACEMENTS[label] = [('land', 8, 'SALANDIT', 30, 30), ('land', 10, 'LARVESTA', 29, 29)]


def main():
    data = json.load(open(PATH, encoding='utf-8'))
    seen = set()
    for group in data['wild_encounter_groups']:
        for enc in group['encounters']:
            label = enc.get('base_label')
            if label not in PLACEMENTS:
                continue
            for kind, slot, species, lo, hi in PLACEMENTS[label]:
                table = enc[kind + '_mons']['mons']
                table[slot] = {'min_level': lo, 'max_level': hi, 'species': 'SPECIES_' + species}
            seen.add(label)
    missing = set(PLACEMENTS) - seen
    if missing:
        raise SystemExit('missing tables: %s' % sorted(missing))
    open(PATH, 'w', encoding='utf-8').write(json.dumps(data, indent=2, ensure_ascii=False) + '\n')


if __name__ == '__main__':
    main()
