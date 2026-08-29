#include "MapPanel.h"
#include "Config.h" // [DEPENDENCY]
#include "core/systems/terrain/TerrainDeformSystem.h"
#include "core/managers/EntityManager.h"
#include "entities/player/Player.h"
#include <algorithm>
#include <iostream>

MapPanel::MapPanel() : mMapSprite(mMapTexture.getTexture()) {
    // Initial Position (Centered-ish or custom)
    mPosition = {100.f, 50.f}; 
    
    // [CONFIG] Use defined window size
    mSize = {cfg::Map::WINDOW_WIDTH, cfg::Map::WINDOW_HEIGHT}; 
    
    mBackground.setSize(mSize);
    mBackground.setFillColor(sf::Color(0, 0, 0, 200));
    mBackground.setOutlineThickness(1.f);
    mBackground.setOutlineColor(sf::Color(100, 100, 100));

    // RenderTexture size matches the inner content area
    // Safety check for size
    unsigned w = (unsigned)std::max(10.f, mSize.x - cfg::Map::WINDOW_INNER_OFFSET_X * 2.f);
    unsigned h = (unsigned)std::max(10.f, mSize.y - cfg::Map::WINDOW_INNER_OFFSET_Y - 10.f);
    
    // Re-create texture if size changed (or initial)
    if (mMapTexture.getSize().x != w || mMapTexture.getSize().y != h) {
        (void)mMapTexture.resize({w, h}); 
        mMapTexture.setSmooth(true);
    }

    mMapSprite.setTexture(mMapTexture.getTexture());
    
    // [CONFIG] Marker settings
    mPlayerMarker.setRadius(cfg::Map::MARKER_RADIUS);
    mPlayerMarker.setOrigin({cfg::Map::MARKER_RADIUS, cfg::Map::MARKER_RADIUS});
    mPlayerMarker.setFillColor(cfg::Map::MARKER_COLOR);
    mPlayerMarker.setOutlineColor(cfg::Map::MARKER_OUTLINE_COLOR);
    mPlayerMarker.setOutlineThickness(1.f);
}

void MapPanel::load(ResourceManager& res) {
    try {
        sf::Texture& tex = res.getTexture("assets/ui/map_frame_bg.png");
        mFrameBgSprite.emplace(tex);
    } catch (...) {}

    try {
        sf::Texture& tex = res.getTexture("assets/ui/dot_minimap.png");
        mPlayerMarkerSprite.emplace(tex);
        sf::FloatRect bounds = mPlayerMarkerSprite->getLocalBounds();
        mPlayerMarkerSprite->setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
    } catch (...) {}

    try {
        mMinimapDotsTex = &res.getTexture("assets/ui/minimap_dots.png");
    } catch (...) {
        mMinimapDotsTex = nullptr;
    }
}

void MapPanel::updateTexture(const ChunkedTileMap& map, const DecorSystem& decor, sf::Vector2f playerPos, const EntityManager* entityMgr, const TerrainDeformSystem* terrainDeform) {
    if (!mIsOpen) return;

    if (cfg::Map::DEBUG_VIEW_MAP_COMPLETE) {
        mSize = {1200.f, 700.f};
        mPosition = {83.f, 34.f};
    } else {
        mSize = {cfg::Map::WINDOW_WIDTH, cfg::Map::WINDOW_HEIGHT};
    }
    mBackground.setSize(mSize);

    unsigned w = (unsigned)std::max(10.f, mSize.x - cfg::Map::WINDOW_INNER_OFFSET_X * 2.f);
    unsigned h = (unsigned)std::max(10.f, mSize.y - cfg::Map::WINDOW_INNER_OFFSET_Y - 10.f);

    if (mMapTexture.getSize().x != w || mMapTexture.getSize().y != h) {
        (void)mMapTexture.resize({w, h}); 
        mMapTexture.setSmooth(true);
    }

    sf::Vector2u mapPx = map.mapSizePx();
    sf::Vector2f texSize = static_cast<sf::Vector2f>(mMapTexture.getSize());

    float zoomFactor = cfg::Map::DEFAULT_ZOOM;
    if (cfg::Map::DEBUG_VIEW_MAP_COMPLETE) {
        zoomFactor = (float)mapPx.x / texSize.x;
    }

    // 1. Setup View
    if (cfg::Map::DEBUG_VIEW_MAP_COMPLETE) {
        mMapView.setSize(static_cast<sf::Vector2f>(mapPx));
        mMapView.setCenter({mapPx.x / 2.f, mapPx.y / 2.f});
    } else {
        mMapView.setSize(texSize);
        mMapView.setCenter(playerPos);
        mMapView.zoom(zoomFactor); 

        // --- VIEW CLAMPING LOGIC (Limit to Map Bounds) ---
        sf::Vector2f viewSize = mMapView.getSize(); 
        sf::Vector2f center = mMapView.getCenter();

        // Clamp X
        if (viewSize.x >= mapPx.x) {
            center.x = mapPx.x / 2.f; // Center map if view is larger
        } else {
            float halfW = viewSize.x / 2.f;
            center.x = std::max(halfW, std::min(center.x, mapPx.x - halfW));
        }

        // Clamp Y
        if (viewSize.y >= mapPx.y) {
            center.y = mapPx.y / 2.f;
        } else {
            float halfH = viewSize.y / 2.f;
            center.y = std::max(halfH, std::min(center.y, mapPx.y - halfH));
        }
        
        mMapView.setCenter(center);
    }
    // --------------------------------------------------

    mMapTexture.setView(mMapView);
    
    // Clear with green color (RGB: 162, 169, 71)
    mMapTexture.clear(sf::Color(162, 169, 71)); 

    // Ensure Sprite uses the updated texture AND resets the texture rect to the new size
    mMapSprite.setTexture(mMapTexture.getTexture(), true); 
    mMapSprite.setTextureRect(sf::IntRect({0, 0}, {static_cast<int>(mMapTexture.getSize().x), static_cast<int>(mMapTexture.getSize().y)}));

    // 2. Draw Mobs using minimap_dots
    if (entityMgr && mMinimapDotsTex) {
        float scaleVal = cfg::Map::ZOOM_FACTOR * zoomFactor;

        for (const auto& ent : entityMgr->getActiveEntities()) {
            if (!ent || !ent->isAlive()) continue;
            if (dynamic_cast<const Player*>(ent.get())) continue; // Evitar dibujar al jugador como mob

            sf::Sprite dotSprite(*mMinimapDotsTex);
            if (ent->isFlashing()) {
                dotSprite.setTextureRect(sf::IntRect({1, 0}, {1, 1}));
            } else {
                dotSprite.setTextureRect(sf::IntRect({0, 0}, {1, 1}));
            }
            dotSprite.setScale({scaleVal, scaleVal});
            dotSprite.setOrigin({0.5f, 0.5f});
            dotSprite.setPosition(ent->getPosition());
            mMapTexture.draw(dotSprite);
        }
    } else if (entityMgr) {
        float scaledRadius = cfg::Map::MARKER_RADIUS * (zoomFactor / 3.0f);
        if (scaledRadius < 2.5f) scaledRadius = 2.5f;

        for (const auto& ent : entityMgr->getActiveEntities()) {
            if (!ent || !ent->isAlive()) continue;
            if (dynamic_cast<const Player*>(ent.get())) continue;

            sf::CircleShape mMarker(scaledRadius);
            mMarker.setOrigin({scaledRadius, scaledRadius});
            mMarker.setFillColor(ent->isFlashing() ? sf::Color::Yellow : sf::Color(220, 40, 40));
            mMarker.setOutlineColor(sf::Color(10, 10, 10));
            mMarker.setOutlineThickness(0.5f * (scaledRadius / 2.5f));
            mMarker.setPosition(ent->getPosition());
            mMapTexture.draw(mMarker);
        }
    }

    // 3. Draw Player Marker
    if (mPlayerMarkerSprite) {
        float uiZoom = cfg::Map::ZOOM_FACTOR;
        float scale = uiZoom * zoomFactor;
        mPlayerMarkerSprite->setScale({scale, scale});
        mPlayerMarkerSprite->setPosition(playerPos);
        mMapTexture.draw(*mPlayerMarkerSprite);
    } else {
        // Scale marker with zoom so it remains visible
        sf::CircleShape pMarker = mPlayerMarker;
        float scaledRadius = cfg::Map::MARKER_RADIUS * (zoomFactor / 2.0f); // Heuristic scaling
        if (scaledRadius < cfg::Map::MARKER_RADIUS) scaledRadius = cfg::Map::MARKER_RADIUS;
        
        pMarker.setRadius(scaledRadius);
        pMarker.setOrigin({scaledRadius, scaledRadius}); 
        pMarker.setPosition(playerPos);
        
        mMapTexture.draw(pMarker);
    }

    mMapTexture.display();
}

void MapPanel::draw(sf::RenderTarget& target) {
    if (!mIsOpen) return;

    // Update positions
    if (mFrameBgSprite && !cfg::Map::DEBUG_VIEW_MAP_COMPLETE) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        mFrameBgSprite->setPosition(mPosition);
        mFrameBgSprite->setScale({zoom, zoom}); // [ADDED] Escala al igual que el resto
        target.draw(*mFrameBgSprite);
    } else {
        mBackground.setPosition(mPosition);
        target.draw(mBackground);
        
        sf::RectangleShape titleBar({mSize.x, 30.f});
        titleBar.setPosition(mPosition);
        titleBar.setFillColor(sf::Color(50, 50, 50));
        target.draw(titleBar);
    }

    // El sprite del mapa (RenderTexture) lo posicionamos segun offsets definidos en config
    float mapOffX = cfg::Map::WINDOW_INNER_OFFSET_X;
    float mapOffY = cfg::Map::WINDOW_INNER_OFFSET_Y;
    mMapSprite.setPosition({mPosition.x + mapOffX, mPosition.y + mapOffY});

    // Draw Map Content
    target.draw(mMapSprite);
}

void MapPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

sf::FloatRect MapPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

void MapPanel::onMousePress(sf::Vector2f mousePos) {
    if (!mIsOpen) return;

    sf::FloatRect bounds(mPosition, mSize);
    if (bounds.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
    }
}

void MapPanel::onMouseRelease() {
    mIsBeingDragged = false;
}

void MapPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mIsBeingDragged) {
        mPosition = mousePos - mDragOffset;
    }
}
