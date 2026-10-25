# Save over an existing file from the start menu: SAVE, yes, overwrite yes.
b press START 2
b wait 60
b menu_to SAVE
b press A 2
b wait 50
b press A 2
b wait 120
b press A 2
b wait 300
b press A 2
b wait 40
b mash_field 1500 B
b until_field 3000
b hex gSaveCounter 4
