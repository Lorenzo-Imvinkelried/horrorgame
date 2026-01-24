#ifndef WIND_SYSTEM_H
#define WIND_SYSTEM_H

#include <glm/glm.hpp>
#include <random>

class WindSystem {
public:
    WindSystem();

    void Update(float deltaTime);
    void SetDirection(float x, float y);
    glm::vec2 GetDirection() const;
    float GetStrength() const;

private:
    glm::vec2 m_CurrentDir;
    glm::vec2 m_TargetDir;
    
    float m_Timer;
    float m_ChangeInterval; // e.g. 120.0f
    
    // Smooth transition
    float m_TransitionSpeed;

    void PickNewTarget();
};

#endif
