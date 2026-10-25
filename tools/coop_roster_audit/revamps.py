#!/usr/bin/env python3
"""Revamps for build 0xC017012F: stats, types, abilities, level-up moves.

Writes the species_info and gen_9 level-up learnset sources. Idempotent:
each species block is rewritten to the values below. Stats are HP, Atk,
Def, SpA, SpD, Spe. Learnset entries are added (or moved) by name; a level
of None removes the move.
"""
import os, re
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))

REVAMPS = {
    'MIGHTYENA': dict(stats=(80, 110, 75, 50, 65, 90), abilities=('INTIMIDATE', 'STRONG_JAW', 'MOXIE'),
                      moves={'THROAT_CHOP': 32, 'PSYCHIC_FANGS': 40, 'PLAY_ROUGH': 46, 'PARTING_SHOT': 56}),
    'MASQUERAIN': dict(stats=(75, 40, 72, 105, 92, 90), types=('BUG', 'WATER'),
                       moves={'STICKY_WEB': 1, 'BUBBLE_BEAM': 24, 'SCALD': 29, 'TAILWIND': 34, 'HYDRO_PUMP': 48}),
    'BANETTE': dict(stats=(74, 125, 75, 60, 73, 80), abilities=('INSOMNIA', 'FRISK', 'PRANKSTER'),
                    moves={'TAUNT': 24, 'SHADOW_CLAW': 28, 'POLTERGEIST': 38, 'PLAY_ROUGH': 44, 'SWORDS_DANCE': 50}),
    'CACTURNE': dict(stats=(80, 115, 75, 110, 75, 55), abilities=('WATER_ABSORB', 'NONE', 'SAND_VEIL'),
                     moves={'DRAIN_PUNCH': 28, 'SEED_BOMB': 32, 'DARK_PULSE': 41, 'LEAF_STORM': 47}),
    'FLYGON': dict(stats=(80, 100, 85, 100, 80, 100),
                   moves={'DRAGON_PULSE': 36, 'SCORCHING_SANDS': 42, 'DRACO_METEOR': 56}),
    'SWALOT': dict(stats=(115, 70, 95, 73, 100, 45),
                   moves={'CLEAR_SMOG': 22, 'KNOCK_OFF': 33, 'PAIN_SPLIT': 45}),
    'DELIBIRD': dict(stats=(65, 75, 55, 75, 65, 110), abilities=('VITAL_SPIRIT', 'HUSTLE', 'SNOW_WARNING'),
                     moves={'FAKE_OUT': 1, 'POWDER_SNOW': 1, 'PECK': 1, 'ICE_SHARD': 8, 'ICY_WIND': 13,
                            'HELPING_HAND': 18, 'TAILWIND': 30, 'FREEZE_DRY': 35, 'AURORA_VEIL': 40,
                            'BRAVE_BIRD': 45, 'SPIKES': 50}),
    'BEAUTIFLY': dict(stats=(65, 50, 60, 115, 65, 100), abilities=('SWARM', 'TINTED_LENS', 'RIVALRY'),
                      moves={'PROTECT': None, 'PSYCHIC': 37, 'HURRICANE': 44}),
    'DUSTOX': dict(stats=(80, 40, 90, 60, 105, 55), abilities=('SHIELD_DUST', 'COMPOUND_EYES', 'HEALER'),
                   moves={'QUIVER_DANCE': None, 'RAGE_POWDER': 24, 'TOXIC_SPIKES': 28, 'SLUDGE_BOMB': 40,
                          'SAFEGUARD': 45}),
    'ARMALDO': dict(stats=(85, 130, 110, 45, 90, 50),
                    moves={'RAPID_SPIN': 1, 'KNOCK_OFF': 30, 'STONE_EDGE': 40, 'SWORDS_DANCE': 52}),
    'LUXRAY': dict(stats=(80, 125, 79, 75, 79, 88), types=('ELECTRIC', 'DARK'),
                   moves={'FIRE_FANG': 1, 'ICE_FANG': 1, 'THUNDER_FANG': 1, 'CRUNCH': 42, 'WILD_CHARGE': 50,
                          'PSYCHIC_FANGS': 58, 'SUPERCELL_SLAM': 68, 'SWAGGER': None}),
    # Build 0xC0170131.
    'LEDIAN': dict(stats=(70, 105, 65, 35, 95, 95), abilities=('IRON_FIST', 'EARLY_BIRD', 'SWARM'),
                   moves={'SWIFT': None, 'STRUGGLE_BUG': None, 'BUG_BUZZ': None, 'AIR_SLASH': None,
                          'COMET_PUNCH': 1, 'LUNGE': 24, 'ICE_PUNCH': 26, 'THUNDER_PUNCH': 26, 'FIRE_PUNCH': 30,
                          'DRAIN_PUNCH': 36, 'ACROBATICS': 42, 'SWORDS_DANCE': 46}),
    'LEDYBA': dict(stats=(40, 20, 30, 40, 80, 55), moves={'COMET_PUNCH': 1}),
    'ARIADOS': dict(stats=(90, 100, 85, 50, 85, 40), abilities=('SWARM', 'INSOMNIA', 'STAKEOUT'),
                    moves={'PSYCHIC': None, 'SPIDER_WEB': 1, 'TOXIC_THREAD': 30, 'STICKY_WEB': 34,
                           'KNOCK_OFF': 38, 'POISON_JAB': 41, 'LEECH_LIFE': 44}),
    'SUNFLORA': dict(stats=(85, 50, 70, 125, 90, 30), types=('GRASS', 'FIRE'),
                     abilities=('CHLOROPHYLL', 'SOLAR_POWER', 'DROUGHT'),
                     moves={'POUND': None, 'BULLET_SEED': None, 'DOUBLE_EDGE': None,
                            'EMBER': 0, 'WEATHER_BALL': 16, 'SYNTHESIS': 24, 'FLAMETHROWER': 28,
                            'HEAT_WAVE': 37, 'EARTH_POWER': 46}),
    'MAGCARGO': dict(stats=(85, 45, 120, 85, 105, 30), abilities=('SOLID_ROCK', 'FLAME_BODY', 'MAGMA_ARMOR'),
                     moves={'STEALTH_ROCK': 38, 'RECOVER': 40, 'WILL_O_WISP': 42, 'BODY_PRESS': 50}),
    'SLUGMA': dict(stats=(40, 40, 40, 70, 40, 20), moves={'WILL_O_WISP': 30}),
    'DEWGONG': dict(stats=(100, 60, 90, 80, 105, 60),
                    moves={'SHEER_COLD': None, 'LIFE_DEW': 0, 'FAKE_OUT': 1, 'SCALD': 38, 'HAZE': 42,
                           'ICE_BEAM': 48}),
    'NOCTOWL': dict(stats=(110, 40, 60, 95, 110, 70),
                    moves={'SKY_ATTACK': None, 'TAKE_DOWN': None, 'UPROAR': None,
                           'LIGHT_SCREEN': 20, 'ROOST': 25, 'HYPNOSIS': 30, 'WHIRLWIND': 33,
                           'PSYSHOCK': 36, 'HURRICANE': 50}),
}

STAT_KEYS = ('baseHP', 'baseAttack', 'baseDefense', 'baseSpAttack', 'baseSpDefense', 'baseSpeed')


def species_file(name):
    for f in sorted(os.listdir(os.path.join(ROOT, 'src/data/pokemon/species_info'))):
        path = os.path.join(ROOT, 'src/data/pokemon/species_info', f)
        if '[SPECIES_%s] =' % name in open(path).read():
            return path


def edit_species(name, spec):
    path = species_file(name)
    s = open(path).read()
    i = s.index('[SPECIES_%s] =' % name)
    j = s.index('\n    },', i)
    blk = s[i:j]
    for key, val in zip(STAT_KEYS, spec['stats']):
        blk = re.sub(r'(\.%s\s*=\s*)[^,\n]+(,)' % key, r'\g<1>%d\2' % val, blk, count=1)
    if 'types' in spec:
        blk = re.sub(r'\.types = MON_TYPES\([^)]*\)', '.types = MON_TYPES(%s)' % ', '.join('TYPE_' + t for t in spec['types']), blk, count=1)
    if 'abilities' in spec:
        blk = re.sub(r'\.abilities = \{[^}]*\}', '.abilities = { %s }' % ', '.join('ABILITY_' + a for a in spec['abilities']), blk, count=1)
    open(path, 'w').write(s[:i] + blk + s[j:])


def edit_learnset(name, moves):
    path = os.path.join(ROOT, 'src/data/pokemon/level_up_learnsets/gen_9.h')
    s = open(path).read()
    var = 's%sLevelUpLearnset[] = {' % name.title()
    i = s.index(var)
    j = s.index('LEVEL_UP_END', i)
    entries = re.findall(r'LEVEL_UP_MOVE\(\s*(\d+),\s*MOVE_(\w+)\)', s[i:j])
    entries = [(int(l), m) for l, m in entries if m not in moves]
    entries += [(l, m) for m, l in moves.items() if l is not None]
    entries.sort(key=lambda e: e[0])
    body = ''.join('    LEVEL_UP_MOVE(%2d, MOVE_%s),\n' % e for e in entries)
    k = s.index('\n', i) + 1
    open(path, 'w').write(s[:k] + body + '    ' + s[j:])


for name, spec in REVAMPS.items():
    edit_species(name, spec)
    edit_learnset(name, spec.get('moves', {}))
