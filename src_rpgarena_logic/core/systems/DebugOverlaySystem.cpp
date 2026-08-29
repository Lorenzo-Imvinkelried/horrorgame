#include "DebugOverlaySystem.h"
#include "utils/TinyJson.h"
#include <iostream>
#include <cmath>

DebugOverlaySystem::DebugOverlaySystem() {
}

void DebugOverlaySystem::loadAssets(const std::string& configPath) {
    if (!mFontTexture.loadFromFile("assets/fonts/font.png")) {
        std::cerr << "[DebugOverlaySystem] ERROR: Failed to load assets/fonts/font.png\n";
    }

    mAssets.clear();
    
    json::Value root = json::parseFile(configPath);
    if (root.type != json::Type::Array) {
        std::cerr << "[DebugOverlaySystem] ERROR: Failed to load debug assets from " << configPath << " (not an array)\n";
        return;
    }

    const auto& assetList = root.asArray();
    for (const auto& assetVal : assetList) {
        const auto& obj = assetVal.asObject();
        
        DebugAsset asset;
        asset.texturePath = obj.at("texture").asString();
        asset.position.x = (float)obj.at("x").asDouble();
        asset.position.y = (float)obj.at("y").asDouble();

        asset.texture = std::make_unique<sf::Texture>();
        if (asset.texture->loadFromFile(asset.texturePath)) {
            asset.sprite = std::make_unique<sf::Sprite>(*asset.texture);
            
            // Center bottom as origin for reference pillars
            sf::FloatRect bounds = asset.sprite->getLocalBounds();
            asset.sprite->setOrigin({std::round(bounds.size.x * 0.5f), std::round(bounds.size.y)});
            asset.sprite->setPosition({std::round(asset.position.x), std::round(asset.position.y)});

            // Label
            asset.label.setTexture(&mFontTexture);
            
            // Extract filename from path
            std::string filename = asset.texturePath;
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }
            asset.label.setString(filename);
            asset.label.setColor(sf::Color::White);
            asset.label.setScale({1.f, 1.f}); // Original pixel perfect size

            // Center label
            sf::FloatRect lBounds = asset.label.getLocalBounds();
            asset.label.setOrigin({std::round(lBounds.size.x * 0.5f), std::round(lBounds.size.y)});
            // Position it above the sprite
            asset.label.setPosition({std::round(asset.position.x), std::round(asset.position.y - bounds.size.y - 5.f)});

            mAssets.push_back(std::move(asset));
        } else {
            std::cerr << "[DebugOverlaySystem] ERROR: Could not load texture: " << asset.texturePath << "\n";
        }
    }
    
    std::cout << "[DebugOverlaySystem] Loaded " << mAssets.size() << " debug assets.\n";
}

void DebugOverlaySystem::draw(sf::RenderTarget& target) {
    for (auto& asset : mAssets) {
        if (asset.sprite) {
            target.draw(*asset.sprite);
            target.draw(asset.label);
        }
    }
}
