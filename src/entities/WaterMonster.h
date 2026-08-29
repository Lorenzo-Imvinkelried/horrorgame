#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "ModelLoader.h"
#include "ParticleSystem.h"
#include "DamageNumberSystem.h"

class Player;

enum class WaterMonsterState {
    SUBMERGED_LURKING,
    EMERGING_LUNGE,
    DRAGGING_PLAYER,
    RETREAT_DEEP,
    DEAD
};

class WaterMonster {
public:
    WaterMonster(glm::vec3 spawnPos);
    ~WaterMonster();

    void Update(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers);
    bool TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers);
    void Render(GLuint shaderProgram);
    void RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos);

    glm::vec3 GetPosition() const { return m_pos; }
    std::string GetName() const { return "Devorador del Lago"; }
    int GetLevel() const { return 5; }
    int GetCurrentHP() const { return m_currentHp; }
    int GetMaxHP() const { return m_maxHp; }
    int GetExpReward() const { return 180; }
    float GetRadius() const { return 1.2f; }
    bool IsAlive() const { return m_state != WaterMonsterState::DEAD; }
    bool IsDragging() const { return m_state == WaterMonsterState::DRAGGING_PLAYER; }
    bool IsRemovable() const { return m_state == WaterMonsterState::DEAD && m_deathTimer > 4.0f; }

private:
    void initMesh();
    void updateModelMesh();

    WaterMonsterState m_state;
    glm::vec3 m_pos;
    glm::vec3 m_lakeCenterPos;
    float m_yaw;
    float m_animTimer;
    float m_dragTimer;
    float m_splashTimer;
    float m_attackCooldown;
    float m_eyeGlowPulse;

    int m_maxHp;
    int m_currentHp;
    int m_defense;

    float m_hitFlashTimer;
    float m_showHpBarTimer;
    float m_deathTimer;

    std::vector<BoxDef> m_baseBoxes;
    GLuint m_VAO;
    GLuint m_VBO;
    size_t m_vertexCount;

    static GLuint s_hpBarVAO;
    static GLuint s_hpBarVBO;
    static void initHpBarMesh();
};
