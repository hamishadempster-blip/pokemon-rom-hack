// "Who can learn this?" (collection update §11).
//
// In the BAG's TM/HM pocket, CAN LEARN on a TM or HM searches this save's
// own party and every PC box for Pokémon that can learn its move and do
// not know it yet, and says how many, then where, a page at a time (A for
// the next page, B to stop): "PARTY: Mudkip, Taillow" / "BOX 3: Geodude,
// Nosepass, ...". A page that does not hold a whole box goes on in the next
// one ("BOX 3, more: ..."). It only reads: nothing is taught, moved or
// changed, and the partner's Pokémon are not searched (they are on the
// other game). The rule is the party menu's own (CanTeachMove in
// src/party_menu.c): the species' teachable moves, not an egg, not known.
#include "global.h"
#include "coop_learn.h"
#include "move.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "string_util.h"
#include "text.h"
#include "constants/characters.h"

#define LINE_WIDTH 200      // pixels, the bag's message box
#define PAGE_LINES 2
#define LOC_PARTY  0xFF     // sLearn.box while in the party

static EWRAM_DATA struct {
    u16 move;
    u8 box;             // LOC_PARTY, then 0 .. TOTAL_BOXES_COUNT - 1
    u8 pos;             // the next slot to look at
    u16 learnable;
    u16 learnableParty;
    u16 knowing;
    bool8 continued;    // the page goes on with the location of the last
} sLearn = {0};

// The Pokémon at a place, or NULL when there is none.
static struct BoxPokemon *Learn_MonAt(u8 box, u8 pos)
{
    if (box == LOC_PARTY)
        return pos < PARTY_SIZE ? &gParties[B_TRAINER_PLAYER][pos].box : NULL;
    return pos < IN_BOX_COUNT ? GetBoxedMonPtr(box, pos) : NULL;
}

static bool8 Learn_Knows(struct BoxPokemon *mon)
{
    u32 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetBoxMonData(mon, MON_DATA_MOVE1 + i) == sLearn.move)
            return TRUE;
    }
    return FALSE;
}

// 0 nothing, 1 can learn it, 2 knows it already.
static u8 Learn_Check(struct BoxPokemon *mon)
{
    u16 species;

    if (mon == NULL || !GetBoxMonData(mon, MON_DATA_SANITY_HAS_SPECIES) || GetBoxMonData(mon, MON_DATA_IS_EGG))
        return 0;
    species = GetBoxMonData(mon, MON_DATA_SPECIES);
    if (!CanLearnTeachableMove(species, sLearn.move))
        return 0;
    return Learn_Knows(mon) ? 2 : 1;
}

void CoopLearn_Begin(u16 move)
{
    u8 box, pos, result;

    sLearn.move = move;
    sLearn.box = LOC_PARTY;
    sLearn.pos = 0;
    sLearn.learnable = sLearn.learnableParty = sLearn.knowing = 0;
    sLearn.continued = FALSE;
    for (pos = 0; pos < PARTY_SIZE; pos++)
    {
        result = Learn_Check(&gParties[B_TRAINER_PLAYER][pos].box);
        sLearn.learnable += (result == 1);
        sLearn.learnableParty += (result == 1);
        sLearn.knowing += (result == 2);
    }
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            result = Learn_Check(GetBoxedMonPtr(box, pos));
            sLearn.learnable += (result == 1);
            sLearn.knowing += (result == 2);
        }
    }
}

// "5 of your POKéMON can learn ROAR\n(2 in the party). 1 knows it already."
void CoopLearn_Summary(u8 *dest)
{
    u8 *p = dest;

    if (sLearn.learnable == 0)
    {
        p = StringCopy(p, COMPOUND_STRING("None of your POKéMON can learn\n"));
        p = StringCopy(p, GetMoveName(sLearn.move));
        if (sLearn.knowing != 0)
        {
            p = StringCopy(p, COMPOUND_STRING(" now. "));
            p = ConvertIntToDecimalStringN(p, sLearn.knowing, STR_CONV_MODE_LEFT_ALIGN, 3);
            p = StringCopy(p, sLearn.knowing == 1 ? COMPOUND_STRING(" knows it.") : COMPOUND_STRING(" know it."));
        }
        else
        {
            *p++ = CHAR_PERIOD;
            *p = EOS;
        }
        return;
    }
    p = ConvertIntToDecimalStringN(p, sLearn.learnable, STR_CONV_MODE_LEFT_ALIGN, 3);
    p = StringCopy(p, COMPOUND_STRING(" of your POKéMON can learn "));
    p = StringCopy(p, GetMoveName(sLearn.move));
    p = StringCopy(p, COMPOUND_STRING("\n("));
    p = ConvertIntToDecimalStringN(p, sLearn.learnableParty, STR_CONV_MODE_LEFT_ALIGN, 1);
    p = StringCopy(p, COMPOUND_STRING(" in the party)."));
    if (sLearn.knowing != 0)
    {
        *p++ = CHAR_SPACE;
        p = ConvertIntToDecimalStringN(p, sLearn.knowing, STR_CONV_MODE_LEFT_ALIGN, 3);
        p = StringCopy(p, sLearn.knowing == 1 ? COMPOUND_STRING(" knows it.") : COMPOUND_STRING(" know it."));
    }
}

static void Learn_LocationName(u8 *dest, u8 box)
{
    if (box == LOC_PARTY)
        StringCopy(dest, COMPOUND_STRING("PARTY"));
    else
        StringCopy(dest, GetBoxNamePtr(box));
}

// Moves the cursor to the next Pokémon that can learn the move. FALSE at the end.
static bool8 Learn_Seek(void)
{
    while (sLearn.box == LOC_PARTY || sLearn.box < TOTAL_BOXES_COUNT)
    {
        u8 count = sLearn.box == LOC_PARTY ? PARTY_SIZE : IN_BOX_COUNT;

        for (; sLearn.pos < count; sLearn.pos++)
        {
            if (Learn_Check(Learn_MonAt(sLearn.box, sLearn.pos)) == 1)
                return TRUE;
        }
        sLearn.box = (sLearn.box == LOC_PARTY) ? 0 : sLearn.box + 1;
        sLearn.pos = 0;
        sLearn.continued = FALSE;
    }
    return FALSE;
}

// The next page: one place's Pokémon that can learn the move, as many as
// fit in two lines. FALSE when every one has been shown.
bool8 CoopLearn_NextPage(u8 *dest)
{
    u8 name[POKEMON_NAME_LENGTH + 1];
    u8 line[64];
    u8 *p = dest;
    u8 box, lines = 0, onLine = 0;

    if (!Learn_Seek())
        return FALSE;
    box = sLearn.box;
    Learn_LocationName(line, box);
    p = StringCopy(p, line);
    if (sLearn.continued)
        p = StringCopy(p, COMPOUND_STRING(", more"));
    p = StringCopy(p, COMPOUND_STRING(": "));
    *p = EOS;
    while (sLearn.box == box && Learn_Seek() && sLearn.box == box)
    {
        u8 *lineStart;
        u8 *save = p;

        GetBoxMonData(Learn_MonAt(sLearn.box, sLearn.pos), MON_DATA_NICKNAME, name);
        StringGet_Nickname(name);
        if (onLine != 0)
            p = StringCopy(p, COMPOUND_STRING(", "));
        p = StringCopy(p, name);
        // Width of the line being written.
        lineStart = dest;
        {
            u8 *q;
            for (q = dest; q < p; q++)
                if (*q == CHAR_NEWLINE)
                    lineStart = q + 1;
        }
        StringCopyN(line, lineStart, min(p - lineStart, (int)sizeof(line) - 1));
        line[min(p - lineStart, (int)sizeof(line) - 1)] = EOS;
        if (GetStringWidth(FONT_NORMAL, line, 0) > LINE_WIDTH)
        {
            // Does not fit: end this line, and the page after two lines.
            p = save;
            if (onLine != 0)
                p = StringCopy(p, COMPOUND_STRING(","));
            *p = EOS;
            if (++lines == PAGE_LINES)
            {
                sLearn.continued = TRUE;
                return TRUE;
            }
            *p++ = CHAR_NEWLINE;
            *p = EOS;
            onLine = 0;
            continue;   // the same Pokémon again, on the new line
        }
        onLine++;
        sLearn.pos++;
    }
    *p = EOS;
    return TRUE;
}
