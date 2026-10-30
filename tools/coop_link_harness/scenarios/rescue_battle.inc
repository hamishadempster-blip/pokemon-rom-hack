# The Poochyena double after either player opened the bag.
# Wait for the action menus by state, not by a frame count: the co-op battle
# intro is shorter than it was (D1400002), and blind A presses timed for the
# old intro land on the menu and choose FIGHT.
b wait 200
b shot m1
b dump battle1
0 until_mem gBattlerControllerFuncs:4 HandleInputChooseAction 12000
1 until_mem gBattlerControllerFuncs+8:4 HandleInputChooseAction 12000
b shot m3
b dump battle2
b expect_battle roster
