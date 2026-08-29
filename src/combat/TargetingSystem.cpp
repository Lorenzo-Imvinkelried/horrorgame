#include "TargetingSystem.h"
#include "PassiveMob.h"
#include "Monster.h"
#include "EnemyMob.h"
#include "WaterMonster.h"
#include "WorldGenerator.h"

void TargetingSystem::initRingMesh() {
    if (m_ringVAO != 0) return;

    std::vector<float> ringVertices;
    const int segments = 32;
    const float innerRadius = 1.05f;
    const float outerRadius = 1.25f;

    for (int i = 0; i < segments; ++i) {
        float theta1 = (float)i / segments * 6.28318f;
        float theta2 = (float)(i + 1) / segments * 6.28318f;

        float c1 = cos(theta1), s1 = sin(theta1);
        float c2 = cos(theta2), s2 = sin(theta2);

        glm::vec3 p1(c1 * innerRadius, 0.0f, s1 * innerRadius);
        glm::vec3 p2(c1 * outerRadius, 0.0f, s1 * outerRadius);
        glm::vec3 p3(c2 * outerRadius, 0.0f, s2 * outerRadius);
        glm::vec3 p4(c2 * innerRadius, 0.0f, s2 * innerRadius);

        glm::vec3 col(0.95f, 0.25f, 0.15f); // Red/Orange targeting glow

        auto addVert = [&](const glm::vec3& p) {
            ringVertices.push_back(p.x); ringVertices.push_back(p.y); ringVertices.push_back(p.z);
            ringVertices.push_back(col.r); ringVertices.push_back(col.g); ringVertices.push_back(col.b);
            ringVertices.push_back(0.0f); ringVertices.push_back(0.0f);
            ringVertices.push_back(0.0f); ringVertices.push_back(1.0f); ringVertices.push_back(0.0f);
        };

        addVert(p1); addVert(p2); addVert(p3);
        addVert(p1); addVert(p3); addVert(p4);
    }

    m_ringVertexCount = (int)ringVertices.size() / 11;

    glGenVertexArrays(1, &m_ringVAO);
    glGenBuffers(1, &m_ringVBO);
    glBindVertexArray(m_ringVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_ringVBO);
    glBufferData(GL_ARRAY_BUFFER, ringVertices.size() * sizeof(float), ringVertices.data(), GL_STATIC_DRAW);

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

glm::vec3 TargetingSystem::GetTargetPosition() const {
    if (m_targetType == TargetType::PASSIVE_MOB && m_passiveTarget) {
        return m_passiveTarget->GetPosition();
    }
    if (m_targetType == TargetType::MONSTER && m_monsterTarget) {
        return m_monsterTarget->GetPosition();
    }
    if (m_targetType == TargetType::ENEMY_MOB && m_enemyTarget) {
        return m_enemyTarget->GetPosition();
    }
    if (m_targetType == TargetType::WATER_MONSTER && m_waterTarget) {
        return m_waterTarget->GetPosition();
    }
    return glm::vec3(0.0f);
}

std::string TargetingSystem::GetTargetName() const {
    if (m_targetType == TargetType::PASSIVE_MOB && m_passiveTarget) return m_passiveTarget->GetName();
    if (m_targetType == TargetType::MONSTER) return "Monstruo de las Sombras";
    if (m_targetType == TargetType::ENEMY_MOB && m_enemyTarget) return m_enemyTarget->GetName();
    if (m_targetType == TargetType::WATER_MONSTER && m_waterTarget) return m_waterTarget->GetName();
    return "";
}

int TargetingSystem::GetTargetLevel() const {
    if (m_targetType == TargetType::PASSIVE_MOB && m_passiveTarget) return m_passiveTarget->GetLevel();
    if (m_targetType == TargetType::MONSTER) return 3;
    if (m_targetType == TargetType::ENEMY_MOB && m_enemyTarget) return m_enemyTarget->GetLevel();
    if (m_targetType == TargetType::WATER_MONSTER && m_waterTarget) return m_waterTarget->GetLevel();
    return 1;
}

int TargetingSystem::GetTargetCurrentHP() const {
    if (m_targetType == TargetType::PASSIVE_MOB && m_passiveTarget) return m_passiveTarget->GetCurrentHP();
    if (m_targetType == TargetType::MONSTER && m_monsterTarget) return (int)m_monsterTarget->GetHealth();
    if (m_targetType == TargetType::ENEMY_MOB && m_enemyTarget) return m_enemyTarget->GetCurrentHP();
    if (m_targetType == TargetType::WATER_MONSTER && m_waterTarget) return m_waterTarget->GetCurrentHP();
    return 0;
}

int TargetingSystem::GetTargetMaxHP() const {
    if (m_targetType == TargetType::PASSIVE_MOB && m_passiveTarget) return m_passiveTarget->GetMaxHP();
    if (m_targetType == TargetType::MONSTER && m_monsterTarget) return 150;
    if (m_targetType == TargetType::ENEMY_MOB && m_enemyTarget) return m_enemyTarget->GetMaxHP();
    if (m_targetType == TargetType::WATER_MONSTER && m_waterTarget) return m_waterTarget->GetMaxHP();
    return 100;
}

bool TargetingSystem::IsTargetAlive() const {
    if (m_targetType == TargetType::PASSIVE_MOB && m_passiveTarget) return m_passiveTarget->IsAlive();
    if (m_targetType == TargetType::MONSTER && m_monsterTarget) return !m_monsterTarget->IsDead();
    if (m_targetType == TargetType::ENEMY_MOB && m_enemyTarget) return m_enemyTarget->IsAlive();
    if (m_targetType == TargetType::WATER_MONSTER && m_waterTarget) return m_waterTarget->IsAlive();
    return false;
}

void TargetingSystem::Update(float deltaTime, glm::vec3 playerPos, bool manualInputActive) {
    if (manualInputActive) {
        m_autoApproaching = false;
    }

    if (HasTarget()) {
        if (!IsTargetAlive()) {
            m_autoApproaching = false;
        }

        glm::vec3 tPos = GetTargetPosition();
        float dist = glm::distance(glm::vec2(playerPos.x, playerPos.z), glm::vec2(tPos.x, tPos.z));

        // Leash range
        if (dist > 85.0f) {
            ClearTarget();
        }
    }

    m_ringAngle += deltaTime * 2.0f;
}

void TargetingSystem::RenderTargetRing(GLuint shaderProgram) {
    if (!HasTarget() || m_ringVAO == 0) return;

    glm::vec3 targetPos = GetTargetPosition();
    float groundY = WorldGenerator::GetHeight(targetPos.x, targetPos.z) + 0.08f;

    float scale = 1.0f;
    if (m_targetType == TargetType::ENEMY_MOB && m_enemyTarget) {
        scale = m_enemyTarget->GetRadius();
    } else if (m_targetType == TargetType::PASSIVE_MOB && m_passiveTarget) {
        scale = m_passiveTarget->GetRadius();
    } else if (m_targetType == TargetType::WATER_MONSTER && m_waterTarget) {
        scale = m_waterTarget->GetRadius();
    }

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(targetPos.x, groundY, targetPos.z));
    model = glm::rotate(model, m_ringAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(scale));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1);

    glBindVertexArray(m_ringVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_ringVertexCount);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glBindVertexArray(0);
}
