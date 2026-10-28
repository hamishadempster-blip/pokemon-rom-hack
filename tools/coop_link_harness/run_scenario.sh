#!/usr/bin/env bash
# usage: run_scenario.sh SCRIPT [OUTDIR] [MAX_FRAMES]   (or MAX_FRAMES in the environment)
# ROM overrides the image (default: pokeemerald.gba in the repo root).
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
bin="${HARNESS_OUT:-/tmp/coop_harness_bin}"
script="$1"
out="${2:-/tmp/coop_harness/$(basename "$script" .txt)}"
mkdir -p "$out"
rm -f "$out/p0.sav" "$out/p1.sav"
[ -n "${SAVE0:-}" ] && cp "$SAVE0" "$out/p0.sav"
[ -n "${SAVE1:-}" ] && cp "$SAVE1" "$out/p1.sav"
exec "$bin/coop_link_harness" "${ROM:-$root/pokeemerald.gba}" "$bin/syms.txt" "$out/p0.sav" "$out/p1.sav" "$script" "$out" "${3:-${MAX_FRAMES:-200000}}"
