#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "ParticleSystem.h"
#include "DamageNumberSystem.h"

class Player;

struct MagicProjectile {
    glm::vec3 pos;
    glm::vec3 vel;
    int damage;
    glm::vec4 color;
    float lifeTimer;
    float maxLife;
    float radius;
    bool active;
};

class ProjectileSystem {
public:
    ProjectileSystem();
    ~ProjectileSystem();

    void Spawn(glm::vec3 startPos, glm::vec3 targetPos, float speed, int damage, glm::vec4 color);
    void Update(float deltaTime, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers);
    void Render(GLuint shaderProgram);

private:
    void initMesh();

    std::vector<MagicProjectile> m_projectiles;
    GLuint m_VAO;
    GLuint m_VBO;
    int m_vertexCount;
};
