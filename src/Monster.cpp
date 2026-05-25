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

static inline glm::vec3 SafeNormalize(const glm::vec3& v, const glm::vec3& fallback = glm::vec3(0.0f, 0.0f, 1.0f)) {
    float len = glm::length(v);
    if (!std::isnan(len) && len > 0.0001f) {
        return v / len;
    }
    return fallback;
}

static inline glm::vec2 SafeNormalize(const glm::vec2& v, const glm::vec2& fallback = glm::vec2(1.0f, 0.0f)) {
    float len = glm::length(v);
    if (!std::isnan(len) && len > 0.0001f) {
        return v / len;
    }
    return fallback;
}

static inline glm::vec3 SafeCross(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& fallback = glm::vec3(1.0f, 0.0f, 0.0f)) {
    glm::vec3 c = glm::cross(v1, v2);
    float len = glm::length(c);
    if (!std::isnan(len) && len > 0.0001f) {
        return c / len;
    }
    return fallback;
}

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
      m_canopyWaitTime(8.0f), m_confidence(0.5f), m_stress(0.0f),
      m_chooseLeftFlank(false), m_flankDecisionTimer(0.0f), m_tacticalTargetOffset(0.0f),
      m_exposure(0.0f), m_isCrouching(false), m_crouchFactor(0.0f), m_distractionTimer(5.0f), m_spiralSearchTimer(0.0f), m_spiralSearchRadius(3.0f),
      m_isEnraged(false), m_rageTimer(0.0f), m_isPanicked(false), m_panicTimer(0.0f),
      m_eyeColor(1.0f, 0.0f, 0.0f), m_eyeBrightness(1.0f), m_antiCampCenter(startPos), m_antiCampTimer(0.0f),
      m_bleedTimer(0.0f), m_fakeChargeTimer(-1.0f), m_climbCooldownTimer(0.0f), m_shouldScream(false), m_spiralSearchAngle(0.0f), m_knownPlayerTreePos(0.0f),
      m_lastScentWorldPos(0.0f), m_hasLastScent(false), m_leafDropTimer(0.0f)
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
    
    float hipBend = 0.0f;
    float kneeBend = 0.0f;
    float heightOffset = 0.0f;
    if (m_crouchFactor > 0.001f) {
        bodyLean = glm::mix(bodyLean, 0.6f, m_crouchFactor);
        hipBend = -0.5f * m_crouchFactor;
        kneeBend = 0.9f * m_crouchFactor;
        heightOffset = -0.4f * m_crouchFactor;
    }

    // Pivots
    glm::vec3 lShoulder(-0.4f, 2.0f + heightOffset, 0.4f);
    glm::vec3 rShoulder(0.4f, 2.0f + heightOffset, 0.4f);
    glm::vec3 lHip(-0.18f, 1.15f + heightOffset, 0.0f);
    glm::vec3 rHip(0.18f, 1.15f + heightOffset, 0.0f);
    glm::vec3 lKnee(-0.18f, 0.75f + heightOffset, 0.0f);
    glm::vec3 rKnee(0.18f, 0.75f + heightOffset, 0.0f);
    glm::vec3 neckPivot(0.0f, 2.16f + heightOffset, 0.5f);
    glm::vec3 hipsPivot(0.0f, 1.2f + heightOffset, 0.0f);
    
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
            box.Pos = rotatePivotX(box.Pos, lHip, limbSwingL + hipBend);
            box.Rot.x += limbSwingL + hipBend;
            isUpperBody = false;
        } 
        else if (box.Name.find("L_CALF") != std::string::npos) {
            // Fold knee back during backswing
            float calfSwing = (limbSwingL < 0.0f) ? -limbSwingL * 1.2f : 0.0f;
            box.Pos = rotatePivotX(box.Pos, lKnee, calfSwing + kneeBend);
            box.Rot.x += calfSwing + kneeBend;
            // Now rotate around hip
            box.Pos = rotatePivotX(box.Pos, lHip, limbSwingL + hipBend);
            box.Rot.x += limbSwingL + hipBend;
            isUpperBody = false;
        } 
        else if (box.Name.find("R_LEG") != std::string::npos) {
            box.Pos = rotatePivotX(box.Pos, rHip, limbSwingR + hipBend);
            box.Rot.x += limbSwingR + hipBend;
            isUpperBody = false;
        } 
        else if (box.Name.find("R_CALF") != std::string::npos) {
            float calfSwing = (limbSwingR < 0.0f) ? -limbSwingR * 1.2f : 0.0f;
            box.Pos = rotatePivotX(box.Pos, rKnee, calfSwing + kneeBend);
            box.Rot.x += calfSwing + kneeBend;
            // Now rotate around hip
            box.Pos = rotatePivotX(box.Pos, rHip, limbSwingR + hipBend);
            box.Rot.x += limbSwingR + hipBend;
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

    for (auto& box : animatedBoxes) {
        if (box.Name.find("EYE") != std::string::npos) {
            box.Color = m_eyeColor * m_eyeBrightness;
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

        // If volume is Config::Gameplay::GunshotSoundRange, it's a shotgun blast! Incur bullet count estimation
        if (std::abs(volume - Config::Gameplay::GunshotSoundRange) < 0.1f) {
            m_estimatedPlayerAmmo = std::max(0, m_estimatedPlayerAmmo - 1);
            std::cout << "[AI] Heard shotgun blast! Estimated player ammo: " << m_estimatedPlayerAmmo << std::endl;
        } else if (std::abs(volume - 120.0f) < 0.1f) {
            m_estimatedPlayerAmmo = std::max(0, m_estimatedPlayerAmmo - 1);
            std::cout << "[AI] Heard normal gunshot! Estimated ammo: " << m_estimatedPlayerAmmo << std::endl;
        }
    }
}

void Monster::Update(float deltaTime, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir,
                     ChunkManager& chunkManager, ScentSystem& scentSystem, ParticleSystem& particles,
                     glm::vec3 playerVelocity, int playerAmmo, bool isPlayerReloading,
                     bool isPlayerClimbing, glm::vec3 playerClimbingTreePos,
                     bool isFlashlightOn) 
{
    if (m_isDead) return;
    m_prevPos = m_pos;

    float distToPlayer = glm::distance(m_pos, playerPos);
    
    // Aim Detection
    glm::vec3 dirToMonster = glm::normalize(m_pos - playerPos);
    float dotProd = glm::dot(playerFront, dirToMonster);
    bool isAimedAt = (dotProd > 0.96f); // Tightened: Must aim directly at it

    // Vision Scanner (Field of View + Max Distance Check)
    bool hasLOS = false;
    if (distToPlayer < 70.0f) { // Max vision range (fog limit)
        // Calculate monster's look direction (body yaw + head yaw and head pitch)
        float totalYawRad = glm::radians(m_yaw + m_headYaw);
        float pitchRad = glm::radians(m_headPitch);
        glm::vec3 lookDir(
            sin(totalYawRad) * cos(pitchRad),
            -sin(pitchRad),
            cos(totalYawRad) * cos(pitchRad)
        );
        
        glm::vec3 dirToPlayerVec = playerPos - m_pos;
        if (glm::length(dirToPlayerVec) > 0.01f) {
            glm::vec3 dirToPlayer = glm::normalize(dirToPlayerVec);
            
            // FOV: 180 degrees peripheral when close (< 12m), 100 degrees normal when far
            float fovLimit = (distToPlayer < 12.0f) ? 90.0f : 50.0f;
            float cosAngle = glm::dot(lookDir, dirToPlayer);
            if (cosAngle > cos(glm::radians(fovLimit))) {
                hasLOS = CheckLineOfSight(playerPos, chunkManager);
            }
        }
    }
    
    // Flashlight Detection: If player has flashlight ON, is within range (35m), and has clear line of sight
    bool detectedByFlashlight = isFlashlightOn && (distToPlayer < 35.0f) && CheckLineOfSight(playerPos, chunkManager);
    if (detectedByFlashlight) {
        hasLOS = true;
    }
    m_hasVisualContact = hasLOS;

    if (m_shouldScream) {
        m_shouldScream = false;
        // Spawn big burst of red scream/rage vapor from the monster's head
        glm::vec3 headCenterLocal(0.0f, 2.34f, 0.65f);
        glm::mat4 bodyMat = glm::mat4(1.0f);
        bodyMat = glm::translate(bodyMat, m_pos);
        bodyMat = glm::rotate(bodyMat, glm::radians(m_yaw), glm::vec3(0, 1, 0));
        glm::vec3 headWorld = glm::vec3(bodyMat * glm::vec4(headCenterLocal, 1.0f));
        for (int i = 0; i < 25; ++i) {
            glm::vec3 pVel(
                (rand() % 100 / 50.0f - 1.0f) * 2.5f,
                (rand() % 100 / 50.0f - 0.2f) * 3.0f,
                (rand() % 100 / 50.0f - 1.0f) * 2.5f
            );
            particles.SpawnParticle(headWorld, pVel, glm::vec4(0.9f, 0.0f, 0.0f, 0.8f), 0.15f, 0.8f, 0.0f);
        }
    }

    // Senses: Update known player tree if player is climbing
    if (m_climbCooldownTimer > 0.0f) {
        m_climbCooldownTimer -= deltaTime;
    }

    if (isPlayerClimbing && m_climbCooldownTimer <= 0.0f) {
        bool playerMoving = (glm::length(playerVelocity) > 0.1f);
        bool heardClimbing = (distToPlayer < 15.0f) && (playerMoving || m_memTimeSinceHeard < 2.0f);
        bool closeProximity = (distToPlayer < 8.0f);
        bool heardGunshot = (m_memTimeSinceHeard < 1.0f); // Heard player shooting from the tree
        
        if (hasLOS || heardClimbing || closeProximity || heardGunshot || (m_memTimeSinceSeen < 2.0f && distToPlayer < 20.0f)) {
            m_knownPlayerTreePos = playerClimbingTreePos;
        } else {
            // Gradually forget player's tree position if silent and out of sight
            if (m_memTimeSinceSeen > 3.0f && m_memTimeSinceHeard > 3.0f) {
                m_knownPlayerTreePos = glm::vec3(0.0f);
            }
        }
    } else {
        m_knownPlayerTreePos = glm::vec3(0.0f);
    }

    // Cache nearby trees once per frame at the start of Update (only disable if actually climbing vertically)
    m_nearbyTreesCache.clear();
    bool isVerticallyClimbing = (m_action == MonsterAction::CLIMB_TREE && (m_isClimbing || m_treeClimbHeight > 0.0f));
    if (!isVerticallyClimbing) {
        chunkManager.GetTreesInRange(m_pos, 5.0f, m_nearbyTreesCache);
    }

    // --- SISTEMA ANTI-CAMPERO ---
    if (glm::distance(playerPos, m_antiCampCenter) > 4.0f) {
        // El jugador se mueve bien, enfriamos el timer gradualmente para evitar wiggles
        m_antiCampCenter = playerPos;
        m_antiCampTimer = std::max(0.0f, m_antiCampTimer - deltaTime * 3.0f);
    } else {
        m_antiCampTimer += deltaTime;
        if (m_antiCampTimer > 6.0f) {
            // El jugador está campeando o dando vueltas en el mismo lugar
            m_confidence = 1.0f; // Furia máxima
            m_stress = 0.0f;
            if (m_antiCampTimer - deltaTime <= 6.0f) {
                std::cout << "[AI] DETECTADO CAMPEO. Entrando en modo Berserk." << std::endl;
            }
        }
    }

    glm::vec3 initialVel = m_velocity;
    glm::vec3 targetVelocity = glm::vec3(0.0f);

    // Incremental animation time
    float speed = glm::length(m_velocity);
    m_animTime += speed * deltaTime; 
    
    // Bleeding Effect
    if (m_health < 2.0f) {
        m_bleedTimer += deltaTime;
        if (m_bleedTimer > 0.3f) { 
            m_bleedTimer = 0.0f;
            glm::vec3 bleedPos = m_pos + glm::vec3((rand()%100/200.0f - 0.25f), 1.5f, (rand()%100/200.0f - 0.25f));
            particles.SpawnParticle(bleedPos, glm::vec3(0, -2.0f, 0), glm::vec4(0.8f, 0.0f, 0.0f, 1.0f), 0.1f, 1.0f, -9.8f);
        }
    }

    // --- AGGRESSION SENSES & FLASH DETECT ---
    // 1. INSTANT KILL (Handled in main.cpp for glitched screen)
    
    // Update dynamic exposure meter
    float instantExposure = MultiRaycastExposure(playerPos, chunkManager);
    float exposureChangeSpeed = 2.5f; // Speed of visual adaptation
    m_exposure = glm::mix(m_exposure, instantExposure, glm::clamp(deltaTime * exposureChangeSpeed, 0.0f, 1.0f));
    
    // Crouch trigger condition
    bool targetCrouch = (m_action == MonsterAction::STALK) || 
                        (m_action == MonsterAction::RETREAT && m_exposure < 0.6f) ||
                        (m_action == MonsterAction::INVESTIGATE && m_exposure < 0.6f && m_estimatedPlayerAmmo > 0);
    
    float crouchTransitionSpeed = 5.0f; // Speed of crouch visual transition
    m_crouchFactor = glm::mix(m_crouchFactor, targetCrouch ? 1.0f : 0.0f, glm::clamp(deltaTime * crouchTransitionSpeed, 0.0f, 1.0f));
    m_isCrouching = (m_crouchFactor > 0.5f);
    
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
    if (isAimedAt && hasLOS && m_estimatedPlayerAmmo > 0 && m_startleCooldownTimer <= 0.0f) {
        if (!m_hasStartled) {
            m_hasStartled = true;
            m_startleCooldownTimer = 10.0f; // 10 seconds of immunity to startling
            m_action = MonsterAction::RETREAT;
            m_stateTimer = 0.0f;
            m_bestTreeIndex = -1;
            m_isPanicked = true;
            m_panicTimer = 3.0f; // Panicked state duration
            std::cout << "[AI] STARTLED! Entering Panic Flee mode for 3s..." << std::endl;
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

    // Blood Rage check: Health is critical
    if (m_health > 0.0f && m_health <= 1.2f) {
        if (!m_isEnraged) {
            m_isEnraged = true;
            m_rageTimer = 0.0f;
            std::cout << "[AI-Emotion] BLOOD RAGE ACTIVATED! Health critical: " << m_health << std::endl;
        }
    } else {
        m_isEnraged = false;
    }

    // Decay panic state
    if (m_panicTimer > 0.0f) {
        m_panicTimer -= deltaTime;
        if (m_panicTimer <= 0.0f) {
            m_isPanicked = false;
        }
    }

    // Spawn Blood Rage red vapor trail
    if (m_isEnraged) {
        m_rageTimer += deltaTime;
        static float rageParticleTimer = 0.0f;
        rageParticleTimer += deltaTime;
        if (rageParticleTimer >= 0.03f) {
            rageParticleTimer = 0.0f;
            glm::vec3 headCenterLocal(0.0f, 1.8f, 0.3f);
            glm::mat4 bodyMat = glm::mat4(1.0f);
            bodyMat = glm::translate(bodyMat, m_pos);
            bodyMat = glm::rotate(bodyMat, glm::radians(m_yaw), glm::vec3(0, 1, 0));
            glm::vec3 ragePos = glm::vec3(bodyMat * glm::vec4(headCenterLocal, 1.0f));
            
            particles.SpawnParticle(ragePos, glm::vec3((rand()%100/50.0f - 1.0f)*0.8f, (rand()%100/100.0f)*1.5f, (rand()%100/50.0f - 1.0f)*0.8f), glm::vec4(1.0f, 0.0f, 0.0f, 0.9f), 0.18f, 0.8f, 0.0f);
        }
    }

    // --- EYE GLOW STATE MODULATION ---
    if (m_action == MonsterAction::CHASE) {
        if (m_isEnraged) {
            m_eyeColor = glm::vec3(1.0f, 0.0f, 0.0f);
            m_eyeBrightness = 2.0f; // Overcharged red
        } else {
            m_eyeColor = glm::vec3(1.0f, 0.0f, 0.0f);
            m_eyeBrightness = 1.2f; // Solid high-intensity red
        }
    } else if (m_action == MonsterAction::STALK) {
        // Pulsing dim red to stay stealthy
        float pulse = 0.2f + 0.15f * sin(m_stateTimer * 3.0f);
        m_eyeColor = glm::vec3(pulse, 0.0f, 0.0f);
        m_eyeBrightness = pulse;
    } else if (m_action == MonsterAction::RETREAT) {
        // Rapidly flashing red/dim-red (panic)
        float flash = (std::fmod(m_stateTimer, 0.25f) < 0.125f) ? 1.0f : 0.1f;
        m_eyeColor = glm::vec3(flash, 0.0f, 0.0f);
        m_eyeBrightness = flash;
    } else {
        // Other states (Wander, Investigate, Scent, Climb)
        m_eyeColor = glm::vec3(0.5f, 0.1f, 0.0f); // Dim amber/red
        m_eyeBrightness = 0.5f;
    }

    // Falling leaves visual indicator when monster is on a tree
    if (m_treeClimbHeight > 0.0f || (m_action == MonsterAction::CLIMB_TREE && m_isClimbing)) {
        m_leafDropTimer -= deltaTime;
        if (m_leafDropTimer <= 0.0f) {
            // Reset timer: random interval between 0.4s and 1.2s
            m_leafDropTimer = 0.4f + (rand() % 100 / 100.0f) * 0.8f;
            
            // Spawn 1 to 4 falling leaf particles from the canopy/climb height
            int count = 1 + rand() % 4;
            for (int i = 0; i < count; ++i) {
                // Pos around the monster's current height, spreading out slightly
                glm::vec3 pPos = m_pos + glm::vec3(
                    (rand() % 100 / 50.0f - 1.0f) * 1.8f,
                    (rand() % 100 / 100.0f) * -0.5f,
                    (rand() % 100 / 50.0f - 1.0f) * 1.8f
                );
                
                // Slow downward velocity with random horizontal drifting
                glm::vec3 pVel(
                    (rand() % 100 / 100.0f - 0.5f) * 0.8f,
                    -1.2f - (rand() % 100 / 100.0f) * 0.8f,
                    (rand() % 100 / 100.0f - 0.5f) * 0.8f
                );
                
                // Foliage green/brown colors
                glm::vec4 pCol = (rand() % 2 == 0) ? glm::vec4(0.12f, 0.38f, 0.08f, 0.8f) : glm::vec4(0.32f, 0.28f, 0.18f, 0.7f);
                
                particles.SpawnParticle(pPos, pVel, pCol, 0.08f, 3.5f, -0.6f); // Low gravity for floating leaves
            }
        }
    }

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
    if (m_flankDecisionTimer > 0.0f) {
        m_flankDecisionTimer -= deltaTime;
    }
    if (m_distractionTimer > 0.0f) {
        m_distractionTimer -= deltaTime;
    }
    if (m_fakeChargeTimer > 0.0f) {
        m_fakeChargeTimer -= deltaTime;
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
            m_lastScentWorldPos = bestScent->pos;
            m_hasLastScent = true;
            // ONLY update memory to the exact player position if extremely close (within direct smelling distance)
            if (distToPlayer < 7.0f) {
                m_memPlayerPos = playerPos;
            } else {
                // Otherwise, target the smell packet itself, not the player's exact coordinate!
                m_memPlayerPos = bestScent->pos;
            }
            m_memTimeSinceSmelled = 0.0f;
            
            // Get local gradient direction
            m_trackingDir = scentSystem.GetLocalScentGradient(m_pos, 30.0f);
        } else {
            m_hasLastScent = false;
            m_trackingDir = glm::vec3(0.0f);
        }
    }

    // --- 4. UTILITY SCORING ---
    {
        bool knowsPlayerPos = hasLOS || (m_memTimeSinceSeen < 6.0f) || (m_memTimeSinceHeard < 4.0f) || (m_memTimeSinceSmelled < 2.0f && distToPlayer < 8.0f);

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
                
                float peekThreshold = 0.6f + m_stress * 0.8f;
                if (m_peekTimer >= peekThreshold && m_peekTimer < 900.0f && !threatPersists) {
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

        // CHASE and STALK scoring restricted to knowing player position
        if (knowsPlayerPos) {
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

            // FAKE CHARGE: Maintain active fake charge or trigger a new one
            if (m_fakeChargeTimer > 0.0f) {
                scoreChase = 160.0f; // Keep the bait active!
            } else if (m_estimatedPlayerAmmo > 0 && isAimedAt && distToPlayer > 12.0f && distToPlayer < 25.0f) {
                if (rand() % 200 == 0) { // Trigger check (approx. 0.5% per frame)
                    m_fakeChargeTimer = 1.2f; // Perform fake charge for 1.2 seconds
                    scoreChase = 160.0f;
                }
            }

            // TRAMPA DE EMBOSCADA: If hiding behind cover tree, player gets close (< 7.5m)
            bool isCornered = (distToPlayer < 7.5f);
            bool ambushOpportunity = (m_action == MonsterAction::RETREAT && m_bestTreeIndex != -1 && isCornered);
            if (ambushOpportunity) {
                if (m_estimatedPlayerAmmo == 0) {
                    scoreChase = 200.0f; // JUMP OUT AMBUSH!
                } else if (m_estimatedPlayerAmmo == 1 && !isAimedAt) {
                    scoreChase = 150.0f; // Slink out if player is looking away
                }
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
        } else {
            scoreChase = 0.0f;
            scoreStalk = 0.0f;
        }

        scoreStalk = std::max(0.0f, scoreStalk);

        // CLIMB_TREE scoring:
        if (m_climbCooldownTimer > 0.0f) {
            scoreClimbTree = 0.0f; // Prevent re-climbing entirely during cooldown
        } else if (glm::length(m_knownPlayerTreePos) > 0.1f) {
            // Cautious check: if player is up in the tree, and monster is NOT enraged, player is NOT reloading, and has ammo
            bool playerVulnerable = m_isEnraged || isPlayerReloading || m_estimatedPlayerAmmo == 0;
            if (playerVulnerable) {
                scoreClimbTree = 250.0f; // High priority: climb the player's tree to attack them!
                scoreChase = 0.0f;
                scoreStalk = 0.0f;
                scoreRetreat = 0.0f;
                scoreWander = 0.0f;
                scoreInvestigate = 0.0f;
                scoreTrackScent = 0.0f;
            } else {
                scoreClimbTree = 0.0f; // Cautious: do not climb to avoid being shot
                
                // Cautious Tree Tactics: Hide behind a cover tree or stalk/orbit the player's tree
                float distToPlayerTree = glm::distance(m_pos, m_knownPlayerTreePos);
                if (distToPlayerTree < 15.0f) {
                    scoreRetreat = 140.0f; // Retreat to a cover tree (hide)
                    scoreStalk = 100.0f;
                } else {
                    scoreStalk = 130.0f;   // Circle/orbit the player's tree
                    scoreRetreat = 90.0f;
                }
                // Suppress aggressive or unfocused actions when cautious of tree threat
                scoreChase = 0.0f;
                scoreWander = 0.0f;
                scoreInvestigate = 0.0f;
                scoreTrackScent = 0.0f;
            }
        } else if (m_estimatedPlayerAmmo == 2) {
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
            scoreClimbTree = 250.0f; // Keep it high so we don't interrupt climbing!
            scoreChase = 0.0f;
            scoreStalk = 0.0f;
            scoreRetreat = 0.0f;
            scoreWander = 0.0f;
            scoreInvestigate = 0.0f;
            scoreTrackScent = 0.0f;
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

        bool playerVulnerable = isPlayerReloading || (m_estimatedPlayerAmmo == 0);
        bool isReloadClose = isPlayerReloading && distToPlayer < 10.0f;
        
        // Evitar que la IA aborte la acción si está subida a un árbol (evita teletransportes al suelo)
        bool isMidClimb = (m_action == MonsterAction::CLIMB_TREE && m_treeClimbHeight > 0.0f);
        
        bool canChangeAction = (!isMidClimb) && ((m_decisionLockTimer <= 0.0f) ||
                               (bestAction == MonsterAction::CHASE) ||
                               (bestAction == MonsterAction::CLIMB_TREE) ||
                               playerVulnerable ||
                               (bestAction == MonsterAction::RETREAT && (isAimedAt || isVisibleToPlayer)));

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
            m_stateTimer += deltaTime;
            
            // 1. Intercepción predictiva (calcula dónde va a estar el jugador)
            float chaseSpeedMult = 2.2f;
            if (m_isEnraged) chaseSpeedMult = 3.5f; // Blood rage speed boost
            else if (m_estimatedPlayerAmmo == 0) chaseSpeedMult = 2.8f;
            else if (isPlayerReloading && distToPlayer < 12.0f) chaseSpeedMult = 2.5f;

            // 2. Efecto Slender: Si no lo estás mirando, corre MUCHO más rápido
            if (!isVisibleToPlayer) {
                chaseSpeedMult *= 1.35f; // Castigo por darle la espalda
            }

            float currentSpeed = m_speed * chaseSpeedMult;
            float lookAheadTime = distToPlayer / currentSpeed;
            lookAheadTime = std::min(lookAheadTime, 2.5f); // Limit prediction window to 2.5s
            glm::vec3 predictedPos = playerPos + (playerVelocity * lookAheadTime * 0.7f); 
            
            glm::vec3 chaseTarget = predictedPos;
            glm::vec4 blockingTree;
            if (GetBlockingTree(predictedPos, chunkManager, blockingTree)) {
                glm::vec3 treePos(blockingTree.x, m_pos.y, blockingTree.z);
                glm::vec3 dirToTree = SafeNormalize(treePos - m_pos);
                glm::vec3 rightDir = SafeCross(dirToTree, glm::vec3(0.0f, 1.0f, 0.0f));
                float radius = 0.5f + (0.6f * blockingTree.w);
                float offsetDist = radius + 1.2f;
                glm::vec3 leftFlank = treePos - rightDir * offsetDist;
                glm::vec3 rightFlank = treePos + rightDir * offsetDist;

                if (m_flankDecisionTimer <= 0.0f) {
                    glm::vec3 pToLeft = glm::normalize(leftFlank - playerPos);
                    glm::vec3 pToRight = glm::normalize(rightFlank - playerPos);
                    float dotLeft = glm::dot(playerFront, pToLeft);
                    float dotRight = glm::dot(playerFront, pToRight);

                    if (dotLeft > dotRight + 0.05f) {
                        m_chooseLeftFlank = false; // Player looking left, go right
                    } else if (dotRight > dotLeft + 0.05f) {
                        m_chooseLeftFlank = true; // Player looking right, go left
                    } else {
                        m_chooseLeftFlank = (rand() % 2 == 0);
                    }
                    m_flankDecisionTimer = 1.0f + (rand() % 100 / 100.0f) * 1.5f;
                }

                glm::vec3 flankTarget = m_chooseLeftFlank ? leftFlank : rightFlank;
                if (glm::distance(m_pos, flankTarget) > 1.0f) {
                    chaseTarget = flankTarget;
                }
            }

            glm::vec3 desiredDir(0.0f);
            float predDist = glm::distance(chaseTarget, m_pos);
            if (predDist > 0.01f) {
                desiredDir = glm::normalize(chaseTarget - m_pos);
            } else {
                glm::vec3 dirToPlayerVec = playerPos - m_pos;
                if (glm::length(dirToPlayerVec) > 0.01f) {
                    desiredDir = glm::normalize(dirToPlayerVec);
                } else {
                    desiredDir = glm::vec3(0.0f, 0.0f, 1.0f);
                }
            }

            // 3. Zigzag defensivo: Si me estás apuntando o en Blood Rage, serpenteo para que falles
            if ((isAimedAt || m_isEnraged) && distToPlayer > 4.0f) {
                glm::vec3 rightDir(1.0f, 0.0f, 0.0f);
                glm::vec3 crossDir = glm::cross(desiredDir, glm::vec3(0.0f, 1.0f, 0.0f));
                if (glm::length(crossDir) > 0.01f) {
                    rightDir = glm::normalize(crossDir);
                }
                // Alta frecuencia de zigzag impulsada por el estrés del monstruo, even higher if enraged
                float zigFreq = m_isEnraged ? 18.0f : glm::clamp(10.0f + m_stress * 5.0f, 6.0f, 14.0f);
                float zigWidth = m_isEnraged ? 0.75f : 0.55f;
                float zigZag = sin(m_stateTimer * zigFreq) * zigWidth;
                desiredDir = glm::normalize(desiredDir + rightDir * zigZag);
            }

            m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
            targetVelocity = desiredDir * currentSpeed; 
            
            // Animación enfocada
            m_headYaw = 0.0f; 
            m_headPitch = (distToPlayer < 3.0f) ? 20.0f : 0.0f; // Baja la cabeza para morder si está cerca
            break;
        }

        case MonsterAction::INVESTIGATE: {
            glm::vec3 target = m_memPlayerPos;
            bool nearTree = false;
            glm::vec3 treePos(0.0f);
            float treeRadius = 0.0f;
            
            // Check if last known player position is near a tree
            std::vector<glm::vec4> nearbyTrees;
            chunkManager.GetTreesInRange(m_memPlayerPos, 3.0f, nearbyTrees);
            if (!nearbyTrees.empty()) {
                nearTree = true;
                treePos = glm::vec3(nearbyTrees[0].x, 0.0f, nearbyTrees[0].z);
                treeRadius = 0.5f + (0.6f * nearbyTrees[0].w);
                
                // Target a point behind the tree relative to the monster's starting approach direction
                glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 toTree = treePos - flatPos;
                if (glm::length(toTree) > 0.1f) {
                    glm::vec3 dirToTree = glm::normalize(toTree);
                    target = treePos + dirToTree * (treeRadius + 1.8f); // 1.8 meters behind tree
                }
            }

            glm::vec3 flatPos = glm::vec3(m_pos.x, 0.0f, m_pos.z);
            glm::vec3 flatTarget = glm::vec3(target.x, 0.0f, target.z);
            
            // Flanking target for tree investigation
            glm::vec3 investigateTarget = target;
            if (nearTree) {
                glm::vec4 blockingTree;
                if (GetBlockingTree(target, chunkManager, blockingTree)) {
                    glm::vec3 treePos(blockingTree.x, m_pos.y, blockingTree.z);
                    glm::vec3 dirToTree = SafeNormalize(treePos - m_pos);
                    glm::vec3 rightDir = SafeCross(dirToTree, glm::vec3(0.0f, 1.0f, 0.0f));
                    float radius = 0.5f + (0.6f * blockingTree.w);
                    float offsetDist = radius + 1.2f;
                    glm::vec3 leftFlank = treePos - rightDir * offsetDist;
                    glm::vec3 rightFlank = treePos + rightDir * offsetDist;

                    if (m_flankDecisionTimer <= 0.0f) {
                        glm::vec3 pToLeft = glm::normalize(leftFlank - playerPos);
                        glm::vec3 pToRight = glm::normalize(rightFlank - playerPos);
                        float dotLeft = glm::dot(playerFront, pToLeft);
                        float dotRight = glm::dot(playerFront, pToRight);

                        if (dotLeft > dotRight + 0.05f) {
                            m_chooseLeftFlank = false; // Player looking left, go right
                        } else if (dotRight > dotLeft + 0.05f) {
                            m_chooseLeftFlank = true; // Player looking right, go left
                        } else {
                            m_chooseLeftFlank = (rand() % 2 == 0);
                        }
                        m_flankDecisionTimer = 1.0f + (rand() % 100 / 100.0f) * 1.5f;
                    }

                    glm::vec3 flankTarget = m_chooseLeftFlank ? leftFlank : rightFlank;
                    if (glm::distance(m_pos, flankTarget) > 1.0f) {
                        investigateTarget = flankTarget;
                        flatTarget = glm::vec3(investigateTarget.x, 0.0f, investigateTarget.z);
                    }
                }
            }

            if (glm::distance(flatPos, glm::vec3(target.x, 0.0f, target.z)) < 1.5f) {
                targetVelocity = glm::vec3(0.0f);
                m_memTimeSinceHeard += 10.0f * deltaTime; // decay state
                m_headYaw = sin(m_stateTimer * 4.0f) * 40.0f;
                m_headPitch = 0.0f;
            } else {
                glm::vec3 desiredDir = glm::normalize(flatTarget - flatPos);
                float speedMult = 0.7f;
                
                m_headYaw = sin(m_stateTimer * 4.0f) * 40.0f;
                m_headPitch = 0.0f;
                
                if (nearTree && m_estimatedPlayerAmmo > 0) {
                    float distToTree = glm::distance(flatPos, treePos);
                    if (distToTree < 7.0f && distToTree > (treeRadius + 0.5f)) {
                        // Slow down to a cautious creep
                        speedMult = 0.25f;
                        
                        // Feint/Bait logic based on periodic timer
                        float cycleTime = std::fmod(m_stateTimer, 2.5f);
                        if (cycleTime < 0.8f) {
                            // Phase 1: Pause and scan (head moves side to side, no velocity)
                            speedMult = 0.0f;
                            m_headYaw = sin(m_stateTimer * 6.0f) * 55.0f;
                        } else if (cycleTime < 1.4f) {
                            // Phase 2: Quick side-step (feint left/right)
                            glm::vec3 rightDir = SafeCross(desiredDir, glm::vec3(0,1,0));
                            
                            float sideSign = (std::fmod(m_stateTimer, 5.0f) < 2.5f) ? 1.0f : -1.0f;
                            desiredDir = glm::normalize(desiredDir * 0.1f + rightDir * 0.9f * sideSign);
                            speedMult = 1.2f; // Quick dash step
                            m_headYaw = -25.0f * sideSign; // Tilt head in direction of step
                        } else {
                            // Phase 3: Creep forward cautiously
                            speedMult = 0.35f;
                            m_headYaw = sin(m_stateTimer * 3.0f) * 20.0f;
                        }
                    }
                }
                
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                targetVelocity = desiredDir * m_speed * speedMult;
            }
            break;
        }

        case MonsterAction::TRACK_SCENT: {
            m_stateTimer += deltaTime;

            bool scentPresent = (glm::length(m_trackingDir) > 0.01f);
            
            // Check if gradient is zero but cached scent is still valid (fallback targeting)
            if (!scentPresent && m_hasLastScent) {
                glm::vec3 fallbackDir = glm::normalize(m_lastScentWorldPos - m_pos);
                if (glm::length(fallbackDir) > 0.1f) {
                    m_trackingDir = fallbackDir;
                    scentPresent = true; // Recover normal tracking
                }
            }

            if (scentPresent) {
                // Scent is back! Reset spiral search state
                m_spiralSearchTimer = 0.0f;
                m_spiralSearchRadius = 3.0f;
                m_spiralSearchAngle = 0.0f;

                // Sinusoidal wobble (drift) to make search patterns winding and organic
                float wobble = sin(m_stateTimer * 3.5f) * 0.35f;
                glm::vec3 rightDir = SafeCross(m_trackingDir, glm::vec3(0.0f, 1.0f, 0.0f));
                
                glm::vec3 desiredDir = glm::normalize(m_trackingDir + rightDir * wobble);
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                targetVelocity = desiredDir * Config::Monster::ScentTrackSpeed;
                
                // Sniffing animations
                m_headPitch = 25.0f + sin(m_stateTimer * 8.0f) * 10.0f;
                m_headYaw = sin(m_stateTimer * 4.0f) * 20.0f;
            } else {
                // Lost scent trail: Expanding spiral search around last smelled position (m_memPlayerPos)
                m_spiralSearchTimer += deltaTime;
                m_spiralSearchRadius = 3.0f + m_spiralSearchTimer * 0.75f;
                if (m_spiralSearchRadius > 18.0f) {
                    m_spiralSearchRadius = 18.0f;
                }
                
                // Smooth spiral angular step to build curved pathing waypoints
                float angleStep = 0.8f; // Rad per second
                m_spiralSearchAngle += angleStep * deltaTime;
                
                glm::vec3 center = m_memPlayerPos;
                glm::vec3 targetOffset(cos(m_spiralSearchAngle) * m_spiralSearchRadius, 0.0f, sin(m_spiralSearchAngle) * m_spiralSearchRadius);
                glm::vec3 targetSpiralPos = center + targetOffset;
                
                glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 flatTarget(targetSpiralPos.x, 0.0f, targetSpiralPos.z);
                
                glm::vec3 desiredDir(0.0f);
                float distToTarget = glm::distance(flatPos, flatTarget);
                if (distToTarget > 1.5f) {
                    desiredDir = glm::normalize(flatTarget - flatPos);
                } else {
                    // Maintain current heading gently to avoid spinning in place
                    desiredDir = (glm::length(m_velocity) > 0.01f) ? glm::normalize(m_velocity) : glm::vec3(sin(glm::radians(m_yaw)), 0.0f, cos(glm::radians(m_yaw)));
                }
                
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                targetVelocity = desiredDir * Config::Monster::ScentTrackSpeed * 1.1f;
                
                // Sniffing animations while searching
                m_headPitch = 30.0f + sin(m_stateTimer * 6.0f) * 12.0f;
                m_headYaw = sin(m_stateTimer * 3.0f) * 45.0f;
                
                // If we've been searching for more than 15 seconds, give up
                if (m_spiralSearchTimer > 15.0f) {
                    m_memTimeSinceSmelled = 999.0f;
                    m_spiralSearchTimer = 0.0f;
                    m_spiralSearchRadius = 3.0f;
                    m_spiralSearchAngle = 0.0f;
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
            float prevTime = m_stateTimer;
            m_stateTimer += deltaTime;
            
            // Check for twig-snap trigger
            if (prevTime < 3.0f && m_stateTimer >= 3.0f) {
                m_stalkDashTimer = 0.6f;
                // Dash perpendicular to the vector pointing towards the player
                glm::vec3 toPlayerVec = playerPos - m_pos;
                toPlayerVec.y = 0.0f;
                if (glm::length(toPlayerVec) > 0.1f) {
                    glm::vec3 toPlayerDir = glm::normalize(toPlayerVec);
                    glm::vec3 rightDir = SafeCross(toPlayerDir, glm::vec3(0.0f, 1.0f, 0.0f));
                    float side = (rand() % 2 == 0) ? 1.0f : -1.0f;
                    m_stalkDashDir = rightDir * side;
                } else {
                    m_stalkDashDir = glm::vec3(1.0f, 0.0f, 0.0f);
                }
                std::cout << "[AI] Twig-Snap! Spawning fake audio event to distract player." << std::endl;
                
                // Spawn a burst of brown particles simulating snapped branches/leaves
                for (int i = 0; i < 12; i++) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/100.0f)*1.0f, (rand()%100/50.0f - 1.0f)*1.5f);
                    particles.SpawnParticle(m_pos + glm::vec3(0.0f, 0.1f, 0.0f), pVel, glm::vec4(0.4f, 0.3f, 0.2f, 0.8f), 0.12f, 0.8f, -9.8f);
                }
            }



            // Distant Twig-Snap Distraction Trap
            if (m_distractionTimer <= 0.0f && isVisibleToPlayer) {
                std::vector<glm::vec4> nearbyTrees;
                chunkManager.GetTreesInRange(playerPos, 22.0f, nearbyTrees);
                
                glm::vec4 chosenDistractionTree(0.0f);
                bool foundDistractionTree = false;
                
                for (const auto& t : nearbyTrees) {
                    float dToPlayer = glm::distance(glm::vec3(t.x, 0.0f, t.z), glm::vec3(playerPos.x, 0.0f, playerPos.z));
                    if (dToPlayer >= 12.0f && dToPlayer <= 22.0f) {
                        if (m_bestTreeIndex != -1 && glm::distance(glm::vec3(t.x, 0.0f, t.z), m_assignedTreePos) < 2.0f) {
                            continue;
                        }
                        if (glm::distance(glm::vec3(t.x, 0.0f, t.z), m_pos) < 6.0f) {
                            continue;
                        }
                        chosenDistractionTree = t;
                        foundDistractionTree = true;
                        break;
                    }
                }
                
                if (foundDistractionTree) {
                    glm::vec3 treePos(chosenDistractionTree.x, WorldGenerator::GetHeight(chosenDistractionTree.x, chosenDistractionTree.z), chosenDistractionTree.z);
                    std::cout << "[AI] Distraction Twig-Snap! Spawning fake audio event at tree ("
                              << treePos.x << ", " << treePos.z << ") to distract player." << std::endl;
                    
                    // Spawn leaf/wood particles at that tree
                    for (int i = 0; i < 15; i++) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.2f, (rand()%100/100.0f)*1.5f + 0.5f, (rand()%100/50.0f - 1.0f)*1.2f);
                        particles.SpawnParticle(treePos + glm::vec3(0.0f, 0.2f, 0.0f), pVel, glm::vec4(0.35f, 0.25f, 0.15f, 0.9f), 0.15f, 1.2f, -9.8f);
                    }
                    // Reset distraction cooldown to 12-18 seconds
                    m_distractionTimer = 12.0f + (rand() % 100 / 100.0f) * 6.0f;
                }
            }

            if (m_stalkDashTimer > 0.0f) {
                m_stalkDashTimer -= deltaTime;
                if (m_stalkDashTimer <= 0.0f) {
                    m_stalkDashTimer = 0.0f;
                    m_stateTimer = 0.0f; // Reset twig-snap cycle immediately when dash ends
                }
                targetVelocity = m_stalkDashDir * m_speed * 2.5f; // Fast dash
                
                // Face the player while dashing
                glm::vec3 desiredDir = glm::normalize(playerPos - m_pos);
                m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
            } else {
                // Wide circular flanking around the player's FOV
                glm::vec3 toMonster = m_pos - playerPos;
                toMonster.y = 0.0f;
                float distToPlayer = glm::length(toMonster);
                if (distToPlayer < 0.1f) toMonster = glm::vec3(1.0f, 0.0f, 0.0f);
                
                glm::vec3 target(0.0f);
                bool isBehindPlayer = !isVisibleToPlayer && hasLOS && (glm::length(m_knownPlayerTreePos) <= 0.1f);
                
                if (isBehindPlayer) {
                    // Player's back is turned! Creep up directly towards the player's position to ambush them.
                    target = playerPos;
                } else {
                    // Player is looking in our direction, or we don't have LOS. Maintain wide flanking.
                    if (distToPlayer > 28.0f) {
                        // Too far: close in to flanking range laterally
                        glm::vec3 flankDir = SafeCross(playerFront, glm::vec3(0, 1, 0));
                        float side = (glm::dot(flankDir, toMonster) > 0.0f) ? 1.0f : -1.0f;
                        target = playerPos + flankDir * (24.0f * side) - playerFront * 10.0f;
                    } else if (distToPlayer < 15.0f) {
                        // Too close: back away laterally to maintain flanking distance
                        glm::vec3 flankDir = SafeCross(playerFront, glm::vec3(0, 1, 0));
                        float side = (glm::dot(flankDir, toMonster) > 0.0f) ? 1.0f : -1.0f;
                        target = playerPos + flankDir * (26.0f * side) - playerFront * 15.0f;
                    } else {
                        // In flanking sweet spot: orbit towards player's rear quadrant
                        glm::vec3 pRight = SafeCross(playerFront, glm::vec3(0, 1, 0));
                        float side = (glm::dot(pRight, toMonster) > 0.0f) ? 1.0f : -1.0f;
                        target = playerPos + pRight * (22.0f * side) - playerFront * 18.0f;
                    }
                }

                glm::vec3 stalkTarget = target;
                glm::vec4 blockingTree;
                if (GetBlockingTree(target, chunkManager, blockingTree)) {
                    glm::vec3 treePos(blockingTree.x, m_pos.y, blockingTree.z);
                    glm::vec3 dirToTree = SafeNormalize(treePos - m_pos);
                    glm::vec3 rightDir = SafeCross(dirToTree, glm::vec3(0.0f, 1.0f, 0.0f));
                    float radius = 0.5f + (0.6f * blockingTree.w);
                    float offsetDist = radius + 1.2f;
                    glm::vec3 leftFlank = treePos - rightDir * offsetDist;
                    glm::vec3 rightFlank = treePos + rightDir * offsetDist;

                    if (m_flankDecisionTimer <= 0.0f) {
                        glm::vec3 pToLeft = glm::normalize(leftFlank - playerPos);
                        glm::vec3 pToRight = glm::normalize(rightFlank - playerPos);
                        float dotLeft = glm::dot(playerFront, pToLeft);
                        float dotRight = glm::dot(playerFront, pToRight);

                        if (dotLeft > dotRight + 0.05f) {
                            m_chooseLeftFlank = false; // Player looking left, go right
                        } else if (dotRight > dotLeft + 0.05f) {
                            m_chooseLeftFlank = true; // Player looking right, go left
                        } else {
                            m_chooseLeftFlank = (rand() % 2 == 0);
                        }
                        m_flankDecisionTimer = 1.0f + (rand() % 100 / 100.0f) * 1.5f;
                    }

                    glm::vec3 flankTarget = m_chooseLeftFlank ? leftFlank : rightFlank;
                    if (glm::distance(m_pos, flankTarget) > 1.0f) {
                        stalkTarget = flankTarget;
                    }
                }

                glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 flatTarget(stalkTarget.x, 0.0f, stalkTarget.z);
                
                if (glm::distance(flatPos, flatTarget) < 1.5f) {
                    targetVelocity = glm::vec3(0.0f);
                } else {
                    glm::vec3 desiredDir = glm::normalize(flatTarget - flatPos);
                    m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                    float sneakSpeedMult = isBehindPlayer ? 0.98f : (0.75f + m_confidence * 0.25f);
                    targetVelocity = desiredDir * m_speed * sneakSpeedMult; // Quiet sneak speed
                }
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
                                glm::vec3 surroundDir = SafeCross(toPlayer, glm::vec3(0, 1, 0));
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
                        glm::vec3 rightDir = SafeCross(desiredDir, glm::vec3(0.0f, 1.0f, 0.0f));
                        
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
                    if (m_isPanicked) retreatSpeedMult = 3.6f; // Panic speed boost
                    else if (m_estimatedPlayerAmmo == 2) retreatSpeedMult = 2.8f;
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
                            targetVelocity = glm::normalize(orbitDelta) * m_speed * 0.9f; // Orbit slower
                        }
                    } else {
                        m_peekTimer += deltaTime;
                        float peekThreshold = 0.6f + m_stress * 0.8f;
                        if (m_peekTimer < peekThreshold) {
                            // Stop and turn the body to look directly at the player
                            targetVelocity = glm::vec3(0.0f);
                            LookAt(playerPos);
                            m_headYaw = 0.0f;
                            m_headPitch = 0.0f;
                        } else {
                            // After peekThreshold seconds of looking back, evaluate threat
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
                                    targetVelocity = glm::normalize(orbitDelta) * m_speed * 0.9f; // Orbit slower
                                }
                            }
                        }
                    }
                }
            } else {
                // No tree, flee circularly in a curving arc to break the straight line!
                glm::vec3 awayDir = SafeNormalize(m_pos - playerPos);
                glm::vec3 rightDir = SafeCross(awayDir, glm::vec3(0, 1, 0));
                
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
                            glm::vec3 surroundDir = SafeCross(toPlayer, glm::vec3(0, 1, 0));
                            if (glm::dot(surroundDir, m_velocity) < 0.0f) {
                                surroundDir = -surroundDir;
                            }
                            runDir = surroundDir;
                        }
                    }
                }
                
                m_targetYaw = glm::degrees(atan2(runDir.x, runDir.z));
                float retreatSpeedMult = 1.8f;
                if (m_isPanicked) retreatSpeedMult = 3.2f; // Panic speed boost
                else if (m_estimatedPlayerAmmo == 2) retreatSpeedMult = 2.4f;
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
            
            // Force re-targeting if we know player's tree but are targeting something else
            if (glm::length(m_knownPlayerTreePos) > 0.1f && 
                glm::distance(glm::vec2(m_assignedTreePos.x, m_assignedTreePos.z), glm::vec2(m_knownPlayerTreePos.x, m_knownPlayerTreePos.z)) > 0.1f) {
                m_bestTreeIndex = -1;
                m_isClimbing = false;
                m_treeClimbHeight = 0.0f;
            }
            
            if (m_bestTreeIndex == -1) {
                if (glm::length(m_knownPlayerTreePos) > 0.1f) {
                    m_assignedTreePos = m_knownPlayerTreePos;
                    m_assignedTreeScale = 1.0f;
                    m_bestTreeIndex = 0; // Mock index
                    
                    // Search detected trees for scale
                    m_detectedTrees.clear();
                    chunkManager.GetTreesInRange(m_pos, 60.0f, m_detectedTrees);
                    for (int i = 0; i < m_detectedTrees.size(); i++) {
                        if (glm::distance(glm::vec3(m_detectedTrees[i].x, 0.0f, m_detectedTrees[i].z), glm::vec3(m_knownPlayerTreePos.x, 0.0f, m_knownPlayerTreePos.z)) < 0.5f) {
                            m_bestTreeIndex = i;
                            m_assignedTreeScale = m_detectedTrees[i].w;
                            break;
                        }
                    }
                    m_isClimbing = false;
                    m_treeClimbHeight = 0.0f;
                    m_stateTimer = 0.0f;
                    std::cout << "[AI] TARGETING PLAYER TREE! Walking to tree base..." << std::endl;
                } else {
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
                        m_bestTreeIndex = -1;
                        m_action = MonsterAction::WANDER;
                        m_stateTimer = 0.0f;
                    }
                }
            } else {
                glm::vec3 flatPos(m_pos.x, 0.0f, m_pos.z);
                glm::vec3 flatTree(m_assignedTreePos.x, 0.0f, m_assignedTreePos.z);
                
                float distToTree = glm::distance(flatPos, flatTree);
                float targetClimbHeight = 9.5f * m_assignedTreeScale;
                
                if (!m_isClimbing && m_treeClimbHeight <= 0.0f) {
                    // Phase 1: Walk to tree base
                    float minD = 0.5f + (0.6f * m_assignedTreeScale);
                    if (distToTree > minD + 0.2f) {
                        glm::vec3 desiredDir = glm::normalize(flatTree - flatPos);
                        m_targetYaw = glm::degrees(atan2(desiredDir.x, desiredDir.z));
                        float speedMult = m_isEnraged ? 2.3f : 1.0f; // Sprint if enraged/shot
                        targetVelocity = desiredDir * m_speed * speedMult;
                        m_headPitch = 0.0f;
                    } else {
                        // Directly start climbing (without snapping to trunk position)
                        targetVelocity = glm::vec3(0.0f);
                        m_isClimbing = true;
                        m_treeClimbHeight = 0.05f;
                        std::cout << "[AI] Reached tree base. Starting vertical climb..." << std::endl;
                    }
                } else if (m_isClimbing) {
                    // Phase 2: Climb up
                    float climbSpeed = m_isEnraged ? 15.0f : 7.0f; // Climb extremely fast if enraged
                    m_treeClimbHeight += climbSpeed * deltaTime;
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
                    // 1. If estimated player ammo is 0, or player is very close (dist2D < 3.5f), drop down!
                    // Do not drop down if the player is climbing/on the tree!
                    float dist2D = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));
                    bool playerUnderTree = (dist2D < 3.5f) && !isPlayerClimbing;
                    bool shouldDropAttack = (m_estimatedPlayerAmmo == 0 && !isPlayerClimbing) || playerUnderTree;
                    
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

    // If cautious, player is in a tree, and we are too close to the player's tree trunk, run away to a safe distance
    if (m_action != MonsterAction::CLIMB_TREE && glm::length(m_knownPlayerTreePos) > 0.1f) {
        float distToPlayerTree = glm::distance(glm::vec3(m_pos.x, 0.0f, m_pos.z), glm::vec3(m_knownPlayerTreePos.x, 0.0f, m_knownPlayerTreePos.z));
        if (distToPlayerTree < 6.0f) {
            glm::vec3 awayDir = m_pos - m_knownPlayerTreePos;
            awayDir.y = 0.0f;
            if (glm::length(awayDir) > 0.01f) {
                awayDir = glm::normalize(awayDir);
            } else {
                awayDir = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            m_targetYaw = glm::degrees(atan2(awayDir.x, awayDir.z));
            targetVelocity = awayDir * m_speed * 2.2f; // Run away fast to back off from trunk
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
    // Ensure Visual Yaw follows Physics Yaw
    m_visualYaw = m_yaw;

    if (glm::length(m_velocity) > 0.01f) {
        // Smooth Rotation (Physics) - Dynamic rotation speed (CHASE is much faster)
        float rotSpeed = deltaTime * (m_action == MonsterAction::CHASE ? 15.0f : 5.0f);
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
                     // Check if this is our target tree in CLIMB_TREE mode
                     if (m_action == MonsterAction::CLIMB_TREE && !m_isClimbing && m_treeClimbHeight <= 0.0f) {
                         float distToTargetTree = glm::distance(glm::vec2(m_assignedTreePos.x, m_assignedTreePos.z), glm::vec2(t.x, t.z));
                         if (distToTargetTree < 0.8f) {
                             // Reached target tree base! Start climbing immediately.
                             m_isClimbing = true;
                             m_treeClimbHeight = 0.05f;
                             m_velocity = glm::vec3(0.0f);
                             nextPos = m_pos; // Don't move horizontally
                             isStuckOnTree = false;
                             m_stuckTimer = 0.0f;
                             std::cout << "[AI] Collision with target tree base. Starting vertical climb..." << std::endl;
                             break;
                         }
                     }

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
                    glm::vec3 lateralDir = SafeCross(toTree, glm::vec3(0, 1, 0));
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
                        glm::vec2 push = SafeNormalize(glm::vec2(nextPos.x - stuckTreePos.x, nextPos.z - stuckTreePos.z)) * (stuckMinD - d);
                        nextPos.x += push.x;
                        nextPos.z += push.y;
                    }
                }
            } else {
                // Not stuck on a tree, but blocked by terrain. Add a random lateral kick
                glm::vec3 runDir = SafeNormalize(m_velocity);
                glm::vec3 rightDir = SafeCross(runDir, glm::vec3(0, 1, 0));
                m_velocity = rightDir * m_speed * ((rand() % 2 == 0) ? 1.0f : -1.0f);
                nextPos = m_pos + m_velocity * deltaTime;
            }
            m_stuckTimer = 0.0f; // Reset after kicking
        }
        
        float limit = (Config::World::MapRadius - 1) * Config::World::ChunkSize * Config::World::ChunkScale;
        if (!std::isnan(nextPos.x) && !std::isnan(nextPos.y) && !std::isnan(nextPos.z)) {
            if (nextPos.x > limit) nextPos.x = limit;
            if (nextPos.x < -limit) nextPos.x = -limit;
            if (nextPos.z > limit) nextPos.z = limit;
            if (nextPos.z < -limit) nextPos.z = -limit;
            m_pos = nextPos;
        }
    }
    
    // Terrain Snap with Climb Height
    m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z) + m_treeClimbHeight;
    m_visualPos = m_pos;

    // Head Tracking: Look at player if we have LOS and are not climbing/startled
    if (hasLOS && m_action != MonsterAction::CLIMB_TREE && m_startleCooldownTimer <= 9.0f) {
        glm::vec3 toPlayer = glm::normalize(playerPos + glm::vec3(0.0f, 1.2f, 0.0f) - (m_pos + glm::vec3(0.0f, 1.8f, 0.0f)));
        float lookYaw = glm::degrees(atan2(toPlayer.x, toPlayer.z)) - m_yaw;
        lookYaw = std::fmod(lookYaw + 180.0f, 360.0f) - 180.0f;
        m_headYaw = glm::clamp(lookYaw, -70.0f, 70.0f);
        m_headPitch = glm::clamp((float)(-glm::degrees(asin(toPlayer.y))), -45.0f, 45.0f);
    }

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
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size() / 11));
    
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
        
        // --- CLIMBING INTERRUPT ---
        // If monster is currently in the tree, force it to fall/drop to the ground immediately
        bool wasOnTree = (m_treeClimbHeight > 0.0f || m_action == MonsterAction::CLIMB_TREE);
        if (wasOnTree) {
            m_treeClimbHeight = 0.0f;
            m_isClimbing = false;
            m_bestTreeIndex = -1;
            m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
            m_visualPos = m_pos;
            m_climbCooldownTimer = 10.0f; // Prevent re-climbing immediately
        }

        // --- CLIMBING ATTACK vs FLEE DECISION ---
        if (glm::length(m_knownPlayerTreePos) > 0.1f || wasOnTree) {
            // Decrement the estimated ammo immediately as they just fired the shot that hit us!
            m_estimatedPlayerAmmo = std::max(0, m_estimatedPlayerAmmo - 1);

            bool shouldFlee = false;
            if (m_estimatedPlayerAmmo == 0) {
                // Player has no ammo left, attack violently!
                shouldFlee = false;
            } else {
                // Roll probability based on health
                float fleeChance = 0.5f; // 50% base
                if (m_health >= 3.0f) fleeChance = 0.3f; // 30% if high health
                else if (m_health < 2.0f) fleeChance = 0.8f; // 80% if low health
                
                if ((rand() % 100) < (fleeChance * 100.0f)) {
                    shouldFlee = true;
                }
            }
            
            if (shouldFlee) {
                std::cout << "[AI-Damage] Shot in tree! Decided to FLEE. Retreating..." << std::endl;
                m_climbCooldownTimer = 20.0f; // 20 seconds cooldown on tree climbing to prevent immediate re-climbing
                m_knownPlayerTreePos = glm::vec3(0.0f);
                m_memTimeSinceSeen = 999.0f;
                m_memTimeSinceHeard = 999.0f;
                m_memTimeSinceSmelled = 999.0f;
                
                m_action = MonsterAction::RETREAT;
                m_stateTimer = 0.0f;
                m_stress = 1.0f;
                m_confidence = 0.0f;
            } else {
                std::cout << "[AI-Damage] Shot in tree! Decided to ATTACK VIOLENTLY. Enraging..." << std::endl;
                m_isEnraged = true;
                m_rageTimer = 0.0f;
                m_confidence = 1.0f;
                m_stress = 0.0f;
                m_shouldScream = true; // Trigger visual scream burst in Update()
                
                m_action = MonsterAction::CHASE; // Charge from the ground
                m_stateTimer = 0.0f;
            }
        }
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
    glm::vec3 headMin = m_headMin;
    glm::vec3 headMax = m_headMax;
    glm::vec3 bodyMin = m_bodyMin;
    glm::vec3 bodyMax = m_bodyMax;

    // Crouch Hitbox Adjustments (Visual matching)
    if (m_crouchFactor > 0.01f) {
        headMin.y -= 0.7f * m_crouchFactor;
        headMax.y -= 0.7f * m_crouchFactor;
        bodyMax.y -= 0.6f * m_crouchFactor;
    }

    // Lean Hitbox Compensation: during chase, body tilts forward (bodyLean = 0.4)
    // Offset bounding box boundaries forward on the Z axis (local forward) and lower head to stay aligned.
    if (m_action == MonsterAction::CHASE) {
        headMin.z += 0.3f;
        headMax.z += 0.5f;
        headMin.y -= 0.25f; // Lower head hitbox to match visually leaned run
        headMax.y -= 0.25f;
        bodyMin.z += 0.1f;
        bodyMax.z += 0.4f;
    }

    // HEAD (Local)
    float tHead = 10000.0f;
    bool hitHead = RayAABBLocal(localOrigin, localDir, headMin, headMax, tHead);

    // BODY (Local)
    float tBody = 10000.0f;
    bool hitBody = RayAABBLocal(localOrigin, localDir, bodyMin, bodyMax, tBody);

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
        
        // A. Trunk Check
        if (distToCenterSq < radius * radius) {
            float dClamp = glm::clamp(d, 0.0f, 1.0f);
            float rayY = startPos.y + (endPos.y - startPos.y) * dClamp;
            if (rayY <= t.y + 6.0f * t.w) {
                return false; // Blocked by trunk
            }
        }
        
        // B. Leaves Check
        float leavesRadius = 3.0f * t.w;
        if (distToCenterSq < leavesRadius * leavesRadius) {
            float dClamp = glm::clamp(d, 0.0f, 1.0f);
            float rayY = startPos.y + (endPos.y - startPos.y) * dClamp;
            if (rayY >= t.y + 5.0f * t.w && rayY <= t.y + 25.0f * t.w) {
                // El monstruo solo puede enfocar la vista a través de las hojas si escuchó algo MUY CERCA (< 15m)
                bool hasAttentiveHearing = (m_memTimeSinceHeard < 4.0f && dist < 15.0f);
                if (!hasAttentiveHearing) {
                    return false; // Blocked by leaves!
                }
            }
        }
    }
    
    return true;
}

bool Monster::GetBlockingTree(glm::vec3 targetPos, ChunkManager& chunkManager, glm::vec4& outTree) {
    glm::vec3 startPos = m_pos + glm::vec3(0, 1.0f, 0); // Ray from lower chest height
    glm::vec3 endPos = targetPos + glm::vec3(0, 1.0f, 0);
    glm::vec3 dir = endPos - startPos;
    float dist = glm::length(dir);
    if (dist < 0.1f) return false;
    dir = glm::normalize(dir);

    // 1. Terrain Check
    float stepSize = 1.0f;
    for (float d = stepSize; d < dist; d += stepSize) {
        glm::vec3 p = startPos + dir * d;
        float h = WorldGenerator::GetHeight(p.x, p.z);
        if (p.y < h) return false; // Blocked by terrain
    }

    // 2. Tree Check
    std::vector<glm::vec4> trees;
    chunkManager.GetTreesInRange(startPos, dist + 2.0f, trees);
    
    glm::vec2 A(startPos.x, startPos.z);
    glm::vec2 B(endPos.x, endPos.z);
    
    float closestDist = 9999.0f;
    bool found = false;

    for (const auto& t : trees) {
        glm::vec2 center(t.x, t.z);
        if (glm::distance(A, center) < 0.1f) continue;
        float radius = 0.5f + (0.6f * t.w); // Tree radius logic

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
            float distToTree = glm::distance(A, center);
            if (distToTree < closestDist) {
                closestDist = distToTree;
                outTree = t;
                found = true;
            }
        }
    }
    return found;
}

float Monster::MultiRaycastExposure(glm::vec3 playerPos, ChunkManager& chunkManager) {
    std::vector<glm::vec3> samplePoints = {
        m_pos + glm::vec3(0.0f, 2.0f, 0.0f),
        m_pos + glm::vec3(0.0f, 1.2f, 0.0f),
        m_pos + glm::vec3(0.0f, 0.2f, 0.0f)
    };
    
    int hits = 0;
    glm::vec3 startPos = playerPos + glm::vec3(0.0f, 1.5f, 0.0f); // Player eye line
    
    // Find maximum distance to fetch trees once
    float maxDist = 0.0f;
    for (const auto& targetPoint : samplePoints) {
        maxDist = std::max(maxDist, glm::distance(startPos, targetPoint));
    }
    
    std::vector<glm::vec4> trees;
    chunkManager.GetTreesInRange(startPos, maxDist + 2.0f, trees);
    
    for (const auto& targetPoint : samplePoints) {
        glm::vec3 dir = targetPoint - startPos;
        float dist = glm::length(dir);
        if (dist < 0.1f) {
            hits++;
            continue;
        }
        dir = glm::normalize(dir);
        
        bool blocked = false;
        
        // A. Check terrain collision
        float stepSize = 1.0f;
        for (float d = stepSize; d < dist; d += stepSize) {
            glm::vec3 p = startPos + dir * d;
            float h = WorldGenerator::GetHeight(p.x, p.z);
            if (p.y < h) {
                blocked = true;
                break;
            }
        }
        
        if (blocked) continue;
        
        // B. Check tree collision using pre-fetched trees list
        glm::vec2 A(startPos.x, startPos.z);
        glm::vec2 B(targetPoint.x, targetPoint.z);
        
        for (const auto& t : trees) {
            glm::vec2 center(t.x, t.z);
            if (glm::distance(A, center) < 0.1f) continue;
            float radius = 0.5f + (0.6f * t.w);
            
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
            
            // Trunk Check
            if (distToCenterSq < radius * radius) {
                float dClamp = glm::clamp(d, 0.0f, 1.0f);
                float rayY = startPos.y + (targetPoint.y - startPos.y) * dClamp;
                if (rayY <= t.y + 6.0f * t.w) {
                    blocked = true;
                    break;
                }
            }
            
            // Leaves Check
            float leavesRadius = 3.0f * t.w;
            if (distToCenterSq < leavesRadius * leavesRadius) {
                float dClamp = glm::clamp(d, 0.0f, 1.0f);
                float rayY = startPos.y + (targetPoint.y - startPos.y) * dClamp;
                if (rayY >= t.y + 5.0f * t.w && rayY <= t.y + 25.0f * t.w) {
                    bool hasAttentiveHearing = (m_memTimeSinceHeard < 4.0f);
                    if (!hasAttentiveHearing) {
                        blocked = true;
                        break;
                    }
                }
            }
        }
        
        if (!blocked) {
            hits++;
        }
    }
    
    return (float)hits / 3.0f;
}

glm::vec3 Monster::ApplyObstacleAvoidance(glm::vec3 desiredVel, ChunkManager& chunkManager) {
    if (glm::length(desiredVel) < 0.01f) return desiredVel;
    
    // Ensure cache is updated if it was empty (e.g. action transitioned from CLIMB_TREE)
    if (m_nearbyTreesCache.empty() && m_action != MonsterAction::CLIMB_TREE) {
        chunkManager.GetTreesInRange(m_pos, 5.0f, m_nearbyTreesCache);
    }
    
    glm::vec3 forward = glm::normalize(desiredVel);
    glm::vec3 avoidForce(0.0f);

    // --- VALLEY/GULLY STEERING BIAS ---
    if (m_action == MonsterAction::STALK || m_action == MonsterAction::RETREAT) {
        glm::vec3 right = SafeCross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
        
        float sampleDist = 3.5f;
        glm::vec3 centerPt = m_pos + forward * sampleDist;
        glm::vec3 leftPt = m_pos + (forward * 0.866f - right * 0.5f) * sampleDist; // 30 deg left
        glm::vec3 rightPt = m_pos + (forward * 0.866f + right * 0.5f) * sampleDist; // 30 deg right
        
        float hCenter = WorldGenerator::GetHeight(centerPt.x, centerPt.z);
        float hLeft = WorldGenerator::GetHeight(leftPt.x, leftPt.z);
        float hRight = WorldGenerator::GetHeight(rightPt.x, rightPt.z);
        
        glm::vec3 terrainForce(0.0f);
        if (hLeft < hCenter && hLeft < hRight) {
            float slopeDiff = hCenter - hLeft;
            terrainForce = -right * glm::clamp(slopeDiff * 1.5f, 0.0f, 2.5f);
        } else if (hRight < hCenter && hRight < hLeft) {
            float slopeDiff = hCenter - hRight;
            terrainForce = right * glm::clamp(slopeDiff * 1.5f, 0.0f, 2.5f);
        }
        
        avoidForce += terrainForce;
    }
    
    float maxAvoidDistance = 4.0f;
    
    // Re-use m_nearbyTreesCache for obstacle avoidance to avoid redundant queries
    for (const auto& t : m_nearbyTreesCache) {
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
                    steerDir = SafeCross(forward, glm::vec3(0, 1, 0));
                }
                
                // Avoidance strength is stronger the closer we are to the tree
                float strength = (maxAvoidDistance - projection) / maxAvoidDistance * Config::Monster::SteerAvoidanceForce;
                avoidForce += steerDir * strength;
            }
        }
    }
    
    return desiredVel + avoidForce;
}

