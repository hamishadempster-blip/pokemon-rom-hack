// Pre-battle team preparation (build 0xC017013E).
//
// A shared story fight (gym, Elite Four, champion, rival, villain, quest
// boss) no longer takes "the first three" of each party. Once the partner
// has said YES, both players see their team for the fight: up to three
// Pokemon from any party slot, in order (LEAD, SECOND, THIRD), with READY,
// CHANGE TEAM (the party screen's team picker) and CANCEL. The fight starts
// when both are READY; nothing is healed and nothing is confirmed for
// anyone on a timeout. For the battle the chosen Pokemon move to the front
// of the party (the co-op battle code fields slots 1-3); afterwards every
// Pokemon goes back to its own slot, carrying what happened to it (HP, PP,
// items, EXP, EVs, moves, evolution).
//
// The two games keep each other informed with a small state message
// (CoopPrepMsg), sent on every change and every half second: session (the
// host's invite seq) and host id, phase, a revision that changes whenever a
// player changes their mind, and the revision the host asks to start with.
// The host starts only after seeing the partner READY at a revision, asks
// the partner to start at that revision (COMMIT) and launches on the
// partner's ACK; the partner starts only if it is still READY at that same
// revision. A message from another session, a duplicate, or a stale
// revision changes nothing.
//
// Unlinked, a gym, Elite Four or champion fight offers the lead only
// (READY / CHANGE LEAD / CANCEL); the whole party fights and the chosen lead
// stays in front, as if moved in the party menu.

#include "global.h"
#include "battle_setup.h"
#include "coop.h"
#include "coop_prep.h"
#include "event_data.h"
#include "field_message_box.h"
#include "field_screen_effect.h"
#include "item.h"
#include "link.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokemon.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/battle_frontier.h"
#include "constants/hold_effects.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define PREP_TEAM_MAX 3
#define PREP_SEND_FRAMES 30
#define PREP_LINGER_SENDS 6

struct CoopPrepMsg
{
    u16 magic;
    u16 session;
    u8 hostId;
    u8 phase;
    u8 rev;
    u8 commitRev;
    u8 reason;
    u8 count;
    u16 checksum;
};

struct CoopPrepState
{
    bool8 active;
    bool8 host;
    bool8 solo;
    bool8 inPicker;
    u8 phase;
    u8 rev;
    u8 commitRev;
    u8 partnerPhase;
    u8 partnerRev;
    u8 partnerCommitRev;
    bool8 partnerKnown;
    u8 pendingEnd;
    u8 hostId;
    u8 count;
    u16 session;
    u16 soloA;
    u16 soloB;
    u8 order[PREP_TEAM_MAX];
    u32 pid[PREP_TEAM_MAX];
    struct CoopWaitClock clock;
    u8 secs;
};

static EWRAM_DATA struct CoopPrepState sPrep = {0};
static EWRAM_DATA struct CoopPrepMsg sOut = {0};
static EWRAM_DATA bool8 sOutDirty = FALSE;
static EWRAM_DATA u8 sOutTimer = 0;
static EWRAM_DATA u8 sOutLinger = 0;
// Party order during a prepared battle: sPerm[i] is the slot the Pokemon at
// slot i came from. sBattleMap[k]: the party slot of battle slot k after
// the battle (identity for battles without a preparation).
static EWRAM_DATA u8 sPerm[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sPermCount = 0;
static EWRAM_DATA bool8 sPermActive = FALSE;
static EWRAM_DATA u8 sBattleMap[PREP_TEAM_MAX] = {0};
static EWRAM_DATA bool8 sBattleMapSet = FALSE;
// The last team chosen (personalities), offered again next time if every
// one of them can still fight.
static EWRAM_DATA u32 sRemembered[PREP_TEAM_MAX] = {0};
static EWRAM_DATA u8 sEndRefusal = 0;
static EWRAM_DATA u8 sSavedFacility = 0;
// Window ids + 1 (0: none) and the screen's task id + 1.
static EWRAM_DATA u8 sTeamWin = 0;
static EWRAM_DATA u8 sMenuWin = 0;
static EWRAM_DATA u8 sTask = 0;
static EWRAM_DATA u8 sDrawnPartner = 0;
static EWRAM_DATA u8 sDrawnSecs = 0;
static EWRAM_DATA u8 sDrawnPhase = 0;

// Test visibility.
EWRAM_DATA u8 gCoopPrepPhase = 0;
EWRAM_DATA u8 gCoopPrepPartnerPhase = 0;
EWRAM_DATA u8 gCoopPrepOrder[3] = {0};
EWRAM_DATA u8 gCoopPrepApplied = 0;
EWRAM_DATA u8 gCoopPrepMaxEntries = 0;   // set before the picker opens

static void Task_CoopPrep(u8 taskId);
static void Prep_End(u8 reason, bool8 tellPartner);

static const u8 sText_Ready[] = _("READY");
static const u8 sText_ChangeTeam[] = _("CHANGE TEAM");
static const u8 sText_ChangeLead[] = _("CHANGE LEAD");
static const u8 sText_Cancel[] = _("CANCEL");
static const struct MenuAction sMenuLinked[] = {
    {sText_Ready, {NULL}},
    {sText_ChangeTeam, {NULL}},
    {sText_Cancel, {NULL}},
};
static const struct MenuAction sMenuSolo[] = {
    {sText_Ready, {NULL}},
    {sText_ChangeLead, {NULL}},
    {sText_Cancel, {NULL}},
};
static const u8 *const sSlotLabels[PREP_TEAM_MAX] = {
    COMPOUND_STRING("LEAD"), COMPOUND_STRING("SECOND"), COMPOUND_STRING("THIRD"),
};

enum { MENU_READY, MENU_CHANGE, MENU_CANCEL };

// ---------------------------------------------------------------------------
// Team

static bool8 Prep_MonCanFight(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG)
        && GetMonData(mon, MON_DATA_HP) != 0;
}

static u8 Prep_Max(void)
{
    return sPrep.solo ? 1 : PREP_TEAM_MAX;
}

static bool8 Prep_InOrder(u8 slot)
{
    u32 i;

    for (i = 0; i < sPrep.count; i++)
    {
        if (sPrep.order[i] == slot)
            return TRUE;
    }
    return FALSE;
}

static void Prep_Add(u8 slot)
{
    if (sPrep.count >= Prep_Max() || Prep_InOrder(slot))
        return;
    sPrep.order[sPrep.count] = slot;
    sPrep.pid[sPrep.count] = GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_PERSONALITY);
    sPrep.count++;
}

// The remembered team where every member can still fight, else the first
// Pokemon able to fight in party order (the old "first three").
static void Prep_DefaultTeam(void)
{
    u32 i, slot;
    u8 found = 0;

    sPrep.count = 0;
    if (!sPrep.solo)
    {
        for (i = 0; i < PREP_TEAM_MAX && sRemembered[i] != 0; i++)
        {
            for (slot = 0; slot < PARTY_SIZE; slot++)
            {
                struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];

                if (Prep_MonCanFight(mon) && GetMonData(mon, MON_DATA_PERSONALITY) == sRemembered[i] && !Prep_InOrder(slot))
                {
                    Prep_Add(slot);
                    found++;
                    break;
                }
            }
            if (slot == PARTY_SIZE)
                break;
        }
        if (i == 0 || found != i || (i < PREP_TEAM_MAX && sRemembered[i] != 0))
            sPrep.count = 0;   // not all of them: start from the party order
    }
    for (slot = 0; slot < PARTY_SIZE && sPrep.count < Prep_Max(); slot++)
    {
        if (Prep_MonCanFight(&gParties[B_TRAINER_PLAYER][slot]))
            Prep_Add(slot);
    }
}

// Every chosen Pokemon is still in its slot and can still fight.
static bool8 Prep_TeamValid(void)
{
    u32 i;

    if (sPrep.count == 0)
        return FALSE;
    for (i = 0; i < sPrep.count; i++)
    {
        struct Pokemon *mon;

        if (sPrep.order[i] >= PARTY_SIZE)
            return FALSE;
        mon = &gParties[B_TRAINER_PLAYER][sPrep.order[i]];
        if (!Prep_MonCanFight(mon) || GetMonData(mon, MON_DATA_PERSONALITY) != sPrep.pid[i])
            return FALSE;
    }
    return TRUE;
}

static void Prep_PublishTest(void)
{
    u32 i;

    gCoopPrepPhase = sPrep.active ? sPrep.phase : COOP_PREP_NONE;
    gCoopPrepPartnerPhase = sPrep.partnerKnown ? sPrep.partnerPhase : COOP_PREP_NONE;
    for (i = 0; i < PREP_TEAM_MAX; i++)
        gCoopPrepOrder[i] = i < sPrep.count ? sPrep.order[i] + 1 : 0;
}

// ---------------------------------------------------------------------------
// Messages

static u16 Prep_Checksum(const struct CoopPrepMsg *msg)
{
    const u16 *words = (const u16 *)msg;
    u32 i, sum = 0x5A17;

    for (i = 0; i < offsetof(struct CoopPrepMsg, checksum) / 2; i++)
        sum = ((sum << 3) | (sum >> 13)) ^ words[i];
    return sum & 0xFFFF;
}

bool8 CoopPrep_IsLinkBlock(const u16 *words)
{
    const struct CoopPrepMsg *msg = (const struct CoopPrepMsg *)words;

    return msg->magic == COOP_PREP_MAGIC && msg->checksum == Prep_Checksum(msg);
}

static void Prep_Stage(void)
{
    memset(&sOut, 0, sizeof(sOut));
    sOut.magic = COOP_PREP_MAGIC;
    sOut.session = sPrep.session;
    sOut.hostId = sPrep.hostId;
    sOut.phase = sPrep.phase;
    sOut.rev = sPrep.rev;
    sOut.commitRev = sPrep.commitRev;
    sOut.reason = sPrep.pendingEnd;
    sOut.count = sPrep.count;
    sOut.checksum = Prep_Checksum(&sOut);
    sOutDirty = TRUE;
}

static bool8 Prep_TrySend(void)
{
    if (gLinkTransferringData)
        return FALSE;
    return SendBlock(BitmaskAllOtherLinkPlayers(), &sOut, sizeof(sOut));
}

// A guest still preparing for a session this host is not running: tell it
// so (it then says "interrupted" instead of waiting for nothing).
static void Prep_SendCancelFor(u16 session, u8 hostId)
{
    if (sPrep.active || sOutLinger != 0)
        return;
    memset(&sOut, 0, sizeof(sOut));
    sOut.magic = COOP_PREP_MAGIC;
    sOut.session = session;
    sOut.hostId = hostId;
    sOut.phase = COOP_PREP_CANCELLED;
    sOut.reason = COOP_PREP_END_LINK;
    sOut.checksum = Prep_Checksum(&sOut);
    sOutDirty = TRUE;
    sOutLinger = 2;
}

// ---------------------------------------------------------------------------
// Screen

static void Prep_RemoveWindows(void)
{
    if (sMenuWin != 0)
    {
        ClearStdWindowAndFrameToTransparent(sMenuWin - 1, TRUE);
        RemoveWindow(sMenuWin - 1);
        sMenuWin = 0;
    }
    if (sTeamWin != 0)
    {
        ClearStdWindowAndFrameToTransparent(sTeamWin - 1, TRUE);
        RemoveWindow(sTeamWin - 1);
        sTeamWin = 0;
    }
}

static void Prep_CloseUi(void)
{
    Prep_RemoveWindows();
    if (sTask != 0)
    {
        if (gTasks[sTask - 1].isActive && gTasks[sTask - 1].func == Task_CoopPrep)
            DestroyTask(sTask - 1);
        sTask = 0;
    }
}

static void Prep_ShowMenu(void)
{
    struct WindowTemplate template;

    if (sMenuWin != 0)
        return;
    // After the team window's tiles and before the message box's (0x194).
    template = CreateWindowTemplate(0, 18, 13, 11, 6, 0xF, 0x14E);
    sMenuWin = AddWindow(&template) + 1;
    SetStandardWindowBorderStyle(sMenuWin - 1, FALSE);
    PrintMenuTable(sMenuWin - 1, 3, sPrep.solo ? sMenuSolo : sMenuLinked);
    InitMenuInUpperLeftCornerNormal(sMenuWin - 1, 3, MENU_READY);
    CopyWindowToVram(sMenuWin - 1, COPYWIN_FULL);
}

static void Prep_HideMenu(void)
{
    if (sMenuWin == 0)
        return;
    ClearStdWindowAndFrameToTransparent(sMenuWin - 1, TRUE);
    RemoveWindow(sMenuWin - 1);
    sMenuWin = 0;
}

static const u8 *Prep_PartnerName(void)
{
    return gLinkPlayers[GetMultiplayerId() ^ 1].name;
}

static void Prep_Print(u8 win, u8 font, const u8 *str, u8 x, u8 y)
{
    AddTextPrinterParameterized(win, font, str, x, y, TEXT_SKIP_DRAW, NULL);
}

static void Prep_DrawTeam(void)
{
    u8 win, *end;
    u8 left;
    u32 i;
    u8 *buf = gStringVar4;

    if (sTeamWin == 0)
        return;
    win = sTeamWin - 1;
    FillWindowPixelBuffer(win, PIXEL_FILL(1));
    // Title: who the fight is against, and the time left.
    end = StringCopy(buf, sPrep.solo ? COMPOUND_STRING("Lead vs ") : COMPOUND_STRING("Team vs "));
    if (sPrep.solo)
    {
        *end = EOS;
        if (sPrep.soloA != TRAINER_NONE && sPrep.soloA < TRAINERS_COUNT)
            end = StringCopy(end, GetTrainerNameFromId(sPrep.soloA));
        if (sPrep.soloB != TRAINER_NONE && sPrep.soloB < TRAINERS_COUNT)
        {
            end = StringCopy(end, COMPOUND_STRING(" & "));
            StringCopy(end, GetTrainerNameFromId(sPrep.soloB));
        }
    }
    else
    {
        Coop_BufferPrepOpponents(end, sPrep.host);
    }
    Prep_Print(win, FONT_NORMAL, buf, 0, 0);
    if (sPrep.solo)
        goto rows;
    left = sPrep.secs >= COOP_PREP_SECONDS ? 0 : COOP_PREP_SECONDS - sPrep.secs;
    end = ConvertIntToDecimalStringN(buf, left / 60, STR_CONV_MODE_LEFT_ALIGN, 1);
    *end++ = CHAR_COLON;
    ConvertIntToDecimalStringN(end, left % 60, STR_CONV_MODE_LEADING_ZEROS, 2);
    Prep_Print(win, FONT_NARROW, buf, 184, 0);
rows:
    // One line per chosen Pokemon: slot role, name, level, HP, item.
    for (i = 0; i < sPrep.count; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][sPrep.order[i]];
        u16 item = GetMonData(mon, MON_DATA_HELD_ITEM);
        u8 y = 16 + i * 14;

        Prep_Print(win, FONT_NARROW, sSlotLabels[i], 0, y);
        GetMonData(mon, MON_DATA_NICKNAME, buf);
        Prep_Print(win, FONT_NARROW, buf, 38, y);
        end = StringCopy(buf, COMPOUND_STRING("Lv"));
        ConvertIntToDecimalStringN(end, GetMonData(mon, MON_DATA_LEVEL), STR_CONV_MODE_LEFT_ALIGN, 3);
        Prep_Print(win, FONT_NARROW, buf, 100, y);
        end = ConvertIntToDecimalStringN(buf, GetMonData(mon, MON_DATA_HP), STR_CONV_MODE_LEFT_ALIGN, 3);
        *end++ = CHAR_SLASH;
        ConvertIntToDecimalStringN(end, GetMonData(mon, MON_DATA_MAX_HP), STR_CONV_MODE_LEFT_ALIGN, 3);
        Prep_Print(win, FONT_NARROW, buf, 128, y);
        if (item != ITEM_NONE)
            Prep_Print(win, FONT_NARROW, GetItemHoldEffect(item) == HOLD_EFFECT_MEGA_STONE ? COMPOUND_STRING("MEGA") : COMPOUND_STRING("ITEM"),
                       176, y);
    }
    if (sPrep.solo)
    {
        Prep_Print(win, FONT_NARROW, COMPOUND_STRING("The rest of your party follows."), 0, 16 + 14);
    }
    // Status: the partner's state, or this player's wait.
    if (!sPrep.solo)
    {
        const u8 *state;

        if (sPrep.phase == COOP_PREP_READY || sPrep.phase == COOP_PREP_COMMIT)
        {
            end = StringCopy(buf, sPrep.phase == COOP_PREP_COMMIT ? COMPOUND_STRING("Starting… ") : COMPOUND_STRING("READY. Waiting for "));
            if (sPrep.phase != COOP_PREP_COMMIT)
            {
                end = StringCopyN(end, Prep_PartnerName(), PLAYER_NAME_LENGTH);
                *end = EOS;
                StringAppend(buf, COMPOUND_STRING("… B: change"));
            }
        }
        else
        {
            if (!sPrep.partnerKnown)
                state = COMPOUND_STRING(": joining…");
            else if (sPrep.partnerPhase == COOP_PREP_READY || sPrep.partnerPhase == COOP_PREP_COMMITTED)
                state = COMPOUND_STRING(": READY");
            else if (sPrep.partnerPhase == COOP_PREP_PICKING)
                state = COMPOUND_STRING(": changing team");
            else
                state = COMPOUND_STRING(": choosing");
            end = StringCopyN(buf, Prep_PartnerName(), PLAYER_NAME_LENGTH);
            *end = EOS;
            StringAppend(buf, state);
        }
        Prep_Print(win, FONT_NARROW, buf, 0, 16 + 3 * 14 + 2);
    }
    CopyWindowToVram(win, COPYWIN_FULL);
    sDrawnPartner = sPrep.partnerKnown ? sPrep.partnerPhase : 0xFE;
    sDrawnSecs = sPrep.secs;
    sDrawnPhase = sPrep.phase;
}

static void Prep_OpenScreen(void)
{
    struct WindowTemplate template;

    if (sTeamWin == 0)
    {
        template = CreateWindowTemplate(0, 1, 1, 26, 9, 0xF, 0x64);
        sTeamWin = AddWindow(&template) + 1;
        SetStandardWindowBorderStyle(sTeamWin - 1, FALSE);
    }
    Prep_DrawTeam();
    if (sPrep.phase == COOP_PREP_REVIEW)
        Prep_ShowMenu();
}

static void CB2_PrepBackFromPicker(void);

static void Prep_OpenPicker(void)
{
    u32 i;

    Prep_RemoveWindows();
    CleanupOverworldWindowsAndTilemaps();
    sSavedFacility = VarGet(VAR_FRONTIER_FACILITY);
    VarSet(VAR_FRONTIER_FACILITY, FACILITY_COOP_PREP);
    gCoopPrepMaxEntries = Prep_Max();
    sPrep.inPicker = TRUE;
    gMain.savedCallback = CB2_PrepBackFromPicker;
    InitChooseHalfPartyForBattle(0);
    // Start from the team as it stands (1-based slots).
    for (i = 0; i < sPrep.count; i++)
        gSelectedOrderFromParty[i] = sPrep.order[i] + 1;
}

static void FieldCB_PrepBack(void)
{
    LockPlayerFieldControls();
    FadeInFromBlack();
    sPrep.inPicker = FALSE;
    if (sPrep.pendingEnd != COOP_PREP_END_NONE && !sPrep.active)
    {
        u8 reason = sPrep.pendingEnd;

        sPrep.pendingEnd = COOP_PREP_END_NONE;
        Prep_End(reason, FALSE);
        return;
    }
    if (!sPrep.active)
        return;
    sTask = CreateTask(Task_CoopPrep, 80) + 1;
}

static void CB2_PrepBackFromPicker(void)
{
    u32 i;

    VarSet(VAR_FRONTIER_FACILITY, sSavedFacility);
    if (sPrep.active && gSelectedOrderFromParty[0] != 0)
    {
        sPrep.count = 0;
        for (i = 0; i < Prep_Max() && gSelectedOrderFromParty[i] != 0; i++)
        {
            u8 slot = gSelectedOrderFromParty[i] - 1;

            if (slot < PARTY_SIZE && Prep_MonCanFight(&gParties[B_TRAINER_PLAYER][slot]))
                Prep_Add(slot);
        }
        if (sPrep.count == 0)
            Prep_DefaultTeam();
    }
    if (sPrep.active)
    {
        sPrep.phase = COOP_PREP_REVIEW;
        sPrep.rev++;
        Prep_Stage();
    }
    Prep_PublishTest();
    gFieldCallback = FieldCB_PrepBack;
    CB2_ReturnToField();
}

// tState: 0 wait for the field, 1 on screen, 2 fading out to the picker.
#define tState data[0]

static void Task_CoopPrep(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (!sPrep.active)
    {
        Prep_CloseUi();
        return;
    }
    switch (task->tState)
    {
    case 0:
        if (gPaletteFade.active || gMain.callback2 != CB2_Overworld)
            return;
        Prep_OpenScreen();
        task->tState = 1;
        return;
    case 1:
        break;
    case 2:
        if (gPaletteFade.active)
            return;
        sTask = 0;
        DestroyTask(taskId);
        Prep_OpenPicker();
        return;
    }

    if (sDrawnSecs != sPrep.secs || sDrawnPhase != sPrep.phase
     || sDrawnPartner != (sPrep.partnerKnown ? sPrep.partnerPhase : 0xFE))
        Prep_DrawTeam();

    if (sPrep.phase == COOP_PREP_REVIEW && sMenuWin != 0)
    {
        s8 input = Menu_ProcessInputNoWrap();

        if (input == MENU_NOTHING_CHOSEN)
            return;
        if (input == MENU_B_PRESSED)
            input = MENU_CANCEL;
        switch (input)
        {
        case MENU_READY:
            if (!Prep_TeamValid())
            {
                PlaySE(SE_FAILURE);
                Prep_DefaultTeam();
                Prep_DrawTeam();
                return;
            }
            PlaySE(SE_SELECT);
            if (sPrep.solo)
            {
                Prep_End(COOP_PREP_END_NONE, FALSE);
                return;
            }
            Prep_HideMenu();
            sPrep.phase = COOP_PREP_READY;
            Prep_Stage();
            Prep_DrawTeam();
            return;
        case MENU_CHANGE:
            PlaySE(SE_SELECT);
            Prep_HideMenu();
            sPrep.phase = COOP_PREP_PICKING;
            sPrep.rev++;
            Prep_Stage();
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            task->tState = 2;
            return;
        default:
            PlaySE(SE_SELECT);
            Prep_End(COOP_PREP_END_SELF, TRUE);
            return;
        }
    }
    if (sPrep.phase == COOP_PREP_READY && JOY_NEW(B_BUTTON))
    {
        // Not ready after all: back to the choice (the partner sees it).
        PlaySE(SE_SELECT);
        sPrep.phase = COOP_PREP_REVIEW;
        sPrep.rev++;
        Prep_Stage();
        Prep_DrawTeam();
        Prep_ShowMenu();
    }
}

#undef tState

// ---------------------------------------------------------------------------
// Start, end

static void Prep_Start(bool8 host, bool8 solo, u16 session, u8 hostId)
{
    memset(&sPrep, 0, sizeof(sPrep));
    sPrep.active = TRUE;
    sPrep.host = host;
    sPrep.solo = solo;
    sPrep.session = session;
    sPrep.hostId = hostId;
    sPrep.phase = COOP_PREP_REVIEW;
    sPrep.rev = 1;
    sPrep.partnerPhase = COOP_PREP_NONE;
    Prep_DefaultTeam();
    Coop_WaitClockStart(&sPrep.clock);
    sDrawnSecs = sDrawnPhase = sDrawnPartner = 0xFF;
    sOutLinger = 0;
    if (!solo)
        Prep_Stage();
    LockPlayerFieldControls();
    Prep_CloseUi();
    sTask = CreateTask(Task_CoopPrep, 80) + 1;
    Prep_PublishTest();
}

void CoopPrep_BeginGuest(u16 session, u8 hostId)
{
    Prep_Start(FALSE, FALSE, session, hostId);
}

bool8 CoopPrep_BeginSolo(u16 trainerA, u16 trainerB)
{
    u32 i, able = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (Prep_MonCanFight(&gParties[B_TRAINER_PLAYER][i]))
            able++;
    }
    // Nothing to choose with one Pokemon able to fight.
    if (able < 2)
        return FALSE;
    // The trainer's line gives way to the lead screen.
    HideFieldMessageBox();
    Prep_Start(FALSE, TRUE, 0, 0);
    sPrep.soloA = trainerA;
    sPrep.soloB = trainerB;
    return TRUE;
}

bool8 CoopPrep_Active(void)
{
    return sPrep.active;
}

bool8 CoopPrep_HostCommitted(void)
{
    return sPrep.active && sPrep.host && sPrep.phase == COOP_PREP_COMMIT;
}

static u8 Prep_Refusal(u8 reason)
{
    switch (reason)
    {
    case COOP_PREP_END_SELF:
        return COOP_REFUSE_PREP_SELF;
    case COOP_PREP_END_PARTNER:
        return COOP_REFUSE_PREP_PARTNER;
    case COOP_PREP_END_TIMEOUT:
        return COOP_REFUSE_PREP_TIMEOUT;
    default:
        return COOP_REFUSE_PREP_STOPPED;
    }
}

// The preparation is over without a linked fight: the solo READY starts
// the solo fight; any other end says why and frees the field.
static void Prep_End(u8 reason, bool8 tellPartner)
{
    bool8 host = sPrep.host, solo = sPrep.solo;

    if (sPrep.active && tellPartner && !solo)
    {
        sPrep.phase = COOP_PREP_CANCELLED;
        sPrep.pendingEnd = reason;
        Prep_Stage();
        sOutLinger = PREP_LINGER_SENDS;
    }
    sPrep.active = FALSE;
    if (sPrep.inPicker)
    {
        // Back from the party screen first; FieldCB_PrepBack finishes this.
        sPrep.pendingEnd = reason == COOP_PREP_END_NONE ? COOP_PREP_END_LINK : reason;
        Prep_PublishTest();
        return;
    }
    Prep_CloseUi();
    Prep_PublishTest();
    if (solo)
    {
        if (reason == COOP_PREP_END_NONE && Prep_TeamValid())
        {
            // The chosen lead moves to the front for good (a party-menu
            // switch), then the stock solo fight starts as before.
            u8 slot = sPrep.order[0];

            if (slot != 0)
            {
                struct Pokemon tmp = gParties[B_TRAINER_PLAYER][0];

                gParties[B_TRAINER_PLAYER][0] = gParties[B_TRAINER_PLAYER][slot];
                gParties[B_TRAINER_PLAYER][slot] = tmp;
            }
            gSpecialVar_Result = TRUE;
            BattleSetup_StartSoloPairBattle(sPrep.soloA, sPrep.soloB);
            return;
        }
        Coop_PrepSoloEnd(Prep_Refusal(reason == COOP_PREP_END_NONE ? COOP_PREP_END_INVALID : reason));
        return;
    }
    if (host)
    {
        Coop_PrepHostEnd(Prep_Refusal(reason));
        return;
    }
    sEndRefusal = Prep_Refusal(reason);
    ScriptContext_SetupScript(Coop_EventScript_PrepEnded);
}

u16 CoopPrep_TakeEndReason(void)
{
    u16 refusal = sEndRefusal;

    sEndRefusal = 0;
    return refusal;
}

void CoopPrep_PartnerWithdrew(u16 session)
{
    if (sPrep.active && !sPrep.host && !sPrep.solo && sPrep.session == session
     && sPrep.phase != COOP_PREP_COMMITTED)
        Prep_End(COOP_PREP_END_PARTNER, FALSE);
}

void CoopPrep_LinkDown(void)
{
    if (sPrep.active && !sPrep.solo && sPrep.phase != COOP_PREP_COMMITTED)
        Prep_End(COOP_PREP_END_LINK, FALSE);
    sOutLinger = 0;
    sOutDirty = FALSE;
}

// ---------------------------------------------------------------------------
// Link

void CoopPrep_HandleBlock(u32 playerId, const u16 *words)
{
    const struct CoopPrepMsg *msg = (const struct CoopPrepMsg *)words;

    if (playerId == GetMultiplayerId() || !CoopPrep_IsLinkBlock(words) || msg->phase > COOP_PREP_CANCELLED)
        return;
    if (!sPrep.active)
    {
        // The partner agreed to this host's waiting story invite and is
        // choosing: this game chooses too.
        if (msg->hostId == GetMultiplayerId() && msg->phase != COOP_PREP_CANCELLED && msg->phase != COOP_PREP_COMMITTED)
        {
            if (Coop_PrepHostCanStart(msg->session))
            {
                Coop_PrepHostStarted();
                Prep_Start(TRUE, FALSE, msg->session, GetMultiplayerId());
            }
            else
            {
                Prep_SendCancelFor(msg->session, msg->hostId);
                return;
            }
        }
        else
        {
            return;
        }
    }
    if (sPrep.solo || msg->session != sPrep.session || msg->hostId != sPrep.hostId)
        return;
    sPrep.partnerKnown = TRUE;
    sPrep.partnerPhase = msg->phase;
    sPrep.partnerRev = msg->rev;
    sPrep.partnerCommitRev = msg->commitRev;
    if (msg->phase == COOP_PREP_CANCELLED && sPrep.phase != COOP_PREP_COMMITTED)
        Prep_End(msg->reason == COOP_PREP_END_TIMEOUT ? COOP_PREP_END_TIMEOUT
               : msg->reason == COOP_PREP_END_SELF ? COOP_PREP_END_PARTNER : COOP_PREP_END_LINK, FALSE);
    Prep_PublishTest();
}

void CoopPrep_Poll(void)
{
    if (sPrep.active && !sPrep.solo)
    {
        sPrep.secs = Coop_WaitClockTick(&sPrep.clock);
        if (sPrep.secs >= COOP_PREP_SECONDS && sPrep.phase != COOP_PREP_COMMITTED)
        {
            Prep_End(COOP_PREP_END_TIMEOUT, TRUE);
        }
        else if (sPrep.host)
        {
            // Both READY: ask the partner to start at the revision it is
            // ready at. Anything else from the partner before its start
            // (a change of team, a newer revision) cancels the ask.
            // (Not while the partner's overworld frames have stopped: a
            // fight is only asked for with the partner visibly there.)
            if (sPrep.phase == COOP_PREP_READY && sPrep.partnerKnown && sPrep.partnerPhase == COOP_PREP_READY
             && Prep_TeamValid() && Coop_PrepPartnerHere())
            {
                sPrep.phase = COOP_PREP_COMMIT;
                sPrep.commitRev = sPrep.partnerRev;
                Prep_Stage();
            }
            else if (sPrep.phase == COOP_PREP_COMMIT && sPrep.partnerPhase != COOP_PREP_COMMITTED
                  && (sPrep.partnerPhase != COOP_PREP_READY || sPrep.partnerRev != sPrep.commitRev))
            {
                sPrep.phase = COOP_PREP_READY;
                sPrep.commitRev = 0;
                Prep_Stage();
            }
            else if (sPrep.phase == COOP_PREP_READY && !Prep_TeamValid())
            {
                // The team cannot be used any more: choose again.
                sPrep.phase = COOP_PREP_REVIEW;
                sPrep.rev++;
                Prep_DefaultTeam();
                Prep_Stage();
            }
        }
        else if (sPrep.phase == COOP_PREP_READY && sPrep.partnerPhase == COOP_PREP_COMMIT
              && sPrep.partnerCommitRev == sPrep.rev && !sPrep.inPicker && gMain.callback2 == CB2_Overworld)
        {
            if (Prep_TeamValid())
            {
                // (No message for this: the ACK that starts the host's
                // battle is the answer.)
                sPrep.phase = COOP_PREP_COMMITTED;
                sOutDirty = FALSE;
                Prep_PublishTest();
                Coop_PrepGuestCommit();
                return;
            }
            sPrep.phase = COOP_PREP_REVIEW;
            sPrep.rev++;
            Prep_DefaultTeam();
            Prep_Stage();
        }
    }
    // Send: on a change, then every half second while preparing; a few
    // more times after a cancel so it is not missed.
    if ((sPrep.active && !sPrep.solo) || sOutLinger != 0)
    {
        if (gMain.inBattle || gCoopBattleLaunching)
            return;
        if (sOutTimer < 0xFF)
            sOutTimer++;
        if ((sOutDirty || sOutTimer >= PREP_SEND_FRAMES) && Prep_TrySend())
        {
            sOutDirty = FALSE;
            sOutTimer = 0;
            if (!sPrep.active && sOutLinger != 0)
                sOutLinger--;
        }
    }
    Prep_PublishTest();
}

// ---------------------------------------------------------------------------
// Battle

// Inside Coop_LaunchBattle, for every co-op fight: a prepared team moves to
// the front of the party; otherwise the order stays as it is.
void CoopPrep_OnLaunch(void)
{
    u32 k;

    if (sPermActive)
        CoopPrep_UndoTeam();   // a launch that never reached the battle
    for (k = 0; k < PREP_TEAM_MAX; k++)
        sBattleMap[k] = k;
    sBattleMapSet = TRUE;
    gCoopPrepApplied = 0;
    if (!sPrep.active || sPrep.solo)
        return;
    if ((sPrep.phase == COOP_PREP_READY || sPrep.phase == COOP_PREP_COMMIT || sPrep.phase == COOP_PREP_COMMITTED)
     && Prep_TeamValid())
        CoopPrep_ApplyTeam();
    Prep_CloseUi();
    sPrep.active = FALSE;
    sOutDirty = FALSE;
    sOutLinger = 0;
    Prep_PublishTest();
}

void CoopPrep_ApplyTeam(void)
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    struct Pokemon *tmp;
    u8 count = CalculatePlayerPartyCount();
    u32 i, n = 0;

    tmp = Alloc(sizeof(struct Pokemon) * PARTY_SIZE);
    if (tmp == NULL)
        return;
    memcpy(tmp, party, sizeof(struct Pokemon) * PARTY_SIZE);
    for (i = 0; i < sPrep.count; i++)
        sPerm[n++] = sPrep.order[i];
    for (i = 0; i < count; i++)
    {
        if (!Prep_InOrder(i))
            sPerm[n++] = i;
    }
    for (i = 0; i < n; i++)
        party[i] = tmp[sPerm[i]];
    Free(tmp);
    sPermCount = n;
    sPermActive = TRUE;
    for (i = 0; i < PREP_TEAM_MAX; i++)
    {
        sBattleMap[i] = i < n ? sPerm[i] : i;
        sRemembered[i] = i < sPrep.count ? sPrep.pid[i] : 0;
    }
    gCoopPrepApplied = sPrep.count;
}

// After the battle (or an aborted launch): every Pokemon back in the slot
// it came from, with whatever the battle did to it.
void CoopPrep_UndoTeam(void)
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    struct Pokemon *tmp;
    u32 i;

    if (!sPermActive)
        return;
    tmp = Alloc(sizeof(struct Pokemon) * PARTY_SIZE);
    if (tmp == NULL)
        return;
    memcpy(tmp, party, sizeof(struct Pokemon) * PARTY_SIZE);
    for (i = 0; i < sPermCount; i++)
        party[sPerm[i]] = tmp[i];
    Free(tmp);
    sPermActive = FALSE;
    CalculatePlayerPartyCount();
}

u8 CoopPrep_PartySlotForBattleSlot(u8 battleSlot)
{
    if (battleSlot >= PREP_TEAM_MAX || !sBattleMapSet)
        return battleSlot;
    return sBattleMap[battleSlot];
}
