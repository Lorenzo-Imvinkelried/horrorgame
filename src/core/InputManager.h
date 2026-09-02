#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include "world/BuildingSystem.h"
#include "ui/UIRenderer.h"

class Player;
class InventorySystem;
class StructureSystem;
class ItemDropSystem;
class SkinningSystem;
class SpellSystem;
class MobManager;
class HorrorPropsSystem;
class DamageNumberSystem;
class ParticleSystem;
class ScentSystem;
class TargetingSystem;
class ProjectileSystem;
class ChunkManager;

class InputManager {
public:
    InputManager();
    ~InputManager();

    void HandleKeyPress(int key, Player& player, InventorySystem& inventory,
                        BuildingSystem& buildingSystem, StructureSystem& structureSystem,
                        ItemDropSystem& itemDropSystem, SkinningSystem& skinningSystem,
                        SpellSystem& spellSystem, MobManager& mobManager,
                        HorrorPropsSystem& horrorProps, DamageNumberSystem& damageNumbers,
                        ParticleSystem& particles, ScentSystem& scentSystem,
                        TargetingSystem& targeting, ProjectileSystem& projectiles);

    void UpdateMouseAndLook(float deltaTime, float curMouseX, float curMouseY,
                           bool leftIsPressed, bool rightIsPressed,
                           int screenW, int screenH, bool hasFocus,
                           Player& player, InventorySystem& inventory,
                           BuildingSystem& buildingSystem, StructureSystem& structureSystem,
                           ItemDropSystem& itemDropSystem, TargetingSystem& targeting,
                           ProjectileSystem& projectiles, MobManager& mobManager,
                           HorrorPropsSystem& horrorProps, DamageNumberSystem& damageNumbers,
                           ParticleSystem& particles, ChunkManager& chunkManager,
                           float dayCycleTime, const glm::vec3& terraTarget,
                           bool hasTerraTarget, const glm::vec3& buildPos);

    // States & Modals
    bool IsGamePaused() const { return m_isGamePaused; }
    void SetGamePaused(bool p) { m_isGamePaused = p; }
    void ToggleGamePaused() { m_isGamePaused = !m_isGamePaused; }

    bool IsCharacterPanelOpen() const { return m_isCharacterPanelOpen; }
    void SetCharacterPanelOpen(bool o) { m_isCharacterPanelOpen = o; }

    bool IsBuildMode() const { return m_isBuildMode; }
    BuildingType GetCurrentBuildType() const { return m_currentBuildType; }
    float GetCurrentBuildYaw() const { return m_currentBuildYaw; }

    bool IsShovelMode() const { return m_isShovelMode; }
    bool ShowHitboxes() const { return m_showHitboxes; }
    bool ShowSpawnArea() const { return m_showSpawnArea; }
    bool ShowMonsterMarker() const { return m_showMonsterMarker; }

    bool IsDebugCam() const { return m_debugCam; }
    glm::vec3 GetFreeCamPos() const { return m_freeCamPos; }
    glm::vec3 GetFreeCamFront() const { return m_freeCamFront; }
    void MoveFreeCam(glm::vec3 delta) { m_freeCamPos += delta; }

    float GetMouseNdcX() const { return m_mouseNdcX; }
    float GetMouseNdcY() const { return m_mouseNdcY; }
    void ResetMouse() { m_firstMouse = true; }

    FatalErrorPopup& GetFatalError() { return m_fatalError; }
    LoreDocumentModal& GetLoreModal() { return m_loreModal; }

private:
    bool m_isGamePaused = false;
    bool m_isCharacterPanelOpen = false;
    bool m_isBuildMode = false;
    BuildingType m_currentBuildType = BuildingType::WALL;
    float m_currentBuildYaw = 0.0f;
    bool m_isShovelMode = false;

    bool m_showHitboxes = false;
    bool m_showSpawnArea = false;
    bool m_showMonsterMarker = false;

    bool m_debugCam = false;
    glm::vec3 m_freeCamPos = glm::vec3(0, 50, 0);
    glm::vec3 m_freeCamFront = glm::vec3(0, -1, 0);
    float m_freeCamYaw = -90.0f;
    float m_freeCamPitch = -45.0f;

    float m_mouseNdcX = 0.0f;
    float m_mouseNdcY = 0.0f;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
    bool m_firstMouse = true;

    bool m_leftWasPressed = false;

    FatalErrorPopup m_fatalError;
    LoreDocumentModal m_loreModal;
};
