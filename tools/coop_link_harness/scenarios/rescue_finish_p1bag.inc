# Same as rescue_finish.inc, but player 0 holds the companion (Houndour)
# and picks its damaging second move; player 1's Torchic uses Scratch.
0 press A 2
1 press A 2
0 until_mem gBattlerControllerFuncs:4 HandleInputChooseMove 2000
1 until_mem gBattlerControllerFuncs+8:4 HandleInputChooseMove 2000
0 press RIGHT 2
b wait 20
0 press A 2
1 press A 2
b wait 30
b press A 2
b wait 60
b press A 2
b wait 60
0 until_nz gChosenMoveByBattler:2 A
0 until_nz gChosenMoveByBattler+4:2 A
1 press A 2
1 wait 20
1 press A 2
1 wait 20
1 press A 2
1 wait 20
1 press A 2
1 wait 20
1 press A 2
1 wait 20
1 press A 2
b expect_battle moves
b wait 1500
b expect_battle hp
b mash_field 20000
b dump afterBattle
b shot ab
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b press A 2
b wait 30
b wait 600
b dump lab
b expect gBattleOutcome 1
b party
b expect_partner lab
b shot lab
