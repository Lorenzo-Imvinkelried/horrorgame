#pragma once

#include "ItemInstance.h"
#include "ItemTypes.h"
#include <array>
#include <vector>

class Inventory;

enum class EquipResult : uint8_t {
    SUCCESS = 0,
    INVALID_ITEM,
    NOT_EQUIPPABLE,
    WRONG_SLOT,
    INSUFFICIENT_INVENTORY_SPACE,
    REQUIREMENTS_NOT_MET
};

/**
 * @brief Equipment: Gestor transaccional y atómico de ranuras de equipamiento.
 * Controla reglas de armas a dos manos (2H), anillos duales y swaps atómicos sin pérdida de items.
 */
class Equipment {
public:
    Equipment();
    ~Equipment() = default;

    /**
     * @brief Equipa de forma atómica un objeto desde una ranura del inventario.
     * Si no hay espacio suficiente en el inventario para los items reemplazados (ej. Mandoble 2H que desequipa arma y escudo),
     * la operación se aborta completamente sin alterar ningún slot.
     */
    EquipResult EquipFromInventory(Inventory& inventory, int inventorySlotIndex, EquipSlot targetSlot = EquipSlot::NONE);

    /**
     * @brief Desequipa un objeto y lo transfiere al inventario si hay espacio libre.
     */
    bool UnequipToInventory(Inventory& inventory, EquipSlot slot);

    /**
     * @brief Calcula la suma total de todas las estadísticas proporcionadas por el equipo actual.
     */
    EquipmentStats CalculateTotalStats() const;

    /**
     * @brief Comprueba si una ranura está bloqueada por otro objeto (ej. OFF_HAND bloqueada por arma 2H).
     */
    bool IsSlotBlocked(EquipSlot slot) const;

    /**
     * @brief Comprueba si el arma principal equipada es de dos manos.
     */
    bool IsMainHandTwoHanded() const;

    const ItemInstance& GetEquipped(EquipSlot slot) const;
    ItemInstance& GetEquipped(EquipSlot slot);
    bool HasEquipped(EquipSlot slot) const;

    void Clear();

    // Obtener todos los slots equipados como array indexable
    const std::array<ItemInstance, static_cast<size_t>(EquipSlot::COUNT)>& GetAllEquipped() const noexcept {
        return m_slots;
    }

private:
    EquipSlot resolveTargetSlot(const struct ItemDefinition& def, EquipSlot preferredSlot) const;

    std::array<ItemInstance, static_cast<size_t>(EquipSlot::COUNT)> m_slots;
};
