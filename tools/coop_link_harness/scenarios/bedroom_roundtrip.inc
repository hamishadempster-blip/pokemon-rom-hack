# Both players are in May's bedroom. The rival must be away, the stairs
# must take each player down, and the stairs below must bring them back.
b expect_no_object 1
b goto 1 2
b press UP 30
b wait 60
b until_field 900
b expect_map 1 2
b goto 2 3
b press UP 30
b wait 60
b until_field 900
b expect_map 1 3
b expect_no_object 1
b goto 1 2
b press UP 30
b wait 60
b until_field 900
b expect_map 1 2
b goto 2 3
b press UP 30
b wait 60
b until_field 900
b expect_map 1 3
