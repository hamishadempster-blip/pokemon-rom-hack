#!/usr/bin/env python3
"""Whole-roster audit and machine-readable acquisition manifest.

Works out, from the built ROM and the sources, every species a player can
actually obtain in Hoenn (wild tables on reachable maps, gifts, static
encounters, trades, fossils, the co-op partner Pokemon, and evolutions whose
items, moves, places and conditions are themselves reachable), then checks
each one's Pokedex treatment (unique Hoenn Pokedex entry, name, types, text,
pictures, footprint, a habitat or acquisition hint).

    tools/coop_roster_audit/build_layout.sh
    tools/coop_roster_audit/roster.py            # report + docs/coop_roster_manifest.json
                                                 # + src/data/coop_dex_hints.h
    tools/coop_roster_audit/roster.py --check    # report only; exit 1 on issues

Verification status per species comes from verified.json (scenario names
whose logs show the species caught, received or evolved in play).
"""
import glob, json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tools/coop_roster'))
import audit as SRC  # noqa: E402  (source-side helpers: reachable maps)

# Offsets and symbols of the ROM as built now (a stale layout reads garbage).
import subprocess  # noqa: E402
subprocess.run([os.path.join(HERE, 'build_layout.sh')], check=True, stdout=subprocess.DEVNULL)

# The ROM-side readers of audit.py (species info, evolutions, enums).
_src = open(os.path.join(HERE, 'audit.py'), encoding='utf-8').read()
_g = {'__name__': 'roster', '__file__': os.path.join(HERE, 'audit.py')}
CHECK = '--check' in sys.argv[1:]
sys.argv = [sys.argv[0]] + [a for a in sys.argv[1:] if not a.startswith('--')]
exec(compile(_src[:_src.index('# --- intended list')], 'audit.py', 'exec'), _g)
info, SPECIES, SPECIES_NAME, ITEMS, ITEM_NAME = _g['info'], _g['SPECIES'], _g['SPECIES_NAME'], _g['ITEMS'], _g['ITEM_NAME']
PLACEHOLDER, read, enum_values = _g['PLACEHOLDER'], _g['read'], _g['enum_values']
_g['EVO'].setdefault(8, 'EVO_SPIN')  # last enum entry has no comma
MOVES = enum_values('include/constants/moves.h', 'MOVE_')
MOVE_NAME = {v: k for k, v in MOVES.items()}
MAP_NAME = {}
for _m in re.finditer(r'^\s*(MAP_\w+)\s*=\s*\((\d+)\s*\|\s*\((\d+)\s*<<\s*8\)\)', read('include/constants/map_groups.h'), re.M):
    MAP_NAME[int(_m.group(2)) | (int(_m.group(3)) << 8)] = _m.group(1)
MAPSEC = {v: k for k, v in enum_values('include/constants/region_map_sections.h', 'MAPSEC_').items()}
# Regional evolution branches (src/pokemon.c GetEvolutionRegion): the places
# that stand in for Alola, Galar and Hisui.
_REG = enum_values('include/constants/regions.h', 'REGION_')
_g['REGION'] = {v: k for k, v in _REG.items()}
_pc = read('src/pokemon.c')
_gr = _pc[_pc.index('static enum Region GetEvolutionRegion'):]
_gr = _gr[:_gr.index('\n}\n')]
REGION_PLACES = {}
for _block in re.findall(r'((?:case MAPSEC_\w+:\s*)+)return (REGION_\w+);', _gr):
    REGION_PLACES[_block[1]] = ', '.join(x.replace('_', ' ').title() for x in re.findall(r'case MAPSEC_(\w+):', _block[0]))
# Script evolution triggers placed on reachable maps (tryspecialevo).
SCRIPT_TRIGGERS = {}
_TRIG = enum_values('include/constants/pokemon.h', 'EVO_TRIGGER_', 'EvoTriggerVersion')
for _sc in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')):
    for _t in re.findall(r'tryspecialevo\s+(EVO_TRIGGER_\w+)', open(_sc, encoding='utf-8').read()):
        if _t in _TRIG:
            SCRIPT_TRIGGERS[_TRIG[_t]] = re.sub(r'(?<=[a-z0-9])(?=[A-Z])', ' ', os.path.basename(os.path.dirname(_sc)))
RAIN_WEATHERS = {v for k, v in enum_values('include/constants/weather.h', 'WEATHER_').items() if k in ('WEATHER_RAIN', 'WEATHER_FOG')}


# --- progression -----------------------------------------------------------
# What opens each map in an ordinary playthrough (badges, a key step). The
# number is the badge count the gate implies, for sorting and summaries.
GATES = [
    (r'SAFARI_ZONE_(NORTHEAST|SOUTHEAST)|BATTLE_FRONTIER', 9, 'after the Hall of Fame (National Dex areas)'),
    (r'ROUTE10[123]$|OLDALE|LITTLEROOT', 0, 'start'),
    (r'ROUTE104$|PETALBURG_(CITY|WOODS)|RUSTBORO|ROUTE116$|RUSTURF', 0, 'Petalburg/Rustboro reached'),
    (r'DEWFORD|GRANITE_CAVE', 1, 'Dewford (Mr. Briney, after the Rustboro Gym)'),
    (r'ROUTE10[5-9]$|SLATEPORT|ROUTE110$|MAUVILLE_CITY|ROUTE117$|VERDANTURF', 2, 'Slateport (after the Dewford Gym)'),
    (r'ROUTE11[1-5]$|FIERY_PATH|FALLARBOR|METEOR_FALLS_1F_1R|JAGGED_PASS|MT_CHIMNEY|MAGMA_HIDEOUT', 3, 'Route 111 north (after the Mauville Gym)'),
    (r'MIRAGE_TOWER|SANDBURIED|DESERT_RUINS', 4, 'the desert (Go-Goggles, after the Lavaridge Gym)'),
    (r'ROUTE118$|NEW_MAUVILLE|ROUTE119$|FORTREE|ROUTE12[0-3]$|LILYCOVE|MT_PYRE|SAFARI_ZONE|ROUTE124$|ABANDONED_SHIP|ROUTE13[234]$', 5, 'Surf (after the Petalburg Gym)'),
    (r'MOSSDEEP|SHOAL_CAVE|ROUTE12[5-7]$|SEAFLOOR|UNDERWATER', 7, 'Mossdeep (after the Fortree Gym)'),
    (r'SOOTOPOLIS|CAVE_OF_ORIGIN|ROUTE12[89]$|ROUTE13[01]$|PACIFIDLOG|METEOR_FALLS_(1F_2R|B1F)|SKY_PILLAR|EVER_GRANDE|VICTORY_ROAD', 8, 'the eighth badge (Waterfall)'),
    (r'SAFARI_ZONE_(NORTHEAST|SOUTHEAST)|BATTLE_FRONTIER', 9, 'after the Hall of Fame (National Dex areas)'),
    (r'DESERT_UNDERPASS|ALTERING_CAVE|ARTISAN_CAVE|METEOR_FALLS_STEVENS|GRANITE_CAVE_STEVENS|TERRA|MARINE|SOUTHERN|FARAWAY|NAVEL|BIRTH', 9, 'after the Hall of Fame'),
]
METHOD_GATE = {'surf': (5, 'Surf'), 'old rod': (1, 'Old Rod (Dewford)'), 'good rod': (5, 'Good Rod (Route 118)'),
               'super rod': (7, 'Super Rod (Mossdeep)'), 'rock smash': (3, 'Rock Smash (Mauville)')}


def map_gate(m):
    m = m.replace('MAP_', '')
    while m:
        for pat, badges, text in GATES:
            if re.search(pat, m):
                return badges, text
        m = m.rpartition('_')[0]   # a building or floor: its route or town
    return None, None


def nice_map(m):
    t = re.sub(r'(?<=[A-Z])(?=\d{3})', ' ', m.replace('MAP_', '')).title().replace('_', ' ')
    return re.sub(r'\b(\d)F\b', r'\1F', t)


# --- sources ---------------------------------------------------------------
WEIGHTS = {'land': [20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1], 'water': [60, 30, 5, 4, 1],
           'rock_smash': [60, 30, 5, 4, 1], 'fishing': [70, 30, 60, 20, 20, 40, 40, 15, 4, 1]}
ROD = {0: 'old rod', 1: 'old rod', 2: 'good rod', 3: 'good rod', 4: 'good rod'}
KIND = {'land': 'grass/cave', 'water': 'surf', 'rock_smash': 'rock smash'}

REACH = SRC.reachable_maps()
# The ticket islands need a Mystery Gift / record-mix event item (Eon, Aurora,
# Mystic Ticket, Old Sea Map); no ordinary gameplay gives one.
EVENT_ONLY = re.compile(r'FARAWAY_ISLAND|BIRTH_ISLAND|NAVEL_ROCK|SOUTHERN_ISLAND')
REACH = {m for m in REACH if not EVENT_ONLY.search(m)}
FOLDER_MAP = {}
for _mj in glob.glob(os.path.join(ROOT, 'data/maps/*/map.json')):
    FOLDER_MAP[os.path.basename(os.path.dirname(_mj))] = json.load(open(_mj))['id']
sources = {}     # species const -> list of source dicts


def add(sp, src):
    # one key per species id (SPECIES_CASTFORM_NORMAL and SPECIES_CASTFORM are one)
    sp = SPECIES_NAME.get(SPECIES.get(sp), sp)
    sources.setdefault(sp, []).append(src)


enc = json.load(open(os.path.join(ROOT, 'src/data/wild_encounters.json')))
for e in enc['wild_encounter_groups'][0]['encounters']:
    m = e.get('map')
    if not m or m not in REACH:
        continue
    for k, v in e.items():
        if not k.endswith('_mons') or not isinstance(v, dict):
            continue
        kind = k[:-5]
        mons = v['mons']
        per = {}
        for i, slot in enumerate(mons):
            w = WEIGHTS[kind][i] if i < len(WEIGHTS[kind]) else 0
            method = KIND.get(kind) or ROD.get(i, 'super rod')
            key = (slot['species'], method)
            p = per.setdefault(key, {'rate': 0, 'lo': 99, 'hi': 0})
            p['rate'] += w
            p['lo'] = min(p['lo'], slot['min_level'])
            p['hi'] = max(p['hi'], slot['max_level'])
        for (sp, method), p in per.items():
            gb, gt = map_gate(m)
            reqs = [gt] if gt else []
            badges = gb if gb is not None else -1
            if method in METHOD_GATE:
                mb, mt = METHOD_GATE[method]
                reqs.append(mt)
                badges = max(badges, mb)
            add(sp, {'method': 'wild', 'encounter': method, 'location': nice_map(m), 'map': m,
                     'levels': '%d-%d' % (p['lo'], p['hi']) if p['lo'] != p['hi'] else str(p['lo']),
                     'rate_percent': p['rate'], 'table_encounter_rate': v.get('encounter_rate'),
                     'requires': '; '.join(reqs) or 'unknown', 'badges': badges, 'renewable': True})

# Visitors by time of day and weather (src/data/coop_visitors.json,
# src/coop_encounters.c): 1 in 4 encounters of their area while their
# condition holds, at the table slot's levels.
VIS = json.load(open(os.path.join(ROOT, 'src/data/coop_visitors.json')))
_VKIND = {'land': 'land', 'surf': 'water', 'fish': 'fishing', 'rock': 'rock_smash'}
_VROD = {'old': (0, 2), 'good': (2, 5), 'super': (5, 10), 'any': (0, 10)}
_VWHEN = {'morning': 'in the morning', 'day': 'by day', 'evening': 'in the evening', 'night': 'at night',
          'rain': 'in rain', 'sand': 'in a sandstorm', 'ash': 'in falling ash'}
_ENC = {e.get('map'): e for e in enc['wild_encounter_groups'][0]['encounters'] if e.get('map') and e['base_label'].startswith('g')}
for v in VIS['visitors']:
    m = v['map']
    if m not in REACH:
        continue
    kind = _VKIND[v['area']]
    slots = _ENC[m][kind + '_mons']['mons']
    rod = v.get('rod', 'any')
    if kind == 'fishing':
        lo_i, hi_i = _VROD[rod]
        slots = slots[lo_i:hi_i]
        method = rod + ' rod' if rod != 'any' else 'fishing'
    else:
        method = KIND[kind]
    lo = min(x['min_level'] for x in slots)
    hi = max(x['max_level'] for x in slots)
    gb, gt = map_gate(m)
    reqs = [gt] if gt else []
    badges = gb if gb is not None else -1
    if method in METHOD_GATE:
        mb, mt = METHOD_GATE[method]
        reqs.append(mt)
        badges = max(badges, mb)
    add(v['species'], {'method': 'wild', 'encounter': '%s, visitor %s' % (method, _VWHEN[v['when']]),
                       'location': nice_map(m), 'map': m,
                       'levels': '%d-%d' % (lo, hi) if lo != hi else str(lo),
                       'rate_percent': VIS.get('chance_percent', 25), 'visitor': v['when'],
                       'requires': '; '.join(reqs + ['visitor ' + _VWHEN[v['when']]]),
                       'badges': badges, 'renewable': True})

# Gifts, static encounters, eggs (scripts and C), with their file.
GIFT_RE = re.compile(r'\b(givemon|setwildbattle|createmon|givecustommon|seteggmon|giveegg|seteventmon|ScriptGiveMon|CreateScriptedWildMon|CreateScriptedDoubleWildMon|setdynamicwildbattle)\b[^\n]*?\b(SPECIES_[A-Z0-9_]+)')
files = glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')) + glob.glob(os.path.join(ROOT, 'data/scripts/*.inc')) + glob.glob(os.path.join(ROOT, 'src/*.c'))
for p in files:
    base = os.path.basename(p)
    if 'debug' in base or '_Frlg' in p or 'frlg' in base.lower() or 'test' in base or base.startswith('gift_'):  # gift_*.inc: Mystery Gift events
        continue
    folder = os.path.basename(os.path.dirname(p))
    if p.endswith('scripts.inc'):
        mapc = FOLDER_MAP.get(folder)
        if mapc not in REACH:
            continue
        where = re.sub(r'(?<=[a-z0-9])(?=[A-Z])', ' ', folder.replace('_', ' '))
    else:
        mapc, where = None, os.path.relpath(p, ROOT)
    txt = read(os.path.relpath(p, ROOT))
    for mm in GIFT_RE.finditer(txt):
        kind = {'givemon': 'gift', 'giveegg': 'egg', 'seteggmon': 'egg'}.get(mm.group(1), 'static encounter')
        gb, gt = map_gate(mapc) if mapc else (None, None)
        add(mm.group(2), {'method': kind, 'location': where, 'map': mapc, 'requires': gt or 'see location',
                          'badges': gb if gb is not None else -1, 'renewable': False})
# In-game trades used by reachable Emerald maps (trade.h also holds the FRLG
# ones; an entry's '#if defined(FIRERED)' half is not this build's).
trade_txt = re.sub(r'#if defined\(FIRERED\)[\s\S]*?#else|#endif', '', read('src/data/trade.h'))
trade_species = {}
for m in re.finditer(r'\[(INGAME_TRADE_\w+)\]\s*=\s*\{([\s\S]*?)\n    \},', trade_txt):
    sp = re.search(r'\.species\s*=\s*(SPECIES_\w+)', m.group(2))
    if sp:
        trade_species[m.group(1)] = sp.group(1)
for sc in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')):
    folder = os.path.basename(os.path.dirname(sc))
    mapc = FOLDER_MAP.get(folder)
    if mapc not in REACH:
        continue
    for tid in set(re.findall(r'\b(INGAME_TRADE_\w+)', open(sc, encoding='utf-8').read())):
        if tid in trade_species:
            gb, gt = map_gate(mapc)
            add(trade_species[tid], {'method': 'in-game trade', 'location': re.sub(r'(?<=[a-z0-9])(?=[A-Z])', ' ', folder.replace('_', ' ')),
                                     'map': mapc, 'requires': gt or 'see location', 'badges': gb if gb is not None else -1, 'renewable': False})

# Stock sources outside the wild tables and the givemon scripts.
# Birch's bag: one case of two same-type starters (gCoopCaseStarters, coop.c):
# the case player takes the first, the partner (or nobody, solo) the second.
_cases = re.search(r'gCoopCaseStarters\[3\]\[2\] = \{(.*?)\};', read('src/coop.c'), re.S).group(1)
for _player, _companion in re.findall(r'\{ (SPECIES_\w+), (SPECIES_\w+) \}', _cases):
    add(_player, {'method': 'first partner (Birch, case player)', 'location': 'Route 101', 'map': 'MAP_ROUTE101',
                  'requires': 'start', 'badges': 0, 'renewable': False, 'one_of': 'Hoenn starter cases'})
    add(_companion, {'method': 'first partner (Birch, partner of the case; co-op)', 'location': 'Route 101', 'map': 'MAP_ROUTE101',
                     'requires': 'start, linked', 'badges': 0, 'renewable': False, 'one_of': 'Hoenn starter cases', 'coop_only': True})
# Legendary encounters handled by src/coop_legend.c (both players' saves).
for m in re.finditer(r'\[COOP_ENC_\w+\]\s*=\s*\{\s*(SPECIES_\w+),\s*(\d+),[^}]*?MAP_NUM\((MAP_\w+)\)', read('src/coop_legend.c')):
    gb, gt = map_gate(m.group(3))
    add(m.group(1), {'method': 'legendary encounter (once)', 'location': nice_map(m.group(3)), 'map': m.group(3), 'levels': m.group(2),
                     'requires': gt or 'see location', 'badges': gb if gb is not None else -1, 'renewable': False})
# Roaming Latias or Latios: the player's TV answer after the Hall of Fame picks one.
for sp in ('SPECIES_LATIAS', 'SPECIES_LATIOS'):
    add(sp, {'method': 'roaming (Latias or Latios, one per save)', 'location': 'Hoenn routes', 'levels': '40',
             'requires': 'after the Hall of Fame', 'badges': 9, 'renewable': False, 'one_of': 'Eon duo'})
# Feebas: six fishing tiles on Route 119 (src/wild_encounter.c).
if 'gWildFeebas' in read('src/wild_encounter.c'):
    add('SPECIES_FEEBAS', {'method': 'wild', 'encounter': 'fishing, 6 tiles', 'location': 'Route 119', 'map': 'MAP_ROUTE119', 'levels': '20-25',
                           'rate_percent': 50, 'requires': 'Surf (after the Petalburg Gym); any rod', 'badges': 5, 'renewable': True})
# Co-op wild pairs: a partner on the same tile turns a single wild Pokemon into
# its counterpart pair (src/wild_encounter.c Coop_WildPairPartner). Co-op only.
_wc = read('src/wild_encounter.c')
_pairs = re.findall(r'case (SPECIES_\w+): return (SPECIES_\w+);', _wc[_wc.index('Coop_WildPairPartner'):_wc.index('void Coop_ApplyWildPair')])
COOP_PAIR_SRC = {}
for a, b in _pairs:
    COOP_PAIR_SRC.setdefault(b, []).append(a)

for b, alist in COOP_PAIR_SRC.items():
    for a in alist:
        for w in [x for x in sources.get(a, []) if x['method'] == 'wild']:
            add(b, {'method': 'co-op wild pair', 'encounter': w['encounter'], 'location': w['location'], 'map': w['map'], 'levels': w['levels'],
                    'rate_percent': w['rate_percent'], 'requires': w['requires'] + '; both players on the same tile when a wild %s appears' % a[8:].title(),
                    'badges': w['badges'], 'renewable': True, 'coop_only': True})
# Birch's Johto starters are a post-game gift (stock Emerald).
for sp in ('SPECIES_CHIKORITA', 'SPECIES_CYNDAQUIL', 'SPECIES_TOTODILE'):
    for x in sources.get(sp, []):
        if x['method'].startswith('first partner'):
            continue
        x['requires'], x['badges'] = 'after the Hall of Fame (Birch)', 9

# Co-op roster code: starter guests and fossils (src/coop_roster.c).
roster_c = read('src/coop_roster.c')
GUEST_TOWN = {'KANTO': 'Rustboro', 'SINNOH': 'Lavaridge', 'UNOVA': 'Fortree', 'KALOS': 'Mossdeep',
              'ALOLA': 'Slateport', 'GALAR': 'Petalburg', 'PALDEA': 'Sootopolis', 'HOENN': 'Fallarbor'}
for m in re.finditer(r'\[COOP_STARTER_GUEST_(\w+)\]\s*=\s*\{\s*COMPOUND_STRING\("\w+"\),\s*(\d+),\s*(\d+),\s*\w+,\s*\{([^}]*)\}', roster_c):
    region, badges, level = m.group(1), int(m.group(2)), m.group(3)
    for sp in re.findall(r'SPECIES_\w+', m.group(4)):
        # the scripted givemon in coop_starter_guests.inc is this source
        sp = SPECIES_NAME.get(SPECIES.get(sp), sp)
        sources[sp] = [s for s in sources.get(sp, []) if 'coop_roster.c' not in s['location']]
        add(sp, {'method': 'partner Pokemon (daily gift)', 'location': '%s Pokemon Center 1F, %s guest' % (GUEST_TOWN.get(region, region), region.title()),
                 'levels': level, 'requires': '%d badge%s' % (badges, '' if badges == 1 else 's'), 'badges': badges, 'renewable': True})
FOSSIL_TO = {}
for it, sp in re.findall(r'\{\s*(ITEM_\w+),\s*(SPECIES_\w+)\s*\}', roster_c):
    FOSSIL_TO[it] = sp
FOSSIL_TO.update({'ITEM_ROOT_FOSSIL': 'SPECIES_LILEEP', 'ITEM_CLAW_FOSSIL': 'SPECIES_ANORITH'})
for it, sp in FOSSIL_TO.items():
    sp = SPECIES_NAME.get(SPECIES.get(sp), sp)
    sources[sp] = [s for s in sources.get(sp, []) if 'coop_roster.c' not in s['location'] and 'DevonCorp' not in s['location']]
    add(sp, {'method': 'fossil revival', 'location': 'Fossil Maniac, Route 114 tunnel (one fossil a day, any kind) -> DEVON 2F, Rustboro',
             'item': it, 'levels': '20', 'requires': 'Route 114 (after the Mauville Gym)', 'badges': 3, 'renewable': True})
# Galar fossils: a pair (upper and lower half) per revival.
for a, b, sp in re.findall(r'\{\s*(ITEM_FOSSILIZED_\w+),\s*(ITEM_FOSSILIZED_\w+),\s*(SPECIES_\w+)\s*\}', roster_c):
    add(sp, {'method': 'fossil revival', 'location': 'Fossil Maniac, Route 114 tunnel (one pair a day) -> DEVON 2F, Rustboro',
             'item': '%s + %s' % (a, b), 'levels': '20', 'requires': 'Route 114 (after the Mauville Gym)', 'badges': 3, 'renewable': True})

# Item sources (for evolution items), as in audit.py.
item_src = {}
for mj in glob.glob(os.path.join(ROOT, 'data/maps/*/map.json')):
    d = json.load(open(mj))
    name = os.path.basename(os.path.dirname(mj))
    if name.endswith('_Frlg'):
        continue
    for o in d.get('object_events', []):
        if o.get('graphics_id') == 'OBJ_EVENT_GFX_ITEM_BALL':
            item_src.setdefault(o.get('trainer_sight_or_berry_tree_id'), set()).add(('item ball', name, False))
    for b in d.get('bg_events', []):
        if b.get('type') == 'hidden_item':
            item_src.setdefault(b.get('item'), set()).add(('hidden item', name, False))
for sc in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')) + glob.glob(os.path.join(ROOT, 'data/scripts/*.inc')):
    if '_Frlg' in sc or 'frlg' in os.path.basename(sc).lower():
        continue
    txt = open(sc, encoding='utf-8').read()
    where = os.path.basename(os.path.dirname(sc)) if '/maps/' in sc else os.path.basename(sc)
    for it in re.findall(r'giveitem\s+(ITEM_\w+)', txt):
        item_src.setdefault(it, set()).add(('given', where, False))
    for it in re.findall(r'\.2byte\s+(ITEM_\w+)', txt):
        item_src.setdefault(it, set()).add(('shop', where, True))
for it in FOSSIL_TO:
    item_src.setdefault(it, set()).add(('daily gift', 'Route114_FossilManiacsTunnel', True))


def quest_gated(where):
    # The POWER SHOP (MauvilleCity's lists) opens after Power Problems; the
    # quest scripts' givers and stalls need their quest.
    return where == 'MauvilleCity' or where.startswith('coop_quest')


def item_sources(it):
    aliases = [k for k, v in ITEMS.items() if v == ITEMS.get(it, -1)] or [it]
    out = set()
    for a in aliases:
        out |= item_src.get(a, set())
    return sorted(out)


# --- evolution closure -----------------------------------------------------
evos_from = {}
for sid in sorted(SPECIES_NAME):
    if sid == 0:
        continue
    try:
        d = info(sid)
    except Exception:
        continue
    for m, p, t, conds in d['evos']:
        evos_from.setdefault(sid, []).append((m, p, t, conds))

learnable_cache = {}


def learns(sid, move):
    if sid not in learnable_cache:
        learnable_cache[sid] = {mv for _, mv in info(sid)['learnset']}
    return move in learnable_cache[sid]


def evo_requirement(sid, m, p, conds):
    """(feasible, solo, text, resources)"""
    res, notes, feasible, solo = [], [], True, True
    if m == 'EVO_NONE':
        return False, False, 'no method', []
    if m == 'EVO_ITEM':
        it = ITEM_NAME.get(p, p)
        src = item_sources(it)
        res.append({'item': it, 'sources': ['%s: %s' % (k, w) for k, w, _ in src], 'renewable': any(r for _, _, r in src)})
        if not src:
            feasible = False
        notes.append('use ' + str(it).replace('ITEM_', '').replace('_', ' ').title())
    elif m in ('EVO_LEVEL', 'EVO_LEVEL_BATTLE_ONLY'):
        notes.append('Lv %d' % p if p else 'level up')
    elif m == 'EVO_TRADE':
        solo = False
        notes.append('trade')
    elif m == 'EVO_SCRIPT_TRIGGER':
        trig = SCRIPT_TRIGGERS.get(p)
        if not trig:
            return False, False, 'script trigger %s: no place in this build' % p, []
        notes.append('at ' + trig)
    elif m == 'EVO_BATTLE_END':
        notes.append('at the end of a battle')
    elif m == 'EVO_SPIN':
        notes.append('spin in place on the field (%s)' % {0: 'clockwise, short', 1: 'counter-clockwise, short', 2: 'clockwise, long', 3: 'counter-clockwise, long', 4: 'either way, 10 s+'}.get(p, p))
    else:
        notes.append(str(m).replace('EVO_', '').lower())
    for c, a in conds:
        if c == 'IF_REGION':
            place = REGION_PLACES.get(_g['REGION'].get(a, a)) if 'REGION' in _g else REGION_PLACES.get(a)
            if not place:
                return False, False, 'needs another region', []
            notes.append('in ' + place)
            continue
        if c in ('IF_HOLD_ITEM', 'IF_BAG_ITEM_COUNT'):
            it = ITEM_NAME.get(a, a)
            src = item_sources(it)
            res.append({'item': it, 'sources': ['%s: %s' % (k, w) for k, w, _ in src], 'renewable': any(r for _, _, r in src)})
            if not src:
                feasible = False
            notes.append('holding ' + str(it).replace('ITEM_', '').replace('_', ' ').title())
        elif c == 'IF_KNOWS_MOVE':
            mv = MOVE_NAME.get(a, a)
            notes.append('knowing ' + str(mv).replace('MOVE_', '').replace('_', ' ').title())
            if not learns(sid, a):
                notes.append('(not by level-up: TM/tutor)')
        elif c == 'IF_KNOWS_MOVE_TYPE':
            notes.append('knowing a %s-type move' % _g['TYPES'].get(a, a))
        elif c == 'IF_MIN_FRIENDSHIP':
            notes.append('high friendship')
        elif c == 'IF_TIME':
            notes.append({0: 'in the morning', 1: 'by day', 2: 'in the evening', 3: 'at night'}.get(a, 'time %d' % a))
        elif c == 'IF_NOT_TIME':
            notes.append({3: 'not at night'}.get(a, 'not time %d' % a))
        elif c == 'IF_IN_MAP':
            mp = MAP_NAME.get(a)
            notes.append('in ' + nice_map(mp) if mp else 'in map %d' % a)
            if mp and mp not in REACH:
                feasible = False
        elif c == 'IF_IN_MAPSEC':
            notes.append('in ' + MAPSEC.get(a, str(a)).replace('MAPSEC_', '').replace('_', ' ').title())
        elif c in ('IF_AMPED_NATURE', 'IF_LOW_KEY_NATURE'):
            notes.append({'IF_AMPED_NATURE': 'Amped nature', 'IF_LOW_KEY_NATURE': 'Low Key nature'}[c])
        elif c == 'IF_GENDER':
            notes.append('female' if a == 254 else 'male' if a == 0 else 'gender %d' % a)
        elif c == 'IF_NOT_REGION':
            pass
        elif c == 'IF_SPECIES_IN_PARTY':
            notes.append('with %s in the party' % (SPECIES_NAME.get(a, str(a))[8:].title()))
            if a not in obtainable:
                feasible = False
        elif c == 'IF_WEATHER':
            notes.append('in rain or fog (Route 119 rain; fog on Mt. Pyre summit, Rusturf Tunnel)' if a in RAIN_WEATHERS else 'weather %s' % a)
            if a not in RAIN_WEATHERS:
                feasible = False
        elif c == 'IF_USED_MOVE_X_TIMES':
            mv = MOVE_NAME.get(a, a)
            notes.append('after using ' + str(mv).replace('MOVE_', '').replace('_', ' ').title() + ' many times')
            if not learns(sid, a):
                notes.append('(not by level-up)')
        elif c == 'IF_CRITICAL_HITS_GE':
            notes.append('landing %d critical hits in one battle' % a)
        elif c == 'IF_CURRENT_DAMAGE_GE':
            notes.append('having lost %d HP or more' % a)
        elif c == 'IF_MIN_OVERWORLD_STEPS':
            notes.append('after %d steps in the lead' % a)
        elif c.startswith('IF_PID'):
            notes.append('decided by personality')
        else:
            notes.append(c.replace('IF_', '').lower())
    return feasible, solo, ', '.join(notes), res


# Egg data from the species sources (egg groups, gender) for breeding.
EGG = {}
for _f in glob.glob(os.path.join(ROOT, 'src/data/pokemon/species_info/*.h')):
    _t = open(_f, encoding='utf-8').read()
    for _m in re.finditer(r'\[(SPECIES_\w+)\]\s*=\s*\{([\s\S]*?)\n    \},', _t):
        _eg = re.search(r'\.eggGroups\s*=\s*MON_EGG_GROUPS\(([^)]*)\)', _m.group(2))
        _gr = re.search(r'\.genderRatio\s*=\s*([^,\n]+),', _m.group(2))
        if _eg:
            gr = _gr.group(1).strip() if _gr else ''
            gender = 'none' if 'GENDERLESS' in gr else 'male' if 'MON_MALE' in gr or 'PERCENT_FEMALE(0)' in gr \
                else 'female' if 'MON_FEMALE' in gr or 'PERCENT_FEMALE(100)' in gr else 'both' if 'PERCENT_FEMALE' in gr else None
            EGG[SPECIES_NAME.get(SPECIES.get(_m.group(1)), _m.group(1))] = ({g.strip() for g in _eg.group(1).split(',')}, gender)
NO_EGGS = 'EGG_GROUP_NO_EGGS_DISCOVERED'
EGG_FIX = {'SPECIES_SINISTEA_ANTIQUE': 'SPECIES_SINISTEA_PHONY', 'SPECIES_POLTCHAGEIST_ARTISAN': 'SPECIES_POLTCHAGEIST_COUNTERFEIT'}
pre_of = {}
for _a in sorted(evos_from):
    for _m, _p, _t, _c in evos_from[_a]:
        pre_of.setdefault(_t, _a)   # GetEggSpecies: the first species (by id) that evolves into it


def egg_species(sid):
    for _ in range(5):
        if sid not in pre_of:
            break
        sid = pre_of[sid]
    name = SPECIES_NAME.get(sid)
    return SPECIES.get(EGG_FIX.get(name, name), sid)


def can_be(name, g):
    e = EGG.get(name)
    return e is not None and NO_EGGS not in e[0] and (e[1] == 'both' or e[1] == g)


# Unknown gender (a ratio set by a macro the parser does not follow): not bred.


obtainable = {}   # sid -> True
evo_info = {}     # target sid -> list of {from, method, solo, resources}


def close_evolutions():
    changed = True
    while changed:
        changed = False
        for a in list(obtainable):
            for m, p, t, conds in evos_from.get(a, []):
                ok, solo, text, res = evo_requirement(a, m, p, conds)
                ent = {'from': SPECIES_NAME.get(a), 'how': text, 'solo': solo, 'feasible': ok, 'resources': res}
                lst = evo_info.setdefault(t, [])
                if ent not in lst:
                    lst.append(ent)
                if ok and t not in obtainable:
                    obtainable[t] = True
                    changed = True


def renewable_set():
    out = {SPECIES[n] for n, l in sources.items() if n in SPECIES and any(x.get('renewable') for x in l)}
    grew = True
    while grew:
        grew = False
        for t, lst in evo_info.items():
            if t not in out and any(e['feasible'] and SPECIES.get(e['from']) in out for e in lst):
                out.add(t)
                grew = True
    return out


# Breeding (Day Care, Route 117; Gen 9 rules: babies hatch without incense):
# a mother of the species, or any parent with Ditto, gives the egg species.
DAYCARE = ('MAP_ROUTE117', 2)
while True:
    for s_ in {SPECIES[s] for s in sources if s in SPECIES}:
        obtainable[s_] = True
    close_evolutions()
    ren = renewable_set()
    ditto = SPECIES['SPECIES_DITTO'] in obtainable
    added = False
    for sid in sorted(obtainable):
        name = SPECIES_NAME.get(sid)
        e = EGG.get(name)
        if not e or NO_EGGS in e[0] or name == 'SPECIES_DITTO' or e[1] is None:
            continue
        egg = egg_species(sid)
        egg_name = SPECIES_NAME.get(egg)
        if egg_name is None or any(x.get('parent') == name for x in sources.get(egg_name, [])):
            continue
        if any(x['method'] != 'breeding' and x.get('renewable') for x in sources.get(egg_name, [])):
            continue   # already renewable without the Day Care
        how = None
        if can_be(name, 'female'):
            if e[1] == 'both' and sid in ren:
                how = 'a female and a male %s' % name[8:].title()
            else:
                for q in sorted(obtainable):
                    qn = SPECIES_NAME.get(q)
                    if q != sid and q in ren and can_be(qn, 'male') and EGG[qn][0] & e[0]:
                        how = 'a female %s and a male %s' % (name[8:].title(), qn[8:].title())
                        break
        if how is None and ditto:
            how = 'with Ditto'
        if how is None:
            continue
        add(egg_name, {'method': 'breeding', 'parent': name, 'location': 'Day Care, Route 117 (%s)' % how, 'map': DAYCARE[0],
                       'requires': 'Route 117; a %s%s' % (name[8:].title().replace('_', ' '), ' and a Ditto' if how == 'with Ditto' else ''),
                       'badges': DAYCARE[1], 'renewable': True, 'needs_ditto': how == 'with Ditto'})
        added = added or SPECIES.get(egg_name) not in obtainable
    if not added:
        break
direct = {SPECIES[s] for s in sources if s in SPECIES}
# Breeding-only sources do not count as a first source for the badge summary
# or for solo play unless the parent itself is solo-obtainable (checked below).

# A species is solo-completable when a solo chain exists: some direct source
# that is not co-op only (a bred egg counts once its parent is solo-completable),
# or a solo evolution from a solo-completable species.
solo_ok = {SPECIES[s] for s, l in sources.items() if s in SPECIES and any(not x.get('coop_only') and x['method'] != 'breeding' for x in l)}
changed = True
while changed:
    changed = False
    for s, l in sources.items():
        if s in SPECIES and SPECIES[s] not in solo_ok and any(x['method'] == 'breeding' and SPECIES.get(x['parent']) in solo_ok for x in l):
            solo_ok.add(SPECIES[s])
            changed = True
    for t, lst in evo_info.items():
        if t in solo_ok:
            continue
        for e in lst:
            if e['feasible'] and e['solo'] and SPECIES.get(e['from']) in solo_ok:
                solo_ok.add(t)
                changed = True
                break

# Earliest badge count at which each species can be had (sources, then
# through evolutions).
earliest = {}
for name, lst in sources.items():
    if name in SPECIES:
        b = [x.get('badges', -1) for x in lst if x['method'] != 'breeding']
        b = [x for x in b if x is not None and x >= 0]
        if b or not any(x['method'] == 'breeding' for x in lst):
            earliest[SPECIES[name]] = min(b) if b else 0
changed = True
while changed:
    changed = False
    for name, lst in sources.items():
        for x in lst:
            par = SPECIES.get(x.get('parent'))
            if x['method'] == 'breeding' and par in earliest and (not x.get('needs_ditto') or SPECIES['SPECIES_DITTO'] in earliest):
                v = max(DAYCARE[1], earliest[par], earliest[SPECIES['SPECIES_DITTO']] if x.get('needs_ditto') else 0)
                if x['badges'] != v:
                    x['badges'] = v
                if SPECIES[name] not in earliest or v < earliest[SPECIES[name]]:
                    earliest[SPECIES[name]] = v
                    changed = True
    for t, lst in evo_info.items():
        for e in lst:
            f = SPECIES.get(e['from'])
            if e['feasible'] and f in earliest and (t not in earliest or earliest[f] < earliest[t]):
                earliest[t] = earliest[f]
                changed = True

# --- Pokedex checks ----------------------------------------------------------
pdx = read('include/constants/pokedex.h')
seg = pdx[pdx.index('define FOREACH_SPECIES_IN_HOENN_DEX_ORDER'):pdx.index('define FOREACH_SPECIES_IN_KANTO_DEX_ORDER')]
hoenn = re.findall(r'F\((\w+)\)', seg)
natnum = {k[len('NATIONAL_DEX_'):]: i for i, k in enumerate(re.findall(r'^\s*(NATIONAL_DEX_\w+)', pdx[pdx.index('enum NationalDexOrder'):], re.M))}
hoenn_by_nat = {}
for i, n in enumerate(hoenn):
    hoenn_by_nat.setdefault(natnum.get(n), []).append(i + 1)

by_nat = {}
for sid in obtainable:
    d = info(sid)
    by_nat.setdefault(d['natdex'], []).append(sid)

issues = []
entries = []
FOOT = _g.get('O_FOOTPRINT')
for nat in sorted(by_nat):
    sids = sorted(by_nat[nat])
    base_sid = min(sids)
    d = info(base_sid)
    const = SPECIES_NAME.get(base_sid)
    hd = hoenn_by_nat.get(nat, [])
    probs = []
    post_game = min(earliest.get(x, 0) for x in sids) >= 9
    if len(hd) > 1 or (not hd and not post_game):
        probs.append('Hoenn Pokedex entries: %d' % len(hd))
    if not d['name'] or '?' in d['name'] or ('~' in d['name'] and nat not in (29, 32)):
        probs.append('name %r' % d['name'])
    if not d['desc']:
        probs.append('no Pokedex text')
    if '?' in d['types']:
        probs.append('types %s' % d['types'])
    for sid in sids:
        g = info(sid)['gfx']
        ph = [k for k, v in g.items() if v in PLACEHOLDER or not v]
        if ph:
            probs.append('%s placeholder %s' % (SPECIES_NAME.get(sid), ph))
    if FOOT is not None and not d.get('footprint'):
        probs.append('no footprint')
    srcs = []
    for sid in sids:
        srcs += sources.get(SPECIES_NAME.get(sid), [])
    wild_here = [s for s in srcs if s['method'] == 'wild']
    evs = []
    for sid in sids:
        for e in evo_info.get(sid, []):
            if e['feasible']:
                evs.append(dict(e, to=SPECIES_NAME.get(sid)))
    if not srcs and not evs:
        probs.append('no acquisition path')
    for p in probs:
        issues.append('%s (#%d): %s' % (const, nat, p))
    entries.append({
        'species': const, 'national_dex': nat, 'hoenn_dex': hd[0] if len(hd) == 1 else (hd or 'National Dex only (post-game)'),
        'earliest_badges_with_evolution': min(earliest.get(x, 0) for x in sids),
        'name': d['name'].strip(), 'types': sorted(set(d['types'])), 'forms_obtainable': [SPECIES_NAME.get(s) for s in sids],
        'sources': srcs, 'evolution': evs,
        'renewable': any(s.get('renewable') for s in srcs) or any(all(r['renewable'] for r in e['resources']) for e in evs),
        'solo': any(s in solo_ok for s in sids),
        'earliest_badges': min([s['badges'] for s in srcs if s.get('badges', -1) >= 0] or [None]) if srcs else None,
        'dex_area': 'wild areas shown' if wild_here else 'acquisition hint',
        'issues': probs,
    })

# Evolution resources must have a source that no quest gates (the POWER
# SHOP opens after Power Problems; quest stalls and givers need their quest).
for e in entries:
    for v in e['evolution']:
        for r in v['resources']:
            if r['sources'] and all(quest_gated(x.split(': ', 1)[-1]) for x in r['sources']):
                issues.append('%s: %s only from a quest (%s)' % (e['species'], r['item'], ', '.join(r['sources'])))

# Verification status: species seen caught/received/evolved in gameplay logs.
ver_path = os.path.join(HERE, 'verified.json')
verified = json.load(open(ver_path)) if os.path.exists(ver_path) else {}
for e in entries:
    v = next((verified[k] for k in [e['species'], 'SPECIES_' + ''.join(c for c in e['name'].upper() if c.isalnum())] + e['forms_obtainable'] if k in verified), None)
    e['verification'] = v if v else 'source-checked (audit), not played'

# Hints for the Pokedex AREA screen (species with no wild area).
HINT_OVERRIDE = {
    292: 'NINCADA EVOLVES WITH A SPARE POKé BALL',
    869: 'EVOLVE MILCERY: HOLD A SWEET, SPIN IN PLACE',
    849: 'EVOLVE TOXEL: LV 30 (NATURE SETS THE FORM)',
    152: 'GIFT AFTER THE LEAGUE: BIRCH\'S LAB', 155: 'GIFT AFTER THE LEAGUE: BIRCH\'S LAB',
    158: 'GIFT AFTER THE LEAGUE: BIRCH\'S LAB',
    106: 'EVOLVE TYROGUE: LV 20, ATTACK > DEFENSE', 107: 'EVOLVE TYROGUE: LV 20, ATTACK < DEFENSE',
    237: 'EVOLVE TYROGUE: LV 20, ATTACK = DEFENSE',
    589: 'KARRABLAST + LINKING CORD, SHELMET IN PARTY', 617: 'SHELMET + LINKING CORD, KARRABLAST IN PARTY',
    675: 'EVOLVE PANCHAM: LV 32, DARK TYPE IN PARTY', 706: 'EVOLVE SLIGGOO: LV 50 IN RAIN OR FOG',
    865: 'GALAR FARFETCH\'D: 3 CRITS IN ONE BATTLE', 867: 'GALAR YAMASK, 49 HP LOST: RUINS CARVING',
    899: 'EVOLVE STANTLER: USE PSYSHIELD BASH 20 TIMES', 901: 'EVOLVE URSARING: PEAT BLOCK, MT PYRE, NIGHT',
    923: 'EVOLVE PAWMO: 1000 STEPS IN THE LEAD', 947: 'EVOLVE BRAMBLIN: 1000 STEPS IN THE LEAD',
    954: 'EVOLVE RELLOR: 1000 STEPS IN THE LEAD', 979: 'EVOLVE PRIMEAPE: USE RAGE FIST 20 TIMES',
    982: 'DUNSPARCE: LEVEL UP KNOWING HYPER DRILL', 1013: 'POLTCHAGEIST ARTISAN: MASTERPIECE TEACUP',
}


def short_hint(e):
    if e['national_dex'] in HINT_OVERRIDE and not any(x['method'] == 'wild' for x in e['sources']):
        return HINT_OVERRIDE[e['national_dex']]
    s = e['sources']
    if any(x['method'] == 'wild' for x in s):
        return None
    order = ['partner', 'fossil revival', 'gift', 'egg', 'legendary', 'static encounter', 'in-game trade', 'first partner', 'roaming', 'co-op wild pair']
    pick = sorted([x for x in s if x['method'] != 'breeding'], key=lambda x: next((i for i, o in enumerate(order) if x['method'].startswith(o)), 99))
    if pick:
        x = pick[0]
        if x['method'].startswith('partner'):
            town = x['location'].split(' Pokemon')[0].upper()
            return 'DAILY GIFT, %s CENTER: %s' % (town, x['requires'].upper())
        if x['method'] == 'fossil revival':
            return 'FOSSIL: RT 114 TUNNEL, REVIVED AT DEVON'
        if x['method'] in ('gift', 'egg'):
            return ('%s: %s' % (x['method'].upper(), x['location'].upper()))[:44]
        if x['method'].startswith('legendary') or x['method'] == 'static encounter':
            return ('ONE ENCOUNTER: %s' % x['location'].upper())[:44]
        if x['method'] == 'in-game trade':
            return ('IN-GAME TRADE: %s' % x['location'].upper())[:44]
        if x['method'].startswith('roaming'):
            return 'ROAMS HOENN AFTER THE LEAGUE (TV CHOICE)'
    breed = [x for x in s if x['method'] == 'breeding']
    if breed and not e['evolution']:
        par = sorted(breed, key=lambda x: (x['badges'], x['parent']))[0]['parent'][8:].replace('_', ' ')
        return ('DAY CARE EGG FROM %s, ROUTE 117' % par)[:44]
    if e['evolution']:
        evs = sorted(e['evolution'], key=lambda v: (not v['solo'], 'use ' not in v['how'], len(v['how'])))
        ev = evs[0]
        frm = (ev['from'] or '').replace('SPECIES_', '').replace('_', ' ')
        how = ev['how']
        for a, b in (('level up, ', ''), ('high friendship', 'friendship'), ('not at night', 'by day'),
                     ('knowing a Fairy-type move', 'Fairy move'), ('use ', ''), ('decided by personality', 'by personality'),
                     ('(not by level-up: TM/tutor)', ''), ('spin in place on the field', 'spin'), ('holding ', '+ ')):
            how = how.replace(a, b)
        how = re.sub(r', $', '', how.strip())
        txt = 'EVOLVE %s: %s' % (frm, how.upper())
        if len(txt) > 44:
            txt = txt[:43].rstrip(', ') + '.'
        return txt
    return None


hints = []
for e in entries:
    h = short_hint(e)
    if h:
        e['dex_hint'] = h
        hints.append((e['national_dex'], h))

if not CHECK:
    with open(os.path.join(ROOT, 'docs/coop_roster_manifest.json'), 'w') as f:
        json.dump({'generated_by': 'tools/coop_roster_audit/roster.py', 'species_count': len(entries),
                   'species': entries}, f, indent=1, ensure_ascii=False)
        f.write('\n')
    def gba(s):
        return s.replace('"', "'")
    with open(os.path.join(ROOT, 'src/data/coop_dex_hints.h'), 'w') as f:
        f.write('// Generated by tools/coop_roster_audit/roster.py: the Pokedex AREA screen\n')
        f.write('// line for obtainable species with no wild area (gifts, fossils, trades,\n')
        f.write('// static encounters, evolutions). Sorted by National Dex number.\n')
        f.write('static const struct CoopDexHint sCoopDexHints[] = {\n')
        for nat, h in sorted(hints):
            f.write('    { %d, COMPOUND_STRING("%s") },\n' % (nat, gba(h)))
        f.write('};\n')

# --- summary -----------------------------------------------------------------
print('Obtainable species (by National Dex number): %d; forms: %d' % (len(entries), sum(len(e['forms_obtainable']) for e in entries)))
print('Direct sources: %d species; by evolution only: %d' % (sum(1 for e in entries if e['sources']), sum(1 for e in entries if not e['sources'])))
print('Solo-completable: %d / %d' % (sum(1 for e in entries if e['solo']), len(entries)))
print('Not renewable: %s' % ', '.join(e['species'][8:] for e in entries if not e['renewable']))
print('Hoenn Pokedex size: %d entries (unique: %s)' % (len(hoenn), len(hoenn) == len(set(hoenn))))
obt_nats = {e['national_dex'] for e in entries}
dex_unobtainable = [n for n in hoenn if natnum.get(n) not in obt_nats]
print('Hoenn Pokedex entries with no acquisition path in this build: %d (%s)' % (len(dex_unobtainable), ', '.join(dex_unobtainable)))
hoenn_set = set(hoenn)
natnum_name = {v: k for k, v in natnum.items()}
not_regional = [e['species'][8:] for e in entries if natnum_name.get(e['national_dex']) not in hoenn_set]
print('Obtainable species missing from the Hoenn Pokedex: %d%s' % (len(not_regional), ' (%s)' % ', '.join(not_regional) if not_regional else ''))
if not_regional:
    issues.append('obtainable species with no Hoenn number: %s' % ', '.join(not_regional))
for bname in ('BATCH1', 'BATCH2', 'BATCH3'):
    names = getattr(SRC, bname)
    got = [n for n in names if natnum.get(n) in obt_nats]
    print('%s: %d of %d obtainable%s' % (bname, len(got), len(names), '' if len(got) == len(names) else ' (missing: %s)' % ', '.join(n for n in names if n not in got)))
# The intended ordinary roster: every National Dex species except the
# legendary, mythical, Ultra Beast and Paradox ones (deferred: no new
# encounters for them; the existing Hoenn legendaries stay).
_R = lambda a, b: set(range(a, b + 1))
DEFERRED = (_R(144, 146) | {150, 151} | _R(243, 245) | _R(249, 251) | _R(377, 386) | _R(480, 494) | _R(638, 649)
            | _R(716, 721) | {772, 773} | _R(785, 809) | _R(888, 898) | {905} | _R(984, 995) | _R(1001, 1010)
            | _R(1014, 1017) | _R(1020, 1025))
ordinary_missing = [n for n in range(1, 1026) if n not in DEFERRED and n not in obt_nats]
print('Ordinary roster (National Dex minus %d deferred legendary/mythical/UB/Paradox): %d of %d obtainable%s' % (
    len(DEFERRED), 1025 - len(DEFERRED) - len(ordinary_missing), 1025 - len(DEFERRED),
    '' if not ordinary_missing else ' (missing No %s)' % ', '.join(map(str, ordinary_missing))))
if ordinary_missing:
    issues.append('ordinary species with no path: %s' % ordinary_missing)
# The AREA hint is one line in FONT_SMALL_NARROW in a 28-tile window.
_fsrc = read('src/fonts.c')
_ftab = _fsrc[_fsrc.index('gFontSmallNarrowLatinGlyphWidths[] = {'):]
_ftab = [int(x) for x in re.findall(r'\d+', _ftab[_ftab.index('{') + 1:_ftab.index('};')])]
_fchar = {}
for _line in read('charmap.txt').splitlines():
    _m = re.match(r"^'(.)'\s*=\s*([0-9A-Fa-f]{2})\s*$", _line)
    if _m and _m.group(1) not in _fchar:
        _fchar[_m.group(1)] = int(_m.group(2), 16)
for _n, _h in hints:
    _w = sum(_ftab[_fchar[c]] if c in _fchar else 6 for c in _h)
    if _w > 28 * 8:
        issues.append('AREA hint too wide (%d px): %s' % (_w, _h))
missing_hint = [e['species'] for e in entries if e['dex_area'] == 'acquisition hint' and 'dex_hint' not in e]
print('AREA hints: %d (missing %d)' % (len(hints), len(missing_hint)))
for i in issues:
    print('ISSUE', i)
for m in missing_hint:
    print('ISSUE no AREA hint:', m)
print('%d issue(s)' % (len(issues) + len(missing_hint)))
sys.exit(1 if (issues or missing_hint) and CHECK else 0)
