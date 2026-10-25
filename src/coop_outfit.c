// Player outfit colours.
//
// Each save picks one of six colour presets for its player base (Brendan or
// May) at the dresser in either Littleroot bedroom. Only clothing palette
// indices change, so every pose drawn with the player palette (walking,
// running, bikes, surfing, field moves, fishing, watering, decorating) and
// the player's trainer pics (battle back pic, trainer card) match.
//
// Palettes:
//  - OBJ_EVENT_PAL_TAG_BRENDAN, _MAY and their reflections are now used only
//    by the player's own looks (Birch's twins have their own tags), so they
//    carry this save's outfit.
//  - The partner is drawn with OBJ_EVENT_GFX_COOP_PARTNER_* graphics, which
//    use OBJ_EVENT_PAL_TAG_COOP_PARTNER: one palette slot holding the
//    partner's base and outfit. Two players with the same base and
//    different outfits therefore show both at once.
//  - In a co-op battle the link partner's back pic gets its own palette tag.
//
// The partner's choice travels as a DMSG_LOOK deal block, sent at every new
// link and whenever the choice changes. Until it arrives the partner is
// drawn in CLASSIC. When both players have the same base and outfit, a small
// marker over the partner tells them apart (AUTO); it can also be ALWAYS on
// or OFF (CoopSave.partnerMarker).

#include "global.h"
#include "coop.h"
#include "coop_deal.h"
#include "coop_outfit.h"
#include "battle.h"
#include "battle_controllers.h"
#include "data.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "link.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "palette.h"
#include "constants/rgb.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "trainer.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/coop.h"
#include "constants/flags.h"
#include "constants/vars.h"
#include "constants/event_objects.h"
#include "constants/songs.h"
#include "constants/trainers.h"

#include "data/coop_outfits.h"

enum { KIND_OW, KIND_REFLECTION, KIND_TRAINER };

#define TAG_MARKER_TILES   0xC0B3
#define TAG_PARTNER_BACK   0xC0B4
#define TAG_MARKER_PAL     0xC0B5

static EWRAM_DATA u8 sPartnerOutfit = 0;
static EWRAM_DATA u8 sPartnerGender = MALE;
static EWRAM_DATA bool8 sPartnerKnown = FALSE;
static EWRAM_DATA bool8 sPreviewActive = FALSE;
static EWRAM_DATA u8 sPreviewOutfit = 0;
// What the loaded palette slots show. Loads always use the current choice;
// these only spot a change that slots already in use must pick up.
static EWRAM_DATA u8 sShownLocal = 0;         // outfit
static EWRAM_DATA u8 sShownPartner = 0;       // (gender << 4) | outfit
// Checked against its template before use, so 0 at boot is safe.
static EWRAM_DATA u8 sMarkerSpriteId = 0;
static EWRAM_DATA u8 sMarkerTimer = 0;
EWRAM_DATA bool8 gCoopMarkerShown = FALSE;
EWRAM_DATA bool8 gCoopCompassShown = FALSE;  // the edge compass is on screen (read by tests)
static EWRAM_DATA u8 sShownStyle = 0;        // marker palette style + 1 last loaded   // the partner marker is on screen (read by tests)

static u8 Base(u8 gender)
{
    return gender == FEMALE ? 1 : 0;
}

static u8 SavedOutfit(void)
{
    struct CoopSave *save = Coop_GetSave();

    if (save == NULL || save->outfit >= COOP_OUTFIT_COUNT)
        return 0;
    return save->outfit;
}

u8 CoopOutfit_Local(void)
{
    if (sPreviewActive)
        return sPreviewOutfit;
    return SavedOutfit();
}

static u8 MarkerMode(void)
{
    struct CoopSave *save = Coop_GetSave();

    if (save == NULL || save->partnerMarker > COOP_MARKER_OFF)
        return COOP_MARKER_AUTO;
    return save->partnerMarker;
}

const u16 *CoopOutfit_ObjectPalette(u16 tag)
{
    switch (tag)
    {
    case OBJ_EVENT_PAL_TAG_BRENDAN:
        return sCoopOutfitPals[0][KIND_OW][CoopOutfit_Local()];
    case OBJ_EVENT_PAL_TAG_MAY:
        return sCoopOutfitPals[1][KIND_OW][CoopOutfit_Local()];
    case OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION:
        return sCoopOutfitPals[0][KIND_REFLECTION][CoopOutfit_Local()];
    case OBJ_EVENT_PAL_TAG_MAY_REFLECTION:
        return sCoopOutfitPals[1][KIND_REFLECTION][CoopOutfit_Local()];
    case OBJ_EVENT_PAL_TAG_COOP_PARTNER:
        return sCoopOutfitPals[Base(sPartnerGender)][KIND_OW][sPartnerKnown ? sPartnerOutfit : 0];
    }
    return NULL;
}

const u16 *CoopOutfit_TrainerPalette(u16 trainerPic)
{
    if (trainerPic == TRAINER_PIC_BRENDAN)
        return sCoopOutfitPals[0][KIND_TRAINER][CoopOutfit_Local()];
    if (trainerPic == TRAINER_PIC_MAY)
        return sCoopOutfitPals[1][KIND_TRAINER][CoopOutfit_Local()];
    return NULL;
}

// The partner in a co-op battle (ally back pic) or a friendly battle
// (opponent front pic) shares TRAINER_PIC_BRENDAN or _MAY with this player,
// and so its palette tag. Give its sprite the partner's colours under a tag
// of its own. Front and back pics share one palette.
void CoopOutfit_BattleTrainerSprite(u32 battler, u32 spriteId, u32 trainerPic)
{
    struct SpritePalette pal;
    u8 slot;

    if (spriteId >= MAX_SPRITES || !(gBattleTypeFlags & BATTLE_TYPE_LINK) || !COOP_SESSION_LINK_BATTLE())
        return;
    if (GetBattlerMultiplayerId(battler) == GetMultiplayerId())
        return;
    if (trainerPic != TRAINER_PIC_BRENDAN && trainerPic != TRAINER_PIC_MAY)
        return;
    pal.data = sCoopOutfitPals[trainerPic == TRAINER_PIC_MAY ? 1 : 0][KIND_TRAINER][sPartnerKnown ? sPartnerOutfit : 0];
    pal.tag = TAG_PARTNER_BACK;
    slot = IndexOfSpritePaletteTag(TAG_PARTNER_BACK);
    if (slot == 0xFF)
        slot = LoadSpritePalette(&pal);
    else
        LoadSpritePaletteInSlot(&pal, slot);
    if (slot != 0xFF)
        gSprites[spriteId].oam.paletteNum = slot;
}

u16 CoopOutfit_PartnerGfx(bool8 female, u8 pose)
{
    u16 base = female ? OBJ_EVENT_GFX_COOP_PARTNER_MAY_NORMAL : OBJ_EVENT_GFX_COOP_PARTNER_BRENDAN_NORMAL;
    return base + pose;
}

// Push the current choices into palette slots that are already loaded, so a
// change shows at once (the next load picks them up anyway).
static void ReloadTag(u16 tag)
{
    struct SpritePalette pal;
    u8 slot = IndexOfSpritePaletteTag(tag);

    if (slot == 0xFF)
        return;
    pal.data = CoopOutfit_ObjectPalette(tag);
    pal.tag = tag;
    LoadSpritePaletteInSlot(&pal, slot);
}

void CoopOutfit_RefreshLoaded(void)
{
    ReloadTag(OBJ_EVENT_PAL_TAG_BRENDAN);
    ReloadTag(OBJ_EVENT_PAL_TAG_MAY);
    ReloadTag(OBJ_EVENT_PAL_TAG_BRENDAN_REFLECTION);
    ReloadTag(OBJ_EVENT_PAL_TAG_MAY_REFLECTION);
    ReloadTag(OBJ_EVENT_PAL_TAG_COOP_PARTNER);
    sShownLocal = CoopOutfit_Local();
    sShownPartner = (sPartnerGender << 4) | (sPartnerKnown ? sPartnerOutfit : 0);
}

void CoopOutfit_OnPartnerLook(u8 outfit, u8 gender)
{
    sPartnerOutfit = outfit < COOP_OUTFIT_COUNT ? outfit : 0;
    sPartnerGender = gender == FEMALE ? FEMALE : MALE;
    sPartnerKnown = TRUE;
}

void CoopOutfit_OnSessionStart(void)
{
    sPartnerKnown = FALSE;
    sPartnerOutfit = 0;
    CoopDeal_QueueLook();
}

// ------------------------------------------------------------ partner marker

static const u32 sMarkerTiles[] = {
    // Tile 0, 8x8 4bpp: a small down arrow (14 fill, 15 outline).
    0x0FFFFFFF,
    0x0FEEEEEF,
    0x00FEEEF0,
    0x000FEF00,
    0x0000F000,
    0x00000000,
    0x00000000,
    0x00000000,
    // Tile 1: a diamond, the compass at the screen edge.
    0x0000F000,
    0x000FEF00,
    0x00FEDEF0,
    0x0FEDDDEF,
    0x00FEDEF0,
    0x000FEF00,
    0x0000F000,
    0x00000000,
};

static const struct SpriteSheet sMarkerSheet = {sMarkerTiles, sizeof(sMarkerTiles), TAG_MARKER_TILES};

// The marker's own palette: CLASSIC, then the Beacon styles the Twin Beacon
// shrine unlocks. Index 13 highlight, 14 fill, 15 outline.
#define RGB_(r, g, b) RGB((r) >> 3, (g) >> 3, (b) >> 3)
static const u16 sMarkerPals[BEACON_STYLE_COUNT][16] = {
    { [13] = RGB_(200, 200, 216), [14] = RGB_(255, 255, 255), [15] = RGB_(16, 16, 24) },
    { [13] = RGB_(255, 246, 180), [14] = RGB_(255, 206, 66), [15] = RGB_(120, 72, 0) },
    { [13] = RGB_(200, 236, 255), [14] = RGB_(98, 180, 255), [15] = RGB_(16, 48, 128) },
    { [13] = RGB_(255, 214, 230), [14] = RGB_(255, 128, 180), [15] = RGB_(128, 24, 72) },
};

static const struct OamData sMarkerOam = {
    .shape = SPRITE_SHAPE(8x8),
    .size = SPRITE_SIZE(8x8),
    .priority = 1,
};

static const struct SpriteTemplate sMarkerTemplate = {
    .tileTag = TAG_MARKER_TILES,
    .paletteTag = TAG_MARKER_PAL,
    .oam = &sMarkerOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static bool8 MarkerAlive(void)
{
    return sMarkerSpriteId < MAX_SPRITES && gSprites[sMarkerSpriteId].inUse
        && gSprites[sMarkerSpriteId].template == &sMarkerTemplate;
}

static void RemoveMarker(void)
{
    if (MarkerAlive())
        DestroySprite(&gSprites[sMarkerSpriteId]);
    sMarkerSpriteId = MAX_SPRITES;
}

static u8 MarkerStyle(void)
{
    u16 style = VarGet(VAR_BEACON_STYLE);

    if (!FlagGet(FLAG_SHRINE_LORE_READ) || style >= BEACON_STYLE_COUNT)
        return 0;
    return style;
}

static bool8 WantMarker(u8 gender)
{
    switch (MarkerMode())
    {
    case COOP_MARKER_ALWAYS:
        return TRUE;
    case COOP_MARKER_OFF:
        return FALSE;
    default:
        return gender == gSaveBlock2Ptr->playerGender
            && (sPartnerKnown ? sPartnerOutfit : 0) == CoopOutfit_Local();
    }
}

static struct Sprite *EnsureMarker(void)
{
    struct SpritePalette pal;
    u8 slot;

    pal.data = sMarkerPals[MarkerStyle()];
    pal.tag = TAG_MARKER_PAL;
    slot = IndexOfSpritePaletteTag(TAG_MARKER_PAL);
    if (slot == 0xFF)
        slot = LoadSpritePalette(&pal);
    else if (sShownStyle != MarkerStyle() + 1)
        LoadSpritePaletteInSlot(&pal, slot);
    if (slot == 0xFF)
        return NULL;
    sShownStyle = MarkerStyle() + 1;
    if (!MarkerAlive())
    {
        if (GetSpriteTileStartByTag(TAG_MARKER_TILES) == 0xFFFF)
            LoadSpriteSheet(&sMarkerSheet);
        sMarkerSpriteId = CreateSprite(&sMarkerTemplate, 0, 0, 0);
        if (sMarkerSpriteId >= MAX_SPRITES)
            return NULL;
    }
    gSprites[sMarkerSpriteId].oam.paletteNum = slot;
    return &gSprites[sMarkerSpriteId];
}

static void HideMarker(void)
{
    RemoveMarker();
    gCoopMarkerShown = FALSE;
    gCoopCompassShown = FALSE;
}

// Over the partner's head: when looks match (AUTO), or ALWAYS. With the
// shrine's compass, a diamond at the screen edge points to a partner on
// this map who is out of sight.
static void UpdateMarker(void)
{
    u8 gender;
    u8 partnerSprite = Coop_PartnerSpriteId(&gender);
    struct Sprite *partner, *marker;
    s16 px, py, dx, dy;
    bool8 offScreen;

    gCoopCompassShown = FALSE;
    // Out of sight: no sprite, or a sprite beyond the 15x10 tile screen.
    offScreen = partnerSprite >= MAX_SPRITES;
    if (Coop_PartnerMapPos(&px, &py))
    {
        dx = px - gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x;
        dy = py - gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y;
        if (dx < -7 || dx > 7 || dy < -5 || dy > 4)
            offScreen = TRUE;
    }
    else
    {
        dx = dy = 0;
    }
    if (offScreen)
    {
        if (!FlagGet(FLAG_SHRINE_LORE_READ) || MarkerMode() == COOP_MARKER_OFF
         || !Coop_PartnerMapPos(&px, &py) || ArePlayerFieldControlsLocked())
        {
            HideMarker();
            return;
        }
        marker = EnsureMarker();
        if (marker == NULL)
            return;
        marker->oam.tileNum = GetSpriteTileStartByTag(TAG_MARKER_TILES) + 1;
        marker->coordOffsetEnabled = FALSE;
        marker->x = 120 + dx * 16;
        marker->y = 80 + dy * 16;
        if (marker->x < 6) marker->x = 6;
        if (marker->x > DISPLAY_WIDTH - 6) marker->x = DISPLAY_WIDTH - 6;
        if (marker->y < 6) marker->y = 6;
        if (marker->y > DISPLAY_HEIGHT - 6) marker->y = DISPLAY_HEIGHT - 6;
        marker->invisible = ((++sMarkerTimer >> 4) & 3) == 0;
        gCoopMarkerShown = FALSE;
        gCoopCompassShown = TRUE;
        return;
    }
    if (!WantMarker(gender))
    {
        HideMarker();
        return;
    }
    marker = EnsureMarker();
    if (marker == NULL)
    {
        gCoopMarkerShown = FALSE;
        return;
    }
    partner = &gSprites[partnerSprite];
    marker->oam.tileNum = GetSpriteTileStartByTag(TAG_MARKER_TILES);
    // Map coordinates, like the object sprites (camera offset applied).
    marker->coordOffsetEnabled = partner->coordOffsetEnabled;
    marker->x = partner->x + partner->x2;
    marker->y = partner->y + partner->y2 - 10 + ((++sMarkerTimer >> 5) & 1);
    marker->invisible = partner->invisible;
    gCoopMarkerShown = !marker->invisible;
}

void CoopOutfit_FieldFrame(void)
{
    u8 gender;
    u8 shown;

    if (Coop_PartnerSpriteId(&gender) < MAX_SPRITES && gender != sPartnerGender)
        sPartnerGender = gender;
    shown = (sPartnerGender << 4) | (sPartnerKnown ? sPartnerOutfit : 0);
    if (sShownLocal != CoopOutfit_Local() || sShownPartner != shown)
        CoopOutfit_RefreshLoaded();
    UpdateMarker();
}

// --------------------------------------------------------------- dresser

#define MENU_ITEMS (COOP_OUTFIT_COUNT + 2)
#define ITEM_MARKER COOP_OUTFIT_COUNT
#define ITEM_EXIT (COOP_OUTFIT_COUNT + 1)

#define tListTaskId data[0]
#define tWindowId   data[1]
#define tPicSprite  data[2]
#define tOriginal   data[3]

static EWRAM_DATA struct ListMenuItem *sMenuItems = NULL;
static EWRAM_DATA u8 *sMarkerText = NULL;

static const u8 sText_MarkerAuto[] = _("MARKER: AUTO");
static const u8 sText_MarkerAlways[] = _("MARKER: ON");
static const u8 sText_MarkerOff[] = _("MARKER: OFF");
static const u8 sText_Exit[] = _("CANCEL");

static void Task_OutfitMenu(u8 taskId);

static u16 PlayerPic(void)
{
    return gSaveBlock2Ptr->playerGender == FEMALE ? TRAINER_PIC_MAY : TRAINER_PIC_BRENDAN;
}

static void BufferMarkerText(void)
{
    const u8 *text = sText_MarkerAuto;

    if (MarkerMode() == COOP_MARKER_ALWAYS)
        text = sText_MarkerAlways;
    else if (MarkerMode() == COOP_MARKER_OFF)
        text = sText_MarkerOff;
    StringCopy(sMarkerText, text);
}

static void RefreshPreviewPic(u8 taskId)
{
    struct SpritePalette pal;
    u8 slot;
    u16 pic = PlayerPic();

    CoopOutfit_RefreshLoaded();
    if (gTasks[taskId].tPicSprite >= MAX_SPRITES)
        return;
    slot = gSprites[gTasks[taskId].tPicSprite].oam.paletteNum;
    pal.data = CoopOutfit_TrainerPalette(pic);
    pal.tag = GetTrainerPicTag(pic, TRUE);
    LoadSpritePaletteInSlot(&pal, slot);
}

static void MoveCursor(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    u8 taskId = FindTaskIdByFunc(Task_OutfitMenu);

    if (!onInit)
        PlaySE(SE_SELECT);
    // On MARKER and CANCEL the preview goes back to the outfit worn now.
    if (itemIndex < COOP_OUTFIT_COUNT)
        sPreviewOutfit = itemIndex;
    else
        sPreviewOutfit = SavedOutfit();
    if (taskId != TASK_NONE)
        RefreshPreviewPic(taskId);
}

static const struct ListMenuTemplate sListTemplate = {
    .items = NULL,
    .moveCursorFunc = MoveCursor,
    .itemPrintFunc = NULL,
    .totalItems = MENU_ITEMS,
    .maxShowed = MENU_ITEMS,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NORMAL,
    .cursorKind = CURSOR_BLACK_ARROW,
};

static void CloseMenu(u8 taskId)
{
    u16 pic = PlayerPic();

    DestroyListMenuTask(gTasks[taskId].tListTaskId, NULL, NULL);
    ClearStdWindowAndFrameToTransparent(gTasks[taskId].tWindowId, TRUE);
    RemoveWindow(gTasks[taskId].tWindowId);
    if (gTasks[taskId].tPicSprite < MAX_SPRITES)
        FreeAndDestroyTrainerPicSprite(gTasks[taskId].tPicSprite);
    FreeSpritePaletteByTag(GetTrainerPicTag(pic, TRUE));
    Free(sMenuItems);
    Free(sMarkerText);
    sMenuItems = NULL;
    sMarkerText = NULL;
    sPreviewActive = FALSE;
    CoopOutfit_RefreshLoaded();
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static void Task_OutfitMenu(u8 taskId)
{
    struct CoopSave *save = Coop_GetSave();
    s32 input = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);

    if (input == LIST_NOTHING_CHOSEN)
        return;
    if (input == LIST_CANCEL || input == ITEM_EXIT)
    {
        PlaySE(SE_SELECT);
        gSpecialVar_Result = FALSE;
        CloseMenu(taskId);
        return;
    }
    PlaySE(SE_SELECT);
    if (input == ITEM_MARKER)
    {
        if (save != NULL)
            save->partnerMarker = (MarkerMode() + 1) % (COOP_MARKER_OFF + 1);
        BufferMarkerText();
        RedrawListMenu(gTasks[taskId].tListTaskId);
        return;
    }
    if (save != NULL)
        save->outfit = input;
    gSpecialVar_Result = (input != gTasks[taskId].tOriginal);
    if (gSpecialVar_Result)
        CoopDeal_QueueLook();
    CloseMenu(taskId);
}

void CoopOutfit_OpenMenu(void)
{
    struct WindowTemplate template;
    struct ListMenuTemplate list;
    u8 taskId, i, windowId;
    u16 pic = PlayerPic();
    u8 base = Base(gSaveBlock2Ptr->playerGender);

    sMenuItems = AllocZeroed(MENU_ITEMS * sizeof(struct ListMenuItem));
    sMarkerText = AllocZeroed(24);
    for (i = 0; i < COOP_OUTFIT_COUNT; i++)
    {
        sMenuItems[i].name = sCoopOutfitNames[base][i];
        sMenuItems[i].id = i;
    }
    BufferMarkerText();
    sMenuItems[ITEM_MARKER].name = sMarkerText;
    sMenuItems[ITEM_MARKER].id = ITEM_MARKER;
    sMenuItems[ITEM_EXIT].name = sText_Exit;
    sMenuItems[ITEM_EXIT].id = ITEM_EXIT;

    taskId = CreateTask(Task_OutfitMenu, 80);
    gTasks[taskId].tOriginal = SavedOutfit();
    sPreviewOutfit = SavedOutfit();
    sPreviewActive = TRUE;

    template = CreateWindowTemplate(0, 1, 1, 12, MENU_ITEMS * 2, 0xF, 0x64);
    windowId = AddWindow(&template);
    gTasks[taskId].tWindowId = windowId;
    SetStandardWindowBorderStyle(windowId, FALSE);
    list = sListTemplate;
    list.items = sMenuItems;
    list.windowId = windowId;
    gTasks[taskId].tListTaskId = ListMenuInit(&list, 0, SavedOutfit());
    CopyWindowToVram(windowId, COPYWIN_FULL);

    gTasks[taskId].tPicSprite = CreateTrainerPicSprite(pic, TRUE, 192, 72, 0, GetTrainerPicTag(pic, TRUE));
    if (gTasks[taskId].tPicSprite == 0xFFFF)
        gTasks[taskId].tPicSprite = MAX_SPRITES;
    else
        gSprites[gTasks[taskId].tPicSprite].oam.priority = 0;
    RefreshPreviewPic(taskId);
}
