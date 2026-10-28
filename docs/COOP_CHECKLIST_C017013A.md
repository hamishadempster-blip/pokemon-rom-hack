# Audit checklist, build 0xC017013A (co-op polish)

Taken from the source, the built ROM and gameplay runs, not from earlier documents. **Played** means a two-core harness scenario did it on this ROM (fixtures and test helpers are named in each scenario's header and in the [test log](COOP_TESTLOG_C017013A.md)); **source** means checked from the code only. Nothing here was tried in the mGBA GUI, on hardware or over netplay.

## 1. Activity indicators

| Requirement | Status | Evidence |
| --- | --- | --- |
| Icons: walk dot, talk bubble, menu, crossed swords, invitation "?", waiting tick, broken link | Done, played | `partner_activity` reads `gCoopMarkerIcon` for each (walk 0, talk 1, menu 2, battle 3, invite 4, link 6); the tick (5) in `talk_join` |
| Small, outlined, no flashing, no overlapping labels | Done | 8×8 outlined icons (`tools/coop_art/marker_icons.py`); no blink code; the icon replaces the name tag, never both (source); screenshots |
| From real game state; stale statuses clear | Done, played | The partner's own frame bits (activity, `COOP_EXT_*`); a kind must hold a few frames before it shows (`StableKind`), so doors and script starts don't flicker; icons return to the dot after menu, talk, question and battle (`partner_activity`) |

## 2. Partner visibility and movement

| Requirement | Status | Evidence |
| --- | --- | --- |
| Offscreen edge arrow (same or connected map) | Done; same map played, connected map source only | `partner_activity` (east arrow, back to the dot in view); `Coop_PartnerViewCoords` covers connected maps; range 32 tiles |
| FOLLOW in the partner menu, path history, never teleports | Done, played | `partner_menu`: Kai walks Rosa's trail east, north and west by held directions (`sTrail`, 24 steps) |
| Cancels on manual input | Played | Any button stops it ("Stopped following.") |
| Stops before warps, scene triggers, ledges, hazards, encounter terrain | Warps and wild grass played; scene triggers, ledges, water, ice/forced/cracked floors source only | `TileStop` in `src/coop_partner.c`; door stop at the Rustboro Centre; grass on Route 101 |
| Stops on battle, dialogue, menu | Source (battle via `CoopFollow_NoteBattle`; scripts and menus via the field-free check) | |
| No follow loops; on foot only | Loop refusal played; bike/surf refusal source | `partner_menu` |
| No player blocking in doors, corridors, exits | Played | `partner_appearance` (both through one door on the same frames, walking through the partner), `mays_house_doorway`, `doors_stagger` |
| Atomic partner appearance | Played | `Coop_CheckAppearance` every frame: one object, at the partner's place, right graphics, outfit, marker; 0 faults in `partner_appearance` (door, walk-through, Mach Bike, Surf) and five older field scenarios |

## 3. Joining NPC conversations

| Requirement | Status | Evidence |
| --- | --- | --- |
| One authoritative event; per-screen presentation; own reading pace | Done, played | Host runs the script; lines sent as expanded text; Rosa at slow text, Kai at fast (`talk_join` part 1) |
| Checkpoint waits for both, ready indicator, only participants count | Played | Yes/no and `applymovement` wait for the joiner's READY; tick over the waiting host on the joiner's screen, bubble then tick over the joiner on the host's |
| Choices stay with the initiator; answer reported | Played | "Kai answered YES." on Rosa's screen |
| Leaving / disconnecting releases waits; no reading timeout | Played | Joiner leaves with B (part 3); host goes ahead with B (part 2); link muted mid-wait (part 4): host released, joiner gets "The link was lost…" |
| Classification; shops, gifts, special sequences not mirrored | Played (special code, shop); gifts, battles, warps source | Branch-walking classifier; Dewford Hall man (special) and Petalburg Mart clerk (shop) refused |
| Nonblocking "partner is talking" note, not spammed | Played | Corner popup naming why ("Kai is shopping", "Kai is talking"), 3-second cooldown, a second A shows nothing |
| Occasional both-player context lines | Played | Time-of-day greeting once per NPC: "Up late, you two? Ah, Rosa's here too!" |
| Progression restrictions explained with the real missing requirement | Partial | Busy refusals and the corner note name the real reason (battle, menu, another question, a scene, distance, reconnecting; shop, gift, battle, warp). A joined conversation follows the host's story state; the joiner is not told when their own save would get different lines |

## 4. Battle targeting

| Requirement | Status | Evidence |
| --- | --- | --- |
| Highlight the target and its HP box | Done | Target sprite blinks, its HP box bounces; spread moves highlight every Pokémon reached |
| Labels ALLY, FOE, BOTH FOES, ALL OTHER POKéMON (+ YOURSELF, EVERYONE, YOUR SIDE, THE FOES' SIDE, THE WHOLE FIELD) | Played | `battle_cancel`: ALL OTHER POKéMON (Earthquake), FOE (LEFT) Lotad and ALLY Zigzagoon (Tackle), BOTH FOES (Tail Whip) |
| Warn when a move hits the partner | Played | "! This hits your ALLY too" (Earthquake); "! This attacks your ALLY" (Tackle on the ally) |
| No selector for moves with no selectable target | Source (stock rule kept) | Self, ally-only, random and counter moves submit at once |
| Cancel while the turn is open; acknowledged before reopening; late requests rejected consistently | Played | Guest change accepted and the new move used; master's own change accepted; commit boundary: across runs both outcomes seen (accepted; refused as late on both games; B after commit ignored), each consistent on both games |
| Never consume PP, items or Mega on a tentative action | Source | PP and gimmicks are spent when the turn runs; withdrawal undoes run, ball, switch order and the gimmick trigger. Items can't be changed (they leave the bag when chosen, stock); B says so |

## 5. Partner menu

| Item | Status | Evidence |
| --- | --- | --- |
| TRADE, GIFT (GIVE ITEM / GIVE POKéMON), BATTLE | Unchanged from earlier builds, played by existing scenarios | |
| VIEW TEAM: read-only, says when outdated | Played | Fresh ("sent just now") and, with Rosa's game not answering, the last team with its age ("…can't answer right now. This is their team as it was less than a minute ago."; "N minutes ago (may be outdated)" once older) in `partner_menu` |
| PING with cooldown | Played | Sound and note on Rosa's screen; a second one within five seconds is refused ("You pinged a moment ago. Wait N more seconds.") |
| FOLLOW | Played (§2) | |
| Unavailable actions explain why | Played (battle, menu, question); others source | `CoopPartner_BufferBusyReason` |

## 6. Evolution hints (summary screen)

| Requirement | Status | Evidence |
| --- | --- | --- |
| Real data, conditions, whether met now | Played | `evo_hint`: Charmander (level, "6 levels to go"), Pikachu (both Raichu branches, place), Eevee (10 pages), Chansey (friendship), Happiny (held item + time), Charizard (none) |
| Where to get items | Played | "Sold: Mauville Mart", "Sold: Lilycove Dept. 3F" (generated from map data) |
| Time of day and regional rules consistent | Played (night) + source | The hint calls the same check the evolution uses (`DoesMonMeetAdditionalConditions`), which reads the shared time of day while linked; "Not now: it's night." at 23:01 |
| A partner's Pokémon | Played | VIEW TEAM: target and method, "(Your partner's POKéMON.)" |

## 7. Not done / limits

- Progression differences inside a joined conversation are not explained (see §3).
- Items chosen in battle can't be changed.
- FOLLOW: on foot only; stops at grass, caves and other wild terrain; events on a connected map's tiles are checked only once on that map.
- The conversation classifier is conservative (anything it cannot prove is dialogue only is not shared).
- Not played: a connected-map arrow, FOLLOW stops at ledges/water/ice/scene triggers, a joined conversation across a power-off (session state is not saved; a fresh start has none to resume).
