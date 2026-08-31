#pragma once
#include <string>
#include <algorithm>
#include <cmath>
#include <iostream>

#include "inventory/ItemTypes.h"

struct PlayerStats {
    // Progression
    int Level = 1;
    int CurrentExp = 0;
    int NextLevelExp = 100;
    int AvailableStatPoints = 3; // Available points to distribute

    // Core Primary Attributes (src_rpgarena_logic)
    int Strength = 10;     // Boosts Physical Damage
    int Agility = 8;       // Boosts Attack Speed, Crit Chance & Evasion
    int Intelligence = 6;  // Boosts Mana & Magic
    int Vitality = 12;     // Boosts Max HP

    // Calculated Combat Stats
    int MaxHP = 120;
    int CurrentHP = 120;
    int MaxMP = 50;
    int CurrentMP = 50;
    int Attack = 45;
    int Defense = 8;
    int Evasion = 10;
    int Accuracy = 85;
    float CritChance = 12.0f; // Percent
    float CritMultiplier = 1.85f;

    void RecalculateStats(const EquipmentStats& equipBonus = EquipmentStats()) {
        MaxHP = 70 + Vitality * 5 + Strength * 2 + equipBonus.maxHpBonus;
        MaxMP = 20 + Intelligence * 5 + equipBonus.maxMpBonus;
        Attack = 25 + Strength * 2 + (int)(Agility * 0.8f) + equipBonus.attackPower;
        Defense = 4 + (int)(Vitality * 0.5f) + (int)(Strength * 0.3f) + equipBonus.defense;
        Evasion = 5 + (int)(Agility * 0.7f) + equipBonus.evasion;
        Accuracy = 80 + (int)(Agility * 0.5f);
        CritChance = 5.0f + Agility * 0.7f + static_cast<float>(equipBonus.critChance);

        CurrentHP = std::clamp(CurrentHP, 0, MaxHP);
        CurrentMP = std::clamp(CurrentMP, 0, MaxMP);
    }

    float GetAttackSpeedMultiplier() const {
        return 1.0f + std::max(0, Agility - 8) * 0.05f;
    }

    bool AllocateStrength() {
        if (AvailableStatPoints > 0) {
            AvailableStatPoints--;
            Strength++;
            RecalculateStats();
            return true;
        }
        return false;
    }

    bool AllocateAgility() {
        if (AvailableStatPoints > 0) {
            AvailableStatPoints--;
            Agility++;
            RecalculateStats();
            return true;
        }
        return false;
    }

    bool AllocateVitality() {
        if (AvailableStatPoints > 0) {
            AvailableStatPoints--;
            Vitality++;
            RecalculateStats();
            CurrentHP = std::min(CurrentHP + 5, MaxHP);
            return true;
        }
        return false;
    }

    bool AllocateIntelligence() {
        if (AvailableStatPoints > 0) {
            AvailableStatPoints--;
            Intelligence++;
            RecalculateStats();
            CurrentMP = std::min(CurrentMP + 5, MaxMP);
            return true;
        }
        return false;
    }

    bool AddExp(int amount, bool& outLeveledUp) {
        CurrentExp += amount;
        outLeveledUp = false;

        while (CurrentExp >= NextLevelExp) {
            CurrentExp -= NextLevelExp;
            Level++;
            outLeveledUp = true;

            // Exponential Leveling Curve from rpgarena logic (1.45x)
            NextLevelExp = (int)(NextLevelExp * 1.45f);

            // Attribute gains + 3 free allocation points!
            Strength += 1;
            Agility += 1;
            Vitality += 1;
            Intelligence += 1;
            AvailableStatPoints += 3;

            RecalculateStats();

            // Full restoration on level up
            CurrentHP = MaxHP;
            CurrentMP = MaxMP;

            std::cout << "[RPG] LEVEL UP! You reached Level " << Level 
                      << "! Points Available: " << AvailableStatPoints
                      << "! Next Level EXP requirement: " << NextLevelExp << std::endl;
        }

        return outLeveledUp;
    }
};
