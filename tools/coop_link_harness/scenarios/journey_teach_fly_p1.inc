# Player 1 teaches HM02 FLY (item 683) from the Bag to party slot 5,
# forgetting its first move. In the field, menus closed. State-based:
# each step waits for the screen it needs.
1 mash_field 3000 B
1 press START 2
1 wait 40
1 menu_to BAG
1 press A 2
1 until_cb2 CB2_BagMenuRun 2500
1 wait 80
1 bag_item 683
1 shot teach-pocket
# USE; "Booted up an HM." "It contained ..." "Teach ...?" YES: to the party.
1 until_mem gMain+4:4 CB2_UpdatePartyMenu 900 A
1 wait 40
1 until_mem gPartyMenu+9:1 5 600 DOWN
1 wait 20
1 shot teach-party
# The mon; "wants to learn"; "already knows four moves"; replace? YES;
# "Which move should be forgotten?": the summary's move list.
1 press A 2
1 until_nz sMonSummaryScreen:4 A
1 wait 90
1 shot teach-forget
# The first move (the cursor starts there); "1, 2, and... Poof!": back in
# the Bag; out to the field.
1 press A 2
1 wait 60
1 until_mem gMain+4:4 CB2_BagMenuRun 1500 A
1 wait 30
1 mash_field 3000 B
