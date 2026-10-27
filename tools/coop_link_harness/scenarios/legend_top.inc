# Shared-legend fixtures: the journey's post-clear saves (j68: Sky Pillar
# state 3, Rayquaza waiting at the top in both worlds), linked, warped to the
# Sky Pillar top: Kai (player 0) at (14,8) below Rayquaza, Rosa (player 1) at
# (13,8). No wild encounters; moves: the first one (smart_moves aims badly on the link master in this
# one-foe double: its move menu then never confirms; a harness limit, not the
# game: plain A presses work on both screens, see diag_master_move).
include continue.inc
include relink.inc
b expect coop.sLinkState 2
b smart_moves 0
b no_wild 1
b expect_legend claimed 0
b expect_legend pending 0
