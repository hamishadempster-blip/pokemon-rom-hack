# Both boys go up to May's bedroom and meet her at nearly the same time:
# player 0 from the north of the Poke Ball, player 1 from the west.
b sync
0 goto 2 3
0 press UP 30
0 wait 60
0 until_field 600
0 goto 5 3
b sync
1 goto 2 3
1 press UP 30
1 wait 60
1 until_field 600
1 goto 4 4
b sync
b shot upstairs
0 press DOWN 4
0 wait 10
0 press A 2
1 wait 40
1 press RIGHT 4
1 wait 10
1 press A 2
b wait 60
b shot maymeet
b mash_field 4000
