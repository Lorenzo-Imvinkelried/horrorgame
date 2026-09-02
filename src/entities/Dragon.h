#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "ModelLoader.h"
#include "ParticleSystem.h"
#include "DamageNumberSystem.h"

class Player;

enum class DragonState {
    PATROL_SKY,   // Patrulla majestuosa por el cielo y las cumbres
    SOARING,      // Vuelo circular de acecho
    DIVE_BOMB,    // Picada rasante hacia el objetivo con rugido
    BREATH_FIRE,  // Aliento de fuego llameante en picada
    ASCENDING,    // Ascenso de regreso a la altitud de vuelo
    DYING,        // Caída dramática envuelta en llamas y humo
    DEAD          // Yacente en tierra
};

class Dragon {
public:
    Dragon(glm::vec3 spawnPos = glm::vec3(80.0f, 48.0f, 80.0f));
    ~Dragon();

    void Update(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, DamageNumberSystem& damageNumbers, class Player* player = nullptr);
    void Render(GLuint shaderProgram);
    void RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos);

    bool TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, DamageNumberSystem& damageNumbers, class Player* player = nullptr);

    glm::vec3 GetPosition() const { return m_pos; }
    glm::vec3& GetPositionRef() { return m_pos; }
    float GetRadius() const { return 4.5f; }
    int GetCurrentHP() const { return m_currentHp; }
    int GetMaxHP() const { return m_maxHp; }
    std::string GetName() const { return "DRAGON ANCESTRAL WYVERN (JEFE)"; }
    int GetLevel() const { return 25; }
    int GetExpReward() const { return 650; }
    bool IsAlive() const { return m_currentHp > 0; }
    bool IsDying() const { return m_state == DragonState::DYING; }
    bool IsDead() const { return m_state == DragonState::DEAD; }
    bool HasDroppedLoot() const { return m_lootDropped; }
    void SetLootDropped(bool d) { m_lootDropped = d; }
    void SetActive(bool active) {
        if (!active) {
            m_currentHp = 0;
            m_state = DragonState::DEAD;
            m_lootDropped = true;
        } else {
            m_currentHp = m_maxHp;
            m_state = DragonState::PATROL_SKY;
            m_lootDropped = false;
        }
    }

private:
    void initMeshes();
    void updateModelMesh();

    glm::vec3 m_pos;
    glm::vec3 m_territoryCenter;
    glm::vec3 m_velocity;
    float m_yaw;   // Ángulo horizontal en radianes
    float m_pitch; // Inclinación vertical en radianes
    float m_roll;  // Inclinación lateral en giros

    float m_flightAltitude;
    float m_circleRadius;
    float m_flightSpeed;
    float m_orbitAngle;

    float m_animTimer;
    float m_wingFlap;
    float m_stateTimer;
    DragonState m_state;

    int m_maxHp;
    int m_currentHp;
    float m_hitFlashTimer;
    float m_showHpBarTimer;
    float m_breathCooldown;
    float m_deathTimer;
    bool m_lootDropped = false;
    bool m_isAggro = false;

    // OpenGL Buffers
    GLuint m_VAO;
    GLuint m_VBO;
    size_t m_vertexCount;
    std::vector<BoxDef> m_baseBoxes;
};
