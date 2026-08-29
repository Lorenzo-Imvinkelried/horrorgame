#include "ScentSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/type_ptr.hpp>
// #include <glm/gtx/norm.hpp> // Removed (Manual dot product)
#include <iostream>
#include "Config.h" // Needed for World Bounds

ScentSystem::ScentSystem() 
    : m_globalTime(0.0f), m_spawnTimer(0.0f), m_nextPacketId(0), VAO(0), VBO(0)
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
    
    // Logic Culling Radius (Only simulate scents relevant to player area)
    float maxLogicDistSq = Config::Scent::MaxDistance * Config::Scent::MaxDistance; 
    
    // LIMIT MAX PACKETS (Safety Valve - Stricter)
    if (m_packets.size() > 50) { // Reduced from 100
        // Remove oldest
        m_packets.erase(m_packets.begin(), m_packets.begin() + (m_packets.size() - 50));
    }

    // Spawn Packet
    if (m_spawnTimer >= Config::Scent::SpawnInterval) {
        m_spawnTimer = 0.0f;
        
        ScentPacket packet;
        packet.spawnPos = playerPos;
        packet.spawnPos.y += 1.0f; 
        packet.id = m_nextPacketId++; // Assign Unique ID 
        
        // SAFE NORMALIZE
        glm::vec3 flatWind(windDir.x, 0.0f, windDir.z);
        if (glm::length(flatWind) > 0.001f) {
             packet.windDir = glm::normalize(flatWind);
        } else {
             packet.windDir = glm::vec3(1,0,0); 
        }

        packet.spawnTime = m_globalTime;
        m_packets.push_back(packet);
    }

    // Culling & Update Loop
    for (auto it = m_packets.begin(); it != m_packets.end(); ) {
        float age = m_globalTime - it->spawnTime;
        
        // Calculate current tip position
        float distTravelled = Config::Scent::WindSpeed * age;
        glm::vec3 currentPos = it->spawnPos + it->windDir * distTravelled;
        
        // 1. Check Age & Logic Distance
        float distToPlayerSq = glm::dot(currentPos - playerPos, currentPos - playerPos);
        
        if (age > Config::Scent::MaxLifeTime || distToPlayerSq > maxLogicDistSq) {
             it = m_packets.erase(it);
             continue;
        }

        // 2. Check World Bounds (Backup)
        float d2 = currentPos.x*currentPos.x + currentPos.z*currentPos.z;
        if (d2 > killDistSq) {
            it = m_packets.erase(it);
        } else {
            ++it;
        }
    }
}

// Check if a point is inside any scent cone
bool ScentSystem::IsPointInScent(glm::vec3 pos, glm::vec3& outTrackDir) const {
    ScentNode node;
    if (GetScentAtPosition(pos, node)) {
        outTrackDir = -node.windDir; // Legacy support
        return true;
    }
    return false;
}

bool ScentSystem::GetScentAtPosition(glm::vec3 pos, ScentNode& outNode) const {
    if (m_packets.empty()) return false;

    // We assume packets are ordered.
    glm::vec3 prevPos(0);
    float prevWidth = 0;
    bool hasPrev = false;
    
    for (const auto& p : m_packets) {
        float age = m_globalTime - p.spawnTime;
        if (age < 0) continue;
        
        float distTravelled = Config::Scent::WindSpeed * age; 
        glm::vec3 currentPos = p.spawnPos + p.windDir * distTravelled;
        
        // OPTIMIZATION: Early Out if too far
        float distToPoint = glm::dot(pos - currentPos, pos - currentPos);
        if (distToPoint > 10000.0f) {
            // Update Prev for next iteration
            prevPos = currentPos;
            prevWidth = Config::Scent::BaseWidth + (distTravelled * Config::Scent::ExpansionRate);
            hasPrev = true;
            continue; 
        }
        
        float currentWidth = Config::Scent::BaseWidth + (distTravelled * Config::Scent::ExpansionRate);

        // 1. Check Puff (Circle) - Standard radial check
        // OPTIMIZATION: Use Squared Distance to avoid costly SQRT
        float distSqXZ = glm::dot(glm::vec2(pos.x - currentPos.x, pos.z - currentPos.z), 
                                  glm::vec2(pos.x - currentPos.x, pos.z - currentPos.z));
        
        bool collision = false;

        if (distSqXZ <= (currentWidth * currentWidth)) {
             collision = true;
        }

        // 2. Check Ribbon Segment (Connection to previous)
        if (!collision && hasPrev) {
             // Basic strict continuity check (same wind source approx)
             // OPTIMIZATION: Squared check for 20.0f (400.0f)
             float segLenSq = glm::dot(currentPos - prevPos, currentPos - prevPos);
             if (segLenSq < 400.0f) { // 20m^2 = 400
                 
                 // Calculate t for interpolation
                 glm::vec3 ab = currentPos - prevPos;
                 glm::vec3 ap = pos - prevPos;
                 float dotAB = glm::dot(ab, ab);
                 
                 if (dotAB > 0.0001f) {
                     float t = glm::dot(ap, ab) / dotAB;
                     t = glm::clamp(t, 0.0f, 1.0f);
                     
                     glm::vec3 closest = prevPos + ab * t;
                     glm::vec3 distVec = pos - closest;
                     float distSq = glm::dot(distVec, distVec);
                     
                     // Interpolate width based on t
                     float checkWidth = glm::mix(prevWidth, currentWidth, t);
                     
                     if (distSq <= (checkWidth * checkWidth)) {
                         collision = true;
                     }
                 }
             }
        }
        
        if (collision) {
             outNode.pos = currentPos;
             outNode.windDir = p.windDir;
             outNode.sourcePos = p.spawnPos; // CRITICAL: Where the player was
             outNode.strength = 1.0f - (age / Config::Scent::MaxLifeTime);
             outNode.nodeId = p.id; // Return ID
             return true;
        }

        prevPos = currentPos;
        prevWidth = currentWidth;
        hasPrev = true;
    }
    return false;
}

std::vector<glm::vec3> ScentSystem::GetPathToStrongestScent(glm::vec3 startPos, float radius) {
    std::vector<glm::vec3> path;
    if (m_packets.empty()) return path;

    // 1. Find the closest packet to startPos within radius (using XZ 2D check)
    int closestIdx = -1;
    float minDistSq = radius * radius;
    
    // Packets are chronological (mostly).
    for (int i = 0; i < m_packets.size(); ++i) {
        float age = m_globalTime - m_packets[i].spawnTime;
        float distTravelled = Config::Scent::WindSpeed * age;
        glm::vec3 packetPos = m_packets[i].spawnPos + m_packets[i].windDir * distTravelled;
        
        glm::vec2 diff2D(startPos.x - packetPos.x, startPos.z - packetPos.z);
        float distSq = glm::dot(diff2D, diff2D); // 2D distance squared
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestIdx = i;
        }
    }

    if (closestIdx != -1) {
        // 2. Build path from closest packet to the NEWEST packet (end of vector)
        // This makes the monster follow the breadcrumbs towards the player
        for (int i = closestIdx; i < m_packets.size(); ++i) {
            float age = m_globalTime - m_packets[i].spawnTime;
            float distTravelled = Config::Scent::WindSpeed * age;
            glm::vec3 packetPos = m_packets[i].spawnPos + m_packets[i].windDir * distTravelled;
            path.push_back(packetPos);
        }
    }

    return path;
}

glm::vec3 ScentSystem::GetLocalScentGradient(glm::vec3 pos, float radius) {
    glm::vec3 grad(0.0f);
    float totalWeight = 0.0f;
    float radSq = radius * radius;

    for (const auto& p : m_packets) {
        float age = m_globalTime - p.spawnTime;
        float distTravelled = Config::Scent::WindSpeed * age;
        glm::vec3 packetPos = p.spawnPos + p.windDir * distTravelled;

        glm::vec2 diff2D(pos.x - packetPos.x, pos.z - packetPos.z);
        float distSq = glm::dot(diff2D, diff2D);

        if (distSq < radSq) {
            float dist = std::sqrt(distSq);
            // Scent is stronger if fresher (small age) and closer (small dist)
            // Weight decay curves:
            float ageFactor = std::exp(-age * 0.08f); // Decays over age (max 1.0)
            float distFactor = 1.0f - (dist / radius); // Decays over distance (max 1.0)
            float weight = ageFactor * distFactor;

            glm::vec3 dir = packetPos - pos;
            dir.y = 0.0f; // Keep it on the XZ plane
            if (glm::length(dir) > 0.01f) {
                grad += glm::normalize(dir) * weight;
                totalWeight += weight;
            }
        }
    }

    if (totalWeight > 0.01f) {
        return glm::normalize(grad);
    }
    return glm::vec3(0.0f);
}

// Static storage for returning pointer (simple hack to avoid managing memory for single node)
static ScentNode g_tempNode;

ScentNode* ScentSystem::GetStrongestScentInRadius(glm::vec3 startPos, float radius) {
    if (m_packets.empty()) return nullptr;

    int bestIdx = -1;
    float minAge = 10000.0f;
    float radSq = radius * radius;

    // Find NEWEST packet in radius (accounting for puff size using XZ 2D check)
    for (int i = 0; i < m_packets.size(); ++i) {
        float age = m_globalTime - m_packets[i].spawnTime;
        float distTravelled = Config::Scent::WindSpeed * age;
        glm::vec3 packetPos = m_packets[i].spawnPos + m_packets[i].windDir * distTravelled;
        
        // Calculate dynamic puff width
        float currentWidth = Config::Scent::BaseWidth + (distTravelled * Config::Scent::ExpansionRate);
        float combinedRadius = radius + currentWidth;

        glm::vec2 diff2D(startPos.x - packetPos.x, startPos.z - packetPos.z);
        if (glm::dot(diff2D, diff2D) < (combinedRadius * combinedRadius)) {
            if (age < minAge) {
                minAge = age;
                bestIdx = i;
            }
        }
    }

    if (bestIdx != -1) {
        float age = m_globalTime - m_packets[bestIdx].spawnTime;
        float distTravelled = Config::Scent::WindSpeed * age;
        g_tempNode.pos = m_packets[bestIdx].spawnPos + m_packets[bestIdx].windDir * distTravelled;
        g_tempNode.strength = 1.0f; // Dummy strength
        g_tempNode.windDir = m_packets[bestIdx].windDir; // Pass wind direction
        g_tempNode.sourcePos = m_packets[bestIdx].spawnPos; // Pass source position
        return &g_tempNode;
    }

    return nullptr;
}


void ScentSystem::RenderDebug(GLuint shaderProgram, glm::vec3 cullPos) {
    if (m_packets.empty()) return;

    std::vector<float> lines;
    // Pre-allocate to avoid resize spikes. 100 packets * 8 segments * 2 verts * 9 floats ~ 14400 floats.
    lines.reserve(m_packets.size() * 100); 
    
    // Helper to add a 3D line
    auto addLine = [&](glm::vec3 a, glm::vec3 b) {
        lines.push_back(a.x); lines.push_back(a.y); lines.push_back(a.z); 
        lines.push_back(0); lines.push_back(1); lines.push_back(0); 
        lines.push_back(0); lines.push_back(0); lines.push_back(0); 
        
        lines.push_back(b.x); lines.push_back(b.y); lines.push_back(b.z); 
        lines.push_back(0); lines.push_back(1); lines.push_back(0); 
        lines.push_back(0); lines.push_back(0); lines.push_back(0);
    };

    // Helper to add a circle on XZ plane
    // OPTIMIZED: Reduced segments from 16 to 8 (Octagon) for performance
    auto addCircle = [&](glm::vec3 center, float radius) {
        const int segments = 8;
        const float angleStep = 6.2831853f / segments;
        
        for(int i = 0; i < segments; i++) {
            float a1 = i * angleStep;
            float a2 = (i + 1) * angleStep;
            
            glm::vec3 p1 = center + glm::vec3(cos(a1), 0, sin(a1)) * radius;
            glm::vec3 p2 = center + glm::vec3(cos(a2), 0, sin(a2)) * radius;
            
            addLine(p1, p2);
        }
    };

    glm::vec3 prevL, prevR;
    bool hasPrev = false;
    float cullDistSq = 60.0f * 60.0f; // 60 Meters Culling

    for (const auto& p : m_packets) {
        float age = m_globalTime - p.spawnTime;
        if (age < 0) continue;
        
        float distTravelled = Config::Scent::WindSpeed * age;
        glm::vec3 currentPos = p.spawnPos + p.windDir * distTravelled;
        
        // CULLING CHECK
        // If packet is too far from camera, skip adding geometry
        if (glm::dot(currentPos - cullPos, currentPos - cullPos) > cullDistSq) {
             hasPrev = false; // Break the ribbon continuity
             continue;
        }

        float currentWidth = Config::Scent::BaseWidth + (distTravelled * Config::Scent::ExpansionRate);
        
        // Draw the collision circle
        addCircle(currentPos, currentWidth);
        
        glm::vec3 up(0, 1, 0);
        glm::vec3 right = glm::normalize(glm::cross(p.windDir, up));
        
        glm::vec3 L = currentPos - right * currentWidth;
        glm::vec3 R = currentPos + right * currentWidth;
        
        // Use user requested "Lines" to connect the tunnel (Ribbon sides)
        if (hasPrev) {
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
    
    // Position (3) + Color (3) + Normal (3) = 9 floats per vertex
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    // Debug shader might use color as normal or generic param, ensuring attrib 1 is bound
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

    glDrawArrays(GL_LINES, 0, lines.size() / 9);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);
}

void ScentSystem::AddBloodScent(glm::vec3 pos, glm::vec3 windDir) {
    ScentPacket packet;
    packet.spawnPos = pos + glm::vec3(0.0f, 0.5f, 0.0f);
    packet.id = m_nextPacketId++;
    glm::vec3 flatWind(windDir.x, 0.0f, windDir.z);
    packet.windDir = (glm::length(flatWind) > 0.001f) ? glm::normalize(flatWind) : glm::vec3(1.0f, 0.0f, 0.0f);
    packet.spawnTime = m_globalTime;
    m_packets.push_back(packet);
}
