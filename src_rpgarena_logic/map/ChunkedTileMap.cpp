#include "ChunkedTileMap.h"
#include "Config.h"   // << usa cfg::TILESET_SMOOTH y cfg::TEX_EPS
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

// --- Helper: intersección de AABB para SFML 3 (FloatRect con position/size) ---
static bool rectIntersects(const sf::FloatRect& a, const sf::FloatRect& b)
{
    const float ax1 = a.position.x;
    const float ay1 = a.position.y;
    const float ax2 = a.position.x + a.size.x;
    const float ay2 = a.position.y + a.size.y;

    const float bx1 = b.position.x;
    const float by1 = b.position.y;
    const float bx2 = b.position.x + b.size.x;
    const float by2 = b.position.y + b.size.y;

    return !(ax2 <= bx1 || bx2 <= ax1 || ay2 <= by1 || by2 <= ay1);
}
// -----------------------------------------------------------------------------

bool ChunkedTileMap::loadCSV(const std::string& csvPath) {
    std::ifstream f(csvPath);
    if (!f) return false;

    std::string line;
    mData.clear(); mRows = mCols = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        unsigned cols = 0;
        while (std::getline(ss, cell, ',')) {
            if (!cell.empty()) {
                mData.push_back(std::stoi(cell)); // -1 = vacío
                ++cols;
            }
        }
        if (mCols == 0) mCols = cols;
        mRows++;
    }
    return (mRows > 0 && mCols > 0);
}

bool ChunkedTileMap::load(const std::string& tilesetPath,
                          const std::string& csvPath,
                          unsigned tileSize,
                          unsigned chunkSize)
{
    mTileSize  = tileSize;
    mChunkSize = chunkSize;

    if (!mTileset.loadFromFile(tilesetPath)) {
        std::cerr << "[ChunkedTileMap] No pude cargar tileset: " << tilesetPath << "\n";
        return false;
    }
    // Aplicar config (pixel-art recomendado: false)
    mTileset.setSmooth(cfg::Map::TILESET_SMOOTH);

    if (!loadCSV(csvPath)) {
        std::cerr << "[ChunkedTileMap] No pude leer CSV: " << csvPath << "\n";
        return false;
    }

    const auto texSize = mTileset.getSize();
    mTilesPerRow = texSize.x / mTileSize;

    // preparar grilla de chunks
    mChunkCols = (mCols + mChunkSize - 1) / mChunkSize;
    mChunkRows = (mRows + mChunkSize - 1) / mChunkSize;
    mChunks.resize(static_cast<std::size_t>(mChunkCols) * mChunkRows);

    buildChunks();
    precalculateTileColors();
    return true;
}


void ChunkedTileMap::buildChunks() {
    for (unsigned cy = 0; cy < mChunkRows; ++cy) {
        for (unsigned cx = 0; cx < mChunkCols; ++cx) {
            Chunk& chunk = mChunks[cy * mChunkCols + cx];
            chunk.originTile = sf::Vector2i{
                static_cast<int>(cx * mChunkSize),
                static_cast<int>(cy * mChunkSize)
            };

            // tamaño real del chunk (puede ser menor en los bordes)
            const unsigned maxW = std::min(mChunkSize, mCols - (unsigned)chunk.originTile.x);
            const unsigned maxH = std::min(mChunkSize, mRows - (unsigned)chunk.originTile.y);
            chunk.sizeTiles = { maxW, maxH };

            // contar cuántos triángulos necesitamos
            const std::size_t tileCount = static_cast<std::size_t>(maxW) * maxH;
            chunk.vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
            chunk.vertices.resize(tileCount * 6);

            // construir vértices
            std::size_t v = 0;
            chunk.empty = true;

            for (unsigned ty = 0; ty < maxH; ++ty) {
                for (unsigned tx = 0; tx < maxW; ++tx) {
                    const unsigned mapX = (unsigned)chunk.originTile.x + tx;
                    const unsigned mapY = (unsigned)chunk.originTile.y + ty;
                    const int tileIndex = mData[mapY * mCols + mapX];

                    if (tileIndex < 0) {
                        // “vacío”: vértices degenerados (no aportan nada visible)
                        const sf::Vector2f p{0.f, 0.f};
                        for (int i = 0; i < 6; ++i) {
                            chunk.vertices[v + i].position = p;
                            chunk.vertices[v + i].texCoords = p;
                        }
                        v += 6;
                        continue;
                    }

                    chunk.empty = false;

                    const unsigned tu = (unsigned)tileIndex % mTilesPerRow;
                    const unsigned tv = (unsigned)tileIndex / mTilesPerRow;

                    const float px = static_cast<float>(mapX * mTileSize);
                    const float py = static_cast<float>(mapY * mTileSize);

                    const sf::Vector2f p0{px,                 py};
                    const sf::Vector2f p1{px + mTileSize,     py};
                    const sf::Vector2f p2{px + mTileSize,     py + mTileSize};
                    const sf::Vector2f p3{px,                 py + mTileSize};

                    // Coordenadas de textura con “epsilon” para evitar costuras
                    const float tx0 = static_cast<float>(tu * mTileSize);
                    const float ty0 = static_cast<float>(tv * mTileSize);
                    const float e   = cfg::Map::TEX_EPS;

                    const sf::Vector2f t0{tx0 + e,               ty0 + e};
                    const sf::Vector2f t1{tx0 + mTileSize - e,   ty0 + e};
                    const sf::Vector2f t2{tx0 + mTileSize - e,   ty0 + mTileSize - e};
                    const sf::Vector2f t3{tx0 + e,               ty0 + mTileSize - e};

                    // triángulo 1
                    chunk.vertices[v+0].position = p0; chunk.vertices[v+0].texCoords = t0;
                    chunk.vertices[v+1].position = p1; chunk.vertices[v+1].texCoords = t1;
                    chunk.vertices[v+2].position = p2; chunk.vertices[v+2].texCoords = t2;
                    // triángulo 2
                    chunk.vertices[v+3].position = p0; chunk.vertices[v+3].texCoords = t0;
                    chunk.vertices[v+4].position = p2; chunk.vertices[v+4].texCoords = t2;
                    chunk.vertices[v+5].position = p3; chunk.vertices[v+5].texCoords = t3;

                    v += 6;
                }
            }
        }
    }
}

void ChunkedTileMap::drawVisible(sf::RenderTarget& target, const sf::View& view) const {
    // Rectángulo visible (frustum) en coordenadas de mundo
    const sf::FloatRect frustum(
        view.getCenter() - view.getSize() * 0.5f,  // position
        view.getSize()                              // size
    );

    sf::RenderStates states;
    states.texture = &mTileset;

    // Calcular matemáticamente el rango de chunks visibles
    float chunkSizePx = static_cast<float>(mChunkSize * mTileSize);
    
    int startX = std::max(0, static_cast<int>(std::floor(frustum.position.x / chunkSizePx)));
    int endX   = std::min(static_cast<int>(mChunkCols) - 1, static_cast<int>(std::floor((frustum.position.x + frustum.size.x) / chunkSizePx)));
    int startY = std::max(0, static_cast<int>(std::floor(frustum.position.y / chunkSizePx)));
    int endY   = std::min(static_cast<int>(mChunkRows) - 1, static_cast<int>(std::floor((frustum.position.y + frustum.size.y) / chunkSizePx)));

    for (int cy = startY; cy <= endY; ++cy) {
        for (int cx = startX; cx <= endX; ++cx) {
            const Chunk& chunk = mChunks[static_cast<std::size_t>(cy) * mChunkCols + cx];
            if (chunk.empty) continue;
            if (!chunk.visible) continue;

            target.draw(chunk.vertices, states);
        }
    }
}

void ChunkedTileMap::setChunkVisible(int cx, int cy, bool visible) {
    if (cx >= 0 && cx < (int)mChunkCols && cy >= 0 && cy < (int)mChunkRows) {
        mChunks[cy * mChunkCols + cx].visible = visible;
    }
}

void ChunkedTileMap::setAllChunksVisible(bool visible) {
    for (auto& chunk : mChunks) {
        chunk.visible = visible;
    }
}

void ChunkedTileMap::drawChunk(sf::RenderTarget& target, int cx, int cy) const {
    if (cx >= 0 && cx < (int)mChunkCols && cy >= 0 && cy < (int)mChunkRows) {
        const Chunk& chunk = mChunks[static_cast<std::size_t>(cy) * mChunkCols + cx];
        if (!chunk.empty) {
            sf::RenderStates states;
            states.texture = &mTileset;
            target.draw(chunk.vertices, states);
        }
    }
}

void ChunkedTileMap::precalculateTileColors() {
    if (mTileset.getSize().x == 0) return;
    
    sf::Image img = mTileset.copyToImage();
    unsigned numTilesX = img.getSize().x / mTileSize;
    unsigned numTilesY = img.getSize().y / mTileSize;
    unsigned totalTiles = numTilesX * numTilesY;
    mTileColorCache.assign(totalTiles, sf::Color::Transparent);

    for (unsigned i = 0; i < totalTiles; ++i) {
        unsigned tu = i % numTilesX;
        unsigned tv = i / numTilesX;
        
        // Sample center pixel
        unsigned px = tu * mTileSize + mTileSize / 2;
        unsigned py = tv * mTileSize + mTileSize / 2;
        
        if (px < img.getSize().x && py < img.getSize().y) {
            mTileColorCache[i] = img.getPixel({px, py});
        }
    }
}

sf::Color ChunkedTileMap::getColorAtWorldPos(sf::Vector2f worldPos) const {
    int col = static_cast<int>(worldPos.x / mTileSize);
    int row = static_cast<int>(worldPos.y / mTileSize);
    
    if (col >= 0 && col < static_cast<int>(mCols) && row >= 0 && row < static_cast<int>(mRows)) {
        int tileID = mData[row * mCols + col];
        if (tileID >= 0 && tileID < static_cast<int>(mTileColorCache.size())) {
            return mTileColorCache[tileID];
        }
    }
    return sf::Color::Transparent;
}

