# Player 0 teaches HM02 FLY (item 683) from the Bag to party slot 5,
# forgetting its first move. In the field, menus closed. State-based:
# each step waits for the screen it needs.
0 mash_field 3000 B
0 press START 2
0 wait 40
0 menu_to BAG
0 press A 2
0 until_cb2 CB2_BagMenuRun 2500
0 wait 80
0 bag_item 683
0 shot teach-pocket
# USE; "Booted up an HM." "It contained ..." "Teach ...?" YES: to the party.
0 until_mem gMain+4:4 CB2_UpdatePartyMenu 900 A
0 wait 40
0 until_mem gPartyMenu+9:1 5 600 DOWN
0 wait 20
0 shot teach-party
# The mon; "wants to learn"; "already knows four moves"; replace? YES;
# "Which move should be forgotten?": the summary's move list.
0 press A 2
0 until_nz sMonSummaryScreen:4 A
0 wait 90
0 shot teach-forget
# The first move (the cursor starts there); "1, 2, and... Poof!": back in
# the Bag; out to the field.
0 press A 2
0 wait 60
0 until_mem gMain+4:4 CB2_BagMenuRun 1500 A
0 wait 30
0 mash_field 3000 B
