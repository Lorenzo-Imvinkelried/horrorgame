#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class ParticleSystem;

enum class WeatherState {
    CLEAR_SUNNY = 0,
    OVERCAST,
    MYSTIC_FOG,
    LIGHT_RAIN,
    EMBER_WIND,
    BLOOD_MOON_STORM
};

class WeatherSystem {
public:
    WeatherSystem();
    ~WeatherSystem();

    void Update(float deltaTime, float dayCycleTime, glm::vec3 playerPos, glm::vec3 windDir, ParticleSystem& particles);
    void RenderCelestialBodies(GLuint shaderProgram, glm::vec3 cameraPos, float dayCycleTime, float globalTime);

    bool IsBloodMoon() const { return m_state == WeatherState::BLOOD_MOON_STORM; }
    bool IsStorm() const { return m_state == WeatherState::LIGHT_RAIN || m_state == WeatherState::BLOOD_MOON_STORM; }
    float GetLightningFlash() const { return m_lightningFlash; }
    glm::vec3 GetAdjustedFog(glm::vec3 baseFog) const;

    WeatherState GetState() const { return m_state; }
    int GetNightCount() const { return m_dayCounter; }
    const char* GetWeatherName() const;

private:
    void initCelestialMeshes();
    void pickNextWeather(bool isNight);

    WeatherState m_state = WeatherState::CLEAR_SUNNY;
    float m_weatherTimer = 0.0f;
    float m_weatherDuration = 220.0f;
    float m_lightningTimer = 0.0f;
    float m_lightningFlash = 0.0f;
    int m_dayCounter = 1;
    float m_lastDayCycleTime = 0.0f;

    // Celestial Body Geometry (Sun & Moon)
    GLuint m_sunVAO = 0;
    GLuint m_sunVBO = 0;
    size_t m_sunVertexCount = 0;

    GLuint m_moonVAO = 0;
    GLuint m_moonVBO = 0;
    size_t m_moonVertexCount = 0;
};
