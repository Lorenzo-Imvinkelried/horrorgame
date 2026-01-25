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

class BirdSystem {
public:
    BirdSystem();
    
    // Call this when a chunk generates trees
    void TrySpawnBirds(const std::vector<glm::vec4>& treeData); 
    
    // Main update loop
    void Update(float deltaTime, glm::vec3 playerPos, glm::vec3 monsterPos);
    
    void Render(GLuint shaderProgram);

    void ToggleDebug() { m_showDebug = !m_showDebug; }
    
private:
    std::vector<Perch> m_perches;
    std::vector<FlyingBird> m_activeBirds;
    
    bool m_showDebug = false;

    // Model Data
    std::vector<BoxDef> m_basePose;
    std::vector<float> m_meshVertices;
    GLuint VAO, VBO;
    
    // Debug Resources
    GLuint debugVAO = 0, debugVBO = 0;
    
    void BuildMesh();
    void SpawnFlock(glm::vec3 startPos);
    void RenderDebugRing(glm::vec3 pos, glm::vec3 color, GLuint shaderProgram);
};
