#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "WorldGenerator.h" 
#include "FootprintSystem.h"
#include "ChunkManager.h"

class Player {
public:
    Player(glm::vec3 startPos);

    void ProcessMouseMovement(float xoffset, float yoffset);
    void ProcessKeyboard(int key, float deltaTime, ChunkManager& chunkManager, FootprintSystem& footprints);
    void Update(float deltaTime); // Physics update
    void RenderDebug(GLuint shaderProgram);

    glm::mat4 GetViewMatrix();
    glm::vec3 GetWeaponOffset(); // For rendering the weapon

    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Euler Angles
    float Yaw;
    float Pitch;

    // Physics
    glm::vec3 Velocity;
    bool IsGrounded;
    float Gravity = 20.0f;
    float JumpForce = 8.0f;
    float WalkSpeed = 6.0f;
    float PlayerHeight = 1.6f;
    float PlayerRadius = 0.3f;

    // Game Feel
    float HeadBobTimer;
    float HeadBobAmount = 0.1f;
    float HeadBobSpeed = 10.0f;
    
    // Weapon Sway
    glm::vec3 WeaponSwayPos;
    float SwayAmount = 0.05f;
    float SwaySmoothing = 5.0f;

private:
    void updateCameraVectors();
    // Helper to get terrain height
    float getTerrainHeight(float x, float z);
};
