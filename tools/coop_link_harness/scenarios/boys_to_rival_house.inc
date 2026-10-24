# Both players pick the boy, link, watch the TV scene in the same house,
# then walk into May's house one after the other.
include boot_boys.inc
include link.inc
0 goto 7 2
0 press UP 20
0 wait 120
0 mash_field 2500
b sync
1 goto 7 2
1 press UP 20
1 wait 120
1 mash_field 2500
b sync
0 goto 8 7
0 press DOWN 50
0 wait 40
0 until_field 600
0 goto 14 9 1500
0 press UP 40
0 wait 60
0 mash_field 3000
b sync
1 goto 8 7
1 press DOWN 50
1 wait 40
1 until_field 600
1 goto 14 9 1500
1 press UP 40
1 wait 60
1 mash_field 3000
