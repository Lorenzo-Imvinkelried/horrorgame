#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class Player;
class InventorySystem;
class ItemDropSystem;
class DamageNumberSystem;
class ParticleSystem;

enum class StructureType {
    ANCIENT_RUINS_CHEST,
    SACRIFICE_ALTAR,
    ANCIENT_TOWER
};

enum class EnemyType;

struct TowerGuardSpawn {
    glm::vec3 pos;
    int type; // Cast to EnemyType
    int nightLevel = 1;
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
    void GenerateStructures(glm::vec3 playerPos = glm::vec3(0.0f));
    void Render(GLuint shaderProgram, glm::vec3 cameraPos);

    std::string GetPrompt(glm::vec3 playerPos);
    bool TryInteract(glm::vec3 playerPos, 
                     Player& player, 
                     InventorySystem& inventory, 
                     ItemDropSystem& itemDropSystem,
                     DamageNumberSystem& damageNumbers, 
                     ParticleSystem& particles);

    // Sistema de exclusión para que los árboles no crezcan adentro ni cerca de las estructuras
    static bool IsNearStructure(float x, float z, float customDist = 0.0f);

    // Sistema de física y caminabilidad (escaleras sólidas, plataformas y colisión de muros)
    static float GetWalkableHeight(float x, float z, float currentFeetY, float defaultTerrainY);
    static bool CheckCollision(glm::vec3& entityPos, float radius, float height, glm::vec3& velocity);

    // Detección de línea de visión (LOS) y colisión de proyectiles contra estructuras
    static bool Raycast(glm::vec3 start, glm::vec3 end);
    static bool HasLineOfSight(glm::vec3 from, glm::vec3 to);

    std::vector<TowerGuardSpawn> GetTowerGuardSpawns() const;
    const std::vector<WorldStructure>& GetStructures() const { return m_structures; }
    static StructureSystem* GetInstance() { return s_instance; }

private:
    static StructureSystem* s_instance;
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

    GLuint m_towerVAO = 0;
    GLuint m_towerVBO = 0;
    size_t m_towerVertexCount = 0;
};
