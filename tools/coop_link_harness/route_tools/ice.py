# ice.py: Sootopolis Gym 1F ice route (src/field_tasks.c, the gym's
# scripts): each thin-ice tile cracks when stepped on and adds one to
# VAR_ICE_STEP_COUNT; a cracked tile breaks (fall to B1F). The stairs of a
# section open when every ice tile of that section has been stepped on once,
# so each section is a path over all its ice tiles exactly once, from the
# tile the player enters on to the tile below the next stairs.
# Prints harness lines: one walk per tile, then a wait for the stairs.
import json, re, struct, sys
root = '/home/claude/pokemon-rom-hack/'
names = [l.strip().split(',')[0].split()[0] for l in open(root + 'include/constants/metatile_behaviors.h') if l.strip().startswith('MB_')]
def folder(n): return re.sub(r'(?<!^)(?=[A-Z])', '_', n.replace('gTileset_', '')).lower()
def attrs(p):
    b = open(p, 'rb').read(); return list(struct.unpack('<%dH' % (len(b) // 2), b))
d = json.load(open(root + 'data/maps/SootopolisCity_Gym_1F/map.json'))
L = [l for l in json.load(open(root + 'data/layouts/layouts.json'))['layouts'] if l.get('id') == d['layout']][0]
pa = attrs(root + 'data/tilesets/primary/%s/metatile_attributes.bin' % folder(L['primary_tileset']))
sa = attrs(root + 'data/tilesets/secondary/%s/metatile_attributes.bin' % folder(L['secondary_tileset']))
b = open(root + L['blockdata_filepath'], 'rb').read(); v = struct.unpack('<%dH' % (len(b) // 2), b)
W, H = L['width'], L['height']
def beh(x, y):
    e = v[y * W + x]; mid = e & 0x3ff
    return names[((pa + [0] * 512)[mid] if mid < 512 else sa[mid - 512]) & 0xff]
ice = {(x, y) for x in range(W) for y in range(H) if beh(x, y) == 'MB_THIN_ICE'}
def section(s):
    seen = {s}; st = [s]
    while st:
        x, y = st.pop()
        for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if n in ice and n not in seen: seen.add(n); st.append(n)
    return seen
def ham(s, t):
    sec = section(s); path = [s]; used = {s}
    def nb(p): return [n for n in ((p[0] + 1, p[1]), (p[0] - 1, p[1]), (p[0], p[1] + 1), (p[0], p[1] - 1)) if n in sec and n not in used]
    def go():
        if len(path) == len(sec): return path[-1] == t
        cand = sorted(nb(path[-1]), key=lambda n: len(nb(n)))
        for n in cand:
            if n == t and len(path) + 1 < len(sec): continue
            path.append(n); used.add(n)
            if go(): return True
            path.pop(); used.discard(n)
        return False
    assert go(), 'no path'
    return path
# (entry tile, tile below the stairs) per section, bottom to top.
for s, t in (((8, 19), (8, 17)), ((8, 14), (8, 12)), ((8, 9), (8, 6))):
    p = ham(s, t)
    print('# section of %d ice tiles' % len(p))
    for x, y in p: print('walk %d %d 600' % (x, y))
    print('wait 120')
