#!/usr/bin/env bash
# Runs every two-core regression scenario in dependency order and prints one
# line per scenario. Fresh-save scenarios run first; save_lab.txt and
# mays_house_doorway.txt then provide the saves for the reload scenarios.
#
#   tools/coop_link_harness/run_all.sh            # all scenarios
#   ONLY="rival_join rival_pending" run_all.sh    # a subset (fresh-save ones)
#   SEG6=/path/dir run_all.sh                     # also the story_seg6-based
#                                                 # partner menu, QoL, and quest runs
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
       rival_simultaneous rival_reconnect save_lab solo_opening new_species_evo"
if [ -n "${ONLY:-}" ]; then
	for s in $ONLY; do run "$s"; done
	exit $failed
fi

for s in $fresh; do run "$s"; done

lab0="$out/save_lab/p0.sav"; lab1="$out/save_lab/p1.sav"
for s in reconnect route102_reconnect solo_six trainer_reconnect trainer_p1 coop_six rival_story rival_p1 invite_refuse invite_busy coop_menu center_beacon link_recovery wild_route101 wild_route101_p1 battle_abort_recovery invite_timeout_recovery \
         connected_maps wild_places invite_cancel wild_run wild_simultaneous gym_teams rival_late qol \
         solo_rival solo_gym rotom_catalog trainer_overhaul e4_sidney coop_levelup_boxes; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done

# Story run on real walking: segment 1 from the lab saves, then the
# segments and regressions that continue segment 1's saved game.
run story_seg1 SAVE0="$lab0" SAVE1="$lab1"
s1_0="$out/story_seg1/p0.sav"; s1_1="$out/story_seg1/p1.sav"
for s in whiteout_home story_seg2; do
	run "$s" SAVE0="$s1_0" SAVE1="$s1_1"
done
# LONG=1: the uninterrupted story chain continues, each segment from the
# saves the one before made (Rustboro, Roxanne, Devon, Rustboro rival).
if [ -n "${LONG:-}" ]; then
	prev=story_seg2
	for s in story_seg3 story_seg4 story_seg5 story_seg6 catch_walk; do
		run "$s" SAVE0="$out/$prev/p0.sav" SAVE1="$out/$prev/p1.sav"
		prev=$s
	done
fi

run bedroom_reload SAVE0="$out/mays_house_doorway/p0.sav" SAVE1="$out/mays_house_doorway/p1.sav"

# SEG6=dir with p0.sav/p1.sav from story_seg6 (Rustboro, linked play): the
# partner menu, QoL, and Power Problems scenarios, then their reloads.
if [ -n "${SEG6:-}" ]; then
	for s in deal_trade deal_cut_before_partner deal_cut_after_commit deal_lost_ready deal_gifts deal_edge \
	         deal_battle deal_battle_doubles deal_battle_rules deal_dex deal_fast practice save_identity \
	         pp_solo pp_coop pp_reconnect roster_g; do
		run "$s" SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	done
	for s in deal_trade deal_cut_before_partner deal_cut_after_commit deal_gifts; do
		run "${s}_reload" SAVE0="$out/$s/p0.sav" SAVE1="$out/$s/p1.sav"
	done
	run deal_dex_offline SAVE0="$out/deal_dex/p0.sav" SAVE1="$out/deal_dex/p1.sav"
	run pp_stagger SAVE0="$out/pp_solo/p0.sav" SAVE1="$SEG6/p1.sav"
	run pp_stagger_reload SAVE0="$out/pp_solo/p0.sav" SAVE1="$out/pp_stagger/p1.sav"
fi

if [ -n "${AFFECTED:-}" ]; then
	run mays_house_recovery SAVE0="$AFFECTED/host_p0.sav" SAVE1="$AFFECTED/guest_p1.sav"
fi

# Saves made by a TEST START before the respawn fix (no heal location).
if [ -n "${HEAL_AFFECTED:-}" ]; then
	run whiteout_home SAVE0="$HEAL_AFFECTED/p0.sav" SAVE1="$HEAL_AFFECTED/p1.sav"
fi

if [ -n "${ZIG_AFFECTED:-}" ]; then
	run zigzagoon_reload SAVE0="$ZIG_AFFECTED/p0.sav" SAVE1="$ZIG_AFFECTED/p1.sav"
fi

echo "failed scenarios: $failed" | tee -a "$summary"
exit $failed
