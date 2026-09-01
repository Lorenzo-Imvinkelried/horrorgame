#include "RenderPipeline.h"
#include "ShaderLoader.h"
#include "render/DebugDraw.h"
#include "Player.h"
#include "ChunkManager.h"
#include "world/BuildingSystem.h"
#include "world/StructureSystem.h"
#include "world/ItemDropSystem.h"
#include "world/HorrorPropsSystem.h"
#include "world/WeatherSystem.h"
#include "combat/ProjectileSystem.h"
#include "combat/TargetingSystem.h"
#include "combat/DamageNumberSystem.h"
#include "entities/MobManager.h"
#include "ParticleSystem.h"
#include "inventory/InventorySystem.h"
#include "InputManager.h"
#include "WorldGenerator.h"
#include "Config.h"
#include "ui/UIRenderer.h"
#include "ui/FontRenderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>

RenderPipeline::RenderPipeline() {}
RenderPipeline::~RenderPipeline() {
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_colorTex) glDeleteTextures(1, &m_colorTex);
    if (m_depthRbo) glDeleteRenderbuffers(1, &m_depthRbo);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_postProgram) glDeleteProgram(m_postProgram);
    if (m_uiProgram) glDeleteProgram(m_uiProgram);
}

bool RenderPipeline::Init(int internalW, int internalH) {
    m_internalW = internalW;
    m_internalH = internalH;

    // 1. Shaders
    m_shaderProgram = ShaderLoader::Load("assets/shaders/ps1.vert", "assets/shaders/ps1.frag");
    m_postProgram = ShaderLoader::Load("assets/shaders/screen.vert", "assets/shaders/screen.frag");
    m_uiProgram = ShaderLoader::Load("assets/shaders/ui.vert", "assets/shaders/ui.frag");

    // 2. FBO Setup
    ResizeFBO(m_internalW, m_internalH);

    // 3. Screen Quad for Post Process
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // 4. UI VAO & VBO
    glGenVertexArrays(1, &m_uiVAO);
    glGenBuffers(1, &m_uiVBO);
    glBindVertexArray(m_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // 5. Debug VAO & VBO
    glGenVertexArrays(1, &m_debugVAO);
    glGenBuffers(1, &m_debugVBO);

    // 6. Textures & Meshes
    initTextures();
    initMeshes();

    FontRenderer::Init("assets/fonts/Symtext.ttf");
    return true;
}

void RenderPipeline::ResizeFBO(int internalW, int internalH) {
    m_internalW = internalW;
    m_internalH = internalH;

    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_colorTex) glDeleteTextures(1, &m_colorTex);
    if (m_depthRbo) glDeleteRenderbuffers(1, &m_depthRbo);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_internalW, m_internalH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    glGenRenderbuffers(1, &m_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_internalW, m_internalH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[RenderPipeline] ERROR: Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPipeline::initTextures() {
    // 1x1 White Texture
    glGenTextures(1, &m_whiteTexID);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexID);
    unsigned char whitePixel[3] = { 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Procedural Detail Noise Texture (Original PS1 Grain)
    std::vector<unsigned char> textureData = WorldGenerator::GenerateNoiseTexture(64, 64);
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

void RenderPipeline::initMeshes() {
    // Tree Trunks & Leaves (4 Archetypes)
    for (int arch = 0; arch < 4; ++arch) {
        auto tMesh = WorldGenerator::GetTreeTrunkMesh(arch);
        m_trunkVertexCount[arch] = (int)tMesh.size();
        glGenVertexArrays(1, &m_trunkVAO[arch]);
        glGenBuffers(1, &m_trunkVBO[arch]);
        glBindVertexArray(m_trunkVAO[arch]);
        glBindBuffer(GL_ARRAY_BUFFER, m_trunkVBO[arch]);
        glBufferData(GL_ARRAY_BUFFER, tMesh.size() * sizeof(Vertex), tMesh.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);

        auto lMesh = WorldGenerator::GetTreeLeavesMesh(arch);
        m_leavesVertexCount[arch] = (int)lMesh.size();
        glGenVertexArrays(1, &m_leavesVAO[arch]);
        glGenBuffers(1, &m_leavesVBO[arch]);
        glBindVertexArray(m_leavesVAO[arch]);
        glBindBuffer(GL_ARRAY_BUFFER, m_leavesVBO[arch]);
        glBufferData(GL_ARRAY_BUFFER, lMesh.size() * sizeof(Vertex), lMesh.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);
    }
}

void RenderPipeline::RenderScene3D(float deltaTime, float globalTime, float dayCycleTime,
                                   Player& player, ChunkManager& chunkManager,
                                   BuildingSystem& buildingSystem, StructureSystem& structureSystem,
                                   ItemDropSystem& itemDropSystem, HorrorPropsSystem& horrorProps,
                                   ProjectileSystem& projectiles, TargetingSystem& targeting,
                                   MobManager& mobManager, WeatherSystem& weatherSystem,
                                   DamageNumberSystem& damageNumbers, ParticleSystem& particles,
                                   FootprintSystem& footprints,
                                   InputManager& inputMgr, const glm::vec2& windDir,
                                   const glm::vec3& terraTarget, bool hasTerraTarget,
                                   const glm::vec3& buildPos)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_internalW, m_internalH);

    // RESTORE 3D PIPELINE STATES
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    const float dayCycleLength = 240.0f;
    float cycleNormalized = fmod(dayCycleTime, dayCycleLength) / dayCycleLength;
    float nightFactor = 0.0f;
    glm::vec3 fogCol(0.40f, 0.60f, 0.95f);

    if (cycleNormalized < 0.45f) {
        nightFactor = 0.0f;
        fogCol = glm::vec3(0.40f, 0.60f, 0.95f);
    } else if (cycleNormalized < 0.58f) {
        float t = (cycleNormalized - 0.45f) / 0.13f;
        nightFactor = t;
        glm::vec3 sunsetCol(0.85f, 0.32f, 0.14f);
        glm::vec3 nightCol(0.005f, 0.005f, 0.015f);
        fogCol = (t < 0.5f) ? glm::mix(glm::vec3(0.40f, 0.60f, 0.95f), sunsetCol, t * 2.0f) : glm::mix(sunsetCol, nightCol, (t - 0.5f) * 2.0f);
    } else if (cycleNormalized < 0.88f) {
        nightFactor = 1.0f;
        fogCol = glm::vec3(0.005f, 0.005f, 0.015f);
    } else {
        float t = (cycleNormalized - 0.88f) / 0.12f;
        nightFactor = 1.0f - t;
        glm::vec3 sunriseCol(0.90f, 0.45f, 0.25f);
        glm::vec3 dayCol(0.40f, 0.60f, 0.95f);
        fogCol = (t < 0.5f) ? glm::mix(glm::vec3(0.005f, 0.005f, 0.015f), sunriseCol, t * 2.0f) : glm::mix(sunriseCol, dayCol, (t - 0.5f) * 2.0f);
    }

    fogCol = weatherSystem.GetAdjustedFog(fogCol);
    bool isNightTime = (nightFactor > 0.45f) || weatherSystem.IsBloodMoon();

    glClearColor(fogCol.r, fogCol.g, fogCol.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shaderProgram);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_Time"), globalTime);
    glUniform2f(glGetUniformLocation(m_shaderProgram, "u_Resolution"), (float)m_internalW, (float)m_internalH);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_Snap"), 1);

    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_FogStart"), Config::World::FogDistStart);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_FogEnd"), Config::World::FogDistEnd);

    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_IsNight"), isNightTime ? 1 : 0);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_Darkness"), nightFactor * 0.95f);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_TorchActive"), player.HasTorchActive ? 1 : 0);
    glm::vec3 playerTorchPos = player.GetTorchPosition();
    glUniform3f(glGetUniformLocation(m_shaderProgram, "u_TorchPos"), playerTorchPos.x, playerTorchPos.y, playerTorchPos.z);

    std::vector<glm::vec4> worldTorches = buildingSystem.GetClosestTorches(player.Position, 8);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_NumWorldTorches"), (int)worldTorches.size());
    if (!worldTorches.empty()) {
        glUniform4fv(glGetUniformLocation(m_shaderProgram, "u_WorldTorches"), (GLsizei)worldTorches.size(), glm::value_ptr(worldTorches[0]));
    }

    glUniform3f(glGetUniformLocation(m_shaderProgram, "u_PlayerPos"), player.Position.x, player.Position.y, player.Position.z);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "u_PlayerFront"), player.Front.x, player.Front.y, player.Front.z);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "u_FogColor"), fogCol.r, fogCol.g, fogCol.b);

    glm::mat4 view = inputMgr.IsDebugCam() ? glm::lookAt(inputMgr.GetFreeCamPos(), inputMgr.GetFreeCamPos() + inputMgr.GetFreeCamFront(), glm::vec3(0,1,0)) : player.GetViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)m_internalW / (float)m_internalH, 0.1f, 1000.0f);

    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    // FRUSTUM CULLING UPDATE
    chunkManager.UpdateVisibility(proj * view);

    glUniform2f(glGetUniformLocation(m_shaderProgram, "u_WindDirection"), windDir.x, windDir.y);

    // 1. Terrain Render
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_Alpha"), 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_Texture"), 0);
    chunkManager.RenderTerrain(m_shaderProgram);

    // 2. Tree Render
    chunkManager.RenderTrees(m_shaderProgram, m_trunkVAO, m_leavesVAO, m_trunkVertexCount, m_leavesVertexCount, player.Position);

    // 3. Footprints
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_WindStrength"), 0.0f);
    footprints.Render(m_shaderProgram);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_ConformToTerrain"), 0);

    glm::vec3 activeCamPos = inputMgr.IsDebugCam() ? inputMgr.GetFreeCamPos() : player.GetCameraPosition();
    particles.Render(m_shaderProgram, activeCamPos);

    // 4. Player (3rd Person)
    if (player.IsThirdPerson) {
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        player.Render(m_shaderProgram);
    }

    // 5. Mobs, Birds, Critters, Dragon
    mobManager.Render(m_shaderProgram, activeCamPos, m_textureID, globalTime);

    // 6. Celestial Bodies (Sun / Moon)
    weatherSystem.RenderCelestialBodies(m_shaderProgram, activeCamPos, dayCycleTime, globalTime);

    // 7. Projectiles
    projectiles.Render(m_shaderProgram);

    // 8. 3D Target Ring
    targeting.RenderTargetRing(m_shaderProgram);

    // 9. Environmental Horror Props
    std::vector<glm::vec4> nearbyTreesForProps;
    chunkManager.GetTreesInRange(player.Position, 85.0f, nearbyTreesForProps);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    horrorProps.Render(m_shaderProgram, nearbyTreesForProps, globalTime, windDir);

    // 10. Procedural World Structures & Items & Placed Buildings
    structureSystem.Render(m_shaderProgram, activeCamPos);
    itemDropSystem.Render(m_shaderProgram, activeCamPos);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    buildingSystem.Render(m_shaderProgram, activeCamPos);

    // 11. Build Ghost Preview
    if (inputMgr.IsBuildMode()) {
        buildingSystem.RenderGhost(m_shaderProgram, inputMgr.GetCurrentBuildType(), buildPos, inputMgr.GetCurrentBuildYaw(), true, m_whiteTexID);
    }

    // --- SAFETY RESET ---
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_IsInstanced"), 0);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "u_UseBirdAttribs"), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (int i = 0; i < 8; i++) glDisableVertexAttribArray(i);

    // 12. Water (Transparent - Rendered after solid objects)
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    chunkManager.RenderWater(m_shaderProgram);

    // 13. Weapon (Overlay - Clear Depth - 1st Person Only)
    if (!player.IsThirdPerson) {
        glClear(GL_DEPTH_BUFFER_BIT);
        glm::mat4 viewIdentity = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(viewIdentity));
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        player.RenderFirstPersonSword(m_shaderProgram);
        player.RenderFirstPersonTorch(m_shaderProgram);
    }

    // 14. Monster Beacon Markers
    if (inputMgr.ShowMonsterMarker()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "u_IsInstanced"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "u_ConformToTerrain"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "u_Texture"), 0);
        glBindTexture(GL_TEXTURE_2D, m_whiteTexID);
        glDisable(GL_DEPTH_TEST);
        for (const auto& mPtr : mobManager.GetMonsters()) {
            glm::vec3 mPos = mPtr->GetPosition();
            glm::vec3 markerColor = mPtr->HasVisualContact() ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
            DebugDraw::DrawLine(mPos, mPos + glm::vec3(0.0f, 200.0f, 0.0f), markerColor, m_debugVAO, m_debugVBO);
            DebugDraw::DrawLine(mPos - glm::vec3(2.5f, 0.0f, 0.0f), mPos + glm::vec3(2.5f, 0.0f, 0.0f), markerColor, m_debugVAO, m_debugVBO);
            DebugDraw::DrawLine(mPos - glm::vec3(0.0f, 0.0f, 2.5f), mPos + glm::vec3(0.0f, 0.0f, 2.5f), markerColor, m_debugVAO, m_debugVBO);
        }
        glEnable(GL_DEPTH_TEST);
    }

    // 15. Shovel Reticle (Donut)
    if (inputMgr.IsShovelMode() && hasTerraTarget) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "u_IsInstanced"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "u_ConformToTerrain"), 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "u_Texture"), 0);
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glBindTexture(GL_TEXTURE_2D, m_whiteTexID);
        DebugDraw::DrawDonut(terraTarget.x, terraTarget.y + 0.08f, terraTarget.z, 3.3f, 3.6f, glm::vec3(0.35f, 0.90f, 0.40f), m_debugVAO, m_debugVBO);
    }
}

void RenderPipeline::RenderPostProcess(int screenW, int screenH) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_postProgram);
    glBindVertexArray(m_quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glUniform1i(glGetUniformLocation(m_postProgram, "u_ScreenTexture"), 0);
    glUniform1i(glGetUniformLocation(m_postProgram, "u_IsGameOver"), 0);
    glUniform1f(glGetUniformLocation(m_postProgram, "u_GameOverTime"), 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void RenderPipeline::RenderUI2D(Player& player, InventorySystem& inventory,
                                TargetingSystem& targeting, DamageNumberSystem& damageNumbers,
                                WeatherSystem& weatherSystem, MobManager& mobManager,
                                InputManager& inputMgr, float globalTime, int currentFPS)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_uiProgram);

    // 1. Crosshair in 1st Person
    if (!player.IsThirdPerson) {
        UIRenderer::DrawCrosshair(m_uiProgram, m_uiVAO, m_uiVBO, 0.012f, 0.003f, glm::vec3(0.9f, 0.9f, 0.9f));
    }

    // 2. HUD (Vital Signs ECG Monitor, Exp Bar, Target Frame, Damage Floaters)
    glm::mat4 viewProj = player.GetViewMatrix();
    m_uiRenderer.RenderHUD(m_uiProgram, m_uiVAO, m_uiVBO, player.Stats, targeting,
                          damageNumbers, viewProj, mobManager.GetHighestDangerLevel(),
                          globalTime, weatherSystem.GetNightCount(), weatherSystem.IsBloodMoon());

    // 3. Quickbar Action Slots (1-4)
    m_uiRenderer.RenderQuickbarHUD(m_uiProgram, m_uiVAO, m_uiVBO, inventory.GetInventory());

    // 4. Drag & Drop Inventory Window
    if (inventory.IsOpen()) {
        inventory.RenderWindow(m_uiProgram, m_uiVAO, m_uiVBO, inputMgr.GetMouseNdcX(), inputMgr.GetMouseNdcY(), &player.Stats);
    }

    // 5. Character Stats Allocation Panel [C]
    if (inputMgr.IsCharacterPanelOpen()) {
        m_uiRenderer.RenderCharacterPanel(m_uiProgram, m_uiVAO, m_uiVBO, player.Stats, inputMgr.GetMouseNdcX(), inputMgr.GetMouseNdcY());
    }

    // 6. Lore Document Modal
    if (inputMgr.GetLoreModal().active) {
        m_uiRenderer.RenderLoreModal(m_uiProgram, m_uiVAO, m_uiVBO, inputMgr.GetLoreModal(),
                                    inputMgr.GetMouseNdcX(), inputMgr.GetMouseNdcY());
    }

    // 7. Fatal Error Popup
    if (inputMgr.GetFatalError().active) {
        m_uiRenderer.RenderFatalErrorModal(m_uiProgram, m_uiVAO, m_uiVBO, inputMgr.GetFatalError(),
                                          inputMgr.GetMouseNdcX(), inputMgr.GetMouseNdcY());
    }

    // 8. Pause Menu
    if (inputMgr.IsGamePaused()) {
        m_uiRenderer.RenderPauseMenu(m_uiProgram, m_uiVAO, m_uiVBO, inputMgr.GetMouseNdcX(), inputMgr.GetMouseNdcY());
    }

    // 9. Retro Cursor
    UIRenderer::RenderCursor(m_uiProgram, m_uiVAO, m_uiVBO, inputMgr.GetMouseNdcX(), inputMgr.GetMouseNdcY());

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
