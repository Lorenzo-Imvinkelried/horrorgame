#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "ChunkManager.h"
#include "ScentSystem.h"
// #include "HideTronco.h" // REMOVED
#include "ModelLoader.h"

enum class MonsterAction {
    WANDER,
    INVESTIGATE,
    STALK,
    RETREAT,
    CLIMB_TREE,
    CHASE,
    TRACK_SCENT
};

class Monster {
public:
    Monster(glm::vec3 startPos);
    ~Monster();

    void Update(float deltaTime, glm::vec3 playerPos, glm::vec3 playerFront, glm::vec2 windDir,
                ChunkManager& chunkManager, ScentSystem& scentSystem, class ParticleSystem& particles,
                glm::vec3 playerVelocity, int playerAmmo, bool isPlayerReloading,
                bool isPlayerClimbing, glm::vec3 playerClimbingTreePos,
                bool isFlashlightOn);
    
    // Senses
    void HearSound(glm::vec3 sourcePos, float volume);

    // Updated Render to accept texture for eyes
    void Render(GLuint shaderProgram, GLuint whiteTexID); 
    void RenderDebug(GLuint shaderProgram);


    // Combat
    void TakeDamage(float amount, bool isHeadshot);
    bool IntersectRay(glm::vec3 origin, glm::vec3 dir, float& dist, bool& isHeadshot);
    bool IsDead() const { return m_isDead; }
    float GetHealth() const { return m_health; }
    bool HasDroppedLoot() const { return m_lootDropped; }
    void SetLootDropped(bool d) { m_lootDropped = d; }
    
    glm::vec3 GetPosition() const { return m_pos; }
    MonsterAction GetAction() const { return m_action; }
    float GetConfidence() const { return m_confidence; }
    float GetStress() const { return m_stress; }
    int GetEstimatedAmmo() const { return m_estimatedPlayerAmmo; }
    bool HasVisualContact() const { return m_hasVisualContact; }
    glm::vec3 GetKnownPlayerTreePos() const { return m_knownPlayerTreePos; }
    bool IsClimbing() const { return m_isClimbing; }
    float GetTreeClimbHeight() const { return m_treeClimbHeight; }
    bool IsEnraged() const { return m_isEnraged; }
    void SetPosition(glm::vec3 pos) { m_pos = pos; m_visualPos = pos; }
    void LookAt(glm::vec3 target);

private:
    void BuildDeformedMesh();
    void AnimateMesh();
    float m_animTime;
    
    // Body Mesh (Noisy)
    std::vector<float> m_meshVertices;
    GLuint VAO, VBO;

    // Eye Mesh (Solid/No Noise)
    std::vector<float> m_eyeVertices;
    GLuint VAO_Eyes, VBO_Eyes;
    
    // AI & Physics (Smooth/Real)
    glm::vec3 m_pos;
    glm::vec3 m_velocity;
    float m_yaw;
    float m_targetYaw;
    float m_headYaw;
    MonsterAction m_action;
    float m_health;
    bool m_isDead;
    bool m_lootDropped = false;
    float m_speed;
    
    // Scent Pathfinding State
    glm::vec3 m_cachedStealthDir;             // For Input Stability (Hysteresis)
    float m_timeSinceLastScent;               // For Logic Persistence (Memory)
    float m_scentCheckTimer;                  // Control sampling rate (0.1s)
    int m_lastSmelledId;                      // To prevent duplicate messages
    glm::vec3 m_debugScentDir;                // Visualization vector
    std::vector<glm::vec4> m_detectedTrees;   // Cache for debug visualization (vec4 to match ChunkManager)
    std::vector<glm::vec3> m_treeVectors;     // 2D Directions to trees
    glm::vec3 m_bestTreeDir;                  // Direction to the CHOSEN tree (Logic)
    int m_bestTreeIndex;                      // Index of the chosen tree (for visualization)
    
    // Memory & Knowledge (Utility AI)
    glm::vec3 m_memPlayerPos;
    float m_memTimeSinceSeen;
    float m_memTimeSinceHeard;
    float m_memTimeSinceSmelled;
    float m_memTimeSinceAimedAt;
    
    // AI State Machine Variables
    glm::vec3 m_targetPos;
    glm::vec3 m_assignedTreePos;
    glm::vec3 m_trackingTargetPos;
    glm::vec3 m_trackingDir;
    float m_stateTimer;
    bool m_hasVisualContact;
    float m_treeClimbHeight;
    bool m_isClimbing;
    glm::vec3 m_patrolCenter;
    
    // Upgraded AI parameters
    float m_headPitch;
    float m_startleTimer;
    float m_startleCooldownTimer;
    float m_peekTimer;
    float m_peekAngle;
    float m_sniffParticleTimer;
    bool m_hasStartled;
    std::vector<glm::vec3> m_scentPath;
    int m_scentPathIndex;
    float m_assignedTreeScale;
    
    // Stuck detector and feint state variables
    float m_stuckTimer;
    glm::vec3 m_prevPos;
    float m_feintTimer;
    float m_feintAngle;
    
    // Decision commitment and organic uncertainty variables
    float m_decisionLockTimer;
    int m_estimatedPlayerAmmo;
    bool m_wasPlayerReloading;
    glm::vec3 m_estimatedPlayerPos;
    float m_estimatedPlayerPosTimer;
    float m_canopyWaitTime;
    float m_confidence;
    float m_stress;
    float m_leafDropTimer;
    
    // Anti-Camp System
    glm::vec3 m_antiCampCenter;
    float m_antiCampTimer;

    // Twig-Snap Stalking Dash
    float m_stalkDashTimer;
    glm::vec3 m_stalkDashDir;
    
    // Player climbing awareness
    glm::vec3 m_knownPlayerTreePos;
    
    glm::vec3 ApplyObstacleAvoidance(glm::vec3 desiredVel, ChunkManager& chunkManager);
    
    // Visual Decoupling (15 FPS)
    glm::vec3 m_visualPos;
    float m_visualYaw;
    float m_visualTickTimer;
    float m_visualFPS = 15.0f;

    // AI Components
    // HideTronco m_stealthAI; // REMOVED
    
    // Internal Logic
    // Returns index of best tree in m_detectedTrees, or -1
    int GetBestTreeIndex();
    bool CheckLineOfSight(glm::vec3 playerPos, class ChunkManager& chunkManager);
    bool GetBlockingTree(glm::vec3 targetPos, class ChunkManager& chunkManager, glm::vec4& outTree);

    // Flanking & Tactical Navigation
    bool m_chooseLeftFlank;
    float m_flankDecisionTimer;
    glm::vec3 m_tacticalTargetOffset;

    // Legendary Hunter AI
    float m_exposure;
    bool m_isCrouching;
    float m_crouchFactor;
    float m_distractionTimer;
    float m_spiralSearchTimer;
    float m_spiralSearchRadius;
    bool m_isEnraged;
    float m_rageTimer;
    bool m_isPanicked;
    float m_panicTimer;
    glm::vec3 m_eyeColor;
    float m_eyeBrightness;
    float m_bleedTimer;
    float m_fakeChargeTimer;
    float m_climbCooldownTimer;
    bool m_shouldScream;
    float m_spiralSearchAngle;
    glm::vec3 m_lastScentWorldPos;
    bool m_hasLastScent;

    float MultiRaycastExposure(glm::vec3 playerPos, class ChunkManager& chunkManager);

    std::vector<glm::vec4> m_nearbyTreesCache; // Cache for collision loops
    
    // Model Data (Loaded from file)
    std::vector<struct BoxDef> m_basePose;
    
    // Dynamic Hitboxes (Calculated from m_basePose)
    glm::vec3 m_bodyMin, m_bodyMax;
    glm::vec3 m_headMin, m_headMax;
};
