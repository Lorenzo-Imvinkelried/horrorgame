#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class ChunkedTileMap : public sf::Drawable {
public:
    // tileSize: px por tile (32, 64, ...)
    // chunkSize: tiles por lado en cada chunk (ej. 64 → chunk 64×64 tiles)
    bool load(const std::string& tilesetPath,
              const std::string& csvPath,
              unsigned tileSize,
              unsigned chunkSize);

    // Dibuja solo los chunks visibles por 'view'
    void drawVisible(sf::RenderTarget& target, const sf::View& view) const;

    sf::Vector2u mapSizePx() const { return {mCols * mTileSize, mRows * mTileSize}; }

    sf::Color getColorAtWorldPos(sf::Vector2f worldPos) const;

    void setChunkVisible(int cx, int cy, bool visible);
    void setAllChunksVisible(bool visible);
    void drawChunk(sf::RenderTarget& target, int cx, int cy) const;

private:
    struct Chunk {
        sf::VertexArray vertices; // Triangles
        sf::Vector2i    originTile; // (col, row) del tile superior-izq del chunk
        sf::Vector2u    sizeTiles;  // ancho x alto en tiles reales (borde puede ser parcial)
        bool empty = true;
        bool visible = true;
    };

    void precalculateTileColors();
    bool loadCSV(const std::string& csvPath);
    void buildChunks();

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        // no se usa (llamar drawVisible)
    }

private:
    // datos del mapa
    std::vector<int> mData;
    unsigned mRows = 0, mCols = 0;

    // tileset
    sf::Texture mTileset;
    unsigned    mTileSize = 0;
    unsigned    mTilesPerRow = 0;
    std::vector<sf::Color> mTileColorCache;


    // chunking
    unsigned mChunkSize = 0; // en tiles
    unsigned mChunkCols = 0, mChunkRows = 0;
    std::vector<Chunk> mChunks;
};
