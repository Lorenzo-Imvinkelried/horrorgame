#pragma once
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

struct WorldData {
    std::string map;
    std::string tileset;
    std::string decor;
    std::string entities;
};

class WorldRegistry {
public:
    static void load(const std::string& filename);
    static const WorldData& get(const std::string& id);

private:
    static std::map<std::string, WorldData> sWorlds;
};
