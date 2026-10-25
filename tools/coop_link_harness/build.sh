#!/usr/bin/env bash
# Builds the two-player link harness against a matching mGBA 0.10.2 static library
# and writes the symbol table for the current pokeemerald.elf.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
mgba="${MGBA_SRC:-/tmp/mgba-src}"
out="${HARNESS_OUT:-/tmp/coop_harness_bin}"
mkdir -p "$out"

if [ ! -f "$mgba/build/libmgba.a" ]; then
	[ -d "$mgba" ] || git clone -q --depth 1 -b 0.10.2 https://github.com/mgba-emu/mgba.git "$mgba"
	mkdir -p "$mgba/build"
	(cd "$mgba/build" && CC=gcc CXX=g++ cmake .. -DLIBMGBA_ONLY=ON -DBUILD_STATIC=ON -DBUILD_SHARED=OFF \
		-DUSE_FFMPEG=OFF -DUSE_LIBZIP=OFF -DUSE_SQLITE3=OFF -DUSE_ELF=OFF -DUSE_LUA=OFF -DUSE_DEBUGGERS=OFF \
		-DUSE_EDITLINE=OFF -DM_CORE_GB=OFF -DUSE_DISCORD_RPC=OFF -DCMAKE_BUILD_TYPE=Release >/dev/null && make -j8 >/dev/null)
fi

arm-none-eabi-gcc -mthumb -mabi=apcs-gnu -fno-short-enums -DMODERN=1 -DTESTING=0 -DEMERALD \
    -iquote "$root/include" -c "$here/layout.c" -o "$out/layout.o"
arm-none-eabi-objcopy -O binary -j .rodata "$out/layout.o" "$out/layout.bin"

gcc -O2 -Wall -Wno-stringop-truncation -DM_CORE_GBA -DUSE_PTHREADS -I"$mgba/include" -I"$mgba/build/include" -I"$mgba/src" \
	"$here/harness.c" "$mgba/build/libmgba.a" -lz -lpthread -lm -o "$out/coop_link_harness"

# Global symbols, then the RAM statics of a few objects as <file>.<name>,
# placed through the section bases in the linker map.
arm-none-eabi-nm -S "$root/pokeemerald.elf" | grep -vE ' [a-zA-Z] [_.$]' > "$out/syms.txt"
for obj in coop link battle_controllers battle_main start_menu menu overworld; do
	arm-none-eabi-objdump -t "$root/build/emerald/src/$obj.o" | while read -r off scope kind sect size name; do
		[ "$kind" = O ] || continue
		case "$sect" in .bss|.sbss|.data) ;; *) continue ;; esac
		base=$(awk -v s="$sect" -v o="src/$obj.o" '$1 == s && $4 == o {print $2; exit}' "$root/pokeemerald.map")
		[ -n "$base" ] && printf '%08x %s b %s.%s\n' $((base + 0x$off)) "$size" "$obj" "$name"
	done
done >> "$out/syms.txt"
python3 - "$out/layout.bin" "$out/syms.txt" <<'PY_LAYOUT'
import struct, sys
values = struct.unpack('<IIIIII', open(sys.argv[1], 'rb').read())
with open(sys.argv[2], 'a') as f:
    for name, value in zip(('saveVarsOffset', 'saveFlagsOffset', 'encryptionKeyOffset', 'storageBoxesOffset', 'boxMonSize', 'moneyOffset'), values):
        f.write(f'{value:08x} 00000004 a harness.{name}\n')
PY_LAYOUT
echo "$out/coop_link_harness"
