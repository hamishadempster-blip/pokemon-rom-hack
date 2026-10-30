#ifndef GUARD_COOP_FIELD_H
#define GUARD_COOP_FIELD_H

// Battle conditions panel (src/coop_field.c): a snapshot of the engine's
// field state, carried with each action request so the co-op guest has it.

struct CoopFieldSide
{
    u8 reflect, lightScreen, auroraVeil, safeguard, mist, tailwind;  // turns left (0: none)
    u8 spikes, toxicSpikes;     // layers
    u8 stealthRock:1, stickyWeb:1, steelsurge:1, unused:5;
};

struct CoopFieldBattler
{
    u16 species;
    u8 present:1, side:1, unused:6;
    s8 stages[7];   // Atk, Def, Spe, SpA, SpD, Acc, Eva: -6..+6
};

struct CoopFieldSnap
{
    u8 valid;
    u8 weatherTurns;
    u16 weather;    // B_WEATHER_* bits
    u8 terrain;     // B_TERRAIN_*
    u8 terrainTurns;
    u8 trickRoom, gravity, magicRoom, wonderRoom;   // turns left
    struct CoopFieldSide side[2];
    struct CoopFieldBattler battler[4];
};

extern struct CoopFieldSnap gCoopFieldSnap;

void CoopField_Fill(struct CoopFieldSnap *snap);
bool8 CoopField_PanelAvailable(void);
void CoopField_OpenPanel(void);
bool8 CoopField_PanelInput(void);

#endif // GUARD_COOP_FIELD_H
