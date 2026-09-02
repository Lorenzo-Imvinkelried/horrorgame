#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include "WorldGenerator.h" 
#include "FootprintSystem.h"
#include "ChunkManager.h"
#include "ModelLoader.h"
#include "CombatStats.h"
#include "DamageNumberSystem.h"

class Monster;
class PassiveMob;
class ParticleSystem;

class Player {
public:
    Player(glm::vec3 startPos = glm::vec3(0.0f, 15.0f, 0.0f));
    ~Player();

    void ProcessMouseMovement(float xoffset, float yoffset);
    void ProcessMouseScroll(float yoffset);
    void ProcessKeyboard(int key, float deltaTime, ChunkManager& chunkManager, FootprintSystem& footprints);
    void Update(float deltaTime); // Physics update
    void Render(GLuint shaderProgram);
    void RenderFirstPersonSword(GLuint shaderProgram);
    void RenderFirstPersonTorch(GLuint shaderProgram);
    void RenderDebug(GLuint shaderProgram);

    glm::mat4 GetViewMatrix();
    glm::vec3 GetCameraPosition();
    glm::vec3 GetWeaponOffset(); // For rendering the weapon
    glm::vec3 GetTorchPosition() const;
    bool AreBothHandsOccupied() const;
    void ToggleCameraMode() { IsThirdPerson = !IsThirdPerson; }
    void ToggleTorch() { 
        if (AreBothHandsOccupied()) {
            HasTorchActive = false;
            return;
        }
        HasTorchActive = !HasTorchActive; 
    }

    // Torch State (Off-hand left hand)
    bool HasTorchActive = true;

    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // 3rd Person Attributes
    bool IsThirdPerson = true;
    bool IsFreeOrbiting = false;
    float CameraDistance = 3.5f;
    float MinCameraDistance = 1.0f;
    float MaxCameraDistance = 12.0f;
    float CameraPitch = 0.0f;
    float ModelYaw = 0.0f;
    float WalkAnimTimer = 0.0f;

    // Euler Angles
    float Yaw;
    float Pitch;

    // Physics
    glm::vec3 Velocity;
    bool IsGrounded;
    bool IsClimbing = false;
    glm::vec3 ClimbingTreePos = glm::vec3(0.0f);
    float ClimbingTreeScale = 1.0f;
    float Gravity = 20.0f;
    float JumpForce = 8.0f;
    float WalkSpeed = 6.0f;
    float PlayerHeight = 1.6f;
    float PlayerRadius = 0.35f;

    // Noise/Sound Emission System
    float SoundVolumeEmitted = 0.0f;
    float m_climbNoiseTimer = 0.35f;
    float m_cameraNoiseAccumulator = 0.0f;

    // Game Feel
    float HeadBobTimer;
    float HeadBobAmount = 0.08f;
    float HeadBobSpeed = 10.0f;
    
    // Idle Breathing
    float BreathTimer = 0.0f;
    float BreathAmount = 0.035f;
    float BreathSpeed = 2.5f;

    // Weapon Sway
    glm::vec3 WeaponSwayPos;
    float SwayAmount = 0.05f;
    float SwaySmoothing = 5.0f;

    // RPG Stats & Progression (src_rpgarena_logic)
    PlayerStats Stats;

    // Melee Combat (Sword & Shield/Guard)
    float StunTimer = 0.0f;
    bool TryAttack();
    void SetBlocking(bool blocking) { m_isBlocking = blocking; }
    bool IsAttacking() const { return m_attackTimer > 0.0f; }
    bool IsBlocking() const { return m_isBlocking; }
    void TakeDamage(int dmg, DamageNumberSystem& damageNumbers, struct FatalErrorPopup* fatalError = nullptr, bool shadowAegis = false);
    void UpdateCombat(float deltaTime, std::vector<std::unique_ptr<Monster>>& monsters, std::vector<std::unique_ptr<PassiveMob>>& passiveMobs, std::vector<std::unique_ptr<class EnemyMob>>& enemyMobs, std::vector<std::unique_ptr<class WaterMonster>>& waterMonsters, ParticleSystem& particles, DamageNumberSystem& damageNumbers, class Dragon* dragon = nullptr, std::vector<std::unique_ptr<class BaseMob>>* baseMobs = nullptr);

    // Death & Respawn
    bool IsDead() const { return m_isDead; }
    void Respawn(glm::vec3 spawnPos = glm::vec3(0.0f, 15.0f, 0.0f));
    float DeathTimer = 0.0f;

    // Equipment Visuals & 3D Preview
    void UpdateEquipmentVisuals(const std::string& mainHandId, const std::string& chestId, const std::string& headId, const std::string& offHandId, const std::string& legsId = "", const std::string& feetId = "", const std::string& glovesId = "");
    void RenderPreview(GLuint shaderProgram, const glm::mat4& modelMatrix);
    const std::string& GetEquippedMainHand() const { return m_equippedMainHandId; }

private:
    void updateCameraVectors();
    float getTerrainHeight(float x, float z);

    bool m_isDead = false;
    std::string m_equippedMainHandId = "";
    std::string m_equippedChestId = "";
    std::string m_equippedHeadId = "";
    std::string m_equippedOffHandId = "";
    std::string m_equippedLegsId = "";
    std::string m_equippedFeetId = "";
    std::string m_equippedGlovesId = "";
    bool m_fpMeshNeedsRebuild = true;
    GLuint m_fpVAO = 0;
    GLuint m_fpVBO = 0;
    size_t m_fpVertexCount = 0;

    // Combat State
    float m_attackTimer = 0.0f;
    float m_attackDuration = 0.58f;
    float m_attackCooldownTimer = 0.0f;
    float m_attackCooldown = 0.35f;
    bool m_attackHitDone = false;
    bool m_isBlocking = false;
    int m_attackCombo = 0;
    int m_activeAttackHand = 0; // 0 = mano derecha, 1 = mano izquierda (empuñadura dual)

    // 3D Model Resources
    std::vector<BoxDef> m_baseBoxes;
    GLuint m_playerVAO = 0;
    GLuint m_playerVBO = 0;
    size_t m_playerVertexCount = 0;
    void initModel();
    void updateModelMesh();
};

