#include "TitleManager.h"
#include "utils/TinyJson.h"
#include <iostream>
#include <cstdint>

bool TitleManager::loadTitles(const std::string& filepath) {
    mTitles.clear();
    mTitleLookup.clear();

    json::Value root = json::parseFile(filepath);
    if (root.type != json::Type::Array) {
        std::cerr << "[TitleManager] ERROR: Failed to parse titles from " << filepath << " (not an array)\n";
        return false;
    }

    const auto& arr = root.asArray();
    for (const auto& item : arr) {
        if (item.type != json::Type::Object) continue;

        const auto& obj = item.asObject();
        if (obj.count("id") == 0 || obj.count("name") == 0) continue;

        Title title;
        title.id = obj.at("id").asString();
        title.name = obj.at("name").asString();

        // Parse color
        if (obj.count("color") && obj.at("color").type == json::Type::Array) {
            const auto& colArr = obj.at("color").asArray();
            if (colArr.size() >= 3) {
                std::uint8_t r = static_cast<std::uint8_t>(colArr[0].asInt());
                std::uint8_t g = static_cast<std::uint8_t>(colArr[1].asInt());
                std::uint8_t b = static_cast<std::uint8_t>(colArr[2].asInt());
                std::uint8_t a = 255;
                if (colArr.size() >= 4) {
                    a = static_cast<std::uint8_t>(colArr[3].asInt());
                }
                title.color = sf::Color(r, g, b, a);
            }
        }

        // Parse stats
        if (obj.count("stats") && obj.at("stats").type == json::Type::Object) {
            const auto& statsObj = obj.at("stats").asObject();
            for (const auto& [statStr, valVal] : statsObj) {
                Stat stat = stringToStat(statStr);
                if (stat != Stat::None) {
                    title.stats[stat] = static_cast<float>(valVal.asDouble());
                } else {
                    std::cerr << "[TitleManager] Warning: Unknown stat '" << statStr << "' in title '" << title.id << "'\n";
                }
            }
        }

        mTitleLookup[title.id] = mTitles.size();
        mTitles.push_back(title);
    }

    std::cout << "[TitleManager] Successfully loaded " << mTitles.size() << " titles.\n";
    return true;
}

const Title* TitleManager::getTitle(const std::string& id) const {
    auto it = mTitleLookup.find(id);
    if (it != mTitleLookup.end()) {
        return &mTitles[it->second];
    }
    return nullptr;
}
