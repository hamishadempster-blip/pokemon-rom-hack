# (Included.) Player 0 opens CO-OP > WAIT HERE while linked in a Pokemon
# Center: the ninth line (BEACON, UNLINK, PARTNER DEX, JOURNAL, NAME TAG,
# PING, APPEARANCE, SETTINGS, WAIT HERE, BACK); then the time list.
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
0 press DOWN 2
0 wait 8
0 press DOWN 2
0 wait 8
0 press A 2
0 until_task Task_HandleScrollingMultichoiceInput 2000
0 wait 30
