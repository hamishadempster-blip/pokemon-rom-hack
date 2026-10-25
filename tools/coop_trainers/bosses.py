#!/usr/bin/env python3
"""Rivals, bosses, Elite Four, and champions for build 0xC017012F.

Run after overhaul.py. Rewrites only the entries named here; idempotent.

- Solo rival teams: the stock BRENDAN_*/MAY_* slots are what a game that is
  not linked fights (Coop_SoloRivalTrainer). They become SOREN and MIRA:
  the same core as that twin's co-op team for the case (Soren leads the
  starter that beats the case, Mira the companion that beats it), Route
  103 two levels lower (one Pokemon against a level-5 starter), and from
  Route 119 one extra Pokemon for a single battle (Soren: Lucario line,
  Mira: Hatenna line).
- Villains: Maxie, Tabitha, Archie, Shelly, Matt get teams between the
  surrounding trainers and the next gym, with added Pokemon on each.
- Wally: Ralts at Mauville; at Victory Road a six-Pokemon team.
- Elite Four and champions: one added Pokemon per side, no species shared.
- AI for bosses, leaders, partners, and both twins: Check Bad Move / Try To
  Faint / Check Viability / HP Aware / Smart Switching / Try To 2HKO /
  PP Stall Prevention / Weigh Ability Prediction, plus Ace Pokemon where the
  ace is last. Never Omniscient or move/item omniscience.
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import overhaul as O

BOSS_AI = ['Check Bad Move', 'Try To Faint', 'Check Viability', 'HP Aware', 'Smart Switching',
           'Try To 2HKO', 'PP Stall Prevention', 'Weigh Ability Prediction']
RIVAL_AI_EARLY = ['Check Bad Move', 'Try To Faint', 'Check Viability', 'HP Aware']

STAGES = [('ROUTE_103', 'R103'), ('RUSTBORO', 'RUSTBORO'), ('ROUTE_110', 'R110'), ('ROUTE_119', 'R119'),
          ('LILYCOVE', 'LILYCOVE')]
CASE_TYPE = {'TREECKO': 'FIRE', 'TORCHIC': 'WATER', 'MUDKIP': 'GRASS'}
EXTRA = {'SOREN': 'SPECIES_RIOLU', 'MIRA': 'SPECIES_HATENNA'}

# trainer -> (list of (species, level, item, ability, moves)), ace last.
TEAMS = {
    'TRAINER_TABITHA_MT_CHIMNEY': [('Numel', 26), ('Charcadet', 26), ('Mightyena', 27), ('Golbat', 28)],
    'TRAINER_MAXIE_MT_CHIMNEY': [('Mightyena', 29), ('Golbat', 29), ('Krokorok', 30), ('Camerupt', 31, 'Sitrus Berry')],
    'TRAINER_SHELLY_WEATHER_INSTITUTE': [('Mightyena', 34), ('Barraskewda', 34), ('Sharpedo', 36, 'Sitrus Berry')],
    'TRAINER_TABITHA_MAGMA_HIDEOUT': [('Camerupt', 40), ('Mightyena', 40), ('Armarouge', 41), ('Crobat', 42, 'Sitrus Berry')],
    'TRAINER_MAXIE_MAGMA_HIDEOUT': [('Mightyena', 44), ('Crobat', 44), ('Revavroom', 45), ('Camerupt', 46, 'Sitrus Berry')],
    'TRAINER_MATT': [('Mightyena', 41), ('Crobat', 41), ('Crabominable', 43, 'Sitrus Berry')],
    'TRAINER_TABITHA_MOSSDEEP': [('Mightyena', 45), ('Ceruledge', 46), ('Camerupt', 47, 'Charcoal')],
    'TRAINER_MAXIE_MOSSDEEP': [('Mightyena', 47), ('Crobat', 47), ('Krookodile', 48), ('Camerupt', 49, 'Sitrus Berry')],
    'TRAINER_SHELLY_SEAFLOOR_CAVERN': [('Mightyena', 48), ('Basculegion', 49), ('Sharpedo', 50, 'Sitrus Berry')],
    'TRAINER_ARCHIE': [('Mightyena', 52), ('Crobat', 52), ('Dondozo', 53), ('Sharpedo', 54, 'Sitrus Berry')],
    'TRAINER_WALLY_MAUVILLE': [('Ralts', 18)],
}
WALLY_VR = [('Altaria', 58), ('Delcatty', 57), ('Roserade', 58), ('Magneton', 57), ('Whimsicott', 58),
            ('Gardevoir', 60, 'Sitrus Berry')]

# Elite Four and champions: (trainer, slot index, new mon lines). Moves and
# abilities given explicitly where they matter for the side's plan.
E4_SWAPS = {
    'TRAINER_SIDNEY': (1, ['Kingambit @ Black Glasses', 'Ability: Defiant', 'Level: 65',
                           '- Kowtow Cleave', '- Iron Head', '- Sucker Punch', '- Protect']),
    'TRAINER_NOIR': (0, ['Grimmsnarl', 'Ability: Prankster', 'Level: 64',
                         '- Spirit Break', '- Throat Chop', '- Reflect', '- Light Screen']),
    'TRAINER_WRAITH': (0, ['Chandelure', 'Ability: Flash Fire', 'Level: 64',
                           '- Heat Wave', '- Shadow Ball', '- Energy Ball', '- Protect']),
    'TRAINER_GLACIA': (0, ['Frosmoth', 'Ability: Ice Scales', 'Level: 64',
                           '- Quiver Dance', '- Ice Beam', '- Bug Buzz', '- Protect']),
    'TRAINER_FROST': (0, ['Abomasnow', 'Ability: Snow Warning', 'Level: 64',
                          '- Blizzard', '- Wood Hammer', '- Ice Shard', '- Protect']),
    'TRAINER_DRAKE': (0, ['Dragapult', 'Ability: Clear Body', 'Level: 64',
                          '- Dragon Darts', '- Phantom Force', '- U Turn', '- Protect']),
    'TRAINER_STEVEN_CHAMPION': (1, ['Aegislash', 'Ability: Stance Change', 'Level: 64',
                                    '- Kings Shield', '- Shadow Ball', '- Flash Cannon', '- Shadow Sneak']),
    'TRAINER_WALLACE_CHAMPION': (1, ['Basculegion', 'Ability: Swift Swim', 'Level: 64',
                                     '- Wave Crash', '- Phantom Force', '- Aqua Jet', '- Protect']),
}
# Glacia's second Sealeo becomes the ace Walrein; Frost's lead Glalie moves
# to the ace slot behind Cetitan (Slush Rush in Abomasnow's snow).
EXTRA_EDITS = {
    'TRAINER_GLACIA': [(2, ['Walrein @ Sitrus Berry', 'Level: 66', '- Blizzard', '- Surf', '- Body Slam', '- Protect'])],
    'TRAINER_FROST': [(1, ['Cetitan', 'Ability: Slush Rush', 'Level: 65', '- Ice Spinner', '- Earthquake',
                           '- Liquidation', '- Protect']),
                      (2, ['Glalie', 'Level: 66', '- Freeze Dry', '- Crunch', '- Ice Shard', '- Protect'])],
    'TRAINER_DRAKE': [(1, ['Kommo-o', 'Ability: Bulletproof', 'Level: 65', '- Clanging Scales', '- Close Combat',
                           '- Poison Jab', '- Protect'])],
}


def mon_lines(spec, ivs):
    name, level = spec[0], spec[1]
    item = spec[2] if len(spec) > 2 else None
    return [name + (' @ ' + item if item else ''), 'Level: %d' % level, 'IVs: ' + ivs]


def set_ai(head, flags):
    out, done = [], False
    for l in head:
        if l.startswith('AI: '):
            out.append('AI: ' + ' / '.join(flags))
            done = True
        else:
            out.append(l)
    if not done:
        out.append('AI: ' + ' / '.join(flags))
    return out


def set_head(head, key, value):
    return [(key + ': ' + value) if l.startswith(key + ': ') else l for l in head]


def main():
    text = open(O.PARTY, encoding='utf-8').read()
    entries = O.split_entries(text)
    index = {t: i for i, (t, _) in enumerate(entries) if t}

    def get(tid):
        return O.parse_entry(entries[index[tid]][1])

    def put(tid, head, mons):
        entries[index[tid]] = (tid, '\n'.join(head) + '\n\n' + '\n\n'.join('\n'.join(m) for m in mons) + '\n\n')

    report = []
    # Solo rival teams.
    for stock, coop in STAGES:
        for case, typ in CASE_TYPE.items():
            for slot, twin in (('BRENDAN', 'SOREN'), ('MAY', 'MIRA')):
                tid = 'TRAINER_%s_%s_%s' % (slot, stock, case)
                _, cmons = get('TRAINER_%s_%s_%s' % (twin, coop, typ))
                head, smons = get(tid)
                ivs = next((l[5:] for m in cmons for l in m if l.startswith('IVs: ')), '0 HP / 0 Atk / 0 Def / 0 SpA / 0 SpD / 0 Spe')
                core = []
                for m in cmons:
                    lv = O.mon_level(m) - (2 if coop == 'R103' else 0)
                    sp = O.const_of(O.mon_species(m))
                    core.append([O.name_of(O.evolve(O.base_of(sp), lv) if coop == 'R103' else sp),
                                 'Level: %d' % lv, 'IVs: ' + ivs])
                if coop in ('R119', 'LILYCOVE'):
                    lv = min(O.mon_level(m) for m in cmons)
                    core.insert(0, [O.name_of(O.evolve(EXTRA[twin], lv)), 'Level: %d' % lv, 'IVs: ' + ivs])
                # Ace (the twin's signature line) last.
                ace = max(range(len(core)), key=lambda i: O.mon_level(core[i]))
                core.append(core.pop(ace))
                head = set_head(head, 'Name', twin)
                stage_ai = RIVAL_AI_EARLY if coop == 'R103' else BOSS_AI + (['Ace Pokemon'] if len(core) >= 3 else [])
                head = set_ai(head, stage_ai)
                put(tid, head, core)
                report.append((tid, ', '.join('%s %d' % (O.mon_species(m), O.mon_level(m)) for m in core)))

    # Villains and Wally.
    for tid, team in list(TEAMS.items()) + [('TRAINER_WALLY_VR_%d' % i, [(s[0], s[1] + 3 * (i - 1)) + tuple(s[2:]) for s in WALLY_VR]) for i in range(1, 6)]:
        head, mons = get(tid)
        ivs = next((l[5:] for m in mons for l in m if l.startswith('IVs: ')), '0 HP / 0 Atk / 0 Def / 0 SpA / 0 SpD / 0 Spe')
        new = [mon_lines(s, ivs) for s in team]
        put(tid, set_ai(head, BOSS_AI + (['Ace Pokemon'] if len(new) >= 3 else [])), new)
        report.append((tid, ', '.join('%s %d' % (s[0], s[1]) for s in team)))

    # Elite Four and champions.
    for tid, (slot, lines) in E4_SWAPS.items():
        head, mons = get(tid)
        ivs = next((l[5:] for m in mons for l in m if l.startswith('IVs: ')), '31 HP / 31 Atk / 31 Def / 31 SpA / 31 SpD / 31 Spe')
        mons[slot] = lines[:1] + [l for l in lines[1:] if l.startswith('Ability')] + [l for l in lines if l.startswith('Level')] + ['IVs: ' + ivs] + [l for l in lines if l.startswith('- ')]
        for s2, l2 in EXTRA_EDITS.get(tid, []):
            mons[s2] = l2[:1] + [l for l in l2[1:] if l.startswith('Ability')] + [l for l in l2 if l.startswith('Level')] + ['IVs: ' + ivs] + [l for l in l2 if l.startswith('- ')]
        put(tid, head, mons)
        report.append((tid, ', '.join('%s %d' % (O.mon_species(m), O.mon_level(m)) for m in mons)))

    # AI for every leader, partner, E4, champion, twin co-op team, admin, boss.
    text2 = ''.join(b for _, b in entries)
    partners = set()
    import glob
    for f in glob.glob(os.path.join(O.ROOT, 'data/maps/*/scripts.inc')):
        s = open(f).read()
        partners |= set(re.findall(r'setvar VAR_0x800[45], (TRAINER_\w+)', s))
    for i, (tid, body) in enumerate(entries):
        if not tid:
            continue
        head, mons = O.parse_entry(body)
        cls = O.header_get(head, 'Class')
        coop_twin = re.match(r'TRAINER_(SOREN|MIRA)_', tid)
        if not (cls in ('Leader', 'Elite Four', 'Champion', 'Aqua Admin', 'Aqua Leader', 'Magma Admin', 'Magma Leader')
                or tid in partners or coop_twin):
            continue
        if coop_twin and '_R103_' in tid:
            flags = RIVAL_AI_EARLY
        else:
            levels = [O.mon_level(m) for m in mons]
            ace_last = levels and levels[-1] == max(levels) and len(mons) >= 3
            flags = BOSS_AI + (['Ace Pokemon'] if ace_last else [])
        entries[i] = (tid, '\n'.join(set_ai(head, flags)) + '\n\n' + '\n\n'.join('\n'.join(m) for m in mons) + '\n\n')

    open(O.PARTY, 'w', encoding='utf-8').write(''.join(b for _, b in entries))
    with open(os.path.join(O.ROOT, 'docs/COOP_BOSSES_F.tsv'), 'w', encoding='utf-8') as f:
        f.write('trainer\tteam\n')
        for t, r in report:
            f.write('%s\t%s\n' % (t, r))
    print('%d boss and rival entries written' % len(report))


if __name__ == '__main__':
    main()
