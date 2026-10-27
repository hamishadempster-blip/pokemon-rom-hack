# Stability, shared world, first roster batch and revamps: build 0xC0170137

Branch `coop-doorway-scenes-rivals` (continued from 0xC0170136), pushed, not merged. Both players need this build; 0xC0170136 and older refuse to link with it. Older saves load (layout unchanged); party stats are recalculated on CONTINUE (§5). Test log: [COOP_TESTLOG_C0170137.md](COOP_TESTLOG_C0170137.md).

- ROM `emerald-coop-C0170137.gba` from source `c880f0cb`: SHA-1 `c1971178c7aee0beefc253fa171bddc10fe382bc`, SHA-256 `162b51c75ae75cf051575b2eb4af1fc9922d6ba8bf231d0a588cf242782a00c7`.
- Commits: stability `38ab7398`, synchronization `65549f03`, transitions `abc6a576`, roster `45493603`, balancing `99a97e65`, build id `c880f0cb`, docs after it.
- Final suite on this ROM: 158 scenarios, 157 pass; 1 failure was a test-setup loss (a crit on a Lv 5 starter), fixed in the scenario and passed twice on rerun. Earlier: 0xC0170135 117/118, 0xC0170136 131/134 (all three passed on rerun).

## 1. Stability

- **Finding #16 (invalid opponent): unresolved.** A 25-run matrix of the recorded sequence and its variations (both initiators, crossing into Karen's lane, declined invite, three timings) produced no assert. Trainer battles now refuse to start with an opponent that cannot be built (`TrainerBattleParamsValid`): the approach ends, the player is released, and `gCoopDiag` records the site, ids, approach count, co-op state and script. No placeholder opponent. Received invites naming no valid trainer are refused and recorded. The `<= count` loop in `CheckForTrainersWantingBattle` was fixed; it was not the cause.
- **`pp_coop`:** a timing matrix (both roles, delayed choices, replacements, simultaneous faints, heartbeat loss, relink) found no hang or divergence, so no wait-recovery code was changed. The 0xC0170136 bisect still stands: the old hang was battle-TV scoring, fixed by `2004e0f6`.
- **Test driver:** one-frame callbacks are visible (cb2 history); asserts and unexpected resets fail and stop a run; a failed run's saves are quarantined and never promoted; recovery tests are tagged `KIND: recovery`.

## 2. Shared world

- **NPC movement** (`src/coop_npc.c`): on a shared map one game is the movement authority (first to arrive; ties and fresh links go to link player 0). Its random walkers, look-around, face/rotate, back-and-forth and walk-sequence NPCs send each step or turn by stable local id; the other game replays them, walks a missed move back into line (≤2 tiles) or places it, and waits while a player stands in the way or a script runs. Leaving, link loss, or an NPC only one save has: each game runs its own. Uses the heartbeat slot at most every other frame (no block traffic). **Not synced:** trainers, followers, copy-the-player and in-place movers.
- **Busy NPCs:** talking reserves the NPC; the partner gets "They're talking with X right now."
- **Scene join:** Meteor Falls' trigger-tile scene takes a nearby, free partner who still has it ahead to the tile; a partner in a menu is not pulled in (the offer lapses after 10 s). Other story actors (Birch's rescue, Mira/Soren, Wally, rewards) keep their earlier co-op handling; not extended this build.
- **Phantom sprites:** the partner is dropped when they go through a door (checked in `npc_town_order`).

## 3. Transitions and battle items

- Co-op battles against the game (wild and trainer; not practice copies, the Twin Trial or friendly battles) allow each player's **own bag**: healing, status, Revive, PP, X items, Dire Hit, Guard Spec; balls on wild Pokémon only. Only the player's own Pokémon can be targets. Effects on the joining player's benched Pokémon reach that player's save. PvP rules are unchanged.
- Requests set aside during a battle are answered "busy" afterwards, never treated as current; unknown invite kinds are dropped (`gCoopStaleInvites`).
- Only UNLINK ends a session; `transitions_tour` covers doors, nurse, menus, save, Mach Bike, FLY, solo and co-op battles, UNLINK and a new session with a complete battle.

## 4. Roster batch 1 (114 species)

- **Counts:** 0 of 114 obtainable before (audit of the pre-batch source); 114 after. Newly obtainable 114, already obtainable 0, access repaired 0. 33 first stages in wild tables, 12 starters as partner Pokémon, 69 by evolution.
- **Habitats:** 41 encounter slots reuse duplicate slots (table sizes and existing species kept) across grass, caves, the desert, Surf and the Super Rod, from Route 102 to Route 126. Hoenn Pokédex order extended; species IDs unchanged.
- **Partner Pokémon:** a guest at the left of the Centre counter offers one of three starters per day per player: Kanto (Rustboro, 1 badge, Lv 10), Sinnoh (Lavaridge, 4, Lv 18), Unova (Fortree, 6, Lv 25), Kalos (Mossdeep, 7, Lv 30). A full party sends it to the PC.
- **Evolutions:** all supplies are sold (Fire, Thunder, Shiny, Dusk Stone, Linking Cord, Razor Claw); trade evolutions also take a Linking Cord; location (New Mauville), time (night) and move conditions (Stomp, Ancient Power) were played. The field researcher in four Centres gives habitat and evolution hints.
- Manifest: [COOP_ROSTER_BATCH1.md](COOP_ROSTER_BATCH1.md) (generated by `tools/coop_roster/manifest.py`). No legendaries, mythicals, Ultra Beasts, Paradox, custom Megas or regional forms added; Mega Milotic still deferred.

## 5. Twelve revamps

Butterfree, Beedrill, Arbok (Poison/Dark), Parasect, Hypno, Furret, Lanturn, Kricketune, Cherrim (Grass/Fairy), Lumineon, Maractus, Spidops take the proposed stats unchanged. Before/after stats, types, abilities and learnset changes, the showcase trainers and nine realistic-level battles: [COOP_REVAMPS_C0170137.md](COOP_REVAMPS_C0170137.md). Earlier revamps unchanged.

- Butterfree lost Sleep Powder (Compound Eyes made it near-certain). Support moves were spread, not given to all: Heal Pulse ×2, Pollen Puff ×2, Encore ×1, Tailwind ×2.
- **Owned Pokémon:** party stats are recalculated on CONTINUE (IVs, EVs, nature, moves, ability slot untouched; a fainted Pokémon stays fainted); boxed ones on withdrawal. Ability changes follow the slot an individual already has (listed in the revamp doc).
- Telepathy blocks an ally's Pollen Puff heal (official rule); Hypno's first ability is Insomnia.
- Eleven route trainers field them at route level; no party above three.

## 6. Known gaps

- #16 unresolved (above).
- Not exercised in play: same-frame talk by both players to one NPC; Storm Drain, Water Absorb, Poison Touch, Stakeout, Technician; Roland and Tanya (they stand in water); the Sinnoh/Unova/Kalos partner Pokémon given in play (gate and source only); every species caught in play (placements are source-checked).
- The realistic battles are single samples driven by "strongest damaging move", not human play.
- Two visual-check flags (blank name of the absent battler's hidden healthbox after the Bag) were checked on screen: not visible.
- mGBA GUI, real hardware and netplay not tested.

## Where things are (work environment, not committed)

- Checkpoints `/home/claude/journey/jNN` (p0 Kai, p1 Rosa), story_seg6 `/tmp/claude-0/seg6`.
- Final suite output `/home/claude/journey/final137/suite`; clips `/home/claude/journey/clips`.
- Full suite: `OUT=dir HARNESS_OUT=dir LONG=1 SEG6=… J16=… J37=… J43=… J48=… J52=… J65=… J68=… tools/coop_link_harness/run_all.sh`.
