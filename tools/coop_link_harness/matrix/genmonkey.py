#!/usr/bin/env python3
# Random-input ("monkey") scenarios for both players: a crash, assert, reset
# and softlock finder (test driver only; not in run_all.sh). Write OUT into
# scenarios/ as monkey_*.txt (gitignored); INCLUDE starts the session (e.g.
# beacon_battle.txt on lab saves). A FAIL at the end is often the driver (a
# naming screen B cannot leave); look at the dump before calling it a bug.
# usage: genmonkey.py SEED FRAMES INCLUDE OUT
import random, sys
seed, frames, inc, out = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3], sys.argv[4]
rng = random.Random(seed)
keys = [('UP',14),('DOWN',14),('LEFT',14),('RIGHT',14),('A',22),('B',14),('START',4),('SELECT',2),('L',1),('R',1)]
pool = [k for k,w in keys for _ in range(w)]
lines = [f'# monkey seed {seed}, {frames} frames per player (random input; test driver only)', f'include {inc}', 'b mash_field 600 B']
for p in (0, 1):
    t = 0
    while t < frames:
        k = rng.choice(pool)
        n = rng.randint(8, 40) if k in ('UP','DOWN','LEFT','RIGHT') else 2
        w = rng.randint(4, 40)
        lines.append(f'{p} press {k} {n}')
        lines.append(f'{p} wait {w}')
        t += n + 6 + w
lines += ['b mash_field 4000 B', 'b until_field 4000', 'b expect_mons_intact', 'b shot end']
open(out, 'w').write('\n'.join(lines) + '\n')
