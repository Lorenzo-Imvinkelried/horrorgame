#include "WeatherSystem.h"
#include "ParticleSystem.h"
#include <cmath>
#include <algorithm>

WeatherSystem::WeatherSystem() {
    m_lightningTimer = 6.0f + (rand() % 60 / 10.0f);
}

WeatherSystem::~WeatherSystem() {}

void WeatherSystem::Update(float deltaTime, float dayCycleTime, glm::vec3 playerPos, glm::vec3 windDir, ParticleSystem& particles) {
    if (dayCycleTime < m_lastDayCycleTime) {
        m_dayCounter++;
    }
    m_lastDayCycleTime = dayCycleTime;

    bool isNight = (dayCycleTime >= 120.0f && dayCycleTime <= 228.0f);

    // Weather state transitions
    if (isNight && (m_dayCounter % 2 == 0 || m_dayCounter % 3 == 0)) {
        m_state = WeatherState::BLOOD_MOON;
    } else {
        m_weatherTimer += deltaTime;
        if (m_weatherTimer > 70.0f) {
            m_weatherTimer = 0.0f;
            m_state = (m_state == WeatherState::CLEAR) ? WeatherState::STORM : WeatherState::CLEAR;
        }
    }

    // Lightning Flash Decay
    if (m_lightningFlash > 0.0f) {
        m_lightningFlash -= deltaTime * 4.0f;
        if (m_lightningFlash < 0.0f) m_lightningFlash = 0.0f;
    }

    // Storm Precipitation & Lightning System
    if (m_state == WeatherState::STORM) {
        // Heavy Rain Particles
        for (int i = 0; i < 18; ++i) {
            float rx = (rand() % 100 / 50.0f - 1.0f) * 22.0f;
            float rz = (rand() % 100 / 50.0f - 1.0f) * 22.0f;
            glm::vec3 rPos = playerPos + glm::vec3(rx, 14.0f + (rand() % 100 / 20.0f), rz);
            glm::vec3 rVel = glm::vec3(windDir.x * 4.0f, -22.0f, windDir.z * 4.0f);
            particles.SpawnParticle(rPos, rVel, glm::vec4(0.60f, 0.70f, 0.85f, 0.65f), 0.08f, 0.75f, 0.0f);
        }

        // Lightning strike trigger
        m_lightningTimer -= deltaTime;
        if (m_lightningTimer <= 0.0f) {
            m_lightningFlash = 1.0f;
            m_lightningTimer = 7.0f + (rand() % 80 / 10.0f);
        }
    }

    // Blood Moon Embers
    if (m_state == WeatherState::BLOOD_MOON) {
        for (int i = 0; i < 4; ++i) {
            float rx = (rand() % 100 / 50.0f - 1.0f) * 16.0f;
            float rz = (rand() % 100 / 50.0f - 1.0f) * 16.0f;
            glm::vec3 ePos = playerPos + glm::vec3(rx, (rand() % 100 / 25.0f), rz);
            glm::vec3 eVel = glm::vec3(windDir.x * 1.5f, 0.8f + (rand() % 100 / 100.0f), windDir.z * 1.5f);
            particles.SpawnParticle(ePos, eVel, glm::vec4(0.85f, 0.12f, 0.08f, 0.8f), 0.12f, 1.4f, 0.0f);
        }
    }
}

glm::vec3 WeatherSystem::GetAdjustedFog(glm::vec3 baseFog) const {
    if (m_state == WeatherState::BLOOD_MOON) {
        return glm::vec3(0.38f, 0.04f, 0.04f);
    }
    if (m_state == WeatherState::STORM) {
        glm::vec3 stormFog = baseFog * 0.7f + glm::vec3(0.05f, 0.06f, 0.08f);
        if (m_lightningFlash > 0.05f) {
            stormFog += glm::vec3(0.70f, 0.75f, 0.85f) * m_lightningFlash;
        }
        return stormFog;
    }
    return baseFog;
}
