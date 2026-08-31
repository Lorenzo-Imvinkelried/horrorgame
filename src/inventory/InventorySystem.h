#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "ItemTypes.h"
#include "ItemRegistry.h"
#include "Inventory.h"
#include "Equipment.h"
#include "ItemActionSystem.h"
#include "combat/CombatStats.h"

// Enum de compatibilidad hacia atrás
enum class ItemType {
    NONE = 0,
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

class Player;
class ItemDropSystem;

/**
 * @brief InventorySystem: Fachada y gestor de interfaz de usuario para Inventario y Equipamiento.
 * Conecta el contenedor puro Inventory, el gestor Equipment y el sistema de acciones ItemActionSystem con el renderizado OpenGL.
 */
class InventorySystem {
public:
    InventorySystem();
    ~InventorySystem() = default;

    // --- Métodos de compatibilidad y conveniencia ---
    bool AddItem(ItemType type, int count = 1);
    bool AddItem(ItemId id, int count = 1);
    bool AddItemByString(const std::string& stringId, int count = 1);

    bool UseOrEquipSlot(int slotIndex, Player* player, ParticleSystem* particles = nullptr, DamageNumberSystem* damageNumbers = nullptr);
    bool UnequipSlot(EquipSlot slot, Player* player);
    bool DropSlot(int slotIndex, Player* player, ItemDropSystem* itemDropSystem, ParticleSystem* particles = nullptr);
    void RecalculateBonuses(PlayerStats& stats);

    // --- Renderizado e Interacción con UI (Inventario + Equipamiento) ---
    void RenderWindow(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float mouseNdcX, float mouseNdcY, const PlayerStats* playerStats = nullptr);
    bool HandleMouseClick(float mouseNdcX, float mouseNdcY, Player* player, ParticleSystem* particles, DamageNumberSystem* damageNumbers, ItemDropSystem* itemDropSystem, bool& closeRequested);
    void UpdateDrag(float mouseNdcX, float mouseNdcY, bool isLeftMouseDown);

    bool IsOpen() const noexcept { return m_isOpen; }
    void ToggleOpen() noexcept { m_isOpen = !m_isOpen; }
    void SetOpen(bool open) noexcept { m_isOpen = open; }

    int GetSelectedSlot() const noexcept { return m_selectedSlot; }
    void SetSelectedSlot(int s) noexcept { m_selectedSlot = s; }

    Inventory& GetInventory() noexcept { return m_inventory; }
    const Inventory& GetInventory() const noexcept { return m_inventory; }

    Equipment& GetEquipment() noexcept { return m_equipment; }
    const Equipment& GetEquipment() const noexcept { return m_equipment; }

private:
    ItemId mapLegacyType(ItemType type) const;

    bool m_isOpen = false;
    int m_selectedSlot = -1;
    Inventory m_inventory;  // Contenedor de 16 slots
    Equipment m_equipment;  // Gestor de equipo

    // Posición arrastrable de la ventana (Estilo Windows 98)
    float m_winX = -0.81f;
    float m_winY = -0.74f;
    bool m_isDragging = false;
    float m_dragOffsetX = 0.0f;
    float m_dragOffsetY = 0.0f;
};
