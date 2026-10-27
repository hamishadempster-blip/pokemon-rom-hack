#!/usr/bin/env python3
"""Revamp showcase (build 0xC0170137): ordinary route trainers that field
the twelve revamped Pokemon at their route's usual level. Idempotent: each
entry adds the Pokemon (with the trainer's own IVs) if it is not there, or
replaces one named Pokemon. Parties stay at three or fewer."""
import os, re
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
PATH = os.path.join(ROOT, 'src/data/trainers.party')

# trainer: (species, level, replaces or None)
SHOWCASE = {
    'DEREK': ('Butterfree', 17, None),        # Route 117 Bug Maniac
    'MELINA': ('Furret', 18, None),           # Route 117 Triathlete
    'BIANCA': ('Maractus', 20, None),         # Route 111 Picnicker
    'BRENT': ('Kricketune', 27, None),        # Route 119 Bug Maniac
    'DOUG': ('Beedrill', 30, 'Ninjask'),      # Route 119 Bug Catcher (second Ninjask)
    'CATHERINE_1': ('Parasect', 27, None),    # Route 119 Pkmn Ranger
    'VIOLET': ('Cherrim', 28, 'Gloom'),       # Route 123 Aroma Lady
    'JACKI_1': ('Hypno', 33, None),           # Route 123 Psychic
    'KINDRA': ('Arbok', 33, None),            # Route 123 Hex Maniac
    'ROLAND': ('Lanturn', 37, 'Tentacruel'),  # Route 124 Swimmer
    'TANYA': ('Lumineon', 37, None),          # Route 125 Swimmer
}


def main():
    s = open(PATH).read()
    for tr, (sp, lv, rep) in SHOWCASE.items():
        head = '=== TRAINER_%s ===\n' % tr
        i = s.index(head)
        j = s.find('\n=== ', i + 1)
        j = len(s) if j < 0 else j + 1
        blk = s[i:j]
        mons = re.split(r'\n\n', blk.strip('\n'))
        header, party = mons[0], mons[1:]
        if any(m.split('\n')[0].split(' @')[0].strip() == sp for m in party):
            continue
        ivs = re.search(r'IVs: [^\n]*', party[0]).group(0)
        new = '%s\nLevel: %d\n%s' % (sp, lv, ivs)
        if rep:
            idx = max(k for k, m in enumerate(party) if m.split('\n')[0].split(' @')[0].strip() == rep)
            party[idx] = new
        else:
            party.append(new)
        assert len(party) <= 3, tr
        s = s[:i] + '\n\n'.join([header] + party) + '\n\n' + s[j:]
    open(PATH, 'w').write(s)


if __name__ == '__main__':
    main()
