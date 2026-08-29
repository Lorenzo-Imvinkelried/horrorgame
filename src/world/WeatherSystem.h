#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class ParticleSystem;

enum class WeatherState {
    CLEAR,
    STORM,
    BLOOD_MOON
};

class WeatherSystem {
public:
    WeatherSystem();
    ~WeatherSystem();

    void Update(float deltaTime, float dayCycleTime, glm::vec3 playerPos, glm::vec3 windDir, ParticleSystem& particles);

    bool IsBloodMoon() const { return m_state == WeatherState::BLOOD_MOON; }
    bool IsStorm() const { return m_state == WeatherState::STORM; }
    float GetLightningFlash() const { return m_lightningFlash; }
    glm::vec3 GetAdjustedFog(glm::vec3 baseFog) const;

    WeatherState GetState() const { return m_state; }
    int GetNightCount() const { return m_dayCounter; }

private:
    WeatherState m_state = WeatherState::CLEAR;
    float m_weatherTimer = 0.0f;
    float m_lightningTimer = 0.0f;
    float m_lightningFlash = 0.0f;
    int m_dayCounter = 1;
    float m_lastDayCycleTime = 0.0f;
};
