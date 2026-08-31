#include "InventorySystem.h"
#include "Player.h"
#include "ui/UIRenderer.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

ItemId InventorySystem::mapLegacyType(ItemType type) const {
    switch (type) {
        case ItemType::POTION_HEALTH: return ItemRegistry::Get().FindId("potion_health");
        case ItemType::POTION_MANA:   return ItemRegistry::Get().FindId("potion_mana");
        case ItemType::BLOOD_VIAL:     return ItemRegistry::Get().FindId("blood_vial");
        case ItemType::RAW_MEAT:       return ItemRegistry::Get().FindId("raw_meat");
        case ItemType::BEAST_PELT:     return ItemRegistry::Get().FindId("beast_pelt");
        case ItemType::CURSED_SWORD:   return ItemRegistry::Get().FindId("cursed_sword");
        case ItemType::IRON_SHIELD:    return ItemRegistry::Get().FindId("iron_shield");
        case ItemType::SHADOW_RING:    return ItemRegistry::Get().FindId("shadow_ring");
        case ItemType::ANCIENT_AMULET: return ItemRegistry::Get().FindId("ancient_amulet");
        default: return INVALID_ITEM_ID;
    }
}

InventorySystem::InventorySystem()
    : m_inventory(16, 60.0f)
{
    // Pack Inicial del Aventurero
    m_inventory.AddItemByString("potion_health", 2);
    m_inventory.AddItemByString("potion_mana", 1);
    m_inventory.AddItemByString("blood_vial", 2);
    m_inventory.AddItemByString("shadow_ring", 1);
}

bool InventorySystem::AddItem(ItemType type, int count) {
    ItemId id = mapLegacyType(type);
    return m_inventory.AddItem(id, count);
}

bool InventorySystem::AddItem(ItemId id, int count) {
    return m_inventory.AddItem(id, count);
}

bool InventorySystem::AddItemByString(const std::string& stringId, int count) {
    return m_inventory.AddItemByString(stringId, count);
}

bool InventorySystem::UseOrEquipSlot(int slotIndex, Player* player, ParticleSystem* particles, DamageNumberSystem* damageNumbers) {
    if (slotIndex < 0 || slotIndex >= m_inventory.GetSlotCount() || player == nullptr) {
        return false;
    }

    ItemInstance& slotItem = m_inventory.GetSlot(slotIndex);
    if (!slotItem.IsValid()) return false;

    const ItemDefinition& def = ItemRegistry::Get().Get(slotItem.id);

    // 1. Si es Equipable -> Transferencia Atómica a Equipment
    if (def.IsEquippable()) {
        EquipResult res = m_equipment.EquipFromInventory(m_inventory, slotIndex);
        if (res == EquipResult::SUCCESS) {
            // Pipeline de recálculo idempotente
            player->Stats.RecalculateStats(m_equipment.CalculateTotalStats());
            return true;
        }
        return false;
    }

    // 2. Si es Consumible / Usable -> ItemActionSystem desacoplado
    if (def.isUsable) {
        ItemActionContext ctx;
        ctx.player = player;
        ctx.particles = particles;
        ctx.damageNumbers = damageNumbers;

        ItemActionResult actionRes = ItemActionSystem::ExecuteUse(slotItem, ctx);
        if (actionRes.success && actionRes.consumeItem) {
            m_inventory.RemoveItemAt(slotIndex, 1);
            return true;
        }
        return actionRes.success;
    }

    return false;
}

bool InventorySystem::UnequipSlot(EquipSlot slot, Player* player) {
    if (player == nullptr) return false;
    bool success = m_equipment.UnequipToInventory(m_inventory, slot);
    if (success) {
        player->Stats.RecalculateStats(m_equipment.CalculateTotalStats());
    }
    return success;
}

void InventorySystem::RecalculateBonuses(PlayerStats& stats) {
    stats.RecalculateStats(m_equipment.CalculateTotalStats());
}

void InventorySystem::RenderWindow(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float mouseNdcX, float mouseNdcY, const PlayerStats* playerStats) {
    if (!m_isOpen) return;

    float pW = 1.62f, pH = 1.48f;
    float pX = -pW * 0.5f, pY = -pH * 0.5f;

    UIRenderer::DrawWin98Window(uiProgram, uiVAO, uiVBO, pX, pY, pW, pH, "INVENTARIO Y EQUIPO - ARPG FAT16.EXE", true);

    float fSize = 0.024f;
    std::string hoveredDesc = "SELECCIONA UN OBJETO O RANURA PARA VER DETALLES";

    // -------------------------------------------------------------------------
    // PANEL IZQUIERDO: MOCHILA DE ALMACENAMIENTO (4x4 = 16 Slots)
    // -------------------------------------------------------------------------
    std::ostringstream weightStream;
    weightStream << std::fixed << std::setprecision(1) << m_inventory.GetCurrentWeight() << "/" << m_inventory.GetMaxWeight() << " KG";
    std::string bagHeader = "MOCHILA (16 CASILLAS) - PESO: " + weightStream.str();
    UIRenderer::DrawString(bagHeader, pX + 0.035f, pY + pH - 0.12f, fSize, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);

    float slotW = 0.170f, slotH = 0.140f;
    float startX = pX + 0.035f, startY = pY + pH - 0.28f;
    float padX = 0.018f, padY = 0.018f;

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            int idx = r * 4 + c;
            if (idx >= m_inventory.GetSlotCount()) break;

            float sx = startX + c * (slotW + padX);
            float sy = startY - r * (slotH + padY);

            bool hovered = (mouseNdcX >= sx && mouseNdcX <= sx + slotW && mouseNdcY >= sy && mouseNdcY <= sy + slotH);
            const auto& item = m_inventory.GetSlot(idx);

            UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, sx, sy, slotW, slotH, "", hovered, fSize);

            if (item.IsValid()) {
                const ItemDefinition& def = ItemRegistry::Get().Get(item.id);

                if (hovered) {
                    hoveredDesc = def.name + " (" + def.description + ")";
                }

                // Color por rareza
                glm::vec3 textCol = glm::vec3(0.12f, 0.12f, 0.18f);
                if (def.rarity == ItemRarity::UNCOMMON) textCol = glm::vec3(0.10f, 0.55f, 0.15f);
                else if (def.rarity == ItemRarity::RARE) textCol = glm::vec3(0.10f, 0.35f, 0.85f);
                else if (def.rarity == ItemRarity::EPIC) textCol = glm::vec3(0.60f, 0.15f, 0.80f);
                else if (def.rarity == ItemRarity::LEGENDARY) textCol = glm::vec3(0.85f, 0.55f, 0.05f);

                std::string shortName = def.name.substr(0, 7);
                UIRenderer::DrawString(shortName, sx + 0.012f, sy + slotH - 0.050f, 0.022f, textCol, uiProgram, uiVAO, uiVBO);

                if (item.quantity > 1) {
                    std::string cntStr = "x" + std::to_string(item.quantity);
                    UIRenderer::DrawString(cntStr, sx + slotW - 0.060f, sy + 0.015f, 0.022f, glm::vec3(0.80f, 0.20f, 0.15f), uiProgram, uiVAO, uiVBO);
                } else if (def.IsEquippable()) {
                    UIRenderer::DrawString("[EQ]", sx + slotW - 0.065f, sy + 0.015f, 0.020f, glm::vec3(0.15f, 0.60f, 0.25f), uiProgram, uiVAO, uiVBO);
                }
            } else {
                UIRenderer::DrawString("VACIO", sx + 0.035f, sy + (slotH - 0.022f) * 0.5f, 0.020f, glm::vec3(0.65f, 0.65f, 0.70f), uiProgram, uiVAO, uiVBO);
            }
        }
    }

    // -------------------------------------------------------------------------
    // PANEL DERECHO: EQUIPAMIENTO ACTIVO & HOJA DE ATRIBUTOS
    // -------------------------------------------------------------------------
    float eqPanelX = pX + 0.81f;
    UIRenderer::DrawString("EQUIPAMIENTO & ATRIBUTOS:", eqPanelX, pY + pH - 0.12f, fSize, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);

    // Ranuras de Equipamiento
    struct EqSlotUI {
        EquipSlot slot;
        std::string label;
        float rx, ry, rw, rh;
    };

    float eqSlotW = 0.235f, eqSlotH = 0.115f;
    float eqStartY = pY + pH - 0.26f;

    EqSlotUI eqSlots[] = {
        { EquipSlot::HEAD,      "CABEZA",   eqPanelX,            eqStartY,                    eqSlotW, eqSlotH },
        { EquipSlot::CHEST,     "PECHO",    eqPanelX + eqSlotW + 0.02f, eqStartY,             eqSlotW, eqSlotH },
        { EquipSlot::MAIN_HAND, "MANO 1",   eqPanelX,            eqStartY - (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::OFF_HAND,  "MANO 2",   eqPanelX + eqSlotW + 0.02f, eqStartY - (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::RING_1,    "ANILLO 1", eqPanelX,            eqStartY - 2.0f * (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::RING_2,    "ANILLO 2", eqPanelX + eqSlotW + 0.02f, eqStartY - 2.0f * (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::AMULET,    "AMULETO",  eqPanelX,            eqStartY - 3.0f * (eqSlotH + 0.015f), eqSlotW * 2.0f + 0.02f, eqSlotH }
    };

    for (const auto& es : eqSlots) {
        bool hovered = (mouseNdcX >= es.rx && mouseNdcX <= es.rx + es.rw && mouseNdcY >= es.ry && mouseNdcY <= es.ry + es.rh);
        const ItemInstance& eqItem = m_equipment.GetEquipped(es.slot);
        bool isBlocked = m_equipment.IsSlotBlocked(es.slot);

        if (hovered && eqItem.IsValid()) {
            const ItemDefinition& def = ItemRegistry::Get().Get(eqItem.id);
            hoveredDesc = def.name + " (" + def.description + ")";
        }

        UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, es.rx, es.ry, es.rw, es.rh, "", hovered, fSize);
        UIRenderer::DrawString(es.label, es.rx + 0.012f, es.ry + es.rh - 0.040f, 0.018f, glm::vec3(0.40f, 0.40f, 0.45f), uiProgram, uiVAO, uiVBO);

        if (eqItem.IsValid()) {
            const ItemDefinition& def = ItemRegistry::Get().Get(eqItem.id);
            std::string sName = def.name.substr(0, 10);
            UIRenderer::DrawString(sName, es.rx + 0.012f, es.ry + 0.015f, 0.022f, glm::vec3(0.08f, 0.48f, 0.15f), uiProgram, uiVAO, uiVBO);
        } else if (isBlocked) {
            UIRenderer::DrawString("[BLOQUEADO 2H]", es.rx + 0.012f, es.ry + 0.015f, 0.018f, glm::vec3(0.75f, 0.20f, 0.15f), uiProgram, uiVAO, uiVBO);
        } else {
            UIRenderer::DrawString("[LIBRE]", es.rx + 0.035f, es.ry + 0.015f, 0.018f, glm::vec3(0.60f, 0.60f, 0.65f), uiProgram, uiVAO, uiVBO);
        }
    }

    // Cuadro de Resumen de Estadísticas Totales (Inferior Derecho)
    float statsY = pY + 0.22f;
    float statsW = eqSlotW * 2.0f + 0.02f, statsH = 0.24f;
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, eqPanelX, statsY, statsW, statsH, "", false, fSize);

    EquipmentStats totalEq = m_equipment.CalculateTotalStats();
    int atkVal = (playerStats ? playerStats->Attack : 45);
    int defVal = (playerStats ? playerStats->Defense : 8);
    float critVal = (playerStats ? playerStats->CritChance : 12.0f);
    int hpVal = (playerStats ? playerStats->MaxHP : 120);
    int mpVal = (playerStats ? playerStats->MaxMP : 50);

    std::ostringstream s1, s2, s3;
    s1 << "ATQ: " << atkVal << " (+" << totalEq.attackPower << ") | DEF: " << defVal << " (+" << totalEq.defense << ")";
    s2 << "CRIT: " << std::fixed << std::setprecision(1) << critVal << "% (+" << totalEq.critChance << "%)";
    s3 << "HP MAX: " << hpVal << " | MP MAX: " << mpVal;

    UIRenderer::DrawString(s1.str(), eqPanelX + 0.015f, statsY + statsH - 0.055f, 0.020f, glm::vec3(0.10f, 0.12f, 0.20f), uiProgram, uiVAO, uiVBO);
    UIRenderer::DrawString(s2.str(), eqPanelX + 0.015f, statsY + statsH - 0.115f, 0.020f, glm::vec3(0.70f, 0.25f, 0.10f), uiProgram, uiVAO, uiVBO);
    UIRenderer::DrawString(s3.str(), eqPanelX + 0.015f, statsY + statsH - 0.175f, 0.020f, glm::vec3(0.12f, 0.45f, 0.65f), uiProgram, uiVAO, uiVBO);

    // -------------------------------------------------------------------------
    // BARRA INFERIOR DE INSPECCIÓN Y BOTÓN CERRAR
    // -------------------------------------------------------------------------
    float infoY = pY + 0.10f;
    float infoW = pW - 0.07f, infoH = 0.075f;
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, pX + 0.035f, infoY, infoW, infoH, "", false, fSize);
    UIRenderer::DrawString(hoveredDesc, pX + 0.05f, infoY + 0.024f, 0.022f, glm::vec3(0.10f, 0.10f, 0.15f), uiProgram, uiVAO, uiVBO);

    float closeW = 0.38f, closeH = 0.065f;
    float closeX = pX + (pW - closeW) * 0.5f, closeY = pY + 0.020f;
    bool closeHov = (mouseNdcX >= closeX && mouseNdcX <= closeX + closeW && mouseNdcY >= closeY && mouseNdcY <= closeY + closeH);
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, closeX, closeY, closeW, closeH, "CERRAR (I)", closeHov, 0.022f);
}

bool InventorySystem::HandleMouseClick(float mouseNdcX, float mouseNdcY, Player* player, ParticleSystem* particles, DamageNumberSystem* damageNumbers, bool& closeRequested) {
    if (!m_isOpen || player == nullptr) return false;

    float pW = 1.62f, pH = 1.48f;
    float pX = -pW * 0.5f, pY = -pH * 0.5f;

    // Botón de Cerrar [X] en barra de título
    float xBtnW = 0.045f, xBtnH = 0.045f;
    float xBtnX = pX + pW - xBtnW - 0.014f;
    float xBtnY = pY + pH - 0.065f - 0.008f + 0.010f;
    if (mouseNdcX >= xBtnX && mouseNdcX <= xBtnX + xBtnW && mouseNdcY >= xBtnY && mouseNdcY <= xBtnY + xBtnH) {
        closeRequested = true;
        return true;
    }

    // Botón inferior [CERRAR (I)]
    float closeW = 0.38f, closeH = 0.065f;
    float closeX = pX + (pW - closeW) * 0.5f, closeY = pY + 0.020f;
    if (mouseNdcX >= closeX && mouseNdcX <= closeX + closeW && mouseNdcY >= closeY && mouseNdcY <= closeY + closeH) {
        closeRequested = true;
        return true;
    }

    // Clic en Ranuras de Mochila (4x4 = 16)
    float slotW = 0.170f, slotH = 0.140f;
    float startX = pX + 0.035f, startY = pY + pH - 0.28f;
    float padX = 0.018f, padY = 0.018f;

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            int idx = r * 4 + c;
            if (idx >= m_inventory.GetSlotCount()) break;

            float sx = startX + c * (slotW + padX);
            float sy = startY - r * (slotH + padY);

            if (mouseNdcX >= sx && mouseNdcX <= sx + slotW && mouseNdcY >= sy && mouseNdcY <= sy + slotH) {
                UseOrEquipSlot(idx, player, particles, damageNumbers);
                return true;
            }
        }
    }

    // Clic en Ranuras de Equipo (Desequipar)
    float eqPanelX = pX + 0.81f;
    float eqSlotW = 0.235f, eqSlotH = 0.115f;
    float eqStartY = pY + pH - 0.26f;

    struct EqSlotClick {
        EquipSlot slot;
        float rx, ry, rw, rh;
    };

    EqSlotClick eqClicks[] = {
        { EquipSlot::HEAD,      eqPanelX,            eqStartY,                    eqSlotW, eqSlotH },
        { EquipSlot::CHEST,     eqPanelX + eqSlotW + 0.02f, eqStartY,             eqSlotW, eqSlotH },
        { EquipSlot::MAIN_HAND, eqPanelX,            eqStartY - (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::OFF_HAND,  eqPanelX + eqSlotW + 0.02f, eqStartY - (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::RING_1,    eqPanelX,            eqStartY - 2.0f * (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::RING_2,    eqPanelX + eqSlotW + 0.02f, eqStartY - 2.0f * (eqSlotH + 0.015f), eqSlotW, eqSlotH },
        { EquipSlot::AMULET,    eqPanelX,            eqStartY - 3.0f * (eqSlotH + 0.015f), eqSlotW * 2.0f + 0.02f, eqSlotH }
    };

    for (const auto& ec : eqClicks) {
        if (mouseNdcX >= ec.rx && mouseNdcX <= ec.rx + ec.rw && mouseNdcY >= ec.ry && mouseNdcY <= ec.ry + ec.rh) {
            UnequipSlot(ec.slot, player);
            return true;
        }
    }

    return false;
}
