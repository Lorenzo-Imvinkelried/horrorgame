#pragma once
#include "Item.h" // [NEW]

class WeaponSprite : public sf::Drawable, public sf::Transformable {
public:
    WeaponSprite();

    // Recibe referencias a ambas texturas
    void setTextures(const sf::Texture& baseTexture, const sf::Texture& layoutTexture);

    // [MODIFIED] Reverted to legacy: explicit color passing
    void setVisuals(const sf::IntRect& baseRect, const sf::IntRect& overlayRect, sf::Color rarityColor);

    // Define el tamaño de la celda de la grilla (por defecto 16x16)
    void setGridSize(int width, int height);

    void setColor(sf::Color color);

    void setFortificationLevel(int level) { m_fortificationLevel = level; }
    int getFortificationLevel() const { return m_fortificationLevel; }

    const sf::Texture* getBaseTexture() const { return m_baseTexture; }
    const sf::Texture* getLayoutTexture() const { return m_layoutTexture; }
    sf::IntRect getBaseRect() const { return m_baseSprite ? m_baseSprite->getTextureRect() : sf::IntRect(); }
    sf::IntRect getOverlayRect() const { return m_overlaySprite ? m_overlaySprite->getTextureRect() : sf::IntRect(); }
    sf::Color getRarityColor() const { return m_rarityColor; }

    sf::FloatRect getGlobalBounds() const {
        if (m_baseSprite) {
            return getTransform().transformRect(m_baseSprite->getLocalBounds());
        }
        return sf::FloatRect();
    }

protected:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    friend class Animation;
    const sf::Texture* m_baseTexture;
    const sf::Texture* m_layoutTexture;

    std::optional<sf::Sprite> m_baseSprite;
    std::optional<sf::Sprite> m_overlaySprite;

    sf::Vector2i m_gridSize;
    sf::Color m_rarityColor = sf::Color::Transparent;
    int m_fortificationLevel = 0;

};
