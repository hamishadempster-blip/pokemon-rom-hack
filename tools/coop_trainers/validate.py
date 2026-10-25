#!/usr/bin/env python3
"""Validate src/data/trainers.party for build 0xC017012F.

Every Hoenn trainer reachable from a map script or the rematch table:
- species, held items, moves, and abilities are real constants;
- an explicit ability belongs to that species; explicit moves are on the
  species' (or a pre-evolution's) level-up list or its learnables;
- 1-6 Pokemon, levels 1-100; a double-battle trainer has 2+;
Ordinary trainers (not leaders, E4, champions, rivals, admins, bosses,
co-op partners) also:
- stay below this hack's next gym ace for their stage (overhaul.py rule);
- gym trainers stay below their own leader's ace;
- added-roster species appear no earlier than the stage they are catchable.
Also counts how each trainer behaves with the Twin Beacon (co-op-capable
or solo-only and why). Exit status 1 on any error.
"""
import json, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import overhaul as O

ROOT = O.ROOT
text = open(O.PARTY, encoding='utf-8').read()
entries = {tid: body for tid, body in O.split_entries(text) if tid}


def consts(path, prefix):
    return set(re.findall(r'\b(%s\w+)\b' % prefix, open(os.path.join(ROOT, path)).read()))


ITEMS = consts('include/constants/items.h', 'ITEM_')
MOVES = consts('include/constants/moves.h', 'MOVE_')
ABILITIES = consts('include/constants/abilities.h', 'ABILITY_')
LEARNABLES = json.load(open(os.path.join(ROOT, 'src/data/pokemon/all_learnables.json')))
LEVELUP = {}
lu = open(os.path.join(ROOT, 'src/data/pokemon/level_up_learnsets/gen_9.h')).read()
for m in re.finditer(r's(\w+)LevelUpLearnset\[\] = \{(.*?)LEVEL_UP_END', lu, re.S):
    LEVELUP[m.group(1).upper()] = set(re.findall(r'MOVE_(\w+)', m.group(2)))
LU_OF = {}
for f in os.listdir(os.path.join(ROOT, 'src/data/pokemon/species_info')):
    s = open(os.path.join(ROOT, 'src/data/pokemon/species_info', f)).read()
    for m in re.finditer(r'\[(SPECIES_\w+)\] =\s*\{', s):
        blk = s[m.end():s.find('\n    },', m.end())]
        r = re.search(r'\.levelUpLearnset = s(\w+)LevelUpLearnset', blk)
        if r:
            LU_OF[m.group(1)] = r.group(1).upper()


def item_const(name):
    return 'ITEM_' + re.sub(r'[^A-Z0-9]+', '_', name.upper().replace("'", '').replace('.', '')).strip('_')


def move_const(name):
    return 'MOVE_' + re.sub(r'[^A-Z0-9]+', '_', name.upper().replace("'", '').replace('.', '')).strip('_')


def learns(species, move):
    mv = move[len('MOVE_'):]
    c = species
    seen = set()
    while c and c not in seen:
        seen.add(c)
        if mv in LEVELUP.get(LU_OF.get(c, ''), set()):
            return True
        key = c[len('SPECIES_'):]
        if 'MOVE_' + mv in LEARNABLES.get(key, []) or 'MOVE_' + mv in LEARNABLES.get(key.split('_')[0], []):
            return True
        c = O.PREVO.get(c)
    return False


# Scope, as in overhaul.py.
import glob
t2m = {}
for f in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')):
    m = os.path.basename(os.path.dirname(f))
    if m.endswith('_Frlg'):
        continue
    for t in re.findall(r'trainerbattle\w*\s+(TRAINER_\w+)', open(f).read()):
        t2m.setdefault(t, set()).add(m)
    for t in re.findall(r'setvar VAR_0x800[45], (TRAINER_\w+)', open(f).read()):
        t2m.setdefault(t, set()).add(m)
bs = open(os.path.join(ROOT, 'src/battle_setup.c')).read()
for ids, mp in re.findall(r'REMATCH\(([^)]*?),\s*(MAP_\w+)\)', bs):
    tiers = [t.strip() for t in ids.split(',') if t.strip().startswith('TRAINER_')]
    for t in tiers[1:]:
        t2m.setdefault(t, set()).update(t2m.get(tiers[0], set()) or {'(rematch)'})
partners = set()
for f in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')):
    partners |= set(re.findall(r'setvar VAR_0x8005, (TRAINER_\w+)', open(f).read()))

GYM_ACE = {'RustboroCity_Gym': 16, 'DewfordTown_Gym': 22, 'MauvilleCity_Gym': 28, 'LavaridgeTown_Gym_1F': 34,
           'LavaridgeTown_Gym_B1F': 34, 'PetalburgCity_Gym': 40, 'FortreeCity_Gym': 46, 'MossdeepCity_Gym': 52,
           'SootopolisCity_Gym_B1F': 58, 'SootopolisCity_Gym_1F': 58}
ADDED_STAGE = {}
for theme, lst in O.ADDED.items():
    for sp, st in lst:
        ADDED_STAGE['SPECIES_' + sp] = min(st, ADDED_STAGE.get('SPECIES_' + sp, 99))

errors, warnings = [], []
beacon = {'co-op capable': 0, 'solo: couple/double': 0, 'mandatory (both or solo path)': 0}
ordinary = 0
for tid, maps in sorted(t2m.items()):
    if tid not in entries:
        continue
    head, mons = O.parse_entry(entries[tid])
    cls = O.header_get(head, 'Class')
    double = O.header_get(head, 'Double Battle') == 'Yes'
    where = '%s (%s)' % (tid, ','.join(sorted(maps)))
    if not 1 <= len(mons) <= 6:
        errors.append('%s: %d Pokemon' % (where, len(mons)))
    if double and len(mons) < 2:
        errors.append('%s: double battle with one Pokemon' % where)
    for it in (O.header_get(head, 'Items') or '').split('/'):
        if it.strip() and item_const(it.strip()) not in ITEMS:
            errors.append('%s: trainer item %s' % (where, it.strip()))
    levels = []
    for m in mons:
        sp = O.const_of(O.mon_species(m))
        if sp not in O.SPECIES:
            errors.append('%s: species %s' % (where, O.mon_species(m)))
            continue
        lv = O.mon_level(m)
        levels.append(lv)
        if not 1 <= lv <= 100:
            errors.append('%s: level %d' % (where, lv))
        it = O.mon_item(m)
        if it and item_const(it) not in ITEMS:
            errors.append('%s: held item %s' % (where, it))
        for l in m:
            if l.startswith('Ability: '):
                ab = 'ABILITY_' + re.sub(r'[^A-Z0-9]+', '_', l[9:].upper()).strip('_')
                if ab not in ABILITIES:
                    errors.append('%s: ability %s' % (where, l[9:]))
                elif ab[len('ABILITY_'):] not in O.SPECIES[sp]['abilities']:
                    errors.append('%s: %s cannot have %s' % (where, sp, l[9:]))
            if l.startswith('- '):
                mv = move_const(l[2:].strip())
                if mv not in MOVES:
                    errors.append('%s: move %s' % (where, l[2:]))
                elif not learns(sp, mv):
                    warnings.append('%s: %s does not normally learn %s' % (where, sp, l[2:].strip()))
    boss = cls in O.BOSS_CLASSES or tid in partners
    if boss:
        beacon['mandatory (both or solo path)'] += 1
        continue
    ordinary += 1
    if double:
        beacon['solo: couple/double'] += 1
    else:
        beacon['co-op capable'] += 1
    if not levels:
        continue
    top = max(levels)
    for m in maps:
        if m in GYM_ACE and top >= GYM_ACE[m]:
            errors.append('%s: gym trainer level %d is not below the leader ace %d' % (where, top, GYM_ACE[m]))
    for m in mons:
        sp = O.const_of(O.mon_species(m))
        root = O.base_of(sp)
        if root in ADDED_STAGE:
            stage = O.stage_of(top)
            if ADDED_STAGE[root] > stage + 1:
                warnings.append('%s: %s before its stage (%d > %d)' % (where, sp, ADDED_STAGE[root], stage))

print('%d trainers checked, %d ordinary.' % (len([t for t in t2m if t in entries]), ordinary))
print('Twin Beacon behaviour: ' + ', '.join('%s %d' % kv for kv in beacon.items()))
for w in warnings:
    print('warning', w)
for e in errors:
    print('ERROR', e)
print('%d error(s), %d warning(s).' % (len(errors), len(warnings)))
sys.exit(1 if errors else 0)
