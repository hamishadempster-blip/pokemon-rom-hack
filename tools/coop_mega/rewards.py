#!/usr/bin/env python3
"""Mega Stones and the extra overworld rewards (build 0xC017013D onward).

One table (STONES, ITEMS below) gives every Mega Stone of an obtainable
species its first source and every extra item ball its place. This script

- places the item balls: a new object on the map (Common_EventScript_FindItem,
  one flag each, so each save collects its own once); the tile is picked by
  tools/coop_mega/place.py (plain reachable land, never next to a warp,
  trigger or person, never cutting a path; dead ends first) and kept once
  chosen;
- allocates the flags in include/constants/flags.h (unused slots only);
- writes the Trainer Exchange's MEGA STONE counter rows and their unlock
  conditions (read by tools/coop_exchange/stock.py): every stone can be
  bought there again once its first source is done (the ball picked up, the
  quest reward taken) or, for the featured stones, from a BADGE tier;
- writes the in-game hints (src/data/coop_mega_hints.h) and the manifest
  (docs/COOP_MEGA_MANIFEST.md, docs/coop_mega_manifest.json);
- checks coverage: every Mega form of a species the roster manifest
  (docs/coop_roster_manifest.json) lists as obtainable has its stone here,
  with a first source and the Exchange as its rebuy route; a species added
  to the roster later fails this check until its Mega access is added.
  Mega forms must keep their base form's HP.

    rewards.py           write everything, then check
    rewards.py --check   check only (also: the written files are current)
"""
import json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, 'tools/coop_trainers'))
import place  # noqa: E402

# Exchange tiers (src/coop_exchange.c OwnTier): 3 = 5 badges, 4 = 7, 5 = Champion.
EX5, EX7, EXC = 3, 4, 5
PRICE = {EX5: 10000, EX7: 15000, EXC: 20000}

# Stone -> first source. ('exchange', tier): sold from that tier.
# ('ball', MAP, hint): an item ball on that map (surf=True where only Surf
# reaches the spot). ('quest', QUEST): the reward of an existing quest.
STONES = {
    # Starter lines and the early, useful set: sold from 5 BADGES (pre-Winona).
    'BLAZIKENITE': ('exchange', EX5), 'SWAMPERTITE': ('exchange', EX5), 'SCEPTILITE': ('exchange', EX5),
    'DELPHOXITE': ('exchange', EX5), 'FERALIGITE': ('exchange', EX5), 'MEGANIUMITE': ('exchange', EX5),
    'GARDEVOIRITE': ('exchange', EX5), 'GALLADITE': ('exchange', EX5), 'LUCARIONITE': ('exchange', EX5),
    'AGGRONITE': ('exchange', EX5), 'MANECTITE': ('exchange', EX5), 'ALTARIANITE': ('exchange', EX5),
    'ABSOLITE': ('exchange', EX7), 'BANETTITE': ('exchange', EX7), 'CHIMECHITE': ('exchange', EX7),
    'GLALITITE': ('exchange', EX7), 'METAGROSSITE': ('exchange', EX7), 'MEDICHAMITE': ('exchange', EX7),
    'LUCARIONITE_Z': ('exchange', EXC), 'ABSOLITE_Z': ('exchange', EXC), 'GARCHOMPITE_Z': ('exchange', EXC),
    'LATIASITE': ('exchange', EXC), 'LATIOSITE': ('exchange', EXC), 'HOUNDOOMINITE': ('exchange', EXC),
    'AMPHAROSITE': ('exchange', EXC),
    # Quest rewards (each save's own, once; also handed to a save that had
    # already finished the quest, the next time its quest giver is spoken to).
    'DRAGALGITE': ('quest', 'COAST'), 'GARCHOMPITE': ('quest', 'CARAVAN'), 'EELEKTROSSITE': ('quest', 'PP_WATTSON'),
    'FROSLASSITE': ('quest', 'SHOAL_RESCUE'),
    # Exploration pickups.
    'BEEDRILLITE': ('ball', 'PetalburgWoods', 'PETALBURG WOODS'),
    'PIDGEOTITE': ('ball', 'Route104', 'ROUTE 104'),
    'STARAPTITE': ('ball', 'Route116', 'ROUTE 116'),
    'SCOLIPITE': ('ball', 'Route117', 'ROUTE 117'),
    'LOPUNNITE': ('ball', 'Route102', 'ROUTE 102'),
    'ALAKAZITE': ('ball', 'GraniteCave_1F', 'GRANITE CAVE'),
    'SABLENITE': ('ball', 'GraniteCave_B1F', 'GRANITE CAVE'),
    'STEELIXITE': ('ball', 'GraniteCave_B2F', 'GRANITE CAVE'),
    'EXCADRITE': ('ball', 'RusturfTunnel', 'RUSTURF TUNNEL'),
    'GLIMMORANITE': ('ball', 'MeteorFalls_1F_1R', 'METEOR FALLS'),
    'TYRANITARITE': ('ball', 'JaggedPass', 'JAGGED PASS'),
    'VENUSAURITE': ('ball', 'Route110', 'ROUTE 110'),
    'CHARIZARDITE_X': ('ball', 'FieryPath', 'FIERY PATH'),
    'CHARIZARDITE_Y': ('ball', 'Route112', 'ROUTE 112'),
    'BLASTOISINITE': ('ball', 'Route109', 'ROUTE 109 BEACH'),
    'GYARADOSITE': ('ball', 'Route103', 'ROUTE 103'),
    'CAMERUPTITE': ('ball', 'MtChimney', 'MT. CHIMNEY'),
    'AERODACTYLITE': ('ball', 'DesertUnderpass', 'DESERT UNDERPASS'),
    'CLEFABLITE': ('ball', 'MeteorFalls_1F_2R', 'METEOR FALLS'),
    'FALINKSITE': ('ball', 'Route111', 'ROUTE 111'),
    'HAWLUCHANITE': ('ball', 'Route113', 'ROUTE 113'),
    'PYROARITE': ('ball', 'Route114', 'ROUTE 114'),
    'SCOVILLAINITE': ('ball', 'Route115', 'ROUTE 115'),
    'SCRAFTINITE': ('ball', 'Route114', 'ROUTE 114'),
    'SKARMORITE': ('ball', 'Route113', 'ROUTE 113'),
    'AUDINITE': ('ball', 'Route118', 'ROUTE 118'),
    'CHANDELURITE': ('ball', 'MtPyre_4F', 'MT. PYRE'),
    'GENGARITE': ('ball', 'MtPyre_2F', 'MT. PYRE'),
    'CRABOMINITE': ('ball', 'ShoalCave_LowTideLowerRoom', 'SHOAL CAVE'),
    'GOLISOPITE': ('ball', 'Route106', 'ROUTE 106'),
    'GOLURKITE': ('ball', 'Route120', 'ROUTE 120'),
    'HERACRONITE': ('ball', 'Route119', 'ROUTE 119'),
    'KANGASKHANITE': ('ball', 'SafariZone_South', 'SAFARI ZONE'),
    'MEOWSTICITE': ('ball', 'Route120', 'ROUTE 120'),
    'PINSIRITE': ('ball', 'SafariZone_Southwest', 'SAFARI ZONE'),
    'RAICHUNITE_X': ('ball', 'NewMauville_Inside', 'NEW MAUVILLE'),
    'RAICHUNITE_Y': ('ball', 'Route117', 'ROUTE 117'),
    'SCIZORITE': ('ball', 'Route119', 'ROUTE 119'),
    'SHARPEDONITE': ('ball', 'Route118', 'ROUTE 118'),
    'SLOWBRONITE': ('ball', 'Route121', 'ROUTE 121'),
    'VICTREEBELITE': ('ball', 'Route123', 'ROUTE 123'),
    'EMBOARITE': ('ball', 'Route112', 'ROUTE 112'),
    'ABOMASITE': ('ball', 'ShoalCave_LowTideIceRoom', 'SHOAL CAVE'),
    'BARBARACITE': ('ball', 'Route124', 'ROUTE 124'),
    'CHESNAUGHTITE': ('ball', 'Route121', 'ROUTE 121'),
    'DRAGONINITE': ('ball', 'MeteorFalls_B1F_2R', 'METEOR FALLS'),
    'GRENINJITE': ('ball', 'Route123', 'ROUTE 123'),
    'STARMINITE': ('ball', 'Route122', 'ROUTE 122'),
    'BAXCALIBRITE': ('ball', 'ShoalCave_LowTideInnerRoom', 'SHOAL CAVE'),
    'DRAMPANITE': ('ball', 'MtPyre_Exterior', 'MT. PYRE'),
    'MALAMARITE': ('ball', 'VictoryRoad_B1F', 'VICTORY ROAD'),
    'MAWILITE': ('ball', 'VictoryRoad_1F', 'VICTORY ROAD'),
    'SALAMENCITE': ('ball', 'MeteorFalls_B1F_1R', 'METEOR FALLS'),
}

# The quests whose rewards carry a stone: the C test for "this save took the
# reward" (the Exchange rebuy unlock) and the script that hands it out.
QUESTS = {
    'COAST': ('CoopQuest_IsRewardClaimed(COOP_REWARD_COAST)', 'the coastal cleanup (MARLIN, ROUTE 109)'),
    'CARAVAN': ('CoopQuest_IsRewardClaimed(COOP_REWARD_CARAVAN)', 'the desert caravan (ZAHRA, ROUTE 111)'),
    'PP_WATTSON': ('CoopQuest_IsRewardClaimed(COOP_REWARD_PP_WATTSON)', 'POWER PROBLEMS (WATTSON, MAUVILLE)'),
    'SHOAL_RESCUE': ('CoopQuest_IsRewardClaimed(COOP_REWARD_SHOAL_RESCUE)', 'the SHOAL CAVE rescue (the researcher)'),
}

# Ordinary extra item balls: Poke Balls, evolution supplies, battle items and
# training items, through optional corners of Hoenn. Routine consumables
# (Potions, Repels, ordinary Poke Balls) stay in the shops.
ITEMS = [
    ('GREAT_BALL', 'Route116'), ('NET_BALL', 'Route117'), ('DIVE_BALL', 'Route124'), ('DUSK_BALL', 'GraniteCave_B2F'),
    ('QUICK_BALL', 'Route119'), ('TIMER_BALL', 'Route121'), ('ULTRA_BALL', 'Route120'), ('NEST_BALL', 'Route104'),
    ('MOON_STONE', 'MeteorFalls_1F_2R'), ('FIRE_STONE', 'FieryPath'), ('THUNDER_STONE', 'NewMauville_Inside'),
    ('WATER_STONE', 'Route109'), ('LEAF_STONE', 'Route119'), ('SUN_STONE', 'Route111'), ('DUSK_STONE', 'MtPyre_3F'),
    ('OVAL_STONE', 'Route117'), ('RAZOR_FANG', 'Route114'), ('UPGRADE', 'Route110'),
    ('MUSCLE_BAND', 'Route113'), ('WISE_GLASSES', 'Route115'), ('SCOPE_LENS', 'Route118'), ('EXPERT_BELT', 'VictoryRoad_B2F'),
    ('PROTEIN', 'Route112'), ('CARBOS', 'Route120'), ('CALCIUM', 'Route123'), ('ZINC', 'Route122'), ('IRON', 'JaggedPass'),
    ('HP_UP', 'RusturfTunnel'), ('PP_UP', 'Route106'), ('EXP_CANDY_M', 'Route102'), ('EXP_CANDY_L', 'Route125'),
]

FLAG_POOL = list(range(0x4A3, 0x4F0)) + list(range(0x21, 0x50))
# Maps where only Surf reaches open land (the player has it by then).
SURF_MAPS = {'Route106', 'Route122', 'Route124', 'Route125', 'Route109'}


def stone_flag(stone):
    return 'FLAG_ITEM_COOP_MEGA_' + stone


def item_flag(item, mp):
    return 'FLAG_ITEM_COOP_%s_%s' % (re.sub(r'(?<!^)(?=[A-Z])', '_', mp).upper().replace('__', '_'), item)


def balls():
    """(flag, item constant, map) for every item ball, in a fixed order."""
    out = []
    for stone, src in STONES.items():
        if src[0] == 'ball':
            out.append((stone_flag(stone), 'ITEM_' + stone, src[1]))
        elif src[0] == 'quest':
            out.append((stone_flag(stone), 'ITEM_' + stone, None))
    for item, mp in ITEMS:
        out.append((item_flag(item, mp), 'ITEM_' + item, mp))
    return out


# ---------------------------------------------------------------- data
def read(p):
    return open(os.path.join(ROOT, p), encoding='utf-8').read()


def mega_table():
    """item -> [(base species, mega species)] from the form change tables."""
    import guide as G
    out = {}
    for it, lst in G.MEGA_ITEM.items():
        for tbl, mega in lst:
            for sp, d in G.INFO.items():
                if d.get('fct') == tbl and not d['mega']:
                    out.setdefault(it, []).append((sp, mega))
                    break
    moves = []
    fc = read('src/data/pokemon/form_change_tables.h')
    for sp, move in re.findall(r'FORM_CHANGE_BATTLE_MEGA_EVOLUTION_MOVE,\s*(SPECIES_\w+),\s*(MOVE_\w+)', fc):
        moves.append((sp, move))
    return out, moves, G


def obtainable():
    d = json.load(open(os.path.join(ROOT, 'docs/coop_roster_manifest.json')))
    out = {}
    for s in d['species']:
        for f in s['forms_obtainable']:
            out[f] = s
    return out


# ---------------------------------------------------------------- writers
def write_flags(check):
    path = os.path.join(ROOT, 'include/constants/flags.h')
    text = open(path).read()
    defined = set(re.findall(r'#define (FLAG_ITEM_COOP_\w+)', text))
    pool = [f for f in FLAG_POOL if re.search(r'#define FLAG_UNUSED_0x%03X\s' % f, text)]
    errors = []
    for flag, _, _ in balls():
        if flag in defined:
            continue
        if check:
            errors.append('flag %s not allocated' % flag)
            continue
        if not pool:
            raise SystemExit('no free flags left')
        n = pool.pop(0)
        text, k = re.subn(r'#define FLAG_UNUSED_0x%03X\s+0x0*%X\s*// Unused Flag' % (n, n),
                          '#define %-59s 0x%03X // Battle Guide v2 reward (tools/coop_mega/rewards.py)' % (flag, n), text)
        if k != 1:
            raise SystemExit('could not allocate 0x%03X' % n)
    if not check:
        open(path, 'w').write(text)
    return errors


def place_balls(check):
    errors = []
    by_map = {}
    for flag, item, mp in balls():
        if mp:
            by_map.setdefault(mp, []).append((flag, item))
    for mp, lst in sorted(by_map.items()):
        path = os.path.join(ROOT, 'data/maps', mp, 'map.json')
        text = open(path).read()
        d = json.loads(text)
        changed = False
        for flag, item in lst:
            have = [o for o in d['object_events'] if o.get('flag') == flag]
            if have:
                g = place.Grid(mp)
                o = have[0]
                # The ball itself is an object: check the tile without it.
                g.objects.discard((o['x'], o['y']))
                why = g.check((o['x'], o['y']), surf=mp in SURF_MAPS)
                if why:
                    errors.append('%s on %s at (%d,%d): %s' % (flag, mp, o['x'], o['y'], why))
                if o.get('trainer_sight_or_berry_tree_id') != item:
                    errors.append('%s on %s holds %s, want %s' % (flag, mp, o.get('trainer_sight_or_berry_tree_id'), item))
                continue
            if check:
                errors.append('%s not placed on %s' % (flag, mp))
                continue
            t = place.pick(mp, surf=mp in SURF_MAPS)
            if t is None:
                errors.append('no free tile on %s for %s' % (mp, flag))
                continue
            g = place.Grid(mp)
            d['object_events'].append({
                'graphics_id': 'OBJ_EVENT_GFX_ITEM_BALL', 'x': t[0], 'y': t[1], 'elevation': g.elev[t],
                'movement_type': 'MOVEMENT_TYPE_LOOK_AROUND', 'movement_range_x': 0, 'movement_range_y': 0,
                'trainer_type': 'TRAINER_TYPE_NONE', 'trainer_sight_or_berry_tree_id': item,
                'script': 'Common_EventScript_FindItem', 'flag': flag})
            # Write after each ball so the next pick sees it.
            open(path, 'w').write(json.dumps(d, indent=2, ensure_ascii=False) + '\n')
            changed = True
    return errors


def stock_rows():
    """(stone, tier, price, quest name or None) for the Exchange's MEGA counter
    and {quest name: (C condition, doc)} for its unlocks."""
    rows, quests = [], {}
    for stone, src in STONES.items():
        if src[0] == 'exchange':
            rows.append((stone, src[1], PRICE[src[1]], None))
            continue
        q = 'FOUND_' + stone
        if src[0] == 'quest':
            quests[q] = (QUESTS[src[1]][0] + ' || FlagGet(%s)' % stone_flag(stone), 'after ' + QUESTS[src[1]][1])
        else:
            quests[q] = ('FlagGet(%s)' % stone_flag(stone), 'once found (%s)' % src[2])
        rows.append((stone, EX5, PRICE[EX5], q))
    return rows, quests


def hint(stone):
    """Where a stone is first found, as the MEGA STONE counter says it."""
    src = STONES[stone]
    if src[0] == 'exchange':
        return {EX5: 'HERE, 5 BADGES', EX7: 'HERE, 7 BADGES', EXC: 'HERE, CHAMPION'}[src[1]]
    if src[0] == 'quest':
        return {'COAST': 'COAST CLEANUP', 'CARAVAN': 'DESERT CARAVAN',
                'PP_WATTSON': 'POWER PROBLEMS', 'SHOAL_RESCUE': 'SHOAL RESCUE'}[src[1]]
    return src[2]


def write_hints():
    out = ['// Generated by tools/coop_mega/rewards.py. Do not edit.',
           '// Where each Mega Stone is first found (the MEGA STONE counter\'s hints).',
           'static const struct MegaStoneHint sMegaStoneHints[] = {']
    for stone in STONES:
        out.append('    { ITEM_%s, COMPOUND_STRING("%s") },' % (stone, hint(stone)))
    out += ['};', '']
    return '\n'.join(out)


def manifest(megas, moves, obt, G):
    rows = []
    for it, pairs in sorted(megas.items()):
        stone = it[len('ITEM_'):]
        for base, mega in pairs:
            o = obt.get(base)
            rows.append({'species': base, 'mega': mega, 'stone': it, 'obtainable': o is not None,
                         'species_earliest_badges': o.get('earliest_badges_with_evolution') if o else None,
                         'mega_ability': __import__('overhaul').SPECIES[mega]['abilities'][0],
                         'source': STONES.get(stone), 'hint': hint(stone) if stone in STONES else None})
    for sp, move in moves:
        base = sp.replace('_MEGA', '')
        rows.append({'species': base, 'mega': sp, 'stone': None, 'move': move, 'obtainable': base in obt,
                     'source': ('move', move), 'hint': 'KNOWS %s (EXCHANGE RELEARN)' % move[5:].replace('_', ' ')})
    return rows


def write_doc(rows, item_places):
    stock, quests = stock_rows()
    L = ['# Mega Evolution: stones, sources and the extra item balls (generated)', '',
         'Generated by `tools/coop_mega/rewards.py`. The Mega Bracelet: Steven on Route 118 after the fifth badge, '
         'or the MEGA STONE counter of the Rustboro Trainer Exchange for a save that missed it. '
         'One Mega per trainer per battle.', '',
         '## Every Mega of an obtainable species', '',
         '| Species | Mega form | Stone | First source | Rebuy (Exchange MEGA STONE counter) | Price |',
         '| --- | --- | --- | --- | --- | --- |']
    price = {s: p for s, t, p, q in stock}
    unlock = {s: (t, q) for s, t, p, q in stock}
    for r in rows:
        if not r['obtainable']:
            continue
        st = r['stone'][len('ITEM_'):] if r['stone'] else None
        if r.get('move'):
            L.append('| %s | %s | (none: knows %s) | Exchange RELEARN, %s | - | - |' % (
                r['species'][8:].title(), r['mega'][8:].title(), r['move'][5:], 'Dragon Ascent is a level-1 move'))
            continue
        src = r['source']
        if src[0] == 'exchange':
            first = 'Exchange from %s' % {EX5: '5 badges', EX7: '7 badges', EXC: 'Champion'}[src[1]]
            rebuy = 'same'
        elif src[0] == 'quest':
            first = 'reward: %s' % QUESTS[src[1]][1]
            rebuy = 'from 5 badges, after the reward'
        else:
            pl = item_places.get(stone_flag(st))
            first = 'item ball, %s%s' % (src[2], ' (%d,%d)' % pl[1] if pl else '')
            rebuy = 'from 5 badges, once found'
        L.append('| %s | %s | %s | %s | %s | ₽%d |' % (r['species'][8:].replace('_', ' ').title(), r['mega'][8:].replace('_', ' ').title(),
                                                      st.replace('_', ' ').title(), first, rebuy, price[st]))
    L += ['', '## Not obtainable here (no stone placed)', '']
    for r in rows:
        if not r['obtainable']:
            L.append('- %s (%s): the species is a deferred legendary/mythical or not in this roster.' % (r['mega'][8:], r['stone'] or r.get('move')))
    L += ['', '## Extra item balls', '', '| Item | Map | Tile | Flag |', '| --- | --- | --- | --- |']
    for flag, item, mp in balls():
        if mp and not flag.startswith('FLAG_ITEM_COOP_MEGA_'):
            pl = item_places.get(flag)
            L.append('| %s | %s | %s | `%s` |' % (item[5:].replace('_', ' ').title(), mp, '(%d,%d)' % pl[1] if pl else '?', flag))
    L += ['', 'Each ball is a personal pickup (its own flag on each save): both players can take theirs, the partner\'s '
          'copy may still show on the other screen, a full bag leaves the ball where it is, and nothing is shared or synced.', '']
    return '\n'.join(L)


def placed():
    out = {}
    for flag, item, mp in balls():
        if not mp:
            continue
        d = json.load(open(os.path.join(ROOT, 'data/maps', mp, 'map.json')))
        for o in d['object_events']:
            if o.get('flag') == flag:
                out[flag] = (mp, (o['x'], o['y']))
    return out


def main():
    check = '--check' in sys.argv
    errors = []
    errors += write_flags(check)
    errors += place_balls(check)
    megas, moves, G = mega_table()
    obt = obtainable()
    # Coverage: every Mega of an obtainable species.
    for it, pairs in sorted(megas.items()):
        stone = it[len('ITEM_'):]
        for base, mega in pairs:
            if base in obt and stone not in STONES:
                errors.append('%s is obtainable but %s has no source (add it to STONES)' % (base, it))
            if G.INFO[base].get('hp') != G.INFO[mega].get('hp'):
                errors.append('%s HP %s, %s HP %s: a Mega must keep its HP' % (base, G.INFO[base].get('hp'), mega, G.INFO[mega].get('hp')))
    for stone in STONES:
        if 'ITEM_' + stone not in megas:
            errors.append('%s is not a Mega Stone of this engine' % stone)
        elif not any(b in obt for b, _ in megas['ITEM_' + stone]):
            errors.append('%s placed but no species of it is obtainable' % stone)
    for sp, move in moves:
        base = sp.replace('_MEGA', '')
        if base in obt:
            import guide as GG
            if not GG.learn_ok(base, move, 100)[0]:
                errors.append('%s needs %s, which it cannot learn' % (sp, move))
    # Quest stones must be handed out by the quest scripts.
    scripts = read('data/scripts/coop_mega.inc')
    for stone, src in STONES.items():
        if src[0] == 'quest' and stone_flag(stone) not in scripts:
            errors.append('%s: quest reward not in data/scripts/coop_mega.inc' % stone)
    rows = manifest(megas, moves, obt, G)
    item_places = placed()
    outputs = {
        'src/data/coop_mega_hints.h': write_hints(),
        'docs/COOP_MEGA_MANIFEST.md': write_doc(rows, item_places),
        'docs/coop_mega_manifest.json': json.dumps({'generated_by': 'tools/coop_mega/rewards.py', 'megas': rows,
                                                     'balls': [{'flag': f, 'item': i, 'map': m, 'tile': item_places.get(f, (None, None))[1]}
                                                               for f, i, m in balls() if m]}, indent=1, default=list) + '\n',
    }
    for p, text in outputs.items():
        full = os.path.join(ROOT, p)
        if check:
            if not os.path.exists(full) or open(full, encoding='utf-8').read() != text:
                errors.append('stale: %s (run tools/coop_mega/rewards.py)' % p)
        else:
            open(full, 'w', encoding='utf-8').write(text)
    n_ob = sum(1 for r in rows if r['obtainable'])
    for e in errors:
        print('ERROR:', e)
    print('%d Mega forms of obtainable species covered by %d stones (+%d by a move); %d item balls; %d errors' % (
        n_ob, len(STONES), sum(1 for r in rows if r.get('move') and r['obtainable']), len([b for b in balls() if b[2]]), len(errors)))
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
