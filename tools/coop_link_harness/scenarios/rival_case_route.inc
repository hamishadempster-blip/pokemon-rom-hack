# Route 103 co-op rival fight, shared by rival_case_*.txt: from lab saves
# (after oldale_reconnect.inc, which the caller includes), strong cached stats on
# the leads (flow, not balance), both players walk up to the twins. The
# caller sets the case first and places the talker.

b goto 9 0 2500
b press UP 70
b until_field 2000
0 fixture_lead 255 15 2035
1 fixture_lead 228 15 4218
b write gParties+86:2 100
b write gParties+88:2 100
b write gParties+90:2 100
b write gParties+94:2 100
b goto 10 13 2500
b goto 14 13 2500
b goto 14 7 2500
b goto 12 7 2500
b goto 12 6 2500
b goto 5 6 2500
b goto 5 4 2500
