# cave.py Map sx,sy[,surf] tx,ty: one map's route with Strength boulders,
# Rock Smash rocks, Surf (starting from a bank) and water currents (a
# current tile carries a surfer along until it leaves the current).
# BFS over (player, surfing, boulders, rocks broken). Prints harness lines:
#   walk X Y          land steps (collapsed: the harness walk finds them)
#   step_to DIR X Y   one move on water (or a current's whole slide)
#   surf DIR          from the bank
#   push DIR          walk into a boulder (STRENGTH on: the first push of
#                     a map visit is "strength DIR": A, YES, then push)
#   smash DIR         A on a rock, YES (ROCK SMASH)
# Then the target. The harness walk does not know currents, so every step
# is explicit.
import json, re, struct, sys
from collections import deque
root = '/home/claude/pokemon-rom-hack/'
names = [l.strip().split(',')[0].split()[0] for l in open(root + 'include/constants/metatile_behaviors.h') if l.strip().startswith('MB_')]
def folder(n): return re.sub(r'(?<!^)(?=[A-Z])', '_', n.replace('gTileset_', '')).lower()
def attrs(p):
    b = open(p, 'rb').read(); return list(struct.unpack('<%dH' % (len(b) // 2), b))
m = sys.argv[1]
a = sys.argv[2].split(','); S = (int(a[0]), int(a[1])); surf0 = len(a) > 2 and a[2] == 'surf'
T = tuple(map(int, sys.argv[3].split(',')))
d = json.load(open(root + 'data/maps/%s/map.json' % m))
L = [l for l in json.load(open(root + 'data/layouts/layouts.json'))['layouts'] if l.get('id') == __import__('os').environ.get('LAYOUT', d['layout'])][0]
pa = attrs(root + 'data/tilesets/primary/%s/metatile_attributes.bin' % folder(L['primary_tileset']))
sa = attrs(root + 'data/tilesets/secondary/%s/metatile_attributes.bin' % folder(L['secondary_tileset']))
b = open(root + L['blockdata_filepath'], 'rb').read(); v = struct.unpack('<%dH' % (len(b) // 2), b)
W, H = L['width'], L['height']
def tile(x, y):
    e = v[y * W + x]; mid = e & 0x3ff
    at = (pa + [0] * (512 - len(pa)))[mid] if mid < 512 else sa[mid - 512]
    return (e >> 12) & 15, (e >> 10) & 3, names[at & 0xff]
def inside(x, y): return 0 <= x < W and 0 <= y < H
def water(bh): return bh in ('MB_POND_WATER', 'MB_INTERIOR_DEEP_WATER', 'MB_DEEP_WATER', 'MB_SOOTOPOLIS_DEEP_WATER', 'MB_OCEAN_WATER', 'MB_NO_SURFACING', 'MB_SEAWEED', 'MB_EASTWARD_CURRENT', 'MB_WESTWARD_CURRENT', 'MB_NORTHWARD_CURRENT', 'MB_SOUTHWARD_CURRENT')
CUR = {'MB_EASTWARD_CURRENT': (1, 0), 'MB_WESTWARD_CURRENT': (-1, 0), 'MB_NORTHWARD_CURRENT': (0, -1), 'MB_SOUTHWARD_CURRENT': (0, 1)}
DIRS = {'DOWN': (0, 1), 'UP': (0, -1), 'LEFT': (-1, 0), 'RIGHT': (1, 0)}
JUMP = {'DOWN': 'MB_JUMP_SOUTH', 'UP': 'MB_JUMP_NORTH', 'LEFT': 'MB_JUMP_WEST', 'RIGHT': 'MB_JUMP_EAST'}
warps = {(w['x'], w['y']) for w in d['warp_events']}
B0 = frozenset((o['x'], o['y']) for o in d['object_events'] if 'PUSHABLE_BOULDER' in o['graphics_id'])
R0 = frozenset((o['x'], o['y']) for o in d['object_events'] if 'BREAKABLE_ROCK' in o['graphics_id'])
import os
FREE = {tuple(map(int, t.split(','))) for t in os.environ.get('FREE', '').split()}
others = {(o['x'], o['y']) for o in d['object_events'] if 'PUSHABLE_BOULDER' not in o['graphics_id'] and 'BREAKABLE_ROCK' not in o['graphics_id']} - FREE
HAS_CURRENTS = any(tile(x, y)[2] in CUR for x in range(W) for y in range(H))
def elev_ok(e0, e1): return e0 in (0, 15) or e1 in (0, 15) or e0 == e1
s0 = (S, surf0, tile(*S)[0], B0, R0); prev = {s0: None}; q = deque([s0]); goal = None
while q:
    c = q.popleft(); p, sf, ce, bs, rs = c
    if p == T: goal = c; break
    for dn, (dx, dy) in DIRS.items():
        n = (p[0] + dx, p[1] + dy)
        if not inside(*n): continue
        e, col, bh = tile(*n)
        if n in rs:
            if sf: continue
            st = (p, sf, ce, bs, rs - {n}); act = ('smash', dn)
        elif n in bs:
            if sf: continue
            bx, by = n[0] + dx, n[1] + dy
            if not inside(bx, by): continue
            be, bcol, bbh = tile(bx, by)
            if bcol or (bx, by) in bs or (bx, by) in rs or (bx, by) in others or (bx, by) in warps or water(bbh): continue
            st = (p, sf, ce, frozenset(bs - {n} | {(bx, by)}), rs); act = ('push', dn)
        else:
            ne = ce
            if 'JUMP' in bh:
                if bh != JUMP[dn] or sf: continue
                n = (n[0] + dx, n[1] + dy)
                if not inside(*n): continue
                e, col, bh = tile(*n); ne = e
            if n in others or n in bs or n in rs: continue
            if n in warps and n != T: continue
            if col and n != T: continue
            if water(bh):
                act = ('step', dn) if sf else ('surf', dn)
                nsf = True
            else:
                if bh in ('MB_MUDDY_SLOPE', 'MB_BUMPY_SLOPE') and dn != 'DOWN': continue
                if not sf and not elev_ok(e, ne): continue
                act = ('step', dn); nsf = False
            # A current carries the surfer on.
            guard = 0
            while nsf and bh in CUR and guard < 64:
                cx, cy = CUR[bh]; nn = (n[0] + cx, n[1] + cy)
                if not inside(*nn): break
                e2, col2, bh2 = tile(*nn)
                if col2 or nn in others or nn in bs or nn in rs: break
                if not water(bh2):
                    # The current carries the surfer onto the bank: on foot there.
                    n, e, bh = nn, e2, bh2; nsf = False; break
                n, e, bh = nn, e2, bh2; guard += 1
            st = (n, nsf, (ne if e == 15 else e) if not nsf else e, bs, rs)
            if not nsf or (act[0] == 'step' and not HAS_CURRENTS): act = ('walk', dn)
        if st not in prev: prev[st] = (c, act); q.append(st)
if not goal: print('NO PATH', len(prev)); sys.exit(1)
acts = []; c = goal
while prev[c]: acts.append((prev[c][1], c[0])); c = prev[c][0]
acts.reverse()
pushed = False
pending = None   # consecutive land steps: one walk to the last of them
for (kind, dn), pos in acts:
    if kind == 'walk': pending = pos; continue
    if pending: print('walk %d %d' % pending); pending = None
    if kind == 'step': print('step_to %s %d %d' % (dn, pos[0], pos[1]))
    elif kind == 'surf': print('surf %s' % dn)
    elif kind == 'smash': print('smash %s' % dn)
    elif kind == 'push':
        print(('push %s' if pushed else 'strength %s') % dn); pushed = True
if pending: print('walk %d %d' % pending)
