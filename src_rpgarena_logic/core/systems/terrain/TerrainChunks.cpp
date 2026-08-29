#include "TerrainDeformSystem.h"
#include "Config.h"
#include <iostream>
#include <SFML/OpenGL.hpp>

namespace {
unsigned long long g_debugChunkInstantiateTotal = 0;
}

void TerrainDeformSystem::_instantiateChunk(int cx, int cy) {
    if (cx < 0 || cx >= (int)mVisualChunkGridSize.x || cy < 0 || cy >= (int)mVisualChunkGridSize.y) return;
    ++g_debugChunkInstantiateTotal;

    int idx = cy * mVisualChunkGridSize.x + cx;
    auto& chunk = mVisualChunks[idx];
    
    if (chunk.active) return;

    if (!mChunkPool.empty()) {
        chunk.vram = std::move(mChunkPool.back());
        mChunkPool.pop_back();
    } else {
        chunk.vram.grassRT = std::make_unique<sf::RenderTexture>();
        chunk.vram.depthRT = std::make_unique<sf::RenderTexture>();
        chunk.vram.shallowDirtRT = std::make_unique<sf::RenderTexture>();

        if (!chunk.vram.grassRT->resize({VISUAL_CHUNK_SIZE, VISUAL_CHUNK_SIZE}) ||
            !chunk.vram.depthRT->resize({VISUAL_CHUNK_SIZE, VISUAL_CHUNK_SIZE}) ||
            !chunk.vram.shallowDirtRT->resize({VISUAL_CHUNK_SIZE, VISUAL_CHUNK_SIZE})) 
        {
            std::cerr << "[TerrainDeformSystem] Error creando texturas para Chunk " << cx << "," << cy << "\n";
            return;
        }
        chunk.vram.grassRT->setSmooth(false);
        chunk.vram.depthRT->setSmooth(false);
        chunk.vram.shallowDirtRT->setSmooth(false);
    }

    sf::View chunkView(sf::FloatRect({(float)cx * VISUAL_CHUNK_SIZE, (float)cy * VISUAL_CHUNK_SIZE},
                                     {(float)VISUAL_CHUNK_SIZE, (float)VISUAL_CHUNK_SIZE}));

    // Bake Grass (Capa B)
    chunk.vram.grassRT->clear(sf::Color::Transparent);
    chunk.vram.grassRT->setView(chunkView);
    mGrassMap.drawChunk(*chunk.vram.grassRT, cx, cy);
    chunk.vram.grassRT->display();
    
    chunk.grassSprite = std::make_unique<sf::Sprite>(chunk.vram.grassRT->getTexture());
    chunk.grassSprite->setTextureRect(sf::IntRect({-1, -1}, {VISUAL_CHUNK_SIZE + 2, VISUAL_CHUNK_SIZE + 2}));
    chunk.grassSprite->setPosition({(float)cx * VISUAL_CHUNK_SIZE - 1.f, (float)cy * VISUAL_CHUNK_SIZE - 1.f});

    // Bake Shallow Dirt (Capa A1)
    chunk.vram.shallowDirtRT->clear(sf::Color::Transparent);
    chunk.vram.shallowDirtRT->setView(chunkView);
    mShallowDirtMap.drawChunk(*chunk.vram.shallowDirtRT, cx, cy);
    chunk.vram.shallowDirtRT->display();
    chunk.shallowDirtSprite = std::make_unique<sf::Sprite>(chunk.vram.shallowDirtRT->getTexture());
    chunk.shallowDirtSprite->setTextureRect(sf::IntRect({-1, -1}, {VISUAL_CHUNK_SIZE + 2, VISUAL_CHUNK_SIZE + 2}));
    chunk.shallowDirtSprite->setPosition({(float)cx * VISUAL_CHUNK_SIZE - 1.f, (float)cy * VISUAL_CHUNK_SIZE - 1.f});

    // Clear Depth
    chunk.vram.depthRT->setView(chunkView);
    chunk.vram.depthRT->clear(sf::Color::Black);
    chunk.vram.depthRT->display();

    chunk.active = true;
    chunk.timeSinceLastDeform = 0.f;
    chunk.grassRegenAccumulator = 0.f;
    chunk.depthRegenAccumulator = 0.f;
    chunk.timeSinceDepthHealed = 0.f;

    mGrassMap.setChunkVisible(cx, cy, false);
    mShallowDirtMap.setChunkVisible(cx, cy, false);
}

void TerrainDeformSystem::update(sf::Time dt) {
    if (!mInitialized) return;

    mTotalElapsedTime += dt.asSeconds();

    _flushFootprints();

    float depthRegenPerSec = (cfg::Terrain::DIRT_REGEN_TIME_SEC > 0.f) ? (cfg::Terrain::EXPLOSION_OFFSET_PX / cfg::Terrain::DIRT_REGEN_TIME_SEC) : 0.f;
    float grassRegenPerSec = (cfg::Terrain::GRASS_REGEN_TIME_SEC > 0.f) ? (255.f / cfg::Terrain::GRASS_REGEN_TIME_SEC) : 0.f;

    for (size_t i = 0; i < mVisualChunks.size(); ++i) {
        auto& chunk = mVisualChunks[i];
        if (!chunk.active) continue;

        chunk.timeSinceLastDeform += dt.asSeconds();

        int cx = (int)(i % mVisualChunkGridSize.x);
        int cy = (int)(i / mVisualChunkGridSize.x);
        sf::View chunkView(sf::FloatRect({(float)cx * VISUAL_CHUNK_SIZE, (float)cy * VISUAL_CHUNK_SIZE},
                                         {(float)VISUAL_CHUNK_SIZE, (float)VISUAL_CHUNK_SIZE}));
        sf::View defaultView(sf::FloatRect({0.f, 0.f}, {(float)VISUAL_CHUNK_SIZE, (float)VISUAL_CHUNK_SIZE}));

        // 1. Regenerar Profundidad (GPU)
        if (depthRegenPerSec > 0.f) {
            chunk.depthRegenAccumulator += depthRegenPerSec * dt.asSeconds();
            if (chunk.depthRegenAccumulator >= 1.0f) {
                uint8_t amount = static_cast<uint8_t>(chunk.depthRegenAccumulator);
                chunk.depthRegenAccumulator -= static_cast<float>(amount);

                chunk.vram.depthRT->setView(defaultView);
                mRegenRect.setPosition({0.f, 0.f});
                mRegenRect.setFillColor(sf::Color(amount, 0, 0, 0));
                chunk.vram.depthRT->draw(mRegenRect, mRegenDepthStates);
                chunk.vram.depthRT->display();
                chunk.vram.depthRT->setView(chunkView);
            }
        }

        // 2. Regenerar Pasto y Tierra Superficial (GPU)
        if (grassRegenPerSec > 0.f) {
            chunk.grassRegenAccumulator += grassRegenPerSec * dt.asSeconds();
            if (chunk.grassRegenAccumulator >= 1.0f) {
                uint8_t amount = static_cast<uint8_t>(chunk.grassRegenAccumulator);
                chunk.grassRegenAccumulator -= static_cast<float>(amount);

                // --- 2.1 Regenerar Pasto ---
                chunk.vram.grassRT->setView(defaultView);
                chunk.vram.grassRT->setActive(true);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

                if (mRegenShaderLoaded) {
                    mRegenShader.setUniform("u_DepthTex", chunk.vram.depthRT->getTexture());
                    mRegenShader.setUniform("u_Amount", static_cast<float>(amount) / 255.0f);
                    mRegenShader.setUniform("u_TexelSize", sf::Glsl::Vec2(1.f / VISUAL_CHUNK_SIZE, 1.f / VISUAL_CHUNK_SIZE));
                    mRegenShader.setUniform("u_IsGrass", 1.0f);
                    
                    sf::RenderStates shaderStates = mRegenGrassStates;
                    shaderStates.shader = &mRegenShader;
                    
                    sf::Sprite regenSprite(chunk.vram.depthRT->getTexture());
                    regenSprite.setColor(sf::Color::White);
                    chunk.vram.grassRT->draw(regenSprite, shaderStates);
                } else {
                    mRegenRect.setPosition({0.f, 0.f});
                    mRegenRect.setFillColor(sf::Color(0, 0, 0, amount));
                    chunk.vram.grassRT->draw(mRegenRect, mRegenGrassStates);
                }

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                chunk.vram.grassRT->setActive(false);
                chunk.vram.grassRT->display();
                chunk.vram.grassRT->setView(chunkView);

                // --- 2.2 Regenerar Tierra Superficial ---
                chunk.vram.shallowDirtRT->setView(defaultView);
                chunk.vram.shallowDirtRT->setActive(true);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

                if (mRegenShaderLoaded) {
                    mRegenShader.setUniform("u_DepthTex", chunk.vram.depthRT->getTexture());
                    mRegenShader.setUniform("u_Amount", static_cast<float>(amount) / 255.0f);
                    mRegenShader.setUniform("u_TexelSize", sf::Glsl::Vec2(1.f / VISUAL_CHUNK_SIZE, 1.f / VISUAL_CHUNK_SIZE));
                    mRegenShader.setUniform("u_IsGrass", 0.0f);
                    
                    sf::RenderStates shaderStates = mRegenGrassStates;
                    shaderStates.shader = &mRegenShader;
                    
                    sf::Sprite regenSprite(chunk.vram.depthRT->getTexture());
                    regenSprite.setColor(sf::Color::White);
                    chunk.vram.shallowDirtRT->draw(regenSprite, shaderStates);
                } else {
                    mRegenRect.setPosition({0.f, 0.f});
                    mRegenRect.setFillColor(sf::Color(0, 0, 0, amount));
                    chunk.vram.shallowDirtRT->draw(mRegenRect, mRegenGrassStates);
                }

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                chunk.vram.shallowDirtRT->setActive(false);
                chunk.vram.shallowDirtRT->display();
                chunk.vram.shallowDirtRT->setView(chunkView);
            }
        }
    }

    _garbageCollectChunks(dt.asSeconds());
}

void TerrainDeformSystem::_garbageCollectChunks(float dt) {
    for (size_t i = 0; i < mVisualChunks.size(); ++i) {
        auto& vChunk = mVisualChunks[i];
        if (!vChunk.active) continue;
        
        int vcx = (int)(i % mVisualChunkGridSize.x);
        int vcy = (int)(i / mVisualChunkGridSize.x);
        
        sf::Vector2f chunkCenter(
            (float)vcx * VISUAL_CHUNK_SIZE + VISUAL_CHUNK_SIZE / 2.f,
            (float)vcy * VISUAL_CHUNK_SIZE + VISUAL_CHUNK_SIZE / 2.f
        );
        sf::Vector2f diff = chunkCenter - mCurrentCameraCenter;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        
        bool isFarAway = distSq > (3000.f * 3000.f);
        bool stillDirty = (mTotalElapsedTime < mChunkHealTime[i]);

        if (!isFarAway) {
            if (stillDirty) {
                vChunk.timeSinceDepthHealed = 0.f;
            } else {
                vChunk.timeSinceDepthHealed += dt;
            }

            if (stillDirty || vChunk.timeSinceDepthHealed < cfg::Terrain::GRASS_REGEN_TIME_SEC) {
                continue;
            }
        }

        if (!stillDirty || isFarAway) {
            if (mChunkPool.size() < 8) {
                mChunkPool.push_back(std::move(vChunk.vram));
            } else {
                vChunk.vram.grassRT.reset();
                vChunk.vram.shallowDirtRT.reset();
                vChunk.vram.depthRT.reset();
            }
            vChunk.grassSprite.reset();
            vChunk.shallowDirtSprite.reset();
            vChunk.active = false;

            mGrassMap.setChunkVisible(vcx, vcy, true);
            mShallowDirtMap.setChunkVisible(vcx, vcy, true);
            
            if (isFarAway) {
                mChunkHealTime[i] = 0.f;
                int startX = vcx * VISUAL_CHUNK_SIZE;
                int startY = vcy * VISUAL_CHUNK_SIZE;
                int endX = std::min(startX + VISUAL_CHUNK_SIZE, (int)mCpuMapSize.x);
                int endY = std::min(startY + VISUAL_CHUNK_SIZE, (int)mCpuMapSize.y);
                for (int y = startY; y < endY; ++y) {
                    for (int x = startX; x < endX; ++x) {
                        int idx = y * mCpuMapSize.x + x;
                        mMaxDepthMap[idx] = 0;
                        mDeformTimeMap[idx] = 0.f;
                    }
                }
            }
        }
    }
}

size_t TerrainDeformSystem::getDebugActiveVisualChunks() const {
    size_t count = 0;
    for (const auto& c : mVisualChunks) if (c.active) count++;
    return count;
}

size_t TerrainDeformSystem::getDebugChunkPoolSize() const {
    return mChunkPool.size();
}

unsigned long long TerrainDeformSystem::getDebugTotalChunkInstantiations() const {
    return g_debugChunkInstantiateTotal;
}

std::vector<sf::FloatRect> TerrainDeformSystem::getActiveChunkRects() const {
    std::vector<sf::FloatRect> rects;
    for (size_t i = 0; i < mVisualChunks.size(); ++i) {
        if (mVisualChunks[i].active) {
            int cx = i % mVisualChunkGridSize.x;
            int cy = i / mVisualChunkGridSize.x;
            rects.push_back(sf::FloatRect(
                sf::Vector2f((float)cx * VISUAL_CHUNK_SIZE, (float)cy * VISUAL_CHUNK_SIZE),
                sf::Vector2f((float)VISUAL_CHUNK_SIZE, (float)VISUAL_CHUNK_SIZE)
            ));
        }
    }
    return rects;
}
