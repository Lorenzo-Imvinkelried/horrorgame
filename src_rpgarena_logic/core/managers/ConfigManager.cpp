#include "ConfigManager.h"

#include "Config.h"
#include "core/systems/WeightSystem.h"
#include "utils/TinyJson.h"
#include <iostream>
#include <cmath>

void ConfigManager::loadConfig(const std::string& path) {
    std::cout << "[ConfigManager] Loading config from " << path << "...\n";
    
    json::Value root = json::parseFile(path);
    if (root.type == json::Type::Null) {
        std::cerr << "[ConfigManager] ERROR: Failed to load config file. Using hardcoded defaults.\n";
        return;
    }

    const auto& rootObj = root.asObject();

    // --- WINDOW ---
    if (rootObj.count("window")) {
        const auto& winInfo = rootObj.at("window").asObject();
        if (winInfo.count("width")) cfg::Window::WIDTH = winInfo.at("width").asInt();
        if (winInfo.count("height")) cfg::Window::HEIGHT = winInfo.at("height").asInt();
        if (winInfo.count("internal_width")) cfg::Window::INTERNAL_WIDTH = winInfo.at("internal_width").asInt(); // [NEW]
        if (winInfo.count("internal_height")) cfg::Window::INTERNAL_HEIGHT = winInfo.at("internal_height").asInt(); // [NEW]
        if (winInfo.count("fullscreen")) cfg::Window::FULLSCREEN = winInfo.at("fullscreen").asBool();
        if (winInfo.count("vsync")) cfg::Window::VSYNC = winInfo.at("vsync").asBool();
        if (winInfo.count("fps_limit")) cfg::Window::FPS_LIMIT = winInfo.at("fps_limit").asInt();
        if (winInfo.count("camera_zoom")) cfg::Window::CAMERA_ZOOM = (float)winInfo.at("camera_zoom").asDouble();
        if (winInfo.count("enable_rotation_debug")) cfg::Window::ENABLE_ROTATION_DEBUG = winInfo.at("enable_rotation_debug").asBool();
    }

    // --- PLAYER ---
    if (rootObj.count("player")) {
        const auto& val = rootObj.at("player");
        const auto& playerInfo = val.asObject();
        
        if (playerInfo.count("speed")) cfg::Player::SPEED = (float)playerInfo.at("speed").asDouble();
        if (playerInfo.count("tab_target_range")) cfg::Player::TAB_TARGET_RANGE = (float)playerInfo.at("tab_target_range").asDouble();
        if (playerInfo.count("base_range")) cfg::Player::BASE_RANGE = (float)playerInfo.at("base_range").asDouble();
        if (playerInfo.count("base_malice")) cfg::Player::BASE_MALICE = (float)playerInfo.at("base_malice").asDouble();
        
        if (playerInfo.count("base_str")) cfg::Player::BASE_STR = playerInfo.at("base_str").asInt();
        if (playerInfo.count("base_dex")) cfg::Player::BASE_DEX = playerInfo.at("base_dex").asInt();
        if (playerInfo.count("base_int")) cfg::Player::BASE_INT = playerInfo.at("base_int").asInt();
        if (playerInfo.count("base_vit")) cfg::Player::BASE_VIT = playerInfo.at("base_vit").asInt();

        if (playerInfo.count("base_accuracy")) cfg::Player::BASE_ACCURACY = playerInfo.at("base_accuracy").asInt();
        if (playerInfo.count("base_evasion")) cfg::Player::BASE_EVASION = playerInfo.at("base_evasion").asInt();

        if (playerInfo.count("hp_per_vit")) cfg::Player::HP_PER_VIT = playerInfo.at("hp_per_vit").asInt();
        if (playerInfo.count("def_per_vit")) cfg::Player::DEF_PER_VIT = playerInfo.at("def_per_vit").asInt();
        if (playerInfo.count("mp_per_int")) cfg::Player::MP_PER_INT = playerInfo.at("mp_per_int").asInt();
        if (playerInfo.count("atk_per_str")) cfg::Player::ATK_PER_STR = playerInfo.at("atk_per_str").asInt();

        if (playerInfo.count("base_atk_speed")) cfg::Player::BASE_ATK_SPEED = (float)playerInfo.at("base_atk_speed").asDouble();
        if (playerInfo.count("atk_speed_per_agi")) cfg::Player::ATK_SPEED_PER_AGI = (float)playerInfo.at("atk_speed_per_agi").asDouble();

        if (playerInfo.count("base_armor_pen_percent")) cfg::Player::BASE_ARMOR_PEN_PERCENT = (float)playerInfo.at("base_armor_pen_percent").asDouble();
        if (playerInfo.count("base_physical_dmg_bonus")) cfg::Player::BASE_PHYSICAL_DMG_BONUS = (float)playerInfo.at("base_physical_dmg_bonus").asDouble();
        if (playerInfo.count("base_crit_damage")) cfg::Player::BASE_CRIT_DAMAGE = (float)playerInfo.at("base_crit_damage").asDouble();
        if (playerInfo.count("base_crit_chance")) cfg::Player::BASE_CRIT_CHANCE = (float)playerInfo.at("base_crit_chance").asDouble(); // [NEW] Parsing
        if (playerInfo.count("base_armor_pen_flat")) cfg::Player::BASE_ARMOR_PEN_FLAT = playerInfo.at("base_armor_pen_flat").asInt();
        if (playerInfo.count("base_lifesteal_percent")) cfg::Player::BASE_LIFESTEAL_PERCENT = (float)playerInfo.at("base_lifesteal_percent").asDouble();

        if (playerInfo.count("base_double_strike_chance")) cfg::Player::BASE_DOUBLE_STRIKE_CHANCE = (float)playerInfo.at("base_double_strike_chance").asDouble();
        if (playerInfo.count("base_triple_strike_chance")) cfg::Player::BASE_TRIPLE_STRIKE_CHANCE = (float)playerInfo.at("base_triple_strike_chance").asDouble();

        if (playerInfo.count("base_enemy_max_hp_damage_percent")) cfg::Player::BASE_ENEMY_MAX_HP_DAMAGE_PERCENT = (float)playerInfo.at("base_enemy_max_hp_damage_percent").asDouble();
        if (playerInfo.count("base_block_chance")) cfg::Player::BASE_BLOCK_CHANCE = (float)playerInfo.at("base_block_chance").asDouble();
        if (playerInfo.count("base_block_value_percent")) cfg::Player::BASE_BLOCK_VALUE_PERCENT = (float)playerInfo.at("base_block_value_percent").asDouble();
        
        if (playerInfo.count("base_thorns_percent")) cfg::Player::BASE_THORNS_PERCENT = (float)playerInfo.at("base_thorns_percent").asDouble();
        if (playerInfo.count("base_hp_regen_percent")) cfg::Player::BASE_HP_REGEN_PERCENT = (float)playerInfo.at("base_hp_regen_percent").asDouble();
        if (playerInfo.count("base_mp_regen_percent")) cfg::Player::BASE_MP_REGEN_PERCENT = (float)playerInfo.at("base_mp_regen_percent").asDouble();
        
        if (playerInfo.count("base_execute_damage_percent")) cfg::Player::BASE_EXECUTE_DAMAGE_PERCENT = playerInfo.at("base_execute_damage_percent").asInt();
        if (playerInfo.count("base_execute_threshold_percent")) cfg::Player::BASE_EXECUTE_THRESHOLD_PERCENT = playerInfo.at("base_execute_threshold_percent").asInt();
        if (playerInfo.count("base_true_damage_percent")) cfg::Player::BASE_TRUE_DAMAGE_PERCENT = playerInfo.at("base_true_damage_percent").asInt();
        
        // [NEW STATS PARSING]
        if (playerInfo.count("base_tenacity_percent")) cfg::Player::BASE_TENACITY_PERCENT = (float)playerInfo.at("base_tenacity_percent").asDouble();
        if (playerInfo.count("base_damage_reduction_percent")) cfg::Player::BASE_DAMAGE_REDUCTION_PERCENT = (float)playerInfo.at("base_damage_reduction_percent").asDouble();
        if (playerInfo.count("base_crit_avoidance_percent")) cfg::Player::BASE_CRIT_AVOIDANCE_PERCENT = (float)playerInfo.at("base_crit_avoidance_percent").asDouble();
        if (playerInfo.count("base_anti_armor_pen_percent")) cfg::Player::BASE_ANTI_ARMOR_PEN_PERCENT = (float)playerInfo.at("base_anti_armor_pen_percent").asDouble();
        if (playerInfo.count("base_anti_armor_pen_flat")) cfg::Player::BASE_ANTI_ARMOR_PEN_FLAT = playerInfo.at("base_anti_armor_pen_flat").asInt();
        if (playerInfo.count("base_mana_steal_percent")) cfg::Player::BASE_MANA_STEAL_PERCENT = (float)playerInfo.at("base_mana_steal_percent").asDouble();
        if (playerInfo.count("base_xp_bonus_percent")) cfg::Player::BASE_XP_BONUS_PERCENT = (float)playerInfo.at("base_xp_bonus_percent").asDouble();
        if (playerInfo.count("base_cooldown_reduction_percent")) cfg::Player::BASE_COOLDOWN_REDUCTION_PERCENT = (float)playerInfo.at("base_cooldown_reduction_percent").asDouble();
        
        if (playerInfo.count("base_aoe_radius")) cfg::Player::BASE_AOE_RADIUS = (float)playerInfo.at("base_aoe_radius").asDouble();
        if (playerInfo.count("base_aoe_damage_percent")) cfg::Player::BASE_AOE_DAMAGE_PERCENT = (float)playerInfo.at("base_aoe_damage_percent").asDouble();
        
        if (playerInfo.count("base_bleed_duration_flat")) cfg::Player::BASE_BLEED_DURATION_FLAT = (float)playerInfo.at("base_bleed_duration_flat").asDouble();
        if (playerInfo.count("base_bleed_duration_percent")) cfg::Player::BASE_BLEED_DURATION_PERCENT = (float)playerInfo.at("base_bleed_duration_percent").asDouble();
        if (playerInfo.count("base_bleed_flat")) cfg::Player::BASE_BLEED_FLAT = playerInfo.at("base_bleed_flat").asInt();
        if (playerInfo.count("base_bleed_percent")) cfg::Player::BASE_BLEED_PERCENT = (float)playerInfo.at("base_bleed_percent").asDouble();
        
        if (playerInfo.count("base_stun_chance")) cfg::Player::BASE_STUN_CHANCE = (float)playerInfo.at("base_stun_chance").asDouble();
        if (playerInfo.count("base_stun_duration")) cfg::Player::BASE_STUN_DURATION = (float)playerInfo.at("base_stun_duration").asDouble();

        if (playerInfo.count("base_slow_move_percent")) cfg::Player::BASE_SLOW_MOVE_PERCENT = (float)playerInfo.at("base_slow_move_percent").asDouble();
        if (playerInfo.count("base_slow_move_duration")) cfg::Player::BASE_SLOW_MOVE_DURATION = (float)playerInfo.at("base_slow_move_duration").asDouble();
        if (playerInfo.count("base_slow_attack_percent")) cfg::Player::BASE_SLOW_ATTACK_PERCENT = (float)playerInfo.at("base_slow_attack_percent").asDouble();
        if (playerInfo.count("base_slow_attack_duration")) cfg::Player::BASE_SLOW_ATTACK_DURATION = (float)playerInfo.at("base_slow_attack_duration").asDouble();

        if (playerInfo.count("scale_x")) cfg::Player::SCALE_X = (float)playerInfo.at("scale_x").asDouble();
        if (playerInfo.count("scale_y")) cfg::Player::SCALE_Y = (float)playerInfo.at("scale_y").asDouble();
        if (playerInfo.count("auto_fit")) cfg::Player::AUTO_FIT = playerInfo.at("auto_fit").asBool();
        if (playerInfo.count("max_w")) cfg::Player::MAX_W = (float)playerInfo.at("max_w").asDouble();
        if (playerInfo.count("max_h")) cfg::Player::MAX_H = (float)playerInfo.at("max_h").asDouble();
        if (playerInfo.count("weight_kg")) cfg::Weight::PLAYER_DEFAULT_KG = (float)playerInfo.at("weight_kg").asDouble();

        if (playerInfo.count("fx_max_texts")) cfg::Player::FX_MAX_TEXTS = playerInfo.at("fx_max_texts").asInt();
        if (playerInfo.count("gore_max_gibs")) cfg::Player::GORE_MAX_GIBS = playerInfo.at("gore_max_gibs").asInt();
        
        if (playerInfo.count("base_next_level_exp")) cfg::Player::BASE_NEXT_LEVEL_EXP = (long long)playerInfo.at("base_next_level_exp").asDouble();
        if (playerInfo.count("exp_curve_multiplier")) cfg::Player::EXP_CURVE_MULTIPLIER = (float)playerInfo.at("exp_curve_multiplier").asDouble();
        if (playerInfo.count("stat_gain_on_level_up")) cfg::Player::STAT_GAIN_ON_LEVEL_UP = playerInfo.at("stat_gain_on_level_up").asInt();
        
        if (playerInfo.count("feet_width")) cfg::Player::FEET_WIDTH = (float)playerInfo.at("feet_width").asDouble();
        if (playerInfo.count("feet_height")) cfg::Player::FEET_HEIGHT = (float)playerInfo.at("feet_height").asDouble();

        // [WEIGHT SYSTEM]
        if (playerInfo.count("weight_kg")) cfg::Weight::PLAYER_DEFAULT_KG = (float)playerInfo.at("weight_kg").asDouble();

        // [ANIM CONFIG]
        if (playerInfo.count("animConfig")) {
            const auto& animConfig = playerInfo.at("animConfig").asObject();
            if (animConfig.count("groundOffsetY")) {
                cfg::Player::GROUND_OFFSET_Y = (float)animConfig.at("groundOffsetY").asDouble();
            }
            if (animConfig.count("baseAnimSpeed")) {
                cfg::Player::BASE_ANIM_SPEED = (float)animConfig.at("baseAnimSpeed").asDouble();
            }
            if (animConfig.count("headOffset")) {
                const auto& arr = animConfig.at("headOffset").asArray();
                cfg::Player::HEAD_OFFSET = {(float)arr[0].asDouble(), (float)arr[1].asDouble()};
            }
            if (animConfig.count("handLOffset")) {
                const auto& arr = animConfig.at("handLOffset").asArray();
                cfg::Player::HAND_L_OFFSET = {(float)arr[0].asDouble(), (float)arr[1].asDouble()};
            }
            if (animConfig.count("handROffset")) {
                const auto& arr = animConfig.at("handROffset").asArray();
                cfg::Player::HAND_R_OFFSET = {(float)arr[0].asDouble(), (float)arr[1].asDouble()};
            }
            if (animConfig.count("footLOffset")) {
                const auto& arr = animConfig.at("footLOffset").asArray();
                cfg::Player::FOOT_L_OFFSET = {(float)arr[0].asDouble(), (float)arr[1].asDouble()};
            }
            if (animConfig.count("footROffset")) {
                const auto& arr = animConfig.at("footROffset").asArray();
                cfg::Player::FOOT_R_OFFSET = {(float)arr[0].asDouble(), (float)arr[1].asDouble()};
            }
        }
    }

    // --- MAP ---
    if (rootObj.count("map")) {
        const auto& mapInfo = rootObj.at("map").asObject();
        if(mapInfo.count("zoom_factor")) cfg::Map::ZOOM_FACTOR = (float)mapInfo.at("zoom_factor").asDouble();
        if(mapInfo.count("default_zoom")) cfg::Map::DEFAULT_ZOOM = (float)mapInfo.at("default_zoom").asDouble();
        if(mapInfo.count("culling_margin_px")) cfg::Map::CULLING_MARGIN_PX = (float)mapInfo.at("culling_margin_px").asDouble();
        
        if(mapInfo.count("window_width")) cfg::Map::WINDOW_WIDTH = (float)mapInfo.at("window_width").asDouble();
        if(mapInfo.count("window_height")) cfg::Map::WINDOW_HEIGHT = (float)mapInfo.at("window_height").asDouble();
        if(mapInfo.count("window_inner_offset_x")) cfg::Map::WINDOW_INNER_OFFSET_X = (float)mapInfo.at("window_inner_offset_x").asDouble();
        if(mapInfo.count("window_inner_offset_y")) cfg::Map::WINDOW_INNER_OFFSET_Y = (float)mapInfo.at("window_inner_offset_y").asDouble();
        
        if(mapInfo.count("tile_size")) cfg::Map::TILE_SIZE = (unsigned)mapInfo.at("tile_size").asInt();
        if(mapInfo.count("chunk_size")) cfg::Map::CHUNK_SIZE = (unsigned)mapInfo.at("chunk_size").asInt();
        if(mapInfo.count("tileset_margin_px")) cfg::Map::TILESET_MARGIN_PX = (unsigned)mapInfo.at("tileset_margin_px").asInt();
        if(mapInfo.count("tileset_spacing_px")) cfg::Map::TILESET_SPACING_PX = (unsigned)mapInfo.at("tileset_spacing_px").asInt();
        if(mapInfo.count("tileset_smooth")) cfg::Map::TILESET_SMOOTH = mapInfo.at("tileset_smooth").asBool();
        if(mapInfo.count("tex_eps")) cfg::Map::TEX_EPS = (float)mapInfo.at("tex_eps").asDouble();

        if(mapInfo.count("marker_radius")) cfg::Map::MARKER_RADIUS = (float)mapInfo.at("marker_radius").asDouble();
        
        if(mapInfo.count("marker_color")) {
            auto arr = mapInfo.at("marker_color").asArray();
            if(arr.size() >= 3) {
                 cfg::Map::MARKER_COLOR.r = arr[0].asInt();
                 cfg::Map::MARKER_COLOR.g = arr[1].asInt();
                 cfg::Map::MARKER_COLOR.b = arr[2].asInt();
                 if(arr.size() >= 4) cfg::Map::MARKER_COLOR.a = arr[3].asInt();
            }
        }
        if(mapInfo.count("marker_outline_color")) {
            auto arr = mapInfo.at("marker_outline_color").asArray();
            if(arr.size() >= 3) {
                 cfg::Map::MARKER_OUTLINE_COLOR.r = arr[0].asInt();
                 cfg::Map::MARKER_OUTLINE_COLOR.g = arr[1].asInt();
                 cfg::Map::MARKER_OUTLINE_COLOR.b = arr[2].asInt();
                 if(arr.size() >= 4) cfg::Map::MARKER_OUTLINE_COLOR.a = arr[3].asInt();
            }
        }
        if(mapInfo.count("debug_view_map_complete")) cfg::Map::DEBUG_VIEW_MAP_COMPLETE = mapInfo.at("debug_view_map_complete").asBool();
    }

    // --- COMBAT ---
    if (rootObj.count("combat")) {
        const auto& combatInfo = rootObj.at("combat").asObject();
        if(combatInfo.count("player_attack_range_px")) cfg::Combat::PLAYER_ATTACK_RANGE_PX = (float)combatInfo.at("player_attack_range_px").asDouble();
        if(combatInfo.count("defense_constant_base")) cfg::Combat::DEFENSE_CONSTANT_BASE = (float)combatInfo.at("defense_constant_base").asDouble();
        if(combatInfo.count("defense_constant_level_scale")) cfg::Combat::DEFENSE_CONSTANT_LEVEL_SCALE = (float)combatInfo.at("defense_constant_level_scale").asDouble();
        
        if(combatInfo.count("level_diff_damage_penalty")) cfg::Combat::LEVEL_DIFF_DAMAGE_PENALTY = (float)combatInfo.at("level_diff_damage_penalty").asDouble();
        if(combatInfo.count("level_diff_hit_penalty"))    cfg::Combat::LEVEL_DIFF_HIT_PENALTY    = (float)combatInfo.at("level_diff_hit_penalty").asDouble();
        
        if(combatInfo.count("min_level_multiplier")) cfg::Combat::MIN_LEVEL_MULTIPLIER = (float)combatInfo.at("min_level_multiplier").asDouble();
        if(combatInfo.count("max_level_multiplier")) cfg::Combat::MAX_LEVEL_MULTIPLIER = (float)combatInfo.at("max_level_multiplier").asDouble();
        
        if(combatInfo.count("combat_cooldown_hit")) cfg::Combat::COMBAT_COOLDOWN_HIT = (float)combatInfo.at("combat_cooldown_hit").asDouble();
        if(combatInfo.count("combat_cooldown_attack")) cfg::Combat::COMBAT_COOLDOWN_ATTACK = (float)combatInfo.at("combat_cooldown_attack").asDouble();
        if(combatInfo.count("damage_variance_percent")) cfg::Combat::DAMAGE_VARIANCE_PERCENT = combatInfo.at("damage_variance_percent").asInt();
        if(combatInfo.count("player_attack_delay_factor")) cfg::Combat::PLAYER_ATTACK_DELAY_FACTOR = (float)combatInfo.at("player_attack_delay_factor").asDouble();
        if(combatInfo.count("accuracy_evasion_factor")) cfg::Combat::ACCURACY_EVASION_FACTOR = (float)combatInfo.at("accuracy_evasion_factor").asDouble();
        if(combatInfo.count("auto_deselect_margin")) cfg::Combat::AUTO_DESELECT_MARGIN = (float)combatInfo.at("auto_deselect_margin").asDouble();
    }
    
    // --- UI ---
    if (rootObj.count("ui")) {
         const auto& uiInfo = rootObj.at("ui").asObject();
         if(uiInfo.count("cursor_hotspot_x")) cfg::UI::CURSOR_HOTSPOT_X = (float)uiInfo.at("cursor_hotspot_x").asDouble();
         if(uiInfo.count("cursor_hotspot_y")) cfg::UI::CURSOR_HOTSPOT_Y = (float)uiInfo.at("cursor_hotspot_y").asDouble();
         if(uiInfo.count("font_scale")) cfg::UI::FONT_SCALE = (float)uiInfo.at("font_scale").asDouble();
         if(uiInfo.count("damage_number_duration")) cfg::UI::DAMAGE_NUMBER_DURATION = (float)uiInfo.at("damage_number_duration").asDouble();
         if(uiInfo.count("damage_offset_base")) cfg::UI::DAMAGE_OFFSET_BASE = (float)uiInfo.at("damage_offset_base").asDouble();
         if(uiInfo.count("damage_offset_base")) cfg::UI::DAMAGE_OFFSET_BASE = (float)uiInfo.at("damage_offset_base").asDouble();
         if(uiInfo.count("damage_offset_stack")) cfg::UI::DAMAGE_OFFSET_STACK = (float)uiInfo.at("damage_offset_stack").asDouble();
         if(uiInfo.count("damage_scale_bonus_hit")) cfg::UI::DAMAGE_SCALE_BONUS_HIT = (float)uiInfo.at("damage_scale_bonus_hit").asDouble();
         
         if(uiInfo.count("minimap_diameter_default")) cfg::UI::MINIMAP_DIAMETER_DEFAULT = (float)uiInfo.at("minimap_diameter_default").asDouble();
         if(uiInfo.count("minimap_margin_default")) cfg::UI::MINIMAP_MARGIN_DEFAULT = (float)uiInfo.at("minimap_margin_default").asDouble();
         if(uiInfo.count("minimap_width_fraction")) cfg::UI::MINIMAP_WIDTH_FRACTION = (float)uiInfo.at("minimap_width_fraction").asDouble();
         if(uiInfo.count("minimap_margin_fraction")) cfg::UI::MINIMAP_MARGIN_FRACTION = (float)uiInfo.at("minimap_margin_fraction").asDouble();
         if(uiInfo.count("minimap_min_diameter")) cfg::UI::MINIMAP_MIN_DIAMETER = (float)uiInfo.at("minimap_min_diameter").asDouble();
         if(uiInfo.count("minimap_min_margin")) cfg::UI::MINIMAP_MIN_MARGIN = (float)uiInfo.at("minimap_min_margin").asDouble();
         if(uiInfo.count("minimap_update_rate")) cfg::UI::MINIMAP_UPDATE_RATE = (float)uiInfo.at("minimap_update_rate").asDouble();
         if(uiInfo.count("minimap_view_size_tiles")) cfg::UI::MINIMAP_VIEW_SIZE_TILES = (unsigned)uiInfo.at("minimap_view_size_tiles").asInt();
         
         if(uiInfo.count("panel_title_height")) cfg::UI::PANEL_TITLE_HEIGHT = (float)uiInfo.at("panel_title_height").asDouble();

         if(uiInfo.count("fortify_panel")) {
             const auto& fInfo = uiInfo.at("fortify_panel").asObject();
             if(fInfo.count("x")) cfg::UI::FortifyPanel::X = (float)fInfo.at("x").asDouble();
             if(fInfo.count("y")) cfg::UI::FortifyPanel::Y = (float)fInfo.at("y").asDouble();
             if(fInfo.count("slot_offset_x")) cfg::UI::FortifyPanel::SLOT_OFFSET_X = (float)fInfo.at("slot_offset_x").asDouble();
             if(fInfo.count("slot_offset_y")) cfg::UI::FortifyPanel::SLOT_OFFSET_Y = (float)fInfo.at("slot_offset_y").asDouble();
             if(fInfo.count("button_offset_x")) cfg::UI::FortifyPanel::BUTTON_OFFSET_X = (float)fInfo.at("button_offset_x").asDouble();
             if(fInfo.count("button_offset_y")) cfg::UI::FortifyPanel::BUTTON_OFFSET_Y = (float)fInfo.at("button_offset_y").asDouble();
             if(fInfo.count("close_btn_x")) cfg::UI::FortifyPanel::CLOSE_BTN_X = (float)fInfo.at("close_btn_x").asDouble();
             if(fInfo.count("close_btn_y")) cfg::UI::FortifyPanel::CLOSE_BTN_Y = (float)fInfo.at("close_btn_y").asDouble();
             if(fInfo.count("close_btn_size")) cfg::UI::FortifyPanel::CLOSE_BTN_SIZE = (float)fInfo.at("close_btn_size").asDouble();
             if(fInfo.count("loading_bar_offset_x")) cfg::UI::FortifyPanel::LOADING_BAR_OFFSET_X = (float)fInfo.at("loading_bar_offset_x").asDouble();
             if(fInfo.count("loading_bar_offset_y")) cfg::UI::FortifyPanel::LOADING_BAR_OFFSET_Y = (float)fInfo.at("loading_bar_offset_y").asDouble();
             if(fInfo.count("loading_time_seconds")) cfg::UI::FortifyPanel::LOADING_TIME_SECONDS = (float)fInfo.at("loading_time_seconds").asDouble();
         }
         
         if(uiInfo.count("character_panel_width")) cfg::UI::CharacterPanel::WIDTH = (float)uiInfo.at("character_panel_width").asDouble();
         if(uiInfo.count("character_panel_height")) cfg::UI::CharacterPanel::HEIGHT = (float)uiInfo.at("character_panel_height").asDouble();
         if(uiInfo.count("character_panel_margin")) cfg::UI::CharacterPanel::MARGIN = (float)uiInfo.at("character_panel_margin").asDouble();
         if(uiInfo.count("character_panel_font_size")) cfg::UI::CharacterPanel::FONT_SIZE = uiInfo.at("character_panel_font_size").asInt();
         if(uiInfo.count("character_panel_line_spacing")) cfg::UI::CharacterPanel::LINE_SPACING = (float)uiInfo.at("character_panel_line_spacing").asDouble();
         
         if(uiInfo.count("character_panel_equip_offset_x")) cfg::UI::CharacterPanel::EQUIP_GRID_OFFSET_X = (float)uiInfo.at("character_panel_equip_offset_x").asDouble();
         if(uiInfo.count("character_panel_equip_offset_y")) cfg::UI::CharacterPanel::EQUIP_GRID_OFFSET_Y = (float)uiInfo.at("character_panel_equip_offset_y").asDouble();
         
         if(uiInfo.count("character_panel_text_offset_x")) cfg::UI::CharacterPanel::TEXT_OFFSET_X = (float)uiInfo.at("character_panel_text_offset_x").asDouble();
         if(uiInfo.count("character_panel_text_offset_y")) cfg::UI::CharacterPanel::TEXT_OFFSET_Y = (float)uiInfo.at("character_panel_text_offset_y").asDouble();
         if(uiInfo.count("character_panel_title_height")) cfg::UI::CharacterPanel::TITLE_HEIGHT = (float)uiInfo.at("character_panel_title_height").asDouble();
         if(uiInfo.count("character_panel_close_btn_x")) cfg::UI::CharacterPanel::CLOSE_BTN_X = (float)uiInfo.at("character_panel_close_btn_x").asDouble();
         if(uiInfo.count("character_panel_close_btn_y")) cfg::UI::CharacterPanel::CLOSE_BTN_Y = (float)uiInfo.at("character_panel_close_btn_y").asDouble();
         if(uiInfo.count("character_panel_close_btn_size")) cfg::UI::CharacterPanel::CLOSE_BTN_SIZE = (float)uiInfo.at("character_panel_close_btn_size").asDouble();
         
         if(uiInfo.count("unified_slot_size")) cfg::UI::UNIFIED_SLOT_SIZE = (float)uiInfo.at("unified_slot_size").asDouble();
         if(uiInfo.count("base_slot_size")) cfg::UI::BASE_SLOT_SIZE = (float)uiInfo.at("base_slot_size").asDouble();
         if(uiInfo.count("base_icon_size")) cfg::UI::BASE_ICON_SIZE = (float)uiInfo.at("base_icon_size").asDouble();
         if(uiInfo.count("unified_slot_margin")) cfg::UI::UNIFIED_SLOT_MARGIN = (float)uiInfo.at("unified_slot_margin").asDouble();
         if(uiInfo.count("common_margin")) cfg::UI::COMMON_MARGIN = (float)uiInfo.at("common_margin").asDouble();
         
         if(uiInfo.count("exp_bar_height")) cfg::UI::EXP_BAR_HEIGHT = (float)uiInfo.at("exp_bar_height").asDouble();
         if(uiInfo.count("exp_bar_bottom_offset")) cfg::UI::EXP_BAR_BOTTOM_OFFSET = (float)uiInfo.at("exp_bar_bottom_offset").asDouble();

         if(uiInfo.count("hud_bg_offset_y")) cfg::UI::HUD_BG_OFFSET_Y = (float)uiInfo.at("hud_bg_offset_y").asDouble();
         if(uiInfo.count("hud_slots_offset_x")) cfg::UI::HUD_SLOTS_OFFSET_X = (float)uiInfo.at("hud_slots_offset_x").asDouble();
         if(uiInfo.count("hud_slots_offset_y")) cfg::UI::HUD_SLOTS_OFFSET_Y = (float)uiInfo.at("hud_slots_offset_y").asDouble();

         if(uiInfo.count("hotbar_slots")) cfg::UI::HOTBAR_SLOTS = uiInfo.at("hotbar_slots").asInt();
         if(uiInfo.count("slot_margin")) cfg::UI::SLOT_MARGIN = (float)uiInfo.at("slot_margin").asDouble();
         if(uiInfo.count("hud_slot_height_percent")) cfg::UI::HUD_SLOT_HEIGHT_PERCENT = (float)uiInfo.at("hud_slot_height_percent").asDouble();
         if(uiInfo.count("hud_slot_min_size")) cfg::UI::HUD_SLOT_MIN_SIZE = (float)uiInfo.at("hud_slot_min_size").asDouble();
         if(uiInfo.count("hud_slot_max_size")) cfg::UI::HUD_SLOT_MAX_SIZE = (float)uiInfo.at("hud_slot_max_size").asDouble();
         if(uiInfo.count("combat_status_y_percent")) cfg::UI::COMBAT_STATUS_Y_PERCENT = (float)uiInfo.at("combat_status_y_percent").asDouble();
         
         if(uiInfo.count("skill_icon_offset_x")) cfg::UI::SKILL_ICON_OFFSET_X = (float)uiInfo.at("skill_icon_offset_x").asDouble();
         if(uiInfo.count("skill_icon_offset_y")) cfg::UI::SKILL_ICON_OFFSET_Y = (float)uiInfo.at("skill_icon_offset_y").asDouble();

         if(uiInfo.count("inventory_cols")) cfg::UI::Inventory::COLS = uiInfo.at("inventory_cols").asInt();
         if(uiInfo.count("inventory_rows")) cfg::UI::Inventory::ROWS = uiInfo.at("inventory_rows").asInt();
         if(uiInfo.count("inventory_grid_offset_x")) cfg::UI::Inventory::GRID_OFFSET_X = (float)uiInfo.at("inventory_grid_offset_x").asDouble();
         if(uiInfo.count("inventory_grid_offset_y")) cfg::UI::Inventory::GRID_OFFSET_Y = (float)uiInfo.at("inventory_grid_offset_y").asDouble();
         if(uiInfo.count("inventory_close_btn_x")) cfg::UI::Inventory::CLOSE_BTN_X = (float)uiInfo.at("inventory_close_btn_x").asDouble();
         if(uiInfo.count("inventory_close_btn_y")) cfg::UI::Inventory::CLOSE_BTN_Y = (float)uiInfo.at("inventory_close_btn_y").asDouble();
         if(uiInfo.count("inventory_close_btn_size")) cfg::UI::Inventory::CLOSE_BTN_SIZE = (float)uiInfo.at("inventory_close_btn_size").asDouble();
         if(uiInfo.count("inventory_cols") && uiInfo.count("inventory_rows")) {
              cfg::UI::Inventory::TOTAL_SLOTS = cfg::UI::Inventory::COLS * cfg::UI::Inventory::ROWS;
         }
         
         if(uiInfo.count("target_frame_portrait_size")) cfg::UI::TargetFrame::PORTRAIT_SIZE = (float)uiInfo.at("target_frame_portrait_size").asDouble();
         if(uiInfo.count("target_frame_portrait_view_size")) cfg::UI::TargetFrame::PORTRAIT_VIEW_SIZE = (float)uiInfo.at("target_frame_portrait_view_size").asDouble();
         if(uiInfo.count("target_frame_portrait_view_offset_y")) cfg::UI::TargetFrame::PORTRAIT_VIEW_OFFSET_Y = (float)uiInfo.at("target_frame_portrait_view_offset_y").asDouble();
         
         if(uiInfo.count("player_frame_portrait_view_size")) cfg::UI::PlayerFrame::PORTRAIT_VIEW_SIZE = (float)uiInfo.at("player_frame_portrait_view_size").asDouble();
         if(uiInfo.count("player_frame_portrait_view_offset_y")) cfg::UI::PlayerFrame::PORTRAIT_VIEW_OFFSET_Y = (float)uiInfo.at("player_frame_portrait_view_offset_y").asDouble();
         if(uiInfo.count("player_frame_portrait_offset_x")) cfg::UI::PlayerFrame::PORTRAIT_OFFSET_X = (float)uiInfo.at("player_frame_portrait_offset_x").asDouble();
         if(uiInfo.count("player_frame_portrait_offset_y")) cfg::UI::PlayerFrame::PORTRAIT_OFFSET_Y = (float)uiInfo.at("player_frame_portrait_offset_y").asDouble();
         if(uiInfo.count("player_frame_text_block_offset_x")) cfg::UI::PlayerFrame::TEXT_BLOCK_OFFSET_X = (float)uiInfo.at("player_frame_text_block_offset_x").asDouble();
         if(uiInfo.count("player_frame_text_block_offset_y")) cfg::UI::PlayerFrame::TEXT_BLOCK_OFFSET_Y = (float)uiInfo.at("player_frame_text_block_offset_y").asDouble();
         if(uiInfo.count("player_frame_hp_offset_y")) cfg::UI::PlayerFrame::HP_BAR_OFFSET_Y = (float)uiInfo.at("player_frame_hp_offset_y").asDouble();
         if(uiInfo.count("player_frame_mp_spacing")) cfg::UI::PlayerFrame::MP_BAR_SPACING = (float)uiInfo.at("player_frame_mp_spacing").asDouble();

         if(uiInfo.count("player_frame_hp_bar_x")) cfg::UI::PlayerFrame::HP_BAR_X = (float)uiInfo.at("player_frame_hp_bar_x").asDouble();
         if(uiInfo.count("player_frame_hp_bar_y")) cfg::UI::PlayerFrame::HP_BAR_Y = (float)uiInfo.at("player_frame_hp_bar_y").asDouble();
         if(uiInfo.count("player_frame_mp_bar_x")) cfg::UI::PlayerFrame::MP_BAR_X = (float)uiInfo.at("player_frame_mp_bar_x").asDouble();
         if(uiInfo.count("player_frame_mp_bar_y")) cfg::UI::PlayerFrame::MP_BAR_Y = (float)uiInfo.at("player_frame_mp_bar_y").asDouble();

         if(uiInfo.count("target_frame_hp_bar_x")) cfg::UI::TargetFrame::HP_BAR_X = (float)uiInfo.at("target_frame_hp_bar_x").asDouble();
         if(uiInfo.count("target_frame_hp_bar_y")) cfg::UI::TargetFrame::HP_BAR_Y = (float)uiInfo.at("target_frame_hp_bar_y").asDouble();
         if(uiInfo.count("target_frame_mp_bar_x")) cfg::UI::TargetFrame::MP_BAR_X = (float)uiInfo.at("target_frame_mp_bar_x").asDouble();
         if(uiInfo.count("target_frame_mp_bar_y")) cfg::UI::TargetFrame::MP_BAR_Y = (float)uiInfo.at("target_frame_mp_bar_y").asDouble();

         if(uiInfo.count("target_frame_margin")) cfg::UI::TargetFrame::MARGIN = (float)uiInfo.at("target_frame_margin").asDouble();
         if(uiInfo.count("target_frame_bar_width")) cfg::UI::TargetFrame::BAR_WIDTH = (float)uiInfo.at("target_frame_bar_width").asDouble();
         if(uiInfo.count("target_frame_bar_height")) cfg::UI::TargetFrame::BAR_HEIGHT = (float)uiInfo.at("target_frame_bar_height").asDouble();
         
         // [NEW] Text Offsets
         if(uiInfo.count("target_frame_name_offset_y")) cfg::UI::TargetFrame::NAME_OFFSET_Y = (float)uiInfo.at("target_frame_name_offset_y").asDouble();
         if(uiInfo.count("target_frame_label_offset_x")) cfg::UI::TargetFrame::LABEL_OFFSET_X = (float)uiInfo.at("target_frame_label_offset_x").asDouble();
         if(uiInfo.count("target_frame_label_offset_y")) cfg::UI::TargetFrame::LABEL_OFFSET_Y = (float)uiInfo.at("target_frame_label_offset_y").asDouble();
         if(uiInfo.count("target_frame_value_offset_y")) cfg::UI::TargetFrame::VALUE_OFFSET_Y = (float)uiInfo.at("target_frame_value_offset_y").asDouble();
         if(uiInfo.count("target_frame_portrait_offset_x")) cfg::UI::TargetFrame::PORTRAIT_OFFSET_X = (float)uiInfo.at("target_frame_portrait_offset_x").asDouble();
         if(uiInfo.count("target_frame_portrait_offset_y")) cfg::UI::TargetFrame::PORTRAIT_OFFSET_Y = (float)uiInfo.at("target_frame_portrait_offset_y").asDouble();
         if(uiInfo.count("target_frame_text_block_offset_x")) cfg::UI::TargetFrame::TEXT_BLOCK_OFFSET_X = (float)uiInfo.at("target_frame_text_block_offset_x").asDouble();
         if(uiInfo.count("target_frame_text_block_offset_y")) cfg::UI::TargetFrame::TEXT_BLOCK_OFFSET_Y = (float)uiInfo.at("target_frame_text_block_offset_y").asDouble();
         
         if(uiInfo.count("tooltip_border_size")) cfg::UI::Tooltip::BORDER_SIZE = (float)uiInfo.at("tooltip_border_size").asDouble();
         if(uiInfo.count("tooltip_border_color")) {
             auto arr = uiInfo.at("tooltip_border_color").asArray();
             if(arr.size() >= 3) {
                 cfg::UI::Tooltip::BORDER_COLOR.r = arr[0].asInt();
                 cfg::UI::Tooltip::BORDER_COLOR.g = arr[1].asInt();
                 cfg::UI::Tooltip::BORDER_COLOR.b = arr[2].asInt();
                 if(arr.size() >= 4) cfg::UI::Tooltip::BORDER_COLOR.a = arr[3].asInt();
             }
         }
         if(uiInfo.count("tooltip_bg_color")) {
             auto arr = uiInfo.at("tooltip_bg_color").asArray();
             if(arr.size() >= 3) {
                 cfg::UI::Tooltip::BG_COLOR.r = arr[0].asInt();
                 cfg::UI::Tooltip::BG_COLOR.g = arr[1].asInt();
                 cfg::UI::Tooltip::BG_COLOR.b = arr[2].asInt();
                 if(arr.size() >= 4) cfg::UI::Tooltip::BG_COLOR.a = arr[3].asInt();
             }
         }
    }
    
    // --- CHAT ---
    if (rootObj.count("ui") && rootObj.at("ui").asObject().count("chat")) {
         const auto& chatInfo = rootObj.at("ui").asObject().at("chat").asObject();
         if(chatInfo.count("fallback_width")) cfg::UI::Chat::FALLBACK_WIDTH = (float)chatInfo.at("fallback_width").asDouble();
         if(chatInfo.count("fallback_height")) cfg::UI::Chat::FALLBACK_HEIGHT = (float)chatInfo.at("fallback_height").asDouble();
         if(chatInfo.count("margin_left")) cfg::UI::Chat::MARGIN_LEFT = (float)chatInfo.at("margin_left").asDouble();
         if(chatInfo.count("margin_bottom")) cfg::UI::Chat::MARGIN_BOTTOM = (float)chatInfo.at("margin_bottom").asDouble();
         if(chatInfo.count("padding_left")) cfg::UI::Chat::PADDING_LEFT = (float)chatInfo.at("padding_left").asDouble();
         if(chatInfo.count("padding_right")) cfg::UI::Chat::PADDING_RIGHT = (float)chatInfo.at("padding_right").asDouble();
         if(chatInfo.count("padding_top")) cfg::UI::Chat::PADDING_TOP = (float)chatInfo.at("padding_top").asDouble();
         if(chatInfo.count("padding_bottom")) cfg::UI::Chat::PADDING_BOTTOM = (float)chatInfo.at("padding_bottom").asDouble();
    }
    
     if (rootObj.count("ui") && rootObj.at("ui").asObject().count("floating_text")) {
          const auto& ftInfo = rootObj.at("ui").asObject().at("floating_text").asObject();
          if(ftInfo.count("size_normal")) cfg::UI::FloatingText::SIZE_NORMAL = ftInfo.at("size_normal").asInt();
          if(ftInfo.count("size_crit")) cfg::UI::FloatingText::SIZE_CRIT = ftInfo.at("size_crit").asInt();
          if(ftInfo.count("velocity_y_normal")) cfg::UI::FloatingText::VELOCITY_Y_NORMAL = (float)ftInfo.at("velocity_y_normal").asDouble();
          if(ftInfo.count("lifetime_normal")) cfg::UI::FloatingText::LIFETIME_NORMAL = (float)ftInfo.at("lifetime_normal").asDouble();
     }

     if (rootObj.count("ui") && rootObj.at("ui").asObject().count("floating_text_offsets")) {
          const auto& offInfo = rootObj.at("ui").asObject().at("floating_text_offsets").asObject();
          if(offInfo.count("damage")) cfg::UI::FloatingText::OFFSET_Y_DAMAGE = (float)offInfo.at("damage").asDouble();
          if(offInfo.count("true_extra")) cfg::UI::FloatingText::OFFSET_Y_TRUE_EXTRA = (float)offInfo.at("true_extra").asDouble();
          if(offInfo.count("heal")) cfg::UI::FloatingText::OFFSET_Y_HEAL = (float)offInfo.at("heal").asDouble();
          if(offInfo.count("miss")) cfg::UI::FloatingText::OFFSET_Y_MISS = (float)offInfo.at("miss").asDouble();
          if(offInfo.count("xp")) cfg::UI::FloatingText::OFFSET_Y_XP = (float)offInfo.at("xp").asDouble();
     }
    
    if (rootObj.count("ui") && rootObj.at("ui").asObject().count("inventory_icon_scale")) {
         cfg::UI::Inventory::ICON_SCALE = (float)rootObj.at("ui").asObject().at("inventory_icon_scale").asDouble();
    }
    
    // --- MOB ---
    if (rootObj.count("mob")) {
        const auto& mInfo = rootObj.at("mob").asObject();
        if(mInfo.count("patrol_radius")) cfg::Mob::PATROL_RADIUS = (float)mInfo.at("patrol_radius").asDouble();
        if(mInfo.count("patrol_radius")) cfg::Mob::PATROL_RADIUS_SQ = cfg::Mob::PATROL_RADIUS * cfg::Mob::PATROL_RADIUS; 

        if(mInfo.count("leash_time")) cfg::Mob::LEASH_TIME = (float)mInfo.at("leash_time").asDouble();
        if(mInfo.count("base_range")) cfg::Mob::BASE_RANGE = (float)mInfo.at("base_range").asDouble();
        if(mInfo.count("idle_time_min")) cfg::Mob::IDLE_TIME_MIN = (float)mInfo.at("idle_time_min").asDouble();
        if(mInfo.count("idle_time_max")) cfg::Mob::IDLE_TIME_MAX = (float)mInfo.at("idle_time_max").asDouble();
        if(mInfo.count("respawn_time")) cfg::Mob::RESPAWN_TIME = (float)mInfo.at("respawn_time").asDouble();
        if(mInfo.count("respawn_jitter")) cfg::Mob::RESPAWN_JITTER = (float)mInfo.at("respawn_jitter").asDouble();
        if(mInfo.count("fade_in_duration")) cfg::Mob::FADE_IN_DURATION = (float)mInfo.at("fade_in_duration").asDouble();
        if(mInfo.count("knockback_dist")) cfg::Mob::HIT_KNOCKBACK_DIST = (float)mInfo.at("knockback_dist").asDouble();
    }

    // --- WORLD ---
    if (rootObj.count("world")) {
        const auto& wInfo = rootObj.at("world").asObject();
        if(wInfo.count("bounds_margin")) cfg::World::BOUNDS_MARGIN = (float)wInfo.at("bounds_margin").asDouble();
    }

    // --- DEBUG ---
    if (rootObj.count("debug")) {
        const auto& dInfo = rootObj.at("debug").asObject();
        if(dInfo.count("xp_gain")) cfg::Debug::XP_GAIN = dInfo.at("xp_gain").asInt();
        if(dInfo.count("stat_boost")) cfg::Debug::STAT_BOOST = dInfo.at("stat_boost").asInt();
        if(dInfo.count("enable_weapons_debug")) cfg::Debug::ENABLE_WEAPONS_DEBUG = dInfo.at("enable_weapons_debug").asBool();
        if(dInfo.count("enable_perf_log")) cfg::Debug::ENABLE_PERF_LOG = dInfo.at("enable_perf_log").asBool(); // [NEW]
        if(dInfo.count("enable_perf_chat")) cfg::Debug::ENABLE_PERF_CHAT = dInfo.at("enable_perf_chat").asBool(); // [NEW]
        if(dInfo.count("enable_culling_debug")) cfg::Debug::ENABLE_CULLING_DEBUG = dInfo.at("enable_culling_debug").asBool();
        if(dInfo.count("culling_debug_margin")) cfg::Debug::CULLING_DEBUG_MARGIN = (float)dInfo.at("culling_debug_margin").asDouble();
        if(dInfo.count("enable_floating_text")) cfg::Debug::ENABLE_FLOATING_TEXT = dInfo.at("enable_floating_text").asBool();
        if(dInfo.count("enable_combat_particles")) cfg::Debug::ENABLE_COMBAT_PARTICLES = dInfo.at("enable_combat_particles").asBool();
        if(dInfo.count("enable_debug_overlay")) cfg::Debug::ENABLE_DEBUG_OVERLAY = dInfo.at("enable_debug_overlay").asBool();
        if(dInfo.count("show_occlusion_green")) cfg::Debug::SHOW_OCCLUSION_GREEN = dInfo.at("show_occlusion_green").asBool();
        if(dInfo.count("show_part_sorting_points")) cfg::Debug::SHOW_PART_SORTING_POINTS = dInfo.at("show_part_sorting_points").asBool();
        if(dInfo.count("enable_tracy")) cfg::Debug::ENABLE_TRACY = dInfo.at("enable_tracy").asBool();
        if(dInfo.count("enable_char_panel_slots_debug")) cfg::Debug::ENABLE_CHAR_PANEL_SLOTS_DEBUG = dInfo.at("enable_char_panel_slots_debug").asBool();
        if(dInfo.count("enable_char_panel-slots_debug")) cfg::Debug::ENABLE_CHAR_PANEL_SLOTS_DEBUG = dInfo.at("enable_char_panel-slots_debug").asBool();
    }

    // --- SHADOW & CONTOUR ---
    if (rootObj.count("shadow")) {
        const auto& sInfo = rootObj.at("shadow").asObject();
        float baseScaleY = 0.3f;
        if(sInfo.count("scale_y")) baseScaleY = (float)sInfo.at("scale_y").asDouble();
        cfg::Shadow::SCALE_Y = baseScaleY;

        if(sInfo.count("alpha")) cfg::Shadow::ALPHA = (float)sInfo.at("alpha").asDouble();
        if(sInfo.count("offset_x")) cfg::Shadow::OFFSET_X = (float)sInfo.at("offset_x").asDouble();
        if(sInfo.count("offset_y")) cfg::Shadow::OFFSET_Y = (float)sInfo.at("offset_y").asDouble();
        if(sInfo.count("scale_x")) cfg::Shadow::SCALE_X = (float)sInfo.at("scale_x").asDouble();
        if(sInfo.count("skew_x")) cfg::Shadow::SKEW_X = (float)sInfo.at("skew_x").asDouble();

        if(sInfo.count("sun_angle")) {
            float sunAngle = (float)sInfo.at("sun_angle").asDouble();
            cfg::Shadow::SUN_ANGLE = sunAngle;
            if (sunAngle > 75.f) sunAngle = 75.f;
            if (sunAngle < -75.f) sunAngle = -75.f;
            float angleRad = sunAngle * 3.14159265f / 180.f;
            cfg::Shadow::SKEW_X = -std::tan(angleRad);
            cfg::Shadow::SCALE_Y = baseScaleY / std::cos(angleRad);
        }

        if(sInfo.count("enable_contour")) cfg::Shadow::ENABLE_CONTOUR = sInfo.at("enable_contour").asBool();
        if(sInfo.count("contour_thickness")) cfg::Shadow::CONTOUR_THICKNESS = (float)sInfo.at("contour_thickness").asDouble();
        if(sInfo.count("sobel_step")) cfg::Shadow::SOBEL_STEP = (float)sInfo.at("sobel_step").asDouble();
        if(sInfo.count("contour_alpha")) cfg::Shadow::CONTOUR_ALPHA = (float)sInfo.at("contour_alpha").asDouble();
        if(sInfo.count("contour_base_alpha")) cfg::Shadow::CONTOUR_BASE_ALPHA = (float)sInfo.at("contour_base_alpha").asDouble();
        if(sInfo.count("contour_color_r")) cfg::Shadow::CONTOUR_COLOR_R = sInfo.at("contour_color_r").asInt();
        if(sInfo.count("contour_color_g")) cfg::Shadow::CONTOUR_COLOR_G = sInfo.at("contour_color_g").asInt();
        if(sInfo.count("contour_color_b")) cfg::Shadow::CONTOUR_COLOR_B = sInfo.at("contour_color_b").asInt();
    }

    // --- WIND ---
    if (rootObj.count("wind")) {
        const auto& wInfo = rootObj.at("wind").asObject();
        if(wInfo.count("enable")) cfg::Wind::ENABLE = wInfo.at("enable").asBool();
        if(wInfo.count("speed")) cfg::Wind::SPEED = (float)wInfo.at("speed").asDouble();
        if(wInfo.count("strength")) cfg::Wind::STRENGTH = (float)wInfo.at("strength").asDouble();
        if(wInfo.count("frequency")) cfg::Wind::FREQUENCY = (float)wInfo.at("frequency").asDouble();
        if(wInfo.count("turbulence")) cfg::Wind::TURBULENCE = (float)wInfo.at("turbulence").asDouble();
        if(wInfo.count("gust_strength")) cfg::Wind::GUST_STRENGTH = (float)wInfo.at("gust_strength").asDouble();
        if(wInfo.count("gust_frequency")) cfg::Wind::GUST_FREQUENCY = (float)wInfo.at("gust_frequency").asDouble();
    }

    // --- DECOR ---
    if (rootObj.count("decor")) {
        const auto& decInfo = rootObj.at("decor").asObject();
        if(decInfo.count("scale_small_plant")) cfg::Decor::SCALE_SMALL_PLANT = (float)decInfo.at("scale_small_plant").asDouble();
        if(decInfo.count("scale_tree")) cfg::Decor::SCALE_TREE = (float)decInfo.at("scale_tree").asDouble();
        if(decInfo.count("scale_default")) cfg::Decor::SCALE_DEFAULT = (float)decInfo.at("scale_default").asDouble();
        if(decInfo.count("grid_cell_size")) cfg::Decor::GRID_CELL_SIZE = decInfo.at("grid_cell_size").asInt();
        
        if(decInfo.count("trunk_width")) cfg::Decor::TRUNK_WIDTH = (float)decInfo.at("trunk_width").asDouble();
        if(decInfo.count("trunk_height")) cfg::Decor::TRUNK_HEIGHT = (float)decInfo.at("trunk_height").asDouble();
    }
    
    // --- OPTIMIZATION ---
    if (rootObj.count("optimization")) {
        const auto& optInfo = rootObj.at("optimization").asObject();
        if (optInfo.count("wake_up_check_interval")) cfg::Optimization::WAKE_UP_CHECK_INTERVAL = (float)optInfo.at("wake_up_check_interval").asDouble();
        if (optInfo.count("max_respawns_per_frame")) cfg::Optimization::MAX_RESPAWNS_PER_FRAME = optInfo.at("max_respawns_per_frame").asInt();
        if (optInfo.count("wake_up_margin_px")) cfg::Optimization::WAKE_UP_MARGIN_PX = (float)optInfo.at("wake_up_margin_px").asDouble();
        if (optInfo.count("sleep_hysteresis_px")) cfg::Optimization::SLEEP_HYSTERESIS_PX = (float)optInfo.at("sleep_hysteresis_px").asDouble();
    }
    
    // --- RESOURCES ---
    if (rootObj.count("resources")) {
        const auto& resInfo = rootObj.at("resources").asObject();
        if (resInfo.count("error_texture_size")) cfg::Resources::ERROR_TEXTURE_SIZE = (unsigned)resInfo.at("error_texture_size").asInt();
    }

    // --- STUN ---
    if (rootObj.count("stun")) {
         const auto& sInfo = rootObj.at("stun").asObject();
         if (sInfo.count("halo_radius")) cfg::Player::STUN_HALO_RADIUS = (float)sInfo.at("halo_radius").asDouble();
         if (sInfo.count("particle_size")) cfg::Player::STUN_PARTICLE_SIZE = (float)sInfo.at("particle_size").asDouble();
    }

    // --- TERRAIN ---
    if (rootObj.count("terrain")) {
        const auto& tInfo = rootObj.at("terrain").asObject();
        if (tInfo.count("enable_terrain_deform"))
            cfg::Terrain::ENABLE_TERRAIN_DEFORM = tInfo.at("enable_terrain_deform").asBool();
        if (tInfo.count("footprint_lift_threshold"))
            cfg::Terrain::FOOTPRINT_LIFT_THRESHOLD = (float)tInfo.at("footprint_lift_threshold").asDouble();
        if (tInfo.count("dirt_offset_px"))
            cfg::Terrain::DIRT_OFFSET_PX = (float)tInfo.at("dirt_offset_px").asDouble();
        if (tInfo.count("explosion_offset_px"))
            cfg::Terrain::EXPLOSION_OFFSET_PX = (float)tInfo.at("explosion_offset_px").asDouble();
        if (tInfo.count("occlusion_projection_px"))
            cfg::Terrain::OCCLUSION_PROJECTION_PX = (float)tInfo.at("occlusion_projection_px").asDouble();
        if (tInfo.count("dirt_regen_time_sec"))
            cfg::Terrain::DIRT_REGEN_TIME_SEC = (float)tInfo.at("dirt_regen_time_sec").asDouble();
        if (tInfo.count("grass_regen_time_sec"))
            cfg::Terrain::GRASS_REGEN_TIME_SEC = (float)tInfo.at("grass_regen_time_sec").asDouble();
        if (tInfo.count("debug_paint_active_chunks"))
            cfg::Terrain::DEBUG_PAINT_ACTIVE_CHUNKS = tInfo.at("debug_paint_active_chunks").asBool();
    }

    // --- ITEM DROP ---
    if (rootObj.count("item_drop")) {
        const auto& dInfo = rootObj.at("item_drop").asObject();
        if (dInfo.count("float_speed"))
            cfg::ItemDrop::FLOAT_SPEED = (float)dInfo.at("float_speed").asDouble();
        if (dInfo.count("float_amplitude"))
            cfg::ItemDrop::FLOAT_AMPLITUDE = (float)dInfo.at("float_amplitude").asDouble();
        if (dInfo.count("float_offset_y"))
            cfg::ItemDrop::FLOAT_OFFSET_Y = (float)dInfo.at("float_offset_y").asDouble();
    }

    // --- FX ---
    if (rootObj.count("fx")) {
        const auto& fxInfo = rootObj.at("fx").asObject();
        if (fxInfo.count("hit_ring_alpha"))
            cfg::FX::HIT_RING_ALPHA = (float)fxInfo.at("hit_ring_alpha").asDouble();
    }

    // --- GORE ---
    if (rootObj.count("gore")) {
        const auto& gInfo = rootObj.at("gore").asObject();
        if (gInfo.count("gravity")) cfg::Gore::GRAVITY = (float)gInfo.at("gravity").asDouble();
        if (gInfo.count("ground_spread_min")) cfg::Gore::GROUND_SPREAD_MIN = (float)gInfo.at("ground_spread_min").asDouble();
        if (gInfo.count("ground_spread_max")) cfg::Gore::GROUND_SPREAD_MAX = (float)gInfo.at("ground_spread_max").asDouble();
        if (gInfo.count("upward_boost_base")) cfg::Gore::UPWARD_BOOST_BASE = (float)gInfo.at("upward_boost_base").asDouble();
        if (gInfo.count("height_multiplier")) cfg::Gore::HEIGHT_MULTIPLIER = (float)gInfo.at("height_multiplier").asDouble();
        if (gInfo.count("h_speed_min")) cfg::Gore::H_SPEED_MIN = (float)gInfo.at("h_speed_min").asDouble();
        if (gInfo.count("h_speed_max")) cfg::Gore::H_SPEED_MAX = (float)gInfo.at("h_speed_max").asDouble();
        if (gInfo.count("angular_vel_min")) cfg::Gore::ANGULAR_VEL_MIN = (float)gInfo.at("angular_vel_min").asDouble();
        if (gInfo.count("angular_vel_max")) cfg::Gore::ANGULAR_VEL_MAX = (float)gInfo.at("angular_vel_max").asDouble();
        if (gInfo.count("lifetime_min")) cfg::Gore::LIFETIME_MIN = (float)gInfo.at("lifetime_min").asDouble();
        if (gInfo.count("lifetime_max")) cfg::Gore::LIFETIME_MAX = (float)gInfo.at("lifetime_max").asDouble();
        if (gInfo.count("fade_duration")) cfg::Gore::FADE_DURATION = (float)gInfo.at("fade_duration").asDouble();
        if (gInfo.count("restitution")) cfg::Gore::RESTITUTION = (float)gInfo.at("restitution").asDouble();
        if (gInfo.count("friction")) cfg::Gore::FRICTION = (float)gInfo.at("friction").asDouble();
        if (gInfo.count("min_bounce_velocity")) cfg::Gore::MIN_BOUNCE_VELOCITY = (float)gInfo.at("min_bounce_velocity").asDouble();
        if (gInfo.count("constraint_max_dist_factor")) cfg::Gore::CONSTRAINT_MAX_DIST_FACTOR = (float)gInfo.at("constraint_max_dist_factor").asDouble();
        if (gInfo.count("spring_pull_factor")) cfg::Gore::SPRING_PULL_FACTOR = (float)gInfo.at("spring_pull_factor").asDouble();
        if (gInfo.count("loot_armor_attached")) cfg::Gore::LOOT_ARMOR_ATTACHED = gInfo.at("loot_armor_attached").asBool();
        if (gInfo.count("enable_bone_decay")) cfg::Gore::ENABLE_BONE_DECAY = gInfo.at("enable_bone_decay").asBool();
        if (gInfo.count("decay_delay_sec")) cfg::Gore::DECAY_DELAY_SEC = (float)gInfo.at("decay_delay_sec").asDouble();
        if (gInfo.count("decay_fade_duration")) cfg::Gore::DECAY_FADE_DURATION = (float)gInfo.at("decay_fade_duration").asDouble();
        if (gInfo.count("bone_lifetime_sec")) cfg::Gore::BONE_LIFETIME_SEC = (float)gInfo.at("bone_lifetime_sec").asDouble();
        if (gInfo.count("bone_fade_duration")) cfg::Gore::BONE_FADE_DURATION = (float)gInfo.at("bone_fade_duration").asDouble();
        if (gInfo.count("sink_distance")) cfg::Gore::SINK_DISTANCE = (float)gInfo.at("sink_distance").asDouble();
        if (gInfo.count("clip_offset_y")) cfg::Gore::CLIP_OFFSET_Y = (float)gInfo.at("clip_offset_y").asDouble();
    }
    
    // --- COMBAT ---
    if (rootObj.count("combat")) {
        const auto& cInfo = rootObj.at("combat").asObject();
        if (cInfo.count("knockback_weight_factor")) cfg::Combat::KNOCKBACK_WEIGHT_FACTOR = (float)cInfo.at("knockback_weight_factor").asDouble();
        if (cInfo.count("knockback_strength_factor")) cfg::Combat::KNOCKBACK_STRENGTH_FACTOR = (float)cInfo.at("knockback_strength_factor").asDouble();
    }

    // --- WORLD ---
    if (rootObj.count("world")) {
        const auto& wInfo = rootObj.at("world").asObject();
        if (wInfo.count("initial_world")) cfg::World::INITIAL_WORLD = wInfo.at("initial_world").asString();
        if (wInfo.count("initial_spawn_x")) cfg::World::INITIAL_SPAWN_X = (float)wInfo.at("initial_spawn_x").asDouble();
        if (wInfo.count("initial_spawn_y")) cfg::World::INITIAL_SPAWN_Y = (float)wInfo.at("initial_spawn_y").asDouble();
    }

    // --- AUDIO ---
    if (rootObj.count("audio")) {
        const auto& audioInfo = rootObj.at("audio").asObject();
        if (audioInfo.count("max_sounds"))
            cfg::Audio::MAX_SOUNDS = audioInfo.at("max_sounds").asInt();
        if (audioInfo.count("footstep_volume"))
            cfg::Audio::FOOTSTEP_VOLUME = (float)audioInfo.at("footstep_volume").asDouble();
        if (audioInfo.count("mob_footstep_max_distance"))
            cfg::Audio::MOB_FOOTSTEP_MAX_DISTANCE = (float)audioInfo.at("mob_footstep_max_distance").asDouble();
    }

    // --- Y-SORTING ---
    if (rootObj.count("y_sorting_offsets")) {
        const auto& yInfo = rootObj.at("y_sorting_offsets").asObject();
        if (yInfo.count("player")) cfg::YSorting::PLAYER = (float)yInfo.at("player").asDouble();
        if (yInfo.count("portal")) cfg::YSorting::PORTAL = (float)yInfo.at("portal").asDouble();
        if (yInfo.count("decor_tree")) cfg::YSorting::DECOR_TREE = (float)yInfo.at("decor_tree").asDouble();
        if (yInfo.count("item_drop")) cfg::YSorting::ITEM_DROP = (float)yInfo.at("item_drop").asDouble();
    }

    // --- WEIGHT SYSTEM ---
    if (rootObj.count("weight")) {
        const auto& wInfo = rootObj.at("weight").asObject();
        if (wInfo.count("threshold_kg"))
            cfg::Weight::THRESHOLD_KG = (float)wInfo.at("threshold_kg").asDouble();
        if (wInfo.count("divisor"))
            cfg::Weight::DIVISOR = (float)wInfo.at("divisor").asDouble();
    }

    // --- POST FX ---
    if (rootObj.count("post_fx")) {
        const auto& pfx = rootObj.at("post_fx").asObject();
        if (pfx.count("enabled"))           cfg::PostFX::ENABLED          = pfx.at("enabled").asBool();
        if (pfx.count("dither_strength"))   cfg::PostFX::DITHER_STRENGTH  = (float)pfx.at("dither_strength").asDouble();
        if (pfx.count("grain_strength"))    cfg::PostFX::GRAIN_STRENGTH   = (float)pfx.at("grain_strength").asDouble();
        if (pfx.count("grid_strength"))     cfg::PostFX::GRID_STRENGTH    = (float)pfx.at("grid_strength").asDouble();
        if (pfx.count("vignette_strength")) cfg::PostFX::VIGNETTE_STRENGTH = (float)pfx.at("vignette_strength").asDouble();
        if (pfx.count("palette_enabled"))   cfg::PostFX::PALETTE_ENABLED  = pfx.at("palette_enabled").asBool();
        if (pfx.count("palette_levels"))    cfg::PostFX::PALETTE_LEVELS   = (float)pfx.at("palette_levels").asDouble();
        if (pfx.count("brightness"))        cfg::PostFX::BRIGHTNESS       = (float)pfx.at("brightness").asDouble();
        if (pfx.count("contrast"))          cfg::PostFX::CONTRAST         = (float)pfx.at("contrast").asDouble();
        if (pfx.count("tint_r"))            cfg::PostFX::TINT_R           = (float)pfx.at("tint_r").asDouble();
        if (pfx.count("tint_g"))            cfg::PostFX::TINT_G           = (float)pfx.at("tint_g").asDouble();
        if (pfx.count("tint_b"))            cfg::PostFX::TINT_B           = (float)pfx.at("tint_b").asDouble();
    }

    // --- PARTICLES ---
    if (rootObj.count("particles")) {
        const auto& pt = rootObj.at("particles").asObject();
        if (pt.count("gravity")) cfg::Particles::GRAVITY = (float)pt.at("gravity").asDouble();
        if (pt.count("base_size")) cfg::Particles::BASE_SIZE = (float)pt.at("base_size").asDouble();

        if (pt.count("walk_dust_count_min")) cfg::Particles::WALK_DUST_COUNT_MIN = pt.at("walk_dust_count_min").asInt();
        if (pt.count("walk_dust_count_max")) cfg::Particles::WALK_DUST_COUNT_MAX = pt.at("walk_dust_count_max").asInt();
        if (pt.count("walk_dust_offset_x")) cfg::Particles::WALK_DUST_OFFSET_X = (float)pt.at("walk_dust_offset_x").asDouble();
        if (pt.count("walk_dust_offset_y")) cfg::Particles::WALK_DUST_OFFSET_Y = (float)pt.at("walk_dust_offset_y").asDouble();
        if (pt.count("walk_dust_speed_x")) cfg::Particles::WALK_DUST_SPEED_X = (float)pt.at("walk_dust_speed_x").asDouble();
        if (pt.count("walk_dust_speed_y_min")) cfg::Particles::WALK_DUST_SPEED_Y_MIN = (float)pt.at("walk_dust_speed_y_min").asDouble();
        if (pt.count("walk_dust_speed_y_max")) cfg::Particles::WALK_DUST_SPEED_Y_MAX = (float)pt.at("walk_dust_speed_y_max").asDouble();
        if (pt.count("walk_dust_life_min")) cfg::Particles::WALK_DUST_LIFE_MIN = (float)pt.at("walk_dust_life_min").asDouble();
        if (pt.count("walk_dust_life_max")) cfg::Particles::WALK_DUST_LIFE_MAX = (float)pt.at("walk_dust_life_max").asDouble();
        if (pt.count("walk_dust_color_mult")) cfg::Particles::WALK_DUST_COLOR_MULT = (float)pt.at("walk_dust_color_mult").asDouble();
        if (pt.count("walk_dust_alpha")) cfg::Particles::WALK_DUST_ALPHA = pt.at("walk_dust_alpha").asInt();

        if (pt.count("hit_count_min")) cfg::Particles::HIT_COUNT_MIN = pt.at("hit_count_min").asInt();
        if (pt.count("hit_count_max")) cfg::Particles::HIT_COUNT_MAX = pt.at("hit_count_max").asInt();
        if (pt.count("hit_speed_min")) cfg::Particles::HIT_SPEED_MIN = (float)pt.at("hit_speed_min").asDouble();
        if (pt.count("hit_speed_max")) cfg::Particles::HIT_SPEED_MAX = (float)pt.at("hit_speed_max").asDouble();
        if (pt.count("hit_life_min")) cfg::Particles::HIT_LIFE_MIN = (float)pt.at("hit_life_min").asDouble();
        if (pt.count("hit_life_max")) cfg::Particles::HIT_LIFE_MAX = (float)pt.at("hit_life_max").asDouble();
        if (pt.count("hit_bias_y")) cfg::Particles::HIT_BIAS_Y = (float)pt.at("hit_bias_y").asDouble();

        if (pt.count("rock_count_min")) cfg::Particles::ROCK_COUNT_MIN = pt.at("rock_count_min").asInt();
        if (pt.count("rock_count_max")) cfg::Particles::ROCK_COUNT_MAX = pt.at("rock_count_max").asInt();
        if (pt.count("rock_speed_min")) cfg::Particles::ROCK_SPEED_MIN = (float)pt.at("rock_speed_min").asDouble();
        if (pt.count("rock_speed_max")) cfg::Particles::ROCK_SPEED_MAX = (float)pt.at("rock_speed_max").asDouble();
        if (pt.count("rock_life_min")) cfg::Particles::ROCK_LIFE_MIN = (float)pt.at("rock_life_min").asDouble();
        if (pt.count("rock_life_max")) cfg::Particles::ROCK_LIFE_MAX = (float)pt.at("rock_life_max").asDouble();

        if (pt.count("debris_count_min")) cfg::Particles::DEBRIS_COUNT_MIN = pt.at("debris_count_min").asInt();
        if (pt.count("debris_count_max")) cfg::Particles::DEBRIS_COUNT_MAX = pt.at("debris_count_max").asInt();
        if (pt.count("debris_speed_min")) cfg::Particles::DEBRIS_SPEED_MIN = (float)pt.at("debris_speed_min").asDouble();
        if (pt.count("debris_speed_max")) cfg::Particles::DEBRIS_SPEED_MAX = (float)pt.at("debris_speed_max").asDouble();
        if (pt.count("debris_life_min")) cfg::Particles::DEBRIS_LIFE_MIN = (float)pt.at("debris_life_min").asDouble();
        if (pt.count("debris_life_max")) cfg::Particles::DEBRIS_LIFE_MAX = (float)pt.at("debris_life_max").asDouble();
        if (pt.count("debris_bias_y")) cfg::Particles::DEBRIS_BIAS_Y = (float)pt.at("debris_bias_y").asDouble();
        if (pt.count("debris_gravity_scale")) cfg::Particles::DEBRIS_GRAVITY_SCALE = (float)pt.at("debris_gravity_scale").asDouble();

        if (pt.count("power_count_min")) cfg::Particles::POWER_COUNT_MIN = pt.at("power_count_min").asInt();
        if (pt.count("power_count_max")) cfg::Particles::POWER_COUNT_MAX = pt.at("power_count_max").asInt();
        if (pt.count("power_speed_min")) cfg::Particles::POWER_SPEED_MIN = (float)pt.at("power_speed_min").asDouble();
        if (pt.count("power_speed_max")) cfg::Particles::POWER_SPEED_MAX = (float)pt.at("power_speed_max").asDouble();
        if (pt.count("power_life_min")) cfg::Particles::POWER_LIFE_MIN = (float)pt.at("power_life_min").asDouble();
        if (pt.count("power_life_max")) cfg::Particles::POWER_LIFE_MAX = (float)pt.at("power_life_max").asDouble();

        if (pt.count("blood_count_min")) cfg::Particles::BLOOD_COUNT_MIN = pt.at("blood_count_min").asInt();
        if (pt.count("blood_count_max")) cfg::Particles::BLOOD_COUNT_MAX = pt.at("blood_count_max").asInt();
        if (pt.count("blood_speed_min")) cfg::Particles::BLOOD_SPEED_MIN = (float)pt.at("blood_speed_min").asDouble();
        if (pt.count("blood_speed_max")) cfg::Particles::BLOOD_SPEED_MAX = (float)pt.at("blood_speed_max").asDouble();
        if (pt.count("blood_life_min")) cfg::Particles::BLOOD_LIFE_MIN = (float)pt.at("blood_life_min").asDouble();
        if (pt.count("blood_life_max")) cfg::Particles::BLOOD_LIFE_MAX = (float)pt.at("blood_life_max").asDouble();

        if (pt.count("charge_count_min")) cfg::Particles::CHARGE_COUNT_MIN = pt.at("charge_count_min").asInt();
        if (pt.count("charge_count_max")) cfg::Particles::CHARGE_COUNT_MAX = pt.at("charge_count_max").asInt();
        if (pt.count("charge_speed_min")) cfg::Particles::CHARGE_SPEED_MIN = (float)pt.at("charge_speed_min").asDouble();
        if (pt.count("charge_speed_max")) cfg::Particles::CHARGE_SPEED_MAX = (float)pt.at("charge_speed_max").asDouble();
        if (pt.count("charge_life_min")) cfg::Particles::CHARGE_LIFE_MIN = (float)pt.at("charge_life_min").asDouble();
        if (pt.count("charge_life_max")) cfg::Particles::CHARGE_LIFE_MAX = (float)pt.at("charge_life_max").asDouble();
        if (pt.count("charge_gravity_scale")) cfg::Particles::CHARGE_GRAVITY_SCALE = (float)pt.at("charge_gravity_scale").asDouble();

        if (pt.count("charge_tick_count_min")) cfg::Particles::CHARGE_TICK_COUNT_MIN = pt.at("charge_tick_count_min").asInt();
        if (pt.count("charge_tick_count_max")) cfg::Particles::CHARGE_TICK_COUNT_MAX = pt.at("charge_tick_count_max").asInt();
        if (pt.count("charge_tick_speed_min")) cfg::Particles::CHARGE_TICK_SPEED_MIN = (float)pt.at("charge_tick_speed_min").asDouble();
        if (pt.count("charge_tick_speed_max")) cfg::Particles::CHARGE_TICK_SPEED_MAX = (float)pt.at("charge_tick_speed_max").asDouble();
        if (pt.count("charge_tick_life_min")) cfg::Particles::CHARGE_TICK_LIFE_MIN = (float)pt.at("charge_tick_life_min").asDouble();
        if (pt.count("charge_tick_life_max")) cfg::Particles::CHARGE_TICK_LIFE_MAX = (float)pt.at("charge_tick_life_max").asDouble();
        if (pt.count("charge_tick_gravity_scale")) cfg::Particles::CHARGE_TICK_GRAVITY_SCALE = (float)pt.at("charge_tick_gravity_scale").asDouble();
    }

    // Load IK config file if present
    loadIKConfig("assets/data/ik_config.json");

    // Load Wind config file if present
    loadWindConfig("assets/data/wind.json");

    std::cout << "[ConfigManager] Configuration loaded successfully.\n";
}

void ConfigManager::loadIKConfig(const std::string& path) {
    std::cout << "[ConfigManager] Loading IK config from " << path << "...\n";
    json::Value root = json::parseFile(path);
    if (root.type == json::Type::Null) {
        std::cerr << "[ConfigManager] WARNING: Failed to load IK config file (" << path << "). Using default values.\n";
        return;
    }

    const auto& ikObj = root.asObject();
    if (ikObj.count("recoil_stiffness")) cfg::IK::RECOIL_STIFFNESS = (float)ikObj.at("recoil_stiffness").asDouble();
    if (ikObj.count("recoil_damping")) cfg::IK::RECOIL_DAMPING = (float)ikObj.at("recoil_damping").asDouble();
    if (ikObj.count("recoil_force_lin")) cfg::IK::RECOIL_FORCE_LIN = (float)ikObj.at("recoil_force_lin").asDouble();
    if (ikObj.count("recoil_force_rot")) cfg::IK::RECOIL_FORCE_ROT = (float)ikObj.at("recoil_force_rot").asDouble();

    if (ikObj.count("attack_stiffness")) cfg::IK::ATTACK_STIFFNESS = (float)ikObj.at("attack_stiffness").asDouble();
    if (ikObj.count("attack_damping")) cfg::IK::ATTACK_DAMPING = (float)ikObj.at("attack_damping").asDouble();
    if (ikObj.count("attack_force_lin")) cfg::IK::ATTACK_FORCE_LIN = (float)ikObj.at("attack_force_lin").asDouble();
    if (ikObj.count("attack_force_rot")) cfg::IK::ATTACK_FORCE_ROT = (float)ikObj.at("attack_force_rot").asDouble();

    if (ikObj.count("sway_lean_mult")) cfg::IK::SWAY_LEAN_MULT = (float)ikObj.at("sway_lean_mult").asDouble();
    if (ikObj.count("sway_max_lean")) cfg::IK::SWAY_MAX_LEAN = (float)ikObj.at("sway_max_lean").asDouble();
    if (ikObj.count("weight_lag_mult")) cfg::IK::WEIGHT_LAG_MULT = (float)ikObj.at("weight_lag_mult").asDouble();
    if (ikObj.count("sway_smooth_freq")) cfg::IK::SWAY_SMOOTH_FREQ = (float)ikObj.at("sway_smooth_freq").asDouble();
    if (ikObj.count("lean_smooth_freq")) cfg::IK::LEAN_SMOOTH_FREQ = (float)ikObj.at("lean_smooth_freq").asDouble();

    if (ikObj.count("foot_lerp_freq")) cfg::IK::FOOT_LERP_FREQ = (float)ikObj.at("foot_lerp_freq").asDouble();
    if (ikObj.count("foot_sample_offset")) cfg::IK::FOOT_SAMPLE_OFFSET = (float)ikObj.at("foot_sample_offset").asDouble();
    if (ikObj.count("foot_max_rot")) cfg::IK::FOOT_MAX_ROT = (float)ikObj.at("foot_max_rot").asDouble();
    if (ikObj.count("body_depth_mult")) cfg::IK::BODY_DEPTH_MULT = (float)ikObj.at("body_depth_mult").asDouble();

    if (ikObj.count("hitstop_time_scale")) cfg::IK::HITSTOP_TIME_SCALE = (float)ikObj.at("hitstop_time_scale").asDouble();

    if (ikObj.count("grip_offset_x")) cfg::IK::GRIP_OFFSET_X = (float)ikObj.at("grip_offset_x").asDouble();
    if (ikObj.count("grip_offset_y")) cfg::IK::GRIP_OFFSET_Y = (float)ikObj.at("grip_offset_y").asDouble();
    if (ikObj.count("grip_lerp_factor")) cfg::IK::GRIP_LERP_FACTOR = (float)ikObj.at("grip_lerp_factor").asDouble();
    if (ikObj.count("grip_rot_offset")) cfg::IK::GRIP_ROT_OFFSET = (float)ikObj.at("grip_rot_offset").asDouble();

    if (ikObj.count("step_impact_stiffness")) cfg::IK::STEP_IMPACT_STIFFNESS = (float)ikObj.at("step_impact_stiffness").asDouble();
    if (ikObj.count("step_impact_damping")) cfg::IK::STEP_IMPACT_DAMPING = (float)ikObj.at("step_impact_damping").asDouble();
    if (ikObj.count("step_impact_force")) cfg::IK::STEP_IMPACT_FORCE = (float)ikObj.at("step_impact_force").asDouble();
    if (ikObj.count("head_impact_lag")) cfg::IK::HEAD_IMPACT_LAG = (float)ikObj.at("head_impact_lag").asDouble();
    if (ikObj.count("weight_stance_sink")) cfg::IK::WEIGHT_STANCE_SINK = (float)ikObj.at("weight_stance_sink").asDouble();

    std::cout << "[ConfigManager] IK Configuration loaded successfully.\n";
}

void ConfigManager::loadWindConfig(const std::string& path) {
    std::cout << "[ConfigManager] Loading Wind config from " << path << "...\n";
    json::Value root = json::parseFile(path);
    if (root.type == json::Type::Null) {
        std::cerr << "[ConfigManager] WARNING: Failed to load Wind config file (" << path << "). Using default values.\n";
        return;
    }

    const auto& wObj = root.asObject();
    if (wObj.count("enable")) cfg::Wind::ENABLE = wObj.at("enable").asBool();
    if (wObj.count("enabled")) cfg::Wind::ENABLE = wObj.at("enabled").asBool();
    if (wObj.count("speed")) cfg::Wind::SPEED = (float)wObj.at("speed").asDouble();
    if (wObj.count("strength")) cfg::Wind::STRENGTH = (float)wObj.at("strength").asDouble();
    if (wObj.count("frequency")) cfg::Wind::FREQUENCY = (float)wObj.at("frequency").asDouble();
    if (wObj.count("direction_x")) cfg::Wind::DIRECTION_X = (float)wObj.at("direction_x").asDouble();
    if (wObj.count("direction_y")) cfg::Wind::DIRECTION_Y = (float)wObj.at("direction_y").asDouble();
    if (wObj.count("turbulence")) cfg::Wind::TURBULENCE = (float)wObj.at("turbulence").asDouble();
    if (wObj.count("turbulence_speed")) cfg::Wind::TURBULENCE_SPEED = (float)wObj.at("turbulence_speed").asDouble();
    if (wObj.count("turbulence_spatial_mult")) cfg::Wind::TURBULENCE_SPATIAL_MULT = (float)wObj.at("turbulence_spatial_mult").asDouble();
    if (wObj.count("gust_strength")) cfg::Wind::GUST_STRENGTH = (float)wObj.at("gust_strength").asDouble();
    if (wObj.count("gust_frequency")) cfg::Wind::GUST_FREQUENCY = (float)wObj.at("gust_frequency").asDouble();
    if (wObj.count("gust_spatial_factor")) cfg::Wind::GUST_SPATIAL_FACTOR = (float)wObj.at("gust_spatial_factor").asDouble();
    if (wObj.count("gust_turbulence_boost")) cfg::Wind::GUST_TURBULENCE_BOOST = (float)wObj.at("gust_turbulence_boost").asDouble();
    if (wObj.count("trunk_threshold")) cfg::Wind::TRUNK_THRESHOLD = (float)wObj.at("trunk_threshold").asDouble();
    if (wObj.count("canopy_curve_exponent")) cfg::Wind::CANOPY_CURVE_EXPONENT = (float)wObj.at("canopy_curve_exponent").asDouble();
    if (wObj.count("micro_curve_multiplier")) cfg::Wind::MICRO_CURVE_MULTIPLIER = (float)wObj.at("micro_curve_multiplier").asDouble();

    std::cout << "[ConfigManager] Wind Configuration loaded successfully (ENABLE=" << (cfg::Wind::ENABLE ? "true" : "false") << ").\n";
}
