# One training cycle from a heal in the Rustboro Centre: out, east to
# Route 116's west grass, six wild battles each, switching the lead out for
# party slot 2 at the start of each (the lead still earns full experience);
# stop early when the lead or slot 2 drops below 40% HP;
# then back to the Centre (Kai first), heal.
b walk 7 8 2000
b cross DOWN 0 3 2000
b walk 39 10 5000
b cross RIGHT 0 31 3000
0 walk 7 15 6000
1 walk 10 15 6000
0 grind_sw 6 40 2
1 grind_sw 6 40 2
b sync
b walk 0 10 6000
b cross LEFT 0 3 3000
0 walk 16 38 5000
0 until_map 11 5 3000
1 wait 200
1 walk 16 38 5000
1 until_map 11 5 3000
include journey_heal_center.inc
b party
