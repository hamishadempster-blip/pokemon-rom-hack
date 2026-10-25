# Linked: player 0 turns the Twin Beacon on from the CO-OP menu and player
# 1 agrees (from deal_battle.txt). Player 1 faces away while it taps A.
1 press UP 1
1 wait 20
0 press START 2
0 wait 40
0 menu_to COOP
0 press A 2
0 wait 60
0 press A 2
1 until_mem coop.sBeacon+1:1 1 3000 A
0 mash_field 1200 B
b until_mem coop.sBeacon+1:1 1 1200
b mash_field 1500 B
b wait 30
b expect coop.sBeacon+1:1 1
