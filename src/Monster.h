#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "WorldGenerator.h"
#include "ChunkManager.h"
#include "ScentManager.h"

enum class MonsterState {
    SEARCHING,    
    TRACKING,     
    STALK,        
    PEEK,         // Tree hugging
    CLIMB_SCOUT,  
    HUNT_FLANK,   // Circular flanking
    FLEE,         
    CHARGE,       
    ATTACK
};

class Monster {
public:
    Monster(glm::vec3 startPos);
    ~Monster();

    void Update(float deltaTime, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir,
                ChunkManager& chunkManager, ScentManager& scentManager);
    void Render(GLuint shaderProgram);
    void RenderDebug(GLuint shaderProgram);

    void TakeDamage(float amount);
    
    glm::vec3 GetPosition() const { return m_pos; }
    MonsterState GetState() const { return m_state; }
    void SetPosition(glm::vec3 pos) { m_pos = pos; m_visualPos = pos; }
    void LookAt(glm::vec3 target);

private:
    void BuildDeformedMesh();
    void AnimateMesh();
    float m_animTime;
    std::vector<float> m_meshVertices;
    GLuint VAO, VBO;
    
    // Sub-Updates
    void UpdatePerception(float dt, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir, ChunkManager& cm);
    void UpdateAI(float dt, glm::vec3 playerPos, glm::vec2 windDir, ChunkManager& cm, ScentManager& sm);
    void UpdatePhysics(float dt, ChunkManager& cm);
    void ResolveCollisions(ChunkManager& cm);

    // AI & Physics (Smooth/Real)
    glm::vec3 m_pos;
    glm::vec3 m_velocity;
    float m_yaw;
    float m_targetYaw;
    float m_headYaw;
    MonsterState m_state;
    float m_health;
    
    // Visual Decoupling (15 FPS)
    glm::vec3 m_visualPos;
    float m_visualYaw;
    float m_visualTickTimer;
    float m_visualFPS = 15.0f;

    // Perception Vars
    float m_smellConfidence;
    float m_timeSinceLastContact;
    bool m_canSeePlayer;
    bool m_isWatched;
    
    // State Vars
    float m_stateTimer;
    float m_peekTimer;
    float m_flankAngle;
    glm::vec3 m_clingingTreePos;
    bool m_isClinging;
    float m_noWatchTime;
    
    // Vision Debug
    std::vector<glm::vec3> m_debugVisionEnds;
    
    // Distraction
    void ThrowStone(glm::vec3 target);
    glm::vec3 m_stonePos;
    glm::vec3 m_stoneVel;
    bool m_hasStone;
    float m_stoneCooldown;
    
    // Helpers
    bool RaycastVision(glm::vec3 playerPos, ChunkManager& cm);
    bool IsPlayerLookingAtMe(glm::vec3 playerPos, glm::vec3 playerFront);
};
