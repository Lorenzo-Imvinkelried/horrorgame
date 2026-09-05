#include "EnemyMob.h"
#include "WorldGenerator.h"
#include "Player.h"
#include "world/StructureSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

void EnemyMob::pickWanderTarget() {
    float wanderRadius = (m_type == EnemyType::NEUTRAL_GIANT || m_type == EnemyType::TREANT) ? 35.0f : 20.0f;
    float angle = (float)(rand() % 360) * 0.01745f;
    float dist = 4.0f + (float)(rand() % (int)wanderRadius);
    m_targetPos.x = m_spawnOrigin.x + cos(angle) * dist;
    m_targetPos.z = m_spawnOrigin.z + sin(angle) * dist;
    float terrainY = WorldGenerator::GetHeight(m_targetPos.x, m_targetPos.z);
    m_targetPos.y = StructureSystem::GetWalkableHeight(m_targetPos.x, m_targetPos.z, m_pos.y, terrainY);
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

    // Rango de detección cuerpo a cuerpo reducido (antes 28.0m / 38.0m) para evitar que vengan desde muy lejos
    float aggroDist = 14.0f;
    float loseDist = 22.0f;
    if (m_type == EnemyType::SHADOW_ASSASSIN || m_type == EnemyType::VAMPIRE) {
        aggroDist = 15.0f;
        loseDist = 23.5f;
    } else if (m_type == EnemyType::DEATH_KNIGHT) {
        aggroDist = 13.0f;
        loseDist = 21.0f;
    }

    if (distToPlayer < aggroDist && m_state != EnemyState::CHASE) {
        glm::vec3 mobEyes = m_pos + glm::vec3(0.0f, 1.4f * m_scale, 0.0f);
        glm::vec3 playerChest = playerPos - glm::vec3(0.0f, 0.4f, 0.0f);
        if (StructureSystem::HasLineOfSight(mobEyes, playerChest)) {
            m_state = EnemyState::CHASE;
            m_stateTimer = 4.0f;
        }
    }

    if (m_state == EnemyState::CHASE) {
        glm::vec3 mobEyes = m_pos + glm::vec3(0.0f, 1.4f * m_scale, 0.0f);
        glm::vec3 playerChest = playerPos - glm::vec3(0.0f, 0.4f, 0.0f);
        if (!StructureSystem::HasLineOfSight(mobEyes, playerChest)) {
            m_stateTimer -= deltaTime;
            if (m_stateTimer <= 0.0f || distToPlayer > loseDist) {
                m_state = EnemyState::IDLE;
                m_stateTimer = 3.0f;
                m_speed = 0.0f;
            }
        } else {
            m_stateTimer = 4.0f;
            if (distToPlayer > loseDist) {
                m_state = EnemyState::IDLE;
                m_stateTimer = 3.0f;
                m_speed = 0.0f;
            }
        }
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
        
        // Altura del suelo y alcance en pendiente
        float playerFeetY = playerPos.y - 1.6f; // playerPos está a los ojos (+1.6m sobre el suelo)
        float groundDiff = playerFeetY - m_pos.y;
        glm::vec3 mobChest = m_pos + glm::vec3(0.0f, 1.0f * m_scale, 0.0f);
        glm::vec3 playerChest = playerPos - glm::vec3(0.0f, 0.6f, 0.0f);
        float distBody = glm::distance(mobChest, playerChest);
        bool isClimbingTree = (player != nullptr && player->IsClimbing);

        // Alcanzable en pendiente natural (hasta 2.6m de elevacion de terreno y 3.8m de distancia de cuerpo).
        // Si el jugador esta trepando un arbol o en la copa (>2.8m), no lo puede alcanzar.
        bool playerReachable = !isClimbingTree && (groundDiff <= 2.6f && groundDiff >= -3.0f && distBody <= 3.8f);

        if (d2D > 2.4f && m_attackAnimProgress <= 0.0f) {
            m_pos.x += toP.x * m_speed * deltaTime;
            m_pos.z += toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 9.0f;
        } else {
            m_speed = 0.0f;

            // Iniciar animación de ataque solo si el jugador está al alcance cuerpo a cuerpo (no arriba de un árbol)
            if (m_attackCooldown <= 0.0f && m_attackAnimProgress <= 0.0f && player != nullptr && playerReachable) {
                m_attackAnimProgress = 0.01f;
            }

            // Progreso de animación de ataque (Windup -> Tajo descendente -> Retorno)
            if (m_attackAnimProgress > 0.0f) {
                float prevProgress = m_attackAnimProgress;
                m_attackAnimProgress += deltaTime * 2.2f;

                // En el clímax del tajo (progreso 50%), asesta el impacto y daño solo si sigue al alcance vertical
                if (prevProgress < 0.50f && m_attackAnimProgress >= 0.50f && player != nullptr && playerReachable) {
                    int baseDmg = 34;
                    float cd = 1.3f;

                    if (m_type == EnemyType::BERSERKER_WARRIOR) {
                        baseDmg = 46 + (rand() % 14);
                        cd = 0.95f;
                    } else if (m_type == EnemyType::DEATH_KNIGHT) {
                        baseDmg = 65 + (rand() % 18);
                        cd = 1.8f;
                    } else if (m_type == EnemyType::SHADOW_ASSASSIN) {
                        baseDmg = 35 + (rand() % 10);
                        cd = 0.85f;
                    } else if (m_type == EnemyType::VAMPIRE) {
                        baseDmg = 40 + (rand() % 12);
                        cd = 1.2f;
                    } else {
                        baseDmg = 34 + (rand() % 10);
                        cd = 1.3f;
                    }

                    int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.28f));
                    m_attackCooldown = cd;
                    player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                    if (m_type == EnemyType::VAMPIRE) {
                        m_currentHp = std::min(m_maxHp, m_currentHp + scaledDmg / 2);
                        damageNumbers.SpawnDamage(m_pos + glm::vec3(0, 2.0f, 0), scaledDmg / 2, true);
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

                if (m_attackAnimProgress >= 1.0f) {
                    m_attackAnimProgress = 0.0f;
                }
            }
        }
    } else {
        updateIdleWander(deltaTime);
    }
}

void EnemyMob::updateArcherAI(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, ProjectileSystem& projectiles) {
    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    glm::vec3 bowPos = m_pos + glm::vec3(0.24f, 1.2f, 0.2f);
    glm::vec3 playerChest = playerPos - glm::vec3(0.0f, 0.4f, 0.0f);
    bool hasLOS = StructureSystem::HasLineOfSight(bowPos, playerChest);

    if (distToPlayer < 30.0f && hasLOS) {
        glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
        float d2D = glm::length(toP);
        if (d2D > 0.001f) toP /= d2D;

        float targetYaw = atan2(toP.x, toP.y);
        float diff = targetYaw - m_yaw;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        m_yaw += diff * 5.0f * deltaTime;

        // Comportamiento de arquero: mantener distancia óptima (Kite Back si está demasiado cerca)
        if (distToPlayer < 8.0f) {
            m_speed = 3.5f;
            m_pos.x -= toP.x * m_speed * deltaTime;
            m_pos.z -= toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 7.0f;
        } else {
            m_speed = 0.0f;
        }

        if (m_attackCooldown <= 0.0f && distToPlayer <= 26.0f) {
            int archerDmg = (int)((28 + (rand() % 8)) * (1.0f + (m_nightLevel - 1) * 0.28f));
            projectiles.Spawn(bowPos, playerChest, 28.0f, archerDmg, glm::vec4(0.9f, 0.85f, 0.2f, 1.0f), false, ProjectileType::ARROW);

            for (int i = 0; i < 8; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/50.0f + 0.2f)*2.0f, (rand()%100/50.0f - 1.0f)*1.5f);
                particles.SpawnParticle(bowPos, pVel, glm::vec4(0.9f, 0.8f, 0.2f, 1.0f), 0.10f, 0.4f, 0.0f);
            }

            m_attackCooldown = 1.5f;
        }
    } else {
        updateIdleWander(deltaTime);
    }
}

void EnemyMob::updateMageAI(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, ProjectileSystem& projectiles) {
    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    glm::vec3 staffOrbPos = m_pos + glm::vec3(0.32f, 2.08f, 0.15f);
    glm::vec3 playerChest = playerPos - glm::vec3(0.0f, 0.4f, 0.0f);
    bool hasLOS = StructureSystem::HasLineOfSight(staffOrbPos, playerChest);

    if (distToPlayer < 32.0f && hasLOS) {
        glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
        float d2D = glm::length(toP);
        if (d2D > 0.001f) toP /= d2D;

        float targetYaw = atan2(toP.x, toP.y);
        float diff = targetYaw - m_yaw;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        m_yaw += diff * 4.5f * deltaTime;

        if (distToPlayer < 8.0f) {
            m_speed = 3.2f;
            m_pos.x -= toP.x * m_speed * deltaTime;
            m_pos.z -= toP.y * m_speed * deltaTime;
            m_animTimer += deltaTime * 6.0f;
        } else {
            m_speed = 0.0f;
        }

        if (m_attackCooldown <= 0.0f && distToPlayer <= 25.0f) {
            int mageDmg = (int)((34 + (rand() % 10)) * (1.0f + (m_nightLevel - 1) * 0.28f));
            projectiles.Spawn(staffOrbPos, playerChest, 13.5f, mageDmg, glm::vec4(0.95f, 0.15f, 0.90f, 1.0f));

            for (int i = 0; i < 14; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.0f, (rand()%100/50.0f + 0.2f)*2.5f, (rand()%100/50.0f - 1.0f)*2.0f);
                particles.SpawnParticle(staffOrbPos, pVel, glm::vec4(1.0f, 0.4f, 1.0f, 1.0f), 0.14f, 0.6f, 0.0f);
            }

            m_attackCooldown = 1.8f;
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

    float playerFeetY = playerPos.y - 1.6f;
    float groundDiff = playerFeetY - m_pos.y;
    glm::vec3 mobChest = m_pos + glm::vec3(0.0f, 2.0f, 0.0f);
    glm::vec3 playerChest = playerPos - glm::vec3(0.0f, 0.6f, 0.0f);
    float distBody = glm::distance(mobChest, playerChest);
    bool isClimbingTree = (player != nullptr && player->IsClimbing);
    bool playerReachable = !isClimbingTree && (groundDiff <= 3.8f && groundDiff >= -3.5f && distBody <= 4.8f);

    if (d2D > 2.8f && m_attackAnimProgress <= 0.0f) {
        m_pos.x += toP.x * m_speed * deltaTime;
        m_pos.z += toP.y * m_speed * deltaTime;
        m_animTimer += deltaTime * 4.0f;
    } else {
        m_speed = 0.0f;
        if (m_attackCooldown <= 0.0f && m_attackAnimProgress <= 0.0f && player != nullptr && playerReachable) {
            m_attackAnimProgress = 0.01f;
        }

        if (m_attackAnimProgress > 0.0f) {
            float prev = m_attackAnimProgress;
            m_attackAnimProgress += deltaTime * 1.6f;

            if (prev < 0.50f && m_attackAnimProgress >= 0.50f && player != nullptr && playerReachable) {
                m_attackCooldown = 1.8f;
                int baseDmg = 55 + (rand() % 15);
                int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.28f));
                player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                glm::vec3 slamPos = m_pos + glm::vec3(toP.x * 1.5f, 0.2f, toP.y * 1.5f);
                for (int i = 0; i < 30; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.4f)*3.8f, (rand()%100/50.0f - 1.0f)*3.5f);
                    particles.SpawnParticle(slamPos, pVel, glm::vec4(0.42f, 0.30f, 0.15f, 1.0f), 0.20f, 0.9f, -9.8f);
                }
            }

            if (m_attackAnimProgress >= 1.0f) {
                m_attackAnimProgress = 0.0f;
            }
        }
    }
}

void EnemyMob::updateGiantAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    if (!m_isEnraged) {
        updateIdleWander(deltaTime);
        return;
    }

    glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
    float d2D = glm::length(toP);
    if (d2D > 0.001f) toP /= d2D;

    float targetYaw = atan2(toP.x, toP.y);
    float diff = targetYaw - m_yaw;
    while (diff > 3.14159f) diff -= 6.28318f;
    while (diff < -3.14159f) diff += 6.28318f;
    m_yaw += diff * 3.5f * deltaTime;

    m_speed = 3.6f;

    float playerFeetY = playerPos.y - 1.6f;
    float groundDiff = playerFeetY - m_pos.y;
    glm::vec3 mobChest = m_pos + glm::vec3(0.0f, 2.8f, 0.0f);
    glm::vec3 playerChest = playerPos - glm::vec3(0.0f, 0.6f, 0.0f);
    float distBody = glm::distance(mobChest, playerChest);
    bool isClimbingTree = (player != nullptr && player->IsClimbing);
    bool playerReachable = !isClimbingTree && (groundDiff <= 5.0f && groundDiff >= -4.0f && distBody <= 6.2f);

    if (d2D > 3.2f && m_attackAnimProgress <= 0.0f) {
        m_pos.x += toP.x * m_speed * deltaTime;
        m_pos.z += toP.y * m_speed * deltaTime;
        m_animTimer += deltaTime * 5.0f;
    } else {
        m_speed = 0.0f;
        if (m_attackCooldown <= 0.0f && m_attackAnimProgress <= 0.0f && player != nullptr && playerReachable) {
            m_attackAnimProgress = 0.01f;
        }

        if (m_attackAnimProgress > 0.0f) {
            float prev = m_attackAnimProgress;
            m_attackAnimProgress += deltaTime * 1.5f;

            if (prev < 0.50f && m_attackAnimProgress >= 0.50f && player != nullptr && playerReachable) {
                m_attackCooldown = 2.0f;
                int baseDmg = 85 + (rand() % 25);
                int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.28f));
                player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                glm::vec3 smashPos = m_pos + glm::vec3(toP.x * 2.0f, 0.2f, toP.y * 2.0f);
                for (int i = 0; i < 35; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.4f)*4.5f, (rand()%100/50.0f - 1.0f)*4.0f);
                    particles.SpawnParticle(smashPos, pVel, glm::vec4(0.85f, 0.35f, 0.05f, 1.0f), 0.22f, 1.0f, -9.8f);
                }
            }

            if (m_attackAnimProgress >= 1.0f) {
                m_attackAnimProgress = 0.0f;
            }
        }
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
