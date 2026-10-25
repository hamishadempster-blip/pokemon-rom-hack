# CO-OP list while linked: BEACON, UNLINK, PARTNER DEX, JOURNAL, BACK. UNLINK, then YES drops the
# link. A is tapped until the link is down, so any text speed works.
b press START 2
b wait 40
b menu_to COOP
b press A 2
b wait 40
b press DOWN 2
b wait 10
b press A 2
b until_mem coop.sLinkState 0 2400 A
b mash_field 900 B
