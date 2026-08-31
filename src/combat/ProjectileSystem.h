#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "ParticleSystem.h"
#include "DamageNumberSystem.h"

class Player;
class Monster;
class PassiveMob;
class EnemyMob;
class WaterMonster;
class Dragon;

struct MagicProjectile {
    glm::vec3 pos;
    glm::vec3 vel;
    int damage;
    glm::vec4 color;
    float lifeTimer;
    float maxLife;
    float radius;
    bool active;
    bool isFromPlayer;
};

class ProjectileSystem {
public:
    ProjectileSystem();
    ~ProjectileSystem();

    void Spawn(glm::vec3 startPos, glm::vec3 targetPos, float speed, int damage, glm::vec4 color, bool isFromPlayer = false);
    void Update(float deltaTime, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                std::vector<std::unique_ptr<Monster>>* monsters = nullptr,
                std::vector<std::unique_ptr<PassiveMob>>* passiveMobs = nullptr,
                std::vector<std::unique_ptr<EnemyMob>>* enemyMobs = nullptr,
                std::vector<std::unique_ptr<WaterMonster>>* waterMonsters = nullptr,
                Dragon* dragon = nullptr);
    void Render(GLuint shaderProgram);

private:
    void initMesh();

    std::vector<MagicProjectile> m_projectiles;
    GLuint m_VAO;
    GLuint m_VBO;
    int m_vertexCount;
};
