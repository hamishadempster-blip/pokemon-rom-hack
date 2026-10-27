# Player 1 gets on the MACH BIKE (Bag, Key Items: USE).
1 mash_field 3000 B
1 press START 2
1 wait 40
1 menu_to BAG
1 press A 2
1 until_cb2 CB2_BagMenuRun 2500
1 wait 80
1 bag_item 707
1 press A 2
1 wait 40
1 shot bike-menu
1 press A 2
1 until_field 3000
1 wait 30
1 expect_range gPlayerAvatar:1 2 2 0x1F
