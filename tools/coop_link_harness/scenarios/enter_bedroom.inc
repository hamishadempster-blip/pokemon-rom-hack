# From outside the lab, player 0 then player 1 walk into May's house and
# up to her bedroom. The second player arrives while the first is there.
0 goto 14 9 2500
0 press UP 40
0 wait 60
0 until_field 900
0 goto 2 3
0 press UP 30
0 wait 60
0 until_field 900
0 expect_map 1 3
b sync
1 goto 14 9 2500
1 press UP 40
1 wait 60
1 until_field 900
1 goto 2 3
1 press UP 30
1 wait 60
1 until_field 900
1 expect_map 1 3
b wait 30
