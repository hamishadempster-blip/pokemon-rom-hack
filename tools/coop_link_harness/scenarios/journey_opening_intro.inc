# Opening for two new games: OPTION (text FAST), NEW GAME, real Birch intro
# (no test start), names Kai (boy) / Rosa (girl).
include journey_opening_menu.inc
b until_task Task_NewGameBirchSpeech_ChooseGender 6000 A
b wait 30
1 press DOWN 2
b wait 20
b press A 2
b until_cb2 CB2_NamingScreen 3000 A
b wait 60
0 press DOWN 2
0 wait 16
0 press RIGHT 2
0 wait 16
0 press RIGHT 2
0 wait 16
0 press RIGHT 2
0 wait 16
0 press RIGHT 2
0 wait 16
0 press A 2
0 wait 48
0 press UP 2
0 wait 16
0 press LEFT 2
0 wait 16
0 press LEFT 2
0 wait 16
0 press LEFT 2
0 wait 16
0 press LEFT 2
0 wait 16
0 press A 2
0 wait 48
0 press DOWN 2
0 wait 16
0 press RIGHT 2
0 wait 16
0 press RIGHT 2
0 wait 16
0 press A 2
0 wait 48
1 press DOWN 2
1 wait 16
1 press DOWN 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press A 2
1 wait 48
1 press LEFT 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press A 2
1 wait 48
1 press RIGHT 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press RIGHT 2
1 wait 16
1 press A 2
1 wait 48
1 press UP 2
1 wait 16
1 press UP 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press LEFT 2
1 wait 16
1 press A 2
1 wait 48
b sync
b shot names
b press START 2
b wait 30
b press A 2
b wait 200
b mash_field 15000
b shot truck
# Out of the truck: Mum meets each player, then into the house.
0 cross RIGHT 1 0 6000
1 cross RIGHT 1 2 6000
b mash_field 6000
# Upstairs to set the clock (Mum asks).
0 walk 8 2 2000
1 walk 2 2 2000
0 until_map 1 1 2000
1 until_map 1 3 2000
b mash_field 2000
0 walk 5 2 2000
1 walk 3 2 2000
b press UP 4
b wait 20
b press A 2
b until_cb2 CB2_WallClock 900 A
b wait 90
b press A 2
b wait 90
b press UP 2
b wait 20
b press A 2
b wait 60
b mash_field 3000
b shot afterclock
