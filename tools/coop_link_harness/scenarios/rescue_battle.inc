# The Poochyena double after either player opened the bag.
b wait 500
b shot m1
b dump battle1
b press A 2
b wait 60
b shot m2
b press A 2
b wait 200
b shot m3
b dump battle2
0 until_mem gBattlerControllerFuncs:4 HandleInputChooseAction 12000
1 until_mem gBattlerControllerFuncs+8:4 HandleInputChooseAction 12000
b expect_battle roster
