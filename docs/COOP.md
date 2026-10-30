# Emerald Co-op

Two people, two saves, one Hoenn. Each game keeps its own party, bag, money, and catches. The host only hosts the link. While you are linked, gyms, the rival pair, the Elite Four, the champion pair, the villain bosses, and Groudon with Kyogre wait for both of you. Nothing needs a second player: a game that is not linked plays every one of those fights alone (see [Playing alone](#playing-alone)).

This is a non-commercial fan patch. It does not include a ROM.

Based off RHH's pokeemerald-expansion 1.17.1
https://github.com/rh-hideout/pokeemerald-expansion/
Pinned commit `bb1093a7c72d3961d9a37d8e02ad2abc2c866682`.

Build id baked into the link handshake: `0xD1500001` on the isolated feature branch (the release line is `0xC0170140` and later) (also shown on CONTINUE and in the SAVE window). Both games must run this build. Explicit unlink is signalled separately from temporary missing heartbeats.

## Clean ROM

`Pokemon - Emerald Version (USA, Europe).gba`

- Size: 16,777,216 bytes
- SHA-256: `a9dec84dfe7f62ab2220bafaef7479da0929d066ece16a6885f6226db19085af`
- SHA-1: `f3ae088181bf583e55daf962a92bb46f4f1d07b7`
- Title `POKEMON EMER`, code `BPEE`, revision 0, 128 KB flash

Keep that file on your PC, in `rom/` if you want `tools/coop_make_patch.sh` to find it. Expansion 1.17.1 does not need the ROM to compile. The ROM is only the base for the patch. It is gitignored. Do not commit it, and do not commit `pokeemerald.gba`.

## Build on Windows

Use WSL2 and the steps in [docs/install/windows/WSL.md](install/windows/WSL.md). Debian's `gcc-arm-none-eabi` 13.2 is the compiler this tree was checked with. From the repo root:

```sh
make -j$(nproc)
```

The ROM image is `pokeemerald.gba` in the repo root. Give each player their own copy and their own `.sav`. Two windows must not share one save file.

**Updating to a new build.** Back up each `.sav` first. Each player replaces their ROM with the new one under the same file name (mGBA loads `name.sav` for `name.gba`), or renames their save to match the new ROM's name. CONTINUE shows the build; both players must run the same build to link. A save from an older build loads on a newer one with nothing lost; a save written by a newer build is not guaranteed to work on an older ROM, so do not go back. Details: [COOP_SAVE_COMPAT.md](COOP_SAVE_COMPAT.md).

`tools/coop_protocol_test.c` checks the link frame on the host compiler, with no GBA tools:

```sh
gcc -DCOOP_HOST_TEST -iquote include tools/coop_protocol_test.c -o /tmp/coop_test && /tmp/coop_test
```

## Emulators

| Where | Emulator | What it is for |
| --- | --- | --- |
| One PC | mGBA 0.10.5 | Local cable. File, New Multiplayer Window. Two games, two saves. |
| Two PCs | RetroArch 1.22.2, gpSP core v1.1.0 | Internet. Netplay mode **Link Cable - Pokémon Gen3**. |

mGBA's internet link is not a release. Use RetroArch for that.

RetroArch netplay that locksteps one screen is a different feature. This hack needs the Gen 3 link-cable mode, which carries the Pokémon session (`MPK1`, 8 data words plus a checksum) between two independent games. Commands ride that session. `LINKCMD_COOP` (`0xC0A1`) is the normal command. If a core drops unknown commands, rebuild with `COOP_SEND_AS_HELD_KEYS` set to 1 in `include/coop.h`. Both layouts are accepted.

No custom server, and no relay is built into this ROM. gpSP has to be set to **Link Cable - Pokémon Gen3** before you open netplay. The core's Auto setting picks the wireless adapter for Emerald, and this game speaks the cable protocol, so Auto never links. If the public RetroArch relay is down, try UPnP, then a direct IP, then Tailscale. A room-code server is not part of this build.

gpSP drops a peer after four seconds with no cable packet. The game uses that same wait before it decides the cable is gone, so a short hitch does not end the session and a pulled cable still does.

## New game

**NEW GAME** plays the normal opening: Birch's speech, boy or girl, your name, the moving van and your room. (Builds before 0xC017013E asked whether to skip to the clock in your room; that developer start is now only offered when the test harness arms it, never in play.)

## How a session works

1. Both games are the same build. Each copy of the ROM needs its own file name and its own save. In mGBA, open one copy, choose File, New Multiplayer Window, and open the other copy there. RetroArch has to already be in a Gen 3 link lobby. Open the start menu, choose **CO-OP**, then **LINK UP**. The question stays on the bedroom. The game then waits on a message that counts how many games the cable can see. It should go from 1 to 2, then say who you linked with. Press B to cancel. A count that stays at 1 means the two mGBA windows are not in the same multiplayer session.
2. The first exchange compares the build id and the level cap. Either mismatch aborts. Change the cap from CO-OP before linking. The cap is on unless you turn it off.
3. You can walk different maps. The link stays up when either of you goes through a door. The partner sprite appears only on the same map, and it walks. A door is a new map, so the sprite drops and comes back when you return. The partner sprite never blocks you: you can walk through it, so a partner standing on stairs or in a doorway cannot trap you. When you share a tile, the sprite is drawn on the first free side of you (south, north, west, east), never inside a wall or an NPC.
   While linked, opening the start menu shows a small status window: the link and Twin Beacon state ("LINKED · BEACON ON", "BEACON OFF", or "no BEACON yet" before you have the item), your partner's name and what they are doing (exploring, talking, healing at a nurse, in a menu, battling), the map they are on ("…, with you" on yours, "…, inside" for a building), whether a fight here would be together ("Fights: together") or solo and why ("Solo: partner in menu", "Fights: solo (apart)", "Solo: front 3 fainted", ...), and which twin is your rival. Choosing **CO-OP** while linked first explains the current state in one line (for example "BEACON ON: fights here can be co-op with your partner." or "Linked: you explore together. BEACON OFF: optional fights solo.") and lists **BEACON ON** or **BEACON OFF** (named by what it will do; the same request as using the item), **UNLINK** (asks first), **PARTNER DEX**, **JOURNAL**, **PARTNER MARK**, **PING**, **APPEARANCE** (the wardrobe's outfit list, from anywhere) and **BACK**. PARTNER MARK cycles how your partner is marked on your screen: **INDICATOR** (the default: a small outlined marker just above their head, in your partner's player colour), **NAME** (their name in small outlined letters, no box) and **OFF**. Each player has a stable colour, blue, pink, gold or green, taken from the Twin Beacon style they chose at the shrine (gold, blue or pink) or otherwise from their trainer id; it never follows outfit palettes or day and night tinting. The message after each change names both colours. The mark only marks your partner, never yourself; it hides while a message, menu, scene, battle, warp or screen fade is up, and goes with the partner when they leave the map or the link. The setting is saved; saves from earlier builds keep their choice (ON becomes INDICATOR, NEARBY becomes NAME). B or BACK returns to the start menu. Not linked, CO-OP lists **LINK UP**, **JOURNAL**, **PARTNER DEX** (the catches saved at the last link), **LEVEL CAP**, **APPEARANCE**, and **BACK**.
   - **Linked** means you see each other, explore independently, and share story progress.
   - **Beacon on** means ordinary wild Pokémon and trainers can become co-op battles when you are on the same map.
   - **Beacon off** means those stay solo, and you play independently. Mandatory story battles (gyms, rivals, villains, Elite Four, champion) are shared only with the Beacon on and your partner's YES; with it off they say so and start nothing (turn it on, or UNLINK to face them alone). See [Asking each other](#asking-each-other). Legendaries never need a partner: see [Shared legendaries](#shared-legendaries).
4. After the rescue fight, Birch gives each of you a Twin Beacon in the lab. The starter was already chosen on Route 101, so the lab nicknames it. The Beacon stays in the Key Items pocket. Using it asks the other player. Ordinary trainer battles become co-op only after both agree, and both games switch on that agreed exchange. Either person can ask to turn it off. A request made during a battle or trade waits until that fight or trade is over. Explicitly choosing UNLINK turns the Beacon off; temporary communication stalls preserve it. The item stays in the bag. Gyms, Soren and Mira, the Elite Four and the champion need the Beacon on and both of you on the map, and start only when your partner says YES (since 0xC017013E). Legendaries ignore the Beacon too, and are shared only when both of you agree.
5. Press A facing your partner for the partner menu: **BATTLE**, **TRADE**, **GIVE ITEM**, **GIVE POKéMON**, **VIEW TEAM**, **PING**, **FOLLOW** (see [With your partner](#with-your-partner)). If your partner is talking to someone you can join in instead. The party screen no longer has its own TRADE.
6. Explicitly disconnect from the CO-OP menu: you read "You unlinked." and your partner reads "<your name> unlinked." A real cable fault still says "The link dropped." Missing overworld heartbeats hide the unavailable partner and show "Link trouble: retrying..." in the start menu; they preserve the session and Beacon choice. Valid frames restore availability automatically when the existing transport resumes. UNLINK remains accessible during recovery. A closed emulator cannot communicate, and restarting it may require linking again. Fresh link sessions exchange shared progress and begin with Beacon off.

## Asking each other

Since build 0xC017013E every request to the other player (a shared wild or trainer fight, a shared story fight, a legendary, a Beacon change) follows the same rules.

- **Unlinked:** you play solo; story fights have their one-player versions.
- **Linked, Beacon OFF:** you play independently. Optional fights stay solo. A shared story fight (a gym leader, a rival, a villain, the Elite Four, the champion) says "the TWIN BEACON is OFF" and starts nothing: turn the Beacon on, or choose UNLINK in the CO-OP menu to face it alone. The opening rescue of Birch is the one exception: it never needs the Beacon.
- **Linked, Beacon ON:** a shared fight needs your partner nearby (same map), free, and answering YES.
- **Free** means: not in a battle or a screen (bag, party, PC, shop, summary), not reading a message, not answering another question, not in a trade or gift, not in a scene or locked script (even one with no text on screen). A partner who is not free is never interrupted: their screen stays as it is. For a story fight your game asks again every half second and shows "Waiting for <partner> to be free… N"; once they are free their game asks them.
- **Story fights ask.** Your partner reads "<you> is ready to battle <leader> with you. This fight needs you both. Ready now?". Nothing starts until they say YES; closing a conversation never starts a fight by itself. NO: you read "<partner> isn't ready for this fight yet. Nothing started." If both of you talk to the same leader at the same moment, link player 0's request wins and both games join one fight.
- **Timing.** The asking player waits about 30 seconds, with a countdown on screen; the partner's question closes after about 25 seconds (a question nobody touched closes too). The time is read from the cartridge clock, so it is about 30 real seconds whatever the emulator speed; if the clock is missing or stopped the game counts frames instead (30 seconds at normal speed). Busy time does not eat the answer time: a partner who becomes free gets the full time to answer, and a partner busy for 30 seconds ends the wait ("<partner> stayed busy…").
- **Cancelling.** B on the countdown: for an optional fight "battle alone" now; for a story fight "stop waiting" (nothing starts, nothing is used up). The partner's open question then closes as too late ("Your partner stopped waiting. Nothing started."), and a late YES starts nothing.
- **UNLINK** stays available from the CO-OP menu at all times. Legendaries keep their own clearly labelled question (see [Shared legendaries](#shared-legendaries)).

## Playing apart and relinking

Since build 0xC017013E one ownership rule decides what a link carries.

- **Always personal:** party, PC, money, bag, caught Pokémon, purchases and item pickups. A link never copies them (trades and gifts go through the partner menu, with your consent).
- **Earned alone stays alone:** a badge, a trainer or rival win, a quest step or a finished quest (Power Problems, the Desert Caravan, Coastal Cleanup, the shrine, a rival introduction) that one save earned while not linked stays on that save. Linking later does not hand it to the partner: they still play it on their own save. This is not a desync; it is each save's own story.
- **Earned together counts for both:** a gym, rival or story fight won together, and quest progress made while linked, are recorded on both saves. Each save notes who it earned them with (by trainer id, never by name). If one game lost power before saving such a win, the next link **with the same partner** restores it (once). A joint win recorded with someone else is never given to a new partner.
- A link never copies whole flag or variable tables and never moves a save backward. Pending trades, gifts and legendary rewards belong to the partner they were made with (see [With your partner](#with-your-partner) and [Shared legendaries](#shared-legendaries)).
- Saves from before 0xC017013E keep everything they have; their earlier joint wins have no partner on record, so they are never handed on again.
- Changed in 0xC017013E: before, a quest finished alone (and the rival introduction met alone) reached the partner at the next link, Power Problems breakers took the higher count, and joint badges and the rival stage were merged with any partner.

## Team preparation

Since build 0xC017013E a shared story fight (gym leaders, the Elite Four, the champion, Soren and Mira, Wally, the Aqua and Magma bosses, quest bosses) starts with both players choosing their team. Wild Pokémon and ordinary trainers do not use it: they still send your first three.

- After your partner says YES, both screens show **Team vs <opponents>** with up to three of your Pokémon in order (LEAD, SECOND, THIRD), each with level, HP and ITEM or MEGA for a held item or Mega Stone, the time left, and what your partner is doing ("choosing", "changing team", "READY").
- The first offer is the team you chose last time if all of them can still fight, otherwise the first three Pokémon able to fight.
- **READY** waits for your partner ("READY. Waiting for <partner>… B: change"); B takes it back. **CHANGE TEAM** opens the party screen's team picker: choose up to three from any slot (slots 4–6 included) in the order you want, then CONFIRM; fainted Pokémon and eggs cannot be chosen. **CANCEL** (or B on the menu) steps back: both players are told, nothing starts.
- The fight starts only when both are READY at the same moment. If either changes their mind first, nothing starts until both are READY again. Nothing is healed, and nobody is confirmed automatically: after about two minutes of game clock with no start the choice closes on both games ("Choosing teams took too long. Nothing started.").
- In the battle your chosen Pokémon take the places of your first three. Afterwards every Pokémon is back in its own party slot with what happened to it (HP, PP, items, EXP, EVs, new moves, evolution); the order of your party does not change.
- **Unlinked**, a gym leader, Elite Four member or the champion offers the lead only (**Lead vs …**, READY / CHANGE LEAD / CANCEL) when two or more of your Pokémon can fight; the whole party fights, and the lead you choose moves to the front of your party for good, as if you had switched it in the party menu. Solo rivals and villains start as before.

## Duo Frontier

Since build 0xC017013E every Battle Frontier lobby has a **DUO DESK** (a link receptionist standing near the entrance). The single-player counters are unchanged.

| Facility | Duo mode | Status |
|---|---|---|
| Battle Tower | DUO TOWER | Open |
| Battle Factory | DUO RENTALS | Not available in this version (the desk says so) |
| Battle Dome | DUO DOME | Not available in this version |
| Battle Palace | DUO PALACE | Not available in this version |
| Battle Arena | DUO ARENA | Not available in this version |
| Battle Pike | DUO PIKE | Not available in this version |
| Battle Pyramid | DUO EXPEDITION | Not available in this version |

**DUO TOWER** is for two linked players standing in the Battle Tower lobby. No Twin Beacon is needed: talking to the desk and the partner's YES are the consent.

- **Starting.** One player talks to the desk and chooses **LV. 50** or **OPEN LEVEL**. The partner reads "DUO TOWER: <name> is ready. New run, LV. 50. Join? (25 s)". After YES both see the team screen ("DUO TOWER LV. 50, battle 1") and choose exactly three Pokémon each; the battle starts when both are READY.
- **Rules (shown under RULES at the desk).** Three Pokémon each, one in battle at a time each, against two Tower trainers at once. No two of the pair's six Pokémon may be the same species or hold the same item (both screens say which, e.g. "Both teams have SHROOMISH: change one", and nothing starts until one of you changes). Eggs and the species the Frontier bans cannot enter. LV. 50: Pokémon above Lv. 50 fight at Lv. 50. OPEN LEVEL: the opponents match the highest level of the six (at least Lv. 60). Mega Evolution works with your own Mega Bracelet (there is no separate no-Mega bracket).
- **Your Pokémon never change.** Each battle is fought by healed copies of the three you chose; afterwards (win, loss or a failed link) your party is exactly as before: same levels, EXP, HP, items. No EXP, no prize money, no bag in battle, no trainer flags, no catches.
- **Between battles** whoever talks to the desk is asked "Go on to battle N?"; YES asks the partner again (the ready check), and the team screen then offers only READY or CANCEL with the run's team. NO saves and stops; CONTINUE at the desk later goes on.
- **Opponents.** Each battle's two trainers and their teams come from the Battle Frontier trainer and Pokémon tables, harder with each set of seven (as the solo Tower). Both games build the same battle from the run's id and the battle number. Battles 21 and 49 are **DUO MILESTONE** pairs (two fixed top-tier Tower trainers with perfect IVs; not a Frontier Brain).
- **BP and records.** After every 7th win each player gets BP once: 3 for the first set, then one more per set up to 10, plus 5 at battle 21 and 10 at battle 49. Each save keeps its own best duo streak for LV. 50 and OPEN LEVEL, separate from the solo Tower records, and its own duo milestone marks (not the solo Symbols). A lost battle ends the run.
- **A run belongs to one partner** (by trainer id). A save with a run with someone else is refused ("<name> has a DUO run with someone else") until that player retires it at the desk. RETIRE ends your own run (the record stays).
- **Interruptions.** If the link fails during a battle, nothing is won or lost on either game; CONTINUE goes on from the same battle. Each save records a win when it saves after the battle. If one game lost power before saving, the run goes on from the earlier of the two saves ("<name>'s save is at battle N"); BP already paid are never paid again.

## Field kit: BOX LINK and HEALING VIAL

Since build 0xC017013E the first POKéMON CENTER nurse you talk to after getting your first Pokémon gives each save two KEY ITEMS (once; a full pocket leaves them for next time):

- **POKéMON BOX LINK** opens the POKéMON STORAGE SYSTEM from the field (BAG, KEY ITEMS, USE). Storing a Pokémon does not heal it: it comes back with the HP and status it went in with.
- **HEALING VIAL** (BAG, KEY ITEMS, USE) is a POKéMON CENTER's full heal for your whole party, outside battle: HP, status and PP, and fainted Pokémon come back too. There is no party menu; one use takes one charge. It holds 1 charge, 2 after your first badge, 3 at four badges and 4 at seven, and every Center heal refills it. With the whole team already in perfect shape it says so and keeps the charge. (Before build 0xD1500001 it healed one Pokémon's HP and status, no revive, no PP, with 1 charge plus 1 per two badges.)
- Neither works in the Battle Frontier (including the Pyramid and the Pike), the Safari Zone or Trainer Hill, nor while a trade, gift, shared-battle question or team preparation with your partner is open.

## Move information in battle

In the move menu, **L** opens a compact panel over the top of the battlefield (L again, A or B closes it; cursor moves update it): the category icon, **PWR** (**var.** when the power depends on the battle, with what decides it, such as "Power: foe's weight."), **ACC** (**sure** for a move that never misses), who the move hits ("One target.", "Both foes.", "All others.", "Self."...), "No PP left." or "Disabled." when that applies (Disabled is shown on the game that runs the battle; the other player's game does not know it), then the move's description. The move menu's own box still shows PP and type.

The PP row also shows how the highlighted move matches up against the foes in front of you, on both players' screens and in every fight (since build 0xD1500001; it used to need the foe to be "seen" in the Pokédex, which a link battle never records, so it never appeared): **○** normal damage, **◎** super effective, **★** four times, **△** not very effective, **▽** barely, **✕** no effect. A status move shows nothing. With two foes it shows the better result, and choosing a target shows that target's own result. It goes by the types and abilities in play.

## Experience for the whole party

Since build 0xD1500001 every Pokémon in the party gets the full experience for a knockout, not only the ones that fought, in solo and co-op fights alike (the engine's "Exp. Share" is not needed; a held Exp. Share does nothing extra). The Pokémon that fought are paid as before, with their bar and level-up screen. The others (the whole bench, and anyone on the field who was never sent out) are paid when the battle is over, silently, each on its own player's game: the same formula, level cap, Lucky Egg and Exp. Charm as a Pokémon that fought, effort values and friendship included, no screen and no link traffic per Pokémon. A Pokémon that was fainted at the time of a knockout gets nothing for it.

- Level-ups from the silent payout: new moves are learned when there is a free move slot; a full moveset keeps its moves (the Move Relearner in Fallarbor Town teaches what was skipped), except that the Pokémon on a co-op field get the usual new-move prompt after the battle. Evolution is offered after the battle for every Pokémon that levelled, including benched ones.
- Duo Frontier battles, Frontier facilities and link battles against another player give no experience, as before.
- A Pokémon at the level cap gains nothing (but the effort values still count at level 100), as for a Pokémon that fought.

## Battle conditions (SELECT)

At the action menu (FIGHT/BAG/POKéMON/RUN), **SELECT** shows what is on the field, read-only, in the message box. The prompt shows the SELECT key when it fits after the name.

- **Pages** (LEFT/RIGHT to change; B, A or SELECT to close):
  - FIELD: weather, terrain, rooms, Gravity.
  - OUR SIDE and FOE SIDE: screens, Safeguard, Mist, Tailwind, hazards.
  - One page per Pokémon with its stat stages. With a partner, each of ours is marked YOUR or ALLY.
- **Turns** are shown where they are known. A foe's screens, and weather or terrain that an item may have lengthened, say "?".
- **No side effects.** Opening it chooses nothing and does not hold up your partner, who sees you still choosing.
- **Not offered** in link battles against a friend.

## Story rules

- Both players' rescue saves get the same story state. Whoever opens Birch's bag and whoever waits on the route both run `Route101_EventScript_ApplyRescueState`. (Before build `0xC017012C`, the waiting player's save kept the rival visible in their bedroom, standing on the stair warp; see [Fixes](#fixes-in-build-0xc017012c).)
- From the rescue until Lilycove the rival is out on the road, so both rival bedrooms hide them on entry and when a save made inside the room is loaded.
- **The rival twins.** Professor Birch's twins are both rivals. **Mira** (a wide-brim sun hat with a teal band, teal and sand clothes) is warm, curious, keen to befriend Pokémon everywhere. **Soren** (spiky dark hair, a gold headband, red jacket, dark trousers) is brash, competitive, sure he is winning. Both have their own overworld sprites (walking, running, bike) and their own trainer pictures for battle intros, the PokéNav and the match call list, so neither looks like a player. The bedroom introduces the twin who lives there on your save (Mira for a boy, Soren for a girl, as in the stock game); that twin mentions the other, who is out helping Birch. Wherever the story places the rival on the overworld (Route 103, Birch's lab, Oldale, Rustboro, Route 104, Route 110, Lavaridge, Route 119, Lilycove, the champion's room, Littleroot), the other twin stands beside them and follows them, so both are present for every rival fight. The second twin cannot collide with anyone and never stands on a warp. Talk to them for a short line.
- **Seeing each other across boundaries.** Your partner is drawn on directly connected outdoor maps too (north, south, east, and west connections, including offset ones such as Route 102 and Petalburg), within the strip of the next map your screen can show. Walking over a boundary is a normal step, not a jump. When your partner goes through a door, into a cave, or warps or flies away, they disappear and a short note appears in the map name window ("NAME went inside", "NAME: PETALBURG CITY"). Interiors and maps further away never show them. Seeing your partner across a boundary does not make them eligible for battles: that still needs the same map. A surfing partner sits on a surf mount.
- **Your rival.** The case decides it: whoever holds the case starter faces Soren, whose lead beats it, and whoever holds the companion faces Mira, whose lead beats that. Your save stores it at the rescue; older saves work it out from the party once. Before the Route 103 fight your rival names their counter-pick, and the twin you have not met introduces themselves. The start menu status window shows your rival.
- **Joining important scenes.** When one player meets the rival in the bedroom, a partner standing in the same room who has not met the twins yet joins from their own save: the rival turns to them as well, greets you both, and both saves record it. A partner in a menu is not interrupted; they join as soon as the menu closes. A partner elsewhere receives the finished introduction the next time they change maps. In the bedroom the rival is then at the PC with a short line, not the introduction. If both players press A at the Poké Ball together, each save plays its own introduction once.
- **Shared story ledger.** Shared milestones travel on the cable with stable ids (`include/constants/coop.h`), are stored in the save, only ever add state, and are written at a map transition, before that map's own scripts run, so nobody is pulled into a cutscene. A reconnect exchanges the ledger again. The ledger currently carries the rival introduction; the existing badge, rival stage, and legend exchanges are unchanged.
- **Rival fights for the guest.** The partner of whoever talked to the rival now gets the same follow-on state as the talker: after Route 103, Birch's lab is ready to hand out the Pokédex and the rival waits there and in Oldale; after Rustboro, the rival is met and registered; after Lilycove, the rival is home again. Raises never move a later save backward.
- **Shared items.** The Dowsing Machine (Route 110) and HM Fly (Route 119) reach the guest exactly once. With a full bag the story still moves on and the item waits in the save; it arrives with a message once there is room.
- **Shared and personal outcomes of a scene fought together.** Story state stays per save and is never copied wholesale between saves. When a scene's fight is won together, the fight is shared and the rest is personal: each save gets its own state for that scene, its own reward, and its own short catch-up. Petalburg Woods: the player whose walk set off the Devon researcher's scene plays it in full; a partner who joined the grunt fight and won gets a few lines from their own researcher, their own GREAT BALL, and their scene marked done, so walking past that spot does not replay it. A partner who did not join (Beacon off, refused, or elsewhere) still meets the scene and the grunt on their own save. Ordinary trainers stay independent: beating one together marks it beaten on both saves, nothing more.
- **Time of day.** Each game keeps its own clock, and nothing is written to it, so berries and daily events run on your own clock. While linked, both games use the link host's time of day (hours and minutes) for what depends on it: the lighting, time-based evolutions and their hints, wild encounters and night music. The two players never see different lighting side by side, and what an evolution hint says matches the sky. Unlinking returns each game to its own time. Menus, doors, healing and battles restore the outdoor tint instead of resetting it.
- Birch on Route 101 waits until both of you are on that map and linked. One person opens the bag. The other waits on the route with their menu closed.
- The ball you highlight names both Pokémon: Torchic brings Fennekin, Mudkip brings Totodile, Treecko brings Chikorita (Battle Guide v2; saves started earlier keep the Houndour, Wingull or Shroomish they got). You keep the one you highlighted. Your partner gets the other. Both are level 5, with their own level-up moves. Then both of you fight two level 2 Poochyena. Press B on the wait if your partner is still in a menu. After the fight, both games go to the lab on their own. Birch gives the Twin Beacon and nicknames the lead. A full Key Items pocket lets you walk away, make room, and talk to Birch again. The cable does not have to stay up for that gift.
- Soren and Mira take the case that beats yours: choose Fire and they use Water, Water and they use Grass, Grass and they use Fire. They are the pair in every rival fight (Route 103, Rustboro, Route 110, Route 119, Lilycove), and both stand on the map for those scenes. While linked, both of you have to be there; a game that is not linked fights its own twin alone (see [Playing alone](#playing-alone)). Soren owns the case line (Swampert, Sceptile or Blaziken) and Mira the companion line (Feraligatr, Meganium or Delphox). Which twin is your rival comes from your save (set at the rescue), never from your party. In a linked fight the inviter's case picks the teams. Arrive at Lilycove before Winona's badge and the twins bring an earlier team (levels 44-46, no Mega Stone); from six badges they bring their regular team, each ace with its Mega Stone. The teams are listed in [COOP_TEAM_SPEC.md](COOP_TEAM_SPEC.md).
- If one of you reaches a rival scene alone while linked, the rival says you both have to be there (UNLINK first to face them alone), walks back to their spot, and the map music returns; the scene plays in full when you are together. The twin who stands beside the rival has a line that changes with how far your rival story has got.
- Gyms are two leaders, three Pokémon each, and both stand in the room (Heath beside Roxanne, Reed beside Brawly, Joule beside Wattson, Cole beside Flannery, Hana beside Norman, Gale beside Winona, Wallace beside Juan; Tate and Liza as before). Talking to either starts the same fight; after it each has their own words. The Elite Four rooms and the Champion's room have their second trainer standing beside the first in the same way. Linked, you start them together; not linked, you fight both alone in a double battle. Ace levels are 18, 24, 30, 36, 42, 48, 54, 60 (Battle Guide v2). Rematches are a second pair around level 74-76, and a rematch team is only used once your progress allows its levels.
- With the Twin Beacon off, ordinary trainers stay single even on the same map. With it on, an ordinary trainer is a co-op battle while you are together on their map. Two trainers who walk up together both fight, as long as they are different trainers. A couple who already battle as a pair, such as the twins, stay a single-player double. The Battle Frontier, the Safari Zone, the Battle Pyramid, and Trainer Hill stay single-player (the DUO TOWER is a separate, explicit two-player challenge: see [Duo Frontier](#duo-frontier)). Gym levels do not scale.
- While linked, leaders, the Elite Four, the champion, rivals (Wally included), and the Aqua and Magma bosses are fought by both of you: the Beacon must be on and your partner must say YES (see [Asking each other](#asking-each-other)). Not linked, each has a one-player version (see [Playing alone](#playing-alone)). Archie, Maxie, Shelly at the Weather Institute, and Matt move the guest's story forward too. The person who started the scene still plays the cutscene. After Archie, the guest warps to Route 128 and the host arrives there when the cutscene ends. The guest receives the Dowsing Machine on Route 110 and HM Fly on Route 119 from the shared win. Castform stays with the person who talks to the scientist.
- At Mossdeep Space Center, Steven asks if you are ready and then both of you fight Maxie and Tabitha. Steven does not take a party slot. Your first three are the team. A win clears Team Magma on both saves.
- The Elite Four and the champion are the same 3+3 shape. The champion pair is Steven and Wallace. After the win the guest goes straight to the Hall of Fame and records their own party, while the host plays the champion's room scene and arrives after. Both saves set the champion flag. If you both walk up and talk to an Elite Four member or Wallace at the same moment, link player 1 joins player 0's fight.
- Legendaries: see [Shared legendaries](#shared-legendaries).

### Party in co-op battles

- A co-op battle fields party slots 1–3 from each player, through the stock link-multi exchange (`COOP_BATTLE_SLOTS` in `include/coop_protocol.h`). Solo battles use all six. Enemy trainers keep their full teams.
- Slots 4–6 leave the party for the length of the battle. They cannot be switched in, they do not keep a fight going, and they never count toward a whiteout.
- When the battle finishes, slots 1–3 keep what happened: HP, PP, status, held items. Slots 4–6 come back exactly as they were. A Pokémon that joins during the battle goes into the next free party slot, or the PC if the party is full.
- A dropped cable mid-battle puts all six back the way they were when the battle started. No badge, no prize.
- If your first three are all fainted or eggs, a gym, rival, Elite Four, champion, or legendary fight refuses to start and tells you to heal or move one up. If your partner's first three are down, it refuses with a message about your partner. An ordinary trainer with the Twin Beacon on stays a solo fight for whoever's first three are down.
- When one player's three faint and the other player is still standing, the battle carries on. It ends in a loss only when both players' first threes are down.
- Co-op battles award experience and EVs to each player's participating first three and held Exp. Share holders, respecting the shared level cap. Ordinary competitive link battles still give no experience. The guest chooses level-up move replacements after the battle. Both players receive level-up evolution checks after returning to the field. Dropping the cable during the battle discards pending progression along with the party rollback.
- A co-op battle hands the cable back open, so the session stays linked after the fight.
- Each player confirms a move and its target. The player who finishes first sees “Ready. Waiting for <partner>… B: change your choice” until the turn starts. On the link master's screen the action prompt also says “Partner choosing…” or “Partner ready.” (the other game cannot see the master's choice before the turn runs, so it keeps the stock prompt).
- **Targets.** While you pick a target, the target's sprite blinks and its HP box bounces, and the move list is replaced by a prompt: **FOE (LEFT)** or **FOE (RIGHT)**, **ALLY** or **YOURSELF** with the Pokémon's name; for moves that hit several, **BOTH FOES**, **ALL OTHER POKéMON**, **EVERYONE**, **YOUR SIDE**, **THE FOES' SIDE** or **THE WHOLE FIELD**, with every Pokémon it reaches highlighted. A damaging move that reaches your ally says so on the second line (“! This hits your ALLY too” for Earthquake, Surf, Discharge…; “! This attacks your ALLY” when you aim at them); otherwise that line says “A: OK  B: Back”. Moves with nothing to choose (self, ally-only, random, counter moves) never show a selector.
- **Changing your choice.** While the turn has not started, B on the waiting message takes back a move, a switch or RUN (B in the first third of a second after choosing is ignored, so pressing B through the menus doesn't trigger it): “Changing your choice…”, and the action menu opens again only once the link master has withdrawn it (its side effects are undone; nothing was spent, since PP and Mega-style gimmicks are used only when the turn runs). The master decides in one place, so both games always agree: a request that reaches it after the turn has begun is refused (“Too late to change: the turn has already begun.”) and the first choice is used. An item can't be taken back once chosen (it has already left the bag, as in the stock game); B there says so.
- The intro party indicators use each player's actual participating party. Ordinary shared trainer victories mark that trainer defeated on both saves.
- A partner talking to an idle NPC turns that NPC toward them. Your own dialogue and cutscene movement take priority. Shared story hide flags remove completed NPCs on the current map without requiring a map reload.

### Wild Pokémon together

Ordinary land and surfing encounters can become shared wild battles. Safari Zone, Battle Frontier, Trainer Hill, and Battle Pyramid retain their facility rules. Fishing, Rock Smash, roamers, outbreaks, and scripted encounters retain their existing dedicated paths.

- An ordinary land or surfing encounter can become a co-op wild double when the Twin Beacon is on, both of you are on the same map (any tile), and both of you have a battle-ready first three. The game that rolls the encounter picks two wild Pokémon and asks the partner: “Your partner ran into a wild X nearby. Join the battle?” The player who stepped in the grass sees “Calling your partner over… B: battle alone.” A YES within about 25 seconds starts the double on both games (the person who stepped in the grass waits about 30, with a countdown). A NO, B on either side, a late answer, or a partner who is on another map, in a menu, a script, a battle, or a trade means the wild battle starts as a stock solo fight for the player who stepped in the grass. Nobody is ever added to a battle that is already running.
- When you are linked with the Beacon on and a fight still ends up solo, its first battle message says why on a second line: "Solo: partner is on another map.", "Solo: your partner is busy.", "Solo: your partner said no.", "Solo: your partner didn't answer.", "Solo: partner's team can't fight.", "Solo: your front three can't fight.", "Solo: the link is recovering.", "Solo: no co-op battles here.", or "Solo: a one-player encounter." (fishing, Rock Smash, Sweet Scent, roamers, outbreaks, and scripted encounters keep their one-player path). Pressing B to battle alone shows no note.
- A partner you can see across a route/town boundary is still on another map: the encounter is solo, with that note.
- Caves count: a cave floor is a land encounter (Granite Cave was tested). Surf encounters ask the same way.
- Captures: each ball belongs to its thrower and comes out of the thrower's bag. The thrower keeps the Pokémon (party, or PC when the party is full); the other save only gets a "your partner caught" line. A guest's catch is re-keyed to the guest's trainer ID with its data intact (before build `0xC017012E` it was corrupted).
- RUN ends a co-op wild fight only when both players pick it on the same turn; one RUN shows "X wants to run. Both players have to pick RUN on the same turn."
- Shared trainer wins pay the stock prize money to both saves (before build `0xC017012E`, co-op trainer wins paid nothing to either save).
- With the Beacon on, an ordinary trainer asks the partner on the same map the same way (“X challenged your partner nearby. Join the battle?”), with the same answer window and the same solo fallback. Mandatory fights (gyms, rivals, villains, the Elite Four, the champion) ask too, but have no solo fallback while you are linked: see [Asking each other](#asking-each-other). Legendaries have their own question (see [Shared legendaries](#shared-legendaries)).
- The bag takes Poké Balls only. Choose a ball, then press left or right to pick which wild Pokémon it is for, and A to throw. B puts the ball back. If only one wild Pokémon is left, the ball goes to that one.
- The battle shows who threw. A caught Pokémon leaves the field and the fight goes on against the other one.
- If both of you throw at the same Pokémon on the same turn, the first ball to land keeps it. The second ball misses and is used up. No copy is made.
- Throwing at different Pokémon on the same turn can catch both, one each.
- A catch belongs to the person who threw the ball. It goes into their party, or their PC if the party is full. Only their Pokédex marks it caught, only their save counts the capture, and only they are asked for a nickname. That happens after the battle, back on the field. Back on the field each screen says who owns what: “Your partner's ball caught the X. It belongs to them.” or “Your ball caught the X. It's yours to keep.”
- A full party and a full PC refuse the ball before it is thrown, and the fight continues.
- Pulling the cable during the shakes, or at any point before the battle ends, gives nobody the Pokémon.
- Running away needs both of you to choose RUN on the same turn. One player alone gets a message, and that turn is spent. A player whose first three are all down does not need to choose RUN. Roar, Whirlwind, and Red Card do not end the battle early. Your own Teleport switches out. A wild Pokémon can still flee with Teleport.
- Co-op wild wins award experience under the same participation and cap rules as co-op trainer battles.

### Rick and Allen (Route 102)

Rick (`TRAINER_RICK`, 615) and Allen (`TRAINER_ALLEN`, 333) on Route 102 are the optional pair. Rick is the left trainer and Allen is the right.

- Alone, or with the Twin Beacon off, each one is his normal solo fight. Beating one sets only that trainer's flag on your save.
- Walk into either one while linked, with the Beacon on and your partner on Route 102. If neither save has beaten either of them, you are asked whether to call your partner. Your partner gets a yes or no prompt. If both of you say yes, it is a 2-on-2 against Rick and Allen.
- A no, a busy partner, a partner who waits too long, or a partner whose save has beaten Rick or Allen turns it into a solo fight against the trainer you walked into.
- A joint win sets both trainer flags on both saves. A solo win sets only the flag of the trainer you fought. A loss or a pulled cable sets no flag.
- Flags never copy across the link. Each save checks its own two flags before it says yes.

Megas, Z-Moves, Dynamax, and Terastallization are off, including Primal Reversion.

A few Hoenn Pokémon were retuned for doubles. See [Roster changes](#roster-changes). When both players stand on the same tile, a wild double that rolls Plusle, Minun, Volbeat, Illumise, Lunatone, Solrock, Seviper, Zangoose, or either Indeedee brings that Pokémon's pair instead of a random second species. Gym leaders, their partners, the Elite Four, and the champion also switch more carefully and look for a two-hit knockout. Their ace levels are unchanged.

## Shared legendaries

**Rule.** When two eligible players take part in a shared legendary encounter, one successful capture gives each of them their own permanent Pokémon from that encounter, once per save. Alone (not linked, or the partner elsewhere or not taking part) you face it as in the stock game and keep your own catch. A second player is never needed.

**Encounters.** Shared: Rayquaza (Sky Pillar top), Groudon (Terra Cave), Kyogre (Marine Cave), each level 70, and Regirock, Regice and Registeel (level 40). Each is a single static encounter in each player's world. Not shared, by design:
- Latias or Latios roam: each world has its own roamer and it is met alone, as in the stock game (a roamer can't be met by both at once).
- The ticket islands (Southern Island, Faraway Island, Birth Island, Navel Rock) are event-only and reset after every Hall of Fame, so sharing them would repeat rewards. Their fights are unchanged and solo.
- FRLG legendaries are not in Hoenn.
- The Battle Frontier and other rental or rematch battles never give legendaries.

**Starting one.** Talk to the legendary while linked with your partner on the same map. For Rayquaza the Twin Beacons resonate: "Your two worlds touch here, and each holds its own Rayquaza." Then: "Face Rayquaza with <partner>? If it's caught, each of you keeps your own Rayquaza: no second catch and no extra BALL needed." Your partner's game asks them the same question. Both must say YES. If either says NO, doesn't answer, is busy, or isn't eligible, you're told why and asked whether to face it alone.

**Eligibility** is checked on each save separately: that world's story has reached the encounter (Rayquaza: awakened at Sky Pillar; Groudon and Kyogre: after the Hall of Fame; Regis: their puzzle done), that world's legendary is still there, and the save has no claim or pending reward for this encounter. Being linked or nearby never gives anything.

**Battle.** The co-op link double with your first three each against the one legendary (the engine's normal one-Pokémon-side double; no stand-in second legendary). Both players fight, and either may throw a ball. The first ball that catches it ends the battle; that ball is used up as usual, and the other player's ball is never needed. Running away, a knockout or a loss gives no one anything. The legendary stays for another try (a knockout no longer removes it for good), and it reappears the next time you enter the map, as in the stock game after running. A knockout of Groudon or Kyogre still ends their abnormal weather (as stock), so the cave entrance closes; the Weather Institute's 2F scientist then reports the next abnormal weather, which leads back to it.

**Reward.** "Rayquaza was caught! Both trainers can now receive their Rayquaza." The catcher keeps the caught Pokémon as usual. The partner's game makes its own: their own OT and trainer id, met at that place and level, in the ball that made the catch, with valid stats and level-up moves. Its nature, IVs and shininess are fixed by that save's trainer id and the encounter, so reconnecting or reloading can't reroll it. It goes to the party, or to the PC when the party is full; each player names theirs, and each Pokédex records its own catch. If party and PC are both full the reward waits ("Your Rayquaza is waiting. Make room in your party or PC, and it will come to you.") and arrives as soon as there is room, linked or not.

**Exactly once.** Every save keeps per-encounter records (claimed, pending, and owed for the catcher until the partner's save confirms). These records never look at what is in the party or PC, so trading, releasing or depositing the Pokémon changes nothing. The catcher's game saves, then sends the reward notice. The partner's game records it as pending and saves before it confirms ("<partner>'s Rayquaza is safely recorded on their save."). Then it delivers and saves again. If the link drops first, the catcher is told the reward will arrive the next time the two saves link, and the notice is resent then (only to that partner). A notice for a reward the save already has is confirmed without a second Pokémon. A save that already claimed an encounter, with any partner, can't share it again; if its legendary is somehow still standing there, talking to it just removes it.

**Older saves.** Saves from earlier builds are converted on first load. A legendary the save caught itself counts as claimed. A legendary the old shared fight gave to the partner (the save's own legend record says "partner") becomes a pending reward, delivered once. A legendary already gone from that world (its defeated flag set) stays gone.

## With your partner

Press A while facing your partner's sprite (linked, same map). A partner who is in a menu, a script, a battle, or another exchange is "busy": your request waits up to eight seconds and then says so, and nothing interrupts them. If you both ask at once, link player 0's question goes first and player 1 is asked instead. Every wait ends: on an answer, on B where the text says so, on a timeout, or when the partner leaves the map or the link.

**BATTLE** (a friendly battle, nothing at stake):
- Choose SINGLES or DOUBLES, then OWN LEVELS, ALL LV. 50, or ALL LV. 100, then UP TO 6, 4, or 3 Pokémon each, then STANDARD (no species and no held item twice in a team) or UNRESTRICTED. Your partner sees the whole rule set before answering.
- With 3 or 4, each player picks a team on a team screen; otherwise the whole party. Eggs sit out, and every Pokémon starts the battle fully healed.
- Afterwards both parties are exactly as before the battle (HP, PP, items, levels): no EXP, money, items, Pokédex entries, or trainer flags. A dropped link ends it at once for both, and the parties are restored.
- The Twin Beacon setting is unchanged by a friendly battle.

**VIEW TEAM**: your partner's party in the summary screen, read only (nothing opens on their screen). If their game can't answer right now, the last team you were sent is shown and the message says how old it is (“may be outdated”).

**PING**: a sound and a short note on your partner's screen (“Kai pinged you!”, or “Ping: <map>” when you are on another map), and their mark hops for a moment; one every five seconds. It is also in the CO-OP menu.

**FOLLOW**: your character walks the path your partner walked, on foot, one step behind, never jumping or teleporting. Since 0xC017013E it goes with your partner through an ordinary door, door mat, stairs, ladder or escalator, but only when that warp leads to the map your partner went to; your own game does the warp, and following carries on from the exit (or stops with "trail lost" if your partner has walked too far meanwhile). It stops (with a short note) on any button you press, and before any other warp (holes, puzzle warps, Secret Bases, the Battle Frontier) or a door to somewhere else ("not that door"), a tile that starts a scene, a ledge, water, ice or other tricky floor, or grass and cave floor where wild Pokémon can appear; and when a battle, a message or a menu opens, or your partner leaves by other means. Each game counts only its own steps (Repel, eggs, friendship). Your partner can't follow you while you follow them.

**Joining a conversation.** Press A on someone your partner is talking to (or on your partner while they talk) and you are asked “Join the conversation?”. Each game shows the conversation in its own message box, so each of you reads at your own speed; the person's script runs only on your partner's game. Where the conversation waits for an answer or the person moves, your partner's game waits until you have read up to there: you see a tick over your partner (they are waiting for you), and they see a speech bubble over you that turns into a tick once you have caught up; questions are answered by the player who started, and you see their answer (“Kai answered YES.”). B leaves once you have caught up; your partner can go ahead with B while waiting. Leaving, a dropped link or a closed game releases every wait; there is no reading time limit. Shops, gifts, battles, warps and other special scripts can't be joined: pressing A on that person or on your partner then shows a short corner note saying why (“Kai is shopping”, “A gift for Kai”, “Kai is battling”, “Kai is moving on”, or “Kai is talking” for story scenes and special scripts), at most once every few seconds, and nothing waits; you can talk to the person yourself once your partner is done.

**TRADE**: each player offers one Pokémon; both see exactly what they give and get, and both confirm. **GIVE POKéMON** and **GIVE ITEM** send one Pokémon or a stack of items; the receiver accepts or declines.
- You cannot give or trade away your last Pokémon that can battle, an egg, or a Pokémon holding an item that cannot be given. Key items, HMs, Mail, and other important items cannot be given.
- A full party sends a received Pokémon to the PC; a full PC or bag pocket refuses before anything moves.
- Trade evolutions happen (including held-item ones) after the trade.
- Each exchange is saved on both games before it completes: what you give is held in your save until your partner's game has saved too. If the link or power drops in between, nothing is lost or copied: the next time the same two saves link, they finish the exchange or return it. A partner whose save is lost for good leaves your held Pokémon or item pending; the start menu note says so.

## Quality of life

- **Partner catches in the Pokédex.** While linked, the two games exchange which species each has caught. The Pokédex list marks each entry: your ball only, your partner's (green), or both (red over green). CO-OP, PARTNER DEX says whose catches these are and when they were last synced; not linked, it shows the copy saved at the last link.
- **BATTLE SCENE: FAST.** OPTION, BATTLE SCENE now has ON, OFF, and FAST. FAST skips move animations and shortens message pauses. A shared battle (co-op or friendly) uses only what both players allow: FAST only if both chose it, animations only if both want them.
- **Which save is this?** CONTINUE and the SAVE window show the location and the ROM build ("Build C0170132"), so both players can check they have the same build before linking.
- **Outfits.** The wardrobe in your Littleroot bedroom (top of the room, free, any time) changes your outfit colours. There are six for each look: Brendan has CLASSIC, FOREST, CHARCOAL, OCEAN, PLUM and SUNSET; May has CLASSIC, SKY, LEMON, LILAC, ROSE and FOREST. The menu previews the trainer picture and the walking sprite before you choose. Only clothing colours change (skin, hair and the other people stay as they are), in every pose that uses the player palette: walking, running, both bikes, surfing, field moves, battle back sprite, trainer card. The outfit is saved with your game and sent to your partner, so two players with the same look can wear different colours at once. Older saves start in CLASSIC. It is separate from gender, rival and story.
- **Partner marker and activity.** The INDICATOR mark (CO-OP, PARTNER MARK) is a small outlined icon over your partner's head that shows what they are doing, from their game's own state: a dot while exploring, a speech bubble while talking, a menu while in a menu, crossed swords in a battle, a question mark while they wait for an answer from you, a tick while they wait for you to catch up, and a broken link (at your screen's corner when they are not on screen) while the link is reconnecting. It never flashes; a status that ends clears within a few frames, and short moments (a door, a script starting) don't flicker it. When your partner is off screen on your map or a connected one (up to 32 tiles away), a steady arrow at the screen edge points to them. After the Twin Beacon shrine's lore the mark takes the BEACON STYLE colour.
- **Evolution hints.** In the summary screen's INFO page, SELECT shows what the Pokémon evolves into and how, from the game's own evolution data: the level, item, time of day, held item, move, friendship or place (regional forms name the place this game uses: near Dewford for Alola, on Route 114 for Galar, on Mt. Pyre for Hisui), whether each condition is met now (“Now Lv. 10: 6 levels to go.”, “None in BAG. Sold: Mauville Mart”), and where the item can be had. Pokémon with several evolutions page through them with SELECT; the last press returns to the trainer memo. On a partner's Pokémon (VIEW TEAM) it says what and how, but not whether it is met.
- **Practice (the COACH).** A COACH stands in the Pokémon Centers of Rustboro, Dewford, Slateport, Verdanturf, Fallarbor, Mossdeep, Sootopolis, Pacifidlog, and Ever Grande. He sets up an AI battle against a copy of your own team or (linked) your partner's, singles or doubles, at your own levels or all at Lv. 50. Everything that fights is a copy: your party and money are put back afterwards, the bag is closed during the fight, and nothing is awarded.

## Saving, exploring and collecting (build 0xC0170142)

- **Autosave.** On entering a map, at most every five minutes, the game saves by itself into the slot that does not hold your own last SAVE. Loading an autosave asks whether to keep it or go back to your SAVE. CO-OP > SETTINGS > AUTOSAVE turns it off. [COOP_AUTOSAVE.md](COOP_AUTOSAVE.md)
- **Waiting at a Pokémon Center.** CO-OP > WAIT HERE on a Center's ground floor rests until MORNING, DAY, EVENING or NIGHT (only the time of day moves, never the day). While linked, it is done together with the partner in the same Center. [COOP_TIME.md](COOP_TIME.md)
- **Visitors.** 21 kinds of wild visitor (57 appearances on 26 maps) come by time of day and weather, in grass, caves, surfing and on a rod: 1 encounter in 4 while one is around. WAIT HERE > REPORT in a Center pages through the nearby ones. [COOP_ENCOUNTERS.md](COOP_ENCOUNTERS.md)
- **"Who can learn this?"** BAG > TMs & HMs > CAN LEARN counts and lists your party and PC Pokémon that can learn that move. [COOP_LEARN.md](COOP_LEARN.md)
- **FIELD NOTES.** CO-OP > JOURNAL > FIELD NOTES tracks your collection and pays collection rewards once per save. Rewards reached while playing are paid at once; on a save that had already reached some (one from an earlier build), they wait in FIELD NOTES, which asks to collect them. [COOP_NOTES.md](COOP_NOTES.md)
- **Hidden habitats.** Five hidden places with their own Pokémon, a find, and a reward for catching their three residents. [COOP_HABITATS.md](COOP_HABITATS.md)
- **Town specialities.** Six specialists, in Dewford, Fallarbor, Verdanturf, Fortree, Mossdeep and Pacifidlog, each with a small track. [COOP_TOWNS.md](COOP_TOWNS.md)
- **Expedition HQ.** Not in the game yet; its design is in [COOP_HQ_PLAN.md](COOP_HQ_PLAN.md).

## Quests

**CO-OP, JOURNAL** lists each quest's progress on your save, with the next step. Six quests are in this build; all are optional and can be done alone ([COOP_QUESTS.md](COOP_QUESTS.md) has the walkthroughs and data). Whatever either player finds counts for both; a finished quest reaches the other save at its next map change; each save collects its own rewards once, and a full bag keeps them waiting.

**New Mauville: Power Problems.** After WATTSON gives you the BASEMENT KEY, he warns that surges keep tripping New Mauville's three circuit breakers. A MAINTENANCE LOG by the stairs gives the order (STORAGE, SERVICE, CONTROL); a wrong order trips them all again. A ROLYCOLY and a JOLTIK are in the way of the first two panels. VOLT, who powers his amplifiers from the generator, stands on the MAIN SWITCH. Once the three are reset and VOLT is beaten, the switch stops the generator, the APPLIANCE ROOM (with a ROTOM) opens, and WATTSON pays you. Three torn pages point to a sealed chamber that opens with the generator off.
- **Alone:** everything can be done by one player.
- **Together:** while linked, a breaker either of you resets is reset on both saves at once (a wrong press trips both). With your partner on the map and the Twin Beacon on, VOLT calls FUSE and you can take them on together; NO keeps it a solo fight.
- **Apart:** if your partner stops the generator first, it is off on your save too from your next map change. WATTSON still has your own reward, and the ROTOM, PORYGON, and chamber cache are yours to find.
- **POWER SHOP:** a clerk by Mauville's Gym sells evolution items once the generator is off, with more at the sixth, seventh, and eighth badges.

**Shoal Cave: The Missing Researcher.** PIERS at the cave mouth has lost his partner ISLA in the ice rooms. Find her thermometer, notes and radio, then bring her out. ISLA gives 2 ICE STONES and 2 SNOWBALLS, then an ICE STONE a day; an ALOLAN VULPIX appears by the ice room ladder.

**The Desert Caravan.** ZAHRA's camp sits south of Route 111's desert. Her three crates are in the SANDBURIED RUINS (Go-Goggles), guarded by the raiders DUNE and MESA (together, a duo with your partner). Press the glyphs SUN, STAR, MOON to open the chamber, home to a SIGILYPH. ZAHRA gives SAFETY GOGGLES and her MERCHANT opens a daily shop.

**Mira and Soren: Separate Ambitions.** From the second badge the twins turn up on their own errands: Soren in Slateport, Fortree and Mossdeep (two spars, for a PP UP and a MUSCLE BAND), Mira on Route 109, at the caravan camp and in Shoal Cave. If your partner already met a twin, you get a short version. After all six, they hold an exhibition in Littleroot: a double against both alone, or a co-op battle together; EXPERT BELT and SOOTHE BELL.

**Coastal Cleanup.** MARLIN on Route 109 says someone is dumping on the beach. Find the sheen, the drum and the tracks, beat the crew (SKIFF and BARREL, a duo with your partner) and their chief SLAG, and calm the ALOLAN GRIMER. MARLIN gives 5 LURE BALLS and MYSTIC WATER, and Route 109 and Dewford fishing improves.

**The Twin Beacon's History.** Inscriptions outside the Desert Ruins, Island Cave and Ancient Tomb point to the SCORCHED SLAB, where a passage opens. In the shrine: two plates (your partner on one, or the idol boulder with Strength), two lanterns lit together (one each, or both quickly alone) and a riddle. The KEEPER's story unlocks BEACON STYLE colours for the partner mark (the edge arrow is now standard), and his TWIN TRIAL (three rounds, a co-op battle with your partner or a double alone) gives an ABILITY CAPSULE, then a RARE CANDY a day.

**POWER SHOP stalls.** Each finished quest adds a stall after the POWER SHOP's own stock: ICE GOODS, SEA GOODS, DESERT GOODS, BEACON GOODS.

## Roster changes

Build 0xC017013D (Hoenn Co-op Battle Guide v2) replaced every boss, rival, gym trainer and Aqua/Magma team from `tools/coop_trainers/battle_plan.json`; [COOP_TEAM_SPEC.md](COOP_TEAM_SPEC.md) lists each team, which ids carry it, when each variant is used, and every place the engine's data forced a substitution. The gym notes further down describe the teams before that build.

Stats below are HP / Attack / Defense / Sp. Atk / Sp. Def / Speed. Abilities are listed as ability 1, ability 2, hidden.

| Pokémon | Before | After | What changed |
| --- | --- | --- | --- |
| Plusle | 60/50/40/85/75/95, Plus / Lightning Rod | 60/50/40/100/75/105, Plus / Lightning Rod | Special attack and speed. Already learns Helping Hand and Encore. |
| Minun | 60/40/50/75/85/95, Minus / Volt Absorb | 60/40/50/75/100/105, Minus / Volt Absorb | Special defense and speed. Same cheering moves as Plusle. |
| Castform | 70 across, Forecast | 80/70/70/100/100/90, Forecast | All four forms. Weather Ball and the weather moves were already there. |
| Luvdisc | 43/30/55/40/65/97, Swift Swim / Hydration | 70/30/65/70/95/105, Swift Swim / Hydration | Bulk so Swift Swim is worth a slot. Learns Rain Dance and Heal Pulse. |
| Delcatty | 70/65/65/55/55/90, Cute Charm / Normalize / Wonder Skin | 80/65/65/80/65/110, Cute Charm / Prankster / Normalize | Prankster support. Learns Thunder Wave and Assist. |
| Spinda | 60 across, Own Tempo / Tangled Feet / Contrary | 80/60/60/70/60/80, Prankster / Tangled Feet / Contrary | Contrary stays hidden. Learns Skill Swap and Assist. |
| Volbeat | 65/73/75/47/85/85, Illuminate / Swarm / Prankster | 75/73/85/47/85/95, Prankster / Swarm / Illuminate | Prankster is now the first ability. Learns Encore. Tail Glow stays. |
| Illumise | 65/47/75/73/85/85, Oblivious / Tinted Lens / Prankster | 75/47/85/80/85/95, Prankster / Tinted Lens / Oblivious | Learns Follow Me. Encore and Wish were already on the learnset. |
| Lunatone | 90/55/65/95/85/70, Levitate | 90/55/70/110/90/60, Levitate | Slower, so Trick Room still wants it. Learns Trick Room. |
| Solrock | 90/95/85/55/65/70, Levitate | 90/115/90/55/70/60, Levitate | Physical twin, still slow. Learns Trick Room. |
| Zangoose | 73/115/60/60/60/90, Immunity / Toxic Boost | 73/125/60/60/60/105, Toxic Boost / Immunity | Faster attacker. Learns Facade. Toxic Boost is the ability you see first. |
| Seviper | 73/100/60/100/60/65, Shed Skin / Infiltrator | 73/100/60/115/65/80, Shed Skin / Merciless / Infiltrator | Merciless crits a poisoned target. Learns Toxic. |
| Kecleon | 60/90/70/60/120/40, Color Change / Protean | 70/100/70/70/120/70, Color Change / Protean | Faster, and it learns Skill Swap. Protean stays hidden. |
| Tropius | 99/68/83/72/87/51, Chlorophyll / Solar Power / Harvest | 109/78/83/82/87/71, Chlorophyll / Solar Power / Harvest | Learns Tailwind. Wide Guard stays. |
| Chimecho | 75/50/80/95/90/65, Levitate | 85/50/85/110/100/85, Levitate / Telepathy | Telepathy is hidden, so Levitate is still the usual ability. Heal Bell and Heal Pulse stay. |
| Huntail | 55/104/105/94/75/52, Swift Swim / Water Veil | 55/120/105/94/75/80, Swift Swim / Water Veil | Learns Aqua Jet. Shell Smash and Rain Dance stay. |
| Gorebyss | 55/84/105/114/75/52, Swift Swim / Hydration | 55/84/105/125/80/80, Swift Swim / Hydration | Faster special attacker. Draining Kiss and Shell Smash stay. |
| Relicanth | 100/90/130/45/65/55, Swift Swim / Rock Head / Sturdy | 100/110/130/45/70/65, Swift Swim / Rock Head / Sturdy | Head Smash was already on the learnset. |
| Mawile | 50/85/85/55/55/50, Hyper Cutter / Intimidate / Sheer Force | 65/100/90/55/65/50, Intimidate / Hyper Cutter / Sheer Force | Intimidate is the first ability. Learns Play Rough at level 32. |
| Sableye | 50/75/75/65/65/50, Keen Eye / Stall / Prankster | 70/75/85/75/85/40, Prankster / Stall / Keen Eye | Prankster is the first ability. Learns Will-O-Wisp. Quash was already at level 30. |
| Nosepass | 30/45/135/45/90/30, Sturdy / Magnet Pull / Sand Force | 60/45/135/45/90/30, Sturdy / Magnet Pull / Sand Force | More HP. Sturdy still takes the first hit. |
| Clamperl | 35/64/85/74/55/32, Shell Armor / Rattled | 55/64/90/80/60/40, Shell Armor / Rattled | Enough bulk and speed to reach Huntail or Gorebyss. |
| Whiscash | 110/78/73/76/71/60, Oblivious / Anticipation / Hydration | 110/95/73/76/71/60, Oblivious / Anticipation / Hydration | Higher attack. Hydration stays hidden. Juan's Whiscash is set to Hydration. |
| Mightyena | 70/90/70/60/60/70, Intimidate / Quick Feet / Moxie | 80/110/75/50/65/90, Intimidate / Strong Jaw / Moxie | Faster physical attacker. Strong Jaw powers its fang moves. Learns Throat Chop 32, Psychic Fangs 40, Play Rough 46, Parting Shot 56 beside Snarl, Taunt, and Yawn. |
| Masquerain | 70/60/62/100/82/80, Intimidate / Unnerve | 75/40/72/105/92/90, Intimidate / Unnerve | Bug/Water, like Surskit: no more 4× Rock weakness. Keeps Intimidate, Stun Spore, and Quiver Dance; adds Sticky Web 1, Bubble Beam 24, Scald 29, Tailwind 34, Hydro Pump 48. |
| Banette | 64/115/65/83/63/65, Insomnia / Frisk / Cursed Body | 74/125/75/60/73/80, Insomnia / Frisk / Prankster | Sturdier, faster physical Ghost. Hidden Prankster (its Mega's ability) for priority Will-O-Wisp, Taunt 24, and Curse. Shadow Claw 28, Poltergeist 38, Play Rough 44, Swords Dance 50. |
| Cacturne | 70/115/60/115/60/55, Sand Veil / Water Absorb | 80/115/75/110/75/55, Water Absorb / Sand Veil | Water Absorb is now the usual ability and heals it. More bulk; still slow. Drain Punch 28, Seed Bomb 32, Dark Pulse 41, Leaf Storm 47, beside Spiky Shield and Sucker Punch. |
| Flygon | 80/100/80/80/80/100, Levitate | 80/100/85/100/80/100, Levitate | Mixed attacker: Special Attack 80 to 100. Dragon Pulse 36, Earth Power 38 (already), Scorching Sands 42, Draco Meteor 56. Ground/Dragon and Levitate kept. |
| Swalot | 100/73/83/73/83/55, Liquid Ooze / Sticky Hold / Gluttony | 115/70/95/73/100/45, Liquid Ooze / Sticky Hold / Gluttony | Durable partner support. Keeps Yawn, Encore, Stockpile, and Toxic; adds Clear Smog 22 (wipes foes' boosts), Knock Off 33, Pain Split 45. |
| Delibird | 45/55/45/65/45/75, Vital Spirit / Hustle / Insomnia | 65/75/55/75/65/110, Vital Spirit / Hustle / Snow Warning | Fast support. Hidden Snow Warning sets snow for its own Aurora Veil 40. Fake Out, Ice Shard 8, Icy Wind 13, Helping Hand 18, Tailwind 30, Freeze-Dry 35, Brave Bird 45, Spikes 50. |
| Beautifly | 60/70/50/100/50/65, Swarm / Rivalry | 65/50/60/115/65/100, Swarm / Tinted Lens / Rivalry | Fast special attacker. Tinted Lens is a normal ability now. Psychic 37 and Hurricane 44 beside Bug Buzz, Giga Drain, and Quiver Dance (Protect removed). |
| Dustox | 60/50/70/50/90/65, Shield Dust / Compound Eyes | 80/40/90/60/105/55, Shield Dust / Compound Eyes / Healer | Defensive status support. Hidden Healer can cure its partner. Rage Powder 24, Toxic Spikes 28, Sludge Bomb 40, Safeguard 45 beside Moonlight, Light Screen, Toxic, and Whirlwind; Quiver Dance moved to Beautifly only. |
| Armaldo | 75/125/100/70/80/45, Battle Armor / Swift Swim | 85/130/110/45/90/50, Battle Armor / Swift Swim | Bulkier physical attacker. Rapid Spin 1, Knock Off 30, Stone Edge 40, Swords Dance 52 beside X-Scissor and Rock Blast. |
| Luxray | 80/120/79/95/79/70, Rivalry / Intimidate / Guts | 80/125/79/75/79/88, Rivalry / Intimidate / Guts | Electric/Dark (Shinx and Luxio stay Electric). Faster. Fire, Ice, and Thunder Fang at 1, Crunch 42, Wild Charge 50, Psychic Fangs 58, Supercell Slam 68; Swagger removed. |
| Ledian | 55/35/50/55/110/85, Swarm / Early Bird / Iron Fist | 70/105/65/35/95/95, Iron Fist / Early Bird / Swarm | Physical punching specialist (Iron Fist is now its first ability). Comet Punch 1, Mach Punch 15, Lunge 24, Ice and Thunder Punch 26, Fire Punch 30, Drain Punch 36, Acrobatics 42, Swords Dance 46; special Swift, Struggle Bug, Bug Buzz, Air Slash removed. Still four times weak to Rock. |
| Ariados | 70/90/70/60/70/40, Swarm / Insomnia / Sniper | 90/100/85/50/85/40, Swarm / Insomnia / Stakeout | Durable trapper. Spider Web 1, Toxic Thread 30, Sticky Web 34, Knock Off 38, Poison Jab 41, Leech Life 44 beside Shadow Sneak and Sucker Punch; Psychic removed. Still slow. |
| Sunflora | 75/75/55/105/85/30, Chlorophyll / Solar Power / Early Bird | 85/50/70/125/90/30, Chlorophyll / Solar Power / Drought | Grass/Fire slow sun attacker (Sunkern stays Grass). Ember on evolution, Weather Ball 16, Synthesis 24, Flamethrower 28, Heat Wave 37, Earth Power 46; physical Pound, Bullet Seed, Double-Edge removed. Weak to Rock, Flying and Poison. |
| Magcargo | 60/50/120/90/80/30, Magma Armor / Flame Body / Weak Armor | 85/45/120/85/105/30, Solid Rock / Flame Body / Magma Armor | Defensive wall. Stealth Rock 38, Recover 40 (was 47), Will-O-Wisp 42 (Slugma 30), Body Press 50. Still four times weak to Water and Ground, softened by Solid Rock. |
| Dewgong | 90/70/80/70/95/70, Thick Fat / Hydration / Ice Body | 100/60/90/80/105/60, Thick Fat / Hydration / Ice Body | Doubles support. Life Dew on evolution (heals itself and its partner), Fake Out 1, Scald 38, Haze 42, Ice Beam 48 (was 55), beside Icy Wind, Encore and Aqua Ring; Sheer Cold removed. |
| Noctowl | 100/50/50/86/96/70, Insomnia / Keen Eye / Tinted Lens | 110/40/60/95/110/70, Insomnia / Keen Eye / Tinted Lens | Specially defensive screen support. Light Screen 20 (Reflect 12), Roost 25, Hypnosis 30, Whirlwind 33, Psyshock 36, Hurricane 50; physical Sky Attack and Take Down, and Uproar removed. |
| Butterfree | 60/45/50/90/80/70, Compound Eyes / Tinted Lens | 70/35/65/100/90/85, Compound Eyes / Tinted Lens | Special attacker and support. Pollen Puff 30 (heals the partner's Pokémon); Tailwind 36, Rage Powder 40 and Quiver Dance stay. Sleep Powder removed (with Compound Eyes it slept a target almost every time). |
| Beedrill | 65/90/40/45/80/75, Swarm / Sniper | 70/110/65/35/80/105, Poison Touch / Sniper | Fast physical attacker. U-turn 26, Leech Life 30, Knock Off 41. |
| Arbok | 60/95/69/65/79/80, Intimidate / Shed Skin / Unnerve | 75/110/80/55/80/85, Intimidate / Shed Skin / Unnerve | Now Poison/Dark (Ekans stays Poison). Poison Jab 32, Sucker Punch 36, Knock Off 44; the fangs and Crunch stay. |
| Parasect | 60/95/80/60/80/30, Effect Spore / Dry Skin / Damp | 85/110/95/45/95/30, Dry Skin / Effect Spore / Damp | Slow physical support; Dry Skin first (Paras too). Leech Life 29, Seed Bomb 33, Rage Powder 40 (was 59). Spore stays. |
| Hypno | 85/73/70/73/115/67, Insomnia / Forewarn / Inner Focus | 95/55/85/100/120/55, Insomnia / Telepathy / Inner Focus | Bulky special support; Telepathy replaces Forewarn (Drowzee too). Heal Pulse 37; Swagger removed. Telepathy also blocks an ally's Pollen Puff heal (official rule). |
| Furret | 85/76/64/45/55/90, Run Away / Keen Eye / Frisk | 90/90/75/35/70/105, Technician / Keen Eye / Frisk | Fast utility; Technician first (Sentret too). Tail Slap 28, U-turn 39, Knock Off 42; Slam and Amnesia removed; Follow Me and Helping Hand stay. |
| Lanturn | 125/58/58/76/76/67, Volt Absorb / Illuminate / Water Absorb | 125/45/70/85/85/60, Volt Absorb / Water Absorb / Illuminate | Bulky partner support (Chinchou's slots too). Volt Switch 27, Scald 33, Heal Pulse 40; Take Down removed. |
| Kricketune | 77/85/51/55/51/65, Swarm / Technician | 80/105/65/40/65/85, Technician / Swarm | Physical utility. Bug Bite 14, Aerial Ace 24, U-turn 32, Encore 40; Absorb removed. |
| Cherrim | 70/60/70/87/78/85, Flower Gift | 75/95/80/65/90/95, Flower Gift | Grass/Fairy in both forms; Flower Gift turns it to Sunshine form in sun. Seed Bomb 24, Play Rough 34, Solar Blade 55; Take Down and Solar Beam removed. |
| Lumineon | 69/69/76/69/86/91, Swift Swim / Storm Drain / Water Veil | 85/45/85/85/100/100, Storm Drain / Swift Swim / Water Veil | Fast support; Storm Drain first (Finneon too). Icy Wind 26, Scald 33, Tailwind 38 (was 59); Attract removed. |
| Maractus | 75/86/67/106/67/60, Water Absorb / Chlorophyll / Storm Drain | 85/65/80/110/85/55, Water Absorb / Chlorophyll / Storm Drain | Special support. Pollen Puff 36, Energy Ball 40. |
| Spidops | 60/79/92/52/86/35, Insomnia / Stakeout | 80/100/105/35/100/30, Stakeout / Insomnia | Slow physical utility; Stakeout first (Tarountula too). Lunge 28, Knock Off 37, U-turn 41; Headbutt removed. |

Together on the same tile, a wild double of any of these pairs is the pair itself: Plusle and Minun, Volbeat and Illumise, Lunatone and Solrock, Seviper and Zangoose, Indeedee male and female.

| New Pokémon | Where | Level |
| --- | --- | --- |
| Veluza | Route 106, surfing | 20–25 |
| Basculegion | Route 107, surfing | 25–30 |
| Dondozo | Route 109, surfing | 25–30 |
| Sandile | Route 111 desert | 19 and 21 |
| Helioptile | Route 111 desert | 20 and 22 |
| Gigalith | Desert Underpass | 42 |
| Greavard | Mt. Pyre 1F | 24 |
| Dreepy | Mt. Pyre 1F | 28 |
| Mimikyu | Mt. Pyre 1F | 29 |
| Politoed | Route 119 surf, and Route 120 surf | 26–30, and 25–30 |
| Abomasnow | Shoal Cave, low tide inner room | 32 |
| Lechonk | Route 102 | 3 |
| Tandemaus | Route 102 | 4 |
| Applin | Route 104 | 4 |
| Tarountula | Petalburg Woods | 5 |
| Shroodle | Petalburg Woods | 6 |
| Nacli | Granite Cave 1F | 7 |
| Glimmet | Granite Cave 1F | 10 |
| Pawmi | Route 110 | 13 |
| Bramblin | Route 113 | 16 |
| Noibat | Meteor Falls 1F | 20 |
| Charcadet | Jagged Pass | 22 |
| Toedscool | Route 119 | 24 |
| Indeedee (male) | Route 118 | 24 |
| Indeedee (female) | Route 118 | 26 |
| Fidough | Safari Zone, south | 27 |
| Cyclizar | Safari Zone, south | 29 |
| Cetoddle | Shoal Cave, low tide inner room | 30 |
| Finizen | Route 124, surfing | 25–30 |
| Tatsugiri | Route 128, surfing | 25–30 |
| Wiglett | Underwater Route 126 | 30–35 |
| Varoom | Magma Hideout 1F | 33 |
| Tinkatink | Victory Road 1F | 38 |
| Gimmighoul | Victory Road 1F | 36 |
| Frigibax | Victory Road 1F | 40 |
| Jangmo-o | Meteor Falls B1F | 40 |
| Rookidee | Route 104 | 4 and 5 |
| Wooloo | Route 116 | 6 and 7 |
| Nickit | Route 116 | 6 and 8 |
| Crabrawler | Route 106, surfing | 10–14 |
| Arrokuda | Route 108, surfing | 20–25 |
| Capsakid | Route 111 desert | 20 and 22 |
| Cufant | Jagged Pass | 22 and 23 |
| Flamigo | Route 115 | 23 and 26 |
| Tadbulb | New Mauville | 22 and 24 |
| Impidimp | Route 121 | 26 and 28 |
| Pawniard | Victory Road B1F | 38 and 40 |
| Hatenna | Petalburg Woods | 5 and 6 |
| Wattrel | Route 104, and Route 110 | 4 and 5, and 12 and 13 |
| Riolu | Granite Cave 1F | 9 and 10 |
| Shinx | Route 110 | 12 and 13 |
| Cottonee | Route 117 | 13 and 14 |
| Smoliv | Route 117 | 13 and 14 |
| Rockruff | Route 114 | 16 and 17 |
| Rockruff (Own Tempo) | Route 114 | 16 |
| Joltik | New Mauville | 23 and 24 |
| Rotom | New Mauville | 24 and 25 |
| Litwick | Mt. Pyre 1F–3F | 25–27 |
| Foongus | Route 119 | 25 and 27 |
| Honedge | Altering Cave (Route 103, by Surf) | 26–30 |
| Petilil | Route 123 | 26 and 28 |
| Snom | Shoal Cave, low tide ice and inner rooms | 28–32 |
| Buizel | Route 104, and Route 104 surfing | 4 and 5, and 15–20 |
| Sewaddle | Petalburg Woods | 5 |
| Venipede | Petalburg Woods, and Rusturf Tunnel | 5, and 6 and 8 |
| Rolycoly | Rusturf Tunnel, and New Mauville | 6 and 7, and 22 and 23 |
| Skiddo | Route 116, and Route 118 | 7 and 8, and 25 and 26 |
| Drilbur | Granite Cave 1F, B1F, B2F | 9, 10, 11 |
| Fomantis | Route 117, and Route 123 | 13, and 26 and 27 |
| Darumaka | Fiery Path | 15 and 16 |
| Sizzlipede | Fiery Path | 15 |
| Salandit | Fiery Path, and Magma Hideout (a female is given in Lavaridge) | 15 and 16, and 30 |
| Scraggy | Route 111 desert | 20 |
| Mudbray | Route 111 desert, Route 112, and Jagged Pass | 21, 15, and 21 |
| Rufflet | Route 112, and Jagged Pass | 14 and 16, and 21 |
| Vullaby | Route 113, and Jagged Pass | 14 and 16, and 21 |
| Clobbopus | Route 115 | 23 and 24 |
| Clauncher | Routes 106 and 107, and Dewford (Good Rod) | 15–25 |
| Skrelp | Routes 105 and 109 surfing, and Routes 108 and 109 (Super Rod) | 20–30 |
| Grimer (Alola) | Slateport City and Route 110, surfing | 20–30 |
| Pancham | Route 119 | 25 |
| Solosis | Route 119, and Route 120 | 25, and 26 |
| Gothita | Route 120 | 25 and 27 |
| Vulpix (Alola) | Shoal Cave, low tide ice and lower rooms | 28 and 30 |
| Larvesta | Magma Hideout (rare) | 29 and 30 |
| Deino | Meteor Falls B1F back room (needs Waterfall) | 32 and 35 |
| Ledyba | Route 102, and Route 117 | 4, and 13 |
| Spinarak | Route 102, and Route 119 | 4, and 24 |
| Hoothoot | Route 116, and Route 121 | 6, and 26 |
| Sunkern | Route 116, Route 123, and the Safari Zone | 6, 26, and 33–34 |
| Seel | Routes 105 and 125, and Shoal Cave, surfing | 15–30 |
| Fletchling | Route 102 | 4 |
| Starly | Route 104 | 5 |
| Hoppip | Route 104 | 5 |
| Shellos (West) | Route 104, surfing | 15–25 |
| Shellos (East) | Route 124, surfing | 25–30 |
| Cutiefly | Petalburg Woods | 6 |
| Timburr | Rusturf Tunnel | 6 |
| Paras | Rusturf Tunnel | 7 |
| Minccino | Route 116 | 7 |
| Venonat | Route 116 | 8 |
| Ferroseed | Granite Cave B1F | 10 |
| Larvitar | Granite Cave B2F | 12 |
| Grubbin | Route 110 | 13 |
| Drowzee | Route 117 | 14 |
| Bounsweet | Route 117 | 13 |
| Dewpider | Route 117 and Route 120, surfing | 12–16, 20–25 |
| Tynamo | New Mauville | 22 |
| Ponyta | Route 112 | 16 |
| Growlithe | Route 112 and Fiery Path | 16, 15 |
| Skorupi | Route 111 desert | 21 |
| Gible | Route 111 desert | 19 |
| Sandygast | Route 111 desert | 20 |
| Axew | Meteor Falls 1F | 18 |
| Phantump | Route 119 | 26 |
| Dratini | Route 119, Super Rod | 25–30 |
| Croagunk | Route 120 | 27 |
| Zorua | Route 120 | 27 |
| Drifloon | Route 121 | 27 |
| Pumpkaboo (Average) | Mt. Pyre exterior | 29 |
| Pumpkaboo (Super) | Mt. Pyre exterior (rare) | 26 |
| Murkrow | Mt. Pyre exterior | 27 |
| Swinub | Shoal Cave, low tide | 28–30 |
| Sneasel | Shoal Cave, low tide | 30–32 |
| Mareanie | Routes 125 and 126, surfing | 25–32 |
| Frillish | Routes 124 and 126, Super Rod | 25–30 |
| Bulbasaur | Rustboro Pokémon Center guest, 1 badge | 10 |
| Charmander | Rustboro Pokémon Center guest, 1 badge | 10 |
| Squirtle | Rustboro Pokémon Center guest, 1 badge | 10 |
| Turtwig | Lavaridge Pokémon Center guest, 4 badges | 18 |
| Chimchar | Lavaridge Pokémon Center guest, 4 badges | 18 |
| Piplup | Lavaridge Pokémon Center guest, 4 badges | 18 |
| Snivy | Fortree Pokémon Center guest, 6 badges | 25 |
| Tepig | Fortree Pokémon Center guest, 6 badges | 25 |
| Oshawott | Fortree Pokémon Center guest, 6 badges | 25 |
| Chespin | Mossdeep Pokémon Center guest, 7 badges | 30 |
| Fennekin | Mossdeep Pokémon Center guest, 7 badges | 30 |
| Froakie | Mossdeep Pokémon Center guest, 7 badges | 30 |

Before/after details for the twelve 0xC0170137 revamps, the showcase trainers and the realistic-level test battles are in `docs/COOP_REVAMPS_C0170137.md`.

The full first-batch manifest (114 species: habitats, unlocks, evolution methods and where the evolution items are sold) is `docs/COOP_ROSTER_BATCH1.md`, generated by `tools/coop_roster/manifest.py`; `tools/coop_roster/audit.py` re-checks obtainability from the sources.

Gigalith is on the Desert Underpass. The underpass encounter table already runs from 35 to 45, so level 42 is inside that table. Sandile and Helioptile stay at the desert's own levels. Tadbulb uses New Mauville's own levels, 22 and 24, because that cave's table runs from 22 to 26. Crabrawler is a surf encounter on Route 106, which has no grass.

Their evolution lines are in the Hoenn Pokédex beside the route where you first meet them: Corvisquire and Corviknight, Dubwool, Thievul, Crabominable, Barraskewda, Scovillain, Copperajah, Bellibolt, Morgrem and Grimmsnarl, Bisharp and Kingambit. Flamigo has no evolution.

Build `0xC017012F` adds fourteen more families (see [COOP_ROSTER_F.md](COOP_ROSTER_F.md) for every level, item, and condition). Each save catches its own; every evolution item is sold every day, so neither player can miss one. Riolu becomes Lucario with high friendship outside night hours; Snom becomes Frosmoth with high friendship at night. Lampent and Doublade use a Dusk Stone and Cottonee and Petilil a Sun Stone (Verdanturf and Lilycove sell Sun Stones; Lilycove sells Dusk Stones). A Rockruff becomes Midday Lycanroc by day or Midnight Lycanroc at night; an Own Tempo Rockruff becomes Dusk Lycanroc at level 25 at any hour (the stock rule needs 19:00–19:59, too narrow here; the note on the Route 114 POOCHYENA says so). The scientist in New Mauville's entrance gives each save a Rotom Catalog, which moves Rotom in and out of its five appliance forms. Hisuian Lilligant was not obtainable then; build `0xC0170139` makes Mt. Pyre the Hisui stand-in, so a Sun Stone used on Petilil there gives it.

Three evolutions could not happen in this game and were changed in build `0xC017012E`: Gimmighoul needed 999 Gimmighoul Coins (none exist here) and now becomes Gholdengo at level 50; Bisharp needed to defeat Bisharp holding a Leader's Crest (none exist here) and now becomes Kingambit at level 64; Charcadet's Auspicious and Malicious Armor, and Boldore's Linking Cord, are now sold by the Lilycove evolution clerk.

Gym leaders, gym partners, rematches, the Elite Four, and both champions now have Smart Switching and Try To 2HKO on top of their old AI. The doubles-trick score bonuses are a bit higher, so they commit to Thunder Wave, Fake Out, Discharge, Eruption, Protect, Helping Hand, Tailwind, Trick Room, and the safe Earthquakes more often. Ace levels are the same.

The first clear of each gym was also edited so the two sides do not hit each other. Ace levels stay 16, 22, 28, 34, 40, 46, 52, and 58. Build `0xC017012E` puts the added Pokémon on every gym (see [COOP_GYM_AND_ROSTER_AUDIT.md](COOP_GYM_AND_ROSTER_AUDIT.md) for each final team): Nacli (Roxanne), Crabrawler (Brawly) and Pawmo (Reed), Tadbulb (Wattson), Armarouge (Flannery) and Scovillain with Chlorophyll under Drought (Cole), Maushold with Follow Me and Friend Guard (Norman), Corviknight with Tailwind (Winona) and Flamigo with Wide Guard (Gale), Indeedee-F with Psychic Surge and Follow Me (Tate and Liza), Veluza (Juan) and Dondozo (Wallace). They replace Geodude, Makuhita, Primeape, Electrode, Magcargo, Ninetales, Vigoroth, Swellow, Crobat, Claydol, Ludicolo, and Wailord. Onix uses Protect, and so does Nacli. Lileep uses Mega Drain. Fake Out stays on Hariyama and Medicham, and Meditite is Telepathy. Voltorb uses Light Screen, and Joule sends Minun with Volt Absorb so Wattson's Discharge heals it. Camerupt uses Protect, and Cole sends Tropius with Solar Power beside Flannery's Eruption. Exploud uses Shadow Ball, Spinda uses Protect, and Zangoose holds a Toxic Orb. Altaria uses Dragon Pulse, Gyarados uses Ice Fang, and Winona's Pelipper is Drizzle with Hurricane. Claydol uses Protect, Xatu uses Trick Room, and Gardevoir uses Helping Hand. Whiscash is Hydration with Liquidation, and Kingdra is Swift Swim with Surf, Ice Beam, Dragon Pulse, and Protect. The level 68–70 rematches are a separate bracket and were left as they are.

Build `0xC017012F` also rebuilds the ordinary trainers and the bosses (see [COOP_ROSTER_F.md](COOP_ROSTER_F.md#3-trainers-and-bosses)). Ordinary trainers gain a few levels (never reaching the next gym), themed teams that use the added Pokémon once players can catch them, a partner for most lone Pokémon from Mauville on, occasional held items, and better move choice. Their rematches keep the same species. Maxie, Tabitha, Archie, Shelly, Matt, and Wally have new teams with added Pokémon; each Elite Four side and champion gains one added Pokémon (no species repeated); the bosses, leaders, and twins weigh switches, two-hit KOs, PP stalling, and likely abilities, but never see your moves, items, or abilities before you use them.

## Hoenn Pokédex

Houndour and Houndoom sit with Poochyena, so the Cinder starter registers as soon as you take it. Every visitor above, and the rest of its evolution line, is in the Hoenn Pokédex after Deoxys. The new pages sit in roughly the order you can meet them. Seeing or catching one registers it before the National Dex is unlocked. The area screen reads the wild tables, so a seen species lights the route, cave, or water where that species is encountered. An evolution lights a place only when that evolved form is itself in a wild table. Politoed, Abomasnow, and Gigalith are. A pre-evolution you have not seen yet stays a blank slot until you do.

A species with no Hoenn number no longer counts as a Hoenn species for the summary screen or a link trade. Before the National Dex, a link trade still accepts anything in this regional list, including the visitors.

Build `0xC0170138` extends the regional list to 646 entries (the second batch, then Porygon, Hippopotas, Dwebble and Sigilyph, which were obtainable but had no regional number). Every one of them can be obtained in this build except Jirachi and Deoxys (event-only, as in stock Emerald). For a species with no wild area, the AREA page now says how to get it instead of AREA UNKNOWN: a daily gift, a fossil, a gift or one-time encounter, an in-game trade, an egg from the Day Care, or the evolution that makes it (for example "EVOLVE EEVEE: FRIENDSHIP, FAIRY MOVE"). Type search, as in stock Emerald, lists caught Pokémon only.

### Second roster batch (build 0xC0170138)

The 114 species are listed with their habitats, unlocks, levels, rates and evolution items in [COOP_ROSTER_BATCH2.md](COOP_ROSTER_BATCH2.md). In short:

- **Partner Pokémon.** Three more visitors at the left of the Pokémon Center counter give one of their region's three starters a day: Alola in Slateport (2 badges, Lv 14), Galar in Petalburg (5 badges, Lv 22), Paldea in Sootopolis (8 badges, Lv 35). A fourth, in Fallarbor (3 badges, Lv 16), gives Treecko, Torchic or Mudkip, so every Hoenn starter can be had on one save. A full party sends the gift to the PC.
- **Fossils.** The FOSSIL MANIAC in the Route 114 tunnel digs up one fossil a day: any of the eleven (Root, Claw, Helix, Dome, Old Amber, Skull, Armor, Cover, Plume, Jaw, Sail). The DEVON scientist in Rustboro revives any of them (Lv 20). The stock Root/Claw choice in the desert is unchanged, but it no longer decides anything.
- **Wild.** Eevee (Route 117), Togepi and Munna (Route 113), Cleffa, Clefairy and Elgyem (Meteor Falls), Lillipup and Skwovet (Route 101), Pidove and Cramorant (Route 103), Purrloin (Route 104), Blipbug (Petalburg Woods), Tympole and Chewtle (Petalburg, Route 102 fishing), Woobat (Rusturf Tunnel), Blitzle and Yamper (Route 110), Gossifleur (Route 117), Toxel (New Mauville), Milcery and Stonjourner (Route 114), Falinks (Jagged Pass), Silicobra (Sandburied Ruins), Pincurchin (Route 107), Ducklett and Morpeko (Route 118), Emolga (Route 119), Deerling and Audino (Route 120), Bouffalant (Route 123), Sinistea (Mt. Pyre), Vanillite and Eiscue (Shoal Cave ice room), Druddigon (Meteor Falls, deeper).
- **Evolution items.** The POWER SHOP sells the seven Sweets and the Soothe Bell from the start, and the Cracked and Chipped Pots from six badges; stones are sold as before.
- **Unusual evolutions.** Eevee: a stone picks its form; very friendly and knowing a Fairy move gives Sylveon (Sylveon comes first: Eevee learns Baby-Doll Eyes at level 20); very friendly otherwise gives Espeon by day or Umbreon at night; leveling up in Petalburg Woods or the Shoal Cave ice room gives Leafeon or Glaceon. Milcery holding a Sweet evolves when you spin in place on the field (turn through the four directions); the direction, length and time of day pick the cream. Toxel's nature picks Amped or Low Key Toxtricity at level 30. Sinistea evolves with a Cracked Pot (Phony) or Chipped Pot (Antique).

**Stock gaps closed.** Some regional entries could only be had by trade in stock Emerald. Each now has one wild slot: Surskit (Route 102), Budew (Route 103; Roselia by friendship by day, Roserade with a Shiny Stone), Galarian Zigzagoon (Route 103; Obstagoon at level 35 at night), Zangoose (Route 114), Lunatone (Meteor Falls), Meditite (Mt. Pyre outside), Galarian Corsola (surfing in the flooded Abandoned Ship rooms; Cursola at 38). Poliwag, Snover, Roggenrola, Igglybuff, Azurill, Chingling and Bonsly hatch from Day Care eggs of their evolved forms.

**Forms.** Cosmetic forms are not counted as separate species. Supported: Toxtricity (Amped, Low Key), Sinistea and Polteageist (Phony, Antique), twelve Alcremie forms, Shellos and Gastrodon (West, East), Pumpkaboo and Gourgeist (Average, Super), Lycanroc (Midday, Midnight, Dusk), the Alolan and Galarian forms above. Deerling and Sawsbuck are Spring Form only (Hoenn has no seasons). Battle forms show on both players' screens in co-op battles: Eiscue (Ice Face to Noice Face), Morpeko (Full Belly to Hangry Mode) and Cramorant (Gulping) were tested this build, Cherrim in the last one. Cramorant's Gorging form and Castform's weather forms use the same code but were not played.

### Third roster batch (build 0xC0170139)

This batch completes the ordinary roster: every National Dex species except the legendary, mythical, Ultra Beast and Paradox ones (125, deferred) can be obtained on one save, alone: **900 of 900**. With the Hoenn legendaries already in the game that is 908 species. The whole roster, every batch, is in [COOP_ROSTER_ALL.md](COOP_ROSTER_ALL.md) (searchable: species, National and Hoenn numbers, batch, how and where, unlock, levels and rates, evolution resources and where they are sold); this batch alone in [COOP_ROSTER_BATCH3.md](COOP_ROSTER_BATCH3.md).

- **Wild.** 143 slots in 69 tables on 58 maps: grass, caves, surfing, the three rods, Rock Smash (Route 111 and 114 rocks), the Safari Zone (its post-game North-East and South-East areas too) and Altering Cave. Each new species took a duplicate slot or a slot of a widespread species that is still in at least three tables (Poochyena 8 → 3 tables, Zigzagoon 6 → 4, Whismur and Carvanha 4 → 3, Shuppet 10 → 9); no species became obtainable later than before. Rates are 4% to 30% of a table.
- **Regional forms.** 54 are obtainable (the 7 earlier ones plus 47). Hoenn has no other regions, so three places stand in for them, for evolution only: **Alola** = Dewford Town, Granite Cave, Routes 106 and 107; **Galar** = Route 114; **Hisui** = Mt. Pyre (inside and out). Evolving a Pokémon there gives the regional form where one exists (Pikachu with a Thunder Stone in Dewford: Alolan Raichu; on any other route: Raichu); elsewhere the ordinary form. Wild regional forms: Alolan Rattata and Meowth (Route 115), Diglett and Geodude (Granite Cave), Sandshrew (Shoal Cave); Galarian Meowth (Route 115), Ponyta and Farfetch'd (Safari Zone), Slowpoke (surf, Route 120), Stunfisk (surf, Route 114), Darumaka (Shoal Cave), Yamask (Sandburied Ruins); Hisuian Growlithe (Jagged Pass), Voltorb (New Mauville), Qwilfish (Route 129, Super Rod), Sneasel (Meteor Falls), Zorua (Mt. Pyre 6F); Paldean Wooper (surf, Petalburg) and the three Paldean Tauros breeds (Safari Zone North). Eggs hatch in the ordinary form unless the regional parent holds an Everstone (Gen 9 rule).
- **Evolution resources.** The Lilycove evolution clerk (3F) now sells every item a new evolution needs: the Sweets, Soothe Bell, Oval Stone, Everstone, Up-Grade, Dubious Disc, Electirizer, Magmarizer, Protector, Razor Claw and Fang, the pots and teacups, Sachet, Whipped Dream, Leek, Galarica Cuff and Wreath, Black Augurite, Peat Block, Metal Alloy and the four nectars. No item is tied to a quest.
- **Trade evolutions.** Each also works with a Linking Cord (sold); held-item ones (Steelix, Scizor, Electivire, Slowking…) by using the item. Karrablast and Shelmet evolve with a Linking Cord while the other one is in the party.
- **Places, times, moves.** Tyrogue by its Attack and Defense at level 20 (Hitmonlee, Hitmonchan, Hitmontop); Burmy's cloak follows the place of its last battle (grass: Plant, cave or sand: Sandy, a building such as Mt. Pyre: Trash), then a female becomes that Wormadam and a male Mothim; female Combee: Vespiquen; Espurr: male or female Meowstic; Happiny with an Oval Stone by day; Mantyke with a Remoraid in the party; Yanma, Tangela (Ancient Power), Lickitung (Rollout), Mime Jr. (Mimic), Dunsparce (Hyper Drill), Hisuian Qwilfish (Barb Barrage) by knowing the move; Goomy's Sliggoo becomes Goodra at 50 in rain or fog (Route 119's rain, the fog on Mt. Pyre's summit and in Rusturf Tunnel); Ursaring with a Peat Block at night on Mt. Pyre (the stock full moon cannot be seen here).
- **Unusual ones.** Primeape that has used Rage Fist 20 times evolves on its next level (Annihilape); Galarian Farfetch'd that lands three critical hits in one battle evolves when it ends (Sirfetch'd; a Leek and Focus Energy help). Both count in battles fought alone only: co-op battles are link battles, and like stock link battles they do not count. Rellor that has walked 1000 steps in the lead evolves on its next level (Rabsca); the count starts again when the lead changes or the game is switched off. Galarian Yamask that has lost 49 HP or more evolves into Runerigus when you touch the cracked carving by the east wall of the Sandburied Ruins' lower room (the stock trigger is a Galar landmark); with no such Pokémon the carving says so. Duraludon uses a Metal Alloy (Archaludon); Poltchageist uses an Unremarkable Teacup, its Artisan form (Mt. Pyre 2F) a Masterpiece Teacup. Dunsparce gives a three-segment Dudunsparce 1 time in 100 (personality, as in stock). Inkay becomes Malamar at level 30 (the stock rule needs the console held upside down).
- **Galar fossils.** The Fossil Maniac's daily list adds the four pairs (Bird + Drake, Bird + Dino, Fish + Drake, Fish + Dino); DEVON revives a pair as Dracozolt, Arctozolt, Dracovish or Arctovish.
- **Forms.** Oricorio changes style with a nectar (all four sold). Burmy and Wormadam: three cloaks. Meowstic: both. Vivillon is the Icy Snow pattern only (the pattern is the console's region in stock); Flabébé, Floette and Florges the Red Flower; Furfrou is not trimmed; Minior's core is red; Squawkabilly is Green Plumage. Unown letters come from personality, as in stock Emerald.
- **Hints.** The field researcher's guide has a fifth group, FINAL ARRIVALS (25 entries), and the Pokédex AREA page gives an acquisition hint for 494 species with no wild area.
- **Pokédex.** The Hoenn list has 910 entries (all unique): every obtainable species, including the 28 that stock Emerald left National-only (the Johto starters, Ditto, Mareep, Aipom, Pineco, Gligar, Snubbull, Shuckle, Teddiursa, Stantler, Smeargle, Miltank and their evolutions), plus Jirachi and Deoxys, the only two that cannot be obtained (event-only, as in stock). The diplomas now ask for every ordinary Pokémon in their list; legendary, mythical, Ultra Beast and Paradox entries are optional, so a save that saw only one of Latias and Latios can still finish.
- Not in this build: new legendary, mythical, Ultra Beast or Paradox encounters; custom Megas (Mega Milotic); any balance changes beyond the revamps already listed (all of them are kept: 328 species checked, 0 differences).

## People, trades, and errands

These work for one player. A link partner is not required, and none of them hand out a story item or a badge.

In-game trades copy the level of the Pokémon you give. Each trade happens once.

| Who | Where | They want | They give |
| --- | --- | --- | --- |
| Lass | Route 102 | Zigzagoon | Lechonk, nicknamed Oinker |
| Woman | Petalburg Woods | Wurmple | Tarountula, nicknamed Webby |
| Black Belt | Jagged Pass | Numel | Charcadet, nicknamed Cinder |
| Expert | Meteor Falls 1F | Bagon | Jangmo-o, nicknamed Scales |
| Swimmer | Route 124 | Wingull | Finizen, nicknamed Bubbles |
| Man | Route 110 | Electrike | Pawmi, nicknamed Spark |
| Black Belt | Granite Cave 1F | Aron | Nacli, nicknamed Salt |
| Camper | Route 113 | Spinda | Bramblin, nicknamed Tumble |
| Woman | Route 118 | Ralts | Indeedee, nicknamed Helper |

Show the person a matching Pokémon from your party or your PC. The errand is once each. A full Bag refuses the item, and they ask again.

| Who | Where | Show them | Reward |
| --- | --- | --- | --- |
| Expert | Granite Cave 1F | Nacli or Glimmet | Hard Stone |
| Lass | Route 110 | Pawmi | Magnet |
| Hex Maniac | Route 113 | Bramblin | Soft Sand |
| Psychic | Route 118 | Indeedee, either | Destiny Knot |
| Woman | Route 119 | Toedscool | Big Mushroom |
| Hex Maniac | Mt. Pyre 1F | Greavard, Mimikyu, or Dreepy | Spell Tag |
| Pokéfan | Safari Zone, south | Fidough or Cyclizar | Soothe Bell |
| Expert | Shoal Cave, low tide inner room | Cetoddle or Abomasnow | Never-Melt Ice |
| Expert | Victory Road 1F | Gimmighoul, Tinkatink, or Frigibax | Nugget |
| Bug Catcher | Petalburg Woods | Shroodle | Poison Barb |
| Woman | Route 104 | Applin | Sweet Apple |
| Fisherman | Route 106, on the sand | Crabrawler | Protective Pads |
| Boy | Mauville City | Plusle, then Minun | Cell Battery |
| Hiker | Meteor Falls 1F | Noibat | Sharp Beak |
| Camper | Route 111 desert | Sandile or Helioptile | Heat Rock |
| Expert | Magma Hideout 1F | Varoom | Air Balloon |
| Swimmer | Underwater Route 126 | Wiglett | Dive Ball |
| Fisherman | Lilycove City | Dondozo | Mystic Water |

The Mauville boy wants Plusle on the first visit and Minun on the next. The Cell Battery is given only after both. A full Bag on the second visit keeps the errand open.

Two people only talk. A girl in Oldale explains the Pokédex map. A scientist in Mauville explains the same-spot pairs, including Indeedee.

## Types and evolution items

Fairy is part of the battle rules. The type chart, the summary icons, and the Pokédex search all include it. Clefairy, Jigglypuff, Marill, Ralts, Mawile, and Impidimp are Fairy. Charm and Baby-Doll Eyes are Fairy moves, so an Eevee that knows one can become Sylveon by friendship.

Fire, Water, Thunder, Leaf, Moon, and Sun Stones were already placed in Hoenn. These later evolution items were not. Each one below is a single Poké Ball. Metal Coat, Dragon Scale, Prism Scale, and Reaper Cloth can be used from the bag. The same stones, apples, and held items are also sold by the evolution clerk on the third floor of the Lilycove Department Store, and every evolution item is at the Trainer Exchange in Rustboro (see below).

Poké Marts sell more balls as you go east. Oldale adds Great Balls once the Pokédex is running. Petalburg and Rustboro add Nest Balls, and Rustboro also adds Net Balls. Mauville adds Ultra, Repeat, and Timer Balls, plus a Thunder Stone and a Moon Stone. Verdanturf sells a Leaf Stone and a Sun Stone. Fallarbor sells a Moon Stone. Lavaridge sells a Fire Stone. Fortree adds Dusk Balls and a Shiny Stone. Lilycove's second floor sells the full ball case, including Dive, Quick, Heal, Luxury, and Premier Balls. Mossdeep adds a Dawn Stone and an Ice Stone. Dewford and Pacifidlog have no mart, so a clerk in each Pokémon Center sells a small stock. Pacifidlog's clerk also sells Dive Balls, a Water Stone, and a Prism Scale.

| Item | Where | What it evolves |
| --- | --- | --- |
| Tart Apple | Route 104 | Applin into Flapple |
| Sweet Apple | Route 104 errand | Applin into Appletun |
| Syrupy Apple | Route 123 | Applin into Dipplin. The next level, after it learns Dragon Cheer, becomes Hydrapple |
| Ice Stone | Shoal Cave, low tide ice room | Cetoddle into Cetitan |
| Dawn Stone | Meteor Falls 1F | A male Kirlia into Gallade |
| Dawn Stone | Shoal Cave, low tide inner room | A female Snorunt into Froslass |
| Shiny Stone | Route 117 | Roselia into Roserade |
| Reaper Cloth | Mt. Pyre 1F | Dusclops into Dusknoir |
| Prism Scale | Route 119 | Feebas into Milotic |
| Metal Coat | Granite Cave B2F | Onix into Steelix |
| Dragon Scale | Route 124, on the sandbar | Seadra into Kingdra |

Crabrawler and Eevee can also become Crabominable and Glaceon by leveling in the Shoal Cave ice room. Eevee can become Leafeon by leveling in Petalburg Woods.

## Trainer Exchange (Rustboro)

A new building in the south of Rustboro, down the road from the Pokémon Center and the Poké Mart toward Route 104: from the Mart's door go seven steps east and seven south (door at (23,52), a sign on each side). It is open from the start, before Roxanne. Inside, a guide by the entrance and a directory on the back wall; five counters:

| Counter | Where | What |
| --- | --- | --- |
| Moves & TMs | Left wall | BUY TMs; TEACH A MOVE (91 tutor moves, chosen by type); RELEARN A MOVE (level-up and evolution moves, ₽1,000); EGG MOVE (any egg move of the family, ₽5,000, from 3 BADGES) |
| Battle items | Front, left | Berries, type boosters, incenses, then Leftovers, Choice items, Life Orb, Focus Sash, Assault Vest, Eviolite and more |
| Evolution | Front, middle | Every evolution item the roster uses: stones, sweets, apples, trade substitutes (Linking Cord, Metal Coat, King's Rock, …), held items |
| Training & Nature | Front, right | EXP candies, feathers, vitamins, EV-lowering berries, mints, PP Up/Max, Ability Capsule and Patch; RESET EVs (₽2,000, from 1 BADGE); HYPER TRAINING (one stat ₽15,000 from 7 BADGES, all six ₽60,000 as CHAMPION, Lv. 50+); HOW IT WORKS |
| Breeding | Right wall | Everstone, Power items, Destiny Knot, Mirror Herb; the BREEDING GUIDE |

What you can buy depends on **your own** BADGES (0, 1, 3, 5, 7, CHAMPION) and, for the Ability Capsule, the Twin Trial. A partner's badges never unlock anything for you. A partner can still give you what they bought (partner menu, GIVE ITEM), with no badge rule on receiving. COMING SOON at each counter names what the next step opens (a few names, then "and N more"). The full stock, prices and unlocks are in [COOP_EXCHANGE_STOCK.md](COOP_EXCHANGE_STOCK.md).

- **Prices are one table**: an item costs the same in every shop that sells it (Lilycove, Slateport, Mauville's Power Shop, city marts), and sells for a quarter.
- **TMs are single-use** in this ROM, so they are cheaper than reusable TMs would be (₽2,000–12,000). The TM numbers and old saves are unchanged; extra moves are taught by the tutor instead.
- **Tutor moves** cover all 18 types with the TMs; no signature moves. A move is offered to a Pokémon only if its species can learn it (the same rule as TMs). You choose the Pokémon from the party or a box; you pay once it has learned the move, and nothing if you cancel.
- **Co-op**: both players can use the same counter at the same time, each in their own shop with their own money and bag. To the partner, the shopper shows the menu icon; the partner menu says they're busy instead of sending a request.

### Nature, EVs, IVs and levels

- **Mints** change which stats the nature raises and lowers. The Pokémon's own nature (shown with the mint's) is unchanged, and that is what an Everstone passes to an egg.
- **Vitamins** (HP Up, Protein, Iron, Calcium, Zinc, Carbos) add 10 EVs; **feathers** 1. 252 a stat, 510 in all. They are EV items, not IV items. **Pomeg, Kelpsy, Qualot, Hondew, Grepa and Tamato Berries** take EVs away; **RESET EVs** clears all six at once.
- **Hyper Training** makes a stat count as if its IV were 31. The IV itself is kept, so it is not passed on by breeding. There is no Bottle Cap item; Hyper Training is the counter's service.
- **EXP candies** stop at the level cap (Rare Candy too). An EXP candy that reaches the cap says how far it went and that the rest of its EXP was lost; at the cap, candies can't be used and are kept. Obedience is unchanged: your own Pokémon follow the met-level rule; one from a trade may disobey above your badge level, as with battle EXP.

### Budget check

Using the money the main journey's players actually had:

| When | Money | A sensible basket |
| --- | --- | --- |
| Before Roxanne | ₽7,100–7,300 | TM Bullet Seed ₽2,500 + Low Kick tutor ₽2,000 + 2 Oran Berries ₽400 + 5 EXP Candy XS ₽1,000 = ₽5,900, ₽1,300 left for balls and Potions |
| 4 badges | ₽72,000–74,000 | Leftovers ₽12,000 + two TMs ₽11,000 + two tutor moves ₽13,000 + three vitamins ₽9,000 + a Power item ₽7,000 + an egg move ₽5,000 + an EV reset ₽2,000 = ₽59,000 |
| Before the League (8 badges) | ₽221,000–232,000 | Choice Scarf and Life Orb ₽40,000 + Focus Sash and Assault Vest ₽30,000 + four mints ₽32,000 + four single-stat Hyper Trainings ₽60,000 + Close Combat and Moonblast ₽33,000 = ₽195,000 |

Full optimisation of a team (six Hyper Trainings of all six stats, ₽360,000) is a post-League goal. Selling gives a quarter of the price, so nothing bought can be sold at a profit; berries from your own trees sell for a little more than before (a quarter of the new berry prices), which is small next to trainer payouts.

## Breeding and the Day Care

The Day Care on Route 117 works as in the main series, on each player's own save: your deposits, your steps, your eggs, your fees. Two players can use the Day Care lady and man at the same time; each gets their own conversation. Only your own steps count toward eggs and hatching. A guide in the Day Care (and at the Exchange's breeding counter) explains the rules:

- The egg is the mother's first stage, or the other parent's with Ditto. Babies hatch without incense (Azurill from Marill, and so on). Undiscovered-group Pokémon don't breed.
- Ability: usually the mother's (or the non-Ditto parent's).
- Nature: an Everstone passes the holder's own nature, not a mint's.
- IVs: three from the parents, five with a Destiny Knot; a Power item passes its stat's IV. Hyper Training isn't passed on (the parents' own IVs are used).
- Egg moves known by either parent pass down. A parent holding a Mirror Herb, with a free move slot, learns the other's egg moves when the two are left together.
- Regional forms: the mother's form passes on when she holds an Everstone; otherwise the egg hatches in its usual (Hoenn) form.
- Day Care Pokémon gain EXP up to the level cap and may learn level-up moves, replacing old ones as in the main series (RELEARN A MOVE at the Exchange gets them back).
- Eggs can't battle, and can't be traded or given (the partner menu says so).
- Ditto lives in the Desert Underpass after the Hall of Fame.

## Level cap

Default on. The steps are 18 before the first badge, then 24, 30, 36, 42, 48, 54, 60 after badges 1-7, 66 after the eighth, 76 once the League's door guards have let you in (kept after a loss or leaving), and 80 as Champion. EXP, Rare Candy, EXP candies, the day care, and EXP Share stop at the cap. A Pokémon at or below the cap always obeys. A Pokémon already above the cap is not lowered. The party screen shows the cap next to the level. Both saves have to agree on the cap setting before a link comes up. Gym levels do not scale; in a linked fight the opponents are picked for the lower of the two players' progress.

## Mega Evolution

- **Mega Bracelet.** After Norman's badge, Steven meets you on Route 118 (the scene where he talks about Mega Evolution) and gives you a Mega Bracelet; it goes to Key Items. Each save gets its own, in its own scene. A save that saw that scene before this build, or had a full bag then, gets it from the MEGA STONE clerk at the Rustboro Trainer Exchange (back right corner) once it has 5 badges.
- **Using it.** In battle, on the move menu, press START to turn the Mega trigger on (the icon beside your Pokémon lights up), then choose the move. The Pokémon must hold its own Mega Stone. One Mega per trainer per battle: each player has their own, and so does each opposing trainer. Changing your choice (B at "Ready. Waiting...") before the turn starts gives the Mega back. In a linked battle you need your own bracelet; your partner's doesn't count.
- **After the battle** a Mega is its own species again, with the same moves, EXP, held stone and identity; its HP is unchanged.
- **Stones.** Every Mega of a Pokémon you can get has a stone ([COOP_MEGA_MANIFEST.md](COOP_MEGA_MANIFEST.md)): 12 sold from 5 badges (the starters', the companions', Gardevoir, Gallade, Lucario, Aggron, Manectric, Altaria), 6 more from 7 badges, 7 as Champion, four quest rewards (Coastal Cleanup: Dragalgite; Desert Caravan: Garchompite; Power Problems: Eelektrossite; Shoal Cave rescue: Froslassite; a save that finished the quest earlier gets it the next time it talks to the quest's person), and the rest in item balls around Hoenn. A stone you found or earned is sold again at the MEGA STONE counter from 5 badges. WHERE TO FIND at that counter lists the ones you haven't got with a hint.
- **Item balls** (the stones and 31 other items, listed in the manifest) are personal: each save picks up its own, and a partner standing at the same ball gets their own too. With a full pocket the ball stays.
- Z-Moves, Dynamax, Terastal and Primal Reversion stay off. Mega Milotic and other custom Megas are not part of this build.

## Patch

After `make`:

```sh
sh tools/coop_make_patch.sh emerald-coop.bps
```

The script checks the clean ROM hash. It uses `flips` for BPS, or `beat` for UPS. If neither is installed it prints the command and does not invent a patch. Apply the patch to a clean ROM with the same tool. Do not put the patched ROM in this repo.

## Local regression checks

The headless two-core checks and their fixture scope are documented in
[tools/coop_link_harness/README.md](../tools/coop_link_harness/README.md).
The required upstream `tools/compresSmol` source is included so a fresh clone
can build; its generated executables remain ignored.

## Two-player script

This is the pass to run on two PCs. It was not executed in the environment that produced this tree. A passing host protocol test is not that pass.

1. Patch two copies from the same clean ROM. Put a different `.sav` next to each copy.
2. mGBA 0.10.5: open both, File, New Multiplayer Window. RetroArch 1.22.2, gpSP core 1.1.0: set Link Cable Connectivity to Link Cable - Pokémon Gen3 (not Auto), then host netplay. The other PC joins by UPnP, a direct IP, or Tailscale, in that order. Write down which one you used. A relay is not part of this ROM. The pass is the Route 101 sprite plus one gym. Lockstep netplay of one screen is a different feature and does not count.
3. Cold boot. CO-OP on both games. Walk to different maps. Confirm no sprite. Meet on Route 101. Confirm the sprite and that the CO-OP menu names that map. Walk through a door. Confirm the sprite vanishes and comes back. The Twin Beacon is off after every reconnect. One player asks, the other says no, and an ordinary trainer stays solo. Both agree, and an ordinary trainer can be a co-op battle. Ask to turn it off during a gym. The gym finishes in the mode it started, then the Beacon clears. The item is still in the bag.
4. Rescue Birch together. One person opens the bag. Confirm that person gets the highlighted Pokémon and the partner gets the other. Confirm both games enter one double against two Poochyena, each using their new Pokémon. Win, lose, or both press RUN. Confirm both arrive in the lab, each receives a Twin Beacon, and each can walk after the nickname. Fill the Key Items pocket first and confirm Birch asks you to make room instead of freezing you.
5. Clear Rustboro together. Pull the cable during the gym. Confirm no badge and both parties restored. Reconnect. Clear it. Confirm both saves have the Stone Badge and the TMs stayed with whoever finished the speech.
6. First three: each player carries six. Start a co-op battle. Confirm only slots 1–3 appear in battle, on the party screen, and in the switch prompt. Faint all three on one side and confirm the partner keeps fighting. Win it. Confirm slots 1–3 kept their damage and PP, slots 4–6 match how they were before, and the CO-OP menu still shows the link. Run a solo battle and confirm all six can switch in. Faint all of one player's first three outside battle, with a healthy fifth slot, and confirm a gym refuses on both games with the right message. Pull the cable mid-battle and confirm all six come back as they were.
7. Trade one Pokémon from the party screen. Save both games. Reload. Confirm the trade stuck and the parties were not swapped twice.
8. Turn the cap off on one save, try to link, confirm it refuses. Turn it back on and link.
9. Build mismatch: one older patch against this one. Confirm the link stops.
10. Route 102 wild, Beacon on, both on the route. Step in grass and confirm both games enter one wild double with the same two Pokémon. Catch the left one and leave the right one, then finish. Confirm only the thrower has it, in the party or the PC, with a nickname prompt and a caught Pokédex entry. Both throw at the same Pokémon on one turn: one owner, the second ball is gone, no copy. Throw at different ones: both are caught, one each. Pull the cable during the shakes: nobody gets it. Fill the party and confirm the catch goes to the PC. Fill the PC as well and confirm the ball refuses and the fight goes on. Choose RUN on one game only and confirm the battle continues. Choose RUN on both and confirm it ends on both. Repeat on Route 101 and confirm an accepted invitation starts a shared double there too.
11. Rick and Allen. Fresh pair of saves. Beat Rick solo on one save with the cable off. Confirm Allen still fights. Link, Beacon on, both on Route 102. Walk into Allen on that save and confirm a solo fight with no partner prompt. On a second pair of fresh saves, walk into Rick, say yes on both, and win. Confirm both saves have both flags. Repeat and pull the cable mid-fight: no flags. Repeat with the partner in a menu: solo fight against the one you walked into.
12. Late game, on a second pass: Groudon and Kyogre, pull the link after the ball shakes and before the catch commits, confirm nobody has the Pokémon. Catch it on a retry and confirm only one save owns it.
13. Story, with both players on the same map. Beat the Route 110 rival and confirm the guest received the Dowsing Machine and the rival does not challenge them again. Beat Maxie on Mt. Chimney and confirm Team Magma is gone on the guest's save without knocking any later town state backward. Beat Archie and confirm the guest lands on Route 128, then the host arrives there after the cutscene. At Mossdeep Space Center, confirm the fight is Maxie and Tabitha against both of you, and that Steven does not replace either player.
14. Post-game. With the Beacon on, enter the Battle Frontier on both games. Fight a facility trainer and confirm it stays a single-player battle. A gym rematch still asks for both of you, and the ace levels are the rematch levels, not a scaled set.
15. Two PCs, once. Same ROM on both. gpSP Gen 3 link as in step 2. Meet on Route 101 and confirm the sprite. Clear one gym. Record UPnP, direct IP, or Tailscale. If that session fails, write what failed. Do not add a relay in its place until that note exists.

## Playing alone

No part of the game needs a second player. A game that is **not linked** (never linked, or after UNLINK) plays alone:

| Fight | Linked (partner on the map) | Not linked |
| --- | --- | --- |
| Birch's rescue, Route 101 | Both players, two POOCHYENA | You pick a case, get its starter, and fight one POOCHYENA |
| Gyms, Elite Four, champion (talk to either trainer) | Co-op double, each side's first three | Double battle against both trainers with your whole party |
| Rival twins (Route 103, Rustboro, Routes 110 and 119, Lilycove) | Both twins against both players | A single battle against your own twin (Soren if you opened Birch's bag, Mira if you got the companion; recorded in your save at the rescue), with that twin's solo team |
| Maxie and Tabitha, Space Center | Co-op double | The stock fight with Steven as your partner |
| Other Aqua/Magma bosses, Wally | Co-op | The stock single battle |
| Rayquaza, Groudon, Kyogre, Regirock, Regice, Registeel | Shared if both agree (one catch, one each), otherwise alone | Stock single battle (level 70; the Regis 40) |

While linked with your partner somewhere else, these fights wait and say so: come back together, or choose UNLINK in the CO-OP menu to face one alone. A win alone counts on your save only; linking again does not copy it to your partner, who can still clear it alone or with you.

## Small conveniences

- The start menu's party screen reopens on the Pokémon you last highlighted.
- A second visit to the same Pokémon Center nurse in one session heals at once ("Welcome back! I'll heal them right away.") without the question.
- The Bag already remembers its pocket and position, and an expired Repel already offers another one from the bag (stock expansion features, unchanged).
- **CO-OP → SETTINGS** (your own, not your partner's):
  - LOW HP ALARM: 3 BEEPS (default; once per low-health episode) or OFF.
  - EASY FISHING: a bite waits for A; B lets it go.
  - SHINY CHECK: RUN from a wild battle with a shiny still there asks "A shiny Pokémon is still here. Leave?".
  - CATCH RESULT: after a catch, the species, NEW in your Pokédex, nature, ability and where it went.
- A shiny Pokémon's battle box shows a star after its name.
- **Trainer Bond:** after the STONE BADGE, the BOND MENTOR in Rustboro's Pokémon Center lets you choose one Partner Pokémon. Battles and travel together earn it Bond XP, and its rank adds +5, +10 or +15 to each stat.
  - The bonus is off in the Battle Frontier, Battle Tents, practice and link battles against a friend.
  - Every Pokémon keeps its own Bond XP, even after you switch partners.
  - Details: [COOP_BOND.md](COOP_BOND.md).

## Known limits

- With one boy and one girl, Littleroot's two houses are swapped between the screens: the rival twins live in May's house on the boy's save and in Brendan's house on the girl's save, as in the stock game. Two players of the same gender see the same house.
- Only the twin who lives in the bedroom speaks there. In the later rival scenes the scene's own rival speaks and the second twin stands beside them and follows; their own lines are the Route 103 introduction and the talk line.
- “Partner ready.” is shown on the link master only.
- Route 101 across the internet was not proven here. The RetroArch steps above are the test, not a result.
- Coordinates in the link frame are 8-bit. A map wider than 255 tiles wraps the partner sprite.
- The acro bike is the extra bit in the facing nibble. Mach bike and acro bike can both be shown. Surf wins over a bike.
- After a shared gym or Elite Four win, the guest receives the same story flags and the way forward. The person who talked to the leader still gets the TM. The guest can ask the leader for that TM afterward.
- A shared champion win sends the guest to the Hall of Fame so their party is recorded too.
- The rescue fight waits until your partner is on Route 101 with their menu closed. Press B to back out and try again. After the fight, each game walks itself to the lab.
- Co-op battles field three Pokémon per player, through the stock link-multi exchange. In wild and ordinary trainer battles those are your first three (slots 4–6 sit out); shared story fights and the DUO TOWER let you choose the three on the team screen.
- Mega Evolution is on (see [Mega Evolution](#mega-evolution)); Z-Moves, Dynamax, Terastal and primals stay off.
- Id 865 is unused on purpose. It is the in-game partner id for Steven, and a normal trainer cannot live there.
- Invalid link frames are dropped. One bad packet does not disconnect you.
- The partner mark is a sprite like the partner: in a dark cave without FLASH, a partner outside your light circle and their mark are both hidden.
- A shared legendary battle opens with the link-multi VS screen, whose opponent card is blank (a wild Pokémon has no trainer card).
- Joining conversations is conservative: a script is joinable only if every branch it can take shows text, asks yes/no or moves people; shops, gifts, battles, warps, special code and scene scripts are not, even where sharing would have been harmless.
- FOLLOW is on foot only and stops at grass, caves and other wild terrain (so you never walk into an encounter you didn't choose). It goes through ordinary doors, stairs and ladders that lead to your partner's map, and stops before any other warp. Events on a connected map's tiles are checked only once you are on that map.
- In battle, only moves, switches and RUN can be changed after choosing; items cannot.
- A BADGE both players won together in a co-op gym battle is recorded on both saves; on LINK UP a save that lacks it gets it back. That is the only way a partner can add to your badges, and so to your Exchange stock.
- A partner busy at an Exchange counter or the Day Care is reported as "shopping".
- A gym, rival, Elite Four or story fight waits while your partner has a menu screen or a message open (up to 30 seconds); if they stay busy you are told so and nothing starts.
- In a co-op battle your Pokémon and your partner's can't take, give or swap items (Trick, Switcheroo, Bestow, Thief, Covet fail between you).
- Strength boulders, rocks and trees are each save's own; a boulder your partner pushes doesn't move in your game.
- In Secret Bases, Battle Pyramid floors and Trainer Hill floors your partner isn't shown (each room is built from its own save).
- Losing a co-op battle costs no money; each of you wakes at your own last heal place.

## Fixes in build 0xC0170136

- **Long co-op battles no longer freeze on a move animation.** Opening the Bag while linked (each ball thrown counts) left an extra copy of the co-op background task each time; enough copies filled the game's task table, and a move animation that needed several tasks (Rayquaza's HYPER VOICE) waited forever on both screens.
- **Saves made in the background** (trades, gifts, shared legendary rewards) could be written from stale memory if you walked through a door or a battle started while the save was still being written; such a save crashed on CONTINUE. The save now starts again from the right place.
- **No partner "ghost" after loading** a save made while linked.
- **Partner colours in fog** (Terra Cave, foggy routes): the partner no longer looks pale for the whole visit.

## Fixes in build 0xC017012C

- **Rival in the bedroom doorway (guest only).** After the rescue, the player who waited on Route 101 reached the lab through `Coop_EventScript_WarpToLab`, which skipped Route 101's step that hides the rival in their bedroom (and the map-name popup flag). The bedroom only repositions the rival before the rescue, so on that save the rival stood on its template tile, the stair warp, and blocked the only exit. Unlinking did not help because the state was in the save. Both roles now share one rescue script, and both bedrooms re-hide the rival from the rescue until Lilycove on entry and on load, which repairs saves made before the fix without changing other progress.
- **Route 103 for the guest.** The guest's follow-up after the first rival fight set the defeat flag but not the lab state, so Birch never offered the guest the Pokédex. The follow-ups for Route 103, Rustboro, and Lilycove now write the talker's state, raised only.

## Credit

Pokémon and Pokémon Emerald are Nintendo, Game Freak, and Creatures. This project does not ship their ROM.

Engine: pret pokeemerald, and RHH pokeemerald-expansion 1.17.1.

Art: Mira's and Soren's overworld sheets and trainer pictures are edits of Emerald's own May and Brendan graphics (from the pret decompilation): the bodies and animation frames are kept so scale and motion match the game; the heads (Mira's sun hat, Soren's spiked hair and headband) and all palettes were redrawn for this project by `tools/coop_art/rivals.py`. The outfit colours are palette sets written by `tools/coop_art/outfits.py`. No third-party sprite packs are used.
