#include "HideTronco.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <iostream>

HideTronco::HideTronco() : VAO(0), VBO(0) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

glm::vec3 HideTronco::Update(glm::vec3 monsterPos, glm::vec3 scentDir, ChunkManager& chunkManager, float deltaTime) {
    
    // STATE MACHINE
    if (m_state == State::PEEKING) {
        m_peekTimer -= deltaTime;
        if (m_peekTimer <= 0.0f) {
            // Finished peeking, move to next tree
            std::cout << "[HideTronco] Peeking FINISHED (" << m_peekDuration << "s). Switching to MOVING." << std::endl;
            m_state = State::MOVING;
            m_hasTarget = false; // Force re-scan
        } else {
            // Stay at Hide Pos
            // Optional spam log: std::cout << "[HideTronco] Peeking... " << m_peekTimer << std::endl;
            return m_targetHidePos;
        }
    }

    // MOVING STATE
    
    // Check if we arrived at current target (if any)
    if (m_hasTarget) {
        m_moveTimer += deltaTime;
        
        float dist = glm::length(monsterPos - m_targetHidePos);
        
        // TIMEOUT FAILSAFE (If stuck walking into tree)
        if (m_moveTimer > m_moveTimeout) {
             std::cout << "[HideTronco] STUCK DETECTED (Time: " << m_moveTimer << "s). Forcing PEEK state." << std::endl;
             m_state = State::PEEKING;
             m_peekTimer = m_peekDuration;
             m_lastTreePos = m_targetTreePos; 
             return m_targetHidePos;
        }

        if (dist < 0.5f) {
            // Arrived!
            std::cout << "[HideTronco] ARRIVED at target (Dist: " << dist << "). Switching to PEEKING." << std::endl;
            m_state = State::PEEKING;
            m_peekTimer = m_peekDuration;
            m_lastTreePos = m_targetTreePos; // Remember this tree to avoid it next
            return m_targetHidePos;
        }
        // Continue to target
        return m_targetHidePos;
    }

    // Find New Target
    // THROTTLING: Only scan every 0.2s (5Hz)
    // If we already have a target, we don't need to re-scan constantly unless we lose it?
    // Actually, we re-scan when we DON'T have a target.
    // If m_hasTarget is false, we scan.
    
    m_updateTimer -= deltaTime;
    if (m_updateTimer > 0.0f) {
        return monsterPos; // Wait for next tick
    }
    m_updateTimer = 0.2f; // Reset timer (5Hz)

    // 1. Get Trees
    m_treeCache.clear();
    chunkManager.GetTreesInRange(monsterPos, m_searchRadius, m_treeCache);
    
    if (m_state != State::PEEKING) {
        // Only log during search to avoid spam
         // std::cout << "[HideTronco] Scanning... Found " << m_treeCache.size() << " trees in radius " << m_searchRadius << std::endl;
    }

    if (m_treeCache.empty()) return monsterPos; 
    
    // We use the scent direction as the target direction
    glm::vec3 dirToTarget = glm::normalize(scentDir);
    
    // 2. Filter & Score
    int bestTreeIdx = -1;
    float closestDist = 10000.0f;
    
    for (int i = 0; i < m_treeCache.size(); i++) {
        glm::vec4& t = m_treeCache[i];
        glm::vec3 tPos(t.x, t.y, t.z);
        
        // AVOID LAST TREE
        if (glm::length(tPos - m_lastTreePos) < 2.0f) {
             // std::cout << "  Tree " << i << ": EXCLUDED (Last Tree)" << std::endl;
             continue; 
        }

        glm::vec3 toTree = tPos - monsterPos;
        float distToTree = glm::length(toTree);
        
        if (distToTree < 1.0f || distToTree > m_searchRadius) {
            // std::cout << "  Tree " << i << ": Skip Radius (Dist: " << distToTree << ")" << std::endl;
            continue;
        }
        
        // DIRECTIONAL LOGIC
        float forwardDist = glm::dot(toTree, dirToTarget);
        
        if (forwardDist < 3.0f) {
            // std::cout << "  Tree " << i << ": Rejected (Backwards/Sideways: " << forwardDist << "m)" << std::endl;
            continue; 
        }
        
        // Valid candidate
        // std::cout << "  Tree " << i << ": Candidate! Forward: " << forwardDist << "m, Dist: " << distToTree << "m" << std::endl;

        if (distToTree < closestDist) {
            closestDist = distToTree;
            bestTreeIdx = i;
        }
    }
    
    if (bestTreeIdx != -1) {
        // std::cout << "[HideTronco] SELECTED Tree Index " << bestTreeIdx << " at dist " << closestDist << std::endl;
        m_targetTreePos = glm::vec3(m_treeCache[bestTreeIdx].x, m_treeCache[bestTreeIdx].y, m_treeCache[bestTreeIdx].z);
        m_hasTarget = true;
        m_moveTimer = 0.0f; // Reset Failsafe
        
        // Dynamic Timeout: (Distance / Speed) + Buffer
        // Assume MonsterSpeed ~3.5f. Let's use 3.0f conservatively.
        // Buffer = 4.0s for acceleration/rotation/collision
        m_moveTimeout = (closestDist / 3.0f) + 4.0f;
        
        // Hide Position
        // Hide Position: Behind the tree relative to the scent source
        // ScentDir points to Source. We want to be on the opposite side.
        glm::vec3 hideDir = -dirToTarget; // Point AWAY from source
        m_targetHidePos = m_targetTreePos + hideDir * 2.5f; // Increased distance to avoid collision stuck
        
        return m_targetHidePos;
    } else {
        // std::cout << "[HideTronco] Failed to find ANY valid tree in direction." << std::endl;
    }
    
    return monsterPos; // No valid tree found
}

void HideTronco::RenderDebug(GLuint shaderProgram, glm::vec3 monsterPos) {
    // 1. Draw Radius Circle (Violet)
    // Violet: 0.5, 0.0, 1.0
    
    std::vector<float> lines;
    int segments = 32;
    float step = 6.28318f / segments;
    
    // Circle
    for(int i=0; i<segments; i++) {
        float a1 = i * step;
        float a2 = (i+1) * step;
        
        glm::vec3 p1 = monsterPos + glm::vec3(cos(a1), 0, sin(a1)) * m_searchRadius;
        glm::vec3 p2 = monsterPos + glm::vec3(cos(a2), 0, sin(a2)) * m_searchRadius;
        
        // P1
        lines.push_back(p1.x); lines.push_back(p1.y + 0.5f); lines.push_back(p1.z);
        lines.push_back(0.6f); lines.push_back(0.0f); lines.push_back(1.0f); // Violet
        lines.push_back(0); lines.push_back(0); lines.push_back(0);
        
        // P2
        lines.push_back(p2.x); lines.push_back(p2.y + 0.5f); lines.push_back(p2.z);
        lines.push_back(0.6f); lines.push_back(0.0f); lines.push_back(1.0f);
        lines.push_back(0); lines.push_back(0); lines.push_back(0);
    }
    
    // 2. Draw Target Tree Box (if any)
    if (m_hasTarget) {
        float size = 0.5f;
        glm::vec3 t = m_targetTreePos;
        glm::vec3 c1(0.8f, 0.0f, 1.0f); // Brighter Violet
        
        // Simple Vertical Line or Box
        // Let's do a vertical marker
        auto addLine = [&](glm::vec3 a, glm::vec3 b) {
            lines.push_back(a.x); lines.push_back(a.y); lines.push_back(a.z); 
            lines.push_back(c1.r); lines.push_back(c1.g); lines.push_back(c1.b); 
            lines.push_back(0); lines.push_back(0); lines.push_back(0); 
            
            lines.push_back(b.x); lines.push_back(b.y); lines.push_back(b.z); 
            lines.push_back(c1.r); lines.push_back(c1.g); lines.push_back(c1.b); 
            lines.push_back(0); lines.push_back(0); lines.push_back(0); 
        };
        
        // Box Corners roughly
        glm::vec3 bMin = t - glm::vec3(0.5f, 0, 0.5f);
        glm::vec3 bMax = t + glm::vec3(0.5f, 4.0f, 0.5f);
        
        addLine({bMin.x, bMin.y, bMin.z}, {bMax.x, bMin.y, bMin.z});
        addLine({bMin.x, bMin.y, bMin.z}, {bMin.x, bMax.y, bMin.z});
        addLine({bMin.x, bMin.y, bMin.z}, {bMin.x, bMin.y, bMax.z});
        // ... (minimal box)
        addLine(t, t + glm::vec3(0, 5, 0)); // Spike
    }

    // 3. Draw Last Tree (Red) to verify exclusion
    {
        glm::vec3 t = m_lastTreePos;
        glm::vec3 c2(1.0f, 0.0f, 0.0f); // Red
        
        auto addLineRed = [&](glm::vec3 a, glm::vec3 b) {
            lines.push_back(a.x); lines.push_back(a.y); lines.push_back(a.z); 
            lines.push_back(c2.r); lines.push_back(c2.g); lines.push_back(c2.b); 
            lines.push_back(0); lines.push_back(0); lines.push_back(0); 
            
            lines.push_back(b.x); lines.push_back(b.y); lines.push_back(b.z); 
            lines.push_back(c2.r); lines.push_back(c2.g); lines.push_back(c2.b); 
            lines.push_back(0); lines.push_back(0); lines.push_back(0); 
        };

        if (glm::length(t) > 0.1f) {
            glm::vec3 bMin = t - glm::vec3(0.3f, 0, 0.3f);
            glm::vec3 bMax = t + glm::vec3(0.3f, 3.0f, 0.3f);
            
            addLineRed({bMin.x, bMin.y, bMin.z}, {bMax.x, bMin.y, bMin.z});
            addLineRed({bMin.x, bMin.y, bMin.z}, {bMin.x, bMax.y, bMin.z});
            addLineRed({bMin.x, bMin.y, bMin.z}, {bMin.x, bMin.y, bMax.z});
        }
    }
    
    if (lines.empty()) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glDisable(GL_DEPTH_TEST); 
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1); // Color

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

    glDrawArrays(GL_LINES, 0, lines.size() / 9);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);
}
