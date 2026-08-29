#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include "core/graphics/BitmapText.h"

class DebugOverlaySystem {
public:
    struct DebugAsset {
        std::unique_ptr<sf::Sprite> sprite;
        std::unique_ptr<sf::Texture> texture;
        BitmapText label;
        std::string texturePath;
        sf::Vector2f position;
    };

    DebugOverlaySystem();
    ~DebugOverlaySystem() = default;

    void loadAssets(const std::string& configPath);
    void draw(sf::RenderTarget& target);

private:
    std::vector<DebugAsset> mAssets;
    sf::Texture mFontTexture;
};
