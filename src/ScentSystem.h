#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct ScentNode {
    glm::vec3 pos;
    float strength;
    glm::vec3 windDir; // Direction the scent is traveling
    glm::vec3 sourcePos; // The origin of this scent (Player Pos at spawn)
    int nodeId; // Unique ID of the packet
};

struct ScentPacket {
    glm::vec3 spawnPos;
    glm::vec3 windDir;
    float spawnTime;
    int id;
};

class ScentSystem {
public:
    ScentSystem();
    ~ScentSystem();

    void Update(float deltaTime, glm::vec3 playerPos, glm::vec3 windDir, float windStrength);
    void RenderDebug(GLuint shaderProgram, glm::vec3 cullPos);

    // Physics / Logic
    // Returns true if 'point' is inside any active scent cone.
    // outTrackDir will be set to the direction tracking should go (usually -windDir)
    bool IsPointInScent(glm::vec3 point, glm::vec3& outTrackDir) const;

    // AI Query
    // Returns true if 'pos' is inside a scent packet. Fills 'outNode' with details.
    bool GetScentAtPosition(glm::vec3 pos, ScentNode& outNode) const;

    // AI Query Methods
    // Returns a path of points from the closest scent packet to the newest (player position)
    std::vector<glm::vec3> GetPathToStrongestScent(glm::vec3 startPos, float radius);
    
    // Fallback: Just get single best node
    ScentNode* GetStrongestScentInRadius(glm::vec3 startPos, float radius);

private:
    std::vector<ScentPacket> m_packets;
    float m_globalTime;
    float m_spawnTimer;
    int m_nextPacketId;
    
    // Rendering resources
    GLuint VAO, VBO;
};
