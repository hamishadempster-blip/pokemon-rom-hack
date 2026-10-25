#ifndef GUARD_CONSTANTS_COOP_H
#define GUARD_CONSTANTS_COOP_H

// Shared story milestones. Saves store these bit numbers and the cable
// carries them, so the list is append only: never renumber or reuse one.
// A milestone is story state both saves should agree on. Parties, bags,
// money, catches, and optional conversations are never milestones.
#define COOP_STORY_RIVAL_MET     0  // Introduced to the rival twins in Littleroot
#define COOP_STORY_POWER_RESTORED 1 // New Mauville's generator is off (Power Problems)
#define COOP_STORY_SHOAL_RESCUED 2  // Shoal Cave: the researcher is safe at the entrance
#define COOP_STORY_CARAVAN_SUPPLIED 3 // Route 111: the caravan has its supplies (merchant open)
#define COOP_STORY_COAST_CLEAN   4  // Route 109: the dumping crew is gone, the beach clean
#define COOP_STORY_SHRINE_OPEN   5  // Scorched Slab: the passage to the Twin Beacon shrine is open
#define COOP_STORY_SHRINE_SOLVED 6  // Twin Beacon shrine: the sanctum door is open
#define COOP_STORY_COUNT         7
#define COOP_STORY_KNOWN_MASK    ((1 << COOP_STORY_COUNT) - 1)

// Scenes a nearby partner can join. 0 is "no scene".
#define COOP_SCENE_NONE          0
#define COOP_SCENE_RIVAL_INTRO   1

// Shared progression items, bit numbers in the save. Append only.
#define COOP_SHARED_DOWSING      0
#define COOP_SHARED_HM_FLY       1

// COOP_KIND_WILD in include/coop.h, for scripts.
#define COOP_KIND_WILD_VALUE     10

// The two rivals. Birch's twins: Soren carries the starter of the case that
// beats yours, Mira carries its companion. Stored in the save, so stable.
#define COOP_RIVAL_NONE          0
#define COOP_RIVAL_SOREN         1
#define COOP_RIVAL_MIRA          2

// Partner interaction (src/coop_deal.c). Deal kinds, also used by scripts.
#define COOP_DEAL_NONE        0
#define COOP_DEAL_BATTLE      1
#define COOP_DEAL_TRADE       2
#define COOP_DEAL_GIVE_MON    3
#define COOP_DEAL_GIVE_ITEM   4

// Escrow record states in the save. Append only.
#define COOP_TXN_NONE         0
#define COOP_TXN_PREPARED     1

// Results a deal script reads from VAR_RESULT.
#define COOP_DEAL_OK          0
#define COOP_DEAL_DECLINED    1
#define COOP_DEAL_BUSY        2
#define COOP_DEAL_NO_ROOM     3
#define COOP_DEAL_CANCELLED   4
#define COOP_DEAL_LOST        5   // the link dropped or the partner left
#define COOP_DEAL_PENDING     6   // an earlier transfer is still settling
#define COOP_DEAL_INVALID     7   // the offer is no longer valid
#define COOP_DEAL_TIMEOUT     8

// Friendly battle rules, packed in one byte: format bit 0, level bits 1-2,
// team size bits 3-4, unrestricted bit 5.
#define COOP_RULE_DOUBLES     (1 << 0)
#define COOP_RULE_LEVEL_SHIFT 1
#define COOP_RULE_LEVEL_ACTUAL 0
#define COOP_RULE_LEVEL_50    1
#define COOP_RULE_LEVEL_100   2
#define COOP_RULE_SIZE_SHIFT  3
#define COOP_RULE_SIZE_3      0
#define COOP_RULE_SIZE_4      1
#define COOP_RULE_SIZE_6      2
#define COOP_RULE_OPEN        (1 << 5)

// Fast battle options (CoopSave.battleSpeed). Bit 0: skip move animations.
// Bit 1: shorter message pauses. Shared battles use what both games allow.
#define COOP_SPEED_NO_ANIMS   (1 << 0)
#define COOP_SPEED_SHORT_TEXT (1 << 1)

// Per-save rewards (CoopSave.rewardsClaimed bit numbers). Append only.
#define COOP_REWARD_PP_WATTSON   0   // Power Problems: Wattson's reward
#define COOP_REWARD_PP_CHAMBER   1   // Power Problems: the hidden chamber
#define COOP_REWARD_SHOAL_RESCUE 2   // Shoal Cave: the researcher's thanks
#define COOP_REWARD_CARAVAN      3   // Desert caravan: the leader's thanks
#define COOP_REWARD_COAST        4   // Coastal cleanup: the fishers' thanks
#define COOP_REWARD_TWIN_SOREN   5   // Exhibition: Soren's gift
#define COOP_REWARD_TWIN_MIRA    6   // Exhibition: Mira's gift
#define COOP_REWARD_TOURNEY      7   // Twin Beacon tournament: first win
#define COOP_REWARD_TWIN_SPAR_1  8   // Soren's first spar prize
#define COOP_REWARD_TWIN_SPAR_2  9   // Soren's second spar prize
#define COOP_REWARD_SHRINE_LORE  10  // Twin Beacon: the keeper's charm

// Quest state carried live over the cable (src/coop_quest.c). Append only.
#define COOP_QUEST_PP_BREAKERS   1   // value: breakers reset in order, 0-3
#define COOP_QUEST_SHOAL_CLUES   2   // value: VAR_SHOAL_CLUES bits (merged with OR)
#define COOP_QUEST_CARAVAN_FOUND 3   // value: VAR_CARAVAN_FOUND crate bits (OR)
#define COOP_QUEST_COAST_CLUES   4   // value: VAR_COAST_CLUES bits (OR)
#define COOP_QUEST_SHRINE_MARKS  5   // value: VAR_SHRINE_MARKS bits (OR)
#define COOP_QUEST_TWIN_HEARD    6   // value: the sender's VAR_TWIN_ARC bits (OR into VAR_TWIN_HEARD)
#define COOP_QUEST_SHRINE_LANTERN 7  // value: 1 or 2, the lantern the sender just lit (live only)
#define COOP_QUEST_TRIAL_HEAL    8   // the Twin Trial restored teams (live only; the partner heals if in the shrine)
#define COOP_QUEST_SHRINE_GATES  9   // value: shrine gates open (1 plates, 2 lanterns); OR into the flags

// Journal entries (CoopQuest_BufferJournal, VAR_0x8004).
#define COOP_JOURNAL_PP          0
#define COOP_JOURNAL_SHOAL       1
#define COOP_JOURNAL_CARAVAN     2
#define COOP_JOURNAL_TWINS       3
#define COOP_JOURNAL_COAST       4
#define COOP_JOURNAL_BEACON      5

// VAR_TWIN_ARC / VAR_TWIN_HEARD bits.
#define TWIN_ARC_S1              (1 << 0)   // Soren, Slateport
#define TWIN_ARC_S2              (1 << 1)   // Soren, Route 119 (Fortree side)
#define TWIN_ARC_S3              (1 << 2)   // Soren, Mossdeep
#define TWIN_ARC_M1              (1 << 3)   // Mira, Route 109 beach
#define TWIN_ARC_M2              (1 << 4)   // Mira, the caravan camp on Route 111
#define TWIN_ARC_M3              (1 << 5)   // Mira, Shoal Cave
#define TWIN_ARC_EXHIBITION      (1 << 6)   // the exhibition in Littleroot
#define TWIN_ARC_SPAR_1          (1 << 7)   // beat Soren's first spar
#define TWIN_ARC_SPAR_2          (1 << 8)   // beat Soren's second spar

// Quest states.
#define SHOAL_NONE               0
#define SHOAL_BRIEFED            1
#define SHOAL_FOUND              2
#define SHOAL_RESCUED            3
#define SHOAL_CLUE_THERMO        (1 << 0)
#define SHOAL_CLUE_NOTES         (1 << 1)
#define SHOAL_CLUE_RADIO         (1 << 2)
#define SHOAL_CLUE_ALL           7

#define CARAVAN_NONE             0
#define CARAVAN_BRIEFED          1
#define CARAVAN_FOUND            2
#define CARAVAN_MERCHANT         3
#define CARAVAN_CRATE_ALL        7
#define CARAVAN_GLYPH_SHIFT      4          // bits 4-5: glyph pedestals pressed in order (0-3)

#define COAST_NONE               0
#define COAST_BRIEFED            1
#define COAST_CLUES              2
#define COAST_CREW_BEATEN        3
#define COAST_CLEAN              5
#define COAST_CLUE_SHEEN         (1 << 0)
#define COAST_CLUE_DRUM          (1 << 1)
#define COAST_CLUE_TRACKS        (1 << 2)
#define COAST_CLUE_ALL           7

#define SHRINE_NONE              0
#define SHRINE_STARTED           1
#define SHRINE_OPENED            2
#define SHRINE_MARK_DESERT       (1 << 0)
#define SHRINE_MARK_ISLAND       (1 << 1)
#define SHRINE_MARK_TOMB         (1 << 2)
#define SHRINE_MARK_ALL          7

#define BEACON_STYLE_COUNT       4

// Power Problems breakers, in the order the log asks for.
#define PP_BREAKER_STORAGE       0
#define PP_BREAKER_SERVICE       1
#define PP_BREAKER_CONTROL       2
#define PP_BREAKER_COUNT         3

// CoopQuest_PressBreaker results.
#define PP_PRESS_RESET           0   // this breaker is now on
#define PP_PRESS_ALREADY         1   // it was already on
#define PP_PRESS_TRIPPED         2   // wrong order: every breaker tripped
#define PP_PRESS_DONE            3   // generator already off; nothing to do

// Player outfit colours (CoopSave.outfit, src/coop_outfit.c): presets per
// player base. 0 is the stock look, which older saves read as.
#define COOP_OUTFIT_COUNT        6
// CoopSave.partnerMarker: a small arrow over the partner.
#define COOP_MARKER_AUTO         0   // only when both look the same
#define COOP_MARKER_ALWAYS       1
#define COOP_MARKER_OFF          2

#endif // GUARD_CONSTANTS_COOP_H
