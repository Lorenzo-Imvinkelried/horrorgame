#include "WindSystem.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

WindSystem::WindSystem() 
    : m_Timer(0.0f)
    , m_ChangeInterval(120.0f) // 2 Minutes
    , m_TransitionSpeed(0.5f)
{
    // Initial random start
    PickNewTarget();
    m_CurrentDir = m_TargetDir;
}

void WindSystem::PickNewTarget() {
    // Random angle 0..2PI
    float angle = ((float)rand() / RAND_MAX) * 6.28318f;
    m_TargetDir = glm::vec2(cos(angle), sin(angle));
    
    // Normalize just in case
    if (glm::length(m_TargetDir) > 0.0f) {
        m_TargetDir = glm::normalize(m_TargetDir);
    }
    
    // Debug output
    std::cout << "[WindSystem] Wind changing direction to: (" 
              << m_TargetDir.x << ", " << m_TargetDir.y << ")" << std::endl;
}

void WindSystem::SetDirection(float x, float y) {
    m_TargetDir = glm::normalize(glm::vec2(x, y));
    m_CurrentDir = m_TargetDir; // Snap immediately for responsiveness
    std::cout << "[WindSystem] Manual override: (" << x << ", " << y << ")" << std::endl;
}

void WindSystem::Update(float deltaTime) {
    m_Timer += deltaTime;
    if (m_Timer >= m_ChangeInterval) {
        m_Timer = 0.0f;
        PickNewTarget();
    }

    // Smoothly interpolate current -> target
    // Simple Lerp: current = mix(current, target, speed * dt)
    // For direction vectors, Slerp is better, but for small steps Lerp+Normalize is fine.
    glm::vec2 diff = m_TargetDir - m_CurrentDir;
    if (glm::length(diff) > 0.001f) {
        m_CurrentDir += diff * (m_TransitionSpeed * deltaTime);
        m_CurrentDir = glm::normalize(m_CurrentDir);
    }
}

glm::vec2 WindSystem::GetDirection() const {
    return m_CurrentDir;
}

float WindSystem::GetStrength() const {
    return 1.0f; // Could oscillate this too if desired later
}
