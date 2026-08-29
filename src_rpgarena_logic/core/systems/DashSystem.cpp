#include "DashSystem.h"
#include "entities/Entity.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include "core/systems/SoundSystem.h"
#include "map/ChunkedTileMap.h"
#include "utils/TinyJson.h"
#include <cmath>
#include <iostream>

DashSystem* DashSystem::sInstance = nullptr;

DashSystem::DashSystem() {
    sInstance = this;
    loadConfig();
}

DashSystem::~DashSystem() {
    if (sInstance == this) {
        sInstance = nullptr;
    }
}

bool DashSystem::loadConfig(const std::string& path) {
    json::Value root = json::parseFile(path);
    if (root.type != json::Type::Object) {
        std::cerr << "[DashSystem] No se pudo cargar " << path << ", usando valores por defecto.\n";
        return false;
    }

    const auto& obj = root.asObject();
    if (obj.count("dash_duration") && obj.at("dash_duration").type == json::Type::Number)
        mDashDuration = static_cast<float>(obj.at("dash_duration").asDouble());
    if (obj.count("dash_cooldown") && obj.at("dash_cooldown").type == json::Type::Number)
        mDashCooldown = static_cast<float>(obj.at("dash_cooldown").asDouble());
    if (obj.count("dash_speed") && obj.at("dash_speed").type == json::Type::Number)
        mDashSpeed = static_cast<float>(obj.at("dash_speed").asDouble());
    if (obj.count("particle_emit_interval") && obj.at("particle_emit_interval").type == json::Type::Number)
        mParticleEmitInterval = static_cast<float>(obj.at("particle_emit_interval").asDouble());
    if (obj.count("sound_path") && obj.at("sound_path").type == json::Type::String)
        mSoundPath = obj.at("sound_path").asString();
    if (obj.count("sound_volume") && obj.at("sound_volume").type == json::Type::Number)
        mSoundVolume = static_cast<float>(obj.at("sound_volume").asDouble());

    std::cout << "[DashSystem] Configuración cargada desde " << path << ": duration=" << mDashDuration
              << "s, cooldown=" << mDashCooldown << "s, speed=" << mDashSpeed << "px/s\n";
    return true;
}

bool DashSystem::canDash(const Entity* entity) const {
    if (!entity || !entity->isAlive()) return false;
    if (entity->isStunned()) return false;
    
    auto it = mDashStates.find(entity);
    if (it != mDashStates.end()) {
        if (it->second.isDashing || it->second.cooldownTimer > 0.f) {
            return false;
        }
    }
    return true;
}

bool DashSystem::isDashing(const Entity* entity) const {
    if (!entity) return false;
    auto it = mDashStates.find(entity);
    if (it != mDashStates.end()) {
        return it->second.isDashing;
    }
    return false;
}

bool DashSystem::isInvulnerable(const Entity* entity) const {
    return isDashing(entity);
}

float DashSystem::getCooldownRemaining(const Entity* entity) const {
    if (!entity) return 0.f;
    auto it = mDashStates.find(entity);
    if (it != mDashStates.end()) {
        return it->second.cooldownTimer;
    }
    return 0.f;
}

bool DashSystem::triggerDash(Entity* entity, sf::Vector2f moveDir, ParticleSystem* particles, const ChunkedTileMap* map, TerrainDeformSystem* terrain, SoundSystem* sound) {
    if (!canDash(entity)) return false;

    // Normalizar dirección
    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.001f) {
        moveDir /= len;
    } else {
        // Dirección predeterminada basada en hacia dónde mira la entidad
        moveDir = sf::Vector2f(static_cast<float>(entity->getFacingDir()), 0.f);
    }

    DashState& state = mDashStates[entity];
    state.isDashing = true;
    state.dashTimer = mDashDuration;
    state.cooldownTimer = mDashDuration + mDashCooldown;
    state.dashDir = moveDir;
    state.particleEmitTimer = 0.f;

    // Sonido
    if (sound) {
        sound->playSound(mSoundPath, mSoundVolume);
    } else if (auto* ss = SoundSystem::getInstance()) {
        ss->playSound(mSoundPath, mSoundVolume);
    }

    // Emisión inicial de partículas en el suelo
    if (particles && map) {
        sf::Vector2f footPos = entity->getPosition();
        sf::Color groundCol = map->getColorAtWorldPos(footPos);
        if (terrain) {
            groundCol = terrain->getDeformedColorAt(footPos, groundCol);
        }
        particles->emitWalkDust(footPos, groundCol, entity);
    }

    std::cout << "[DashSystem] Dash iniciado en direccion (" << moveDir.x << ", " << moveDir.y << ")\n";
    return true;
}

void DashSystem::update(sf::Time dt, ParticleSystem* particles, const ChunkedTileMap* map, TerrainDeformSystem* terrain) {
    float s = dt.asSeconds();

    for (auto it = mDashStates.begin(); it != mDashStates.end(); ) {
        DashState& state = it->second;
        const Entity* entityKey = it->first;
        Entity* entity = const_cast<Entity*>(entityKey);

        if (!entity || !entity->isAlive()) {
            it = mDashStates.erase(it);
            continue;
        }

        if (state.cooldownTimer > 0.f) {
            state.cooldownTimer -= s;
        }

        if (state.isDashing) {
            state.dashTimer -= s;
            state.particleEmitTimer -= s;

            // Mover la entidad a velocidad elevada
            sf::Vector2f movement = state.dashDir * mDashSpeed * s;
            entity->setPosition(entity->getPosition() + movement);

            // Emitir ráfaga continua de polvo/aire en el suelo durante el dash
            if (state.particleEmitTimer <= 0.f) {
                state.particleEmitTimer = mParticleEmitInterval;
                if (particles && map) {
                    sf::Vector2f footPos = entity->getPosition();
                    sf::Color groundCol = map->getColorAtWorldPos(footPos);
                    if (terrain) {
                        groundCol = terrain->getDeformedColorAt(footPos, groundCol);
                    }
                    particles->emitWalkDust(footPos, groundCol, entity);
                }
            }

            if (state.dashTimer <= 0.f) {
                state.isDashing = false;
            }
        } else if (state.cooldownTimer <= 0.f) {
            it = mDashStates.erase(it);
            continue;
        }

        ++it;
    }
}

void DashSystem::reset() {
    mDashStates.clear();
}
