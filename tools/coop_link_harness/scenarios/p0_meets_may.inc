# Player 0 alone goes up to May's bedroom and meets her at the Poke Ball.
0 goto 2 3
0 press UP 30
0 wait 60
0 until_field 600
0 goto 5 3
0 press DOWN 4
0 wait 10
0 press A 2
0 wait 60
0 mash_field 4000
0 expect_var 0x408D 3
0 goto 1 2
0 press UP 30
0 wait 60
0 until_field 600
0 expect_map 1 2
