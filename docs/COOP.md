# Emerald Co-op

Two people, two saves, one Hoenn. Each game keeps its own party, bag, money, and catches. The host only hosts the link. Gyms, the rival pair, the Elite Four, the champion pair, and Groudon with Kyogre cannot be cleared alone.

This is a non-commercial fan patch. It does not include a ROM.

Based off RHH's pokeemerald-expansion 1.17.1
https://github.com/rh-hideout/pokeemerald-expansion/
Pinned commit `bb1093a7c72d3961d9a37d8e02ad2abc2c866682`.

Build id baked into the link handshake: `0xC0170117`.

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

No custom server. If the public RetroArch relay is down, try UPnP, then a direct IP, then Tailscale. A room-code server is not part of this build.

## How a session works

1. Both games are the same build. Open the start menu, choose **CO-OP**, and say yes to the link. The other game does the same. mGBA has to already be in a multiplayer window. RetroArch has to already be in a Gen 3 link lobby.
2. The first exchange compares the build id and the level cap. Either mismatch aborts. Change the cap from CO-OP before linking. The cap is on unless you turn it off.
3. You can walk different maps. The partner sprite appears only on the same map, including through doors. The CO-OP menu names their map when they are somewhere else.
4. Talk to the sprite to see where they are. From the party screen, while linked, **TRADE** offers one Pokémon. Both sides have to agree. Pulling the cable before both sides commit leaves both parties alone. You cannot trade your last conscious Pokémon.
5. Disconnect from the CO-OP menu, or from a dropped cable. Parties are not copied across. Reconnect exchanges badges earned together, the rival stage, and who owns Groudon, Kyogre, and Rayquaza.

## Story rules

- Birch on Route 101 waits until both of you are on that map and linked. The lab will not open the cases until both of you are in the lab.
- One case, then you split the two Pokémon. Cinder is Torchic and Houndour. Tide is Mudkip and Wingull. Thorn is Treecko and Shroomish. If you both claim the same one, the host keeps the claim and the guest gets the other.
- Soren and Mira take the case that beats yours. They show up in the old Brendan and May fights. Both of you have to be there.
- Gyms are two leaders, three Pokémon each. You cannot start one alone. Ace levels are 16, 22, 28, 34, 40, 46, 52, 58. Rematches are a second pair around level 70.
- Most trainers lean doubles while you are together on their map. A trainer whose id is divisible by 4 stays a single. Magma and Aqua grunts are a pair. Leaders, the Elite Four, the champion, and rivals (Wally included) always ask for both of you.
- The Elite Four and the champion are the same 3+3 shape. The champion pair is Steven and Wallace. The guest does not watch the Hall of Fame cutscene. Both saves still set the champion flag.
- Terra Cave and Marine Cave both start Groudon and Kyogre together. Their weather overwrites itself. Sky Pillar is Rayquaza. One catch. The person whose ball lands keeps it. The other person can get it only by the party trade. A KO still clears the story flags.

Each player brings a party of 6 (`COOP_PARTY_SIZE` in `include/coop.h`). Enemy gyms are 6 either way. If a link battle desyncs on the extra Pokémon, set `COOP_PARTY_SIZE` to 3 and rebuild. That fallback is a decision, not a silent cut. This tree still sends 6.

Megas, Z-Moves, Dynamax, and Terastallization are off, including Primal Reversion. Species tables are unchanged.

## Level cap

Default on. The steps are 16, 22, 28, 34, 40, 46, 52, 58, then 66 for the Elite Four, then 75 after the champion. EXP, Rare Candy, the day care, and EXP Share stop at the cap. A Pokémon already above the cap is not lowered. The party screen shows the cap next to the level. Both saves have to agree before a link comes up. Gym levels do not scale.

## Patch

After `make`:

```sh
sh tools/coop_make_patch.sh emerald-coop.bps
```

The script checks the clean ROM hash. It uses `flips` for BPS, or `beat` for UPS. If neither is installed it prints the command and does not invent a patch. Apply the patch to a clean ROM with the same tool. Do not put the patched ROM in this repo.

## Two-player script

This is the pass to run on two PCs. It was not executed in the environment that produced this tree. A passing host protocol test is not that pass.

1. Patch two copies from the same clean ROM. Put a different `.sav` next to each copy.
2. mGBA 0.10.5: open both, File, New Multiplayer Window. RetroArch: both sides on gpSP 1.1.0, Link Cable - Pokémon Gen3, relay or UPnP or a direct IP.
3. Cold boot. CO-OP on both games. Walk to different maps. Confirm no sprite. Meet on Route 101. Confirm the sprite. Walk through a door. Confirm it vanishes and comes back.
4. Rescue Birch together. Pick one case. Split the two starters. Fight the first rival pair.
5. Clear Rustboro together. Pull the cable during the gym. Confirm no badge and both parties restored. Reconnect. Clear it. Confirm both saves have the Stone Badge and the TMs stayed with whoever finished the speech.
6. Trade one Pokémon from the party screen. Save both games. Reload. Confirm the trade stuck and the parties were not swapped twice.
7. Turn the cap off on one save, try to link, confirm it refuses. Turn it back on and link.
8. Build mismatch: one older patch against this one. Confirm the link stops.
9. Late game, on a second pass: Groudon and Kyogre, pull the link after the ball shakes and before the catch commits, confirm nobody has the Pokémon. Catch it on a retry and confirm only one save owns it.

## Known limits

- Route 101 across the internet was not proven here. The RetroArch steps above are the test, not a result.
- Coordinates in the link frame are 8-bit. A map wider than 255 tiles wraps the partner sprite.
- The acro bike is the extra bit in the facing nibble. Mach bike and acro bike can both be shown. Surf wins over a bike.
- The guest in the champion room misses the Hall of Fame scene.
- TMs and items stay with the person who talks to the leader after the win.
- A partner who is already inside a script can miss the lab warp. Stand on the overworld when the other person opens Birch's bag.
- Six Pokémon per player is the path this tree builds. It is the risky part of link battles.
- gimmicks stay off, including primals.
- Id 865 is unused on purpose. It is the in-game partner id for Steven, and a normal trainer cannot live there.
- Invalid link frames are dropped. One bad packet does not disconnect you.

## Credit

Pokémon and Pokémon Emerald are Nintendo, Game Freak, and Creatures. This project does not ship their ROM.

Engine: pret pokeemerald, and RHH pokeemerald-expansion 1.17.1.
