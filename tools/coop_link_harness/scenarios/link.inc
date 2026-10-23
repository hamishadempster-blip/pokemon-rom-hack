# START, CO-OP, yes, wait for the session.
b press START 2
b wait 30
b press DOWN 2 4
b press A 2
b wait 60
b press A 2
b until_mem coop.sLinkState 2 2000
b mash_field 1500
