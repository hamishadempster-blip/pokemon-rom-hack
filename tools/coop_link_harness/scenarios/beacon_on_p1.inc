# Linked, in the field: player 1 uses the Twin Beacon from Key Items, player 0 accepts.
1 press START 2
1 wait 40
1 menu_to BAG
1 press A 2
1 until_cb2 CB2_BagMenuRun 2500
1 wait 80
1 hex gBagPosition 8
1 press RIGHT 4
1 wait 30
1 press RIGHT 4
1 wait 30
1 press RIGHT 4
1 wait 30
1 press RIGHT 4
1 wait 30
1 hex gBagPosition 8
1 expect gBagPosition+5:1 4
1 press A 4
1 wait 40
1 shot context
1 press A 4
1 wait 40
1 shot afteruse
1 hex coop.sBeacon 6
1 hex gLinkCallback 4
0 until_mem sGlobalScriptContextStatus:1 1 6000
0 wait 90
0 press A 4
0 wait 50
0 press A 4
0 wait 50
0 press A 4
0 wait 80
0 hex coop.sBeacon 6
1 wait 40
1 press B 4
1 wait 20
1 press B 4
b wait 1500
b hex coop.sBeacon 6
b expect coop.sBeacon+1:1 1
