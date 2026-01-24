#pragma once

#include <vector>
#include <glm/glm.hpp>

struct ScentNode {
    glm::vec3 pos;
    float time; // Time when the scent was dropped
};

class ScentManager {
public:
    ScentManager(int maxNodes = 60);
    
    // Call every frame/tick
    void Update(glm::vec3 playerPos, float deltaTime);
    
    // Finds the closest scent node to a given position
    // returns -1 if no nodes found
    int GetClosestNodeIndex(glm::vec3 pos, float maxRadius = 10.0f) const;
    
    // Returns a node by index
    glm::vec3 GetNodePos(int index) const;
    
    const std::vector<ScentNode>& GetNodes() const { return m_nodes; }

private:
    std::vector<ScentNode> m_nodes;
    int m_maxNodes;
    float m_dropTimer;
    float m_dropInterval = 2.0f; // Drop scent every 2 seconds
    float m_globalTime = 0.0f;
};
