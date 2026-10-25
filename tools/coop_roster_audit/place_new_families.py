#!/usr/bin/env python3
"""Wild placements for the 14 families added in build 0xC017012F.

Each entry replaces one land slot (index into the 12-slot land table, rates
20,20,10,10,10,10,5,5,4,4,1,1) on a map's base table. Native species keep
their other slots. Idempotent: rerunning writes the same table.
"""
import json, os
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
PATH = os.path.join(ROOT, 'src/data/wild_encounters.json')

# map label -> [(slot, species, level)]
PLACEMENTS = {
    'gPetalburgWoods':   [(6, 'HATENNA', 5), (7, 'HATENNA', 6)],
    'gRoute104':         [(8, 'WATTREL', 4), (9, 'WATTREL', 5)],
    'gGraniteCave_1F':   [(2, 'RIOLU', 9), (7, 'RIOLU', 10)],
    'gRoute110':         [(0, 'SHINX', 12), (3, 'SHINX', 13), (8, 'WATTREL', 12), (9, 'WATTREL', 13)],
    'gRoute117':         [(3, 'COTTONEE', 14), (5, 'SMOLIV', 13), (8, 'COTTONEE', 13), (9, 'SMOLIV', 14)],
    'gRoute114':         [(3, 'ROCKRUFF', 16), (6, 'ROCKRUFF', 17), (9, 'ROCKRUFF_OWN_TEMPO', 16)],
    'gNewMauville_Inside': [(1, 'JOLTIK', 24), (5, 'JOLTIK', 23), (6, 'ROTOM', 25), (7, 'ROTOM', 24)],
    'gMtPyre_1F':        [(3, 'LITWICK', 25)],
    'gMtPyre_2F':        [(2, 'LITWICK', 26), (3, 'LITWICK', 25)],
    'gMtPyre_3F':        [(2, 'LITWICK', 27), (3, 'LITWICK', 26)],
    'gRoute119':         [(2, 'FOONGUS', 25), (6, 'FOONGUS', 27)],
    'gAlteringCave1':    [(1, 'HONEDGE', 27), (4, 'HONEDGE', 28), (6, 'HONEDGE', 30), (9, 'HONEDGE', 26)],
    'gRoute123':         [(5, 'PETILIL', 26), (6, 'PETILIL', 28)],
    'gShoalCave_LowTideIceRoom':   [(4, 'SNOM', 30), (9, 'SNOM', 28)],
    'gShoalCave_LowTideInnerRoom': [(6, 'SNOM', 32)],
}

def main():
    raw = open(PATH, encoding='utf-8').read()
    data = json.loads(raw)
    seen = set()
    for group in data['wild_encounter_groups']:
        for enc in group['encounters']:
            label = enc.get('base_label')
            if label not in PLACEMENTS:
                continue
            mons = enc['land_mons']['mons']
            for slot, species, level in PLACEMENTS[label]:
                mons[slot] = {'min_level': level, 'max_level': level, 'species': 'SPECIES_' + species}
            seen.add(label)
    missing = set(PLACEMENTS) - seen
    if missing:
        raise SystemExit('missing tables: %s' % sorted(missing))
    open(PATH, 'w', encoding='utf-8').write(json.dumps(data, indent=2, ensure_ascii=False) + '\n')

if __name__ == '__main__':
    main()
