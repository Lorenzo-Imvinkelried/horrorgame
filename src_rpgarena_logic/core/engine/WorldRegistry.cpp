#include "WorldRegistry.h"

// [CRITICAL] Definition of static member to avoid linker errors
std::map<std::string, WorldData> WorldRegistry::sWorlds;

void WorldRegistry::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[WorldRegistry] ERROR: Could not open " << filename << "\n";
        return;
    }

    sWorlds.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string id, map, tileset, decor, entities;
        if (ss >> id >> map >> tileset >> decor >> entities) {
            sWorlds[id] = {map, tileset, decor, entities};
            std::cout << "[WorldRegistry] Loaded world: " << id << "\n";
        }
    }
}

const WorldData& WorldRegistry::get(const std::string& id) {
    auto it = sWorlds.find(id);
    if (it == sWorlds.end()) {
        std::cerr << "[WorldRegistry] CRITICAL: World ID not found: " << id << "\n";
        static WorldData empty; // Return empty to prevent crash
        return empty;
    }
    return it->second;
}
