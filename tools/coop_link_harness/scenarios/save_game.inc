# First save on a fresh file: SAVE, yes, and the confirmation.
b press START 2
b wait 60
b menu_to SAVE
b press A 2
b wait 50
b press A 2
b wait 250
b press A 2
b wait 40
b press B 2
b until_mem gSaveCounter:4 1 3000
b until_field 3000
