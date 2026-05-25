#include "Player.h"
#include <cmath>
#include <iostream>
#include <SFML/Window/Keyboard.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include "Config.h"

Player::Player(glm::vec3 startPos) 
    : Position(startPos), Front(glm::vec3(0.0f, 0.0f, -1.0f)), WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
      Yaw(-90.0f), Pitch(0.0f), Velocity(glm::vec3(0.0f)), IsGrounded(false),
      HeadBobTimer(0.0f), BreathTimer(0.0f), WeaponSwayPos(glm::vec3(0.0f))
{
    WalkSpeed = Config::Gameplay::PlayerSpeed;
    updateCameraVectors();
}

void Player::ProcessMouseMovement(float xoffset, float yoffset) {
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    WeaponSwayPos.x += -xoffset * SwayAmount;
    WeaponSwayPos.y += yoffset * SwayAmount;

    float maxSway = 0.1f;
    WeaponSwayPos.x = glm::clamp(WeaponSwayPos.x, -maxSway, maxSway);
    WeaponSwayPos.y = glm::clamp(WeaponSwayPos.y, -maxSway, maxSway);

    if (IsClimbing) {
        m_cameraNoiseAccumulator += std::abs(xoffset) + std::abs(yoffset);
        if (m_cameraNoiseAccumulator > 15.0f) {
            SoundVolumeEmitted = 8.0f; // Leaves rustling sound (quieter, 8m range)
            m_cameraNoiseAccumulator = 0.0f;
        }
    }

    updateCameraVectors();
}

void Player::ProcessKeyboard(int key, float deltaTime, ChunkManager& chunkManager, FootprintSystem& footprints) {
    glm::vec3 moveDir(0.0f);
    glm::vec3 flatFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    glm::vec3 flatRight = glm::normalize(glm::cross(flatFront, WorldUp));

    // Proximity check to nearby trees for climbing
    std::vector<glm::vec4> nearbyTrees;
    chunkManager.GetTreesInRange(Position, 2.5f, nearbyTrees);
    
    glm::vec4 closestTree(0.0f);
    bool treeNear = false;
    float closestDist = 9999.0f;
    
    for (const auto& t : nearbyTrees) {
        glm::vec3 tPos(t.x, t.y, t.z);
        float dist = glm::distance(glm::vec3(Position.x, tPos.y, Position.z), tPos);
        float scaledRadius = 0.6f * t.w;
        // Check if player is close enough to touch the trunk
        if (dist < (scaledRadius + PlayerRadius + 0.35f)) {
            if (dist < closestDist) {
                closestDist = dist;
                closestTree = t;
                treeNear = true;
            }
        }
    }

    if (IsClimbing) {
        // Lock player's 2D position to the trunk boundary
        glm::vec3 treePos(ClimbingTreePos.x, Position.y, ClimbingTreePos.z);
        glm::vec3 toPlayer = Position - treePos;
        toPlayer.y = 0.0f;
        if (glm::length(toPlayer) > 0.01f) {
            toPlayer = glm::normalize(toPlayer);
        } else {
            toPlayer = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        
        float scaledRadius = 0.6f * ClimbingTreeScale;
        glm::vec3 snapPos = treePos + toPlayer * (scaledRadius + PlayerRadius + 0.05f);
        Position.x = snapPos.x;
        Position.z = snapPos.z;
        
        Velocity.x = 0.0f;
        Velocity.z = 0.0f;
        Velocity.y = 0.0f;
        IsGrounded = false;
        
        float climbSpeed = 5.0f;
        bool isMoving = false;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            Position.y += climbSpeed * deltaTime;
            isMoving = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            Position.y -= climbSpeed * deltaTime;
            isMoving = true;
        }
        
        if (isMoving) {
            m_climbNoiseTimer += deltaTime;
            if (m_climbNoiseTimer >= 0.35f) {
                SoundVolumeEmitted = 12.0f; // Rustling/creaking sound (quieter, 12m range)
                m_climbNoiseTimer = 0.0f;
            }
        } else {
            m_climbNoiseTimer = 0.35f; // Ready to sound immediately on next move
        }
        
        // Canopy limit
        float maxClimb = ClimbingTreePos.y + 16.0f * ClimbingTreeScale; // leaves area
        if (Position.y > maxClimb) {
            Position.y = maxClimb;
        }
        
        // Ground release
        float terrainHeight = WorldGenerator::GetHeight(Position.x, Position.z);
        if (Position.y < terrainHeight + PlayerHeight) {
            Position.y = terrainHeight + PlayerHeight;
            IsClimbing = false;
            IsGrounded = true;
        }
        
        // Jump off
        static bool spaceWasPressed = false;
        bool spaceIsPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
        if (spaceIsPressed && !spaceWasPressed) {
            IsClimbing = false;
            Velocity.y = JumpForce * 0.8f;
            Velocity.x = toPlayer.x * WalkSpeed * 0.8f;
            Velocity.z = toPlayer.z * WalkSpeed * 0.8f;
        }
        spaceWasPressed = spaceIsPressed;
        
    } else {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { moveDir += flatFront; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { moveDir -= flatFront; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { moveDir -= flatRight; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { moveDir += flatRight; }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            if (treeNear) {
                IsClimbing = true;
                ClimbingTreePos = glm::vec3(closestTree.x, closestTree.y, closestTree.z);
                ClimbingTreeScale = closestTree.w;
                Velocity = glm::vec3(0.0f);
                IsGrounded = false;
            } else if (IsGrounded) {
                Velocity.y = JumpForce;
                IsGrounded = false;
            }
        }

        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 displacement = moveDir * WalkSpeed * deltaTime;
            glm::vec3 nextPos = Position + displacement;
            
            // Footprints
            if(IsGrounded) {
                 static float distAccumulator = 0.0f;
                 distAccumulator += glm::length(displacement);
                 if(distAccumulator > 1.5f) {
                     distAccumulator = 0.0f;
                     footprints.AddFootprint(Position - glm::vec3(0.0f, PlayerHeight, 0.0f), Yaw);
                 }
            }

            // Tree Collision using ChunkManager (Skip when not climbing)
            std::vector<glm::vec4> collideTrees;
            chunkManager.GetTreesInRange(nextPos, 3.0f, collideTrees); 

            for (const auto& treeData : collideTrees) {
                glm::vec3 treePos(treeData.x, treeData.y, treeData.z);
                float treeScale = treeData.w;
                
                float dx = nextPos.x - treePos.x;
                float dz = nextPos.z - treePos.z;
                float dist = sqrt(dx*dx + dz*dz);
                
                float scaledRadius = 0.6f * treeScale; 
                float minDist = PlayerRadius + scaledRadius;

                if (dist < minDist) {
                    if (dist > 0.0001f) {
                        float push = minDist - dist;
                        nextPos.x += (dx / dist) * push;
                        nextPos.z += (dz / dist) * push;
                    }
                }
            }
            
            // Limit to World Bounds (Invisible Wall)
            float limit = (Config::World::MapRadius - 1) * Config::World::ChunkSize * Config::World::ChunkScale;
            
            if (nextPos.x > limit) nextPos.x = limit;
            if (nextPos.x < -limit) nextPos.x = -limit;
            if (nextPos.z > limit) nextPos.z = limit;
            if (nextPos.z < -limit) nextPos.z = -limit;

            Position.x = nextPos.x;
            Position.z = nextPos.z;

            Velocity.x = moveDir.x * WalkSpeed;
            Velocity.z = moveDir.z * WalkSpeed;

            if(IsGrounded) {
                HeadBobTimer += deltaTime * HeadBobSpeed;
            }
        } else {
            HeadBobTimer = 0.0f;
            Velocity.x = 0.0f;
            Velocity.z = 0.0f;
        }
    }
}

void Player::Update(float deltaTime) {
    if (IsClimbing) {
        BreathTimer += deltaTime * BreathSpeed;
        m_cameraNoiseAccumulator = std::max(0.0f, m_cameraNoiseAccumulator - deltaTime * 10.0f);
        return;
    }

    Velocity.y -= Gravity * deltaTime;
    Position.y += Velocity.y * deltaTime;

    // Breathing Animation
    BreathTimer += deltaTime * BreathSpeed;

    float terrainHeight = WorldGenerator::GetHeight(Position.x, Position.z);
    
    if (Position.y < terrainHeight + PlayerHeight) {
        float targetY = terrainHeight + PlayerHeight;
        Position.y = glm::mix(Position.y, targetY, deltaTime * 15.0f); // Smooth LERP up
        Velocity.y = 0.0f;
        IsGrounded = true;
    } else {
        // DOWNHILL SNAP (Sticky Feet)
        // If we are floating just above the ground (step down) and falling/level, snap down
        float distToGround = Position.y - (terrainHeight + PlayerHeight);
        if (IsGrounded && distToGround < 0.5f && Velocity.y <= 0.0f) {
             float targetY = terrainHeight + PlayerHeight;
             Position.y = glm::mix(Position.y, targetY, deltaTime * 20.0f); // Faster snap down
             Velocity.y = 0.0f;
             IsGrounded = true; // Maintain grounded state
        } else {
             IsGrounded = false;
        }
    }

    WeaponSwayPos = glm::mix(WeaponSwayPos, glm::vec3(0.0f), deltaTime * SwaySmoothing);
}

glm::mat4 Player::GetViewMatrix() {
    glm::vec3 pos = Position;
    if(IsGrounded) {
        if (HeadBobTimer > 0.001f) {
            pos.y += sin(HeadBobTimer) * HeadBobAmount;
        } else {
            pos.y += sin(BreathTimer) * BreathAmount;
        }
    }
    return glm::lookAt(pos, pos + Front, Up);
}

glm::vec3 Player::GetWeaponOffset() {
    glm::vec3 base = glm::vec3(0.2f, -0.25f, 0.4f); 
    base.x += WeaponSwayPos.x;
    base.y += WeaponSwayPos.y;
    return base;
}

void Player::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}

float Player::getTerrainHeight(float x, float z) {
    return WorldGenerator::GetHeight(x, z);
}

void Player::RenderDebug(GLuint shaderProgram) {
    static GLuint debugVAO = 0, debugVBO = 0;
    if (debugVAO == 0) {
        float b = 0.5f;
        float h = 1.0f;
        float cube[] = {
            -b,h,b, b,h,b, b,-h,b, -b,h,b, b,-h,b, -b,-h,b,
            b,h,-b, -b,h,-b, -b,-h,-b, b,h,-b, -b,-h,-b, b,-h,-b,
            -b,h,-b, -b,h,b, -b,-h,b, -b,h,-b, -b,-h,b, -b,-h,-b,
            b,h,b, b,h,-b, b,-h,-b, b,h,b, b,-h,-b, b,-h,b,
            -b,h,-b, b,h,-b, b,h,b, -b,h,-b, b,h,b, -b,h,b,
            -b,-h,b, b,-h,b, b,-h,-b, -b,-h,b, b,-h,-b, -b,-h,-b 
        };
        glGenVertexArrays(1, &debugVAO);
        glGenBuffers(1, &debugVBO);
        glBindVertexArray(debugVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glDisable(GL_DEPTH_TEST);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 2); // 2 = Blue

    glBindVertexArray(debugVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);

    // DRAW VIEW VECTOR (Blue Line)
    glDisable(GL_DEPTH_TEST);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 2); // 2 = Blue

    glm::vec3 lineStart = Position;
    glm::vec3 lineEnd = Position + Front * 5.0f; // 5 meters long (longer than monster's)

    std::vector<float> lineData = {
        lineStart.x, lineStart.y, lineStart.z,   0,0,1,  0,0,0, // Start (Pos, Col, Norm)
        lineEnd.x, lineEnd.y, lineEnd.z,         0,0,1,  0,0,0  // End
    };

    GLuint lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STREAM_DRAW);

    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Col
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // Reset Model Matrix for Line (Already in World Space)
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glDrawArrays(GL_LINES, 0, 2);

    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);
}
