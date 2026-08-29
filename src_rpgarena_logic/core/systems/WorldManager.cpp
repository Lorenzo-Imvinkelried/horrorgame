#include "WorldManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "../engine/WorldRegistry.h"
#include "Config.h"
#include "utils/TinyJson.h"

WorldManager::WorldManager(ResourceManager& res)
    : mDecorSystem(res) 
{
}

bool WorldManager::loadLevel(const std::string& worldID, ResourceManager& res) {
    mCurrentWorldID = worldID;
    // [REGISTRY]
    const WorldData& worldData = WorldRegistry::get(worldID);
    if (worldData.map.empty()) {
        std::cerr << "[WorldManager] CRITICAL: Failed to find world data for ID: " << worldID << "\n";
        return false;
    }

    // 1. Load Map
    if (!mMap.load(worldData.tileset, worldData.map, cfg::Map::TILE_SIZE, cfg::Map::CHUNK_SIZE)) {
        std::cerr << "[WorldManager] Error loading map: " << worldData.map << "\n";
        return false;
    }

    // 2. Load Decor
    // Calculate size in tiles
    sf::Vector2u mapPx = mMap.mapSizePx();
    unsigned wTiles = mapPx.x / cfg::Map::TILE_SIZE;
    unsigned hTiles = mapPx.y / cfg::Map::TILE_SIZE;

    mDecorSystem.clear(); // Important when switching levels
    if (!mDecorSystem.loadFromCsv(worldData.decor, wTiles, hTiles, cfg::Map::TILE_SIZE)) {
        std::cerr << "[WorldManager] Warning: Failed to load decor: " << worldData.decor << "\n";
    }

    // 3. Load Portals
    // Convention: mapfile.csv -> mapfile_portals.json
    // e.g. assets/maps/level1.csv -> assets/maps/level1_portals.json
    std::string mapFile = worldData.map;
    std::string portalFile = mapFile.substr(0, mapFile.find_last_of('.')) + "_portals.json";
    loadPortals(portalFile, res);

    return true;
}

void WorldManager::loadPortals(const std::string& filename, ResourceManager& res) {
    mPortals.clear();
    
    std::string portalJsonFile = filename;
    if (portalJsonFile.size() >= 4 && portalJsonFile.substr(portalJsonFile.size() - 4) == ".txt") {
        portalJsonFile = portalJsonFile.substr(0, portalJsonFile.size() - 4) + ".json";
    }

    sf::Texture& portalTex = res.getTexture("assets/textures/entorno/vfx_vortex_220px.png");

    json::Value root = json::parseFile(portalJsonFile);
    if (root.type != json::Type::Array) {
        std::cerr << "[WorldManager] WARN: Could not load portals JSON: " << portalJsonFile << "\n";
        return;
    }

    for (const auto& val : root.asArray()) {
        if (val.type != json::Type::Object) continue;
        const auto& obj = val.asObject();

        if (obj.count("x") && obj.count("y") && obj.count("w") && obj.count("h") &&
            obj.count("targetWorldID") && obj.count("spawnX") && obj.count("spawnY")) {
            
            float x = static_cast<float>(obj.at("x").asDouble());
            float y = static_cast<float>(obj.at("y").asDouble());
            float w = static_cast<float>(obj.at("w").asDouble());
            float h = static_cast<float>(obj.at("h").asDouble());
            std::string targetID = obj.at("targetWorldID").asString();
            float sx = static_cast<float>(obj.at("spawnX").asDouble());
            float sy = static_cast<float>(obj.at("spawnY").asDouble());

            auto p = std::make_unique<Portal>();
            p->bounds = sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(w, h));
            p->targetWorldID = targetID;
            p->spawnPos = sf::Vector2f(sx, sy);
            
            // Visuals
            p->sprite = std::make_unique<sf::Sprite>(portalTex);
            
            // Portal de 220x220 (Pixel Perfect)
            int frameW = 220;
            int frameH = 220;
            
            p->sprite->setTextureRect(sf::IntRect({0, 0}, {frameW, frameH}));
            
            p->sprite->setOrigin({110.f, 110.f}); 
            p->sprite->setPosition({x + w / 2.f, y + h / 2.f}); 
            p->sprite->setScale({1.f, 1.f});

            // Debug shape
            p->debugShape = sf::RectangleShape(sf::Vector2f(w, h));
            p->debugShape.setPosition(sf::Vector2f(x, y));
            p->debugShape.setFillColor(sf::Color(0, 255, 255, 50)); 
            
            mPortals.push_back(std::move(p));
        }
    }
}

void WorldManager::update(sf::Time dt) {
    // Portal animation
    float s = dt.asSeconds();
    for (auto& p : mPortals) {
        if (!p->sprite) continue;
        
        p->animTimer += s;
        // ~15 FPS animation speed
        if (p->animTimer > 0.066f) {
            p->animTimer = 0.f;
            p->currentFrame = (p->currentFrame + 1) % 16;
            
            // Cada frame mide 220px en la fila
            p->sprite->setTextureRect(sf::IntRect({(int)p->currentFrame * 220, 0}, {220, 220}));
        }
    }
}

void Portal::getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const {
    if (!sprite) return;
    
    texture = &sprite->getTexture();
    sf::Transform trans = sprite->getTransform();
    const auto& seedRect = sprite->getTextureRect();
    sf::FloatRect rect(sf::Vector2f(seedRect.position), sf::Vector2f(seedRect.size));
    sf::Color col = sprite->getColor();

    // Local quad points
    float left = 0.f; 
    float top = 0.f;
    float right = rect.size.x;
    float bottom = rect.size.y;

    // Transform to World
    sf::Vector2f p1 = trans.transformPoint({left, top});
    sf::Vector2f p2 = trans.transformPoint({right, top});
    sf::Vector2f p3 = trans.transformPoint({right, bottom});
    sf::Vector2f p4 = trans.transformPoint({left, bottom});

    // UVs
    float u1 = rect.position.x; 
    float v1 = rect.position.y;
    float u2 = rect.position.x + rect.size.x; 
    float v2 = rect.position.y + rect.size.y;

    // Tri 1
    vertices.emplace_back(sf::Vertex{p1, col, sf::Vector2f(u1, v1)});
    vertices.emplace_back(sf::Vertex{p2, col, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{p4, col, sf::Vector2f(u1, v2)});

    // Tri 2
    vertices.emplace_back(sf::Vertex{p2, col, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{p3, col, sf::Vector2f(u2, v2)});
    vertices.emplace_back(sf::Vertex{p4, col, sf::Vector2f(u1, v2)});
}

void WorldManager::drawMap(sf::RenderTarget& target, const sf::View& view) {
    mMap.drawVisible(target, view);
}

void WorldManager::drawDecorBottomLayer(sf::RenderTarget& target) {
    mDecorSystem.drawStaticLayer(target);
}

void WorldManager::drawDebugPortals(sf::RenderTarget& target) {
    for (const auto& p : mPortals) {
        if (p->sprite) target.draw(*p->sprite);
        // target.draw(p->debugShape);
    }
}

sf::Vector2u WorldManager::getMapSizePx() const {
    return mMap.mapSizePx();
}

const Portal* WorldManager::checkPortalCollision(const sf::FloatRect& bounds) const {
    for (const auto& p : mPortals) {
        if (p->bounds.findIntersection(bounds)) {
            return p.get();
        }
    }
    return nullptr;
}
