#!/usr/bin/env python3
"""Pixel width of script text lines in the normal font.

    tools/coop_text_width.py FILE.inc [max_px]

Prints every .string line segment (split at \\n, \\l, \\p) wider than max_px
(default 208, the standard message box). {PLAYER} and {STR_VAR_n} count as
7 capital letters. Exit status 1 when anything is too wide.
"""
import re, sys, os
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
src = open(os.path.join(ROOT, 'src/fonts.c')).read()
tab = src[src.index('gFontNormalLatinGlyphWidths[] = {'):]
tab = tab[tab.index('{') + 1:tab.index('};')]
WIDTH = [int(x) for x in re.findall(r'\d+', tab)]
CHAR = {}
for line in open(os.path.join(ROOT, 'charmap.txt'), encoding='utf-8'):
    m = re.match(r"^'(.)'\s*=\s*([0-9A-Fa-f]{2})\s*$", line)
    if m and m.group(1) not in CHAR:
        CHAR[m.group(1)] = int(m.group(2), 16)


def width(text):
    text = re.sub(r'\{(PLAYER|STR_VAR_\d|RIVAL)\}', 'MMMMMMM', text)
    text = re.sub(r'\{[^}]*\}', '', text)
    w = 0
    for ch in text:
        code = CHAR.get(ch)
        w += WIDTH[code] if code is not None and code < len(WIDTH) else 6
    return w


def main():
    path = sys.argv[1]
    limit = int(sys.argv[2]) if len(sys.argv) > 2 else 208
    bad = 0
    for n, line in enumerate(open(path, encoding='utf-8'), 1):
        m = re.match(r'\s*\.string\s+"(.*)"\s*$', line)
        if not m:
            continue
        for seg in re.split(r'\\[nlp]', m.group(1).replace('$', '')):
            w = width(seg)
            if w > limit:
                print('%s:%d: %dpx: %s' % (path, n, w, seg))
                bad += 1
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
