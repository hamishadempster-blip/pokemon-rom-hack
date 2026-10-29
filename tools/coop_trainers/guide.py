#!/usr/bin/env python3
"""Hoenn Co-op Battle Guide v2 -> trainer data (build 0xC017013D onward).

The one team specification for every fight the guide covers. Input is
`battle_plan.json` (extracted from Hoenn_Coop_Boss_Team_Guide_1.pdf with
pypdf; guide_version 2). This script

- resolves each guide card to the real trainer ids that use it (first-clear
  gym pairs are the `*_1` ids, gym rematches the `*_COOP` ids, the solo
  rival slots are the stock BRENDAN_*/MAY_* ids, Steven's Space Center ally
  is PARTNER_STEVEN in battle_partners.party);
- writes those parties into src/data/trainers.party and
  src/data/battle_partners.party (header lines are kept, except Items, which
  is removed: no opponent bag healing, and AI, which is set here);
- writes src/data/coop_team_variants.h: the cards that share one trainer id
  but depend on progress or the saved starter case (pre-Winona arrivals,
  the three exhibition branches, Gabby and Ty's tiers, the stock gym
  rematch tiers). The game picks the party when the battle starts
  (Coop_TeamPartyId), from the lower progress of the two players in co-op;
- validates everything (species, forms, genders, abilities, Mega abilities
  and stones, moves against level-up/egg/TM/tutor learnsets, items, EVs,
  IVs, friendship, evolution level, caps at the earliest access, one Mega
  per trainer, 57 gym trainers).

    guide.py            write the files, then validate
    guide.py --check    validate only; also fails if the files differ from
                        what this script would write (stale data)

overhaul.py and bosses.py skip every id owned here (OWNED), so rerunning
them cannot bring back an older team.
"""
import json, os, re, sys

sys.path.insert(0, os.path.dirname(__file__))
import overhaul as O

ROOT = O.ROOT
PLAN = json.load(open(os.path.join(os.path.dirname(__file__), 'battle_plan.json'), encoding='utf-8'))
PARTY = O.PARTY
PARTNERS = os.path.join(ROOT, 'src/data/battle_partners.party')
VARIANTS_H = os.path.join(ROOT, 'src/data/coop_team_variants.h')
SPEC_MD = os.path.join(ROOT, 'docs/COOP_TEAM_SPEC.md')

# Progress tiers (Coop_ProgressTier): 0 before badge 1, 1-8 badges, 9 League
# admission, 10 Champion. CAPS[tier] is the level cap at that tier.
CAPS = [18, 24, 30, 36, 42, 48, 54, 60, 66, 76, 80]

BOSS_AI = ['Check Bad Move', 'Try To Faint', 'Check Viability', 'HP Aware', 'Smart Switching',
           'Try To 2HKO', 'PP Stall Prevention', 'Weigh Ability Prediction']
GYM_TRAINER_AI = ['Check Bad Move', 'Try To Faint', 'Check Viability']
GRUNT_AI = ['Check Bad Move', 'Try To Faint', 'Check Viability']
TUTORIAL_AI = ['Check Bad Move']

# Rival types of the plan -> the stock solo slot suffix (the case starter the
# player holds: Grass case faces Fire rivals, and so on).
RIVAL_SOLO_CASE = {'FIRE': 'TREECKO', 'WATER': 'TORCHIC', 'GRASS': 'MUDKIP'}
# VAR_STARTER_MON case (0 grass, 1 fire, 2 water) -> rival type.
CASE_RIVAL = {0: 'FIRE', 1: 'WATER', 2: 'GRASS'}
STAGE_SOLO = {'R103': 'ROUTE_103', 'RUSTBORO': 'RUSTBORO', 'R110': 'ROUTE_110', 'R119': 'ROUTE_119',
              'LILYCOVE': 'LILYCOVE'}

FIRST_CLEAR = {
    'TRAINER_ROXANNE_COOP': 'TRAINER_ROXANNE_1', 'TRAINER_HEATH_COOP': 'TRAINER_HEATH_1',
    'TRAINER_BRAWLY_COOP': 'TRAINER_BRAWLY_1', 'TRAINER_REED_COOP': 'TRAINER_REED_1',
    'TRAINER_WATTSON_COOP': 'TRAINER_WATTSON_1', 'TRAINER_JOULE_COOP': 'TRAINER_JOULE_1',
    'TRAINER_FLANNERY_COOP': 'TRAINER_FLANNERY_1', 'TRAINER_COLE_COOP': 'TRAINER_COLE_1',
    'TRAINER_NORMAN_COOP': 'TRAINER_NORMAN_1', 'TRAINER_HANA_COOP': 'TRAINER_HANA_1',
    'TRAINER_WINONA_COOP': 'TRAINER_WINONA_1', 'TRAINER_GALE_COOP': 'TRAINER_GALE_1',
    'TRAINER_TATE_COOP': 'TRAINER_TATE_AND_LIZA_1', 'TRAINER_LIZA_COOP': 'TRAINER_LIZA_1',
    'TRAINER_JUAN_COOP': 'TRAINER_JUAN_1', 'TRAINER_WALLACE_GYM_COOP': 'TRAINER_WALLACE_GYM',
}
REMATCH = {  # REMAP_EXISTING_<X>_REMATCH_TIERS -> (rematch id, stock tiers that must not keep old teams)
    'ROXANNE': ('TRAINER_ROXANNE_COOP', ['TRAINER_ROXANNE_%d' % i for i in range(2, 6)]),
    'HEATH': ('TRAINER_HEATH_COOP', []),
    'BRAWLY': ('TRAINER_BRAWLY_COOP', ['TRAINER_BRAWLY_%d' % i for i in range(2, 6)]),
    'REED': ('TRAINER_REED_COOP', []),
    'WATTSON': ('TRAINER_WATTSON_COOP', ['TRAINER_WATTSON_%d' % i for i in range(2, 6)]),
    'JOULE': ('TRAINER_JOULE_COOP', []),
    'FLANNERY': ('TRAINER_FLANNERY_COOP', ['TRAINER_FLANNERY_%d' % i for i in range(2, 6)]),
    'COLE': ('TRAINER_COLE_COOP', []),
    'NORMAN': ('TRAINER_NORMAN_COOP', ['TRAINER_NORMAN_%d' % i for i in range(2, 6)]),
    'HANA': ('TRAINER_HANA_COOP', []),
    'WINONA': ('TRAINER_WINONA_COOP', ['TRAINER_WINONA_%d' % i for i in range(2, 6)]),
    'GALE': ('TRAINER_GALE_COOP', []),
    'TATE': ('TRAINER_TATE_COOP', ['TRAINER_TATE_AND_LIZA_%d' % i for i in range(2, 6)]),
    'LIZA': ('TRAINER_LIZA_COOP', []),
    'JUAN': ('TRAINER_JUAN_COOP', ['TRAINER_JUAN_%d' % i for i in range(2, 6)]),
    'WALLACE': ('TRAINER_WALLACE_GYM_COOP', []),
}
# Labels the guide gives without the source id (checked against the map scripts).
RENAME = {
    'TRAINER_GRUNT_WOODS': 'TRAINER_GRUNT_PETALBURG_WOODS',
    'TRAINER_GRUNT_TUNNEL': 'TRAINER_GRUNT_RUSTURF_TUNNEL',
    'MUSEUM_GRUNT_1': 'TRAINER_GRUNT_MUSEUM_1',
    'MUSEUM_GRUNT_2': 'TRAINER_GRUNT_MUSEUM_2',
}
# Repeat entries of the Twin Trial final use the final teams (guide 16.22).
ALSO = {'TRAINER_BT_FINAL_LUMEN': ['TRAINER_BT_REPEAT_LUMEN'],
        'TRAINER_BT_FINAL_UMBRA': ['TRAINER_BT_REPEAT_UMBRA']}

# New party-only ids (no trainer flag; the base id keeps the flag). Order is
# the id order in include/constants/opponents.h.
NEW_IDS = [
    'TRAINER_SOREN_LILYCOVE_FIRE_EARLY', 'TRAINER_SOREN_LILYCOVE_WATER_EARLY', 'TRAINER_SOREN_LILYCOVE_GRASS_EARLY',
    'TRAINER_MIRA_LILYCOVE_FIRE_EARLY', 'TRAINER_MIRA_LILYCOVE_WATER_EARLY', 'TRAINER_MIRA_LILYCOVE_GRASS_EARLY',
    'TRAINER_TABITHA_MAGMA_HIDEOUT_EARLY', 'TRAINER_MAXIE_MAGMA_HIDEOUT_EARLY', 'TRAINER_MATT_EARLY',
    'TRAINER_TWIN_EXH_SOREN_FIRE', 'TRAINER_TWIN_EXH_SOREN_GRASS',
    'TRAINER_TWIN_EXH_MIRA_FIRE', 'TRAINER_TWIN_EXH_MIRA_GRASS',
]
FIRST_NEW_ID = 926

# Encounters a player can reach before the tier their card is written for
# (all other cards are fought at or after their own tier: story order).
# value: (earliest tier, why)
EARLY_ACCESS = {
    'TRAINER_TABITHA_MAGMA_HIDEOUT': (5, 'Jagged Pass and the hideout open before Winona'),
    'TRAINER_MAXIE_MAGMA_HIDEOUT': (5, 'as Tabitha'),
    'TRAINER_MATT': (5, 'Lilycove and the Aqua Hideout before Winona'),
    'TRAINER_SOREN_LILYCOVE_FIRE': (5, 'Lilycove before Winona'),
    'TRAINER_SOREN_LILYCOVE_WATER': (5, 'Lilycove before Winona'),
    'TRAINER_SOREN_LILYCOVE_GRASS': (5, 'Lilycove before Winona'),
    'TRAINER_MIRA_LILYCOVE_FIRE': (5, 'Lilycove before Winona'),
    'TRAINER_MIRA_LILYCOVE_WATER': (5, 'Lilycove before Winona'),
    'TRAINER_MIRA_LILYCOVE_GRASS': (5, 'Lilycove before Winona'),
}

# Narrow substitutions where the engine does not support the guide's choice.
# (card tid, species, original move) -> replacement. Documented in
# docs/COOP_TEAM_SPEC.md. Filled in after validation.
# '*' matches every card. Each entry is a move, ability, species or level the
# engine data does not allow for that Pokemon; the replacement keeps the role.
MOVE_SUBS = {
    ('*', 'Glimmet', 'Mud Slap'): 'Mud Shot',          # Glimmet has no Mud-Slap; same type and Speed drop
    ('*', 'Corvisquire', 'Steel Wing'): 'Taunt',       # no Steel move before Corviknight; Taunt at 22
    ('*', 'Heatmor', 'Snarl'): 'Sucker Punch',         # Dark coverage Heatmor can learn
    ('*', 'Stoutland', 'Body Slam'): 'Take Down',      # Normal physical STAB on its own level-up list
    ('*', 'Flamigo', 'Brick Break'): 'Low Sweep',      # Fighting physical of similar power
    ('*', 'Lapras', 'Scald'): 'Water Pulse',           # single-target special Water (no ally damage)
    ('*', 'Walrein', 'Scald'): 'Brine',                # single-target special Water
    ('*', 'Chimecho', 'Flash Cannon'): 'Dazzling Gleam',  # Chimecho has no Steel move; Fairy covers Dark
    ('*', 'Luvdisc', 'Dazzling Gleam'): 'Draining Kiss',  # Fairy special it learns
    ('*', 'Luvdisc', 'Helping Hand'): 'Heal Pulse',       # ally support it learns
    ('*', 'Starmie', 'Psychic Fangs'): 'Zen Headbutt',    # physical Psychic it learns (Huge Power set)
    ('*', 'Starmie', 'Ice Spinner'): 'Avalanche',         # physical Ice it learns
    ('*', 'Cradily', 'Power Gem'): 'Ancient Power',       # special Rock it learns
    # Level-up moves the guide gives below the level they are learned at.
    ('TRAINER_SOREN_R103_WATER', 'Mudkip', 'Mud Slap'): 'Rock Smash',  # Lv7: Rock Smash (6); still no Water move
    ('TRAINER_MIRA_R103_WATER', 'Totodile', 'Rage'): None,            # Rage not on its list; Water Gun would be STAB
    ('TRAINER_SOREN_R103_FIRE', 'Torchic', 'Sand Attack'): 'Quick Attack',  # Sand Attack 15, Quick Attack 6
    ('TRAINER_SOREN_R103_FIRE', 'Torchic', 'Focus Energy'): 'Protect',      # as the other Route 103 sets
    ('TRAINER_MIRA_R103_FIRE', 'Fennekin', 'Howl'): None,                   # Howl 11
    ('*', 'Carvanha', 'Bite'): 'Poison Fang',         # Bite 16; Poison Fang 4
    ('*', 'Roggenrola', 'Rock Blast'): 'Rock Tomb',   # Rock Blast 32
    ('*', 'Roggenrola', 'Headbutt'): 'Tackle',        # Headbutt 24
    ('*', 'Geodude', 'Rock Throw'): 'Rock Tomb',      # Rock Throw 16 (Lv15)
    ('*', 'Nacli', 'Headbutt'): 'Tackle',             # Headbutt 16 (Lv15)
    ('*', 'Nacli', 'Recover'): 'Harden',              # Recover 25
    ('*', 'Riolu', 'Force Palm'): 'Rock Smash',       # Riolu has no Force Palm; Rock Smash 20
    ('*', 'Mienfoo', 'Force Palm'): 'Low Kick',       # Force Palm 25; Low Kick egg move
    ('*', 'Pawmo', 'Spark'): 'Thunder Fang',          # Spark 23; Nuzzle already in the set
    ('*', 'Tentacool', 'Bubble Beam'): 'Water Pulse', # Bubble Beam 24
    ('*', 'Zubat', 'Bite'): 'Poison Fang',            # Bite 30
    ('*', 'Golbat', 'Bite'): 'Astonish',              # Bite 34; Poison Fang already in the set
    ('*', 'Slugma', 'Lava Plume'): 'Flame Burst',     # Lava Plume 34
    ('*', 'Flaaffy', 'Discharge'): 'Electroweb',      # Discharge 38; spread move that spares the ally
    ('*', 'Loudred', 'Uproar'): 'Echoed Voice',       # Uproar 43
    ('*', 'Lucario', 'Extreme Speed'): 'Bullet Punch',  # Extreme Speed 56; priority egg move
    ('*', 'Pelipper', 'Hurricane'): 'Air Slash',      # Hurricane 55 (Lv43)
    ('*', 'Dondozo', 'Wave Crash'): 'Liquidation',    # Wave Crash 65
}
LEVEL_DEPENDENT = {'Bite', 'Rock Blast', 'Headbutt', 'Rock Throw', 'Recover', 'Spark', 'Bubble Beam', 'Lava Plume',
                   'Discharge', 'Uproar', 'Extreme Speed', 'Hurricane', 'Wave Crash', 'Force Palm'}
ABILITY_SUBS = {
    ('*', 'Delcatty'): 'Cute Charm',    # this engine's Delcatty: Cute Charm / Prankster / Normalize
    ('*', 'Spinda'): 'Tangled Feet',    # revamped Spinda: Prankster / Tangled Feet / Contrary
    ('*', 'Wailord'): 'Water Veil',     # Wailord: Water Veil / Oblivious / Pressure
}
SPECIES_SUBS = {  # evolved forms the guide places below their evolution level
    ('*', 'Hattrem', 26): 'Hatenna',        # Hattrem from 32 (cap 30 here)
    ('*', 'Muk-Alola', 24): 'Grimer-Alola', # Muk from 38
    ('*', 'Weezing', 25): 'Koffing',        # Weezing from 35
    ('*', 'Magcargo', 33): 'Slugma',        # Magcargo from 38
}
LEVEL_SUBS = {
    ('TRAINER_WATTSON_COOP', 'Magneton'): 30,  # Magneton from 30; still at the cap
}


def sub(table, tid, *key):
    return table.get((tid,) + key, table.get(('*',) + key))


def native_moves(sp, level):
    """What the engine gives a Pokemon created at this level: the last four
    level-up moves at or below it."""
    lst = [(lv, mv) for lv, mv in LEVELUP.get(INFO[sp].get('lu', ''), []) if lv <= level]
    out = []
    for lv, mv in lst:
        if mv in out:
            out.remove(mv)
        out.append(mv)
    return [x.replace('_', ' ').title() for x in out[-4:]]


def mon_view(card, m):
    """The guide Pokemon with the documented substitutions applied.
    Starters and the opening wild Pokemon keep the engine's own level-up
    moves (the guide's beginner sets are not all on their lists at Lv5/Lv2)."""
    tid = card['team']['tid']
    if card.get('label') in ('STARTER PAIR', 'OPENING RESCUE'):
        v = dict(m)
        v['guide_name'] = m['name']
        v['base_ability'] = m['ability'].split('>')[0].strip()
        v['mega_ability'] = None
        v['moves'] = native_moves(species_const(m['name']), m['level'])
        return v
    v = dict(m)
    v['guide_name'] = m['name']
    v['name'] = sub(SPECIES_SUBS, tid, m['name'], m['level']) or m['name']
    v['level'] = sub(LEVEL_SUBS, tid, m['name']) or m['level']
    ab = m['ability'].split('>')
    v['base_ability'] = sub(ABILITY_SUBS, tid, m['name']) or ab[0].strip()
    v['mega_ability'] = ab[1].strip() if len(ab) > 1 else None
    moves = []
    for mv in m['moves']:
        key = (tid, m['name'], mv) if (tid, m['name'], mv) in MOVE_SUBS else ('*', m['name'], mv)
        if key in MOVE_SUBS:
            new = MOVE_SUBS[key]
            # A level-up move is only replaced where it is below its level.
            if learn_ok(species_const(v['name']), cname('MOVE_', mv), v['level'])[0] and key[0] == '*' and mv in LEVEL_DEPENDENT:
                new = mv
            if new:
                moves.append(new)
        else:
            moves.append(mv)
    v['moves'] = moves
    return v

GYM_LABEL = re.compile(r'^GYM (\d) TRAINERS$')


def cname(prefix, name):
    return prefix + re.sub(r'[^A-Z0-9]+', '_', name.upper().replace("'", '').replace('.', '').replace('é', 'E')).strip('_')


SPECIES_NAME = {'Alolan Ninetales': 'Ninetales-Alola'}


def species_const(name):
    return O.const_of(SPECIES_NAME.get(name, name))


def tier_of_cap(cap):
    return CAPS.index(cap)


# ---------------------------------------------------------------------------
# Resolution: every guide card -> the ids that carry it.
def resolve():
    """Returns (cards, partner_cards, starters, rescue).

    cards: list of dict(page, team, ids=[...], role, kind, tier, variant=None)
    """
    cards, partner, starters, rescue = [], [], [], None
    for page in PLAN['pages']:
        label, tier = page['label'], tier_of_cap(page['cap'])
        for team in page['teams']:
            tid = team['tid']
            base = dict(page=page, team=team, tier=tier, label=label, title=page['title'])
            if label == 'STARTER PAIR':
                starters.append(base)
                continue
            if tid == 'RESCUE_WILD':
                rescue = base
                continue
            if tid == 'SPACE_CENTER_STEVEN_ALLY':
                partner.append(dict(base, ids=['PARTNER_STEVEN'], kind='ally'))
                continue
            m = re.match(r'REMAP_EXISTING_(\w+)_REMATCH_TIERS$', tid)
            if m:
                rid, tiers = REMATCH[m.group(1)]
                cards.append(dict(base, ids=[rid], copies=tiers, kind='gym_rematch'))
                continue
            if label.startswith('GYM ') and not GYM_LABEL.match(label):
                cards.append(dict(base, ids=[FIRST_CLEAR[tid]], kind='gym_first'))
                continue
            if GYM_LABEL.match(label):
                cards.append(dict(base, ids=[tid], kind='gym_trainer', gym=int(GYM_LABEL.match(label).group(1))))
                continue
            if label == 'EARLY ARRIVAL / LILYCOVE' or label == 'EARLY ARRIVAL / BEFORE BADGE SIX':
                cards.append(dict(base, ids=[tid + '_EARLY'], kind='early', variant_of=tid))
                continue
            if label == 'POSTGAME / RIVAL EXHIBITION':
                rtype = re.search(r'You chose (\w+)', page['title']).group(1)
                rtype = PLAN['rival_counter_type'][rtype].upper()
                tgt = tid if rtype == 'WATER' else '%s_%s' % (tid, rtype)
                cards.append(dict(base, ids=[tgt], kind='exhibition', rtype=rtype, variant_of=tid))
                continue
            m = re.match(r'TRAINER_(SOREN|MIRA)_(\w+)_(FIRE|WATER|GRASS)$', tid)
            if m:
                twin, stage, rtype = m.groups()
                solo = 'TRAINER_%s_%s_%s' % ('BRENDAN' if twin == 'SOREN' else 'MAY', STAGE_SOLO[stage], RIVAL_SOLO_CASE[rtype])
                cards.append(dict(base, ids=[tid, solo], kind='rival', stage=stage, rtype=rtype))
                continue
            ids = [RENAME.get(tid, tid)] + ALSO.get(tid, [])
            cards.append(dict(base, ids=ids, kind='story'))
    return cards, partner, starters, rescue


# ---------------------------------------------------------------------------
# Party text.
STATS = ['HP', 'Atk', 'Def', 'SpA', 'SpD', 'Spe']


def mon_text(card, m):
    m = mon_view(card, m)
    sp = SPECIES_NAME.get(m['name'], m['name'])
    gender = '' if m['gender'] == '-' else ' (%s)' % m['gender']
    item = '' if m['item'] == 'None' else ' @ ' + m['item']
    ability = m['base_ability']
    lines = [sp + gender + item, 'Ability: ' + ability, 'Level: %d' % m['level'], 'Nature: ' + m['nature']]
    lines.append('IVs: ' + ' / '.join('%d %s' % (m['ivs'][s], s) for s in STATS))
    evs = [(m['evs'][s], s) for s in STATS if m['evs'][s]]
    if evs:
        lines.append('EVs: ' + ' / '.join('%d %s' % e for e in evs))
    if 'friendship' in m:
        lines.append('Happiness: %d' % m['friendship'])
    for mv in m['moves']:
        lines.append('- ' + mv)
    return lines


def ai_for(card):
    if card['kind'] == 'gym_trainer':
        return GYM_TRAINER_AI
    if card['kind'] == 'rival' and card['stage'] == 'R103':
        return TUTORIAL_AI
    if card['kind'] == 'story' and card['ids'][0].startswith('TRAINER_GRUNT'):
        return GRUNT_AI
    mons = card['team']['mons']
    flags = list(BOSS_AI)
    if len(mons) > 1 and mons[-1]['ace']:
        flags.append('Ace Pokemon')
    return flags


def set_header(head, card, base_id=None, new_id=None):
    out = []
    for l in head:
        if l.startswith('=== '):
            out.append('=== %s ===' % (new_id or l[4:-4]))
        elif l.startswith('Items: ') or l.startswith('AI: '):
            continue
        else:
            out.append(l)
    out.append('AI: ' + ' / '.join(ai_for(card)))
    return out


def entry_text(head, card, new_id=None):
    head = set_header(head, card, new_id=new_id)
    body = '\n\n'.join('\n'.join(mon_text(card, m)) for m in card['team']['mons'])
    return '\n'.join(head) + '\n\n' + body + '\n\n'


# ---------------------------------------------------------------------------
def build(text, cards, partner_text, partner_cards):
    entries = O.split_entries(text)
    index = {tid: i for i, (tid, _) in enumerate(entries) if tid}
    new_entries = {}
    owned = {}
    for card in cards:
        for tid in card['ids'] + card.get('copies', []):
            if tid in owned:
                raise SystemExit('two guide cards write %s: %s and %s' % (tid, owned[tid], card['title']))
            owned[tid] = card['title']
            if tid in index:
                head, _ = O.parse_entry(entries[index[tid]][1])
                entries[index[tid]] = (tid, entry_text(head, card))
            elif tid in NEW_IDS:
                src = card.get('variant_of')
                head, _ = O.parse_entry(entries[index[src]][1])
                new_entries[tid] = entry_text(head, card, new_id=tid)
            else:
                raise SystemExit('no trainer entry for %s (%s)' % (tid, card['title']))
    out = ''.join(b for _, b in entries)
    if not out.endswith('\n\n'):
        out = out.rstrip('\n') + '\n\n'
    for tid in NEW_IDS:
        if tid not in new_entries:
            raise SystemExit('new id %s has no card' % tid)
        out += new_entries[tid]
    out = out.rstrip('\n') + '\n'

    pentries = split_any(partner_text)
    pout = []
    for tid, body in pentries:
        card = next((c for c in partner_cards if tid in c['ids']), None)
        if card:
            head, _ = O.parse_entry(body)
            head = [l for l in head if not l.startswith('AI: ')] + ['AI: ' + ' / '.join(BOSS_AI)]
            body = '\n'.join(head) + '\n\n' + '\n\n'.join('\n'.join(mon_text(card, m)) for m in card['team']['mons']) + '\n\n'
        pout.append(body)
    ptext = ''.join(pout).rstrip('\n') + '\n'
    return out, ptext, owned


def split_any(text):
    parts = re.split(r'(?m)^(?==== \w+ ===\n)', text)
    out = []
    for p in parts:
        m = re.match(r'=== (\w+) ===\n', p)
        out.append((m.group(1) if m else None, p))
    return out


# ---------------------------------------------------------------------------
def variants_h(cards):
    """The selection table. Rules are tried in order; the first that matches
    gives the party. COOP_TEAMSEL_BELOW_TIER: progress tier below arg.
    COOP_TEAMSEL_CASE: saved starter case equals arg. COOP_TEAMSEL_ALWAYS.
    A party chosen by a rule is looked up again (chains), at most 8 times."""
    rows = []
    for c in cards:
        if c['kind'] == 'early':
            base = c['variant_of']
            rows.append((base, 'COOP_TEAMSEL_BELOW_TIER', 6, c['ids'][0], c['title']))
            m = re.match(r'TRAINER_(SOREN|MIRA)_LILYCOVE_(\w+)$', base)
            if m:
                solo = 'TRAINER_%s_LILYCOVE_%s' % ('BRENDAN' if m.group(1) == 'SOREN' else 'MAY', RIVAL_SOLO_CASE[m.group(2)])
                rows.append((solo, 'COOP_TEAMSEL_BELOW_TIER', 6, c['ids'][0], c['title'] + ' (solo slot)'))
        elif c['kind'] == 'exhibition' and c['rtype'] != 'WATER':
            case = {v: k for k, v in CASE_RIVAL.items()}[c['rtype']]
            rows.append((c['variant_of'], 'COOP_TEAMSEL_CASE', case, c['ids'][0], c['title']))
        elif c['kind'] == 'gym_rematch':
            for t in c['copies']:
                rows.append((t, 'COOP_TEAMSEL_ALWAYS', 0, c['ids'][0], 'stock rematch tier -> postgame rematch'))
    # Gabby and Ty: the repeat count picks the id (stock script); a tier the
    # player has not reached falls back to the one before. The first tier
    # is clamped to the cap instead (COOP_TEAMSEL_CLAMP).
    gabby = sorted([c for c in cards if c['ids'][0].startswith('TRAINER_GABBY_AND_TY_')], key=lambda c: c['ids'][0])
    for i, c in enumerate(gabby):
        need = min(t for t in range(len(CAPS)) if CAPS[t] >= max(m['level'] for m in c['team']['mons']))
        c['min_tier'] = need
        if i == 0:
            rows.append((c['ids'][0], 'COOP_TEAMSEL_CLAMP', need, c['ids'][0], 'below tier %d: levels clamped to the cap' % need))
        else:
            rows.append((c['ids'][0], 'COOP_TEAMSEL_BELOW_TIER', need, gabby[i - 1]['ids'][0], 'tier %d not reached' % need))
    out = ['// Generated by tools/coop_trainers/guide.py from battle_plan.json. Do not edit.',
           '// Which party a trainer id fields, by progress tier and saved starter case',
           '// (Coop_TeamPartyId, src/coop_teams.c).', '',
           'static const struct CoopTeamVariant sCoopTeamVariants[] =', '{']
    for base, how, arg, tgt, why in rows:
        out.append('    { %s, %s, %d, %s }, // %s' % (base, how, arg, tgt, why))
    out += ['};', '']
    return '\n'.join(out), rows


def opponents_defines():
    return ''.join('#define %-36s %d\n' % (t, FIRST_NEW_ID + i) for i, t in enumerate(NEW_IDS))


# ---------------------------------------------------------------------------
# Validation data.
LEVELUP = {}
lu = open(os.path.join(ROOT, 'src/data/pokemon/level_up_learnsets/gen_9.h')).read()
for m in re.finditer(r's(\w+)LevelUpLearnset\[\] = \{(.*?)LEVEL_UP_END', lu, re.S):
    LEVELUP[m.group(1)] = [(int(a), b) for a, b in re.findall(r'LEVEL_UP_MOVE\(\s*(\d+),\s*MOVE_(\w+)\)', m.group(2))]
EGG = {}
eg = open(os.path.join(ROOT, 'src/data/pokemon/egg_moves.h')).read()
for m in re.finditer(r's(\w+)EggMoveLearnset\[\] = \{(.*?)\};', eg, re.S):
    EGG[m.group(1)] = set(re.findall(r'MOVE_(\w+)', m.group(2)))
TEACH = {}
te = open(os.path.join(ROOT, 'src/data/pokemon/teachable_learnsets.h')).read()
for m in re.finditer(r's(\w+)TeachableLearnset\[\] = \{(.*?)\};', te, re.S):
    TEACH[m.group(1)] = set(re.findall(r'MOVE_(\w+)', m.group(2)))
INFO = {}
for f in sorted(os.listdir(os.path.join(ROOT, 'src/data/pokemon/species_info'))):
    s = open(os.path.join(ROOT, 'src/data/pokemon/species_info', f)).read()
    for m in re.finditer(r'\[(SPECIES_\w+)\] =\s*\{', s):
        blk = s[m.end():s.find('\n    },', m.end())]
        d = {}
        for key, rx in (('lu', r'\.levelUpLearnset = s(\w+)LevelUpLearnset'), ('egg', r'\.eggMoveLearnset = s(\w+)EggMoveLearnset'),
                        ('teach', r'\.teachableLearnset = s(\w+)TeachableLearnset'), ('gender', r'\.genderRatio = ([^,\n]+)'),
                        ('fct', r'\.formChangeTable = s(\w+)FormChangeTable'), ('hp', r'\.baseHP\s*=\s*(\d+)')):
            r = re.search(rx, blk)
            if r:
                d[key] = r.group(1).strip()
        d['mega'] = '.isMegaEvolution = TRUE' in blk
        INFO[m.group(1)] = d
MEGA_ITEM = {}  # item -> (form change table, mega species)
fc = open(os.path.join(ROOT, 'src/data/pokemon/form_change_tables.h')).read()
for m in re.finditer(r's(\w+)FormChangeTable\[\] =\s*\{(.*?)\};', fc, re.S):
    for sp, it in re.findall(r'FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM,\s*(SPECIES_\w+),\s*(ITEM_\w+)', m.group(2)):
        MEGA_ITEM.setdefault(it, []).append((m.group(1), sp))
ITEMS = set(re.findall(r'\b(ITEM_\w+)\b', open(os.path.join(ROOT, 'include/constants/items.h')).read()))
MOVES = set(re.findall(r'\b(MOVE_\w+)\b', open(os.path.join(ROOT, 'include/constants/moves.h')).read()))
NATURES = {'Hardy', 'Lonely', 'Brave', 'Adamant', 'Naughty', 'Bold', 'Docile', 'Relaxed', 'Impish', 'Lax', 'Timid',
           'Hasty', 'Serious', 'Jolly', 'Naive', 'Modest', 'Mild', 'Quiet', 'Bashful', 'Rash', 'Calm', 'Gentle',
           'Sassy', 'Careful', 'Quirky'}
LEARNABLES = json.load(open(os.path.join(ROOT, 'src/data/pokemon/all_learnables.json')))
FRIENDSHIP_MOVES = {'RETURN', 'FRUSTRATION'}


def learn_ok(sp, mv, level):
    """(ok, how) for a move on a species at a level, own line or prevos.
    all_learnables.json (every game's TM/tutor/level-up lists) counts only
    for moves that are not on the line's level-up lists here, so a level-up
    move is never accepted below its level."""
    key = mv[len('MOVE_'):]
    c, seen, line_lu = sp, set(), False
    while c and c not in seen:
        seen.add(c)
        d = INFO.get(c, {})
        for lv, x in LEVELUP.get(d.get('lu', ''), []):
            if x == key:
                line_lu = True
                if lv <= level:
                    return True, 'level %d%s' % (lv, '' if c == sp else ' as ' + c)
        if key in TEACH.get(d.get('teach', ''), set()):
            return True, 'TM/tutor'
        if key in EGG.get(d.get('egg', ''), set()):
            return True, 'egg'
        c = O.PREVO.get(c)
    if not line_lu:
        c, seen = sp, set()
        while c and c not in seen:
            seen.add(c)
            k2 = c[len('SPECIES_'):]
            if mv in LEARNABLES.get(k2, []) or mv in LEARNABLES.get(k2.split('_')[0], []):
                return True, 'TM/tutor in other games (not sold here)'
            c = O.PREVO.get(c)
    return False, ''


def evo_level(sp):
    """Lowest level a trainer could have this species by level-up evolution
    (0 when none is needed); None when its line evolves another way."""
    prev = O.PREVO.get(sp)
    if not prev:
        return 0
    for method, param, target, rest in O.SPECIES.get(prev, {}).get('evos', []):
        if target != sp:
            continue
        if method in ('EVO_LEVEL', 'EVO_LEVEL_BATTLE_ONLY'):
            try:
                p = int(param)
            except ValueError:
                return None
            below = evo_level(prev)
            if p == 0:
                return None if below is None else below
            return max(p, below or 0)
        return None
    return 0


def validate(cards, partner_cards, starters, rescue, rows, owned):
    errors, notes = [], []
    gym_trainers = set()
    for c in cards + partner_cards + starters + ([rescue] if rescue else []):
        tid = c['team']['tid'] or c['team']['trainer']
        where = '%s / %s' % (c['title'], tid)
        megas = 0
        for m in c['team']['mons']:
            m = mon_view(c, m)
            sp = species_const(m['name'])
            if sp not in INFO:
                errors.append('%s: species %s' % (where, m['name']))
                continue
            d = INFO[sp]
            if m['level'] < 1 or m['level'] > 100:
                errors.append('%s: level %d' % (where, m['level']))
            g = d.get('gender', '')
            if m['gender'] == '-':
                if g not in ('MON_GENDERLESS',):
                    notes.append('%s: %s has no gender given (random by ratio)' % (where, m['name']))
            elif g == 'MON_GENDERLESS' or (g == 'MON_MALE' and m['gender'] == 'F') or (g == 'MON_FEMALE' and m['gender'] == 'M'):
                errors.append('%s: %s cannot be %s (%s)' % (where, m['name'], m['gender'], g))
            elif g == 'PERCENT_FEMALE(0)' and m['gender'] == 'F' or g == 'PERCENT_FEMALE(100)' and m['gender'] == 'M':
                errors.append('%s: %s cannot be %s' % (where, m['name'], m['gender']))
            base_ab = m['base_ability']
            if cname('', base_ab) not in O.SPECIES[sp]['abilities']:
                errors.append('%s: %s cannot have %s (has %s)' % (where, m['name'], base_ab, O.SPECIES[sp]['abilities']))
            if m['nature'] not in NATURES:
                errors.append('%s: nature %s' % (where, m['nature']))
            if any(not 0 <= m['ivs'][s] <= 31 for s in STATS):
                errors.append('%s: IVs %s' % (where, m['ivs']))
            if any(not 0 <= m['evs'][s] <= 252 for s in STATS) or sum(m['evs'].values()) > 510:
                errors.append('%s: EVs %s' % (where, m['evs']))
            if m['item'] != 'None' and cname('ITEM_', m['item']) not in ITEMS:
                errors.append('%s: item %s' % (where, m['item']))
            if len(m['moves']) > 4 or len(set(m['moves'])) != len(m['moves']):
                errors.append('%s: moves %s' % (where, m['moves']))
            for mv2 in m['moves']:
                k = cname('MOVE_', mv2)
                if k not in MOVES:
                    errors.append('%s: move %s' % (where, mv2))
                    continue
                ok, how = learn_ok(sp, k, m['level'])
                if not ok:
                    errors.append('%s: %s (Lv%d) cannot learn %s' % (where, m['name'], m['level'], mv2))
                if k[5:] in FRIENDSHIP_MOVES and m.get('friendship', 0) < 255 and k == 'MOVE_RETURN':
                    errors.append('%s: %s Return without full friendship' % (where, m['name']))
            el = evo_level(sp)
            if el is None:
                if m['level'] < 20:
                    notes.append('%s: %s at Lv%d evolves by item/trade/friendship' % (where, m['name'], m['level']))
            elif el > m['level']:
                errors.append('%s: %s at Lv%d (evolves at %d)' % (where, m['name'], m['level'], el))
            if m['mega']:
                megas += 1
                it = cname('ITEM_', m['item'])
                tbl = MEGA_ITEM.get(it)
                if not tbl:
                    errors.append('%s: %s is not a Mega Stone' % (where, m['item']))
                else:
                    mega_sp = tbl[0][1]
                    fct = INFO[sp].get('fct')
                    if not any(t == fct for t, _ in tbl):
                        errors.append('%s: %s does not Mega Evolve with %s' % (where, m['name'], m['item']))
                    want = m['mega_ability']
                    if want and cname('', want) not in O.SPECIES[mega_sp]['abilities']:
                        errors.append('%s: %s has %s, guide says %s' % (where, mega_sp, O.SPECIES[mega_sp]['abilities'], want))
            elif m['item'] != 'None' and cname('ITEM_', m['item']) in MEGA_ITEM:
                errors.append('%s: holds %s without a Mega' % (where, m['item']))
        if megas > 1:
            errors.append('%s: %d Megas on one trainer' % (where, megas))
        if c.get('kind') == 'gym_trainer':
            gym_trainers.add(tid)
            if megas or len(c['team']['mons']) > 3:
                errors.append('%s: gym trainer above trainer rules' % where)
        # Caps: the levels at the tier the card is fought, and at the
        # earliest tier it can be reached (which must then pick a variant).
        if c['team']['mons']:
            top = max(m['level'] for m in c['team']['mons'])
            if top > CAPS[c['tier']]:
                errors.append('%s: Lv%d above the cap %d' % (where, top, CAPS[c['tier']]))
            base = c['ids'][0] if c.get('ids') else ''
            if base in EARLY_ACCESS:
                early = EARLY_ACCESS[base][0]
                var = next((r for r in rows if r[0] == base and r[1] == 'COOP_TEAMSEL_BELOW_TIER' and r[2] > early), None)
                if var is None and top > CAPS[early]:
                    errors.append('%s: reachable at tier %d (cap %d) with Lv%d and no variant' % (where, early, CAPS[early], top))
            if c.get('kind') == 'early' and top > CAPS[5]:
                errors.append('%s: early variant Lv%d above the badge-5 cap' % (where, top))
    if len(gym_trainers) != PLAN['gym_trainer_count']:
        errors.append('gym trainers: %d, guide says %d' % (len(gym_trainers), PLAN['gym_trainer_count']))
    # Gym trainers must be the trainers of their gym's maps.
    gym_maps = {1: ['RustboroCity_Gym'], 2: ['DewfordTown_Gym'], 3: ['MauvilleCity_Gym'],
                4: ['LavaridgeTown_Gym_1F', 'LavaridgeTown_Gym_B1F'], 5: ['PetalburgCity_Gym'], 6: ['FortreeCity_Gym'],
                7: ['MossdeepCity_Gym'], 8: ['SootopolisCity_Gym_1F', 'SootopolisCity_Gym_B1F']}
    for g, maps in gym_maps.items():
        found = set()
        for mp in maps:
            found |= set(re.findall(r'trainerbattle\w*\s+(?:\w+,\s*)?(TRAINER_\w+)', open(os.path.join(ROOT, 'data/maps', mp, 'scripts.inc')).read()))
        found = {t for t in found if 'TATE_AND_LIZA' not in t}
        mine = {c['team']['tid'] for c in cards if c.get('gym') == g}
        missing = found - mine
        extra = mine - found
        if missing or extra:
            errors.append('gym %d: not in guide %s, guide not in gym %s' % (g, sorted(missing), sorted(extra)))
    # Renamed story ids must be fought by the scripts.
    scripts = ''
    for dp, _, fs in os.walk(os.path.join(ROOT, 'data')):
        for f in fs:
            if f.endswith('.inc'):
                scripts += open(os.path.join(dp, f), encoding='utf-8', errors='replace').read()
    for a, b in RENAME.items():
        if b not in scripts:
            errors.append('%s -> %s is not used by any script' % (a, b))
    # Rival routing: each stage x type has both twins and a solo slot.
    for stage in STAGE_SOLO:
        for rt in RIVAL_SOLO_CASE:
            for twin in ('SOREN', 'MIRA'):
                tid = 'TRAINER_%s_%s_%s' % (twin, stage, rt)
                if tid not in owned:
                    errors.append('rival %s missing' % tid)
    return errors, notes


# ---------------------------------------------------------------------------
def spec_md(cards, partner_cards, starters, rescue, rows):
    L = ['# Co-op team specification (generated)', '',
         'Generated by `tools/coop_trainers/guide.py` from `tools/coop_trainers/battle_plan.json` '
         '(Hoenn Co-op Battle Guide v2). Do not edit by hand.', '',
         'Progress tiers: 0 before badge 1; 1-8 badges; 9 League admission; 10 Champion. '
         'Caps by tier: ' + ', '.join('%d: %d' % (i, c) for i, c in enumerate(CAPS)) + '.', '',
         '## Cards and the ids that carry them', '',
         '| Guide | Card | Ids | Levels | Megas |', '| --- | --- | --- | --- | --- |']
    for c in cards + partner_cards:
        lv = [m['level'] for m in c['team']['mons']]
        megas = ', '.join(m['name'] for m in c['team']['mons'] if m['mega']) or '-'
        ids = ', '.join('`%s`' % i for i in c['ids'] + c.get('copies', []))
        L.append('| %s | %s: %s | %s | %d-%d | %s |' % (c['page']['order'] if isinstance(c['page']['order'], int) else round(c['page']['order'], 3),
                                                      c['title'], c['team']['trainer'], ids, min(lv), max(lv), megas))
    L += ['', '## Selection rules (src/data/coop_team_variants.h)', '',
          'Picked once when the battle starts, never during it. Solo: this save. Co-op: the lower progress '
          'tier of the two players and the inviter\'s saved starter case, both carried by the invite and its answer.', '',
          '| Base id | Rule | Party |', '| --- | --- | --- |']
    for base, how, arg, tgt, why in rows:
        rule = {'COOP_TEAMSEL_BELOW_TIER': 'tier below %d' % arg, 'COOP_TEAMSEL_CASE': 'starter case %d' % arg,
                'COOP_TEAMSEL_ALWAYS': 'always', 'COOP_TEAMSEL_CLAMP': 'levels clamped to the cap below tier %d' % arg}[how]
        L.append('| `%s` | %s | `%s` (%s) |' % (base, rule, tgt, why))
    L += ['', '## Substitutions (engine data does not allow the guide value)', '',
          'Reasons are in the comments of `tools/coop_trainers/guide.py`. `*` = every card with that Pokemon.', '']
    for (tid, sp, mv), new in sorted(MOVE_SUBS.items()):
        L.append('- `%s` %s: move %s -> %s' % (tid, sp, mv, new))
    for (tid, sp), new in sorted(ABILITY_SUBS.items()):
        L.append('- `%s` %s: ability -> %s' % (tid, sp, new))
    for (tid, sp, lv), new in sorted(SPECIES_SUBS.items()):
        L.append('- `%s` %s Lv%d: species -> %s (evolution level)' % (tid, sp, lv, new))
    for (tid, sp), lv in sorted(LEVEL_SUBS.items()):
        L.append('- `%s` %s: level -> %d (evolution level)' % (tid, sp, lv))
    L.append('')
    return '\n'.join(L)


def main():
    check = '--check' in sys.argv
    dry = '--dry' in sys.argv
    cards, partner_cards, starters, rescue = resolve()
    text = open(PARTY, encoding='utf-8').read()
    ptext = open(PARTNERS, encoding='utf-8').read()
    new_text, new_ptext, owned = build(text, cards, ptext, partner_cards)
    vh, rows = variants_h(cards)
    md = spec_md(cards, partner_cards, starters, rescue, rows)
    errors, notes = validate(cards, partner_cards, starters, rescue, rows, owned)
    opp = open(os.path.join(ROOT, 'include/constants/opponents.h')).read()
    if opponents_defines() not in opp:
        errors.append('include/constants/opponents.h lacks the new ids:\n' + opponents_defines())
    for n in notes:
        print('note:', n)
    if check:
        stale = [p for p, want in ((PARTY, new_text), (PARTNERS, new_ptext), (VARIANTS_H, vh), (SPEC_MD, md))
                 if not os.path.exists(p) or open(p, encoding='utf-8').read() != want]
        for p in stale:
            errors.append('stale: %s (run tools/coop_trainers/guide.py)' % os.path.relpath(p, ROOT))
    elif not dry:
        open(PARTY, 'w', encoding='utf-8').write(new_text)
        open(PARTNERS, 'w', encoding='utf-8').write(new_ptext)
        open(VARIANTS_H, 'w', encoding='utf-8').write(vh)
        open(SPEC_MD, 'w', encoding='utf-8').write(md)
    for e in errors:
        print('ERROR:', e)
    print('%d cards (%d ids), %d selection rules, %d errors' % (len(cards) + len(partner_cards), len(owned), len(rows), len(errors)))
    return 1 if errors else 0


def owned_ids():
    cards, partner_cards, _, _ = resolve()
    ids = set()
    for c in cards:
        ids |= set(c['ids']) | set(c.get('copies', []))
    return ids


if __name__ == '__main__':
    sys.exit(main())
