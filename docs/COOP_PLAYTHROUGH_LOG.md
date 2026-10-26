# Co-op playthrough log (main journey)

A continuous two-player run of the main story on one ROM build, driven by the
harness (`tools/coop_link_harness`), scenario files `journey_NN_*.txt`. Each
segment CONTINUEs the saves the previous segment wrote. No story flags, badge
grants, warps, stat boosts or forced wins in these segments; the only driver
aids are named in each file (for example `smart_moves`, which keeps the move
cursor on the strongest damaging move, and `grind`, which paces in grass and
fights until a battle count or an HP floor).

Players: player 0 **Kai** (boy), player 1 **Rosa** (girl). Checkpoint saves
live outside the repository (`/home/claude/journey/<segment>/p0.sav, p1.sav`
in the work environment); they are not committed.

## Status

| Segment | Scenario | Build / commit | Result | Furthest point |
| --- | --- | --- | --- | --- |
| 01 | journey_01_opening | 2004e0f6 | PASS | New games (real intro, no test start), truck, clock, link, Route 101 rescue, starters (Kai: Torchic, Rosa: Houndour), lab, both saved |
| 02 | journey_02_route103 | 2004e0f6 | PASS (2nd attempt) | Nine heal/grind cycles on Route 101 (Oldale Centre), Route 103 twins beaten (Lv11/Lv9), Pokedex from Birch, saved. First attempt (no `smart_moves`, no healing) whited out; then the twins won at Lv8/7: driver and balance, not game bugs |
| 03 | journey_03_petalburg | 2004e0f6 | PASS | Route 102 catches (Kai: Poochyena, Rosa: Wurmple), Petalburg Centre, Norman and Wally scene on both saves, saved in the gym. Kai Torchic Lv12, Rosa Houndour Lv10 |
| 04 | journey_04_woods | 2004e0f6 | PASS | Beacon ON (Twin Beacon), Route 104, Petalburg Woods: Kai triggers the Devon scene, Rosa joins the grunt fight from the invite; both get the catch-up (GREAT BALL, woods state 1 on both saves); Rustboro; saved |
| 05 | journey_05_rustboro_catch | 2004e0f6 ROM, e174463c harness | PASS | Heal (Rosa leads), ten POKe BALLS each at the Mart, Route 104 hunts: Kai BUIZEL, Rosa MARILL (other species run from); healed; saved in the Rustboro Centre |
| 06 | journey_06_rustboro_train | 0f33fe41 | PASS (3rd attempt) | Six training cycles on Route 116 (switch-training: the new catch leads, the starter fights). Kai Buizel Lv12, Rosa Marill Lv13; Torchic evolved into Combusken (Lv16, learned Double Kick); move-learning question answered (weakest move forgotten). Saved in the Rustboro Centre. 1st attempt: LINK UP failed (finding #2, fixed). 2nd: one battle lost to a wild Marill's Rollout chain, whiteout to the Centre (kept as `06a_whiteout.log`) |
| 07 | journey_07_roxanne | 75c9e7b5 / 0f670313 | FAIL (lost, not saved) | Beacon ON; Josh and Tommy+Marc beaten together (co-op); Roxanne + Hiker Heath co-op lost; both whited out to the Rustboro Centre, healed, still linked. Earlier attempts found findings #5-#8 (same-trainer spotting, "link recovering", Beacon desync, host-replacement hang); all fixed, final attempt ran clean |
| 08 | journey_08_shroomish | 0f670313 | PASS | Split up (Rosa to the Woods first, Kai via the Mart), both caught SHROOMISH in Petalburg Woods, back to Rustboro, eight Route 116 training cycles. Kai: Shroomish 14, Combusken 16, Buizel 15 (Water Gun); Rosa: Shroomish 15 (Mega Drain), Houndour 14, Marill 16. Saved |
| 09 | journey_09_roxanne | 0f670313 + harness fixes | 1st attempt lost (not saved); rerun in progress | Party reorder for co-op (slots 1-3), Beacon ON from Rosa's side, gym trainers co-op, heal trip, Kai talks to Roxanne. The loss exposed a driver bug: smart_moves read the type chart with the wrong stride (entries are 4 bytes), so Buizel and Shroomish used Tackle instead of Water Gun / Mega Drain |

## Findings

| # | Where | What | Kind | Status |
| --- | --- | --- | --- | --- |
| 1 | pp_coop (VOLT+FUSE), any link battle | A special move hitting through Light Screen froze the scoring screen (NULL trap in battle TV) | Game bug | Fixed in 2004e0f6 |
| 2 | Journey 06 start: saves continued in the Rustboro Centre, both unlinked | LINK UP never connected ("Cable players: 0"); also after walking outside. The Centre's wireless Union Room scan, run while unlinked, left the RFU serial/timer handlers installed and the comm type wireless; the cable handshake never got a serial interrupt. Any unlinked Centre visit before linking hit it | Game bug | Fixed in 0f33fe41 (`Coop_BeginLink` ends the scan, restores the cable handlers). New regression `centre_link` (unlinked heal then link outdoors; unlink and relink inside a Centre): PASS. Before: `p06e` probe, state stays CONNECTING with `gIntrTable` pointing at the RFU handlers |
| 3 | Rustboro Mart | Escape Rope listed at 0 | Intended: `I_KEY_ESCAPE_ROPE` (Gen 8+) makes it a free, reusable key item | No change |
| 5 | Journey 07, Rustboro Gym, both players walking up together | Youngster Josh spotted both players at once. Each game ran his approach and intro, asked the other, heard "busy", and both fought him alone ("Solo: your partner is busy."). Same for any trainer whose sight line covers both players | Game bug | Fixed (uncommitted at time of writing): an invite for a trainer whose approach/intro is playing on this side is held and joined when this side reaches the battle; if both games already asked, link player 0's invite wins and the other joins. Regressions `same_trainer_spot` (same frame) and `same_trainer_spot_late` (player 1 reads its intro 400 frames longer): PASS. Before: `07a_samespot_before.log` |
| 6 | Journey 07, a player in a long solo battle | The other player's screen said "Solo: the link is recovering." and the partner sprite vanished: a solo battle sent no co-op frames, so the 240-frame quiet limit ran out | Misleading UI | Fixed (uncommitted): solo battles keep sending the position frame, marked "in battle"; the partner reads "Solo: your partner is busy." and no invite waits on a player who is fighting |
| 7 | Twin Beacon handshake (seen in `same_trainer_spot`, run under load) | Player 0 ended with the Beacon ON and player 1 OFF, player 1 stuck "confirming": the offerer applied on the echo and stopped sending YES after a frame or two, which player 1 missed while busy | Game bug (desync) | Fixed (uncommitted): a side still confirming applies once the partner no longer offers or answers (only an applied partner does that). Unit test added in `tools/coop_protocol_test.c` |
| 8 | Journey 07, Roxanne co-op (three attempts) | After Kai (the link host) had sent in two replacements, his party held three copies of his last Pokemon and a full-HP copy of Poochyena in slot 5 (Buizel gone). When his last Pokemon fainted the game still asked him for a replacement (the stray Poochyena), his party menu offered nothing usable, and the battle could not continue (both games waiting; with recording on they also crawled at a few frames per second). Cause: the switch-in order update used the link-multi split for one shared six-slot party; co-op players have separate parties, so half of the host's order went into the partner battler's and party ids were duplicated; the next party menu's reorder then copied one Pokemon over another | Game bug (battle hang, party corruption) | Fixed (uncommitted): in co-op a chosen replacement sets only that player's whole order; Roar-style forced switches no longer touch the partner's order. Diagnostic `coop_host_two_replacements` (FIXTURE: low HP): before, party `[Combusken, Poochyena 0 HP, Combusken]` and order `211345` after the second replacement; after, party intact and order `201345`. `coop_host_team_down`: party species intact after the fight. Before logs: `07d`-`07f` |
| 9 | Driver (smart_moves), segments 07-09 | Type effectiveness read with a 2-byte stride from a 4-byte table: most matchups scored 0, so the driver often picked Tackle over super-effective moves | Test-driver problem | Fixed in the harness (stride from the build); doubles targeting now aims at the foe the move hurts most |
| 4 | All journey saves | Starters are nicknamed "Aaaaaaaaaaaa": the driver's A presses at the starter's nickname question opened the naming screen | Test-driver artifact | Not a game bug; left as is in this run |

## Next action

See the bottom of the latest segment row.
