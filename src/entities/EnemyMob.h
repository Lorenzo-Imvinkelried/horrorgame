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
    BERSERKER_WARRIOR,
    DEATH_KNIGHT,
    SHADOW_ASSASSIN,
    SKELETON_ARCHER,
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

/**
 * @brief EnemyMob: Entidad enemiga modular y escalable.
 * Responsabilidades divididas en unidades funcionales:
 * - EnemyMob.cpp: Ciclo de vida, estadísticas y daño.
 * - EnemyMobAI.cpp: Inteligencia Artificial y máquinas de estado.
 * - EnemyMobMesh.cpp: Definición de mallas 3D procedurales.
 * - EnemyMobAnimation.cpp: Cinemática de extremidades y animación procedural.
 * - EnemyMobRenderer.cpp: Renderizado 3D y barras de salud en billboard.
 */
class EnemyMob {
public:
    EnemyMob(glm::vec3 spawnPos, EnemyType type, int nightLevel = 1);
    ~EnemyMob();

    // Actualización y Combate
    void Update(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers, ProjectileSystem& projectiles);
    bool TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers);

    // Renderizado
    void Render(GLuint shaderProgram);
    void RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos);

    // Getters y Setters
    glm::vec3 GetPosition() const { return m_pos; }
    glm::vec3& GetPositionRef() { return m_pos; }
    void SetPosition(const glm::vec3& p) { m_pos = p; }
    EnemyType GetType() const { return m_type; }
    std::string GetName() const;
    int GetLevel() const;
    int GetCurrentHP() const { return m_currentHp; }
    int GetMaxHP() const { return m_maxHp; }
    int GetExpReward() const;
    float GetRadius() const;
    bool IsAlive() const { return m_state != EnemyState::DEAD; }
    bool IsRemovable() const { return m_state == EnemyState::DEAD && m_deathTimer > 4.5f; }
    bool HasDroppedLoot() const { return m_lootDropped; }
    void SetLootDropped(bool d) { m_lootDropped = d; }
    void SetDead() { m_state = EnemyState::DEAD; m_currentHp = 0; m_deathTimer = 5.0f; }
    int GetNightLevel() const { return m_nightLevel; }
    int GetDefense() const { return m_defense; }
    int GetEvasion() const { return m_evasion; }

private:
    // Submódulos de IA (EnemyMobAI.cpp)
    void updateAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers, ProjectileSystem& projectiles);
    void updateMeleeAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers);
    void updateArcherAI(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, ProjectileSystem& projectiles);
    void updateMageAI(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, ProjectileSystem& projectiles);
    void updateTreantAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers);
    void updateGiantAI(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers);
    void updateIdleWander(float deltaTime);
    void pickWanderTarget();

    // Submódulo de Geometría (EnemyMobMesh.cpp)
    void initMeshes();

    // Submódulo de Animación y Cinemática (EnemyMobAnimation.cpp)
    void updateModelMesh();

    // Submódulo de Renderizado y UI (EnemyMobRenderer.cpp)
    static void initHpBarMesh();

    // Atributos de Estado e IA
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
    bool m_isEnraged;
    bool m_isAwakened;
    int m_nightLevel;

    // Atributos de RPG
    int m_maxHp;
    int m_currentHp;
    int m_defense;
    int m_evasion;

    // Temporizadores visuales y efectos
    float m_hitFlashTimer;
    float m_showHpBarTimer;
    float m_deathTimer;
    float m_eyePulse;
    bool m_lootDropped = false;

    // Buffers de OpenGL
    std::vector<BoxDef> m_baseBoxes;
    GLuint m_VAO;
    GLuint m_VBO;
    size_t m_vertexCount;

    static GLuint s_hpBarVAO;
    static GLuint s_hpBarVBO;
};
