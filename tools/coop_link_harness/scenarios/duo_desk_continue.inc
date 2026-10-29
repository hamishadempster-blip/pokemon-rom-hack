# DUO DESK, a run open on both saves: Kai (player 0, below the desk) chooses
# CONTINUE; Rosa answers YES; both team screens are up with the run's team.
0 press UP 2
0 wait 20
0 press A 2
0 wait 90
0 until_task Task_HandleScrollingMultichoiceInput 3000 A
0 wait 30
0 shot desk-run-menu
0 press A 2
1 until_mem coop.sGuestPairPending 1 3000
1 wait 90
1 press A 2
b until_mem gCoopPrepPhase 1 1500
b wait 150
