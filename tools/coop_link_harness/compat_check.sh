#!/bin/bash
# Save compatibility check (build 0xC017013F).
#
#   OLD_ROM=old.gba OLD_HB=dir NEW_HB=dir OUT=dir compat_check.sh NAME:SAVEDIR ...
#
# For each NAME:SAVEDIR (p0.sav/p1.sav): load the saves on the old build and
# on this build (compat_load: CONTINUE, dump, SAVE, dump), power-cycle this
# build's saves (compat_reload: CONTINUE, dump, LINK UP, dump), then compare
# with compat.py:
#   old load  vs new load     same data across builds (migration only)
#   new saved vs new reload   a save written by this build reads back the same
#   new reload vs relinked    linking touches only link bookkeeping
# OLD_HB / NEW_HB are harness builds (build.sh) for each ROM; the old one is
# built from this harness source against the old tree's ELF.
set -u
here="$(cd "$(dirname "$0")" && pwd)"
: "${OLD_ROM:?}" "${OLD_HB:?}" "${NEW_HB:?}" "${OUT:?}"
NEW_ROM="${NEW_ROM:-$here/../../pokeemerald.gba}"
mkdir -p "$OUT"
run() { local n=$1 rom=$2 hb=$3 sd=$4 sc=$5; rm -rf "$OUT/$n"
	env ROM="$rom" HARNESS_OUT="$hb" SAVE0="$sd/p0.sav" SAVE1="$sd/p1.sav" "$here/run_scenario.sh" "$here/scenarios/$sc.txt" "$OUT/$n" 200000 > "$OUT/$n.log" 2>&1
	echo "$n rc=$?"; }
# Fields rebuilt on every load (map templates, the map view), the play-time
# and berry clocks, and the daily values the game rerolls on a new day.
AL="objectEventTemplates,mapView,playTimeHours,playTimeMinutes,playTimeSeconds,playTimeVBlanks,lastBerryTreeUpdate,dailySeed,var 0x404B,var 0x404C"
# The relink bookkeeping: the partner's Pokedex snapshot and quest partner.
RL="$AL,coop.partnerCaught,coop.partnerDexHours,coop.partnerDexMinutes,coop.partnerDexName,coop.partnerDexOtId,coop.partnerDexValid,coop.questPartnerOtId"
fail=0
cmp() { local r; r=$(python3 "$here/compat.py" "$@"); local rc=$?; printf '%s\n' "$r" | grep -v ' same$'; [ $rc -eq 0 ] || fail=1; }
for pair in "$@"; do
	k=${pair%%:*}; d=${pair#*:}
	run "${k}_old" "$OLD_ROM" "$OLD_HB" "$d" compat_load &
	run "${k}_new" "$NEW_ROM" "$NEW_HB" "$d" compat_load &
	wait
	run "${k}_reload" "$NEW_ROM" "$NEW_HB" "$OUT/${k}_new" compat_reload
	echo "== $k: old build vs this build (CoopSave layout byte: the migration)"
	cmp --allow "$AL,coop.layout" "$OUT/${k}_old:load" "$OUT/${k}_new:load"
	echo "== $k: saved vs reloaded"
	cmp --allow "$AL" "$OUT/${k}_new:saved" "$OUT/${k}_reload:reload"
	echo "== $k: reloaded vs relinked"
	cmp --allow "$RL" "$OUT/${k}_reload:reload" "$OUT/${k}_reload:relinked"
done
echo "overall: $([ $fail -eq 0 ] && echo PASS || echo FAIL)"
exit $fail
