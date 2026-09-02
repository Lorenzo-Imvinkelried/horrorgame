#include "DummyMob.h"
#include "WorldGenerator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

DummyMob::DummyMob(glm::vec3 spawnPos)
    : BaseMob(spawnPos, "DUMMY DE PRUEBAS (1.000.000 HP)")
{
    m_maxHp = 1000000;
    m_currentHp = 1000000;
    m_defense = 0;
    m_evasion = 0;
    m_level = 99;
    m_scale = 1.0f;
    m_radius = 0.85f;

    initMesh();
}

void DummyMob::initMesh() {
    // Carga de datos de modelo desacoplados desde assets/models/mobs/dummy.txt
    std::vector<BoxDef> boxes = ModelLoader::Load("assets/models/mobs/dummy.txt");
    if (boxes.empty()) {
        std::cerr << "[DummyMob] No se encontro assets/models/mobs/dummy.txt, usando fallback." << std::endl;
        boxes = ModelLoader::Load("assets/models/equipment/player_body.txt");
        for (auto& b : boxes) {
            b.Color = glm::vec3(1.0f, 0.85f, 0.15f);
        }
    }

    std::vector<float> vertexData;
    ModelLoader::GenerateMesh(boxes, vertexData);
    m_vertexCount = vertexData.size() / 11;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    // Layout: Pos (0), Color (1), UV (2), Normal (3)
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

void DummyMob::Update(float deltaTime, glm::vec3 playerPos, Player* player,
                      ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                      ProjectileSystem& projectiles)
{
    if (m_hitFlashTimer > 0.0f) m_hitFlashTimer -= deltaTime;
    if (m_showHpBarTimer > 0.0f) m_showHpBarTimer -= deltaTime;

    // Se mantiene fijo sobre el relieve del terreno sin moverse ni rotar
    m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
}

void DummyMob::Render(GLuint shaderProgram) {
    if (m_VAO == 0 || m_vertexCount == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_pos);
    model = glm::rotate(model, m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
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

bool DummyMob::TakeDamage(int damage, glm::vec3 hitOrigin,
                          ParticleSystem& particles, Player* player,
                          DamageNumberSystem& damageNumbers)
{
    int effectiveDamage = std::max(1, damage - m_defense);
    m_currentHp -= effectiveDamage;
    m_hitFlashTimer = 0.20f;
    m_showHpBarTimer = 5.0f;

    // Chispas doradas brillantes al recibir impacto
    glm::vec3 hitPos = m_pos + glm::vec3(0.0f, 1.2f * m_scale, 0.0f);
    glm::vec4 sparkCol = glm::vec4(1.0f, 0.90f, 0.20f, 1.0f);

    for (int i = 0; i < 18; ++i) {
        glm::vec3 pVel((rand() % 100 / 50.0f - 1.0f) * 3.0f,
                       (rand() % 100 / 50.0f + 0.3f) * 3.5f,
                       (rand() % 100 / 50.0f - 1.0f) * 3.0f);
        particles.SpawnParticle(hitPos, pVel, sparkCol, 0.14f, 0.8f, -9.8f);
    }

    // Si la salud llega a 0, se autorregenera instantáneamente para seguir testeando
    if (m_currentHp <= 0) {
        m_currentHp = m_maxHp;
        return false;
    }

    return false;
}
