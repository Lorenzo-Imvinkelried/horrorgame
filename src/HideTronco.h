#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include <iostream>
#include "ChunkManager.h"

class HideTronco {
public:
    HideTronco();
    
    // Core Logic
    // Returns the position the monster should move to.
    // If no tree found, returns a zero vector or keeps current?
    glm::vec3 Update(glm::vec3 monsterPos, glm::vec3 scentDir, ChunkManager& chunkManager, float deltaTime);
    
    // Debug
    void RenderDebug(GLuint shaderProgram, glm::vec3 monsterPos);

    // State
    enum class State {
        MOVING,
        PEEKING,
        FINISHED // If reached player or no trees
    };

    bool HasTarget() const { return m_hasTarget; }
    glm::vec3 GetTargetTreePos() const { return m_targetTreePos; }
    State GetState() const { return m_state; }
    void Reset() { 
        if (m_hasTarget || m_state == State::PEEKING) {
            std::cout << "[HideTronco] RESET! Scent lost or Interrupted." << std::endl;
        }
        m_state = State::MOVING; 
        m_hasTarget = false; 
        m_moveTimer = 0.0f;
    }

private:
    float m_searchRadius = 32.0f; // Increased Range to prevent getting stuck
    bool m_hasTarget = false;
    glm::vec3 m_targetTreePos;    
    glm::vec3 m_targetHidePos;    
    
    // State Logic
    State m_state = State::MOVING;
    float m_peekTimer = 0.0f;
    float m_peekDuration = 1.0f; // Reduced to 1.0s (Was 2.0s) to feel more aggressive
    float m_moveTimer = 0.0f;    // Failsafe for getting stuck
    float m_moveTimeout = 5.0f;  // If moving > 5s, force next
    glm::vec3 m_lastTreePos = glm::vec3(0.0f); // To prevent re-selecting immediately
    
    // Resources
    GLuint VAO, VBO;
};
