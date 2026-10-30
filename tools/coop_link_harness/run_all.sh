#!/usr/bin/env bash
# Runs every two-core regression scenario in dependency order and prints one
# line per scenario. Fresh-save scenarios run first; save_lab.txt and
# mays_house_doorway.txt then provide the saves for the reload scenarios.
#
#   tools/coop_link_harness/run_all.sh            # all scenarios
#   ONLY="rival_join rival_pending" run_all.sh    # a subset (fresh-save ones)
#   J16=dir J37=dir: finding #16, shared NPCs, scene joins (J16: co-op revamps)
#   J43=dir J48=dir: revamps in solo play, showcase trainers at route level
#   J06=dir: checkpoint 6 (before Roxanne): Trainer Exchange stock
#   J13=dir J44=dir J59=dir: gym invites to a busy partner; Strength boulders
#   J52=dir J65=dir J68=dir: late-game checkpoints (late wild encounters;
#   Wally and roster evolutions; shared legendaries)
#   J46 J50 J51 J57 J58 J66 OLD13C_COAST=dir: Battle Guide v2 checks (see the
#   block near the end)
#   J29=dir: checkpoint 29 (Slateport Centre): Trainer Bond PC and tent
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
       rival_simultaneous rival_reconnect save_lab solo_opening new_species_evo autosave_new_game"
if [ -n "${ONLY:-}" ]; then
	for s in $ONLY; do run "$s"; done
	exit $failed
fi

for s in $fresh; do run "$s"; done
# Release NEW GAME (no developer start): two boys and two girls, with the
# two games' clocks set apart.
run release_newgame_boys RTC_EPOCH0=1793250000 RTC_EPOCH1=1793290000
run release_newgame_girls RTC_EPOCH0=1793300000 RTC_EPOCH1=1793251000

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
	run roster_b3_battle SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	# Target prompts; changing a chosen action while the turn is open.
	run battle_cancel SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	# Items never move between the two players' Pokemon; both lose a wild
	# double.
	run coop_held_items SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	run coop_wild_loss SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
fi

# J43/J48=dir: checkpoints 43 (Rustboro, after Route 117) and 48 (Route
# 119): revamps in solo battles and the showcase trainers at route level.
if [ -n "${J43:-}" ]; then
	run revamp_solo_checks SAVE0="$J43/p0.sav" SAVE1="$J43/p1.sav"
	run revamp_trainers_early SAVE0="$J43/p0.sav" SAVE1="$J43/p1.sav"
	# Trainer Exchange stock at 4 BADGES.
	run exchange_stock_t2 SAVE0="$J43/p0.sav" SAVE1="$J43/p1.sav"
fi
# J06=dir: checkpoint 6 (Rustboro before Roxanne): the Trainer Exchange's
# stock with no BADGE.
if [ -n "${J06:-}" ]; then
	run exchange_stock_t0 SAVE0="$J06/p0.sav" SAVE1="$J06/p1.sav"
fi
# J13/J44/J59=dir: checkpoints 13 (Rustboro Gym ahead), 44 (Petalburg, four
# badges) and 59 (Mossdeep, Seafloor Cavern ahead): a gym's shared fight
# while the partner is in a menu, a message or stays busy; Strength
# boulders are each save's own.
if [ -n "${J13:-}" ]; then
	run mandatory_busy_talk SAVE0="$J13/p0.sav" SAVE1="$J13/p1.sav"
	# Team preparation (0xC017013E): both players choose from any slot; a
	# won fight returns everyone to their own slot; reload; solo lead.
	run prep_win_j13 SAVE0="$J13/p0.sav" SAVE1="$J13/p1.sav"
	run prep_win_j13_reload SAVE0="$out/prep_win_j13/p0.sav" SAVE1="$out/prep_win_j13/p1.sav"
	run prep_solo_lead SAVE0="$J13/p0.sav" SAVE1="$J13/p1.sav"
	run field_kit SAVE0="$J13/p0.sav" SAVE1="$J13/p1.sav"
fi
if [ -n "${J44:-}" ]; then
	run mandatory_busy_menu SAVE0="$J44/p0.sav" SAVE1="$J44/p1.sav"
	run mandatory_busy_timeout SAVE0="$J44/p0.sav" SAVE1="$J44/p1.sav"
	run prep_slots456 SAVE0="$J44/p0.sav" SAVE1="$J44/p1.sav"
fi
if [ -n "${J59:-}" ]; then
	run coop_boulder_personal SAVE0="$J59/p0.sav" SAVE1="$J59/p1.sav"
fi
if [ -n "${J48:-}" ]; then
	run revamp_trainers_mid SAVE0="$J48/p0.sav" SAVE1="$J48/p1.sav"
fi

if [ -n "${J37:-}" ]; then
	for s in scene_meteor_join scene_meteor_menu; do
		run "$s" SAVE0="$J37/p0.sav" SAVE1="$J37/p1.sav"
	done
	# Trainer Exchange in co-op (both at one counter, mixed BADGES, gifts,
	# bag full, too poor), then power-on.
	run exchange_coop SAVE0="$J37/p0.sav" SAVE1="$J37/p1.sav"
	run exchange_coop_reload SAVE0="$out/exchange_coop/p0.sav" SAVE1="$out/exchange_coop/p1.sav"
fi

# J65=dir with p0.sav/p1.sav from the main journey's checkpoint 65 (Ever
# Grande, eight badges): late-game co-op regressions.
if [ -n "${J65:-}" ]; then
	run coop_wally_vr_partner SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	run story_beacon_off_wally SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
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
	run roster_b3_evo_place SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav" RTC_EPOCH0=1699923600 RTC_EPOCH1=1699963200
	run roster_b3_evo_misc SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav" RTC_EPOCH0=1699923600 RTC_EPOCH1=1699963200
	run roster_b3_services SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	# Summary-screen evolution hints (SELECT on the INFO page).
	run evo_hint SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav" RTC_EPOCH0=1699963200
	# Trainer Exchange training counter and candies at the cap; the Day
	# Care with both players, the breeding rules, and their power-on.
	run exchange_training SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	# Egg rolls are random (one try per 256 steps): room beyond the default budget.
	run breeding_coop SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav" MAX_FRAMES=400000
	run breeding_rules SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
	run breeding_rules_reload SAVE0="$out/breeding_rules/p0.sav" SAVE1="$out/breeding_rules/p1.sav"
fi

# J52=dir with p0.sav/p1.sav from checkpoint 52 (Lilycove, linked): normal
# wild encounters after Route 119 (grass, Surf, fishing, catch, run,
# Teleport, a building transition between battles).
if [ -n "${J52:-}" ]; then
	run visitors_weather_fish SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"
	run learn_who SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"
	run learn_who_full SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"
	run late_wild_tour SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav" MAX_FRAMES=500000
	run revamp_trainers_late SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"
	run exchange_stock_t3 SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"
fi

# J68=dir with p0.sav/p1.sav from the main journey's checkpoint 68 (after the
# Hall of Fame, Rayquaza waiting at the Sky Pillar top on both saves): shared
# legendaries (src/coop_legend.c) and the partner indicator tour, then the
# runs that continue their saves (reload, reconnect).
if [ -n "${J68:-}" ]; then
	run habitat_tour SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	run towns_tour SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	# FIELD NOTES on a save that reached rewards before loading: no payout on
	# the first steps, collected in the notes, kept after a reload.
	run notes_catchup SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	run notes_catchup_collect SAVE0="$out/notes_catchup/p0.sav" SAVE1="$out/notes_catchup/p1.sav"
	run notes_catchup_reload SAVE0="$out/notes_catchup_collect/p0.sav" SAVE1="$out/notes_catchup_collect/p1.sav"
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
	# Catches by hunting: luck sets the length; room beyond the default budget.
	run roster_b3_catch SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav" MAX_FRAMES=500000
	# Joining a partner's conversation; the partner's appearance self-check.
	run talk_join SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	run partner_appearance SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	run exchange_stock_t5 SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	for s in shoal_solo caravan_solo coast_solo twins_solo beacon_solo pp_solo; do
		run_as "${s}_j68" "$s" SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	done
fi

# SEG6=dir with p0.sav/p1.sav from story_seg6 (Rustboro, linked play): the
# partner menu, QoL, and Power Problems scenarios, then their reloads.
if [ -n "${SEG6:-}" ]; then
	for s in deal_trade deal_cut_before_partner deal_cut_after_commit deal_lost_ready deal_gifts deal_edge \
	         deal_battle deal_battle_doubles deal_battle_rules deal_dex deal_fast practice save_identity \
	         pp_solo pp_coop pp_reconnect roster_g exchange_counters exchange_moves exchange_old_spot coop_secret_base_room; do
		run "$s" SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	done
	for s in deal_trade deal_cut_before_partner deal_cut_after_commit deal_gifts exchange_old_spot; do
		run "${s}_reload" SAVE0="$out/$s/p0.sav" SAVE1="$out/$s/p1.sav"
	done
	run deal_dex_offline SAVE0="$out/deal_dex/p0.sav" SAVE1="$out/deal_dex/p1.sav"
	run pp_stagger SAVE0="$out/pp_solo/p0.sav" SAVE1="$SEG6/p1.sav"
	run pp_stagger_reload SAVE0="$out/pp_solo/p0.sav" SAVE1="$out/pp_stagger/p1.sav"
	# Relink ownership policy (0xC017013E): setup unlinked, link, power cycle.
	run relink_policy_setup SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run relink_policy SAVE0="$out/relink_policy_setup/p0.sav" SAVE1="$out/relink_policy_setup/p1.sav"
	run relink_policy_reload SAVE0="$out/relink_policy/p0.sav" SAVE1="$out/relink_policy/p1.sav"
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
	# Activity icons, the edge arrow; partner menu VIEW TEAM, PING, FOLLOW.
	run partner_activity SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run partner_menu SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# One trainer, two of his Pokemon down on one turn, one left in reserve.
	run coop_one_trainer_faints SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run coop_admin_flag SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# First roster batch: new species caught, traded, gifted (party and PC),
	# reloaded; the partner-Pokemon badge gate.
	run roster_b1_play SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" MAX_FRAMES=400000
	run roster_b1_reload SAVE0="$out/roster_b1_play/p0.sav" SAVE1="$out/roster_b1_play/p1.sav"
	run roster_b1_gate SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	# Revamps: an owned revamped Pokemon's stats after a reload.
	run revamp_owned_stats SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run revamp_owned_stats_reload SAVE0="$out/revamp_owned_stats/p0.sav" SAVE1="$out/revamp_owned_stats/p1.sav"
	run lighting_link SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699963200 RTC_EPOCH1=1699923600
	# Second roster batch in play: catch, trade, store, reload.
	run roster_b2_play SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run roster_b2_reload SAVE0="$out/roster_b2_play/p0.sav" SAVE1="$out/roster_b2_play/p1.sav"
	run roster_b3_play SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	run roster_b3_reload SAVE0="$out/roster_b3_play/p0.sav" SAVE1="$out/roster_b3_play/p1.sav"
	# Quests: Shoal Cave at both tides (and left half done across a reload),
	# loss/retreat/retry, a busy partner and simultaneous interaction.
	run_as shoal_solo_high shoal_solo SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699963200
	run_as shoal_solo_low shoal_solo SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699938000
	run shoal_vulpix_full_party SAVE0="$out/shoal_solo_high/p0.sav" SAVE1="$out/shoal_solo_high/p1.sav"
	run shoal_tide_a SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav" RTC_EPOCH0=1699963200
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

# Battle Guide v2 (build 0xC017013D): team selection, rival routing by
# starter case, boss pairs, level caps, Mega Evolution, Mega Stones and the
# personal item balls. Lab saves always; the rest when its checkpoint is
# given (J46 Route 118 before Steven's scene, J50 five badges with the
# Devon Scope, J51 six badges, J57 Mossdeep, J58 Mossdeep before the Space
# Center, J66 League 1F before the guards; OLD13C_COAST: coast_solo_j68's
# saves from a 0xC017013C suite, a quest finished before Mega Stones).
run guide_team_select SAVE0="$lab0" SAVE1="$lab1"
run catch_before_dex SAVE0="$lab0" SAVE1="$lab1"
for s in story_consent story_beacon_off story_cancel story_unanswered story_simultaneous invite_expiry_clock \
         prep_cancel prep_timeout prep_timeout_host_first prep_link_stall battle_visual_wild_double wild_enemy_slots startup_timing_wild startup_timing_gym \
         party_exp_wild party_exp_levelup party_exp_solo party_exp_solo_levelup vial_team_heal move_effectiveness \
         catch_full_party_solo catch_full_party_cancel catch_full_party_linked_solo hm_no_mon hm_party_menu \
         appearance_menu move_inspector story_not_ready debug_inputs_off battle_spare_buttons; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done
# Autosave (after 0xC0170141): slot roles, the question after CONTINUE,
# going back to the manual save, power cut (once and twice), setting,
# linked, NEW GAME over an autosave, the map view an autosave stores.
run autosave_basic SAVE0="$lab0" SAVE1="$lab1"
for s in autosave_reload autosave_load_manual autosave_power_cut autosave_go_back_autosave autosave_new_game_over; do
	run "$s" SAVE0="$out/autosave_basic/p0.sav" SAVE1="$out/autosave_basic/p1.sav"
done
run autosave_load_manual_reload SAVE0="$out/autosave_load_manual/p0.sav" SAVE1="$out/autosave_load_manual/p1.sav"
run autosave_power_cut_load SAVE0="$out/autosave_power_cut/p0.sav" SAVE1="$out/autosave_power_cut/p1.sav"
run autosave_power_cut_twice SAVE0="$out/autosave_power_cut/p0.sav" SAVE1="$out/autosave_power_cut/p1.sav"
run autosave_power_cut_twice_load SAVE0="$out/autosave_power_cut_twice/p0.sav" SAVE1="$out/autosave_power_cut_twice/p1.sav"
run autosave_go_back_autosave_reload SAVE0="$out/autosave_go_back_autosave/p0.sav" SAVE1="$out/autosave_go_back_autosave/p1.sav"
for s in autosave_off autosave_coop; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done
run autosave_mapview SAVE0="$lab0" SAVE1="$lab1"
run autosave_mapview_reload SAVE0="$out/autosave_mapview/p0.sav" SAVE1="$out/autosave_mapview/p1.sav"
# Waiting at a Pokemon Center (after 0xC0170141).
run centre_wait_solo SAVE0="$lab0" SAVE1="$lab1"
run centre_wait_solo_reload SAVE0="$out/centre_wait_solo/p0.sav" SAVE1="$out/centre_wait_solo/p1.sav"
run centre_wait_coop SAVE0="$lab0" SAVE1="$lab1" RTC_EPOCH0=1699963200 RTC_EPOCH1=1699923600
run visitors_time SAVE0="$lab0" SAVE1="$lab1"
# After 0xC0170142: the trainer AI's ability guess with a negative rating
# (Roxanne's crash), one-trainer co-op HP boxes and replacements, ROAR in
# a one-trainer co-op battle, EXP from the second trainer of a pair, the
# CONTINUE screen's top row (shots).
for s in ai_ability_klutz_coop coop_one_trainer_hpbox coop_one_trainer_replace coop_roar_one_trainer coop_pair_exp continue_screen_names; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done
run notes_rewards SAVE0="$lab0" SAVE1="$lab1"
run notes_rewards_reload SAVE0="$out/notes_rewards/p0.sav" SAVE1="$out/notes_rewards/p1.sav"
# The same expiry by the wall clock while the harness runs faster than real
# time: more frames pass, the same 30 seconds of clock time.
run_as invite_expiry_clock_wall invite_expiry_clock SAVE0="$lab0" SAVE1="$lab1" RTC_WALL0=1 RTC_WALL1=1
for s in boss_pair_solo boss_pair_coop rival_case_grass rival_case_fire rival_case_water; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done
if [ -n "${J16:-}" ]; then
	run items_personal SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	run habitat_moss SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	run items_personal_reload SAVE0="$out/items_personal/p0.sav" SAVE1="$out/items_personal/p1.sav"
fi
if [ -n "${J46:-}" ]; then
	run mega_bracelet SAVE0="$J46/p0.sav" SAVE1="$J46/p1.sav"
	run mega_bracelet_reload SAVE0="$out/mega_bracelet/p0.sav" SAVE1="$out/mega_bracelet/p1.sav"
fi
# Duo Frontier (0xC017013E) and optional travel, on the champions' saves.
if [ -n "${J68:-}" ]; then
	run duo_tower_first SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	for s in duo_tower_set duo_tower_abort duo_tower_behind duo_tower_partners duo_tower_decline; do
		run "$s" SAVE0="$out/duo_tower_first/p0.sav" SAVE1="$out/duo_tower_first/p1.sav"
	done
	run duo_tower_set_reload SAVE0="$out/duo_tower_set/p0.sav" SAVE1="$out/duo_tower_set/p1.sav"
	for s in duo_desks opt_escape_dig opt_travel opt_contest sustained_session; do
		run "$s" SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
	done
fi
[ -n "${J50:-}" ] && run rival_lilycove_early SAVE0="$J50/p0.sav" SAVE1="$J50/p1.sav"
[ -n "${J51:-}" ] && run rival_lilycove_late SAVE0="$J51/p0.sav" SAVE1="$J51/p1.sav"
if [ -n "${J57:-}" ]; then
	for s in mega_coop_tate mega_cancel mega_abort mega_no_bracelet mega_za_abilities mega_hp_same mega_exchange; do
		run "$s" SAVE0="$J57/p0.sav" SAVE1="$J57/p1.sav"
	done
fi
[ -n "${J58:-}" ] && run space_center_solo SAVE0="$J58/p0.sav" SAVE1="$J58/p1.sav"
[ -n "${J66:-}" ] && run caps_league SAVE0="$J66/p0.sav" SAVE1="$J66/p1.sav"
[ -n "${J68:-}" ] && run caps_champion SAVE0="$J68/p0.sav" SAVE1="$J68/p1.sav"
if [ -n "${SEG6:-}" ]; then
	for s in exh_case_grass exh_case_fire exh_case_water; do
		run "$s" SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	done
fi
[ -n "${OLD13C_COAST:-}" ] && run quest_stone_catchup SAVE0="$OLD13C_COAST/p0.sav" SAVE1="$OLD13C_COAST/p1.sav"

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

# Save storage (collection update, docs/COOP_STORAGE_PLAN.md): a power cut
# halfway through a save, and a save from a newer build (fixture copies).
if [ -n "${J16:-}" ]; then
	run save_power_cut SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	run save_power_cut_load SAVE0="$out/save_power_cut/p0.sav" SAVE1="$out/save_power_cut/p1.sav"
fi
if [ -n "${J68:-}" ]; then
	mkdir -p "$out/fx_newer"
	python3 "$here/sav_poke.py" "$J68/p0.sav" "$out/fx_newer/p0.sav" 13 6
	python3 "$here/sav_poke.py" "$J68/p1.sav" "$out/fx_newer/p1.sav" 4 0x5A
	run save_schema_newer SAVE0="$out/fx_newer/p0.sav" SAVE1="$out/fx_newer/p1.sav"
	for i in 0 1; do
		cmp -s "$out/fx_newer/p$i.sav" "$out/save_schema_newer/p$i.sav" \
			|| { echo "save_schema_newer: p$i.sav was changed" | tee -a "$summary"; failed=$((failed + 1)); }
	done
fi

# Polish (docs/COOP_POLISH.md): personal settings, shiny star and SHINY
# CHECK, catch result, low-HP alarm, battle conditions panel (lab saves);
# EASY FISHING (J52).
for s in shiny_run shiny_coop catch_result lowhp_beeps battle_panel_coop; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done
[ -n "${J52:-}" ] && run easy_fishing SAVE0="$J52/p0.sav" SAVE1="$J52/p1.sav"

# Trainer Bond (docs/COOP_BOND.md). Lab saves: co-op XP and agreement,
# support XP (solo and the guest), the badge's one-time bonus. J16 (a save
# from before the update): the mentor, ranks and reload, wild XP. SEG6:
# trade, practice. J29 (Slateport Centre): the PC, a Battle Tent. J65:
# evolution and SHEDINJA. J57: Mega Evolution in a co-op double.
for s in bond_coop_wild bond_support bond_coop_support bond_major; do
	run "$s" SAVE0="$lab0" SAVE1="$lab1"
done
if [ -n "${J16:-}" ]; then
	run bond_mentor SAVE0="$J16/p0.sav" SAVE1="$J16/p1.sav"
	run bond_ranks SAVE0="$out/bond_mentor/p0.sav" SAVE1="$out/bond_mentor/p1.sav"
	run bond_ranks_reload SAVE0="$out/bond_ranks/p0.sav" SAVE1="$out/bond_ranks/p1.sav"
	run bond_xp_wild SAVE0="$out/bond_mentor/p0.sav" SAVE1="$out/bond_mentor/p1.sav"
fi
if [ -n "${SEG6:-}" ]; then
	for s in bond_trade bond_practice; do
		run "$s" SAVE0="$SEG6/p0.sav" SAVE1="$SEG6/p1.sav"
	done
fi
if [ -n "${J29:-}" ]; then
	for s in bond_box bond_tent; do
		run "$s" SAVE0="$J29/p0.sav" SAVE1="$J29/p1.sav"
	done
fi
[ -n "${J65:-}" ] && run bond_evolve SAVE0="$J65/p0.sav" SAVE1="$J65/p1.sav"
[ -n "${J57:-}" ] && run bond_mega SAVE0="$J57/p0.sav" SAVE1="$J57/p1.sav"

echo "failed scenarios: $failed; visual-check failures: $visualFailed" | tee -a "$summary"
exit $failed
