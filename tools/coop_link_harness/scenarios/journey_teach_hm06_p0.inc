# Player 0 teaches HM06 ROCK SMASH from the Bag to the party lead,
# forgetting its first move (MUD SHOT). In the field, menus closed.
0 press START 2
0 wait 40
0 menu_to BAG
0 press A 2
0 until_cb2 CB2_BagMenuRun 2500
0 wait 80
# The TM/HM pocket (2), then the top of its list, then HM06 (row 5).
0 until_mem gBagPosition+5:1 2 600 RIGHT
0 wait 40
0 until_mem gBagPosition+12:2 0 600 UP
0 wait 20
0 until_mem gBagPosition+12:2 5 600 DOWN
0 wait 20
0 shot tm-pocket
# USE; "Booted up an HM." "It contained Rock Smash." "Teach ...?" YES.
0 press A 2
0 wait 40
0 press A 2
0 wait 40
0 press A 2
0 wait 40
0 press A 2
0 wait 40
0 press A 2
0 until_cb2 CB2_UpdatePartyMenu 600
0 wait 60
0 shot teach-party
# The lead; "wants to learn"; "already knows four moves"; replace? YES;
# "Which move should be forgotten?"; the summary's move list: the first
# (MUD SHOT on both leads); "1, 2, and... Poof!"; out of the Bag.
0 press A 2
0 wait 60
0 press A 2
0 wait 60
0 press A 2
0 wait 60
0 press A 2
0 wait 60
0 shot forget-which
0 press A 2
0 wait 90
0 shot forget-list
0 press A 2
0 wait 60
0 mash_field 3000 B
