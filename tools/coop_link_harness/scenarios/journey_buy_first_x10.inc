# Both players buy ten of the first item in a Mart with the standard
# counter (clerk at 1,3; talk from 3,3), player 1 after player 0: talk,
# "Welcome!", BUY, the first row; quantity RIGHT (+10) and DOWN (-1).
# player 0: ten of the first item (RIGHT +10 from 1, DOWN -1), with time
# for the "How many would you like?" text before the quantity box.
0 walk 3 3 2000
0 press LEFT 2
0 wait 10
0 press A 2
0 wait 120
0 press A 2
0 wait 120
0 press A 2
0 wait 120
0 press A 2
0 wait 240
0 press RIGHT 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press A 2
0 wait 120
0 press A 2
0 wait 120
0 press A 2
0 wait 120
0 press B 2
0 wait 60
0 mash_field 2000 B
1 wait 120
# player 1: ten of the first item (RIGHT +10 from 1, DOWN -1), with time
# for the "How many would you like?" text before the quantity box.
1 walk 3 3 2000
1 press LEFT 2
1 wait 10
1 press A 2
1 wait 120
1 press A 2
1 wait 120
1 press A 2
1 wait 120
1 press A 2
1 wait 240
1 press RIGHT 2
1 wait 20
1 press DOWN 2
1 wait 20
1 press A 2
1 wait 120
1 press A 2
1 wait 120
1 press A 2
1 wait 120
1 press B 2
1 wait 60
1 mash_field 2000 B
b sync
