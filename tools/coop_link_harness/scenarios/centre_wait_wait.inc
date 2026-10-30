# (Included.) Player 0 opens CO-OP > WAIT HERE unlinked in a Pokemon
# Center: the seventh line (LINK UP, JOURNAL, PARTNER DEX, LEVEL CAP,
# APPEARANCE, SETTINGS, WAIT HERE, BACK).
0 press START 2
0 wait 40
0 menu_to COOP
0 press A 2
0 until_task Task_HandleScrollingMultichoiceInput 2000
0 wait 20
0 press DOWN 2
0 wait 8
0 press DOWN 2
0 wait 8
0 press DOWN 2
0 wait 8
0 press DOWN 2
0 wait 8
0 press DOWN 2
0 wait 8
0 press DOWN 2
0 wait 8
0 press A 2
