#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "ModelLoader.h"

enum class BuildingType {
    WALL = 0,   // Pared sólida
    ROOF = 1,   // Techo / Bóveda de Cueva
    TORCH = 2   // Antorcha de suelo / pared
};

struct BuildingPiece {
    BuildingType type;
    glm::vec3 pos;
    float yaw; // Rotación en grados (0, 90, 180, 270)
    glm::vec3 halfExtents;
    int id;
};

class BuildingSystem {
public:
    BuildingSystem();
    ~BuildingSystem();

    void Init();
    void Update(float deltaTime, glm::vec3 playerPos, class ParticleSystem& particles);
    
    // Renderizado de todas las estructuras construidas
    void Render(GLuint shaderProgram, glm::vec3 playerPos);
    
    // Renderizado de la pieza fantasma/preview al construir
    void RenderGhost(GLuint shaderProgram, BuildingType type, glm::vec3 pos, float yaw, bool isValid, GLuint whiteTexID);

    // Colocar una nueva pieza
    bool PlacePiece(BuildingType type, glm::vec3 pos, float yaw, class ParticleSystem* particles = nullptr);

    // Demoler/quitar pieza más cercana al punto de mira
    bool RemovePieceAt(glm::vec3 aimPos, float maxDist = 4.0f, class ParticleSystem* particles = nullptr);

    // Comprobación de colisiones contra el jugador o monstruos (retorna vector de empuje)
    bool CheckCollision(glm::vec3& entityPos, float radius, float height, glm::vec3& outPush);

    // Obtener la altura efectiva del piso si está sobre un techo construido
    float GetFloorHeight(float x, float z, float currentFeetY, float defaultTerrainY) const;

    // Obtener las antorchas activas más cercanas para iluminación
    std::vector<glm::vec4> GetClosestTorches(glm::vec3 playerPos, int maxTorches = 8);

    const std::vector<BuildingPiece>& GetPieces() const { return m_pieces; }

private:
    std::vector<BuildingPiece> m_pieces;
    int m_nextId;

    // Meshes para cada tipo de pieza
    GLuint m_VAO[3];
    GLuint m_VBO[3];
    int m_vertexCounts[3];

    // Instancing VBOs
    GLuint m_instanceVBO[3];

    void buildMeshes();
};
