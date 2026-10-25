# Player 0 presses A on player 1, picks TRADE, and offers party slot 1;
# player 1 accepts and offers its slot 1 back. Both stop at the final
# "Make this trade?" YES (confirmed, not yet escrowed).
0 press A 2
0 wait 40
0 press DOWN 2
0 press A 2
0 until_cb2 CB2_UpdatePartyMenu 600 A
0 wait 30
0 press DOWN 2
0 press A 2
0 until_mem coop_deal.sDeal 1 600 A
1 until_mem coop_deal.sDeal 2 900
1 until_cb2 CB2_UpdatePartyMenu 900 A
1 wait 30
1 press DOWN 2
1 press A 2
