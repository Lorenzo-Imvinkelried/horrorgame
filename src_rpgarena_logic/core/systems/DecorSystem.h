// DecorSystem.h
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <map> // Necesario
#include <utility>
#include "../engine/IRenderable.h"

#include "../engine/ResourceManager.h"
#include "Config.h" 
#include "WindSystem.h"

struct TreeShadowConfig {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

class DecorSystem {
public:
    struct DecorInstance : public IRenderable {
        sf::Sprite sprite;
        bool blocking;
        TreeShadowConfig shadowConfig;
        float getY() const { return sprite.getPosition().y; }

        DecorInstance(sf::Texture& t, bool b, TreeShadowConfig sc = {})
            : sprite(t), blocking(b), shadowConfig(sc) {}
            
        // [OPTIMIZATION]
        RenderType getRenderType() const override { return RenderType::Decor; }

        void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const override {
            texture = &sprite.getTexture(); 
            
            sf::Transform trans = sprite.getTransform();
            const auto& seedRect = sprite.getTextureRect();
            sf::FloatRect rect(sf::Vector2f(seedRect.position), sf::Vector2f(seedRect.size));
            sf::Color col = sprite.getColor();

            float left = 0.f; 
            float right = rect.size.x;
            float totalH = rect.size.y;

            sf::Vector2f pBottomLeft = trans.transformPoint({left, totalH});
            sf::Vector2f pBottomRight = trans.transformPoint({right, totalH});
            float groundY = std::max(pBottomLeft.y, pBottomRight.y);
            float baseX = (pBottomLeft.x + pBottomRight.x) * 0.5f;

            float u1 = rect.position.x; 
            float u2 = rect.position.x + rect.size.x;

            if (!cfg::Wind::ENABLE) {
                sf::Vector2f tL = trans.transformPoint({left, 0.f});
                sf::Vector2f tR = trans.transformPoint({right, 0.f});
                sf::Vector2f bL = trans.transformPoint({left, totalH});
                sf::Vector2f bR = trans.transformPoint({right, totalH});

                float vTop = rect.position.y;
                float vBottom = rect.position.y + totalH;

                vertices.emplace_back(sf::Vertex{tL, col, sf::Vector2f(u1, vTop)});
                vertices.emplace_back(sf::Vertex{tR, col, sf::Vector2f(u2, vTop)});
                vertices.emplace_back(sf::Vertex{bL, col, sf::Vector2f(u1, vBottom)});

                vertices.emplace_back(sf::Vertex{tR, col, sf::Vector2f(u2, vTop)});
                vertices.emplace_back(sf::Vertex{bR, col, sf::Vector2f(u2, vBottom)});
                vertices.emplace_back(sf::Vertex{bL, col, sf::Vector2f(u1, vBottom)});
                return;
            }

            float gamePixel = std::abs(sprite.getScale().x);
            if (gamePixel < 1.0f) gamePixel = 1.0f;

            int totalRows = static_cast<int>(totalH);
            for (int i = 0; i < totalRows; ++i) {
                float y0 = totalH - static_cast<float>(i);
                float y1 = totalH - static_cast<float>(i + 1);

                float v0 = rect.position.y + y0;
                float v1 = rect.position.y + y1;

                float h = (static_cast<float>(i) + 0.5f) / totalH;
                float rawOffset = 0.0f;
                if (!blocking) {
                    rawOffset = (h > 0.15f) 
                        ? WindSystem::get().getWindOffset(baseX, groundY, 0.30f + ((h - 0.15f) / 0.85f) * 0.70f) 
                        : 0.0f;
                } else {
                    rawOffset = WindSystem::get().getWindOffset(baseX, groundY, h);
                }
                float snappedOffset = std::round(rawOffset / gamePixel) * gamePixel;

                sf::Vector2f bL = trans.transformPoint({left, y0});
                sf::Vector2f bR = trans.transformPoint({right, y0});
                sf::Vector2f tL = trans.transformPoint({left, y1});
                sf::Vector2f tR = trans.transformPoint({right, y1});

                bL.x += snappedOffset;
                bR.x += snappedOffset;
                tL.x += snappedOffset;
                tR.x += snappedOffset;

                // Tri 1: tL, tR, bL
                vertices.emplace_back(sf::Vertex{tL, col, sf::Vector2f(u1, v1)});
                vertices.emplace_back(sf::Vertex{tR, col, sf::Vector2f(u2, v1)});
                vertices.emplace_back(sf::Vertex{bL, col, sf::Vector2f(u1, v0)});

                // Tri 2: tR, bR, bL
                vertices.emplace_back(sf::Vertex{tR, col, sf::Vector2f(u2, v1)});
                vertices.emplace_back(sf::Vertex{bR, col, sf::Vector2f(u2, v0)});
                vertices.emplace_back(sf::Vertex{bL, col, sf::Vector2f(u1, v0)});
            }
        }

        bool castsShadow() const override { return true; }

        void getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const override {
            texture = &sprite.getTexture(); 
            
            sf::Transform trans = sprite.getTransform();
            const auto& seedRect = sprite.getTextureRect();
            sf::FloatRect rect(sf::Vector2f(seedRect.position), sf::Vector2f(seedRect.size));
            
            sf::Color col = sprite.getColor();
            sf::Color shadowCol(46, 34, 47, col.a);

            float left = 0.f; 
            float right = rect.size.x;
            float totalH = rect.size.y;

            sf::Vector2f pBottomLeft = trans.transformPoint({left, totalH});
            sf::Vector2f pBottomRight = trans.transformPoint({right, totalH});
            float groundY = std::max(pBottomLeft.y, pBottomRight.y);
            float baseX = (pBottomLeft.x + pBottomRight.x) * 0.5f;

            float shadowScaleY = cfg::Shadow::SCALE_Y * shadowConfig.scaleY;
            float shadowScaleX = cfg::Shadow::SCALE_X * shadowConfig.scaleX;
            float shadowSkewX = cfg::Shadow::SKEW_X;
            float shOffsetX = cfg::Shadow::OFFSET_X + shadowConfig.offsetX;
            float shOffsetY = cfg::Shadow::OFFSET_Y + shadowConfig.offsetY;

            auto projectShadow = [&](sf::Vector2f p) -> sf::Vector2f {
                float relX = p.x - baseX;
                float height = groundY - p.y;
                return {
                    baseX + relX * shadowScaleX + shOffsetX + height * shadowSkewX,
                    groundY + (p.y - groundY) * shadowScaleY + shOffsetY
                };
            };

            float u1 = rect.position.x; 
            float u2 = rect.position.x + rect.size.x;

            if (!cfg::Wind::ENABLE) {
                sf::Vector2f tL = trans.transformPoint({left, 0.f});
                sf::Vector2f tR = trans.transformPoint({right, 0.f});
                sf::Vector2f bL = trans.transformPoint({left, totalH});
                sf::Vector2f bR = trans.transformPoint({right, totalH});

                sf::Vector2f s_tL = projectShadow(tL);
                sf::Vector2f s_tR = projectShadow(tR);
                sf::Vector2f s_bL = projectShadow(bL);
                sf::Vector2f s_bR = projectShadow(bR);

                float vTop = rect.position.y;
                float vBottom = rect.position.y + totalH;

                vertices.emplace_back(sf::Vertex{s_tL, shadowCol, sf::Vector2f(u1, vTop)});
                vertices.emplace_back(sf::Vertex{s_tR, shadowCol, sf::Vector2f(u2, vTop)});
                vertices.emplace_back(sf::Vertex{s_bL, shadowCol, sf::Vector2f(u1, vBottom)});

                vertices.emplace_back(sf::Vertex{s_tR, shadowCol, sf::Vector2f(u2, vTop)});
                vertices.emplace_back(sf::Vertex{s_bR, shadowCol, sf::Vector2f(u2, vBottom)});
                vertices.emplace_back(sf::Vertex{s_bL, shadowCol, sf::Vector2f(u1, vBottom)});
                return;
            }

            float gamePixel = std::abs(sprite.getScale().x);
            if (gamePixel < 1.0f) gamePixel = 1.0f;

            int totalRows = static_cast<int>(totalH);
            for (int i = 0; i < totalRows; ++i) {
                float y0 = totalH - static_cast<float>(i);
                float y1 = totalH - static_cast<float>(i + 1);

                float v0 = rect.position.y + y0;
                float v1 = rect.position.y + y1;

                float h = (static_cast<float>(i) + 0.5f) / totalH;
                float rawOffset = 0.0f;
                if (!blocking) {
                    rawOffset = (h > 0.15f) 
                        ? WindSystem::get().getWindOffset(baseX, groundY, 0.30f + ((h - 0.15f) / 0.85f) * 0.70f) 
                        : 0.0f;
                } else {
                    rawOffset = WindSystem::get().getWindOffset(baseX, groundY, h);
                }
                float snappedOffset = std::round(rawOffset / gamePixel) * gamePixel;

                sf::Vector2f bL = trans.transformPoint({left, y0});
                sf::Vector2f bR = trans.transformPoint({right, y0});
                sf::Vector2f tL = trans.transformPoint({left, y1});
                sf::Vector2f tR = trans.transformPoint({right, y1});

                bL.x += snappedOffset;
                bR.x += snappedOffset;
                tL.x += snappedOffset;
                tR.x += snappedOffset;

                sf::Vector2f s_bL = projectShadow(bL);
                sf::Vector2f s_bR = projectShadow(bR);
                sf::Vector2f s_tL = projectShadow(tL);
                sf::Vector2f s_tR = projectShadow(tR);

                // Tri 1: s_tL, s_tR, s_bL
                vertices.emplace_back(sf::Vertex{s_tL, shadowCol, sf::Vector2f(u1, v1)});
                vertices.emplace_back(sf::Vertex{s_tR, shadowCol, sf::Vector2f(u2, v1)});
                vertices.emplace_back(sf::Vertex{s_bL, shadowCol, sf::Vector2f(u1, v0)});

                // Tri 2: s_tR, s_bR, s_bL
                vertices.emplace_back(sf::Vertex{s_tR, shadowCol, sf::Vector2f(u2, v1)});
                vertices.emplace_back(sf::Vertex{s_bR, shadowCol, sf::Vector2f(u2, v0)});
                vertices.emplace_back(sf::Vertex{s_bL, shadowCol, sf::Vector2f(u1, v0)});
            }
        }
    }; 

public:
    DecorSystem(ResourceManager& res);

    void loadShadowConfig(const std::string& path = "assets/data/shadow_vegetacion.json");
    TreeShadowConfig getShadowConfigForId(int id) const;
    std::string keyForId(int id) const;

    bool loadFromCsv(const std::string& csvPath,
                     int mapWidth,
                     int mapHeight,
                     float tileSize);
    
    void clear(); // [LEVEL TRANSITION]

    // --- NUEVO: Dibujar la capa estática (Pasto/Flores) ---
    void drawStaticLayer(sf::RenderTarget& target) const;

    const std::vector<sf::FloatRect>& getBlockingRects() const {
        // (Opcional, si usas el grid espacial esto ya no se usa tanto)
        return mBlockingRects; 
    }

    // Devuelve SOLO los objetos dinámicos (Árboles) para el Y-Sort
    const std::vector<DecorInstance>& getInstances() const {
        return mInstances;
    }

    // --- OPTIMIZATION 2: Culling inside DecorSystem ---
    void getVisibleInstances(const sf::FloatRect& viewRect, std::vector<const DecorInstance*>& outVisible) const;

    // --- OPTIMIZATION 3: Memory reuse for collisions ---
    void getObstaclesNearby(sf::Vector2f position, std::vector<sf::FloatRect>& outObstacles) const;

    // 2.5D Depth & Pixel-perfect collision check against decor (trees)
    bool checkPixelCollision(sf::Vector2f screenPos, float groundY, sf::Vector2f velocity = {0.f, 0.f}, float distanceTraveled = 0.f) const;

private:
    ResourceManager& mRes;
    
    // Aquí guardamos solo los árboles/objetos con colisión
    std::vector<DecorInstance> mInstances;
    
    // (Legacy)
    std::vector<sf::FloatRect> mBlockingRects;

    // Grid Espacial (Mantenemos esto para colisiones rápidas)
    std::map<std::pair<int, int>, std::vector<sf::FloatRect>> mSpatialGrid; 

    // [OPTIMIZATION] Grid Espacial para Renderizado (Punteros a instancias)
    std::map<std::pair<int, int>, std::vector<const DecorInstance*>> mRenderGrid; 

    // --- NUEVO: BATCHING POR TEXTURA ---
    // La llave es el puntero a la textura, el valor es el array de vértices
    std::map<const sf::Texture*, sf::VertexArray> mStaticBatches;
    // ----------------------------------

    TreeShadowConfig mDefaultShadowConfig;
    std::map<std::string, TreeShadowConfig> mTreeShadowConfigs;

    bool loadCsvGrid(const std::string& csvPath,
                     std::vector<int>& outData,
                     int expectedWidth,
                     int expectedHeight);

    std::string texturePathForId(int id) const;
    bool isBlockingId(int id) const;
};