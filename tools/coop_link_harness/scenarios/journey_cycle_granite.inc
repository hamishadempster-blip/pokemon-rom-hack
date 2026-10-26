# One training cycle from a heal in the Dewford Centre: out, Route 106,
# Granite Cave 1F, the ladder to B1F's south-west part, eight wild battles
# each pacing a corridor there (stop early below 40% HP), back the same
# way, heal.
b walk 7 8 2000
b cross DOWN 0 11 2000
b walk 5 0 4000
b cross UP 0 21 3000
b walk 48 17 8000
b cross UP 24 7 3000
b walk 17 11 8000
b until_map 24 8 3000
0 walk 3 17 4000
1 walk 6 14 4000
0 grind 8 40
1 grind 8 40
b sync
b walk 4 21 6000
b until_map 24 7 3000
b walk 37 12 8000
b cross DOWN 0 21 3000
b walk 65 19 8000
b cross DOWN 0 11 3000
0 walk 2 10 4000
0 until_map 3 1 3000
1 wait 200
1 walk 2 10 4000
1 until_map 3 1 3000
include journey_heal_center.inc
b party
