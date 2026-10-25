#!/usr/bin/env python3
"""Passable edge tiles of a map (for writing walk scenarios): edges.py MapName"""
import json, struct, sys, os
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
name = sys.argv[1]
m = json.load(open(os.path.join(ROOT, 'data/maps', name, 'map.json')))
lay = [l for l in json.load(open(os.path.join(ROOT, 'data/layouts/layouts.json')))['layouts'] if l['id'] == m['layout']][0]
w, h = lay['width'], lay['height']
blk = open(os.path.join(ROOT, lay['blockdata_filepath']), 'rb').read()
ok = lambda x, y: not ((struct.unpack_from('<H', blk, (y * w + x) * 2)[0] >> 10) & 3)
print(name, w, 'x', h)
print(' up   ', [x for x in range(w) if ok(x, 0)])
print(' down ', [x for x in range(w) if ok(x, h - 1)])
print(' left ', [y for y in range(h) if ok(0, y)])
print(' right', [y for y in range(h) if ok(w - 1, y)])
print(' warps', [(e['x'], e['y'], e['dest_map']) for e in m['warp_events']])
