#include "EnemyMob.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

GLuint EnemyMob::s_hpBarVAO = 0;
GLuint EnemyMob::s_hpBarVBO = 0;

void EnemyMob::initHpBarMesh() {
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

void EnemyMob::Render(GLuint shaderProgram) {
    if (m_VAO == 0 || m_vertexCount == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_pos);
    model = glm::rotate(model, m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_scale));

    // Animación de caída al morir
    if (m_state == EnemyState::DEAD) {
        float fallAngle = std::min(m_deathTimer * 90.0f, 90.0f);
        model = glm::rotate(model, glm::radians(fallAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    glBindVertexArray(0);
}

void EnemyMob::RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (m_state == EnemyState::DEAD || m_showHpBarTimer <= 0.0f || s_hpBarVAO == 0) return;

    float hpPct = std::clamp((float)m_currentHp / (float)m_maxHp, 0.0f, 1.0f);
    float barHeightOffset = (m_type == EnemyType::NEUTRAL_GIANT || m_type == EnemyType::TREANT) ? 4.8f : 2.2f;
    glm::vec3 barPos = m_pos + glm::vec3(0.0f, barHeightOffset, 0.0f);

    glm::vec3 toCam = glm::normalize(cameraPos - barPos);
    float yaw = atan2(toCam.x, toCam.z);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");

    // 1. Fondo de la barra de vida (Gris / Rojo oscuro)
    glm::mat4 bgModel = glm::mat4(1.0f);
    bgModel = glm::translate(bgModel, barPos);
    bgModel = glm::rotate(bgModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    bgModel = glm::scale(bgModel, glm::vec3(1.2f, 1.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgModel));

    glBindVertexArray(s_hpBarVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Barra de vida frontal (Rojo brillante)
    if (hpPct > 0.01f) {
        glm::mat4 fgModel = glm::mat4(1.0f);
        fgModel = glm::translate(fgModel, barPos + toCam * 0.01f);
        fgModel = glm::rotate(fgModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        fgModel = glm::translate(fgModel, glm::vec3(-0.6f * (1.0f - hpPct), 0.0f, 0.0f));
        fgModel = glm::scale(fgModel, glm::vec3(1.2f * hpPct, 1.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(fgModel));

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}
