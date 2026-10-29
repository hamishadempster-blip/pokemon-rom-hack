# The Littleroot exhibition as a co-op duo, shared by exh_case_*.txt:
# story_seg6 saves, Twin Beacon on, seven badges and every twin meeting
# played (FIXTURES: badge flags, VAR_TWIN_ARC), the lead copied to six
# slots with strong stats and 999 HP on three, fixture_warp, no random wild battles. The caller sets the case
# (VAR_STARTER_MON, FIXTURE) after this file, before the talk.
include deal_setup.inc
include beacon_menu_on.inc
b no_wild 1
b smart_moves 1
b strong_battlers 600
b fixture_flag 0x867 1
b fixture_flag 0x868 1
b fixture_flag 0x869 1
b fixture_flag 0x86a 1
b fixture_flag 0x86b 1
b fixture_flag 0x86c 1
b fixture_flag 0x86d 1
b fixture_var 0x409D 0x1BF
b fixture_lead 255 100 1059860
b fixture_six
# 999 HP on the first three copies (flow, not balance).
b write gParties+86:2 999
b write gParties+88:2 999
b write gParties+186:2 999
b write gParties+188:2 999
b write gParties+286:2 999
b write gParties+288:2 999
0 fixture_warp 0 9 10 19
1 fixture_warp 0 9 11 19
b until_field 3000
b sync
1 walk 12 18 1000
0 walk 10 18 1000
b sync
b wait 60
