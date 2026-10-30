# Player 0 withdraws the Pokemon in box 1 slot 1 from the Centre PC (PC at
# 10,1; SOMEONE'S PC, the third choice WITHDRAW POKeMON, the first box slot, WITHDRAW): it
# joins the end of the party. Then leaves the PC.
0 walk 10 2 3000
0 press UP 2
0 wait 10
0 press A 2
0 until_task Task_HandleMultichoiceInput 2000 A
0 wait 30
0 press A 2
0 wait 200
0 press A 2
0 wait 100
0 press A 2
0 wait 200
0 shot pc-menu-w
0 press DOWN 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press A 2
0 wait 200
0 shot pc-withdraw-box
0 press A 2
0 wait 60
0 shot pc-withdraw-menu
0 press A 2
0 wait 200
0 shot pc-withdrawn
0 press B 2
0 wait 120
0 press B 2
0 wait 120
0 mash_field 3000 B
0 party
