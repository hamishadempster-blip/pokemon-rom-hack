#!/usr/bin/env python3
"""Read an Emerald co-op battery save (.sav) without an emulator.

    coop_save_inspect.py [--json] FILE.sav [...]

For each file: both save slots (counter, sector ids, signatures, checksums),
which slot the game would load, and from that slot every real Pokémon
(party, PC boxes, Day Care): checksum, species, and the Pokémon-record bits
pokeemerald-expansion leaves unused, which this project reserves for the
Trainer Bond (docs/COOP_STORAGE_PLAN.md). Also the CoopSave layout byte.

Offsets come from this tree's headers (compiled for the GBA with
arm-none-eabi-gcc), so run it from a built tree (`make` generates headers
the save structures include). Read-only: the file is never written.
"""
import json, os, re, struct, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
FLAGS = ['-mthumb', '-mthumb-interwork', '-mabi=apcs-gnu', '-mtune=arm7tdmi', '-march=armv4t',
         '-iquote', os.path.join(ROOT, 'include'), '-DMODERN=1', '-DTESTING=0', '-DEMERALD', '-std=gnu17', '-S', '-o', '-']

EXPRS = {
    'sb1Size': 'sizeof(struct SaveBlock1)', 'sb2Size': 'sizeof(struct SaveBlock2)',
    'sb3Size': 'sizeof(struct SaveBlock3)', 'pcSize': 'sizeof(struct PokemonStorage)',
    'partyCount': '__builtin_offsetof(struct SaveBlock1, playerPartyCount)',
    'party': '__builtin_offsetof(struct SaveBlock1, playerParty)',
    'daycare': '__builtin_offsetof(struct SaveBlock1, daycare)',
    'daycareMonSize': 'sizeof(struct DaycareMon)',
    'boxes': '__builtin_offsetof(struct PokemonStorage, boxes)',
    'monSize': 'sizeof(struct Pokemon)', 'boxMonSize': 'sizeof(struct BoxPokemon)',
    'secure': '__builtin_offsetof(struct BoxPokemon, secure)',
    'checksum': '__builtin_offsetof(struct BoxPokemon, checksum)',
    'coop': '__builtin_offsetof(struct SaveBlock3, coop)',
    'coopLayout': '__builtin_offsetof(struct CoopSave, layout)',
    'coopSize': 'sizeof(struct CoopSave)',
    'v5': '__builtin_offsetof(struct CoopSave, v5)',
    'v5Size': 'sizeof(struct CoopSaveV5)',
    'v5Kind': '__builtin_offsetof(struct CoopSaveV5, saveKind)',
    'trainerId': '__builtin_offsetof(struct SaveBlock2, playerTrainerId)',
}

# Bits of the encrypted substructs that expansion 1.17.1 declares unused
# (include/pokemon.h): (substruct, byte offset in it, bit mask) per u16/u32.
# Substruct 0: unused_02:6 (u16 at 2, bits 10-15), unused_04:3 (u32 at 4,
# bits 29-31), unused_0A:2 (u16 at 10, bits 14-15). Substruct 1:
# unused_04:5 (u16 at 4, bits 11-15), unused_06:3 (u16 at 6, bits 11-13).
# Substruct 3: unused_0B:1 (u32 at 8, bit 28). Positions checked by compiling
# each field set alone (docs/COOP_STORAGE_PLAN.md). Outside the encryption:
# BoxPokemon unused_13 (byte 19 bit 7) and unused_1E (byte 31 bit 7).
UNUSED = [(0, 2, 2, 0xFC00), (0, 4, 4, 0xE0000000), (0, 10, 2, 0xC000),
          (1, 4, 2, 0xF800), (1, 6, 2, 0x3800), (3, 8, 4, 0x10000000)]

ORDER = [  # sSubstructOffsets[type][pid % 24] (src/pokemon.c)
    [0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3],
    [1, 1, 2, 3, 2, 3, 0, 0, 0, 0, 0, 0, 2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2],
    [2, 3, 1, 1, 3, 2, 2, 3, 1, 1, 3, 2, 0, 0, 0, 0, 0, 0, 3, 2, 3, 2, 1, 1],
    [3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0],
]


def offsets():
    c = '#include "global.h"\n#include "pokemon_storage_system.h"\n#include "pokemon.h"\n'
    for k, e in EXPRS.items():
        c += 'const unsigned %s = %s;\n' % (k, e)
    with tempfile.NamedTemporaryFile('w', suffix='.c', delete=False) as t:
        t.write(c)
    r = subprocess.run(['arm-none-eabi-gcc'] + FLAGS + [t.name], capture_output=True, text=True)
    os.unlink(t.name)
    if r.returncode:
        sys.exit('offset compile failed (run from a built tree):\n' + r.stderr[:1500])
    v = {}
    for name, kind, val in re.findall(r'\n(\w+):\s*\n\s*\.(word|space)\s+(\d+)', r.stdout):
        v[name] = int(val) if kind == 'word' else 0
    return v


def checksum(b, size):
    s = 0
    for i in range(0, size & ~3, 4):
        s = (s + struct.unpack_from('<I', b, i)[0]) & 0xFFFFFFFF
    return ((s >> 16) + s) & 0xFFFF


def chunk_sizes(total):
    out = []
    for k in range((total + 3967) // 3968):
        out.append(min(3968, total - k * 3968))
    return out


def read_slots(data, O):
    sizes = {0: O['sb2Size']}
    for k, s in enumerate(chunk_sizes(O['sb1Size'])):
        sizes[1 + k] = s
    for k, s in enumerate(chunk_sizes(O['pcSize'])):
        sizes[5 + k] = s
    slots = []
    for slot in (0, 1):
        secs = {}
        info = []
        for i in range(14):
            off = (slot * 14 + i) * 0x1000
            sec = data[off:off + 0x1000]
            sid, ck, sig, ctr = struct.unpack_from('<HHII', sec, 0xFF4)
            ok = sig == 0x08012025 and sid in sizes and ck == checksum(sec, sizes[sid])
            info.append({'phys': i, 'id': sid, 'counter': ctr, 'signature': sig == 0x08012025, 'checksum': ok})
            if ok:
                secs[sid] = (sec, ctr)
        complete = len(secs) == 14
        counters = sorted(set(c for _, c in secs.values()))
        kind = None
        if complete:
            sb3 = b''.join(secs[k][0][3968:3968 + 116] for k in range(14))
            v5 = O['coop'] + O['v5']
            size = struct.unpack_from('<H', sb3, v5)[0]
            kind = 'auto' if size == O['v5Size'] and sb3[v5 + O['v5Kind']] == 1 else 'manual'
        slots.append({'complete': complete, 'counters': counters, 'sectors': info, 'data': secs, 'kind': kind})
    return slots, sizes


def pick(slots):
    # GetSaveValidStatus: a slot is OK when all 14 ids have a valid sector;
    # the higher counter wins (with the 0/-1 wrap rule).
    ok = [i for i in (0, 1) if slots[i]['complete']]
    if not ok:
        return None
    if len(ok) == 1:
        return ok[0]
    a, b = slots[0]['counters'][-1], slots[1]['counters'][-1]
    if (a, b) in ((0xFFFFFFFF, 0), (0, 0xFFFFFFFF)):
        return 1 if (a + 1) & 0xFFFFFFFF < (b + 1) & 0xFFFFFFFF else 0
    return 1 if a < b else 0


def assemble(slot, sizes, O):
    sb2 = slot['data'][0][0][:sizes[0]]
    sb1 = b''.join(slot['data'][1 + k][0][:sizes[1 + k]] for k in range(4))
    pc = b''.join(slot['data'][5 + k][0][:sizes[5 + k]] for k in range(9))
    sb3 = b''.join(slot['data'][k][0][3968:3968 + 116] for k in range(14))[:O['sb3Size']]
    return sb1, sb2, sb3, pc


def boxmon(raw, O):
    pid, otid = struct.unpack_from('<II', raw, 0)
    flags = raw[19]
    has_species = bool(flags & 2)
    key = pid ^ otid
    sec = bytearray(raw[O['secure']:O['secure'] + 48])
    for i in range(0, 48, 4):
        struct.pack_into('<I', sec, i, struct.unpack_from('<I', sec, i)[0] ^ key)
    stored = struct.unpack_from('<H', raw, O['checksum'])[0]
    calc = sum(struct.unpack_from('<24H', sec)) & 0xFFFF
    subs = [bytes(sec[ORDER[t][pid % 24] * 12:ORDER[t][pid % 24] * 12 + 12]) for t in range(4)]
    species = struct.unpack_from('<H', subs[0], 0)[0] & 0x7FF
    unused = 0
    for t, off, size, mask in UNUSED:
        v = struct.unpack_from('<H' if size == 2 else '<I', subs[t], off)[0]
        unused |= (v & mask) != 0
    unused |= bool(raw[19] & 0x80) or bool(raw[31] & 0x80)
    return {'pid': pid, 'otid': otid, 'hasSpecies': has_species, 'species': species,
            'checksumOk': stored == calc, 'unusedBitsSet': bool(unused)}


def inspect(path, O):
    data = open(path, 'rb').read()
    slots, sizes = read_slots(data, O)
    res = {'file': path, 'size': len(data), 'slots': [], 'loaded': None}
    for s in slots:
        res['slots'].append({'complete': s['complete'], 'kind': s['kind'], 'counters': ['%x' % c for c in s['counters']],
                             'badSectors': [x for x in s['sectors'] if not (x['signature'] and x['checksum'])]})
    k = pick(slots)
    if k is None:
        return res
    res['loaded'] = k
    sb1, sb2, sb3, pc = assemble(slots[k], sizes, O)
    mons = []
    n = sb1[O['partyCount']]
    for i in range(min(n, 6)):
        mons.append(('party', i, boxmon(sb1[O['party'] + i * O['monSize']:][:O['boxMonSize']], O)))
    for i in range(14 * 30):
        raw = pc[O['boxes'] + i * O['boxMonSize']:][:O['boxMonSize']]
        if raw[19] & 2:
            mons.append(('box', i, boxmon(raw, O)))
    for i in range(2):
        raw = sb1[O['daycare'] + i * O['daycareMonSize']:][:O['boxMonSize']]
        if raw[19] & 2:
            mons.append(('daycare', i, boxmon(raw, O)))
    res['trainerId'] = '%08x' % struct.unpack_from('<I', sb2, O['trainerId'])[0]
    res['coopLayout'] = sb3[O['coop'] + O['coopLayout']]
    res['mons'] = len(mons)
    res['badChecksum'] = [(w, i) for w, i, m in mons if not m['checksumOk']]
    res['unusedBitsSet'] = [(w, i, m['species']) for w, i, m in mons if m['unusedBitsSet']]
    return res


def main():
    args = sys.argv[1:]
    as_json = args and args[0] == '--json'
    if as_json:
        args = args[1:]
    O = offsets()
    out = [inspect(p, O) for p in args]
    if as_json:
        print(json.dumps(out, indent=1))
        return
    for r in out:
        print('%s: slot0 %s %s %s, slot1 %s %s %s, loads slot %s, trainer %s, CoopSave layout %s, %s Pokémon, %d bad checksums, %d with reserved bits set' % (
            r['file'], 'complete' if r['slots'][0]['complete'] else 'INCOMPLETE', r['slots'][0]['counters'], r['slots'][0]['kind'],
            'complete' if r['slots'][1]['complete'] else 'INCOMPLETE', r['slots'][1]['counters'], r['slots'][1]['kind'],
            r['loaded'], r.get('trainerId'), r.get('coopLayout'), r.get('mons'), len(r.get('badChecksum', [])),
            len(r.get('unusedBitsSet', []))))


if __name__ == '__main__':
    main()
