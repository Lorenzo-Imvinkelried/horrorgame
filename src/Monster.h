#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "ChunkManager.h"
#include "ScentSystem.h"
// #include "HideTronco.h" // REMOVED
#include "ModelLoader.h"

// Reset state enum
enum class MonsterState {
    IDLE
};

class Monster {
public:
    Monster(glm::vec3 startPos);
    ~Monster();

    void Update(float deltaTime, glm::vec3 playerPos, glm::vec2 windDir,
                ChunkManager& chunkManager, ScentSystem& scentSystem, class ParticleSystem& particles);
    
    // Updated Render to accept texture for eyes
    void Render(GLuint shaderProgram, GLuint whiteTexID); 
    void RenderDebug(GLuint shaderProgram);


    // Combat
    void TakeDamage(float amount, bool isHeadshot);
    bool IntersectRay(glm::vec3 origin, glm::vec3 dir, float& dist, bool& isHeadshot);
    bool IsDead() const { return m_isDead; }
    
    glm::vec3 GetPosition() const { return m_pos; }
    MonsterState GetState() const { return m_state; }
    void SetPosition(glm::vec3 pos) { m_pos = pos; m_visualPos = pos; }
    void LookAt(glm::vec3 target);

private:
    void BuildDeformedMesh();
    void AnimateMesh();
    float m_animTime;
    
    // Body Mesh (Noisy)
    std::vector<float> m_meshVertices;
    GLuint VAO, VBO;

    // Eye Mesh (Solid/No Noise)
    std::vector<float> m_eyeVertices;
    GLuint VAO_Eyes, VBO_Eyes;
    
    // AI & Physics (Smooth/Real)
    glm::vec3 m_pos;
    glm::vec3 m_velocity;
    float m_yaw;
    float m_targetYaw;
    float m_headYaw;
    MonsterState m_state;
    float m_health;
    bool m_isDead;
    float m_speed;
    
    // Scent Pathfinding State
    glm::vec3 m_cachedStealthDir;             // For Input Stability (Hysteresis)
    float m_timeSinceLastScent;               // For Logic Persistence (Memory)
    float m_scentCheckTimer;                  // Control sampling rate (0.1s)
    int m_lastSmelledId;                      // To prevent duplicate messages
    glm::vec3 m_debugScentDir;                // Visualization vector
    std::vector<glm::vec4> m_detectedTrees;   // Cache for debug visualization (vec4 to match ChunkManager)
    std::vector<glm::vec3> m_treeVectors;     // 2D Directions to trees
    glm::vec3 m_bestTreeDir;                  // Direction to the CHOSEN tree (Logic)
    int m_bestTreeIndex;                      // Index of the chosen tree (for visualization)
    
    // Visual Decoupling (15 FPS)
    glm::vec3 m_visualPos;
    float m_visualYaw;
    float m_visualTickTimer;
    float m_visualFPS = 15.0f;

    // AI Components
    // HideTronco m_stealthAI; // REMOVED
    
    // Internal Logic
    // Returns index of best tree in m_detectedTrees, or -1
    int GetBestTreeIndex();

    std::vector<glm::vec4> m_nearbyTreesCache; // Cache for collision loops
    
    // Model Data (Loaded from file)
    std::vector<struct BoxDef> m_basePose;
    
    // Dynamic Hitboxes (Calculated from m_basePose)
    glm::vec3 m_bodyMin, m_bodyMax;
    glm::vec3 m_headMin, m_headMax;
};
