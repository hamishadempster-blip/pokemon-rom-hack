# Player 0 teaches HM07 WATERFALL (item 688) from the Bag to party slot 0 (WHISCASH),
# forgetting its fourth move (ZEN HEADBUTT; DIVE and ROCK SMASH are HM moves, which
# cannot be forgotten). In the field, menus closed. State-based:
# each step waits for the screen it needs.
0 mash_field 3000 B
0 press START 2
0 wait 40
0 menu_to BAG
0 press A 2
0 until_cb2 CB2_BagMenuRun 2500
0 wait 80
0 bag_item 688
0 shot teach-wf-pocket
# USE; "Booted up an HM." "It contained ..." "Teach ...?" YES: to the party.
0 until_mem gMain+4:4 CB2_UpdatePartyMenu 900 A
0 wait 40
0 until_mem gPartyMenu+9:1 0 600 DOWN
0 wait 20
0 shot teach-wf-party
# The mon; "wants to learn"; "already knows four moves"; replace? YES;
# "Which move should be forgotten?": the summary's move list.
0 press A 2
0 until_nz sMonSummaryScreen:4 A
0 wait 90
0 shot teach-wf-forget
# The move to forget (the cursor starts on the first; DOWN 3 times);
# "1, 2, and... Poof!": back in the Bag; out to the field.
0 press DOWN 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press DOWN 2
0 wait 20
0 press A 2
0 wait 60
0 until_mem gMain+4:4 CB2_BagMenuRun 1500 A
0 wait 30
0 mash_field 3000 B
