#!/bin/bash
# run.sh LANE [EPOCHS]: half of the #16 matrix
cd /home/claude/pokemon-rom-hack
out=/home/claude/journey/m16
eps=${2:-1700000000}
i=0
for f in tools/coop_link_harness/scenarios/gen16_*.txt; do
  n=$(basename $f .txt); i=$((i+1)); [ $((i % 2)) -eq $1 ] || continue
  for e in $eps; do
    env BATTLE_LOG=1 RTC_EPOCH0=$e RTC_EPOCH1=$((e+7)) SAVE0=/home/claude/journey/j16/p0.sav SAVE1=/home/claude/journey/j16/p1.sav HARNESS_OUT=${HB:-/tmp/hb_K} timeout 1500 tools/coop_link_harness/run_scenario.sh $f $out/${n}_$e 400000 > $out/${n}_$e.log 2>&1
    echo "${n}_$e $(grep -c 'ASSERT CRASH' $out/${n}_$e.log) asserts | $(grep -cE 'challenged by' $out/${n}_$e.log) challenges | $(grep -c 'FAIL' $out/${n}_$e.log) FAIL | $(tail -1 $out/${n}_$e.log)" >> $out/summary.txt
  done
done
echo done$1 >> $out/summary.txt
