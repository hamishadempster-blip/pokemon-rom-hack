#ifndef GUARD_COOP_OUTFIT_H
#define GUARD_COOP_OUTFIT_H

// Player outfit colours (src/coop_outfit.c). Six presets for each player
// base (Brendan, May). The choice is per save, separate from gender, rival
// and story; the partner's choice arrives over the link (DMSG_LOOK).

// Palette data for an overworld palette tag: the local player's looks for
// OBJ_EVENT_PAL_TAG_BRENDAN/_MAY (and reflections), the partner's for
// OBJ_EVENT_PAL_TAG_COOP_PARTNER. NULL: use the stock table entry.
const u16 *CoopOutfit_ObjectPalette(u16 tag);
// Trainer pic palette for TRAINER_PIC_BRENDAN/_MAY in the local outfit
// (battle back pic, trainer card, intro). NULL for any other pic.
const u16 *CoopOutfit_TrainerPalette(u16 trainerPic);
// Battle: a link partner's back pic gets the partner's outfit.
void CoopOutfit_BattleTrainerSprite(u32 battler, u32 spriteId, u32 trainerPic);

u8 CoopOutfit_Local(void);
u16 CoopOutfit_PartnerGfx(bool8 female, u8 pose);
void CoopOutfit_OnPartnerLook(u8 outfit, u8 gender);
void CoopOutfit_OnSessionStart(void);
void CoopOutfit_RefreshLoaded(void);
void CoopOutfit_FieldFrame(void);

// Specials.
void CoopOutfit_OpenMenu(void);
u16 CoopOutfit_CycleNameTag(void);

enum {
    COOP_OUTFIT_POSE_NORMAL,
    COOP_OUTFIT_POSE_MACH_BIKE,
    COOP_OUTFIT_POSE_ACRO_BIKE,
    COOP_OUTFIT_POSE_SURFING,
};

#endif // GUARD_COOP_OUTFIT_H
