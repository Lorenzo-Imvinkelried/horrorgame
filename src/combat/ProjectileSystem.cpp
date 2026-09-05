#include "ProjectileSystem.h"
#include "Player.h"
#include "Monster.h"
#include "PassiveMob.h"
#include "EnemyMob.h"
#include "WaterMonster.h"
#include "entities/Dragon.h"
#include "WorldGenerator.h"
#include "ModelLoader.h"
#include "world/StructureSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>

ProjectileSystem::ProjectileSystem()
    : m_VAO(0)
    , m_VBO(0)
    , m_vertexCount(0)
    , m_arrowVAO(0)
    , m_arrowVBO(0)
    , m_arrowVertexCount(0)
{
    initMesh();
    initArrowMesh();
}

ProjectileSystem::~ProjectileSystem() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_arrowVAO) glDeleteVertexArrays(1, &m_arrowVAO);
    if (m_arrowVBO) glDeleteBuffers(1, &m_arrowVBO);
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

void ProjectileSystem::initArrowMesh() {
    std::vector<BoxDef> arrowBoxes;
    // Wooden Shaft (cuerpo alargado de madera de cedro)
    arrowBoxes.push_back({ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.045f, 0.045f, 0.95f), glm::vec3(0.0f), glm::vec3(0.65f, 0.44f, 0.22f), "Shaft" });
    // Steel Arrowhead (punta afilada de hierro forjado al frente +Z)
    arrowBoxes.push_back({ glm::vec3(0.0f, 0.0f, 0.52f), glm::vec3(0.10f, 0.04f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.85f, 0.90f), "ArrowHead" });
    arrowBoxes.push_back({ glm::vec3(0.0f, 0.0f, 0.58f), glm::vec3(0.04f, 0.04f, 0.12f), glm::vec3(0.0f), glm::vec3(0.95f, 0.95f, 1.00f), "ArrowTip" });
    // Fletching Feathers (plumas de estabilización rojas/blancas en la cola -Z)
    arrowBoxes.push_back({ glm::vec3(0.0f, 0.0f, -0.36f), glm::vec3(0.02f, 0.15f, 0.20f), glm::vec3(0.0f), glm::vec3(0.90f, 0.22f, 0.20f), "FeatherV" });
    arrowBoxes.push_back({ glm::vec3(0.0f, 0.0f, -0.36f), glm::vec3(0.15f, 0.02f, 0.20f), glm::vec3(0.0f), glm::vec3(0.90f, 0.22f, 0.20f), "FeatherH" });

    std::vector<float> rawMesh;
    ModelLoader::GenerateMesh(arrowBoxes, rawMesh);
    m_arrowVertexCount = static_cast<int>(rawMesh.size() / 11);

    glGenVertexArrays(1, &m_arrowVAO);
    glGenBuffers(1, &m_arrowVBO);
    glBindVertexArray(m_arrowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowVBO);
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

void ProjectileSystem::Spawn(glm::vec3 startPos, glm::vec3 targetPos, float speed, int damage, glm::vec4 color, bool isFromPlayer, ProjectileType type) {
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
    p.radius = (type == ProjectileType::ARROW) ? 0.40f : (isFromPlayer ? 0.85f : 0.45f);
    p.active = true;
    p.isFromPlayer = isFromPlayer;
    p.type = type;

    m_projectiles.push_back(p);
}

void ProjectileSystem::Update(float deltaTime, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                             std::vector<std::unique_ptr<Monster>>* monsters,
                             std::vector<std::unique_ptr<PassiveMob>>* passiveMobs,
                             std::vector<std::unique_ptr<EnemyMob>>* enemyMobs,
                             std::vector<std::unique_ptr<WaterMonster>>* waterMonsters,
                             Dragon* dragon) 
{
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
        glm::vec3 prevPos = it->pos;
        it->pos += it->vel * deltaTime;

        // Spawn trail particles
        if (it->type == ProjectileType::ARROW) {
            // Distinct visible flight streak behind arrowhead and red fletching
            particles.SpawnParticle(it->pos, -it->vel * 0.05f, glm::vec4(1.0f, 0.95f, 0.70f, 0.90f), 0.12f, 0.30f, 0.0f);
            particles.SpawnParticle(it->pos, glm::vec3(0.0f), glm::vec4(1.0f, 0.35f, 0.20f, 0.85f), 0.09f, 0.22f, 0.0f);
        } else {
            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*0.6f, (rand()%100/50.0f - 1.0f)*0.6f, (rand()%100/50.0f - 1.0f)*0.6f);
            particles.SpawnParticle(it->pos, pVel - it->vel * 0.15f, it->color, 0.16f, 0.45f, 0.0f);
            particles.SpawnParticle(it->pos, pVel * 0.4f, glm::vec4(1.0f, 0.8f, 1.0f, 0.8f), 0.10f, 0.35f, 0.0f);
        }

        // Check collision with structure stone walls and floors
        if (StructureSystem::Raycast(prevPos, it->pos)) {
            for (int i = 0; i < 16; ++i) {
                glm::vec3 explodeVel((rand()%100/50.0f - 1.0f)*2.8f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.8f);
                particles.SpawnParticle(it->pos, explodeVel, glm::vec4(0.82f, 0.80f, 0.76f, 1.0f), 0.14f, 0.5f, -9.8f);
            }
            it->active = false;
            it = m_projectiles.erase(it);
            continue;
        }

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

        bool hitSomething = false;

        if (it->isFromPlayer) {
            // Check collision with Dragon
            if (dragon != nullptr && dragon->IsAlive() && !dragon->IsDying()) {
                float dist = glm::distance(it->pos, dragon->GetPosition() + glm::vec3(0, 1.5f, 0));
                if (dist < (it->radius + dragon->GetRadius())) {
                    bool killed = dragon->TakeDamage(it->damage, it->pos, particles, damageNumbers, player);
                    damageNumbers.SpawnDamage(dragon->GetPosition() + glm::vec3(0, 2.0f, 0), it->damage, true);
                    if (killed && player != nullptr) {
                        bool lvlUp = false;
                        player->Stats.AddExp(dragon->GetExpReward(), lvlUp);
                        damageNumbers.SpawnExp(dragon->GetPosition() + glm::vec3(0, 2.5f, 0), dragon->GetExpReward());
                        if (lvlUp) damageNumbers.SpawnLevelUp(player->Position);
                    }
                    hitSomething = true;
                }
            }

            // Check collision with EnemyMobs
            if (!hitSomething && enemyMobs != nullptr) {
                for (auto& enemy : *enemyMobs) {
                    if (!enemy->IsAlive()) continue;
                    float dist = glm::distance(it->pos, enemy->GetPosition() + glm::vec3(0, 1.2f, 0));
                    if (dist < (it->radius + enemy->GetRadius())) {
                        bool killed = enemy->TakeDamage(it->damage, it->pos, particles, player, damageNumbers);
                        damageNumbers.SpawnDamage(enemy->GetPosition() + glm::vec3(0, 1.6f, 0), it->damage, false);
                        if (killed && player != nullptr) {
                            bool lvlUp = false;
                            player->Stats.AddExp(enemy->GetExpReward(), lvlUp);
                            damageNumbers.SpawnExp(enemy->GetPosition() + glm::vec3(0, 1.8f, 0), enemy->GetExpReward());
                            if (lvlUp) damageNumbers.SpawnLevelUp(player->Position);
                        }
                        hitSomething = true;
                        break;
                    }
                }
            }

            // Check collision with Monsters
            if (!hitSomething && monsters != nullptr) {
                for (auto& m : *monsters) {
                    if (m->IsDead()) continue;
                    float dist = glm::distance(it->pos, m->GetPosition() + glm::vec3(0, 1.4f, 0));
                    if (dist < (it->radius + 1.8f)) {
                        m->TakeDamage((float)it->damage, false);
                        damageNumbers.SpawnDamage(m->GetPosition() + glm::vec3(0, 1.6f, 0), it->damage, false);
                        if (m->IsDead() && player != nullptr) {
                            bool lvlUp = false;
                            player->Stats.AddExp(85, lvlUp);
                            damageNumbers.SpawnExp(m->GetPosition() + glm::vec3(0, 1.8f, 0), 85);
                            if (lvlUp) damageNumbers.SpawnLevelUp(player->Position);
                        }
                        hitSomething = true;
                        break;
                    }
                }
            }

            // Check collision with WaterMonsters
            if (!hitSomething && waterMonsters != nullptr) {
                for (auto& wm : *waterMonsters) {
                    if (!wm->IsAlive()) continue;
                    float dist = glm::distance(it->pos, wm->GetPosition() + glm::vec3(0, 1.0f, 0));
                    if (dist < (it->radius + wm->GetRadius())) {
                        bool killed = wm->TakeDamage(it->damage, it->pos, particles, player, damageNumbers);
                        damageNumbers.SpawnDamage(wm->GetPosition() + glm::vec3(0, 1.5f, 0), it->damage, false);
                        if (killed && player != nullptr) {
                            bool lvlUp = false;
                            player->Stats.AddExp(wm->GetExpReward(), lvlUp);
                            damageNumbers.SpawnExp(wm->GetPosition() + glm::vec3(0, 1.7f, 0), wm->GetExpReward());
                            if (lvlUp) damageNumbers.SpawnLevelUp(player->Position);
                        }
                        hitSomething = true;
                        break;
                    }
                }
            }

            // Check collision with PassiveMobs
            if (!hitSomething && passiveMobs != nullptr) {
                for (auto& deer : *passiveMobs) {
                    if (!deer->IsAlive()) continue;
                    float dist = glm::distance(it->pos, deer->GetPosition() + glm::vec3(0, 1.0f, 0));
                    if (dist < (it->radius + deer->GetRadius())) {
                        bool killed = deer->TakeDamage(it->damage, it->pos, particles, player, damageNumbers);
                        damageNumbers.SpawnDamage(deer->GetPosition() + glm::vec3(0, 1.4f, 0), it->damage, false);
                        if (killed && player != nullptr) {
                            bool lvlUp = false;
                            player->Stats.AddExp(deer->GetExpReward(), lvlUp);
                            damageNumbers.SpawnExp(deer->GetPosition() + glm::vec3(0, 1.6f, 0), deer->GetExpReward());
                            if (lvlUp) damageNumbers.SpawnLevelUp(player->Position);
                        }
                        hitSomething = true;
                        break;
                    }
                }
            }

            if (hitSomething) {
                for (int i = 0; i < 20; ++i) {
                    glm::vec3 explodeVel((rand()%100/50.0f - 1.0f)*3.2f, (rand()%100/50.0f + 0.3f)*3.4f, (rand()%100/50.0f - 1.0f)*3.2f);
                    particles.SpawnParticle(it->pos, explodeVel, it->color, 0.16f, 0.7f, -8.0f);
                }
                it->active = false;
                it = m_projectiles.erase(it);
                continue;
            }
        } else {
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
        }

        ++it;
    }
}

void ProjectileSystem::Render(GLuint shaderProgram) {
    if (m_projectiles.empty()) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    // CRITICAL: u_IsDebug = 3 instructs ps1.frag to passthrough vertex colors without texture discard
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3);

    // 1. Render Magic Orbs (Spinning Emissive)
    if (m_VAO != 0) {
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1);
        glBindVertexArray(m_VAO);
        for (const auto& p : m_projectiles) {
            if (!p.active || p.type == ProjectileType::ARROW) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, p.pos);
            model = glm::rotate(model, p.lifeTimer * 6.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, p.lifeTimer * 4.0f, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(p.type == ProjectileType::FIREBALL ? 1.8f : 1.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
        }
    }

    // 2. Render Arrows (Directionally Aligned Flying Mesh)
    if (m_arrowVAO != 0) {
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
        glBindVertexArray(m_arrowVAO);
        for (const auto& p : m_projectiles) {
            if (!p.active || p.type != ProjectileType::ARROW) continue;

            glm::vec3 vDir = glm::length(p.vel) > 0.001f ? glm::normalize(p.vel) : glm::vec3(0, 0, 1);
            float yaw = atan2(vDir.x, vDir.z);
            float pitch = -asin(std::clamp(vDir.y, -0.999f, 0.999f));

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, p.pos);
            model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(2.0f)); // Clear, prominent size in flight

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_arrowVertexCount);
        }
    }

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glBindVertexArray(0);
}
