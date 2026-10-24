include oldale_reconnect.inc
# Walk back into Route 101: the old build left Zigzagoon here on both saves.
b expect_flag 0x2EE 1
b goto 9 18 2000
b press DOWN 70
b until_field 2000
b expect_no_object 4
b expect_no_object 2
b goto 9 5 2000
b goto 13 5 2000
b expect coop.sLinkState 2
b expect coop.sBeacon+1:1 1
