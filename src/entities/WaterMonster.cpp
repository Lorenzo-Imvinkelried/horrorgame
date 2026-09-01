#include "WaterMonster.h"
#include "WorldGenerator.h"
#include "Player.h"
#include "Config.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>

GLuint WaterMonster::s_hpBarVAO = 0;
GLuint WaterMonster::s_hpBarVBO = 0;

void WaterMonster::initHpBarMesh() {
    if (s_hpBarVAO != 0) return;

    float quadVertices[] = {
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

WaterMonster::WaterMonster(glm::vec3 spawnPos)
    : m_state(WaterMonsterState::SUBMERGED_LURKING)
    , m_pos(spawnPos)
    , m_lakeCenterPos(spawnPos)
    , m_yaw((float)(rand() % 360) * 0.01745f)
    , m_animTimer(0.0f)
    , m_dragTimer(0.0f)
    , m_splashTimer(0.0f)
    , m_attackCooldown(0.0f)
    , m_eyeGlowPulse(0.0f)
    , m_maxHp(120)
    , m_currentHp(120)
    , m_defense(4)
    , m_hitFlashTimer(0.0f)
    , m_showHpBarTimer(0.0f)
    , m_deathTimer(0.0f)
    , m_VAO(0)
    , m_VBO(0)
    , m_vertexCount(0)
{
    // Search nearby deeper water point to designate as the lake center
    for (int i = 0; i < 8; ++i) {
        float angle = (float)i * 0.7853f;
        float testX = spawnPos.x + cos(angle) * 12.0f;
        float testZ = spawnPos.z + sin(angle) * 12.0f;
        float testY = WorldGenerator::GetHeight(testX, testZ);
        if (testY < WorldGenerator::GetHeight(m_lakeCenterPos.x, m_lakeCenterPos.z)) {
            m_lakeCenterPos = glm::vec3(testX, testY, testZ);
        }
    }

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
    initMesh();
    updateModelMesh();
}

WaterMonster::~WaterMonster() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void WaterMonster::initMesh() {
    m_baseBoxes.clear();

    // Grotesque Amphibious Lake Demon
    // Head with needle fangs & gill crest
    m_baseBoxes.push_back({ glm::vec3(0.0f, 1.40f, 0.20f), glm::vec3(0.38f, 0.32f, 0.45f), glm::vec3(0.0f), glm::vec3(0.08f, 0.16f, 0.14f), "HEAD" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 1.62f, 0.10f), glm::vec3(0.06f, 0.22f, 0.35f), glm::vec3(-0.25f, 0.0f, 0.0f), glm::vec3(0.12f, 0.28f, 0.22f), "CREST" });

    // Piercing Bioluminescent Aquatic Eyes
    m_baseBoxes.push_back({ glm::vec3(-0.11f, 1.48f, 0.38f), glm::vec3(0.05f, 0.04f, 0.04f), glm::vec3(0.0f), glm::vec3(0.10f, 0.95f, 0.85f), "EYE_L" });
    m_baseBoxes.push_back({ glm::vec3(0.11f, 1.48f, 0.38f), glm::vec3(0.05f, 0.04f, 0.04f), glm::vec3(0.0f), glm::vec3(0.10f, 0.95f, 0.85f), "EYE_R" });

    // Slimy Scaled Torso
    m_baseBoxes.push_back({ glm::vec3(0.0f, 0.95f, 0.0f), glm::vec3(0.55f, 0.65f, 0.40f), glm::vec3(0.0f), glm::vec3(0.06f, 0.12f, 0.10f), "TORSO" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, -0.22f), glm::vec3(0.04f, 0.45f, 0.35f), glm::vec3(0.15f, 0.0f, 0.0f), glm::vec3(0.15f, 0.32f, 0.25f), "SPINE_FIN" });

    // Extended Webbed Claw Arms (for grabbing victims)
    m_baseBoxes.push_back({ glm::vec3(-0.36f, 1.05f, 0.25f), glm::vec3(0.15f, 0.15f, 0.70f), glm::vec3(0.0f), glm::vec3(0.08f, 0.16f, 0.14f), "ARM_L" });
    m_baseBoxes.push_back({ glm::vec3(0.36f, 1.05f, 0.25f), glm::vec3(0.15f, 0.15f, 0.70f), glm::vec3(0.0f), glm::vec3(0.08f, 0.16f, 0.14f), "ARM_R" });
    m_baseBoxes.push_back({ glm::vec3(-0.36f, 1.05f, 0.65f), glm::vec3(0.24f, 0.08f, 0.22f), glm::vec3(0.0f), glm::vec3(0.18f, 0.35f, 0.28f), "CLAWS_L" });
    m_baseBoxes.push_back({ glm::vec3(0.36f, 1.05f, 0.65f), glm::vec3(0.24f, 0.08f, 0.22f), glm::vec3(0.0f), glm::vec3(0.18f, 0.35f, 0.28f), "CLAWS_R" });

    // Scaled Aquatic Legs & Muscular Tail
    m_baseBoxes.push_back({ glm::vec3(-0.16f, 0.45f, -0.08f), glm::vec3(0.18f, 0.65f, 0.20f), glm::vec3(0.0f), glm::vec3(0.06f, 0.12f, 0.10f), "LEG_L" });
    m_baseBoxes.push_back({ glm::vec3(0.16f, 0.45f, -0.08f), glm::vec3(0.18f, 0.65f, 0.20f), glm::vec3(0.0f), glm::vec3(0.06f, 0.12f, 0.10f), "LEG_R" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 0.40f, -0.45f), glm::vec3(0.18f, 0.25f, 0.70f), glm::vec3(-0.20f, 0.0f, 0.0f), glm::vec3(0.08f, 0.15f, 0.12f), "TAIL" });
}

void WaterMonster::Update(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    if (m_hitFlashTimer > 0.0f) m_hitFlashTimer -= deltaTime;
    if (m_showHpBarTimer > 0.0f) m_showHpBarTimer -= deltaTime;
    if (m_attackCooldown > 0.0f) m_attackCooldown -= deltaTime;
    m_eyeGlowPulse += deltaTime * 5.0f;

    if (m_state == WaterMonsterState::DEAD) {
        m_deathTimer += deltaTime;
        updateModelMesh();
        return;
    }

    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));
    m_splashTimer += deltaTime;

    // =========================================================================
    // 1. SUBMERGED LURKING (Escondido bajo el agua del lago)
    // =========================================================================
    if (m_state == WaterMonsterState::SUBMERGED_LURKING) {
        m_pos.y = Config::Water::Level - 0.75f; // Hidden underwater

        // Subtle water ripple bubbles
        if (m_splashTimer >= 0.25f) {
            m_splashTimer = 0.0f;
            glm::vec3 bPos = m_pos + glm::vec3((rand()%100/100.0f - 0.5f)*1.4f, 0.8f, (rand()%100/100.0f - 0.5f)*1.4f);
            particles.SpawnParticle(bPos, glm::vec3(0.0f, 0.6f, 0.0f), glm::vec4(0.4f, 0.75f, 0.95f, 0.6f), 0.15f, 0.8f, 0.0f);
        }

        // Trigger Ambush: Player approaches water shore (< 7.0m)
        if (distToPlayer < 7.0f) {
            m_state = WaterMonsterState::EMERGING_LUNGE;

            // Explosion of water splashes as it surfaces!
            for (int i = 0; i < 35; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.5f)*4.5f, (rand()%100/50.0f - 1.0f)*4.0f);
                particles.SpawnParticle(m_pos + glm::vec3(0, 0.8f, 0), pVel, glm::vec4(0.6f, 0.85f, 1.0f, 0.85f), 0.18f, 1.0f, -9.8f);
            }
        }
    }

    // =========================================================================
    // 2. EMERGING LUNGE (Sale del agua y se abalanza a agarrar al jugador)
    // =========================================================================
    else if (m_state == WaterMonsterState::EMERGING_LUNGE) {
        float targetY = Config::Water::Level + 0.1f;
        m_pos.y = glm::mix(m_pos.y, targetY, deltaTime * 6.0f);

        glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
        float d2D = glm::length(toP);
        if (d2D > 0.001f) toP /= d2D;

        float targetYaw = atan2(toP.x, toP.y);
        m_yaw = targetYaw;

        // Fast lunge out of water
        m_pos.x += toP.x * 4.2f * deltaTime;
        m_pos.z += toP.y * 4.2f * deltaTime;
        m_animTimer += deltaTime * 12.0f;

        // Spawn ongoing water froth
        if (m_splashTimer >= 0.06f) {
            m_splashTimer = 0.0f;
            particles.SpawnParticle(m_pos + glm::vec3(0, 0.3f, 0), glm::vec3(0.0f, 1.2f, 0.0f), glm::vec4(0.7f, 0.9f, 1.0f, 0.8f), 0.18f, 0.5f, -4.0f);
        }

        // LATCH ON AND GRAB PLAYER (solo si el jugador está a nivel del agua y no sobre un árbol/estructura)
        float vertDiff = std::abs(playerPos.y - m_pos.y);
        if (distToPlayer < 1.9f && vertDiff <= 1.8f) {
            m_state = WaterMonsterState::DRAGGING_PLAYER;
            m_dragTimer = 0.0f;
            if (player != nullptr) {
                player->TakeDamage(12, damageNumbers);
            }
        }
        else if (distToPlayer > 18.0f) {
            // Player escaped back to high dry ground
            m_state = WaterMonsterState::SUBMERGED_LURKING;
        }
    }

    // =========================================================================
    // 3. DRAGGING PLAYER (Te agarra y te arrastra al fondo del agua)
    // =========================================================================
    else if (m_state == WaterMonsterState::DRAGGING_PLAYER) {
        m_dragTimer += deltaTime;

        // Direction towards deep lake center
        glm::vec2 toLake = glm::vec2(m_lakeCenterPos.x - m_pos.x, m_lakeCenterPos.z - m_pos.z);
        float d2Lake = glm::length(toLake);
        if (d2Lake > 0.001f) toLake /= d2Lake;

        m_yaw = atan2(toLake.x, toLake.y);

        // Monster swims and pulls towards deep water at 2.6 m/s
        m_pos.x += toLake.x * 2.6f * deltaTime;
        m_pos.z += toLake.y * 2.6f * deltaTime;
        m_pos.y = Config::Water::Level - 0.2f;
        m_animTimer += deltaTime * 10.0f;

        // DRAG THE PLAYER INTO THE WATER!
        if (player != nullptr) {
            // Pull player right behind the monster in its grip
            player->Position.x = m_pos.x - toLake.x * 0.85f;
            player->Position.z = m_pos.z - toLake.y * 0.85f;
            float terrainBelow = WorldGenerator::GetHeight(player->Position.x, player->Position.z);
            player->Position.y = std::min(Config::Water::Level + 0.2f, terrainBelow + player->PlayerHeight);
            player->IsGrounded = true;

            // Deal crushing drowning damage every 0.9s
            if (m_attackCooldown <= 0.0f) {
                player->TakeDamage(8, damageNumbers);
                m_attackCooldown = 0.9f;
            }

            // Continuous violent water splashing
            if (m_splashTimer >= 0.05f) {
                m_splashTimer = 0.0f;
                glm::vec3 splashP = player->Position + glm::vec3((rand()%100/50.0f - 1.0f)*0.6f, 0.1f, (rand()%100/50.0f - 1.0f)*0.6f);
                particles.SpawnParticle(splashP, glm::vec3((rand()%100/50.0f - 1.0f)*2.0f, (rand()%100/50.0f + 0.5f)*2.5f, (rand()%100/50.0f - 1.0f)*2.0f), glm::vec4(0.5f, 0.8f, 1.0f, 0.8f), 0.18f, 0.6f, -9.8f);
            }

            // DROWNING LETHAL CHECK:
            // If monster successfully dragged player deep into water or timer expires (> 5.5s) without dying:
            if (terrainBelow < 0.2f || m_dragTimer > 5.5f) {
                // Drag player down into the dark abyss -> DROWNING DEATH!
                player->TakeDamage(999, damageNumbers);
                player->Stats.CurrentHP = 0;

                for (int i = 0; i < 40; ++i) {
                    glm::vec3 drownVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.3f)*4.0f, (rand()%100/50.0f - 1.0f)*4.0f);
                    particles.SpawnParticle(player->Position, drownVel, glm::vec4(0.1f, 0.02f, 0.02f, 1.0f), 0.22f, 1.5f, -9.8f);
                    particles.SpawnParticle(player->Position, drownVel * 0.7f, glm::vec4(0.3f, 0.6f, 0.9f, 0.8f), 0.20f, 1.2f, -4.0f);
                }

                m_state = WaterMonsterState::SUBMERGED_LURKING;
            }
        }
    }

    updateModelMesh();
}

bool WaterMonster::TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers) {
    if (m_state == WaterMonsterState::DEAD) return false;

    int effectiveDamage = std::max(1, damage - m_defense);
    m_currentHp -= effectiveDamage;
    m_hitFlashTimer = 0.20f;
    m_showHpBarTimer = 5.0f;

    // Blood & water impact
    glm::vec3 hitPos = m_pos + glm::vec3(0.0f, 1.1f, 0.0f);
    for (int i = 0; i < 18; ++i) {
        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.3f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
        particles.SpawnParticle(hitPos, pVel, glm::vec4(0.12f, 0.45f, 0.35f, 1.0f), 0.16f, 0.75f, -9.8f);
        particles.SpawnParticle(hitPos, pVel * 0.6f, glm::vec4(0.6f, 0.05f, 0.05f, 1.0f), 0.14f, 0.75f, -9.8f);
    }

    if (m_currentHp <= 0) {
        m_currentHp = 0;
        m_state = WaterMonsterState::DEAD;
        m_deathTimer = 0.0f;

        // Explosion of death black water and blood
        for (int i = 0; i < 35; ++i) {
            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.5f)*4.5f, (rand()%100/50.0f - 1.0f)*4.0f);
            particles.SpawnParticle(hitPos, pVel, glm::vec4(0.10f, 0.02f, 0.05f, 1.0f), 0.18f, 1.2f, -9.8f);
            particles.SpawnParticle(hitPos, pVel * 0.8f, glm::vec4(0.4f, 0.8f, 1.0f, 0.8f), 0.18f, 1.0f, -9.8f);
        }
        return true; // KILLED! Player saved from drowning!
    }

    return false;
}

void WaterMonster::updateModelMesh() {
    if (m_baseBoxes.empty() || m_VAO == 0) return;

    std::vector<TransformedBox> transformedBoxes;
    transformedBoxes.reserve(m_baseBoxes.size());

    float armReach = (m_state == WaterMonsterState::DRAGGING_PLAYER || m_state == WaterMonsterState::EMERGING_LUNGE) ? 0.35f + sin(m_animTimer) * 0.15f : 0.0f;
    float tailSwish = sin(m_animTimer * 1.5f) * 0.40f;

    glm::vec3 armPivotL(-0.36f, 1.05f, 0.0f);
    glm::vec3 armPivotR(0.36f, 1.05f, 0.0f);
    glm::vec3 tailPivot(0.0f, 0.40f, -0.10f);

    for (const auto& box : m_baseBoxes) {
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, box.Pos);
        M = glm::rotate(M, box.Rot.z, glm::vec3(0,0,1));
        M = glm::rotate(M, box.Rot.y, glm::vec3(0,1,0));
        M = glm::rotate(M, box.Rot.x, glm::vec3(1,0,0));
        M = glm::scale(M, box.Scale);

        glm::vec3 finalColor = box.Color;

        if (box.Name == "EYE_L" || box.Name == "EYE_R") {
            float pulse = 1.3f + 0.5f * sin(m_eyeGlowPulse);
            finalColor *= pulse;
        }

        if (m_hitFlashTimer > 0.0f) {
            finalColor = glm::mix(finalColor, glm::vec3(1.0f, 0.2f, 0.2f), 0.75f);
        }

        if (m_state == WaterMonsterState::DEAD) {
            // Sinks under water to the bottom
            glm::mat4 deadM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f));
            deadM = glm::rotate(deadM, glm::radians(75.0f), glm::vec3(1, 0, 0));
            deadM = deadM * M;
            transformedBoxes.push_back({ deadM, finalColor * 0.55f });
            continue;
        }

        // Grabbing Arms reach forward
        if (box.Name.find("ARM") != std::string::npos || box.Name.find("CLAWS") != std::string::npos) {
            glm::mat4 armM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, armReach)) * M;
            transformedBoxes.push_back({ armM, finalColor });
        }
        // Tail swimming swish
        else if (box.Name == "TAIL") {
            glm::mat4 tailM = glm::translate(glm::mat4(1.0f), tailPivot) * glm::rotate(glm::mat4(1.0f), tailSwish, glm::vec3(0,1,0)) * glm::translate(glm::mat4(1.0f), -tailPivot) * M;
            transformedBoxes.push_back({ tailM, finalColor });
        }
        else {
            transformedBoxes.push_back({ M, finalColor });
        }
    }

    std::vector<float> rawVertices;
    ModelLoader::GenerateMeshTransformed(transformedBoxes, rawVertices);
    m_vertexCount = rawVertices.size() / 11;

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WaterMonster::Render(GLuint shaderProgram) {
    if (m_vertexCount == 0 || m_VAO == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_pos);
    model = glm::rotate(model, m_yaw, glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(1.35f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    glBindVertexArray(0);
}

void WaterMonster::RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (m_showHpBarTimer <= 0.0f || m_state == WaterMonsterState::DEAD) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::vec3 barPos = m_pos + glm::vec3(0.0f, 2.4f, 0.0f);

    glm::vec3 toCam = glm::normalize(cameraPos - barPos);
    float yaw = atan2(toCam.x, toCam.z);
    float pitch = -asin(toCam.y);

    glm::mat4 bgModel = glm::mat4(1.0f);
    bgModel = glm::translate(bgModel, barPos);
    bgModel = glm::rotate(bgModel, yaw, glm::vec3(0, 1, 0));
    bgModel = glm::rotate(bgModel, pitch, glm::vec3(1, 0, 0));
    bgModel = glm::scale(bgModel, glm::vec3(1.3f, 1.2f, 1.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgModel));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1);

    glBindVertexArray(s_hpBarVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    float hpPercent = (float)m_currentHp / (float)m_maxHp;
    if (hpPercent > 0.0f) {
        glm::mat4 fgModel = glm::mat4(1.0f);
        fgModel = glm::translate(fgModel, barPos + toCam * 0.02f);
        fgModel = glm::rotate(fgModel, yaw, glm::vec3(0, 1, 0));
        fgModel = glm::rotate(fgModel, pitch, glm::vec3(1, 0, 0));
        fgModel = glm::translate(fgModel, glm::vec3(-(1.0f - hpPercent) * 0.65f, 0.0f, 0.0f));
        fgModel = glm::scale(fgModel, glm::vec3(hpPercent * 1.3f, 1.0f, 1.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(fgModel));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glBindVertexArray(0);
}
