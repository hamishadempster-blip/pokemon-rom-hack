# (generated) player 1 buys mart list row 2 x3 (counter: clerk at 1,3,
# talk from 3,3). Waits on the shop's own tasks, so slow text cannot
# shift the presses.
1 walk 3 3 2000
1 press LEFT 2
1 wait 10
1 press A 2
1 until_task Task_ShopMenu 900 A
1 wait 20
1 wait 60
1 until_task Task_BuyMenu 900 A
1 wait 30
1 press DOWN 2
1 wait 12
1 press DOWN 2
1 wait 12
1 press A 2
1 until_task Task_BuyHowManyDialogueHandleInput 600
1 wait 10
1 press UP 2
1 wait 10
1 press UP 2
1 wait 10
1 press A 2
1 wait 60
1 press A 2
1 wait 60
1 wait 60
1 until_task Task_BuyMenu 900 A
1 wait 20
1 press B 2
1 until_task Task_ShopMenu 900 A
1 wait 20
1 press B 2
1 mash_field 2000 B
