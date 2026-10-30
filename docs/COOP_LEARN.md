# "Who can learn this?"

Section 11 of the collection update, added after build 0xC0170141.

## What the player does

BAG > TMs & HMs > a TM or HM > **CAN LEARN** (the free fourth place in that pocket's menu: USE, GIVE / CAN LEARN, CANCEL).

1. The message box says how many of the player's own Pokémon, party and every PC box, can learn its move and do not know it yet: "4 of your POKéMON can learn Dig (2 in the party)." It adds "N know(s) it." when some already do. With none: "None of your POKéMON can learn Dig." (or "… now. 2 know it.").
2. Then A pages through where they are: "PARTY: Mudkip, Taillow", "BOX1: Poochyena, Buizel". Each page holds one place, two lines of names (nicknames). A box that does not fit goes on as "BOX1, more: …". B at any page goes straight back to the BAG.

## Rules

- **Who counts:** the party menu's own test for a TM (`CanTeachMove`, `src/party_menu.c`). The species' teachable moves count; eggs never do; a Pokémon that already knows the move is counted apart.
- **Read only:** nothing is taught, moved or changed, and the BAG stays open. The partner's Pokémon are on the other game and are not searched.
- **Where it works:** only in the field BAG (the pocket menu used outside battle). The battle bag, shops and the PC's item storage use other menus.

Code: `src/coop_learn.c`, with the BAG action in `src/item_menu.c` (`ItemMenu_CanLearn`).

## Tests (harness)

| Scenario | Checks |
| --- | --- |
| `learn_who` | j52 saves, TM28 DIG (FIXTURE). "4 of your POKéMON can learn Dig (2 in the party)."; "PARTY: Aaaaaaaaaaaa, Nincada"; "BOX1: Poochyena, Buizel"; then back in the BAG, party and TM unchanged. Expected values were worked out from the party and boxes dumped in the run and the species' teachable learnsets: of the nine Pokémon, Combusken, Nincada, Poochyena and Buizel can learn Dig |
| `learn_who_full` | Every empty PC slot filled with a copy of the party's Whiscash (FIXTURE, 417 copies), TM26 Earthquake. "418 … (1 in the party)."; PARTY page; "BOX1: …" then "BOX1, more: …"; B stops at once; party unchanged |

Not tested: the "know it" wording in play; an HM; a Pokémon with a nickname wider than usual.
