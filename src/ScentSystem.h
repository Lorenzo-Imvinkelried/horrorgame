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
    void RenderDebug(GLuint shaderProgram);

    // Physics / Logic
    // Returns true if 'point' is inside any active scent cone.
    // outTrackDir will be set to the direction tracking should go (usually -windDir)
    bool IsPointInScent(glm::vec3 point, glm::vec3& outTrackDir) const;

    // Config Constants (Shared between Logic and Render)
    static constexpr float BaseWidth = 1.0f; // Slightly wider start
    static constexpr float ExpansionRate = 0.3f; // Much slower expansion (was 3.0)
    static constexpr float WindSpeed = 5.0f; 

private:
    std::vector<ScentPacket> m_packets;
    float m_globalTime;
    float m_spawnTimer;
    
    // Rendering resources
    GLuint VAO, VBO;
    
    // Config
    float m_packetLifeTime = 120.0f; // Infinite essentially (bounds check handles it)
    float m_spawnInterval = 2.0f;   
};
