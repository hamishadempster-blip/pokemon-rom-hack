#!/usr/bin/env python3
"""Audit the later-generation Pokémon this hack adds to Hoenn.

The intended list is the "New Pokémon" table in docs/COOP.md plus each one's
evolution line. Everything else the expansion engine supports is out of scope.
Species data is read from the built ROM (pokeemerald.gba + .elf) through this
build's struct layout (build_layout.sh), so it checks what actually shipped:
enabled entry, name, stats, types, abilities, graphics (not the "?"
placeholder), cry, Pokédex text, learnset, evolutions. Obtainability comes
from the sources: wild tables, in-game trades, and where each evolution item
is placed, hidden, sold, or given.

    tools/coop_roster_audit/build_layout.sh && tools/coop_roster_audit/audit.py
"""
import glob, json, os, re, struct, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/coop_roster_audit'


def read(path):
    return open(os.path.join(ROOT, path), encoding='utf-8').read()


def enum_values(path, prefix, block=None):
    """NAME = expr, entries of the enums in a header; expressions may name
    any earlier entry (ABILITY_TANGLED_FEET = ABILITIES_COUNT_GEN3).
    block limits the search to one named enum."""
    allv, vals, nxt = {}, {}, 0
    src = read(path)
    if block:
        src = src[src.index('enum ' + block):]
        src = src[:src.index('};')]
    for m in re.finditer(r'^\s*([A-Z][A-Z0-9_]*)\s*(?:=\s*([^,\n]+))?,', src, re.M):
        name, expr = m.group(1), m.group(2)
        if expr is not None:
            expr = expr.split('//')[0].strip()
            try:
                nxt = int(eval(re.sub(r'\b([A-Z][A-Z0-9_]*)\b', lambda k: str(allv.get(k.group(1), 0)), expr)))
            except Exception:
                continue
        allv[name] = nxt
        if name.startswith(prefix):
            vals[name] = nxt
        nxt += 1
    return vals


SPECIES = enum_values('include/constants/species.h', 'SPECIES_')
SPECIES_NAME = {v: k for k, v in SPECIES.items() if not k.startswith('SPECIES_EGG')}
ITEMS = enum_values('include/constants/items.h', 'ITEM_')
ITEM_NAME = {v: k for k, v in ITEMS.items()}
TYPES = {v: k[5:].title() for k, v in enum_values('include/constants/pokemon.h', 'TYPE_').items()}
ABILITY_NAME = {v: k[8:].replace('_', ' ').title() for k, v in enum_values('include/constants/abilities.h', 'ABILITY_').items()}
EVO = {v: k for k, v in enum_values('include/constants/pokemon.h', 'EVO_', 'EvolutionMethods').items()}
COND = {v: k for k, v in enum_values('include/constants/pokemon.h', 'IF_', 'EvolutionConditions').items()}
MOVES_COUNT = enum_values('include/constants/moves.h', 'MOVE').get('MOVES_COUNT_ALL') or enum_values('include/constants/moves.h', 'MOVE').get('MOVES_COUNT', 2000)

# Layout written by build_layout.sh from this build's headers.
L = struct.unpack('<21I', open(os.path.join(OUT, 'layout.bin'), 'rb').read())
(SI_SIZE, O_HP, O_TYPES, O_ABIL, O_NAME, O_DESC, O_FRONT, O_BACK, O_PAL, O_SHINY, O_ICON,
 O_LEARN, O_TEACH, O_EVOS, O_CATCH, EVO_SIZE, EVO_METHOD, EVO_PARAM, EVO_TARGET, LUM_SIZE, O_HEIGHT) = L
# cryId:16 and natDexNum:16 are bitfields after the 13-byte name: the cry
# starts at the byte after the name (unaligned), the dex number is aligned.
O_CRY = O_NAME + 13
O_NATDEX = O_HEIGHT - 2

syms = {}
for line in open(os.path.join(OUT, 'syms.txt')):
    p = line.split()
    if len(p) >= 3:
        syms[p[-1]] = int(p[0], 16)
rom = open(os.path.join(ROOT, 'pokeemerald.gba'), 'rb').read()


def r8(a): return rom[a - 0x08000000]
def r16(a): return struct.unpack_from('<H', rom, a - 0x08000000)[0]
def r32(a): return struct.unpack_from('<I', rom, a - 0x08000000)[0]


CHARS = {0x00: ' ', 0xAB: '!', 0xAC: '?', 0xAD: '.', 0xAE: '-', 0xB4: "'", 0xB8: ',', 0x1B: 'é', 0xBA: '/', 0xFE: ' '}
def text(a, limit=200):
    s = ''
    for i in range(limit):
        c = r8(a + i)
        if c == 0xFF:
            break
        if 0xBB <= c <= 0xD4: s += chr(ord('A') + c - 0xBB)
        elif 0xD5 <= c <= 0xEE: s += chr(ord('a') + c - 0xD5)
        elif 0xA1 <= c <= 0xAA: s += chr(ord('0') + c - 0xA1)
        else: s += CHARS.get(c, '~')
    return s


PLACEHOLDER = {syms.get(n) for n in ('gMonFrontPic_CircledQuestionMark', 'gMonBackPic_CircledQuestionMark',
                                     'gMonPalette_CircledQuestionMark', 'gMonShinyPalette_CircledQuestionMark',
                                     'gMonIcon_QuestionMark')}
SPECIES_INFO = syms['gSpeciesInfo']


def info(sid):
    a = SPECIES_INFO + sid * SI_SIZE
    d = {'id': sid, 'const': SPECIES_NAME.get(sid, '?')}
    d['stats'] = [r8(a + O_HP + i) for i in range(6)]  # HP Atk Def Spe SpA SpD
    d['types'] = [TYPES.get(r8(a + O_TYPES + i), '?') for i in range(2)]
    d['abilities'] = [ABILITY_NAME.get(r16(a + O_ABIL + 2 * i), '?') for i in range(3)]
    d['name'] = text(a + O_NAME, 13)
    d['cry'] = r8(a + O_CRY) | (r8(a + O_CRY + 1) << 8)
    d['natdex'] = r16(a + O_NATDEX)
    desc = r32(a + O_DESC)
    d['desc'] = text(desc) if 0x08000000 <= desc < 0x0A000000 else ''
    d['gfx'] = {k: r32(a + o) for k, o in (('front', O_FRONT), ('back', O_BACK), ('palette', O_PAL),
                                          ('shiny', O_SHINY), ('icon', O_ICON))}
    learn = r32(a + O_LEARN)
    moves = []
    if 0x08000000 <= learn < 0x0A000000:
        for i in range(120):
            mv, lv = struct.unpack_from('<HH', rom, learn - 0x08000000 + i * LUM_SIZE)
            if mv == 0xFFFF:
                break
            moves.append((lv, mv))
    d['learnset'] = moves
    evos = []
    e = r32(a + O_EVOS)
    if 0x08000000 <= e < 0x0A000000:
        for i in range(12):
            base = e + i * EVO_SIZE
            method = r16(base + EVO_METHOD)
            if method == 0xFFFF:
                break
            conds = []
            params = r32(base + 8)
            if 0x08000000 <= params < 0x0A000000:
                for j in range(6):
                    c, a1, a2, a3 = struct.unpack_from('<HHHH', rom, params - 0x08000000 + j * 8)
                    if COND.get(c) is None or c == 0xFFFF or COND.get(c, '') == 'CONDITIONS_END':
                        break
                    conds.append((COND.get(c, c), a1))
            evos.append((EVO.get(method, method), r16(base + EVO_PARAM), r16(base + EVO_TARGET), conds))
    d['evos'] = evos
    return d


# --- intended list: the docs table, plus whole evolution lines --------------
doc = read('docs/COOP.md')
table = doc[doc.index('| New Pokémon | Where | Level |'):]
table = table[:table.index('\n\n')]
intended = []
for row in table.splitlines()[2:]:
    cells = [c.strip() for c in row.strip('|').split('|')]
    name = re.sub(r'\s*\(.*\)', '', cells[0])
    const = 'SPECIES_' + re.sub(r'[^A-Z0-9]', '_', name.upper().replace('-', '_')).strip('_')
    # A form in brackets, such as "Rockruff (Own Tempo)", is its own species.
    paren = re.search(r'\((.*)\)', cells[0])
    if paren:
        form = const + '_' + re.sub(r'[^A-Z0-9]', '_', paren.group(1).upper()).strip('_')
        if form in SPECIES:
            const = form
    const = {'SPECIES_JANGMO_O': 'SPECIES_JANGMO_O', 'SPECIES_INDEEDEE': 'SPECIES_INDEEDEE_M'}.get(const, const)
    if const == 'SPECIES_INDEEDEE_M' and 'female' in cells[0]:
        const = 'SPECIES_INDEEDEE_F'
    intended.append((const, cells[0], cells[1], cells[2]))

prevo = {}
for sid in range(1, max(SPECIES_NAME) + 1):
    try:
        for m, p, t, c in info(sid)['evos']:
            prevo.setdefault(t, sid)
    except Exception:
        pass


# Evolutions that cannot happen in Hoenn, by design: region-locked forms
# (Hisuian Lilligant) and totem forms with no method. Reported, not audited.
EXCLUDED = {}


def reachable_evo(m, conds):
    if m == 'EVO_NONE':
        return 'no method (totem form)'
    for c, a in conds:
        if c == 'IF_REGION':
            return 'needs another region'
    return None


def line_of(sid):
    root = sid
    while root in prevo:
        root = prevo[root]
    out, todo = [], [root]
    while todo:
        s = todo.pop(0)
        if s in out:
            continue
        out.append(s)
        for m, _, t, conds in info(s)['evos']:
            why = reachable_evo(m, conds)
            if why:
                EXCLUDED[t] = why
            elif t not in out:
                todo.append(t)
    return out


# Forms reached with a key item from the bag, and where the item comes from.
FORM_ITEMS = {
    'SPECIES_ROTOM': ('ITEM_ROTOM_CATALOG', ['SPECIES_ROTOM_HEAT', 'SPECIES_ROTOM_WASH', 'SPECIES_ROTOM_FROST',
                                             'SPECIES_ROTOM_FAN', 'SPECIES_ROTOM_MOW']),
}


# --- obtainability ---------------------------------------------------------
wild = {}
enc = json.loads(read('src/data/wild_encounters.json'))
for g in enc['wild_encounter_groups']:
    for e in g['encounters']:
        for kind, v in e.items():
            if isinstance(v, dict) and 'mons' in v:
                for m in v['mons']:
                    wild.setdefault(SPECIES_NAME.get(SPECIES.get(m['species'], -1), m['species']), set()).add((e.get('map', e.get('base_label')), kind.replace('_mons', ''), m['min_level'], m['max_level']))

trades = {SPECIES_NAME.get(SPECIES.get(t, -1), t) for t in re.findall(r'\.species\s*=\s*(SPECIES_\w+)', read('src/data/trade.h'))}

item_sources = {}
for mj in glob.glob(os.path.join(ROOT, 'data/maps/*/map.json')):
    m = json.load(open(mj))
    mapname = os.path.basename(os.path.dirname(mj))
    for o in m.get('object_events', []):
        if o.get('graphics_id') == 'OBJ_EVENT_GFX_ITEM_BALL':
            item_sources.setdefault(o.get('trainer_sight_or_berry_tree_id'), set()).add('ball: ' + mapname)
    for b in m.get('bg_events', []):
        if b.get('type') == 'hidden_item':
            item_sources.setdefault(b.get('item'), set()).add('hidden: ' + mapname)
for sc in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')) + glob.glob(os.path.join(ROOT, 'data/scripts/*.inc')):
    src = open(sc, encoding='utf-8').read()
    where = os.path.basename(os.path.dirname(sc)) if 'maps' in sc else os.path.basename(sc)
    for it in re.findall(r'giveitem\s+(ITEM_\w+)', src):
        item_sources.setdefault(it, set()).add('given: ' + where)
    for block in re.findall(r'pokemartlistend|\.2byte\s+ITEM_\w+[\s\S]*?pokemartlistend', src):
        pass
    for mart in re.findall(r'(?:pokemart\s+(\w+))', src):
        pass
    for it in re.findall(r'\.2byte\s+(ITEM_\w+)', src):
        item_sources.setdefault(it, set()).add('mart: ' + where)
    for it in re.findall(r'pokemartlistentry\s+(ITEM_\w+)|^\s*(ITEM_\w+)\s*$', src, re.M):
        pass


ITEM_ALIASES = {}
for k, v in ITEMS.items():
    ITEM_ALIASES.setdefault(v, []).append(k)


def sources_for(item):
    out = set()
    for name in ITEM_ALIASES.get(ITEMS.get(item, -1), [item]):
        out |= item_sources.get(name, set())
    return out


def placeholder_gfx(d):
    return [k for k, v in d['gfx'].items() if v in PLACEHOLDER or not (0x08000000 <= v < 0x0A000000)]


def fmt_evo(m, p, t, conds):
    tgt = SPECIES_NAME.get(t, t).replace('SPECIES_', '').title()
    if m == 'EVO_ITEM':
        how = ITEM_NAME.get(p, p).replace('ITEM_', '').replace('_', ' ').title()
    elif m in ('EVO_LEVEL', 'EVO_LEVEL_BATTLE_ONLY'):
        how = f'Lv {p}' if p else 'level up'
    elif m == 'EVO_TRADE':
        how = 'trade'
    else:
        how = m.replace('EVO_', '').lower()
    extra = []
    for c, a in conds:
        if c == 'IF_HOLD_ITEM':
            extra.append('holding ' + ITEM_NAME.get(a, a).replace('ITEM_', '').replace('_', ' ').title())
        elif c == 'IF_KNOWS_MOVE':
            extra.append('knows move %d' % a)
        elif c in ('IF_MIN_FRIENDSHIP',):
            extra.append('friendship')
        elif c in ('IF_GENDER',):
            extra.append('gender')
        elif c == 'IF_TIME':
            extra.append('time %d' % a)
        else:
            extra.append(c.replace('IF_', '').lower())
    return f'{how}{" (" + ", ".join(extra) + ")" if extra else ""} → {tgt}'


def items_needed(evos):
    out = []
    for m, p, t, conds in evos:
        if m == 'EVO_ITEM':
            out.append(ITEM_NAME.get(p, p))
        for c, a in conds:
            if c in ('IF_HOLD_ITEM', 'IF_BAG_ITEM_COUNT'):
                out.append(ITEM_NAME.get(a, a))
            if c == 'IF_DEFEAT_X_WITH_ITEMS':
                out.append('wild Pokemon holding item %d' % a)
    return out


hoenn = re.findall(r'F\((\w+)\)', read('include/constants/pokedex.h')[read('include/constants/pokedex.h').index('FOREACH_SPECIES_IN_HOENN_DEX_ORDER'):])
hoenn_index = {('SPECIES_' + n): i + 1 for i, n in enumerate(hoenn)}
hoenn_by_natdex = {}
for n, i in hoenn_index.items():
    if n in SPECIES:
        hoenn_by_natdex.setdefault(info(SPECIES[n])['natdex'], i)

rows, issues, seen = [], [], set()
for const, label, where, level in intended:
    sid = SPECIES.get(const)
    if sid is None:
        issues.append(f'{label}: no species constant {const}')
        continue
    line = line_of(sid)
    for base, (item, forms) in FORM_ITEMS.items():
        if SPECIES[base] in line:
            line += [SPECIES[f] for f in forms if SPECIES[f] not in line]
    for s in line:
        if s in seen:
            continue
        seen.add(s)
        d = info(s)
        c = d['const']
        probs = []
        if d['stats'][0] == 0 or not d['name'].strip():
            probs.append('not enabled (empty entry)')
        bad = placeholder_gfx(d)
        if bad:
            probs.append('placeholder ' + '/'.join(bad))
        if d['cry'] == 0:
            probs.append('no cry')
        if len(d['desc'].strip()) < 10:
            probs.append('no Pokédex text')
        if not d['learnset']:
            probs.append('no level-up moves')
        if any(mv >= MOVES_COUNT for _, mv in d['learnset']):
            probs.append('invalid move id in learnset')
        hd = hoenn_by_natdex.get(d['natdex'])
        if hd is None:
            probs.append('not in Hoenn Pokédex')
        how = []
        for (mp, kind, lo, hi) in sorted(wild.get(c, ())):
            how.append(f'{mp.replace("MAP_", "").title()} {kind} {lo}' + (f'–{hi}' if hi != lo else ''))
        if c in trades:
            how.append('in-game trade')
        if s != line_of(s)[0] and s in prevo:
            how.append('evolve ' + SPECIES_NAME[prevo[s]].replace('SPECIES_', '').title())
        for base, (item, forms) in FORM_ITEMS.items():
            if c in forms:
                src = sources_for(item)
                how.append(f'{item.replace("ITEM_", "").replace("_", " ").title()} on {base.replace("SPECIES_", "").title()}'
                           + (f' ({", ".join(sorted(src))})' if src else ''))
                if not src:
                    probs.append(f'{item} not obtainable')
        if not how and s == line_of(s)[0] and any(wild.get(SPECIES_NAME[o]) or SPECIES_NAME[o] in trades for o in line_of(s)):
            how.append('breed an evolved form (Day Care)')
        if not how and info(s)['natdex'] != info(line_of(s)[0])['natdex'] or (not how and s in prevo):
            pass
        if not how:
            probs.append('not obtainable')
        for it in items_needed(d['evos']):
            src = sources_for(it)
            if not src:
                probs.append(f'{it} not obtainable')
        rows.append((c, d, hd, how, probs))
        for p in probs:
            issues.append(f'{c}: {p}')

print('| Species | Hoenn # | Types | Abilities | HP/Atk/Def/SpA/SpD/Spe | How to get | Evolves | Check |')
print('| --- | --- | --- | --- | --- | --- | --- | --- |')
for c, d, hd, how, probs in rows:
    st = d['stats']
    stats = f'{st[0]}/{st[1]}/{st[2]}/{st[4]}/{st[5]}/{st[3]}'
    types = d['types'][0] + ('' if d['types'][1] == d['types'][0] else '/' + d['types'][1])
    abil = ' / '.join(a for a in d['abilities'] if a and a != 'None')
    evo = '; '.join(fmt_evo(*e) for e in d['evos']) or '—'
    print(f'| {d["name"]} | {hd or "—"} | {types} | {abil} | {stats} | {"; ".join(how) or "—"} | {evo} | {"OK" if not probs else "; ".join(probs)} |')
print()
if EXCLUDED:
    print('Forms left out on purpose (cannot be reached in Hoenn):')
    for t, why in sorted(EXCLUDED.items()):
        print(f'- {SPECIES_NAME.get(t, t).replace("SPECIES_", "").title()}: {why}')
    print()
print('Items those evolutions need, and where they come from:')
needed = sorted({it for _, d, _, _, _ in rows for it in items_needed(d['evos'])})
for it in needed:
    print(f'- {it}: {", ".join(sorted(sources_for(it))) or "NOT OBTAINABLE"}')
print()
# The retuned Hoenn Pokémon: the docs' "After" stats and abilities against the ROM.
ret = doc[doc.index('| Pokémon | Before | After | What changed |'):]
ret = ret[:ret.index('\n\n')]
print('Retuned Hoenn Pokémon (docs "After" column against the ROM):')
for row in ret.splitlines()[2:]:
    cells = [c.strip() for c in row.strip('|').split('|')]
    const = 'SPECIES_' + cells[0].upper()
    if const not in SPECIES:
        issues.append(f'retune {cells[0]}: unknown species')
        continue
    d = info(SPECIES[const])
    want = cells[2].split(',')[0]
    if 'across' in want:
        want_stats = None
    else:
        want_stats = [int(x) for x in want.split('/')]
    st = d['stats']
    have = [st[0], st[1], st[2], st[4], st[5], st[3]]
    abil_want = [a.strip() for a in cells[2].split(',', 1)[1].split('/')]
    abil_have = [a for a in d['abilities'] if a and a != 'None']
    ok_stats = want_stats is None or want_stats == have
    ok_abil = [a.lower().replace("-", " ") for a in abil_want] == [a.lower() for a in abil_have]
    flag = 'OK' if ok_stats and ok_abil else 'MISMATCH'
    if flag != 'OK':
        issues.append(f'retune {cells[0]}: ROM {have} {abil_have}, docs {want} {abil_want}')
    print(f'- {cells[0]}: {"/".join(map(str, have))}, {" / ".join(abil_have)} — {flag}')
print()
print(f'{len(rows)} species checked; {len(issues)} issue(s).')
for i in issues:
    print('ISSUE', i)
