#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Identificador tipado y compacto de 32 bits para objetos.
 * Evita asignaciones de memoria dinámicas (std::string) por ranura de inventario.
 */
using ItemNumericId = uint32_t;

struct ItemId {
    ItemNumericId value = 0; // 0 representa INVALID_ITEM / EMPTY_SLOT

    constexpr ItemId() noexcept : value(0) {}
    constexpr explicit ItemId(ItemNumericId val) noexcept : value(val) {}

    constexpr bool IsValid() const noexcept { return value != 0; }
    constexpr bool operator==(const ItemId& other) const noexcept = default;
    constexpr bool operator!=(const ItemId& other) const noexcept = default;
    constexpr bool operator<(const ItemId& other) const noexcept { return value < other.value; }
};

static constexpr ItemId INVALID_ITEM_ID = ItemId(0);

enum class ItemCategory : uint8_t {
    MATERIAL,      // Madera, Piedra, Huesos, Minerales
    CONSUMABLE,    // Pociones, Carne, Viales
    EQUIPMENT,     // Armas, Escudos, Armaduras, Joyas
    SCROLL,        // Pergaminos de hechizos y lore
    KEY_QUEST      // Llaves y objetos de misión
};

enum class ItemRarity : uint8_t {
    COMMON,        // Blanco / Gris (#CCCCCC)
    UNCOMMON,      // Verde (#33CC33)
    RARE,          // Azul (#3399FF)
    EPIC,          // Morado (#AA33FF)
    LEGENDARY      // Dorado (#FFAA00)
};

enum class EquipSlot : uint8_t {
    NONE = 0,
    MAIN_HAND,
    OFF_HAND,
    CHEST,
    HEAD,
    LEGS,
    FEET,
    GLOVES,
    RING_1,
    RING_2,
    AMULET,
    COUNT
};

struct EquipmentStats {
    int attackPower = 0;
    int defense = 0;
    int critChance = 0;   // Porcentaje (+5%)
    int evasion = 0;      // Porcentaje (+8%)
    int maxHpBonus = 0;
    int maxMpBonus = 0;

    EquipmentStats& operator+=(const EquipmentStats& other) {
        attackPower += other.attackPower;
        defense += other.defense;
        critChance += other.critChance;
        evasion += other.evasion;
        maxHpBonus += other.maxHpBonus;
        maxMpBonus += other.maxMpBonus;
        return *this;
    }
};

enum class ItemEffectType : uint8_t {
    NONE = 0,
    RESTORE_HP,
    RESTORE_MP,
    BUFF_ATTACK,
    BUFF_DEFENSE,
    BUFF_SPEED,
    LEARN_SPELL,
    GRANT_EXP
};

struct ItemEffect {
    ItemEffectType type = ItemEffectType::NONE;
    float magnitude = 0.0f;
    float duration = 0.0f; // 0.0f = Instantáneo, >0.0f = Buff temporal con duración en segundos
};
