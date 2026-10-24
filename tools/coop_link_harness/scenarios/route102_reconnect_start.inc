include oldale_reconnect.inc
# The lab fixture marks the opening adventure gate complete above.
b goto 10 10 2000
b goto 1 10 2000
b press LEFT 40
b until_field 2000
b dump route102
b shot route102

b expect coop.sLinkState 2
b expect coop.sBeacon+1:1 1
