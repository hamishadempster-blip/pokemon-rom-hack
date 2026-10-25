# Continue the story_seg6 saves (Rustboro, linked play), link, and stand the
# two players side by side between the Pokemon Centre and the Mart: player 0
# at (15,41) facing right, player 1 at (16,41) facing left. Each walks its
# last step toward the other, so it ends facing the partner.
include continue.inc
include relink.inc
b expect coop.sLinkState 2
b no_wild 1
1 walk 17 41 4000
1 walk 16 41 2000
0 walk 14 41 4000
0 walk 15 41 2000
b sync
b wait 30
b expect_partner side-by-side
