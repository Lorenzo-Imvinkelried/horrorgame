#include "Monster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include "Config.h"

Monster::Monster(glm::vec3 startPos) 
    : m_pos(startPos), m_visualPos(startPos), m_yaw(0.0f), m_visualYaw(0.0f), 
      m_state(MonsterState::IDLE), m_health(100.0f), m_visualTickTimer(0.0f),
      m_animTime(0.0f), m_velocity(0.0f), m_targetYaw(0.0f), m_headYaw(0.0f),
      m_speed(Config::Gameplay::MonsterSpeed)
{
    BuildDeformedMesh();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Use GL_DYNAMIC_DRAW since we will update it often
    glBufferData(GL_ARRAY_BUFFER, m_meshVertices.size() * sizeof(float), m_meshVertices.data(), GL_DYNAMIC_DRAW);

    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    // Normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(3);
}

Monster::~Monster() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Monster::BuildDeformedMesh() {
    // Initial static pose (standing)
    m_animTime = 0.0f;
    AnimateMesh();
}

void Monster::AnimateMesh() {
    m_meshVertices.clear();
    
    // Helper to add faces
    auto addFace = [&](glm::vec3 p, glm::vec3 n, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 color) {
        auto push = [&](glm::vec3 v) {
            // APPLY 180 DEGREE ROTATION PERMANENTLY TO MESH (Fixing Backwards Model)
            // Rotate around Y axis: x' = -x, z' = -z
            float vx = -(v.x + p.x);
            float vy = v.y + p.y;
            float vz = -(v.z + p.z);
            float nx = -n.x;
            float ny = n.y;
            float nz = -n.z;

            m_meshVertices.push_back(vx); m_meshVertices.push_back(vy); m_meshVertices.push_back(vz);
            m_meshVertices.push_back(color.r); m_meshVertices.push_back(color.g); m_meshVertices.push_back(color.b);
            m_meshVertices.push_back(nx); m_meshVertices.push_back(ny); m_meshVertices.push_back(nz);
        };
        push(p1); push(p2); push(p3);
        push(p1); push(p3); push(p4);
    };

    auto addBox = [&](glm::vec3 p, glm::vec3 size, glm::vec3 color, float rotX = 0.0f) {
        // Simple rotation around X axis (for swinging limbs)
        auto rot = [&](glm::vec3 v) {
            if (rotX == 0.0f) return v;
            float c = cos(rotX);
            float s = sin(rotX);
            return glm::vec3(v.x, v.y * c - v.z * s, v.y * s + v.z * c);
        };
        
        float w = size.x * 0.5f; float h = size.y * 0.5f; float l = size.z * 0.5f;
        // Apply rotation to local offsets
        glm::vec3 v1 = rot({-w,h,l}); glm::vec3 v2 = rot({w,h,l}); 
        glm::vec3 v3 = rot({w,-h,l}); glm::vec3 v4 = rot({-w,-h,l});
        glm::vec3 v5 = rot({w,h,-l}); glm::vec3 v6 = rot({-w,h,-l}); 
        glm::vec3 v7 = rot({-w,-h,-l}); glm::vec3 v8 = rot({w,-h,-l});

        // Face normals need rotation too ideally, but for PS1 style checking, static is ok or approximate
        addFace(p, {0,0,1}, v1, v2, v3, v4, color); // Front
        addFace(p, {0,0,-1}, v5, v6, v7, v8, color); // Back
        addFace(p, {0,1,0}, v6, v5, v2, v1, color); // Top
        addFace(p, {0,-1,0}, v4, v3, v8, v7, color); // Bottom
        addFace(p, {1,0,0}, v2, v5, v8, v3, color); // Right
        addFace(p, {-1,0,0}, v6, v1, v4, v7, color); // Left
    };

    glm::vec3 bone(0.75f, 0.72f, 0.65f); // Grey-Pale
    
    // Animation offsets
    float walkSpeed = 10.0f;
    bool isMoving = glm::length(m_velocity) > 0.1f;
    float lLegRot = isMoving ? sin(m_animTime * walkSpeed) * 0.6f : 0.0f;
    float rLegRot = isMoving ? sin(m_animTime * walkSpeed + 3.14159f) * 0.6f : 0.0f;
    float lArmRot = isMoving ? sin(m_animTime * walkSpeed + 3.14159f) * 0.5f : 0.0f;
    float rArmRot = isMoving ? sin(m_animTime * walkSpeed) * 0.5f : 0.0f;

    // Body parts
    addBox({0, 1.3f, 0}, {0.4f, 0.3f, 0.25f}, bone); // Hips (Root)
    addBox({0, 1.7f, -0.1f}, {0.5f, 0.6f, 0.35f}, bone); // Torso
    addBox({0, 2.4f, -0.25f}, {0.25f, 0.3f, 0.25f}, bone * 0.9f); // Head
    
    // Pivot adjustments for limbs (simple translate-rotate-translate simulation via offset p)
    addBox({-0.35f, 1.3f, -0.1f}, {0.1f, 1.8f, 0.1f}, bone, lArmRot); // L Arm
    addBox({0.35f, 1.3f, -0.1f}, {0.1f, 1.8f, 0.1f}, bone, rArmRot); // R Arm
    addBox({-0.15f, 0.6f, 0.1f}, {0.12f, 1.2f, 0.12f}, bone, lLegRot); // L Leg
    addBox({0.15f, 0.6f, 0.1f}, {0.12f, 1.2f, 0.12f}, bone, rLegRot); // R Leg
}

void Monster::Update(float deltaTime, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir,
                     ChunkManager& chunkManager, ScentManager& scentManager) 
{
    // Minimal Update Loop
    m_animTime += deltaTime;

    // Visual Decoupling (15 FPS PS1 effect)
    m_visualTickTimer += deltaTime;
    if (m_visualTickTimer >= 1.0f / m_visualFPS) {
        m_visualTickTimer = 0.0f;
        m_visualPos = m_pos;
        m_visualYaw = m_yaw;
        
        // Update Animation VBO
        AnimateMesh();
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_meshVertices.size() * sizeof(float), m_meshVertices.data());
    }
}

void Monster::Render(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    
    // Draw Body using m_visualPos/m_visualYaw (PS1 15 FPS effect)
    glm::mat4 bodyModel = glm::mat4(1.0f);
    bodyModel = glm::translate(bodyModel, m_visualPos);
    // Mesh is now natively corrected in AnimateMesh, so standard rotation works.
    bodyModel = glm::rotate(bodyModel, glm::radians(m_visualYaw), glm::vec3(0,1,0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bodyModel));
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size()/9 - 6)); 

    // Draw Head separately
    glm::mat4 headModel = bodyModel;
    headModel = glm::translate(headModel, glm::vec3(0, 2.1f, -0.25f)); 
    headModel = glm::rotate(headModel, glm::radians(m_headYaw), glm::vec3(0,1,0));
    headModel = glm::translate(headModel, -glm::vec3(0, 2.1f, -0.25f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(headModel));
    glDrawArrays(GL_TRIANGLES, (GLsizei)(m_meshVertices.size()/9 - 6), 6); 
}

void Monster::RenderDebug(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    
    // X-Ray Mode: Disable Depth Test
    glDisable(GL_DEPTH_TEST); 
    // Debug Color: Red (controlled by shader uniform 1)
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1); 
    
    // Draw Body using m_visualPos/m_visualYaw (Matches Render)
    glm::mat4 bodyModel = glm::mat4(1.0f);
    bodyModel = glm::translate(bodyModel, m_visualPos);
    // Mesh is natively corrected
    bodyModel = glm::rotate(bodyModel, glm::radians(m_visualYaw), glm::vec3(0,1,0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bodyModel));
    
    glBindVertexArray(VAO);
    // Draw everything except the "Head" part (last 6 vertices)
    if (m_meshVertices.size() > 54) { // Ensure safe size
         glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size()/9 - 6)); 
    }

    // Draw Head separately (Matches Render)
    glm::mat4 headModel = bodyModel;
    // Head pivot offset (matches Render)
    headModel = glm::translate(headModel, glm::vec3(0, 2.1f, -0.25f)); 
    headModel = glm::rotate(headModel, glm::radians(m_headYaw), glm::vec3(0,1,0));
    headModel = glm::translate(headModel, -glm::vec3(0, 2.1f, -0.25f)); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(headModel));
    
    if (m_meshVertices.size() >= 54) {
        glDrawArrays(GL_TRIANGLES, (GLsizei)(m_meshVertices.size()/9 - 6), 6); 
    }
    
    // Reset Debug State
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);

    // DRAW FACE VECTOR (Line)
    // Use u_IsDebug = 1 for Red
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1);

    // Calculate Head World Position
    // Head Center Local: (0, 2.4f, 0.25f) -> WAS -0.25f, but we rotated mesh 180 in generation
    glm::vec3 headCenterLocal(0.0f, 2.4f, 0.25f);
    glm::mat4 bodyMat = glm::mat4(1.0f);
    bodyMat = glm::translate(bodyMat, m_visualPos);
    bodyMat = glm::rotate(bodyMat, glm::radians(m_visualYaw), glm::vec3(0,1,0));
    glm::vec3 headPos = glm::vec3(bodyMat * glm::vec4(headCenterLocal, 1.0f));

    // Calculate Forward Direction
    // Total Yaw = Body Yaw + Head Yaw
    float totalYaw = m_visualYaw + m_headYaw;
    glm::vec3 forwardDir(sin(glm::radians(totalYaw)), 0.0f, cos(glm::radians(totalYaw)));
    
    glm::vec3 lineEnd = headPos + forwardDir * 2.0f; // 2 meters long

    // Draw Line
    std::vector<float> lineData = {
        headPos.x, headPos.y, headPos.z,   1,0,0,  0,0,0, // Start (Pos, Col, Norm)
        lineEnd.x, lineEnd.y, lineEnd.z,   1,0,0,  0,0,0  // End
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

void Monster::LookAt(glm::vec3 target) {
    glm::vec3 dir = glm::normalize(target - m_pos);
    m_yaw = glm::degrees(atan2(dir.x, dir.z));
    m_targetYaw = m_yaw;
    m_visualYaw = m_yaw;
}

void Monster::TakeDamage(float amount) {
    m_health -= amount;
}
