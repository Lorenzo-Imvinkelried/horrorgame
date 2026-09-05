#include "ItemDropSystem.h"
#include "inventory/InventorySystem.h"
#include "inventory/ItemRegistry.h"
#include "combat/DamageNumberSystem.h"
#include "ParticleSystem.h"
#include "WorldGenerator.h"
#include "ModelLoader.h"
#include "inventory/ItemModelRegistry.h"
#include "world/StructureSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <iostream>

ItemDropSystem::ItemDropSystem() {
    initMesh();
    // Inicializar el registro de modelos 3D de objetos inmediatamente al iniciar el juego
    ItemModelRegistry::Get().Init();
}

ItemDropSystem::~ItemDropSystem() {
    if (m_itemVAO != 0) {
        glDeleteVertexArrays(1, &m_itemVAO);
        m_itemVAO = 0;
    }
    if (m_itemVBO != 0) {
        glDeleteBuffers(1, &m_itemVBO);
        m_itemVBO = 0;
    }
}

void ItemDropSystem::initMesh() {
    // Generar un modelo 3D estilizado de bolsa de botín / gema para los objetos arrojados
    std::vector<BoxDef> boxes;
    // Base de la bolsa de botín (cuero marrón enriquecido)
    boxes.push_back({ glm::vec3(0.0f, 0.22f, 0.0f), glm::vec3(0.46f, 0.38f, 0.46f), glm::vec3(0.0f), glm::vec3(0.68f, 0.42f, 0.18f), "LootPouchBase" });
    // Refuerzo inferior de la bolsa
    boxes.push_back({ glm::vec3(0.0f, 0.06f, 0.0f), glm::vec3(0.38f, 0.12f, 0.38f), glm::vec3(0.0f), glm::vec3(0.45f, 0.25f, 0.10f), "LootPouchBottom" });
    // Cuello de amarre con cuerda dorada
    boxes.push_back({ glm::vec3(0.0f, 0.45f, 0.0f), glm::vec3(0.28f, 0.12f, 0.28f), glm::vec3(0.0f), glm::vec3(0.95f, 0.80f, 0.25f), "TieRope" });
    // Cima plisada de la bolsa
    boxes.push_back({ glm::vec3(0.0f, 0.56f, 0.0f), glm::vec3(0.36f, 0.12f, 0.36f), glm::vec3(0.0f), glm::vec3(0.62f, 0.38f, 0.15f), "PouchTop" });
    // Broche / gema mística central reflectante
    boxes.push_back({ glm::vec3(0.0f, 0.26f, 0.24f), glm::vec3(0.14f, 0.14f, 0.08f), glm::vec3(0.0f), glm::vec3(0.95f, 0.85f, 0.20f), "GemLock" });

    std::vector<float> verts;
    ModelLoader::GenerateMesh(boxes, verts);
    m_itemVertexCount = verts.size() / 11;

    glGenVertexArrays(1, &m_itemVAO);
    glGenBuffers(1, &m_itemVBO);
    glBindVertexArray(m_itemVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_itemVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    // Formato Vertex: Pos(3), Color(3), UV(2), Normal(3) -> 11 floats
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

void ItemDropSystem::SpawnDrop(const ItemInstance& item, glm::vec3 pos, glm::vec3 initialVelocity, float pickupDelay) {
    if (!item.IsValid()) return;

    WorldItemDrop drop;
    drop.instance = item;
    drop.position = pos + glm::vec3(0.0f, 0.4f, 0.0f);
    
    if (glm::length(initialVelocity) > 0.01f) {
        drop.velocity = initialVelocity;
    } else {
        float vx = (rand() % 100 / 50.0f - 1.0f) * 1.2f;
        float vz = (rand() % 100 / 50.0f - 1.0f) * 1.2f;
        float vy = 2.6f + (rand() % 50 / 50.0f) * 1.0f;
        drop.velocity = glm::vec3(vx, vy, vz);
    }
    
    drop.rotationYaw = static_cast<float>(rand() % 360);
    drop.bobTimer = (rand() % 100 / 100.0f) * 6.28f;
    drop.lifetime = 0.0f;
    drop.pickupDelay = pickupDelay;
    drop.isCollected = false;

    m_drops.push_back(drop);
}

void ItemDropSystem::SpawnDrops(const std::vector<ItemInstance>& items, glm::vec3 pos, float pickupDelay) {
    for (const auto& item : items) {
        SpawnDrop(item, pos, glm::vec3(0.0f), pickupDelay);
    }
}

void ItemDropSystem::Update(float deltaTime, glm::vec3 playerPos, InventorySystem& inventory, 
                           DamageNumberSystem& damageNumbers, ParticleSystem& particles) 
{
    for (auto it = m_drops.begin(); it != m_drops.end();) {
        it->lifetime += deltaTime;
        it->rotationYaw += 80.0f * deltaTime;
        it->bobTimer += deltaTime * 3.5f;
        if (it->pickupDelay > 0.0f) {
            it->pickupDelay -= deltaTime;
        }

        float terrainY = WorldGenerator::GetHeight(it->position.x, it->position.z);
        float groundFloorY = StructureSystem::GetWalkableHeight(it->position.x, it->position.z, it->position.y, terrainY);
        float minItemY = groundFloorY + 0.22f;

        // Físicas de caída parabólica
        if (glm::length(it->velocity) > 0.05f) {
            it->position += it->velocity * deltaTime;
            it->velocity.y -= 12.0f * deltaTime; // Gravedad
            it->velocity.x *= (1.0f - 2.5f * deltaTime);
            it->velocity.z *= (1.0f - 2.5f * deltaTime);

            // Rebotar / colisionar con almenas y muros de la estructura
            StructureSystem::CheckCollision(it->position, 0.25f, 0.3f, it->velocity);

            if (it->position.y <= minItemY) {
                it->position.y = minItemY;
                it->velocity = glm::vec3(0.0f);
            }
        } else {
            // Asegurar que nunca quede enterrado o por debajo del piso de la estructura
            if (it->position.y < minItemY) {
                it->position.y = minItemY;
                it->velocity = glm::vec3(0.0f);
            }
        }


        // Partículas temáticas y haz de luz vertical continuo según rareza
        if ((rand() % 100) < 40) {
            const ItemDefinition& def = ItemRegistry::Get().Get(it->instance.id);
            glm::vec4 sparkCol(0.9f, 0.9f, 0.9f, 0.8f);
            float beamH = 1.2f;

            if (def.rarity == ItemRarity::UNCOMMON) {
                sparkCol = glm::vec4(0.2f, 0.85f, 0.3f, 0.9f);
                beamH = 1.6f;
            } else if (def.rarity == ItemRarity::RARE) {
                sparkCol = glm::vec4(0.15f, 0.45f, 0.98f, 0.95f);
                beamH = 2.4f;
            } else if (def.rarity == ItemRarity::EPIC) {
                sparkCol = glm::vec4(0.75f, 0.20f, 0.95f, 1.0f);
                beamH = 3.2f;
            } else if (def.rarity == ItemRarity::LEGENDARY) {
                sparkCol = ((rand() % 2) == 0) ? glm::vec4(1.0f, 0.75f, 0.1f, 1.0f) : glm::vec4(1.0f, 0.25f, 0.1f, 1.0f);
                beamH = 4.5f;
            }

            glm::vec3 emitPos = it->position + glm::vec3(
                (rand() % 100 / 50.0f - 1.0f) * 0.25f,
                0.1f + (rand() % 100 / 100.0f) * beamH,
                (rand() % 100 / 50.0f - 1.0f) * 0.25f
            );
            particles.SpawnParticle(emitPos, glm::vec3(0, 0.6f, 0), sparkCol, 0.12f, 0.65f, 0.0f);
        }

        ++it;
    }
}

std::string ItemDropSystem::GetNearbyPrompt(glm::vec3 playerPos) const {
    float closestDist = 3.2f;
    const WorldItemDrop* bestDrop = nullptr;

    for (const auto& drop : m_drops) {
        float dist = glm::distance(playerPos, drop.position);
        if (dist < closestDist) {
            closestDist = dist;
            bestDrop = &drop;
        }
    }

    if (bestDrop != nullptr) {
        const ItemDefinition& def = ItemRegistry::Get().Get(bestDrop->instance.id);
        return "[E] RECOGER " + def.name + " (x" + std::to_string(bestDrop->instance.quantity) + ")";
    }

    return "";
}

bool ItemDropSystem::TryCollectNearby(glm::vec3 playerPos, InventorySystem& inventory, 
                                      DamageNumberSystem& damageNumbers, ParticleSystem& particles) 
{
    float closestDist = 3.2f;
    auto bestIt = m_drops.end();

    for (auto it = m_drops.begin(); it != m_drops.end(); ++it) {
        float dist = glm::distance(playerPos, it->position);
        if (dist < closestDist) {
            closestDist = dist;
            bestIt = it;
        }
    }

    if (bestIt != m_drops.end()) {
        int remaining = 0;
        bool added = inventory.GetInventory().AddInstance(bestIt->instance, &remaining);
        if (added) {
            const ItemDefinition& def = ItemRegistry::Get().Get(bestIt->instance.id);

            // Partículas de recolección
            for (int i = 0; i < 20; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.4f, (rand()%100/50.0f + 0.5f)*2.8f, (rand()%100/50.0f - 1.0f)*2.4f);
                particles.SpawnParticle(bestIt->position + glm::vec3(0, 0.3f, 0), pVel, glm::vec4(0.2f, 0.95f, 0.35f, 1.0f), 0.15f, 0.8f, -6.0f);
            }

            if (remaining > 0) {
                bestIt->instance.quantity = static_cast<uint16_t>(remaining);
            } else {
                m_drops.erase(bestIt);
            }
            return true;
        }
    }

    return false;
}

void ItemDropSystem::Render(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (m_drops.empty()) return;

    ItemModelRegistry::Get().Init();

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    for (const auto& drop : m_drops) {
        float bobOffset = sin(drop.bobTimer) * 0.12f;
        glm::vec3 renderPos = drop.position + glm::vec3(0.0f, bobOffset, 0.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, renderPos);
        model = glm::rotate(model, glm::radians(drop.rotationYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.35f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        const ItemDefinition& def = ItemRegistry::Get().Get(drop.instance.id);
        const ItemMesh* mesh = ItemModelRegistry::Get().GetMesh(def.stringId);
        if (mesh && mesh->vao != 0) {
            glBindVertexArray(mesh->vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh->vertexCount));
        } else if (m_itemVAO != 0) {
            glBindVertexArray(m_itemVAO);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_itemVertexCount));
        }
    }

    glBindVertexArray(0);
}

void ItemDropSystem::Clear() {
    m_drops.clear();
}
