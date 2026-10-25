# Both players heal at a Pokemon Centre (standard layout: nurse at 7,2;
# entrance 6-7,8), player 1 a few seconds behind; then both step outside.
# Use after both have walked in (until_map on the Centre's 1F).
0 walk 7 4 2000
0 press UP 2
0 wait 10
0 press A 2
0 mash_field 6000
1 wait 240
1 walk 7 4 2000
1 press UP 2
1 wait 10
1 press A 2
1 mash_field 6000
b sync
