# Build 0xD1500003: Rustboro Gym fixes (on D1500002)

Branch `fixes/catch-swap-hm`. Opus's branch and `main` untouched. Save layout
unchanged. Both players need this build.

## 1. Crash: `src/random.c:197: sum <= 0xFFFF` (blue screen)

- Report: in the co-op battle against Roxanne, the game showed the assert screen
  when Buneary, the player's last Pokémon, was sent out ("every time someone sends
  out a 3rd Pokémon").
- Cause: `AI_DecideKnownAbilityForTurn` (`src/battle_ai_util.c`). When the trainer
  AI guesses an opposing Pokémon's ability, it weights each possible ability by
  `gAbilitiesInfo[].aiRating`. That rating is signed: Truant and Slow Start are -2;
  Normalize, Stall, Klutz, Defeatist, Heavy Metal, Zen Mode and Perish Body are -1.
  The weights were stored in an unsigned array, so -1 wrapped to a huge number,
  the sum broke `RandomWeightedArray`'s assert, and the game stopped. Buneary
  (Run Away / Klutz / Limber) hits it whenever an AI that predicts abilities
  (Roxanne's) looks at it. Pre-existing: the same crash happens on Opus's 0xC0170141.
- Fix: the weights are `u16`, and each valid ability weighs at least 1 (a
  negative or zero rating counts as 1).
- Evidence: `diag_gym_roxanne_last` from the players' own saves (Roxanne and
  Heath, co-op; Flopper and Appletun at 0 HP, so Buneary leads): **assert in 3 of
  3 runs on 0xC0170141, none in 3 of 3 on the fix**, plus once more on the final
  D1500003 ROM.

## 2. Missing foe HP box against a single trainer (co-op)

- Report: first Rustboro Gym trainer (Josh, Roggenrola and Dwebble) showed only
  Roggenrola's HP box.
- Cause: a co-op battle against one trainer is flagged MULTI, and
  `TwoMonsAtSendOut` sends both of that trainer's Pokémon from the left
  opponent's controller. The opponent intro code (`battle_controller_opponent.c`
  on the host and `battle_controller_link_opponent.c` on the guest) only showed
  the partner's HP box, tried its shiny animation and freed its ball sprite when
  MULTI was clear. So the second foe got no HP box on either screen, and its ball
  sprite was never freed. Pre-existing on 0xC0170141 and D1400002.
- Fix: `OpponentIntroShowsPartner()` / `LinkOpponentIntroShowsPartner()` also
  cover `IsCoopSingleTrainerBattle()`.
- Evidence: `diag_gym_josh_coop` from the players' saves: both screens show the
  Roggenrola and Dwebble boxes (screenshots checked); before the fix, only
  Roggenrola's box showed, on D1400002, 0xC0170141 and D1500002.

## Tests on the fixed source (lab saves unless stated)

coop_six (one-foe trainer co-op), battle_visual_wild_double, party_exp_wild,
party_exp_solo, move_effectiveness, solo_six, catch_full_party_solo,
catch_full_party_cancel, catch_before_dex, vial_team_heal, hm_no_mon,
hm_party_menu: 12/12 PASS. The protocol test passed. On the final D1500003 ROM:
diag_gym_roxanne_last and diag_gym_josh_coop with no assert.

## Not tested

- Other single-trainer doubles with a replacement mid-battle (a foe fainting and
  the trainer sending in a third) on the guest.
- Two-PC netplay or mGBA GUI; no full suite (the j-series checkpoints are not
  available).
