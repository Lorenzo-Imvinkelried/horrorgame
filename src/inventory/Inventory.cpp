#include "Inventory.h"
#include "ItemRegistry.h"
#include <algorithm>

Inventory::Inventory(int slotCount, float maxWeight)
    : m_slots(slotCount > 0 ? slotCount : 16)
    , m_maxWeight(maxWeight > 0.0f ? maxWeight : 60.0f)
{
}

bool Inventory::AddItem(ItemId id, int quantity, int* outRemaining) {
    if (!id.IsValid() || quantity <= 0) {
        if (outRemaining) *outRemaining = quantity;
        return false;
    }

    ItemInstance inst(id, static_cast<uint16_t>(quantity));
    return AddInstance(inst, outRemaining);
}

bool Inventory::AddItemByString(const std::string& stringId, int quantity, int* outRemaining) {
    ItemId id = ItemRegistry::Get().FindId(stringId);
    return AddItem(id, quantity, outRemaining);
}

bool Inventory::AddInstance(const ItemInstance& instance, int* outRemaining) {
    if (!instance.IsValid()) {
        if (outRemaining) *outRemaining = 0;
        return false;
    }

    const ItemDefinition& def = ItemRegistry::Get().Get(instance.id);
    if (!def.id.IsValid()) {
        if (outRemaining) *outRemaining = instance.quantity;
        return false;
    }

    int remainingToAdd = instance.quantity;

    // 1. Fase de Apilamiento (Si es apilable, rellenar ranuras existentes compatibles)
    if (def.IsStackable() && instance.customDataHandle == 0) {
        for (auto& slot : m_slots) {
            if (slot.CanStackWith(instance, def.maxStack)) {
                int spaceInSlot = def.maxStack - slot.quantity;
                int amountToPut = std::min(remainingToAdd, spaceInSlot);
                slot.quantity += static_cast<uint16_t>(amountToPut);
                remainingToAdd -= amountToPut;

                if (remainingToAdd <= 0) {
                    if (outRemaining) *outRemaining = 0;
                    return true;
                }
            }
        }
    }

    // 2. Fase de Ranuras Nuevas (Buscar casillas vacías para lo que quede)
    while (remainingToAdd > 0) {
        int freeIndex = FindFreeSlot();
        if (freeIndex == -1) {
            // Inventario Lleno
            if (outRemaining) *outRemaining = remainingToAdd;
            return remainingToAdd < instance.quantity; // Retorna true si al menos se guardó una parte
        }

        int amountToPut = def.IsStackable() ? std::min(remainingToAdd, def.maxStack) : 1;
        m_slots[freeIndex] = ItemInstance(instance.id, static_cast<uint16_t>(amountToPut), instance.durability, instance.customDataHandle);
        remainingToAdd -= amountToPut;
    }

    if (outRemaining) *outRemaining = 0;
    return true;
}

bool Inventory::RemoveItemAt(int slotIndex, int quantity) {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(m_slots.size()) || quantity <= 0) {
        return false;
    }

    ItemInstance& slot = m_slots[slotIndex];
    if (!slot.IsValid() || slot.quantity < quantity) {
        return false;
    }

    slot.quantity -= static_cast<uint16_t>(quantity);
    if (slot.quantity == 0) {
        slot.Clear();
    }
    return true;
}

bool Inventory::RemoveItemById(ItemId id, int quantity) {
    if (!id.IsValid() || quantity <= 0 || GetItemCount(id) < quantity) {
        return false;
    }

    int remainingToRemove = quantity;
    for (auto& slot : m_slots) {
        if (slot.id == id) {
            int amount = std::min(remainingToRemove, static_cast<int>(slot.quantity));
            slot.quantity -= static_cast<uint16_t>(amount);
            remainingToRemove -= amount;

            if (slot.quantity == 0) {
                slot.Clear();
            }

            if (remainingToRemove <= 0) {
                return true;
            }
        }
    }
    return remainingToRemove == 0;
}

bool Inventory::SwapSlots(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_slots.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(m_slots.size()) ||
        fromIndex == toIndex) 
    {
        return false;
    }

    // Si ambos slots contienen el mismo item apilable, combinarlos
    ItemInstance& from = m_slots[fromIndex];
    ItemInstance& to = m_slots[toIndex];

    if (from.IsValid() && to.IsValid() && from.id == to.id) {
        const ItemDefinition& def = ItemRegistry::Get().Get(from.id);
        if (to.CanStackWith(from, def.maxStack)) {
            int spaceInTo = def.maxStack - to.quantity;
            int amountToMove = std::min(static_cast<int>(from.quantity), spaceInTo);
            to.quantity += static_cast<uint16_t>(amountToMove);
            from.quantity -= static_cast<uint16_t>(amountToMove);

            if (from.quantity == 0) {
                from.Clear();
            }
            return true;
        }
    }

    std::swap(m_slots[fromIndex], m_slots[toIndex]);
    return true;
}

bool Inventory::SplitSlot(int fromIndex, int toIndex, int count) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_slots.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(m_slots.size()) ||
        fromIndex == toIndex || count <= 0) 
    {
        return false;
    }

    ItemInstance& from = m_slots[fromIndex];
    ItemInstance& to = m_slots[toIndex];

    if (!from.IsValid() || from.quantity <= count) {
        return false; // No hay suficientes para dividir
    }

    if (to.IsEmpty()) {
        to = ItemInstance(from.id, static_cast<uint16_t>(count), from.durability, from.customDataHandle);
        from.quantity -= static_cast<uint16_t>(count);
        return true;
    }

    if (to.id == from.id) {
        const ItemDefinition& def = ItemRegistry::Get().Get(from.id);
        if (to.CanStackWith(from, def.maxStack)) {
            int space = def.maxStack - to.quantity;
            int actualMove = std::min(count, space);
            to.quantity += static_cast<uint16_t>(actualMove);
            from.quantity -= static_cast<uint16_t>(actualMove);
            return actualMove > 0;
        }
    }

    return false;
}

void Inventory::Clear() {
    for (auto& slot : m_slots) {
        slot.Clear();
    }
}

int Inventory::GetItemCount(ItemId id) const {
    if (!id.IsValid()) return 0;
    int total = 0;
    for (const auto& slot : m_slots) {
        if (slot.id == id) {
            total += slot.quantity;
        }
    }
    return total;
}

bool Inventory::HasItem(ItemId id, int quantity) const {
    return GetItemCount(id) >= quantity;
}

float Inventory::GetCurrentWeight() const {
    float totalWeight = 0.0f;
    for (const auto& slot : m_slots) {
        if (slot.IsValid()) {
            const ItemDefinition& def = ItemRegistry::Get().Get(slot.id);
            totalWeight += def.weight * slot.quantity;
        }
    }
    return totalWeight;
}

int Inventory::GetOccupiedSlotCount() const {
    int count = 0;
    for (const auto& slot : m_slots) {
        if (slot.IsValid()) ++count;
    }
    return count;
}

int Inventory::FindFirstSlot(ItemId id) const {
    if (!id.IsValid()) return -1;
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) {
        if (m_slots[i].id == id) return i;
    }
    return -1;
}

int Inventory::FindItemByString(const std::string& stringId) const {
    ItemId id = ItemRegistry::Get().FindId(stringId);
    if (!id.IsValid()) return -1;
    return FindFirstSlot(id);
}

int Inventory::CountItemByString(const std::string& stringId) const {
    ItemId id = ItemRegistry::Get().FindId(stringId);
    if (!id.IsValid()) return 0;
    return GetItemCount(id);
}

int Inventory::FindFreeSlot() const {
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) {
        if (m_slots[i].IsEmpty()) return i;
    }
    return -1;
}

const ItemInstance& Inventory::GetSlot(int index) const {
    static const ItemInstance emptyDummy;
    if (index >= 0 && index < static_cast<int>(m_slots.size())) {
        return m_slots[index];
    }
    return emptyDummy;
}

ItemInstance& Inventory::GetSlot(int index) {
    static ItemInstance emptyDummy;
    if (index >= 0 && index < static_cast<int>(m_slots.size())) {
        return m_slots[index];
    }
    emptyDummy.Clear();
    return emptyDummy;
}
