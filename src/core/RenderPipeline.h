#pragma once
#include <glad/glad.h>
#include "ui/UIRenderer.h"

class Player;
class ChunkManager;
class BuildingSystem;
class StructureSystem;
class ItemDropSystem;
class HorrorPropsSystem;
class ProjectileSystem;
class TargetingSystem;
class MobManager;
class WeatherSystem;
class DamageNumberSystem;
class ParticleSystem;
class FootprintSystem;
class InventorySystem;
class InputManager;
class SpellSystem;
class SkinningSystem;

class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    bool Init(int internalW, int internalH);
    void ResizeFBO(int internalW, int internalH);

    void RenderScene3D(float deltaTime, float globalTime, float dayCycleTime,
                       Player& player, ChunkManager& chunkManager,
                       BuildingSystem& buildingSystem, StructureSystem& structureSystem,
                       ItemDropSystem& itemDropSystem, HorrorPropsSystem& horrorProps,
                       ProjectileSystem& projectiles, TargetingSystem& targeting,
                       MobManager& mobManager, WeatherSystem& weatherSystem,
                       DamageNumberSystem& damageNumbers, ParticleSystem& particles,
                       FootprintSystem& footprints,
                       InputManager& inputMgr, const glm::vec2& windDir,
                       const glm::vec3& terraTarget, bool hasTerraTarget,
                       const glm::vec3& buildPos);

    void RenderPostProcess(int screenW, int screenH);

    void RenderUI2D(Player& player, InventorySystem& inventory,
                    TargetingSystem& targeting, DamageNumberSystem& damageNumbers,
                    WeatherSystem& weatherSystem, MobManager& mobManager,
                    InputManager& inputMgr, float globalTime, int currentFPS,
                    SpellSystem& spellSystem, StructureSystem& structureSystem,
                    ItemDropSystem& itemDropSystem, SkinningSystem& skinningSystem,
                    HorrorPropsSystem& horrorProps);

    GLuint GetMainShaderProgram() const { return m_shaderProgram; }
    GLuint GetUIShaderProgram() const { return m_uiProgram; }
    GLuint GetWhiteTexID() const { return m_whiteTexID; }
    GLuint GetMainTextureID() const { return m_textureID; }
    GLuint GetUIVAO() const { return m_uiVAO; }
    GLuint GetUIVBO() const { return m_uiVBO; }
    int GetInternalW() const { return m_internalW; }
    int GetInternalH() const { return m_internalH; }
    int GetWindowW() const { return m_windowW; }
    int GetWindowH() const { return m_windowH; }

private:
    void initTextures();
    void initMeshes();

    int m_internalW = 640;
    int m_internalH = 480;
    int m_windowW = 1280;
    int m_windowH = 720;

    GLuint m_fbo = 0;
    GLuint m_colorTex = 0;
    GLuint m_depthRbo = 0;

    GLuint m_shaderProgram = 0;
    GLuint m_postProgram = 0;
    GLuint m_uiProgram = 0;

    GLuint m_whiteTexID = 0;
    GLuint m_textureID = 0;

    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    GLuint m_uiVAO = 0;
    GLuint m_uiVBO = 0;

    GLuint m_debugVAO = 0;
    GLuint m_debugVBO = 0;

    UIRenderer m_uiRenderer;

    // Tree Meshes (4 Archetypes)
    GLuint m_trunkVAO[4] = {0};
    GLuint m_trunkVBO[4] = {0};
    int m_trunkVertexCount[4] = {0};

    GLuint m_leavesVAO[4] = {0};
    GLuint m_leavesVBO[4] = {0};
    int m_leavesVertexCount[4] = {0};

    GLuint m_shadowVAO = 0;
    GLuint m_shadowVBO = 0;
    GLuint m_instanceVBO = 0;
};
