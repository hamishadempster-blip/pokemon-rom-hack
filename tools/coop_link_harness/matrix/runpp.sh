#!/bin/bash
# mpp_run.sh LANE: half of the pp_coop timing matrix
cd /home/claude/pokemon-rom-hack
out=/home/claude/journey/mpp; mkdir -p $out
i=0
for f in tools/coop_link_harness/scenarios/genpp_*.txt; do
  n=$(basename $f .txt); i=$((i+1)); [ $((i % 2)) -eq $1 ] || continue
  env BATTLE_LOG=2 SAVE0=/tmp/claude-0/seg6/p0.sav SAVE1=/tmp/claude-0/seg6/p1.sav HARNESS_OUT=${HB:-/tmp/hb_K} timeout 1500 tools/coop_link_harness/run_scenario.sh $f $out/$n 400000 > $out/$n.log 2>&1
  echo "$n | $(grep -c 'ASSERT CRASH' $out/$n.log) asserts | $(grep -c 'FAIL' $out/$n.log) FAIL | $(grep -c 'ABORTED\|UNEXPECTED RESET' $out/$n.log) abort | $(tail -1 $out/$n.log)" >> $out/summary.txt
done
echo done$1 >> $out/summary.txt
