# mach.py Map sx,sy tx,ty: one continuous Mach Bike ride from a standstill at
# S to the door T (src/bike.c, src/field_tasks.c): the bike's speed is the
# count of moves made without stopping (0, 1, then 2 = fastest; turns keep
# it); a cracked floor breaks under anything slower than fastest and turns
# into a hole a few frames after the bike passes, so each cracked tile is
# crossed once, at full speed (here: from the third move on). Holes and
# walls stop the ride. Prints the move letters for the harness `ride`.
import json, re, struct, sys
from collections import deque
root = '/home/claude/pokemon-rom-hack/'
names = [l.strip().split(',')[0].split()[0] for l in open(root + 'include/constants/metatile_behaviors.h') if l.strip().startswith('MB_')]
def folder(n): return re.sub(r'(?<!^)(?=[A-Z])', '_', n.replace('gTileset_', '')).lower()
def attrs(p):
    b = open(p, 'rb').read(); return list(struct.unpack('<%dH' % (len(b) // 2), b))
m = sys.argv[1]
S = tuple(map(int, sys.argv[2].split(','))); T = tuple(map(int, sys.argv[3].split(',')))
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
objs = {(o['x'], o['y']) for o in d['object_events']}
warps = {(w['x'], w['y']) for w in d['warp_events']}
DIRS = {'D': (0, 1), 'U': (0, -1), 'L': (-1, 0), 'R': (1, 0)}
# State (tile, speed); a cracked tile already on this path (walked back
# up the parents) cannot be used again. Not complete, but small and quick.
def on_path(st, n):
    while st is not None:
        if st[0] == n: return True
        st = prev[st][0] if prev[st] else None
    return False
s0 = (S, 0); prev = {s0: None}; q = deque([s0]); goal = None
while q:
    c = q.popleft(); p, k = c
    if p == T: goal = c; break
    for dn, (dx, dy) in DIRS.items():
        n = (p[0] + dx, p[1] + dy)
        if not (0 <= n[0] < W and 0 <= n[1] < H): continue
        e, col, bh = tile(*n)
        if n != T and (col or n in objs or n in warps): continue
        if 'HOLE' in bh: continue
        if bh == 'MB_CRACKED_FLOOR' and (k < 2 or on_path(c, n)): continue
        st = (n, min(k + 1, 2))
        if st not in prev: prev[st] = (c, dn); q.append(st)
if not goal: print('NO PATH', len(prev)); sys.exit(1)
moves = []; c = goal
while prev[c]: moves.append(prev[c][1]); c = prev[c][0]
print(''.join(reversed(moves)))
