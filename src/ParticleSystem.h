#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Particle {
    glm::vec3 pos;
    glm::vec3 velocity;
    glm::vec4 color; // RGBA
    float size;
    float life;      // Current life
    float maxLife;   // Total life duration
    float gravity;   // Gravity factor (positive = up for smoke)
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void SpawnParticle(glm::vec3 pos, glm::vec3 vel, glm::vec4 color, float size, float life, float gravity);
    void Update(float deltaTime);
    void Render(GLuint shaderProgram, glm::vec3 camPos);

private:
    std::vector<Particle> particles;
    GLuint VAO, VBO;
};
