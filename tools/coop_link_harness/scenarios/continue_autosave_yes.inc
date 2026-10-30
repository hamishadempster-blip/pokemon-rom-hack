# CONTINUE when the newest slot is an autosave (player 0 alone; player 1
# continues normally): answers YES to "This is your AUTOSAVE ... Keep
# playing from here?".
b wait 1100
b press START 2
b wait 300
b press A 2
b wait 250
b press A 2
1 mash_field 4000
1 until_field 5000
0 until_cb2 CB2_Overworld 5000 A
0 until_task Task_HandleYesNoInput 1500
0 wait 20
0 press A 2
0 mash_field 1500 B
0 until_field 3000
