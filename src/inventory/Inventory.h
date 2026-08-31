#pragma once

#include "ItemInstance.h"
#include <vector>
#include <string>

/**
 * @brief Inventory: Contenedor puro de almacenamiento de objetos (Sin lógica de combate ni curación).
 * Gestiona de forma determinista el apilamiento (stacking), límite de peso, división y reordenamiento de ranuras.
 */
class Inventory {
public:
    explicit Inventory(int slotCount = 16, float maxWeight = 60.0f);
    ~Inventory() = default;

    // --- Operaciones de Inserción y Extracción ---
    bool AddItem(ItemId id, int quantity = 1, int* outRemaining = nullptr);
    bool AddItemByString(const std::string& stringId, int quantity = 1, int* outRemaining = nullptr);
    bool AddInstance(const ItemInstance& instance, int* outRemaining = nullptr);

    bool RemoveItemAt(int slotIndex, int quantity = 1);
    bool RemoveItemById(ItemId id, int quantity = 1);
    bool SwapSlots(int fromIndex, int toIndex);
    bool SplitSlot(int fromIndex, int toIndex, int count);
    void Clear();

    // --- Consultas de Estado ---
    int GetItemCount(ItemId id) const;
    bool HasItem(ItemId id, int quantity = 1) const;
    float GetCurrentWeight() const;
    float GetMaxWeight() const noexcept { return m_maxWeight; }
    void SetMaxWeight(float maxW) noexcept { m_maxWeight = maxW; }

    int GetSlotCount() const noexcept { return static_cast<int>(m_slots.size()); }
    int GetOccupiedSlotCount() const;
    int FindFirstSlot(ItemId id) const;
    int FindFreeSlot() const;

    int FindItemByString(const std::string& stringId) const;
    int CountItemByString(const std::string& stringId) const;

    const ItemInstance& GetSlot(int index) const;
    ItemInstance& GetSlot(int index);
    const std::vector<ItemInstance>& GetAllSlots() const noexcept { return m_slots; }

private:
    std::vector<ItemInstance> m_slots;
    float m_maxWeight;
};
