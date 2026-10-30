// FIELD NOTES and collection rewards (collection update §4).
//
// CO-OP > JOURNAL > FIELD NOTES shows this save's collection: kinds of
// Pokémon caught and the next reward, visitors caught (src/
// coop_encounters.c), hidden habitats found (src/coop_habitat.c), and any
// reward still waiting for room in the BAG.
//
// Rewards are paid once per save, automatically, at the first calm moment
// in the field after entering a map (as the autosave waits for one; never in
// a menu, script, battle, deal, facility or link room): "FIELD NOTES: 25
// kinds caught! ASH got 5 ULTRA BALLS!". Each is one bit of
// CoopSaveV5.collectionClaimed, set in the same moment the item goes into the
// BAG, so both reach the flash with the same save: a reload before saving
// pays it again exactly once, never twice. With no room the reward waits in
// CoopSaveV5.pending (4 places, shared with habitat and town rewards) and is
// handed over at a later calm moment once there is room; with the queue
// full it is not claimed yet and is offered again later.
//
// The table is append-only: a reward's bit is its place in sRewards.
#include "global.h"
#include "coop.h"
#include "coop_autosave.h"
#include "coop_deal.h"
#include "coop_encounters.h"
#include "coop_habitat.h"
#include "coop_notes.h"
#include "event_data.h"
#include "event_scripts.h"
#include "item.h"
#include "pokedex.h"
#include "script.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/coop.h"
#include "constants/items.h"

enum
{
    NOTES_DEX,          // kinds of Pokémon caught (National Pokédex)
    NOTES_VISITORS,     // visitor kinds caught
    NOTES_VISITORS_ALL, // every visitor kind caught
};

struct NotesReward
{
    u8 kind;
    u16 need;
    u16 item;
    u8 quantity;
};

static const struct NotesReward sRewards[] =
{
    [0]  = { NOTES_DEX, 10, ITEM_GREAT_BALL, 5 },
    [1]  = { NOTES_DEX, 25, ITEM_ULTRA_BALL, 5 },
    [2]  = { NOTES_DEX, 50, ITEM_EXP_CANDY_M, 3 },
    [3]  = { NOTES_DEX, 100, ITEM_ABILITY_CAPSULE, 1 },
    [4]  = { NOTES_DEX, 150, ITEM_BOTTLE_CAP, 1 },
    [5]  = { NOTES_DEX, 200, ITEM_MASTER_BALL, 1 },
    [6]  = { NOTES_DEX, 300, ITEM_ABILITY_PATCH, 1 },
    [7]  = { NOTES_DEX, 400, ITEM_SHINY_CHARM, 1 },
    [8]  = { NOTES_VISITORS, 5, ITEM_DUSK_BALL, 5 },
    [9]  = { NOTES_VISITORS_ALL, 0, ITEM_QUICK_BALL, 10 },
};
STATIC_ASSERT(ARRAY_COUNT(sRewards) <= 8 * sizeof(((struct CoopSaveV5 *)0)->collectionClaimed), NotesLedgerFits);

EWRAM_DATA bool8 gCoopNotesCheck = FALSE;   // set on entering a map (test-visible: a harness fixture may set it)
EWRAM_DATA u8 gCoopNotesPaid = 0;   // test visibility: rewards handed over this session

static struct CoopSaveV5 *Notes_Save(void)
{
    struct CoopSave *save = Coop_GetSave();

    if (save == NULL || save->v5.size != sizeof(struct CoopSaveV5) || !Coop_SaveSchemaSupported())
        return NULL;
    return &save->v5;
}

static bool8 Notes_Claimed(const struct CoopSaveV5 *v5, u32 i)
{
    return (v5->collectionClaimed[i / 8] >> (i % 8)) & 1;
}

static void Notes_SetClaimed(struct CoopSaveV5 *v5, u32 i)
{
    v5->collectionClaimed[i / 8] |= 1 << (i % 8);
}

// The progress counts, worked out once per payout check: each count reads
// a thousand Pokédex flags, and doing it per reward cost about a frame.
struct NotesCounts
{
    u16 dex;
    u16 visitors;
    u16 visitorKinds;
};

static void Notes_Count(struct NotesCounts *c)
{
    c->dex = GetNationalPokedexCount(FLAG_GET_CAUGHT);
    c->visitors = CoopVisitor_CountCaught(&c->visitorKinds);
}

static bool8 Notes_Earned(u32 i, const struct NotesCounts *c)
{
    switch (sRewards[i].kind)
    {
    case NOTES_DEX:
        return c->dex >= sRewards[i].need;
    case NOTES_VISITORS:
        return c->visitors >= sRewards[i].need;
    case NOTES_VISITORS_ALL:
        return c->visitorKinds != 0 && c->visitors >= c->visitorKinds;
    }
    return FALSE;
}

static bool8 Notes_PendingValid(const struct CoopPendingReward *r)
{
    return r->item != ITEM_NONE && r->item < ITEMS_COUNT && r->quantity != 0 && r->quantity <= 99;
}

// A reward that had no room: into the first free place of the queue.
// FALSE when the queue is full (the caller does not claim it then).
bool8 CoopNotes_QueueReward(u16 item, u8 quantity, u8 source)
{
    struct CoopSaveV5 *v5 = Notes_Save();
    u32 i;

    if (v5 == NULL)
        return FALSE;
    for (i = 0; i < ARRAY_COUNT(v5->pending); i++)
    {
        if (!Notes_PendingValid(&v5->pending[i]))
        {
            v5->pending[i].item = item;
            v5->pending[i].quantity = quantity;
            v5->pending[i].source = source;
            return TRUE;
        }
    }
    return FALSE;
}

void CoopNotes_NoteMapEntry(void)
{
    gCoopNotesCheck = TRUE;
}

static void Notes_BufferItem(u16 item, u8 quantity, u8 *dest)
{
    u8 *p = ConvertIntToDecimalStringN(dest, quantity, STR_CONV_MODE_LEFT_ALIGN, 2);

    *p++ = CHAR_SPACE;
    CopyItemNameHandlePlural(item, p, quantity);
}

enum
{
    PAY_NONE,
    PAY_GIVEN,          // a new reward, in the BAG
    PAY_QUEUED,         // a new reward, waiting for room
    PAY_DELIVERED,      // a waiting reward, in the BAG now
};

static EWRAM_DATA u8 sPayKind = PAY_NONE;
static EWRAM_DATA bool8 sNotesPendingPageShown = FALSE;

// What to pay now (into the BAG, or the queue), with the texts for the
// script in STR_VAR_1 (why) and STR_VAR_2 (what). PAY_NONE: nothing.
static u8 Notes_Pay(void)
{
    struct CoopSaveV5 *v5 = Notes_Save();
    struct NotesCounts counts;
    u32 i;

    if (v5 == NULL)
        return PAY_NONE;
    // A reward waiting for room first, in order.
    for (i = 0; i < ARRAY_COUNT(v5->pending); i++)
    {
        struct CoopPendingReward *r = &v5->pending[i];

        if (r->item == ITEM_NONE)
            continue;
        if (!Notes_PendingValid(r))
        {
            memset(r, 0, sizeof(*r));   // never handed over: not a real reward
            continue;
        }
        if (!CheckBagHasSpace(r->item, r->quantity))
            continue;
        AddBagItem(r->item, r->quantity);
        Notes_BufferItem(r->item, r->quantity, gStringVar2);
        memset(r, 0, sizeof(*r));
        return PAY_DELIVERED;
    }
    Notes_Count(&counts);
    for (i = 0; i < ARRAY_COUNT(sRewards); i++)
    {
        const struct NotesReward *r = &sRewards[i];

        if (Notes_Claimed(v5, i) || !Notes_Earned(i, &counts))
            continue;
        switch (r->kind)
        {
        case NOTES_DEX:
            ConvertIntToDecimalStringN(gStringVar1, r->need, STR_CONV_MODE_LEFT_ALIGN, 3);
            StringAppend(gStringVar1, COMPOUND_STRING(" kinds caught!"));
            break;
        case NOTES_VISITORS:
            ConvertIntToDecimalStringN(gStringVar1, r->need, STR_CONV_MODE_LEFT_ALIGN, 3);
            StringAppend(gStringVar1, COMPOUND_STRING(" visitors caught!"));
            break;
        case NOTES_VISITORS_ALL:
            StringCopy(gStringVar1, COMPOUND_STRING("all "));
            ConvertIntToDecimalStringN(gStringVar1 + 4, counts.visitorKinds, STR_CONV_MODE_LEFT_ALIGN, 3);
            StringAppend(gStringVar1, COMPOUND_STRING(" visitors!"));
            break;
        }
        Notes_BufferItem(r->item, r->quantity, gStringVar2);
        if (CheckBagHasSpace(r->item, r->quantity))
        {
            AddBagItem(r->item, r->quantity);
            Notes_SetClaimed(v5, i);
            return PAY_GIVEN;
        }
        if (CoopNotes_QueueReward(r->item, r->quantity, COOP_REWARD_SRC_NOTES))
        {
            Notes_SetClaimed(v5, i);
            return PAY_QUEUED;
        }
        return PAY_NONE;    // the queue is full: offered again later
    }
    // A hidden habitat completed (src/coop_habitat.c).
    {
        u16 item;
        u8 quantity;

        if (CoopHabitat_PayDue(&item, &quantity, gStringVar1))
        {
            Notes_BufferItem(item, quantity, gStringVar2);
            if (CheckBagHasSpace(item, quantity))
            {
                AddBagItem(item, quantity);
                CoopHabitat_ConfirmPaid();
                return PAY_GIVEN;
            }
            if (CoopNotes_QueueReward(item, quantity, COOP_REWARD_SRC_HABITAT))
            {
                CoopHabitat_ConfirmPaid();
                return PAY_QUEUED;
            }
        }
    }
    return PAY_NONE;
}

// From CB1_Overworld (src/overworld.c), before the autosave. TRUE when a
// reward script was started (the autosave then waits).
bool8 CoopNotes_Poll(void)
{
    if (!gCoopNotesCheck || !CoopField_Calm())
        return FALSE;
    if (!CoopField_OrdinaryPlace())
    {
        gCoopNotesCheck = FALSE;
        return FALSE;
    }
    sPayKind = Notes_Pay();
    if (sPayKind == PAY_NONE)
    {
        gCoopNotesCheck = FALSE;
        return FALSE;
    }
    // One at a time: look again once this one is shown.
    gCoopNotesPaid++;
    LockPlayerFieldControls();
    ScriptContext_SetupScript(Coop_EventScript_NotesReward);
    return TRUE;
}

// Special for the script: which message (PAY_*).
u16 CoopNotes_PayKind(void)
{
    return sPayKind;
}

// ------------------------------------------------------------ the notebook

// Special. VAR_0x8004: the page. gStringVar4: its text. VAR_RESULT: FALSE
// after the last.
u16 CoopNotes_BufferPage(void)
{
    struct CoopSaveV5 *v5 = Notes_Save();
    u8 *p = gStringVar4;
    u16 total, caught;
    u32 i;

    if (gSpecialVar_0x8004 == 0)
        sNotesPendingPageShown = FALSE;
    switch (gSpecialVar_0x8004)
    {
    case 0:
        caught = GetNationalPokedexCount(FLAG_GET_CAUGHT);
        p = StringCopy(p, COMPOUND_STRING("Caught: "));
        p = ConvertIntToDecimalStringN(p, caught, STR_CONV_MODE_LEFT_ALIGN, 4);
        p = StringCopy(p, COMPOUND_STRING(" kinds of POKéMON.\n"));
        for (i = 0; i < ARRAY_COUNT(sRewards); i++)
        {
            if (sRewards[i].kind == NOTES_DEX && caught < sRewards[i].need)
                break;
        }
        if (i < ARRAY_COUNT(sRewards) && sRewards[i].kind == NOTES_DEX)
        {
            p = StringCopy(p, COMPOUND_STRING("Next reward at "));
            p = ConvertIntToDecimalStringN(p, sRewards[i].need, STR_CONV_MODE_LEFT_ALIGN, 4);
            *p++ = CHAR_PERIOD;
            *p = EOS;
        }
        else
        {
            StringCopy(p, COMPOUND_STRING("Every catch reward is yours."));
        }
        return TRUE;
    case 1:
        caught = CoopVisitor_CountCaught(&total);
        p = StringCopy(p, COMPOUND_STRING("Visitors caught: "));
        p = ConvertIntToDecimalStringN(p, caught, STR_CONV_MODE_LEFT_ALIGN, 3);
        p = StringCopy(p, COMPOUND_STRING(" of "));
        p = ConvertIntToDecimalStringN(p, total, STR_CONV_MODE_LEFT_ALIGN, 3);
        p = StringCopy(p, COMPOUND_STRING(" kinds.\nCENTER REPORTS tell where."));
        return TRUE;
    }
    // Hidden habitats: a summary, then one page each found or heard of.
    if (gSpecialVar_0x8004 >= 2 && CoopHabitat_NotesPage(gSpecialVar_0x8004 - 2, gStringVar4))
        return TRUE;
    if (sNotesPendingPageShown)
        return FALSE;
    sNotesPendingPageShown = TRUE;
    {
        if (v5 == NULL)
            return FALSE;
        for (i = 0; i < ARRAY_COUNT(v5->pending); i++)
        {
            if (Notes_PendingValid(&v5->pending[i]))
                break;
        }
        if (i == ARRAY_COUNT(v5->pending))
            return FALSE;
        p = StringCopy(p, COMPOUND_STRING("Waiting for room in the BAG:\n"));
        Notes_BufferItem(v5->pending[i].item, v5->pending[i].quantity, p);
        return TRUE;
    }
}
