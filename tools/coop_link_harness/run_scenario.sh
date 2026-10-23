#!/usr/bin/env bash
# usage: run_scenario.sh SCRIPT [OUTDIR] [MAX_FRAMES]
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
bin="${HARNESS_OUT:-/tmp/coop_harness_bin}"
script="$1"
out="${2:-/tmp/coop_harness/$(basename "$script" .txt)}"
mkdir -p "$out"
rm -f "$out/p0.sav" "$out/p1.sav"
[ -n "${SAVE0:-}" ] && cp "$SAVE0" "$out/p0.sav"
[ -n "${SAVE1:-}" ] && cp "$SAVE1" "$out/p1.sav"
exec "$bin/coop_link_harness" "$root/pokeemerald.gba" "$bin/syms.txt" "$out/p0.sav" "$out/p1.sav" "$script" "$out" "${3:-200000}"
