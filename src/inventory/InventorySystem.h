#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "CombatStats.h"

enum class ItemType {
    NONE,
    POTION_HEALTH,
    POTION_MANA,
    BLOOD_VIAL,
    RAW_MEAT,
    BEAST_PELT,
    CURSED_SWORD,
    IRON_SHIELD,
    SHADOW_RING,
    ANCIENT_AMULET
};

enum class EquipSlot {
    NONE,
    WEAPON,
    SHIELD,
    RING,
    AMULET
};

struct InventoryItem {
    ItemType type = ItemType::NONE;
    std::string name = "";
    std::string desc = "";
    int count = 0;
    EquipSlot slot = EquipSlot::NONE;
    int bonusAttack = 0;
    int bonusDefense = 0;
    int bonusCrit = 0;
    int bonusHp = 0;
    int bonusMp = 0;
};

class InventorySystem {
public:
    InventorySystem();
    ~InventorySystem();

    bool AddItem(ItemType type, int count = 1);
    bool UseOrEquipSlot(int slotIndex, PlayerStats& stats);
    bool UnequipSlot(EquipSlot slot, PlayerStats& stats);
    void RecalculateBonuses(PlayerStats& stats);

    void RenderWindow(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float mouseNdcX, float mouseNdcY);
    bool HandleMouseClick(float mouseNdcX, float mouseNdcY, PlayerStats& stats, bool& closeRequested);

    bool IsOpen() const { return m_isOpen; }
    void ToggleOpen() { m_isOpen = !m_isOpen; }
    void SetOpen(bool open) { m_isOpen = open; }

    static InventoryItem CreateItem(ItemType type, int count = 1);

private:
    bool m_isOpen = false;
    std::vector<InventoryItem> m_slots; // 12 slots (4x3 grid)
    InventoryItem m_equipWeapon;
    InventoryItem m_equipShield;
    InventoryItem m_equipRing;
    InventoryItem m_equipAmulet;

    int m_appliedBonusAttack = 0;
    int m_appliedBonusDefense = 0;
    int m_appliedBonusCrit = 0;
    int m_appliedBonusHp = 0;
    int m_appliedBonusMp = 0;
};
