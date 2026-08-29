#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include "map/ChunkedTileMap.h"
#include "DecorSystem.h"
#include "../engine/IRenderable.h"

// Definition of Portal struct moved here (or we can keep it inside WorldManager if private, but it's used for transitions)
struct Portal : public IRenderable {
    sf::FloatRect bounds;
    std::string targetWorldID; 
    sf::Vector2f spawnPos;
    std::unique_ptr<sf::Sprite> sprite;
    sf::RectangleShape debugShape; // Backup visual
    
    // Animation
    float animTimer = 0.f;
    int currentFrame = 0;
    
    Portal() = default; // Unique ptr defaults to null, so this is valid
    Portal(Portal&&) = default; // Move only
    Portal& operator=(Portal&&) = default;

    // Remove copying for safety (unique_ptr handles it by deleting copy, but being explicit is nice)
    Portal(const Portal&) = delete;
    Portal& operator=(const Portal&) = delete;

    // IRenderable implementation
    RenderType getRenderType() const override { return RenderType::Generic; }
    void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const override;
    const sf::Drawable* getDrawable() const override { return sprite.get(); }
    
    // Sort at the base + configurable offset
    float getY() const { return bounds.position.y + bounds.size.y - cfg::YSorting::PORTAL; } 
};

class WorldManager {
public:
    WorldManager(ResourceManager& res);

    // Loads a level by ID using WorldRegistry logic
    // Returns true on success
    bool loadLevel(const std::string& worldID, ResourceManager& res);

    void update(sf::Time dt);
    
    // Draw methods
    void drawMap(sf::RenderTarget& target, const sf::View& view);
    void drawDecorBottomLayer(sf::RenderTarget& target); // Static decor (flowers, etc)
    void drawDebugPortals(sf::RenderTarget& target);

    // Queries
    sf::Vector2u getMapSizePx() const;
    const std::string& getCurrentWorldID() const { return mCurrentWorldID; }
    const ChunkedTileMap& getMap() const { return mMap; }
    DecorSystem& getDecorSystem() { return mDecorSystem; } 
    const std::vector<std::unique_ptr<Portal>>& getPortals() const { return mPortals; }

    // Portal logic
    // Returns a pointer to the portal if collision occurs, or nullptr
    const Portal* checkPortalCollision(const sf::FloatRect& bounds) const;

private:
    void loadPortals(const std::string& filename, ResourceManager& res);

private:
    std::string    mCurrentWorldID;
    ChunkedTileMap mMap;
    DecorSystem    mDecorSystem;
    std::vector<std::unique_ptr<Portal>> mPortals;
};
