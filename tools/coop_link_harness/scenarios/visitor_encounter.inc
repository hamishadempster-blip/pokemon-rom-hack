# (Included.) Player 0 meets one wild Pokemon in Route 101's grass and runs.
# FIXTURE force_encounter (the next grass step rolls an encounter).
0 fixture_warp 0 16 13 5
0 until_field 3000
0 wait 30
0 force_encounter 1
0 no_wild 0
0 pace_until_battle 3000
0 force_encounter 0
0 no_wild 1
0 until_cb2 BattleMainCB2 3000
0 hex gCoopVisitorLast 2
0 until_mem gBattlerControllerFuncs:4 HandleInputChooseAction 6000 A
0 wait 30
0 press RIGHT 2
0 wait 6
0 press DOWN 2
0 wait 6
0 press A 2
0 mash_field 12000 B
