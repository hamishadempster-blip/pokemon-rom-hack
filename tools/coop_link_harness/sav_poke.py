#!/usr/bin/env python3
"""Test fixture: change one SaveBlock3 byte in a COPY of a .sav.

    sav_poke.py IN.sav OUT.sav SB3_OFFSET VALUE

Writes OUT.sav (never IN.sav) with SaveBlock3 byte SB3_OFFSET set to VALUE
in both save slots. SaveBlock3 lives in the 116 bytes after each sector's
data, outside the sector checksum, so the sectors stay valid. Used to make
a save that looks like it came from a newer build (save_schema_newer).
"""
import struct, sys
src, dst, off, val = sys.argv[1], sys.argv[2], int(sys.argv[3], 0), int(sys.argv[4], 0)
if src == dst:
    sys.exit('refusing to write over the input')
d = bytearray(open(src, 'rb').read())
for slot in (0, 1):
    for i in range(14):
        base = (slot * 14 + i) * 0x1000
        sid = struct.unpack_from('<H', d, base + 0xFF4)[0]
        if sid == off // 116:
            d[base + 3968 + off % 116] = val
open(dst, 'wb').write(d)
