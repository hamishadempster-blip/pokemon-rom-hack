#!/usr/bin/env bash
# Offsets of the species tables for this build (ARM ABI), for audit.py.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$(cd "$here/../.." && pwd)"
out="${1:-/tmp/coop_roster_audit}"; mkdir -p "$out"
arm-none-eabi-gcc -mthumb -mabi=apcs-gnu -fno-short-enums -DMODERN=1 -DTESTING=0 -DEMERALD \
    -iquote "$root/include" -c "$here/layout.c" -o "$out/layout.o"
arm-none-eabi-objcopy -O binary -j .rodata "$out/layout.o" "$out/layout.bin"
arm-none-eabi-nm -S "$root/pokeemerald.elf" > "$out/syms.txt"
echo "$out"
