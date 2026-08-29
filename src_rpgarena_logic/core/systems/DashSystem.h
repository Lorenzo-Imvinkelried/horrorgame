#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>

class Entity;
class ParticleSystem;
class TerrainDeformSystem;
class SoundSystem;
class ChunkedTileMap;

struct DashState {
    bool isDashing = false;
    float dashTimer = 0.f;        // Duración restante del dash activo
    float cooldownTimer = 0.f;    // Cooldown restante
    sf::Vector2f dashDir{0.f, 0.f};
    float particleEmitTimer = 0.f; // Temporizador para emitir polvo en los pies
};

class DashSystem {
public:
    DashSystem();
    ~DashSystem();

    static DashSystem* getInstance() { return sInstance; }

    bool canDash(const Entity* entity) const;
    bool isDashing(const Entity* entity) const;
    bool isInvulnerable(const Entity* entity) const;
    float getCooldownRemaining(const Entity* entity) const;

    bool triggerDash(Entity* entity, sf::Vector2f moveDir, ParticleSystem* particles = nullptr, const ChunkedTileMap* map = nullptr, TerrainDeformSystem* terrain = nullptr, SoundSystem* sound = nullptr);
    
    void update(sf::Time dt, ParticleSystem* particles = nullptr, const ChunkedTileMap* map = nullptr, TerrainDeformSystem* terrain = nullptr);

    bool loadConfig(const std::string& path = "assets/data/dash.json");

    void reset();

private:
    static DashSystem* sInstance;

    std::unordered_map<const Entity*, DashState> mDashStates;

    float mDashDuration = 0.20f;         // Duración activa en segundos (invulnerabilidad + velocidad)
    float mDashCooldown = 1.00f;         // Tiempo de recarga tras finalizar el dash
    float mDashSpeed = 1150.f;           // Píxeles por segundo durante el impulso
    float mParticleEmitInterval = 0.04f; // Intervalo para emisión de partículas
    std::string mSoundPath = "assets/sounds/player/charge.wav";
    float mSoundVolume = 90.0f;
};
