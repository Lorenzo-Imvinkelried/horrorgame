#include "WeatherSystem.h"
#include "ParticleSystem.h"
#include "ModelLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <iostream>

WeatherSystem::WeatherSystem() {
    initCelestialMeshes();
    m_lightningTimer = 8.0f + (rand() % 60 / 10.0f);
    m_weatherDuration = 240.0f;
}

WeatherSystem::~WeatherSystem() {
    if (m_sunVAO) glDeleteVertexArrays(1, &m_sunVAO);
    if (m_sunVBO) glDeleteBuffers(1, &m_sunVBO);
    if (m_moonVAO) glDeleteVertexArrays(1, &m_moonVAO);
    if (m_moonVBO) glDeleteBuffers(1, &m_moonVBO);
}

void WeatherSystem::initCelestialMeshes() {
    // -------------------------------------------------------------------------
    // 1. SOL CELESTIAL (Corona Dorada Brillante & Rayos Solares)
    // -------------------------------------------------------------------------
    std::vector<BoxDef> sunBoxes;
    // Núcleo Solar Blanco/Dorado
    sunBoxes.push_back({ glm::vec3(0.0f), glm::vec3(9.0f, 9.0f, 2.0f), glm::vec3(0.0f), glm::vec3(1.0f, 0.95f, 0.55f), "SunCore" });
    // Corona Solar Rotada a 45 grados
    sunBoxes.push_back({ glm::vec3(0.0f), glm::vec3(8.5f, 8.5f, 1.8f), glm::vec3(0.0f, 0.0f, 0.785f), glm::vec3(1.0f, 0.72f, 0.15f), "SunCorona" });
    // Rayos Solares Cruzados (Horizontal / Vertical)
    sunBoxes.push_back({ glm::vec3(0.0f), glm::vec3(14.0f, 3.5f, 1.2f), glm::vec3(0.0f), glm::vec3(1.0f, 0.55f, 0.08f), "SunRayH" });
    sunBoxes.push_back({ glm::vec3(0.0f), glm::vec3(3.5f, 14.0f, 1.2f), glm::vec3(0.0f), glm::vec3(1.0f, 0.55f, 0.08f), "SunRayV" });
    // Rayos Diagonales
    sunBoxes.push_back({ glm::vec3(0.0f), glm::vec3(12.0f, 3.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.785f), glm::vec3(1.0f, 0.45f, 0.05f), "SunRayD1" });
    sunBoxes.push_back({ glm::vec3(0.0f), glm::vec3(3.0f, 12.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.785f), glm::vec3(1.0f, 0.45f, 0.05f), "SunRayD2" });

    std::vector<float> sunVerts;
    ModelLoader::GenerateMesh(sunBoxes, sunVerts);
    m_sunVertexCount = sunVerts.size() / 11;

    glGenVertexArrays(1, &m_sunVAO);
    glGenBuffers(1, &m_sunVBO);
    glBindVertexArray(m_sunVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sunVBO);
    glBufferData(GL_ARRAY_BUFFER, sunVerts.size() * sizeof(float), sunVerts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // -------------------------------------------------------------------------
    // 2. LUNA CELESTIAL (Disco Plateado con Cráteres & Halo Místico)
    // -------------------------------------------------------------------------
    std::vector<BoxDef> moonBoxes;
    // Disco Lunar Plateado
    moonBoxes.push_back({ glm::vec3(0.0f), glm::vec3(8.0f, 8.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.92f, 0.94f, 0.98f), "MoonCore" });
    moonBoxes.push_back({ glm::vec3(0.0f), glm::vec3(7.5f, 7.5f, 1.8f), glm::vec3(0.0f, 0.0f, 0.785f), glm::vec3(0.85f, 0.88f, 0.95f), "MoonHalo" });
    // Cráteres Lunares (Mares Basálticos oscuros)
    moonBoxes.push_back({ glm::vec3(-1.8f,  1.4f, 1.05f), glm::vec3(2.4f, 2.2f, 0.3f), glm::vec3(0.0f), glm::vec3(0.58f, 0.62f, 0.72f), "Crater1" });
    moonBoxes.push_back({ glm::vec3( 1.6f, -1.2f, 1.05f), glm::vec3(2.0f, 1.8f, 0.3f), glm::vec3(0.0f), glm::vec3(0.52f, 0.58f, 0.68f), "Crater2" });
    moonBoxes.push_back({ glm::vec3( 1.2f,  1.6f, 1.05f), glm::vec3(1.5f, 1.4f, 0.3f), glm::vec3(0.0f), glm::vec3(0.55f, 0.60f, 0.70f), "Crater3" });

    std::vector<float> moonVerts;
    ModelLoader::GenerateMesh(moonBoxes, moonVerts);
    m_moonVertexCount = moonVerts.size() / 11;

    glGenVertexArrays(1, &m_moonVAO);
    glGenBuffers(1, &m_moonVBO);
    glBindVertexArray(m_moonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_moonVBO);
    glBufferData(GL_ARRAY_BUFFER, moonVerts.size() * sizeof(float), moonVerts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);
}

void WeatherSystem::pickNextWeather(bool isNight) {
    if (isNight && (m_dayCounter % 4 == 0)) {
        m_state = WeatherState::BLOOD_MOON_STORM;
        m_weatherDuration = 110.0f;
        return;
    }

    int roll = rand() % 100;
    if (roll < 45) {
        m_state = WeatherState::CLEAR_SUNNY;
        m_weatherDuration = 240.0f;
    } else if (roll < 70) {
        m_state = WeatherState::OVERCAST;
        m_weatherDuration = 180.0f;
    } else if (roll < 85) {
        m_state = WeatherState::MYSTIC_FOG;
        m_weatherDuration = 150.0f;
    } else if (roll < 94) {
        m_state = WeatherState::EMBER_WIND;
        m_weatherDuration = 140.0f;
    } else {
        // Lluvia ligera rara y de corta duración
        m_state = WeatherState::LIGHT_RAIN;
        m_weatherDuration = 55.0f;
    }
}

void WeatherSystem::Update(float deltaTime, float dayCycleTime, glm::vec3 playerPos, glm::vec3 windDir, ParticleSystem& particles) {
    float cycleNorm = fmod(dayCycleTime, 240.0f) / 240.0f;
    if (cycleNorm < 0.0f) cycleNorm += 1.0f;
    float normDayTime = cycleNorm * 240.0f;

    if (normDayTime < m_lastDayCycleTime) {
        m_dayCounter++;
        pickNextWeather(false);
    }
    m_lastDayCycleTime = normDayTime;

    m_isNight = (cycleNorm >= 0.50f && cycleNorm <= 0.88f);
    bool isNight = m_isNight;

    // Increment night counter when night passes (difficulty increases for each survived night)
    if (!isNight && m_wasNight) {
        m_nightCounter++;
        std::cout << "[WeatherSystem] Noche concluida. Dificultad incrementada a Nivel " << m_nightCounter << std::endl;
    }
    m_wasNight = isNight;

    // Weather state timer update
    m_weatherTimer += deltaTime;
    if (m_weatherTimer >= m_weatherDuration) {
        m_weatherTimer = 0.0f;
        pickNextWeather(isNight);
    }

    // Lightning Flash Decay
    if (m_lightningFlash > 0.0f) {
        m_lightningFlash -= deltaTime * 4.0f;
        if (m_lightningFlash < 0.0f) m_lightningFlash = 0.0f;
    }

    // -------------------------------------------------------------------------
    // GENERACIÓN DE PARTÍCULAS OPTIMIZADAS SEGÚN EL CLIMA
    // -------------------------------------------------------------------------
    if (m_state == WeatherState::LIGHT_RAIN) {
        // Lluvia suave optimizada (6 partículas por tick en vez de 18 pesadas)
        for (int i = 0; i < 6; ++i) {
            float rx = (rand() % 100 / 50.0f - 1.0f) * 20.0f;
            float rz = (rand() % 100 / 50.0f - 1.0f) * 20.0f;
            glm::vec3 rPos = playerPos + glm::vec3(rx, 12.0f + (rand() % 100 / 25.0f), rz);
            glm::vec3 rVel = glm::vec3(windDir.x * 3.0f, -18.0f, windDir.z * 3.0f);
            particles.SpawnParticle(rPos, rVel, glm::vec4(0.65f, 0.75f, 0.90f, 0.55f), 0.07f, 0.65f, 0.0f);
        }

        m_lightningTimer -= deltaTime;
        if (m_lightningTimer <= 0.0f) {
            m_lightningFlash = 0.8f;
            m_lightningTimer = 14.0f + (rand() % 100 / 10.0f);
        }
    } else if (m_state == WeatherState::MYSTIC_FOG) {
        // Esporas de polen bioluminiscentes flotantes (2 partículas por tick)
        for (int i = 0; i < 2; ++i) {
            float rx = (rand() % 100 / 50.0f - 1.0f) * 16.0f;
            float rz = (rand() % 100 / 50.0f - 1.0f) * 16.0f;
            glm::vec3 pPos = playerPos + glm::vec3(rx, 0.4f + (rand() % 100 / 30.0f), rz);
            glm::vec3 pVel = glm::vec3(windDir.x * 0.8f, 0.2f + (rand() % 100 / 200.0f), windDir.z * 0.8f);
            glm::vec4 pCol = (i % 2 == 0) ? glm::vec4(0.20f, 0.95f, 0.80f, 0.70f) : glm::vec4(0.75f, 0.35f, 0.95f, 0.70f);
            particles.SpawnParticle(pPos, pVel, pCol, 0.10f, 1.8f, 0.0f);
        }
    } else if (m_state == WeatherState::EMBER_WIND) {
        // Brizas con chispas cálidas / luciérnagas (2 partículas por tick)
        for (int i = 0; i < 2; ++i) {
            float rx = (rand() % 100 / 50.0f - 1.0f) * 18.0f;
            float rz = (rand() % 100 / 50.0f - 1.0f) * 18.0f;
            glm::vec3 ePos = playerPos + glm::vec3(rx, 0.5f + (rand() % 100 / 25.0f), rz);
            glm::vec3 eVel = glm::vec3(windDir.x * 1.8f, 0.4f + (rand() % 100 / 150.0f), windDir.z * 1.8f);
            particles.SpawnParticle(ePos, eVel, glm::vec4(0.98f, 0.75f, 0.15f, 0.80f), 0.11f, 1.5f, 0.0f);
        }
    } else if (m_state == WeatherState::BLOOD_MOON_STORM) {
        // Cenizas carmesí incandescentes de la luna de sangre
        for (int i = 0; i < 3; ++i) {
            float rx = (rand() % 100 / 50.0f - 1.0f) * 18.0f;
            float rz = (rand() % 100 / 50.0f - 1.0f) * 18.0f;
            glm::vec3 ePos = playerPos + glm::vec3(rx, 0.8f + (rand() % 100 / 20.0f), rz);
            glm::vec3 eVel = glm::vec3(windDir.x * 2.2f, 0.6f + (rand() % 100 / 100.0f), windDir.z * 2.2f);
            particles.SpawnParticle(ePos, eVel, glm::vec4(0.95f, 0.15f, 0.10f, 0.90f), 0.12f, 1.4f, 0.0f);
        }

        m_lightningTimer -= deltaTime;
        if (m_lightningTimer <= 0.0f) {
            m_lightningFlash = 1.0f;
            m_lightningTimer = 9.0f + (rand() % 80 / 10.0f);
        }
    }
}

void WeatherSystem::SetStateFromString(const std::string& name) {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "CLEAR_SUNNY" || upper == "CLEAR" || upper == "SUNNY" || upper == "DESPEJADO") {
        m_state = WeatherState::CLEAR_SUNNY;
    } else if (upper == "OVERCAST" || upper == "NUBLADO") {
        m_state = WeatherState::OVERCAST;
    } else if (upper == "MYSTIC_FOG" || upper == "FOG" || upper == "NIEBLA" || upper == "NIEBLA_MISTICA") {
        m_state = WeatherState::MYSTIC_FOG;
    } else if (upper == "LIGHT_RAIN" || upper == "RAIN" || upper == "LLUVIA" || upper == "LLUVIA_SUAVE" || upper == "STORM") {
        m_state = WeatherState::LIGHT_RAIN;
    } else if (upper == "EMBER_WIND" || upper == "EMBER" || upper == "CENIZAS" || upper == "BRIZA_DE_CENIZAS") {
        m_state = WeatherState::EMBER_WIND;
    } else if (upper == "BLOOD_MOON_STORM" || upper == "BLOOD_MOON" || upper == "LUNA_DE_SANGRE") {
        m_state = WeatherState::BLOOD_MOON_STORM;
    }
    m_weatherTimer = 0.0f;
}

void WeatherSystem::RenderCelestialBodies(GLuint shaderProgram, glm::vec3 cameraPos, float dayCycleTime, float globalTime) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1); // Emisivo brillante sin sombra
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3); // Passthrough directo de color de vértices (evita que la niebla lo tape a la distancia)

    float cycleNorm = fmod(dayCycleTime, 240.0f) / 240.0f;
    if (cycleNorm < 0.0f) cycleNorm += 1.0f;

    // Órbita celestial continua y suave de 360° en 240.0 segundos
    // 0.00 = Amanecer en el Este
    // 0.25 = Mediodía / Cenit Solar
    // 0.50 = Atardecer en el Oeste
    // 0.75 = Medianoche / Cenit Lunar
    float sunAngle = cycleNorm * 6.2831853f;

    // -------------------------------------------------------------------------
    // 1. RENDERIZADO DEL SOL (Visible mientras esté sobre o próximo al horizonte)
    // -------------------------------------------------------------------------
    glm::vec3 sunDir = glm::normalize(glm::vec3(-cosf(sunAngle) * 0.90f, sinf(sunAngle), 0.35f));

    if (sunDir.y > -0.20f && m_sunVAO != 0) {
        glm::vec3 sunPos = cameraPos + sunDir * 135.0f;
        glm::vec3 toCam = glm::normalize(cameraPos - sunPos);
        float yaw = atan2f(toCam.x, toCam.z);
        float pitch = -asinf(std::clamp(toCam.y, -0.999f, 0.999f));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, sunPos);
        model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, globalTime * 0.15f, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotación suave de rayos solares
        model = glm::scale(model, glm::vec3(2.6f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(m_sunVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_sunVertexCount));
    }

    // -------------------------------------------------------------------------
    // 2. RENDERIZADO DE LA LUNA (Ubicada exactamente opuesta a 180° / PI del Sol)
    // -------------------------------------------------------------------------
    float moonAngle = sunAngle + 3.14159265f;
    glm::vec3 moonDir = glm::normalize(glm::vec3(-cosf(moonAngle) * 0.90f, sinf(moonAngle), -0.35f));

    if (moonDir.y > -0.20f && m_moonVAO != 0) {
        glm::vec3 moonPos = cameraPos + moonDir * 135.0f;
        glm::vec3 toCam = glm::normalize(cameraPos - moonPos);
        float yaw = atan2f(toCam.x, toCam.z);
        float pitch = -asinf(std::clamp(toCam.y, -0.999f, 0.999f));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, moonPos);
        model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(IsBloodMoon() ? 3.2f : 2.5f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(m_moonVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_moonVertexCount));
    }

    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glBindVertexArray(0);
}

glm::vec3 WeatherSystem::GetAdjustedFog(glm::vec3 baseFog) const {
    if (m_state == WeatherState::BLOOD_MOON_STORM) {
        glm::vec3 bloodFog = glm::vec3(0.42f, 0.05f, 0.05f);
        if (m_lightningFlash > 0.05f) {
            bloodFog += glm::vec3(0.85f, 0.35f, 0.35f) * m_lightningFlash;
        }
        return bloodFog;
    }
    if (m_state == WeatherState::LIGHT_RAIN) {
        glm::vec3 stormFog = baseFog * 0.78f + glm::vec3(0.08f, 0.10f, 0.14f);
        if (m_lightningFlash > 0.05f) {
            stormFog += glm::vec3(0.70f, 0.75f, 0.85f) * m_lightningFlash;
        }
        return stormFog;
    }
    if (m_state == WeatherState::MYSTIC_FOG) {
        return baseFog * 0.85f + glm::vec3(0.12f, 0.28f, 0.32f);
    }
    if (m_state == WeatherState::EMBER_WIND) {
        return baseFog * 0.90f + glm::vec3(0.18f, 0.12f, 0.04f);
    }
    if (m_state == WeatherState::OVERCAST) {
        return baseFog * 0.82f + glm::vec3(0.08f, 0.08f, 0.10f);
    }
    return baseFog;
}

const char* WeatherSystem::GetWeatherName() const {
    switch (m_state) {
        case WeatherState::CLEAR_SUNNY: return "DESPEJADO";
        case WeatherState::OVERCAST: return "NUBLADO";
        case WeatherState::MYSTIC_FOG: return "NIEBLA MISTICA";
        case WeatherState::LIGHT_RAIN: return "LLUVIA SUAVE";
        case WeatherState::EMBER_WIND: return "BRIZA DE CENIZAS";
        case WeatherState::BLOOD_MOON_STORM: return "LUNA DE SANGRE";
        default: return "DESPEJADO";
    }
}
