# Player 1 flies: party slot 5 knows FLY (party_action picks it from the
# submenu by its action id, 33); the destination is the including
# scenario's fly_map line. Field control first.
1 mash_field 3000 B
1 press START 2
1 wait 40
1 menu_to POKEMON
1 press A 2
1 until_cb2 CB2_UpdatePartyMenu 600
1 wait 30
1 party_action 5 33
1 until_cb2 CB2_FlyMap 900
1 wait 60
