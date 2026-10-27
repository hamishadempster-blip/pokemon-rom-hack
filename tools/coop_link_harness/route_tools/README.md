# Route tools (journey scenario planning)

Offline helpers that read the map data (map.json, layouts, metatile
attributes) and print harness `walk`/`cross` lines. They follow the
harness walk rules: collision, elevation 0/15 transitions, ledges, objects
block, no water (Surf is a separate `surf` command), muddy slopes one way.

- `reach.py Map x,y [y0-y1]` — land reachability grid (FREE="x,y ..." frees object tiles).
- `plan.py Maps(a,b,..) StartMap x,y GoalMap x,y [Map:x,y;...]` — route over
  warps (arrow warps taken by moving their way, step-on warps); last arg frees object tiles.
- `boulders.py Map sx,sy tx,ty` — Strength boulder pushes on one map.
- `gates.py FortreeCity_Gym sx,sy tx,ty` — rotating-gate puzzle with the
  game's gate rules (src/rotating_gate.c) and trainer sight (a trainer who
  sees the player walks up and stays); `mkfg.py` writes journey_51's walks.

They plan; the harness run is the test. Paths are absolute to
/home/claude/pokemon-rom-hack (edit `root` to move them).
