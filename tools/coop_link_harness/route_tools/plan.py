# plan.py MAPS StartMap x,y GoalMap x,y [free=Map:x,y;...]
# Route over several maps joined by warps, with the harness walk rules
# (collision, elevation 0/15 transitions, ledges, objects block, no water),
# arrow warps (taken by moving in the arrow's direction from the tile) and
# step-on warps (doors, ladders, stairs, holes). Prints harness lines:
#   walk X Y    (to the tile the warp is taken from)
#   cross DIR GROUP NUM
# and finally walk to the goal.
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
M = {}
for m in sys.argv[1].split(','):
    d = json.load(open(root + 'data/maps/%s/map.json' % m))
    L = [l for l in json.load(open(root + 'data/layouts/layouts.json'))['layouts'] if l.get('id') == __import__('os').environ.get('LAYOUT', d['layout'])][0]
    pa = attrs(root + 'data/tilesets/primary/%s/metatile_attributes.bin' % folder(L['primary_tileset']))
    sa = attrs(root + 'data/tilesets/secondary/%s/metatile_attributes.bin' % folder(L['secondary_tileset']))
    b = open(root + L['blockdata_filepath'], 'rb').read(); v = struct.unpack('<%dH' % (len(b) // 2), b)
    M[m] = dict(d=d, W=L['width'], H=L['height'], v=list(v), pa=pa, sa=sa, id=d['id'])
ids = {M[m]['id']: m for m in M}
free = set()
for f in (sys.argv[6].split(';') if len(sys.argv) > 6 and sys.argv[6] else []):
    mm, xy = f.split(':'); free.add((mm,) + tuple(map(int, xy.split(','))))
def tile(m, x, y):
    mm = M[m]; e = mm['v'][y * mm['W'] + x]; mid = e & 0x3ff
    a = (mm['pa'] + [0] * (512 - len(mm['pa'])))[mid] if mid < 512 else mm['sa'][mid - 512]
    return (e >> 12) & 15, (e >> 10) & 3, names[a & 0xff]
def water(bh): return 'BRIDGE' not in bh and bh != 'MB_PUDDLE' and bh != 'MB_SHALLOW_WATER' and any(k in bh for k in ('OCEAN', 'POND', 'DEEP_WATER', 'WATERFALL', 'CURRENT', '_WATER', 'SEAWEED'))
warps = {}
for m in M:
    for w in M[m]['d']['warp_events']:
        dm = w['dest_map']
        if dm in ids:
            dw = M[ids[dm]]['d']['warp_events'][int(w['dest_warp_id'])]
            warps[(m, w['x'], w['y'])] = (ids[dm], dw['x'], dw['y'])
objs = {(m, o['x'], o['y']) for m in M for o in M[m]['d']['object_events']} - free
DIRS = {'D': (0, 1), 'U': (0, -1), 'L': (-1, 0), 'R': (1, 0)}
JUMP = {'D': 'MB_JUMP_SOUTH', 'U': 'MB_JUMP_NORTH', 'L': 'MB_JUMP_WEST', 'R': 'MB_JUMP_EAST'}
ARROW = {'MB_SOUTH_ARROW_WARP': 'D', 'MB_NORTH_ARROW_WARP': 'U', 'MB_WEST_ARROW_WARP': 'L', 'MB_EAST_ARROW_WARP': 'R',
         'MB_WATER_SOUTH_ARROW_WARP': 'D', 'MB_DEEP_SOUTH_WARP': 'D'}
FULL = {'U': 'UP', 'D': 'DOWN', 'L': 'LEFT', 'R': 'RIGHT'}
start = (sys.argv[2],) + tuple(map(int, sys.argv[3].split(','))); goal = (sys.argv[4],) + tuple(map(int, sys.argv[5].split(',')))
s0 = start + (tile(*start)[0],)
prev = {s0: None}; q = deque([s0]); found = None
while q:
    c = q.popleft(); m, x, y, ce = c
    if (m, x, y) == goal: found = c; break
    e0, c0, bh0 = tile(m, x, y)
    for dn, (dx, dy) in DIRS.items():
        # An arrow warp: moving its way from the tile.
        if (m, x, y) in warps and ARROW.get(bh0) == dn:
            t = warps[(m, x, y)]; n = t + (tile(*t)[0],)
            if n not in prev: prev[n] = (c, 'X' + dn, (m, x, y)); q.append(n)
            continue
        nx, ny = x + dx, y + dy
        if not (0 <= nx < M[m]['W'] and 0 <= ny < M[m]['H']): continue
        e, col, bh = tile(m, nx, ny); ne = ce
        if 'JUMP' in bh:
            if bh != JUMP[dn]: continue
            nx, ny = nx + dx, ny + dy; e, col, bh = tile(m, nx, ny); ne = e
        n3 = (m, nx, ny)
        if n3 in objs and n3 != goal: continue
        if n3 in warps and bh not in ARROW:
            # A step-on warp (door, ladder, stairs, hole, warp pad).
            if not (e in (0, 15) or ne in (0, 15) or e == ne): continue
            t = warps[n3]; n = t + (tile(*t)[0],)
            if n not in prev: prev[n] = (c, 'W' + dn, n3); q.append(n)
            continue
        if col and n3 != goal: continue
        if water(bh) or bh in ('MB_MUDDY_SLOPE', 'MB_BUMPY_SLOPE') and dn != 'D': continue
        if not (e in (0, 15) or ne in (0, 15) or e == ne): continue
        n = (m, nx, ny, ne if e == 15 else e)
        if n not in prev: prev[n] = (c, dn, None); q.append(n)
if not found:
    print('NO PATH; reached maps:', sorted({k[0] for k in prev})); sys.exit(1)
legs = []; c = found
while prev[c]: legs.append((prev[c][0], prev[c][1], prev[c][2], c)); c = prev[c][0]
legs.reverse()
for p, step, wt, n in legs:
    if step[0] in 'XW':
        g = groups['MAP_' + re.sub(r'(?<!^)(?=[A-Z])', '_', n[0]).upper()] if False else None
        dest = M[n[0]]['id']
        gi, ni = groups[n[0]]
        print('walk %d %d   # %s' % (p[1], p[2], p[0]))
        if n[0] == p[0]:
            print('step_to %s %d %d   # warp pad on the same map' % (FULL[step[1]], n[1], n[2]))
        else:
            print('cross %s %d %d   # -> %s (%d,%d)' % (FULL[step[1]], gi, ni, n[0], n[1], n[2]))
print('walk %d %d   # goal %s' % (goal[1], goal[2], goal[0]))
