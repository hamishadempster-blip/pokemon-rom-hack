#!/bin/bash
# pp_coop timing matrix (generated, not committed): VOLT + FUSE duo with
# varied roles, choice delays, replacements, simultaneous faints and a link
# interruption before the fight. story_seg6 saves.
cd /home/claude/pokemon-rom-hack/tools/coop_link_harness/scenarios
rm -f genpp_*.txt
hdr() {
cat <<H
# GENERATED pp_coop timing matrix variant $1 (not committed).
# FIXTURES: Wattson's key step, breakers solved (var 0x40f7=3), warps,
# no random wild battles; $2.
include continue.inc
include relink.inc
b expect coop.sLinkState 2
b no_wild 1
b smart_moves 1
b fixture_flag 0xd0 1
b fixture_var 0x40ba 1
1 press UP 1
1 wait 20
0 press START 2
0 wait 40
0 menu_to COOP
0 press A 2
0 wait 60
0 press A 2
1 until_mem coop.sBeacon+1:1 1 3000 A
0 mash_field 1200 B
b until_mem coop.sBeacon+1:1 1 1200
b mash_field 1500 B
b wait 30
b expect coop.sBeacon+1:1 1
b fixture_flag 0x28c 1
b fixture_flag 0x28d 1
b fixture_var 0x40f7 3
H
}
# $1 asker  $2 strong(1/0)  $3 hp-fixture lines  $4 pre-fight lines  $5 asker battle delay  $6 joiner battle delay
gen() {
  local name=$1 a=$2 strong=$3 hp=$4 pre=$5 da=$6 dj=$7 note=$8
  local j=$((1-a))
  {
  hdr $name "$note"
  [ "$strong" = 1 ] && echo "b strong_battlers 600"
  echo "$a fixture_warp 24 53 33 8"
  echo "$j fixture_warp 24 53 31 7"
  echo "b until_field 3000"
  echo "b sync"
  printf "%b" "$pre"
  echo "b wait 60"
  printf "%b" "$hp"
  echo "$a walk 33 7 600"
  echo "$a press UP 2"
  echo "$a wait 20"
  echo "$a press A 2"
  echo "$a wait 200"
  echo "$a press A 2"
  echo "$a wait 120"
  echo "$a press A 2"
  echo "$j until_mem sGlobalScriptContextStatus:1 1 3000"
  echo "$j wait 120"
  echo "$j press A 2"
  echo "b until_cb2 BattleMainCB2 3000 A"
  echo "b shot duo-battle"
  [ "$da" != 0 ] && echo "$a wait $da"
  [ "$dj" != 0 ] && echo "$j wait $dj"
  echo "b mash_field 40000"
  echo "b mash_field 4000"
  echo "b until_field 3000"
  echo "b expect_flag 0x855 1"
  echo "b expect_flag 0x856 1"
  echo "b expect_object 11 32 7"
  echo "b expect_tasks MAX 2"
  echo "b expect coop.sLinkState 2"
  } > genpp_$name.txt
}
hp0="0 write gParties+86:2 1\n"; hp1="1 write gParties+86:2 1\n"
mute="b mute_coop 300\nb wait 420\nb until_mem coop.sLinkState 2 3000\n"
relink="include unlink.inc\nb wait 120\ninclude relink.inc\nb expect coop.sLinkState 2\nb expect coop.sBeacon+1:1 0\n1 press UP 1\n1 wait 20\n0 press START 2\n0 wait 40\n0 menu_to COOP\n0 press A 2\n0 wait 60\n0 press A 2\n1 until_mem coop.sBeacon+1:1 1 3000 A\n0 mash_field 1200 B\nb until_mem coop.sBeacon+1:1 1 1200\nb mash_field 1500 B\nb expect coop.sBeacon+1:1 1\n"
for a in 0 1; do
  gen ask${a}_plain $a 1 "" "" 0 0 "strong battlers"
  gen ask${a}_delayj $a 1 "" "" 0 420 "strong battlers; the joiner waits 7 s before its first choice"
  gen ask${a}_delaya $a 1 "" "" 420 0 "strong battlers; the asker waits 7 s before its first choice"
  gen ask${a}_replace $a 1 "$hp0" "" 0 0 "strong battlers; player 0's lead at 1 HP (a replacement mid-fight)"
  gen ask${a}_simfaint $a 1 "$hp0$hp1" "" 0 0 "strong battlers; both leads at 1 HP (both faint in one turn, two replacements)"
  gen ask${a}_mute $a 1 "" "$mute" 0 0 "strong battlers; 5 s heartbeat loss right before the fight"
done
gen ask0_relink 0 1 "" "$relink" 0 0 "strong battlers; explicit UNLINK and LINK UP right before the fight (a new session: the Beacon is agreed again)"
ls genpp_* | wc -l
