#include "ScentManager.h"

ScentManager::ScentManager(int maxNodes) 
    : m_maxNodes(maxNodes), m_dropTimer(0.0f), m_dropInterval(2.0f), m_globalTime(0.0f) 
{
}

void ScentManager::Update(glm::vec3 playerPos, float deltaTime) {
    m_dropTimer += deltaTime;
    m_globalTime += deltaTime;
    
    if (m_dropTimer >= m_dropInterval) {
        m_dropTimer = 0.0f;
        
        // Add new node
        ScentNode node;
        node.pos = playerPos;
        node.time = m_globalTime;
        
        m_nodes.push_back(node);
        
        // Circular buffer behavior
        if ((int)m_nodes.size() > m_maxNodes) {
            m_nodes.erase(m_nodes.begin());
        }
    }
}

int ScentManager::GetClosestNodeIndex(glm::vec3 pos, float maxRadius) const {
    int closest = -1;
    float minDistSq = maxRadius * maxRadius;
    
    for (int i = 0; i < (int)m_nodes.size(); ++i) {
        float ds = glm::dot(pos - m_nodes[i].pos, pos - m_nodes[i].pos);
        if (ds < minDistSq) {
            minDistSq = ds;
            closest = i;
        }
    }
    
    return closest;
}

glm::vec3 ScentManager::GetNodePos(int index) const {
    if (index >= 0 && index < (int)m_nodes.size()) {
        return m_nodes[index].pos;
    }
    return glm::vec3(0.0f);
}
