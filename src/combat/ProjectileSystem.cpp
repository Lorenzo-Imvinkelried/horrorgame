#include "ProjectileSystem.h"
#include "Player.h"
#include "WorldGenerator.h"
#include "ModelLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>

ProjectileSystem::ProjectileSystem()
    : m_VAO(0)
    , m_VBO(0)
    , m_vertexCount(0)
{
    initMesh();
}

ProjectileSystem::~ProjectileSystem() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void ProjectileSystem::initMesh() {
    // Glowing magical sphere/gem (composed of low poly intersecting facets)
    std::vector<BoxDef> orbBoxes;
    orbBoxes.push_back({ glm::vec3(0.0f), glm::vec3(0.35f, 0.35f, 0.35f), glm::vec3(0.0f), glm::vec3(1.0f, 0.2f, 0.9f), "CORE" });
    orbBoxes.push_back({ glm::vec3(0.0f), glm::vec3(0.25f, 0.45f, 0.25f), glm::vec3(0.785f, 0.0f, 0.785f), glm::vec3(1.2f, 0.5f, 1.2f), "RING_A" });
    orbBoxes.push_back({ glm::vec3(0.0f), glm::vec3(0.45f, 0.25f, 0.25f), glm::vec3(0.0f, 0.785f, 0.785f), glm::vec3(0.9f, 0.1f, 0.5f), "RING_B" });

    std::vector<float> rawMesh;
    ModelLoader::GenerateMesh(orbBoxes, rawMesh);
    m_vertexCount = rawMesh.size() / 11;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, rawMesh.size() * sizeof(float), rawMesh.data(), GL_STATIC_DRAW);

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

void ProjectileSystem::Spawn(glm::vec3 startPos, glm::vec3 targetPos, float speed, int damage, glm::vec4 color) {
    glm::vec3 toTarget = targetPos - startPos;
    float dist = glm::length(toTarget);
    if (dist < 0.001f) return;
    glm::vec3 dir = toTarget / dist;

    MagicProjectile p;
    p.pos = startPos;
    p.vel = dir * speed;
    p.damage = damage;
    p.color = color;
    p.lifeTimer = 0.0f;
    p.maxLife = 5.0f;
    p.radius = 0.45f;
    p.active = true;

    m_projectiles.push_back(p);
}

void ProjectileSystem::Update(float deltaTime, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers) {
    for (auto it = m_projectiles.begin(); it != m_projectiles.end();) {
        if (!it->active) {
            it = m_projectiles.erase(it);
            continue;
        }

        it->lifeTimer += deltaTime;
        if (it->lifeTimer >= it->maxLife) {
            it->active = false;
            it = m_projectiles.erase(it);
            continue;
        }

        // Advance position
        it->pos += it->vel * deltaTime;

        // Spawn magical arcane particle trail
        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*0.6f, (rand()%100/50.0f - 1.0f)*0.6f, (rand()%100/50.0f - 1.0f)*0.6f);
        particles.SpawnParticle(it->pos, pVel - it->vel * 0.15f, it->color, 0.16f, 0.45f, 0.0f);
        particles.SpawnParticle(it->pos, pVel * 0.4f, glm::vec4(1.0f, 0.8f, 1.0f, 0.8f), 0.10f, 0.35f, 0.0f);

        // Check collision with ground/terrain
        float groundY = WorldGenerator::GetHeight(it->pos.x, it->pos.z);
        if (it->pos.y <= groundY + 0.1f) {
            // Impact explosion
            for (int i = 0; i < 16; ++i) {
                glm::vec3 explodeVel((rand()%100/50.0f - 1.0f)*2.8f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.8f);
                particles.SpawnParticle(it->pos, explodeVel, it->color, 0.14f, 0.6f, -9.8f);
            }
            it->active = false;
            it = m_projectiles.erase(it);
            continue;
        }

        // Check collision with Player
        if (player != nullptr) {
            glm::vec3 playerCenter = player->Position + glm::vec3(0.0f, 1.0f, 0.0f);
            float distToPlayer = glm::distance(it->pos, playerCenter);

            if (distToPlayer < (it->radius + 0.65f)) {
                // Apply player damage
                player->TakeDamage(it->damage, damageNumbers);

                // Burst impact particles
                for (int i = 0; i < 22; ++i) {
                    glm::vec3 explodeVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.2f)*3.5f, (rand()%100/50.0f - 1.0f)*3.5f);
                    particles.SpawnParticle(it->pos, explodeVel, it->color, 0.16f, 0.75f, -9.8f);
                    particles.SpawnParticle(it->pos, explodeVel * 0.5f, glm::vec4(1.0f, 0.9f, 0.2f, 1.0f), 0.12f, 0.5f, -4.0f);
                }

                it->active = false;
                it = m_projectiles.erase(it);
                continue;
            }
        }

        ++it;
    }
}

void ProjectileSystem::Render(GLuint shaderProgram) {
    if (m_projectiles.empty() || m_VAO == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1); // Full emissive brightness

    glBindVertexArray(m_VAO);

    for (const auto& p : m_projectiles) {
        if (!p.active) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, p.pos);
        model = glm::rotate(model, p.lifeTimer * 6.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, p.lifeTimer * 4.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    }

    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glBindVertexArray(0);
}
