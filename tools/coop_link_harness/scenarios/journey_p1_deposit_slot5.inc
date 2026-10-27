# Player 1 deposits party slot 5 in the Centre PC (PC at 10,1; SOMEONE'S PC,
# DEPOSIT POKeMON, the slot, DEPOSIT, the first box), then leaves the PC.
1 walk 10 2 3000
1 press UP 2
1 wait 10
1 press A 2
1 until_task Task_HandleMultichoiceInput 2000 A
1 wait 30
1 press A 2
1 wait 200
1 press A 2
1 wait 100
1 press A 2
1 wait 200
1 shot pc-menu
1 press DOWN 2
1 wait 30
1 press A 2
1 wait 200
1 shot pc-party
1 press DOWN 2
1 wait 20
1 press DOWN 2
1 wait 20
1 press DOWN 2
1 wait 20
1 press DOWN 2
1 wait 20
1 press DOWN 2
1 wait 20
1 press A 2
1 wait 60
1 shot pc-mon-menu
1 press A 2
1 wait 90
1 shot pc-box
1 press A 2
1 wait 200
1 shot pc-deposited
1 press B 2
1 wait 120
1 press B 2
1 wait 120
1 mash_field 3000 B
1 party
