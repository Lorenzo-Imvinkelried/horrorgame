#include "GameApp.h"
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
#include "inventory/LootManager.h"
#include "RenderPipeline.h"
#include "InputManager.h"
#include "WorldGenerator.h"
#include "Config.h"
#include "core/PlatformInput.h"

#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

GameApp* GameApp::s_instance = nullptr;

GameApp::GameApp() {
    s_instance = this;
}

GameApp::~GameApp() {
    // Release subsystems before destroying OpenGL context
    m_mobManager.reset();
    m_player.reset();
    m_chunkManager.reset();
    m_buildingSystem.reset();
    m_structureSystem.reset();
    m_itemDropSystem.reset();
    m_skinningSystem.reset();
    m_weatherSystem.reset();
    m_horrorProps.reset();
    m_spellSystem.reset();
    m_targeting.reset();
    m_damageNumbers.reset();
    m_projectiles.reset();
    m_particles.reset();
    m_scentSystem.reset();
    m_windSystem.reset();
    m_footprints.reset();
    m_inventory.reset();
    m_renderPipeline.reset();
    m_inputManager.reset();

    s_instance = nullptr;
#ifdef __EMSCRIPTEN__
    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
#endif
}

bool GameApp::Init() {
    srand((unsigned int)time(NULL));
    WorldGenerator::SetSeed(rand());

#ifndef __EMSCRIPTEN__
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 0;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_window.create(desktop, "VRAM DUNGEON", sf::Style::None, settings);
    m_window.setVerticalSyncEnabled(Config::Graphics::VSyncEnabled);
    m_window.setMouseCursorVisible(true);
    PlatformInput::Init(&m_window);
    m_windowWidth = desktop.width;
    m_windowHeight = desktop.height;

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
#else
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    double screenW = 1280, screenH = 720;
    emscripten_get_element_css_size("#canvas", &screenW, &screenH);
    if (screenW <= 0) screenW = 1280;
    if (screenH <= 0) screenH = 720;
    m_windowWidth = (int)screenW;
    m_windowHeight = (int)screenH;

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "VRAM DUNGEON", NULL, NULL);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    PlatformInput::Init(m_window);

    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD WebGL2" << std::endl;
        return false;
    }

    glfwSetKeyCallback(m_window, [](GLFWwindow*, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS && GameApp::GetInstance()) {
            GameApp* app = GameApp::GetInstance();
            if (app->m_inputManager && app->m_player) {
                app->m_inputManager->HandleKeyPress(key, *app->m_player, *app->m_inventory,
                                                   *app->m_buildingSystem, *app->m_structureSystem,
                                                   *app->m_itemDropSystem, *app->m_skinningSystem,
                                                   *app->m_spellSystem, *app->m_mobManager,
                                                   *app->m_horrorProps, *app->m_damageNumbers,
                                                   *app->m_particles, *app->m_scentSystem);
            }
        }
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow*, double xoffset, double yoffset) {
        if (GameApp::GetInstance() && GameApp::GetInstance()->m_player) {
            GameApp::GetInstance()->m_player->ProcessMouseScroll((float)yoffset);
        }
    });

    glfwSetWindowFocusCallback(m_window, [](GLFWwindow*, int focused) {
        if (focused == GLFW_FALSE && GameApp::GetInstance() && GameApp::GetInstance()->m_inputManager) {
            GameApp::GetInstance()->m_inputManager->SetGamePaused(true);
        }
    });
#endif

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Instantiating Subsystems NOW that OpenGL context is valid
    m_renderPipeline = std::make_unique<RenderPipeline>();
    if (!m_renderPipeline->Init(Config::Graphics::InternalWidth, Config::Graphics::InternalHeight)) {
        std::cerr << "Failed to initialize RenderPipeline" << std::endl;
        return false;
    }

    m_inputManager = std::make_unique<InputManager>();
    m_chunkManager = std::make_unique<ChunkManager>(Config::World::RenderDistance);
    m_buildingSystem = std::make_unique<BuildingSystem>();
    m_structureSystem = std::make_unique<StructureSystem>();
    m_itemDropSystem = std::make_unique<ItemDropSystem>();
    m_skinningSystem = std::make_unique<SkinningSystem>();
    m_weatherSystem = std::make_unique<WeatherSystem>();
    m_horrorProps = std::make_unique<HorrorPropsSystem>();
    m_spellSystem = std::make_unique<SpellSystem>();
    m_targeting = std::make_unique<TargetingSystem>();
    m_damageNumbers = std::make_unique<DamageNumberSystem>();
    m_projectiles = std::make_unique<ProjectileSystem>();
    m_mobManager = std::make_unique<MobManager>();
    m_particles = std::make_unique<ParticleSystem>();
    m_scentSystem = std::make_unique<ScentSystem>();
    m_windSystem = std::make_unique<WindSystem>();
    m_footprints = std::make_unique<FootprintSystem>();
    m_inventory = std::make_unique<InventorySystem>();

    initWorld();
    return true;
}

void GameApp::initWorld() {
    float px = (float)(rand() % 160 - 80);
    float pz = (float)(rand() % 160 - 80);
    float py = WorldGenerator::GetHeight(px, pz) + 1.0f;
    m_player = std::make_unique<Player>(glm::vec3(px, py, pz));

    m_chunkManager->SetBirdSystem(&m_mobManager->GetBirds());
    m_chunkManager->Init();
    m_chunkManager->Update(m_player->Position);

    m_mobManager->Init(m_player->Position);
}

void GameApp::updateGameLogic(float deltaTime) {
    if (m_inputManager->IsGamePaused()) return;

    // 1. Wind & Scent Update
    m_windSystem->Update(deltaTime);
    glm::vec2 windDir = m_windSystem->GetDirection();
    m_scentSystem->Update(deltaTime, m_player->Position, glm::vec3(windDir.x, 0, windDir.y), 1.0f);

    // 2. Chunk Manager terrain streaming
    m_chunkManager->Update(m_player->Position);

    // 3. Spells & Weather
    m_spellSystem->Update(deltaTime, *m_player, *m_particles);
    m_weatherSystem->Update(deltaTime, m_dayCycleTime, m_player->Position, glm::vec3(windDir.x, 0, windDir.y), *m_particles);

    // 4. Day / Night cycle progression
    m_dayCycleTime += deltaTime;

    // 5. Buildings
    m_buildingSystem->Update(deltaTime, m_player->Position, *m_particles);
    glm::vec3 bPush(0.0f);
    m_buildingSystem->CheckCollision(m_player->Position, m_player->PlayerRadius, m_player->PlayerHeight, bPush);

    // 6. Ground Loot System
    m_itemDropSystem->Update(deltaTime, m_player->Position, *m_inventory, *m_damageNumbers, *m_particles);

    // Check Loot Drops from Dead Monsters & Bosses
    for (auto& enemy : m_mobManager->GetEnemyMobs()) {
        if (!enemy->IsAlive() && !enemy->HasDroppedLoot()) {
            enemy->SetLootDropped(true);
            LootTable table = LootManager::GetEnemyLoot(enemy->GetType(), enemy->GetNightLevel());
            std::vector<ItemInstance> drops = table.GenerateLoot(1.0f);
            m_itemDropSystem->SpawnDrops(drops, enemy->GetPosition());
        }
    }
    for (auto& mPtr : m_mobManager->GetMonsters()) {
        if (mPtr->IsDead() && !mPtr->HasDroppedLoot()) {
            mPtr->SetLootDropped(true);
            LootTable table = LootManager::GetEnemyLoot(EnemyType::CORRUPTED_WARRIOR, 1);
            std::vector<ItemInstance> drops = table.GenerateLoot(1.0f);
            m_itemDropSystem->SpawnDrops(drops, mPtr->GetPosition());
        }
    }
    if (m_mobManager->GetDragon().IsDead() && !m_mobManager->GetDragon().HasDroppedLoot()) {
        m_mobManager->GetDragon().SetLootDropped(true);
        LootTable dTable = LootManager::GetDragonLoot();
        std::vector<ItemInstance> drops = dTable.GenerateLoot(1.0f);
        m_itemDropSystem->SpawnDrops(drops, m_mobManager->GetDragon().GetPosition() + glm::vec3(0, 0.8f, 0));
    }

    // 7. Projectiles & Flying Spells
    m_projectiles->Update(deltaTime, m_player.get(), *m_particles, *m_damageNumbers,
                          &m_mobManager->GetMonsters(), &m_mobManager->GetPassiveMobs(),
                          &m_mobManager->GetEnemyMobs(), &m_mobManager->GetWaterMonsters(),
                          &m_mobManager->GetDragon());

    // 8. Mob Manager (AI, Tracking, Combat)
    m_mobManager->Update(deltaTime, *m_player, *m_chunkManager, *m_scentSystem,
                         *m_windSystem, *m_particles, *m_damageNumbers,
                         *m_itemDropSystem, *m_projectiles, m_globalTime,
                         m_dayCycleTime, m_weatherSystem->GetNightCount(),
                         m_weatherSystem->IsBloodMoon(), &m_inputManager->GetFatalError());

    // 9. Particles & Floating Numbers
    m_particles->Update(deltaTime);
    m_damageNumbers->Update(deltaTime);

    // 10. Melee Combat Check
    m_player->UpdateCombat(deltaTime, m_mobManager->GetMonsters(), m_mobManager->GetPassiveMobs(),
                           m_mobManager->GetEnemyMobs(), m_mobManager->GetWaterMonsters(),
                           *m_particles, *m_damageNumbers, &m_mobManager->GetDragon());

    // 11. Targeting assist
    m_targeting->Update(deltaTime, m_player->Position, false);
}

void GameApp::UpdateFrame() {
#ifndef __EMSCRIPTEN__
    float deltaTime = m_clock.restart().asSeconds();
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    m_frameCount++;
    if (m_fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
        m_currentFPS = m_frameCount;
        m_frameCount = 0;
        m_fpsClock.restart();
    }

    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) m_window.close();
        if (event.type == sf::Event::LostFocus && m_inputManager) m_inputManager->SetGamePaused(true);
        if (event.type == sf::Event::MouseWheelScrolled && m_player) {
            m_player->ProcessMouseScroll(event.mouseWheelScroll.delta);
        }
        if (event.type == sf::Event::KeyPressed && m_inputManager && m_player) {
            m_inputManager->HandleKeyPress(event.key.code, *m_player, *m_inventory,
                                          *m_buildingSystem, *m_structureSystem,
                                          *m_itemDropSystem, *m_skinningSystem,
                                          *m_spellSystem, *m_mobManager,
                                          *m_horrorProps, *m_damageNumbers,
                                          *m_particles, *m_scentSystem);
        }
    }

    sf::Vector2i mPos = sf::Mouse::getPosition(m_window);
    float curMouseX = (float)mPos.x;
    float curMouseY = (float)mPos.y;
    bool leftIsPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool rightIsPressed = sf::Mouse::isButtonPressed(sf::Mouse::Right);
    bool hasFocus = m_window.hasFocus();
#else
    static double lastFrameTime = emscripten_get_now() / 1000.0;
    static double lastFpsTime = lastFrameTime;
    double currentNow = emscripten_get_now() / 1000.0;
    float deltaTime = (float)(currentNow - lastFrameTime);
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    lastFrameTime = currentNow;

    m_frameCount++;
    if (currentNow - lastFpsTime >= 1.0) {
        m_currentFPS = m_frameCount;
        m_frameCount = 0;
        lastFpsTime = currentNow;
    }

    glfwPollEvents();
    double mx = 0, my = 0;
    glfwGetCursorPos(m_window, &mx, &my);
    float curMouseX = (float)mx;
    float curMouseY = (float)my;
    bool leftIsPressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool rightIsPressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    bool hasFocus = (glfwGetWindowAttrib(m_window, GLFW_FOCUSED) != 0);
#endif

    // Player keyboard movement (WASD)
    if (!m_inputManager->IsGamePaused()) {
        m_player->ProcessKeyboard(0, deltaTime, *m_chunkManager, *m_footprints);
        m_player->Update(deltaTime);
        m_footprints->Update(deltaTime);
    }

    // Shovel Raycast / Terraforming Target calculation
    glm::vec3 rayOrigin = m_player->GetCameraPosition();
    glm::vec3 rayDir = m_player->Front;
    m_hasTerraTarget = false;
    for (float t = 1.0f; t < 18.0f; t += 0.4f) {
        glm::vec3 testPt = rayOrigin + rayDir * t;
        float h = WorldGenerator::GetHeight(testPt.x, testPt.z);
        if (testPt.y <= h + 0.15f) {
            m_terraTarget = glm::vec3(testPt.x, h, testPt.z);
            m_hasTerraTarget = true;
            break;
        }
    }

    // Build Position Preview calculation
    glm::vec3 forwardNoY = glm::normalize(glm::vec3(m_player->Front.x, 0.0f, m_player->Front.z));
    glm::vec3 rawPos = m_player->Position + forwardNoY * 5.0f;
    float snapSize = 2.0f;
    float bx = round(rawPos.x / snapSize) * snapSize;
    float bz = round(rawPos.z / snapSize) * snapSize;
    float by = WorldGenerator::GetHeight(bx, bz);
    m_buildPos = glm::vec3(bx, by, bz);

    // Mouse, Camera Look & Combat Actions
    m_inputManager->UpdateMouseAndLook(deltaTime, curMouseX, curMouseY,
                                      leftIsPressed, rightIsPressed,
                                      m_windowWidth, m_windowHeight, hasFocus,
                                      *m_player, *m_inventory, *m_buildingSystem,
                                      *m_structureSystem, *m_itemDropSystem,
                                      *m_targeting, *m_projectiles, *m_mobManager,
                                      *m_horrorProps, *m_damageNumbers, *m_particles,
                                      *m_chunkManager, m_dayCycleTime,
                                      m_terraTarget, m_hasTerraTarget, m_buildPos);

    // Continuous dynamic terrain deformation (Pala G / H)
    bool isDigging = m_inputManager->IsShovelMode() && (PlatformInput::IsKeyPressed(PlatformInput::G) || PlatformInput::IsKeyPressed(PlatformInput::Num3));
    bool isBuilding = m_inputManager->IsShovelMode() && (PlatformInput::IsKeyPressed(PlatformInput::H) || PlatformInput::IsKeyPressed(PlatformInput::Num4) || (rightIsPressed && !m_player->IsThirdPerson));
    if (m_inputManager->IsShovelMode() && m_hasTerraTarget && (isDigging || isBuilding)) {
        float deltaH = (isBuilding ? +1.0f : -1.0f) * 3.8f * deltaTime;
        m_chunkManager->ModifyTerrain(m_terraTarget.x, m_terraTarget.z, 3.6f, deltaH, m_particles.get());
    }

    // Update Subsystems logic
    updateGameLogic(deltaTime);
    m_globalTime += deltaTime;

    // --- RENDER PASSES ---
    // Pass 1: 3D Scene to Low-Res FBO
    m_renderPipeline->RenderScene3D(deltaTime, m_globalTime, m_dayCycleTime,
                                   *m_player, *m_chunkManager, *m_buildingSystem,
                                   *m_structureSystem, *m_itemDropSystem,
                                   *m_horrorProps, *m_projectiles, *m_targeting,
                                   *m_mobManager, *m_weatherSystem, *m_damageNumbers,
                                   *m_particles, *m_footprints, *m_inputManager,
                                   m_windSystem->GetDirection(),
                                   m_terraTarget, m_hasTerraTarget, m_buildPos);

    // Pass 2: Post-Processing Retro Scaling
    m_renderPipeline->RenderPostProcess(m_windowWidth, m_windowHeight);

    // Pass 3: 2D Retro UI Overlay
    m_renderPipeline->RenderUI2D(*m_player, *m_inventory, *m_targeting,
                                 *m_damageNumbers, *m_weatherSystem,
                                 *m_mobManager, *m_inputManager,
                                 m_globalTime, m_currentFPS);

#ifndef __EMSCRIPTEN__
    m_window.display();
#else
    glfwSwapBuffers(m_window);
#endif
}

void GameApp::Run() {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop([]() {
        if (GameApp::GetInstance()) {
            GameApp::GetInstance()->UpdateFrame();
        }
    }, 0, 1);
#else
    while (m_window.isOpen()) {
        UpdateFrame();
    }
#endif
}
