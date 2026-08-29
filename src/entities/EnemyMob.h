#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "ModelLoader.h"
#include "ParticleSystem.h"
#include "DamageNumberSystem.h"
#include "ProjectileSystem.h"

enum class EnemyType {
    CORRUPTED_WARRIOR,
    NEUTRAL_GIANT,
    DARK_MAGE,
    TREANT,
    VAMPIRE
};

enum class EnemyState {
    IDLE,
    WANDER,
    CHASE,
    ATTACK,
    KITE_BACK,
    DEAD
};

class Player;

class EnemyMob {
public:
    EnemyMob(glm::vec3 spawnPos, EnemyType type, int nightLevel = 1);
    ~EnemyMob();

    void Update(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers, ProjectileSystem& projectiles);
    bool TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers);
    void Render(GLuint shaderProgram);
    void RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos);

    glm::vec3 GetPosition() const { return m_pos; }
    EnemyType GetType() const { return m_type; }
    std::string GetName() const;
    int GetLevel() const;
    int GetCurrentHP() const { return m_currentHp; }
    int GetMaxHP() const { return m_maxHp; }
    int GetExpReward() const;
    float GetRadius() const;
    bool IsAlive() const { return m_state != EnemyState::DEAD; }
    bool IsRemovable() const { return m_state == EnemyState::DEAD && m_deathTimer > 4.5f; }

private:
    void initMeshes();
    void updateModelMesh();
    void pickWanderTarget();

    EnemyType m_type;
    EnemyState m_state;
    float m_scale;
    glm::vec3 m_pos;
    glm::vec3 m_spawnOrigin;
    glm::vec3 m_targetPos;
    float m_yaw;
    float m_speed;
    float m_animTimer;
    float m_stateTimer;
    float m_attackCooldown;
    float m_attackAnimProgress;
    bool m_isEnraged; // For Neutral Giant
    bool m_isAwakened = false; // For Living Treant
    int m_nightLevel = 1;

    int m_maxHp;
    int m_currentHp;
    int m_defense;
    int m_evasion;

    float m_hitFlashTimer;
    float m_showHpBarTimer;
    float m_deathTimer;
    float m_eyePulse;

    std::vector<BoxDef> m_baseBoxes;
    GLuint m_VAO;
    GLuint m_VBO;
    size_t m_vertexCount;

    static GLuint s_hpBarVAO;
    static GLuint s_hpBarVBO;
    static void initHpBarMesh();
};
