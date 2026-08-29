#include "CommandManager.h"
#include "entities/Entity.h"
#include <algorithm>

std::string CommandManager::processCommand(const std::string& fullText, Player* player, Entity* target) {
    if (fullText.empty() || !player) return "";

    // Support pasting/executing multiple commands separated by newlines
    if (fullText.find('\n') != std::string::npos) {
        std::stringstream ss(fullText);
        std::string line;
        std::string combinedFeedback = "";
        int commandCount = 0;

        while (std::getline(ss, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                line.pop_back();
            }
            int startIdx = 0;
            while (startIdx < line.size() && line[startIdx] == ' ') {
                startIdx++;
            }
            if (startIdx >= line.size()) continue;

            std::string cleanLine = line.substr(startIdx);
            if (cleanLine[0] == '/') {
                std::string feedback = processCommand(cleanLine, player, target);
                if (!feedback.empty()) {
                    if (!combinedFeedback.empty()) combinedFeedback += "\n";
                    combinedFeedback += feedback;
                }
                commandCount++;
            }
        }
        if (commandCount > 0) {
            return combinedFeedback;
        }
    }

    // 1. [TRIM & PREFIX CHECK]
    int startIdx = 0;
    while(startIdx < fullText.size() && fullText[startIdx] == ' ') {
        startIdx++;
    }
    
    if (startIdx >= fullText.size() || fullText[startIdx] != '/') {
        // Not a command, treated as Chat
        return "[YO]: " + fullText;
    }

    // Remove leading spaces and '/'
    std::string commandText = fullText.substr(startIdx + 1);
    if (commandText.empty()) return "";

    auto args = splitString(commandText);
    if (args.empty()) return "";

    std::string action = args[0];
    // Convert to uppercase for case-insensitive commands
    std::transform(action.begin(), action.end(), action.begin(), ::toupper);

    if (action == "DARME" || action == "GIVE" || action == "D") {
        return cmdGive(args, player, target);
    }
    else if (action == "KILL" || action == "K") {
        return cmdKill(args, player, target);
    }
    else if (action == "COMANDOS" || action == "HELP") {
        return cmdList();
    }
    
    return "Comando desconocido: " + action + ". Usa /COMANDOS para ayuda.";
}

std::vector<std::string> CommandManager::splitString(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string item;
    while (ss >> item) {
        tokens.push_back(item);
    }
    return tokens;
}

std::string CommandManager::cmdGive(const std::vector<std::string>& args, Player* player, Entity* target) {
    bool isFixed = false;
    std::string stat;
    std::string amountStr;

    if (args.size() >= 4 && (args[1] == "F" || args[1] == "f")) {
        isFixed = true;
        stat = args[2];
        amountStr = args[3];
    } else if (args.size() >= 3) {
        stat = args[1];
        amountStr = args[2];
    } else {
        return "Uso: /D [F] <stat/codigo> <cantidad>";
    }

    stat = mapNumericStat(stat);
    std::string statL = stat;
    std::transform(statL.begin(), statL.end(), statL.begin(), ::tolower);

    float amountF = 0.0f;
    try {
        amountF = std::stof(amountStr);
    } catch (...) {
        return "Cantidad invalida.";
    }
    int amountI = static_cast<int>(amountF);

    Entity* targetEntity = target ? target : player;

    // 1. XP Alias
    if (statL == "xp" || statL == "exp") {
        if (auto* p = dynamic_cast<Player*>(targetEntity)) {
            p->addExperience(amountI, false);
            return "XP DADA: " + std::to_string(amountI);
        } else {
            return "La XP solo se puede dar al Jugador.";
        }
    }

    // Curar/Setear HP
    if (statL == "hp") {
        if (isFixed) {
            targetEntity->setCurrentHp(amountI);
            std::string name = (targetEntity == player) ? "Jugador" : targetEntity->getName();
            return "Vida fijada de " + name + " a " + std::to_string(amountI) + " HP";
        } else {
            targetEntity->heal(amountI);
            std::string name = (targetEntity == player) ? "Jugador" : targetEntity->getName();
            return "Vida curada a " + name + ": +" + std::to_string(amountI) + " HP";
        }
    }

    // Restaurar/Setear Mana
    if (statL == "mana") {
        if (isFixed) {
            targetEntity->setCurrentMp(amountI);
            std::string name = (targetEntity == player) ? "Jugador" : targetEntity->getName();
            return "Mana fijado de " + name + " a " + std::to_string(amountI) + " MP";
        } else {
            targetEntity->restoreMana(amountI);
            std::string name = (targetEntity == player) ? "Jugador" : targetEntity->getName();
            return "Mana restaurado a " + name + ": +" + std::to_string(amountI) + " MP";
        }
    }
    
    // Delega el mapeo de sumar stats directamente a las variables de la entidad seleccionada
    bool recognized = targetEntity->debugAddStat(statL, amountF, isFixed);
    
    if (!recognized) {
        return "Stat no reconocida: " + stat;
    }

    std::string targetName = (targetEntity == player) ? "Jugador" : targetEntity->getName();
    std::string mode = isFixed ? "FIJADA EN " : "DADA A ";
    return "STAT " + statL + " " + mode + targetName;
}

std::string CommandManager::mapNumericStat(const std::string& input) {
    if (input == "1") return "xp";
    if (input == "2") return "str";
    if (input == "3") return "dex";
    if (input == "4") return "int";
    if (input == "5") return "vit";
    if (input == "6") return "peso";
    if (input == "7") return "speed";
    if (input == "8") return "atkspeed";
    if (input == "9") return "atkspeedperagi";
    if (input == "10") return "accuracy";
    if (input == "11") return "evasion";
    if (input == "12") return "armorpenpercent";
    if (input == "13") return "armorpenflat";
    if (input == "14") return "physicaldmgbonus";
    if (input == "15") return "critchance";
    if (input == "16") return "critdamage";
    if (input == "17") return "lifestealpercent";
    if (input == "18") return "manastealpercent";
    if (input == "19") return "doublechance";
    if (input == "20") return "triplechance";
    if (input == "21") return "enemymaxhpdamagepercent";
    if (input == "22") return "truedamagepercent";
    if (input == "23") return "aoeradius";
    if (input == "24") return "aoedamagepercent";
    if (input == "25") return "executedamagepercent";
    if (input == "26") return "executethresholdpercent";
    if (input == "27") return "blockchance";
    if (input == "28") return "blockvaluepercent";
    if (input == "29") return "thornspercent";
    if (input == "30") return "tenacitypercent";
    if (input == "31") return "damagereductionpercent";
    if (input == "32") return "critavoidancepercent";
    if (input == "33") return "antiarmorpenpercent";
    if (input == "34") return "antiarmorpenflat";
    if (input == "35") return "hppervit";
    if (input == "36") return "defpervit";
    if (input == "37") return "mpperint";
    if (input == "38") return "atkperstr";
    if (input == "39") return "hpregenpercent";
    if (input == "40") return "mpregenpercent";
    if (input == "41") return "xpbonuspercent";
    if (input == "42") return "bleeddurationflat";
    if (input == "43") return "bleeddurationpercent";
    if (input == "44") return "bleedflat";
    if (input == "45") return "bleedpercent";
    if (input == "46") return "stunchance";
    if (input == "47") return "stunduration";
    if (input == "48") return "slowmovepercent";
    if (input == "49") return "slowmoveduration";
    if (input == "50") return "slowattackpercent";
    if (input == "51") return "slowattackduration";
    if (input == "52") return "maxhp";
    if (input == "53") return "maxmp";
    if (input == "54") return "atk";
    if (input == "55") return "def";
    if (input == "56") return "range";
    if (input == "57" || input == "cdr") return "cooldownreductionpercent";
    if (input == "58" || input == "hp" || input == "health" || input == "vida") return "hp";
    if (input == "59" || input == "mana" || input == "mp") return "mana";
    if (input == "99" || input == "malice" || input == "malicia") return "malice";
    return input;
}

std::string CommandManager::cmdList() {
    std::stringstream ss;
    ss << "=== COMANDOS DISPONIBLES ===\n";
    ss << "/DARME <stat> <valor>  o  /D <codigo> <valor>\n";
    ss << "-- Atributos Base --\n";
    ss << "  1:xp, 2:str, 3:dex, 4:int, 5:vit, 6:peso\n";
    ss << "-- Combate --\n";
    ss << "  7:speed, 8:atkspeed\n";
    ss << "  10:accuracy, 11:evasion\n";
    ss << "-- Ofensivos --\n";
    ss << "  12:armorpenpercent, 13:armorpenflat\n";
    ss << "  14:physicaldmgbonus\n";
    ss << "  15:critchance, 16:critdamage\n";
    ss << "  17:lifestealpercent, 18:manastealpercent\n";
    ss << "  19:doublechance, 20:triplechance\n";
    ss << "  21:enemymaxhpdamagepercent, 22:truedamagepercent\n";
    ss << "  23:aoeradius, 24:aoedamagepercent\n";
    ss << "  25:executedamagepercent, 26:executethresholdpercent\n";
    ss << "-- Defensivos --\n";
    ss << "  27:blockchance, 28:blockvaluepercent\n";
    ss << "  29:thornspercent, 30:tenacitypercent\n";
    ss << "  31:damagereductionpercent\n";
    ss << "  32:critavoidancepercent\n";
    ss << "  33:antiarmorpenpercent, 34:antiarmorpenflat\n";
    ss << "-- Scaling & Regen --\n";
    ss << "  9:atkspeedperagi\n";
    ss << "  35:hppervit, 36:defpervit, 37:mpperint, 38:atkperstr\n";
    ss << "  39:hpregenpercent, 40:mpregenpercent\n";
    ss << "  41:xpbonuspercent, 57:cooldownreductionpercent (cdr)\n";
    ss << "-- Estados (Bleed/Stun/Slow) --\n";
    ss << "  42:bleeddurationflat, 43:bleeddurationpercent\n";
    ss << "  44:bleedflat, 45:bleedpercent\n";
    ss << "  46:stunchance, 47:stunduration\n";
    ss << "  48:slowmovepercent, 49:slowmoveduration\n";
    ss << "  50:slowattackpercent, 51:slowattackduration\n";
    ss << "-- Stats Directos --\n";
    ss << "  52:maxhp, 53:maxmp, 54:atk, 55:def, 56:range, 58:hp, 59:mana\n";
    ss << "\n/KILL      (Mata a la entidad seleccionada)";
    ss << "\n/COMANDOS  (Muestra esta lista)";
    return ss.str();
}

std::string CommandManager::cmdKill(const std::vector<std::string>& args, Player* player, Entity* target) {
    if (!target) {
        return "No hay ninguna entidad seleccionada.";
    }
    if (!target->isAlive()) {
        return "La entidad ya esta muerta: " + target->getName();
    }
    target->takeDamage(9999999, player, false, true); // true damage to bypass block/evasion/armor
    return "ENTIDAD ELIMINADA: " + target->getName();
}
