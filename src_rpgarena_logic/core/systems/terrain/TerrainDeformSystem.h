#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include "map/ChunkedTileMap.h"

class TerrainDeformSystem {
public:
    struct FootprintStamp {
        sf::Vector2f pos;
        float        rotDeg  = 0.f;
        sf::Vector2f scale   = {1.f, 1.f};
        sf::Vector2f origin;
        float        depthPx = 0.f;

        const sf::Texture* customTex = nullptr;
        const sf::Image*   customImg = nullptr;
    };

    TerrainDeformSystem();

    // -----------------------------------------------------------------------
    // INICIALIZACIÓN
    // -----------------------------------------------------------------------
    bool init(sf::Vector2u mapSizePx, sf::Color initialColor = sf::Color::Transparent);

    void initDirtLayer(const std::string& csvPath,
                       const std::string& dirtTilesetPath,
                       const std::string& debugTilesetPath,
                       unsigned tileSize,
                       unsigned chunkSize,
                       sf::Vector2u mapSizePx);

    void bakeGrassLayer(const ChunkedTileMap& grassMap, sf::Vector2u mapSizePx);
    void initEdgeShader();
    void clear(sf::Vector2u newMapSizePx);

    // -----------------------------------------------------------------------
    // CONFIGURACIÓN
    // -----------------------------------------------------------------------
    void setFootTexture(const sf::Texture* tex, sf::Vector2f localOrigin);
    void setExplosionTexture(const sf::Texture* tex, sf::Vector2f localOrigin);

    // -----------------------------------------------------------------------
    // UPDATE
    // -----------------------------------------------------------------------
    void queueFootprint(const FootprintStamp& stamp);
    void queueExplosion(sf::Vector2f pos);
    void update(sf::Time dt);

    // -----------------------------------------------------------------------
    // RENDER
    // -----------------------------------------------------------------------
    void drawDirt(sf::RenderTarget& target, const sf::View& view);
    void applyFootprintsAndDraw(sf::RenderTarget& target, const sf::View& worldView);

    // -----------------------------------------------------------------------
    // QUERIES
    // -----------------------------------------------------------------------
    bool isInitialized()  const { return mInitialized; }
    bool hasDirtLayer()   const { return mDirtLayerReady; }
    bool hasGrassBaked()  const { return mGrassBaked; }
    bool hasEdgeShader()  const { return mEdgeShaderLoaded; }
    void flushFootprints() { _flushFootprints(); }

    const sf::Texture& getGrassTexture() const { return mGrassTexture.getTexture(); }
    const sf::Texture& getDepthTexture() const { return mDepthRT.getTexture(); }

    struct ChunkInfo {
        const sf::Texture* texture = nullptr;
        const sf::Texture* depthTexture = nullptr;
        sf::Vector2f offset = {0.f, 0.f};
        float size = 0.f;
    };
    ChunkInfo getGrassChunkInfo(sf::Vector2f worldPos) const;

    sf::Vector2u getMapSizePx() const { return mCpuMapSize; }

    size_t getDebugActiveVisualChunks() const;
    size_t getDebugChunkPoolSize() const;
    unsigned long long getDebugTotalChunkInstantiations() const;

    float getDepthAt(sf::Vector2f worldPos) const;
    sf::Color getDeformedColorAt(sf::Vector2f worldPos, sf::Color baseColor) const;
    std::vector<sf::FloatRect> getActiveChunkRects() const;

private:
    void _flushFootprints();
    void _instantiateChunk(int cx, int cy);
    void _garbageCollectChunks(float dt);
    void _carveDepthMask(const sf::Sprite& sprite, const sf::Image& maskImg, uint8_t depthVal);

private:
    // Legacy RTs (1x1 fallbacks)
    sf::RenderTexture mGrassTexture;
    sf::RenderTexture mDepthRT;

    ChunkedTileMap mDeepDirtMap;
    ChunkedTileMap mShallowDirtMap;
    ChunkedTileMap mGrassMap;

    bool mDirtLayerReady = false;
    bool mShallowDirtBaked = false;
    bool mGrassBaked = false;

    // Chunks visuales
    static constexpr int VISUAL_CHUNK_SIZE = 512;
    struct VRAMTuple {
        std::unique_ptr<sf::RenderTexture> grassRT;
        std::unique_ptr<sf::RenderTexture> shallowDirtRT;
        std::unique_ptr<sf::RenderTexture> depthRT;
    };

    struct VisualChunk {
        VRAMTuple vram;
        std::unique_ptr<sf::Sprite> grassSprite;
        std::unique_ptr<sf::Sprite> shallowDirtSprite;
        
        bool active = false;
        float timeSinceLastDeform = 0.f;
        float grassRegenAccumulator = 0.f;
        float depthRegenAccumulator = 0.f;
        float timeSinceDepthHealed = 0.f;
    };
    
    sf::Vector2u mVisualChunkGridSize;
    std::vector<VisualChunk> mVisualChunks;
    std::vector<VRAMTuple> mChunkPool;

    sf::Vector2f mCurrentCameraCenter = {0.f, 0.f};

    // Shaders
    sf::Shader  mEdgeShader;
    bool        mEdgeShaderLoaded = false;

    sf::Shader  mRegenShader;
    bool        mRegenShaderLoaded = false;

    sf::Shader  mDepthStampShader;
    bool        mDepthStampShaderLoaded = false;

    // Textures & Sprites
    const sf::Texture*          mFootTex    = nullptr;
    sf::Vector2f                mFootOrigin;
    std::unique_ptr<sf::Sprite> mEraserSprite;

    const sf::Texture*          mExplosionTex    = nullptr;
    sf::Vector2f                mExplosionOrigin;
    std::unique_ptr<sf::Sprite> mExplosionSprite;

    sf::CircleShape             mEraserCircle;
    sf::RenderStates            mEraseStates;

    std::vector<FootprintStamp> mPendingFootprints;
    std::vector<sf::Vector2f>   mPendingExplosions;
    bool                        mInitialized = false;

    // CPU Depth Map
    sf::Vector2u         mCpuMapSize;
    std::vector<uint8_t> mMaxDepthMap;
    std::vector<float>   mDeformTimeMap;
    std::vector<float>   mChunkHealTime;
    float                mTotalElapsedTime = 0.f;
    sf::Image            mFootImage;
    sf::Image            mExplosionImage;
    
    // BlendModes
    sf::RenderStates     mDepthAddStates;
    sf::RectangleShape   mRegenRect;
    sf::RenderStates     mRegenGrassStates;
    sf::RenderStates     mRegenDepthStates;
};
