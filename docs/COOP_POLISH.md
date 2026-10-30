# Polish: settings, shiny, catch result, battle conditions

Sections 5–7 and 10 of the collection update. Each player's settings are their own (CoopSave layout 5, `v5.settings`, `COOP_SET_*` in `include/global.h`). They are never sent to the partner and never touch battle state or the RNG.

## CO-OP > SETTINGS

The new entry sits just above BACK in the CO-OP list, both linked and unlinked. A changes the line under the cursor, and the list stays open on it.

| Line | Values (default first) | What it does |
| --- | --- | --- |
| LOW HP ALARM | 3 BEEPS / OFF | See below. The red HP bar is unchanged either way. |
| EASY FISHING | OFF / ON | ON: after "Oh! A bite!" the rod waits for A, and B lets it go. The rod, the map's fishing Pokémon and the encounter are unchanged. |
| SHINY CHECK | ON / OFF | ON: RUN in a wild battle while a shiny foe is still on the field asks first. |
| CATCH RESULT | ON / OFF | ON: a short result after each catch. |
| AUTOSAVE | ON / OFF | ON: the game saves by itself on entering a map, at most every five minutes ([COOP_AUTOSAVE.md](COOP_AUTOSAVE.md)). |

A migrated save has all bits 0, which is these defaults. The only difference from before the update is the low-HP alarm: three beeps instead of the stock four, and once per episode.

## Low-HP alarm (`src/battle_gfx_sfx_util.c`)

- **Stock:** SE_LOW_HEALTH plays four two-tone beeps (36 frames each) when the active Pokémon enters the red, then ends.
- **3 BEEPS:** the alarm is stopped after the third beep, 100 frames in.
  - It plays once per low-health episode of each Pokémon on the field: the first time it is in the red. A different Pokémon in the red, or the same one after it recovers, starts a new episode. Every battle start resets them.
  - Closing the bag or party menu does not replay it.
- **OFF:** no alarm.
- **Measured** (`lowhp_beeps`): it started at frame 2833 and was stopped at 2933. After the bag closed it was still stopped. With OFF, a new battle with the same red bar played nothing.

## Shiny visibility and SHINY CHECK

- **Star.** A shiny Pokémon's healthbox shows a star after its name for the whole battle, on both sides (`UpdateNickInHealthbox`). The disguise of an Illusion user shows the disguise's.
  - The stock sparkle and sound at the send-out are unchanged.
  - Both co-op screens showed the star on the same wild Pokémon (`shiny_coop`). The fixture made them shiny on the master's game only, so the guest's star shows that the guest uses the master's Pokémon.
- **SHINY CHECK.** In a wild battle (not trainer, Safari, Frontier or recorded), RUN with a shiny foe still on the field asks "A shiny Pokémon is still here. Leave?" on this player's screen only, with NO selected.
  - YES sends RUN as before. NO or B goes back to the action menu.
  - Nothing is sent until the answer, so a co-op partner just sees this player still choosing. There is no new wait and no frozen state.
  - Forced escapes (ROAR, TELEPORT, a fleeing foe), capture odds and shiny odds are not changed.
  - The YES/NO box is drawn only after the question has finished printing, because text printers share one colour table and drawing the box mid-print recoloured the question (found and fixed while testing).
- **Not done:** a shiny marker on followers, Surf appearances or base residents. This project has no shiny-specific overworld appearances to check.

## Catch result (`src/coop_catch.c`)

- **Solo (in battle):** after "Gotcha!" and the experience, two short pages:
  - `Hoothoot★: NEW in your POKéDEX!` / `Relaxed nature, Insomnia.`
  - `It will join your party.`, or `Your party is full: it can go to the PC, or you can swap it in.`
  - After that come the stock Pokédex page, the nickname question and the stock party-swap choice. These still do the moving and explain who goes to the PC.
  - A or B dismisses each page.
- **Co-op (in the field after the shared battle):** after "Your ball caught the …", the catcher sees the same summary, then where it went: "It joined your party." or "Your party was full, so it went to BOX n in the PC." Only then comes the nickname question.
  - The partner only sees "… caught …" (unchanged). Nothing opens on the partner's side and nothing interrupts a battle.

## Battle conditions panel (`src/coop_field.c`)

- **Opening it.** At the action menu, SELECT opens a read-only panel in the message box. The action prompt shows the SELECT key after "… do?" when it fits after the name.
  - LEFT/RIGHT (or UP/DOWN) change page; the page number shows top right.
  - B, A or SELECT closes it, back to the same action menu.
  - Not offered in PvP link battles (the sides differ per game) or with the debug battle menu on.
- **Pages:**
  - FIELD: weather, terrain, Trick Room, Gravity, Magic Room, Wonder Room.
  - OUR SIDE and FOE SIDE: Reflect, Light Screen, Aurora Veil, Safeguard, Mist, Tailwind, Spikes and Toxic Spikes layers, Stealth Rock, Sticky Web, Steelsurge.
  - One page per Pokémon on the field: its stat stages as modifiers (Atk+1, Spe-4), never stats. With a partner on our side, each of ours is labelled YOUR or ALLY.
- **Where the data comes from.** The engine attaches a snapshot of its field state to each action request (`BtlController_EmitChooseAction`). The request reaches the asked player's controller on every game, so the co-op guest, which does not run the engine, shows the master's real state as of that choice.
  - Opening the panel sends nothing, chooses nothing, consumes no RNG and does not hold up the link. The partner sees this player still choosing.
- **Durations** are shown only where they are known without hidden information:
  - Exact: rooms, Gravity, Tailwind, Safeguard, Mist, and our own screens.
  - "?" turns: a foe's screens (its LIGHT CLAY is hidden), and weather or terrain with a timer (an extending item's owner is not recorded).
  - "lasting": weather without an end.
  - The count includes the current turn.


| Scenario | Saves | Result |
| --- | --- | --- |
| `shiny_run` | lab (fixture: next wild Pokémon shiny) | Star on the healthbox; RUN asks; B returns to the menu; RUN, YES runs (outcome RAN) |
| `shiny_coop` | lab, linked | Star on both screens for the same Pokémon; the guest's RUN asks on the guest only; B back; the battle finishes normally |
| `catch_result` | lab | The result pages appear (species, star, NEW, nature, ability, "join your party"); CATCH RESULT OFF in the real menu; a second catch shows none |
| `lowhp_beeps` | lab | Three beeps then stop; no replay after the bag; OFF plays none |
| `battle_panel_coop` | lab, linked | The guest's panel after REFLECT and TAILWIND (FIELD, OUR SIDE with turns, FOE SIDE, stat stages, YOUR/ALLY); the master's panel shows the same state; B returns to the action menu; after "Reflect wore off" it is no longer listed; the battle ends normally |
| `easy_fishing` | j52 (SUPER ROD fixture, real SURF to Route 122) | A pressed 200 frames after the bite still hooks the Pokémon (the stock limit is 30) |

**Not gameplay-tested:**
- **Co-op catch result in the field:** this harness could not throw a ball in a two-target co-op double, which needs the co-op target choice. The field code path was only read in source.
- **EASY FISHING with B to let go**, and with the OLD ROD and GOOD ROD.
- **SHINY CHECK with YES in a co-op double** (what RUN does there is unchanged).
