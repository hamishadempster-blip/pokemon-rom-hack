# One training cycle from a heal in the Rustboro Centre: out, south to
# Route 104's north grass, five wild battles each (stop early below 40% HP
# on the lead), back to the Centre (Rosa first this time), heal.
b walk 7 8 2000
b cross DOWN 0 3 2000
b walk 16 59 4000
b cross DOWN 0 19 3000
0 walk 4 7 8000
1 walk 6 7 8000
0 grind 5 40
1 grind 5 40
b sync
b walk 16 0 8000
b cross UP 0 3 3000
1 walk 16 38 4000
1 until_map 11 5 3000
0 wait 200
0 walk 16 38 4000
0 until_map 11 5 3000
include journey_heal_center.inc
b party
