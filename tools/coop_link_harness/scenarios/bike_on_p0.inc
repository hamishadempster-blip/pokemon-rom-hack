# Player 0 gets on the MACH BIKE (Bag, Key Items: USE).
0 mash_field 3000 B
0 press START 2
0 wait 40
0 menu_to BAG
0 press A 2
0 until_cb2 CB2_BagMenuRun 2500
0 wait 80
0 bag_item 707
0 press A 2
0 wait 40
0 shot bike-menu
0 press A 2
0 until_field 3000
0 wait 30
0 expect_range gPlayerAvatar:1 2 2 0x1F
