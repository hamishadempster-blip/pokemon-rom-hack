# Open the lead's summary from the start menu (POKéMON, A, SUMMARY).
0 press START 2
0 wait 40
0 menu_to POKEMON
0 press A 2
0 until_cb2 CB2_UpdatePartyMenu 600
0 wait 60
0 press A 2
0 wait 40
0 press A 2
0 until_cb2 CB2_InitSummaryScreen 600
0 wait 150
