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
    SOARING,      // Vuelo circular majestuoso por el cielo
    DIVE_BOMB,    // Picada rasante hacia el jugador con rugido
    BREATH_FIRE,  // Aliento de fuego llameante en picada
    ASCENDING     // Ascenso de regreso a la altitud de vuelo
};

class Dragon {
public:
    Dragon(glm::vec3 spawnPos = glm::vec3(0.0f, 42.0f, 0.0f));
    ~Dragon();

    void Update(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles, DamageNumberSystem& damageNumbers);
    void Render(GLuint shaderProgram);
    void RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos);

    bool TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, DamageNumberSystem& damageNumbers);

    glm::vec3 GetPosition() const { return m_pos; }
    glm::vec3& GetPositionRef() { return m_pos; }
    float GetRadius() const { return 3.6f; }
    int GetCurrentHP() const { return m_currentHp; }
    int GetMaxHP() const { return m_maxHp; }
    std::string GetName() const { return "Dragon Ancestral Wyvern"; }
    int GetLevel() const { return 15; }
    bool IsAlive() const { return m_currentHp > 0; }

private:
    void initMeshes();
    void updateModelMesh();

    glm::vec3 m_pos;
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

    // OpenGL Buffers
    GLuint m_VAO;
    GLuint m_VBO;
    size_t m_vertexCount;
    std::vector<BoxDef> m_baseBoxes;
};
