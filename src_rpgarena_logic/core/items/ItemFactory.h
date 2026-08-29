#pragma once
#include "Item.h"
#include <string>

class ItemFactory {
public:
    // Escala una armadura procedural basada en su nivel y calidad
    static void scaleArmor(Item& item, int level, ItemQuality quality);

    // Escala un anillo procedural basado en su nivel y calidad
    static void scaleRing(Item& item, int level, ItemQuality quality);

    // Roll de calidad basado en nivel (reutiliza lógica similar a Weapon)
    static ItemQuality rollQuality(int level);

    // Multiplicadores
    static float getQualityMultiplier(ItemQuality quality);
    static float getLevelMultiplier(int level);
};
