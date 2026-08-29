#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

class Player;
class TargetingSystem;
class ProjectileSystem;
class ParticleSystem;
class DamageNumberSystem;
class Monster;
class PassiveMob;
class EnemyMob;
class WaterMonster;

class SpellSystem {
public:
    SpellSystem();
    ~SpellSystem();

    void Update(float deltaTime, Player& player, ParticleSystem& particles);

    bool CastBloodBurst(Player& player, 
                        std::vector<std::unique_ptr<Monster>>& monsters,
                        std::vector<std::unique_ptr<PassiveMob>>& passiveMobs,
                        std::vector<std::unique_ptr<EnemyMob>>& enemyMobs,
                        std::vector<std::unique_ptr<WaterMonster>>& waterMonsters,
                        ParticleSystem& particles, 
                        DamageNumberSystem& damageNumbers);

    bool CastShadowAegis(Player& player, ParticleSystem& particles);

    bool CastArcaneBeam(Player& player, 
                        TargetingSystem& targeting, 
                        ProjectileSystem& projectiles, 
                        ParticleSystem& particles);

    void RenderHUDSpells(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO);

    bool IsShadowAegisActive() const { return m_shadowAegisTimer > 0.0f; }

    float GetQCooldown() const { return m_cdBloodBurst; }
    float GetECooldown() const { return m_cdShadowAegis; }
    float GetRCooldown() const { return m_cdArcaneBeam; }

private:
    float m_cdBloodBurst = 0.0f;
    float m_maxCdBloodBurst = 4.5f;

    float m_cdShadowAegis = 0.0f;
    float m_maxCdShadowAegis = 8.0f;
    float m_shadowAegisTimer = 0.0f;

    float m_cdArcaneBeam = 0.0f;
    float m_maxCdArcaneBeam = 3.0f;
};
