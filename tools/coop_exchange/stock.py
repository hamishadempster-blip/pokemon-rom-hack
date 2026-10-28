#!/usr/bin/env python3
"""The Rustboro Trainer Exchange's stock, prices and unlocks (one place).

    python3 tools/coop_exchange/stock.py            # write the C data + doc table
    python3 tools/coop_exchange/stock.py --check    # validation only

Writes src/data/coop_exchange_stock.h (the counters' stock, the tutor moves,
the price table every shop uses) and docs/COOP_EXCHANGE_STOCK.md. Checks:
every constant exists; every evolution item the roster uses is stocked;
prices sit inside each tier's band; no item is sold below a quarter of its
sell-back value (no buy/sell loop); tutor moves are not signature moves.

Tiers are the BUYING player's own progress: 0 no badge, 1 one or two, 2
three or four, 3 five or six, 4 seven or eight, 5 Champion.
"""
import json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TIER_NAMES = ['Start', '1 badge', '3 badges', '5 badges', '7 badges', 'Champion']

# Counters
TM, HELD, EVO, TRAIN, BREED = range(5)
COUNTER_NAMES = ['Moves & TMs', 'Battle items', 'Evolution', 'Training & Nature', 'Breeding']

# (item, tier, price or None to keep the item's own price, quest requirement or None)
STOCK = {
TM: [
    ('TM_WATER_PULSE', 0, 2500), ('TM_BULLET_SEED', 0, 2500), ('TM_PROTECT', 0, 2000), ('TM_SECRET_POWER', 0, 2000),
    ('TM_ROAR', 0, 2000), ('TM_TAUNT', 0, 2500),
    ('TM_ROCK_TOMB', 1, 3000), ('TM_AERIAL_ACE', 1, 3000), ('TM_SHOCK_WAVE', 1, 3000), ('TM_FACADE', 1, 3000),
    ('TM_REST', 1, 3000), ('TM_ATTRACT', 1, 3000), ('TM_TORMENT', 1, 3000), ('TM_THIEF', 1, 3000),
    ('TM_STEEL_WING', 1, 3000), ('TM_DIG', 1, 3000), ('TM_HIDDEN_POWER', 1, 3000), ('TM_RETURN', 1, 3000),
    ('TM_FRUSTRATION', 1, 3000), ('TM_SUNNY_DAY', 1, 3000), ('TM_RAIN_DANCE', 1, 3000), ('TM_SANDSTORM', 1, 3000),
    ('TM_HAIL', 1, 3000), ('TM_SAFEGUARD', 1, 3000),
    ('TM_BRICK_BREAK', 2, 5000), ('TM_BULK_UP', 2, 6000), ('TM_CALM_MIND', 2, 6000), ('TM_LIGHT_SCREEN', 2, 5000),
    ('TM_REFLECT', 2, 5000), ('TM_GIGA_DRAIN', 2, 5000), ('TM_IRON_TAIL', 2, 5000), ('TM_DRAGON_CLAW', 2, 6000),
    ('TM_SLUDGE_BOMB', 2, 6000), ('TM_TOXIC', 2, 5000), ('TM_SKILL_SWAP', 2, 5000), ('TM_SNATCH', 2, 5000),
    ('TM_FOCUS_PUNCH', 2, 5000), ('TM_DOUBLE_TEAM', 2, 5000),
    ('TM_FLAMETHROWER', 3, 9000), ('TM_THUNDERBOLT', 3, 9000), ('TM_ICE_BEAM', 3, 9000), ('TM_PSYCHIC', 3, 9000),
    ('TM_SHADOW_BALL', 3, 9000), ('TM_OVERHEAT', 3, 9000), ('TM_SOLAR_BEAM', 3, 8000),
    ('TM_EARTHQUAKE', 4, 12000), ('TM_FIRE_BLAST', 4, 10000), ('TM_THUNDER', 4, 10000), ('TM_BLIZZARD', 4, 10000),
    ('TM_HYPER_BEAM', 4, 10000),
],
HELD: [
    ('ORAN_BERRY', 0, 200), ('CHERI_BERRY', 0, 200), ('CHESTO_BERRY', 0, 200), ('PECHA_BERRY', 0, 200),
    ('RAWST_BERRY', 0, 200), ('ASPEAR_BERRY', 0, 200), ('PERSIM_BERRY', 0, 200), ('LEPPA_BERRY', 0, 300),
    ('SITRUS_BERRY', 1, 600), ('LUM_BERRY', 1, 800), ('QUICK_CLAW', 1, 4000), ('SMOKE_BALL', 1, 2000),
    ('CHARCOAL', 1, 3000), ('MYSTIC_WATER', 1, 3000), ('MIRACLE_SEED', 1, 3000), ('MAGNET', 1, 3000),
    ('NEVER_MELT_ICE', 1, 3000), ('BLACK_BELT', 1, 3000), ('POISON_BARB', 1, 3000), ('SOFT_SAND', 1, 3000),
    ('SHARP_BEAK', 1, 3000), ('TWISTED_SPOON', 1, 3000), ('SILVER_POWDER', 1, 3000), ('HARD_STONE', 1, 3000),
    ('SPELL_TAG', 1, 3000), ('DRAGON_FANG', 1, 3000), ('BLACK_GLASSES', 1, 3000), ('SILK_SCARF', 1, 3000),
    ('FAIRY_FEATHER', 1, 3000), ('METAL_COAT', 1, 3000),
    # Incense: battle and travel effects (babies hatch without it here)
    ('SEA_INCENSE', 1, 2000), ('ROCK_INCENSE', 1, 2000), ('ODD_INCENSE', 1, 2000), ('WAVE_INCENSE', 1, 2000),
    ('ROSE_INCENSE', 1, 2000), ('LAX_INCENSE', 1, 3000), ('FULL_INCENSE', 1, 3000), ('LUCK_INCENSE', 1, 3000),
    ('PURE_INCENSE', 1, 2000),
    ('LEFTOVERS', 2, 12000), ('BLACK_SLUDGE', 2, 8000), ('MUSCLE_BAND', 2, 6000), ('WISE_GLASSES', 2, 6000),
    ('SCOPE_LENS', 2, 8000), ('EXPERT_BELT', 2, 10000), ('LIGHT_CLAY', 2, 8000), ('MENTAL_HERB', 2, 5000),
    ('WHITE_HERB', 2, 5000), ('AIR_BALLOON', 2, 6000), ('ROCKY_HELMET', 2, 10000), ('SHELL_BELL', 2, 8000),
    ('BRIGHT_POWDER', 2, 8000), ('BIG_ROOT', 2, 5000), ('TOXIC_ORB', 2, 6000), ('FLAME_ORB', 2, 6000),
    ('SHED_SHELL', 2, 6000), ('RED_CARD', 2, 4000), ('HEAT_ROCK', 2, 5000), ('DAMP_ROCK', 2, 5000),
    ('SMOOTH_ROCK', 2, 5000), ('ICY_ROCK', 2, 5000), ('KINGS_ROCK', 2, 5000), ('RAZOR_CLAW', 3, 5000), ('ABSORB_BULB', 2, 3000), ('CELL_BATTERY', 2, 3000),
    ('SNOWBALL', 2, 3000), ('LUMINOUS_MOSS', 2, 3000),
    ('OCCA_BERRY', 2, 1000), ('PASSHO_BERRY', 2, 1000), ('WACAN_BERRY', 2, 1000), ('RINDO_BERRY', 2, 1000),
    ('YACHE_BERRY', 2, 1000), ('CHOPLE_BERRY', 2, 1000), ('KEBIA_BERRY', 2, 1000), ('SHUCA_BERRY', 2, 1000),
    ('COBA_BERRY', 2, 1000), ('PAYAPA_BERRY', 2, 1000), ('TANGA_BERRY', 2, 1000), ('CHARTI_BERRY', 2, 1000),
    ('KASIB_BERRY', 2, 1000), ('HABAN_BERRY', 2, 1000), ('COLBUR_BERRY', 2, 1000), ('BABIRI_BERRY', 2, 1000),
    ('CHILAN_BERRY', 2, 1000), ('ROSELI_BERRY', 2, 1000),
    ('CHOICE_BAND', 3, 20000), ('CHOICE_SPECS', 3, 20000), ('CHOICE_SCARF', 3, 20000), ('LIFE_ORB', 3, 20000),
    ('FOCUS_SASH', 3, 15000), ('ASSAULT_VEST', 3, 15000), ('EVIOLITE', 3, 15000), ('WEAKNESS_POLICY', 3, 15000),
    ('POWER_HERB', 3, 10000), ('HEAVY_DUTY_BOOTS', 3, 12000), ('SAFETY_GOGGLES', 3, 10000),
    ('PROTECTIVE_PADS', 3, 10000), ('COVERT_CLOAK', 3, 12000), ('CLEAR_AMULET', 3, 12000),
    ('THROAT_SPRAY', 3, 8000), ('WIDE_LENS', 3, 8000), ('ZOOM_LENS', 3, 8000), ('METRONOME', 3, 8000),
    ('LOADED_DICE', 3, 12000), ('PUNCHING_GLOVE', 3, 10000), ('ABILITY_SHIELD', 3, 12000),
    ('UTILITY_UMBRELLA', 3, 10000), ('TERRAIN_EXTENDER', 3, 10000), ('EJECT_BUTTON', 3, 10000),
    ('EJECT_PACK', 3, 10000), ('BLUNDER_POLICY', 3, 8000), ('ROOM_SERVICE', 3, 8000),
    ('ELECTRIC_SEED', 3, 6000), ('PSYCHIC_SEED', 3, 6000), ('MISTY_SEED', 3, 6000), ('GRASSY_SEED', 3, 6000),
    ('LIECHI_BERRY', 4, 3000), ('GANLON_BERRY', 4, 3000), ('SALAC_BERRY', 4, 3000), ('PETAYA_BERRY', 4, 3000),
    ('APICOT_BERRY', 4, 3000), ('KEE_BERRY', 4, 3000), ('MARANGA_BERRY', 4, 3000),
    ('CUSTAP_BERRY', 5, 5000), ('LANSAT_BERRY', 5, 5000), ('STARF_BERRY', 5, 5000),
],
EVO: [
    ('MOON_STONE', 1, 3000), ('LEAF_STONE', 1, 3000), ('SUN_STONE', 1, 3000), ('STRAWBERRY_SWEET', 1, 3000),
    ('LOVE_SWEET', 1, 3000), ('BERRY_SWEET', 1, 3000), ('CLOVER_SWEET', 1, 3000), ('FLOWER_SWEET', 1, 3000),
    ('STAR_SWEET', 1, 3000), ('RIBBON_SWEET', 1, 3000), ('OVAL_STONE', 1, 3000), ('SWEET_APPLE', 1, 3000),
    ('TART_APPLE', 1, 3000),
    ('METAL_COAT', 1, 3000),
    ('THUNDER_STONE', 2, 3000), ('FIRE_STONE', 2, 3000), ('WATER_STONE', 2, 3000), ('KINGS_ROCK', 2, 5000),
    ('SYRUPY_APPLE', 2, 3000), ('CRACKED_POT', 2, 3000), ('CHIPPED_POT', 2, 3000),
    ('UNREMARKABLE_TEACUP', 2, 3000), ('MASTERPIECE_TEACUP', 2, 3000), ('GALARICA_CUFF', 2, 3000),
    ('GALARICA_WREATH', 2, 3000), ('PRISM_SCALE', 2, 3000), ('DEEP_SEA_TOOTH', 2, 3000),
    ('DEEP_SEA_SCALE', 2, 3000), ('LEEK', 2, 2000), ('SACHET', 2, 3000), ('WHIPPED_DREAM', 2, 3000),
    ('RED_NECTAR', 2, 1000), ('YELLOW_NECTAR', 2, 1000), ('PINK_NECTAR', 2, 1000), ('PURPLE_NECTAR', 2, 1000),
    ('DUSK_STONE', 3, 4000), ('SHINY_STONE', 3, 4000), ('DAWN_STONE', 3, 4000), ('ICE_STONE', 3, 4000),
    ('LINKING_CORD', 3, 6000), ('DRAGON_SCALE', 3, 5000), ('UPGRADE', 3, 5000), ('REAPER_CLOTH', 3, 5000),
    ('PROTECTOR', 3, 5000), ('ELECTIRIZER', 3, 5000), ('MAGMARIZER', 3, 5000), ('DUBIOUS_DISC', 3, 6000),
    ('RAZOR_CLAW', 3, 5000), ('RAZOR_FANG', 3, 5000), ('BLACK_AUGURITE', 3, 5000), ('PEAT_BLOCK', 3, 5000),
    ('AUSPICIOUS_ARMOR', 3, 5000), ('MALICIOUS_ARMOR', 3, 5000), ('METAL_ALLOY', 3, 6000),
],
TRAIN: [
    ('EXP_CANDY_XS', 0, 200), ('HEALTH_FEATHER', 0, 300), ('MUSCLE_FEATHER', 0, 300),
    ('RESIST_FEATHER', 0, 300), ('GENIUS_FEATHER', 0, 300), ('CLEVER_FEATHER', 0, 300), ('SWIFT_FEATHER', 0, 300),
    ('EXP_CANDY_S', 1, 800), ('HP_UP', 1, 3000), ('PROTEIN', 1, 3000), ('IRON', 1, 3000), ('CALCIUM', 1, 3000),
    ('ZINC', 1, 3000), ('CARBOS', 1, 3000), ('POMEG_BERRY', 1, 500), ('KELPSY_BERRY', 1, 500),
    ('QUALOT_BERRY', 1, 500), ('HONDEW_BERRY', 1, 500), ('GREPA_BERRY', 1, 500), ('TAMATO_BERRY', 1, 500),
    ('MACHO_BRACE', 1, 3000),
    ('EXP_CANDY_M', 2, 2500), ('PP_UP', 2, 8000),
    ('EXP_CANDY_L', 3, 6000),
    ('LONELY_MINT', 3, 8000), ('ADAMANT_MINT', 3, 8000), ('NAUGHTY_MINT', 3, 8000), ('BRAVE_MINT', 3, 8000),
    ('BOLD_MINT', 3, 8000), ('IMPISH_MINT', 3, 8000), ('LAX_MINT', 3, 8000), ('RELAXED_MINT', 3, 8000),
    ('MODEST_MINT', 3, 8000), ('MILD_MINT', 3, 8000), ('RASH_MINT', 3, 8000), ('QUIET_MINT', 3, 8000),
    ('CALM_MINT', 3, 8000), ('GENTLE_MINT', 3, 8000), ('CAREFUL_MINT', 3, 8000), ('SASSY_MINT', 3, 8000),
    ('TIMID_MINT', 3, 8000), ('HASTY_MINT', 3, 8000), ('JOLLY_MINT', 3, 8000), ('NAIVE_MINT', 3, 8000),
    ('SERIOUS_MINT', 3, 8000),
    ('EXP_CANDY_XL', 4, 15000), ('PP_MAX', 4, 20000),
    ('ABILITY_CAPSULE', 4, 30000, 'TWIN_TRIAL'),
    ('RARE_CANDY', 5, 10000), ('ABILITY_PATCH', 5, 50000),
],
BREED: [
    ('EVERSTONE', 0, 2000),
    ('POWER_WEIGHT', 2, 7000), ('POWER_BRACER', 2, 7000), ('POWER_BELT', 2, 7000), ('POWER_LENS', 2, 7000),
    ('POWER_BAND', 2, 7000), ('POWER_ANKLET', 2, 7000),
    ('DESTINY_KNOT', 3, 15000), ('MIRROR_HERB', 3, 10000),
],
}

# Quest requirements (the buyer's own save): name -> C condition.
QUESTS = {
    'TWIN_TRIAL': ('CoopQuest_IsRewardClaimed(COOP_REWARD_TOURNEY)', 'after the Twin Trial at the Beacon shrine'),
}

# Tutor moves: (move, tier, price). Types come from the move data.
TUTOR = [
    # Start: modest reliable attacks and utility
    ('MUD_SLAP', 0, 1500), ('BUG_BITE', 0, 1500), ('STRUGGLE_BUG', 0, 1500), ('HELPING_HAND', 0, 1500),
    ('THUNDER_WAVE', 0, 2000), ('FLAME_CHARGE', 0, 2000), ('LOW_KICK', 0, 2000), ('ICY_WIND', 0, 2000),
    ('DRAINING_KISS', 0, 2000),
    # 1-2 badges: basic coverage
    ('BODY_SLAM', 1, 3000), ('SWIFT', 1, 2500), ('BULLDOZE', 1, 3000), ('ROCK_SLIDE', 1, 3500), ('ELECTROWEB', 1, 3000),
    ('SEED_BOMB', 1, 3500), ('SHADOW_CLAW', 1, 3500), ('SNARL', 1, 3000), ('ACROBATICS', 1, 3500),
    ('ZEN_HEADBUTT', 1, 3500), ('POISON_FANG', 1, 3000), ('FIRE_FANG', 1, 3000), ('ICE_FANG', 1, 3000),
    ('THUNDER_FANG', 1, 3000), ('ANCIENT_POWER', 1, 3000), ('BREAKING_SWIPE', 1, 3500), ('SUBSTITUTE', 1, 3000),
    ('AGILITY', 1, 3000), ('IRON_DEFENSE', 1, 3000),
    # 3-4 badges: stronger STAB, screens, speed control, setup
    ('FIRE_PUNCH', 2, 6000), ('ICE_PUNCH', 2, 6000), ('THUNDER_PUNCH', 2, 6000), ('WILL_O_WISP', 2, 6000),
    ('SWORDS_DANCE', 2, 7000), ('GRASS_KNOT', 2, 6000), ('VENOSHOCK', 2, 6000), ('AQUA_TAIL', 2, 7000),
    ('DRAIN_PUNCH', 2, 7000), ('POISON_JAB', 2, 7000), ('X_SCISSOR', 2, 7000), ('IRON_HEAD', 2, 7000),
    ('CRUNCH', 2, 7000), ('AIR_SLASH', 2, 7000), ('PLAY_ROUGH', 2, 7000), ('ENCORE', 2, 6000),
    ('TAILWIND', 2, 7000), ('STEALTH_ROCK', 2, 7000), ('HEX', 2, 6000), ('POWER_GEM', 2, 6000),
    # 5-6 badges: strong attacks, pivots, priority, spread damage
    ('HEAT_WAVE', 3, 10000), ('HYPER_VOICE', 3, 10000), ('MUDDY_WATER', 3, 10000), ('ENERGY_BALL', 3, 10000),
    ('VOLT_SWITCH', 3, 12000), ('U_TURN', 3, 12000), ('KNOCK_OFF', 3, 12000), ('SUCKER_PUNCH', 3, 10000),
    ('FOUL_PLAY', 3, 10000), ('DARK_PULSE', 3, 10000), ('PSYSHOCK', 3, 10000), ('FLASH_CANNON', 3, 10000),
    ('DAZZLING_GLEAM', 3, 10000), ('DRAGON_PULSE', 3, 10000), ('EARTH_POWER', 3, 12000), ('STOMPING_TANTRUM', 3, 10000),
    ('WILD_CHARGE', 3, 10000), ('SCALD', 3, 12000), ('TOXIC_SPIKES', 3, 8000), ('SPIKES', 3, 8000),
    ('TRICK', 3, 10000), ('HEAVY_SLAM', 3, 10000), ('DOUBLE_EDGE', 3, 10000), ('SUPERPOWER', 3, 10000),
    # 7-8 badges: powerful attacks and specialised strategies
    ('CLOSE_COMBAT', 4, 18000), ('FOCUS_BLAST', 4, 15000), ('STONE_EDGE', 4, 15000), ('GUNK_SHOT', 4, 15000),
    ('OUTRAGE', 4, 18000), ('LEAF_STORM', 4, 15000), ('HYDRO_PUMP', 4, 15000), ('BRAVE_BIRD', 4, 18000),
    ('LIQUIDATION', 4, 15000), ('BUG_BUZZ', 4, 15000), ('MOONBLAST', 4, 15000), ('NASTY_PLOT', 4, 15000),
    ('TRICK_ROOM', 4, 15000), ('HIGH_HORSEPOWER', 4, 15000), ('PHANTOM_FORCE', 4, 15000), ('ROOST', 4, 15000),
    # Champion
    ('DRACO_METEOR', 5, 25000), ('DRAGON_DANCE', 5, 25000), ('AURA_SPHERE', 5, 25000),
]

SERVICES = {   # fixed prices in C (see src/coop_exchange.c); listed for the doc
    'Relearn a level-up or evolution move': (0, 1000),
    'Teach an Egg Move of its family': (2, 5000),
    'Reset all EVs of one Pokemon': (1, 2000),
    'Hyper Training: one stat (Lv. 50+)': (4, 15000),
    'Hyper Training: all six stats (Lv. 50+)': (5, 60000),
}

# Tier price bands (min, max), checked
BAND = [(100, 3000), (200, 8000), (500, 12000), (1000, 20000), (3000, 30000), (5000, 60000)]

def read(p):
    return open(os.path.join(ROOT, p)).read()

def main():
    check_only = '--check' in sys.argv
    errors = []
    items_h = read('src/data/items.h')
    item_names = set(re.findall(r'\[(ITEM_\w+)\]\s*=', items_h))
    moves_h = read('include/constants/moves.h')
    move_names = set(re.findall(r'\b(MOVE_\w+)\s*=', moves_h))
    special = json.loads(read('src/data/pokemon/special_movesets.json'))
    sig = set(special.get('signatureTeachables', []))
    seen = {}
    for counter, rows in STOCK.items():
        for row in rows:
            item, tier, price = row[0], row[1], row[2]
            quest = row[3] if len(row) > 3 else None
            c = 'ITEM_' + item
            if c not in item_names:
                errors.append('unknown item ' + c)
            if c in seen and seen[c][1] != price:
                errors.append('%s at two prices (%s, %s)' % (c, COUNTER_NAMES[seen[c][0]], COUNTER_NAMES[counter]))
            seen[c] = (counter, price)
            lo, hi = BAND[tier]
            if not (lo <= price <= hi):
                errors.append('%s price %d outside tier %d band %s' % (c, price, tier, BAND[tier]))
            if quest and quest not in QUESTS:
                errors.append('unknown quest ' + quest)
    # every evolution item of the roster
    evo_items = set()
    import glob
    for f in glob.glob(os.path.join(ROOT, 'src/data/pokemon/species_info/*.h')):
        s = open(f).read()
        evo_items |= set(re.findall(r'\{EVO_ITEM,\s*(ITEM_\w+)', s))
        evo_items |= set(re.findall(r'IF_HOLD_ITEM,\s*(ITEM_\w+)', s))
        evo_items |= set(re.findall(r'IF_BAG_ITEM_COUNT,\s*(ITEM_\w+)', s))
    excluded = {'ITEM_POKE_BALL': 'every Mart', 'ITEM_SCROLL_OF_DARKNESS': 'Kubfu (deferred legendary)',
                'ITEM_SCROLL_OF_WATERS': 'Kubfu (deferred legendary)'}
    for it in sorted(evo_items):
        if it not in seen and it not in excluded:
            errors.append('evolution item %s not stocked' % it)
    tutor_seen = set()
    for move, tier, price in TUTOR:
        c = 'MOVE_' + move
        if c not in move_names:
            errors.append('unknown move ' + c)
        if c in sig:
            errors.append('signature move ' + c)
        if c in tutor_seen:
            errors.append('tutor move twice ' + c)
        tutor_seen.add(c)
        lo, hi = BAND[tier]
        if not (lo <= price <= hi):
            errors.append('%s price %d outside tier %d band' % (c, price, tier))
    # TM moves are not also tutored
    tms = re.findall(r'F\((\w+)\)', read('include/constants/tms_hms.h'))
    for m in tms:
        if 'MOVE_' + m in tutor_seen:
            errors.append('MOVE_%s is a TM and a tutor move' % m)
    # No key items, Mega Stones, Z-Crystals or other battle-gimmick items.
    blocks = {}
    for m in re.finditer(r'\[(ITEM_\w+)\]\s*=\s*\{(.*?)\n    \},', items_h, re.S):
        blocks[m.group(1)] = m.group(2)
    for it in seen:
        b = blocks.get(it, '')
        if 'POCKET_KEY_ITEMS' in b:
            errors.append('%s is a key item' % it)
        for bad in ('HOLD_EFFECT_MEGA_STONE', 'HOLD_EFFECT_Z_CRYSTAL', 'HOLD_EFFECT_PRIMAL_ORB', 'HOLD_EFFECT_GEMS'):
            if bad in b:
                errors.append('%s has %s' % (it, bad))
    # Species compatibility: every tutor move has learners in the roster's
    # generated teachable learnsets; report how many.
    teach = read('src/data/pokemon/teachable_learnsets.h')
    learners = {}
    for m in re.finditer(r'static const u16 s(\w+)TeachableLearnset\[\] = \{(.*?)\};', teach, re.S):
        for mv in re.findall(r'(MOVE_\w+)', m.group(2)):
            learners[mv] = learners.get(mv, 0) + 1
    tutor_learners = {}
    for move, tier, price in TUTOR:
        n = learners.get('MOVE_' + move, 0)
        tutor_learners[move] = n
        if n == 0:
            errors.append('MOVE_%s: no species can learn it' % move)
    # The 18 types are all covered by the TM counter and the tutor together.
    moves_info = read('src/data/moves_info.h')
    def move_type(mv):
        m = re.search(r'\[%s\]\s*=\s*\{.*?\.type\s*=\s*(TYPE_\w+)' % mv, moves_info, re.S)
        return m.group(1) if m else None
    types = set()
    for move, _, _ in TUTOR:
        types.add(move_type('MOVE_' + move))
    for row in STOCK[TM]:
        tm = row[0][3:]  # TM_NAME
        types.add(move_type('MOVE_' + tm))
    want = {'TYPE_' + t for t in ('NORMAL FIGHTING FLYING POISON GROUND ROCK BUG GHOST STEEL FIRE WATER GRASS ELECTRIC PSYCHIC ICE DRAGON DARK FAIRY'.split())}
    if want - types:
        errors.append('types without a move: %s' % sorted(want - types))
    if errors:
        print('\n'.join(errors))
        sys.exit(1)
    print('stock ok: %d items, %d tutor moves, %d evolution items covered' % (len(seen), len(TUTOR), len(evo_items) - len(set(excluded) & evo_items)))
    print('18 types covered; no key, Mega, Z or gem items; tutor learners per move: min %d (%s), median %d' % (
        min(tutor_learners.values()), min(tutor_learners, key=tutor_learners.get),
        sorted(tutor_learners.values())[len(tutor_learners) // 2]))
    if '--learners' in sys.argv:
        for mv, n in sorted(tutor_learners.items(), key=lambda x: x[1]):
            print('  %-16s %d' % (mv, n))
    if check_only:
        return
    write_c(seen)
    write_doc()
    # the tutor moves need species compatibility data: extraTutors
    # (only the extraTutors list is rewritten, in the file's own layout)
    path = os.path.join(ROOT, 'src/data/pokemon/special_movesets.json')
    text = open(path).read()
    moves = sorted('MOVE_' + m for m, _, _ in TUTOR)
    body = '[\n' + ',\n'.join('      "%s"' % m for m in moves) + '\n    ]'
    text = re.sub(r'("extraTutors":\s*)\[[^\]]*\]', lambda m: m.group(1) + body, text)
    with open(path, 'w') as f:
        f.write(text)

def write_c(seen):
    out = ['// Generated by tools/coop_exchange/stock.py: the Rustboro Trainer Exchange.',
           '// Edit the stock there and run it again.', '']
    out.append('static const struct ExchangeStock sExchangeStock[] = {')
    for counter, rows in STOCK.items():
        for row in rows:
            quest = row[3] if len(row) > 3 else None
            out.append('    { ITEM_%s, %d, %d, %s },' % (row[0], counter, row[1], ('EXQ_' + quest) if quest else 'EXQ_NONE'))
    out.append('};')
    out.append('')
    out.append('static const struct ExchangeTutor sExchangeTutor[] = {')
    for move, tier, price in TUTOR:
        out.append('    { MOVE_%s, %d, %d },' % (move, tier, price))
    out.append('};')
    out.append('')
    prices = sorted(set(('ITEM_' + row[0], row[2]) for rows in STOCK.values() for row in rows))
    out.append('// Every shop sells these at these prices (GetItemPrice).')
    out.append('static const struct ExchangePrice sExchangePrices[] = {')
    for it, p in prices:
        out.append('    { %s, %d },' % (it, p))
    out.append('};')
    out.append('')
    out.append('static bool32 ExchangeQuestDone(u8 quest)')
    out.append('{')
    out.append('    switch (quest)')
    out.append('    {')
    for i, (q, (cond, _)) in enumerate(QUESTS.items()):
        out.append('    case EXQ_%s: return %s;' % (q, cond))
    out.append('    }')
    out.append('    return TRUE;')
    out.append('}')
    with open(os.path.join(ROOT, 'src/data/coop_exchange_stock.h'), 'w') as f:
        f.write('\n'.join(out) + '\n')

def write_doc():
    items_h = read('src/data/items.h')
    names = dict(re.findall(r'\[(ITEM_\w+)\]\s*=\s*\{\s*\.name\s*=\s*(?:ITEM_NAME|_|COMPOUND_STRING)\("([^"]*)"\)', items_h))
    moves_names = dict(re.findall(r'\[(MOVE_\w+)\]\s*=\s*\{\s*\.name\s*=\s*(?:MOVE_NAME|COMPOUND_STRING|_)\("([^"]*)"\)', read('src/data/moves_info.h')))
    moves_info = read('src/data/moves_info.h')
    def mname(mv):
        return moves_names.get(mv, mv[5:].replace('_', ' ').title())
    def iname(it):
        c = 'ITEM_' + it
        n = names.get(c, it.replace('_', ' ').title()).replace('{PKMN}', 'Pkmn')
        if it.startswith('TM_'):
            n += ' ' + mname('MOVE_' + it[3:])
        return n
    def mtype(mv):
        m = re.search(r'\[%s\]\s*=\s*\{.*?\.type\s*=\s*TYPE_(\w+)' % mv, moves_info, re.S)
        return m.group(1).title() if m else ''
    lines = ['# Trainer Exchange stock (Rustboro City)', '',
             'Generated by `tools/coop_exchange/stock.py` (run it after editing the stock; `--check` only validates). '
             'Prices apply in every shop that sells the item; selling gives a quarter. '
             'Unlocks use the buying player\'s own BADGES (and quest, where named); a partner\'s progress never counts. '
             'TMs are single-use in this ROM.', '',
             '## Items for sale by BADGES', '',
             '| Counter | ' + ' | '.join(TIER_NAMES) + ' |',
             '| --- |' + ' --- |' * len(TIER_NAMES)]
    for counter, rows in STOCK.items():
        counts = [sum(1 for r in rows if r[1] <= t) for t in range(len(TIER_NAMES))]
        lines.append('| %s | %s |' % (COUNTER_NAMES[counter], ' | '.join(str(c) for c in counts)))
    counts = [sum(1 for m in TUTOR if m[1] <= t) for t in range(len(TIER_NAMES))]
    lines.append('| Tutor moves | %s |' % ' | '.join(str(c) for c in counts))
    lines.append('')
    lines.append('Totals include everything from earlier steps. The ABILITY CAPSULE also needs the Twin Trial (one fewer before it).')
    lines.append('')
    for counter, rows in STOCK.items():
        lines.append('## %s' % COUNTER_NAMES[counter])
        lines.append('')
        lines.append('| Unlock | Item | Price |')
        lines.append('| --- | --- | --- |')
        for row in rows:
            quest = row[3] if len(row) > 3 else None
            unlock = TIER_NAMES[row[1]] + ((', ' + QUESTS[quest][1]) if quest else '')
            lines.append('| %s | %s | ₽%s |' % (unlock, iname(row[0]), format(row[2], ',')))
        lines.append('')
    lines.append('## Tutor moves (Moves & TMs counter, TEACH A MOVE)')
    lines.append('')
    lines.append('Each move is taught only to species whose teachable learnset has it; you pay once it is learned.')
    lines.append('')
    lines.append('| Unlock | Move | Type | Price |')
    lines.append('| --- | --- | --- | --- |')
    for move, tier, price in TUTOR:
        lines.append('| %s | %s | %s | ₽%s |' % (TIER_NAMES[tier], mname('MOVE_' + move), mtype('MOVE_' + move), format(price, ',')))
    lines.append('')
    lines.append('## Services')
    lines.append('')
    lines.append('| Unlock | Service | Price |')
    lines.append('| --- | --- | --- |')
    for name, (tier, price) in SERVICES.items():
        lines.append('| %s | %s | ₽%s |' % (TIER_NAMES[tier], name, format(price, ',')))
    with open(os.path.join(ROOT, 'docs/COOP_EXCHANGE_STOCK.md'), 'w') as f:
        f.write('\n'.join(lines) + '\n')

if __name__ == '__main__':
    main()
