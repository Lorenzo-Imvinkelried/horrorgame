#include "DecorSystem.h"
#include "utils/TinyJson.h"
#include <fstream>
#include <sstream>
#include <numeric> 
#include <algorithm> 
#include <iostream>
#include <array>

DecorSystem::DecorSystem(ResourceManager& res)
    : mRes(res)
{
    loadShadowConfig();
}

std::string DecorSystem::keyForId(int id) const {
    switch (id) {
        case 1: return "planta_1";
        case 2: return "planta_2";
        case 3: return "arbol_1";
        case 4: return "arbol_2";
        case 5: return "arbol_3";
        case 6: return "arbol_4";
        case 7: return "arbol_5";
        case 8: return "arbol_6";
        default: return "arbol_" + std::to_string(id);
    }
}

void DecorSystem::loadShadowConfig(const std::string& path) {
    mTreeShadowConfigs.clear();
    mDefaultShadowConfig = TreeShadowConfig{};

    json::Value root = json::parseFile(path);
    if (root.type != json::Type::Object) {
        std::cerr << "[DecorSystem] WARNING: No se pudo cargar " << path << " o formato invalido. Usando valores por defecto.\n";
        return;
    }

    const auto& rootObj = root.asObject();

    if (rootObj.count("defaults") && rootObj.at("defaults").type == json::Type::Object) {
        const auto& defObj = rootObj.at("defaults").asObject();
        if (defObj.count("offset_x")) mDefaultShadowConfig.offsetX = (float)defObj.at("offset_x").asDouble();
        if (defObj.count("offset_y")) mDefaultShadowConfig.offsetY = (float)defObj.at("offset_y").asDouble();
        if (defObj.count("scale_x")) mDefaultShadowConfig.scaleX = (float)defObj.at("scale_x").asDouble();
        if (defObj.count("scale_y")) mDefaultShadowConfig.scaleY = (float)defObj.at("scale_y").asDouble();
    }

    if (rootObj.count("trees") && rootObj.at("trees").type == json::Type::Object) {
        const auto& treesObj = rootObj.at("trees").asObject();
        for (const auto& [treeKey, treeVal] : treesObj) {
            if (treeVal.type != json::Type::Object) continue;
            const auto& tObj = treeVal.asObject();
            TreeShadowConfig cfg = mDefaultShadowConfig;
            if (tObj.count("offset_x")) cfg.offsetX = (float)tObj.at("offset_x").asDouble();
            if (tObj.count("offset_y")) cfg.offsetY = (float)tObj.at("offset_y").asDouble();
            if (tObj.count("scale_x")) cfg.scaleX = (float)tObj.at("scale_x").asDouble();
            if (tObj.count("scale_y")) cfg.scaleY = (float)tObj.at("scale_y").asDouble();
            mTreeShadowConfigs[treeKey] = cfg;
        }
    }
    std::cout << "[DecorSystem] Cargada configuracion de sombras de vegetacion (" << mTreeShadowConfigs.size() << " arboles configurados).\n";
}

TreeShadowConfig DecorSystem::getShadowConfigForId(int id) const {
    std::string key = keyForId(id);
    auto it = mTreeShadowConfigs.find(key);
    if (it != mTreeShadowConfigs.end()) {
        return it->second;
    }
    std::string numKey = std::to_string(id);
    it = mTreeShadowConfigs.find(numKey);
    if (it != mTreeShadowConfigs.end()) {
        return it->second;
    }
    return mDefaultShadowConfig;
}

bool DecorSystem::loadCsvGrid(const std::string& csvPath,
                              std::vector<int>& outData,
                              int expectedWidth,
                              int expectedHeight)
{
    std::ifstream in(csvPath);
    if (!in.is_open()) {
        std::cerr << "[DecorSystem] ERROR: no pude abrir " << csvPath << "\n";
        return false;
    }

    outData.clear();
    outData.reserve(expectedWidth * expectedHeight);

    std::string line;
    int rowCount = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        int colCount = 0;

        while (std::getline(ss, cell, ',')) {
            if (!cell.empty()) {
                int v = std::stoi(cell);
                outData.push_back(v);
                ++colCount;
            }
        }

        if (colCount != expectedWidth) {
            std::cerr << "[DecorSystem] WARNING: fila " << rowCount
                      << " tiene " << colCount << " columnas, esperaba "
                      << expectedWidth << "\n";
        }

        ++rowCount;
    }

    if (rowCount != expectedHeight) {
        std::cerr << "[DecorSystem] WARNING: el CSV tiene " << rowCount
                  << " filas, esperaba " << expectedHeight << "\n";
    }

    return !outData.empty();
}

std::string DecorSystem::texturePathForId(int id) const {
    switch (id) {
        case 1: return "assets/textures/entorno/vegetacion/planta_1.png";
        case 2: return "assets/textures/entorno/vegetacion/planta_2.png";
        case 3: return "assets/textures/entorno/vegetacion/arbol_1.png";
        case 4: return "assets/textures/entorno/vegetacion/arbol_2.png";
        case 5: return "assets/textures/entorno/vegetacion/arbol_3.png";
        case 6: return "assets/textures/entorno/vegetacion/arbol_4.png";
        default: return "";
    }
}

bool DecorSystem::isBlockingId(int id) const {
    switch (id) {
        case 3:
        case 4:
        case 5:
        case 6:
            return true;
        default:
            return false;
    }
}

static float scaleForId(int id) {                 
    switch (id) {                                 
        case 1:                                   
        case 2:                                   
            return cfg::Decor::SCALE_SMALL_PLANT;                          
        case 3:                                   
        case 4:
        case 5:
        case 6:
        case 7: // Assuming new IDs might behave like trees
            return cfg::Decor::SCALE_TREE;                          
        default:
            return cfg::Decor::SCALE_DEFAULT;
    }
}

bool DecorSystem::loadFromCsv(const std::string& csvPath,
                              int mapWidth,
                              int mapHeight,
                              float tileSize)
{
    loadShadowConfig("assets/data/shadow_vegetacion.json");

    std::vector<int> grid;
    if (!loadCsvGrid(csvPath, grid, mapWidth, mapHeight)) {
        return false;
    }

    mInstances.clear();
    mSpatialGrid.clear();
    mRenderGrid.clear(); // [OPTIMIZATION] Clear render grid
    
    // --- LIMPIEZA DE BATCHES ---
    mStaticBatches.clear(); 
    // ---------------------------

    int totalStatic = 0; // Contador debug

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            int index = y * mapWidth + x;
            int id    = grid[index];
            if (id <= 0) continue; 

            std::string texPath = texturePathForId(id);
            if (texPath.empty()) continue;

            sf::Texture& tex = mRes.getTexture(texPath);
            bool blocking = isBlockingId(id);
            float s = scaleForId(id);

            // Calculamos posición mundo
            sf::Vector2f worldPos(                       
                x * tileSize + tileSize * 0.5f,          
                y * tileSize + tileSize                  
            );

            TreeShadowConfig shadowCfg = getShadowConfigForId(id);
            DecorInstance inst(tex, blocking, shadowCfg);
            
            sf::Vector2u sz = tex.getSize();
            // Ajuste de origen para vegetación (árboles y plantas)
            inst.sprite.setOrigin({
                static_cast<float>(sz.x) * 0.5f,         
                static_cast<float>(sz.y) - cfg::YSorting::DECOR_TREE          
            });
            inst.sprite.setScale({s, s});
            inst.sprite.setPosition(worldPos);

            if (blocking) {
                // Objetos sólidos (árboles): añaden collider de tronco al spatial grid
                float trunkWidth  = cfg::Decor::TRUNK_WIDTH; 
                float trunkHeight = cfg::Decor::TRUNK_HEIGHT; 

                sf::Vector2f trunkPos{
                    worldPos.x - (trunkWidth * 0.5f),
                    worldPos.y - (trunkHeight * 0.5f)
                };
                
                sf::FloatRect trunkRect(trunkPos, {trunkWidth, trunkHeight});

                int cellX = static_cast<int>(trunkRect.position.x) / cfg::Decor::GRID_CELL_SIZE;
                int cellY = static_cast<int>(trunkRect.position.y) / cfg::Decor::GRID_CELL_SIZE;
                mSpatialGrid[{cellX, cellY}].push_back(trunkRect);
            }

            mInstances.push_back(std::move(inst));
        }
    }

    std::cout << "[DecorSystem] Cargados " << mInstances.size() << " elementos de vegetacion (con viento, sombra y contorno) desde " << csvPath << "\n";

    // [OPTIMIZATION] Populate Render Grid
    // We do this AFTER pushing everything to mInstances to ensure pointers are stable.
    for (const auto& inst : mInstances) {
        sf::FloatRect bounds = inst.sprite.getGlobalBounds();
        sf::Vector2f center = { bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f };
        
        // We register the instance in the cell where its center resides.
        // Or better: check corners/bounds? Center is usually enough for 256px cells and small trees.
        // For large objects, we might register in multiple cells, but trees are usually < 256px.
        // Let's stick to center point for O(1) insertion.
        int cellX = static_cast<int>(center.x) / cfg::Decor::GRID_CELL_SIZE;
        int cellY = static_cast<int>(center.y) / cfg::Decor::GRID_CELL_SIZE;
        
        mRenderGrid[{cellX, cellY}].push_back(&inst);
    }

    return true;
}

// --- NUEVA FUNCIÓN DE DIBUJADO OPTIMIZADO ---
void DecorSystem::drawStaticLayer(sf::RenderTarget& target) const {
    // Recorremos el mapa de batches
    // key: textura*, value: VertexArray
    for (const auto& pair : mStaticBatches) {
        const sf::Texture* tex = pair.first;
        const sf::VertexArray& varray = pair.second;
        
        // RenderStates nos permite decirle a la GPU qué textura usar
        sf::RenderStates states;
        states.texture = tex;
        
        target.draw(varray, states);
    }
}

// --- NUEVA FUNCIÓN OPTIMIZADA (OPTIMIZATION 3) ---
void DecorSystem::getObstaclesNearby(sf::Vector2f pos, std::vector<sf::FloatRect>& outObstacles) const {
    outObstacles.clear(); 
    // No reservamos aquí porque se supone que el vector viene cacheado, pero por seguridad:
    // outObstacles.reserve(20); 

    // Calculamos en qué celda está el jugador
    int px = static_cast<int>(pos.x) / cfg::Decor::GRID_CELL_SIZE;
    int py = static_cast<int>(pos.y) / cfg::Decor::GRID_CELL_SIZE;

    // Recorremos las 9 celdas alrededor del jugador (3x3)
    // Desde (x-1, y-1) hasta (x+1, y+1)
    for (int nx = px - 1; nx <= px + 1; ++nx) {
        for (int ny = py - 1; ny <= py + 1; ++ny) {
            
            // Buscamos si existe esa celda en el mapa
            auto it = mSpatialGrid.find({nx, ny});
            if (it != mSpatialGrid.end()) {
                // Si existe, copiamos sus rectángulos a nuestra lista local
                const auto& rectsInCell = it->second;
                outObstacles.insert(outObstacles.end(), rectsInCell.begin(), rectsInCell.end());
            }
        }
    }
}

void DecorSystem::getVisibleInstances(const sf::FloatRect& viewRect, std::vector<const DecorInstance*>& outVisible) const {
    /* 
       [OPTIMIZATION] Grid-based Culling
       Instead of iterating all mInstances (O(N)), we calculate which grid cells
       overlap with the viewRect and iterate only those (O(K)).
    */

    // 1. Calculate grid range
    int minX = static_cast<int>(viewRect.position.x) / cfg::Decor::GRID_CELL_SIZE;
    int minY = static_cast<int>(viewRect.position.y) / cfg::Decor::GRID_CELL_SIZE;
    int maxX = static_cast<int>(viewRect.position.x + viewRect.size.x) / cfg::Decor::GRID_CELL_SIZE;
    int maxY = static_cast<int>(viewRect.position.y + viewRect.size.y) / cfg::Decor::GRID_CELL_SIZE;

    // 2. Iterate relevant cells
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            auto it = mRenderGrid.find({x, y});
            if (it != mRenderGrid.end()) {
                const auto& cellInstances = it->second;
                
                // 3. Iterate instances in this cell
                for (const auto* inst : cellInstances) {
                     // Fine-grained check: Does the sprite actually overlap or is it contained?
                     // Using contains(position) as per original logic to avoid popping
                     // Note: viewRect passed here usually includes CULLING_MARGIN
                     if (viewRect.contains(inst->sprite.getPosition())) {
                         outVisible.push_back(inst);
                     }
                }
            }
        }
    }
}

// [PORTAL SYSTEM]
void DecorSystem::clear() {
    mInstances.clear();
    mSpatialGrid.clear();
    mRenderGrid.clear(); // [OPTIMIZATION] Clear render grid
    mStaticBatches.clear();
    mBlockingRects.clear();
}

bool DecorSystem::checkPixelCollision(sf::Vector2f screenPos, float groundY, sf::Vector2f velocity, float distanceTraveled) const {
    static std::map<const sf::Texture*, sf::Image> sDecorImageCache;

    int px = static_cast<int>(screenPos.x) / cfg::Decor::GRID_CELL_SIZE;
    int py = static_cast<int>(screenPos.y) / cfg::Decor::GRID_CELL_SIZE;

    // Check 3x3 neighbor cells
    for (int nx = px - 1; nx <= px + 1; ++nx) {
        for (int ny = py - 1; ny <= py + 1; ++ny) {
            auto it = mRenderGrid.find({nx, ny});
            if (it == mRenderGrid.end()) continue;

            for (const auto* inst : it->second) {
                if (!inst || !inst->blocking) continue;

                const sf::Sprite& sp = inst->sprite;
                float treeGroundY = sp.getPosition().y;
                float treeGroundX = sp.getPosition().x;

                // 1. 2.5D Depth Slice Check
                // The tree trunk occupies a depth slice around its ground base
                constexpr float kDepthSliceThickness = 10.f;
                float depthDiff = std::abs(groundY - treeGroundY);
                if (depthDiff > kDepthSliceThickness) {
                    // Out of depth slice: projectile is passing in front or behind in 3D depth!
                    continue;
                }

                // 2. Trunk column bounds check (width +/- 12px, vertical trunk from ground base up to 38px)
                float dx = std::abs(screenPos.x - treeGroundX);
                if (dx > 12.f) {
                    // Outside trunk horizontal footprint (passes freely through outer foliage/canopy)
                    continue;
                }
                if (screenPos.y < treeGroundY - 38.f || screenPos.y > treeGroundY + 8.f) {
                    // Outside vertical trunk column
                    continue;
                }

                // 3. Directional check for initial spawn immunity
                // If projectile is close to spawn and moving away from tree, don't collide
                if (distanceTraveled < 25.f && (velocity.x != 0.f || velocity.y != 0.f)) {
                    sf::Vector2f toTree(treeGroundX - screenPos.x, treeGroundY - groundY);
                    if (velocity.x * toTree.x + velocity.y * toTree.y <= 0.f) {
                        continue; // Moving away from tree: don't explode in face
                    }
                }

                // 4. Pixel-Perfect Alpha Check on Tree Texture
                const sf::Texture& tex = sp.getTexture();
                auto imgIt = sDecorImageCache.find(&tex);
                if (imgIt == sDecorImageCache.end()) {
                    sDecorImageCache[&tex] = tex.copyToImage();
                    imgIt = sDecorImageCache.find(&tex);
                }
                const sf::Image& img = imgIt->second;
                sf::Vector2u imgSize = img.getSize();

                sf::Vector2f localPos = sp.getInverseTransform().transformPoint(screenPos);
                int lx = static_cast<int>(std::round(localPos.x));
                int ly = static_cast<int>(std::round(localPos.y));

                if (lx >= 0 && lx < static_cast<int>(imgSize.x) &&
                    ly >= 0 && ly < static_cast<int>(imgSize.y)) {
                    sf::Color col = img.getPixel(sf::Vector2u(static_cast<unsigned int>(lx), static_cast<unsigned int>(ly)));
                    if (col.a > 50) {
                        return true; // Exact pixel hit on solid trunk!
                    }
                }
            }
        }
    }
    return false;
}