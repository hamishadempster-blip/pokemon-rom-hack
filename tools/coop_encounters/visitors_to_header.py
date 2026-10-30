#!/usr/bin/env python3
"""src/data/coop_visitors.json -> src/data/coop_visitors.h (run by make).

Checks every entry against the sources and fails on a mistake: an unknown
map, species, area, condition or rod; a map without that kind of wild
table; a visitor already in that table (it would add nothing); a species
with no hint; a report town or map that does not exist.
"""
import json, os, re, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SRC = os.path.join(ROOT, 'src/data/coop_visitors.json')
OUT = os.path.join(ROOT, 'src/data/coop_visitors.h')

AREAS = {'land': ('COOP_VISIT_LAND', 'land_mons'), 'surf': ('COOP_VISIT_SURF', 'water_mons'),
         'fish': ('COOP_VISIT_FISH', 'fishing_mons'), 'rock': ('COOP_VISIT_ROCK', 'rock_smash_mons')}
WHEN = {'morning': 'COOP_WHEN_MORNING', 'day': 'COOP_WHEN_DAY', 'evening': 'COOP_WHEN_EVENING',
        'night': 'COOP_WHEN_NIGHT', 'rain': 'COOP_WHEN_RAIN', 'sand': 'COOP_WHEN_SAND', 'ash': 'COOP_WHEN_ASH'}
RODS = {'any': 'COOP_ROD_ANY', 'old': 'COOP_ROD_OLD', 'good': 'COOP_ROD_GOOD', 'super': 'COOP_ROD_SUPER'}
# Fishing table slots by rod (include/constants/wild_encounter.h groups).
ROD_SLOTS = {'old': range(0, 2), 'good': range(2, 5), 'super': range(5, 10), 'any': range(0, 10)}


def read(p):
    return open(os.path.join(ROOT, p), encoding='utf-8').read()


def fail(msg):
    sys.exit('coop_visitors.json: ' + msg)


WHEN_TEXT = {'morning': 'MORNING', 'day': 'DAY', 'evening': 'EVENING', 'night': 'NIGHT',
             'rain': 'RAIN', 'sand': 'SANDSTORM', 'ash': 'falling ASH'}
PREFIX = {('surf', 'any'): 'Surfing: ', ('fish', 'any'): 'Fishing: ', ('fish', 'old'): 'OLD ROD: ',
          ('fish', 'good'): 'GOOD ROD: ', ('fish', 'super'): 'SUPER ROD: ', ('rock', 'any'): 'Rocks: '}


def map_name(m):
    m = m[4:]
    r = re.match(r'ROUTE(\d+)$', m)
    return 'ROUTE %s' % r.group(1) if r else m.replace('_', ' ')


def check_report(rows, reports, hints):
    """The report's pages (src/coop_encounters.c) fit the message box and
    the page buffer: names counted in capitals (the widest)."""
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    from coop_text_width import width
    for sec, ms in reports.items():
        pages = 0
        for m in ms:
            groups = {}
            for r in rows:
                if r['map'] == m:
                    groups.setdefault((r['when'], r['area'], r.get('rod', 'any')), [])
                    g = groups[(r['when'], r['area'], r.get('rod', 'any'))]
                    if r['species'] not in g:
                        g.append(r['species'])
            for (when, area, rod), sp in groups.items():
                pages += 1
                if len(sp) > 3:
                    fail('%s %s %s: more than 3 visitors on one report page' % (m, when, area))
                line1 = '%s, %s:' % (map_name(m), WHEN_TEXT[when])
                pre = PREFIX.get((area, rod), '')
                line2 = pre + ', '.join(x[8:].replace('_', ' ') for x in sp)
                lines = [line1, line2]
                if len(sp) == 1:
                    lines.append(pre + 'rumor of ' + hints[sp[0]])
                for line in lines:
                    if width(line) > 208:
                        fail('report line too wide (%d px): %s' % (width(line), line))
        if pages > 16:
            fail('report %s: %d pages (16 at most)' % (sec, pages))


def main():
    data = json.load(open(SRC, encoding='utf-8'))
    species = set(re.findall(r'\b(SPECIES_\w+)\s*=', read('include/constants/species.h')))
    maps = set(re.findall(r'^\s*(MAP_\w+)\s*=', read('include/constants/map_groups.h'), re.M))
    mapsecs = set(re.findall(r'^\s*(MAPSEC_\w+)\s*[,=]', read('include/constants/region_map_sections.h'), re.M))
    wild = {}
    for e in json.load(open(os.path.join(ROOT, 'src/data/wild_encounters.json')))['wild_encounter_groups'][0]['encounters']:
        if e.get('map') and e['base_label'].startswith('g'):
            wild[e['map']] = e
    chance = int(data.get('chance_percent', 25))
    if not 1 <= chance <= 100:
        fail('chance_percent must be 1-100')
    hints = data['hints']
    rows = []
    for i, v in enumerate(data['visitors']):
        where = 'visitor %d (%s %s)' % (i, v.get('map'), v.get('species'))
        if v['map'] not in maps:
            fail(where + ': unknown map')
        if v['species'] not in species:
            fail(where + ': unknown species')
        if v['area'] not in AREAS:
            fail(where + ': unknown area')
        if v['when'] not in WHEN:
            fail(where + ': unknown condition')
        rod = v.get('rod', 'any')
        if rod not in RODS or (rod != 'any' and v['area'] != 'fish'):
            fail(where + ': bad rod')
        table = wild.get(v['map'], {}).get(AREAS[v['area']][1])
        if not table:
            fail(where + ': the map has no %s table' % v['area'])
        slots = table['mons']
        if v['area'] == 'fish':
            slots = [slots[j] for j in ROD_SLOTS[rod] if j < len(slots)]
        if any(s['species'] == v['species'] for s in slots):
            fail(where + ': already in that table')
        if v['species'] not in hints or not 0 < len(hints[v['species']]) <= 24:
            fail(where + ': no hint (1-24 characters)')
        rows.append(v)
    reports = data['reports']
    for sec, ms in reports.items():
        if sec not in mapsecs:
            fail('report %s: unknown MAPSEC' % sec)
        if not 0 < len(ms) <= 4:
            fail('report %s: 1-4 maps' % sec)
        for m in ms:
            if m not in maps:
                fail('report %s: unknown map %s' % (sec, m))
            if not any(r['map'] == m for r in rows):
                fail('report %s: %s has no visitors' % (sec, m))
    rows.sort(key=lambda r: (r['map'], r['area'], r['when']))
    check_report(rows, reports, hints)

    out = ['// DO NOT MODIFY THIS FILE! It is generated by tools/coop_encounters/visitors_to_header.py',
           '// from src/data/coop_visitors.json.', '',
           '#define COOP_VISITOR_CHANCE %d' % chance, '']
    names = {}
    for sp in sorted(set(r['species'] for r in rows)):
        n = 'sCoopVisitorHint_' + sp[8:].title().replace('_', '')
        names[sp] = n
        out.append('static const u8 %s[] = _("%s");' % (n, hints[sp]))
    out += ['', 'static const struct CoopVisitor sCoopVisitors[] =', '{']
    for r in rows:
        out.append('    { %s, %s, %s, %s, %s, %s },' % (r['map'], r['species'], AREAS[r['area']][0], WHEN[r['when']],
                                                    RODS[r.get('rod', 'any')], names[r['species']]))
    out += ['};', '', 'static const struct CoopVisitorReport sCoopVisitorReports[] =', '{']
    for sec, ms in reports.items():
        out.append('    { %s, { %s } },' % (sec, ', '.join(ms + ['MAP_UNDEFINED'] * (4 - len(ms)))))
    out += ['};', '']
    text = '\n'.join(out)
    open(OUT, 'w', encoding='utf-8').write(text)


main()
