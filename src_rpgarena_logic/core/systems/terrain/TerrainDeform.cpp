#include "TerrainDeformSystem.h"
#include "Config.h"
#include <algorithm>
#include <SFML/OpenGL.hpp>

void TerrainDeformSystem::queueFootprint(const FootprintStamp& stamp) {
    if (!mInitialized) return;
    mPendingFootprints.push_back(stamp);
}

void TerrainDeformSystem::queueExplosion(sf::Vector2f pos) {
    if (!mInitialized) return;
    mPendingExplosions.push_back(pos);
}

float TerrainDeformSystem::getDepthAt(sf::Vector2f worldPos) const {
    if (mMaxDepthMap.empty() || mCpuMapSize.x == 0) return 0.f;
    int px = static_cast<int>(worldPos.x);
    int py = static_cast<int>(worldPos.y);
    if (px >= 0 && px < static_cast<int>(mCpuMapSize.x) && py >= 0 && py < static_cast<int>(mCpuMapSize.y)) {
        int idx = py * mCpuMapSize.x + px;
        float elapsedTime = mTotalElapsedTime - mDeformTimeMap[idx];
        float regenAmount = (elapsedTime / cfg::Terrain::DIRT_REGEN_TIME_SEC) * cfg::Terrain::EXPLOSION_OFFSET_PX;
        return std::max(0.f, static_cast<float>(mMaxDepthMap[idx]) - regenAmount);
    }
    return 0.f;
}

sf::Color TerrainDeformSystem::getDeformedColorAt(sf::Vector2f worldPos, sf::Color baseColor) const {
    float depth = getDepthAt(worldPos);
    if (depth > 0.f) {
        sf::Color deepColor = mDeepDirtMap.getColorAtWorldPos(worldPos);
        if (deepColor != sf::Color::Transparent) {
            return deepColor;
        }
        
        sf::Color shallowColor = mShallowDirtMap.getColorAtWorldPos(worldPos);
        if (shallowColor != sf::Color::Transparent) {
            return shallowColor;
        }
    }
    return baseColor;
}

TerrainDeformSystem::ChunkInfo TerrainDeformSystem::getGrassChunkInfo(sf::Vector2f worldPos) const {
    ChunkInfo info;
    info.size = (float)VISUAL_CHUNK_SIZE;

    int cx = (int)(worldPos.x / VISUAL_CHUNK_SIZE);
    int cy = (int)(worldPos.y / VISUAL_CHUNK_SIZE);

    if (cx >= 0 && cx < (int)mVisualChunkGridSize.x && cy >= 0 && cy < (int)mVisualChunkGridSize.y) {
        int idx = cy * mVisualChunkGridSize.x + cx;
        const auto& chunk = mVisualChunks[idx];
        if (chunk.active && chunk.vram.grassRT && chunk.grassSprite) {
            info.texture = &chunk.vram.grassRT->getTexture();
            info.depthTexture = &chunk.vram.depthRT->getTexture();
            info.offset = {(float)cx * VISUAL_CHUNK_SIZE, (float)cy * VISUAL_CHUNK_SIZE};
            return info;
        }
    }

    info.texture = &mGrassTexture.getTexture();
    info.depthTexture = &mDepthRT.getTexture();
    return info;
}

void TerrainDeformSystem::_carveDepthMask(const sf::Sprite& sprite, const sf::Image& maskImg, uint8_t depthVal) {
    if (mMaxDepthMap.empty() || mCpuMapSize.x == 0) return;
    if (maskImg.getSize().x == 0) return;

    sf::Transform t = sprite.getTransform();
    sf::Transform inv = t.getInverse();

    sf::Vector2u size = maskImg.getSize();
    sf::FloatRect bounds = t.transformRect(sf::FloatRect({0.f, 0.f}, {(float)size.x, (float)size.y}));

    int minX = std::max(0, static_cast<int>(bounds.position.x));
    int minY = std::max(0, static_cast<int>(bounds.position.y));
    int maxX = std::min(static_cast<int>(mCpuMapSize.x) - 1, static_cast<int>(bounds.position.x + bounds.size.x));
    int maxY = std::min(static_cast<int>(mCpuMapSize.y) - 1, static_cast<int>(bounds.position.y + bounds.size.y));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            sf::Vector2f srcPos = inv.transformPoint({(float)x, (float)y});
            int sx = static_cast<int>(srcPos.x);
            int sy = static_cast<int>(srcPos.y);

            if (sx >= 0 && sx < static_cast<int>(size.x) && sy >= 0 && sy < static_cast<int>(size.y)) {
                uint8_t alpha = maskImg.getPixel({(unsigned)sx, (unsigned)sy}).a;
                if (alpha > 128) {
                    int idx = y * mCpuMapSize.x + x;
                    
                    float elapsedTime = mTotalElapsedTime - mDeformTimeMap[idx];
                    float regenAmount = (elapsedTime / cfg::Terrain::DIRT_REGEN_TIME_SEC) * cfg::Terrain::EXPLOSION_OFFSET_PX;
                    float currentDepth = std::max(0.f, static_cast<float>(mMaxDepthMap[idx]) - regenAmount);

                    if (currentDepth < depthVal) {
                        mMaxDepthMap[idx] = depthVal;
                        mDeformTimeMap[idx] = mTotalElapsedTime;
                        
                        int cx = x / VISUAL_CHUNK_SIZE;
                        int cy = y / VISUAL_CHUNK_SIZE;
                        if (cx >= 0 && cx < (int)mVisualChunkGridSize.x && cy >= 0 && cy < (int)mVisualChunkGridSize.y) {
                            int chunkIdx = cy * mVisualChunkGridSize.x + cx;
                            float healDuration = (static_cast<float>(depthVal) / cfg::Terrain::EXPLOSION_OFFSET_PX) * cfg::Terrain::DIRT_REGEN_TIME_SEC;
                            mChunkHealTime[chunkIdx] = std::max(mChunkHealTime[chunkIdx], mTotalElapsedTime + healDuration);
                        }
                    }
                }
            }
        }
    }
}

void TerrainDeformSystem::_flushFootprints() {
    if (mPendingFootprints.empty() && mPendingExplosions.empty()) return;

    struct PendingChunkDraw {
        std::vector<const FootprintStamp*> footprints;
        std::vector<sf::Vector2f> explosions;
    };

    std::vector<PendingChunkDraw> chunkDraws(mVisualChunkGridSize.x * mVisualChunkGridSize.y);

    // 1. Agrupar pisadas por chunk
    for (const FootprintStamp& stamp : mPendingFootprints) {
        if (!mEraserSprite && !mFootTex) continue;

        mEraserSprite->setPosition(stamp.pos);
        mEraserSprite->setRotation(sf::degrees(stamp.rotDeg));
        mEraserSprite->setScale(stamp.scale);
        const sf::Texture* activeTex = stamp.customTex ? stamp.customTex : mFootTex;
        if (activeTex) {
            mEraserSprite->setTexture(*activeTex);
            mEraserSprite->setOrigin(stamp.origin);
        }

        sf::FloatRect bounds = mEraserSprite->getGlobalBounds();
        int minCx = std::max(0, (int)(bounds.position.x / VISUAL_CHUNK_SIZE));
        int minCy = std::max(0, (int)(bounds.position.y / VISUAL_CHUNK_SIZE));
        int maxCx = std::min((int)mVisualChunkGridSize.x - 1, (int)((bounds.position.x + bounds.size.x) / VISUAL_CHUNK_SIZE));
        int maxCy = std::min((int)mVisualChunkGridSize.y - 1, (int)((bounds.position.y + bounds.size.y) / VISUAL_CHUNK_SIZE));

        for (int cy = minCy; cy <= maxCy; ++cy) {
            for (int cx = minCx; cx <= maxCx; ++cx) {
                chunkDraws[cy * mVisualChunkGridSize.x + cx].footprints.push_back(&stamp);
            }
        }
    }

    // 2. Agrupar explosiones por chunk
    for (const sf::Vector2f& pos : mPendingExplosions) {
        if (!mExplosionSprite) continue;

        mExplosionSprite->setPosition(pos);
        sf::FloatRect bounds = mExplosionSprite->getGlobalBounds();
        int minCx = std::max(0, (int)(bounds.position.x / VISUAL_CHUNK_SIZE));
        int minCy = std::max(0, (int)(bounds.position.y / VISUAL_CHUNK_SIZE));
        int maxCx = std::min((int)mVisualChunkGridSize.x - 1, (int)((bounds.position.x + bounds.size.x) / VISUAL_CHUNK_SIZE));
        int maxCy = std::min((int)mVisualChunkGridSize.y - 1, (int)((bounds.position.y + bounds.size.y) / VISUAL_CHUNK_SIZE));

        for (int cy = minCy; cy <= maxCy; ++cy) {
            for (int cx = minCx; cx <= maxCx; ++cx) {
                chunkDraws[cy * mVisualChunkGridSize.x + cx].explosions.push_back(pos);
            }
        }
    }

    // 3. Renderizar cada chunk agrupado
    for (size_t i = 0; i < chunkDraws.size(); ++i) {
        const auto& drawReq = chunkDraws[i];
        if (drawReq.footprints.empty() && drawReq.explosions.empty()) continue;

        int cx = (int)(i % mVisualChunkGridSize.x);
        int cy = (int)(i / mVisualChunkGridSize.x);
        _instantiateChunk(cx, cy);
        auto& chunk = mVisualChunks[i];
        chunk.timeSinceLastDeform = 0.f;

        // Pasto (Capa B)
        chunk.vram.grassRT->setActive(true);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

        for (const auto* stampPtr : drawReq.footprints) {
            mEraserSprite->setPosition(stampPtr->pos);
            mEraserSprite->setRotation(sf::degrees(stampPtr->rotDeg));
            mEraserSprite->setScale(stampPtr->scale);
            const sf::Texture* activeTex = stampPtr->customTex ? stampPtr->customTex : mFootTex;
            if (activeTex) {
                mEraserSprite->setTexture(*activeTex);
                mEraserSprite->setOrigin(stampPtr->origin);
            }
            chunk.vram.grassRT->draw(*mEraserSprite, mEraseStates);
        }

        for (const auto& pos : drawReq.explosions) {
            mExplosionSprite->setPosition(pos);
            chunk.vram.grassRT->draw(*mExplosionSprite, mEraseStates);
        }

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        chunk.vram.grassRT->setActive(false);
        chunk.vram.grassRT->display();

        // Tierra Superficial (Capa A1)
        // Se perfora hacia la tierra profunda (marrón) en caso de explosiones o pisadas pesadas con profundidad (depthByte >= 1)
        bool drawShallowDirt = false;
        for (const auto* stampPtr : drawReq.footprints) {
            uint8_t depthByte = static_cast<uint8_t>(std::min(stampPtr->depthPx, 255.f));
            if (depthByte >= 1) { drawShallowDirt = true; break; }
        }
        if (!drawReq.explosions.empty()) drawShallowDirt = true;

        if (drawShallowDirt) {
            for (const auto* stampPtr : drawReq.footprints) {
                uint8_t depthByte = static_cast<uint8_t>(std::min(stampPtr->depthPx, 255.f));
                if (depthByte >= 1) {
                    mEraserSprite->setPosition(stampPtr->pos);
                    mEraserSprite->setRotation(sf::degrees(stampPtr->rotDeg));
                    mEraserSprite->setScale(stampPtr->scale);
                    const sf::Texture* activeTex = stampPtr->customTex ? stampPtr->customTex : mFootTex;
                    if (activeTex) {
                        mEraserSprite->setTexture(*activeTex);
                        mEraserSprite->setOrigin(stampPtr->origin);
                    }
                    chunk.vram.shallowDirtRT->draw(*mEraserSprite, mEraseStates);
                }
            }
            for (const auto& pos : drawReq.explosions) {
                mExplosionSprite->setPosition(pos);
                chunk.vram.shallowDirtRT->draw(*mExplosionSprite, mEraseStates);
            }
            chunk.vram.shallowDirtRT->display();
        }

        // Mapa de Profundidad (depthRT)
        bool drawDepth = false;
        for (const auto* stampPtr : drawReq.footprints) {
            uint8_t depthByte = static_cast<uint8_t>(std::min(stampPtr->depthPx, 255.f));
            if (depthByte > 0) { drawDepth = true; break; }
        }
        if (!drawReq.explosions.empty()) drawDepth = true;

        if (drawDepth) {
            for (const auto* stampPtr : drawReq.footprints) {
                uint8_t depthByte = static_cast<uint8_t>(std::min(stampPtr->depthPx, 255.f));
                if (depthByte > 0) {
                    mEraserSprite->setPosition(stampPtr->pos);
                    mEraserSprite->setRotation(sf::degrees(stampPtr->rotDeg));
                    mEraserSprite->setScale(stampPtr->scale);
                    const sf::Texture* activeTex = stampPtr->customTex ? stampPtr->customTex : mFootTex;
                    if (activeTex) {
                        mEraserSprite->setTexture(*activeTex);
                        mEraserSprite->setOrigin(stampPtr->origin);
                    }
                    mEraserSprite->setColor(sf::Color(depthByte, 0, 0, 255));
                    if (mDepthStampShaderLoaded) {
                        sf::RenderStates stampStates = mDepthAddStates;
                        stampStates.shader = &mDepthStampShader;
                        chunk.vram.depthRT->draw(*mEraserSprite, stampStates);
                    } else {
                        chunk.vram.depthRT->draw(*mEraserSprite, mDepthAddStates);
                    }
                    mEraserSprite->setColor(sf::Color::White);
                }
            }
            for (const auto& pos : drawReq.explosions) {
                mExplosionSprite->setPosition(pos);
                uint8_t depthVal = static_cast<uint8_t>(cfg::Terrain::EXPLOSION_OFFSET_PX);
                mExplosionSprite->setColor(sf::Color(depthVal, 0, 0, 255));
                if (mDepthStampShaderLoaded) {
                    sf::RenderStates stampStates = mDepthAddStates;
                    stampStates.shader = &mDepthStampShader;
                    chunk.vram.depthRT->draw(*mExplosionSprite, stampStates);
                } else {
                    chunk.vram.depthRT->draw(*mExplosionSprite, mDepthAddStates);
                }
                mExplosionSprite->setColor(sf::Color::White);
            }
            chunk.vram.depthRT->display();
        }
    }

    // 4. Actualizar Mapa CPU
    for (const FootprintStamp& stamp : mPendingFootprints) {
        uint8_t depthByte = static_cast<uint8_t>(std::min(stamp.depthPx, 255.f));
        const sf::Texture* activeTex = stamp.customTex ? stamp.customTex : mFootTex;
        const sf::Image*   activeImg = stamp.customImg ? stamp.customImg : &mFootImage;
        if (depthByte > 0 && activeImg) {
            mEraserSprite->setPosition(stamp.pos);
            mEraserSprite->setRotation(sf::degrees(stamp.rotDeg));
            mEraserSprite->setScale(stamp.scale);
            if (activeTex) {
                mEraserSprite->setTexture(*activeTex);
                mEraserSprite->setOrigin(stamp.origin);
            }
            _carveDepthMask(*mEraserSprite, *activeImg, depthByte);
        }
    }

    for (const sf::Vector2f& pos : mPendingExplosions) {
        if (mExplosionSprite) {
            mExplosionSprite->setPosition(pos);
            uint8_t depthVal = static_cast<uint8_t>(cfg::Terrain::EXPLOSION_OFFSET_PX);
            _carveDepthMask(*mExplosionSprite, mExplosionImage, depthVal);
        }
    }

    mPendingFootprints.clear();
    mPendingExplosions.clear();
}
