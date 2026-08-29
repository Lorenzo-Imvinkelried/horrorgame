#pragma once
#include <SFML/Graphics.hpp>
#include "Item.h"

class WeaponsVisuals {
public:
    // Retorna el BlendMode apropiado según la calidad
    static sf::BlendMode getBlendMode(ItemQuality quality);
    
    // Retorna el color de brillo (más intenso para efectos aditivos)
    static sf::Color getGlowColor(ItemQuality quality);
};
