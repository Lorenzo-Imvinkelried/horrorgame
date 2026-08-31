#include "Dragon.h"
#include "WorldGenerator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

Dragon::Dragon(glm::vec3 spawnPos)
    : m_pos(spawnPos), m_velocity(0.0f), m_yaw(0.0f), m_pitch(0.0f), m_roll(0.0f),
      m_flightAltitude(44.0f), m_circleRadius(85.0f), m_flightSpeed(14.0f), m_orbitAngle(0.0f),
      m_animTimer(0.0f), m_wingFlap(0.0f), m_stateTimer(12.0f), m_state(DragonState::SOARING),
      m_maxHp(1800), m_currentHp(1800), m_hitFlashTimer(0.0f), m_showHpBarTimer(0.0f),
      m_breathCooldown(6.0f), m_VAO(0), m_VBO(0), m_vertexCount(0)
{
    initMeshes();
}

Dragon::~Dragon() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void Dragon::initMeshes() {
    m_baseBoxes.clear();

    glm::vec3 scaleCol = glm::vec3(0.55f, 0.12f, 0.12f);    // Rojo carmesí escamoso
    glm::vec3 darkScale = glm::vec3(0.24f, 0.08f, 0.08f);   // Escamas dorsales oscuras
    glm::vec3 bellyCol = glm::vec3(0.72f, 0.48f, 0.22f);    // Placas ventrales doradas
    glm::vec3 hornCol = glm::vec3(0.18f, 0.18f, 0.18f);     // Cuernos obsidiana
    glm::vec3 wingSkin = glm::vec3(0.42f, 0.10f, 0.10f);    // Membrana alar carmesí
    glm::vec3 eyeCol = glm::vec3(1.0f, 0.85f, 0.15f);       // Ojos de fuego ámbar

    // 1. Torso y Pecho
    m_baseBoxes.push_back({ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.6f, 1.4f, 3.2f), glm::vec3(0.0f), scaleCol, "TORSO" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, -0.4f, 0.2f), glm::vec3(1.4f, 0.8f, 2.8f), glm::vec3(0.0f), bellyCol, "BELLY" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 0.75f, 0.0f), glm::vec3(0.25f, 0.4f, 3.0f), glm::vec3(0.0f), darkScale, "SPINE_DORSAL" });

    // 2. Cuello articulado
    m_baseBoxes.push_back({ glm::vec3(0.0f, 0.35f, 2.0f), glm::vec3(1.0f, 1.0f, 1.4f), glm::vec3(0.0f), scaleCol, "NECK_BASE" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 0.85f, 3.0f), glm::vec3(0.85f, 0.85f, 1.4f), glm::vec3(0.0f), scaleCol, "NECK_MID" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 1.35f, 3.9f), glm::vec3(0.75f, 0.75f, 1.2f), glm::vec3(0.0f), scaleCol, "NECK_TOP" });

    // 3. Cabeza y Fauces
    m_baseBoxes.push_back({ glm::vec3(0.0f, 1.6f, 4.8f), glm::vec3(1.1f, 0.85f, 1.6f), glm::vec3(0.0f), scaleCol, "HEAD" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 1.45f, 5.8f), glm::vec3(0.8f, 0.55f, 1.3f), glm::vec3(0.0f), darkScale, "SNOUT" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 5.5f), glm::vec3(0.7f, 0.35f, 1.2f), glm::vec3(0.0f), darkScale, "JAW_LOWER" });

    // Cuernos y Ojos
    m_baseBoxes.push_back({ glm::vec3(-0.45f, 2.1f, 4.6f), glm::vec3(0.18f, 0.65f, 0.9f), glm::vec3(-0.25f, -0.15f, -0.2f), hornCol, "HORN_L" });
    m_baseBoxes.push_back({ glm::vec3(0.45f, 2.1f, 4.6f), glm::vec3(0.18f, 0.65f, 0.9f), glm::vec3(-0.25f, 0.15f, 0.2f), hornCol, "HORN_R" });
    m_baseBoxes.push_back({ glm::vec3(-0.48f, 1.75f, 5.2f), glm::vec3(0.15f, 0.15f, 0.25f), glm::vec3(0.0f), eyeCol, "EYE_L" });
    m_baseBoxes.push_back({ glm::vec3(0.48f, 1.75f, 5.2f), glm::vec3(0.15f, 0.15f, 0.25f), glm::vec3(0.0f), eyeCol, "EYE_R" });

    // 4. Cola articulada con aleta de timón
    m_baseBoxes.push_back({ glm::vec3(0.0f, -0.05f, -2.0f), glm::vec3(1.1f, 1.0f, 1.8f), glm::vec3(0.0f), scaleCol, "TAIL_1" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, -0.10f, -3.6f), glm::vec3(0.85f, 0.8f, 1.8f), glm::vec3(0.0f), scaleCol, "TAIL_2" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, -0.15f, -5.2f), glm::vec3(0.65f, 0.65f, 1.8f), glm::vec3(0.0f), scaleCol, "TAIL_3" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, -0.20f, -6.8f), glm::vec3(0.45f, 0.45f, 1.8f), glm::vec3(0.0f), scaleCol, "TAIL_4" });
    m_baseBoxes.push_back({ glm::vec3(0.0f, -0.15f, -7.8f), glm::vec3(0.15f, 0.85f, 1.2f), glm::vec3(0.0f), darkScale, "TAIL_FIN" });

    // 5. Patas traseras aerodinámicas (plegadas)
    m_baseBoxes.push_back({ glm::vec3(-0.95f, -0.6f, -1.2f), glm::vec3(0.55f, 1.2f, 0.85f), glm::vec3(-0.35f, 0.0f, 0.1f), scaleCol, "THIGH_L" });
    m_baseBoxes.push_back({ glm::vec3(0.95f, -0.6f, -1.2f), glm::vec3(0.55f, 1.2f, 0.85f), glm::vec3(-0.35f, 0.0f, -0.1f), scaleCol, "THIGH_R" });
    m_baseBoxes.push_back({ glm::vec3(-0.95f, -1.3f, -1.6f), glm::vec3(0.35f, 0.35f, 0.75f), glm::vec3(0.2f, 0.0f, 0.0f), hornCol, "CLAW_L" });
    m_baseBoxes.push_back({ glm::vec3(0.95f, -1.3f, -1.6f), glm::vec3(0.35f, 0.35f, 0.75f), glm::vec3(0.2f, 0.0f, 0.0f), hornCol, "CLAW_R" });

    // 6. Alas gigantescas articuladas (Brazo, Codo y Membrana)
    // Ala Izquierda
    m_baseBoxes.push_back({ glm::vec3(-1.4f, 0.45f, 0.6f), glm::vec3(2.2f, 0.35f, 0.45f), glm::vec3(0.0f), darkScale, "WING_ARM_INNER_L" });
    m_baseBoxes.push_back({ glm::vec3(-3.4f, 0.55f, 0.8f), glm::vec3(2.4f, 0.28f, 0.35f), glm::vec3(0.0f), darkScale, "WING_ARM_OUTER_L" });
    m_baseBoxes.push_back({ glm::vec3(-2.4f, 0.45f, -0.2f), glm::vec3(4.2f, 0.08f, 2.2f), glm::vec3(0.0f), wingSkin, "WING_MEMBRANE_L" });

    // Ala Derecha
    m_baseBoxes.push_back({ glm::vec3(1.4f, 0.45f, 0.6f), glm::vec3(2.2f, 0.35f, 0.45f), glm::vec3(0.0f), darkScale, "WING_ARM_INNER_R" });
    m_baseBoxes.push_back({ glm::vec3(3.4f, 0.55f, 0.8f), glm::vec3(2.4f, 0.28f, 0.35f), glm::vec3(0.0f), darkScale, "WING_ARM_OUTER_R" });
    m_baseBoxes.push_back({ glm::vec3(2.4f, 0.45f, -0.2f), glm::vec3(4.2f, 0.08f, 2.2f), glm::vec3(0.0f), wingSkin, "WING_MEMBRANE_R" });

    // Crear VAO/VBO
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

    updateModelMesh();
}

void Dragon::updateModelMesh() {
    if (m_baseBoxes.empty() || m_VAO == 0) return;

    std::vector<TransformedBox> transformedBoxes;
    transformedBoxes.reserve(m_baseBoxes.size());

    // Animación de aleteo y ondulación de cola
    float flapAngle = sin(m_wingFlap) * 0.55f;
    float flapTip = sin(m_wingFlap - 0.4f) * 0.35f;
    float tailSway = sin(m_animTimer * 2.2f) * 0.22f;

    glm::vec3 wingPivotL(-0.8f, 0.45f, 0.6f);
    glm::vec3 wingPivotR(0.8f, 0.45f, 0.6f);
    glm::vec3 tailPivot(0.0f, 0.0f, -1.0f);

    for (const auto& box : m_baseBoxes) {
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, box.Pos);
        M = glm::rotate(M, box.Rot.z, glm::vec3(0,0,1));
        M = glm::rotate(M, box.Rot.y, glm::vec3(0,1,0));
        M = glm::rotate(M, box.Rot.x, glm::vec3(1,0,0));
        M = glm::scale(M, box.Scale);

        glm::vec3 finalColor = box.Color;
        if (m_hitFlashTimer > 0.0f) {
            finalColor = glm::mix(finalColor, glm::vec3(1.0f, 0.95f, 0.95f), 0.70f);
        }

        // Ala Izquierda
        if (box.Name.find("WING_ARM_INNER_L") != std::string::npos || box.Name.find("WING_MEMBRANE_L") != std::string::npos) {
            glm::mat4 wingM = glm::translate(glm::mat4(1.0f), wingPivotL) * glm::rotate(glm::mat4(1.0f), flapAngle, glm::vec3(0,0,1)) * glm::translate(glm::mat4(1.0f), -wingPivotL) * M;
            transformedBoxes.push_back({ wingM, finalColor });
        }
        else if (box.Name.find("WING_ARM_OUTER_L") != std::string::npos) {
            glm::mat4 wingM = glm::translate(glm::mat4(1.0f), wingPivotL) * glm::rotate(glm::mat4(1.0f), flapAngle + flapTip, glm::vec3(0,0,1)) * glm::translate(glm::mat4(1.0f), -wingPivotL) * M;
            transformedBoxes.push_back({ wingM, finalColor });
        }
        // Ala Derecha
        else if (box.Name.find("WING_ARM_INNER_R") != std::string::npos || box.Name.find("WING_MEMBRANE_R") != std::string::npos) {
            glm::mat4 wingM = glm::translate(glm::mat4(1.0f), wingPivotR) * glm::rotate(glm::mat4(1.0f), -flapAngle, glm::vec3(0,0,1)) * glm::translate(glm::mat4(1.0f), -wingPivotR) * M;
            transformedBoxes.push_back({ wingM, finalColor });
        }
        else if (box.Name.find("WING_ARM_OUTER_R") != std::string::npos) {
            glm::mat4 wingM = glm::translate(glm::mat4(1.0f), wingPivotR) * glm::rotate(glm::mat4(1.0f), -flapAngle - flapTip, glm::vec3(0,0,1)) * glm::translate(glm::mat4(1.0f), -wingPivotR) * M;
            transformedBoxes.push_back({ wingM, finalColor });
        }
        // Cola
        else if (box.Name.find("TAIL") != std::string::npos) {
            glm::mat4 tailM = glm::translate(glm::mat4(1.0f), tailPivot) * glm::rotate(glm::mat4(1.0f), tailSway, glm::vec3(0,1,0)) * glm::translate(glm::mat4(1.0f), -tailPivot) * M;
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

void Dragon::Update(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    m_animTimer += deltaTime;
    m_wingFlap += deltaTime * 3.4f;

    if (m_hitFlashTimer > 0.0f) m_hitFlashTimer -= deltaTime;
    if (m_showHpBarTimer > 0.0f) m_showHpBarTimer -= deltaTime;
    if (m_breathCooldown > 0.0f) m_breathCooldown -= deltaTime;

    // FSM de vuelo dracónico
    m_stateTimer -= deltaTime;

    switch (m_state) {
        case DragonState::SOARING: {
            m_orbitAngle += (m_flightSpeed / m_circleRadius) * deltaTime;
            
            float targetX = playerPos.x + cos(m_orbitAngle) * m_circleRadius;
            float targetZ = playerPos.z + sin(m_orbitAngle) * m_circleRadius;
            float targetY = m_flightAltitude + sin(m_animTimer * 0.8f) * 4.0f;

            glm::vec3 toTarget = glm::vec3(targetX, targetY, targetZ) - m_pos;
            float dist = glm::length(toTarget);
            if (dist > 0.1f) {
                glm::vec3 moveDir = glm::normalize(toTarget);
                m_pos += moveDir * m_flightSpeed * deltaTime;
                
                float desiredYaw = atan2(moveDir.x, moveDir.z);
                float diff = desiredYaw - m_yaw;
                while (diff > 3.14159f) diff -= 6.28318f;
                while (diff < -3.14159f) diff += 6.28318f;
                m_yaw += diff * 2.2f * deltaTime;

                m_pitch = glm::clamp(-moveDir.y * 0.6f, -0.4f, 0.4f);
                m_roll = glm::clamp(diff * 1.5f, -0.65f, 0.65f);
            }

            if (m_stateTimer <= 0.0f && m_breathCooldown <= 0.0f) {
                m_state = DragonState::DIVE_BOMB;
                m_stateTimer = 4.5f;
            }
            break;
        }

        case DragonState::DIVE_BOMB: {
            // Rasante veloz hacia el jugador
            glm::vec3 diveTarget = playerPos + glm::vec3(0.0f, 6.5f, 0.0f);
            glm::vec3 toDive = diveTarget - m_pos;
            float dist = glm::length(toDive);

            if (dist > 2.0f) {
                glm::vec3 moveDir = glm::normalize(toDive);
                m_pos += moveDir * (m_flightSpeed * 1.85f) * deltaTime;
                m_yaw = atan2(moveDir.x, moveDir.z);
                m_pitch = glm::clamp(-moveDir.y * 0.8f, -0.7f, 0.7f);
            }

            if (dist < 22.0f) {
                m_state = DragonState::BREATH_FIRE;
                m_stateTimer = 2.8f;
            } else if (m_stateTimer <= 0.0f) {
                m_state = DragonState::ASCENDING;
                m_stateTimer = 4.0f;
            }
            break;
        }

        case DragonState::BREATH_FIRE: {
            // Aliento de fuego llameante en el aire
            glm::vec3 snoutPos = m_pos + glm::vec3(sin(m_yaw) * 4.5f, 1.2f, cos(m_yaw) * 4.5f);
            glm::vec3 fireDir = glm::normalize(playerPos - snoutPos + glm::vec3(0, 0.5f, 0));

            for (int i = 0; i < 4; ++i) {
                glm::vec3 fVel = fireDir * (18.0f + (rand() % 10)) + glm::vec3(
                    (rand() % 100 / 50.0f - 1.0f) * 2.0f,
                    (rand() % 100 / 50.0f - 1.0f) * 1.5f,
                    (rand() % 100 / 50.0f - 1.0f) * 2.0f
                );
                glm::vec4 fireCol = ((rand() % 2) == 0) ? glm::vec4(1.0f, 0.65f, 0.12f, 0.95f) : glm::vec4(0.95f, 0.25f, 0.05f, 0.95f);
                particles.SpawnParticle(snoutPos, fVel, fireCol, 0.22f, 0.85f, -3.0f);
            }

            // Continuar avanzando hacia adelante
            m_pos += glm::vec3(sin(m_yaw), 0.1f, cos(m_yaw)) * m_flightSpeed * deltaTime;

            if (m_stateTimer <= 0.0f) {
                m_state = DragonState::ASCENDING;
                m_stateTimer = 5.0f;
                m_breathCooldown = 9.0f;
            }
            break;
        }

        case DragonState::ASCENDING: {
            // Ascenso majestuoso a las nubes
            m_pos.y += 8.0f * deltaTime;
            m_pos += glm::vec3(sin(m_yaw), 0.0f, cos(m_yaw)) * m_flightSpeed * deltaTime;
            m_pitch = -0.35f;

            if (m_pos.y >= m_flightAltitude || m_stateTimer <= 0.0f) {
                m_state = DragonState::SOARING;
                m_stateTimer = 14.0f + (rand() % 8);
            }
            break;
        }
    }

    // Partículas de estela de brasas en las alas/hocico
    if ((rand() % 100) < 35) {
        glm::vec3 emberPos = m_pos + glm::vec3((rand() % 100 / 50.0f - 1.0f) * 1.5f, 0.0f, (rand() % 100 / 50.0f - 1.0f) * 1.5f);
        particles.SpawnParticle(emberPos, glm::vec3(0, 0.5f, 0), glm::vec4(1.0f, 0.55f, 0.1f, 0.8f), 0.12f, 0.5f, 0.0f);
    }

    updateModelMesh();
}

void Dragon::Render(GLuint shaderProgram) {
    if (m_VAO == 0 || m_vertexCount == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_pos);
    model = glm::rotate(model, m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, m_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, m_roll, glm::vec3(0.0f, 0.0f, 1.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    glBindVertexArray(0);
}

void Dragon::RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (m_showHpBarTimer <= 0.0f || m_currentHp <= 0) return;

    float hpPct = std::clamp((float)m_currentHp / (float)m_maxHp, 0.0f, 1.0f);
    glm::vec3 barPos = m_pos + glm::vec3(0.0f, 4.2f, 0.0f);

    glm::vec3 toCam = cameraPos - barPos;
    float yawToCam = atan2(toCam.x, toCam.z);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, barPos);
    model = glm::rotate(model, yawToCam, glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Simple red/green quad billboard
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
}

bool Dragon::TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    if (m_currentHp <= 0) return false;

    m_currentHp -= damage;
    m_hitFlashTimer = 0.22f;
    m_showHpBarTimer = 6.0f;

    for (int i = 0; i < 20; ++i) {
        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.3f)*4.0f, (rand()%100/50.0f - 1.0f)*3.5f);
        particles.SpawnParticle(hitOrigin, pVel, glm::vec4(0.95f, 0.40f, 0.10f, 1.0f), 0.16f, 0.8f, -9.8f);
    }

    if (m_currentHp <= 0) {
        m_currentHp = 0;
        return true;
    }
    return false;
}
