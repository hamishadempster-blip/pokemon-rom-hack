#!/usr/bin/env python3
"""Mira and Soren, Birch's twins: overworld sheets, trainer front pics and
palettes, made from Emerald's Brendan (Soren) and May (Mira) by redrawing
the head (spiked hair and headband; a wide-brim sun hat) and giving each a
new palette. Pixel edits are original to this project; the bodies keep
Emerald's frames so the animation and scale match the game.

    tools/coop_art/rivals.py            # writes the PNGs and .pal files
    tools/coop_art/rivals.py --preview DIR
"""
import os, sys
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OW = os.path.join(ROOT, 'graphics/object_events/pics/people')
OWPAL = os.path.join(ROOT, 'graphics/object_events/palettes')
TR = os.path.join(ROOT, 'graphics/trainers')


def write_pal(path, cols):
    with open(path, 'w', newline='\r\n') as f:
        f.write('JASC-PAL\n0100\n16\n')
        for c in cols:
            f.write('%d %d %d\n' % c)


def read_pal(path):
    lines = open(path).read().split()
    vals = [int(x) for x in lines[3:3 + 48]]
    return [tuple(vals[i:i + 3]) for i in range(0, 48, 3)]


def save_indexed(path, w, h, data, pal):
    im = Image.new('P', (w, h))
    flat = []
    for c in pal:
        flat += list(c)
    im.putpalette(flat + [0] * (768 - len(flat)))
    im.putdata(data)
    im.save(path)


# --------------------------------------------------------------- palettes
# Index roles are kept from the source sprites where the body still uses
# them; the head stamps use the indices named in STAMP_KEYS.
SOREN_PAL = [
    (115, 197, 164),  # 0 transparent
    (255, 213, 180),  # 1 skin
    (255, 197, 148),  # 2 skin
    (222, 148, 115),  # 3 skin shade
    (123, 65, 65),    # 4 outline brown
    (74, 56, 82),     # 5 hair
    (123, 98, 139),   # 6 hair light
    (41, 32, 49),     # 7 hair dark / outline
    (24, 24, 32),     # 8 eyes, under-hair, jacket dark
    (172, 41, 49),    # 9 jacket crimson
    (255, 205, 65),   # 10 headband gold
    (197, 139, 32),   # 11 headband shade
    (238, 238, 246),  # 12 trim light
    (148, 148, 164),  # 13 trim shade
    (255, 255, 255),  # 14 white
    (0, 0, 0),        # 15 black
]

MIRA_PAL = [
    (115, 197, 164),  # 0 transparent
    (255, 222, 205),  # 1 skin
    (222, 164, 148),  # 2 skin
    (205, 131, 115),  # 3 skin shade
    (123, 90, 82),    # 4 outline brown
    (189, 156, 98),   # 5 hat shade / khaki
    (90, 74, 57),     # 6 shorts brown
    (148, 82, 57),    # 7 hair auburn
    (82, 49, 41),     # 8 hair dark
    (246, 230, 172),  # 9 hat light
    (230, 197, 115),  # 10 hat straw
    (49, 156, 148),   # 11 ribbon / top teal
    (98, 205, 189),   # 12 top light
    (41, 115, 115),   # 13 top teal dark
    (255, 255, 255),  # 14 white
    (0, 0, 0),        # 15 black
]

# Body recolour (source index -> new index), by twin.
SOREN_BODY = {5: 8, 6: 8, 7: 8, 8: 9, 9: 13, 10: 10, 11: 11, 12: 12, 13: 9, 14: 12}
SOREN_HEADLOW = {5: 7, 9: 5, 14: 6, 10: 10, 11: 11, 8: 8}
MIRA_BODY = {5: 6, 6: 6, 9: 9, 10: 5, 11: 6, 12: 12, 13: 11, 14: 14}
MIRA_HEADLOW = {9: 10, 14: 9, 10: 10, 11: 11, 5: 5}

STAMP_KEYS = {'S': {'O': 7, 'H': 5, 'L': 6, 'B': 10, 'b': 11, 'K': 15, 'D': 8},
              'M': {'O': 8, 'H': 10, 'L': 9, 'S': 5, 'R': 11, 'r': 13, 'K': 15, 'D': 7, 'd': 8}}

# Head stamps in reference-frame coordinates (walking frames 0, 1, 2):
# rows START..START+len-1 are redrawn; ' ' is transparent, '.' keeps the
# source pixel (recoloured).
SOREN_STAMPS = {
    'down': (7, [
        "     O  O  O    ",
        "    OHO OHOO    ",
        "  O OHHOHHHOO   ",
        "  OHOHHHHLHHHO  ",
        " OHHHLHHHHHHHHO ",
        " OHLHHHHHHHLHHO ",
        " OHHHHHHHHHHHHO ",
        "  KBBBBBBBBBBK  ",
        "  KbBBBBBBBBbK  ",
        "  KHDHHDDHHDHK  ",
        "  KD........DK  ",
    ]),
    'up': (7, [
        "     O  O  O    ",
        "    OHO OHOO    ",
        "  O OHHOHHHOO   ",
        "  OHOHHHLHHHHO  ",
        " OHHHLHHHHHHLHO ",
        " OHLHHHHHHHHHHO ",
        " OHHHHHHHLHHHHO ",
        " OBBBBBBBBBBBBO ",
        " ObbbbbbbbbbbbO ",
        "  OHHHHHHHHHHO  ",
        "  OHDHHHHHHDHO  ",
    ]),
    'left': (7, [
        "         O  O   ",
        "        OHOOHO  ",
        "     O OHHHHHO  ",
        "    OHOHHHHLHHO ",
        "  OOHHHHLHHHHHHO",
        "  OHHHHHHHHHHHO ",
        " OHHHLHHHHHHHO  ",
        " KBBBBBBBBBHHO  ",
        " KbbbbbbbbBHHO  ",
        " KHDH......DHO  ",
        " KD.........DO  ",
    ]),
}

MIRA_STAMPS = {
    'down': (8, [
        "     OOOOOO     ",
        "    OLLHHHHO    ",
        "   OLHHHHHHSO   ",
        "   OHRRRRRRSO   ",
        "  OOrrrrrrrrOO  ",
        "OOLHHHHHHHHHHSOO",
        "OSSSHHHHHHHHSSSO",
        " OOdddddddddOO  ",
    ]),
    'up': (8, [
        "     OOOOOO     ",
        "    OLLHHHHO    ",
        "   OLHHHHHHSO   ",
        "   OHHHHHHHSO   ",
        "  OORRRRRRRROO  ",
        "OOLHHHHHHHHHHSOO",
        "OSSSHHHHHHHHSSSO",
        " OOdddddddddOO  ",
    ]),
    'left': (8, [
        "      OOOOOO    ",
        "     OLLHHHHO   ",
        "    OLHHHHHHSO  ",
        "    ORRRRRRRSO  ",
        "  OOrrrrrrrrOOO ",
        "OOLHHHHHHHHHHHSO",
        "OSSSSHHHHHHHSSO ",
        " OOO.dddddddOO  ",
    ]),
}

DIRS = ['down', 'up', 'left', 'down', 'down', 'up', 'up', 'left', 'left']


def top_pixel(px, x0, w, h):
    for y in range(h):
        for x in range(x0, x0 + w):
            if px[x, y] != 0:
                return x - x0, y
    return 0, 0


# Down, up and left frames in sheet order; up frames borrow the vertical
# offset of the down frame in the same animation phase.
UP_PAIR = {1: 0, 5: 3, 6: 4}


def skin_row(px, x0, w):
    for y in range(32):
        for x in range(x0, x0 + w):
            if px[x, y] == 1:
                return y
    return None


def head_span(px, x0, w, y0, y1):
    xs = [x for y in range(max(0, y0), min(32, y1)) for x in range(w) if px[x0 + x, y]]
    return (min(xs) + max(xs)) / 2.0 if xs else w / 2.0


class Ref:
    """Head geometry of the walking sheet's frames 0, 1, 2."""
    def __init__(self, path):
        im = Image.open(path)
        px = im.load()
        self.skin = {'down': skin_row(px, 0, 16), 'left': skin_row(px, 32, 16)}
        self.skin['up'] = self.skin['down']
        self.top = {d: top_pixel(px, i * 16, 16, 32)[1] for i, d in enumerate(('down', 'up', 'left'))}
        self.cx = {d: head_span(px, i * 16, 16, self.top[d], self.skin['down'] + 2)
                   for i, d in enumerate(('down', 'up', 'left'))}


def build_sheet(src_path, fw, twin, ref):
    im = Image.open(src_path)
    px = im.load()
    stamps = SOREN_STAMPS if twin == 'S' else MIRA_STAMPS
    body = SOREN_BODY if twin == 'S' else MIRA_BODY
    headlow = SOREN_HEADLOW if twin == 'S' else MIRA_HEADLOW
    keys = STAMP_KEYS[twin]
    n = im.width // fw
    out = [0] * (im.width * im.height)
    dys = {}
    for f in range(n):
        d = DIRS[f]
        x0 = f * fw
        if d == 'up':
            g = UP_PAIR[f]
            dy = dys[g]
        else:
            dy = skin_row(px, x0, fw) - ref.skin[d]
        dys[f] = dy
        cx = head_span(px, x0, fw, ref.top[d] + dy, ref.skin['down'] + dy + 2)
        dx = int(round(cx - ref.cx[d]))
        start, rows = stamps[d]
        end = start + len(rows)
        for y in range(32):
            for x in range(fw):
                v = px[x0 + x, y]
                ry = y - dy
                if v == 0 or ry < end:
                    nv = 0
                elif ry < end + 4:
                    nv = headlow.get(v, v)
                    # May's hair highlights share the skin index: at the
                    # back of the head they become plain hair for Mira.
                    if twin == 'M' and v in (2, 3) and (d == 'up' or (d == 'left' and x >= 8 + dx)):
                        nv = 7
                else:
                    nv = body.get(v, v)
                out[y * im.width + x0 + x] = nv
        for r, line in enumerate(rows):
            y = start + r + dy
            if y < 0 or y >= 32:
                continue
            for c, ch in enumerate(line):
                x = c + dx
                if x < 0 or x >= fw or ch == ' ':
                    continue
                if ch == '.':
                    v = px[x0 + x, y]
                    out[y * im.width + x0 + x] = headlow.get(v, v) if v else 0
                else:
                    out[y * im.width + x0 + x] = keys[ch]
    return im.width, im.height, out


def main():
    preview = None
    if len(sys.argv) > 2 and sys.argv[1] == '--preview':
        preview = sys.argv[2]
    made = {}
    for twin, base, name, pal in (('S', 'brendan', 'soren', SOREN_PAL), ('M', 'may', 'mira', MIRA_PAL)):
        ref = Ref(os.path.join(OW, base, 'walking.png'))
        os.makedirs(os.path.join(OW, name), exist_ok=True)
        for sheet, fw in (('walking', 16), ('running', 16), ('mach_bike', 32)):
            w, h, data = build_sheet(os.path.join(OW, base, sheet + '.png'), fw, twin, ref)
            path = os.path.join(OW, name, sheet + '.png')
            save_indexed(path, w, h, data, pal)
            made[(name, sheet)] = path
        write_pal(os.path.join(OWPAL, name + '.pal'), pal)
    if preview:
        os.makedirs(preview, exist_ok=True)
        for (name, sheet), path in made.items():
            im = Image.open(path).convert('RGBA')
            im = im.resize((im.width * 6, im.height * 6), Image.NEAREST)
            bg = Image.new('RGBA', im.size, (150, 200, 150, 255))
            bg.alpha_composite(im)
            bg.save(os.path.join(preview, '%s_%s.png' % (name, sheet)))


if __name__ == '__main__':
    main()


# ------------------------------------------------------------ front pics
SOREN_FRONT_PAL = [
    (115, 197, 164), (255, 222, 205), (222, 164, 148), (205, 131, 115), (123, 90, 82),
    (230, 82, 82),    # 5 crimson light
    (176, 41, 49),    # 6 crimson
    (112, 24, 41),    # 7 crimson dark
    (32, 24, 40),     # 8 near black: outline, undershirt, hair dark
    (100, 100, 116),  # 9 charcoal light (trousers)
    (255, 205, 65),   # 10 gold
    (197, 139, 32),   # 11 gold shade
    (74, 56, 82),     # 12 hair
    (123, 98, 139),   # 13 hair light
    (255, 255, 255), (0, 0, 0),
]
MIRA_FRONT_PAL = [
    (115, 197, 164), (255, 222, 205), (222, 164, 148), (205, 131, 115), (123, 90, 82),
    (189, 156, 98),   # 5 khaki
    (214, 66, 66),    # 6 red (Poke Ball)
    (148, 82, 57),    # 7 hair auburn
    (82, 49, 41),     # 8 hair dark / outline
    (246, 230, 172),  # 9 hat light
    (230, 197, 115),  # 10 straw
    (41, 115, 115),   # 11 teal dark
    (98, 205, 189),   # 12 teal light
    (49, 156, 148),   # 13 teal
    (255, 255, 255), (0, 0, 0),
]

FRONT = {
    'S': dict(base='brendan', name='soren', pal=SOREN_FRONT_PAL, clear_below=16, cols=(14, 44),
              keys={'O': 8, 'H': 12, 'L': 13, 'B': 10, 'b': 11},
              head={0xa: 12, 0xb: 8, 6: 8, 5: 8, 9: 12, 0xe: 14, 8: 8, 7: 8},
              head_rows=24,
              torso={5: 5, 6: 6, 7: 7, 8: 7, 0xd: 8, 0xc: 12, 0xa: 10, 0xb: 11, 0xe: 14, 9: 9},
              legs_from=38,
              ball=((40, 26, 52, 35), {0xc: 5, 0xd: 6, 0xe: 14}),
              legs={5: 9, 6: 12, 7: 8, 8: 8, 0xd: 6, 0xc: 5, 0xa: 10, 0xb: 11, 0xe: 14, 9: 9},
              stamps=[(3, 18, [
                  "             O        ",
                  "       O    OLO    O  ",
                  "      OLO  OLHO   OLO ",
                  "  O   OLHO OLHHO OLHO ",
                  "  OLO OLHHOLHHHOOLHHO ",
                  "   OLOLHHHHHHHHHLHHHO ",
                  "  OOLLHHHHHLHHHHHHHHOO",
                  " OLLHHHHHHLHHHHHHHHHLO",
                  "  OHHHHHHLHHHHHHHHHHHO",
                  "  OHHHHHHHHHHHHHHHHHHO",
                  "  OHHHHHHHHHHHHHHHHHO ",
                  "   OBBBBBBBBBBBBBBBBO ",
                  "   ObbbbbbbbbbbbbbbbO ",
              ]), (14, 38, [
                  "OBBO ",
                  "ObBBO",
                  " ObbO",
                  "  OO ",
              ])]),
    'M': dict(base='may', name='mira', pal=MIRA_FRONT_PAL, clear_below=15, cols=(10, 46),
              keys={'O': 8, 'H': 10, 'L': 9, 'S': 5, 'R': 13, 'r': 11},
              head={0xa: 7, 0xb: 8, 5: 8, 9: 9, 0xe: 14, 0xc: 2, 0xd: 3},
              ball=((8, 26, 18, 35), {0xc: 6, 0xd: 6, 0xe: 14, 6: 15, 5: 4}),
              head_rows=24,
              torso={0xc: 12, 0xd: 13, 0xa: 5, 0xb: 8, 0xe: 14, 9: 9, 5: 5, 6: 8},
              legs_from=40,
              legs={0xc: 12, 0xd: 13, 0xa: 5, 0xb: 8, 0xe: 14, 9: 9, 5: 5, 6: 8},
              stamps=[(3, 12, [
                  "            OOOOOOOOO          ",
                  "          OOLLLHHHHHHOO        ",
                  "         OLLHHHHHHHHHHSO       ",
                  "        OLHHHHHHHHHHHHHSO      ",
                  "        OHHHHHHHHHHHHHHSO      ",
                  "        ORRRRRRRRRRRRRRRO      ",
                  "        OrrrrrrrrrrrrrrrO      ",
                  "    OOOOOHHHHHHHHHHHHHHSOOOOO  ",
                  "  OOLLLHHHHHHHHHHHHHHHHHHSSSOO ",
                  " OLLHHHHHHHHHHHHHHHHHHHHHHSSSO ",
                  "  OOSSSHHHHHHHHHHHHHHHHSSSSOO  ",
                  "    OOOOSSSSSSSSSSSSSSSSOOO    ",
              ])]),
}


def build_front(twin):
    cfg = FRONT[twin]
    im = Image.open(os.path.join(TR, 'front_pics', cfg['base'] + '.png'))
    px = im.load()
    out = [0] * (64 * 64)
    for y in range(64):
        for x in range(64):
            v = px[x, y]
            if v == 0:
                continue
            if y < cfg['clear_below'] and cfg['cols'][0] <= x < cfg['cols'][1]:
                continue
            bx0, by0, bx1, by1 = cfg['ball'][0]
            if bx0 <= x < bx1 and by0 <= y < by1 and v in cfg['ball'][1]:
                out[y * 64 + x] = cfg['ball'][1][v]
                continue
            if y < cfg['head_rows']:
                m = cfg['head']
            elif y < cfg['legs_from']:
                m = cfg['torso']
            else:
                m = cfg['legs']
            out[y * 64 + x] = m.get(v, v)
    for row0, col0, rows in cfg['stamps']:
        for r, line in enumerate(rows):
            for c, ch in enumerate(line):
                if ch != ' ':
                    out[(row0 + r) * 64 + col0 + c] = cfg['keys'][ch]
    path = os.path.join(TR, 'front_pics', cfg['name'] + '.png')
    save_indexed(path, 64, 64, out, cfg['pal'])
    write_pal(os.path.join(TR, 'palettes', cfg['name'] + '.pal'), cfg['pal'])
    return path


def fronts(preview=None):
    for t in ('S', 'M'):
        p = build_front(t)
        if preview:
            im = Image.open(p).convert('RGB').resize((512, 512), Image.NEAREST)
            im.save(os.path.join(preview, os.path.basename(p).replace('.png', '_front.png')))


if __name__ == '__main__' and True:
    fronts(sys.argv[2] if len(sys.argv) > 2 and sys.argv[1] == '--preview' else None)
