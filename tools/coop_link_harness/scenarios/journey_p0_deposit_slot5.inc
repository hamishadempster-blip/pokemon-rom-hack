# Player 0 deposits party slot 5 in the Centre PC (PC at 10,1; SOMEONE'S PC,
# DEPOSIT POKeMON, the slot, DEPOSIT, the first box), then leaves the PC.
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
0 shot pc-menu
0 press DOWN 2
0 wait 30
0 press A 2
0 wait 200
0 shot pc-party
0 press DOWN 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press A 2
0 wait 60
0 shot pc-mon-menu
0 press A 2
0 wait 90
0 shot pc-box
0 press A 2
0 wait 200
0 shot pc-deposited
0 press B 2
0 wait 120
0 press B 2
0 wait 120
0 mash_field 3000 B
0 party
