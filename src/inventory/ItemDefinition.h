#pragma once

#include "ItemTypes.h"
#include <string>
#include <vector>

/**
 * @brief ItemDefinition: Ficha técnica inmutable de un objeto (Patrón Flyweight).
 * Se crea una sola vez en el ItemRegistry y se referencia por ItemId en tiempo de ejecución.
 */
struct ItemDefinition {
    ItemId id = INVALID_ITEM_ID;
    std::string stringId = "";       // Identificador textual estable: "potion_healing_t1"
    std::string name = "";           // Nombre legible: "Poción de Vida Mayor"
    std::string description = "";    // Descripción y lore
    ItemCategory category = ItemCategory::MATERIAL;
    ItemRarity rarity = ItemRarity::COMMON;

    int maxStack = 99;               // Límite de apilamiento en una sola casilla
    float weight = 0.1f;             // Peso unitario en Kilogramos
    int value = 1;                   // Valor base en oro / trueque
    std::string iconId = "";         // Identificador para renderizado de icono/ASCII

    // Componentes condicionales de equipamiento
    EquipSlot equipSlot = EquipSlot::NONE;
    bool isTwoHanded = false;
    EquipmentStats equipStats;

    // Componentes condicionales de uso / consumibles
    bool isUsable = false;
    std::vector<ItemEffect> effects;

    bool IsEquippable() const noexcept {
        return equipSlot != EquipSlot::NONE;
    }

    bool IsStackable() const noexcept {
        return maxStack > 1;
    }
};
