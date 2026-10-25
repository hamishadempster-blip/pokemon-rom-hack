# One gym fight from the leader's tile. Set GYM_* through the caller:
# this file is included after the caller's fixture_warp lines.
b until_field 3000
b sync
b wait 40
b sync
b write gParties+86:2 999
b write gParties+88:2 999
b write gParties+90:2 600
b write gParties+92:2 600
b write gParties+94:2 600
b write gParties+96:2 600
b write gParties+98:2 600
0 press UP 2
0 wait 20
b note_money
0 press A 2
b until_cb2 BattleMainCB2 9000 A
b expect coop.sInBattle 1
b expect gBattlersCount 4
0 until_mem gBattlerControllerFuncs:4 HandleInputChooseAction 6000 A
1 until_mem gBattlerControllerFuncs+8:4 HandleInputChooseAction 6000 A
b wait 30
