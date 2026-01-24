#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "WorldGenerator.h"
#include "ChunkManager.h"
#include "ScentManager.h"

// Reset state enum
enum class MonsterState {
    IDLE
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
    
    // AI & Physics (Smooth/Real)
    glm::vec3 m_pos;
    glm::vec3 m_velocity;
    float m_yaw;
    float m_targetYaw;
    float m_headYaw;
    MonsterState m_state;
    float m_health;
    float m_speed;
    
    // Visual Decoupling (15 FPS)
    glm::vec3 m_visualPos;
    float m_visualYaw;
    float m_visualTickTimer;
    float m_visualFPS = 15.0f;
};
