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

enum class ProjectileType {
    MAGIC_ORB = 0,
    ARROW,
    FIREBALL
};

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
    ProjectileType type = ProjectileType::MAGIC_ORB;
};

class ProjectileSystem {
public:
    ProjectileSystem();
    ~ProjectileSystem();

    void Spawn(glm::vec3 startPos, glm::vec3 targetPos, float speed, int damage, glm::vec4 color, 
               bool isFromPlayer = false, ProjectileType type = ProjectileType::MAGIC_ORB);
    void Update(float deltaTime, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                std::vector<std::unique_ptr<Monster>>* monsters = nullptr,
                std::vector<std::unique_ptr<PassiveMob>>* passiveMobs = nullptr,
                std::vector<std::unique_ptr<EnemyMob>>* enemyMobs = nullptr,
                std::vector<std::unique_ptr<WaterMonster>>* waterMonsters = nullptr,
                Dragon* dragon = nullptr);
    void Render(GLuint shaderProgram);

private:
    void initMesh();
    void initArrowMesh();

    std::vector<MagicProjectile> m_projectiles;
    GLuint m_VAO;
    GLuint m_VBO;
    int m_vertexCount;

    GLuint m_arrowVAO = 0;
    GLuint m_arrowVBO = 0;
    int m_arrowVertexCount = 0;
};
