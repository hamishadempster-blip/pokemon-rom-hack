// Shared legendary encounters.
//
// Rule: when two eligible players take part in a shared legendary encounter,
// one successful capture gives each of them their own permanent Pokémon from
// that encounter, once per save. A player alone completes the encounter and
// keeps their own legendary as in the stock game; a second player is never
// required. (For Rayquaza the Twin Beacons resonate: the trainers' parallel
// worlds touch during the encounter, and each world keeps its own Rayquaza.)
//
// Encounters (COOP_ENC_*): Rayquaza (Sky Pillar), Groudon (Terra Cave),
// Kyogre (Marine Cave), Regirock (Desert Ruins), Regice (Island Cave) and
// Registeel (Ancient Tomb): each a single static encounter in each world.
// Not shared, by design (docs/COOP.md): the roaming Latias/Latios (each world
// has its own roamer; met alone, as stock), and the ticket islands (Southern
// Island, Faraway, Birth, Navel Rock: event-only, and reset after every Hall
// of Fame, so they would repeat rewards).
//
// Records (CoopSave layout 3), one bit per encounter, never inferred from
// what a party or box holds now (trading, releasing or depositing the
// Pokémon changes nothing):
//   legendClaimed  this save received its own Pokémon (caught it, alone or in
//                  a shared encounter, or received the shared reward);
//   legendPending  earned, not yet delivered (no room, or interrupted);
//   legendOwed     this save made the shared catch; the partner's save has
//                  not yet confirmed its reward (legendOwedOtId: who).
//
// Battle: the co-op link double against one wild legendary (the opposing
// right battler absent, a format the engine supports for any double where a
// side has one Pokémon). Either player may throw a ball; one catch resolves
// the encounter (the catch ledger keeps a second ball from catching it
// again). Fleeing, a knockout, or a loss grant nothing, and the legendary
// stays for another try (a knockout no longer removes it for good).
//
// Delivery: the catcher's game keeps the caught Pokémon (thrown ball used up
// as normal), records its claim and the partner's debt, saves, and sends
// EARN (encounter, ball) over the deal link, again on every new link with
// that partner until it hears ACK. The other game, on EARN, records the
// reward as pending, saves, and only then answers ACK; it then makes its own
// Pokémon (its own trainer as OT, met at the encounter, met level, the ball
// that made the shared catch; nature, IVs and shininess fixed by that save's
// trainer id and the encounter, so nothing rerolls), delivers it to the
// party or PC, marks the claim, and saves again. A save that already
// claimed the encounter answers ACK without a second Pokémon.

#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_legend.h"
#include "battle.h"
#include "battle_setup.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "item.h"
#include "link.h"
#include "main.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "task.h"
#include "constants/characters.h"
#include "constants/battle.h"
#include "constants/coop.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/region_map_sections.h"
#include "constants/vars.h"

struct CoopEncounter
{
    u16 species;
    u8 level;
    u8 mapsec;
    u16 defeatedFlag;
    u16 hideFlag;
    u8 localId;
    u8 mapGroup;
    u8 mapNum;
};

static const struct CoopEncounter sEncounters[COOP_ENC_COUNT] = {
    [COOP_ENC_RAYQUAZA]  = { SPECIES_RAYQUAZA,  70, MAPSEC_SKY_PILLAR,    FLAG_DEFEATED_RAYQUAZA,  FLAG_HIDE_SKY_PILLAR_TOP_RAYQUAZA_STILL, 2, MAP_GROUP(MAP_SKY_PILLAR_TOP),   MAP_NUM(MAP_SKY_PILLAR_TOP) },
    [COOP_ENC_GROUDON]   = { SPECIES_GROUDON,   70, MAPSEC_TERRA_CAVE,    FLAG_DEFEATED_GROUDON,   FLAG_HIDE_TERRA_CAVE_GROUDON,            1, MAP_GROUP(MAP_TERRA_CAVE_END),   MAP_NUM(MAP_TERRA_CAVE_END) },
    [COOP_ENC_KYOGRE]    = { SPECIES_KYOGRE,    70, MAPSEC_MARINE_CAVE,   FLAG_DEFEATED_KYOGRE,    FLAG_HIDE_MARINE_CAVE_KYOGRE,            1, MAP_GROUP(MAP_MARINE_CAVE_END),  MAP_NUM(MAP_MARINE_CAVE_END) },
    [COOP_ENC_REGIROCK]  = { SPECIES_REGIROCK,  40, MAPSEC_DESERT_RUINS,  FLAG_DEFEATED_REGIROCK,  FLAG_HIDE_REGIROCK,                      1, MAP_GROUP(MAP_DESERT_RUINS),     MAP_NUM(MAP_DESERT_RUINS) },
    [COOP_ENC_REGICE]    = { SPECIES_REGICE,    40, MAPSEC_ISLAND_CAVE,   FLAG_DEFEATED_REGICE,    FLAG_HIDE_REGICE,                        1, MAP_GROUP(MAP_ISLAND_CAVE),      MAP_NUM(MAP_ISLAND_CAVE) },
    [COOP_ENC_REGISTEEL] = { SPECIES_REGISTEEL, 40, MAPSEC_ANCIENT_TOMB,  FLAG_DEFEATED_REGISTEEL, FLAG_HIDE_REGISTEEL,                     1, MAP_GROUP(MAP_ANCIENT_TOMB),     MAP_NUM(MAP_ANCIENT_TOMB) },
};

STATIC_ASSERT(COOP_ENC_COUNT - 1 == COOP_LEGEND_SLOTS, CoopLegendSlotsMatchEncounters);
STATIC_ASSERT(COOP_ENC_COUNT - 1 <= 8, CoopLegendMasksFitAByte);

// ACK results (DMSG_LEGEND, sub COOP_LEGEND_MSG_ACK).
#define LEGEND_ACK_RECORDED 1   // this save now has its reward (pending or delivered)
#define LEGEND_ACK_ALREADY  2   // this save had already claimed this encounter

// Frames between EARN resends while an ACK is outstanding.
#define LEGEND_RESEND_FRAMES 240
// Frames the catcher waits for the partner's ACK before saying it will
// arrive on a later link.
#define LEGEND_ACK_WAIT 900

static EWRAM_DATA u8 sSharedNoteEnc = COOP_ENC_NONE;    // "caught! both trainers..." to show
static EWRAM_DATA u8 sAckNoteEnc = COOP_ENC_NONE;       // partner's save confirmed
static EWRAM_DATA u8 sAckNoteResult = 0;
static EWRAM_DATA u8 sLaterNoteEnc = COOP_ENC_NONE;     // no answer yet: it arrives later
static EWRAM_DATA u16 sResendTimer = 0;
static EWRAM_DATA u16 sAckWait = 0;
static EWRAM_DATA u8 sAckWaitEnc = COOP_ENC_NONE;
static EWRAM_DATA bool8 sNoRoomNoted = FALSE;
static EWRAM_DATA u8 sDeliverEnc = COOP_ENC_NONE;       // the reward being delivered now
static EWRAM_DATA u8 sRemoveEnc = COOP_ENC_NONE;        // its object goes once back on the field
static EWRAM_DATA u8 sAwayNoteEnc = COOP_ENC_NONE;      // guest: "flew away" to show
// Test visibility: rewards delivered this session, EARN and ACK counts.
EWRAM_DATA u8 gCoopLegendDelivered = 0;
EWRAM_DATA u8 gCoopLegendEarnSeen = 0;
EWRAM_DATA u8 gCoopLegendAckSeen = 0;

static void ResolveFlags(u8 enc);

static struct CoopSave *Save(void)
{
    return Coop_GetSave();
}

static bool8 ValidEnc(u8 enc)
{
    return enc > COOP_ENC_NONE && enc < COOP_ENC_COUNT;
}

static u8 Bit(u8 enc)
{
    return 1 << (enc - 1);
}

u16 CoopLegend_Species(u8 enc)
{
    return ValidEnc(enc) ? sEncounters[enc].species : SPECIES_NONE;
}

u8 CoopLegend_Level(u8 enc)
{
    return ValidEnc(enc) ? sEncounters[enc].level : 0;
}

u8 CoopLegend_EncForSpecies(u16 species)
{
    u32 enc;

    for (enc = COOP_ENC_RAYQUAZA; enc < COOP_ENC_COUNT; enc++)
    {
        if (sEncounters[enc].species == species)
            return enc;
    }
    return COOP_ENC_NONE;
}

// This save's story has reached the encounter (its world's legendary is
// there to be met).
static bool8 StoryReady(u8 enc)
{
    switch (enc)
    {
    case COOP_ENC_RAYQUAZA:
        return VarGet(VAR_SKY_PILLAR_STATE) >= 2;
    case COOP_ENC_GROUDON:
    case COOP_ENC_KYOGRE:
        return FlagGet(FLAG_SYS_GAME_CLEAR);
    case COOP_ENC_REGIROCK:
        return FlagGet(FLAG_SYS_REGIROCK_PUZZLE_COMPLETED);
    case COOP_ENC_REGICE:
        return FlagGet(FLAG_SYS_BRAILLE_REGICE_COMPLETED);
    case COOP_ENC_REGISTEEL:
        return FlagGet(FLAG_SYS_REGISTEEL_PUZZLE_COMPLETED);
    }
    return FALSE;
}

// May this save take part in (and be rewarded by) the shared encounter?
bool8 CoopLegend_Eligible(u8 enc)
{
    struct CoopSave *save = Save();

    if (!ValidEnc(enc) || save == NULL)
        return FALSE;
    if ((save->legendClaimed | save->legendPending) & Bit(enc))
        return FALSE;
    if (FlagGet(sEncounters[enc].defeatedFlag))
        return FALSE;
    return StoryReady(enc);
}

static u32 PartnerOtId(void)
{
    if (!gReceivedRemoteLinkPlayers || GetMultiplayerId() > 1)
        return 0;
    return gLinkPlayers[GetMultiplayerId() ^ 1].trainerId;
}

static void BufferNames(u8 enc)
{
    StringCopy(gStringVar1, GetSpeciesName(CoopLegend_Species(enc)));
    gStringVar2[0] = EOS;
    if (gReceivedRemoteLinkPlayers && GetMultiplayerId() <= 1)
    {
        StringCopyN(gStringVar2, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
        gStringVar2[PLAYER_NAME_LENGTH] = EOS;
    }
}

// Special (VAR_0x8004 = COOP_ENC_*): the encounter starts. COOP_LEGEND_ASK
// when the linked partner is on this map, so the script offers to share it;
// COOP_LEGEND_SOLO otherwise. STR_VAR_1: the species, STR_VAR_2: the
// partner. Whether the partner may share is their game's answer.
u16 CoopLegend_Offer(void)
{
    u8 enc = VarGet(VAR_0x8004);

    struct CoopSave *save = Save();

    BufferNames(enc);
    // Already this save's (claimed, or a reward waiting for room): no second
    // battle; its flags are set and the script removes it.
    if (ValidEnc(enc) && save != NULL && ((save->legendClaimed | save->legendPending) & Bit(enc)))
    {
        ResolveFlags(enc);
        return COOP_LEGEND_GONE;
    }
    if (!ValidEnc(enc) || !Coop_SessionActive() || !Coop_PartnerOnSameMap())
        return COOP_LEGEND_SOLO;
    return COOP_LEGEND_ASK;
}

// Special: why the shared encounter did not start (into gStringVar4).
u16 CoopLegend_BufferDeclined(void)
{
    u8 enc = VarGet(VAR_0x8004);
    u8 why = Coop_PeekSoloNote();
    u8 *str = gStringVar4;

    BufferNames(enc);
    switch (why)
    {
    case COOP_SOLO_LEGEND_NOT_ELIGIBLE:
        str = StringCopy(str, gStringVar2);
        str = StringCopy(str, COMPOUND_STRING(" can't share this one: in their\nworld, "));
        str = StringCopy(str, gStringVar1);
        StringCopy(str, COMPOUND_STRING(" is already caught\lor not there yet."));
        break;
    case COOP_SOLO_NO_ANSWER:
        str = StringCopy(str, gStringVar2);
        StringCopy(str, COMPOUND_STRING(" didn't answer."));
        break;
    case COOP_SOLO_SELF_NOT_READY:
        StringCopy(str, COMPOUND_STRING("Your first three POKéMON can't\nbattle, so you can't share it."));
        break;
    case COOP_SOLO_ELSEWHERE:
    case COOP_SOLO_RECOVERING:
        str = StringCopy(str, gStringVar2);
        StringCopy(str, COMPOUND_STRING(" can't be reached here\nright now."));
        break;
    case COOP_SOLO_BUSY:
    case COOP_SOLO_PARTNER_NOT_READY:
        str = StringCopy(str, gStringVar2);
        StringCopy(str, COMPOUND_STRING(" isn't ready right now."));
        break;
    default:
        str = StringCopy(str, gStringVar2);
        StringCopy(str, COMPOUND_STRING(" decided not to join."));
        break;
    }
    Coop_ClearSoloNote();
    return why;
}

static void RemoveObjectHere(u8 enc)
{
    const struct CoopEncounter *e = &sEncounters[enc];

    if (gSaveBlock1Ptr->location.mapGroup == e->mapGroup && gSaveBlock1Ptr->location.mapNum == e->mapNum)
        RemoveObjectEventByLocalIdAndMap(e->localId, e->mapNum, e->mapGroup);
}

// The legendary in this world is gone with its trainer (caught here, or
// this save's reward from a shared catch): its flag and its object, and for
// Groudon and Kyogre the end of their weather.
static void ResolveFlags(u8 enc)
{
    const struct CoopEncounter *e = &sEncounters[enc];

    FlagSet(e->defeatedFlag);
    FlagSet(e->hideFlag);
    if (enc == COOP_ENC_GROUDON || enc == COOP_ENC_KYOGRE)
        VarSet(VAR_SHOULD_END_ABNORMAL_WEATHER, 1);
}

static void ResolveHere(u8 enc)
{
    ResolveFlags(enc);
    // Its object on this map goes now if this is the field; from a battle,
    // once the field is back (the battle owns the sprites until then).
    if (gMain.callback2 == CB2_Overworld)
        RemoveObjectHere(enc);
    else
        sRemoveEnc = enc;
}

// Special (VAR_0x8004 = COOP_ENC_*): start the stock one-player battle.
void CoopLegend_StartSolo(void)
{
    u8 enc = VarGet(VAR_0x8004);

    if (!ValidEnc(enc))
        return;
    CreateScriptedWildMon(sEncounters[enc].species, sEncounters[enc].level, ITEM_NONE);
    if (enc >= COOP_ENC_REGIROCK)
        StartRegiBattle();
    else
        BattleSetup_StartLegendaryBattle();
}

// Special, after either battle (VAR_0x8004 = COOP_ENC_*). VAR_RESULT for the
// script: 0 caught (remove the legendary), 1 it got away (a knockout or a
// flight: it stays for another try), 2 the player lost (the game whites out).
u16 CoopLegend_AfterBattle(void)
{
    u8 enc = VarGet(VAR_0x8004);
    u16 outcome = gBattleOutcome;
    struct CoopSave *save = Save();

    if (outcome == B_OUTCOME_CAUGHT)
    {
        if (ValidEnc(enc))
        {
            // Alone, the catch is this save's claim. Shared, both games have
            // already recorded their parts (CoopLegend_OnSharedEnd).
            if (save != NULL && !(gBattleTypeFlags & BATTLE_TYPE_COOP))
                save->legendClaimed |= Bit(enc);
            ResolveHere(enc);
        }
        return 0;
    }
    if (outcome == B_OUTCOME_LOST || outcome == B_OUTCOME_DREW)
        return 2;
    // Groudon and Kyogre knocked out: their weather still ends, as stock.
    if (outcome == B_OUTCOME_WON && (enc == COOP_ENC_GROUDON || enc == COOP_ENC_KYOGRE))
        VarSet(VAR_SHOULD_END_ABNORMAL_WEATHER, 1);
    return 1;
}

// Both games, when a shared encounter ends cleanly on this machine (the
// battle's catch notes say who caught it). Nothing happens unless it was
// caught.
void CoopLegend_OnSharedEnd(u8 enc, bool8 caughtHere, bool8 caughtThere, u16 ball)
{
    struct CoopSave *save = Save();

    if (!ValidEnc(enc) || (!caughtHere && !caughtThere))
        return;
    ResolveHere(enc);
    sSharedNoteEnc = enc;
    if (save == NULL)
        return;
    if (caughtHere)
    {
        save->legendClaimed |= Bit(enc);
        save->legendOwed |= Bit(enc);
        save->legendOwedOtId = PartnerOtId();
        save->legendOwedBall[enc - 1] = ball < ITEMS_COUNT && GetItemPocket(ball) == POCKET_POKE_BALLS ? ItemIdToBallId(ball) : BALL_POKE;
        sResendTimer = LEGEND_RESEND_FRAMES;   // EARN goes out at once
        sAckWait = 0;
        sAckWaitEnc = enc;
        CoopDeal_SaveInBackground(COOP_ENC_NONE, 0);
    }
    // The other game made the catch: this save's reward comes with its EARN
    // (it carries the ball), which also covers a link lost at the very end.
}

// The guest's side of a shared encounter that ended without a catch (a
// knockout or a flight): its legendary leaves for this visit, as on the
// starter's screen; a knockout still ends Groudon's or Kyogre's weather.
void CoopLegend_OnSharedAway(u8 enc, u8 outcome)
{
    if (!ValidEnc(enc))
        return;
    if (outcome == B_OUTCOME_WON && (enc == COOP_ENC_GROUDON || enc == COOP_ENC_KYOGRE))
        VarSet(VAR_SHOULD_END_ABNORMAL_WEATHER, 1);
    sAwayNoteEnc = enc;
    sRemoveEnc = enc;
}

// Special: "The X flew away!" for the guest (once). VAR_RESULT TRUE with
// STR_VAR_1 the species.
u16 CoopLegend_TakeAwayNote(void)
{
    u8 enc = sAwayNoteEnc;

    sAwayNoteEnc = COOP_ENC_NONE;
    if (!ValidEnc(enc))
        return FALSE;
    StringCopy(gStringVar1, GetSpeciesName(sEncounters[enc].species));
    return TRUE;
}

// Special: the shared-catch line for the after-battle script (once).
// VAR_RESULT TRUE with STR_VAR_1 the species.
u16 CoopLegend_TakeSharedNote(void)
{
    u8 enc = sSharedNoteEnc;

    sSharedNoteEnc = COOP_ENC_NONE;
    if (!ValidEnc(enc))
        return FALSE;
    StringCopy(gStringVar1, GetSpeciesName(sEncounters[enc].species));
    return TRUE;
}

// The partner reward: a Pokémon of this save, fixed by its trainer id and
// the encounter (no rerolls by reconnecting or claiming again).
static void MakeReward(struct Pokemon *mon, u8 enc, u8 ballId)
{
    const struct CoopEncounter *e = &sEncounters[enc];
    rng_value_t saved = gRngValue;
    const u8 *id = gSaveBlock2Ptr->playerTrainerId;
    u32 seed = id[0] | (id[1] << 8) | (id[2] << 16) | ((u32)id[3] << 24);
    u8 metLocation = e->mapsec, metLevel = e->level;
    u32 personality;

    seed = (seed ^ 0x9E3779B9u) * 0x85EBCA6Bu + enc * 0xC2B2AE35u;
    SeedRng(seed);
    personality = Random32();
    CreateMonWithIVs(mon, e->species, e->level, personality, OTID_STRUCT_PLAYER_ID, USE_RANDOM_IVS);
    GiveMonInitialMoveset(mon);
    gRngValue = saved;
    if (ballId == 0 || ballId >= POKEBALL_COUNT)
        ballId = BALL_POKE;
    SetMonData(mon, MON_DATA_POKEBALL, &ballId);
    SetMonData(mon, MON_DATA_MET_LOCATION, &metLocation);
    SetMonData(mon, MON_DATA_MET_LEVEL, &metLevel);
    CalculateMonStats(mon);
}

static bool8 HasRoom(void)
{
    return CalculatePlayerPartyCount() < PARTY_SIZE || Coop_PcFreeSlots() != 0;
}

static u8 NextPending(void)
{
    struct CoopSave *save = Save();
    u32 enc;

    if (save == NULL)
        return COOP_ENC_NONE;
    for (enc = COOP_ENC_RAYQUAZA; enc < COOP_ENC_COUNT; enc++)
    {
        if (save->legendPending & Bit(enc))
            return enc;
    }
    return COOP_ENC_NONE;
}

// Special: deliver the next pending reward. VAR_RESULT MON_GIVEN_TO_PARTY or
// MON_GIVEN_TO_PC (STR_VAR_1 the species; the PC box in the usual vars), or
// MON_CANT_GIVE when there is nothing to deliver or no room.
u16 CoopLegend_DeliverNext(void)
{
    struct CoopSave *save = Save();
    struct Pokemon mon;
    u8 enc = NextPending(), result;

    sDeliverEnc = COOP_ENC_NONE;
    if (save == NULL || enc == COOP_ENC_NONE || !HasRoom())
        return MON_CANT_GIVE;
    MakeReward(&mon, enc, save->legendPendingBall[enc - 1]);
    result = GiveCapturedMonToPlayer(&mon);
    if (result == MON_CANT_GIVE)
        return MON_CANT_GIVE;
    HandleSetPokedexFlagFromMon(&mon, FLAG_SET_SEEN);
    HandleSetPokedexFlagFromMon(&mon, FLAG_SET_CAUGHT);
    save->legendPending &= ~Bit(enc);
    save->legendPendingBall[enc - 1] = 0;
    save->legendClaimed |= Bit(enc);
    ResolveHere(enc);
    StringCopy(gStringVar1, GetSpeciesName(sEncounters[enc].species));
    VarSet(VAR_TEMP_TRANSFERRED_SPECIES, sEncounters[enc].species);
    sDeliverEnc = enc;
    if (gCoopLegendDelivered < 0xFF)
        gCoopLegendDelivered++;
    CoopDeal_SaveInBackground(COOP_ENC_NONE, 0);
    return result;
}

// Special (waitstate): until this game's save has reached the flash.
static void Task_WaitLegendSave(u8 taskId)
{
    if (!CoopDeal_SaveBusy())
    {
        DestroyTask(taskId);
        ScriptContext_Enable();
    }
}

void CoopLegend_WaitSaved(void)
{
    CreateTask(Task_WaitLegendSave, 80);
}

// Special: the partner's confirmation to show (once). VAR_RESULT 1 recorded,
// 2 they had already claimed it, 3 no answer yet (it arrives on a later
// link), 0 nothing. STR_VAR_1 species, STR_VAR_2 partner.
u16 CoopLegend_TakeAckNote(void)
{
    u8 enc, result;

    if (ValidEnc(sAckNoteEnc))
    {
        enc = sAckNoteEnc;
        result = sAckNoteResult;
        sAckNoteEnc = COOP_ENC_NONE;
    }
    else if (ValidEnc(sLaterNoteEnc))
    {
        enc = sLaterNoteEnc;
        result = 3;
        sLaterNoteEnc = COOP_ENC_NONE;
    }
    else
    {
        return 0;
    }
    BufferNames(enc);
    return result;
}

// The deal link: EARN and ACK.
void CoopLegend_HandleMsg(u8 sub, u8 enc, u16 ball, u16 result, u32 fromOtId)
{
    struct CoopSave *save = Save();

    if (save == NULL || !ValidEnc(enc))
        return;
    if (sub == COOP_LEGEND_MSG_EARN)
    {
        if (gCoopLegendEarnSeen < 0xFF)
            gCoopLegendEarnSeen++;
        // (No check of the defeated flag here: the shared battle's end has
        // already set it on this save. Who may take part at all is decided
        // by the invite's eligibility check.)
        if (save->legendClaimed & Bit(enc))
        {
            // This very debt, already paid (the first ACK was lost): say so.
            // Any other claim (a catch of its own, another partner's catch)
            // means nothing was added.
            CoopDeal_SendLegend(COOP_LEGEND_MSG_ACK, enc, 0,
                (fromOtId != 0 && save->legendFromOtId[enc - 1] == fromOtId) ? LEGEND_ACK_RECORDED : LEGEND_ACK_ALREADY);
            return;
        }
        if (!(save->legendPending & Bit(enc)))
        {
            save->legendPending |= Bit(enc);
            save->legendFromOtId[enc - 1] = fromOtId;
            save->legendPendingBall[enc - 1] = (ball != 0 && ball < POKEBALL_COUNT) ? ball : BALL_POKE;
            // This world's legendary went with this trainer's partner's
            // catch: it is not waiting here any more.
            ResolveHere(enc);
            sNoRoomNoted = FALSE;
        }
        // Recorded in the flash first; then the ACK goes out.
        CoopDeal_SaveInBackground(enc, LEGEND_ACK_RECORDED);
        return;
    }
    if (sub == COOP_LEGEND_MSG_ACK)
    {
        if (gCoopLegendAckSeen < 0xFF)
            gCoopLegendAckSeen++;
        if (!(save->legendOwed & Bit(enc)) || fromOtId != save->legendOwedOtId)
            return;
        save->legendOwed &= ~Bit(enc);
        save->legendOwedBall[enc - 1] = 0;
        if (save->legendOwed == 0)
            save->legendOwedOtId = 0;
        if (sAckWaitEnc == enc)
            sAckWaitEnc = COOP_ENC_NONE;
        sAckNoteEnc = enc;
        sAckNoteResult = result == LEGEND_ACK_ALREADY ? 2 : 1;
        if (sLaterNoteEnc == enc)
            sLaterNoteEnc = COOP_ENC_NONE;
    }
}

// A new link: owed rewards go out again (to that partner only).
void CoopLegend_OnSessionStart(void)
{
    sResendTimer = LEGEND_RESEND_FRAMES - 60;
    sNoRoomNoted = FALSE;
}

// Every overworld frame: resend owed EARNs; watch for a late ACK; start a
// pending delivery when this player is free and there is room.
void CoopLegend_FieldFrame(void)
{
    struct CoopSave *save = Save();
    u32 enc;

    if (save == NULL)
        return;
    if (ValidEnc(sRemoveEnc))
    {
        RemoveObjectHere(sRemoveEnc);
        sRemoveEnc = COOP_ENC_NONE;
    }
    if (save->legendOwed && Coop_SessionActive() && PartnerOtId() == save->legendOwedOtId && PartnerOtId() != 0)
    {
        if (++sResendTimer >= LEGEND_RESEND_FRAMES)
        {
            sResendTimer = 0;
            for (enc = COOP_ENC_RAYQUAZA; enc < COOP_ENC_COUNT; enc++)
            {
                if (save->legendOwed & Bit(enc))
                {
                    CoopDeal_SendLegend(COOP_LEGEND_MSG_EARN, enc, save->legendOwedBall[enc - 1], 0);
                    break;
                }
            }
        }
    }
    if (ValidEnc(sAckWaitEnc) && (save->legendOwed & Bit(sAckWaitEnc)) && ++sAckWait >= LEGEND_ACK_WAIT)
    {
        sLaterNoteEnc = sAckWaitEnc;
        sAckWaitEnc = COOP_ENC_NONE;
    }
}

// TRUE when a pending reward should be delivered now (the caller starts the
// delivery script; an idle player only). Sets *noRoomNote the first time a
// pending reward finds no room.
bool8 CoopLegend_DeliveryDue(bool8 *noRoomNote)
{
    *noRoomNote = FALSE;
    if (NextPending() == COOP_ENC_NONE || CoopDeal_SaveBusy())
        return FALSE;
    if (!HasRoom())
    {
        if (!sNoRoomNoted)
        {
            sNoRoomNoted = TRUE;
            StringCopy(gStringVar1, GetSpeciesName(sEncounters[NextPending()].species));
            *noRoomNote = TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

// TRUE when a note for the field is waiting (shared catch, partner's ACK).
bool8 CoopLegend_NoteDue(void)
{
    return ValidEnc(sAckNoteEnc) || ValidEnc(sLaterNoteEnc);
}
