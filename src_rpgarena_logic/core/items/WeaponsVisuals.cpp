#include "WeaponsVisuals.h"

sf::BlendMode WeaponsVisuals::getBlendMode(ItemQuality quality) {
    switch (quality) {
        case ItemQuality::Legendary:
        case ItemQuality::Mythic:
        case ItemQuality::Godly:
            // Efecto brillante (Additive) para items de alto nivel
            return sf::BlendAdd;
            
        case ItemQuality::Common:
        case ItemQuality::Uncommon:
        case ItemQuality::Rare:
        case ItemQuality::Epic:
        default:
            // Normal alpha blending
            return sf::BlendAlpha;
    }
}

sf::Color WeaponsVisuals::getGlowColor(ItemQuality quality) {
    switch(quality) {
        // Colores "Nucleares" para BlendAdd (Saturación máxima)
        case ItemQuality::Uncommon:  return sf::Color(0, 255, 0);      // Verde Puro
        case ItemQuality::Rare:      return sf::Color(0, 100, 255);    // Azul Eléctrico
        case ItemQuality::Epic:      return sf::Color(255, 140, 0);    // Naranja Fuego
        case ItemQuality::Legendary: return sf::Color(255, 215, 0);    // Oro Puro
        case ItemQuality::Mythic:    return sf::Color(200, 0, 255);    // Violeta Neón
        case ItemQuality::Godly:     return sf::Color(255, 0, 0);      // Rojo Sangre Puro
        default:                     return sf::Color::White;
    }
}
