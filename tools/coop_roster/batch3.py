#!/usr/bin/env python3
"""Third (final) roster expansion: wild placements for the remaining ordinary
species and regional forms.

    tools/coop_roster/batch3.py          # apply to src/data/wild_encounters.json
    tools/coop_roster/batch3.py --check  # report only

Each placement replaces one slot. Normally the slot held a duplicate of a
species already in the same table; where a table had no spare slot, it takes
a slot of a widespread stock species (Poochyena, Zigzagoon, Oddish, ...),
which must stay in at least three tables (checked against the previous
build's manifest, docs/coop_roster_manifest.json before this batch: no
species lost a source or became obtainable later). The new arrival keeps the
slot's levels. No slot under 4 percent is used. Picked by a slot picker
(spare duplicates first), then fixed here so re-running is reproducible.
"""
import json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
JSON = os.path.join(ROOT, 'src/data/wild_encounters.json')

# (map, table, slot, expected old species, new species, min, max)  # slot weight
PLACEMENTS = [
    ('MAP_ROUTE101', 'land_mons', 2, 'WURMPLE', 'CATERPIE', 2, 2),  # 10%
    ('MAP_ROUTE101', 'land_mons', 4, 'POOCHYENA', 'WEEDLE', 3, 3),  # 10%
    ('MAP_ROUTE101', 'land_mons', 3, 'WURMPLE', 'PIDGEY', 3, 3),  # 10%
    ('MAP_ROUTE101', 'land_mons', 5, 'POOCHYENA', 'SENTRET', 3, 3),  # 10%
    ('MAP_ROUTE101', 'land_mons', 9, 'ZIGZAGOON', 'BIDOOF', 2, 2),  # 4%
    ('MAP_ROUTE101', 'land_mons', 8, 'ZIGZAGOON', 'KRICKETOT', 2, 2),  # 4%
    ('MAP_ROUTE104', 'land_mons', 0, 'POOCHYENA', 'SCATTERBUG', 4, 4),  # 20%
    ('MAP_ROUTE103', 'land_mons', 3, 'POOCHYENA', 'RATTATA', 4, 4),  # 10%
    ('MAP_ROUTE103', 'land_mons', 8, 'WINGULL', 'SPEAROW', 3, 3),  # 4%
    ('MAP_ROUTE103', 'land_mons', 9, 'WINGULL', 'PATRAT', 3, 3),  # 4%
    ('MAP_ROUTE103', 'land_mons', 1, 'POOCHYENA', 'YUNGOOS', 3, 3),  # 20%
    ('MAP_ROUTE103', 'land_mons', 4, 'WINGULL', 'PIKIPEK', 2, 2),  # 10%
    ('MAP_ROUTE103', 'land_mons', 5, 'ZIGZAGOON', 'NYMBLE', 3, 3),  # 10%
    ('MAP_PETALBURG_WOODS', 'land_mons', 9, 'SLAKOTH', 'BURMY', 5, 5),  # 4%
    ('MAP_PETALBURG_WOODS', 'land_mons', 0, 'POOCHYENA', 'COMBEE', 5, 5),  # 20%
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 4, 'ROLYCOLY', 'DIGLETT', 6, 6),  # 10%
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 7, 'VENIPEDE', 'BUNNELBY', 8, 8),  # 5%
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 9, 'WHISMUR', 'DUNSPARCE', 8, 8),  # 4%
    ('MAP_RUSTURF_TUNNEL', 'land_mons', 8, 'WOOBAT', 'KLINK', 6, 6),  # 4%
    ('MAP_ROUTE111', 'rock_smash_mons', 2, 'GEODUDE', 'DURANT', 15, 20),  # 5%
    ('MAP_GRANITE_CAVE_1F', 'land_mons', 4, 'MAKUHITA', 'ONIX', 9, 9),  # 10%
    ('MAP_GRANITE_CAVE_1F', 'land_mons', 7, 'RIOLU', 'GEODUDE_ALOLA', 10, 10),  # 5%
    ('MAP_GRANITE_CAVE_B1F', 'land_mons', 3, 'ARON', 'DIGLETT_ALOLA', 11, 11),  # 10%
    ('MAP_GRANITE_CAVE_B2F', 'land_mons', 3, 'ZUBAT', 'CARBINK', 11, 11),  # 10%
    ('MAP_SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM', 'land_mons', 3, 'SPHEAL', 'SANDSHREW_ALOLA', 28, 28),  # 10%
    ('MAP_ROUTE115', 'land_mons', 2, 'SWABLU', 'RATTATA_ALOLA', 25, 25),  # 10%
    ('MAP_ROUTE115', 'land_mons', 4, 'FLAMIGO', 'MEOWTH_ALOLA', 26, 26),  # 10%
    ('MAP_ROUTE115', 'land_mons', 7, 'JIGGLYPUFF', 'MEOWTH_GALAR', 25, 25),  # 5%
    ('MAP_ROUTE115', 'land_mons', 9, 'CLOBBOPUS', 'THROH', 24, 24),  # 4%
    ('MAP_JAGGED_PASS', 'land_mons', 5, 'MACHOP', 'SAWK', 20, 20),  # 10%
    ('MAP_ROUTE104', 'fishing_mons', 7, 'MAGIKARP', 'KRABBY', 20, 25),  # 15%
    ('MAP_ROUTE104', 'fishing_mons', 8, 'MAGIKARP', 'SHELLDER', 35, 40),  # 4%
    ('MAP_ROUTE104', 'fishing_mons', 3, 'MAGIKARP', 'WISHIWASHI', 10, 30),  # 20%
    ('MAP_PETALBURG_CITY', 'water_mons', 2, 'MARILL', 'STUNFISK', 30, 35),  # 5%
    ('MAP_PETALBURG_CITY', 'water_mons', 3, 'MARILL', 'WOOPER_PALDEA', 5, 10),  # 4%
    ('MAP_ROUTE102', 'water_mons', 2, 'MARILL', 'PANPOUR', 30, 35),  # 5%
    ('MAP_ROUTE110', 'land_mons', 9, 'WATTREL', 'PACHIRISU', 13, 13),  # 4%
    ('MAP_ROUTE112', 'land_mons', 4, 'NUMEL', 'EKANS', 14, 14),  # 10%
    ('MAP_NEW_MAUVILLE_ENTRANCE', 'land_mons', 2, 'VOLTORB', 'ELEKID', 25, 25),  # 10%
    ('MAP_NEW_MAUVILLE_ENTRANCE', 'land_mons', 3, 'MAGNEMITE', 'DEDENNE', 25, 25),  # 10%
    ('MAP_NEW_MAUVILLE_ENTRANCE', 'land_mons', 5, 'ROLYCOLY', 'VOLTORB_HISUI', 23, 23),  # 10%
    ('MAP_NEW_MAUVILLE_INSIDE', 'land_mons', 5, 'JOLTIK', 'TOGEDEMARU', 23, 23),  # 10%
    ('MAP_ROUTE110', 'fishing_mons', 7, 'WAILMER', 'FINNEON', 20, 25),  # 15%
    ('MAP_ROUTE110', 'fishing_mons', 8, 'WAILMER', 'PYUKUMUKU', 35, 40),  # 4%
    ('MAP_ROUTE112', 'land_mons', 6, 'RUFFLET', 'LITLEO', 16, 16),  # 5%
    ('MAP_ROUTE112', 'land_mons', 7, 'MARILL', 'PANSEAR', 16, 16),  # 5%
    ('MAP_ROUTE112', 'land_mons', 1, 'NUMEL', 'MANKEY', 15, 15),  # 20%
    ('MAP_ROUTE112', 'land_mons', 0, 'NUMEL', 'CUBONE', 15, 15),  # 20%
    ('MAP_JAGGED_PASS', 'land_mons', 1, 'NUMEL', 'GROWLITHE_HISUI', 21, 21),  # 20%
    ('MAP_ROUTE114', 'rock_smash_mons', 2, 'GEODUDE', 'KLAWF', 15, 20),  # 5%
    ('MAP_ROUTE121', 'land_mons', 3, 'SHUPPET', 'STUFFUL', 28, 28),  # 10%
    ('MAP_ROUTE113', 'land_mons', 5, 'SLUGMA', 'BOMBIRDIER', 14, 14),  # 10%
    ('MAP_ROUTE113', 'land_mons', 6, 'VULLABY', 'MASCHIFF', 16, 16),  # 5%
    ('MAP_ROUTE113', 'land_mons', 7, 'SLUGMA', 'FLITTLE', 16, 16),  # 5%
    ('MAP_ROUTE113', 'land_mons', 9, 'SKARMORY', 'STUNKY', 16, 16),  # 4%
    ('MAP_ROUTE113', 'land_mons', 0, 'SPINDA', 'GLAMEOW', 15, 15),  # 20%
    ('MAP_ROUTE114', 'land_mons', 4, 'LOTAD', 'HAWLUCHA', 15, 15),  # 10%
    ('MAP_GRANITE_CAVE_B2F', 'land_mons', 4, 'ARON', 'KLEFKI', 12, 12),  # 10%
    ('MAP_ROUTE111', 'rock_smash_mons', 3, 'GEODUDE', 'ORTHWORM', 15, 20),  # 4%
    ('MAP_ROUTE111', 'rock_smash_mons', 1, 'GEODUDE', 'RELLOR', 5, 10),  # 30%
    ('MAP_SANDBURIED_RUINS', 'land_mons', 4, 'SCRAGGY', 'YAMASK_GALAR', 32, 32),  # 10%
    ('MAP_SKY_PILLAR_1F', 'land_mons', 2, 'GOLBAT', 'UNOWN', 35, 35),  # 10%
    ('MAP_SANDBURIED_RUINS', 'land_mons', 5, 'MUDBRAY', 'MARACTUS', 32, 32),  # 10%
    ('MAP_ROUTE114', 'rock_smash_mons', 3, 'GEODUDE', 'BONSLY', 15, 20),  # 4%
    ('MAP_ROUTE114', 'water_mons', 2, 'MARILL', 'STUNFISK_GALAR', 30, 35),  # 5%
    ('MAP_FIERY_PATH', 'land_mons', 7, 'DARUMAKA', 'MAGBY', 16, 16),  # 5%
    ('MAP_FIERY_PATH', 'land_mons', 8, 'SALANDIT', 'HEATMOR', 16, 16),  # 4%
    ('MAP_FIERY_PATH', 'land_mons', 9, 'TORKOAL', 'TURTONATOR', 16, 16),  # 4%
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 2, 'ZUBAT', 'BRONZOR', 18, 18),  # 10%
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 1, 'ZUBAT', 'MINIOR', 17, 17),  # 20%
    ('MAP_METEOR_FALLS_1F_1R', 'land_mons', 0, 'ZUBAT', 'SNEASEL_HISUI', 16, 16),  # 20%
    ('MAP_ROUTE116', 'land_mons', 1, 'WHISMUR', 'NIDORAN_F', 6, 6),  # 20%
    ('MAP_ROUTE102', 'land_mons', 0, 'POOCHYENA', 'BUNEARY', 3, 3),  # 20%
    ('MAP_SAFARI_ZONE_SOUTH', 'land_mons', 3, 'GIRAFARIG', 'NIDORAN_M', 27, 27),  # 10%
    ('MAP_ROUTE118', 'land_mons', 9, 'WINGULL', 'CHATOT', 26, 26),  # 4%
    ('MAP_ROUTE118', 'land_mons', 8, 'WINGULL', 'SQUAWKABILLY', 26, 26),  # 4%
    ('MAP_ROUTE119', 'land_mons', 0, 'ZIGZAGOON', 'KOMALA', 25, 25),  # 20%
    ('MAP_ROUTE121', 'land_mons', 4, 'MIGHTYENA', 'PASSIMIAN', 28, 28),  # 10%
    ('MAP_ROUTE121', 'land_mons', 8, 'IMPIDIMP', 'ORANGURU', 26, 26),  # 4%
    ('MAP_ROUTE121', 'land_mons', 1, 'SHUPPET', 'ESPURR', 26, 26),  # 20%
    ('MAP_ROUTE123', 'land_mons', 3, 'SHUPPET', 'GOOMY', 28, 28),  # 10%
    ('MAP_ROUTE123', 'land_mons', 6, 'PETILIL', 'MORELULL', 28, 28),  # 5%
    ('MAP_ROUTE123', 'land_mons', 9, 'FOMANTIS', 'COMFEY', 27, 27),  # 4%
    ('MAP_ROUTE119', 'fishing_mons', 7, 'CARVANHA', 'BRUXISH', 20, 25),  # 15%
    ('MAP_ROUTE119', 'fishing_mons', 4, 'CARVANHA', 'SLOWPOKE', 10, 30),  # 20%
    ('MAP_ROUTE120', 'water_mons', 2, 'MARILL', 'SLOWPOKE_GALAR', 30, 35),  # 5%
    ('MAP_MAGMA_HIDEOUT_1F', 'land_mons', 2, 'GEODUDE', 'PANSAGE', 28, 28),  # 10%
    ('MAP_MT_PYRE_2F', 'land_mons', 4, 'SHUPPET', 'GASTLY', 29, 29),  # 10%
    ('MAP_MT_PYRE_3F', 'land_mons', 4, 'SHUPPET', 'MISDREAVUS', 29, 29),  # 10%
    ('MAP_MT_PYRE_4F', 'land_mons', 2, 'SHUPPET', 'YAMASK', 26, 26),  # 10%
    ('MAP_MT_PYRE_5F', 'land_mons', 2, 'SHUPPET', 'GOLETT', 26, 26),  # 10%
    ('MAP_MT_PYRE_6F', 'land_mons', 2, 'SHUPPET', 'ZORUA_HISUI', 26, 26),  # 10%
    ('MAP_MT_PYRE_1F', 'land_mons', 5, 'SHUPPET', 'POLTCHAGEIST', 24, 24),  # 10%
    ('MAP_MT_PYRE_2F', 'land_mons', 5, 'SHUPPET', 'POLTCHAGEIST_ARTISAN', 24, 24),  # 10%
    ('MAP_MT_PYRE_SUMMIT', 'land_mons', 2, 'SHUPPET', 'SPIRITOMB', 27, 27),  # 10%
    ('MAP_MT_PYRE_EXTERIOR', 'land_mons', 2, 'SHUPPET', 'SPRITZEE', 28, 28),  # 10%
    ('MAP_MT_PYRE_EXTERIOR', 'land_mons', 6, 'VULPIX', 'SWIRLIX', 29, 29),  # 5%
    ('MAP_SAFARI_ZONE_SOUTH', 'land_mons', 9, 'WOBBUFFET', 'KANGASKHAN', 27, 27),  # 4%
    ('MAP_SAFARI_ZONE_SOUTH', 'land_mons', 1, 'ODDISH', 'TAUROS', 27, 27),  # 20%
    ('MAP_SAFARI_ZONE_SOUTHEAST', 'land_mons', 2, 'SUNKERN', 'CHANSEY', 35, 35),  # 10%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'land_mons', 3, 'GIRAFARIG', 'SCYTHER', 27, 27),  # 10%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'land_mons', 9, 'WOBBUFFET', 'LICKITUNG', 27, 27),  # 4%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'land_mons', 1, 'ODDISH', 'EXEGGCUTE', 27, 27),  # 20%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'land_mons', 7, 'WOBBUFFET', 'TANGELA', 27, 27),  # 5%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'land_mons', 8, 'PIKACHU', 'FARFETCHD', 25, 25),  # 4%
    ('MAP_SAFARI_ZONE_NORTH', 'land_mons', 2, 'PHANPY', 'TAUROS_PALDEA_COMBAT', 29, 29),  # 10%
    ('MAP_SAFARI_ZONE_NORTH', 'land_mons', 3, 'ODDISH', 'TAUROS_PALDEA_BLAZE', 29, 29),  # 10%
    ('MAP_SAFARI_ZONE_NORTH', 'land_mons', 6, 'GLOOM', 'TAUROS_PALDEA_AQUA', 31, 31),  # 5%
    ('MAP_SAFARI_ZONE_NORTH', 'land_mons', 7, 'NATU', 'PONYTA_GALAR', 29, 29),  # 5%
    ('MAP_SAFARI_ZONE_NORTH', 'land_mons', 8, 'XATU', 'FARFETCHD_GALAR', 29, 29),  # 4%
    ('MAP_SAFARI_ZONE_NORTH', 'land_mons', 9, 'HERACROSS', 'MIME_JR', 27, 27),  # 4%
    ('MAP_SAFARI_ZONE_NORTHWEST', 'land_mons', 2, 'RHYHORN', 'BELLSPROUT', 29, 29),  # 10%
    ('MAP_SAFARI_ZONE_NORTHWEST', 'land_mons', 3, 'ODDISH', 'YANMA', 29, 29),  # 10%
    ('MAP_SAFARI_ZONE_NORTHWEST', 'land_mons', 6, 'GLOOM', 'MUNCHLAX', 31, 31),  # 5%
    ('MAP_SAFARI_ZONE_NORTHWEST', 'land_mons', 7, 'DODUO', 'TYROGUE', 29, 29),  # 5%
    ('MAP_SAFARI_ZONE_NORTHWEST', 'land_mons', 8, 'DODRIO', 'HAPPINY', 29, 29),  # 4%
    ('MAP_SAFARI_ZONE_NORTHWEST', 'land_mons', 9, 'PINSIR', 'CHERUBI', 27, 27),  # 4%
    ('MAP_ROUTE117', 'land_mons', 0, 'POOCHYENA', 'FLABEBE', 13, 13),  # 20%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'water_mons', 2, 'PSYDUCK', 'LAPRAS', 30, 35),  # 5%
    ('MAP_SAFARI_ZONE_NORTHWEST', 'water_mons', 2, 'PSYDUCK', 'MANTYKE', 30, 35),  # 5%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'fishing_mons', 8, 'SEAKING', 'QWILFISH', 35, 40),  # 4%
    ('MAP_SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM', 'land_mons', 5, 'SPHEAL', 'CUBCHOO', 30, 30),  # 10%
    ('MAP_SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM', 'land_mons', 3, 'SPHEAL', 'BERGMITE', 28, 28),  # 10%
    ('MAP_SHOAL_CAVE_LOW_TIDE_LOWER_ROOM', 'land_mons', 3, 'SPHEAL', 'CRYOGONAL', 28, 28),  # 10%
    ('MAP_SHOAL_CAVE_LOW_TIDE_INNER_ROOM', 'land_mons', 3, 'SPHEAL', 'DELIBIRD', 28, 28),  # 10%
    ('MAP_SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM', 'land_mons', 5, 'SPHEAL', 'SMOOCHUM', 30, 30),  # 10%
    ('MAP_SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM', 'land_mons', 2, 'ZUBAT', 'DARUMAKA_GALAR', 28, 28),  # 10%
    ('MAP_ROUTE126', 'fishing_mons', 8, 'WAILMER', 'ALOMOMOLA', 35, 40),  # 4%
    ('MAP_ROUTE129', 'fishing_mons', 7, 'WAILMER', 'QWILFISH_HISUI', 25, 30),  # 15%
    ('MAP_ROUTE128', 'fishing_mons', 8, 'WAILMER', 'INKAY', 35, 40),  # 4%
    ('MAP_ROUTE127', 'water_mons', 2, 'WINGULL', 'BINACLE', 15, 25),  # 5%
    ('MAP_ROUTE121', 'water_mons', 2, 'WINGULL', 'WIMPOD', 15, 25),  # 5%
    ('MAP_ABANDONED_SHIP_ROOMS_B1F', 'fishing_mons', 8, 'TENTACRUEL', 'DHELMISE', 25, 30),  # 4%
    ('MAP_METEOR_FALLS_B1F_2R', 'land_mons', 4, 'BAGON', 'DURALUDON', 35, 35),  # 10%
    ('MAP_METEOR_FALLS_1F_2R', 'land_mons', 2, 'GOLBAT', 'DRAMPA', 33, 33),  # 10%
    ('MAP_VICTORY_ROAD_B1F', 'land_mons', 4, 'GOLBAT', 'MIENFOO', 38, 38),  # 10%
    ('MAP_MAGMA_HIDEOUT_1F', 'land_mons', 4, 'GEODUDE', 'TRUBBISH', 29, 29),  # 10%
    ('MAP_ROUTE120', 'land_mons', 0, 'POOCHYENA', 'CARNIVINE', 25, 25),  # 20%
    ('MAP_SAFARI_ZONE_NORTHEAST', 'land_mons', 2, 'AIPOM', 'FURFROU', 35, 35),  # 10%
    ('MAP_ALTERING_CAVE', 'land_mons', 2, 'ZUBAT', 'KARRABLAST', 8, 8),  # 10%
    ('MAP_ALTERING_CAVE', 'land_mons', 3, 'ZUBAT', 'SHELMET', 14, 14),  # 10%
    ('MAP_SAFARI_ZONE_SOUTHEAST', 'land_mons', 3, 'MAREEP', 'ORICORIO', 36, 36),  # 10%
    ('MAP_SAFARI_ZONE_SOUTHWEST', 'fishing_mons', 3, 'GOLDEEN', 'REMORAID', 10, 25),  # 20%
]


def apply(check):
    d = json.load(open(JSON))
    tables = {}
    for e in d['wild_encounter_groups'][0]['encounters']:
        if e.get('map') and 'FireRed' not in e.get('base_label', '') and 'LeafGreen' not in e.get('base_label', ''):
            tables.setdefault(e['map'], []).append(e)
    changed, problems = 0, []
    for (m, kind, slot, old, new, lo, hi) in PLACEMENTS:
        ent = [e for e in tables.get(m, []) if kind in e]
        if not ent:
            problems.append('%s has no %s' % (m, kind))
            continue
        mons = ent[0][kind]['mons']
        cur = mons[slot]['species'].replace('SPECIES_', '')
        if cur == new:
            continue
        if cur != old:
            problems.append('%s %s slot %d holds %s, not %s' % (m, kind, slot, cur, old))
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
