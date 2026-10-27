#!/usr/bin/env python3
"""Before/after table for a set of revamps: compares the species_info and
gen_9 level-up learnset sources at a git ref (before) with the working tree
(after). Usage: revamp_report.py REF NAME... > report.md"""
import os, re, subprocess, sys
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
STAT_KEYS = ('baseHP', 'baseAttack', 'baseDefense', 'baseSpAttack', 'baseSpDefense', 'baseSpeed')
GEN_DEFAULTS = {'P_UPDATED_STATS': 99}


def src(ref, path):
    if ref is None:
        return open(os.path.join(ROOT, path)).read()
    return subprocess.run(['git', '-C', ROOT, 'show', '%s:%s' % (ref, path)], capture_output=True, text=True, check=True).stdout


def species_block(ref, name):
    for f in sorted(os.listdir(os.path.join(ROOT, 'src/data/pokemon/species_info'))):
        t = src(ref, 'src/data/pokemon/species_info/' + f)
        k = t.find('[SPECIES_%s] =' % name)
        if k >= 0:
            return t[k:t.index('\n    },', k)]


def value(expr):
    # "P_UPDATED_STATS >= GEN_6 ? 90 : 80" -> 90 (this build uses the latest stats)
    m = re.match(r'\s*P_UPDATED_STATS\s*>=\s*GEN_\d+\s*\?\s*(\d+)\s*:\s*\d+', expr)
    return int(m.group(1)) if m else int(expr)


def summary(ref, name):
    b = species_block(ref, name)
    stats = [value(re.search(r'\.%s\s*=\s*([^,\n]+),' % k, b).group(1)) for k in STAT_KEYS]
    types = re.search(r'MON_TYPES\(([^)]*)\)', b).group(1).replace('TYPE_', '').title().replace(', ', '/')
    abil = [a.replace('ABILITY_', '').replace('_', ' ').title() for a in re.search(r'\.abilities = \{([^}]*)\}', b).group(1).replace(' ', '').split(',')]
    abil = ' / '.join(dict.fromkeys(a for a in abil if a != 'None'))
    return stats, types, abil


def learnset(ref, name):
    t = src(ref, 'src/data/pokemon/level_up_learnsets/gen_9.h')
    k = t.index('s%sLevelUpLearnset[] = {' % name.title())
    return [(int(l), m) for l, m in re.findall(r'LEVEL_UP_MOVE\(\s*(\d+),\s*MOVE_(\w+)\)', t[k:t.index('LEVEL_UP_END', k)])]


def nice(m):
    return m.replace('_', ' ').title()


def main():
    ref, names = sys.argv[1], sys.argv[2:]
    print('| Pokémon | Before (HP/Atk/Def/SpA/SpD/Spe) | After | Types | Abilities | Level-up moves |')
    print('|---|---|---|---|---|---|')
    for spec in names:
        name, _, ls = spec.partition(':')
        s0, t0, a0 = summary(ref, name)
        s1, t1, a1 = summary(None, name)
        types = t1 if t0 == t1 else '%s → **%s**' % (t0, t1)
        abil = a1 if a0 == a1 else '%s → **%s**' % (a0, a1)
        moves = '—'
        if ls != '-':
            l0, l1 = learnset(ref, ls or name), learnset(None, ls or name)
            m0 = {m: l for l, m in l0}
            m1 = {m: l for l, m in l1}
            ch = ['+%s %d' % (nice(m), l) for m, l in m1.items() if m not in m0]
            ch += ['−%s' % nice(m) for m in m0 if m not in m1]
            ch += ['%s %d→%d' % (nice(m), m0[m], l) for m, l in m1.items() if m in m0 and m0[m] != l]
            moves = ', '.join(ch) or '—'
        print('| %s | %s | %s | %s | %s | %s |' % (name.replace('_', ' ').title(), '/'.join(map(str, s0)), '/'.join(map(str, s1)), types, abil, moves))


if __name__ == '__main__':
    main()
