# gates.py Map sx,sy tx,ty [blocked x,y ...]: path through a rotating-gate
# puzzle (Fortree Gym), with the game's gate rules (src/rotating_gate.c) and
# trainer sight (a trainer who sees the player walks up next to them and
# stays there). Prints one tile per line ("x y"), plus the trainers met.
import sys, json, re, struct
from collections import deque
root = '/home/claude/pokemon-rom-hack/'
m = sys.argv[1]
S = tuple(map(int, sys.argv[2].split(','))); T = tuple(map(int, sys.argv[3].split(',')))
extra = {tuple(map(int, a.split(','))) for a in sys.argv[4:]}
d = json.load(open(root + 'data/maps/%s/map.json' % m))
L = [l for l in json.load(open(root + 'data/layouts/layouts.json'))['layouts'] if l.get('id') == d['layout']][0]
W, H = L['width'], L['height']
b = open(root + L['blockdata_filepath'], 'rb').read(); v = struct.unpack('<%dH' % (len(b) // 2), b)
def col(x, y):
    if not (0 <= x < W and 0 <= y < H): return 1
    return (v[y * W + x] >> 10) & 3
# Gate config (Fortree), parsed from the source.
src = open(root + 'src/rotating_gate.c').read()
SH = ['L1', 'L2', 'L3', 'L4', 'T1', 'T2', 'T3', 'T4']
cfgname = {'FortreeCity_Gym': 'Fortree'}[m]
blk = re.search(r'sRotatingGate_%sPuzzleConfig\[\] =\s*\{(.*?)\};' % cfgname, src, re.S).group(1)
gates = [(int(a), int(bb), SH.index(c), int(o)) for a, bb, c, o in re.findall(r'\{\s*(\d+),\s*(\d+),\s*GATE_SHAPE_(\w+),\s*GATE_ORIENTATION_(\d+)\}', blk)]
gates = [(x, y, s, o // 90) for x, y, s, o in gates]
ARM = [[1,0,1,0,0,0,0,0],[1,1,1,0,0,0,0,0],[1,0,1,1,0,0,0,0],[1,1,1,1,0,0,0,0],
       [1,0,1,0,1,0,0,0],[1,1,1,0,1,0,0,0],[1,0,1,1,1,0,0,0],[1,0,1,0,1,1,0,0]]
CW = [(0,-1),(1,-2),(0,0),(1,0),(-1,0),(-1,1),(-1,-1),(-2,-1)]
ACW = [(-1,-1),(-1,-2),(0,-1),(1,-1),(0,0),(0,1),(-1,0),(-2,0)]
N_, E_, S_, W_ = 0, 1, 2, 3
NONE = None
def R(rot, arm, lng): return (rot, arm, lng)  # rot 1 = anticlockwise, 2 = clockwise
RN = [NONE]*4 + [R(2,W_,1), R(2,W_,0), R(1,E_,0), R(1,E_,1)] + [NONE]*8
RS = [NONE]*8 + [R(1,W_,1), R(1,W_,0), R(2,E_,0), R(2,E_,1)] + [NONE]*4
RW = [NONE, R(1,N_,1), NONE, NONE, NONE, R(1,N_,0), NONE, NONE, NONE, R(2,S_,0), NONE, NONE, NONE, R(2,S_,1), NONE, NONE]
RE = [NONE, NONE, R(2,N_,1), NONE, NONE, NONE, R(2,N_,0), NONE, NONE, NONE, R(1,S_,0), NONE, NONE, NONE, R(1,S_,1), NONE]
INFO = {'U': RN, 'D': RS, 'L': RW, 'R': RE}
def can_rotate(g, ori, rot):
    gx, gy, sh, _ = gates[g]
    pos = ACW if rot == 1 else CW
    for i in range(4):
        for j in range(2):
            if ARM[sh][2 * i + j]:
                ax, ay = pos[2 * ((ori + i) % 4) + j]
                if col(gx + ax, gy + ay) == 1: return False
    return True
def has_arm(g, ori, arm, lng):
    return ARM[gates[g][2]][((arm - ori + 4) % 4) * 2 + lng]
def gate_step(dn, x, y, oris):
    """None: blocked; else the new orientations."""
    for g, (gx, gy, sh, _) in enumerate(gates):
        if gx - 2 <= x <= gx + 1 and gy - 2 <= y <= gy + 1:
            info = INFO[dn][(y - gy + 2) * 4 + (x - gx + 2)]
            if info is None: continue
            rot, arm, lng = info
            if has_arm(g, oris[g], arm, lng):
                if can_rotate(g, oris[g], rot):
                    o = list(oris); o[g] = (o[g] - 1) % 4 if rot == 1 else (o[g] + 1) % 4
                    return tuple(o)
                return None
    return oris
# Trainers and other objects.
DIRS = {'U': (0, -1), 'D': (0, 1), 'L': (-1, 0), 'R': (1, 0)}
FACE = {'MOVEMENT_TYPE_FACE_DOWN': 'D', 'MOVEMENT_TYPE_FACE_UP': 'U', 'MOVEMENT_TYPE_FACE_LEFT': 'L', 'MOVEMENT_TYPE_FACE_RIGHT': 'R'}
objs = {}; trainers = []
for o in d['object_events']:
    objs[(o['x'], o['y'])] = o.get('script', '')
    if o.get('trainer_type', 'TRAINER_TYPE_NONE') != 'TRAINER_TYPE_NONE' and o['movement_type'] in FACE:
        if not __import__("os").environ.get("NOSIGHT"): trainers.append([o['x'], o['y'], FACE[o['movement_type']], int(o['trainer_sight_or_berry_tree_id']), o['script'], False])
def search(start, oris, blockers):
    st = (start, oris); prev = {st: None}; q = deque([st])
    while q:
        c = q.popleft(); (x, y), o = c
        if (x, y) == T:
            path = []
            while prev[c]: path.append(c); c = prev[c]
            return path[::-1]
        for dn, (dx, dy) in DIRS.items():
            nx, ny = x + dx, y + dy
            if (nx, ny) != T and (col(nx, ny) or (nx, ny) in blockers): continue
            no = gate_step(dn, nx, ny, o)
            if no is None: continue
            n = ((nx, ny), no)
            if n in prev: continue
            prev[n] = c; q.append(n)
    return None
pos = S; oris = tuple(g[3] for g in gates)
blockers = set(objs) | extra
out = []; met = []
def trigger(p):
    """The first undefeated trainer who sees p walks up; True if one did."""
    for t in trainers:
        if t[5]: continue
        dx, dy = DIRS[t[2]]
        for k in range(1, t[3] + 1):
            cx, cy = t[0] + dx * k, t[1] + dy * k
            if (cx, cy) == p:
                t[5] = True
                blockers.discard((t[0], t[1]))
                nt = (p[0] - dx, p[1] - dy)
                blockers.add(nt); met.append((t[4], (t[0], t[1]), nt, p))
                return True
            if col(cx, cy) or (cx, cy) in blockers: break
    return False
for it in range(40):
    while trigger(pos): pass
    path = search(pos, oris, blockers)
    if path is None: print('NO PATH from', pos); sys.exit(1)
    replan = False
    for (p, o) in path:
        out.append(p); pos, oris = p, o
        if trigger(pos): replan = True; break
    if not replan: break
for p in out: print(p[0], p[1])
for mt in met: print('# trainer', mt[0], 'from', mt[1], 'to', mt[2], 'seeing', mt[3])
