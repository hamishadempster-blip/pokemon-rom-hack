#!/usr/bin/env bash
# Runs every two-core regression scenario in dependency order and prints one
# line per scenario. Fresh-save scenarios run first; save_lab.txt and
# mays_house_doorway.txt then provide the saves for the reload scenarios.
#
#   tools/coop_link_harness/run_all.sh            # all scenarios
#   ONLY="rival_join rival_pending" run_all.sh    # a subset (fresh-save ones)
#   AFFECTED=/path/dir run_all.sh                 # also run mays_house_recovery
#                                                 # on host_p0.sav/guest_p1.sav
#
# HARNESS_OUT picks the harness build (for example an mGBA 0.10.3 build).
set -u
here="$(cd "$(dirname "$0")" && pwd)"
out="${OUT:-/tmp/coop_regress}"
mkdir -p "$out"
summary="$out/summary.txt"
: > "$summary"
failed=0

run() {
	local name="$1"; shift
	local start=$SECONDS
	if env "$@" "$here/run_scenario.sh" "$here/scenarios/$name.txt" "$out/$name" > "$out/$name.log" 2>&1; then
		status=PASS
	else
		status=FAIL
		failed=$((failed + 1))
	fi
	local frames
	frames=$(grep -oE 'frames p0=[0-9]+ p1=[0-9]+ failures=[0-9]+' "$out/$name.log" | tail -1)
	printf '%-28s %s  %s  (%ss)\n' "$name" "$status" "$frames" $((SECONDS - start)) | tee -a "$summary"
}

fresh="littleroot doors_stagger rescue guest_move guest_evolution target_navigation spread_selection
       mays_house_doorway mays_house_doorway_p1bag rival_join rival_pending rival_latejoin
       rival_simultaneous rival_reconnect save_lab"
if [ -n "${ONLY:-}" ]; then
	for s in $ONLY; do run "$s"; done
	exit $failed
fi

for s in $fresh; do run "$s"; done

lab0="$out/save_lab/p0.sav"; lab1="$out/save_lab/p1.sav"
for s in reconnect route102_reconnect solo_six trainer_reconnect trainer_p1 coop_six rival_story rival_p1 invite_refuse invite_busy coop_menu center_beacon link_recovery wild_route101 wild_route101_p1 battle_abort_recovery invite_timeout_recovery \
         connected_maps wild_places invite_cancel wild_run; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done

run bedroom_reload SAVE0="$out/mays_house_doorway/p0.sav" SAVE1="$out/mays_house_doorway/p1.sav"

if [ -n "${AFFECTED:-}" ]; then
	run mays_house_recovery SAVE0="$AFFECTED/host_p0.sav" SAVE1="$AFFECTED/guest_p1.sav"
fi

if [ -n "${ZIG_AFFECTED:-}" ]; then
	run zigzagoon_reload SAVE0="$ZIG_AFFECTED/p0.sav" SAVE1="$ZIG_AFFECTED/p1.sav"
fi

echo "failed scenarios: $failed" | tee -a "$summary"
exit $failed
