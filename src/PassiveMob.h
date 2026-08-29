#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "ModelLoader.h"
#include "ParticleSystem.h"

enum class PassiveMobState {
    IDLE,
    WANDER,
    FLEE,
    CHASE,
    DEAD
};

enum class DeerSize {
    FAWN,
    ADULT,
    ALPHA,
    DEMONIC
};

class Player;
class DamageNumberSystem;

class PassiveMob {
public:
    PassiveMob(glm::vec3 spawnPos, DeerSize size = DeerSize::ADULT);
    ~PassiveMob();

    void Update(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers);
    void Render(GLuint shaderProgram);
    void RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos);

    bool TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers);

    bool IsAlive() const { return m_state != PassiveMobState::DEAD; }
    bool IsRemovable() const { return m_state == PassiveMobState::DEAD && m_deathTimer > 5.0f; }
    glm::vec3 GetPosition() const { return m_pos; }
    float GetRadius() const { return 0.85f * m_scale; }
    int GetCurrentHP() const { return m_currentHp; }
    int GetMaxHP() const { return m_maxHp; }
    DeerSize GetDeerSize() const { return m_size; }
    std::string GetName() const;
    int GetLevel() const;
    int GetExpReward() const;
    bool IsSkinned() const { return m_isSkinned; }
    void SetSkinned(bool s) { m_isSkinned = s; }

private:
    void updateModelMesh();
    void pickNewWanderTarget();

    DeerSize m_size;
    float m_scale;
    glm::vec3 m_pos;
    glm::vec3 m_spawnOrigin;
    glm::vec3 m_targetPos;
    glm::vec3 m_fleeDir;
    float m_yaw; // Angle in radians
    float m_speed;
    float m_animTimer;
    float m_stateTimer;
    float m_attackCooldown;
    PassiveMobState m_state;

    // RPG Stats (src_rpgarena_logic)
    int m_maxHp;
    int m_currentHp;
    int m_defense;
    int m_evasion;
    float m_hitFlashTimer;
    float m_showHpBarTimer;
    float m_deathTimer;
    float m_headGrazeAngle;
    float m_glowPulse;
    bool m_isSkinned = false;

    // OpenGL Buffers
    std::vector<BoxDef> m_baseBoxes;
    GLuint m_VAO;
    GLuint m_VBO;
    size_t m_vertexCount;

    // Static Health Bar Mesh
    static GLuint s_hpBarVAO;
    static GLuint s_hpBarVBO;
    static void initHpBarMesh();
};
