#include "InventorySystem.h"
#include "ItemModelRegistry.h"
#include "Player.h"
#include "ParticleSystem.h"
#include "world/ItemDropSystem.h"
#include "ui/UIRenderer.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
    : m_inventory(30, 160.0f)
{
    // El jugador empieza con el inventario completamente vacío
    ItemModelRegistry::Get().Init();
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

#include "world/ItemDropSystem.h"

bool InventorySystem::DropSlot(int slotIndex, Player* player, ItemDropSystem* itemDropSystem, ParticleSystem* particles) {
    if (slotIndex < 0 || slotIndex >= m_inventory.GetSlotCount() || player == nullptr) return false;

    ItemInstance item = m_inventory.GetSlot(slotIndex);
    if (!item.IsValid()) return false;

    ItemInstance toDrop = item;
    toDrop.quantity = 1;

    if (m_inventory.RemoveItemAt(slotIndex, 1)) {
        glm::vec3 throwFwd = glm::vec3(sin(glm::radians(player->ModelYaw)), 0.0f, cos(glm::radians(player->ModelYaw)));
        if (glm::length(throwFwd) < 0.01f) throwFwd = player->Front;
        throwFwd.y = 0.0f;
        if (glm::length(throwFwd) > 0.01f) throwFwd = glm::normalize(throwFwd);

        glm::vec3 dropPos = player->Position + throwFwd * 1.6f + glm::vec3(0.0f, 0.8f, 0.0f);
        glm::vec3 throwVel = throwFwd * 4.5f + glm::vec3(0.0f, 3.6f, 0.0f);

        if (itemDropSystem != nullptr) {
            itemDropSystem->SpawnDrop(toDrop, dropPos, throwVel, 2.0f);
        }
        if (particles != nullptr) {
            for (int i = 0; i < 16; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.8f, (rand()%100/50.0f + 0.5f)*2.0f, (rand()%100/50.0f - 1.0f)*1.8f);
                particles->SpawnParticle(dropPos, pVel, glm::vec4(0.95f, 0.75f, 0.15f, 1.0f), 0.14f, 0.6f, -9.8f);
            }
        }
        if (!m_inventory.GetSlot(slotIndex).IsValid()) {
            m_selectedSlot = -1;
        }
        return true;
    }
    return false;
}

void InventorySystem::UpdateDrag(float mouseNdcX, float mouseNdcY, bool isLeftMouseDown) {
    if (!m_isOpen) {
        m_isDragging = false;
        return;
    }

    float pW = 1.62f, pH = 1.48f;

    if (!isLeftMouseDown) {
        m_isDragging = false;
        return;
    }

    if (m_isDragging) {
        m_winX = mouseNdcX - m_dragOffsetX;
        m_winY = mouseNdcY - m_dragOffsetY;

        // Limitar dentro de la pantalla
        float minX = -1.0f;
        float maxX = 1.0f - pW;
        float minY = -1.0f;
        float maxY = 1.0f - pH;
        m_winX = std::clamp(m_winX, minX, maxX);
        m_winY = std::clamp(m_winY, minY, maxY);
    } else {
        // Verificar si el clic comenzó en la barra de título superior
        float tbX = m_winX;
        float tbY = m_winY + pH - 0.075f;
        float tbW = pW - 0.065f; // excluir botón [X]
        float tbH = 0.075f;

        if (mouseNdcX >= tbX && mouseNdcX <= tbX + tbW && mouseNdcY >= tbY && mouseNdcY <= tbY + tbH) {
            m_isDragging = true;
            m_dragOffsetX = mouseNdcX - m_winX;
            m_dragOffsetY = mouseNdcY - m_winY;
        }
    }
}

void InventorySystem::RenderWindow(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float mouseNdcX, float mouseNdcY, const PlayerStats* playerStats) {
    if (!m_isOpen) return;

    float pW = 1.62f, pH = 1.48f;
    float pX = m_winX, pY = m_winY;

    UIRenderer::DrawWin98Window(uiProgram, uiVAO, uiVBO, pX, pY, pW, pH, "INVENTARIO Y EQUIPO - VRAM DUNGEON FAT16.EXE", true);

    float fSize = 0.024f;
    std::string hoveredDesc = "SELECCIONA UN OBJETO O RANURA PARA VER DETALLES / TIRAR AL SUELO";

    m_hasHoveredItem = false;
    m_hoveredItemStringId = "";
    m_hoveredItemId = INVALID_ITEM_ID;

    // -------------------------------------------------------------------------
    // PANEL IZQUIERDO: MOCHILA DE ALMACENAMIENTO (5x6 = 30 Slots)
    // -------------------------------------------------------------------------
    std::ostringstream weightStream;
    weightStream << std::fixed << std::setprecision(1) << m_inventory.GetCurrentWeight() << "/" << m_inventory.GetMaxWeight() << " KG";
    std::string bagHeader = "MOCHILA (30 CASILLAS) - PESO: " + weightStream.str();
    UIRenderer::DrawString(bagHeader, pX + 0.035f, pY + pH - 0.12f, fSize, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);

    float slotW = 0.138f, slotH = 0.125f;
    float startX = pX + 0.035f, startY = pY + pH - 0.25f;
    float padX = 0.012f, padY = 0.014f;

    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 5; ++c) {
            int idx = r * 5 + c;
            if (idx >= m_inventory.GetSlotCount()) break;

            float sx = startX + c * (slotW + padX);
            float sy = startY - r * (slotH + padY);

            bool hovered = (mouseNdcX >= sx && mouseNdcX <= sx + slotW && mouseNdcY >= sy && mouseNdcY <= sy + slotH);
            bool isSelected = (m_selectedSlot == idx);
            const auto& item = m_inventory.GetSlot(idx);

            UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, sx, sy, slotW, slotH, "", hovered || isSelected, fSize);

            if (item.IsValid()) {
                const ItemDefinition& def = ItemRegistry::Get().Get(item.id);

                if (hovered) {
                    m_hasHoveredItem = true;
                    m_hoveredItemStringId = def.stringId;
                    m_hoveredItemId = item.id;
                    hoveredDesc = def.name + " (" + def.description + ")";
                } else if (isSelected && !m_hasHoveredItem) {
                    hoveredDesc = def.name + " (" + def.description + ")";
                }

                // Color por rareza real
                glm::vec3 textCol = glm::vec3(0.12f, 0.12f, 0.18f);
                if (def.rarity == ItemRarity::UNCOMMON) textCol = glm::vec3(0.10f, 0.55f, 0.15f);
                else if (def.rarity == ItemRarity::RARE) textCol = glm::vec3(0.10f, 0.35f, 0.85f);
                else if (def.rarity == ItemRarity::EPIC) textCol = glm::vec3(0.60f, 0.15f, 0.80f);
                else if (def.rarity == ItemRarity::LEGENDARY) textCol = glm::vec3(0.85f, 0.55f, 0.05f);

                std::string shortName = def.name.substr(0, 6);
                UIRenderer::DrawString(shortName, sx + 0.008f, sy + slotH - 0.038f, 0.017f, textCol, uiProgram, uiVAO, uiVBO);

                if (isSelected) {
                    UIRenderer::DrawString("[SEL]", sx + slotW - 0.055f, sy + slotH - 0.038f, 0.016f, glm::vec3(0.85f, 0.45f, 0.05f), uiProgram, uiVAO, uiVBO);
                }

                if (item.quantity > 1) {
                    std::string cntStr = "x" + std::to_string(item.quantity);
                    UIRenderer::DrawString(cntStr, sx + slotW - 0.048f, sy + 0.010f, 0.018f, glm::vec3(0.85f, 0.15f, 0.10f), uiProgram, uiVAO, uiVBO);
                } else if (def.IsEquippable()) {
                    UIRenderer::DrawString("[EQ]", sx + slotW - 0.050f, sy + 0.010f, 0.016f, glm::vec3(0.15f, 0.60f, 0.25f), uiProgram, uiVAO, uiVBO);
                }
            } else {
                UIRenderer::DrawString("---", sx + 0.048f, sy + (slotH - 0.018f) * 0.5f, 0.018f, glm::vec3(0.65f, 0.65f, 0.70f), uiProgram, uiVAO, uiVBO);
            }
        }
    }

    // -------------------------------------------------------------------------
    // PANEL DERECHO: EQUIPAMIENTO ANATOMICO COMPLETO (ITEMS 3D INTERACTIVOS)
    // -------------------------------------------------------------------------
    float eqPanelX = pX + 0.81f;
    UIRenderer::DrawString("EQUIPAMIENTO ACTIVO (3D POR CASILLA):", eqPanelX, pY + pH - 0.12f, fSize, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);

    float eqSlotW = 0.242f, eqSlotH = 0.118f;
    float eqStartY = pY + pH - 0.25f;
    float colGap = 0.018f;
    float col1X = eqPanelX;
    float col2X = eqPanelX + eqSlotW + colGap;
    float col3X = eqPanelX + (eqSlotW + colGap) * 2.0f;

    float row0Y = eqStartY;
    float row1Y = eqStartY - (eqSlotH + 0.016f);
    float row2Y = eqStartY - 2.0f * (eqSlotH + 0.016f);
    float row3Y = eqStartY - 3.0f * (eqSlotH + 0.016f);

    struct EqSlotUI {
        EquipSlot slot;
        std::string label;
        float rx, ry, rw, rh;
    };

    EqSlotUI eqSlots[] = {
        { EquipSlot::HEAD,      "CABEZA",    col2X, row0Y, eqSlotW, eqSlotH },
        { EquipSlot::MAIN_HAND, "MANO 1",    col1X, row1Y, eqSlotW, eqSlotH },
        { EquipSlot::CHEST,     "PECHO",     col2X, row1Y, eqSlotW, eqSlotH },
        { EquipSlot::OFF_HAND,  "MANO 2",    col3X, row1Y, eqSlotW, eqSlotH },
        { EquipSlot::GLOVES,    "GUANTES",   col1X, row2Y, eqSlotW, eqSlotH },
        { EquipSlot::LEGS,      "PANTALON",  col2X, row2Y, eqSlotW, eqSlotH },
        { EquipSlot::AMULET,    "AMULETO",   col3X, row2Y, eqSlotW, eqSlotH },
        { EquipSlot::RING_1,    "ANILLO 1",  col1X, row3Y, eqSlotW, eqSlotH },
        { EquipSlot::FEET,      "BOTAS",     col2X, row3Y, eqSlotW, eqSlotH },
        { EquipSlot::RING_2,    "ANILLO 2",  col3X, row3Y, eqSlotW, eqSlotH }
    };

    for (const auto& es : eqSlots) {
        bool hovered = (mouseNdcX >= es.rx && mouseNdcX <= es.rx + es.rw && mouseNdcY >= es.ry && mouseNdcY <= es.ry + es.rh);
        const ItemInstance& eqItem = m_equipment.GetEquipped(es.slot);
        bool isBlocked = m_equipment.IsSlotBlocked(es.slot);

        if (hovered && eqItem.IsValid()) {
            const ItemDefinition& def = ItemRegistry::Get().Get(eqItem.id);
            m_hasHoveredItem = true;
            m_hoveredItemStringId = def.stringId;
            m_hoveredItemId = eqItem.id;
            hoveredDesc = def.name + " (" + def.description + ")";
        }

        UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, es.rx, es.ry, es.rw, es.rh, "", hovered, fSize);
        UIRenderer::DrawString(es.label, es.rx + 0.010f, es.ry + es.rh - 0.035f, 0.016f, glm::vec3(0.40f, 0.40f, 0.45f), uiProgram, uiVAO, uiVBO);

        if (eqItem.IsValid()) {
            const ItemDefinition& def = ItemRegistry::Get().Get(eqItem.id);
            std::string sName = def.name.substr(0, 10);

            // MANTENER LA CALIDAD REAL DEL ITEM (No todo verde)
            glm::vec3 rarityCol = glm::vec3(0.92f, 0.92f, 0.95f); // COMMON
            if (def.rarity == ItemRarity::UNCOMMON) rarityCol = glm::vec3(0.15f, 0.85f, 0.25f);
            else if (def.rarity == ItemRarity::RARE) rarityCol = glm::vec3(0.20f, 0.55f, 1.0f);
            else if (def.rarity == ItemRarity::EPIC) rarityCol = glm::vec3(0.85f, 0.25f, 0.95f);
            else if (def.rarity == ItemRarity::LEGENDARY) rarityCol = glm::vec3(1.0f, 0.75f, 0.10f);

            UIRenderer::DrawString(sName, es.rx + 0.010f, es.ry + 0.012f, 0.018f, rarityCol, uiProgram, uiVAO, uiVBO);
        } else if (isBlocked) {
            UIRenderer::DrawString("[BLOQ 2H]", es.rx + 0.010f, es.ry + 0.012f, 0.016f, glm::vec3(0.75f, 0.20f, 0.15f), uiProgram, uiVAO, uiVBO);
        } else {
            UIRenderer::DrawString("[LIBRE]", es.rx + 0.020f, es.ry + 0.012f, 0.016f, glm::vec3(0.60f, 0.60f, 0.65f), uiProgram, uiVAO, uiVBO);
        }
    }

    // Cuadro de Resumen de Estadísticas Totales (Inferior Derecho)
    float statsY = pY + 0.18f;
    float statsW = 0.76f, statsH = 0.17f;
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

    UIRenderer::DrawString(s1.str(), eqPanelX + 0.015f, statsY + statsH - 0.045f, 0.019f, glm::vec3(0.10f, 0.12f, 0.20f), uiProgram, uiVAO, uiVBO);
    UIRenderer::DrawString(s2.str(), eqPanelX + 0.015f, statsY + statsH - 0.090f, 0.019f, glm::vec3(0.70f, 0.25f, 0.10f), uiProgram, uiVAO, uiVBO);
    UIRenderer::DrawString(s3.str(), eqPanelX + 0.015f, statsY + statsH - 0.135f, 0.019f, glm::vec3(0.12f, 0.45f, 0.65f), uiProgram, uiVAO, uiVBO);

    // -------------------------------------------------------------------------
    // BARRA INFERIOR DE INSPECCIÓN Y BOTONES DE ACCIÓN
    // -------------------------------------------------------------------------
    float infoY = pY + 0.095f;
    float infoW = pW - 0.07f, infoH = 0.075f;
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, pX + 0.035f, infoY, infoW, infoH, "", false, fSize);
    UIRenderer::DrawString(hoveredDesc, pX + 0.05f, infoY + 0.024f, 0.022f, glm::vec3(0.10f, 0.10f, 0.15f), uiProgram, uiVAO, uiVBO);

    // Botón [TIRAR OBJETO (DROP)]
    float dropW = 0.42f, dropH = 0.065f;
    float dropX = pX + 0.035f, dropY = pY + 0.020f;
    bool dropHov = (mouseNdcX >= dropX && mouseNdcX <= dropX + dropW && mouseNdcY >= dropY && mouseNdcY <= dropY + dropH);
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, dropX, dropY, dropW, dropH, "TIRAR OBJETO (DROP)", dropHov, 0.020f);

    // Botón [USAR / EQUIPAR]
    float useW = 0.52f, useH = 0.065f;
    float useX = dropX + dropW + 0.035f, useY = pY + 0.020f;
    bool useHov = (mouseNdcX >= useX && mouseNdcX <= useX + useW && mouseNdcY >= useY && mouseNdcY <= useY + useH);
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, useX, useY, useW, useH, "USAR / EQUIPAR OBJETO", useHov, 0.020f);

    // Botón [CERRAR (I)]
    float closeW = 0.48f, closeH = 0.065f;
    float closeX = pX + pW - closeW - 0.035f, closeY = pY + 0.020f;
    bool closeHov = (mouseNdcX >= closeX && mouseNdcX <= closeX + closeW && mouseNdcY >= closeY && mouseNdcY <= closeY + closeH);
    UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, closeX, closeY, closeW, closeH, "CERRAR VENTANA (I)", closeHov, 0.020f);

    // -------------------------------------------------------------------------
    // TOOLTIP 3D DE INSPECCIÓN FLOTANTE (ESTILO MU ONLINE / DIABLO)
    // -------------------------------------------------------------------------
    if (m_hasHoveredItem && m_hoveredItemId != INVALID_ITEM_ID) {
        const ItemDefinition& def = ItemRegistry::Get().Get(m_hoveredItemId);
        float tipW = 0.44f, tipH = 0.60f;
        float tipX = mouseNdcX + 0.04f;
        if (tipX + tipW > 0.98f) tipX = mouseNdcX - tipW - 0.04f;
        float tipY = mouseNdcY - tipH * 0.45f;
        if (tipY < -0.96f) tipY = -0.96f;
        if (tipY + tipH > 0.96f) tipY = 0.96f - tipH;

        // Marco de ventana Win98 para la ficha del ítem
        UIRenderer::DrawWin98Window(uiProgram, uiVAO, uiVBO, tipX, tipY, tipW, tipH, def.name, false);

        // Vitrina 3D Inset
        m_tipShowcaseX = tipX + 0.025f;
        m_tipShowcaseY = tipY + tipH - 0.35f;
        m_tipShowcaseW = tipW - 0.05f;
        m_tipShowcaseH = 0.27f;

        UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, m_tipShowcaseX, m_tipShowcaseY, m_tipShowcaseW, m_tipShowcaseH, "", false, fSize);
        UIRenderer::drawColoredQuad(uiProgram, uiVAO, uiVBO, m_tipShowcaseX + 0.006f, m_tipShowcaseY + 0.006f, m_tipShowcaseW - 0.012f, m_tipShowcaseH - 0.012f, glm::vec3(0.03f, 0.04f, 0.07f));
        UIRenderer::DrawString("3D INSPECT", m_tipShowcaseX + 0.012f, m_tipShowcaseY + m_tipShowcaseH - 0.026f, 0.015f, glm::vec3(0.40f, 0.65f, 0.90f), uiProgram, uiVAO, uiVBO);

        // Rarity Badge
        glm::vec3 rCol = glm::vec3(0.92f, 0.92f, 0.95f);
        std::string rLabel = "[COMUN]";
        if (def.rarity == ItemRarity::UNCOMMON) { rCol = glm::vec3(0.15f, 0.85f, 0.25f); rLabel = "[POCO COMUN]"; }
        else if (def.rarity == ItemRarity::RARE) { rCol = glm::vec3(0.20f, 0.55f, 1.0f); rLabel = "[RARO]"; }
        else if (def.rarity == ItemRarity::EPIC) { rCol = glm::vec3(0.85f, 0.25f, 0.95f); rLabel = "[EPICO]"; }
        else if (def.rarity == ItemRarity::LEGENDARY) { rCol = glm::vec3(1.0f, 0.75f, 0.10f); rLabel = "[LEGENDARIO]"; }

        UIRenderer::DrawString(rLabel, tipX + 0.025f, tipY + tipH - 0.385f, 0.018f, rCol, uiProgram, uiVAO, uiVBO);

        std::string typeStr = "TIPO: " + def.name.substr(0, 18);
        if (def.IsEquippable()) typeStr = "EQUIPABLE: " + def.name.substr(0, 16);
        else if (def.isUsable) typeStr = "TIPO: CONSUMIBLE";
        else typeStr = "TIPO: RECURSO / MISC";
        UIRenderer::DrawString(typeStr, tipX + 0.025f, tipY + tipH - 0.420f, 0.016f, glm::vec3(0.12f, 0.14f, 0.22f), uiProgram, uiVAO, uiVBO);

        if (def.equipStats.attackPower > 0) {
            std::string s = "ATQ: +" + std::to_string(def.equipStats.attackPower) + " | CRIT: +" + std::to_string((int)def.equipStats.critChance) + "%";
            UIRenderer::DrawString(s, tipX + 0.025f, tipY + tipH - 0.455f, 0.017f, glm::vec3(0.85f, 0.25f, 0.10f), uiProgram, uiVAO, uiVBO);
        } else if (def.equipStats.defense > 0) {
            std::string s = "DEF: +" + std::to_string(def.equipStats.defense) + " | HP: +" + std::to_string(def.equipStats.maxHpBonus);
            UIRenderer::DrawString(s, tipX + 0.025f, tipY + tipH - 0.455f, 0.017f, glm::vec3(0.15f, 0.45f, 0.85f), uiProgram, uiVAO, uiVBO);
        } else if (!def.effects.empty()) {
            std::string s = "EFECTO: RESTAURA " + std::to_string((int)def.effects[0].magnitude);
            if (def.effects[0].type == ItemEffectType::RESTORE_HP) s += " HP";
            else if (def.effects[0].type == ItemEffectType::RESTORE_MP) s += " MP";
            UIRenderer::DrawString(s, tipX + 0.025f, tipY + tipH - 0.455f, 0.017f, glm::vec3(0.10f, 0.65f, 0.25f), uiProgram, uiVAO, uiVBO);
        }

        std::string desc = def.description.substr(0, 30);
        UIRenderer::DrawString(desc, tipX + 0.025f, tipY + tipH - 0.495f, 0.015f, glm::vec3(0.35f, 0.35f, 0.40f), uiProgram, uiVAO, uiVBO);
        if (def.description.length() > 30) {
            std::string desc2 = def.description.substr(30, 30);
            UIRenderer::DrawString(desc2, tipX + 0.025f, tipY + tipH - 0.525f, 0.015f, glm::vec3(0.35f, 0.35f, 0.40f), uiProgram, uiVAO, uiVBO);
        }

        std::ostringstream ws;
        ws << "PESO: " << std::fixed << std::setprecision(1) << def.weight << " KG";
        UIRenderer::DrawString(ws.str(), tipX + 0.025f, tipY + 0.025f, 0.015f, glm::vec3(0.40f, 0.40f, 0.45f), uiProgram, uiVAO, uiVBO);
    }
}

bool InventorySystem::HandleMouseClick(float mouseNdcX, float mouseNdcY, Player* player, ParticleSystem* particles, DamageNumberSystem* damageNumbers, ItemDropSystem* itemDropSystem, bool& closeRequested) {
    if (!m_isOpen || player == nullptr) return false;

    float pW = 1.62f, pH = 1.48f;
    float pX = m_winX, pY = m_winY;

    // Botón de Cerrar [X] en barra de título
    float xBtnW = 0.045f, xBtnH = 0.045f;
    float xBtnX = pX + pW - xBtnW - 0.014f;
    float xBtnY = pY + pH - 0.065f - 0.008f + 0.010f;
    if (mouseNdcX >= xBtnX && mouseNdcX <= xBtnX + xBtnW && mouseNdcY >= xBtnY && mouseNdcY <= xBtnY + xBtnH) {
        closeRequested = true;
        return true;
    }

    // Botón [TIRAR OBJETO (DROP)]
    float dropW = 0.42f, dropH = 0.065f;
    float dropX = pX + 0.035f, dropY = pY + 0.020f;
    if (mouseNdcX >= dropX && mouseNdcX <= dropX + dropW && mouseNdcY >= dropY && mouseNdcY <= dropY + dropH) {
        if (m_selectedSlot >= 0 && m_selectedSlot < m_inventory.GetSlotCount()) {
            DropSlot(m_selectedSlot, player, itemDropSystem, particles);
            return true;
        }
    }

    // Botón [USAR / EQUIPAR OBJETO]
    float useW = 0.52f, useH = 0.065f;
    float useX = dropX + dropW + 0.035f, useY = pY + 0.020f;
    if (mouseNdcX >= useX && mouseNdcX <= useX + useW && mouseNdcY >= useY && mouseNdcY <= useY + useH) {
        if (m_selectedSlot >= 0 && m_selectedSlot < m_inventory.GetSlotCount()) {
            UseOrEquipSlot(m_selectedSlot, player, particles, damageNumbers);
            return true;
        }
    }

    // Botón inferior [CERRAR VENTANA (I)]
    float closeW = 0.48f, closeH = 0.065f;
    float closeX = pX + pW - closeW - 0.035f, closeY = pY + 0.020f;
    if (mouseNdcX >= closeX && mouseNdcX <= closeX + closeW && mouseNdcY >= closeY && mouseNdcY <= closeY + closeH) {
        closeRequested = true;
        return true;
    }

    // Clic en Ranuras de Mochila (5x6 = 30)
    float slotW = 0.138f, slotH = 0.125f;
    float startX = pX + 0.035f, startY = pY + pH - 0.25f;
    float padX = 0.012f, padY = 0.014f;

    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 5; ++c) {
            int idx = r * 5 + c;
            if (idx >= m_inventory.GetSlotCount()) break;

            float sx = startX + c * (slotW + padX);
            float sy = startY - r * (slotH + padY);

            if (mouseNdcX >= sx && mouseNdcX <= sx + slotW && mouseNdcY >= sy && mouseNdcY <= sy + slotH) {
                if (m_selectedSlot == idx) {
                    UseOrEquipSlot(idx, player, particles, damageNumbers);
                } else {
                    m_selectedSlot = idx;
                }
                return true;
            }
        }
    }

    // Clic en Ranuras de Equipo (Desequipar)
    float eqPanelX = pX + 0.81f;
    float eqSlotW = 0.242f, eqSlotH = 0.118f;
    float eqStartY = pY + pH - 0.25f;
    float colGap = 0.018f;
    float col1X = eqPanelX;
    float col2X = eqPanelX + eqSlotW + colGap;
    float col3X = eqPanelX + (eqSlotW + colGap) * 2.0f;

    float row0Y = eqStartY;
    float row1Y = eqStartY - (eqSlotH + 0.016f);
    float row2Y = eqStartY - 2.0f * (eqSlotH + 0.016f);
    float row3Y = eqStartY - 3.0f * (eqSlotH + 0.016f);

    struct EqSlotClick {
        EquipSlot slot;
        float rx, ry, rw, rh;
    };

    EqSlotClick eqClicks[] = {
        { EquipSlot::HEAD,      col2X, row0Y, eqSlotW, eqSlotH },
        { EquipSlot::MAIN_HAND, col1X, row1Y, eqSlotW, eqSlotH },
        { EquipSlot::CHEST,     col2X, row1Y, eqSlotW, eqSlotH },
        { EquipSlot::OFF_HAND,  col3X, row1Y, eqSlotW, eqSlotH },
        { EquipSlot::GLOVES,    col1X, row2Y, eqSlotW, eqSlotH },
        { EquipSlot::LEGS,      col2X, row2Y, eqSlotW, eqSlotH },
        { EquipSlot::AMULET,    col3X, row2Y, eqSlotW, eqSlotH },
        { EquipSlot::RING_1,    col1X, row3Y, eqSlotW, eqSlotH },
        { EquipSlot::FEET,      col2X, row3Y, eqSlotW, eqSlotH },
        { EquipSlot::RING_2,    col3X, row3Y, eqSlotW, eqSlotH }
    };

    for (const auto& ec : eqClicks) {
        if (mouseNdcX >= ec.rx && mouseNdcX <= ec.rx + ec.rw && mouseNdcY >= ec.ry && mouseNdcY <= ec.ry + ec.rh) {
            UnequipSlot(ec.slot, player);
            return true;
        }
    }

    return false;
}

void InventorySystem::Render3DItemSlots(GLuint shaderProgram, float globalTime, int screenW, int screenH, float mouseNdcX, float mouseNdcY) {
    if (!m_isOpen) return;

    // Solo si hay un ítem bajo el cursor mostramos el visor 3D interactivo en el Tooltip
    if (!m_hasHoveredItem || m_hoveredItemStringId.empty()) return;

    ItemModelRegistry::Get().Init();

    GLint curVp[4] = {0};
    glGetIntegerv(GL_VIEWPORT, curVp);
    int realW = (curVp[2] > 0) ? curVp[2] : screenW;
    int realH = (curVp[3] > 0) ? curVp[3] : screenH;

    // Vitrina 3D Flotante dentro del Tooltip de Inspección (Rotación continua en 360° estilo MU Online)
    int vpX = (int)((m_tipShowcaseX + 0.006f + 1.0f) * 0.5f * realW);
    int vpY = (int)((m_tipShowcaseY + 0.006f + 1.0f) * 0.5f * realH);
    int vpW = (int)((m_tipShowcaseW - 0.012f) * 0.5f * realW);
    int vpH = (int)((m_tipShowcaseH - 0.012f) * 0.5f * realH);

    ItemModelRegistry::Get().RenderItemInSlot(m_hoveredItemStringId, vpX, vpY, vpW, vpH, true, globalTime, shaderProgram);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, realW, realH);
}
