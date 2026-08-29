#include "ParticleSystem.h"
#include "../../entities/Entity.h"
#include "Config.h" // [NEW]
#include <cmath>
#include "utils/Random.h"
#include <iostream>
#include <cstdint>

ParticleSystem::ParticleSystem(ResourceManager& res) 
    : mActiveCount(0), mRes(res)
{
    mParticles.resize(MAX_PARTICLES);
    // 6 vertices por partícula (2 triángulos)
    mParticles.resize(MAX_PARTICLES);
    // [LAYERED RENDERING]
    mGroundVertices.reserve(MAX_PARTICLES * 6);
    mAirVertices.reserve(MAX_PARTICLES * 6);
    
    // [Z-SORTING FIX] Pre-allocate fixed pool to avoid reallocation (which invalidates pointers)
    mRenderablePool.resize(MAX_PARTICLES); // Fixed size, never reallocates
    mActiveRenderables.reserve(MAX_PARTICLES);
    mRenderablePoolIndex = 0;
}

void ParticleSystem::init() {
    // Futura carga de textura
    
    // [DATA DRIVEN] Register Default Profiles
    
    // 1. HIT IMPACT (Generic Red) -> Replaces emitHitImpact hardcode
    ParticleProfile hit;
    hit.countMin = cfg::Particles::HIT_COUNT_MIN; hit.countMax = cfg::Particles::HIT_COUNT_MAX;
    hit.speedMin = cfg::Particles::HIT_SPEED_MIN; hit.speedMax = cfg::Particles::HIT_SPEED_MAX;
    hit.lifeMin = cfg::Particles::HIT_LIFE_MIN; hit.lifeMax = cfg::Particles::HIT_LIFE_MAX;
    hit.colors = { sf::Color(232, 59, 59), sf::Color(174, 35, 52), sf::Color(110, 39, 39) };
    hit.velocityBias = {0.f, cfg::Particles::HIT_BIAS_Y};
    registerProfile("HIT_IMPACT", hit);

    // 2. ROCK BURST (AOE)
    ParticleProfile rock;
    rock.countMin = cfg::Particles::ROCK_COUNT_MIN; rock.countMax = cfg::Particles::ROCK_COUNT_MAX;
    rock.spawnRadiusMin = 0.f; rock.spawnRadiusMax = 1.0f; 
    rock.groundRatio = 1.0f; 
    rock.colors = { sf::Color(101, 67, 33), sf::Color(120, 100, 80), sf::Color(60, 40, 20) };
    rock.speedMin = cfg::Particles::ROCK_SPEED_MIN; rock.speedMax = cfg::Particles::ROCK_SPEED_MAX; 
    rock.velocityBias = {0.f, 0.f};
    rock.lifeMin = cfg::Particles::ROCK_LIFE_MIN; rock.lifeMax = cfg::Particles::ROCK_LIFE_MAX;
    registerProfile("ROCK_BURST", rock);

    // [VISUAL FIX] "Small particles jump aside from the circle"
    ParticleProfile debris;
    debris.countMin = cfg::Particles::DEBRIS_COUNT_MIN; debris.countMax = cfg::Particles::DEBRIS_COUNT_MAX;
    debris.spawnRadiusMin = 0.f; debris.spawnRadiusMax = 1.0f; 
    debris.groundRatio = 0.0f; 
    debris.colors = { sf::Color(80, 60, 40), sf::Color(100, 80, 60) };
    debris.speedMin = cfg::Particles::DEBRIS_SPEED_MIN; debris.speedMax = cfg::Particles::DEBRIS_SPEED_MAX;
    debris.velocityBias = {0.f, cfg::Particles::DEBRIS_BIAS_Y};
    debris.lifeMin = cfg::Particles::DEBRIS_LIFE_MIN; debris.lifeMax = cfg::Particles::DEBRIS_LIFE_MAX;
    debris.gravityScale = cfg::Particles::DEBRIS_GRAVITY_SCALE; 
    registerProfile("ROCK_DEBRIS", debris);

    // 2. POWER STRIKE (Gold/Orange Burst)
    ParticleProfile power;
    power.countMin = cfg::Particles::POWER_COUNT_MIN; power.countMax = cfg::Particles::POWER_COUNT_MAX;
    power.speedMin = cfg::Particles::POWER_SPEED_MIN; power.speedMax = cfg::Particles::POWER_SPEED_MAX;
    power.lifeMin = cfg::Particles::POWER_LIFE_MIN; power.lifeMax = cfg::Particles::POWER_LIFE_MAX;
    power.colors = { sf::Color(255, 215, 0), sf::Color(255, 140, 0), sf::Color(255, 255, 200) };
    power.angleMin = 0.f; power.angleMax = 6.28f;
    registerProfile("POWER_STRIKE", power);
    
    // 3. BLOOD DRIP
    ParticleProfile blood;
    blood.countMin = cfg::Particles::BLOOD_COUNT_MIN; blood.countMax = cfg::Particles::BLOOD_COUNT_MAX;
    blood.speedMin = cfg::Particles::BLOOD_SPEED_MIN; blood.speedMax = cfg::Particles::BLOOD_SPEED_MAX;
    blood.lifeMin = cfg::Particles::BLOOD_LIFE_MIN; blood.lifeMax = cfg::Particles::BLOOD_LIFE_MAX;
    blood.colors = { sf::Color(232, 59, 59), sf::Color(174, 35, 52), sf::Color(110, 39, 39) };
    registerProfile("BLOOD_DRIP", blood);

    // 4. GOLD CHARGE (Hand)
    ParticleProfile charge;
    charge.countMin = cfg::Particles::CHARGE_COUNT_MIN; charge.countMax = cfg::Particles::CHARGE_COUNT_MAX;
    charge.speedMin = cfg::Particles::CHARGE_SPEED_MIN; charge.speedMax = cfg::Particles::CHARGE_SPEED_MAX;
    charge.lifeMin = cfg::Particles::CHARGE_LIFE_MIN; charge.lifeMax = cfg::Particles::CHARGE_LIFE_MAX;
    charge.colors = { sf::Color(255, 255, 0), sf::Color(255, 215, 0), sf::Color(255, 255, 200) }; 
    charge.spawnRadiusMax = 10.f; 
    charge.gravityScale = cfg::Particles::CHARGE_GRAVITY_SCALE; 
    registerProfile("GOLD_CHARGE", charge);

    // 5. GOLD CHARGE EMIT (Continuous Trail)
    ParticleProfile chargeTick;
    chargeTick.countMin = cfg::Particles::CHARGE_TICK_COUNT_MIN; chargeTick.countMax = cfg::Particles::CHARGE_TICK_COUNT_MAX;
    chargeTick.speedMin = cfg::Particles::CHARGE_TICK_SPEED_MIN; chargeTick.speedMax = cfg::Particles::CHARGE_TICK_SPEED_MAX;
    chargeTick.lifeMin = cfg::Particles::CHARGE_TICK_LIFE_MIN; chargeTick.lifeMax = cfg::Particles::CHARGE_TICK_LIFE_MAX;
    chargeTick.colors = { sf::Color(255, 255, 100), sf::Color(255, 200, 50) };
    chargeTick.spawnRadiusMax = 15.f; 
    chargeTick.gravityScale = cfg::Particles::CHARGE_TICK_GRAVITY_SCALE; 
    registerProfile("GOLD_CHARGE_EMIT", chargeTick);
}

void ParticleSystem::registerProfile(const std::string& name, const ParticleProfile& profile) {
    mProfiles[name] = profile;
}

void ParticleSystem::emit(const std::string& profileName, sf::Vector2f center, float overrideRadius) {
    auto it = mProfiles.find(profileName);
    if (it == mProfiles.end()) return; // Profile not found
    
    const ParticleProfile& p = it->second;
    
    // [DYNAMIC COUNT FOR AOE?]
    int count = Random::Int(p.countMin, p.countMax);
    if (overrideRadius > 0.f && profileName == "ROCK_BURST") {
        // Preserve old logic: count = radius * 0.8
        count = static_cast<int>(overrideRadius * 0.8f);
        if (count < 20) count = 20;
        if (count > 100) count = 100;
    }
    
    float radiusMax = (overrideRadius > 0.f) ? overrideRadius : p.spawnRadiusMax;

    for(int i=0; i<count; ++i) {
        if(mActiveCount >= MAX_PARTICLES) return;
        
        // 1. Position (Radius)
        float anglePos = Random::Float(0.f, 6.28f);
        float dist = std::sqrt(Random::Float(0.f, 1.f)) * Random::Float(p.spawnRadiusMin, radiusMax);
        sf::Vector2f pos = center + sf::Vector2f(std::cos(anglePos)*dist, std::sin(anglePos)*dist);
        
        // 2. Velocity (Speed + Angle + Bias)
        float angleVel = Random::Float(p.angleMin, p.angleMax);
        float speed = Random::Float(p.speedMin, p.speedMax);
        sf::Vector2f vel(std::cos(angleVel)*speed, std::sin(angleVel)*speed);
        
        // [BIAS] Add directional bias (e.g. Up for blood/debris)
        vel += p.velocityBias;
        
        // 3. Life & Color
        float life = Random::Float(p.lifeMin, p.lifeMax);
        sf::Color color = sf::Color::White;
        if(!p.colors.empty()) {
            color = p.colors[Random::Int(0, static_cast<int>(p.colors.size()) - 1)];
        }
        
        // 4. Ground vs Air (Logic from RockBurst)
        resetParticle(mActiveCount, pos, vel, life, color);
        mParticles[mActiveCount].gravityScale = p.gravityScale; // [PHYSICS FIX]
        
         if (Random::Float(0.f, 1.f) < p.groundRatio) {
            mParticles[mActiveCount].onGround = true;
            mParticles[mActiveCount].groundY = pos.y; // Assume flat ground at spawn Y
            mParticles[mActiveCount].velocity = {0.f, 0.f}; // Freeze ground particles usually?
        }

        mActiveCount++;
    }
}

// [Z-SORTING]
void ParticleSystem::emitOwned(const std::string& profileName, sf::Vector2f center, const Entity* owner, float overrideRadius, int overrideCount) {
    auto it = mProfiles.find(profileName);
    if (it == mProfiles.end()) return;

    const ParticleProfile& p = it->second;
    int count = (overrideCount > 0) ? overrideCount : Random::Int(p.countMin, p.countMax);
    float radiusMax = (overrideRadius > 0.f) ? overrideRadius : p.spawnRadiusMax;

    for(int i=0; i<count; ++i) {
        if(mActiveCount >= MAX_PARTICLES) return;
        
        float anglePos = Random::Float(0.f, 6.28f);
        float dist = std::sqrt(Random::Float(0.f, 1.f)) * Random::Float(p.spawnRadiusMin, radiusMax);
        sf::Vector2f pos = center + sf::Vector2f(std::cos(anglePos)*dist, std::sin(anglePos)*dist);
        
        float angleVel = Random::Float(p.angleMin, p.angleMax);
        float speed = Random::Float(p.speedMin, p.speedMax);
        sf::Vector2f vel(std::cos(angleVel)*speed, std::sin(angleVel)*speed);
        vel += p.velocityBias;
        
        float life = Random::Float(p.lifeMin, p.lifeMax);
        sf::Color color = sf::Color::White;
        if(!p.colors.empty()) color = p.colors[Random::Int(0, static_cast<int>(p.colors.size()) - 1)];
        
        resetParticle(mActiveCount, pos, vel, life, color);
        mParticles[mActiveCount].gravityScale = p.gravityScale; 
        mParticles[mActiveCount].sortOwner = owner; // KEY

        // [PHYSICS FIX] Ensure particles fall to the entity's actual feet, not just below their spawn point
        if (owner) {
            mParticles[mActiveCount].groundY = owner->getGroundPosition().y + Random::Float(-5.f, 5.f);
        }

        // [VISUAL FIX] "Cae el AOE" -> emitOwned ignored groundRatio!
        // We must check if this profile is meant to be on ground (like ROCK_BURST)
        if (Random::Float(0.f, 1.f) < p.groundRatio) {
             mParticles[mActiveCount].onGround = true;
             mParticles[mActiveCount].groundY = pos.y;
             mParticles[mActiveCount].velocity = {0.f, 0.f}; // Freeze
             mParticles[mActiveCount].gravityScale = 0.f;
        }

        mActiveCount++;
    }
}

void ParticleSystem::emitAttached(const std::string& profileName, Entity* owner, sf::Vector2f offset) {
    if (!owner) return;
    auto it = mProfiles.find(profileName);
    if (it == mProfiles.end()) return;

    const ParticleProfile& p = it->second;
    int count = Random::Int(p.countMin, p.countMax);
    
    for(int i=0; i<count; ++i) {
        if(mActiveCount >= MAX_PARTICLES) return;
        
        float anglePos = Random::Float(0.f, 6.28f);
        float dist = std::sqrt(Random::Float(0.f, 1.f)) * p.spawnRadiusMax;
        sf::Vector2f relativePos = sf::Vector2f(std::cos(anglePos)*dist, std::sin(anglePos)*dist);
        sf::Vector2f spawnPos = owner->getPosition() + offset + relativePos;
        
        float angleVel = Random::Float(p.angleMin, p.angleMax);
        float speed = Random::Float(p.speedMin, p.speedMax);
        sf::Vector2f vel(std::cos(angleVel)*speed, std::sin(angleVel)*speed);
        vel += p.velocityBias;
        
        float life = 1000.0f; // Infinite
        sf::Color color = sf::Color::White;
        if(!p.colors.empty()) color = p.colors[Random::Int(0, p.colors.size()-1)];
        
        resetParticle(mActiveCount, spawnPos, vel, life, color);
        
        mParticles[mActiveCount].velocity = {0.f, 0.f}; // Fixed
        mParticles[mActiveCount].gravityScale = 0.f; // Attached = No Gravity (controlled manually)
        mParticles[mActiveCount].profileName = profileName;
        mParticles[mActiveCount].followedEntity = owner;
        mParticles[mActiveCount].followOffset = offset + relativePos;
        
        mActiveCount++;
    }
}

void ParticleSystem::removeParticlesByProfile(Entity* owner, const std::string& profileName) {
    for(int i=0; i<mActiveCount; ++i) {
        if (mParticles[i].followedEntity == owner && mParticles[i].profileName == profileName) {
            mParticles[i].lifeRemaining = 0.f; // Kill
        }
    }
}

void ParticleSystem::emitOrbParticles(sf::Vector2f origin, int count, const sf::Texture* tex, const std::vector<sf::IntRect>& rects, float speedMin, float speedMax, float lifeMin, float lifeMax, float spawnRadius, sf::Vector2f directionBias) {
    if (!tex || rects.empty()) return;

    for (int i = 0; i < count; ++i) {
        if (mActiveCount >= MAX_PARTICLES) return;

        float anglePos = Random::Float(0.f, 6.2831853f);
        float dist = Random::Float(0.f, spawnRadius);
        sf::Vector2f spawnPos = origin + sf::Vector2f(std::cos(anglePos) * dist, std::sin(anglePos) * dist);

        float angleVel = Random::Float(0.f, 6.2831853f);
        float speed = Random::Float(speedMin, speedMax);
        sf::Vector2f vel = sf::Vector2f(std::cos(angleVel) * speed, std::sin(angleVel) * speed) + directionBias;

        float life = Random::Float(lifeMin, lifeMax);

        int idx = mActiveCount;
        resetParticle(idx, spawnPos, vel, life, sf::Color::White);

        Particle& p = mParticles[idx];
        p.gravityScale = 0.f;
        p.isIndependentSorted = true;
        p.texture = tex;
        p.texRect = rects[Random::Int(0, static_cast<int>(rects.size()) - 1)];
        p.sortingY = spawnPos.y;

        mActiveCount++;
    }
}

void ParticleSystem::emitRisingParticles(sf::Vector2f center, float radius, int count, const sf::Texture* tex, const std::vector<sf::IntRect>& rects, float speedMin, float speedMax, float lifeMin, float lifeMax) {
    if (!tex || rects.empty()) return;

    for (int i = 0; i < count; ++i) {
        if (mActiveCount >= MAX_PARTICLES) return;

        float anglePos = Random::Float(0.f, 6.2831853f);
        float dist = std::sqrt(Random::Float(0.f, 1.f)) * radius;
        sf::Vector2f spawnPos = center + sf::Vector2f(std::cos(anglePos) * dist, std::sin(anglePos) * dist * 0.5f);

        float riseSpeed = Random::Float(speedMin, speedMax);
        float driftX = Random::Float(-6.f, 6.f);
        sf::Vector2f vel = sf::Vector2f(driftX, -riseSpeed);

        float life = Random::Float(lifeMin, lifeMax);

        int idx = mActiveCount;
        resetParticle(idx, spawnPos, vel, life, sf::Color::White);

        Particle& p = mParticles[idx];
        p.gravityScale = 0.f;
        p.isIndependentSorted = true;
        p.texture = tex;
        p.texRect = rects[Random::Int(0, static_cast<int>(rects.size()) - 1)];
        p.sortingY = spawnPos.y;

        mActiveCount++;
    }
}

void ParticleSystem::emitBlood(sf::Vector2f origin, int amount, sf::Color color, const Entity* owner) {
    // Deprecated wrapper -> redirects to Generic Profile
    // We ignore 'amount' and 'color' args in favor of Profile defaults to stay clean,
    // or we could overload emit() to accept overrides.
    // For faithful reproduction:
    if (owner) emitOwned("HIT_IMPACT", origin, owner);
    else emit("HIT_IMPACT", origin);
}

void ParticleSystem::emitHitImpact(sf::Vector2f origin, int amount, sf::Color color, const Entity* owner) {
    // [FULL MIGRATION]
    if (owner) emitOwned("HIT_IMPACT", origin, owner);
    else emit("HIT_IMPACT", origin);
}

// [POWER STRIKE VISUALS]
void ParticleSystem::emitPowerStrike(sf::Vector2f center, int amount) {
    // [REFACTOR] Now uses Data-Driven profile!
    // But we honor 'amount' if passed? For now profile dictates count.
    // If you want to override, we'd need generic override support.
    // Simpler: Just call generic.
    emit("POWER_STRIKE", center);
}

void ParticleSystem::emitBloodDrip(sf::Vector2f origin, int amount, sf::Color color, const Entity* owner) {
    for (int i = 0; i < amount; ++i) {
        if (mActiveCount >= MAX_PARTICLES) return;

        // [DRIPPING BLOOD EFFECT]
        // Instead of explosion, small horizontal spread and gravity pull
        float vx = Random::Float(-25.f, 25.f); 
        float vy = Random::Float(-50.f, 0.f); // Less pop than impact

        sf::Vector2f vel(vx, vy);
        float life = Random::Float(1.5f, 3.0f); // Longer, lingering drip

        // Add some randomness to the spawn position around the origin
        sf::Vector2f spawnPos = origin;
        spawnPos.x += Random::Float(-5.f, 5.f);
        spawnPos.y += Random::Float(-5.f, 5.f);

        sf::Color finalColor = color;
        if (color == sf::Color::Red) {
            static const sf::Color paletteReds[] = {
                sf::Color(232, 59, 59),
                sf::Color(174, 35, 52),
                sf::Color(110, 39, 39)
            };
            finalColor = paletteReds[Random::Int(0, 2)];
        }

        resetParticle(mActiveCount, spawnPos, vel, life, finalColor);
        mParticles[mActiveCount].sortOwner = owner; // [Z-SORTING]
        
        // [PHYSICS FIX] Make blood drip fall exactly to the entity's feet!
        if (owner) {
            mParticles[mActiveCount].groundY = owner->getGroundPosition().y + Random::Float(-3.f, 3.f);
        }
        
        mActiveCount++;
    }
}

// [AOE VISUALS]
// [AOE VISUALS]
void ParticleSystem::emitRockBurst(sf::Vector2f center, float radius, int amount, const Entity* owner) {
    // [AOE FIX] User requested "Instant" application respecting radius.
    // We pass 'radius' as overrideRadius. This spawns particles instantly across the full area.
    // Since 'owner' is passed, they are Z-Sorted correctly with the target.
    if (owner) emitOwned("ROCK_BURST", center, owner, radius, amount);
    else emit("ROCK_BURST", center, radius); // Fallback
}

void ParticleSystem::emit(sf::Vector2f origin, int amount, sf::Color color, float speed, float lifetime) {
     // Implementación genérica similar a emitBlood si se necesita
}

void ParticleSystem::resetParticle(int index, sf::Vector2f origin, sf::Vector2f vel, float lifetime, sf::Color color) {
    Particle& p = mParticles[index];
    p.active = true;
    p.position = origin;
    p.velocity = vel;
    p.color = color;
    p.totalLifetime = lifetime;
    p.lifeRemaining = lifetime;
    p.onGround = false;
    p.groundY = origin.y + Random::Float(10.f, 20.f); 
    p.isStunParticle = false; // Default
    p.isWalkDust = false; // [NEW] Default
    p.profileName = "";
    p.gravityScale = 1.0f; // [PHYSICS FIX] Reset to default gravity
    p.sortOwner = nullptr; // [Z-SORTING]
    p.owner = nullptr;     // Prevent dangling pointer on reuse
    p.followedEntity = nullptr; // Prevent dangling pointer on reuse
    p.isIndependentSorted = false;
    p.texture = nullptr;
    p.texRect = {};
    p.sortingY = 0.f;
}

void ParticleSystem::emitStunStars(const Entity* target, float duration) {
    if (!target) return;

    // [ANTI-STACKING] Remove existing stun particles for this target
    for (int i = 0; i < mActiveCount; ++i) {
        if (mParticles[i].active && mParticles[i].isStunParticle && mParticles[i].owner == target) {
             mParticles[i].active = false;
             // We don't decrement mActiveCount here to avoid complex swapping logic mid-loop
             // Instead, set lifetime to 0 to be cleaned up next update, OR force active=false
             mParticles[i].lifeRemaining = 0.f; 
        }
    }

    int amount = 8; 
    float radius = cfg::Player::STUN_HALO_RADIUS; // [CONFIG]
    sf::Vector2f center = target->getVisualPoint("head");

    for (int i = 0; i < amount; ++i) {
        if (mActiveCount >= MAX_PARTICLES) return;
        
        // Use resetParticle to ensure clean state (clears independent flag etc)
        // Position will be overwritten by orbit logic
        resetParticle(mActiveCount, center, {0.f, 0.f}, duration, sf::Color::Yellow);
        
        Particle& p = mParticles[mActiveCount];
        
        p.orbitCenter = center;
        p.orbitRadius = radius;
        p.orbitAngle = (float)i * (6.28f / (float)amount); // Distribute evenly
        p.orbitSpeed = 5.0f; // Speed of rotation
        p.isStunParticle = true;
        p.owner = target;
        p.sortOwner = target; // [FIX] Ensure it draws with the owner (On Top)
        
        // [CRITICAL FIX] Reset physics state to ensure orbit logic runs
        p.onGround = false; 
        p.groundY = center.y + 1000.f; // Irrelevant but safe

        // Initial pos
        p.position.x = p.orbitCenter.x + std::cos(p.orbitAngle) * p.orbitRadius;
        p.position.y = p.orbitCenter.y + std::sin(p.orbitAngle) * (p.orbitRadius * 0.5f);
        
        mActiveCount++;
    }
}

// [Z-SORTING] Added owner arg
void ParticleSystem::emitWalkDust(sf::Vector2f origin, sf::Color color, const Entity* owner) {
    if (color == sf::Color::Transparent) return;
    if (mActiveCount >= MAX_PARTICLES) return;

    // Ajustar color y opacidad según configuración
    color.r = static_cast<std::uint8_t>(std::min(255.0f, color.r * cfg::Particles::WALK_DUST_COLOR_MULT));
    color.g = static_cast<std::uint8_t>(std::min(255.0f, color.g * cfg::Particles::WALK_DUST_COLOR_MULT));
    color.b = static_cast<std::uint8_t>(std::min(255.0f, color.b * cfg::Particles::WALK_DUST_COLOR_MULT));
    color.a = static_cast<std::uint8_t>(cfg::Particles::WALK_DUST_ALPHA);

    int amount = Random::Int(cfg::Particles::WALK_DUST_COUNT_MIN, cfg::Particles::WALK_DUST_COUNT_MAX);

    for (int i = 0; i < amount; ++i) {
        if (mActiveCount >= MAX_PARTICLES) break;

        // Origen exactamente alrededor de la suela de la bota
        sf::Vector2f spawnPos = origin;
        spawnPos.x += Random::Float(-cfg::Particles::WALK_DUST_OFFSET_X, cfg::Particles::WALK_DUST_OFFSET_X);
        spawnPos.y += Random::Float(-cfg::Particles::WALK_DUST_OFFSET_Y, cfg::Particles::WALK_DUST_OFFSET_Y);

        // Impulso suave hacia los lados y apenas un levante hacia arriba
        float vx = Random::Float(-cfg::Particles::WALK_DUST_SPEED_X, cfg::Particles::WALK_DUST_SPEED_X); 
        float vy = Random::Float(cfg::Particles::WALK_DUST_SPEED_Y_MIN, cfg::Particles::WALK_DUST_SPEED_Y_MAX);
        
        float life = Random::Float(cfg::Particles::WALK_DUST_LIFE_MIN, cfg::Particles::WALK_DUST_LIFE_MAX);

        resetParticle(mActiveCount, spawnPos, {vx, vy}, life, color);
        // Anclar el nivel de suelo a la altura exacta donde pisó para que no reboten como pulgas
        mParticles[mActiveCount].groundY = origin.y + Random::Float(-1.f, 2.f);
        mParticles[mActiveCount].gravityScale = 0.5f; // Gravedad más suave para que flote como polvo
        mParticles[mActiveCount].isIndependentSorted = false;
        mParticles[mActiveCount].sortOwner = nullptr; // [FIX] Draw walk dust below entity parts (on the bottom layer)
        mParticles[mActiveCount].isWalkDust = true;
        
        mActiveCount++;
    }
}

void ParticleSystem::update(sf::Time dt) {

    float dtSec = dt.asSeconds();
    
    // [OPTIMIZATION] Cache last looked up entity to avoid std::map searches in loop
    const Entity* lastOwner = nullptr;
    std::vector<sf::Vertex>* lastBatch = nullptr;

    // [LAYERED RENDERING] Clear vertex arrays (keep capacity)
    mGroundVertices.clear();
    mAirVertices.clear();
    // [Z-SORTING]
    // [MEMORY LEAK FIX] Move prune BEFORE clearing, and do it more often (every 10 frames)
    mCleanupCounter++;
    if (mCleanupCounter >= 10) {
        mCleanupCounter = 0;
        for (auto it = mOwnedVertices.begin(); it != mOwnedVertices.end(); ) {
            if (it->second.empty()) {
                it = mOwnedVertices.erase(it);
            } else {
                ++it;
            }
        }
    }

    // [OPTIMIZATION] Clear existing vectors in the map for reuse
    for (auto& pair : mOwnedVertices) {
        pair.second.clear();
    }
    
    // [Z-SORTING FIX] Reset pool index (fixed-size pool, no reallocation)
    mRenderablePoolIndex = 0;
    mActiveRenderables.clear();

    // [OPTIMIZATION] Solo iteramos hasta mActiveCount
    for (int i = 0; i < mActiveCount; ++i) {
        Particle& p = mParticles[i];
        
        // Muerte por tiempo
        p.lifeRemaining -= dtSec;

        if (p.lifeRemaining <= 0.f) {
            // [SWAP-REMOVE OPTIMIZATION]
            mParticles[i] = mParticles[mActiveCount - 1]; 
            mActiveCount--;
            i--; 
            continue;
        }



        // [ATTACHMENT PHYSICS]
        if (p.followedEntity) {
             // Hard-lock position to owner + offset
             sf::Vector2f ownerPos = p.followedEntity->getPosition();
             
             // [ANIMATION SYNC HACK]
             // Ideally we know the frame. But for now, just owner pos.
             
             p.position = ownerPos + p.followOffset;
             
             // Dynamic Jitter for "Energy" feel?
             // float time = dt.asSeconds();
             // p.followOffset.y -= 10.f * time; // Float up within the attachment?
             // If we modify followOffset they will drift away forever.
             // We want them to vibrate.
        } else {
             // Normal Physics
             // ...
        }

        // Física
        if (!p.onGround && !p.followedEntity) { // [FIX] Don't apply gravity to attached particles
             if (p.isStunParticle) {
              // [DEATH/UNSTUN CHECK]
              if (p.owner && (p.owner->getCurrentHp() <= 0 || !p.owner->isStunned())) {
                  p.lifeRemaining = 0.f; // Kill immediately
                  continue;
              }

              // [STUN HALO REFRESH] Keep particles alive as long as owner is stunned!
              if (p.owner && p.owner->isStunned()) {
                  p.lifeRemaining = 1.0f;
              }
             
              // [FOLLOW TARGET]
              if (p.owner) {
                  p.orbitCenter = p.owner->getVisualPoint("head");
              }

             // [STUN ORBIT LOGIC]
             p.orbitAngle += p.orbitSpeed * dt.asSeconds();
             p.position.x = p.orbitCenter.x + std::cos(p.orbitAngle) * p.orbitRadius;
             // Oval shape? standard circle for now
             p.position.y = p.orbitCenter.y + std::sin(p.orbitAngle) * (p.orbitRadius * 0.5f); // Flattened Y for perspective
             
             // No Gravity
            } else {

             // [STANDARD PHYSICS]
             // [PHYSICS FIX] Apply per-particle gravity scale
             p.velocity.y += cfg::Particles::GRAVITY * p.gravityScale * dt.asSeconds();
             p.position += p.velocity * dt.asSeconds();
 
             if (p.position.y >= p.groundY && !p.onGround) {
                 p.position.y = p.groundY;
                 p.velocity.y = -p.velocity.y * 0.5f; 
                 p.velocity.x *= 0.5f;
                 p.onGround = true;
             }
            }
        } 

        // Generate Vertices (on stack)
        float currentSize = cfg::Particles::BASE_SIZE;
        if (p.isStunParticle) currentSize = cfg::Player::STUN_PARTICLE_SIZE; // [CONFIG]

        float half = currentSize * 0.5f;
        sf::Vector2f pos = p.position;

        std::uint8_t alpha = 255;
        // Alpha Logic (Fade In / Fade Out)
        // Use totalLifetime to determine phase
        float lifeRatio = 0.f;
        if (p.totalLifetime > 0.001f) lifeRatio = p.lifeRemaining / p.totalLifetime;
        
        // Strategy: Fade In fast (first 20% of life, so ratio 1.0 -> 0.8)
        // Then Fade Out slow (ratio 0.8 -> 0.0)
        
        if (lifeRatio > 0.8f) {
            // FADE IN
            float t = (1.0f - lifeRatio) / 0.2f; // 0.0 -> 1.0
            alpha = static_cast<std::uint8_t>(255 * t);
        } else {
            // FADE OUT
            float t = lifeRatio / 0.8f; // 1.0 -> 0.0
            alpha = static_cast<std::uint8_t>(255 * t);
        }
        sf::Color finalColor = p.color;
        
        // [STUN FIX] Instant Alpha for Stun (No Fade-In)
        if (p.isStunParticle) {
             finalColor.a = 255; 
        } else {
             finalColor.a = alpha;
        }
        
        // [PIXEL PERFECT FIX] Snap position to grid for sharp 1x1 pixels
        sf::Vector2f snappedPos;
        snappedPos.x = std::floor(pos.x);
        snappedPos.y = std::floor(pos.y);

        sf::Vertex quad[6];
        // Triangulos para formar Quad (size is currentSize)
        quad[0].position = {snappedPos.x, snappedPos.y};
        quad[1].position = {snappedPos.x + currentSize, snappedPos.y};
        quad[2].position = {snappedPos.x + currentSize, snappedPos.y + currentSize};

        quad[3].position = {snappedPos.x + currentSize, snappedPos.y + currentSize};
        quad[4].position = {snappedPos.x, snappedPos.y + currentSize};
        quad[5].position = {snappedPos.x, snappedPos.y};

        for (int k = 0; k < 6; ++k) quad[k].color = finalColor;

        // [LAYERED RENDERING with SMART SORTING]
        // We separate Owned vs Air again.
        // PlayingState will check (via hasOwnedParticles) if an entity should be un-batched to draw these particles interleaved.
        if (p.onGround || p.isWalkDust) {
            for (int k = 0; k < 6; ++k) mGroundVertices.push_back(quad[k]);
        } else if (p.sortOwner) {
            // [Z-SORTING] Owned by Entity
            std::vector<sf::Vertex>* batchPtr = nullptr;
            if (p.sortOwner == lastOwner) {
                batchPtr = lastBatch;
            } else {
                batchPtr = &mOwnedVertices[p.sortOwner];
                lastOwner = p.sortOwner;
                lastBatch = batchPtr;
            }
            if (batchPtr->empty()) batchPtr->reserve(60); // Heuristic
            for (int k = 0; k < 6; ++k) batchPtr->push_back(quad[k]);
        } else if (p.isIndependentSorted) {
             // [Z-SORTING FIX] Independent Sorted — use pre-allocated pool (no realloc)
             if (mRenderablePoolIndex < mRenderablePool.size()) {
                 mRenderablePool[mRenderablePoolIndex].setParticle(&p);
                 mActiveRenderables.push_back(&mRenderablePool[mRenderablePoolIndex]);
                 mRenderablePoolIndex++;
             }
        } else {
            // Unowned Air
            for (int k = 0; k < 6; ++k) mAirVertices.push_back(quad[k]);
        }
    }
}

// [Z-SORTING]
void ParticleSystem::drawOwnedParticles(const Entity* owner, sf::RenderTarget& target) {
    if (!owner) return;
    auto it = mOwnedVertices.find(owner);
    if (it != mOwnedVertices.end()) {
        const std::vector<sf::Vertex>& batch = it->second;
        if (!batch.empty()) {
            target.draw(batch.data(), batch.size(), sf::PrimitiveType::Triangles);
        }
    }
}

void ParticleSystem::drawUnownedParticles(sf::RenderTarget& target) {
    if (!mAirVertices.empty()) {
        target.draw(mAirVertices.data(), mAirVertices.size(), sf::PrimitiveType::Triangles);
    }
}

void ParticleSystem::drawBottom(sf::RenderTarget& target) {
    if (mGroundVertices.empty()) return;
    target.draw(mGroundVertices.data(), mGroundVertices.size(), sf::PrimitiveType::Triangles);
}



void ParticleSystem::clear() {
    mActiveCount = 0;
    mOwnedVertices.clear(); // Also clear the map
}

void ParticleSystem::onEntityDeath(const Entity* owner) {
    if (!owner) return;
    // Efficiently remove from map to reclaim memory/prevent growth
    mOwnedVertices.erase(owner);
    // Null out references to prevent dangling pointers and allow remaining particles to draw
    for (int i = 0; i < mActiveCount; ++i) {
        if (mParticles[i].sortOwner == owner) {
            mParticles[i].sortOwner = nullptr;
        }
        if (mParticles[i].owner == owner) {
            mParticles[i].owner = nullptr;
        }
        if (mParticles[i].followedEntity == owner) {
            mParticles[i].followedEntity = nullptr;
            mParticles[i].lifeRemaining = 0.f; // Kill attached particles on owner death
        }
    }
}

void ParticleSystem::emitBerserkerFury(const Entity* caster, int amount) {
    if (!caster) return;

    // Load texture if not loaded yet
    static const sf::Texture* furyTexture = nullptr;
    if (!furyTexture) {
        try {
            furyTexture = &mRes.getTexture("assets/ui/skills/particle_skills/fury_particulas.png");
        } catch (...) {
            std::cerr << "[ParticleSystem] Failed to load fury_particulas.png!\n";
            return;
        }
    }

    // Coordinates for the 4 particles of 2x2:
    static const sf::IntRect rects[] = {
        sf::IntRect({0, 0}, {2, 2}),
        sf::IntRect({2, 0}, {2, 2}),
        sf::IntRect({0, 2}, {2, 2}),
        sf::IntRect({2, 2}, {2, 2})
    };

    // Center on ground/feet (using getGroundPosition() instead of getPosition() to correctly match feet Y-coordinate)
    sf::Vector2f groundCenter = caster->getGroundPosition();

    // Emit particles around the player
    for (int i = 0; i < amount; ++i) {
        if (mActiveCount >= MAX_PARTICLES) break;

        // Position: flat ellipse around player's feet
        float theta = Random::Float(0.f, 2.f * 3.14159f);
        float radius = Random::Float(10.f, 25.f); // radius of the aura
        
        float spawnX = groundCenter.x + std::cos(theta) * radius;
        // Y is flattened by 0.5 for perspective projection
        float spawnY = groundCenter.y + std::sin(theta) * radius * 0.5f;

        // Visual height offset: spawn at various heights of the player's body (uniform distribution)
        float heightOffset = Random::Float(0.f, 35.f); 

        // Velocity: rising straight upwards!
        float vx = 0.f;
        float vy = Random::Float(-35.f, -65.f); // going up!

        float life = Random::Float(0.8f, 1.5f);
        sf::Color color = sf::Color::White;

        // Spawn with the height offset applied to the screen Y coordinate
        resetParticle(mActiveCount, {spawnX, spawnY - heightOffset}, {vx, vy}, life, color);

        // Configure custom fields
        Particle& p = mParticles[mActiveCount];
        p.gravityScale = 0.f; // constant upward drift
        p.isIndependentSorted = true; // Y-sorted individually!
        p.sortOwner = nullptr; // so it's not batch-drawn on top of the entity
        p.texture = furyTexture;
        p.texRect = rects[Random::Int(0, 3)];
        p.sortingY = spawnY; // Crucial: sort by ground Y!

        mActiveCount++;
    }
}

