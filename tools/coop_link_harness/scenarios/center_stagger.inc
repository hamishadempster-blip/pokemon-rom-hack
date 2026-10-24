# Start in Oldale with Beacon on. One player heals while the other stays out.
b goto 9 17 1500
b goto 6 17 2500
0 press UP 35
0 until_field 2500
b wait 600
b expect coop.sLinkState 2
b expect coop.sBeacon+1:1 1
b expect gWirelessCommType:1 0
b expect_no_partner center-p0-only
0 goto 7 4 1500
0 press UP 2
0 press A 2
0 mash_field 5000
0 until_field 1500
b expect coop.sLinkState 2
b expect coop.sBeacon+1:1 1
b expect gWirelessCommType:1 0
1 press UP 35
1 until_field 2500
b wait 120
b expect_partner center-reunited
1 goto 7 4 1500
1 press UP 2
1 press A 2
1 mash_field 5000
1 until_field 1500
0 goto 6 7 1500
0 press DOWN 35
0 until_field 2500
b wait 600
b expect_no_partner center-p1-only
b expect coop.sLinkState 2
b expect coop.sBeacon+1:1 1
b expect gWirelessCommType:1 0
1 goto 6 7 1500
1 press DOWN 35
1 until_field 2500
b wait 120
b expect_partner center-both-outside
b shot center-both-outside
