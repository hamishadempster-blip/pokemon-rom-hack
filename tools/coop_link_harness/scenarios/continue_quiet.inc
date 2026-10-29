# CONTINUE without pressing A in the field: continue.inc's A mash can reach
# the field and talk to whoever the save faces (quest_stone_catchup: Marlin
# handed over the stone before the test began).
b wait 1100
b press START 2
b wait 300
b press A 2
b wait 250
b press A 2
b until_cb2 CB2_ContinueSavedGame 4000 A
b mash_field 3000 B
b until_field 5000
