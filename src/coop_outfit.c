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
// drawn in CLASSIC. The partner indicator (below) marks the partner in the
// partner's own colour, whatever either player wears.

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
#define TAG_NAMETAG_TILES  0xC0B6

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
EWRAM_DATA bool8 gCoopMarkerShown = FALSE;
EWRAM_DATA bool8 gCoopCompassShown = FALSE;  // the edge compass is on screen (read by tests)
// The partner's name tag: its sprite (checked against the template like the
// marker), and the name its tiles hold.
static EWRAM_DATA u8 sTagSpriteId = 0;
static EWRAM_DATA u8 sTagName[PLAYER_NAME_LENGTH + 1] = {0};
static EWRAM_DATA u8 sTagWidth = 0;          // the drawn name's width in pixels
EWRAM_DATA bool8 gCoopNameTagShown = FALSE;  // the name tag is on screen (read by tests)

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
    CoopOutfit_OnPartnerStyle(0);
    CoopDeal_QueueLook();
}

// Special: the BEACON STYLE changed at the shrine; tell the partner, whose
// screen draws this player's marker in it.
void CoopOutfit_SendLook(void)
{
    CoopDeal_QueueLook();
}

// ------------------------------------------------------- partner indicator
//
// How the partner is told apart on the field (CoopSave.partnerMarker bits
// 4-5, COOP_SHOW_*): INDICATOR (the default) is a small outlined marker over
// the partner's head; NAME is the partner's name in small outlined letters,
// with no background box; OFF shows neither. The marker's colour belongs to
// the partner, not to their outfit: each player's own colour (their BEACON
// STYLE from the Twin Beacon shrine, else one picked from their trainer ID)
// travels with the DMSG_LOOK block, so a player is the same colour on every
// partner's screen whatever they wear. The marker's palette tag is blend
// immune, so night tinting does not dull it; its dark outline keeps it
// readable on grass, water, buildings and cave floors.
//
// Both are drawn only while this player can move: never over a message box,
// a menu, a scene, a warp or a fade, and never for a partner who is not
// drawn here (another map, in a battle, a quiet link). They follow the
// partner's sprite every frame (its own pixel offsets while walking, running,
// cycling or surfing, and the camera offset while the map scrolls or crosses
// a connection), at the partner's layer so a tree top that hides the partner
// hides the marker too. A map load frees every sprite: the template checks
// below notice, and the next field frame draws them again; battles never see
// them (they exist only in the field). A steady arrow at the screen edge
// points to a partner nearby (this map or a connected one) who is out of
// sight.

#include "data/coop_marker_icons.h"

static const struct SpriteSheet sMarkerSheet = {sCoopMarkerIconTiles, sizeof(sCoopMarkerIconTiles), TAG_MARKER_TILES};

// Player colours: highlight, fill, outline. The outline is dark in every
// colour so the marker reads on light and dark ground alike.
#define RGB_(r, g, b) RGB((r) >> 3, (g) >> 3, (b) >> 3)
enum { PCOL_BLUE, PCOL_PINK, PCOL_GOLD, PCOL_GREEN, PCOL_COUNT };
static const u16 sPlayerColours[PCOL_COUNT][3] = {
    [PCOL_BLUE]  = { RGB_(200, 236, 255), RGB_(72, 160, 255),  RGB_(8, 24, 72) },
    [PCOL_PINK]  = { RGB_(255, 214, 230), RGB_(255, 104, 168), RGB_(72, 8, 40) },
    [PCOL_GOLD]  = { RGB_(255, 246, 180), RGB_(255, 200, 40),  RGB_(72, 40, 0) },
    [PCOL_GREEN] = { RGB_(214, 255, 214), RGB_(72, 208, 104),  RGB_(8, 56, 24) },
};
static const u8 sPlayerColourNames[PCOL_COUNT][6] = {
    [PCOL_BLUE] = _("BLUE"), [PCOL_PINK] = _("PINK"), [PCOL_GOLD] = _("GOLD"), [PCOL_GREEN] = _("GREEN"),
};

// The partner's BEACON STYLE + 1 from their look block (0: not heard yet).
static EWRAM_DATA u8 sPartnerStyle = 0;
static EWRAM_DATA u8 sShownColour = 0;      // colour + 1 last loaded into the marker palette

// A player's colour: a style chosen at the shrine (GOLD, AZURE, ROSE), else
// (CLASSIC, or no style yet) one of four picked from the trainer ID.
static u8 ColourFor(u32 otId, u8 style)
{
    switch (style)
    {
    case 1: return PCOL_GOLD;
    case 2: return PCOL_BLUE;
    case 3: return PCOL_PINK;
    }
    return ((otId & 0xFFFF) ^ (otId >> 16)) % PCOL_COUNT;
}

u8 CoopOutfit_LocalStyle(void)
{
    u16 style = VarGet(VAR_BEACON_STYLE);

    if (!FlagGet(FLAG_SHRINE_LORE_READ) || style >= BEACON_STYLE_COUNT)
        return 0;
    return style;
}

static u32 LocalOtId(void)
{
    const u8 *id = gSaveBlock2Ptr->playerTrainerId;

    return id[0] | (id[1] << 8) | (id[2] << 16) | (id[3] << 24);
}

static bool8 PartnerOtId(u32 *otId)
{
    if (!gReceivedRemoteLinkPlayers || GetMultiplayerId() > 1)
        return FALSE;
    *otId = gLinkPlayers[GetMultiplayerId() ^ 1].trainerId;
    return TRUE;
}

static u8 PartnerColour(void)
{
    u32 otId = 0;

    PartnerOtId(&otId);
    return ColourFor(otId, sPartnerStyle ? sPartnerStyle - 1 : 0);
}

void CoopOutfit_OnPartnerStyle(u8 styleCode)
{
    // styleCode: the partner's style + 1 (0 from builds that do not send it).
    sPartnerStyle = styleCode <= BEACON_STYLE_COUNT ? styleCode : 0;
}

static u8 ShowMode(void)
{
    struct CoopSave *save = Coop_GetSave();
    u8 mode;

    if (save == NULL)
        return COOP_SHOW_INDICATOR;
    mode = (save->partnerMarker & COOP_SHOW_MASK) >> COOP_SHOW_SHIFT;
    return mode > COOP_SHOW_OFF ? COOP_SHOW_INDICATOR : mode;
}

static const struct OamData sMarkerOam = {
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
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

// The marker palette (shared by the indicator, the compass and the name):
// loaded once per colour; a slot already holding it is reused.
static u8 MarkerPaletteSlot(void)
{
    u16 colours[16] = {0};
    struct SpritePalette pal;
    u8 colour = PartnerColour(), slot;

    colours[12] = RGB_(248, 248, 248);
    colours[13] = sPlayerColours[colour][0];
    colours[14] = sPlayerColours[colour][1];
    colours[15] = sPlayerColours[colour][2];
    pal.data = colours;
    pal.tag = TAG_MARKER_PAL;
    slot = IndexOfSpritePaletteTag(TAG_MARKER_PAL);
    if (slot == 0xFF)
        slot = LoadSpritePalette(&pal);
    else if (sShownColour != colour + 1)
        LoadSpritePaletteInSlot(&pal, slot);
    if (slot != 0xFF)
        sShownColour = colour + 1;
    return slot;
}

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

static struct Sprite *EnsureMarker(void)
{
    u8 slot = MarkerPaletteSlot();

    if (slot == 0xFF)
        return NULL;
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

// The partner's name in small letters with a one-pixel outline and no
// background: one 64x32 sprite (the text uses the top rows), the marker's
// palette (14 letters, 15 outline).

#define TAG_W 64
#define TAG_TILES ((TAG_W / 8) * 4)
static const u32 sNameTagBlank[TAG_TILES * 8] = {0};
static const struct SpriteSheet sNameTagSheet = {sNameTagBlank, sizeof(sNameTagBlank), TAG_NAMETAG_TILES};

static const struct OamData sNameTagOam = {
    .shape = SPRITE_SHAPE(64x32),
    .size = SPRITE_SIZE(64x32),
    .priority = 1,
};

static const struct SpriteTemplate sNameTagTemplate = {
    .tileTag = TAG_NAMETAG_TILES,
    .paletteTag = TAG_MARKER_PAL,
    .oam = &sNameTagOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static bool8 TagAlive(void)
{
    return sTagSpriteId < MAX_SPRITES && gSprites[sTagSpriteId].inUse
        && gSprites[sTagSpriteId].template == &sNameTagTemplate
        && GetSpriteTileStartByTag(TAG_NAMETAG_TILES) != 0xFFFF;
}

static void RemoveTag(void)
{
    if (sTagSpriteId < MAX_SPRITES && gSprites[sTagSpriteId].inUse
     && gSprites[sTagSpriteId].template == &sNameTagTemplate)
        DestroySprite(&gSprites[sTagSpriteId]);
    sTagSpriteId = MAX_SPRITES;
    if (GetSpriteTileStartByTag(TAG_NAMETAG_TILES) != 0xFFFF)
        FreeSpriteTilesByTag(TAG_NAMETAG_TILES);
    sTagName[0] = EOS;
    gCoopNameTagShown = FALSE;
}

// One 4bpp pixel of the tag's tiles (64x32, 1D mapping).
static u8 *TagPixelByte(u8 *tiles, u32 x, u32 y)
{
    return tiles + ((y / 8) * (TAG_W / 8) + x / 8) * TILE_SIZE_4BPP + (y % 8) * 4 + (x % 8) / 2;
}

static u8 TagPixelGet(u8 *tiles, u32 x, u32 y)
{
    u8 b = *TagPixelByte(tiles, x, y);

    return (x & 1) ? (b >> 4) : (b & 0x0F);
}

static void TagPixel(u8 *tiles, u32 x, u32 y, u8 color)
{
    u8 *b = TagPixelByte(tiles, x, y);

    if (x & 1)
        *b = (*b & 0x0F) | (color << 4);
    else
        *b = (*b & 0xF0) | color;
}

#define TAG_TEXT_ROWS 14

static void DrawTag(const u8 *name)
{
    static const union TextColor sColor = { .background = 0, .foreground = 14, .shadow = 0, .accent = 0 };
    u8 *tiles = (u8 *)(OBJ_VRAM0) + GetSpriteTileStartByTag(TAG_NAMETAG_TILES) * TILE_SIZE_4BPP;
    s32 width = GetStringWidth(FONT_SMALL, name, 0);
    u32 x, y;

    if (width > TAG_W - 4)
        width = TAG_W - 4;
    sTagWidth = width;
    CpuFill32(0, tiles, TAG_TILES * TILE_SIZE_4BPP);
    AddSpriteTextPrinterParameterized6(sTagSpriteId, FONT_SMALL, (TAG_W - width) / 2, 1, 0, 0, sColor, TEXT_SKIP_DRAW, name);
    // The outline: every clear pixel next to a letter pixel.
    for (y = 0; y < TAG_TEXT_ROWS; y++)
    {
        for (x = 0; x < TAG_W; x++)
        {
            if (TagPixelGet(tiles, x, y) != 0)
                continue;
            if ((x > 0 && TagPixelGet(tiles, x - 1, y) == 14)
             || (x + 1 < TAG_W && TagPixelGet(tiles, x + 1, y) == 14)
             || (y > 0 && TagPixelGet(tiles, x, y - 1) == 14)
             || (y + 1 < TAG_TEXT_ROWS && TagPixelGet(tiles, x, y + 1) == 14))
                TagPixel(tiles, x, y, 15);
        }
    }
}

static bool8 PartnerName(u8 *dest)
{
    if (!gReceivedRemoteLinkPlayers || GetMultiplayerId() > 1)
        return FALSE;
    StringCopyN(dest, gLinkPlayers[GetMultiplayerId() ^ 1].name, PLAYER_NAME_LENGTH);
    dest[PLAYER_NAME_LENGTH] = EOS;
    return dest[0] != EOS;
}

static bool8 DrawName(struct Sprite *partner)
{
    u8 name[PLAYER_NAME_LENGTH + 1];
    u8 slot;
    struct Sprite *tag;

    if (!PartnerName(name))
        return FALSE;
    slot = MarkerPaletteSlot();
    if (slot == 0xFF)
        return FALSE;
    if (!TagAlive())
    {
        RemoveTag();
        LoadSpriteSheet(&sNameTagSheet);
        if (GetSpriteTileStartByTag(TAG_NAMETAG_TILES) == 0xFFFF)
            return FALSE;
        sTagSpriteId = CreateSprite(&sNameTagTemplate, 0, 0, 0);
        if (sTagSpriteId >= MAX_SPRITES)
        {
            FreeSpriteTilesByTag(TAG_NAMETAG_TILES);
            return FALSE;
        }
        gSprites[sTagSpriteId].data[1] = SPRITE_NONE;
        gSprites[sTagSpriteId].data[2] = SPRITE_NONE;
    }
    tag = &gSprites[sTagSpriteId];
    tag->oam.paletteNum = slot;
    if (StringCompare(name, sTagName) != 0)
    {
        DrawTag(name);
        StringCopy(sTagName, name);
    }
    // Map coordinates, like the partner's own sprite: the letters' bottom
    // row just over the head.
    tag->coordOffsetEnabled = partner->coordOffsetEnabled;
    tag->x = partner->x + partner->x2;
    tag->y = partner->y + partner->y2 - 4;
    tag->oam.priority = partner->oam.priority;
    tag->subpriority = 0;
    tag->invisible = FALSE;
    gCoopNameTagShown = TRUE;
    return TRUE;
}

EWRAM_DATA u8 gCoopMarkerIcon = 0;        // the icon on screen (COOP_MARK_*, arrows +16), read by tests

static u16 IconTile(u8 icon)
{
    return GetSpriteTileStartByTag(TAG_MARKER_TILES) + icon * 4;
}

// A received PING makes the marker hop for a moment (src/coop.c).
static s16 PingHop(void)
{
    return gCoopPingCue != 0 && ((gCoopPingCue >> 3) & 1) ? -2 : 0;
}

static EWRAM_DATA u8 sMarkerAnchor = 0;      // (0xFF: none) the sprite the marker is drawn over this frame

static bool8 DrawIndicator(struct Sprite *partner, u8 icon, s16 dx)
{
    struct Sprite *marker = EnsureMarker();

    if (marker == NULL)
        return FALSE;
    sMarkerAnchor = partner - gSprites;
    marker->oam.tileNum = IconTile(icon);
    marker->coordOffsetEnabled = partner->coordOffsetEnabled;
    // The icon's drawn rows end just above the head: an object event's
    // sprite y sits 6 pixels below the top of its head, and the 16x16
    // icons rest on the square's second-to-last row.
    marker->x = partner->x + partner->x2 + dx;
    marker->y = partner->y + partner->y2 - 15 + PingHop();
    marker->oam.priority = partner->oam.priority;
    marker->subpriority = 0;
    marker->invisible = FALSE;
    gCoopMarkerShown = TRUE;
    gCoopMarkerIcon = icon;
    return TRUE;
}

// The broken-link icon over the tile the partner was last drawn on, while
// the link recovers (no sprite is drawn there meanwhile).
static bool8 DrawBrokenAt(s16 mapX, s16 mapY)
{
    struct Sprite *marker;
    s16 x = mapX, y = mapY;

    // Placed like an object sprite (32 tall: its y is the tile's top); the
    // camera offset is applied by the sprite system, so test it here too.
    SetSpritePosToOffsetMapCoords(&x, &y, 8, 0);
    if (x + gSpriteCoordOffsetX < 8 || x + gSpriteCoordOffsetX > DISPLAY_WIDTH - 8
     || y + gSpriteCoordOffsetY < 16 || y + gSpriteCoordOffsetY > DISPLAY_HEIGHT)
        return FALSE;
    marker = EnsureMarker();
    if (marker == NULL)
        return FALSE;
    marker->oam.tileNum = IconTile(COOP_MARK_BROKEN);
    marker->coordOffsetEnabled = TRUE;
    marker->x = x;
    marker->y = y - 15;
    marker->oam.priority = 1;
    marker->subpriority = 0;
    marker->invisible = FALSE;
    gCoopMarkerShown = TRUE;
    gCoopMarkerIcon = COOP_MARK_BROKEN;
    return TRUE;
}

// Recovering with no known spot on screen: the same icon, steady, in the
// top-right corner (the field's own windows never use that spot).
static void DrawBrokenCorner(void)
{
    struct Sprite *marker = EnsureMarker();

    if (marker == NULL)
        return;
    marker->oam.tileNum = IconTile(COOP_MARK_BROKEN);
    marker->coordOffsetEnabled = FALSE;
    marker->x = DISPLAY_WIDTH - 12;
    marker->y = 12;
    marker->oam.priority = 1;
    marker->subpriority = 0;
    marker->invisible = FALSE;
    gCoopMarkerShown = TRUE;
    gCoopMarkerIcon = COOP_MARK_BROKEN;
}

// The edge arrow: points from the middle of the screen toward a partner
// who is off it, on this map or a connected one. Steady (no blinking).
static void DrawCompass(s16 dx, s16 dy)
{
    static const u8 sArrowFor[3][3] = {
        // [dy + 1][dx + 1] for the signs of dx and dy
        { COOP_ICON_ARROW_NW, COOP_ICON_ARROW_N, COOP_ICON_ARROW_NE },
        { COOP_ICON_ARROW_W,  COOP_ICON_ARROW_E, COOP_ICON_ARROW_E  },
        { COOP_ICON_ARROW_SW, COOP_ICON_ARROW_S, COOP_ICON_ARROW_SE },
    };
    struct Sprite *marker = EnsureMarker();
    s16 adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    s8 sx = dx < 0 ? -1 : dx > 0 ? 1 : 0, sy = dy < 0 ? -1 : dy > 0 ? 1 : 0;
    u8 icon;

    if (marker == NULL)
        return;
    // Mostly along one axis: a straight arrow.
    if (adx > 2 * ady)
        sy = 0;
    else if (ady > 2 * adx)
        sx = 0;
    icon = sArrowFor[sy + 1][sx + 1];
    marker->oam.tileNum = IconTile(icon);
    marker->coordOffsetEnabled = FALSE;
    marker->oam.priority = 1;
    marker->x = 120 + dx * 16;
    marker->y = 80 + dy * 16;
    if (marker->x < 10) marker->x = 10;
    if (marker->x > DISPLAY_WIDTH - 10) marker->x = DISPLAY_WIDTH - 10;
    if (marker->y < 10) marker->y = 10;
    if (marker->y > DISPLAY_HEIGHT - 10) marker->y = DISPLAY_HEIGHT - 10;
    marker->y += PingHop();
    marker->invisible = FALSE;
    gCoopCompassShown = TRUE;
    gCoopMarkerIcon = 16 + (icon - COOP_ICON_ARROW_E);
}

// Special: PARTNER MARK in the CO-OP menu (and the dresser's line):
// INDICATOR, NAME, OFF in turn; the new setting's line in gStringVar4.
u16 CoopOutfit_CycleNameTag(void)
{
    struct CoopSave *save = Coop_GetSave();
    u8 mode = (ShowMode() + 1) % (COOP_SHOW_OFF + 1);
    u8 *str = gStringVar4;

    if (save != NULL)
        save->partnerMarker = (save->partnerMarker & ~COOP_SHOW_MASK) | (mode << COOP_SHOW_SHIFT);
    if (mode == COOP_SHOW_INDICATOR)
    {
        str = StringCopy(str, COMPOUND_STRING("Partner mark: INDICATOR. Your\npartner has a "));
        str = StringCopy(str, sPlayerColourNames[PartnerColour()]);
        str = StringCopy(str, COMPOUND_STRING(" marker; you\lshow as "));
        str = StringCopy(str, sPlayerColourNames[ColourFor(LocalOtId(), CoopOutfit_LocalStyle())]);
        StringCopy(str, COMPOUND_STRING(" on their screen."));
    }
    else if (mode == COOP_SHOW_NAME)
    {
        StringCopy(str, COMPOUND_STRING("Partner mark: NAME. Your partner's\nname shows over their head."));
    }
    else
    {
        StringCopy(str, COMPOUND_STRING("Partner mark: OFF."));
    }
    return mode;
}

static void HideAll(void)
{
    HideMarker();
    if (TagAlive())
        gSprites[sTagSpriteId].invisible = TRUE;
    gCoopNameTagShown = FALSE;
}

// How far off screen the edge arrow still points (tiles); a PING shows it
// at any distance for a moment.
#define ARROW_RANGE 32

// The partner's activity changes for a frame or two at every warp fade and
// script start; an icon follows it only once it has held for a moment, so
// the marker never flickers between icons. Battle and the broken link show
// at once (they last).
#define KIND_SETTLE_FRAMES 10
static EWRAM_DATA u8 sStableKind = 0;
static EWRAM_DATA u8 sPendingKind = 0;
static EWRAM_DATA u8 sPendingFrames = 0;

static u8 StableKind(u8 kind)
{
    if (kind == sStableKind)
    {
        sPendingFrames = 0;
        return kind;
    }
    if (kind == COOP_MARK_NONE || kind == COOP_MARK_BROKEN || kind == COOP_MARK_BATTLE)
    {
        sStableKind = kind;
        sPendingFrames = 0;
        return kind;
    }
    if (kind != sPendingKind)
    {
        sPendingKind = kind;
        sPendingFrames = 0;
    }
    if (++sPendingFrames >= KIND_SETTLE_FRAMES)
    {
        sStableKind = kind;
        sPendingFrames = 0;
    }
    // From nothing, show the new kind at once.
    if (sStableKind == COOP_MARK_NONE)
        return kind;
    return sStableKind;
}

void CoopOutfit_FieldFrame(void)
{
    u8 gender, mode = ShowMode();
    u8 partnerSprite, kind;
    u8 shown;
    struct Sprite *partner;
    s16 px, py, dx, dy;
    bool8 busy, havePos;

    partnerSprite = Coop_PartnerSpriteId(&gender);
    if (partnerSprite < MAX_SPRITES && gender != sPartnerGender)
        sPartnerGender = gender;
    shown = (sPartnerGender << 4) | (sPartnerKnown ? sPartnerOutfit : 0);
    if (sShownLocal != CoopOutfit_Local() || sShownPartner != shown)
        CoopOutfit_RefreshLoaded();

    gCoopMarkerShown = FALSE;
    gCoopCompassShown = FALSE;
    gCoopNameTagShown = FALSE;
    gCoopMarkerIcon = 0xFF;
    sMarkerAnchor = 0xFF;
    // This player busy: a message, a menu, a scene, a warp or a fade.
    // Waiting on the partner's answer or readiness is not busy: the marker
    // (their '?' or tick) is what this player is watching.
    busy = (ArePlayerFieldControlsLocked() || ScriptContext_IsEnabled() || gPaletteFade.active)
        && !(Coop_LocalAwaitingPartner() && !gPaletteFade.active);
    kind = StableKind(Coop_PartnerMarkKind());
    if (mode == COOP_SHOW_OFF || kind == COOP_MARK_NONE || busy)
    {
        HideMarker();
        RemoveTag();
        return;
    }
    if (kind == COOP_MARK_BROKEN)
    {
        // Recovering: nothing is known about the partner but where they
        // were; mark that spot if it is on screen.
        RemoveTag();
        if (!Coop_PartnerLastSeen(&px, &py) || !DrawBrokenAt(px, py))
            DrawBrokenCorner();
        return;
    }
    havePos = Coop_PartnerViewCoords(&px, &py);
    if (!havePos)
    {
        HideMarker();
        RemoveTag();
        return;
    }
    dx = px - gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.x;
    dy = py - gObjectEvents[gPlayerAvatar.objectEventId].currentCoords.y;
    if (partnerSprite >= MAX_SPRITES || dx < -7 || dx > 7 || dy < -5 || dy > 4)
    {
        // Out of sight: the edge arrow, while the partner is near (or has
        // just pinged).
        if (TagAlive())
            gSprites[sTagSpriteId].invisible = TRUE;
        if ((dx < -ARROW_RANGE || dx > ARROW_RANGE || dy < -ARROW_RANGE || dy > ARROW_RANGE) && gCoopPingCue == 0)
            HideMarker();
        else
            DrawCompass(dx, dy);
        return;
    }
    partner = &gSprites[partnerSprite];
    if (partner->invisible)
    {
        HideAll();
        return;
    }
    if (mode == COOP_SHOW_NAME)
    {
        if (!DrawName(partner))
        {
            RemoveTag();
            HideMarker();
        }
        // The name, and what they are doing to its left (walking needs no
        // icon beside a name).
        else if (kind == COOP_MARK_WALK || !DrawIndicator(partner, kind, -(sTagWidth / 2) - 8))
            HideMarker();
        else
            gSprites[sMarkerSpriteId].y += 1;
    }
    else
    {
        if (TagAlive())
            RemoveTag();
        if (!DrawIndicator(partner, kind, 0))
            HideMarker();
    }
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

static const u8 sText_MarkerIndicator[] = _("MARK: INDICATOR");
static const u8 sText_MarkerName[] = _("MARK: NAME");
static const u8 sText_MarkerOff[] = _("MARK: OFF");
static const u8 sText_Exit[] = _("CANCEL");

static void Task_OutfitMenu(u8 taskId);

static u16 PlayerPic(void)
{
    return gSaveBlock2Ptr->playerGender == FEMALE ? TRAINER_PIC_MAY : TRAINER_PIC_BRENDAN;
}

static void BufferMarkerText(void)
{
    const u8 *text = sText_MarkerIndicator;

    if (ShowMode() == COOP_SHOW_NAME)
        text = sText_MarkerName;
    else if (ShowMode() == COOP_SHOW_OFF)
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
        CoopOutfit_CycleNameTag();
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
    u16 picSprite;
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
    // No preview sprite yet: ListMenuInit below moves the cursor (and so
    // refreshes the preview) before it exists. Task data starts at 0, which
    // is the player's own field sprite, and its palette slot would get the
    // trainer pic's colours and tag.
    gTasks[taskId].tPicSprite = MAX_SPRITES;

    template = CreateWindowTemplate(0, 1, 1, 12, MENU_ITEMS * 2, 0xF, 0x64);
    windowId = AddWindow(&template);
    gTasks[taskId].tWindowId = windowId;
    SetStandardWindowBorderStyle(windowId, FALSE);
    list = sListTemplate;
    list.items = sMenuItems;
    list.windowId = windowId;
    gTasks[taskId].tListTaskId = ListMenuInit(&list, 0, SavedOutfit());
    CopyWindowToVram(windowId, COPYWIN_FULL);

    picSprite = CreateTrainerPicSprite(pic, TRUE, 192, 72, 0, GetTrainerPicTag(pic, TRUE));
    if (picSprite >= MAX_SPRITES)   // 0xFFFF: no sprite (task data is s16)
        picSprite = MAX_SPRITES;
    else
        gSprites[picSprite].oam.priority = 0;
    gTasks[taskId].tPicSprite = picSprite;
    RefreshPreviewPic(taskId);
}

// Self-checks for src/coop.c (Coop_CheckAppearance): the loaded partner
// palette is the partner's current look, and a marker over a partner is
// over this sprite (not one from before a respawn).
bool8 CoopOutfit_PartnerLookCurrent(void)
{
    return sShownPartner == ((sPartnerGender << 4) | (sPartnerKnown ? sPartnerOutfit : 0));
}

bool8 CoopOutfit_MarkerOnSprite(u8 spriteId)
{
    if (!gCoopMarkerShown || sMarkerAnchor == 0xFF)
        return TRUE;
    return sMarkerAnchor == spriteId;
}
