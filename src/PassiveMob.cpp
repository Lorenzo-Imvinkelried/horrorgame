#include "PassiveMob.h"
#include "WorldGenerator.h"
#include "Player.h"
#include "DamageNumberSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>

GLuint PassiveMob::s_hpBarVAO = 0;
GLuint PassiveMob::s_hpBarVBO = 0;

void PassiveMob::initHpBarMesh() {
    if (s_hpBarVAO != 0) return;

    // Billboard quad [-0.5, 0.5]
    float quadVertices[] = {
        // Pos             // Color           // UV         // Normal
        -0.5f, -0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &s_hpBarVAO);
    glGenBuffers(1, &s_hpBarVBO);
    glBindVertexArray(s_hpBarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_hpBarVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

PassiveMob::PassiveMob(glm::vec3 spawnPos, DeerSize size)
    : m_size(size)
    , m_scale(1.0f)
    , m_pos(spawnPos)
    , m_spawnOrigin(spawnPos)
    , m_targetPos(spawnPos)
    , m_fleeDir(0.0f, 0.0f, 1.0f)
    , m_yaw((float)(rand() % 360) * 0.01745f)
    , m_speed(0.0f)
    , m_animTimer((float)(rand() % 100) * 0.1f)
    , m_stateTimer(2.0f + (rand() % 100) * 0.03f)
    , m_attackCooldown(0.0f)
    , m_state(PassiveMobState::IDLE)
    , m_maxHp(60)
    , m_currentHp(60)
    , m_defense(4)
    , m_evasion(12)
    , m_hitFlashTimer(0.0f)
    , m_showHpBarTimer(0.0f)
    , m_deathTimer(0.0f)
    , m_headGrazeAngle(0.0f)
    , m_glowPulse(0.0f)
    , m_VAO(0)
    , m_VBO(0)
    , m_vertexCount(0)
{
    if (m_size == DeerSize::FAWN) {
        m_scale = 0.65f;
        m_maxHp = 30;
        m_defense = 1;
        m_evasion = 18;
    } else if (m_size == DeerSize::ADULT) {
        m_scale = 1.00f;
        m_maxHp = 60;
        m_defense = 4;
        m_evasion = 12;
    } else if (m_size == DeerSize::ALPHA) {
        m_scale = 1.40f;
        m_maxHp = 120;
        m_defense = 8;
        m_evasion = 8;
    } else if (m_size == DeerSize::DEMONIC) {
        m_scale = 1.30f;
        m_maxHp = 160;
        m_defense = 7;
        m_evasion = 6;
    }
    m_currentHp = m_maxHp;

    m_baseBoxes = ModelLoader::Load("assets/models/passive_deer.txt");

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    initHpBarMesh();
    updateModelMesh();
}

PassiveMob::~PassiveMob() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

std::string PassiveMob::GetName() const {
    if (m_size == DeerSize::FAWN) return "Cervatillo del Bosque";
    if (m_size == DeerSize::ALPHA) return "Gran Ciervo Alfa";
    if (m_size == DeerSize::DEMONIC) return "Ciervo Endemoniado";
    return "Venado del Bosque";
}

int PassiveMob::GetLevel() const {
    if (m_size == DeerSize::FAWN) return 1;
    if (m_size == DeerSize::ALPHA) return 4;
    if (m_size == DeerSize::DEMONIC) return 5;
    return 2;
}

int PassiveMob::GetExpReward() const {
    if (m_size == DeerSize::FAWN) return 25;
    if (m_size == DeerSize::ALPHA) return 90;
    if (m_size == DeerSize::DEMONIC) return 150;
    return 45;
}

void PassiveMob::pickNewWanderTarget() {
    float wanderRadius = (m_size == DeerSize::DEMONIC) ? 25.0f : 18.0f;
    float angle = (float)(rand() % 360) * 0.01745f;
    float dist = 4.0f + (rand() % 100) * 0.01f * wanderRadius;
    
    m_targetPos.x = m_spawnOrigin.x + cos(angle) * dist;
    m_targetPos.z = m_spawnOrigin.z + sin(angle) * dist;
    m_targetPos.y = WorldGenerator::GetHeight(m_targetPos.x, m_targetPos.z);
}

void PassiveMob::Update(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    if (m_hitFlashTimer > 0.0f) m_hitFlashTimer -= deltaTime;
    if (m_showHpBarTimer > 0.0f) m_showHpBarTimer -= deltaTime;
    if (m_attackCooldown > 0.0f) m_attackCooldown -= deltaTime;
    m_glowPulse += deltaTime * 4.5f;

    if (m_state == PassiveMobState::DEAD) {
        m_deathTimer += deltaTime;
        updateModelMesh();
        return;
    }

    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    // =========================================================================
    // 1. DEMONIC DEER: Continuous Hostile Aggression & Relentless Chasing
    // =========================================================================
    if (m_size == DeerSize::DEMONIC) {
        // De-aggro if player sprints far away (> 28m)
        if (m_state == PassiveMobState::CHASE && distToPlayer > 28.0f) {
            m_state = PassiveMobState::IDLE;
            m_stateTimer = 3.5f;
            m_speed = 0.0f;
        }
        // Detect and chase player when within awareness range (< 22m)
        else if (distToPlayer < 22.0f && m_state != PassiveMobState::CHASE) {
            m_state = PassiveMobState::CHASE;
        }

        if (m_state == PassiveMobState::CHASE) {
            glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
            float d2D = glm::length(toP);
            if (d2D > 0.001f) toP /= d2D;

            float targetYaw = atan2(toP.x, toP.y);

            // Fast smooth tracking
            float diff = targetYaw - m_yaw;
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            m_yaw += diff * 8.0f * deltaTime;

            // Maintain distance (1.65m) so mob stays in front of the player without merging inside!
            if (distToPlayer > 1.65f) {
                m_speed = 3.8f;
                m_pos.x += sin(m_yaw) * m_speed * deltaTime;
                m_pos.z += cos(m_yaw) * m_speed * deltaTime;
                m_animTimer += deltaTime * 12.0f;
                m_headGrazeAngle = -0.25f;
            } else {
                // In melee range: slow down and rear up to attack
                m_speed = 0.4f;
                m_animTimer += deltaTime * 16.0f;
                // Violent lunge / headbutt attack animation
                m_headGrazeAngle = -0.50f + sin(m_animTimer * 2.5f) * 0.40f;
            }

            // Attack player in melee range
            if (distToPlayer < 2.5f && m_attackCooldown <= 0.0f) {
                if (player != nullptr) {
                    player->TakeDamage(18, damageNumbers);
                }

                // Dark blood & necrotic miasma particles
                glm::vec3 hitPos = m_pos + glm::vec3(toP.x * 0.8f, 1.2f, toP.y * 0.8f);
                for (int i = 0; i < 20; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.3f)*3.5f, (rand()%100/50.0f - 1.0f)*3.5f);
                    particles.SpawnParticle(hitPos, pVel, glm::vec4(0.70f, 0.02f, 0.02f, 1.0f), 0.16f, 0.9f, -9.8f);
                    particles.SpawnParticle(hitPos, pVel * 0.6f, glm::vec4(0.08f, 0.02f, 0.10f, 1.0f), 0.18f, 1.1f, -4.0f);
                }

                m_attackCooldown = 1.6f; // Strike again shortly
            }

            m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
            updateModelMesh();
            return;
        }
    }

    // =========================================================================
    // 2. ALPHA DEER: Headbutt Counter & Reposition Flee
    // =========================================================================
    if (m_size == DeerSize::ALPHA && distToPlayer < 2.5f && m_attackCooldown <= 0.0f && m_state != PassiveMobState::DEAD) {
        // Face player
        glm::vec2 toP = glm::normalize(glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z));
        m_yaw = atan2(toP.x, toP.y);

        // Deliver Headbutt damage to player
        if (player != nullptr) {
            player->TakeDamage(16, damageNumbers);
        }

        // Spawn hit particles
        glm::vec3 hitPos = m_pos + glm::vec3(toP.x * 0.8f, 1.2f, toP.y * 0.8f);
        for (int i = 0; i < 14; ++i) {
            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.3f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
            particles.SpawnParticle(hitPos, pVel, glm::vec4(0.85f, 0.65f, 0.20f, 1.0f), 0.12f, 0.75f, -9.8f);
        }

        m_attackCooldown = 4.5f;

        // Immediately flee to reposition ("me pega y corre")
        m_state = PassiveMobState::FLEE;
        m_stateTimer = 2.6f;
        glm::vec2 away = -toP;
        m_fleeDir = glm::vec3(away.x, 0.0f, away.y);
    }
    // Fawn & Adult Deer: Startled reaction
    else if (distToPlayer < 2.5f && m_state != PassiveMobState::FLEE) {
        m_state = PassiveMobState::FLEE;
        m_stateTimer = 2.0f;
        glm::vec2 away = glm::normalize(glm::vec2(m_pos.x - playerPos.x, m_pos.z - playerPos.z));
        m_fleeDir = glm::vec3(away.x, 0.0f, away.y);
    }

    switch (m_state) {
        case PassiveMobState::IDLE: {
            m_speed = 0.0f;
            m_stateTimer -= deltaTime;
            
            // Grazing animation: Head lowers to eat grass
            m_headGrazeAngle = sin(m_animTimer * 1.5f) * 0.35f + 0.20f;
            m_animTimer += deltaTime;

            if (m_stateTimer <= 0.0f) {
                pickNewWanderTarget();
                m_state = PassiveMobState::WANDER;
            }
            break;
        }

        case PassiveMobState::WANDER: {
            m_headGrazeAngle = 0.0f;
            glm::vec2 toTarget(m_targetPos.x - m_pos.x, m_targetPos.z - m_pos.z);
            float dist = glm::length(toTarget);

            if (dist < 1.0f) {
                m_state = PassiveMobState::IDLE;
                m_stateTimer = 3.0f + (rand() % 100) * 0.04f;
            } else {
                glm::vec2 moveDir = glm::normalize(toTarget);
                float targetYaw = atan2(moveDir.x, moveDir.y);

                // Smooth rotation
                float diff = targetYaw - m_yaw;
                while (diff > 3.14159f) diff -= 6.28318f;
                while (diff < -3.14159f) diff += 6.28318f;
                m_yaw += diff * 4.0f * deltaTime;

                m_speed = (m_size == DeerSize::DEMONIC) ? 1.6f : 1.2f;
                m_pos.x += sin(m_yaw) * m_speed * deltaTime;
                m_pos.z += cos(m_yaw) * m_speed * deltaTime;
                m_animTimer += deltaTime * 5.0f;
            }
            break;
        }

        case PassiveMobState::FLEE: {
            m_headGrazeAngle = -0.15f; // Head held high while trotting away
            m_stateTimer -= deltaTime;

            // Recalculate flee direction away from player
            glm::vec2 away = glm::normalize(glm::vec2(m_pos.x - playerPos.x, m_pos.z - playerPos.z));
            m_fleeDir = glm::vec3(away.x, 0.0f, away.y);

            float targetYaw = atan2(m_fleeDir.x, m_fleeDir.z);
            float diff = targetYaw - m_yaw;
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            m_yaw += diff * 6.0f * deltaTime;

            m_speed = (m_size == DeerSize::FAWN) ? 3.4f : 3.2f;
            m_pos.x += sin(m_yaw) * m_speed * deltaTime;
            m_pos.z += cos(m_yaw) * m_speed * deltaTime;
            m_animTimer += deltaTime * 9.5f;

            if (m_stateTimer <= 0.0f) {
                m_state = PassiveMobState::IDLE;
                m_stateTimer = 2.5f;
            }
            break;
        }

        default:
            break;
    }

    // Keep grounded on terrain
    m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);

    updateModelMesh();
}

bool PassiveMob::TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers) {
    if (m_state == PassiveMobState::DEAD) return false;

    // RPG Damage Mitigation Formula
    int effectiveDamage = std::max(1, damage - m_defense);
    m_currentHp -= effectiveDamage;
    m_hitFlashTimer = 0.20f;
    m_showHpBarTimer = 5.0f;

    // Spawn Blood & Hit Particles
    glm::vec3 hitPos = m_pos + glm::vec3(0.0f, 1.2f * m_scale, 0.0f);
    glm::vec4 bloodCol = (m_size == DeerSize::DEMONIC) ? glm::vec4(0.50f, 0.02f, 0.05f, 1.0f) : glm::vec4(0.85f, 0.05f, 0.05f, 1.0f);
    for (int i = 0; i < 16; ++i) {
        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.3f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
        particles.SpawnParticle(hitPos, pVel, bloodCol, 0.12f, 0.75f, -9.8f);
    }

    if (m_currentHp <= 0) {
        m_currentHp = 0;
        m_state = PassiveMobState::DEAD;
        m_deathTimer = 0.0f;
        for (int i = 0; i < 28; ++i) {
            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.5f)*4.5f, (rand()%100/50.0f - 1.0f)*4.0f);
            particles.SpawnParticle(hitPos, pVel, bloodCol, 0.15f, 1.0f, -9.8f);
        }
        return true; // KILLED!
    }

    // Demonic Deer becomes fiercely enraged on hit and engages in chase!
    if (m_size == DeerSize::DEMONIC) {
        m_state = PassiveMobState::CHASE;
        return false;
    }

    // Alpha counterattack on hit if ready!
    if (m_size == DeerSize::ALPHA && m_attackCooldown <= 0.0f && player != nullptr) {
        player->TakeDamage(16, damageNumbers);
        m_attackCooldown = 4.0f;
    }

    // Immediate panic & trot away
    m_state = PassiveMobState::FLEE;
    m_stateTimer = 2.4f;
    glm::vec2 away = glm::normalize(glm::vec2(m_pos.x - hitOrigin.x, m_pos.z - hitOrigin.z));
    m_fleeDir = glm::vec3(away.x, 0.0f, away.y);

    return false; // NOT KILLED!
}

void PassiveMob::updateModelMesh() {
    if (m_baseBoxes.empty() || m_VAO == 0) return;

    std::vector<TransformedBox> transformedBoxes;
    transformedBoxes.reserve(m_baseBoxes.size());

    // Leg swings (Trot / Gallop)
    float legSwing = (m_speed > 0.1f) ? sin(m_animTimer) * 0.45f : 0.0f;
    float spineBob = (m_speed > 0.1f) ? std::abs(sin(m_animTimer * 2.0f)) * 0.06f : 0.0f;

    // Hip/Shoulder Pivots
    glm::vec3 frontLegPivotL(-0.20f, 0.95f, 0.45f);
    glm::vec3 frontLegPivotR(0.20f, 0.95f, 0.45f);
    glm::vec3 backLegPivotL(-0.20f, 0.95f, -0.45f);
    glm::vec3 backLegPivotR(0.20f, 0.95f, -0.45f);
    glm::vec3 neckPivot(0.00f, 1.30f, 0.50f);

    for (const auto& box : m_baseBoxes) {
        // Skip antlers on Fawn
        if (m_size == DeerSize::FAWN && box.Name.find("ANTLER") != std::string::npos) {
            continue;
        }

        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, box.Pos);
        M = glm::rotate(M, box.Rot.z, glm::vec3(0,0,1));
        M = glm::rotate(M, box.Rot.y, glm::vec3(0,1,0));
        M = glm::rotate(M, box.Rot.x, glm::vec3(1,0,0));
        M = glm::scale(M, box.Scale);

        glm::vec3 finalColor = box.Color;

        if (m_size == DeerSize::ALPHA) {
            finalColor *= 0.85f; // Darker majestic coat for Alpha
            if (box.Name.find("ANTLER") != std::string::npos) finalColor = glm::vec3(0.95f, 0.90f, 0.70f);
        } else if (m_size == DeerSize::FAWN) {
            finalColor = glm::mix(finalColor, glm::vec3(0.65f, 0.42f, 0.22f), 0.35f); // Light coat
        } else if (m_size == DeerSize::DEMONIC) {
            // Rotten necrotic body colors
            if (box.Name == "EYE_L" || box.Name == "EYE_R") {
                // Piercing glowing blood-red eyes
                float pulse = 1.3f + 0.5f * sin(m_glowPulse);
                finalColor = glm::vec3(1.0f, 0.03f, 0.03f) * pulse;
            } else if (box.Name.find("ANTLER") != std::string::npos) {
                // Jagged blood-drenched antlers
                finalColor = glm::vec3(0.55f, 0.08f, 0.08f);
            } else {
                // Decaying diseased flesh (ash greenish-grey with necrotic dark spots)
                finalColor = glm::vec3(0.18f, 0.22f, 0.16f);
                if (box.Name == "BELLY" || box.Name == "SNOUT") finalColor = glm::vec3(0.12f, 0.14f, 0.10f);
            }
        }

        if (m_hitFlashTimer > 0.0f) {
            finalColor = glm::mix(finalColor, glm::vec3(1.0f, 0.15f, 0.15f), 0.75f);
        }

        if (m_state == PassiveMobState::DEAD) {
            // Collapse to side on ground
            glm::mat4 deadM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.35f * m_scale, 0.0f));
            deadM = glm::rotate(deadM, glm::radians(85.0f), glm::vec3(0, 0, 1));
            deadM = deadM * M;
            transformedBoxes.push_back({deadM, finalColor * 0.65f});
            continue;
        }

        // Front-Left & Back-Right (Pair 1)
        if (box.Name == "LEG_FL") {
            glm::mat4 legM = glm::translate(glm::mat4(1.0f), frontLegPivotL) * glm::rotate(glm::mat4(1.0f), legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -frontLegPivotL) * M;
            transformedBoxes.push_back({legM, finalColor});
        }
        else if (box.Name == "LEG_BR") {
            glm::mat4 legM = glm::translate(glm::mat4(1.0f), backLegPivotR) * glm::rotate(glm::mat4(1.0f), legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -backLegPivotR) * M;
            transformedBoxes.push_back({legM, finalColor});
        }
        // Front-Right & Back-Left (Pair 2)
        else if (box.Name == "LEG_FR") {
            glm::mat4 legM = glm::translate(glm::mat4(1.0f), frontLegPivotR) * glm::rotate(glm::mat4(1.0f), -legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -frontLegPivotR) * M;
            transformedBoxes.push_back({legM, finalColor});
        }
        else if (box.Name == "LEG_BL") {
            glm::mat4 legM = glm::translate(glm::mat4(1.0f), backLegPivotL) * glm::rotate(glm::mat4(1.0f), -legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -backLegPivotL) * M;
            transformedBoxes.push_back({legM, finalColor});
        }
        // Head, Snout, Ears, Eyes, Antlers (Grazing / Aggressive rotation)
        else if (box.Name == "NECK" || box.Name == "HEAD" || box.Name == "SNOUT" || box.Name == "EYE_L" || box.Name == "EYE_R" || box.Name.find("EAR") != std::string::npos || box.Name.find("ANTLER") != std::string::npos) {
            glm::mat4 headM = glm::translate(glm::mat4(1.0f), neckPivot) * glm::rotate(glm::mat4(1.0f), -m_headGrazeAngle, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -neckPivot) * M;
            transformedBoxes.push_back({headM, finalColor});
        }
        // Torso / Chest with spine bounce
        else {
            glm::mat4 bodyM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, spineBob, 0.0f)) * M;
            transformedBoxes.push_back({bodyM, finalColor});
        }
    }

    std::vector<float> rawVertices;
    ModelLoader::GenerateMeshTransformed(transformedBoxes, rawVertices);
    m_vertexCount = rawVertices.size() / 11;

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PassiveMob::Render(GLuint shaderProgram) {
    if (m_vertexCount == 0 || m_VAO == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_pos);
    model = glm::rotate(model, m_yaw, glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(m_scale));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    glBindVertexArray(0);
}

void PassiveMob::RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (m_showHpBarTimer <= 0.0f || m_state == PassiveMobState::DEAD) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::vec3 barPos = m_pos + glm::vec3(0.0f, 2.55f * m_scale, 0.0f);

    // Billboard rotation facing camera
    glm::vec3 toCam = glm::normalize(cameraPos - barPos);
    float yaw = atan2(toCam.x, toCam.z);
    float pitch = -asin(toCam.y);

    // Background Bar (Black/Dark Red)
    glm::mat4 bgModel = glm::mat4(1.0f);
    bgModel = glm::translate(bgModel, barPos);
    bgModel = glm::rotate(bgModel, yaw, glm::vec3(0, 1, 0));
    bgModel = glm::rotate(bgModel, pitch, glm::vec3(1, 0, 0));
    bgModel = glm::scale(bgModel, glm::vec3(1.1f * m_scale, 1.2f * m_scale, 1.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgModel));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1); // Skip lighting for UI

    glBindVertexArray(s_hpBarVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Foreground Health Bar
    float hpPercent = (float)m_currentHp / (float)m_maxHp;
    if (hpPercent > 0.0f) {
        glm::mat4 fgModel = glm::mat4(1.0f);
        fgModel = glm::translate(fgModel, barPos + toCam * 0.02f);
        fgModel = glm::rotate(fgModel, yaw, glm::vec3(0, 1, 0));
        fgModel = glm::rotate(fgModel, pitch, glm::vec3(1, 0, 0));
        fgModel = glm::translate(fgModel, glm::vec3(-(1.0f - hpPercent) * 0.5f * m_scale, 0.0f, 0.0f));
        fgModel = glm::scale(fgModel, glm::vec3(hpPercent * m_scale, 1.0f * m_scale, 1.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(fgModel));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glBindVertexArray(0);
}
