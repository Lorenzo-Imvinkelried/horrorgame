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

StructureSystem* StructureSystem::s_instance = nullptr;

StructureSystem::StructureSystem() {
    s_instance = this;
    InitMesh();
    GenerateStructures();
}

StructureSystem::~StructureSystem() {
    if (s_instance == this) s_instance = nullptr;
    if (m_pillarVAO) glDeleteVertexArrays(1, &m_pillarVAO);
    if (m_pillarVBO) glDeleteBuffers(1, &m_pillarVBO);
    if (m_chestVAO) glDeleteVertexArrays(1, &m_chestVAO);
    if (m_chestVBO) glDeleteBuffers(1, &m_chestVBO);
    if (m_altarVAO) glDeleteVertexArrays(1, &m_altarVAO);
    if (m_altarVBO) glDeleteBuffers(1, &m_altarVBO);
    if (m_towerVAO) glDeleteVertexArrays(1, &m_towerVAO);
    if (m_towerVBO) glDeleteBuffers(1, &m_towerVBO);
}

void StructureSystem::InitMesh() {
    // 1. Pillar / Ruin Arch Mesh
    std::vector<BoxDef> pillarBoxes = ModelLoader::Load("assets/models/structures/ruin_arch.txt");
    if (pillarBoxes.empty()) {
        pillarBoxes.push_back({ glm::vec3(-2.0f, 2.0f, -2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar1" });
        pillarBoxes.push_back({ glm::vec3( 2.0f, 2.0f, -2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar2" });
        pillarBoxes.push_back({ glm::vec3(-2.0f, 2.0f,  2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar3" });
        pillarBoxes.push_back({ glm::vec3( 2.0f, 2.0f,  2.0f), glm::vec3(0.7f, 4.0f, 0.7f), glm::vec3(0.0f), glm::vec3(0.48f, 0.48f, 0.50f), "Pillar4" });
        pillarBoxes.push_back({ glm::vec3(0.0f, 4.2f, -2.0f), glm::vec3(4.8f, 0.6f, 0.8f), glm::vec3(0.0f), glm::vec3(0.42f, 0.42f, 0.45f), "Arch1" });
        pillarBoxes.push_back({ glm::vec3(0.0f, 4.2f,  2.0f), glm::vec3(4.8f, 0.6f, 0.8f), glm::vec3(0.0f), glm::vec3(0.42f, 0.42f, 0.45f), "Arch2" });
        pillarBoxes.push_back({ glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(5.5f, 0.2f, 5.5f), glm::vec3(0.0f), glm::vec3(0.35f, 0.35f, 0.38f), "Base" });
    }

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
    std::vector<BoxDef> chestBoxes = ModelLoader::Load("assets/models/structures/ancient_chest.txt");
    if (chestBoxes.empty()) {
        chestBoxes.push_back({ glm::vec3(0.0f, 0.35f, 0.0f), glm::vec3(1.1f, 0.7f, 0.75f), glm::vec3(0.0f), glm::vec3(0.38f, 0.22f, 0.12f), "ChestBody" });
        chestBoxes.push_back({ glm::vec3(0.0f, 0.75f, 0.0f), glm::vec3(1.15f, 0.2f, 0.80f), glm::vec3(0.0f), glm::vec3(0.28f, 0.16f, 0.08f), "ChestLid" });
        chestBoxes.push_back({ glm::vec3(0.0f, 0.40f, 0.40f), glm::vec3(0.18f, 0.22f, 0.08f), glm::vec3(0.0f), glm::vec3(0.85f, 0.75f, 0.20f), "ChestLock" });
    }

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
    std::vector<BoxDef> altarBoxes = ModelLoader::Load("assets/models/structures/sacrifice_altar.txt");
    if (altarBoxes.empty()) {
        altarBoxes.push_back({ glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.6f, 1.0f, 1.6f), glm::vec3(0.0f), glm::vec3(0.18f, 0.14f, 0.20f), "AltarBase" });
        altarBoxes.push_back({ glm::vec3(0.0f, 1.05f, 0.0f), glm::vec3(1.9f, 0.18f, 1.9f), glm::vec3(0.0f), glm::vec3(0.70f, 0.12f, 0.12f), "AltarBowl" });
        altarBoxes.push_back({ glm::vec3(-0.9f, 1.4f, -0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch1" });
        altarBoxes.push_back({ glm::vec3( 0.9f, 1.4f, -0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch2" });
        altarBoxes.push_back({ glm::vec3(-0.9f, 1.4f,  0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch3" });
        altarBoxes.push_back({ glm::vec3( 0.9f, 1.4f,  0.9f), glm::vec3(0.18f, 0.8f, 0.18f), glm::vec3(0.0f), glm::vec3(0.85f, 0.35f, 0.10f), "Torch4" });
    }

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

    // 4. Ancient Tower Mesh (Gran Torre de Piedra con escaleras y almenas)
    std::vector<BoxDef> towerBoxes = ModelLoader::Load("assets/models/structures/ancient_tower.txt");
    if (!towerBoxes.empty()) {
        std::vector<float> towerVerts;
        ModelLoader::GenerateMesh(towerBoxes, towerVerts);
        m_towerVertexCount = towerVerts.size() / 11;

        glGenVertexArrays(1, &m_towerVAO);
        glGenBuffers(1, &m_towerVBO);
        glBindVertexArray(m_towerVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_towerVBO);
        glBufferData(GL_ARRAY_BUFFER, towerVerts.size() * sizeof(float), towerVerts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
    }

    glBindVertexArray(0);
}

void StructureSystem::GenerateStructures(glm::vec3 playerPos) {
    m_structures.clear();

    // 1. Santuarios y Ruinas ancestrales
    float y1 = WorldGenerator::GetHeight(22.0f, 26.0f);
    m_structures.push_back({ StructureType::ANCIENT_RUINS_CHEST, glm::vec3(22.0f, y1, 26.0f), 0.3f, false });

    float y2 = WorldGenerator::GetHeight(48.0f, -35.0f);
    m_structures.push_back({ StructureType::SACRIFICE_ALTAR, glm::vec3(48.0f, y2, -35.0f), 0.8f, false });

    float y3 = WorldGenerator::GetHeight(-55.0f, 45.0f);
    m_structures.push_back({ StructureType::ANCIENT_RUINS_CHEST, glm::vec3(-55.0f, y3, 45.0f), 1.2f, false });

    float y4 = WorldGenerator::GetHeight(-65.0f, -60.0f);
    m_structures.push_back({ StructureType::SACRIFICE_ALTAR, glm::vec3(-65.0f, y4, -60.0f), 2.1f, false });

    float y5 = WorldGenerator::GetHeight(85.0f, 75.0f);
    m_structures.push_back({ StructureType::ANCIENT_RUINS_CHEST, glm::vec3(85.0f, y5, 75.0f), 0.0f, false });

    // 2. TORRES ANCESTRALES GENERADAS PROCEDURALMENTE
    // A) Torre Garantizada cercana al jugador (a 45 - 55m de distancia, en tierra firme)
    float baseAngle = (float)(rand() % 360) * 0.01745329f;
    float distNear = 48.0f;
    float tNearX = playerPos.x + cos(baseAngle) * distNear;
    float tNearZ = playerPos.z + sin(baseAngle) * distNear;
    float tNearY = WorldGenerator::GetHeight(tNearX, tNearZ);

    for (int a = 0; a < 12 && (tNearY < Config::Water::Level + 1.2f || tNearY > 52.0f); ++a) {
        baseAngle += 0.5235f; // girar 30 grados
        tNearX = playerPos.x + cos(baseAngle) * distNear;
        tNearZ = playerPos.z + sin(baseAngle) * distNear;
        tNearY = WorldGenerator::GetHeight(tNearX, tNearZ);
    }
    m_structures.push_back({ StructureType::ANCIENT_TOWER, glm::vec3(tNearX, tNearY, tNearZ), 0.0f, false });
    std::cout << "[StructureSystem] Torre cercana generada en (" << tNearX << ", " << tNearY << ", " << tNearZ 
              << ") a distancia " << glm::distance(glm::vec2(playerPos.x, playerPos.z), glm::vec2(tNearX, tNearZ)) << "m" << std::endl;

    // B) Torres distribuidas proceduralmente en los distintos cuadrantes del mapa
    struct TowerRegion { float minX, maxX, minZ, maxZ; };
    std::vector<TowerRegion> regions = {
        { -240.0f, -90.0f,   90.0f,  240.0f }, // Noroeste
        {   90.0f,  240.0f,  90.0f,  240.0f }, // Noreste
        { -240.0f, -90.0f,  -240.0f, -90.0f }, // Suroeste
        {   90.0f,  240.0f, -240.0f, -90.0f }, // Sureste
        { -340.0f,  340.0f, -340.0f, 340.0f }  // Tierras remotas
    };

    for (const auto& reg : regions) {
        for (int attempts = 0; attempts < 15; ++attempts) {
            float rx = reg.minX + (float)(rand() % 1000) / 1000.0f * (reg.maxX - reg.minX);
            float rz = reg.minZ + (float)(rand() % 1000) / 1000.0f * (reg.maxZ - reg.minZ);
            float ry = WorldGenerator::GetHeight(rx, rz);

            if (ry < Config::Water::Level + 1.2f || ry > 52.0f) continue;

            bool tooClose = false;
            for (const auto& s : m_structures) {
                if (s.type == StructureType::ANCIENT_TOWER && glm::distance(glm::vec2(rx, rz), glm::vec2(s.pos.x, s.pos.z)) < 75.0f) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) continue;

            float yaw = (float)(rand() % 4) * 1.5707963f;
            m_structures.push_back({ StructureType::ANCIENT_TOWER, glm::vec3(rx, ry, rz), yaw, false });
            break;
        }
    }
}

bool StructureSystem::IsNearStructure(float x, float z, float customDist) {
    if (!s_instance) return false;

    for (const auto& s : s_instance->m_structures) {
        float dx = x - s.pos.x;
        float dz = z - s.pos.z;
        float distSq = dx * dx + dz * dz;

        float threshold = customDist;
        if (threshold <= 0.0f) {
            if (s.type == StructureType::ANCIENT_TOWER) threshold = 14.5f; // Despeje total de 14.5m para que los arboles no crezcan adentro
            else if (s.type == StructureType::ANCIENT_RUINS_CHEST) threshold = 7.0f;
            else if (s.type == StructureType::SACRIFICE_ALTAR) threshold = 6.0f;
            else threshold = 8.0f;
        }

        if (distSq < threshold * threshold) {
            return true;
        }
    }
    return false;
}

void StructureSystem::Render(GLuint shaderProgram, glm::vec3 cameraPos) {
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);

    for (const auto& s : m_structures) {
        float dist = glm::distance(cameraPos, s.pos);
        // Distancia de visibilidad aumentada a 175m para la Gran Torre (visible en el horizonte/niebla)
        float maxDist = (s.type == StructureType::ANCIENT_TOWER) ? 175.0f : 115.0f;
        if (dist > maxDist) continue;

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
        } else if (s.type == StructureType::ANCIENT_TOWER) {
            // Render Gran Torre Ancestral
            if (m_towerVAO != 0) {
                glBindVertexArray(m_towerVAO);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_towerVertexCount);
            }

            // Render Cofre de la Torre en la cima (pedestal dorado en Y = +14.45m)
            if (!s.looted && m_chestVAO != 0) {
                glm::mat4 chestModel = glm::translate(model, glm::vec3(0.0f, 14.55f, 0.0f));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(chestModel));
                glBindVertexArray(m_chestVAO);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_chestVertexCount);
            }
        }
    }
    glBindVertexArray(0);
}

std::string StructureSystem::GetPrompt(glm::vec3 playerPos) {
    for (const auto& s : m_structures) {
        if (s.looted) continue;

        if (s.type == StructureType::ANCIENT_RUINS_CHEST) {
            if (glm::distance(playerPos, s.pos) < 3.2f) {
                return "[E] ABRIR COFRE ANCESTRAL";
            }
        } else if (s.type == StructureType::SACRIFICE_ALTAR) {
            if (glm::distance(playerPos, s.pos) < 3.2f) {
                return "[E] OFRENDAR SANGRE (+1 PTO STAT)";
            }
        } else if (s.type == StructureType::ANCIENT_TOWER) {
            glm::vec3 chestPos = s.pos + glm::vec3(0.0f, 14.55f, 0.0f);
            chestPos.y = WorldGenerator::GetHeight(s.pos.x, s.pos.z) + 14.55f;
            if (glm::distance(playerPos, chestPos) < 3.2f) {
                return "[E] ABRIR GRAN COFRE DE LA TORRE";
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

        if (s.type == StructureType::ANCIENT_RUINS_CHEST) {
            if (glm::distance(playerPos, s.pos) < 3.2f) {
                s.looted = true;

                LootTable chestLoot = LootManager::GetChestLoot(1);
                std::vector<ItemInstance> drops = chestLoot.GenerateLoot(1.0f, 1);
                itemDropSystem.SpawnDrops(drops, s.pos + glm::vec3(0.0f, 0.6f, 0.0f));

                bool lvlUp = false;
                player.Stats.AddExp(80, lvlUp);
                damageNumbers.SpawnExp(s.pos + glm::vec3(0, 1.6f, 0), 80);
                if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);

                for (int i = 0; i < 30; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.5f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
                    particles.SpawnParticle(s.pos + glm::vec3(0, 0.6f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.18f, 1.0f, -8.0f);
                }
                return true;
            }
        } else if (s.type == StructureType::SACRIFICE_ALTAR) {
            if (glm::distance(playerPos, s.pos) < 3.2f) {
                s.looted = true;

                player.Stats.CurrentHP = std::max(5, player.Stats.CurrentHP - 25);
                player.Stats.AvailableStatPoints += 1;

                damageNumbers.SpawnDamage(player.Position + glm::vec3(0, 1.4f, 0), 25, false);

                for (int i = 0; i < 45; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.2f, (rand()%100/50.0f + 0.8f)*4.0f, (rand()%100/50.0f - 1.0f)*3.2f);
                    particles.SpawnParticle(s.pos + glm::vec3(0, 1.1f, 0), pVel, glm::vec4(0.85f, 0.10f, 0.10f, 1.0f), 0.22f, 1.2f, -8.0f);
                }
                return true;
            }
        } else if (s.type == StructureType::ANCIENT_TOWER) {
            glm::vec3 chestPos = s.pos + glm::vec3(0.0f, 14.55f, 0.0f);
            chestPos.y = WorldGenerator::GetHeight(s.pos.x, s.pos.z) + 14.55f;

            if (glm::distance(playerPos, chestPos) < 3.2f) {
                s.looted = true;

                // Botín Superior de la Torre Ancestral (Tier 2 con mejores armas/armaduras)
                LootTable towerLoot = LootManager::GetChestLoot(2);
                std::vector<ItemInstance> drops = towerLoot.GenerateLoot(1.35f, 2);
                itemDropSystem.SpawnDrops(drops, chestPos + glm::vec3(0.0f, 0.6f, 0.0f));

                bool lvlUp = false;
                player.Stats.AddExp(240, lvlUp);
                damageNumbers.SpawnExp(chestPos + glm::vec3(0, 1.8f, 0), 240);
                if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);

                // Celebración de partículas doradas y celestes en la cima
                for (int i = 0; i < 50; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.6f)*4.8f, (rand()%100/50.0f - 1.0f)*4.0f);
                    particles.SpawnParticle(chestPos + glm::vec3(0, 0.6f, 0), pVel, glm::vec4(1.0f, 0.85f, 0.20f, 1.0f), 0.22f, 1.4f, -7.0f);
                }
                for (int i = 0; i < 30; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.4f)*4.0f, (rand()%100/50.0f - 1.0f)*3.5f);
                    particles.SpawnParticle(chestPos + glm::vec3(0, 0.6f, 0), pVel, glm::vec4(0.35f, 0.85f, 1.0f, 1.0f), 0.18f, 1.2f, -7.0f);
                }
                return true;
            }
        }
    }
    return false;
}

float StructureSystem::GetWalkableHeight(float x, float z, float currentFeetY, float defaultTerrainY) {
    if (!s_instance) return defaultTerrainY;

    float bestY = defaultTerrainY;

    for (const auto& s : s_instance->m_structures) {
        if (s.type != StructureType::ANCIENT_TOWER) continue;

        float baseY = s.pos.y;
        float dx = x - s.pos.x;
        float dz = z - s.pos.z;

        if (dx * dx + dz * dz > 144.0f) continue; // fuera de radio de 12m

        // Transformación al espacio local unificado de la torre según su rotación yaw
        float rad = -s.yaw;
        float cosY = cos(rad);
        float sinY = sin(rad);
        float localX = dx * cosY - dz * sinY;
        float localZ = dx * sinY + dz * cosY;

        if (std::abs(localX) > 7.5f || std::abs(localZ) > 7.5f) continue;

        std::vector<float> candidates;

        // 1. Suelo base de la plaza baja (Y = +0.40m)
        if (std::abs(localX) <= 6.2f && std::abs(localZ) <= 6.2f) {
            candidates.push_back(baseY + 0.40f);
        }

        // 2. Tramo 1 de escaleras (Lado Este: localX en [2.6, 6.0], localZ en [-4.6, 5.0])
        if (localX >= 2.6f && localX <= 6.0f && localZ >= -4.6f && localZ <= 5.0f) {
            float t = (5.0f - localZ) / 9.6f;
            t = glm::clamp(t, 0.0f, 1.0f);
            candidates.push_back(baseY + 0.40f + t * 4.10f);
        }

        // 3. Descansillo 1 (Noreste: Y = +4.50m)
        if (localX >= 2.2f && localX <= 6.0f && localZ >= -6.0f && localZ <= -2.2f) {
            candidates.push_back(baseY + 4.50f);
        }

        // 4. Tramo 2 de escaleras (Lado Norte: localZ en [-6.0, -2.4], localX en [-4.8, 2.5])
        if (localZ >= -6.0f && localZ <= -2.4f && localX >= -4.8f && localX <= 2.5f) {
            float t = (2.5f - localX) / 7.2f;
            t = glm::clamp(t, 0.0f, 1.0f);
            candidates.push_back(baseY + 4.50f + t * 4.50f);
        }

        // 5. Descansillo 2 (Noroeste: Y = +9.00m)
        if (localX >= -6.0f && localX <= -2.2f && localZ >= -6.0f && localZ <= -2.2f) {
            candidates.push_back(baseY + 9.00f);
        }

        // 6. Tramo 3 de escaleras (Lado Oeste: localX en [-6.0, -2.0], localZ en [-2.6, 5.8])
        if (localX >= -6.0f && localX <= -2.0f && localZ >= -2.6f && localZ <= 5.8f) {
            float t = (localZ - (-2.6f)) / 7.9f;
            t = glm::clamp(t, 0.0f, 1.0f);
            candidates.push_back(baseY + 9.00f + t * 4.65f); // 9.00m -> 13.65m
        }

        // 6b. Escalón de salida/transición a la azotea (STEP_3_11: localX en [-3.8, -1.2], localZ en [4.4, 6.0])
        if (localX >= -3.8f && localX <= -1.2f && localZ >= 4.4f && localZ <= 6.0f) {
            candidates.push_back(baseY + 13.925f);
        }

        // 7. Mirador / Azotea de la Torre (Rooftop Deck: Y = +14.15m)
        if (std::abs(localX) <= 6.2f && std::abs(localZ) <= 6.2f) {
            // Hueco abierto de la escalera: X en [-6.0, -1.4], Z en [0.8, 6.0]
            bool isStairHole = (localX <= -1.4f && localX >= -6.0f && localZ >= 0.8f && localZ <= 6.0f);
            if (!isStairHole) {
                candidates.push_back(baseY + 14.15f);
            }
        }

        // Seleccionar la superficie que soporte los pies:
        // Todo plano por debajo de los pies o al que se esté subiendo (tolerancia de zancada de 0.70m)
        float highestCandidate = -9999.0f;
        for (float cy : candidates) {
            if (currentFeetY >= cy - 0.70f) {
                if (cy > highestCandidate) {
                    highestCandidate = cy;
                }
            }
        }

        if (highestCandidate > -9000.0f && highestCandidate > bestY) {
            bestY = highestCandidate;
        }
    }

    return bestY;
}

bool StructureSystem::CheckCollision(glm::vec3& entityPos, float radius, float height, glm::vec3& velocity) {
    if (!s_instance) return false;
    bool collided = false;

    float feetY = entityPos.y - height;

    for (const auto& s : s_instance->m_structures) {
        if (s.type != StructureType::ANCIENT_TOWER) continue;

        float baseY = s.pos.y;
        float dx = entityPos.x - s.pos.x;
        float dz = entityPos.z - s.pos.z;

        if (dx * dx + dz * dz > 144.0f) continue;

        float rad = -s.yaw;
        float cosY = cos(rad);
        float sinY = sin(rad);
        float localX = dx * cosY - dz * sinY;
        float localZ = dx * sinY + dz * cosY;

        if (std::abs(localX) > 8.5f || std::abs(localZ) > 8.5f) continue;

        bool localModified = false;

        // 1. Muros exteriores y perimetrales mientras se sube (< 13.8m)
        if (feetY < baseY + 13.8f) {
            float innerBound = 5.75f - radius;

            // Muro Este
            if (localX > innerBound && localX < 6.8f && std::abs(localZ) < 6.2f) {
                localX = innerBound;
                localModified = true;
            }
            // Muro Norte
            if (localZ < -innerBound && localZ > -6.8f && std::abs(localX) < 6.2f) {
                localZ = -innerBound;
                localModified = true;
            }
            // Muro Oeste
            if (localX < -innerBound && localX > -6.8f && std::abs(localZ) < 6.2f) {
                localX = -innerBound;
                localModified = true;
            }
            // Muro Sur (excepto entrada en suelo bajo)
            if (localZ > innerBound && localZ < 6.8f && std::abs(localX) < 6.2f) {
                bool isEntrance = (feetY < baseY + 3.2f && std::abs(localX) < 1.8f);
                if (!isEntrance) {
                    localZ = innerBound;
                    localModified = true;
                }
            }

            // Pilar central macizo (evitar atravesar el núcleo hueco)
            float coreBound = 2.05f + radius;
            if (std::abs(localX) < coreBound && std::abs(localZ) < coreBound) {
                float penX = coreBound - std::abs(localX);
                float penZ = coreBound - std::abs(localZ);
                if (penX < penZ) {
                    localX = (localX > 0 ? coreBound : -coreBound);
                } else {
                    localZ = (localZ > 0 ? coreBound : -coreBound);
                }
                localModified = true;
            }
        } else {
            // 2. En la azotea / mirador: las almenas y parapetos protegen los bordes
            float roofBound = 5.65f - radius;
            if (localX > roofBound)  { localX = roofBound;  localModified = true; }
            if (localX < -roofBound) { localX = -roofBound; localModified = true; }
            if (localZ > roofBound)  { localZ = roofBound;  localModified = true; }
            if (localZ < -roofBound) { localZ = -roofBound; localModified = true; }

            // Pretiles de seguridad del hueco de la escalera en la azotea
            // Pretil Norte: impide caer hacia el sur al pozo de la escalera desde la azotea norte
            if (localZ > (0.80f - radius) && localZ < 1.30f && localX <= -1.35f && localX >= -5.9f) {
                localZ = 0.80f - radius;
                localModified = true;
            }
            // Pretil Este: impide caer hacia el oeste al pozo de la escalera desde la azotea este (deja libre Z > 4.4m para el paso)
            if (localX < (-1.40f + radius) && localX > -2.0f && localZ >= 0.70f && localZ <= 4.40f) {
                localX = -1.40f + radius;
                localModified = true;
            }
        }

        if (localModified) {
            float invRad = s.yaw;
            float cInv = cos(invRad);
            float sInv = sin(invRad);
            entityPos.x = s.pos.x + (localX * cInv - localZ * sInv);
            entityPos.z = s.pos.z + (localX * sInv + localZ * cInv);
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            collided = true;
        }
    }

    return collided;
}

bool StructureSystem::Raycast(glm::vec3 start, glm::vec3 end) {
    if (!s_instance) return false;

    auto intersectAABB = [](const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& bMin, const glm::vec3& bMax) -> bool {
        glm::vec3 d = p2 - p1;
        float tmin = 0.0f;
        float tmax = 1.0f;

        for (int i = 0; i < 3; ++i) {
            if (std::abs(d[i]) < 1e-6f) {
                if (p1[i] < bMin[i] || p1[i] > bMax[i]) return false;
            } else {
                float invD = 1.0f / d[i];
                float t1 = (bMin[i] - p1[i]) * invD;
                float t2 = (bMax[i] - p1[i]) * invD;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax) return false;
            }
        }
        return true;
    };

    for (const auto& s : s_instance->m_structures) {
        if (s.type != StructureType::ANCIENT_TOWER) continue;

        glm::vec2 p1_2D(start.x, start.z);
        glm::vec2 p2_2D(end.x, end.z);
        glm::vec2 tower_2D(s.pos.x, s.pos.z);

        glm::vec2 seg = p2_2D - p1_2D;
        float lenSq = glm::dot(seg, seg);
        float t = (lenSq > 0.001f) ? glm::clamp(glm::dot(tower_2D - p1_2D, seg) / lenSq, 0.0f, 1.0f) : 0.0f;
        glm::vec2 closest = p1_2D + seg * t;
        if (glm::distance(closest, tower_2D) > 13.0f) continue;

        float rad = -s.yaw;
        float cosY = cos(rad);
        float sinY = sin(rad);

        auto toLocal = [&](glm::vec3 p) -> glm::vec3 {
            float dx = p.x - s.pos.x;
            float dy = p.y - s.pos.y;
            float dz = p.z - s.pos.z;
            return glm::vec3(dx * cosY - dz * sinY, dy, dx * sinY + dz * cosY);
        };

        glm::vec3 lp1 = toLocal(start);
        glm::vec3 lp2 = toLocal(end);

        // Muros Exteriores
        if (intersectAABB(lp1, lp2, glm::vec3(5.6f, 0.0f, -5.8f), glm::vec3(6.8f, 14.0f, 5.8f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3(-5.8f, 0.0f, -6.8f), glm::vec3(5.8f, 14.0f, -5.6f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3(-6.8f, 0.0f, -5.8f), glm::vec3(-5.6f, 14.0f, 5.8f))) return true;

        // Muro Sur (dejando libre el umbral de la puerta)
        if (intersectAABB(lp1, lp2, glm::vec3(-5.8f, 0.0f, 5.6f), glm::vec3(-1.8f, 14.0f, 6.8f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3( 1.8f, 0.0f, 5.6f), glm::vec3( 5.8f, 14.0f, 6.8f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3(-1.8f, 3.6f, 5.6f), glm::vec3( 1.8f, 14.0f, 6.8f))) return true;

        // Pilar Central Macizo
        if (intersectAABB(lp1, lp2, glm::vec3(-2.1f, 0.0f, -2.1f), glm::vec3(2.1f, 14.0f, 2.1f))) return true;

        // Suelo de la Azotea (dejando abierto el hueco de la escalera: X in [-6.0, -1.4], Z in [0.8, 6.0])
        // Parte Este y Centro (X: [-1.4, 6.2])
        if (intersectAABB(lp1, lp2, glm::vec3(-1.4f, 13.85f, -6.2f), glm::vec3(6.2f, 14.25f, 6.2f))) return true;
        // Parte Noroeste (X: [-6.2, -1.4], Z: [-6.2, 0.8])
        if (intersectAABB(lp1, lp2, glm::vec3(-6.2f, 13.85f, -6.2f), glm::vec3(-1.4f, 14.25f, 0.8f))) return true;

        // Almenas y parapetos
        if (intersectAABB(lp1, lp2, glm::vec3(-6.0f, 14.15f, 5.6f), glm::vec3(6.0f, 15.6f, 6.3f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3(-6.0f, 14.15f, -6.3f), glm::vec3(6.0f, 15.6f, -5.6f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3(5.6f, 14.15f, -6.0f), glm::vec3(6.3f, 15.6f, 6.0f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3(-6.3f, 14.15f, -6.0f), glm::vec3(-5.6f, 14.15f, 6.0f))) return true;

        // Descansillos
        if (intersectAABB(lp1, lp2, glm::vec3(2.2f, 4.3f, -6.0f), glm::vec3(6.0f, 4.65f, -2.2f))) return true;
        if (intersectAABB(lp1, lp2, glm::vec3(-6.0f, 8.8f, -6.0f), glm::vec3(-2.2f, 9.15f, -2.2f))) return true;
    }

    return false;
}

bool StructureSystem::HasLineOfSight(glm::vec3 from, glm::vec3 to) {
    return !Raycast(from, to);
}

std::vector<TowerGuardSpawn> StructureSystem::GetTowerGuardSpawns() const {
    std::vector<TowerGuardSpawn> spawns;
    for (const auto& s : m_structures) {
        if (s.type != StructureType::ANCIENT_TOWER) continue;

        auto rotatePoint = [&](glm::vec3 offset) -> glm::vec3 {
            float c = cos(s.yaw);
            float s_sin = sin(s.yaw);
            return glm::vec3(
                s.pos.x + (offset.x * c - offset.z * s_sin),
                s.pos.y + offset.y,
                s.pos.z + (offset.x * s_sin + offset.z * c)
            );
        };

        // Guardia 1 (Descansillo 1 a 4.5m de altura): Berserker
        spawns.push_back({ rotatePoint(glm::vec3(4.2f, 4.6f, -4.5f)), 1, 1 }); // EnemyType::BERSERKER_WARRIOR

        // Guardia 2 (Descansillo 2 a 9.0m de altura): Arquero Esqueleto
        spawns.push_back({ rotatePoint(glm::vec3(-4.5f, 9.1f, -4.5f)), 4, 2 }); // EnemyType::SKELETON_ARCHER

        // Guardia 3 (Tramo de escaleras 3 a 12.0m): Guerrero Caído
        spawns.push_back({ rotatePoint(glm::vec3(-4.5f, 12.0f, 1.5f)), 0, 2 }); // EnemyType::CORRUPTED_WARRIOR

        // Guardia 4 (Jefe en la azotea / cofre a 14.2m de altura): Caballero de la Muerte
        spawns.push_back({ rotatePoint(glm::vec3(0.0f, 14.2f, 2.6f)), 2, 2 }); // EnemyType::DEATH_KNIGHT
    }
    return spawns;
}
