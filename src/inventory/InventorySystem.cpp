#include "InventorySystem.h"
#include "ui/UIRenderer.h"
#include <algorithm>
#include <iostream>

InventoryItem InventorySystem::CreateItem(ItemType type, int count) {
    InventoryItem item;
    item.type = type;
    item.count = count;

    switch (type) {
        case ItemType::POTION_HEALTH:
            item.name = "MEDICINA_VENDA";
            item.desc = "RESTAURA +45 DE VIDA";
            item.slot = EquipSlot::NONE;
            break;
        case ItemType::POTION_MANA:
            item.name = "ETER_CORRUPTO";
            item.desc = "RESTAURA +35 DE MANA";
            item.slot = EquipSlot::NONE;
            break;
        case ItemType::BLOOD_VIAL:
            item.name = "VIAL_DE_SANGRE";
            item.desc = "CEBO DE SANGRE FRESCA";
            item.slot = EquipSlot::NONE;
            break;
        case ItemType::RAW_MEAT:
            item.name = "CARNE_CRUDA";
            item.desc = "RESTAURA +25 DE VIDA";
            item.slot = EquipSlot::NONE;
            break;
        case ItemType::BEAST_PELT:
            item.name = "PIEL_DE_BESTIA";
            item.desc = "MATERIAL DE VALOR (+40 EXP)";
            item.slot = EquipSlot::NONE;
            break;
        case ItemType::CURSED_SWORD:
            item.name = "ESPADA_MALDITA";
            item.desc = "ARMA: +8 ATQ / +5% CRIT";
            item.slot = EquipSlot::WEAPON;
            item.bonusAttack = 8;
            item.bonusCrit = 5;
            break;
        case ItemType::IRON_SHIELD:
            item.name = "ESCUDO_HIERRO";
            item.desc = "ESCUDO: +6 DEF / +10 HP";
            item.slot = EquipSlot::SHIELD;
            item.bonusDefense = 6;
            item.bonusHp = 10;
            break;
        case ItemType::SHADOW_RING:
            item.name = "ANILLO_SOMBRAS";
            item.desc = "ANILLO: +12% CRIT / +8% EVA";
            item.slot = EquipSlot::RING;
            item.bonusCrit = 12;
            break;
        case ItemType::ANCIENT_AMULET:
            item.name = "AMULETO_ANTIGUO";
            item.desc = "AMULETO: +15 HP / +20 MP";
            item.slot = EquipSlot::AMULET;
            item.bonusHp = 15;
            item.bonusMp = 20;
            break;
        default:
            break;
    }
    return item;
}

InventorySystem::InventorySystem() {
    m_slots.resize(12);
    // Starter pack
    m_slots[0] = CreateItem(ItemType::POTION_HEALTH, 2);
    m_slots[1] = CreateItem(ItemType::POTION_MANA, 1);
    m_slots[2] = CreateItem(ItemType::BLOOD_VIAL, 2);
    m_slots[3] = CreateItem(ItemType::SHADOW_RING, 1);
}

InventorySystem::~InventorySystem() {}

bool InventorySystem::AddItem(ItemType type, int count) {
    if (type == ItemType::NONE || count <= 0) return false;

    // Try stacking first
    for (auto& slot : m_slots) {
        if (slot.type == type && slot.slot == EquipSlot::NONE) {
            slot.count += count;
            return true;
        }
    }

    // Find empty slot
    for (auto& slot : m_slots) {
        if (slot.type == ItemType::NONE) {
            slot = CreateItem(type, count);
            return true;
        }
    }

    return false; // Inventory full!
}

bool InventorySystem::UseOrEquipSlot(int slotIndex, PlayerStats& stats) {
    if (slotIndex < 0 || slotIndex >= (int)m_slots.size()) return false;
    auto& item = m_slots[slotIndex];
    if (item.type == ItemType::NONE) return false;

    // 1. Consumable Items
    if (item.slot == EquipSlot::NONE) {
        if (item.type == ItemType::POTION_HEALTH) {
            stats.CurrentHP = std::min(stats.CurrentHP + 45, stats.MaxHP);
        } else if (item.type == ItemType::POTION_MANA) {
            stats.CurrentMP = std::min(stats.CurrentMP + 35, stats.MaxMP);
        } else if (item.type == ItemType::RAW_MEAT) {
            stats.CurrentHP = std::min(stats.CurrentHP + 25, stats.MaxHP);
        } else if (item.type == ItemType::BEAST_PELT) {
            bool lvlUp = false;
            stats.AddExp(40, lvlUp);
        } else if (item.type == ItemType::BLOOD_VIAL) {
            // Used as quick blood splash
            stats.CurrentMP = std::min(stats.CurrentMP + 10, stats.MaxMP);
        }

        item.count--;
        if (item.count <= 0) {
            item = InventoryItem();
        }
        return true;
    }

    // 2. Equipment Items
    InventoryItem* targetEquip = nullptr;
    if (item.slot == EquipSlot::WEAPON) targetEquip = &m_equipWeapon;
    else if (item.slot == EquipSlot::SHIELD) targetEquip = &m_equipShield;
    else if (item.slot == EquipSlot::RING) targetEquip = &m_equipRing;
    else if (item.slot == EquipSlot::AMULET) targetEquip = &m_equipAmulet;

    if (targetEquip != nullptr) {
        // Swap equipment
        InventoryItem temp = *targetEquip;
        *targetEquip = item;
        m_slots[slotIndex] = temp;
        RecalculateBonuses(stats);
        return true;
    }

    return false;
}

bool InventorySystem::UnequipSlot(EquipSlot slot, PlayerStats& stats) {
    InventoryItem* targetEquip = nullptr;
    if (slot == EquipSlot::WEAPON) targetEquip = &m_equipWeapon;
    else if (slot == EquipSlot::SHIELD) targetEquip = &m_equipShield;
    else if (slot == EquipSlot::RING) targetEquip = &m_equipRing;
    else if (slot == EquipSlot::AMULET) targetEquip = &m_equipAmulet;

    if (targetEquip == nullptr || targetEquip->type == ItemType::NONE) return false;

    // Find empty slot in backpack
    for (auto& bagSlot : m_slots) {
        if (bagSlot.type == ItemType::NONE) {
            bagSlot = *targetEquip;
            *targetEquip = InventoryItem();
            RecalculateBonuses(stats);
            return true;
        }
    }

    return false; // Backpack full!
}

void InventorySystem::RecalculateBonuses(PlayerStats& stats) {
    // 1. Remove previous applied bonuses
    stats.Attack -= m_appliedBonusAttack;
    stats.Defense -= m_appliedBonusDefense;
    stats.CritChance -= (float)m_appliedBonusCrit;
    stats.MaxHP -= m_appliedBonusHp;
    stats.MaxMP -= m_appliedBonusMp;

    // 2. Calculate new bonuses from all 4 equipment slots
    m_appliedBonusAttack = m_equipWeapon.bonusAttack + m_equipShield.bonusAttack + m_equipRing.bonusAttack + m_equipAmulet.bonusAttack;
    m_appliedBonusDefense = m_equipWeapon.bonusDefense + m_equipShield.bonusDefense + m_equipRing.bonusDefense + m_equipAmulet.bonusDefense;
    m_appliedBonusCrit = m_equipWeapon.bonusCrit + m_equipShield.bonusCrit + m_equipRing.bonusCrit + m_equipAmulet.bonusCrit;
    m_appliedBonusHp = m_equipWeapon.bonusHp + m_equipShield.bonusHp + m_equipRing.bonusHp + m_equipAmulet.bonusHp;
    m_appliedBonusMp = m_equipWeapon.bonusMp + m_equipShield.bonusMp + m_equipRing.bonusMp + m_equipAmulet.bonusMp;

    // 3. Apply new bonuses
    stats.Attack += m_appliedBonusAttack;
    stats.Defense += m_appliedBonusDefense;
    stats.CritChance += (float)m_appliedBonusCrit;
    stats.MaxHP += m_appliedBonusHp;
    stats.MaxMP += m_appliedBonusMp;

    if (stats.CurrentHP > stats.MaxHP) stats.CurrentHP = stats.MaxHP;
    if (stats.CurrentMP > stats.MaxMP) stats.CurrentMP = stats.MaxMP;
}

void InventorySystem::RenderWindow(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float mouseNdcX, float mouseNdcY) {
    if (!m_isOpen) return;

    float pW = 0.98f, pH = 1.54f;
    float pX = -pW * 0.5f, pY = -pH * 0.5f;

    UIRenderer::DrawWin98Window(uiProgram, uiVAO, uiVBO, pX, pY, pW, pH, "INVENTARIO_FAT16.EXE", true);

    float fSize = 0.026f;

    // Section 1: Backpack Slots Grid (3 Rows x 4 Cols)
    UIRenderer::DrawString("ALMACENAMIENTO (CLICK USAR/EQUIPAR):", pX + 0.04f, pY + pH - 0.12f, fSize, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);

    float slotW = 0.20f, slotH = 0.16f;
    float startX = pX + 0.04f, startY = pY + pH - 0.32f;
    float padX = 0.030f, padY = 0.030f;

    std::string hoveredDesc = "SELECCIONA UN OBJETO PARA VER DETALLES";

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            int idx = r * 4 + c;
            float sx = startX + c * (slotW + padX);
            float sy = startY - r * (slotH + padY);

            bool hovered = (mouseNdcX >= sx && mouseNdcX <= sx + slotW && mouseNdcY >= sy && mouseNdcY <= sy + slotH);
            const auto& item = m_slots[idx];

            if (hovered && item.type != ItemType::NONE) {
                hoveredDesc = item.name + ": " + item.desc;
            }

            // Slot Background Box
            glm::vec3 fillCol = hovered ? glm::vec3(0.88f, 0.88f, 0.92f) : glm::vec3(0.96f, 0.96f, 0.94f);
            UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, sx, sy, slotW, slotH, "", hovered, fSize);

            // Item Name & Count Text
            if (item.type != ItemType::NONE) {
                std::string shortName = item.name.substr(0, 7);
                UIRenderer::DrawString(shortName, sx + 0.015f, sy + slotH - 0.055f, 0.022f, glm::vec3(0.10f, 0.12f, 0.20f), uiProgram, uiVAO, uiVBO);
                if (item.count > 1) {
                    std::string cntStr = "x" + std::to_string(item.count);
                    UIRenderer::DrawString(cntStr, sx + slotW - 0.065f, sy + 0.020f, 0.024f, glm::vec3(0.80f, 0.20f, 0.15f), uiProgram, uiVAO, uiVBO);
                } else if (item.slot != EquipSlot::NONE) {
                    UIRenderer::DrawString("[EQ]", sx + slotW - 0.075f, sy + 0.020f, 0.022f, glm::vec3(0.15f, 0.60f, 0.25f), uiProgram, uiVAO, uiVBO);
                }
            } else {
                UIRenderer::DrawString("VACIO", sx + 0.045f, sy + (slotH - 0.022f) * 0.5f, 0.022f, glm::vec3(0.65f, 0.65f, 0.70f), uiProgram, uiVAO, uiVBO);
            }
        }
    }

    // Section 2: Equipment Slots Banner (Right side of window)
    float eqY = pY + 0.44f;
    UIRenderer::DrawString("EQUIPAMIENTO ACTIVO (CLICK DESEQUIPAR):", pX + 0.04f, eqY, fSize, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);

    float eqW = 0.21f, eqH = 0.14f;
    float eqStartX = pX + 0.04f;
    float eqPadX = 0.024f;

    auto drawEquipSlot = [&](int slotIndex, EquipSlot eSlot, const std::string& label, const InventoryItem& eqItem) {
        float ex = eqStartX + slotIndex * (eqW + eqPadX);
        float ey = eqY - eqH - 0.025f;
        bool hovered = (mouseNdcX >= ex && mouseNdcX <= ex + eqW && mouseNdcY >= ey && mouseNdcY <= ey + eqH);

        if (hovered && eqItem.type != ItemType::NONE) {
            hoveredDesc = eqItem.name + ": " + eqItem.desc;
        }

        UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, ex, ey, eqW, eqH, "", hovered, fSize);

        UIRenderer::DrawString(label, ex + 0.015f, ey + eqH - 0.045f, 0.020f, glm::vec3(0.40f, 0.40f, 0.45f), uiProgram, uiVAO, uiVBO);
        if (eqItem.type != ItemType::NONE) {
            std::string sName = eqItem.name.substr(0, 7);
            UIRenderer::DrawString(sName, ex + 0.015f, ey + 0.020f, 0.022f, glm::vec3(0.08f, 0.45f, 0.15f), uiProgram, uiVAO, uiVBO);
        } else {
            UIRenderer::DrawString("[LIBRE]", ex + 0.035f, ey + 0.020f, 0.020f, glm::vec3(0.60f, 0.60f, 0.65f), uiProgram, uiVAO, uiVBO);
        }
    };

    drawEquipSlot(0, EquipSlot::WEAPON, "ARMA", m_equipWeapon);
    drawEquipSlot(1, EquipSlot::SHIELD, "ESCUDO", m_equipShield);
    drawEquipSlot(2, EquipSlot::RING, "ANILLO", m_equipRing);
    drawEquipSlot(3, EquipSlot::AMULET, "AMULETO", m_equipAmulet);

    // Section 3: Item Inspector Info Bar
    float infoY = pY + 0.14f;
    float infoW = pW - 0.08f, infoH = 0.080f;
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, pX + 0.04f, infoY, infoW, infoH, "", false, fSize);
    UIRenderer::DrawString(hoveredDesc, pX + 0.06f, infoY + 0.026f, 0.024f, glm::vec3(0.10f, 0.10f, 0.15f), uiProgram, uiVAO, uiVBO);

    // Section 4: Footer Close Button
    float closeW = 0.42f, closeH = 0.075f;
    float closeX = pX + (pW - closeW) * 0.5f, closeY = pY + 0.025f;
    bool closeHov = (mouseNdcX >= closeX && mouseNdcX <= closeX + closeW && mouseNdcY >= closeY && mouseNdcY <= closeY + closeH);
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, closeX, closeY, closeW, closeH, "CERRAR INVENTARIO (I)", closeHov, 0.024f);
}

bool InventorySystem::HandleMouseClick(float mouseNdcX, float mouseNdcY, PlayerStats& stats, bool& closeRequested) {
    if (!m_isOpen) return false;

    float pW = 0.98f, pH = 1.54f;
    float pX = -pW * 0.5f, pY = -pH * 0.5f;

    // Check Close Button [X] in Title Bar
    float xBtnW = 0.045f, xBtnH = 0.045f;
    float xBtnX = pX + pW - xBtnW - 0.014f;
    float xBtnY = pY + pH - 0.065f - 0.008f + 0.010f;
    if (mouseNdcX >= xBtnX && mouseNdcX <= xBtnX + xBtnW && mouseNdcY >= xBtnY && mouseNdcY <= xBtnY + xBtnH) {
        closeRequested = true;
        return true;
    }

    // Check Footer [CERRAR INVENTARIO (I)] Button
    float closeW = 0.42f, closeH = 0.075f;
    float closeX = pX + (pW - closeW) * 0.5f, closeY = pY + 0.025f;
    if (mouseNdcX >= closeX && mouseNdcX <= closeX + closeW && mouseNdcY >= closeY && mouseNdcY <= closeY + closeH) {
        closeRequested = true;
        return true;
    }

    // Check Backpack Slots
    float slotW = 0.20f, slotH = 0.16f;
    float startX = pX + 0.04f, startY = pY + pH - 0.32f;
    float padX = 0.030f, padY = 0.030f;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            int idx = r * 4 + c;
            float sx = startX + c * (slotW + padX);
            float sy = startY - r * (slotH + padY);

            if (mouseNdcX >= sx && mouseNdcX <= sx + slotW && mouseNdcY >= sy && mouseNdcY <= sy + slotH) {
                UseOrEquipSlot(idx, stats);
                return true;
            }
        }
    }

    // Check Equip Slots (Click to Unequip)
    float eqY = pY + 0.44f;
    float eqW = 0.21f, eqH = 0.14f;
    float eqStartX = pX + 0.04f;
    float eqPadX = 0.024f;

    for (int i = 0; i < 4; ++i) {
        float ex = eqStartX + i * (eqW + eqPadX);
        float ey = eqY - eqH - 0.025f;

        if (mouseNdcX >= ex && mouseNdcX <= ex + eqW && mouseNdcY >= ey && mouseNdcY <= ey + eqH) {
            EquipSlot eSlot = (i == 0) ? EquipSlot::WEAPON : ((i == 1) ? EquipSlot::SHIELD : ((i == 2) ? EquipSlot::RING : EquipSlot::AMULET));
            UnequipSlot(eSlot, stats);
            return true;
        }
    }

    return false;
}
