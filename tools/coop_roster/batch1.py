#!/usr/bin/env python3
"""First roster expansion (114 species): wild placements and the manifest.

    tools/coop_roster/batch1.py          # apply to src/data/wild_encounters.json
    tools/coop_roster/batch1.py --check  # report only

Each placement replaces one slot that held a duplicate of a species already
in the same table (so no table loses a species, and the new arrival takes a
low-weight slot). Re-running is harmless: a slot already holding the new
species is left as it is.
Slot weights: land 20 20 10 10 10 10 5 5 4 4 1 1; water 60 30 5 4 1;
fishing old rod 70 30, good rod 60 20 20, super rod 40 40 15 4 1.
"""
import json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
JSON = os.path.join(ROOT, 'src/data/wild_encounters.json')

# (map, table, slot, expected old species, new species, min, max)
PLACEMENTS = [
    ('MAP_ROUTE102', 'land_mons', 2, 'POOCHYENA', 'FLETCHLING', 4, 4),
    ('MAP_ROUTE104', 'land_mons', 7, 'ROOKIDEE', 'STARLY', 5, 5),
    ('MAP_ROUTE104', 'land_mons', 9, 'WATTREL', 'HOPPIP', 5, 5),
    ('MAP_ROUTE104', 'water_mons', 2, 'WINGULL', 'SHELLOS_WEST', 15, 25),
    ('MAP_PETALBURG_WOODS', 'land_mons', 7, 'HATENNA', 'CUTIEFLY', 6, 6),
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 2, 'WHISMUR', 'TIMBURR', 6, 6),
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 5, 'WHISMUR', 'PARAS', 7, 7),
    ('MAP_ROUTE116', 'land_mons', 6, 'WOOLOO', 'MINCCINO', 7, 7),
    ('MAP_ROUTE116', 'land_mons', 8, 'SKIDDO', 'VENONAT', 8, 8),
    ('MAP_GRANITE_CAVE_B1F', 'land_mons', 9, 'SABLEYE', 'FERROSEED', 10, 10),
    ('MAP_GRANITE_CAVE_B2F', 'land_mons', 8, 'SABLEYE', 'LARVITAR', 12, 12),
    ('MAP_ROUTE110', 'land_mons', 6, 'MINUN', 'GRUBBIN', 13, 13),
    ('MAP_ROUTE117', 'land_mons', 2, 'POOCHYENA', 'DROWZEE', 14, 14),
    ('MAP_ROUTE117', 'land_mons', 8, 'COTTONEE', 'BOUNSWEET', 13, 13),
    ('MAP_ROUTE117', 'water_mons', 3, 'MARILL', 'DEWPIDER', 12, 16),
    ('MAP_NEW_MAUVILLE_INSIDE', 'land_mons', 9, 'MAGNEMITE', 'TYNAMO', 22, 22),
    ('MAP_ROUTE112', 'land_mons', 8, 'NUMEL', 'PONYTA', 16, 16),
    ('MAP_ROUTE112', 'land_mons', 9, 'NUMEL', 'GROWLITHE', 16, 16),
    ('MAP_FIERY_PATH', 'land_mons', 11, 'GRIMER', 'GROWLITHE', 15, 15),
    ('MAP_ROUTE111', 'land_mons', 3, 'TRAPINCH', 'SKORUPI', 21, 21),
    ('MAP_ROUTE111', 'land_mons', 7, 'TRAPINCH', 'GIBLE', 19, 19),
    ('MAP_ROUTE111', 'land_mons', 9, 'HELIOPTILE', 'SANDYGAST', 20, 20),
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 6, 'SOLROCK', 'AXEW', 18, 18),
    ('MAP_ROUTE119', 'land_mons', 9, 'TROPIUS', 'PHANTUMP', 26, 26),
    ('MAP_ROUTE119', 'fishing_mons', 8, 'CARVANHA', 'DRATINI', 25, 30),
    ('MAP_ROUTE120', 'land_mons', 7, 'MARILL', 'CROAGUNK', 27, 27),
    ('MAP_ROUTE120', 'land_mons', 9, 'ABSOL', 'ZORUA', 27, 27),
    ('MAP_ROUTE120', 'water_mons', 3, 'MARILL', 'DEWPIDER', 20, 25),
    ('MAP_ROUTE121', 'land_mons', 9, 'WINGULL', 'DRIFLOON', 27, 27),
    ('MAP_MT_PYRE_EXTERIOR', 'land_mons', 3, 'SHUPPET', 'PUMPKABOO_AVERAGE', 29, 29),
    ('MAP_MT_PYRE_EXTERIOR', 'land_mons', 8, 'WINGULL', 'MURKROW', 27, 27),
    ('MAP_MT_PYRE_EXTERIOR', 'land_mons', 10, 'WINGULL', 'PUMPKABOO_SUPER', 26, 26),
    ('MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM', 'land_mons', 5, 'SPHEAL', 'SNEASEL', 30, 30),
    ('MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM', 'land_mons', 7, 'SPHEAL', 'SWINUB', 28, 28),
    ('MAP_SHOAL_CAVE_LOW_TIDE_LOWER_ROOM', 'land_mons', 9, 'SPHEAL', 'SWINUB', 30, 30),
    ('MAP_SHOAL_CAVE_LOW_TIDE_LOWER_ROOM', 'land_mons', 11, 'SPHEAL', 'SNEASEL', 32, 32),
    ('MAP_ROUTE124', 'water_mons', 2, 'WINGULL', 'SHELLOS_EAST', 25, 30),
    ('MAP_ROUTE124', 'fishing_mons', 7, 'WAILMER', 'FRILLISH', 25, 30),
    ('MAP_ROUTE125', 'water_mons', 3, 'PELIPPER', 'MAREANIE', 25, 30),
    ('MAP_ROUTE126', 'water_mons', 2, 'WINGULL', 'MAREANIE', 28, 32),
    ('MAP_ROUTE126', 'fishing_mons', 7, 'WAILMER', 'FRILLISH', 25, 30),
]


def apply(check):
    d = json.load(open(JSON))
    tables = {}
    for e in d['wild_encounter_groups'][0]['encounters']:
        if e.get('map'):
            tables.setdefault(e['map'], []).append(e)
    changed, problems = 0, []
    for (m, kind, slot, old, new, lo, hi) in PLACEMENTS:
        ent = [e for e in tables.get(m, []) if kind in e]
        if not ent:
            problems.append('%s has no %s' % (m, kind))
            continue
        for e in ent:   # every time-of-day variant of the map
            mons = e[kind]['mons']
            cur = mons[slot]['species'].replace('SPECIES_', '')
            if cur == new:
                continue
            if cur != old:
                problems.append('%s %s slot %d holds %s, not %s' % (m, kind, slot, cur, old))
                continue
            others = [x['species'] for k, x in enumerate(mons) if k != slot]
            if 'SPECIES_' + old not in others:
                problems.append('%s %s slot %d: %s would leave the table' % (m, kind, slot, old))
                continue
            mons[slot] = {'min_level': lo, 'max_level': hi, 'species': 'SPECIES_' + new}
            changed += 1
    for p in problems:
        print('PROBLEM', p)
    if not check and changed:
        with open(JSON, 'w') as f:
            json.dump(d, f, indent=2, ensure_ascii=False)
            f.write('\n')
    print('%d slot(s) %s, %d placement(s), %d problem(s)' % (changed, 'to change' if check else 'changed', len(PLACEMENTS), len(problems)))
    return not problems


if __name__ == '__main__':
    sys.exit(0 if apply('--check' in sys.argv) else 1)
