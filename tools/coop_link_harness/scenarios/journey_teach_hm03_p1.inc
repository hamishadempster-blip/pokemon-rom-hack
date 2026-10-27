# Player 1 teaches HM03 SURF from the Bag to the party lead,
# forgetting its second move (DIG). In the field, menus closed.
1 press START 2
1 wait 40
1 menu_to BAG
1 press A 2
1 until_cb2 CB2_BagMenuRun 2500
1 wait 80
# The TM/HM pocket (2), then the top of its list (14 UPs: the bag remembers its place;
# its saved cursor fields are not live), then down, then HM03 (row 6).
1 until_mem gBagPosition+5:1 2 600 RIGHT
1 wait 40
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 press UP 2
1 wait 8
1 wait 20
1 press DOWN 2
1 wait 8
1 press DOWN 2
1 wait 8
1 press DOWN 2
1 wait 8
1 press DOWN 2
1 wait 8
1 press DOWN 2
1 wait 8
1 press DOWN 2
1 wait 8
1 wait 20
1 shot tm-pocket
# USE; "Booted up an HM." "It contained Surf." "Teach ...?" YES.
1 press A 2
1 wait 40
1 press A 2
1 wait 40
1 press A 2
1 wait 40
1 press A 2
1 wait 40
1 press A 2
1 until_cb2 CB2_UpdatePartyMenu 600
1 wait 60
1 shot teach-party
# The lead; "wants to learn"; "already knows four moves"; replace? YES;
# "Which move should be forgotten?"; the summary's move list: the first
# (the cursor down one: DIG on both leads); "1, 2, and... Poof!"; out of the Bag.
1 press A 2
1 wait 60
1 press A 2
1 wait 60
1 press A 2
1 wait 60
1 press A 2
1 wait 60
1 shot forget-which
1 press A 2
1 wait 90
1 shot forget-list
1 press DOWN 2
1 wait 20
1 shot forget-cursor
1 press A 2
1 wait 60
1 mash_field 3000 B
