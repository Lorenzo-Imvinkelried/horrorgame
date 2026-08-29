#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <functional>
#include <map>
#include "entities/player/Player.h"

class Entity;

class CommandManager {
public:
    CommandManager() = default;

    // Processes a raw command string
    // Returns a feedback message to show in chat
    std::string processCommand(const std::string& fullText, Player* player, Entity* target = nullptr);

private:
    std::vector<std::string> splitString(const std::string& text);
    std::string mapNumericStat(const std::string& input);
    
    // Actual command executors
    std::string cmdGive(const std::vector<std::string>& args, Player* player, Entity* target = nullptr);
    std::string cmdKill(const std::vector<std::string>& args, Player* player, Entity* target = nullptr);
    std::string cmdList(); // [NEW] /COMANDOS
};
