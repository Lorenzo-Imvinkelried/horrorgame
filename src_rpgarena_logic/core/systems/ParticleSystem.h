#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include "../engine/ResourceManager.h"
#include "../engine/IRenderable.h" // [FIX] Required for inheritance
#include <cmath> // [FIX] Required for std::floor

// Forward Declaration
class Entity;

// [DATA DRIVEN] Particle Profile
struct ParticleProfile {
    // Spawning
    float spawnRadiusMin = 0.f; 
    float spawnRadiusMax = 0.f;
    int countMin = 1; 
    int countMax = 1;
    
    // Physics
    float speedMin = 50.f;
    float speedMax = 100.f;
    float lifeMin = 0.5f;
    float lifeMax = 1.0f;
    float gravityScale = 1.0f; // 1.0 = Normal, 0.0 = Float, -1.0 = Up
    float angleMin = 0.f;      // 0 - 2PI (6.28)
    float angleMax = 6.28f;
    sf::Vector2f velocityBias = {0.f, 0.f}; // [NEW] Add constant velocity (e.g. Upward wind)
    
    // Visuals
    std::vector<sf::Color> colors;
    
    // Advanced
    float groundRatio = 0.0f; // 0.0 = All Air, 1.0 = All Ground
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float lifeRemaining;
    float totalLifetime;
    bool active;
    bool onGround;
    float groundY; // Altura Y donde "choca" con el piso

    float gravityScale = 1.0f; // [PHYSICS FIX]
    
    // [Z-SORTING]
    const Entity* sortOwner = nullptr;

    // [ATTACHMENT]
    Entity* followedEntity = nullptr;
    sf::Vector2f followOffset;
    std::string profileName; // [CLEANUP] To identify and remove specific effects
    
    // [STUN VISUALS]
    bool isStunParticle;
    sf::Vector2f orbitCenter;
    float orbitRadius;
    float orbitAngle;
    float orbitSpeed;
    const Entity* owner = nullptr; // [VISUALS] Owner info
    
    // [Z-SORTING FIX]
    bool isIndependentSorted = false;
    bool isWalkDust = false;

    // [NEW] Textured particles support
    const sf::Texture* texture = nullptr;
    sf::IntRect texRect;
    float sortingY = 0.f;
};

class ParticleSystem {
public:
    ParticleSystem(ResourceManager& res);
    
    // Inicializar (cargar textura, crear pool, registrar perfiles)
    void init();

    void update(sf::Time dt);

    
    // [LAYERED RENDERING]
    void drawBottom(sf::RenderTarget& target); // Ground particles
    void drawOwnedParticles(const Entity* owner, sf::RenderTarget& target); // [Z-SORTING] Draw particles sorted with owner
    void drawUnownedParticles(sf::RenderTarget& target); // [Z-SORTING] Draw remaining air particles
    
    // Deprecated? No, used if no sortOwner
    void drawTop(sf::RenderTarget& target) { drawUnownedParticles(target); } 
    
    void clear(); // [FIX] Clear all particles

    // Emitir partículas tipo explosión (golpe directo)
    void emitHitImpact(sf::Vector2f origin, int amount, sf::Color color = sf::Color::Red, const Entity* owner = nullptr); // [Z-SORTING]
    
    // [NEW] Power Strike Effect
    void emitPowerStrike(sf::Vector2f center, int amount = 30);

    // Emitir partículas tipo goteo (sangrado)
    void emitBloodDrip(sf::Vector2f origin, int amount, sf::Color color = sf::Color::Red, const Entity* owner = nullptr); // [Z-SORTING]

    // Emitir partículas tipo sangre (golpe)
    void emitBlood(sf::Vector2f origin, int amount, sf::Color color = sf::Color::Red, const Entity* owner = nullptr); // [Z-SORTING]

    // [AOE VISUALS]
    void emitRockBurst(sf::Vector2f center, float radius, int amount, const Entity* owner = nullptr); // [Z-SORTING]
    
    // [STUN VISUALS]
    void emitStunStars(const Entity* target, float duration);
    
    // [NEW] Berserker Fury particles
    void emitBerserkerFury(const Entity* caster, int amount = 30);
    
    // [PHYSICS]
    void emitWalkDust(sf::Vector2f origin, sf::Color color, const Entity* owner = nullptr); // [Z-SORTING]

    // Emitir otro tipo (genérico)
    void emit(sf::Vector2f origin, int amount, sf::Color color, float speed, float lifetime);

    // [DATA DRIVEN] Generic Emitter
    // overrideRadius: If > 0, overrides profile's spawnRadiusMax. Useful for variable AOE.
    void emit(const std::string& profileName, sf::Vector2f center, float overrideRadius = 0.f);
    // [Z-SORTING]
    // overrideRadius > 0 usa ese radio en vez del profile
    // overrideCount > 0 usa esa cantidad en vez del profile
    void emitOwned(const std::string& profileName, sf::Vector2f center, const Entity* owner, float overrideRadius = 0.f, int overrideCount = 0); // [Z-SORTING]
    
    // [ATTACHMENT]
    void emitAttached(const std::string& profileName, Entity* owner, sf::Vector2f offset);
    void removeParticlesByProfile(Entity* owner, const std::string& profileName); // Cleans up attached particles
    
    // [TEXTURED SKILL PARTICLES]
    void emitOrbParticles(sf::Vector2f origin, int count, const sf::Texture* tex, const std::vector<sf::IntRect>& rects, float speedMin, float speedMax, float lifeMin, float lifeMax, float spawnRadius = 0.f, sf::Vector2f directionBias = {0.f, 0.f});
    void emitRisingParticles(sf::Vector2f center, float radius, int count, const sf::Texture* tex, const std::vector<sf::IntRect>& rects, float speedMin, float speedMax, float lifeMin, float lifeMax);
    
    // Registry
    // Registry
    void registerProfile(const std::string& name, const ParticleProfile& profile);

    // [SMART BATCHING] Public Accessor
    bool hasOwnedParticles(const Entity* owner) const {
        return mOwnedVertices.find(owner) != mOwnedVertices.end();
    }

    // [OPTIMIZATION] Explicit cleanup to prevent map thrashing
    void onEntityDeath(const Entity* owner);
    
    // [DEBUG]
    int getActiveCount() const { return mActiveCount; }
    size_t getOwnedParticleBatchCount() const { return mOwnedVertices.size(); }

    // [Z-SORTING FIX] Independent Renderable for Sorting
    class ParticleRenderable : public IRenderable {
    public:
        ParticleRenderable() : mParticle(nullptr) {}
        ParticleRenderable(const Particle* p) : mParticle(p) {}
        void setParticle(const Particle* p) { mParticle = p; }
        
        float getY() const {
            if (!mParticle) return 0.f;
            return mParticle->sortingY != 0.f ? mParticle->sortingY : mParticle->position.y;
        }

        virtual void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const override {
            if (!mParticle) return;
            // [PIXEL PERFECT FIX] Snap position to grid
            sf::Vector2f snappedPos;
            snappedPos.x = std::floor(mParticle->position.x);
            snappedPos.y = std::floor(mParticle->position.y);
            sf::Color col = mParticle->color;
            
            if (mParticle->texture) {
                float size = 2.0f; // Size is 2x2 for these particles
                sf::FloatRect texCoords(mParticle->texRect);
                
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x, snappedPos.y), col, sf::Vector2f(texCoords.position.x, texCoords.position.y)});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x + size, snappedPos.y), col, sf::Vector2f(texCoords.position.x + texCoords.size.x, texCoords.position.y)});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x + size, snappedPos.y + size), col, sf::Vector2f(texCoords.position.x + texCoords.size.x, texCoords.position.y + texCoords.size.y)});

                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x + size, snappedPos.y + size), col, sf::Vector2f(texCoords.position.x + texCoords.size.x, texCoords.position.y + texCoords.size.y)});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x, snappedPos.y + size), col, sf::Vector2f(texCoords.position.x, texCoords.position.y + texCoords.size.y)});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x, snappedPos.y), col, sf::Vector2f(texCoords.position.x, texCoords.position.y)});
                
                texture = mParticle->texture;
            } else {
                float size = 1.0f; // Matching PARTICLE_SIZE
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x, snappedPos.y), col});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x + size, snappedPos.y), col});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x + size, snappedPos.y + size), col});

                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x + size, snappedPos.y + size), col});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x, snappedPos.y + size), col});
                vertices.push_back(sf::Vertex{sf::Vector2f(snappedPos.x, snappedPos.y), col});
                
                texture = nullptr;
            }
        }
        
    private:
        const Particle* mParticle = nullptr;
    };

    // [Z-SORTING FIX] Accessor for independent sortable particles
    const std::vector<ParticleRenderable*>& getIndependentRenderables() const {
        return mActiveRenderables;
    }

private:
    std::map<std::string, ParticleProfile> mProfiles;

private:
    void resetParticle(int index, sf::Vector2f origin, sf::Vector2f vel, float lifetime, sf::Color color);

private:
    std::vector<Particle> mParticles;
    
    // [LAYERED RENDERING] Split vertex arrays
    std::vector<sf::Vertex> mGroundVertices;
    std::vector<sf::Vertex> mAirVertices;
    
    // [Z-SORTING] Batch per owner
    std::map<const Entity*, std::vector<sf::Vertex>> mOwnedVertices;

    // [Z-SORTING FIX] Pool for independent sorting (fixed-size, no reallocation)
    std::vector<ParticleRenderable> mRenderablePool;
    std::vector<ParticleRenderable*> mActiveRenderables;
    size_t mRenderablePoolIndex = 0; // [MEMORY FIX] Index into fixed pool
    int mCleanupCounter = 0; // [MEMORY FIX] Periodic map cleanup
    
    const sf::Texture* mTexture = nullptr; // [OPTIMIZATION] Solo iterar vivas
    int mActiveCount = 0; // [OPTIMIZATION] Solo iterar vivas
    static const int MAX_PARTICLES = 3000; // [OPTIMIZATION] Reduced from 10k
    
    ResourceManager& mRes;
};
