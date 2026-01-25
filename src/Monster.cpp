#include "Monster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include "Config.h"

Monster::Monster(glm::vec3 startPos) 
    : m_pos(startPos), m_visualPos(startPos), m_yaw(0.0f), m_visualYaw(0.0f), 
      m_state(MonsterState::IDLE), m_health(2.0f), m_visualTickTimer(0.0f),
      m_animTime(0.0f), m_velocity(0.0f), m_targetYaw(0.0f), m_headYaw(0.0f),
      m_speed(Config::Gameplay::MonsterSpeed), m_isDead(false)
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

#include "ParticleSystem.h" // Needed for particles
#include <algorithm> // for min/max

// ... (Existing Constructor) ...
// We need to update constructor to set m_health = 2.0f and m_isDead = false

void Monster::Update(float deltaTime, glm::vec3 playerPos, glm::vec2 windDir,
                     ChunkManager& chunkManager, ScentSystem& scentSystem, ParticleSystem& particles) 
{
    if (m_isDead) return; // Do nothing if dead (or just fade out?)

    // Minimal Update Loop
    m_animTime += deltaTime;

    // Bleeding Effect (If injured)
    if (m_health < 2.0f) {
        static float bleedTimer = 0.0f;
        bleedTimer += deltaTime;
        if (bleedTimer > 0.3f) { // Leak blood every 0.3s
            bleedTimer = 0.0f;
            // Drip from body center
            glm::vec3 bleedPos = m_pos + glm::vec3((rand()%100/200.0f - 0.25f), 1.5f, (rand()%100/200.0f - 0.25f));
            particles.SpawnParticle(bleedPos, glm::vec3(0, -2.0f, 0), glm::vec4(0.8f, 0.0f, 0.0f, 1.0f), 0.1f, 1.0f, -9.8f);
        }
    }

    // 4. SCENT TRACKING AI (STEALTH: HideTronco)
    glm::vec3 trackDir; // Direction UPWIND (Towards player usually)
    bool smellsPlayer = scentSystem.IsPointInScent(m_pos, trackDir);

    if (smellsPlayer) {
        static float lastLog = 0.0f;
        m_animTime += deltaTime; // Hack to use animTime for log timer if needed, or just use static
        // Use a simple static timer
        // We can reuse lastLogTime logic if available, or just log occasionally
        // For debugging "early smell", log EVERY frame if smell is true initially? No, too spammy.
        // Let's log if state changes or every 1s.
        std::cout << "[Monster] SMELLS PLAYER! Dir: (" << trackDir.x << ", " << trackDir.z << ")" << std::endl;
        // Find best tree to hide behind while moving towards player
        glm::vec3 targetHidePos = m_stealthAI.Update(m_pos, trackDir, chunkManager, deltaTime);
        
        // If we have a valid hide target, move there. 
        // If not (open field?), maybe just move upwind directly?
        glm::vec3 moveTarget;
        
        if (m_stealthAI.HasTarget()) {
             moveTarget = targetHidePos;
        } else {
             // Fallback: Just walk upwind (old logic) if no trees
             moveTarget = m_pos + trackDir * 5.0f;
             // std::cout << "[Monster] No Hide Target. Falling back to simple Upwind walk." << std::endl;
        }
        
        glm::vec3 diff = moveTarget - m_pos;
        float distSq = glm::dot(diff, diff);
        
        if (distSq > 0.01f) {
            // We have somewhere to go
            glm::vec3 desiredDir = glm::normalize(diff);
            
            // 1. Rotate
            float targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
            float rotSpeed = 5.0f * deltaTime;
            float yawDiff = targetYaw - m_yaw;
            while (yawDiff < -180) yawDiff += 360;
            while (yawDiff > 180) yawDiff -= 360;
            m_yaw += yawDiff * rotSpeed;
            m_visualYaw = m_yaw; 

            // 2. Move
            glm::vec3 displacement = desiredDir * m_speed * deltaTime;
            glm::vec3 nextPos = m_pos + displacement;
            
            // Basic Collision (Trees)
            std::vector<glm::vec4> nearbyTrees;
            chunkManager.GetTreesInRange(nextPos, 3.0f, nearbyTrees); 
            for (const auto& t : nearbyTrees) {
                float dist = glm::length(glm::vec2(nextPos.x - t.x, nextPos.z - t.z));
                float minD = 0.5f + (0.6f * t.w);
                if (dist < minD) {
                     glm::vec2 push = glm::normalize(glm::vec2(nextPos.x - t.x, nextPos.z - t.z)) * (minD - dist);
                     nextPos.x += push.x;
                     nextPos.z += push.y;
                }
            }
            
             float limit = (Config::World::MapRadius - 1) * Config::World::ChunkSize * Config::World::ChunkScale;
             if (abs(nextPos.x) < limit && abs(nextPos.z) < limit) {
                 m_pos = nextPos;
             }
        } else {
             // Too close to target (Arrived/Peeking), just rotate to look at player?
             // std::cout << "[Monster] Holding Position (Peeking/Arrived)." << std::endl;
             // Ideally here we look AT the player if peeking?
             // For now just don't move.
        }

        m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);

        m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
    } else {
        // SCENT LOST: Stop Moving Immediately (User Request)
        m_stealthAI.Reset(); 
        // Just do nothing -> transitions to IDLE effectively by not moving
    }

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
    // Render Stealth AI Debug (Violet Radius and Tree)
    m_stealthAI.RenderDebug(shaderProgram, m_pos);

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

void Monster::TakeDamage(float amount, bool isHeadshot) {
    if (m_isDead) return;

    if (isHeadshot) {
        m_health = 0.0f; // Insta-kill
    } else {
        m_health -= amount;
    }

    if (m_health <= 0.0f) {
        m_health = 0.0f;
        m_isDead = true;
        m_pos = glm::vec3(0, -1000, 0); // Hack: Move away
        m_visualPos = m_pos;
    }
}

// Helper for AABB overlap
bool RayAABBLocal(glm::vec3 origin, glm::vec3 dir, glm::vec3 minB, glm::vec3 maxB, float& tScale) {
    float t1 = (minB.x - origin.x)/dir.x;
    float t2 = (maxB.x - origin.x)/dir.x;
    float t3 = (minB.y - origin.y)/dir.y;
    float t4 = (maxB.y - origin.y)/dir.y;
    float t5 = (minB.z - origin.z)/dir.z;
    float t6 = (maxB.z - origin.z)/dir.z;

    float tNear = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tFar = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    if (tFar < 0 || tNear > tFar) return false;
    
    tScale = tNear;
    return true;
}

bool Monster::IntersectRay(glm::vec3 origin, glm::vec3 dir, float& dist, bool& isHeadshot) {
    if (m_isDead) return false;

    // Transform Ray to Local Space
    // 1. Translate
    glm::vec3 localOrigin = origin - m_pos;
    
    // 2. Rotate (Inverse Yaw)
    // Angles in radians
    float angle = -glm::radians(m_yaw); 
    float c = cos(angle);
    float s = sin(angle);
    
    auto rotateY = [&](glm::vec3 v) {
        return glm::vec3(v.x * c - v.z * s, v.y, v.x * s + v.z * c);
    };
    
    localOrigin = rotateY(localOrigin);
    glm::vec3 localDir = rotateY(dir);

    // Define AABBs in Local Space (Based on BuildDeformedMesh)
    // Note: In AnimateMesh, we noticed a 180 flip. 
    // If m_pos matches the feet, and we rotated by m_yaw...
    // The mesh is built around (0,0,0) offset.
    
    // HEAD (Local)
    // Visual Center: (0, 2.4, 0.25)
    // Visual Size: 0.25 x 0.3 x 0.25 (Extents: 0.125, 0.15, 0.125)
    // Hitbox: Expanded significantly for gameplay feel
    glm::vec3 hMin(-0.25f, 2.1f, 0.0f); 
    glm::vec3 hMax(0.25f, 2.8f, 0.5f);

    // BODY (Local)
    // Visual Center ~ (0, 1.1, 0)
    // Hitbox: Expanded to remove neck gap and cover shoulders better
    glm::vec3 bMin(-0.35f, 0.0f, -0.2f);
    glm::vec3 bMax(0.35f, 2.3f, 0.45f); // 2.3 overlaps with Head(2.1), checking Head first handles this.

    float tHead = 10000.0f;
    float tBody = 10000.0f;
    bool hitHead = RayAABBLocal(localOrigin, localDir, hMin, hMax, tHead);
    bool hitBody = RayAABBLocal(localOrigin, localDir, bMin, bMax, tBody);

    if (hitHead && hitBody) {
        // Prioritize Head if depths are similar or if head is reasonably close
        // Even if body is hit slightly earlier (e.g. shoulder before ear), give it to the player.
        // Bias head depth by -0.5 units (huge bias!) to favor headshots
        if ((tHead - 0.5f) < tBody) { isHeadshot = true; dist = tHead; return true; }
        else { isHeadshot = false; dist = tBody; return true; }
    } else if (hitHead) {
        isHeadshot = true; dist = tHead; return true;
    } else if (hitBody) {
        isHeadshot = false; dist = tBody; return true;
    }

    return false;
}
