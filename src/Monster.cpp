#include "Monster.h"
// =================================================================================================
// ARCHIVO: Monster.cpp
// DESCRIPCION: Entidad principal del Monstruo.
// RESPONSABILIDAD:
// 1. Integrar la IA (HideTronco y ScentSystem).
// 2. Gestionar la fisica (velocidad, colisiones con arboles y terreno).
// 3. Renderizar el modelo y los efectos (sangrado, debug).
// =================================================================================================
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/type_ptr.hpp>
// #include <glm/gtx/norm.hpp> // Removed
#include <iostream>
#include <algorithm>
#include <cmath>
#include "Config.h"

Monster::Monster(glm::vec3 startPos) 
    : m_pos(startPos), // [POSICION DEL MOB] Se inicializa aqui
      m_visualPos(startPos), m_yaw(0.0f), m_visualYaw(0.0f), 
      m_action(MonsterAction::WANDER), m_health(2.0f), m_visualTickTimer(0.0f),
      m_animTime(0.0f), m_velocity(0.0f), m_targetYaw(0.0f), m_headYaw(0.0f),
      m_speed(Config::Gameplay::MonsterSpeed), m_isDead(false),
      m_cachedStealthDir(1.0f, 0.0f, 0.0f), m_timeSinceLastScent(100.0f), m_scentCheckTimer(0.0f), m_lastSmelledId(-1), m_debugScentDir(0.0f), m_bestTreeDir(0.0f),
      m_bestTreeIndex(-1), m_memPlayerPos(startPos), m_memTimeSinceSeen(999.0f), m_memTimeSinceHeard(999.0f), m_memTimeSinceSmelled(999.0f), m_memTimeSinceAimedAt(999.0f),
      m_targetPos(startPos), m_assignedTreePos(startPos), 
      m_trackingTargetPos(startPos), m_trackingDir(0.0f), m_stateTimer(0.0f),
      m_hasVisualContact(false), m_treeClimbHeight(0.0f), m_isClimbing(false), m_patrolCenter(startPos)
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

void Monster::HearSound(glm::vec3 sourcePos, float volume) {
    float dist = glm::distance(m_pos, sourcePos);
    if (dist < volume) {
        m_memPlayerPos = sourcePos;
        m_memTimeSinceHeard = 0.0f;
        std::cout << "[AI] Heard noise! Memory updated." << std::endl;
    }
}

void Monster::Update(float deltaTime, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir,
                     ChunkManager& chunkManager, ScentSystem& scentSystem, ParticleSystem& particles) 
{
    if (m_isDead) return;

    // Minimal Update Loop (Moved back)
    float speed = glm::length(m_velocity);
    m_animTime += speed * deltaTime; 
    
    // Bleeding Effect
    if (m_health < 2.0f) {
        static float bleedTimer = 0.0f;
        bleedTimer += deltaTime;
        if (bleedTimer > 0.3f) { 
            bleedTimer = 0.0f;
            glm::vec3 bleedPos = m_pos + glm::vec3((rand()%100/200.0f - 0.25f), 1.5f, (rand()%100/200.0f - 0.25f));
            particles.SpawnParticle(bleedPos, glm::vec3(0, -2.0f, 0), glm::vec4(0.8f, 0.0f, 0.0f, 1.0f), 0.1f, 1.0f, -9.8f);
        }
    }

    bool doHeavyLogic = true; // FORCE 60 FPS

    // LOGICA (Se ejecuta cada frame ahora)
    if (doHeavyLogic) {
        // --- LOGICA DE AGRESION ---
        float distToPlayer = glm::distance(m_pos, playerPos);
        
        // 1. INSTANT KILL (Contacto)
        if (distToPlayer < 1.0f) {
            std::cout << "JUMPSCARE! GAME OVER." << std::endl;
            exit(0);
        }

        // Aim Detection & Line of Sight
        glm::vec3 dirToMonster = glm::normalize(m_pos - playerPos);
        float dotProd = glm::dot(playerFront, dirToMonster);
        bool isAimedAt = (dotProd > 0.85f); 
        bool hasLOS = CheckLineOfSight(playerPos, chunkManager);
        m_hasVisualContact = hasLOS;

        // --- 1. UPDATE MEMORY & SENSES ---
        m_memTimeSinceHeard += deltaTime;
        m_memTimeSinceSmelled += deltaTime;
        m_memTimeSinceSeen += deltaTime;
        m_memTimeSinceAimedAt += deltaTime;

        if (hasLOS) {
            m_memPlayerPos = playerPos;
            m_memTimeSinceSeen = 0.0f;
            if (isAimedAt) {
                m_memTimeSinceAimedAt = 0.0f;
            }
        }

        // Scent Check
        m_scentCheckTimer += deltaTime;
        if (m_scentCheckTimer >= 0.1f) {
            m_scentCheckTimer = 0.0f;
            ScentNode debugNode;
            if (scentSystem.GetScentAtPosition(m_pos, debugNode)) {
                if (debugNode.nodeId != m_lastSmelledId) {
                    m_lastSmelledId = debugNode.nodeId;
                    std::cout << "[AI] Scent detected (ID: " << m_lastSmelledId << ")." << std::endl;
                }
                m_memPlayerPos = debugNode.sourcePos;
                m_memTimeSinceSmelled = 0.0f;
            }
        }

        // --- 2. UTILITY SCORING ---
        float scoreWander = 10.0f;
        float scoreInvestigate = 0.0f;
        float scoreStalk = 0.0f;
        float scoreRetreat = 0.0f;
        float scoreClimbTree = 0.0f;
        float scoreChase = 0.0f;

        distToPlayer = glm::distance(m_pos, playerPos);

        // RETREAT: Triggered by being aimed at (even without perfect LOS - uses memory)
        if (isAimedAt && hasLOS) {
            scoreRetreat = 120.0f; // Maximum priority
        } else if (m_memTimeSinceAimedAt < 5.0f) {
            // Recently aimed at - stay in retreat for a while!
            scoreRetreat = 90.0f - (m_memTimeSinceAimedAt * 10.0f);
        }

        if (hasLOS && !isAimedAt) {
            if (distToPlayer < 15.0f) {
                scoreChase = 95.0f; // Very close + not aimed = GO FOR THE KILL
            } else {
                scoreChase = 70.0f - glm::min(distToPlayer, 40.0f);
                scoreStalk = 40.0f + glm::min(distToPlayer, 40.0f);
            }
        }

        // No LOS behaviors
        if (!hasLOS) {
            if (m_memTimeSinceHeard < 10.0f) {
                scoreInvestigate = 80.0f - (m_memTimeSinceHeard * 3.0f);
            }
            if (m_memTimeSinceSmelled < 10.0f) {
                scoreStalk = 65.0f - (m_memTimeSinceSmelled * 2.0f);
            }
            if (m_memTimeSinceSeen > 5.0f && m_memTimeSinceSeen < 20.0f) {
                float inv = glm::max(0.0f, 55.0f - (m_memTimeSinceSeen * 2.0f));
                if (inv > scoreInvestigate) scoreInvestigate = inv;
            }
            if (m_memTimeSinceSeen > 15.0f && m_memTimeSinceHeard > 15.0f && m_memTimeSinceSmelled > 15.0f) {
                scoreClimbTree = 70.0f;
            }
        }

        // Hysteresis (Bonus to current action)
        switch(m_action) {
            case MonsterAction::WANDER: scoreWander += 15.0f; break;
            case MonsterAction::INVESTIGATE: scoreInvestigate += 15.0f; break;
            case MonsterAction::STALK: scoreStalk += 15.0f; break;
            case MonsterAction::RETREAT: scoreRetreat += 15.0f; break;
            case MonsterAction::CLIMB_TREE: scoreClimbTree += 25.0f; break; // Sticky tree climb
            case MonsterAction::CHASE: scoreChase += 15.0f; break;
        }

        // --- 3. SELECT BEST ACTION ---
        MonsterAction bestAction = MonsterAction::WANDER;
        float bestScore = scoreWander;
        if (scoreInvestigate > bestScore) { bestScore = scoreInvestigate; bestAction = MonsterAction::INVESTIGATE; }
        if (scoreStalk > bestScore) { bestScore = scoreStalk; bestAction = MonsterAction::STALK; }
        if (scoreRetreat > bestScore) { bestScore = scoreRetreat; bestAction = MonsterAction::RETREAT; }
        if (scoreClimbTree > bestScore) { bestScore = scoreClimbTree; bestAction = MonsterAction::CLIMB_TREE; }
        if (scoreChase > bestScore) { bestScore = scoreChase; bestAction = MonsterAction::CHASE; }

        if (m_action != bestAction) {
            std::cout << "[AI] Action Change -> ";
            switch(bestAction) {
                case MonsterAction::WANDER: std::cout << "WANDER"; break;
                case MonsterAction::INVESTIGATE: std::cout << "INVESTIGATE"; break;
                case MonsterAction::STALK: std::cout << "STALK"; break;
                case MonsterAction::RETREAT: std::cout << "RETREAT"; break;
                case MonsterAction::CLIMB_TREE: std::cout << "CLIMB_TREE"; break;
                case MonsterAction::CHASE: std::cout << "CHASE"; break;
            }
            std::cout << " (Score: " << bestScore << ")" << std::endl;
            m_action = bestAction;
            m_stateTimer = 0.0f;
            m_bestTreeIndex = -1; // Reset tree assignment
        }

        // --- 4. EXECUTE ACTION ---
        switch (m_action) {
            case MonsterAction::WANDER: {
                m_stateTimer += deltaTime;
                if (m_stateTimer > 5.0f || glm::distance(glm::vec3(m_pos.x, 0, m_pos.z), glm::vec3(m_targetPos.x, 0, m_targetPos.z)) < 1.0f) {
                    m_stateTimer = 0.0f;
                    glm::vec3 offset((rand()%100/50.0f)-1.0f, 0, (rand()%100/50.0f)-1.0f);
                    m_targetPos = m_patrolCenter + offset * 25.0f;
                }
                glm::vec3 flatPos = glm::vec3(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 dir = m_targetPos - flatPos;
                if (glm::length(dir) > 1.0f) {
                    glm::vec3 desiredDir = glm::normalize(dir);
                    m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                    m_velocity = desiredDir * m_speed * 0.4f; // Slow wander
                } else {
                    m_velocity = glm::vec3(0.0f);
                }
                break;
            }

            case MonsterAction::CHASE: {
                glm::vec3 desiredDir = glm::normalize(playerPos - m_pos);
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                m_velocity = desiredDir * m_speed * 1.5f; // Fast sprint
                break;
            }

            case MonsterAction::INVESTIGATE: {
                glm::vec3 flatPos = glm::vec3(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 flatTarget = glm::vec3(m_memPlayerPos.x, 0.0f, m_memPlayerPos.z);
                if (glm::distance(flatPos, flatTarget) < 1.5f) {
                    m_velocity = glm::vec3(0.0f); // Reached target
                    // Increase timer manually to decay investigate score if we reached it
                    m_memTimeSinceHeard += 10.0f * deltaTime;
                } else {
                    glm::vec3 desiredDir = glm::normalize(flatTarget - flatPos);
                    m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                    m_velocity = desiredDir * m_speed * 0.7f; // Cautious walk
                }
                break;
            }

            case MonsterAction::STALK: {
                glm::vec3 flatPos = glm::vec3(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 flatTarget = glm::vec3(m_memPlayerPos.x, 0.0f, m_memPlayerPos.z);
                glm::vec3 trackingDir = glm::normalize(flatTarget - flatPos);
                
                if (glm::distance(flatPos, flatTarget) < 2.0f) {
                    m_velocity = glm::vec3(0.0f);
                    break;
                }
                
                bool needsNewTree = false;
                if (m_bestTreeIndex == -1) {
                    needsNewTree = true;
                } else {
                    if (glm::distance(flatPos, glm::vec3(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z)) < 1.5f) {
                        needsNewTree = true;
                    }
                }
                
                if (needsNewTree) {
                    m_detectedTrees.clear();
                    chunkManager.GetTreesInRange(m_pos, 25.0f, m_detectedTrees);
                    float bestScoreTree = -9999.0f;
                    m_bestTreeIndex = -1;
                    for(int i=0; i<m_detectedTrees.size(); i++) {
                        glm::vec3 treeFlatPos(m_detectedTrees[i].x, 0.0f, m_detectedTrees[i].z);
                        glm::vec3 dirToTree = glm::normalize(treeFlatPos - flatPos);
                        float dotProdTree = glm::dot(dirToTree, trackingDir);
                        if (dotProdTree > 0.3f && glm::distance(treeFlatPos, flatTarget) < glm::distance(flatPos, flatTarget)) {
                            float s = dotProdTree + (rand()%100/100.0f) * 0.5f;
                            if (s > bestScoreTree) { bestScoreTree = s; m_bestTreeIndex = i; }
                        }
                    }
                    if (m_bestTreeIndex != -1) {
                        m_assignedTreePos = glm::vec3(m_detectedTrees[m_bestTreeIndex].x, 0.0f, m_detectedTrees[m_bestTreeIndex].z);
                    }
                }
                
                glm::vec3 desiredDir;
                if (m_bestTreeIndex != -1) {
                    desiredDir = glm::normalize(glm::vec3(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z) - flatPos);
                } else {
                    desiredDir = trackingDir;
                }
                
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                m_velocity = desiredDir * m_speed * 1.0f;
                break;
            }

            case MonsterAction::RETREAT: {
                m_stateTimer += deltaTime;
                
                // ALWAYS re-evaluate best retreat tree every 0.5s or if we don't have one
                bool needNewRetreatTree = (m_bestTreeIndex == -1);
                if (!needNewRetreatTree) {
                    glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                    glm::vec3 flatTree(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z);
                    if (glm::distance(flatPos, flatTree) < 1.5f) {
                        needNewRetreatTree = true; // Reached tree, find next one further away
                    }
                }
                
                if (needNewRetreatTree) {
                    m_detectedTrees.clear();
                    chunkManager.GetTreesInRange(m_pos, 30.0f, m_detectedTrees);
                    float bestScoreRetreat = -9999.0f;
                    m_bestTreeIndex = -1;
                    glm::vec3 awayFromPlayer = glm::normalize(m_pos - playerPos);
                    
                    for(int i=0; i<m_detectedTrees.size(); i++) {
                        glm::vec3 tPos(m_detectedTrees[i].x, 0.0f, m_detectedTrees[i].z);
                        float distFromMe = glm::distance(tPos, glm::vec3(m_pos.x, 0, m_pos.z));
                        if (distFromMe < 2.0f) continue; // Skip trees we're already at
                        float distFromPlayer = glm::distance(tPos, glm::vec3(playerPos.x, 0, playerPos.z));
                        glm::vec3 dirToTree = glm::normalize(tPos - glm::vec3(m_pos.x, 0, m_pos.z));
                        float dotA = glm::dot(dirToTree, awayFromPlayer);
                        
                        if (dotA > -0.2f) { // Accept trees slightly to the side too
                            float s = distFromPlayer * 0.5f + (dotA * 15.0f) + distFromMe * 0.3f;
                            if (s > bestScoreRetreat) { bestScoreRetreat = s; m_bestTreeIndex = i; }
                        }
                    }
                    if (m_bestTreeIndex != -1) {
                        m_assignedTreePos = glm::vec3(m_detectedTrees[m_bestTreeIndex].x, 0.0f, m_detectedTrees[m_bestTreeIndex].z);
                        std::cout << "[AI] RETREAT: New cover tree selected." << std::endl;
                    }
                }

                if (m_bestTreeIndex != -1) {
                    glm::vec3 awayFromPlayer = glm::normalize(m_assignedTreePos - glm::vec3(playerPos.x, 0, playerPos.z));
                    glm::vec3 hideSpot = m_assignedTreePos + awayFromPlayer * 1.5f;
                    
                    glm::vec3 flatPos = glm::vec3(m_pos.x, 0.0f, m_pos.z);
                    glm::vec3 dir = hideSpot - flatPos;
                    if (glm::length(dir) > 0.5f) {
                        glm::vec3 desiredDir = glm::normalize(dir);
                        m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                        m_velocity = desiredDir * m_speed * 2.0f; // SPRINT to cover
                    } else {
                        m_velocity = glm::vec3(0.0f);
                        m_targetYaw = glm::degrees(atan2(-awayFromPlayer.x, -awayFromPlayer.z));
                    }
                } else {
                    // No trees at all - just run away fast
                    glm::vec3 runDir = glm::normalize(m_pos - playerPos);
                    m_targetYaw = glm::degrees(atan2(runDir.x, runDir.z));
                    m_velocity = runDir * m_speed * 2.0f;
                }
                break;
            }

            case MonsterAction::CLIMB_TREE: {
                m_velocity = glm::vec3(0.0f); 
                
                if (m_bestTreeIndex == -1) {
                    m_detectedTrees.clear();
                    chunkManager.GetTreesInRange(m_pos, 25.0f, m_detectedTrees);
                    if (!m_detectedTrees.empty()) {
                        m_bestTreeIndex = rand() % m_detectedTrees.size();
                        m_assignedTreePos = glm::vec3(m_detectedTrees[m_bestTreeIndex].x, 0.0f, m_detectedTrees[m_bestTreeIndex].z);
                        m_pos.x = m_assignedTreePos.x;
                        m_pos.z = m_assignedTreePos.z;
                        m_isClimbing = true;
                        m_treeClimbHeight = 0.0f;
                        std::cout << "[AI] Starting tree climb." << std::endl;
                    } else {
                        // Impossible to climb, manually decay timer to force state out next frame
                        m_memTimeSinceSeen = 0.0f; 
                    }
                } else {
                    if (m_isClimbing) {
                        m_treeClimbHeight += 8.0f * deltaTime; 
                        if (m_treeClimbHeight > 15.0f) {
                            m_treeClimbHeight = 15.0f;
                            m_isClimbing = false;
                            m_stateTimer = 0.0f; 
                            std::cout << "[AI] Reached canopy, scanning..." << std::endl;
                        }
                    } else {
                        m_stateTimer += deltaTime;
                        if (m_stateTimer > 4.0f) { 
                            // Give a fake hint to memory
                            float errorX = (rand()%400/10.0f) - 20.0f; 
                            float errorZ = (rand()%400/10.0f) - 20.0f;
                            m_memPlayerPos = playerPos + glm::vec3(errorX, 0.0f, errorZ);
                            // We don't reset visual timer completely to avoid instant re-climbing
                            m_memTimeSinceHeard = 0.0f; // Treat hint as a sound queue for utility
                            m_memTimeSinceSeen -= 5.0f; // Delay next climb
                            
                            m_treeClimbHeight -= 15.0f * deltaTime; 
                            if (m_treeClimbHeight <= 0.0f) {
                                m_treeClimbHeight = 0.0f;
                                m_bestTreeIndex = -1; 
                                std::cout << "[AI] Climb over. Gained intuition hint." << std::endl;
                            }
                        }
                    }
                }
                break;
            }
        }

        if (m_action != MonsterAction::CLIMB_TREE) {
            m_treeClimbHeight = 0.0f;
        }

        // Verificacion de Colisiones de Arboles (Solo si nos movemos horizontalmente)
        if (glm::length(m_velocity) > 0.1f && m_action != MonsterAction::CLIMB_TREE) {
             m_nearbyTreesCache.clear();
             chunkManager.GetTreesInRange(m_pos + m_velocity * 0.1f, 3.0f, m_nearbyTreesCache); 
        }
    }

    // ALWAYS RUN: Physics Integration & Smooth Rotation
    
    // Ensure Visual Yaw follows Physics Yaw (with optional smoothing if desired, currently direct)
    m_visualYaw = m_yaw;

    if (glm::length(m_velocity) > 0.01f) {
        // Smooth Rotation (Physics)
        float rotSpeed = 5.0f * deltaTime;
        float yawDiff = std::fmod(m_targetYaw - m_yaw, 360.0f);
        if (yawDiff < -180) yawDiff += 360;
        else if (yawDiff > 180) yawDiff -= 360;
        
        m_yaw += yawDiff * rotSpeed;
        m_visualYaw = m_yaw; // Sync again just in case

        // Integrate Position
        glm::vec3 nextPos = m_pos + m_velocity * deltaTime;
        
        // Resolve Collisions using Cached Trees
        if (!m_nearbyTreesCache.empty()) {
            for (const auto& t : m_nearbyTreesCache) {
                float dist = glm::length(glm::vec2(nextPos.x - t.x, nextPos.z - t.z));
                float minD = 0.5f + (0.6f * t.w);
                if (dist < minD) {
                     glm::vec2 push = glm::normalize(glm::vec2(nextPos.x - t.x, nextPos.z - t.z)) * (minD - dist);
                     nextPos.x += push.x;
                     nextPos.z += push.y;
                }
            }
        }
        
        float limit = (Config::World::MapRadius - 1) * Config::World::ChunkSize * Config::World::ChunkScale;
        if (abs(nextPos.x) < limit && abs(nextPos.z) < limit) {
             m_pos = nextPos;
        }
    }
    

    
    // Terrain Snap with Climb Height
    m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z) + m_treeClimbHeight;
    m_visualPos = m_pos;
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

    glDeleteBuffers(1, &lineVBO);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);

// 2. RENDER DEBUG
    // ... (Inside RenderDebug, after Face Vector) ...
    // DRAW SCENT VECTOR (Green Line)
    if (glm::length(m_debugScentDir) > 0.01f) {
         glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 2); // Assume 2 is Green or set color via uniform if supported
         // Manual Color Override in Line Data if shader uses attribs:
         
         glm::vec3 startPos = m_visualPos + glm::vec3(0, 2.8f, 0); // Above Head (2.1 + 0.7)
         glm::vec3 endPos = startPos + m_debugScentDir * 2.0f; // 2 meters long
         
         // Arrowhead Logic
         glm::vec3 dir = glm::normalize(m_debugScentDir);
         glm::vec3 right = glm::cross(dir, glm::vec3(0,1,0));
         if (glm::length(right) < 0.01f) right = glm::vec3(1,0,0); // Handle vertical case
         else right = glm::normalize(right);
         
         glm::vec3 arrowBase = endPos - dir * 0.5f;
         glm::vec3 arrowL = arrowBase + right * 0.3f;
         glm::vec3 arrowR = arrowBase - right * 0.3f;

         // Green Color (0,1,0)
         std::vector<float> scentLine = {
            startPos.x, startPos.y, startPos.z,   0,1,0,  0,0,0,
            endPos.x, endPos.y, endPos.z,         0,1,0,  0,0,0,
            
            // Arrowhead Left
            endPos.x, endPos.y, endPos.z,         0,1,0,  0,0,0,
            arrowL.x, arrowL.y, arrowL.z,         0,1,0,  0,0,0,

            // Arrowhead Right
            endPos.x, endPos.y, endPos.z,         0,1,0,  0,0,0,
            arrowR.x, arrowR.y, arrowR.z,         0,1,0,  0,0,0
         };
         
         GLuint sVAO, sVBO;
         glGenVertexArrays(1, &sVAO);
         glGenBuffers(1, &sVBO);
         glBindVertexArray(sVAO);
         glBindBuffer(GL_ARRAY_BUFFER, sVBO);
         glBufferData(GL_ARRAY_BUFFER, scentLine.size() * sizeof(float), scentLine.data(), GL_STREAM_DRAW);
         
         // Attribs
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

         glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
         glDrawArrays(GL_LINES, 0, 6);
         
         glDeleteVertexArrays(1, &sVAO);
         glDeleteBuffers(1, &sVBO);
         
         // --- DRAW TREE SCAN CIRCLE ---
         glm::vec3 scanPos = m_pos;
         scanPos.y = m_pos.y + 0.5f; // Just above ground
         
         std::vector<float> circleData;
         const int segments = 32;
         float radius = Config::Monster::TreeScanRadius;
         
         for (int i = 0; i < segments; ++i) {
             float theta = (float)i / segments * 2.0f * 3.14159f;
             float nextTheta = (float)(i+1) / segments * 2.0f * 3.14159f;
             
             float x1 = cos(theta) * radius;
             float z1 = sin(theta) * radius;
             float x2 = cos(nextTheta) * radius;
             float z2 = sin(nextTheta) * radius;
             
             // P1 (Purple)
             circleData.push_back(scanPos.x + x1); circleData.push_back(scanPos.y); circleData.push_back(scanPos.z + z1);
             circleData.push_back(1); circleData.push_back(0); circleData.push_back(1); // Purple
             circleData.push_back(0); circleData.push_back(0); circleData.push_back(0); // Norm
             
             // P2
             circleData.push_back(scanPos.x + x2); circleData.push_back(scanPos.y); circleData.push_back(scanPos.z + z2);
             circleData.push_back(1); circleData.push_back(0); circleData.push_back(1); 
             circleData.push_back(0); circleData.push_back(0); circleData.push_back(0); 
         }
         
         GLuint cVAO, cVBO;
         glGenVertexArrays(1, &cVAO);
         glGenBuffers(1, &cVBO);
         glBindVertexArray(cVAO);
         glBindBuffer(GL_ARRAY_BUFFER, cVBO);
         glBufferData(GL_ARRAY_BUFFER, circleData.size() * sizeof(float), circleData.data(), GL_STREAM_DRAW);
         
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

         glDrawArrays(GL_LINES, 0, segments * 2);
         
         glDeleteVertexArrays(1, &cVAO);
         glDeleteBuffers(1, &cVBO);

         // RESET DEBUG STATE
         glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    }

    // DRAW DETECTED TREES (Points)
    if (!m_detectedTrees.empty()) {
        std::vector<float> points;
        for (int i = 0; i < m_detectedTrees.size(); ++i) {
            glm::vec3 pos = glm::vec3(m_detectedTrees[i]); 
            
            points.push_back(pos.x); points.push_back(pos.y); points.push_back(pos.z);
            
            // Highlight BEST TREE in MAGENTA (1,0,1), others RED (1,0,0)
            if (i == m_bestTreeIndex) {
                points.push_back(1); points.push_back(0); points.push_back(1); 
            } else {
                points.push_back(1); points.push_back(0); points.push_back(0); 
            }
            
            points.push_back(0); points.push_back(1); points.push_back(0); 
        }

        GLuint pVAO, pVBO;
        glGenVertexArrays(1, &pVAO);
        glGenBuffers(1, &pVBO);
        glBindVertexArray(pVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pVBO);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STREAM_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3); // Vertex Color Mode
        glPointSize(12.0f); // Big dots
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glDrawArrays(GL_POINTS, 0, (GLsizei)m_detectedTrees.size());
        glPointSize(1.0f); 

        glDeleteVertexArrays(1, &pVAO);
        glDeleteBuffers(1, &pVBO);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    }

    // DRAW TREE VECTORS (Yellow Lines)
    if (!m_treeVectors.empty()) {
        std::vector<float> lines;
        glm::vec3 headPos = m_visualPos + glm::vec3(0, 2.5f, 0); 
        
        for (const auto& dir : m_treeVectors) {
            glm::vec3 endPos = headPos + dir * 5.0f; 
            
            lines.push_back(headPos.x); lines.push_back(headPos.y); lines.push_back(headPos.z);
            lines.push_back(1); lines.push_back(1); lines.push_back(0); // Yellow
            lines.push_back(0); lines.push_back(0); lines.push_back(0);

            lines.push_back(endPos.x); lines.push_back(endPos.y); lines.push_back(endPos.z);
            lines.push_back(1); lines.push_back(1); lines.push_back(0); 
            lines.push_back(0); lines.push_back(0); lines.push_back(0);
            
            // Arrowhead (Yellow)
            glm::vec3 right = glm::cross(dir, glm::vec3(0,1,0));
            if (glm::length(right) < 0.01f) right = glm::vec3(1,0,0); else right = glm::normalize(right);
            glm::vec3 arrowBase = endPos - dir * 0.5f;
            glm::vec3 arrowL = arrowBase + right * 0.3f;
            glm::vec3 arrowR = arrowBase - right * 0.3f;

            lines.push_back(endPos.x); lines.push_back(endPos.y); lines.push_back(endPos.z);
            lines.push_back(1); lines.push_back(1); lines.push_back(0); lines.push_back(0); lines.push_back(0); lines.push_back(0);
            lines.push_back(arrowL.x); lines.push_back(arrowL.y); lines.push_back(arrowL.z);
            lines.push_back(1); lines.push_back(1); lines.push_back(0); lines.push_back(0); lines.push_back(0); lines.push_back(0);

            lines.push_back(endPos.x); lines.push_back(endPos.y); lines.push_back(endPos.z);
            lines.push_back(1); lines.push_back(1); lines.push_back(0); lines.push_back(0); lines.push_back(0); lines.push_back(0);
            lines.push_back(arrowR.x); lines.push_back(arrowR.y); lines.push_back(arrowR.z);
            lines.push_back(1); lines.push_back(1); lines.push_back(0); lines.push_back(0); lines.push_back(0); lines.push_back(0);
        }

        GLuint lVAO, lVBO;
        glGenVertexArrays(1, &lVAO);
        glGenBuffers(1, &lVBO);
        glBindVertexArray(lVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lVBO);
        glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STREAM_DRAW);

        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3); // Use Vertex Colors
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glDrawArrays(GL_LINES, 0, (GLsizei)lines.size() / 9);

        glDeleteVertexArrays(1, &lVAO);
        glDeleteBuffers(1, &lVBO);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    }

             
    // DRAW BEST TREE VECTOR (Magenta Arrow)
    if (glm::length(m_bestTreeDir) > 0.01f) {
         glm::vec3 startPos = m_visualPos + glm::vec3(0, 2.6f, 0); 
         glm::vec3 endPos = startPos + m_bestTreeDir * 5.0f; 
         
         // Arrow Calculation
         glm::vec3 right = glm::cross(m_bestTreeDir, glm::vec3(0,1,0));
         if (glm::length(right) < 0.01f) right = glm::vec3(1,0,0); else right = glm::normalize(right);
         glm::vec3 arrowBase = endPos - m_bestTreeDir * 0.5f;
         glm::vec3 arrowL = arrowBase + right * 0.3f;
         glm::vec3 arrowR = arrowBase - right * 0.3f;
         
         // Magenta (1,0,1)
         std::vector<float> finalLineData = {
             startPos.x, startPos.y, startPos.z, 1,0,1, 0,0,0,
             endPos.x, endPos.y, endPos.z,       1,0,1, 0,0,0,
             
             endPos.x, endPos.y, endPos.z,       1,0,1, 0,0,0,
             arrowL.x, arrowL.y, arrowL.z,       1,0,1, 0,0,0,
             
             endPos.x, endPos.y, endPos.z,       1,0,1, 0,0,0,
             arrowR.x, arrowR.y, arrowR.z,       1,0,1, 0,0,0
         };

         GLuint cVAO, cVBO;
         glGenVertexArrays(1, &cVAO);
         glGenBuffers(1, &cVBO);
         glBindVertexArray(cVAO);
         glBindBuffer(GL_ARRAY_BUFFER, cVBO);
         glBufferData(GL_ARRAY_BUFFER, finalLineData.size() * sizeof(float), finalLineData.data(), GL_STREAM_DRAW);

         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

         glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3); // Vertex Color
         glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1);
         glLineWidth(3.0f); // Thicker for Best Tree
         glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
         glDrawArrays(GL_LINES, 0, 6);
         glLineWidth(1.0f);
         glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);

         glDeleteVertexArrays(1, &cVAO);
         glDeleteBuffers(1, &cVBO);
         glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    }
    
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



int Monster::GetBestTreeIndex() {
    if (m_detectedTrees.empty()) return -1;

    int bestIndex = -1;
    float bestDot = -2.0f; // Dot product range is [-1, 1]

    glm::vec3 flatMonsterPos(m_pos.x, 0.0f, m_pos.z);
    
    // We already have m_debugScentDir (which is -windDir, aka vector TO source)
    // We need to compare it with Vector(Monster -> Tree)
    glm::vec3 flatScentDir = glm::normalize(glm::vec3(m_debugScentDir.x, 0.0f, m_debugScentDir.z));

    for (int i = 0; i < m_detectedTrees.size(); ++i) {
        glm::vec3 flatTreePos(m_detectedTrees[i].x, 0.0f, m_detectedTrees[i].z);
        
        glm::vec3 dirToTree = flatTreePos - flatMonsterPos;
        if (glm::length(dirToTree) < 0.1f) continue; // Too close
        
        dirToTree = glm::normalize(dirToTree);
        
        // Dot Product: 1.0 = Tree is exactly in direction of scent source
        float dot = glm::dot(dirToTree, flatScentDir);
        
        if (dot > bestDot) {
            bestDot = dot;
            bestIndex = i;
        }
    }

    return bestIndex;
}

bool Monster::CheckLineOfSight(glm::vec3 playerPos, ChunkManager& chunkManager) {
    // Basic Raycast from Monster Head to Player Head
    glm::vec3 startPos = m_visualPos + glm::vec3(0, 2.0f, 0); // Approx Head
    glm::vec3 endPos = playerPos + glm::vec3(0, 1.5f, 0);
    glm::vec3 dir = endPos - startPos;
    float dist = glm::length(dir);
    if (dist < 0.1f) return true;
    dir = glm::normalize(dir);

    // 1. Terrain Check (Step along ray)
    float stepSize = 1.0f;
    for (float d = stepSize; d < dist; d += stepSize) {
        glm::vec3 p = startPos + dir * d;
        float h = WorldGenerator::GetHeight(p.x, p.z);
        if (p.y < h) return false; // Blocked by terrain
    }

    // 2. Tree Check
    std::vector<glm::vec4> trees;
    chunkManager.GetTreesInRange(startPos, dist + 2.0f, trees);
    
    // Line segment A-B
    glm::vec2 A(startPos.x, startPos.z);
    glm::vec2 B(endPos.x, endPos.z);
    
    for (const auto& t : trees) {
        glm::vec2 center(t.x, t.z);
        float radius = 0.5f + (0.6f * t.w); // Tree radius logic

        // Vector from A to center
        glm::vec2 ac = center - A;
        glm::vec2 ab = B - A;
        float ab2 = glm::dot(ab, ab);
        if (ab2 == 0.0f) continue;

        float d = glm::dot(ac, ab) / ab2;
        
        glm::vec2 closest;
        if (d < 0.0f) closest = A;
        else if (d > 1.0f) closest = B;
        else closest = A + d * ab;

        float distToCenterSq = glm::dot(closest - center, closest - center);
        if (distToCenterSq < radius * radius) {
            return false; // Blocked by tree
        }
    }
    
    return true;
}

