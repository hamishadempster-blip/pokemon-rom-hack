#!/usr/bin/env python3
"""Obtainability audit for the co-op roster batches.

Reads the wild encounter tables, scripts and C sources for gifts and
scripted encounters, and the evolution data, then reports for each species
of a batch how it can be obtained in this ROM (or that it cannot).
Usage: audit.py [BATCH]   (default: batch1, the 114-species first expansion)
"""
import json, os, re, sys, glob

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

BATCH1 = """BULBASAUR IVYSAUR VENUSAUR CHARMANDER CHARMELEON CHARIZARD SQUIRTLE WARTORTLE BLASTOISE
TURTWIG GROTLE TORTERRA CHIMCHAR MONFERNO INFERNAPE PIPLUP PRINPLUP EMPOLEON
SNIVY SERVINE SERPERIOR TEPIG PIGNITE EMBOAR OSHAWOTT DEWOTT SAMUROTT
CHESPIN QUILLADIN CHESNAUGHT FENNEKIN BRAIXEN DELPHOX FROAKIE FROGADIER GRENINJA
FLETCHLING FLETCHINDER TALONFLAME STARLY STARAVIA STARAPTOR GIBLE GABITE GARCHOMP
AXEW FRAXURE HAXORUS MAREANIE TOXAPEX DEWPIDER ARAQUANID SANDYGAST PALOSSAND
FRILLISH JELLICENT CROAGUNK TOXICROAK SKORUPI DRAPION GRUBBIN CHARJABUG VIKAVOLT
CUTIEFLY RIBOMBEE BOUNSWEET STEENEE TSAREENA PHANTUMP TREVENANT PUMPKABOO GOURGEIST
SWINUB PILOSWINE MAMOSWINE SNEASEL WEAVILE GROWLITHE ARCANINE PONYTA RAPIDASH
DRATINI DRAGONAIR DRAGONITE LARVITAR PUPITAR TYRANITAR SHELLOS GASTRODON
DRIFLOON DRIFBLIM ZORUA ZOROARK MINCCINO CINCCINO FERROSEED FERROTHORN
TYNAMO EELEKTRIK EELEKTROSS TIMBURR GURDURR CONKELDURR VENONAT VENOMOTH
PARAS PARASECT HOPPIP SKIPLOOM JUMPLUFF DROWZEE HYPNO MURKROW HONCHKROW""".split()

def read(p):
    with open(p, encoding='utf-8', errors='replace') as f:
        return f.read()

def reachable_maps():
    """Maps reachable from Littleroot by connections, warp events and
    scripted warps (ferries, Fly, Dive and Teleport destinations are maps
    found the same way)."""
    maps, idmap = {}, {}
    for p in glob.glob(os.path.join(ROOT, 'data/maps/*/map.json')):
        if os.path.basename(os.path.dirname(p)).endswith('_Frlg'):
            continue  # Kanto maps: FireRed/LeafGreen builds only
        d = json.load(open(p))
        maps[d['id']] = (os.path.dirname(p), d)
    edges = {}
    for mid, (folder, d) in maps.items():
        out = set()
        for c in d.get('connections') or []:
            out.add(c['map'])
        for w in d.get('warp_events') or []:
            out.add(w['dest_map'])
        sp = os.path.join(folder, 'scripts.inc')
        if os.path.exists(sp):
            out |= set(re.findall(r'\b(?:warp\w*|setwarp|setdynamicwarp|setescapewarp|setholewarp|setdivewarp)\s+(MAP_[A-Z0-9_]+)', read(sp)))
        edges[mid] = out
    # Shared scripts (ferry, Fly destinations via heal locations, etc.).
    shared = set()
    for p in glob.glob(os.path.join(ROOT, 'data/scripts/*.inc')):
        if 'debug' in p or 'frlg' in p.lower():
            continue
        shared |= set(re.findall(r'\b(?:warp\w*|setwarp|setdynamicwarp)\s+(MAP_[A-Z0-9_]+)', read(p)))
    seen, todo = set(), ['MAP_LITTLEROOT_TOWN'] + sorted(shared)
    while todo:
        m = todo.pop()
        if m in seen or m not in maps:
            continue
        seen.add(m)
        todo.extend(edges.get(m, ()))
    return seen

def wild_sources():
    d = json.load(open(os.path.join(ROOT, 'src/data/wild_encounters.json')))
    reach = reachable_maps()
    found = {}
    for g in d['wild_encounter_groups']:
        fields = [f['type'] for f in g.get('fields', [])]
        for e in g['encounters']:
            where = e.get('map') or e.get('base_label')
            if not e.get('map') or e['map'] not in reach:
                continue  # unreachable maps; Battle Pyramid/Pike (nothing kept)
            for key, val in e.items():
                if not key.endswith('_mons') or not isinstance(val, dict):
                    continue
                for slot in val.get('mons', []):
                    sp = slot['species'].replace('SPECIES_', '')
                    found.setdefault(sp, set()).add('%s %s Lv%s-%s' % (where.replace('MAP_', ''), key.replace('_mons', ''), slot['min_level'], slot['max_level']))
    return found

GIFT_RE = re.compile(r'\b(givemon|setwildbattle|createmon|givecustommon|seteggmon|giveegg|ScriptGiveMon|CreateScriptedWildMon|CreateScriptedDoubleWildMon|setdynamicwildbattle)\b[^\n]*?SPECIES_([A-Z0-9_]+)')

def scripted_sources():
    found = {}
    files = glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')) + glob.glob(os.path.join(ROOT, 'data/scripts/*.inc')) \
          + glob.glob(os.path.join(ROOT, 'src/*.c')) + glob.glob(os.path.join(ROOT, 'src/data/*.h'))
    for p in files:
        if 'debug' in os.path.basename(p) or '_Frlg' in p or 'frlg' in os.path.basename(p):
            continue
        txt = read(p)
        for m in GIFT_RE.finditer(txt):
            found.setdefault(m.group(2), set()).add('%s (%s)' % (os.path.relpath(p, ROOT), m.group(1)))
    # Co-op partner Pokemon (renewable starter guests, src/coop_roster.c)
    roster = os.path.join(ROOT, 'src/coop_roster.c')
    if os.path.exists(roster):
        for m in re.finditer(r'\[COOP_STARTER_GUEST_(\w+)\]\s*=\s*\{\s*COMPOUND_STRING[^{]*\{([^}]*)\}', read(roster)):
            for sp in re.findall(r'SPECIES_([A-Z0-9_]+)', m.group(2)):
                found.setdefault(sp, set()).add('partner Pokemon (%s guest)' % m.group(1))
    # In-game trades
    trade = os.path.join(ROOT, 'src/data/trade.h')
    if os.path.exists(trade):
        for m in re.finditer(r'\.species\s*=\s*SPECIES_([A-Z0-9_]+)', read(trade)):
            found.setdefault(m.group(1), set()).add('in-game trade')
    return found

EVO_RE = re.compile(r'\[SPECIES_([A-Z0-9_]+)\]\s*=\s*\{(.*?)\n    \},', re.S)
def evolutions():
    evo = {}
    for p in glob.glob(os.path.join(ROOT, 'src/data/pokemon/species_info/gen_*_families.h')):
        txt = read(p)
        for m in EVO_RE.finditer(txt):
            sp, body = m.group(1), m.group(2)
            k = body.find('.evolutions')
            if k < 0:
                continue
            k = body.find('EVOLUTION(', k)
            depth, j = 0, k + len('EVOLUTION')
            while j < len(body):
                if body[j] == '(':
                    depth += 1
                elif body[j] == ')':
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            args = body[k + len('EVOLUTION('):j]
            # Top-level {...} groups (conditions nest their own braces).
            depth, start = 0, None
            for i, ch in enumerate(args):
                if ch == '{':
                    if depth == 0:
                        start = i + 1
                    depth += 1
                elif ch == '}':
                    depth -= 1
                    if depth == 0 and start is not None:
                        t = re.match(r'\s*(EVO_[A-Z_]+),\s*([^,]*),\s*SPECIES_([A-Z0-9_]+)(.*)', args[start:i], re.S)
                        if t:
                            evo.setdefault(sp, []).append((t.group(1), t.group(2).strip(), t.group(3), t.group(4).strip(' ,\n')))
    return evo

# Batch names that stand for a species' ordinary forms (no regional forms).
FORMS = {'SHELLOS': ('SHELLOS_WEST', 'SHELLOS_EAST'), 'GASTRODON': ('GASTRODON_WEST', 'GASTRODON_EAST'),
         'PUMPKABOO': ('PUMPKABOO_AVERAGE', 'PUMPKABOO_SMALL', 'PUMPKABOO_LARGE', 'PUMPKABOO_SUPER'),
         'GOURGEIST': ('GOURGEIST_AVERAGE', 'GOURGEIST_SMALL', 'GOURGEIST_LARGE', 'GOURGEIST_SUPER')}

def fold_forms(table):
    for base, forms in FORMS.items():
        for f in forms:
            if f in table:
                table.setdefault(base, set()).update('%s: %s' % (f, x) for x in table[f])
    return table

def main():
    batch = BATCH1
    wild, scripted, evo = fold_forms(wild_sources()), fold_forms(scripted_sources()), evolutions()
    for base, forms in FORMS.items():
        for f in forms:
            for e in evo.get(f, []):
                b = e[2]
                for b2, fs in FORMS.items():
                    if b in fs: b = b2
                evo.setdefault(base, []).append((e[0], e[1], b, e[3]))
    pre = {}
    for a, lst in evo.items():
        for (kind, param, b, extra) in lst:
            pre.setdefault(b, []).append((a, kind, param, extra))
    direct = {s for s in batch if s in wild or s in scripted}
    def reach(s, seen=()):
        if s in wild or s in scripted:
            return True
        return any(reach(a, seen + (s,)) for (a, _, _, _) in pre.get(s, []) if a not in seen)
    for s in batch:
        how = sorted(wild.get(s, set()))[:4] + sorted(scripted.get(s, set()))[:3]
        if not how and reach(s):
            how = ['evolves from ' + ', '.join(sorted(set('%s (%s %s %s)' % (a, k, p, x) for (a, k, p, x) in pre.get(s, []))))]
        print('%-12s %s' % (s, '; '.join(how) if how else 'NOT OBTAINABLE'))
    obt = [s for s in batch if reach(s)]
    print('\n%d of %d obtainable (%d with a direct source)' % (len(obt), len(batch), len(direct)))

if __name__ == '__main__':
    main()
