#!/usr/bin/env python3
"""Ordinary trainer overhaul for build 0xC017012F.

Rebuilds every ordinary Hoenn trainer (route, cave, building, gym trainer,
Aqua/Magma grunt, and their rematch tiers) from the parties at BASE_COMMIT,
so rerunning gives the same file. Gym leaders, rivals, the Elite Four,
champions, Aqua/Magma admins and leaders, co-op partners, Soren and Mira,
FRLG trainers, and facility trainers are left exactly as they are now.

Rules, all deterministic (seeded by the trainer's base name):
- Stage: from the trainer's original highest level, against the stock gym
  ace levels (15, 19, 24, 29, 31, 33, 42, 46). Stage picks which species
  are available and how much the levels rise.
- Levels: + 0, 1, 2, 2, 4, 5, 4, 5, 5 by stage, never above this hack's ace
  for that stage minus 2 (16, 22, 28, 34, 40, 46, 52, 58, 64), never lower
  than before.
- Theme: by trainer class (gym trainers by their gym's type). One non-ace
  slot of most teams becomes a Pokemon from this hack's added roster that
  fits the theme and is catchable by that stage.
- Party size: unchanged through stage 1; a lone Pokemon gets a partner
  from stage 2; some pairs get a third from stage 4. Double-battle pairs,
  and Calvin (the first trainer, kept as the one-Pokemon co-op case), stay.
- Evolution: a Pokemon at least 4 levels past its level evolution evolves;
  stone, friendship, and trade evolutions from level 32 (36 for trade).
- Moves: kept for evolved Pokemon, dropped (engine picks by level) for new
  ones. Ability lines are dropped if the new species cannot have them.
- Held items: about one trainer in four from stage 2 gives the ace a berry
  (Oran, Sitrus from stage 4); about one in six from stage 5 a type item.
- AI: Check Bad Move / Try To Faint, plus Check Viability from stage 2 and
  HP Aware from stage 5. Never Omniscient.
"""
import glob, hashlib, os, re, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
PARTY = os.path.join(ROOT, 'src/data/trainers.party')
BASE_COMMIT = '029b55fe'

STOCK_ACE = [15, 19, 24, 29, 31, 33, 42, 46, 99]
HACK_ACE = [16, 22, 28, 34, 40, 46, 52, 58, 64]
DELTA = [0, 1, 2, 2, 4, 5, 4, 5, 5]

BOSS_CLASSES = {'Leader', 'Elite Four', 'Champion', 'Rival', 'Aqua Admin', 'Aqua Leader',
                'Magma Admin', 'Magma Leader'}
# Calvin (Route 102's first trainer, the one-Pokemon co-op test case) and
# the Route 102 co-op pair keep their teams; only AI changes.
EXEMPT = {'TRAINER_CALVIN_1', 'TRAINER_ALLEN', 'TRAINER_RICK'}
EXEMPT_SIZE = EXEMPT

GYM_THEME = {
    'RustboroCity_Gym': 'ROCK', 'DewfordTown_Gym': 'FIGHTING', 'MauvilleCity_Gym': 'ELECTRIC',
    'LavaridgeTown_Gym_1F': 'FIRE', 'LavaridgeTown_Gym_B1F': 'FIRE', 'PetalburgCity_Gym': 'NORMAL',
    'FortreeCity_Gym': 'FLYING', 'MossdeepCity_Gym': 'PSYCHIC', 'SootopolisCity_Gym_B1F': 'WATER',
    'SootopolisCity_Gym_1F': 'WATER',
}

CLASS_THEME = {
    'Bug Catcher': ['BUG'], 'Bug Maniac': ['BUG'], 'Kindler': ['FIRE'], 'Hiker': ['ROCK'],
    'Ruin Maniac': ['ROCK'], 'Black Belt': ['FIGHTING'], 'Battle Girl': ['FIGHTING'], 'Expert': ['FIGHTING'],
    'Psychic': ['PSYCHIC'], 'Hex Maniac': ['GHOST'], 'Bird Keeper': ['FLYING'], 'Guitarist': ['ELECTRIC'],
    'Aroma Lady': ['GRASS'], 'Swimmer M': ['WATER'], 'Swimmer F': ['WATER'], 'Fisherman': ['WATER'],
    'Tuber M': ['WATER'], 'Tuber F': ['WATER'], 'Parasol Lady': ['WATER'], 'Sailor': ['WATER', 'FIGHTING'],
    'Dragon Tamer': ['DRAGON'], 'Ninja Boy': ['POISON', 'DARK'], 'Team Aqua': ['WATER', 'DARK'],
    'Team Magma': ['FIRE', 'DARK'], 'Pkmn Ranger': ['GRASS', 'BUG'], 'Beauty': ['FAIRY', 'NORMAL'],
    'Lady': ['FAIRY', 'NORMAL'], 'Rich Boy': ['NORMAL'], 'Gentleman': ['NORMAL', 'STEEL'],
    'Pokefan': ['NORMAL', 'FAIRY'], 'Pkmn Breeder': ['NORMAL', 'FAIRY'], 'Youngster': ['NORMAL', 'BUG'],
    'Lass': ['NORMAL', 'FAIRY'], 'School Kid': ['GRASS', 'PSYCHIC'], 'Camper': ['ROCK', 'BUG', 'NORMAL'],
    'Picnicker': ['GRASS', 'NORMAL', 'FAIRY'], 'Triathlete': ['ELECTRIC', 'FLYING', 'WATER'],
    'Cooltrainer': ['MIXED'], 'Cooltrainer 2': ['MIXED'], 'Collector': ['STEEL', 'MIXED'],
    'Pokemaniac': ['DRAGON', 'ROCK'], 'Winstrate': ['MIXED'], 'Young Couple': ['NORMAL', 'FAIRY'],
    'Old Couple': ['NORMAL', 'FAIRY'], 'Twins': ['NORMAL', 'FAIRY'], 'Sis And Bro': ['WATER', 'NORMAL'],
    'Sr And Jr': ['NORMAL', 'FAIRY'], 'Pkmn Trainer 1': ['MIXED'],
}

# This hack's added roster (visitors and the fourteen new families), by the
# stage from which they are catchable. Base forms; evolution follows level.
ADDED = {
    'BUG': [('TAROUNTULA', 0), ('JOLTIK', 2), ('SNOM', 6)],
    'FIRE': [('CHARCADET', 2), ('CAPSAKID', 3), ('LITWICK', 4)],
    'WATER': [('WATTREL', 0), ('ARROKUDA', 4), ('VELUZA', 4), ('BASCULEGION', 4), ('DONDOZO', 4),
              ('FINIZEN', 6), ('TATSUGIRI', 7), ('WIGLETT', 7)],
    'GRASS': [('APPLIN', 0), ('COTTONEE', 1), ('SMOLIV', 1), ('BRAMBLIN', 2), ('CAPSAKID', 3),
              ('TOEDSCOOL', 4), ('FOONGUS', 4), ('PETILIL', 5)],
    'ELECTRIC': [('WATTREL', 0), ('PAWMI', 1), ('SHINX', 1), ('TADBULB', 2), ('JOLTIK', 2), ('ROTOM', 2),
                 ('HELIOPTILE', 3)],
    'ROCK': [('NACLI', 0), ('GLIMMET', 0), ('ROCKRUFF', 2), ('CUFANT', 2), ('SANDILE', 3)],
    'FIGHTING': [('RIOLU', 0), ('PAWMI', 1), ('FLAMIGO', 2), ('CRABRAWLER', 4)],
    'PSYCHIC': [('HATENNA', 0), ('INDEEDEE_M', 4), ('INDEEDEE_F', 4), ('VELUZA', 4)],
    'GHOST': [('BRAMBLIN', 2), ('ROTOM', 2), ('HONEDGE', 4), ('LITWICK', 4), ('GREAVARD', 5),
              ('MIMIKYU', 5), ('DREEPY', 5)],
    'FLYING': [('ROOKIDEE', 0), ('WATTREL', 0), ('NOIBAT', 2), ('FLAMIGO', 2)],
    'DRAGON': [('APPLIN', 0), ('NOIBAT', 2), ('DREEPY', 5), ('CYCLIZAR', 6), ('TATSUGIRI', 7),
               ('JANGMO_O', 8), ('FRIGIBAX', 8)],
    'POISON': [('SHROODLE', 0), ('GLIMMET', 0), ('FOONGUS', 4), ('VAROOM', 6)],
    'DARK': [('NICKIT', 0), ('SHROODLE', 0), ('SANDILE', 3), ('IMPIDIMP', 5), ('PAWNIARD', 8)],
    'NORMAL': [('LECHONK', 0), ('TANDEMAUS', 0), ('WOOLOO', 0), ('SMOLIV', 1), ('FIDOUGH', 6)],
    'FAIRY': [('HATENNA', 0), ('COTTONEE', 1), ('IMPIDIMP', 5), ('FIDOUGH', 6), ('TINKATINK', 8)],
    'STEEL': [('ROOKIDEE', 0), ('CUFANT', 2), ('HONEDGE', 4), ('VAROOM', 6), ('TINKATINK', 8)],
    'ICE': [('SNOM', 6), ('CETODDLE', 6), ('FRIGIBAX', 8)],
}
# Native Hoenn partners for teams that grow, by theme.
NATIVE = {
    'BUG': [('WURMPLE', 0), ('SURSKIT', 0), ('NINCADA', 0), ('VOLBEAT', 1), ('ILLUMISE', 1)],
    'FIRE': [('NUMEL', 2), ('SLUGMA', 2), ('TORKOAL', 2), ('VULPIX', 4)],
    'WATER': [('WINGULL', 0), ('MARILL', 0), ('TENTACOOL', 0), ('CARVANHA', 2), ('WAILMER', 1),
              ('CORPHISH', 3), ('CHINCHOU', 6), ('SPHEAL', 6)],
    'GRASS': [('LOTAD', 0), ('SEEDOT', 0), ('SHROOMISH', 0), ('ODDISH', 1), ('ROSELIA', 1), ('CACNEA', 3),
              ('TROPIUS', 4)],
    'ELECTRIC': [('ELECTRIKE', 1), ('PLUSLE', 1), ('MINUN', 1), ('MAGNEMITE', 2), ('VOLTORB', 2)],
    'ROCK': [('GEODUDE', 0), ('NOSEPASS', 0), ('ARON', 0), ('TRAPINCH', 2), ('BALTOY', 2), ('LUNATONE', 2),
             ('SOLROCK', 2)],
    'FIGHTING': [('MAKUHITA', 0), ('MACHOP', 2), ('MEDITITE', 2)],
    'PSYCHIC': [('RALTS', 0), ('ABRA', 0), ('SPOINK', 2), ('BALTOY', 2), ('CHIMECHO', 5)],
    'GHOST': [('SABLEYE', 0), ('SHUPPET', 4), ('DUSKULL', 4)],
    'FLYING': [('TAILLOW', 0), ('WINGULL', 0), ('SWABLU', 2), ('SKARMORY', 2), ('TROPIUS', 4)],
    'DRAGON': [('SWABLU', 2), ('TRAPINCH', 2), ('BAGON', 7)],
    'POISON': [('ZUBAT', 0), ('GULPIN', 1), ('KOFFING', 2), ('GRIMER', 2), ('SEVIPER', 2)],
    'DARK': [('POOCHYENA', 0), ('SABLEYE', 0), ('CARVANHA', 2), ('ABSOL', 4)],
    'NORMAL': [('ZIGZAGOON', 0), ('SKITTY', 0), ('WHISMUR', 0), ('SLAKOTH', 0), ('SPINDA', 2), ('KECLEON', 3)],
    'FAIRY': [('MARILL', 0), ('RALTS', 0), ('AZURILL', 0), ('SKITTY', 0)],
    'STEEL': [('ARON', 0), ('MAGNEMITE', 2), ('SKARMORY', 2), ('MAWILE', 0)],
    'ICE': [('SNORUNT', 6), ('SPHEAL', 6), ('SNOVER', 6)],
}
TYPE_ITEM = {
    'FIRE': 'Charcoal', 'WATER': 'Mystic Water', 'GRASS': 'Miracle Seed', 'ELECTRIC': 'Magnet',
    'ICE': 'Never-Melt Ice', 'FIGHTING': 'Black Belt', 'POISON': 'Poison Barb', 'GROUND': 'Soft Sand',
    'FLYING': 'Sharp Beak', 'PSYCHIC': 'Twisted Spoon', 'BUG': 'Silver Powder', 'ROCK': 'Hard Stone',
    'GHOST': 'Spell Tag', 'DRAGON': 'Dragon Fang', 'DARK': 'Black Glasses', 'STEEL': 'Metal Coat',
    'NORMAL': 'Silk Scarf', 'FAIRY': 'Fairy Feather',
}


def h(*parts):
    return int(hashlib.sha1('|'.join(map(str, parts)).encode()).hexdigest(), 16)


# --- species data from the sources -----------------------------------------
SPECIES = {}  # const -> dict(name, types, abilities, evos)


def parse_evos(text):
    out = []
    i = text.find('EVOLUTION(')
    if i < 0:
        return out
    j = i + len('EVOLUTION(')
    depth, start = 0, None
    while j < len(text):
        c = text[j]
        if c == '{':
            if depth == 0:
                start = j
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                body = text[start + 1:j]
                m = re.match(r'\s*(EVO_\w+)\s*,\s*([^,]+?)\s*,\s*(SPECIES_\w+)(.*)', body, re.S)
                if m:
                    out.append((m.group(1), m.group(2).strip(), m.group(3), m.group(4)))
        elif c == ')' and depth == 0:
            break
        j += 1
    return out


for f in sorted(glob.glob(os.path.join(ROOT, 'src/data/pokemon/species_info/*.h'))):
    s = open(f).read()
    for m in re.finditer(r'\[(SPECIES_\w+)\] =\s*\{', s):
        const = m.group(1)
        end = s.find('\n    },', m.end())
        blk = s[m.end():end]
        types = re.search(r'\.types = MON_TYPES\(([^)]*)\)', blk)
        abil = re.search(r'\.abilities = \{([^}]*)\}', blk)
        SPECIES[const] = {
            'types': [t.strip().replace('TYPE_', '') for t in types.group(1).split(',')] if types else [],
            'abilities': [a.strip().replace('ABILITY_', '') for a in abil.group(1).split(',')] if abil else [],
            'evos': parse_evos(blk),
        }

ALIASES = {}
for _a, _b in re.findall(r'^\s+(SPECIES_\w+) = (SPECIES_\w+),', open(os.path.join(ROOT, 'include/constants/species.h')).read(), re.M):
    ALIASES[_a] = _b
ALIASES.update({'SPECIES_INDEEDEE': 'SPECIES_INDEEDEE_M', 'SPECIES_BASCULEGION': 'SPECIES_BASCULEGION_M',
           'SPECIES_OINKOLOGNE': 'SPECIES_OINKOLOGNE_M', 'SPECIES_LYCANROC': 'SPECIES_LYCANROC_MIDDAY',
           'SPECIES_AEGISLASH': 'SPECIES_AEGISLASH_SHIELD'})


def const_of(name):
    c = 'SPECIES_' + re.sub(r'[^A-Z0-9]+', '_', name.upper().replace("'", '').replace('.', '')).strip('_')
    c = c.replace('_FEMALE', '_F').replace('_MALE', '_M')
    if c in SPECIES:
        return c
    return ALIASES.get(c, c)


def name_of(const):
    parts = const[len('SPECIES_'):].split('_')
    special = {'INDEEDEE_M': 'Indeedee', 'INDEEDEE_F': 'Indeedee-F', 'JANGMO_O': 'Jangmo-o',
               'HAKAMO_O': 'Hakamo-o', 'KOMMO_O': 'Kommo-o', 'BASCULEGION_M': 'Basculegion',
               'LYCANROC_MIDDAY': 'Lycanroc', 'AEGISLASH_SHIELD': 'Aegislash',
               'MAUSHOLD_THREE': 'Maushold'}
    key = const[len('SPECIES_'):]
    if key in special:
        return special[key]
    return '-'.join(p.title() for p in parts)


def evolve(const, level):
    """The form a trainer would field at this level."""
    seen = set()
    while const not in seen:
        seen.add(const)
        nxt = None
        for method, param, target, rest in SPECIES.get(const, {}).get('evos', []):
            if 'IF_REGION' in rest or method == 'EVO_NONE' or target not in SPECIES:
                continue
            if method in ('EVO_LEVEL', 'EVO_LEVEL_BATTLE_ONLY'):
                try:
                    p = int(param)
                except ValueError:
                    continue
                if p == 0:
                    # Friendship only; place, move, or party conditions are
                    # left to players.
                    conds = re.findall(r'IF_\w+', rest)
                    ok = level >= 32 and conds and all(c in ('IF_MIN_FRIENDSHIP', 'IF_TIME', 'IF_NOT_TIME') for c in conds)
                elif re.search(r'IF_(IN_MAP|KNOWS_MOVE|PARTY|SPECIES_IN_PARTY|WEATHER|HOLD_ITEM|BAG_ITEM|DEFEAT|RECOIL|USED_MOVE|MIN_OVERWORLD_STEPS|CRITICAL|AMPED|LOW_KEY|NATURE)', rest):
                    ok = False
                else:
                    ok = level >= p + (2 if p >= 30 else 4)
            elif method == 'EVO_ITEM':
                ok = level >= 32
            elif method == 'EVO_TRADE':
                ok = level >= 36
            else:
                ok = False
            if ok:
                nxt = target
                break
        if nxt is None:
            return const
        const = nxt
    return const


# --- party file -------------------------------------------------------------
def split_entries(text):
    parts = re.split(r'(?m)^(?==== TRAINER_\w+ ===\n)', text)
    out = []
    for p in parts:
        m = re.match(r'=== (TRAINER_\w+) ===\n', p)
        out.append((m.group(1) if m else None, p))
    return out


def parse_entry(text):
    head, _, rest = text.partition('\n\n')
    mons = []
    for blk in re.split(r'\n\n+', rest.strip('\n')):
        if blk.strip():
            mons.append(blk.strip('\n').split('\n'))
    return head.split('\n'), mons


def header_get(head, key):
    for l in head:
        if l.startswith(key + ': '):
            return l[len(key) + 2:]
    return None


def mon_species(lines):
    first = lines[0]
    sp = first.split(' @ ')[0].strip()
    m = re.match(r'.*\(([^)]+)\)\s*$', sp)
    if m and m.group(1) not in ('M', 'F'):
        sp = m.group(1)
    sp = re.sub(r'\s*\((M|F)\)\s*$', '', sp)
    return sp


def mon_level(lines):
    for l in lines:
        if l.startswith('Level: '):
            return int(l[7:])
    return 1


def mon_item(lines):
    return lines[0].split(' @ ')[1].strip() if ' @ ' in lines[0] else None


def set_mon(lines, species=None, level=None, item=None, keep_moves=True):
    first = lines[0]
    sp = species if species is not None else first.split(' @ ')[0].strip()
    it = item if item is not None else mon_item(lines)
    out = [sp + (' @ ' + it if it else '')]
    for l in lines[1:]:
        if l.startswith('Level: ') and level is not None:
            out.append('Level: %d' % level)
        elif l.startswith('- ') and not keep_moves:
            continue
        elif l.startswith('Ability: ') and species is not None:
            ab = l[9:].strip().upper().replace(' ', '_').replace('-', '_')
            if ab not in SPECIES.get(const_of(species), {}).get('abilities', []):
                continue
            out.append(l)
        else:
            out.append(l)
    return out


def stage_of(level):
    for i, a in enumerate(STOCK_ACE):
        if level <= a:
            return i
    return len(STOCK_ACE) - 1


def themes_for(cls, maps):
    for m in maps:
        if m in GYM_THEME:
            return [GYM_THEME[m]]
    return CLASS_THEME.get(cls, ['MIXED'])


def pool(themes, stage, added_only):
    src = ADDED if added_only else NATIVE
    out = []
    for t in themes:
        keys = list(ADDED) if t == 'MIXED' else [t]
        for k in keys:
            for sp, st in src.get(k, []):
                if st <= stage and ('SPECIES_' + sp) not in [o for o, _ in out]:
                    out.append(('SPECIES_' + sp, k))
    return out


def base_of(const):
    """Root of the evolution line (for duplicate checks)."""
    prevo = PREVO.get(const)
    seen = set()
    while prevo and prevo not in seen:
        seen.add(prevo)
        const = prevo
        prevo = PREVO.get(const)
    return const


PREVO = {}
for c, d in SPECIES.items():
    for _, _, t, _ in d['evos']:
        PREVO.setdefault(t, c)


def new_mon(const, level, ivs):
    return [name_of(const), 'Level: %d' % level, 'IVs: ' + ivs]


def grows_now(tid, stage, double, plan):
    return stage >= 2 and plan.get('key') not in EXEMPT and not double


def rebuild(tid, head, mons, maps, plan):
    """plan: shared per base name so rematch tiers use the same species."""
    cls = header_get(head, 'Class')
    double = header_get(head, 'Double Battle') == 'Yes'
    levels = [mon_level(m) for m in mons]
    top = max(levels)
    stage = stage_of(top)
    cap = HACK_ACE[stage] - 2
    delta = DELTA[stage]
    themes = themes_for(cls, maps)
    seed = h(plan.setdefault('key', tid))
    ivs = next((l[5:] for m in mons for l in m if l.startswith('IVs: ')), '0 HP / 0 Atk / 0 Def / 0 SpA / 0 SpD / 0 Spe')

    new = []
    for m, lv in zip(mons, levels):
        lv2 = max(lv, min(lv + delta, cap))
        const = const_of(mon_species(m))
        evo = evolve(const, lv2)
        if evo != const and evo in SPECIES:
            new.append(set_mon(m, species=name_of(evo), level=lv2))
        else:
            new.append(set_mon(m, level=lv2))
    lineup = {base_of(const_of(mon_species(m))) for m in new}

    # One themed slot from the added roster (decided once per base name).
    if plan['key'] in EXEMPT:
        plan.setdefault('swap', None)
        plan.setdefault('extra', None)
    if 'swap' not in plan:
        choices = [c for c, _ in pool(themes, stage, True) if base_of(c) not in lineup]
        grows = len(mons) == 1 and stage >= 2 and tid not in EXEMPT_SIZE and not double
        want = (len(mons) >= 2 and seed % 3 != 0) or (len(mons) == 1 and not grows and seed % 3 == 0)
        plan['swap'] = choices[seed % len(choices)] if (want and choices) else None
        plan['slot'] = None
    if plan['swap'] and len(new) == 1 and grows_now(tid, stage, double, plan):
        # A lone Pokemon that gets a partner keeps itself; the swap is the partner.
        plan['extra'] = plan['swap']
    elif plan['swap']:
        ace = levels.index(top)
        slots = [i for i in range(len(new)) if i != ace] or [ace]
        slot = slots[(seed >> 8) % len(slots)] if plan['slot'] is None else min(plan['slot'], len(new) - 1)
        plan['slot'] = slot
        lv = mon_level(new[slot])
        if base_of(plan['swap']) not in lineup:
            sp = evolve(plan['swap'], lv)
            new[slot] = [name_of(sp), 'Level: %d' % lv] + [l for l in new[slot][1:] if l.startswith('IVs: ')]
            lineup = {base_of(const_of(mon_species(m))) for m in new}

    # Party size.
    size = len(new)
    target = size
    if plan['key'] not in EXEMPT and not double:
        if stage >= 2 and size == 1:
            target = 2
        elif stage >= 4 and size == 2 and (seed >> 4) % 3 == 0:
            target = 3
    if target > size:
        if 'extra' not in plan:
            pick = [c for c, _ in pool(themes, stage, (seed >> 12) % 2 == 0) if base_of(c) not in lineup]
            if not pick:
                pick = [c for c, _ in pool(themes, stage, True) + pool(themes, stage, False) if base_of(c) not in lineup]
            plan['extra'] = pick[(seed >> 16) % len(pick)] if pick else None
        if plan['extra'] and base_of(plan['extra']) not in lineup:
            lv = max(2, min(levels) + delta - 1)
            lv = min(lv, cap)
            new.append(new_mon(evolve(plan['extra'], lv), lv, ivs))
            lineup = {base_of(const_of(mon_species(m))) for m in new}

    # Rematch tiers keep the species their group added, in a non-ace slot.
    if plan.get('key') != tid:
        for added in (plan.get('swap'), plan.get('extra')):
            if not added or base_of(added) in lineup or len(new) < 2:
                continue
            ace_i = max(range(len(new)), key=lambda i: mon_level(new[i]))
            low = min((i for i in range(len(new)) if i != ace_i), key=lambda i: mon_level(new[i]))
            lv = mon_level(new[low])
            fresh = [name_of(evolve(added, lv)), 'Level: %d' % lv, 'IVs: ' + ivs]
            if len(new) < 4 and not double:
                new.append(fresh)
            else:
                new[low] = fresh
            lineup = {base_of(const_of(mon_species(m))) for m in new}

    # Held items on the ace.
    ace = max(range(len(new)), key=lambda i: mon_level(new[i]))
    if mon_item(new[ace]) is None:
        if stage >= 5 and (seed >> 20) % 6 == 0:
            t = (SPECIES.get(const_of(mon_species(new[ace])), {}).get('types') or ['NORMAL'])[0]
            if t in TYPE_ITEM:
                new[ace] = set_mon(new[ace], item=TYPE_ITEM[t])
        elif stage >= 2 and (seed >> 24) % 4 == 0:
            new[ace] = set_mon(new[ace], item='Sitrus Berry' if stage >= 4 else 'Oran Berry')

    # AI.
    flags = ['Check Bad Move', 'Try To Faint']
    if stage >= 2:
        flags.append('Check Viability')
    if stage >= 5:
        flags.append('HP Aware')
    old_ai = header_get(head, 'AI') or ''
    for extra in ('Force Setup First Turn',):
        if extra in old_ai:
            flags.append(extra)
    head2 = []
    has_ai = False
    for l in head:
        if l.startswith('AI: '):
            head2.append('AI: ' + ' / '.join(flags))
            has_ai = True
        else:
            head2.append(l)
    if not has_ai:
        head2.append('AI: ' + ' / '.join(flags))
    return head2, new, stage


def main():
    current = open(PARTY, encoding='utf-8').read()
    base_text = subprocess.check_output(['git', '-C', ROOT, 'show', BASE_COMMIT + ':src/data/trainers.party']).decode('utf-8')
    base_entries = dict(split_entries(base_text))

    t2m = {}
    for f in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')):
        m = os.path.basename(os.path.dirname(f))
        if m.endswith('_Frlg'):
            continue
        for t in re.findall(r'trainerbattle\w*\s+(TRAINER_\w+)', open(f).read()):
            t2m.setdefault(t, set()).add(m)
    rematch, group = {}, {}
    bs = open(os.path.join(ROOT, 'src/battle_setup.c')).read()
    for ids, mp in re.findall(r'REMATCH\(([^)]*?),\s*(MAP_\w+)\)', bs):
        tiers = [t.strip() for t in ids.split(',')]
        for t in tiers:
            if t.startswith('TRAINER_'):
                group[t] = tiers[0]
        for t in tiers[1:]:
            if t.startswith('TRAINER_'):
                rematch[t] = set(t2m.get(tiers[0], set()))
    # Co-op partners named in scripts (gym, E4, champion partners).
    partners = set()
    for f in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')):
        partners |= set(re.findall(r'setvar VAR_0x8005, (TRAINER_\w+)', open(f).read()))

    scope = {}
    for tid, maps in list(t2m.items()) + list(rematch.items()):
        if tid in partners or tid not in base_entries:
            continue
        head, mons = parse_entry(base_entries[tid])
        cls = header_get(head, 'Class')
        if cls in BOSS_CLASSES or not mons:
            continue
        scope[tid] = maps

    # Fights the Battle Guide v2 specifies belong to guide.py: never rebuilt
    # here, so rerunning this script cannot bring back an older team.
    import guide
    owned = guide.owned_ids()
    plans, report = {}, []
    out = []
    for tid, text in split_entries(current):
        if tid not in scope or tid in owned:
            out.append(text)
            continue
        head, mons = parse_entry(base_entries[tid])
        # Rematch tiers share one plan, so they field the same species.
        plan = plans.setdefault(group.get(tid, tid), {})
        head2, new, stage = rebuild(tid, head, mons, scope[tid], plan)
        before = ', '.join('%s %d' % (mon_species(m), mon_level(m)) for m in mons)
        after = ', '.join('%s %d%s' % (mon_species(m), mon_level(m), ' @ ' + mon_item(m) if mon_item(m) else '') for m in new)
        report.append((tid, header_get(head, 'Class'), sorted(scope[tid]), stage, before, after))
        body = '\n'.join(head2) + '\n\n' + '\n\n'.join('\n'.join(m) for m in new) + '\n\n'
        out.append(body)
    open(PARTY, 'w', encoding='utf-8').write(''.join(out))
    with open(os.path.join(ROOT, 'docs/COOP_TRAINERS_F.tsv'), 'w', encoding='utf-8') as f:
        f.write('trainer\tclass\tmaps\tstage\tbefore\tafter\n')
        for r in report:
            f.write('%s\t%s\t%s\t%d\t%s\t%s\n' % (r[0], r[1], ' '.join(r[2]) or '(rematch)', r[3], r[4], r[5]))
    print('%d ordinary trainers rebuilt (%d guide fights left to guide.py)' % (len(report), len(owned & set(scope))))


if __name__ == '__main__':
    main()
