#include "WeaponSprite.h"
#include <cmath>


WeaponSprite::WeaponSprite() 
    : m_baseTexture(nullptr), m_layoutTexture(nullptr), m_gridSize(16, 16), m_fortificationLevel(0)
{
}

void WeaponSprite::setTextures(const sf::Texture& baseTexture, const sf::Texture& layoutTexture) {
    m_baseTexture = &baseTexture;
    m_layoutTexture = &layoutTexture;

    m_baseSprite.emplace(*m_baseTexture);
    m_overlaySprite.emplace(*m_layoutTexture);
}

void WeaponSprite::setGridSize(int width, int height) {
    m_gridSize = {width, height};
}

// [REVERTED] Clean implementation - pure visual container
void WeaponSprite::setVisuals(const sf::IntRect& baseRect, const sf::IntRect& overlayRect, sf::Color rarityColor) {
    if (!m_baseSprite) return;

    m_rarityColor = rarityColor;

    // Configurar Rect del Base Sprite
    m_baseSprite->setTextureRect(baseRect);

    // Configurar Rect del Overlay Sprite
    // Si el rect es invalido o vacio, ocultamos
    if (overlayRect.size.x <= 0 || overlayRect.size.y <= 0) {
        m_overlaySprite.reset();
    } else {
        // Aseguramos que existe
        if (!m_overlaySprite) m_overlaySprite.emplace(*m_layoutTexture);

        m_overlaySprite->setTextureRect(overlayRect);
        
        // Use the passed color directly.
        m_overlaySprite->setColor(rarityColor);
    }
}

void WeaponSprite::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Si no tenemos ni base, no dibujamos nada
    if (!m_baseSprite) return;

    // Guardar estado original de transformaciones antes de aplicar locales
    sf::RenderStates originalStates = states;
    states.transform *= getTransform();

    // Dibujar el aura detrás si tiene fortificación >= 6 usando el mismo renderizador unificado de ItemAuraRenderer
    if (m_fortificationLevel >= 6) {
        sf::RenderStates glowStates = originalStates;
        glowStates.transform = states.transform;
        ItemAuraRenderer::drawAura(target, *m_baseSprite, m_fortificationLevel, 1.0f, glowStates);
    }

    // Dibujar base primero
    target.draw(*m_baseSprite, states);

    // Dibujar overlay encima si existe
    if (m_overlaySprite) {
        target.draw(*m_overlaySprite, states);
    }
}

void WeaponSprite::setColor(sf::Color color) {
    if (m_baseSprite) m_baseSprite->setColor(color);
    if (m_overlaySprite) {
        sf::Color finalColor = m_rarityColor;
        finalColor.a = static_cast<std::uint8_t>((static_cast<float>(m_rarityColor.a) * color.a) / 255.f);
        m_overlaySprite->setColor(finalColor);
    }
}
