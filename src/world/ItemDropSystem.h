#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "inventory/ItemInstance.h"

class InventorySystem;
class DamageNumberSystem;
class ParticleSystem;

struct WorldItemDrop {
    ItemInstance instance;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float rotationYaw = 0.0f;
    float bobTimer = 0.0f;
    float lifetime = 0.0f;
    bool isCollected = false;
};

/**
 * @brief ItemDropSystem: Gestiona los objetos físicos arrojados en el mundo 3D por monstruos, cofres y tala.
 * Incluye físicas de caída, rotación, flotación, atracción magnética al jugador y recogida interactiva con tecla [F].
 */
class ItemDropSystem {
public:
    ItemDropSystem();
    ~ItemDropSystem();

    void SpawnDrop(const ItemInstance& item, glm::vec3 pos, glm::vec3 initialVelocity = glm::vec3(0.0f));
    void SpawnDrops(const std::vector<ItemInstance>& items, glm::vec3 pos);

    void Update(float deltaTime, glm::vec3 playerPos, InventorySystem& inventory, 
                DamageNumberSystem& damageNumbers, ParticleSystem& particles);

    void Render(GLuint shaderProgram, glm::vec3 cameraPos);

    std::string GetNearbyPrompt(glm::vec3 playerPos) const;
    bool TryCollectNearby(glm::vec3 playerPos, InventorySystem& inventory, 
                          DamageNumberSystem& damageNumbers, ParticleSystem& particles);

    void Clear();

    size_t GetActiveDropCount() const noexcept { return m_drops.size(); }

private:
    void initMesh();

    std::vector<WorldItemDrop> m_drops;
    GLuint m_itemVAO = 0;
    GLuint m_itemVBO = 0;
    size_t m_itemVertexCount = 0;
};
