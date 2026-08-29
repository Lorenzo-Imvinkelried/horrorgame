#include "Monster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "Config.h"
#include "WorldGenerator.h"

static inline glm::vec3 SafeCross(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& fallback = glm::vec3(1.0f, 0.0f, 0.0f)) {
    glm::vec3 c = glm::cross(v1, v2);
    float len = glm::length(c);
    if (!std::isnan(len) && len > 0.0001f) {
        return c / len;
    }
    return fallback;
}

void Monster::LookAt(glm::vec3 target) {
    glm::vec3 dir = glm::normalize(target - m_pos);
    m_yaw = glm::degrees(atan2(dir.x, dir.z));
    m_targetYaw = m_yaw;
    m_visualYaw = m_yaw;
}

int Monster::GetBestTreeIndex() {
    if (m_detectedTrees.empty()) return -1;

    int bestIndex = -1;
    float bestDot = -2.0f;

    glm::vec3 flatMonsterPos(m_pos.x, 0.0f, m_pos.z);
    glm::vec3 flatScentDir = glm::normalize(glm::vec3(m_debugScentDir.x, 0.0f, m_debugScentDir.z));

    for (int i = 0; i < (int)m_detectedTrees.size(); ++i) {
        glm::vec3 flatTreePos(m_detectedTrees[i].x, 0.0f, m_detectedTrees[i].z);
        glm::vec3 dirToTree = flatTreePos - flatMonsterPos;
        if (glm::length(dirToTree) < 0.1f) continue;
        
        dirToTree = glm::normalize(dirToTree);
        float dot = glm::dot(dirToTree, flatScentDir);
        
        if (dot > bestDot) {
            bestDot = dot;
            bestIndex = i;
        }
    }

    return bestIndex;
}

bool Monster::CheckLineOfSight(glm::vec3 playerPos, ChunkManager& chunkManager) {
    glm::vec3 startPos = m_visualPos + glm::vec3(0, 2.0f, 0); // Approx Head
    glm::vec3 endPos = playerPos + glm::vec3(0, 1.5f, 0);
    glm::vec3 dir = endPos - startPos;
    float dist = glm::length(dir);
    if (dist < 0.1f) return true;
    dir = glm::normalize(dir);

    // 1. Terrain Check
    float stepSize = 1.0f;
    for (float d = stepSize; d < dist; d += stepSize) {
        glm::vec3 p = startPos + dir * d;
        float h = WorldGenerator::GetHeight(p.x, p.z);
        if (p.y < h) return false;
    }

    // 2. Tree Check
    std::vector<glm::vec4> trees;
    chunkManager.GetTreesInRange(startPos, dist + 2.0f, trees);
    
    glm::vec2 A(startPos.x, startPos.z);
    glm::vec2 B(endPos.x, endPos.z);
    
    for (const auto& t : trees) {
        glm::vec2 center(t.x, t.z);
        if (glm::distance(A, center) < 0.1f) continue;
        float radius = 0.5f + (0.6f * t.w);

        glm::vec2 ac = center - A;
        glm::vec2 ab = B - A;
        float ab2 = glm::dot(ab, ab);
        if (ab2 == 0.0f) continue;

        float d = glm::dot(ac, ab) / ab2;
        
        glm::vec2 closest;
        if (d < 0.0f) closest = A;
        else if (d > 1.0f) closest = B;
        else closest = A + d * ab;

        float distToCenterSq = glm::dot(closest - center, closest - center);
        
        // A. Trunk Check
        if (distToCenterSq < radius * radius) {
            float dClamp = glm::clamp(d, 0.0f, 1.0f);
            float rayY = startPos.y + (endPos.y - startPos.y) * dClamp;
            if (rayY <= t.y + 6.0f * t.w) {
                return false;
            }
        }
        
        // B. Leaves Check
        float leavesRadius = 3.0f * t.w;
        if (distToCenterSq < leavesRadius * leavesRadius) {
            float dClamp = glm::clamp(d, 0.0f, 1.0f);
            float rayY = startPos.y + (endPos.y - startPos.y) * dClamp;
            if (rayY >= t.y + 5.0f * t.w && rayY <= t.y + 25.0f * t.w) {
                bool hasAttentiveHearing = (m_memTimeSinceHeard < 4.0f && dist < 15.0f);
                if (!hasAttentiveHearing) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool Monster::GetBlockingTree(glm::vec3 targetPos, ChunkManager& chunkManager, glm::vec4& outTree) {
    glm::vec3 startPos = m_pos + glm::vec3(0, 1.0f, 0);
    glm::vec3 endPos = targetPos + glm::vec3(0, 1.0f, 0);
    glm::vec3 dir = endPos - startPos;
    float dist = glm::length(dir);
    if (dist < 0.1f) return false;
    dir = glm::normalize(dir);

    // 1. Terrain Check
    float stepSize = 1.0f;
    for (float d = stepSize; d < dist; d += stepSize) {
        glm::vec3 p = startPos + dir * d;
        float h = WorldGenerator::GetHeight(p.x, p.z);
        if (p.y < h) return false;
    }

    // 2. Tree Check
    std::vector<glm::vec4> trees;
    chunkManager.GetTreesInRange(startPos, dist + 2.0f, trees);
    
    glm::vec2 A(startPos.x, startPos.z);
    glm::vec2 B(endPos.x, endPos.z);
    
    float closestDist = 9999.0f;
    bool found = false;

    for (const auto& t : trees) {
        glm::vec2 center(t.x, t.z);
        if (glm::distance(A, center) < 0.1f) continue;
        float radius = 0.5f + (0.6f * t.w);

        glm::vec2 ac = center - A;
        glm::vec2 ab = B - A;
        float ab2 = glm::dot(ab, ab);
        if (ab2 == 0.0f) continue;

        float d = glm::dot(ac, ab) / ab2;
        
        glm::vec2 closest;
        if (d < 0.0f) closest = A;
        else if (d > 1.0f) closest = B;
        else closest = A + d * ab;

        float distToCenterSq = glm::dot(closest - center, closest - center);
        if (distToCenterSq < radius * radius) {
            float distToTree = glm::distance(A, center);
            if (distToTree < closestDist) {
                closestDist = distToTree;
                outTree = t;
                found = true;
            }
        }
    }
    return found;
}

float Monster::MultiRaycastExposure(glm::vec3 playerPos, ChunkManager& chunkManager) {
    std::vector<glm::vec3> samplePoints = {
        m_pos + glm::vec3(0.0f, 2.0f, 0.0f),
        m_pos + glm::vec3(0.0f, 1.2f, 0.0f),
        m_pos + glm::vec3(0.0f, 0.2f, 0.0f)
    };
    
    int hits = 0;
    glm::vec3 startPos = playerPos + glm::vec3(0.0f, 1.5f, 0.0f);
    
    float maxDist = 0.0f;
    for (const auto& targetPoint : samplePoints) {
        maxDist = std::max(maxDist, glm::distance(startPos, targetPoint));
    }
    
    std::vector<glm::vec4> trees;
    chunkManager.GetTreesInRange(startPos, maxDist + 2.0f, trees);
    
    for (const auto& targetPoint : samplePoints) {
        glm::vec3 dir = targetPoint - startPos;
        float dist = glm::length(dir);
        if (dist < 0.1f) {
            hits++;
            continue;
        }
        dir = glm::normalize(dir);
        
        bool blocked = false;
        
        // A. Check terrain collision
        float stepSize = 1.0f;
        for (float d = stepSize; d < dist; d += stepSize) {
            glm::vec3 p = startPos + dir * d;
            float h = WorldGenerator::GetHeight(p.x, p.z);
            if (p.y < h) {
                blocked = true;
                break;
            }
        }
        
        if (blocked) continue;
        
        // B. Check tree collision
        glm::vec2 A(startPos.x, startPos.z);
        glm::vec2 B(targetPoint.x, targetPoint.z);
        
        for (const auto& t : trees) {
            glm::vec2 center(t.x, t.z);
            if (glm::distance(A, center) < 0.1f) continue;
            float radius = 0.5f + (0.6f * t.w);
            
            glm::vec2 ac = center - A;
            glm::vec2 ab = B - A;
            float ab2 = glm::dot(ab, ab);
            if (ab2 == 0.0f) continue;
            
            float d = glm::dot(ac, ab) / ab2;
            
            glm::vec2 closest;
            if (d < 0.0f) closest = A;
            else if (d > 1.0f) closest = B;
            else closest = A + d * ab;
            
            float distToCenterSq = glm::dot(closest - center, closest - center);
            
            // Trunk Check
            if (distToCenterSq < radius * radius) {
                float dClamp = glm::clamp(d, 0.0f, 1.0f);
                float rayY = startPos.y + (targetPoint.y - startPos.y) * dClamp;
                if (rayY <= t.y + 6.0f * t.w) {
                    blocked = true;
                    break;
                }
            }
            
            // Leaves Check
            float leavesRadius = 3.0f * t.w;
            if (distToCenterSq < leavesRadius * leavesRadius) {
                float dClamp = glm::clamp(d, 0.0f, 1.0f);
                float rayY = startPos.y + (targetPoint.y - startPos.y) * dClamp;
                if (rayY >= t.y + 5.0f * t.w && rayY <= t.y + 25.0f * t.w) {
                    bool hasAttentiveHearing = (m_memTimeSinceHeard < 4.0f);
                    if (!hasAttentiveHearing) {
                        blocked = true;
                        break;
                    }
                }
            }
        }
        
        if (!blocked) {
            hits++;
        }
    }
    
    return (float)hits / 3.0f;
}

glm::vec3 Monster::ApplyObstacleAvoidance(glm::vec3 desiredVel, ChunkManager& chunkManager) {
    if (glm::length(desiredVel) < 0.01f) return desiredVel;
    
    if (m_nearbyTreesCache.empty() && m_action != MonsterAction::CLIMB_TREE) {
        chunkManager.GetTreesInRange(m_pos, 5.0f, m_nearbyTreesCache);
    }
    
    glm::vec3 forward = glm::normalize(desiredVel);
    glm::vec3 avoidForce(0.0f);

    // Valley/Gully steering bias
    if (m_action == MonsterAction::STALK || m_action == MonsterAction::RETREAT) {
        glm::vec3 right = SafeCross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
        
        float sampleDist = 3.5f;
        glm::vec3 centerPt = m_pos + forward * sampleDist;
        glm::vec3 leftPt = m_pos + (forward * 0.866f - right * 0.5f) * sampleDist;
        glm::vec3 rightPt = m_pos + (forward * 0.866f + right * 0.5f) * sampleDist;
        
        float hCenter = WorldGenerator::GetHeight(centerPt.x, centerPt.z);
        float hLeft = WorldGenerator::GetHeight(leftPt.x, leftPt.z);
        float hRight = WorldGenerator::GetHeight(rightPt.x, rightPt.z);
        
        glm::vec3 terrainForce(0.0f);
        if (hLeft < hCenter && hLeft < hRight) {
            float slopeDiff = hCenter - hLeft;
            terrainForce = -right * glm::clamp(slopeDiff * 1.5f, 0.0f, 2.5f);
        } else if (hRight < hCenter && hRight < hLeft) {
            float slopeDiff = hCenter - hRight;
            terrainForce = right * glm::clamp(slopeDiff * 1.5f, 0.0f, 2.5f);
        }
        
        avoidForce += terrainForce;
    }
    
    float maxAvoidDistance = 4.0f;
    
    for (const auto& t : m_nearbyTreesCache) {
        glm::vec3 treePos(t.x, m_pos.y, t.z);
        glm::vec3 toTree = treePos - m_pos;
        
        float projection = glm::dot(toTree, forward);
        
        if (projection > 0.0f && projection < maxAvoidDistance) {
            glm::vec3 lateral = toTree - forward * projection;
            float lateralDist = glm::length(lateral);
            
            float treeRadius = 0.5f + (0.6f * t.w);
            float safeMargin = treeRadius + 0.6f;
            
            if (lateralDist < safeMargin) {
                glm::vec3 steerDir;
                if (lateralDist > 0.01f) {
                    steerDir = glm::normalize(-lateral);
                } else {
                    steerDir = SafeCross(forward, glm::vec3(0, 1, 0));
                }
                
                float strength = (maxAvoidDistance - projection) / maxAvoidDistance * Config::Monster::SteerAvoidanceForce;
                avoidForce += steerDir * strength;
            }
        }
    }
    
    return desiredVel + avoidForce;
}
