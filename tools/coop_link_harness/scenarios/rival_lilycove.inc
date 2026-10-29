# Lilycove rival fight, shared by rival_lilycove_early/late: linked journey
# saves; FIXTURE: fixture_warp below the rival (27,7) instead of walking
# Routes 120/121; strong cached stats on the leads (flow, not balance).
include continue.inc
include relink.inc
b expect coop.sLinkState 2
b no_wild 1
b mash_field 2000 B
0 fixture_warp 0 5 27 8
1 fixture_warp 0 5 26 8
b until_field 3000
b sync
b wait 40
b sync
b expect_any_object 246
b shot lilycove-before
b write gParties+86:2 999
b write gParties+88:2 999
b write gParties+90:2 600
b write gParties+92:2 600
b write gParties+94:2 600
b write gParties+96:2 600
b write gParties+98:2 600
0 press UP 2
0 wait 10
0 press A 2
b until_cb2 BattleMainCB2 12000 A
b expect coop.sInBattle 1
b expect gBattlersCount 4
b expect coop.sRivalFightStage 4
