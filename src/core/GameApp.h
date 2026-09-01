#pragma once
#include <glad/glad.h>
#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>
#else
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#endif

#include <memory>
#include <glm/glm.hpp>

class Player;
class ChunkManager;
class BuildingSystem;
class StructureSystem;
class ItemDropSystem;
class SkinningSystem;
class WeatherSystem;
class HorrorPropsSystem;
class SpellSystem;
class TargetingSystem;
class DamageNumberSystem;
class ProjectileSystem;
class MobManager;
class ParticleSystem;
class ScentSystem;
class WindSystem;
class FootprintSystem;
class InventorySystem;
class RenderPipeline;
class InputManager;

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
    void loadConfigFile();
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

    // Subsystems (Allocated after OpenGL context creation)
    std::unique_ptr<Player> m_player;
    std::unique_ptr<ChunkManager> m_chunkManager;
    std::unique_ptr<BuildingSystem> m_buildingSystem;
    std::unique_ptr<StructureSystem> m_structureSystem;
    std::unique_ptr<ItemDropSystem> m_itemDropSystem;
    std::unique_ptr<SkinningSystem> m_skinningSystem;
    std::unique_ptr<WeatherSystem> m_weatherSystem;
    std::unique_ptr<HorrorPropsSystem> m_horrorProps;
    std::unique_ptr<SpellSystem> m_spellSystem;
    std::unique_ptr<TargetingSystem> m_targeting;
    std::unique_ptr<DamageNumberSystem> m_damageNumbers;
    std::unique_ptr<ProjectileSystem> m_projectiles;
    std::unique_ptr<MobManager> m_mobManager;
    std::unique_ptr<ParticleSystem> m_particles;
    std::unique_ptr<ScentSystem> m_scentSystem;
    std::unique_ptr<WindSystem> m_windSystem;
    std::unique_ptr<FootprintSystem> m_footprints;
    std::unique_ptr<InventorySystem> m_inventory;

    std::unique_ptr<RenderPipeline> m_renderPipeline;
    std::unique_ptr<InputManager> m_inputManager;

    // Terraforming / Building helpers
    glm::vec3 m_terraTarget = glm::vec3(0.0f);
    bool m_hasTerraTarget = false;
    glm::vec3 m_buildPos = glm::vec3(0.0f);
};
