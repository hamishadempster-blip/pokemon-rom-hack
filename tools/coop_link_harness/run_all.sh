#!/usr/bin/env bash
# Runs every two-core regression scenario in dependency order and prints one
# line per scenario. Fresh-save scenarios run first; save_lab.txt and
# mays_house_doorway.txt then provide the saves for the reload scenarios.
#
#   tools/coop_link_harness/run_all.sh            # all scenarios
#   ONLY="rival_join rival_pending" run_all.sh    # a subset (fresh-save ones)
#   J65=dir J68=dir: late-game checkpoints (Wally; shared legendaries)
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
visualFailed=0

run() {
	local name="$1"; shift
	local start=$SECONDS
	if env BATTLE_LOG=1 "$@" "$here/run_scenario.sh" "$here/scenarios/$name.txt" "$out/$name" > "$out/$name.log" 2>&1; then
		status=PASS
	else
		status=FAIL
		failed=$((failed + 1))
	fi
	local frames visual
	frames=$(grep -oE 'frames p0=[0-9]+ p1=[0-9]+ failures=[0-9]+' "$out/$name.log" | tail -1)
	# What the screens said in battle (BATTLE_LOG), checked by visual_check.py.
	if python3 "$here/visual_check.py" "$out/$name.log" > "$out/$name.visual.txt" 2>&1; then
		visual="visual ok"
	else
		visual="VISUAL FAIL ($(grep -c '^FAIL' "$out/$name.visual.txt"))"
		visualFailed=$((visualFailed + 1))
	fi
	local tag=""
	grep -q '^# KIND: recovery' "$here/scenarios/$name.txt" 2>/dev/null && tag=" [deliberate fault/recovery test]"
	[ -f "$out/$name/QUARANTINED" ] && tag="$tag [saves quarantined]"
	printf '%-28s %s  %s  %s  (%ss)%s\n' "$name" "$status" "$frames" "$visual" $((SECONDS - start)) "$tag" | tee -a "$summary"
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
for s in reconnect route102_reconnect solo_six trainer_reconnect trainer_p1 coop_six coop_fainted_lead rival_story rival_p1 invite_refuse invite_busy coop_menu center_beacon link_recovery wild_route101 wild_route101_p1 battle_abort_recovery invite_timeout_recovery \
         connected_maps wild_places invite_cancel wild_run wild_simultaneous gym_teams rival_late qol \
         solo_rival solo_gym rotom_catalog trainer_overhaul e4_sidney coop_levelup_boxes \
         rival_looks outfit_same_base; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done
run outfit_same_base_reload SAVE0="$out/outfit_same_base/p0.sav" SAVE1="$out/outfit_same_base/p1.sav"

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

# J65=dir with p0.sav/p1.sav from the main journey's checkpoint 65 (Ever
# Grande, eight badges): late-game co-op regressions.
if [ -n "${J65:-}" ]; then
	run coop_wally_vr_partner SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
fi

# J68=dir with p0.sav/p1.sav from the main journey's checkpoint 68 (after the
# Hall of Fame, Rayquaza waiting at the Sky Pillar top on both saves): shared
# legendaries (src/coop_legend.c) and the partner indicator tour, then the
# runs that continue their saves (reload, reconnect).
if [ -n "${J68:-}" ]; then
	for s in legend_shared_ray legend_ray_rosa legend_ray_solo legend_ray_claimed legend_ray_story legend_groudon legend_regirock \
	         legend_ray_simul legend_ray_ko legend_ray_full legend_ray_interrupt legend_ray_lostack legend_migration coop_bag_battle_tasks indicator_tour; do
		run "$s" SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	done
	run legend_ray_resume SAVE0="$out/legend_ray_full/p0.sav" SAVE1="$out/legend_ray_full/p1.sav"
	run legend_ray_reconnect SAVE0="$out/legend_ray_interrupt/p0.sav" SAVE1="$out/legend_ray_interrupt/p1.sav"
	run legend_ray_reconnect2 SAVE0="$out/legend_ray_lostack/p0.sav" SAVE1="$out/legend_ray_lostack/p1.sav"
fi

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
	# Outfits on the seg6 pair, and the optional quests (solo and linked),
	# each followed by a power-cycle reload of the save it made.
	for s in outfit_bases shoal_solo shoal_coop coast_solo coast_coop caravan_solo caravan_coop \
	         twins_solo twins_coop beacon_solo beacon_coop quest_goods; do
		run "$s" SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	done
	run outfit_bases_reload SAVE0="$out/outfit_bases/p0.sav" SAVE1="$out/outfit_bases/p1.sav"
	# Visual polish on the seg6 pair: the partner mark (INDICATOR, NAME, OFF), the status
	# window and UNLINK messages; and the outdoor lighting across linking,
	# menus and the Centre with the two clocks set apart (23:00 and 12:00).
	run nametag SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# LINK UP after a Centre visit made while unlinked, and inside a Centre.
	run centre_link SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# Block continuations with no open block (playthrough finding #18).
	run link_stray_block SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run battle_stray_block SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run coop_field_move_fly SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run coop_partner_mach_bike SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# One trainer, two of his Pokemon down on one turn, one left in reserve.
	run coop_one_trainer_faints SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run coop_admin_flag SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run lighting_link SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699963200 RTC_EPOCH1=1699923600
	for s in shoal coast caravan twins beacon; do
		run "${s}_reload" SAVE0="$out/${s}_solo/p0.sav" SAVE1="$out/${s}_solo/p1.sav"
	done
	for s in shoal coast caravan twins beacon; do
		run "${s}_coop_reload" SAVE0="$out/${s}_coop/p0.sav" SAVE1="$out/${s}_coop/p1.sav"
	done
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

echo "failed scenarios: $failed; visual-check failures: $visualFailed" | tee -a "$summary"
exit $failed
