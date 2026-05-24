#include "Monster.h"
// =================================================================================================
// ARCHIVO: Monster.cpp
// DESCRIPCION: Entidad principal del Monstruo. (Forced rebuild 2)
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
      m_action(MonsterAction::WANDER), m_health(4.0f), m_visualTickTimer(0.0f),
      m_animTime(0.0f), m_velocity(0.0f), m_targetYaw(0.0f), m_headYaw(0.0f),
      m_speed(Config::Gameplay::MonsterSpeed), m_isDead(false),
      m_cachedStealthDir(1.0f, 0.0f, 0.0f), m_timeSinceLastScent(100.0f), m_scentCheckTimer(0.0f), m_lastSmelledId(-1), m_debugScentDir(0.0f), m_bestTreeDir(0.0f),
      m_bestTreeIndex(-1), m_memPlayerPos(startPos), m_memTimeSinceSeen(999.0f), m_memTimeSinceHeard(999.0f), m_memTimeSinceSmelled(999.0f), m_memTimeSinceAimedAt(999.0f),
      m_targetPos(startPos), m_assignedTreePos(startPos), 
      m_trackingTargetPos(startPos), m_trackingDir(0.0f), m_stateTimer(0.0f),
      m_hasVisualContact(false), m_treeClimbHeight(0.0f), m_isClimbing(false), m_patrolCenter(startPos),
      m_headPitch(0.0f), m_startleTimer(0.0f), m_startleCooldownTimer(0.0f), m_peekTimer(0.0f), m_peekAngle(0.0f), m_sniffParticleTimer(0.0f),
      m_hasStartled(false), m_scentPathIndex(0), m_assignedTreeScale(1.0f),
      m_stuckTimer(0.0f), m_prevPos(startPos), m_feintTimer(0.0f), m_feintAngle(0.0f),
      m_decisionLockTimer(0.0f), m_estimatedPlayerAmmo(2), m_wasPlayerReloading(false), m_estimatedPlayerPos(startPos), m_estimatedPlayerPosTimer(0.0f),
      m_canopyWaitTime(8.0f), m_confidence(0.5f), m_stress(0.0f)
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
    m_meshVertices.clear();
    m_eyeVertices.clear();
    
    // Copy the base pose so we can animate it
    std::vector<BoxDef> animatedBoxes = m_basePose;
    
    // Define animation parameters based on time and speed
    float speed = glm::length(m_velocity);
    float speedFac = glm::clamp(speed / m_speed, 0.0f, 2.0f);
    float cycle = m_animTime * 2.2f; // Speed up cycle
    
    // Calculate limb swings (sinusoidal)
    float limbSwingL = sin(cycle) * 0.45f * speedFac;
    float limbSwingR = -sin(cycle) * 0.45f * speedFac;
    
    // State dependent swings
    float armSwingL = limbSwingL;
    float armSwingR = limbSwingR;
    float bodyLean = 0.0f;
    float headYawRad = glm::radians(m_headYaw);
    float headPitchRad = glm::radians(m_headPitch);
    
    if (m_action == MonsterAction::CHASE) {
        // Aggressive predatory reach
        armSwingL = -0.9f + sin(cycle * 2.0f) * 0.15f;
        armSwingR = -0.9f + cos(cycle * 2.0f) * 0.15f;
        bodyLean = 0.4f; // Lean forward
    } else if (m_action == MonsterAction::TRACK_SCENT) {
        // Sniffing pose
        armSwingL = 0.2f + sin(cycle) * 0.1f;
        armSwingR = 0.2f - sin(cycle) * 0.1f;
        bodyLean = 0.3f; // Lean forward
        headPitchRad = 0.45f + sin(cycle * 2.0f) * 0.1f; // Sniffing head tilt
    } else if (m_startleCooldownTimer > 9.0f) {
        // Shocked scream while running
        armSwingL = 1.3f; // Arms raised high
        armSwingR = 1.3f;
        bodyLean = -0.15f; // Lean slightly backward
        headPitchRad = -0.4f; // Head looking up screaming
    }
    
    // Pivots
    glm::vec3 lShoulder(-0.4f, 2.0f, 0.4f);
    glm::vec3 rShoulder(0.4f, 2.0f, 0.4f);
    glm::vec3 lHip(-0.18f, 1.15f, 0.0f);
    glm::vec3 rHip(0.18f, 1.15f, 0.0f);
    glm::vec3 lKnee(-0.18f, 0.75f, 0.0f);
    glm::vec3 rKnee(0.18f, 0.75f, 0.0f);
    glm::vec3 neckPivot(0.0f, 2.16f, 0.5f);
    glm::vec3 hipsPivot(0.0f, 1.2f, 0.0f);
    
    // Helper to rotate a point around an arbitrary pivot and axis (X-axis)
    auto rotatePivotX = [](glm::vec3 p, glm::vec3 pivot, float angle) {
        glm::vec3 local = p - pivot;
        float s = sin(angle), c = cos(angle);
        float ny = local.y * c - local.z * s;
        float nz = local.y * s + local.z * c;
        return pivot + glm::vec3(local.x, ny, nz);
    };
    
    // Helper to rotate a point around arbitrary pivot with Yaw and Pitch
    auto rotateNeck = [](glm::vec3 p, glm::vec3 pivot, float yaw, float pitch) {
        glm::vec3 local = p - pivot;
        // Yaw (around Y)
        float sY = sin(yaw), cY = cos(yaw);
        float nx = local.x * cY - local.z * sY;
        float nz = local.x * sY + local.z * cY;
        local.x = nx; local.z = nz;
        
        // Pitch (around X)
        float sP = sin(pitch), cP = cos(pitch);
        float ny = local.y * cP - local.z * sP;
        float nz2 = local.y * sP + local.z * cP;
        return pivot + glm::vec3(local.x, ny, nz2);
    };
    
    for (auto& box : animatedBoxes) {
        bool isUpperBody = true;
        
        // --- 1. LOCAL TRANSFORMS ---
        if (box.Name.find("L_LEG") != std::string::npos) {
            box.Pos = rotatePivotX(box.Pos, lHip, limbSwingL);
            box.Rot.x += limbSwingL;
            isUpperBody = false;
        } 
        else if (box.Name.find("L_CALF") != std::string::npos) {
            // Fold knee back during backswing
            float calfSwing = (limbSwingL < 0.0f) ? -limbSwingL * 1.2f : 0.0f;
            box.Pos = rotatePivotX(box.Pos, lKnee, calfSwing);
            box.Rot.x += calfSwing;
            // Now rotate around hip
            box.Pos = rotatePivotX(box.Pos, lHip, limbSwingL);
            box.Rot.x += limbSwingL;
            isUpperBody = false;
        } 
        else if (box.Name.find("R_LEG") != std::string::npos) {
            box.Pos = rotatePivotX(box.Pos, rHip, limbSwingR);
            box.Rot.x += limbSwingR;
            isUpperBody = false;
        } 
        else if (box.Name.find("R_CALF") != std::string::npos) {
            float calfSwing = (limbSwingR < 0.0f) ? -limbSwingR * 1.2f : 0.0f;
            box.Pos = rotatePivotX(box.Pos, rKnee, calfSwing);
            box.Rot.x += calfSwing;
            // Now rotate around hip
            box.Pos = rotatePivotX(box.Pos, rHip, limbSwingR);
            box.Rot.x += limbSwingR;
            isUpperBody = false;
        }
        else if (box.Name.find("HIPS") != std::string::npos) {
            isUpperBody = false; // Hips do not lean
        }
        else if (box.Name.find("L_ARM") != std::string::npos || 
                 box.Name.find("L_FOREARM") != std::string::npos ||
                 box.Name.find("L_HAND") != std::string::npos ||
                 box.Name.find("L_CLAW") != std::string::npos) 
        {
            box.Pos = rotatePivotX(box.Pos, lShoulder, armSwingL);
            box.Rot.x += armSwingL;
        }
        else if (box.Name.find("R_ARM") != std::string::npos || 
                 box.Name.find("R_FOREARM") != std::string::npos ||
                 box.Name.find("R_HAND") != std::string::npos ||
                 box.Name.find("R_CLAW") != std::string::npos) 
        {
            box.Pos = rotatePivotX(box.Pos, rShoulder, armSwingR);
            box.Rot.x += armSwingR;
        }
        else if (box.Name.find("HEAD") != std::string::npos || 
                 box.Name.find("EYE") != std::string::npos) 
        {
            box.Pos = rotateNeck(box.Pos, neckPivot, headYawRad, headPitchRad);
            box.Rot.y += headYawRad;
            box.Rot.x += headPitchRad;
        }
        
        // --- 2. PARENT UPPER BODY LEAN ---
        if (isUpperBody) {
            box.Pos = rotatePivotX(box.Pos, hipsPivot, bodyLean);
            box.Rot.x += bodyLean;
        }
    }
    
    // Split boxes into Body and Eyes
    std::vector<BoxDef> bodyBoxes;
    std::vector<BoxDef> eyeBoxes;

    for (const auto& box : animatedBoxes) {
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

        // If volume is 150.0f, it's a shotgun blast! Incur bullet count estimation
        if (std::abs(volume - 150.0f) < 0.1f) {
            m_estimatedPlayerAmmo = std::max(0, m_estimatedPlayerAmmo - 1);
            std::cout << "[AI] Heard shotgun blast! Estimated player ammo: " << m_estimatedPlayerAmmo << std::endl;
        }
    }
}

void Monster::Update(float deltaTime, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir,
                     ChunkManager& chunkManager, ScentSystem& scentSystem, ParticleSystem& particles,
                     glm::vec3 playerVelocity, int playerAmmo, bool isPlayerReloading) 
{
    if (m_isDead) return;
    m_prevPos = m_pos;
    glm::vec3 initialVel = m_velocity;
    glm::vec3 targetVelocity = glm::vec3(0.0f);

    // Incremental animation time
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

    // --- AGGRESSION SENSES & FLASH DETECT ---
    float distToPlayer = glm::distance(m_pos, playerPos);
    
    // 1. INSTANT KILL (Handled in main.cpp for glitched screen)

    // Aim Detection & Line of Sight
    glm::vec3 dirToMonster = glm::normalize(m_pos - playerPos);
    float dotProd = glm::dot(playerFront, dirToMonster);
    bool isAimedAt = (dotProd > 0.96f); // Tightened: Must aim directly at it
    bool hasLOS = CheckLineOfSight(playerPos, chunkManager);
    m_hasVisualContact = hasLOS;
    
    // Update estimated player position (introduces organic human error when not in LOS)
    if (hasLOS) {
        m_estimatedPlayerPos = playerPos;
        m_estimatedPlayerPosTimer = 0.0f;
    } else {
        m_estimatedPlayerPosTimer += deltaTime;
        if (m_estimatedPlayerPosTimer >= 1.0f) {
            m_estimatedPlayerPosTimer = 0.0f;
            float noiseRange = 3.5f;
            m_estimatedPlayerPos = playerPos + glm::vec3((rand() % 100 / 50.0f - 1.0f) * noiseRange, 0.0f, (rand() % 100 / 50.0f - 1.0f) * noiseRange);
        }
    }

    // Update inferred player ammo count when the reload is fully completed.
    // The reload is complete when isPlayerReloading transitions from true to false.
    if (m_wasPlayerReloading && !isPlayerReloading) {
        m_estimatedPlayerAmmo = 2;
        std::cout << "[AI] Player reload complete! Reset estimated player ammo to 2." << std::endl;
    }
    m_wasPlayerReloading = isPlayerReloading;

    bool isVisibleToPlayer = (dotProd > 0.82f) && hasLOS; // Visible on player's screen

    // Detect if player is trying to approach the monster
    bool isPlayerApproaching = false;
    if (glm::length(playerVelocity) > 0.1f) {
        glm::vec3 playerVelDir = glm::normalize(playerVelocity);
        if (glm::dot(playerVelDir, dirToMonster) > 0.4f && distToPlayer < 35.0f) {
            isPlayerApproaching = true;
        }
    }

    // --- 2. STARTLE STATE (Reaction to flashlight / aim) ---
    if (isAimedAt && hasLOS && playerAmmo > 0 && m_startleCooldownTimer <= 0.0f) {
        if (!m_hasStartled) {
            m_hasStartled = true;
            m_startleCooldownTimer = 10.0f; // 10 seconds of immunity to startling
            m_action = MonsterAction::RETREAT;
            m_stateTimer = 0.0f;
            m_bestTreeIndex = -1;
            std::cout << "[AI] STARTLED! Playing running scream... Cooldown started (10s)" << std::endl;
        }
    } else {
        m_hasStartled = false;
    }

    // Spawn scream particles while running startled
    if (m_startleCooldownTimer > 9.0f) {
        static float screamParticleTimer = 0.0f;
        screamParticleTimer += deltaTime;
        if (screamParticleTimer >= 0.04f) {
            screamParticleTimer = 0.0f;
            glm::vec3 headCenterLocal(0.0f, 2.34f, 0.65f);
            glm::mat4 bodyMat = glm::mat4(1.0f);
            bodyMat = glm::translate(bodyMat, m_pos);
            bodyMat = glm::rotate(bodyMat, glm::radians(m_yaw), glm::vec3(0, 1, 0));
            glm::vec3 headWorld = glm::vec3(bodyMat * glm::vec4(headCenterLocal, 1.0f));
            
            // Spawn scream red vapor
            particles.SpawnParticle(headWorld, glm::vec3((rand()%100/50.0f - 1.0f)*0.6f, (rand()%100/100.0f)*1.2f, (rand()%100/50.0f - 1.0f)*0.6f), glm::vec4(0.8f, 0.0f, 0.0f, 0.7f), 0.15f, 0.7f, 0.0f);
            particles.SpawnParticle(headWorld, glm::vec3((rand()%100/50.0f - 1.0f)*0.4f, (rand()%100/100.0f)*0.7f, (rand()%100/50.0f - 1.0f)*0.4f), glm::vec4(0.1f, 0.1f, 0.1f, 0.8f), 0.2f, 0.9f, 0.0f);
        }
    }

    // --- EMOTIONAL STATE DYNAMICS ---
    if (m_estimatedPlayerAmmo == 0) {
        // Player is defenseless! Build confidence rapidly
        m_confidence += deltaTime * 0.15f;
        m_stress -= deltaTime * 0.20f;
    } else if (isPlayerReloading) {
        // Player is reloading, highly vulnerable!
        m_confidence += deltaTime * 0.25f;
        m_stress -= deltaTime * 0.15f;
    } else {
        // Standard decay/updates
        if (isAimedAt && hasLOS) {
            // Under pressure!
            m_stress += deltaTime * 0.35f;
            m_confidence -= deltaTime * 0.15f;
        } else if (isVisibleToPlayer) {
            // Spotted, slightly stressed
            m_stress += deltaTime * 0.15f;
            m_confidence -= deltaTime * 0.05f;
        } else {
            // Oblivious player: monster gains confidence and stress cools down
            m_confidence += deltaTime * 0.03f;
            m_stress -= deltaTime * 0.08f;
        }
    }

    // Clamp variables to [0.0, 1.0] range
    m_confidence = glm::clamp(m_confidence, 0.0f, 1.0f);
    m_stress = glm::clamp(m_stress, 0.0f, 1.0f);

    // --- 3. UPDATE MEMORY TIMERS ---
    m_memTimeSinceHeard += deltaTime;
    m_memTimeSinceSmelled += deltaTime;
    m_memTimeSinceSeen += deltaTime;
    m_memTimeSinceAimedAt += deltaTime;
    m_feintTimer += deltaTime;
    if (m_decisionLockTimer > 0.0f) {
        m_decisionLockTimer -= deltaTime;
    }
    if (m_startleCooldownTimer > 0.0f) {
        m_startleCooldownTimer -= deltaTime;
    }

    // Re-evaluate cover if player is far away and out of sight (threat has passed)
    if (m_action == MonsterAction::RETREAT && distToPlayer > 30.0f && (!hasLOS || m_memTimeSinceSeen > 5.0f)) {
        m_decisionLockTimer = 0.0f; // Break lock to allow transitioning back to stalk/wander
    }

    if (hasLOS) {
        m_memPlayerPos = playerPos;
        m_memTimeSinceSeen = 0.0f;
        if (isAimedAt) {
            m_memTimeSinceAimedAt = 0.0f;
        }
    }

    // Scent Check (Local gradient query instead of GPS)
    m_scentCheckTimer += deltaTime;
    if (m_scentCheckTimer >= 0.15f) {
        m_scentCheckTimer = 0.0f;
        ScentNode* bestScent = scentSystem.GetStrongestScentInRadius(m_pos, 35.0f);
        if (bestScent) {
            m_memPlayerPos = bestScent->sourcePos;
            m_memTimeSinceSmelled = 0.0f;
            
            // Get local gradient direction
            m_trackingDir = scentSystem.GetLocalScentGradient(m_pos, 30.0f);
        } else {
            m_trackingDir = glm::vec3(0.0f);
        }
    }

    // --- 4. UTILITY SCORING ---
    {
        float scoreWander = 10.0f;
        float scoreInvestigate = 0.0f;
        float scoreStalk = 0.0f;
        float scoreRetreat = 0.0f;
        float scoreClimbTree = 0.0f;
        float scoreChase = 0.0f;
        float scoreTrackScent = 0.0f;

        // RETREAT scoring:
        if (m_estimatedPlayerAmmo == 2) {
            // Very afraid! Retreat immediately if aimed at, or if player is approaching
            bool recentlySeenAndClose = (m_memTimeSinceSeen < 6.0f && (distToPlayer < 16.0f || isPlayerApproaching));
            
            if (hasLOS) {
                if (isAimedAt || isPlayerApproaching) {
                    scoreRetreat = 150.0f; // Dominates!
                } else if (isVisibleToPlayer) {
                    scoreRetreat = 110.0f; // Get to cover if we are on player's screen
                }
            }
            
            // Decay retreat score slowly to prevent state flickering/jittering at visibility boundaries
            if (scoreRetreat < 110.0f) {
                float decaySeen = 130.0f - (m_memTimeSinceSeen * 15.0f);
                float decayAimed = 120.0f - (m_memTimeSinceAimedAt * 15.0f);
                float maxDecay = std::max({0.0f, decaySeen, decayAimed});
                
                if (recentlySeenAndClose) {
                    scoreRetreat = std::max(125.0f, maxDecay);
                } else {
                    scoreRetreat = maxDecay;
                }
            }
            // Emotional adjustment
            scoreRetreat += m_stress * 30.0f - m_confidence * 20.0f;
        } else if (m_estimatedPlayerAmmo == 1) {
            // Moderately afraid, retreat if aimed at
            if (isAimedAt && hasLOS) {
                scoreRetreat = 120.0f;
            } else if (m_memTimeSinceAimedAt < 3.0f) {
                scoreRetreat = 80.0f - (m_memTimeSinceAimedAt * 15.0f);
            }
            scoreRetreat += m_stress * 25.0f - m_confidence * 15.0f;
        } else { // 0 Bullets
            // No reason to retreat!
            scoreRetreat = 0.0f; 
        }

        // Force transition out of RETREAT if we are at cover, peek completed, and no threat persists.
        if (m_action == MonsterAction::RETREAT && m_bestTreeIndex != -1) {
            glm::vec3 treeFlat(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z);
            glm::vec3 playerFlat(playerPos.x, 0.0f, playerPos.z);
            glm::vec3 awayFromPlayer = glm::normalize(treeFlat - playerFlat);
            glm::vec3 hideSpot = treeFlat + awayFromPlayer * Config::Monster::OrbitDistance;
            glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
            if (glm::distance(flatPos, hideSpot) <= 0.5f) {
                bool threatPersists = (isAimedAt && hasLOS) || isVisibleToPlayer || isPlayerApproaching;
                glm::vec3 toTree = m_assignedTreePos - playerPos;
                toTree.y = 0.0f;
                if (glm::length(toTree) > 0.1f) {
                    bool playerLookingAtTree = (glm::dot(playerFront, glm::normalize(toTree)) > 0.82f);
                    threatPersists = threatPersists || playerLookingAtTree;
                }
                
                if (m_peekTimer >= 0.6f && m_peekTimer < 900.0f && !threatPersists) {
                    scoreRetreat = 0.0f;
                }
            }
        }

        // If we decided to hide behind the trunk, maintain retreat state to stay hidden
        if (m_action == MonsterAction::RETREAT && m_bestTreeIndex != -1 && m_peekTimer > 900.0f) {
            bool threatPersists = (isAimedAt && hasLOS) || isVisibleToPlayer || isPlayerApproaching;
            glm::vec3 toTree = m_assignedTreePos - playerPos;
            toTree.y = 0.0f;
            if (glm::length(toTree) > 0.1f) {
                bool playerLookingAtTree = (glm::dot(playerFront, glm::normalize(toTree)) > 0.82f);
                threatPersists = threatPersists || playerLookingAtTree;
            }
            
            if (distToPlayer > 30.0f || !threatPersists) {
                // Player walked away or looked away, stop hiding and transition out!
                m_peekTimer = 0.0f;
                m_bestTreeIndex = -1;
                m_decisionLockTimer = 0.0f;
                scoreRetreat = 0.0f;
            } else {
                scoreRetreat = 120.0f;
            }
        }

        scoreRetreat = std::max(0.0f, scoreRetreat);

        // RELOADING behavior:
        // During reload, the player is vulnerable (effective ammo is 0). The monster charges!
        if (isPlayerReloading) {
            scoreChase = 180.0f + m_confidence * 20.0f - m_stress * 10.0f;
            scoreRetreat = 0.0f;
        }

        // CHASE scoring:
        if (m_estimatedPlayerAmmo == 0) {
            // Absolute aggression! Charge the player immediately
            scoreChase = 180.0f + m_confidence * 20.0f - m_stress * 10.0f;
        } else if (m_estimatedPlayerAmmo == 1) {
            // Only chase if player is oblivious and very close
            if (hasLOS && !isAimedAt && distToPlayer < 5.0f) {
                scoreChase = 80.0f + m_confidence * 15.0f - m_stress * 15.0f;
            }
        } else { // 2 Bullets
            // Never charge directly!
            scoreChase = 0.0f;
        }

        // FAKE CHARGE: 20% de probabilidad de cargar aunque tengas balas, para hacerte entrar en pánico.
        if (m_estimatedPlayerAmmo > 0 && isAimedAt && distToPlayer > 12.0f && distToPlayer < 25.0f) {
            if (m_stateTimer < 1.0f && rand() % 100 < 20) {
                scoreChase = 160.0f; // Amaga a morderte
            }
        }

        // TRAMPA DE EMBOSCADA: If hiding behind cover tree, player gets close (< 7.5m)
        bool isCornered = (distToPlayer < 7.5f);
        bool ambushOpportunity = (m_action == MonsterAction::RETREAT && m_bestTreeIndex != -1 && isCornered);
        if (ambushOpportunity) {
            scoreChase = 200.0f; // JUMP OUT AMBUSH! Jumps out of cover to strike.
        }

        // Surprise attack from behind (Stalk to Chase transition)
        if ((m_action == MonsterAction::STALK || m_action == MonsterAction::CHASE) && distToPlayer < 12.0f && hasLOS && !isVisibleToPlayer) {
            scoreChase = 160.0f; // Surprise leap/charge!
        }

        // STALK (Flanking) scoring:
        if (m_estimatedPlayerAmmo > 0) {
            // Good opportunity to stalk if player is looking away (very stealthy!)
            if (!isVisibleToPlayer) {
                scoreStalk = 95.0f + (m_confidence - m_stress) * 20.0f; // Stalk from behind/flanks
            } else {
                scoreStalk = 0.0f; // Visible, don't stalk
            }
        } else { // 0 Bullets
            // Don't stalk, just chase!
            scoreStalk = 0.0f;
        }

        scoreStalk = std::max(0.0f, scoreStalk);

        // CLIMB_TREE scoring:
        if (m_estimatedPlayerAmmo == 2) {
            if (m_action == MonsterAction::RETREAT && m_bestTreeIndex != -1 && distToPlayer < 8.0f && !isAimedAt) {
                scoreClimbTree = 140.0f;
            } else if (!hasLOS && m_memTimeSinceSeen > 10.0f) {
                scoreClimbTree = 60.0f; // climb to observe
            }
        } else if (m_estimatedPlayerAmmo == 1) {
            if (m_action == MonsterAction::RETREAT && m_bestTreeIndex != -1 && distToPlayer < 5.0f && !isAimedAt) {
                scoreClimbTree = 110.0f;
            }
        } else {
            // 0 Bullets: No need to climb to hide!
            scoreClimbTree = 0.0f;
        }

        // Keep climbing if we are climbing up or in canopy (don't interrupt the climb/hide sequence)
        if (m_action == MonsterAction::CLIMB_TREE && m_treeClimbHeight > 0.0f && m_stateTimer <= m_canopyWaitTime) {
            scoreClimbTree = 160.0f;
        }

        // TRACK_SCENT: Follow player path if memory is fading but scent is fresh
        bool hasScent = (m_trackingDir != glm::vec3(0.0f)) || (m_memTimeSinceSmelled < 4.0f);
        if (m_estimatedPlayerAmmo > 0 && !hasLOS && m_memTimeSinceSmelled < 15.0f && hasScent) {
            scoreTrackScent = 80.0f - (m_memTimeSinceSmelled * 1.5f);
        } else if (m_estimatedPlayerAmmo == 0) {
            // If 0 ammo and not visible, track scent to find and murder them!
            if (!hasLOS && m_memTimeSinceSmelled < 20.0f && hasScent) {
                scoreTrackScent = 150.0f; // high priority to locate
            }
        }

        // INVESTIGATE: Go to noise or last seen position
        if (!hasLOS && scoreTrackScent < 40.0f) {
            if (m_memTimeSinceHeard < 12.0f) {
                scoreInvestigate = 85.0f - (m_memTimeSinceHeard * 4.0f);
            } else if (m_memTimeSinceSeen > 5.0f && m_memTimeSinceSeen < 22.0f) {
                scoreInvestigate = 60.0f - (m_memTimeSinceSeen * 2.0f);
            }
        }

        // Hysteresis
        switch (m_action) {
            case MonsterAction::WANDER: scoreWander += 15.0f; break;
            case MonsterAction::INVESTIGATE: scoreInvestigate += 15.0f; break;
            case MonsterAction::STALK: scoreStalk += 15.0f; break;
            case MonsterAction::RETREAT: scoreRetreat += 15.0f; break;
            case MonsterAction::CLIMB_TREE: scoreClimbTree += 25.0f; break;
            case MonsterAction::CHASE: scoreChase += 15.0f; break;
            case MonsterAction::TRACK_SCENT: scoreTrackScent += 15.0f; break;
        }

        // Select Best
        MonsterAction bestAction = MonsterAction::WANDER;
        float bestScore = scoreWander;
        if (scoreInvestigate > bestScore) { bestScore = scoreInvestigate; bestAction = MonsterAction::INVESTIGATE; }
        if (scoreTrackScent > bestScore) { bestScore = scoreTrackScent; bestAction = MonsterAction::TRACK_SCENT; }
        if (scoreStalk > bestScore) { bestScore = scoreStalk; bestAction = MonsterAction::STALK; }
        if (scoreRetreat > bestScore) { bestScore = scoreRetreat; bestAction = MonsterAction::RETREAT; }
        if (scoreClimbTree > bestScore) { bestScore = scoreClimbTree; bestAction = MonsterAction::CLIMB_TREE; }
        if (scoreChase > bestScore) { bestScore = scoreChase; bestAction = MonsterAction::CHASE; }

        bool isReloadClose = isPlayerReloading && distToPlayer < 10.0f;
        bool canChangeAction = (m_decisionLockTimer <= 0.0f) ||
                               (bestAction == MonsterAction::CHASE) ||
                               isReloadClose ||
                               (bestAction == MonsterAction::RETREAT && (isAimedAt || isVisibleToPlayer));

        if (m_action != bestAction && canChangeAction) {
            std::cout << "[AI] Action Change -> ";
            switch(bestAction) {
                case MonsterAction::WANDER: std::cout << "WANDER"; break;
                case MonsterAction::INVESTIGATE: std::cout << "INVESTIGATE"; break;
                case MonsterAction::STALK: std::cout << "STALK"; break;
                case MonsterAction::RETREAT: std::cout << "RETREAT"; break;
                case MonsterAction::CLIMB_TREE: std::cout << "CLIMB_TREE"; break;
                case MonsterAction::CHASE: std::cout << "CHASE"; break;
                case MonsterAction::TRACK_SCENT: std::cout << "TRACK_SCENT"; break;
            }
            std::cout << " (Score: " << bestScore << ")" << std::endl;
            m_action = bestAction;
            m_stateTimer = 0.0f;
            m_bestTreeIndex = -1; 
            
            // Lock decisions for a random duration (1.5 - 3.5 seconds) to avoid erratic twitching
            m_decisionLockTimer = 1.5f + (rand() % 100 / 100.0f) * 2.0f;
        }
    }

    // --- 5. EXECUTE ACTION STATE ---
    switch (m_action) {
        case MonsterAction::WANDER: {
            m_stateTimer += deltaTime;
            
            // Check if we reached our current target or need a new tree target
            glm::vec3 flatPos = glm::vec3(m_pos.x, 0.0f, m_pos.z);
            glm::vec3 flatTarget = glm::vec3(m_targetPos.x, 0.0f, m_targetPos.z);
            float distToTarget = glm::distance(flatPos, flatTarget);
            
            bool needNewTarget = (distToTarget < 1.2f || m_stateTimer > 7.0f);
            
            if (needNewTarget) {
                // Find a tree that gets us closer to the player's position
                m_detectedTrees.clear();
                chunkManager.GetTreesInRange(m_pos, 25.0f, m_detectedTrees);
                
                glm::vec3 dirToPlayer = glm::normalize(playerPos - m_pos);
                float bestPatrolScore = -99999.0f;
                int chosenTreeIdx = -1;
                
                for (int i = 0; i < m_detectedTrees.size(); i++) {
                    glm::vec3 tPos(m_detectedTrees[i].x, 0.0f, m_detectedTrees[i].z);
                    float dToTree = glm::distance(flatPos, tPos);
                    if (dToTree < 2.0f) continue; // Skip trees we are already at
                    
                    glm::vec3 dirToTree = glm::normalize(tPos - flatPos);
                    float dotP = glm::dot(dirToTree, dirToPlayer);
                    
                    // Score trees that are in the direction of the player
                    if (dotP > 0.0f) {
                        float score = dotP * 30.0f - dToTree * 0.1f;
                        if (score > bestPatrolScore) {
                            bestPatrolScore = score;
                            chosenTreeIdx = i;
                        }
                    }
                }
                
                if (chosenTreeIdx != -1) {
                    glm::vec3 chosenTreePos(m_detectedTrees[chosenTreeIdx].x, 0.0f, m_detectedTrees[chosenTreeIdx].z);
                    // Hide slightly behind the tree relative to the player
                    glm::vec3 playerToTree = glm::normalize(chosenTreePos - playerPos);
                    m_targetPos = chosenTreePos + playerToTree * 1.3f;
                    m_stateTimer = 0.0f;
                    // Reset velocity to pause at the tree briefly
                    targetVelocity = glm::vec3(0.0f);
                } else {
                    // No trees in player direction, move direct to player's general direction
                    m_stateTimer = 0.0f;
                    glm::vec3 offset((rand()%100/50.0f)-1.0f, 0, (rand()%100/50.0f)-1.0f);
                    m_targetPos = playerPos + offset * 15.0f;
                }
            }
            
            // Re-calculate distance and move
            flatTarget = glm::vec3(m_targetPos.x, 0.0f, m_targetPos.z);
            distToTarget = glm::distance(flatPos, flatTarget);
            
            if (distToTarget > 1.2f) {
                // Sneaking to tree
                glm::vec3 dir = m_targetPos - flatPos;
                glm::vec3 desiredDir = glm::normalize(dir);
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                targetVelocity = desiredDir * m_speed * 0.6f; // Sneak walk speed
                m_headYaw = 0.0f; m_headPitch = 0.0f;
            } else {
                // Paused at cover: scan environment
                targetVelocity = glm::vec3(0.0f);
                m_headYaw = sin(m_stateTimer * 4.0f) * 45.0f; // Look side to side
                m_headPitch = 0.0f;
            }
            break;
        }

        case MonsterAction::CHASE: {
            glm::vec3 desiredDir = glm::normalize(playerPos - m_pos);
            m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
            float chaseSpeedMult = 2.2f;
            if (playerAmmo == 0) chaseSpeedMult = 2.8f;
            else if (isPlayerReloading && distToPlayer < 12.0f) chaseSpeedMult = 2.5f;
            targetVelocity = desiredDir * m_speed * chaseSpeedMult; // High speed sprint
            m_headYaw = 0.0f; m_headPitch = 0.0f; // Focused forward
            break;
        }

        case MonsterAction::INVESTIGATE: {
            glm::vec3 flatPos = glm::vec3(m_pos.x, 0.0f, m_pos.z);
            glm::vec3 flatTarget = glm::vec3(m_memPlayerPos.x, 0.0f, m_memPlayerPos.z);
            if (glm::distance(flatPos, flatTarget) < 1.5f) {
                targetVelocity = glm::vec3(0.0f);
                m_memTimeSinceHeard += 10.0f * deltaTime; // decay state
            } else {
                glm::vec3 desiredDir = glm::normalize(flatTarget - flatPos);
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                targetVelocity = desiredDir * m_speed * 0.7f;
            }
            // Alert rotation of the head (look side to side)
            m_headYaw = sin(m_stateTimer * 4.0f) * 40.0f;
            m_headPitch = 0.0f;
            break;
        }

        case MonsterAction::TRACK_SCENT: {
            m_stateTimer += deltaTime;

            bool scentPresent = (glm::length(m_trackingDir) > 0.01f);
            if (scentPresent) {
                // Sinusoidal wobble (drift) to make search patterns winding and organic
                float wobble = sin(m_stateTimer * 3.5f) * 0.35f;
                glm::vec3 rightDir = glm::normalize(glm::cross(m_trackingDir, glm::vec3(0.0f, 1.0f, 0.0f)));
                if (glm::length(rightDir) < 0.01f) rightDir = glm::vec3(1.0f, 0.0f, 0.0f);
                
                glm::vec3 desiredDir = glm::normalize(m_trackingDir + rightDir * wobble);
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                targetVelocity = desiredDir * Config::Monster::ScentTrackSpeed;
                
                // Sniffing animations
                m_headPitch = 25.0f + sin(m_stateTimer * 8.0f) * 10.0f;
                m_headYaw = sin(m_stateTimer * 4.0f) * 20.0f;
            } else {
                // Lost scent trail: Slow down, scan, and sniff around
                targetVelocity = glm::vec3(0.0f);
                m_headYaw = sin(m_stateTimer * 3.0f) * 45.0f; // Look left/right scanning
                m_headPitch = 30.0f + sin(m_stateTimer * 6.0f) * 15.0f; // Sniff ground intensely
                
                // If we've been searching for more than 4 seconds, force transition by decaying memory
                if (m_stateTimer > 4.0f) {
                    m_memTimeSinceSmelled = 999.0f; 
                }
            }

            // Spawn sniff dust particles
            m_sniffParticleTimer += deltaTime;
            if (m_sniffParticleTimer >= 0.06f) {
                m_sniffParticleTimer = 0.0f;
                float yawRad = glm::radians(m_yaw);
                glm::vec3 forwardDir(sin(yawRad), 0.0f, cos(yawRad));
                glm::vec3 snoutPos = m_pos + glm::vec3(0.0f, 0.4f, 0.0f) + forwardDir * 0.9f;
                particles.SpawnParticle(snoutPos, glm::vec3(0, 0.15f, 0), glm::vec4(0.35f, 0.3f, 0.25f, 0.3f), 0.04f, 0.6f, 0.0f);
            }
            break;
        }

        case MonsterAction::STALK: {
            // Wide circular flanking around the player's FOV
            glm::vec3 toMonster = m_pos - playerPos;
            toMonster.y = 0.0f;
            float distToPlayer = glm::length(toMonster);
            if (distToPlayer < 0.1f) toMonster = glm::vec3(1.0f, 0.0f, 0.0f);
            
            glm::vec3 target(0.0f);
            bool isBehindPlayer = !isVisibleToPlayer && hasLOS;
            
            if (isBehindPlayer) {
                // Player's back is turned! Creep up directly towards the player's position to ambush them.
                target = playerPos;
            } else {
                // Player is looking in our direction, or we don't have LOS. Maintain wide flanking.
                if (distToPlayer > 28.0f) {
                    // Too far: close in to flanking range laterally
                    glm::vec3 flankDir = glm::normalize(glm::cross(playerFront, glm::vec3(0, 1, 0)));
                    float side = (glm::dot(flankDir, toMonster) > 0.0f) ? 1.0f : -1.0f;
                    target = playerPos + flankDir * (24.0f * side) - playerFront * 10.0f;
                } else if (distToPlayer < 15.0f) {
                    // Too close: back away laterally to maintain flanking distance
                    glm::vec3 flankDir = glm::normalize(glm::cross(playerFront, glm::vec3(0, 1, 0)));
                    float side = (glm::dot(flankDir, toMonster) > 0.0f) ? 1.0f : -1.0f;
                    target = playerPos + flankDir * (26.0f * side) - playerFront * 15.0f;
                } else {
                    // In flanking sweet spot: orbit towards player's rear quadrant
                    glm::vec3 pRight = glm::normalize(glm::cross(playerFront, glm::vec3(0, 1, 0)));
                    float side = (glm::dot(pRight, toMonster) > 0.0f) ? 1.0f : -1.0f;
                    target = playerPos + pRight * (22.0f * side) - playerFront * 18.0f;
                }
            }

            glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
            glm::vec3 flatTarget(target.x, 0.0f, target.z);
            
            if (glm::distance(flatPos, flatTarget) < 1.5f) {
                targetVelocity = glm::vec3(0.0f);
            } else {
                glm::vec3 desiredDir = glm::normalize(flatTarget - flatPos);
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                float sneakSpeedMult = isBehindPlayer ? 0.98f : 0.85f;
                targetVelocity = desiredDir * m_speed * sneakSpeedMult; // Quiet sneak speed
            }

            // Head looks directly at player
            glm::vec3 toPlayer = glm::normalize(playerPos + glm::vec3(0.0f, 1.5f, 0.0f) - (m_pos + glm::vec3(0.0f, 2.0f, 0.0f)));
            float lookYaw = glm::degrees(atan2(toPlayer.x, toPlayer.z)) - m_yaw;
            lookYaw = std::fmod(lookYaw + 180.0f, 360.0f) - 180.0f;
            m_headYaw = glm::clamp(lookYaw, -65.0f, 65.0f);
            m_headPitch = glm::clamp((float)(-glm::degrees(asin(toPlayer.y))), -45.0f, 45.0f);
            break;
        }

        case MonsterAction::RETREAT: {
            m_stateTimer += deltaTime;
            
            // 1. Cover scan / re-evaluation logic
            // If the player gets too close to our current cover tree and has LOS, abandon it and flee/find another tree!
            if (m_bestTreeIndex != -1 && distToPlayer < 12.0f && hasLOS) {
                m_bestTreeIndex = -1; // Force choosing a new tree
                std::cout << "[AI] Player too close to cover. Re-routing to a new tree..." << std::endl;
            }
            
            bool needNewCover = (m_bestTreeIndex == -1);
            if (!needNewCover) {
                glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 flatTree(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z);
                if (glm::distance(flatPos, flatTree) < 1.8f) {
                    // Reached trunk area, transition to orbiting
                }
            }
            
            if (needNewCover) {
                m_detectedTrees.clear();
                chunkManager.GetTreesInRange(m_pos, 65.0f, m_detectedTrees);
                float bestCoverScore = -99999.0f;
                m_bestTreeIndex = -1;
                
                glm::vec3 awayFromPlayer = glm::normalize(m_pos - playerPos);
                
                for(int i = 0; i < m_detectedTrees.size(); i++) {
                    glm::vec3 tPos(m_detectedTrees[i].x, 0.0f, m_detectedTrees[i].z);
                    float dToMe = glm::distance(tPos, glm::vec3(m_pos.x, 0, m_pos.z));
                    
                    // Force the monster to choose trees that are far away from its current position to avoid local peek-a-boos
                    if (dToMe < 15.0f) continue;
                    
                    // Prevent selecting the same tree we are currently at or trees very close to it when chaining retreats
                    if (m_action == MonsterAction::RETREAT && glm::distance(tPos, m_assignedTreePos) < 6.0f) continue;
                    
                    float dToPlayer = glm::distance(tPos, glm::vec3(playerPos.x, 0, playerPos.z));
                    glm::vec3 dToTree = glm::normalize(tPos - glm::vec3(m_pos.x, 0, m_pos.z));
                    float dotDir = glm::dot(dToTree, awayFromPlayer);
                    
                    // Allow trees in front and to the sides (dotDir > -0.4f) to avoid running back towards the player
                    if (dotDir > -0.4f) {
                        // Reward lateral trees to break straight lines and reward longer distance to fade into fog
                        float lateral = 1.0f - std::abs(dotDir);
                        float score = dToMe * 1.5f + dToPlayer * 1.0f + (lateral * 45.0f);
                        
                        if (score > bestCoverScore) {
                            bestCoverScore = score;
                            m_bestTreeIndex = i;
                        }
                    }
                }
                
                if (m_bestTreeIndex != -1) {
                    m_assignedTreePos = glm::vec3(m_detectedTrees[m_bestTreeIndex].x, 0.0f, m_detectedTrees[m_bestTreeIndex].z);
                }
            }

            if (m_bestTreeIndex != -1) {
                glm::vec3 treeFlat(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z);
                glm::vec3 playerFlat(playerPos.x, 0.0f, playerPos.z);
                glm::vec3 awayFromPlayer = glm::normalize(treeFlat - playerFlat);
                glm::vec3 hideSpot = treeFlat + awayFromPlayer * Config::Monster::OrbitDistance;
                
                glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 diff = hideSpot - flatPos;
                
                if (glm::length(diff) > 0.5f) {
                    // Dashing to cover
                    glm::vec3 desiredDir = glm::normalize(diff);
                    
                    // Feint & Surround system when player has LOS and is close
                    bool isFeinting = false;
                    if (hasLOS && distToPlayer < 22.0f) {
                        if (m_feintTimer > 3.5f) {
                            m_feintTimer = -0.8f; // 0.8 seconds of feint
                            m_feintAngle = (rand() % 2 == 0) ? 90.0f : -90.0f;
                        }
                        
                        if (m_feintTimer < 0.0f) {
                            // Phase 1: Feint (run sideways relative to the target path)
                            float rad = glm::radians(m_feintAngle);
                            glm::vec3 rotDir(
                                desiredDir.x * cos(rad) - desiredDir.z * sin(rad),
                                0.0f,
                                desiredDir.x * sin(rad) + desiredDir.z * cos(rad)
                            );
                            desiredDir = glm::normalize(rotDir);
                            isFeinting = true;
                        } 
                        else if (m_feintTimer < 1.0f) {
                            // Phase 2: Surround (circle player to flank/rear)
                            glm::vec3 toPlayer = playerPos - m_pos;
                            toPlayer.y = 0.0f;
                            if (glm::length(toPlayer) > 0.1f) {
                                glm::vec3 surroundDir = glm::normalize(glm::cross(toPlayer, glm::vec3(0, 1, 0)));
                                if (glm::dot(surroundDir, m_velocity) < 0.0f) {
                                    surroundDir = -surroundDir;
                                }
                                desiredDir = surroundDir;
                                isFeinting = true;
                            }
                        }
                    }

                    // Curved/Dodging pathing when running normally to cover (not feinting)
                    if (!isFeinting) {
                        glm::vec3 rightDir = glm::normalize(glm::cross(desiredDir, glm::vec3(0.0f, 1.0f, 0.0f)));
                        if (glm::length(rightDir) < 0.01f) rightDir = glm::vec3(1.0f, 0.0f, 0.0f);
                        
                        // Weave erratically using composite zig-zag and slow wave
                        float zigzag = sin(m_stateTimer * 8.0f) * 0.45f;
                        float wave = sin(m_stateTimer * 1.5f) * 0.35f;
                        
                        // Straighten out when close to the tree (arrivalFactor clamps to 0 when near hideSpot)
                        float distToCover = glm::length(diff);
                        float arrivalFactor = glm::clamp(distToCover / 4.0f, 0.0f, 1.0f);
                        
                        desiredDir = desiredDir + rightDir * (zigzag + wave) * arrivalFactor;
                        desiredDir = glm::normalize(desiredDir);
                    }

                    m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                    float retreatSpeedMult = 2.2f;
                    if (m_estimatedPlayerAmmo == 2) retreatSpeedMult = 2.8f;
                    else if (m_estimatedPlayerAmmo == 1) retreatSpeedMult = 2.4f;
                    targetVelocity = desiredDir * m_speed * retreatSpeedMult;
                    m_headYaw = 0.0f; m_headPitch = 0.0f;
                    m_peekTimer = 0.0f;
                } else {
                    // --- AT COVER: Look Back, Peek, & Choice to Hide or Chain ---
                    if (m_peekTimer > 900.0f) {
                        // We decided to hide and orbit behind this tree trunk!
                        targetVelocity = glm::vec3(0.0f);
                        LookAt(m_assignedTreePos);
                        
                        // Steer orbit to match moving player
                        glm::vec3 orbitDelta = hideSpot - m_pos;
                        if (glm::length(orbitDelta) > 0.08f) {
                            targetVelocity = glm::normalize(orbitDelta) * m_speed * 1.6f; // Orbit faster
                        }
                    } else {
                        m_peekTimer += deltaTime;
                        if (m_peekTimer < 0.6f) {
                            // Stop and turn the body to look directly at the player
                            targetVelocity = glm::vec3(0.0f);
                            LookAt(playerPos);
                            m_headYaw = 0.0f;
                            m_headPitch = 0.0f;
                        } else {
                            // After 0.6 seconds of looking back, evaluate threat
                            bool threatPersists = (isAimedAt && hasLOS) || isVisibleToPlayer || isPlayerApproaching;
                            glm::vec3 toTree = m_assignedTreePos - playerPos;
                            toTree.y = 0.0f;
                            if (glm::length(toTree) > 0.1f) {
                                bool playerLookingAtTree = (glm::dot(playerFront, glm::normalize(toTree)) > 0.82f);
                                threatPersists = threatPersists || playerLookingAtTree;
                            }

                            if (threatPersists) {
                                // Player is still aiming at us, looking at us, or approaching!
                                // 50% chance to chain retreat, 50% chance to hide behind this trunk
                                if (rand() % 100 < 50) {
                                    // Chain retreat: Invalidate current cover, reset timer, reset decision lock to force immediate re-routing to a new tree.
                                    m_bestTreeIndex = -1;
                                    m_peekTimer = 0.0f;
                                    m_decisionLockTimer = 0.0f;
                                    std::cout << "[AI] Threat persists! Chaining retreat to a new tree..." << std::endl;
                                } else {
                                    // Stay and hide behind this tree trunk!
                                    m_peekTimer = 999.0f; 
                                    std::cout << "[AI] Threat persists but decided to HIDE behind this trunk!" << std::endl;
                                }
                            } else {
                                // Threat has subsided or player looked away!
                                // Break the decision lock to immediately transition to STALK and flank
                                m_decisionLockTimer = 0.0f;
                                m_peekTimer = 0.0f;
                                
                                // Orbit/fallback behavior in this frame until the state machine transitions
                                targetVelocity = glm::vec3(0.0f);
                                LookAt(m_assignedTreePos);
                                
                                // Steer orbit to match moving player
                                glm::vec3 orbitDelta = hideSpot - m_pos;
                                if (glm::length(orbitDelta) > 0.08f) {
                                    targetVelocity = glm::normalize(orbitDelta) * m_speed * 1.6f; // Orbit faster
                                }
                            }
                        }
                    }
                }
            } else {
                // No tree, flee circularly in a curving arc to break the straight line!
                glm::vec3 awayDir = glm::normalize(m_pos - playerPos);
                glm::vec3 rightDir = glm::normalize(glm::cross(awayDir, glm::vec3(0, 1, 0)));
                
                // Curve direction based on a slow wave (changing left/right curving over time)
                float curveWeight = 0.8f;
                float curveDirection = (sin(m_stateTimer * 0.7f) > 0.0f) ? 1.0f : -1.0f;
                
                glm::vec3 runDir = awayDir * 0.6f + rightDir * curveWeight * curveDirection;
                runDir = glm::normalize(runDir);
                
                // Add high-frequency lateral oscillation (zig-zag) to make aiming harder
                runDir += rightDir * (float)(sin(m_stateTimer * 8.0f) * 0.45f);
                runDir = glm::normalize(runDir);
                
                // Feint & Surround system when no tree cover
                if (hasLOS && distToPlayer < 22.0f) {
                    if (m_feintTimer > 3.5f) {
                        m_feintTimer = -0.8f;
                        m_feintAngle = (rand() % 2 == 0) ? 90.0f : -90.0f;
                    }
                    
                    if (m_feintTimer < 0.0f) {
                        float rad = glm::radians(m_feintAngle);
                        glm::vec3 rotDir(
                            runDir.x * cos(rad) - runDir.z * sin(rad),
                            0.0f,
                            runDir.x * sin(rad) + runDir.z * cos(rad)
                        );
                        runDir = glm::normalize(rotDir);
                    }
                    else if (m_feintTimer < 1.0f) {
                        glm::vec3 toPlayer = playerPos - m_pos;
                        toPlayer.y = 0.0f;
                        if (glm::length(toPlayer) > 0.1f) {
                            glm::vec3 surroundDir = glm::normalize(glm::cross(toPlayer, glm::vec3(0, 1, 0)));
                            if (glm::dot(surroundDir, m_velocity) < 0.0f) {
                                surroundDir = -surroundDir;
                            }
                            runDir = surroundDir;
                        }
                    }
                }
                
                m_targetYaw = glm::degrees(atan2(runDir.x, runDir.z));
                float retreatSpeedMult = 1.8f;
                if (m_estimatedPlayerAmmo == 2) retreatSpeedMult = 2.4f;
                else if (m_estimatedPlayerAmmo == 1) retreatSpeedMult = 2.0f;
                targetVelocity = runDir * m_speed * retreatSpeedMult;
                
                // Gira la cabeza para mirar por encima del hombro al jugador mientras huye
                glm::vec3 toPlayer = playerPos - m_pos;
                float lookBackYaw = glm::degrees(atan2(toPlayer.x, toPlayer.z)) - m_yaw;
                lookBackYaw = std::fmod(lookBackYaw + 180.0f, 360.0f) - 180.0f;
                m_headYaw = glm::clamp(lookBackYaw, -75.0f, 75.0f);
                m_headPitch = 0.0f;
            }
            break;
        }

        case MonsterAction::CLIMB_TREE: {
            m_headYaw = 0.0f;
            
            if (m_bestTreeIndex == -1) {
                m_detectedTrees.clear();
                chunkManager.GetTreesInRange(m_pos, 25.0f, m_detectedTrees);
                if (!m_detectedTrees.empty()) {
                    m_bestTreeIndex = rand() % m_detectedTrees.size();
                    m_assignedTreePos = glm::vec3(m_detectedTrees[m_bestTreeIndex].x, 0.0f, m_detectedTrees[m_bestTreeIndex].z);
                    m_assignedTreeScale = m_detectedTrees[m_bestTreeIndex].w;
                    m_isClimbing = false; // Walk there first
                    m_treeClimbHeight = 0.0f;
                    m_stateTimer = 0.0f;
                    std::cout << "[AI] Decided to climb tree. Walking to tree base..." << std::endl;
                } else {
                    m_memTimeSinceSeen = 0.0f; // force state change
                    targetVelocity = glm::vec3(0.0f);
                }
            } else {
                glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 flatTree(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z);
                
                float distToTree = glm::distance(flatPos, flatTree);
                float targetClimbHeight = 9.5f * m_assignedTreeScale;
                
                if (!m_isClimbing && m_treeClimbHeight <= 0.0f) {
                    // Phase 1: Walk to tree base
                    if (distToTree > 0.7f) {
                        glm::vec3 desiredDir = glm::normalize(flatTree - flatPos);
                        m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                        targetVelocity = desiredDir * m_speed * 1.0f; // Normal run/walk speed to tree
                        m_headPitch = 0.0f;
                    } else {
                        // Snap directly to trunk and start climbing
                        m_pos.x = m_assignedTreePos.x;
                        m_pos.z = m_assignedTreePos.z;
                        targetVelocity = glm::vec3(0.0f);
                        m_isClimbing = true;
                        m_treeClimbHeight = 0.05f;
                        std::cout << "[AI] Reached tree base. Starting vertical climb..." << std::endl;
                    }
                } else if (m_isClimbing) {
                    // Phase 2: Climb up
                    targetVelocity = glm::vec3(0.0f);
                    m_treeClimbHeight += 7.0f * deltaTime;
                    m_headPitch = 45.0f; // Look down while climbing
                    if (m_treeClimbHeight >= targetClimbHeight) {
                        m_treeClimbHeight = targetClimbHeight;
                        m_isClimbing = false;
                        m_stateTimer = 0.0f;
                        // Choose a random observation wait time between 6.0 and 14.0 seconds
                        m_canopyWaitTime = 6.0f + (rand() % 100 / 100.0f) * 8.0f;
                        std::cout << "[AI] Reached canopy leaves, observing player (Wait time: " << m_canopyWaitTime << "s)..." << std::endl;
                    }
                } else {
                    // Phase 3: At canopy, observing
                    targetVelocity = glm::vec3(0.0f);
                    m_stateTimer += deltaTime;
                    m_headPitch = 30.0f;
                    m_headYaw = sin(m_stateTimer * 2.0f) * 45.0f; // Scan from canopy
                    
                    // Drop ambush logic:
                    // 1. If player ammo is 0, or player is very close (dist2D < 3.5f), drop down!
                    float dist2D = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));
                    bool playerUnderTree = (dist2D < 3.5f);
                    bool shouldDropAttack = (playerAmmo == 0) || playerUnderTree;
                    
                    if (shouldDropAttack) {
                        m_treeClimbHeight = 0.0f;
                        m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
                        m_bestTreeIndex = -1;
                        m_action = MonsterAction::CHASE;
                        m_stateTimer = 0.0f;
                        
                        // Spawn leaf burst particles
                        for (int i = 0; i < 30; i++) {
                            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/100.0f)*4.0f, (rand()%100/50.0f - 1.0f)*3.0f);
                            glm::vec4 pCol = (rand()%2 == 0) ? glm::vec4(0.15f, 0.35f, 0.1f, 0.9f) : glm::vec4(0.35f, 0.3f, 0.2f, 0.8f);
                            particles.SpawnParticle(m_pos + glm::vec3(0, 1.0f, 0), pVel, pCol, 0.15f, 1.2f, -9.8f);
                        }
                        std::cout << "[AI] DROP AMBUSH! Spawning leaf burst and chasing player..." << std::endl;
                        break;
                    }
                    
                    // 2. If the player is looking towards the tree, stay hidden (cap stateTimer)
                    if (isVisibleToPlayer) {
                        if (m_stateTimer > (m_canopyWaitTime - 0.5f)) {
                            m_stateTimer = m_canopyWaitTime - 0.5f; // Prevent moving to Phase 4 (climb down)
                        }
                    }
                    
                    if (m_stateTimer > m_canopyWaitTime) {
                        // Phase 4: Climb down
                        m_treeClimbHeight -= 10.0f * deltaTime;
                        m_headPitch = 25.0f;
                        if (m_treeClimbHeight <= 0.0f) {
                            m_treeClimbHeight = 0.0f;
                            m_bestTreeIndex = -1; // Done
                            m_memTimeSinceSeen = 0.0f; // Reset search memory
                            std::cout << "[AI] Climb down completed. Returning to search." << std::endl;
                        }
                    }
                }
            }
            break;
        }
    }

    // Apply movement inertia by smoothly interpolating m_velocity towards targetVelocity.
    // If the monster is climbing trees vertically, snap velocity directly to avoid drifting.
    if (m_action != MonsterAction::CLIMB_TREE) {
        // Apply obstacle avoidance directly to the target velocity for smooth predictive steering.
        targetVelocity = ApplyObstacleAvoidance(targetVelocity, chunkManager);
        
        float lerpFactor = 5.0f; // Smooth acceleration/deceleration weight
        m_velocity = glm::mix(m_velocity, targetVelocity, glm::clamp(deltaTime * lerpFactor, 0.0f, 1.0f));
    } else {
        m_velocity = targetVelocity;
    }

    if (m_action != MonsterAction::CLIMB_TREE) {
        if (m_treeClimbHeight > 0.0f) {
            // Just fell/transitioned out of tree, spawn leaf burst particles!
            for (int i = 0; i < 15; i++) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.0f, (rand()%100/100.0f)*3.0f, (rand()%100/50.0f - 1.0f)*2.0f);
                glm::vec4 pCol = (rand()%2 == 0) ? glm::vec4(0.15f, 0.35f, 0.1f, 0.9f) : glm::vec4(0.35f, 0.3f, 0.2f, 0.8f);
                particles.SpawnParticle(m_pos + glm::vec3(0, 1.0f, 0), pVel, pCol, 0.12f, 1.0f, -9.8f);
            }
            m_treeClimbHeight = 0.0f;
        }
    }

    // Tree collision caching
    if (glm::length(m_velocity) > 0.1f && m_action != MonsterAction::CLIMB_TREE) {
        m_nearbyTreesCache.clear();
        chunkManager.GetTreesInRange(m_pos + m_velocity * 0.1f, 3.0f, m_nearbyTreesCache); 
    }

physics_tick:
    // Ensure Visual Yaw follows Physics Yaw
    m_visualYaw = m_yaw;

    if (glm::length(m_velocity) > 0.01f) {
        // Smooth Rotation (Physics)
        float rotSpeed = 5.0f * deltaTime;
        float yawDiff = std::fmod(m_targetYaw - m_yaw, 360.0f);
        if (yawDiff < -180) yawDiff += 360;
        else if (yawDiff > 180) yawDiff -= 360;
        
        m_yaw += yawDiff * rotSpeed;
        m_visualYaw = m_yaw;

        // Integrate Position
        glm::vec3 nextPos = m_pos + m_velocity * deltaTime;
        
        bool isStuckOnTree = false;
        glm::vec3 stuckTreePos(0.0f);
        float stuckMinD = 0.0f;

        // Resolve Collisions using Cached Trees (as double-safety check)
        if (!m_nearbyTreesCache.empty()) {
            for (const auto& t : m_nearbyTreesCache) {
                float dist = glm::length(glm::vec2(nextPos.x - t.x, nextPos.z - t.z));
                float minD = 0.5f + (0.6f * t.w);
                if (dist < minD) {
                     glm::vec2 push = glm::normalize(glm::vec2(nextPos.x - t.x, nextPos.z - t.z)) * (minD - dist);
                     nextPos.x += push.x;
                     nextPos.z += push.y;
                     
                     isStuckOnTree = true;
                     stuckTreePos = glm::vec3(t.x, m_pos.y, t.z);
                     stuckMinD = minD;
                }
            }
        }

        // Project velocity onto tangent of the tree to slide smoothly when colliding
        if (isStuckOnTree && glm::length(m_velocity) > 0.1f) {
            glm::vec3 toTree = stuckTreePos - m_pos;
            toTree.y = 0.0f;
            if (glm::length(toTree) > 0.01f) {
                glm::vec3 normToTree = glm::normalize(toTree);
                float dotP = glm::dot(m_velocity, normToTree);
                if (dotP > 0.0f) {
                    m_velocity -= normToTree * dotP;
                }
            }
        }

        // Stuck Detector: If trying to move but actual progress is tiny (blocked by tree/obstacle)
        float expectedDist = glm::length(m_velocity) * deltaTime;
        float actualDist = glm::distance(nextPos, m_prevPos);
        if (glm::length(m_velocity) > 0.5f && actualDist < expectedDist * 0.15f) {
            m_stuckTimer += deltaTime;
        } else {
            m_stuckTimer = 0.0f;
        }

        if (m_stuckTimer > 0.20f) { // Stuck for more than 200ms
            if (isStuckOnTree) {
                // If in retreat, invalidate current tree so we choose another
                if (m_action == MonsterAction::RETREAT) {
                    m_bestTreeIndex = -1;
                }
                
                // Force a lateral slide around the tree trunk
                glm::vec3 toTree = stuckTreePos - m_pos;
                toTree.y = 0.0f;
                if (glm::length(toTree) > 0.01f) {
                    glm::vec3 lateralDir = glm::normalize(glm::cross(toTree, glm::vec3(0, 1, 0)));
                    // Choose the lateral direction that aligns better with current velocity direction
                    if (glm::dot(lateralDir, m_velocity) < 0.0f) {
                        lateralDir = -lateralDir;
                    }
                    // Apply a strong push to get around the trunk
                    m_velocity = lateralDir * m_speed * 1.5f;
                    nextPos = m_pos + m_velocity * deltaTime;
                    
                    // Re-resolve collision for the lateral move
                    float d = glm::length(glm::vec2(nextPos.x - stuckTreePos.x, nextPos.z - stuckTreePos.z));
                    if (d < stuckMinD) {
                        glm::vec2 push = glm::normalize(glm::vec2(nextPos.x - stuckTreePos.x, nextPos.z - stuckTreePos.z)) * (stuckMinD - d);
                        nextPos.x += push.x;
                        nextPos.z += push.y;
                    }
                }
            } else {
                // Not stuck on a tree, but blocked by terrain. Add a random lateral kick
                glm::vec3 runDir = glm::normalize(m_velocity);
                glm::vec3 rightDir = glm::normalize(glm::cross(runDir, glm::vec3(0, 1, 0)));
                m_velocity = rightDir * m_speed * ((rand() % 2 == 0) ? 1.0f : -1.0f);
                nextPos = m_pos + m_velocity * deltaTime;
            }
            m_stuckTimer = 0.0f; // Reset after kicking
        }
        
        float limit = (Config::World::MapRadius - 1) * Config::World::ChunkSize * Config::World::ChunkScale;
        if (abs(nextPos.x) < limit && abs(nextPos.z) < limit) {
             m_pos = nextPos;
        }
    }
    
    // Terrain Snap with Climb Height
    m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z) + m_treeClimbHeight;
    m_visualPos = m_pos;

    // Upgraded Real-time Procedural Animations Upload
    AnimateMesh();
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_meshVertices.size() * sizeof(float), m_meshVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Eyes);
    glBufferData(GL_ARRAY_BUFFER, m_eyeVertices.size() * sizeof(float), m_eyeVertices.data(), GL_DYNAMIC_DRAW);
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
        m_decisionLockTimer = 0.0f; // Break decision lock on taking damage
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
        if (glm::distance(A, center) < 0.1f) continue;
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

glm::vec3 Monster::ApplyObstacleAvoidance(glm::vec3 desiredVel, ChunkManager& chunkManager) {
    if (glm::length(desiredVel) < 0.01f) return desiredVel;
    
    glm::vec3 forward = glm::normalize(desiredVel);
    glm::vec3 avoidForce(0.0f);
    
    std::vector<glm::vec4> nearbyTrees;
    // Query trees in a 5.0 meter radius from the monster
    chunkManager.GetTreesInRange(m_pos, 5.0f, nearbyTrees);
    
    float maxAvoidDistance = 4.0f;
    
    for (const auto& t : nearbyTrees) {
        glm::vec3 treePos(t.x, m_pos.y, t.z);
        glm::vec3 toTree = treePos - m_pos;
        
        // Project onto forward vector
        float projection = glm::dot(toTree, forward);
        
        // If tree is in front of the monster
        if (projection > 0.0f && projection < maxAvoidDistance) {
            glm::vec3 lateral = toTree - forward * projection;
            float lateralDist = glm::length(lateral);
            
            float treeRadius = 0.5f + (0.6f * t.w);
            float safeMargin = treeRadius + 0.6f; // Safe offset
            
            if (lateralDist < safeMargin) {
                // Determine direction to steer away
                glm::vec3 steerDir;
                if (lateralDist > 0.01f) {
                    steerDir = glm::normalize(-lateral);
                } else {
                    // Tree is perfectly in front, steer to the side (use cross product with UP)
                    steerDir = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
                }
                
                // Avoidance strength is stronger the closer we are to the tree
                float strength = (maxAvoidDistance - projection) / maxAvoidDistance * Config::Monster::SteerAvoidanceForce;
                avoidForce += steerDir * strength;
            }
        }
    }
    
    return desiredVel + avoidForce;
}

