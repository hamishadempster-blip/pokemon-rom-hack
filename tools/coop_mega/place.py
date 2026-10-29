#!/usr/bin/env python3
"""Tile checks for item balls placed by tools/coop_mega/rewards.py.

A ball is a solid object, so it must never stand where it can trap anyone:
it goes on a plain walkable tile (no water, ledge, warp, sign, trigger, ice,
puzzle tile), reachable from the map's warps or edges, not next to a warp,
a trigger or a person, not on a tile whose removal would cut the walkable
area in two (an articulation point), so every path stays open, and not on
the shortest way between two entrances, where players walk through.
Dead ends are preferred: the classic place for an item ball.

    place.py MAP [MAP...]     print the best candidate tiles of each map
"""
import json, os, re, struct, sys
from collections import deque

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
_LAYOUTS = {l['id']: l for l in json.load(open(os.path.join(ROOT, 'data/layouts/layouts.json')))['layouts'] if 'id' in l}
_BEH = [l.strip().split()[0].rstrip(',') for l in open(os.path.join(ROOT, 'include/constants/metatile_behaviors.h'))
        if l.strip().startswith('MB_')]
LAND = {'MB_NORMAL', 'MB_TALL_GRASS', 'MB_SHORT_GRASS', 'MB_CAVE', 'MB_SAND', 'MB_ASHGRASS', 'MB_MOUNTAIN_TOP',
        'MB_INDOOR_ENCOUNTER', 'MB_FOOTPRINTS', 'MB_NO_RUNNING'}
WATER_WORDS = ('WATER', 'OCEAN', 'POND', 'WATERFALL')
CONDITIONAL_WARPS = {'MAP_TERRA_CAVE_ENTRANCE', 'MAP_MARINE_CAVE_ENTRANCE'}


def _folder(name):
    return re.sub(r'(?<!^)(?=[A-Z])', '_', name.replace('gTileset_', '')).lower()


def _attrs(path):
    b = open(os.path.join(ROOT, path), 'rb').read()
    return list(struct.unpack('<%dH' % (len(b) // 2), b))


class Grid:
    def __init__(self, map_folder):
        self.name = map_folder
        self.map = json.load(open(os.path.join(ROOT, 'data/maps', map_folder, 'map.json')))
        lay = _LAYOUTS[self.map['layout']]
        self.W, self.H = lay['width'], lay['height']
        pa = _attrs('data/tilesets/primary/%s/metatile_attributes.bin' % _folder(lay['primary_tileset']))
        sa = _attrs('data/tilesets/secondary/%s/metatile_attributes.bin' % _folder(lay['secondary_tileset']))
        pa = pa + [0] * (512 - len(pa))
        b = open(os.path.join(ROOT, lay['blockdata_filepath']), 'rb').read()
        v = struct.unpack('<%dH' % (len(b) // 2), b)
        self.col, self.elev, self.beh = {}, {}, {}
        for y in range(self.H):
            for x in range(self.W):
                e = v[y * self.W + x]
                mid = e & 0x3FF
                a = pa[mid] if mid < 512 else (sa[mid - 512] if mid - 512 < len(sa) else 0)
                self.col[(x, y)] = (e >> 10) & 3
                self.elev[(x, y)] = (e >> 12) & 0xF
                bi = a & 0xFF
                self.beh[(x, y)] = _BEH[bi] if bi < len(_BEH) else '?'
        self.objects = {(o['x'], o['y']) for o in self.map['object_events']}
        self.warps = {(w['x'], w['y']) for w in self.map['warp_events']}
        # Terra and Marine Cave entrances open only on some days and only to
        # the legendary hunt; they are not a way into a pocket of the map.
        self.entrances = {(w['x'], w['y']) for w in self.map['warp_events']
                          if w['dest_map'] not in CONDITIONAL_WARPS}
        self.coords = {(c['x'], c['y']) for c in self.map.get('coord_events', [])}
        self.bgs = {(g['x'], g['y']) for g in self.map.get('bg_events', [])}

    def water(self, t):
        return any(w in self.beh.get(t, '') for w in WATER_WORDS)

    def land(self, t):
        return self.col.get(t, 1) == 0 and self.beh.get(t) in LAND

    def walk(self, t):
        b = self.beh.get(t, '')
        return self.col.get(t, 1) == 0 and (b in LAND or 'JUMP' in b or 'LEDGE' in b or 'BRIDGE' in b or 'STAIRS' in b
                                            or 'DOOR' in b or 'ARROW' in b or 'WARP' in b or b == 'MB_DEEP_SAND'
                                            or 'GRASS' in b or 'PUDDLE' in b or 'SHALLOW' in b or 'LADDER' in b
                                            )

    def step_ok(self, a, b, surf):
        if not (self.walk(b) or (surf and self.water(b) and self.col.get(b, 1) == 0)):
            return False
        ea, eb = self.elev[a], self.elev[b]
        return ea == eb or ea in (0, 15) or eb in (0, 15) or (surf and (self.water(a) or self.water(b)))

    def neighbours(self, t):
        x, y = t
        for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= n[0] < self.W and 0 <= n[1] < self.H:
                yield n

    def reachable(self, surf):
        # Entrances: the warps, and the edges that connect to another map.
        start = [w for w in self.entrances]
        sides = {c['direction'] for c in (self.map.get('connections') or [])}
        for x in range(self.W):
            if 'up' in sides:
                start.append((x, 0))
            if 'down' in sides:
                start.append((x, self.H - 1))
        for y in range(self.H):
            if 'left' in sides:
                start.append((0, y))
            if 'right' in sides:
                start.append((self.W - 1, y))
        seen = set()
        q = deque()
        for s in start:
            if s in self.col and (s in self.entrances or self.walk(s) or (surf and self.water(s))):
                seen.add(s)
                q.append(s)
        while q:
            t = q.popleft()
            for n in self.neighbours(t):
                if n not in seen and n not in self.objects and self.step_ok(t, n, surf):
                    seen.add(n)
                    q.append(n)
        return seen

    def entrance_groups(self):
        """Each warp, and each connected edge as one group of tiles."""
        groups = [[w] for w in self.entrances]
        sides = {c['direction'] for c in (self.map.get('connections') or [])}
        if 'up' in sides:
            groups.append([(x, 0) for x in range(self.W)])
        if 'down' in sides:
            groups.append([(x, self.H - 1) for x in range(self.W)])
        if 'left' in sides:
            groups.append([(0, y) for y in range(self.H)])
        if 'right' in sides:
            groups.append([(self.W - 1, y) for y in range(self.H)])
        return groups

    def main_paths(self, surf):
        """Tiles on a shortest walk between any two entrances: where players
        go through, so a ball there would stand in their way even when a
        detour exists."""
        groups = self.entrance_groups()
        out = set()
        for i, g in enumerate(groups):
            prev = {}
            q = deque()
            for s in g:
                if s in self.col and (s in self.entrances or self.walk(s) or (surf and self.water(s))):
                    prev[s] = None
                    q.append(s)
            while q:
                t = q.popleft()
                for n in self.neighbours(t):
                    if n not in prev and n not in self.objects and self.step_ok(t, n, surf):
                        prev[n] = t
                        q.append(n)
            for h in groups[i + 1:]:
                ends = [t for t in h if t in prev]
                if not ends:
                    continue
                t = ends[0]
                while t is not None:
                    out.add(t)
                    t = prev[t]
        return out

    def near_connection(self, t, margin=4):
        """Within `margin` tiles of an edge that leads to another map."""
        x, y = t
        sides = {c['direction'] for c in (self.map.get('connections') or [])}
        return (('up' in sides and y < margin) or ('down' in sides and y >= self.H - margin)
                or ('left' in sides and x < margin) or ('right' in sides and x >= self.W - margin))

    def articulation(self, nodes, surf):
        """Tarjan's articulation points of the walkable graph on nodes."""
        idx, low, ap = {}, {}, set()
        counter = [0]
        for root in nodes:
            if root in idx:
                continue
            stack = [(root, None, iter(list(self.neighbours(root))))]
            idx[root] = low[root] = counter[0]
            counter[0] += 1
            children = {root: 0}
            while stack:
                t, parent, it = stack[-1]
                advanced = False
                for n in it:
                    if n not in nodes or not self.step_ok(t, n, surf):
                        continue
                    if n not in idx:
                        idx[n] = low[n] = counter[0]
                        counter[0] += 1
                        children[t] = children.get(t, 0) + 1
                        children.setdefault(n, 0)
                        stack.append((n, t, iter(list(self.neighbours(n)))))
                        advanced = True
                        break
                    elif n != parent:
                        low[t] = min(low[t], idx[n])
                if advanced:
                    continue
                stack.pop()
                if parent is not None:
                    low[parent] = min(low[parent], low[t])
                    if stack and stack[-1][1] is not None and low[t] >= idx[parent]:
                        ap.add(parent)
            if children.get(root, 0) > 1:
                ap.add(root)
        return ap

    def candidates(self, surf=False):
        reach = self.reachable(surf)
        ap = self.articulation(reach, surf)
        busy = self.objects | self.warps | self.coords | self.bgs
        paths = self.main_paths(surf)
        out = []
        for t in sorted(reach):
            x, y = t
            if not self.land(t) or t in busy or t in ap or t in paths or self.near_connection(t):
                continue
            if x < 2 or y < 2 or x > self.W - 3 or y > self.H - 3:
                continue
            near = [n for n in ((x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)) if n != t]
            if any(n in self.warps or n in self.coords or n in self.objects or n in self.bgs for n in near):
                continue
            deg = sum(1 for n in self.neighbours(t) if n in reach and self.step_ok(t, n, surf))
            if deg == 0:
                continue
            out.append((t, deg))
        return out

    def check(self, t, surf=False):
        """None when tile t is a safe ball tile, else why not."""
        reach = self.reachable(surf)
        if t not in self.col:
            return 'off the map'
        if not self.land(t):
            return 'not plain land (%s, collision %d)' % (self.beh.get(t), self.col.get(t))
        if t not in reach:
            return 'not reachable from the warps or edges'
        if t in self.objects | self.warps | self.coords | self.bgs:
            return 'occupied'
        if t in self.articulation(reach, surf):
            return 'would cut the walkable area in two'
        if t in self.main_paths(surf):
            return 'on the main way between two entrances'
        if self.near_connection(t):
            return 'next to an edge into another map'
        x, y = t
        for n in ((x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
            if n != t and (n in self.warps or n in self.coords):
                return 'next to a warp or trigger'
        return None


def pick(map_folder, surf=False, near=None, avoid=()):
    """Best tile: the most enclosed first (a dead end, then a nook against
    two walls...), then (when `near` is given) the closest to it, else the
    farthest from the warps."""
    g = Grid(map_folder)
    cands = [(t, d) for t, d in g.candidates(surf) if t not in avoid]
    if not cands:
        return None
    def score(c):
        t, d = c
        dead = d
        if near:
            return (dead, abs(t[0] - near[0]) + abs(t[1] - near[1]), t)
        far = min([abs(t[0] - w[0]) + abs(t[1] - w[1]) for w in g.warps] or [0])
        return (dead, -far, t)
    return min(cands, key=score)[0]


if __name__ == '__main__':
    for m in sys.argv[1:]:
        g = Grid(m)
        c = g.candidates()
        print(m, len(c), 'candidates; dead ends:', [t for t, d in c if d == 1][:12], 'pick', pick(m))
