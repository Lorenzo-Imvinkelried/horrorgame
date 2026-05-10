#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <glad/glad.h> // Fixed: Added GLAD
#include "ModelLoader.h"
#include "Config.h"

struct Perch {
    glm::vec3 pos;
    bool hasBirds;
};

struct FlyingBird {
    glm::vec3 pos;
    glm::vec3 startPos;
    glm::vec3 targetPos;
    float yaw;
    float flapTimer;
    float speed;
    float elapsedTime; // Added for lerp
    float flightDuration; // Added for lerp
    float arcHeight; // Randomized arc height
};

#include <map>

// Simple spatial key for the grid
struct ChunkKey {
    int x, z;
    bool operator<(const ChunkKey& other) const {
        if (x != other.x) return x < other.x;
        return z < other.z;
    }
};

class BirdSystem {
public:
    BirdSystem();
    
    // Call this when a chunk generates trees
    void TrySpawnBirds(const std::vector<glm::vec4>& treeData); 
    
    // Main update loop
    void Update(float deltaTime, glm::vec3 playerPos, glm::vec3 monsterPos);
    
    void Render(GLuint shaderProgram);
    
    // Cleanup helper
    void CleanupDistantBirds(glm::vec3 playerPos, float maxDistance = 100.0f);

    void ToggleDebug() { m_showDebug = !m_showDebug; }
    
private:
    // Spatial Partitioning for Perches
    std::map<ChunkKey, std::vector<Perch>> m_perchGrid;
    
    std::vector<FlyingBird> m_activeBirds;
    
    bool m_showDebug = false;

    // Model Data
    // Model Data
    std::vector<BoxDef> m_basePose;
    std::vector<float> m_meshVertices; // Static base mesh (uploaded once)
    
    // Hardware Instancing
    struct BirdInstance {
        float x, y, z;
        float scale;
        float yaw;
        // Align to 32 bytes (8 floats) for GPU efficiency & Attribute 7 offset safety
        float padding[3]; 
    };
    std::vector<BirdInstance> m_instances; 
    GLuint VAO = 0, VBO = 0;        // Static Mesh Buffer
    GLuint instanceVBO = 0;     // Dynamic Data Buffer
    GLuint m_defaultTex = 0; // 1x1 White Texture
    
    // Debug Resources
    GLuint debugVAO = 0, debugVBO = 0;
    
    void BuildMesh();
    void SpawnFlock(glm::vec3 startPos);
    void RenderDebugRing(glm::vec3 pos, glm::vec3 color, GLuint shaderProgram);
    
    // Helper to get chunk key from world pos
    ChunkKey GetChunkKey(glm::vec3 pos);
};
