#include "EnemyMob.h"
#include "WorldGenerator.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

void EnemyMob::pickWanderTarget() {
    float wanderRadius = (m_type == EnemyType::NEUTRAL_GIANT || m_type == EnemyType::TREANT) ? 35.0f : 20.0f;
    float angle = (float)(rand() % 360) * 0.01745f;
    float dist = 4.0f + (float)(rand() % (int)wanderRadius);
    m_targetPos.x = m_spawnOrigin.x + cos(angle) * dist;
    m_targetPos.z = m_spawnOrigin.z + sin(angle) * dist;
    m_targetPos.y = WorldGenerator::GetHeight(m_targetPos.x, m_targetPos.z);
}

void EnemyMob::updateAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers, ProjectileSystem& projectiles) {
    switch (m_type) {
        case EnemyType::CORRUPTED_WARRIOR:
        case EnemyType::BERSERKER_WARRIOR:
        case EnemyType::DEATH_KNIGHT:
        case EnemyType::SHADOW_ASSASSIN:
        case EnemyType::VAMPIRE:
            updateMeleeAI(deltaTime, playerPos, player, particles, damageNumbers);
            break;

        case EnemyType::SKELETON_ARCHER:
            updateArcherAI(deltaTime, playerPos, particles, projectiles);
            break;

        case EnemyType::DARK_MAGE:
            updateMageAI(deltaTime, playerPos, particles, projectiles);
            break;

        case EnemyType::TREANT:
            updateTreantAI(deltaTime, playerPos, player, particles, damageNumbers);
            break;

        case EnemyType::NEUTRAL_GIANT:
            updateGiantAI(deltaTime, playerPos, player, particles, damageNumbers);
            break;
    }
}

void EnemyMob::updateMeleeAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    if (distToPlayer < 28.0f && m_state != EnemyState::CHASE) {
        m_state = EnemyState::CHASE;
    }

    if (m_state == EnemyState::CHASE && distToPlayer > 38.0f) {
        m_state = EnemyState::IDLE;
        m_stateTimer = 3.0f;
        m_speed = 0.0f;
    }

    if (m_state == EnemyState::CHASE) {
        glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
        float d2D = glm::length(toP);
        if (d2D > 0.001f) toP /= d2D;

        float targetYaw = atan2(toP.x, toP.y);
        float diff = targetYaw - m_yaw;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        m_yaw += diff * 6.5f * deltaTime;

        // Velocidad según clase de guerrero
        if (m_type == EnemyType::SHADOW_ASSASSIN) m_speed = 7.2f;
        else if (m_type == EnemyType::VAMPIRE) m_speed = 7.5f;
        else if (m_type == EnemyType::BERSERKER_WARRIOR) m_speed = 6.4f;
        else if (m_type == EnemyType::CORRUPTED_WARRIOR) m_speed = 5.4f;
        else m_speed = 4.2f; // Death Knight

        if (d2D > 2.2f) {
            m_pos.x += toP.x * m_speed * deltaTime;
            m_pos.z += toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 9.0f;
        } else {
            m_speed = 0.0f;
            if (m_attackCooldown <= 0.0f && player != nullptr) {
                int baseDmg = 20;
                float cd = 1.5f;

                if (m_type == EnemyType::BERSERKER_WARRIOR) {
                    baseDmg = 26 + (rand() % 9);
                    cd = 1.1f;
                } else if (m_type == EnemyType::DEATH_KNIGHT) {
                    baseDmg = 36 + (rand() % 12);
                    cd = 2.2f;
                } else if (m_type == EnemyType::SHADOW_ASSASSIN) {
                    baseDmg = 18 + (rand() % 7);
                    cd = 1.0f;
                } else if (m_type == EnemyType::VAMPIRE) {
                    baseDmg = 24 + (rand() % 8);
                    cd = 1.4f;
                } else {
                    baseDmg = 20 + (rand() % 6);
                    cd = 1.6f;
                }

                int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.25f));
                m_attackCooldown = cd;
                player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                if (m_type == EnemyType::VAMPIRE) {
                    m_currentHp = std::min(m_maxHp, m_currentHp + scaledDmg);
                    damageNumbers.SpawnDamage(m_pos + glm::vec3(0, 2.0f, 0), scaledDmg, true);
                }

                glm::vec3 hitPos = playerPos + glm::vec3(0.0f, 1.2f, 0.0f);
                glm::vec4 sparkCol = glm::vec4(0.85f, 0.05f, 0.05f, 1.0f);
                if (m_type == EnemyType::DEATH_KNIGHT) sparkCol = glm::vec4(0.2f, 0.75f, 1.0f, 1.0f);
                else if (m_type == EnemyType::SHADOW_ASSASSIN) sparkCol = glm::vec4(0.2f, 0.95f, 0.3f, 1.0f);

                for (int i = 0; i < 20; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.2f, (rand()%100/50.0f + 0.3f)*3.5f, (rand()%100/50.0f - 1.0f)*3.2f);
                    particles.SpawnParticle(hitPos, pVel, sparkCol, 0.14f, 0.8f, -9.8f);
                }
            }
        }
    } else {
        updateIdleWander(deltaTime);
    }
}

void EnemyMob::updateArcherAI(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, ProjectileSystem& projectiles) {
    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    if (distToPlayer < 30.0f) {
        glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
        float d2D = glm::length(toP);
        if (d2D > 0.001f) toP /= d2D;

        float targetYaw = atan2(toP.x, toP.y);
        float diff = targetYaw - m_yaw;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        m_yaw += diff * 5.0f * deltaTime;

        // Kiting: retrocede si el jugador está demasiado cerca
        if (distToPlayer < 8.0f) {
            m_speed = 4.2f;
            m_pos.x -= toP.x * m_speed * deltaTime;
            m_pos.z -= toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 7.5f;
        } else if (distToPlayer > 22.0f) {
            m_speed = 3.6f;
            m_pos.x += toP.x * m_speed * deltaTime;
            m_pos.z += toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 6.0f;
        } else {
            m_speed = 0.0f;
        }

        // Disparo de flecha física
        if (m_attackCooldown <= 0.0f && distToPlayer <= 26.0f) {
            m_attackCooldown = 2.0f;
            glm::vec3 bowPos = m_pos + glm::vec3(sin(m_yaw) * 0.4f, 1.25f, cos(m_yaw) * 0.4f);
            glm::vec3 aimTarget = playerPos + glm::vec3(0.0f, 1.0f, 0.0f);

            int arrowDmg = (int)(18 * (1.0f + (m_nightLevel - 1) * 0.25f));
            projectiles.Spawn(bowPos, aimTarget, 22.0f, arrowDmg, glm::vec4(0.92f, 0.85f, 0.60f, 1.0f));

            for (int i = 0; i < 12; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/50.0f + 0.2f)*2.0f, (rand()%100/50.0f - 1.0f)*1.5f);
                particles.SpawnParticle(bowPos, pVel, glm::vec4(0.9f, 0.9f, 0.7f, 0.8f), 0.12f, 0.45f, -4.0f);
            }
        }
    } else {
        updateIdleWander(deltaTime);
    }
}

void EnemyMob::updateMageAI(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, ProjectileSystem& projectiles) {
    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    if (distToPlayer < 24.0f) {
        glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
        float d2D = glm::length(toP);
        if (d2D > 0.001f) toP /= d2D;

        float targetYaw = atan2(toP.x, toP.y);
        float diff = targetYaw - m_yaw;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        m_yaw += diff * 4.0f * deltaTime;

        if (distToPlayer < 8.0f) {
            m_speed = 3.2f;
            m_pos.x -= toP.x * m_speed * deltaTime;
            m_pos.z -= toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 6.0f;
        } else {
            m_speed = 0.0f;
        }

        if (m_attackCooldown <= 0.0f && distToPlayer <= 25.0f) {
            glm::vec3 staffOrbPos = m_pos + glm::vec3(0.32f, 2.08f, 0.15f);
            glm::vec3 playerChest = playerPos + glm::vec3(0.0f, 1.1f, 0.0f);

            int mageDmg = (int)(16 * (1.0f + (m_nightLevel - 1) * 0.25f));
            projectiles.Spawn(staffOrbPos, playerChest, 12.0f, mageDmg, glm::vec4(0.95f, 0.15f, 0.90f, 1.0f));

            for (int i = 0; i < 14; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.0f, (rand()%100/50.0f + 0.2f)*2.5f, (rand()%100/50.0f - 1.0f)*2.0f);
                particles.SpawnParticle(staffOrbPos, pVel, glm::vec4(1.0f, 0.4f, 1.0f, 1.0f), 0.14f, 0.6f, 0.0f);
            }

            m_attackCooldown = 2.4f;
        }
    } else {
        updateIdleWander(deltaTime);
    }
}

void EnemyMob::updateTreantAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    if (!m_isAwakened) {
        if (distToPlayer < 7.5f) {
            m_isAwakened = true;
            for (int i = 0; i < 28; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.4f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
                particles.SpawnParticle(m_pos + glm::vec3(0, 0.4f, 0), pVel, glm::vec4(0.38f, 0.28f, 0.12f, 1.0f), 0.18f, 0.8f, -9.8f);
            }
        }
        m_speed = 0.0f;
        return;
    }

    glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
    float d2D = glm::length(toP);
    if (d2D > 0.001f) toP /= d2D;

    float targetYaw = atan2(toP.x, toP.y);
    float diff = targetYaw - m_yaw;
    while (diff > 3.14159f) diff -= 6.28318f;
    while (diff < -3.14159f) diff += 6.28318f;
    m_yaw += diff * 2.5f * deltaTime;

    m_speed = 2.4f;

    if (d2D > 2.8f) {
        m_pos.x += toP.x * m_speed * deltaTime;
        m_pos.z += toP.y * m_speed * deltaTime;
        m_animTimer += deltaTime * 4.0f;
    } else {
        m_speed = 0.0f;
        if (m_attackCooldown <= 0.0f && player != nullptr) {
            m_attackCooldown = 2.2f;
            int baseDmg = 32 + (rand() % 10);
            int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.25f));
            player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

            glm::vec3 slamPos = m_pos + glm::vec3(toP.x * 1.5f, 0.2f, toP.y * 1.5f);
            for (int i = 0; i < 30; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.4f)*3.8f, (rand()%100/50.0f - 1.0f)*3.5f);
                particles.SpawnParticle(slamPos, pVel, glm::vec4(0.42f, 0.30f, 0.15f, 1.0f), 0.20f, 0.9f, -9.8f);
            }
        }
    }
}

void EnemyMob::updateGiantAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    if (m_isEnraged) {
        glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
        float d2D = glm::length(toP);
        if (d2D > 0.001f) toP /= d2D;

        float targetYaw = atan2(toP.x, toP.y);
        float diff = targetYaw - m_yaw;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        m_yaw += diff * 3.0f * deltaTime;

        m_speed = 3.8f;

        if (d2D > 3.4f) {
            m_pos.x += toP.x * m_speed * deltaTime;
            m_pos.z += toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 5.0f;
        } else {
            m_speed = 0.0f;
            if (m_attackCooldown <= 0.0f && player != nullptr) {
                m_attackCooldown = 2.4f;
                int baseDmg = 35 + (rand() % 12);
                int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.25f));
                player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                glm::vec3 slamPos = m_pos + glm::vec3(toP.x * 2.0f, 0.2f, toP.y * 2.0f);
                for (int i = 0; i < 35; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.5f)*4.5f, (rand()%100/50.0f - 1.0f)*4.0f);
                    particles.SpawnParticle(slamPos, pVel, glm::vec4(0.55f, 0.45f, 0.30f, 1.0f), 0.22f, 1.0f, -9.8f);
                }
            }
        }
    } else {
        updateIdleWander(deltaTime);
    }
}

void EnemyMob::updateIdleWander(float deltaTime) {
    switch (m_state) {
        case EnemyState::IDLE: {
            m_speed = 0.0f;
            m_stateTimer -= deltaTime;
            m_animTimer += deltaTime;
            if (m_stateTimer <= 0.0f) {
                pickWanderTarget();
                m_state = EnemyState::WANDER;
            }
            break;
        }

        case EnemyState::WANDER: {
            glm::vec2 toTarget(m_targetPos.x - m_pos.x, m_targetPos.z - m_pos.z);
            float dist = glm::length(toTarget);

            if (dist < 1.5f) {
                m_state = EnemyState::IDLE;
                m_stateTimer = 3.0f + (rand() % 100) * 0.04f;
            } else {
                glm::vec2 moveDir = glm::normalize(toTarget);
                float targetYaw = atan2(moveDir.x, moveDir.y);

                float diff = targetYaw - m_yaw;
                while (diff > 3.14159f) diff -= 6.28318f;
                while (diff < -3.14159f) diff += 6.28318f;
                m_yaw += diff * 3.5f * deltaTime;

                m_speed = (m_type == EnemyType::NEUTRAL_GIANT || m_type == EnemyType::TREANT) ? 1.0f : 1.3f;
                m_pos.x += sin(m_yaw) * m_speed * deltaTime;
                m_pos.z += cos(m_yaw) * m_speed * deltaTime;
                m_animTimer += deltaTime * 5.0f;
            }
            break;
        }

        default:
            break;
    }
}
