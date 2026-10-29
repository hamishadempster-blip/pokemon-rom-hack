#!/usr/bin/env python3
"""Save compatibility: compare save_dump files of two runs, field by field.

    compat.py [--allow FIELD,...] RUN_A:LABEL RUN_B:LABEL

RUN is a scenario output directory holding pN_LABEL_{sb1,sb2,sb3,pc,party}.bin
(the harness verb save_dump). Field offsets come from this tree's headers
(include/global.h, compiled for the GBA with arm-none-eabi-gcc), so the dumps
must share the save layout of SaveBlock1/2, the PC and the party; SaveBlock3's
co-op block is compared up to the shorter dump (new fields appended at its
end are reported as such, never as a mismatch).

Differences in fields listed by --allow are reported but do not fail (for
example the play time clock). Exit status 1 when any other field differs.
"""
import os, re, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
FLAGS = ['-mthumb', '-mthumb-interwork', '-mabi=apcs-gnu', '-mtune=arm7tdmi', '-march=armv4t',
         '-iquote', os.path.join(ROOT, 'include'), '-DMODERN=1', '-DTESTING=0', '-DEMERALD', '-std=gnu17', '-S', '-o', '-']


def members(src, name):
    m = re.search(r'struct %s\s*\{(.*?)\n\};' % name, src, re.S)
    body = re.sub(r'/\*.*?\*/', '', re.sub(r'//[^\n]*', '', m.group(1)), flags=re.S)
    out = []
    for part in body.split(';'):
        lines = [x for x in part.split('\n') if not x.strip().startswith('#')]
        l = '\n'.join(lines).strip()
        if not l or '{' in l or '}' in l:
            continue
        mm = re.search(r'(\w+)\s*(\[[^\]]*\])*\s*(:\s*\d+)?$', l)
        if mm and not mm.group(3):
            out.append(mm.group(1))
    return out


def offsets():
    src = open(os.path.join(ROOT, 'include/global.h')).read()
    fields = []
    for s in ('SaveBlock1', 'SaveBlock2', 'CoopSave'):
        for f in members(src, s):
            fields.append((s, f))
    while True:
        c = '#include "global.h"\n#include "pokemon_storage_system.h"\n'
        for i, (s, f) in enumerate(fields):
            c += 'const unsigned o%d = __builtin_offsetof(struct %s, %s);\n' % (i, s, f)
        c += 'const unsigned coopBase = __builtin_offsetof(struct SaveBlock3, coop);\n'
        for k, e in (('kMoney', '__builtin_offsetof(struct SaveBlock1, money)'), ('kCoins', '__builtin_offsetof(struct SaveBlock1, coins)'),
                     ('kStats', '__builtin_offsetof(struct SaveBlock1, gameStats)'), ('kNStats', 'NUM_GAME_STATS'),
                     ('kBag', '__builtin_offsetof(struct SaveBlock1, bag)'), ('kBagSize', 'sizeof(struct Bag)'),
                     ('kSlot', 'sizeof(struct ItemSlot)'), ('kQty', '__builtin_offsetof(struct ItemSlot, quantity)'),
                     ('kKey', '__builtin_offsetof(struct SaveBlock2, encryptionKey)'),
                     ('kPowder', '__builtin_offsetof(struct SaveBlock2, berryCrush.berryPowderAmount)')):
            c += 'const unsigned %s = %s;\n' % (k, e)
        with tempfile.NamedTemporaryFile('w', suffix='.c', delete=False) as t:
            t.write(c)
        r = subprocess.run(['arm-none-eabi-gcc'] + FLAGS + [t.name], capture_output=True, text=True)
        os.unlink(t.name)
        if r.returncode == 0:
            break
        bad = set(int(x) for x in re.findall(r"'o(\d+)'", r.stderr))
        bad |= set(int(x) for x in re.findall(r':(\d+):\d+: error', r.stderr) and [])
        if not bad:
            # errors name the member: drop members that are not in the struct
            missing = set(re.findall(r"has no member named '(\w+)'", r.stderr))
            if not missing:
                sys.exit('offset compile failed:\n' + r.stderr[:2000])
            fields = [x for x in fields if x[1] not in missing]
        else:
            fields = [x for i, x in enumerate(fields) if i not in bad]
    vals = {}
    for name, kind, v in re.findall(r'\n(o\d+|coopBase|k\w+):\s*\n\s*\.(word|space)\s+(\d+)', r.stdout):
        vals[name] = int(v) if kind == 'word' else 0
    table = {'SaveBlock1': [], 'SaveBlock2': [], 'CoopSave': []}
    for i, (s, f) in enumerate(fields):
        if 'o%d' % i in vals:
            table[s].append((vals['o%d' % i], f))
    for k in table:
        table[k].sort()
    return table, vals.get('coopBase', 0), vals


def field_at(table, off):
    name = '?'
    for o, f in table:
        if o <= off:
            name = f
        else:
            break
    return name


def main():
    args = sys.argv[1:]
    allow = set()
    if args and args[0] == '--allow':
        allow = set(args[1].split(','))
        args = args[2:]
    (ra, la), (rb, lb) = [a.rsplit(':', 1) for a in args]
    table, coopBase, K = offsets()

    def u32(b, o):
        return int.from_bytes(b[o:o + 4], 'little')

    def decrypt(sb1, sb2):
        # Money, coins, game stats, bag quantities and berry powder are kept
        # XORed with SaveBlock2's encryption key, which the game changes on
        # every map load: compare the values, not the stored bytes.
        key = u32(sb2, K['kKey'])
        sb1, sb2 = bytearray(sb1), bytearray(sb2)
        def x32(buf, o):
            buf[o:o + 4] = (u32(buf, o) ^ key).to_bytes(4, 'little')
        def x16(buf, o):
            buf[o:o + 2] = ((int.from_bytes(buf[o:o + 2], 'little') ^ key) & 0xFFFF).to_bytes(2, 'little')
        x32(sb1, K['kMoney']); x16(sb1, K['kCoins'])
        for i in range(K['kNStats']):
            x32(sb1, K['kStats'] + 4 * i)
        for o in range(K['kBag'], K['kBag'] + K['kBagSize'], K['kSlot']):
            x16(sb1, o + K['kQty'])
        x32(sb2, K['kPowder'])
        sb2[K['kKey']:K['kKey'] + 4] = b'\0\0\0\0'
        return bytes(sb1), bytes(sb2)
    bad = 0
    for pid in (0, 1):
        for part, tab, base in (('sb1', 'SaveBlock1', 0), ('sb2', 'SaveBlock2', 0), ('sb3', 'CoopSave', coopBase), ('pc', None, 0), ('party', None, 0)):
            fa = os.path.join(ra, 'p%d_%s_%s.bin' % (pid, la, part))
            fb = os.path.join(rb, 'p%d_%s_%s.bin' % (pid, lb, part))
            if not (os.path.exists(fa) and os.path.exists(fb)):
                print('p%d %s: missing dump' % (pid, part)); bad += 1; continue
            a, b = open(fa, 'rb').read(), open(fb, 'rb').read()
            if part in ('sb1', 'sb2'):
                pa = [open(os.path.join(ra, 'p%d_%s_%s.bin' % (pid, la, q)), 'rb').read() for q in ('sb1', 'sb2')]
                pb = [open(os.path.join(rb, 'p%d_%s_%s.bin' % (pid, lb, q)), 'rb').read() for q in ('sb1', 'sb2')]
                da, db = decrypt(*pa), decrypt(*pb)
                a, b = (da[0], db[0]) if part == 'sb1' else (da[1], db[1])
            n = min(len(a), len(b))
            diff = {}
            for i in range(n):
                if a[i] != b[i]:
                    if tab is None:
                        key = part
                    elif part == 'sb3':
                        key = 'coop.' + field_at(table[tab], i - base) if i >= base else 'sb3[%d]' % i
                    else:
                        key = field_at(table[tab], i)
                        fo = dict((f, o) for o, f in table[tab])
                        if part == 'sb1' and key == 'vars':
                            key = 'var 0x%04X' % (0x4000 + (i - fo['vars']) // 2)
                        elif part == 'sb1' and key == 'flags':
                            byte = i - fo['flags']
                            key = 'flags 0x%04X-0x%04X' % (byte * 8, byte * 8 + 7)
                    diff[key] = diff.get(key, 0) + 1
            extra = '' if len(a) == len(b) else ' (sizes %d / %d: %d bytes appended)' % (len(a), len(b), abs(len(a) - len(b)))
            if not diff:
                print('p%d %-5s same%s' % (pid, part, extra))
                continue
            for k, v in sorted(diff.items()):
                tag = 'allowed' if k in allow else 'DIFF'
                if tag == 'DIFF':
                    bad += 1
                print('p%d %-5s %-7s %s: %d bytes%s' % (pid, part, tag, k, v, extra))
    print('result:', 'FAIL' if bad else 'PASS')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
