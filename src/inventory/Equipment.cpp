#include "Equipment.h"
#include "Inventory.h"
#include "ItemRegistry.h"

Equipment::Equipment() {
    Clear();
}

void Equipment::Clear() {
    for (auto& slot : m_slots) {
        slot.Clear();
    }
}

const ItemInstance& Equipment::GetEquipped(EquipSlot slot) const {
    size_t idx = static_cast<size_t>(slot);
    if (idx < m_slots.size()) {
        return m_slots[idx];
    }
    static const ItemInstance emptyDummy;
    return emptyDummy;
}

ItemInstance& Equipment::GetEquipped(EquipSlot slot) {
    size_t idx = static_cast<size_t>(slot);
    if (idx < m_slots.size()) {
        return m_slots[idx];
    }
    static ItemInstance emptyDummy;
    emptyDummy.Clear();
    return emptyDummy;
}

bool Equipment::HasEquipped(EquipSlot slot) const {
    return GetEquipped(slot).IsValid();
}

bool Equipment::IsMainHandTwoHanded() const {
    const ItemInstance& mainHand = GetEquipped(EquipSlot::MAIN_HAND);
    if (!mainHand.IsValid()) return false;
    const ItemDefinition& def = ItemRegistry::Get().Get(mainHand.id);
    return def.isTwoHanded;
}

bool Equipment::IsSlotBlocked(EquipSlot slot) const {
    if (slot == EquipSlot::OFF_HAND) {
        return IsMainHandTwoHanded();
    }
    return false;
}

EquipSlot Equipment::resolveTargetSlot(const ItemDefinition& def, EquipSlot preferredSlot) const {
    if (def.equipSlot == EquipSlot::RING_1 || def.equipSlot == EquipSlot::RING_2) {
        if (preferredSlot == EquipSlot::RING_1 || preferredSlot == EquipSlot::RING_2) {
            return preferredSlot;
        }
        // Auto-selección inteligente de ranura de anillo
        if (!HasEquipped(EquipSlot::RING_1)) return EquipSlot::RING_1;
        if (!HasEquipped(EquipSlot::RING_2)) return EquipSlot::RING_2;
        return EquipSlot::RING_1; // Por defecto reemplaza el anillo 1
    }
    return def.equipSlot;
}

EquipResult Equipment::EquipFromInventory(Inventory& inventory, int inventorySlotIndex, EquipSlot targetSlot) {
    if (inventorySlotIndex < 0 || inventorySlotIndex >= inventory.GetSlotCount()) {
        return EquipResult::INVALID_ITEM;
    }

    ItemInstance itemToEquip = inventory.GetSlot(inventorySlotIndex);
    if (!itemToEquip.IsValid()) {
        return EquipResult::INVALID_ITEM;
    }

    const ItemDefinition& newDef = ItemRegistry::Get().Get(itemToEquip.id);
    if (!newDef.IsEquippable()) {
        return EquipResult::NOT_EQUIPPABLE;
    }

    EquipSlot resolvedSlot = resolveTargetSlot(newDef, targetSlot);
    if (resolvedSlot == EquipSlot::NONE || resolvedSlot >= EquipSlot::COUNT) {
        return EquipResult::WRONG_SLOT;
    }

    // -------------------------------------------------------------------------
    // 1. Identificación de Objetos Desplazados (Conflicto de Ranuras)
    // -------------------------------------------------------------------------
    std::vector<std::pair<EquipSlot, ItemInstance>> displacedItems;

    // A. El item que ya estaba en la ranura destino
    if (HasEquipped(resolvedSlot)) {
        displacedItems.push_back({ resolvedSlot, GetEquipped(resolvedSlot) });
    }

    // B. Caso Especial: Equipar arma de dos manos (2H) en MAIN_HAND
    // Si equipamos un arma 2H, la OFF_HAND (ej. escudo) también debe desequiparse
    if (resolvedSlot == EquipSlot::MAIN_HAND && newDef.isTwoHanded) {
        if (HasEquipped(EquipSlot::OFF_HAND)) {
            displacedItems.push_back({ EquipSlot::OFF_HAND, GetEquipped(EquipSlot::OFF_HAND) });
        }
    }

    // C. Caso Especial: Equipar en OFF_HAND teniendo un arma 2H en MAIN_HAND
    // Si tenemos un arma 2H y equipamos un escudo en OFF_HAND, el arma 2H debe salir
    if (resolvedSlot == EquipSlot::OFF_HAND && IsMainHandTwoHanded()) {
        if (HasEquipped(EquipSlot::MAIN_HAND)) {
            displacedItems.push_back({ EquipSlot::MAIN_HAND, GetEquipped(EquipSlot::MAIN_HAND) });
        }
    }

    // -------------------------------------------------------------------------
    // 2. Validación de Espacio Transaccional (Atomicidad)
    // -------------------------------------------------------------------------
    // Al sacar `itemToEquip` del inventario, la casilla `inventorySlotIndex` queda libre (1 slot liberado).
    // Necesitamos verificar si el inventario puede albergar todos los items desplazados.
    int netSlotsNeeded = static_cast<int>(displacedItems.size()) - 1;
    if (netSlotsNeeded > 0) {
        int freeSlotsCount = 0;
        for (int i = 0; i < inventory.GetSlotCount(); ++i) {
            if (i != inventorySlotIndex && inventory.GetSlot(i).IsEmpty()) {
                ++freeSlotsCount;
            }
        }
        if (freeSlotsCount < netSlotsNeeded) {
            // Abortar la transacción completa sin alterar nada
            return EquipResult::INSUFFICIENT_INVENTORY_SPACE;
        }
    }

    // -------------------------------------------------------------------------
    // 3. Fase de Commit (Aplicación de cambios atómicos)
    // -------------------------------------------------------------------------
    // A. Quitar el item del inventario
    inventory.GetSlot(inventorySlotIndex).Clear();

    // B. Limpiar las ranuras de equipo desplazadas
    for (const auto& [slot, item] : displacedItems) {
        m_slots[static_cast<size_t>(slot)].Clear();
    }

    // C. Equipar el nuevo objeto
    m_slots[static_cast<size_t>(resolvedSlot)] = itemToEquip;

    // D. Devolver los items desplazados al inventario
    bool firstItemPlaced = false;
    for (const auto& [slot, item] : displacedItems) {
        if (!firstItemPlaced) {
            // El primer item desplazado vuelve exactamente a la casilla original del inventario (Excelente UX)
            inventory.GetSlot(inventorySlotIndex) = item;
            firstItemPlaced = true;
        } else {
            // Los siguientes items desplazados se colocan en ranuras libres
            inventory.AddInstance(item);
        }
    }

    return EquipResult::SUCCESS;
}

bool Equipment::UnequipToInventory(Inventory& inventory, EquipSlot slot) {
    if (slot == EquipSlot::NONE || slot >= EquipSlot::COUNT) {
        return false;
    }

    if (!HasEquipped(slot)) {
        return false;
    }

    int freeIndex = inventory.FindFreeSlot();
    if (freeIndex == -1) {
        return false; // Inventario lleno
    }

    ItemInstance item = GetEquipped(slot);
    m_slots[static_cast<size_t>(slot)].Clear();
    inventory.AddInstance(item);

    return true;
}

EquipmentStats Equipment::CalculateTotalStats() const {
    EquipmentStats total;
    for (size_t i = 0; i < m_slots.size(); ++i) {
        const ItemInstance& inst = m_slots[i];
        if (inst.IsValid()) {
            const ItemDefinition& def = ItemRegistry::Get().Get(inst.id);
            total += def.equipStats;
        }
    }
    return total;
}
