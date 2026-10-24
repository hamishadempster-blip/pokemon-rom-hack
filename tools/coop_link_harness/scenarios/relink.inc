# CO-OP menu while unlinked: yes to linking, then wait for the session.
# A is tapped through the question at any text speed.
b press START 2
b wait 50
b menu_to COOP
b press A 2
b until_mem coop.sLinkState 2 3600 A
b mash_field 1500
