#pragma once
#include "../items/Item.h"
#include <cmath>
#include <string>

class FortifySystem {
public:
    static void fortify(Item& item) {
        // Increment fortification level
        item.fortificationLevel++;
        
        // Update name (e.g., "Sword +1")
        // Find if it already has a " +" suffix
        size_t plusPos = item.name.find(" +");
        if (plusPos != std::string::npos) {
            item.name = item.name.substr(0, plusPos);
        }
        item.name += " +" + std::to_string(item.fortificationLevel);

        if (item.type == ItemType::Weapon) {
            // Capture base stats if they weren't initialized
            if (item.basePhysicalDamage == 0 && item.stats.physicalDamage > 0) {
                item.basePhysicalDamage = item.stats.physicalDamage;
            }
            // Increase Physical Damage by 10% (ceil)
            item.stats.physicalDamage = (int)std::ceil(item.stats.physicalDamage * 1.1f);
        } else if (item.type == ItemType::Armor) {
            // Capture base stats if they weren't initialized
            if (item.baseDefense == 0 && item.stats.defense > 0) {
                item.baseDefense = item.stats.defense;
            }
            // Increase Defense by 10% (ceil)
            item.stats.defense = (int)std::ceil(item.stats.defense * 1.1f);
        }

        item.stats.value = (int)std::ceil(item.stats.value * 1.5f); // Value grows faster
    }
};
