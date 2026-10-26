# One training cycle from a heal in the Slateport Centre: out, north to
# Route 110's south grass, eight wild battles each (stop early below 40%
# HP; switch_lead, when set, opens each battle with that switch), back to
# the Centre, heal.
b walk 7 8 2000
b cross DOWN 0 1 2000
b walk 17 0 8000
b cross UP 0 25 3000
0 walk 11 94 6000
1 walk 13 95 6000
0 grind 8 40
1 grind 8 40
b sync
b walk 17 99 8000
b cross DOWN 0 1 3000
0 walk 19 19 6000
0 until_map 9 11 3000
1 wait 200
1 walk 19 19 6000
1 until_map 9 11 3000
include journey_heal_center.inc
b party
