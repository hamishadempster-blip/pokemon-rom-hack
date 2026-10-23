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

gcc -O2 -Wall -DM_CORE_GBA -DUSE_PTHREADS -I"$mgba/include" -I"$mgba/build/include" -I"$mgba/src" \
	"$here/harness.c" "$mgba/build/libmgba.a" -lz -lpthread -lm -o "$out/coop_link_harness"

# Global symbols, then coop.c statics as coop.<name> from the object's .sbss base.
arm-none-eabi-nm -S "$root/pokeemerald.elf" | grep -vE ' [a-zA-Z] [_.$]' > "$out/syms.txt"
base=$(awk '/^ \.sbss/ && /src\/coop\.o$/ {print $2; exit}' "$root/pokeemerald.map")
arm-none-eabi-nm -S --defined-only "$root/build/emerald/src/coop.o" | while read -r off size type name; do
	case "$type" in b|B) printf '%08x %s %s coop.%s\n' $((base + 0x$off)) "$size" "$type" "$name" ;; esac
done >> "$out/syms.txt"
echo "$out/coop_link_harness"
