#pragma once
#include <SFML/Graphics.hpp>
#include "map/ChunkedTileMap.h"
#include "../systems/DecorSystem.h"
#include "../engine/ResourceManager.h"

// Simple base class interface if UIPanel doesn't exist separately
// or assumes specific methods. Based on InventoryPanel usage:
class TerrainDeformSystem;

class MapPanel {
public:
    MapPanel();
    void load(ResourceManager& res);
    
    // Updates internal texture (call when opening or periodically)
    void updateTexture(const ChunkedTileMap& map, const DecorSystem& decor, sf::Vector2f playerPos, const class EntityManager* entityMgr = nullptr, const TerrainDeformSystem* terrainDeform = nullptr);

    void draw(sf::RenderTarget& target);

    // Window / Panel Logic
    void setPosition(sf::Vector2f pos);
    sf::Vector2f getPosition() const { return mPosition; }
    sf::FloatRect getBounds() const;

    // Input
    void onMousePress(sf::Vector2f mousePos);
    void onMouseRelease();
    void onMouseMove(sf::Vector2f mousePos);

    void toggle() { mIsOpen = !mIsOpen; if (!mIsOpen) mIsBeingDragged = false; }
    bool isOpen() const { return mIsOpen; }
    void open() { mIsOpen = true; }
    void close() { mIsOpen = false; mIsBeingDragged = false; }

private:
    bool mIsOpen = false;
    sf::Vector2f mPosition;
    sf::Vector2f mSize;
    
    // Dragging
    bool mIsBeingDragged = false;
    sf::Vector2f mDragOffset;

    // Rendering
    sf::RenderTexture mMapTexture; // The offscreen buffer
    sf::Sprite mMapSprite;         // The sprite to draw the buffer
    sf::View mMapView;
    sf::RectangleShape mBackground;
    std::optional<sf::Sprite> mFrameBgSprite; // [NEW] Fondo manual UI
    std::optional<sf::Sprite> mPlayerMarkerSprite; // [NEW] Icono PNG para el jugador en el mapa
    
    const sf::Texture* mMinimapDotsTex = nullptr;
    sf::CircleShape mPlayerMarker;
};

