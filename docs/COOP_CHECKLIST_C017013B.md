# Audit checklist, build 0xC017013B (Trainer Exchange, breeding)

**Played** means a two-core harness scenario did it on this ROM (fixtures and test helpers are named in each scenario and in the [test log](COOP_TESTLOG_C017013B.md)); **source** means checked from the code or an automated data check only.

## 1. Trainer Exchange in Rustboro

| Requirement | Status | Evidence |
| --- | --- | --- |
| Marked building near the Centre, reachable before Roxanne, no NPC/event/door replaced | Done, played | Open ground south-east of the Mart (x21–26, y48–52); door and two signs; `exchange_counters`, `exchange_stock_t0` (checkpoint 6, no badge) |
| Five counters with description and unlock info | Done, played | Intros, directory, guide, COMING SOON per counter (`exchange_counters`, `exchange_stock_*`) |
| Buyer's own badges and quest flags; partner can't unlock | Done, played | `OwnTier()` reads only this save; `exchange_coop`: Kai 84 / Rosa 39 at the same counter; after power-on Rosa still 39. Joint badges (won together) are restored on LINK UP: documented |
| Existing shops audited, prices consistent, shared definitions | Done, source | One table (`stock.py`) feeds the Exchange and `GetItemPrice` for every shop; existing shops keep their stock |

## 2. Badge-based stock

| Requirement | Status | Evidence |
| --- | --- | --- |
| Tiers 0 / 1–2 / 3–4 / 5–6 / 7–8 / Champion | Done, played at 0, 1, 3, 4, 6, 8, Champion | `exchange_stock_t0/t2/t3/t5`, `exchange_counters`, `exchange_coop`, `exchange_training` |
| Maintainable data | Done | `tools/coop_exchange/stock.py` (checks, header, doc) |
| Locked stock as a preview | Done, played | COMING SOON: next step's items (8 names, "and N more"), the Twin Trial hint, "Everything … open" |

## 3. Move access

| Requirement | Status | Evidence |
| --- | --- | --- |
| Curated, all 18 types, no signature moves | Done, source | `stock.py --check` |
| Progression by usefulness | Done | Tiers in `stock.py` (e.g. Helping Hand and Icy Wind early; screens, Tailwind, Stealth Rock at 3; U-turn, Volt Switch, Knock Off at 5; Close Combat, Trick Room at 7) |
| Species compatibility | Done, played | Teachable learnsets (`extraTutors`); the box chooser marks who can learn; `exchange_moves` |
| Relearning incl. evolution moves; egg moves | Done, played | `exchange_moves` (RELEARN; EGG MOVE refused below 3 badges) |
| TM reuse rule kept, priced for it | Done, source | Single-use; ₽2,000–12,000; no renumbering |

## 4. Battle and evolution items

| Requirement | Status | Evidence |
| --- | --- | --- |
| Renewable battle items; Choice, Life Orb, Sash, Vest, Eviolite later | Done | 5-badge tier; berries and boosters early |
| Evolution stock covers the roster | Done, source | All 49 evolution items stocked (`stock.py`) |
| No debug, key, Mega or legendary items | Done, source | `stock.py` rejects key items, Mega Stones, Z-Crystals, orbs, Gems; Kubfu scrolls excluded |
| Quest unlocks kept with hints | Done, played | Ability Capsule after the Twin Trial (`exchange_stock_t5`: 49 before it) |

## 5. Training

| Requirement | Status | Evidence |
| --- | --- | --- |
| Mints / vitamins (EVs) / EV berries and reset / Hyper Training / candies distinguished | Done, played | HOW IT WORKS text; `exchange_training` |
| Accessible EV correction | Done, played | EV berries (₽500, 1 badge) and RESET EVs (₽2,000, 1 badge) |
| Candies respect cap and obedience; no silent waste | Done, played | Cap stop with message, refusal at the cap (`exchange_training`); obedience unchanged (source) |
| Hyper Training and mints vs breeding explained | Done, played | `breeding_rules(_reload)`, guide text, COOP.md |
| Budget check; exploits | Done | COOP.md table from journey money; sell price a quarter |

## 6. Breeding and Day Care

| Requirement | Status | Evidence |
| --- | --- | --- |
| Ownership per save; no mixing | Played | `breeding_coop` (both at the lady at once, own pairs, own eggs) |
| Identity kept through deposit/withdraw | Played | Personality, species, held item compared (`breeding_coop`) |
| Families, Ditto, babies, Undiscovered | Played (representative) | Vulpix + Ditto, Vulpix pair, Marill pair → Azurill, Pichu + Ditto refused |
| Regional form, nature, IVs, Destiny Knot, Power items, egg moves, incense | Played | `breeding_coop`, `breeding_rules(_reload)` |
| Day Care EXP doesn't corrupt | Played (identity), source (level cap) | Withdrawals compared |
| Steps from the owning player only | Played | Rosa's counters unchanged while Kai walked |
| Full party, pending egg, save, reload | Played | Full party (egg waits; withdraw refused), pending egg across power-on |
| Reconnect | Source | No Day Care data crosses the link |
| Eggs can't battle; trading eggs explained | Played (refusal), source (battle) | "An EGG can't be traded or given." |
| Hatching with a partner's request | Played | Rosa's A at Kai during the hatch: nothing opens, the hatch runs on |
| Pokédex on hatch | Played | Vulpix, Azurill |
| Renewable Ditto; guide; no shared storage | Done | Desert Underpass (existing); guide at both places |

## 7. Co-op shop safety

| Requirement | Status | Evidence |
| --- | --- | --- |
| Independent browsing and buying, same counter | Played | `exchange_coop` (same frame; and one already inside) |
| Buyer's money, bag, progress only | Played | Money changes per player; stock per player |
| Bag full, repeated inputs, cancel, save | Played except repeated inputs | Bag full, too poor, QUIT and B paths, declined tutor, save and power-on; mashing A in the Mart was not tested separately (the stock Mart code) |
| Partner indicator; invitations treat shopper as busy | Played | Menu icon; partner menu "Rosa is shopping" |
| Gifting purchases | Played | LEFTOVERS from Kai (3 badges) to Rosa (2 badges); no badge rule on receiving |
