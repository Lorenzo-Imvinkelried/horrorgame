#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class BitmapText : public sf::Drawable, public sf::Transformable {
public:
    BitmapText();

    void setTexture(const sf::Texture* texture);
    void setString(const std::string& str);
    const std::string& getString() const { return mString; } // [NEW]
    void setColor(sf::Color color);
    sf::Color getColor() const { return mColor; } // [NEW]
    void setShadowOffset(sf::Vector2f offset);
    void setShadowColor(sf::Color color);
    
    // Bounds for centering
    sf::FloatRect getLocalBounds() const;
    sf::FloatRect getGlobalBounds() const;
    float getWidth() const { return mTextWidth; } // [NEW] Logical width including spaces
    const sf::VertexArray& getVertices() const { return mVertices; } // [NEW] Expose for batching

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    void updateGeometry();

private:
    const sf::Texture* mTexture;
    sf::VertexArray mVertices;
    std::string mString;
    sf::Color mColor;
    float mTextWidth = 0.f; // [NEW]
    
    // Shadow config
    sf::Vector2f mShadowOffset;
    sf::Color mShadowColor;

    // Asset constants (User Specified)
    // Asset constants (User Specified)
    static const int GLYPH_WIDTH = 3;
    static const int GLYPH_HEIGHT = 5;
};
