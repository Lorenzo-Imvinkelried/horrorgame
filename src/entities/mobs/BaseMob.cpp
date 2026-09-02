#include "BaseMob.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

GLuint BaseMob::s_hpBarVAO = 0;
GLuint BaseMob::s_hpBarVBO = 0;

BaseMob::BaseMob(glm::vec3 spawnPos, const std::string& name)
    : m_pos(spawnPos)
    , m_name(name)
{
    initHpBarMesh();
}

BaseMob::~BaseMob() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void BaseMob::initHpBarMesh() {
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

void BaseMob::RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (!IsAlive() || m_showHpBarTimer <= 0.0f || s_hpBarVAO == 0) return;

    float hpPct = std::clamp((float)m_currentHp / (float)m_maxHp, 0.0f, 1.0f);
    glm::vec3 barPos = m_pos + glm::vec3(0.0f, 2.2f * m_scale, 0.0f);

    glm::vec3 toCam = glm::normalize(cameraPos - barPos);
    float yaw = atan2(toCam.x, toCam.z);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");

    // 1. Fondo de la barra (Gris oscuro)
    glm::mat4 bgModel = glm::mat4(1.0f);
    bgModel = glm::translate(bgModel, barPos);
    bgModel = glm::rotate(bgModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    bgModel = glm::scale(bgModel, glm::vec3(1.2f, 1.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgModel));

    glBindVertexArray(s_hpBarVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Barra de vida frontal (Rojo brillante)
    if (hpPct > 0.001f) {
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
