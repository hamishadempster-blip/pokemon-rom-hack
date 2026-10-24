# Both boys leave May's house, pass the twin, and rescue Birch together.
# The bag, the battle, and the lab return are separate includes.
0 goto 1 2
0 press UP 30
0 wait 60
0 until_field 900
0 goto 2 7
0 press DOWN 50
0 wait 40
0 until_field 900
0 goto 11 2 1500
b sync
1 goto 1 2
1 press UP 30
1 wait 60
1 until_field 900
1 goto 2 7
1 press DOWN 50
1 wait 40
1 until_field 900
1 goto 10 2 1500
b sync
b wait 40
b expect_partner outside2
0 goto 11 1
b wait 40
b mash_field 2000
1 goto 11 2
1 goto 11 1
b wait 40
1 mash_field 2000
b expect_partner afterTwin
0 goto 11 1
b press UP 40
b wait 30
b mash_field 3000
b expect_partner route101
