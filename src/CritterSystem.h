#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "WorldGenerator.h"

struct Butterfly {
    glm::vec3 pos;
    glm::vec3 origin;
    float flightAngle;
    float flightRadius;
    float wingTimer;
    float wingSpeed;
    glm::vec3 color;
};

struct Firefly {
    glm::vec3 pos;
    glm::vec3 origin;
    float phaseOffset;
    float glowTimer;
    glm::vec3 color;
    float driftSpeed;
};

struct Frog {
    glm::vec3 pos;
    glm::vec3 startJumpPos;
    glm::vec3 targetJumpPos;
    float yaw;
    float jumpTimer;
    float jumpDuration;
    float nextJumpTimer;
    bool isJumping;
    float croakTimer;
};

class CritterSystem {
public:
    CritterSystem();
    ~CritterSystem();

    void Init(glm::vec3 playerPos);
    void Update(float deltaTime, glm::vec3 playerPos);
    void Render(GLuint shaderProgram);

private:
    std::vector<Butterfly> m_butterflies;
    std::vector<Firefly> m_fireflies;
    std::vector<Frog> m_frogs;

    GLuint m_critterVAO = 0;
    GLuint m_critterVBO = 0;
    size_t m_vertexCount = 0;

    void updateCritterMesh();
    void buildFrogMesh(const Frog& frog, std::vector<Vertex>& vertices);
    void buildButterflyMesh(const Butterfly& b, std::vector<Vertex>& vertices);
    void buildFireflyMesh(const Firefly& f, std::vector<Vertex>& vertices);
};
