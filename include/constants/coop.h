#ifndef GUARD_CONSTANTS_COOP_H
#define GUARD_CONSTANTS_COOP_H

// Shared story milestones. Saves store these bit numbers and the cable
// carries them, so the list is append only: never renumber or reuse one.
// A milestone is story state both saves should agree on. Parties, bags,
// money, catches, and optional conversations are never milestones.
#define COOP_STORY_RIVAL_MET     0  // Introduced to the rival twins in Littleroot
#define COOP_STORY_COUNT         1
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

#endif // GUARD_CONSTANTS_COOP_H
