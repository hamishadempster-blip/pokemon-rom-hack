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

#endif // GUARD_CONSTANTS_COOP_H
