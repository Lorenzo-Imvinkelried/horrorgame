#include "DamageNumberSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

DamageNumberSystem::DamageNumberSystem()
    : m_VAO(0)
    , m_VBO(0)
{
    initMesh();
}

DamageNumberSystem::~DamageNumberSystem() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void DamageNumberSystem::initMesh() {
    if (m_VAO != 0) return;

    // Unit Quad [-0.5, 0.5]
    float quadVertices[] = {
        // Pos             // Color           // UV         // Normal
        -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
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

void DamageNumberSystem::SpawnDamage(glm::vec3 pos, int damage, bool isCrit) {
    FloatingNumber fn;
    float jitterX = (rand()%100/100.0f - 0.5f)*0.2f;
    float jitterZ = (rand()%100/100.0f - 0.5f)*0.2f;
    // Nace directamente en el centro del cuerpo/pecho del mob (Y + 1.0m)
    fn.Pos = pos + glm::vec3(jitterX, 1.0f, jitterZ);
    fn.Velocity = glm::vec3(0.0f, 1.2f, 0.0f);
    fn.Color = isCrit ? glm::vec4(1.0f, 0.88f, 0.10f, 1.0f) : glm::vec4(1.0f, 1.0f, 0.95f, 1.0f);
    fn.Scale = isCrit ? 0.60f : 0.42f;
    fn.Lifetime = isCrit ? 1.4f : 1.1f;
    fn.MaxLifetime = fn.Lifetime;
    fn.Value = damage;
    fn.IsCrit = isCrit;
    fn.IsExp = false;
    fn.IsLevelUp = false;
    fn.IsPlayerDamage = false;
    fn.IsHeal = false;
    fn.IsMana = false;
    m_numbers.push_back(fn);
}

void DamageNumberSystem::SpawnPlayerDamage(glm::vec3 playerPos, int damage, glm::vec3 camFront) {
    FloatingNumber fn;
    glm::vec3 forwardNoY = glm::normalize(glm::vec3(camFront.x, 0.0f, camFront.z));
    if (glm::length(forwardNoY) < 0.001f) forwardNoY = glm::vec3(0, 0, -1);
    glm::vec3 right = glm::normalize(glm::cross(forwardNoY, glm::vec3(0, 1, 0)));
    float sideOffset = ((rand() % 100) / 100.0f - 0.5f) * 0.3f;

    // Nace en el torso del jugador hacia el frente
    fn.Pos = playerPos + forwardNoY * 0.7f + right * sideOffset + glm::vec3(0.0f, 1.1f, 0.0f);
    fn.Velocity = glm::vec3(0.0f, 1.2f, 0.0f);
    fn.Color = glm::vec4(1.0f, 0.12f, 0.12f, 1.0f);
    fn.Scale = 0.50f;
    fn.Lifetime = 1.4f;
    fn.MaxLifetime = fn.Lifetime;
    fn.Value = damage;
    fn.IsCrit = false;
    fn.IsExp = false;
    fn.IsLevelUp = false;
    fn.IsPlayerDamage = true;
    fn.IsHeal = false;
    fn.IsMana = false;
    m_numbers.push_back(fn);
}

void DamageNumberSystem::SpawnHeal(glm::vec3 pos, int amount) {
    FloatingNumber fn;
    fn.Pos = pos + glm::vec3(0.0f, 1.3f, 0.0f);
    fn.Velocity = glm::vec3(0.0f, 1.2f, 0.0f);
    fn.Color = glm::vec4(0.15f, 0.95f, 0.30f, 1.0f); // Verde Esmeralda Radiante
    fn.Scale = 0.54f;
    fn.Lifetime = 1.3f;
    fn.MaxLifetime = fn.Lifetime;
    fn.Value = amount;
    fn.IsCrit = false;
    fn.IsExp = false;
    fn.IsLevelUp = false;
    fn.IsPlayerDamage = false;
    fn.IsHeal = true;
    fn.IsMana = false;
    m_numbers.push_back(fn);
}

void DamageNumberSystem::SpawnMana(glm::vec3 pos, int amount) {
    FloatingNumber fn;
    fn.Pos = pos + glm::vec3(0.0f, 1.3f, 0.0f);
    fn.Velocity = glm::vec3(0.0f, 1.2f, 0.0f);
    fn.Color = glm::vec4(0.20f, 0.65f, 1.0f, 1.0f); // Azul Maná
    fn.Scale = 0.54f;
    fn.Lifetime = 1.3f;
    fn.MaxLifetime = fn.Lifetime;
    fn.Value = amount;
    fn.IsCrit = false;
    fn.IsExp = false;
    fn.IsLevelUp = false;
    fn.IsPlayerDamage = false;
    fn.IsHeal = false;
    fn.IsMana = true;
    m_numbers.push_back(fn);
}

void DamageNumberSystem::SpawnExp(glm::vec3 pos, int exp) {
    FloatingNumber fn;
    fn.Pos = pos + glm::vec3(0.0f, 1.8f, 0.0f);
    fn.Velocity = glm::vec3(0.0f, 2.5f, 0.0f);
    fn.Color = glm::vec4(0.30f, 0.90f, 1.0f, 1.0f); // Cyan EXP glow
    fn.Scale = 0.32f;
    fn.Lifetime = 1.5f;
    fn.MaxLifetime = fn.Lifetime;
    fn.Value = exp;
    fn.IsCrit = false;
    fn.IsExp = true;
    fn.IsLevelUp = false;
    fn.IsPlayerDamage = false;
    fn.IsHeal = false;
    fn.IsMana = false;
    m_numbers.push_back(fn);
}

void DamageNumberSystem::SpawnLevelUp(glm::vec3 pos) {
    FloatingNumber fn;
    fn.Pos = pos + glm::vec3(0.0f, 2.2f, 0.0f);
    fn.Velocity = glm::vec3(0.0f, 1.6f, 0.0f);
    fn.Color = glm::vec4(1.0f, 0.88f, 0.20f, 1.0f); // Golden Level Up Banner
    fn.Scale = 0.65f;
    fn.Lifetime = 2.4f;
    fn.MaxLifetime = fn.Lifetime;
    fn.Value = 0;
    fn.IsCrit = true;
    fn.IsExp = false;
    fn.IsLevelUp = true;
    m_numbers.push_back(fn);
}

void DamageNumberSystem::Update(float deltaTime) {
    for (auto it = m_numbers.begin(); it != m_numbers.end();) {
        it->Lifetime -= deltaTime;
        it->Pos += it->Velocity * deltaTime;
        it->Velocity.y *= 0.94f; // Drag

        if (it->Lifetime <= 0.0f) {
            it = m_numbers.erase(it);
        } else {
            ++it;
        }
    }
}

void DamageNumberSystem::Render(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (m_numbers.empty() || m_VAO == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1); // Skip lighting for floating UI
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_VAO);

    for (const auto& fn : m_numbers) {
        glm::vec3 toCam = glm::normalize(cameraPos - fn.Pos);
        float yaw = atan2(toCam.x, toCam.z);
        float pitch = -asin(toCam.y);

        float alpha = std::clamp(fn.Lifetime / (fn.MaxLifetime * 0.35f), 0.0f, 1.0f);

        if (fn.IsLevelUp) {
            // Giant glowing Golden Level Up Star / Banner
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, fn.Pos);
            model = glm::rotate(model, yaw, glm::vec3(0, 1, 0));
            model = glm::rotate(model, pitch, glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(fn.Scale * 2.2f, fn.Scale * 0.75f, 1.0f));

            glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), alpha);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
            continue;
        }

        // Draw segmented digit boxes
        std::string text = fn.IsExp ? ("+" + std::to_string(fn.Value) + " XP") : (std::to_string(fn.Value));
        float charSpacing = fn.Scale * 0.45f;
        float totalWidth = (float)text.length() * charSpacing;
        glm::vec3 startPos = fn.Pos - glm::vec3(cos(yaw) * totalWidth * 0.5f, 0.0f, -sin(yaw) * totalWidth * 0.5f);

        for (size_t i = 0; i < text.length(); ++i) {
            glm::vec3 charPos = startPos + glm::vec3(cos(yaw) * (i * charSpacing), 0.0f, -sin(yaw) * (i * charSpacing));

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, charPos);
            model = glm::rotate(model, yaw, glm::vec3(0, 1, 0));
            model = glm::rotate(model, pitch, glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(fn.Scale * 0.45f, fn.Scale * 0.65f, 1.0f));

            glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), alpha);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glBindVertexArray(0);
}
