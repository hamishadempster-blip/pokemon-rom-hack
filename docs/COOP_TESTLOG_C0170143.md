# Test log: build 0xC0170143 (bug hunt after 0xC0170142)

- **ROM:** `emerald-coop-C0170143.gba`, built from `f0c79a77`.
  - SHA-1 `9ec429318df5ba97527b3482154113d76a54cde7`
  - SHA-256 `8d11afa08fac94d3cfb5936de5c19058ef64736b6cf8fc9b3144205ebd4ab7fb`
- **Build id:** 0xC0170143. Battle behaviour changed in this build, so it links only with 0xC0170143.
- **Save layout:** unchanged. Saves from 0xC0170142 and older load as before; compatibility was checked on 11 save pairs, **33/33 PASS**.
- **Harness:** the two-core mGBA 0.10.3 harness from `tools/coop_link_harness`, built against this ELF.

Harness results are not evidence for the mGBA GUI, real hardware or two-PC netplay.

## How the bugs were looked for

1. **The players' latest report.** The D1500003 notes (another branch, never pushed to the remote) describe two bugs from the Rustboro Gym. Both were reproduced on 0xC0170142 and fixed here.
2. **Random input.** 22 long sessions of random button presses for both players, linked: lab saves and checkpoints j16, j52 and j68, with 24,000–30,000 frames per player. They looked for crashes, asserts, resets and softlocks. The generator is `tools/coop_link_harness/matrix/genmonkey.py`; it is not part of the suite.
3. **Code review.** An independent read of co-op battles against one NPC trainer and against an NPC pair, looking for the same class of bug as the HP box fix: stock code that reads the MULTI flag as "two trainers per side, three Pokémon each".

## Fixed (each reproduced on the 0xC0170142 ROM first, then passing on this build)

| Bug | What the players saw | Cause | Fix | Regression |
| --- | --- | --- | --- | --- |
| **Crash in gym battles (Roxanne and others)** | An assert screen, `random.c: sum <= 0xFFFF`, when the trainer AI looked at a Pokémon whose possible abilities include Klutz, Truant, Normalize and similar (Buneary, Lopunny, Slakoth…) | The AI's ability guess used signed ratings (-1, -2) as unsigned weights | Weights are u16, and each valid ability weighs at least 1 | `ai_ability_klutz_coop`: asserted on 0xC0170142, passes now |
| **Second foe's HP box missing** | Against one trainer together (Rustboro Gym's Josh), only the first foe had an HP box, on both screens | The battle is flagged MULTI, so the opponent intro code skipped the partner's box, shiny check and ball sprite | `OpponentIntroShowsPartner` / `LinkOpponentIntroShowsPartner` | `coop_one_trainer_hpbox` (sprite flags 05 → 01). `coop_one_trainer_replace`: boxes after replacements were already right |
| **No EXP from the second trainer of a pair** | In the Roxanne and Heath fight, knocking out Heath's first Pokémon gave nobody EXP. The 0xC0170142 suite log shows "The opposing Geodude fainted!" with no EXP line | The paid-EXP record was indexed by trainer id & 2. Heath is trainer 3, so the index was 2, past the 2-byte array | Indexed opponent A/B | `coop_pair_exp` |
| **ROAR failing** | ROAR and WHIRLWIND (also Dragon Tail and Red Card) failed against a lone trainer who still had healthy Pokémon | Co-op battles used the link multi rule: the foe's slots 4–6 never came in, and a slot of another party was excluded | Co-op uses the NPC's whole party, and excludes the partner's slot only when the two share a party | `coop_roar_one_trainer` ("But it failed!" before, "dragged out" now) |
| **AI reserve count** (found by code review only) | Trainer AI choices that depend on reserves (Explosion, Memento, "trapped") misjudged co-op players and NPC pairs | The partner's slot was excluded from a different party | Excluded only when the party is shared | Source-checked; covered by the battle scenarios in the suite |
| **Revival Blessing** (found by code review only) | Could re-send the wrong battler when the slot numbers matched | Same class: no shared-party check | Shared-party check | Source-checked only |
| **CONTINUE screen** | "Build C0170142" ran into long locations (RUSTURF TUNNEL, LITTLEROOT TOWN) | Fixed positions | When needed, the build drops "Build " and the location takes a narrower font | `continue_screen_names`: shots checked by eye on four save sets |

## Found but not fixed

- **A heap assert seen once in random-input play.** `malloc.c:98`, `block->allocated == TRUE`, in `FreeBattleResources` at the end of a solo wild battle while linked (`monkey_j16_1`, j16 saves).
  - Earlier in the same session, the player had caught a Pokémon with a nickname, typed a random name, and answered NO to "Add to your party?".
  - Not reproduced: the same seed again (the two-core run is not exactly repeatable), and three targeted scenarios (nickname plus PC, linked and unlinked, full and non-full party). Log kept in `/tmp/claude-0/monkey/out1/monkey_j16_1.log`. Open.
- **From the code review, not fixed:**
  - The AI's knowledge of the human players' moves and abilities is stored by side but written by trainer, so the right-hand player's records alias the host's.
  - U-turn and Volt Switch do not switch out when the target trainer's own party is empty but its partner trainer's is not.
  - A critical-hit evolution counter (Sirfetch'd) is credited to the wrong party in co-op.
  - Co-op NPC trainers never use bag items (stock rule for link battles; may be intended).
- **Driver limits seen in random play, not game faults:**
  - a naming screen at the end, which B cannot leave;
  - a message that arrived after the final mash;
  - one harness segfault when its scenario file had been deleted.

## Final suite (0xC0170143)

`run_all.sh` with `LONG=1`, `SEG6`, `J06 J13 J16 J29 J37 J43 J44 J46 J48 J50 J51 J52 J57 J58 J59 J65 J66 J68` and `OLD13C_COAST`. ROM 9ec429318df5 (`f0c79a77`); the harness and scenarios are from a snapshot of `f0c79a77`. Output: `/home/claude/journey/final143/suite`.

The session restarted while the suite was running; 315 runs had finished. The rest was run by the same script with `RESUME=1`, which skips runs already in the summary. That option is in the snapshot copy only.

**357 scenarios: 355 PASS and 2 FAIL on the first run; 0 visual-check flags.**

| First-run failure | Cause | What followed |
| --- | --- | --- |
| `wild_places` | **Luck:** the wild ABRA used TELEPORT and the battle ended without a win (outcome 10). Same cause as its first-run failure on 0xC0170142 | Reruns on the same ROM: 2/2 PASS |
| `roster_repair` | The known harness `hunt` regression from 0xC0170140: the global frame limit is reached at the same commands as before (pc 47 and 23) | **Not fixed** (harness driver, not the game) |

All six new scenarios passed on the first run: `ai_ability_klutz_coop`, `coop_one_trainer_hpbox`, `coop_one_trainer_replace`, `coop_roar_one_trainer`, `coop_pair_exp` and `continue_screen_names`. The 12 scenarios given the FIELD NOTES fixture after 0xC0170142 also passed, as did `new_species_evo`, which failed once in the stopped earlier start.

Nothing in the game code or the scenarios changed after the suite. This is not one clean run: the first-run failures above are the record.

An earlier start on ROM 3193d223313c (before the pair-EXP fix) was stopped after 28 runs. Its output is in `final143_stale`; there, `new_species_evo` failed once on the host/guest party-HP cross-check (20/20 against 42/42), and it passed in the final suite.

## Not covered

- Everything listed as not covered for 0xC0170142, unless fixed above.
- **Not tested at all:** the mGBA GUI by a person, real hardware, two-PC netplay.
