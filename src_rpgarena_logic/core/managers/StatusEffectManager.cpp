#include "StatusEffectManager.h"
#include "utils/TinyJson.h"
#include <iostream>

bool StatusEffectManager::loadConfig(const std::string& filepath) {
    mEffects.clear();
    std::cout << "[StatusEffectManager] Loading status effects config from " << filepath << "...\n";

    json::Value root = json::parseFile(filepath);
    if (root.type == json::Type::Null) {
        std::cerr << "[StatusEffectManager] ERROR: Failed to load config file: " << filepath << "\n";
        return false;
    }

    if (root.type != json::Type::Object) {
        std::cerr << "[StatusEffectManager] ERROR: Root element in JSON is not an object.\n";
        return false;
    }

    const auto& rootObj = root.asObject();

    // Parse offsets
    if (rootObj.count("player_offset_x")) mPlayerOffsetX = static_cast<float>(rootObj.at("player_offset_x").asDouble());
    if (rootObj.count("player_offset_y")) mPlayerOffsetY = static_cast<float>(rootObj.at("player_offset_y").asDouble());
    if (rootObj.count("target_offset_x")) mTargetOffsetX = static_cast<float>(rootObj.at("target_offset_x").asDouble());
    if (rootObj.count("target_offset_y")) mTargetOffsetY = static_cast<float>(rootObj.at("target_offset_y").asDouble());
    if (rootObj.count("hover_offset_x")) mHoverOffsetX = static_cast<float>(rootObj.at("hover_offset_x").asDouble());
    if (rootObj.count("hover_offset_y")) mHoverOffsetY = static_cast<float>(rootObj.at("hover_offset_y").asDouble());

    if (rootObj.count("effects") && rootObj.at("effects").type == json::Type::Array) {
        const auto& arr = rootObj.at("effects").asArray();
        for (const auto& item : arr) {
            if (item.type != json::Type::Object) continue;

            const auto& obj = item.asObject();
            if (obj.count("id") == 0) continue;

            StatusEffectInfo info;
            info.id = obj.at("id").asString();

            if (obj.count("name")) info.name = obj.at("name").asString();
            if (obj.count("description")) info.description = obj.at("description").asString();
            
            // Parse atlas_index as [X, Y]
            if (obj.count("atlas_index") && obj.at("atlas_index").type == json::Type::Array) {
                const auto& idxArr = obj.at("atlas_index").asArray();
                if (idxArr.size() >= 2) {
                    info.atlasX = idxArr[0].asInt();
                    info.atlasY = idxArr[1].asInt();
                }
            }
            if (obj.count("is_debuff")) info.isDebuff = obj.at("is_debuff").asBool();

            mEffects[info.id] = info;
        }
    }

    std::cout << "[StatusEffectManager] Loaded " << mEffects.size() << " status effects. "
              << "Offsets: Player(" << mPlayerOffsetX << "," << mPlayerOffsetY << "), "
              << "Target(" << mTargetOffsetX << "," << mTargetOffsetY << ")\n";
    return true;
}

const StatusEffectInfo* StatusEffectManager::getEffectInfo(const std::string& id) const {
    auto it = mEffects.find(id);
    if (it != mEffects.end()) {
        return &it->second;
    }
    return nullptr;
}
