//HealthBarSystem.h
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "entities/Entity.h"
#include "../engine/ResourceManager.h" // [NEW] Required for load()
#include <unordered_map>
#include <string>


#include "../graphics/BitmapText.h" // [BITMAP]

// Constantes de espaciado (Lógicas)
static constexpr float NAME_OFFSET_Y = 40.f;
static constexpr float BAR_OFFSET_Y = 30.f;

class AggroSystem;

class HealthBarSystem {
public:
    HealthBarSystem(sf::Texture* fontTexture);
    void load(ResourceManager& res); // [NEW] Load textures

    void draw(sf::RenderTarget& target, 
              const sf::View& worldView, 
              const std::vector<std::unique_ptr<Entity>>& entities, 
              Entity* localPlayer,
              Entity* targetEntity,
              sf::Vector2f mouseWorldPos,
              const AggroSystem* aggroSystem = nullptr);

private:
    sf::Texture*     mFontTexture;
    sf::Texture*     mGreenTexture = nullptr;
    sf::Texture*     mRedTexture = nullptr;
    BitmapText       mNameText;
    // sf::RectangleShape mHealthBarBg; // Replaced by VertexArray
    // sf::RectangleShape mHealthBarFill; // Replaced by VertexArray
    
    sf::VertexArray mBgVertices;
    sf::VertexArray mFillVertices;
    sf::VertexArray mTextVertices;
    
    std::unordered_map<std::string, BitmapText> mTextCache;

    // (Las constantes ya no están aquí)
};