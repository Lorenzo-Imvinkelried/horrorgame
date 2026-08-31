#include "SpellSystem.h"
#include "Player.h"
#include "TargetingSystem.h"
#include "ProjectileSystem.h"
#include "ParticleSystem.h"
#include "DamageNumberSystem.h"
#include "Monster.h"
#include "PassiveMob.h"
#include "EnemyMob.h"
#include "WaterMonster.h"
#include "entities/Dragon.h"
#include "ui/UIRenderer.h"
#include <cmath>
#include <iostream>

SpellSystem::SpellSystem() {}
SpellSystem::~SpellSystem() {}

void SpellSystem::Update(float deltaTime, Player& player, ParticleSystem& particles) {
    if (m_cdBloodBurst > 0.0f) m_cdBloodBurst -= deltaTime;
    if (m_cdShadowAegis > 0.0f) m_cdShadowAegis -= deltaTime;
    if (m_cdArcaneBeam > 0.0f) m_cdArcaneBeam -= deltaTime;

    // Shadow Aegis Protective Aura Effect
    if (m_shadowAegisTimer > 0.0f) {
        m_shadowAegisTimer -= deltaTime;
        if ((rand() % 100) < 35) {
            float angle = (float)(rand() % 360) * 0.01745f;
            float radius = 0.9f;
            glm::vec3 orbPos = player.Position + glm::vec3(cos(angle) * radius, 0.4f + (rand() % 100 / 100.0f) * 1.2f, sin(angle) * radius);
            particles.SpawnParticle(orbPos, glm::vec3(0.0f, 0.6f, 0.0f), glm::vec4(0.45f, 0.15f, 0.75f, 0.9f), 0.16f, 0.6f, 0.0f);
        }
    }
}

bool SpellSystem::CastBloodBurst(Player& player, 
                                std::vector<std::unique_ptr<Monster>>& monsters,
                                std::vector<std::unique_ptr<PassiveMob>>& passiveMobs,
                                std::vector<std::unique_ptr<EnemyMob>>& enemyMobs,
                                std::vector<std::unique_ptr<WaterMonster>>& waterMonsters,
                                ParticleSystem& particles, 
                                DamageNumberSystem& damageNumbers,
                                Dragon* dragon) 
{
    const int manaCost = 25;
    if (m_cdBloodBurst > 0.0f || player.Stats.CurrentMP < manaCost || player.StunTimer > 0.0f) return false;

    player.Stats.CurrentMP -= manaCost;
    m_cdBloodBurst = m_maxCdBloodBurst;

    // Shockwave Ring Particle FX
    for (int i = 0; i < 48; ++i) {
        float angle = (float)i * 0.1308f;
        glm::vec3 pVel(cos(angle) * 7.5f, 0.4f + (rand() % 100 / 100.0f) * 1.2f, sin(angle) * 7.5f);
        particles.SpawnParticle(player.Position + glm::vec3(0, 0.3f, 0), pVel, glm::vec4(0.85f, 0.08f, 0.08f, 1.0f), 0.22f, 0.85f, -4.0f);
    }

    int spellDamage = 34 + player.Stats.Strength / 2 + player.Stats.Intelligence / 3;
    float aoeRadius = 8.5f;

    // Damage dragon if in AoE range
    if (dragon != nullptr && dragon->IsAlive() && !dragon->IsDying()) {
        float d = glm::distance(player.Position, dragon->GetPosition());
        if (d < (aoeRadius + dragon->GetRadius())) {
            bool killed = dragon->TakeDamage(spellDamage, player.Position, particles, damageNumbers, &player);
            damageNumbers.SpawnDamage(dragon->GetPosition() + glm::vec3(0, 2.5f, 0), spellDamage, true);
            if (killed) {
                bool lvlUp = false;
                player.Stats.AddExp(dragon->GetExpReward(), lvlUp);
                damageNumbers.SpawnExp(dragon->GetPosition() + glm::vec3(0, 3.0f, 0), dragon->GetExpReward());
                if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);
            }
        }
    }

    // Damage all nearby monsters
    for (auto& mPtr : monsters) {
        if (!mPtr->IsDead()) {
            float d = glm::distance(player.Position, mPtr->GetPosition());
            if (d < aoeRadius) {
                mPtr->TakeDamage((float)spellDamage, false);
                damageNumbers.SpawnDamage(mPtr->GetPosition() + glm::vec3(0, 1.4f, 0), spellDamage, true);
                if (mPtr->IsDead()) {
                    bool lvlUp = false;
                    player.Stats.AddExp(120, lvlUp);
                    damageNumbers.SpawnExp(mPtr->GetPosition() + glm::vec3(0, 1.8f, 0), 120);
                    if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);
                }
            }
        }
    }

    // Damage all nearby passive & demonic deer
    for (auto& deer : passiveMobs) {
        if (deer->IsAlive()) {
            float d = glm::distance(player.Position, deer->GetPosition());
            if (d < aoeRadius) {
                deer->TakeDamage(spellDamage, player.Position, particles, &player, damageNumbers);
                if (!deer->IsAlive()) {
                    bool lvlUp = false;
                    player.Stats.AddExp(deer->GetExpReward(), lvlUp);
                    damageNumbers.SpawnExp(deer->GetPosition() + glm::vec3(0, 1.6f, 0), deer->GetExpReward());
                    if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);
                }
            }
        }
    }

    // Damage all nearby enemy mobs (Warriors, Giants, Mages)
    for (auto& enemy : enemyMobs) {
        if (enemy->IsAlive()) {
            float d = glm::distance(player.Position, enemy->GetPosition());
            if (d < aoeRadius) {
                enemy->TakeDamage(spellDamage, player.Position, particles, &player, damageNumbers);
                if (!enemy->IsAlive()) {
                    bool lvlUp = false;
                    player.Stats.AddExp(enemy->GetExpReward(), lvlUp);
                    damageNumbers.SpawnExp(enemy->GetPosition() + glm::vec3(0, 1.8f, 0), enemy->GetExpReward());
                    if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);
                }
            }
        }
    }

    // Damage water monsters
    for (auto& wm : waterMonsters) {
        if (wm->IsAlive()) {
            float d = glm::distance(player.Position, wm->GetPosition());
            if (d < aoeRadius) {
                wm->TakeDamage(spellDamage, player.Position, particles, &player, damageNumbers);
                if (!wm->IsAlive()) {
                    bool lvlUp = false;
                    player.Stats.AddExp(wm->GetExpReward(), lvlUp);
                    damageNumbers.SpawnExp(wm->GetPosition() + glm::vec3(0, 1.6f, 0), wm->GetExpReward());
                    if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);
                }
            }
        }
    }

    return true;
}

bool SpellSystem::CastShadowAegis(Player& player, ParticleSystem& particles) {
    const int manaCost = 20;
    if (m_cdShadowAegis > 0.0f || player.Stats.CurrentMP < manaCost || player.StunTimer > 0.0f) return false;

    player.Stats.CurrentMP -= manaCost;
    m_cdShadowAegis = m_maxCdShadowAegis;
    m_shadowAegisTimer = 5.0f;

    // Burst of shadow aura
    for (int i = 0; i < 30; ++i) {
        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.8f, (rand()%100/50.0f + 0.5f)*3.0f, (rand()%100/50.0f - 1.0f)*2.8f);
        particles.SpawnParticle(player.Position + glm::vec3(0, 1.0f, 0), pVel, glm::vec4(0.40f, 0.12f, 0.70f, 1.0f), 0.20f, 1.0f, -6.0f);
    }

    return true;
}

bool SpellSystem::CastArcaneBeam(Player& player, 
                                TargetingSystem& targeting, 
                                ProjectileSystem& projectiles, 
                                ParticleSystem& particles) 
{
    const int manaCost = 30;
    if (m_cdArcaneBeam > 0.0f || player.Stats.CurrentMP < manaCost || player.StunTimer > 0.0f) return false;

    player.Stats.CurrentMP -= manaCost;
    m_cdArcaneBeam = m_maxCdArcaneBeam;

    glm::vec3 spawnPos = player.Position + glm::vec3(0.0f, 1.3f, 0.0f);
    glm::vec3 targetPos = spawnPos + player.Front * 35.0f;

    if (targeting.HasTarget()) {
        targetPos = targeting.GetTargetPosition() + glm::vec3(0.0f, 1.2f, 0.0f);
    }

    int beamDamage = 46 + player.Stats.Intelligence * 2;

    projectiles.Spawn(spawnPos, targetPos, 26.0f, beamDamage, glm::vec4(0.20f, 0.85f, 0.95f, 1.0f));

    // Muzzle flash particles
    for (int i = 0; i < 18; ++i) {
        glm::vec3 pVel = glm::normalize(targetPos - spawnPos) * 4.0f + glm::vec3((rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/50.0f - 1.0f)*1.5f);
        particles.SpawnParticle(spawnPos, pVel, glm::vec4(0.20f, 0.85f, 0.95f, 1.0f), 0.14f, 0.5f, 0.0f);
    }

    return true;
}

void SpellSystem::RenderHUDSpells(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO) {
    float startX = 0.38f;
    float startY = -0.92f;
    float btnW = 0.18f, btnH = 0.10f;
    float padX = 0.024f;

    auto drawSpellSlot = [&](int idx, const std::string& hotkey, const std::string& name, float cd, glm::vec3 col) {
        float sx = startX + idx * (btnW + padX);
        float sy = startY;

        UIRenderer::DrawWin98Button(uiProgram, uiVAO, uiVBO, sx, sy, btnW, btnH, "", false, 0.022f);

        // Icon hotkey banner
        UIRenderer::DrawString(hotkey, sx + 0.012f, sy + btnH - 0.038f, 0.024f, col, uiProgram, uiVAO, uiVBO);
        UIRenderer::DrawString(name, sx + 0.012f, sy + 0.016f, 0.018f, glm::vec3(0.15f, 0.15f, 0.20f), uiProgram, uiVAO, uiVBO);

        // Cooldown Overlay
        if (cd > 0.0f) {
            std::string cdStr = std::to_string((int)ceil(cd)) + "s";
            UIRenderer::DrawString(cdStr, sx + btnW - 0.055f, sy + btnH - 0.038f, 0.024f, glm::vec3(0.85f, 0.15f, 0.10f), uiProgram, uiVAO, uiVBO);
        }
    };

    drawSpellSlot(0, "[Q]", "ONDA SANGRE", m_cdBloodBurst, glm::vec3(0.85f, 0.15f, 0.15f));
    drawSpellSlot(1, "[E]", "ESCUDO SOMBRA", m_cdShadowAegis, glm::vec3(0.55f, 0.20f, 0.85f));
    drawSpellSlot(2, "[R]", "RAYO ARCANO", m_cdArcaneBeam, glm::vec3(0.15f, 0.65f, 0.95f));
}
