#pragma once

#include "ItemTypes.h"
#include <algorithm>

/**
 * @brief ItemInstance: Estado mutable de una copia concreta de un objeto en una ranura.
 * Es ultraligero (12 bytes) y no almacena cadenas de texto repetidas.
 */
struct ItemInstance {
    ItemId id = INVALID_ITEM_ID;
    uint16_t quantity = 0;
    uint16_t durability = 1000;      // 1000 = 100.0%, 0 = Roto / Inutilizable
    uint32_t customDataHandle = 0;   // 0 = Item estándar; >0 = Puntero a pool de afijos/encantamientos

    constexpr ItemInstance() noexcept = default;
    
    constexpr ItemInstance(ItemId itemId, uint16_t qty, uint16_t dur = 1000, uint32_t handle = 0) noexcept
        : id(itemId), quantity(qty), durability(dur), customDataHandle(handle) {}

    constexpr bool IsValid() const noexcept {
        return id.IsValid() && quantity > 0;
    }

    constexpr bool IsEmpty() const noexcept {
        return !IsValid();
    }

    void Clear() noexcept {
        id = INVALID_ITEM_ID;
        quantity = 0;
        durability = 1000;
        customDataHandle = 0;
    }

    /**
     * @brief Comprueba si esta instancia puede apilarse con otra.
     * Dos objetos solo pueden apilarse si comparten el mismo ID, durabilidad al 100% y no tienen datos únicos personalizados.
     */
    bool CanStackWith(const ItemInstance& other, int maxStack) const noexcept {
        if (!IsValid() || !other.IsValid()) return false;
        if (id != other.id) return false;
        if (maxStack <= 1) return false;
        if (customDataHandle != 0 || other.customDataHandle != 0) return false;
        if (durability != other.durability) return false;
        return quantity < maxStack;
    }
};
