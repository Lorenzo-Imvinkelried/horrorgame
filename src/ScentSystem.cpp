#include "ScentSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Config.h" // Needed for World Bounds

ScentSystem::ScentSystem() 
    : m_globalTime(0.0f), m_spawnTimer(0.0f), VAO(0), VBO(0)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

ScentSystem::~ScentSystem() {
    if(VAO) glDeleteVertexArrays(1, &VAO);
    if(VBO) glDeleteBuffers(1, &VBO);
}

void ScentSystem::Update(float deltaTime, glm::vec3 playerPos, glm::vec3 windDir, float windStrength) {
    m_globalTime += deltaTime;
    m_spawnTimer += deltaTime;

    // Radius of world in meters
    float worldRadius = Config::World::MapRadius * Config::World::ChunkSize * Config::World::ChunkScale;
    float killDistSq = worldRadius * worldRadius;

    // Spawn Packet (1 per second)
    if (m_spawnTimer >= m_spawnInterval) {
        m_spawnTimer = 0.0f;
        
        ScentPacket packet;
        packet.spawnPos = playerPos;
        packet.spawnPos.y += 1.0f; 
        
        packet.windDir = glm::normalize(glm::vec3(windDir.x, 0.0f, windDir.z)); 
        packet.spawnTime = m_globalTime;
        
        m_packets.push_back(packet);
    }

    // Cull only if out of world
    for (auto it = m_packets.begin(); it != m_packets.end(); ) {
        float age = m_globalTime - it->spawnTime;
        float distTravelled = ScentSystem::WindSpeed * age;
        glm::vec3 tip = it->spawnPos + it->windDir * distTravelled;
        
        // Simple distance check from center (0,0)
        float d2 = tip.x*tip.x + tip.z*tip.z;
        if (d2 > killDistSq) {
            it = m_packets.erase(it);
        } else {
            ++it;
        }
    }
}

// Check if a point is inside any scent cone
bool ScentSystem::IsPointInScent(glm::vec3 pos, glm::vec3& outTrackDir) const {
    if (m_packets.empty()) return false;

    // Iterate segments between packets to check "Ribbon" containment
    // We assume packets are ordered.
    
    // Helper: Distance squared from point P to segment AB
    auto distSqToSegment = [](glm::vec3 p, glm::vec3 a, glm::vec3 b) {
        glm::vec3 ab = b - a;
        glm::vec3 ap = p - a;
        float t = glm::dot(ap, ab) / glm::dot(ab, ab);
        t = glm::clamp(t, 0.0f, 1.0f);
        glm::vec3 closest = a + ab * t;
        glm::vec3 distVec = p - closest;
        return glm::dot(distVec, distVec);
    };

    glm::vec3 prevPos(0);
    float prevWidth = 0;
    bool hasPrev = false;

    for (const auto& p : m_packets) {
        float age = m_globalTime - p.spawnTime;
        if (age < 0) continue;
        
        float distTravelled = 5.0f * age; 
        glm::vec3 currentPos = p.spawnPos + p.windDir * distTravelled;
        float currentWidth = BaseWidth + (distTravelled * ExpansionRate); 
        
        // 1. Check Puff (Circle) - Good for single packets
        float distXZ = glm::length(glm::vec2(pos.x - currentPos.x, pos.z - currentPos.z));
        if (distXZ <= currentWidth) {
             outTrackDir = -p.windDir;
             return true;
        }

        // 2. Check Ribbon Segment (Connection to previous)
        if (hasPrev) {
             // Basic strict continuity check (same wind source approx)
             if (glm::distance(currentPos, prevPos) < 20.0f) { // 20m gap max
                // CLIP FORWARD CAP: Ensure point is behind the wave front
                // currentPos is the leading edge. p.windDir is the direction of travel.
                // If dot > 0, we are ahead of the front.
                if (glm::dot(pos - currentPos, p.windDir) > 0.0f) {
                    // Ahead of the wave (Invisible Capsule Cap) -> Ignore
                    prevPos = currentPos;
                    prevWidth = currentWidth;
                    continue; 
                }

                 float dSq = distSqToSegment(pos, prevPos, currentPos);
                 // Interpolate width roughly? Use max for safety.
                 float maxWidth = std::max(currentWidth, prevWidth);
                 

                     if (dSq <= (maxWidth * maxWidth)) {
                         // DEBUG: Log Hit
                         static float lastLogTime = 0.0f;
                         if (m_globalTime - lastLogTime > 0.5f) {
                             std::cout << "[ScentSystem] HIT! DistSq: " << dSq << " RadiusSq: " << (maxWidth*maxWidth) << "\n"
                                       << "   Monster: (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n"
                                       << "   SegStart: (" << prevPos.x << ", " << prevPos.y << ", " << prevPos.z << ")\n"
                                       << "   SegEnd:   (" << currentPos.x << ", " << currentPos.y << ", " << currentPos.z << ")" << std::endl;
                             lastLogTime = m_globalTime;
                         }
                         outTrackDir = -p.windDir; // Use current packet wind
                         return true;
                     } // End Hit Check
             }
        }
        
        prevPos = currentPos;
        prevWidth = currentWidth;
        hasPrev = true;
    }
    return false;
}

void ScentSystem::RenderDebug(GLuint shaderProgram) {
    // VISUALIZATION: RIBBON (Connected Lines)
    
    if (m_packets.empty()) return;

    std::vector<float> lines;
    
    auto addLine = [&](glm::vec3 a, glm::vec3 b) {
        lines.push_back(a.x); lines.push_back(a.y); lines.push_back(a.z); 
        lines.push_back(0); lines.push_back(1); lines.push_back(0); 
        lines.push_back(0); lines.push_back(0); lines.push_back(0); 
        
        lines.push_back(b.x); lines.push_back(b.y); lines.push_back(b.z); 
        lines.push_back(0); lines.push_back(1); lines.push_back(0); 
        lines.push_back(0); lines.push_back(0); lines.push_back(0);
    };

    glm::vec3 prevL, prevR;
    bool hasPrev = false;

    // Packets are likely ordered by spawn time? 
    // Newest is last?
    // We want to connect them in order.
    // Iterating begin() to end() should follow the order of creation.
    
    for (const auto& p : m_packets) {
        float age = m_globalTime - p.spawnTime;
        if (age < 0) continue;
        
        float distTravelled = ScentSystem::WindSpeed * age;
        glm::vec3 currentPos = p.spawnPos + p.windDir * distTravelled;
        float currentWidth = BaseWidth + (distTravelled * ExpansionRate);
        
        glm::vec3 up(0, 1, 0);
        glm::vec3 right = glm::normalize(glm::cross(p.windDir, up));
        
        glm::vec3 L = currentPos - right * currentWidth;
        glm::vec3 R = currentPos + right * currentWidth;
        
        // Draw the "Wave Front" (The current pulse line)
        addLine(L, R);
        
        // Connect to previous packet (Ribbon sides)
        // User requested: "uni esas dos lineas" (Close the area)
        if (hasPrev) {
             // Check if packets are related (same wind source?)
             // Simple heuristic: distance check. If too far, don't connect (new trail)
             if (glm::distance(currentPos, (prevL+prevR)*0.5f) < 20.0f) {
                 addLine(L, prevL);
                 addLine(R, prevR);
             }
        }
        
        prevL = L;
        prevR = R;
        hasPrev = true;
    }
    
    if (lines.empty()) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glDisable(GL_DEPTH_TEST); 
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

    glDrawArrays(GL_LINES, 0, lines.size() / 9);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);
}
