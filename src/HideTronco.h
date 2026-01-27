#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include <iostream>
#include "ChunkManager.h"

// =================================================================================================
// HELPER STRUCTS
// =================================================================================================

struct MovementController {
    float moveSpeed = 5.0f;
    float arrivalThreshold = 2.5f; // Increased for robustness (Tree Radius + Monster Radius margin)
    float alignThreshold = 15.0f; // Degrees
    
    enum class MoveStatus { ARRIVED, ROTATING, MOVING };

    // Returns status
    MoveStatus MoveTowards(glm::vec3& monsterPos, float& monsterYaw, glm::vec3 targetPos, float deltaTime);
    float CalculateAngleDiff(glm::vec3 forward, glm::vec3 toTarget);
};

// =================================================================================================
// MAIN CLASS
// =================================================================================================

class HideTronco {
public:
    HideTronco();
    
    // Core Update Function
    // param scentDir: The direction vector towards the strongest scent (Normalized)
    // returns: The position the monster should move towards.
    glm::vec3 Update(glm::vec3 monsterPos, float& monsterYaw, glm::vec3 scentDir, ChunkManager& chunkManager, float deltaTime);
    
    void RenderDebug(GLuint shaderProgram, glm::vec3 monsterPos, glm::vec3 scentDir);

    enum class State {
        SMELLING,       // 1. Detect Scent & Select Tree (One Frame)
        MOVING,         // 2. Move to Selected Tree (Block Scent)
        WAITING,        // 3. Wait at Tree (1 second)
        CHASING         // 4. Final Approach (If close to scent)
    };

    State GetState() const { return m_state; }
    bool HasTarget() const { return m_hasTarget; }
    void Reset();

private:
    // Sub-systems
    MovementController m_movement;
    
    // State
    State m_state = State::SMELLING;
    float m_stateTimer = 0.0f; // Generic timer for states (e.g. Waiting)
    
    // Navigation Data
    glm::vec3 m_currentTargetPos; // Where we are going (Tree or Player)
    bool m_hasTarget = false;
    
    // Configuration
    float m_searchRadius = 35.0f;
    float m_waitDuration = 2.0f; // Wait 2s at each tree
    float m_chaseDistance = 15.0f; // If scent is closer than this, just go for it (Removed limits)

    // Internal Logic
    // Returns index of best tree, or -1
    int SelectBestTree(const std::vector<glm::vec4>& trees, glm::vec3 monsterPos, glm::vec3 scentDir);
    
    // Visuals
    GLuint VAO, VBO;
};
