#include "CharacterPanel.h"
#include "Config.h"
#include "utils/TinyJson.h"
#include <fstream>
#include <iostream>

void CharacterPanel::setDebugLayoutMode(bool enabled) {
    mDebugLayoutMode = enabled;
    if (!enabled) {
        mSelectedDebugSlotIndex = -1;
    }
}

void CharacterPanel::loadLayout() {
    if (sLayoutInitialized) return;
    
    float zoom = cfg::Map::ZOOM_FACTOR;
    float unscaled_cell_w = cfg::UI::BASE_SLOT_SIZE + (cfg::UI::UNIFIED_SLOT_MARGIN / zoom);
    float unscaled_cell_h = cfg::UI::BASE_SLOT_SIZE + (cfg::UI::UNIFIED_SLOT_MARGIN / zoom);
    
    sSlotLayouts.clear();
    struct DefaultSlot {
        const char* name;
        float gridX;
        float gridY;
    };
    const DefaultSlot DEFAULTS[] = {
        { "Casco", 1.0f, 0.0f },
        { "Capa", 0.0f, 1.0f },
        { "Pechera", 1.0f, 1.0f },
        { "Guantes", 2.0f, 1.0f },
        { "Arma 1", 0.0f, 2.0f },
        { "Pantalon", 1.0f, 2.0f },
        { "Arma 2", 2.0f, 2.0f },
        { "Anillo 1", 0.0f, 3.0f },
        { "Botas", 1.0f, 3.0f },
        { "Anillo 2", 2.0f, 3.0f },
        { "subarma1", 0.0f, 4.0f },
        { "subarma2", 2.0f, 4.0f },
        { "Cultivo", 1.0f, 4.0f }
    };
    
    for (const auto& d : DEFAULTS) {
        SlotConfig config;
        config.name = d.name;
        config.offset.x = cfg::UI::CharacterPanel::EQUIP_GRID_OFFSET_X + d.gridX * unscaled_cell_w;
        config.offset.y = cfg::UI::CharacterPanel::EQUIP_GRID_OFFSET_Y + d.gridY * unscaled_cell_h;
        sSlotLayouts.push_back(config);
    }
    
    std::string path = "assets/data/character_panel_layout.json";
    json::Value root = json::parseFile(path);
    if (root.type == json::Type::Object) {
        const auto& obj = root.asObject();
        for (auto& slot : sSlotLayouts) {
            auto it = obj.find(slot.name);
            if (it != obj.end() && it->second.type == json::Type::Object) {
                const auto& slotData = it->second.asObject();
                auto itX = slotData.find("x");
                auto itY = slotData.find("y");
                if (itX != slotData.end() && itX->second.type == json::Type::Number &&
                    itY != slotData.end() && itY->second.type == json::Type::Number) {
                    slot.offset.x = static_cast<float>(itX->second.asDouble());
                    slot.offset.y = static_cast<float>(itY->second.asDouble());
                }
            }
        }
    }
    
    sLayoutInitialized = true;
}

void CharacterPanel::saveLayout() {
    std::string path = "assets/data/character_panel_layout.json";
    std::ofstream f(path);
    if (!f) {
        std::cerr << "[CharacterPanel] Failed to open layout file for writing: " << path << "\n";
        return;
    }
    
    f << "{\n";
    for (size_t i = 0; i < sSlotLayouts.size(); ++i) {
        const auto& slot = sSlotLayouts[i];
        f << "  \"" << slot.name << "\": {\n";
        f << "    \"x\": " << slot.offset.x << ",\n";
        f << "    \"y\": " << slot.offset.y << "\n";
        f << "  }";
        if (i + 1 < sSlotLayouts.size()) {
            f << ",";
        }
        f << "\n";
    }
    f << "}\n";
    f.close();
    std::cout << "[CharacterPanel] Layout saved to " << path << "\n";
}
