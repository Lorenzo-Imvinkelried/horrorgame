#pragma once
#include "Item.h"
#include <string>

class WeaponsFactory {
public:
    // Escala un arma existente al nivel y calidad deseados
    // Modifica los stats del item pasado por referencia
    static void scaleWeapon(Item& item, int level, ItemQuality quality);

    // Estructura para definir los chances de drop
    struct RarityChances {
        float godly = 0.0f;
        float mythic = 0.0f;
        float legendary = 0.0f;
        float epic = 0.0f;
        float rare = 0.0f;
        float uncommon = 0.0f;
    };

    // Genera una calidad aleatoria basada en el nivel
    static ItemQuality rollQuality(int level);

    // Helpers (por si se necesitan fuera)
    static float getQualityMultiplier(ItemQuality quality);
    static float getLevelMultiplier(int level); // Calcula el multiplicador base por nivel

    // Data-driven accessors
    static const std::vector<std::string>& getBaseNames();
    static std::string getCustom32x32Name(int index);
    static void ensureConfigLoaded();
};
