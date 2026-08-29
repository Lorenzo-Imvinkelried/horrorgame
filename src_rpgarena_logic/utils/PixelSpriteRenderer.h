#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>

class PixelSpriteRenderer {
public:
    struct CacheKey {
        const sf::Texture* texture;
        sf::IntRect rect;

        bool operator<(const CacheKey& other) const {
            if (texture != other.texture) return texture < other.texture;
            if (rect.position.x != other.rect.position.x) return rect.position.x < other.rect.position.x;
            if (rect.position.y != other.rect.position.y) return rect.position.y < other.rect.position.y;
            if (rect.size.x != other.rect.size.x) return rect.size.x < other.rect.size.x;
            return rect.size.y < other.rect.size.y;
        }
    };

    // Extract and cache the sub-image of a texture rect
    static const sf::Image& getSubImage(const sf::Texture& texture, const sf::IntRect& rect);

    // Get (or generate and cache) a software-rotated texture for a given angle
    static const sf::Texture& getRotatedTexture(const sf::Texture& texture, const sf::IntRect& rect, 
                                                int angleKey, sf::Vector2f origin, sf::Vector2f& outDestOrigin);

    // Draw the rotated sprite using the software-rotated texture cache
    static void draw(sf::RenderTarget& target, const sf::Texture& texture, const sf::IntRect& rect, 
                     sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale, float angleDeg, 
                     sf::Color colorTint, sf::RenderStates states = sf::RenderStates::Default);
};
