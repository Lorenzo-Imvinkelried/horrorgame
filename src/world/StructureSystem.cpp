#include "StructureSystem.h"
#include "Player.h"
#include "inventory/InventorySystem.h"
#include "inventory/LootManager.h"
#include "combat/DamageNumberSystem.h"
#include "ParticleSystem.h"
#include "ModelLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

StructureSystem::StructureSystem() {
    InitMesh();
    GenerateStructures();
}

StructureSystem::~StructureSystem() {
    if (m_pillarVAO) glDeleteVertexArrays(1, &m_pillarVAO);
    if (m_pillarVBO) glDeleteBuffers(1, &m_pillarVBO);
    if (m_chestVAO) glDeleteVertexArrays(1, &m_chestVAO);
    if (m_chestVBO) glDeleteBuffers(1, &m_chestVBO);
    if (m_altarVAO) glDeleteVertexArrays(1, &m_altarVAO);
    if (m_altarVBO) glDeleteBuffers(1, &m_altarVBO);
}

void StructureSystem::InitMesh() {
    // 1. Pillar / Ruin Arch Mesh
    std::vector<BoxDef> pillarBoxes;
    pillarBoxes.push_back({ glm::vec3(-2.0f, 2.0f, -2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar1" });
    pillarBoxes.push_back({ glm::vec3( 2.0f, 2.0f, -2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar2" });
    pillarBoxes.push_back({ glm::vec3(-2.0f, 2.0f,  2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar3" });
    pillarBoxes.push_back({ glm::vec3( 2.0f, 2.0f,  2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar4" });
    pillarBoxes.push_back({ glm::vec3(0.0f, 4.2f, -2.0f), glm::vec3(4.8f, 0.6f, 0.8f), glm::vec3(0.0f), glm::vec3(0.42f, 0.42f, 0.45f), "Arch1" });
    pillarBoxes.push_back({ glm::vec3(0.0f, 4.2f,  2.0f), glm::vec3(4.8f, 0.6f, 0.8f), glm::vec3(0.0f), glm::vec3(0.42f, 0.42f, 0.45f), "Arch2" });
    pillarBoxes.push_back({ glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(5.5f, 0.2f, 5.5f), glm::vec3(0.0f), glm::vec3(0.35f, 0.35f, 0.38f), "Base" });

    std::vector<float> pillarVerts;
    ModelLoader::GenerateMesh(pillarBoxes, pillarVerts);
    m_pillarVertexCount = pillarVerts.size() / 11;

    glGenVertexArrays(1, &m_pillarVAO);
    glGenBuffers(1, &m_pillarVBO);
    glBindVertexArray(m_pillarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pillarVBO);
    glBufferData(GL_ARRAY_BUFFER, pillarVerts.size() * sizeof(float), pillarVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    // 2. Chest Mesh
    std::vector<BoxDef> chestBoxes;
    chestBoxes.push_back({ glm::vec3(0.0f, 0.35f, 0.0f), glm::vec3(1.1f, 0.7f, 0.75f), glm::vec3(0.0f), glm::vec3(0.38f, 0.22f, 0.12f), "ChestBody" });
    chestBoxes.push_back({ glm::vec3(0.0f, 0.75f, 0.0f), glm::vec3(1.15f, 0.2f, 0.80f), glm::vec3(0.0f), glm::vec3(0.28f, 0.16f, 0.08f), "ChestLid" });
    chestBoxes.push_back({ glm::vec3(0.0f, 0.40f, 0.40f), glm::vec3(0.18f, 0.22f, 0.08f), glm::vec3(0.0f), glm::vec3(0.85f, 0.75f, 0.20f), "ChestLock" });

    std::vector<float> chestVerts;
    ModelLoader::GenerateMesh(chestBoxes, chestVerts);
    m_chestVertexCount = chestVerts.size() / 11;

    glGenVertexArrays(1, &m_chestVAO);
    glGenBuffers(1, &m_chestVBO);
    glBindVertexArray(m_chestVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_chestVBO);
    glBufferData(GL_ARRAY_BUFFER, chestVerts.size() * sizeof(float), chestVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    // 3. Sacrifice Altar Mesh
    std::vector<BoxDef> altarBoxes;
    altarBoxes.push_back({ glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.6f, 1.0f, 1.6f), glm::vec3(0.0f), glm::vec3(0.18f, 0.14f, 0.20f), "AltarBase" });
    altarBoxes.push_back({ glm::vec3(0.0f, 1.05f, 0.0f), glm::vec3(1.9f, 0.18f, 1.9f), glm::vec3(0.0f), glm::vec3(0.70f, 0.12f, 0.12f), "AltarBowl" });
    altarBoxes.push_back({ glm::vec3(-0.9f, 1.4f, -0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch1" });
    altarBoxes.push_back({ glm::vec3( 0.9f, 1.4f, -0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch2" });
    altarBoxes.push_back({ glm::vec3(-0.9f, 1.4f,  0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch3" });
    altarBoxes.push_back({ glm::vec3( 0.9f, 1.4f,  0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch4" });

    std::vector<float> altarVerts;
    ModelLoader::GenerateMesh(altarBoxes, altarVerts);
    m_altarVertexCount = altarVerts.size() / 11;

    glGenVertexArrays(1, &m_altarVAO);
    glGenBuffers(1, &m_altarVBO);
    glBindVertexArray(m_altarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_altarVBO);
    glBufferData(GL_ARRAY_BUFFER, altarVerts.size() * sizeof(float), altarVerts.data(), GL_STATIC_DRAW);
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

void StructureSystem::GenerateStructures() {
    m_structures.clear();

    // 1. Starting Ruin with Chest
    float y1 = WorldGenerator::GetHeight(22.0f, 26.0f);
    m_structures.push_back({ StructureType::ANCIENT_RUINS_CHEST, glm::vec3(22.0f, y1, 26.0f), 0.3f, false });

    // 2. Sacrifice Blood Altar (East clearing)
    float y2 = WorldGenerator::GetHeight(48.0f, -35.0f);
    m_structures.push_back({ StructureType::SACRIFICE_ALTAR, glm::vec3(48.0f, y2, -35.0f), 0.8f, false });

    // 3. Deep Forest Ruins with Ancient Chest
    float y3 = WorldGenerator::GetHeight(-55.0f, 45.0f);
    m_structures.push_back({ StructureType::ANCIENT_RUINS_CHEST, glm::vec3(-55.0f, y3, 45.0f), 1.2f, false });

    // 4. Northern Dark Altar
    float y4 = WorldGenerator::GetHeight(-65.0f, -60.0f);
    m_structures.push_back({ StructureType::SACRIFICE_ALTAR, glm::vec3(-65.0f, y4, -60.0f), 2.1f, false });

    // 5. Far Eastern Citadel Ruins
    float y5 = WorldGenerator::GetHeight(85.0f, 75.0f);
    m_structures.push_back({ StructureType::ANCIENT_RUINS_CHEST, glm::vec3(85.0f, y5, 75.0f), 0.0f, false });
}

void StructureSystem::Render(GLuint shaderProgram, glm::vec3 cameraPos) {
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);

    for (const auto& s : m_structures) {
        float dist = glm::distance(cameraPos, s.pos);
        if (dist > 95.0f) continue;

        glm::vec3 renderPos = s.pos;
        renderPos.y = WorldGenerator::GetHeight(renderPos.x, renderPos.z);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPos);
        model = glm::rotate(model, s.yaw, glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));

        if (s.type == StructureType::ANCIENT_RUINS_CHEST) {
            // Render Ruins Pillars
            glBindVertexArray(m_pillarVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_pillarVertexCount);

            // Render Center Chest
            if (!s.looted) {
                glBindVertexArray(m_chestVAO);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_chestVertexCount);
            }
        } else if (s.type == StructureType::SACRIFICE_ALTAR) {
            // Render Sacrifice Altar
            glBindVertexArray(m_altarVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_altarVertexCount);
        }
    }
    glBindVertexArray(0);
}

std::string StructureSystem::GetPrompt(glm::vec3 playerPos) {
    for (const auto& s : m_structures) {
        if (s.looted) continue;

        float dist = glm::distance(playerPos, s.pos);
        if (dist < 3.2f) {
            if (s.type == StructureType::ANCIENT_RUINS_CHEST) {
                return "[E] ABRIR COFRE ANCESTRAL";
            } else if (s.type == StructureType::SACRIFICE_ALTAR) {
                return "[E] OFRENDAR SANGRE (+1 PTO STAT)";
            }
        }
    }
    return "";
}

#include "world/ItemDropSystem.h"

bool StructureSystem::TryInteract(glm::vec3 playerPos, 
                                  Player& player, 
                                  InventorySystem& inventory, 
                                  ItemDropSystem& itemDropSystem,
                                  DamageNumberSystem& damageNumbers, 
                                  ParticleSystem& particles) 
{
    for (auto& s : m_structures) {
        if (s.looted) continue;

        float dist = glm::distance(playerPos, s.pos);
        if (dist < 3.2f) {
            s.looted = true;

            if (s.type == StructureType::ANCIENT_RUINS_CHEST) {
                // Chest Loot basado en datos y ruleta ponderada - se dropea como bolsas de botín
                LootTable chestLoot = LootManager::GetChestLoot(1);
                std::vector<ItemInstance> drops = chestLoot.GenerateLoot(1.0f, 1);
                itemDropSystem.SpawnDrops(drops, s.pos + glm::vec3(0.0f, 0.6f, 0.0f));

                bool lvlUp = false;
                player.Stats.AddExp(80, lvlUp);
                damageNumbers.SpawnExp(s.pos + glm::vec3(0, 1.6f, 0), 80);
                if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);

                // Golden sparkles
                for (int i = 0; i < 30; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.5f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
                    particles.SpawnParticle(s.pos + glm::vec3(0, 0.6f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.18f, 1.0f, -8.0f);
                }
                return true;
            } else if (s.type == StructureType::SACRIFICE_ALTAR) {
                // Sacrifice Altar (+1 Permanent Stat Point)
                player.Stats.CurrentHP = std::max(5, player.Stats.CurrentHP - 25);
                player.Stats.AvailableStatPoints += 1;

                damageNumbers.SpawnDamage(player.Position + glm::vec3(0, 1.4f, 0), 25, false);

                // Blood magic explosion
                for (int i = 0; i < 45; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.2f, (rand()%100/50.0f + 0.8f)*4.0f, (rand()%100/50.0f - 1.0f)*3.2f);
                    particles.SpawnParticle(s.pos + glm::vec3(0, 1.1f, 0), pVel, glm::vec4(0.85f, 0.10f, 0.10f, 1.0f), 0.22f, 1.2f, -8.0f);
                }
                return true;
            }
        }
    }
    return false;
}
