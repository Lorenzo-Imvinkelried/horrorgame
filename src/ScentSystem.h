#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct ScentPacket {
    glm::vec3 spawnPos;
    glm::vec3 windDir;
    float spawnTime;
};

class ScentSystem {
public:
    ScentSystem();
    ~ScentSystem();

    void Update(float deltaTime, glm::vec3 playerPos, glm::vec3 windDir, float windSpeed);
    void RenderDebug(GLuint shaderProgram, glm::vec3 cullPos);

    // Physics / Logic
    // Returns true if 'point' is inside any active scent cone.
    // outTrackDir will be set to the direction tracking should go (usually -windDir)
    bool IsPointInScent(glm::vec3 point, glm::vec3& outTrackDir) const;

private:
    std::vector<ScentPacket> m_packets;
    float m_globalTime;
    float m_spawnTimer;
    
    // Rendering resources
    GLuint VAO, VBO;
};
