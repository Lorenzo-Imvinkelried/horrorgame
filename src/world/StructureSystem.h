#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class Player;
class InventorySystem;
class DamageNumberSystem;
class ParticleSystem;

enum class StructureType {
    ANCIENT_RUINS_CHEST,
    SACRIFICE_ALTAR
};

struct WorldStructure {
    StructureType type;
    glm::vec3 pos;
    float yaw;
    bool looted = false;
};

class StructureSystem {
public:
    StructureSystem();
    ~StructureSystem();

    void InitMesh();
    void GenerateStructures();
    void Render(GLuint shaderProgram, glm::vec3 cameraPos);

    std::string GetPrompt(glm::vec3 playerPos);
    bool TryInteract(glm::vec3 playerPos, 
                     Player& player, 
                     InventorySystem& inventory, 
                     DamageNumberSystem& damageNumbers, 
                     ParticleSystem& particles);

private:
    std::vector<WorldStructure> m_structures;

    GLuint m_pillarVAO = 0;
    GLuint m_pillarVBO = 0;
    size_t m_pillarVertexCount = 0;

    GLuint m_chestVAO = 0;
    GLuint m_chestVBO = 0;
    size_t m_chestVertexCount = 0;

    GLuint m_altarVAO = 0;
    GLuint m_altarVBO = 0;
    size_t m_altarVertexCount = 0;
};
