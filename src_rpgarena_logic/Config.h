#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace cfg {

namespace Window {
inline const char *TITLE = "RPG";
inline int WIDTH = 0;
inline int HEIGHT = 0;
inline int INTERNAL_WIDTH = 640; // Virtual Resolution
inline int INTERNAL_HEIGHT = 360;
inline bool FULLSCREEN = true;
inline bool VSYNC = true;
inline int FPS_LIMIT = 60;
inline float CAMERA_ZOOM =
    3.0f; // World camera zoom (1.0 = no zoom, 3.0 = same as 360p view on 1080p)
inline bool ENABLE_ROTATION_DEBUG = false;
} // namespace Window

namespace Player {
inline float SPEED = 340.f;
inline float BASE_RANGE = 50.0f;
inline float BASE_MALICE = 0.0f;
// probando commit
//  Initial Attributes
inline int BASE_STR = 10;
inline int BASE_DEX = 10;
inline int BASE_INT = 10;
inline int BASE_VIT = 10;

// ACCURACY/EVASION
inline int BASE_ACCURACY = 100;
inline int BASE_EVASION = 200;

// Stat Derivation Multipliers
inline int HP_PER_VIT = 20;
inline int DEF_PER_VIT = 2;
inline int MP_PER_INT = 20;
inline int ATK_PER_STR = 3;

// Attack Speed
inline float BASE_ATK_SPEED = 1.4f;
inline float ATK_SPEED_PER_AGI = 0.1f;

// Initial Modifiers
inline float BASE_ARMOR_PEN_PERCENT = 0.0f;
inline float BASE_PHYSICAL_DMG_BONUS = 100.0f;
inline float BASE_CRIT_DAMAGE = 150.0f;
inline float BASE_CRIT_CHANCE = 0.0f;
inline int BASE_ARMOR_PEN_FLAT = 0;
inline float BASE_LIFESTEAL_PERCENT = 15.0f; // Robo de vida base

// MULTI STRIKE
inline float BASE_DOUBLE_STRIKE_CHANCE = 100.0f;
inline float BASE_TRIPLE_STRIKE_CHANCE = 0.0f;

// BONUS DAMAGE
inline float BASE_ENEMY_MAX_HP_DAMAGE_PERCENT = 0.0f;

// DEFENSIVE STATS
inline float BASE_BLOCK_CHANCE = 50.0f;
inline float BASE_BLOCK_VALUE_PERCENT = 50.0f; // Reduce 50% damage on block
inline float BASE_THORNS_PERCENT = 0.0f;
inline float BASE_HP_REGEN_PERCENT = 5.0f; // % HP per sec
inline float BASE_MP_REGEN_PERCENT = 5.0f; // % MP per sec

// NEW STATS
inline float BASE_TENACITY_PERCENT = 0.0f;         // Stun duration reduction
inline float BASE_DAMAGE_REDUCTION_PERCENT = 0.0f; // Direct damage mitigation
inline float BASE_CRIT_AVOIDANCE_PERCENT =
    0.0f; // Subtracts from enemy crit chance
inline float BASE_ANTI_ARMOR_PEN_PERCENT =
    0.0f;                                // Reduces effectiveness of enemy ArPen
inline int BASE_ANTI_ARMOR_PEN_FLAT = 0; // Reduces enemy Flat ArPen
inline float BASE_MANA_STEAL_PERCENT = 0.0f;         // % Mana restored on hit
inline float BASE_XP_BONUS_PERCENT = 0.0f;           // % Extra XP
inline float BASE_COOLDOWN_REDUCTION_PERCENT = 0.0f; // % Cooldown Reduction

// EXECUTE: Base stats
inline int BASE_EXECUTE_DAMAGE_PERCENT = 0;
inline int BASE_EXECUTE_THRESHOLD_PERCENT = 0; // % HP threshold

// TRUE DAMAGE
inline int BASE_TRUE_DAMAGE_PERCENT = 20;

// STUN
inline float BASE_STUN_CHANCE = 5.0f;
inline float BASE_STUN_DURATION = 1.0f;

// STUN VISUALS
inline float STUN_HALO_RADIUS = 8.0f;   // Radio del circulo de stun
inline float STUN_PARTICLE_SIZE = 2.0f; // Tamaño de las particulas de stun

// DEBUFFS
inline float BASE_SLOW_MOVE_PERCENT = 0.0f;
inline float BASE_SLOW_MOVE_DURATION = 0.0f;
inline float BASE_SLOW_ATTACK_PERCENT = 0.0f;
inline float BASE_SLOW_ATTACK_DURATION = 0.0f;

// AOE
inline float BASE_AOE_RADIUS = 60.0f; // Rango de daño de area
inline float BASE_AOE_DAMAGE_PERCENT =
    100.0f; // % del daño original aplicado en area (ej. 25%)

// BLEEDING
inline float BASE_BLEED_DURATION_FLAT = 0.0f;    // Duración sangrado fijo
inline float BASE_BLEED_DURATION_PERCENT = 0.0f; // Duración sangrado %
inline int BASE_BLEED_FLAT = 0;                  // Daño fijo por tick
inline float BASE_BLEED_PERCENT =
    0.0f; // % de vida máxima del objetivo por tick

// Visuals / Scale
inline float SCALE_X = 1.0f;
inline float SCALE_Y = 1.0f;
inline bool AUTO_FIT = false;
inline float MAX_W = 800.f;
inline float MAX_H = 800.f;

// Visual Limits
inline int FX_MAX_TEXTS = 100; // Further reduced to 100 to minimize draw calls
inline int GORE_MAX_GIBS = 3000;

// Leveling
inline long long BASE_NEXT_LEVEL_EXP = 50;
inline float EXP_CURVE_MULTIPLIER = 1.1f;
inline int STAT_GAIN_ON_LEVEL_UP = 10;
// test
//  Collision / Hitbox
inline float FEET_WIDTH = 30.f;
inline float FEET_HEIGHT = 15.f;

// animConfig
inline float GROUND_OFFSET_Y = 35.0f;
inline float BASE_ANIM_SPEED = 15.0f;
inline sf::Vector2f HEAD_OFFSET = {-10.f, -40.f};
inline sf::Vector2f HAND_L_OFFSET = {-12.f, -5.f};
inline sf::Vector2f HAND_R_OFFSET = {12.f, 5.f};
inline sf::Vector2f FOOT_L_OFFSET = {-6.f, 25.f};
inline sf::Vector2f FOOT_R_OFFSET = {6.f, 35.f};
inline float TAB_TARGET_RANGE = 350.f; // [NEW] Rango de seleccion con Tab
} // namespace Player

namespace Map {
// Tilemap / Render
inline unsigned TILE_SIZE = 64;
inline unsigned CHUNK_SIZE =
    8; // 8x8 tiles = 512x512 pixels (matches Visual Chunks)

inline unsigned TILESET_MARGIN_PX = 0;
inline unsigned TILESET_SPACING_PX = 0;
inline bool TILESET_SMOOTH = false;
inline float TEX_EPS = 0.01f;

inline unsigned WORLD_W_TILES = 0;
inline unsigned WORLD_H_TILES = 0;

// Rendering
inline float CULLING_MARGIN_PX = 800.f; // Increased to prevent pop-in

// Map Panel Window
inline float WINDOW_WIDTH = 800.f; // Bigger window
inline float WINDOW_HEIGHT = 600.f;
inline float WINDOW_INNER_OFFSET_X = 10.f; // [NEW] Offsets for Map Panel
inline float WINDOW_INNER_OFFSET_Y = 30.f;
inline float ZOOM_FACTOR = 4.0f;
inline float DEFAULT_ZOOM = 10.0f;

inline float MARKER_RADIUS = 5.0f;
inline sf::Color MARKER_COLOR = sf::Color::Red;
inline sf::Color MARKER_OUTLINE_COLOR = sf::Color::White;
inline bool DEBUG_VIEW_MAP_COMPLETE = false;
} // namespace Map

namespace Mob {
inline float PATROL_RADIUS = 100.f;
inline float PATROL_RADIUS_SQ = PATROL_RADIUS * PATROL_RADIUS;

inline float LEASH_TIME = 5.0f;
inline float BASE_RANGE = 50.0f;

// AI Response Time: Reduced for snappier behavior
inline float IDLE_TIME_MIN = 0.1f;
inline float IDLE_TIME_MAX = 0.3f;

// Respawn System
inline float RESPAWN_TIME = 10.0f;
inline float RESPAWN_JITTER = 5.0f;
inline float FADE_IN_DURATION = 1.0f;
inline float HIT_KNOCKBACK_DIST = 10.0f;
} // namespace Mob

namespace Combat {
inline float PLAYER_ATTACK_RANGE_PX = 50.0f;
inline float DEFENSE_CONSTANT_BASE = 100.f;
inline float DEFENSE_CONSTANT_LEVEL_SCALE = 15.f;

// Level Scaling Config
inline float LEVEL_DIFF_DAMAGE_PENALTY = 0.02f; // For Damage Multiplier
inline float LEVEL_DIFF_HIT_PENALTY = 0.002f;   // For Accuracy/Evasion Formula

inline float MIN_LEVEL_MULTIPLIER = 0.1f;
inline float MAX_LEVEL_MULTIPLIER = 1.5f;
// Separate timers for responsiveness vs safety
inline float COMBAT_COOLDOWN_HIT =
    5.0f; // Keep combat long if taking damage (safety)

inline float COMBAT_COOLDOWN_ATTACK =
    0.6f; // Drop combat fast if just attacking (responsiveness)

inline int DAMAGE_VARIANCE_PERCENT = 6; // +/- 6% variance

// Delay Damage: 0.5 = 50% of animation duration
inline float PLAYER_ATTACK_DELAY_FACTOR = 0.6f;

// Accuracy Formula: Chance = Acc / (Acc + Eva * Factor)
inline float ACCURACY_EVASION_FACTOR = 1.0f;

inline float AUTO_DESELECT_MARGIN = 20.0f;

// Death Knockback Physics
inline float KNOCKBACK_WEIGHT_FACTOR = 1.0f;
inline float KNOCKBACK_STRENGTH_FACTOR = 1.0f;
} // namespace Combat

namespace Gore {
inline float GRAVITY = 500.f;
inline float GROUND_SPREAD_MIN = -3.f;
inline float GROUND_SPREAD_MAX = 10.f;
inline float UPWARD_BOOST_BASE = -40.f;
inline float HEIGHT_MULTIPLIER = 1.5f;
inline float H_SPEED_MIN = 20.f;
inline float H_SPEED_MAX = 60.f;
inline float ANGULAR_VEL_MIN = -360.f;
inline float ANGULAR_VEL_MAX = 360.f;
inline float LIFETIME_MIN = 4.0f;
inline float LIFETIME_MAX = 6.0f;
inline float FADE_DURATION = 1.0f;
inline float RESTITUTION = 0.5f;
inline float FRICTION = 0.8f;
inline float MIN_BOUNCE_VELOCITY = 20.0f;
inline float CONSTRAINT_MAX_DIST_FACTOR = 1.6f;
inline float SPRING_PULL_FACTOR = 0.15f;
inline bool LOOT_ARMOR_ATTACHED = true;
inline bool ENABLE_BONE_DECAY = true;
inline float DECAY_DELAY_SEC = 1.0f;
inline float DECAY_FADE_DURATION = 1.5f;
inline float BONE_LIFETIME_SEC = 15.0f;
inline float BONE_FADE_DURATION = 3.0f;
inline float SINK_DISTANCE = 15.0f; // Warcraft 3 style ground sinking distance in pixels
inline float CLIP_OFFSET_Y = 12.0f; // Pixels to move eraser clipping line higher up
} // namespace Gore

namespace World {
inline float BOUNDS_MARGIN = 20.f;
inline std::string INITIAL_WORLD = "level1";
inline float INITIAL_SPAWN_X = 1173.0f;
inline float INITIAL_SPAWN_Y = 1007.0f;
} // namespace World

namespace Debug {
inline int XP_GAIN = 350000;
inline int STAT_BOOST = 1000; // ahora añade agilidad cada vez que se pulsa la
                              // J
inline bool ENABLE_WEAPONS_DEBUG = true;
inline bool ENABLE_TRACY = false;
inline bool ENABLE_CHAR_PANEL_SLOTS_DEBUG = false;

// Performance Logging
inline bool ENABLE_PERF_LOG = false;  // Toggle console logging (F3)
inline bool ENABLE_PERF_CHAT = false; // Toggle chat logging

// Logic Culling Debug
inline bool ENABLE_CULLING_DEBUG = false; // Set to true to see blue frozen mobs
inline float CULLING_DEBUG_MARGIN =
    -250.f; // Negative margin to see culling on screen

// Visual Toggles
inline bool ENABLE_FLOATING_TEXT = true;
inline bool ENABLE_COMBAT_PARTICLES = true;
inline bool ENABLE_DEBUG_OVERLAY = false;
inline bool SHOW_OCCLUSION_GREEN = false;
inline bool SHOW_PART_SORTING_POINTS = true;
} // namespace Debug

namespace UI {
inline float CURSOR_HOTSPOT_X = 0.0f;
inline float CURSOR_HOTSPOT_Y = 0.0f;
inline float LOGICAL_WIDTH = 1366.f;
inline float LOGICAL_HEIGHT = 768.f;
inline float FONT_SCALE = 2.0f;

inline float DAMAGE_NUMBER_DURATION = 60.f;

// DAMAGE NUMBERS
inline float DAMAGE_OFFSET_BASE = 20.f;  // Closer to head (was 60)
inline float DAMAGE_OFFSET_STACK = 10.f; // Tighter stack (was 55)

// Common Layout (Pixel Perfect)
inline float BASE_SLOT_SIZE = 20.0f;   // Tamaño base del PNG del slot (patrón)
inline float BASE_ICON_SIZE = 16.0f;   // Tamaño base de iconos/ítems (patrón)
inline float UNIFIED_SLOT_SIZE = 60.f; // (Legacy)
inline float UNIFIED_SLOT_MARGIN = 6.f;
inline float PANEL_TITLE_HEIGHT = 30.f;
inline float COMMON_MARGIN = 10.f;

// Minimap
inline float MINIMAP_UPDATE_RATE = 0.033f; // ~30 FPS
inline float MINIMAP_DIAMETER_DEFAULT = 220.f;
inline float MINIMAP_MARGIN_DEFAULT = 12.f;
inline float MINIMAP_WIDTH_FRACTION = 0.22f;
inline float MINIMAP_MARGIN_FRACTION = 0.02f;
inline float MINIMAP_MIN_DIAMETER = 140.f;
inline float MINIMAP_MIN_MARGIN = 6.f;

inline unsigned MINIMAP_VIEW_SIZE_TILES = 50;

// Experience Bar
inline float EXP_BAR_HEIGHT = 15.0f;
inline float EXP_BAR_BOTTOM_OFFSET = 95.0f;

// HUD / Hotbar
inline int HOTBAR_SLOTS = 12;
inline float SLOT_MARGIN = 6.f;

inline float HUD_BG_OFFSET_Y = 0.f;
inline float HUD_SLOTS_OFFSET_X = 0.f;
inline float HUD_SLOTS_OFFSET_Y = 0.f;
inline float HUD_SLOT_HEIGHT_PERCENT = 0.08f; // 8% del alto
inline float HUD_SLOT_MIN_SIZE = 40.f;
inline float HUD_SLOT_MAX_SIZE = 100.f;
inline float COMBAT_STATUS_Y_PERCENT = 0.15f;

// Skill Icon Offset
inline float SKILL_ICON_OFFSET_X = 3.f;
inline float SKILL_ICON_OFFSET_Y = 3.f;

namespace FortifyPanel {
inline float X = 400.f;
inline float Y = 100.f;
inline float SLOT_OFFSET_X = 50.f;
inline float SLOT_OFFSET_Y = 50.f;
inline float BUTTON_OFFSET_X = 35.f;
inline float BUTTON_OFFSET_Y = 100.f;

inline float CLOSE_BTN_X = 110.f;  // [NEW]
inline float CLOSE_BTN_Y = 5.f;    // [NEW]
inline float CLOSE_BTN_SIZE = 5.f; // [NEW]

inline float LOADING_BAR_OFFSET_X = 50.f;
inline float LOADING_BAR_OFFSET_Y = 80.f;
inline float LOADING_TIME_SECONDS = 0.15f;
} // namespace FortifyPanel

// Centralized UI Config
namespace CharacterPanel {
inline float WIDTH = 460.f;
inline float HEIGHT = 500.f;
// inline float SLOT_SIZE = 60.f; // [DEPRECATED] Use UNIFIED_SLOT_SIZE
// inline float SLOT_MARGIN = 6.f; // [DEPRECATED] Use UNIFIED_SLOT_MARGIN
inline float MARGIN = 10.f;
inline int FONT_SIZE = 10;
inline float LINE_SPACING = 15.f;

// Equipment Grid Position
inline float EQUIP_GRID_OFFSET_X = 210.f;
inline float EQUIP_GRID_OFFSET_Y = 50.f;

// Positioning & Offsets
inline float TEXT_OFFSET_X = 10.f;
inline float TEXT_OFFSET_Y = 10.f;
inline float TITLE_HEIGHT = 40.f;
inline float CLOSE_BTN_X = 400.f;
inline float CLOSE_BTN_Y = 5.f;
inline float CLOSE_BTN_SIZE = 20.f;
} // namespace CharacterPanel

namespace PlayerFrame {
inline float BAR_WIDTH = 150.f;
inline float BAR_HEIGHT = 18.f;
inline float PORTRAIT_SIZE = 60.f;
inline float PORTRAIT_VIEW_SIZE = 50.f;
inline float PORTRAIT_VIEW_OFFSET_Y = 18.f;

// Layout Offsets
inline float PORTRAIT_OFFSET_X = 8.f;
inline float PORTRAIT_OFFSET_Y = 8.f;
inline float TEXT_BLOCK_OFFSET_X = 76.f;
inline float TEXT_BLOCK_OFFSET_Y = 8.f;
inline float HP_BAR_OFFSET_Y = 22.f;
inline float MP_BAR_SPACING = 4.f;
inline float HP_BAR_X = 36.f;
inline float HP_BAR_Y = 9.f;
inline float MP_BAR_X = 36.f;
inline float MP_BAR_Y = 19.f;
} // namespace PlayerFrame

namespace Inventory {
inline int COLS = 5;
inline int ROWS = 4;
inline int TOTAL_SLOTS = 20;
inline float ICON_SCALE = 0.8f;
inline float GRID_OFFSET_X = 4.0f;
inline float GRID_OFFSET_Y = 10.0f;
inline float CLOSE_BTN_X = 100.0f;
inline float CLOSE_BTN_Y = 0.0f;
inline float CLOSE_BTN_SIZE = 6.0f;
} // namespace Inventory

namespace TargetFrame {
inline float PORTRAIT_SIZE = 60.f;
inline float PORTRAIT_VIEW_SIZE = 80.f;
inline float PORTRAIT_VIEW_OFFSET_Y = -20.f;
inline float MARGIN = 10.f;
inline float BAR_WIDTH = 150.f;
inline float BAR_HEIGHT = 18.f; // Legacy check usage

// Text Positions
inline float NAME_OFFSET_Y = 22.f;
inline float BAR_SPACING = 4.f;
inline float LABEL_OFFSET_X = 4.f;
inline float LABEL_OFFSET_Y = 18.f;
inline float VALUE_OFFSET_Y = 1.f;

// Layout Offsets
inline float PORTRAIT_OFFSET_X = 8.f;
inline float PORTRAIT_OFFSET_Y = 8.f;
inline float TEXT_BLOCK_OFFSET_X = 76.f;
inline float TEXT_BLOCK_OFFSET_Y = 8.f;

inline float HP_BAR_X = 36.f;
inline float HP_BAR_Y = 9.f;
inline float MP_BAR_X = 36.f;
inline float MP_BAR_Y = 19.f;
} // namespace TargetFrame

namespace Chat {
inline float FALLBACK_WIDTH = 400.f;
inline float FALLBACK_HEIGHT = 200.f;
inline float MARGIN_LEFT = 10.f;
inline float MARGIN_BOTTOM = 10.f;
inline float PADDING_LEFT = 8.f;
inline float PADDING_RIGHT = 8.f;
inline float PADDING_TOP = 8.f;
inline float PADDING_BOTTOM = 8.f;
} // namespace Chat

inline float DAMAGE_SCALE_BASE = 1.0f; // Asset is drawn 1:1
inline float DAMAGE_SCALE_BONUS_HIT =
    0.75f; // [REDUCED] to distinguish secondary hits in multi-strike

namespace FloatingText {
inline int SIZE_NORMAL = 20;
inline int SIZE_CRIT = 22;
inline int SIZE_XP = 22;
inline int SIZE_MISS = 20;

inline float LIFETIME_NORMAL = 0.8f; // Faster (was 1.5)
inline float LIFETIME_XP = 1.5f;     // (Was 2.0)
inline float LIFETIME_MISS = 0.8f;   // (Was 1.0)

inline float VELOCITY_Y_NORMAL = -10.f; // Slower float (was -30)
inline float VELOCITY_Y_FAST = -20.f;   // (Was -40)
inline float VELOCITY_Y_SLOW = -5.f;    // (Was -20)

inline float OFFSET_Y_DAMAGE = 20.f; // Unified with DAMAGE_OFFSET_BASE (was 40.f)
inline float OFFSET_Y_TRUE_EXTRA = 10.f;
inline float OFFSET_Y_HEAL = 25.f;
inline float OFFSET_Y_MISS = 25.f;
inline float OFFSET_Y_XP = 30.f;
} // namespace FloatingText

namespace Tooltip {
inline float BORDER_SIZE = 2.0f;
inline sf::Color BORDER_COLOR = sf::Color(100, 100, 100, 255);
inline sf::Color BG_COLOR = sf::Color(20, 20, 20, 230);
} // namespace Tooltip
} // namespace UI

namespace Decor {
inline int GRID_CELL_SIZE = 256;

// Tree Hitbox
inline float TRUNK_WIDTH = 30.0f;
inline float TRUNK_HEIGHT = 10.0f;

// Scales
inline float SCALE_SMALL_PLANT = 1.0f;
inline float SCALE_TREE = 1.0f;
inline float SCALE_DEFAULT = 1.0f;
} // namespace Decor

namespace Resources {
inline unsigned ERROR_TEXTURE_SIZE = 32;
}

namespace Optimization {
// Active/Sleeping Partition
// Frequency to check if sleeping entities should wake up (in seconds).
// Lower = more reactive, Higher = better CPU performance.
inline float WAKE_UP_CHECK_INTERVAL = 0.1f;

inline float WAKE_UP_MARGIN_PX = 300.0f;   // Reduced from 400
inline float SLEEP_HYSTERESIS_PX = 100.0f; // Reduced from 200

// Spatial Grid
inline int GRID_CELL_SIZE = 256;

inline int MAX_RESPAWNS_PER_FRAME = 3;
} // namespace Optimization

namespace Terrain {
inline bool ENABLE_TERRAIN_DEFORM = true;

// Sensibilidad de pisada: cuánto debe levantarse el pie (px efectivos)
// para registrarse como "en el aire" y luego dejar huella al bajar.
// 0.01 = cualquier movimiento deja huella | 1.5 = requiere paso completo.
inline float FOOTPRINT_LIFT_THRESHOLD = 0.01f;

// Profundidad visual de la capa de tierra (px que la tierra aparece por debajo
// del pasto). Ajustar en config.json sin recompilar.
inline float DIRT_OFFSET_PX = 1.0f;
inline float EXPLOSION_OFFSET_PX = 30.0f; // [NEW] Explosion depth
inline float OCCLUSION_PROJECTION_PX =
    21.0f; // Offset projection for 2.5D crater occlusion
inline float DIRT_REGEN_TIME_SEC = 4.0f;  // Time to heal full depth (255)
inline float GRASS_REGEN_TIME_SEC = 5.0f; // Time to heal grass fully
inline bool DEBUG_PAINT_ACTIVE_CHUNKS = false;
} // namespace Terrain

namespace ItemDrop {
inline float FLOAT_SPEED = 3.0f;
inline float FLOAT_AMPLITUDE = 5.0f;
inline float FLOAT_OFFSET_Y = 0.0f;
} // namespace ItemDrop

namespace YSorting {
inline float PLAYER = 30.0f;
inline float PORTAL = 20.0f;
inline float DECOR_TREE = 10.0f;
inline float ITEM_DROP = 0.0f;
} // namespace YSorting

namespace FX {
inline float HIT_RING_ALPHA = 1.0f; // 0.0 to 1.0
}

namespace Audio {
inline int MAX_SOUNDS = 16;
inline float FOOTSTEP_VOLUME = 15.0f;
inline float MOB_FOOTSTEP_MAX_DISTANCE = 500.0f;
} // namespace Audio

namespace PostFX {
inline bool ENABLED = false; // Master toggle
// Ordered Dithering (Bayer 4×4) — reduces color banding
inline float DITHER_STRENGTH = 0.0f; // 0 = off, 1 = full
// Film Grain — animated noise overlay
inline float GRAIN_STRENGTH = 0.0f; // 0 = off, 1 = heavy
// Pixel Grid — faint grid lines at pixel boundaries
inline float GRID_STRENGTH = 0.04f; // 0 = off, keep ≤ 0.1
// Vignette — corner darkening
inline float VIGNETTE_STRENGTH = 0.15f; // 0 = off, 1 = very dark
// Palette Limiting — posterization
inline bool PALETTE_ENABLED = false;
inline float PALETTE_LEVELS = 24.0f; // Higher = more colors
// Color Grading
inline float BRIGHTNESS = 0.0f; // Additive (-0.1 to 0.1)
inline float CONTRAST = 1.0f;   // Multiplicative (0.8 to 1.2)
inline float TINT_R = 1.0f;     // RGB tint multipliers
inline float TINT_G = 1.0f;
inline float TINT_B = 1.0f;
} // namespace PostFX

namespace Particles {
inline float GRAVITY = 800.f;
inline float BASE_SIZE = 1.0f;

// Walk Dust
inline int WALK_DUST_COUNT_MIN = 1;
inline int WALK_DUST_COUNT_MAX = 1;
inline float WALK_DUST_OFFSET_X = 0.0f;
inline float WALK_DUST_OFFSET_Y = 0.0f;
inline float WALK_DUST_SPEED_X = 20.f;
inline float WALK_DUST_SPEED_Y_MIN = -30.f;
inline float WALK_DUST_SPEED_Y_MAX = -10.f;
inline float WALK_DUST_LIFE_MIN = 1.0f;
inline float WALK_DUST_LIFE_MAX = 1.5f;
inline float WALK_DUST_COLOR_MULT = 0.6f;
inline int WALK_DUST_ALPHA = 200;

// Profiles
inline int HIT_COUNT_MIN = 3;
inline int HIT_COUNT_MAX = 5;
inline float HIT_SPEED_MIN = 30.f;
inline float HIT_SPEED_MAX = 150.f;
inline float HIT_LIFE_MIN = 0.5f;
inline float HIT_LIFE_MAX = 1.0f;
inline float HIT_BIAS_Y = -100.f;

inline int ROCK_COUNT_MIN = 5;
inline int ROCK_COUNT_MAX = 10;
inline float ROCK_SPEED_MIN = 5.f;
inline float ROCK_SPEED_MAX = 20.f;
inline float ROCK_LIFE_MIN = 1.0f;
inline float ROCK_LIFE_MAX = 1.5f;

inline int DEBRIS_COUNT_MIN = 2;
inline int DEBRIS_COUNT_MAX = 4;
inline float DEBRIS_SPEED_MIN = 30.f;
inline float DEBRIS_SPEED_MAX = 80.f;
inline float DEBRIS_LIFE_MIN = 0.5f;
inline float DEBRIS_LIFE_MAX = 1.0f;
inline float DEBRIS_BIAS_Y = -120.f;
inline float DEBRIS_GRAVITY_SCALE = 2.0f;

inline int POWER_COUNT_MIN = 5;
inline int POWER_COUNT_MAX = 10;
inline float POWER_SPEED_MIN = 100.f;
inline float POWER_SPEED_MAX = 200.f;
inline float POWER_LIFE_MIN = 0.4f;
inline float POWER_LIFE_MAX = 0.8f;

inline int BLOOD_COUNT_MIN = 1;
inline int BLOOD_COUNT_MAX = 3;
inline float BLOOD_SPEED_MIN = 5.f;
inline float BLOOD_SPEED_MAX = 20.f;
inline float BLOOD_LIFE_MIN = 0.5f;
inline float BLOOD_LIFE_MAX = 1.0f;

inline int CHARGE_COUNT_MIN = 8;
inline int CHARGE_COUNT_MAX = 12;
inline float CHARGE_SPEED_MIN = 20.f;
inline float CHARGE_SPEED_MAX = 40.f;
inline float CHARGE_LIFE_MIN = 0.3f;
inline float CHARGE_LIFE_MAX = 0.6f;
inline float CHARGE_GRAVITY_SCALE = -2.0f;

inline int CHARGE_TICK_COUNT_MIN = 1;
inline int CHARGE_TICK_COUNT_MAX = 1;
inline float CHARGE_TICK_SPEED_MIN = 2.f;
inline float CHARGE_TICK_SPEED_MAX = 8.f;
inline float CHARGE_TICK_LIFE_MIN = 0.4f;
inline float CHARGE_TICK_LIFE_MAX = 0.6f;
inline float CHARGE_TICK_GRAVITY_SCALE = -0.5f;
} // namespace Particles

namespace Shadow {
inline float SCALE_Y = 0.3f;
inline float ALPHA = 160.f;
inline float OFFSET_X = 0.f;
inline float OFFSET_Y = 0.f;
inline float SCALE_X = 1.0f;
inline float SKEW_X = 0.0f;
inline float SUN_ANGLE = 0.0f; // Sun angle in degrees (-75 to 75)

// Contorno de Sombra 2D (Sobel Contour)
inline bool ENABLE_CONTOUR = true;       // Activar o desactivar contorno de sombra
inline float CONTOUR_THICKNESS = 1.2f;   // Grosor del contorno (1.0 = 1px nitido, 2.0 = 2px)
inline float SOBEL_STEP = 2.0f;          // Paso / resolucion del filtro Sobel (1.0=fino, 2.0=normal, 3.0=grueso)
inline float CONTOUR_ALPHA = 1.0f;       // Factor multiplicador de opacidad del contorno (0.0 a 1.0)
inline float CONTOUR_BASE_ALPHA = 0.35f;  // Opacidad base 360 grados del contorno (0.0 = solo sombra inferior, 0.35 = contorno completo cerrado)
inline int CONTOUR_COLOR_R = 6;          // Color del contorno R (0 - 255)
inline int CONTOUR_COLOR_G = 4;          // Color del contorno G (0 - 255)
inline int CONTOUR_COLOR_B = 8;          // Color del contorno B (0 - 255)
} // namespace Shadow

namespace IK {
inline float RECOIL_STIFFNESS = 180.f;
inline float RECOIL_DAMPING = 14.f;
inline float RECOIL_FORCE_LIN = 50.f;
inline float RECOIL_FORCE_ROT = 20.f;

inline float ATTACK_STIFFNESS = 160.f;
inline float ATTACK_DAMPING = 12.f;
inline float ATTACK_FORCE_LIN = 50.f;
inline float ATTACK_FORCE_ROT = 15.f;

inline float SWAY_LEAN_MULT = 0.04f;
inline float SWAY_MAX_LEAN = 14.f;
inline float WEIGHT_LAG_MULT = 0.012f;
inline float SWAY_SMOOTH_FREQ = 15.f;
inline float LEAN_SMOOTH_FREQ = 10.f;

inline float FOOT_LERP_FREQ = 10.f;
inline float FOOT_SAMPLE_OFFSET = 5.f;
inline float FOOT_MAX_ROT = 50.f;
inline float BODY_DEPTH_MULT = 0.6f;

inline float HITSTOP_TIME_SCALE = 0.1f;

inline float GRIP_OFFSET_X = 12.f;
inline float GRIP_OFFSET_Y = 4.f;
inline float GRIP_LERP_FACTOR = 0.75f;
inline float GRIP_ROT_OFFSET = 0.f;

inline float STEP_IMPACT_STIFFNESS = 240.f;
inline float STEP_IMPACT_DAMPING = 18.f;
inline float STEP_IMPACT_FORCE = 40.f;
inline float HEAD_IMPACT_LAG = 1.15f;
inline float WEIGHT_STANCE_SINK = 0.6f;
} // namespace IK

namespace Wind {
inline bool ENABLE = true;
inline float SPEED = 1.8f;
inline float STRENGTH = 2.0f;
inline float FREQUENCY = 0.007f;
inline float DIRECTION_X = 1.0f;
inline float DIRECTION_Y = 0.5f;
inline float TURBULENCE = 0.35f;
inline float TURBULENCE_SPEED = 2.7f;
inline float TURBULENCE_SPATIAL_MULT = 3.1f;
inline float GUST_STRENGTH = 1.8f;
inline float GUST_FREQUENCY = 0.25f;
inline float GUST_SPATIAL_FACTOR = 0.3f;
inline float GUST_TURBULENCE_BOOST = 0.5f;
inline float TRUNK_THRESHOLD = 0.30f;
inline float CANOPY_CURVE_EXPONENT = 1.8f;
inline float MICRO_CURVE_MULTIPLIER = 2.0f;
} // namespace Wind

} // namespace cfg
