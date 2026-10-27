# mcave.py MAPS StartMap sx,sy[,surf] GoalMap gx,gy: a route over several
# cave maps joined by warps (cave.py's rules on each map: Strength boulders,
# Rock Smash rocks, Surf from a bank, ledges, elevation; plus the one-way
# MB_IMPASSABLE_* edges). Boulders and rocks are temporary-flag objects:
# leaving a map puts them back, so each map's are reset on arrival.
# BFS over (map, tile, surfing, elevation, boulders, rocks broken).
# Prints harness lines:
#   walk X Y | surf DIR | step_to DIR X Y | strength DIR | push DIR |
#   smash DIR | cross DIR GROUP NUM (a step onto a warp)
# FREE="Map:x,y ..." drops objects (people who have moved).
import json, re, struct, sys, os
from collections import deque
root = '/home/claude/pokemon-rom-hack/'
names = [l.strip().split(',')[0].split()[0] for l in open(root + 'include/constants/metatile_behaviors.h') if l.strip().startswith('MB_')]
def folder(n): return re.sub(r'(?<!^)(?=[A-Z])', '_', n.replace('gTileset_', '')).lower()
def attrs(p):
    b = open(p, 'rb').read(); return list(struct.unpack('<%dH' % (len(b) // 2), b))
groups = {}
mg = json.load(open(root + 'data/maps/map_groups.json'))
for gi, g in enumerate(mg['group_order']):
    for ni, m in enumerate(mg[g]): groups[m] = (gi, ni)
FREE = set()
for t in os.environ.get('FREE', '').split():
    mm, xy = t.split(':'); FREE.add((mm,) + tuple(map(int, xy.split(','))))
M = {}
for m in sys.argv[1].split(','):
    d = json.load(open(root + 'data/maps/%s/map.json' % m))
    L = [l for l in json.load(open(root + 'data/layouts/layouts.json'))['layouts'] if l.get('id') == d['layout']][0]
    b = open(root + L['blockdata_filepath'], 'rb').read()
    M[m] = dict(d=d, W=L['width'], H=L['height'], v=struct.unpack('<%dH' % (len(b) // 2), b),
                pa=attrs(root + 'data/tilesets/primary/%s/metatile_attributes.bin' % folder(L['primary_tileset'])),
                sa=attrs(root + 'data/tilesets/secondary/%s/metatile_attributes.bin' % folder(L['secondary_tileset'])))
    objs = d['object_events']
    M[m]['B0'] = frozenset((o['x'], o['y']) for o in objs if 'PUSHABLE_BOULDER' in o['graphics_id'])
    M[m]['R0'] = frozenset((o['x'], o['y']) for o in objs if 'BREAKABLE_ROCK' in o['graphics_id'])
    M[m]['others'] = {(o['x'], o['y']) for o in objs if 'PUSHABLE_BOULDER' not in o['graphics_id'] and 'BREAKABLE_ROCK' not in o['graphics_id'] and (m, o['x'], o['y']) not in FREE}
ids = {M[m]['d']['id']: m for m in M}
warps = {}
for m in M:
    for w in M[m]['d']['warp_events']:
        if w['dest_map'] in ids:
            dm = ids[w['dest_map']]; dw = M[dm]['d']['warp_events'][int(w['dest_warp_id'])]
            warps[(m, w['x'], w['y'])] = (dm, dw['x'], dw['y'])
def tile(m, x, y):
    mm = M[m]; e = mm['v'][y * mm['W'] + x]; mid = e & 0x3ff
    at = (mm['pa'] + [0] * (512 - len(mm['pa'])))[mid] if mid < 512 else mm['sa'][mid - 512]
    return (e >> 12) & 15, (e >> 10) & 3, names[at & 0xff]
def inside(m, x, y): return 0 <= x < M[m]['W'] and 0 <= y < M[m]['H']
def water(bh): return bh in ('MB_POND_WATER', 'MB_INTERIOR_DEEP_WATER', 'MB_DEEP_WATER', 'MB_SOOTOPOLIS_DEEP_WATER', 'MB_OCEAN_WATER', 'MB_NO_SURFACING', 'MB_SEAWEED')
SIDE = {'S': ('MB_IMPASSABLE_SOUTH', 'MB_IMPASSABLE_SOUTHEAST', 'MB_IMPASSABLE_SOUTHWEST', 'MB_IMPASSABLE_SOUTH_AND_NORTH'),
        'N': ('MB_IMPASSABLE_NORTH', 'MB_IMPASSABLE_NORTHEAST', 'MB_IMPASSABLE_NORTHWEST', 'MB_IMPASSABLE_SOUTH_AND_NORTH'),
        'W': ('MB_IMPASSABLE_WEST', 'MB_IMPASSABLE_NORTHWEST', 'MB_IMPASSABLE_SOUTHWEST', 'MB_IMPASSABLE_WEST_AND_EAST'),
        'E': ('MB_IMPASSABLE_EAST', 'MB_IMPASSABLE_NORTHEAST', 'MB_IMPASSABLE_SOUTHEAST', 'MB_IMPASSABLE_WEST_AND_EAST')}
DIRS = {'DOWN': (0, 1, 'S', 'N'), 'UP': (0, -1, 'N', 'S'), 'LEFT': (-1, 0, 'W', 'E'), 'RIGHT': (1, 0, 'E', 'W')}
JUMP = {'DOWN': 'MB_JUMP_SOUTH', 'UP': 'MB_JUMP_NORTH', 'LEFT': 'MB_JUMP_WEST', 'RIGHT': 'MB_JUMP_EAST'}
def elev_ok(e0, e1): return e0 in (0, 15) or e1 in (0, 15) or e0 == e1
a = sys.argv[3].split(','); S = (sys.argv[2], int(a[0]), int(a[1])); surf0 = len(a) > 2 and a[2] == 'surf'
G = (sys.argv[4],) + tuple(map(int, sys.argv[5].split(',')))
s0 = (S, surf0, tile(*S)[0], M[S[0]]['B0'], M[S[0]]['R0']); prev = {s0: None}; q = deque([s0]); goal = None
while q:
    c = q.popleft(); p, sf, ce, bs, rs = c; m = p[0]
    if p == G: goal = c; break
    cb = tile(*p)[2]
    for dn, (dx, dy, side, opp) in DIRS.items():
        n = (m, p[1] + dx, p[2] + dy)
        if not inside(*n): continue
        if cb in SIDE[side]: continue
        e, col, bh = tile(*n)
        if bh in SIDE[opp]: continue
        if n[1:] in rs:
            if sf: continue
            st = (p, sf, ce, bs, rs - {n[1:]}); act = ('smash', dn, None)
        elif n[1:] in bs:
            if sf: continue
            bx, by = n[1] + dx, n[2] + dy
            if not inside(m, bx, by): continue
            be, bcol, bbh = tile(m, bx, by)
            if bcol or (bx, by) in bs or (bx, by) in rs or (bx, by) in M[m]['others'] or (m, bx, by) in warps or water(bbh): continue
            st = (p, sf, ce, frozenset(bs - {n[1:]} | {(bx, by)}), rs); act = ('push', dn, None)
        else:
            ne = ce
            if 'JUMP' in bh:
                if bh != JUMP[dn] or sf: continue
                n = (m, n[1] + dx, n[2] + dy)
                if not inside(*n): continue
                e, col, bh = tile(*n); ne = e
            if n[1:] in M[m]['others'] or n[1:] in bs or n[1:] in rs: continue
            if n in warps and n != G:
                if not elev_ok(e, ne) and not sf: continue
                t = warps[n]
                st = (t, False, tile(*t)[0], M[t[0]]['B0'], M[t[0]]['R0']); act = ('warp', dn, n)
                if st not in prev: prev[st] = (c, act); q.append(st)
                continue
            if col and n != G: continue
            if water(bh):
                act = ('step', dn, None) if sf else ('surf', dn, None); nsf = True
            else:
                if bh in ('MB_MUDDY_SLOPE', 'MB_BUMPY_SLOPE') and dn != 'DOWN': continue
                if not sf and not elev_ok(e, ne): continue
                act = ('walk', dn, None); nsf = False
            if act[0] == 'step': act = ('walk', dn, None)
            st = (n, nsf, (ne if e == 15 else e) if not nsf else e, bs, rs)
        if st not in prev: prev[st] = (c, act); q.append(st)
if not goal: print('NO PATH; maps reached:', sorted({k[0][0] for k in prev})); sys.exit(1)
acts = []; c = goal
while prev[c]: acts.append((prev[c][1], c[0])); c = prev[c][0]
acts.reverse()
pushed = set(); pending = None; cur = S[0]
for (kind, dn, w), pos in acts:
    if kind == 'walk': pending = pos; continue
    if pending: print('walk %d %d' % pending[1:]); pending = None
    if kind == 'surf': print('surf %s' % dn)
    elif kind == 'smash': print('smash %s' % dn)
    elif kind == 'push':
        print(('push %s' if cur in pushed else 'strength %s') % dn); pushed.add(cur)
    elif kind == 'warp':
        gi, ni = groups[pos[0]]
        print('cross %s %d %d   # (%d,%d) -> %s (%d,%d)' % (dn, gi, ni, w[1], w[2], pos[0], pos[1], pos[2]))
        cur = pos[0]; pushed.discard(cur)
if pending: print('walk %d %d' % pending[1:])
