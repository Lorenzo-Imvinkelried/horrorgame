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
#include "inventory/ItemRegistry.h"
#include "RenderPipeline.h"
#include "InputManager.h"
#include "WorldGenerator.h"
#include "Config.h"
#include "core/PlatformInput.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

static double s_webMouseX = 640.0;
static double s_webMouseY = 360.0;
static bool s_webLeftPressed = false;
static bool s_webRightPressed = false;
static bool s_webHasPointerLock = false;

static EM_BOOL onWebMouseMove(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    if (s_webHasPointerLock) {
        s_webMouseX += (double)e->movementX;
        s_webMouseY += (double)e->movementY;
    } else {
        s_webMouseX = (double)e->targetX;
        s_webMouseY = (double)e->targetY;
    }
    PlatformInput::s_WebMousePos = glm::vec2((float)s_webMouseX, (float)s_webMouseY);
    return EM_FALSE;
}

static EM_BOOL onWebMouseDown(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    if (e->button == 0) {
        s_webLeftPressed = true;
        PlatformInput::s_WebLeftPressed = true;
    }
    if (e->button == 2) {
        s_webRightPressed = true;
        PlatformInput::s_WebRightPressed = true;
    }
    return EM_FALSE;
}

static EM_BOOL onWebMouseUp(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    if (e->button == 0) {
        s_webLeftPressed = false;
        PlatformInput::s_WebLeftPressed = false;
    }
    if (e->button == 2) {
        s_webRightPressed = false;
        PlatformInput::s_WebRightPressed = false;
    }
    return EM_FALSE;
}

static EM_BOOL onWebPointerLockChange(int eventType, const EmscriptenPointerlockChangeEvent* e, void* userData) {
    s_webHasPointerLock = (e->isActive != 0);
    if (GameApp::GetInstance() && GameApp::GetInstance()->GetInputManager()) {
        GameApp::GetInstance()->GetInputManager()->ResetMouse();
    }
    return EM_FALSE;
}
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

    double screenW = 0, screenH = 0;
    emscripten_get_element_css_size("#canvas", &screenW, &screenH);
    if (screenW < 320 || screenH < 240) {
        emscripten_get_element_css_size("#game-container", &screenW, &screenH);
    }
    if (screenW < 320 || screenH < 240) {
        screenW = 1280;
        screenH = 720;
    }
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

    emscripten_set_canvas_element_size("#canvas", m_windowWidth, m_windowHeight);
    s_webMouseX = (double)m_windowWidth * 0.5;
    s_webMouseY = (double)m_windowHeight * 0.5;
    PlatformInput::s_WebMousePos = glm::vec2((float)s_webMouseX, (float)s_webMouseY);

    emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, onWebMouseMove);
    emscripten_set_mousedown_callback("#canvas", nullptr, false, onWebMouseDown);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, onWebMouseUp);
    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, false, onWebPointerLockChange);

    glfwSetKeyCallback(m_window, [](GLFWwindow*, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS && GameApp::GetInstance()) {
            GameApp* app = GameApp::GetInstance();
            if (app->m_inputManager && app->m_player) {
                app->m_inputManager->HandleKeyPress(key, *app->m_player, *app->m_inventory,
                                                   *app->m_buildingSystem, *app->m_structureSystem,
                                                   *app->m_itemDropSystem, *app->m_skinningSystem,
                                                   *app->m_spellSystem, *app->m_mobManager,
                                                   *app->m_horrorProps, *app->m_damageNumbers,
                                                   *app->m_particles, *app->m_scentSystem,
                                                   *app->m_targeting, *app->m_projectiles);
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
    float initAspect = (float)m_windowWidth / (float)m_windowHeight;
    int initBaseH = Config::Graphics::InternalHeight;
    int initBaseW = (int)round((float)initBaseH * initAspect);
    if (initBaseW % 2 != 0) initBaseW++;
    if (!m_renderPipeline->Init(initBaseW, initBaseH)) {
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

    loadConfigFile();
    initWorld();
    return true;
}

void GameApp::loadConfigFile() {
    std::vector<std::string> paths = {
        "config.json",
        "../config.json",
        "../../config.json",
        "bin/config.json"
    };

    std::string foundPath = "";
    for (const auto& p : paths) {
        std::ifstream file(p);
        if (file.is_open()) {
            foundPath = p;
            file.close();
            break;
        }
    }

    if (foundPath.empty()) {
        std::cout << "[Config] config.json no encontrado, usando valores por defecto." << std::endl;
        return;
    }

    std::ifstream file(foundPath);
    if (!file.is_open()) return;

    std::cout << "[Config] Cargando configuracion desde: " << foundPath << std::endl;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("initialWeather") != std::string::npos || line.find("weather") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), '\"'), valStr.end());
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ' '), valStr.end());
                valStr.erase(std::remove(valStr.begin(), valStr.end(), '\r'), valStr.end());
                valStr.erase(std::remove(valStr.begin(), valStr.end(), '\n'), valStr.end());
                if (m_weatherSystem && !valStr.empty()) {
                    m_weatherSystem->SetStateFromString(valStr);
                    std::cout << "[Config] Clima inicial configurado: " << m_weatherSystem->GetWeatherName() << " (" << valStr << ")" << std::endl;
                }
            }
        }
        if (line.find("isNight") != std::string::npos) {
            if (line.find("true") != std::string::npos) {
                m_dayCycleTime = 135.0f; // Night time
            } else if (line.find("false") != std::string::npos) {
                m_dayCycleTime = 25.0f; // Day time
            }
        }
        if (line.find("flashlightEnabled") != std::string::npos && m_player) {
            if (line.find("true") != std::string::npos) {
                m_player->HasTorchActive = true;
            } else if (line.find("false") != std::string::npos) {
                m_player->HasTorchActive = false;
            }
        }
        if (line.find("baseFreqX") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::Terrain::BaseFreqX;
            }
        }
        if (line.find("baseFreqZ") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::Terrain::BaseFreqZ;
            }
        }
        if (line.find("baseAmplitude") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::Terrain::BaseAmplitude;
            }
        }
        if (line.find("detailFreqX") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::Terrain::DetailFreqX;
            }
        }
        if (line.find("detailFreqZ") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::Terrain::DetailFreqZ;
            }
        }
        if (line.find("detailAmplitude") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::Terrain::DetailAmplitude;
            }
        }
        if (line.find("fogDensity") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::World::FogDensity;
                std::cout << "[Config] Densidad de niebla configurada: " << Config::World::FogDensity << std::endl;
            }
        }
        if (line.find("fogDistStart") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::World::FogDistStart;
            }
        }
        if (line.find("fogDistEnd") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::World::FogDistEnd;
            }
        }
        if (line.find("startingStatPoints") != std::string::npos || line.find("statPoints") != std::string::npos || line.find("initialStatPoints") != std::string::npos) {
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string valStr = line.substr(colon + 1);
                valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                std::stringstream ss(valStr);
                ss >> Config::Gameplay::StartingStatPoints;
                std::cout << "[Config] Puntos de estadisticas iniciales: " << Config::Gameplay::StartingStatPoints << std::endl;
            }
        }
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
        if (lowerLine.find("spawnmobs") != std::string::npos ||
            lowerLine.find("spanmobs") != std::string::npos ||
            lowerLine.find("spawn_mobs") != std::string::npos ||
            lowerLine.find("mobsenabled") != std::string::npos ||
            lowerLine.find("enablemobs") != std::string::npos) {
            if (lowerLine.find("false") != std::string::npos || lowerLine.find(": false") != std::string::npos) {
                Config::Gameplay::SpawnMobs = false;
                std::cout << "[Config] Spawning de Mobs DESACTIVADO (Modo testing)" << std::endl;
            } else if (lowerLine.find("true") != std::string::npos || lowerLine.find(": true") != std::string::npos) {
                Config::Gameplay::SpawnMobs = true;
                std::cout << "[Config] Spawning de Mobs ACTIVADO" << std::endl;
            }
        }
    }
}

void GameApp::initWorld() {
    float px = (float)(rand() % 160 - 80);
    float pz = (float)(rand() % 160 - 80);
    float py = WorldGenerator::GetHeight(px, pz) + 1.0f;
    m_player = std::make_unique<Player>(glm::vec3(px, py, pz));
    m_player->Stats.AvailableStatPoints = Config::Gameplay::StartingStatPoints;

    if (m_structureSystem) {
        m_structureSystem->GenerateStructures(m_player->Position);
    }

    m_chunkManager->SetBirdSystem(&m_mobManager->GetBirds());
    m_chunkManager->Init();
    m_chunkManager->Update(m_player->Position);

    m_mobManager->Init(m_player->Position);
    if (m_structureSystem) {
        m_mobManager->SpawnTowerGuards(m_structureSystem->GetTowerGuardSpawns());
    }
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

    // 4. Day / Night cycle progression (240s per day)
    m_dayCycleTime += deltaTime;
    if (m_dayCycleTime >= 240.0f) {
        m_dayCycleTime = fmod(m_dayCycleTime, 240.0f);
    }

    // 5. Buildings & Structures
    m_buildingSystem->Update(deltaTime, m_player->Position, *m_particles);
    glm::vec3 bPush(0.0f);
    m_buildingSystem->CheckCollision(m_player->Position, m_player->PlayerRadius, m_player->PlayerHeight, bPush);
    if (m_structureSystem) {
        m_structureSystem->CheckCollision(m_player->Position, m_player->PlayerRadius, m_player->PlayerHeight, m_player->Velocity);
    }

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
                         m_weatherSystem->IsBloodMoon(), &m_inputManager->GetFatalError(),
                         m_targeting.get());

    // 9. Particles & Floating Numbers
    m_particles->Update(deltaTime);
    m_damageNumbers->Update(deltaTime);

    // 10. Melee Combat Check
    m_player->UpdateCombat(deltaTime, m_mobManager->GetMonsters(), m_mobManager->GetPassiveMobs(),
                           m_mobManager->GetEnemyMobs(), m_mobManager->GetWaterMonsters(),
                           *m_particles, *m_damageNumbers, &m_mobManager->GetDragon(),
                           &m_mobManager->GetBaseMobs());

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
        if (event.type == sf::Event::Resized) {
            m_windowWidth = event.size.width;
            m_windowHeight = event.size.height;
            if (m_renderPipeline && m_windowWidth > 0 && m_windowHeight > 0) {
                float aspect = (float)m_windowWidth / (float)m_windowHeight;
                int baseH = Config::Graphics::InternalHeight;
                int baseW = (int)round((float)baseH * aspect);
                if (baseW % 2 != 0) baseW++;
                m_renderPipeline->ResizeFBO(baseW, baseH);
            }
        }
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
                                          *m_particles, *m_scentSystem,
                                          *m_targeting, *m_projectiles);
        }
    }

    sf::Vector2i mPos = sf::Mouse::getPosition(m_window);
    float curMouseX = (float)mPos.x;
    float curMouseY = (float)mPos.y;
    bool leftIsPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool rightIsPressed = sf::Mouse::isButtonPressed(sf::Mouse::Right);
    bool hasFocus = m_window.hasFocus();

    bool showSysCursor = m_player->IsThirdPerson || 
                         m_inputManager->IsCharacterPanelOpen() || 
                         m_inputManager->GetLoreModal().active || 
                         m_inputManager->GetFatalError().active || 
                         m_inventory->IsOpen() || 
                         m_inputManager->IsGamePaused() || 
                         m_player->IsDead();
    m_window.setMouseCursorVisible(showSysCursor);
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

    // Dynamically adjust canvas buffer & internal FBO aspect ratio whenever screen/window changes
    double cssW = 0, cssH = 0;
    emscripten_get_element_css_size("#canvas", &cssW, &cssH);
    int targetW = (int)cssW;
    int targetH = (int)cssH;
    if (targetW >= 320 && targetH >= 240 && (targetW != m_windowWidth || targetH != m_windowHeight)) {
        m_windowWidth = targetW;
        m_windowHeight = targetH;
        emscripten_set_canvas_element_size("#canvas", m_windowWidth, m_windowHeight);
        glfwSetWindowSize(m_window, m_windowWidth, m_windowHeight);

        if (m_renderPipeline) {
            float aspect = (float)m_windowWidth / (float)m_windowHeight;
            int baseH = Config::Graphics::InternalHeight;
            int baseW = (int)round((float)baseH * aspect);
            if (baseW % 2 != 0) baseW++;
            m_renderPipeline->ResizeFBO(baseW, baseH);
        }
    }

    float curMouseX = (float)s_webMouseX;
    float curMouseY = (float)s_webMouseY;
    bool leftIsPressed = s_webLeftPressed || (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    bool rightIsPressed = s_webRightPressed || (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    bool hasFocus = true;
#endif

    // Sync equipped 3D armor & weapon visuals on player model
    if (m_player && m_inventory) {
        const auto& eq = m_inventory->GetEquipment();
        std::string mh = eq.HasEquipped(EquipSlot::MAIN_HAND) ? ItemRegistry::Get().Get(eq.GetEquipped(EquipSlot::MAIN_HAND).id).stringId : "";
        std::string ch = eq.HasEquipped(EquipSlot::CHEST) ? ItemRegistry::Get().Get(eq.GetEquipped(EquipSlot::CHEST).id).stringId : "";
        std::string hd = eq.HasEquipped(EquipSlot::HEAD) ? ItemRegistry::Get().Get(eq.GetEquipped(EquipSlot::HEAD).id).stringId : "";
        std::string oh = eq.HasEquipped(EquipSlot::OFF_HAND) ? ItemRegistry::Get().Get(eq.GetEquipped(EquipSlot::OFF_HAND).id).stringId : "";
        std::string lg = eq.HasEquipped(EquipSlot::LEGS) ? ItemRegistry::Get().Get(eq.GetEquipped(EquipSlot::LEGS).id).stringId : "";
        std::string ft = eq.HasEquipped(EquipSlot::FEET) ? ItemRegistry::Get().Get(eq.GetEquipped(EquipSlot::FEET).id).stringId : "";
        std::string gl = eq.HasEquipped(EquipSlot::GLOVES) ? ItemRegistry::Get().Get(eq.GetEquipped(EquipSlot::GLOVES).id).stringId : "";
        m_player->UpdateEquipmentVisuals(mh, ch, hd, oh, lg, ft, gl);
    }

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
                                 m_globalTime, m_currentFPS,
                                 *m_spellSystem, *m_structureSystem,
                                 *m_itemDropSystem, *m_skinningSystem,
                                 *m_horrorProps);

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
