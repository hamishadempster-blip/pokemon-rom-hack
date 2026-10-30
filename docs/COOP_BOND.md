# Trainer Bond

One **Partner Pokémon** per player, chosen at the Bond Mentor. As its Bond XP grows it reaches ranks that add a flat bonus to each of its calculated stats. Code: `src/coop_bond.c`, `include/coop_bond.h`, `data/scripts/coop_bond.inc`; storage: [COOP_STORAGE_PLAN.md](COOP_STORAGE_PLAN.md).

## For players

- **Where:** the BOND MENTOR, an old woman in Rustboro's Pokémon Center. She checks for the STONE BADGE every time you talk to her, so a save that already has it (from any later point in the story) can use her at once. She tells her introduction once.
- **Menu:**
  - **CHOOSE/CHANGE PARTNER:** opens the party. You see a preview (rank, Bond XP, bonus) and confirm. When you switch, she reminds you that the old partner keeps its Bond XP but its bonus pauses. Eggs can't be chosen. A Pokémon that can't be told apart from another of yours (same personality and Trainer ID) can't be chosen either.
  - **CHECK BOND:** the partner's rank, Bond XP, the next rank and the exact XP still needed, and the bonus. If the partner is in the PC, she says it isn't with you.
  - **HOW IT WORKS** and **BACK**.
- **Ranks.** The bonuses replace one another; they don't add up.

  | Rank | Bond XP | Bonus to each stat |
  | --- | ---: | ---: |
  | New Partner | 0 | +0 |
  | Companion | 2,500 | +5 |
  | Trusted Partner | 10,000 | +10 |
  | Ace Partner | 25,000 | +15 |

  The bonus goes on HP, Attack, Defense, Sp. Atk, Sp. Def and Speed, after the ordinary stat (level, nature, IVs, EVs). It is not an EV, so EV training and vitamins keep their full allowance. SHEDINJA's 1 HP stays 1.
- **Seeing it:**
  - In the party menu, a heart after the partner's name.
  - On the summary's INFO page, SELECT goes through the EVO pages to the BOND page. It shows ACTIVE PARTNER or PREVIOUS PARTNER, the rank, Bond XP, the next rank and the XP needed (or MAX RANK), the bonus or why it is off, and a bar showing progress within the current rank.
  - After a battle, a small popup says "Partner: Bond +N", or "Bond: <rank>!" on a rank-up. Walking is never announced.
- **Off here:** Battle Frontier facilities, Battle Tents, practice battles, link battles against a friend, and duo copies. The summary says "Bonus inactive here". No XP is earned there and nothing is lost.

## Bond XP sources (initial tuning; `src/coop_bond.c`)

| Source | Amount | Cap / rule |
| --- | --- | --- |
| Took part in a knockout of a foe (on the field when it fell) | 40 | 4 per battle |
| Damage dealt to foes | 1 per 2% of the foe's max HP | actual damage only (never past the HP the foe had); 60 per battle |
| Won a battle | 60 trainer, 30 wild | only if the partner was sent out |
| Helped a catch | 40 | the partner dealt damage or gave support in that battle |
| Support | 10 each | REFLECT, LIGHT SCREEN, AURORA VEIL, TAILWIND, TRICK ROOM (their success message), healing an injured ally; 3 per battle |
| First major victory | 400 | each badge, each Elite Four member, the title: once per save ever (`firstWins` bits) |
| Walking at the front, healthy (over half HP, no status) | 1 per 32 steps | real steps only, not forced movement |
| A genuine friendship gain (walking, level-up, gym/League battle) | 2 per point | 10 per event |

- **No XP from:** losses, escapes and draws (only knockouts, damage and support already done count); interrupted link battles (nothing at all); ally damage; Frontier, tent, practice or PvP battles; menus; idle time.
- **Where it is counted:** in a co-op battle each game counts its own partner from its own battle controllers (health bar, faint, battle message). XP therefore belongs to the right player on the master and the guest alike, and nothing is counted twice.
- **Pacing (estimate, not yet measured in unassisted play):**
  - A trainer battle the partner fights in is worth roughly 150–250.
  - Companion comes after about a dozen such battles, around the second or third gym.
  - Trusted Partner needs about 50 battles plus badges.
  - Ace Partner needs a long relationship of roughly 120 or more battles.

## Rules and storage

- **Per-Pokémon record.** Bond XP (0–32,767), a "has been a partner" bit, an "active" bit and the rank the stored stats were calculated with live in the Pokémon's own record, in bits pokeemerald-expansion leaves unused (proven zero on existing saves).
- **Every Pokémon keeps its own history.** It travels through party, PC, Day Care, evolution, forms, saving and trades. Nothing is evicted.
- **The save names its partner** by personality and Trainer ID (CoopSave layout 5), never by slot, species or nickname.
- **When the bonus applies.** Only the named Pokémon with the active bit, in this player's party, outside a suppressed context, gets the bonus and earns XP.
  - A partner in the PC keeps its designation but is absent: no XP, no bonus effect.
  - The other player's Pokémon in a co-op battle are trusted by their record's active bit. Both games apply the same rule to the same record, so they agree before launch (tested: each game's copy of the other's lead has the owner's stats).
- **The partner leaves the save** (trade, gift, release): the designation ends.
  - In a partner-menu deal, offering the partner asks first ("… is your Partner POKéMON … Offer it?"), and the commit clears the designation at once.
  - Otherwise, the next field refresh finds that the partner is no longer in the party, the PC, the Day Care or a pending deal's escrow, and clears it.
  - An arriving Pokémon keeps its history but loses any active bit. The new owner must choose it at the mentor.
- **HP never heals.**
  - When the bonus changes (rank-up, switching, withdrawing, entering or leaving a facility), current HP is mapped in proportion and rounded down: `hp × newMax / oldMax`, at least 1 if alive.
  - Full HP stays full and a fainted Pokémon stays fainted. Repeated toggles can only lose HP to rounding.
  - Mega Evolution and form changes recalculate from clean values with the bonus applied once.
  - A copy of the save's own partner outside battle (the PC's moving Pokémon) keeps the bonus it carries, so moving it in the PC loses nothing.
- **Copies that are never partners.** SHEDINJA (it shares NINCADA's personality), eggs, rentals, practice copies and display sprites are never partners. SHEDINJA starts with no Bond XP.

## Tests (harness, 0xC0170141 development builds)

| Scenario | Saves | What it checks |
| --- | --- | --- |
| `bond_mentor` | j16 (a save from before the update, layout 2) | Badge check, intro once, choose the lead, preview, flags and designation, stats unchanged at rank 0 |
| `bond_ranks`, `bond_ranks_reload` | bond_mentor's | Every boundary (2,499/2,500…), exact +5/+10/+15, HP 20/67 → 15/52 → 19/67, fainted stays fainted, switching keeps XP, reload |
| `bond_xp_wild` | bond_mentor's | Solo wild battle: 120 XP (KO 40, damage 50, win 30), popup; the benched Pokémon gains nothing |
| `bond_coop_wild` | lab | Co-op double, both roles: each game's copy of the other's lead matches the owner; each partner's XP from its own part |
| `bond_support`, `bond_coop_support` | lab | Support XP solo (150 = 3 supports + KO + damage + win) and on the co-op guest (Rosa's REFLECT counted on her game only) |
| `bond_major` | lab | STONE BADGE after the gym script: the one-time 400 on top of the battle's XP |
| `bond_trade` | seg6 | Offering the partner asks first; trading it away ends the designation; the receiver keeps its 3,000 XP and history but no bonus until chosen, then +5 |
| `bond_box` | j29 (real PC menus) | Deposit (absent), withdraw (keeps +10 and 10/29 HP), switch while boxed (flag cleared in the PC), withdraw (19 Max HP, 6 HP), choose again (+10, 9 HP: no healing) |
| `bond_tent` | j29 | Walking into the Slateport Battle Tent: bonus off (52/52), out: back (67/67) |
| `bond_practice` | seg6 | Practice battle: bonus off inside, party byte-identical afterwards, no XP |
| `bond_evolve` | j65 | RARE CANDY: NINCADA → NINJASK keeps XP, designation and +5; the SHEDINJA has none |
| `bond_mega` | j57 | Co-op Mega Evolution: the bonus survives the form change (Mega BANETTE Attack 204 = 199 + 5), master battle data agrees, after the battle stats are as before |

**Found and fixed by these tests:**
- PC withdrawal lost 1 HP to rounding on each move (`bond_box`).
- SHEDINJA copied the partner's bond and became a second partner (`bond_evolve`, first run on the ROM before the fix).
- The badge bonus never paid, because the check ran before the gym script set the badge (found reading the code; `bond_major` written after the fix).

## Not done or not yet tested

- **Presentation:**
  - No preferred follower, resting spot or rank-based greetings.
  - Rank-reached times are not recorded.
  - Bond history can be read on any Pokémon's summary (party or PC); there is no separate history list.
- **Support XP:** speed control other than TAILWIND and TRICK ROOM (for example a foe's Speed lowered by ICY WIND, or THUNDER WAVE) is not counted.
- **PC release:** releasing the partner from the PC uses the PC's own confirmation, with no partner-specific warning. The designation ends at the next field refresh.
- **Not tested:**
  - TRANSFORM/IMPOSTER. By design, XP goes only to the record whose personality and Trainer ID match, and the bonus is part of the copied stats, as with any stat.
  - Full PC delivery after a trade.
  - Link-trade (Cable Club) arrivals, which rely on the same arrival rule.
  - Relink mid-battle.
- **Mega in co-op:** the guest's party copy of the partner's Mega keeps pre-Mega party stats during the battle. The battle runs on the master, whose data is correct. This was already the case before the Bond.
- **Balance:** award rates are not yet tuned with representative unassisted play.
