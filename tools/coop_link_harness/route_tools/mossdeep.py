# mossdeep.py sx,sy tx,ty [k0,k1,k2,k3,k4]: Mossdeep Gym (Emerald) route. The game's rules
# (src/rotating_tile_puzzle.c): a floor switch (coord event) moves every
# object standing on an arrow tile of its colour one step along the arrow
# (no collision check), and each moved object turns a quarter
# anticlockwise; warp pads (warp events) link in pairs; trainers (sight 1)
# battle when the player stands in front of them, and then stay as
# obstacles. BFS over (player, object positions, trainers beaten).
# Prints harness lines: walk X Y (one tile at a time) and step_to for pads.
import json, re, struct, sys
from collections import deque
root = '/home/claude/pokemon-rom-hack/'
m = 'MossdeepCity_Gym'
S = tuple(map(int, sys.argv[1].split(','))); T = tuple(map(int, sys.argv[2].split(',')))
d = json.load(open(root + 'data/maps/%s/map.json' % m))
L = [l for l in json.load(open(root + 'data/layouts/layouts.json'))['layouts'] if l.get('id') == d['layout']][0]
b = open(root + L['blockdata_filepath'], 'rb').read(); v = struct.unpack('<%dH' % (len(b) // 2), b)
W, H = L['width'], L['height']
def col(x, y): return not (0 <= x < W and 0 <= y < H) or (v[y * W + x] >> 10) & 3
def mt(x, y): return v[y * W + x] & 0x3ff
START = 0x250
def arrow(x, y):
    """(colour, dir index 0 R 1 D 2 L 3 U) or None."""
    k = mt(x, y) - START
    if k < 0 or k // 8 >= 5 or k % 8 >= 4: return None
    return k // 8, k % 8
DXY = [(1, 0), (0, 1), (-1, 0), (0, -1)]
FACE = {'MOVEMENT_TYPE_FACE_RIGHT': 0, 'MOVEMENT_TYPE_FACE_DOWN': 1, 'MOVEMENT_TYPE_FACE_LEFT': 2, 'MOVEMENT_TYPE_FACE_UP': 3}
COLOUR = {'Yellow': 0, 'Blue': 1, 'Green': 2, 'Purple': 3, 'Red': 4}
objs = d['object_events']
pos0 = tuple((o['x'], o['y']) for o in objs)
face0 = tuple(FACE.get(o['movement_type'], -1) for o in objs)
trainer = [o.get('trainer_type', 'TRAINER_TYPE_NONE') != 'TRAINER_TYPE_NONE' for o in objs]
switches = {}
for c in d['coord_events']:
    s = c['script']
    for k, cv in COLOUR.items():
        if k + 'FloorSwitch' in s: switches[(c['x'], c['y'])] = cv
    if 'WarpToEntrance' in s: switches[(c['x'], c['y'])] = -1
warps = {}
for w in d['warp_events']:
    if w['dest_map'] == 'MAP_MOSSDEEP_CITY_GYM':
        dw = d['warp_events'][int(w['dest_warp_id'])]
        warps[(w['x'], w['y'])] = (dw['x'], dw['y'])
exits = {(w['x'], w['y']) for w in d['warp_events'] if w['dest_map'] != 'MAP_MOSSDEEP_CITY_GYM'}
NAMES = {(0, 1): 'DOWN', (0, -1): 'UP', (1, 0): 'RIGHT', (-1, 0): 'LEFT'}
def rotate(pos, face, colour):
    pos = list(pos); face = list(face)
    for i, (x, y) in enumerate(pos):
        a = arrow(x, y)
        if a and a[0] == colour:
            dx, dy = DXY[a[1]]; pos[i] = (x + dx, y + dy)
            if face[i] >= 0: face[i] = (face[i] + 3) % 4      # anticlockwise: R->U->L->D
    return tuple(pos), tuple(face)
def battles(p, pos, face, beaten):
    nb = set(beaten)
    for i in range(len(objs)):
        if trainer[i] and i not in nb and face[i] >= 0:
            dx, dy = DXY[face[i]]
            if (pos[i][0] + dx, pos[i][1] + dy) == p: nb.add(i)
    return frozenset(nb)
# All objects of a colour move together round closed arrow loops, so the
# objects' state is one step count per colour (mod the loop length).
LOOP = []
for colour in range(5):
    pos, face = pos0, face0; seen = [(pos, face)]
    for k in range(1, 64):
        pos, face = rotate(pos, face, colour)
        if pos == pos0 and face == face0: break
        seen.append((pos, face))
    LOOP.append(len(seen))
def state_objs(ks):
    pos, face = pos0, face0
    for colour, k in enumerate(ks):
        for _ in range(k): pos, face = rotate(pos, face, colour)
    return pos, face
cache = {}
def objs_at(ks):
    if ks not in cache: cache[ks] = state_objs(ks)
    return cache[ks]
K0 = tuple(map(int, sys.argv[3].split(','))) if len(sys.argv) > 3 else (0, 0, 0, 0, 0)
s0 = (S, K0); prev = {s0: None}; q = deque([s0]); goal = None
while q:
    c = q.popleft(); p, ks = c
    if p == T: goal = c; break
    pos, face = objs_at(ks); occupied = set(pos)
    for (dx, dy), name in NAMES.items():
        n = (p[0] + dx, p[1] + dy)
        if n != T and (col(*n) or n in exits): continue
        if n in occupied: continue
        np_, nks = n, ks; act = ('walk', n)
        if n in switches:
            if switches[n] < 0: continue
            k = list(ks); k[switches[n]] = (k[switches[n]] + 1) % LOOP[switches[n]]; nks = tuple(k)
            if n in set(objs_at(nks)[0]): continue
        elif n in warps:
            np_ = warps[n]; act = ('pad', name, np_)
        st = (np_, nks)
        if st not in prev: prev[st] = (c, act); q.append(st)
if not goal: print('NO PATH', len(prev), LOOP); sys.exit(1)
acts = []; c = goal
while prev[c]: acts.append(prev[c][1]); c = prev[c][0]
for a in reversed(acts):
    if a[0] == 'walk': print('walk %d %d' % a[1])
    else: print('step_to %s %d %d' % (a[1], a[2][0], a[2][1]))
print('# loops', LOOP, 'switch counts', goal[1])
