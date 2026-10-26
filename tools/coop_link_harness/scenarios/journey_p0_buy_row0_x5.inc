# (generated) player 0 buys mart list row 0 x5 (counter: clerk at 1,3,
# talk from 3,3). Waits on the shop's own tasks, so slow text cannot
# shift the presses.
0 walk 3 3 2000
0 press LEFT 2
0 wait 10
0 press A 2
0 until_task Task_ShopMenu 900 A
0 wait 20
0 wait 60
0 until_task Task_BuyMenu 900 A
0 wait 30
0 press A 2
0 until_task Task_BuyHowManyDialogueHandleInput 600
0 wait 10
0 press UP 2
0 wait 10
0 press UP 2
0 wait 10
0 press UP 2
0 wait 10
0 press UP 2
0 wait 10
0 press A 2
0 wait 60
0 press A 2
0 wait 60
0 wait 60
0 until_task Task_BuyMenu 900 A
0 wait 20
0 press B 2
0 until_task Task_ShopMenu 900 A
0 wait 20
0 press B 2
0 mash_field 2000 B
