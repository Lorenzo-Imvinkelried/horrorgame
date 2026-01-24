#include "Monster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

Monster::Monster(glm::vec3 startPos) 
    : m_pos(startPos), m_visualPos(startPos), m_yaw(0.0f), m_visualYaw(0.0f), 
      m_state(MonsterState::SEARCHING), m_health(100.0f), m_visualTickTimer(0.0f),
      m_hasStone(false), m_stoneCooldown(5.0f), m_isClinging(false),
      m_smellConfidence(0.0f), m_timeSinceLastContact(0.0f), 
      m_canSeePlayer(false), m_isWatched(false), m_noWatchTime(0.0f), m_animTime(0.0f)
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
            m_meshVertices.push_back(v.x + p.x); m_meshVertices.push_back(v.y + p.y); m_meshVertices.push_back(v.z + p.z);
            m_meshVertices.push_back(color.r); m_meshVertices.push_back(color.g); m_meshVertices.push_back(color.b);
            m_meshVertices.push_back(n.x); m_meshVertices.push_back(n.y); m_meshVertices.push_back(n.z);
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

void Monster::ThrowStone(glm::vec3 target) {
    if (m_hasStone) return;
    m_hasStone = true;
    m_stonePos = m_pos + glm::vec3(0, 2, 0);
    m_stoneVel = glm::normalize(target - m_stonePos) * 20.0f + glm::vec3(0, 5, 0); // Arc
    m_stoneCooldown = 8.0f;
}

void Monster::Update(float deltaTime, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir,
                     ChunkManager& chunkManager, ScentManager& scentManager) 
{
    m_stateTimer += deltaTime;
    m_stoneCooldown -= deltaTime;
    m_timeSinceLastContact += deltaTime;
    m_animTime += deltaTime;

    // 1. Perception
    UpdatePerception(deltaTime, playerPos, playerFront, windDir, chunkManager);

    // 2. AI Decision
    UpdateAI(deltaTime, playerPos, windDir, chunkManager, scentManager);

    // 3. Physics & Collisions
    UpdatePhysics(deltaTime, chunkManager);

    // 4. Visual Decoupling (15 FPS PS1 effect)
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

    // Stone Physics Sync
    if (m_hasStone) {
        m_stoneVel.y -= 9.8f * deltaTime;
        m_stonePos += m_stoneVel * deltaTime;
        if (m_stonePos.y < WorldGenerator::GetHeight(m_stonePos.x, m_stonePos.z)) m_hasStone = false;
    }
}

void Monster::UpdatePerception(float dt, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir, ChunkManager& cm) {
    m_debugVisionEnds.clear();
    m_canSeePlayer = RaycastVision(playerPos, cm);
    
    // 3D CONE SCANNER (Grid of Rays)
    float fovH = 90.0f;
    float fovV = 60.0f;
    int stepsH = 3; // 3 rows
    int stepsV = 3; // 3 columns
    
    float yawRadBase = glm::radians(m_headYaw + m_yaw);
    
    for (int v = 0; v < stepsV; v++) {
        float pitchAngle = -fovV * 0.5f + (fovV / (stepsV - 1)) * v;
        float pitchRad = glm::radians(pitchAngle);
        
        for (int h = 0; h < stepsH; h++) {
            float yawAngle = -fovH * 0.5f + (fovH / (stepsH - 1)) * h;
            float yawRad = yawRadBase + glm::radians(yawAngle);
            
            // 3D Direction Math
            glm::vec3 rayDir(
                sin(yawRad) * cos(pitchRad),
                sin(pitchRad),
                cos(yawRad) * cos(pitchRad)
            );
            
            glm::vec3 ro = m_pos + glm::vec3(0, 2.4f, 0) + rayDir * 0.4f;
            float dist = 500.0f; // Matches RaycastVision max range
            glm::vec3 bestEnd = ro + rayDir * dist;
            
            std::vector<glm::vec4> trees;
            cm.GetTreesInRange(m_pos, dist, trees);
            for(const auto& t : trees) {
                glm::vec3 tPos(t.x, t.y, t.z);
                float tRad = t.w * 0.55f;
                glm::vec3 vToTree = tPos - ro;
                float proj = glm::dot(vToTree, rayDir);
                if(proj > 0 && proj < dist) {
                    glm::vec3 closest = ro + rayDir * proj;
                    if(glm::distance(closest, tPos) < tRad) {
                        bestEnd = closest;
                        dist = proj;
                    }
                }
            }
            m_debugVisionEnds.push_back(bestEnd);
        }
    }

    m_isWatched = IsPlayerLookingAtMe(playerPos, playerFront);

    // Scent Logic with WindFactor
    glm::vec3 toPlayer = playerPos - m_pos;
    float dist = glm::length(toPlayer);
    if (dist < 0.1f) dist = 0.1f;
    glm::vec2 toPlayer2D = glm::normalize(glm::vec2(toPlayer.x, toPlayer.z));
    
    float windFactor = glm::dot(windDir, toPlayer2D);
    if (windFactor > 0.7f && dist < 120.0f) {
        m_smellConfidence += dt;
        // Turn head to scent
        if (m_smellConfidence > 10.0f && !m_canSeePlayer) {
            float angleToWind = glm::degrees(atan2(windDir.x, windDir.y));
            m_headYaw = glm::mix(m_headYaw, angleToWind - m_yaw, dt * 2.0f);
        }
    } else {
        m_smellConfidence -= dt * 0.5f;
    }
    m_smellConfidence = std::clamp(m_smellConfidence, 0.0f, 20.0f);

    if (m_canSeePlayer || m_smellConfidence > 12.0f) m_timeSinceLastContact = 0.0f;
}

void Monster::UpdateAI(float dt, glm::vec3 playerPos, glm::vec2 windDir, ChunkManager& cm, ScentManager& sm) {
    float toPlayerDist = glm::distance(m_pos, playerPos);
    glm::vec3 nextVel(0.0f);

    switch(m_state) {
        case MonsterState::SEARCHING:
            nextVel = glm::vec3(sin(m_stateTimer * 0.2f), 0, cos(m_stateTimer * 0.2f)) * 3.0f;
            m_targetYaw = glm::degrees(atan2(nextVel.x, nextVel.z));
            if (m_canSeePlayer || m_smellConfidence > 8.0f) {
                m_state = MonsterState::TRACKING;
                m_stateTimer = 0.0f;
            }
            if (m_isWatched && m_canSeePlayer) { m_state = MonsterState::STALK; m_stateTimer = 0.0f; }
            if (m_timeSinceLastContact > 15.0f) { m_state = MonsterState::CLIMB_SCOUT; m_stateTimer = 0.0f; }
            // Failsafe: Don't search forever if nothing found, try to scoot closer
            if (m_stateTimer > 8.0f) { m_state = MonsterState::TRACKING; m_stateTimer = 0.0f; }
            break;

        case MonsterState::TRACKING:
            {
                // Transition to STALK much earlier to avoid direct approach
                if (toPlayerDist < 100.0f) {
                    m_state = MonsterState::STALK;
                    m_stateTimer = 0.0f;
                    break;
                }

                int nodeIdx = sm.GetClosestNodeIndex(m_pos, 80.0f);
                if (nodeIdx != -1) {
                    glm::vec3 trail = sm.GetNodePos(nodeIdx);
                    glm::vec3 toTrail = trail - m_pos;
                    if (glm::length(toTrail) > 0.1f) {
                        nextVel = glm::normalize(toTrail) * 7.0f;
                        m_targetYaw = glm::degrees(atan2(nextVel.x, nextVel.z));
                    }
                } else {
                    // FALLBACK: If lost track, go find a tree to scout from
                    m_state = MonsterState::STALK;
                    m_stateTimer = 0.0f;
                }
                
                m_targetYaw = glm::degrees(atan2(playerPos.x - m_pos.x, playerPos.z - m_pos.z));
            }
            break;

        case MonsterState::STALK:
            {
                std::vector<glm::vec4> trees;
                cm.GetTreesInRange(m_pos, 50.0f, trees);
                
                if (!trees.empty()) {
                    // SELECTION LOGIC: Find a tree that is closer to player than we are, 
                    // but not in a direct line (zig-zag).
                    int bestTreeIdx = -1;
                    float bestScore = -1000.0f;
                    
                    glm::vec3 toPlayer = glm::normalize(playerPos - m_pos);

                    for (int i = 0; i < (int)trees.size(); i++) {
                        glm::vec3 tPos(trees[i].x, trees[i].y, trees[i].z);
                        float distToT = glm::distance(m_pos, tPos);
                        if (distToT < 3.0f) continue; // Skip current tree

                        float distTToP = glm::distance(tPos, playerPos);
                        
                        // We want: 
                        // 1. Closer to player than currently (but not TOO close, keep distance)
                        // 2. Not in a direct line (dot product near 0 with 'right' vector)
                        float progress = toPlayerDist - distTToP;
                        if (progress < -5.0f) continue; // Don't retreat too much

                        glm::vec3 toT = glm::normalize(tPos - m_pos);
                        float directness = glm::abs(glm::dot(toT, toPlayer)); // 1.0 = direct line, 0.0 = side
                        
                        float score = progress * 2.0f - directness * 15.0f - distToT * 0.1f;
                        if (score > bestScore) {
                            bestScore = score;
                            bestTreeIdx = i;
                        }
                    }

                    if (bestTreeIdx != -1) {
                        glm::vec3 tPos = glm::vec3(trees[bestTreeIdx].x, trees[bestTreeIdx].y, trees[bestTreeIdx].z);
                        float tRad = trees[bestTreeIdx].w * 0.5f;
                        
                        // Calculate hide spot (opposite side of player)
                        glm::vec3 hideDir = glm::normalize(tPos - playerPos);
                        glm::vec3 hideSpot = tPos + hideDir * (tRad + 1.5f);
                        hideSpot.y = WorldGenerator::GetHeight(hideSpot.x, hideSpot.z);
                        
                        glm::vec3 toSpot = hideSpot - m_pos;
                        float dSpot = glm::length(toSpot);
                        
                        if (dSpot > 0.5f) {
                            // REACTIVE STEALTH:
                            if (m_isWatched) {
                                // 1. Danger Close? -> FLEE immediately to trees further back
                                if (toPlayerDist < 20.0f) {
                                    m_state = MonsterState::FLEE;
                                    m_stateTimer = 0.0f;
                                } 
                                // 2. Exposed? -> SPRINT to the nearest cover (hideSpot)
                                else {
                                    nextVel = glm::normalize(toSpot) * 13.0f; // Fast sprint to hide
                                }
                            } else {
                                // 3. Unseen -> Stalk quietly
                                nextVel = glm::normalize(toSpot) * 9.0f;
                            }
                        } else {
                                // Arrived
                                m_state = MonsterState::PEEK;
                                m_clingingTreePos = tPos;
                                m_stateTimer = 0.0f;
                                m_noWatchTime = 0.0f;
                            }
                    } else {
                        // FALLBACK: If no good tree found, DO NOT approach directly.
                        // Flank (Circle) to find better angle/terrain.
                        m_state = MonsterState::HUNT_FLANK;
                    }
                } else {
                    // FALLBACK: No trees nearby -> Flank/Circle.
                    m_state = MonsterState::HUNT_FLANK;
                }
                
                m_targetYaw = glm::degrees(atan2(playerPos.x - m_pos.x, playerPos.z - m_pos.z));
            }
            break;

        case MonsterState::PEEK:
            nextVel = glm::vec3(0);
            {
                glm::vec3 toTree = m_clingingTreePos - m_pos;
                if (glm::length(toTree) > 0.1f) m_targetYaw = glm::degrees(atan2(toTree.x, toTree.z));

                // Wait longer if being watched
                float waitMultiplier = m_isWatched ? 2.0f : 1.0f;
                
                // Procedural Peeking
                float peekCycle = sin(m_stateTimer * 1.5f);
                if (peekCycle > 0.7f) m_headYaw = glm::mix(m_headYaw, 45.0f, dt * 4.0f);
                else if (peekCycle < -0.7f) m_headYaw = glm::mix(m_headYaw, -45.0f, dt * 4.0f);
                else m_headYaw = glm::mix(m_headYaw, 0.0f, dt * 4.0f);

                // Only move to next tree if player is NOT looking for at least 3 seconds
                // OR if we've been here way too long
                if (!m_isWatched) m_noWatchTime += dt;
                else m_noWatchTime = 0.0f;

                if ((m_noWatchTime > 3.0f || m_stateTimer > 8.0f * waitMultiplier) && toPlayerDist > 12.0f) {
                    m_state = MonsterState::STALK;
                    m_stateTimer = 0.0f;
                    m_noWatchTime = 0.0f;
                }

                if (toPlayerDist < 10.0f && !m_isWatched) m_state = MonsterState::HUNT_FLANK;
            }
            break;

        case MonsterState::HUNT_FLANK:
            {
                glm::vec3 toP = playerPos - m_pos;
                if (glm::length(toP) > 0.1f) {
                    glm::vec3 dirToP = glm::normalize(toP);
                    glm::vec3 right = glm::normalize(glm::cross(dirToP, glm::vec3(0,1,0)));
                    nextVel = (dirToP + right * 1.2f) * 11.0f;
                    m_targetYaw = glm::degrees(atan2(nextVel.x, nextVel.z));
                }
                
                if (toPlayerDist < 7.0f) m_state = MonsterState::CHARGE;
                if (m_isWatched) m_state = MonsterState::STALK; // Hide again if spotted
            }
            break;

        case MonsterState::FLEE:
            {
                glm::vec3 fromP = m_pos - playerPos;
                if (glm::length(fromP) > 0.1f) {
                    nextVel = glm::normalize(fromP) * 18.0f;
                    m_targetYaw = glm::degrees(atan2(nextVel.x, nextVel.z));
                }
                if (toPlayerDist > 80.0f) m_state = MonsterState::SEARCHING;
            }
            break;

        case MonsterState::CHARGE:
            {
                glm::vec3 toP = playerPos - m_pos;
                if (glm::length(toP) > 0.1f) {
                    nextVel = glm::normalize(toP) * 20.0f;
                    m_targetYaw = glm::degrees(atan2(nextVel.x, nextVel.z));
                }
                if (toPlayerDist < 2.0f) m_state = MonsterState::ATTACK;
                if (toPlayerDist > 40.0f) m_state = MonsterState::TRACKING;
            }
            break;
            
        case MonsterState::CLIMB_SCOUT:
            {
                std::vector<glm::vec4> trees;
                cm.GetTreesInRange(m_pos, 15.0f, trees);
                if (!trees.empty()) {
                    glm::vec3 trunkPos = glm::vec3(trees[0].x, m_pos.y, trees[0].z);
                    float distToTrunk2D = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(trunkPos.x, trunkPos.z));
                    float trunkRadius = trees[0].w * 0.5f;
                    float targetDist = trunkRadius + 0.6f;

                    // PHASE 1: Move to Trunk
                    if (distToTrunk2D > targetDist + 0.2f && m_stateTimer < 5.0f) { // Added timer check to prevent getting stuck in phase 1
                        glm::vec3 toTrunk = glm::normalize(trunkPos - m_pos);
                        nextVel = toTrunk * 6.0f;
                        m_targetYaw = glm::degrees(atan2(toTrunk.x, toTrunk.z));
                    } 
                    // PHASE 2: Climb or Descend
                    else {
                        nextVel = glm::vec3(0);
                        // Maintain distance to trunk (hug it)
                        glm::vec2 flatPos(m_pos.x, m_pos.z);
                        glm::vec2 flatTrunk(trees[0].x, trees[0].z);
                        glm::vec2 toMe = glm::normalize(flatPos - flatTrunk);
                        glm::vec2 correctPos = flatTrunk + toMe * targetDist;
                        m_pos.x = glm::mix(m_pos.x, correctPos.x, dt * 5.0f);
                        m_pos.z = glm::mix(m_pos.z, correctPos.y, dt * 5.0f);

                        float targetY = trees[0].y + 14.0f;
                        
                        // Vertical Logic
                        bool shouldDescend = false;
                        float descendSpeed = 10.0f;

                        // Case A: Spotted Player -> FAST Descend
                        if (m_canSeePlayer) {
                            shouldDescend = true;
                            descendSpeed = 25.0f;
                        } 
                        // Case B: Timer Expired (10s) -> Descend to scout elsewhere
                        else if (m_stateTimer > 10.0f) {
                            shouldDescend = true;
                            descendSpeed = 8.0f;
                        }

                        if (shouldDescend) {
                             if (m_pos.y < trees[0].y + 2.0f) {
                                m_state = MonsterState::STALK; // Hit ground -> Go find next tree
                                m_stateTimer = 0.0f;
                            } else {
                                m_pos.y -= dt * descendSpeed;
                            }
                        } else {
                            // Climbing / Waiting
                            m_pos.y = glm::mix(m_pos.y, targetY, dt * 1.5f);
                            m_headYaw = sin(m_stateTimer * 0.8f) * 90.0f; // Look around
                        }
                    }
                } else {
                    m_state = MonsterState::SEARCHING;
                }
            }
            break;
    }

    // BUG FIX: Tree Vibration. Interpolate velocity instead of snapping
    m_velocity = glm::mix(m_velocity, nextVel, dt * 5.0f);
    m_yaw = glm::mix(m_yaw, m_targetYaw, dt * 5.0f);
}

void Monster::UpdatePhysics(float dt, ChunkManager& cm) {
    // Apply velocity
    if (m_state != MonsterState::CLIMB_SCOUT && m_state != MonsterState::PEEK) {
        m_pos += m_velocity * dt;
        m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
    }
    
    ResolveCollisions(cm);
}

void Monster::ResolveCollisions(ChunkManager& cm) {
    std::vector<glm::vec4> trees;
    cm.GetTreesInRange(m_pos, 3.0f, trees);
    for (const auto& t : trees) {
        glm::vec3 tPos(t.x, t.y, t.z);
        float radius = t.w * 0.5f + 0.4f; // Tree radius + monster radius
        float dist = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(tPos.x, tPos.z));
        if (dist < radius) {
            glm::vec2 pushDir = glm::normalize(glm::vec2(m_pos.x, m_pos.z) - glm::vec2(tPos.x, tPos.z));
            glm::vec2 newPos = glm::vec2(tPos.x, tPos.z) + pushDir * radius;
            m_pos.x = newPos.x;
            m_pos.z = newPos.y;
        }
    }
}

void Monster::Render(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    
    // Draw Body using m_visualPos/m_visualYaw (PS1 15 FPS effect)
    glm::mat4 bodyModel = glm::mat4(1.0f);
    bodyModel = glm::translate(bodyModel, m_visualPos);
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

    // RENDER STONE
    if (m_hasStone) {
        glm::mat4 sModel = glm::mat4(1.0f);
        sModel = glm::translate(sModel, m_stonePos);
        sModel = glm::scale(sModel, glm::vec3(0.2f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sModel));
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size()/9)); 
    }
}

void Monster::RenderDebug(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    
    glDisable(GL_DEPTH_TEST); 
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1); // 1 = Red
    
    // DRAW VISION RAYS
    if (!m_debugVisionEnds.empty()) {
        std::vector<float> lineData;
        float rad = glm::radians(m_headYaw + m_yaw);
        glm::vec3 ro = m_pos + glm::vec3(0, 2.4f, 0) + glm::vec3(sin(rad), 0, cos(rad)) * 0.3f;
        for (const auto& end : m_debugVisionEnds) {
            // Start
            lineData.push_back(ro.x); lineData.push_back(ro.y); lineData.push_back(ro.z);
            lineData.push_back(1.0f); lineData.push_back(0.0f); lineData.push_back(0.0f); // Red
            lineData.push_back(0.0f); lineData.push_back(1.0f); lineData.push_back(0.0f); // Normal dummy
            // End
            lineData.push_back(end.x); lineData.push_back(end.y); lineData.push_back(end.z);
            lineData.push_back(1.0f); lineData.push_back(0.0f); lineData.push_back(0.0f); // Red
            lineData.push_back(0.0f); lineData.push_back(1.0f); lineData.push_back(0.0f); // Normal dummy
        }
        
        GLuint lVAO, lVBO;
        glGenVertexArrays(1, &lVAO);
        glGenBuffers(1, &lVBO);
        glBindVertexArray(lVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lVBO);
        glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STREAM_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glm::mat4 id = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(id));
        glDrawArrays(GL_LINES, 0, (GLsizei)(lineData.size() / 9));
        
        glDeleteVertexArrays(1, &lVAO);
        glDeleteBuffers(1, &lVBO);
    }

    // DRAW HITBOX
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_pos + glm::vec3(0, 1.3f, 0));
    model = glm::scale(model, glm::vec3(1.2f, 2.6f, 1.2f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size()/9));
    
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);
}

void Monster::LookAt(glm::vec3 target) {
    glm::vec3 dir = glm::normalize(target - m_pos);
    m_yaw = glm::degrees(atan2(dir.x, dir.z));
    m_targetYaw = m_yaw;
    m_visualYaw = m_yaw;
}

bool Monster::RaycastVision(glm::vec3 playerPos, ChunkManager& chunkManager) {
    float dist = glm::distance(m_pos, playerPos);
    if (dist > 500.0f) return false; // Extended range
    
    glm::vec3 dirToPlayer = glm::normalize(playerPos - m_pos);
    float rad = glm::radians(m_headYaw + m_yaw);
    glm::vec3 headForward(sin(rad), 0, cos(rad));
    
    // Dot product check allows for some vertical offset now (0.35 threshold is wider)
    if (glm::dot(headForward, dirToPlayer) < 0.35f) return false;

    std::vector<glm::vec4> trees;
    chunkManager.GetTreesInRange(m_pos, dist, trees);
    for (const auto& t : trees) {
        glm::vec3 treePos(t.x, t.y, t.z);
        float radius = 0.55f * t.w;
        glm::vec3 rayOrigin = m_pos + glm::vec3(0, 2.4f, 0) + headForward * 0.4f;
        glm::vec3 v = treePos - rayOrigin;
        float proj = glm::dot(v, dirToPlayer);
        if (proj < 0 || proj > dist) continue;
        glm::vec3 closest = rayOrigin + dirToPlayer * proj;
        if (glm::distance(closest, treePos) < radius) return false;
    }
    return true; 
}

bool Monster::IsPlayerLookingAtMe(glm::vec3 playerPos, glm::vec3 playerFront) {
    glm::vec3 dirToMonster = glm::normalize(m_pos - playerPos);
    return glm::dot(playerFront, dirToMonster) > 0.86f; // ~30 deg cone (was 20)
}

void Monster::TakeDamage(float amount) {
    m_health -= amount;
    if (m_state != MonsterState::FLEE) {
        m_state = MonsterState::FLEE;
        m_stateTimer = 0.0f;
    }
}
