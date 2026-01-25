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
    // Load Model
    std::string path = "assets/models/monster.txt";
    m_basePose = ModelLoader::Load(path);
    if (m_basePose.empty()) {
        std::cerr << "Failed to load monster model from " << path << std::endl;
        // Fallback: Simple Box
        BoxDef b; b.Pos=glm::vec3(0,1.5,0); b.Scale=glm::vec3(0.5,1.8,0.5); b.Color=glm::vec3(1,0,0); b.Name="TORSO";
        m_basePose.push_back(b);
    }

    // --- CALCULATE DYNAMIC HITBOXES ---
    // Initialize with inverted infinity
    m_bodyMin = glm::vec3(10000.0f); m_bodyMax = glm::vec3(-10000.0f);
    m_headMin = glm::vec3(10000.0f); m_headMax = glm::vec3(-10000.0f);
    
    bool headFound = false;

    for (const auto& box : m_basePose) {
        // Calculate box min/max in local space (assuming scaling is applied to a unit cube centered at origin)
        // Unit cube is [-0.5, 0.5]. 
        // Box Range: Pos +/- (Scale * 0.5)
        glm::vec3 halfSize = box.Scale * 0.5f;
        glm::vec3 boxMin = box.Pos - halfSize;
        glm::vec3 boxMax = box.Pos + halfSize;

        // Union with Body AABB (Include everything)
        m_bodyMin = glm::min(m_bodyMin, boxMin);
        m_bodyMax = glm::max(m_bodyMax, boxMax);

        // Specific Head AABB
        if (box.Name == "HEAD") {
            // Expand head slightly for fairness
            m_headMin = boxMin - glm::vec3(0.05f); 
            m_headMax = boxMax + glm::vec3(0.05f);
            headFound = true;
        }
    }
    
    // Safety if limits are invalid (e.g. empty model)
    if (m_bodyMin.x > m_bodyMax.x) { m_bodyMin = glm::vec3(-0.5, 0, -0.5); m_bodyMax = glm::vec3(0.5, 2, 0.5); }
    // If no head found, approximate top of body
    if (!headFound) {
        m_headMin = m_bodyMax - glm::vec3(0.3, 0.5, 0.3); // Top 50cm center
        m_headMax = m_bodyMax;  
    } else {
        // Ensure Head is also part of body (it is by logic above), but let's make sure Body covers it well
        // Actually, we usually want separate detectors. 
        // Logic: Headshot > BodyShot.
    }

    BuildDeformedMesh();

    // Body VAO/VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_meshVertices.size() * sizeof(float), m_meshVertices.data(), GL_DYNAMIC_DRAW);

    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    // Normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3);

    // Eyes VAO/VBO
    glGenVertexArrays(1, &VAO_Eyes);
    glGenBuffers(1, &VBO_Eyes);
    glBindVertexArray(VAO_Eyes);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Eyes);
    glBufferData(GL_ARRAY_BUFFER, m_eyeVertices.size() * sizeof(float), m_eyeVertices.data(), GL_STATIC_DRAW);
    
    // Same Attributes
    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    // Normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3);
}

Monster::~Monster() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO_Eyes);
    glDeleteBuffers(1, &VBO_Eyes);
}

void Monster::BuildDeformedMesh() {
    // Initial static pose (standing)
    m_animTime = 0.0f;
    AnimateMesh();
}

void Monster::AnimateMesh() {
    // STATIC MESH BUILD (No Animation)
    m_meshVertices.clear();
    m_eyeVertices.clear();
    
    // Split boxes into Body and Eyes
    std::vector<BoxDef> bodyBoxes;
    std::vector<BoxDef> eyeBoxes;

    for (const auto& box : m_basePose) {
        if (box.Name.find("EYE") != std::string::npos) {
            eyeBoxes.push_back(box);
        } else {
            bodyBoxes.push_back(box);
        }
    }

    // Generate Meshes
    ModelLoader::GenerateMesh(bodyBoxes, m_meshVertices);
    ModelLoader::GenerateMesh(eyeBoxes, m_eyeVertices);
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
    // Animate based on distance traveled (Speed sync)
    float speed = glm::length(m_velocity);
    m_animTime += speed * deltaTime; 

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
        // ... (Log removed or minimized) ...
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

            // 2. Move (Tentative)
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
                 // Calculate ACTUAL velocity based on moved distance (handles collisions)
                 if (deltaTime > 0.0001f) {
                    m_velocity = (nextPos - m_pos) / deltaTime;
                 } else {
                    m_velocity = glm::vec3(0.0f);
                 }
                 m_pos = nextPos;
             }
        } else {
             // Too close to target (Arrived/Peeking), just rotate to look at player?
             // std::cout << "[Monster] Holding Position (Peeking/Arrived)." << std::endl;
             // Ideally here we look AT the player if peeking?
             // For now just don't move.
             m_velocity = glm::vec3(0.0f);
        }

        m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);

        m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
    } else {
        // SCENT LOST: Stop Moving Immediately (User Request)
        m_stealthAI.Reset(); 
        m_velocity = glm::vec3(0.0f);
        // Just do nothing -> transitions to IDLE effectively by not moving
    }

    // Visual Decoupling (15 FPS PS1 effect)
    m_visualTickTimer += deltaTime;
    // Visual Decoupling (REMOVED: User requested 60 FPS / Smooth)
    m_visualTickTimer = 0.0f; // Unused
    {
        m_visualPos = m_pos;
        m_visualYaw = m_yaw;
        
        // No animation update needed, VBO is static
    }
}

void Monster::Render(GLuint shaderProgram, GLuint whiteTexID) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    
    // Draw Body using m_visualPos/m_visualYaw (PS1 15 FPS effect)
    glm::mat4 bodyModel = glm::mat4(1.0f);
    bodyModel = glm::translate(bodyModel, m_visualPos);
    // Mesh is now natively corrected in AnimateMesh, so standard rotation works.
    bodyModel = glm::rotate(bodyModel, glm::radians(m_visualYaw), glm::vec3(0,1,0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bodyModel));
    
    // 1. Draw Body (Noise Texture - Assumed bound by caller)
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size()/11)); 

    // 2. Draw Eyes (Solid - No Noise)
    // Bind White Texture to disable noise modulation
    glBindTexture(GL_TEXTURE_2D, whiteTexID);
    glBindVertexArray(VAO_Eyes);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_eyeVertices.size()/11));
    
    // Note: Caller (Main) will need to re-bind noise texture if needed for next object, 
    // or we can rely on Main to set state per object loop.
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

    // Use Pre-Calculated Dynamic AABBs
    // HEAD (Local)
    float tHead = 10000.0f;
    bool hitHead = RayAABBLocal(localOrigin, localDir, m_headMin, m_headMax, tHead);

    // BODY (Local)
    float tBody = 10000.0f;
    bool hitBody = RayAABBLocal(localOrigin, localDir, m_bodyMin, m_bodyMax, tBody);

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
