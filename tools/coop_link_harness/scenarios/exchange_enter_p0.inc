# Player 0 into the Trainer Exchange from Rustboro (Rustboro is loaded
# again first: an older save's own view of the town predates the building).
0 fixture_warp 0 3 20 47
0 until_field 3000
0 walk 23 53 4000
0 press UP 20
0 until_field 3000
0 wait 30
0 expect_map 11 17
