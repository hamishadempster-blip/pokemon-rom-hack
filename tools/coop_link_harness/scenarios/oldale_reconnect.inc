# Start from lab saves made by save_lab.txt.
include beacon_battle.txt
b mash_field 2000 B
b write sWildEncountersDisabled 1
b fixture_flag 0x74 1
b fixture_var 0x4051 1
b press DOWN 200
b until_field 2000
b dump left-lab
b goto 11 1 1500
b press UP 60
b until_field 2000
b dump route101-again
b goto 7 17 2000
b goto 7 10 2000
b goto 15 10 2000
b goto 15 5 2000
b goto 9 5 2000
b goto 9 0 2000
b press UP 70
b until_field 2000
b dump oldale
