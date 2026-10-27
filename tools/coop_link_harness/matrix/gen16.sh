#!/bin/bash
# Regenerate the finding #16 matrix (not committed).
cd /home/claude/pokemon-rom-hack/tools/coop_link_harness/scenarios
rm -f gen16_*.txt
hdr() {
cat <<H
# GENERATED (finding #16 matrix; not committed). j16 saves, Route 116 below
# School Kid Karen (22,16; faces down and up, sight 3). Beacon ON. FIXTURES:
# every Route 116 grass slot is ABRA (Teleport), warps to the lane.
include continue.inc
include relink.inc
b expect coop.sLinkState 2
b smart_moves 1
include beacon_on.inc
b mash_field 2000 B
b expect coop.sBeacon+1:1 1
b no_wild 1
H
for k in 0 1 2 3 4 5 6 7 8 9 10 11; do echo "b fixture_rom16 gRoute116_LandMons+$((k*4+2)) 63"; done
echo "0 fixture_warp 0 31 23 17"
echo "1 fixture_warp 0 31 25 17"
echo "b until_field 3000"
echo "b sync"
echo "b wait $1"
}
tail_() {
cat <<T
b until_cb2 BattleMainCB2 4000 A
b force_encounter 0
b no_wild 1
b shot first-battle
b until_cb2 CB2_Overworld 30000 A
b shot back
$1
b wait 600
b mash_field 40000
b wait 300
b shot end
b mash_field 20000
b expect_tasks MAX 2
T
}
for w in 0 37 91; do
 { hdr $w; echo "0 no_wild 0"; echo "0 press LEFT 3"; echo "0 wait 20"; echo "0 force_encounter 1"; echo "0 write sWildEncounterImmunitySteps:1 4"; echo "0 write sPrevMetatileBehavior:2 2"; echo "0 press LEFT 10"; tail_ ""; } > gen16_kai_$w.txt
 { hdr $w; echo "1 no_wild 0"; echo "1 press LEFT 3"; echo "1 wait 20"; echo "1 force_encounter 1"; echo "1 write sWildEncounterImmunitySteps:1 4"; echo "1 write sPrevMetatileBehavior:2 2"; echo "1 press LEFT 10"; tail_ "0 walk_try 22 17 400"; } > gen16_rosa_$w.txt
 { hdr $w; echo "b no_wild 0"; echo "b press LEFT 3"; echo "b wait 20"; echo "b force_encounter 1"; echo "b write sWildEncounterImmunitySteps:1 4"; echo "b write sPrevMetatileBehavior:2 2"; echo "b press LEFT 10"; tail_ ""; } > gen16_both_$w.txt
 { hdr $w; echo "1 no_wild 0"; echo "1 press LEFT 3"; echo "1 wait 20"; echo "1 force_encounter 1"; echo "1 write sWildEncounterImmunitySteps:1 4"; echo "1 write sPrevMetatileBehavior:2 2"; echo "1 press LEFT 10"; echo "0 walk_try 22 17 400"; tail_ ""; } > gen16_cross_$w.txt
 { hdr $w; echo "0 no_wild 0"; echo "0 press LEFT 3"; echo "0 wait 20"; echo "0 force_encounter 1"; echo "0 write sWildEncounterImmunitySteps:1 4"; echo "0 write sPrevMetatileBehavior:2 2"; echo "0 press LEFT 10"; echo "1 wait 90"; echo "1 press B 4"; echo "1 wait 30"; echo "1 press B 4"; tail_ ""; } > gen16_decline_$w.txt
done
# The recorded order: Karen looking away (sight hidden by fixture) while Kai
# reaches (22,17), a wild battle there, then her sight back on return.
blind_tail() {
cat <<T
b until_cb2 BattleMainCB2 4000 A
b force_encounter 0
b no_wild 1
b fixture_obj 17 7 1
b shot first-battle
b until_cb2 CB2_Overworld 30000 A
b shot back
b wait 600
b mash_field 40000
b wait 300
b shot end
b mash_field 20000
b expect gCoopDiag 0
b expect_tasks MAX 2
T
}
for w in 0 37 91; do
 { hdr $w; echo "b fixture_obj 17 7 0"; echo "0 no_wild 0"; echo "0 press LEFT 3"; echo "0 wait 20"; echo "0 force_encounter 1"; echo "0 write sWildEncounterImmunitySteps:1 4"; echo "0 write sPrevMetatileBehavior:2 2"; echo "0 press LEFT 10"; blind_tail; } > gen16_blindkai_$w.txt
 { hdr $w; echo "b fixture_obj 17 7 0"; echo "0 walk_try 22 17 400"; echo "1 wait 40"; echo "1 no_wild 0"; echo "1 press LEFT 3"; echo "1 wait 20"; echo "1 force_encounter 1"; echo "1 write sWildEncounterImmunitySteps:1 4"; echo "1 write sPrevMetatileBehavior:2 2"; echo "1 press LEFT 10"; blind_tail; } > gen16_blindrosa_$w.txt
 { hdr $w; echo "b fixture_obj 17 7 0"; echo "0 no_wild 0"; echo "0 press LEFT 3"; echo "0 wait 20"; echo "0 force_encounter 1"; echo "0 write sWildEncounterImmunitySteps:1 4"; echo "0 write sPrevMetatileBehavior:2 2"; echo "0 press LEFT 10"; echo "1 wait 90"; echo "1 press B 4"; echo "1 wait 30"; echo "1 press B 4"; blind_tail; } > gen16_blinddecl_$w.txt
done
ls gen16_* | wc -l
