#pragma once
#include <glad/glad.h>
#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>
#else
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#endif

#include "Player.h"
#include "ChunkManager.h"
#include "world/BuildingSystem.h"
#include "world/StructureSystem.h"
#include "world/ItemDropSystem.h"
#include "world/SkinningSystem.h"
#include "world/WeatherSystem.h"
#include "world/HorrorPropsSystem.h"
#include "combat/SpellSystem.h"
#include "combat/TargetingSystem.h"
#include "combat/DamageNumberSystem.h"
#include "combat/ProjectileSystem.h"
#include "entities/MobManager.h"
#include "ParticleSystem.h"
#include "ScentSystem.h"
#include "WindSystem.h"
#include "FootprintSystem.h"
#include "inventory/InventorySystem.h"
#include "RenderPipeline.h"
#include "InputManager.h"

class GameApp {
public:
    GameApp();
    ~GameApp();

    bool Init();
    void Run();
    void UpdateFrame();

    static GameApp* GetInstance() { return s_instance; }

private:
    void initWorld();
    void updateGameLogic(float deltaTime);

    static GameApp* s_instance;

#ifdef __EMSCRIPTEN__
    GLFWwindow* m_window = nullptr;
#else
    sf::Window m_window;
    sf::Clock m_clock;
    sf::Clock m_fpsClock;
#endif

    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    int m_frameCount = 0;
    int m_currentFPS = 0;

    float m_globalTime = 0.0f;
    float m_dayCycleTime = 25.0f;

    // Subsystems
    Player m_player;
    ChunkManager m_chunkManager;
    BuildingSystem m_buildingSystem;
    StructureSystem m_structureSystem;
    ItemDropSystem m_itemDropSystem;
    SkinningSystem m_skinningSystem;
    WeatherSystem m_weatherSystem;
    HorrorPropsSystem m_horrorProps;
    SpellSystem m_spellSystem;
    TargetingSystem m_targeting;
    DamageNumberSystem m_damageNumbers;
    ProjectileSystem m_projectiles;
    MobManager m_mobManager;
    ParticleSystem m_particles;
    ScentSystem m_scentSystem;
    WindSystem m_windSystem;
    FootprintSystem m_footprints;
    InventorySystem m_inventory;

    RenderPipeline m_renderPipeline;
    InputManager m_inputManager;

    // Terraforming / Building helpers
    glm::vec3 m_terraTarget = glm::vec3(0.0f);
    bool m_hasTerraTarget = false;
    glm::vec3 m_buildPos = glm::vec3(0.0f);
};
