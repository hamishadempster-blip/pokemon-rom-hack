# Player 1 had not met May. Going upstairs now must not start the
# introduction: the shared state lands on the stairs, May waits at the PC,
# talking to her is her short line, and the Poke Ball is gone.
1 goto 2 3
1 press UP 30
1 wait 60
1 until_field 600
1 expect_map 1 3
1 expect_var 0x408D 3
1 expect_var 0x4050 1
1 expect_object 1 8 2
1 shot p1-bedroom-after
1 goto 1 3
1 goto 8 3
1 press UP 4
1 wait 10
1 press A 2
1 wait 30
1 shot p1-talks-to-may
1 mash_field 600
1 expect_var 0x408D 3
1 goto 1 2
1 press UP 30
1 wait 60
1 until_field 600
1 expect_map 1 2
