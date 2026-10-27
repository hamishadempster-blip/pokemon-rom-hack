# boulders.py Map sx,sy tx,ty: Strength boulder puzzle on one map (no warps
# used): BFS over (player, boulders). Prints harness lines: "walk X Y" to the
# tile before each push and "push DIR" for the push; then the final walk.
import json, re, struct, sys
from collections import deque
root = '/home/claude/pokemon-rom-hack/'
names = [l.strip().split(',')[0].split()[0] for l in open(root + 'include/constants/metatile_behaviors.h') if l.strip().startswith('MB_')]
def folder(n): return re.sub(r'(?<!^)(?=[A-Z])', '_', n.replace('gTileset_', '')).lower()
def attrs(p):
    b = open(p, 'rb').read(); return list(struct.unpack('<%dH' % (len(b) // 2), b))
m = sys.argv[1]; S = tuple(map(int, sys.argv[2].split(','))); T = tuple(map(int, sys.argv[3].split(',')))
d = json.load(open(root + 'data/maps/%s/map.json' % m))
L = [l for l in json.load(open(root + 'data/layouts/layouts.json'))['layouts'] if l.get('id') == d['layout']][0]
pa = attrs(root + 'data/tilesets/primary/%s/metatile_attributes.bin' % folder(L['primary_tileset']))
sa = attrs(root + 'data/tilesets/secondary/%s/metatile_attributes.bin' % folder(L['secondary_tileset']))
b = open(root + L['blockdata_filepath'], 'rb').read(); v = struct.unpack('<%dH' % (len(b) // 2), b)
W, H = L['width'], L['height']
def tile(x, y):
    e = v[y * W + x]; mid = e & 0x3ff
    a = (pa + [0] * (512 - len(pa)))[mid] if mid < 512 else sa[mid - 512]
    return (e >> 12) & 15, (e >> 10) & 3, names[a & 0xff]
warps = {(w['x'], w['y']) for w in d['warp_events']}
boulders0 = frozenset((o['x'], o['y']) for o in d['object_events'] if 'BOULDER' in o['graphics_id'])
others = {(o['x'], o['y']) for o in d['object_events'] if 'BOULDER' not in o['graphics_id']}
def free(x, y):
    if not (0 <= x < W and 0 <= y < H): return False
    e, c, bh = tile(x, y)
    return not c and (x, y) not in others and (x, y) not in warps
DIRS = {'DOWN': (0, 1), 'UP': (0, -1), 'LEFT': (-1, 0), 'RIGHT': (1, 0)}
s0 = (S, boulders0); prev = {s0: None}; q = deque([s0]); goal = None
while q:
    c = q.popleft(); (x, y), bs = c
    if (x, y) == T: goal = c; break
    for dn, (dx, dy) in DIRS.items():
        nx, ny = x + dx, y + dy
        if (nx, ny) in bs:
            bx, by = nx + dx, ny + dy
            if not free(bx, by) or (bx, by) in bs: continue
            nbs = frozenset(bs - {(nx, ny)} | {(bx, by)})
            n = ((nx, ny), nbs); act = ('push', dn, (x, y))
        else:
            if not free(nx, ny) and (nx, ny) != T: continue
            e0 = tile(x, y)[0]; e1 = tile(nx, ny)[0]
            if not (e0 in (0, 15) or e1 in (0, 15) or e0 == e1): continue
            n = ((nx, ny), bs); act = ('step', dn, None)
        if n not in prev: prev[n] = (c, act); q.append(n)
if not goal: print('NO PATH'); sys.exit(1)
acts = []; c = goal
while prev[c]: acts.append(prev[c][1]); c = prev[c][0]
acts.reverse()
# Steps between pushes collapse to one walk (the harness finds the way).
for a in acts:
    if a[0] == 'push':
        print('walk %d %d' % a[2]); print('push %s' % a[1])
print('walk %d %d' % T)
print('# boulders end at', sorted(goal[1]))
