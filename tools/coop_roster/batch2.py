#!/usr/bin/env python3
"""Second roster expansion (114 species): wild placements.

    tools/coop_roster/batch2.py          # apply to src/data/wild_encounters.json
    tools/coop_roster/batch2.py --check  # report only

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
    # Unova
    ('MAP_ROUTE101', 'land_mons', 6, 'WURMPLE', 'LILLIPUP', 3, 3),
    ('MAP_ROUTE101', 'land_mons', 7, 'POOCHYENA', 'SKWOVET', 3, 3),
    ('MAP_ROUTE103', 'land_mons', 2, 'POOCHYENA', 'PIDOVE', 3, 3),
    ('MAP_ROUTE103', 'water_mons', 1, 'WINGULL', 'CRAMORANT', 10, 20),
    ('MAP_ROUTE104', 'land_mons', 5, 'BUIZEL', 'PURRLOIN', 4, 4),
    ('MAP_PETALBURG_WOODS', 'land_mons', 8, 'TAILLOW', 'BLIPBUG', 5, 5),
    ('MAP_PETALBURG_CITY', 'water_mons', 1, 'MARILL', 'TYMPOLE', 10, 20),
    ('MAP_PETALBURG_CITY', 'fishing_mons', 4, 'CORPHISH', 'CHEWTLE', 10, 15),
    ('MAP_ROUTE102', 'fishing_mons', 4, 'CORPHISH', 'CHEWTLE', 10, 15),
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 6, 'WHISMUR', 'WOOBAT', 6, 6),
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 8, 'WHISMUR', 'WOOBAT', 6, 6),
    ('MAP_ROUTE110', 'land_mons', 3, 'SHINX', 'BLITZLE', 13, 13),
    ('MAP_ROUTE110', 'land_mons', 7, 'GULPIN', 'YAMPER', 13, 13),
    ('MAP_ROUTE117', 'land_mons', 5, 'SMOLIV', 'EEVEE', 13, 13),
    ('MAP_ROUTE117', 'land_mons', 6, 'ILLUMISE', 'GOSSIFLEUR', 13, 13),
    ('MAP_NEW_MAUVILLE_ENTRANCE', 'land_mons', 8, 'VOLTORB', 'TOXEL', 22, 22),
    ('MAP_NEW_MAUVILLE_ENTRANCE', 'land_mons', 9, 'MAGNEMITE', 'TOXEL', 22, 22),
    ('MAP_ROUTE113', 'land_mons', 1, 'SPINDA', 'TOGEPI', 15, 15),
    ('MAP_ROUTE113', 'land_mons', 4, 'SPINDA', 'MUNNA', 15, 15),
    ('MAP_ROUTE114', 'land_mons', 2, 'SWABLU', 'MILCERY', 16, 16),
    ('MAP_ROUTE114', 'land_mons', 3, 'ROCKRUFF', 'STONJOURNER', 16, 16),
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 3, 'ZUBAT', 'ELGYEM', 15, 15),
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 4, 'ZUBAT', 'CLEFFA', 14, 14),
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 8, 'ZUBAT', 'CLEFAIRY', 19, 19),
    ('MAP_METEOR_FALLS_B1F_1R', 'land_mons', 3, 'SOLROCK', 'DRUDDIGON', 33, 33),
    ('MAP_JAGGED_PASS', 'land_mons', 4, 'SPOINK', 'FALINKS', 21, 21),
    ('MAP_SANDBURIED_RUINS', 'land_mons', 3, 'SANDILE', 'SILICOBRA', 30, 30),
    ('MAP_ROUTE107', 'water_mons', 1, 'WINGULL', 'PINCURCHIN', 15, 20),
    ('MAP_ROUTE118', 'water_mons', 1, 'WINGULL', 'DUCKLETT', 10, 20),
    ('MAP_ROUTE118', 'land_mons', 6, 'SKIDDO', 'MORPEKO', 25, 25),
    ('MAP_ROUTE119', 'land_mons', 2, 'FOONGUS', 'EMOLGA', 25, 25),
    ('MAP_ROUTE120', 'land_mons', 1, 'MIGHTYENA', 'DEERLING', 25, 25),
    ('MAP_ROUTE120', 'land_mons', 6, 'GOTHITA', 'AUDINO', 27, 27),
    ('MAP_ROUTE123', 'land_mons', 2, 'MIGHTYENA', 'BOUFFALANT', 27, 27),
    ('MAP_MT_PYRE_1F', 'land_mons', 2, 'SHUPPET', 'SINISTEA_PHONY', 26, 26),
    ('MAP_MT_PYRE_1F', 'land_mons', 9, 'SHUPPET', 'SINISTEA_ANTIQUE', 24, 24),
    ('MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM', 'land_mons', 1, 'SPHEAL', 'VANILLITE', 26, 26),
    ('MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM', 'land_mons', 8, 'GOLBAT', 'EISCUE', 30, 30),
    ('MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM', 'land_mons', 9, 'SNOM', 'EISCUE', 28, 28),
]

# Access repairs (not batch 2): species in the Hoenn Pokedex that stock Emerald
# only offers by trade (Ruby/Sapphire exclusives) or as Galarian forms the
# Pokedex lists. Same duplicate-slot rule.
REPAIRS = [
    ('MAP_ROUTE102', 'land_mons', 7, 'ZIGZAGOON', 'SURSKIT', 3, 3),
    ('MAP_ROUTE103', 'land_mons', 6, 'ZIGZAGOON', 'BUDEW', 3, 3),
    ('MAP_ROUTE103', 'land_mons', 7, 'ZIGZAGOON', 'ZIGZAGOON_GALAR', 4, 4),
    ('MAP_ROUTE114', 'land_mons', 7, 'LOMBRE', 'ZANGOOSE', 17, 17),
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 7, 'SOLROCK', 'LUNATONE', 14, 14),
    ('MAP_MT_PYRE_EXTERIOR', 'land_mons', 5, 'VULPIX', 'MEDITITE', 27, 27),
    ('MAP_ABANDONED_SHIP_ROOMS_B1F', 'water_mons', 2, 'TENTACOOL', 'CORSOLA_GALAR', 25, 30),
    ('MAP_ABANDONED_SHIP_HIDDEN_FLOOR_CORRIDORS', 'water_mons', 2, 'TENTACOOL', 'CORSOLA_GALAR', 25, 30),
]
PLACEMENTS = PLACEMENTS + REPAIRS


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
