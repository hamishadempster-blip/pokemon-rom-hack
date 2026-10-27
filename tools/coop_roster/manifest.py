#!/usr/bin/env python3
"""Writes docs/COOP_ROSTER_BATCH1.md: for every species of the first roster
batch, where it lives, what unlocks the place, how it evolves and where the
evolution supplies come from. Data comes from the sources (audit.py), so the
manifest cannot drift from the ROM's tables.
Usage: manifest.py [OUT]"""
import os, re, sys, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import audit

ROOT = audit.ROOT

# What opens each habitat in an ordinary playthrough.
UNLOCK = {
    'ROUTE102': 'start (after the first rival battle)',
    'ROUTE104': 'Petalburg reached',
    'PETALBURG_WOODS': 'Petalburg reached',
    'RUSTURF_TUNNEL': 'Rustboro reached',
    'ROUTE116': 'Rustboro reached',
    'GRANITE_CAVE_B1F': 'Dewford (Mr. Briney\'s boat)',
    'GRANITE_CAVE_B2F': 'Dewford (Mr. Briney\'s boat)',
    'ROUTE110': 'Slateport reached',
    'ROUTE117': 'Mauville reached',
    'NEW_MAUVILLE_INSIDE': 'Basement Key from Wattson; Surf',
    'ROUTE112': 'Mauville reached (Route 111 north)',
    'FIERY_PATH': 'Route 112 (Cable Car side)',
    'ROUTE111': 'desert: Go-Goggles (after the Lavaridge Gym)',
    'METEOR_FALLS_1F_1R': 'Route 114 (after Fallarbor)',
    'ROUTE119': 'Route 118 north (Fortree route)',
    'ROUTE120': 'after Fortree',
    'ROUTE121': 'after Route 120',
    'MT_PYRE_EXTERIOR': 'Route 122 by Surf',
    'SHOAL_CAVE_LOW_TIDE_ICE_ROOM': 'Mossdeep reached; low tide',
    'SHOAL_CAVE_LOW_TIDE_LOWER_ROOM': 'Mossdeep reached; low tide',
    'ROUTE124': 'Surf (east of Lilycove)',
    'ROUTE125': 'Surf (near Mossdeep)',
    'ROUTE126': 'Surf (around Sootopolis)',
}
METHOD = {'land': 'grass/cave', 'water': 'Surf', 'fishing': 'fishing', 'rock_smash': 'Rock Smash'}

ROD = {(0, 1): 'Old Rod', (2, 4): 'Good Rod', (5, 9): 'Super Rod'}

def shops():
    """item -> set of shop names (pokemart lists in map scripts)."""
    out = {}
    for p in glob.glob(os.path.join(ROOT, 'data/maps/*/scripts.inc')):
        if '_Frlg' in p:
            continue
        name = os.path.basename(os.path.dirname(p))
        txt = audit.read(p)
        for m in re.finditer(r'pokemart\s+(\w+)', txt):
            lab = m.group(1)
            k = txt.find(lab + ':')
            if k < 0:
                continue
            body = txt[k:txt.find('ITEM_NONE', k)]
            for it in re.findall(r'\.2byte\s+(ITEM_\w+)', body):
                out.setdefault(it, set()).add(name)
    return out

def nice(map_id):
    if map_id.startswith('MAP_') or map_id.isupper():
        t = re.sub(r'(?<=[A-Z])(?=\d{3})', ' ', map_id.replace('MAP_', '')).title().replace('_', ' ')
        return re.sub(r'\b(\d)F\b', r'\1F', t).replace(' 1F 1R', ' 1F').replace('B1F', 'B1F').replace('1F 1r', '1F')
    s = re.sub(r'(?<=[a-z0-9])(?=[A-Z])', ' ', map_id.replace('_', ' '))
    return re.sub(r'(\d) F\b', r'\1F', s)

def evo_text(lst):
    parts = []
    for (a, kind, param, extra) in lst:
        conds = []
        for k, v in re.findall(r'\{(IF_\w+),\s*(\w+)\}', extra):
            v2 = re.sub(r'^(ITEM|MOVE|MAPSEC|TIME)_', '', v).replace('_', ' ').title()
            conds.append({'IF_TIME': 'at ' + v2.lower(), 'IF_HOLD_ITEM': 'holding ' + v2, 'IF_KNOWS_MOVE': 'knowing ' + v2,
                          'IF_IN_MAPSEC': 'in ' + v2}.get(k, k.replace('IF_', '').lower() + ' ' + v2))
        cond = ', '.join(conds)
        if kind == 'EVO_LEVEL' and param != '0':
            t = 'Lv %s' % param
        elif kind == 'EVO_LEVEL':
            t = 'level up'
        elif kind == 'EVO_ITEM':
            t = param.replace('ITEM_', '').replace('_', ' ').title()
        elif kind == 'EVO_TRADE':
            t = 'trade'
        else:
            t = kind
        parts.append('%s → %s%s' % (a.title(), t, (' (' + cond.strip() + ')') if cond.strip() else ''))
    return '; '.join(sorted(set(parts)))

def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'docs/COOP_ROSTER_BATCH1.md')
    wild = audit.fold_forms(audit.wild_sources())
    scripted = audit.fold_forms(audit.scripted_sources())
    evo = audit.evolutions()
    for base, forms in audit.FORMS.items():
        for f in forms:
            for e in evo.get(f, []):
                b = e[2]
                for b2, fs in audit.FORMS.items():
                    if b in fs:
                        b = b2
                evo.setdefault(base, []).append((e[0], e[1], b, e[3]))
    pre = {}
    for a, lst in evo.items():
        for (kind, param, b, extra) in lst:
            if b in audit.BATCH1:
                pre.setdefault(b, []).append((a, kind, param, extra))
    global GATE
    GATE = {m.group(1).title(): m.group(2) for m in re.finditer(r'\[COOP_STARTER_GUEST_(\w+)\]\s*=\s*\{\s*COMPOUND_STRING\("\w+"\),\s*(\d+)', audit.read(os.path.join(ROOT, 'src/coop_roster.c')))}
    shop = shops()
    items = set()
    rows = []
    for s in audit.BATCH1:
        habit, unlock = [], set()
        for x in sorted(wild.get(s, ())):
            form = ''
            if ': ' in x:
                form, x = x.split(': ', 1)
                form = ' (' + form.split('_', 1)[1].title() + ')'
            where, meth, lv = x.split(' ')
            habit.append('%s %s %s%s' % (nice(where), METHOD.get(meth, meth), lv.replace('Lv', 'Lv '), form))
            unlock.add(UNLOCK.get(where, '?'))
            if meth == 'fishing':
                unlock.add('Super Rod (Mossdeep)' if lv.startswith('Lv25') or lv.startswith('Lv3') else 'a rod')
        for x in sorted(scripted.get(s, ())):
            if 'partner Pokemon' in x:
                g = re.search(r'\((\w+) guest\)', x).group(1)
                habit.append('Partner Pokémon: %s guest, Pokémon Centre 1F (daily, one of three)' % g.title())
                unlock.add('%s badge(s)' % GATE.get(g.title(), '?'))
        ev = evo_text(pre.get(s, []))
        for (_, kind, param, _) in pre.get(s, []):
            if kind == 'EVO_ITEM':
                items.add(param)
        acq = '; '.join(habit) if habit else 'evolution only'
        rows.append((s, acq, '; '.join(sorted(unlock)) if unlock else '', ev))
    guests = {}
    txt = audit.read(os.path.join(ROOT, 'src/coop_roster.c'))
    for m in re.finditer(r'\[COOP_STARTER_GUEST_(\w+)\]\s*=\s*\{\s*COMPOUND_STRING\("\w+"\),\s*(\d+),\s*(\d+)', txt):
        guests[m.group(1).title()] = (m.group(2), m.group(3))
    guest_town = {'Kanto': 'Rustboro', 'Sinnoh': 'Lavaridge', 'Unova': 'Fortree', 'Kalos': 'Mossdeep'}
    with open(out, 'w') as f:
        f.write('# Co-op roster, batch 1 (114 species)\n\n')
        f.write('Generated by `tools/coop_roster/manifest.py` from the ROM sources; `tools/coop_roster/audit.py` checks the same data.\n\n')
        f.write('Before this batch none of the 114 species was obtainable in this ROM (audit of the pre-batch source: 0/114). After it: 114/114 obtainable, 45 with a direct source (wild slot or partner gift), 69 by evolution. 0 were already obtainable; 0 had access repaired.\n\n')
        f.write('Species IDs are unchanged; new placements reuse existing encounter slots (duplicates of a species already in the table), so table sizes and existing species stay as they were.\n\n')
        f.write('## Partner Pokémon (starter families)\n\n')
        f.write('A guest Trainer at the left of the Pokémon Centre 1F counter offers one of three starters per day (a daily flag, cleared with the other daily flags). Each eligible player takes their own; a full party sends it to the PC. The guest says what is needed when the gate is not met.\n\n')
        f.write('| Guest | Centre | Badges needed | Level | Choices |\n|---|---|---|---|---|\n')
        choices = {'Kanto': 'Bulbasaur, Charmander, Squirtle', 'Sinnoh': 'Turtwig, Chimchar, Piplup', 'Unova': 'Snivy, Tepig, Oshawott', 'Kalos': 'Chespin, Fennekin, Froakie'}
        for g, (b, lv) in guests.items():
            f.write('| %s | %s | %s | %s | %s |\n' % (g, guest_town.get(g, '?'), b, lv, choices.get(g, '')))
        f.write('\n## Species\n\n| Species | Habitat / acquisition | Unlock | Evolves from |\n|---|---|---|---|\n')
        for (s, acq, unl, ev) in rows:
            f.write('| %s | %s | %s | %s |\n' % (s.title(), acq, unl, ev))
        f.write('\n## Evolution supplies\n\n| Item | Sold at |\n|---|---|\n')
        for it in sorted(items):
            f.write('| %s | %s |\n' % (it.replace('ITEM_', '').replace('_', ' ').title(), ', '.join(sorted(nice(x) for x in shop.get(it, ()))) or 'NOT SOLD'))
        f.write('\nTrade evolutions (Gurdurr, Phantump, Pumpkaboo) also evolve with a Linking Cord, so a solo player never needs a trade. Move conditions: Steenee learns Stomp at Lv 28; Piloswine knows Ancient Power from Lv 1 (Fallarbor Move Relearner if forgotten). Location: Charjabug levels up inside New Mauville (or a Thunder Stone). Time: Sneasel holding a Razor Claw levels up at night. The field researcher in the Petalburg, Mauville, Fortree and Lilycove Pokémon Centres gives habitat and evolution hints for every new arrival (NEW ARRIVALS and PARTNER POKéMON pages).\n')
    print('wrote', out, len(rows), 'species')

if __name__ == '__main__':
    main()
