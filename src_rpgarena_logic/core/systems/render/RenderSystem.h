#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include "core/engine/SpriteBatcher.h"
#include "core/engine/IRenderable.h"
#include "core/systems/DecorSystem.h"
#include "core/systems/terrain/TerrainDeformSystem.h"

class WorldManager;
class EntityManager;
class ParticleSystem;
class GoreSystem;
class Entity;
class ItemDropSystem;

class RenderSystem {
public:
    RenderSystem();

    void render(sf::RenderTarget& target, 
                sf::View& view, 
                WorldManager& worldManager, 
                EntityManager& entityManager, 
                ParticleSystem& particleSystem, 
                GoreSystem& goreSystem,
                TerrainDeformSystem& terrainDeform,
                ItemDropSystem& itemDrops,
                Entity* targetedEntity = nullptr);

    size_t getRenderedCount() const { return mLastRenderCount; }

private:
    void renderShadowPass(sf::RenderTarget& target, const sf::View& view, 
                          TerrainDeformSystem& terrainDeform, GoreSystem& goreSystem,
                          sf::Vector2f& shadowViewTopLeft, sf::Vector2f& shadowViewSize, bool& shadowViewValid);
                          
    void renderQueue(sf::RenderTarget& target, const sf::View& view,
                     TerrainDeformSystem& terrainDeform, ParticleSystem& particleSystem,
                     Entity* targetedEntity, const sf::Vector2f& shadowViewTopLeft,
                     const sf::Vector2f& shadowViewSize, bool shadowViewValid);

private:
    size_t mLastRenderCount = 0;
    
    sf::Shader mOcclusionShader;
    bool mOcclusionShaderLoaded = false;

    sf::Shader mGroundShadowShader;
    bool mGroundShadowShaderLoaded = false;

    sf::Shader mShadowEncodeShader;
    bool mShadowEncodeShaderLoaded = false;

    struct YSortable {
        float y;
        const IRenderable* renderable = nullptr;
        const Entity* entity = nullptr; 
        int partLayer = 0; // 0 = full, 1 = back, 2 = middle, 3 = front

        YSortable(float _y, const IRenderable* r) : y(_y), renderable(r), partLayer(0) {}
        YSortable(float _y, const Entity* e, int layer = 0) : y(_y), entity(e), partLayer(layer) {}
        
        bool operator<(const YSortable& other) const {
            if (y != other.y) {
                return y < other.y;
            }
            if (partLayer != other.partLayer) {
                return partLayer < other.partLayer;
            }
            if (entity != other.entity) {
                return entity < other.entity;
            }
            return renderable < other.renderable;
        }
    };

    SpriteBatcher mBatcher;
    
    std::vector<YSortable> mRenderQueueDynamic;
    std::vector<YSortable> mRenderQueueCombined;
    
    std::vector<const DecorSystem::DecorInstance*> mVisibleDecorCache; 

    std::vector<sf::Vertex> mTempVerts;
    sf::RenderTexture mShadowRT;
    sf::RenderTexture mStaticShadowRT;
    sf::Shader mDynamicShadowEncodeShader;
    bool mDynamicShadowShaderLoaded = false;

    sf::RenderTexture mHeightMapRT;
    sf::Shader mHeightEncodeShader;
    bool mHeightEncodeShaderLoaded = false;

    sf::RenderTexture mSelectionRT;
    sf::Shader mSelectionOutlineShader;
    bool mSelectionOutlineShaderLoaded = false;

    sf::RenderTexture mEntityRT;
    sf::Texture mContourTexture;
    
    const Entity* mCurrentTargetedEntity = nullptr;
    float mCurrentTargetedEntityHeight = 0.f;
};
