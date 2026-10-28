#!/usr/bin/env bash
# Runs every two-core regression scenario in dependency order and prints one
# line per scenario. Fresh-save scenarios run first; save_lab.txt and
# mays_house_doorway.txt then provide the saves for the reload scenarios.
#
#   tools/coop_link_harness/run_all.sh            # all scenarios
#   ONLY="rival_join rival_pending" run_all.sh    # a subset (fresh-save ones)
#   J16=dir J37=dir: finding #16, shared NPCs, scene joins (J16: co-op revamps)
#   J43=dir J48=dir: revamps in solo play, showcase trainers at route level
#   J52=dir J65=dir J68=dir: late-game checkpoints (late wild encounters;
#   Wally and roster evolutions; shared legendaries)
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
	run_as "$1" "$@"
}

# run_as OUTNAME SCENARIO [VAR=value...]: the same scenario on other saves
# or at another hour, kept under its own name.
run_as() {
	local name="$1" scen="$2"; shift 2
	local start=$SECONDS
	if env BATTLE_LOG=1 "$@" "$here/run_scenario.sh" "$here/scenarios/$scen.txt" "$out/$name" > "$out/$name.log" 2>&1; then
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
	grep -q '^# KIND: recovery' "$here/scenarios/$scen.txt" 2>/dev/null && tag=" [deliberate fault/recovery test]"
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

# J16=dir with p0.sav/p1.sav from the main journey's checkpoint 16 (Route
# 116, by Rustboro): finding #16 sequences and shared NPC movement.
if [ -n "${J16:-}" ]; then
	for s in finding16_wild_teleport_karen finding16_partner_wild_karen npc_town_order npc_interior_talk npc_reconnect coop_battle_items coop_battle_items_wild; do
		run "$s" SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	done
fi

# J37=dir with p0.sav/p1.sav from checkpoint 37 (Fallarbor, before Meteor
# Falls): joining a partner's story scene on its trigger tile.
# J16 also: revamped Pokemon's support moves and abilities in co-op doubles.
if [ -n "${J16:-}" ]; then
	run revamp_coop_support SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	# Second roster batch: Cramorant, Morpeko and Eiscue forms on both games.
	run battleforms_coop SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
fi

# J43/J48=dir: checkpoints 43 (Rustboro, after Route 117) and 48 (Route
# 119): revamps in solo battles and the showcase trainers at route level.
if [ -n "${J43:-}" ]; then
	run revamp_solo_checks SAVE0="$J43/p0.sav" SAVE1="$J43/p1.sav"
	run revamp_trainers_early SAVE0="$J43/p0.sav" SAVE1="$J43/p1.sav"
fi
if [ -n "${J48:-}" ]; then
	run revamp_trainers_mid SAVE0="$J48/p0.sav" SAVE1="$J48/p1.sav"
fi

if [ -n "${J37:-}" ]; then
	for s in scene_meteor_join scene_meteor_menu; do
		run "$s" SAVE0="$J37/p0.sav" SAVE1="$J37/p1.sav"
	done
fi

# J65=dir with p0.sav/p1.sav from the main journey's checkpoint 65 (Ever
# Grande, eight badges): late-game co-op regressions.
if [ -n "${J65:-}" ]; then
	run coop_wally_vr_partner SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	# First roster batch: every distinct evolution method (player 1 at night).
	run roster_b1_evolutions SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav" RTC_EPOCH0=1699923600 RTC_EPOCH1=1699963200
	# Second roster batch: guests, fossils, every distinct evolution method,
	# the stock-gap repairs and a Day Care egg.
	run roster_b2_gifts SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	run roster_b2_evolutions SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav" RTC_EPOCH0=1699923600 RTC_EPOCH1=1699963200
	run roster_repair SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	run roster_breeding SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	run fieldguide_probe SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	run shop_b2 SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
fi

# J52=dir with p0.sav/p1.sav from checkpoint 52 (Lilycove, linked): normal
# wild encounters after Route 119 (grass, Surf, fishing, catch, run,
# Teleport, a building transition between battles).
if [ -n "${J52:-}" ]; then
	run late_wild_tour SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"
	run revamp_trainers_late SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"
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
	# Pokedex reach and search; every optional quest started and finished
	# on a save that has cleared the League.
	run dex_probe SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	for s in shoal_solo caravan_solo coast_solo twins_solo beacon_solo pp_solo; do
		run_as "${s}_j68" "$s" SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	done
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
	# Doors, nurse, menus, save, bikes, FLY, a solo and a co-op battle, UNLINK/LINK UP.
	run transitions_tour SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run coop_partner_mach_bike SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# One trainer, two of his Pokemon down on one turn, one left in reserve.
	run coop_one_trainer_faints SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run coop_admin_flag SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# First roster batch: new species caught, traded, gifted (party and PC),
	# reloaded; the partner-Pokemon badge gate.
	run roster_b1_play SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run roster_b1_reload SAVE0="$out/roster_b1_play/p0.sav" SAVE1="$out/roster_b1_play/p1.sav"
	run roster_b1_gate SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# Revamps: an owned revamped Pokemon's stats after a reload.
	run revamp_owned_stats SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run revamp_owned_stats_reload SAVE0="$out/revamp_owned_stats/p0.sav" SAVE1="$out/revamp_owned_stats/p1.sav"
	run lighting_link SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699963200 RTC_EPOCH1=1699923600
	# Second roster batch in play: catch, trade, store, reload.
	run roster_b2_play SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run roster_b2_reload SAVE0="$out/roster_b2_play/p0.sav" SAVE1="$out/roster_b2_play/p1.sav"
	# Quests: Shoal Cave at both tides (and left half done across a reload),
	# loss/retreat/retry, a busy partner and simultaneous interaction.
	run_as shoal_solo_high shoal_solo SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699923600
	run_as shoal_solo_low shoal_solo SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699938000
	run shoal_vulpix_full_party SAVE0="$out/shoal_solo_high/p0.sav" SAVE1="$out/shoal_solo_high/p1.sav"
	run shoal_tide_a SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699923600
	run shoal_tide_b SAVE0="$out/shoal_tide_a/p0.sav" SAVE1="$out/shoal_tide_a/p1.sav" RTC_EPOCH0=1699938000
	run quest_loss_retry SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run quest_busy_partner SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
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
